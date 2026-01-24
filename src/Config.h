#pragma once
#include <Arduino.h>

// --- SYSTEM LIMITS ---
#define NUM_TRACKS 4 // Flexible: change to 8 or 16 later
#define NUM_STEPS 16
#define MAX_PATTERNS 64     // The "Pool" of unique beats
#define MAX_SONG_LENGTH 128 // The "Playlist" sequence

// --- TIMING ---
#define PULSE_WIDTH_MS 15 // How long the +5V trigger stays HIGH
#define DEFAULT_BPM 120

// --- HARDWARE MAPPING ---
// Maps logical Track IDs (0-3) to physical Teensy Pins
// We use a const array so we can easily swap this for Shift Register logic later
const int OUTPUT_MAP[NUM_TRACKS] = {2, 3, 4, 5};

// --- OUTPUT POLARITY ---

// OPTION A: Direct Drive (LEDs, 74HCT245, 74HC595)
// #define TRIGGER_ON HIGH
// #define TRIGGER_OFF LOW

// OPTION B: Inverted Drive (Transistors, 74LS04 inverter)
#define TRIGGER_ON LOW
#define TRIGGER_OFF HIGH