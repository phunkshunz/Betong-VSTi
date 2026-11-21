// Ny BETONG: låter den fett från första takten, och lever den? (mål BetongStartTest)
#include "../src/PluginProcessor.h"
#include <cstdio>
using namespace juce;
struct PH : AudioPlayHead { int64 s = 0; Optional<PositionInfo> getPosition() const override
    { PositionInfo p; p.setIsPlaying (true); p.setBpm (130); p.setPpqPosition (s / 48000.0 * 130 / 60.0); return p; } };
int main()
{
    ScopedJuceInitialiser_GUI init;
    PH ph; BetongProcessor p; p.setPlayHead (&ph);
    p.setRateAndBufferSizeDetails (48000, 512); p.prepareToPlay (48000, 512);
    std::printf ("motor=%.0f form=%.0f liv=%.2f\n", p.getParam ("mOn"), p.getParam ("mForm"), p.getParam ("liv"));
    const int perBar = (int) (4 * 60.0 / 130 * 48000);
    AudioBuffer<float> b (2, 512); int64 n = 0; double s2 = 0; float pk = 0; bool nan = false; int bar = 0;
    AudioBuffer<float> all (2, perBar * 64); all.clear();
    while (bar < 64)
    {
        MidiBuffer m; p.processBlock (b, m); ph.s += 512;
        for (int i = 0; i < 512; ++i)
        {
            const float x = b.getSample (0, i); nan |= ! std::isfinite (x); s2 += x * x; pk = std::max (pk, std::abs (x));
            if (n < all.getNumSamples()) { all.setSample (0, (int) n, x); all.setSample (1, (int) n, b.getSample (1, i)); }
            if (++n % perBar == 0)
            {
                if (bar % 4 == 0) std::printf ("takt %2d  rms %.3f  topp %.2f  del %d  spår %02x\n", bar, std::sqrt (s2 / perBar), pk, p.motorSection.load(), p.motorLaneMask.load());
                s2 = 0; pk = 0; ++bar;
            }
        }
    }
    std::printf ("nan=%d\n", (int) nan);
    // BETONG-KOD: tur och retur
    p.randomAll();
    const auto kod = p.betongKod();
    std::printf ("kod %d tecken: %s...\n", kod.length(), kod.substring (0, 60).toRawUTF8());
    BetongProcessor q; q.prepareToPlay (48000, 512);
    const auto svar = q.laddaKod ("kolla mitt beat!\n" + kod + "\n");
    bool same = true;
    for (int l = 0; l < betong::kLanes; ++l) same &= betong::laneToString (p.getPattern().lanes[(size_t) l]) == betong::laneToString (q.getPattern().lanes[(size_t) l]);
    bool padsLika = true;
    for (int i = 0; i < betong::kPads; ++i) padsLika &= p.getPad (i).sample->source == q.getPad (i).sample->source;
    int olika = 0;
    for (auto* prm : p.getParameters())
        if (auto* wp = dynamic_cast<AudioProcessorParameterWithID*> (prm))
            if (wp->paramID != "midiOut" && wp->paramID != "midiMap" && wp->paramID != "seqMode" && std::abs (p.getParam (wp->paramID) - q.getParam (wp->paramID)) > 0.01f * (1.0f + std::abs (p.getParam (wp->paramID))))
            { if (olika < 5) std::printf ("  olika %s %.3f %.3f\n", wp->paramID.toRawUTF8(), p.getParam (wp->paramID), q.getParam (wp->paramID)); ++olika; }
    std::printf ("pads lika %d, olika rattar %d\n", (int) padsLika, olika);
    std::printf ("svar: %s | kit %s = %s | fx %s = %s | bpm %.1f = %.1f | spår lika %d\n", svar.toRawUTF8(), p.currentKit.toRawUTF8(), q.currentKit.toRawUTF8(),
                 p.currentFx.toRawUTF8(), q.currentFx.toRawUTF8(), p.getParam ("bpm"), q.getParam ("bpm"), (int) same);
    std::printf ("trasig: %s | ingen: %s\n", q.laddaKod ("BETONG:abc").toRawUTF8(), q.laddaKod ("hej").toRawUTF8());
    File f ("/tmp/claude-0/betong-start.wav"); f.deleteFile();
    std::unique_ptr<AudioFormatWriter> w (WavAudioFormat().createWriterFor (new FileOutputStream (f), 48000, 2, 24, {}, 0));
    w->writeFromAudioSampleBuffer (all, 0, all.getNumSamples());
}
