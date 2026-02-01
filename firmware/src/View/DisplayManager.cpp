#include "DisplayManager.h"

// Constructor: Initialize U8g2 and StepLeds
DisplayManager::DisplayManager(SequencerModel &model, UIManager &ui, uint8_t latchPin)
    : _model(model),
      _ui(ui),
      _leds(latchPin),
      _u8g2(U8G2_R0, U8X8_PIN_NONE)
{
  _lastDrawTime = 0;
  _hasRunDiagnostic = false;
  _lastDiagnosticResult = false;
}

void DisplayManager::init()
{
  _u8g2.begin();
  _u8g2.setFont(u8g2_font_profont10_mr); // Small, crisp font

  // Initialize LEDs
  _leds.begin();
  _leds.clear();
  _leds.show();
}

void DisplayManager::update()
{
  // THROTTLE: Limit to ~30 FPS (33ms)
  if (millis() - _lastDrawTime < 33)
    return;

  _lastDrawTime = millis();

  // --- UPDATE LEDs (Synchronized with Screen) ---
  _leds.clear();

  // CASE 1: HARDWARE DIAGNOSTIC (Chaser Animation)
  if (_model.getPlayMode() == MODE_HARDWARE_TEST)
  {
    // Light up ONLY the current step (driven by fast clock)
    _leds.set(_model.getCurrentStep(), true);
  }
  // CASE 2: NORMAL OPERATION
  else if (_ui.getMode() == UI_MODE_STEP_EDIT || _ui.getMode() == UI_MODE_PERFORM)
  {
    int activeTrack = _model.activeTrackID;
    int viewPattern = _model.currentViewPatternID;

    for (int i = 0; i < NUM_STEPS; i++)
    {
      uint16_t mask = _model.getTriggersForStep(viewPattern, i);
      // Check if the bit for the active track is set
      if ((mask >> activeTrack) & 1)
      {
        _leds.set(i, true);
      }
    }
  }
  _leds.show();

  // --- UPDATE OLED ---
  _u8g2.clearBuffer();

  // CASE 1: HARDWARE DIAGNOSTIC
  if (_model.getPlayMode() == MODE_HARDWARE_TEST)
  {
    // Run the Electrical Test ONCE per session
    if (!_hasRunDiagnostic)
    {
      _lastDiagnosticResult = _leds.selfTest();
      _hasRunDiagnostic = true;
    }

    _u8g2.setFont(u8g2_font_6x10_tf);
    _u8g2.drawFrame(10, 20, 108, 30);
    _u8g2.setCursor(20, 40);
    _u8g2.print("HW CHECK: ");
    _u8g2.print(_lastDiagnosticResult ? "OK" : "FAIL");
  }
  else
  {
    // Reset diagnostic flag when we leave the mode
    _hasRunDiagnostic = false;

    _drawHeader();

    // CONTEXT SWITCHING:
    if (_ui.getMode() == UI_MODE_CONFIRM_CLEAR_TRACK ||
        _ui.getMode() == UI_MODE_CONFIRM_CLEAR_PATTERN)
    {
      _u8g2.setDrawColor(0); // Clear box background
      _u8g2.drawBox(10, 20, 108, 35);
      _u8g2.setDrawColor(1); // Border
      _u8g2.drawFrame(10, 20, 108, 35);

      _u8g2.setCursor(18, 35);

      // Dynamic Text based on specific mode
      if (_ui.getMode() == UI_MODE_CONFIRM_CLEAR_TRACK)
      {
        _u8g2.print("CLR TRACK? y/n");
        _u8g2.setCursor(18, 48);
        _u8g2.print("[#] for Pattern"); // Hint to user
      }
      else
      {
        _u8g2.print("CLR PATTERN? y/n");
        _u8g2.setCursor(18, 48);
        _u8g2.print("[#] for Track");
      }
    }
    // If in Song Mode, show the Playlist Editor
    else if (_model.getPlayMode() == MODE_SONG)
    {
      _drawPlaylist();
    }
    else
    {
      _drawGrid();
    }
  }

  _u8g2.sendBuffer();
}

