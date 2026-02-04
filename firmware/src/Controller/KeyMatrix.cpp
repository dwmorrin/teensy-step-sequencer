#include "KeyMatrix.h"

// LUT: Maps [Row][Col] to Switch Number (1-32)
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
  _head = 0;
  _tail = 0;
}

void KeyMatrix::init()
{
  // Setup Columns (Inputs)
  for (int i = 0; i < MATRIX_COLS; i++)
  {
    pinMode(_colPins[i], INPUT_PULLUP);
  }

  // Setup Rows (Outputs) - Open Drain Emulation
  for (int i = 0; i < MATRIX_ROWS; i++)
  {
    pinMode(_rowPins[i], INPUT);
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

void KeyMatrix::update()
{
  // THROTTLE: Scan at ~200Hz (5ms) to handle debounce naturally
  if (millis() - _lastScanTime < 5)
    return;
  _lastScanTime = millis();

  // SCAN LOOP
  for (int r = 0; r < MATRIX_ROWS; r++)
  {
    // Activate Row (Drive Low)
    pinMode(_rowPins[r], OUTPUT);
    digitalWrite(_rowPins[r], LOW);
    delayMicroseconds(10); // Settle time

    // Read Cols
    for (int c = 0; c < MATRIX_COLS; c++)
    {
      bool isPressed = (digitalRead(_colPins[c]) == LOW);

      // DETECT RISING EDGE (Just Pressed)
      if (isPressed && !_lastState[r][c])
      {
        _state[r][c] = true;

        // PUSH TO RING BUFFER
        int nextHead = (_head + 1) % EVENT_BUFFER_SIZE;
        if (nextHead != _tail)
        { // Prevent overflow overwriting
          _eventBuffer[_head] = SWITCH_MAP[r][c];
          _head = nextHead;
        }
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
}

int KeyMatrix::getNextEvent()
{
  // Return 0 if buffer is empty
  if (_head == _tail)
    return 0;

  int id = _eventBuffer[_tail];
  _tail = (_tail + 1) % EVENT_BUFFER_SIZE;
  return id;
}

bool KeyMatrix::isShiftHeld()
{
  // Switch 32 is Row 3 (Index 3), Col 7
  return _state[3][7];
}