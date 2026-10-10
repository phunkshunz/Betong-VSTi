#pragma once

#include "PluginProcessor.h"

namespace BetongColours
{
    const juce::Colour ground   { 0xff121314 };
    const juce::Colour panel    { 0xff1a1c1e };
    const juce::Colour raised   { 0xff232527 };
    const juce::Colour line     { 0xff2c2f32 };
    const juce::Colour text     { 0xffd9d6cf };
    const juce::Colour muted    { 0xff8b8e91 };
    const juce::Colour sodium   { 0xffff8a1f };
    const juce::Colour cold     { 0xff4fd1e0 };
}

// En pad som spelar ljudet när man trycker och blinkar vid varje ton.
class PadButton : public juce::Component
{
public:
    explicit PadButton (BetongProcessor& p) : processor (p) {}

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    float flash = 0.0f;

private:
    BetongProcessor& processor;
    bool pressed = false;
};

class BetongEditor : public juce::AudioProcessorEditor,
                     public juce::FileDragAndDropTarget,
                     private juce::ListBoxModel,
                     private juce::Timer
{
public:
    explicit BetongEditor (BetongProcessor&);
    ~BetongEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragHover = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragHover = false; repaint(); }

private:
    void timerCallback() override;
    void openFileChooser();
    void openFolderChooser();
    void refreshLibrary();
    void loadFile (const juce::File& file);

    // Biblioteket (listan med ljud från egna mappar)
    int getNumRows() override { return libraryFiles.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    void returnKeyPressed (int row) override { loadRow (row); }
    void loadRow (int row);

    BetongProcessor& processor;

    juce::TextButton loadButton { "Ladda sampling" };
    juce::TextButton kickButton { "Inbyggd kick" };
    juce::TextButton addFolderButton;
    juce::TextButton removeFolderButton;
    juce::ComboBox folderBox;
    juce::ListBox libraryList;
    juce::Array<juce::File> libraryFiles;
    PadButton pad;

    std::unique_ptr<juce::FileChooser> chooser;
    juce::String statusText;
    bool dragHover = false;
    int seenNotes = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BetongEditor)
};
