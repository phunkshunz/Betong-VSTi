#include "PluginEditor.h"
#include "Presets.h"

// Cmd på Mac, Ctrl på Windows (JUCE:s isCommandDown följer samma regel)
#if JUCE_MAC
 #define CMD_KEY "Cmd"
#else
 #define CMD_KEY "Ctrl"
#endif

using namespace BetongColours;
using namespace betong;

namespace
{
const juce::StringArray kChopModes { "Anslag", utf8 ("Jämnt 8"), utf8 ("Jämnt 16") };

juce::String upper (const juce::String& t)
{
    return t.toUpperCase().replace (utf8 ("å"), utf8 ("Å")).replace (utf8 ("ä"), utf8 ("Ä")).replace (utf8 ("ö"), utf8 ("Ö"));
}

juce::String padName (BetongProcessor& p, int pad)
{
    auto info = p.getPad (pad);
    return info.sample ? info.sample->name : utf8 ("Tom");
}

void styleIncDec (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::IncDecButtons);
    s.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 26, 20);
    s.setIncDecButtonsMode (juce::Slider::incDecButtonsDraggable_Vertical);
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    s.setColour (juce::TextButton::buttonColourId, raised);
}
} // namespace

//==============================================================================
void DiceButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    auto bg = solid ? sodium : raised;
    if (down) bg = bg.brighter (0.15f); else if (over) bg = bg.brighter (0.07f);
    g.setColour (bg); g.fillRoundedRectangle (r, 4.0f);
    g.setColour (sodium); g.drawRoundedRectangle (r, 4.0f, 1.0f);

    const auto fg = solid ? ground : sodium;
    auto icon = r.removeFromLeft (r.getHeight() * 0.9f).reduced (r.getHeight() * 0.25f);
    g.setColour (fg);
    g.drawRoundedRectangle (icon, 2.5f, 1.4f);
    const float d = icon.getWidth() * 0.16f;
    for (auto pt : { icon.getCentre(), icon.getTopLeft() + juce::Point<float> (icon.getWidth() * 0.28f, icon.getHeight() * 0.28f),
                     icon.getBottomRight() - juce::Point<float> (icon.getWidth() * 0.28f, icon.getHeight() * 0.28f) })
        g.fillEllipse (juce::Rectangle<float> (d, d).withCentre (pt));
    g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    g.drawText (getName(), r.withTrimmedRight (4), juce::Justification::centredLeft, false);
}

//==============================================================================
// Vågform

void WaveView::rebuildPeaks (const Sample& s)
{
    peaksFor = &s;
    const int cols = juce::jmax (1, getWidth());
    peaks.assign ((size_t) cols, { 0.0f, 0.0f });
    const int len = s.data.getNumSamples();
    const float* d = s.data.getReadPointer (0);
    for (int c = 0; c < cols; ++c)
    {
        const int a = (int) ((juce::int64) len * c / cols), b = juce::jmax (a + 1, (int) ((juce::int64) len * (c + 1) / cols));
        float lo = 0, hi = 0;
        for (int i = a; i < b && i < len; i += juce::jmax (1, (b - a) / 64))
        { lo = juce::jmin (lo, d[i]); hi = juce::jmax (hi, d[i]); }
        peaks[(size_t) c] = { lo, hi };
    }
}

float WaveView::xForSample (int s, int len) const { return len > 0 ? (float) getWidth() * (float) s / (float) len : 0.0f; }
int WaveView::sampleForX (float x, int len) const { return juce::jlimit (0, len, (int) ((double) x / getWidth() * len)); }

void WaveView::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xff141516));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 3.0f);

    auto info = proc.getPad (proc.selectedPad);
    if (! info.sample)
    {
        g.setColour (muted); g.setFont (juce::FontOptions (12.0f));
        g.drawText (utf8 ("Dra in ett ljud här, eller välj i biblioteket"), getLocalBounds(), juce::Justification::centred);
        return;
    }
    if (peaksFor != info.sample.get() || (int) peaks.size() != getWidth()) rebuildPeaks (*info.sample);

    const int len = info.sample->data.getNumSamples();
    const float mid = getHeight() * 0.5f, scale = getHeight() * 0.46f;
    const float x0 = xForSample (info.start, len), x1 = xForSample (info.end, len);
    g.setColour (sodium.withAlpha (0.08f));
    g.fillRect (juce::Rectangle<float> (x0, 0, x1 - x0, (float) getHeight()));

    for (int c = 0; c < (int) peaks.size(); ++c)
    {
        const bool inSlice = c >= (int) x0 && c <= (int) x1;
        g.setColour (inSlice ? sodium : juce::Colour (0xff5b5f63));
        const auto [lo, hi] = peaks[(size_t) c];
        g.fillRect ((float) c, mid - hi * scale, 1.0f, juce::jmax (1.0f, (hi - lo) * scale));
    }

    // markeringar för alla pads som använder samma ljud
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    for (int p = 0; p < kPads; ++p)
    {
        auto pi = proc.getPad (p);
        if (pi.sample != info.sample) continue;
        const float x = xForSample (pi.start, len);
        g.setColour (p == proc.selectedPad ? sodium : muted);
        g.fillRect (x, 0.0f, 1.0f, (float) getHeight());
        g.drawText (juce::String (p + 1), juce::Rectangle<float> (x + 2, 1, 20, 11), juce::Justification::left);
    }
    g.setColour (sodium);
    g.fillRect (x1 - 1.0f, 0.0f, 2.0f, (float) getHeight());
}

void WaveView::mouseMove (const juce::MouseEvent& e)
{
    auto info = proc.getPad (proc.selectedPad);
    if (! info.sample) return;
    const int len = info.sample->data.getNumSamples();
    const bool near = std::abs (e.position.x - xForSample (info.start, len)) < 6 || std::abs (e.position.x - xForSample (info.end, len)) < 6;
    setMouseCursor (near ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
}

void WaveView::mouseDown (const juce::MouseEvent& e)
{
    auto info = proc.getPad (proc.selectedPad);
    dragEdge = 0;
    if (! info.sample) return;
    const int len = info.sample->data.getNumSamples();
    const float ds = std::abs (e.position.x - xForSample (info.start, len));
    const float de = std::abs (e.position.x - xForSample (info.end, len));
    if (ds < 8 || de < 8) dragEdge = ds <= de ? 1 : 2;
    else proc.triggerPad (proc.selectedPad); // klick i vågformen = provlyssna
}

void WaveView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragEdge == 0) return;
    auto info = proc.getPad (proc.selectedPad);
    if (! info.sample) return;
    const int len = info.sample->data.getNumSamples();
    const int s = sampleForX (e.position.x, len);
    if (dragEdge == 1) proc.setPadRange (proc.selectedPad, juce::jmin (s, info.end - 64), info.end);
    else proc.setPadRange (proc.selectedPad, info.start, juce::jmax (s, info.start + 64));
    repaint();
}

