# 开源调研：MRP 模拟器如何做鸿蒙化

日期：2026-03-09

## 结论先说

当前公开能参考的 MRP 开源项目，**最有价值的不是“鸿蒙版现成项目”**，而是：

1. **已有 Android 时代的 Mrpoid 开源实现**
2. **HarmonyOS 上现成的 NAPI/C++ 工具链与样例**

所以这事最合理的鸿蒙化路线不是“从零重写一个 ArkTS 模拟器”，而是：

- **复用 Mrpoid / libMrpoid 这类 C/C++ 模拟器核心思路**
- **把 Android Java 壳替换成 HarmonyOS ArkTS 壳**
- **用 NAPI 把 ArkTS 与 C/C++ 内核桥起来**

---

## 一、找到的 MRP 开源项目

### 1) Yichou/mrpoid2018
- 地址：`https://github.com/Yichou/mrpoid2018`
- 特征：
  - GitHub 搜索结果中最成熟的公开仓库之一
  - README 明确是 Mrpoid 开源项目
  - 目录结构包含：
    - `app/`
    - `lib/`
  - 语言占比大致为：
    - C 80%+
    - Java 17%+
- 这说明它本质上是：
  - **C/C++ 为主的模拟器内核**
  - 外面包一层 **Android Java 应用壳**

### 2) Yichou/libMrpoid
- 地址：`https://github.com/Yichou/libMrpoid`
- 特征：
  - 项目描述直接写了：**mrpoid 内核（依赖项目）**
- 价值：
  - 这是“可复用核心”的关键线索
  - 如果要做鸿蒙化，最值得研究的就是它的：
    - 运行时核心
    - 渲染/事件循环
    - 文件/音频/输入抽象

### 3) 其他派生仓库
GitHub / Gitee 上还能看到一些派生或镜像：
- `msojocs/mrpoid2022`
- `FormatFa/Mrpoid`
- Gitee 上的 `mrpoid`、`mrpoid2018`、`mrpoid2400` 等

这些仓库的价值主要是：
- 看别人二次开发改了什么
- 看兼容性补丁是怎么堆的
- 看 Android 侧壳层与 native 核心是怎么连接的

但**主参考源仍应优先放在 `mrpoid2018` + `libMrpoid`**。

---

## 二、没有找到什么

### 1) 没找到成熟的“现成鸿蒙版 MRP 模拟器”
没有查到一个成熟、被广泛使用的：
- HarmonyOS 版 MRP 模拟器
- OpenHarmony 版 Mrpoid
- Mythroad Emulator 原生鸿蒙实现

这意味着：
- 这事没有现成答案可直接 fork 完事
- 但也说明**路线更应该走“内核迁移 + 壳层重建”**，而不是幻想找现成成品

### 2) “mythroad emulator” 直接关键词命中很差
说明开源世界里更多还是围绕 **Mrpoid** 这个名字沉淀，而不是围绕 Mythroad 关键字。

---

## 三、找到的 HarmonyOS / NAPI 可复用项目

### 1) ohos-transplant/ohos_utils
- 地址：`https://gitee.com/thirdparty-transplant/ohos_utils`
- 页面描述显示：
  - 提供更优雅的 HarmonyOS NAPI C++ 接口封装
  - 支持函数、interface、class、异步处理、多线程、运行时切换等
- 价值：
  - 适合在鸿蒙化时减少手写 NAPI 胶水代码
  - 尤其适合复杂 C++ 能力对 ArkTS 暴露时做统一封装

### 2) ohos-transplant/napi_generator
- 地址：`https://gitee.com/thirdparty-transplant/napi_generator`
- 页面说明：
  - 能从 `.d.ts` 生成 NAPI C++ 代码与测试代码
  - 生成完成度可达 90%+
  - 可生成 `CMakeLists.txt`、`*_napi.cpp`、`napi_init.cpp` 等
- 价值：
  - 非常适合我们未来做：
    - `index.d.ts`
    - `SimulatorFacade.d.ts`
    - 然后自动生成一版 NAPI 桥接骨架
  - 可以显著减少 ArkTS ↔ C++ 桥接的体力活

---

## 四、从开源项目反推出的“鸿蒙化正确姿势”

结合 Mrpoid 的结构和 HarmonyOS 的 NAPI 工具链，当前最靠谱的方案是：

