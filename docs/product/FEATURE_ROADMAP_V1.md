# Feature Roadmap V1

Date: 2026-05-09

## Product Direction

Build a trustworthy local MRP emulator for HarmonyOS.

The app should feel like a practical emulator tool first, then gradually become a polished nostalgia product.

Minimum platform: HarmonyOS `6.1.0(23)`, because the product direction depends on HDS effects.

## Phase A: Internal Alpha

Goal: make the current runtime dependable enough for repeated testing.

Features:

- Local bundled package start.
- Local imported package start.
- Stable frame rendering.
- Basic virtual keypad.
- Touch input.
- Exit and restart.
- Package resource loading.
- Gzip resource decompression.
- Basic compatibility logs.

Done when:

- 5 to 10 representative packages can be tested with a written compatibility result.
- Runtime failures can be mapped to file, rendering, input, audio, lifecycle, or unknown.

## Phase B: Public MVP

Goal: release a safe local runtime app.

Features:

- Local import through file picker.
- Imported app list.
- Runtime screen.
- Virtual keypad.
- Basic screenshot.
- Basic save/config persistence.
- Import legality notice.
- Minimal privacy policy.
- No online content.
- No third-party bundled game content.

UI work:

- Clean app list.
- Clear import entry.
- Runtime layout that fits common screens.
- First-run notice.
- Error state for unsupported packages.
- Product UI/UX blueprint documented in `PRODUCT_UI_UX_DESIGN_V1.md`.
- Function boundary documented in `PRODUCT_FUNCTION_MATRIX_V1.md`.
- Screen interaction behavior documented in `UI_SCREEN_INTERACTION_SPEC_V1.md`.
- HDS navigation migration documented in `HDS_NAVIGATION_IMPLEMENTATION_PLAN_V1.md`.
- Visual system documented in `UI_VISUAL_SYSTEM_V1.md`.
- ArkTS StateV2 rule documented in `ARKTS_STATE_MANAGEMENT_RULES_V1.md`.
- HDS liquid-glass visual direction documented in `UI_STYLE_DIRECTION_HDS_LIQUID_GLASS.md`.
- ArkTS implementation mapping documented in `UI_IMPLEMENTATION_SPEC_V1.md`.

Done when:

- A user can install, import a local file, run it, exit, and relaunch without developer tools.

## Phase C: Pro V1

Goal: add paid value without selling content.

Features:

- Pro one-time unlock.
- Custom key layout.
- Per-game settings.
- Save manager.
- Batch import.
- Runtime skin pack.
- Advanced scaling.
- Log export.

Good Pro boundary:

- Free users can play.
- Pro users can organize, customize, and troubleshoot better.

## Phase D: Compatibility Expansion

Goal: improve runtime coverage.

Features:

- Compatibility modes.
- Case-insensitive path option.
- More exact `_mr_readFile` lookfor behavior.
- Better file write semantics.
- Audio playback improvements.
- External keyboard mapping.
- Optional landscape layout.

Done when:

- Most failures are package-specific edge cases rather than missing base runtime features.

## Phase E: Collector Tools

Goal: support archive and technical users.

Features:

- MRP metadata viewer.
- Package resource list viewer.
- Manual display name and cover editing.
- Local compatibility notes.
- Per-package diagnostic page.
- Exportable report bundle.

## Backlog

Possible later features:

- Gamepad support.
- Cloud save backup.
- Cross-device settings sync.
- Community compatibility database.
- Online content with explicit authorization only.

Avoid for now:

- Built-in download center.
- Game recommendation feed.
- Unlicensed cover scraping.
- Subscription without real cloud value.
- Multi-instance runtime.
- Heavy debugger UI before compatibility is stable.
