#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "GUI/HardwarePanel.h"

class CZ5000AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit CZ5000AudioProcessorEditor (CZ5000AudioProcessor&);
    ~CZ5000AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    CZ5000AudioProcessor& processorRef;
    cz::HardwarePanel hardwarePanel;
    std::unique_ptr<juce::GenericAudioProcessorEditor> genericEditor;
};
