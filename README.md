# Real Time Typing Tutor using Custom C Libraries

## Overview

This is a terminal-based real time typing tutor written in C. It integrates custom libraries for memory management, string processing, math computations, keyboard input, and screen rendering.

## Architecture

Library integration flow:

`keyboard.c -> string_utils.c -> memory.c -> math_utils.c -> screen.c`

- `keyboard.c`: Non-blocking keystroke capture in raw terminal mode.
- `string_utils.c`: Character and text comparison for typing accuracy.
- `memory.c`: Dynamic memory allocation and buffer handling.
- `math_utils.c`: Accuracy and WPM calculations.
- `screen.c`: Dynamic terminal UI rendering.

## Build and Run

```bash
make
./typing_tutor
```

Or load target text dynamically from a file:

```bash
./typing_tutor sample.txt
```

## Features

- Real time input capture and screen refresh.
- Live accuracy and words-per-minute metrics.
- Dynamic input buffer with overflow protection.
- Backspace handling and safe input boundaries.
- Proper resource cleanup.

## Project Rules Compliance

- No usage of `string.h` or `math.h`.
- String and math logic implemented in custom libraries.
- Dynamic memory operations centralized in `memory.c`.
- Real time loop and terminal rendering implemented manually.
# Typing-Tutor-OS
