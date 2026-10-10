#include "PluginEditor.h"

using namespace BetongColours;

// JUCE vill att text med å, ä, ö och · skickas som UTF-8.
static juce::String utf8 (const char* s) { return juce::String::fromUTF8 (s); }

//==============================================================================
void PadButton::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (raised.interpolatedWith (sodium, flash * 0.6f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (pressed ? sodium : line);
    g.drawRoundedRectangle (r, 6.0f, 1.5f);

    g.setColour (sodium);
    g.fillRoundedRectangle (r.reduced (10.0f).removeFromTop (5.0f), 2.0f);

    g.setColour (text);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText ("PAD 1", r.reduced (10.0f), juce::Justification::bottomLeft);
    g.setColour (muted);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (utf8 ("Tryck för att spela  ·  C3"), r.reduced (10.0f).withTrimmedBottom (20.0f),
                juce::Justification::bottomLeft);
}

void PadButton::mouseDown (const juce::MouseEvent&)
{
    pressed = true;
    processor.keyboardState.noteOn (1, 60, 1.0f);
    repaint();
}

void PadButton::mouseUp (const juce::MouseEvent&)
{
    pressed = false;
    processor.keyboardState.noteOff (1, 60, 0.0f);
    repaint();
}

//==============================================================================
BetongEditor::BetongEditor (BetongProcessor& p)
    : AudioProcessorEditor (&p), processor (p), pad (p)
{
    for (auto* b : { &loadButton, &kickButton })
    {
        b->setColour (juce::TextButton::buttonColourId, raised);
        b->setColour (juce::TextButton::textColourOffId, sodium);
        b->setColour (juce::ComboBox::outlineColourId, sodium);
        addAndMakeVisible (*b);
    }

    loadButton.onClick = [this] { openFileChooser(); };
    kickButton.onClick = [this]
    {
        processor.loadBuiltInKick();
        statusText = {};
        repaint();
    };

    addFolderButton.setButtonText (utf8 ("+ Lägg till mapp"));
    removeFolderButton.setButtonText (utf8 ("Ta bort mapp"));
    for (auto* b : { &addFolderButton, &removeFolderButton })
    {
        b->setColour (juce::TextButton::buttonColourId, raised);
        b->setColour (juce::TextButton::textColourOffId, sodium);
        addAndMakeVisible (*b);
    }
    addFolderButton.onClick = [this] { openFolderChooser(); };
    removeFolderButton.onClick = [this]
    {
        if (folderBox.getSelectedItemIndex() >= 0)
        {
            processor.removeSampleFolder (folderBox.getText());
            refreshLibrary();
        }
    };

    folderBox.setTextWhenNothingSelected (utf8 ("Inga egna mappar ännu"));
    folderBox.setColour (juce::ComboBox::backgroundColourId, raised);
    folderBox.setColour (juce::ComboBox::textColourId, text);
    folderBox.setColour (juce::ComboBox::outlineColourId, line);
    addAndMakeVisible (folderBox);

    libraryList.setModel (this);
    libraryList.setRowHeight (24);
    libraryList.setColour (juce::ListBox::backgroundColourId, panel);
    libraryList.setColour (juce::ListBox::outlineColourId, line);
    libraryList.setOutlineThickness (1);
    addAndMakeVisible (libraryList);

    addAndMakeVisible (pad);
    refreshLibrary();
    setSize (900, 540);
    startTimerHz (30);
}

void BetongEditor::paint (juce::Graphics& g)
{
    g.fillAll (ground);

    // Toppen
    auto area = getLocalBounds().reduced (16);
    auto header = area.removeFromTop (64);
    g.setColour (panel);
    g.fillRoundedRectangle (header.toFloat(), 6.0f);
    g.setColour (line);
    g.drawRoundedRectangle (header.toFloat(), 6.0f, 1.0f);

    auto h = header.reduced (16, 8);
    g.setColour (sodium);
    g.setFont (juce::FontOptions (34.0f, juce::Font::bold));
    g.drawText ("BETONG", h.removeFromLeft (180), juce::Justification::centredLeft);
    g.setColour (muted);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (utf8 ("TECHNO SAMPLER  ·  v0.1  ·  STEG 1"), h, juce::Justification::centredLeft);

    // Samplingspanelen
    area.removeFromTop (12);
    auto samplePanel = area.removeFromTop (110);
    g.setColour (dragHover ? raised : panel);
    g.fillRoundedRectangle (samplePanel.toFloat(), 6.0f);
    g.setColour (dragHover ? sodium : line);
    g.drawRoundedRectangle (samplePanel.toFloat(), 6.0f, dragHover ? 2.0f : 1.0f);

    auto sp = samplePanel.reduced (16, 12);
    g.setColour (muted);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("SAMPLING", sp.removeFromTop (16), juce::Justification::centredLeft);
    g.setColour (text);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText (processor.getSampleName(), sp.removeFromTop (30), juce::Justification::centredLeft, true);
    g.setColour (statusText.isNotEmpty() ? sodium : muted);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (statusText.isNotEmpty() ? statusText
                                        : juce::String ("Dra in en WAV, AIFF eller MP3 hit, eller tryck Ladda sampling"),
                sp.removeFromTop (20), juce::Justification::centredLeft, true);
}

void BetongEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    area.removeFromTop (64 + 12);

    auto samplePanel = area.removeFromTop (110);
    auto buttons = samplePanel.reduced (16, 12).removeFromRight (170);
    loadButton.setBounds (buttons.removeFromTop (40));
    buttons.removeFromTop (6);
    kickButton.setBounds (buttons.removeFromTop (34));

    area.removeFromTop (12);
    pad.setBounds (area.removeFromLeft (220));

    area.removeFromLeft (12);
    auto top = area.removeFromTop (34);
    addFolderButton.setBounds (top.removeFromLeft (150));
    top.removeFromLeft (8);
    removeFolderButton.setBounds (top.removeFromRight (120));
    top.removeFromRight (8);
    folderBox.setBounds (top);

    area.removeFromTop (8);
    libraryList.setBounds (area);
}

