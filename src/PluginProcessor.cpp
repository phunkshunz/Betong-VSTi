#include "PluginProcessor.h"
#include "Rytmbank.h"
#include "PluginEditor.h"
#include "Presets.h"
#include <cstring>

using namespace betong;

namespace
{
const juce::StringArray kFilterTypes { "LP 24", "LP 12", "HP", "BP", "Notch", "Av" };
const juce::StringArray kDelayTimes { "1/8", "1/8 punkt", "3/16", "1/8T", "5/16", "1/4" };
constexpr double kDelayBeats[] = { 0.5, 0.75, 0.75, 1.0 / 3.0, 1.25, 1.0 };
const juce::StringArray kStutterLens { "1/4", "1/8", "1/16", "1/32" };
constexpr double kStutterBeats[] = { 1.0, 0.5, 0.25, 0.125 };

juce::NormalisableRange<float> freqRange (float lo, float hi)
{
    juce::NormalisableRange<float> r (lo, hi);
    r.setSkewForCentre (std::sqrt (lo * hi));
    return r;
}

int rateFromCode (const juce::String& c)
{
    if (c == "8") return 1;
    if (c == "16T") return 2;
    if (c == "32") return 3;
    if (c == "5") return 4;
    if (c == "7") return 5;
    return 0;
}

float digit (const char* s, int i) { return (s != nullptr && i < (int) std::strlen (s)) ? (float) (s[i] - '0') / 9.0f : 0.0f; }

// Standardpad per spår när ett mönster inte anger något
constexpr int kDefaultLanePads[kLanes] = { 0, 2, 3, 4, 6, 8, 12, 13 };
} // namespace

//==============================================================================
enum class Unit { Pct, Hz, Ms, Db, Semi, Bpm, Pan, Sweep, Swing, LaneSwing, Dens, Ms1, Fac };

static juce::String formatValue (Unit u, float v)
{
    using juce::String;
    switch (u)
    {
        case Unit::Pct:   return String (juce::roundToInt (v * 100.0f)) + " %";
        case Unit::Hz:    return v >= 1000.0f ? String (v / 1000.0f, 1) + " kHz" : String (juce::roundToInt (v)) + " Hz";
        case Unit::Ms:    return String (juce::roundToInt (v)) + " ms";
        case Unit::Db:    return (v > 0 ? "+" : "") + String (v, 1) + " dB";
        case Unit::Semi:  return (v > 0 ? "+" : "") + (std::abs (v - std::round (v)) < 0.005f ? String (juce::roundToInt (v)) : String (v, 2)) + " st";
        case Unit::Bpm:   return String (v, 1);
        case Unit::Pan:   return std::abs (v) < 0.02f ? String ("C") : (v < 0 ? "V " : "H ") + String (juce::roundToInt (std::abs (v) * 100.0f));
        case Unit::Sweep: return std::abs (v) < 0.02f ? String::fromUTF8 ("Av") : (v < 0 ? "LP " : "HP ") + String (juce::roundToInt (std::abs (v) * 100.0f)) + " %";
        case Unit::Swing: return String (juce::roundToInt (50.0f + v * 50.0f)) + " %";
        case Unit::Dens: return std::abs (v) < 0.02f ? String ("0") : (v > 0 ? "+" : "-") + String (juce::roundToInt (std::abs (v) * 100.0f)) + " %";
        case Unit::Ms1:   return String::fromUTF8 ("± ") + String (v, 1) + " ms";
        case Unit::Fac:   return String::fromUTF8 ("× ") + String (v, 2);
        case Unit::LaneSwing: return v < 0.0f ? String ("Global") : String (juce::roundToInt (50.0f + v * 50.0f)) + " %";
    }
    return String (v, 2);
}

static juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto f = [&] (const String& id, const String& name, NormalisableRange<float> r, float def, Unit unit = Unit::Pct)
    {
        auto attrs = AudioParameterFloatAttributes()
                         .withStringFromValueFunction ([unit] (float v, int) { return formatValue (unit, v); })
                         .withValueFromStringFunction ([] (const String& t) { return t.retainCharacters ("-0123456789.").getFloatValue(); });
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def, attrs));
    };
    auto c = [&] (const String& id, const String& name, const StringArray& choices, int def)
    { layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, choices, def)); };
    auto b = [&] (const String& id, const String& name, bool def)
    { layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def)); };

    f ("swing", "Swing", { 0.0f, 0.75f }, 0.1f, Unit::Swing);
    f ("crazy", "Galenskap", { 0.0f, 1.0f }, 0.4f);
    f ("bpm", "Internt tempo", { 60.0f, 200.0f, 0.1f }, 130.0f, Unit::Bpm);
    f ("sweep", "Master-svep", { -1.0f, 1.0f }, 0.0f, Unit::Sweep);
    f ("clip", "Clipper", { 0.0f, 1.0f }, 0.35f);
    f ("mcrush", "Master-grus", { 0.0f, 1.0f }, 0.0f);
    f ("out", "Utnivå", { -24.0f, 6.0f }, 0.0f, Unit::Db);
    c ("dTime", "Delay-tid", kDelayTimes, 2);
    f ("dFb", "Delay-återkoppling", { 0.0f, 0.95f }, 0.55f);
    f ("dTone", "Delay-ton", { 0.0f, 1.0f }, 0.45f);
    f ("dWob", "Delay-wobble", { 0.0f, 1.0f }, 0.2f);
    b ("dFreeze", "Delay-freeze", false);
    f ("dRet", "Delay-nivå", { 0.0f, 1.0f }, 0.4f);
    f ("rSize", "Rumble-storlek", { 0.0f, 1.0f }, 0.8f);
    f ("rDrive", "Rumble-drive", { 0.0f, 1.0f }, 0.55f);
    f ("rCut", "Rumble-cutoff", freqRange (40.0f, 300.0f), 100.0f, Unit::Hz);
    f ("rDuck", "Rumble-duck", { 0.0f, 1.0f }, 0.9f);
    f ("rRet", "Rumble-nivå", { 0.0f, 1.0f }, 0.7f);
    b ("stutter", "Stutter", false);
    c ("stutLen", "Stutter-längd", kStutterLens, 2);
    b ("fill", "Fill", false);
    f ("svaj", "Svaj", { 0.0f, 1.0f }, 0.0f);
    f ("glitch", "Glitch", { 0.0f, 1.0f }, 0.0f);
    f ("dens", "Densitet", { -1.0f, 1.0f }, 0.0f, Unit::Dens);
    f ("hammer", "Hammare", { 0.0f, 1.0f }, 0.0f);   // SMIDET: tätare, hårdare, fler skurar
    f ("glow", "Glöd", { 0.0f, 1.0f }, 0.0f);        // SMIDET: resonans, drive och mer rumble
    // FRIKTION: mikrotiming, hocketing och tejp. Allt på noll = rakt rutnät som förut.
    f ("pocket", "Pocket / släp", { 0.0f, 45.0f }, 0.0f, Unit::Ms);
    f ("troghet", String::fromUTF8 ("Tröghet"), { 0.0f, 1.0f }, 0.0f);
    f ("humanize", "Humanize", { 0.0f, 3.0f }, 0.0f, Unit::Ms1);
    f ("interlock", "Interlock", { 0.0f, 1.0f }, 0.0f);
    f ("mratchet", "Mikro-ratchet", { 0.0f, 1.0f }, 0.0f);
    f ("vdecay", "Velocity till decay", { 0.0f, 1.0f }, 0.0f);
    f ("tapeglue", "Tape glue / pump", { 0.0f, 1.0f }, 0.0f);
    // MIDI IN: stäng av BETONGS sekvenser och låt DAW:en, GLAPP eller KOLV spela padsen
    c ("seqMode", "Sekvenser", StringArray { "INTERN", String::fromUTF8 ("MIDI IN (sekvensern av)") }, 0);
    c ("midiMap", "MIDI-karta", StringArray { "BETONG (C1 = pad 1)", String::fromUTF8 ("GM-trummor"), String::fromUTF8 ("GM, bara kanal 10") }, 0);

    for (int l = 0; l < kLanes; ++l)
    {
        const String n = "Spår " + String (l + 1) + " ";
        c (BetongProcessor::laneId (l, "type"), n + "filtertyp", kFilterTypes, 5);
        f (BetongProcessor::laneId (l, "cut"), n + "cutoff", freqRange (30.0f, 18000.0f), 18000.0f, Unit::Hz);
        f (BetongProcessor::laneId (l, "res"), n + "resonans", { 0.0f, 1.0f }, 0.15f);
        f (BetongProcessor::laneId (l, "drive"), n + "drive", { 0.0f, 1.0f }, 0.0f);
        f (BetongProcessor::laneId (l, "env"), n + "envelope", { 0.0f, 1.0f }, 0.0f);
        f (BetongProcessor::laneId (l, "dec"), n + "env-decay", freqRange (5.0f, 1500.0f), 150.0f, Unit::Ms);
        f (BetongProcessor::laneId (l, "crush"), n + "crush", { 0.0f, 1.0f }, 0.0f);
        f (BetongProcessor::laneId (l, "sendA"), n + "sänd delay", { 0.0f, 1.0f }, 0.0f);
        f (BetongProcessor::laneId (l, "sendB"), n + "sänd rumble", { 0.0f, 1.0f }, l == 0 ? 0.9f : 0.0f);
        f (BetongProcessor::laneId (l, "vol"), n + "volym", { 0.0f, 1.5f }, 1.0f);
        f (BetongProcessor::laneId (l, "pan"), n + "panorering", { -1.0f, 1.0f }, 0.0f, Unit::Pan);
        f (BetongProcessor::laneId (l, "swing"), n + "swing", { -0.05f, 0.75f }, -0.05f, Unit::LaneSwing);
        f (BetongProcessor::laneId (l, "drag"), n + String::fromUTF8 ("släp"), { 0.0f, 2.0f }, 1.0f, Unit::Fac);
    }
    for (int p = 0; p < kPads; ++p)
    {
        const String n = "Pad " + String (p + 1) + " ";
        f (BetongProcessor::padId (p, "pitch"), n + "pitch", { -24.0f, 24.0f, 0.01f }, 0.0f, Unit::Semi);
        f (BetongProcessor::padId (p, "decay"), n + "decay", { 0.0f, 1.0f }, 1.0f);
        b (BetongProcessor::padId (p, "rev"), n + "baklänges", false);
        f (BetongProcessor::padId (p, "vol"), n + "volym", { -30.0f, 6.0f, 0.1f }, 0.0f, Unit::Db);
        f (BetongProcessor::padId (p, "pan"), n + "panorering", { -1.0f, 1.0f }, 0.0f, Unit::Pan);
    }

    // MOTOR
    b ("mOn", "Motor", false);
    f ("mMove", String::fromUTF8 ("Motor rörelse"), { 0.0f, 1.0f }, 0.4f);
    f ("mDepth", "Motor djup", { 0.0f, 1.0f }, 0.4f);
    f ("mChaos", "Motor kaos", { 0.0f, 1.0f }, 0.2f);
    f ("mFilt", "Motor filter", { 0.0f, 1.0f }, 0.5f);
    f ("mDub", "Motor dub", { 0.0f, 1.0f }, 0.4f);
    f ("mPump", "Motor pump", { 0.0f, 1.0f }, 0.3f);
    f ("mFeel", String::fromUTF8 ("Motor sväng"), { 0.0f, 1.0f }, 0.35f);
    {
        StringArray forms;
        for (int i = 0; i < betong::motor::kNumForms; ++i) forms.add (String::fromUTF8 (betong::motor::formName (i)));
        c ("mForm", "Motor form", forms, betong::motor::FormLat);
    }
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "mSeed", 1 }, "Motor seed", 1, 9999, 1));

    // MORF mellan scener
    f ("morph", "Morf", { 0.0f, 1.0f }, 0.0f);
    c ("morphTo", "Morf mot scen", { "A", "B", "C", "D" }, 1);
    return layout;
}

//==============================================================================
BetongProcessor::BetongProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BETONG", createLayout())
{
    formats.registerBasicFormats();

    juce::PropertiesFile::Options opts;
    opts.applicationName = "settings";
    opts.folderName = BETONG_PRODUCT_NAME;   // varje maskin har sina egna inställningar och mappar
    opts.filenameSuffix = ".xml";
    opts.osxLibrarySubFolder = "Application Support";
    settings = std::make_unique<juce::PropertiesFile> (opts);

    auto raw = [this] (const juce::String& id) { return apvts.getRawParameterValue (id); };
    pSwing = raw ("swing"); pBpm = raw ("bpm"); pSweep = raw ("sweep"); pClip = raw ("clip");
    pMCrush = raw ("mcrush"); pOut = raw ("out");
    pDTime = raw ("dTime"); pDFb = raw ("dFb"); pDTone = raw ("dTone"); pDWob = raw ("dWob");
    pDFreeze = raw ("dFreeze"); pDRet = raw ("dRet");
    pRSize = raw ("rSize"); pRDrive = raw ("rDrive"); pRCut = raw ("rCut"); pRDuck = raw ("rDuck"); pRRet = raw ("rRet");
    pStutter = raw ("stutter"); pStutLen = raw ("stutLen"); pFill = raw ("fill");
    pSvaj = raw ("svaj"); pGlitch = raw ("glitch"); pDens = raw ("dens"); pHammer = raw ("hammer"); pGlow = raw ("glow");
    pPocket = raw ("pocket"); pTroghet = raw ("troghet"); pHuman = raw ("humanize"); pInterlock = raw ("interlock");
    pMRatchet = raw ("mratchet"); pVDecay = raw ("vdecay"); pGlue = raw ("tapeglue");
    pSeqMode = raw ("seqMode"); pMidiMap = raw ("midiMap");
    for (int l = 0; l < kLanes; ++l) pLaneDrag[(size_t) l] = raw (laneId (l, "drag"));
    glitchSeed = (juce::uint32) juce::Random::getSystemRandom().nextInt();
    pMOn = raw ("mOn"); pMMove = raw ("mMove"); pMDepth = raw ("mDepth"); pMChaos = raw ("mChaos");
    pMFilt = raw ("mFilt"); pMDub = raw ("mDub"); pMPump = raw ("mPump"); pMFeel = raw ("mFeel");
    pMForm = raw ("mForm"); pMSeed = raw ("mSeed");
    pMorph = raw ("morph"); pMorphTo = raw ("morphTo");
    for (int l = 0; l < kLanes; ++l)
        lp[(size_t) l] = { raw (laneId (l, "type")), raw (laneId (l, "cut")), raw (laneId (l, "res")), raw (laneId (l, "drive")),
                           raw (laneId (l, "env")), raw (laneId (l, "dec")), raw (laneId (l, "crush")),
                           raw (laneId (l, "sendA")), raw (laneId (l, "sendB")), raw (laneId (l, "vol")), raw (laneId (l, "pan")),
                           raw (laneId (l, "swing")) };
    for (int p = 0; p < kPads; ++p)
        pp[(size_t) p] = { raw (padId (p, "pitch")), raw (padId (p, "decay")), raw (padId (p, "rev")),
                           raw (padId (p, "vol")), raw (padId (p, "pan")) };

    // Startläge: rena grundkitet, grundmönstret och fuktigt betonggolv
    auto findIndex = [] (auto& table, int count, const char* name)
    {
        for (int i = 0; i < count; ++i) if (juce::String::fromUTF8 (table[i].name) == juce::String::fromUTF8 (name)) return i;
        return 0;
    };
    applyKit (findIndex (kKits, kNumKits, kIsSmidet ? "Smedjan" : "Betong Grund"));
    applyPattern (findIndex (kPatterns, kNumPatterns, BETONG_DEFAULT_PATTERN));
    applyFx (findIndex (kFx, kNumFx, kIsSmidet ? "Smedjeglöd" : "Fuktigt Betonggolv"));
    // en ny maskin svänger direkt: lagom släp, tröghet och interlock. Sparade projekt ersätter detta.
    for (auto [id, v] : { std::pair<const char*, float> { "pocket", 14.0f }, { "troghet", 0.55f }, { "humanize", 1.5f },
                          { "interlock", 0.4f }, { "mratchet", 0.08f }, { "vdecay", 0.5f }, { "tapeglue", 0.3f } })
        setParam (id, v);

    startTimerHz (1);
}

