// Ritar BETONG-panelen till PNG: BetongSnapshot ut.png
#include "../src/PluginEditor.h"
#include "../src/PluginProcessor.h"
using namespace juce;
int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    BetongProcessor p; p.setRateAndBufferSizeDetails (48000, 512); p.prepareToPlay (48000, 512);
    std::unique_ptr<AudioProcessorEditor> ed (p.createEditor());
    ed->setSize (BetongContent::W, BetongContent::H);
    ed->resized();
    // valfri flik i den nedre panelen: BetongSnapshot ut.png 1|2
    if (argc > 2)
    {
        const int tab = atoi (argv[2]);
        std::function<void (Component&)> walk = [&] (Component& c)
        {
            for (auto* ch : c.getChildren())
            {
                if (auto* b = dynamic_cast<TextButton*> (ch))
                {
                    const auto t = b->getButtonText();
                    if ((tab == 1 && (t == "EFFEKTER" || t == "EFFECTS")) || (tab == 2 && (t == "SPELA" || t == "PLAY"))) if (b->onClick) b->onClick();
                }
                walk (*ch);
            }
        };
        walk (*ed);
    }
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    File f (File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "betong.png"));
    f.deleteFile(); FileOutputStream out (f); PNGImageFormat().writeImageToStream (img, out);
    std::printf ("skrev %s (%dx%d)\n", f.getFullPathName().toRawUTF8(), ed->getWidth(), ed->getHeight());
}
