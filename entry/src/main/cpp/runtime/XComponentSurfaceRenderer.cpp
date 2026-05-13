#include "XComponentSurfaceRenderer.h"

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <hilog/log.h>
#include <native_buffer/buffer_common.h>
#include <native_window/external_window.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <mutex>

namespace {
std::mutex g_surfaceMutex;
OHNativeWindow *g_window = nullptr;
uint64_t g_surfaceWidth = 0;
uint64_t g_surfaceHeight = 0;
int64_t g_renderCount = 0;
int64_t g_surfaceGeneration = 0;

OH_NativeXComponent_Callback g_callback = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
};

void ConfigureWindowLocked(int32_t width, int32_t height)
{
    if (g_window == nullptr) {
        return;
    }
    (void)OH_NativeWindow_NativeWindowHandleOpt(g_window, SET_BUFFER_GEOMETRY, width, height);
    (void)OH_NativeWindow_NativeWindowHandleOpt(g_window, SET_FORMAT, NATIVEBUFFER_PIXEL_FMT_BGRA_8888);
    (void)OH_NativeWindow_NativeWindowHandleOpt(g_window, SET_SWAP_INTERVAL, 1);
}

void OnSurfaceCreated(OH_NativeXComponent *component, void *window)
{
    if (component == nullptr || window == nullptr) {
        return;
    }
    uint64_t width = 0;
    uint64_t height = 0;
    (void)OH_NativeXComponent_GetXComponentSize(component, window, &width, &height);
    {
        std::lock_guard<std::mutex> lock(g_surfaceMutex);
        g_window = static_cast<OHNativeWindow *>(window);
        g_surfaceWidth = width;
        g_surfaceHeight = height;
        g_renderCount = 0;
        ++g_surfaceGeneration;
        (void)OH_NativeWindow_NativeObjectReference(g_window);
        ConfigureWindowLocked(240, 320);
    }
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                 "XComponent surface created %{public}llux%{public}llu",
                 static_cast<unsigned long long>(width), static_cast<unsigned long long>(height));
}

void OnSurfaceChanged(OH_NativeXComponent *component, void *window)
{
    if (component == nullptr || window == nullptr) {
        return;
    }
    uint64_t width = 0;
    uint64_t height = 0;
    (void)OH_NativeXComponent_GetXComponentSize(component, window, &width, &height);
    std::lock_guard<std::mutex> lock(g_surfaceMutex);
    g_surfaceWidth = width;
    g_surfaceHeight = height;
    ++g_surfaceGeneration;
    ConfigureWindowLocked(240, 320);
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                 "XComponent surface changed %{public}llux%{public}llu",
                 static_cast<unsigned long long>(width), static_cast<unsigned long long>(height));
}

void OnSurfaceDestroyed(OH_NativeXComponent *component, void *window)
{
    (void)component;
    (void)window;
    std::lock_guard<std::mutex> lock(g_surfaceMutex);
    if (g_window != nullptr) {
        (void)OH_NativeWindow_NativeObjectUnreference(g_window);
        g_window = nullptr;
    }
    g_surfaceWidth = 0;
    g_surfaceHeight = 0;
    ++g_surfaceGeneration;
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP", "XComponent surface destroyed");
}
} // namespace

void MrpRegisterXComponent(napi_env env, napi_value exports)
{
    bool hasXComponent = false;
    if (napi_has_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &hasXComponent) != napi_ok || !hasXComponent) {
        return;
    }

    napi_value exportInstance = nullptr;
    if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance) != napi_ok) {
        return;
    }

    OH_NativeXComponent *nativeXComponent = nullptr;
    if (napi_unwrap(env, exportInstance, reinterpret_cast<void **>(&nativeXComponent)) != napi_ok ||
        nativeXComponent == nullptr) {
        return;
    }

    char id[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
    uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
    (void)OH_NativeXComponent_GetXComponentId(nativeXComponent, id, &idSize);

    g_callback.OnSurfaceCreated = OnSurfaceCreated;
    g_callback.OnSurfaceChanged = OnSurfaceChanged;
    g_callback.OnSurfaceDestroyed = OnSurfaceDestroyed;
    g_callback.DispatchTouchEvent = nullptr;
    (void)OH_NativeXComponent_RegisterCallback(nativeXComponent, &g_callback);
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP", "XComponent registered id=%{public}s", id);
}

