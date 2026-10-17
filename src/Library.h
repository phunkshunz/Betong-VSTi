#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <map>
#include <memory>

namespace betong
{
// Ett laddat ljud. Delas mellan pads (samma fil kan hackas till 16 pads).
struct Sample
{
    juce::AudioBuffer<float> data; // 1 eller 2 kanaler
    double sampleRate = 48000.0;
    juce::String name;
    juce::String source;           // "builtin:<namn>" eller filsökväg
};
using SamplePtr = std::shared_ptr<const Sample>;

// Kategorier för det inbyggda biblioteket (prefix i filnamnet).
struct Category { const char* prefix; const char* name; };
constexpr Category kCategories[] = {
    { "K", "Kickar" }, { "H", "Hats och cymbaler" }, { "C", "Clap och snare" }, { "P", "Perc" },
    { "B", "Bas" }, { "S", "Stabs och ackord" }, { "L", "Leads" }, { "T", "Texturer och drönare" },
    { "V", "Röster" }, { "X", "FX" }, { "Lp", "Loopar 130 BPM" }, { "F", "Smedja" },
};

// Inbyggda ljud, laddas en gång per process och delas av alla BETONG-instanser.
class BuiltInLibrary
{
public:
    BuiltInLibrary();

    const juce::StringArray& getNames() const { return names; }
    juce::StringArray getNamesInCategory (const juce::String& prefix) const;
    static juce::String categoryOf (const juce::String& name);
    static juce::String displayName (const juce::String& name);

    // Avkodar vid första användningen och cachar. Bara från meddelandetråden.
    SamplePtr get (const juce::String& name);

private:
    juce::AudioFormatManager formats;
    juce::StringArray names;                 // "K01_909_Punch" ...
    std::map<juce::String, juce::String> resourceFor; // namn -> BinaryData-resurs
    std::map<juce::String, SamplePtr> cache;
    juce::CriticalSection lock;
};

// Laddar en ljudfil från disk (max 60 s).
SamplePtr loadSampleFile (const juce::File& file, juce::AudioFormatManager& formats);

// Hackar ett ljud i bitar. Returnerar startpunkter i sampel (första är alltid 0).
std::vector<int> findSlices (const Sample& s, int mode, float sensitivity, int maxSlices);
} // namespace betong
