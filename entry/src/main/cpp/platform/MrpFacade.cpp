#include "MrpFacade.h"

#include <hilog/log.h>
#include <unordered_set>

#include "../render/FrameBuffer.h"
#include "../runtime/MrpAudioBridge.h"
#include "../runtime/MrpRuntime.h"
#include "../runtime/VmrpHostBridge.h"
#include "../runtime/XComponentSurfaceRenderer.h"

#include <vector>

extern "C" {
#include "header/types.h"
#include "header/vmrp.h"
}

namespace {
MrpRuntime g_runtime;
int64_t g_inputCount = 0;
std::unordered_set<int32_t> g_pressedKeys;
}

CommonResult MrpFacade::Init(const InitOptions &options)
{
    options_ = options;
    lastFrameId_ = 0;
    lastRenderedSurfaceGeneration_ = 0;
    return g_runtime.Init(options.workDir, options.width, options.height, options.debug);
}

CommonResult MrpFacade::LoadPackage(const std::string &packagePath)
{
    packagePath_ = packagePath;
    g_pressedKeys.clear();
    return g_runtime.LoadPackage(packagePath);
}

CommonResult MrpFacade::Start()
{
    lastFrameId_ = 0;
    lastRenderedSurfaceGeneration_ = 0;
    g_pressedKeys.clear();
    return g_runtime.Start();
}

CommonResult MrpFacade::Pause()
{
    return g_runtime.Pause();
}

CommonResult MrpFacade::Resume()
{
    return g_runtime.Resume();
}

CommonResult MrpFacade::SendInput(const MrpInputEvent &event)
{
    if (!g_runtime.IsRunning()) {
        return CommonResult{false, 1003, "runtime not running"};
    }
    DispatchInput(event);
    ++g_inputCount;
    const bool shouldLog = g_inputCount <= 20 || (g_inputCount % 50) == 0;
    if (event.type == "touch") {
        int32_t code = MR_MOUSE_MOVE;
        if (event.action == "down") {
            code = MR_MOUSE_DOWN;
        } else if (event.action == "up") {
            code = MR_MOUSE_UP;
        }
        const int32_t ret = ::event(code, event.x, event.y);
        if (shouldLog) {
            OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                         "input #%{public}lld touch action=%{public}s x=%{public}d y=%{public}d ret=%{public}d",
                         static_cast<long long>(g_inputCount), event.action.c_str(), event.x, event.y, ret);
        }
    } else {
        if (event.action == "tap") {
            const int32_t downRet = ::event(MR_KEY_PRESS, event.keyCode, 0);
            const int32_t upRet = ::event(MR_KEY_RELEASE, event.keyCode, 0);
            g_pressedKeys.erase(event.keyCode);
            if (shouldLog) {
                OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                             "input #%{public}lld key action=tap code=%{public}d down=%{public}d up=%{public}d",
                             static_cast<long long>(g_inputCount), event.keyCode, downRet, upRet);
            }
            return CommonResult{true, 0, ""};
        }

        if (event.action == "down") {
            if (g_pressedKeys.find(event.keyCode) != g_pressedKeys.end()) {
                if (shouldLog) {
                    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                                 "input #%{public}lld key action=%{public}s code=%{public}d skipped-duplicate-down",
                                 static_cast<long long>(g_inputCount), event.action.c_str(), event.keyCode);
                }
                return CommonResult{true, 0, ""};
            }
            g_pressedKeys.insert(event.keyCode);
            const int32_t ret = ::event(MR_KEY_PRESS, event.keyCode, 0);
            if (shouldLog) {
                OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                             "input #%{public}lld key action=%{public}s code=%{public}d ret=%{public}d",
                             static_cast<long long>(g_inputCount), event.action.c_str(), event.keyCode, ret);
            }
            return CommonResult{true, 0, ""};
        }
        if (event.action == "up") {
            if (g_pressedKeys.find(event.keyCode) == g_pressedKeys.end()) {
                if (shouldLog) {
                    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                                 "input #%{public}lld key action=%{public}s code=%{public}d skipped-stray-up",
                                 static_cast<long long>(g_inputCount), event.action.c_str(), event.keyCode);
                }
                return CommonResult{true, 0, ""};
            }
            g_pressedKeys.erase(event.keyCode);
            const int32_t ret = ::event(MR_KEY_RELEASE, event.keyCode, 0);
            if (shouldLog) {
                OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                             "input #%{public}lld key action=%{public}s code=%{public}d ret=%{public}d",
                             static_cast<long long>(g_inputCount), event.action.c_str(), event.keyCode, ret);
            }
            return CommonResult{true, 0, ""};
        }

        const int32_t ret = ::event(MR_KEY_PRESS, event.keyCode, 0);
        if (shouldLog) {
            OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                         "input #%{public}lld key action=%{public}s code=%{public}d normalized-to-down ret=%{public}d",
                         static_cast<long long>(g_inputCount), event.action.c_str(), event.keyCode, ret);
        }
    }
    return CommonResult{true, 0, ""};
}

