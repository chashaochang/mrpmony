#ifndef MRP_PLATFORM_MRP_FACADE_H
#define MRP_PLATFORM_MRP_FACADE_H

#include <cstdint>
#include <string>
#include <vector>

#include "../input/InputEvent.h"

struct InitOptions {
    std::string workDir;
    int32_t width = 240;
    int32_t height = 320;
    bool debug = false;
};

struct CommonResult {
    bool ok = false;
    int32_t errorCode = 0;
    std::string errorMessage;
};

struct FrameResult {
    bool ok = false;
    bool hasNewFrame = false;
    bool exited = false;
    int32_t width = 0;
    int32_t height = 0;
    std::string pixelFormat = "RGBA_8888";
    std::vector<uint8_t> buffer;
    int64_t frameId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
};

struct EditRequestResult {
    bool ok = false;
    bool hasRequest = false;
    uint64_t requestId = 0;
    int32_t handle = 0;
    int32_t type = 0;
    int32_t maxSize = 0;
    std::string title;
    std::string text;
    int32_t errorCode = 0;
    std::string errorMessage;
};

class MrpFacade {
public:
    CommonResult Init(const InitOptions &options);
    CommonResult LoadPackage(const std::string &packagePath);
    CommonResult Start();
    CommonResult Pause();
    CommonResult Resume();
    CommonResult SendInput(const MrpInputEvent &event);
    FrameResult PullFrame();
    EditRequestResult PullEditRequest();
    CommonResult SubmitEditResult(uint64_t requestId, bool confirmed, const std::string &text);
    CommonResult Release();

private:
    InitOptions options_;
    std::string packagePath_;
    int64_t lastFrameId_ = 0;
    int64_t lastRenderedSurfaceGeneration_ = 0;
};

#endif
