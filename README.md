# CZ5000VST — Virtueller Casio CZ-5000 (VST3 + Standalone)

Ein JUCE-basiertes Synth-Plugin, das die Phase-Distortion-Synthese der
Casio CZ-Serie (CZ-101 / CZ-1000 / CZ-5000) nachbildet **und** per USB-MIDI
direkt mit einem echten CZ-Geraet kommunizieren kann (Tone-Dumps senden
und empfangen).

## Features

- Zwei "Lines" (Line 1 / Line 2) mit je einem Phase-Distortion-Oszillator
  (8 Wellenformen), DCA-/DCW-/DCO-Huellkurven (8 Stufen, Rate/Level, mit
  Sustain-Punkt wie beim Original).
- Line-Mix-Modi: Line1, Line2, Mix, Ringmodulation.
- **HardwareBridge**: oeffnet unabhaengig vom DAW-MIDI-Routing eine eigene
  USB-MIDI-Verbindung zu einem echten CZ-Geraet. Sendet komplette
  Tone-Dumps (SysEx) an die Hardware und empfaengt Dumps, die am Geraet
  ausgeloest werden, um die Plugin-Parameter live zu synchronisieren.
- Baut als **VST3** und als **Standalone**-App aus derselben Codebasis.
- GitHub-Actions-CI baut automatisch fuer Windows, macOS und Linux.

## Wichtige Hinweise / bekannte Einschraenkungen (bitte lesen!)

1. **DSP ist eine eigenstaendige Annaeherung, kein Bit-Klon.** Die
   Phase-Distortion-Implementierung in `Source/DSP/PDOscillator.h` ist
   eine ORIGINALE Umsetzung des oeffentlich bekannten PD-Prinzips. Sie
   verwendet keinerlei Casio-ROM-Daten, keinen proprietaeren Code und
   kein Patentmaterial. Der Klang wird dem Original aehneln, aber nicht
   sample-exakt sein, da Casios interne Lookup-Tabellen nicht oeffentlich
   dokumentiert sind.
2. **SysEx-Struktur ist verifiziert.** Frame-Aufbau (264 Byte,
   `F0 44 00 00 [ch] 20 [tone] [256 Nibbles] F7`), Nibble-Packung
   (Low-Nibble zuerst) und Feldreihenfolge (DCA -> DCW -> DCO) wurden
   gegen echte CZ-5000-Dumps verifiziert (Round-Trip decode->encode
   bitgenau) und stimmen mit den dokumentierten Quellen
   `github.com/ajwills72/cz101` und `kasploosh.com/cz/11800-spelunking`
   ueberein. Siehe `Source/MIDI/CZSysEx.h` fuer alle Feld-Offsets.
3. **Waveform-Byte-Mapping ist ein Platzhalter.** Die genaue Bit-Belegung
   von Wave1/Wave2/Window/Modulation innerhalb der `mfw`/`sfw`-Bytes
   wurde nicht vollstaendig verifiziert. Aktuell wird der Wellenform-Index
   (0-7) direkt ins Low-Byte geschrieben — intern konsistent, aber
   moeglicherweise NICHT 1:1 kompatibel mit echten Hardware-Patches beim
   Hin- und Herladen. Vor dem produktiven Hardware-Sync bitte mit einem
   bekannten Hardware-Tone gegentesten und ggf. in
   `PluginProcessor::buildToneFromParameters()` /
   `applyToneToParameters()` anpassen.
4. **"Live-Sync" sendet volle Tone-Dumps, keine Einzelparameter.** Die
   CZ-Serie akzeptiert laut Dokumentation nur komplette Tone-Dumps, kein
   granulares Pro-Parameter-Echtzeitprotokoll. Das Plugin sendet daher
   bei aktivem "Live-Sync" gedrosselt (max. 8x/Sekunde) einen kompletten
   aktuellen Tone-Dump an die Hardware — das fuehlt sich in der Praxis
   wie Echtzeitsteuerung an.
5. **In dieser Umgebung nicht kompiliert.** Der Code wurde ohne Internet-
   zugang und ohne installierten C++-Compiler/JUCE in einer Sandbox
   geschrieben und daher NICHT gegen JUCE kompiliert. Die SysEx-Logik
   (`CZSysEx.h/.cpp`) ist reiner Standard-C++17 ohne JUCE-Abhaengigkeit
   und wurde logisch gegen die Python/JS-Referenzimplementierung
   gegengeprueft. Beim ersten Build (siehe unten) koennen kleinere
   API-Abweichungen zur verwendeten JUCE-Version auftreten — meist
   reichen 1-2 Zeilen Fix. Poste mir gerne die Fehlermeldung, dann
   helfe ich beim Beheben.

