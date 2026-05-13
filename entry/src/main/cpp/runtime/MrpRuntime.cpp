#include "MrpRuntime.h"

#include <cerrno>
#include <cstring>
#include <hilog/log.h>
#include <unistd.h>

#include <string>

#include "VmrpHostBridge.h"

extern "C" {
#include "header/types.h"
#include "header/vmrp.h"
}

namespace {
std::string Basename(const std::string &path)
{
    const size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos) {
        return path;
    }
    return path.substr(slash + 1U);
}

bool EndsWith(const std::string &value, const std::string &suffix)
{
    return value.size() >= suffix.size() &&
        value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool CanRead(const std::string &path)
{
    return access(path.c_str(), R_OK) == 0;
}

std::string ErrnoMessage(const std::string &action, const std::string &path)
{
    return action + " failed: " + path + ": " + std::strerror(errno);
}
} // namespace

CommonResult MrpRuntime::Init(const std::string &workDir, int32_t width, int32_t height, bool debug)
{
    width_ = width > 0 ? width : 240;
    height_ = height > 0 ? height : 320;
    debug_ = debug;
    workDir_ = workDir;
    frameId_ = 0;
    packageName_ = "dsm_gm.mrp";
    extName_ = "start.mr";
    VmrpHostReset(width_, height_);
    state_ = State::Initialized;
    return CommonResult{true, 0, ""};
}

CommonResult MrpRuntime::LoadPackage(const std::string &packagePath)
{
    if (state_ != State::Initialized && state_ != State::Released) {
        return CommonResult{false, 1001, "runtime not initialized"};
    }
    const std::string name = Basename(packagePath);
    if (EndsWith(name, ".mrp")) {
        packageName_ = name;
    } else if (!name.empty() && name != "demo.imported.001") {
        packageName_ = name;
    } else {
        packageName_ = "dsm_gm.mrp";
    }
    state_ = State::Loaded;
    return CommonResult{true, 0, ""};
}

CommonResult MrpRuntime::Start()
{
    if (state_ != State::Loaded && state_ != State::Paused) {
        return CommonResult{false, 1002, "runtime not ready to start"};
    }
    if (workDir_.empty()) {
        return CommonResult{false, 1005, "workDir is empty"};
    }
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP", "Start workDir=%{public}s package=%{public}s ext=%{public}s",
                 workDir_.c_str(), packageName_.c_str(), extName_.c_str());
    if (chdir(workDir_.c_str()) != 0) {
        return CommonResult{false, 1006, ErrnoMessage("chdir", workDir_)};
    }
    if (!CanRead("cfunction.ext")) {
        return CommonResult{false, 1008, ErrnoMessage("missing cfunction.ext", workDir_ + "/cfunction.ext")};
    }
    if (!CanRead("mythroad/" + packageName_)) {
        return CommonResult{false, 1009, ErrnoMessage("missing package", workDir_ + "/mythroad/" + packageName_)};
    }
    if (!CanRead("mythroad/system/gb12.uc2")) {
        return CommonResult{false, 1010, ErrnoMessage("missing font", workDir_ + "/mythroad/system/gb12.uc2")};
    }
    if (!CanRead("mythroad/system/gb16.uc2")) {
        return CommonResult{false, 1011, ErrnoMessage("missing font", workDir_ + "/mythroad/system/gb16.uc2")};
    }

    stopVmrp();
    VmrpHostReset(width_, height_);
    const int ret = startVmrpWithPath(packageName_.c_str(), extName_.c_str());
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP", "startVmrpWithPath ret=%{public}d", ret);
    if (ret != MR_SUCCESS) {
        return CommonResult{false, 1007, "startVmrpWithPath failed"};
    }
    state_ = State::Running;
    for (int i = 0; i < 10; ++i) {
        const int timerRet = timer();
        OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP", "initial timer pump #%{public}d ret=%{public}d drawCount=%{public}lld",
                     i + 1, timerRet, static_cast<long long>(VmrpHostDrawCount()));
        if (VmrpHostFrameId() > 0) {
            break;
        }
    }
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
    stopVmrp();
    state_ = State::Released;
    frameId_ = 0;
    VmrpHostReset(width_, height_);
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

bool MrpRuntime::IsRunning() const
{
    return state_ == State::Running && !VmrpHostExited();
}

int32_t MrpRuntime::PumpTimer(int32_t iterations)
{
    if (!IsRunning()) {
        return MR_FAILED;
    }
    const int32_t count = iterations > 0 ? iterations : 1;
    int32_t lastRet = MR_SUCCESS;
    for (int32_t i = 0; i < count; ++i) {
        lastRet = timer();
    }
    return lastRet;
}
