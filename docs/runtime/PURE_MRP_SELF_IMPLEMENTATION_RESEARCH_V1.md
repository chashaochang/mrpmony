# Pure MRP self-implementation research

Date: 2026-05-12

Goal: evaluate whether we can build our own non-vMRP/non-Unicorn MRP runtime by
using mrpoid2018 and the public `vmrp/*` repositories as behavioral references,
without copying license-incompatible code into a closed-source commercial build.

## Sources checked

- `android-mrpoid2018` local copy.
- `https://github.com/vmrp` organization.
- `vmrp/vmrp`
- `vmrp/vmrp.github.io`
- `vmrp/MythroadSDK`
- `vmrp/mpc`
- `vmrp/vmrp_arm`
- `vmrp/mrpbuilder`
- `vmrp/unicorn_engine_tutorial`
- `vmrp/ems`
- `vmrp/mrpdev`
- `vmrp/emrp`
- `vmrp/mythroad`

## What each repository is useful for

| Repository | Usefulness for us | License posture |
| --- | --- | --- |
| `vmrp/vmrp` | Best reference for modern emulator architecture, MRP loading, Unicorn integration, ext loading, r9/r10 notes, file/network behavior. | GPL-3.0. Do not copy into closed-source runtime. |
| `vmrp/vmrp_arm` | Historical standalone Mythroad-layer direction. Useful to understand where full Mythroad support was attempted. | GPL-3.0. Reference only for closed-source. |
| `vmrp/mrpbuilder` | GCC MRP build path, useful for generating controlled test MRP cases. | GPL-3.0. Tools/reference only. |
| `vmrp/unicorn_engine_tutorial` | Unicorn examples. Not helpful if the goal is no Unicorn. | MIT, but avoid depending on Unicorn for closed-source path. |
| `vmrp/MythroadSDK` | Very useful documentation/sample source for early Lua-like Mythroad `.mr` format, framework functions, timers, global APIs. | No clear repo license in GitHub page. Treat as reference/docs only. |
| `vmrp/mrpdev` | SDK docs, API CHM/PDF archives, MRP simulator solution notes, extension API material. Very useful as a behavior spec source. | No clear license. Reference only. |
| `vmrp/mythroad` | Realistic sample filesystem: `DSM_GM.mrp`, `system`, plugins, config, sample `.mrp` files. Useful to understand expected filesystem layout. | No clear license and includes third-party content. Do not bundle. |
| `vmrp/emrp` | MRP package structure: README says MRP is a bundle of gzip-compressed files and uncompressed packaging can run. Useful for parser validation. | No clear license. Reference only. |
| `vmrp/mpc` | MRP-era app and development environment. Useful as compatibility corpus and API usage examples. | No clear license; contains binaries/archives. Do not bundle. |
| `vmrp/ems` | Real C MRP app source. Useful as small API usage sample. | No clear license. Reference only. |
| `vmrp/vmrp.github.io` | Web packaged output and demo resources; useful to compare runtime assets. | No clear license. Do not copy assets. |

## Feasibility conclusion

We can implement our own runtime, but there are two very different meanings:

1. Clean-room compatible runtime for closed-source commercial use.
   - Feasible.
   - Do not copy `vmrp/vmrp`, `vmrp_arm`, mrpoid2018 native code, or bundled assets.
   - Use the repos as behavior references only.
   - Need our own loader, platform API layer, and execution strategy.

2. Fast port by copying mrpoid2018/vmrp code.
   - Technically faster.
   - Not clean for closed-source commercial use because `vmrp/vmrp` and `vmrp_arm`
     are GPL-3.0, while mrpoid2018 and several vmrp archive repos have unclear
     licensing.

## Recommended architecture

Keep the current app UI and product layer. Add a runtime backend boundary:

```text
ArkTS UI
  -> NativeRuntimeAdapter
  -> MrpFacade
  -> IRuntimeBackend
       -> VmrpBackend          current GPL/open-source path
       -> PureMrpBackend       future clean-room path
```

`PureMrpBackend` should have these internal modules:

- `MrpPackageReader`
  - parse MRP header and embedded file table
  - detect compressed/uncompressed entries
  - expose virtual file reads by in-package path

- `MythroadFs`
  - emulate `mythroad/`, `system/`, plugins, save files
  - implement case-insensitive lookup where Mythroad expects it
  - define `lookfor`, directory listing, rename/remove, path normalization

- `MythroadApi`
  - drawing, text, bitmap, timers, input, audio, edit, network, sysinfo
  - API table numbers and calling conventions derived from docs/tests, not copied

- `MrArmExecutor`
  - execute ARM/Thumb MRP ext code
  - options:
    - self-written interpreter for the subset used by MRP
    - generated interpreter from public ARM docs
    - commercial/permissive CPU core after legal review

- `CompatibilityHarness`
  - run tiny synthetic MRP cases for file/audio/input/edit/network
  - compare behavior against current vMRP backend and known device expectations

## Key technical findings from vmrp

- C-language MRP is ARM machine code. mrpoid could directly run it on ARM
  Android by loading code into memory and patching the function table.
- `vmrp/vmrp` uses Unicorn to run ARM code so it is not tied to an ARM host CPU.
- `vmrp/vmrp` eventually moved most Mythroad behavior into ARM-side code loaded
  through `cfunction.ext`; its own C Mythroad layer is mainly used to load ext.
- The r9/r10 register behavior is a core compatibility trap. ext global state is
  tied to r9, and callbacks between ext/Mythroad layers can corrupt r9/r10 if
  not modeled carefully.
- Early Mythroad `.mr` is Lua 5.0.2-derived; modern C MRP often contains ext
  code, so Lua compatibility alone is not enough.

## Implementation phases

### Phase 0: preserve references

- Keep `experimental/mrpoid2018/origin` as a read-only reference snapshot.
- Do not wire reference code into production CMake.
- Add notes and tests based on observed behavior.

### Phase 1: package + filesystem

Target: inspect/import/list/extract MRP reliably without executing it.

- Implement package parser.
- Add tests using synthetic packages.
- Implement virtual path mapping and save-file layout.
- Reuse current app import/manage UI.

Estimated: 1-2 weeks.

### Phase 2: API contract tests

Target: define expected Mythroad semantics before writing the executor.

- Build small test MRP programs using available public tools/docs.
- Cover file, `lookfor`, rename/remove, input, edit text, timer, audio, network.
- Run tests on current backend and record expected behavior.

Estimated: 2-4 weeks.

### Phase 3: executor prototype

Target: run one C hello-world MRP and draw a frame.

- Implement or integrate a legally clean ARM/Thumb executor.
- Implement memory map, function table, callbacks, r9/r10 behavior.
- Support drawing and timer.

Estimated:
- 4-8 weeks if using a permissive/commercial CPU core.
- 2-4 months if writing our own interpreter.

### Phase 4: product compatibility

Target: run common apps and games close to current compatibility.

- Fill API gaps.
- Harden audio, edit, network, saves, and lifecycle.
- Add compatibility matrix and crash diagnostics.

