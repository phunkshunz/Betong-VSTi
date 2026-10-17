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
    if (category < (int) std::size (kCategories)) { if (row < builtInItems.size()) name = BuiltInLibrary::displayName (builtInItems[row]); where = BETONG_PRODUCT_NAME; }
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
        const float alpha = st.ghost ? 0.38f : (st.prob < 100 ? 0.45f + st.prob * 0.004f : 1.0f);
        g.setColour (st.on ? col.withAlpha (alpha) : (beat ? juce::Colour (0xff2a2d30) : raised));
        g.fillRoundedRectangle (r, 3.0f);
        if (st.on && st.accent)
        {
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.fillRect (r.withHeight (2.5f).reduced (2.0f, 0.0f));
        }
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
                              : st.semi != 0 ? (st.semi > 0 ? "+" : "") + juce::String ((int) st.semi)
                              : juce::String();
            g.setColour (st.ghost ? text : ground);
            g.drawText (mark, r, juce::Justification::centred, false);
            if (st.semi != 0 && (st.ratchet > 1 || st.prob < 100))
            {
                g.setColour (sodium);
                g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre ({ r.getRight() - 4.0f, r.getBottom() - 4.0f }));
            }
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
        if (mods.isShiftDown())
        {
            // normal -> stark -> svag -> normal
            if (! st.on) { st.on = true; st.accent = true; st.ghost = false; }
            else if (st.accent) { st.accent = false; st.ghost = true; }
            else if (st.ghost) st.ghost = false;
            else st.accent = true;
            paintValue = true;
        }
        else if (mods.isAltDown()) { st.on = true; st.prob = st.prob == 100 ? 75 : st.prob == 75 ? 50 : st.prob == 50 ? 25 : 100; }
        else if (mods.isCommandDown()) { st.on = true; st.ratchet = (uint8_t) (st.ratchet >= 4 ? 1 : st.ratchet + 1); }
        else { st.on = ! st.on; paintValue = st.on; if (! st.on) { st.accent = false; st.ghost = false; st.prob = 100; st.ratchet = 1; st.semi = 0; } }
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

void StepGrid::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    // rulla över ett tänt steg = ändra tonen en halvton i taget
    wheelAccum += w.deltaY * (w.isReversed ? -1.0f : 1.0f);
    const int dir = wheelAccum > 0.12f ? 1 : wheelAccum < -0.12f ? -1 : 0;
    if (dir == 0) return;
    wheelAccum = 0.0f;
    const int s = stepAt (e.x);
    bool changed = false;
    proc.editLane (lane, [&] (Lane& l)
    {
        auto& st = l.steps[(size_t) s];
        if (s < l.length && st.on) { st.semi = (int8_t) juce::jlimit (-24, 24, st.semi + dir); changed = true; }
    });
    if (changed)
    {
        proc.triggerPadPitched (proc.getPattern().lanes[(size_t) lane].pad, proc.getPattern().lanes[(size_t) lane].steps[(size_t) s].semi);
        repaint();
    }
}

