#pragma once
#include "DSP/Engine.h"
#include <juce_audio_utils/juce_audio_utils.h>
class TheTitsProcessor final : public juce::AudioProcessor {
  public:
    TheTitsProcessor();
    ~TheTitsProcessor() override = default;
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout &) const override;
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    void processBlockBypassed(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override {
        return true;
    }
    const juce::String getName() const override {
        return "The Tits";
    }
    bool acceptsMidi() const override {
        return false;
    }
    bool producesMidi() const override {
        return false;
    }
    double getTailLengthSeconds() const override {
        return 420;
    }
    int getNumPrograms() override {
        return 1;
    }
    int getCurrentProgram() override {
        return 0;
    }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override {
        return {};
    }
    void changeProgramName(int, const juce::String &) override {}
    void getStateInformation(juce::MemoryBlock &) override;
    void setStateInformation(const void *, int) override;
    juce::AudioProcessorParameter *getBypassParameter() const override {
        return state.getParameter("bypass");
    }
    juce::AudioProcessorValueTreeState state;
    tits::Engine engine;
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    float value(const juce::String &) const;
    void set(const juce::String &, float);
    tits::Config config() const;
    tits::MotionSettings motion() const;
    void run(juce::AudioBuffer<float> &, bool);
    std::atomic<bool> triggerRequested{false}, resetRequested{false}, panicRequested{false};
    std::atomic<int> presetIndex{0};
    std::atomic<bool> dirty{false};
    std::atomic<double> tempo{126};
    bool importJSON(const juce::String &, juce::String &error);
    juce::String exportJSON();
    void loadFactory(int, bool recallPerformance = false);
    int factoryCount() const;
    juce::var factory(int) const;
    juce::String presetName = "Disco Lift", category = "Mix bus";
    std::atomic<bool> loading{false};
    std::atomic<unsigned> configGeneration{0};
    std::atomic<float> animationAmount{1};
    std::atomic<bool> reducedMotion{false};

  private:
    bool triggerWas = false, resetWas = false;
    double fallbackPPQ = 0;
    tits::Config audioConfig{};
    juce::var factoryBank;
    std::vector<std::atomic<float> *> realtime;
};
