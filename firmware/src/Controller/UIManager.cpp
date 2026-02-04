#include "UIManager.h"
#include "Debug.h"

// MACROS
#define ASCII_BS 8
#define ASCII_TAB 9
#define ASCII_LF 10
#define ASCII_CR 13
#define ASCII_ESC 27
#define ASCII_SPACE 32
#define ASCII_DEL 127

UIManager::UIManager(SequencerModel &model, OutputDriver &driver, ClockEngine &clock)
    : _model(model),
      _driver(driver),
      _clock(clock),
      _tempoPot(PIN_POT_TEMPO, POT_INVERT_POLARITY ? 300 : 30, POT_INVERT_POLARITY ? 30 : 300, 4),
      _paramPot(PIN_POT_PARAM, POT_INVERT_POLARITY ? 63 : 0, POT_INVERT_POLARITY ? 0 : 63, 2)
{
  _currentMode = UI_MODE_STEP_EDIT;
  _uiSelectedSlot = 0;
  _songModeBankOffset = 0;
  _lastSwingChangeTime = 0;
  _lastSwingValue = 0;
}

void UIManager::init()
{
  _keyMatrix.init();
}

void UIManager::processInput()
{
  // 1. ANALOG
  if (_tempoPot.update())
  {
    _model.setBPM(_tempoPot.getValue());
  }

  if (_paramPot.update())
  {
    bool shift = _keyMatrix.isShiftHeld();

    // CASE A: SWING CONTROL (Shift + Param)
    if (shift)
    {
      int swingVal = map(_paramPot.getValue(), 0, 63, 0, 100);
      _model.setTrackSwing(_model.activeTrackID, swingVal);
      _lastSwingValue = swingVal;
      _lastSwingChangeTime = millis();
      LOG("Track %d Swing: %d\n", _model.activeTrackID, swingVal);
    }
    // CASE B: SONG MODE Pattern Select (No Shift)
    else if (_model.getPlayMode() == MODE_SONG)
    {
      int pID = _paramPot.getValue();
      if (pID < 0)
        pID = 0;
      if (pID >= MAX_PATTERNS)
        pID = MAX_PATTERNS - 1;
      _model.setPlaylistPattern(_uiSelectedSlot, pID);
    }
    // CASE C: NORMAL OPERATION
    else
    {
      LOG("Param Pot: %d\n", _paramPot.getValue());
    }
  }

  // 2. MATRIX SCAN
  _keyMatrix.update();

  while (int swID = _keyMatrix.getNextEvent())
  {
    LOG("Matrix Event: Switch %d\n", swID);
    InputCommand cmd = _mapMatrixToCommand(swID);
    if (cmd != CMD_NONE)
    {
      handleCommand(cmd);
    }
  }
}

