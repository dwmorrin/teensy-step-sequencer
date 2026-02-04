#pragma once
#include <Arduino.h>
#include "Config.h"
#include "Model/SequencerModel.h"
#include "OutputDriver.h"

class ClockEngine
{
public:
  ClockEngine(SequencerModel &model, OutputDriver &driver);
  void init();
  void update();
  void manualTrigger(uint16_t mask);
  static void onTick();

private:
  static ClockEngine *_instance;
  IntervalTimer _timer;
  SequencerModel &_model;
  OutputDriver &_driver;

  int _cachedBPM;

  // Timing State (Float for precision accumulation)
  volatile float _accumulatedTime;
  volatile float _tickInterval;

  volatile unsigned long _pulseCounter;
  volatile bool _triggersActive;
  volatile bool _running;
  volatile bool _isFirstTick;

  void _handleTick();
  void _calculateInterval(int bpm);

  // Helper to check per-track swing logic
  void _checkTriggers(int step, int tick);
};