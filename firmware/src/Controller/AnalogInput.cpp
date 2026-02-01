#include "AnalogInput.h"

AnalogInput::AnalogInput(int pin, int minOut, int maxOut, int threshold)
    : _pin(pin), _minOut(minOut), _maxOut(maxOut), _threshold(threshold)
{
  pinMode(_pin, INPUT);
  // Initialize state with current reading to avoid immediate jump on boot
  _lastStableRaw = analogRead(_pin);
  _currentMapped = map(_lastStableRaw, 0, 1023, _minOut, _maxOut);
}

bool AnalogInput::update()
{
  int currentRaw = analogRead(_pin);

  // HYSTERESIS CHECK
  // We only update if the pot has moved significantly (more than noise threshold)
  if (abs(currentRaw - _lastStableRaw) > _threshold)
  {

    // Update the stable raw value
    _lastStableRaw = currentRaw;

    // Re-calculate the mapped output
    int newMapped = map(currentRaw, 0, 1023, _minOut, _maxOut);

    // Only return true if the MAPPED value actually changed
    // (Prevents true return if noise was high but map bucket was same)
    if (newMapped != _currentMapped)
    {
      _currentMapped = newMapped;
      return true;
    }
  }

  return false;
}

int AnalogInput::getValue() const
{
  return _currentMapped;
}

int AnalogInput::getRaw() const
{
  return _lastStableRaw;
}