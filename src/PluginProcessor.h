#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

// BETONG steg 1: en enkel sampler som spelar ett ljud på alla MIDI-toner.
// Standardljudet är en inbyggd kick. Användaren kan ladda en egen fil.
class BetongProcessor : public juce::AudioProcessor
{
public:
    BetongProcessor();
    ~BetongProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BETONG"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Laddar en ljudfil. Returnerar false om filen inte gick att läsa.
    bool loadSampleFile (const juce::File& file);
    void loadBuiltInKick();

    juce::String getSampleName() const;

    // Egna sample-mappar. Sparas globalt, så de finns i alla projekt.
    juce::StringArray getSampleFolders() const;
    void addSampleFolder (const juce::File& folder);
    void removeSampleFolder (const juce::String& path);
    juce::Array<juce::File> scanSampleFolders() const;
    static constexpr const char* audioWildcard = "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg";

    // Senaste spelade tonen, så att GUI:t kan blinka.
    std::atomic<int> lastNoteCounter { 0 };

    // Låter GUI-paden spela ljudet utan MIDI-keyboard.
    juce::MidiKeyboardState keyboardState;

private:
    bool loadFromReader (std::unique_ptr<juce::AudioFormatReader> reader, const juce::String& name);

    juce::AudioFormatManager formatManager;
    juce::Synthesiser synth;
    juce::File currentFile;
    juce::String sampleName;
    mutable juce::CriticalSection nameLock;

    std::unique_ptr<juce::PropertiesFile> settings;

    static constexpr int numVoices = 16;
    static constexpr int rootNote = 60; // C3 spelar ljudet i originaltonhöjd

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BetongProcessor)
};