void WaveView::filesDropped (const juce::StringArray& files, int, int)
{
    if (! files.isEmpty() && proc.loadFileToPad (proc.selectedPad, juce::File (files[0])))
        proc.triggerPad (proc.selectedPad);
    repaint();
}

//==============================================================================
// Pads

juce::Rectangle<int> PadGrid::padBounds (int i) const
{
    const int cols = 8, gap = 6;
    const int w = (getWidth() - gap * (cols - 1)) / cols;
    const int h = (getHeight() - gap) / 2;
    return { (i % cols) * (w + gap), (i / cols) * (h + gap), w, h };
}

int PadGrid::padAt (juce::Point<int> p) const
{
    for (int i = 0; i < kPads; ++i) if (padBounds (i).contains (p)) return i;
    return -1;
}

void PadGrid::tick()
{
    bool dirty = false;
    for (int i = 0; i < kPads; ++i)
    {
        const int h = proc.padHits[(size_t) i].load();
        if (h != seenHits[(size_t) i]) { seenHits[(size_t) i] = h; flash[(size_t) i] = 1.0f; dirty = true; }
        else if (flash[(size_t) i] > 0) { flash[(size_t) i] = juce::jmax (0.0f, flash[(size_t) i] - 0.15f); dirty = true; }
    }
    if (dirty) repaint();
}

void PadGrid::paint (juce::Graphics& g)
{
    const auto pat = proc.getPattern();
    for (int i = 0; i < kPads; ++i)
    {
        auto r = padBounds (i).toFloat();
        int laneIdx = -1;
        for (int l = 0; l < kLanes; ++l) if (pat.lanes[(size_t) l].pad == i) { laneIdx = l; break; }
        const auto col = laneIdx >= 0 ? BetongColours::lane (laneIdx) : juce::Colour (0xff5b5f63);

        g.setColour (raised.interpolatedWith (col, flash[(size_t) i] * 0.45f));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (i == proc.selectedPad ? sodium : line2);
        g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, i == proc.selectedPad ? 2.0f : 1.0f);

        auto inner = r.reduced (7.0f, 6.0f);
        g.setColour (col);
        g.fillRoundedRectangle (inner.removeFromTop (4.0f), 2.0f);
        g.setColour (muted);
        g.setFont (juce::FontOptions (9.0f));
        g.drawText ("PAD " + juce::String (i + 1) + (laneIdx >= 0 ? utf8 (" · SPÅR ") + juce::String (laneIdx + 1) : juce::String()),
                    inner.removeFromBottom (12.0f), juce::Justification::bottomLeft, true);
        g.setColour (text);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawFittedText (padName (proc, i), inner.toNearestInt().withTrimmedTop (2), juce::Justification::topLeft, 3, 0.85f);
    }
}

void PadGrid::mouseDown (const juce::MouseEvent& e)
{
    const int pad = padAt (e.getPosition());
    if (pad < 0) return;
    proc.selectedPad = pad;
    const auto pat = proc.getPattern();
    for (int l = 0; l < kLanes; ++l) if (pat.lanes[(size_t) l].pad == pad) { proc.selectedLane = l; break; }
    if (e.mods.isPopupMenu()) showPadMenu (pad);
    else proc.triggerPad (pad);
    if (selectChanged) selectChanged();
    repaint();
}

void PadGrid::showPadMenu (int pad)
{
    juce::PopupMenu m;
    auto& lib = proc.library();
    const auto names = lib.getNames();
    for (auto& cat : kCategories)
    {
        juce::PopupMenu sub;
        for (int i = 0; i < names.size(); ++i)
            if (BuiltInLibrary::categoryOf (names[i]) == cat.prefix)
                sub.addItem (1000 + i, BuiltInLibrary::displayName (names[i]));
        m.addSubMenu (utf8 (cat.name), sub);
    }
    m.addSeparator();
    m.addItem (1, utf8 ("Ladda fil..."));
    m.addItem (2, utf8 ("Hacka detta ljud till alla pads"));

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(),
                     [this, pad, names] (int r)
                     {
                         if (r >= 1000) { proc.loadBuiltInToPad (pad, names[r - 1000]); proc.triggerPad (pad); }
                         else if (r == 1)
                         {
                             chooser = std::make_unique<juce::FileChooser> (utf8 ("Välj ett ljud"), juce::File(), BetongProcessor::audioWildcard);
                             chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                                   [this, pad] (const juce::FileChooser& fc)
                                                   {
                                                       if (fc.getResult().existsAsFile() && proc.loadFileToPad (pad, fc.getResult()))
                                                           proc.triggerPad (pad);
                                                       if (selectChanged) selectChanged();
                                                   });
                         }
                         else if (r == 2) proc.chopToPads (pad, 0, 0.6f);
                         if (selectChanged) selectChanged();
                     });
}

void PadGrid::filesDropped (const juce::StringArray& files, int x, int y)
{
    int pad = padAt ({ x, y });
    if (pad < 0) pad = proc.selectedPad;
    // flera filer fyller pads i ordning
    for (int i = 0; i < files.size() && pad + i < kPads; ++i)
        proc.loadFileToPad (pad + i, juce::File (files[i]));
    proc.selectedPad = pad;
    proc.triggerPad (pad);
    if (selectChanged) selectChanged();
}

//==============================================================================
// Bibliotek

