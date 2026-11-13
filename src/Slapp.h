#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include "Library.h"
#include "Pattern.h"

// BETONG 3: SLÄPP IN. En trumloop blir ett kit och ett mönster.
//
// Loopen delas i tre band (bas, mitt, diskant) och varje band får en egen anslagsdetektor, så att en kick
// och en hat som slår samtidigt båda hittas. Varje slag sorteras (kick, clap/virvel, stängd och öppen hat,
// två sorters perc), det renaste slaget i varje grupp blir padens ljud, och var slagen landar blir mönstret.
// Hur sent hats och perc ligger mot rutnätet blir FRIKTIONENS släp och spårens swing: loopens groove följer med.
//
// Körs på GUI-tråden (en gång när man släpper in en loop), aldrig i ljudtråden.
namespace betong::slapp
{
enum Kind { Kick = 0, Clap, Hat, OpenHat, PercLow, PercHigh, kKinds };

inline const char* kindName (int k)
{
    static const char* n[] = { "KICK", "CLAP", "HAT", "ÖPPEN HAT", "PERC", "PERC 2" };
    return n[juce::jlimit (0, kKinds - 1, k)];
}

// var varje grupp hamnar: pad (rollerna i Motor.h) och spår
constexpr int kPadFor[kKinds]  = { 0, 4, 2, 3, 6, 7 };
constexpr int kLaneFor[kKinds] = { 0, 1, 2, 3, 4, 5 };
constexpr int kVariantPad[kKinds] = { 1, 5, -1, -1, 8, 9 };   // näst bästa slaget i gruppen
constexpr int kLoopPad = 15, kLoopLane = 7;

struct Hit
{
    int sample = 0;          // var slaget börjar i loopen
    float strength = 0.0f;   // dB
    float clean = 0.0f;      // hur rent slaget är (eget band mot de andra), dB
    int step = 0;            // närmaste steg i loopens rutnät
    float offsetSteps = 0.0f;// hur långt från rutnätet (+ = sent), i steg
};

struct Result
{
    int bars = 1;
    double stepSamples = 0.0;
    int laneLength = 16, laneRate = 0;      // spårets längd och takt (index i kRates)
    std::array<std::vector<Hit>, kKinds> hits;
    std::array<int, kKinds> best { -1, -1, -1, -1, -1, -1 }, second { -1, -1, -1, -1, -1, -1 };
    std::array<int, kKinds> sliceLen {};
    float pocketMs = 0.0f;                   // FRIKTION: släp (bara > 0 om loopen ligger efter)
    std::array<float, kKinds> dragFactor { 1, 1, 1, 1, 1, 1 };
    std::array<float, kKinds> swing { -0.05f, -0.05f, -0.05f, -0.05f, -0.05f, -0.05f };
    bool ok = false;
};

//==============================================================================
namespace detail
{
struct BandOnset { int sample; float peakDb; };

// energi i dB per ram (2 ms hopp). Fönstret är längre för basbandet, annars ser varje bassvängning ut som ett slag.
inline std::vector<float> frameDb (const std::vector<float>& x, int hop, int winHops)
{
    const int frames = (int) x.size() / hop;
    std::vector<float> e ((size_t) juce::jmax (0, frames), -120.0f);
    for (int f = 0; f < frames; ++f)
    {
        double s = 0.0;
        const int a = f * hop, b = juce::jmin ((int) x.size(), a + winHops * hop);
        for (int i = a; i < b; ++i) s += (double) x[(size_t) i] * x[(size_t) i];
        e[(size_t) f] = (float) (10.0 * std::log10 (1e-12 + s / juce::jmax (1, b - a)));
    }
    return e;
}

inline std::vector<BandOnset> onsets (const std::vector<float>& x, int hop, float riseDb, float rangeDb, int minGapFrames, int winHops = 2, float startFrac = 0.3f)
{
    const auto e = frameDb (x, hop, winHops);
    std::vector<BandOnset> out;
    if (e.size() < 6) return out;
    const float top = *std::max_element (e.begin(), e.end());
    int last = -minGapFrames;
    // jämför med fönster som helt ligger före det här (annars har de redan fått med anslaget)
    const int back = winHops + 2;
    for (int f = back; f + 6 + winHops < (int) e.size(); ++f)
    {
        float before = 1e9f;
        for (int k = winHops; k <= back; ++k) before = juce::jmin (before, e[(size_t) f - k]);
        if (f - last < minGapFrames) continue;
        // ökningen kan ta ett par fönster (en kick som byggs upp ovanpå förra kickens svans)
        float ahead = e[(size_t) f];
        for (int k = 1; k <= winHops; ++k) ahead = juce::jmax (ahead, e[(size_t) f + k]);
        if (ahead - before < riseDb || ahead < top - rangeDb) continue;
        if (e[(size_t) f] - before < riseDb * 0.35f) continue;      // vänta tills ökningen verkligen börjat
        float pk = ahead;
        for (int k = 1; k <= 6 + winHops; ++k) pk = juce::jmax (pk, e[(size_t) f + k]);
        // exakt start: halvvägs mellan nivån före (svansen) och den nya toppen
        const int a = juce::jmax (0, (f - 2) * hop), b = juce::jmin ((int) x.size(), (f + 2 * winHops + 6) * hop);
        float amp = 0.0f, base = 0.0f;
        for (int i = a; i < b; ++i) amp = juce::jmax (amp, std::abs (x[(size_t) i]));
        for (int i = juce::jmax (0, a - 2 * winHops * hop); i < a; ++i) base = juce::jmax (base, std::abs (x[(size_t) i]));
        base = juce::jmin (base, 0.8f * amp);
        int start = f * hop;
        for (int i = a; i < b; ++i) if (std::abs (x[(size_t) i]) >= base + startFrac * (amp - base)) { start = juce::jmax (0, i - hop / 4); break; }
        out.push_back ({ start, pk });
        last = f;
    }
    return out;
}

// toppnivå (dB) i ett fönster
inline float peakDb (const std::vector<float>& x, int a, int len)
{
    float m = 1e-9f;
    const int b = juce::jmin ((int) x.size(), a + len);
    for (int i = juce::jmax (0, a); i < b; ++i) m = juce::jmax (m, std::abs (x[(size_t) i]));
    return 20.0f * std::log10 (m);
}

inline float rmsDb (const std::vector<float>& x, int a, int len)
{
    double s = 0.0; int n = 0;
    const int b = juce::jmin ((int) x.size(), a + len);
    for (int i = juce::jmax (0, a); i < b; ++i) { s += (double) x[(size_t) i] * x[(size_t) i]; ++n; }
    return (float) (10.0 * std::log10 (1e-12 + s / juce::jmax (1, n)));
}

// hur länge bandet klingar: tid tills det fallit 20 dB från toppen (sampel)
inline int ringLength (const std::vector<float>& x, int start, int maxLen, int hop)
{
    const float pk = peakDb (x, start, hop * 6);
    for (int a = start + hop * 3; a < juce::jmin ((int) x.size(), start + maxLen); a += hop)
        if (rmsDb (x, a, hop) < pk - 23.0f) return a - start;
    return maxLen;
}

// nollgenomgångar per sekund: brus (clap, virvel) har många, en trumma med ton har få
inline float zcrRate (const std::vector<float>& x, int a, int len, double sr)
{
    int z = 0, n = 0;
    const int b = juce::jmin ((int) x.size(), a + len);
    for (int i = juce::jmax (1, a); i < b; ++i, ++n) z += (x[(size_t) i - 1] < 0.0f) != (x[(size_t) i] < 0.0f) ? 1 : 0;
    return n > 0 ? (float) (z * sr / n) : 0.0f;
}

inline std::vector<float> filtered (const std::vector<float>& x, const juce::IIRCoefficients& c1, const juce::IIRCoefficients& c2)
{
    juce::IIRFilter f1, f2;
    f1.setCoefficients (c1); f2.setCoefficients (c2);
    std::vector<float> y (x);
    f1.processSamples (y.data(), (int) y.size());
    f2.processSamples (y.data(), (int) y.size());
    return y;
}

inline float median (std::vector<float> v)
{
    if (v.empty()) return 0.0f;
    std::nth_element (v.begin(), v.begin() + (long) v.size() / 2, v.end());
    return v[v.size() / 2];
}
} // namespace detail

//==============================================================================
// hostBpm används bara för att gissa hur många takter loopen är (om filen inte passar tempot gissas
// det tempo mellan 90 och 180 BPM som ger jämna takter).
inline Result analyse (const Sample& s, double hostBpm)
{
    using namespace detail;
    Result r;
    const int len = s.data.getNumSamples();
    const double sr = s.sampleRate;
    if (len < (int) (sr * 0.4)) return r;

    std::vector<float> mono ((size_t) len, 0.0f);
    for (int ch = 0; ch < s.data.getNumChannels(); ++ch)
    {
        const float* d = s.data.getReadPointer (ch);
        for (int i = 0; i < len; ++i) mono[(size_t) i] += d[i] / (float) s.data.getNumChannels();
    }

    // ---- hur många takter: först värdens tempo, annars det jämna tempo som ligger närmast 128
    const double secs = len / sr;
    int bars = 0;
    {
        const double beats = secs * hostBpm / 60.0;
        for (int b : { 1, 2, 4, 8 })
            if (std::abs (beats - b * 4.0) / (b * 4.0) < 0.04) { bars = b; break; }
        if (bars == 0)
        {
            double bestErr = 1e9;
            for (int b : { 1, 2, 4, 8 })
            {
                const double bpm = b * 4.0 * 60.0 / secs;
                if (bpm < 85.0 || bpm > 185.0) continue;
                const double err = std::abs (bpm - 128.0);
                if (err < bestErr) { bestErr = err; bars = b; }
            }
        }
        if (bars == 0) bars = juce::jlimit (1, 8, (int) std::lround (secs * 128.0 / 240.0));
    }
    r.bars = bars;
    r.stepSamples = (double) len / (bars * 16.0);
    // spåret: 16-delar, upp till två takter (längre loopar: de två första takterna, så hatsens 16-delar behålls)
    r.laneLength = juce::jmin (32, bars * 16);
    r.laneRate = 0;

    // ---- tre band
    const auto lowC  = juce::IIRCoefficients::makeLowPass (sr, 140.0, 0.7);
    const auto midHp = juce::IIRCoefficients::makeHighPass (sr, 250.0, 0.7);
    const auto midLp = juce::IIRCoefficients::makeLowPass (sr, 4500.0, 0.7);
    const auto hiHp  = juce::IIRCoefficients::makeHighPass (sr, 6500.0, 0.7);
    const auto low  = filtered (mono, lowC, lowC);
    const auto mid  = filtered (mono, midHp, midLp);
    const auto high = filtered (mono, hiHp, hiHp);
    const auto bodyHp = juce::IIRCoefficients::makeHighPass (sr, 180.0, 0.7), bodyLp = juce::IIRCoefficients::makeLowPass (sr, 700.0, 0.7);
    const auto body = filtered (mono, bodyHp, bodyLp);          // trumskinn
    const auto noiseHp = juce::IIRCoefficients::makeHighPass (sr, 2000.0, 0.7), noiseLp = juce::IIRCoefficients::makeLowPass (sr, 7000.0, 0.7);
    const auto noise = filtered (mono, noiseHp, noiseLp);       // smatter

    const int hop = juce::jmax (32, (int) (sr * 0.002));
    const int w30 = (int) (sr * 0.03);
    auto lowOn  = onsets (low,  hop, 6.0f, 15.0f, (int) (0.09 * sr / hop), 8, 0.08f);
    // en kick är basdominerad: congas och toms i en topploop har mer mitten än bas
    lowOn.erase (std::remove_if (lowOn.begin(), lowOn.end(), [&] (const BandOnset& o)
                 { return peakDb (low, o.sample, (int) (sr * 0.04)) < peakDb (mid, o.sample, (int) (sr * 0.04)) - 2.0f; }), lowOn.end());
    {   // kickarna är loopens starkaste basslag: svagare basslag (toms, rumble, bas) räknas inte
        float topLow = -200.0f;
        for (auto& o : lowOn) topLow = juce::jmax (topLow, o.peakDb);
        lowOn.erase (std::remove_if (lowOn.begin(), lowOn.end(), [&] (const BandOnset& o) { return o.peakDb < topLow - 5.0f; }), lowOn.end());
    }   // kicken byggs upp långsamt i basbandet
    const auto midOn  = onsets (mid,  hop, 8.0f, 32.0f, (int) (0.05 * sr / hop));
    const auto highOn = onsets (high, hop, 7.0f, 34.0f, (int) (0.035 * sr / hop));

    auto near = [&] (const std::vector<BandOnset>& v, int smp, int tol)
    {
        for (const auto& o : v) if (std::abs (o.sample - smp) <= tol) return true;
        return false;
    };
    const int tol = (int) (sr * 0.012);
    auto place = [&] (int smp, Hit& h)
    {
        const double pos = smp / r.stepSamples;
        h.step = (int) std::lround (pos);
        h.offsetSteps = (float) (pos - h.step);
        h.step %= bars * 16;
    };

    // KICK: basbandet. Exakt start ur hela ljudet (basbandet är fördröjt av filtret och kicken byggs upp långsamt).
    for (const auto& o0 : lowOn)
    {
        auto o = o0;
        {
            const int a = juce::jmax (0, o.sample - (int) (sr * 0.015)), b = juce::jmin (len, o.sample + (int) (sr * 0.01));
            float amp = 0.0f, base = 0.0f;
            for (int i = a; i < b; ++i) amp = juce::jmax (amp, std::abs (mono[(size_t) i]));
            for (int i = juce::jmax (0, a - (int) (sr * 0.02)); i < a; ++i) base = juce::jmax (base, std::abs (mono[(size_t) i]));
            base = juce::jmin (base, 0.8f * amp);
            for (int i = a; i < b; ++i) if (std::abs (mono[(size_t) i]) >= base + 0.3f * (amp - base)) { o.sample = i; break; }
        }
        Hit h; h.sample = o.sample; h.strength = o.peakDb;
        h.clean = o.peakDb - juce::jmax (peakDb (high, o.sample, w30), peakDb (mid, o.sample, w30) - 6.0f);
        place (o.sample, h);
        r.hits[Kick].push_back (h);
    }
    // MITT: clap/virvel eller perc. Mittenslag som bara är kickens klick hoppas över.
    struct MidCand { Hit h; float noisy; bool backbeat; };
    std::vector<MidCand> mids;
    for (const auto& o : midOn)
    {
        const float lowLvl = peakDb (low, o.sample, w30);
        // brusighet i dB mot 1,5 kHz nollgenomgångar (clap ~ +2..+6, kickens klick och congas ~ -6..-12)
        const float noisy = 10.0f * std::log10 (1.0f + zcrRate (mid, o.sample, (int) (sr * 0.04), sr)) - 10.0f * std::log10 (1500.0f);
        // kickens klick: samtidigt som en kick, inte brusigt och mycket svagare än kickens bas
        if (near (lowOn, o.sample, tol) && noisy < -2.5f && o.peakDb < lowLvl + 2.0f) continue;
        // en hats nedre del: diskanten är starkare än mitten
        if (peakDb (high, o.sample, w30) > o.peakDb - 3.0f) continue;
        Hit h; h.sample = o.sample; h.strength = o.peakDb;
        h.clean = o.peakDb - juce::jmax (lowLvl - 3.0f, peakDb (high, o.sample, w30));
        place (o.sample, h);
        const int s16 = h.step % 16;
        mids.push_back ({ h, noisy, s16 == 4 || s16 == 12 });
    }
    {
        // svaga mittenslag (smuts, eko, läckage från hats) räknas inte
        float midTop = -200.0f;
        for (auto& m : mids) midTop = juce::jmax (midTop, m.h.strength);
        mids.erase (std::remove_if (mids.begin(), mids.end(), [&] (const MidCand& m) { return m.h.strength < midTop - 16.0f; }), mids.end());
        // clap/virvel: brusigt eller på 2 och 4 (och inte helt tonalt). Resten är perc.
        std::vector<float> percBright;
        for (auto& m : mids)
        {
            const bool clap = m.noisy > 0.0f || (m.backbeat && m.noisy > -1.5f);
            if (clap) r.hits[Clap].push_back (m.h);
            else percBright.push_back (m.noisy);
        }
        const float split = median (percBright);
        for (auto& m : mids)
        {
            const bool clap = m.noisy > 0.0f || (m.backbeat && m.noisy > -1.5f);
            if (! clap) r.hits[m.noisy <= split ? PercLow : PercHigh].push_back (m.h);
        }
        // få perc-slag: lägg allt i en grupp
        if (r.hits[PercHigh].size() < 2)
        {
            for (auto& h : r.hits[PercHigh]) r.hits[PercLow].push_back (h);
            r.hits[PercHigh].clear();
        }
    }
    // DISKANT: hats. Diskantslag mitt i en clap räknas bara om de är starkare än clappens smatter.
    for (const auto& o : highOn)
    {
        const float midLvl = peakDb (mid, o.sample, w30);
        bool inClap = false;                       // en clap har flera smällar på 30 ms: hela den räknas
        for (const auto& c : r.hits[Clap]) if (o.sample - c.sample >= -tol && o.sample - c.sample <= (int) (sr * 0.04)) { inClap = true; break; }
        if (inClap && o.peakDb < midLvl + 3.0f) continue;
        if (near (lowOn, o.sample, tol) && o.peakDb < peakDb (low, o.sample, w30) - 14.0f) continue;   // kickens klick
        Hit h; h.sample = o.sample; h.strength = o.peakDb;
        h.clean = o.peakDb - juce::jmax (peakDb (low, o.sample, w30) - 12.0f, midLvl - 4.0f);
        place (o.sample, h);
        int until = (int) (sr * 0.5);                                   // mät bara fram till nästa hat
        for (const auto& n : highOn) if (n.sample > o.sample + hop * 4) { until = juce::jmin (until, n.sample - o.sample); break; }
        const int ring = ringLength (high, o.sample, until, hop);
        r.hits[ring > (int) (sr * 0.15) ? OpenHat : Hat].push_back (h);
    }
    // bara öppna hats och inga stängda: de korta av dem är stängda
    if (r.hits[Hat].empty() && r.hits[OpenHat].size() > 3) std::swap (r.hits[Hat], r.hits[OpenHat]);

    // ---- bästa slaget per grupp (renast, inte svagast) och hur långt det får klinga
    const float maxLenSec[kKinds] = { 0.42f, 0.34f, 0.12f, 0.45f, 0.30f, 0.26f };
    for (int k = 0; k < kKinds; ++k)
    {
        auto& v = r.hits[(size_t) k];
        if (v.empty()) continue;
        float top = -200.0f;
        for (auto& h : v) top = juce::jmax (top, h.strength);
        int b1 = -1, b2 = -1; float s1 = -1e9f, s2 = -1e9f;
        for (int i = 0; i < (int) v.size(); ++i)
        {
            const float score = v[(size_t) i].clean + 0.5f * (v[(size_t) i].strength - top);
            if (score > s1) { s2 = s1; b2 = b1; s1 = score; b1 = i; }
            else if (score > s2) { s2 = score; b2 = i; }
        }
        r.best[(size_t) k] = b1; r.second[(size_t) k] = b2;
        r.sliceLen[(size_t) k] = (int) (sr * maxLenSec[k]);
    }

    // ---- grooven: hur sent ligger det mot rutnätet? (kicken är ankaret)
    {
        std::vector<float> kickOff;
        for (auto& h : r.hits[Kick]) kickOff.push_back (h.offsetSteps);
        const float anchor = median (kickOff);          // loopen kan börja lite före eller efter rutnätet
        for (auto& v : r.hits) for (auto& h : v) h.offsetSteps -= anchor;
        const double stepMs = r.stepSamples / sr * 1000.0;
        std::array<float, kKinds> lateMs {};
        std::vector<float> all;
        for (int k = Clap; k < kKinds; ++k)
        {
            std::vector<float> even, odd;
            for (auto& h : r.hits[(size_t) k]) (h.step % 2 ? odd : even).push_back (h.offsetSteps);
            const float e = median (even), o = median (odd);
            lateMs[(size_t) k] = (float) (median (even.empty() ? odd : even) * stepMs);
            if (! even.empty()) all.push_back (lateMs[(size_t) k]);
            // swing: udda 16-delar senare än de jämna (BETONGS swing skjuter udda steg sw/2 steg)
            if (even.size() >= 2 && odd.size() >= 2 && o - e > 0.04f)
                r.swing[(size_t) k] = juce::jlimit (0.0f, 0.75f, 2.0f * (o - e));
        }
        const float pocket = median (all);
        if (pocket > 2.5f)
        {
            r.pocketMs = juce::jlimit (0.0f, 45.0f, pocket);
            const float roleF[kKinds] = { 1.0f, 0.8f, 1.0f, 1.0f, 1.2f, 1.2f };
            for (int k = Clap; k < kKinds; ++k)
                r.dragFactor[(size_t) k] = juce::jlimit (0.0f, 2.0f, lateMs[(size_t) k] / (r.pocketMs * roleF[k]));
        }
    }
    int found = 0;
    for (auto& v : r.hits) found += v.empty() ? 0 : 1;
    r.ok = found > 0;
    return r;
}

// mönstret för en grupp: slagen i loopens rutnät, starka = accent, svaga = ghost
inline Lane laneFor (const Result& r, int k, const Lane& base)
{
    Lane L = base;
    L.pad = kPadFor[k];
    L.length = r.laneLength;
    L.rate = r.laneRate;
    L.euclid = false;
    L.life = 0; L.callResp = 0; L.fillOf = -1; L.rotEvery = 0; L.breathe = 0;
    L.accHits = 0; L.ratHits = 0; L.toneHits = 0;
    L.mute = false; L.solo = false;
    for (auto& s : L.steps) s = Step();
    const auto& v = r.hits[(size_t) k];
    if (v.empty()) return L;
    float top = -200.0f;
    for (auto& h : v) top = juce::jmax (top, h.strength);
    for (auto& h : v)
    {
        const int st = h.step;
        if (st >= L.length) continue;
        auto& s = L.steps[(size_t) st];
        const bool wasOn = s.on;
        s.on = true;
        const float rel = h.strength - top;
        if (! wasOn || rel > -2.5f) { s.accent = rel > -2.5f; s.ghost = rel < -10.0f; }
    }
    return L;
}
} // namespace betong::slapp
