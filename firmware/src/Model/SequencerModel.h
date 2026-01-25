#pragma once
#include <Arduino.h>
#include "Config.h"

struct Pattern
{
  bool steps[NUM_TRACKS][NUM_STEPS];
};

enum PlayMode
{
  MODE_PATTERN_LOOP, // Loop the current View Pattern
  MODE_SONG          // Play through the Playlist
};

class SequencerModel
{
public:
  SequencerModel();

  // --- TRANSPORT ---
  void play();
  void stop();
  bool isPlaying() const { return _playing; }

  // --- NAVIGATION (VIEW) ---
  int currentViewPatternID;
  int activeTrackID;

  void nextPattern();
  void prevPattern();

  // --- PLAYLIST (SONG) ---
  void setPlayMode(PlayMode mode);
  PlayMode getPlayMode() const { return _playMode; }

  int getPlaylistLength() const;
  int getPlaylistCursor() const;

  // CRUD Operations for Playlist
  uint8_t getPlaylistPattern(int slotIndex) const;
  void setPlaylistPattern(int slotIndex, uint8_t patternID);
  void insertPlaylistSlot(int slotIndex, uint8_t patternID);
  void deletePlaylistSlot(int slotIndex);

  // --- EDITING ---
  // Toggles a step in the CURRENTLY VIEWED pattern
  void toggleStep(int track, int step);
  void clearCurrentPattern();
  void clearTrack(int trackId);

  // --- UNDO SYSTEM ---
  void createSnapshot();
  void undo();

  // --- ENGINE INTERFACE ---
  // Returns a bitmask of triggers for the specific step in the specific pattern.
  uint16_t getTriggersForStep(int patternID, int step);

  // Logic helper for the View to know what is actually making sound
  int getPlayingPatternID() const;

  // Advances the internal step counter.
  // Returns TRUE if we just wrapped around (useful for UI updates)
  bool advanceStep();

  // Getters for the Engine/View
  int getCurrentStep() const { return _currentStep; }

  // --- TEMPO ---
  void setBPM(int bpm);
  int getBPM() const;

private:
  Pattern _patternPool[MAX_PATTERNS];
  Pattern _undoBuffer;

  // Playlist State
  uint8_t _playlist[MAX_SONG_LENGTH];
  int _playlistLength;
  int _playlistCursor;

  bool _playing;
  int _currentStep;
  PlayMode _playMode;
  int _bpm;
};