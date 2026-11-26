#include "InsertRack.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace glorp
{
namespace
{
float dbToGain(float db) noexcept { return std::pow(10.0f, db / 20.0f); }

const std::array<InsertParamInfo, maxInsertParams> &paramArrayFor(InsertType type)
{
    static const auto eq = []
    {
        std::array<InsertParamInfo, maxInsertParams> params {};

        static const char *onNames[eqBandCount] {"LC ON", "LS ON", "B1 ON", "B2 ON",
                                                 "B3 ON", "B4 ON", "HS ON", "HC ON"};
        static const char *freqNames[eqBandCount] {"LC FREQ", "LS FREQ", "B1 FREQ", "B2 FREQ",
                                                   "B3 FREQ", "B4 FREQ", "HS FREQ", "HC FREQ"};
        static const char *gainNames[eqBandCount] {"LC GAIN", "LS GAIN", "B1 GAIN", "B2 GAIN",
                                                   "B3 GAIN", "B4 GAIN", "HS GAIN", "HC GAIN"};
        static const char *qNames[eqBandCount] {"LC Q", "LS Q", "B1 Q", "B2 Q",
                                                "B3 Q", "B4 Q", "HS Q", "HC Q"};
        static const char *slopeNames[eqBandCount] {"LC SLOPE", "LS SLOPE", "B1 SLOPE", "B2 SLOPE",
                                                    "B3 SLOPE", "B4 SLOPE", "HS SLOPE", "HC SLOPE"};

        for (int band = 0; band < eqBandCount; ++band)
        {
            const auto i = static_cast<size_t>(band);
            const bool cut = eqBandIsCut(band);
            params[static_cast<size_t>(eqParamOn(band))] = {onNames[i], 0.0f, 1.0f, 1.0f, ""};
            params[static_cast<size_t>(eqParamFreq(band))] = {freqNames[i], 20.0f, 20000.0f,
                                                              eqDefaultFreq(band), "Hz"};
            params[static_cast<size_t>(eqParamGain(band))] = {gainNames[i], -18.0f, 18.0f, 0.0f, "dB"};
            params[static_cast<size_t>(eqParamQ(band))] = {qNames[i], 0.1f, 18.0f, 1.0f, "Q"};
            params[static_cast<size_t>(eqParamSlope(band))] = {slopeNames[i], 6.0f, 30.0f,
                                                               static_cast<float>(eqDefaultSlope(band)),
                                                               "dB/oct"};
            juce::ignoreUnused(cut);
        }

        params[static_cast<size_t>(eqParamOutputGain)] = {"OUT", -18.0f, 18.0f, 0.0f, "dB"};
        return params;
    }();

    static const std::array<InsertParamInfo, maxInsertParams> comp {{
        {"THRESH", -40.0f, 0.0f, -14.0f, "dB"},
        {"RATIO", 1.0f, 20.0f, 3.0f, ":1"},
        {"ATTACK", 0.5f, 50.0f, 15.0f, "ms"},
        {"RELEASE", 20.0f, 500.0f, 100.0f, "ms"},
        {"MAKEUP", 0.0f, 24.0f, 0.0f, "dB"},
    }};

    static const std::array<InsertParamInfo, maxInsertParams> drive {{
        {"DRIVE", 0.0f, 1.0f, 0.25f, ""},
        {"OUTPUT", -12.0f, 12.0f, 0.0f, "dB"},
    }};

    static const std::array<InsertParamInfo, maxInsertParams> transient {{
        {"ATTACK", -1.0f, 1.0f, 0.0f, ""},
        {"SUSTAIN", -1.0f, 1.0f, 0.0f, ""},
    }};

    static const std::array<InsertParamInfo, maxInsertParams> limiter {{
        {"CEILING", -12.0f, 0.0f, -0.3f, "dB"},
        {"RELEASE", 20.0f, 500.0f, 100.0f, "ms"},
    }};

    static const std::array<InsertParamInfo, maxInsertParams> clipper {{
        {"DRIVE", 0.0f, 1.0f, 0.35f, ""},
        {"CEILING", -12.0f, 0.0f, -0.5f, "dB"},
    }};

    static const std::array<InsertParamInfo, maxInsertParams> none {};

    switch (type)
    {
        case InsertType::eq: return eq;
        case InsertType::compressor: return comp;
        case InsertType::drive: return drive;
        case InsertType::transient: return transient;
        case InsertType::limiter: return limiter;
        case InsertType::clipper: return clipper;
        default: return none;
    }
}

int paramCountFor(InsertType type) noexcept
{
    switch (type)
    {
        case InsertType::eq: return eqParamSlope(eqBandCount - 1) + 1;
        case InsertType::compressor: return 5;
        case InsertType::drive: return 2;
        case InsertType::transient: return 2;
        case InsertType::limiter: return 2;
        case InsertType::clipper: return 2;
        default: return 0;
    }
}
} // namespace

const char *insertTypeName(InsertType type) noexcept
{
    switch (type)
    {
        case InsertType::eq: return "EQ";
        case InsertType::compressor: return "COMPRESSOR";
        case InsertType::drive: return "DRIVE";
        case InsertType::transient: return "TRANSIENT";
        case InsertType::limiter: return "LIMITER";
        case InsertType::clipper: return "CLIPPER";
        default: return "EMPTY";
    }
}

int insertParamCount(InsertType type) noexcept
{
    return paramCountFor(type);
}

const InsertParamInfo &insertParamInfo(InsertType type, int index) noexcept
{
    static const InsertParamInfo fallback {};
    if (index < 0 || index >= paramCountFor(type))
        return fallback;
    return paramArrayFor(type)[static_cast<size_t>(index)];
}

InsertEqSettings insertEqSettingsFromValues(const std::array<float, maxInsertParams> &values) noexcept
{
    InsertEqSettings settings;
    for (int band = 0; band < eqBandCount; ++band)
    {
        auto &b = settings.bands[static_cast<size_t>(band)];
        b.on = values[static_cast<size_t>(eqParamOn(band))] >= 0.5f;
        b.freq = values[static_cast<size_t>(eqParamFreq(band))];
        b.gainDb = values[static_cast<size_t>(eqParamGain(band))];
        b.q = values[static_cast<size_t>(eqParamQ(band))];
        b.slope = static_cast<int>(std::lround(values[static_cast<size_t>(eqParamSlope(band))]));
    }
    settings.outputGainDb = values[static_cast<size_t>(eqParamOutputGain)];
    return settings;
}

void LimiterProcessor::prepare(double sampleRate, int)
{
    sr = std::max(1.0, sampleRate);
    reset();
}

void LimiterProcessor::reset()
{
    envelope = 1.0f;
}

void LimiterProcessor::process(float *left, float *right, int numSamples, float ceilingDb, float releaseMs)
{
    const float ceiling = dbToGain(std::min(0.0f, ceilingDb));
    const float attack = 1.0f - std::exp(-1.0f / static_cast<float>(sr * 0.0005));
    const float release = 1.0f - std::exp(-1.0f / static_cast<float>(sr * std::max(1.0f, releaseMs) / 1000.0));

    for (int i = 0; i < numSamples; ++i)
    {
        const float peak = std::max(std::abs(left[i]), std::abs(right[i]));
        const float target = peak > ceiling ? ceiling / peak : 1.0f;
        const float coeff = target < envelope ? attack : release;
        envelope += (target - envelope) * coeff;
        envelope = std::clamp(envelope, 0.0f, 1.0f);
        left[i] = std::clamp(left[i] * envelope, -ceiling, ceiling);
        right[i] = std::clamp(right[i] * envelope, -ceiling, ceiling);
    }
}

void ClipperProcessor::prepare(double sampleRate, int)
{
    sr = std::max(1.0, sampleRate);
}

void ClipperProcessor::reset() {}

void ClipperProcessor::process(float *left, float *right, int numSamples, float drive, float ceilingDb)
{
    const float gain = dbToGain(drive * 24.0f);
    const float ceiling = dbToGain(std::min(0.0f, ceilingDb));
    for (int i = 0; i < numSamples; ++i)
    {
        left[i] = std::tanh(left[i] * gain / ceiling) * ceiling;
        right[i] = std::tanh(right[i] * gain / ceiling) * ceiling;
    }
}

void InsertSlot::prepare(double sampleRate, int maxBlockSize)
{
    fx.prepare(sampleRate, maxBlockSize);
    eq.prepare(sampleRate, maxBlockSize);
    limiter.prepare(sampleRate, maxBlockSize);
    clipper.prepare(sampleRate, maxBlockSize);
    prepared = true;
}

void InsertSlot::reset()
{
    fx.reset();
    eq.reset();
    limiter.reset();
}

InsertSlotState InsertSlot::state() const noexcept
{
    return {type, bypass, gainReductionDb, values};
}

void InsertSlot::setType(InsertType newType)
{
    type = newType;
    bypass = false;
    values.fill(0.0f);

    if (type == InsertType::eq)
    {
        const auto defaults = defaultInsertEqSettings();
        for (int band = 0; band < eqBandCount; ++band)
        {
            const auto &b = defaults.bands[static_cast<size_t>(band)];
            values[static_cast<size_t>(eqParamOn(band))] = b.on ? 1.0f : 0.0f;
            values[static_cast<size_t>(eqParamFreq(band))] = b.freq;
            values[static_cast<size_t>(eqParamGain(band))] = b.gainDb;
            values[static_cast<size_t>(eqParamQ(band))] = b.q;
            values[static_cast<size_t>(eqParamSlope(band))] = static_cast<float>(b.slope);
        }
        values[static_cast<size_t>(eqParamOutputGain)] = defaults.outputGainDb;
    }
    else
    {
        const auto count = insertParamCount(type);
        for (int i = 0; i < count; ++i)
            values[static_cast<size_t>(i)] = insertParamInfo(type, i).defaultValue;
    }

    fx.reset();
    eq.reset();
    limiter.reset();
}

void InsertSlot::process(float *left, float *right, int numSamples)
{
    if (!prepared || type == InsertType::none || bypass || numSamples <= 0)
    {
        gainReductionDb = 0.0f;
        return;
    }

    switch (type)
    {
        case InsertType::eq:
        {
            eq.setSettings(insertEqSettingsFromValues(values));
            eq.process(left, right, numSamples);
            break;
        }
        case InsertType::compressor:
        {
            MicStripParams p;
            p.eqEnabled = false;
            p.compEnabled = true;
            p.compThresholdDb = values[0];
            p.compRatio = values[1];
            p.compAttackMs = values[2];
            p.compReleaseMs = values[3];
            p.compMakeupDb = values[4];
            p.saturationDrive = 0.0f;
            p.transientAttack = 0.0f;
            p.transientSustain = 0.0f;

            float inPeak = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                inPeak = std::max(inPeak, std::max(std::abs(left[i]), std::abs(right[i])));

            fx.process(left, right, numSamples, p, nullptr, nullptr);

            float outPeak = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                outPeak = std::max(outPeak, std::max(std::abs(left[i]), std::abs(right[i])));

            // outPeak includes the make-up gain, so add it back to report the
            // gain reduction the compressor actually applied.
            if (inPeak > 1.0e-5f && outPeak > 1.0e-9f)
                gainReductionDb = std::clamp(20.0f * std::log10(inPeak / outPeak) + values[4],
                                             0.0f, 30.0f);
            else
                gainReductionDb = 0.0f;
            break;
        }
        case InsertType::drive:
        {
            MicStripParams p;
            p.eqEnabled = false;
            p.compEnabled = false;
            p.saturationDrive = std::clamp(values[0], 0.0f, 1.0f);
            p.transientAttack = 0.0f;
            p.transientSustain = 0.0f;
            fx.process(left, right, numSamples, p, nullptr, nullptr);
            const float out = dbToGain(values[1]);
            juce::FloatVectorOperations::multiply(left, out, numSamples);
            juce::FloatVectorOperations::multiply(right, out, numSamples);
            break;
        }
        case InsertType::transient:
        {
            MicStripParams p;
            p.eqEnabled = false;
            p.compEnabled = false;
            p.saturationDrive = 0.0f;
            p.transientAttack = std::clamp(values[0], -1.0f, 1.0f);
            p.transientSustain = std::clamp(values[1], -1.0f, 1.0f);
            fx.process(left, right, numSamples, p, nullptr, nullptr);
            break;
        }
        case InsertType::limiter:
            limiter.process(left, right, numSamples, values[0], values[1]);
            break;
        case InsertType::clipper:
            clipper.process(left, right, numSamples, values[0], values[1]);
            break;
        default:
            break;
    }
}

InsertChain::InsertChain()
{
    for (auto &s : slots)
        s = std::make_unique<InsertSlot>();
}

void InsertChain::prepare(double sampleRate, int maxBlockSize)
{
    for (auto &s : slots)
        s->prepare(sampleRate, maxBlockSize);
}

void InsertChain::reset()
{
    for (auto &s : slots)
        s->reset();
}

void InsertChain::process(float *left, float *right, int numSamples)
{
    for (auto &s : slots)
        s->process(left, right, numSamples);
}

InsertSlot &InsertChain::slot(int index) noexcept
{
    return *slots[static_cast<size_t>(std::clamp(index, 0, slotCount - 1))];
}

const InsertSlot &InsertChain::slot(int index) const noexcept
{
    return *slots[static_cast<size_t>(std::clamp(index, 0, slotCount - 1))];
}

void InsertChain::moveSlot(int from, int to)
{
    from = std::clamp(from, 0, slotCount - 1);
    to = std::clamp(to, 0, slotCount - 1);
    if (from == to)
        return;

    auto moving = std::move(slots[static_cast<size_t>(from)]);
    if (from < to)
        for (int i = from; i < to; ++i)
            slots[static_cast<size_t>(i)] = std::move(slots[static_cast<size_t>(i + 1)]);
    else
        for (int i = from; i > to; --i)
            slots[static_cast<size_t>(i)] = std::move(slots[static_cast<size_t>(i - 1)]);
    slots[static_cast<size_t>(to)] = std::move(moving);
}

bool InsertChain::anyActive() const noexcept
{
    for (const auto &s : slots)
        if (s->type != InsertType::none && !s->bypass)
            return true;
    return false;
}

} // namespace glorp
