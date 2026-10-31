#include "Engine.h"
namespace tits {
void TransportClock::begin(const Timing &t, const MotionSettings &s, bool trigger,
                           bool resetEvent) {
    bpm = juce::jlimit(30., 300., t.bpm);
    bar = t.numerator * 4.0 / juce::jmax(1, t.denominator);
    play = t.playing;
    advancing = play || !t.hasPosition;
    bool jump = t.hasPosition && std::abs(t.ppq - previousPPQ) > .25;
    if (t.hasPosition)
        ppq = t.ppq;
    if (jump && pending) {
        double unit = s.quantise == 2 ? bar : 1;
        startAt = t.barStart + std::ceil((ppq - t.barStart) / unit - 1e-9) * unit;
    }
    if (s.mode != oldMode) {
        active = pending = false;
        phase = 0;
        resetting = false;
        oldMode = s.mode;
    }
    if (resetting && std::abs(s.manual - resetManual) > .00001f)
        resetting = false;
    if (resetEvent) {
        active = pending = false;
        resetting = true;
        resetManual = s.manual;
    }
    if (trigger) {
        resetting = false;
        active = false;
        pending = true;
        reverse = false;
        rampDuration = s.bars * bar;
        remaining = 0;
        double unit = s.quantise == 2 ? bar : 1;
        startAt =
            s.quantise == 0 ? ppq : t.barStart + std::ceil((ppq - t.barStart) / unit - 1e-9) * unit;
    }
    previousPPQ = ppq;
}
float TransportClock::tick(const MotionSettings &s) {
    const double inc = bpm / (60 * sr);
    if (advancing && pending && ppq + 1e-10 >= startAt) {
        pending = false;
        active = true;
        remaining = 0;
    }
    float target = s.manual;
    if (s.mode == 1) {
        target = held;
        if (active && advancing) {
            remaining += inc;
            double t = juce::jlimit(0., 1., remaining / rampDuration);
            target = float(reverse ? 1 - t : t);
            held = target;
            if (t >= 1) {
                if (s.endMode == 2 && !reverse) {
                    reverse = true;
                    remaining = 0;
                } else {
                    active = false;
                    if (s.endMode != 0)
                        resetting = true;
                }
            }
        }
    }
    if (s.mode == 2) {
        double rate = s.sync ? (advancing ? inc / (s.bars * bar) : 0) : s.freeHz / sr;
        phase += rate;
        phase -= std::floor(phase);
        double w = 0;
        switch (s.wave) {
        case 0:
            w = .5 - .5 * std::cos(2 * pi * phase);
            break;
        case 1:
            w = 1 - std::abs(2 * phase - 1);
            break;
        case 2:
            w = phase;
            break;
        case 3:
            w = 1 - phase;
            break;
        default:
            w = .5 + .5 * std::tanh(5 * std::sin(2 * pi * phase));
            break;
        }
        target = s.minimum + s.depth * float(w);
    }
    if (resetting)
        target = 0;
    effective += float(1 - std::exp(-1 / (sr * .008))) * (clamp(target) - effective);
    if (std::abs(effective - target) < 1e-7f)
        effective = clamp(target);
    if (advancing)
        ppq += inc;
    previousPPQ = ppq;
    return effective;
}
float SVF::run(float x, float hz, float q, double sr, int mode) noexcept {
    auto g = float(std::tan(pi * juce::jlimit(15., sr * .45, double(hz)) / sr));
    auto k = 1.f / juce::jlimit(.5f, 5.f, q);
    auto a = 1 / (1 + g * (g + k));
    auto v1 = a * (ic1 + g * (x - ic2));
    auto v2 = ic2 + g * v1;
    ic1 = 2 * v1 - ic1;
    ic2 = 2 * v2 - ic2;
    return mode == 0 ? v2 : mode == 1 ? x - k * v1 - v2 : v1;
}
void Biquad::set(double sr, float hz, float db, int type) {
    db = clamp(db, -24, 24);
    double A = std::pow(10., db / 40.), w = 2 * pi * std::min(double(hz), sr * .44) / sr,
           c = std::cos(w), s = std::sin(w), alpha = s / 1.41421356237,
           beta = 2 * std::sqrt(A) * alpha, a0;
    if (type == 1) {
        a0 = 1 + alpha / A;
        b0 = float((1 + alpha * A) / a0);
        b1 = float(-2 * c / a0);
        b2 = float((1 - alpha * A) / a0);
        a1 = b1;
        a2 = float((1 - alpha / A) / a0);
    } else if (type == 0) {
        a0 = (A + 1) + (A - 1) * c + beta;
        b0 = float(A * ((A + 1) - (A - 1) * c + beta) / a0);
        b1 = float(2 * A * ((A - 1) - (A + 1) * c) / a0);
        b2 = float(A * ((A + 1) - (A - 1) * c - beta) / a0);
        a1 = float(-2 * ((A - 1) + (A + 1) * c) / a0);
        a2 = float(((A + 1) + (A - 1) * c - beta) / a0);
    } else {
        a0 = (A + 1) - (A - 1) * c + beta;
        b0 = float(A * ((A + 1) + (A - 1) * c + beta) / a0);
        b1 = float(-2 * A * ((A - 1) + (A + 1) * c) / a0);
        b2 = float(A * ((A + 1) + (A - 1) * c - beta) / a0);
        a1 = float(2 * ((A - 1) - (A + 1) * c) / a0);
        a2 = float(((A + 1) - (A - 1) * c - beta) / a0);
    }
}
void EffectSlot::prepare(double rate) {
    sr = rate;
    ring.prepare(int(sr * 8) + 4);
    for (auto &bank : captured)
        for (auto &channel : bank)
            channel.assign(int(sr * 2) + 4, 0);
    reverb.setSampleRate(sr);
    reset();
}
void EffectSlot::reset() {
    ring.clear();
    reverb.reset();
    for (auto &c : filters)
        for (auto &f : c)
            f.reset();
    for (auto &f : feedbackFilter)
        f.reset();
    for (auto &c : eq)
        for (auto &f : c)
            f.reset();
    for (int c = 0; c < 2; ++c) {
        feedback[c] = last[c] = hold[c] = 0;
        for (auto &v : allpass[c])
            v = 0;
    }
    phase = osc = samplePhase = loopPhase = 0;
    captureLength = history = 0;
    lastDivision = -1;
    wasStutter = false;
    noiseState = sourceEnvelope = tailBlend = 0;
    previousBeat = 0;
    oldLength = crossfade = 0;
    update = 0;
}
void EffectSlot::set(const SlotConfig &c, float m, double bpm) {
    int newType = c.enabled ? c.type : 0;
    if (newType != type) {
        type = newType;
        reset();
    }
    for (int i = 0; i < controls; ++i)
        target[i] = c.map[i].at(m);
    juce::ignoreUnused(bpm);
}
void EffectSlot::process(float &l, float &r, double beat, bool excite, float macro) {
    for (int i = 0; i < controls; ++i)
        p[i] += (target[i] - p[i]) * float(1 - std::exp(-1 / (sr * .012)));
    sourceEnvelope +=
        float(1 - std::exp(-1 / (sr * .01))) * ((excite ? macro : 0) - sourceEnvelope);
    float in[2]{l, r}, out[2]{l, r};
    float blend = clamp(p[7]);
    if (type == Delay || type == Tape || type == Reverb) {
        tailBlend = juce::jmax(blend, tailBlend * float(std::exp(-1 / (sr * 1.5))));
        if (!excite)
            blend = tailBlend;
    }
    ++update;
    switch (type) {
    case Empty:
        return;
    case HPF:
    case LPF:
    case BPF:
        for (int c = 0; c < 2; ++c) {
            out[c] = filters[c][0].run(in[c], p[0], p[1], sr,
                                       type == HPF   ? 1
                                       : type == LPF ? 0
                                                     : 2);
            if (p[2] > .5f)
                out[c] = filters[c][1].run(out[c], p[0], p[1], sr,
                                           type == HPF   ? 1
                                           : type == LPF ? 0
                                                         : 2);
            out[c] *= 1 / std::sqrt(juce::jmax(1.f, p[1] * .7f));
        }
        break;
    case Tone:
        if ((update & 31) == 0)
            for (auto &ch : eq) {
                ch[0].set(sr, 160, p[0], 0);
                ch[1].set(sr, 1200, p[1], 1);
                ch[2].set(sr, 7000, p[2], 2);
            }
        for (int c = 0; c < 2; ++c) {
            out[c] = in[c];
            for (auto &f : eq[c])
                out[c] = f.run(out[c]);
        }
        break;
    case Delay:
    case Tape: {
        double time = juce::jlimit(.001, 7.5, double(p[0]));
        float duck = 1 / (1 + clamp(p[3]) * 8 * std::max(std::abs(l), std::abs(r)));
        float fb = clamp(p[1], 0, .88f);
        for (int c = 0; c < 2; ++c) {
            float v = ring.read(c, time * sr);
            // Wet return calibration is independent of feedback and survives reset tails.
            out[c] = v * duck * std::pow(10.f, clamp(p[6], -24, 6) / 20);
            feedback[c] = feedbackFilter[c].run(v, p[2], .707f, sr, 0);
            if (type == Tape)
                feedback[c] = std::tanh(feedback[c] * 1.5f) / 1.5f;
            feedback[c] = filters[c][0].run(feedback[c], 20, .707f, sr, 1);
        }
        ring.write(l + fb * feedback[p[4] > .5f ? 1 : 0], r + fb * feedback[p[4] > .5f ? 0 : 1]);
        break;
    }
    case Reverb: {
        ring.write(l, r);
        out[0] = ring.read(0, juce::jmax(1., double(p[3]) * sr));
        out[1] = ring.read(1, juce::jmax(1., double(p[3]) * sr));
        if ((update & 31) == 0) {
            juce::Reverb::Parameters v;
            v.roomSize = clamp(p[0]);
            v.damping = clamp(p[1]);
            v.width = clamp(p[2]);
            v.wetLevel = std::pow(10.f, clamp(p[4], -24, 0) / 20);
            v.dryLevel = 0;
            v.freezeMode = 0;
            reverb.setParameters(v);
        }
        reverb.processStereo(&out[0], &out[1], 1);
        break;
    }
    case Stutter: {
        int division = juce::jlimit(0, 5, int(std::round(p[0])));
        bool enabled = macro > p[2] && blend > .001f;
        int len = juce::jlimit(32, ring.size() - 4,
                               int(sr * clamp(p[3], .05f, 2.f) / std::pow(2., division)));
        bool edge = std::floor(beat * std::pow(2., division)) !=
                    std::floor(previousBeat * std::pow(2., division));
        previousBeat = beat;
        if (enabled && history >= len && ((!wasStutter || division != lastDivision) && edge)) {
            oldStart = captureStart;
            oldLength = captureLength;
            oldPhase = loopPhase;
            crossfade = 128;
            captureStart = (ring.position() - len + ring.size()) % ring.size();
            captureLength = len;
            captureBank = 1 - captureBank;
            for (int ch = 0; ch < 2; ++ch)
                for (int sample = 0; sample < len; ++sample)
                    captured[captureBank][ch][sample] = ring.absolute(ch, captureStart + sample);
            loopPhase = 0;
            lastDivision = division;
            wasStutter = true;
        }
        if (!enabled)
            wasStutter = false;
        if (wasStutter && captureLength > 0) {
            double pos = std::fmod(loopPhase, captureLength);
            double cross = juce::jmin(128., captureLength * .2);
            for (int c = 0; c < 2; ++c) {
                auto sample = [&](double offset) {
                    if (p[1] > .5f)
                        offset = captureLength - 1 - offset;
                    int a = int(offset);
                    float f = float(offset - a);
                    return captured[captureBank][c][a % captureLength] * (1 - f) +
                           captured[captureBank][c][(a + 1) % captureLength] * f;
                };
                float v = sample(pos);
                if (pos < cross) {
                    float t = float(pos / cross);
                    v = v * t + sample(captureLength - cross + pos) * (1 - t);
                }
                if (crossfade > 0 && oldLength > 0) {
                    int previous = int(std::fmod(oldPhase, oldLength));
                    if (p[1] > .5f)
                        previous = oldLength - 1 - previous;
                    float old = captured[1 - captureBank][c][previous];
                    float fade = 1 - crossfade / 128.f;
                    v = old * (1 - fade) + v * fade;
                }
                out[c] = v;
            }
            loopPhase += 1;
            oldPhase += 1;
            if (crossfade > 0)
                --crossfade;
        } else {
            out[0] = l;
            out[1] = r;
        }
        // Dedicated ping-pong capture banks allow continuous history and a short capture-change
        // crossfade.
        ring.write(l, r);
        history = juce::jmin(history + 1, ring.size() - 1);
        break;
    }
    case Tremolo:
    case Autopan: {
        float w = .5f - .5f * std::cos(float(2 * pi * beat * p[0]));
        float d = clamp(p[1]);
        if (type == Tremolo) {
            float gain = 1 - d * std::pow(w, clamp(p[2], .2f, 8.f));
            out[0] *= gain;
            out[1] *= gain;
        } else {
            float pan = std::sin(float(2 * pi * beat * p[0])) * d;
            out[0] *= 1 - juce::jmax(0.f, pan);
            out[1] *= 1 + juce::jmin(0.f, pan);
        }
        break;
    }
    case Chorus: {
        ring.write(l, r);
        for (int c = 0; c < 2; ++c)
            out[c] = ring.read(
                c, sr * (.012 + .006 * clamp(p[1]) * std::sin(2 * pi * beat * p[0] + c * pi * .5)));
        break;
    }
    case Phaser: {
        for (int c = 0; c < 2; ++c) {
            float depth = p[1] > .001f ? clamp(p[1]) : 1.f;
            float sweep = .5f + .5f * std::sin(float(2 * pi * beat * p[0] + c * p[3] * pi));
            float hz = 180.f * std::pow(24.f, sweep * depth);
            float tangent = float(std::tan(pi * std::min(double(hz), sr * .4) / sr));
            float a = (1 - tangent) / (1 + tangent);
            float x = in[c] + clamp(p[2], -.8f, .8f) * feedback[c];
            for (int k = 0; k < 4; ++k) {
                float y = -a * x + allpass[c][k];
                allpass[c][k] = x + a * y;
                x = y;
            }
            feedback[c] = x;
            out[c] = x;
        }
        break;
    }
    case Flanger: {
        float delayed[2];
        for (int c = 0; c < 2; ++c) {
            double wave = .5 + .5 * std::sin(2 * pi * beat * p[0] + c * pi * .5);
            double milliseconds = clamp(p[3], .15f, 5.f) + clamp(p[1], .1f, 8.f) * wave;
            delayed[c] = ring.read(c, sr * milliseconds * .001);
            feedback[c] = feedbackFilter[c].run(delayed[c], 25, .707f, sr, 1);
            out[c] = delayed[c] * (p[4] > .5f ? -1.f : 1.f);
        }
        float fb = clamp(p[2], -.8f, .8f);
        ring.write(l + fb * feedback[0], r + fb * feedback[1]);
        break;
    }
    case Degrade: {
        samplePhase += clamp(p[1], 100.f, float(sr)) / sr;
        if (samplePhase >= 1) {
            samplePhase -= std::floor(samplePhase);
            float levels = std::pow(2.f, clamp(p[0], 3, 24) - 1);
            for (int c = 0; c < 2; ++c)
                hold[c] = std::round(in[c] * levels) / levels;
        }
        for (int c = 0; c < 2; ++c)
            out[c] = filters[c][0].run(hold[c], clamp(p[2], 300.f, float(sr * .44)), .707f, sr, 0);
        break;
    }
    case Drive: {
        float d = std::pow(10.f, clamp(p[0], 0, 30) / 20);
        for (int c = 0; c < 2; ++c)
            out[c] = filters[c][0].run(std::tanh(in[c] * d) / std::sqrt(d), 20, .707f, sr, 1);
        break;
    }
    case Siren: {
        float pitch =
            clamp(clamp(p[0], 24, 96) + clamp(p[1], -36, 36) * macro +
                      clamp(p[2], 0, 24) * std::sin(float(2 * pi * beat * clamp(p[3], .125f, 16))),
                  12, 120);
        osc += std::min(sr * .4, 440 * std::pow(2., (pitch - 69) / 12)) / sr;
        osc -= std::floor(osc);
        float v = p[4] > .5f ? float(1 - 4 * std::abs(osc - .5)) : std::sin(float(2 * pi * osc));
        v *= clamp(p[5], 0, .2f) * sourceEnvelope;
        out[0] += v;
        out[1] += v;
        blend = 1;
        break;
    }
    case Noise: {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        float v = float(double(random) / 2147483648. - 1);
        noiseState = .97f * noiseState + .03f * v;
        v = p[2] > .5f ? noiseState * 3 : v;
        v = filters[0][0].run(v, p[0], .707f, sr, 2);
        v *= clamp(p[1], 0, .2f) * sourceEnvelope;
        out[0] += v;
        out[1] += v;
        blend = 1;
        break;
    }
    case Utility: {
        float mid = (l + r) * .5f, side = (l - r) * .5f * clamp(p[1], 0, 1.5f),
              g = std::pow(10.f, clamp(p[0], -24, 6) / 20);
        out[0] = (mid + side) * g * (1 - juce::jmax(0.f, clamp(p[2], -1, 1)));
        out[1] = (mid - side) * g * (1 + juce::jmin(0.f, clamp(p[2], -1, 1)));
        break;
    }
    default:
        break;
    }
    l = in[0] + blend * (out[0] - in[0]);
    r = in[1] + blend * (out[1] - in[1]);
    if (!std::isfinite(l) || !std::isfinite(r)) {
        l = r = 0;
        reset();
    }
    l = juce::jlimit(-16.f, 16.f, l);
    r = juce::jlimit(-16.f, 16.f, r);
}
void Engine::prepare(double s, int) {
    sr = s;
    for (auto &f : fx)
        f.prepare(s);
    clock.prepare(s);
    reset();
}
void Engine::reset() {
    for (auto &f : fx)
        f.reset();
    clock.reset();
    macro = 0;
    changeGain = 1;
    bypassGain = 1;
    panicGain = 1;
    zeroSamples = 0;
    changing = panicPending = false;
}
void Engine::apply() {
    active = desired;
    for (int i = 0; i < slots; ++i) {
        fx[i].reset();
        fx[i].type = active.slot[i].enabled ? active.slot[i].type : 0;
    }
}
void Engine::process(float *const *data, int channels, int count, const Config &cfg,
                     const MotionSettings &s, const Timing &t, float mix, float output, bool bypass,
                     bool trigger, bool resetEvent, bool tails, bool panic) {
    desired = cfg;
    for (int i = 0; i < slots; ++i)
        if (active.slot[i].type != cfg.slot[i].type ||
            active.slot[i].enabled != cfg.slot[i].enabled)
            changing = true;
    if (!changing)
        active = cfg;
    clock.begin(t, s, trigger, resetEvent || panic);
    panicPending |= panic;
    fallback.store(!t.hasPosition, std::memory_order_relaxed);
    float ip = 0, op = 0;
    float smooth = float(1 - std::exp(-1 / (sr * .005)));
    for (int n = 0; n < count; ++n) {
        macro = clock.tick(s);
        if (changing) {
            changeGain = juce::jmax(0.f, changeGain - float(1 / (sr * .005)));
            if (changeGain == 0) {
                apply();
                changing = false;
            }
        } else
            changeGain = juce::jmin(1.f, changeGain + float(1 / (sr * .005)));
        if (panicPending) {
            panicGain = juce::jmax(0.f, panicGain - float(1 / (sr * .005)));
            if (panicGain == 0) {
                for (auto &f : fx)
                    f.reset();
                panicPending = false;
            }
        } else
            panicGain = juce::jmin(1.f, panicGain + float(1 / (sr * .005)));
        float l = std::isfinite(data[0][n]) ? data[0][n] : 0,
              r = channels > 1 ? (std::isfinite(data[1][n]) ? data[1][n] : 0) : l;
        ip = juce::jmax(ip, std::abs(l), std::abs(r));
        float wetL = macro > 1e-6f && !bypass ? l : 0, wetR = macro > 1e-6f && !bypass ? r : 0;
        bool excite = (t.playing || !t.hasPosition) && !bypass && macro > 1e-5f && !panicPending;
        if ((n & 15) == 0)
            for (int i = 0; i < slots; ++i) {
                fx[i].set(
                    active.slot[i], macro,
                    t.bpm); // Delay time and captured quarter-note duration use one shared clock.
                if ((fx[i].type == Delay || fx[i].type == Tape) &&
                    active.slot[i].map[5].at(macro) > .5f) {
                    auto c = active.slot[i];
                    auto seconds =
                        juce::jlimit(.001f, 7.5f, c.map[0].at(macro) * float(60 / t.bpm));
                    c.map[0].from = c.map[0].to = seconds;
                    fx[i].set(c, macro, t.bpm);
                }
                if (fx[i].type == Stutter) {
                    auto c = active.slot[i];
                    c.map[3].from = c.map[3].to = float(60 / t.bpm);
                    fx[i].set(c, macro, t.bpm);
                }
            }
        for (auto &f : fx)
            f.process(wetL, wetR, clock.beat(), excite, macro);
        bypassGain += smooth * ((bypass ? 0.f : 1.f) - bypassGain);
        mixSmooth += smooth * (clamp(mix) - mixSmooth);
        gainSmooth += smooth * (std::pow(10.f, clamp(output, -24, 6) / 20) - gainSmooth);
        float amount = macro < 1e-6f ? 0.f : mixSmooth;
        float residual = (tails && amount == 0) ? mixSmooth : 0;
        float fade = changeGain * panicGain * bypassGain;
        float a = (l * (1 - amount * fade) + wetL * (amount + residual) * fade) * gainSmooth,
              b = (r * (1 - amount * fade) + wetR * (amount + residual) * fade) * gainSmooth;
        if (!tails && macro < 1e-6f) {
            if (++zeroSamples == int(sr * .1))
                for (auto &f : fx)
                    f.reset();
        } else
            zeroSamples = 0;
        data[0][n] = std::isfinite(a) ? a : 0;
        if (channels > 1)
            data[1][n] = std::isfinite(b) ? b : 0;
        op = juce::jmax(op, std::abs(a), std::abs(b));
    }
    effective.store(macro, std::memory_order_relaxed);
    inputPeak.store(ip, std::memory_order_relaxed);
    outputPeak.store(op, std::memory_order_relaxed);
}
} // namespace tits