void StepGrid::showStepMenu (int s)
{
    juce::PopupMenu m;
    m.addSectionHeader ("STEG " + juce::String (s + 1));
    m.addItem (1, utf8 ("På / av"));
    juce::PopupMenu lvl;
    lvl.addItem (21, "Stark");
    lvl.addItem (22, "Normal");
    lvl.addItem (23, "Svag (ghost)");
    m.addSubMenu ("Styrka", lvl);
    juce::PopupMenu tone;
    for (int v = 12; v >= -12; --v) tone.addItem (200 + v + 24, (v > 0 ? "+" : "") + juce::String (v) + utf8 (v == 0 ? " (grundton)" : ""));
    m.addSubMenu ("Ton", tone);
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
            else if (r == 21) { st.on = true; st.accent = true; st.ghost = false; }
            else if (r == 22) { st.on = true; st.accent = false; st.ghost = false; }
            else if (r == 23) { st.on = true; st.accent = false; st.ghost = true; }
            else if (r >= 176) { st.on = true; st.semi = (int8_t) (r - 200 - 24); }
            else if (r == 3) for (auto& x : l.steps) x = Step {};
            else if (r == 4) for (int i = 0; i < l.length; ++i) l.steps[(size_t) i].on = true;
            else if (r > 100 && r <= 200) { st.on = true; st.prob = (uint8_t) (r - 100); }
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
    if (compact)
    {
        auto r = getLocalBounds().reduced (2, 2);
        auto top = r.removeFromTop (r.getHeight() / 2).reduced (0, 2);
        auto bot = r.reduced (0, 2);
        labelArea = top.removeFromLeft (150);
        auto put = [] (juce::Rectangle<int>& row, juce::Component& c, int w) { c.setBounds (row.removeFromLeft (w)); row.removeFromLeft (6); };
        put (top, padBox, 80); put (top, rateBox, 80); put (top, lenSlider, 70);
        put (bot, euclidButton, 30); put (bot, hitsSlider, 70); put (bot, rotSlider, 70);
        auto right = bot.removeFromRight (3 * 34 + 4);
        muteButton.setBounds (right.removeFromLeft (32)); right.removeFromLeft (2);
        soloButton.setBounds (right.removeFromLeft (32)); right.removeFromLeft (2);
        lockButton.setBounds (right.removeFromLeft (38));
        return;
    }
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
// Euklid-fliken

EuclidPanel::EuclidPanel (BetongProcessor& p, std::function<void()> onChange) : proc (p), changed (std::move (onChange))
{
    rotEvery.addItemList ({ "Av", "Varje takt", "Var 2:a takt", "Var 3:e takt", "Var 4:e takt", "Var 5:e takt", "Var 8:e takt", "Var 16:e takt" }, 1);
    breatheBars.addItemList ({ "4 takter", "8 takter", "16 takter", "32 takter" }, 1);
    fillOf.addItem ("Av", 1);
    for (int i = 0; i < kLanes; ++i) fillOf.addItem (utf8 ("Spår ") + juce::String (i + 1), i + 2);
    for (auto* c : { &rotEvery, &breatheBars, &fillOf }) addAndMakeVisible (*c);

    auto setup = [this] (juce::Slider& sl, int lo, int hi, const juce::String& tip)
    {
        sl.setRange (lo, hi, 1);
        styleIncDec (sl);
        sl.setTooltip (tip);
        addAndMakeVisible (sl);
    };
    setup (breathe, 0, 16, utf8 ("Hur många slag antalet sveper upp och ner (euklidiska spår), eller hur mycket tätheten andas"));
    setup (accHits, 0, 32, utf8 ("Accentlager: antal starka slag"));
    setup (accLen, 1, 32, utf8 ("Accentlager: längd i steg. Annan längd än spåret = accenterna vandrar"));
    setup (ratHits, 0, 32, utf8 ("Ratchetlager: antal skurar"));
    setup (ratLen, 1, 32, utf8 ("Ratchetlager: längd i steg"));
    setup (ratCount, 2, 4, utf8 ("Ratchetlager: slag per skur"));
    setup (toneHits, 0, 32, utf8 ("Tonlager: antal tonhopp"));
    setup (toneLen, 1, 32, utf8 ("Tonlager: längd i steg"));
    setup (toneSemi, -24, 24, utf8 ("Tonlager: tonhoppet i halvtoner"));
    clearAll.setTooltip (utf8 ("Stäng av all euklid-magi på spåret"));
    addAndMakeVisible (clearAll);

    rotEvery.onChange = [this] { static const int v[] = { 0, 1, 2, 3, 4, 5, 8, 16 }; const int i = rotEvery.getSelectedItemIndex(); if (i >= 0) edit ([&] (Lane& l) { l.rotEvery = v[i]; }); };
    breatheBars.onChange = [this] { static const int v[] = { 4, 8, 16, 32 }; const int i = breatheBars.getSelectedItemIndex(); if (i >= 0) edit ([&] (Lane& l) { l.breatheBars = v[i]; }); };
    fillOf.onChange = [this] { const int id = fillOf.getSelectedId(); if (id > 0) edit ([&] (Lane& l) { l.fillOf = id - 2; }); };
    breathe.onValueChange = [this] { edit ([&] (Lane& l) { l.breathe = (int) breathe.getValue(); }); };
    accHits.onValueChange = [this] { edit ([&] (Lane& l) { l.accHits = (int) accHits.getValue(); }); };
    accLen.onValueChange = [this] { edit ([&] (Lane& l) { l.accLen = (int) accLen.getValue(); }); };
    ratHits.onValueChange = [this] { edit ([&] (Lane& l) { l.ratHits = (int) ratHits.getValue(); }); };
    ratLen.onValueChange = [this] { edit ([&] (Lane& l) { l.ratLen = (int) ratLen.getValue(); }); };
    ratCount.onValueChange = [this] { edit ([&] (Lane& l) { l.ratCount = (int) ratCount.getValue(); }); };
    toneHits.onValueChange = [this] { edit ([&] (Lane& l) { l.toneHits = (int) toneHits.getValue(); }); };
    toneLen.onValueChange = [this] { edit ([&] (Lane& l) { l.toneLen = (int) toneLen.getValue(); }); };
    toneSemi.onValueChange = [this] { edit ([&] (Lane& l) { l.toneSemi = (int) toneSemi.getValue(); }); };
    clearAll.onClick = [this]
    {
        edit ([] (Lane& l) { l.rotEvery = 0; l.breathe = 0; l.accHits = 0; l.ratHits = 0; l.toneHits = 0; l.fillOf = -1; });
        refresh();
    };
}

void EuclidPanel::edit (std::function<void (Lane&)> fn)
{
    if (updating) return;
    proc.editLane (proc.selectedLane, [&] (Lane& l) { fn (l); });
    if (changed) changed();
}

void EuclidPanel::refresh()
{
    const auto pat = proc.getPattern();
    const auto& l = pat.lanes[(size_t) proc.selectedLane];
    const juce::ScopedValueSetter<bool> svs (updating, true);
    static const int rv[] = { 0, 1, 2, 3, 4, 5, 8, 16 };
    rotEvery.setText (utf8 ("Var ") + juce::String (l.rotEvery) + utf8 (":e takt"), juce::dontSendNotification);
    for (int i = 0; i < 8; ++i) if (rv[i] == l.rotEvery) rotEvery.setSelectedItemIndex (i, juce::dontSendNotification);
    static const int bv[] = { 4, 8, 16, 32 };
    int bi = 2; for (int i = 0; i < 4; ++i) if (bv[i] == l.breatheBars) bi = i;
    breatheBars.setSelectedItemIndex (bi, juce::dontSendNotification);
    fillOf.setSelectedId (l.fillOf + 2, juce::dontSendNotification);
    breathe.setValue (l.breathe, juce::dontSendNotification);
    accHits.setValue (l.accHits, juce::dontSendNotification); accLen.setValue (l.accLen, juce::dontSendNotification);
    ratHits.setValue (l.ratHits, juce::dontSendNotification); ratLen.setValue (l.ratLen, juce::dontSendNotification);
    ratCount.setValue (l.ratCount, juce::dontSendNotification);
    toneHits.setValue (l.toneHits, juce::dontSendNotification); toneLen.setValue (l.toneLen, juce::dontSendNotification);
    toneSemi.setValue (l.toneSemi, juce::dontSendNotification);
    repaint();
}

void EuclidPanel::paint (juce::Graphics& g)
{
    static const char* labels[] = { "ROTERA SJÄLV", "ANDAS (slag · tid)", "ACCENTLAGER (slag/längd)", "RATCHETLAGER (slag/längd/×)",
                                    "TONLAGER (slag/längd/ton)", "FYLL LUCKORNA I", "" };
    for (size_t i = 0; i < rows.size(); ++i)
    {
        if (labels[i][0] == 0) continue;
        g.setColour (muted);
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText (utf8 (labels[i]), rows[i].withWidth (170), juce::Justification::centredLeft, true);
    }
}

void EuclidPanel::resized()
{
    auto r = getLocalBounds();
    const int rh = r.getHeight() / (int) rows.size();
    for (auto& row : rows) row = r.removeFromTop (rh);
    auto put = [] (juce::Rectangle<int> row, std::initializer_list<juce::Component*> cs)
    {
        row.removeFromLeft (172);
        const int w = row.getWidth() / (int) cs.size();
        for (auto* c : cs) c->setBounds (row.removeFromLeft (w).reduced (2, 2));
    };
    put (rows[0], { &rotEvery });
    put (rows[1], { &breathe, &breatheBars });
    put (rows[2], { &accHits, &accLen });
    put (rows[3], { &ratHits, &ratLen, &ratCount });
    put (rows[4], { &toneHits, &toneLen, &toneSemi });
    put (rows[5], { &fillOf });
    clearAll.setBounds (rows[6].withSizeKeepingCentre (150, rows[6].getHeight() - 4).withX (rows[6].getRight() - 150));
}

//==============================================================================
// Ring-vyn

juce::Point<float> RingView::centre() const
{
    const float R = outerRadius();
    return { R + 14.0f, (float) getHeight() * 0.5f };
}
float RingView::outerRadius() const { return (float) getHeight() * 0.5f - 4.0f; }
float RingView::ringRadius (int lane) const
{
    // spår 1 (kicken) ytterst, spår 8 innerst
    const float R = outerRadius(), r0 = R * 0.2f;
    return R - (R - r0) * ((float) lane + 0.5f) / (float) kLanes;
}
int RingView::laneAt (juce::Point<float> p) const
{
    const float d = p.getDistanceFrom (centre());
    int best = -1; float bestD = 1.0e9f;
    for (int l = 0; l < kLanes; ++l)
    {
        const float dd = std::abs (d - ringRadius (l));
        if (dd < bestD) { bestD = dd; best = l; }
    }
    const float band = (outerRadius() * 0.8f) / (float) kLanes;
    return bestD <= band * 0.6f ? best : -1;
}
float RingView::angleOf (juce::Point<float> p) const
{
    const auto c = centre();
    return std::atan2 (p.y - c.y, p.x - c.x);
}

void RingView::paint (juce::Graphics& g)
{
    const auto pat = proc.getPattern();
    const auto c = centre();
    const float R = outerRadius();
    const float band = (R * 0.8f) / (float) kLanes;
    const bool running = proc.hostPlaying.load() || proc.internalPlay.load();

    g.setColour (juce::Colour (0xff141516));
    g.fillEllipse (juce::Rectangle<float> (R * 2.0f + 8.0f, R * 2.0f + 8.0f).withCentre (c));

    for (int l = 0; l < kLanes; ++l)
    {
        const auto& ln = pat.lanes[(size_t) l];
        const float r = ringRadius (l);
        const auto col = BetongColours::lane (l);
        const bool sel = proc.selectedLane == l;
        g.setColour (sel ? col.withAlpha (0.35f) : line);
        g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c), sel ? 2.0f : 1.0f);

        const int len = juce::jlimit (1, kMaxSteps, ln.length);
        const float dot = juce::jmin (band * 0.42f, juce::MathConstants<float>::twoPi * r / (float) len * 0.36f);
        const int cur = proc.laneStep[(size_t) l].load();
        for (int i = 0; i < len; ++i)
        {
            const float a = -juce::MathConstants<float>::halfPi + juce::MathConstants<float>::twoPi * (float) i / (float) len;
            const juce::Point<float> p (c.x + r * std::cos (a), c.y + r * std::sin (a));
            const auto& st = ln.steps[(size_t) i];
            if (st.on)
            {
                const float s = dot * (st.accent ? 2.2f : st.ghost ? 1.3f : 1.8f);
                g.setColour (col.withAlpha (st.ghost ? 0.5f : 1.0f));
                g.fillEllipse (juce::Rectangle<float> (s, s).withCentre (p));
                if (st.accent) { g.setColour (juce::Colours::white.withAlpha (0.8f)); g.drawEllipse (juce::Rectangle<float> (s, s).withCentre (p), 1.2f); }
            }
            else
            {
                g.setColour (juce::Colour (0xff3a3e42));
                g.fillEllipse (juce::Rectangle<float> (dot * 0.7f, dot * 0.7f).withCentre (p));
            }
            if (running && i == cur)
            {
                g.setColour (juce::Colours::white.withAlpha (0.9f));
                g.drawEllipse (juce::Rectangle<float> (dot * 2.8f, dot * 2.8f).withCentre (p), 1.5f);
            }
        }
    }

    // förklaring till höger: ett spår per rad med dess euklid-magi
    auto info = getLocalBounds().withTrimmedLeft ((int) (R * 2.0f + 40.0f)).reduced (0, 2);
    const int rh = info.getHeight() / kLanes;
    for (int l = 0; l < kLanes; ++l)
    {
        auto row = info.removeFromTop (rh);
        const auto& ln = pat.lanes[(size_t) l];
        int on = 0; for (int i = 0; i < ln.length; ++i) on += ln.steps[(size_t) i].on ? 1 : 0;
        g.setColour (BetongColours::lane (l));
        g.fillRoundedRectangle (row.removeFromLeft (6).withSizeKeepingCentre (6, 14).toFloat(), 2.0f);
        row.removeFromLeft (8);
        auto info0 = proc.getPad (ln.pad);
        g.setColour (proc.selectedLane == l ? sodium : text);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        const bool narrow = row.getWidth() < 330;   // smal förklaring (SMIDET): namn på första raden, resten under
        auto nameArea = narrow ? row.removeFromTop (row.getHeight() / 2) : row.removeFromLeft (190);
        g.drawText (juce::String (l + 1) + " " + (info0.sample ? info0.sample->name : utf8 ("Tom")), nameArea, juce::Justification::centredLeft, true);
        juce::String d = juce::String (on) + "/" + juce::String (ln.length) + "  " + kRates[juce::jlimit (0, kNumRates - 1, ln.rate)].name;
        if (ln.euclid) d += "  E rot " + juce::String (ln.rotate);
        juce::StringArray magic;
        if (ln.rotEvery > 0) magic.add (utf8 ("roterar var ") + juce::String (ln.rotEvery));
        if (ln.breathe > 0) magic.add (utf8 ("andas ±") + juce::String (ln.breathe));
        if (ln.accHits > 0) magic.add ("acc " + juce::String (ln.accHits) + "/" + juce::String (ln.accLen));
        if (ln.ratHits > 0) magic.add ("rat " + juce::String (ln.ratHits) + "/" + juce::String (ln.ratLen));
        if (ln.toneHits > 0) magic.add ("ton " + juce::String (ln.toneHits) + "/" + juce::String (ln.toneLen));
        if (ln.fillOf >= 0) magic.add (utf8 ("fyller luckor i ") + juce::String (ln.fillOf + 1));
        g.setColour (muted);
        g.setFont (juce::FontOptions (11.0f));
        if (narrow)
        {
            auto sub = row.withTrimmedBottom (row.getHeight() / 3);
            g.drawText (d, sub.removeFromTop (sub.getHeight() / 2 + 2), juce::Justification::centredLeft, true);
            g.setColour (cold);
            g.drawText (magic.joinIntoString (" · "), row.withTrimmedTop (row.getHeight() / 2 - 2), juce::Justification::centredLeft, true);
        }
        else
        {
            g.drawText (d, row.removeFromLeft (150), juce::Justification::centredLeft, true);
            g.setColour (cold);
            g.drawText (magic.joinIntoString (" · "), row, juce::Justification::centredLeft, true);
        }
    }
    g.setColour (muted);
    g.setFont (juce::FontOptions (10.0f));
    if (getWidth() - (int) (R * 2.0f + 40.0f) >= 330)
        g.drawText (utf8 ("Dra runt = rotera · dra upp/ner eller rulla = antal slag · dubbelklick = euklid på/av"),
                    getLocalBounds().removeFromBottom (14).withTrimmedLeft ((int) (R * 2.0f + 40.0f)), juce::Justification::centredLeft, true);
}

