#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "MIDI/CZSysEx.h"

CZ5000AudioProcessor::CZ5000AudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", cz::createParameterLayout())
{
    for (int i = 0; i < kNumVoices; ++i)
        synth.addVoice (new cz::CZVoice());
    synth.addSound (new cz::CZSound());

    hardwareBridge.onToneReceived = [this] (const cz::ParsedFrame& frame)
    {
        applyToneToParameters (frame.tone);
    };

    startTimerHz (8); // Live-Sync Throttle: max. 8x/Sekunde an Hardware senden
}

CZ5000AudioProcessor::~CZ5000AudioProcessor() {}

void CZ5000AudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<cz::CZVoice*> (synth.getVoice (i)))
            v->setSampleRateForVoice (sampleRate);
}

bool CZ5000AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void CZ5000AudioProcessor::updateVoicesFromParameters()
{
    auto readEnv = [this] (cz::EnvelopeCZ& env, int lineIdx, const char* name)
    {
        for (int s = 0; s < 8; ++s)
        {
            env.stages[(size_t) s].rate  = apvts.getRawParameterValue (cz::ParamIDs::envRate  (lineIdx, name, s))->load();
            env.stages[(size_t) s].level = apvts.getRawParameterValue (cz::ParamIDs::envLevel (lineIdx, name, s))->load() / 99.0f;
        }
        env.endStep = (int) apvts.getRawParameterValue (cz::ParamIDs::envEnd (lineIdx, name))->load();
    };

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        auto* v = dynamic_cast<cz::CZVoice*> (synth.getVoice (i));
        if (v == nullptr) continue;

        v->line1.waveform    = (cz::CZWaveform) (int) apvts.getRawParameterValue (cz::ParamIDs::waveform (1))->load();
        v->line1.octave      = apvts.getRawParameterValue (cz::ParamIDs::octave (1))->load();
        v->line1.detuneCents = apvts.getRawParameterValue (cz::ParamIDs::detune (1))->load();
        readEnv (v->line1.dca, 1, "dca");
        readEnv (v->line1.dcw, 1, "dcw");
        readEnv (v->line1.dco, 1, "dco");

        v->line2.waveform    = (cz::CZWaveform) (int) apvts.getRawParameterValue (cz::ParamIDs::waveform (2))->load();
        v->line2.octave      = apvts.getRawParameterValue (cz::ParamIDs::octave (2))->load();
        v->line2.detuneCents = apvts.getRawParameterValue (cz::ParamIDs::detune (2))->load();
        readEnv (v->line2.dca, 2, "dca");
        readEnv (v->line2.dcw, 2, "dcw");
        readEnv (v->line2.dco, 2, "dco");

        v->mixMode    = (cz::LineMixMode) (int) apvts.getRawParameterValue (cz::ParamIDs::mixMode)->load();
        v->masterGain = apvts.getRawParameterValue (cz::ParamIDs::masterGain)->load();
    }
}

void CZ5000AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    updateVoicesFromParameters();
    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* CZ5000AudioProcessor::createEditor()
{
    return new CZ5000AudioProcessorEditor (*this);
}

void CZ5000AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void CZ5000AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

