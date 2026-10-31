#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <BinaryData.h>
namespace {
void add(juce::AudioProcessorValueTreeState::ParameterLayout &l, const juce::String &id,
         const juce::String &name, float lo, float hi, float def, float step = 0) {
    auto range = juce::NormalisableRange<float>(lo, hi, step);
    if (id.endsWith("from") || id.endsWith("to")) {
        // Signed log1p scaling preserves precision for sub-second times and small
        // generator levels while still supporting sample-rate endpoints up to 192 kHz.
        range = juce::NormalisableRange<float>(
            lo, hi,
            [](float, float maximum, float n) {
                double x = (double(n) - .5) * 2;
                return float(
                    std::copysign(std::expm1(std::abs(x) * std::log1p(double(maximum))), x));
            },
            [](float, float maximum, float x) {
                return float(.5 + .5 * std::copysign(std::log1p(std::abs(double(x))) /
                                                         std::log1p(double(maximum)),
                                                     x));
            });
    }
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name, range, def));
}
juce::String sid(int s, int p, const juce::String &key) {
    return "s" + juce::String(s) + "p" + juce::String(p) + key;
}
} // namespace
juce::AudioProcessorValueTreeState::ParameterLayout TheTitsProcessor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    add(l, "build", "BUILD", 0, 1, 0);
    add(l, "mix", "Mix", 0, 1, 1);
    add(l, "output", "Output dB", -24, 6, 0);
    add(l, "bypass", "Bypass", 0, 1, 0, 1);
    add(l, "trigger", "Trigger", 0, 1, 0, 1);
    add(l, "reset", "Reset", 0, 1, 0, 1);
    add(l, "mode", "Motion mode", 0, 2, 0, 1);
    add(l, "bars", "Ramp bars (power of 2)", 0, 4, 2, 1);
    add(l, "quantise", "Start quantisation", 0, 2, 2, 1);
    add(l, "endMode", "End: hold / return / reverse", 0, 2, 0, 1);
    add(l, "wave", "LFO waveform", 0, 4, 0, 1);
    add(l, "depth", "LFO depth", 0, 1, 1);
    add(l, "minimum", "LFO minimum", 0, 1, 0);
    add(l, "sync", "LFO sync", 0, 1, 1, 1);
    add(l, "freeHz", "LFO Hz", .02f, 10, .25f);
    add(l, "tempo", "Fallback tempo", 40, 240, 126);
    add(l, "tails", "Preserve tails", 0, 1, 1, 1);
    add(l, "protection", "Output soft clipper", 0, 1, 0, 1);
    for (int s = 0; s < tits::slots; ++s) {
        auto prefix = "s" + juce::String(s);
        add(l, prefix + "type", "Slot " + juce::String(s + 1) + " module", 0, tits::ModuleCount - 1,
            0, 1);
        add(l, prefix + "on", "Slot enabled", 0, 1, 1, 1);
        for (int p = 0; p < tits::controls; ++p) {
            add(l, sid(s, p, "from"), "Mapping start", -192000, 192000, 0);
            add(l, sid(s, p, "to"), "Mapping end", -192000, 192000, 0);
            add(l, sid(s, p, "begin"), "Macro activation", 0, .999f, 0);
            add(l, sid(s, p, "end"), "Macro finish", .001f, 1, 1);
            add(l, sid(s, p, "curve"), "Curve exponent", .15f, 6, 1);
            add(l, sid(s, p, "domain"), "Linear / logarithmic", 0, 1, 0, 1);
        }
    }
    return l;
}
TheTitsProcessor::TheTitsProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "TheTitsState", layout()) {
    factoryBank = juce::JSON::parse(
        juce::String::fromUTF8(BinaryData::factory_json, BinaryData::factory_jsonSize));
    // Cache parameter atomics in deterministic order. processBlock performs no ID construction or
    // tree traversal.
    for (auto &v : getParameters())
        if (auto *p = dynamic_cast<juce::RangedAudioParameter *>(v))
            realtime.push_back(state.getRawParameterValue(p->paramID));
    loadFactory(0);
}
float TheTitsProcessor::value(const juce::String &id) const {
    auto *p = state.getRawParameterValue(id);
    return p ? p->load() : 0;
}
void TheTitsProcessor::set(const juce::String &id, float v) {
    if (auto *p = state.getParameter(id)) {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(v));
        p->endChangeGesture();
    }
}
tits::Config TheTitsProcessor::config() const {
    tits::Config c;
    size_t n = 18;
    for (auto &s : c.slot) {
        s.type = int(realtime[n++]->load());
        s.enabled = realtime[n++]->load() > .5f;
        for (auto &m : s.map) {
            m.from = realtime[n++]->load();
            m.to = realtime[n++]->load();
            m.begin = realtime[n++]->load();
            m.end = realtime[n++]->load();
            m.curve = realtime[n++]->load();
            m.domain = int(realtime[n++]->load());
        }
    }
    return c;
}
tits::MotionSettings TheTitsProcessor::motion() const {
    tits::MotionSettings s;
    s.manual = realtime[0]->load();
    s.mode = int(realtime[6]->load());
    s.bars = 1 << int(realtime[7]->load());
    s.quantise = int(realtime[8]->load());
    s.endMode = int(realtime[9]->load());
    s.wave = int(realtime[10]->load());
    s.depth = realtime[11]->load();
    s.minimum = realtime[12]->load();
    s.sync = realtime[13]->load() > .5f;
    s.freeHz = realtime[14]->load();
    return s;
}
void TheTitsProcessor::prepareToPlay(double sr, int block) {
    engine.prepare(sr, block);
    setLatencySamples(0);
    fallbackPPQ = 0;
}
bool TheTitsProcessor::isBusesLayoutSupported(const BusesLayout &l) const {
    return l.getMainInputChannelSet() == l.getMainOutputChannelSet() &&
           (l.getMainOutputChannelSet() == juce::AudioChannelSet::mono() ||
            l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}
void TheTitsProcessor::processBlock(juce::AudioBuffer<float> &b, juce::MidiBuffer &) {
    run(b, false);
}
void TheTitsProcessor::processBlockBypassed(juce::AudioBuffer<float> &b, juce::MidiBuffer &) {
    run(b, true);
}
void TheTitsProcessor::run(juce::AudioBuffer<float> &b, bool hostBypass) {
    juce::ScopedNoDenormals guard;
    for (int c = getTotalNumInputChannels(); c < b.getNumChannels(); ++c)
        b.clear(c, 0, b.getNumSamples());
    if (b.getNumChannels() < 1)
        return;
    tits::Timing t;
    t.bpm = realtime[15]->load();
    t.ppq = fallbackPPQ;
    if (auto *ph = getPlayHead())
        if (auto pos = ph->getPosition()) {
            t.playing = pos->getIsPlaying();
            if (auto bpm = pos->getBpm())
                t.bpm = *bpm;
            if (auto ppq = pos->getPpqPosition()) {
                t.ppq = *ppq;
                t.hasPosition = true;
            }
            if (auto meter = pos->getTimeSignature()) {
                t.numerator = meter->numerator;
                t.denominator = meter->denominator;
            }
            if (auto bar = pos->getPpqPositionOfLastBarStart())
                t.barStart = *bar;
        }
    t.bpm = juce::jlimit(30., 300., t.bpm);
    tempo.store(t.bpm, std::memory_order_relaxed);
    fallbackPPQ = t.ppq + b.getNumSamples() * t.bpm / (60 * getSampleRate());
    bool tr = realtime[4]->load() > .5f, re = realtime[5]->load() > .5f;
    bool trigger = triggerRequested.exchange(false) || (tr && !triggerWas),
         reset = resetRequested.exchange(false) || (re && !resetWas);
    triggerWas = tr;
    resetWas = re;
    unsigned generation = configGeneration.load(std::memory_order_acquire);
    if ((generation & 1) == 0) {
        auto candidate = config();
        if (configGeneration.load(std::memory_order_acquire) == generation)
            audioConfig = candidate;
    }
    engine.process(b.getArrayOfWritePointers(), juce::jmin(2, b.getNumChannels()),
                   b.getNumSamples(), audioConfig, motion(), t, realtime[1]->load(),
                   realtime[2]->load(), hostBypass || realtime[3]->load() > .5f, trigger, reset,
                   realtime[16]->load() > .5f, panicRequested.exchange(false));
    if (realtime[17]->load() > .5f && engine.effective.load() > 1e-6f)
        for (int c = 0; c < b.getNumChannels(); ++c)
            for (int i = 0; i < b.getNumSamples(); ++i) {
                float x = b.getSample(c, i);
                if (std::abs(x) > .8f)
                    b.setSample(c, i,
                                std::copysign(.8f + .2f * std::tanh((std::abs(x) - .8f) * 5), x));
            }
}
juce::AudioProcessorEditor *TheTitsProcessor::createEditor() {
    return new TheTitsEditor(*this);
}
void TheTitsProcessor::getStateInformation(juce::MemoryBlock &b) {
    auto tree = state.copyState();
    tree.setProperty("version", 1, nullptr);
    tree.setProperty("presetName", presetName, nullptr);
    tree.setProperty("category", category, nullptr);
    tree.setProperty("animation", animationAmount.load(), nullptr);
    tree.setProperty("reducedMotion", reducedMotion.load(), nullptr);
    copyXmlToBinary(*tree.createXml(), b);
}
void TheTitsProcessor::setStateInformation(const void *data, int size) {
    if (size < 1 || size > 4 * 1024 * 1024)
        return;
    if (auto xml = getXmlFromBinary(data, size)) {
        auto tree = juce::ValueTree::fromXml(*xml);
        if (!tree.hasType("TheTitsState") || int(tree.getProperty("version", 0)) != 1)
            return;
        // Reject invalid host state rather than importing unchecked parameter ranges.
        for (auto child : tree) {
            auto id = child.getProperty("id").toString();
            auto *p = state.getParameter(id);
            float v = float(child.getProperty("value"));
            if (!p || !std::isfinite(v) || v < p->getNormalisableRange().start ||
                v > p->getNormalisableRange().end)
                return;
        }
        state.replaceState(tree);
        presetName = tree.getProperty("presetName", "User preset").toString();
        category = tree.getProperty("category", "User").toString();
        animationAmount = tits::clamp(float(tree.getProperty("animation", 1)));
        reducedMotion = bool(tree.getProperty("reducedMotion", false));
        dirty = false;
    }
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new TheTitsProcessor();
}
