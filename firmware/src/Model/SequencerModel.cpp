#include "SequencerModel.h"

SequencerModel::SequencerModel()
{
  _bpm = 120;
#ifdef DEFAULT_BPM
  _bpm = DEFAULT_BPM;
#endif

  _playing = false;
  _currentStep = 0;
  _playMode = MODE_PATTERN_LOOP;

  currentViewPatternID = 0;
  activeTrackID = 0;

  _quantizationMode = Q_BAR;
  _playingPatternID = 0;
  _nextPatternID = 0;

  _playlistLength = 1;
  _playlistCursor = 0;
  for (int i = 0; i < MAX_SONG_LENGTH; i++)
    _playlist[i] = 0;

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

int SequencerModel::getBPM() const { return _bpm; }

// -------------------------------------------------------------------------
// TRANSPORT
// -------------------------------------------------------------------------
void SequencerModel::play()
{
  _playing = true;
  // On Start, sync everything
  _playingPatternID = currentViewPatternID;
  _nextPatternID = currentViewPatternID;
}

void SequencerModel::stop()
{
  _playing = false;
  _currentStep = 0;
  _playlistCursor = 0;
  // On Stop, sync everything
  _playingPatternID = currentViewPatternID;
  _nextPatternID = currentViewPatternID;
}

// -------------------------------------------------------------------------
// NAVIGATION & TRANSITIONS
// -------------------------------------------------------------------------
void SequencerModel::setPattern(int patternID)
{
  if (patternID < 0)
    patternID = 0;
  if (patternID >= MAX_PATTERNS)
    patternID = MAX_PATTERNS - 1;

  // Update the UI immediately
  currentViewPatternID = patternID;

  // Update the Waiting Room
  _nextPatternID = patternID;

  // If Stopped OR Instant Mode, update Audio immediately
  if (!_playing || _quantizationMode == Q_INSTANT)
  {
    _playingPatternID = patternID;
  }
}

void SequencerModel::nextPattern()
{
  if (currentViewPatternID < MAX_PATTERNS - 1)
  {
    setPattern(currentViewPatternID + 1);
  }
}

void SequencerModel::prevPattern()
{
  if (currentViewPatternID > 0)
  {
    setPattern(currentViewPatternID - 1);
  }
}

void SequencerModel::setQuantization(QuantizationMode mode)
{
  _quantizationMode = mode;
}

void SequencerModel::applyPendingPattern()
{
  _playingPatternID = _nextPatternID;
}

void SequencerModel::setPlayMode(PlayMode mode)
{
  _playMode = mode;
}

// -------------------------------------------------------------------------
// PLAYLIST (SONG) CRUD
// -------------------------------------------------------------------------
int SequencerModel::getPlaylistLength() const { return _playlistLength; }
int SequencerModel::getPlaylistCursor() const { return _playlistCursor; }

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
  if (patternID >= MAX_PATTERNS)
    patternID = 0;
  _playlist[slotIndex] = patternID;
}

void SequencerModel::insertPlaylistSlot(int slotIndex, uint8_t patternID)
{
  if (_playlistLength >= MAX_SONG_LENGTH)
    return;
  if (slotIndex < 0)
    slotIndex = 0;
  if (slotIndex > _playlistLength)
    slotIndex = _playlistLength;
  for (int i = _playlistLength; i > slotIndex; i--)
    _playlist[i] = _playlist[i - 1];
  _playlist[slotIndex] = patternID;
  _playlistLength++;
}

void SequencerModel::deletePlaylistSlot(int slotIndex)
{
  if (_playlistLength <= 1)
    return;
  if (slotIndex < 0 || slotIndex >= _playlistLength)
    return;
  for (int i = slotIndex; i < _playlistLength - 1; i++)
    _playlist[i] = _playlist[i + 1];
  _playlistLength--;
  if (_playlistCursor >= _playlistLength)
    _playlistCursor = _playlistLength - 1;
}

// -------------------------------------------------------------------------
// EDITING
// -------------------------------------------------------------------------
void SequencerModel::toggleStep(int track, int step)
{
  if (track >= NUM_TRACKS || step >= NUM_STEPS)
    return;
  bool current = _patternPool[currentViewPatternID].steps[track][step];
  _patternPool[currentViewPatternID].steps[track][step] = !current;
}

void SequencerModel::clearCurrentPattern()
{
  createSnapshot();
  for (int t = 0; t < NUM_TRACKS; t++)
    for (int s = 0; s < NUM_STEPS; s++)
      _patternPool[currentViewPatternID].steps[t][s] = false;
}

void SequencerModel::clearTrack(int trackID)
{
  if (trackID < 0 || trackID >= NUM_TRACKS)
    return;
  createSnapshot();
  for (int s = 0; s < NUM_STEPS; s++)
    _patternPool[currentViewPatternID].steps[trackID][s] = false;
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
    return _playingPatternID;
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

  if (_currentStep >= NUM_STEPS)
  {
    _currentStep = 0;

    if (_playMode == MODE_SONG)
    {
      _playlistCursor++;
      if (_playlistCursor >= _playlistLength)
      {
        _playlistCursor = 0;
      }
    }
    return true;
  }

  return false;
}