#pragma once
#include "Config.h"
#include "Model/SequencerModel.h"
#include "Engine/OutputDriver.h"
#include "AnalogInput.h"
#include "InputCommands.h"

enum InterfaceMode
{
  UI_MODE_STEP_EDIT,
  UI_MODE_PERFORM,
  UI_MODE_BPM_INPUT,
  UI_MODE_CONFIRM_CLEAR_TRACK,
  UI_MODE_CONFIRM_CLEAR_PATTERN,
};

class UIManager
{
public:
  UIManager(SequencerModel &model, OutputDriver &driver);

  void init();
  void processInput();

  // The Translator: Maps Hardware Keys -> Commands
  void handleKeyPress(int key);

  // The Executor: Performs actions based on Commands
  void handleCommand(InputCommand cmd);

  // (Callback system removed - logic is now direct)

  InterfaceMode getMode() const { return _currentMode; };
  const char *getInputBuffer() const;
  int getSelectedSlot() const { return _uiSelectedSlot; }

private:
  SequencerModel &_model;
  OutputDriver &_driver;

  InterfaceMode _currentMode;

  // ANALOG INPUTS
  AnalogInput _tempoPot;
  AnalogInput _paramPot;

  // BPM Input State
  char _inputBuffer[4];
  int _inputPtr;
  int _uiSelectedSlot;

  // Internal Logic Handlers
  void _handleTrigger(int stepIndex);
  void _handleBPMInput(int key);
};