Estimated: 2-4 additional months.

## Practical recommendation

Start a `PureMrpBackend` clean-room branch, but keep current vMRP backend as the
shipping open-source/free path. The first concrete deliverable should not be a
full emulator. It should be:

1. a clean MRP package parser,
2. a virtual Mythroad filesystem implementation,
3. a test harness that can compare behavior against current vMRP.

Once those are stable, decide whether to buy/use a permissive ARM executor or
write our own interpreter.

This route is realistic, but only if we are disciplined about clean-room rules:
reference docs and behavior are fine; copying GPL/unclear source into a closed
runtime is not.

## First implementation step

Added an independent `pure_mrp/MrpPackageReader` module. It does not use copied
source from mrpoid2018 or vMRP. It implements the package facts we can treat as
format behavior:

- `MRPG` magic at the beginning of the package.
- little-endian header fields for info size, package size, and file-list offset.
- fixed metadata byte ranges for internal filename, app name, app id, version,
  vendor, and description.
- file-list entries made of name length, name bytes, package offset, and size.
- optional gzip expansion for embedded files.

The reader is compiled into the native target but is not yet used by the
shipping runtime.

## Second implementation step

Added an independent `pure_mrp/MythroadFs` module. It is still not wired into
the shipping runtime, but it gives the clean backend a shared filesystem
contract:

- normalizes logical Mythroad paths and protects against `..` escaping;
- resolves existing disk paths with ASCII case-insensitive segment matching;
- supports `lookfor` across package entries and the host filesystem;
- reads package entries first, then physical files;
- supports physical write, rename, remove, size, and directory listing;
- merges package entries into directory listings without duplicating physical
  names.

This is the first place where current product bugs around path case, `lookfor`,
directory listing, and file mutation can be turned into backend-independent
tests.

## Third implementation step

Added native developer probes:

- `inspectPureMrpPackage(path)`
  - returns parsed package metadata and package entries.
- `probePureMythroadFs({ rootDir, packagePath, path, listDir })`
  - initializes the clean filesystem layer;
  - optionally attaches a parsed package;
  - runs normalize, `lookfor`, size, and directory-list checks.

These are deliberately diagnostic APIs. They make the clean-room backend
measurable before it becomes the active runtime.

## Fourth implementation step

Added a self-written `pure_mrp/PureMrpBackend` shell and exposed it through the
diagnostic NAPI function `probePureMrpBackend(options)`.

Current backend scope:

- initializes a clean Mythroad work directory;
- loads and parses an MRP package through `MrpPackageReader`;
- attaches package entries to `MythroadFs`;
- moves into a prepared state through `Start()`;
- exposes `lookfor`, resource size, resource read byte count, directory listing,
  and package/runtime summary for smoke testing.

This still does not execute ARM/Thumb code. It is intentionally a measurable
loader/filesystem backend boundary first, so the next step can add API contracts
and executor experiments without disturbing the shipping vMRP runtime.

## Fifth implementation step

Added an ArkTS-side pure backend smoke probe:

- copies bundled `mrp/mythroad/dsm_gm.mrp` into a hidden
  `.pure_mrp_smoke/` work file;
- calls `probePureMrpBackend()` after the shell page loads;
- logs init/load/start status, package name, entry count, `lookfor`, read byte
  count, and list count;
- keeps the probe isolated from the visible app list and from the shipping vMRP
  runtime path.

This gives device logs a direct signal that the clean parser, filesystem, and
backend shell are alive before any executor work begins.

## Sixth implementation step

Added a self-written `pure_mrp/MythroadApi` skeleton:

- owns an isolated RGBA framebuffer for the clean backend;
- supports reset, clear, exit flag, frame snapshot, and RGB565 full-buffer dirty
  rectangle drawing;
- avoids vMRP's global C callback symbols, so it can coexist with the current
  runtime;
- is now held by `PureMrpBackend`, with frame size, frame id, draw count, and
  exit state included in `probePureMrpBackend()` summary.

This prepares the drawing/API landing zone for a future ARM/Thumb executor. The
executor still does not exist yet; this step only defines where display-side
Mythroad calls will land.

## Seventh implementation step

Added the first clean Mythroad API function table skeleton:

- defines stable pure-runtime API ids, names, and signatures for display, timer,
  filesystem, edit, audio, network, and screen-info functions;
- marks only the functions that currently have local behavior as implemented:
  `drawBitmap`, `timerStart`, `timerStop`, `getTime`, and `exit`;
- exposes API counts and the function table through `probePureMrpBackend()`;
- runs a diagnostic scalar API probe for `timerStart -> getTime -> timerStop`.

The table is intentionally descriptive for now. It is the contract the future
executor will bind to, while pointer-heavy calls such as `readFile`, edit, audio,
and network still need memory-bridge work before they can be invoked safely.

## Eighth implementation step

Added the first pure runtime memory bridge:

- `pure_mrp/PureMemory` owns a bounded virtual address space with a configurable
  base address;
- supports aligned allocation, free marking, range validation, byte read/write,
  UTF-8 string read/write, and little-endian `u32` read/write;
- `PureMrpBackend` initializes a 16 MB clean runtime memory region;
- `probePureMrpBackend()` now exposes memory stats and runs a string bridge
  smoke probe by writing and reading `start.mr`.

This is not a CPU memory map yet. It is the safety layer needed before
pointer-based Mythroad APIs such as `readFile`, edit text, audio buffers, and
network buffers can be implemented without passing raw host pointers around.

## Ninth implementation step

Implemented the first pointer-based Mythroad API path:

- `PureMrpBackend::InvokeReadFileApi(filenameAddress, lengthAddress, lookfor)`
  reads a null-terminated filename from `PureMemory`;
- loads the resource through `MythroadFs`, preserving package-first behavior;
- allocates a pure-memory data buffer for the file content;
- writes the file length back through the caller-provided `len*` address;
- returns the data buffer virtual address as the API return value;
- `probePureMrpBackend()` now includes `readFileProbe` with filename/length/data
  addresses, byte count, write-back length, and the first bytes of the result.

`lookfor` is accepted but still coarse. The next compatibility pass should make
its search behavior match Mythroad more exactly across package, disk, and system
paths.

## Tenth implementation step

Added a pure file-handle layer for common Mythroad file APIs:

- `open(filename*, mode)` creates an internal handle starting at `5`, matching
  the common Mythroad behavior where `0` means failure;
- `read(handle, buffer*, len)` and `write(handle, buffer*, len)` move bytes
  through `PureMemory`;
- `seek(handle, offset, method)` supports set/current/end methods;
- `close(handle)` flushes dirty writable buffers back through `MythroadFs`;
- `getLen(filename*)`, `remove(filename*)`, and `rename(old*, new*)` are wired to
  `MythroadFs`;
- package-backed files can be opened for read, while writable handles flush to
  disk overlays.

`probePureMrpBackend()` now includes a `fileHandleProbe` that writes `SAVE` to
`mythroad/disk/a/pure_file_probe.sav`, seeks, reads it back, closes, checks
length, renames, removes, and reports every step.

