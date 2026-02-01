#pragma once
#include <Arduino.h>

class AnalogInput
{
public:
  // threshold: Raw ADC variance required to trigger an update (default 8)
  AnalogInput(int pin, int minOut, int maxOut, int threshold = 8);

  // Call this in your main loop or processInput
  // Returns true if the value has changed significantly
  bool update();

  // Returns the current smoothed/mapped value
  int getValue() const;

  // Returns the raw ADC reading (useful for debugging)
  int getRaw() const;

private:
  int _pin;
  int _minOut;
  int _maxOut;
  int _threshold;

  int _lastStableRaw; // The raw value at the last confirmed update
  int _currentMapped; // The calculate output value
};