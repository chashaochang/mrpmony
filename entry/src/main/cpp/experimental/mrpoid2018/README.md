# mrp2018 runtime experiment

This directory is an isolated experiment for evaluating a HarmonyOS port of
the mrpoid2018 native runtime.

It is intentionally not wired into the production `mrp_napi` target yet. The
current product build still uses the existing runtime path. Keeping this code
out of the main CMake target lets us measure and port the mrp2018 code without
breaking the working emulator.

## Layout

- `origin/`: not included in the public tree. Keep any local read-only
  reference snapshot outside this repository.
- `Mrpoid2018Backend.*`: a HarmonyOS-facing backend skeleton with the same
  lifecycle shape as the current runtime.
- `PORTING_NOTES.md`: current blockers and porting order.

## Rules

If you keep a local reference snapshot, do not modify it directly. Put
HarmonyOS-specific edits in a separate port layer so behavior remains auditable.

Do not ship this backend until licensing is clarified. The upstream project is
described as open source, but this experiment was intentionally separated from
the public repository because redistribution terms remain unclear.
