#ifndef MRP_RUNTIME_XCOMPONENT_SURFACE_RENDERER_H
#define MRP_RUNTIME_XCOMPONENT_SURFACE_RENDERER_H

#include <node_api.h>

#include <cstdint>
#include <vector>

void MrpRegisterXComponent(napi_env env, napi_value exports);
bool MrpRenderFrameToXComponent(const std::vector<uint8_t> &bgra, int32_t width, int32_t height);
int64_t MrpXComponentSurfaceGeneration();

#endif
