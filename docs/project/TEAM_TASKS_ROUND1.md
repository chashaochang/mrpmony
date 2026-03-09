# 开发团队第一轮任务单

项目：MRP 模拟器鸿蒙化
日期：2026-03-09

## 目标

围绕“MRP 模拟器鸿蒙化”进入第一轮开发团队收敛，输出能直接支撑 POC 的技术产物。

## 总原则
- 一期按 POC / Alpha 推进
- 先做最小闭环，不做大而全
- 优先路线：ArkTS 壳 + NAPI + C/C++ 核心
- 涉及鸿蒙平台能力与 NAPI 时优先查官方文档

## 角色任务

### 1. Architect
输出：
- Android/Mrpoid 结构 → HarmonyOS 结构映射
- 模块边界图
- JNI → NAPI 桥接映射策略
- 线程模型与运行时边界建议

### 2. HarmonyOS Senior Engineer (`harmonyos-dev`)
输出：
- HarmonyOS POC 工程结构建议
- ArkTS 与 Native 的边界
- NAPI facade 接口草案
- 官方文档检查清单
- 首批技术风险与绕法

### 3. RD Frontend
输出：
- ArkTS 壳层页面结构
- 运行页/导入页/设置页的页面流
- 组件拆分建议
- 状态管理草案

### 4. Backend Dev
输出：
- 一期是否需要 BFF / 服务端支持的判断
- 若不需要，明确边界
- 若需要，列最小 API / 配置 / 日志回传方案

### 5. RD QA
输出：
- 首批样例集选择策略
- 功能验证清单
- 兼容矩阵模板
- 性能/稳定性专项检查项

## 当前预期产物
- `docs/architecture/ANDROID_TO_HARMONY_MAPPING.md`
- `docs/harmonyos/NAPI_FACADE_DRAFT.md`
- `docs/harmonyos/POC_STRUCTURE_DRAFT.md`
- `docs/product/APP_SHELL_FLOW.md`
- `docs/qa/COMPATIBILITY_BASELINE.md`

## 当前阻塞
- 还未把现有 Mrpoid / libMrpoid 代码正式接入本仓库
- 需尽快完成可迁移性拆解
