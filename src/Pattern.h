#pragma once

#include <juce_core/juce_core.h>
#include <array>

namespace betong
{
constexpr int kLanes = 8;
constexpr int kMaxSteps = 32;
constexpr int kPads = 16;

// Takter per spår: steg per taktslag (fjärdedel). 5 och 7 ger kvintoler och septoler.
struct RateInfo { const char* name; int stepsPerBeat; };
constexpr RateInfo kRates[] = {
    { "1/16", 4 }, { "1/8", 2 }, { "1/16T", 6 }, { "1/32", 8 }, { "5-del", 5 }, { "7-del", 7 },
};
constexpr int kNumRates = (int) (sizeof (kRates) / sizeof (kRates[0]));

struct Step
{
    bool on = false;
    bool accent = false;
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
};

struct Pattern
{
    std::array<Lane, kLanes> lanes {};
};

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

// Text <-> steg. Tecken: . av, x på, X accent, o 50 %, p 25 %, 2/3/4 ratchet.
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
            else c = st.accent ? 'X' : 'x';
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
    juce::String probs;
    for (int i = 0; i < l.length; ++i) probs += juce::String ((int) l.steps[(size_t) i].prob) + ",";
    return juce::StringArray { juce::String (l.pad), juce::String (l.length), juce::String (l.rate),
                               l.euclid ? "1" : "0", juce::String (l.hits), juce::String (l.rotate),
                               l.mute ? "1" : "0", l.solo ? "1" : "0", l.lock ? "1" : "0",
                               stepsToString (l), acc, probs }.joinIntoString ("|");
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
    if (l.euclid) applyEuclid (l);
}
} // namespace betong