cz::CZTone CZ5000AudioProcessor::buildToneFromParameters() const
{
    cz::CZTone tone;

    auto writeEnv = [this, &tone] (cz::FieldRange endField, cz::FieldRange envField, int lineIdx, const char* name, bool isDco)
    {
        int endStep = (int) apvts.getRawParameterValue (cz::ParamIDs::envEnd (lineIdx, name))->load();
        tone.set (endField, 0, (uint8_t) endStep);
        for (int s = 0; s < 8; ++s)
        {
            int rate  = (int) apvts.getRawParameterValue (cz::ParamIDs::envRate  (lineIdx, name, s))->load();
            int level = (int) apvts.getRawParameterValue (cz::ParamIDs::envLevel (lineIdx, name, s))->load();
            uint8_t rb = cz::encodeDcaRate (rate, false);
            uint8_t lb = isDco ? cz::encodeDcoLevel (level) : cz::encodeDcaLevel (level);
            tone.set (envField, s * 2,     rb);
            tone.set (envField, s * 2 + 1, lb);
        }
    };

    // Vereinfachtes Waveform-Mapping: Index direkt in Low-Byte.
    // Siehe README.md Punkt 3 fuer Details zur noch zu verifizierenden
    // exakten Wave1/Wave2/Window/Modulation Bit-Belegung.
    int wave1 = (int) apvts.getRawParameterValue (cz::ParamIDs::waveform (1))->load();
    int wave2 = (int) apvts.getRawParameterValue (cz::ParamIDs::waveform (2))->load();
    tone.set (cz::Field::mfw, 0, (uint8_t) wave1);
    tone.set (cz::Field::mfw, 1, 0);
    tone.set (cz::Field::sfw, 0, (uint8_t) wave2);
    tone.set (cz::Field::sfw, 1, 0);

    writeEnv (cz::Field::dca1End, cz::Field::dca1Env, 1, "dca", false);
    writeEnv (cz::Field::dcw1End, cz::Field::dcw1Env, 1, "dcw", false);
    writeEnv (cz::Field::dco1End, cz::Field::dco1Env, 1, "dco", true);
    writeEnv (cz::Field::dca2End, cz::Field::dca2Env, 2, "dca", false);
    writeEnv (cz::Field::dcw2End, cz::Field::dcw2Env, 2, "dcw", false);
    writeEnv (cz::Field::dco2End, cz::Field::dco2Env, 2, "dco", true);

    return tone;
}

void CZ5000AudioProcessor::applyToneToParameters (const cz::CZTone& tone)
{
    auto readEnv = [this, &tone] (cz::FieldRange endField, cz::FieldRange envField, int lineIdx, const char* name, bool isDco)
    {
        int endStep = tone.get (endField, 0);
        if (auto* p = apvts.getParameter (cz::ParamIDs::envEnd (lineIdx, name)))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) endStep));

        for (int s = 0; s < 8; ++s)
        {
            uint8_t rb = tone.get (envField, s * 2);
            uint8_t lb = tone.get (envField, s * 2 + 1);

            int rate; bool levelFalls;
            cz::decodeDcaRate (rb, rate, levelFalls);
            int level = isDco ? cz::decodeDcoLevel (lb) : cz::decodeDcaLevel (lb & 0x7F);

            if (auto* pr = apvts.getParameter (cz::ParamIDs::envRate (lineIdx, name, s)))
                pr->setValueNotifyingHost (pr->convertTo0to1 ((float) rate));
            if (auto* pl = apvts.getParameter (cz::ParamIDs::envLevel (lineIdx, name, s)))
                pl->setValueNotifyingHost (pl->convertTo0to1 ((float) level));
        }
    };

    int wave1 = tone.get (cz::Field::mfw, 0);
    int wave2 = tone.get (cz::Field::sfw, 0);
    if (auto* p = apvts.getParameter (cz::ParamIDs::waveform (1)))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) juce::jlimit (0, 7, wave1)));
    if (auto* p = apvts.getParameter (cz::ParamIDs::waveform (2)))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) juce::jlimit (0, 7, wave2)));

    readEnv (cz::Field::dca1End, cz::Field::dca1Env, 1, "dca", false);
    readEnv (cz::Field::dcw1End, cz::Field::dcw1Env, 1, "dcw", false);
    readEnv (cz::Field::dco1End, cz::Field::dco1Env, 1, "dco", true);
    readEnv (cz::Field::dca2End, cz::Field::dca2Env, 2, "dca", false);
    readEnv (cz::Field::dcw2End, cz::Field::dcw2Env, 2, "dcw", false);
    readEnv (cz::Field::dco2End, cz::Field::dco2Env, 2, "dco", true);
}

void CZ5000AudioProcessor::sendCurrentToneToHardware()
{
    if (! hardwareBridge.isOutputOpen())
        return;

    auto tone = buildToneFromParameters();
    int channel = (int) apvts.getRawParameterValue (cz::ParamIDs::hwChannel)->load() - 1;
    int toneSlot = (int) apvts.getRawParameterValue (cz::ParamIDs::hwToneSlot)->load();
    hardwareBridge.sendTone (tone, 0x20 + toneSlot, channel);
}

void CZ5000AudioProcessor::timerCallback()
{
    bool liveSyncOn = apvts.getRawParameterValue (cz::ParamIDs::liveSync)->load() > 0.5f;
    if (liveSyncOn)
        sendCurrentToneToHardware();
}

// Diese Funktion erzeugt die vom Host benoetigte Plugin-Instanz.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CZ5000AudioProcessor();
}
