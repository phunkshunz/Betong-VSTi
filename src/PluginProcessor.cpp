#include "PluginProcessor.h"
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
enum class Unit { Pct, Hz, Ms, Db, Semi, Bpm, Pan, Sweep, Swing, LaneSwing };

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
    opts.folderName = "BETONG";
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
    for (int l = 0; l < kLanes; ++l)
        lp[(size_t) l] = { raw (laneId (l, "type")), raw (laneId (l, "cut")), raw (laneId (l, "res")), raw (laneId (l, "drive")),
                           raw (laneId (l, "env")), raw (laneId (l, "dec")), raw (laneId (l, "crush")),
                           raw (laneId (l, "sendA")), raw (laneId (l, "sendB")), raw (laneId (l, "vol")), raw (laneId (l, "pan")),
                           raw (laneId (l, "swing")) };
    for (int p = 0; p < kPads; ++p)
        pp[(size_t) p] = { raw (padId (p, "pitch")), raw (padId (p, "decay")), raw (padId (p, "rev")),
                           raw (padId (p, "vol")), raw (padId (p, "pan")) };

    // Startläge: ångvälts-kit, mönster och fuktigt betonggolv
    auto findIndex = [] (auto& table, int count, const char* name)
    {
        for (int i = 0; i < count; ++i) if (juce::String::fromUTF8 (table[i].name) == juce::String::fromUTF8 (name)) return i;
        return 0;
    };
    applyKit (findIndex (kKits, kNumKits, "Ångvälten"));
    applyPattern (findIndex (kPatterns, kNumPatterns, "Ångvält 138"));
    applyFx (findIndex (kFx, kNumFx, "Fuktigt Betonggolv"));

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

int BetongProcessor::laneForPad (int pad) const
{
    for (int l = 0; l < kLanes; ++l)
        if (audioPattern.lanes[(size_t) l].pad == pad) return l;
    return kLanes; // direktbussen
}

