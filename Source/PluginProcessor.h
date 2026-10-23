#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "DSP/CZVoice.h"
#include "MIDI/HardwareBridge.h"

class CZ5000AudioProcessor : public juce::AudioProcessor,
                              private juce::Timer
{
public:
    CZ5000AudioProcessor();
    ~CZ5000AudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "CZ-5000 Virtual"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    cz::HardwareBridge hardwareBridge;

    cz::CZTone buildToneFromParameters() const;
    void applyToneToParameters (const cz::CZTone& tone);
    void sendCurrentToneToHardware();

private:
    juce::Synthesiser synth;
    static constexpr int kNumVoices = 8;

    void updateVoicesFromParameters();
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CZ5000AudioProcessor)
};