### 路线：内核复用 + 壳层替换 + 桥接重建

#### 1. 复用/迁移 C/C++ 核心
重点研究并尽量复用：
- MRP 解释执行核心
- 事件循环
- 虚拟机/内存模型
- 兼容性补丁逻辑
- 部分渲染与输入抽象

#### 2. 丢掉 Android Java 壳
Android 侧这些东西不要硬搬：
- Activity / SurfaceView 组织方式
- Android 专属输入/权限/生命周期写法
- 旧 JNI 接口形态

这些在鸿蒙要替换成：
- ArkTS 页面
- UIAbility / Stage Model 生命周期
- HarmonyOS 文件/输入/音频能力

#### 3. 用 NAPI 代替 JNI
把 Android 的 JNI 桥接思路换成：
- ArkTS 定义 facade 接口
- NAPI 暴露粗粒度 native 方法
- C++ 内核留在 native 层运行

建议桥接接口从最小集合开始：
- `init()`
- `loadPackage()`
- `start()`
- `pause()`
- `resume()`
- `stop()`
- `sendKey()`
- `sendTouch()`
- `saveState()`
- `loadState()`
- `setOption()`

#### 4. 渲染链路按“先跑通，再优化”推进
从旧 Mrpoid 的历史信息可以看出，它本来就长期在处理：
- Surface 刷新
- 多线程冲突
- 花屏/闪退
- 音乐接口
- 后台返回重启等问题

这说明鸿蒙化时不要想当然：
- 一开始先做 **最小可跑通渲染链路**
- 然后再处理：
  - 帧率
  - 缩放
  - 触摸映射
  - 音频同步
  - 前后台切换

---

## 五、当前最推荐的落地顺序

### Phase 1：研究现有 Mrpoid 结构
优先研究：
- `mrpoid2018/app`
- `mrpoid2018/lib`
- `libMrpoid`

要回答的问题：
- 哪部分是 UI 壳？
- 哪部分是真正内核？
- JNI 接口有哪些？
- 渲染输出怎么组织？
- 音频、输入、文件、线程模型怎么做的？

### Phase 2：设计 HarmonyOS 壳层
在当前仓库先定义：
- ArkTS 页面壳
- 生命周期
- 文件导入
- 运行页容器
- 控制层/按键层

### Phase 3：定义 NAPI 接口草案
先写 `.d.ts` 或接口草案，再决定是否用生成器：
- `SimulatorModule`
- `RuntimeController`
- `InputBridge`
- `StorageBridge`

### Phase 4：做最小 POC
目标不是全兼容，而是：
- 跑起一个样例
- 有画面
- 能输入
- 能退出

---

## 六、对我们项目的直接启发

### 启发 1：不要走“纯 ArkTS 重写模拟器核心”路线
从公开 Mrpoid 结构看，核心明显偏 native。
如果我们现在直接用 ArkTS 重写整个模拟器核心，风险会非常高：
- 性能不稳
- 渲染和音频压力大
- 历史兼容细节难抄

### 启发 2：最该抄的是“结构”，不是“界面”
真正该抄的是：
- 核心/壳层边界
- 渲染/输入/文件/音频抽象
- 兼容补丁机制

不是旧 Android UI 长什么样。

### 启发 3：NAPI 工具链可以明显加速鸿蒙化
我们后面如果先把 ArkTS 接口收敛好，再用：
- `ohos_utils`
- `napi_generator`

去减少桥接代码，会比全手搓靠谱得多。

---

## 七、当前建议

当前建议非常明确：

> **先以 `mrpoid2018 + libMrpoid` 作为内核研究样本，按“ArkTS 壳 + NAPI + C/C++ 核心”路线做鸿蒙化。**

这条路线比“纯 ArkTS 重写”靠谱，也比“照搬 Android 工程”清醒。

---

## 八、下一步可执行动作

1. 把 `mrpoid2018` / `libMrpoid` 拉到本地做结构审查
2. 输出一份：
   - Android 结构 → HarmonyOS 结构映射表
3. 输出一份：
   - JNI 接口 → NAPI 接口草案
4. 在本仓库补：
   - HarmonyOS POC 目录结构
   - NAPI facade 设计文档

如果继续推进，下一步最值的是：
**直接把 `mrpoid2018` 代码拉下来做一次“可迁移性拆解”。**