BetongProcessor::~BetongProcessor() { stopTimer(); }

void BetongProcessor::setParam (const juce::String& id, float realValue)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (realValue));
}

float BetongProcessor::getParam (const juce::String& id) const
{
    if (auto* v = apvts.getRawParameterValue (id)) return v->load();
    return 0.0f;
}

bool BetongProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
void BetongProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    maxBlock = juce::jmax (32, samplesPerBlock);
    for (auto& b : laneBufs) b.setSize (2, maxBlock);
    for (auto& c : chains) c.prepare (sr, maxBlock);
    sendA.setSize (2, maxBlock); sendB.setSize (1, maxBlock);
    delayOut.setSize (2, maxBlock); rumbleOut.setSize (1, maxBlock);
    delay.prepare (sr);
    rumble.prepare (sr, maxBlock);
    stutter.prepare (sr);
    for (auto& s : masterSvf) s.reset();
    sideLp.setCutoff (120.0f, sr);
    for (auto& v : voices) v.s = nullptr;
    pumpBuf.assign ((size_t) maxBlock, 1.0f);
    glueBuf.assign ((size_t) maxBlock, 1.0f);
    throwEnv.fill (0.0f);
    pumpEnv = 0.0f;
    setLatencySamples (juce::roundToInt (chains[0].getLatency()));
}

//==============================================================================
// Pads och röster

BetongProcessor::PadInfo BetongProcessor::getPad (int pad) const
{
    PadInfo info;
    info.sample = padOwners[(size_t) pad];
    info.start = padSlots[(size_t) pad].start.load();
    info.end = padSlots[(size_t) pad].end.load();
    return info;
}

void BetongProcessor::setPad (int pad, SamplePtr s, int start, int end)
{
    if (! juce::isPositiveAndBelow (pad, kPads)) return;
    const int len = s ? s->data.getNumSamples() : 0;
    start = start < 0 ? 0 : juce::jlimit (0, juce::jmax (0, len - 1), start);
    end = end < 0 ? len : juce::jlimit (start + 1, juce::jmax (start + 1, len), end);

    auto& slot = padSlots[(size_t) pad];
    // ordningen spelar roll: först område, sedan pekaren (ljudtråden läser pekaren först)
    slot.sample.store (nullptr);
    slot.start.store (start);
    slot.end.store (end);
    slot.sample.store (s.get());

    if (padOwners[(size_t) pad] && padOwners[(size_t) pad] != s)
        graveyard.push_back ({ padOwners[(size_t) pad], juce::Time::getMillisecondCounter() });
    padOwners[(size_t) pad] = std::move (s);
    ++stateVersion;
}

void BetongProcessor::setPadRange (int pad, int start, int end)
{
    auto info = getPad (pad);
    if (info.sample) setPad (pad, info.sample, start, end);
}

bool BetongProcessor::loadFileToPad (int pad, const juce::File& file)
{
    auto s = loadSampleFile (file, formats);
    if (! s) return false;
    setPad (pad, s);
    return true;
}

void BetongProcessor::loadBuiltInToPad (int pad, const juce::String& name)
{
    if (auto s = builtIn->get (name)) setPad (pad, s);
}

int BetongProcessor::chopToPads (int sourcePad, int mode, float sensitivity)
{
    auto info = getPad (sourcePad);
    if (! info.sample) return 0;
    auto starts = findSlices (*info.sample, mode, sensitivity, kPads);
    const int len = info.sample->data.getNumSamples();
    const int n = (int) starts.size();
    for (int i = 0; i < n; ++i)
        setPad (i, info.sample, starts[(size_t) i], i + 1 < n ? starts[(size_t) i + 1] : len);
    return n;
}

void BetongProcessor::triggerPad (int pad) { triggerPadPitched (pad, 0); }

void BetongProcessor::triggerPadPitched (int pad, int semi)
{
    const int value = pad + (juce::jlimit (-24, 24, semi) + 24) * 32;
    int s1, n1, s2, n2;
    padFifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0) padFifoData[(size_t) s1] = value;
    else if (n2 > 0) padFifoData[(size_t) s2] = value;
    padFifo.finishedWrite (n1 + n2);
}

// MIDI-not till pad. GM-kartan följer pad-rollerna (1 kick, 3 stängd hat, 4 öppen, 5 clap, 6 virvel,
// 7–8 toms, 9–10 perc, 11 rim, 12 shaker). Perc-toner mellan 60 och 90 spelar pad 9 och behåller melodin.
int BetongProcessor::padForMidi (int note, int channel, int map, float& semi)
{
    semi = 0.0f;
    if (map == 0) return note - 36;
    if (map == 2 && channel != 10) return -1;
    switch (note)
    {
        case 35: case 36: return 0;
        case 37: return 10;
        case 38: case 40: return 5;
        case 39: return 4;
        case 41: case 43: return 6;
        case 45: case 47: case 48: case 50: return 7;
        case 42: case 44: return 2;
        case 46: case 51: case 53: case 59: return 3;
        case 49: case 52: case 55: case 57: return 15;
        case 54: case 69: case 70: case 82: return 11;
        case 56: return 8;
        default: break;
    }
    if (note >= 60 && note <= 90) { semi = (float) juce::jlimit (-12, 12, note - 75); return 8; }
    return -1;
}

int BetongProcessor::laneForPad (int pad) const
{
    for (int l = 0; l < kLanes; ++l)
        if (audioPattern.lanes[(size_t) l].pad == pad) return l;
    return kLanes; // direktbussen
}

void BetongProcessor::startVoice (int pad, int lane, float gain, int offset, bool chokeLane, float semi, int flags, float decayMul, float dark)
{
    auto& slot = padSlots[(size_t) pad];
    const Sample* s = slot.sample.load();
    if (s == nullptr) return;
    const int start = slot.start.load();
    const int end = juce::jmin (slot.end.load(), s->data.getNumSamples());
    if (end - start < 4) return;

    for (auto& v : voices)
    {
        if (v.s == nullptr || v.releasing) continue;
        // ett spår är monofoniskt (snyggt för bas och acid); hats chokar varandra (pad 3 och 4)
        const bool hatChoke = pad == 2 && v.pad == 3; // stängd hat tystar öppen
        if ((chokeLane && v.lane == lane && lane < kLanes) || hatChoke || v.pad == pad)
        {
            v.releasing = true;
            v.fadeStep = 1.0f / (float) (0.004 * sr);
        }
    }

    Voice* target = nullptr;
    for (auto& v : voices) if (v.s == nullptr) { target = &v; break; }
    if (target == nullptr) // stjäl den äldsta (minst kvar)
    {
        target = &voices[0];
        for (auto& v : voices) if (v.samplesLeft < target->samplesLeft) target = &v;
    }

    const auto& prm = pp[(size_t) pad];
    const float semis = prm.pitch->load() + semi;
    const bool reverse = (prm.rev->load() > 0.5f) != ((flags & 1) != 0);
    const float decay = prm.decay->load();

    Voice v;
    v.s = s; v.start = start; v.end = end; v.pad = pad; v.lane = lane;
    v.inc = std::exp2 (semis / 12.0) * s->sampleRate / sr * (reverse ? -1.0 : 1.0);
    v.pos = reverse ? (double) end - 1.001 : (double) start;
    v.gain = gain * juce::Decibels::decibelsToGain (prm.vol->load(), -30.0f);
    {
        const float pan = prm.pan->load();
        v.gl = std::cos ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f) * 1.4142f;
        v.gr = std::sin ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f) * 1.4142f;
    }
    v.delay = offset;
    v.amp = 1.0f; v.fade = 1.0f; v.releasing = false;
    v.decayCoef = decay >= 0.999f ? 1.0f
                                  : std::exp (-1.0f / (float) ((0.015 * std::pow (250.0, (double) decay)) * sr * decayMul));
    if (decay >= 0.999f && decayMul < 0.999f)   // FRIKTION: även ett ljud utan decay kan bli kortare (spökslag)
        v.decayCoef = std::exp (-1.0f / (float) (0.35 * decayMul * sr));
    if (flags & 2) v.decayCoef = std::exp (-1.0f / (float) (0.025 * sr)); // glitch: avhugget slag
    // FRIKTION: spökslag blir mörkare (en enkel lågpass runt 2 kHz)
    v.dark = juce::jlimit (0.0f, 1.0f, dark);
    v.lpC = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 2000.0f / (float) sr);
    v.lpL = v.lpR = 0.0f;
    v.samplesLeft = (int64_t) (sr * 40.0);
    *target = v;
    padHits[(size_t) pad].fetch_add (1);
}

void BetongProcessor::renderVoices (int numSamples)
{
    for (auto& v : voices)
    {
        if (v.s == nullptr) continue;
        auto& out = laneBufs[(size_t) v.lane];
        float* oL = out.getWritePointer (0);
        float* oR = out.getWritePointer (1);
        const float* inL = v.s->data.getReadPointer (0);
        const float* inR = v.s->data.getNumChannels() > 1 ? v.s->data.getReadPointer (1) : inL;
        const int lo = v.start, hi = v.end - 1;

        if (v.delay >= numSamples) { v.delay -= numSamples; continue; } // svaj: slaget kommer i ett senare block
        for (int i = v.delay; i < numSamples; ++i)
        {
            const int idx = (int) std::floor (v.pos);
            if (idx < lo || idx > hi || v.samplesLeft-- <= 0) { v.s = nullptr; break; }
            const float fr = (float) (v.pos - idx);
            const int i0 = juce::jmax (lo, idx - 1), i2 = juce::jmin (hi, idx + 1), i3 = juce::jmin (hi, idx + 2);
            // 4-punkts Hermite: skarpt även när ljudet pitchas
            auto herm = [fr] (float y0, float y1, float y2, float y3)
            {
                const float c1 = 0.5f * (y2 - y0);
                const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
                const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
                return ((c3 * fr + c2) * fr + c1) * fr + y1;
            };
            float l = herm (inL[i0], inL[idx], inL[i2], inL[i3]);
            float r = herm (inR[i0], inR[idx], inR[i2], inR[i3]);
            if (v.dark > 0.0f)
            {
                v.lpL += (l - v.lpL) * v.lpC; v.lpR += (r - v.lpR) * v.lpC;
                l += (v.lpL - l) * v.dark; r += (v.lpR - r) * v.dark;
            }

            // mjuk start (1 ms) mot klick när en slice börjar mitt i ett ljud
            const float g = v.gain * v.amp * v.fade;
            oL[i] += l * g * v.gl;
            oR[i] += r * g * v.gr;

            v.pos += v.inc;
            v.amp *= v.decayCoef;
            if (v.releasing) { v.fade -= v.fadeStep; if (v.fade <= 0.0f) { v.s = nullptr; break; } }
            if (v.amp < 0.0001f) { v.s = nullptr; break; }
        }
        v.delay = 0;
    }
}

//==============================================================================
float BetongProcessor::hash01 (juce::int64 n, int lane, juce::uint32 salt) const
{
    juce::uint32 x = (juce::uint32) n * 2654435761u ^ (juce::uint32) (lane + 1) * 40503u ^ salt * 0x9E3779B9u ^ glitchSeed;
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return (float) (x & 0xFFFFFF) / 16777216.0f;
}

