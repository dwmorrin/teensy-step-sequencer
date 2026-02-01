#include "StepLeds.h"

// 2MHz is plenty fast, MSB First is standard for 595s
StepLeds::StepLeds(uint8_t latchPin)
    : _latchPin(latchPin), _ledState(0), _spiSettings(2000000, MSBFIRST, SPI_MODE0)
{
}

void StepLeds::begin()
{
  pinMode(_latchPin, OUTPUT);
  digitalWrite(_latchPin, HIGH); // Default idle state
  SPI.begin();
}

void StepLeds::set(uint8_t index, bool state)
{
  if (index >= 16)
    return; // Safety check

  if (state)
  {
    _ledState |= (1 << index);
  }
  else
  {
    _ledState &= ~(1 << index);
  }
}

void StepLeds::setAll(uint16_t state)
{
  _ledState = state;
}

void StepLeds::clear()
{
  _ledState = 0;
}

// Fast update - Blind fire
void StepLeds::show()
{
  SPI.beginTransaction(_spiSettings);
  digitalWrite(_latchPin, LOW);
  SPI.transfer16(_ledState); // Teensy optimized 16-bit transfer
  digitalWrite(_latchPin, HIGH);
  SPI.endTransaction();
}

// Diagnostic transfer - Returns the data that was shifted OUT
uint16_t StepLeds::transfer(uint16_t data)
{
  uint16_t received = 0;

  SPI.beginTransaction(_spiSettings);
  digitalWrite(_latchPin, LOW);
  received = SPI.transfer16(data);
  digitalWrite(_latchPin, HIGH);
  SPI.endTransaction();

  return received;
}

// Validates the full loop: MOSI -> SR1 -> SR2 -> MISO
bool StepLeds::selfTest()
{
  // 1. Clear the path first (flush unknown state)
  transfer(0x0000);

  // 2. Send a specific "Test Pattern" (0xA5A5)
  // We won't see this back yet; it is now sitting in the register.
  transfer(0xA5A5);

  // 3. Send a "Dummy Pattern" (0xFFFF) to push the Test Pattern out
  uint16_t result = transfer(0xFFFF);

  // 4. Restore the actual user state so the LEDs don't stay weird
  show();

  return (result == 0xA5A5);
}