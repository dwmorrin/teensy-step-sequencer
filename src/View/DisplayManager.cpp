#include "DisplayManager.h"

// Constructor: Initialize the U8g2 object and store the model reference
DisplayManager::DisplayManager(SequencerModel &model, UIManager &ui)
    : _model(model),
      _ui(ui),
      _u8g2(U8G2_R0, U8X8_PIN_NONE)
{
  _lastDrawTime = 0;
}

void DisplayManager::init()
{
  _u8g2.begin();
  _u8g2.setFont(u8g2_font_profont10_mr); // Small, crisp font
}

void DisplayManager::update()
{
  // THROTTLE: Limit to ~30 FPS (33ms)
  // This prevents the I2C bus from choking the CPU and delaying USB/Audio
  if (millis() - _lastDrawTime < 33)
    return;

  _lastDrawTime = millis();

  _u8g2.clearBuffer();

  _drawHeader();

  // CONTEXT SWITCHING:
  // If in Song Mode, show the Playlist Editor.
  // Otherwise, show the Step Grid.
  if (_model.getPlayMode() == MODE_SONG)
  {
    _drawPlaylist();
  }
  else
  {
    _drawGrid();
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

  // 1. Play State
  _u8g2.setCursor(0, 8);
  if (_model.isPlaying())
  {
    _u8g2.print("PLAY >>");
  }
  else
  {
    _u8g2.print("STOP ||");
  }

  // 2. Pattern ID
  _u8g2.setCursor(45, 8);
  _u8g2.print("PAT:");
  if (_model.currentViewPatternID < 9)
    _u8g2.print("0");
  _u8g2.print(_model.currentViewPatternID + 1);

  // 3. Song Mode Indicator
  _u8g2.setCursor(80, 8);
  if (_model.getPlayMode() == MODE_SONG)
  {
    _u8g2.print("SONG");
  }
  else
  {
    _u8g2.print("LOOP");
  }

  // 4. Current BPM Display
  _u8g2.setCursor(110, 8);
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

    // 1. Draw Labels
    _u8g2.setCursor(x, startY);
    _u8g2.print("S");
    if (currentSlotIndex < 9)
      _u8g2.print("0");                // Pad single digits
    _u8g2.print(currentSlotIndex + 1); // Show human index (1-based)

    // 2. Draw Pattern Value
    _u8g2.setCursor(x, startY + 12);
    int patID = _model.getPlaylistPattern(currentSlotIndex);
    if (patID < 9)
      _u8g2.print("0");
    _u8g2.print(patID + 1);

    // 3. Draw Selection Box (The Cursor)
    if (currentSlotIndex == selectedSlot)
    {
      _u8g2.drawFrame(x - 2, startY - 9, slotWidth - 2, 26);
    }

    // 4. Draw Playhead (Where the audio is)
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