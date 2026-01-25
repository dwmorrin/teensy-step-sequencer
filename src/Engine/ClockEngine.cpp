#include "ClockEngine.h"

ClockEngine::ClockEngine(SequencerModel &model, OutputDriver &driver)
    : _model(model), _driver(driver)
{
  _lastStepTime = 0;
  _cachedBPM = 0;
}

void ClockEngine::run()
{
  // 1. If we aren't playing, do nothing
  if (!_model.isPlaying())
    return;

  int targetBPM = _model.getBPM();
  if (targetBPM != _cachedBPM)
  {
    _cachedBPM = targetBPM;
    // calculate ms per step
    // (60,000 ms / BPM) / 4 steps per beat
    _stepInterval = (60'000 / _cachedBPM) / 4;
  }

  // 2. Check Time
  unsigned long currentMillis = millis();
  if (currentMillis - _lastStepTime >= _stepInterval)
  {
    _lastStepTime = currentMillis;

    // 3. Advance the Brain
    // This moves the cursor to the next step (and handles pattern wrapping)
    _model.advanceStep();

    // 4. Get Data for the NEW step
    int patID = _model.getPlayingPatternID();
    int step = _model.getCurrentStep();

    // Ask the model: "Which tracks are active for this specific moment?"
    uint16_t mask = _model.getTriggersForStep(patID, step);

    // 5. Fire Hardware
    if (mask > 0)
    {
      _driver.fireTriggers(mask);
    }
  }
}