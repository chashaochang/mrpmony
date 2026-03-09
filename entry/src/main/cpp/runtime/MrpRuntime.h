#ifndef MRP_RUNTIME_MRP_RUNTIME_H
#define MRP_RUNTIME_MRP_RUNTIME_H

#include "../platform/MrpFacade.h"

class MrpRuntime {
public:
    CommonResult Init(int32_t width, int32_t height, bool debug);
    CommonResult LoadPackage(const std::string &packagePath);
    CommonResult Start();
    CommonResult Pause();
    CommonResult Resume();
    CommonResult Release();

    int32_t Width() const;
    int32_t Height() const;
    int64_t NextFrameId();
    int64_t CurrentFrameId() const;

private:
    enum class State {
        Idle,
        Initialized,
        Loaded,
        Running,
        Paused,
        Released,
    };

    State state_ = State::Idle;
    int32_t width_ = 240;
    int32_t height_ = 320;
    bool debug_ = false;
    int64_t frameId_ = 0;
};

#endif
