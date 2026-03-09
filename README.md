# MRP Simulator HarmonyOS

HarmonyOS 侧的 MRP 模拟器 POC/Alpha 工程。

当前代码状态不是“纯方案仓库”，而是已经包含：
- **标准 HarmonyOS/DevEco 工程壳**
- **ArkTS 壳层页面与运行页占位链路**
- **Native/NAPI/C++ 最小骨架**
- **RunPage -> Native smoke start -> pullFrame() 占位渲染** 的首轮联调路径

---

## 当前仓库结构（最小可导入）

```text
.
├─ AppScope/
│  └─ app.json5
├─ build-profile.json5
├─ hvigorfile.ts
├─ hvigorw
├─ hvigorw.bat
├─ local.properties.example
├─ oh-package.json5
├─ entry/
│  ├─ build-profile.json5
│  ├─ oh-package.json5
│  └─ src/main/
│     ├─ module.json5
│     ├─ ets/
│     ├─ cpp/
│     └─ resources/
└─ README.md
```

---

## 先说明 3 个容易误解的点

### 1. `local.properties`
这是**本地机器文件**，不应提交真实值进仓库。
仓库提供了：
- `local.properties.example`

你需要自己复制一份：

```bash
cp local.properties.example local.properties
```

然后把 `hwsdk.dir` 改成你本机 HarmonyOS SDK 路径。

### 2. 签名文件
仓库**不再引用**不存在的：
- `AppScope/debug.p12`
- `AppScope/debug.mobileprovision`

原因：这些是本地/团队环境相关文件，硬编码到仓库会直接阻塞初始化。

### 3. `hvigorw` / `hvigorw.bat`
已补入仓库。它们会优先调用：
- 本地 `node_modules/.bin/hvigor`
- 或系统中的 `hvigor`

如果你的环境里还没有 `hvigor`，先用 DevEco Studio 打开项目同步，或自行安装 hvigor。

---

## 初始化步骤（可复现）

### 方式 A：DevEco Studio 导入（推荐）
1. 克隆仓库
2. 复制模板：`local.properties.example -> local.properties`
3. 修改 `local.properties` 中的 `hwsdk.dir`
4. 用 DevEco Studio 打开**仓库根目录**
5. 等待工程同步完成
6. 如需构建/运行，在 DevEco 中补本地调试签名配置

### 方式 B：命令行初始化
前提：本机已具备 `node` 和 `hvigor`

```bash
cp local.properties.example local.properties
# 编辑 local.properties，填入 hwsdk.dir
./hvigorw tasks
```

如果 `./hvigorw` 提示找不到 `hvigor`，先回到方式 A，用 DevEco 完成首次同步。

---

## 当前可验证结果

当前工程已具备以下最小可验证链路：
- `EntryAbility` 可加载 `ShellRootPage`
- 运行页可进入 native smoke 启动路径
- `pullFrame()` 已接到 ArkTS 层
- 页面可显示测试帧元数据与测试占位渲染块

这意味着：
- **可以导入工程**
- **可以继续做 Native/NAPI 联调**
- 但**还不是完整可运行的 MRP 模拟器产品**

---

## 下一阶段重点
- 把 `pullFrame()` 的占位帧替换为真实 native 帧
- 把 `loadPackage(appId)` 替换为真实 MRP 包路径/入口
- 把输入映射从 smoke path 替换为真实内核事件分发
