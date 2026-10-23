#include "HardwarePanel.h"
#include "../PluginProcessor.h"
#include "../Parameters.h"
#include "../MIDI/HardwareBridge.h"

namespace cz
{

HardwarePanel::HardwarePanel (CZ5000AudioProcessor& proc) : processor (proc)
{
    addAndMakeVisible (inputLabel);
    addAndMakeVisible (outputLabel);
    addAndMakeVisible (inputBox);
    addAndMakeVisible (outputBox);
    addAndMakeVisible (connectButton);
    addAndMakeVisible (sendButton);
    addAndMakeVisible (liveSyncToggle);
    addAndMakeVisible (statusLabel);

    refreshDeviceLists();

    connectButton.onClick = [this]
    {
        auto ins  = HardwareBridge::getAvailableInputs();
        auto outs = HardwareBridge::getAvailableOutputs();
        int inIdx  = inputBox.getSelectedItemIndex();
        int outIdx = outputBox.getSelectedItemIndex();
        if (juce::isPositiveAndBelow (inIdx, ins.size()))
            processor.hardwareBridge.openInput (ins[inIdx]);
        if (juce::isPositiveAndBelow (outIdx, outs.size()))
            processor.hardwareBridge.openOutput (outs[outIdx]);
    };

    sendButton.onClick = [this] { processor.sendCurrentToneToHardware(); };

    liveSyncToggle.onClick = [this]
    {
        if (auto* p = processor.apvts.getParameter (ParamIDs::liveSync))
            p->setValueNotifyingHost (liveSyncToggle.getToggleState() ? 1.0f : 0.0f);
    };

    startTimerHz (2);
}

HardwarePanel::~HardwarePanel() { stopTimer(); }

void HardwarePanel::refreshDeviceLists()
{
    inputBox.clear();
    outputBox.clear();
    auto ins  = HardwareBridge::getAvailableInputs();
    auto outs = HardwareBridge::getAvailableOutputs();
    for (int i = 0; i < ins.size(); ++i)
        inputBox.addItem (ins[i].name, i + 1);
    for (int i = 0; i < outs.size(); ++i)
        outputBox.addItem (outs[i].name, i + 1);
    if (ins.size() > 0)  inputBox.setSelectedItemIndex (0);
    if (outs.size() > 0) outputBox.setSelectedItemIndex (0);
}

void HardwarePanel::timerCallback()
{
    juce::String status = "Hardware: ";
    status << (processor.hardwareBridge.isInputOpen()  ? "IN verbunden" : "IN offen");
    status << " / ";
    status << (processor.hardwareBridge.isOutputOpen() ? "OUT verbunden" : "OUT offen");
    statusLabel.setText (status, juce::dontSendNotification);
}

void HardwarePanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.3f));
}

void HardwarePanel::resized()
{
    auto area = getLocalBounds().reduced (6);
    auto row1 = area.removeFromTop (28);
    inputLabel.setBounds (row1.removeFromLeft (140));
    inputBox.setBounds (row1.removeFromLeft (200));
    row1.removeFromLeft (10);
    outputLabel.setBounds (row1.removeFromLeft (150));
    outputBox.setBounds (row1.removeFromLeft (200));

    area.removeFromTop (6);
    auto row2 = area.removeFromTop (28);
    connectButton.setBounds (row2.removeFromLeft (120));
    row2.removeFromLeft (10);
    sendButton.setBounds (row2.removeFromLeft (200));
    row2.removeFromLeft (10);
    liveSyncToggle.setBounds (row2);

    area.removeFromTop (6);
    statusLabel.setBounds (area.removeFromTop (20));
}

} // namespace cz
