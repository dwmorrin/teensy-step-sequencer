#pragma once
#include <Arduino.h>
#include "Config.h"

// 4 Rows, 8 Columns
#define MATRIX_ROWS 4
#define MATRIX_COLS 8

// Buffer size for simultaneous keypresses
#define EVENT_BUFFER_SIZE 16

class KeyMatrix
{
public:
  KeyMatrix();
  void init();

  // Scans the hardware and fills the internal buffer
  // Call this once per main loop / processInput
  void update();

  // Pops the next event from the queue.
  // Returns 0 if empty.
  int getNextEvent();

  // Check if a specific modifier key is currently held down
  bool isShiftHeld();

private:
  int _rowPins[MATRIX_ROWS] = {PIN_ROW_1, PIN_ROW_2, PIN_ROW_3, PIN_ROW_4};
  int _colPins[MATRIX_COLS] = {PIN_COL_1, PIN_COL_2, PIN_COL_3, PIN_COL_4,
                               PIN_COL_5, PIN_COL_6, PIN_COL_7, PIN_COL_8};

  // State
  bool _state[MATRIX_ROWS][MATRIX_COLS];
  bool _lastState[MATRIX_ROWS][MATRIX_COLS];

  unsigned long _lastScanTime;

  // RING BUFFER
  int _eventBuffer[EVENT_BUFFER_SIZE];
  int _head; // Write index
  int _tail; // Read index
};