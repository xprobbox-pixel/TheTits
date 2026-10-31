#include "PluginEditor.h"
#include "PluginProcessor.h"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <thread>
thread_local bool auditAudio = false;
thread_local int audioAllocations = 0;
void *operator new(std::size_t n) {
    if (auditAudio)
        ++audioAllocations;
    if (auto *p = std::malloc(n ? n : 1))
        return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t n) {
    return ::operator new(n);
}
void operator delete(void *p) noexcept {
    std::free(p);
}
void operator delete[](void *p) noexcept {
    std::free(p);
}
using namespace tits;
namespace {
int failures = 0, checks = 0;
void expect(bool ok, const char *what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::cerr << "FAIL " << what << '\n';
    }
}
class Host : public juce::AudioPlayHead {
  public:
    Timing t;
    juce::Optional<PositionInfo> getPosition() const override {
        PositionInfo p;
        p.setBpm(t.bpm);
        p.setPpqPosition(t.ppq);
        p.setIsPlaying(t.playing);
        p.setTimeSignature(juce::AudioPlayHead::TimeSignature{t.numerator, t.denominator});
        p.setPpqPositionOfLastBarStart(std::floor(t.ppq / (t.numerator * 4. / t.denominator)) *
                                       (t.numerator * 4. / t.denominator));
        return p;
    }
};
float signal(double t, int ch) {
    double beat = std::fmod(t * 126 / 60, 1.);
    double kick =
        .3 * std::sin(2 * pi * (48 * t + 8 * (1 - std::exp(-t * 30)))) * std::exp(-beat * 18);
    double bass = .12 * std::sin(2 * pi * 110 * t);
    double chord = .035 * (std::sin(2 * pi * 220 * t) + std::sin(2 * pi * 277.18 * t) +
                           std::sin(2 * pi * 329.63 * t));
    double hat =
        .035 * std::sin(2 * pi * (7700 + ch * 270) * t) * std::exp(-std::fmod(beat * 4, 1.) * 22);
    return float(kick + bass + chord + hat);
}
} // namespace
int main(int argc, char **argv) {
    juce::ScopedJuceInitialiser_GUI init;
    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile("output");
    for (int a = 1; a < argc; ++a)
        if (juce::String(argv[a]).startsWith("--output="))
            out = juce::File::getCurrentWorkingDirectory().getChildFile(
                juce::String(argv[a]).fromFirstOccurrenceOf("--output=", false, false));
    out.createDirectory();
    bool render = false, screens = false, stress = false, stills = false;
    for (int a = 1; a < argc; ++a) {
        render |= std::string(argv[a]) == "--render";
        screens |= std::string(argv[a]) == "--screens";
        stills |= std::string(argv[a]) == "--stills";
        stress |= std::string(argv[a]) == "--stress";
    }
    auto p = std::make_unique<TheTitsProcessor>();
    juce::MidiBuffer midi;
    expect(p->factoryCount() == 24, "24 factory presets");
    p->set("s0p0from", 25);
    expect(std::abs(p->value("s0p0from") - 25) < .001f, "mapping precision at low frequencies");
    p->loadFactory(0);
    std::ofstream report(out.getChildFile("test-results.txt").getFullPathName().toStdString());
    report << "The Tits 0.3.1 DSP verification\n";
    // Exact unity and bypass across every requested sample-rate / channel / buffer combination.
    for (double sr : {44100., 48000., 88200., 96000., 192000.})
        for (int channels : {1, 2}) {
            p->setPlayConfigDetails(channels, channels, sr, 2048);
            p->prepareToPlay(sr, 2048);
            p->set("build", 0);
            for (int n : {1, 32, 64, 127, 512, 2048}) {
                juce::AudioBuffer<float> b(channels, n);
                for (int c = 0; c < channels; ++c)
                    for (int i = 0; i < n; ++i)
                        b.setSample(c, i, float(std::sin(i * .2 + c)) * .2f);
                auto copy = b;
                p->processBlock(b, midi);
                for (int c = 0; c < channels; ++c)
                    for (int i = 0; i < n; ++i)
                        expect(b.getSample(c, i) == copy.getSample(c, i),
                               "zero macro exact dry unity");
                p->processBlockBypassed(b, midi);
                for (int c = 0; c < channels; ++c)
                    for (int i = 0; i < n; ++i)
                        expect(b.getSample(c, i) == copy.getSample(c, i), "bypass dry unity");
            }
        }
    report << "Unity and bypass: 5 rates x mono/stereo x 6 block sizes; sample-exact.\n";
    juce::String error;
    auto json = p->exportJSON();
    expect(p->importJSON(json, error), "JSON round trip");
    expect(!p->importJSON("{}", error), "malformed JSON rejected");
    auto bad = juce::JSON::parse(json);
    bad.getDynamicObject()->setProperty("version", 9);
    expect(!p->importJSON(juce::JSON::toString(bad), error), "unknown version rejected");
    bad = juce::JSON::parse(json);
    (*bad["slots"].getArray())[0].getDynamicObject()->setProperty("type", 999);
    expect(!p->importJSON(juce::JSON::toString(bad), error), "invalid module rejected");
    p->set("build", .57f);
    p->set("output", -3);
    juce::MemoryBlock state;
    p->getStateInformation(state);
    p->set("build", 0);
    p->setStateInformation(state.getData(), int(state.getSize()));
    expect(std::abs(p->value("build") - .57f) < 1e-6f, "host state recall");
    p->set("output", 0);
    p->set("build", 0);
    TransportClock clock;
    clock.prepare(1000);
    Timing t;
    t.bpm = 120;
    t.numerator = 3;
    t.denominator = 4;
    t.hasPosition = true;
    t.playing = true;
    MotionSettings ms;
    ms.mode = 1;
    ms.bars = 1;
    ms.quantise = 0;
    clock.begin(t, ms, true, false);
    for (int i = 0; i < 1600; ++i)
        clock.tick(ms);
    expect(clock.value() > .999f, "one 3/4 bar is 3 quarter notes");
    clock.prepare(1000);
    t.ppq = .2;
    ms.quantise = 2;
    clock.begin(t, ms, true, false);
    for (int i = 0; i < 100; ++i)
        clock.tick(ms);
    expect(clock.value() < .001f, "next bar quantisation waits");
    t.ppq = 9.2;
    clock.begin(t, ms, false, false);
    for (int i = 0; i < 100; ++i)
        clock.tick(ms);
    expect(clock.value() < .001f, "seek reschedules pending event");
    JiggleAnimator spring;
    for (int i = 0; i < 10000; ++i)
        spring.step(i % 27 == 0 ? 2. : .016f, float(i % 100) / 100, 1, false);
    expect(std::isfinite(spring.springs[0].x) && std::abs(spring.springs[0].x) <= .085,
           "spring remains bounded after stalls");
    for (int i = 0; i < 1000; ++i)
        spring.step(.016, 0, 1, false);
    expect(std::abs(spring.springs[0].x) < 1e-6, "spring settles");
    spring.step(.016, 1, 0, false);
    expect(spring.springs[0].x == 0, "animation off");
    double amplitude[2]{};
    for (int level = 0; level < 2; ++level) {
        JiggleAnimator a;
        for (int i = 0; i < 600; ++i) {
            a.step(1. / 60, level == 0 ? .75f : 1.f, 1, false);
            if (i > 120)
                amplitude[level] = std::max(amplitude[level], std::abs(a.springs[0].x));
        }
    }
    expect(amplitude[1] > amplitude[0] * 1.6, "stronger animation boost at full effect");
    Host host;
    host.t.hasPosition = true;
    host.t.playing = true;
    p->setPlayHead(&host);
    // A flanger must produce a millisecond comb tap, including polarity reversal,
    // rather than silently falling through to dry audio or a long delay.
    for (int polarity : {0, 1}) {
        Engine e;
        e.prepare(48000, 1024);
        Config cfg;
        cfg.slot[0].type = Flanger;
        const float values[]{.25f, .1f, .6f, 2.f, float(polarity), 0, 0, .5f};
        for (int i = 0; i < controls; ++i)
            cfg.slot[0].map[i].from = cfg.slot[0].map[i].to = values[i];
        MotionSettings motion;
        motion.manual = 1;
        Timing time;
        float left[1024]{}, right[1024]{};
        float *channels[]{left, right};
        for (int block = 0; block < 50; ++block)
            e.process(channels, 2, 1024, cfg, motion, time, 1, 0, false, false, false, false,
                      false);
        left[0] = right[0] = 1;
        e.process(channels, 2, 1024, cfg, motion, time, 1, 0, false, false, false, false, false);
        double tap = 0, feedbackEnergy = 0;
        for (int i = 60; i < 150; ++i)
            tap += left[i];
        for (int i = 160; i < 800; ++i)
            feedbackEnergy += left[i] * left[i];
        expect(polarity ? tap < -.3 : tap > .3, "flanger has short comb tap with correct polarity");
        expect(feedbackEnergy > .005, "flanger feedback produces subsequent comb taps");
    }
    // Compare actual processor output with identical source, transport, warm-up and level.
    // Normalize each capture's RMS so gain changes alone cannot make duplicates pass.
    std::vector<std::vector<float>> signatures;
    for (int preset = 0; preset < p->factoryCount(); ++preset) {
        std::vector<float> signature;
        for (float amount : {.25f, .6f, 1.f}) {
            p->prepareToPlay(48000, 512);
            p->loadFactory(preset);
            p->set("mode", 0);
            p->set("build", amount);
            p->set("output", 0);
            host.t.bpm = 126;
            host.t.playing = true;
            host.t.numerator = 4;
            auto cfg = p->config();
            for (int s = 0; s < slots; ++s)
                expect(cfg.slot[s].type == int(p->factory(preset)["slots"][s]["type"]),
                       "recalled processor chain matches factory module identities");
            std::vector<float> capture;
            for (int offset = 0; offset < 144000; offset += 512) {
                int n = std::min(512, 144000 - offset);
                juce::AudioBuffer<float> b(2, n);
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < n; ++i)
                        b.setSample(c, i, signal((offset + i) / 48000., c));
                host.t.ppq = offset / 48000. * 126 / 60;
                p->processBlock(b, midi);
                for (int i = 0; i < n; i += 2)
                    if (offset + i >= 96000)
                        for (int c = 0; c < 2; ++c)
                            capture.push_back(b.getSample(c, i));
            }
            double energy = 0;
            for (auto x : capture)
                energy += x * x;
            double rms = std::sqrt(energy / capture.size());
            expect(rms > .00001 && std::isfinite(rms), "factory produces usable finite audio");
            for (auto x : capture)
                signature.push_back(float(x / std::max(rms, 1e-9)));
        }
        signatures.push_back(std::move(signature));
    }
    std::ofstream diversity(
        out.getChildFile("preset-diversity.csv").getFullPathName().toStdString());
    diversity << "preset_a,preset_b,normalized_waveform_distance\n";
    double nearestDistance = 100;
    for (int a = 0; a < int(signatures.size()); ++a)
        for (int b = a + 1; b < int(signatures.size()); ++b) {
            double sum = 0;
            for (size_t i = 0; i < signatures[a].size(); ++i) {
                double d = signatures[a][i] - signatures[b][i];
                sum += d * d;
            }
            double distance = std::sqrt(sum / signatures[a].size());
            nearestDistance = std::min(nearestDistance, distance);
            diversity << p->factory(a)["name"].toString() << ',' << p->factory(b)["name"].toString()
                      << ',' << distance << '\n';
            expect(distance > .15, "factory presets differ beyond normalized gain changes");
        }
    std::cout << "Closest factory waveform distance: " << nearestDistance << '\n';
    std::ofstream metrics(out.getChildFile("preset-metrics.csv").getFullPathName().toStdString());
    metrics << "preset,sample_rate,peak,rms,wall_seconds,audio_seconds\n";
    std::vector<double> rates = stress ? std::vector<double>{44100, 48000, 88200, 96000, 192000}
                                       : std::vector<double>{48000};
    for (double sr : rates)
        for (int preset = 0; preset < 24; ++preset) {
            p->setPlayConfigDetails(2, 2, sr, 512);
            p->prepareToPlay(sr, 512);
            p->loadFactory(preset);
            expect(p->presetName == p->factory(preset)["name"].toString(),
                   "factory preset actually loads");
            p->set("mode", 0);
            p->set("build", 0);
            host.t.ppq = 0;
            host.t.bpm = 126;
            host.t.numerator = 4;
            double seconds = render ? 8 * 4 * 60 / 126. + 3 : stress ? 2 : 1;
            int total = int(sr * seconds);
            double peak = 0, sum = 0, maxDelta = 0;
            float previous[2]{};
            int samples = 0;
            std::unique_ptr<juce::AudioFormatWriter> writer;
            if (render && sr == 48000) {
                juce::WavAudioFormat wav;
                auto file = out.getChildFile(juce::String(preset + 1).paddedLeft('0', 2) + "-" +
                                             p->presetName.replaceCharacter(' ', '_') + ".wav");
                file.deleteFile();
                auto stream = file.createOutputStream();
                writer.reset(wav.createWriterFor(stream.release(), sr, 2, 24, {}, 0));
            }
            auto start = std::chrono::steady_clock::now();
            int offset = 0, block = 0;
            while (offset < total) {
                int sizes[]{1, 32, 64, 127, 512, 2048};
                int n = juce::jmin(stress ? sizes[block++ % 6] : 512, total - offset);
                juce::AudioBuffer<float> b(2, n);
                for (int i = 0; i < n; ++i)
                    for (int c = 0; c < 2; ++c)
                        b.setSample(c, i, signal((offset + i) / sr, c));
                float progress = float(offset) / float(juce::jmax(1, total - int(sr * 3)));
                if (!render)
                    progress = float(offset) / float(total);
                p->set("build", progress <= 1 ? clamp(progress) : 0);
                host.t.ppq = offset / sr * 126 / 60;
                host.t.bpm = (stress && offset > total / 2) ? 128 : 126;
                if (stress && !render && offset > total * 3 / 4)
                    host.t.numerator = 3;
                auditAudio = true;
                p->processBlock(b, midi);
                auditAudio = false;
                expect(audioAllocations == 0, "no C++ heap allocations in processBlock");
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < n; ++i) {
                        float x = b.getSample(c, i);
                        if (!std::isfinite(x)) {
                            expect(false, "finite DSP");
                            x = 0;
                        }
                        peak = juce::jmax(peak, double(std::abs(x)));
                        maxDelta = juce::jmax(maxDelta, double(std::abs(x - previous[c])));
                        previous[c] = x;
                        sum += x * x;
                        ++samples;
                    }
                if (writer)
                    writer->writeFromAudioSampleBuffer(b, 0, n);
                offset += n;
            }
            double elapsed =
                std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            metrics << p->presetName << ',' << sr << ',' << peak << ',' << std::sqrt(sum / samples)
                    << ',' << elapsed << ',' << seconds << '\n';
            expect(peak < 4, "factory output bounded");
            expect(peak < .95, "factory reference loop retains output headroom");
            expect(maxDelta < 2.5, "factory sample discontinuities bounded");
            std::cout << "Measured " << p->presetName << " " << sr << " Hz peak=" << peak
                      << " realtime=" << elapsed / seconds << '\n';
        }
    // Silence, impulse, rapid automation and block changes across all modules including nonfactory
    // LPF/EQ/utility.
    for (int module = 0; module < ModuleCount; ++module) {
        p->loadFactory(0);
        p->set("s0type", float(module));
        p->set("build", 1);
        p->prepareToPlay(48000, 512);
        for (int block = 0; block < 30; ++block) {
            juce::AudioBuffer<float> b(2, 127);
            b.clear();
            if (block == 0)
                b.setSample(0, 0, 1);
            p->set("build", block % 2 ? 0.f : 1.f);
            p->processBlock(b, midi);
            expect(std::isfinite(b.getMagnitude(0, b.getNumSamples())),
                   "impulse/silence module recovery");
        }
    }
    // Deterministic noise and logarithmic sweeps with changing tempo, meter, seeks,
    // silence, impulses, stop/start and fast macro automation on mono/stereo.
    for (double rate : {44100., 48000., 88200., 96000., 192000.})
        for (int channels : {1, 2}) {
            p->setPlayConfigDetails(channels, channels, rate, 512);
            p->prepareToPlay(rate, 512);
            p->loadFactory(11);
            uint32_t random = 1234567;
            double phase = 0, position = 0;
            for (int block = 0; block < 240; ++block) {
                int sizes[]{1, 32, 64, 127, 512, 2048};
                int n = sizes[block % 6];
                juce::AudioBuffer<float> b(channels, n);
                for (int i = 0; i < n; ++i) {
                    random = random * 1664525 + 1013904223;
                    float noise = float(double(random) / 2147483648. - 1) * .15f;
                    phase += 2 * pi * 20 * std::pow(1000., block / 240.) / rate;
                    float v = block < 40    ? 0
                              : block < 80  ? (i == 0 ? .5f : 0)
                              : block < 160 ? noise
                                            : .2f * std::sin(float(phase));
                    for (int c = 0; c < channels; ++c)
                        b.setSample(c, i, v);
                }
                host.t.bpm = block < 80 ? 90 : block < 160 ? 126 : 155;
                host.t.numerator = block < 120 ? 3 : 4;
                host.t.playing = block % 60 > 10;
                host.t.ppq = block % 48 == 0 ? 0 : position;
                position = host.t.ppq + n / rate * host.t.bpm / 60;
                p->set("build", block % 2 ? 1.f : 0.f);
                p->processBlock(b, midi);
                for (int c = 0; c < channels; ++c)
                    for (int i = 0; i < n; ++i)
                        expect(std::isfinite(b.getSample(c, i)) && std::abs(b.getSample(c, i)) < 4,
                               "sweep/noise automation and transport stress");
            }
        }
    host.t.playing = true;
    // Stereo width must preserve mono collapse, and no generator may excite at zero.
    {
        Engine e;
        e.prepare(48000, 127);
        Config c;
        c.slot[0].type = Utility;
        c.slot[0].map[1].from = c.slot[0].map[1].to = 1.5f;
        c.slot[0].map[7].from = c.slot[0].map[7].to = 1;
        MotionSettings motion;
        motion.manual = 1;
        Timing time;
        float left[127], right[127];
        float *channels[]{left, right};
        for (int block = 0; block < 200; ++block) {
            for (int i = 0; i < 127; ++i) {
                left[i] = .2f + .1f * std::sin(i * .2f);
                right[i] = .2f - .1f * std::sin(i * .2f);
            }
            e.process(channels, 2, 127, c, motion, time, 1, 0, false, false, false, false, false);
            for (int i = 0; i < 127; ++i)
                expect(std::abs((left[i] + right[i]) * .5f - .2f) < 1e-6, "width mono collapse");
        }
        for (int type : {Siren, Noise}) {
            e.reset();
            c.slot[0].type = type;
            motion.manual = 0;
            for (int i = 0; i < 127; ++i)
                left[i] = right[i] = 0;
            e.process(channels, 2, 127, c, motion, time, 1, 0, false, false, false, false, false);
            for (int i = 0; i < 127; ++i)
                expect(left[i] == 0 && right[i] == 0, "generator silent at zero");
        }
    }
    // Concurrent preset publication while audio runs, and a separate processor instance.
    {
        auto second = std::make_unique<TheTitsProcessor>();
        second->setPlayConfigDetails(2, 2, 48000, 64);
        second->prepareToPlay(48000, 64);
        p->prepareToPlay(48000, 64);
        std::atomic<bool> done{false};
        std::atomic<int> blocks{0};
        std::thread worker([&] {
            juce::AudioBuffer<float> b(2, 64);
            juce::MidiBuffer events;
            while (!done.load()) {
                b.clear();
                auditAudio = true;
                p->processBlock(b, events);
                second->processBlock(b, events);
                auditAudio = false;
                ++blocks;
            }
            expect(audioAllocations == 0, "no callback new during rapid preset publication");
        });
        while (blocks.load() == 0)
            std::this_thread::yield();
        for (int i = 0; i < 48; ++i)
            p->loadFactory(i % 24);
        done = true;
        worker.join();
        expect(blocks > 0, "multiple instances process during preset switching");
    }
    if (screens) {
        p->loadFactory(0);
        p->set("mode", 0);
        p->prepareToPlay(48000, 512);
        auto editor =
            std::unique_ptr<TheTitsEditor>(static_cast<TheTitsEditor *>(p->createEditor()));
        for (int level : {0, 25, 50, 65, 75, 100}) {
            p->set("build", level / 100.f);
            for (int j = 0; j < 100; ++j) {
                juce::AudioBuffer<float> b(2, 512);
                b.clear();
                p->processBlock(b, midi);
            }
            editor->capture(out.getChildFile("editor-" + juce::String(level) + ".png"));
        }
        auto *canvas = editor->getChildComponent(0);
        expect(canvas->getNumChildComponents() == 2, "preset and effect on artwork canvas");
        expect(canvas->findChildWithID("presets") && canvas->findChildWithID("build") &&
                   editor->findChildWithID("output"),
               "only preset, effect and output exposed");
        expect(editor->getWidth() == 768 && editor->getHeight() == 512, "compact default window");
        expect(editor->getConstrainer()->getFixedAspectRatio() == 0,
               "window has no fixed aspect ratio");
        for (auto size : {juce::Point<int>(480, 360), juce::Point<int>(1100, 450),
                          juce::Point<int>(600, 900)}) {
            editor->setSize(size.x, size.y);
            auto *dial = canvas->findChildWithID("build");
            auto bounds = editor->getLocalArea(dial, dial->getLocalBounds());
            expect(editor->getLocalBounds().contains(bounds), "dial fits arbitrary window aspect");
            expect(std::abs(bounds.getWidth() - bounds.getHeight()) <= 1,
                   "free resizing preserves circular dial");
            editor->capture(out.getChildFile("editor-" + juce::String(size.x) + "x" +
                                             juce::String(size.y) + ".png"));
        }
        editor->setSize(768, 512);
        editor->capture(out.getChildFile("editor-small.png"));
        if (auto *combo = dynamic_cast<juce::ComboBox *>(canvas->findChildWithID("presets"));
            combo && juce::Desktop::getInstance().getDisplays().getPrimaryDisplay() != nullptr) {
            editor->setVisible(true);
            combo->showPopup();
            editor->capture(out.getChildFile("editor-presets-expanded.png"));
            combo->hidePopup();
        }
        editor->setSize(1536, 1024);
        editor->capture(out.getChildFile("editor-large.png"));
        editor->setSize(1152, 768);
        editor->capture(out.getChildFile("editor-2x.png"), 2);
        for (int frame = 0; frame < (stills ? 0 : 120); ++frame) {
            editor->advanceAnimationForCapture(1. / 60);
            editor->capture(
                out.getChildFile("animation-" + juce::String(frame).paddedLeft('0', 3) + ".png"));
        }

        if (auto *slider = dynamic_cast<juce::Slider *>(
                editor->getChildComponent(0)->findChildWithID("build"))) {
            expect(slider->getDoubleClickReturnValue() == 0, "dial double-click returns to zero");
            slider->setValue(0, juce::sendNotificationSync);
            expect(p->value("build") == 0, "dial can return effect to dry");
            slider->keyPressed(juce::KeyPress(juce::KeyPress::rightKey));
            expect(p->value("build") > .0005f, "BUILD keyboard increment");
        } else
            expect(false, "BUILD control found");
    }
    // Process-call timing for the six-slot preset, excluding preparation and file I/O.
    std::ofstream cpu(out.getChildFile("cpu-six-slot.csv").getFullPathName().toStdString());
    cpu << "sample_rate,block_size,audio_seconds,process_seconds,average_core_percent,max_block_"
           "ms\n";
    for (double rate : {48000., 96000.})
        for (int size : {32, 64, 127, 512}) {
            p->setPlayConfigDetails(2, 2, rate, size);
            p->prepareToPlay(rate, size);
            p->loadFactory(11);
            p->set("mode", 0);
            p->set("build", .9f);
            host.t.bpm = 126;
            host.t.numerator = 4;
            host.t.playing = true;
            host.t.ppq = 0;
            juce::AudioBuffer<float> buffer(2, size);
            double elapsed = 0, maximum = 0;
            int blocks = int(rate * 5 / size);
            for (int block = 0; block < blocks + 100; ++block) {
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < size; ++i)
                        buffer.setSample(c, i,
                                         .12f * std::sin(float((block * size + i) * .05 + c * .2)));
                host.t.ppq = (block * size) / rate * 126 / 60;
                auto start = std::chrono::steady_clock::now();
                p->processBlock(buffer, midi);
                auto duration =
                    std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
                if (block >= 100) {
                    elapsed += duration;
                    maximum = juce::jmax(maximum, duration);
                }
            }
            double seconds = blocks * size / rate;
            cpu << rate << ',' << size << ',' << seconds << ',' << elapsed << ','
                << elapsed / seconds * 100 << ',' << maximum * 1000 << '\n';
        }
    p->setPlayHead(nullptr);
    report << "Checks: " << checks << " failures: " << failures
           << "\nJSON validation, host session roundtrip, 3/4 one-shot, next-bar wait and seek, "
              "bounded springs; 24 presets measured.\nNo listening test or DAW validation is "
              "implied.\n";
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
