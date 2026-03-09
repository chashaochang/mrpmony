# 主推进连续施工 Backlog

> 目标：让当前主推进人进入“连续施工模式”，做完一个自动继续下一个，除非遇到明确阻塞，不再停在“等催办”。

## 当前任务
MRP 模拟器鸿蒙化

## 主推进人
- `harmonyos-dev`
- `rd-frontend`

---

## HarmonyOS Senior Engineer Backlog

### HOS-001 POC 文件级骨架清单
交付：
- ArkTS 侧骨架文件清单
- Native/C++ 侧骨架文件清单
- 模块关系说明

### HOS-002 最小 NAPI Facade v0
交付：
- 方法清单
- 参数/返回值
- ArkTS ↔ Native 对齐说明
- 哪些先占位、哪些一期必须落地

### HOS-003 第一周技术断点与验证顺序
交付：
- 第一周最先验证的 3 个技术断点
- 每个断点的通过/失败判据
- 明确什么情况需要拉 architect/QA 介入

规则：
- 完成一个自动进入下一个
- 不因接口命名未冻结而等待
- 如缺实际内核接口，先交可落地占位版

---

## RD Frontend Backlog

### FE-001 ArkTS 壳层目录结构最终版
交付：
- `pages / components / store / services / adapters / models` 目录树
- 页面职责分工

### FE-002 页面-Store-Service-Adapter 契约
交付：
- AppListPage / ImportPage / RunPage 的输入输出边界
- AppCatalogStore / ImportStore / RuntimeSessionStore 最小接口
- AppService / ImportService / RuntimeService 最小接口

### FE-003 页面与 Native 边界清单
交付：
- 哪些逻辑可以留在 ArkTS
- 哪些逻辑必须走 Adapter / Native
- 页面层禁止下沉的内容清单

规则：
- 完成一个自动进入下一个
- 不等待 native 最终命名冻结
- 先交壳层占位契约版

---

## 支援角色
- `architect`：仅在 HOS-003 发现技术断点需要架构拍板时介入
- `backend-dev`：当前待命
- `rd-qa`：等 HOS/FE 交付第一版骨架与契约后，接验证模板对齐
