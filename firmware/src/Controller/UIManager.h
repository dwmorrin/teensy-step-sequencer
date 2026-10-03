#pragma once
#include "Config.h"
#include "Model/SequencerModel.h"
#include "Engine/OutputDriver.h"
#include "Engine/ClockEngine.h"
#include "AnalogInput.h"
#include "InputCommands.h"
#include "KeyMatrix.h"
#include "StorageManager/StorageManager.h"

enum InterfaceMode
{
  UI_MODE_STEP_EDIT,
  UI_MODE_PERFORM,
  UI_MODE_BPM_INPUT,
  UI_MODE_CONFIRM_CLEAR_TRACK,
  UI_MODE_CONFIRM_CLEAR_PATTERN,
  UI_MODE_QUANTIZE_MENU
};

class UIManager
{
public:
  UIManager(SequencerModel &model, OutputDriver &driver, ClockEngine &clock, StorageManager &storage);

  void init();
  void processInput();

  void handleKeyPress(int key);
  void handleCommand(InputCommand cmd);

  InterfaceMode getMode() const { return _currentMode; };
  const char *getInputBuffer() const;
  int getSelectedSlot() const { return _uiSelectedSlot; }
  int getSongModeBankOffset() const { return _songModeBankOffset; }

  // Getters for DisplayManager to show temporary overlays
  unsigned long getLastSwingChangeTime() const { return _lastSwingChangeTime; }
  int getLastSwingValue() const { return _lastSwingValue; }
  unsigned long getLastSaveTime() const { return _lastSaveTime; }
  unsigned long getLastLoadTime() const { return _lastLoadTime; }

  bool isSavePending() const { return _isSavePending; }
  bool isLoadPending() const { return _isLoadPending; }

  void executePendingSave();
  void executePendingLoad();

private:
  SequencerModel &_model;
  OutputDriver &_driver;
  ClockEngine &_clock;
  StorageManager &_storage;

  InterfaceMode _currentMode;

  // INPUTS
  AnalogInput _tempoPot;
  AnalogInput _paramPot;
  KeyMatrix _keyMatrix;

  char _inputBuffer[4];
  int _inputPtr;
  int _uiSelectedSlot;
  int _songModeBankOffset;

  unsigned long _lastSwingChangeTime;
  int _lastSwingValue;

  unsigned long _lastSaveTime;
  unsigned long _lastLoadTime;

  bool _isLoadPending;
  bool _isSavePending;

  void _handleTrigger(int stepIndex);
  void _handleBPMInput(int key);

  InputCommand _mapMatrixToCommand(int switchID);
};