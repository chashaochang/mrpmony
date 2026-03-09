# ArkTS 壳层代码骨架占位版 v0

项目：MRP 模拟器鸿蒙化  
角色：RD Frontend

## 已落地目录

```text
entry/src/main/ets/
├─ pages/
├─ components/common/
├─ components/appList/
├─ components/import/
├─ components/runtime/
├─ store/
├─ services/
├─ adapters/
└─ models/
```

## 已落地文件

### pages
- `pages/AppListPage.ets`
- `pages/ImportPage.ets`
- `pages/RunPage.ets`

### common components
- `components/common/PageHeader.ets`
- `components/common/LoadingBlock.ets`
- `components/common/StatusBanner.ets`
- `components/common/ConfirmDialog.ets`
- `components/common/EmptyState.ets`

### appList components
- `components/appList/AppListView.ets`
- `components/appList/AppCard.ets`
- `components/appList/ImportEntryButton.ets`

### import components
- `components/import/ImportSourceSelector.ets`
- `components/import/PackagePreviewCard.ets`
- `components/import/ImportProgressPanel.ets`
- `components/import/ImportResultPanel.ets`

### runtime components
- `components/runtime/RuntimeContainer.ets`
- `components/runtime/RuntimeToolbar.ets`
- `components/runtime/RuntimeStatusOverlay.ets`

### store
- `store/AppCatalogStore.ts`
- `store/ImportStore.ts`
- `store/RuntimeSessionStore.ts`

### services
- `services/AppService.ts`
- `services/ImportService.ts`
- `services/RuntimeService.ts`

### adapters
- `adapters/NativeAppAdapter.ts`
- `adapters/NativeImportAdapter.ts`
- `adapters/NativeRuntimeAdapter.ts`

### models
- `models/AppItemVM.ts`
- `models/PackagePreviewVM.ts`
- `models/RuntimeSessionState.ts`
- `models/UiError.ts`

## 当前状态
- 目录结构已与 FE-001 对齐
- 契约层已与 FE-002 对齐
- 页面与 native 边界遵守 FE-003
- 当前为占位版，不依赖 native 最终命名冻结

## 下一步建议
1. 给 3 个 Page 接入真实组件树
2. 为 Store 增加 ArkTS 响应式改造
3. 为 Adapter 接入 NAPI façade 占位实现
4. 增加路由与页面跳转最小闭环
