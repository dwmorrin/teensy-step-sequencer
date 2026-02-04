#include "DisplayManager.h"

DisplayManager::DisplayManager(SequencerModel &model, UIManager &ui, uint8_t latchPin)
    : _model(model), _ui(ui), _leds(latchPin), _u8g2(U8G2_R0, U8X8_PIN_NONE)
{
  _lastDrawTime = 0;
  _hasRunDiagnostic = false;
  _lastDiagnosticResult = false;
}

void DisplayManager::init()
{
  _u8g2.begin();
  _u8g2.setFont(u8g2_font_profont10_mr);
  _leds.begin();
  _leds.clear();
  _leds.show();
}

void DisplayManager::update()
{
  if (millis() - _lastDrawTime < 33)
    return;
  _lastDrawTime = millis();

  // --- LEDs ---
  _leds.clear();

  if (_model.getPlayMode() == MODE_HARDWARE_TEST)
  {
    _leds.set(_model.getCurrentStep(), true);
  }
  else if (_model.getPlayMode() == MODE_SONG)
  {
    if (_model.isPlaying())
    {
      _leds.set(_model.getCurrentStep(), true);
    }
    else
    {
      int patID = _model.getPlaylistPattern(_ui.getSelectedSlot());
      int ledIndex = patID - _ui.getSongModeBankOffset();
      if (ledIndex >= 0 && ledIndex < 16)
        _leds.set(ledIndex, true);
    }
  }
  else
  {
    // Pattern Loop Mode (Show Triggers)
    int activeTrack = _model.activeTrackID;
    int viewPattern = _model.currentViewPatternID;
    for (int i = 0; i < NUM_STEPS; i++)
    {
      uint16_t mask = _model.getTriggersForStep(viewPattern, i);
      if ((mask >> activeTrack) & 1)
        _leds.set(i, true);
    }
  }
  _leds.show();

  // --- OLED ---
  _u8g2.clearBuffer();

  if (_model.getPlayMode() == MODE_HARDWARE_TEST)
  {
    // ... (Diagnostic Display) ...
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

    // MODALS
    if (_ui.getMode() == UI_MODE_CONFIRM_CLEAR_TRACK || _ui.getMode() == UI_MODE_CONFIRM_CLEAR_PATTERN)
    {
      _u8g2.setDrawColor(0);
      _u8g2.drawBox(5, 20, 118, 40);
      _u8g2.setDrawColor(1);
      _u8g2.drawFrame(5, 20, 118, 40);
      _u8g2.setFont(u8g2_font_6x10_tf);
      _u8g2.setCursor(12, 35);
      if (_ui.getMode() == UI_MODE_CONFIRM_CLEAR_TRACK)
        _u8g2.print("CLR TRACK?");
      else
        _u8g2.print("CLR PATTERN?");
      _u8g2.setFont(u8g2_font_profont10_mr);
      _u8g2.setCursor(12, 48);
      _u8g2.print("YES: 1-4 | NO: 13-16");
    }
    // QUANTIZE MENU
    else if (_ui.getMode() == UI_MODE_QUANTIZE_MENU)
    {
      _u8g2.setDrawColor(0);
      _u8g2.drawBox(5, 18, 118, 44);
      _u8g2.setDrawColor(1);
      _u8g2.drawFrame(5, 18, 118, 44);

      _u8g2.setFont(u8g2_font_6x10_tf);
      _u8g2.setCursor(15, 30);
      _u8g2.print("LAUNCH QUANTIZE");

      _u8g2.setFont(u8g2_font_profont10_mr);
      _u8g2.setCursor(12, 44);
      _u8g2.print("[1] 1 Bar   [2] 1/4");
      _u8g2.setCursor(12, 54);
      _u8g2.print("[3] 1/8     [4] Inst");

      // Highlight current selection
      int y = 0, x = 0;
      switch (_model.getQuantization())
      {
      case Q_BAR:
        y = 44;
        x = 12;
        break;
      case Q_QUARTER:
        y = 44;
        x = 66;
        break;
      case Q_EIGHTH:
        y = 54;
        x = 12;
        break;
      case Q_INSTANT:
        y = 54;
        x = 66;
        break;
      }
      _u8g2.drawStr(x - 6, y, ">");
    }
    // SWING OVERLAY (Transient: Shows for 1.5 seconds)
    else if (millis() - _ui.getLastSwingChangeTime() < 1500)
    {
      // Draw Box
      _u8g2.setDrawColor(0);
      _u8g2.drawBox(20, 20, 88, 30);
      _u8g2.setDrawColor(1);
      _u8g2.drawFrame(20, 20, 88, 30);

      _u8g2.setFont(u8g2_font_6x10_tf);

      // Line 1: Track Name
      _u8g2.setCursor(25, 35);
      _u8g2.print("TRK ");
      // Convert Track ID 0-3 to A-D
      _u8g2.print((char)('A' + _model.activeTrackID));
      _u8g2.print(" SWING");

      // Line 2: Value
      _u8g2.setCursor(50, 47);
      _u8g2.print(_ui.getLastSwingValue());
      _u8g2.print("%");
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

  // Play Icon
  if (_model.isPlaying())
    _u8g2.drawTriangle(0, 0, 0, 8, 8, 4);
  else
    _u8g2.drawBox(0, 0, 8, 9);

  // Pattern Status
  _u8g2.setCursor(20, 8);

  // NEW: Transition Visualization
  int pending = _model.getPendingPatternID();
  int playing = _model.getPlayingPatternID();

  if (pending != playing)
  {
    // Transitioning! Blink the Target
    _u8g2.print(playing + 1);
    _u8g2.print(">");
    if ((millis() / 150) % 2 == 0)
    { // Fast Blink
      _u8g2.print(pending + 1);
    }
  }
  else
  {
    // Stable
    _u8g2.print("PAT:");
    if (playing < 9)
      _u8g2.print("0");
    _u8g2.print(playing + 1);
  }

  // Mode
  _u8g2.setCursor(65, 8);
  if (_model.getPlayMode() == MODE_SONG)
    _u8g2.print("SONG");
  else
    _u8g2.print("LOOP");

  // BPM
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
    uint8_t swing = _model.getTrackSwing(track);

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

      // --- VISUAL SWING LOGIC ---
      int boxX = x + 1;
      int boxW = 6;

      // If this is an Off-Beat (Odd Index) and Swing is applied (>10%)
      if ((step % 2 != 0) && (swing > 10))
      {
        // "Delay" the visual representation
        // Shift right by 2 pixels, shrink width by 2 pixels
        boxX += 2;
        boxW -= 2;
      }
      // --------------------------

      if (isNoteOn)
      {
        _u8g2.drawBox(boxX, y, boxW, 8);
      }
      else
      {
        // Move the empty dot too, to keep the grid aligned visual
        int dotX = x + 3;
        if ((step % 2 != 0) && (swing > 10))
          dotX += 2;

        _u8g2.drawPixel(dotX, y + 4);
      }
    }
  }

  if (_model.isPlaying() && (viewPattern == playingPattern))
  {
    int cursorX = _model.getCurrentStep() * stepWidth;
    // Note: We don't swing the cursor visual because the cursor represents "Grid Time",
    // whereas the note represents "Trigger Time". It's actually helpful to see the cursor hit "early" vs the block.
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
      _u8g2.drawFrame(x - 2, startY - 9, slotWidth - 2, 26);
    if (_model.isPlaying() && currentSlotIndex == _model.getPlaylistCursor())
      _u8g2.drawStr(x + 5, startY + 22, "^");
  }
  _u8g2.setCursor(0, 60);
  _u8g2.setFont(u8g2_font_profont10_mr);
  _u8g2.print("Shft+<>:Ins | Clr:Del");
}