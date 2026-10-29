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
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    int stepAt (int x) const;
    float wheelAccum = 0.0f;
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
    void showLaneMenu();
    void setCompact (bool c) { compact = c; grid.setVisible (! c); resized(); }
    StepGrid grid;

private:
    bool compact = false;   // SMIDET: två rader utan stegrutnät, i smedjepanelen
    BetongProcessor& proc;
    int lane;
    std::function<void (int)> selectLane;
    juce::ComboBox padBox, rateBox;
    juce::Slider lenSlider, hitsSlider, rotSlider;
    juce::TextButton euclidButton { "E" }, muteButton { "M" }, soloButton { "S" }, lockButton;
    juce::Rectangle<int> labelArea;
};

//==============================================================================
// EUKLID-fliken: euklid-magin för valt spår (rotation, andning, lager, luckor).
class EuclidPanel : public juce::Component
{
public:
    EuclidPanel (BetongProcessor& p, std::function<void()> onChange);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();

private:
    void edit (std::function<void (betong::Lane&)> fn);
    BetongProcessor& proc;
    std::function<void()> changed;
    juce::ComboBox rotEvery, breatheBars, fillOf;
    juce::Slider breathe, accHits, accLen, ratHits, ratLen, ratCount, toneHits, toneLen, toneSemi;
    juce::TextButton clearAll { utf8 ("Nollställ magin") };
    std::array<juce::Rectangle<int>, 7> rows;
    bool updating = false;
};

//==============================================================================
// MORF-reglaget: högerklick väljer vilken scen det glider mot.
class MorphSlider : public BarSlider
{
public:
    MorphSlider() : BarSlider ("MORF") {}
    std::function<void()> onRightClick;
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() && onRightClick) { onRightClick(); return; }
        BarSlider::mouseDown (e);
    }
};

//==============================================================================
// MOTOR-fliken: energikurvan genom låten och rattarna som styr hur beatet lever.
class EnergyView : public juce::Component, public juce::TooltipClient
{
public:
    explicit EnergyView (BetongProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { mapMode = ! mapMode; repaint(); }
    juce::String getTooltip() override { return juce::String::fromUTF8 ("Klicka för att växla mellan energikurvan och utvecklingskartan"); }

private:
    void paintMap (juce::Graphics&, juce::Rectangle<float> plot, double from, double span);
    BetongProcessor& proc;
    bool mapMode = false;
    juce::Image mapImage;
    juce::String mapKey;
};

// MIDI-knappen: dra den till DAW:en så släpps det som spelas som en MIDI-fil. Klick ger menyn.
class MidiDragButton : public juce::TextButton
{
public:
    explicit MidiDragButton (BetongProcessor& p) : juce::TextButton (juce::String::fromUTF8 ("MIDI ⇣")), proc (p) {}
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void showMenu();
    BetongProcessor& proc;
    int choice = 0;   // 0 = hela formen, annars antal takter
    bool dragging = false;
    std::unique_ptr<juce::FileChooser> chooser;
};

class MotorPanel : public juce::Component
{
public:
    MotorPanel (BetongProcessor& p, std::function<void()> onToggle);
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick();

private:
    BetongProcessor& proc;
    EnergyView energy;
    juce::TextButton onButton, seedButton { "NY" }, aiButton { juce::String::fromUTF8 ("AI ▾") }, presetButton { juce::String::fromUTF8 ("LÄGE ▾") };
    void showPresetMenu();
    MidiDragButton midiButton { proc };
    void showAiMenu();
    juce::Label seedLabel;
    juce::ComboBox formBox;
    Knob kMove, kDepth, kChaos, kFilt, kDub, kPump, kFeel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> formAtt;
    juce::Rectangle<int> statusArea;
    juce::String status;
    int shownSeed = -1;
};

// Ring-vyn: spåren som koncentriska ringar. Dra runt = rotera, rulla eller dra upp/ner = antal slag.
class RingView : public juce::Component, public juce::FileDragAndDropTarget, public juce::TooltipClient
{
public:
    RingView (BetongProcessor& p, std::function<void (int)> onSelect, std::function<void()> onChange)
        : proc (p), selectLane (std::move (onSelect)), changed (std::move (onChange)) {}
    std::function<void (int)> onLoadFile;   // ladda egen sampling till spåret
    std::function<void (int)> onLaneMenu;   // högerklick på ett spår
    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void fileDragMove (const juce::StringArray&, int x, int y) override { dropLane = laneOrRowAt ({ (float) x, (float) y }); repaint(); }
    void fileDragExit (const juce::StringArray&) override { dropLane = -1; repaint(); }
    void filesDropped (const juce::StringArray& files, int x, int y) override;
    juce::String getTooltip() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragLane = -1; }
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::Point<float> centre() const;
    float outerRadius() const;
    float ringRadius (int lane) const;
    int laneAt (juce::Point<float> p) const;
    struct LegendRow { juce::Rectangle<int> row, mute, solo, load; bool narrow = false; };
    LegendRow legendRow (int lane) const;
    int legendRowAt (juce::Point<float> p) const;
    int laneOrRowAt (juce::Point<float> p) const { const int l = laneAt (p); return l >= 0 ? l : legendRowAt (p); }
    bool audible (const betong::Pattern& pat, int lane) const;
    float angleOf (juce::Point<float> p) const;
    void changeHits (int lane, int delta);

