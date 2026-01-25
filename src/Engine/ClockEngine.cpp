#include "ClockEngine.h"

ClockEngine::ClockEngine(SequencerModel &model, OutputDriver &driver)
    : _model(model), _driver(driver)
{
  _lastStepTime = 0;
  _cachedBPM = 0;
  _wasPlaying = false;
}

void ClockEngine::run()
{
  int targetBPM = _model.getBPM();
  if (targetBPM != _cachedBPM)
  {
    _cachedBPM = targetBPM;
    // calculate ms per step
    // (60,000 ms / BPM) / 4 steps per beat
    _stepInterval = (60'000 / _cachedBPM) / 4;
  }

  bool isPlaying = _model.isPlaying();
  if (!isPlaying)
  {
    _wasPlaying = false;
    return;
  }

  if (isPlaying && !_wasPlaying)
  {
    _wasPlaying = true;
    _handleStepFiring();
    _lastStepTime = millis();
    return;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - _lastStepTime >= _stepInterval)
  {
    _lastStepTime = currentMillis;
    _model.advanceStep();
    _handleStepFiring();
  }
}

void ClockEngine::_handleStepFiring()
{
  int patID = _model.getPlayingPatternID();
  int step = _model.getCurrentStep();

  uint16_t mask = _model.getTriggersForStep(patID, step);

  // Only talk to hardware if there is something to say
  if (mask > 0)
  {
    _driver.fireTriggers(mask);
  }
}
