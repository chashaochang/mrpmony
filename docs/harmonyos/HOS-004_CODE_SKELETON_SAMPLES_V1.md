# HOS-004 MRP HarmonyOS POC 第一周代码骨架样例（ArkTS + NAPI/C++）

## 1. ArkTS：`MrpNativeBridge.ets`

```ts
// entry/src/main/ets/features/mrp/infra/native/MrpNativeBridge.ets

import nativeMrp from 'libmrp_napi.so'
import type {
  InitOptions,
  CommonResult,
  InputEvent,
  FrameResult,
} from '../../model/SimulatorTypes'

export class MrpNativeBridge {
  static init(options: InitOptions): CommonResult {
    return nativeMrp.init(options) as CommonResult
  }

  static loadPackage(packagePath: string): CommonResult {
    return nativeMrp.loadPackage(packagePath) as CommonResult
  }

  static start(): CommonResult {
    return nativeMrp.start() as CommonResult
  }

  static pause(): CommonResult {
    return nativeMrp.pause() as CommonResult
  }

  static resume(): CommonResult {
    return nativeMrp.resume() as CommonResult
  }

  static sendInput(event: InputEvent): CommonResult {
    return nativeMrp.sendInput(event) as CommonResult
  }

  static pullFrame(): FrameResult {
    return nativeMrp.pullFrame() as FrameResult
  }

  static release(): CommonResult {
    return nativeMrp.release() as CommonResult
  }
}
```

---

## 2. ArkTS：`SimulatorTypes.ets`

```ts
// entry/src/main/ets/features/mrp/model/SimulatorTypes.ets

export interface InitOptions {
  workDir: string
  width: number
  height: number
  debug?: boolean
}

export interface CommonResult {
  ok: boolean
  errorCode?: number
  errorMessage?: string
}

export type InputType = 'key' | 'touch'
export type InputAction = 'down' | 'up' | 'move'

export interface InputEvent {
  type: InputType
  action: InputAction
  keyCode?: number
  x?: number
  y?: number
  timestamp?: number
}

export interface FrameResult {
  ok: boolean
  hasNewFrame: boolean
  width: number
  height: number
  pixelFormat: 'RGBA_8888'
  buffer?: ArrayBuffer
  frameId?: number
  errorCode?: number
  errorMessage?: string
}
```

---

## 3. ArkTS：`MrpSimulatorFacade.ets`

```ts
// entry/src/main/ets/features/mrp/facade/MrpSimulatorFacade.ets

import { MrpNativeBridge } from '../infra/native/MrpNativeBridge'
import type { InitOptions, InputEvent, FrameResult, CommonResult } from '../model/SimulatorTypes'

export class MrpSimulatorFacade {
  init(options: InitOptions): CommonResult {
    return MrpNativeBridge.init(options)
  }

  loadPackage(packagePath: string): CommonResult {
    return MrpNativeBridge.loadPackage(packagePath)
  }

  start(): CommonResult {
    return MrpNativeBridge.start()
  }

  pause(): CommonResult {
    return MrpNativeBridge.pause()
  }

  resume(): CommonResult {
    return MrpNativeBridge.resume()
  }

  sendInput(event: InputEvent): CommonResult {
    return MrpNativeBridge.sendInput(event)
  }

  pullFrame(): FrameResult {
    return MrpNativeBridge.pullFrame()
  }

  release(): CommonResult {
    return MrpNativeBridge.release()
  }
}
```

---

## 4. ArkTS：`EmulatorPage.ets`

