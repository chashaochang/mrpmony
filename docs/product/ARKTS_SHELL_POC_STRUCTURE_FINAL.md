# ArkTS 壳层 POC 目录结构最终版

项目：MRP 模拟器鸿蒙化  
角色：RD Frontend  
范围：FE-001

## 1. 目录树最终版

```text
entry/src/main/ets/
├─ pages/
│  ├─ AppListPage.ets
│  ├─ ImportPage.ets
│  └─ RunPage.ets
├─ components/
│  ├─ common/
│  │  ├─ PageHeader.ets
│  │  ├─ LoadingBlock.ets
│  │  ├─ StatusBanner.ets
│  │  ├─ ConfirmDialog.ets
│  │  └─ EmptyState.ets
│  ├─ appList/
│  │  ├─ AppListView.ets
│  │  ├─ AppCard.ets
│  │  └─ ImportEntryButton.ets
│  ├─ import/
│  │  ├─ ImportSourceSelector.ets
│  │  ├─ PackagePreviewCard.ets
│  │  ├─ ImportProgressPanel.ets
│  │  └─ ImportResultPanel.ets
│  └─ runtime/
│     ├─ RuntimeContainer.ets
│     ├─ RuntimeToolbar.ets
│     └─ RuntimeStatusOverlay.ets
├─ store/
│  ├─ AppCatalogStore.ts
│  ├─ ImportStore.ts
│  └─ RuntimeSessionStore.ts
├─ services/
│  ├─ AppService.ts
│  ├─ ImportService.ts
│  └─ RuntimeService.ts
├─ adapters/
│  ├─ NativeAppAdapter.ts
│  ├─ NativeImportAdapter.ts
│  └─ NativeRuntimeAdapter.ts
└─ models/
   ├─ AppItemVM.ts
   ├─ PackagePreviewVM.ts
   ├─ RuntimeSessionState.ts
   └─ UiError.ts
```

---

## 2. 页面职责分工

### 2.1 AppListPage
职责：
- 展示已导入应用列表
- 空态引导导入
- 触发列表刷新
- 选择应用并进入 RunPage
- 跳转 ImportPage

不负责：
- 包解析
- 文件导入实现
- native 路径/数据库细节
- runtime 会话细节

### 2.2 ImportPage
职责：
- 选择本地 MRP 文件
- 展示包预览信息
- 发起导入
- 展示导入中状态
- 展示导入成功/失败结果
- 导入成功后通知列表刷新

不负责：
- URI 转沙箱路径规则
- 解压/校验/复制实现
- 导入兼容性底层规则
- native 错误码判断

### 2.3 RunPage
职责：
- 承载模拟器运行区域
- 展示启动中/运行中/异常/已退出状态
- 提供返回/重启/退出操作
- 响应页面生命周期并转发给 RuntimeService

不负责：
- 渲染句柄管理
- surface / XComponent 细节
- 输入注入协议
- 模拟器核心线程与事件循环

---

## 3. 首版明确不做的页面

### 3.1 SettingsPage
不做原因：
- 不属于 POC 最小闭环
- 容易过早暴露 native 配置细节
- 首版使用默认配置即可

### 3.2 AppDetailPage
不做原因：
- 与列表页信息重叠
- 不影响导入/运行主链路

### 3.3 ImportHistoryPage
不做原因：
- 需要额外持久化与查询逻辑
- POC 验证价值低

### 3.4 ErrorDiagnosisPage / LogPage
不做原因：
- 首版只需要结构化错误提示
- 独立诊断页会过早耦合 native 日志格式与路径

### 3.5 SessionRecoveryPage
不做原因：
- 依赖会话恢复协议与异常恢复机制
- 二阶段再进入

---

## 4. 页面层保留原则

页面层只保留：
- 页面路由
- UI 组件组合
- 用户交互
- 状态展示
- 调用 Store Action

页面层不保留：
- native 原始对象
- 文件系统路径
- 导入实现细节
- 渲染句柄
- 错误码映射

---

## 5. 结论

ArkTS 壳层 POC 最小页面结构固定为：
- 3 个页面：AppListPage / ImportPage / RunPage
- 13 个组件
- 3 个 Store
- 3 个 Service
- 3 个 Adapter
- 4 个最小模型

该结构用于支撑：
- 本地导入
- 应用列表展示
- 单实例运行
- 基础错误反馈
