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
  _isFirstStep = false;
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

  // 2. PULSE MANAGEMENT
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
    _isFirstStep = true;          // Mark first step
  }
  else if (!isPlaying && _running)
  {
    _running = false;
  }

  if (!_running)
    return;

  // 4. STEP ADVANCEMENT
  _stepCounter++;
  if (_stepCounter >= _stepInterval)
  {
    _stepCounter = 0;

    if (_isFirstStep)
    {
      _isFirstStep = false;
      // Do not advance on first step, just fire.
    }
    else
    {
      _model.advanceStep();

      // --- QUANTIZED TRANSITION LOGIC ---
      // Only apply pending patterns at specific intervals
      if (_model.getPlayMode() == MODE_PATTERN_LOOP)
      {
        int currentStep = _model.getCurrentStep();
        QuantizationMode q = _model.getQuantization();
        bool readyToSwitch = false;

        switch (q)
        {
        case Q_BAR:
          readyToSwitch = (currentStep == 0);
          break;
        case Q_QUARTER:
          readyToSwitch = (currentStep % 4 == 0);
          break;
        case Q_EIGHTH:
          readyToSwitch = (currentStep % 2 == 0);
          break;
        case Q_INSTANT:
          readyToSwitch = true;
          break;
        }

        if (readyToSwitch)
        {
          _model.applyPendingPattern();
        }
      }
      // ---------------------------------------
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