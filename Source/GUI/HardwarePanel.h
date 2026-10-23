#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class CZ5000AudioProcessor; // global forward declaration (NICHT in namespace cz!)

namespace cz
{
class HardwarePanel : public juce::Component, private juce::Timer
{
public:
    explicit HardwarePanel (CZ5000AudioProcessor& proc);
    ~HardwarePanel() override;

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    CZ5000AudioProcessor& processor;

    juce::ComboBox inputBox, outputBox;
    juce::Label inputLabel  { {}, "MIDI In (Hardware):" };
    juce::Label outputLabel { {}, "MIDI Out (Hardware):" };
    juce::TextButton connectButton { "Verbinden" };
    juce::TextButton sendButton    { "Tone -> Hardware senden" };
    juce::ToggleButton liveSyncToggle { "Live-Sync aktiv" };
    juce::Label statusLabel;

    void refreshDeviceLists();
    void timerCallback() override;
};
} // namespace cz
