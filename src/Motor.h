#pragma once

#include "Pattern.h"
#include <cmath>
#include <cstdint>

// MOTOR: gör att beatet lever och utvecklas av sig självt.
//
// Allt här är en ren funktion av (inställningar, seed, position i takter). Samma
// seed och samma plats i låten ger alltid samma resultat, så en loop i DAW:en
// låter likadant varje varv och en export blir som det du hörde. Mönstret som
// användaren har ritat ändras aldrig: motorn räknar fram en levande kopia.
//
// Delar:
//  - Energikurva och låtform (intro, bygg, topp, break, lyft, topp 2, utrull)
//  - Spår som kommer in och går ut med energin, mute-dans och kick ut
//  - Små mutationer per fras som växer under en cykel och sedan återkommer
//    (minne: ibland kommer ett gammalt motiv tillbaka, lite förändrat)
//  - Andningsfilter, resonanstoppar, dub-kast, rullningar inför frasgränser
//  - Pump och drive som följer energin, velocity-vågor och mikrotiming
namespace betong::motor
{
//==============================================================================
// Former. 0 = bara liv (ingen form, allt spelar), 1 = fri (ändlösa vågor),
// 2-4 = låtform som går runt efter 64, 128 eller 256 takter.
enum Form { FormLiv = 0, FormFri, FormKort, FormLat, FormAfterhour, kNumForms };

inline const char* formName (int f)
{
    static const char* n[] = { "Bara liv", "Fri", "Kort 64", "Låt 128", "Afterhour 256" };
    return n[f < 0 || f >= kNumForms ? 0 : f];
}

inline int formBars (int f) { return f == FormKort ? 64 : f == FormLat ? 128 : f == FormAfterhour ? 256 : 0; }

enum Section { SecLiv = 0, SecIntro, SecBygg, SecTopp, SecBreak, SecLyft, SecTopp2, SecUtrull, SecVag, kNumSections };

inline const char* sectionName (int s)
{
    static const char* n[] = { "LIV", "INTRO", "BYGG", "TOPP", "BREAK", "LYFT", "TOPP 2", "UTRULL", "VÅG" };
    return n[s < 0 || s >= kNumSections ? 0 : s];
}

// Ljudets roll, gissad från paden. Kiten följer samma ordning:
// 1-2 kick, 3 stängd hat, 4 öppen hat, 5-6 clap/snare, 7-12 perc, 13 bas, 14 stab, 15 lead, 16 fx.
enum Role { Kick = 0, Hat, OpenHat, Clap, Perc, Bass, Stab, Lead, Fx };

inline Role roleOf (int pad)
{
    if (pad <= 1) return Kick;
    if (pad == 2) return Hat;
    if (pad == 3) return OpenHat;
    if (pad <= 5) return Clap;
    if (pad <= 11) return Perc;
    if (pad == 12) return Bass;
    if (pad == 13) return Stab;
    if (pad == 14) return Lead;
    return Fx;
}

inline bool isTonal (Role r) { return r == Bass || r == Stab || r == Lead; }

//==============================================================================
struct Settings
{
    bool on = false;
    int form = FormLat;
    uint32_t seed = 1;
    float move = 0.4f;   // RÖRELSE: hur ofta saker ändras
    float depth = 0.4f;  // DJUP: hur stora ändringarna blir
    float chaos = 0.2f;  // KAOS: drar i allt på en gång
    float filt = 0.5f;   // FILTER: andningsfilter, stängning inför skiften, resonanstoppar
    float dub = 0.4f;    // DUB: dub-kast på sista slaget i en fras
    float pump = 0.3f;   // PUMP: sidechain och drive som växer med energin
    float feel = 0.35f;  // SVÄNG: velocity-vågor, mikrotiming och swing som driver