LibraryPanel::LibraryPanel (BetongProcessor& p, std::function<void()> onLoaded) : proc (p), loaded (std::move (onLoaded))
{
    addFolder.setButtonText (utf8 ("+ Lägg till mapp"));
    removeFolder.setButtonText (utf8 ("Ta bort mapp"));
    closeButton.setButtonText (utf8 ("Stäng"));
    for (auto* b : { &addFolder, &removeFolder, &closeButton }) addAndMakeVisible (*b);
    folderBox.setTextWhenNothingSelected (utf8 ("Inga egna mappar ännu"));
    addAndMakeVisible (folderBox);

    catList.setModel (&catsModel);
    itemList.setModel (this);
    for (auto* l : { &catList, &itemList })
    {
        l->setRowHeight (22);
        l->setColour (juce::ListBox::backgroundColourId, raised);
        addAndMakeVisible (*l);
    }

    closeButton.onClick = [this] { setVisible (false); };
    addFolder.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> (utf8 ("Välj en mapp med samplingar"));
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                              [this] (const juce::FileChooser& fc)
                              {
                                  if (fc.getResult().isDirectory())
                                  {
                                      proc.addSampleFolder (fc.getResult());
                                      refresh();
                                      selectCategory ((int) std::size (kCategories));
                                  }
                              });
    };
    removeFolder.onClick = [this]
    {
        if (folderBox.getSelectedItemIndex() >= 0) { proc.removeSampleFolder (folderBox.getText()); refresh(); selectCategory (category); }
    };
    refresh();
    selectCategory (0);
}

void LibraryPanel::refresh()
{
    const auto folders = proc.getSampleFolders();
    folderBox.clear (juce::dontSendNotification);
    folderBox.addItemList (folders, 1);
    if (! folders.isEmpty()) folderBox.setSelectedItemIndex (folders.size() - 1, juce::dontSendNotification);
    removeFolder.setEnabled (! folders.isEmpty());
    catList.updateContent();
}

void LibraryPanel::selectCategory (int index)
{
    category = index;
    builtInItems.clear(); fileItems.clear();
    if (index < (int) std::size (kCategories)) builtInItems = proc.library().getNamesInCategory (kCategories[index].prefix);
    else fileItems = proc.scanSampleFolders();
    catList.selectRow (index, true, true);
    itemList.updateContent();
    itemList.repaint();
}

int LibraryPanel::Cats::getNumRows() { return (int) std::size (kCategories) + 1; }

void LibraryPanel::Cats::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (selected) { g.setColour (sodium.withAlpha (0.18f)); g.fillRect (0, 0, w, h); g.setColour (sodium); g.fillRect (0, 0, 3, h); }
    g.setColour (selected ? sodium : text);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    const auto name = row < (int) std::size (kCategories) ? utf8 (kCategories[row].name) : utf8 ("Egna mappar");
    g.drawText (name, 10, 0, w - 12, h, juce::Justification::centredLeft, true);
}

void LibraryPanel::Cats::listBoxItemClicked (int row, const juce::MouseEvent&) { owner.selectCategory (row); }

int LibraryPanel::getNumRows() { return category < (int) std::size (kCategories) ? builtInItems.size() : fileItems.size(); }

void LibraryPanel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (selected) { g.setColour (sodium.withAlpha (0.18f)); g.fillRect (0, 0, w, h); }
    juce::String name, where;
    if (category < (int) std::size (kCategories)) { if (row < builtInItems.size()) name = BuiltInLibrary::displayName (builtInItems[row]); where = "BETONG"; }
    else if (row < fileItems.size()) { name = fileItems[row].getFileNameWithoutExtension(); where = fileItems[row].getParentDirectory().getFileName(); }
    g.setColour (selected ? sodium : text);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (name, 10, 0, w - 150, h, juce::Justification::centredLeft, true);
    g.setColour (muted);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (where, w - 140, 0, 132, h, juce::Justification::centredRight, true);
}

void LibraryPanel::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    const int pad = proc.selectedPad;
    if (category < (int) std::size (kCategories)) { if (row < builtInItems.size()) proc.loadBuiltInToPad (pad, builtInItems[row]); }
    else if (row < fileItems.size()) proc.loadFileToPad (pad, fileItems[row]);
    proc.triggerPad (pad);
    if (loaded) loaded();
}

void LibraryPanel::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds(), utf8 ("BIBLIOTEK"), utf8 ("Klicka för att ladda till PAD ") + juce::String (proc.selectedPad + 1));
}

void LibraryPanel::resized()
{
    auto r = getLocalBounds().reduced (12, 8);
    r.removeFromTop (22);
    auto top = r.removeFromTop (30);
    closeButton.setBounds (top.removeFromRight (70));
    top.removeFromRight (6);
    addFolder.setBounds (top.removeFromLeft (130));
    top.removeFromLeft (6);
    removeFolder.setBounds (top.removeFromRight (110));
    top.removeFromRight (6);
    folderBox.setBounds (top);
    r.removeFromTop (8);
    catList.setBounds (r.removeFromLeft (190));
    r.removeFromLeft (8);
    itemList.setBounds (r);
}

//==============================================================================
// Sequencer

int StepGrid::stepAt (int x) const { return juce::jlimit (0, kMaxSteps - 1, x * kMaxSteps / juce::jmax (1, getWidth())); }

