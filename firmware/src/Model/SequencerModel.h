#pragma once
#include <Arduino.h>
#include "Config.h"

struct Pattern
{
  bool steps[NUM_TRACKS][NUM_STEPS];
  uint8_t trackSwing[NUM_TRACKS]; // 0 (50%) to 100 (75%)
};

enum PlayMode
{
  MODE_PATTERN_LOOP,
  MODE_SONG,
  MODE_HARDWARE_TEST
};
enum QuantizationMode
{
  Q_BAR,
  Q_QUARTER,
  Q_EIGHTH,
  Q_INSTANT
};

class SequencerModel
{
public:
  SequencerModel();

  // --- TRANSPORT ---
  void play();
  void stop();
  bool isPlaying() const { return _playing; }

  // --- NAVIGATION ---
  int currentViewPatternID;
  int activeTrackID;

  void nextPattern();
  void prevPattern();
  void setPattern(int patternID);

  // --- GROOVE / SWING (NEW) ---
  // Sets swing (0-100) for the Active Track in the Current Pattern
  void setTrackSwing(int trackID, uint8_t swingValue);
  uint8_t getTrackSwing(int trackID) const;

  // Helper for the Engine to get swing for the PLAYING pattern
  uint8_t getPlayingTrackSwing(int trackID) const;

  // --- QUANTIZATION ---
  void setQuantization(QuantizationMode mode);
  QuantizationMode getQuantization() const { return _quantizationMode; }
  int getPendingPatternID() const { return _nextPatternID; }
  void applyPendingPattern();

  // --- PLAYLIST ---
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

  // --- UNDO ---
  void createSnapshot();
  void undo();

  // --- ENGINE INTERFACE ---
  uint16_t getTriggersForStep(int patternID, int step);
  int getPlayingPatternID() const;

  // Returns TRUE if we wrapped a bar
  bool advanceTick();

  int getCurrentStep() const { return _currentStep; }
  int getCurrentTick() const { return _currentTick; }

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

  // PPQN Counter
  int _currentTick; // 0 to 23 (for 16th notes at 96 PPQN)

  PlayMode _playMode;
  int _bpm;

  QuantizationMode _quantizationMode;
  int _playingPatternID;
  int _nextPatternID;
};