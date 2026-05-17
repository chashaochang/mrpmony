# Unicorn 说明

仓库里已经带了 Unicorn 的头文件和许可证文件。

当前运行时默认链接这个动态库：

- `entry/src/main/libs/arm64-v8a/libunicorn.so.2`

但这不是写死的，你可以换成自己编译的版本。

## 改用你自己的 libunicorn.so.2

`CMakeLists.txt` 里支持自定义路径变量：

```bash
-DUNICORN_SHARED_LIB_PATH=/绝对路径/libunicorn.so.2
```

如果不传，默认使用：

```text
entry/src/main/libs/arm64-v8a/libunicorn.so.2
```

## 自己编译 OHOS 版 Unicorn

注意，这里不是直接拿 Unicorn 上游源码就一定能编通。

我们本地这份 `unicorn-src-proxy` 对 `CMakeLists.txt` 做过 OHOS 支持补丁，
核心就是补了 `CMAKE_SYSTEM_NAME STREQUAL "OHOS"` 这段架构判断。

也就是说，别人如果要自己重编，必须满足下面两种情况之一：

1. 直接使用我们这份改过的 `unicorn-src-proxy`
2. 自己在上游 Unicorn 的 `CMakeLists.txt` 里手动补上 OHOS 分支

补丁核心大致如下：

```cmake
elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    if(OHOS_ARCH STREQUAL "arm64-v8a")
        set(UNICORN_TARGET_ARCH "aarch64")
    elseif(OHOS_ARCH STREQUAL "armeabi-v7a")
        set(UNICORN_TARGET_ARCH "arm")
        set(ATOMIC_LINKAGE_FIX TRUE)
    elseif(OHOS_ARCH STREQUAL "x86_64")
        set(UNICORN_TARGET_ARCH "i386")
    else()
        message(FATAL_ERROR "Unsupported OHOS_ARCH for Unicorn: ${OHOS_ARCH}")
    endif()
```

如果你本地有这份已经打过补丁的 `unicorn-src-proxy` 源码，可以直接自己编。

示例：

```bash
cmake -S /path/to/unicorn-src-proxy -B /path/to/unicorn-build-ohos-arm64-armonly \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=ON \
  -DUNICORN_ARCH=arm \
  -DCMAKE_TOOLCHAIN_FILE=/Applications/DevEco-Studio.app/Contents/sdk/default/openharmony/native/build/cmake/ohos.toolchain.cmake \
  -DOHOS_ARCH=arm64-v8a \
  -DOHOS_PLATFORM_LEVEL=12
```

然后执行：

```bash
cmake --build /path/to/unicorn-build-ohos-arm64-armonly
```

正常情况下生成文件在这里：

```text
/path/to/unicorn-build-ohos-arm64-armonly/libunicorn.so.2
```

## 两种使用方式

### 方式 1：直接替换仓库里的库

```bash
cp /path/to/unicorn-build-ohos-arm64-armonly/libunicorn.so.2 \
  entry/src/main/libs/arm64-v8a/libunicorn.so.2
```

### 方式 2：不替换仓库文件，构建时指定路径

```bash
cmake ... -DUNICORN_SHARED_LIB_PATH=/path/to/unicorn-build-ohos-arm64-armonly/libunicorn.so.2
```

第二种更干净，适合本地调试。
