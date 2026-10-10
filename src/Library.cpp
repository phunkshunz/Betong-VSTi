#include "Library.h"
#include "BinaryData.h"

namespace betong
{
BuiltInLibrary::BuiltInLibrary()
{
    formats.registerBasicFormats();

    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const char* res = BinaryData::namedResourceList[i];
        const juce::String file (BinaryData::getNamedResourceOriginalFilename (res));
        if (! file.endsWithIgnoreCase (".flac"))
            continue;
        const auto name = file.upToLastOccurrenceOf (".", false, false);
        names.add (name);
        resourceFor[name] = res;
    }
    names.sortNatural();
}

juce::String BuiltInLibrary::categoryOf (const juce::String& name)
{
    if (name.startsWith ("Lp")) return "Lp";
    return name.substring (0, 1);
}

juce::String BuiltInLibrary::displayName (const juce::String& name)
{
    // "K01_909_Punch" -> "909 Punch"
    auto rest = name.fromFirstOccurrenceOf ("_", false, false);
    return rest.replaceCharacter ('_', ' ');
}

juce::StringArray BuiltInLibrary::getNamesInCategory (const juce::String& prefix) const
{
    juce::StringArray out;
    for (const auto& n : names)
        if (categoryOf (n) == prefix)
            out.add (n);
    return out;
}

SamplePtr BuiltInLibrary::get (const juce::String& name)
{
    const juce::ScopedLock sl (lock);
    if (auto it = cache.find (name); it != cache.end())
        return it->second;

    auto rit = resourceFor.find (name);
    if (rit == resourceFor.end())
        return nullptr;

    int size = 0;
    const char* data = BinaryData::getNamedResource (rit->second.toRawUTF8(), size);
    if (data == nullptr || size <= 0)
        return nullptr;

    std::unique_ptr<juce::AudioFormatReader> reader (
        formats.createReaderFor (std::make_unique<juce::MemoryInputStream> (data, (size_t) size, false)));
    if (reader == nullptr)
        return nullptr;

    auto s = std::make_shared<Sample>();
    s->sampleRate = reader->sampleRate;
    s->name = displayName (name);
    s->source = "builtin:" + name;
    s->data.setSize ((int) juce::jmin (2u, reader->numChannels), (int) reader->lengthInSamples);
    reader->read (&s->data, 0, (int) reader->lengthInSamples, 0, true, s->data.getNumChannels() > 1);
    SamplePtr result = s;
    cache[name] = result;
    return result;
}

SamplePtr loadSampleFile (const juce::File& file, juce::AudioFormatManager& formats)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr)
        return nullptr;

    const auto maxLen = (juce::int64) (reader->sampleRate * 60.0);
    const int len = (int) juce::jmin (reader->lengthInSamples, maxLen);
    if (len <= 0)
        return nullptr;

    auto s = std::make_shared<Sample>();
    s->sampleRate = reader->sampleRate;
    s->name = file.getFileNameWithoutExtension();
    s->source = file.getFullPathName();
    s->data.setSize ((int) juce::jmin (2u, reader->numChannels), len);
    reader->read (&s->data, 0, len, 0, true, s->data.getNumChannels() > 1);
    return s;
}

std::vector<int> findSlices (const Sample& s, int mode, float sensitivity, int maxSlices)
{
    const int len = s.data.getNumSamples();
    std::vector<int> starts;
    if (len <= 0) return starts;

    if (mode > 0) // jämna bitar: mode = antal
    {
        const int count = juce::jlimit (1, maxSlices, mode);
        for (int i = 0; i < count; ++i)
            starts.push_back ((int) ((juce::int64) len * i / count));
        return starts;
    }

    // Anslag: energiökning i korta fönster (enkel onset-detektor)
    const int hop = juce::jmax (64, (int) (s.sampleRate * 0.005));
    const int frames = len / hop;
    std::vector<float> energy ((size_t) frames, 0.0f);
    for (int f = 0; f < frames; ++f)
    {
        float e = 0.0f;
        for (int ch = 0; ch < s.data.getNumChannels(); ++ch)
        {
            const float* d = s.data.getReadPointer (ch, f * hop);
            for (int i = 0; i < hop; ++i) e += d[i] * d[i];
        }
        energy[(size_t) f] = std::log10 (1e-9f + e / (float) hop) * 10.0f; // dB
    }

    struct Onset { int frame; float strength; };
    std::vector<Onset> onsets;
    const float threshold = 14.0f - sensitivity * 11.0f; // dB ökning som krävs
    const int minGap = juce::jmax (1, (int) (0.06 * s.sampleRate / hop));
    int last = -minGap;
    for (int f = 2; f < frames; ++f)
    {
        const float rise = energy[(size_t) f] - juce::jmin (energy[(size_t) f - 1], energy[(size_t) f - 2]);
        if (rise > threshold && energy[(size_t) f] > -55.0f && f - last >= minGap)
        {
            onsets.push_back ({ f, rise });
            last = f;
        }
    }

    // behåll de starkaste om det blev för många
    if ((int) onsets.size() > maxSlices - 1)
    {
        std::sort (onsets.begin(), onsets.end(), [] (auto& a, auto& b) { return a.strength > b.strength; });
        onsets.resize ((size_t) (maxSlices - 1));
        std::sort (onsets.begin(), onsets.end(), [] (auto& a, auto& b) { return a.frame < b.frame; });
    }

    starts.push_back (0);
    for (auto& o : onsets)
    {
        const int pos = juce::jmax (0, o.frame * hop - hop / 2);
        if (pos - starts.back() > hop * 4)
            starts.push_back (pos);
    }

    if (starts.size() < 2) // inga anslag hittades: dela jämnt i 8
        return findSlices (s, 8, sensitivity, maxSlices);
    return starts;
}
} // namespace betong