void StepGrid::paint (juce::Graphics& g)
{
    const auto pat = proc.getPattern();
    const auto& ln = pat.lanes[(size_t) lane];
    const int cur = proc.laneStep[(size_t) lane].load();
    const bool running = proc.hostPlaying.load() || proc.internalPlay.load();
    const auto col = BetongColours::lane (lane);
    const float cw = (float) getWidth() / kMaxSteps;

    for (int i = 0; i < kMaxSteps; ++i)
    {
        auto r = juce::Rectangle<float> (i * cw, 0, cw, (float) getHeight()).reduced (1.5f, 1.0f);
        const auto& st = ln.steps[(size_t) i];
        if (i >= ln.length)
        {
            g.setColour (line.withAlpha (0.5f));
            g.drawRoundedRectangle (r, 3.0f, 1.0f);
            continue;
        }
        const bool beat = (i % 4) == 0;
        g.setColour (st.on ? col.withAlpha (st.prob < 100 ? 0.45f + st.prob * 0.004f : 1.0f) : (beat ? juce::Colour (0xff2a2d30) : raised));
        g.fillRoundedRectangle (r, 3.0f);
        if (running && i == cur)
        {
            g.setColour (juce::Colours::white.withAlpha (0.55f));
            g.drawRoundedRectangle (r, 3.0f, 1.5f);
        }
        if (st.on)
        {
            g.setColour (ground);
            g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
            juce::String mark = st.ratchet > 1 ? juce::String::charToString (0xd7) + juce::String ((int) st.ratchet)
                              : st.prob < 100 ? juce::String ((int) st.prob)
                              : st.accent ? juce::String::charToString (0x25b2) : juce::String();
            g.drawText (mark, r, juce::Justification::centred, false);
        }
    }
}

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    const int s = stepAt (e.x);
    if (e.mods.isPopupMenu()) { showStepMenu (s); return; }
    const auto mods = e.mods;
    proc.editLane (lane, [&] (Lane& l)
    {
        l.euclid = false;
        if (s >= l.length) l.length = s + 1;
        auto& st = l.steps[(size_t) s];
        if (mods.isShiftDown()) { st.on = true; st.accent = ! st.accent; paintValue = st.on; }
        else if (mods.isAltDown()) { st.on = true; st.prob = st.prob == 100 ? 75 : st.prob == 75 ? 50 : st.prob == 50 ? 25 : 100; }
        else if (mods.isCommandDown()) { st.on = true; st.ratchet = (uint8_t) (st.ratchet >= 4 ? 1 : st.ratchet + 1); }
        else { st.on = ! st.on; paintValue = st.on; if (! st.on) { st.accent = false; st.prob = 100; st.ratchet = 1; } }
    });
    lastPainted = s;
    repaint();
}

void StepGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu() || e.mods.isAnyModifierKeyDown()) return;
    const int s = stepAt (e.x);
    if (s == lastPainted) return;
    lastPainted = s;
    proc.editLane (lane, [&] (Lane& l) { if (s < l.length) { l.euclid = false; l.steps[(size_t) s].on = paintValue; } });
    repaint();
}

void StepGrid::showStepMenu (int s)
{
    juce::PopupMenu m;
    m.addSectionHeader ("STEG " + juce::String (s + 1));
    m.addItem (1, utf8 ("På / av"));
    m.addItem (2, "Accent");
    juce::PopupMenu prob;
    for (int v : { 100, 75, 50, 25, 10 }) prob.addItem (100 + v, juce::String (v) + " %");
    m.addSubMenu ("Sannolikhet", prob);
    juce::PopupMenu rat;
    for (int v = 1; v <= 4; ++v) rat.addItem (10 + v, v == 1 ? utf8 ("Ingen") : juce::String::charToString (0xd7) + juce::String (v));
    m.addSubMenu ("Ratchet", rat);
    m.addSeparator();
    m.addItem (3, utf8 ("Töm spåret"));
    m.addItem (4, utf8 ("Fyll varje steg"));
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [this, s] (int r)
    {
        if (r == 0) return;
        proc.editLane (lane, [&] (Lane& l)
        {
            l.euclid = false;
            auto& st = l.steps[(size_t) s];
            if (r == 1) st.on = ! st.on;
            else if (r == 2) { st.on = true; st.accent = ! st.accent; }
            else if (r == 3) for (auto& x : l.steps) x = Step {};
            else if (r == 4) for (int i = 0; i < l.length; ++i) l.steps[(size_t) i].on = true;
            else if (r > 100) { st.on = true; st.prob = (uint8_t) (r - 100); }
            else if (r > 10 && r <= 14) { st.on = true; st.ratchet = (uint8_t) (r - 10); }
        });
        repaint();
    });
}

LaneRow::LaneRow (BetongProcessor& p, int laneIndex, std::function<void (int)> onSelect)
    : grid (p, laneIndex), proc (p), lane (laneIndex), selectLane (std::move (onSelect))
{
    for (int i = 1; i <= kPads; ++i) padBox.addItem ("Pad " + juce::String (i), i);
    for (int i = 0; i < kNumRates; ++i) rateBox.addItem (kRates[i].name, i + 1);
    padBox.setTooltip (utf8 ("Vilken pad spåret spelar"));
    rateBox.setTooltip (utf8 ("Spårets takt. 5-del och 7-del ger skeva polyrytmer"));

    lenSlider.setRange (1, kMaxSteps, 1);
    hitsSlider.setRange (0, kMaxSteps, 1);
    rotSlider.setRange (0, kMaxSteps - 1, 1);
    for (auto* s : { &lenSlider, &hitsSlider, &rotSlider }) { styleIncDec (*s); addAndMakeVisible (*s); }
    lenSlider.setTooltip (utf8 ("Spårets längd i steg"));
    hitsSlider.setTooltip (utf8 ("Euklid: antal slag"));
    rotSlider.setTooltip (utf8 ("Euklid: rotation"));

    lockButton.setButtonText (utf8 ("Lås"));
    for (auto* b : { &euclidButton, &muteButton, &soloButton, &lockButton })
    {
        b->setClickingTogglesState (true);
        addAndMakeVisible (*b);
    }
    muteButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff6b6f73));
    soloButton.setColour (juce::TextButton::buttonOnColourId, cold);
    euclidButton.setTooltip (utf8 ("Euklidiskt läge: sprider slagen jämnt"));
    lockButton.setTooltip (utf8 ("Lås spåret så att slumpen inte rör det"));
    addAndMakeVisible (padBox);
    addAndMakeVisible (rateBox);
    addAndMakeVisible (grid);

    padBox.onChange = [this] { proc.editLane (lane, [&] (Lane& l) { l.pad = padBox.getSelectedId() - 1; }); repaint(); };
    rateBox.onChange = [this] { proc.editLane (lane, [&] (Lane& l) { l.rate = rateBox.getSelectedId() - 1; }); };
    lenSlider.onValueChange = [this] { proc.editLane (lane, [&] (Lane& l) { l.length = (int) lenSlider.getValue(); }); grid.repaint(); };
    hitsSlider.onValueChange = [this]
    {
        proc.editLane (lane, [&] (Lane& l) { l.hits = (int) hitsSlider.getValue(); });
        grid.repaint();
    };
    rotSlider.onValueChange = [this] { proc.editLane (lane, [&] (Lane& l) { l.rotate = (int) rotSlider.getValue(); }); grid.repaint(); };
    euclidButton.onClick = [this]
    {
        const bool on = euclidButton.getToggleState();
        proc.editLane (lane, [&] (Lane& l)
        {
            l.euclid = on;
            if (on) { int n = 0; for (int i = 0; i < l.length; ++i) n += l.steps[(size_t) i].on ? 1 : 0; l.hits = juce::jmax (1, n); }
        });
        refresh();
    };
    muteButton.onClick = [this] { proc.editLane (lane, [&] (Lane& l) { l.mute = muteButton.getToggleState(); }); };
    soloButton.onClick = [this] { proc.editLane (lane, [&] (Lane& l) { l.solo = soloButton.getToggleState(); }); };
    lockButton.onClick = [this] { proc.editLane (lane, [&] (Lane& l) { l.lock = lockButton.getToggleState(); }); };
    refresh();
}

