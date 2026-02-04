#include "OutputDriver.h"

void OutputDriver::init()
{
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    pinMode(OUTPUT_MAP[i], OUTPUT);
    digitalWrite(OUTPUT_MAP[i], TRIGGER_OFF);
  }
}

void OutputDriver::setTriggers(uint16_t trackMask)
{
  if (trackMask == 0)
    return;
  _writeHardware(trackMask, true);
}

void OutputDriver::clearAllTriggers()
{
  _writeHardware(0xFFFF, false);
}

void OutputDriver::_writeHardware(uint16_t mask, bool state)
{
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    if (mask & (1 << i))
    {
      digitalWrite(OUTPUT_MAP[i], state ? TRIGGER_ON : TRIGGER_OFF);
    }
  }
}