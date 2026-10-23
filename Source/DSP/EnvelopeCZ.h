/*
  EnvelopeCZ.h
  ------------
  8-stufige Rate/Level-Huellkurve wie bei DCA/DCW/DCO der Casio CZ-Serie.
  "endStep" markiert den Sustain-Index: die Huellkurve laeuft bei NoteOn
  die Stufen 0..endStep durch und haelt danach den Level von endStep, bis
  NoteOff kommt. Danach laeuft sie durch die restlichen Stufen (Release).
*/
#pragma once
#include <array>
#include <algorithm>
#include <cmath>

namespace cz
{

class EnvelopeCZ
{
public:
    struct Stage
    {
        float rate  = 50.0f; // 0..99, 99 = am schnellsten (kurze Zeit)
        float level = 0.0f;  // 0..1 normalisiert
    };

    std::array<Stage, 8> stages {};
    int endStep = 7; // Sustain-Stufe (0..7)

    void setSampleRate (double sr) noexcept { sampleRate = sr > 0.0 ? sr : 44100.0; }

    void noteOn() noexcept
    {
        currentStage = 0;
        released = false;
        finished = false;
        startLevel = currentLevel;
        beginStage (0);
    }

    void noteOff() noexcept
    {
        released = true;
        // Nach dem Sustain-Punkt mit der naechsten Stufe weitermachen (Release).
        if (currentStage <= endStep)
        {
            currentStage = endStep + 1;
            if (currentStage > 7)
            {
                finished = true;
                return;
            }
            startLevel = currentLevel;
            beginStage (currentStage);
        }
    }

    /** Muss einmal pro Sample aufgerufen werden. Gibt den aktuellen Level (0..1) zurueck. */
    float process() noexcept
    {
        if (finished)
            return currentLevel;

        stageTime += 1.0 / sampleRate;
        float t = stageDuration > 0.0 ? (float) (stageTime / stageDuration) : 1.0f;
        t = std::clamp (t, 0.0f, 1.0f);
        currentLevel = startLevel + (targetLevel - startLevel) * t;

        if (t >= 1.0f)
        {
            currentLevel = targetLevel;

            if (! released && currentStage >= endStep)
            {
                // Sustain erreicht: Level halten, keine weiteren Stufen bis NoteOff.
                return currentLevel;
            }

            int nextStage = currentStage + 1;
            if (nextStage > 7)
            {
                finished = true;
                return currentLevel;
            }
            currentStage = nextStage;
            startLevel = currentLevel;
            beginStage (currentStage);
        }
        return currentLevel;
    }

    bool isActive() const noexcept { return ! finished; }
    float getCurrentLevel() const noexcept { return currentLevel; }

    void reset (float initialLevel = 0.0f) noexcept
    {
        currentLevel = initialLevel;
        startLevel = initialLevel;
        targetLevel = initialLevel;
        currentStage = 0;
        stageTime = 0.0;
        stageDuration = 0.0;
        released = false;
        finished = true;
    }

private:
    double sampleRate = 44100.0;
    int currentStage = 0;
    float currentLevel = 0.0f;
    float startLevel = 0.0f;
    float targetLevel = 0.0f;
    double stageTime = 0.0;
    double stageDuration = 0.0;
    bool released = false;
    bool finished = true;

    static constexpr double kMaxStageSeconds = 8.0;
    static constexpr double kMinStageSeconds = 0.0015;

    void beginStage (int stageIdx) noexcept
    {
        stageIdx = std::clamp (stageIdx, 0, 7);
        targetLevel = stages[(size_t) stageIdx].level;
        float rate = std::clamp (stages[(size_t) stageIdx].rate, 0.0f, 99.0f);
        // rate 99 -> kMinStageSeconds (schnell), rate 0 -> kMaxStageSeconds (langsam)
        double t = (double) rate / 99.0;
        stageDuration = kMaxStageSeconds + (kMinStageSeconds - kMaxStageSeconds) * t;
        stageTime = 0.0;
    }
};

} // namespace cz
