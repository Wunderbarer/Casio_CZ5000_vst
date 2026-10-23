#include "CZVoice.h"

namespace cz
{

CZVoice::CZVoice()
{
    line1.dca.reset (0.0f);
    line1.dcw.reset (0.0f);
    line1.dco.reset (0.0f);
    line2.dca.reset (0.0f);
    line2.dcw.reset (0.0f);
    line2.dco.reset (0.0f);
}

bool CZVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<CZSound*> (sound) != nullptr;
}

void CZVoice::setSampleRateForVoice (double sr)
{
    sampleRate = sr;
    osc1.setSampleRate (sr);
    osc2.setSampleRate (sr);
    line1.dca.setSampleRate (sr);
    line1.dcw.setSampleRate (sr);
    line1.dco.setSampleRate (sr);
    line2.dca.setSampleRate (sr);
    line2.dcw.setSampleRate (sr);
    line2.dco.setSampleRate (sr);
}

float CZVoice::midiNoteToFreq (int midiNote, float octaveOffset, float detuneCents) const
{
    float note = (float) midiNote + octaveOffset * 12.0f + detuneCents / 100.0f;
    return 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f);
}

void CZVoice::startNote (int midiNoteNumber, float /*velocity*/,
                         juce::SynthesiserSound*, int /*currentPitchWheelPosition*/)
{
    noteIsOn = true;
    baseFreqHz = midiNoteToFreq (midiNoteNumber, 0.0f, 0.0f);

    float f1 = midiNoteToFreq (midiNoteNumber, line1.octave, line1.detuneCents);
    float f2 = midiNoteToFreq (midiNoteNumber, line2.octave, line2.detuneCents);

    osc1.setFrequency (f1);
    osc2.setFrequency (f2);
    osc1.setWaveform (line1.waveform);
    osc2.setWaveform (line2.waveform);
    osc1.resetPhase (0.0);
    osc2.resetPhase (0.0);

    line1.dca.noteOn();
    line1.dcw.noteOn();
    line1.dco.noteOn();
    line2.dca.noteOn();
    line2.dcw.noteOn();
    line2.dco.noteOn();
}

void CZVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        line1.dca.noteOff();
        line1.dcw.noteOff();
        line1.dco.noteOff();
        line2.dca.noteOff();
        line2.dcw.noteOff();
        line2.dco.noteOff();
    }
    else
    {
        clearCurrentNote();
        noteIsOn = false;
    }
}

void CZVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (! noteIsOn && ! line1.dca.isActive() && ! line2.dca.isActive())
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        float dcw1 = line1.dcw.process();
        float dcw2 = line2.dcw.process();
        osc1.setWindow (dcw1);
        osc2.setWindow (dcw2);

        float pitchMod1 = (line1.dco.process() - 0.5f) * 2.0f;
        float pitchMod2 = (line2.dco.process() - 0.5f) * 2.0f;
        osc1.setFrequency (baseFreqHz * std::pow (2.0f, pitchMod1 * (1.0f / 12.0f)));
        osc2.setFrequency (baseFreqHz * std::pow (2.0f, pitchMod2 * (1.0f / 12.0f)));

        float s1 = osc1.render();
        float s2 = osc2.render();

        float a1 = line1.dca.process();
        float a2 = line2.dca.process();
        s1 *= a1;
        s2 *= a2;

        float mixed;
        switch (mixMode)
        {
            case LineMixMode::Line1Only: mixed = s1; break;
            case LineMixMode::Line2Only: mixed = s2; break;
            case LineMixMode::RingMod:   mixed = s1 * s2; break;
            case LineMixMode::Mix:
            default:                     mixed = (s1 + s2) * 0.5f; break;
        }

        mixed *= masterGain;

        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            outputBuffer.addSample (ch, startSample + i, mixed);

        if (! line1.dca.isActive() && ! line2.dca.isActive())
        {
            clearCurrentNote();
            noteIsOn = false;
            break;
        }
    }
}

} // namespace cz
