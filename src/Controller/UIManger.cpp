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
  _currentMode = UI_MODE_STEP_EDIT;
  _uiSelectedSlot = 0; // Initialize Playlist Cursor
}

void UIManager::init()
{
  // Future: Load last used mode from Config/EEPROM
}

void UIManager::processInput()
{
  // Reserved for polling (Rotary Encoders, etc.)
}

void UIManager::handleKeyPress(int key)
{
  LOG("Key Code Received: %d\n", key);

  // ----------------------------------------------------------------
  // PRIORITY 1: EXCLUSIVE MODALS (The Trap)
  // ----------------------------------------------------------------
  if (_currentMode == UI_MODE_BPM_INPUT)
  {
    _handleBPMInput(key);
    return;
  }

  if (_currentMode == UI_MODE_CONFIRM_CLEAR)
  {
    _handleConfirmClear(key);
    return;
  }

  // ----------------------------------------------------------------
  // PRIORITY 2: GLOBAL / COMMON NAVIGATION
  // ----------------------------------------------------------------
  if (_handleGlobalKeys(key))
  {
    return;
  }

  // ----------------------------------------------------------------
  // PRIORITY 3: CONTEXT SPECIFIC
  // ----------------------------------------------------------------
  // If in Song Mode, override standard behavior to edit playlist
  if (_model.getPlayMode() == MODE_SONG)
  {
    _handlePlaylistEdit(key);
  }
  else
  {
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
    return true;
  }

  // Mode Toggle (TAB)
  if (key == ASCII_TAB)
  {
    _currentMode = (_currentMode == UI_MODE_STEP_EDIT) ? UI_MODE_PERFORM : UI_MODE_STEP_EDIT;
    return true;
  }

  // Pattern Navigation
  // Only handle globally if NOT in Song Mode.
  // In Song Mode, these keys edit the playlist slot value.
  if (_model.getPlayMode() != MODE_SONG)
  {
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

  // other editing commands
  if (key == '#')
  {
    _currentMode = UI_MODE_CONFIRM_CLEAR;
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

  if (targetTrack != -1)
  {
    uint16_t mask = (1 << targetTrack);
    _driver.fireTriggers(mask);
  }
}

// --- PLAYLIST MODE: Keys edit the Song structure ---
void UIManager::_handlePlaylistEdit(int key)
{
  // 1. Navigation (Left/Right)
  if (key == 216)
  { // Left Arrow
    if (_uiSelectedSlot > 0)
      _uiSelectedSlot--;
  }
  if (key == 215)
  { // Right Arrow
    if (_uiSelectedSlot < _model.getPlaylistLength() - 1)
      _uiSelectedSlot++;
  }

  // 2. Change Pattern Value ([ / ])
  if (key == ']' || key == '[')
  {
    int currentPat = _model.getPlaylistPattern(_uiSelectedSlot);
    if (key == ']')
      currentPat++;
    if (key == '[')
      currentPat--;

    // Wrap around logic
    if (currentPat < 0)
      currentPat = MAX_PATTERNS - 1;
    if (currentPat >= MAX_PATTERNS)
      currentPat = 0;

    _model.setPlaylistPattern(_uiSelectedSlot, currentPat);
  }

  // 3. Insert Slot (i)
  if (key == 'i' || key == 'I')
  {
    // Insert a copy of the current pattern at the current position
    int currentPat = _model.getPlaylistPattern(_uiSelectedSlot);
    _model.insertPlaylistSlot(_uiSelectedSlot + 1, currentPat);
    // Move selection to new slot
    _uiSelectedSlot++;
  }

  // 4. Delete Slot (x)
  if (key == 'x' || key == 'X')
  {
    _model.deletePlaylistSlot(_uiSelectedSlot);
    // Bounds check in case we deleted the last one
    if (_uiSelectedSlot >= _model.getPlaylistLength())
    {
      _uiSelectedSlot = _model.getPlaylistLength() - 1;
    }
  }
}

void UIManager::_handleBPMInput(int key)
{
  // 1. Handle Numbers
  if (key >= '0' && key <= '9')
  {
    if (_inputPtr < 3)
    {
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

void UIManager::_handleConfirmClear(int key)
{
  // Confirm ('y' or Enter)
  if (key == 'y' || key == 'Y' || key == ASCII_CR || key == ASCII_LF)
  {
    _model.clearTrack(_model.activeTrackID);
    _currentMode = UI_MODE_STEP_EDIT;
  }

  // Cancel ('n', ESC, or 'c' again)
  else if (key == 'n' || key == 'N' || key == ASCII_ESC || key == 'c')
  {
    _currentMode = UI_MODE_STEP_EDIT;
  }
}