void RingView::mouseDown (const juce::MouseEvent& e)
{
    dragLane = laneAt (e.position);
    if (dragLane < 0) return;
    if (selectLane) selectLane (dragLane);
    const auto pat = proc.getPattern();
    const auto& ln = pat.lanes[(size_t) dragLane];
    startAngle = angleOf (e.position);
    startY = e.position.y;
    startRotate = ln.rotate;
    startSteps = ln.steps;
    int on = 0; for (int i = 0; i < ln.length; ++i) on += ln.steps[(size_t) i].on ? 1 : 0;
    startHits = ln.euclid ? ln.hits : on;
    lastStepDelta = 0;
    repaint();
}

void RingView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragLane < 0) return;
    const auto pat = proc.getPattern();
    const auto& ln = pat.lanes[(size_t) dragLane];
    const int len = juce::jlimit (1, kMaxSteps, ln.length);

    if (std::abs (e.position.y - startY) > 24.0f && std::abs (e.getDistanceFromDragStartX()) < 12)
    {
        // dra rakt upp/ner = antal slag
        const int target = juce::jlimit (0, len, startHits + (int) ((startY - e.position.y) / 14.0f));
        proc.editLane (dragLane, [&] (Lane& l) { l.euclid = true; l.hits = target; });
    }
    else
    {
        float da = angleOf (e.position) - startAngle;
        while (da > juce::MathConstants<float>::pi) da -= juce::MathConstants<float>::twoPi;
        while (da < -juce::MathConstants<float>::pi) da += juce::MathConstants<float>::twoPi;
        const int delta = juce::roundToInt (da / (juce::MathConstants<float>::twoPi / (float) len));
        if (delta == lastStepDelta) return;
        lastStepDelta = delta;
        proc.editLane (dragLane, [&] (Lane& l)
        {
            if (l.euclid) l.rotate = ((startRotate + delta) % len + len) % len;
            else for (int i = 0; i < len; ++i) l.steps[(size_t) ((i + delta) % len + len) % len] = startSteps[(size_t) i];
        });
    }
    if (changed) changed();
    repaint();
}

