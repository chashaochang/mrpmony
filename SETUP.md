# SETUP

## 目标
让别人拿到仓库后，能按固定步骤完成：
- 初始化项目
- 导入 DevEco Studio
- 继续做 ArkTS + Native/NAPI 联调

---

## 1. 准备本地文件
仓库不提交真实 `local.properties`，请自行复制：

```bash
cp local.properties.example local.properties
```

然后修改：

```properties
hwsdk.dir=/absolute/path/to/HarmonyOS/Sdk
```

---

## 2. 导入 DevEco Studio
- 打开仓库根目录：`mrp-simulator-harmonyos`
- 等待工程同步
- 若 IDE 提示补全依赖/构建环境，按 IDE 引导完成

---

## 3. 关于签名
当前仓库已移除对不存在签名文件的硬引用。

如果你要实际构建/运行：
- 在 DevEco 中配置本地 debug signing
- 或按团队规范补本地签名材料

---

## 4. 关于 hvigorw
仓库已提供：
- `hvigorw`
- `hvigorw.bat`

如果本机还没有 `hvigor`：
- 先用 DevEco Studio 打开并同步
- 或自行安装 hvigor 后再执行命令行任务

---

## 5. 最小自检清单
导入后至少确认：
- 根目录可识别为 HarmonyOS 工程
- `entry` 模块可识别
- `entry/src/main/ets` 存在
- `entry/src/main/cpp` 存在
- `entry/src/main/resources` 存在
- `EntryAbility` 与 `ShellRootPage` 可被 IDE 正常索引
