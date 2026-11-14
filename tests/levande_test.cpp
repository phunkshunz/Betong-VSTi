// BETONG 3: LIV, UTBROTT, FÅNGA och NAV (cmake -DBETONG_SNAPSHOT=ON, mål BetongLevandeTest)
#include "../src/PluginProcessor.h"
#include <cstdio>
using namespace juce;
static int fails = 0;
static void check (bool ok, const char* w) { std::printf ("%s  %s\n", ok ? "ok  " : "FAIL", w); if (! ok) ++fails; }
struct PH : AudioPlayHead { int64 s = 0; Optional<PositionInfo> getPosition() const override
    { PositionInfo p; p.setIsPlaying (true); p.setBpm (128); p.setPpqPosition (s / 48000.0 * 128 / 60.0); return p; } };
struct Res { std::vector<float> L; float peak = 0; bool nan = false; };
static Res render (BetongProcessor& p, PH& ph, int blocks)
{
    Res r; AudioBuffer<float> b (2, 512);
    for (int k = 0; k < blocks; ++k)
    {
        MidiBuffer m; p.processBlock (b, m); ph.s += 512;
        for (int i = 0; i < 512; ++i) { const float x = b.getSample (0, i), y = b.getSample (1, i); r.L.push_back (x);
            if (! std::isfinite (x) || ! std::isfinite (y)) r.nan = true; r.peak = std::max ({ r.peak, std::abs (x), std::abs (y) }); }
    }
    return r;
}
static std::unique_ptr<BetongProcessor> make (PH& ph, float liv, float ub)
{
    auto p = std::make_unique<BetongProcessor>(); p->setPlayHead (&ph);
    p->setRateAndBufferSizeDetails (48000, 512); p->prepareToPlay (48000, 512);
    p->setParam ("liv", liv); p->setParam ("utbrott", ub);
    return p;
}
int main()
{
    ScopedJuceInitialiser_GUI init;
    const int blocks = (int) (8 * 4 * 60.0 / 128 * 48000 / 512);   // 8 takter
    PH a, b, c;
    auto p0 = make (a, 0, 0); auto p0b = make (b, 0, 0); auto p1 = make (c, 1, 1);
    // samma utgångsläge i alla tre (BETONG slumpar fröet när den startar)
    MemoryBlock st; p0->getStateInformation (st);
    p0b->setStateInformation (st.getData(), (int) st.getSize()); p1->setStateInformation (st.getData(), (int) st.getSize());
    p1->setParam ("liv", 1.0f); p1->setParam ("utbrott", 1.0f);
    for (auto* q : { p0.get(), p0b.get(), p1.get() }) { q->setSeedForTest (1234); q->prepareToPlay (48000, 512); }
    const auto r0 = render (*p0, a, blocks), r0b = render (*p0b, b, blocks), r1 = render (*p1, c, blocks);
    double d0 = 0, d1 = 0; for (size_t i = 0; i < r0.L.size(); ++i) { d0 = std::max (d0, (double) std::abs (r0.L[i] - r0b.L[i])); d1 = std::max (d1, (double) std::abs (r0.L[i] - r1.L[i])); }
    std::printf ("LEVANDE  av: skillnad %.2e, topp %.2f  |  på: skillnad %.3f, topp %.2f\n", d0, r0.peak, d1, r1.peak);
    check (d0 == 0.0 && ! r0.nan && r0.peak <= 1.0f, "av: samma ljud varje gång, inga toppar");
    check (d1 > 0.01 && ! r1.nan && r1.peak <= 1.0f, "LIV och UTBROTT hörs, inga toppar, inga NaN");

    // FÅNGA
    const int n = p1->fanga (2, 0, 0.6f);
    const auto dir = File::getSpecialLocation (File::userMusicDirectory).getChildFile ("MASKINHALLEN").getChildFile (String::fromUTF8 ("BETONG FÅNGAT"));
    const bool file = dir.findChildFiles (File::findFiles, false, "*.wav").size() > 0;
    std::printf ("FÅNGA    %d ljud på padsen, fil: %s\n", n, file ? "ja" : "nej");
    check (n >= 2 && file, "FÅNGA hackar de senaste takterna till pads och sparar en WAV");
    const auto after = render (*p1, c, 200);
    check (! after.nan && after.peak <= 1.0f && after.peak > 0.01f, "det fångade kitet spelar");

    // NAV
    p1->setParam ("nav", 1.0f);
    for (int i = 0; i < 3; ++i) { p1->callTimerForTest(); render (*p1, c, 20); }
    mh::Nav reader; const bool open = reader.open();
    const bool alive = open && reader.alive (Time::currentTimeMillis());
    std::printf ("NAV      öppen %s, lever %s, bpm %.1f\n", open ? "ja" : "nej", alive ? "ja" : "nej", open ? reader.state()->bpm : 0.0);
    check (alive && std::abs (reader.state()->bpm - 128.0) < 0.01, "NAV skickar tempo och puls");
    std::printf ("\n%s\n", fails ? "FEL" : "ALL OK");
    return fails ? 1 : 0;
}
