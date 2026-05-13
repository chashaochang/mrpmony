#include "VmrpHostBridge.h"

#include <algorithm>
#include <hilog/log.h>
#include <mutex>
#include <string>

extern "C" {
#include "header/memory.h"
#include "header/types.h"
}

namespace {
std::mutex g_frameMutex;
std::vector<uint8_t> g_rgba;
int32_t g_width = 240;
int32_t g_height = 320;
int64_t g_frameId = 0;
int64_t g_drawCount = 0;
int64_t g_copyCount = 0;
uint16_t g_timerInterval = 0;
std::string g_editText;
char *g_editTextMem = nullptr;
std::string g_editTitle;
int32_t g_editType = 0;
int32_t g_editMaxSize = 0;
int32_t g_editHandle = 0;
uint64_t g_editRequestId = 0;
bool g_editPending = false;
bool g_exited = false;

constexpr int32_t kDefaultEditLimit = 256;
constexpr int32_t kEditHandle = 1234;

uint8_t Expand5(uint16_t value)
{
    return static_cast<uint8_t>((value << 3U) | (value >> 2U));
}

uint8_t Expand6(uint16_t value)
{
    return static_cast<uint8_t>((value << 2U) | (value >> 4U));
}

size_t Utf8CharCount(const std::string &value)
{
    size_t count = 0;
    for (size_t i = 0; i < value.size();) {
        const uint8_t ch = static_cast<uint8_t>(value[i]);
        size_t step = 1;
        if ((ch & 0x80U) == 0) {
            step = 1;
        } else if ((ch & 0xE0U) == 0xC0U) {
            step = 2;
        } else if ((ch & 0xF0U) == 0xE0U) {
            step = 3;
        } else if ((ch & 0xF8U) == 0xF0U) {
            step = 4;
        }
        if (i + step > value.size()) {
            step = 1;
        }
        i += step;
        ++count;
    }
    return count;
}

std::string TruncateUtf8ByChars(const std::string &value, int32_t maxChars)
{
    if (maxChars <= 0) {
        return value;
    }
    size_t count = 0;
    size_t i = 0;
    for (; i < value.size() && static_cast<int32_t>(count) < maxChars;) {
        const uint8_t ch = static_cast<uint8_t>(value[i]);
        size_t step = 1;
        if ((ch & 0x80U) == 0) {
            step = 1;
        } else if ((ch & 0xE0U) == 0xC0U) {
            step = 2;
        } else if ((ch & 0xF0U) == 0xE0U) {
            step = 3;
        } else if ((ch & 0xF8U) == 0xF0U) {
            step = 4;
        }
        if (i + step > value.size()) {
            step = 1;
        }
        i += step;
        ++count;
    }
    if (i >= value.size()) {
        return value;
    }
    return value.substr(0, i);
}

std::string SanitizeByType(const std::string &value, int32_t editType)
{
    // MR_EDIT_NUMERIC = 1
    if (editType != 1) {
        return value;
    }
    std::string numeric;
    numeric.reserve(value.size());
    for (char ch : value) {
        if (ch >= '0' && ch <= '9') {
            numeric.push_back(ch);
        }
    }
    return numeric;
}

void FreeEditTextMem()
{
    if (g_editTextMem != nullptr) {
        my_freeExt(g_editTextMem);
        g_editTextMem = nullptr;
    }
}
} // namespace

void VmrpHostReset(int32_t width, int32_t height)
{
    std::lock_guard<std::mutex> lock(g_frameMutex);
    g_width = width > 0 ? width : 240;
    g_height = height > 0 ? height : 320;
    g_rgba.assign(static_cast<size_t>(g_width) * static_cast<size_t>(g_height) * 4U, 0);
    g_frameId = 0;
    g_drawCount = 0;
    g_copyCount = 0;
    g_timerInterval = 0;
    FreeEditTextMem();
    g_editText.clear();
    g_editTitle.clear();
    g_editType = 0;
    g_editMaxSize = 0;
    g_editHandle = kEditHandle;
    g_editRequestId = 0;
    g_editPending = false;
    g_exited = false;
}

