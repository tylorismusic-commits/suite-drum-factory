#include "InsertEQ.h"

#include <algorithm>
#include <complex>

namespace glorp
{
namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr double twoPi = pi * 2.0;

float clampF(float value, float minimum, float maximum) noexcept
{
    return std::min(std::max(value, minimum), maximum);
}

float gainToDb(float gain) noexcept
{
    return 20.0f * std::log10(std::max(gain, 1.0e-12f));
}

float dbToGain(float db) noexcept
{
    return std::pow(10.0f, db / 20.0f);
}

int slopeSections(int slope) noexcept
{
    switch (slope)
    {
        case 6: return 0;
        case 12: return 1;
        case 18: return 1;
        case 24: return 2;
        case 30: return 2;
        default: return 2;
    }
}

bool slopeUsesFirstOrder(int slope) noexcept
{
    return slope == 6 || slope == 18 || slope == 30;
}
} // namespace

EqBandShape eqBandShape(int band) noexcept
{
    static constexpr EqBandShape shapes[eqBandCount] {
        EqBandShape::lowCut, EqBandShape::lowShelf, EqBandShape::bell,  EqBandShape::bell,
        EqBandShape::bell,   EqBandShape::bell,     EqBandShape::highShelf, EqBandShape::highCut};
    return shapes[static_cast<size_t>(std::clamp(band, 0, eqBandCount - 1))];
}

bool eqBandHasGain(int band) noexcept
{
    const auto shape = eqBandShape(band);
    return shape == EqBandShape::lowShelf || shape == EqBandShape::bell || shape == EqBandShape::highShelf;
}

bool eqBandIsCut(int band) noexcept
{
    const auto shape = eqBandShape(band);
    return shape == EqBandShape::lowCut || shape == EqBandShape::highCut;
}

float eqDefaultFreq(int band) noexcept
{
    static constexpr float freqs[eqBandCount] {20.0f, 75.0f, 100.0f, 270.0f, 750.0f, 2500.0f, 4600.0f, 20000.0f};
    return freqs[static_cast<size_t>(std::clamp(band, 0, eqBandCount - 1))];
}

int eqDefaultSlope(int band) noexcept
{
    static_cast<void>(band);
    return 24;
}

InsertEqSettings defaultInsertEqSettings() noexcept
{
    InsertEqSettings settings;
    for (int i = 0; i < eqBandCount; ++i)
    {
        settings.bands[static_cast<size_t>(i)].on = true;
        settings.bands[static_cast<size_t>(i)].freq = eqDefaultFreq(i);
        settings.bands[static_cast<size_t>(i)].gainDb = 0.0f;
        settings.bands[static_cast<size_t>(i)].q = 1.0f;
        settings.bands[static_cast<size_t>(i)].slope = eqDefaultSlope(i);
    }
    settings.outputGainDb = 0.0f;
    return settings;
}

//==============================================================================
void EqBiquad::identity() noexcept
{
    b0 = 1.0f;
    b1 = b2 = a1 = a2 = 0.0f;
}

void EqBiquad::setPeaking(float f0, float gainDb, float q, double sampleRate) noexcept
{
    if (std::abs(gainDb) < 1.0e-5f)
    {
        identity();
        return;
    }

    const double sr = std::max(1.0, sampleRate);
    const double w0 = twoPi * std::clamp(static_cast<double>(f0), 5.0, sr * 0.49) / sr;
    const double cosw = std::cos(w0);
    const double alpha = std::sin(w0) / (2.0 * std::max(0.05, static_cast<double>(q)));
    const double a = std::pow(10.0, static_cast<double>(gainDb) / 40.0);

    const double a0 = 1.0 + alpha / a;
    b0 = static_cast<float>((1.0 + alpha * a) / a0);
    b1 = static_cast<float>(-2.0 * cosw / a0);
    b2 = static_cast<float>((1.0 - alpha * a) / a0);
    a1 = static_cast<float>(-2.0 * cosw / a0);
    a2 = static_cast<float>((1.0 - alpha / a) / a0);
}

