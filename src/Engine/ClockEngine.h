#pragma once
#include <Arduino.h>
#include "Config.h"
#include "Model/SequencerModel.h"
#include "OutputDriver.h"

class ClockEngine
{
public:
  // Dependency Injection: The Clock needs to control both the Brain and the Hands
  ClockEngine(SequencerModel &model, OutputDriver &driver);

  void setBPM(int bpm);

  // Call this in the main loop. It handles the timing checks.
  void run();

private:
  SequencerModel &_model;
  OutputDriver &_driver;

  unsigned long _lastStepTime;
  unsigned long _stepInterval; // Milliseconds between steps
};