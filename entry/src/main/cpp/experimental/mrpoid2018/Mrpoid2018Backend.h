#ifndef MRP_EXPERIMENTAL_MRPOID2018_BACKEND_H
#define MRP_EXPERIMENTAL_MRPOID2018_BACKEND_H

#include <cstdint>
#include <string>
#include <vector>

#include "../../input/InputEvent.h"
#include "../../platform/MrpFacade.h"

class Mrpoid2018Backend {
public:
    CommonResult Init(const std::string &workDir, int32_t width, int32_t height, bool debug);
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
    enum class State {
        Idle,
        Initialized,
        Loaded,
        Running,
        Paused,
        Released,
    };

    CommonResult NotImplemented(const std::string &operation) const;

    State state_ = State::Idle;
    std::string workDir_;
    std::string packagePath_;
    int32_t width_ = 240;
    int32_t height_ = 320;
    bool debug_ = false;
};

#endif
