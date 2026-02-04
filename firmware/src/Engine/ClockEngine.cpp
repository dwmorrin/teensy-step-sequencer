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
  _isFirstStep = false; // Init
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

void ClockEngine::manualTrigger(uint16_t mask)
{
  _driver.setTriggers(mask);
  _pulseCounter = 0;
  _triggersActive = true;
}

void ClockEngine::_handleTick()
{
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

  if (_triggersActive)
  {
    _pulseCounter++;
    if (_pulseCounter >= PULSE_WIDTH_MS)
    {
      _driver.clearAllTriggers();
      _triggersActive = false;
    }
  }

  bool isPlaying = _model.isPlaying();

  // START EVENT
  if (isPlaying && !_running)
  {
    _running = true;
    _stepCounter = _stepInterval; // Force immediate execution
    _isFirstStep = true;          // <--- MARK AS FIRST STEP
  }
  // STOP EVENT
  else if (!isPlaying && _running)
  {
    _running = false;
  }

  if (!_running)
    return;

  // STEP LOGIC
  _stepCounter++;
  if (_stepCounter >= _stepInterval)
  {
    _stepCounter = 0;

    // Skip advance on the very first trigger
    if (_isFirstStep)
    {
      _isFirstStep = false;
      // Do not advance. Fire the step we are currently sitting on (Step 1).
    }
    else
    {
      _model.advanceStep();
    }

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