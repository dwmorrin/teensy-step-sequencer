#pragma once
#include <Arduino.h>

// --- SYSTEM LIMITS ---
#define NUM_TRACKS 4
#define NUM_STEPS 16
#define MAX_PATTERNS 64
#define MAX_SONG_LENGTH 128

// --- TIMING ---
#define PULSE_WIDTH_MS 15
#define DEFAULT_BPM 120

// --- HARDWARE MAPPING ---
const int OUTPUT_MAP[NUM_TRACKS] = {25, 26, 27, 28};

// --- INPUTS ---
const int PIN_POT_TEMPO = 14;
const int PIN_POT_PARAM = 15;

// KEY MATRIX PINS
// Rows (Active Low Output)
const int PIN_ROW_1 = 36;
const int PIN_ROW_2 = 34;
const int PIN_ROW_3 = 38;
const int PIN_ROW_4 = 40;

// Cols (Input Pullup)
const int PIN_COL_1 = 20;
const int PIN_COL_2 = 17;
const int PIN_COL_3 = 16;
const int PIN_COL_4 = 41;
const int PIN_COL_5 = 39;
const int PIN_COL_6 = 37;
const int PIN_COL_7 = 35;
const int PIN_COL_8 = 33;

// PCB REVISION NOTES
const bool POT_INVERT_POLARITY = true;

// --- OUTPUT POLARITY ---
#define TRIGGER_ON HIGH
#define TRIGGER_OFF LOW

// --- DISPLAY ---
#define TRACK_EDIT_STR "<"
#define TRACK_PERFORM_STR "*"