// Räknar fram vad spår li spelar på sitt steg n: rotation som rör sig, andning, densitet,
// euklid-lager och "fyll luckorna". Mönstret självt ändras aldrig.
Step BetongProcessor::effectiveStep (const SeqState& q, int li, juce::int64 n, int depth) const
{
    const auto& base = q.pattern.lanes[(size_t) li];
    const int spb = kRates[juce::jlimit (0, kNumRates - 1, base.rate)].stepsPerBeat;
    const int len = juce::jlimit (1, kMaxSteps, base.length);
    const double bar = (double) n / spb / 4.0;
    const auto barI = (juce::int64) std::floor (bar);
    // motorn och Game of Life ändrar bara stegen, aldrig längd eller takt
    const auto& lane = (q.motorOn || base.life > 0 || q.morphAmt > 0.001f) ? liveLane (q, li, barI) : base;
    const int rot = lane.rotEvery > 0 ? (int) ((barI / lane.rotEvery) % len) : 0;
    const int i = (int) ((((n - rot) % len) + len) % len);
    Step st = lane.steps[(size_t) i];

    const float dens = juce::jlimit (-1.0f, 1.0f, pDens->load() + pHammer->load() * 0.6f);
    const float breath = lane.breathe > 0
        ? (float) lane.breathe * (float) std::sin (juce::MathConstants<double>::twoPi * bar / juce::jmax (1, lane.breatheBars))
        : 0.0f;

    if (lane.euclid)
    {
        float h = (float) lane.hits + breath;
        if (dens < 0.0f) h *= 1.0f + dens;
        else h += dens * ((float) len - h);
        const int hits = juce::jlimit (0, len, juce::roundToInt (h));
        st.on = euclidOn (i - lane.rotate, hits, len);
    }
    else
    {
        const float d = dens + breath / (float) len;
        if (d < 0.0f && st.on && ! st.accent && li > 0 && hash01 (n, li, 77) < -d) st.on = false;
        if (d > 0.0f && ! st.on && li > 0 && hash01 (n, li, 78) < d * 0.5f)
        {
            st.on = true; st.ghost = true; st.accent = false; st.prob = 100; st.ratchet = 1;
        }
    }

    // lagren räknas på det globala steget, så att de glider mot spårets egen längd
    if (lane.accHits > 0)
    {
        const bool a = euclidOn ((int) (n % juce::jmax (1, lane.accLen)), lane.accHits, lane.accLen);
        st.accent = a;
        if (a) st.ghost = false;
    }
    if (lane.ratHits > 0 && euclidOn ((int) (n % juce::jmax (1, lane.ratLen)), lane.ratHits, lane.ratLen))
        st.ratchet = (uint8_t) juce::jmax ((int) st.ratchet, lane.ratCount);
    if (lane.toneHits > 0 && euclidOn ((int) (n % juce::jmax (1, lane.toneLen)), lane.toneHits, lane.toneLen))
        st.semi = (int8_t) juce::jlimit (-24, 24, st.semi + lane.toneSemi);

    // fyll luckorna: spela bara där målspåret är tyst
    if (depth == 0 && lane.fillOf >= 0 && lane.fillOf != li)
    {
        bool own = lane.euclid && lane.hits > 0;
        for (int k = 0; k < len && ! own; ++k) own = lane.steps[(size_t) k].on;
        if (! own) { st.on = true; st.prob = 100; }
        if (st.on)
        {
            const auto& tl = q.pattern.lanes[(size_t) lane.fillOf];
            const int tspb = kRates[juce::jlimit (0, kNumRates - 1, tl.rate)].stepsPerBeat;
            const double pos = (double) n / spb * tspb;
            const double rp = std::round (pos);
            if (std::abs (pos - rp) < 1.0e-6 && effectiveStep (q, lane.fillOf, (juce::int64) rp, 1).on)
                st.on = false;
        }
    }

    // trig-villkor: 1:4, 3:4, FILL, PRE ...
    if (st.on && st.cond != CondAlways)
    {
        const bool fillNow = fillHeld.load() || pFill->load() > 0.5f;
        auto holds = [&] (int c, juce::int64 stepN) { return condTrue (c, motor::floorDiv (stepN, len), fillNow); };
        if (st.cond == CondPre || st.cond == CondNotPre)
        {
            // PRE: slog förra villkoret på spåret in? (det närmaste steget bakåt som har ett eget villkor)
            bool res = true;
            for (int k = 1; k <= len; ++k)
            {
                const auto& ps = lane.steps[(size_t) ((((i - k) % len) + len) % len)];
                if (ps.on && ps.cond != CondAlways && ps.cond < CondPre) { res = holds (ps.cond, n - k); break; }
            }
            st.on = st.cond == CondPre ? res : ! res;
        }
        else st.on = holds (st.cond, n);
    }

    // fråga och svar: spåret har ordet varannat block
    if (st.on && callRespSilent (lane, barI)) st.on = false;
    return st;
}

//==============================================================================
// MOTOR

// Motorns händelser på ett steg: false om spåret vilar, annars rullning och dub-kast.
bool BetongProcessor::motorStep (const SeqState& q, int li, juce::int64 n, int spb, Step& st, bool& throwHit) const
{
    const double beatPos = (double) n / spb;
    const auto barI = (juce::int64) std::floor (beatPos / 4.0);
    if (((motorMask (q, barI) >> li) & 1) == 0) return false;
    int roll = -1, thrown = -1;
    motorEvents (q, barI, roll, thrown);
    const double inBar = beatPos - (double) barI * 4.0;
    if (roll == li && inBar >= 3.0)
    {
        // rullning på sista taktslaget: tätare och starkare mot gränsen
        const int k = juce::jlimit (0, 3, (int) ((inBar - 3.0) * 4.0));
        st.on = true; st.prob = 100; st.ghost = k == 0; st.accent = k == 3;
        st.ratchet = (uint8_t) juce::jmax ((int) st.ratchet, 1 + k);
    }
    throwHit = thrown == li && inBar >= 3.5;
    return true;
}

motor::Settings BetongProcessor::motorSettings() const
{
    motor::Settings st;
    st.on = pMOn->load() > 0.5f;
    st.form = juce::jlimit (0, motor::kNumForms - 1, (int) pMForm->load());
    st.seed = (juce::uint32) juce::jmax (1, (int) std::lround (pMSeed->load()));
    st.move = pMMove->load(); st.depth = pMDepth->load(); st.chaos = pMChaos->load();
    st.filt = pMFilt->load(); st.dub = pMDub->load(); st.pump = pMPump->load(); st.feel = pMFeel->load();
    return st;
}

void BetongProcessor::newMotorSeed()
{
    setParam ("mSeed", (float) (1 + juce::Random::getSystemRandom().nextInt (9999)));
}

const Lane& BetongProcessor::liveLane (const SeqState& q, int li, juce::int64 bar) const
{
    auto& cached = q.evoLanes[(size_t) li];
    if (q.evoBars[(size_t) li] != bar)
    {
        cached = q.pattern.lanes[(size_t) li];
        makeLive (cached, li, q.ms, q.motorOn, bar, &q.scenes[(size_t) q.morphScene].lanes[(size_t) li], q.morphAmt);
        q.evoBars[(size_t) li] = bar;
    }
    return cached;
}

int BetongProcessor::motorMask (const SeqState& q, juce::int64 bar) const
{
    if (bar != q.maskBar)
    {
        q.maskBar = bar;
        q.maskCache = 0;
        for (int li = 0; li < kLanes; ++li)
        {
            const auto& l = q.pattern.lanes[(size_t) li];
            if (l.lock || motor::laneActive (q.ms, li, l.pad, (double) bar + 0.01)) q.maskCache |= 1 << li;
        }
    }
    return q.maskCache;
}

void BetongProcessor::motorEvents (const SeqState& q, juce::int64 bar, int& roll, int& thrown) const
{
    if (bar != q.eventBar)
    {
        q.eventBar = bar;
        const int mask = motorMask (q, bar);
        auto info = [&] (int li, int& pad, bool& ok)
        {
            const auto& l = q.pattern.lanes[(size_t) li];
            pad = l.pad;
            ok = ((mask >> li) & 1) != 0 && ! l.mute && ! l.lock;
        };
        q.rollCache = motor::rollLane (q.ms, (double) bar + 0.01, info);
        q.throwCache = motor::throwLane (q.ms, (double) bar + 0.01, info);
    }
    roll = q.rollCache; thrown = q.throwCache;
}

void BetongProcessor::runSequencer (int numSamples, double ppqStart, double bpm)
{
    const double samplesPerBeat = sr * 60.0 / bpm;
    const double ppqEnd = ppqStart + numSamples / samplesPerBeat;
    const float swing = pSwing->load();
    const bool fill = fillHeld.load() || pFill->load() > 0.5f;
    const float svaj = pSvaj->load();
    float glitch = juce::jmin (1.0f, pGlitch->load() + pHammer->load() * 0.25f);
    if (motorOn) glitch = juce::jmin (1.0f, glitch + motor::glitchBoost (ms, motor::energyAt (ms, ppqStart / 4.0)));
    const double ppqPerMs = bpm / 60000.0;
    const auto fk = readFric();
    // hur långt bakåt i tiden ett slag kan ha dragits in i det här blocket
    const double maxShiftPpq = fk.any ? fk.maxMs * ppqPerMs + 0.125 + 0.35 : 0.0;

    bool anySolo = false;
    for (auto& l : audioPattern.lanes) anySolo |= l.solo;

    for (int li = 0; li < kLanes; ++li)
    {
        const auto& lane = audioPattern.lanes[(size_t) li];
        const int spb = kRates[juce::jlimit (0, kNumRates - 1, lane.rate)].stepsPerBeat;
        const int len = juce::jlimit (1, kMaxSteps, lane.length);
        {
            const auto cur = (juce::int64) std::floor (ppqStart * spb);
            const int rot = lane.rotEvery > 0 ? (int) (((juce::int64) std::floor ((double) cur / spb / 4.0) / lane.rotEvery) % len) : 0;
            laneStep[(size_t) li].store ((int) ((((cur - rot) % len) + len) % len));
        }

        if (lane.mute || (anySolo && ! lane.solo)) continue;
        const bool swingable = (spb % 2) == 0;
        const float laneSw = lp[(size_t) li].swing->load();
        float sw = laneSw >= 0.0f ? laneSw : swing;
        const bool laneMotor = motorOn && ! lane.lock;
        const auto role = (li == 0 && lane.pad <= 1) ? motor::Kick : motor::roleOf (lane.pad);
        if (laneMotor) sw = juce::jlimit (0.0f, 0.75f, sw + motor::swingDrift (ms, ppqStart / 4.0));
        // Fill: clap, snare och toms (pad 5-8) rullar 1/16 med stigande styrka, övriga spår dubblar sina slag.
        const bool fillRoll = fill && li > 0 && lane.pad >= 4 && lane.pad <= 7;
        const bool fillDouble = fill && li > 0 && ! fillRoll;

        const auto first = (juce::int64) std::floor ((ppqStart - maxShiftPpq) * spb) - 1;
        const auto last = (juce::int64) std::floor (ppqEnd * spb) + 1; // +1: mikrotiming kan dra nästa slag hit
        for (auto n = juce::jmax ((juce::int64) 0, first); n <= last; ++n)
        {
            Step st = effectiveStep (seq, li, n, 0);
            bool throwHit = false;
            if (laneMotor && ! motorStep (seq, li, n, spb, st, throwHit)) continue;   // motorn har tystat spåret just nu
            if (fillRoll)
            {
                const double inBar = std::fmod ((double) n / spb, 4.0) / 4.0; // 0..1 genom takten
                st.on = true; st.prob = 100; st.ghost = false; st.accent = inBar > 0.7;
                st.ratchet = (uint8_t) (inBar > 0.75 ? 2 : 1);
            }
            else if (fillDouble && st.on)
                st.ratchet = (uint8_t) juce::jmax (2, (int) st.ratchet);
            if (! st.on) continue;

            // deterministisk slump per steg, så att ratchets över blockgränser fattar samma beslut
            auto h01 = [&] (juce::uint32 salt) { return hash01 (n, li, salt); };
            int flags = 0;
            float semiF = (float) st.semi;
            int ratOverride = 0;
            if (glitch > 0.001f && ! fillRoll && h01 (1) < glitch * 0.25f * (li == 0 ? 0.2f : 1.0f))
            {
                static const float jumps[] = { -12.0f, -7.0f, -5.0f, 5.0f, 7.0f, 12.0f };
                switch ((int) (h01 (2) * 5.0f))
                {
                    case 0: ratOverride = 2 + (int) (h01 (3) * 3.0f); break;   // skur
                    case 1: flags |= 1; break;                                // baklänges
                    case 2: semiF += jumps[(int) (h01 (4) * 6.0f) % 6]; break; // tonhopp
                    case 3: flags |= 2; break;                                // avhugget
                    default: if (li > 0) continue; break;                     // tappat slag
                }
            }
            double t0 = (double) n / spb;
            if (swingable && (n & 1)) t0 += sw * 0.5 / spb;
            float velMul = 1.0f;
            if (laneMotor)
            {
                t0 += motor::microMs (ms, li, role, n) * ppqPerMs;   // mikrotiming
                velMul = motor::velocityWave (ms, li, role, n);     // velocity-våg
            }
            // FRIKTION: släp, tröghet, humanize, interlock (duck eller spökrull), mikro-ratchet och velocity till decay
            FricHit fh;
            if (fk.any && ! fillRoll) fh = friktionHit (seq, li, n, spb, role, st, fk);
            t0 += fh.shiftMs * ppqPerMs;
            const int rat = ratOverride > 0 ? ratOverride : juce::jlimit (1, 4, (int) st.ratchet);
            for (int k = 0; k < rat; ++k)
            {
                // täta ratchets släpar också mer och mer
                const double tk = t0 + (double) k / (spb * rat) + (k > 0 ? fk.troghet * 4.0 * k * ppqPerMs : 0.0);
                if (tk < ppqStart || tk >= ppqEnd) continue;
                if (st.prob < 100 && audioRng.nextInt (100) >= st.prob) continue;
                int offset = juce::jlimit (0, numSamples - 1, (int) ((tk - ppqStart) * samplesPerBeat));
                float gain = stepGain (st) * (k == 0 ? 1.0f : 0.85f) * velMul * fh.gain;
                if (fillRoll) gain *= 0.45f + 0.55f * (float) (std::fmod (tk, 4.0) / 4.0);
                float semiHit = semiF;
                if (svaj > 0.001f)
                {
                    // svaj: slaget släpar upp till 16 ms, styrkan varierar och tonen glider några cent
                    const juce::uint32 kk = (juce::uint32) k;
                    offset += (int) (h01 (10 + kk) * svaj * 0.016 * sr * (li == 0 ? 0.4 : 1.0));
                    gain *= 1.0f - svaj * 0.35f * h01 (20 + kk);
                    semiHit += (h01 (30) - 0.5f) * svaj * 0.3f;
                }
                startVoice (lane.pad, li, gain, offset, true, semiHit, flags, fh.decay, fh.dark);
                if (k == 0)
                {
                    uiDragMs[(size_t) li].store ((float) fh.shiftMs);
                    uiLaneHits[(size_t) li].fetch_add (1);
                    if (fh.ghost) uiGhost[(size_t) li].fetch_add (1);
                    if (fh.ducked) uiDuck[(size_t) li].fetch_add (1);
                }
                laneTriggered[(size_t) li] = true;
                if (throwHit) throwEnv[(size_t) li] = 1.0f;
                if (li == 0 && role == motor::Kick && k == 0) kickHitAt = kickHitAt < 0 ? offset : juce::jmin (kickHitAt, offset);
                laneTrigAmount[(size_t) li] = juce::jmax (laneTrigAmount[(size_t) li], st.accent ? 1.0f : st.ghost ? 0.35f : 0.7f);
            }
            // MIKRO-RATCHET: slaget studsar 2-3 gånger, snabbare och svagare, som en repig nål eller en sen kvintol
            if (fh.stutter > 0 && ! (st.prob < 100 && h01 (77) * 100.0f >= (float) st.prob))
            {
                double tb = t0;
                for (int b = 1; b <= fh.stutter; ++b)
                {
                    tb += 0.1 * (1.0 - 0.18 * (b - 1));   // 1/32-kvintoler som tätnar
                    if (tb < ppqStart || tb >= ppqEnd) continue;
                    const int off = juce::jlimit (0, numSamples - 1, (int) ((tb - ppqStart) * samplesPerBeat));
                    startVoice (lane.pad, li, stepGain (st) * velMul * fh.gain * std::pow (0.55f, (float) b), off, true,
                                semiF, flags, 0.45f, juce::jmin (1.0f, 0.3f + 0.25f * (float) b));
                    if (b == 1) uiStut[(size_t) li].fetch_add (1);
                }
            }
        }
    }
}

