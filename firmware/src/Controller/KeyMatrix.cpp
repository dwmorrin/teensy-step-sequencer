#include "KeyMatrix.h"

// LUT: Maps [Row][Col] to Switch Number (1-32) based on your schematic
// Rows 0-1 are interleaved for Top Physical Row (1-16)
// Rows 2-3 are interleaved for Bottom Physical Row (17-32)
const int SWITCH_MAP[4][8] = {
    // Col 0, 1, 2, 3, 4, 5, 6, 7
    {1, 3, 5, 7, 9, 11, 13, 15},      // Row 1 (Pin 36) -> Odd Triggers
    {2, 4, 6, 8, 10, 12, 14, 16},     // Row 2 (Pin 34) -> Even Triggers
    {17, 19, 21, 23, 25, 27, 29, 31}, // Row 3 (Pin 38) -> Odd Functions
    {18, 20, 22, 24, 26, 28, 30, 32}  // Row 4 (Pin 40) -> Even Functions
};

KeyMatrix::KeyMatrix()
{
  _lastScanTime = 0;
}

void KeyMatrix::init()
{
  // Setup Columns (Inputs)
  for (int i = 0; i < MATRIX_COLS; i++)
  {
    pinMode(_colPins[i], INPUT_PULLUP);
  }

  // Setup Rows (Outputs)
  // OPEN DRAIN EMULATION:
  // Active = OUTPUT LOW
  // Inactive = INPUT (High Impedance)
  for (int i = 0; i < MATRIX_ROWS; i++)
  {
    pinMode(_rowPins[i], INPUT); // Default to floating/inactive
  }

  // Clear state
  for (int r = 0; r < MATRIX_ROWS; r++)
  {
    for (int c = 0; c < MATRIX_COLS; c++)
    {
      _state[r][c] = false;
      _lastState[r][c] = false;
    }
  }
}

int KeyMatrix::getEvent()
{
  // THROTTLE: Scan at ~200Hz (5ms) to handle debounce naturally
  if (millis() - _lastScanTime < 5)
    return 0;
  _lastScanTime = millis();

  int eventID = 0;

  // SCAN LOOP
  for (int r = 0; r < MATRIX_ROWS; r++)
  {
    // Activate Row (Drive Low)
    pinMode(_rowPins[r], OUTPUT);
    digitalWrite(_rowPins[r], LOW);

    // Allow signal to settle (very short delay)
    delayMicroseconds(10);

    // Read Cols
    for (int c = 0; c < MATRIX_COLS; c++)
    {
      // LOW means PRESSED (because of Pullups)
      bool isPressed = (digitalRead(_colPins[c]) == LOW);

      // DETECT RISING EDGE (Just Pressed)
      if (isPressed && !_lastState[r][c])
      {
        _state[r][c] = true;
        // We only return ONE event per scan to keep logic simple.
        // Priority goes to first found.
        eventID = SWITCH_MAP[r][c];
      }
      // DETECT FALLING EDGE (Released)
      else if (!isPressed && _lastState[r][c])
      {
        _state[r][c] = false;
      }

      _lastState[r][c] = isPressed;
    }

    // Deactivate Row (Float)
    pinMode(_rowPins[r], INPUT);
  }

  return eventID;
}

bool KeyMatrix::isShiftHeld()
{
  // Switch 32 is Row 3 (Index 3), Col 7
  return _state[3][7];
}