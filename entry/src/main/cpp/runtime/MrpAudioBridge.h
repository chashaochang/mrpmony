#ifndef MRP_RUNTIME_MRP_AUDIO_BRIDGE_H
#define MRP_RUNTIME_MRP_AUDIO_BRIDGE_H

#include <cstdint>

extern "C" int32_t vmrpHostPlaySound(int32_t type, const void *data, uint32_t dataLen, int32_t loop);
extern "C" int32_t vmrpHostStopSound(int32_t type);
extern "C" void vmrpHostReleaseAudio();

#endif