This gives the clean backend a basic save-file path before the executor exists.
The remaining filesystem work is exact mode semantics, directory creation/removal,
and findStart/findGetNext iteration behavior.

## Eleventh implementation step

Added the first pure directory API layer:

- `mkDir(path*)` and `rmDir(path*)` now route through `MythroadFs`;
- `findStart(path*, buffer*, len)` lists a logical directory and writes the
  first entry name into pure memory;
- `findGetNext(handle, buffer*, len)` advances the search handle;
- `findStop(handle)` releases the search state;
- search handles start at `1000`, keeping them separate from file handles.

`probePureMrpBackend()` now includes a `directoryProbe` that creates
`mythroad/disk/a/pure_dir_probe`, writes two files into it, verifies first/next
directory iteration, stops the search, removes both files, and removes the
directory.

This completes a basic read/write/rename/remove/list/mkdir/rmdir diagnostic
path. Remaining filesystem compatibility work is exact `lookfor` flag behavior,
wildcard/pattern matching details, and stricter Mythroad mode edge cases.

## Twelfth implementation step

Adjusted pure `_mr_readFile` `lookfor` handling to match the reference behavior
more closely:

- `lookfor == 1` now performs an existence check and returns `1` when found;
- the existence check does not allocate/read the file payload;
- the backend smoke `readFileProbe` now uses `lookfor == 0` so it continues to
  verify real payload reads separately.

`lookfor == 2` still needs a more exact decision once the executor has ownership
and free semantics, because the legacy code uses it as an internal pointer path
for RAM-backed packages.

## Thirteenth implementation step

Added pure `mr_info(filename*)` file-state semantics:

- returns `MR_IS_FILE` (`1`) for files;
- returns `MR_IS_DIR` (`2`) for directories;
- returns `MR_IS_INVALID` (`8`) for missing or unsupported paths;
- disk overlays are checked before package resources, so saved files shadow
  packaged entries;
- package entries can still report as files, and package prefixes can report as
  virtual directories for diagnostic listing behavior.

`probePureMrpBackend()` now includes `infoProbe`, which creates a temporary
directory and file, verifies file/dir/missing return values, and cleans up the
probe files. This covers the common save/config bootstrap flow where apps check
whether a directory exists before creating save data.

## Fourteenth implementation step

Added pure `mr_getScreenInfo(screeninfo*)` support:

- writes the 12-byte Mythroad `mr_screeninfo` layout into pure memory
  (`uint32 width`, `uint32 height`, `uint32 bit`);
- reports the backend virtual screen size and a 16-bit RGB565 depth;
- marks `getScreenInfo` implemented in the pure API table;
- adds `screenInfoProbe` to `probePureMrpBackend()`.

This covers another common bootstrap dependency: apps can now query screen
dimensions before choosing layout, drawing mode, or framebuffer assumptions.

## Fifteenth implementation step

Added pure `mr_getUserInfo(userinfo*)` support:

- writes the 64-byte Mythroad `mr_userinfo` layout into pure memory;
- fills stable synthetic device values instead of reading real device
  identifiers:
  - `IMEI[16]`: `860000000000000`;
  - `IMSI[16]`: `460000000000000`;
  - `manufactory[8]`: `huawei`;
  - `type[8]`: `mate80`;
  - `ver`: `101020180`;
- marks `getUserInfo` implemented in the pure API table;
- adds `userInfoProbe` to `probePureMrpBackend()`.

This gives older apps a deterministic device-info surface for initialization and
simple branch checks without exposing real IMEI/IMSI values.

## Sixteenth implementation step

Added pure `mr_getDatetime(datetime*)` support:

- writes the Mythroad `mr_datetime` layout into pure memory:
  - `uint16 year`;
  - `uint8 month`;
  - `uint8 day`;
  - `uint8 hour`;
  - `uint8 minute`;
  - `uint8 second`;
- uses the host local time so apps get a realistic current date/time;
- marks `getDatetime` implemented in the pure API table;
- adds `datetimeProbe` to `probePureMrpBackend()`.

The probe reads the structure back from pure memory and verifies the value
ranges, giving us coverage for another common startup and save-file dependency.

## Seventeenth implementation step

Added legacy Mythroad function-table slot metadata to the pure API descriptors:

- each descriptor now has a `legacyIndex`;
- mapped slots include the common bootstrap/filesystem/device APIs:
  - `drawBitmap = 30`;
  - `timerStart/timerStop/getTime/getDatetime/getUserInfo = 32..36`;
  - `open/close/info/write/read/seek/getLen/remove/rename/mkDir/rmDir/find* = 41..54`;
  - `exit = 55`;
  - `playSound/stopSound = 58..59`;
  - `editCreate/editRelease/editGetText = 76..78`;
  - `getScreenInfo = 81`;
  - network sockets begin at `82`;
- `probePureMrpBackend()` exports `legacyIndex` through `apiTable`;
- the ArkTS smoke log now reports `apiMapped`.

This is not an executor yet, but it creates the dispatch map the executor will
need when VM/MRC calls into the Mythroad porting table.

## Eighteenth implementation step

Added the first real legacy-slot dispatcher:

- `PureMrpBackend::InvokeLegacyApi(index, args)` dispatches old Mythroad table
  slots to the clean backend APIs;
- implemented dispatch paths include timer, time/date, user info, screen info,
  draw bitmap, file handles, file status, directory creation/removal, directory
  search, and exit;
- `InvokeDrawBitmapApi()` now reads RGB565 pixels from pure memory and writes
  into the pure framebuffer through `MythroadApi`;
- `probePureMrpBackend()` now includes `legacyDispatchProbe`, which calls:
  - `timerStart/getTime/timerStop` through slots `32/34/33`;
  - `getScreenInfo/getUserInfo/getDatetime` through slots `81/36/35`;
  - `drawBitmap` through slot `30`, verifying that a frame is produced.

This moves the clean runtime from "API methods exist" toward "executor can call
APIs by legacy Mythroad function-table index".

## Nineteenth implementation step

Added pure package startup/resource analysis as an executor precondition:

- `MrpPackageReader::AnalyzeStartup()` classifies package entries without
  executing them;
- detects common startup hints:
  - `start.mr`;
  - `cfunction.ext`;
  - `.mrc` extension modules;
  - `.lua` scripts;
- classifies supporting assets into image/audio/font/config/other buckets;
- derives a conservative `runtimeKind`:
  - `mrc-extension` when C extension hints or `.mrc` modules exist;
  - `mythroad-script` when `start.mr` or other `.mr` scripts exist;
  - `lua` when only Lua scripts are present;
  - `resource-only` for packages with entries but no executable hint;
- exports the result as `startupProbe` from `probePureMrpBackend()`;
- the ArkTS smoke log now prints startup kind, selected entry, MRC count, and
  basic asset count.

This still does not execute bytecode or native MRC code. It gives the upcoming
executor a deterministic package plan before loading script/module state.

