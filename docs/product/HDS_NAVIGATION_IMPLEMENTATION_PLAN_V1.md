# HDS Navigation Implementation Plan V1

Date: 2026-05-09

## Goal

Move the app shell to `HdsNavigation` while preserving current business behavior.

This is a visual and navigation-container refactor. It is not a runtime behavior refactor.

State rule:

- Use State Management V2 for new navigation shell work.
- Prefer `@ComponentV2`, `@Local`, and `@Param`.
- Do not add new `@State` or `@Prop` while introducing `HdsNavigation`.

Platform rule:

- Minimum compatible SDK: HarmonyOS `6.1.0(23)`.
- `HdsNavigation` and HDS liquid-glass effects are baseline UI dependencies.
- Do not design a lower-API fallback shell.

Product decision rule:

- Read `PRODUCT_UI_DECISIONS_LOCKED_V1.md` before changing navigation UI.
- Use `HdsTabs` for the home bottom floating navigation.
- Home bottom tabs are `应用`, `最近`, `设置`.
- Import is a floating plus / mini bar action, not a fourth tab.
- Do not add a custom opaque background behind the HDS floating tab bar.
- Follow official HDS examples for `HdsNavigation` and floating tabs; do not guess APIs.

Top navigation material rule:

- `HdsNavigation` and second-level `NavDestination` top bars must use the official HDS gradient blur material style.
- On HarmonyOS `6.1.0(23)+`, prefer `ScrollEffectType.IMMERSIVE_GRADIENT_BLUR`.
- Pair the scroll effect with:

```ts
systemMaterialEffect: {
  materialType: hdsMaterial.MaterialType.IMMERSIVE,
  materialLevel: hdsMaterial.MaterialLevel.ADAPTIVE
}
```

- Bind the title bar to scrollable content with `.bindToScrollable([scroller])`.
- Use `.ignoreLayoutSafeArea([LayoutSafeAreaType.SYSTEM], [LayoutSafeAreaEdge.TOP, LayoutSafeAreaEdge.BOTTOM])` when following the immersive HDS title-bar pattern and the page needs content/material to extend under system areas.
- Do not hand-build a fake top blur with plain `opacity`, `backgroundBlurStyle`, or gradients if the official HDS title-bar API can be used.
- From HarmonyOS `6.0.2(22)`, do not manually import `HdsNavigationAttribute` unless the current SDK/API specifically requires it.

## Current State

Current `ShellRootPage.ets` uses manual state:

```text
currentRoute: 'appList' | 'import' | 'run'
currentAppId: string
catalogRefreshTick: number
```

Current transitions:

```text
AppList -> Import
AppList -> Run(appId)
Import success -> AppList(refresh)
Run back -> AppList
```

These semantics should stay intact.

## Target State

Use `HdsNavigation` as the native page container.

Suggested destinations:

```text
HomeDestination
ImportDestination
RuntimeDestination(appId)
ProToolsDestination
SettingsDestination
```

The implementation should keep a typed destination model, not loose string routing scattered across components.

## Implementation Steps

### 1. Add HDS Dependency

The project currently has empty dependency blocks in:

```text
oh-package.json5
entry/oh-package.json5
```

Add the official HDS package required by HarmonyOS `6.1.0(23)` before importing `HdsNavigation`.

### 2. Introduce Destination Types

Create a small shell navigation model:

```text
Home
Import
Runtime(appId)
ProTools
Settings
```

Keep `appId` explicit for runtime.

### 3. Wrap Pages With HdsNavigation

Replace the manual page container with an HDS navigation container.

Preserve callbacks:

- `onNavigateImport`
- `onNavigateRun(appId)`
- `onBack`
- `onImportSuccess`

The visual navigation stack can change, but these intent callbacks should remain understandable.

### 4. Move Titles Into HDS Layer

Pages should stop owning plain debug headers once HDS navigation owns title behavior.

Migration:

- `PageHeader.ets` becomes temporary adapter or is removed later.
- Runtime keeps `FloatingTitleBar` for runtime-specific actions inside the page.

HDS title implementation sketch:

```ts
import {
  hdsMaterial,
  HdsNavigation,
  HdsNavigationTitleMode,
  ScrollEffectType,
} from '@kit.UIDesignKit'

private scroller: Scroller = new Scroller()
@Local materialLevel: hdsMaterial.MaterialLevel = hdsMaterial.MaterialLevel.ADAPTIVE
@Local materialType: hdsMaterial.MaterialType = hdsMaterial.MaterialType.IMMERSIVE

build() {
  HdsNavigation(this.pathStack) {
    Scroll(this.scroller) {
      // page content
    }
    .edgeEffect(EdgeEffect.Spring)
    .height('100%')
  }
  .titleBar({
    content: {
      title: {
        mainTitle: 'mrpmony',
      },
    },
    style: {
      scrollEffectOpts: {
        scrollEffectType: ScrollEffectType.IMMERSIVE_GRADIENT_BLUR,
      },
      systemMaterialEffect: {
        materialType: this.materialType,
        materialLevel: this.materialLevel,
      },
    },
  })
  .bindToScrollable([this.scroller])
  .titleMode(HdsNavigationTitleMode.MINI)
  .ignoreLayoutSafeArea([LayoutSafeAreaType.SYSTEM], [LayoutSafeAreaEdge.TOP, LayoutSafeAreaEdge.BOTTOM])
}
```

Adapt the code to StateV2 and the real page title/actions. The snippet records the required HDS pattern, not final production code.

### 5. Validate Runtime Safety

After HDS shell integration, test:

- Tapping a list row launches the selected `appId`.
- Import success refreshes the list.
- Runtime back returns to Home.
- XComponent still receives touch.
- Virtual keys still emit the same down/up pairs.
- No HDS overlay intercepts gameplay touches.

## Non-goals

Do not:

- Add a game detail page before launch.
- Change import result behavior.
- Change runtime lifecycle.
- Change input code mapping.
- Replace runtime toolbar logic with unrelated controls.
- Make Pro or Settings block core app launch.

## Suggested Shell Shape

```text
HdsNavigation
  HomeDestination
    AppListPage
  ImportDestination
    ImportPage
  RuntimeDestination(appId)
    RunPage(appId)
  ProToolsDestination
    ProToolsPage
  SettingsDestination
    SettingsPage
```

## Acceptance Criteria

The HDS navigation pass is complete when:

- The app visibly uses HDS-native page navigation.
- Existing launch/import/runtime flow still works.
- Page titles and back affordances feel system-native.
- App list behavior is unchanged.
- Runtime controls and viewport behavior are unchanged.
- No new online/content-store affordance appears.
