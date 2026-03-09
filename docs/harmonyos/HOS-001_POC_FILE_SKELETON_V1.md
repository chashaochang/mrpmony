# HOS-001 POC 文件级骨架清单（HarmonyOS POC v1）

## 目标
首周只支撑最小 POC 闭环：**初始化 → 加载包 → 启动 → 拉帧显示 → 输入 → 暂停/恢复 → 释放**。

---

## 一、ArkTS 侧骨架文件清单

```text
entry/src/main/ets/application/
├─ EntryAbility.ets
└─ AppContext.ets

entry/src/main/ets/pages/
├─ HomePage.ets
├─ EmulatorPage.ets
├─ SettingsPage.ets
└─ DebugPage.ets

entry/src/main/ets/components/
├─ EmulatorCanvas.ets
├─ KeypadOverlay.ets
└─ StatusBar.ets

entry/src/main/ets/features/mrp/facade/
└─ MrpSimulatorFacade.ets

entry/src/main/ets/features/mrp/service/
├─ SimulatorSession.ets
├─ InputMapper.ets
├─ FrameScheduler.ets
└─ PackageLoader.ets

entry/src/main/ets/features/mrp/model/
├─ SimulatorTypes.ets
├─ SimulatorState.ets
└─ KeyMapping.ets

entry/src/main/ets/features/mrp/infra/native/
├─ MrpNativeBridge.ets
└─ MrpNativeTypes.ets

entry/src/main/ets/features/mrp/infra/platform/
├─ AbilityLifecycle.ets
└─ FileAccess.ets

entry/src/main/ets/features/mrp/infra/storage/
└─ ConfigStore.ets

entry/src/main/ets/common/logger/
└─ Logger.ets

entry/src/main/ets/common/error/
└─ ErrorCode.ets

entry/src/main/ets/common/constants/
└─ MrpConstants.ets
```

### ArkTS 模块关系说明
- `pages/`：页面入口，负责编排 UI，不直接触达 native。
- `components/`：模拟器画面与按键层的纯视图组件。
- `facade/MrpSimulatorFacade.ets`：页面唯一可依赖的运行时接口。
- `service/SimulatorSession.ets`：承接生命周期、运行状态、调用顺序控制。
- `service/FrameScheduler.ets`：POC 阶段负责定时调用 `pullFrame()` 并驱动页面刷新。
- `service/InputMapper.ets`：将鸿蒙触摸/按键统一映射为模拟器输入。
- `infra/native/`：ArkTS 与 NAPI 的唯一边界层。
- `infra/platform/`：文件沙箱、Ability 生命周期等平台能力封装。
- `model/`：对齐 ArkTS ↔ Native DTO，避免页面层散落字面量结构。

---

## 二、Native/C++ 侧骨架文件清单

```text
entry/src/main/cpp/
├─ CMakeLists.txt
├─ napi/
│  ├─ module_init.cpp
│  ├─ facade_binding.cpp
│  ├─ napi_common.h
│  ├─ napi_common.cpp
│  └─ converters/
│     ├─ js_to_native.cpp
│     └─ native_to_js.cpp
├─ facade/
│  ├─ MrpFacade.h
│  └─ MrpFacade.cpp
├─ core/
│  ├─ runtime/
│  │  ├─ MrpRuntime.h
│  │  └─ MrpRuntime.cpp
│  ├─ render/
│  │  ├─ FrameBuffer.h
│  │  └─ FrameBuffer.cpp
│  ├─ input/
│  │  ├─ InputEvent.h
│  │  └─ InputDispatcher.cpp
│  ├─ fs/
│  │  ├─ VirtualFs.h
│  │  └─ VirtualFs.cpp
│  └─ types/
│     ├─ SimulatorConfig.h
│     └─ Result.h
└─ adapter/
   └─ existing_core/
      ├─ LegacyMrpAdapter.h
      └─ LegacyMrpAdapter.cpp
```

### Native 模块关系说明
- `napi/`：只做 JS/ArkTS 参数解析、返回值转换、导出注册。
- `facade/`：对 ArkTS 暴露稳定接口，不让上层感知内核细节。
- `core/runtime/`：主循环、状态流转、运行态控制。
- `core/render/`：帧缓冲、像素输出。
- `core/input/`：输入归一化、按键/触摸投递。
- `core/fs/`：工作目录、包访问、存档目录等虚拟文件系统能力。
- `adapter/existing_core/`：对接现有 MRP 模拟器内核，首周可占位。

---

## 三、首周必须先创建的最小骨架文件

### ArkTS 必须项
```text
entry/src/main/ets/pages/EmulatorPage.ets
entry/src/main/ets/components/EmulatorCanvas.ets
entry/src/main/ets/features/mrp/facade/MrpSimulatorFacade.ets
entry/src/main/ets/features/mrp/service/SimulatorSession.ets
entry/src/main/ets/features/mrp/service/FrameScheduler.ets
entry/src/main/ets/features/mrp/service/InputMapper.ets
entry/src/main/ets/features/mrp/infra/native/MrpNativeBridge.ets
entry/src/main/ets/features/mrp/model/SimulatorTypes.ets
```

### Native 必须项
```text
entry/src/main/cpp/CMakeLists.txt
entry/src/main/cpp/napi/module_init.cpp
entry/src/main/cpp/napi/facade_binding.cpp
entry/src/main/cpp/facade/MrpFacade.h
entry/src/main/cpp/facade/MrpFacade.cpp
entry/src/main/cpp/core/runtime/MrpRuntime.h
entry/src/main/cpp/core/runtime/MrpRuntime.cpp
entry/src/main/cpp/core/render/FrameBuffer.h
entry/src/main/cpp/core/render/FrameBuffer.cpp
entry/src/main/cpp/core/input/InputEvent.h
entry/src/main/cpp/core/input/InputDispatcher.cpp
```

---

## 四、首周明确不进入骨架范围的内容
- 多实例管理
- 存档读写完整实现
- 音频底层完整接入
- 调试器/脚本注入
- 兼容补丁机制
- 回调事件总线

---

## 五、一期边界结论
- ArkTS 只做壳层与编排；页面不直接调 native。
- Native 层只暴露粗粒度 facade；不暴露碎片化底层接口。
- 若缺少现有 MRP 内核接口文档，`adapter/existing_core/` 先占位，不阻塞 POC 骨架推进。