## Build-Anleitung

Voraussetzungen: CMake >= 3.22, ein C++17-Compiler, Internetzugang (JUCE
wird automatisch via FetchContent heruntergeladen), bei Linux zusaetzliche
Systembibliotheken (siehe `.github/workflows/build.yml`).

```bash
git clone <dein-repo-url>
cd CZ5000VST
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

Nach erfolgreichem Build findest du:
- **VST3**: `build/CZ5000VST_artefacts/Release/VST3/CZ-5000 Virtual.vst3`
  (Pfad kann je nach JUCE/CMake-Version leicht variieren — einfach nach
  `*.vst3` suchen).
- **Standalone**: `build/CZ5000VST_artefacts/Release/Standalone/`

Fuer FL Studio: VST3-Ordner in den von FL Studio gescannten Plugin-Pfad
kopieren (z.B. `C:\Program Files\Common Files\VST3`) und in FL Studio
"Scan for new plugins" ausfuehren.

### Alternative: JUCE als Git-Submodule statt FetchContent

Falls du keinen Internetzugriff beim Konfigurieren willst:
```bash
git submodule add https://github.com/juce-framework/JUCE.git JUCE
```
und in `CMakeLists.txt` den `FetchContent`-Block durch
`add_subdirectory(JUCE)` ersetzen.

## Hardware-Verbindung nutzen

1. CZ-5000 per USB-MIDI-Interface mit dem Rechner verbinden.
2. Im Plugin (oberer Bereich "HardwarePanel"): MIDI-In/Out-Geraet
   auswaehlen, "Verbinden" klicken.
3. "Tone -> Hardware senden" sendet den aktuellen Plugin-Sound als
   Tone-Dump an den gewaehlten Tone-Slot (einstellbar ueber den
   Parameter "Hardware Tone Slot").
4. "Live-Sync aktiv" sendet laufend (gedrosselt) den aktuellen Sound an
   die Hardware, waehrend du Parameter im Plugin aenderst.
5. Wenn du am CZ-Geraet selbst einen Tone-Dump ausloest (Data-Transmit-
   Funktion am Panel), empfaengt das Plugin diesen automatisch und
   aktualisiert seine eigenen Parameter entsprechend.

## Projektstruktur

```
CZ5000VST/
├── CMakeLists.txt
├── Source/
│   ├── PluginProcessor.h/.cpp   # AudioProcessor, Parameter-Handling, Hardware-Sync
│   ├── PluginEditor.h/.cpp      # GUI-Host (GenericAudioProcessorEditor + HardwarePanel)
│   ├── Parameters.h/.cpp        # APVTS Parameter-Layout (alle Synth-Parameter)
│   ├── DSP/
│   │   ├── PDOscillator.h       # Phase-Distortion-Oszillator (8 Wellenformen)
│   │   ├── EnvelopeCZ.h         # 8-Stufen Rate/Level-Huellkurve mit Sustain
│   │   ├── CZSound.h / CZVoice.h/.cpp  # Synthesiser-Voice (2 Lines, Mix-Modi)
│   ├── MIDI/
│   │   ├── CZSysEx.h/.cpp       # SysEx Encoder/Decoder (verifiziert)
│   │   └── HardwareBridge.h     # Direkte USB-MIDI-Verbindung zur Hardware
│   └── GUI/
│       └── HardwarePanel.h/.cpp # MIDI-Device-Auswahl, Sync-Steuerung
└── .github/workflows/build.yml  # CI: Windows/macOS/Linux Build + Artefakte
```

## Lizenz

GPLv3 (siehe `LICENSE.md`) — bedingt durch die kostenlose JUCE-Lizenz.
Fuer eine proprietaere/kommerzielle Distribution ist eine kommerzielle
JUCE-Lizenz von juce.com erforderlich.

## Quellen / Credits

Die SysEx-Feldstruktur wurde verifiziert gegen drei echte CZ-5000-Dumps
und abgeglichen mit:
- `github.com/ajwills72/cz101` (offizielle Feldtabellen-Doku)
- `kasploosh.com/cz/11800-spelunking` (Nibble-Packung, Byte-Offset-Beispiele)
