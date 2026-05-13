# Build And Install Runbook

Date: 2026-05-09

## Build

Run from the HarmonyOS project root:

```bash
cd /Users/machunjiang/mrp_emulator/harmonyos-mrp
/Applications/DevEco-Studio.app/Contents/tools/node/bin/node \
  /Applications/DevEco-Studio.app/Contents/tools/hvigor/bin/hvigorw.js assembleApp
```

Successful output ends with:

```text
BUILD SUCCESSFUL
```

The signed debug HAP is generated at:

```text
/Users/machunjiang/mrp_emulator/harmonyos-mrp/entry/build/default/outputs/default/entry-default-signed.hap
```

## Install

```bash
/Applications/DevEco-Studio.app/Contents/sdk/default/openharmony/toolchains/hdc install -r \
  /Users/machunjiang/mrp_emulator/harmonyos-mrp/entry/build/default/outputs/default/entry-default-signed.hap
```

Expected install result:

```text
install bundle successfully
```

## Start App

```bash
/Applications/DevEco-Studio.app/Contents/sdk/default/openharmony/toolchains/hdc shell aa start \
  -b com.xiaobai.mrpsimulator \
  -a EntryAbility
```

Expected start result:

```text
start ability successfully
```

## Stop App

```bash
/Applications/DevEco-Studio.app/Contents/sdk/default/openharmony/toolchains/hdc shell aa force-stop com.xiaobai.mrpsimulator
```

## Useful Log Markers

Look for these lines when validating runtime startup:

```text
MRP Start workDir=... package=... ext=start.mr
MRP startVmrpWithPath ret=0
MRP timer pump ...
MRP guiDrawBitmap ...
MRP copyFrame ...
MRP MRPDBG readFile pack-hit ...
MRP MRPDBG readFile gzip-ok ...
MRP input #... key action=...
```

For `mpc.mrp`, a healthy resource path should include gzip decompression for entries such as:

```text
MRPDBG readFile gzip-ok file=start.mr
MRPDBG readFile gzip-ok file=game.ext
```

## Build Warnings

Current ArkTS warnings about file APIs throwing exceptions are known. They do not block the debug build, but should be cleaned before release hardening.

The NAPI import warning for `libmrp_napi.so` is also expected as long as the `.d.ts` declaration remains aligned with the native exports.

