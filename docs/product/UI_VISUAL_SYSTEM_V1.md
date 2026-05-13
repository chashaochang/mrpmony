# UI Visual System V1

Date: 2026-05-09

## Goal

Define the visual system for the HarmonyOS HDS liquid-glass direction and feature-phone runtime controls.

The product should look native to modern HarmonyOS, while the runtime control surface should clearly evoke a Chinese keypad phone.

## Visual Formula

```text
HDS liquid glass shell
+ compact local-file utility layout
+ 240x320 MRP viewport
+ tactile feature-phone keypad
= MRP local runner identity
```

## Background

Use a light system background with subtle depth.

Recommended direction:

- Pearl white base.
- Mist gray lower layer.
- Very soft silver-blue/cyan environmental tint.
- Minimal warm amber only for notices or Pro emphasis.

Avoid:

- Heavy dark gamer background.
- Purple-blue SaaS gradient.
- Beige nostalgia theme.
- Pixel-art wallpaper.
- Decorative blobs or floating orbs.

## Liquid Glass Material

A glass surface must have more than opacity.

Required layers:

- Translucent fill.
- Bright top/edge rim.
- Soft inner highlight.
- Slight darker lower edge.
- Subtle shadow lifted from background.
- Background color bleed.

Suggested surface types:

### Navigation Glass

Use for HDS navigation/title areas.

Traits:

- Highest clarity.
- Light translucent fill.
- Very clean edge highlight.
- No heavy outline.
- Title remains readable over all backgrounds.

### Content Glass

Use for file rows, notices, settings groups.

Traits:

- Medium translucency.
- Softer edge.
- Pressed state slightly darkens and lowers.
- Row separators should be subtle or absent.

### Control Glass

Use for runtime title/action strip.

Traits:

- Compact.
- Higher contrast icons/text.
- Does not overlap gameplay.
- Touch targets remain clear.

## Feature-phone Key Material

Keys should not look like app buttons.

Key surface:

- Glossy plastic/rubber.
- Rounded rectangular or oval physical key shape.
- Beveled face.
- Inner highlight near top.
- Darker lower edge.
- Pressed state lowers, darkens bottom shadow, and moves highlight.

Key hierarchy:

- D-pad/OK largest.
- Soft keys medium.
- Call/hang-up distinct color cues.
- Number keys consistent grid.

Colors:

- Default key: graphite/silver translucent material.
- Active/OK: cool cyan highlight.
- Call: restrained green.
- Hang-up: restrained red.

Avoid:

- Flat filled rectangles.
- Android Material buttons.
- Console controller buttons.
- NES gamepad shapes.
- Large saturated candy colors.

## MRP Viewport Frame

The viewport should be a modern glass/device frame around old content.

Frame traits:

- Stable 3:4 area.
- Black or deep graphite fallback.
- Thin glass rim.
- Slight inner shadow.
- No fake console bezel.
- No cartridge or arcade motifs.

The MRP content itself can be low-resolution. The surrounding shell should be polished and modern.

## Typography

Use compact, system-like hierarchy.

Guidelines:

- Title: clear, not oversized.
- Row title: readable and dense.
- Metadata: smaller, graphite with opacity.
- Button labels: short Chinese text.
- Key labels: centered, stable, no clipping.

Avoid:

- Hero-scale text inside tool screens.
- Negative letter spacing.
- Decorative pixel fonts.
- Long instructional paragraphs in the UI.

## Motion

Motion should feel system-native.

Allowed:

- HDS navigation transition.
- Glass surface lift on press.
- Key depression on touch.
- Small light shift on selected control.
- Smooth runtime setting panel entrance.

Avoid:

- Bouncy arcade animations.
- Particle effects.
- Flashing retro effects.
- Motion that shifts viewport or keypad layout during gameplay.

## Layout Density

This is a tool and runtime, not a marketing page.

Rules:

- Home can be moderately dense.
- Runtime prioritizes viewport and keypad.
- Do not use large hero sections.
- Do not nest cards inside cards.
- Do not make repeated page sections look like floating marketing cards.

## Component Style Checklist

Before accepting a UI pass:

- Does the shell look HarmonyOS/HDS-native?
- Does the glass have rim, depth, and inner light?
- Does the keypad read as a physical phone keypad?
- Does the viewport stay stable?
- Does Home still feel like a local file tool?
- Did any visual change alter launch/import/runtime behavior?
- Did any text or key label clip on device?
- Does the app avoid game-store language?

