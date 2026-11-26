/*
    InsertRack.h - serial insert slots for the factory mixer.

    Four slots per channel. EQ, compressor, drive and transient use the existing
    ChannelStripFX one section at a time; limiter and clipper are small dedicated
    processors. The chain is deliberately allocation-free after prepare().
*/

#pragma once

#include "ChannelFX.h"
#include "InsertEQ.h"

#include <array>
#include <cstddef>
#include <memory>

namespace glorp
{

enum class InsertType
{
    none = 0,
    eq,
    compressor,
    drive,
    transient,
    limiter,
    clipper,
    count
};

/// Every insert uses one flat float array. The EQ needs 41 slots (eight bands
/// of on/freq/gain/Q, output gain and per-band cut slopes); the other effect
/// types only fill the first few.
constexpr int maxInsertParams = 48;

constexpr int eqParamOn(int band) noexcept { return band * 4; }
constexpr int eqParamFreq(int band) noexcept { return band * 4 + 1; }
constexpr int eqParamGain(int band) noexcept { return band * 4 + 2; }
constexpr int eqParamQ(int band) noexcept { return band * 4 + 3; }
constexpr int eqParamOutputGain = eqBandCount * 4;
constexpr int eqParamSlope(int band) noexcept { return eqParamOutputGain + 1 + band; }

InsertEqSettings insertEqSettingsFromValues(const std::array<float, maxInsertParams> &values) noexcept;

struct InsertParamInfo
{
    const char *name = "";
    float minimum = 0.0f;
    float maximum = 1.0f;
    float defaultValue = 0.0f;
    const char *unit = "";
};

const char *insertTypeName(InsertType type) noexcept;
int insertParamCount(InsertType type) noexcept;
const InsertParamInfo &insertParamInfo(InsertType type, int index) noexcept;

class LimiterProcessor
{
  public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();
    void process(float *left, float *right, int numSamples, float ceilingDb, float releaseMs);

  private:
    double sr = 48000.0;
    float envelope = 0.0f;
};

class ClipperProcessor
{
  public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();
    void process(float *left, float *right, int numSamples, float drive, float ceilingDb);

  private:
    double sr = 48000.0;
};

struct InsertSlotState
{
    InsertType type = InsertType::none;
    bool bypass = false;
    float gainReductionDb = 0.0f;
    std::array<float, maxInsertParams> values {};
};

struct InsertSlot
{
    InsertType type = InsertType::none;
    bool bypass = false;
    float gainReductionDb = 0.0f;
    std::array<float, maxInsertParams> values {};

    void prepare(double sampleRate, int maxBlockSize);
    void reset();
    void process(float *left, float *right, int numSamples);
    void setType(InsertType newType);
    InsertSlotState state() const noexcept;

  private:
    ChannelStripFX fx;
    InsertEq eq;
    LimiterProcessor limiter;
    ClipperProcessor clipper;
    bool prepared = false;
};

class InsertChain
{
  public:
    static constexpr int slotCount = 4;

    InsertChain();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();
    void process(float *left, float *right, int numSamples);

    InsertSlot &slot(int index) noexcept;
    const InsertSlot &slot(int index) const noexcept;

    void moveSlot(int from, int to);
    bool anyActive() const noexcept;

    std::array<std::unique_ptr<InsertSlot>, slotCount> slots;
};

} // namespace glorp
