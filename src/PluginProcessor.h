#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <deque>
#include "Dsp.h"
#include "Pattern.h"
#include "Library.h"

// BETONG: techno-sampler med 16 pads, 8-spårs polyrytmisk sequencer,
// techno-filter, dub-delay, rumble och slump.
class BetongProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    BetongProcessor();
    ~BetongProcessor() override;

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
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;
    static juce::String laneId (int lane, const char* name) { return "l" + juce::String (lane + 1) + "_" + name; }
    static juce::String padId (int pad, const char* name) { return "p" + juce::String (pad + 1) + "_" + name; }
    void setParam (const juce::String& id, float realValue);
    float getParam (const juce::String& id) const;

    // Mönster. GUI:t ändrar via editLane/setPattern, ljudtråden kopierar en gång per block.
    betong::Pattern getPattern() const;
    void setPattern (const betong::Pattern& p);
    template <typename Fn> void editLane (int lane, Fn&& fn)
    {
        const juce::SpinLock::ScopedLockType sl (patternLock);
        auto& l = scenes[(size_t) scene.load()].lanes[(size_t) lane];
        fn (l);
        if (l.euclid) betong::applyEuclid (l);
    }

    // Scener A-D. Byte sker på nästa takt när det spelar, direkt annars.
    int getScene() const { return scene.load(); }
    int getQueuedScene() const { return queuedScene.load(); }
    void queueScene (int s);
    void copySceneTo (int target);
    std::atomic<bool> fillHeld { false };

    std::array<std::atomic<int>, betong::kLanes> laneStep {};
    std::array<std::atomic<int>, betong::kPads> padHits {};

    // Pads
    struct PadInfo { betong::SamplePtr sample; int start = 0, end = 0; };
    PadInfo getPad (int pad) const;
    void setPad (int pad, betong::SamplePtr s, int start = -1, int end = -1);
    void setPadRange (int pad, int start, int end);
    bool loadFileToPad (int pad, const juce::File& file);
    void loadBuiltInToPad (int pad, const juce::String& name);
    int chopToPads (int sourcePad, int mode, float sensitivity); // returnerar antal bitar
    void triggerPad (int pad);                                    // från GUI:t
    void triggerPadPitched (int pad, int semi);                   // provlyssna en ton
    int laneForPad (int pad) const;

    // Presets och slump
    void applyKit (int index);
    void applyPattern (int index);
    void applyFx (int index);
    void randomKit();
    void randomPattern();
    void randomFx();
    void randomAll();
    bool undoRandom();
    bool canUndo() const { return ! undoStack.empty(); }
    juce::String currentKit, currentPatternName, currentFx;
    std::atomic<int> stateVersion { 0 }; // ökar när något stort ändras, så GUI:t ritar om

    int selectedPad = 0, selectedLane = 0;

    // Egna sample-mappar (sparas globalt för alla projekt)
    juce::StringArray getSampleFolders() const;
    void addSampleFolder (const juce::File& folder);
    void removeSampleFolder (const juce::String& path);
    juce::Array<juce::File> scanSampleFolders() const;
    static constexpr const char* audioWildcard = "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg";

    betong::BuiltInLibrary& library() { return *builtIn; }
    juce::AudioFormatManager formats;

    std::atomic<bool> internalPlay { false };
    std::atomic<bool> hostPlaying { false };
    std::atomic<double> currentBpm { 130.0 };