void RingView::changeHits (int lane, int delta)
{
    proc.editLane (lane, [&] (Lane& l)
    {
        int on = 0; for (int i = 0; i < l.length; ++i) on += l.steps[(size_t) i].on ? 1 : 0;
        const int base = l.euclid ? l.hits : on;
        l.euclid = true;
        l.hits = juce::jlimit (0, l.length, base + delta);
    });
    if (changed) changed();
    repaint();
}

void RingView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int lane = laneAt (e.position);
    if (lane < 0) return;
    wheelAccum += w.deltaY * (w.isReversed ? -1.0f : 1.0f);
    const int dir = wheelAccum > 0.12f ? 1 : wheelAccum < -0.12f ? -1 : 0;
    if (dir == 0) return;
    wheelAccum = 0.0f;
    if (selectLane) selectLane (lane);
    changeHits (lane, dir);
}

void RingView::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int lane = laneAt (e.position);
    if (lane < 0) return;
    proc.editLane (lane, [&] (Lane& l)
    {
        if (! l.euclid) { int on = 0; for (int i = 0; i < l.length; ++i) on += l.steps[(size_t) i].on ? 1 : 0; l.hits = juce::jmax (1, on); }
        l.euclid = ! l.euclid;
    });
    if (changed) changed();
    repaint();
}

