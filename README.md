# altenter

Toggle borderless fullscreen with Alt+Enter / F11 in Unity games (x64).

> This is an AI Slop, but least human have touched, and tested on Arknights PC.

> Disclaimer: Use at your own risk. This injects a DLL into game processes, which may violate a game's ToS or trigger anti-cheat. Use only on single-player or offline games.

## Use

1. Copy `version.dll` next to the game exe.
2. Run the game.
3. Press Alt+Enter or F11 to toggle.

Edit `altenter.ini` for hotkeys.

Crash? maybe rename `winmm.dll`? untested btw.

## Build

```bat
cmake -B build -A x64
cmake --build build --config Release
```

Output: `build\Release\version.dll`
