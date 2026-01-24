#pragma once
#include <Arduino.h>
#include "Config.h"

// A single 16-step pattern for all tracks
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
  // Controls what the user is seeing/editing
  int currentViewPatternID;
  int activeTrackID;

  // *** MISSING DECLARATIONS ADDED HERE ***
  void nextPattern(); // Increment view pattern
  void prevPattern(); // Decrement view pattern

  // --- PLAYLIST (SONG) ---
  uint8_t playlist[MAX_SONG_LENGTH];
  int playlistLength;
  int playlistCursor; // Which index of the playlist are we playing?
  void setPlayMode(PlayMode mode);
  PlayMode getPlayMode() const { return _playMode; }

  // --- EDITING ---
  // Toggles a step in the CURRENTLY VIEWED pattern
  void toggleStep(int track, int step);
  void clearCurrentPattern();

  // --- UNDO SYSTEM ---
  void createSnapshot(); // Saves current View Pattern to undo buffer
  void undo();           // Restores undo buffer to View Pattern

  // --- ENGINE INTERFACE ---
  // The Clock Engine calls this to know what to play.
  // Returns a bitmask of triggers for the specific step in the specific pattern.
  uint16_t getTriggersForStep(int patternID, int step);

  // Logic helper for the View to know what is actually making sound
  int getPlayingPatternID() const;

  // Advances the internal step counter.
  // Returns TRUE if we just wrapped around (useful for UI updates)
  bool advanceStep();

  // Getters for the Engine/View
  int getCurrentStep() const { return _currentStep; }

private:
  Pattern _patternPool[MAX_PATTERNS];
  Pattern _undoBuffer; // Single level undo

  bool _playing;
  int _currentStep;
  PlayMode _playMode;
};