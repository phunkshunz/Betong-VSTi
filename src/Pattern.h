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
    uint8_t cond = 0;      // trig-villkor (se condName), 0 = alltid
};

// Trig-villkor à la Elektron. "1:4" = spela första varvet av fyra, "3:4" = tredje varvet.
// FILL spelar bara när fill hålls inne, PRE om förra villkoret på spåret slog in.
enum CondKind { CondAlways = 0, CondFill = 12, CondNotFill = 13, CondPre = 14, CondNotPre = 15, kNumConds = 16 };

inline const char* condName (int c)
{
    static const char* n[] = { "Alltid", "1:2", "2:2", "1:3", "2:3", "3:3", "1:4", "2:4", "3:4", "4:4", "1:8", "8:8",
                               "FILL", "!FILL", "PRE", "!PRE" };
    return n[c < 0 || c >= kNumConds ? 0 : c];
}

// Villkor som inte beror på andra steg. iter = vilket varv av spåret (0, 1, 2 ...)
inline bool condTrue (int c, juce::int64 iter, bool fill)
{
    static const int a[] = { 0, 1, 2, 1, 2, 3, 1, 2, 3, 4, 1, 8 };
    static const int b[] = { 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 8, 8 };
    if (c <= 0 || c >= kNumConds) return true;
    if (c == CondFill) return fill;
    if (c == CondNotFill) return ! fill;
    if (c >= CondPre) return true; // räknas av sequencern
    const juce::int64 m = ((iter % b[c]) + b[c]) % b[c];
    return m == a[c] - 1;
}

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

    // Liv och samtal
    int life = 0;      // Game of Life på spåret: 0 av, 1 puls, 2 kaos, 3 liv (se lifeName)
    int callResp = 0;  // fråga och svar: 0 av, 1 fråga (spelar jämna block), 2 svar (udda block)
    int crBars = 2;    // blockets längd i takter
};

//==============================================================================
// Game of Life på ett spår: stegen är celler som föds och dör en gång per takt.
// Startraden är det du har ritat, och var 16:e takt börjar livet om från den.
enum LifeKind { LifeOff = 0, LifePuls, LifeKaos, LifeLiv, kNumLife };

inline const char* lifeName (int k)
{
    static const char* n[] = { "Av", "Puls (regel 90)", "Kaos (regel 30)", "Liv (föds 2-3, lever 2 eller 4)" };
    return n[k < 0 || k >= kNumLife ? 0 : k];
}

inline juce::uint32 lifeStep (juce::uint32 row, int len, int kind)
{
    const juce::uint32 mask = len >= 32 ? 0xFFFFFFFFu : ((1u << len) - 1u);
    auto at = [&] (int i) { i = ((i % len) + len) % len; return (row >> i) & 1u; };
    juce::uint32 out = 0;
    for (int i = 0; i < len; ++i)
    {
        const auto l = at (i - 1), c = at (i), r = at (i + 1);
        juce::uint32 v = 0;
        if (kind == LifePuls) v = l ^ r;
        else if (kind == LifeKaos) v = l ^ (c | r);
        else
        {
            const auto n = l + r + at (i - 2) + at (i + 2);
            v = c ? (n == 2 || n == 4) : (n == 2 || n == 3);
        }
        out |= v << i;
    }
    return out & mask;
}

// Spårets rad för takten bar. Dör livet ut, eller blir allt fullt, börjar det om från ritningen.
inline juce::uint32 lifeRow (juce::uint32 base, int len, int kind, juce::int64 bar)
{
    const int gen = (int) (((bar % 16) + 16) % 16);
    juce::uint32 row = base;
    const juce::uint32 mask = len >= 32 ? 0xFFFFFFFFu : ((1u << len) - 1u);
    for (int g = 0; g < gen; ++g)
    {
        row = lifeStep (row, len, kind);
        if (row == 0 || row == mask) row = base;
    }
    return row;
}

// Gör om spårets steg till generationen för takten bar. Nyfödda slag blir svaga.
inline void applyLife (Lane& L, juce::int64 bar);

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

inline void applyEuclid (Lane& lane);

inline void applyLife (Lane& L, juce::int64 bar)
{
    if (L.life <= LifeOff || L.life >= kNumLife) return;
    if (L.euclid) { applyEuclid (L); L.euclid = false; }
    const int len = juce::jlimit (1, kMaxSteps, L.length);
    juce::uint32 base = 0;
    int firstSemi = 0; bool found = false;
    for (int i = 0; i < len; ++i)
        if (L.steps[(size_t) i].on)
        {
            base |= 1u << i;
            if (! found) { firstSemi = L.steps[(size_t) i].semi; found = true; }
        }
    if (base == 0) return;
    const auto row = lifeRow (base, len, L.life, bar);
    for (int i = 0; i < len; ++i)
    {
        auto& s = L.steps[(size_t) i];
        const bool alive = (row >> i) & 1u;
        if (alive && ! s.on) { s = Step {}; s.on = true; s.ghost = true; s.semi = (int8_t) firstSemi; }
        else if (! alive) s.on = false;
    }
}

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
    juce::String probs, ghosts, semis, conds;
    for (int i = 0; i < l.length; ++i)
    {
        const auto& st = l.steps[(size_t) i];
        probs += juce::String ((int) st.prob) + ",";
        ghosts += st.ghost ? "1" : "0";
        semis += juce::String ((int) st.semi) + ",";
        conds += juce::String ((int) st.cond) + ",";
    }
    return juce::StringArray { juce::String (l.pad), juce::String (l.length), juce::String (l.rate),
                               l.euclid ? "1" : "0", juce::String (l.hits), juce::String (l.rotate),
                               l.mute ? "1" : "0", l.solo ? "1" : "0", l.lock ? "1" : "0",
                               stepsToString (l), acc, probs, ghosts, semis,
                               juce::StringArray { juce::String (l.rotEvery), juce::String (l.breathe), juce::String (l.breatheBars),
                                                   juce::String (l.accHits), juce::String (l.accLen),
                                                   juce::String (l.ratHits), juce::String (l.ratLen), juce::String (l.ratCount),
                                                   juce::String (l.toneHits), juce::String (l.toneLen), juce::String (l.toneSemi),
                                                   juce::String (l.fillOf) }.joinIntoString (","),
                               conds,
                               juce::StringArray { juce::String (l.life), juce::String (l.callResp), juce::String (l.crBars) }.joinIntoString (",") }.joinIntoString ("|");
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
    if (t.size() > 15)
    {
        auto c = juce::StringArray::fromTokens (t[15], ",", {});
        for (int i = 0; i < c.size() && i < kMaxSteps; ++i)
            if (c[i].isNotEmpty())
                l.steps[(size_t) i].cond = (uint8_t) juce::jlimit (0, kNumConds - 1, c[i].getIntValue());
    }
    if (t.size() > 16)
    {
        auto e = juce::StringArray::fromTokens (t[16], ",", {});
        auto at = [&] (int i, int def) { return i < e.size() && e[i].isNotEmpty() ? e[i].getIntValue() : def; };
        l.life = juce::jlimit (0, kNumLife - 1, at (0, 0));
        l.callResp = juce::jlimit (0, 2, at (1, 0));
        l.crBars = juce::jlimit (1, 16, at (2, 2));
    }
    if (l.euclid) applyEuclid (l);
}
} // namespace betong
