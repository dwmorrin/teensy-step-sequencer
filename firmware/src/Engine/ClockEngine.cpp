#include "ClockEngine.h"

// 96 PPQN Constants
#define PPQN 96
#define TICKS_PER_STEP 24
#define MAX_SWING_TICKS 12

ClockEngine *ClockEngine::_instance = nullptr;

ClockEngine::ClockEngine(SequencerModel &model, OutputDriver &driver)
    : _model(model), _driver(driver)
{
  _instance = this;
  _cachedBPM = 0;
  _accumulatedTime = 0;
  _pulseCounter = 0;
  _triggersActive = false;
  _running = false;
  _isFirstTick = false;
}

void ClockEngine::init()
{
  _timer.begin(onTick, 500);
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
    return;
  }

  // PULSE MANAGEMENT
  if (_triggersActive)
  {
    _pulseCounter++;
    if (_pulseCounter >= 30)
    {
      _driver.clearAllTriggers();
      _triggersActive = false;
    }
  }

  bool isPlaying = _model.isPlaying();

  if (isPlaying && !_running)
  {
    _running = true;
    _accumulatedTime = _tickInterval;
    _isFirstTick = true;
  }
  else if (!isPlaying && _running)
  {
    _running = false;
  }

  if (!_running)
    return;

  // TIME ACCUMULATION
  _accumulatedTime += 0.5f;

  if (_accumulatedTime >= _tickInterval)
  {
    _accumulatedTime -= _tickInterval;

    // --- CORE PPQN LOGIC ---
    if (_isFirstTick)
    {
      _isFirstTick = false;
      _checkTriggers(_model.getCurrentStep(), 0);
    }
    else
    {
      _model.advanceTick();

      // Quantization Check
      if (_model.getCurrentTick() == 0)
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
          _model.applyPendingPattern();
      }

      // Fire Triggers
      _checkTriggers(_model.getCurrentStep(), _model.getCurrentTick());
    }
  }
}

void ClockEngine::_checkTriggers(int step, int tick)
{
  int patID = _model.getPlayingPatternID();
  uint16_t fireMask = 0;

  for (int t = 0; t < NUM_TRACKS; t++)
  {
    uint8_t swingAmount = _model.getPlayingTrackSwing(t);

    int targetTick = 0;
    // Apply delay only to ODD steps (the "ands" of the beat)
    if (step % 2 != 0)
    {
      targetTick = (swingAmount * MAX_SWING_TICKS) / 100;
    }

    if (tick == targetTick)
    {
      uint16_t trackMask = (1 << t);
      if (_model.getTriggersForStep(patID, step) & trackMask)
      {
        fireMask |= trackMask;
      }
    }
  }

  if (fireMask > 0)
  {
    _driver.setTriggers(fireMask);
    _triggersActive = true;
    _pulseCounter = 0;
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
  _tickInterval = 60000.0f / (bpm * 96.0f);
}