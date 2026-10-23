#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace cz
{
/** Triviale Sound-Klasse: unsere CZVoice kann jede Note auf jedem Kanal spielen. */
class CZSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};
} // namespace cz