## Twentieth implementation step

Converted startup analysis into a reusable startup plan:

- the backend now caches `MrpPackageStartupInfo` during `LoadPackage()`;
- the plan exposes:
  - `startFile`, matching the Mythroad distinction between `start.mr` and
    custom start files such as `cfunction.ext`;
  - `packageArgument`, the package path that would be passed as the entry
    argument;
  - `needsScriptVm`, for script/Lua execution paths;
  - `usesCFunctionBridge`, for C/MRC extension startup paths;
- `probePureMrpBackend()` now exports these plan fields;
- the smoke log reports the selected start file and whether the bridge path is
  required.

This is the boundary between package inspection and execution. The next runtime
layer can consume a stable plan instead of re-deriving startup behavior from raw
file names.

## Twenty-first implementation step

Added the first clean-room pure executor shell:

- new `PureExecutor` owns execution state independently from package parsing and
  API probes;
- accepts the cached startup plan and rejects packages with no executable entry;
- tracks planned/running/paused/stopped/unsupported/failed states;
- exposes whether the current app is waiting for:
  - the script VM implementation;
  - the C/MRC function bridge implementation;
- backend lifecycle now drives the executor:
  - `LoadPackage()` configures it;
  - `Start()` enters running state;
  - `Pause()` and `Resume()` update executor state;
  - `Release()` stops it;
- `probePureMrpBackend()` exports `executorProbe`;
- the ArkTS smoke log reports executor state and capability waits.

This is intentionally not a fake interpreter. It gives the runtime a concrete
execution state machine, so future VM/MRC work can plug into a stable lifecycle
instead of living in `Start()` as one-off logic.

## Twenty-second implementation step

Added startup file preload into the pure executor path:

- after package inspection and filesystem overlay setup, `LoadPackage()` reads
  the planned `startFile`;
- the executor records whether the start file was loaded, whether it came from
  the package layer, and how many bytes were available;
- `Start()` now refuses to enter running state if the startup plan names a file
  but that file was not loaded;
- `executorProbe` exports `startFileLoaded`, `startFileFromPackage`, and
  `startFileBytes`;
- the smoke log reports loaded startup bytes.

This means the pure path now reaches the point where an interpreter or MRC
loader has concrete input bytes, instead of only a filename.

## Twenty-third implementation step

Added a larger execution-framework batch instead of another tiny probe:

- introduced `PureProgramAnalyzer`:
  - classifies loaded entry/module bytes as `text-script`, `lua-bytecode`,
    `mrc-binary`, `native-binary`, `unknown-binary`, or `empty`;
  - records FNV-1a checksum, byte count, line count, printable-byte count, and
    basic binary markers;
- expanded `PureExecutor` from a lifecycle shell into a runtime staging layer:
  - analyzes the loaded start file;
  - preloads every `.mrc` module listed in the startup plan;
  - records module count, total bytes, and aggregate checksum;
  - tracks VM state separately from backend state;
  - owns a lifecycle event queue;
  - dispatches `init`, `pause`, `resume`, and `exit` through the queue;
  - reports a truthful `blockedReason` instead of pretending script/MRC
    execution is complete;
- `LoadPackage()` now reads and analyzes the start file, then preloads MRC
  modules before marking the package loaded;
- `probePureMrpBackend()` now runs a pause/resume lifecycle probe and exports:
  - entry format/checksum/line count;
  - module preload count/bytes/checksum;
  - event queue and dispatched event counters;
  - lifecycle pause/resume statuses;
  - the exact execution blocker (`Mythroad script VM is not implemented` or
    `C/MRC bridge is not implemented`);
- the ArkTS smoke log includes entry/module/event/lifecycle/blocker details.

This still does not complete full Mythroad compatibility. What it does complete
is the clean execution staging surface: packages now move from parsed metadata
to analyzed entry bytes, preloaded modules, lifecycle events, and a precise
capability blocker that the real VM/MRC implementation can replace.

## Twenty-fourth implementation step

Added the first dedicated pure script VM layer:

- new `PureScriptVm` owns script-side state separately from the generic
  executor;
- registers the Mythroad API table into VM metadata:
  - total API function count;
  - implemented API count;
  - legacy-table mapped function count;
  - synthetic global count;
- loads the analyzed entry chunk and records:
  - chunk format;
  - chunk bytes;
  - checksum;
  - lifecycle handler hints for text-like chunks;
- receives executor lifecycle events (`init`, `pause`, `resume`, `exit`);
- records queued, blocked, and dispatched event counters;
- reports the exact VM blocker:
  `Mythroad bytecode interpreter is not implemented`;
- `PureExecutorStatus` now exposes script VM state, API counts, globals, and
  blocked event count;
- NAPI, ArkTS declarations, and smoke logs now include script VM details.

This is the VM shell the real interpreter can attach to. It deliberately stops
before executing bytecode, but the state model, global/API registration, entry
load path, and lifecycle event path now exist as production code rather than
loose probes.

## Twenty-fifth implementation step

Added the first pure VM core data layer:

- new `PureScriptCore` provides reusable VM structures:
  - script constants;
  - prototype metadata;
  - chunk metadata;
  - stack frame metadata;
- added safe chunk parsing:
  - text chunks are scanned for quoted string constants and mapped to a top
    prototype summary;
  - Lua-style bytecode chunks parse the header, endian/sizing fields, source
    name, line range, stack size, instruction count, constant table entries
    (`nil`, `boolean`, `number`, `string`), and child prototype count;
  - diagnostic parsing is bounded and reports partial/truncated states instead
    of reading past the buffer;
- `PureScriptVm::LoadEntry()` now receives the real entry bytes, parses them,
  and stores the resulting chunk/prototype summary;
- VM setup now creates a global table model:
  - base globals such as `_VERSION`, `vmver`, `system`, lifecycle handler names;
  - every Mythroad API descriptor name;
  - global read/write counters;
- loading a script creates an initial `main` stack frame with a max stack size
  from the chunk prototype when available;
- event dispatch now performs global handler lookup accounting before recording
  the event as blocked;
- `executorProbe` now exposes:
  - prototype count;
  - instruction count;
  - constant and string constant counts;
  - child prototype count;
  - VM max stack size;
  - stack frame depth and slot count;
  - global read/write counts.

This is still pre-opcode. The important shift is that the VM is no longer just
"entry bytes plus a blocker": it now has parsed chunk structure, constants,
prototype metadata, a main call frame, and a seeded global/API namespace.

## Twenty-sixth implementation step

Added the first opcode decoding layer:

- added `PureScriptInstructionInfo` for decoded instruction metadata:
  - raw 32-bit instruction;
  - `pc`;
  - `opcode`;
  - `A/B/C`;
  - `Bx`;
  - `sBx`;
  - mnemonic name;
- the Lua-style binary chunk parser now preserves the top prototype instruction
  stream instead of only skipping it;
- decoded instructions use the common Lua 5.1/Mythroad-style 6-bit opcode,
  8-bit `A`, 9-bit `B/C`, and 18-bit `Bx` layout;