BetongProcessor::FricKnobs BetongProcessor::readFric() const
{
    FricKnobs f;
    f.pocket = pPocket->load();
    f.troghet = pTroghet->load();
    f.humanMs = pHuman->load();
    f.interlock = pInterlock->load();
    f.mratchet = pMRatchet->load();
    f.vdecay = pVDecay->load();
    for (int l = 0; l < kLanes; ++l) f.laneDrag[(size_t) l] = pLaneDrag[(size_t) l]->load();
    f.maxMs = f.pocket * 2.0f * 1.25f + 40.0f * f.troghet + f.humanMs;
    f.any = f.pocket > 0.01f || f.troghet > 0.001f || f.humanMs > 0.01f || f.interlock > 0.001f || f.mratchet > 0.001f || f.vdecay > 0.001f;
    return f;
}

// Spårets prioritet i INTERLOCK: lägre = starkare. Kicken och tonerna är inte med (kicken har pumpen).
static int interlockRank (motor::Role r)
{
    switch (r)
    {
        case motor::Clap: return 1;
        case motor::Perc: return 2;
        case motor::Hat: case motor::OpenHat: return 3;
        default: return -1;
    }
}

BetongProcessor::FricHit BetongProcessor::friktionHit (const SeqState& q, int li, juce::int64 n, int spb, motor::Role role,
                                                      const Step& st, const FricKnobs& fk) const
{
    FricHit h;
    if (role == motor::Kick) return h;   // ankaret: alltid på rutnätet
    auto h01 = [&] (juce::uint32 salt) { return hash01 (n, li, 9000u + salt); };
    const auto& lane = q.pattern.lanes[(size_t) li];

    // POCKET: grundsläpet, olika mycket per roll, gånger spårets egen faktor
    float roleF = 1.0f;
    switch (role)
    {
        case motor::Hat: case motor::OpenHat: roleF = 1.0f; break;
        case motor::Clap: roleF = 0.8f; break;
        case motor::Perc: roleF = 1.2f; break;
        case motor::Bass: roleF = 0.3f; break;
        case motor::Stab: roleF = 0.6f; break;
        case motor::Lead: roleF = 0.5f; break;
        default: roleF = 1.0f; break;
    }
    double ms = fk.pocket * roleF * fk.laneDrag[(size_t) li];

    // TRÖGHET: slag i rad släpar mer och mer (+8, +18, +30 ms) och släpper vid nästa paus
    if (fk.troghet > 0.001f)
    {
        int run = 0;
        for (int b = 1; b <= 3; ++b)
        {
            if (n - b < 0 || ! effectiveStep (q, li, n - b, 0).on) break;
            ++run;
        }
        const bool next = effectiveStep (q, li, n + 1, 0).on;
        if (run > 0 || next)
        {
            static const float add[4] = { 8.0f, 18.0f, 30.0f, 30.0f };
            ms += fk.troghet * add[juce::jmin (run, 3)];
        }
    }
    // HUMANIZE: ± några ms, samma varje varv (följer slumpfröet), så det andas men inte brusar
    if (fk.humanMs > 0.01f) ms += (h01 (1) * 2.0f - 1.0f) * fk.humanMs;

    // INTERLOCK: krockar slaget med ett starkare mitt- eller diskantspår?
    const int myRank = interlockRank (role);
    if (fk.interlock > 0.001f && myRank > 0)
    {
        const double t = (double) n / spb;
        bool clash = false;
        for (int lj = 0; lj < kLanes && ! clash; ++lj)
        {
            if (lj == li) continue;
            const auto& other = q.pattern.lanes[(size_t) lj];
            if (other.mute) continue;
            const int r = interlockRank (motor::roleOf (other.pad));
            if (r < 1 || r > myRank || (r == myRank && lj > li)) continue;
            const int spbj = kRates[juce::jlimit (0, kNumRates - 1, other.rate)].stepsPerBeat;
            const double m = t * spbj;
            const auto mi = (juce::int64) std::llround (m);
            if (std::abs (m - (double) mi) > 1.0e-6) continue;
            clash = effectiveStep (q, lj, mi, 0).on;
        }
        if (clash)
        {
            if (fk.interlock > 0.5f && h01 (2) < (fk.interlock - 0.5f) * 2.0f)
            {   // SPÖKRULL: flytta 1/32 eller 1/64 bakåt, tystare, kortare, mörkare
                const double beats = h01 (3) < 0.55f ? 0.125 : 0.0625;
                ms += beats * 60000.0 / currentBpm.load();
                h.gain *= 0.6f; h.decay *= 0.55f; h.dark = 0.6f; h.ghost = true;
            }
            else
            {   // DUCK/CHOKE: det starkare spåret trycker ner det svagare, både volym och svans
                const float a = juce::jmin (1.0f, fk.interlock * 2.0f);
                h.gain *= 1.0f - 0.5f * a;
                h.decay *= 1.0f - 0.6f * a;
                h.ducked = true;
            }
        }
    }

    // VELOCITY TILL DECAY: spökslag kortare och mörkare, accenter öppnar svansen
    if (fk.vdecay > 0.001f)
    {
        if (st.accent) h.decay *= 1.0f + 0.7f * fk.vdecay;
        else if (st.ghost || h.ghost) { h.decay *= 1.0f - 0.45f * fk.vdecay; h.dark = juce::jmin (1.0f, h.dark + 0.45f * fk.vdecay); }
        // ju senare slaget landar, desto mörkare (det har "tappat energi" på vägen)
        h.dark = juce::jmin (1.0f, h.dark + fk.vdecay * (float) juce::jlimit (0.0, 1.0, ms / 60.0) * 0.35f);
    }

    // MIKRO-RATCHET: chans att slaget studsar (aldrig på taktslagen)
    if (fk.mratchet > 0.001f && (n % spb) != 0 && h01 (4) < fk.mratchet * 0.25f)
        h.stutter = h01 (5) < 0.6f ? 2 : 3;

    h.shiftMs = juce::jmax (-3.0, ms);
    juce::ignoreUnused (lane);
    return h;
}

