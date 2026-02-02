#pragma once
#include <Arduino.h>
#include "Config.h"

// 4 Rows, 8 Columns
#define MATRIX_ROWS 4
#define MATRIX_COLS 8

class KeyMatrix
{
public:
  KeyMatrix();
  void init();

  // Call this in the main loop/processInput
  // Scans the matrix and returns the ID of a key PRESSED this frame.
  // Returns 0 if no new press detected.
  int getEvent();

  // Check if a specific modifier key is currently held down
  bool isShiftHeld();

private:
  // Pin Arrays for easy iteration
  int _rowPins[MATRIX_ROWS] = {PIN_ROW_1, PIN_ROW_2, PIN_ROW_3, PIN_ROW_4};
  int _colPins[MATRIX_COLS] = {PIN_COL_1, PIN_COL_2, PIN_COL_3, PIN_COL_4,
                               PIN_COL_5, PIN_COL_6, PIN_COL_7, PIN_COL_8};

  // State
  // We keep track of the previous state to detect "Rising Edges" (Presses)
  bool _state[MATRIX_ROWS][MATRIX_COLS];
  bool _lastState[MATRIX_ROWS][MATRIX_COLS];

  // Debounce: We use a simple timer-based lockout or state confirmation
  unsigned long _lastScanTime;

  // Helper: The Lookup Table to map (r,c) to Switch ID (1-32)
  int _mapToSwitchID(int row, int col);
};