//==============================================================================
// Huvudvyn

BetongContent::BetongContent (BetongProcessor& p)
    : proc (p), wave (p), pads (p, [this] { selectionChanged(); }), library (p, [this] { selectionChanged(); })
{
    setLookAndFeel (&lnf);
    auto& s = proc.apvts;

    // header
    // presets i genremappar (undermenyer), i den ordning grupperna står i Presets.h
    auto fillGrouped = [] (juce::ComboBox& box, auto& table, int count)
    {
        juce::StringArray groups;
        for (int i = 0; i < count; ++i)
            groups.addIfNotAlreadyThere (utf8 (table[i].group != nullptr ? table[i].group : "Övrigt"));
        for (const auto& g : groups)
        {
            juce::PopupMenu sub;
            for (int i = 0; i < count; ++i)
                if (utf8 (table[i].group != nullptr ? table[i].group : "Övrigt") == g)
                    sub.addItem (i + 1, utf8 (table[i].name));
            box.getRootMenu()->addSubMenu (g, sub);
        }
    };
    fillGrouped (kitBox, kKits, kNumKits);
    fillGrouped (patternBox, kPatterns, kNumPatterns);
    fillGrouped (fxBox, kFx, kNumFx);
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
    for (auto* k : { &kCut, &kRes, &kDrive, &kEnv, &kDec, &kCrush, &kSendA, &kSendB, &kVol, &kPan, &kLaneSwing, &kPitch, &kDecay, &kPadVol, &kPadPan }) addAndMakeVisible (*k);
    kLaneSwing.setAccent (cold);
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
    for (int i = 0; i < kScenes; ++i)
    {
        auto& b = sceneButtons[(size_t) i];
        b.setButtonText (juce::String::charToString ((juce::juce_wchar) ('A' + i)));
        b.setTooltip (utf8 ("Scen ") + b.getButtonText() + utf8 (": byter på nästa takt när det spelar"));
        b.onClick = [this, i] { proc.queueScene (i); selectionChanged(); };
        addAndMakeVisible (b);
    }
    copyScene.setTooltip (utf8 ("Kopiera scenen du ser till en annan scen"));
    copyScene.onClick = [this]
    {
        juce::PopupMenu m;
        for (int i = 0; i < kScenes; ++i)
            if (i != proc.getScene()) m.addItem (i + 1, utf8 ("Kopiera till scen ") + juce::String::charToString ((juce::juce_wchar) ('A' + i)));
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&copyScene), [this] (int r) { if (r > 0) proc.copySceneTo (r - 1); });
    };
    addAndMakeVisible (copyScene);
    fillButton.setTooltip (utf8 ("Håll inne: trumvirvel på clap, snare och toms, dubbla slag på resten"));
    fillButton.setColour (juce::TextButton::textColourOffId, sodium);
    fillButton.onStateChange = [this] { proc.fillHeld.store (fillButton.isDown()); };
    addAndMakeVisible (fillButton);
    svajAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "svaj", svajSlider);
    glitchAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "glitch", glitchSlider);
    svajSlider.setTooltip (utf8 ("Svaj: slagen släpar lite, styrkan varierar och tonen glider. Mönstret ändras inte."));
    glitchSlider.setTooltip (utf8 ("Glitch: slumpvisa skurar, baklängesslag, tonhopp och tappade slag. Mönstret ändras inte."));
    densAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "dens", densSlider);
    densSlider.setTooltip (utf8 ("Densitet: vrid upp för fler slag, ner för glesare. Euklidiska spår lägger till och tar bort slag jämnt fördelat."));
    densSlider.setDoubleClickReturnValue (true, 0.0);
    for (auto* b : { &svajSlider, &glitchSlider, &densSlider })
    {
        b->setColour (juce::Slider::trackColourId, cold.withAlpha (0.45f));
        b->setColour (juce::Slider::textBoxTextColourId, text);
        b->setColour (juce::Slider::backgroundColourId, raised);
        b->setColour (juce::Slider::textBoxOutlineColourId, line2);
        addAndMakeVisible (*b);
    }

    // euklid-flik och ring-vy
    euclidPanel = std::make_unique<EuclidPanel> (proc, [this] { for (auto& r : rows) r->refresh(); if (ringView) ringView->repaint(); });
    addChildComponent (*euclidPanel);
    ringView = std::make_unique<RingView> (proc, [this] (int ln) { selectLane (ln); }, [this] { for (auto& r : rows) r->refresh(); if (euclidPanel) euclidPanel->refresh(); });
    addChildComponent (*ringView);
    for (auto* b : { &tabFilter, &tabEuclid, &ringButton })
    {
        b->setClickingTogglesState (false);
        addAndMakeVisible (*b);
    }
    tabFilter.setToggleState (true, juce::dontSendNotification);
    tabFilter.onClick = [this] { setSidePage (false); };
    tabEuclid.onClick = [this] { setSidePage (true); };
    tabEuclid.setTooltip (utf8 ("Euklid-magi för valt spår: rotation som rör sig, andning, lager och fyll luckorna"));
    ringButton.setTooltip (utf8 ("Visa spåren som euklidiska ringar"));
    ringButton.onClick = [this] { setRingMode (! ringView->isVisible()); };
    ringButton.setColour (juce::TextButton::buttonOnColourId, cold);

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

    if (kIsSmidet)
    {
        // SMIDET: ringarna är maskinen. Inget vågformsfönster, inga pads; spåret redigeras i smedjepanelen.
        kHammer.attach (s, "hammer", "HAMMARE");
        kGlow.attach (s, "glow", utf8 ("GLÖD"));
        kGlow.setAccent (juce::Colour (0xffffb347));
        kHammer.slider.setTooltip (utf8 ("Hammare: tätare, hårdare och fler skurar på en gång"));
        kGlow.slider.setTooltip (utf8 ("Glöd: resonans, drive och mer rumble"));
        soundButton.setTooltip (utf8 ("Välj ljud för det valda spåret"));
        soundButton.onClick = [this] { showLaneSoundMenu(); };
        soundButton.setColour (juce::TextButton::textColourOffId, sodium);
        for (auto* c : std::initializer_list<juce::Component*> { &kHammer, &kGlow, &soundButton }) addAndMakeVisible (*c);
        for (auto* c : std::initializer_list<juce::Component*> { &wave, &pads, &loadButton, &libButton, &chopButton, &chopMode, &sensitivity, &ringButton })
            c->setVisible (false);
        for (auto& r : rows) r->setCompact (true);
        setSidePage (true);
        setRingMode (true);
    }

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

