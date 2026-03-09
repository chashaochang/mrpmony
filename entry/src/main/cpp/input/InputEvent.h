#ifndef MRP_INPUT_INPUT_EVENT_H
#define MRP_INPUT_INPUT_EVENT_H

#include <cstdint>
#include <string>

struct MrpInputEvent {
    std::string type = "key";
    std::string action = "down";
    int32_t keyCode = 0;
    int32_t x = 0;
    int32_t y = 0;
    int64_t timestamp = 0;
};

bool DispatchInput(const MrpInputEvent &event);

#endif
