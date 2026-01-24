#include "OutputDriver.h"

OutputDriver::OutputDriver()
{
  _offTime = 0;
  _isActive = false;
}

void OutputDriver::init()
{
  // Initialize all mapped pins as Outputs and set them LOW
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    pinMode(OUTPUT_MAP[i], OUTPUT);
    digitalWrite(OUTPUT_MAP[i], LOW);
  }
}

void OutputDriver::fireTriggers(uint16_t trackMask)
{
  // Optimization: If no tracks need to fire (mask is 0), do nothing.
  if (trackMask == 0)
    return;

  // 1. Turn ON the physical pins requested by the mask
  _writeHardware(trackMask, true);

  // 2. Set the timestamp to turn them OFF
  // Note: We use millis() for now. If you need tighter timing later,
  // we can easily swap this logic to use micros().
  _offTime = millis() + PULSE_WIDTH_MS;
  _isActive = true;
}

void OutputDriver::process()
{
  // This is called every loop iteration.
  // It checks if we are currently holding a gate HIGH ("Active")
  // and if the pulse width time has expired.
  if (_isActive && millis() >= _offTime)
  {
    // Time is up! Turn everything OFF.
    // We pass a full mask (0xFFFF) to ensure every track is pulled LOW.
    _writeHardware(0xFFFF, false);
    _isActive = false;
  }
}

void OutputDriver::_writeHardware(uint16_t mask, bool state)
{
  // Iterate through our defined tracks
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    // Check if the specific bit for this track is set in the mask.
    // (1 << i) creates a bitmask for the current index (1, 2, 4, 8, etc.)
    // If the result is non-zero, the bit was ON.
    if (mask & (1 << i))
    {
      // Write to the physical pin mapped in Config.h
      digitalWrite(OUTPUT_MAP[i], state ? HIGH : LOW);
    }
  }
}