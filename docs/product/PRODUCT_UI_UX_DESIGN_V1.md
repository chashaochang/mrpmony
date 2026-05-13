# Product UI/UX Design V1

Date: 2026-05-09

## Product Definition

This app is a HarmonyOS-native local MRP runner.

One sentence:

> Import local `.mrp` files, run them reliably, and control them through a tactile feature-phone keypad.

It is not a game store, ROM library, or downloader. The product value comes from runtime stability, local file management, keypad feel, compatibility tooling, and tasteful nostalgia.

Minimum platform:

- HarmonyOS `6.1.0(23)`.
- HDS liquid-glass effects are part of the baseline product experience.

## Product Pillars

### 1. Local Trust

The user should always understand that files stay local and that the app is a runtime tool.

Core features:

- Local `.mrp` import.
- Local file list.
- Recently played.
- Clear legality notice.
- No online content entry.
- No third-party bundled commercial packages in release builds.

### 2. Run Fast

The shortest path should be:

```text
Open app -> tap local file -> enter runtime -> play
```

Do not add a selection wizard or library ceremony before runtime launch unless the user explicitly asks for that behavior.

### 3. Keypad Identity

The keypad is the brand signal. It should feel like a Chinese keypad phone, not a NES console, Android emulator, or generic gamepad.

Required controls:

- Left soft key.
- Right soft key.
- D-pad with OK.
- Green call key.
- Red hang-up key.
- Number keys `1` to `9`.
- `*`, `0`, `#`.

### 4. HDS Native Polish

The app shell should feel like HarmonyOS 6.1 / HDS:

- `HdsNavigation` for page container, title, back behavior, and transitions.
- Floating translucent navigation surfaces.
- Liquid-glass material with rim highlights, internal light, and layered depth.
- Calm system typography.
- Clean safe-area handling.

The visual language should be modern and system-native; the MRP identity lives inside the viewport and keypad.

## Information Architecture

```text
HdsNavigationShell
  Home
    Local Files
    Recently Played
    Import Entry
  Import
    Source Picker
    Package Preview
    Import Result
  Runtime
    Viewport
    Feature Phone Keypad
    Runtime Actions
  Pro Tools
    Key Layout
    Per-game Config
    Save Manager
    Skins
    Diagnostics
  Settings
    Legal Notice
    Privacy
    About
```

V1 can keep Pro Tools and Settings as simple pages or disabled entries, but the navigation shape should leave space for them.

## Navigation UX

Use `HdsNavigation` as the native shell. The goal is to make page movement feel like HarmonyOS, not to change product behavior.

Current business navigation semantics to preserve:

- App list item tap launches that package.
- Import success returns to the list and refreshes it.
- Runtime back returns to the list.
- `appId` is passed directly from selected file to runtime.

Suggested destination model:

- `HomeDestination`
- `ImportDestination`
- `RuntimeDestination(appId)`
- `ProToolsDestination`
- `SettingsDestination`

Do not use the HDS navigation refactor as an excuse to change list behavior, launch flow, or runtime lifecycle.

## Home Screen

Goal: make local files feel organized and trustworthy.

Primary content:

- Title: `mrpmony`.
- Subtitle/positioning: `MRP 怀旧模拟器`.
- Local notice: `仅运行你拥有或已获授权的本地文件`.
- Primary action: `导入 MRP`.
- Sections: `最近运行`, `本地文件`.

Interaction:

- Tap a file row: launch runtime immediately.
- Long press or overflow action: rename, settings, delete, diagnostics.
- Empty state: one import action, no marketing copy.

Visual:

- HDS floating title area.
- Liquid-glass notice.
- Glass list rows with filename, display name, and small status.
- No game covers in V1 unless user supplies local images.

## Import Screen

Goal: keep import direct and transparent.

Flow:

```text
Choose local file -> preview package info -> import -> return to Home
```

UI:

- HDS navigation title: `导入 MRP`.
- Source picker as primary action.
- Preview panel showing filename, size, and detected metadata.
- Result panel with success/failure and next action.

Behavior:

- On success, refresh local list.
- Do not auto-download metadata.
- Do not scrape covers.

## Runtime Screen

Goal: let the user play without the shell fighting the game.

Layout:

```text
HDS runtime title/action strip
MRP viewport frame, close to 240x320 ratio
Feature-phone keypad
```

Runtime actions:

- Back.
- Restart.
- Screenshot.
- Settings.
- Diagnostics/log export later.

Viewport:

- Keep 3:4 frame stable.
- Preserve XComponent id and native touch mapping.
- Do not place text over gameplay.
- Use a subtle glass/device frame outside the viewport.

Keypad:

- Physical key look with bevel, highlight, and pressed depression.
- Larger D-pad and OK center.
- Color only where meaningful: green call, red hang-up, tiny active highlight.
- Stable key sizes so repeated pressing does not shift layout.

## Pro Tools

Goal: sell better control and management, not access to games.

Recommended Pro V1:

- Custom key layout.
- Per-game configuration.
- Save manager.
- Batch import.
- Skin pack.
- Compatibility log export.

Free users should still be able to import and play. Pro should make the app more personal, organized, and debuggable.

## Visual Direction

Use:

- Pearl, mist gray, silver blue, graphite.
- Soft cyan active states.
- Small warm amber notice/pro accents.
- Floating glass surfaces with bright edge highlights.
- Subtle inner glow and material layering.

Avoid:

- Android Material surfaces.
- NES or retro console framing.
- Pixel-art-first branding.
- Heavy dark gamer panels.
- Purple SaaS gradients.
- Generic flat frosted cards.

## First Product Milestone

V1 UI/UX is done when:

- Home, Import, Runtime, Pro Tools, and Settings have a coherent HDS navigation structure.
- Runtime controls clearly read as feature-phone keys.
- The app no longer feels like a debug shell.
- App list tap-to-run behavior is unchanged.
- Runtime input codes and duplicate-key filtering are unchanged.
- The app store screenshots can communicate local import, runtime viewport, keypad, and Pro tooling without showing third-party copyrighted content.

## Detailed Specs

Follow-up specs:

- `PRODUCT_FUNCTION_MATRIX_V1.md`: Free, Pro, later, and non-goal boundaries.
- `UI_SCREEN_INTERACTION_SPEC_V1.md`: per-screen content, states, and interactions.
- `HDS_NAVIGATION_IMPLEMENTATION_PLAN_V1.md`: how to move the shell to `HdsNavigation` without changing business behavior.
- `UI_VISUAL_SYSTEM_V1.md`: material, keypad, viewport, typography, motion, and density rules.
