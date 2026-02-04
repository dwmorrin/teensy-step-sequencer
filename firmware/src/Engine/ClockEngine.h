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

  // NEW: Allow manual firing (e.g. form UI) that respects the ISR timing
  void manualTrigger(uint16_t mask);

  static void onTick();

private:
  static ClockEngine *_instance;
  IntervalTimer _timer;

  SequencerModel &_model;
  OutputDriver &_driver;

  int _cachedBPM;

  volatile unsigned long _stepCounter;
  volatile unsigned long _pulseCounter;
  volatile unsigned long _stepInterval;

  // Triggers can be active even if sequencer is stopped (Manual Fire)
  volatile bool _triggersActive;
  volatile bool _running;

  void _handleTick();
  void _calculateInterval(int bpm);
};