void EqBiquad::setLowShelf(float f0, float gainDb, float q, double sampleRate) noexcept
{
    if (std::abs(gainDb) < 1.0e-5f)
    {
        identity();
        return;
    }

    const double sr = std::max(1.0, sampleRate);
    const double w0 = twoPi * std::clamp(static_cast<double>(f0), 5.0, sr * 0.49) / sr;
    const double cosw = std::cos(w0);
    const double sinw = std::sin(w0);
    const double alpha = sinw / (2.0 * std::max(0.05, static_cast<double>(q)));
    const double a = std::pow(10.0, static_cast<double>(gainDb) / 40.0);
    const double sqrtA = std::sqrt(a);
    const double a0 = (a + 1.0) + (a - 1.0) * cosw + 2.0 * sqrtA * alpha;

    b0 = static_cast<float>(a * ((a + 1.0) - (a - 1.0) * cosw + 2.0 * sqrtA * alpha) / a0);
    b1 = static_cast<float>(2.0 * a * ((a - 1.0) - (a + 1.0) * cosw) / a0);
    b2 = static_cast<float>(a * ((a + 1.0) - (a - 1.0) * cosw - 2.0 * sqrtA * alpha) / a0);
    a1 = static_cast<float>(-2.0 * ((a - 1.0) + (a + 1.0) * cosw) / a0);
    a2 = static_cast<float>(((a + 1.0) + (a - 1.0) * cosw - 2.0 * sqrtA * alpha) / a0);
}

void EqBiquad::setHighShelf(float f0, float gainDb, float q, double sampleRate) noexcept
{
    if (std::abs(gainDb) < 1.0e-5f)
    {
        identity();
        return;
    }

    const double sr = std::max(1.0, sampleRate);
    const double w0 = twoPi * std::clamp(static_cast<double>(f0), 5.0, sr * 0.49) / sr;
    const double cosw = std::cos(w0);
    const double sinw = std::sin(w0);
    const double alpha = sinw / (2.0 * std::max(0.05, static_cast<double>(q)));
    const double a = std::pow(10.0, static_cast<double>(gainDb) / 40.0);
    const double sqrtA = std::sqrt(a);
    const double a0 = (a + 1.0) - (a - 1.0) * cosw + 2.0 * sqrtA * alpha;

    b0 = static_cast<float>(a * ((a + 1.0) + (a - 1.0) * cosw + 2.0 * sqrtA * alpha) / a0);
    b1 = static_cast<float>(-2.0 * a * ((a - 1.0) + (a + 1.0) * cosw) / a0);
    b2 = static_cast<float>(a * ((a + 1.0) + (a - 1.0) * cosw - 2.0 * sqrtA * alpha) / a0);
    a1 = static_cast<float>(2.0 * ((a - 1.0) - (a + 1.0) * cosw) / a0);
    a2 = static_cast<float>(((a + 1.0) - (a - 1.0) * cosw - 2.0 * sqrtA * alpha) / a0);
}

void EqBiquad::setFirstOrderLowPass(float f0, double sampleRate) noexcept
{
    const double sr = std::max(1.0, sampleRate);
    const double g = std::tan(pi * std::clamp(static_cast<double>(f0), 5.0, sr * 0.49) / sr);
    const double G = g / (1.0 + g);
    b0 = static_cast<float>(G);
    b1 = static_cast<float>(G);
    b2 = 0.0f;
    a1 = static_cast<float>(2.0 * G - 1.0);
    a2 = 0.0f;
}

void EqBiquad::setFirstOrderHighPass(float f0, double sampleRate) noexcept
{
    const double sr = std::max(1.0, sampleRate);
    const double g = std::tan(pi * std::clamp(static_cast<double>(f0), 5.0, sr * 0.49) / sr);
    const double G = g / (1.0 + g);
    b0 = static_cast<float>(1.0 - G);
    b1 = static_cast<float>(-(1.0 - G));
    b2 = 0.0f;
    a1 = static_cast<float>(2.0 * G - 1.0);
    a2 = 0.0f;
}

