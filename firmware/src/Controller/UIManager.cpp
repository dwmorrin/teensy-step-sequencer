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
      _paramPot(PIN_POT_PARAM, POT_INVERT_POLARITY ? 100 : 0, POT_INVERT_POLARITY ? 0 : 100, 8)
{
  _currentMode = UI_MODE_STEP_EDIT;
  _uiSelectedSlot = 0;
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
    LOG("Param Pot: %d\n", _paramPot.getValue());
  }

  // 2. MATRIX SCAN
  // Run the hardware scan (pushes events to buffer)
  _keyMatrix.update();

  // Consume ALL events in the buffer
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
// MAPPER: MATRIX SWITCH ID -> COMMAND
// ----------------------------------------------------------------------
InputCommand UIManager::_mapMatrixToCommand(int id)
{
  // ROW 1 (Physical): Steps 1-16
  if (id >= 1 && id <= 16)
  {
    return (InputCommand)(CMD_TRIGGER_1 + (id - 1));
  }

  // ROW 2 (Physical): Function Keys 17-32
  switch (id)
  {
  case 17:
    return CMD_TRACK_1; // A
  case 18:
    return CMD_TRACK_2; // B
  case 19:
    return CMD_TRACK_3; // C
  case 20:
    return CMD_TRACK_4; // D

  case 25:
    return CMD_CLEAR_PROMPT; // Clear
  case 26:
    return CMD_TRACK_PREV; // Up
  case 27:
    return CMD_TRACK_NEXT; // Down
  case 28:
    return CMD_PATTERN_PREV; // Left
  case 29:
    return CMD_PATTERN_NEXT; // Right
  case 30:
    return CMD_TRANSPORT_TOGGLE; // Play
  case 31:
    return CMD_MODE_TOGGLE; // Mode

  case 32:
    return CMD_NONE; // SHIFT

  default:
    return CMD_NONE;
  }
}

// ----------------------------------------------------------------------
// TRANSLATOR: ASCII -> COMMAND
// ----------------------------------------------------------------------
void UIManager::handleKeyPress(int key)
{
  LOG("Key: %d\n", key);

  if (_currentMode == UI_MODE_BPM_INPUT)
  {
    _handleBPMInput(key);
    return;
  }

  InputCommand cmd = CMD_NONE;

  // 1. GLOBAL KEYS
  if (key == ASCII_SPACE)
    cmd = CMD_TRANSPORT_TOGGLE;
  else if (key == ASCII_TAB)
    cmd = CMD_MODE_TOGGLE;
  else if (key == ASCII_CR || key == ASCII_LF)
    cmd = CMD_SONG_MODE_TOGGLE;
  else if (key == 't')
    cmd = CMD_TEST_TOGGLE;
  else if (key == 'b' || key == 'B')
    cmd = CMD_BPM_ENTER;
  else if (key == ASCII_BS || key == ASCII_DEL)
    cmd = CMD_UNDO;

  // 2. NAVIGATION
  else if (key == '[')
    cmd = CMD_PATTERN_PREV;
  else if (key == ']')
    cmd = CMD_PATTERN_NEXT;
  else if (key == 218)
    cmd = CMD_TRACK_PREV;
  else if (key == 217)
    cmd = CMD_TRACK_NEXT;
  else if (key == 216)
    cmd = CMD_PLAYLIST_PREV;
  else if (key == 215)
    cmd = CMD_PLAYLIST_NEXT;

  // 3. EDITING / TRIGGERS
  else if (key >= '1' && key <= '4')
    cmd = (InputCommand)(CMD_TRIGGER_1 + (key - '1'));
  else if (key == 'q')
    cmd = CMD_TRIGGER_5;
  else if (key == 'w')
    cmd = CMD_TRIGGER_6;
  else if (key == 'e')
    cmd = CMD_TRIGGER_7;
  else if (key == 'r')
    cmd = CMD_TRIGGER_8;
  else if (key == 'a')
    cmd = CMD_TRIGGER_9;
  else if (key == 's')
    cmd = CMD_TRIGGER_10;
  else if (key == 'd')
    cmd = CMD_TRIGGER_11;
  else if (key == 'f')
    cmd = CMD_TRIGGER_12;
  else if (key == 'z')
    cmd = CMD_TRIGGER_13;
  else if (key == 'c')
    cmd = CMD_TRIGGER_15;
  else if (key == 'v')
    cmd = CMD_TRIGGER_16;

  else if (key == 'x')
  {
    if (_model.getPlayMode() == MODE_SONG)
      cmd = CMD_PLAYLIST_DELETE;
    else
      cmd = CMD_TRIGGER_14;
  }

  // 4. PLAYLIST / MODAL
  else if (key == 'i' || key == 'I')
    cmd = CMD_PLAYLIST_INSERT;
  else if (key == '#')
    cmd = CMD_CLEAR_PROMPT;
  else if (key == 'y' || key == 'Y')
    cmd = CMD_CONFIRM_YES;
  else if (key == 'n' || key == 'N' || key == ASCII_ESC)
    cmd = CMD_CONFIRM_NO;

  if (cmd != CMD_NONE)
  {
    handleCommand(cmd);
  }
}

