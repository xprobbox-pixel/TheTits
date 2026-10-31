#include "PluginProcessor.h"
// Custom shell: the standalone shows the same three controls as the plugin.
// Audio files can be dropped onto the artwork; read-ahead runs on a worker thread.
class AuditionAudio final : public juce::AudioIODeviceCallback {
  public:
    TheTitsProcessor processor;
    AuditionAudio() : readThread("The Tits file read-ahead") {
        formats.registerBasicFormats();
        readThread.startThread();
        devices.initialise(0, 2, nullptr, true);
        devices.addAudioCallback(this);
    }
    ~AuditionAudio() override {
        devices.removeAudioCallback(this);
        source.stop();
        source.setSource(nullptr);
        reader.reset();
        readThread.stopThread(2000);
    }
    bool load(const juce::File &file) {
        std::unique_ptr<juce::AudioFormatReader> next(formats.createReaderFor(file));
        if (!next)
            return false;
        devices.removeAudioCallback(this);
        source.stop();
        source.setSource(nullptr);
        auto rate = next->sampleRate;
        reader = std::make_unique<juce::AudioFormatReaderSource>(next.release(), true);
        reader->setLooping(true);
        source.setSource(reader.get(), 32768, &readThread, rate, 2);
        devices.addAudioCallback(this);
        source.start();
        return true;
    }
    void toggle() {
        if (source.isPlaying())
            source.stop();
        else if (reader)
            source.start();
    }
    void audioDeviceAboutToStart(juce::AudioIODevice *device) override {
        auto sr = device->getCurrentSampleRate();
        auto block = device->getCurrentBufferSizeSamples();
        work.setSize(2, juce::jmax(8192, block));
        processor.setPlayConfigDetails(2, 2, sr, block);
        processor.prepareToPlay(sr, block);
        source.prepareToPlay(block, sr);
    }
    void audioDeviceStopped() override {
        source.releaseResources();
        processor.releaseResources();
    }
    void audioDeviceIOCallbackWithContext(const float *const *, int, float *const *output,
                                          int channels, int count,
                                          const juce::AudioIODeviceCallbackContext &) override {
        int offset = 0;
        while (offset < count) {
            int n = juce::jmin(count - offset, work.getNumSamples());
            juce::AudioBuffer<float> block(work.getArrayOfWritePointers(), 2, n);
            block.clear();
            source.getNextAudioBlock({&block, 0, n});
            processor.processBlock(block, midi);
            for (int c = 0; c < channels; ++c)
                if (output[c])
                    juce::FloatVectorOperations::copy(output[c] + offset,
                                                      block.getReadPointer(juce::jmin(c, 1)), n);
            offset += n;
        }
    }

  private:
    juce::AudioDeviceManager devices;
    juce::AudioFormatManager formats;
    juce::TimeSliceThread readThread;
    std::unique_ptr<juce::AudioFormatReaderSource> reader;
    juce::AudioTransportSource source;
    juce::AudioBuffer<float> work;
    juce::MidiBuffer midi;
};
class PosterWindow final : public juce::DocumentWindow, public juce::FileDragAndDropTarget {
  public:
    PosterWindow(AuditionAudio &a)
        : DocumentWindow("The Tits", juce::Colour(0xfff6ce51), allButtons), audio(a) {
        setUsingNativeTitleBar(true);
        setContentOwned(audio.processor.createEditor(), true);
        setResizable(true, false);
        setResizeLimits(480, 360, 2560, 1600);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }
    void closeButtonPressed() override {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
    bool isInterestedInFileDrag(const juce::StringArray &files) override {
        return files.size() == 1 &&
               juce::File(files[0]).hasFileExtension("wav;aif;aiff;flac;mp3;ogg");
    }
    void filesDropped(const juce::StringArray &files, int, int) override {
        if (!audio.load(juce::File(files[0])))
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon, "Audio file could not be opened",
                "Drop a supported WAV, AIFF, FLAC, MP3 or Ogg file.");
    }
    bool keyPressed(const juce::KeyPress &key) override {
        if (key == juce::KeyPress::spaceKey) {
            audio.toggle();
            return true;
        }
        return DocumentWindow::keyPressed(key);
    }

  private:
    AuditionAudio &audio;
};
class TheTitsApplication final : public juce::JUCEApplication {
  public:
    const juce::String getApplicationName() override {
        return "The Tits";
    }
    const juce::String getApplicationVersion() override {
        return JucePlugin_VersionString;
    }
    void initialise(const juce::String &) override {
        audio = std::make_unique<AuditionAudio>();
        window = std::make_unique<PosterWindow>(*audio);
    }
    void shutdown() override {
        window.reset();
        audio.reset();
    }

  private:
    std::unique_ptr<AuditionAudio> audio;
    std::unique_ptr<PosterWindow> window;
};
START_JUCE_APPLICATION(TheTitsApplication)
