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
// Maps logical Track IDs (0-3) to physical Teensy Pins
const int OUTPUT_MAP[NUM_TRACKS] = {2, 3, 4, 5};

// --- INPUTS ---
const int PIN_POT_TEMPO = 14;
const int PIN_POT_PARAM = 15;

// PCB REVISION NOTES
// V1 Error: Pots are wired backwards (CW = 0V, CCW = 3.3V).
// Set to true for V1. Set to false for V2 (Corrected).
const bool POT_INVERT_POLARITY = true;

// --- OUTPUT POLARITY ---
// OPTION A: Direct Drive (Probing / LEDs)
#define TRIGGER_ON HIGH
#define TRIGGER_OFF LOW

// OPTION B: Inverted Drive (Transistors)
// #define TRIGGER_ON LOW
// #define TRIGGER_OFF HIGH

// --- DISPLAY ---
#define TRACK_EDIT_STR "<"
#define TRACK_PERFORM_STR "*"