bool VmrpHostCopyFrame(std::vector<uint8_t> &out, int32_t &width, int32_t &height, int64_t &frameId)
{
    std::lock_guard<std::mutex> lock(g_frameMutex);
    width = g_width;
    height = g_height;
    frameId = g_frameId;
    if (g_frameId == 0 || g_rgba.empty()) {
        out.clear();
        return false;
    }
    out = g_rgba;
    ++g_copyCount;
    if (g_copyCount <= 3 || (g_copyCount % 60) == 0) {
        int32_t nonBlack = 0;
        for (size_t i = 0; i + 3U < g_rgba.size(); i += 4U) {
            if (g_rgba[i] != 0 || g_rgba[i + 1U] != 0 || g_rgba[i + 2U] != 0) {
                ++nonBlack;
            }
        }
        OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP", "copyFrame #%{public}lld frame=%{public}lld bytes=%{public}zu nonBlack=%{public}d first=%{public}u,%{public}u,%{public}u,%{public}u",
                     static_cast<long long>(g_copyCount), static_cast<long long>(g_frameId), g_rgba.size(), nonBlack,
                     static_cast<uint32_t>(g_rgba[0]), static_cast<uint32_t>(g_rgba[1]),
                     static_cast<uint32_t>(g_rgba[2]), static_cast<uint32_t>(g_rgba[3]));
    }
    return true;
}

int64_t VmrpHostFrameId()
{
    std::lock_guard<std::mutex> lock(g_frameMutex);
    return g_frameId;
}

int64_t VmrpHostDrawCount()
{
    std::lock_guard<std::mutex> lock(g_frameMutex);
    return g_drawCount;
}

bool VmrpHostExited()
{
    std::lock_guard<std::mutex> lock(g_frameMutex);
    return g_exited;
}

extern "C" void vmrpHostOnExit()
{
    std::lock_guard<std::mutex> lock(g_frameMutex);
    g_exited = true;
    g_timerInterval = 0;
    FreeEditTextMem();
    if (g_rgba.empty()) {
        g_rgba.assign(static_cast<size_t>(g_width) * static_cast<size_t>(g_height) * 4U, 0);
    } else {
        std::fill(g_rgba.begin(), g_rgba.end(), 0);
    }
    ++g_frameId;
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP", "vmrpHostOnExit clear frame, frame=%{public}lld",
                 static_cast<long long>(g_frameId));
}

extern "C" void guiDrawBitmap(uint16_t *bmp, int32_t x, int32_t y, int32_t w, int32_t h)
{
    if (bmp == nullptr || w <= 0 || h <= 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_frameMutex);
    if (g_exited) {
        return;
    }
    if (g_rgba.empty()) {
        g_rgba.assign(static_cast<size_t>(g_width) * static_cast<size_t>(g_height) * 4U, 0);
    }

    for (int32_t row = 0; row < h; ++row) {
        const int32_t dstY = y + row;
        if (dstY < 0 || dstY >= g_height) {
            continue;
        }
        for (int32_t col = 0; col < w; ++col) {
            const int32_t dstX = x + col;
            if (dstX < 0 || dstX >= g_width) {
                continue;
            }

            // MRP drawBitmap passes the full screen buffer, while x/y/w/h
            // describe the dirty rect. Source sampling must use absolute
            // screen coordinates instead of row*w local indexing.
            const uint16_t pixel = bmp[dstY * g_width + dstX];
            const size_t index = static_cast<size_t>(dstY * g_width + dstX) * 4U;
            g_rgba[index] = Expand5(static_cast<uint16_t>(pixel & 0x1FU));
            g_rgba[index + 1U] = Expand6(static_cast<uint16_t>((pixel >> 5U) & 0x3FU));
            g_rgba[index + 2U] = Expand5(static_cast<uint16_t>((pixel >> 11U) & 0x1FU));
            g_rgba[index + 3U] = 255;
        }
    }
    ++g_frameId;
    ++g_drawCount;
    if (g_drawCount <= 2 || (g_drawCount % 60) == 0) {
        int32_t nonBlack = 0;
        for (int32_t i = 0; i < w * h; ++i) {
            if (bmp[i] != 0) {
                ++nonBlack;
            }
        }
        OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP", "guiDrawBitmap #%{public}lld x=%{public}d y=%{public}d w=%{public}d h=%{public}d nonBlack=%{public}d first=%{public}u",
                     static_cast<long long>(g_drawCount), x, y, w, h, nonBlack, static_cast<uint32_t>(bmp[0]));
    }
}