private:
    void timerCallback() override;
    void pushUndo();
    juce::ValueTree buildState();
    void restoreState (const juce::ValueTree& tree);

    // ljudtråden
    struct Voice
    {
        const betong::Sample* s = nullptr;
        double pos = 0, inc = 1;
        int start = 0, end = 0, delay = 0, lane = 0, pad = 0;
        float gain = 1, gl = 1, gr = 1, amp = 1, decayCoef = 1, fade = 1, fadeStep = 0;
        bool releasing = false;
        int64_t samplesLeft = 0;
    };
    // flags: 1 = spela baklänges (växlar padens läge), 2 = kort slag (glitch)
    void startVoice (int pad, int lane, float gain, int offset, bool chokeLane, float semi = 0.0f, int flags = 0);
    void renderVoices (int numSamples);
    void runSequencer (int numSamples, double ppqStart, double bpm);
    betong::Step effectiveStep (int lane, juce::int64 n, int depth) const;   // steget som faktiskt spelas
    float hash01 (juce::int64 n, int lane, juce::uint32 salt) const;

    juce::SharedResourcePointer<betong::BuiltInLibrary> builtIn;
    std::unique_ptr<juce::PropertiesFile> settings;

    mutable juce::SpinLock patternLock;
    std::array<betong::Pattern, betong::kScenes> scenes;
    betong::Pattern audioPattern;
    std::array<betong::Pattern, betong::kScenes> audioScenes;
    std::atomic<int> scene { 0 }, queuedScene { -1 };

    struct PadSlot { std::atomic<const betong::Sample*> sample { nullptr }; std::atomic<int> start { 0 }, end { 0 }; };
    std::array<PadSlot, betong::kPads> padSlots;
    std::array<betong::SamplePtr, betong::kPads> padOwners;               // meddelandetråden äger
    std::vector<std::pair<betong::SamplePtr, juce::uint32>> graveyard;    // gamla ljud som kan spelas klart

    std::array<Voice, 48> voices;
    juce::AbstractFifo padFifo { 64 };
    std::array<int, 64> padFifoData {};

    static constexpr int kBuses = betong::kLanes + 1; // 8 spår + 1 direkt
    std::array<juce::AudioBuffer<float>, kBuses> laneBufs;
    std::array<betong::LaneChain, betong::kLanes> chains;
    std::array<bool, betong::kLanes> laneTriggered {};
    std::array<float, betong::kLanes> laneTrigAmount {};
    juce::AudioBuffer<float> sendA, sendB, delayOut, rumbleOut;
    betong::DubDelay delay;
    betong::Rumble rumble;
    betong::Stutter stutter;
    betong::Svf masterSvf[2];
    betong::OnePole sideLp;
    float mHoldL = 0, mHoldR = 0, mHoldCount = 0;
    juce::Random audioRng;
    double sr = 44100.0;
    int maxBlock = 512;
    double internalPpq = 0;
    bool wasInternal = false;
    std::atomic<float> sweepSmoothed { 0 };

    // parameter-pekare
    std::atomic<float>* pSwing = nullptr; std::atomic<float>* pBpm = nullptr;
    std::atomic<float>* pSweep = nullptr; std::atomic<float>* pClip = nullptr; std::atomic<float>* pMCrush = nullptr;
    std::atomic<float>* pOut = nullptr;
    std::atomic<float>* pDTime = nullptr; std::atomic<float>* pDFb = nullptr; std::atomic<float>* pDTone = nullptr;
    std::atomic<float>* pDWob = nullptr; std::atomic<float>* pDFreeze = nullptr; std::atomic<float>* pDRet = nullptr;
    std::atomic<float>* pRSize = nullptr; std::atomic<float>* pRDrive = nullptr; std::atomic<float>* pRCut = nullptr;
    std::atomic<float>* pRDuck = nullptr; std::atomic<float>* pRRet = nullptr;
    std::atomic<float>* pStutter = nullptr; std::atomic<float>* pStutLen = nullptr; std::atomic<float>* pFill = nullptr;
    std::atomic<float>* pSvaj = nullptr; std::atomic<float>* pGlitch = nullptr; std::atomic<float>* pDens = nullptr;
    juce::uint32 glitchSeed = 0x5eed;
    struct LaneParams { std::atomic<float>* type; std::atomic<float>* cut; std::atomic<float>* res; std::atomic<float>* drive;
                        std::atomic<float>* env; std::atomic<float>* dec; std::atomic<float>* crush;
                        std::atomic<float>* sendA; std::atomic<float>* sendB; std::atomic<float>* vol; std::atomic<float>* pan;
                        std::atomic<float>* swing; };
    std::array<LaneParams, betong::kLanes> lp {};
    struct PadParams { std::atomic<float>* pitch; std::atomic<float>* decay; std::atomic<float>* rev;
                       std::atomic<float>* vol; std::atomic<float>* pan; };
    std::array<PadParams, betong::kPads> pp {};

    std::deque<juce::String> undoStack;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BetongProcessor)
};
