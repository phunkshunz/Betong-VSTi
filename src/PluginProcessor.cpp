#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

BetongProcessor::BetongProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.registerBasicFormats(); // WAV, AIFF, FLAC, OGG, MP3 (på Mac)

    // Globala inställningar: ~/Library/Application Support/BETONG/settings.xml
    juce::PropertiesFile::Options opts;
    opts.applicationName = "settings";
    opts.folderName = "BETONG";
    opts.filenameSuffix = ".xml";
    opts.osxLibrarySubFolder = "Application Support";
    settings = std::make_unique<juce::PropertiesFile> (opts);

    for (int i = 0; i < numVoices; ++i)
        synth.addVoice (new juce::SamplerVoice());

    loadBuiltInKick();
}

bool BetongProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void BetongProcessor::prepareToPlay (double sampleRate, int)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
}

void BetongProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    for (const auto metadata : midi)
        if (metadata.getMessage().isNoteOn())
            ++lastNoteCounter;

    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());
}

bool BetongProcessor::loadFromReader (std::unique_ptr<juce::AudioFormatReader> reader, const juce::String& name)
{
    if (reader == nullptr)
        return false;

    juce::BigInteger allNotes;
    allNotes.setRange (0, 128, true);

    // Synthesiser låser själv internt när ljud byts, så detta är säkert under uppspelning.
    synth.clearSounds();
    synth.addSound (new juce::SamplerSound (name, *reader, allNotes, rootNote,
                                            0.0,   // attack (s)
                                            0.02,  // release (s)
                                            30.0)); // max längd (s)

    const juce::ScopedLock sl (nameLock);
    sampleName = name;
    return true;
}

bool BetongProcessor::loadSampleFile (const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (! loadFromReader (std::move (reader), file.getFileNameWithoutExtension()))
        return false;

    currentFile = file;
    return true;
}

void BetongProcessor::loadBuiltInKick()
{
    auto stream = std::make_unique<juce::MemoryInputStream> (BinaryData::Kick_909_Punch_wav,
                                                             (size_t) BinaryData::Kick_909_Punch_wavSize,
                                                             false);
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatReader> reader (wav.createReaderFor (stream.release(), true));
    loadFromReader (std::move (reader), "Kick_909_Punch (inbyggd)");
    currentFile = juce::File();
}

juce::String BetongProcessor::getSampleName() const
{
    const juce::ScopedLock sl (nameLock);
    return sampleName;
}

juce::StringArray BetongProcessor::getSampleFolders() const
{
    settings->reload(); // en annan BETONG-instans kan ha ändrat listan
    juce::StringArray folders;
    folders.addTokens (settings->getValue ("sampleFolders"), "\n", {});
    folders.removeEmptyStrings();
    return folders;
}

void BetongProcessor::addSampleFolder (const juce::File& folder)
{
    auto folders = getSampleFolders();
    folders.addIfNotAlreadyThere (folder.getFullPathName());
    settings->setValue ("sampleFolders", folders.joinIntoString ("\n"));
    settings->saveIfNeeded();
}

void BetongProcessor::removeSampleFolder (const juce::String& path)
{
    auto folders = getSampleFolders();
    folders.removeString (path);
    settings->setValue ("sampleFolders", folders.joinIntoString ("\n"));
    settings->saveIfNeeded();
}

juce::Array<juce::File> BetongProcessor::scanSampleFolders() const
{
    juce::Array<juce::File> result;
    for (const auto& path : getSampleFolders())
    {
        const juce::File folder (path);
        if (! folder.isDirectory())
            continue;

        for (const auto& entry : juce::RangedDirectoryIterator (folder, true, audioWildcard, juce::File::findFiles))
        {
            result.add (entry.getFile());
            if (result.size() >= 5000) // skydd mot jättemappar
                return result;
        }
    }

    struct ByName
    {
        static int compareElements (const juce::File& a, const juce::File& b)
        {
            return a.getFileName().compareNatural (b.getFileName());
        }
    } sorter;
    result.sort (sorter);
    return result;
}

void BetongProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement xml ("BETONG");
    xml.setAttribute ("version", 1);
    xml.setAttribute ("samplePath", currentFile.getFullPathName());
    copyXmlToBinary (xml, destData);
}

void BetongProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        const juce::File file (xml->getStringAttribute ("samplePath"));
        if (file.existsAsFile())
            loadSampleFile (file);
        else
            loadBuiltInKick();
    }
}

juce::AudioProcessorEditor* BetongProcessor::createEditor()
{
    return new BetongEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BetongProcessor();
}
