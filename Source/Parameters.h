#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace cz
{
namespace ParamIDs
{
    juce::String linePrefix (int lineIdx);
    juce::String waveform (int lineIdx);
    juce::String octave (int lineIdx);
    juce::String detune (int lineIdx);
    juce::String dcaKeyFollow (int lineIdx);
    juce::String dcwKeyFollow (int lineIdx);
    juce::String envRate (int lineIdx, const juce::String& env, int stage);
    juce::String envLevel (int lineIdx, const juce::String& env, int stage);
    juce::String envEnd (int lineIdx, const juce::String& env);

    extern const juce::String mixMode;
    extern const juce::String masterGain;
    extern const juce::String hwChannel;
    extern const juce::String hwToneSlot;
    extern const juce::String liveSync;
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace cz
