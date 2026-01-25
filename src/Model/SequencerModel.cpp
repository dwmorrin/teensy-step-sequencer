#include "SequencerModel.h"

SequencerModel::SequencerModel()
{
  // Defaults
  _bpm = 120; // Default if DEFAULT_BPM not defined, otherwise use constant
#ifdef DEFAULT_BPM
  _bpm = DEFAULT_BPM;
#endif

  _playing = false;
  _currentStep = 0;
  _playMode = MODE_PATTERN_LOOP;

  // Default View Settings
  currentViewPatternID = 0;
  activeTrackID = 0;

  // Initialize Playlist (Default: simple loop of Pattern 0)
  _playlistLength = 1;
  _playlistCursor = 0;
  for (int i = 0; i < MAX_SONG_LENGTH; i++)
  {
    _playlist[i] = 0;
  }

  // Initialize Pattern Pool (Clear all)
  for (int p = 0; p < MAX_PATTERNS; p++)
  {
    for (int t = 0; t < NUM_TRACKS; t++)
    {
      for (int s = 0; s < NUM_STEPS; s++)
      {
        _patternPool[p].steps[t][s] = false;
      }
    }
  }
}

// -------------------------------------------------------------------------
// TEMPO
// -------------------------------------------------------------------------
void SequencerModel::setBPM(int bpm)
{
  if (bpm < 10)
    bpm = 10;
  if (bpm > 300)
    bpm = 300;
  _bpm = bpm;
}

int SequencerModel::getBPM() const
{
  return _bpm;
}

// -------------------------------------------------------------------------
// TRANSPORT
// -------------------------------------------------------------------------
void SequencerModel::play()
{
  _playing = true;
}

void SequencerModel::stop()
{
  _playing = false;
  _currentStep = 0;
  _playlistCursor = 0; // Reset song position on stop
}

// -------------------------------------------------------------------------
// NAVIGATION & MODES
// -------------------------------------------------------------------------
void SequencerModel::nextPattern()
{
  if (currentViewPatternID < MAX_PATTERNS - 1)
  {
    currentViewPatternID++;
  }
}

void SequencerModel::prevPattern()
{
  if (currentViewPatternID > 0)
  {
    currentViewPatternID--;
  }
}

void SequencerModel::setPlayMode(PlayMode mode)
{
  _playMode = mode;
}

// -------------------------------------------------------------------------
// PLAYLIST (SONG) CRUD
// -------------------------------------------------------------------------
int SequencerModel::getPlaylistLength() const
{
  return _playlistLength;
}

int SequencerModel::getPlaylistCursor() const
{
  return _playlistCursor;
}

uint8_t SequencerModel::getPlaylistPattern(int slotIndex) const
{
  if (slotIndex < 0 || slotIndex >= _playlistLength)
    return 0;
  return _playlist[slotIndex];
}

void SequencerModel::setPlaylistPattern(int slotIndex, uint8_t patternID)
{
  if (slotIndex < 0 || slotIndex >= _playlistLength)
    return;

  // Safety clamp
  if (patternID >= MAX_PATTERNS)
    patternID = 0;

  _playlist[slotIndex] = patternID;
}

void SequencerModel::insertPlaylistSlot(int slotIndex, uint8_t patternID)
{
  // Prevent overflow
  if (_playlistLength >= MAX_SONG_LENGTH)
    return;

  // Clamp index
  if (slotIndex < 0)
    slotIndex = 0;
  if (slotIndex > _playlistLength)
    slotIndex = _playlistLength;

  // Shift elements to the right to make space
  for (int i = _playlistLength; i > slotIndex; i--)
  {
    _playlist[i] = _playlist[i - 1];
  }

  // Insert new item
  _playlist[slotIndex] = patternID;
  _playlistLength++;
}

void SequencerModel::deletePlaylistSlot(int slotIndex)
{
  // Prevent empty playlist (must have at least 1 slot)
  if (_playlistLength <= 1)
    return;

  if (slotIndex < 0 || slotIndex >= _playlistLength)
    return;

  // Shift elements to the left to fill gap
  for (int i = slotIndex; i < _playlistLength - 1; i++)
  {
    _playlist[i] = _playlist[i + 1];
  }

  _playlistLength--;

  // Safety: If cursor was at the end, pull it back so it doesn't point to void
  if (_playlistCursor >= _playlistLength)
  {
    _playlistCursor = _playlistLength - 1;
  }
}

// -------------------------------------------------------------------------
// EDITING
// -------------------------------------------------------------------------
void SequencerModel::toggleStep(int track, int step)
{
  if (track >= NUM_TRACKS || step >= NUM_STEPS)
    return;

  // Toggle the bit in the currently viewed pattern
  bool current = _patternPool[currentViewPatternID].steps[track][step];
  _patternPool[currentViewPatternID].steps[track][step] = !current;
}

void SequencerModel::clearCurrentPattern()
{
  createSnapshot(); // Auto-save to undo buffer before clearing
  for (int t = 0; t < NUM_TRACKS; t++)
  {
    for (int s = 0; s < NUM_STEPS; s++)
    {
      _patternPool[currentViewPatternID].steps[t][s] = false;
    }
  }
}

// -------------------------------------------------------------------------
// UNDO SYSTEM
// -------------------------------------------------------------------------
void SequencerModel::createSnapshot()
{
  _undoBuffer = _patternPool[currentViewPatternID];
}

void SequencerModel::undo()
{
  Pattern temp = _patternPool[currentViewPatternID];
  _patternPool[currentViewPatternID] = _undoBuffer;
  _undoBuffer = temp;
}

// -------------------------------------------------------------------------
// ENGINE INTERFACE
// -------------------------------------------------------------------------
int SequencerModel::getPlayingPatternID() const
{
  if (_playMode == MODE_SONG)
  {
    return _playlist[_playlistCursor];
  }
  else
  {
    return currentViewPatternID;
  }
}

uint16_t SequencerModel::getTriggersForStep(int patternID, int step)
{
  uint16_t mask = 0;

  for (int t = 0; t < NUM_TRACKS; t++)
  {
    if (_patternPool[patternID].steps[t][step])
    {
      mask |= (1 << t);
    }
  }
  return mask;
}

bool SequencerModel::advanceStep()
{
  _currentStep++;

  // Check if we reached the end of the measure
  if (_currentStep >= NUM_STEPS)
  {
    _currentStep = 0;

    // If in SONG mode, move to the next pattern in the list
    if (_playMode == MODE_SONG)
    {
      _playlistCursor++;
      if (_playlistCursor >= _playlistLength)
      {
        _playlistCursor = 0; // Loop Song
      }
    }

    return true; // "We wrapped around"
  }

  return false;
}