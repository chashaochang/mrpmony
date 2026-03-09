#include "MrpFacade.h"

#include "../render/FrameBuffer.h"
#include "../runtime/MrpRuntime.h"

namespace {
MrpRuntime g_runtime;
}

CommonResult MrpFacade::Init(const InitOptions &options)
{
    options_ = options;
    return g_runtime.Init(options.width, options.height, options.debug);
}

CommonResult MrpFacade::LoadPackage(const std::string &packagePath)
{
    packagePath_ = packagePath;
    return g_runtime.LoadPackage(packagePath);
}

CommonResult MrpFacade::Start()
{
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
    DispatchInput(event);
    return CommonResult{true, 0, ""};
}

FrameResult MrpFacade::PullFrame()
{
    FrameBuffer frameBuffer(g_runtime.Width(), g_runtime.Height());
    frameBuffer.FillTestPattern(g_runtime.NextFrameId());

    FrameResult result;
    result.ok = true;
    result.hasNewFrame = true;
    result.width = g_runtime.Width();
    result.height = g_runtime.Height();
    result.pixelFormat = "RGBA_8888";
    result.buffer = frameBuffer.Data();
    result.frameId = g_runtime.CurrentFrameId();
    return result;
}

CommonResult MrpFacade::Release()
{
    packagePath_.clear();
    return g_runtime.Release();
}
