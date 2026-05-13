#include <node_api.h>

#include <cstring>
#include <mutex>
#include <string>
#include <vector>
#include <zlib.h>

#include "../input/InputEvent.h"
#include "../platform/MrpFacade.h"

namespace {
MrpFacade g_facade;
std::mutex g_facadeMutex;

enum class AsyncCommonAction {
    StartSession,
    Release,
};

struct AsyncCommonWork {
    napi_env env = nullptr;
    napi_async_work work = nullptr;
    napi_deferred deferred = nullptr;
    AsyncCommonAction action = AsyncCommonAction::Release;
    InitOptions options;
    std::string packagePath;
    CommonResult result;
};

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

int64_t GetInt64Property(napi_env env, napi_value obj, const char *name, int64_t fallback)
{
    bool hasProperty = false;
    napi_has_named_property(env, obj, name, &hasProperty);
    if (!hasProperty) {
        return fallback;
    }

    napi_value value;
    napi_get_named_property(env, obj, name, &value);
    int64_t result = fallback;
    napi_get_value_int64(env, value, &result);
    return result;
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

napi_value MakeUint32(napi_env env, uint32_t value)
{
    napi_value result;
    napi_create_uint32(env, value, &result);
    return result;
}

napi_value MakeUint64AsDouble(napi_env env, uint64_t value)
{
    napi_value result;
    napi_create_double(env, static_cast<double>(value), &result);
    return result;
}

std::string GetStringArgument(napi_env env, napi_value value)
{
    size_t length = 0;
    napi_get_value_string_utf8(env, value, nullptr, 0, &length);
    std::string result(length, '\0');
    napi_get_value_string_utf8(env, value, result.data(), length + 1, &length);
    return result;
}

napi_value MakeEditRequestResult(napi_env env, const EditRequestResult &result)
{
    napi_value obj;
    napi_create_object(env, &obj);

    napi_value ok;
    napi_get_boolean(env, result.ok, &ok);
    napi_set_named_property(env, obj, "ok", ok);

    napi_value hasRequest;
    napi_get_boolean(env, result.hasRequest, &hasRequest);
    napi_set_named_property(env, obj, "hasRequest", hasRequest);

    napi_value requestId;
    napi_create_int64(env, static_cast<int64_t>(result.requestId), &requestId);
    napi_set_named_property(env, obj, "requestId", requestId);

    napi_value handle;
    napi_create_int32(env, result.handle, &handle);
    napi_set_named_property(env, obj, "handle", handle);

    napi_value type;
    napi_create_int32(env, result.type, &type);
    napi_set_named_property(env, obj, "type", type);

    napi_value maxSize;
    napi_create_int32(env, result.maxSize, &maxSize);
    napi_set_named_property(env, obj, "maxSize", maxSize);

    napi_set_named_property(env, obj, "title", MakeString(env, result.title));
    napi_set_named_property(env, obj, "text", MakeString(env, result.text));

    napi_value errorCode;
    napi_create_int32(env, result.errorCode, &errorCode);
    napi_set_named_property(env, obj, "errorCode", errorCode);
    napi_set_named_property(env, obj, "errorMessage", MakeString(env, result.errorMessage));
    return obj;
}

napi_value MakeByteArray(napi_env env, const std::vector<uint8_t> &bytes)
{
    napi_value array;
    napi_create_array_with_length(env, bytes.size(), &array);
    for (size_t i = 0; i < bytes.size(); ++i) {
        napi_set_element(env, array, static_cast<uint32_t>(i), MakeUint32(env, bytes[i]));
    }
    return array;
}

napi_value MakeStringArray(napi_env env, const std::vector<std::string> &items)
{
    napi_value array;
    napi_create_array_with_length(env, items.size(), &array);
    for (size_t i = 0; i < items.size(); ++i) {
        napi_set_element(env, array, static_cast<uint32_t>(i), MakeString(env, items[i]));
    }
    return array;
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

    std::lock_guard<std::mutex> lock(g_facadeMutex);
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

    std::lock_guard<std::mutex> lock(g_facadeMutex);
    return MakeCommonResult(env, g_facade.LoadPackage(packagePath));
}

napi_value Start(napi_env env, napi_callback_info info)
{
    (void)info;
    std::lock_guard<std::mutex> lock(g_facadeMutex);
    return MakeCommonResult(env, g_facade.Start());
}

napi_value Pause(napi_env env, napi_callback_info info)
{
    (void)info;
    std::lock_guard<std::mutex> lock(g_facadeMutex);
    return MakeCommonResult(env, g_facade.Pause());
}

napi_value Resume(napi_env env, napi_callback_info info)
{
    (void)info;
    std::lock_guard<std::mutex> lock(g_facadeMutex);
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

    std::lock_guard<std::mutex> lock(g_facadeMutex);
    return MakeCommonResult(env, g_facade.SendInput(event));
}

napi_value PullFrame(napi_env env, napi_callback_info info)
{
    (void)info;
    FrameResult frame;
    {
        std::lock_guard<std::mutex> lock(g_facadeMutex);
        frame = g_facade.PullFrame();
    }

    napi_value obj;
    napi_create_object(env, &obj);

    napi_value ok;
    napi_get_boolean(env, frame.ok, &ok);
    napi_set_named_property(env, obj, "ok", ok);

    napi_value hasNewFrame;
    napi_get_boolean(env, frame.hasNewFrame, &hasNewFrame);
    napi_set_named_property(env, obj, "hasNewFrame", hasNewFrame);

    napi_value exited;
    napi_get_boolean(env, frame.exited, &exited);
    napi_set_named_property(env, obj, "exited", exited);

    napi_value width;
    napi_create_int32(env, frame.width, &width);
    napi_set_named_property(env, obj, "width", width);

    napi_value height;
    napi_create_int32(env, frame.height, &height);
    napi_set_named_property(env, obj, "height", height);

    napi_set_named_property(env, obj, "pixelFormat", MakeString(env, frame.pixelFormat));

    napi_value bufferBytes;
    napi_create_int32(env, static_cast<int32_t>(frame.buffer.size()), &bufferBytes);
    napi_set_named_property(env, obj, "bufferBytes", bufferBytes);

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
    std::lock_guard<std::mutex> lock(g_facadeMutex);
    return MakeCommonResult(env, g_facade.Release());
}

napi_value PullEditRequest(napi_env env, napi_callback_info info)
{
    (void)info;
    std::lock_guard<std::mutex> lock(g_facadeMutex);
    return MakeEditRequestResult(env, g_facade.PullEditRequest());
}

napi_value SubmitEditResult(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    uint64_t requestId = 0;
    bool confirmed = false;
    std::string text;
    if (argc == 1 && argv[0] != nullptr) {
        requestId = static_cast<uint64_t>(GetInt64Property(env, argv[0], "requestId", 0));
        confirmed = GetBoolProperty(env, argv[0], "confirmed", false);
        text = GetStringProperty(env, argv[0], "text", "");
    }

    std::lock_guard<std::mutex> lock(g_facadeMutex);
    return MakeCommonResult(env, g_facade.SubmitEditResult(requestId, confirmed, text));
}

void ExecuteAsyncCommon(napi_env env, void *data)
{
    (void)env;
    auto *work = static_cast<AsyncCommonWork *>(data);
    std::lock_guard<std::mutex> lock(g_facadeMutex);
    if (work->action == AsyncCommonAction::StartSession) {
        work->result = g_facade.Init(work->options);
        if (!work->result.ok) {
            return;
        }
        work->result = g_facade.LoadPackage(work->packagePath);
        if (!work->result.ok) {
            return;
        }
        work->result = g_facade.Start();
        return;
    }
    work->result = g_facade.Release();
}

void CompleteAsyncCommon(napi_env env, napi_status status, void *data)
{
    auto *work = static_cast<AsyncCommonWork *>(data);
    if (status == napi_ok) {
        napi_value result = MakeCommonResult(env, work->result);
        napi_resolve_deferred(env, work->deferred, result);
    } else {
        napi_value message = MakeString(env, "native async work cancelled");
        napi_reject_deferred(env, work->deferred, message);
    }
    napi_delete_async_work(env, work->work);
    delete work;
}

napi_value CreateAsyncCommon(napi_env env, AsyncCommonWork *work, const char *name)
{
    napi_value promise;
    napi_create_promise(env, &work->deferred, &promise);
    work->env = env;

    napi_value resourceName;
    napi_create_string_utf8(env, name, NAPI_AUTO_LENGTH, &resourceName);
    napi_status status = napi_create_async_work(env, nullptr, resourceName, ExecuteAsyncCommon,
                                                CompleteAsyncCommon, work, &work->work);
    if (status != napi_ok) {
        napi_value message = MakeString(env, "create native async work failed");
        napi_reject_deferred(env, work->deferred, message);
        delete work;
        return promise;
    }
    status = napi_queue_async_work(env, work->work);
    if (status != napi_ok) {
        napi_value message = MakeString(env, "queue native async work failed");
        napi_reject_deferred(env, work->deferred, message);
        napi_delete_async_work(env, work->work);
        delete work;
    }
    return promise;
}

napi_value StartSessionAsync(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    auto *work = new AsyncCommonWork();
    work->action = AsyncCommonAction::StartSession;
    if (argc >= 1 && argv[0] != nullptr) {
        work->options.workDir = GetStringProperty(env, argv[0], "workDir", "");
        work->options.width = GetIntProperty(env, argv[0], "width", 240);
        work->options.height = GetIntProperty(env, argv[0], "height", 320);
        work->options.debug = GetBoolProperty(env, argv[0], "debug", false);
    }
    if (argc >= 2 && argv[1] != nullptr) {
        size_t length = 0;
        napi_get_value_string_utf8(env, argv[1], nullptr, 0, &length);
        work->packagePath.resize(length);
        napi_get_value_string_utf8(env, argv[1], work->packagePath.data(), length + 1, &length);
    }
    return CreateAsyncCommon(env, work, "mrpStartSessionAsync");
}

napi_value ReleaseAsync(napi_env env, napi_callback_info info)
{
    (void)info;
    auto *work = new AsyncCommonWork();
    work->action = AsyncCommonAction::Release;
    return CreateAsyncCommon(env, work, "mrpReleaseAsync");
}

napi_value Gunzip(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 1 || argv[0] == nullptr) {
        napi_throw_error(env, nullptr, "gunzip requires an ArrayBuffer");
        return nullptr;
    }

    void *inputData = nullptr;
    size_t inputLength = 0;
    if (napi_get_arraybuffer_info(env, argv[0], &inputData, &inputLength) != napi_ok || inputData == nullptr) {
        napi_throw_error(env, nullptr, "gunzip input must be an ArrayBuffer");
        return nullptr;
    }
    if (inputLength == 0) {
        napi_value result;
        void *outputData = nullptr;
        napi_create_arraybuffer(env, 0, &outputData, &result);
        return result;
    }

    z_stream stream {};
    stream.next_in = static_cast<Bytef *>(inputData);
    stream.avail_in = static_cast<uInt>(inputLength);
    int status = inflateInit2(&stream, 16 + MAX_WBITS);
    if (status != Z_OK) {
        napi_throw_error(env, nullptr, "inflateInit2 failed");
        return nullptr;
    }

    std::vector<uint8_t> output;
    if (inputLength >= 4) {
        const auto *bytes = static_cast<const uint8_t *>(inputData);
        size_t expected = static_cast<size_t>(bytes[inputLength - 4]) |
            (static_cast<size_t>(bytes[inputLength - 3]) << 8) |
            (static_cast<size_t>(bytes[inputLength - 2]) << 16) |
            (static_cast<size_t>(bytes[inputLength - 1]) << 24);
        if (expected > 0 && expected < 16 * 1024 * 1024) {
            output.reserve(expected);
        }
    }

    uint8_t buffer[4096];
    do {
        stream.next_out = buffer;
        stream.avail_out = sizeof(buffer);
        status = inflate(&stream, Z_NO_FLUSH);
        if (status != Z_OK && status != Z_STREAM_END) {
            inflateEnd(&stream);
            napi_throw_error(env, nullptr, "inflate failed");
            return nullptr;
        }
        const size_t produced = sizeof(buffer) - stream.avail_out;
        output.insert(output.end(), buffer, buffer + produced);
    } while (status != Z_STREAM_END);

    inflateEnd(&stream);

    napi_value result;
    void *outputData = nullptr;
    napi_create_arraybuffer(env, output.size(), &outputData, &result);
    if (!output.empty()) {
        std::memcpy(outputData, output.data(), output.size());
    }
    return result;
}

napi_value InspectPureMrpPackage(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    napi_value obj;
    napi_create_object(env, &obj);
    napi_value okValue;
    napi_get_boolean(env, false, &okValue);
    if (argc < 1 || argv[0] == nullptr) {
        napi_set_named_property(env, obj, "ok", okValue);
        napi_set_named_property(env, obj, "errorMessage", MakeString(env, "inspectPureMrpPackage requires a path"));
        return obj;
    }

    const std::string path = GetStringArgument(env, argv[0]);
    std::string errorMessage;
    napi_set_named_property(env, obj, "ok", okValue);
    napi_set_named_property(env, obj, "errorMessage", MakeString(env, errorMessage));
    return obj;
}
}

napi_value ExportMrpFacade(napi_env env, napi_value exports)
{
    napi_property_descriptor descriptors[] = {
        {"init", nullptr, Init, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"loadPackage", nullptr, LoadPackage, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"start", nullptr, Start, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"startSessionAsync", nullptr, StartSessionAsync, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"pause", nullptr, Pause, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"resume", nullptr, Resume, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendInput", nullptr, SendInput, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"pullFrame", nullptr, PullFrame, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"pullEditRequest", nullptr, PullEditRequest, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"submitEditResult", nullptr, SubmitEditResult, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"release", nullptr, Release, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"releaseAsync", nullptr, ReleaseAsync, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"gunzip", nullptr, Gunzip, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"inspectPureMrpPackage", nullptr, InspectPureMrpPackage, nullptr, nullptr, nullptr, napi_default, nullptr},
    };

    napi_define_properties(env, exports, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
    return exports;
}