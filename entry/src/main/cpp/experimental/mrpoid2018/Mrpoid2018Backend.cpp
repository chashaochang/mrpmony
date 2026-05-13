#include "Mrpoid2018Backend.h"

#include <hilog/log.h>

namespace {
constexpr int32_t kNotImplemented = 9001;
}

CommonResult Mrpoid2018Backend::Init(const std::string &workDir, int32_t width, int32_t height, bool debug)
{
    workDir_ = workDir;
    width_ = width > 0 ? width : 240;
    height_ = height > 0 ? height : 320;
    debug_ = debug;
    packagePath_.clear();
    state_ = State::Initialized;
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP2018", "Init experimental backend workDir=%{public}s size=%{public}dx%{public}d",
                 workDir_.c_str(), width_, height_);
    return CommonResult{true, 0, ""};
}

CommonResult Mrpoid2018Backend::LoadPackage(const std::string &packagePath)
{
    if (state_ != State::Initialized && state_ != State::Released) {
        return CommonResult{false, 9002, "mrpoid2018 backend not initialized"};
    }
    packagePath_ = packagePath;
    state_ = State::Loaded;
    return CommonResult{true, 0, ""};
}

CommonResult Mrpoid2018Backend::Start()
{
    if (state_ != State::Loaded && state_ != State::Paused) {
        return CommonResult{false, 9003, "mrpoid2018 backend not loaded"};
    }
    return NotImplemented("Start");
}

CommonResult Mrpoid2018Backend::Pause()
{
    if (state_ != State::Running) {
        return CommonResult{false, 9004, "mrpoid2018 backend not running"};
    }
    state_ = State::Paused;
    return CommonResult{true, 0, ""};
}

CommonResult Mrpoid2018Backend::Resume()
{
    if (state_ != State::Paused) {
        return CommonResult{false, 9005, "mrpoid2018 backend not paused"};
    }
    state_ = State::Running;
    return CommonResult{true, 0, ""};
}

CommonResult Mrpoid2018Backend::SendInput(const MrpInputEvent &event)
{
    (void)event;
    return NotImplemented("SendInput");
}

FrameResult Mrpoid2018Backend::PullFrame()
{
    FrameResult result;
    result.ok = false;
    result.errorCode = kNotImplemented;
    result.errorMessage = "mrpoid2018 backend PullFrame is not implemented";
    return result;
}

EditRequestResult Mrpoid2018Backend::PullEditRequest()
{
    EditRequestResult result;
    result.ok = true;
    result.hasRequest = false;
    return result;
}

CommonResult Mrpoid2018Backend::SubmitEditResult(uint64_t requestId, bool confirmed, const std::string &text)
{
    (void)requestId;
    (void)confirmed;
    (void)text;
    return NotImplemented("SubmitEditResult");
}

CommonResult Mrpoid2018Backend::Release()
{
    packagePath_.clear();
    state_ = State::Released;
    return CommonResult{true, 0, ""};
}

CommonResult Mrpoid2018Backend::NotImplemented(const std::string &operation) const
{
    return CommonResult{false, kNotImplemented, "mrpoid2018 backend " + operation + " is not implemented"};
}