//==============================================================================
void BetongProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int total = buffer.getNumSamples();
    buffer.clear();

    {   // senaste mönstren från GUI:t
        const juce::SpinLock::ScopedTryLockType tl (patternLock);
        if (tl.isLocked()) audioScenes = scenes;
    }

    // tempo och position
    double bpm = pBpm->load();
    bool hostIsPlaying = false;
    double ppq = 0.0;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            hostIsPlaying = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) ppq = *q;
        }
    }
    bpm = juce::jlimit (30.0, 300.0, bpm);
    hostPlaying.store (hostIsPlaying);
    currentBpm.store (bpm);
    const bool useInternal = ! hostIsPlaying && internalPlay.load();
    if (useInternal && ! wasInternal) internalPpq = 0.0;
    wasInternal = useInternal;
    if (useInternal) ppq = internalPpq;
    const bool sequencing = hostIsPlaying || useInternal;
    const bool seqOff = pSeqMode->load() > 0.5f;
    midiInMode.store (seqOff);
    const double beatSec = 60.0 / bpm;

    // scenbyte: på nästa takt (4 taktslag) när det spelar, direkt annars
    if (const int q = queuedScene.load(); q >= 0)
    {
        bool now = ! sequencing;
        if (sequencing)
        {
            const double endPpq = ppq + (double) total / (sr * beatSec);
            const double nextBar = std::ceil (ppq / 4.0 - 1.0e-9) * 4.0;
            now = nextBar < endPpq;
        }
        if (now) { scene.store (q); queuedScene.store (-1); ++stateVersion; }
    }
    audioPattern = audioScenes[(size_t) juce::jlimit (0, kScenes - 1, scene.load())];

    // morf mot en annan scen (gör inget om målet är scenen som spelar)
    morphScene = juce::jlimit (0, kScenes - 1, (int) pMorphTo->load());
    morphAmt = morphScene == scene.load() ? 0.0f : pMorph->load();

    // motorn: inställningarna läses en gång per block, cacharna börjar om
    ms = motorSettings();
    motorOn = ms.on && sequencing;
    motorRunning.store (motorOn);
    seq.resetCaches();
    if (! motorOn) { throwEnv.fill (0.0f); pumpEnv = 0.0f; pumpRising = false; }

    int done = 0;
    while (done < total)
    {
        const int n = juce::jmin (maxBlock, total - done);
        for (auto& b : laneBufs) b.clear (0, n);
        laneTriggered.fill (false);
        laneTrigAmount.fill (0.0f);
        kickHitAt = -1;

        // pads från GUI:t
        {
            int s1, n1, s2, n2;
            padFifo.prepareToRead (padFifo.getNumReady(), s1, n1, s2, n2);
            // värdet bär pad (0-31) och ton: pad + (ton + 24) * 32
            auto play = [this] (int v) { const int p = v % 32, semi = v / 32 - 24; if (juce::isPositiveAndBelow (p, kPads)) startVoice (p, laneForPad (p), 0.9f, 0, false, semi); };
            for (int i = 0; i < n1; ++i) play (padFifoData[(size_t) (s1 + i)]);
            for (int i = 0; i < n2; ++i) play (padFifoData[(size_t) (s2 + i)]);
            padFifo.finishedRead (n1 + n2);
        }

        // MIDI: BETONG-kartan (C1 = pad 1) eller GM-trummor. I läget MIDI IN är det enda som spelar.
        const int map = (int) pMidiMap->load();
        for (const auto m : midi)
        {
            const auto msg = m.getMessage();
            if (m.samplePosition < done || m.samplePosition >= done + n || ! msg.isNoteOn()) continue;
            float semi = 0.0f;
            const int pad = padForMidi (msg.getNoteNumber(), msg.getChannel(), map, semi);
            if (! juce::isPositiveAndBelow (pad, kPads)) continue;
            const int li = laneForPad (pad);
            const int off = m.samplePosition - done;
            startVoice (pad, li, msg.getFloatVelocity(), off, seqOff, semi);
            if (juce::isPositiveAndBelow (li, kLanes))
            {
                // samma bokföring som sekvensern: spårets filter-envelope, pumpen och tape glue reagerar
                laneTriggered[(size_t) li] = true;
                laneTrigAmount[(size_t) li] = juce::jmax (laneTrigAmount[(size_t) li], 0.4f + 0.6f * msg.getFloatVelocity());
                if (li == 0) kickHitAt = kickHitAt < 0 ? off : juce::jmin (kickHitAt, off);
            }
        }

        if (sequencing && ! seqOff)
        {
            const double chunkPpq = ppq + (double) done / (sr * beatSec);
            runSequencer (n, chunkPpq, bpm);
        }

        renderVoices (n);

        // motorn: var i låten vi är, och pumpen som duckas av kicken
        motor::Where mw;
        const double chunkBar = (ppq + (double) done / (sr * beatSec)) / 4.0;
        if (motorOn) mw = motor::locate (ms, chunkBar);
        const bool pumping = motorOn && ms.pump > 0.001f;
        if (pumping)
        {
            const float depthP = motor::pumpDepth (ms, mw.energy);
            const float rel = std::exp (-1.0f / (0.001f * motor::pumpReleaseMs (ms) * (float) sr));
            const float att = std::exp (-1.0f / (0.003f * (float) sr)); // 3 ms in, så det inte klickar
            for (int i = 0; i < n; ++i)
            {
                if (i == kickHitAt) pumpRising = true;
                if (pumpRising) { pumpEnv = 1.0f - (1.0f - pumpEnv) * att; if (pumpEnv > 0.98f) pumpRising = false; }
                else pumpEnv *= rel;
                pumpBuf[(size_t) i] = 1.0f - depthP * pumpEnv;
            }
        }

        // TAPE GLUE / PUMP: kickens energi trycker ner rumssvansarna från resten, så maskinen andas i baktakt
        const float glue = pGlue->load();
        const bool glueOn = glue > 0.001f && (int) glueBuf.size() >= n;
        if (glueOn)
        {
            const float rel = std::exp (-1.0f / (float) (beatSec * 0.28 * sr));
            const float att = std::exp (-1.0f / (0.002f * (float) sr));
            for (int i = 0; i < n; ++i)
            {
                if (i == kickHitAt) glueRising = true;
                if (glueRising) { gluePump = 1.0f - (1.0f - gluePump) * att; if (gluePump > 0.98f) glueRising = false; }
                else gluePump *= rel;
                glueBuf[(size_t) i] = 1.0f - glue * 0.45f * gluePump;
            }
            uiGlue.store (gluePump);
        }
        else { gluePump = 0.0f; glueRising = false; }

        // spårkedjor och sändningar
        sendA.clear (0, n); sendB.clear (0, n);
        auto* outL = buffer.getWritePointer (0, done);
        auto* outR = buffer.getWritePointer (1, done);
        for (int li = 0; li < kLanes; ++li)
        {
            auto& lb = laneBufs[(size_t) li];
            const auto& prm = lp[(size_t) li];
            if (laneTriggered[(size_t) li])
            {
                chains[(size_t) li].trigger (laneTrigAmount[(size_t) li]);
                if (li == 0) rumble.kickHit();
            }
            LaneChain::Params cp;
            cp.type = (int) prm.type->load();
            cp.cutoff = prm.cut->load();
            const float hammer = pHammer->load(), glow = pGlow->load();
            cp.resonance = juce::jmin (0.97f, prm.res->load() + glow * 0.35f);
            cp.drive = juce::jmin (1.0f, prm.drive->load() + hammer * 0.35f + glow * 0.25f);
            cp.envAmount = juce::jmin (1.0f, prm.env->load() + glow * 0.3f);
            cp.envDecayMs = prm.dec->load();
            cp.crush = prm.crush->load();
            const auto& laneData = audioPattern.lanes[(size_t) li];
            const bool laneMotor = motorOn && ! laneData.lock;
            if (laneMotor)
            {
                // andningsfilter, stängning inför skiften, resonanstoppar och drive som följer energin
                const auto m = motor::laneMod (ms, li, laneData.pad, chunkBar, mw);
                if (cp.type == LaneChain::Off)
                {
                    if (m.cutOct < -0.02f) { cp.type = LaneChain::LP12; cp.cutoff = 18000.0f * std::exp2 (m.cutOct); }
                }
                else if (cp.type == LaneChain::HP12) cp.cutoff = juce::jmin (16000.0f, cp.cutoff * std::exp2 (-m.cutOct * 0.6f));
                else cp.cutoff = juce::jmax (30.0f, cp.cutoff * std::exp2 (m.cutOct));
                cp.resonance = juce::jmin (0.95f, cp.resonance + m.res);
                cp.drive = juce::jmin (1.0f, cp.drive + m.drive);
            }
            chains[(size_t) li].process (lb, n, cp);

            const float vol = prm.vol->load();
            const float pan = prm.pan->load();
            const float gl = vol * std::cos ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f) * 1.4142f;
            const float gr = vol * std::sin ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f) * 1.4142f;
            float sa = prm.sendA->load();
            const float sb = prm.sendB->load();
            if (laneMotor) sa = juce::jmax (sa, throwEnv[(size_t) li] * 0.9f * ms.dub);   // dub-kast
            const bool ducked = pumping && li > 0 && laneData.pad > 1;
            const float* l = lb.getReadPointer (0);
            const float* r = lb.getReadPointer (1);
            auto* aL = sendA.getWritePointer (0); auto* aR = sendA.getWritePointer (1);
            auto* bM = sendB.getWritePointer (0);
            for (int i = 0; i < n; ++i)
            {
                const float pg = (ducked ? pumpBuf[(size_t) i] : 1.0f) * (glueOn && li > 0 && laneData.pad > 1 ? glueBuf[(size_t) i] : 1.0f);
                const float L = l[i] * gl * pg, R = r[i] * gr * pg;
                outL[i] += L; outR[i] += R;
                aL[i] += L * sa; aR[i] += R * sa;
                bM[i] += 0.5f * (L + R) * sb;
            }
        }
        // direktbussen (pads som inte ligger på något spår)
        buffer.addFrom (0, done, laneBufs[kLanes], 0, 0, n);
        buffer.addFrom (1, done, laneBufs[kLanes], 1, 0, n);

        // dub-kasten klingar av under ungefär ett taktslag och höjer återkopplingen medan de lever
        float maxThrow = 0.0f;
        if (motorOn)
        {
            const float dec = std::exp (-(float) n / (float) (beatSec * sr * 0.9));
            for (auto& t : throwEnv) { maxThrow = juce::jmax (maxThrow, t); t *= dec; }
        }
        const float dFb = juce::jmin (0.93f, pDFb->load() + 0.15f * ms.dub * maxThrow);

        // dub-delay
        const int dIdx = juce::jlimit (0, 5, (int) pDTime->load());
        delay.process (sendA.getReadPointer (0), sendA.getReadPointer (1),
                       delayOut.getWritePointer (0), delayOut.getWritePointer (1), n,
                       (float) (kDelayBeats[dIdx] * beatSec), dFb, pDTone->load(), pDWob->load(),
                       pDFreeze->load() > 0.5f);
        // rumble
        rumble.process (sendB.getReadPointer (0), rumbleOut.getWritePointer (0), n,
                        pRSize->load(), pRDrive->load(), pRCut->load(), pRDuck->load(), beatSec);

        const float glowM = pGlow->load();
        const float dRet = juce::jmin (1.0f, pDRet->load() + glowM * 0.25f);
        const float rRet = pRRet->load() * 0.7f * (1.0f + glowM * 0.8f); // rumblen ska ligga under kicken
        const auto* dL = delayOut.getReadPointer (0); const auto* dR = delayOut.getReadPointer (1);
        const auto* rm = rumbleOut.getReadPointer (0);

        // master: DJ-svep, grus, bas-mono, stutter, clipper
        const float sweepTarget = juce::jlimit (-1.0f, 1.0f, pSweep->load() + (motorOn ? motor::masterSweep (ms, mw) : 0.0f));
        float sweep = sweepSmoothed.load();
        const float mcrush = pMCrush->load();
        const float clip = juce::jmin (1.0f, pClip->load() + pHammer->load() * 0.25f);
        const float outGain = juce::Decibels::decibelsToGain (pOut->load());
        // clipper: drive in, men nästan oförändrad nivå för svaga signaler
        const float clipG = 1.0f + clip * clip * 1.6f; // max ca +8 dB in i mättningen: rakar toppar, plattar inte till
        const float clipNorm = 0.97f; // tak strax under 0 dBFS

        // -6 dB headroom på mixbussen innan mastern, så clippern får jobba med marginal
        for (int i = 0; i < n; ++i)
        {
            outL[i] = (outL[i] + dL[i] * dRet + rm[i] * rRet) * 0.5f;
            outR[i] = (outR[i] + dR[i] * dRet + rm[i] * rRet) * 0.5f;
        }

        // TAPE GLUE: varm, asymmetrisk bandmättnad över hela mixen, toppen rundas av runt 12 kHz
        if (glueOn)
        {
            const float drive = 1.0f + glue * 4.0f, bias = 0.15f * glue;
            const float norm = 1.0f / std::pow (drive, 0.6f), offs = std::tanh (bias);
            const float c = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 12000.0f / (float) sr);
            const float mix = juce::jmin (1.0f, glue * 1.5f);
            float* ch[2] = { outL, outR };
            for (int k = 0; k < 2; ++k)
                for (int i = 0; i < n; ++i)
                {
                    const float y = (std::tanh (ch[k][i] * drive + bias) - offs) * norm;
                    glueLp[k] += (y - glueLp[k]) * c;
                    ch[k][i] += (glueLp[k] - ch[k][i]) * mix;
                }
        }

        // svep: filterkoefficienter uppdateras var 32:a sampel
        for (int i = 0; i < n; ++i)
        {
            if ((i & 31) == 0)
            {
                sweep += (sweepTarget - sweep) * 0.08f;
                if (sweep < -0.02f)
                {
                    const float cut = 20000.0f * std::pow (100.0f / 20000.0f, -sweep);
                    masterSvf[0].setParams (cut, 0.25f, sr); masterSvf[1].setParams (cut, 0.25f, sr);
                }
                else if (sweep > 0.02f)
                {
                    const float cut = 20.0f * std::pow (250.0f, sweep);
                    masterSvf[0].setParams (cut, 0.25f, sr); masterSvf[1].setParams (cut, 0.25f, sr);
                }
            }
            if (sweep < -0.02f) { outL[i] = masterSvf[0].process (outL[i], Svf::LP); outR[i] = masterSvf[1].process (outR[i], Svf::LP); }
            else if (sweep > 0.02f) { outL[i] = masterSvf[0].process (outL[i], Svf::HP); outR[i] = masterSvf[1].process (outR[i], Svf::HP); }

            // bas-mono: ta bort sidosignalen under ca 120 Hz
            const float mid = 0.5f * (outL[i] + outR[i]);
            float side = 0.5f * (outL[i] - outR[i]);
            side -= sideLp.lp (side);
            outL[i] = mid + side; outR[i] = mid - side;

            if (mcrush > 0.001f)
            {
                const float q = std::exp2 (15.0f - mcrush * 11.0f);
                const float holdLen = 1.0f + mcrush * mcrush * (float) (sr / 5000.0);
                mHoldCount += 1.0f;
                if (mHoldCount >= holdLen) { mHoldCount -= holdLen; mHoldL = outL[i]; mHoldR = outR[i]; }
                outL[i] = std::round (mHoldL * q) / q;
                outR[i] = std::round (mHoldR * q) / q;
            }
        }
        sweepSmoothed.store (sweep);

        const int stIdx = juce::jlimit (0, 3, (int) pStutLen->load());
        stutter.process (outL, outR, n, pStutter->load() > 0.5f, (int) (kStutterBeats[stIdx] * beatSec * sr));

        for (int i = 0; i < n; ++i)
        {
            outL[i] = (clip > 0.01f ? fastTanh (outL[i] * clipG) * clipNorm : juce::jlimit (-1.0f, 1.0f, outL[i])) * outGain;
            outR[i] = (clip > 0.01f ? fastTanh (outR[i] * clipG) * clipNorm : juce::jlimit (-1.0f, 1.0f, outR[i])) * outGain;
        }

        done += n;
    }

    // läget till GUI:t
    if (sequencing) motorBar.store (ppq / 4.0);   // spelpositionen i takter (används även av liv och fråga/svar)
    seqRunning.store (sequencing);
    if (motorOn)
    {
        const double bar = ppq / 4.0;
        const auto w = motor::locate (ms, bar);
        motorEnergy.store (w.energy);
        motorSection.store (w.section);
        motorLaneMask.store (motorMask (seq, (juce::int64) std::floor (bar)));
    }
    else motorLaneMask.store (0xFF);

    if (useInternal)
        internalPpq += (double) total / (sr * beatSec);
}

//==============================================================================
// Mönster

Pattern BetongProcessor::getPattern() const
{
    const juce::SpinLock::ScopedLockType sl (patternLock);
    return scenes[(size_t) scene.load()];
}

void BetongProcessor::setPattern (const Pattern& p)
{
    const juce::SpinLock::ScopedLockType sl (patternLock);
    auto& dst = scenes[(size_t) scene.load()];
    dst = p;
    for (auto& l : dst.lanes) if (l.euclid) applyEuclid (l);
}

void BetongProcessor::queueScene (int s)
{
    s = juce::jlimit (0, kScenes - 1, s);
    if (s == scene.load()) { queuedScene.store (-1); return; }
    queuedScene.store (s);
}

Pattern BetongProcessor::getScenePattern (int s) const
{
    const juce::SpinLock::ScopedLockType sl (patternLock);
    return scenes[(size_t) juce::jlimit (0, kScenes - 1, s)];
}

// Avla: barnet får drag från båda föräldrarna. Varje spår ärver sin grund (pad, takt, längd)
// från en av dem, stegen korsas på ett slumpat ställe och en liten mutation läggs till.
void BetongProcessor::breedScenes (int partner, int target)
{
    partner = juce::jlimit (0, kScenes - 1, partner);
    target = juce::jlimit (0, kScenes - 1, target);
    pushUndo();
    juce::Random rng;
    const float crazy = getParam ("crazy");
    std::array<Pattern, kScenes> all;
    {
        const juce::SpinLock::ScopedLockType sl (patternLock);
        all = scenes;
    }
    const auto& a = all[(size_t) scene.load()];
    const auto& b = all[(size_t) partner];
    Pattern child;
    for (int li = 0; li < kLanes; ++li)
    {
        Lane pa = a.lanes[(size_t) li], pb = b.lanes[(size_t) li];
        if (pa.lock) { child.lanes[(size_t) li] = pa; continue; }
        if (pa.euclid) applyEuclid (pa);
        if (pb.euclid) applyEuclid (pb);
        const bool fromB = rng.nextBool();
        Lane c = fromB ? pb : pa;
        const Lane& other = fromB ? pa : pb;
        c.euclid = false;
        const int len = juce::jlimit (1, kMaxSteps, c.length);
        if (li == 0 && c.pad <= 1)
        {
            // kicken: fyra på golvet från grundföräldern, bara de svaga extraslagen korsas
            for (int i = 0; i < len; ++i)
                if (i % 4 != 0 && i < other.length && rng.nextBool()) c.steps[(size_t) i] = other.steps[(size_t) i];
        }
        else
        {
            const int cut = 1 + rng.nextInt (juce::jmax (1, len - 1));
            for (int i = cut; i < len; ++i)
                c.steps[(size_t) i] = i < other.length ? other.steps[(size_t) i] : Step {};
            // mutation: ett slag till eller ett mindre, oftare när galenskapen är uppe
            if (rng.nextFloat() < 0.3f + crazy * 0.5f)
            {
                auto& s = c.steps[(size_t) rng.nextInt (len)];
                if (s.on && ! s.accent) s.on = false;
                else if (! s.on) { s = Step {}; s.on = true; s.ghost = true; }
            }
        }
        c.mute = a.lanes[(size_t) li].mute;
        c.solo = false;
        child.lanes[(size_t) li] = c;
    }
    {
        const juce::SpinLock::ScopedLockType sl (patternLock);
        scenes[(size_t) target] = child;
    }
    currentPatternName = juce::String ("Avlat ") + juce::String::charToString ((juce::juce_wchar) ('A' + scene.load()))
                       + juce::String::charToString (0xd7) + juce::String::charToString ((juce::juce_wchar) ('A' + partner));
    queueScene (target);
    ++stateVersion;
}

//==============================================================================
// MIDI-export: det som faktiskt spelas (motor, liv, villkor, morf, swing och mikrotiming)
// räknas fram från takt 1 och skrivs som en MIDI-fil med ett spår per sequencerspår.
// Trumspår blir pad-noter (C1 = pad 1), tonala spår blir toner där F2 är grundtonen.

int BetongProcessor::midiExportBars (int choice) const
{
    if (choice > 0) return choice;
    const int f = motor::formBars (juce::jlimit (0, motor::kNumForms - 1, (int) pMForm->load()));
    return f > 0 ? f : 64;
}

