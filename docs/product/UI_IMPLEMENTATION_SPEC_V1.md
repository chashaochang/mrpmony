# UI Implementation Spec V1

Date: 2026-05-09

## Goal

Translate `UI_STYLE_DIRECTION_HDS_LIQUID_GLASS.md` into concrete ArkTS implementation work.

This spec is intentionally practical: it maps the design direction to reusable components, screens, and files in the current project.

Before implementing any new UI pass, read:

```text
docs/product/PRODUCT_UI_DECISIONS_LOCKED_V1.md
```

The approved visual reference is:

```text
docs/product/reference/gpt-image2-liquid-glass-keyphone-direction.png
```

Do not use `docs/product/home-redesign-v3-liquid-keyphone.svg` as an implementation target; it has been rejected as too wireframe-like and not polished enough.

State rule:

- New ArkTS UI state must follow State Management V2.
- New components should use `@ComponentV2`, `@Local`, and `@Param`.
- Do not introduce new `@State` or `@Prop` in fresh components.
- See `ARKTS_STATE_MANAGEMENT_RULES_V1.md`.

Platform rule:

- Minimum compatible SDK is HarmonyOS `6.1.0(23)`.
- HDS effects are treated as a baseline capability, not an optional fallback for lower APIs.
- Do not spend UI implementation effort preserving API 20 compatibility.

## Current UI State

The current UI is functional but still debug-like.

Current traits:

- Plain headers.
- Plain status banners.
- Standard text buttons.
- Dark runtime background.
- Simple rectangular virtual keys.
- Runtime controls feel like app buttons, not feature-phone hardware.

The next UI pass should preserve runtime behavior while replacing the visual shell and keypad presentation.

## Component Targets

### `GlassSurface`

Purpose: shared HDS liquid-glass panel style.

Use for:

- File rows.
- Status notices.
- Runtime toolbar.
- Settings groups.
- Pro feature groups.

Behavior:

- Rounded translucent surface.
- Layered background tint.
- Thin bright edge highlight.
- Soft shadow.
- Optional inner highlight.

Suggested props:

- `radius`
- `padding`
- `tone`
- `pressed`

Likely location:

```text
entry/src/main/ets/components/common/GlassSurface.ets
```

### `HdsNavigationShell`

Purpose: HDS-native page container and navigation structure.

Use for:

- App list page container.
- Import page container.
- Runtime page container.
- Page title, subtitle, back behavior, and transition.

Elements:

- `HdsNavigation` as the page shell.
- HDS-style title area using official gradient blur material.
- Optional right-side actions.
- Native back affordance.
- Floating translucent background treatment.

Required title bar material:

- Home and second-level pages should use HDS `.titleBar(...)`.
- Use `ScrollEffectType.IMMERSIVE_GRADIENT_BLUR` on API 23+.
- Pair it with `systemMaterialEffect`:
  - `hdsMaterial.MaterialType.IMMERSIVE`
  - `hdsMaterial.MaterialLevel.ADAPTIVE`
- Bind each scrollable page to the title bar with `.bindToScrollable([scroller])`.
- Use official HDS imports from `@kit.UIDesignKit` for new HDS navigation work.
- Do not fake the top material with custom overlay gradients if HDS supports the page.

Dependency note:

The current project does not declare an HDS dependency in `oh-package.json5`. The implementation pass should first add the official HDS package required by HarmonyOS `6.1.0(23)`, then wire `ShellRootPage.ets` to `HdsNavigation`.

Implementation note:

The current `ShellRootPage.ets` uses a manual `currentRoute` state:

```text
appList -> import
appList -> run(appId)
import -> appList(refresh)
run -> appList
```

The UI pass can replace the visual and container layer with `HdsNavigation`, but it should preserve these business navigation semantics unless product behavior is explicitly redesigned.

Likely target:

```text
entry/src/main/ets/pages/ShellRootPage.ets
```

`PageHeader.ets` can either become a small HDS title/action adapter or be removed after the HDS shell owns page titles.

### `FloatingTitleBar`

Purpose: runtime-only floating glass title/action strip inside the `HdsNavigation` page.

Use for:

- Runtime package title.
- Restart, screenshot, settings, and diagnostics actions.
- Compact status text.

This should not replace `HdsNavigation`; it is a local runtime overlay/control area inside the destination page.

### `GlassNotice`

Purpose: replace plain yellow `StatusBanner`.

Use for:

- Local-only file notice.
- Error and warning states.
- Runtime hints.

Likely replacement target:

```text
entry/src/main/ets/components/common/StatusBanner.ets
```

### `FeaturePhoneKey`

Purpose: one physical keypad key.

Use for:

- Soft-left and soft-right.
- Direction keys.
- Numeric keys.
- Call and hang-up keys.
- `*`, `0`, `#`.

Design:

- Not a standard button.
- Glossy rubber/plastic surface.
- Beveled key face.
- Pressed state lowers visual depth.
- Text centered and readable.
- Stable width and height.

Likely location:

```text
entry/src/main/ets/components/runtime/FeaturePhoneKey.ets
```