- the parser records:
  - decoded instruction count;
  - unknown opcode count;
  - whether a `RETURN` opcode exists;
  - first and last opcode names;
- `PureScriptVmStatus` and `executorProbe` now expose opcode-level counters;
- ArkTS smoke logs include `scriptOpcode=decoded/unknown/hasReturn`.

This still does not execute opcodes. It creates the decoder boundary required
for the next step: implementing a small interpreter loop for safe opcodes such
as `MOVE`, `LOADK`, `LOADBOOL`, `LOADNIL`, global access, `CALL`, and `RETURN`.

## Twenty-seventh implementation step

Added the first safe opcode execution loop:

- added a minimal VM value model for `nil`, `boolean`, `number`, `string`, and
  global references;
- added register storage sized from the top prototype/main frame stack size;
- implemented safe opcode handling for:
  - `MOVE`;
  - `LOADK`;
  - `LOADBOOL` without jump-skip;
  - `LOADNIL`;
  - `GETGLOBAL`;
  - `SETGLOBAL`;
  - `RETURN`;
- unsupported or control-flow-sensitive opcodes stop the interpreter and record
  the exact blocker, e.g. `CALL`, `ADD`, or `LOADBOOL:pc-skip`;
- the VM now records:
  - executed instruction count;
  - supported instruction count;
  - unsupported instruction count;
  - register count/write count;
  - load constant count;
  - move count;
  - global opcode count;
  - whether `RETURN` was actually reached;
  - unsupported opcode name;
- if a safe prefix reaches `RETURN` with no unsupported instruction, the VM
  clears the bytecode blocker and marks itself executable for that chunk.

This is the first real interpreter behavior in the clean runtime. It is still
deliberately narrow: calls, arithmetic, tables, closures, jumps, loops, and
varargs are not executed yet.

## Twenty-eighth implementation step

Added the first control-flow opcode execution:

- changed the interpreter from a linear scan to a `pc`-driven loop;
- added a step limit to prevent accidental infinite loops;
- added RK operand handling for compare/global-related instructions:
  - `< 256` reads a register;
  - `>= 256` reads a constant table entry;
- implemented:
  - `JMP`;
  - `EQ`;
  - `LT`;
  - `LE`;
  - `TEST`;
  - `TESTSET`;
  - `LOADBOOL` with skip semantics;
- added truthiness and basic equality/ordering helpers for nil, booleans,
  numbers, strings, and global references;
- added interpreter counters for:
  - jumps;
  - comparisons;
  - tests;
  - branch skips;
- exported the counters through `executorProbe` and ArkTS smoke logging.

The VM can now run simple conditional bytecode prefixes. Arithmetic, table
access, function calls, closures, loops, and varargs still stop at a precise
unsupported opcode.

## Twenty-ninth implementation step

Expanded the clean script VM beyond control flow into the data opcodes that
real Mythroad scripts commonly hit immediately after startup:

- raised the diagnostic constant table retention from 32 entries to 512 entries,
  so larger startup chunks can resolve more `LOADK` and RK operands before
  stopping;
- added numeric conversion for Lua-style arithmetic operands;
- implemented arithmetic opcodes:
  - `ADD`;
  - `SUB`;
  - `MUL`;
  - `DIV`;
  - `MOD`;
  - `POW`;
- implemented unary/data opcodes:
  - `UNM`;
  - `NOT`;
  - `LEN`;
  - `CONCAT`;
- added a lightweight table model and implemented:
  - `NEWTABLE`;
  - `GETTABLE`;
  - `SETTABLE`;
  - `SELF`;
  - `SETLIST` for fixed-size flushes;
- global table reads such as `mr.drawBitmap` now resolve into conservative
  global function references, allowing bridge-shaped calls to be identified;
- added a guarded `CALL`/`TAILCALL` placeholder for global/API references:
  it records the call and writes fixed-count nil return slots, but it does not
  execute product APIs yet;
- exported new interpreter counters:
  - arithmetic ops;
  - unary ops;
  - concatenations;
  - table ops;
  - global/API call placeholders.

This moves the VM from "safe prefix plus branches" to a usable bootstrap
interpreter skeleton. The remaining major blockers are still real call
semantics, closures/upvalues, loops/varargs, coroutine-like script callbacks,
and mapping bytecode calls into the Mythroad API bridge.

## Thirtieth implementation step

Connected script `CALL` execution to the existing pure backend API bridge:

- `PureExecutor` can now inject a narrow API invoker callback into
  `PureScriptVm`;
- `PureMrpBackend` wires that callback to the existing `InvokeApi` path, keeping
  the clean VM isolated from backend internals;
- the VM now stores API descriptors from `MythroadApi::FunctionTable`;
- global function names are normalized, so both direct calls such as
  `timerStart()` and table-shaped references such as `mr.timerStart()` can map
  to the same descriptor;
- `CALL`/`TAILCALL` now:
  - reads fixed-count Lua call arguments from registers;
  - accepts scalar nil/boolean/number arguments;
  - invokes implemented pure APIs through the backend callback;
  - writes fixed-count numeric return values into registers;
  - records precise blockers for dynamic arguments, dynamic returns,
    non-scalar arguments, unknown globals, and API failures;
- exported counters for:
  - API calls reached from bytecode;
  - API call failures.

This is still not full Lua call semantics. It does not yet handle closures,
vararg calls, dynamic return counts, string/userdata arguments, or real script
function frames. It does, however, move simple bytecode startup paths from
"CALL is unsupported" to "CALL can execute implemented scalar Mythroad APIs".

## Thirty-first implementation step

Added the first closure/upvalue/loop execution skeleton:

- added VM value support for closure references;
- added a lightweight closure store that records the referenced child prototype
  index and captures the current upvalue snapshot;
- added top-level upvalue storage sized from the parsed prototype metadata;
- implemented:
  - `GETUPVAL`;
  - `SETUPVAL`;
  - `CLOSURE`;
  - `CLOSE` as a safe no-op boundary;
  - `VARARG` for fixed-count nil-filled vararg reads;
  - numeric `FORPREP`;
  - numeric `FORLOOP`;
- numeric loops now adjust the control register, expose the visible loop index,
  and jump according to Lua 5.1-style step/limit semantics;
- exported counters for:
  - closures/close boundaries;
  - upvalue reads/writes;
  - numeric loops;
  - fixed vararg reads.

This still does not execute nested Lua function bodies. `CLOSURE` creates a
callable-shaped value, but calling a script closure still needs real frame
creation, parameter passing, return propagation, and child prototype parsing.
The gain is that many startup chunks can now pass through common function
declaration, fixed vararg, and numeric loop setup opcodes instead of stopping
immediately.

## Thirty-second implementation step

Added recursive Lua prototype parsing:

- replaced the top-only binary chunk parser with a reusable prototype parser;
- parsed and retained child prototype metadata, constants, raw instructions,
  decoded instructions, opcode names, return-opcode markers, and literal string
  counts;
