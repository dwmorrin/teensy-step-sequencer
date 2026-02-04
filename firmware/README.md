# 8-Track Performance Sequencer (Firmware v1.0.0)

A interrupt-driven step sequencer for Teensy 4.1.

## Features

- **8-Track Polyphony:** 8 independent trigger outputs.
- **96 PPQN Timing Engine:** High-resolution jitter-free timing based on hardware interrupts.
- **Groove Engine:** Per-track Swing (0-100%) with visual grid feedback.
- **Performance Quantization:** Launch patterns synced to 1 Bar, 1/4 Note, 1/8 Note, or Instant.
- **Song Mode:** Chained pattern playback with insert/delete editing.
- **Scrolling Interface:** 128x64 OLED UI with auto-scrolling track view and "Gutter" labels.

## Controls

### Global & Transport

| Button   | Function                | Shift Function                      |
| :------- | :---------------------- | :---------------------------------- |
| **PLAY** | Play / Stop             | **Quantize Menu** (Set Launch Mode) |
| **MODE** | Toggle Song / Loop Mode | Toggle Edit / Perform Mode          |
| **BPM**  | Enter BPM Menu          | -                                   |
| **UNDO** | -                       | **Undo Last Action** (Shift + H)    |

### Navigation & Selection

| Button    | Function                             |
| :-------- | :----------------------------------- |
| **A - H** | Select Active Track (1 - 8)          |
| **< / >** | Previous / Next Pattern              |
| **^ / v** | Previous / Next Track (Scrolls View) |

### Editing (Step Edit Mode)

| Control           | Action                           |
| :---------------- | :------------------------------- |
| **Steps 1-16**    | Toggle Step ON/OFF               |
| **Param Pot**     | _Unused in Loop Mode_            |
| **Shift + Param** | **Set Swing %** for Active Track |

### Performance (Perform Mode)

| Control       | Action                           |
| :------------ | :------------------------------- |
| **Steps 1-4** | Manual Trigger / Finger Drumming |

### Song Mode (Playlist)

| Control        | Action                          | Shift Action             |
| :------------- | :------------------------------ | :----------------------- |
| **Param Pot**  | Select Pattern for current slot | -                        |
| **< / >**      | Move Cursor / Scroll Playlist   | Insert Slot Before/After |
| **Clear**      | Delete Current Slot             | -                        |
| **Steps 1-16** | Quick Select Pattern Bank       | Select Bank 1-4          |

## Architecture

- **Model:** `SequencerModel` holds the state (Patterns, Playlist, Swing). It is decoupled from the engine.
- **Engine:** `ClockEngine` runs at **2kHz** (0.5ms interval), accumulating time to drive a **96 PPQN** virtual clock. It handles swing delays and trigger pulse widths.
- **Controller:** `UIManager` maps a 4x8 Matrix and Analog Inputs to Commands.
- **View:** `DisplayManager` renders the state to an SSD1306 OLED, handling scrolling offsets and overlays.

## Hardware Map

- **Outputs 1-8:** Pins 25-32
- **Tempo Pot:** Pin 14
- **Param Pot:** Pin 15
- **Matrix Rows:** 36, 34, 38, 40 (Active Low)
- **Matrix Cols:** 20, 17, 16, 41, 39, 37, 35, 33 (Input Pullup)