void LaneRow::refresh()
{
    const auto pat = proc.getPattern();
    const auto& l = pat.lanes[(size_t) lane];
    padBox.setSelectedId (l.pad + 1, juce::dontSendNotification);
    rateBox.setSelectedId (l.rate + 1, juce::dontSendNotification);
    if (! lenSlider.isMouseButtonDown()) lenSlider.setValue (l.length, juce::dontSendNotification);
    if (! hitsSlider.isMouseButtonDown()) hitsSlider.setValue (l.hits, juce::dontSendNotification);
    if (! rotSlider.isMouseButtonDown()) rotSlider.setValue (l.rotate, juce::dontSendNotification);
    euclidButton.setToggleState (l.euclid, juce::dontSendNotification);
    muteButton.setToggleState (l.mute, juce::dontSendNotification);
    soloButton.setToggleState (l.solo, juce::dontSendNotification);
    lockButton.setToggleState (l.lock, juce::dontSendNotification);
    hitsSlider.setEnabled (l.euclid);
    rotSlider.setEnabled (l.euclid);
    repaint();
}

void LaneRow::paint (juce::Graphics& g)
{
    const auto pat = proc.getPattern();
    const auto& l = pat.lanes[(size_t) lane];
    const bool sel = proc.selectedLane == lane;
    if (sel)
    {
        g.setColour (sodium.withAlpha (0.07f));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
    }
    auto r = labelArea;
    g.setColour (BetongColours::lane (lane));
    g.fillRoundedRectangle (r.removeFromLeft (6).withSizeKeepingCentre (6, 16).toFloat(), 2.0f);
    r.removeFromLeft (6);
    g.setColour (sel ? sodium : text);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (juce::String (lane + 1) + " " + upper (padName (proc, l.pad)), r, juce::Justification::centredLeft, true);
}

void LaneRow::resized()
{
    auto r = getLocalBounds().reduced (2, 3);
    labelArea = r.removeFromLeft (118);
    auto put = [&] (juce::Component& c, int w) { c.setBounds (r.removeFromLeft (w)); r.removeFromLeft (5); };
    put (padBox, 70);
    put (lenSlider, 58);
    put (rateBox, 70);
    put (euclidButton, 26);
    put (hitsSlider, 58);
    put (rotSlider, 58);
    auto right = r.removeFromRight (3 * 30 + 4);
    muteButton.setBounds (right.removeFromLeft (28)); right.removeFromLeft (2);
    soloButton.setBounds (right.removeFromLeft (28)); right.removeFromLeft (2);
    lockButton.setBounds (right.removeFromLeft (34));
    r.removeFromRight (6);
    grid.setBounds (r);
}

void LaneRow::mouseDown (const juce::MouseEvent& e)
{
    if (labelArea.contains (e.getPosition()) && selectLane) selectLane (lane);
}

//==============================================================================
// Huvudvyn

