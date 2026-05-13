# ArkTS State Management Rules V1

Date: 2026-05-09

## Rule

New HarmonyOS UI state should use State Management V2.

Minimum platform for UI work:

- HarmonyOS `6.1.0(23)`.
- StateV2 and HDS-oriented UI work can assume API 23 as the floor.

Prefer:

- `@ComponentV2` for new components.
- `@Local` for local mutable state.
- `@Param` for parent-provided input.
- StateV2 event/callback patterns for newly designed component APIs.

Avoid adding new V1 state decorators:

- `@State`
- `@Prop`
- `@Link`
- `@Provide`
- `@Consume`
- `@ObjectLink`

## Migration Policy

Do not convert the entire app in one visual pass.

Use this order:

1. New components use StateV2 from the start.
2. Components touched for non-trivial UI work should be candidates for V2 conversion.
3. Existing V1 components can remain temporarily if converting them would risk runtime behavior.
4. Runtime and input behavior takes priority over decorator churn.

## Current Codebase Note

The current ArkTS shell still contains V1 decorators in existing pages and components.

Known V1 areas:

- `ShellRootPage.ets`
- `AppListPage.ets`
- `ImportPage.ets`
- `RunPage.ets`
- common display components with `@Prop`

These should be migrated deliberately, page by page, after the visual shell stabilizes.

## Acceptance Criteria For Future UI Work

- No new component should introduce `@State` or `@Prop`.
- New visual primitives such as glass surfaces, title bars, keypad pieces, Pro tools, and Settings should start with `@ComponentV2`.
- When modifying runtime controls, preserve key codes, native lifecycle, and XComponent wiring.
- If a component cannot be migrated safely yet, document why and keep the diff narrow.
