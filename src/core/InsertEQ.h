/*
    InsertEQ.h - eight-band musical EQ for the mixer insert rack.

    The band layout and filter topologies are adapted from Common Sense EQ
    (GPL-3.0-or-later) so the insert matches a familiar console-style curve:
    low cut, low shelf, four bells, high shelf and high cut, with selectable
    cut slopes.

    Pure C++ on purpose: the response helpers are shared by the audio path and
    the editor, and the core test binary can exercise the DSP without JUCE.
*/

#pragma once

#include <array>
#include <cmath>
#include <cstddef>

namespace glorp
{

enum class EqBandShape
{
    lowCut = 0,
    lowShelf,
    bell,
    highShelf,
    highCut
};

struct EqBandSettings
{
    bool on = true;
    float freq = 1000.0f;
    float gainDb = 0.0f;
    float q = 1.0f;
    int slope = 24;
};

struct InsertEqSettings
{
    std::array<EqBandSettings, 8> bands {};
    float outputGainDb = 0.0f;
};

constexpr int eqBandCount = 8;

EqBandShape eqBandShape(int band) noexcept;
bool eqBandHasGain(int band) noexcept;
bool eqBandIsCut(int band) noexcept;
float eqDefaultFreq(int band) noexcept;
int eqDefaultSlope(int band) noexcept;

/// One RBJ biquad section used by every band type.
struct EqBiquad
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;

    void identity() noexcept;
    void setPeaking(float f0, float gainDb, float q, double sampleRate) noexcept;
    void setLowShelf(float f0, float gainDb, float q, double sampleRate) noexcept;
    void setHighShelf(float f0, float gainDb, float q, double sampleRate) noexcept;
    void setFirstOrderLowPass(float f0, double sampleRate) noexcept;
    void setFirstOrderHighPass(float f0, double sampleRate) noexcept;
    void setHighPass(float f0, float q, double sampleRate) noexcept;
    void setLowPass(float f0, float q, double sampleRate) noexcept;
    float magnitudeDb(float freq, double sampleRate) const noexcept;
};

/// Magnitude response of one band in dB, matching the audio path.
float eqBandMagnitudeDbForIndex(int index, const EqBandSettings &band, float freq, double sampleRate) noexcept;

/// Summed response of every enabled band (without output gain).
float eqTotalMagnitudeDb(const InsertEqSettings &settings, float freq, double sampleRate) noexcept;

class InsertEq
{
  public:
    void prepare(double sampleRate, int maxBlockSize) noexcept;
    void reset() noexcept;
    void setSettings(const InsertEqSettings &settings) noexcept;

    void process(float *left, float *right, int numSamples) noexcept;

  private:
    struct BiquadState
    {
        float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;

        void reset() noexcept;
        float process(float x, const EqBiquad &c) noexcept;
    };

    struct BandRuntime
    {
        std::array<EqBiquad, 3> sections {};
        std::array<BiquadState, 3> left {};
        std::array<BiquadState, 3> right {};
        int sectionCount = 0;
        bool active = false;
    };

    void updateBand(int index, const EqBandSettings &settings) noexcept;

    double sr = 48000.0;
    std::array<BandRuntime, eqBandCount> bands {};
    InsertEqSettings current {};
    bool hasCurrent = false;
    bool anyActive = false;
    float outputGain = 1.0f;
};

/// Fills a settings block with the factory default bands.
InsertEqSettings defaultInsertEqSettings() noexcept;

} // namespace glorp
