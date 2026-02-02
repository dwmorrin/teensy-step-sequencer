#pragma once

enum InputCommand
{
  CMD_NONE,

  // GLOBAL / TRANSPORT
  CMD_TRANSPORT_TOGGLE, // Space
  CMD_MODE_TOGGLE,      // Tab
  CMD_SONG_MODE_TOGGLE, // Enter
  CMD_TEST_TOGGLE,      // 't'
  CMD_BPM_ENTER,        // 'b'
  CMD_UNDO,             // Backspace

  // NAVIGATION
  CMD_PATTERN_PREV,  // '['
  CMD_PATTERN_NEXT,  // ']'
  CMD_TRACK_PREV,    // Up Arrow
  CMD_TRACK_NEXT,    // Down Arrow
  CMD_PLAYLIST_PREV, // Left Arrow
  CMD_PLAYLIST_NEXT, // Right Arrow

  // STEPS / TRIGGERS (1-16)
  // Mapped to 1-4, q-r, a-f, z-v on QWERTY
  // Mapped to Row 1 (1-16) on Matrix
  CMD_TRIGGER_1,
  CMD_TRIGGER_2,
  CMD_TRIGGER_3,
  CMD_TRIGGER_4,
  CMD_TRIGGER_5,
  CMD_TRIGGER_6,
  CMD_TRIGGER_7,
  CMD_TRIGGER_8,
  CMD_TRIGGER_9,
  CMD_TRIGGER_10,
  CMD_TRIGGER_11,
  CMD_TRIGGER_12,
  CMD_TRIGGER_13,
  CMD_TRIGGER_14,
  CMD_TRIGGER_15,
  CMD_TRIGGER_16,

  // PLAYLIST EDITING
  CMD_PLAYLIST_INSERT, // 'i'
  CMD_PLAYLIST_DELETE, // 'x'

  // CONFIRMATION MODALS
  CMD_CLEAR_PROMPT, // '#'
  CMD_CONFIRM_YES,  // 'y'
  CMD_CONFIRM_NO    // 'n' / Esc
};