#pragma once

#include "PluginProcessor.h"
#include "Gui.h"
#include "About.h"

class BetongEditor;

//==============================================================================
class DiceButton : public juce::Button
{
public:
    explicit DiceButton (const juce::String& label, bool filled = false) : juce::Button (label), solid (filled) {}
    void paintButton (juce::Graphics&, bool over, bool down) override;
private:
    bool solid;
};

//==============================================================================
class WaveView : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    explicit WaveView (BetongProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void filesDropped (const juce::StringArray& files, int, int) override;
    void refresh() { repaint(); }

private:
    float xForSample (int s, int len) const;
    int sampleForX (float x, int len) const;
    void rebuildPeaks (const betong::Sample& s);

    BetongProcessor& proc;
    const betong::Sample* peaksFor = nullptr;
    std::vector<std::pair<float, float>> peaks;
    int dragEdge = 0; // 1 start, 2 slut
};

//==============================================================================
class PadGrid : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    PadGrid (BetongProcessor& p, std::function<void()> onSelect) : proc (p), selectChanged (std::move (onSelect)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void filesDropped (const juce::StringArray& files, int x, int y) override;
    void tick();

private:
    int padAt (juce::Point<int> p) const;
    juce::Rectangle<int> padBounds (int i) const;
    void showPadMenu (int pad);

    BetongProcessor& proc;
    std::function<void()> selectChanged;
    std::array<float, betong::kPads> flash {};
    std::array<int, betong::kPads> seenHits {};
    std::unique_ptr<juce::FileChooser> chooser;
};

//==============================================================================
class LibraryPanel : public juce::Component, private juce::ListBoxModel
{
public:
    LibraryPanel (BetongProcessor& p, std::function<void()> onLoaded);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();

private:
    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;

    struct Cats : public juce::ListBoxModel
    {
        LibraryPanel& owner;
        explicit Cats (LibraryPanel& o) : owner (o) {}
        int getNumRows() override;
        void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
        void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    } catsModel { *this };

    void selectCategory (int index);

    BetongProcessor& proc;
    std::function<void()> loaded;
    juce::ListBox catList, itemList;
    juce::TextButton addFolder, removeFolder, closeButton;
    juce::ComboBox folderBox;
    juce::StringArray builtInItems;
    juce::Array<juce::File> fileItems;
    int category = 0;
    std::unique_ptr<juce::FileChooser> chooser;
};

//==============================================================================
class StepGrid : public juce::Component
{
public:
    StepGrid (BetongProcessor& p, int laneIndex) : proc (p), lane (laneIndex) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    int stepAt (int x) const;
    void showStepMenu (int step);
    BetongProcessor& proc;
    int lane;
    bool paintValue = true;
    int lastPainted = -1;
};

class LaneRow : public juce::Component
{
public:
    LaneRow (BetongProcessor& p, int laneIndex, std::function<void (int)> onSelect);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void refresh();
    StepGrid grid;

private:
    BetongProcessor& proc;
    int lane;
    std::function<void (int)> selectLane;
    juce::ComboBox padBox, rateBox;
    juce::Slider lenSlider, hitsSlider, rotSlider;
    juce::TextButton euclidButton { "E" }, muteButton { "M" }, soloButton { "S" }, lockButton;
    juce::Rectangle<int> labelArea;
};

//==============================================================================
class BetongContent : public juce::Component, private juce::Timer
{
public:
    explicit BetongContent (BetongProcessor&);
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    ~BetongContent() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int W = 1280, H = 820;

private:
    void timerCallback() override;
    void selectionChanged();
    void selectLane (int lane);
    void refreshPresetBoxes();
    void openFileForSelectedPad();

    BetongProcessor& proc;
    BetongLnF lnf;

    // header
    juce::ComboBox kitBox, patternBox, fxBox;
    DiceButton diceKit { "KIT" }, diceFx { "FX" }, dicePattern { utf8 ("MÖNSTER") }, diceAll { "ALLT", true };
    juce::TextButton undoButton { utf8 ("Ångra") }, playButton;
    Knob crazyKnob, swingKnob, bpmKnob;

    // vågform och pads
    WaveView wave;
    PadGrid pads;
    LibraryPanel library;
    juce::TextButton loadButton { "Ladda ljud" }, libButton { "Bibliotek" }, chopButton { "Hacka till pads" };
    juce::ComboBox chopMode;
    juce::Slider sensitivity;

    // sidopanel
    juce::ComboBox filterType;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterTypeAtt;
    Knob kCut, kRes, kDrive, kEnv, kDec, kCrush, kSendA, kSendB, kVol, kPan;
    Knob kPitch, kDecay;
    juce::TextButton revButton { utf8 ("Baklänges") };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> revAtt;

    // sequencer
    std::vector<std::unique_ptr<LaneRow>> rows;

    // effekter
    juce::ComboBox delayTime, stutLen;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> delayTimeAtt, stutLenAtt;
    juce::TextButton freezeButton { "Freeze" }, stutterButton { "STUTTER" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAtt;
    Knob kDFb, kDTone, kDWob, kDRet, kRSize, kRDrive, kRCut, kRDuck, kRRet, kSweep, kMCrush, kClip, kOut;

    std::unique_ptr<juce::FileChooser> chooser;
    AboutOverlay about { "BETONG", juce::String::fromUTF8 ("techno-sampler · v1.0") };
    juce::Rectangle<int> logoArea;
    int seenVersion = -1;
    int shownPad = -1, shownLane = -1;
    juce::Rectangle<int> headerArea, waveArea, padsArea, sideArea, seqArea, fxArea[4];
};

//==============================================================================
class BetongEditor : public juce::AudioProcessorEditor
{
public:
    explicit BetongEditor (BetongProcessor&);
    ~BetongEditor() override;
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (BetongColours::ground); }

private:
    BetongProcessor& proc;
    BetongContent content;
};
