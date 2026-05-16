# Third-Party Notices

This repository has been cleaned for public source release, but a few runtime
dependencies still matter when you build or redistribute the app.

## Local-only items removed from source control

- Real HarmonyOS signing certificates, profiles, and keystores
- Private machine paths from `build-profile.json5`
- The copied `mrpoid2018/origin` experiment snapshot

## External native dependencies

### vMRP core

The native runtime is wired against a sibling `vmrp-core/` directory through
`entry/src/main/cpp/CMakeLists.txt`. That dependency is not vendored inside this
repository. Review the upstream project license and keep your redistribution
terms compatible with it before publishing binaries.

### Unicorn Engine

Local builds also expect Unicorn headers from a sibling `unicorn-src-proxy/`
directory, and the app currently links against the prebuilt shared libraries in:

- `entry/src/main/libs/arm64-v8a/libunicorn.so`
- `entry/src/main/libs/arm64-v8a/libunicorn.so.2`

When redistributing binaries, keep the upstream Unicorn copyright and license
text with your release materials.

## Runtime assets

This repository still contains runtime compatibility assets under
`entry/src/main/resources/rawfile/mrp/`. Before a public binary release, review
the provenance and redistribution rights of those assets and replace any item
whose license is unclear.
