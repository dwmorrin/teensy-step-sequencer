#include "DisplayManager.h"

// Constructor: Initialize the U8g2 object and store the model reference
DisplayManager::DisplayManager(SequencerModel &model)
    : _model(model),
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
  _drawGrid();

  _u8g2.sendBuffer();
}

void DisplayManager::_drawHeader()
{
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
  _u8g2.setCursor(50, 8);
  _u8g2.print("PAT:");
  // Formatting: Print "01" instead of "1"
  if (_model.currentViewPatternID < 9)
    _u8g2.print("0");
  _u8g2.print(_model.currentViewPatternID + 1);

  // 3. Mode (Song vs Loop)
  _u8g2.setCursor(95, 8);
  if (_model.getPlayMode() == MODE_SONG)
  {
    _u8g2.print("SONG");
  }
  else
  {
    _u8g2.print("LOOP");
  }
}

void DisplayManager::_drawGrid()
{
  int stepWidth = 8;
  int trackHeight = 12;
  int startY = 16;

  int viewPattern = _model.currentViewPatternID;
  int playingPattern = _model.getPlayingPatternID();

  // Iterate Tracks
  for (int track = 0; track < NUM_TRACKS; track++)
  {
    // Draw Track Indicator (The "<" arrow)
    // We do this before the loop to keep code clean
    if (track == _model.activeTrackID)
    {
      int arrowY = startY + (track * trackHeight) + 8;
      _u8g2.drawStr(122, arrowY, "<");
    }

    // Iterate Steps
    for (int step = 0; step < NUM_STEPS; step++)
    {
      int x = step * stepWidth;
      int y = startY + (track * trackHeight);

      // DATA RETRIEVAL:
      // We use the public helper we wrote in the Model.
      // It returns a bitmask for that specific step.
      uint16_t mask = _model.getTriggersForStep(viewPattern, step);

      // Check if the bit for this track is set
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
  // Only draw the cursor if we are actually LOOKING at the pattern that is PLAYING.
  // (If you are editing Pattern 5 but Pattern 1 is playing, don't show the cursor)
  if (_model.isPlaying() && (viewPattern == playingPattern))
  {
    int cursorX = _model.getCurrentStep() * stepWidth;

    // XOR Mode: Inverts colors behind it (White->Black, Black->White)
    _u8g2.setDrawColor(2);
    _u8g2.drawBox(cursorX, startY - 2, stepWidth - 1, (NUM_TRACKS * trackHeight) + 4);
    _u8g2.setDrawColor(1); // Restore normal mode
  }
}