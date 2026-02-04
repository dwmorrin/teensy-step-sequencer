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
    _leds.set(_model.getCurrentStep(), true);
  }

  // CASE 2: SONG MODE (New Logic)
  else if (_model.getPlayMode() == MODE_SONG)
  {
    if (_model.isPlaying())
    {
      // A. PLAYING: Show the Rhythm Chaser (Current Step)
      _leds.set(_model.getCurrentStep(), true);
    }
    else
    {
      // B. STOPPED: Show the Pattern ID for the Selected Slot
      // This allows users to see "Pattern 5 is in this slot"
      int patID = _model.getPlaylistPattern(_ui.getSelectedSlot());
      int bankOffset = _ui.getSongModeBankOffset();

      // Map Pattern ID to 1-16 range based on current Bank
      int ledIndex = patID - bankOffset;

      if (ledIndex >= 0 && ledIndex < 16)
      {
        _leds.set(ledIndex, true);
      }
    }
  }

  // CASE 3: PATTERN LOOP / STEP EDIT (Legacy Logic)
  else
  {
    int activeTrack = _model.activeTrackID;
    int viewPattern = _model.currentViewPatternID;

    // Show Triggers for the active track
    for (int i = 0; i < NUM_STEPS; i++)
    {
      uint16_t mask = _model.getTriggersForStep(viewPattern, i);
      if ((mask >> activeTrack) & 1)
      {
        _leds.set(i, true);
      }
    }

    // Optional: Overlay a blinking cursor on the LEDs if playing?
    // For now, let's keep it simple (Triggers only).
  }

  _leds.show();

  // --- UPDATE OLED ---
  _u8g2.clearBuffer();

  // CASE 1: HARDWARE DIAGNOSTIC
  if (_model.getPlayMode() == MODE_HARDWARE_TEST)
  {
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
    _hasRunDiagnostic = false;

    _drawHeader();

    // CONTEXT SWITCHING:
    if (_ui.getMode() == UI_MODE_CONFIRM_CLEAR_TRACK ||
        _ui.getMode() == UI_MODE_CONFIRM_CLEAR_PATTERN)
    {
      _u8g2.setDrawColor(0);         // Clear box background
      _u8g2.drawBox(5, 20, 118, 40); // Slightly larger box
      _u8g2.setDrawColor(1);         // Border
      _u8g2.drawFrame(5, 20, 118, 40);

      _u8g2.setFont(u8g2_font_6x10_tf);
      _u8g2.setCursor(12, 35);

      if (_ui.getMode() == UI_MODE_CONFIRM_CLEAR_TRACK)
      {
        _u8g2.print("CLR TRACK?");
      }
      else
      {
        _u8g2.print("CLR PATTERN?");
      }

      _u8g2.setFont(u8g2_font_profont10_mr);
      _u8g2.setCursor(12, 48);
      _u8g2.print("YES: 1-4 | NO: 13-16");

      _u8g2.setCursor(12, 57);
      _u8g2.print("[CLEAR] to Toggle");
    }

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
  _u8g2.setFont(u8g2_font_6x10_tf);

  if (_ui.getMode() == UI_MODE_BPM_INPUT)
  {
    _u8g2.setCursor(0, 8);
    _u8g2.print("SET BPM: > ");
    _u8g2.print(_ui.getInputBuffer());

    if ((millis() / 500) % 2 == 0)
      _u8g2.print("_");
    return;
  }

  if (_model.isPlaying())
  {
    _u8g2.drawTriangle(0, 0, 0, 8, 8, 4);
  }
  else
  {
    _u8g2.drawBox(0, 0, 8, 9);
  }

  _u8g2.setCursor(20, 8);
  _u8g2.print("PAT:");
  if (_model.currentViewPatternID < 9)
    _u8g2.print("0");
  _u8g2.print(_model.currentViewPatternID + 1);

  _u8g2.setCursor(65, 8);
  if (_model.getPlayMode() == MODE_SONG)
  {
    _u8g2.print("SONG");
  }
  else
  {
    _u8g2.print("LOOP");
  }

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

  for (int track = 0; track < NUM_TRACKS; track++)
  {
    if (track == _model.activeTrackID)
    {
      int arrowY = startY + (track * trackHeight) + 8;
      _u8g2.drawStr(122, arrowY, _ui.getMode() == UI_MODE_PERFORM ? TRACK_PERFORM_STR : TRACK_EDIT_STR);
    }

    for (int step = 0; step < NUM_STEPS; step++)
    {
      int x = step * stepWidth;
      int y = startY + (track * trackHeight);

      uint16_t mask = _model.getTriggersForStep(viewPattern, step);
      bool isNoteOn = (mask >> track) & 1;

      if (isNoteOn)
      {
        _u8g2.drawBox(x + 1, y, 6, 8);
      }
      else
      {
        _u8g2.drawPixel(x + 3, y + 4);
      }
    }
  }

  // PLAYHEAD CURSOR (Grid View)
  if (_model.isPlaying() && (viewPattern == playingPattern))
  {
    int cursorX = _model.getCurrentStep() * stepWidth;
    _u8g2.setDrawColor(2);
    _u8g2.drawBox(cursorX, startY - 2, stepWidth - 1, (NUM_TRACKS * trackHeight) + 4);
    _u8g2.setDrawColor(1);
  }
}

void DisplayManager::_drawPlaylist()
{
  int slotWidth = 24;
  int startX = 5;
  int startY = 25;

  _u8g2.setFont(u8g2_font_6x10_tf);

  int selectedSlot = _ui.getSelectedSlot();
  int startView = selectedSlot - 2;
  if (startView < 0)
    startView = 0;

  for (int i = 0; i < 5; i++)
  {
    int currentSlotIndex = startView + i;

    if (currentSlotIndex >= _model.getPlaylistLength())
      break;

    int x = startX + (i * slotWidth);

    _u8g2.setCursor(x, startY);
    _u8g2.print("S");
    if (currentSlotIndex < 9)
      _u8g2.print("0");
    _u8g2.print(currentSlotIndex + 1);

    _u8g2.setCursor(x, startY + 12);
    int patID = _model.getPlaylistPattern(currentSlotIndex);
    if (patID < 9)
      _u8g2.print("0");
    _u8g2.print(patID + 1);

    if (currentSlotIndex == selectedSlot)
    {
      _u8g2.drawFrame(x - 2, startY - 9, slotWidth - 2, 26);
    }

    if (_model.isPlaying() && currentSlotIndex == _model.getPlaylistCursor())
    {
      _u8g2.drawStr(x + 5, startY + 22, "^");
    }
  }

  // Helper Text (Updated)
  _u8g2.setCursor(0, 60);
  _u8g2.setFont(u8g2_font_profont10_mr);
  _u8g2.print("Shft+<>:Ins | Clr:Del");
}