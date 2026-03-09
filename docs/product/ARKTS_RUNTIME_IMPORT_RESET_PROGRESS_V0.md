# RunPage mock 启动过渡与 ImportPage 重置进展 v0

项目：MRP 模拟器鸿蒙化  
角色：RD Frontend

## 本轮完成

### 1. Runtime mock 启动过渡已补齐
- `NativeRuntimeAdapter.startSession()` 增加 600ms mock 延迟
- `NativeRuntimeAdapter.restartSession()` 增加 400ms mock 延迟
- `RuntimeSessionStore` 在 start/restart 时显式进入 `starting`
- 运行页现在可观察到 `starting -> running` 状态过渡

### 2. ImportPage 返回/成功后重置已补齐
- 新增 `onBackClick()`
- 返回列表前调用 `store.reset()`
- 导入成功后先 `store.reset()` 再回调 `onImportSuccess()`
- `aboutToDisappear()` 兜底重置导入态

## 当前结论
- 运行页已具备最小 mock 启动状态演示能力
- 导入页不会把旧预览/旧错误带回下一轮导入

## 下一步
1. 让 RunPage 在不可 restart 时禁用重启按钮
2. 给 AppList 增加“导入成功后新应用高亮”占位
3. 补一份 POC 演示路径说明
