# UI Style Direction: HDS Liquid Glass

Date: 2026-05-09

Canonical decisions:

- See `PRODUCT_UI_DECISIONS_LOCKED_V1.md` before starting any new UI work.
- The approved visual reference is `docs/product/reference/gpt-image2-liquid-glass-keyphone-direction.png`.
- The hand-authored `home-redesign-v3-liquid-keyphone.svg` is rejected as a visual target.

## Design Goal

The app should look like a modern HarmonyOS 6.1 native app, while the runtime controls should clearly evoke a Chinese feature phone / shanzhai phone MRP device.

Minimum platform:

- HarmonyOS `6.1.0(23)`.
- HDS effects are baseline product requirements.

The target feeling:

> HarmonyOS HDS system material on the outside, old keypad-phone MRP soul on the inside.

This is not a retro console emulator visual direction.

The accepted reference image is the first `gpt-image-2` concept: three polished phone screens with liquid-glass surfaces, a physical feature-phone runtime shell, and a calm Pro toolbox. Future implementation should preserve that level of material quality and product taste.

## Keywords

Use:

- HarmonyOS native
- HDS
- immersive light
- liquid glass
- floating navigation
- floating tabs
- translucent system material
- refraction
- edge highlights
- internal glow
- layered depth
- tactile keypad
- feature phone
- shanzhai phone
- 240x320 screen

Avoid:

- Android Material
- flat cards
- generic frosted glass
- NES style
- arcade style
- ROM library style
- game store style
- pixel-art-first visual identity
- heavy dark gamer UI
- neon cyberpunk UI

## Visual Structure

### App Shell

The app shell should use HarmonyOS-like HDS surfaces:

- `HdsNavigation` as the primary page shell.
- Floating translucent navigation bars.
- Floating tab or segmented controls where useful.
- Glass panels that reveal blurred background color through the material.
- Soft shadows and depth separation.
- Clear hierarchy with calm spacing.
- Refined Chinese typography.

The glass should not be just `opacity + blur`. It needs visible material behavior:

- Bright rim highlights.
- Subtle inner light.
- Slight background distortion/refraction.
- Layered translucency.
- Small specular highlights on edges.
- System-level polish rather than decorative glass cards.

### Runtime Area

The runtime screen should feel like an old mobile app running inside a modern shell:

- Keep the MRP viewport visually close to 240x320.
- Use a subtle glass or device-like frame around the viewport.
- The content inside the viewport can look low-resolution, but should resemble old Chinese feature-phone UI, not console pixel art.
- Avoid fake NES tiles, cartridge metaphors, or gamepad-console tropes.

### Keypad Area

The keypad is the core identity element.

It should look like a classic physical phone keypad:

- Soft-left and soft-right keys.
- Green call key and red hang-up key when layout allows.
- D-pad ring with OK center.
- Numeric keys 1-9.
- `*`, `0`, `#`.
- Glossy plastic or rubber feel.
- Tactile bevels.
- Pressed highlights.
- Slight material reflection.

The keys should not look like ordinary rectangular app buttons.

## Suggested Screens

### Home

Purpose: manage local files.

Elements:

- Title: `mrpmony`.
- Subtitle/positioning: `MRP 怀旧模拟器`.
- Local-only notice: `仅运行你拥有或已获授权的本地文件`.
- Sections: `本地文件`, `最近运行`, `导入 MRP`.
- File rows with name, filename, and status.
- No download center.
- No third-party game recommendations.

### Runtime

Purpose: play/run one package.

Elements:

- Floating title bar with package name.
- 240x320 viewport.
- Feature-phone keypad.
- Minimal runtime actions: back, restart, screenshot, settings.
- Optional compatibility hint/log entry point.

### Pro Tools

Purpose: paid tool features without content selling.

Elements:

- Title: `Pro 工具箱`.
- Rows: `按键布局`, `每游戏配置`, `存档管理`, `兼容性日志`, `外观皮肤`.
- Tasteful Pro badge.
- No aggressive paywall language.

## Color And Material

Recommended palette:

- Pearl white.
- Mist gray.
- Silver blue.
- Soft cyan.
- Graphite text.
- Small warm amber accents.

Use color mainly through:

- Background content bleeding through glass.
- Gentle active states.
- Key highlights.
- Pro badge accent.

Avoid:

- Large saturated gradients.
- Purple-blue SaaS gradient dominance.
- Dark gamer panels.
- Beige/brown nostalgia theme.
- One-note monochrome blue.

## Motion Direction

Future motion should be subtle and system-like:

- Floating surface lift on press.
- Keypad press depression.
- Tiny light sweep on active glass controls.
- Smooth navigation transition.
- Viewport should remain visually stable during gameplay.

Avoid:

- Bouncy game UI.
- Excessive particle effects.
- Cartoon transitions.
- Arcade-style flashes.

## Implementation Notes For ArkTS

When implementing:

- Prefer HarmonyOS native components and HDS-style component structure.
- Use `HdsNavigation` for page container, title, back behavior, and transitions.
- Preserve current business navigation semantics while replacing the shell.
- Keep runtime controls stable in size to avoid layout shift.
- Use clear safe-area handling.
- Treat glass as a reusable material style.
- Keep text readable over translucent surfaces.
- Make keypad hit targets large enough for real gameplay.
- Test on actual device, not only preview.

Potential reusable style concepts:

- `GlassSurface`
- `HdsNavigationShell`
- `FloatingTitleBar`
- `GlassListRow`
- `FeaturePhoneKey`
- `DPadControl`
- `RuntimeViewportFrame`

## App Store Visual Guidance

Store screenshots should show:

- Local file management.
- Runtime viewport.
- Feature-phone keypad.
- Pro tool features.

Store screenshots should not show:

- Copyrighted game characters.
- Third-party game covers.
- Download-center UI.
- ROM language.

## One-Line Direction

> A HarmonyOS 6.1 HDS liquid-glass local emulator shell wrapped around a tactile Chinese feature-phone MRP control surface.