// ----------------------------------------------------------------------
// MAPPER
// ----------------------------------------------------------------------
InputCommand UIManager::_mapMatrixToCommand(int id)
{
  bool shift = _keyMatrix.isShiftHeld();

  // ROW 1 & 2 (Physical): Steps 1-16
  if (id >= 1 && id <= 16)
  {
    if (shift && id <= 4)
    {
      switch (id)
      {
      case 1:
        return CMD_PLAYLIST_BANK_1;
      case 2:
        return CMD_PLAYLIST_BANK_2;
      case 3:
        return CMD_PLAYLIST_BANK_3;
      case 4:
        return CMD_PLAYLIST_BANK_4;
      default:
        return CMD_NONE;
      }
    }
    return (InputCommand)(CMD_TRIGGER_1 + (id - 1));
  }

  // FUNCTION KEYS
  switch (id)
  {
  case 17:
    return CMD_TRACK_1;
  case 18:
    return CMD_TRACK_2;
  case 19:
    return CMD_TRACK_3;
  case 20:
    return CMD_TRACK_4;

  // NEW: Map physical buttons E, F, G (IDs 21-23)
  case 21:
    return CMD_TRACK_5;
  case 22:
    return CMD_TRACK_6;
  case 23:
    return CMD_TRACK_7;

  // NEW: Map physical button H (ID 24)
  case 24:
    if (shift)
      return CMD_UNDO;  // Shift + H = UNDO
    return CMD_TRACK_8; // H = Track 8

  case 25:
    if (_model.getPlayMode() == MODE_SONG)
      return CMD_PLAYLIST_DELETE;
    return CMD_CLEAR_PROMPT;

  case 26:
    return CMD_TRACK_PREV;
  case 27:
    return CMD_TRACK_NEXT;

  case 28:
    if (shift && _model.getPlayMode() == MODE_SONG)
      return CMD_PLAYLIST_INSERT_PREV;
    if (_model.getPlayMode() == MODE_SONG)
      return CMD_PLAYLIST_PREV;
    return CMD_PATTERN_PREV;

  case 29:
    if (shift && _model.getPlayMode() == MODE_SONG)
      return CMD_PLAYLIST_INSERT_NEXT;
    if (_model.getPlayMode() == MODE_SONG)
      return CMD_PLAYLIST_NEXT;
    return CMD_PATTERN_NEXT;

  case 30: // PLAY
    if (shift)
      return CMD_QUANTIZE_MENU;
    return CMD_TRANSPORT_TOGGLE;

  case 31:
    if (shift)
      return CMD_MODE_TOGGLE;
    return CMD_SONG_MODE_TOGGLE;

  case 32:
    return CMD_NONE;

  default:
    return CMD_NONE;
  }
}

// ----------------------------------------------------------------------
// TRANSLATOR
// ----------------------------------------------------------------------
void UIManager::handleKeyPress(int key)
{
  InputCommand cmd = CMD_NONE;

  if (key == ASCII_SPACE)
    cmd = CMD_TRANSPORT_TOGGLE;
  else if (key == ASCII_TAB)
    cmd = CMD_MODE_TOGGLE;
  else if (key == ASCII_CR || key == ASCII_LF)
    cmd = CMD_SONG_MODE_TOGGLE;
  else if (key == 'z')
    cmd = CMD_UNDO;
  else if (key == 'q')
    cmd = CMD_QUANTIZE_MENU;

  // Track Direct Selection (A-H)
  else if (key == 'a')
    cmd = CMD_TRACK_1;
  else if (key == 'b')
    cmd = CMD_TRACK_2;
  else if (key == 'c')
    cmd = CMD_TRACK_3;
  else if (key == 'd')
    cmd = CMD_TRACK_4;
  else if (key == 'e')
    cmd = CMD_TRACK_5;
  else if (key == 'f')
    cmd = CMD_TRACK_6;
  else if (key == 'g')
    cmd = CMD_TRACK_7;
  else if (key == 'h')
    cmd = CMD_TRACK_8;

  // Navigation
  else if (key == '[')
    cmd = CMD_PATTERN_PREV;
  else if (key == ']')
    cmd = CMD_PATTERN_NEXT;

  // Triggers
  else if (key >= '1' && key <= '4')
    cmd = (InputCommand)(CMD_TRIGGER_1 + (key - '1'));

  if (cmd != CMD_NONE)
    handleCommand(cmd);
}

