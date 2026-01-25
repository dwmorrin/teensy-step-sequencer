#pragma once
#include "Config.h"
#include "Model/SequencerModel.h"
#include "Engine/OutputDriver.h"

enum InterfaceMode
{
  UI_MODE_STEP_EDIT, // Buttons 1-16 toggle steps on the Active Track
  UI_MODE_PERFORM,   // Buttons 1-16 instantly fire Tracks 1-16
  UI_MODE_BPM_INPUT, // Modal input
};

class UIManager
{
public:
  // Dependency Injection: The UI needs access to the Brain and the Hardware
  UIManager(SequencerModel &model, OutputDriver &driver);

  void init();
  void processInput(); // Main polling method called in loop()

  // The Router: Decides what a keypress does based on the current Mode
  void handleKeyPress(int key);

  InterfaceMode getMode() const { return _currentMode; };
  const char *getInputBuffer() const;

  // Allows the DisplayManager to know which slot to highlight
  int getSelectedSlot() const { return _uiSelectedSlot; }

private:
  SequencerModel &_model;
  OutputDriver &_driver;

  InterfaceMode _currentMode;

  // BPM Input State
  char _inputBuffer[4]; // holds "999" + null terminator
  int _inputPtr;        // current cursor position

  int _uiSelectedSlot;

  void _handleStepEdit(int key);
  void _handlePerformance(int key);
  void _handleBPMInput(int key);
  bool _handleGlobalKeys(int key);

  void _handlePlaylistEdit(int key);
};