    BetongProcessor& proc;
    std::function<void (int)> selectLane;
    std::function<void()> changed;
    int dragLane = -1, startRotate = 0, startHits = 0, lastStepDelta = 0;
    float startAngle = 0.0f, startY = 0.0f;
    std::array<betong::Step, betong::kMaxSteps> startSteps {};
    float wheelAccum = 0.0f;
    int dropLane = -1;
};

//==============================================================================
// VÄVEN: FRIKTION i realtid. En rad per spår: strecket är rutnätet, pricken är var slaget landade,
// svansen bakom är de senaste slagen. S = spökslag, D = duckad av ett starkare spår, R = mikro-ratchet.
class WeaveView : public juce::Component, private juce::Timer
{
public:
    explicit WeaveView (BetongProcessor& p) : proc (p) { startTimerHz (30); setInterceptsMouseClicks (false, false); }
    ~WeaveView() override { stopTimer(); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    BetongProcessor& proc;
    static constexpr int kTrail = 8;
    std::array<std::array<float, kTrail>, betong::kLanes> trail {};
    std::array<int, betong::kLanes> trailPos {}, seenHits {}, seenGhost {}, seenDuck {}, seenStut {};
    std::array<float, betong::kLanes> flash {}, fS {}, fD {}, fR {};
    float glue = 0.0f;
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

    static constexpr int W = 1280, H = 944;

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
    Knob kCut, kRes, kDrive, kEnv, kDec, kCrush, kSendA, kSendB, kVol, kPan, kLaneSwing, kLaneDrag;
    Knob kPitch, kDecay, kPadVol, kPadPan;
    juce::TextButton revButton { utf8 ("Baklänges") };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> revAtt;

    // sequencer
    std::vector<std::unique_ptr<LaneRow>> rows;
    std::array<juce::TextButton, betong::kScenes> sceneButtons;
    juce::TextButton copyScene { "Kopiera till..." }, fillButton { "FILL" };
    BarSlider svajSlider { "SVAJ" }, glitchSlider { "GLITCH" }, densSlider { "DENSITET" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> svajAtt, glitchAtt, densAtt;
    juce::TextButton ringButton { "RING" }, tabFilter { "FILTER" }, tabEuclid { "EUKLID" }, tabMotor { "MOTOR" };
    juce::TextButton motorButton { "MOTOR" }, breedButton { "AVLA" };
    MorphSlider morphSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAtt;
    void showBreedMenu();
    void showMorphMenu();
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> motorAtt;
    std::unique_ptr<EuclidPanel> euclidPanel;
    std::unique_ptr<MotorPanel> motorPanel;
    std::unique_ptr<RingView> ringView;
    juce::TooltipWindow tooltips { this, 700 };
    int sidePage = 0;   // 0 filter, 1 euklid, 2 motor
public:
    enum SidePage { PageFilter = 0, PageEuclid, PageMotor };
    void setSidePage (int page);
    void setRingMode (bool on);
private:
    // SMIDET
    juce::Rectangle<int> forgeArea;
    Knob kHammer, kGlow;
    juce::TextButton soundButton;
    void showLaneSoundMenu (juce::Component* target = nullptr);
    void loadFileForLane (int lane);
    void showLaneMenu (int lane);
    juce::TextButton loadOwnButton;
    void layoutSmidet();
    bool blinkOn = false;
    int blinkCount = 0;

    // effekter
    juce::ComboBox delayTime, stutLen;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> delayTimeAtt, stutLenAtt;
    juce::TextButton freezeButton { "Freeze" }, stutterButton { "STUTTER" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAtt;
    Knob kDFb, kDTone, kDWob, kDRet, kRSize, kRDrive, kRCut, kRDuck, kRRet, kSweep, kMCrush, kClip, kOut;

    std::unique_ptr<juce::FileChooser> chooser;
    // FRIKTION
    juce::Rectangle<int> fricArea;
    Knob kPocket, kTroghet, kHuman, kInterlock, kMRatchet, kVDecay, kGlue;
    DiceButton pocketDice { "DICE // POCKET", true };
    juce::TextButton polyButton { juce::String::fromUTF8 ("POLY 16·15·13·11") };
    std::unique_ptr<WeaveView> weave;

    AboutOverlay about { BETONG_PRODUCT_NAME, juce::String::fromUTF8 (BETONG_ABOUT_LINE " · v1.1") };
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
