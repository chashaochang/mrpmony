# Third-Party Notices

This repository has been cleaned for public source release, but a few runtime
dependencies still matter when you build or redistribute the app.

## External native dependencies

### vMRP core

The native runtime is built from the vendored module at:

- `entry/src/main/cpp/third_party/vmrp-core`

Upstream project:

- https://github.com/zengming00/vmrp

That code path is currently part of the shipped native runtime. Review the
upstream license and keep your redistribution terms compatible with it before
publishing binaries.

### Unicorn Engine

The repository now carries the Unicorn headers under:

- `entry/src/main/cpp/third_party/unicorn/include`

Upstream project:

- https://github.com/unicorn-engine/unicorn

The app links against the prebuilt shared library in:

- `entry/src/main/libs/arm64-v8a/libunicorn.so.2`

The OHOS rebuild path currently depends on a local patch to the Unicorn source
tree `CMakeLists.txt` so it can recognize `CMAKE_SYSTEM_NAME STREQUAL "OHOS"`
and map `OHOS_ARCH` to the Unicorn target architecture.

When redistributing binaries, keep the upstream Unicorn copyright and license
text with your release materials.

## Runtime assets

This repository still contains runtime compatibility assets under
`entry/src/main/resources/rawfile/mrp/`. Before a public binary release, review
the provenance and redistribution rights of those assets and replace any item
whose license is unclear.
