# ArkTS 导入成功后的列表刷新进展 v0

项目：MRP 模拟器鸿蒙化  
角色：RD Frontend

## 本轮完成

### 1. AppListPage 已支持 refreshToken
新增：
- `@Prop refreshToken`
- `ensureRefreshByToken()`
- 当 ShellRootPage 变更 refreshToken 时，触发 `store.refreshApps()`

### 2. ShellRootPage 已接导入成功后的刷新意图
当前逻辑：
- Import 成功 -> `backToListAfterImport()`
- `catalogRefreshTick += 1`
- 切回 `appList`
- `AppListPage` 接收新 token 并刷新列表

### 3. AppListView 已禁用不可运行应用点击
当前逻辑：
- `status === 'ready'` 才允许触发 `onItemTap`
- 不可运行应用降透明度显示

## 当前结论
- 导入成功后的列表刷新占位链路已补齐
- 列表页已具备最小可交互约束
- 仍为 POC 占位实现，后续可替换为正式路由与状态框架

## 下一步
1. 给 RunPage 增加 mock 启动中 -> 运行中状态过渡
2. 给 ImportPage 增加返回后重置本地状态
3. 补 ArkTS 页面流演示说明图