void EqBiquad::setHighPass(float f0, float q, double sampleRate) noexcept
{
    const double sr = std::max(1.0, sampleRate);
    const double w0 = twoPi * std::clamp(static_cast<double>(f0), 5.0, sr * 0.49) / sr;
    const double cosw = std::cos(w0);
    const double alpha = std::sin(w0) / (2.0 * std::max(0.05, static_cast<double>(q)));
    const double a0 = 1.0 + alpha;

    b0 = static_cast<float>((1.0 + cosw) * 0.5 / a0);
    b1 = static_cast<float>(-(1.0 + cosw) / a0);
    b2 = b0;
    a1 = static_cast<float>(-2.0 * cosw / a0);
    a2 = static_cast<float>((1.0 - alpha) / a0);
}

void EqBiquad::setLowPass(float f0, float q, double sampleRate) noexcept
{
    const double sr = std::max(1.0, sampleRate);
    const double w0 = twoPi * std::clamp(static_cast<double>(f0), 5.0, sr * 0.49) / sr;
    const double cosw = std::cos(w0);
    const double alpha = std::sin(w0) / (2.0 * std::max(0.05, static_cast<double>(q)));
    const double a0 = 1.0 + alpha;

    b0 = static_cast<float>((1.0 - cosw) * 0.5 / a0);
    b1 = static_cast<float>((1.0 - cosw) / a0);
    b2 = b0;
    a1 = static_cast<float>(-2.0 * cosw / a0);
    a2 = static_cast<float>((1.0 - alpha) / a0);
}

float EqBiquad::magnitudeDb(float freq, double sampleRate) const noexcept
{
    const double sr = std::max(1.0, sampleRate);
    const double w = twoPi * std::max(1.0f, freq) / sr;
    const std::complex<double> z1(std::cos(w), -std::sin(w));
    const std::complex<double> z2 = z1 * z1;
    const std::complex<double> num = static_cast<double>(b0) + static_cast<double>(b1) * z1
                                     + static_cast<double>(b2) * z2;
    const std::complex<double> den = 1.0 + static_cast<double>(a1) * z1 + static_cast<double>(a2) * z2;
    return gainToDb(static_cast<float>(std::abs(num) / std::max(std::abs(den), 1.0e-12)));
}

//==============================================================================
void InsertEq::BiquadState::reset() noexcept
{
    x1 = x2 = y1 = y2 = 0.0f;
}

float InsertEq::BiquadState::process(float x, const EqBiquad &c) noexcept
{
    const float y = c.b0 * x + c.b1 * x1 + c.b2 * x2 - c.a1 * y1 - c.a2 * y2;
    x2 = x1;
    x1 = x;
    y2 = y1;
    y1 = y;
    return y;
}

void InsertEq::prepare(double sampleRate, int) noexcept
{
    sr = std::max(1.0, sampleRate);
    reset();
}

void InsertEq::reset() noexcept
{
    for (auto &band : bands)
    {
        for (auto &state : band.left)
            state.reset();
        for (auto &state : band.right)
            state.reset();
    }
}

namespace
{
void buildSections(EqBiquad *sections, int &count, int index, const EqBandSettings &band,
                   double sampleRate) noexcept
{
    count = 0;
    const auto shape = eqBandShape(index);
    const float freq = clampF(band.freq, 5.0f, static_cast<float>(sampleRate * 0.49));
    const float q = clampF(band.q, 0.05f, 25.0f);

    switch (shape)
    {
        case EqBandShape::bell:
            sections[0].setPeaking(freq, band.gainDb, q, sampleRate);
            count = 1;
            break;
        case EqBandShape::lowShelf:
            sections[0].setLowShelf(freq, band.gainDb, q, sampleRate);
            count = 1;
            break;
        case EqBandShape::highShelf:
            sections[0].setHighShelf(freq, band.gainDb, q, sampleRate);
            count = 1;
            break;
        case EqBandShape::lowCut:
        case EqBandShape::highCut:
        {
            const bool lowCut = shape == EqBandShape::lowCut;
            const bool useFirst = slopeUsesFirstOrder(band.slope);
            const int secondOrder = slopeSections(band.slope);

            if (useFirst)
            {
                if (lowCut)
                    sections[count++].setFirstOrderHighPass(freq, sampleRate);
                else
                    sections[count++].setFirstOrderLowPass(freq, sampleRate);
            }

            for (int section = 0; section < secondOrder; ++section)
            {
                if (lowCut)
                    sections[count++].setHighPass(freq, 0.70710678f, sampleRate);
                else
                    sections[count++].setLowPass(freq, 0.70710678f, sampleRate);
            }
            break;
        }
    }
}
} // namespace