// ----------------------------------------------------------------------
// EXECUTOR: COMMAND -> ACTION
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

  // --- PRIORITY 2: MODAL HANDLING ---
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
      {
        _currentMode = (_currentMode == UI_MODE_CONFIRM_CLEAR_TRACK) ? UI_MODE_CONFIRM_CLEAR_PATTERN : UI_MODE_CONFIRM_CLEAR_TRACK;
      }
      else
      {
        _currentMode = UI_MODE_STEP_EDIT;
      }
    }
    return;
  }

  // --- PRIORITY 3: GLOBAL KEYS ---
  switch (cmd)
  {
  case CMD_TRANSPORT_TOGGLE:
    _model.isPlaying() ? _model.stop() : _model.play();
    return;

  case CMD_MODE_TOGGLE:
    _currentMode = (_currentMode == UI_MODE_STEP_EDIT) ? UI_MODE_PERFORM : UI_MODE_STEP_EDIT;
    return;

  case CMD_SONG_MODE_TOGGLE:
  {
    PlayMode pm = _model.getPlayMode();
    _model.setPlayMode(pm == MODE_PATTERN_LOOP ? MODE_SONG : MODE_PATTERN_LOOP);
    return;
  }

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
    case CMD_PATTERN_NEXT:
    {
      int p = _model.getPlaylistPattern(_uiSelectedSlot);
      p = (p + 1) % MAX_PATTERNS;
      _model.setPlaylistPattern(_uiSelectedSlot, p);
      break;
    }
    case CMD_PATTERN_PREV:
    {
      int p = _model.getPlaylistPattern(_uiSelectedSlot);
      p--;
      if (p < 0)
        p = MAX_PATTERNS - 1;
      _model.setPlaylistPattern(_uiSelectedSlot, p);
      break;
    }
    case CMD_PLAYLIST_INSERT:
      _model.insertPlaylistSlot(_uiSelectedSlot + 1, _model.getPlaylistPattern(_uiSelectedSlot));
      _uiSelectedSlot++;
      break;
    case CMD_PLAYLIST_DELETE:
      _model.deletePlaylistSlot(_uiSelectedSlot);
      if (_uiSelectedSlot >= _model.getPlaylistLength())
        _uiSelectedSlot = _model.getPlaylistLength() - 1;
      break;
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
    {
      _clock.manualTrigger(1 << stepIndex);
    }
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
      {
        _model.setBPM(newBPM);
      }
    }
    _currentMode = UI_MODE_STEP_EDIT;
  }
  if (key == ASCII_ESC)
  {
    _currentMode = UI_MODE_STEP_EDIT;
  }
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