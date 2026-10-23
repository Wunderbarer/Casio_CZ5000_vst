/*
  CZVoice.h
  ---------
  Eine Polyphonie-Stimme des virtuellen CZ-5000: zwei "Lines" (Line1/Line2),
  jede mit eigenem PD-Oszillator (DCO) und drei Huellkurven (DCA, DCW, DCO).
  Line-Mix-Modi (vereinfacht): Addition oder Ringmodulation der beiden Lines.
*/
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PDOscillator.h"
#include "EnvelopeCZ.h"
#include "CZSound.h"

namespace cz
{

enum class LineMixMode
{
    Line1Only = 0,
    Line2Only,
    Mix,
    RingMod
};

struct LineParams
{
    CZWaveform waveform = CZWaveform::Saw;
    float octave   = 0.0f;  // in Oktaven, z.B. -2..+1
    float detuneCents = 0.0f;
    float dcaKeyFollow = 0.0f; // 0..1, wie stark Rate/Level von der Tonhoehe abhaengen
    float dcwKeyFollow = 0.0f;

    EnvelopeCZ dca;
    EnvelopeCZ dcw;
    EnvelopeCZ dco; // moduliert die Tonhoehe (Pitch-Envelope)
};

class CZVoice : public juce::SynthesiserVoice
{
public:
    CZVoice();

    bool canPlaySound (juce::SynthesiserSound* sound) override;

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void setSampleRateForVoice (double sr);

    // Live-Parameter-Zugriff (wird vom Processor pro Block aus APVTS aktualisiert)
    LineParams line1, line2;
    LineMixMode mixMode = LineMixMode::Mix;
    float masterGain = 0.8f;

private:
    PDOscillator osc1, osc2;
    double sampleRate = 44100.0;
    float baseFreqHz = 440.0f;
    bool noteIsOn = false;

    float midiNoteToFreq (int midiNote, float octaveOffset, float detuneCents) const;
    void  pushEnvelopeParamsToOscillators();
};

} // namespace cz
