# ArkTS 壳层最小可演示闭环说明 v1

项目：mrp-simulator-harmonyos

## 1. 运行入口在哪里

运行入口在：
- `entry/src/main/ets/pages/ShellRootPage.ets`
- `entry/src/main/ets/pages/AppListPage.ets`

当前可见入口：
1. App 启动进入 `ShellRootPage`
2. 默认路由进入 `AppListPage`
3. 用户点击 **“可运行”应用卡片**
4. 通过 `onNavigateRun(appId)` 切到 `RunPage`

## 2. 页面流如何走

当前最小页面流：
- `AppListPage -> ImportPage -> AppListPage`
- `AppListPage -> RunPage -> AppListPage`

具体链路：
1. **列表页**
   - 展示应用列表
   - 展示运行入口提示
   - 点击“导入应用”进入 `ImportPage`
   - 点击“可运行”卡片进入 `RunPage`
2. **导入页**
   - 选择文件
   - 展示预览
   - 点击“开始导入”
   - 导入成功后返回列表页
   - 返回列表页时触发列表刷新 token
3. **运行页**
   - 进入时根据 `appId` 发起 `startSession(appId)`
   - 状态从 `starting -> running`
   - 点击退出后回到列表页

## 3. 哪些接口已经为 NAPI 预留

### 3.1 App 列表相关
- `entry/src/main/ets/adapters/NativeAppAdapter.ts`
  - `listInstalledApps()`

### 3.2 导入相关
- `entry/src/main/ets/adapters/NativeImportAdapter.ts`
  - `pickFile()`
  - `inspectPackage(fileUri)`
  - `importPackage(fileUri)`

### 3.3 运行时相关
- `entry/src/main/ets/adapters/NativeRuntimeAdapter.ts`
  - `startSession(appId)`
  - `restartSession(appId)`
  - `stopSession()`
  - `notifyPageShow()`
  - `notifyPageHide()`

### 3.4 Service 收口层
- `entry/src/main/ets/services/AppService.ts`
- `entry/src/main/ets/services/ImportService.ts`
- `entry/src/main/ets/services/RuntimeService.ts`

说明：
- 后续 NAPI 接入时，优先替换 Adapter 内部实现
- 页面层和 Store 层不直接碰 native 细节

## 4. 当前可见结果

当前已经具备：
- 列表页可见运行入口与导入入口
- 导入成功后返回列表并刷新
- 运行页可见 `starting / running / stopped` 状态展示
- 运行页工具栏根据状态控制按钮可用性

## 5. 本轮不做的事

本轮不继续扩大量新页面/组件，只收口：
- 现有页面可见闭环
- 现有 Adapter/NAPI 接缝
- 现有运行入口与状态提示