// ----------------------------------------------------------------------
// EXECUTOR
// ----------------------------------------------------------------------
void UIManager::handleCommand(InputCommand cmd)
{
  // --- PRIORITY 1: HARDWARE TEST ---
  if (cmd == CMD_TEST_TOGGLE)
  {
    if (_model.getPlayMode() == MODE_HARDWARE_TEST)
    {
      _model.setPlayMode(MODE_PATTERN_LOOP);
      _model.stop();
    }
    else
    {
      _model.setPlayMode(MODE_HARDWARE_TEST);
    }
    return;
  }

  // --- PRIORITY 2: MODAL HANDLING (MENUS) ---

  // A. QUANTIZE MENU
  if (_currentMode == UI_MODE_QUANTIZE_MENU)
  {
    if (cmd >= CMD_TRIGGER_1 && cmd <= CMD_TRIGGER_4)
    {
      switch (cmd)
      {
      case CMD_TRIGGER_1:
        _model.setQuantization(Q_BAR);
        break;
      case CMD_TRIGGER_2:
        _model.setQuantization(Q_QUARTER);
        break;
      case CMD_TRIGGER_3:
        _model.setQuantization(Q_EIGHTH);
        break;
      case CMD_TRIGGER_4:
        _model.setQuantization(Q_INSTANT);
        break;
      default:
        break;
      }
      _currentMode = UI_MODE_PERFORM;
    }
    else
    {
      _currentMode = UI_MODE_PERFORM;
    }
    return;
  }

  // B. CLEAR PROMPT
  if (_currentMode == UI_MODE_CONFIRM_CLEAR_TRACK ||
      _currentMode == UI_MODE_CONFIRM_CLEAR_PATTERN)
  {
    bool isYes = (cmd == CMD_CONFIRM_YES || cmd == CMD_SONG_MODE_TOGGLE);
    if (cmd >= CMD_TRIGGER_1 && cmd <= CMD_TRIGGER_4)
      isYes = true;

    bool isNo = (cmd == CMD_CONFIRM_NO);
    if (cmd >= CMD_TRIGGER_13 && cmd <= CMD_TRIGGER_16)
      isNo = true;

    if (isYes)
    {
      if (_currentMode == UI_MODE_CONFIRM_CLEAR_TRACK)
        _model.clearTrack(_model.activeTrackID);
      else
        _model.clearCurrentPattern();
      _currentMode = UI_MODE_STEP_EDIT;
    }
    else if (isNo || cmd == CMD_CLEAR_PROMPT)
    {
      if (cmd == CMD_CLEAR_PROMPT)
        _currentMode = (_currentMode == UI_MODE_CONFIRM_CLEAR_TRACK) ? UI_MODE_CONFIRM_CLEAR_PATTERN : UI_MODE_CONFIRM_CLEAR_TRACK;
      else
        _currentMode = UI_MODE_STEP_EDIT;
    }
    return;
  }

  // --- PRIORITY 3: GLOBAL KEYS ---
  switch (cmd)
  {
  case CMD_TRANSPORT_TOGGLE:
    _model.isPlaying() ? _model.stop() : _model.play();
    return;

  case CMD_QUANTIZE_MENU:
    _currentMode = UI_MODE_QUANTIZE_MENU;
    return;

  case CMD_MODE_TOGGLE:
    _currentMode = (_currentMode == UI_MODE_STEP_EDIT) ? UI_MODE_PERFORM : UI_MODE_STEP_EDIT;
    return;

  case CMD_SONG_MODE_TOGGLE:
  {
    PlayMode pm = _model.getPlayMode();
    _model.setPlayMode(pm == MODE_PATTERN_LOOP ? MODE_SONG : MODE_PATTERN_LOOP);
    if (_model.getPlayMode() == MODE_SONG)
      _songModeBankOffset = 0;
  }
    return;

  case CMD_UNDO:
    _model.undo();
    return;

  case CMD_BPM_ENTER:
    _currentMode = UI_MODE_BPM_INPUT;
    _inputPtr = 0;
    memset(_inputBuffer, 0, sizeof(_inputBuffer));
    return;

  default:
    break;
  }

  // --- PRIORITY 4: CONTEXT SPECIFIC ---

  // A. SONG MODE
  if (_model.getPlayMode() == MODE_SONG)
  {
    switch (cmd)
    {
    case CMD_PLAYLIST_PREV:
      if (_uiSelectedSlot > 0)
        _uiSelectedSlot--;
      break;
    case CMD_PLAYLIST_NEXT:
      if (_uiSelectedSlot < _model.getPlaylistLength() - 1)
        _uiSelectedSlot++;
      break;
    case CMD_PLAYLIST_INSERT_PREV:
      _model.insertPlaylistSlot(_uiSelectedSlot, _model.getPlaylistPattern(_uiSelectedSlot));
      break;
    case CMD_PLAYLIST_INSERT_NEXT:
      _model.insertPlaylistSlot(_uiSelectedSlot + 1, _model.getPlaylistPattern(_uiSelectedSlot));
      _uiSelectedSlot++;
      break;
    case CMD_PLAYLIST_DELETE:
      _model.deletePlaylistSlot(_uiSelectedSlot);
      if (_uiSelectedSlot >= _model.getPlaylistLength())
        _uiSelectedSlot = max(0, _model.getPlaylistLength() - 1);
      break;

    case CMD_PLAYLIST_BANK_1:
      _songModeBankOffset = 0;
      break;
    case CMD_PLAYLIST_BANK_2:
      _songModeBankOffset = 16;
      break;
    case CMD_PLAYLIST_BANK_3:
      _songModeBankOffset = 32;
      break;
    case CMD_PLAYLIST_BANK_4:
      _songModeBankOffset = 48;
      break;

    case CMD_TRIGGER_1:
      _model.setPlaylistPattern(_uiSelectedSlot, _songModeBankOffset + 0);
      break;
    case CMD_TRIGGER_2:
      _model.setPlaylistPattern(_uiSelectedSlot, _songModeBankOffset + 1);
      break;
    case CMD_TRIGGER_3:
      _model.setPlaylistPattern(_uiSelectedSlot, _songModeBankOffset + 2);
      break;
    case CMD_TRIGGER_4:
      _model.setPlaylistPattern(_uiSelectedSlot, _songModeBankOffset + 3);
      break;
    case CMD_TRIGGER_5:
      _model.setPlaylistPattern(_uiSelectedSlot, _songModeBankOffset + 4);
      break;
    case CMD_TRIGGER_6:
      _model.setPlaylistPattern(_uiSelectedSlot, _songModeBankOffset + 5);
      break;
    case CMD_TRIGGER_7:
      _model.setPlaylistPattern(_uiSelectedSlot, _songModeBankOffset + 6);
      break;
    case CMD_TRIGGER_8:
      _model.setPlaylistPattern(_uiSelectedSlot, _songModeBankOffset + 7);
      break;
    case CMD_TRIGGER_9:
      _model.setPlaylistPattern(_uiSelectedSlot, _songModeBankOffset + 8);
      break;
    case CMD_TRIGGER_10:
      _model.setPlaylistPattern(_uiSelectedSlot, _songModeBankOffset + 9);
      break;
    case CMD_TRIGGER_11:
      _model.setPlaylistPattern(_uiSelectedSlot, _songModeBankOffset + 10);
      break;
    case CMD_TRIGGER_12:
      _model.setPlaylistPattern(_uiSelectedSlot, _songModeBankOffset + 11);
      break;
    case CMD_TRIGGER_13:
      _model.setPlaylistPattern(_uiSelectedSlot, _songModeBankOffset + 12);
      break;
    case CMD_TRIGGER_14:
      _model.setPlaylistPattern(_uiSelectedSlot, _songModeBankOffset + 13);
      break;
    case CMD_TRIGGER_15:
      _model.setPlaylistPattern(_uiSelectedSlot, _songModeBankOffset + 14);
      break;
    case CMD_TRIGGER_16:
      _model.setPlaylistPattern(_uiSelectedSlot, _songModeBankOffset + 15);
      break;

    case CMD_PATTERN_NEXT:
    {
      int p = (_model.getPlaylistPattern(_uiSelectedSlot) + 1) % MAX_PATTERNS;
      _model.setPlaylistPattern(_uiSelectedSlot, p);
      break;
    }
    case CMD_PATTERN_PREV:
    {
      int p = _model.getPlaylistPattern(_uiSelectedSlot) - 1;
      if (p < 0)
        p = MAX_PATTERNS - 1;
      _model.setPlaylistPattern(_uiSelectedSlot, p);
      break;
    }

    case CMD_TRACK_1:
      _model.activeTrackID = 0;
      break;
    case CMD_TRACK_2:
      _model.activeTrackID = 1;
      break;
    case CMD_TRACK_3:
      _model.activeTrackID = 2;
      break;
    case CMD_TRACK_4:
      _model.activeTrackID = 3;
      break;
    case CMD_TRACK_5:
      _model.activeTrackID = 4;
      break;
    case CMD_TRACK_6:
      _model.activeTrackID = 5;
      break;
    case CMD_TRACK_7:
      _model.activeTrackID = 6;
      break;
    case CMD_TRACK_8:
      _model.activeTrackID = 7;
      break;
    default:
      break;
    }
    return;
  }

  // B. PATTERN / PERFORM MODE
  switch (cmd)
  {
  case CMD_TRACK_1:
    _model.activeTrackID = 0;
    break;
  case CMD_TRACK_2:
    _model.activeTrackID = 1;
    break;
  case CMD_TRACK_3:
    _model.activeTrackID = 2;
    break;
  case CMD_TRACK_4:
    _model.activeTrackID = 3;
    break;
  case CMD_TRACK_5:
    _model.activeTrackID = 4;
    break;
  case CMD_TRACK_6:
    _model.activeTrackID = 5;
    break;
  case CMD_TRACK_7:
    _model.activeTrackID = 6;
    break;
  case CMD_TRACK_8:
    _model.activeTrackID = 7;
    break;

  case CMD_PATTERN_PREV:
    _model.prevPattern();
    break;
  case CMD_PATTERN_NEXT:
    _model.nextPattern();
    break;

  case CMD_TRACK_NEXT:
    if (_model.activeTrackID < NUM_TRACKS - 1)
      _model.activeTrackID++;
    break;
  case CMD_TRACK_PREV:
    if (_model.activeTrackID > 0)
      _model.activeTrackID--;
    break;
  case CMD_CLEAR_PROMPT:
    _currentMode = UI_MODE_CONFIRM_CLEAR_TRACK;
    break;

  case CMD_TRIGGER_1:
    _handleTrigger(0);
    break;
  case CMD_TRIGGER_2:
    _handleTrigger(1);
    break;
  case CMD_TRIGGER_3:
    _handleTrigger(2);
    break;
  case CMD_TRIGGER_4:
    _handleTrigger(3);
    break;
  case CMD_TRIGGER_5:
    _handleTrigger(4);
    break;
  case CMD_TRIGGER_6:
    _handleTrigger(5);
    break;
  case CMD_TRIGGER_7:
    _handleTrigger(6);
    break;
  case CMD_TRIGGER_8:
    _handleTrigger(7);
    break;
  case CMD_TRIGGER_9:
    _handleTrigger(8);
    break;
  case CMD_TRIGGER_10:
    _handleTrigger(9);
    break;
  case CMD_TRIGGER_11:
    _handleTrigger(10);
    break;
  case CMD_TRIGGER_12:
    _handleTrigger(11);
    break;
  case CMD_TRIGGER_13:
    _handleTrigger(12);
    break;
  case CMD_TRIGGER_14:
    _handleTrigger(13);
    break;
  case CMD_TRIGGER_15:
    _handleTrigger(14);
    break;
  case CMD_TRIGGER_16:
    _handleTrigger(15);
    break;

  default:
    break;
  }
}

void UIManager::_handleTrigger(int stepIndex)
{
  if (_currentMode == UI_MODE_PERFORM)
  {
    if (stepIndex < 4)
      _clock.manualTrigger(1 << stepIndex);
  }
  else
  {
    _model.createSnapshot();
    _model.toggleStep(_model.activeTrackID, stepIndex);
  }
}

void UIManager::_handleBPMInput(int key)
{
  if (key >= '0' && key <= '9')
  {
    if (_inputPtr < 3)
    {
      _inputBuffer[_inputPtr] = (char)key;
      _inputPtr++;
    }
  }
  if (key == ASCII_CR || key == ASCII_LF)
  {
    if (_inputPtr > 0)
    {
      int newBPM = atoi(_inputBuffer);
      if (newBPM >= 30 && newBPM <= 300)
        _model.setBPM(newBPM);
    }
    _currentMode = UI_MODE_STEP_EDIT;
  }
  if (key == ASCII_ESC)
    _currentMode = UI_MODE_STEP_EDIT;
  if ((key == ASCII_BS || key == ASCII_DEL) && _inputPtr > 0)
  {
    _inputPtr--;
    _inputBuffer[_inputPtr] = 0;
  }
}

const char *UIManager::getInputBuffer() const { return _inputBuffer; }