```ts
// entry/src/main/ets/pages/EmulatorPage.ets

import { MrpSimulatorFacade } from '../features/mrp/facade/MrpSimulatorFacade'
import type { FrameResult } from '../features/mrp/model/SimulatorTypes'

@Entry
@Component
struct EmulatorPage {
  private simulator: MrpSimulatorFacade = new MrpSimulatorFacade()
  @State frameId: number = 0
  @State statusText: string = 'idle'

  aboutToAppear(): void {
    this.simulator.init({
      workDir: '/data/storage/el2/base/files/mrp',
      width: 240,
      height: 320,
      debug: true,
    })
    this.simulator.loadPackage('/data/storage/el2/base/files/demo/demo.mrp')
    this.simulator.start()
    this.statusText = 'running'
  }

  aboutToDisappear(): void {
    this.simulator.release()
    this.statusText = 'released'
  }

  onTick(): void {
    const frame: FrameResult = this.simulator.pullFrame()
    if (frame.ok && frame.hasNewFrame) {
      this.frameId = frame.frameId ?? (this.frameId + 1)
    }
  }

  build() {
    Column() {
      Text(`status: ${this.statusText}`)
      Text(`frameId: ${this.frameId}`)
      Button('UP').onClick(() => {
        this.simulator.sendInput({ type: 'key', action: 'down', keyCode: 19, timestamp: Date.now() })
      })
      Button('Pause').onClick(() => {
        this.simulator.pause()
        this.statusText = 'paused'
      })
      Button('Resume').onClick(() => {
        this.simulator.resume()
        this.statusText = 'running'
      })
      Button('Tick PullFrame').onClick(() => this.onTick())
    }
  }
}
```

---

## 5. C++：`module_init.cpp`

```cpp
// entry/src/main/cpp/napi/module_init.cpp

#include <node_api.h>

napi_value ExportMrpFacade(napi_env env, napi_value exports);

static napi_value Init(napi_env env, napi_value exports) {
    return ExportMrpFacade(env, exports);
}

static napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "mrp_napi",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterMrpModule(void) {
    napi_module_register(&g_module);
}
```

---

## 6. C++：`MrpFacade.h`

```cpp
// entry/src/main/cpp/facade/MrpFacade.h

#ifndef MRP_FACADE_H
#define MRP_FACADE_H

#include <string>
#include <vector>

struct CommonResult {
    bool ok;
    int errorCode;
    std::string errorMessage;
};

struct InitOptions {
    std::string workDir;
    int width;
    int height;
    bool debug;
};

struct InputEvent {
    std::string type;
    std::string action;
    int keyCode;
    int x;
    int y;
    long long timestamp;
};

struct FrameResult {
    bool ok;
    bool hasNewFrame;
    int width;
    int height;
    std::string pixelFormat;
    std::vector<uint8_t> buffer;
    long long frameId;
    int errorCode;
    std::string errorMessage;
};

class MrpFacade {
public:
    CommonResult Init(const InitOptions &options);
    CommonResult LoadPackage(const std::string &packagePath);
    CommonResult Start();
    CommonResult Pause();
    CommonResult Resume();
    CommonResult SendInput(const InputEvent &event);
    FrameResult PullFrame();
    CommonResult Release();
};

#endif
```

---

## 7. C++：`MrpFacade.cpp`

```cpp
// entry/src/main/cpp/facade/MrpFacade.cpp

#include "MrpFacade.h"

CommonResult MrpFacade::Init(const InitOptions &options)
{
    return {true, 0, ""};
}

CommonResult MrpFacade::LoadPackage(const std::string &packagePath)
{
    return {true, 0, ""};
}

CommonResult MrpFacade::Start()
{
    return {true, 0, ""};
}

CommonResult MrpFacade::Pause()
{
    return {true, 0, ""};
}

CommonResult MrpFacade::Resume()
{
    return {true, 0, ""};
}

CommonResult MrpFacade::SendInput(const InputEvent &event)
{
    return {true, 0, ""};
}

FrameResult MrpFacade::PullFrame()
{
    return {true, false, 240, 320, "RGBA_8888", {}, 0, 0, ""};
}

CommonResult MrpFacade::Release()
{
    return {true, 0, ""};
}
```

---

## 8. C++：`facade_binding.cpp`