- fixed constant-table consumption so chunks with more constants than the
  diagnostic retention limit still advance the reader correctly;
- skipped Lua 5.1 debug sections after each prototype:
  - line info;
  - local variable records;
  - upvalue names;
- added limits for stored prototypes and nesting depth to avoid runaway parser
  behavior on damaged chunks;
- text chunks now also populate the unified function list with a synthetic top
  function;
- `CLOSURE` now resolves `Bx` to an actual parsed function slot instead of only
  recording a raw prototype index.

This is the parser side required for real script closure execution. The VM can
now create closure values backed by parsed child function metadata. The next
step is to execute those child function instruction streams with a proper call
frame instead of treating closures as non-global call blockers.

## Thirty-third implementation step

Added the first real script closure call frame:

- split top-level execution into a reusable `ExecuteFunction` path;
- added an active-function context so RK constants, `LOADK`, `GETGLOBAL`, and
  `SETGLOBAL` read from the currently executing prototype instead of always
  reading the top-level constant table;
- `CALL`/`TAILCALL` now dispatches both:
  - global/API references through the backend API invoker;
  - closure references through a script function frame;
- closure calls now:
  - collect fixed-count arguments from caller registers;
  - switch to the child prototype instruction stream;
  - create a temporary register file sized by the child prototype max stack;
  - restore caller registers and upvalues after return;
  - write fixed-count return values back into the caller registers;
  - enforce a call-depth limit;
- `RETURN` now exports fixed-count return values from the active function;
- exported counters for:
  - closure calls;
  - frame entries;
  - max observed call depth.

This is still a conservative call model. It does not yet implement Lua's open
return counts, dynamic argument forwarding, full upvalue binding instructions
following `CLOSURE`, or child-prototype local debug naming. But closure values
can now enter parsed child bytecode instead of immediately failing as
non-global calls.

## Thirty-fourth implementation step

Implemented Lua 5.1 closure upvalue binding:

- `CLOSURE` now reads the child prototype's declared upvalue count;
- the VM consumes the binding pseudo-instructions that immediately follow
  `CLOSURE`;
- supported binding sources:
  - `MOVE` captures a value from the current register file;
  - `GETUPVAL` captures a value from the current closure's upvalue array;
- captured values are stored directly on the created closure;
- the interpreter advances `pc` past the binding pseudo-instructions so they
  are not executed as normal code;
- added precise blockers for missing or invalid binding instructions;
- exported `scriptUpvalueBindCount` through the executor probe and smoke log.

This makes nested closures materially more correct: closures no longer capture
a whole ambient upvalue snapshot by accident, and child functions receive the
specific values requested by the bytecode. The remaining gap is live upvalue
reference semantics: captured values are still snapshots, not shared mutable
cells.

## Thirty-fifth implementation step

Added conservative open call/open return support:

- `CALL` and `TAILCALL` no longer fail immediately when `B == 0`;
- dynamic call arguments are collected from the caller register file and trailing
  nil values are trimmed;
- global/API calls can now accept dynamic scalar arguments;
- closure calls can now accept dynamic arguments;
- `CALL` no longer fails immediately when `C == 0`;
- open returns write the actual returned values back to the caller register
  window;
- `RETURN B == 0` now exports a dynamic return list, trimming trailing nil
  values;
- `TAILCALL C == 0` now exports dynamic return values instead of forcing an
  empty result;
- exported counters for:
  - open-argument calls;
  - open-return writes.

This is intentionally conservative because the VM does not yet model Lua's
exact stack top. It is good enough to keep many simple dynamic call chains
moving, but complex multi-return forwarding still needs a real stack-top model.

## Thirty-sixth implementation step

Added first-pass generic table iteration support for the clean script VM:

- seeded Lua-style helper globals:
  - `next`;
  - `pairs`;
  - `ipairs`;
  - `type`;
  - `tonumber`;
  - `tostring`;
  - `print`;
- implemented VM-owned builtin dispatch before falling back to Mythroad API
  function lookup;
- `pairs(table)` now returns `next`, the table, and a nil control key;
- `ipairs(table)` now returns an internal `ipairs_iter`, the table, and a zero
  numeric control key;
- `next(table, key)` walks VM table fields in a stable sorted order and returns
  the next key/value pair;
- `ipairs_iter(table, index)` walks numeric array-style entries until it finds a
  nil value;
- implemented `TFORLOOP` using Lua 5.1 register conventions:
  - iterator function in `A`;
  - invariant state in `A + 1`;
  - control variable in `A + 2`;
  - iterator results written to `A + 3...`;
  - nil first result skips the following loop jump;
  - non-nil first result updates the control variable;
- closure iterators are also accepted through the existing closure call path;
- exported counters for iterator calls and generic-loop executions through the
  executor probe and smoke log.

This is enough for common table-driven script initialization patterns such as
`for k, v in pairs(t)` and array walks through `ipairs(t)`. The remaining gap is
full Lua stack-top fidelity for exotic iterator/multireturn combinations.

## Thirty-seventh implementation step

Added persistent global-value semantics to the clean script VM:

- introduced a VM-owned global value table separate from the global-name index;
- `SETGLOBAL` now stores the actual register value instead of only recording the
  global name;
- `GETGLOBAL` now prefers stored script values and only falls back to a global
  function placeholder when no value has been assigned;
- script-created global closures now survive after the top-level chunk returns;
- script-created global tables and scalar values are now visible to later code;
- global table-style writes such as `mr.foo = value` now preserve the assigned
  value, while unknown API/module reads still fall back to the previous
  `mr.foo` placeholder behavior;
- lifecycle event dispatch now calls global closure handlers such as `init`,
  `pause`, `resume`, and `exit` when the script defines them.

This removes a major Lua compatibility hole: bytecode generated from
`function name(...) ... end` or `name = function(...) ... end` now creates a
callable global function instead of losing the closure at `SETGLOBAL` time.
The remaining gap is a fuller model of `_G`, environments, and metatables.

## Thirty-eighth implementation step

Added first-pass global environment support to the clean script VM:

- seeded `_G` as a VM-owned global-environment proxy value;
- `_G.name` and `_G["name"]` now read from the same global value table used by
  `GETGLOBAL`;
- `_G.name = value` and `_G["name"] = value` now write to the same global value
  table used by `SETGLOBAL`;
- `getfenv()` returns the global environment proxy;
- `setfenv(fn, env)` is accepted conservatively and returns the function value,
  without changing closure environments yet;
- `rawget(table, key)` and `rawset(table, key, value)` work for normal VM tables
  and `_G`;
- `pairs(_G)` and `next(_G, key)` can iterate over the current global names in
  stable sorted order;
- `type(_G)` reports `table`, and `#_G` reports the known global-name count;
- exported `scriptEnvironmentAccessCount` through the executor probe and smoke
  log.

This makes indirect global access patterns substantially more compatible,
including scripts that store lifecycle handlers or API aliases through `_G`.
The remaining gap is per-closure environment replacement and metatable-aware
lookup.

## Thirty-ninth implementation step