BetongContent::BetongContent (BetongProcessor& p)
    : proc (p), wave (p), pads (p, [this] { selectionChanged(); }), library (p, [this] { selectionChanged(); })
{
    setLookAndFeel (&lnf);
    auto& s = proc.apvts;

    // header
    for (int i = 0; i < kNumKits; ++i) kitBox.addItem (utf8 (kKits[i].name), i + 1);
    for (int i = 0; i < kNumPatterns; ++i) patternBox.addItem (utf8 (kPatterns[i].name), i + 1);
    for (int i = 0; i < kNumFx; ++i) fxBox.addItem (utf8 (kFx[i].name), i + 1);
    kitBox.onChange = [this] { if (kitBox.getSelectedId() > 0) { proc.applyKit (kitBox.getSelectedId() - 1); selectionChanged(); } };
    patternBox.onChange = [this] { if (patternBox.getSelectedId() > 0) { proc.applyPattern (patternBox.getSelectedId() - 1); selectionChanged(); } };
    fxBox.onChange = [this] { if (fxBox.getSelectedId() > 0) { proc.applyFx (fxBox.getSelectedId() - 1); selectionChanged(); } };
    for (auto* c : { &kitBox, &patternBox, &fxBox }) addAndMakeVisible (*c);

    diceKit.onClick = [this] { proc.randomKit(); selectionChanged(); };
    diceFx.onClick = [this] { proc.randomFx(); selectionChanged(); };
    dicePattern.onClick = [this] { proc.randomPattern(); selectionChanged(); };
    diceAll.onClick = [this] { proc.randomAll(); selectionChanged(); };
    undoButton.onClick = [this] { proc.undoRandom(); selectionChanged(); };
    for (auto* b : std::initializer_list<juce::Component*> { &diceKit, &diceFx, &dicePattern, &diceAll, &undoButton }) addAndMakeVisible (*b);
    diceKit.setTooltip (utf8 ("Slumpa ljud till alla olåsta pads"));
    diceFx.setTooltip (utf8 ("Slumpa effekter"));
    dicePattern.setTooltip (utf8 ("Slumpa ett nytt skevt mönster"));
    diceAll.setTooltip (utf8 ("Slumpa allt. Galenskap styr hur vilt"));

    playButton.setClickingTogglesState (true);
    playButton.setButtonText (utf8 ("SPELA"));
    playButton.setTooltip (utf8 ("Spela mönstret utan att starta DAW:en"));
    playButton.onClick = [this] { proc.internalPlay.store (playButton.getToggleState()); };
    addAndMakeVisible (playButton);

    crazyKnob.attach (s, "crazy", "GALENSKAP");
    swingKnob.attach (s, "swing", "SWING");
    bpmKnob.attach (s, "bpm", "TEMPO");
    bpmKnob.setAccent (cold);
    for (auto* k : { &crazyKnob, &swingKnob, &bpmKnob }) addAndMakeVisible (*k);

    // vågform, pads, bibliotek
    addAndMakeVisible (wave);
    addAndMakeVisible (pads);
    chopMode.addItemList (kChopModes, 1);
    chopMode.setSelectedId (1, juce::dontSendNotification);
    sensitivity.setSliderStyle (juce::Slider::LinearBar);
    sensitivity.setRange (0.0, 1.0, 0.01);
    sensitivity.setValue (0.6, juce::dontSendNotification);
    sensitivity.setTextValueSuffix (utf8 (" känslighet"));
    sensitivity.setNumDecimalPlacesToDisplay (2);
    sensitivity.setColour (juce::Slider::trackColourId, sodium.withAlpha (0.35f));
    loadButton.onClick = [this] { openFileForSelectedPad(); };
    libButton.onClick = [this] { library.setVisible (! library.isVisible()); library.refresh(); };
    chopButton.onClick = [this]
    {
        const int mode = chopMode.getSelectedId() == 2 ? 8 : chopMode.getSelectedId() == 3 ? 16 : 0;
        proc.chopToPads (proc.selectedPad, mode, (float) sensitivity.getValue());
        proc.selectedPad = 0;
        selectionChanged();
    };
    chopButton.setColour (juce::TextButton::textColourOffId, sodium);
    for (auto* c : std::initializer_list<juce::Component*> { &loadButton, &libButton, &chopButton, &chopMode, &sensitivity }) addAndMakeVisible (*c);
    addChildComponent (library);

    // sidopanel
    filterType.addItemList ({ "LP 24", "LP 12", "HP", "BP", "Notch", "Av" }, 1);
    addAndMakeVisible (filterType);
    for (auto* k : { &kCut, &kRes, &kDrive, &kEnv, &kDec, &kCrush, &kSendA, &kSendB, &kVol, &kPan, &kPitch, &kDecay }) addAndMakeVisible (*k);
    revButton.setClickingTogglesState (true);
    addAndMakeVisible (revButton);
    kEnv.setAccent (cold); kDec.setAccent (cold);
    kSendA.setAccent (cold); kSendB.setAccent (cold);

    // sequencer
    for (int l = 0; l < kLanes; ++l)
    {
        rows.push_back (std::make_unique<LaneRow> (proc, l, [this] (int ln) { selectLane (ln); }));
        addAndMakeVisible (*rows.back());
    }

    // effekter
    delayTime.addItemList ({ "1/8", "1/8 punkt", "3/16", "1/8T", "5/16", "1/4" }, 1);
    stutLen.addItemList ({ "1/4", "1/8", "1/16", "1/32" }, 1);
    delayTimeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, "dTime", delayTime);
    stutLenAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, "stutLen", stutLen);
    freezeButton.setClickingTogglesState (true);
    freezeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, "dFreeze", freezeButton);
    freezeButton.setColour (juce::TextButton::buttonOnColourId, cold);
    stutterButton.setColour (juce::TextButton::textColourOffId, sodium);
    stutterButton.onStateChange = [this]
    {
        const bool down = stutterButton.isDown();
        if (auto* prm = proc.apvts.getParameter ("stutter"))
            if ((prm->getValue() > 0.5f) != down) prm->setValueNotifyingHost (down ? 1.0f : 0.0f);
    };
    for (auto* c : std::initializer_list<juce::Component*> { &delayTime, &stutLen, &freezeButton, &stutterButton }) addAndMakeVisible (*c);
    kDFb.attach (s, "dFb", utf8 ("ÅTERKOPPL.")); kDTone.attach (s, "dTone", "TON"); kDWob.attach (s, "dWob", "WOBBLE"); kDRet.attach (s, "dRet", utf8 ("NIVÅ"));
    kRSize.attach (s, "rSize", "STORLEK"); kRDrive.attach (s, "rDrive", "DRIVE"); kRCut.attach (s, "rCut", "CUTOFF");
    kRDuck.attach (s, "rDuck", "DUCK"); kRRet.attach (s, "rRet", utf8 ("NIVÅ"));
    kSweep.attach (s, "sweep", "SVEP"); kMCrush.attach (s, "mcrush", "GRUS"); kClip.attach (s, "clip", "CLIPPER"); kOut.attach (s, "out", "UT");
    for (auto* k : { &kDFb, &kDTone, &kDWob, &kDRet, &kRSize, &kRDrive, &kRCut, &kRDuck, &kRRet, &kSweep, &kMCrush, &kClip, &kOut }) addAndMakeVisible (*k);
    kDFb.setAccent (cold); kDTone.setAccent (cold); kDWob.setAccent (cold); kDRet.setAccent (cold);

    setSize (W, H);
    selectionChanged();
    addChildComponent (about);
    startTimerHz (30);
}

BetongContent::~BetongContent()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void BetongContent::openFileForSelectedPad()
{
    chooser = std::make_unique<juce::FileChooser> (utf8 ("Välj ett ljud"), juce::File(), BetongProcessor::audioWildcard);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
    {
        if (fc.getResult().existsAsFile() && proc.loadFileToPad (proc.selectedPad, fc.getResult()))
            proc.triggerPad (proc.selectedPad);
        selectionChanged();
    });
}

void BetongContent::selectLane (int lane)
{
    proc.selectedLane = lane;
    proc.selectedPad = proc.getPattern().lanes[(size_t) lane].pad;
    selectionChanged();
}

