# mrpoid2018 HarmonyOS port notes

## Current status

- Native source snapshot copied from `android-mrpoid2018/lib/src/main/jni/src`.
- Copied file count: 168.
- Prebuilt `mr_pre/*.a` libraries were deliberately excluded.
- This code is not part of the production CMake target.

## Main blockers

1. License
   - The repo has a README that says open source, but there is no clear license
     file for the native runtime.
   - Some Java/XML files mention Apache 2.0, but that does not automatically
     cover the C/JNI/MR VM code.
   - The prebuilt VM libraries are not copied and should not be shipped without
     explicit authorization.

2. Android/JNI coupling
   - `emulator.c`, `register_natives.c`, `vm.c`, and helper files depend on
     `JNIEnv`, `jobject`, Android Bitmap, and `__android_log_print`.
   - These calls need to be replaced with the existing ArkTS/NAPI bridge:
     framebuffer, edit request, audio bridge, timer, and input dispatcher.

3. 64-bit hazards
   - The code contains many `uint32` pointer casts and 32-bit address
     assumptions.
   - ARM-specific assembly/cache flush paths must be removed or replaced.
   - The port should first compile with warnings promoted for pointer truncation
     before it is allowed into the main runtime.

4. Platform semantics
   - File operations, path lookup, edit text encoding, network, audio loop/stop,
     timers, and input repeat behavior need to be mapped to the behavior already
     fixed in the current HarmonyOS runtime.

## Suggested port order

1. Build a small static library from the platform-independent MR/MRP parser and
   draw helpers only.
2. Replace Android logging and JNI types with a tiny HarmonyOS platform shim.
3. Port memory allocation and framebuffer output.
4. Port file open/read/write/rename/remove/find semantics.
5. Port input/timer lifecycle.
6. Port edit box, GBK/Unicode conversion, audio, and networking.
7. Wire `Mrpoid2018Backend` behind a debug-only runtime selector.
8. Run the same QQ/game compatibility cases used for the current runtime.

## Do not do yet

- Do not replace the current runtime.
- Do not add copied mrp2018 sources to `entry/src/main/cpp/CMakeLists.txt`.
- Do not ship this code in a commercial closed-source build before licensing is
  confirmed.