bool MrpRenderFrameToXComponent(const std::vector<uint8_t> &bgra, int32_t width, int32_t height)
{
    std::lock_guard<std::mutex> lock(g_surfaceMutex);
    if (g_window == nullptr || bgra.empty() || width <= 0 || height <= 0) {
        return false;
    }

    uint64_t surfaceId = 0;
    int32_t ret = OH_NativeWindow_GetSurfaceId(g_window, &surfaceId);
    if (ret != 0 || surfaceId == 0) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, 0xFF00, "MRP", "get XComponent surfaceId failed ret=%{public}d", ret);
        return false;
    }

    OHNativeWindow *renderWindow = nullptr;
    ret = OH_NativeWindow_CreateNativeWindowFromSurfaceId(surfaceId, &renderWindow);
    if (ret != 0 || renderWindow == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, 0xFF00, "MRP", "create render window failed ret=%{public}d", ret);
        return false;
    }

    (void)OH_NativeWindow_NativeWindowHandleOpt(renderWindow, SET_BUFFER_GEOMETRY, width, height);
    (void)OH_NativeWindow_NativeWindowHandleOpt(renderWindow, SET_FORMAT, NATIVEBUFFER_PIXEL_FMT_BGRA_8888);
    (void)OH_NativeWindow_NativeWindowHandleOpt(renderWindow, SET_SWAP_INTERVAL, 1);

    OHNativeWindowBuffer *windowBuffer = nullptr;
    int fenceFd = -1;
    ret = OH_NativeWindow_NativeWindowRequestBuffer(renderWindow, &windowBuffer, &fenceFd);
    if (ret != 0 || windowBuffer == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, 0xFF00, "MRP", "request XComponent buffer failed ret=%{public}d", ret);
        if (fenceFd >= 0) {
            close(fenceFd);
        }
        OH_NativeWindow_DestroyNativeWindow(renderWindow);
        return false;
    }

    BufferHandle *handle = OH_NativeWindow_GetBufferHandleFromNative(windowBuffer);
    if (handle == nullptr || handle->size <= 0) {
        (void)OH_NativeWindow_NativeWindowAbortBuffer(renderWindow, windowBuffer);
        if (fenceFd >= 0) {
            close(fenceFd);
        }
        OH_NativeWindow_DestroyNativeWindow(renderWindow);
        return false;
    }

    void *mapped = handle->virAddr;
    bool didMap = false;
    if ((mapped == nullptr || mapped == MAP_FAILED) && handle->fd >= 0) {
        mapped = mmap(nullptr, static_cast<size_t>(handle->size), PROT_READ | PROT_WRITE, MAP_SHARED, handle->fd, 0);
        didMap = mapped != MAP_FAILED;
    }
    if (mapped == nullptr || mapped == MAP_FAILED) {
        (void)OH_NativeWindow_NativeWindowAbortBuffer(renderWindow, windowBuffer);
        if (fenceFd >= 0) {
            close(fenceFd);
        }
        OH_NativeWindow_DestroyNativeWindow(renderWindow);
        return false;
    }

    const int32_t dstWidth = std::max(0, handle->width);
    const int32_t dstHeight = std::max(0, handle->height);
    const size_t rowBytes = handle->stride > 0 ? static_cast<size_t>(handle->stride)
                                               : static_cast<size_t>(dstWidth) * 4U;
    auto *dst = static_cast<uint8_t *>(mapped);
    const size_t bufferSize = static_cast<size_t>(handle->size);
    std::memset(dst, 0, bufferSize);

    const int32_t copyWidth = std::min(width, dstWidth);
    const int32_t copyHeight = std::min(height, dstHeight);
    const size_t srcRowBytes = static_cast<size_t>(copyWidth) * 4U;
    const size_t copyBytes = std::min(srcRowBytes, rowBytes);
    for (int32_t y = 0; y < copyHeight; ++y) {
        const size_t dstOffset = static_cast<size_t>(y) * rowBytes;
        if (dstOffset >= bufferSize) {
            break;
        }
        const size_t remaining = bufferSize - dstOffset;
        const size_t safeCopyBytes = std::min(copyBytes, remaining);
        if (safeCopyBytes == 0) {
            break;
        }
        const uint8_t *srcRow = bgra.data() + static_cast<size_t>(y * width) * 4U;
        uint8_t *dstRow = dst + dstOffset;
        std::memcpy(dstRow, srcRow, safeCopyBytes);
    }

    Region region = {nullptr, 0};
    ret = OH_NativeWindow_NativeWindowFlushBuffer(renderWindow, windowBuffer, fenceFd, region);
    if (didMap) {
        (void)munmap(mapped, static_cast<size_t>(handle->size));
    }
    if (ret != 0) {
        (void)OH_NativeWindow_NativeWindowAbortBuffer(renderWindow, windowBuffer);
        OH_NativeWindow_DestroyNativeWindow(renderWindow);
        OH_LOG_Print(LOG_APP, LOG_ERROR, 0xFF00, "MRP", "flush XComponent buffer failed ret=%{public}d", ret);
        return false;
    }

    OH_NativeWindow_DestroyNativeWindow(renderWindow);

    ++g_renderCount;
    if (g_renderCount <= 3 || (g_renderCount % 60) == 0) {
        OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                     "render XComponent #%{public}lld src=%{public}dx%{public}d dst=%{public}dx%{public}d strideBytes=%{public}zu size=%{public}d copyBytes=%{public}zu",
                     static_cast<long long>(g_renderCount), width, height, dstWidth, dstHeight, rowBytes, handle->size,
                     copyBytes);
    }
    return true;
}

int64_t MrpXComponentSurfaceGeneration()
{
    std::lock_guard<std::mutex> lock(g_surfaceMutex);
    return g_window == nullptr ? 0 : g_surfaceGeneration;
}
