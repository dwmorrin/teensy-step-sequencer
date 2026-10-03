#include "Debug.h"
#include <Arduino.h>
#include <USBHost_t36.h>

// Core Components
#include "Model/SequencerModel.h"
#include "View/DisplayManager.h"
#include "Controller/UIManager.h"
#include "Engine/OutputDriver.h"
#include "Engine/ClockEngine.h"
#include "StorageManager/StorageManager.h"

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
StorageManager storage;
UIManager ui(model, driver, clockEngine, storage);

// DisplayManager now receives the Latch Pin for the LEDs
DisplayManager display(model, ui, PIN_SR_LATCH);

// --- FORWARD DECLARATION ---
void globalKeyPress(int key);

// --- SETUP ---
void setup()
{
#ifdef DEBUG_MODE
  Serial.begin(9600);
#endif

  // 1. Init Subsystems
  driver.init();
  display.init(); // Inits OLED and LEDs
  ui.init();

  if (storage.init())
    storage.loadState(model);

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
  // HARDWARE TASKS
  myusb.Task();

  // TIMING ENGINE
  clockEngine.update();

  // INTERFACE
  ui.processInput();
  display.update();

  // STORAGE
  if (ui.isSavePending())
    ui.executePendingSave();
  if (ui.isLoadPending())
    ui.executePendingLoad();
}