# Current Alpha Status

Date: 2026-05-10

## Summary

The HarmonyOS MRP simulator is now in an internal alpha state. The core game flow is mostly usable: bundled MRP packages can be prepared into the app sandbox, selected packages can start through the Native/NAPI runtime, frames render through XComponent, and virtual keypad input reaches the vMRP event path.

The remaining work should be treated as compatibility hardening, not basic bring-up.

## Working

- HarmonyOS app shell launches into the app list.
- Bundled `.mrp` assets are copied into the runtime `mythroad` directory.
- `.mrp` packages can be listed and started from the app.
- Native runtime initializes vMRP with a 240x320 framebuffer.
- XComponent displays the native framebuffer.
- Dirty-rect frame updates now sample the full screen buffer correctly.
- Basic touch and virtual keypad input are connected.
- Duplicate key-down and stray key-up events are filtered before reaching the MRP core.
- Directory enumeration skips `.` and `..`.
- `_mr_readFile` can load entries from the active `.mrp` package.
- Gzip-compressed package entries are decompressed before being returned to the MRP VM.
- Runtime file writes now stay in the app sandbox and support parent directory creation, recreate/truncate, rename, remove, directory listing, and close-time flush for save/config data.
- The standard Mythroad writable disk directories `mythroad/disk/a`, `mythroad/disk/b`, and `mythroad/disk/x` are prepared on every startup, even when bundled assets were already marked ready.
- Runtime exit no longer kills the HarmonyOS app process.
- Release clears the XComponent surface to black.

## Recently Fixed

- Menu and partial redraw corruption caused by dirty-rect rendering using local `row * w + col` indexing.
- Package resource reads returning compressed bytes instead of decompressed script/resource content.
- Rapid or duplicated touch events causing repeated key-down delivery.
- Some MRP packages seeing only a partial resource set because package-internal reads were missing.
- File persistence semantics for saves/configs: writable parent directories are auto-created, `MR_FILE_RECREATE` truncates in write/create contexts, rename creates destination parents, and writable handles are flushed before close.

## Known Gaps

- Audio playback is still not product-grade. The bridge exists, but real playback behavior and latency need focused testing.
- Compatibility is only proven on a small bundled sample set.
- Save/config file persistence has the host-side file semantics in place, but still needs a pass with real games that write progress.
- Import flow should be tested with external legally owned `.mrp` files, not only bundled resources.
- Logs are still verbose and debug-oriented.
- ArkTS file APIs still report "Function may throw exceptions" warnings during build.
- AppGallery readiness is not in scope yet. Bundled sample rights, icon, privacy policy, and store metadata still need review.

## Current Rule For Future Work

Do not change app-list behavior while debugging runtime compatibility unless the bug is proven to be in selection/routing. Most remaining issues should be investigated inside:

- `vmrp-core/bridge.c`
- `vmrp-core/vmrp.c`
- `harmonyos-mrp/entry/src/main/cpp/runtime`
- `harmonyos-mrp/entry/src/main/cpp/platform`
- `harmonyos-mrp/entry/src/main/ets/components/runtime`
