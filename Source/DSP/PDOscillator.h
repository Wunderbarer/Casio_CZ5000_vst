/*
  PDOscillator.h
  --------------
  Phase-Distortion-Oszillator, angelehnt an das von Casio in der CZ-Serie
  verwendete Syntheseprinzip.

  WICHTIGER HINWEIS ZUR GENAUIGKEIT:
  Dies ist eine ORIGINALE, eigenstaendige Implementierung des allgemein
  bekannten und oeffentlich dokumentierten "Phase Distortion"-Prinzips
  (Phasen-Verzerrung einer linearen Saegezahn-Rampe vor einem Sinus-Lookup).
  Es werden KEINE Casio-ROM-Daten, keine Patente und kein proprietaerer
  Code von Casio verwendet oder reproduziert. Der Klang wird dem Original
  nahekommen, aber NICHT bit-/sample-exakt sein, da Casios genaue interne
  Lookup-Tabellen nicht oeffentlich sind. Die 8 Wellenform-Typen entsprechen
  den allgemein unter diesem Namen bekannten CZ-Wellenformen (Sage, Rechteck,
  Puls, Doppel-Sinus, Sage-Puls, drei Resonanz-Typen).
*/
#pragma once
#include <cmath>
#include <functional>
#include <algorithm>

namespace cz
{

enum class CZWaveform
{
    Saw = 0,
    Square,
    Pulse,
    DoubleSine,
    SawPulse,
    Reso1,   // resonanter Sage-Typ
    Reso2,   // resonanter Rechteck/Dreieck-Typ
    Reso3,   // resonanter Trapez-Typ
    NumWaveforms
};

class PDOscillator
{
public:
    void setSampleRate (double sr) noexcept { sampleRate = sr > 0.0 ? sr : 44100.0; }

    void setFrequency (float hz) noexcept
    {
        frequency = hz;
        phaseInc = (double) hz / sampleRate;
    }

    void setWaveform (CZWaveform w) noexcept { waveform = w; }

    /** window: 0..1, entspricht dem DCW-Huellkurvenwert (Phasendistortions-Betrag). */
    void setWindow (float window01) noexcept { window = std::clamp (window01, 0.0f, 1.0f); }

    void setResonanceRatio (float ratioHarmonics) noexcept { resoRatio = std::max (1.0f, ratioHarmonics); }

    void resetPhase (double p = 0.0) noexcept { phase = p; }

    /** Erzeugt ein Sample im Bereich [-1, 1] und schreitet die Phase fort. */
    float render() noexcept
    {
        float out = computeSample ((float) phase);
        phase += phaseInc;
        if (phase >= 1.0) phase -= 1.0;
        return out;
    }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    double phaseInc = 0.0;
    float frequency = 440.0f;
    float window = 0.5f;
    float resoRatio = 4.0f;
    CZWaveform waveform = CZWaveform::Saw;

    static constexpr float kTwoPi = 6.283185307179586f;

    // Zwei-Segment-Phasenverzerrung: klassische PD-Technik.
    // breakpoint in (0,1) exklusiv. Ergebnis 0..1 -> wird spaeter *2pi an sin() uebergeben.
    static float warpTwoSegment (float ph, float breakpoint) noexcept
    {
        breakpoint = std::clamp (breakpoint, 0.001f, 0.999f);
        if (ph < breakpoint)
            return 0.5f * ph / breakpoint;
        return 0.5f + 0.5f * (ph - breakpoint) / (1.0f - breakpoint);
    }

    float computeSample (float ph) const noexcept
    {
        switch (waveform)
        {
            case CZWaveform::Saw:
            {
                // window 0 -> reiner Sinus (breakpoint=0.5), window 1 -> harte Saege
                float bp = juceMap (window, 0.0f, 1.0f, 0.5f, 0.02f);
                float warped = warpTwoSegment (ph, bp);
                return std::sin (kTwoPi * warped);
            }
            case CZWaveform::Square:
            {
                // zwei gespiegelte Segmente -> rechteckartiger Spektralgehalt
                float bp = juceMap (window, 0.0f, 1.0f, 0.5f, 0.05f);
                float half = ph < 0.5f ? ph * 2.0f : (ph - 0.5f) * 2.0f;
                float warped = warpTwoSegment (half, bp);
                float s = std::sin (kTwoPi * warped);
                return ph < 0.5f ? s : -s;
            }
            case CZWaveform::Pulse:
            {
                // asymmetrische Pulsbreite gesteuert durch window
                float duty = juceMap (window, 0.0f, 1.0f, 0.5f, 0.05f);
                float bp = duty;
                float warped = warpTwoSegment (ph, bp);
                return std::sin (kTwoPi * warped) * (ph < duty ? 1.0f : 0.6f);
            }
            case CZWaveform::DoubleSine:
            {
                // zwei Sinusschwingungen pro Zyklus, zweite Haelfte amplitudenmoduliert
                float s1 = std::sin (kTwoPi * (ph * 2.0f));
                float env = ph < 0.5f ? 1.0f : (1.0f - window);
                return s1 * env;
            }
            case CZWaveform::SawPulse:
            {
                // Crossfade zwischen Saw- und Pulse-Charakter
                float bpSaw = 0.5f;
                float saw = std::sin (kTwoPi * warpTwoSegment (ph, bpSaw));
                float duty = 0.25f;
                float pulse = std::sin (kTwoPi * warpTwoSegment (ph, duty)) * (ph < duty ? 1.0f : 0.5f);
                return saw * (1.0f - window) + pulse * window;
            }
            case CZWaveform::Reso1:
                return renderResonant (ph, [] (float p) { return 1.0f - p; }); // Saegezahn-Fenster
            case CZWaveform::Reso2:
                return renderResonant (ph, [] (float p) { return p < 0.5f ? p * 2.0f : (1.0f - p) * 2.0f; }); // Dreieck-Fenster
            case CZWaveform::Reso3:
                return renderResonant (ph, [] (float p) {
                    // Trapez-Fenster
                    if (p < 0.25f) return p * 4.0f;
                    if (p > 0.75f) return (1.0f - p) * 4.0f;
                    return 1.0f;
                });
            default:
                return std::sin (kTwoPi * ph);
        }
    }

    // Resonante PD-Wellen: traegerfrequenz = resoRatio * Grundfrequenz,
    // amplitudenmoduliert mit einer vom window-Parameter gesteuerten Fensterfunktion.
    float renderResonant (float ph, const std::function<float(float)>& windowShape) const noexcept
    {
        float carrierPhase = std::fmod (ph * resoRatio, 1.0f);
        float carrier = std::sin (kTwoPi * carrierPhase);
        float w = windowShape (ph);
        // window-Parameter steuert zusaetzlich die "Schaerfe" des Fensters
        float sharpened = std::pow (std::max (0.0f, w), juceMap (window, 0.0f, 1.0f, 0.3f, 3.0f));
        return carrier * sharpened;
    }

    static float juceMap (float v, float inMin, float inMax, float outMin, float outMax) noexcept
    {
        if (inMax - inMin == 0.0f) return outMin;
        float t = (v - inMin) / (inMax - inMin);
        return outMin + t * (outMax - outMin);
    }
};

} // namespace cz
