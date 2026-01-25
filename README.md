# Teensy Step Sequencer

This project implements a hardware-based step sequencer using the Teensy 4.1 microcontroller. It utilizes the Teensy's onboard USB Host capabilities to accept input from standard USB keyboards and drives hardware triggers via GPIO.

## System Overview

The application is designed as a standalone, embedded instrument. It functions as a 4-track step sequencer with support for pattern chaining (Song Mode), real-time performance triggers, and visual feedback via an I2C OLED display.

### Hardware Dependencies

- **Microcontroller:** Teensy 4.1 (Selected for native USB Host and RAM capacity).
- **Display:** 1.3" SH1106 OLED (I2C).
- **Input:** Standard USB HID Keyboard.
- **Output:** 5V Logic Triggers

## Software Architecture

The codebase adopts a modular architecture separating data, presentation, and hardware control. The source code located in `src/` is organized into the following domains:

- **Model** (`src/Model/`)
  This directory contains the core logic and data structures. It serves as the single source of truth for the application, managing the Pattern Pool, the Playlist (Song Mode), and the Undo buffer. The Model is decoupled from hardware; it does not know if it is being driven by a clock or edited by a keyboard.

- **View** (`src/View/`)
  This directory handles all visual output. The `DisplayManager` observes the Model and renders the interface (Grid, Transport, Headers) to the OLED. It includes internal throttling logic to maintain a consistent frame rate without blocking the audio/timing threads.

- **Controller** (`src/Controller/`)
  This directory manages user input and state routing. The `UIManager` interprets raw USB key codes and translates them into system commands. It implements a state machine to switch between context-specific modes (e.g., Step Editing vs. Performance triggering).

- **Engine** (`src/Engine/`)
  This directory handles real-time operations.
  - `ClockEngine`: Manages the BPM timer and advances the Model's playhead.
  - `OutputDriver`: Abstraction layer for the physical hardware. It converts track bitmasks into GPIO signals. This layer is isolated to facilitate future migration to Shift Registers or MIDI output without refactoring the core logic.

- **Configuration** (`src/Config.h`)
  A global header file defining build-time constraints, including track count, pattern limits, and pin mappings.

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

### Input Mapping

Specific key bindings are defined within the `UIManager`. This class maps ASCII codes to internal commands. New input methods (e.g., encoders, MIDI input) should be implemented by routing their signals through the `UIManager` to ensure consistent state handling.

## BPM calculation

The "beat" is implicitly quarter notes, and the system is designed for you to program 16 sixteenth notes per measure.

The math: we want to work out milliseconds per sixteenth note (or sixteenth note period).

This is (60,000 milliseconds/minute)\*(minutes/quarter note beat)\*(1 quarter note beat/4 sixteenth note beats).

This can be simplified to

15,000 = (sixteenth note period) \* (BPM)

from which we can note that a default trigger period of 15 ms is equivalent to 1000 BPM (ridiculously fast) so 15 ms is a good trigger size that won't "overlap" with a sixteenth note beat period.
