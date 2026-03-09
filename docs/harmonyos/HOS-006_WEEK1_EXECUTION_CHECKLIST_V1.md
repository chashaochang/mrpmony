# HOS-006 MRP HarmonyOS POC 第一周执行清单（开发顺序 + 验证顺序）

## 一、第一周开发顺序（固定）

### Day 1：模块与接口底座
1. 建立 ArkTS 壳层目录
2. 建立 `MrpNativeBridge.ets`
3. 建立 `MrpSimulatorFacade.ets`
4. 建立 `SimulatorTypes.ets`
5. 建立 C++ `module_init.cpp`
6. 建立 C++ `facade_binding.cpp`
7. 建立 `MrpFacade.h/.cpp`

**当日最小交付**
- ArkTS 能调用到 native 的 `init()`
- NAPI 模块在真机可加载

---

### Day 2：运行时与拉帧链路
1. 建立 `MrpRuntime.h/.cpp`
2. 建立 `FrameBuffer.h/.cpp`
3. 在 `facade_binding.cpp` 接入 `pullFrame()`
4. 建立 `EmulatorPage.ets`
5. 建立 `FrameScheduler.ets`
6. 建立 `EmulatorCanvas.ets`

**当日最小交付**
- 页面能拿到测试帧
- 画面可显示，不黑屏

---

### Day 3：输入链路
1. 建立 `InputEvent.h`
2. 建立 `InputDispatcher.cpp`
3. 建立 `InputMapper.ets`
4. 建立 `KeypadOverlay.ets`
5. 在页面接入 `sendInput()`

**当日最小交付**
- 至少一个按键事件可打到 native
- 至少一个输入能触发可见反馈

---

### Day 4：生命周期与回收
1. 建立 `SimulatorSession.ets`
2. 建立 `AbilityLifecycle.ets`
3. 接入 `pause()` / `resume()` / `release()`
4. 验证页面退出与重新进入

**当日最小交付**
- 退后台可暂停
- 回前台可恢复
- 页面退出不崩溃

---

### Day 5：联调与收口
1. 固化验证用例
2. 对齐错误码和日志口径
3. 梳理后续真实内核接入点
4. 输出首周结论

**当日最小交付**
- 三个技术断点均有结论
- 能明确是否进入真实内核适配阶段

---

## 二、第一周验证顺序（固定）

### 验证 1：NAPI 模块装载
- 验证 `init()`
- 验证真机可加载
- 验证重复进入页面无异常

### 验证 2：拉帧链路
- 验证 `start()` + `pullFrame()`
- 验证 `buffer` 长度、像素格式、宽高
- 验证页面显示稳定

### 验证 3：输入闭环
- 验证 `sendInput()`
- 验证按键/触摸至少一条链路可用
- 验证输入后有可见反馈

### 验证 4：生命周期
- 验证 `pause()`
- 验证 `resume()`
- 验证 `release()` 后再次进入页面是否正常

---

## 三、严格禁止的推进顺序错误
- NAPI 未装载成功前，不进入页面细化
- 拉帧未通前，不做复杂输入联调
- 输入未闭环前，不做兼容性扩展
- 生命周期未稳前，不做音频接入

---

## 四、每日回报最小模板
- 刚完成：xx
- 当前在做：xx
- 下一项：xx
- 预计时间：xx
- 是否阻塞：有/无

---

## 五、首周收口判据
满足以下条件才算首周完成：
1. `init/loadPackage/start/pullFrame/sendInput/pause/resume/release` 八个接口已冻结
2. 三个技术断点已有通过/失败结论
3. 第一周代码骨架与验证模板已齐
4. 已能决定第二周是否进入真实 MRP 内核适配
