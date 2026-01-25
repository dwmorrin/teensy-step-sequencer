#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "Model/SequencerModel.h"
#include "Controller/UIManager.h"

class DisplayManager
{
public:
  DisplayManager(SequencerModel &model, UIManager &ui);

  void init();
  void update(); // Call this in the main loop (handles its own throttling)

private:
  SequencerModel &_model;
  UIManager &_ui;

  // The specific driver for your 1.3" SH1106 OLED
  U8G2_SH1106_128X64_NONAME_F_HW_I2C _u8g2;

  unsigned long _lastDrawTime;

  // Internal Drawing Routines
  void _drawHeader();
  void _drawGrid();
};