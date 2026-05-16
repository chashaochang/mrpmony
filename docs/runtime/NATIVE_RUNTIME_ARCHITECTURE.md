# Native Runtime Architecture

Date: 2026-05-09

## Runtime Stack

The current runtime path is:

```text
ArkTS pages
  -> NativeRuntimeAdapter
  -> libmrp_napi.so
  -> MrpFacade
  -> MrpRuntime
  -> third_party/vmrp-core
  -> Unicorn ARM emulation
```

## ArkTS Responsibilities

ArkTS owns app-level UX and orchestration:

- App list and import pages.
- Runtime page lifecycle.
- XComponent placement.
- Virtual keypad UI.
- Calls into NAPI for init, load, start, pull-frame, input, pause, resume, and release.

Runtime compatibility fixes should generally not start in ArkTS unless the issue is clearly caused by UI event delivery or lifecycle timing.

## NAPI And C++ Responsibilities

The NAPI/C++ layer owns runtime state and platform adaptation:

- Convert ArkTS calls into native runtime calls.
- Maintain runtime state through `MrpFacade`.
- Start and stop vMRP.
- Pull frames and render through XComponent.
- Normalize input events.
- Bridge audio, exit, and text/edit callbacks.

Key files:

- `entry/src/main/cpp/napi/facade_binding.cpp`
- `entry/src/main/cpp/platform/MrpFacade.cpp`
- `entry/src/main/cpp/runtime/MrpRuntime.cpp`
- `entry/src/main/cpp/runtime/VmrpHostBridge.cpp`
- `entry/src/main/cpp/runtime/XComponentSurfaceRenderer.cpp`

## vMRP Core Responsibilities

`vmrp-core` owns the emulated Mythroad environment:

- Unicorn CPU lifecycle.
- Mythroad bridge function table.
- File and directory APIs.
- Drawing primitives.
- Timers and input events.
- Package-internal resource reads.

Most compatibility fixes live in:

- `entry/src/main/cpp/third_party/vmrp-core/bridge.c`
- `entry/src/main/cpp/third_party/vmrp-core/vmrp.c`
- `entry/src/main/cpp/third_party/vmrp-core/fileLib.c`
- `entry/src/main/cpp/third_party/vmrp-core/mythroad/*`

## Frame Rendering

MRP code draws into a 16-bit RGB565 screen buffer. `_DispUpEx` reports a dirty rectangle, but the source buffer remains the full screen. `guiDrawBitmap` therefore must sample by absolute screen coordinates:

```text
pixel = screen[dstY * screenWidth + dstX]
```

Using local dirty-rect indexing causes garbled partial redraws.

## Package Resource Reads

MRP packages can read resources from their own package through `_mr_readFile`.

Current bridge behavior:

1. Resolve active `pack_filename`.
2. Search the package index.
3. Read the matching entry.
4. If the entry is gzip, decompress it.
5. Return a VM memory pointer and write the final byte length.

This is important because many MRP package entries are stored as gzip streams, including scripts and plugin `.ext` files.

## Input Handling

ArkTS sends virtual key touch events as down/up pairs. Native filters:

- duplicate down for a key already pressed;
- stray up for a key that was not pressed.

This keeps rapid touch delivery from creating stuck or repeated key states inside the MRP VM.
