#pragma once

#include <juce_core/juce_core.h>
#include <array>

namespace betong
{
constexpr int kLanes = 8;
constexpr int kMaxSteps = 32;
constexpr int kPads = 16;
constexpr int kScenes = 4;

// Takter per spår: steg per taktslag (fjärdedel). 5 och 7 ger kvintoler och septoler.
struct RateInfo { const char* name; int stepsPerBeat; };
constexpr RateInfo kRates[] = {
    { "1/16", 4 }, { "1/8", 2 }, { "1/16T", 6 }, { "1/32", 8 }, { "5-del", 5 }, { "7-del", 7 },
};
constexpr int kNumRates = (int) (sizeof (kRates) / sizeof (kRates[0]));

struct Step
{
    bool on = false;
    bool accent = false;   // stark
    bool ghost = false;    // svag (ghost-slag)
    int8_t semi = 0;       // ton per steg i halvtoner (-24..+24)
    uint8_t prob = 100;    // sannolikhet i procent
    uint8_t ratchet = 1;   // 1..4 upprepningar inom steget
};

struct Lane
{
    int pad = 0;            // vilken pad (0..15) spåret spelar
    int length = 16;        // 1..32
    int rate = 0;           // index i kRates
    bool euclid = false;    // om stegen räknas fram euklidiskt
    int hits = 4;
    int rotate = 0;
    bool mute = false, solo = false, lock = false;
    std::array<Step, kMaxSteps> steps {};

