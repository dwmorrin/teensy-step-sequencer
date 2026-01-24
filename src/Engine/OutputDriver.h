#pragma once
#include <Arduino.h>
#include "Config.h"

class OutputDriver
{
public:
  OutputDriver();

  // Sets pin modes. Call this in setup().
  void init();

  // The main API: Pass a bitmask of tracks to fire.
  // Bit 0 = Track 1, Bit 1 = Track 2, etc.
  // Example: 0001 (1) fires Track 1. 0101 (5) fires Tracks 1 and 3.
  void fireTriggers(uint16_t trackMask);

  // Call this every loop iteration.
  // It checks if enough time has passed to turn the triggers OFF (LOW).
  void process();

private:
  unsigned long _offTime; // Timestamp when triggers should go LOW
  bool _isActive;         // Are triggers currently HIGH?

  // Internal helper to perform the actual digitalWrite (or Shift Register write)
  void _writeHardware(uint16_t mask, bool state);
};