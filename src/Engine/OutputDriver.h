#pragma once
#include <Arduino.h>
#include "Config.h"

class OutputDriver
{
public:
  // Sets pin modes. Call this in setup().
  void init();

  // The main API: Pass a bitmask of tracks to fire.
  // Bit 0 = Track 1, Bit 1 = Track 2, etc.
  // Example: 0001 (1) fires Track 1. 0101 (5) fires Tracks 1 and 3.
  void fireTriggers(uint16_t trackMask);

  static void turnOffISR();

private:
  static IntervalTimer _timer;
  // Internal helper
  static void _writeHardware(uint16_t mask, bool state);
};