#include <node_api.h>

#include "../runtime/XComponentSurfaceRenderer.h"

napi_value ExportMrpFacade(napi_env env, napi_value exports);

static napi_value InitModule(napi_env env, napi_value exports)
{
    napi_value result = ExportMrpFacade(env, exports);
    MrpRegisterXComponent(env, result);
    return result;
}

static napi_module g_mrpModule = {
    1,
    0,
    nullptr,
    InitModule,
    "mrp_napi",
    nullptr,
    {0},
};

extern "C" __attribute__((constructor)) void RegisterMrpModule(void)
{
    napi_module_register(&g_mrpModule);
}
