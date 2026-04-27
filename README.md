# Real-Time Typing Tutor Using Custom C Libraries

A terminal-based, Monkeytype-inspired typing tutor written in C with custom libraries for keyboard input, string logic, memory management, math metrics, and screen rendering.

## What This Project Does

The app runs fully in terminal and provides:

- Setup screen with selectable timer (`15/30/60/120/300` seconds)
- Difficulty selection (`Easy`, `Medium`, `Hard`)
- Dynamic random word stream generation by difficulty
- Real-time typing without pressing Enter
- Live WPM, accuracy, error count, word stats, and streaks
- Instant controls: `TAB` restart, `ESC` quit
- Final summary screen with personal-best WPM (per program run)

## Tech Stack

- Language: C (`-std=c11`)
- Build: `Makefile`
- Runtime: POSIX terminal APIs (`termios`, `fcntl`, `unistd`)

## Project Structure

- `src/main.c`: session flow, setup loop, typing loop, summary flow
- `src/keyboard.c`: raw non-canonical, non-blocking keyboard handling
- `src/string_utils.c`: custom string helpers and correctness counting
- `src/memory.c`: custom pool allocator (`mem_alloc`, `mem_free`, `mem_resize`, etc.)
- `src/math_utils.c`: accuracy, WPM, and safe min helper
- `src/screen.c`: terminal setup/live/summary rendering
- `include/*.h`: library interfaces
- `GUIDE.md`: full implementation and viva handbook

## Build and Run

```bash
make
./typing_tutor
```

Optional:

```bash
make run
make clean
```

## Runtime Controls

### Setup Screen

- `1` to `5`: select timer mode
- `E` / `M` / `H`: select difficulty
- `ENTER`: start test
- `ESC`: quit

### Live Typing Screen

- Any printable key: type
- `BACKSPACE` / `DELETE`: erase one character
- `TAB`: restart immediately with same settings
- `ESC`: quit

### Summary Screen

- `ENTER`: return to setup
- `ESC`: quit
- Any other key (including `TAB`): restart with same settings

## Real-Time Flow

The core loop follows:

`keyboard -> string -> memory -> math -> screen -> repeat`

In practice:

1. Read key in non-blocking mode.
2. Update input buffer / control actions.
3. Extend target stream and/or input buffer if needed.
4. Compute live stats (correct chars, errors, WPM, accuracy, word progress, streaks).
5. Re-render terminal UI.

## Custom Library Integration

`main.c` orchestrates all custom libraries:

- `keyboard.c` provides immediate key capture.
- `memory.c` manages dynamic buffers from a custom memory pool.
- `string_utils.c` compares input vs target for correctness.
- `math_utils.c` converts counts/time into user metrics.
- `screen.c` displays setup, live test, and summary screens.

Removing any one of these libraries breaks a required part of the real-time typing workflow.

## Notes

- The timer starts only on the first valid non-space typed character.
- Input and target buffers are resized safely to prevent overflow.
- Terminal state is restored on exit via `keyboard_shutdown`.

## Browser Files

`index.html` and `practice_sentences.js` are included as a separate frontend demo and are not wired to the terminal C engine.