void BetongContent::selectionChanged()
{
    auto& s = proc.apvts;
    const int l = proc.selectedLane, p = proc.selectedPad;
    if (l != shownLane)
    {
        shownLane = l;
        filterTypeAtt.reset();
        filterTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, BetongProcessor::laneId (l, "type"), filterType);
        kCut.attach (s, BetongProcessor::laneId (l, "cut"), "CUTOFF");
        kRes.attach (s, BetongProcessor::laneId (l, "res"), "RESONANS");
        kDrive.attach (s, BetongProcessor::laneId (l, "drive"), "DRIVE");
        kEnv.attach (s, BetongProcessor::laneId (l, "env"), "ENVELOPE");
        kDec.attach (s, BetongProcessor::laneId (l, "dec"), "DECAY");
        kCrush.attach (s, BetongProcessor::laneId (l, "crush"), "CRUSH");
        kSendA.attach (s, BetongProcessor::laneId (l, "sendA"), utf8 ("SÄND DELAY"));
        kSendB.attach (s, BetongProcessor::laneId (l, "sendB"), utf8 ("SÄND RUMBLE"));
        kVol.attach (s, BetongProcessor::laneId (l, "vol"), "VOLYM");
        kPan.attach (s, BetongProcessor::laneId (l, "pan"), "PANORERING");
    }
    if (p != shownPad)
    {
        shownPad = p;
        kPitch.attach (s, BetongProcessor::padId (p, "pitch"), "PITCH");
        kDecay.attach (s, BetongProcessor::padId (p, "decay"), "DECAY");
        revAtt.reset();
        revAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, BetongProcessor::padId (p, "rev"), revButton);
    }
    for (auto& r : rows) r->refresh();
    refreshPresetBoxes();
    library.repaint();
    wave.refresh();
    pads.repaint();
    repaint();
}

void BetongContent::refreshPresetBoxes()
{
    auto setBox = [] (juce::ComboBox& box, const juce::String& name)
    {
        for (int i = 0; i < box.getNumItems(); ++i)
            if (box.getItemText (i) == name) { box.setSelectedItemIndex (i, juce::dontSendNotification); return; }
        box.setText (name.isEmpty() ? juce::String ("-") : name, juce::dontSendNotification);
    };
    setBox (kitBox, proc.currentKit);
    setBox (patternBox, proc.currentPatternName);
    setBox (fxBox, proc.currentFx);
    seenVersion = proc.stateVersion.load();
}

void BetongContent::timerCallback()
{
    if (proc.stateVersion.load() != seenVersion) selectionChanged();
    pads.tick();
    for (auto& r : rows) r->grid.repaint();

    const bool host = proc.hostPlaying.load();
    playButton.setButtonText (host ? utf8 ("DAW") : (playButton.getToggleState() ? utf8 ("STOPP") : utf8 ("SPELA")));
    playButton.setEnabled (! host);
    undoButton.setEnabled (proc.canUndo());
    auto hr = headerArea;
    repaint (hr.removeFromRight (120));
}

void BetongContent::paint (juce::Graphics& g)
{
    g.fillAll (ground);

    // header
    drawPanel (g, headerArea, {});
    auto h = headerArea.reduced (16, 8);
    auto logo = h.removeFromLeft (150);
    g.setColour (sodium);
    g.setFont (juce::FontOptions (36.0f, juce::Font::bold).withHorizontalScale (0.78f));
    g.drawText ("BETONG", logo.removeFromTop (36), juce::Justification::centredLeft);
    g.setColour (muted);
    g.setFont (juce::FontOptions (9.5f));
    g.drawText (utf8 ("TECHNO SAMPLER · MASKINHALLEN"), logo, juce::Justification::topLeft);

    auto bpmArea = headerArea.reduced (16, 8).removeFromRight (90);
    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText (proc.hostPlaying.load() ? utf8 ("BPM · VÄRD") : utf8 ("BPM"), bpmArea.removeFromTop (14), juce::Justification::centredLeft);
    g.setColour (text);
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold).withHorizontalScale (0.8f));
    g.drawText (juce::String (proc.currentBpm.load(), 1), bpmArea.removeFromTop (30), juce::Justification::centredLeft);

    // vågform + pads
    drawPanel (g, waveArea, utf8 ("SAMPLING · PAD ") + juce::String (proc.selectedPad + 1));
    drawPanel (g, padsArea, "PADS", utf8 ("Klick spelar · högerklick väljer ljud · dra in filer"));

    // sidopanel
    drawPanel (g, sideArea, utf8 ("TECHNO-FILTER · SPÅR ") + juce::String (proc.selectedLane + 1),
               "PAD " + juce::String (proc.selectedPad + 1));
    g.setColour (line);
    g.drawHorizontalLine (sideArea.getY() + 206, (float) sideArea.getX() + 12, (float) sideArea.getRight() - 12);

    // sequencer
    drawPanel (g, seqArea, "SEQUENCER",
               utf8 ("Klick på/av · Shift accent · Alt sannolikhet · " CMD_KEY " ratchet · Högerklick meny"));

    // effekter
    const char* fxTitles[] = { "DUB-DELAY", "RUMBLE", "MASTER", "PERFORMANCE · INTERNT TEMPO" };
    for (int i = 0; i < 4; ++i) drawPanel (g, fxArea[i], utf8 (fxTitles[i]));
}

