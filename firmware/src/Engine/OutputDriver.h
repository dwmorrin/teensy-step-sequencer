#pragma once
#include <Arduino.h>
#include "Config.h"

class OutputDriver
{
public:
  void init();

  // Turns specific pins HIGH based on the mask
  void setTriggers(uint16_t trackMask);

  // Turns ALL trigger pins LOW
  void clearAllTriggers();

private:
  void _writeHardware(uint16_t mask, bool state);
};