# Product UI Decisions Locked V1

Date: 2026-05-09

This document records product and UI decisions that have already been agreed.

Future UI work should start here before changing code. Do not reinterpret the product direction from scratch.

## Canonical Visual Reference

The approved visual direction is the first `gpt-image-2` concept image:

```text
docs/product/reference/gpt-image2-liquid-glass-keyphone-direction.png
```

This image is the baseline for visual taste, not a literal pixel-perfect implementation target.

What to preserve from it:

- HarmonyOS / iOS-like liquid glass atmosphere.
- High-end translucent material with refraction, rim light, depth, and soft highlights.
- Feature-phone / shanzhai-phone MRP identity.
- Runtime screen embedded inside a physical keypad-phone shell.
- Bottom navigation as a floating glass HDS-style surface.
- Pro / settings screen as a calm native tool panel.
- App shell feels modern and native; emulator core feels like old keypad-phone MRP.

What not to copy blindly:

- The exact sample content.
- Any text that conflicts with current product naming.
- Any copyrighted game imagery or third-party content.
- Any layout detail that is impossible or unsafe in ArkTS/HDS.

## Product Direction

The app is a local MRP emulator shell for HarmonyOS.

Product naming:

- Brand/app name: `mrpmony`.
- Chinese descriptive subtitle: `MRP 怀旧模拟器`.
- Recommended store display name: `mrpmony - MRP怀旧模拟器`.
- In primary UI, use `mrpmony` as the title and explain the product with supporting text.

It is not:

- A game store.
- A ROM marketplace.
- A download center.
- A NES / arcade / console emulator skin.
- An Android Material-style file manager.
- A generic AI-looking dashboard.

One-line direction:

> A HarmonyOS HDS liquid-glass local MRP launcher wrapped around a tactile Chinese feature-phone control surface.

## Visual Direction

Use:

- HarmonyOS native style.
- HDS liquid glass.
- Immersive light material.
- Transparent / translucent backgrounds.
- Cool white, mist gray, silver blue, soft cyan.
- Subtle low-saturation green only for app-type differentiation.
- Physical keypad materials: glossy plastic, rubber, bevels, reflections.

Avoid:

- Yellow as a primary UI color.
- Big opaque blocks behind the bottom tab.
- Purple-blue SaaS gradients.
- Beige/brown nostalgia theme.
- Heavy dark gamer UI.
- NES-style or handheld-console visual metaphors.
- Decorative card piles and marketing landing-page composition.
- UI that looks like a quick AI mockup.

## Icon Asset Rule

All custom product icons must be generated bitmap assets from imagegen and checked into the app resources. Do not ship placeholder glyphs, emoji-like text characters, hand-drawn ArkUI shape icons, or mismatched SVG stand-ins for product UI icons.

Allowed exceptions:

- Official HDS / system-provided navigation symbols.
- Text-only buttons where no icon is needed.
- Runtime keypad labels that intentionally reproduce feature-phone keys.

## App Icon Direction

The app icon should follow HarmonyOS-style icon taste:

- Comfortable rounded-square app icon shape with enough safe-area padding.
- Clean, readable subject at launcher size.
- Liquid-glass / translucent material, with soft depth, rim light, and restrained highlights.
- Blue-white, silver-blue, and soft cyan as the main colors.
- A feature-phone / shanzhai-phone MRP cue: small 240x320 screen plus tactile keypad.
- Optional tiny `MRP` mark only if it remains readable and does not crowd the icon.

Avoid:

- Full `mrpmony` wordmark inside the icon.
- NES, handheld-console, gamepad, arcade, or Android Material metaphors.
- Yellow as a primary color.
- Busy screenshots or copyrighted game imagery.

## Navigation Decisions

Use `HdsNavigation` for page navigation and destination management.

`HdsNavigation` and second-level `NavDestination` title bars must use the HDS gradient blur material style.

Required title-bar direction:

- Use the official HDS title bar API, not a custom fake header.
- Use `ScrollEffectType.IMMERSIVE_GRADIENT_BLUR` on API 23+ for immersive top title areas.
- Pair the scroll effect with `systemMaterialEffect`.
- Preferred material:
  - `materialType: hdsMaterial.MaterialType.IMMERSIVE`
  - `materialLevel: hdsMaterial.MaterialLevel.ADAPTIVE`
- Bind the title bar to the page scrollable via `bindToScrollable(...)` when the page content scrolls.
- For non-immersive list pages, `ScrollEffectType.GRADIENT_BLUR` may be considered only if it better matches the official HDS guidance; the default product direction is still immersive liquid glass.
- Do not manually recreate this with plain opacity, blur, or gradient overlays unless HDS cannot support the page.

Use `HdsTabs` for the home bottom navigation floating bar.

The home bottom navigation has two main tabs:

- `应用`
- `设置`

`最近` is removed for now because runtime history is not yet recorded as a real product data source. Restore it only when launching an MRP writes durable recent history and the recent page has verified layouts.

Import is not a fourth tab.

Import should be exposed as a floating plus / mini bar action near the bottom navigation, consistent with HDS floating tab behavior.

For HDS floating tabs, follow official documentation and examples. Do not guess API shape from SDK names.

Required bottom-tab constraints:

- `barPosition(BarPosition.End)`
- `vertical(false)`
- `barOverlap(true)`
- floating `barFloatingStyle`
- no custom opaque bottom background behind the HDS bar
- list content should extend behind the floating bar

## Home Screen Decisions

The home screen is the main local library and launcher.

Primary task:

- Find an imported MRP.
- Distinguish games and apps.
- Launch quickly.

The list is the focus.

Top content must stay compact. Do not let title/status panels push the list out of the first screen.

Do not add:

- Big hero cards.
- Marketing explanations.
- Feature descriptions.
- Batch-management controls on the main list.
- Fixed/pinned controls in primary list rows.
- Extra summary rows that repeat the counts already shown by filters.

Recommended home structure:

- Compact title/status layer.
- Search.
- Segmented filter: `全部 / 游戏 / 应用`.
- Dense local list.
- Floating HDS tab + import plus action.

## App/Game Differentiation

Games and apps must be visibly distinguishable.

Use multiple cues:

- Segment filter counts.
- Left-side MRP type mark.
- Row right-side type label.
- Different but restrained tint for game and app.

The `应用` filter must show actual application entries and must be easy to find.

Classification must not put obvious games into the app category when avoidable.

## Runtime Screen Decisions

The runtime page should make the MRP shell identity strongest.

Required direction:

- MRP viewport resembles a 240x320 feature-phone screen.
- Physical keypad shell below or around it.
- Soft-left, soft-right, call, hang-up, D-pad/OK, numeric keys, `*`, `0`, `#`.
- Keys should feel tactile, not like generic app buttons.

Avoid:

- NES controller style.
- Generic touch-gamepad style.
- Overlapping controls on gameplay content.
- Resizing or shifting the viewport during gameplay.

## Technical Decisions

Minimum API:

- HarmonyOS `6.1.0(23)`.

State management:

- New UI work uses State Management V2.
- Prefer `@ComponentV2`, `@Param`, `@Local`, `@Event`, `@Monitor`.
- Do not introduce new `@State` / `@Prop` in new components.

Implementation process:

1. Produce or select a visual design reference first.
2. Write the implementation target/spec.
3. Implement against that spec.
4. Build.
5. Install on device.
6. Capture screenshots.
7. Compare against the reference and acceptance criteria.

Do not:

- Keep adding features to compensate for weak design.
- Claim a visual issue is fixed without screenshot verification.
- Randomly change routes, list behavior, or runtime behavior while doing visual work.
- Guess HDS APIs without checking official docs/examples.

## Current Rejected Direction

The hand-authored SVG direction below is rejected as a visual target:

```text
docs/product/home-redesign-v3-liquid-keyphone.svg
```

Reason:

- It looks like an engineering wireframe, not a polished product design.
- It loses the high-fidelity glass and device-material quality of the approved `gpt-image-2` reference.
- It should not be used as the basis for implementation.
