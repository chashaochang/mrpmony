# UI Screen Interaction Spec V1

Date: 2026-05-09

## Goal

Turn the product direction into concrete screen behavior.

This spec is written for implementation. It defines what appears on each screen, what the user can do, and which behaviors must not change during visual polish.

## Global Shell

Use `HdsNavigation` as the page container.

Global structure:

- HDS native navigation title and transitions.
- Light liquid-glass background.
- Safe-area aware content.
- Floating surfaces only where they serve hierarchy.
- No Android-like top app bar.
- No debug-looking full-width banners.

Global actions:

- Back uses HDS-native behavior.
- Runtime back asks the runtime to exit or returns to list according to current app behavior.
- Settings and Pro entries can be present but quiet.

## Home

Purpose:

- Manage local packages.
- Start a package quickly.

Content order:

1. HDS navigation title: `mrpmony`.
2. Liquid-glass local notice.
3. Import action.
4. Recently played, if any.
5. Local files.

Primary states:

- Empty: show one clear `导入 MRP` action.
- Loading: small HDS loading row, no full-page drama.
- Error: glass notice with retry.
- Populated: list rows grouped by recent and all files.

File row content:

- Display name.
- Original filename.
- Small status: `本地`, `最近运行`, `可能不兼容`.
- Optional overflow action.

Interactions:

- Tap row: run that `appId` immediately.
- Tap import: navigate to Import.
- Long press row: open local actions later.
- Overflow row: rename/delete/settings later.

Do not:

- Add a game detail page before launch.
- Add cover-heavy card grids for V1.
- Add online recommendations.

## Import

Purpose:

- Import a local `.mrp` package.

Content order:

1. HDS navigation title: `导入 MRP`.
2. Local legality notice.
3. Source picker.
4. Package preview.
5. Import result.

States:

- Idle: show file picker action.
- Preview: show filename, size, detected name if available.
- Importing: show progress, disable duplicate taps.
- Success: show result, return to Home and refresh.
- Failure: show reason and retry.

Interactions:

- Pick file: open local file picker.
- Confirm import: copy/import selected file.
- Success: return to Home.
- Back: return to Home without list mutation.

Copy:

- Prefer direct utility text.
- Avoid "资源", "游戏库", "下载" language.

## Runtime

Purpose:

- Run one selected package.

Content order:

1. HDS navigation container.
2. Floating runtime title/action strip.
3. MRP viewport frame.
4. Feature-phone keypad.

Title/action strip:

- Package display name or filename.
- Back.
- Restart.
- Screenshot.
- Settings later.

Viewport:

- 3:4 ratio.
- Black fallback while loading.
- No text overlays on gameplay.
- Glass/device frame outside content only.

Keypad layout:

```text
左软        右软
拨号    DPad/OK    挂机
1       2       3
4       5       6
7       8       9
*       0       #
```

Key behavior:

- Press down sends existing key-down code.
- Release sends existing key-up code.
- Pressed visual depresses the key.
- No layout shift while pressing.

Runtime states:

- Loading package.
- Running.
- Paused or backgrounded.
- Runtime error.
- Exited normally.

Do not:

- Change key codes.
- Change native lifecycle.
- Change XComponent id or library.
- Let glass overlays intercept viewport touches.

## Pro Tools

Purpose:

- Show paid tool value without making the product feel like a store.

Content:

- HDS navigation title: `Pro 工具箱`.
- Pro status row.
- Feature groups:
  - `按键布局`
  - `每游戏配置`
  - `存档管理`
  - `批量导入`
  - `外观皮肤`
  - `兼容性日志`

Interaction:

- Locked item opens Pro explanation.
- Unlocked item opens tool page.
- No aggressive countdowns.

Paywall content:

- One-time unlock.
- Explain local-tool value.
- List included tools.
- Keep price clear.

## Settings

Purpose:

- Trust, legal, and app info.

Content:

- Local file/legal notice.
- Privacy.
- Storage usage later.
- About.
- Runtime version and build info.

Do not:

- Put discovery/content features here.
- Hide legal boundary in small print only.

## Visual States

### Empty

Use quiet glass panel and one action.

### Error

Use glass notice, clear reason, and one recovery action.

### Loading

Use small inline loading.

### Locked

Use subtle Pro badge and clear unlock affordance.

### Pressed Key

Use visible key depression, darker lower edge, and inner highlight shift.

## Copy Tone

Use simple Chinese product text:

- `导入 MRP`
- `本地文件`
- `最近运行`
- `仅运行你拥有或已获授权的本地文件`
- `自定义按键`
- `兼容性日志`

Avoid:

- `海量`
- `资源库`
- `游戏下载`
- `破解`
- `全网`
- `ROM`