Added the first compatibility slice of Lua-style standard libraries to the
clean script VM:

- seeded module-style globals for `table`, `string`, and `math`;
- table helpers:
  - `table.insert`;
  - `table.remove`;
  - `table.getn`;
  - `table.concat`;
- string helpers:
  - `string.len`;
  - `string.sub`;
  - `string.byte`;
  - `string.char`;
  - `string.lower`;
  - `string.upper`;
- math helpers:
  - `math.floor`;
  - `math.ceil`;
  - `math.abs`;
  - `math.max`;
  - `math.min`;
  - `math.mod`;
  - conservative deterministic `math.random`;
- nil assignment to VM tables now removes table fields, which makes
  `table.remove` and array-length probing behave closer to Lua;
- exported `scriptStandardLibraryCallCount` through the executor probe and
  smoke log.

The goal is not a complete Lua standard library yet. This slice targets common
MRP script initialization work: building resource tables, joining path strings,
normalizing names, and doing simple numeric bounds.

## Fortieth implementation step

Added protected-call and additional string utility support to the clean script
VM:

- seeded and implemented:
  - `assert`;
  - `error`;
  - `pcall`;
- `pcall(fn, ...)` can invoke VM closures, VM builtins, and implemented scalar
  Mythroad API functions;
- protected-call failures return `false, message` instead of immediately
  poisoning the outer interpreter status;
- added `string.find` using plain substring matching and Lua-style 1-based
  return indexes;
- added conservative `string.format` support for common `%s`, `%d`, `%i`,
  `%f`, `%g`, and `%%` cases;
- exported `scriptProtectedCallCount` through the executor probe and smoke log.

This improves compatibility with defensive script loaders that probe optional
features through `pcall` before deciding which code path to use. Pattern syntax
for `string.find`, full `string.format` flags, and traceback-style error
metadata remain future work.

## Forty-first implementation step

Added the first module-loading path for clean script execution:

- the executor now registers attached script modules with the script VM;
- the backend now attaches package `.mr` and `.lua` script files besides the
  start script, so they are visible to the VM module loader;
- module names are normalized case-insensitively with slash separators;
- `require(name)` can find a registered module, parse it on demand, execute its
  top-level function, cache its return values, and return cached values on later
  calls;
- `dofile(name)` executes a registered module without using the require cache;
- `loadfile(name)` returns a callable module closure, or `nil, message` when the
  module cannot be found/parsed;
- module prototypes are appended to the VM's shared function table with a
  per-module prototype base so nested closures inside modules keep resolving the
  correct child prototypes;
- exported module counters:
  - registered script modules;
  - module load executions;
  - require cache hits.

This is intentionally still a registered-module loader, not an arbitrary
filesystem loader. The next compatibility step is to connect `loadfile/dofile`
to Mythroad filesystem reads when a script asks for a path that was not
pre-registered at package-load time.

## Forty-second implementation step

Connected script module loading to the Mythroad virtual filesystem:

- added a module resolver callback from `PureScriptVm` through `PureExecutor`
  to `PureMrpBackend`;
- when `require`, `loadfile`, or `dofile` cannot find a pre-registered module,
  the VM now asks the backend to read the requested path through `MythroadFs`;
- dynamically resolved modules are analyzed, registered, parsed on demand, and
  then follow the same prototype-base execution path as pre-registered modules;
- dynamically resolved modules participate in the `require` cache after the
  first successful execution;
- exported `scriptModuleFsLoadCount` so the smoke log can distinguish:
  - pre-registered modules;
  - executed module loads;
  - require cache hits;
  - filesystem-backed dynamic module loads.

This moves `loadfile/dofile` from a static package-index feature toward real
Mythroad script behavior. Remaining gaps are Lua source parsing for text scripts
and richer path search rules such as package-specific `?.lua`/`?.mr` patterns.

## Forty-third implementation step

Added a conservative text-script function registration path:

- `.mr`/`.lua` text chunks are now scanned for top-level
  `function name(...)` and `name = function(...)` declarations;
- `local function name(...)` is intentionally ignored because it should not
  publish a global handler, and local function assignments are skipped for the
  same reason;
- the text parser now synthesizes a top-level instruction stream that creates
  closures for discovered functions, stores them with `SETGLOBAL`, and then
  returns cleanly;
- each discovered text function receives a placeholder no-op prototype with a
  valid `RETURN`, so lifecycle dispatch can safely call handlers discovered in
  source files;
- this applies to both entry scripts and modules loaded through
  `require`/`loadfile`/`dofile`, including modules resolved through
  `MythroadFs`.

This is not a full Lua source parser yet. Function bodies are still placeholders,
but text-script packages can now expose lifecycle and module entry points to the
same global/closure machinery used by binary chunks.

## Forty-fourth implementation step

Improved clean script module search semantics:

- `require`, `loadfile`, and `dofile` now share the same module resolution
  path;
- module lookup builds normalized candidates for:
  - the requested name;
  - dot-to-slash package names, such as `foo.bar` -> `foo/bar`;
  - extensionless names;
  - `.lua` and `.mr` suffixes;
  - package-style `name/init.lua` and `name/init.mr`;
- the same candidate list is used against already registered package scripts
  and against dynamic `MythroadFs` reads;
- `loadfile` now participates in filesystem-backed dynamic module loading
  instead of only searching pre-registered modules.

This makes the clean runtime less dependent on the exact spelling used inside
an MRP script package. It still does not implement full Lua `package.path`, but
it now covers the common compact Mythroad layout cases without adding a global
package subsystem too early.

## Forty-fifth implementation step

Added a minimal Lua `package` compatibility surface:

- seeded a global `package` table;
- seeded `package.path` with the clean runtime's supported search patterns:
  `?.lua;?.mr;?/init.lua;?/init.mr`;
- seeded `package.loaded` as a real VM table;
- `require(name)` now checks `package.loaded` before searching package files;
- successful cached `require` loads publish their first return value, or `true`
  when no value is returned, into `package.loaded` under the requested name and
  normalized module aliases;
- legacy/global-style reads of `package.path` and `package.loaded` are also
  seeded for bytecode that treats dotted names as globals.

This covers common scripts that probe `package.path`, prefill
`package.loaded`, or expect `require` to leave a loaded-module marker. Full Lua
package searcher hooks and custom path parsing remain future work.

## Forty-sixth implementation step

Made the minimal package layer active instead of only descriptive:

- module candidate generation now reads the current `package.path`;
- semicolon-separated path patterns with `?` are expanded for both direct names
  and dot-to-slash names;
- writes to `package.path` through the package table are preferred over the
  seeded dotted global fallback;
- `require`, `loadfile`, and `dofile` all use the active package path when
  searching registered modules and filesystem-backed modules;
- added conservative `module(name, ...)` support that creates or reuses a
  module table, records `_NAME`, publishes it to `package.loaded`, and exposes
  it as a global;
- added `package.seeall` as a no-op compatibility helper so Lua 5.1-era module
  declarations can pass startup probes.

