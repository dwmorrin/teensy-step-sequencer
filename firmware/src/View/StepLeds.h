#pragma once

#include <Arduino.h>
#include <SPI.h>

class StepLeds
{
public:
  StepLeds(uint8_t latchPin);

  // Hardware setup
  void begin();

  // Frame Buffer Manipulation
  void set(uint8_t index, bool state);
  void setAll(uint16_t state);
  void clear();

  // Core Update Methods
  void show(); // Fast, write-only (Standard Operation)

  // Diagnostics & Validation
  // Writes current buffer, returns what was PREVIOUSLY in the register
  uint16_t transfer(uint16_t data);

  // Performs a full write-read-compare cycle to verify hardware path
  bool selfTest();

private:
  uint8_t _latchPin;
  uint16_t _ledState; // The "Frame Buffer"
  SPISettings _spiSettings;
};