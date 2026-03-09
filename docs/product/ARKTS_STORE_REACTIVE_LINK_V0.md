# ArkTS Store 最小响应式联动方案 v0

项目：MRP 模拟器鸿蒙化  
角色：RD Frontend

## 本轮完成

### 1. 新增 StoreObserver
文件：`entry/src/main/ets/store/StoreObserver.ts`

职责：
- 提供 `subscribe()`
- 提供 `notify()`
- 作为 3 个 Store 的最小观察者基类

### 2. 3 个 Store 已接通知机制
- `AppCatalogStore`
- `ImportStore`
- `RuntimeSessionStore`

做法：
- 状态变更后调用 `notify()`
- 页面不直接轮询 Store
- 页面通过订阅回调触发局部刷新

### 3. 3 个页面已接最小刷新联动
- `AppListPage`
- `ImportPage`
- `RunPage`

做法：
- 页面新增 `@State refreshTick`
- `aboutToAppear()` 中订阅 Store
- `aboutToDisappear()` 中取消订阅
- Store 状态变化时递增 `refreshTick`，驱动页面重绘

## 当前结论
- 已从“Store 可调用”推进到“Store 可驱动页面刷新”的最小版
- 当前方案是 POC 响应式占位实现，不替代后续正式状态框架

## 下一步
1. 把 AppListPage 的列表项点击接到 `RunPage` 跳转占位
2. 为 Adapter 增加 mock 数据，形成可演示的页面闭环
3. 补一份 ArkTS 页面路由占位草案