    // Euklid-magi (räknas fram medan det spelar, mönstret i sig ändras inte)
    int rotEvery = 0;                        // flytta rotationen ett steg var N:e takt (0 = av)
    int breathe = 0, breatheBars = 16;       // andning: antal slag sveper ±breathe över breatheBars takter
    int accHits = 0, accLen = 8;             // accentlager: accentHits av accLen
    int ratHits = 0, ratLen = 8, ratCount = 2; // ratchetlager
    int toneHits = 0, toneLen = 5, toneSemi = 12; // tonlager: tonhopp på toneHits av toneLen
    int fillOf = -1;                         // spela bara där spår fillOf är tyst (-1 = av)
};

// Euklidiskt slag på plats idx (0..len-1) med hits slag. Samma formel som applyEuclid.
inline bool euclidOn (int idx, int hits, int len)
{
    if (len <= 0 || hits <= 0) return false;
    idx = ((idx % len) + len) % len;
    return ((idx * juce::jmin (hits, len)) % len) < juce::jmin (hits, len);
}

struct Pattern
{
    std::array<Lane, kLanes> lanes {};
};

// Styrka per steg: svag, normal, stark
inline float stepGain (const Step& s) { return s.accent ? 1.0f : s.ghost ? 0.42f : 0.78f; }

// Björklunds algoritm (Bresenham-varianten): sprider hits jämnt över len steg.
inline void applyEuclid (Lane& lane)
{
    const int len = juce::jlimit (1, kMaxSteps, lane.length);
    const int hits = juce::jlimit (0, len, lane.hits);
    for (int i = 0; i < kMaxSteps; ++i)
    {
        auto& s = lane.steps[(size_t) i];
        if (i >= len) { s.on = false; continue; }
        const int idx = ((i - lane.rotate) % len + len) % len;
        s.on = hits > 0 && ((idx * hits) % len) < hits;
    }
}

// Text <-> steg. Tecken: . av, x på, X accent, g svag, o 50 %, p 25 %, 2/3/4 ratchet.
inline void setStepsFromString (Lane& lane, const juce::String& s)
{
    lane.length = juce::jlimit (1, kMaxSteps, s.length());
    for (int i = 0; i < kMaxSteps; ++i)
    {
        Step st;
        if (i < s.length())
        {
            const auto c = s[i];
            st.on = c != '.';
            st.accent = c == 'X';
            st.ghost = c == 'g';
            st.prob = c == 'o' ? 50 : c == 'p' ? 25 : 100;
            st.ratchet = (c >= '2' && c <= '4') ? (uint8_t) (c - '0') : 1;
        }
        lane.steps[(size_t) i] = st;
    }
}

inline juce::String stepsToString (const Lane& lane)
{
    juce::String s;
    for (int i = 0; i < lane.length; ++i)
    {
        const auto& st = lane.steps[(size_t) i];
        juce::juce_wchar c = '.';
        if (st.on)
        {
            if (st.ratchet > 1) c = (juce::juce_wchar) ('0' + st.ratchet);
            else if (st.prob <= 25) c = 'p';
            else if (st.prob < 100) c = 'o';
            else c = st.accent ? 'X' : st.ghost ? 'g' : 'x';
        }
        s += juce::String::charToString (c);
    }
    return s;
}

// Komplett spår-data som text (för att spara projektet). Accent på ratchet/sannolikhet sparas separat.
inline juce::String laneToString (const Lane& l)
{
    juce::String acc;
    for (int i = 0; i < l.length; ++i) acc += l.steps[(size_t) i].accent ? "1" : "0";
    juce::String probs, ghosts, semis;
    for (int i = 0; i < l.length; ++i)
    {
        const auto& st = l.steps[(size_t) i];
        probs += juce::String ((int) st.prob) + ",";
        ghosts += st.ghost ? "1" : "0";
        semis += juce::String ((int) st.semi) + ",";
    }
    return juce::StringArray { juce::String (l.pad), juce::String (l.length), juce::String (l.rate),
                               l.euclid ? "1" : "0", juce::String (l.hits), juce::String (l.rotate),
                               l.mute ? "1" : "0", l.solo ? "1" : "0", l.lock ? "1" : "0",
                               stepsToString (l), acc, probs, ghosts, semis,
                               juce::StringArray { juce::String (l.rotEvery), juce::String (l.breathe), juce::String (l.breatheBars),
                                                   juce::String (l.accHits), juce::String (l.accLen),
                                                   juce::String (l.ratHits), juce::String (l.ratLen), juce::String (l.ratCount),
                                                   juce::String (l.toneHits), juce::String (l.toneLen), juce::String (l.toneSemi),
                                                   juce::String (l.fillOf) }.joinIntoString (",") }.joinIntoString ("|");
}

inline void laneFromString (Lane& l, const juce::String& s)
{
    auto t = juce::StringArray::fromTokens (s, "|", {});
    if (t.size() < 10) return;
    l.pad = juce::jlimit (0, kPads - 1, t[0].getIntValue());
    l.rate = juce::jlimit (0, kNumRates - 1, t[2].getIntValue());
    l.euclid = t[3] == "1";
    l.hits = t[4].getIntValue();
    l.rotate = t[5].getIntValue();
    l.mute = t[6] == "1"; l.solo = t[7] == "1"; l.lock = t[8] == "1";
    setStepsFromString (l, t[9]);
    l.length = juce::jlimit (1, kMaxSteps, t[1].getIntValue());
    if (t.size() > 10)
        for (int i = 0; i < t[10].length() && i < kMaxSteps; ++i)
            l.steps[(size_t) i].accent = t[10][i] == '1';
    if (t.size() > 11)
    {
        auto probs = juce::StringArray::fromTokens (t[11], ",", {});
        for (int i = 0; i < probs.size() && i < kMaxSteps; ++i)
            if (probs[i].isNotEmpty())
                l.steps[(size_t) i].prob = (uint8_t) juce::jlimit (0, 100, probs[i].getIntValue());
    }
    if (t.size() > 12)
        for (int i = 0; i < t[12].length() && i < kMaxSteps; ++i)
            l.steps[(size_t) i].ghost = t[12][i] == '1' && ! l.steps[(size_t) i].accent;
    if (t.size() > 13)
    {
        auto semis = juce::StringArray::fromTokens (t[13], ",", {});
        for (int i = 0; i < semis.size() && i < kMaxSteps; ++i)
            if (semis[i].isNotEmpty())
                l.steps[(size_t) i].semi = (int8_t) juce::jlimit (-24, 24, semis[i].getIntValue());
    }
    if (t.size() > 14)
    {
        auto e = juce::StringArray::fromTokens (t[14], ",", {});
        auto at = [&] (int i, int def) { return i < e.size() && e[i].isNotEmpty() ? e[i].getIntValue() : def; };
        l.rotEvery = juce::jlimit (0, 16, at (0, 0));
        l.breathe = juce::jlimit (0, kMaxSteps, at (1, 0)); l.breatheBars = juce::jlimit (1, 64, at (2, 16));
        l.accHits = juce::jlimit (0, kMaxSteps, at (3, 0)); l.accLen = juce::jlimit (1, kMaxSteps, at (4, 8));
        l.ratHits = juce::jlimit (0, kMaxSteps, at (5, 0)); l.ratLen = juce::jlimit (1, kMaxSteps, at (6, 8)); l.ratCount = juce::jlimit (2, 4, at (7, 2));
        l.toneHits = juce::jlimit (0, kMaxSteps, at (8, 0)); l.toneLen = juce::jlimit (1, kMaxSteps, at (9, 5)); l.toneSemi = juce::jlimit (-24, 24, at (10, 12));
        l.fillOf = juce::jlimit (-1, kLanes - 1, at (11, -1));
    }
    if (l.euclid) applyEuclid (l);
}
} // namespace betong
