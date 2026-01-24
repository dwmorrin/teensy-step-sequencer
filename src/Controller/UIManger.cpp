#include "UIManager.h"
#include "Debug.h"

// RENAMED MACROS (To avoid conflict with Teensy Core definitions)
#define ASCII_BACKSPACE 8
#define ASCII_TAB 9
#define ASCII_LF 10
#define ASCII_CR 13
#define ASCII_ESC 27
#define ASCII_SPACE 32
#define ASCII_DELETE 127

UIManager::UIManager(SequencerModel &model, OutputDriver &driver)
    : _model(model), _driver(driver)
{
  _currentMode = UI_MODE_STEP_EDIT; // Default to Composing
}

void UIManager::init()
{
  // Future: Load last used mode from Config/EEPROM
}

void UIManager::processInput()
{
  // Reserved for polling (Rotary Encoders, etc.)
  // Currently, main.cpp pushes keys here via handleKeyPress.
}

void UIManager::handleKeyPress(int key)
{
  LOG("Key Code Received: %d\n", key);
  // ----------------------------------------------------------------
  // 1. GLOBAL COMMANDS (Always Active)
  // ----------------------------------------------------------------

  // Transport (Spacebar)
  if (key == ASCII_SPACE)
  {
    if (_model.isPlaying())
    {
      _model.stop();
    }
    else
    {
      _model.play();
    }
    return; // Don't process this key further
  }

  // Mode Toggle (TAB)
  // Switches between "Composing" (Grid) and "Jamming" (Pads)
  if (key == ASCII_TAB)
  {
    if (_currentMode == UI_MODE_STEP_EDIT)
    {
      _currentMode = UI_MODE_PERFORM;
    }
    else
    {
      _currentMode = UI_MODE_STEP_EDIT;
    }
    return;
  }

  // Pattern Navigation ([ and ])
  if (key == '[')
    _model.prevPattern();
  if (key == ']')
    _model.nextPattern();

  // Song Mode Toggle (Enter)
  if (key == ASCII_CR || key == ASCII_LF)
  {
    if (_model.getPlayMode() == MODE_PATTERN_LOOP)
    {
      _model.setPlayMode(MODE_SONG);
    }
    else
    {
      _model.setPlayMode(MODE_PATTERN_LOOP);
    }
  }

  // Undo (Backspace)
  if (key == ASCII_BACKSPACE || key == ASCII_DELETE)
  {
    _model.undo();
    return;
  }

  // Track Selection (Up/Down)
  // Mapping standard Teensy USB Host arrow codes (218=Up, 217=Down)
  if (key == 217)
  { // Down Arrow
    if (_model.activeTrackID < NUM_TRACKS - 1)
      _model.activeTrackID++;
  }
  if (key == 218)
  { // Up Arrow
    if (_model.activeTrackID > 0)
      _model.activeTrackID--;
  }

  // ----------------------------------------------------------------
  // 2. CONTEXT-SPECIFIC COMMANDS
  // ----------------------------------------------------------------
  if (_currentMode == UI_MODE_STEP_EDIT)
  {
    _handleStepEdit(key);
  }
  else
  {
    _handlePerformance(key);
  }
}

// --- EDIT MODE: Keys toggle steps on the grid ---
void UIManager::_handleStepEdit(int key)
{
  int targetStep = -1;

  // Row 1 (Steps 1-4)
  if (key == '1')
    targetStep = 0;
  if (key == '2')
    targetStep = 1;
  if (key == '3')
    targetStep = 2;
  if (key == '4')
    targetStep = 3;
  // Row 2 (Steps 5-8)
  if (key == 'q')
    targetStep = 4;
  if (key == 'w')
    targetStep = 5;
  if (key == 'e')
    targetStep = 6;
  if (key == 'r')
    targetStep = 7;
  // Row 3 (Steps 9-12)
  if (key == 'a')
    targetStep = 8;
  if (key == 's')
    targetStep = 9;
  if (key == 'd')
    targetStep = 10;
  if (key == 'f')
    targetStep = 11;
  // Row 4 (Steps 13-16)
  if (key == 'z')
    targetStep = 12;
  if (key == 'x')
    targetStep = 13;
  if (key == 'c')
    targetStep = 14;
  if (key == 'v')
    targetStep = 15;

  // If valid key, toggle the bit in the Model
  if (targetStep != -1)
  {
    _model.createSnapshot(); // Auto-save for Undo
    _model.toggleStep(_model.activeTrackID, targetStep);
  }
}

// --- PERFORM MODE: Keys fire outputs immediately ---
void UIManager::_handlePerformance(int key)
{
  int targetTrack = -1;

  // Map Keys 1-4 to Tracks 1-4
  if (key == '1')
    targetTrack = 0;
  if (key == '2')
    targetTrack = 1;
  if (key == '3')
    targetTrack = 2;
  if (key == '4')
    targetTrack = 3;

  // If valid key, fire the hardware immediately
  if (targetTrack != -1)
  {
    // Create bitmask: (1 << 0) = 1, (1 << 1) = 2, etc.
    uint16_t mask = (1 << targetTrack);

    _driver.fireTriggers(mask);

    // Future: If "Recording" is active, we would also write to the Model here.
  }
}