# marquee_console

Windows-only C++ immediate-mode marquee console.

Build (using MinGW/g++ or similar):

```bash
g++ -std=c++20 -O2 -o marquee.exe src/main.cpp
```

Run:

```bash
./marquee.exe
```

Controls:
- Type characters to edit the input line.
- Backspace removes the previous character.
- Enter commits the current line to the top of the history stack (newest-first).
- Escape exits the program.

Notes on polling and refresh:
- The app samples keyboard input once per frame by default and caps rendering to ~60 Hz.
- Tuning: if you see input lag or screen tearing, try adjusting the frame sleep in `src/main.cpp` (the `targetFrame` value) — lowering it increases CPU use but can reduce perceived tearing; increasing it reduces CPU and may make typing more responsive depending on your hardware.


