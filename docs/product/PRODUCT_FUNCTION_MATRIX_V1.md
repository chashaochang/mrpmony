# Product Function Matrix V1

Date: 2026-05-09

## Goal

Define what the app should do, what belongs in Free, what belongs in Pro, and what should wait.

The product should feel generous at the base layer. Users should trust it before they are asked to pay.

## Core Free Functions

These functions define the app. Do not put them behind Pro.

### Local Import

User value:

- Bring in `.mrp` files they already own.
- See whether the file can be recognized.
- Return to the list after import.

MVP behavior:

- Select one local `.mrp` file.
- Copy/import it into app storage.
- Show import success or failure.
- Refresh the local file list.

Later:

- Batch import moves to Pro.
- Better metadata extraction can be Free if it improves trust.

### Local File List

User value:

- Find imported files.
- Start a package quickly.

MVP behavior:

- Show imported files.
- Tap row to run immediately.
- Show filename and basic status.
- Show local-only notice.

Important boundary:

- Do not introduce a store-like library surface.
- Do not add recommendation feeds.
- Do not change tap-to-run behavior during visual work.

### Runtime

User value:

- Play/run the selected MRP package.

MVP behavior:

- Single runtime instance.
- Portrait layout.
- 3:4 viewport frame.
- Feature-phone virtual keypad.
- Touch input.
- Back, restart, screenshot.
- Basic failure state.

Important boundary:

- Keep native input codes stable.
- Keep duplicate key filtering.
- Keep XComponent wiring stable.

### Basic Persistence

User value:

- Leave and return without losing basic state.

MVP behavior:

- Basic save/config persistence.
- Last-run metadata.
- Recently played list, small and simple.

## Pro Functions

Pro should improve control, personalization, and diagnostics. It should not sell access to games.

### Custom Key Layout

User value:

- Make hard-to-play packages comfortable.
- Support different hand sizes and game types.

Pro behavior:

- Edit key positions.
- Resize keys.
- Save per-game or global layout.
- Reset to default.

### Per-game Configuration

User value:

- Different packages often need different quirks.

Pro behavior:

- Scaling mode.
- Key layout preset.
- Compatibility toggles.
- Orientation preference later.
- Last-used settings saved per package.

### Save Manager

User value:

- Protect progress.
- Switch between save snapshots.

Pro behavior:

- View save slots.
- Duplicate slot.
- Rename slot.
- Export/import save bundle later.

### Batch Import

User value:

- Bring in many local files at once.

Pro behavior:

- Select multiple `.mrp` files.
- Import in queue.
- Show per-file result.
- Skip duplicates safely.

### Skins

User value:

- Make the emulator feel like an old keypad phone.

Pro behavior:

- Built-in feature-phone skin pack.
- Keypad material choices.
- Runtime frame styles.

Important boundary:

- Skins should not impersonate specific protected phone brands.
- Keep HDS liquid-glass shell consistent.

### Diagnostics And Log Export

User value:

- Understand why a package fails.
- Share useful logs for compatibility work.

Pro or advanced behavior:

- Runtime log viewer.
- Export diagnostic bundle.
- Compatibility flags attached to package.

## Later Functions

These should wait until the local runner is stable.

- Landscape mode.
- External keyboard mapping.
- Gamepad support.
- Cloud save backup.
- Cross-device sync.
- Community compatibility database.
- Resource list viewer.
- MRP metadata editor.
- Local cover editing.

## Explicit Non-goals

Do not build these into V1:

- Built-in game download center.
- Third-party game recommendations.
- Unlicensed cover scraping.
- "Hot games" feed.
- Subscription-only basic runtime.
- Online account requirement.
- Multi-instance runtime.

## Paywall Rules

The paywall should appear only after the user understands the base product.

Allowed moments:

- User taps `自定义按键`.
- User taps `批量导入`.
- User taps `存档管理`.
- User taps a locked skin.
- User taps `导出兼容性日志`.

Avoid:

- Paywall before first import.
- Paywall before first run.
- Blocking basic launch.
- Fake scarcity like limited daily play count.

## Recommended Pricing

Launch:

- Free base app.
- Pro early unlock: `¥12`.
- Pro stable unlock: `¥18` to `¥25`.

Later:

- Supporter pack: `¥6`, `¥12`, `¥30`.
- Optional skin pack: `¥3` to `¥6`.

Subscription should wait until there is real cloud value.