juce::File BetongProcessor::exportMidi (int bars)
{
    bars = juce::jlimit (1, 512, bars);
    SeqState q;
    {
        const juce::SpinLock::ScopedLockType sl (patternLock);
        q.scenes = scenes;
        q.pattern = scenes[(size_t) scene.load()];
    }
    q.ms = motorSettings();
    q.motorOn = q.ms.on;
    q.morphScene = juce::jlimit (0, kScenes - 1, (int) pMorphTo->load());
    q.morphAmt = q.morphScene == scene.load() ? 0.0f : pMorph->load();
    q.resetCaches();

    const double bpm = currentBpm.load();
    const double ppqPerMs = bpm / 60000.0;
    constexpr int ticks = 960;
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (ticks);
    {
        juce::MidiMessageSequence head;
        head.addEvent (juce::MidiMessage::textMetaEvent (3, BETONG_PRODUCT_NAME), 0);
        head.addEvent (juce::MidiMessage::tempoMetaEvent (juce::roundToInt (60000000.0 / bpm)), 0);
        head.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);
        mf.addTrack (head);
    }

    bool anySolo = false;
    for (auto& l : q.pattern.lanes) anySolo |= l.solo;
    const float swing = pSwing->load();

    for (int li = 0; li < kLanes; ++li)
    {
        const auto& lane = q.pattern.lanes[(size_t) li];
        if (lane.mute || (anySolo && ! lane.solo)) continue;
        bool tonal = lane.pad >= 12;
        for (const auto& st : lane.steps) tonal |= st.on && st.semi != 0;
        const int spb = kRates[juce::jlimit (0, kNumRates - 1, lane.rate)].stepsPerBeat;
        const bool swingable = (spb % 2) == 0;
        const float laneSw = lp[(size_t) li].swing->load();
        const bool laneMotor = q.motorOn && ! lane.lock;
        const auto role = (li == 0 && lane.pad <= 1) ? motor::Kick : motor::roleOf (lane.pad);

        juce::MidiMessageSequence tr;
        auto info = getPad (lane.pad);
        tr.addEvent (juce::MidiMessage::textMetaEvent (3, juce::String (li + 1) + " " + (info.sample ? info.sample->name : juce::String ("Tom"))
                                                          + (tonal ? juce::String (" (F2 = grundton)") : juce::String())), 0);
        const juce::int64 total = (juce::int64) bars * 4 * spb;
        for (juce::int64 n = 0; n < total; ++n)
        {
            Step st = effectiveStep (q, li, n, 0);
            bool throwHit = false;
            if (laneMotor && ! motorStep (q, li, n, spb, st, throwHit)) continue;
            if (! st.on) continue;
            if (st.prob < 100 && hash01 (n, li, 555) * 100.0f >= (float) st.prob) continue;

            double t0 = (double) n / spb;
            float sw = laneSw >= 0.0f ? laneSw : swing;
            float velMul = 1.0f;
            if (laneMotor)
            {
                sw = juce::jlimit (0.0f, 0.75f, sw + motor::swingDrift (q.ms, t0 / 4.0));
                t0 += motor::microMs (q.ms, li, role, n) * ppqPerMs;
                velMul = motor::velocityWave (q.ms, li, role, n);
            }
            if (swingable && (n & 1)) t0 += sw * 0.5 / spb;
            const int rat = juce::jlimit (1, 4, (int) st.ratchet);
            const int note = juce::jlimit (0, 127, tonal ? 41 + st.semi : 36 + lane.pad);
            const int noteLen = juce::jmax (30, (int) (ticks * 0.9 / (spb * rat)));
            for (int k = 0; k < rat; ++k)
            {
                const double tk = t0 + (double) k / (spb * rat);
                const int tick = juce::jmax (0, juce::roundToInt (tk * ticks));
                const float vel = stepGain (st) * (k == 0 ? 1.0f : 0.85f) * velMul;
                tr.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) juce::jlimit (1, 127, juce::roundToInt (vel * 127.0f))), tick);
                tr.addEvent (juce::MidiMessage::noteOff (1, note), tick + noteLen);
            }
        }
        tr.sort();
        tr.updateMatchedPairs();
        mf.addTrack (tr);
    }

    auto name = currentPatternName.isEmpty() ? juce::String ("Monster") : currentPatternName;
    name = juce::File::createLegalFileName (juce::String (BETONG_PRODUCT_NAME) + " " + name + " " + juce::String (bars) + " takter.mid");
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (juce::String (BETONG_PRODUCT_NAME) + " MIDI");
    dir.createDirectory();
    auto file = dir.getChildFile (name);
    file.deleteFile();
    if (auto out = file.createOutputStream())
        mf.writeTo (*out, 1);
    return file;
}

//==============================================================================
// AI kopiera-klistra: frågan beskriver formatet, kitet och mönstret som spelas.
// Användaren klistrar in den i valfri AI-chatt och klistrar sedan tillbaka svaret.

juce::String BetongProcessor::aiPrompt() const
{
    auto pat = getPattern();
    juce::Array<juce::var> lanes;
    for (int li = 0; li < kLanes; ++li)
    {
        auto l = pat.lanes[(size_t) li];
        if (l.euclid) applyEuclid (l);
        bool any = false;
        for (int i = 0; i < l.length; ++i) any |= l.steps[(size_t) i].on;
        if (! any) continue;
        auto* o = new juce::DynamicObject();
        o->setProperty ("pad", l.pad + 1);
        o->setProperty ("rate", juce::String (kRates[juce::jlimit (0, kNumRates - 1, l.rate)].name));
        o->setProperty ("steps", stepsToString (l));
        bool tonal = false;
        juce::Array<juce::var> notes;
        for (int i = 0; i < l.length; ++i) { notes.add ((int) l.steps[(size_t) i].semi); tonal |= l.steps[(size_t) i].semi != 0; }
        if (tonal) o->setProperty ("notes", notes);
        lanes.add (juce::var (o));
    }
    auto* root = new juce::DynamicObject();
    root->setProperty ("name", currentPatternName);
    root->setProperty ("swing", juce::roundToInt (getParam ("swing") * 100.0f));
    root->setProperty ("lanes", lanes);
    const juce::String current = juce::JSON::toString (juce::var (root), true);

    juce::String pads;
    for (int p = 0; p < kPads; ++p)
    {
        auto info = getPad (p);
        pads << "  " << (p + 1) << " = " << (info.sample ? info.sample->name : juce::String ("tom")) << "\n";
    }

    juce::String s;
    s << juce::String::fromUTF8 ("STIL: [SKRIV HÄR vad du vill ha, t.ex. \"hypnotisk minimal, torr och mörk, som 1994\" eller ett låtnamn]\n\n")
      << juce::String::fromUTF8 ("Du är en technoproducent som programmerar trummaskinen BETONG. Gör ett mönster i stilen ovan. "
                                 "Om stilen är en låt eller artist: utgå från vad som är känt om dess tempo, groove, ljud och arrangemang, "
                                 "men kopiera inte låten, gör något nytt i samma anda.\n\n")
      << juce::String::fromUTF8 ("Svara ENDAST med JSON i exakt det här formatet, ingen annan text:\n")
      << "{\n"
      << juce::String::fromUTF8 ("  \"name\": \"kort namn\",\n")
      << juce::String::fromUTF8 ("  \"swing\": 0-75,\n")
      << juce::String::fromUTF8 ("  \"lanes\": [ högst 8 st: { \"pad\": 1-16, \"rate\": \"1/16\" | \"1/8\" | \"1/16T\" | \"1/32\" | \"5-del\" | \"7-del\", "
                                 "\"steps\": \"X...x...x...x...\", \"notes\": [halvtoner per steg, valfritt], \"cond\": [villkor per steg, valfritt] } ],\n")
      << juce::String::fromUTF8 ("  \"motor\": { \"on\": true, \"form\": \"Bara liv\" | \"Fri\" | \"Kort 64\" | \"Låt 128\" | \"Afterhour 256\", "
                                 "\"move\": 0-100, \"depth\": 0-100, \"chaos\": 0-100, \"filter\": 0-100, \"dub\": 0-100, \"pump\": 0-100, \"feel\": 0-100 }\n")
      << "}\n\n"
      << juce::String::fromUTF8 ("Regler:\n"
                                 "- steps: ett tecken per steg, 1-32 steg. . = tyst, x = slag, X = starkt, g = svagt (ghost), o = 50 % chans, p = 25 % chans, 2/3/4 = ratchet. "
                                 "Olika längder på spåren (t.ex. 16, 12, 7) ger polyrytmer som glider mot varandra.\n"
                                 "- Spår 1 är kicken och bär golvet. Minimal techno: få element, små variationer, ghost-noter och offbeat-hats.\n"
                                 "- notes: halvtoner från grundtonen F (0 = F, 3 = Ab, 7 = C, 10 = Eb, 12 = F upp). Bara för bas, stabs och lead.\n"
                                 "- cond: 0 alltid, 1 = 1:2, 2 = 2:2, 3 = 1:3, 4 = 2:3, 5 = 3:3, 6 = 1:4, 7 = 2:4, 8 = 3:4, 9 = 4:4, 10 = 1:8, 11 = 8:8, "
                                 "12 = bara under fill, 13 = inte under fill, 14 = om förra villkoret slog in, 15 = om det inte gjorde det.\n"
                                 "- motor: får beatet att leva av sig självt (rörelse, djup, kaos, filterandning, dub-kast, pump, sväng).\n\n"
                                 "Ljuden på padarna just nu:\n")
      << pads << "\n"
      << juce::String::fromUTF8 ("Mönstret som spelar nu (som exempel på formatet):\n") << current << "\n";
    return s;
}

juce::String BetongProcessor::applyAiAnswer (const juce::String& text)
{
    const int a = text.indexOfChar ('{'), b = text.lastIndexOfChar ('}');
    if (a < 0 || b <= a) return juce::String::fromUTF8 ("Hittade ingen JSON. Kopiera hela svaret från AI:n och försök igen.");
    juce::var root;
    const auto res = juce::JSON::parse (text.substring (a, b + 1), root);
    if (res.failed() || ! root.isObject()) return juce::String::fromUTF8 ("Svaret gick inte att läsa som JSON: ") + res.getErrorMessage();
    const auto* arr = root["lanes"].getArray();
    if (arr == nullptr || arr->isEmpty()) return juce::String::fromUTF8 ("Svaret saknar spår (\"lanes\").");

    pushUndo();
    Pattern p = getPattern();
    std::array<float, kLanes> laneSwing;
    laneSwing.fill (-0.05f);
    for (int li = 0; li < kLanes; ++li)
    {
        auto& lane = p.lanes[(size_t) li];
        if (lane.lock) continue;
        const bool keepMute = lane.mute;
        lane = Lane {};
        lane.mute = keepMute;
        lane.pad = kDefaultLanePads[li];
        if (li >= arr->size()) continue;
        const auto& L = arr->getReference (li);
        lane.pad = juce::jlimit (0, kPads - 1, (int) L.getProperty ("pad", li + 1) - 1);
        const auto rate = L.getProperty ("rate", "1/16").toString().trim();
        for (int r = 0; r < kNumRates; ++r)
            if (rate.equalsIgnoreCase (juce::String::fromUTF8 (kRates[r].name)) || (rate == rate.retainCharacters ("0123456789") && rate.getIntValue() == kRates[r].stepsPerBeat * 4))
                lane.rate = r;
        const auto steps = L.getProperty ("steps", "").toString().retainCharacters (".xXgop234").substring (0, kMaxSteps);
        if (steps.isEmpty()) continue;
        setStepsFromString (lane, steps);
        if (const auto* notes = L.getProperty ("notes", {}).getArray())
            for (int i = 0; i < notes->size() && i < kMaxSteps; ++i)
                lane.steps[(size_t) i].semi = (int8_t) juce::jlimit (-24, 24, (int) notes->getReference (i));
        if (const auto* conds = L.getProperty ("cond", {}).getArray())
            for (int i = 0; i < conds->size() && i < kMaxSteps; ++i)
                lane.steps[(size_t) i].cond = (uint8_t) juce::jlimit (0, kNumConds - 1, (int) conds->getReference (i));
    }
    setPattern (p);
    for (int l = 0; l < kLanes; ++l)
        if (! p.lanes[(size_t) l].lock) setParam (laneId (l, "swing"), laneSwing[(size_t) l]);
    if (root.hasProperty ("swing")) setParam ("swing", juce::jlimit (0.0f, 0.75f, (float) root["swing"] / 100.0f));

    if (const auto motorV = root["motor"]; motorV.isObject())
    {
        if (motorV.hasProperty ("on")) setParam ("mOn", (bool) motorV["on"] ? 1.0f : 0.0f);
        if (motorV.hasProperty ("form"))
        {
            const auto f = motorV["form"].toString();
            int form = -1;
            for (int i = 0; i < motor::kNumForms; ++i)
                if (f.equalsIgnoreCase (juce::String::fromUTF8 (motor::formName (i)))) form = i;
            if (form < 0) // AI:n skrev formen lite annorlunda: gå på siffran eller ordet
                form = f.contains ("256") ? motor::FormAfterhour : f.contains ("128") ? motor::FormLat : f.contains ("64") ? motor::FormKort
                     : f.containsIgnoreCase ("fri") ? motor::FormFri : f.containsIgnoreCase ("liv") ? motor::FormLiv : -1;
            if (form >= 0) setParam ("mForm", (float) form);
        }
        const std::pair<const char*, const char*> keys[] = { { "move", "mMove" }, { "depth", "mDepth" }, { "chaos", "mChaos" }, { "filter", "mFilt" },
                                                             { "dub", "mDub" }, { "pump", "mPump" }, { "feel", "mFeel" } };
        for (const auto& k : keys)
            if (motorV.hasProperty (k.first)) setParam (k.second, juce::jlimit (0.0f, 1.0f, (float) motorV[k.first] / 100.0f));
        if (motorV.hasProperty ("seed")) setParam ("mSeed", (float) juce::jlimit (1, 9999, (int) motorV["seed"]));
    }
    currentPatternName = "AI: " + root.getProperty ("name", "utan namn").toString().substring (0, 40);
    ++stateVersion;
    return {};
}

void BetongProcessor::copySceneTo (int target)
{
    target = juce::jlimit (0, kScenes - 1, target);
    const juce::SpinLock::ScopedLockType sl (patternLock);
    scenes[(size_t) target] = scenes[(size_t) scene.load()];
}

//==============================================================================
// Presets

void BetongProcessor::applyKit (int index)
{
    index = juce::jlimit (0, kNumKits - 1, index);
    for (int p = 0; p < kPads; ++p)
    {
        loadBuiltInToPad (p, kKits[index].pads[p]);
        setParam (padId (p, "pitch"), 0.0f);
        setParam (padId (p, "decay"), 1.0f);
        setParam (padId (p, "rev"), 0.0f);
        setParam (padId (p, "vol"), 0.0f);
        setParam (padId (p, "pan"), 0.0f);
    }
    currentKit = juce::String::fromUTF8 (kKits[index].name);
    ++stateVersion;
}

