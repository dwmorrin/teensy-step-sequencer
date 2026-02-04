#include "OutputDriver.h"
#include "Config.h"

void OutputDriver::init()
{
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    pinMode(OUTPUT_MAP[i], OUTPUT);
    digitalWrite(OUTPUT_MAP[i], TRIGGER_OFF);
  }
}

void OutputDriver::setTriggers(uint16_t mask)
{
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    // Check if bit 'i' is set
    if ((mask >> i) & 1)
    {
      digitalWrite(OUTPUT_MAP[i], TRIGGER_ON);
    }
  }
}

void OutputDriver::clearAllTriggers()
{
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    digitalWrite(OUTPUT_MAP[i], TRIGGER_OFF);
  }
}