### `DPadControl`

Purpose: feature-phone directional ring with OK center.

Keys:

- Up.
- Down.
- Left.
- Right.
- OK.

Design:

- Ring-like or clustered physical control.
- Larger than numeric keys.
- Strong tactile identity.

Likely location:

```text
entry/src/main/ets/components/runtime/DPadControl.ets
```

### `FeaturePhoneKeypad`

Purpose: full keypad module.

Layout:

```text
左软        右软
拨号    DPad/OK    挂机
1       2       3
4       5       6
7       8       9
*       0       #
```

The exact compact layout can be adjusted for device height, but it must remain clearly feature-phone-like.

Likely replacement target:

```text
entry/src/main/ets/components/runtime/RuntimeContainer.ets
```

### `RuntimeViewportFrame`

Purpose: frame the XComponent as an old MRP phone screen inside the modern shell.

Requirements:

- Preserve 3:4 aspect ratio.
- Keep XComponent touch mapping correct.
- Do not overlay text on gameplay.
- Add subtle glass/device frame outside the viewport.
- Support black fallback while waiting for a frame.

Likely location:

```text
entry/src/main/ets/components/runtime/RuntimeViewportFrame.ets
```

## Screen Plan

### App List

Files:

- `pages/AppListPage.ets`
- `components/appList/AppListView.ets`
- `components/appList/ImportEntryButton.ets`
- `components/common/PageHeader.ets`
- `components/common/StatusBanner.ets`

Changes:

- Replace plain header with compact liquid-glass navigation/title treatment.
- Replace yellow status banner with liquid-glass notice when a notice is truly needed.
- Turn list rows into polished glass list rows inspired by the approved `gpt-image-2` reference.
- Keep the list as the main first-screen content; do not let headers, summaries, or explanation panels push it down.
- Distinguish games and apps through filters, type marks, right-side labels, and restrained tint.
- Keep current tap-to-run behavior unchanged.
- Keep local-only notice visible but more elegant.

Do not:

- Add a game download center.
- Add a two-step selection flow unless requested.
- Change business navigation semantics while doing visual work.
- Break current `appId` propagation from list tap to runtime launch.
- Interpret `HdsNavigation` as a reason to redesign list behavior.

`HdsNavigation` should be used to implement the native page container, title, back behavior, and transitions. It should not silently change what a tap does.

### Runtime

Files:

- `pages/RunPage.ets`
- `components/runtime/RuntimeToolbar.ets`
- `components/runtime/RuntimeContainer.ets`

Changes:

- Convert toolbar into floating glass controls.
- Wrap viewport in `RuntimeViewportFrame`.
- Replace standard buttons with `FeaturePhoneKeypad`.
- Keep existing input codes and event dispatch unchanged.
- Keep duplicate key filtering.

Do not:

- Change native runtime behavior.
- Change key codes.
- Change XComponent id or library name.
- Let visual overlays intercept gameplay touches unintentionally.

### Import

Files:

- `pages/ImportPage.ets`
- `components/import/*`

Changes:

- Use same floating navigation.
- Glassify preview cards and result panels.
- Keep import legality language clear.

## Style Tokens

Start with local constants before introducing a full theme system.

Suggested names:

- `COLOR_BG_TOP`
- `COLOR_BG_BOTTOM`
- `COLOR_TEXT_PRIMARY`
- `COLOR_TEXT_SECONDARY`
- `COLOR_GLASS_LIGHT`
- `COLOR_GLASS_STROKE`
- `COLOR_ACCENT_CYAN`
- `COLOR_ACCENT_AMBER`
- `RADIUS_GLASS_L`
- `RADIUS_GLASS_M`
- `SHADOW_FLOATING`

Suggested visual values:

- Background: pearl/mist light.
- Primary text: graphite.
- Secondary text: translucent graphite.
- Accent: cyan for active states.
- Warning/accent: tiny warm amber only.

Avoid large saturated gradients.

## Implementation Order

1. Add shared visual primitives: `GlassSurface`, `GlassNotice`, `FloatingTitleBar`.
2. Add `HdsNavigationShell` in `ShellRootPage` while preserving the current business navigation semantics.
3. Update `PageHeader` and `StatusBanner` to use the new primitives or remove `PageHeader` after HDS navigation owns titles.
4. Build `FeaturePhoneKey`, `DPadControl`, and `FeaturePhoneKeypad`.
5. Refactor `RuntimeContainer` to use the keypad without changing input semantics.
6. Add `RuntimeViewportFrame`.
7. Polish app list rows.
8. Test on device for hit target size, safe area, text contrast, and runtime touch mapping.

## Device Test Checklist

- App list text remains readable over glass.
- Runtime viewport keeps 3:4 aspect ratio.
- Gameplay touch mapping still lands correctly.
- Every virtual key sends exactly one down/up pair.
- No key text is clipped.
- Keypad fits without being hidden by system navigation area.
- Toolbar actions remain reachable.
- Screen does not feel like Android Material.
- Runtime controls read as feature-phone keypad, not app buttons.
