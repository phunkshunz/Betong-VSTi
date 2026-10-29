#pragma once

// BETONG – rytmbanken. Delad med SPÅN och VENTIL (MASKINHALLEN).
// Arketyper ur 90/00-talets minimal techno och mikrohus, groove-DNA och regler
// per roll som ser till att slumpade rytmer alltid låter sjyssta och aldrig knas.

#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <vector>
#include <string>

namespace mh
{
inline float clampf (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float lerpf (float a, float b, float t) { return a + (b - a) * t; }

// ---------------------------------------------------------------- slump
// Frödstyrd hash: samma frö ger alltid samma tal. All "slump" i musiken går
// genom den här, så att samma ställe i låten låter likadant varje gång.
inline uint32_t hash32 (uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352dU;
    x ^= x >> 15; x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}
inline uint32_t hashMix (uint32_t a, uint32_t b) { return hash32 (a * 0x9e3779b9U ^ hash32 (b + 0x632be59bU)); }
inline uint32_t hashMix (uint32_t a, uint32_t b, uint32_t c) { return hashMix (hashMix (a, b), c); }
inline uint32_t hashMix (uint32_t a, uint32_t b, uint32_t c, uint32_t d) { return hashMix (hashMix (a, b, c), d); }
inline float hashFloat (uint32_t h) { return (float) (h >> 8) * (1.0f / 16777216.0f); }

struct Rng
{
    uint32_t s = 0x12345678U;
    Rng() = default;
    explicit Rng (uint32_t seed) : s (hash32 (seed ^ 0xa511e9b3U) | 1U) {}
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni() { return (float) (next() >> 8) * (1.0f / 16777216.0f); }
    float bi() { return uni() * 2.0f - 1.0f; }
    int below (int n) { return n <= 1 ? 0 : (int) (next() % (uint32_t) n); }
    bool chance (float p) { return uni() < p; }
};


// ------------------------------------------------------------ groove-DNA
struct GrooveDna
{
    const char* name;
    float offset[16];   // förskjutning i andelar av en 16-del (+ = efter)
    float vel[16];      // betoning 0..1
    float ghost[16];    // chans för spökslag på tomma steg
};

constexpr int kNumDna = 8;   // 7 fasta + EGEN

inline const GrooveDna& dnaTable (int i)
{
    static const GrooveDna t[kNumDna] = {
        { "909 RAK",
          { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 },
          { 1,.62f,.78f,.6f, .95f,.62f,.78f,.6f, 1,.62f,.78f,.6f, .95f,.62f,.78f,.6f },
          { 0,.02f,.02f,.03f, 0,.02f,.02f,.03f, 0,.02f,.02f,.03f, 0,.02f,.02f,.05f } },
        { "909 SHUFFLE",
          { 0,.16f,0,.16f, 0,.16f,0,.16f, 0,.16f,0,.16f, 0,.16f,0,.16f },
          { 1,.55f,.85f,.58f, .95f,.55f,.85f,.6f, 1,.55f,.85f,.58f, .95f,.55f,.85f,.62f },
          { 0,.05f,.02f,.06f, 0,.05f,.02f,.06f, 0,.05f,.02f,.06f, 0,.05f,.02f,.08f } },
        { "SLAPP SAMPLER",
          { 0,.12f,.01f,.14f, -.01f,.12f,.02f,.13f, 0,.11f,.01f,.14f, -.01f,.13f,.02f,.12f },
          { 1,.5f,.82f,.56f, .92f,.52f,.8f,.58f, .98f,.5f,.84f,.55f, .9f,.53f,.8f,.6f },
          { 0,.08f,.03f,.09f, 0,.08f,.03f,.09f, 0,.08f,.03f,.09f, 0,.08f,.03f,.12f } },
        { "BERUSAD GUNG",
          { 0,.22f,.04f,.18f, -.02f,.24f,.06f,.14f, 0,.2f,.02f,.26f, -.03f,.18f,.08f,.12f },
          { 1,.45f,.72f,.5f, .88f,.48f,.7f,.55f, .96f,.44f,.74f,.5f, .86f,.47f,.68f,.58f },
          { 0,.12f,.05f,.14f, 0,.12f,.06f,.12f, 0,.13f,.05f,.14f, 0,.12f,.06f,.16f } },
        { "MIKROHUS",
          { 0,.08f,-.03f,.1f, 0,.07f,-.02f,.11f, 0,.08f,-.03f,.1f, 0,.07f,-.02f,.12f },
          { 1,.58f,.74f,.62f, .94f,.6f,.72f,.64f, 1,.58f,.74f,.62f, .94f,.6f,.72f,.66f },
          { 0,.1f,.04f,.1f, 0,.1f,.04f,.1f, 0,.1f,.04f,.1f, 0,.1f,.04f,.12f } },
        { "ROMINIMAL",
          { 0,.06f,-.05f,.1f, 0,.05f,-.06f,.12f, 0,.06f,-.04f,.09f, 0,.05f,-.06f,.13f },
          { 1,.52f,.66f,.58f, .9f,.54f,.64f,.6f, .97f,.52f,.66f,.58f, .9f,.54f,.64f,.62f },
          { 0,.09f,.06f,.08f, 0,.09f,.06f,.08f, 0,.09f,.06f,.08f, 0,.09f,.06f,.1f } },
        { "DETROIT",
          { 0,.1f,0,.1f, 0,.1f,0,.1f, 0,.1f,0,.1f, 0,.1f,0,.1f },
          { .9f,.55f,.7f,.6f, 1,.55f,.7f,.62f, .9f,.55f,.7f,.6f, 1,.55f,.72f,.64f },
          { 0,.04f,.02f,.05f, 0,.04f,.02f,.05f, 0,.04f,.02f,.05f, 0,.04f,.02f,.06f } },
        { "EGEN",
          { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 },
          { 1,.6f,.8f,.6f, 1,.6f,.8f,.6f, 1,.6f,.8f,.6f, 1,.6f,.8f,.6f },
          { 0,.04f,.02f,.04f, 0,.04f,.02f,.04f, 0,.04f,.02f,.04f, 0,.04f,.02f,.04f } },
    };
    return t[(i >= 0 && i < kNumDna) ? i : 0];
}

// En aktiv DNA (kan vara EGEN med uttagna värden).
struct Dna
{
    float offset[16] {}, vel[16] {}, ghost[16] {};
    void load (const GrooveDna& g)
    {
        for (int i = 0; i < 16; ++i) { offset[i] = g.offset[i]; vel[i] = g.vel[i]; ghost[i] = g.ghost[i]; }
    }
};

// Plocka ut svängen ur ett trumloop: transienternas läge relativt 16-delsrutnätet.
// onsets i sampel, loopens längd i sampel, antal takter (heltal).
inline bool extractDna (const std::vector<int>& onsets, const std::vector<float>& strengths, int lengthSamples, int bars, Dna& out)
{
    if (onsets.size() < 6 || bars < 1 || lengthSamples <= 0) return false;
    const double sixteenth = (double) lengthSamples / (bars * 16.0);
    float sum[16] {}, cnt[16] {}, vsum[16] {};
    float vmax = 1.0e-6f;
    for (auto s : strengths) vmax = std::max (vmax, s);
    for (size_t i = 0; i < onsets.size(); ++i)
    {
        const double pos = onsets[i] / sixteenth;
        const double k = std::floor (pos + 0.5);
        const float off = (float) (pos - k);
        if (std::fabs (off) > 0.45f) continue;
        const int idx = ((int) k % 16 + 16) % 16;
        sum[idx] += off; cnt[idx] += 1.0f; vsum[idx] += strengths[i] / vmax;
    }
    int filled = 0;
    for (int i = 0; i < 16; ++i)
    {
        if (cnt[i] > 0) { ++filled; out.offset[i] = clampf (sum[i] / cnt[i], -0.3f, 0.35f); out.vel[i] = clampf (vsum[i] / cnt[i], 0.3f, 1.0f); }
        else { out.offset[i] = 0; out.vel[i] = 0.6f; }
        out.ghost[i] = cnt[i] > 0 ? 0.08f : 0.02f;
    }
    // jämna ut: tomma 16-delar får grannarnas sväng
    for (int i = 0; i < 16; ++i)
        if (cnt[i] <= 0) out.offset[i] = (i % 2 == 1) ? out.offset[(i + 2) % 16] * 0.5f + out.offset[(i + 14) % 16] * 0.5f : 0.0f;
    return filled >= 4;
}

// ------------------------------------------------------------ roller
enum class Role { Kick = 0, Bas, Klick, Tick, Rost, Ton, Luft, Spoke };

// ------------------------------------------------------------ rytmbiblioteket
// 'X' betonat, 'x' slag, 'g' spökslag, '.' tyst. Längden får vara 3..32.
struct LibPattern { const char* steps; float density; };

inline const std::vector<LibPattern>& libraryFor (Role r)
{
    static const std::vector<LibPattern> tick = {
        { "..x...x...x...x.", .25f },  { "..X...x...X...x.", .25f },  { "..x.g.x...x.g.x.", .35f },
        { "x.x.x.x.x.x.x.x.", .5f },   { "g.x.g.x.g.x.g.x.", .5f },   { "..x.x...x.x...x.", .35f },
        { "...x..x....x..x.", .25f },  { "..xg..x...xg..x.", .38f },  { "xgxgxgxgxgxgxgxg", .9f },
        { "..x..gx...x..gx.", .35f },  { ".gx..gx..gx..gx.", .45f },  { "..x...xg..x...xg", .32f },
        { "..x...x...x..xx.", .3f },   { "g.X.g.x.g.X.g.xg", .55f },
    };
    static const std::vector<LibPattern> klick = {
        { "....x.......x...", .12f },  { "....x.......x..g", .15f },  { "...x..x...x..x..", .25f },
        { "x..x..x...x..x..", .3f },   { "..x..x....x..x..", .25f },  { "....x..g....x...", .15f },
        { ".......x.......x", .12f },  { "...x.......x....", .12f },  { "....x..x....x.x.", .25f },
        { "..g.x.....g.x...", .2f },   { "x.....x.....x...", .18f },  { "...g..x..g....x.", .22f },
    };
    static const std::vector<LibPattern> bas = {
        { "..x...x...x...x.", .25f },  { "..x..xx...x..x..", .3f },   { "..xx..x...xx..x.", .38f },
        { "...x..x....x..x.", .25f },  { "..x....x..x....x", .25f },  { "..x.x.....x.x...", .25f },
        { "...xx.....x..x..", .25f },  { "..x...xx..x...x.", .32f },  { "......x.......x.", .12f },
        { "..x..x..x.....x.", .25f },
    };
    static const std::vector<LibPattern> rost = {
        { "......x........x", .12f },  { "...x.......x....", .12f },  { "..x..x.......x..", .18f },
        { ".....x..x.....x.", .18f },  { "x......x..x.....", .18f },  { "...g..x.....g..x", .2f },
        { "..x..x..x..x..x.", .3f },   { ".......x.x.....x", .18f },
    };
    static const std::vector<LibPattern> ton = {
        { "x.....x.....x...", .18f },  { "...x.....x......", .12f },  { "x..x..x..x..x...", .3f },
        { "..x.......x..x..", .18f },  { "x.......x..x....", .18f },  { ".x....x....x....", .18f },
        { "...x..x...x.....", .18f },  { "x..x....x.......", .18f },
    };
    static const std::vector<LibPattern> luft = {
        { "x...............................", .03f }, { "........x.......", .06f }, { "x.......x.......", .12f },
        { "....x...........", .06f }, { "x...........x...", .12f },
    };
    static const std::vector<LibPattern> spoke = {
        { "..x......x..x...", .2f }, { "x...x.......", .16f }, { "...x....x...x.", .2f }, { "x..x.....", .2f },
    };
    static const std::vector<LibPattern> kick = { { "x...x...x...x...", .25f } };
    switch (r)
    {
        case Role::Tick: return tick;
        case Role::Klick: return klick;
        case Role::Bas: return bas;
        case Role::Rost: return rost;
        case Role::Ton: return ton;
        case Role::Luft: return luft;
        case Role::Spoke: return spoke;
        case Role::Kick: default: return kick;
    }
}

// Euklidiskt mönster (Bjorklund via Bresenham), hits <= steps.
inline void euclid (int hits, int steps, int rot, bool* out)
{
    for (int i = 0; i < steps; ++i)
    {
        const int j = ((i + rot) % steps + steps) % steps;
        out[i] = ((j * hits) % steps) < hits;
    }
}

// Goda polymeter-längder per roll (skeva men musikaliska).
inline int goodCycle (Role r, Rng& rng)
{
    static const int rostL[] = { 16, 16, 12, 10, 14, 7, 9, 11, 13 };
    static const int tonL[] = { 16, 12, 12, 10, 14, 6 };
    static const int spokeL[] = { 9, 15, 7, 11, 13, 5 };
    static const int klickL[] = { 16, 16, 16, 12, 14 };
    switch (r)
    {
        case Role::Rost: return rostL[rng.below (9)];
        case Role::Ton: return tonL[rng.below (6)];
        case Role::Spoke: return spokeL[rng.below (6)];
        case Role::Klick: return klickL[rng.below (5)];
        case Role::Luft: return rng.chance (0.5f) ? 32 : 16;
        case Role::Kick: case Role::Bas: case Role::Tick: default: return 16;
    }
}

// ------------------------------------------------------------ reglerna
// Musikalitetsregler: kontrollerar ett steg-mönster för en roll.
// Returnerar true om mönstret är "sjysst". Används för att förkasta mutationer
// och slumpmönster som skulle låta knas.
inline bool rhythmIsMusical (Role r, const bool* hits, const bool* ghost, int len)
{
    int count = 0, run = 0, maxRun = 0, onKick = 0;
    for (int i = 0; i < len; ++i)
    {
        if (hits[i]) { ++count; ++run; maxRun = std::max (maxRun, run); if (len % 4 == 0 && i % 4 == 0 && ! ghost[i]) ++onKick; }
        else run = 0;
    }
    // runt kanten
    if (hits[0] && hits[len - 1]) maxRun = std::max (maxRun, 2);
    switch (r)
    {
        case Role::Tick:  return count >= 1 && (count <= len - 2 || len >= 16);
        case Role::Klick: return count >= 1 && maxRun <= 2 && count <= len / 2;
        case Role::Bas:   return count >= 1 && maxRun <= 3 && onKick <= std::max (1, len / 8) && count <= (len * 5) / 8;
        case Role::Rost:  return count >= 1 && maxRun <= 3 && count <= (len * 5) / 8;
        case Role::Ton:   return count >= 1 && maxRun <= 3 && count <= len / 2;
        case Role::Luft:  return count >= 1 && count <= std::max (2, len / 4);
        case Role::Spoke: return count >= 1 && count <= len / 2;
        case Role::Kick:  default: return true;
    }
}
} // namespace mh
