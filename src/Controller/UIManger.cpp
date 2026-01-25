#include "UIManager.h"
#include "Debug.h"

// RENAMED MACROS (To avoid conflict with Teensy Core definitions)
#define ASCII_BS 8
#define ASCII_TAB 9
#define ASCII_LF 10
#define ASCII_CR 13
#define ASCII_ESC 27
#define ASCII_SPACE 32
#define ASCII_DEL 127

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
  // PRIORITY 1: EXCLUSIVE MODALS (The Trap)
  // ----------------------------------------------------------------
  // If we are inputting text/numbers, we DO NOT want Global keys
  // to fire (e.g. Spacebar shouldn't stop music, Backspace shouldn't Undo).
  if (_currentMode == UI_MODE_BPM_INPUT)
  {
    _handleBPMInput(key);
    return; // STOP HERE.
  }

  // ----------------------------------------------------------------
  // PRIORITY 2: GLOBAL / COMMON NAVIGATION
  // ----------------------------------------------------------------
  // Checks for Space, Tab, Undo, etc.
  // If one of these fires, we return immediately.
  if (_handleGlobalKeys(key))
  {
    return;
  }

  // ----------------------------------------------------------------
  // PRIORITY 3: CONTEXT SPECIFIC
  // ----------------------------------------------------------------
  switch (_currentMode)
  {
  case UI_MODE_STEP_EDIT:
    _handleStepEdit(key);
    break;
  case UI_MODE_PERFORM:
    _handlePerformance(key);
    break;
  default:
    break;
  }
}

// Moves all the "Generic" logic out of the main function
bool UIManager::_handleGlobalKeys(int key)
{

  // Transport (Spacebar)
  if (key == ASCII_SPACE)
  {
    if (_model.isPlaying())
      _model.stop();
    else
      _model.play();
    return true; // Key Consumed
  }

  // Mode Toggle (TAB)
  if (key == ASCII_TAB)
  {
    _currentMode = (_currentMode == UI_MODE_STEP_EDIT) ? UI_MODE_PERFORM : UI_MODE_STEP_EDIT;
    return true;
  }

  // Pattern Navigation
  if (key == '[')
  {
    _model.prevPattern();
    return true;
  }
  if (key == ']')
  {
    _model.nextPattern();
    return true;
  }

  // Song Mode Toggle (Enter)
  if (key == ASCII_CR || key == ASCII_LF)
  {
    PlayMode pm = _model.getPlayMode();
    _model.setPlayMode(pm == MODE_PATTERN_LOOP ? MODE_SONG : MODE_PATTERN_LOOP);
    return true;
  }

  // Undo (Backspace)
  if (key == ASCII_BS || key == ASCII_DEL)
  {
    _model.undo();
    return true;
  }

  // Track Selection (Arrows)
  if (key == 217)
  { // Down
    if (_model.activeTrackID < NUM_TRACKS - 1)
      _model.activeTrackID++;
    return true;
  }
  if (key == 218)
  { // Up
    if (_model.activeTrackID > 0)
      _model.activeTrackID--;
    return true;
  }

  // BPM Input Toggle ('b')
  if (key == 'b' || key == 'B')
  {
    _currentMode = UI_MODE_BPM_INPUT;
    _inputPtr = 0;
    memset(_inputBuffer, 0, sizeof(_inputBuffer));
    return true;
  }

  return false; // Key not used, pass it to Context Handler
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

void UIManager::_handleBPMInput(int key)
{
  // 1. Handle Numbers
  if (key >= '0' && key <= '9')
  {
    if (_inputPtr < 3)
    { // Max 3 digits (e.g. 999)
      _inputBuffer[_inputPtr] = (char)key;
      _inputPtr++;
    }
  }

  // 2. Handle Confirm (Enter)
  if (key == ASCII_CR || key == ASCII_LF)
  {
    if (_inputPtr > 0)
    {
      int newBPM = atoi(_inputBuffer);
      // Constraint check: Don't allow 0 or crazy speeds
      if (newBPM >= 30 && newBPM <= 300)
      {
        _model.setBPM(newBPM);
      }
    }
    _currentMode = UI_MODE_STEP_EDIT; // Return to normal
  }

  // 3. Handle Cancel (ESC)
  if (key == ASCII_ESC)
  {
    _currentMode = UI_MODE_STEP_EDIT;
  }

  // 4. Handle Backspace
  if ((key == ASCII_BS || key == ASCII_DEL) && _inputPtr > 0)
  {
    _inputPtr--;
    _inputBuffer[_inputPtr] = 0;
  }
}

const char *UIManager::getInputBuffer() const
{
  return _inputBuffer;
}
