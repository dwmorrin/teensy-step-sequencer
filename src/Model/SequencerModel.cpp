#include "SequencerModel.h"

SequencerModel::SequencerModel()
{
  _bpm = DEFAULT_BPM;
  _playing = false;
  _currentStep = 0;
  _playMode = MODE_PATTERN_LOOP;

  // Default View Settings
  currentViewPatternID = 0;
  activeTrackID = 0;

  // Initialize Playlist (Default: simple loop of Pattern 0)
  playlistLength = 1;
  playlistCursor = 0;
  for (int i = 0; i < MAX_SONG_LENGTH; i++)
  {
    playlist[i] = 0;
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
  playlistCursor = 0; // Reset song position on stop
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
  // When switching modes, we might want to sync the cursor
  // but for now, we leave it independent.
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
  // Copy current view pattern to undo buffer
  _undoBuffer = _patternPool[currentViewPatternID];
}

void SequencerModel::undo()
{
  // Swap buffer with current pattern
  // (This allows "Redo" if you hit undo again)
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
    return playlist[playlistCursor];
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
      // Set bit 't' to 1
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
      playlistCursor++;
      if (playlistCursor >= playlistLength)
      {
        playlistCursor = 0; // Loop Song
      }
    }

    return true; // "We wrapped around" (useful for UI cues)
  }

  return false;
}