void BetongContent::setSidePage (bool euclid)
{
    tabFilter.setToggleState (! euclid, juce::dontSendNotification);
    tabEuclid.setToggleState (euclid, juce::dontSendNotification);
    for (auto* c : std::initializer_list<juce::Component*> { &filterType, &kCut, &kRes, &kDrive, &kEnv, &kDec, &kCrush, &kSendA, &kSendB, &kVol, &kPan, &kLaneSwing })
        c->setVisible (! euclid);
    euclidPanel->setVisible (euclid);
    if (euclid) euclidPanel->refresh();
    repaint (sideArea);
}

void BetongContent::setRingMode (bool on)
{
    ringView->setVisible (on);
    ringButton.setToggleState (on, juce::dontSendNotification);
    for (auto& r : rows) r->setVisible (! on);
    repaint (seqArea);
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
        kLaneSwing.attach (s, BetongProcessor::laneId (l, "swing"), utf8 ("SWING SPÅR"));
    }
    if (p != shownPad)
    {
        shownPad = p;
        kPitch.attach (s, BetongProcessor::padId (p, "pitch"), "PITCH");
        kDecay.attach (s, BetongProcessor::padId (p, "decay"), "DECAY");
        kPadVol.attach (s, BetongProcessor::padId (p, "vol"), "PAD VOLYM");
        kPadPan.attach (s, BetongProcessor::padId (p, "pan"), "PAD PAN");
        revAtt.reset();
        revAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, BetongProcessor::padId (p, "rev"), revButton);
    }
    for (auto& r : rows) r->refresh();
    if (kIsSmidet)
    {
        for (int i = 0; i < kLanes; ++i) rows[(size_t) i]->setVisible (i == proc.selectedLane);
        soundButton.setButtonText (utf8 ("Ljud: ") + padName (proc, proc.getPattern().lanes[(size_t) proc.selectedLane].pad) + utf8 ("  ▾"));
    }
    if (euclidPanel != nullptr && euclidPanel->isVisible()) euclidPanel->refresh();
    if (ringView != nullptr) ringView->repaint();
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
    if (ringView != nullptr && ringView->isVisible()) ringView->repaint();

    const bool host = proc.hostPlaying.load();
    playButton.setButtonText (host ? utf8 ("DAW") : (playButton.getToggleState() ? utf8 ("STOPP") : utf8 ("SPELA")));
    playButton.setEnabled (! host);
    undoButton.setEnabled (proc.canUndo());
    // scenknappar: aktiv = fylld, köad = blinkar
    if (++blinkCount >= 8) { blinkCount = 0; blinkOn = ! blinkOn; }
    for (int i = 0; i < kScenes; ++i)
    {
        auto& b = sceneButtons[(size_t) i];
        const bool active = proc.getScene() == i, queued = proc.getQueuedScene() == i;
        b.setToggleState (active || (queued && blinkOn), juce::dontSendNotification);
        b.setColour (juce::TextButton::buttonOnColourId, queued ? cold : sodium);
    }
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
    g.drawText (BETONG_PRODUCT_NAME, logo.removeFromTop (36), juce::Justification::centredLeft);
    g.setColour (muted);
    g.setFont (juce::FontOptions (9.5f));
    g.drawText (utf8 (BETONG_TAGLINE " · MASKINHALLEN"), logo, juce::Justification::topLeft);

    auto bpmArea = headerArea.reduced (16, 8).removeFromRight (90);
    g.setColour (muted);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText (proc.hostPlaying.load() ? utf8 ("BPM · VÄRD") : utf8 ("BPM"), bpmArea.removeFromTop (14), juce::Justification::centredLeft);
    g.setColour (text);
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold).withHorizontalScale (0.8f));
    g.drawText (juce::String (proc.currentBpm.load(), 1), bpmArea.removeFromTop (30), juce::Justification::centredLeft);

    // vågform + pads (BETONG) eller smedjepanelen (SMIDET)
    if (kIsSmidet)
    {
        drawPanel (g, forgeArea, utf8 ("SMEDJAN · SPÅR ") + juce::String (proc.selectedLane + 1), utf8 ("Klicka på en ring för att välja spår"));
    }
    else
    {
        drawPanel (g, waveArea, utf8 ("SAMPLING · PAD ") + juce::String (proc.selectedPad + 1));
        drawPanel (g, padsArea, "PADS", utf8 ("Klick spelar · högerklick väljer ljud · dra in filer"));
    }

    // sidopanel
    drawPanel (g, sideArea, utf8 ("SPÅR ") + juce::String (proc.selectedLane + 1),
               "PAD " + juce::String (proc.selectedPad + 1));
    g.setColour (line);
    g.drawHorizontalLine (sideArea.getY() + 206, (float) sideArea.getX() + 12, (float) sideArea.getRight() - 12);

    // sequencer
    drawPanel (g, seqArea, kIsSmidet ? utf8 ("SMEDJANS RINGAR") : juce::String ("SEQUENCER"));

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
        auto tabs = s.removeFromTop (24).withTrimmedBottom (3);
        tabs.removeFromLeft (70);
        tabFilter.setBounds (tabs.removeFromLeft (76)); tabs.removeFromLeft (4);
        tabEuclid.setBounds (tabs.removeFromLeft (76));
        euclidPanel->setBounds (s.withHeight (24 + 6 + 70 + 70));
        auto typeRow = s.removeFromTop (24);
        filterType.setBounds (typeRow.removeFromLeft (110));
        s.removeFromTop (6);
        auto row1 = s.removeFromTop (70);
        auto row2 = s.removeFromTop (70);
        const int kw = row1.getWidth() / 6;
        for (auto* k : { &kCut, &kRes, &kDrive, &kEnv, &kDec }) k->setBounds (row1.removeFromLeft (kw));
        for (auto* k : { &kCrush, &kSendA, &kSendB, &kVol, &kPan, &kLaneSwing }) k->setBounds (row2.removeFromLeft (kw));
        s.removeFromTop (8);
        auto padRow = s.removeFromTop (70);
        for (auto* k : { &kPitch, &kDecay, &kPadVol, &kPadPan }) k->setBounds (padRow.removeFromLeft (kw));
        padRow.removeFromLeft (10);
        revButton.setBounds (padRow.withSizeKeepingCentre (padRow.getWidth(), 30));
    }

    // sequencer
    {
        auto s = seqArea.reduced (12, 8);
        auto head = s.removeFromTop (24).withTrimmedBottom (3);
        head.removeFromLeft (110);
        for (auto& b : sceneButtons) { b.setBounds (head.removeFromLeft (30)); head.removeFromLeft (3); }
        head.removeFromLeft (6);
        copyScene.setBounds (head.removeFromLeft (104));
        head.removeFromLeft (8);
        fillButton.setBounds (head.removeFromLeft (64));
        head.removeFromLeft (10);
        svajSlider.setBounds (head.removeFromLeft (124));
        head.removeFromLeft (6);
        glitchSlider.setBounds (head.removeFromLeft (124));
        head.removeFromLeft (6);
        densSlider.setBounds (head.removeFromLeft (140));
        head.removeFromLeft (10);
        ringButton.setBounds (head.removeFromLeft (60));
        const int rh = s.getHeight() / kLanes;
        ringView->setBounds (s);
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
    if (kIsSmidet) layoutSmidet();
}

