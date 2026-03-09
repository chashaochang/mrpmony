# HOS-002 最小 NAPI Facade v0

## 目标
将首周接口收敛为只支撑 POC 运行闭环的**最小必需集**，避免在接口冻结前过度设计。

---

## 一、最小 NAPI Facade 最终候选接口

> 最终保留 8 个接口；其中 `pause/resume` 视作一期必须项，因为 HarmonyOS 生命周期切换会直接用到。

### 1. `init(options: InitOptions): CommonResult`

```ts
interface InitOptions {
  workDir: string
  width: number
  height: number
  debug?: boolean
}
```

```ts
interface CommonResult {
  ok: boolean
  errorCode?: number
  errorMessage?: string
}
```

用途：初始化工作目录、逻辑分辨率、运行环境。

---

### 2. `loadPackage(packagePath: string): CommonResult`

```ts
loadPackage(packagePath: string): CommonResult
```

用途：加载一个固定测试包/资源入口。

---

### 3. `start(): CommonResult`

```ts
start(): CommonResult
```

用途：启动模拟器主循环。

---

### 4. `pause(): CommonResult`

```ts
pause(): CommonResult
```

用途：Ability 退后台、页面失焦时暂停运行。

---

### 5. `resume(): CommonResult`

```ts
resume(): CommonResult
```

用途：从暂停状态恢复。

---

### 6. `sendInput(event: InputEvent): CommonResult`

```ts
interface InputEvent {
  type: 'key' | 'touch'
  action: 'down' | 'up' | 'move'
  keyCode?: number
  x?: number
  y?: number
  timestamp?: number
}
```

用途：统一输入入口，屏蔽页面层对内核输入模型的感知。

---

### 7. `pullFrame(): FrameResult`

```ts
interface FrameResult {
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

用途：POC 阶段拉取整帧像素缓冲，优先验证链路正确性。

---

### 8. `release(): CommonResult`

```ts
release(): CommonResult
```

用途：退出页面、异常兜底或销毁实例时统一释放资源。

---

## 二、ArkTS ↔ Native 对齐说明

### ArkTS 侧调用顺序
```text
init -> loadPackage -> start -> [pullFrame/sendInput] -> pause/resume -> release
```

### Native 侧状态流转
```text
idle -> initialized -> loaded -> running -> paused -> released
```

### 对齐原则
- `EmulatorPage` 不直接调用 NAPI，统一经过 `MrpSimulatorFacade.ets`。
- `MrpNativeBridge.ets` 只做 API 映射，不做业务编排。
- `pullFrame()` 一期先使用 `ArrayBuffer` 全帧回传；性能问题放到第二阶段再优化。
- `sendInput()` 统一收口，避免导出 `sendKeyDown/sendKeyUp/sendTouchMove...` 多套接口。
- 错误一律走 `CommonResult / FrameResult`，不在一期设计复杂异常订阅体系。

---

## 三、哪些先占位、哪些一期必须落地

### 一期必须落地
- `init`
- `loadPackage`
- `start`
- `pause`
- `resume`
- `sendInput`
- `pullFrame`
- `release`

### 可以先占位实现
- `loadPackage` 内部的真实 MRP 资源解析
- `sendInput` 到真实内核的最终分发
- `pullFrame` 的真实像素输出（可先返回测试帧）
- `pause/resume` 的完整状态保存逻辑

---

## 四、当前明确不纳入 v0 的接口
- `stop()`
- `getStats()`
- `saveState()/loadState()`
- 音频控制相关接口
- 多实例管理
- 回调订阅/事件监听接口

---

## 五、一期接口设计结论
- 优先保证接口**少、稳、粗粒度**。
- 首周允许 facade 背后仍是占位实现，但 ArkTS 与 Native 的方法签名必须先冻结。
- 若实际内核接口与 facade 不一致，由 `adapter/existing_core/` 吸收差异，不回流污染 ArkTS。 
