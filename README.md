# Teensy Step Sequencer

This project implements a hardware-based step sequencer using the Teensy 4.1 microcontroller. It utilizes the Teensy's onboard USB Host capabilities to accept input from standard USB keyboards, alongside a custom 32-switch diode matrix for tactile control.

## Gallery

### Prototype

![Breadboard Setup](docs/images/prototype-breadboard-setup.jpg)

### Hardware Renders

|                       Main Board                        |                        Switchplate                        |
| :-----------------------------------------------------: | :-------------------------------------------------------: |
| ![Main Board Render](docs/images/main-board-render.png) | ![Switchplate Render](docs/images/switchplate-render.png) |

## System Overview

The application is designed as a standalone, embedded instrument. It functions as a 4-track step sequencer with support for pattern chaining (Song Mode), real-time performance triggers, and visual feedback via an I2C OLED display and a dedicated LED step array.

### Hardware Dependencies

- **Microcontroller:** Teensy 4.1 (Selected for native USB Host and RAM capacity).
- **Display:** - 1.3" SH1106 OLED (I2C) for the graphical interface.
  - 16x LEDs driven by 74HC595 Shift Registers for step visualization.
- **Input:** - Standard USB HID Keyboard (via USB Host).
  - 4x8 Diode Matrix Keyboard (32 switches).
  - 2x Analog Potentiometers (Tempo and Parameter control).
- **Output:** 4x 5V Logic Triggers (GPIO).

## Software Architecture

The codebase adopts a modular architecture separating data, presentation, and hardware control. The source code located in `src/` is organized into the following domains:

- **Model** (`src/Model/`)
  This directory contains the core logic and data structures. It serves as the single source of truth for the application, managing the Pattern Pool, the Playlist (Song Mode), and the Undo buffer. The Model is decoupled from hardware; it does not know if it is being driven by a clock or edited by a keyboard.

- **View** (`src/View/`)
  This directory handles all visual output.
  - `DisplayManager`: Observes the Model and renders the interface (Grid, Transport, Headers) to the OLED.
  - `StepLeds`: Manages the 16-step LED array via SPI shift registers.
    The view layer includes internal throttling logic to maintain a consistent frame rate without blocking the audio/timing threads.

- **Controller** (`src/Controller/`)
  This directory manages user input and state routing. The `UIManager` aggregates inputs from multiple hardware sources:
  - **USB Keyboard:** Handled via the Teensy USBHost_t36 library.
  - **Matrix:** Handled by `KeyMatrix`, a custom driver for scanning the 4x8 diode array.
  - **Analog:** Handled by `AnalogInput`, which provides hysteresis and signal smoothing for potentiometers.

  Inputs are translated into abstract `InputCommands` (e.g., `CMD_TRANSPORT_TOGGLE`), decoupling the physical control method from the internal application logic.

- **Engine** (`src/Engine/`)
  This directory handles real-time operations.
  - `ClockEngine`: Manages the BPM timer and advances the Model's playhead.
  - `OutputDriver`: Abstraction layer for the physical hardware. It converts track bitmasks into GPIO signals using high-precision interrupt timers.

- **Configuration** (`src/Config.h`)
  A global header file defining build-time constraints, including track count, pattern limits, pin mappings, and signal polarity.

## Expansion Guidelines

The system is designed to be extensible. Developers looking to expand functionality should consider the following architectural patterns:

### Increasing Track Count

The system currently defaults to 4 tracks. To expand this:

1.  Update the `NUM_TRACKS` definition in `Config.h`.
2.  Update the hardware mapping in `src/Engine/OutputDriver`.
    The Model and View logic are written dynamically and will automatically adjust to handle 8 or 16 tracks.

### Persistence (SD Card Storage)

Current pattern data is stored in volatile RAM and is lost upon power cycle. The Teensy 4.1 includes a native micro-SD card slot. Future development should implement a `PersistenceManager` class. This component should:

1.  Serialize the `SequencerModel` data structures (Pattern Pool and Playlist) into a binary or JSON format.
2.  Write to the SD card using the standard Arduino SD library.
3.  Interface with the `UIManager` to allow loading and saving of projects.

## Timing Calculation

The system timing is based on a standard 4/4 time signature where a "beat" represents a quarter note. The sequencer resolution is 16th notes (4 steps per beat).

The interval $T$ (milliseconds per step) is calculated as:

$T = \frac{60,000}{BPM \times 4} = \frac{15,000}{BPM}$

For a standard trigger pulse width of 15ms, the maximum theoretical tempo before pulse overlap is 1000 BPM.