void InsertEq::setSettings(const InsertEqSettings &settings) noexcept
{
    bool active = false;

    for (int i = 0; i < eqBandCount; ++i)
    {
        const auto &next = settings.bands[static_cast<size_t>(i)];
        const auto &previous = current.bands[static_cast<size_t>(i)];

        const bool changed = !hasCurrent || next.on != previous.on || next.freq != previous.freq
                             || next.gainDb != previous.gainDb || next.q != previous.q
                             || next.slope != previous.slope;
        if (changed)
            updateBand(i, next);

        current.bands[static_cast<size_t>(i)] = next;
        active = active || bands[static_cast<size_t>(i)].active;
    }

    if (!hasCurrent || settings.outputGainDb != current.outputGainDb)
        outputGain = dbToGain(settings.outputGainDb);

    current.outputGainDb = settings.outputGainDb;
    anyActive = active;
    hasCurrent = true;
}

void InsertEq::updateBand(int index, const EqBandSettings &settings) noexcept
{
    auto &band = bands[static_cast<size_t>(index)];
    const auto shape = eqBandShape(index);

    bool active = settings.on && std::isfinite(settings.freq);
    if (active && eqBandHasGain(index) && std::abs(settings.gainDb) < 1.0e-4f)
        active = false;
    if (active && shape == EqBandShape::lowCut && settings.freq <= 21.0f)
        active = false;
    if (active && shape == EqBandShape::highCut && settings.freq >= 19500.0f)
        active = false;

    int count = 0;
    if (active)
        buildSections(band.sections.data(), count, index, settings, sr);

    if (active && !band.active)
    {
        for (auto &state : band.left)
            state.reset();
        for (auto &state : band.right)
            state.reset();
    }

    band.sectionCount = count;
    band.active = active;
}

void InsertEq::process(float *left, float *right, int numSamples) noexcept
{
    if (left == nullptr || right == nullptr || numSamples <= 0)
        return;

    if (!anyActive && std::abs(outputGain - 1.0f) <= 1.0e-6f)
        return;

    for (auto &band : bands)
    {
        if (!band.active || band.sectionCount <= 0)
            continue;

        for (int section = 0; section < band.sectionCount; ++section)
        {
            auto &coeff = band.sections[static_cast<size_t>(section)];
            auto &stateL = band.left[static_cast<size_t>(section)];
            auto &stateR = band.right[static_cast<size_t>(section)];
            for (int i = 0; i < numSamples; ++i)
            {
                const float l = left[i];
                const float r = right[i];
                left[i] = stateL.process(l, coeff);
                right[i] = stateR.process(r, coeff);
            }
        }
    }

    if (std::abs(outputGain - 1.0f) > 1.0e-6f)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            left[i] *= outputGain;
            right[i] *= outputGain;
        }
    }
}

//==============================================================================
float eqBandMagnitudeDbForIndex(int index, const EqBandSettings &band, float freq, double sampleRate) noexcept
{
    if (!band.on || !std::isfinite(band.freq))
        return 0.0f;

    EqBiquad sections[3];
    int count = 0;
    buildSections(sections, count, index, band, sampleRate);

    float db = 0.0f;
    for (int i = 0; i < count; ++i)
        db += sections[i].magnitudeDb(freq, sampleRate);
    return db;
}

float eqTotalMagnitudeDb(const InsertEqSettings &settings, float freq, double sampleRate) noexcept
{
    float db = 0.0f;
    for (int i = 0; i < eqBandCount; ++i)
        db += eqBandMagnitudeDbForIndex(i, settings.bands[static_cast<size_t>(i)], freq, sampleRate);
    return db;
}

} // namespace glorp