FrameResult MrpFacade::PullFrame()
{
    FrameResult result;
    if (g_runtime.IsRunning()) {
        (void)g_runtime.PumpTimer(lastFrameId_ > 0 ? 1 : 16);
    }
    int32_t width = g_runtime.Width();
    int32_t height = g_runtime.Height();
    int64_t frameId = VmrpHostFrameId();
    std::vector<uint8_t> buffer;
    const int64_t surfaceGeneration = MrpXComponentSurfaceGeneration();
    const bool hasNewFrame = (frameId > 0) && (frameId != lastFrameId_);
    const bool needsSurfaceRedraw = (frameId > 0) && (surfaceGeneration > 0) &&
        (surfaceGeneration != lastRenderedSurfaceGeneration_);
    if (hasNewFrame || needsSurfaceRedraw) {
        const bool copied = VmrpHostCopyFrame(buffer, width, height, frameId);
        if (copied) {
            const bool rendered = MrpRenderFrameToXComponent(buffer, width, height);
            // Only consume frameId after a successful surface render.
            // Otherwise the first frame can be lost before XComponent is ready,
            // causing "black until next input-generated frame".
            if (rendered) {
                lastFrameId_ = frameId;
                lastRenderedSurfaceGeneration_ = surfaceGeneration;
            }
        }
    }

    result.ok = true;
    result.hasNewFrame = hasNewFrame || needsSurfaceRedraw;
    result.exited = VmrpHostExited();
    result.width = width;
    result.height = height;
    result.pixelFormat = "BGRA_8888";
    result.buffer = buffer;
    result.frameId = frameId;
    return result;
}

EditRequestResult MrpFacade::PullEditRequest()
{
    EditRequestResult result;
    VmrpEditRequest req;
    if (!VmrpHostGetPendingEditRequest(req)) {
        result.ok = false;
        result.errorCode = 2001;
        result.errorMessage = "read edit request failed";
        return result;
    }

    result.ok = true;
    result.hasRequest = req.pending;
    result.requestId = req.requestId;
    result.handle = req.handle;
    result.type = req.type;
    result.maxSize = req.maxSize;
    result.title = req.title;
    result.text = req.text;
    return result;
}

CommonResult MrpFacade::SubmitEditResult(uint64_t requestId, bool confirmed, const std::string &text)
{
    if (!VmrpHostResolveEditRequest(requestId, confirmed, text)) {
        return CommonResult{false, 2002, "stale or missing edit request"};
    }

    if (!g_runtime.IsRunning()) {
        return CommonResult{false, 1003, "runtime not running"};
    }

    VmrpEditRequest req;
    (void)VmrpHostGetPendingEditRequest(req);
    const int32_t dialogHandle = req.handle;
    const int32_t dialogKey = confirmed ? 0 : 1; // MR_DIALOG_KEY_OK / MR_DIALOG_KEY_CANCEL

    int32_t ret = ::event(MR_DIALOG_EVENT, dialogKey, 0);
    if (ret == MR_FAILED) {
        // Some runtimes expect dialog handle in param2.
        ret = ::event(MR_DIALOG_EVENT, dialogKey, dialogHandle);
    }
    if (ret == MR_FAILED) {
        // Some packages use reversed params for legacy reasons.
        ret = ::event(MR_DIALOG_EVENT, dialogHandle, dialogKey);
    }
    if (ret == MR_FAILED) {
        // Some packages don't accept MR_DIALOG_EVENT but do react to keypad
        // confirmation/cancel keys in their custom input flow.
        const int32_t fallbackKey = confirmed ? MR_KEY_SELECT : MR_KEY_SOFTRIGHT;
        const int32_t downRet = ::event(MR_KEY_PRESS, fallbackKey, 0);
        const int32_t upRet = ::event(MR_KEY_RELEASE, fallbackKey, 0);
        if (downRet == MR_SUCCESS || upRet == MR_SUCCESS) {
            ret = MR_SUCCESS;
        }
        OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                     "submitEditResult fallback key=%{public}d handle=%{public}d down=%{public}d up=%{public}d",
                     fallbackKey, dialogHandle, downRet, upRet);
    }
    if (ret != MR_SUCCESS) {
        return CommonResult{false, ret, "deliver edit confirm event failed"};
    }
    (void)VmrpHostCompleteEditRequest(requestId);
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                 "submitEditResult request=%{public}llu handle=%{public}d confirmed=%{public}d ret=%{public}d",
                 static_cast<unsigned long long>(requestId), dialogHandle, confirmed ? 1 : 0, ret);
    return CommonResult{true, 0, ""};
}

CommonResult MrpFacade::Release()
{
    packagePath_.clear();
    lastFrameId_ = 0;
    lastRenderedSurfaceGeneration_ = 0;
    g_pressedKeys.clear();
    vmrpHostReleaseAudio();
    const int32_t width = g_runtime.Width();
    const int32_t height = g_runtime.Height();
    CommonResult result = g_runtime.Release();
    if (result.ok && width > 0 && height > 0) {
        const size_t bufferSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4U;
        std::vector<uint8_t> black(bufferSize, 0);
        (void)MrpRenderFrameToXComponent(black, width, height);
    }
    return result;
}
