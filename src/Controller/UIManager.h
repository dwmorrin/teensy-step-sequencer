#pragma once
#include "Config.h"
#include "Model/SequencerModel.h"
#include "Engine/OutputDriver.h"

enum InterfaceMode
{
  UI_MODE_STEP_EDIT, // Buttons 1-16 toggle steps on the Active Track
  UI_MODE_PERFORM    // Buttons 1-16 instantly fire Tracks 1-16
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

private:
  SequencerModel &_model;
  OutputDriver &_driver;

  InterfaceMode _currentMode;

  // Specific Handlers to keep code clean
  void _handleStepEdit(int key);
  void _handlePerformance(int key); // The "Jamming" logic
};