void BetongContent::layoutSmidet()
{
    auto r = getLocalBounds().reduced (16);
    r.removeFromTop (64 + 12);
    r.removeFromBottom (96 + 12);
    auto right = r.removeFromRight (452);
    r.removeFromRight (12);
    seqArea = r;
    sideArea = right.removeFromTop (296);
    right.removeFromTop (12);
    forgeArea = right;
    waveArea = padsArea = {};

    auto s = seqArea.reduced (12, 8);
    auto head = s.removeFromTop (24).withTrimmedBottom (3);
    head.removeFromLeft (150);
    for (auto& b : sceneButtons) { b.setBounds (head.removeFromLeft (30)); head.removeFromLeft (3); }
    head.removeFromLeft (6);
    copyScene.setBounds (head.removeFromLeft (92)); head.removeFromLeft (6);
    fillButton.setBounds (head.removeFromLeft (56)); head.removeFromLeft (8);
    svajSlider.setBounds (head.removeFromLeft (110)); head.removeFromLeft (4);
    glitchSlider.setBounds (head.removeFromLeft (110)); head.removeFromLeft (4);
    densSlider.setBounds (head.removeFromLeft (juce::jmin (130, head.getWidth())));
    ringView->setBounds (s.withTrimmedTop (4));

    auto f = forgeArea.reduced (12, 8);
    f.removeFromTop (24);
    auto knobs = f.removeFromTop (112);
    kHammer.setBounds (knobs.removeFromLeft (108));
    kGlow.setBounds (knobs.removeFromLeft (108));
    knobs.removeFromLeft (12);
    soundButton.setBounds (knobs.withSizeKeepingCentre (knobs.getWidth(), 36));
    f.removeFromTop (10);
    const auto laneArea = f.removeFromTop (64);
    for (auto& row : rows) row->setBounds (laneArea);
}

