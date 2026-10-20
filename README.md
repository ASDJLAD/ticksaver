# Tick Saver (Geode mod)

A frame-saver / input-buffer mod for Geometry Dash 2.2081 (Geode 5.11).
Press **F6** to cycle the tick save: off, 1, 2, 4, 8. The value is saved between sessions.

> Do not use this on completions you submit to lists or leaderboards.

## What it does

A tick is one physics step (`GJGameState::m_currentProgress` goes up by one per step).

- **Early press buffer.** A jump pressed while it cannot be used (in the air, not on an orb) is
  remembered. If the player lands or touches an orb within N ticks, the press is applied then.
- **Extend holds** (off by default). Every release is delayed by N ticks. A press in that window
  cancels the release, so the hold simply continues.
- **State tracking and overlay.** Shows the current tick save, the ticks used by the last save
  (and what kind: ground / orb / hold), and the total ticks and number of saves in this attempt.

## What it cannot do

A click that comes *after* the frame where it was needed (a late click) cannot be saved. That
would need the game to rewind its state to the earlier frame, and this mod does not do that.

## Files

```
mod.json            metadata and settings
CMakeLists.txt      build config
src/main.cpp        hooks: F6 key, GJBaseGameLayer::handleButton / processCommands, PlayLayer
src/State.hpp       tick-save value, counters
src/Hud.hpp         overlay
.github/workflows/  CI build (Windows)
```

## Hooks (GD 2.2081)

- `CCKeyboardDispatcher::dispatchKeyboardMSG(enumKeyCodes, bool, bool, double)` for F6
- `GJBaseGameLayer::handleButton(bool, int, bool)` to catch presses and releases
- `GJBaseGameLayer::processCommands(float, bool, bool)` once per physics step
- `PlayLayer::setupHasCompleted()` and `PlayLayer::resetLevel()` for the overlay and counters

## Build

Install the Geode SDK + CLI and run `geode build`, or push to GitHub and use the included
workflow. Change `id` and `developer` in `mod.json` before sharing.