This still does not replace the current function environment like stock Lua
`module` does. It is a compatibility shim to keep older script loaders moving
until full environment semantics are justified by real packages.

## Forty-seventh implementation step

Added another compatibility slice for common Lua 5.1 startup helpers:

- seeded and implemented `select`;
- seeded and implemented global `unpack` and `table.unpack`;
- seeded and implemented conservative `setmetatable` / `getmetatable` storage
  for VM tables;
- VM tables now retain a lightweight metatable reference;
- seeded and implemented `string.match` with plain substring behavior;
- seeded and implemented `string.gsub` with plain substring replacement and an
  optional replacement limit;
- these helpers are intentionally conservative: metatables are stored and
  returned, but `__index`, `__call`, arithmetic metamethods, and full Lua string
  pattern syntax are not executed yet.

This removes several early blockers seen in Lua 5.1-style loaders that probe
varargs, unpack config tables, attach metatables during object construction, or
do simple string normalization before reaching Mythroad API calls.

## Forty-eighth implementation step

Made lightweight metatables useful for table lookup and added another small
standard-library slice:

- table reads now honor a metatable `__index` table with a bounded lookup depth
  to avoid cyclic fallback loops;
- global-style `__index` fallback is supported conservatively for dotted global
  tables;
- added `table.pack` with an `n` field;
- added `string.rep`;
- added `string.reverse`;
- added `math.sqrt`;
- added `math.sin`;
- added `math.cos`;
- added `math.tan`.

This still does not execute function-valued `__index` metamethods or other
metamethod families. The intent is to support common prototype-table patterns
used during Lua startup without turning the interpreter into a full metatable
engine prematurely.

## Forty-ninth implementation step

Extended the lightweight metatable and environment-probe surface:

- table reads now support function-valued `__index` when the value is a VM
  closure or known global callable;
- table values can be called through a metatable `__call` closure/global
  callback;
- added `collectgarbage` as a conservative no-op/count probe;
- added `math.randomseed` as a deterministic no-op companion to
  `math.random`;
- seeded an `os` global surface;
- added `os.time`, including table-to-time conversion for common fields;
- added `os.date`, including `*t` table output and basic `strftime` formatting;
- added `os.difftime`.

This moves metatable support from simple storage into common object/factory
patterns while still avoiding the full Lua metamethod matrix. The date/time
helpers are intentionally host-backed and coarse, aimed at scripts that probe or
format timestamps during startup rather than timing-sensitive game logic.

## Fiftieth implementation step

Filled in more metatable behavior used by Lua object helpers:

- table writes now honor `__newindex` when assigning a missing key;
- `__newindex` can point to another table or to a callable VM closure/global;
- the `LEN` opcode now routes table length through a helper that can consult
  callable `__len`;
- table length now falls back to Lua-style contiguous array length instead of
  raw field-count size;
- added more math helpers:
  - `math.log`;
  - `math.exp`;
  - `math.deg`;
  - `math.rad`;
  - `math.atan`.

This makes common class-style tables behave closer to Lua while still keeping
metamethod execution bounded. Arithmetic and comparison metamethods remain
future work because they touch core opcode semantics more broadly.

## Fifty-first implementation step

Added bounded arithmetic and comparison metamethod dispatch:

- arithmetic opcodes now fall back to callable metatable methods when numeric
  coercion fails:
  - `__add`;
  - `__sub`;
  - `__mul`;
  - `__div`;
  - `__mod`;
  - `__pow`;
- unary minus now falls back to `__unm`;
- table equality can consult `__eq` when table identities differ;
- less-than can consult `__lt`;
- less-or-equal can consult `__le`, then conservatively fall back through
  reversed `__lt`;
- metamethod calls reuse the VM's existing closure/global invocation path and
  call-depth guard.

This fills in the most common object/value-wrapper behavior without adding a
separate metamethod execution engine. String and numeric comparisons still use
their direct fast paths first, matching the conservative style used elsewhere in
the clean VM.

## Fifty-second implementation step

Rounded out additional callable/metatable compatibility:

- `tostring(value)` now honors callable `__tostring` on table metatables;
- the `CONCAT` opcode now uses `__concat` when primitive string/number/boolean
  concatenation cannot handle the operands;
- the `CALL` / `TAILCALL` opcode path now uses the unified function invocation
  helper, so table values with `__call` can be called by real bytecode, not only
  by internal helper paths;
- added `xpcall` with conservative error-handler invocation;
- added `loadstring` as a safe placeholder that returns a no-op callable marker
  instead of failing early;
- added the internal `loadstring_result` callable used by the placeholder.

`loadstring` still does not parse and execute arbitrary Lua source. It exists
to keep compatibility probes and optional dynamic-loader paths from aborting
startup while the clean runtime remains focused on package-provided bytecode and
registered text chunks.

## Fifty-third implementation step

Added conservative coroutine/debug compatibility:

- seeded a `coroutine` table with:
  - `create`;
  - `resume`;
  - `status`;
  - `yield`;
  - `wrap`;
  - `running`;
- coroutine values are now represented as a lightweight VM value type;
- `coroutine.resume` synchronously invokes the stored function and marks the
  coroutine dead after completion;
- `coroutine.wrap` returns a callable wrapper value that follows the same
  synchronous execution model;
- `coroutine.status` reports `suspended` or `dead`;
- `coroutine.yield` currently returns its arguments without real suspension;
- seeded a `debug` table with conservative `traceback` and `getinfo` helpers;
- `type()` now reports coroutine values as `thread` and coroutine wrappers as
  `function`.

This is not a full cooperative scheduler. It is enough for common startup
probes, simple one-shot coroutine wrappers, and debug-library feature checks
while avoiding deep control-flow changes to the interpreter.

## Fifty-fourth implementation step

Tightened raw table semantics and added small standard-library details:

- `rawget` now bypasses `__index` instead of using the normal table read path;
- `rawset` now bypasses `__newindex` instead of using the normal table write
  path;
- added `rawequal` for identity/primitive comparison without metamethods;
- added `table.sort` using raw array access and the VM's comparison helper;
- seeded `math.pi`;
- seeded `math.huge`;
- added `os.clock`.

This fixes a subtle regression introduced by adding metatables: raw operations
must remain raw. The additional library helpers cover common compatibility
checks and simple table ordering used by loaders/config code.

## Fifty-fifth implementation step

Extended the clean-room compatibility layer for common loader/runtime helpers:

- added `table.foreach` and `table.foreachi` with conservative callback-stop
  behavior;
- added `string.gmatch` as a lightweight plain-substring iterator value;
- added `string.dump` placeholder output for compatibility probes;
- added `math.pow`, `math.frexp`, and `math.ldexp`;
- added `os.getenv` and `os.tmpname` conservative stubs;
- taught iterator dispatch and stringification about the new internal iterator
  value kind.

This step intentionally keeps pattern semantics simple: `gmatch` currently uses
plain substring scanning rather than full Lua pattern parsing. That is enough to
let many scripts continue through optional helper paths while the VM stays
clean, predictable, and easy to evolve.
