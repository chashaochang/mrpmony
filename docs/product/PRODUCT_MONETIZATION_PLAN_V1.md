# Product And Monetization Plan V1

Date: 2026-05-09

## Product Positioning

The app should be positioned as a local MRP nostalgia emulator and file runtime tool.

Core promise:

> Run and manage local `.mrp` files that the user owns or is authorized to use.

The product should not be positioned as a game downloader, game store, game library, or third-party content distribution platform.

## Target Users

### Nostalgia Players

Users who remember old keypad-phone MRP games and want to run legally owned local files on a modern HarmonyOS device.

They care about:

- Easy import.
- Good virtual keypad feel.
- Stable gameplay.
- Save/config persistence.
- Skins that feel like old phones.

### Emulator And Archive Users

Users who collect old mobile platforms, file formats, and emulators.

They care about:

- Compatibility.
- Package metadata.
- File management.
- Per-game settings.
- Debug logs.

### Technical Users

Users who inspect old MRP apps or want a local compatibility test tool.

They care about:

- Resource list inspection.
- Runtime logs.
- Compatibility modes.
- Exportable diagnostics.

## Product Principles

- Local-first: imported files stay local by default.
- No third-party content distribution.
- Free version must be genuinely useful.
- Paid features should improve experience and management, not sell access to games.
- Compatibility and trust matter more than flashy UI.
- Runtime fixes should not depend on hidden online services.

## Free Version

The free version should prove value immediately:

- Import local `.mrp` files.
- List imported packages.
- Run one package at a time.
- Basic virtual keypad.
- Basic touch input.
- Portrait runtime layout.
- Restart and exit.
- Basic screenshot.
- Basic save/config persistence.
- Clear import legality notice.

Suggested soft limits:

- Keep recent history small.
- Keep only default keypad layout.
- Provide basic scaling only.

Avoid making basic launch or basic input paid. That would damage trust.

## Pro Version

Recommended model: one-time unlock.

Pro features:

- Unlimited library management.
- Custom virtual keypad layouts.
- Multiple keypad presets.
- Portrait and landscape control presets.
- Per-game configuration.
- Save slot management.
- Batch import and batch metadata refresh.
- Advanced scaling modes.
- Display filters, such as nearest-neighbor, soft scale, and LCD-style overlay.
- Custom game names and cover images.
- Compatibility toggles.
- Log export for troubleshooting.
- More emulator skins.

Good first Pro bundle:

- Custom key layout.
- Per-game settings.
- Save manager.
- Batch import.
- Skin pack 1.
- Log export.

## Optional Add-ons

Add-ons are useful later, but should not be required for V1 monetization.

- Extra skins.
- Supporter pack.
- Cloud backup.
- Cloud sync across devices.
- Developer/debug pack.

Cloud features should wait until there is enough usage to justify privacy work, account work, sync conflict handling, and server maintenance.

## Pricing

Recommended launch pricing:

- Free base app.
- Pro early price: `¥12`.
- Pro stable price: `¥18` to `¥25`.
- Supporter pack: `¥6`, `¥12`, or `¥30`.
- Optional skin packs: `¥3` to `¥6` per pack.

Avoid subscription at launch. A one-time unlock matches the emulator/tool audience better.

Subscription only makes sense later if there is real cloud value:

- Cloud save backup.
- Cross-device sync.
- Online compatibility database.

Suggested future subscription, if needed:

- `¥6/month`.
- `¥30/year`.

## V1 MVP

V1 should focus on local runtime trust:

- App list.
- Local import.
- Single runtime.
- Virtual keypad.
- Stable framebuffer rendering.
- Package-internal resource reads.
- Gzip resource decompression.
- Exit and restart safety.
- Basic save/config persistence.
- Basic compatibility test set.
- No online content.
- No third-party bundled game package in release builds.

## V1.1

Improve daily use:

- Better import management.
- Recently played.
- Custom display name.
- Per-game last-used settings.
- Screenshot gallery.
- Keypad haptic feedback.
- Basic skin selection.

## V1.2

Introduce Pro:

- Huawei IAP non-consumable Pro unlock.
- Custom key layout editor.
- Per-game configuration.
- Save manager.
- Batch import.
- Diagnostic log export.

## V2

Power-user and polish phase:

- Landscape mode.
- External keyboard mapping.
- Gamepad support if technically feasible.
- Compatibility modes for file/path/input quirks.
- MRP metadata viewer.
- Resource list viewer.
- Local compatibility notes.

## Store Messaging

Recommended Chinese positioning:

```text
mrpmony - MRP怀旧模拟器
用于导入、管理和运行你拥有或已获授权的本地 MRP 文件。
```

Recommended English positioning:

```text
mrpmony - MRP Nostalgia Emulator
A local runtime tool for importing, managing, and running MRP files you own.
```

Avoid:

- "海量游戏"
- "免费游戏库"
- "经典游戏下载"
- "破解"
- "全网资源"
- "所有 MRP 游戏"

## Legal And Review Boundary

Release builds should not include commercial third-party MRP packages unless there is written authorization.

The app can provide local import, but it should clearly state:

- The app does not provide MRP downloads.
- The app does not host or sell third-party game content.
- Users should only import files they own or are authorized to use.
- Imported files are processed locally by default.

## Recommended Next Product Work

1. Turn the current runtime into a clean internal alpha.
2. Build a small compatibility matrix.
3. Design a simple runtime screen with better keypad ergonomics.
4. Add one in-app import legality notice.
5. Decide which features become the first Pro bundle.