void BetongProcessor::applyPattern (int index)
{
    index = juce::jlimit (0, kNumPatterns - 1, index);
    const auto& pr = kPatterns[index];
    Pattern p = getPattern();
    std::array<float, kLanes> laneSwing;
    laneSwing.fill (-0.05f); // följer global swing
    for (int l = 0; l < kLanes; ++l)
    {
        auto& lane = p.lanes[(size_t) l];
        if (lane.lock) continue;
        const bool keepMute = lane.mute;
        lane = Lane {};
        lane.mute = keepMute;
        lane.pad = kDefaultLanePads[l];
        const char* spec = pr.lanes[l];
        for (int k = 0; k < l; ++k) if (pr.lanes[k] == nullptr) spec = nullptr; // listan tog slut tidigare
        if (spec == nullptr) { lane.length = 16; continue; }

        auto parts = juce::StringArray::fromTokens (juce::String::fromUTF8 (spec), ":", {});
        if (parts.size() < 3) continue;
        lane.pad = juce::jlimit (0, kPads - 1, parts[0].getIntValue() - 1);
        lane.rate = rateFromCode (parts[1]);
        int extra = 3;
        if (parts[2] == "e" && parts.size() >= 6)
        {
            lane.euclid = true;
            lane.hits = parts[3].getIntValue();
            lane.length = juce::jlimit (1, kMaxSteps, parts[4].getIntValue());
            lane.rotate = parts[5].getIntValue();
            applyEuclid (lane);
            extra = 6;
        }
        else
        {
            setStepsFromString (lane, parts[2]);
        }
        // tillägg: "n0,0,12,3" = ton per steg, "s35" = swing för spåret i procent (0-75)
        for (int k = extra; k < parts.size(); ++k)
        {
            const auto& x = parts[k];
            if (x.startsWith ("n"))
            {
                auto v = juce::StringArray::fromTokens (x.substring (1), ",", {});
                for (int i = 0; i < v.size() && i < kMaxSteps; ++i)
                    lane.steps[(size_t) i].semi = (int8_t) juce::jlimit (-24, 24, v[i].getIntValue());
            }
            else if (x.startsWith ("s")) laneSwing[(size_t) l] = juce::jlimit (0.0f, 0.75f, x.substring (1).getFloatValue() / 100.0f);
            else if (x.startsWith ("y"))
            {
                // trig-villkor per steg, t.ex. "y0,0,6,0" (index i condName)
                auto v = juce::StringArray::fromTokens (x.substring (1), ",", {});
                for (int i = 0; i < v.size() && i < kMaxSteps; ++i)
                    lane.steps[(size_t) i].cond = (uint8_t) juce::jlimit (0, kNumConds - 1, v[i].getIntValue());
            }
            else
            {
                // euklid-magi: r4 = rotera var 4:e takt, b3/16 = andas ±3 slag över 16 takter,
                // a3/8 = accentlager, q2/8/3 = ratchetlager, t2/5/7 = tonlager, f1 = fyll luckorna i spår 1
                auto v = juce::StringArray::fromTokens (x.substring (1), "/", {});
                auto at = [&] (int i, int def) { return i < v.size() && v[i].isNotEmpty() ? v[i].getIntValue() : def; };
                const auto c = x[0];
                if (c == 'r') lane.rotEvery = juce::jlimit (0, 16, at (0, 0));
                else if (c == 'b') { lane.breathe = juce::jlimit (0, kMaxSteps, at (0, 0)); lane.breatheBars = juce::jlimit (1, 64, at (1, 16)); }
                else if (c == 'a') { lane.accHits = at (0, 0); lane.accLen = juce::jlimit (1, kMaxSteps, at (1, 8)); }
                else if (c == 'q') { lane.ratHits = at (0, 0); lane.ratLen = juce::jlimit (1, kMaxSteps, at (1, 8)); lane.ratCount = juce::jlimit (2, 4, at (2, 2)); }
                else if (c == 't') { lane.toneHits = at (0, 0); lane.toneLen = juce::jlimit (1, kMaxSteps, at (1, 5)); lane.toneSemi = juce::jlimit (-24, 24, at (2, 12)); }
                else if (c == 'f') lane.fillOf = juce::jlimit (-1, kLanes - 1, at (0, 0) - 1);
                // L2 = Game of Life (1 puls, 2 kaos, 3 liv), v2/4 = svar i block om 4 takter
                else if (c == 'L') lane.life = juce::jlimit (0, kNumLife - 1, at (0, 0));
                else if (c == 'v') { lane.callResp = juce::jlimit (0, 2, at (0, 0)); lane.crBars = juce::jlimit (1, 16, at (1, 2)); }
            }
        }
    }
    setPattern (p);
    setParam ("swing", pr.swing);
    for (int l = 0; l < kLanes; ++l)
        if (! p.lanes[(size_t) l].lock) setParam (laneId (l, "swing"), laneSwing[(size_t) l]);
    currentPatternName = juce::String::fromUTF8 (pr.name);
    ++stateVersion;
}

void BetongProcessor::applyFx (int index)
{
    index = juce::jlimit (0, kNumFx - 1, index);
    const auto& f = kFx[index];
    setParam ("dTime", (float) f.delayTime);
    setParam ("dFb", f.delayFb); setParam ("dTone", f.delayTone); setParam ("dWob", f.delayWobble);
    setParam ("dRet", f.delayReturn); setParam ("dFreeze", 0.0f);
    setParam ("rSize", f.rumbleSize); setParam ("rDrive", f.rumbleDrive); setParam ("rCut", f.rumbleCut);
    setParam ("rDuck", f.rumbleDuck); setParam ("rRet", f.rumbleReturn);
    setParam ("clip", f.clip); setParam ("mcrush", f.crushMaster);
    for (int l = 0; l < kLanes; ++l)
    {
        setParam (laneId (l, "sendA"), digit (f.sendA, l));
        setParam (laneId (l, "sendB"), digit (f.sendB, l));
        setParam (laneId (l, "crush"), digit (f.crush, l) * 0.85f);
        setParam (laneId (l, "drive"), digit (f.drive, l));
        const float res = digit (f.reso, l);
        if (res > 0.0f)
        {
            setParam (laneId (l, "type"), 0.0f);           // LP 24
            setParam (laneId (l, "res"), 0.35f + res * 0.6f);
            setParam (laneId (l, "cut"), 500.0f + 1500.0f * (1.0f - res));
            setParam (laneId (l, "env"), 0.55f);
            setParam (laneId (l, "dec"), 180.0f);
        }
        else
        {
            setParam (laneId (l, "type"), 5.0f);           // av
            setParam (laneId (l, "env"), 0.0f);
        }
    }
    currentFx = juce::String::fromUTF8 (f.name);
    ++stateVersion;
}

//==============================================================================
// Slump

void BetongProcessor::pushUndo()
{
    juce::MemoryBlock mb;
    getStateInformation (mb);
    undoStack.push_back (mb.toBase64Encoding());
    while (undoStack.size() > 20) undoStack.pop_front();
}

bool BetongProcessor::undoRandom()
{
    if (undoStack.empty()) return false;
    juce::MemoryBlock mb;
    mb.fromBase64Encoding (undoStack.back());
    undoStack.pop_back();
    setStateInformation (mb.getData(), (int) mb.getSize());
    return true;
}

void BetongProcessor::randomKit()
{
    pushUndo();
    juce::Random rng;
    const float crazy = getParam ("crazy");
    static const char* betongCats[kPads] = { "K", "K", "H", "H", "C", "C", "P", "P", "P", "P", "P", "P", "B", "S", "L", "X" };
    static const char* smidetCats[kPads] = { "K", "K", "H", "H", "C", "C", "F", "F", "F", "F", "F", "F", "B", "F", "F", "F" };
    const char* const* roleCats = kIsSmidet ? smidetCats : betongCats;
    static const char* wild[] = { "P", "X", "V", "S", "L", "T", "H" };
    const auto pat = getPattern();
    for (int p = 0; p < kPads; ++p)
    {
        bool locked = false;
        for (auto& l : pat.lanes) if (l.lock && l.pad == p) locked = true;
        if (locked) continue;

        juce::String cat = roleCats[p];
        if (p == 14 && rng.nextBool()) cat = "T";
        if (p == 15 && rng.nextBool()) cat = "V";
        if (p >= 2 && rng.nextFloat() < crazy * 0.35f) cat = wild[rng.nextInt (7)];
        auto names = builtIn->getNamesInCategory (cat);
        if (names.isEmpty()) continue;
        loadBuiltInToPad (p, names[rng.nextInt (names.size())]);

        float pitch = 0.0f;
        if (p >= 2 && rng.nextFloat() < crazy * 0.4f)
            pitch = (float) juce::roundToInt ((rng.nextFloat() * 2.0f - 1.0f) * crazy * 7.0f);
        setParam (padId (p, "pitch"), pitch);
        setParam (padId (p, "rev"), (p >= 2 && rng.nextFloat() < crazy * 0.12f) ? 1.0f : 0.0f);
        setParam (padId (p, "decay"), (p >= 2 && rng.nextFloat() < crazy * 0.3f) ? 0.3f + rng.nextFloat() * 0.5f : 1.0f);
    }
    currentKit = juce::String::fromUTF8 ("Slump-kit");
    ++stateVersion;
}

// Smart slump: varje spår får en roll efter sin pad (kick, hats, klapp/perc, bas,
// stab, röst). Mönstren väljs ur rytmbanken (analyserade minimal/techno-arketyper)
// eller räknas fram euklidiskt med polymeter när GALENSKAP är hög, och kontrolleras
// mot reglerna för rollen. Det som skulle låta knas förkastas och slumpas om.
static mh::Role roleForPad (int pad)
{
    switch (pad)
    {
        case 0: case 1:   return mh::Role::Kick;
        case 2: case 3: case 11: return mh::Role::Tick;
        case 12:          return mh::Role::Bas;
        case 13: case 14: return mh::Role::Ton;
        case 15:          return mh::Role::Rost;
        default:          return mh::Role::Klick;   // clap, snare, toms, perc, rim
    }
}

static float roleDensity (mh::Role r, int pad, mh::Rng& rng)
{
    switch (r)
    {
        case mh::Role::Tick:  return pad == 3 ? 0.18f + 0.1f * rng.uni() : 0.3f + 0.35f * rng.uni();   // öppen hat glesare
        case mh::Role::Klick: return pad == 4 || pad == 5 ? 0.12f + 0.08f * rng.uni() : 0.15f + 0.2f * rng.uni();
        case mh::Role::Bas:   return 0.2f + 0.2f * rng.uni();
        case mh::Role::Ton:   return 0.1f + 0.15f * rng.uni();
        case mh::Role::Rost:  return 0.08f + 0.12f * rng.uni();
        case mh::Role::Luft: case mh::Role::Spoke: case mh::Role::Kick: default: return 0.25f;
    }
}

void BetongProcessor::randomPattern()
{
    pushUndo();
    juce::Random jr;
    mh::Rng rng ((uint32_t) jr.nextInt());
    const float crazy = getParam ("crazy");
    Pattern p = getPattern();

    static const int lanePads[kLanes][3] = { { 0, 0, 0 }, { 2, 2, 11 }, { 4, 5, 10 }, { 8, 9, 10 },
                                             { 6, 7, 8 }, { 3, 3, 11 }, { 12, 12, 12 }, { 13, 14, 15 } };

    for (int li = 0; li < kLanes; ++li)
    {
        auto& lane = p.lanes[(size_t) li];
        if (lane.lock) continue;
        const bool keepMute = lane.mute;
        lane = Lane {};
        lane.mute = keepMute;
        lane.pad = lanePads[li][rng.below (3)];
        const mh::Role role = roleForPad (lane.pad);

        if (role == mh::Role::Kick)   // kicken håller golvet
        {
            setStepsFromString (lane, "X...x...x...x...");
            if (rng.chance (crazy * 0.35f)) { auto& s = lane.steps[(size_t) (rng.chance (0.5f) ? 14 : 11)]; s.on = true; s.ghost = true; s.prob = 50; }
            continue;
        }

        const float target = roleDensity (role, lane.pad, rng);
        const bool poly = rng.chance (crazy * 0.7f) && role != mh::Role::Bas;
        bool hits[32] {}, ghosts[32] {}, accents[32] {};
        int len = 16;
        for (int attempt = 0; attempt < 16; ++attempt)
        {
            std::fill (std::begin (hits), std::end (hits), false);
            std::fill (std::begin (ghosts), std::end (ghosts), false);
            std::fill (std::begin (accents), std::end (accents), false);
            const auto& lib = mh::libraryFor (role);
            if (! poly && ! lib.empty())
            {
                int best = 0; float bestScore = 1e9f;
                for (size_t k = 0; k < lib.size(); ++k)
                {
                    const float sc = std::fabs (lib[k].density - target) + rng.uni() * 0.15f;
                    if (sc < bestScore) { bestScore = sc; best = (int) k; }
                }
                const char* str = lib[(size_t) best].steps;
                len = std::min ((int) std::strlen (str), 32);
                for (int k = 0; k < len; ++k)
                {
                    hits[k] = str[k] != '.';
                    ghosts[k] = str[k] == 'g';
                    accents[k] = str[k] == 'X';
                }
            }
            else
            {
                len = mh::goodCycle (role == mh::Role::Tick ? mh::Role::Spoke : role, rng);
                if (role == mh::Role::Tick && ! rng.chance (crazy)) len = 16;
                const int h = std::clamp ((int) std::lround (target * len), 1, std::max (1, len - 1));
                mh::euclid (h, len, rng.below (len), hits);
                if (role == mh::Role::Tick && hits[0])   // hats undviker ettan: offbeat-känsla
                {
                    bool tmp[32] {};
                    for (int k = 0; k < len; ++k) tmp[(k + 2) % len] = hits[k];
                    std::copy (tmp, tmp + len, hits);
                }
                for (int k = 0; k < len; ++k) if (hits[k] && rng.chance (0.2f)) ghosts[k] = true;
            }
            if (role == mh::Role::Bas)   // bas på kicken blir spökslag
                for (int k = 0; k < len; k += 4) if (hits[k]) ghosts[k] = true;
            if (mh::rhythmIsMusical (role, hits, ghosts, len)) break;
        }

        lane.length = len;
        // ljud i moll: få toner, mest grundtonen
        static const int bassNotes[] = { 0, 0, 0, 0, 0, 12, 7, 10, 3, -2 };
        static const int stabNotes[] = { 0, 0, 3, 7, 7, 10, 12, -5 };
        for (int k = 0; k < len; ++k)
        {
            auto& st = lane.steps[(size_t) k];
            st.on = hits[k];
            if (! st.on) continue;
            st.accent = accents[k] || (! ghosts[k] && rng.chance (0.12f));
            st.ghost = ghosts[k];
            if (role == mh::Role::Bas) st.semi = (int8_t) bassNotes[rng.below (10)];
            else if (role == mh::Role::Ton) st.semi = (int8_t) stabNotes[rng.below (8)];
            // galenskap: sannolikhet och ratchets, men aldrig på basen och aldrig på mer än ett par steg
            if (role != mh::Role::Bas && rng.chance (crazy * 0.25f)) st.prob = (uint8_t) (rng.chance (0.5f) ? 50 : 75);
            if ((role == mh::Role::Tick || role == mh::Role::Klick) && rng.chance (crazy * 0.08f)) st.ratchet = (uint8_t) (2 + rng.below (2));
        }
        // ibland långsammare takt på texturer (aldrig på bas och hats)
        if ((role == mh::Role::Ton || role == mh::Role::Rost) && rng.chance (crazy * 0.25f)) lane.rate = 2;
    }
    setPattern (p);

    // svängen från ett slumpat groove-DNA (udda 16-delarnas förskjutning -> swing)
    const int dnaIdx = rng.below (mh::kNumDna - 1);
    const auto& dna = mh::dnaTable (dnaIdx);
    float odd = 0.0f;
    for (int k = 1; k < 16; k += 2) odd += dna.offset[k];
    odd /= 8.0f;
    setParam ("swing", juce::jlimit (0.0f, 0.75f, 2.0f * odd + rng.uni() * 0.06f));
    for (int li = 0; li < kLanes; ++li)
        if (! p.lanes[(size_t) li].lock)
        {
            const auto r = roleForPad (p.lanes[(size_t) li].pad);
            // hats och perc får ibland lite mer sväng än resten, som en människa på en MPC
            const bool extra = (r == mh::Role::Tick || r == mh::Role::Klick) && rng.chance (0.25f + crazy * 0.3f);
            setParam (laneId (li, "swing"), extra ? juce::jlimit (0.0f, 0.75f, 2.0f * odd + 0.12f + rng.uni() * 0.15f) : -0.05f);
        }
    currentPatternName = juce::String::fromUTF8 ("Slump: ") + juce::String (dna.name);
    ++stateVersion;
}

