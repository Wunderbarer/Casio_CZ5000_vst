#include "PluginEditor.h"

CZ5000AudioProcessorEditor::CZ5000AudioProcessorEditor (CZ5000AudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), hardwarePanel (p)
{
    genericEditor = std::make_unique<juce::GenericAudioProcessorEditor> (p);
    addAndMakeVisible (hardwarePanel);
    addAndMakeVisible (*genericEditor);
    setResizable (true, true);
    setSize (760, 820);
}

CZ5000AudioProcessorEditor::~CZ5000AudioProcessorEditor() {}

void CZ5000AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkslategrey);
}

void CZ5000AudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    hardwarePanel.setBounds (area.removeFromTop (110));
    genericEditor->setBounds (area);
}