void BetongEditor::openFolderChooser()
{
    chooser = std::make_unique<juce::FileChooser> (utf8 ("Välj en mapp med samplingar"));
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto folder = fc.getResult();
                              if (folder.isDirectory())
                              {
                                  processor.addSampleFolder (folder);
                                  refreshLibrary();
                              }
                          });
}

void BetongEditor::refreshLibrary()
{
    const auto folders = processor.getSampleFolders();
    folderBox.clear (juce::dontSendNotification);
    folderBox.addItemList (folders, 1);
    if (! folders.isEmpty())
        folderBox.setSelectedItemIndex (folders.size() - 1, juce::dontSendNotification);
    removeFolderButton.setEnabled (! folders.isEmpty());

    libraryFiles = processor.scanSampleFolders();
    libraryList.updateContent();
    libraryList.repaint();
}

void BetongEditor::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    if (! juce::isPositiveAndBelow (row, libraryFiles.size()))
        return;

    if (selected)
    {
        g.setColour (raised);
        g.fillRect (0, 0, width, height);
        g.setColour (sodium);
        g.fillRect (0, 0, 3, height);
    }

    const auto& f = libraryFiles.getReference (row);
    g.setColour (selected ? sodium : text);
    g.setFont (juce::FontOptions (13.0f));
    g.drawText (f.getFileNameWithoutExtension(), 12, 0, width - 160, height, juce::Justification::centredLeft, true);
    g.setColour (muted);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (f.getParentDirectory().getFileName(), width - 150, 0, 140, height, juce::Justification::centredRight, true);
}

void BetongEditor::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    loadRow (row);
}

void BetongEditor::loadRow (int row)
{
    if (juce::isPositiveAndBelow (row, libraryFiles.size()))
        loadFile (libraryFiles[row]);
}

void BetongEditor::timerCallback()
{
    const int notes = processor.lastNoteCounter.load();
    if (notes != seenNotes)
    {
        seenNotes = notes;
        pad.flash = 1.0f;
    }
    else if (pad.flash > 0.0f)
    {
        pad.flash = juce::jmax (0.0f, pad.flash - 0.12f);
    }
    pad.repaint();
}

void BetongEditor::openFileChooser()
{
    chooser = std::make_unique<juce::FileChooser> (utf8 ("Välj en sampling"), juce::File(),
                                                   "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();
                              if (file.existsAsFile())
                                  loadFile (file);
                          });
}

void BetongEditor::loadFile (const juce::File& file)
{
    statusText = processor.loadSampleFile (file) ? juce::String()
                                                 : utf8 ("Kunde inte läsa ") + file.getFileName();
    repaint();
}

bool BetongEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;mp3;flac;ogg"))
            return true;
    return false;
}

void BetongEditor::filesDropped (const juce::StringArray& files, int, int)
{
    dragHover = false;
    if (! files.isEmpty())
        loadFile (juce::File (files[0]));
}