    static float clamp01 (float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }
    float moveEff() const { return clamp01 (move + chaos * 0.45f); }
    float depthEff() const { return clamp01 (depth + chaos * 0.5f); }
};

//==============================================================================
// Deterministisk slump
inline uint32_t mix (uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return x;
}

inline uint32_t hashOf (uint32_t seed, int64_t a, int b = 0, int c = 0)
{
    const auto ua = (uint64_t) a;
    uint32_t x = mix (seed * 0x9E3779B9u + 0x632BE5ABu);
    x = mix (x ^ (uint32_t) ua * 2654435761u);
    x = mix (x ^ (uint32_t) (ua >> 32) * 0x85EBCA6Bu);
    x = mix (x ^ ((uint32_t) b * 40503u + 0x51ED27u));
    x = mix (x ^ ((uint32_t) c * 0x2545F491u + 77u));
    return x;
}

inline float rnd (uint32_t seed, int64_t a, int b = 0, int c = 0)
{
    return (float) (hashOf (seed, a, b, c) & 0xFFFFFF) / 16777216.0f;
}

inline int64_t floorDiv (int64_t a, int64_t b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }

constexpr double kTwoPi = 6.283185307179586;

inline float smooth (float t) { t = t < 0 ? 0 : t > 1 ? 1 : t; return t * t * (3.0f - 2.0f * t); }

//==============================================================================
// Var i låten vi är
struct Where
{
    int section = SecLiv;
    double start = 0.0;    // första takten i delen (kan vara negativ före låtstart)
    double length = 0.0;   // takter, 0 = ändlös
    float pos = 0.0f;      // 0..1 genom delen
    float energy = 0.7f;
    double toNext = 1.0e9; // takter kvar till nästa skifte
};

struct SectionDef { int kind; float share; float e0, e1; };

// Låtformen. Andelarna summerar till 1 och landar på hela 4-taktsfraser i alla tre längder.
inline const SectionDef* songSections (int& count)
{
    static const SectionDef s[] = {
        { SecIntro,  0.125f,  0.25f, 0.45f },
        { SecBygg,   0.125f,  0.45f, 0.75f },
        { SecTopp,   0.25f,   0.80f, 0.90f },
        { SecBreak,  0.0625f, 0.18f, 0.22f },
        { SecLyft,   0.0625f, 0.30f, 0.85f },
        { SecTopp2,  0.25f,   0.92f, 1.00f },
        { SecUtrull, 0.125f,  0.70f, 0.30f },
    };
    count = (int) (sizeof (s) / sizeof (s[0]));
    return s;
}

// Fri form: varje block om 64 takter kan sluta med break + lyft (8 + 8 takter).
inline bool friHasBreak (const Settings& st, int64_t block)
{
    return rnd (st.seed, block, 0, 4001) < 0.55f + 0.35f * st.chaos;
}

inline float friWave (const Settings& st, double bar)
{
    const double p1 = 40.0 + 32.0 * rnd (st.seed, 1, 0, 4101);
    const double p2 = 11.0 + 8.0 * rnd (st.seed, 2, 0, 4102);
    const double f1 = kTwoPi * rnd (st.seed, 3, 0, 4103), f2 = kTwoPi * rnd (st.seed, 4, 0, 4104);
    return (float) (0.62 - 0.22 * std::cos (kTwoPi * bar / p1 + f1) + 0.09 * std::sin (kTwoPi * bar / p2 + f2));
}

inline Where locate (const Settings& st, double bar)
{
    Where w;
    // KAOS gör kurvan ojämn: små vågor ovanpå formen
    const double wp = 7.0 + 6.0 * rnd (st.seed, 5, 0, 4201);
    const float wobble = (float) (st.chaos * 0.07 * std::sin (kTwoPi * bar / wp + kTwoPi * rnd (st.seed, 6, 0, 4202)));

    if (st.form == FormLiv)
    {
        w.section = SecLiv;
        w.energy = Settings::clamp01 (0.72f + 0.08f * (float) std::sin (kTwoPi * bar / 32.0 + kTwoPi * rnd (st.seed, 7, 0, 4203)) + wobble);
        w.length = 0.0;
        // ändå en "fras" var 16:e takt, så rullningar och kast vet var gränserna går
        w.start = std::floor (bar / 16.0) * 16.0;
        w.toNext = w.start + 16.0 - bar;
        w.pos = (float) ((bar - w.start) / 16.0);
        return w;
    }

    if (st.form == FormFri)
    {
        const auto block = (int64_t) std::floor (bar / 64.0);
        const double inBlock = bar - (double) block * 64.0;
        const bool brk = friHasBreak (st, block);
        if (brk && inBlock >= 48.0)
        {
            const bool lyft = inBlock >= 56.0;
            w.section = lyft ? SecLyft : SecBreak;
            w.start = (double) block * 64.0 + (lyft ? 56.0 : 48.0);
            w.length = 8.0;
            w.pos = (float) ((bar - w.start) / 8.0);
            w.toNext = w.start + 8.0 - bar;
            w.energy = lyft ? 0.30f + 0.55f * smooth (w.pos) : 0.20f;
            w.energy = Settings::clamp01 (w.energy + wobble * 0.5f);
            return w;
        }
        // vågen, uppdelad i 16-taktsdelar som får namn efter hur kurvan rör sig
        const double seg = std::floor (bar / 16.0) * 16.0;
        const float e = Settings::clamp01 (friWave (st, bar) + wobble);
        const float a = friWave (st, seg), b = friWave (st, seg + 16.0);
        w.section = a > 0.78f && b > 0.78f ? SecTopp : b - a > 0.06f ? SecBygg : a - b > 0.06f ? SecUtrull : SecVag;
        w.start = seg;
        w.length = 16.0;
        w.pos = (float) ((bar - seg) / 16.0);
        w.toNext = seg + 16.0 - bar;
        if (brk && seg + 16.0 > (double) block * 64.0 + 48.0) w.toNext = (double) block * 64.0 + 48.0 - bar;
        w.energy = e;
        return w;
    }

    const int total = formBars (st.form);
    int count = 0;
    const auto* secs = songSections (count);
    const double cycleStart = std::floor (bar / total) * total;
    double s = cycleStart;
    for (int i = 0; i < count; ++i)
    {
        const double len = secs[i].share * total;
        if (bar < s + len || i == count - 1)
        {
            w.section = secs[i].kind;
            w.start = s;
            w.length = len;
            w.pos = (float) ((bar - s) / len);
            w.toNext = s + len - bar;
            const float t = (w.section == SecLyft || w.section == SecBygg) ? w.pos * w.pos : smooth (w.pos);
            w.energy = Settings::clamp01 (secs[i].e0 + (secs[i].e1 - secs[i].e0) * t + wobble);
            return w;
        }
        s += len;
    }
    return w;
}

inline float energyAt (const Settings& st, double bar) { return locate (st, bar).energy; }

//==============================================================================
// Vilka spår som spelar

// Gränsen där spåret kommer in. Kicken bär golvet och spelar alltid utom i break.
inline float threshold (const Settings& st, int li, Role r)
{
    float t = 0.0f;
    switch (r)
    {
        case Kick:    return 0.0f;
        case Hat:     t = 0.18f; break;
        case OpenHat: t = 0.55f; break;
        case Clap:    t = 0.38f; break;
        case Perc:    t = 0.26f + 0.38f * rnd (st.seed, li, 0, 5001); break;
        case Bass:    t = 0.32f; break;
        case Stab:    t = 0.58f; break;
        case Lead:    t = 0.72f; break;
        case Fx:      t = 0.66f; break;
    }
    return t + (rnd (st.seed, li, 0, 5002) - 0.5f) * 0.12f;
}

inline int phraseBars (const Settings& st) { return st.moveEff() > 0.7f ? 4 : 8; }

// Kicken ut: hela breaken och lyftet, plus ibland en takt före en frasgräns i toppen (droppen).
inline bool kickOut (const Settings& st, double bar)
{
    const auto w = locate (st, bar);
    if (w.section == SecBreak || w.section == SecLyft) return true;
    const auto b = (int64_t) std::floor (bar);
    if (b % 16 == 15)
    {
        const float chance = (w.section == SecLiv ? 0.25f * st.moveEff() : 0.0f)
                           + (w.energy > 0.75f ? 0.12f + 0.4f * st.chaos : 0.0f);
        return rnd (st.seed, b, 0, 5101) < chance;
    }
    return false;
}

// Spelar spåret enligt motorn just nu? Energin läses i början av frasen, så spår kommer in
// och går ut på frasgränser och inte mitt i en takt.
inline bool laneActive (const Settings& st, int li, int pad, double bar)
{
    const Role r = roleOf (pad);
    if (r == Kick && li == 0) return ! kickOut (st, bar);
    if (r == Kick) return ! kickOut (st, bar) && energyAt (st, bar) > 0.5f;

    const int pb = phraseBars (st);
    const auto phrase = floorDiv ((int64_t) std::floor (bar), pb);
    const double phraseStart = (double) phrase * pb;
    // frasen börjar aldrig före delen den ligger i; i lyftet kommer spåren in varannan takt
    const auto now = locate (st, bar);
    const double evalAt = now.section == SecLyft ? juce::jmax (now.start, std::floor (bar / 2.0) * 2.0)
                                                 : juce::jmax (now.start, phraseStart);
    const auto w = locate (st, evalAt + 0.01);

    bool on = st.form == FormLiv ? true : w.energy >= threshold (st, li, r);

    // breaken: atmosfären och hatsen ligger kvar, plus det lugnaste percspåret
    if (w.section == SecBreak)
        on = r == Hat || r == Stab || r == Lead || r == Fx || (r == Perc && threshold (st, li, r) < 0.42f);

    // mute-dans: i toppen tystas ett spår i taget, en fras i taget
    if (on && w.energy > 0.74f)
    {
        const int dancer = 1 + (int) (hashOf (st.seed, phrase, 0, 5201) % 7u);
        if (dancer == li && rnd (st.seed, phrase, 0, 5202) < 0.2f + 0.5f * st.moveEff()) on = false;
    }
    // ett spår som ligger strax under gränsen får ibland smyga in en fras
    if (! on && st.form != FormLiv && w.section != SecBreak)
    {
        const float gap = threshold (st, li, r) - w.energy;
        if (gap < 0.2f && rnd (st.seed, phrase, li, 5203) < 0.3f * st.moveEff()) on = true;
    }
    return on;
}

//==============================================================================
// Mutationer. Under en cykel (4 generationer) läggs en liten ändring till per generation,
// sedan kommer grundmönstret tillbaka. Ibland spelar cykeln ett gammalt motiv igen.
struct Clock { int64_t gen = 0, cycle = 0, memoryCycle = 0; int inCycle = 0, genBars = 8; };

inline Clock clockAt (const Settings& st, double bar)
{
    Clock c;
    const int steps = (int) std::lround (st.moveEff() * 4.0f);
    c.genBars = 1 << (5 - steps); // 32, 16, 8, 4, 2 takter
    c.gen = floorDiv ((int64_t) std::floor (bar), c.genBars);
    c.cycle = floorDiv (c.gen, 4);
    c.inCycle = (int) (c.gen - c.cycle * 4);
    c.memoryCycle = c.cycle;
    if (c.cycle >= 2 && rnd (st.seed, c.cycle, 0, 6001) < 0.35f) c.memoryCycle = c.cycle - 2;
    return c;
}

namespace detail
{
inline bool anchor (Role r, int i, int len)
{
    if (r == Kick) return len >= 4 && i % 4 == 0;          // fyra på golvet
    if (r == Clap) return len == 16 && (i == 4 || i == 12); // backbeat
    if (r == Bass) return i == 0;                           // grundtonen på ettan
    return false;
}

// En mutation på spåret. r(k) ger slumptal 0..1 för just den här mutationen.
template <typename R>
void mutate (Lane& L, Role role, float energy, float depth, R r)
{
    const int len = juce::jlimit (1, kMaxSteps, L.length);

    if (L.euclid)
    {
        const float a = r (0);
        if (a < 0.55f) L.hits = juce::jlimit (1, len, L.hits + ((r (1) < 0.35f + 0.4f * energy) ? 1 : -1));
        else L.rotate = (L.rotate + (r (2) < 0.5f ? 1 : len - 1)) % len;
        if (r (3) < 0.3f) { auto& s = L.steps[(size_t) (int) (r (4) * len) % (size_t) len]; s.accent = ! s.accent; if (s.accent) s.ghost = false; }
        return;
    }

    int on[kMaxSteps], off[kMaxSteps], nOn = 0, nOff = 0;
    for (int i = 0; i < len; ++i)
        (L.steps[(size_t) i].on ? on[nOn++] : off[nOff++]) = i;

    if (role == Kick)
    {
        // kicken håller golvet: bara ett svagt extraslag på en svag 16-del
        static const int ghostSpots[] = { 14, 15, 7, 11, 3, 10 };
        const int spot = ghostSpots[(int) (r (5) * 6.0f) % 6];
        if (spot < len && ! L.steps[(size_t) spot].on)
        {
            auto& s = L.steps[(size_t) spot];
            s.on = true; s.ghost = true; s.accent = false; s.prob = (uint8_t) (r (6) < 0.5f ? 100 : 60); s.ratchet = 1;
        }
        return;
    }

    // vikter: fler slag när energin är hög, glesare när den är låg
    float w[7] = { 0.30f + 0.35f * energy,           // 0 lägg till ett svagt slag
                   0.20f + 0.35f * (1.0f - energy),  // 1 ta bort ett slag
                   0.22f,                            // 2 flytta ett slag ett steg
                   0.20f,                            // 3 byt styrka (stark/svag)
                   0.06f + 0.12f * depth,            // 4 ratchet
                   isTonal (role) ? 0.35f : 0.0f,    // 5 tonsteg i F-moll
                   0.10f };                          // 6 sannolikhet
    if (nOn <= 1) w[1] = w[2] = 0.0f;
    if (nOn == 0) for (int k = 1; k < 7; ++k) w[k] = 0.0f;
    if (nOff == 0) w[0] = 0.0f;
    if (nOn == 0 && nOff == 0) return;
    float sum = 0; for (float x : w) sum += x;
    float pick = r (7) * sum;
    int type = 0;
    for (; type < 6; ++type) { if (pick < w[type]) break; pick -= w[type]; }

    auto pickFrom = [&] (const int* list, int n, int salt) { return list[(int) (r (salt) * (float) n) % n]; };

    switch (type)
    {
        case 0:
        {
            // helst bredvid ett befintligt slag, så det låter som en variation och inte slump
            int best = pickFrom (off, nOff, 8);
            for (int k = 0; k < 3; ++k)
            {
                const int c = pickFrom (off, nOff, 9 + k);
                const bool near = L.steps[(size_t) ((c + 1) % len)].on || L.steps[(size_t) ((c + len - 1) % len)].on;
                if (near) { best = c; break; }
            }
            auto& s = L.steps[(size_t) best];
            s.on = true; s.ghost = r (12) < 0.7f; s.accent = false; s.ratchet = 1;
            s.prob = (uint8_t) (r (13) < 0.25f ? 60 : 100);
            if (isTonal (role) && nOn > 0) s.semi = L.steps[(size_t) on[0]].semi;
            break;
        }
        case 1:
        {
            for (int k = 0; k < 4; ++k)
            {
                const int c = pickFrom (on, nOn, 14 + k);
                if (anchor (role, c, len) || L.steps[(size_t) c].accent) continue;
                L.steps[(size_t) c].on = false;
                break;
            }
            break;
        }
        case 2:
        {
            for (int k = 0; k < 4; ++k)
            {
                const int c = pickFrom (on, nOn, 18 + k);
                if (anchor (role, c, len)) continue;
                const int to = (c + (r (22 + k) < 0.5f ? 1 : len - 1)) % len;
                if (L.steps[(size_t) to].on) continue;
                L.steps[(size_t) to] = L.steps[(size_t) c];
                L.steps[(size_t) c] = Step {};
                break;
            }
            break;
        }
        case 3:
        {
            const int idx = pickFrom (on, nOn, 26);
            auto& s = L.steps[(size_t) idx];
            const bool keepStrong = anchor (role, idx, len); // grundslag blir aldrig svaga
            if (s.accent) { s.accent = false; s.ghost = ! keepStrong && r (27) < 0.5f; }
            else if (s.ghost) s.ghost = false;
            else { s.accent = keepStrong || r (27) < 0.5f; s.ghost = ! s.accent; }
            break;
        }
        case 4:
        {
            auto& s = L.steps[(size_t) pickFrom (on, nOn, 28)];
            s.ratchet = (uint8_t) (2 + (r (29) < 0.3f * depth ? 1 : 0));
            break;
        }
        case 5:
        {
            static const int scale[] = { 0, 3, 5, 7, 10, 12, -2, -5, 0, 7 };
            auto& s = L.steps[(size_t) pickFrom (on, nOn, 30)];
            s.semi = (int8_t) juce::jlimit (-24, 24, scale[(int) (r (31) * 10.0f) % 10]);
            break;
        }
        default:
        {
            auto& s = L.steps[(size_t) pickFrom (on, nOn, 32)];
            if (! anchor (role, (int) (&s - &L.steps[0]), len)) s.prob = (uint8_t) (r (33) < 0.5f ? 50 : 75);
            break;
        }
    }
}
} // namespace detail

// Räknar fram den levande kopian av spåret för takten bar. Låsta spår lämnas i fred.
inline void evolve (Lane& L, int li, const Settings& st, double bar)
{
    if (! st.on || L.lock) return;
    const Role role = (li == 0 && L.pad <= 1) ? Kick : roleOf (L.pad);
    const auto c = clockAt (st, bar);
    if (c.inCycle == 0 && c.memoryCycle == c.cycle) return; // cykelns början: grundmönstret kommer tillbaka

    const float depth = st.depthEff();
    const int perGen = role == Kick ? (depth > 0.3f ? 1 : 0) : 1 + (depth > 0.66f ? 1 : 0) + (depth > 0.9f ? 1 : 0);
    const int count = role == Kick ? juce::jmin (1, c.inCycle) * perGen : c.inCycle * perGen;
    const float energy = energyAt (st, (double) (c.gen * c.genBars) + 0.01);

    auto apply = [&] (int64_t cyc, int j)
    {
        detail::mutate (L, role, energy, depth, [&] (int salt) { return rnd (st.seed ^ 0xA511E9B3u, cyc * 131 + j, li, salt); });
    };
    for (int j = 0; j < count; ++j) apply (c.memoryCycle, j);
    // ett gammalt motiv kommer tillbaka, men inte exakt likadant
    if (c.memoryCycle != c.cycle) apply (c.cycle, 1000 + c.inCycle);
}

//==============================================================================
// Groove

// Velocity-våg: en långsam våg genom styrkan med egen längd per spår (3, 5, 7...),
// så den glider mot mönstret och andas.
inline float velocityWave (const Settings& st, int li, Role r, int64_t n)
{
    if (r == Kick || st.feel <= 0.001f) return 1.0f;
    static const int lens[] = { 3, 5, 7, 6, 9, 11, 13 };
    const int len = lens[hashOf (st.seed, li, 0, 7001) % 7u];
    const double ph = kTwoPi * rnd (st.seed, li, 0, 7002);
    return 1.0f + 0.24f * st.feel * (float) std::sin (kTwoPi * (double) n / len + ph);
}

// Mikrotiming i ms. Hats släpar, clap ligger bakåt, perc får en egen personlighet,
// kicken ligger på gridet. Udda 16-delar släpar lite extra, som en människa.
inline float microMs (const Settings& st, int li, Role r, int64_t n)
{
    if (r == Kick || st.feel <= 0.001f) return 0.0f;
    float base = 0.0f;
    switch (r)
    {
        case Hat: case OpenHat: base = 2.0f + 5.0f * rnd (st.seed, li, 0, 7101); break;
        case Clap:  base = 1.5f + 4.0f * rnd (st.seed, li, 0, 7102); break;
        case Perc:  base = -4.0f + 10.0f * rnd (st.seed, li, 0, 7103); break;
        case Bass:  base = -2.0f + 4.0f * rnd (st.seed, li, 0, 7104); break;
        default:    base = -3.0f + 6.0f * rnd (st.seed, li, 0, 7105); break;
    }
    const float odd = (n & 1) ? 3.0f * rnd (st.seed, li, 0, 7106) : 0.0f;
    return st.feel * (base + odd);
}

// Swing som driver långsamt över tid
inline float swingDrift (const Settings& st, double bar)
{
    if (st.feel <= 0.001f) return 0.0f;
    const double p = 24.0 + 24.0 * rnd (st.seed, 8, 0, 7201);
    return 0.06f * st.feel * (float) std::sin (kTwoPi * bar / p + kTwoPi * rnd (st.seed, 9, 0, 7202));
}

//==============================================================================
// Händelser vid frasgränser

// Rullning på sista taktslaget före en 16-taktsgräns eller ett skifte. Returnerar spåret, eller -1.
template <typename LaneInfo>
int rollLane (const Settings& st, double bar, LaneInfo&& info)
{
    const auto b = (int64_t) std::floor (bar);
    const auto w = locate (st, bar);
    const bool edge = (b % 16 == 15) || (w.toNext <= 1.0 + 1.0e-6 && w.toNext > 0.0);
    if (! edge || rnd (st.seed, b, 0, 8001) >= 0.45f + 0.45f * st.moveEff()) return -1;
    int cands[kLanes], n = 0;
    for (int li = 1; li < kLanes; ++li)
    {
        int pad = 0; bool ok = false;
        info (li, pad, ok);
        const Role r = roleOf (pad);
        if (ok && (r == Clap || r == Perc || r == Hat)) cands[n++] = li;
    }
    return n > 0 ? cands[hashOf (st.seed, b, 0, 8002) % (uint32_t) n] : -1;
}

// Dub-kast: vilket spår som kastas in i delayen på sista halva taktslaget i frasen.
template <typename LaneInfo>
int throwLane (const Settings& st, double bar, LaneInfo&& info)
{
    if (st.dub <= 0.001f) return -1;
    const int pb = 8;
    const auto b = (int64_t) std::floor (bar);
    if (b % pb != pb - 1) return -1;
    const auto phrase = floorDiv (b, pb);
    if (rnd (st.seed, phrase, 0, 8101) >= 0.35f + 0.6f * st.dub) return -1;
    int cands[kLanes], n = 0;
    for (int li = 1; li < kLanes; ++li)
    {
        int pad = 0; bool ok = false;
        info (li, pad, ok);
        const Role r = roleOf (pad);
        if (ok && (r == Clap || r == Perc || r == Stab || r == Lead || r == Fx)) cands[n++] = li;
    }
    return n > 0 ? cands[hashOf (st.seed, phrase, 0, 8102) % (uint32_t) n] : -1;
}

//==============================================================================
// Ljudet: filter, resonans, drive, pump och master-svep

struct LaneMod
{
    float cutOct = 0.0f;   // förskjutning av cutoff i oktaver (negativt = stängt)
    float res = 0.0f;      // extra resonans
    float drive = 0.0f;    // extra drive
};

inline LaneMod laneMod (const Settings& st, int li, int pad, double bar, const Where& w)
{
    LaneMod m;
    const Role r = (li == 0 && pad <= 1) ? Kick : roleOf (pad);
    const float F = st.filt, E = w.energy;

    m.drive = st.pump * E * (r == Kick ? 0.12f : 0.3f);
    if (r == Kick || F <= 0.001f) return m;

    float o = -F * 1.3f * (1.0f - E); // mörkare när energin är låg

    // andning: en långsam våg på hats, bas och en del perc
    const bool breathes = r == Hat || r == OpenHat || r == Bass || r == Stab || rnd (st.seed, li, 0, 9001) < 0.6f;
    if (breathes)
    {
        static const double periods[] = { 12.0, 16.0, 24.0, 32.0 };
        const double p = periods[hashOf (st.seed, li, 0, 9002) % 4u];
        o -= F * 0.9f * (float) (0.5 - 0.5 * std::cos (kTwoPi * bar / p + kTwoPi * rnd (st.seed, li, 0, 9003)));
    }

    // stängs inför ett skifte, öppnar när det nya kommer in
    if (w.toNext < 2.0 && w.section != SecLyft)
    {
        const float t = (float) (1.0 - w.toNext / 2.0);
        o -= F * 2.2f * t * t;
    }
    if (w.section == SecBreak) o -= (r == Fx || r == Stab || r == Lead) ? -F * 0.6f : F * 1.0f;
    if (w.section == SecLyft) o = juce::jmin (o, -F * 3.0f * (1.0f - smooth (w.pos)));

    m.cutOct = juce::jlimit (-5.5f, 0.0f, o);

    // resonanstoppar på perc, stabs och lead när det är hett
    if (r == Perc || r == Stab || r == Lead)
    {
        const float hot = juce::jmax (0.0f, E - 0.6f) / 0.4f;
        m.res = F * 0.28f * hot * (float) (0.5 + 0.5 * std::sin (kTwoPi * bar / 8.0 + kTwoPi * rnd (st.seed, li, 0, 9004)));
    }
    return m;
}

// Master-svep: mörkare i breaken, HP som stiger i lyftet och släpper på droppen.
inline float masterSweep (const Settings& st, const Where& w)
{
    const float F = st.filt;
    if (w.section == SecBreak) return -0.25f * F;
    if (w.section == SecLyft) return 0.32f * F * w.pos * w.pos;
    if (w.section == SecBygg && w.toNext < 1.0) return 0.15f * F * (float) (1.0 - w.toNext);
    return 0.0f;
}

// Pumpens djup och släpptid (ms) för spåren som duckas av kicken
inline float pumpDepth (const Settings& st, float energy) { return st.pump * 0.55f * (0.4f + 0.6f * energy); }
inline float pumpReleaseMs (const Settings& st) { return 90.0f + 120.0f * (1.0f - st.pump); }

// Extra glitch när KAOS är uppe och energin hög
inline float glitchBoost (const Settings& st, float energy) { return st.chaos * 0.12f * energy; }
} // namespace betong::motor

namespace betong
{
// Spåret som det faktiskt låter i takten bar: motorns mutationer (om motorn går) och sedan
// Game of Life (om spåret har liv). Används av både ljudtråden och sequencerns visning.
inline void makeLive (Lane& L, int li, const motor::Settings& st, bool motorOn, juce::int64 bar)
{
    if (motorOn) motor::evolve (L, li, st, (double) bar + 0.01);
    if (L.life > 0) applyLife (L, bar);
}

// Fråga och svar: tyst i de block där spåret inte har ordet
inline bool callRespSilent (const Lane& L, juce::int64 bar)
{
    if (L.callResp <= 0) return false;
    const auto block = motor::floorDiv (bar, juce::jmax (1, L.crBars));
    const bool even = (block % 2) == 0;
    return L.callResp == 1 ? ! even : even;
}
} // namespace betong
