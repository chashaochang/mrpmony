# Compatibility Test Plan

Date: 2026-05-09

## Goal

Use a small, repeatable matrix to decide whether a package is compatible, partially compatible, or blocked by a specific missing emulator feature.

## Test Matrix

Start with 5 to 10 legally owned `.mrp` packages that cover different behavior:

- Menu/list UI package.
- Text-heavy package.
- Sprite-heavy game.
- Package with multiple internal resource pages.
- Package with save/config writes.
- Package with audio playback.
- Package using soft keys.
- Package using directional keys heavily.

## Per-Package Checklist

For each package, record:

- Package filename.
- Launch result.
- First frame rendered.
- Main menu visible.
- Direction keys work.
- OK/select works.
- Soft-left and soft-right work.
- Touch works if the package expects pointer input.
- Resource pages are complete.
- Audio behavior.
- Save/config behavior.
- Exit returns to app without killing process.
- Restart works after exit.

## Log Markers To Capture

Always capture these if a package fails:

```text
Start workDir=... package=... ext=start.mr
startVmrpWithPath ret=...
timer pump ...
guiDrawBitmap ...
copyFrame ...
MRPDBG readFile pack-hit ...
MRPDBG readFile gzip-ok ...
MRPDBG readFile miss ...
input #... key action=...
```

## Classification

Use these labels:

- `pass`: package can be launched, played, exited, and restarted.
- `minor`: playable with visual/audio/input issues that do not block progress.
- `partial`: launches but core game flow is blocked.
- `fail`: cannot launch or exits immediately.
- `crash`: app process or native runtime crashes.

## Debugging Order

When a package fails, check in this order:

1. Did the correct package start?
2. Did `_mr_readFile` find `start.mr`?
3. Were gzip entries decompressed?
4. Did the first frame draw?
5. Are input events reaching native?
6. Did timer pumping continue after startup?
7. Did the package call an unimplemented bridge API?

Avoid changing UI routing or the app list unless step 1 proves the wrong package was started.

