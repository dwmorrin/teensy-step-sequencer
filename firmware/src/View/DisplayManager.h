#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "Model/SequencerModel.h"
#include "Controller/UIManager.h"
#include "StepLeds.h"

class DisplayManager
{
public:
  // Updated constructor to accept the hardware pin for LEDs
  DisplayManager(SequencerModel &model, UIManager &ui, uint8_t latchPin);

  void init();
  void update(); // Handles both OLED and LEDs

private:
  SequencerModel &_model;
  UIManager &_ui;
  StepLeds _leds; // The LED Driver

  // The specific driver for your 1.3" SH1106 OLED
  U8G2_SH1106_128X64_NONAME_F_HW_I2C _u8g2;

  unsigned long _lastDrawTime;

  // Diagnostic State
  bool _hasRunDiagnostic;
  bool _lastDiagnosticResult;

  // Internal Drawing Routines
  void _drawHeader();
  void _drawGrid();
  void _drawPlaylist();
};