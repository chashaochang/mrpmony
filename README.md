# MRP Simulator HarmonyOS

MRP 模拟器鸿蒙化项目。

当前目标不是直接做“大而全”的正式产品，而是先完成一轮 **HarmonyOS 技术验证 / POC**，验证现有 MRP 模拟器能力是否能在鸿蒙侧形成稳定闭环。

## 当前阶段

处于 **第一阶段：方案收敛 + 技术验证准备**。

当前共识：
- 一期按 **POC / Alpha** 推进
- 先做 **单实例、本地导入、可启动、可渲染、可输入、可退出、可存档**
- 推荐路线：
  - **ArkTS 做应用壳/UI**
  - **C++ 做模拟器核心**
  - **NAPI 做粗粒度桥接**
- 服务端/BFF 暂不进入运行时闭环，只在后续按需参与外围能力

## 目录说明

```text
.
├─ docs/
│  ├─ project/
│  ├─ product/
│  ├─ architecture/
│  ├─ harmonyos/
│  └─ qa/
├─ research/
├─ prototypes/
└─ scripts/
```

## 当前文档

- 项目链路：`docs/project/AGENT_CHAIN.md`
- 项目概览：`docs/project/PROJECT_OVERVIEW.md`
- 一期范围：`docs/product/MVP_SCOPE.md`
- 第一轮架构判断：`docs/architecture/ARCHITECTURE_V1.md`
- 鸿蒙技术路线：`docs/harmonyos/HARMONYOS_TECH_ROUTE_V1.md`
- 下一步行动项：`docs/project/NEXT_ACTIONS.md`

## 下一步

1. 明确现有 MRP 模拟器代码资产与语言栈
2. 识别可复用核心与必须重写部分
3. 建立 HarmonyOS POC 工程骨架
4. 选定首批样例集并形成验证基线

## 状态说明

当前仓库以 **方案与技术验证文档** 为主，代码实现将在明确现有内核资产后逐步落地。
