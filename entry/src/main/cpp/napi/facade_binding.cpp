#include <node_api.h>

#include <cstring>
#include <string>
#include <vector>

#include "../input/InputEvent.h"
#include "../platform/MrpFacade.h"

namespace {
MrpFacade g_facade;

napi_value MakeString(napi_env env, const std::string &value)
{
    napi_value result;
    napi_create_string_utf8(env, value.c_str(), value.size(), &result);
    return result;
}

napi_value MakeCommonResult(napi_env env, const CommonResult &result)
{
    napi_value obj;
    napi_create_object(env, &obj);

    napi_value ok;
    napi_get_boolean(env, result.ok, &ok);
    napi_set_named_property(env, obj, "ok", ok);

    napi_value errorCode;
    napi_create_int32(env, result.errorCode, &errorCode);
    napi_set_named_property(env, obj, "errorCode", errorCode);

    napi_set_named_property(env, obj, "errorMessage", MakeString(env, result.errorMessage));
    return obj;
}

std::string GetStringProperty(napi_env env, napi_value obj, const char *name, const std::string &fallback)
{
    bool hasProperty = false;
    napi_has_named_property(env, obj, name, &hasProperty);
    if (!hasProperty) {
        return fallback;
    }

    napi_value value;
    napi_get_named_property(env, obj, name, &value);
    size_t length = 0;
    napi_get_value_string_utf8(env, value, nullptr, 0, &length);
    std::string result(length, '\0');
    napi_get_value_string_utf8(env, value, result.data(), length + 1, &length);
    return result;
}

int32_t GetIntProperty(napi_env env, napi_value obj, const char *name, int32_t fallback)
{
    bool hasProperty = false;
    napi_has_named_property(env, obj, name, &hasProperty);
    if (!hasProperty) {
        return fallback;
    }

    napi_value value;
    napi_get_named_property(env, obj, name, &value);
    int32_t result = fallback;
    napi_get_value_int32(env, value, &result);
    return result;
}

bool GetBoolProperty(napi_env env, napi_value obj, const char *name, bool fallback)
{
    bool hasProperty = false;
    napi_has_named_property(env, obj, name, &hasProperty);
    if (!hasProperty) {
        return fallback;
    }

    napi_value value;
    napi_get_named_property(env, obj, name, &value);
    bool result = fallback;
    napi_get_value_bool(env, value, &result);
    return result;
}

napi_value Init(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    InitOptions options;
    if (argc == 1 && argv[0] != nullptr) {
        options.workDir = GetStringProperty(env, argv[0], "workDir", "");
        options.width = GetIntProperty(env, argv[0], "width", 240);
        options.height = GetIntProperty(env, argv[0], "height", 320);
        options.debug = GetBoolProperty(env, argv[0], "debug", false);
    }

    return MakeCommonResult(env, g_facade.Init(options));
}

napi_value LoadPackage(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    std::string packagePath;
    if (argc == 1 && argv[0] != nullptr) {
        size_t length = 0;
        napi_get_value_string_utf8(env, argv[0], nullptr, 0, &length);
        packagePath.resize(length);
        napi_get_value_string_utf8(env, argv[0], packagePath.data(), length + 1, &length);
    }

    return MakeCommonResult(env, g_facade.LoadPackage(packagePath));
}

napi_value Start(napi_env env, napi_callback_info info)
{
    (void)info;
    return MakeCommonResult(env, g_facade.Start());
}

napi_value Pause(napi_env env, napi_callback_info info)
{
    (void)info;
    return MakeCommonResult(env, g_facade.Pause());
}

napi_value Resume(napi_env env, napi_callback_info info)
{
    (void)info;
    return MakeCommonResult(env, g_facade.Resume());
}

napi_value SendInput(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    MrpInputEvent event;
    if (argc == 1 && argv[0] != nullptr) {
        event.type = GetStringProperty(env, argv[0], "type", "key");
        event.action = GetStringProperty(env, argv[0], "action", "down");
        event.keyCode = GetIntProperty(env, argv[0], "keyCode", 0);
        event.x = GetIntProperty(env, argv[0], "x", 0);
        event.y = GetIntProperty(env, argv[0], "y", 0);
        event.timestamp = static_cast<int64_t>(GetIntProperty(env, argv[0], "timestamp", 0));
    }

    return MakeCommonResult(env, g_facade.SendInput(event));
}

napi_value PullFrame(napi_env env, napi_callback_info info)
{
    (void)info;
    FrameResult frame = g_facade.PullFrame();

    napi_value obj;
    napi_create_object(env, &obj);

    napi_value ok;
    napi_get_boolean(env, frame.ok, &ok);
    napi_set_named_property(env, obj, "ok", ok);

    napi_value hasNewFrame;
    napi_get_boolean(env, frame.hasNewFrame, &hasNewFrame);
    napi_set_named_property(env, obj, "hasNewFrame", hasNewFrame);

    napi_value width;
    napi_create_int32(env, frame.width, &width);
    napi_set_named_property(env, obj, "width", width);

    napi_value height;
    napi_create_int32(env, frame.height, &height);
    napi_set_named_property(env, obj, "height", height);

    napi_set_named_property(env, obj, "pixelFormat", MakeString(env, frame.pixelFormat));

    void *data = nullptr;
    napi_value buffer;
    napi_create_arraybuffer(env, frame.buffer.size(), &data, &buffer);
    if (data != nullptr && !frame.buffer.empty()) {
        std::memcpy(data, frame.buffer.data(), frame.buffer.size());
    }
    napi_set_named_property(env, obj, "buffer", buffer);

    napi_value frameId;
    napi_create_int64(env, frame.frameId, &frameId);
    napi_set_named_property(env, obj, "frameId", frameId);

    napi_value errorCode;
    napi_create_int32(env, frame.errorCode, &errorCode);
    napi_set_named_property(env, obj, "errorCode", errorCode);

    napi_set_named_property(env, obj, "errorMessage", MakeString(env, frame.errorMessage));

    return obj;
}

napi_value Release(napi_env env, napi_callback_info info)
{
    (void)info;
    return MakeCommonResult(env, g_facade.Release());
}
} // namespace

napi_value ExportMrpFacade(napi_env env, napi_value exports)
{
    napi_property_descriptor descriptors[] = {
        {"init", nullptr, Init, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"loadPackage", nullptr, LoadPackage, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"start", nullptr, Start, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"pause", nullptr, Pause, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"resume", nullptr, Resume, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendInput", nullptr, SendInput, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"pullFrame", nullptr, PullFrame, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"release", nullptr, Release, nullptr, nullptr, nullptr, napi_default, nullptr},
    };

    napi_define_properties(env, exports, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
    return exports;
}
