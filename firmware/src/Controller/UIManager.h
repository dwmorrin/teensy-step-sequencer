#pragma once
#include <functional>
#include "Config.h"
#include "Model/SequencerModel.h"
#include "Engine/OutputDriver.h"
#include "AnalogInput.h"

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

  // The Router
  void handleKeyPress(int key);

  // Callback for Hardware Test
  using Callback = std::function<void()>;
  void setHardwareTestCallback(Callback cb) { _onTestRequested = cb; }

  InterfaceMode getMode() const { return _currentMode; };
  const char *getInputBuffer() const;
  int getSelectedSlot() const { return _uiSelectedSlot; }

private:
  SequencerModel &_model;
  OutputDriver &_driver;

  InterfaceMode _currentMode;
  Callback _onTestRequested = nullptr;

  // ANALOG INPUTS
  AnalogInput _tempoPot;
  AnalogInput _paramPot;

  // BPM Input State
  char _inputBuffer[4];
  int _inputPtr;
  int _uiSelectedSlot;

  void _handleStepEdit(int key);
  void _handlePerformance(int key);
  void _handleBPMInput(int key);
  bool _handleGlobalKeys(int key);
  void _handlePlaylistEdit(int key);
  void _handleConfirmClear(int key);
};