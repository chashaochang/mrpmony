# ArkTS 壳层页面-Store-Service-Adapter 契约 v0

项目：MRP 模拟器鸿蒙化  
角色：RD Frontend  
范围：FE-002

## 1. 页面输入输出边界

### 1.1 AppListPage
输入：
- `AppCatalogStore.items`
- `AppCatalogStore.loading`
- `AppCatalogStore.error`

输出：
- `loadApps()`
- `refreshApps()`
- `openImportPage()`
- `openRunPage(appId)`

页面只消费：
- 应用列表 ViewModel
- 加载态
- 错误态

页面不消费：
- native 列表原始结构
- 本地存储路径
- 运行句柄

---

### 1.2 ImportPage
输入：
- `ImportStore.phase`
- `ImportStore.selectedFile`
- `ImportStore.previewInfo`
- `ImportStore.error`

输出：
- `pickFile()`
- `analyzeSelectedFile()`
- `startImport()`
- `backToListAfterSuccess()`
- `retryImport()`

页面只消费：
- 导入阶段
- 预览信息
- 成功/失败结果

页面不消费：
- 文件复制进度实现
- 包校验细节
- URI 到沙箱路径转换逻辑

---

### 1.3 RunPage
输入：
- `RuntimeSessionStore.currentAppId`
- `RuntimeSessionStore.phase`
- `RuntimeSessionStore.error`
- `RuntimeSessionStore.canRestart`
- `RuntimeSessionStore.canExit`

输出：
- `startSession(appId)`
- `restartSession()`
- `stopSession()`
- `leaveRunPage()`
- `notifyPageShow()`
- `notifyPageHide()`

页面只消费：
- 会话状态
- 控件可用性
- 结构化错误提示

页面不消费：
- runtime native handle
- surface/XComponent 生命周期对象
- 输入注入协议

---

## 2. Store 最小接口

### 2.1 AppCatalogStore
```ts
interface AppCatalogStore {
  items: AppItemVM[]
  loading: boolean
  error: UiError | null

  loadApps(): Promise<void>
  refreshApps(): Promise<void>
  clearError(): void
}
```

### 2.2 ImportStore
```ts
type ImportPhase = 'idle' | 'picking' | 'analyzing' | 'importing' | 'success' | 'failed'

interface ImportStore {
  phase: ImportPhase
  selectedFile: string | null
  previewInfo: PackagePreviewVM | null
  error: UiError | null

  pickFile(): Promise<void>
  analyzeSelectedFile(): Promise<void>
  startImport(): Promise<void>
  reset(): void
}
```

### 2.3 RuntimeSessionStore
```ts
type RuntimePhase = 'idle' | 'starting' | 'running' | 'error' | 'stopped'

interface RuntimeSessionStore {
  currentAppId: string | null
  phase: RuntimePhase
  error: UiError | null
  canRestart: boolean
  canExit: boolean

  startSession(appId: string): Promise<void>
  restartSession(): Promise<void>
  stopSession(): Promise<void>
  clearError(): void
}
```

---

## 3. Service 最小接口

### 3.1 AppService
```ts
interface AppService {
  listApps(): Promise<AppItemVM[]>
}
```

职责：
- 从 NativeAppAdapter 拉取应用元数据
- 转换为 AppItemVM
- 屏蔽 native 原始字段

### 3.2 ImportService
```ts
interface ImportService {
  pickMrpFile(): Promise<string | null>
  analyzePackage(fileUri: string): Promise<PackagePreviewVM>
  importPackage(fileUri: string): Promise<{ appId: string }>
}
```

职责：
- 编排文件选择
- 解析包预览信息
- 发起导入
- 输出标准化错误

### 3.3 RuntimeService
```ts
interface RuntimeService {
  start(appId: string): Promise<void>
  restart(appId: string): Promise<void>
  stop(): Promise<void>
  onPageShow(): Promise<void>
  onPageHide(): Promise<void>
}
```

职责：
- 编排运行会话生命周期
- 管理启动/停止/重启
- 转换 runtime 错误为 UiError

---

## 4. Adapter 占位契约

### 4.1 NativeAppAdapter
```ts
interface NativeAppAdapter {
  listInstalledApps(): Promise<Array<{
    appId: string
    name: string
    icon?: string
    version?: string
    runnable: boolean
  }>>
}
```

### 4.2 NativeImportAdapter
```ts
interface NativeImportAdapter {
  pickFile(): Promise<string | null>
  inspectPackage(fileUri: string): Promise<{
    name: string
    version?: string
    icon?: string
    size?: number
    valid: boolean
  }>
  importPackage(fileUri: string): Promise<{
    appId: string
  }>
}
```

### 4.3 NativeRuntimeAdapter
```ts
interface NativeRuntimeAdapter {
  startSession(appId: string): Promise<void>
  restartSession(appId: string): Promise<void>
  stopSession(): Promise<void>
  notifyPageShow(): Promise<void>
  notifyPageHide(): Promise<void>
}
```

---

## 5. 模型最小定义

### 5.1 AppItemVM
```ts
interface AppItemVM {
  id: string
  name: string
  icon?: string
  version?: string
  status: 'ready' | 'invalid'
}
```

### 5.2 PackagePreviewVM
```ts
interface PackagePreviewVM {
  name: string
  version?: string
  icon?: string
  sizeText?: string
  valid: boolean
}
```

### 5.3 UiError
```ts
interface UiError {
  message: string
  retryable: boolean
}
```

---

## 6. 契约约束

### ArkTS 页面必须遵守
- 只调用 Store Action
- 不直接调用 NativeAdapter
- 不持有 native 返回原始对象
- 不解析 native 错误码

### Store 必须遵守
- 不处理 UI 组件逻辑
- 不暴露 native 对象给页面
- 所有异步状态先转成 ArkTS 可消费状态

### Service 必须遵守
- 只做业务编排与错误归一
- 不承载页面导航
- 不输出 native 路径和句柄

### Adapter 必须遵守
- 封装所有 native 细节
- 允许后续替换命名，不影响页面层

---

## 7. 结论

该契约用于先冻结 ArkTS 壳层输入输出边界，不等待 native 最终接口命名冻结。后续 native 能力接入时，仅允许在 Adapter 层收敛变化。 
