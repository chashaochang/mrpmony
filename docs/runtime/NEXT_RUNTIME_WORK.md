# Next Runtime Work

Date: 2026-05-09

## Priority 1: Compatibility Sweep

Run the compatibility test plan on a small package set and collect logs for every failure. The goal is to find missing bridge APIs and package format edge cases, not to polish UI.

Outputs:

- A table of package results.
- One log capture per failed package.
- A short note mapping each failure to rendering, input, file, audio, lifecycle, or unknown.

## Priority 2: File System Semantics

Host-side write semantics are now implemented for sandbox-backed saves/configs. Next, validate package reads, sandbox reads, writes, rename, remove, directory listing, and save/config behavior with packages that actually persist progress.

Focus files:

- `entry/src/main/cpp/third_party/vmrp-core/bridge.c`
- `entry/src/main/cpp/third_party/vmrp-core/fileLib.c`
- `harmonyos-mrp/entry/src/main/ets/adapters/RuntimeAssetBootstrap.ts`

Risks:

- Some games may expect case-insensitive paths.
- Some games may mix package-internal reads with external save files.
- `lookfor` modes in `_mr_readFile` may need more exact Mythroad-compatible behavior.
- `MR_FILE_RECREATE` and share-open use the same flag value in different MRP headers; current behavior only truncates when the flag appears in a write/create context.

## Priority 3: Audio

The bridge has audio hooks, but real playback quality is not yet established.

Focus files:

- `entry/src/main/cpp/runtime/MrpAudioBridge.cpp`
- `entry/src/main/cpp/runtime/VmrpHostBridge.cpp`
- `entry/src/main/cpp/third_party/vmrp-core/bridge.c`

Minimum target:

- Sound calls do not block the runtime.
- Short effects play with acceptable latency.
- Stop/loop semantics are predictable.

## Priority 4: Runtime Lifecycle

Stress:

- launch package;
- exit package;
- return to list;
- launch another package;
- restart same package;
- background and foreground app;
- force-stop and relaunch.

Watch for stale frames, stuck key state, leaked runtime state, and Unicorn shutdown issues.

## Priority 5: Documentation And Release Notes

Keep docs current as behavior changes:

- `CURRENT_ALPHA_STATUS.md`
- `BUILD_INSTALL_RUNBOOK.md`
- `COMPATIBILITY_TEST_PLAN.md`

Before a public release, add separate product-facing docs for privacy, permissions, bundled content rights, and AppGallery metadata.
