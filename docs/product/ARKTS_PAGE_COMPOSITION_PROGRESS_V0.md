# ArkTS 页面组件树接线进展 v0

项目：MRP 模拟器鸿蒙化  
角色：RD Frontend

## 本轮完成

### 1. AppListPage 已接线
已接入：
- `PageHeader`
- `StatusBanner`
- `LoadingBlock`
- `EmptyState`
- `ImportEntryButton`
- `AppListView`

当前页面状态流：
- loading -> `LoadingBlock`
- error -> `StatusBanner`
- empty -> `EmptyState + ImportEntryButton`
- data ready -> `AppListView`

### 2. ImportPage 已接线
已接入：
- `PageHeader`
- `StatusBanner`
- `ImportSourceSelector`
- `PackagePreviewCard`
- `ImportProgressPanel`
- `ImportResultPanel`

当前页面状态流：
- preview ready -> `PackagePreviewCard`
- importing -> `ImportProgressPanel`
- success/failed -> `ImportResultPanel`

### 3. RunPage 已接线
已接入：
- `PageHeader`
- `StatusBanner`
- `RuntimeToolbar`
- `RuntimeContainer`
- `RuntimeStatusOverlay`
- `ConfirmDialog`

当前页面状态流：
- 非 running -> `RuntimeStatusOverlay`
- stopped -> `ConfirmDialog`

## 当前结论
- 3 个页面已从纯占位推进为“组件树已挂接”的壳层页面
- 仍为 POC 占位实现，尚未接入真实路由、真实用户事件和真实 native 数据流

## 下一步
1. 为 Store 做可观察/可驱动 UI 的响应式改造
2. 给页面补最小按钮事件
3. 增加页面之间最小跳转闭环
4. 预留 Adapter 到 NAPI façade 的接入点
