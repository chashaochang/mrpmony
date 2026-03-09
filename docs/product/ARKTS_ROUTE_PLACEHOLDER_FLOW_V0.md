# ArkTS 最小路由占位与页面闭环草案 v0

项目：MRP 模拟器鸿蒙化  
角色：RD Frontend

## 本轮完成

### 1. 新增 ShellRootPage
文件：`entry/src/main/ets/pages/ShellRootPage.ets`

职责：
- 统一维护最小路由状态
- 控制 3 个页面切换
- 承接页面之间的回调跳转

当前路由：
- `appList`
- `import`
- `run`

### 2. AppListPage 已支持路由回调
新增：
- `onNavigateImport()`
- `onNavigateRun(appId)`

### 3. ImportPage 已支持导入后回流
新增：
- `onBack()`
- `onImportSuccess()`

当前闭环：
- AppList -> Import
- Import 成功 -> AppList

### 4. RunPage 已支持运行页回流
新增：
- `appId`
- `onBack()`

当前闭环：
- AppList -> Run
- Run 退出 -> AppList

## 当前结论
- 已从“页面占位”推进到“最小路由闭环占位”
- 当前路由仍为 ArkTS 壳层内管理，不依赖正式 Router 框架
- 可继续作为 POC 页面流演示基线

## 下一步
1. 让导入成功真正触发 AppList 刷新而不是仅返回
2. 让列表中不可运行应用禁用点击
3. 增加运行页 mock 启动中 -> 运行中状态过渡
