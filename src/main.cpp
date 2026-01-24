#include <Arduino.h>
#include <USBHost_t36.h>

// Core Components
#include "Config.h"
#include "Model/SequencerModel.h"
#include "View/DisplayManager.h"
#include "Controller/UIManager.h"
#include "Engine/OutputDriver.h"
#include "Engine/ClockEngine.h" // Now it exists!

// --- USB HOST SETUP ---
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb); // Support for daisy-chained hubs
KeyboardController keyboard1(myusb);

// --- COMPONENT INSTANTIATION ---
SequencerModel model;
OutputDriver driver;
ClockEngine clockEngine(model, driver); // Connects Brain + Hardware
DisplayManager display(model);          // Connects Brain -> Screen
UIManager ui(model, driver);            // Connects User -> Brain/Hardware

// --- FORWARD DECLARATION ---
void globalKeyPress(int key);

// --- SETUP ---
void setup()
{
  // Optional: Wait for Serial for debugging (remove if standalone)
  // Serial.begin(9600);

  // 1. Init Subsystems
  driver.init();
  display.init();
  ui.init();

  // 2. Init USB
  myusb.begin();
  keyboard1.attachPress(globalKeyPress);

  // 3. Set Initial Tempo (Optional, as Constructor does it too)
  clockEngine.setBPM(120);
}

// --- GLOBAL BRIDGE ---
// USBHost library requires a void function(int), not a class method.
void globalKeyPress(int key)
{
  ui.handleKeyPress(key);
}

// --- MAIN LOOP ---
void loop()
{
  // 1. HARDWARE TASKS
  myusb.Task(); // Poll USB bus

  // 2. TIMING ENGINE
  clockEngine.run(); // Check BPM timer -> Fire Triggers

  // 3. INTERFACE
  ui.processInput(); // Poll inputs
  display.update();  // Draw screen (internally throttled to ~30fps)
}