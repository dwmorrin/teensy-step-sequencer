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
  // 1. TIMING OVERRIDE FOR DIAGNOSTICS
  if (_model.getPlayMode() == MODE_HARDWARE_TEST)
  {
    _stepInterval = 50; // Fast 50ms refresh for chaser

    // Force "Playing" logic so steps advance
    if (!_wasPlaying)
    {
      _wasPlaying = true;
      _lastStepTime = millis();
    }

    unsigned long currentMillis = millis();
    if (currentMillis - _lastStepTime >= _stepInterval)
    {
      _lastStepTime = currentMillis;
      _model.advanceStep();
      _handleStepFiring();
    }
    return; // Skip standard BPM logic
  }

  // 2. STANDARD OPERATION
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
  // OUTPUT OVERRIDE: Silence triggers during test
  if (_model.getPlayMode() == MODE_HARDWARE_TEST)
  {
    return;
  }

  int patID = _model.getPlayingPatternID();
  int step = _model.getCurrentStep();

  uint16_t mask = _model.getTriggersForStep(patID, step);

  // Only talk to hardware if there is something to say
  if (mask > 0)
  {
    _driver.fireTriggers(mask);
  }
}