# Open Source Release Status

This repository has been scrubbed for public source release, but these items
still need an explicit go/no-go decision before publishing binaries:

## Done

- Removed private signing paths and encrypted passwords from
  `build-profile.json5`
- Kept `local.properties` out of source control
- Removed the copied `mrpoid2018` reference snapshot from the public tree
- Added third-party and repository setup notes

## Still review before public binary distribution

### 1. External runtime dependency licensing

The current native stack expects:

- `vmrp-core/` licensed under GPLv3
- Unicorn shared libraries whose local source tree ships a GPLv2 `COPYING`

If you plan to publish binaries, confirm your exact redistribution model and
license compatibility for that combined stack.

### 2. Bundled runtime assets

Review every file under `entry/src/main/resources/rawfile/mrp/`, especially
demo packages, fonts, plugins, and compatibility assets, and replace any item
whose redistribution rights are unclear.

### 3. Final repository license

After the dependency and asset review is complete, add the final top-level
`LICENSE` file that matches the published source and binary distribution model.
