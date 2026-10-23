/*
  HardwareBridge.h
  ----------------
  Spricht unabhaengig vom DAW-MIDI-Routing direkt per USB-MIDI mit einer
  echten Casio CZ-101/1000/5000: sendet Tone-Dumps (volle Parametersaetze)
  und empfaengt Tone-Dumps, die am Geraet ausgeloest werden (z.B. per
  "DATA TRANSMIT" / "DUMP" Taste am Panel).

  WICHTIG: Die CZ-Serie akzeptiert laut Doku nur VOLLSTAENDIGE Tone-Dumps,
  kein granulares Echtzeit-CC-Pro-Parameter-Protokoll. "Live-Sync" bedeutet
  hier: bei Aenderung wird (gedrosselt) ein kompletter, aktueller Tone-Dump
  an die Hardware gesendet, was in der Praxis wie Echtzeit-Steuerung wirkt.
*/
#pragma once
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_events/juce_events.h>
#include <functional>
#include <memory>
#include "CZSysEx.h"

namespace cz
{

class HardwareBridge : public juce::MidiInputCallback
{
public:
    HardwareBridge() = default;
    ~HardwareBridge() override { closeInput(); closeOutput(); }

    static juce::Array<juce::MidiDeviceInfo> getAvailableInputs()  { return juce::MidiInput::getAvailableDevices(); }
    static juce::Array<juce::MidiDeviceInfo> getAvailableOutputs() { return juce::MidiOutput::getAvailableDevices(); }

    bool openInput (const juce::MidiDeviceInfo& info)
    {
        closeInput();
        midiInput = juce::MidiInput::openDevice (info.identifier, this);
        if (midiInput != nullptr) { midiInput->start(); return true; }
        return false;
    }

    bool openOutput (const juce::MidiDeviceInfo& info)
    {
        closeOutput();
        midiOutput = juce::MidiOutput::openDevice (info.identifier);
        return midiOutput != nullptr;
    }

    void closeInput()  { if (midiInput != nullptr)  { midiInput->stop(); midiInput.reset(); } }
    void closeOutput() { midiOutput.reset(); }

    bool isInputOpen()  const noexcept { return midiInput  != nullptr; }
    bool isOutputOpen() const noexcept { return midiOutput != nullptr; }

    /** Sendet einen vollstaendigen Tone-Dump an das echte Geraet. */
    void sendTone (const CZTone& tone, int toneNum, int channel)
    {
        if (midiOutput == nullptr) return;
        auto frame = buildSysEx (toneNum, tone, channel);
        // JUCE::createSysExMessage erwartet die Nutzdaten OHNE F0/F7 (werden automatisch ergaenzt).
        auto msg = juce::MidiMessage::createSysExMessage (frame.data() + 1, (int) frame.size() - 2);
        midiOutput->sendMessageNow (msg);
    }

    /** Wird auf dem Message-Thread aufgerufen, sobald ein gueltiger CZ Tone-Dump ankommt. */
    std::function<void (const ParsedFrame&)> onToneReceived;

private:
    std::unique_ptr<juce::MidiInput>  midiInput;
    std::unique_ptr<juce::MidiOutput> midiOutput;

    void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message) override
    {
        if (! message.isSysEx())
            return;

        std::vector<uint8_t> frame;
        frame.reserve ((size_t) message.getSysExDataSize() + 2);
        frame.push_back (0xF0);
        const auto* data = message.getSysExData();
        for (int i = 0; i < message.getSysExDataSize(); ++i)
            frame.push_back ((uint8_t) data[i]);
        frame.push_back (0xF7);

        try
        {
            auto parsed = parseSysEx (frame);
            if (onToneReceived)
            {
                auto callback = onToneReceived;
                juce::MessageManager::callAsync ([callback, parsed] { callback (parsed); });
            }
        }
        catch (const std::exception&)
        {
            // Kein gueltiger CZ-Tone-Dump (z.B. anderes SysEx) -> ignorieren.
        }
    }
};

} // namespace cz
