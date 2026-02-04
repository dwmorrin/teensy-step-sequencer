#include "Debug.h"
#include <Arduino.h>
#include <USBHost_t36.h>

// Core Components
#include "Model/SequencerModel.h"
#include "View/DisplayManager.h"
#include "Controller/UIManager.h"
#include "Engine/OutputDriver.h"
#include "Engine/ClockEngine.h"

// --- USB HOST SETUP ---
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb); // Support for daisy-chained hubs
KeyboardController keyboard1(myusb);

// --- COMPONENT INSTANTIATION ---
// Hardware Definitions
const int PIN_SR_LATCH = 10;

SequencerModel model;
OutputDriver driver;
ClockEngine clockEngine(model, driver);
UIManager ui(model, driver, clockEngine);

// DisplayManager now receives the Latch Pin for the LEDs
DisplayManager display(model, ui, PIN_SR_LATCH);

// --- FORWARD DECLARATION ---
void globalKeyPress(int key);

// --- SETUP ---
void setup()
{
  // Optional: Serial for debugging (remove if standalone)
  // Serial.begin(9600);

  // 1. Init Subsystems
  driver.init();
  display.init(); // Inits OLED and LEDs
  ui.init();

  // 2. Init USB
  myusb.begin();
  keyboard1.attachPress(globalKeyPress);

  clockEngine.init();
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
  myusb.Task();

  // 2. TIMING ENGINE
  clockEngine.update();

  // 3. INTERFACE
  ui.processInput();
  display.update();
}