void BetongContent::resized()
{
    auto r = getLocalBounds().reduced (16);
    headerArea = r.removeFromTop (64);
    r.removeFromTop (12);
    auto mid = r.removeFromTop (296);
    r.removeFromTop (12);
    fxArea[0] = {};
    auto fxRow = r.removeFromBottom (96);
    r.removeFromBottom (12);
    seqArea = r;

    // header
    {
        auto h = headerArea.reduced (16, 10);
        h.removeFromLeft (150);
        h.removeFromRight (96);
        auto knobs = h.removeFromRight (116);
        crazyKnob.setBounds (knobs.removeFromLeft (58).expanded (0, 6));
        swingKnob.setBounds (knobs.removeFromLeft (58).expanded (0, 6));
        h.removeFromRight (8);
        undoButton.setBounds (h.removeFromRight (58).reduced (0, 4));
        h.removeFromRight (6);
        diceAll.setBounds (h.removeFromRight (72).reduced (0, 4)); h.removeFromRight (4);
        dicePattern.setBounds (h.removeFromRight (102).reduced (0, 4)); h.removeFromRight (4);
        diceFx.setBounds (h.removeFromRight (58).reduced (0, 4)); h.removeFromRight (4);
        diceKit.setBounds (h.removeFromRight (64).reduced (0, 4)); h.removeFromRight (10);
        const int w = (h.getWidth() - 12) / 3;
        kitBox.setBounds (h.removeFromLeft (w).reduced (0, 6)); h.removeFromLeft (6);
        patternBox.setBounds (h.removeFromLeft (w).reduced (0, 6)); h.removeFromLeft (6);
        fxBox.setBounds (h.reduced (0, 6));
    }

    // mitten
    sideArea = mid.removeFromRight (452);
    mid.removeFromRight (12);
    waveArea = mid.removeFromTop (128);
    mid.removeFromTop (12);
    padsArea = mid;
    {
        auto w = waveArea.reduced (12, 8);
        auto top = w.removeFromTop (24);
        top.removeFromLeft (260);
        chopButton.setBounds (top.removeFromRight (120)); top.removeFromRight (6);
        sensitivity.setBounds (top.removeFromRight (130).reduced (0, 2)); top.removeFromRight (6);
        chopMode.setBounds (top.removeFromRight (92)); top.removeFromRight (6);
        libButton.setBounds (top.removeFromRight (80)); top.removeFromRight (6);
        loadButton.setBounds (top.removeFromRight (86));
        w.removeFromTop (6);
        wave.setBounds (w);
    }
    pads.setBounds (padsArea.reduced (12, 8).withTrimmedTop (24));
    library.setBounds (waveArea.getUnion (padsArea));

    // sidopanel: spårets filter (2 rader rattar) + pad
    {
        auto s = sideArea.reduced (12, 8);
        s.removeFromTop (24);
        auto typeRow = s.removeFromTop (24);
        filterType.setBounds (typeRow.removeFromLeft (110));
        s.removeFromTop (6);
        auto row1 = s.removeFromTop (70);
        auto row2 = s.removeFromTop (70);
        const int kw = row1.getWidth() / 5;
        for (auto* k : { &kCut, &kRes, &kDrive, &kEnv, &kDec }) k->setBounds (row1.removeFromLeft (kw));
        for (auto* k : { &kCrush, &kSendA, &kSendB, &kVol, &kPan }) k->setBounds (row2.removeFromLeft (kw));
        s.removeFromTop (8);
        auto padRow = s.removeFromTop (70);
        padRow.removeFromLeft (4);
        kPitch.setBounds (padRow.removeFromLeft (kw));
        kDecay.setBounds (padRow.removeFromLeft (kw));
        padRow.removeFromLeft (10);
        revButton.setBounds (padRow.removeFromLeft (110).withSizeKeepingCentre (110, 30));
    }

    // sequencer
    {
        auto s = seqArea.reduced (12, 8);
        s.removeFromTop (24);
        const int rh = s.getHeight() / kLanes;
        for (auto& row : rows) row->setBounds (s.removeFromTop (rh));
    }

    // effekter
    {
        const int gap = 8;
        const int ws[] = { 340, 300, 270 };
        auto f = fxRow;
        for (int i = 0; i < 3; ++i) { fxArea[i] = f.removeFromLeft (ws[i]); f.removeFromLeft (gap); }
        fxArea[3] = f;

        auto layoutKnobs = [] (juce::Rectangle<int> area, std::initializer_list<Knob*> knobs)
        {
            const int kw = area.getWidth() / (int) knobs.size();
            for (auto* k : knobs) k->setBounds (area.removeFromLeft (kw));
        };
        auto d = fxArea[0].reduced (10, 6); d.removeFromTop (18);
        auto dl = d.removeFromLeft (84);
        delayTime.setBounds (dl.removeFromTop (26)); dl.removeFromTop (6);
        freezeButton.setBounds (dl.removeFromTop (26));
        d.removeFromLeft (6);
        layoutKnobs (d, { &kDFb, &kDTone, &kDWob, &kDRet });
        auto rm = fxArea[1].reduced (10, 6); rm.removeFromTop (18);
        layoutKnobs (rm, { &kRSize, &kRDrive, &kRCut, &kRDuck, &kRRet });
        auto m = fxArea[2].reduced (10, 6); m.removeFromTop (18);
        layoutKnobs (m, { &kSweep, &kMCrush, &kClip, &kOut });
        auto pf = fxArea[3].reduced (12, 8); pf.removeFromTop (22);
        bpmKnob.setBounds (pf.removeFromRight (60).expanded (0, 8));
        pf.removeFromRight (6);
        playButton.setBounds (pf.removeFromRight (78).withSizeKeepingCentre (78, 44));
        pf.removeFromRight (8);
        auto col = pf.removeFromRight (64);
        stutLen.setBounds (col.withSizeKeepingCentre (64, 26));
        pf.removeFromRight (6);
        stutterButton.setBounds (pf.withSizeKeepingCentre (pf.getWidth(), 44));
    }

    logoArea = headerArea.reduced (16, 8).removeFromLeft (150);
    about.setBounds (getLocalBounds());
}

void BetongContent::mouseDown (const juce::MouseEvent& e)
{
    if (logoArea.contains (e.getPosition())) { about.setVisible (true); about.toFront (false); }
}

void BetongContent::mouseMove (const juce::MouseEvent& e)
{
    setMouseCursor (logoArea.contains (e.getPosition()) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

//==============================================================================
BetongEditor::BetongEditor (BetongProcessor& p) : AudioProcessorEditor (&p), proc (p), content (p)
{
    addAndMakeVisible (content);
    // passa skärmen: gamla 13-tumsskärmar får en mindre start-storlek
    float scale = 1.0f;
    if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        scale = juce::jlimit (0.6f, 1.0f, (float) (d->userArea.getHeight() - 140) / (float) BetongContent::H);
    setResizable (true, true);
    setResizeLimits (BetongContent::W / 2, BetongContent::H / 2, BetongContent::W * 3 / 2, BetongContent::H * 3 / 2);
    getConstrainer()->setFixedAspectRatio ((double) BetongContent::W / BetongContent::H);
    setSize (juce::roundToInt (BetongContent::W * scale), juce::roundToInt (BetongContent::H * scale));
}

BetongEditor::~BetongEditor() = default;

void BetongEditor::resized()
{
    const float scale = (float) getWidth() / (float) BetongContent::W;
    content.setTransform (juce::AffineTransform::scale (scale));
    content.setBounds (0, 0, BetongContent::W, BetongContent::H);
}
