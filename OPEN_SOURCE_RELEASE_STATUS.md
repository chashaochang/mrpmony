# Open Source Release Status

This repository has been scrubbed for public source release, but these items
still need an explicit go/no-go decision before publishing binaries:

## Done

- Removed private signing paths and encrypted passwords from
  `build-profile.json5.example`
- Kept `local.properties` out of source control
- Kept the real root `build-profile.json5` out of source control
- Removed the unused experimental runtime branch from the public tree
- Added third-party and repository setup notes
- Moved the active `vmrp` and `unicorn` dependency files into vendored modules

## Still review before public binary distribution

### 1. External runtime dependency licensing

The current native stack expects:

- vendored `vmrp` sources under `entry/src/main/cpp/third_party/vmrp-core`
- vendored Unicorn headers plus the prebuilt shared library under
  `entry/src/main/cpp/third_party/unicorn` and `entry/src/main/libs/arm64-v8a`

If you plan to publish binaries, confirm your exact redistribution model and
license compatibility for that combined stack.

### 2. Bundled runtime assets

Review every file under `entry/src/main/resources/rawfile/mrp/`, especially
demo packages, fonts, plugins, and compatibility assets, and replace any item
whose redistribution rights are unclear.

### 3. Final repository license

After the dependency and asset review is complete, add the final top-level
`LICENSE` file that matches the published source and binary distribution model.
