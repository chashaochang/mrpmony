# ArkTS Mock 数据闭环进展 v0

项目：MRP 模拟器鸿蒙化  
角色：RD Frontend

## 本轮完成

### 1. Adapter 已补 mock 数据
- `NativeAppAdapter.listInstalledApps()` 返回 2 个示例应用
- `NativeImportAdapter.pickFile()` 返回 mock 文件 URI
- `NativeImportAdapter.inspectPackage()` 返回 mock 预览信息
- `NativeImportAdapter.importPackage()` 返回 mock appId
- `NativeRuntimeAdapter` 保持空成功返回

### 2. AppListView 已支持点击回调
新增：
- `onItemTap(appId)`

### 3. AppListPage 已接列表点击占位
新增：
- `selectedAppId`
- 点击列表项后记录 appId，并显示 Banner 提示

## 当前结论
- 页面展示已不再完全依赖真实 native 接口
- 列表页已有最小可演示数据流
- 下一步可继续补路由占位与 Import -> List 刷新占位

## 下一步
1. 补最小路由占位文档
2. 让导入成功后触发列表刷新占位
3. 让运行页支持 mock 启动入口
