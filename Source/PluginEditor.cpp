#include "PluginEditor.h"
#include <BinaryData.h>
namespace {
const juce::Colour ink(0xff4d1009), hotPink(0xffef146b), ivory(0xfffff4db);
void ellipse(juce::Graphics &g, juce::Rectangle<float> b, juce::Colour a, juce::Colour z,
             bool radial = false) {
    g.setGradientFill(juce::ColourGradient(a, b.getX() + b.getWidth() * .28f,
                                           b.getY() + b.getHeight() * .18f, z, b.getRight(),
                                           b.getBottom(), radial));
    g.fillEllipse(b);
}
} // namespace
PosterLook::PosterLook() {
    typeface =
        juce::Typeface::createSystemTypefaceFor(BinaryData::Roboto_ttf, BinaryData::Roboto_ttfSize);
    boldTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::RobotoBold_ttf,
                                                           BinaryData::RobotoBold_ttfSize);
    poster = juce::ImageCache::getFromMemory(BinaryData::poster_png, BinaryData::poster_pngSize);
    // Prepare a seamless dynamic selector skin from the reference, keeping its
    // real bevel and arrow while replacing only the baked sample-name area.
    auto original = poster.getClippedImage({45, 80, 558, 155}).createCopy();
    for (int y = 0; y < 155; ++y)
        for (int x = 64; x < 453; ++x) {
            float t = (x - 64) / 388.f;
            original.setPixelAt(
                x, y,
                poster.getPixelAt(109, y + 80).interpolatedWith(poster.getPixelAt(498, y + 80), t));
        }
    selectorSkin = juce::Image(juce::Image::ARGB, 558, 155, true);
    juce::Graphics skinGraphics(selectorSkin);
    juce::Path buttonShape;
    buttonShape.addRoundedRectangle(10, 14, 530, 110, 38);
    juce::DropShadow(ink.withAlpha(.55f), 12, {4, 9}).drawForPath(skinGraphics, buttonShape);
    skinGraphics.reduceClipRegion(buttonShape);
    skinGraphics.setOpacity(1);
    skinGraphics.drawImageAt(original, 0, 0);
    faceMaterial = poster.getClippedImage({622, 438, 146, 146});
    dialSkin = poster.getClippedImage({512, 328, 512, 512}).createCopy();
    // Colour-correct a clean mirrored sector into the small baked-pointer region.
    // Median paint colours exclude calibration ink; feathering prevents visible patch edges.
    auto paintMedian = [&](int radius, float start, float finish) {
        std::array<float, 31> r{}, g{}, b{};
        int n = 0;
        for (int i = 0; i < 31; ++i) {
            float angle = start + (finish - start) * i / 30;
            auto c = dialSkin.getPixelAt(int(256 + std::sin(angle) * radius),
                                         int(256 - std::cos(angle) * radius));
            if (c.getFloatRed() > .62f) {
                r[n] = c.getFloatRed();
                g[n] = c.getFloatGreen();
                b[n] = c.getFloatBlue();
                ++n;
            }
        }
        if (n == 0)
            return juce::Colour(0xffedb99c);
        std::sort(r.begin(), r.begin() + n);
        std::sort(g.begin(), g.begin() + n);
        std::sort(b.begin(), b.begin() + n);
        return juce::Colour::fromFloatRGBA(r[n / 2], g[n / 2], b[n / 2], 1);
    };
    std::array<juce::Colour, 205> a{}, b{}, ma{}, mb{};
    for (int r = 125; r <= 204; ++r) {
        a[r] = paintMedian(r, .20f, .38f);
        b[r] = paintMedian(r, 1.01f, 1.16f);
        ma[r] = paintMedian(r, -.38f, -.20f);
        mb[r] = paintMedian(r, -1.16f, -1.01f);
    }
    auto originalDial = dialSkin.createCopy();
    for (int y = 40; y < 256; ++y)
        for (int x = 256; x < 470; ++x) {
            double dx = x - 256, dy = 256 - y;
            double radius = std::hypot(dx, dy), angle = std::atan2(dx, dy);
            if (radius < 125 || radius > 204 || angle < .4 || angle > .98)
                continue;
            int r = int(radius);
            float t = float((angle - .4) / .58);
            auto source = originalDial.getPixelAt(512 - x, y);
            auto ta = a[r].interpolatedWith(b[r], t), sm = ma[r].interpolatedWith(mb[r], t);
            auto corrected = juce::Colour::fromFloatRGBA(
                tits::clamp(source.getFloatRed() + ta.getFloatRed() - sm.getFloatRed()),
                tits::clamp(source.getFloatGreen() + ta.getFloatGreen() - sm.getFloatGreen()),
                tits::clamp(source.getFloatBlue() + ta.getFloatBlue() - sm.getFloatBlue()), 1);
            float feather = std::min({1.f, float((angle - .4) / .085), float((.98 - angle) / .085),
                                      float((204 - radius) / 9), float((radius - 125) / 8)});
            dialSkin.setPixelAt(
                x, y,
                originalDial.getPixelAt(x, y).interpolatedWith(corrected, tits::clamp(feather)));
        }

    setDefaultSansSerifTypeface(typeface);
    setColour(juce::ComboBox::textColourId, juce::Colours::white);
    setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xffffe4a2));
    setColour(juce::PopupMenu::textColourId, ink);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, hotPink);
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour(juce::Slider::textBoxTextColourId, ink);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}
void PosterLook::drawComboBox(juce::Graphics &g, int width, int height, bool down, int, int, int,
                              int, juce::ComboBox &box) {
    g.setOpacity(1);
    g.addTransform(juce::AffineTransform::scale(width / 558.f, height / 155.f));
    g.drawImageAt(selectorSkin, 0, 0);
    if (down) {
        g.setColour(ink.withAlpha(.10f));
        g.fillRoundedRectangle(10, 14, 530, 110, 38);
    }
    if (box.hasKeyboardFocus(true)) {
        g.setColour(ivory);
        g.drawRoundedRectangle(14, 18, 522, 102, 34, 2);
    }
}
juce::Font PosterLook::getComboBoxFont(juce::ComboBox &) {
    return juce::Font(juce::FontOptions(boldTypeface).withHeight(29));
}
juce::Font PosterLook::getPopupMenuFont() {
    return juce::Font(juce::FontOptions(typeface).withHeight(13));
}
juce::PopupMenu::Options PosterLook::getOptionsForComboBoxPopupMenu(juce::ComboBox &box,
                                                                    juce::Label &label) {
    return juce::LookAndFeel_V4::getOptionsForComboBoxPopupMenu(box, label)
        .withParentComponent(box.findParentComponentOfClass<juce::AudioProcessorEditor>())
        .withMinimumWidth(180)
        .withStandardItemHeight(20)
        .withMinimumNumColumns(2)
        .withMaximumNumColumns(2);
}
void PosterLook::positionComboBoxText(juce::ComboBox &box, juce::Label &label) {
    label.setBounds(44, 20, box.getWidth() - 110, 48);
    label.setFont(getComboBoxFont(box));
    label.setJustificationType(juce::Justification::centredLeft);
}
void PosterLook::drawLinearSlider(juce::Graphics &g, int x, int y, int w, int h, float pos, float,
                                  float, const juce::Slider::SliderStyle, juce::Slider &s) {
    auto cy = y + h * .5f;
    g.setColour(ink.withAlpha(.28f));
    g.fillRoundedRectangle(float(x), cy - 3, float(w), 6, 3);
    g.setColour(hotPink);
    g.fillRoundedRectangle(float(x), cy - 3, juce::jmax(1.f, pos - x), 6, 3);
    ellipse(g, {pos - 9, cy - 9, 18, 18}, ivory, juce::Colour(0xffca6c64));
    g.setColour(ink);
    g.drawEllipse(pos - 9, cy - 9, 18, 18, 1.2f);
    if (s.hasKeyboardFocus(true)) {
        g.setColour(juce::Colours::white);
        g.drawEllipse(pos - 12, cy - 12, 24, 24, 1.5f);
    }
}
void PosterLook::drawRotarySlider(juce::Graphics &g, int x, int y, int w, int h, float position,
                                  float, float, juce::Slider &s) {
    // Preserve the reference's photographed rim and highlights. The entire inner dial,
    // including its pointer and percentage, is functional and redrawn from the parameter.
    auto bounds = juce::Rectangle<float>(float(x), float(y), float(w), float(h));
    float scale = juce::jmin(w / 512.f, h / 512.f);
    auto centre = bounds.getCentre();
    g.addTransform(juce::AffineTransform::translation(-256, -256)
                       .scaled(scale)
                       .translated(centre.x, centre.y));
    juce::Path rim;
    rim.addEllipse(2, 2, 508, 508);
    g.saveState();
    g.reduceClipRegion(rim);
    g.setOpacity(1);
    g.drawImageAt(dialSkin, 0, 0);
    g.restoreState();
    const float first = -2.25f, last = 2.25f, angle = first + position * (last - first);
    // Reuse the clean upper-left material in all quadrants, shaded directionally.
    // This removes the sample percentage and pointer without flattening the texture.
    juce::Path face;
    face.addEllipse(118, 118, 276, 276);
    g.saveState();
    g.reduceClipRegion(face);
    g.drawImageAt(faceMaterial, 110, 110);
    g.drawImageTransformed(faceMaterial, juce::AffineTransform::scale(-1, 1).translated(402, 110));
    g.drawImageTransformed(faceMaterial, juce::AffineTransform::scale(1, -1).translated(110, 402));
    g.drawImageTransformed(faceMaterial, juce::AffineTransform::scale(-1, -1).translated(402, 402));
    g.setGradientFill(juce::ColourGradient(juce::Colours::transparentBlack, 180, 150,
                                           juce::Colour(0xff833c24).withAlpha(.50f), 340, 385,
                                           false));
    g.fillEllipse(118, 118, 276, 276);
    g.restoreState();
    // Rotate the reference's actual sculpted indicator, rather than substituting a flat line.
    juce::Path handle;
    handle.startNewSubPath(786, 550);
    handle.cubicTo(798, 529, 839, 472, 866, 441);
    handle.cubicTo(875, 430, 888, 432, 894, 440);
    handle.cubicTo(899, 448, 890, 462, 881, 474);
    handle.lineTo(821, 562);
    handle.cubicTo(812, 576, 801, 580, 790, 573);
    handle.cubicTo(780, 568, 778, 561, 786, 550);
    handle.closeSubPath();
    auto pointerTransform =
        juce::AffineTransform::rotation(angle - .675f, 768, 584).translated(-512, -328);
    auto shadow = handle;
    shadow.applyTransform(pointerTransform.translated(4, 5));
    g.setColour(ink.withAlpha(.25f));
    g.fillPath(shadow);
    g.saveState();
    g.setOpacity(1);
    g.reduceClipRegion(handle, pointerTransform);
    g.drawImageTransformed(poster, pointerTransform);
    g.restoreState();
    auto outline = handle;
    outline.applyTransform(pointerTransform);
    g.setColour(juce::Colour(0xffffe9d7));
    g.strokePath(outline, juce::PathStrokeType(1.4f));
    juce::Path indicator;
    indicator.startNewSubPath(824, 523);
    indicator.lineTo(881, 447);
    indicator.applyTransform(pointerTransform);
    g.setColour(juce::Colour(0xffff165c));
    g.strokePath(indicator, juce::PathStrokeType(5.f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions(boldTypeface).withHeight(27)));
    g.drawText("EFFECT %", juce::Rectangle<float>{132, 278, 248, 38}, juce::Justification::centred);
    g.setFont(juce::Font(juce::FontOptions("Georgia", 71, juce::Font::bold)));
    g.drawText(juce::String(int(std::round(position * 100))) + "%",
               juce::Rectangle<float>{125, 310, 262, 83}, juce::Justification::centred);
    if (s.hasKeyboardFocus(true)) {
        g.setColour(ivory);
        g.drawEllipse(8, 8, 496, 496, 2);
    }
}
TheTitsEditor::TheTitsEditor(TheTitsProcessor &processor)
    : AudioProcessorEditor(processor), p(processor), tooltip(this, 650) {
    setLookAndFeel(&look);
    addAndMakeVisible(canvas);
    canvas.addAndMakeVisible(presets);
    canvas.addAndMakeVisible(build);
    addAndMakeVisible(output);
    presets.setComponentID("presets");
    presets.setName("Preset");
    for (int i = 0; i < p.factoryCount(); ++i)
        presets.addItem(i == 0 ? juce::String("Jiggle Nipples") : p.factory(i)["name"].toString(),
                        i + 1);
    presets.setSelectedId(p.presetIndex.load() + 1, juce::dontSendNotification);
    presets.setTooltip(
        "Choose a transition preset. The effect position and output level are preserved.");
    presets.onChange = [this] {
        int index = presets.getSelectedId() - 1;
        if (index >= 0)
            p.loadFactory(index);
    };
    build.setComponentID("build");
    build.setName("Main Effect");
    build.setTooltip(
        "Effect intensity. Drag or scroll; arrow keys adjust; double-click returns to zero.");
    build.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    build.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    build.setRotaryParameters(-2.25f, 2.25f, true);
    build.setWantsKeyboardFocus(true);
    build.setMouseClickGrabsKeyboardFocus(true);
    build.setDoubleClickReturnValue(true, 0);
    build.setMouseDragSensitivity(300);
    buildAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.state, "build", build);
    build.setRange(0, 1, .001);
    output.setComponentID("output");
    output.setName("Output level");
    output.setTooltip("Output level: -24 to +6 dB. Double-click restores unity.");
    output.setSliderStyle(juce::Slider::LinearHorizontal);
    output.setTextBoxStyle(juce::Slider::TextBoxRight, false, 76, 28);
    output.setDoubleClickReturnValue(true, 0);
    outputAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.state, "output", output);
    output.textFromValueFunction = [](double v) { return juce::String(v, 1) + " dB"; };
    output.valueFromTextFunction = [](const juce::String &v) { return v.getDoubleValue(); };
    output.updateText();
    setResizable(true, true);
    setResizeLimits(480, 360, 2560, 1600);
    setSize(768, 512);
    setWantsKeyboardFocus(true);
    lastTime = juce::Time::getMillisecondCounterHiRes() * .001;
    startTimerHz(60);
}
TheTitsEditor::~TheTitsEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}
void TheTitsEditor::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(0xfff6ce51));
    g.saveState();
    g.addTransform(sceneTransform());
    artwork.background(g, {0, 0, 1536, 1024});
    artwork.draw(g, {0, 0, 1536, 1024}, jiggle);
    g.setColour(ink.withAlpha(.45f));
    g.setFont(juce::Font(juce::FontOptions(look.boldTypeface).withHeight(32)));
    g.drawText("Preset", juce::Rectangle<float>{66, 47, 400, 47}, juce::Justification::centredLeft);
    g.setColour(juce::Colours::white);
    g.drawText("Preset", juce::Rectangle<float>{63, 44, 400, 47}, juce::Justification::centredLeft);
    g.restoreState();
    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions(look.boldTypeface).withHeight(12)));
    g.drawText("OUTPUT", getWidth() - 304, getHeight() - 29, 62, 22,
               juce::Justification::centredLeft);
    g.setFont(juce::Font(juce::FontOptions(look.typeface).withHeight(12)));
    g.drawText("v" JucePlugin_VersionString, 12, getHeight() - 26, 120, 18,
               juce::Justification::centredLeft);
}
juce::AffineTransform TheTitsEditor::sceneTransform() const {
    float scale = juce::jmin(getWidth() / 1536.f, getHeight() / 1024.f);
    return juce::AffineTransform::scale(scale).translated((getWidth() - 1536 * scale) * .5f,
                                                          (getHeight() - 1024 * scale) * .5f);
}
void TheTitsEditor::resized() {
    canvas.setBounds(0, 0, 1536, 1024);
    canvas.setTransform(sceneTransform());
    presets.setBounds(45, 80, 380, 102);
    build.setBounds(512, 328, 512, 512);
    output.setBounds(getWidth() - 244, getHeight() - 31, 216, 24);
}
void TheTitsEditor::timerCallback() {
    double now = juce::Time::getMillisecondCounterHiRes() * .001;
    current = p.engine.effective.load();
    jiggle.step(now - lastTime, current, p.animationAmount.load(), p.reducedMotion.load());
    lastTime = now;
    int selected = p.presetIndex.load() + 1;
    if (presets.getSelectedId() != selected)
        presets.setSelectedId(selected, juce::dontSendNotification);
    repaint();
    startTimer(current < .0001f && std::abs(jiggle.springs[0].x) < .0001 ? 66 : 16);
}
bool TheTitsEditor::keyPressed(const juce::KeyPress &k) {
    if ((k == juce::KeyPress::rightKey || k == juce::KeyPress::upKey) &&
        build.hasKeyboardFocus(true)) {
        p.set("build", tits::clamp(p.value("build") + .001f));
        return true;
    }
    if ((k == juce::KeyPress::leftKey || k == juce::KeyPress::downKey) &&
        build.hasKeyboardFocus(true)) {
        p.set("build", tits::clamp(p.value("build") - .001f));
        return true;
    }
    return false;
}
void TheTitsEditor::advanceAnimationForCapture(double seconds) {
    jiggle.step(seconds, p.engine.effective.load(), p.animationAmount.load(),
                p.reducedMotion.load());
}
void TheTitsEditor::capture(const juce::File &file, double snapshotScale) {
    timerCallback();
    auto image = createComponentSnapshot(getLocalBounds(), true, float(snapshotScale));
    file.deleteFile();
    juce::FileOutputStream stream(file);
    juce::PNGImageFormat().writeImageToStream(image, stream);
}
