# mrpmony

这是一个 HarmonyOS 上的 MRP 模拟器工程。

## 说明

本项目当前仓库中的代码、文档、配置整理与实现内容，均由 AI 生成。

当前仓库包含：

- HarmonyOS / DevEco 工程壳
- ArkTS 页面、应用列表、导入页、设置页、运行页
- Native NAPI 桥接和宿主侧渲染 / 输入逻辑
- 已集成到仓库内的三方模块：
  - `entry/src/main/cpp/third_party/vmrp-core`
  - `entry/src/main/cpp/third_party/unicorn`
- 应用商店截图：
  - `screenshots/store/`

当前仓库不包含：

- 你的本机 `local.properties`
- 真实签名文件、证书、密码
- 任何机器私有路径配置

## 本地初始化

先复制两个模板文件：

```bash
cp local.properties.example local.properties
cp build-profile.json5.example build-profile.json5
```

然后做这几件事：

1. 在 `local.properties` 里填你本机的 `hwsdk.dir`
2. 在 `build-profile.json5` 里填你自己的 debug / release 签名配置
3. 用 DevEco Studio 打开仓库根目录

## Native 依赖说明

### vMRP

当前运行时直接使用仓库内模块：

- `entry/src/main/cpp/third_party/vmrp-core`

上游参考项目：

- https://github.com/zengming00/vmrp

### Unicorn

仓库内已经带了：

- 头文件：`entry/src/main/cpp/third_party/unicorn/include`
- 默认动态库：`entry/src/main/libs/arm64-v8a/libunicorn.so.2`

如果你不想用仓库里现成的 `libunicorn.so.2`，可以自己重新编译，再替换或改路径。
但要注意：OHOS 编译支持不是 Unicorn 上游天然带的，当前依赖的是我们本地
打过补丁的 `unicorn-src-proxy/CMakeLists.txt`。

上游项目：

- https://github.com/unicorn-engine/unicorn

具体看这里：

- `entry/src/main/cpp/third_party/unicorn/README.md`

## 开源说明

看这两个文件：

- `THIRD_PARTY_NOTICES.md`
- `OPEN_SOURCE_RELEASE_STATUS.md`

## 目前状态

当前代码已经能正常编译通过 native 和 ArkTS 部分。

如果打包失败，通常是因为你本地还没补签名文件，不是代码本身有问题。
