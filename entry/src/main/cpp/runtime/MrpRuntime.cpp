#include "MrpRuntime.h"

CommonResult MrpRuntime::Init(int32_t width, int32_t height, bool debug)
{
    width_ = width > 0 ? width : 240;
    height_ = height > 0 ? height : 320;
    debug_ = debug;
    frameId_ = 0;
    state_ = State::Initialized;
    return CommonResult{true, 0, ""};
}

CommonResult MrpRuntime::LoadPackage(const std::string &packagePath)
{
    if (state_ != State::Initialized && state_ != State::Released) {
        return CommonResult{false, 1001, "runtime not initialized"};
    }
    (void)packagePath;
    state_ = State::Loaded;
    return CommonResult{true, 0, ""};
}

CommonResult MrpRuntime::Start()
{
    if (state_ != State::Loaded && state_ != State::Paused) {
        return CommonResult{false, 1002, "runtime not ready to start"};
    }
    state_ = State::Running;
    return CommonResult{true, 0, ""};
}

CommonResult MrpRuntime::Pause()
{
    if (state_ != State::Running) {
        return CommonResult{false, 1003, "runtime not running"};
    }
    state_ = State::Paused;
    return CommonResult{true, 0, ""};
}

CommonResult MrpRuntime::Resume()
{
    if (state_ != State::Paused) {
        return CommonResult{false, 1004, "runtime not paused"};
    }
    state_ = State::Running;
    return CommonResult{true, 0, ""};
}

CommonResult MrpRuntime::Release()
{
    state_ = State::Released;
    frameId_ = 0;
    return CommonResult{true, 0, ""};
}

int32_t MrpRuntime::Width() const
{
    return width_;
}

int32_t MrpRuntime::Height() const
{
    return height_;
}

int64_t MrpRuntime::NextFrameId()
{
    return ++frameId_;
}

int64_t MrpRuntime::CurrentFrameId() const
{
    return frameId_;
}
