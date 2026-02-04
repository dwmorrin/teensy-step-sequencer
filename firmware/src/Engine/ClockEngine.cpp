#include "ClockEngine.h"

ClockEngine *ClockEngine::_instance = nullptr;

ClockEngine::ClockEngine(SequencerModel &model, OutputDriver &driver)
    : _model(model), _driver(driver)
{
  _instance = this;
  _cachedBPM = 0;
  _stepCounter = 0;
  _pulseCounter = 0;
  _triggersActive = false;
  _running = false;
}

void ClockEngine::init()
{
  _timer.begin(onTick, 1000);
}

void ClockEngine::onTick()
{
  if (_instance)
    _instance->_handleTick();
}

// NEW: Manual Trigger Injection
void ClockEngine::manualTrigger(uint16_t mask)
{
  // Fire immediately
  _driver.setTriggers(mask);

  // Tell ISR to count 15ms and then kill it
  // (Safe to set these volatiles from outside ISR for this simple flag logic)
  _pulseCounter = 0;
  _triggersActive = true;
}

void ClockEngine::_handleTick()
{
  // 1. HARDWARE TEST OVERRIDE
  if (_model.getPlayMode() == MODE_HARDWARE_TEST)
  {
    _stepCounter++;
    if (_stepCounter >= 50)
    {
      _stepCounter = 0;
      _model.advanceStep();
    }
    return;
  }

  // 2. PULSE MANAGEMENT (Priority High)
  // Run this BEFORE checking _running.
  // This ensures manual triggers are turned off even if Sequencer is Stopped.
  if (_triggersActive)
  {
    _pulseCounter++;
    if (_pulseCounter >= PULSE_WIDTH_MS)
    {
      _driver.clearAllTriggers();
      _triggersActive = false;
    }
  }

  // 3. CHECK SEQUENCER STATE
  bool isPlaying = _model.isPlaying();

  if (isPlaying && !_running)
  {
    _running = true;
    _stepCounter = _stepInterval; // Force immediate trigger
  }
  else if (!isPlaying && _running)
  {
    _running = false;
    // Do not clear triggers here; let Pulse Management handle the tail
  }

  if (!_running)
    return;

  // 4. STEP ADVANCEMENT
  _stepCounter++;
  if (_stepCounter >= _stepInterval)
  {
    _stepCounter = 0;

    _model.advanceStep();

    int patID = _model.getPlayingPatternID();
    int step = _model.getCurrentStep();
    uint16_t mask = _model.getTriggersForStep(patID, step);

    if (mask > 0)
    {
      _driver.setTriggers(mask);
      _triggersActive = true;
      _pulseCounter = 0;
    }
  }
}

void ClockEngine::update()
{
  int targetBPM = _model.getBPM();
  if (targetBPM != _cachedBPM)
  {
    _cachedBPM = targetBPM;
    _calculateInterval(_cachedBPM);
  }
}

void ClockEngine::_calculateInterval(int bpm)
{
  if (bpm <= 0)
    bpm = 120;
  _stepInterval = (60000 / bpm) / 4;
}