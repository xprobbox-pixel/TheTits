#pragma once
#include <array>
#include <atomic>
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
namespace tits {
constexpr int slots = 6, controls = 8;
constexpr double pi = 3.14159265358979323846;
inline float clamp(float x, float a = 0, float b = 1) {
    return std::isfinite(x) ? juce::jlimit(a, b, x) : a;
}
enum Module {
    Empty,
    HPF,
    LPF,
    BPF,
    Tone,
    Delay,
    Tape,
    Reverb,
    Stutter,
    Tremolo,
    Autopan,
    Chorus,
    Phaser,
    Degrade,
    Drive,
    Siren,
    Noise,
    Utility,
    Flanger,
    ModuleCount
};
inline const juce::StringArray moduleNames{
    "Empty",        "High pass",   "Low pass",     "Band pass",        "Three band EQ",
    "Stereo delay", "Tape echo",   "Reverb",       "Loop / stutter",   "Tremolo / gate",
    "Autopan",      "Chorus",      "Phaser",       "Bit / rate crush", "Soft drive",
    "Tuned siren",  "Noise riser", "Gain / width", "Flanger"};
struct Mapping {
    float from = 0, to = 0, begin = 0, end = 1, curve = 1;
    int domain = 0;
    float at(float macro) const noexcept {
        float t = clamp((macro - begin) / juce::jmax(.001f, end - begin));
        t = std::pow(t, curve);
        if (domain == 1 && from > 0 && to > 0)
            return from * std::pow(to / from, t);
        return from + (to - from) * t;
    }
};
struct SlotConfig {
    int type = 0;
    bool enabled = true;
    std::array<Mapping, controls> map;
};
struct Config {
    std::array<SlotConfig, slots> slot;
};
struct Timing {
    double bpm = 126, ppq = 0, barStart = 0;
    int numerator = 4, denominator = 4;
    bool playing = false, hasPosition = false;
};
struct MotionSettings {
    float manual = 0, depth = 1, minimum = 0, freeHz = .25f;
    int mode = 0, bars = 4, quantise = 0, endMode = 0, wave = 0;
    bool sync = true;
};
class TransportClock {
  public:
    void prepare(double s) {
        sr = s;
        reset();
    }
    void reset() {
        phase = 0;
        active = pending = resetting = reverse = false;
        effective = held = 0;
        oldMode = 0;
        previousPPQ = 0;
    }
    void begin(const Timing &, const MotionSettings &, bool trigger, bool reset);
    float tick(const MotionSettings &);
    double beat() const {
        return ppq;
    }
    bool running() const {
        return play;
    }
    double tempo() const {
        return bpm;
    }
    float value() const {
        return effective;
    }

  private:
    double sr = 48000, bpm = 126, ppq = 0, phase = 0, bar = 4, startAt = 0, previousPPQ = 0,
           remaining = 0, rampDuration = 16;
    float effective = 0, resetManual = 0, held = 0;
    bool advancing = true, play = false, active = false, pending = false, reverse = false,
         resetting = false;
    int oldMode = 0;
};
struct SVF {
    float ic1 = 0, ic2 = 0;
    float run(float x, float hz, float q, double sr, int mode) noexcept;
    void reset() {
        ic1 = ic2 = 0;
    }
};
struct Biquad {
    float z1 = 0, z2 = 0, b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    void set(double sr, float hz, float db, int type);
    float run(float x) {
        auto y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() {
        z1 = z2 = 0;
    }
};
class Ring {
  public:
    void prepare(int n) {
        data[0].assign(n, 0);
        data[1].assign(n, 0);
        stamp.assign(n, 0);
        length = n;
        index = 0;
        epoch = 1;
    }
    void clear() {
        ++epoch;
        index = 0;
    }
    void write(float l, float r) {
        data[0][index] = l;
        data[1][index] = r;
        stamp[index] = epoch;
        index = (index + 1) % length;
    }
    float read(int ch, double delay) const noexcept {
        auto pos = index - juce::jlimit(1.0, double(length - 2), delay);
        while (pos < 0)
            pos += length;
        int a = int(pos), b = (a + 1) % length;
        float f = float(pos - a);
        return sample(ch, a) * (1 - f) + sample(ch, b) * f;
    }
    int size() const {
        return length;
    }
    int position() const {
        return index;
    }
    float absolute(int ch, int pos) const {
        pos = (pos % length + length) % length;
        return sample(ch, pos);
    }

  private:
    float sample(int ch, int i) const {
        return stamp[i] == epoch ? data[ch][i] : 0;
    }
    std::array<std::vector<float>, 2> data;
    std::vector<uint64_t> stamp;
    int length = 0, index = 0;
    uint64_t epoch = 1;
};
class EffectSlot {
  public:
    void prepare(double);
    void reset();
    void set(const SlotConfig &, float, double);
    void process(float &, float &, double, bool, float);
    int type = 0;

  private:
    double sr = 48000;
    Ring ring;
    std::array<std::array<std::vector<float>, 2>, 2> captured;
    int captureBank = 0;
    juce::Reverb reverb;
    std::array<float, controls> p{}, target{};
    SVF filters[2][2], feedbackFilter[2];
    Biquad eq[2][3];
    float feedback[2]{}, last[2]{}, hold[2]{}, allpass[2][4]{};
    double phase = 0, osc = 0, samplePhase = 0, loopPhase = 0;
    int captureStart = 0, captureLength = 0, history = 0, lastDivision = -1;
    bool wasStutter = false;
    uint32_t random = 0x83ab7431;
    int update = 0;
    float noiseState = 0, sourceEnvelope = 0, tailBlend = 0;
    double previousBeat = 0;
    int oldStart = 0, oldLength = 0, crossfade = 0;
    double oldPhase = 0;
};
class Engine {
  public:
    void prepare(double, int);
    void reset();
    void process(float *const *, int, int, const Config &, const MotionSettings &, const Timing &,
                 float, float, bool, bool, bool, bool, bool);
    std::atomic<float> effective{0}, inputPeak{0}, outputPeak{0};
    std::atomic<bool> fallback{true};

  private:
    std::array<EffectSlot, slots> fx;
    TransportClock clock;
    Config active{}, desired{};
    double sr = 48000;
    float macro = 0, bypassGain = 1, mixSmooth = 1, gainSmooth = 1, changeGain = 1, panicGain = 1;
    bool changing = false, panicPending = false;
    int zeroSamples = 0;
    void apply();
};
} // namespace tits
