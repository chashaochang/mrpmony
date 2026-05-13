# HarmonyOS AppGallery Release Strategy V1

## Positioning

The HarmonyOS version should be positioned as a local MRP runtime tool, not as a game distribution product.

Recommended wording:

- MRP local file runner
- Local MRP compatibility test tool
- Personal file runtime environment
- Local imported file manager

Avoid wording:

- Free classic games
- Massive game library
- Built-in game center
- Play copyrighted games for free
- Compatible with all commercial MRP games

## Main Review Risks

### Copyright

This is the highest risk area.

The app package should not include third-party MRP games, commercial game assets, copyrighted screenshots, brand names, or downloadable game content without written authorization.

The safest first release should only include one of these:

- No bundled MRP package.
- A self-made demo package with clear ownership.
- A minimal test package created only for compatibility verification.

### Content Distribution

Do not provide an online MRP resource library in the first AppGallery release.

Avoid:

- Built-in download center.
- Ranking/recommendation pages for third-party MRP files.
- Links to third-party game package sites.
- Cloud search for MRP resources.

If online content is added later, each item needs traceable copyright permission.

### User Imported Files

The app can support local import, but the UI and store materials should make the boundary clear:

- Users are responsible for the legality of imported files.
- The app does not provide, host, sell, or distribute third-party MRP content.
- The app is only a local runtime and compatibility tool.

Recommended in-app notice:

> Please only import files that you own or are authorized to use. This app does not provide or distribute third-party MRP content.

## Monetization

Charging is possible, but the first release should prefer free download plus optional paid upgrades.

Recommended first monetization path:

- Free download.
- In-app purchase for advanced tool features.
- Optional donation/support purchase.

Better paid feature candidates:

- Multiple save slots.
- Custom virtual key layouts.
- Landscape/portrait layout presets.
- Per-game control profiles.
- Batch file management.
- Cloud backup for app settings and save data.
- Advanced compatibility options.

Avoid selling:

- Game packages.
- Access to third-party MRP content.
- A paid "game library" membership.

## Store Listing Guidance

Recommended title style:

- mrpmony - MRP Nostalgia Emulator
- MRP Local Runner
- MRP Runtime Tool
- MRP File Player

Chinese title examples:

- mrpmony - MRP怀旧模拟器
- MRP 文件运行工具
- MRP 兼容测试工具

Recommended short description:

> A local MRP file runtime tool for managing and testing files you own.

Chinese short description:

> 用于管理和运行个人本地 MRP 文件的工具。

Avoid using screenshots that show recognizable copyrighted games unless authorization is available.

## First Release Scope

Recommended first AppGallery build:

- Local import from user storage.
- Runtime screen.
- Virtual key controls.
- Basic app list.
- Basic save/config storage if available.
- Clear legal notice.
- No online resource library.
- No bundled third-party games.

## Privacy And Permissions

Keep permissions minimal.

Expected permissions:

- Local file picker or document access for importing user-selected files.
- App internal storage for copied runtime assets, configs, and saves.

Avoid requesting broad permissions unless there is a direct feature need.

The privacy statement should mention:

- Imported files are processed locally.
- The app does not upload MRP files by default.
- Runtime configs and save data are stored locally unless cloud backup is explicitly added later.

## Review Preparation Checklist

- [ ] Remove third-party MRP packages from the release build.
- [ ] Remove copyrighted screenshots and brand names from store materials.
- [ ] Add an in-app legal/import notice.
- [ ] Prepare a self-made demo or test package if a demo is needed.
- [ ] Confirm the app launches correctly from the desktop icon.
- [ ] Confirm import, run, pause, exit, and restart flows.
- [ ] Confirm no crash after right-soft-key or exit events.
- [ ] Confirm privacy policy matches actual data behavior.
- [ ] Confirm paid features do not sell or imply access to third-party content.

## Recommended Roadmap

V1:

- Free local runtime tool.
- No content distribution.
- Focus on stability and compatibility.

V1.1:

- Add import management and per-file metadata.
- Add better control layout options.

V1.2:

- Add optional paid advanced tool features through Huawei IAP.

Future:

- Online content only if every item has explicit authorization.
- Consider enterprise/custom-device distribution for features that require system-level integration.