void BetongContent::showLaneSoundMenu()
{
    const int pad = proc.getPattern().lanes[(size_t) proc.selectedLane].pad;
    const auto names = proc.library().getNames();
    juce::PopupMenu m;
    m.addSectionHeader (utf8 ("LJUD FÖR SPÅR ") + juce::String (proc.selectedLane + 1) + " (PAD " + juce::String (pad + 1) + ")");
    auto addCat = [&] (const Category& cat)
    {
        juce::PopupMenu sub;
        for (int i = 0; i < names.size(); ++i)
            if (BuiltInLibrary::categoryOf (names[i]) == cat.prefix)
                sub.addItem (1000 + i, BuiltInLibrary::displayName (names[i]));
        m.addSubMenu (utf8 (cat.name), sub);
    };
    for (auto& cat : kCategories) if (juce::String (cat.prefix) == "F") addCat (cat);   // smedjan först
    for (auto& cat : kCategories) if (juce::String (cat.prefix) != "F") addCat (cat);
    m.addSeparator();
    m.addItem (1, utf8 ("Ladda fil..."));
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&soundButton), [this, pad, names] (int r)
    {
        if (r >= 1000) { proc.loadBuiltInToPad (pad, names[r - 1000]); proc.triggerPad (pad); }
        else if (r == 1)
        {
            chooser = std::make_unique<juce::FileChooser> (utf8 ("Välj ett ljud"), juce::File(), BetongProcessor::audioWildcard);
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this, pad] (const juce::FileChooser& fc)
            {
                if (fc.getResult().existsAsFile() && proc.loadFileToPad (pad, fc.getResult())) proc.triggerPad (pad);
                selectionChanged();
            });
        }
        selectionChanged();
    });
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
