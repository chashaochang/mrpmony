# mrpmony

HarmonyOS MRP simulator project.

This repository contains the ArkTS shell, the HarmonyOS app structure, and the
native bridge layer used to drive the runtime. It is being cleaned up for a
public source release, so local signing materials and private machine paths are
not stored here.

## What is in this repo

- HarmonyOS / DevEco project skeleton
- ArkTS pages, list management, import flow, settings, and runtime UI
- Native NAPI bridge and host-side rendering/input glue
- Store screenshots under `screenshots/store/`

## What is not in this repo

- Real local signing certificates, keystores, and passwords
- `local.properties` for a specific machine
- The private `mrpoid2018/origin` reference snapshot
- A vendored copy of external sibling dependencies such as `vmrp-core/`

## Local setup

1. Copy `local.properties.example` to `local.properties`
2. Fill in your local `hwsdk.dir`
3. Replace the placeholder signing entries in `build-profile.json5`
4. Open the project root in DevEco Studio

```bash
cp local.properties.example local.properties
```

## Build note

`entry/src/main/cpp/CMakeLists.txt` expects these sibling directories outside
this repo:

- `../vmrp-core`
- `../unicorn-src-proxy`

If your local layout differs, update the paths in
`entry/src/main/cpp/CMakeLists.txt`.

## Open-source notes

- See `THIRD_PARTY_NOTICES.md` for dependency and redistribution notes.
- See `OPEN_SOURCE_RELEASE_STATUS.md` for the remaining publish checklist.
- Demo assets and runtime compatibility assets should be reviewed before any
  public binary release.
- `screenshots/store/` contains the polished store materials intended for the
  release listing.
