# HOS-005 MRP HarmonyOS POC 接口冻结稿 + 第一周验证用例模板

## 一、NAPI Facade v0 冻结接口

### 1. `init(options: InitOptions): CommonResult`
- 用途：初始化运行时、工作目录、逻辑分辨率
- 调用时机：页面进入后、首次加载包前

### 2. `loadPackage(packagePath: string): CommonResult`
- 用途：加载固定测试包
- 调用时机：`init()` 成功后、`start()` 前

### 3. `start(): CommonResult`
- 用途：启动模拟器主循环
- 调用时机：`loadPackage()` 成功后

### 4. `pause(): CommonResult`
- 用途：前后台切换、页面失焦暂停
- 调用时机：Ability/页面进入非活跃态时

### 5. `resume(): CommonResult`
- 用途：从暂停态恢复
- 调用时机：Ability/页面恢复活跃态时

### 6. `sendInput(event: InputEvent): CommonResult`
- 用途：统一投递按键/触摸输入
- 调用时机：运行态下由页面事件触发

### 7. `pullFrame(): FrameResult`
- 用途：拉取当前最新一帧图像
- 调用时机：运行态下定时/手动触发

### 8. `release(): CommonResult`
- 用途：页面退出、异常兜底时释放资源
- 调用时机：页面销毁或会话结束

---

## 二、ArkTS ↔ Native 调用时序冻结

```text
aboutToAppear
  -> init
  -> loadPackage
  -> start
  -> [loop: pullFrame]
  -> [user action: sendInput]

onBackground
  -> pause

onForeground
  -> resume

aboutToDisappear
  -> release
```

### 时序约束
- 不允许 `start()` 早于 `loadPackage()`
- 不允许 `sendInput()` 早于 `start()`
- `release()` 后不允许再次 `pullFrame()`
- 二次进入页面时必须重新走 `init -> loadPackage -> start`

---

## 三、第一周验证用例模板

### CASE-01：NAPI 模块装载验证
**目标**
- 真机成功加载 native 模块并完成 `init()`

**前置条件**
- HAP 可安装到真机
- native 模块已打包

**步骤**
1. 启动应用进入 `EmulatorPage`
2. 调用 `init()`
3. 观察返回结果和日志

**通过标准**
- 返回 `ok=true`
- 无模块未注册/符号缺失/加载失败错误

**失败标准**
- 崩溃
- `ok=false`
- 日志出现 load/register/link 失败

---

### CASE-02：拉帧链路验证
**目标**
- `pullFrame()` 返回有效帧并可显示

**前置条件**
- `init/loadPackage/start` 已成功

**步骤**
1. 触发 `pullFrame()`
2. 检查 `hasNewFrame`
3. 检查宽高、pixelFormat、buffer 长度
4. 页面显示测试帧

**通过标准**
- `ok=true`
- `hasNewFrame=true`
- 画面不黑屏、不花屏、不倒置

**失败标准**
- 无画面
- buffer 长度异常
- 颜色顺序错误/错行

---

### CASE-03：输入闭环验证
**目标**
- `sendInput()` 可驱动 native 状态变化

**前置条件**
- 模拟器已处于 running

**步骤**
1. 点击页面虚拟按键
2. 触发 `sendInput()`
3. 观察画面/状态是否有响应
4. 执行 `pause()` -> `resume()` 后再次输入

**通过标准**
- 输入后有可见反馈
- `pause/resume` 后输入链路仍有效

**失败标准**
- 输入成功但无任何反馈
- 生命周期切换后输入失效

---

## 四、何时拉人介入

### 拉 architect
- NAPI 模块打包/装载方案需要改工程结构
- `pullFrame()` 全帧回传性能明显不可接受
- 输入模型与历史内核模型严重不匹配

### 拉 QA
- 三个断点已全部打通
- 需要沉淀设备验证清单和固定回归模板

---

## 五、当前冻结结论
- 首周接口不再扩展新方法
- 先保证时序、参数、返回结构稳定
- 真实内核尚未接入时，允许 `pullFrame()` 返回测试帧、`sendInput()` 走占位逻辑
