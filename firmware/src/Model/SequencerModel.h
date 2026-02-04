#pragma once
#include <Arduino.h>
#include "Config.h"

struct Pattern
{
  bool steps[NUM_TRACKS][NUM_STEPS];
};

enum PlayMode
{
  MODE_PATTERN_LOOP,
  MODE_SONG,
  MODE_HARDWARE_TEST
};

enum QuantizationMode
{
  Q_BAR,     // Wait for Step 1
  Q_QUARTER, // Wait for Step 1, 5, 9, 13
  Q_EIGHTH,  // Wait for Step 1, 3, 5...
  Q_INSTANT  // Switch Immediately
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
  int currentViewPatternID; // What the User SEES
  int activeTrackID;

  void nextPattern();
  void prevPattern();
  void setPattern(int patternID);

  // --- QUANTIZATION ---
  void setQuantization(QuantizationMode mode);
  QuantizationMode getQuantization() const { return _quantizationMode; }

  // Transition Logic
  int getPendingPatternID() const { return _nextPatternID; }
  void applyPendingPattern();

  // --- PLAYLIST (SONG) ---
  void setPlayMode(PlayMode mode);
  PlayMode getPlayMode() const { return _playMode; }

  int getPlaylistLength() const;
  int getPlaylistCursor() const;

  uint8_t getPlaylistPattern(int slotIndex) const;
  void setPlaylistPattern(int slotIndex, uint8_t patternID);
  void insertPlaylistSlot(int slotIndex, uint8_t patternID);
  void deletePlaylistSlot(int slotIndex);

  // --- EDITING ---
  void toggleStep(int track, int step);
  void clearCurrentPattern();
  void clearTrack(int trackId);

  // --- UNDO SYSTEM ---
  void createSnapshot();
  void undo();

  // --- ENGINE INTERFACE ---
  uint16_t getTriggersForStep(int patternID, int step);
  int getPlayingPatternID() const;

  bool advanceStep();
  int getCurrentStep() const { return _currentStep; }

  // --- TEMPO ---
  void setBPM(int bpm);
  int getBPM() const;

private:
  Pattern _patternPool[MAX_PATTERNS];
  Pattern _undoBuffer;

  uint8_t _playlist[MAX_SONG_LENGTH];
  int _playlistLength;
  int _playlistCursor;

  bool _playing;
  int _currentStep;
  PlayMode _playMode;
  int _bpm;

  QuantizationMode _quantizationMode;
  int _playingPatternID;
  int _nextPatternID;
};