void DisplayManager::_drawHeader()
{
  _u8g2.setFont(u8g2_font_6x10_tf); // Ensure font is set

  // --- MODE A: BPM INPUT (Modal Overlay) ---
  if (_ui.getMode() == UI_MODE_BPM_INPUT)
  {
    _u8g2.setCursor(0, 8);
    _u8g2.print("SET BPM: > ");
    _u8g2.print(_ui.getInputBuffer());

    // Blink cursor effect
    if ((millis() / 500) % 2 == 0)
      _u8g2.print("_");
    return; // Don't draw the rest of the header
  }

  // --- MODE B: STANDARD HEADER ---

  // Play State
  if (_model.isPlaying())
  {
    _u8g2.drawTriangle(0, 0, 0, 8, 8, 4);
  }
  else
  {
    _u8g2.drawBox(0, 0, 8, 9);
  }

  // Pattern ID
  _u8g2.setCursor(20, 8);
  _u8g2.print("PAT:");
  if (_model.currentViewPatternID < 9)
    _u8g2.print("0");
  _u8g2.print(_model.currentViewPatternID + 1);

  // Song Mode Indicator
  _u8g2.setCursor(65, 8);
  if (_model.getPlayMode() == MODE_SONG)
  {
    _u8g2.print("SONG");
  }
  else
  {
    _u8g2.print("LOOP");
  }

  // Current BPM Display
  _u8g2.setCursor(100, 8);
  _u8g2.print(_model.getBPM());
}

void DisplayManager::_drawGrid()
{
  int stepWidth = 7;
  int trackHeight = 12;
  int startY = 16;

  int viewPattern = _model.currentViewPatternID;
  int playingPattern = _model.getPlayingPatternID();

  // Iterate Tracks
  for (int track = 0; track < NUM_TRACKS; track++)
  {
    // Draw Track Indicator
    if (track == _model.activeTrackID)
    {
      int arrowY = startY + (track * trackHeight) + 8;
      _u8g2.drawStr(122, arrowY, _ui.getMode() == UI_MODE_PERFORM ? TRACK_PERFORM_STR : TRACK_EDIT_STR);
    }

    // Iterate Steps
    for (int step = 0; step < NUM_STEPS; step++)
    {
      int x = step * stepWidth;
      int y = startY + (track * trackHeight);

      // DATA RETRIEVAL
      uint16_t mask = _model.getTriggersForStep(viewPattern, step);
      bool isNoteOn = (mask >> track) & 1;

      if (isNoteOn)
      {
        _u8g2.drawBox(x + 1, y, 6, 8); // Filled box
      }
      else
      {
        _u8g2.drawPixel(x + 3, y + 4); // Tiny dot
      }
    }
  }

  // PLAYHEAD CURSOR
  // Only draw if we are looking at the pattern that is playing.
  if (_model.isPlaying() && (viewPattern == playingPattern))
  {
    int cursorX = _model.getCurrentStep() * stepWidth;

    // XOR Mode for cursor
    _u8g2.setDrawColor(2);
    _u8g2.drawBox(cursorX, startY - 2, stepWidth - 1, (NUM_TRACKS * trackHeight) + 4);
    _u8g2.setDrawColor(1);
  }
}

void DisplayManager::_drawPlaylist()
{
  // Visual Settings
  int slotWidth = 24;
  int startX = 5;
  int startY = 25;

  _u8g2.setFont(u8g2_font_6x10_tf);

  // CAMERA LOGIC:
  // We can't show all 128 slots. We show a window of ~5 slots.
  // The window centers around the user's selected slot.
  int selectedSlot = _ui.getSelectedSlot();
  int startView = selectedSlot - 2;
  if (startView < 0)
    startView = 0;

  // Draw 5 Slots
  for (int i = 0; i < 5; i++)
  {
    int currentSlotIndex = startView + i;

    // Stop if we run off the end of the actual song
    if (currentSlotIndex >= _model.getPlaylistLength())
      break;

    int x = startX + (i * slotWidth);

    // Draw Labels
    _u8g2.setCursor(x, startY);
    _u8g2.print("S");
    if (currentSlotIndex < 9)
      _u8g2.print("0");                // Pad single digits
    _u8g2.print(currentSlotIndex + 1); // Show human index (1-based)

    // Draw Pattern Value
    _u8g2.setCursor(x, startY + 12);
    int patID = _model.getPlaylistPattern(currentSlotIndex);
    if (patID < 9)
      _u8g2.print("0");
    _u8g2.print(patID + 1);

    // Draw Selection Box (The Cursor)
    if (currentSlotIndex == selectedSlot)
    {
      _u8g2.drawFrame(x - 2, startY - 9, slotWidth - 2, 26);
    }

    // Draw Playhead (Where the audio is)
    // Only if we are playing and this slot is the active one
    if (_model.isPlaying() && currentSlotIndex == _model.getPlaylistCursor())
    {
      _u8g2.drawStr(x + 5, startY + 22, "^");
    }
  }

  // Helper Text
  _u8g2.setCursor(0, 60);
  _u8g2.setFont(u8g2_font_profont10_mr);
  _u8g2.print("[i]nsert  [x]delete");
}