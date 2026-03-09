# ArkTS 页面事件接线进展 v0

项目：MRP 模拟器鸿蒙化  
角色：RD Frontend

## 本轮完成

### 1. 组件已补回调入口
- `ImportEntryButton`：新增 `onTap`
- `ImportSourceSelector`：新增 `onPick`
- `RuntimeToolbar`：新增 `onBack / onRestart / onExit`

### 2. 页面已接动作占位
- `AppListPage`：点击导入按钮触发 `openImportPage()` 占位
- `ImportPage`：点击选择文件后触发 `pickFile() -> analyzeSelectedFile()`
- `RunPage`：工具栏按钮已接 `restartSession()` / `stopSession()` / 返回占位

## 当前结论
- 页面已不再是纯静态组件树
- 页面意图已开始进入 Store Action
- 当前仍未接真实路由与 ArkTS 响应式刷新机制

## 下一步
1. 给 Store 增加可驱动页面刷新的最小响应式方案
2. 补 AppList -> RunPage / AppList -> ImportPage 的路由占位
3. 预留 Adapter 到 NAPI façade 的 mock 结果
