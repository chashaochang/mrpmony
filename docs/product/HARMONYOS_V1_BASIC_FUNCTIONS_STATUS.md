# HarmonyOS MRP Simulator V1 Basic Functions Status

Date: 2026-05-10

## V1 Goal

First usable HarmonyOS version focused on a local MRP runtime:

- Launch from desktop icon into the app list.
- List local `.mrp` packages under the app runtime `mythroad` directory.
- Import local `.mrp` files through the system document picker.
- Start one MRP package at a time.
- Render the 240x320 MRP framebuffer through XComponent.
- Provide basic touch, direction, OK, soft-key, `*`, `#`, dial, `0`, and hang-up input.
- Support pull-frame, restart, exit, and return-to-list flows.
- Avoid process abort when MRP calls `exit(0)`.
- Clear the native surface when runtime exits, preventing stale or corrupted screen content.

## Implemented

### App Entry

- `module.json5` declares launcher skills with `entity.system.home` and `action.system.home`.
- `ShellRootPage` defaults to `appList`, so desktop launch enters the app list instead of a runtime debug page.

### App List

- `NativeAppAdapter` scans the runtime `mythroad` directory for `.mrp` files.
- Bundled runtime assets are prepared before listing.
- Imported packages appear after returning from the import page.

### Import

- `NativeImportAdapter` uses `DocumentViewPicker` with `.mrp` suffix filtering.
- Selected files are inspected for name and size.
- Non-`.mrp` names are rejected instead of being silently renamed to `.mrp`.
- Valid packages are copied into the runtime `mythroad` directory.

### Runtime

- `NativeRuntimeAdapter` initializes native runtime assets, loads the selected package, starts vMRP, pulls frames, and sends input.
- `RunPage` supports start, restart, pull-frame, exit, and return.
- `RuntimeContainer` renders to XComponent and exposes a basic virtual keypad.
- Runtime layout was adjusted so the framebuffer and virtual keypad fit on the Mate 80 screen without being cut off by the bottom navigation area.
- Runtime asset setup always prepares `mythroad/disk/a`, `mythroad/disk/b`, and `mythroad/disk/x` for save/config data.

### Native Stability

- `br_exit()` no longer calls process `exit(0)`.
- MRP exit now notifies the host, stops Unicorn emulation, clears the frame, and returns to the app.
- `MrpFacade::Release()` renders a black frame into XComponent so explicit exit does not leave corrupted or stale pixels.
- Input is rejected when runtime is no longer running.
- File writes, recreate/truncate, rename/remove, recursive directory creation, and close-time flush are implemented for sandbox-backed save/config persistence.

## Verified On Device

Device: HUAWEI Mate 80, build `VYG-AL00 6.1.0.117(SP6C00E115R3P6)`

Verified:

- Debug HAP builds successfully with DevEco hvigor.
- HAP installs with `hdc install -r`.
- `aa start` launches `com.xiaobai.mrpsimulator`.
- Process remains alive after launch.
- Tapping a list item starts the bundled MRP sample.
- XComponent renders the MRP frame.
- Virtual keypad is visible in one screen.
- Exit clears the runtime surface to black instead of showing corrupted content.
- Debug HAP builds after the save/config persistence changes.

Build artifact:

- `entry/build/default/outputs/default/entry-default-signed.hap`

## Remaining V1 Release Gaps

- Real import flow still needs a manual picker test with several external `.mrp` files.
- Save/config persistence needs game-level verification with packages known to write progress.
- Audio bridge currently returns success for sound calls but does not provide real HarmonyOS playback.
- Bundled sample MRP files must be reviewed before AppGallery submission. For public release, remove unauthorized third-party packages or replace them with self-owned demo content.
- ArkTS file APIs still produce "Function may throw exceptions" warnings. They do not block the build, but should be cleaned up before release hardening.
- Product UI is functional but still debug-like. A store-facing build needs copy, icon, permission disclosure, privacy policy, and AppGallery metadata polish.

## Suggested Next Step

Use this V1 as the internal alpha baseline, then run a small compatibility matrix:

- 3 to 5 legally owned `.mrp` packages.
- Start, input, exit, restart, and relaunch each package.
- Confirm file persistence for save/config writes under the app sandbox.
- Capture crash logs for each failure case.
