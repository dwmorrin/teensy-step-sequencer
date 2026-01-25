#include "OutputDriver.h"

IntervalTimer OutputDriver::_timer;

void OutputDriver::init()
{
  // Initialize all mapped pins as Outputs and set them off
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    pinMode(OUTPUT_MAP[i], OUTPUT);
    // Set to safe "Idle" state immediately
    digitalWrite(OUTPUT_MAP[i], TRIGGER_OFF);
  }
}

void OutputDriver::fireTriggers(uint16_t trackMask)
{
  // if no tracks need to fire (mask is 0), do nothing.
  if (trackMask == 0)
    return;

  // 1. Turn ON the triggers
  _writeHardware(trackMask, true);

  // 2. Set the timestamp to turn them OFF
  // Note: We use millis() for now. If you need tighter timing later,
  // we can easily swap this logic to use micros().
  _timer.begin(turnOffISR, PULSE_WIDTH_MS * 1000);
}

void OutputDriver::turnOffISR()
{
  _timer.end();
  _writeHardware(0xFFFF, false);
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
      // "state" is true if we want to Fire, false if we want to Rest.
      // We map this strictly to our Config constants.
      digitalWrite(OUTPUT_MAP[i], state ? TRIGGER_ON : TRIGGER_OFF);
    }
  }
}