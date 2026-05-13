#ifndef MRP_RUNTIME_VMRP_HOST_BRIDGE_H
#define MRP_RUNTIME_VMRP_HOST_BRIDGE_H

#include <cstdint>
#include <string>
#include <vector>

struct VmrpEditRequest {
    bool pending = false;
    uint64_t requestId = 0;
    int32_t handle = 0;
    int32_t type = 0;
    int32_t maxSize = 0;
    std::string title;
    std::string text;
};

void VmrpHostReset(int32_t width, int32_t height);
bool VmrpHostCopyFrame(std::vector<uint8_t> &out, int32_t &width, int32_t &height, int64_t &frameId);
int64_t VmrpHostFrameId();
int64_t VmrpHostDrawCount();
bool VmrpHostExited();
bool VmrpHostGetPendingEditRequest(VmrpEditRequest &out);
bool VmrpHostResolveEditRequest(uint64_t requestId, bool confirmed, const std::string &text);
bool VmrpHostCompleteEditRequest(uint64_t requestId);

#endif