```cpp
// entry/src/main/cpp/napi/facade_binding.cpp

#include <node_api.h>
#include "../facade/MrpFacade.h"

static MrpFacade g_facade;

static napi_value MakeBoolResult(napi_env env, const CommonResult &result)
{
    napi_value obj;
    napi_create_object(env, &obj);

    napi_value ok;
    napi_get_boolean(env, result.ok, &ok);
    napi_set_named_property(env, obj, "ok", ok);
    return obj;
}

static napi_value Init(napi_env env, napi_callback_info info)
{
    InitOptions options {"", 240, 320, false};
    return MakeBoolResult(env, g_facade.Init(options));
}

static napi_value LoadPackage(napi_env env, napi_callback_info info)
{
    return MakeBoolResult(env, g_facade.LoadPackage(""));
}

static napi_value Start(napi_env env, napi_callback_info info)
{
    return MakeBoolResult(env, g_facade.Start());
}

static napi_value Pause(napi_env env, napi_callback_info info)
{
    return MakeBoolResult(env, g_facade.Pause());
}

static napi_value Resume(napi_env env, napi_callback_info info)
{
    return MakeBoolResult(env, g_facade.Resume());
}

static napi_value SendInput(napi_env env, napi_callback_info info)
{
    InputEvent event{"key", "down", 0, 0, 0, 0};
    return MakeBoolResult(env, g_facade.SendInput(event));
}

static napi_value PullFrame(napi_env env, napi_callback_info info)
{
    FrameResult frame = g_facade.PullFrame();
    napi_value obj;
    napi_create_object(env, &obj);

    napi_value ok;
    napi_get_boolean(env, frame.ok, &ok);
    napi_set_named_property(env, obj, "ok", ok);
    return obj;
}

static napi_value Release(napi_env env, napi_callback_info info)
{
    return MakeBoolResult(env, g_facade.Release());
}

napi_value ExportMrpFacade(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        { "init", nullptr, Init, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "loadPackage", nullptr, LoadPackage, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "start", nullptr, Start, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "pause", nullptr, Pause, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "resume", nullptr, Resume, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "sendInput", nullptr, SendInput, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "pullFrame", nullptr, PullFrame, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "release", nullptr, Release, nullptr, nullptr, nullptr, napi_default, nullptr },
    };

    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
```

---

## 9. C++：`MrpRuntime.h`

```cpp
// entry/src/main/cpp/core/runtime/MrpRuntime.h

#ifndef MRP_RUNTIME_H
#define MRP_RUNTIME_H

class MrpRuntime {
public:
    bool Start();
    bool Pause();
    bool Resume();
    bool Release();
};

#endif
```

## 10. C++：`MrpRuntime.cpp`

```cpp
// entry/src/main/cpp/core/runtime/MrpRuntime.cpp

#include "MrpRuntime.h"

bool MrpRuntime::Start() { return true; }
bool MrpRuntime::Pause() { return true; }
bool MrpRuntime::Resume() { return true; }
bool MrpRuntime::Release() { return true; }
```

---

## 11. C++：`FrameBuffer.h`

```cpp
// entry/src/main/cpp/core/render/FrameBuffer.h

#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include <cstdint>
#include <vector>

class FrameBuffer {
public:
    FrameBuffer(int width, int height);
    const std::vector<uint8_t> &Data() const;
private:
    int width_;
    int height_;
    std::vector<uint8_t> rgba_;
};

#endif
```

## 12. C++：`FrameBuffer.cpp`

```cpp
// entry/src/main/cpp/core/render/FrameBuffer.cpp

#include "FrameBuffer.h"

FrameBuffer::FrameBuffer(int width, int height)
    : width_(width), height_(height), rgba_(width * height * 4, 0)
{}

const std::vector<uint8_t> &FrameBuffer::Data() const
{
    return rgba_;
}
```

---

## 13. C++：`InputEvent.h`

```cpp
// entry/src/main/cpp/core/input/InputEvent.h

#ifndef MRP_INPUT_EVENT_H
#define MRP_INPUT_EVENT_H

struct MrpInputEvent {
    int keyCode;
    int x;
    int y;
    int action;
};

#endif
```

## 14. C++：`InputDispatcher.cpp`

```cpp
// entry/src/main/cpp/core/input/InputDispatcher.cpp

#include "InputEvent.h"

bool DispatchInput(const MrpInputEvent &event)
{
    return true;
}
```

---

## 15. 这版骨架的用途
- 先冻结 ArkTS ↔ Native 的方法签名
- 先打通 NAPI 模块注册与最小调用链路
- 先允许 `pullFrame()` 返回测试帧或空帧，优先验证链路
- 真实 MRP 内核适配后续落在 `adapter/existing_core/`