extern "C" int32_t timerStart(uint16_t t)
{
    std::lock_guard<std::mutex> lock(g_frameMutex);
    if (g_exited) {
        return MR_FAILED;
    }
    g_timerInterval = t;
    return MR_SUCCESS;
}

extern "C" int32_t timerStop()
{
    std::lock_guard<std::mutex> lock(g_frameMutex);
    g_timerInterval = 0;
    return MR_SUCCESS;
}

extern "C" int32_t editCreate(const char *title, const char *text, int32_t type, int32_t max_size)
{
    std::lock_guard<std::mutex> lock(g_frameMutex);
    g_editType = type;
    g_editMaxSize = max_size > 0 ? max_size : kDefaultEditLimit;
    g_editTitle = title != nullptr ? title : "";
    std::string initial = text != nullptr ? text : "";
    initial = SanitizeByType(initial, g_editType);
    g_editText = TruncateUtf8ByChars(initial, g_editMaxSize);
    g_editHandle = kEditHandle;
    ++g_editRequestId;
    g_editPending = true;
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                 "editCreate handle=%{public}d request=%{public}llu type=%{public}d max=%{public}d chars=%{public}zu",
                 g_editHandle, static_cast<unsigned long long>(g_editRequestId), g_editType, g_editMaxSize,
                 Utf8CharCount(g_editText));
    return g_editHandle;
}

extern "C" int32 editRelease(int32 edit)
{
    std::lock_guard<std::mutex> lock(g_frameMutex);
    (void)edit;
    g_editPending = false;
    FreeEditTextMem();
    return MR_SUCCESS;
}

extern "C" char *editGetText(int32 edit)
{
    std::lock_guard<std::mutex> lock(g_frameMutex);
    (void)edit;
    FreeEditTextMem();
    g_editTextMem = static_cast<char *>(my_mallocExt(static_cast<uint32_t>(g_editText.size() + 1U)));
    if (g_editTextMem == nullptr) {
        return nullptr;
    }
    if (!g_editText.empty()) {
        std::copy(g_editText.begin(), g_editText.end(), g_editTextMem);
    }
    g_editTextMem[g_editText.size()] = '\0';
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                 "editGetText handle=%{public}d bytes=%{public}zu chars=%{public}zu",
                 edit, g_editText.size(), Utf8CharCount(g_editText));
    return g_editTextMem;
}

bool VmrpHostGetPendingEditRequest(VmrpEditRequest &out)
{
    std::lock_guard<std::mutex> lock(g_frameMutex);
    out.pending = g_editPending;
    out.requestId = g_editRequestId;
    out.handle = g_editHandle;
    out.type = g_editType;
    out.maxSize = g_editMaxSize;
    out.title = g_editTitle;
    out.text = g_editText;
    return true;
}

bool VmrpHostResolveEditRequest(uint64_t requestId, bool confirmed, const std::string &text)
{
    std::lock_guard<std::mutex> lock(g_frameMutex);
    if (!g_editPending || requestId != g_editRequestId) {
        return false;
    }
    if (confirmed) {
        std::string next = SanitizeByType(text, g_editType);
        g_editText = TruncateUtf8ByChars(next, g_editMaxSize);
    }
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP",
                 "editResolve request=%{public}llu confirmed=%{public}d chars=%{public}zu",
                 static_cast<unsigned long long>(requestId), confirmed ? 1 : 0, Utf8CharCount(g_editText));
    return true;
}

bool VmrpHostCompleteEditRequest(uint64_t requestId)
{
    std::lock_guard<std::mutex> lock(g_frameMutex);
    if (!g_editPending || requestId != g_editRequestId) {
        return false;
    }
    g_editPending = false;
    return true;
}