void BetongProcessor::randomFx()
{
    pushUndo();
    juce::Random rng;
    const float crazy = getParam ("crazy");
    applyFx (rng.nextInt (kNumFx));
    auto wiggle = [&] (const char* id, float lo, float hi)
    {
        const float v = getParam (id) + (rng.nextFloat() * 2.0f - 1.0f) * crazy * 0.3f * (hi - lo);
        setParam (id, juce::jlimit (lo, hi, v));
    };
    wiggle ("dFb", 0.0f, 0.9f); wiggle ("dTone", 0.0f, 1.0f); wiggle ("dWob", 0.0f, 1.0f);
    wiggle ("rSize", 0.0f, 1.0f); wiggle ("rDrive", 0.0f, 1.0f); wiggle ("clip", 0.0f, 1.0f);
    if (rng.nextFloat() < crazy * 0.5f) setParam ("dTime", (float) rng.nextInt (6));
    for (int l = 1; l < kLanes; ++l)
        if (rng.nextFloat() < crazy * 0.3f)
            setParam (laneId (l, "sendA"), rng.nextFloat() * 0.8f);
    currentFx = juce::String::fromUTF8 ("Slump-effekter");
    ++stateVersion;
}

void BetongProcessor::randomAll()
{
    pushUndo();
    const auto keep = undoStack;
    randomKit(); randomPattern(); randomFx();
    undoStack = keep; // ett enda ångra-steg för hela slumpen
}

//==============================================================================
// Egna mappar

juce::StringArray BetongProcessor::getSampleFolders() const
{
    settings->reload();
    juce::StringArray folders;
    folders.addTokens (settings->getValue ("sampleFolders"), "\n", {});
    folders.removeEmptyStrings();
    return folders;
}

void BetongProcessor::addSampleFolder (const juce::File& folder)
{
    auto folders = getSampleFolders();
    folders.addIfNotAlreadyThere (folder.getFullPathName());
    settings->setValue ("sampleFolders", folders.joinIntoString ("\n"));
    settings->saveIfNeeded();
}

void BetongProcessor::removeSampleFolder (const juce::String& path)
{
    auto folders = getSampleFolders();
    folders.removeString (path);
    settings->setValue ("sampleFolders", folders.joinIntoString ("\n"));
    settings->saveIfNeeded();
}

juce::Array<juce::File> BetongProcessor::scanSampleFolders() const
{
    juce::Array<juce::File> result;
    for (const auto& path : getSampleFolders())
    {
        const juce::File folder (path);
        if (! folder.isDirectory()) continue;
        for (const auto& entry : juce::RangedDirectoryIterator (folder, true, audioWildcard, juce::File::findFiles))
        {
            result.add (entry.getFile());
            if (result.size() >= 5000) return result;
        }
    }
    struct ByName
    {
        static int compareElements (const juce::File& a, const juce::File& b) { return a.getFileName().compareNatural (b.getFileName()); }
    } sorter;
    result.sort (sorter);
    return result;
}

//==============================================================================
// Spara och läs in

juce::ValueTree BetongProcessor::buildState()
{
    auto tree = apvts.copyState();
    tree.removeChild (tree.getChildWithName ("PADS"), nullptr);
    tree.removeChild (tree.getChildWithName ("PATTERN"), nullptr);

    juce::ValueTree pads ("PADS");
    for (int p = 0; p < kPads; ++p)
    {
        auto info = getPad (p);
        juce::ValueTree pad ("PAD");
        pad.setProperty ("source", info.sample ? info.sample->source : juce::String(), nullptr);
        pad.setProperty ("start", info.start, nullptr);
        pad.setProperty ("end", info.end, nullptr);
        pads.appendChild (pad, nullptr);
    }
    tree.appendChild (pads, nullptr);

    std::array<Pattern, kScenes> all;
    {
        const juce::SpinLock::ScopedLockType sl (patternLock);
        all = scenes;
    }
    juce::ValueTree pat ("PATTERN");   // aktiv scen (bakåtkompatibelt)
    for (int l = 0; l < kLanes; ++l)
    {
        juce::ValueTree lane ("LANE");
        lane.setProperty ("data", laneToString (all[(size_t) scene.load()].lanes[(size_t) l]), nullptr);
        pat.appendChild (lane, nullptr);
    }
    tree.appendChild (pat, nullptr);
    juce::ValueTree sc ("SCENES");
    sc.setProperty ("active", scene.load(), nullptr);
    for (int i = 0; i < kScenes; ++i)
    {
        juce::ValueTree one ("SCENE");
        for (int l = 0; l < kLanes; ++l)
        {
            juce::ValueTree lane ("LANE");
            lane.setProperty ("data", laneToString (all[(size_t) i].lanes[(size_t) l]), nullptr);
            one.appendChild (lane, nullptr);
        }
        sc.appendChild (one, nullptr);
    }
    tree.removeChild (tree.getChildWithName ("SCENES"), nullptr);
    tree.appendChild (sc, nullptr);
    tree.setProperty ("kit", currentKit, nullptr);
    tree.setProperty ("patternName", currentPatternName, nullptr);
    tree.setProperty ("fx", currentFx, nullptr);
    tree.setProperty ("version", 2, nullptr);
    return tree;
}

void BetongProcessor::restoreState (const juce::ValueTree& tree)
{
    if (! tree.hasType (apvts.state.getType())) return;

    auto paramsOnly = tree.createCopy();
    paramsOnly.removeChild (paramsOnly.getChildWithName ("PADS"), nullptr);
    paramsOnly.removeChild (paramsOnly.getChildWithName ("PATTERN"), nullptr);
    paramsOnly.removeChild (paramsOnly.getChildWithName ("SCENES"), nullptr);
    std::vector<juce::String> missing;
    for (auto id : { "pocket", "troghet", "humanize", "interlock", "mratchet", "vdecay", "tapeglue" })
        if (! paramsOnly.getChildWithProperty ("id", id).isValid()) missing.push_back (id);
    apvts.replaceState (paramsOnly);
    // projekt från före FRIKTION: allt på noll, så de låter exakt som när de sparades
    for (auto& id : missing)
        if (auto* prm = apvts.getParameter (id)) prm->setValueNotifyingHost (prm->getDefaultValue());

    auto pads = tree.getChildWithName ("PADS");
    for (int p = 0; p < kPads && p < pads.getNumChildren(); ++p)
    {
        auto pad = pads.getChild (p);
        const juce::String src = pad.getProperty ("source");
        const int start = pad.getProperty ("start", 0);
        const int end = pad.getProperty ("end", -1);
        SamplePtr s;
        if (src.startsWith ("builtin:")) s = builtIn->get (src.fromFirstOccurrenceOf ("builtin:", false, false));
        else if (src.isNotEmpty() && juce::File (src).existsAsFile()) s = loadSampleFile (juce::File (src), formats);
        if (! s) s = builtIn->get (kKits[0].pads[p]); // filen finns inte längre: fyll med standardljud
        if (s) setPad (p, s, start, end);
    }

    auto sc = tree.getChildWithName ("SCENES");
    auto pat = tree.getChildWithName ("PATTERN");
    if (sc.isValid())
    {
        std::array<Pattern, kScenes> all;
        for (int i = 0; i < kScenes && i < sc.getNumChildren(); ++i)
            for (int l = 0; l < kLanes && l < sc.getChild (i).getNumChildren(); ++l)
                laneFromString (all[(size_t) i].lanes[(size_t) l], sc.getChild (i).getChild (l).getProperty ("data").toString());
        {
            const juce::SpinLock::ScopedLockType sl (patternLock);
            scenes = all;
        }
        scene.store (juce::jlimit (0, kScenes - 1, (int) sc.getProperty ("active", 0)));
        queuedScene.store (-1);
    }
    else if (pat.isValid())
    {
        Pattern p;
        for (int l = 0; l < kLanes && l < pat.getNumChildren(); ++l)
            laneFromString (p.lanes[(size_t) l], pat.getChild (l).getProperty ("data").toString());
        scene.store (0);
        setPattern (p);
    }
    currentKit = tree.getProperty ("kit").toString();
    currentPatternName = tree.getProperty ("patternName").toString();
    currentFx = tree.getProperty ("fx").toString();
    ++stateVersion;
}

void BetongProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = buildState().createXml())
        copyXmlToBinary (*xml, destData);
}

void BetongProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        restoreState (juce::ValueTree::fromXml (*xml));
}

void BetongProcessor::timerCallback()
{
    // släpp gamla ljud när inga röster längre kan spela dem (röster lever max 40 s)
    const auto now = juce::Time::getMillisecondCounter();
    graveyard.erase (std::remove_if (graveyard.begin(), graveyard.end(),
                                     [now] (auto& g) { return now - g.second > 45000; }),
                     graveyard.end());
}

//==============================================================================
// POLY: varje trumspår får sin egen längd efter roll. Kicken 16, hats 15, clap/rim 13, perc 11 och 13 ...
// Spåren glider isär och möts igen först efter hundratals takter, men kicken håller golvet.
void BetongProcessor::polyLengths()
{
    pushUndo();
    int percCount = 0;
    for (int li = 0; li < kLanes; ++li)
    {
        const auto pad = getPattern().lanes[(size_t) li].pad;
        const auto role = (li == 0 && pad <= 1) ? motor::Kick : motor::roleOf (pad);
        int len = 16;
        switch (role)
        {
            case motor::Hat: case motor::OpenHat: len = 15; break;
            case motor::Clap: len = 13; break;
            case motor::Perc: len = (percCount++ % 2) == 0 ? 11 : 13; break;
            default: len = 16; break;
        }
        editLane (li, [len] (Lane& l) { if (! l.lock) l.length = len; });
    }
    ++stateVersion;
}

// DICE // POCKET: nytt släp, tröghet och interlock, egna släp-faktorer per spår och nya
// polyrytmiska längder. Alltid inom ramar som svänger, aldrig så det spårar ur. Ångra tar tillbaka.
void BetongProcessor::dicePocket()
{
    pushUndo();
    juce::Random rng;
    auto range = [&] (float lo, float hi) { return lo + (hi - lo) * rng.nextFloat(); };
    setParam ("pocket", range (6.0f, 24.0f));
    setParam ("troghet", range (0.3f, 0.85f));
    setParam ("humanize", range (0.6f, 2.2f));
    setParam ("interlock", range (0.2f, 0.8f));
    setParam ("mratchet", rng.nextFloat() < 0.6f ? range (0.04f, 0.25f) : 0.0f);
    setParam ("vdecay", range (0.3f, 0.85f));
    setParam ("tapeglue", range (0.1f, 0.55f));
    for (int l = 0; l < kLanes; ++l) setParam (laneId (l, "drag"), range (0.5f, 1.6f));

    static const int sets[][4] = { { 15, 13, 11, 16 }, { 16, 15, 13, 11 }, { 15, 11, 13, 16 }, { 13, 15, 11, 16 } };
    const auto& pick = sets[rng.nextInt (4)];
    int perc = 0, elevens = 0;
    for (int li = 0; li < kLanes; ++li)
    {
        const auto pad = getPattern().lanes[(size_t) li].pad;
        const auto role = (li == 0 && pad <= 1) ? motor::Kick : motor::roleOf (pad);
        if (role == motor::Kick || motor::isTonal (role) || role == motor::Fx) continue;
        int len = role == motor::Hat || role == motor::OpenHat ? pick[0] : role == motor::Clap ? pick[1] : pick[2 + (perc++ % 2)];
        if (len == 11 && ++elevens > 1) len = 13;   // högst ett spår på 11
        const int rot = rng.nextInt (len);
        editLane (li, [len, rot] (Lane& l)
        {
            if (l.lock) return;
            l.length = len;
            if (l.euclid) l.rotate = rot;
        });
    }
    ++stateVersion;
}

juce::AudioProcessorEditor* BetongProcessor::createEditor() { return new BetongEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BetongProcessor(); }
