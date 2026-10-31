#pragma once
#include "PluginProcessor.h"
#include "UI/ArtworkRenderer.h"
class PosterLook final : public juce::LookAndFeel_V4 {
  public:
    PosterLook();
    void drawRotarySlider(juce::Graphics &, int, int, int, int, float, float, float,
                          juce::Slider &) override;
    void drawComboBox(juce::Graphics &, int, int, bool, int, int, int, int,
                      juce::ComboBox &) override;
    void positionComboBoxText(juce::ComboBox &, juce::Label &) override;
    juce::Font getComboBoxFont(juce::ComboBox &) override;
    juce::Font getPopupMenuFont() override;
    bool shouldPopupMenuScaleWithTargetComponent(const juce::PopupMenu::Options &) override {
        return false;
    }
    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu(juce::ComboBox &,
                                                            juce::Label &) override;
    void drawLinearSlider(juce::Graphics &, int, int, int, int, float, float, float,
                          const juce::Slider::SliderStyle, juce::Slider &) override;
    juce::Typeface::Ptr typeface, boldTypeface;

  private:
    juce::Image poster, selectorSkin, faceMaterial, dialSkin;
};
class TheTitsEditor final : public juce::AudioProcessorEditor, private juce::Timer {
  public:
    explicit TheTitsEditor(TheTitsProcessor &);
    ~TheTitsEditor() override;
    void paint(juce::Graphics &) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress &) override;
    void capture(const juce::File &, double snapshotScale = 1);
    void advanceAnimationForCapture(double seconds);

  private:
    TheTitsProcessor &p;
    PosterLook look;
    tits::ArtworkRenderer artwork;
    tits::JiggleAnimator jiggle;
    juce::Component canvas;
    juce::TooltipWindow tooltip;
    juce::Slider build, output;
    juce::ComboBox presets;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> buildAttach, outputAttach;
    double lastTime = 0;
    float current = 0;
    void timerCallback() override;
    juce::AffineTransform sceneTransform() const;
};
