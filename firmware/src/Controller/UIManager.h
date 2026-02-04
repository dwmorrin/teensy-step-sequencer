#pragma once
#include "Config.h"
#include "Model/SequencerModel.h"
#include "Engine/OutputDriver.h"
#include "Engine/ClockEngine.h"
#include "AnalogInput.h"
#include "InputCommands.h"
#include "KeyMatrix.h"

enum InterfaceMode
{
  UI_MODE_STEP_EDIT,
  UI_MODE_PERFORM,
  UI_MODE_BPM_INPUT,
  UI_MODE_CONFIRM_CLEAR_TRACK,
  UI_MODE_CONFIRM_CLEAR_PATTERN,
  UI_MODE_QUANTIZE_MENU // NEW
};

class UIManager
{
public:
  UIManager(SequencerModel &model, OutputDriver &driver, ClockEngine &clock);

  void init();
  void processInput();

  void handleKeyPress(int key);
  void handleCommand(InputCommand cmd);

  InterfaceMode getMode() const { return _currentMode; };
  const char *getInputBuffer() const;
  int getSelectedSlot() const { return _uiSelectedSlot; }
  int getSongModeBankOffset() const { return _songModeBankOffset; }

private:
  SequencerModel &_model;
  OutputDriver &_driver;
  ClockEngine &_clock;

  InterfaceMode _currentMode;

  // INPUTS
  AnalogInput _tempoPot;
  AnalogInput _paramPot;
  KeyMatrix _keyMatrix;

  // BPM Input State
  char _inputBuffer[4];
  int _inputPtr;
  int _uiSelectedSlot;
  int _songModeBankOffset;

  // Internal Logic Handlers
  void _handleTrigger(int stepIndex);
  void _handleBPMInput(int key);

  // Matrix Mapper
  InputCommand _mapMatrixToCommand(int switchID);
};