#pragma once
#include <Arduino.h>

// Uncomment this line to enable debug logs globally
// #define DEBUG_MODE

#ifdef DEBUG_MODE
// "Variadic Macros" - acts just like Serial.printf()
#define LOG(...) Serial.printf(__VA_ARGS__)
#define LOGLN(...)            \
  Serial.printf(__VA_ARGS__); \
  Serial.println()
#else
// If disabled, these macros evaporate into nothingness
#define LOG(...)
#define LOGLN(...)
#endif