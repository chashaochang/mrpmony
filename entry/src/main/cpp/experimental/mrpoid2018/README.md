# mrp2018 runtime experiment

This directory is an isolated experiment for evaluating a HarmonyOS port of
the mrpoid2018 native runtime.

It is intentionally not wired into the production `mrp_napi` target yet. The
current product build still uses the existing runtime path. Keeping this code
out of the main CMake target lets us measure and port the mrp2018 code without
breaking the working emulator.

## Layout

- `origin/`: copied mrpoid2018 native sources used as a read-only reference
  snapshot. Prebuilt `.a`/`.so` binaries were not copied.
- `Mrpoid2018Backend.*`: a HarmonyOS-facing backend skeleton with the same
  lifecycle shape as the current runtime.
- `PORTING_NOTES.md`: current blockers and porting order.

## Rules

Do not modify files under `origin/` directly. Put HarmonyOS-specific edits in a
separate port layer so we can compare behavior against the original source and
audit what changed.

Do not ship this backend until licensing is clarified. The upstream project is
described as open source, but this local copy does not include a clear license
covering the native runtime and commercial distribution.
