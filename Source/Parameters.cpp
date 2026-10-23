#include "Parameters.h"

namespace cz
{
namespace ParamIDs
{
    juce::String linePrefix (int lineIdx) { return "line" + juce::String (lineIdx) + "_"; }
    juce::String waveform (int lineIdx)     { return linePrefix (lineIdx) + "waveform"; }
    juce::String octave (int lineIdx)       { return linePrefix (lineIdx) + "octave"; }
    juce::String detune (int lineIdx)       { return linePrefix (lineIdx) + "detune"; }
    juce::String dcaKeyFollow (int lineIdx) { return linePrefix (lineIdx) + "dcaKF"; }
    juce::String dcwKeyFollow (int lineIdx) { return linePrefix (lineIdx) + "dcwKF"; }

    juce::String envRate (int lineIdx, const juce::String& env, int stage)
    { return linePrefix (lineIdx) + env + "_rate" + juce::String (stage); }

    juce::String envLevel (int lineIdx, const juce::String& env, int stage)
    { return linePrefix (lineIdx) + env + "_level" + juce::String (stage); }

    juce::String envEnd (int lineIdx, const juce::String& env)
    { return linePrefix (lineIdx) + env + "_end"; }

    const juce::String mixMode    { "mixMode" };
    const juce::String masterGain { "masterGain" };
    const juce::String hwChannel  { "hwChannel" };
    const juce::String hwToneSlot { "hwToneSlot" };
    const juce::String liveSync   { "liveSync" };
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    juce::StringArray waveNames { "Saw", "Square", "Pulse", "DoubleSine", "SawPulse", "Reso1", "Reso2", "Reso3" };

    for (int lineIdx = 1; lineIdx <= 2; ++lineIdx)
    {
        juce::String lp = "Line " + juce::String (lineIdx) + " ";

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            ParamIDs::waveform (lineIdx), lp + "Waveform", waveNames, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            ParamIDs::octave (lineIdx), lp + "Octave",
            juce::NormalisableRange<float> (-2.0f, 2.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            ParamIDs::detune (lineIdx), lp + "Detune (cents)",
            juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            ParamIDs::dcaKeyFollow (lineIdx), lp + "DCA KeyFollow",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            ParamIDs::dcwKeyFollow (lineIdx), lp + "DCW KeyFollow",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        const char* envNames[3] = { "dca", "dcw", "dco" };
        for (auto* envName : envNames)
        {
            for (int stage = 0; stage < 8; ++stage)
            {
                bool isFirstDcaStage = (juce::String (envName) == "dca" && stage == 0);
                float defaultRate  = isFirstDcaStage ? 99.0f : 50.0f;
                float defaultLevel = isFirstDcaStage ? 99.0f : 0.0f;

                params.push_back (std::make_unique<juce::AudioParameterFloat> (
                    ParamIDs::envRate (lineIdx, envName, stage),
                    lp + envName + " Rate " + juce::String (stage),
                    juce::NormalisableRange<float> (0.0f, 99.0f, 1.0f), defaultRate));

                params.push_back (std::make_unique<juce::AudioParameterFloat> (
                    ParamIDs::envLevel (lineIdx, envName, stage),
                    lp + envName + " Level " + juce::String (stage),
                    juce::NormalisableRange<float> (0.0f, 99.0f, 1.0f), defaultLevel));
            }

            params.push_back (std::make_unique<juce::AudioParameterInt> (
                ParamIDs::envEnd (lineIdx, envName), lp + envName + " End Step", 0, 7, 1));
        }
    }

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        ParamIDs::mixMode, "Line Mix Mode",
        juce::StringArray { "Line1 Only", "Line2 Only", "Mix", "Ring Mod" }, 2));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        ParamIDs::masterGain, "Master Gain", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        ParamIDs::hwChannel, "Hardware MIDI Channel", 1, 16, 1));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        ParamIDs::hwToneSlot, "Hardware Tone Slot", 0, 15, 0));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        ParamIDs::liveSync, "Live Hardware Sync", false));

    return { params.begin(), params.end() };
}

} // namespace cz