void BetongProcessor::startVoice (int pad, int lane, float gain, int offset, bool chokeLane, int semi)
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
    const float semis = prm.pitch->load() + (float) semi;
    const bool reverse = prm.rev->load() > 0.5f;
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
                                  : std::exp (-1.0f / (float) ((0.015 * std::pow (250.0, (double) decay)) * sr));
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
            const float l = herm (inL[i0], inL[idx], inL[i2], inL[i3]);
            const float r = herm (inR[i0], inR[idx], inR[i2], inR[i3]);

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
void BetongProcessor::runSequencer (int numSamples, double ppqStart, double bpm)
{
    const double samplesPerBeat = sr * 60.0 / bpm;
    const double ppqEnd = ppqStart + numSamples / samplesPerBeat;
    const float swing = pSwing->load();
    const bool fill = fillHeld.load() || pFill->load() > 0.5f;

    bool anySolo = false;
    for (auto& l : audioPattern.lanes) anySolo |= l.solo;

    for (int li = 0; li < kLanes; ++li)
    {
        const auto& lane = audioPattern.lanes[(size_t) li];
        const int spb = kRates[juce::jlimit (0, kNumRates - 1, lane.rate)].stepsPerBeat;
        const int len = juce::jlimit (1, kMaxSteps, lane.length);
        laneStep[(size_t) li].store ((int) ((juce::int64) std::floor (ppqStart * spb) % len));

        if (lane.mute || (anySolo && ! lane.solo)) continue;
        const bool swingable = (spb % 2) == 0;
        const float laneSw = lp[(size_t) li].swing->load();
        const float sw = laneSw >= 0.0f ? laneSw : swing;
        // Fill: clap, snare och toms (pad 5-8) rullar 1/16 med stigande styrka, övriga spår dubblar sina slag.
        const bool fillRoll = fill && li > 0 && lane.pad >= 4 && lane.pad <= 7;
        const bool fillDouble = fill && li > 0 && ! fillRoll;

        const auto first = (juce::int64) std::floor (ppqStart * spb) - 1;
        const auto last = (juce::int64) std::floor (ppqEnd * spb);
        for (auto n = juce::jmax ((juce::int64) 0, first); n <= last; ++n)
        {
            Step st = lane.steps[(size_t) (n % len)];
            if (fillRoll)
            {
                const double inBar = std::fmod ((double) n / spb, 4.0) / 4.0; // 0..1 genom takten
                st.on = true; st.prob = 100; st.ghost = false; st.accent = inBar > 0.7;
                st.ratchet = (uint8_t) (inBar > 0.75 ? 2 : 1);
            }
            else if (fillDouble && st.on)
                st.ratchet = (uint8_t) juce::jmax (2, (int) st.ratchet);
            if (! st.on) continue;
            double t0 = (double) n / spb;
            if (swingable && (n & 1)) t0 += sw * 0.5 / spb;
            const int rat = juce::jlimit (1, 4, (int) st.ratchet);
            for (int k = 0; k < rat; ++k)
            {
                const double tk = t0 + (double) k / (spb * rat);
                if (tk < ppqStart || tk >= ppqEnd) continue;
                if (st.prob < 100 && audioRng.nextInt (100) >= st.prob) continue;
                const int offset = juce::jlimit (0, numSamples - 1, (int) ((tk - ppqStart) * samplesPerBeat));
                float gain = stepGain (st) * (k == 0 ? 1.0f : 0.85f);
                if (fillRoll) gain *= 0.45f + 0.55f * (float) (std::fmod (tk, 4.0) / 4.0);
                startVoice (lane.pad, li, gain, offset, true, st.semi);
                laneTriggered[(size_t) li] = true;
                laneTrigAmount[(size_t) li] = juce::jmax (laneTrigAmount[(size_t) li], st.accent ? 1.0f : st.ghost ? 0.35f : 0.7f);
            }
        }
    }
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

    int done = 0;
    while (done < total)
    {
        const int n = juce::jmin (maxBlock, total - done);
        for (auto& b : laneBufs) b.clear (0, n);
        laneTriggered.fill (false);
        laneTrigAmount.fill (0.0f);

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

        // MIDI: C1 (36) och uppåt spelar pad 1-16
        for (const auto m : midi)
        {
            const auto msg = m.getMessage();
            if (m.samplePosition < done || m.samplePosition >= done + n || ! msg.isNoteOn()) continue;
            const int pad = msg.getNoteNumber() - 36;
            if (juce::isPositiveAndBelow (pad, kPads))
                startVoice (pad, laneForPad (pad), msg.getFloatVelocity(), m.samplePosition - done, false);
        }

        if (sequencing)
        {
            const double chunkPpq = ppq + (double) done / (sr * beatSec);
            runSequencer (n, chunkPpq, bpm);
        }

        renderVoices (n);

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
            cp.resonance = prm.res->load();
            cp.drive = prm.drive->load();
            cp.envAmount = prm.env->load();
            cp.envDecayMs = prm.dec->load();
            cp.crush = prm.crush->load();
            chains[(size_t) li].process (lb, n, cp);

            const float vol = prm.vol->load();
            const float pan = prm.pan->load();
            const float gl = vol * std::cos ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f) * 1.4142f;
            const float gr = vol * std::sin ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f) * 1.4142f;
            const float sa = prm.sendA->load(), sb = prm.sendB->load();
            const float* l = lb.getReadPointer (0);
            const float* r = lb.getReadPointer (1);
            auto* aL = sendA.getWritePointer (0); auto* aR = sendA.getWritePointer (1);
            auto* bM = sendB.getWritePointer (0);
            for (int i = 0; i < n; ++i)
            {
                const float L = l[i] * gl, R = r[i] * gr;
                outL[i] += L; outR[i] += R;
                aL[i] += L * sa; aR[i] += R * sa;
                bM[i] += 0.5f * (L + R) * sb;
            }
        }
        // direktbussen (pads som inte ligger på något spår)
        buffer.addFrom (0, done, laneBufs[kLanes], 0, 0, n);
        buffer.addFrom (1, done, laneBufs[kLanes], 1, 0, n);

        // dub-delay
        const int dIdx = juce::jlimit (0, 5, (int) pDTime->load());
        delay.process (sendA.getReadPointer (0), sendA.getReadPointer (1),
                       delayOut.getWritePointer (0), delayOut.getWritePointer (1), n,
                       (float) (kDelayBeats[dIdx] * beatSec), pDFb->load(), pDTone->load(), pDWob->load(),
                       pDFreeze->load() > 0.5f);
        // rumble
        rumble.process (sendB.getReadPointer (0), rumbleOut.getWritePointer (0), n,
                        pRSize->load(), pRDrive->load(), pRCut->load(), pRDuck->load(), beatSec);

        const float dRet = pDRet->load(), rRet = pRRet->load() * 0.7f; // rumblen ska ligga under kicken
        const auto* dL = delayOut.getReadPointer (0); const auto* dR = delayOut.getReadPointer (1);
        const auto* rm = rumbleOut.getReadPointer (0);

        // master: DJ-svep, grus, bas-mono, stutter, clipper
        const float sweepTarget = pSweep->load();
        float sweep = sweepSmoothed.load();
        const float mcrush = pMCrush->load();
        const float clip = pClip->load();
        const float outGain = juce::Decibels::decibelsToGain (pOut->load());
        // clipper: drive in, men nästan oförändrad nivå för svaga signaler
        const float clipG = 1.0f + clip * clip * 6.0f;
        const float clipNorm = 0.97f; // tak strax under 0 dBFS

        // -6 dB headroom på mixbussen innan mastern, så clippern får jobba med marginal
        for (int i = 0; i < n; ++i)
        {
            outL[i] = (outL[i] + dL[i] * dRet + rm[i] * rRet) * 0.5f;
            outR[i] = (outR[i] + dR[i] * dRet + rm[i] * rRet) * 0.5f;
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
    static const char* roleCats[kPads] = { "K", "K", "H", "H", "C", "C", "P", "P", "P", "P", "P", "P", "B", "S", "L", "X" };
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

void BetongProcessor::randomPattern()
{
    pushUndo();
    juce::Random rng;
    const float crazy = getParam ("crazy");
    Pattern p = getPattern();

    static const int lanePads[kLanes][3] = { { 0, 0, 0 }, { 2, 2, 1 }, { 3, 4, 5 }, { 4, 5, 10 },
                                             { 6, 7, 8 }, { 8, 9, 11 }, { 12, 12, 13 }, { 13, 14, 15 } };
    const int calmLens[] = { 16, 16, 16, 16, 8, 12 };
    const int wildLens[] = { 5, 7, 9, 11, 13, 15, 12, 10, 14, 6 };

    for (int li = 0; li < kLanes; ++li)
    {
        auto& lane = p.lanes[(size_t) li];
        if (lane.lock) continue;
        const bool keepMute = lane.mute;
        lane = Lane {};
        lane.mute = keepMute;
        lane.pad = lanePads[li][rng.nextInt (3)];

        if (li == 0) // kicken håller golvet
        {
            setStepsFromString (lane, "X...x...x...x...");
            if (rng.nextFloat() < crazy * 0.5f) lane.steps[(size_t) (10 + rng.nextInt (6))].on = true;
            if (rng.nextFloat() < crazy * 0.3f) { auto& s = lane.steps[15]; s.on = true; s.prob = 50; }
            continue;
        }

        lane.length = rng.nextFloat() < crazy * 0.8f ? wildLens[rng.nextInt (10)] : calmLens[rng.nextInt (6)];
        const float r = rng.nextFloat();
        lane.rate = r < crazy * 0.12f ? 2 : r < crazy * 0.2f ? 4 : r < crazy * 0.27f ? 5 : r < crazy * 0.32f ? 3 : 0;

        if (li == 1 && lane.pad == 2) // stängd hat på offbeats eller 16-delar
        {
            setStepsFromString (lane, rng.nextBool() ? "..x...x...x...x." : "xxxxxxxxxxxxxxxx");
            if (lane.length != 16) { lane.euclid = true; lane.hits = juce::jmax (2, lane.length / 2); }
        }
        else
        {
            lane.euclid = true;
            const float density = li == 6 ? 0.45f : li == 7 ? 0.15f : 0.2f + rng.nextFloat() * 0.35f;
            lane.hits = juce::jlimit (1, lane.length, juce::roundToInt (lane.length * density));
            lane.rotate = rng.nextInt (juce::jmax (1, lane.length));
        }
        if (lane.euclid) applyEuclid (lane);
        lane.euclid = false; // gör stegen redigerbara direkt

        for (int s = 0; s < lane.length; ++s)
        {
            auto& st = lane.steps[(size_t) s];
            if (! st.on) continue;
            st.accent = rng.nextFloat() < 0.2f;
            st.ghost = ! st.accent && rng.nextFloat() < 0.25f;
            // bas och stabs (pad 13-16) får en basgång i F-moll
            if (lane.pad >= 12)
            {
                static const int scale[] = { 0, 0, 0, 3, 5, 7, 10, 12, -2, -5 };
                if (rng.nextFloat() < 0.3f + crazy * 0.5f) st.semi = (int8_t) scale[rng.nextInt (10)];
            }
            if (rng.nextFloat() < crazy * 0.35f) st.prob = (uint8_t) (rng.nextBool() ? 50 : 25);
            if (rng.nextFloat() < crazy * 0.15f) st.ratchet = (uint8_t) (2 + rng.nextInt (3));
        }
    }
    setPattern (p);
    setParam ("swing", rng.nextFloat() * (0.15f + crazy * 0.35f));
    for (int li = 0; li < kLanes; ++li)
        if (! p.lanes[(size_t) li].lock)
            setParam (laneId (li, "swing"), (li > 0 && rng.nextFloat() < 0.15f + crazy * 0.3f) ? 0.2f + rng.nextFloat() * 0.4f : -0.05f);
    currentPatternName = juce::String::fromUTF8 ("Slump-mönster");
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
    apvts.replaceState (paramsOnly);

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

juce::AudioProcessorEditor* BetongProcessor::createEditor() { return new BetongEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BetongProcessor(); }
