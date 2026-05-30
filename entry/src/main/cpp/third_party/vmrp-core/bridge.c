#include "./header/bridge.h"

#include <ctype.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#if defined(__OHOS__)
#include <hilog/log.h>
#elif defined(__ANDROID__)
#include <android/log.h>
#endif

#include <zlib.h>

#include "./mythroad/include/dsm.h"
#include "./mythroad/include/md5.h"
#include "./mythroad/include/tables.h"
#include "./header/fileLib.h"
#include "./header/memory.h"
#include "./header/vmrp.h"
#include "./header/debug.h"
#include "./header/network.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
extern int32_t vmrpHostPlaySound(int32_t type, const void *data, uint32_t dataLen, int32_t loop);
extern int32_t vmrpHostStopSound(int32_t type);
#endif

// mythroad utility objects (e.g. md5/string) expect these symbols.
void *mr_mallocExt(uint32 len) {
    return my_mallocExt(len);
}

void mr_freeExt(void *p) {
    my_freeExt(p);
}
//////////////////////////////////////////////////////////////////////////////////////////
#ifdef LOG
#undef LOG
#endif

#ifdef DEBUG
#define LOG(format, ...) printf("   -> bridge: " format, ##__VA_ARGS__)
#else
#define LOG(format, ...)
#endif

#define SET_RET_V(ret)                        \
    {                                         \
        uint32_t _v = ret;                    \
        uc_reg_write(uc, UC_ARM_REG_R0, &_v); \
    }

#if defined(__OHOS__)
#define BR_HILOGI(fmt, ...) OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "MRP", fmt, ##__VA_ARGS__)
#elif defined(__ANDROID__)
#define BR_HILOGI(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "MRP", fmt, ##__VA_ARGS__)
#else
#define BR_HILOGI(fmt, ...) printf("[MRP] " fmt "\n", ##__VA_ARGS__)
#endif
typedef struct mr_c_function_P_t {
    uint8 *start_of_ER_RW;  // RW段指针
    uint32 ER_RW_Length;    // RW长度
    int32 ext_type;         // ext启动类型，为1时表示ext启动
    void *mrc_extChunk;     // ext模块描述段，下面的结构体。
    int32 stack;            // stack shell 2008-2-28
} mr_c_function_P_t;

static void *mr_table;
static mr_c_function_P_t *mr_c_function_P;
static void *dsm_require_funcs;
static event_t *mr_c_event;  // 用于mrc_event参数传递的内存
static event_t *dsm_event;   // 用于传递真实事件
static start_t *mr_start_dsm_param;
static uint32_t mr_extHelper_addr;

static const char MUTEX_LOCK_FAIL[] = "mutex lock fail";
static const char MUTEX_UNLOCK_FAIL[] = "mutex unlock fail";
static pthread_mutex_t mutex;
static uc_err g_bridgeLastErr = UC_ERR_OK;
static struct rb_root root = RB_ROOT;

static uc_err runCode(uc_engine *uc, uint32_t startAddr, uint32_t stopAddr, bool isThumb);
static void br_timerStart(BridgeMap *o, uc_engine *uc);
static void br_timerStop(BridgeMap *o, uc_engine *uc);
static void br_get_uptime_ms(BridgeMap *o, uc_engine *uc);
static void br_getDatetime(BridgeMap *o, uc_engine *uc);
static void br_sleep(BridgeMap *o, uc_engine *uc);
static void br_mr_plat(BridgeMap *o, uc_engine *uc);
static void br_mr_platEx(BridgeMap *o, uc_engine *uc);
static void br_mr_findStart(BridgeMap *o, uc_engine *uc);
static void br_mr_findGetNext(BridgeMap *o, uc_engine *uc);
static void br_mr_findStop(BridgeMap *o, uc_engine *uc);
static void br_mr_md5_init(BridgeMap *o, uc_engine *uc);
static void br_mr_md5_append(BridgeMap *o, uc_engine *uc);
static void br_mr_md5_finish(BridgeMap *o, uc_engine *uc);
static void br_mr_div(BridgeMap *o, uc_engine *uc);
static void br_mr_mod(BridgeMap *o, uc_engine *uc);
static void br_mr_platDrawChar(BridgeMap *o, uc_engine *uc);
static void br__mr_load_sms_cfg(BridgeMap *o, uc_engine *uc);
static void br__mr_save_sms_cfg(BridgeMap *o, uc_engine *uc);
static void br__DispUpEx(BridgeMap *o, uc_engine *uc);
static void br__DrawPoint(BridgeMap *o, uc_engine *uc);
static void br__DrawBitmap(BridgeMap *o, uc_engine *uc);
static void br__DrawBitmapEx(BridgeMap *o, uc_engine *uc);
static void br_DrawRect(BridgeMap *o, uc_engine *uc);
static void br__DrawText(BridgeMap *o, uc_engine *uc);
static void br__BitmapCheck(BridgeMap *o, uc_engine *uc);
static void br__mr_readFile(BridgeMap *o, uc_engine *uc);
static void br_mr_wstrlen(BridgeMap *o, uc_engine *uc);
static void br_mr_registerAPP(BridgeMap *o, uc_engine *uc);
static void br__DrawTextEx(BridgeMap *o, uc_engine *uc);
static void br__mr_EffSetCon(BridgeMap *o, uc_engine *uc);
static void br__mr_TestCom(BridgeMap *o, uc_engine *uc);
static void br__mr_TestCom1(BridgeMap *o, uc_engine *uc);
static void br_c2u(BridgeMap *o, uc_engine *uc);
static const uint8_t *br_get_char_bitmap_host(uint16_t ch, uint16_t fontSize, int32_t *width, int32_t *height);

#define BR_MAX_ENTRY_NAME 256

static char *br_next_valid_dirent(int32_t h);
static uint32_t br_read_le32(const uint8_t *p);
static const char *br_get_pack_filename(void);
static const char *br_normalize_entry_name(const char *name);
static bool br_entry_name_equals(const char *a, const char *b);
static bool br_is_reasonable_path(const char *s);
static bool br_read_file_from_pack(const char *packFilename, const char *entryName, int lookfor, void **outBuf, int32_t *outLen);
static bool br_is_gzip_buffer(const void *buf, int32_t len);
static bool br_decompress_gzip_buffer(const void *buf, int32_t len, void **outBuf, int32_t *outLen);

static void bridgeSetLastErr(uc_err err, const char *reason) {
    if (g_bridgeLastErr == UC_ERR_OK && err != UC_ERR_OK) {
        g_bridgeLastErr = err;
    }
    if (reason) {
        printf("%s\n", reason);
    }
}

static bool bridgeMutexLockOrFail(void) {
    if (pthread_mutex_lock(&mutex) != 0) {
        perror(MUTEX_LOCK_FAIL);
        bridgeSetLastErr(UC_ERR_EXCEPTION, "bridge mutex lock failed");
        return false;
    }
    return true;
}

static bool bridgeMutexUnlockOrFail(void) {
    if (pthread_mutex_unlock(&mutex) != 0) {
        perror(MUTEX_UNLOCK_FAIL);
        bridgeSetLastErr(UC_ERR_EXCEPTION, "bridge mutex unlock failed");
        return false;
    }
    return true;
}

static void bridgeClearHookMap(void) {
    struct rb_node *node = rb_first(&root);
    while (node != NULL) {
        struct rb_node *next = rb_next(node);
        uIntMap *obj = rb_entry(node, uIntMap, node);
        rb_erase(&obj->node, &root);
        free(obj);
        node = next;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

static void br__mr_c_function_new(BridgeMap *o, uc_engine *uc) {
    // typedef int32 (*T__mr_c_function_new)(MR_C_FUNCTION f, int32 len);
    uint32_t p_f, p_len;
    uc_reg_read(uc, UC_ARM_REG_R0, &p_f);
    uc_reg_read(uc, UC_ARM_REG_R1, &p_len);
    printf("ext call %s(0x%X[%u], 0x%X[%u])\n", o->name, p_f, p_f, p_len, p_len);
    dumpREG(uc);

    mr_extHelper_addr = p_f;
    mr_c_function_P = my_mallocExt(p_len);
    memset(mr_c_function_P, 0, p_len);

    uint32_t v = toMrpMemAddr(mr_c_function_P);
    uc_mem_write(uc, CODE_ADDRESS + 4, &v, 4);
    SET_RET_V(MR_SUCCESS);
}

static void br_mr_malloc(BridgeMap *o, uc_engine *uc) {
    // typedef void* (*T_mr_malloc)(uint32 len);
    uint32_t len;
    uc_reg_read(uc, UC_ARM_REG_R0, &len);
    void *p = my_mallocExt(len);
    if (p) {
        uint32_t ret = toMrpMemAddr(p);
        LOG("ext call %s(0x%X[%u]) ret=0x%X[%u]\n", o->name, len, len, ret, ret);
        SET_RET_V(ret);
        return;
    }
    SET_RET_V((uint32_t)NULL);
}

static void br_mr_free(BridgeMap *o, uc_engine *uc) {
    // typedef void  (*T_mr_free)(void* p, uint32 len);
    uint32_t p, len;
    uc_reg_read(uc, UC_ARM_REG_R0, &p);
    uc_reg_read(uc, UC_ARM_REG_R1, &len);

    LOG("ext call %s(0x%X[%u], 0x%X[%u])\n", o->name, p, p, len, len);
    my_freeExt(getMrpMemPtr(p));
}

static void br_memcpy(BridgeMap *o, uc_engine *uc) {
    //  void* (*T_memcpy)(void* dst, const void* src, int n);
    uint32_t dst, src, n;
    uc_reg_read(uc, UC_ARM_REG_R0, &dst);
    uc_reg_read(uc, UC_ARM_REG_R1, &src);
    uc_reg_read(uc, UC_ARM_REG_R2, &n);
    memcpy(getMrpMemPtr(dst), getMrpMemPtr(src), n);
    SET_RET_V(dst);
}

static void br_mr_realloc(BridgeMap *o, uc_engine *uc) {
    // typedef void* (*T_mr_realloc)(void* p, uint32 oldlen, uint32 len);
    uint32_t p, oldlen, len;
    uc_reg_read(uc, UC_ARM_REG_R0, &p);
    uc_reg_read(uc, UC_ARM_REG_R1, &oldlen);
    uc_reg_read(uc, UC_ARM_REG_R2, &len);

    if (len == 0) {
        if (p != 0) {
            my_freeExt(getMrpMemPtr(p));
        }
        SET_RET_V((uint32_t)NULL);
        return;
    }

    void *newPtr = my_mallocExt(len);
    if (newPtr == NULL) {
        SET_RET_V((uint32_t)NULL);
        return;
    }

    if (p != 0 && oldlen > 0) {
        uint32_t copyLen = oldlen < len ? oldlen : len;
        memcpy(newPtr, getMrpMemPtr(p), copyLen);
        my_freeExt(getMrpMemPtr(p));
    }

    SET_RET_V(toMrpMemAddr(newPtr));
}

static void br_memset(BridgeMap *o, uc_engine *uc) {
    // void* (*T_memset)(void* s, int c, int n);
    uint32_t dst, value, n;
    uc_reg_read(uc, UC_ARM_REG_R0, &dst);
    uc_reg_read(uc, UC_ARM_REG_R1, &value);
    uc_reg_read(uc, UC_ARM_REG_R2, &n);
    memset(getMrpMemPtr(dst), value, n);
    SET_RET_V(dst);
}

static void br_memmove(BridgeMap *o, uc_engine *uc) {
    // void* (*T_memmove)(void* s1, const void* s2, int n);
    uint32_t dst, src, n;
    uc_reg_read(uc, UC_ARM_REG_R0, &dst);
    uc_reg_read(uc, UC_ARM_REG_R1, &src);
    uc_reg_read(uc, UC_ARM_REG_R2, &n);
    memmove(getMrpMemPtr(dst), getMrpMemPtr(src), n);
    SET_RET_V(dst);
}

static void br_strcpy(BridgeMap *o, uc_engine *uc) {
    uint32_t dst, src;
    uc_reg_read(uc, UC_ARM_REG_R0, &dst);
    uc_reg_read(uc, UC_ARM_REG_R1, &src);
    strcpy(getMrpMemPtr(dst), getMrpMemPtr(src));
    SET_RET_V(dst);
}

static void br_strncpy(BridgeMap *o, uc_engine *uc) {
    uint32_t dst, src, n;
    uc_reg_read(uc, UC_ARM_REG_R0, &dst);
    uc_reg_read(uc, UC_ARM_REG_R1, &src);
    uc_reg_read(uc, UC_ARM_REG_R2, &n);
    strncpy(getMrpMemPtr(dst), getMrpMemPtr(src), n);
    SET_RET_V(dst);
}

static void br_strcat(BridgeMap *o, uc_engine *uc) {
    uint32_t dst, src;
    uc_reg_read(uc, UC_ARM_REG_R0, &dst);
    uc_reg_read(uc, UC_ARM_REG_R1, &src);
    strcat(getMrpMemPtr(dst), getMrpMemPtr(src));
    SET_RET_V(dst);
}

static void br_strncat(BridgeMap *o, uc_engine *uc) {
    uint32_t dst, src, n;
    uc_reg_read(uc, UC_ARM_REG_R0, &dst);
    uc_reg_read(uc, UC_ARM_REG_R1, &src);
    uc_reg_read(uc, UC_ARM_REG_R2, &n);
    strncat(getMrpMemPtr(dst), getMrpMemPtr(src), n);
    SET_RET_V(dst);
}

static void br_memcmp(BridgeMap *o, uc_engine *uc) {
    uint32_t s1, s2, n;
    uc_reg_read(uc, UC_ARM_REG_R0, &s1);
    uc_reg_read(uc, UC_ARM_REG_R1, &s2);
    uc_reg_read(uc, UC_ARM_REG_R2, &n);
    SET_RET_V(memcmp(getMrpMemPtr(s1), getMrpMemPtr(s2), n));
}

static void br_strcmp(BridgeMap *o, uc_engine *uc) {
    uint32_t s1, s2;
    uc_reg_read(uc, UC_ARM_REG_R0, &s1);
    uc_reg_read(uc, UC_ARM_REG_R1, &s2);
    SET_RET_V(strcmp(getMrpMemPtr(s1), getMrpMemPtr(s2)));
}

static void br_strncmp(BridgeMap *o, uc_engine *uc) {
    uint32_t s1, s2, n;
    uc_reg_read(uc, UC_ARM_REG_R0, &s1);
    uc_reg_read(uc, UC_ARM_REG_R1, &s2);
    uc_reg_read(uc, UC_ARM_REG_R2, &n);
    SET_RET_V(strncmp(getMrpMemPtr(s1), getMrpMemPtr(s2), n));
}

static void br_strcoll(BridgeMap *o, uc_engine *uc) {
    uint32_t s1, s2;
    uc_reg_read(uc, UC_ARM_REG_R0, &s1);
    uc_reg_read(uc, UC_ARM_REG_R1, &s2);
    SET_RET_V(strcoll(getMrpMemPtr(s1), getMrpMemPtr(s2)));
}

static void br_memchr(BridgeMap *o, uc_engine *uc) {
    uint32_t s, c, n;
    uc_reg_read(uc, UC_ARM_REG_R0, &s);
    uc_reg_read(uc, UC_ARM_REG_R1, &c);
    uc_reg_read(uc, UC_ARM_REG_R2, &n);
    void *ret = memchr(getMrpMemPtr(s), (int)c, n);
    if (ret == NULL) {
        SET_RET_V((uint32_t)NULL);
        return;
    }
    SET_RET_V(toMrpMemAddr(ret));
}

static void br_strlen(BridgeMap *o, uc_engine *uc) {
    uint32_t s;
    uc_reg_read(uc, UC_ARM_REG_R0, &s);
    SET_RET_V((uint32_t)strlen(getMrpMemPtr(s)));
}

static void br_strstr(BridgeMap *o, uc_engine *uc) {
    uint32_t s1, s2;
    uc_reg_read(uc, UC_ARM_REG_R0, &s1);
    uc_reg_read(uc, UC_ARM_REG_R1, &s2);
    char *ret = strstr(getMrpMemPtr(s1), getMrpMemPtr(s2));
    if (ret == NULL) {
        SET_RET_V((uint32_t)NULL);
        return;
    }
    SET_RET_V(toMrpMemAddr(ret));
}

// 获取参数的工具方法，第一个参数n=0
static uint32_t getArg(uc_engine *uc, uint32_t n) {
    uint32_t v;
    if (n <= 3) {  // 前四个参数直接从寄存器读
        uc_reg_read(uc, UC_ARM_REG_R0 + n, &v);
        return v;
    }

    uint32_t addr;
    uc_reg_read(uc, UC_ARM_REG_SP, &addr);

    addr += (n - 4) * 4;
    uc_mem_read(uc, addr, &v, 4);
    return v;
}

static int br_append_text(char *dst, int cursor, int limit, const char *src) {
    if (cursor >= limit || src == NULL) {
        return cursor;
    }
    while (*src && cursor < limit) {
        dst[cursor++] = *src++;
    }
    return cursor;
}

static int br_format_string(char *dst, int limit, const char *fmt, uc_engine *uc, uint32_t firstArgIndex) {
    int cursor = 0;
    uint32_t argIndex = firstArgIndex;
    while (*fmt && cursor < limit) {
        if (*fmt != '%') {
            dst[cursor++] = *fmt++;
            continue;
        }
        ++fmt;
        if (*fmt == '%') {
            dst[cursor++] = *fmt++;
            continue;
        }
        while (*fmt == '0' || *fmt == '-' || *fmt == '+' || *fmt == ' ' || *fmt == '#') {
            ++fmt;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            ++fmt;
        }
        if (*fmt == '.') {
            ++fmt;
            while (*fmt >= '0' && *fmt <= '9') {
                ++fmt;
            }
        }
        while (*fmt == 'l' || *fmt == 'h' || *fmt == 'z' || *fmt == 't') {
            ++fmt;
        }
        if (*fmt == '\0') {
            break;
        }

        char temp[128] = {0};
        switch (*fmt) {
            case 'd':
            case 'i': {
                int32_t v = (int32_t)getArg(uc, argIndex++);
                snprintf(temp, sizeof(temp), "%d", v);
                cursor = br_append_text(dst, cursor, limit, temp);
                break;
            }
            case 'u': {
                uint32_t v = getArg(uc, argIndex++);
                snprintf(temp, sizeof(temp), "%u", v);
                cursor = br_append_text(dst, cursor, limit, temp);
                break;
            }
            case 'x': {
                uint32_t v = getArg(uc, argIndex++);
                snprintf(temp, sizeof(temp), "%x", v);
                cursor = br_append_text(dst, cursor, limit, temp);
                break;
            }
            case 'X': {
                uint32_t v = getArg(uc, argIndex++);
                snprintf(temp, sizeof(temp), "%X", v);
                cursor = br_append_text(dst, cursor, limit, temp);
                break;
            }
            case 'c': {
                char c = (char)getArg(uc, argIndex++);
                if (cursor < limit) {
                    dst[cursor++] = c;
                }
                break;
            }
            case 'p': {
                uint32_t v = getArg(uc, argIndex++);
                snprintf(temp, sizeof(temp), "0x%X", v);
                cursor = br_append_text(dst, cursor, limit, temp);
                break;
            }
            case 's': {
                uint32_t ptr = getArg(uc, argIndex++);
                const char *s = ptr ? (const char *)getMrpMemPtr(ptr) : "(null)";
                cursor = br_append_text(dst, cursor, limit, s);
                break;
            }
            default:
                if (cursor < limit) {
                    dst[cursor++] = '%';
                }
                if (cursor < limit) {
                    dst[cursor++] = *fmt;
                }
                break;
        }
        ++fmt;
    }
    if (cursor < limit) {
        dst[cursor] = '\0';
    } else if (limit >= 0) {
        dst[limit] = '\0';
    }
    return cursor;
}

static void br_sprintf(BridgeMap *o, uc_engine *uc) {
    uint32_t dstAddr, fmtAddr;
    uc_reg_read(uc, UC_ARM_REG_R0, &dstAddr);
    uc_reg_read(uc, UC_ARM_REG_R1, &fmtAddr);
    char *dst = getMrpMemPtr(dstAddr);
    const char *fmt = getMrpMemPtr(fmtAddr);
    char buffer[2048] = {0};
    int written = br_format_string(buffer, (int)sizeof(buffer) - 1, fmt, uc, 2);
    strcpy(dst, buffer);
    SET_RET_V(written);
}

static void br_atoi(BridgeMap *o, uc_engine *uc) {
    uint32_t nptr;
    uc_reg_read(uc, UC_ARM_REG_R0, &nptr);
    SET_RET_V(atoi(getMrpMemPtr(nptr)));
}

static void br_strtoul(BridgeMap *o, uc_engine *uc) {
    uint32_t nptr, endptrAddr, base;
    uc_reg_read(uc, UC_ARM_REG_R0, &nptr);
    uc_reg_read(uc, UC_ARM_REG_R1, &endptrAddr);
    uc_reg_read(uc, UC_ARM_REG_R2, &base);
    char *s = getMrpMemPtr(nptr);
    char *hostEnd = NULL;
    unsigned long v = strtoul(s, &hostEnd, (int)base);
    if (endptrAddr != 0) {
        uint32_t endValue = hostEnd ? (uint32_t)(nptr + (uint32_t)(hostEnd - s)) : 0;
        uc_mem_write(uc, endptrAddr, &endValue, 4);
    }
    SET_RET_V((uint32_t)v);
}

static void br_mr_printf(BridgeMap *o, uc_engine *uc) {
    uint32_t fmtAddr;
    uc_reg_read(uc, UC_ARM_REG_R0, &fmtAddr);
    const char *fmt = getMrpMemPtr(fmtAddr);
    char buffer[1024] = {0};
    (void)br_format_string(buffer, (int)sizeof(buffer) - 1, fmt, uc, 1);
    puts(buffer);
}

static uint32_t g_blankGlyphAddr = 0;
static uint16_t g_blankGlyphSize = 32;
static const uint32_t BR_MR_FONT_SMALL = 0;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t bit;
} br_mr_screeninfo_t;

static void br_mr_getCharBitmap(BridgeMap *o, uc_engine *uc) {
    static uint32_t s_getCharBitmapCount = 0;
    // const char* (*mr_getCharBitmap)(uint16 ch, uint16 fontSize, int* width, int* height);
    uint32_t ch, fontSize, widthPtr, heightPtr;
    uc_reg_read(uc, UC_ARM_REG_R0, &ch);
    uc_reg_read(uc, UC_ARM_REG_R1, &fontSize);
    uc_reg_read(uc, UC_ARM_REG_R2, &widthPtr);
    uc_reg_read(uc, UC_ARM_REG_R3, &heightPtr);
    int32_t width = 0;
    int32_t height = 0;
    const uint8_t *bitmap = br_get_char_bitmap_host((uint16_t)ch, (uint16_t)fontSize, &width, &height);
    s_getCharBitmapCount++;
    if (s_getCharBitmapCount <= 30 || (s_getCharBitmapCount % 400) == 0) {
        BR_HILOGI("MRPDBG getCharBitmap #%{public}u ch=0x%{public}X font=%{public}u w=%{public}d h=%{public}d bitmap=%{public}d",
                  s_getCharBitmapCount, ch, fontSize, width, height, bitmap != NULL ? 1 : 0);
    }
    if (widthPtr != 0) {
        uc_mem_write(uc, widthPtr, &width, 4);
    }
    if (heightPtr != 0) {
        uc_mem_write(uc, heightPtr, &height, 4);
    }

    if (bitmap == NULL) {
        if (g_blankGlyphAddr == 0) {
            void *glyph = my_mallocExt(g_blankGlyphSize);
            if (glyph != NULL) {
                memset(glyph, 0, g_blankGlyphSize);
                g_blankGlyphAddr = toMrpMemAddr(glyph);
            }
        }
        SET_RET_V(g_blankGlyphAddr);
        return;
    }

    uint32_t glyphBytes = (uint32_t)((width * height + 7) >> 3);
    void *copy = my_mallocExt(glyphBytes);
    if (copy == NULL) {
        SET_RET_V((uint32_t)NULL);
        return;
    }
    memcpy(copy, bitmap, glyphBytes);
    SET_RET_V(toMrpMemAddr(copy));
}

static void br_mr_getUserInfo(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_getUserInfo)(char* info);
    uint32_t infoPtr;
    uc_reg_read(uc, UC_ARM_REG_R0, &infoPtr);
    if (infoPtr != 0) {
        char *info = getMrpMemPtr(infoPtr);
        info[0] = '\0';
    }
    SET_RET_V(MR_SUCCESS);
}

static void br_mr_getScreenInfo(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_getScreenInfo)(mr_screeninfo* screeninfo);
    uint32_t screenInfoPtr;
    uc_reg_read(uc, UC_ARM_REG_R0, &screenInfoPtr);
    if (screenInfoPtr != 0) {
        br_mr_screeninfo_t *s = (br_mr_screeninfo_t *)getMrpMemPtr(screenInfoPtr);
        s->width = 240;
        s->height = 320;
        s->bit = 16;
    }
    SET_RET_V(MR_SUCCESS);
}

static void br_mr_timerStart_alias(BridgeMap *o, uc_engine *uc) {
    br_timerStart(o, uc);
}

static void br_mr_timerStop_alias(BridgeMap *o, uc_engine *uc) {
    br_timerStop(o, uc);
}

static void br_mr_getTime_alias(BridgeMap *o, uc_engine *uc) {
    br_get_uptime_ms(o, uc);
}

static void br_mr_getDatetime_alias(BridgeMap *o, uc_engine *uc) {
    br_getDatetime(o, uc);
}

static void br_mr_sleep_alias(BridgeMap *o, uc_engine *uc) {
    br_sleep(o, uc);
}

static void br_mr_ferrno(BridgeMap *o, uc_engine *uc) {
    (void)o;
    (void)uc;
    SET_RET_V(MR_FAILED);
}

static void br_mr_sendSms(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_sendSms)(char *pNumber, char *pContent, int32 encode);
    uint32_t pNumber, pContent, encode;
    uc_reg_read(uc, UC_ARM_REG_R0, &pNumber);
    uc_reg_read(uc, UC_ARM_REG_R1, &pContent);
    uc_reg_read(uc, UC_ARM_REG_R2, &encode);
    LOG("ext call %s(number=%s, encode=%u)\n", o->name, getMrpMemPtr(pNumber), encode);
    (void)pContent;
    SET_RET_V(MR_SUCCESS);
}

static void br_mr_call(BridgeMap *o, uc_engine *uc) {
    uint32_t number;
    uc_reg_read(uc, UC_ARM_REG_R0, &number);
    LOG("ext call %s(number=%s)\n", o->name, getMrpMemPtr(number));
}

static void br_mr_getNetworkID(BridgeMap *o, uc_engine *uc) {
    (void)o;
    (void)uc;
    SET_RET_V(0);
}

static void br_mr_connectWAP(BridgeMap *o, uc_engine *uc) {
    uint32_t wap;
    uc_reg_read(uc, UC_ARM_REG_R0, &wap);
    LOG("ext call %s(%s)\n", o->name, getMrpMemPtr(wap));
}

typedef struct {
    uint8_t used;
    int16_t itemCount;
    int32_t focusIndex;
} br_menu_t;

static br_menu_t g_brMenus[16];
static uint8_t g_brWins[16];

static void br_mr_menuCreate(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_menuCreate)(const char *title, int16 num);
    uint32_t title;
    int32_t num;
    uc_reg_read(uc, UC_ARM_REG_R0, &title);
    uc_reg_read(uc, UC_ARM_REG_R1, &num);
    LOG("ext call %s(title=%s, num=%d)\n", o->name, getMrpMemPtr(title), num);
    for (int32_t i = 0; i < (int32_t)(sizeof(g_brMenus) / sizeof(g_brMenus[0])); ++i) {
        if (!g_brMenus[i].used) {
            g_brMenus[i].used = 1;
            g_brMenus[i].itemCount = (int16_t)(num > 0 ? num : 0);
            g_brMenus[i].focusIndex = 0;
            SET_RET_V(i + 1);
            return;
        }
    }
    SET_RET_V(MR_FAILED);
}

static br_menu_t *br_get_menu(int32_t handle) {
    if (handle <= 0 || handle > (int32_t)(sizeof(g_brMenus) / sizeof(g_brMenus[0]))) {
        return NULL;
    }
    br_menu_t *menu = &g_brMenus[handle - 1];
    return menu->used ? menu : NULL;
}

static void br_mr_menuSetItem(BridgeMap *o, uc_engine *uc) {
    uint32_t handle, text, index;
    uc_reg_read(uc, UC_ARM_REG_R0, &handle);
    uc_reg_read(uc, UC_ARM_REG_R1, &text);
    uc_reg_read(uc, UC_ARM_REG_R2, &index);
    LOG("ext call %s(menu=%u, text=%s, index=%u)\n", o->name, handle, getMrpMemPtr(text), index);
    br_menu_t *menu = br_get_menu((int32_t)handle);
    if (!menu) {
        SET_RET_V(MR_FAILED);
        return;
    }
    if (menu->itemCount <= 0 && (int32_t)index >= 0) {
        menu->itemCount = (int16_t)(index + 1);
    }
    SET_RET_V(MR_SUCCESS);
}

static void br_mr_menuShow(BridgeMap *o, uc_engine *uc) {
    uint32_t handle;
    uc_reg_read(uc, UC_ARM_REG_R0, &handle);
    LOG("ext call %s(menu=%u)\n", o->name, handle);
    SET_RET_V(br_get_menu((int32_t)handle) ? MR_SUCCESS : MR_FAILED);
}

static void br_mr_menuRelease(BridgeMap *o, uc_engine *uc) {
    uint32_t handle;
    uc_reg_read(uc, UC_ARM_REG_R0, &handle);
    LOG("ext call %s(menu=%u)\n", o->name, handle);
    br_menu_t *menu = br_get_menu((int32_t)handle);
    if (!menu) {
        SET_RET_V(MR_FAILED);
        return;
    }
    memset(menu, 0, sizeof(*menu));
    SET_RET_V(MR_SUCCESS);
}

static void br_mr_menuRefresh(BridgeMap *o, uc_engine *uc) {
    uint32_t handle;
    uc_reg_read(uc, UC_ARM_REG_R0, &handle);
    LOG("ext call %s(menu=%u)\n", o->name, handle);
    SET_RET_V(br_get_menu((int32_t)handle) ? MR_SUCCESS : MR_FAILED);
}

static void br_mr_winCreate(BridgeMap *o, uc_engine *uc) {
    (void)o;
    (void)uc;
    for (int32_t i = 0; i < (int32_t)(sizeof(g_brWins) / sizeof(g_brWins[0])); ++i) {
        if (!g_brWins[i]) {
            g_brWins[i] = 1;
            SET_RET_V(i + 1);
            return;
        }
    }
    SET_RET_V(MR_FAILED);
}

static void br_mr_winRelease(BridgeMap *o, uc_engine *uc) {
    uint32_t handle;
    uc_reg_read(uc, UC_ARM_REG_R0, &handle);
    LOG("ext call %s(win=%u)\n", o->name, handle);
    if (handle <= 0 || handle > (uint32_t)(sizeof(g_brWins) / sizeof(g_brWins[0])) || !g_brWins[handle - 1]) {
        SET_RET_V(MR_FAILED);
        return;
    }
    g_brWins[handle - 1] = 0;
    SET_RET_V(MR_SUCCESS);
}

// 实际上mrc_refreshScreen()是调用的这个方法
static void br_mr_drawBitmap(BridgeMap *o, uc_engine *uc) {
    // typedef void (*T_mr_drawBitmap)(uint16* bmp, int16 x, int16 y, uint16 w, uint16 h);
    uint32_t bmp, x, y, w, h;

    uc_reg_read(uc, UC_ARM_REG_R0, &bmp);
    uc_reg_read(uc, UC_ARM_REG_R1, &x);
    uc_reg_read(uc, UC_ARM_REG_R2, &y);
    uc_reg_read(uc, UC_ARM_REG_R3, &w);

    uint32_t sp;
    uc_reg_read(uc, UC_ARM_REG_SP, &sp);
    uc_mem_read(uc, sp, &h, 4);

    LOG("ext call %s(0x%X, %d, %d, %u, %u)\n", o->name, bmp, x, y, w, h);
    guiDrawBitmap(getMrpMemPtr(bmp), x, y, w, h);
}

static void br_mr_open(BridgeMap *o, uc_engine *uc) {
    // typedef int32 (*T_mr_open)(const char* filename,  uint32 mode);
    uint32_t filename, mode;
    uc_reg_read(uc, UC_ARM_REG_R0, &filename);
    uc_reg_read(uc, UC_ARM_REG_R1, &mode);
    char *filenameStr = getMrpMemPtr(filename);
    int32_t ret = my_open(filenameStr, mode);
    LOG("ext call %s(0x%X[%s], 0x%X): %d\n", o->name, filename, filenameStr, mode, ret);
    SET_RET_V(ret);
}

static void br_mr_close(BridgeMap *o, uc_engine *uc) {
    // typedef int32 (*T_mr_close)(int32 f);
    uint32_t f, ret;
    uc_reg_read(uc, UC_ARM_REG_R0, &f);
    ret = my_close(f);
    LOG("ext call %s(%d): %d\n", o->name, f, ret);
    SET_RET_V(ret);
}

static void br_mr_write(BridgeMap *o, uc_engine *uc) {
    // typedef int32 (*T_mr_write)(int32 f,void *p,uint32 l);
    uint32_t f, p, l, ret;

    uc_reg_read(uc, UC_ARM_REG_R0, &f);
    uc_reg_read(uc, UC_ARM_REG_R1, &p);
    uc_reg_read(uc, UC_ARM_REG_R2, &l);

    LOG("ext call %s(0x%X, 0x%X, 0x%X)\n", o->name, f, p, l);
    LOG("ext call %s([%u], [%u], [%u])\n", o->name, f, p, l);

    char *buf = malloc(l);
    uc_mem_read(uc, p, buf, l);
    ret = my_write(f, buf, l);
    free(buf);

    SET_RET_V(ret);
}

static void br_mr_read(BridgeMap *o, uc_engine *uc) {
    // typedef int32 (*T_mr_read)(int32 f,void *p,uint32 l);
    uint32_t f, p, l, ret;
    uc_reg_read(uc, UC_ARM_REG_R0, &f);
    uc_reg_read(uc, UC_ARM_REG_R1, &p);
    uc_reg_read(uc, UC_ARM_REG_R2, &l);
    char *buf = getMrpMemPtr(p);
    ret = my_read(f, buf, l);
    LOG("ext call %s(%d, 0x%X, %u): %d\n", o->name, f, p, l, ret);
    SET_RET_V(ret);
}

static void br_mr_seek(BridgeMap *o, uc_engine *uc) {
    // typedef int32 (*T_mr_seek)(int32 f, int32 pos, int method);
    uint32_t f, pos, method, ret;
    uc_reg_read(uc, UC_ARM_REG_R0, &f);
    uc_reg_read(uc, UC_ARM_REG_R1, &pos);
    uc_reg_read(uc, UC_ARM_REG_R2, &method);
    ret = my_seek(f, pos, method);
    LOG("ext call %s(%d, %d, 0x%X): %d\n", o->name, f, pos, method, ret);
    SET_RET_V(ret);
}

static void br_mr_getLen(BridgeMap *o, uc_engine *uc) {
    // typedef int32 (*T_mr_getLen)(const char* filename);
    uint32_t filename;
    uc_reg_read(uc, UC_ARM_REG_R0, &filename);
    char *filenameStr = getMrpMemPtr(filename);
    LOG("ext call %s(%s)\n", o->name, filenameStr);
    SET_RET_V(my_getLen(filenameStr));
}

static void br_mr_remove(BridgeMap *o, uc_engine *uc) {
    // typedef int32 (*T_mr_remove)(const char* filename);
    uint32_t filename;
    uc_reg_read(uc, UC_ARM_REG_R0, &filename);
    char *filenameStr = getMrpMemPtr(filename);
    LOG("ext call %s(%s)\n", o->name, filenameStr);
    SET_RET_V(my_remove(filenameStr));
}

static void br_mr_rename(BridgeMap *o, uc_engine *uc) {
    // typedef int32 (*T_mr_rename)(const char* oldname, const char* newname);
    uint32_t oldname, newname;
    uc_reg_read(uc, UC_ARM_REG_R0, &oldname);
    uc_reg_read(uc, UC_ARM_REG_R1, &newname);
    char *oldnameStr = getMrpMemPtr(oldname);
    char *newnameStr = getMrpMemPtr(newname);
    LOG("ext call %s(%s, %s)\n", o->name, oldnameStr, newnameStr);
    SET_RET_V(my_rename(oldnameStr, newnameStr));
}

static void br_mr_mkDir(BridgeMap *o, uc_engine *uc) {
    // typedef int32 (*T_mr_mkDir)(const char* name);
    uint32_t name;
    uc_reg_read(uc, UC_ARM_REG_R0, &name);
    char *nameStr = getMrpMemPtr(name);
    LOG("ext call %s(%s)\n", o->name, nameStr);
    SET_RET_V(my_mkDir(nameStr));
}

static void br_mr_rmDir(BridgeMap *o, uc_engine *uc) {
    // typedef int32 (*T_mr_rmDir)(const char* name);
    uint32_t name;
    uc_reg_read(uc, UC_ARM_REG_R0, &name);
    char *nameStr = getMrpMemPtr(name);
    LOG("ext call %s(%s)\n", o->name, nameStr);
    SET_RET_V(my_rmDir(nameStr));
}

static uint64_t uptime_ms;
static void br_get_uptime_ms_init(BridgeMap *o, uc_engine *uc, uint32_t addr) {
    LOG("br_%s_init() 0x%X[%u]\n", o->name, addr, addr);
    uptime_ms = (uint64_t)get_uptime_ms();
    uc_mem_write(uc, addr, &addr, 4);
}

static void br_get_uptime_ms(BridgeMap *o, uc_engine *uc) {
    // uint32 (*get_uptime_ms)(void);
    uint32_t ret = (uint32_t)((uint64_t)get_uptime_ms() - uptime_ms);
    LOG("ext call %s(): 0x%X[%u]\n", o->name, ret, ret);
    SET_RET_V(ret);
}

static void br_log(BridgeMap *o, uc_engine *uc) {
    // void (*log)(char *msg);
    uint32_t msg;
    uc_reg_read(uc, UC_ARM_REG_R0, &msg);

    char *str = (char *)getMrpMemPtr(msg);
    // LOG("ext call %s('%s')\n", o->name, str);
    puts(str);
    // dumpREG(uc);
}

static void br_mem_get(BridgeMap *o, uc_engine *uc) {
    // int32 (*mem_get)(char **mem_base, uint32 *mem_len);
    uint32_t mem_base, mem_len;
    uc_reg_read(uc, UC_ARM_REG_R0, &mem_base);
    uc_reg_read(uc, UC_ARM_REG_R1, &mem_len);

    LOG("ext call %s()\n", o->name);

    uint32_t len = 1024 * 1024 * 4;
    uint32_t buffer = toMrpMemAddr(my_mallocExt(len));

    printf("br_mem_get base=0x%X len=%d(%d kb) =================\n", buffer, len, len / 1024);

    // *mem_base = buffer;
    uc_mem_write(uc, mem_base, &buffer, 4);
    // *mem_len = len;
    uc_mem_write(uc, mem_len, &len, 4);

    SET_RET_V(MR_SUCCESS);
}

static void br_mem_free(BridgeMap *o, uc_engine *uc) {
    // int32 (*mem_free)(char *mem, uint32 mem_len);
    uint32_t mem, mem_len;
    uc_reg_read(uc, UC_ARM_REG_R0, &mem);
    uc_reg_read(uc, UC_ARM_REG_R1, &mem_len);

    LOG("ext call %s(0x%X, 0x%X)\n", o->name, mem, mem_len);
    my_freeExt(getMrpMemPtr(mem));
    SET_RET_V(MR_SUCCESS);
}

static void br_timerStop(BridgeMap *o, uc_engine *uc) {
    // int32 (*timerStop)(void);
    LOG("ext call %s()\n", o->name);
    SET_RET_V(timerStop());
}

static void br_timerStart(BridgeMap *o, uc_engine *uc) {
    // int32 (*timerStart)(uint16 t);
    LOG("ext call %s()\n", o->name);
    int32_t t;
    uc_reg_read(uc, UC_ARM_REG_R0, &t);
    SET_RET_V(timerStart(t));
}

static void br_test(BridgeMap *o, uc_engine *uc) {
    // void (*test)(void);
    LOG("ext call %s()\n", o->name);
}

static void br_exit(BridgeMap *o, uc_engine *uc) {
    // void (*exit)(void);
    LOG("ext call %s()\n", o->name);
    puts("mythroad exit requested.\n");
    vmrpHostOnExit();
    // On HarmonyOS app process, calling exit() from bridged MRP code aborts appspawn.
    // Treat this as "exit current MRP runtime" instead of terminating the host process.
    uc_emu_stop(uc);
    SET_RET_V(MR_SUCCESS);
}

static void br_srand(BridgeMap *o, uc_engine *uc) {
    // void (*srand)(uint32 seed);
    LOG("ext call %s()\n", o->name);
    uint32_t seed;
    uc_reg_read(uc, UC_ARM_REG_R0, &seed);
    srand(seed);
}

static void br_rand(BridgeMap *o, uc_engine *uc) {
    // int32 (*rand)(void);
    LOG("ext call %s()\n", o->name);
    SET_RET_V(rand());
}

static void br_sleep(BridgeMap *o, uc_engine *uc) {
    // int32 (*sleep)(uint32 ms);
    uint32_t ms;
    uc_reg_read(uc, UC_ARM_REG_R0, &ms);
    LOG("ext call %s(%d)\n", o->name, ms);
    usleep(ms * 1000);  //注意 usleep 传的是 微秒 ，所以要 *1000
    SET_RET_V(MR_SUCCESS);
}

static void br_info(BridgeMap *o, uc_engine *uc) {
    // int32 (*info)(const char *filename);
    LOG("ext call %s()\n", o->name);
    uint32_t filename;
    uc_reg_read(uc, UC_ARM_REG_R0, &filename);
    SET_RET_V(my_info(getMrpMemPtr(filename)))
}

static void br_opendir(BridgeMap *o, uc_engine *uc) {
    // int32 (*opendir)(const char *name);
    LOG("ext call %s()\n", o->name);
    uint32_t name;
    uc_reg_read(uc, UC_ARM_REG_R0, &name);
    SET_RET_V(my_opendir(getMrpMemPtr(name)))
}

#define READDIR_SHARED_MEM_SIZE 128
static char *readdirSharedMem;  // 文件名的共享内存
static void br_readdir_init(BridgeMap *o, uc_engine *uc, uint32_t addr) {
    LOG("br_%s_init() 0x%X[%u]\n", o->name, addr, addr);
    readdirSharedMem = (char *)my_mallocExt(READDIR_SHARED_MEM_SIZE);
    readdirSharedMem[READDIR_SHARED_MEM_SIZE - 1] = '\0';
    uc_mem_write(uc, addr, &addr, 4);
}

static void br_readdir(BridgeMap *o, uc_engine *uc) {
    // char *(*readdir)(int32 f);
    LOG("ext call %s()\n", o->name);
    int32_t f;
    uc_reg_read(uc, UC_ARM_REG_R0, &f);

    char *r = my_readdir(f);
    if (r != NULL) {
        strncpy(readdirSharedMem, r, READDIR_SHARED_MEM_SIZE - 1);
        SET_RET_V(toMrpMemAddr(readdirSharedMem));
    } else {
        SET_RET_V((uint32_t)NULL);
    }
}

static void br_closedir(BridgeMap *o, uc_engine *uc) {
    // int32 (*closedir)(int32 f);
    LOG("ext call %s()\n", o->name);
    int32_t f;
    uc_reg_read(uc, UC_ARM_REG_R0, &f);
    SET_RET_V(my_closedir(f));
}

static void br_getDatetime(BridgeMap *o, uc_engine *uc) {
    // int32 (*getDatetime)(mr_datetime *datetime);
    LOG("ext call %s()\n", o->name);
    uint32_t datetime;
    uc_reg_read(uc, UC_ARM_REG_R0, &datetime);
    SET_RET_V(getDatetime(getMrpMemPtr(datetime)));
}

// Some mr_plat/mr_platEx codes are heavily used by non-game MRP apps.
// Keep these implemented in bridge instead of returning "not implemented".
#define BR_MR_PLAT_VALUE_BASE 1000
#define BR_MR_CONNECT 1001
#define BR_MR_CHARACTER_HEIGHT 1201
#define BR_MR_CHECK_TOUCH 1205
#define BR_MR_GET_HANDSET_LG 1206
#define BR_MR_GET_RAND 1211
#define BR_MR_MSDC_QUERY 1218
#define BR_MR_TOUCH_SCREEN (BR_MR_PLAT_VALUE_BASE + 1)
#define BR_MR_CHINESE BR_MR_PLAT_VALUE_BASE
#define BR_MR_MSDC_OK (BR_MR_PLAT_VALUE_BASE + 1)
#define BR_MR_SWITCHPATH 1204

static void br_mr_plat(BridgeMap *o, uc_engine *uc) {
    int32_t code, param;
    uc_reg_read(uc, UC_ARM_REG_R0, &code);
    uc_reg_read(uc, UC_ARM_REG_R1, &param);
    LOG("ext call %s(code=%d, param=%d)\n", o->name, code, param);

    switch (code) {
        case BR_MR_CONNECT:
            SET_RET_V(my_getSocketState(param));
            return;
        case BR_MR_GET_RAND:
            if (param <= 0) {
                SET_RET_V(BR_MR_PLAT_VALUE_BASE);
                return;
            }
            srand((unsigned int)get_uptime_ms());
            SET_RET_V(BR_MR_PLAT_VALUE_BASE + rand() % param);
            return;
        case BR_MR_CHECK_TOUCH:
            SET_RET_V(BR_MR_TOUCH_SCREEN);
            return;
        case BR_MR_GET_HANDSET_LG:
            SET_RET_V(BR_MR_CHINESE);
            return;
        case BR_MR_MSDC_QUERY:
            SET_RET_V(BR_MR_MSDC_OK);
            return;
        default:
            SET_RET_V(MR_IGNORE);
            return;
    }
}

typedef struct {
    int32_t total;
    int32_t tunit;
    int32_t account;
    int32_t unit;
} br_mr_free_space_t;

static br_mr_free_space_t g_br_mr_free_space;
static int32_t g_br_word_info = (16 << 24) | (8 << 16) | (16 << 8) | 16;

static uint32_t br_out_ptr_to_mrp(void *hostPtr, uint32_t lenHint) {
    if (hostPtr == NULL) {
        return 0;
    }
    uint8_t *base = (uint8_t *)getMrpMemPtr(START_ADDRESS);
    uint8_t *end = (uint8_t *)getMrpMemPtr(END_ADDRESS);
    uint8_t *p = (uint8_t *)hostPtr;
    if (p >= base && p < end) {
        return toMrpMemAddr(hostPtr);
    }
    if (lenHint == 0) {
        return 0;
    }
    void *copy = my_mallocExt(lenHint);
    if (copy == NULL) {
        return 0;
    }
    memcpy(copy, hostPtr, lenHint);
    return toMrpMemAddr(copy);
}

static void br_mr_platEx(BridgeMap *o, uc_engine *uc) {
    int32_t code, input_len;
    uint32_t inputPtr, outputPtrAddr, outputLenAddr, cb;
    uc_reg_read(uc, UC_ARM_REG_R0, &code);
    uc_reg_read(uc, UC_ARM_REG_R1, &inputPtr);
    uc_reg_read(uc, UC_ARM_REG_R2, &input_len);
    uc_reg_read(uc, UC_ARM_REG_R3, &outputPtrAddr);
    outputLenAddr = getArg(uc, 4);
    cb = getArg(uc, 5);
    LOG("ext call %s(code=%d, input=0x%X, input_len=%d, outPtr=0x%X, outLen=0x%X, cb=0x%X)\n",
        o->name, code, inputPtr, input_len, outputPtrAddr, outputLenAddr, cb);

    (void)cb;

    uint32_t outMrpAddr = 0;
    int32_t outLen = 0;
    int32_t ret = MR_IGNORE;

    switch (code) {
        case BR_MR_SWITCHPATH:
            ret = MR_SUCCESS;
            break;
        case BR_MR_CHARACTER_HEIGHT:
            outLen = 4;
            outMrpAddr = br_out_ptr_to_mrp(&g_br_word_info, (uint32_t)outLen);
            ret = outMrpAddr ? MR_SUCCESS : MR_FAILED;
            break;
        case 1305: {  // MR_GET_FREE_SPACE
            char disk = inputPtr ? *(char *)getMrpMemPtr(inputPtr) : 0;
            switch (disk) {
                case 'A':
                case 'a':
                    g_br_mr_free_space.total = 1722;
                    g_br_mr_free_space.tunit = 1024;
                    g_br_mr_free_space.account = 1271;
                    g_br_mr_free_space.unit = 1024;
                    break;
                case 'B':
                case 'b':
                    g_br_mr_free_space.total = 95;
                    g_br_mr_free_space.tunit = 1024;
                    g_br_mr_free_space.account = 77;
                    g_br_mr_free_space.unit = 1024;
                    break;
                case 'C':
                case 'c':
                    g_br_mr_free_space.total = 1874;
                    g_br_mr_free_space.tunit = 1024 * 1024;
                    g_br_mr_free_space.account = 1873;
                    g_br_mr_free_space.unit = 1024 * 1024;
                    break;
                default:
                    SET_RET_V(MR_IGNORE);
                    return;
            }
            outLen = (int32_t)sizeof(g_br_mr_free_space);
            outMrpAddr = br_out_ptr_to_mrp(&g_br_mr_free_space, (uint32_t)outLen);
            ret = outMrpAddr ? MR_SUCCESS : MR_FAILED;
            break;
        }
        case 1116: {  // build time
            static char buildInfo[32];
            int n = snprintf(buildInfo, sizeof(buildInfo), "%s %s", __TIME__, __DATE__);
            if (n < 0) {
                SET_RET_V(MR_FAILED);
                return;
            }
            if (n >= (int)sizeof(buildInfo)) {
                n = (int)sizeof(buildInfo) - 1;
            }
            outLen = n + 1;
            outMrpAddr = br_out_ptr_to_mrp(buildInfo, (uint32_t)outLen);
            ret = outMrpAddr ? MR_SUCCESS : MR_FAILED;
            break;
        }
        default:
            ret = MR_IGNORE;
            break;
    }

    if (outputPtrAddr) {
        uc_mem_write(uc, outputPtrAddr, &outMrpAddr, 4);
    }
    if (outputLenAddr) {
        uc_mem_write(uc, outputLenAddr, &outLen, 4);
    }
    SET_RET_V(ret);
}

static void br_mr_findStart(BridgeMap *o, uc_engine *uc) {
    uint32_t name, buffer, len;
    uc_reg_read(uc, UC_ARM_REG_R0, &name);
    uc_reg_read(uc, UC_ARM_REG_R1, &buffer);
    uc_reg_read(uc, UC_ARM_REG_R2, &len);
    LOG("ext call %s(name=%s, len=%u)\n", o->name, getMrpMemPtr(name), len);
    int32_t h = my_opendir(getMrpMemPtr(name));
    if (h != MR_FAILED && buffer && len > 0) {
        char *first = br_next_valid_dirent(h);
        if (first) {
            strncpy(getMrpMemPtr(buffer), first, len - 1);
            *((char *)getMrpMemPtr(buffer) + (len - 1)) = '\0';
        } else {
            *((char *)getMrpMemPtr(buffer)) = '\0';
        }
    }
    SET_RET_V(h);
}

static void br_mr_findGetNext(BridgeMap *o, uc_engine *uc) {
    int32_t h;
    uint32_t buffer, len;
    uc_reg_read(uc, UC_ARM_REG_R0, &h);
    uc_reg_read(uc, UC_ARM_REG_R1, &buffer);
    uc_reg_read(uc, UC_ARM_REG_R2, &len);
    LOG("ext call %s(handle=%d, len=%u)\n", o->name, h, len);
    char *next = br_next_valid_dirent(h);
    if (!next || !buffer || len == 0) {
        SET_RET_V(MR_FAILED);
        return;
    }
    strncpy(getMrpMemPtr(buffer), next, len - 1);
    *((char *)getMrpMemPtr(buffer) + (len - 1)) = '\0';
    SET_RET_V(MR_SUCCESS);
}

static void br_mr_findStop(BridgeMap *o, uc_engine *uc) {
    int32_t h;
    uc_reg_read(uc, UC_ARM_REG_R0, &h);
    LOG("ext call %s(handle=%d)\n", o->name, h);
    SET_RET_V(my_closedir(h));
}

static void br_mr_md5_init(BridgeMap *o, uc_engine *uc) {
    uint32_t pms;
    uc_reg_read(uc, UC_ARM_REG_R0, &pms);
    LOG("ext call %s(0x%X)\n", o->name, pms);
    mr_md5_init((md5_state_t *)getMrpMemPtr(pms));
}

static void br_mr_md5_append(BridgeMap *o, uc_engine *uc) {
    uint32_t pms, data, nbytes;
    uc_reg_read(uc, UC_ARM_REG_R0, &pms);
    uc_reg_read(uc, UC_ARM_REG_R1, &data);
    uc_reg_read(uc, UC_ARM_REG_R2, &nbytes);
    LOG("ext call %s(pms=0x%X, data=0x%X, nbytes=%u)\n", o->name, pms, data, nbytes);
    mr_md5_append((md5_state_t *)getMrpMemPtr(pms), (const md5_byte_t *)getMrpMemPtr(data), (int)nbytes);
}

static void br_mr_md5_finish(BridgeMap *o, uc_engine *uc) {
    uint32_t pms, digest;
    uc_reg_read(uc, UC_ARM_REG_R0, &pms);
    uc_reg_read(uc, UC_ARM_REG_R1, &digest);
    LOG("ext call %s(pms=0x%X, digest=0x%X)\n", o->name, pms, digest);
    mr_md5_finish((md5_state_t *)getMrpMemPtr(pms), (md5_byte_t *)getMrpMemPtr(digest));
}

static void br_mr_div(BridgeMap *o, uc_engine *uc) {
    int32_t a, b;
    uc_reg_read(uc, UC_ARM_REG_R0, &a);
    uc_reg_read(uc, UC_ARM_REG_R1, &b);
    LOG("ext call %s(%d, %d)\n", o->name, a, b);
    SET_RET_V(a / b);
}

static void br_mr_mod(BridgeMap *o, uc_engine *uc) {
    int32_t a, b;
    uc_reg_read(uc, UC_ARM_REG_R0, &a);
    uc_reg_read(uc, UC_ARM_REG_R1, &b);
    LOG("ext call %s(%d, %d)\n", o->name, a, b);
    SET_RET_V(a % b);
}

static void br_mr_platDrawChar(BridgeMap *o, uc_engine *uc) {
    static uint32_t s_platDrawCharCount = 0;
    uint32_t ch, x, y, color;
    uc_reg_read(uc, UC_ARM_REG_R0, &ch);
    uc_reg_read(uc, UC_ARM_REG_R1, &x);
    uc_reg_read(uc, UC_ARM_REG_R2, &y);
    uc_reg_read(uc, UC_ARM_REG_R3, &color);
    LOG("ext call %s(ch=%u, x=%d, y=%d, color=0x%X)\n", o->name, (unsigned)ch, (int32_t)x, (int32_t)y, color);
    s_platDrawCharCount++;
    int32_t width = 0;
    int32_t height = 0;
    const uint8_t *bitmap = br_get_char_bitmap_host((uint16_t)ch, 1, &width, &height);  // mr_platDrawChar always uses 16-font
    if (s_platDrawCharCount <= 20 || (s_platDrawCharCount % 200) == 0) {
        BR_HILOGI("MRPDBG platDrawChar #%{public}u ch=0x%{public}X x=%{public}d y=%{public}d color=0x%{public}X bitmap=%{public}d",
                  s_platDrawCharCount, ch, (int32_t)x, (int32_t)y, color, bitmap != NULL ? 1 : 0);
    }
    if (bitmap == NULL || mr_table == NULL) {
        return;
    }
    uint8_t *tbl = (uint8_t *)mr_table;
    uint32_t mrScreenBufAddr = *(uint32_t *)(tbl + 0x16C);
    uint32_t mrScreenW = *(uint32_t *)(tbl + 0x170);
    uint32_t mrScreenH = *(uint32_t *)(tbl + 0x174);
    if (mrScreenW == 0) mrScreenW = 240;
    if (mrScreenH == 0) mrScreenH = 320;
    if (mrScreenBufAddr == 0) {
        return;
    }
    uint16_t *screen = (uint16_t *)getMrpMemPtr(mrScreenBufAddr);
    if (screen == NULL) {
        return;
    }

    // Match mythroad/dsm.c xl_font_sky16_drawChar behavior:
    // every row is 16 bits (2 bytes), scan from bit15 toward bit0.
    for (int32_t row = 0; row < 16; ++row) {
        uint16_t data = (uint16_t)bitmap[row * 2 + 1];
        data |= (uint16_t)(bitmap[row * 2] << 8);
        for (int32_t col = 0; data > 0; ++col) {
            if (data & 0x8000) {
                int32_t px = (int32_t)x + col;
                int32_t py = (int32_t)y + row;
                if (px >= 0 && py >= 0 && px < (int32_t)mrScreenW && py < (int32_t)mrScreenH) {
                    screen[py * (int32_t)mrScreenW + px] = (uint16_t)color;
                }
            }
            data = (uint16_t)(data << 1);
        }
    }
}

static uint16_t br_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((uint16_t)(r >> 3) << 11) | ((uint16_t)(g >> 2) << 5) | ((uint16_t)(b >> 3)));
}

typedef struct {
    uint16_t *buf;
    int32_t w;
    int32_t h;
} br_screen_ctx_t;

typedef struct {
    uint16_t *p;
    uint16_t w;
    uint16_t h;
    uint16_t x;
    uint16_t y;
} br_mr_bitmapDrawSt;

typedef struct {
    int16_t A;
    int16_t B;
    int16_t C;
    int16_t D;
    uint16_t rop;
} br_mr_transMatrixSt;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} br_mr_screenRectSt;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} br_mr_colourSt;

#define BR_MR_SPRITE_INDEX_MASK 0x03FF
#define BR_MR_SPRITE_TRANSPARENT 0x0400
#define BR_MR_TILE_SHIFT 11
#define BR_MR_ROTATE_0 0
#define BR_MR_ROTATE_90 1
#define BR_MR_ROTATE_180 2
#define BR_MR_ROTATE_270 3
#define BR_DRAW_TEXT_EX_IS_UNICODE 1
#define BR_DRAW_TEXT_EX_IS_AUTO_NEWLINE 2

enum {
    BR_BM_OR,
    BR_BM_XOR,
    BR_BM_COPY,
    BR_BM_NOT,
    BR_BM_MERGENOT,
    BR_BM_ANDNOT,
    BR_BM_TRANSPARENT,
    BR_BM_AND,
    BR_BM_GRAY,
    BR_BM_REVERSE
};

static uint8_t g_font16_bitmap[32];
static uint8_t g_font12_bitmap[24];
static int32_t g_font16_file = 0;
static int32_t g_font12_file = 0;

static int32_t br_abs32(int32_t v) {
    return v >= 0 ? v : -v;
}

static int32_t br_max32(int32_t a, int32_t b) {
    return a > b ? a : b;
}

static int32_t br_min32(int32_t a, int32_t b) {
    return a < b ? a : b;
}

static uint16_t br_swap16(uint16_t v) {
    return (uint16_t)((v << 8) | (v >> 8));
}

static bool br_get_screen_ctx(br_screen_ctx_t *ctx) {
    if (ctx == NULL || mr_table == NULL) {
        return false;
    }
    uint8_t *tbl = (uint8_t *)mr_table;
    uint32_t mrScreenBufAddr = *(uint32_t *)(tbl + 0x16C);
    uint32_t mrScreenW = *(uint32_t *)(tbl + 0x170);
    uint32_t mrScreenH = *(uint32_t *)(tbl + 0x174);
    if (mrScreenW == 0) mrScreenW = 240;
    if (mrScreenH == 0) mrScreenH = 320;
    if (mrScreenBufAddr == 0) {
        return false;
    }
    uint16_t *screen = (uint16_t *)getMrpMemPtr(mrScreenBufAddr);
    if (screen == NULL) {
        return false;
    }
    ctx->buf = screen;
    ctx->w = (int32_t)mrScreenW;
    ctx->h = (int32_t)mrScreenH;
    return true;
}

static void br_put_pixel(br_screen_ctx_t *ctx, int32_t x, int32_t y, uint16_t color) {
    if (ctx == NULL || ctx->buf == NULL) {
        return;
    }
    if (x < 0 || y < 0 || x >= ctx->w || y >= ctx->h) {
        return;
    }
    ctx->buf[y * ctx->w + x] = color;
}

static void br_fill_rect_565(br_screen_ctx_t *ctx, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
    if (ctx == NULL || ctx->buf == NULL || w <= 0 || h <= 0) {
        return;
    }
    int32_t maxX = br_min32(ctx->w, x + w);
    int32_t maxY = br_min32(ctx->h, y + h);
    int32_t minX = br_max32(0, x);
    int32_t minY = br_max32(0, y);
    if (maxX <= minX || maxY <= minY) {
        return;
    }
    for (int32_t dy = minY; dy < maxY; ++dy) {
        uint16_t *dst = ctx->buf + dy * ctx->w + minX;
        for (int32_t dx = minX; dx < maxX; ++dx) {
            *dst++ = color;
        }
    }
}

static uint16_t br_gbk_to_ucs2be(uint16_t gbCode) {
    if (gbCode < 0x8140 || gbCode > 0xFE4F) {
        return 0xFDFF;
    }
    int first = 0;
    int last = TAB_GB2UCS_8140_FE4F_SIZE - 1;
    while (last >= first) {
        int mid = (first + last) >> 1;
        gb2ucs_st data = tab_gb2ucs_8140_FE4F[mid];
        if (gbCode < data.gb) {
            last = mid - 1;
        } else if (gbCode > data.gb) {
            first = mid + 1;
        } else {
            return br_swap16((uint16_t)data.ucs);
        }
    }
    return 0xFDFF;
}

static uint16_t *br_ascii_or_gbk_to_u16(const char *src, int32_t *outBytes) {
    if (src == NULL) {
        if (outBytes) {
            *outBytes = 0;
        }
        return NULL;
    }
    size_t inLen = strlen(src);
    size_t maxChars = inLen;
    uint16_t *dst = (uint16_t *)my_mallocExt((uint32_t)((maxChars + 1) * 2));
    if (dst == NULL) {
        if (outBytes) {
            *outBytes = 0;
        }
        return NULL;
    }
    size_t di = 0;
    for (size_t i = 0; i < inLen;) {
        unsigned char c = (unsigned char)src[i];
        if (c <= 0x7F) {
            dst[di++] = (uint16_t)(c << 8);
            i++;
        } else if (c == 0x80) {
            dst[di++] = 0xAC20;  // '€' in UCS2BE
            i++;
        } else {
            if (i + 1 < inLen && src[i + 1] != '\0') {
                uint16_t gbCode = (uint16_t)(((uint16_t)c << 8) | (uint8_t)src[i + 1]);
                dst[di++] = br_gbk_to_ucs2be(gbCode);
                i += 2;
            } else {
                dst[di++] = 0xFDFF;
                break;
            }
        }
    }
    dst[di] = 0;
    if (outBytes) {
        *outBytes = (int32_t)((di + 1) * 2);
    }
    return dst;
}

static uint16_t *br_utf8_to_ucs2be(const char *src, int32_t *outBytes) {
    if (src == NULL) {
        if (outBytes) {
            *outBytes = 0;
        }
        return NULL;
    }

    size_t inLen = strlen(src);
    uint16_t *dst = (uint16_t *)my_mallocExt((uint32_t)((inLen + 1) * sizeof(uint16_t)));
    if (dst == NULL) {
        if (outBytes) {
            *outBytes = 0;
        }
        return NULL;
    }

    size_t di = 0;
    for (size_t i = 0; i < inLen;) {
        uint32_t cp = 0xFFFD;
        unsigned char c = (unsigned char)src[i];
        if (c < 0x80) {
            cp = c;
            i += 1;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < inLen) {
            cp = ((uint32_t)(c & 0x1F) << 6) | ((uint32_t)src[i + 1] & 0x3F);
            i += 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < inLen) {
            cp = ((uint32_t)(c & 0x0F) << 12) |
                 (((uint32_t)src[i + 1] & 0x3F) << 6) |
                 ((uint32_t)src[i + 2] & 0x3F);
            i += 3;
        } else if ((c & 0xF8) == 0xF0 && i + 3 < inLen) {
            cp = 0xFFFD;
            i += 4;
        } else {
            cp = 0xFFFD;
            i += 1;
        }
        if (cp > 0xFFFF) {
            cp = 0xFFFD;
        }
        dst[di++] = br_swap16((uint16_t)cp);
    }
    dst[di] = 0;
    if (outBytes) {
        *outBytes = (int32_t)((di + 1) * sizeof(uint16_t));
    }
    return dst;
}

static char *br_ucs2be_to_utf8(const uint8_t *src) {
    if (src == NULL) {
        return NULL;
    }

    size_t outLen = 1;
    for (size_t i = 0; src[i] != 0 || src[i + 1] != 0; i += 2) {
        uint16_t cp = (uint16_t)(((uint16_t)src[i] << 8) | src[i + 1]);
        if (cp < 0x80) {
            outLen += 1;
        } else if (cp < 0x800) {
            outLen += 2;
        } else {
            outLen += 3;
        }
    }

    char *dst = (char *)my_mallocExt((uint32_t)outLen);
    if (dst == NULL) {
        return NULL;
    }

    size_t di = 0;
    for (size_t i = 0; src[i] != 0 || src[i + 1] != 0; i += 2) {
        uint16_t cp = (uint16_t)(((uint16_t)src[i] << 8) | src[i + 1]);
        if (cp < 0x80) {
            dst[di++] = (char)cp;
        } else if (cp < 0x800) {
            dst[di++] = (char)(0xC0 | (cp >> 6));
            dst[di++] = (char)(0x80 | (cp & 0x3F));
        } else {
            dst[di++] = (char)(0xE0 | (cp >> 12));
            dst[di++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            dst[di++] = (char)(0x80 | (cp & 0x3F));
        }
    }
    dst[di] = '\0';
    return dst;
}

static bool br_is_common_text_codepoint(uint16_t cp) {
    return cp == 0x0009 || cp == 0x000A || cp == 0x000D ||
           (cp >= 0x0020 && cp <= 0x007E) ||
           (cp >= 0x3000 && cp <= 0x30FF) ||
           (cp >= 0x4E00 && cp <= 0x9FFF) ||
           (cp >= 0xFF00 && cp <= 0xFFEF);
}

static bool br_looks_like_ucs2be(const uint8_t *src) {
    if (src == NULL || (src[0] == 0 && src[1] == 0)) {
        return false;
    }

    int pairs = 0;
    int common = 0;
    int suspicious = 0;
    bool terminated = false;
    for (int i = 0; i < 256; i += 2) {
        if (src[i] == 0 && src[i + 1] == 0) {
            terminated = true;
            break;
        }
        uint16_t cp = (uint16_t)(((uint16_t)src[i] << 8) | src[i + 1]);
        pairs++;
        if (br_is_common_text_codepoint(cp)) {
            common++;
        } else if (cp < 0x20 || (cp >= 0xAC00 && cp <= 0xD7AF)) {
            suspicious++;
        }
    }
    return terminated && pairs > 0 && suspicious == 0 && common * 2 >= pairs;
}

static void br_append_utf8_codepoint(char *dst, size_t *cursor, uint16_t cp) {
    if (cp < 0x80) {
        dst[(*cursor)++] = (char)cp;
    } else if (cp < 0x800) {
        dst[(*cursor)++] = (char)(0xC0 | (cp >> 6));
        dst[(*cursor)++] = (char)(0x80 | (cp & 0x3F));
    } else {
        dst[(*cursor)++] = (char)(0xE0 | (cp >> 12));
        dst[(*cursor)++] = (char)(0x80 | ((cp >> 6) & 0x3F));
        dst[(*cursor)++] = (char)(0x80 | (cp & 0x3F));
    }
}

static char *br_gbk_to_utf8(const char *src) {
    if (src == NULL) {
        return NULL;
    }
    size_t inLen = strlen(src);
    char *dst = (char *)my_mallocExt((uint32_t)(inLen * 3 + 1));
    if (dst == NULL) {
        return NULL;
    }
    size_t di = 0;
    for (size_t i = 0; i < inLen;) {
        unsigned char c = (unsigned char)src[i];
        uint16_t cp = 0xFFFD;
        if (c <= 0x7F) {
            cp = c;
            i++;
        } else if (c == 0x80) {
            cp = 0x20AC;
            i++;
        } else if (i + 1 < inLen) {
            uint16_t gbCode = (uint16_t)(((uint16_t)c << 8) | (uint8_t)src[i + 1]);
            cp = br_swap16(br_gbk_to_ucs2be(gbCode));
            i += 2;
        } else {
            i++;
        }
        br_append_utf8_codepoint(dst, &di, cp);
    }
    dst[di] = '\0';
    return dst;
}

static uint16_t br_ucs2_to_gbk_char(uint16_t cp) {
    if (cp >= 0x4E00 && cp <= 0x9FA5) {
        return ucs2gb_4e00_9fa5[cp - 0x4E00];
    }
    int first = 0;
    int last = UCS2GB_OTHER_SIZE - 1;
    while (last >= first) {
        int mid = (first + last) >> 1;
        if (cp < ucs2gb_other[mid].ucs) {
            last = mid - 1;
        } else if (cp > ucs2gb_other[mid].ucs) {
            first = mid + 1;
        } else {
            return ucs2gb_other[mid].gb;
        }
    }
    return 0xA1F4;
}

static char *br_utf8_to_gbk(const char *src) {
    if (src == NULL) {
        return NULL;
    }
    size_t inLen = strlen(src);
    char *dst = (char *)my_mallocExt((uint32_t)(inLen * 2 + 1));
    if (dst == NULL) {
        return NULL;
    }

    size_t di = 0;
    for (size_t i = 0; i < inLen;) {
        uint32_t cp = 0xFFFD;
        unsigned char c = (unsigned char)src[i];
        if (c < 0x80) {
            cp = c;
            i += 1;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < inLen) {
            cp = ((uint32_t)(c & 0x1F) << 6) | ((uint32_t)src[i + 1] & 0x3F);
            i += 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < inLen) {
            cp = ((uint32_t)(c & 0x0F) << 12) |
                 (((uint32_t)src[i + 1] & 0x3F) << 6) |
                 ((uint32_t)src[i + 2] & 0x3F);
            i += 3;
        } else {
            i += 1;
        }

        if (cp <= 0x7F) {
            dst[di++] = (char)cp;
        } else {
            uint16_t gb = br_ucs2_to_gbk_char(cp <= 0xFFFF ? (uint16_t)cp : 0xFFFD);
            dst[di++] = (char)(gb >> 8);
            dst[di++] = (char)(gb & 0xFF);
        }
    }
    dst[di] = '\0';
    return dst;
}

static int32_t br_font_open(const char *path) {
    if (path == NULL) {
        return 0;
    }
    return my_open(path, MR_FILE_RDONLY);
}

static int32_t br_font_ensure_open(bool smallFont) {
    int32_t *fd = smallFont ? &g_font12_file : &g_font16_file;
    if (*fd > 0) {
        return *fd;
    }
    *fd = br_font_open(smallFont ? "system/gb12.uc2" : "system/gb16.uc2");
    if (*fd == 0) {
        *fd = br_font_open(smallFont ? "mythroad/system/gb12.uc2" : "mythroad/system/gb16.uc2");
    }
    if (*fd <= 0) {
        BR_HILOGI("MRPDBG font open failed small=%{public}d", smallFont ? 1 : 0);
    }
    return *fd;
}

static const uint8_t *br_get_char_bitmap_host(uint16_t ch, uint16_t fontSize, int32_t *width, int32_t *height) {
    bool smallFont = (fontSize == BR_MR_FONT_SMALL);
    int32_t fd = br_font_ensure_open(smallFont);
    if (fd <= 0) {
        return NULL;
    }

    int32_t charBytes = smallFont ? 24 : 32;
    if (width != NULL) {
        *width = (ch < 128) ? (smallFont ? 6 : 8) : (smallFont ? 12 : 16);
    }
    if (height != NULL) {
        *height = smallFont ? 12 : 16;
    }

    int32_t off = (int32_t)ch * charBytes;
    if (my_seek(fd, off, 0) != MR_SUCCESS) {
        return NULL;
    }
    uint8_t *buf = smallFont ? g_font12_bitmap : g_font16_bitmap;
    if (my_read(fd, buf, (uint32_t)charBytes) != charBytes) {
        return NULL;
    }
    return buf;
}

static void br_draw_glyph_ucs2be(br_screen_ctx_t *ctx, uint16_t ch, int32_t x, int32_t y, uint16_t color, uint16_t font) {
    int32_t width = 0;
    int32_t height = 0;
    const uint8_t *bitmap = br_get_char_bitmap_host(ch, font, &width, &height);
    if (bitmap == NULL || width <= 0 || height <= 0) {
        return;
    }
    int32_t bytesPerRow = (width + 7) >> 3;
    for (int32_t dy = 0; dy < height; ++dy) {
        for (int32_t dx = 0; dx < width; ++dx) {
            uint8_t byte = bitmap[dy * bytesPerRow + (dx >> 3)];
            if (byte & (uint8_t)(0x80 >> (dx & 7))) {
                br_put_pixel(ctx, x + dx, y + dy, color);
            }
        }
    }
}

static void br__mr_load_sms_cfg(BridgeMap *o, uc_engine *uc) {
    (void)o;
    (void)uc;
    SET_RET_V(MR_IGNORE);
}

static void br__mr_save_sms_cfg(BridgeMap *o, uc_engine *uc) {
    (void)o;
    (void)uc;
    SET_RET_V(MR_IGNORE);
}

static void br__DispUpEx(BridgeMap *o, uc_engine *uc) {
    (void)o;
    int32_t x, y;
    uint32_t w, h;
    uc_reg_read(uc, UC_ARM_REG_R0, &x);
    uc_reg_read(uc, UC_ARM_REG_R1, &y);
    uc_reg_read(uc, UC_ARM_REG_R2, &w);
    uc_reg_read(uc, UC_ARM_REG_R3, &h);

    if (mr_table == NULL || w == 0 || h == 0) {
        SET_RET_V(0);
        return;
    }

    // mr_screenBuf / mr_screen_w / mr_screen_h slots in mr_table_funcMap
    uint8_t *tbl = (uint8_t *)mr_table;
    uint32_t mrScreenBufAddr = *(uint32_t *)(tbl + 0x16C);
    uint32_t mrScreenW = *(uint32_t *)(tbl + 0x170);
    uint32_t mrScreenH = *(uint32_t *)(tbl + 0x174);
    if (mrScreenW == 0) mrScreenW = 240;
    if (mrScreenH == 0) mrScreenH = 320;

    if (mrScreenBufAddr == 0) {
        SET_RET_V(0);
        return;
    }

    uint16_t *screen = (uint16_t *)getMrpMemPtr(mrScreenBufAddr);
    if (screen == NULL) {
        SET_RET_V(0);
        return;
    }

    uint32_t sx = x < 0 ? 0 : (uint32_t)x;
    uint32_t sy = y < 0 ? 0 : (uint32_t)y;
    uint32_t ex = (uint32_t)x + w;
    uint32_t ey = (uint32_t)y + h;
    if (ex > mrScreenW) ex = mrScreenW;
    if (ey > mrScreenH) ey = mrScreenH;
    if (sx >= ex || sy >= ey) {
        SET_RET_V(0);
        return;
    }

    uint32_t cw = ex - sx;
    uint32_t ch = ey - sy;
    // Keep the original MRP semantics: pass the full screen buffer and
    // describe the dirty rect by x/y/w/h.
    guiDrawBitmap(screen, (int16_t)sx, (int16_t)sy, (uint16_t)cw, (uint16_t)ch);
    SET_RET_V(0);
}

static void br__DrawPoint(BridgeMap *o, uc_engine *uc) {
    (void)o;
    static uint32_t s_drawPointCount = 0;
    int32_t x, y;
    uint32_t nativecolor;
    uc_reg_read(uc, UC_ARM_REG_R0, &x);
    uc_reg_read(uc, UC_ARM_REG_R1, &y);
    uc_reg_read(uc, UC_ARM_REG_R2, &nativecolor);
    s_drawPointCount++;
    if (s_drawPointCount <= 40 || (s_drawPointCount % 5000) == 0) {
        BR_HILOGI("MRPDBG DrawPoint #%{public}u x=%{public}d y=%{public}d color=0x%{public}X",
                  s_drawPointCount, x, y, nativecolor);
    }
    br_screen_ctx_t ctx;
    if (!br_get_screen_ctx(&ctx)) {
        return;
    }
    br_put_pixel(&ctx, x, y, (uint16_t)nativecolor);
}

static void br__DrawBitmap(BridgeMap *o, uc_engine *uc) {
    (void)o;
    uint32_t p, w;
    int32_t x, y;
    uint32_t h, rop, transcoler;
    int32_t sx, sy, mw;
    uc_reg_read(uc, UC_ARM_REG_R0, &p);
    uc_reg_read(uc, UC_ARM_REG_R1, &x);
    uc_reg_read(uc, UC_ARM_REG_R2, &y);
    uc_reg_read(uc, UC_ARM_REG_R3, &w);
    h = getArg(uc, 4);
    rop = getArg(uc, 5);
    transcoler = getArg(uc, 6);
    sx = getArg(uc, 7);
    sy = getArg(uc, 8);
    mw = getArg(uc, 9);
    if (p == 0 || w == 0 || h == 0) {
        return;
    }

    br_screen_ctx_t ctx;
    if (!br_get_screen_ctx(&ctx)) {
        return;
    }

    uint16_t *src = (uint16_t *)getMrpMemPtr(p);
    if (src == NULL) {
        return;
    }
    if (mw == 0) {
        mw = (int32_t)w;
    }

    int32_t minY = br_max32(0, y);
    int32_t minX = br_max32(0, x);
    int32_t maxX = br_min32(ctx.w, x + (int32_t)w);
    int32_t maxY = br_min32(ctx.h, y + (int32_t)h);
    if (maxX <= minX || maxY <= minY) {
        return;
    }

    if (rop > BR_MR_SPRITE_TRANSPARENT) {
        uint16_t bitmapRop = (uint16_t)(rop & BR_MR_SPRITE_INDEX_MASK);
        uint16_t bitmapMode = (uint16_t)((rop >> BR_MR_TILE_SHIFT) & 0x3);
        uint16_t bitmapFlip = (uint16_t)((rop >> BR_MR_TILE_SHIFT) & 0x4);
        if (bitmapRop == BR_BM_TRANSPARENT || bitmapRop == BR_BM_COPY) {
            for (int32_t dy = minY; dy < maxY; ++dy) {
                uint16_t *dstp = ctx.buf + dy * ctx.w + minX;
                if (bitmapMode == BR_MR_ROTATE_0) {
                    uint16_t *srcp = bitmapFlip
                        ? src + ((int32_t)h - 1 - (dy - y)) * (int32_t)w + (minX - x)
                        : src + (dy - y) * (int32_t)w + (minX - x);
                    for (int32_t dx = minX; dx < maxX; ++dx) {
                        if (bitmapRop == BR_BM_COPY || *srcp != (uint16_t)transcoler) {
                            *dstp = *srcp;
                        }
                        ++dstp;
                        ++srcp;
                    }
                } else if (bitmapMode == BR_MR_ROTATE_90) {
                    uint16_t *srcp = bitmapFlip
                        ? src + ((int32_t)h - 1 - (minX - x)) * (int32_t)w + ((int32_t)w - 1 - (dy - y))
                        : src + (minX - x) * (int32_t)w + ((int32_t)w - 1 - (dy - y));
                    for (int32_t dx = minX; dx < maxX; ++dx) {
                        if (bitmapRop == BR_BM_COPY || *srcp != (uint16_t)transcoler) {
                            *dstp = *srcp;
                        }
                        ++dstp;
                        srcp = bitmapFlip ? (srcp - (int32_t)w) : (srcp + (int32_t)w);
                    }
                } else if (bitmapMode == BR_MR_ROTATE_180) {
                    uint16_t *srcp = bitmapFlip
                        ? src + (dy - y) * (int32_t)w + ((int32_t)w - 1 - (minX - x))
                        : src + ((int32_t)h - 1 - (dy - y)) * (int32_t)w + ((int32_t)w - 1 - (minX - x));
                    for (int32_t dx = minX; dx < maxX; ++dx) {
                        if (bitmapRop == BR_BM_COPY || *srcp != (uint16_t)transcoler) {
                            *dstp = *srcp;
                        }
                        ++dstp;
                        --srcp;
                    }
                } else {
                    uint16_t *srcp = bitmapFlip
                        ? src + (minX - x) * (int32_t)w + (dy - y)
                        : src + ((int32_t)h - 1 - (minX - x)) * (int32_t)w + (dy - y);
                    for (int32_t dx = minX; dx < maxX; ++dx) {
                        if (bitmapRop == BR_BM_COPY || *srcp != (uint16_t)transcoler) {
                            *dstp = *srcp;
                        }
                        ++dstp;
                        srcp = bitmapFlip ? (srcp + (int32_t)w) : (srcp - (int32_t)w);
                    }
                }
            }
        }
        return;
    }

    for (int32_t dy = minY; dy < maxY; ++dy) {
        uint16_t *dstp = ctx.buf + dy * ctx.w + minX;
        uint16_t *srcp = src + (dy - y + sy) * mw + (minX - x + sx);
        for (int32_t dx = minX; dx < maxX; ++dx) {
            uint16_t sv = *srcp;
            switch (rop) {
                case BR_BM_TRANSPARENT:
                    if (sv != (uint16_t)transcoler) *dstp = sv;
                    break;
                case BR_BM_COPY:
                    *dstp = sv;
                    break;
                case BR_BM_GRAY: {
                    if (sv != (uint16_t)transcoler) {
                        uint32_t r = (sv & 0xf800) >> 11;
                        uint32_t g = (sv & 0x07e0) >> 6;
                        uint32_t b = sv & 0x1f;
                        uint32_t gray = (r * 60 + g * 118 + b * 22) / 25;
                        *dstp = (uint16_t)(((gray & 0x1f) << 11) | (((gray << 1) & 0x3f) << 5) | (gray & 0x1f));
                    }
                    break;
                }
                case BR_BM_REVERSE:
                    if (sv != (uint16_t)transcoler) *dstp = (uint16_t)(~sv);
                    break;
                case BR_BM_OR:
                    *dstp = (uint16_t)(sv | *dstp);
                    break;
                case BR_BM_XOR:
                    *dstp = (uint16_t)(sv ^ *dstp);
                    break;
                case BR_BM_NOT:
                    *dstp = (uint16_t)(~sv);
                    break;
                case BR_BM_MERGENOT:
                    *dstp = (uint16_t)((~sv) | *dstp);
                    break;
                case BR_BM_ANDNOT:
                    *dstp = (uint16_t)((~sv) & *dstp);
                    break;
                case BR_BM_AND:
                    *dstp = (uint16_t)(sv & *dstp);
                    break;
                default:
                    break;
            }
            ++dstp;
            ++srcp;
        }
    }
}

static void br__DrawBitmapEx(BridgeMap *o, uc_engine *uc) {
    (void)o;
    static uint32_t s_drawBitmapExCount = 0;
    uint32_t srcAddr, dstAddr, w, h, transAddr, transColor;
    uc_reg_read(uc, UC_ARM_REG_R0, &srcAddr);
    uc_reg_read(uc, UC_ARM_REG_R1, &dstAddr);
    uc_reg_read(uc, UC_ARM_REG_R2, &w);
    uc_reg_read(uc, UC_ARM_REG_R3, &h);
    transAddr = getArg(uc, 4);
    transColor = getArg(uc, 5);
    if (srcAddr == 0 || dstAddr == 0 || transAddr == 0 || w == 0 || h == 0) {
        return;
    }
    br_mr_bitmapDrawSt *src = (br_mr_bitmapDrawSt *)getMrpMemPtr(srcAddr);
    br_mr_bitmapDrawSt *dst = (br_mr_bitmapDrawSt *)getMrpMemPtr(dstAddr);
    br_mr_transMatrixSt *trans = (br_mr_transMatrixSt *)getMrpMemPtr(transAddr);
    if (src == NULL || dst == NULL || trans == NULL || src->p == NULL || dst->p == NULL) {
        return;
    }

    int32_t A = trans->A;
    int32_t B = trans->B;
    int32_t C = trans->C;
    int32_t D = trans->D;
    s_drawBitmapExCount++;
    if (s_drawBitmapExCount <= 20 || (s_drawBitmapExCount % 120) == 0) {
        BR_HILOGI("MRPDBG DrawBitmapEx #%{public}u w=%{public}u h=%{public}u rop=%{public}u A=%{public}d B=%{public}d C=%{public}d D=%{public}d src(%{public}u,%{public}u,%{public}u,%{public}u) dst(%{public}u,%{public}u,%{public}u,%{public}u)",
                  s_drawBitmapExCount, w, h, trans->rop, A, B, C, D,
                  src->w, src->h, src->x, src->y, dst->w, dst->h, dst->x, dst->y);
    }
    int32_t I = A * D - B * C;
    if (I == 0) {
        return;
    }
    int16_t centerX = (int16_t)(dst->x + w / 2);
    int16_t centerY = (int16_t)(dst->y + h / 2);
    int16_t maxY = (int16_t)((br_abs32(C) * (int32_t)w + br_abs32(D) * (int32_t)h) >> 9);
    int16_t minY = (int16_t)(0 - maxY);
    maxY = (int16_t)br_min32(maxY, (int16_t)(dst->h - centerY));
    minY = (int16_t)br_max32(minY, (int16_t)(0 - centerY));

    for (int32_t dy = minY; dy < maxY; ++dy) {
        int32_t maxXFromD = 999;
        int32_t minXFromD = -999;
        int32_t maxXFromC = 999;
        int32_t minXFromC = -999;
        if (D != 0) {
            int32_t a = ((((int32_t)w * I) >> 9) + B * dy) / D;
            int32_t b = (B * dy - (((int32_t)w * I) >> 9)) / D;
            maxXFromD = br_max32(a, b);
            minXFromD = br_min32(a, b);
        }
        if (C != 0) {
            int32_t a = (A * dy + (((int32_t)h * I) >> 9)) / C;
            int32_t b = (A * dy - (((int32_t)h * I) >> 9)) / C;
            maxXFromC = br_max32(a, b);
            minXFromC = br_min32(a, b);
        }
        int16_t maxX = (int16_t)br_min32(maxXFromD, maxXFromC);
        int16_t minX = (int16_t)br_max32(minXFromD, minXFromC);
        maxX = (int16_t)br_min32(maxX, (int16_t)(dst->w - centerX));
        minX = (int16_t)br_max32(minX, (int16_t)(0 - centerX));
        uint16_t *dstp = dst->p + (dy + centerY) * dst->w + (minX + centerX);

        for (int32_t dx = minX; dx < maxX; ++dx) {
            int32_t offsetY = ((A * dy - C * dx) << 8) / I + (int32_t)h / 2;
            int32_t offsetX = ((D * dx - B * dy) << 8) / I + (int32_t)w / 2;
            if (offsetY >= 0 && offsetY < (int32_t)h && offsetX >= 0 && offsetX < (int32_t)w) {
                uint16_t *srcp = src->p + (offsetY + src->y) * src->w + (offsetX + src->x);
                if (trans->rop == BR_BM_COPY || (*srcp != (uint16_t)transColor)) {
                    *dstp = *srcp;
                }
            }
            ++dstp;
        }
    }
}

static void br_DrawRect(BridgeMap *o, uc_engine *uc) {
    (void)o;
    int32_t x, y, w, h;
    uint32_t r, g, b;
    uc_reg_read(uc, UC_ARM_REG_R0, &x);
    uc_reg_read(uc, UC_ARM_REG_R1, &y);
    uc_reg_read(uc, UC_ARM_REG_R2, &w);
    uc_reg_read(uc, UC_ARM_REG_R3, &h);
    r = getArg(uc, 4);
    g = getArg(uc, 5);
    b = getArg(uc, 6);
    br_screen_ctx_t ctx;
    if (!br_get_screen_ctx(&ctx)) {
        return;
    }
    br_fill_rect_565(&ctx, x, y, w, h, br_rgb565((uint8_t)r, (uint8_t)g, (uint8_t)b));
}

static void br__DrawText(BridgeMap *o, uc_engine *uc) {
    (void)o;
    static uint32_t s_drawTextCount = 0;
    uint32_t pcText;
    int32_t x, y;
    uint32_t r, g, b, is_unicode, font;
    uc_reg_read(uc, UC_ARM_REG_R0, &pcText);
    uc_reg_read(uc, UC_ARM_REG_R1, &x);
    uc_reg_read(uc, UC_ARM_REG_R2, &y);
    uc_reg_read(uc, UC_ARM_REG_R3, &r);
    g = getArg(uc, 4);
    b = getArg(uc, 5);
    is_unicode = getArg(uc, 6);
    font = getArg(uc, 7);
    uint16_t color = br_rgb565((uint8_t)r, (uint8_t)g, (uint8_t)b);
    if (pcText == 0) {
        SET_RET_V(0);
        return;
    }

    br_screen_ctx_t ctx;
    if (!br_get_screen_ctx(&ctx)) {
        SET_RET_V(0);
        return;
    }

    uint16_t *tempBuf = NULL;
    if (is_unicode) {
        tempBuf = (uint16_t *)getMrpMemPtr(pcText);
    } else {
        tempBuf = br_ascii_or_gbk_to_u16((const char *)getMrpMemPtr(pcText), NULL);
    }
    if (tempBuf == NULL) {
        SET_RET_V(0);
        return;
    }

    uint8_t *pCur = (uint8_t *)tempBuf;
    int32_t chx = x;
    int32_t chy = y;
    uint16_t ch = (uint16_t)((pCur[0] << 8) | pCur[1]);
    s_drawTextCount++;
    if (s_drawTextCount <= 20) {
        BR_HILOGI("MRPDBG DrawText #%{public}u x=%{public}d y=%{public}d unicode=%{public}u font=%{public}u firstCh=0x%{public}X color=0x%{public}X",
                  s_drawTextCount, x, y, is_unicode, font, ch, color);
    }
    while (ch != 0) {
        int32_t width = 0, height = 0;
        const uint8_t *bitmap = br_get_char_bitmap_host(ch, (uint16_t)font, &width, &height);
        if (bitmap != NULL && width > 0 && height > 0) {
            int32_t bytesPerRow = (width + 7) >> 3;
            for (int32_t dy = 0; dy < height; ++dy) {
                for (int32_t dx = 0; dx < width; ++dx) {
                    uint8_t v = bitmap[dy * bytesPerRow + (dx >> 3)];
                    if (v & (uint8_t)(0x80 >> (dx & 7))) {
                        br_put_pixel(&ctx, chx + dx, chy + dy, color);
                    }
                }
            }
            chx += width;
        }
        pCur += 2;
        ch = (uint16_t)((pCur[0] << 8) | pCur[1]);
    }

    if (!is_unicode) {
        my_freeExt(tempBuf);
    }

    SET_RET_V(0);
}

static void br__BitmapCheck(BridgeMap *o, uc_engine *uc) {
    (void)o;
    uint32_t p, w, h, transcoler, color_check;
    int32_t x, y;
    uc_reg_read(uc, UC_ARM_REG_R0, &p);
    uc_reg_read(uc, UC_ARM_REG_R1, &x);
    uc_reg_read(uc, UC_ARM_REG_R2, &y);
    uc_reg_read(uc, UC_ARM_REG_R3, &w);
    h = getArg(uc, 4);
    transcoler = getArg(uc, 5);
    color_check = getArg(uc, 6);
    if (p == 0 || w == 0 || h == 0) {
        SET_RET_V(0);
        return;
    }
    br_screen_ctx_t ctx;
    if (!br_get_screen_ctx(&ctx)) {
        SET_RET_V(0);
        return;
    }
    uint16_t *src = (uint16_t *)getMrpMemPtr(p);
    if (src == NULL) {
        SET_RET_V(0);
        return;
    }
    int32_t maxY = br_min32(ctx.h, y + (int32_t)h);
    int32_t maxX = br_min32(ctx.w, x + (int32_t)w);
    int32_t minY = br_max32(0, y);
    int32_t minX = br_max32(0, x);
    int32_t nResult = 0;
    for (int32_t dy = minY; dy < maxY; ++dy) {
        uint16_t *dstp = ctx.buf + dy * ctx.w + minX;
        uint16_t *srcp = src + (dy - y) * (int32_t)w + (minX - x);
        for (int32_t dx = minX; dx < maxX; ++dx) {
            if (*srcp != (uint16_t)transcoler && *dstp != (uint16_t)color_check) {
                nResult++;
            }
            ++dstp;
            ++srcp;
        }
    }
    SET_RET_V(nResult);
}

static void br__mr_readFile(BridgeMap *o, uc_engine *uc) {
    (void)o;
    uint32_t filenamePtr, filelenPtr, lookfor;
    uc_reg_read(uc, UC_ARM_REG_R0, &filenamePtr);
    uc_reg_read(uc, UC_ARM_REG_R1, &filelenPtr);
    uc_reg_read(uc, UC_ARM_REG_R2, &lookfor);
    if (filenamePtr == 0) {
        SET_RET_V((uint32_t)NULL);
        return;
    }
    const char *filename = (const char *)getMrpMemPtr(filenamePtr);
    const char *packFilename = br_get_pack_filename();
    void *buf = NULL;
    int32_t len = 0;
    bool loadedFromPack = false;
    static uint32_t s_readFileCount = 0;
    ++s_readFileCount;

    if (packFilename != NULL && packFilename[0] != '\0') {
        loadedFromPack = br_read_file_from_pack(packFilename, filename, (int)lookfor, &buf, &len);
    }

    if (!loadedFromPack) {
        len = my_getLen(filename);
        if (len < 0) {
            if (s_readFileCount <= 50 || (s_readFileCount % 200) == 0) {
                BR_HILOGI("MRPDBG readFile miss file=%{public}s lookfor=%{public}u pack=%{public}s", filename ? filename : "(null)", lookfor,
                          packFilename ? packFilename : "(null)");
            }
            SET_RET_V((uint32_t)NULL);
            return;
        }
        if (lookfor == 1 || lookfor == 5) {
            if (filelenPtr != 0) {
                uc_mem_write(uc, filelenPtr, &len, 4);
            }
            SET_RET_V(1);
            return;
        }
        int32_t f = my_open(filename, MR_FILE_RDONLY);
        if (f == 0) {
            SET_RET_V((uint32_t)NULL);
            return;
        }
        buf = my_mallocExt((uint32_t)len);
        if (buf == NULL) {
            my_close(f);
            SET_RET_V((uint32_t)NULL);
            return;
        }
        int32_t readBytes = my_read(f, buf, (uint32_t)len);
        my_close(f);
        if (readBytes != len) {
            my_freeExt(buf);
            SET_RET_V((uint32_t)NULL);
            return;
        }
        if (s_readFileCount <= 50 || (s_readFileCount % 200) == 0) {
            BR_HILOGI("MRPDBG readFile fs-hit file=%{public}s len=%{public}d lookfor=%{public}u", filename, len, lookfor);
        }
    } else if (s_readFileCount <= 50 || (s_readFileCount % 200) == 0) {
        BR_HILOGI("MRPDBG readFile pack-hit file=%{public}s len=%{public}d lookfor=%{public}u pack=%{public}s", filename, len, lookfor,
                  packFilename ? packFilename : "(null)");
    }

    if (lookfor == 1 || lookfor == 5) {
        if (buf != NULL) {
            my_freeExt(buf);
        }
        if (filelenPtr != 0) {
            uc_mem_write(uc, filelenPtr, &len, 4);
        }
        SET_RET_V(1);
        return;
    }
    bool forceGzip = (lookfor == 3 || lookfor == 4);
    bool autoGzip = (lookfor == 0);
    bool isGzip = br_is_gzip_buffer(buf, len);
    if (forceGzip && !isGzip) {
        BR_HILOGI("MRPDBG readFile gzip-required-miss file=%{public}s len=%{public}d lookfor=%{public}u", filename, len, lookfor);
        my_freeExt(buf);
        SET_RET_V((uint32_t)NULL);
        return;
    }
    if ((autoGzip || forceGzip) && isGzip) {
        void *unzipped = NULL;
        int32_t unzippedLen = 0;
        if (!br_decompress_gzip_buffer(buf, len, &unzipped, &unzippedLen)) {
            BR_HILOGI("MRPDBG readFile gzip-fail file=%{public}s len=%{public}d", filename, len);
            my_freeExt(buf);
            SET_RET_V((uint32_t)NULL);
            return;
        }
        my_freeExt(buf);
        buf = unzipped;
        len = unzippedLen;
        if (s_readFileCount <= 50 || (s_readFileCount % 200) == 0) {
            BR_HILOGI("MRPDBG readFile gzip-ok file=%{public}s len=%{public}d", filename, len);
        }
    }
    if (filelenPtr != 0) {
        uc_mem_write(uc, filelenPtr, &len, 4);
    }
    SET_RET_V(toMrpMemAddr(buf));
}

static char *br_next_valid_dirent(int32_t h) {
    while (1) {
        char *name = my_readdir(h);
        if (name == NULL) {
            return NULL;
        }
        if ((strcmp(name, ".") == 0) || (strcmp(name, "..") == 0)) {
            continue;
        }
        return name;
    }
}

static uint32_t br_read_le32(const uint8_t *p) {
    return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static const char *br_normalize_entry_name(const char *name) {
    if (name == NULL) {
        return "";
    }
    while (*name == '/' || *name == '\\') {
        ++name;
    }
    while ((name[0] == '.') && (name[1] == '/' || name[1] == '\\')) {
        name += 2;
        while (*name == '/' || *name == '\\') {
            ++name;
        }
    }
    return name;
}

static bool br_append_entry_part(char *out, size_t outSize, const char *part) {
    if (out == NULL || part == NULL || part[0] == '\0') {
        return true;
    }
    size_t len = strlen(out);
    size_t partLen = strlen(part);
    if (len + partLen + (len > 0 ? 1U : 0U) + 1U > outSize) {
        return false;
    }
    if (len > 0) {
        out[len++] = '/';
    }
    memcpy(out + len, part, partLen + 1U);
    return true;
}

static void br_pop_entry_part(char *out) {
    if (out == NULL || out[0] == '\0') {
        return;
    }
    char *slash = strrchr(out, '/');
    if (slash == NULL) {
        out[0] = '\0';
    } else {
        *slash = '\0';
    }
}

static bool br_clean_entry_name(const char *name, char *out, size_t outSize) {
    if (out == NULL || outSize == 0) {
        return false;
    }
    out[0] = '\0';
    const char *cursor = br_normalize_entry_name(name);
    char part[BR_MAX_ENTRY_NAME];
    while (*cursor != '\0') {
        while (*cursor == '/' || *cursor == '\\') {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }
        size_t partLen = 0;
        while (cursor[partLen] != '\0' && cursor[partLen] != '/' && cursor[partLen] != '\\') {
            if (partLen + 1U >= sizeof(part)) {
                return false;
            }
            part[partLen] = cursor[partLen];
            ++partLen;
        }
        part[partLen] = '\0';
        if (strcmp(part, ".") == 0) {
            // Skip.
        } else if (strcmp(part, "..") == 0) {
            br_pop_entry_part(out);
        } else if (!br_append_entry_part(out, outSize, part)) {
            return false;
        }
        cursor += partLen;
    }
    return true;
}

static bool br_entry_name_equals(const char *a, const char *b) {
    char na[BR_MAX_ENTRY_NAME];
    char nb[BR_MAX_ENTRY_NAME];
    if (!br_clean_entry_name(a, na, sizeof(na)) || !br_clean_entry_name(b, nb, sizeof(nb))) {
        return false;
    }
    return strcasecmp(na, nb) == 0;
}

static bool br_is_reasonable_path(const char *s) {
    if (s == NULL || s[0] == '\0') {
        return false;
    }
    for (int i = 0; i < 260; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c == 0) {
            return true;
        }
        if (c < 0x20 || c > 0x7E) {
            return false;
        }
    }
    return false;
}

static const char *br_get_pack_filename(void) {
    if (mr_table == NULL) {
        return NULL;
    }
    uint8_t *tbl = (uint8_t *)mr_table;
    uint32_t packAddr = *(uint32_t *)(tbl + 0x190);  // pack_filename
    if ((packAddr >= START_ADDRESS) && (packAddr < END_ADDRESS)) {
        const char *fromPtr = (const char *)getMrpMemPtr(packAddr);
        if (br_is_reasonable_path(fromPtr)) {
            return fromPtr;
        }
    }
    const char *inlineValue = (const char *)(tbl + 0x190);
    if (br_is_reasonable_path(inlineValue)) {
        return inlineValue;
    }
    return NULL;
}

static bool br_read_file_from_pack(const char *packFilename, const char *entryName, int lookfor, void **outBuf, int32_t *outLen) {
    if (outBuf) {
        *outBuf = NULL;
    }
    if (outLen) {
        *outLen = 0;
    }
    if (packFilename == NULL || entryName == NULL || packFilename[0] == '\0' || entryName[0] == '\0') {
        return false;
    }

    int32_t f = my_open(packFilename, MR_FILE_RDONLY);
    if (f == 0) {
        return false;
    }

    uint8_t head[16];
    int32_t n = my_read(f, head, sizeof(head));
    if (n != 16) {
        my_close(f);
        return false;
    }
    uint32_t magic = br_read_le32(head);
    uint32_t head1 = br_read_le32(head + 4);
    uint32_t head2 = br_read_le32(head + 8);
    uint32_t head3 = br_read_le32(head + 12);
    if (magic != 1196446285U || head1 <= 232U || head3 < 16U) {
        my_close(f);
        return false;
    }

    uint32_t indexLen = head1 + 8U - head3;
    uint8_t *indexBuf = (uint8_t *)my_mallocExt(indexLen);
    if (indexBuf == NULL) {
        my_close(f);
        return false;
    }

    if (my_seek(f, (int32_t)(head3 - 16U), MR_SEEK_CUR) != MR_SUCCESS) {
        my_freeExt(indexBuf);
        my_close(f);
        return false;
    }
    if (my_read(f, indexBuf, indexLen) != (int32_t)indexLen) {
        my_freeExt(indexBuf);
        my_close(f);
        return false;
    }

    uint32_t pos = 0;
    uint32_t filePos = 0;
    uint32_t fileLen = 0;
    bool found = false;
    char tempName[BR_MAX_ENTRY_NAME];
    while (pos + 4U <= indexLen) {
        uint32_t nameLen = br_read_le32(indexBuf + pos);
        pos += 4U;
        if (nameLen < 1U || nameLen >= BR_MAX_ENTRY_NAME || pos + nameLen > indexLen) {
            break;
        }
        memset(tempName, 0, sizeof(tempName));
        memcpy(tempName, indexBuf + pos, nameLen);
        pos += nameLen;
        if (br_entry_name_equals(entryName, tempName)) {
            if (pos + 8U > indexLen) {
                break;
            }
            filePos = br_read_le32(indexBuf + pos);
            fileLen = br_read_le32(indexBuf + pos + 4U);
            if (filePos + fileLen > head2 || fileLen == 0U) {
                break;
            }
            found = true;
            break;
        }
        if (pos + 12U > indexLen) {
            break;
        }
        pos += 12U;
    }

    my_freeExt(indexBuf);
    if (!found) {
        my_close(f);
        return false;
    }
    if (lookfor == 1 || lookfor == 5) {
        my_close(f);
        if (outLen) {
            *outLen = (int32_t)fileLen;
        }
        return true;
    }

    void *buf = my_mallocExt(fileLen);
    if (buf == NULL) {
        my_close(f);
        return false;
    }
    if (my_seek(f, (int32_t)filePos, MR_SEEK_SET) != MR_SUCCESS) {
        my_freeExt(buf);
        my_close(f);
        return false;
    }
    uint32_t readLen = 0;
    while (readLen < fileLen) {
        int32_t chunk = my_read(f, (uint8_t *)buf + readLen, fileLen - readLen);
        if (chunk <= 0) {
            my_freeExt(buf);
            my_close(f);
            return false;
        }
        readLen += (uint32_t)chunk;
    }
    my_close(f);

    if (outBuf) {
        *outBuf = buf;
    }
    if (outLen) {
        *outLen = (int32_t)fileLen;
    }
    return true;
}

static bool br_is_gzip_buffer(const void *buf, int32_t len) {
    if (buf == NULL || len < 18) {
        return false;
    }
    const uint8_t *bytes = (const uint8_t *)buf;
    return bytes[0] == 0x1F && bytes[1] == 0x8B && bytes[2] == 0x08;
}

static bool br_decompress_gzip_buffer(const void *buf, int32_t len, void **outBuf, int32_t *outLen) {
    if (outBuf) {
        *outBuf = NULL;
    }
    if (outLen) {
        *outLen = 0;
    }
    if (!br_is_gzip_buffer(buf, len) || outBuf == NULL || outLen == NULL) {
        return false;
    }

    const uint8_t *src = (const uint8_t *)buf;
    uint32_t expectedLen = br_read_le32(src + len - 4);
    if (expectedLen == 0U || expectedLen > MEMORY_MANAGER_SIZE) {
        return false;
    }

    void *dst = my_mallocExt(expectedLen);
    if (dst == NULL) {
        return false;
    }

    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    stream.next_in = (Bytef *)src;
    stream.avail_in = (uInt)len;
    stream.next_out = (Bytef *)dst;
    stream.avail_out = (uInt)expectedLen;

    int zret = inflateInit2(&stream, 16 + MAX_WBITS);
    if (zret != Z_OK) {
        my_freeExt(dst);
        return false;
    }
    zret = inflate(&stream, Z_FINISH);
    const uLong totalOut = stream.total_out;
    (void)inflateEnd(&stream);
    if (zret != Z_STREAM_END || totalOut == 0U || totalOut > expectedLen) {
        my_freeExt(dst);
        return false;
    }

    *outBuf = dst;
    *outLen = (int32_t)totalOut;
    return true;
}

static void br_mr_wstrlen(BridgeMap *o, uc_engine *uc) {
    (void)o;
    uint32_t txt;
    uc_reg_read(uc, UC_ARM_REG_R0, &txt);
    SET_RET_V(wstrlen((char *)getMrpMemPtr(txt)));
}

static void br_mr_registerAPP(BridgeMap *o, uc_engine *uc) {
    (void)o;
    (void)uc;
    SET_RET_V(MR_SUCCESS);
}

static void br__DrawTextEx(BridgeMap *o, uc_engine *uc) {
    (void)o;
    static uint32_t s_drawTextExCount = 0;
    uint32_t pcText;
    int32_t x, y;
    uint32_t rectWord0, rectWord1, colorWord;
    uint32_t flag, font;
    uc_reg_read(uc, UC_ARM_REG_R0, &pcText);
    uc_reg_read(uc, UC_ARM_REG_R1, &x);
    uc_reg_read(uc, UC_ARM_REG_R2, &y);
    uc_reg_read(uc, UC_ARM_REG_R3, &rectWord0);
    rectWord1 = getArg(uc, 4);
    colorWord = getArg(uc, 5);
    flag = getArg(uc, 6);
    font = getArg(uc, 7);

    br_mr_screenRectSt rect;
    rect.x = (uint16_t)(rectWord0 & 0xFFFF);
    rect.y = (uint16_t)((rectWord0 >> 16) & 0xFFFF);
    rect.w = (uint16_t)(rectWord1 & 0xFFFF);
    rect.h = (uint16_t)((rectWord1 >> 16) & 0xFFFF);
    br_mr_colourSt colorSt;
    colorSt.r = (uint8_t)(colorWord & 0xFF);
    colorSt.g = (uint8_t)((colorWord >> 8) & 0xFF);
    colorSt.b = (uint8_t)((colorWord >> 16) & 0xFF);
    uint16_t color = br_rgb565(colorSt.r, colorSt.g, colorSt.b);

    br_screen_ctx_t ctx;
    if (!br_get_screen_ctx(&ctx)) {
        SET_RET_V(0);
        return;
    }

    if (pcText == 0) {
        SET_RET_V(0);
        return;
    }

    uint16_t *tempBuf = NULL;
    if (flag & BR_DRAW_TEXT_EX_IS_UNICODE) {
        tempBuf = (uint16_t *)getMrpMemPtr(pcText);
    } else {
        tempBuf = br_ascii_or_gbk_to_u16((const char *)getMrpMemPtr(pcText), NULL);
    }
    if (tempBuf == NULL) {
        SET_RET_V(0);
        return;
    }

    int32_t endCharIndex = 0;
    uint8_t *pCur = (uint8_t *)tempBuf;
    uint16_t ch = (uint16_t)((pCur[0] << 8) | pCur[1]);
    s_drawTextExCount++;
    if (s_drawTextExCount <= 12) {
        BR_HILOGI("MRPDBG DrawTextEx #%{public}u x=%{public}d y=%{public}d rect=%{public}ux%{public}u flag=%{public}u font=%{public}u firstCh=0x%{public}X",
                  s_drawTextExCount, x, y, rect.w, rect.h, flag, font, ch);
    }
    int32_t chx = x;
    int32_t chy = y;
    int32_t maxLineH = 0;

    while (ch != 0) {
        int32_t width = 0;
        int32_t height = 0;
        uint16_t drawCh = ((ch == 0x0A) || (ch == 0x0D)) ? 0x20 : ch;
        const uint8_t *bitmap = br_get_char_bitmap_host(drawCh, (uint16_t)font, &width, &height);
        if (bitmap != NULL && width > 0 && height > 0) {
            int32_t curX;
            int32_t curY;
            if (flag & BR_DRAW_TEXT_EX_IS_AUTO_NEWLINE) {
                if (((chx + width) > (x + rect.w)) || (ch == 0x0A)) {
                    if ((chy + maxLineH) < (y + rect.h)) {
                        endCharIndex = (int32_t)(pCur - (uint8_t *)tempBuf);
                    }
                    chx = x;
                    chy = chy + maxLineH + 2;
                    maxLineH = 0;
                    if (chy > (y + rect.h)) {
                        break;
                    }
                }
                curX = chx;
                curY = chy;
                maxLineH = br_max32(maxLineH, height);
            } else {
                if ((chx > (x + rect.w)) || (ch == 0x0A)) {
                    break;
                }
                if ((chx + width) > (x + rect.w)) {
                    endCharIndex = (int32_t)(pCur - (uint8_t *)tempBuf);
                }
                curX = chx;
                curY = chy;
            }

            if ((ch == 0x0A) || (ch == 0x0D)) {
                pCur += 2;
                ch = (uint16_t)((pCur[0] << 8) | pCur[1]);
                continue;
            }

            int32_t bytesPerRow = (width + 7) >> 3;
            for (int32_t dy = 0; dy < height; ++dy) {
                for (int32_t dx = 0; dx < width; ++dx) {
                    uint8_t v = bitmap[dy * bytesPerRow + (dx >> 3)];
                    if ((v & (uint8_t)(0x80 >> (dx & 7))) &&
                        ((curX + dx) < (x + rect.w)) &&
                        ((curY + dy) < (y + rect.h))) {
                        br_put_pixel(&ctx, curX + dx, curY + dy, color);
                    }
                }
            }
            chx += width;
        }
        pCur += 2;
        ch = (uint16_t)((pCur[0] << 8) | pCur[1]);
    }

    if (ch == 0) {
        int32_t total = wstrlen((char *)tempBuf);
        if (flag & BR_DRAW_TEXT_EX_IS_AUTO_NEWLINE) {
            if ((chy + maxLineH) < (y + rect.h)) {
                endCharIndex = total;
            }
        } else if (!(chx > (x + rect.w))) {
            endCharIndex = total;
        }
    }

    if (!(flag & BR_DRAW_TEXT_EX_IS_UNICODE)) {
        my_freeExt(tempBuf);
    }
    SET_RET_V(endCharIndex);
}

static void br__mr_EffSetCon(BridgeMap *o, uc_engine *uc) {
    (void)o;
    (void)uc;
    SET_RET_V(MR_SUCCESS);
}

static void br__mr_TestCom(BridgeMap *o, uc_engine *uc) {
    (void)o;
    (void)uc;
    SET_RET_V(MR_IGNORE);
}

static void br__mr_TestCom1(BridgeMap *o, uc_engine *uc) {
    (void)o;
    (void)uc;
    SET_RET_V(MR_IGNORE);
}

static void br_c2u(BridgeMap *o, uc_engine *uc) {
    (void)o;
    uint32_t cpAddr, errAddr, sizeAddr;
    uc_reg_read(uc, UC_ARM_REG_R0, &cpAddr);
    uc_reg_read(uc, UC_ARM_REG_R1, &errAddr);
    uc_reg_read(uc, UC_ARM_REG_R2, &sizeAddr);
    if (cpAddr == 0) {
        SET_RET_V((uint32_t)NULL);
        return;
    }
    int32_t outBytes = 0;
    uint16_t *u16 = br_ascii_or_gbk_to_u16((const char *)getMrpMemPtr(cpAddr), &outBytes);
    if (u16 == NULL) {
        SET_RET_V((uint32_t)NULL);
        return;
    }
    if (errAddr != 0) {
        int32_t err = 0;
        uc_mem_write(uc, errAddr, &err, 4);
    }
    if (sizeAddr != 0) {
        uc_mem_write(uc, sizeAddr, &outBytes, 4);
    }
    SET_RET_V(toMrpMemAddr(u16));
}

static void br_mr_initNetwork(BridgeMap *o, uc_engine *uc) {
    // int32 (*initNetwork)(NETWORK_CB cb, const char *mode, void *userData);
    LOG("ext call %s()\n", o->name);
    uint32_t cb, mode, userData;
    uc_reg_read(uc, UC_ARM_REG_R0, &cb);
    uc_reg_read(uc, UC_ARM_REG_R1, &mode);
    uc_reg_read(uc, UC_ARM_REG_R2, &userData);
    SET_RET_V(my_initNetwork(uc, (void *)cb, getMrpMemPtr(mode), (void *)userData));
}

static void br_mr_socket(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_socket)(int32 type, int32 protocol);
    int32_t type, protocol;
    uc_reg_read(uc, UC_ARM_REG_R0, &type);
    uc_reg_read(uc, UC_ARM_REG_R1, &protocol);
    int32_t ret = my_socket(type, protocol);
    LOG("ext call %s(): %d \n", o->name, ret);
    SET_RET_V(ret);
}

static void br_mr_connect(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_connect)(int32 s, int32 ip, uint16 port, int32 type);
    LOG("ext call %s()\n", o->name);
    int32_t s, ip, port, type;
    uc_reg_read(uc, UC_ARM_REG_R0, &s);
    uc_reg_read(uc, UC_ARM_REG_R1, &ip);
    uc_reg_read(uc, UC_ARM_REG_R2, &port);
    uc_reg_read(uc, UC_ARM_REG_R3, &type);
    SET_RET_V(my_connect(s, ip, (uint16)port, type));
}

static void br_mr_closeSocket(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_closeSocket)(int32 s);
    LOG("ext call %s()\n", o->name);
    int32_t s;
    uc_reg_read(uc, UC_ARM_REG_R0, &s);
    SET_RET_V(my_closeSocket(s));
}

static void br_mr_closeNetwork(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_closeNetwork)();
    LOG("ext call %s()\n", o->name);
    SET_RET_V(my_closeNetwork());
}

static void br_mr_getHostByName(BridgeMap *o, uc_engine *uc) {
    // int32 (*getHostByName)(const char *ptr, NETWORK_CB cb, void *userData);
    LOG("ext call %s()\n", o->name);
    uint32_t name, cb, userData;
    uc_reg_read(uc, UC_ARM_REG_R0, &name);
    uc_reg_read(uc, UC_ARM_REG_R1, &cb);
    uc_reg_read(uc, UC_ARM_REG_R2, &userData);
    SET_RET_V(my_getHostByName(uc, getMrpMemPtr(name), (void *)cb, (void *)userData));
}

static void br_mr_sendto(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_sendto)(int32 s, const char *buf, int len, int32 ip, uint16 port);
    LOG("ext call %s()\n", o->name);
    uint32_t s, buf, len, ip, port;
    uc_reg_read(uc, UC_ARM_REG_R0, &s);
    uc_reg_read(uc, UC_ARM_REG_R1, &buf);
    uc_reg_read(uc, UC_ARM_REG_R2, &len);
    uc_reg_read(uc, UC_ARM_REG_R3, &ip);
    port = getArg(uc, 4);
    SET_RET_V(my_sendto(s, getMrpMemPtr(buf), len, ip, (uint16_t)port));
}

static void br_mr_send(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_send)(int32 s, const char *buf, int len);
    LOG("ext call %s()\n", o->name);
    int32_t s, buf, len;
    uc_reg_read(uc, UC_ARM_REG_R0, &s);
    uc_reg_read(uc, UC_ARM_REG_R1, &buf);
    uc_reg_read(uc, UC_ARM_REG_R2, &len);
    SET_RET_V(my_send(s, getMrpMemPtr(buf), len));
}

static void br_mr_recvfrom(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_recvfrom)(int32 s, char *buf, int len, int32 *ip, uint16 *port);
    LOG("ext call %s()\n", o->name);
    uint32_t s, buf, len, ip, port;
    uc_reg_read(uc, UC_ARM_REG_R0, &s);
    uc_reg_read(uc, UC_ARM_REG_R1, &buf);
    uc_reg_read(uc, UC_ARM_REG_R2, &len);
    uc_reg_read(uc, UC_ARM_REG_R3, &ip);
    port = getArg(uc, 4);
    SET_RET_V(my_recvfrom(s, getMrpMemPtr(buf), len, getMrpMemPtr(ip), (uint16_t *)getMrpMemPtr(port)));
}

static void br_mr_recv(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_recv)(int32 s, char *buf, int len);
    LOG("ext call %s()\n", o->name);
    int32_t s, buf, len;
    uc_reg_read(uc, UC_ARM_REG_R0, &s);
    uc_reg_read(uc, UC_ARM_REG_R1, &buf);
    uc_reg_read(uc, UC_ARM_REG_R2, &len);
    SET_RET_V(my_recv(s, getMrpMemPtr(buf), len));
}

/*
获取socket connect 状态（主要用于TCP的异步连接）
Syntax
int32 mrc_getSocketState(int32 s);
Parameters
s
   [IN] 打开的socket句柄，由mrc_socket创建

Return Value
   MR_SUCCESS ： 连接成功
   MR_FAILED ： 连接失败
   MR_WAITING ： 连接中
   MR_IGNORE ： 不支持该功能
*/
static void br_mr_getSocketState(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_getSocketState)(int32 s);
    int32_t s;
    uc_reg_read(uc, UC_ARM_REG_R0, &s);
    LOG("ext call %s(%d)\n", o->name, s);
    SET_RET_V(my_getSocketState(s));
}

enum {
    MR_SOUND_MIDI,
    MR_SOUND_WAV,
    MR_SOUND_MP3,
    MR_SOUND_PCM,  // 8K 16bit PCM
    MR_SOUND_M4A,
    MR_SOUND_AMR,
    MR_SOUND_AMR_WB
} MR_SOUND_TYPE;

/*
播放声音数据
type [IN] 声音数据类型，见MR_SOUND_TYPE定义，此函数支持MR_SOUND_MIDI MR_SOUND_WAV MR_SOUND_MP3
data [IN] 声音数据指针
datalen [IN] 声音数据长度
loop [IN] 0:单次播放, 1:循环播放
Return Value MR_SUCCESS 成功 MR_FAILED 失败
*/
#ifdef __EMSCRIPTEN__
EM_JS(int32, js_mr_playSound, (int type, const void *data, uint32 dataLen, int32 loop), {
    return js_playSound(type, data, dataLen, loop);
});
#endif

static void br_mr_playSound(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_playSound)(int type, const void *data, uint32 dataLen, int32 loop);
    int32_t type, data, dataLen, loop;
    uc_reg_read(uc, UC_ARM_REG_R0, &type);
    uc_reg_read(uc, UC_ARM_REG_R1, &data);
    uc_reg_read(uc, UC_ARM_REG_R2, &dataLen);
    uc_reg_read(uc, UC_ARM_REG_R3, &loop);
    LOG("ext call %s(%d, 0x%x, %d, %d)\n", o->name, type, data, dataLen, loop);
#ifdef __EMSCRIPTEN__
    SET_RET_V(js_mr_playSound(type, getMrpMemPtr(data), dataLen, loop));
#else
    SET_RET_V(vmrpHostPlaySound(type, getMrpMemPtr(data), dataLen, loop));
#endif
}

/*
停止播放声音数据
type [IN] 声音数据类型，见MR_SOUND_TYPE定义，此函数支持MR_SOUND_MIDI MR_SOUND_WAV MR_SOUND_MP3
Return Value MR_SUCCESS 成功 MR_FAILED 失败
*/
#ifdef __EMSCRIPTEN__
EM_JS(int32, js_mr_stopSound, (int type), {
    return js_stopSound(type);
});
#endif

static void br_mr_stopSound(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_stopSound)(int type);
    int32_t type;
    uc_reg_read(uc, UC_ARM_REG_R0, &type);
    LOG("ext call %s(%d)\n", o->name, type);

#ifdef __EMSCRIPTEN__
    SET_RET_V(js_mr_stopSound(type));
#else
    SET_RET_V(vmrpHostStopSound(type));
#endif
}

#ifdef __EMSCRIPTEN__
EM_JS(int32, js_mr_startShake, (int32 ms), {
    return js_startShake(ms);
});
#endif

static void br_mr_startShake(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_startShake)(int32 ms);
    int32_t ms;
    uc_reg_read(uc, UC_ARM_REG_R0, &ms);
    LOG("ext call %s()\n", o->name);
#ifdef __EMSCRIPTEN__
    SET_RET_V(js_mr_startShake(ms));
#else
    SET_RET_V(MR_SUCCESS);
#endif
}

#ifdef __EMSCRIPTEN__
EM_JS(int32, js_mr_stopShake, (), {
    return js_stopShake();
});
#endif

static void br_mr_stopShake(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_stopShake)();
    LOG("ext call %s()\n", o->name);
#ifdef __EMSCRIPTEN__
    SET_RET_V(js_mr_stopShake());
#else
    SET_RET_V(MR_SUCCESS);
#endif
}

enum {
    MR_DIALOG_KEY_OK,     // 对话框/文本框等的"确定"键被点击(选择);
    MR_DIALOG_KEY_CANCEL  // 对话框/文本框等的"取消"("返回")键被点击(选择);
};

enum {
    MR_DIALOG_OK,         // 对话框有"确定"键;
    MR_DIALOG_OK_CANCEL,  // 对话框有"确定" "取消"键;
    MR_DIALOG_CANCEL      // 对话框有"返回"键
};

/*
创建一个对话框，并返回对话框句柄。当对话框显示时，如果用户按了对话框上的某个键，系统将构造Mythroad应用消息，通过mrc_event函数传送给Mythroad应用，
消息类型为MR_DIALOG_EVENT，参数为该按键的ID。"确定"键ID为：MR_DIALOG_KEY_OK；"取消"键ID为：MR_DIALOG_KEY_CANCEL。
title [IN]对话框的标题，unicode编码，网络字节序
text [IN]对话框内容，unicode编码，网络字节序
type [IN]对话框类型：MR_DIALOG_OK MR_DIALOG_OK_CANCEL MR_DIALOG_CANCEL

Return Value 正整数 对话框句柄 MR_FAILED 失败
*/
#ifdef __EMSCRIPTEN__
EM_JS(int32, js_mr_dialogCreate, (const char *title, const char *text, int32 type), {
    return js_dialogCreate(title, text, type);
});
#endif

static void br_mr_dialogCreate(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_dialogCreate)(const char *title, const char *text, int32 type);
    uint32_t title, text;
    int32_t type;
    uc_reg_read(uc, UC_ARM_REG_R0, &title);
    uc_reg_read(uc, UC_ARM_REG_R1, &text);
    uc_reg_read(uc, UC_ARM_REG_R2, &type);
    LOG("ext call %s()\n", o->name);
#ifdef __EMSCRIPTEN__
    SET_RET_V(js_mr_dialogCreate(getMrpMemPtr(title), getMrpMemPtr(text), type));
#else
    SET_RET_V(MR_FAILED);
#endif
}

#ifdef __EMSCRIPTEN__
EM_JS(int32, js_mr_dialogRelease, (int32 dialog), {
    return js_dialogRelease(dialog);
});
#endif

static void br_mr_dialogRelease(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_dialogRelease)(int32 dialog);
    int32_t dialog;
    uc_reg_read(uc, UC_ARM_REG_R0, &dialog);
    LOG("ext call %s()\n", o->name);
#ifdef __EMSCRIPTEN__
    SET_RET_V(js_mr_dialogRelease(dialog));
#else
    SET_RET_V(MR_FAILED);
#endif
}

/*
刷新对话框的显示。
dialog [IN]对话框的句柄
title [IN]对话框的标题，unicode编码，网络字节序
text [IN]对话框内容，unicode编码，网络字节序
type [IN]若type为-1，表示type不变,见定义MR_DIALOG_OK MR_DIALOG_OK_CANCEL MR_DIALOG_CANCEL
Return Value MR_SUCCESS 成功 MR_FAILED 失败
*/
#ifdef __EMSCRIPTEN__
EM_JS(int32, js_mr_dialogRefresh, (int32 dialog, const char *title, const char *text, int32 type), {
    return js_dialogRefresh(dialog, title, text, type);
});
#endif

static void br_mr_dialogRefresh(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_dialogRefresh)(int32 dialog, const char *title, const char *text, int32 type);
    LOG("ext call %s()\n", o->name);
    int32_t dialog, type;
    uint32_t title, text;
    uc_reg_read(uc, UC_ARM_REG_R0, &dialog);
    uc_reg_read(uc, UC_ARM_REG_R1, &title);
    uc_reg_read(uc, UC_ARM_REG_R2, &text);
    uc_reg_read(uc, UC_ARM_REG_R3, &type);
#ifdef __EMSCRIPTEN__
    SET_RET_V(js_mr_dialogRefresh(dialog, getMrpMemPtr(title), getMrpMemPtr(text), type));
#else
    SET_RET_V(MR_FAILED);
#endif
}

/*
创建一个文本框，并返回文本框句柄
title [IN]文本框的标题，unicode编码，网络字节序
text [IN]文本框内容，unicode编码，网络字节序
type [IN]文本框按键类型,见定义MR_DIALOG_OK MR_DIALOG_OK_CANCEL MR_DIALOG_CANCEL
Return Value 正整数 文本框句柄 MR_FAILED 失败
Remarks
   文本框用来显示只读的文字信息。文本框和对话框并没有本质的区别，仅仅是显示方式上的不同，在使用上它们的主要区别是：对话框的内容一般较短，文本框的内容一般较长，
   对话框一般实现为弹出式的窗口，文本框一般实现为全屏式的窗口。也可能在手机上对话框和文本框使用了相同的方式实现。文本框和对话框的消息参数是一样的。当文本框显示时，
   如果用户选择了文本框上的某个键，系统将构造Mythroad应用消息，通过mrc_event函数传送给Mythroad 平台，消息类型为MR_DIALOG_EVENT，参数为该按键的ID。
   "确定"键ID为：MR_DIALOG_KEY_OK；"取消"键ID为：MR_DIALOG_KEY_CANCEL。
*/
#ifdef __EMSCRIPTEN__
EM_JS(int32, js_mr_textCreate, (const char *title, const char *text, int32 type), {
    return js_textCreate(title, text, type);
});
#endif

static void br_mr_textCreate(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_textCreate)(const char *title, const char *text, int32 type);
    uint32_t title, text;
    int32_t type;
    uc_reg_read(uc, UC_ARM_REG_R0, &title);
    uc_reg_read(uc, UC_ARM_REG_R1, &text);
    uc_reg_read(uc, UC_ARM_REG_R2, &type);
    LOG("ext call %s()\n", o->name);
#ifdef __EMSCRIPTEN__
    SET_RET_V(js_mr_textCreate(getMrpMemPtr(title), getMrpMemPtr(text), type));
#else
    SET_RET_V(MR_FAILED);
#endif
}

#ifdef __EMSCRIPTEN__
EM_JS(int32, js_mr_textRelease, (int32 handle), {
    return js_textRelease(handle);
});
#endif

static void br_mr_textRelease(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_textRelease)(int32 handle);
    int32_t handle;
    uc_reg_read(uc, UC_ARM_REG_R0, &handle);
    LOG("ext call %s()\n", o->name);
#ifdef __EMSCRIPTEN__
    SET_RET_V(js_mr_textRelease(handle));
#else
    SET_RET_V(MR_FAILED);
#endif
}

#ifdef __EMSCRIPTEN__
EM_JS(int32, js_mr_textRefresh, (int32 handle, const char *title, const char *text), {
    return js_textRefresh(handle, title, text);
});
#endif

static void br_mr_textRefresh(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_textRefresh)(int32 handle, const char *title, const char *text);
    LOG("ext call %s()\n", o->name);
    int32_t handle;
    uint32_t title, text;
    uc_reg_read(uc, UC_ARM_REG_R0, &handle);
    uc_reg_read(uc, UC_ARM_REG_R1, &title);
    uc_reg_read(uc, UC_ARM_REG_R2, &text);
#ifdef __EMSCRIPTEN__
    SET_RET_V(js_mr_textRefresh(handle, getMrpMemPtr(title), getMrpMemPtr(text)));
#else
    SET_RET_V(MR_FAILED);
#endif
}

enum {
    MR_EDIT_ANY,
    MR_EDIT_NUMERIC,
    MR_EDIT_PASSWORD
};

/*
创建一个编辑框，并返回编辑框句柄。
title [IN]文本框的标题，unicode编码，网络字节序
text [IN]文本框内容，unicode编码，网络字节序
type [IN]见MR_EDIT_ANY;MR_EDIT_NUMERIC;MR_EDIT_PASSWORD定义
max_size [IN]最多可以输入的字符（unicode）个数，这里每一个中文、字母、数字、符号都算一个字符
Return Value 正整数 编辑框句柄 MR_FAILED 失败
Remarks
   编辑框用来显示并提供用户编辑文字信息。text是编辑框显示的初始内容。当编辑框显示时，
如果用户选择了编辑框上的某个键，系统将构造Mythroad应用消息，通过mrc_event函数传送给
Mythroad应用，消息类型为MR_DIALOG_EVENT，参数为该按键的ID;"确定"键ID为：MR_DIALOG_KEY_OK；
"取消"键ID为：MR_DIALOG_KEY_CANCEL。
*/
#ifdef __EMSCRIPTEN__
EM_JS(int32, js_mr_editCreate, (const char *title, const char *text, int32 type, int32 max_size), {
    return js_editCreate(title, text, type, max_size);
});
#endif

static uint16_t *g_bridgeEditUnicodeText = NULL;
static char *g_bridgeEditGbkText = NULL;
static bool g_bridgeEditReturnGbk = false;

static void br_free_edit_unicode_text(void) {
    if (g_bridgeEditUnicodeText != NULL) {
        my_freeExt(g_bridgeEditUnicodeText);
        g_bridgeEditUnicodeText = NULL;
    }
    if (g_bridgeEditGbkText != NULL) {
        my_freeExt(g_bridgeEditGbkText);
        g_bridgeEditGbkText = NULL;
    }
}

static void br_mr_editCreate(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_editCreate)(const char *title, const char *text, int32 type, int32 max_size);
    LOG("ext call %s()\n", o->name);
    int32_t type, max_size;
    uint32_t title, text;
    uc_reg_read(uc, UC_ARM_REG_R0, &title);
    uc_reg_read(uc, UC_ARM_REG_R1, &text);
    uc_reg_read(uc, UC_ARM_REG_R2, &type);
    uc_reg_read(uc, UC_ARM_REG_R3, &max_size);
#ifdef __EMSCRIPTEN__
    SET_RET_V(js_mr_editCreate(getMrpMemPtr(title), getMrpMemPtr(text), type, max_size));
#else
    SET_RET_V(editCreate(getMrpMemPtr(title), getMrpMemPtr(text), type, max_size));
#endif
}

static void br_mr_editCreate_unicode(BridgeMap *o, uc_engine *uc) {
    // mr_table edit strings are UCS2BE; host editCreate consumes UTF-8.
    LOG("ext call %s()\n", o->name);
    int32_t type, max_size;
    uint32_t title, text;
    uc_reg_read(uc, UC_ARM_REG_R0, &title);
    uc_reg_read(uc, UC_ARM_REG_R1, &text);
    uc_reg_read(uc, UC_ARM_REG_R2, &type);
    uc_reg_read(uc, UC_ARM_REG_R3, &max_size);

    const uint8_t *rawTitle = title != 0 ? (const uint8_t *)getMrpMemPtr(title) : NULL;
    const uint8_t *rawText = text != 0 ? (const uint8_t *)getMrpMemPtr(text) : NULL;
    bool hasTextSample = (rawTitle != NULL && rawTitle[0] != 0) || (rawText != NULL && rawText[0] != 0);
    bool useUcs2 = (o != NULL && o->extraData != 0) || !hasTextSample ||
                   br_looks_like_ucs2be(rawTitle) || br_looks_like_ucs2be(rawText);
    g_bridgeEditReturnGbk = !useUcs2;

    char *u8Title = useUcs2 ? br_ucs2be_to_utf8(rawTitle) : br_gbk_to_utf8((const char *)rawTitle);
    char *u8Text = useUcs2 ? br_ucs2be_to_utf8(rawText) : br_gbk_to_utf8((const char *)rawText);
    BR_HILOGI("editCreate bridge mode=%{public}s type=%{public}d max=%{public}d",
              useUcs2 ? "ucs2" : "gbk", type, max_size);
#ifdef __EMSCRIPTEN__
    SET_RET_V(js_mr_editCreate(u8Title != NULL ? u8Title : "", u8Text != NULL ? u8Text : "", type, max_size));
#else
    SET_RET_V(editCreate(u8Title != NULL ? u8Title : "", u8Text != NULL ? u8Text : "", type, max_size));
#endif
    if (u8Title != NULL) {
        my_freeExt(u8Title);
    }
    if (u8Text != NULL) {
        my_freeExt(u8Text);
    }
}

#ifdef __EMSCRIPTEN__
EM_JS(int32, js_mr_editRelease, (int32 edit), {
    return js_editRelease(edit);
});
#endif

static void br_mr_editRelease(BridgeMap *o, uc_engine *uc) {
    // int32 (*mr_editRelease)(int32 edit);
    LOG("ext call %s()\n", o->name);
    int32_t edit;
    uc_reg_read(uc, UC_ARM_REG_R0, &edit);
    br_free_edit_unicode_text();
#ifdef __EMSCRIPTEN__
    SET_RET_V(js_mr_editRelease(edit));
#else
    SET_RET_V(editRelease(edit));
#endif
}

/*
获取编辑框内容，unicode编码。调用者若需在编辑框释放后仍然使用编辑框的内容，需要自行保存该内容。该函数需要在编辑框释放之前调用。
Return Value 非NULL 编辑框的内容指针，unicode编码, NULL 失败
*/
#ifdef __EMSCRIPTEN__
EM_JS(const char *, js_mr_editGetText, (int32 edit), {
    return js_editGetText(edit);
});
#endif

static void br_mr_editGetText(BridgeMap *o, uc_engine *uc) {
    // const char *(*mr_editGetText)(int32 edit);
    LOG("ext call %s()\n", o->name);
    int32_t edit;
    uc_reg_read(uc, UC_ARM_REG_R0, &edit);
#ifdef __EMSCRIPTEN__
    char *str = (char *)js_mr_editGetText(edit);
    SET_RET_V(toMrpMemAddr(str));
#else
    char *str = editGetText(edit);
    SET_RET_V(toMrpMemAddr(str));
#endif
}

static void br_mr_editGetText_unicode(BridgeMap *o, uc_engine *uc) {
    // mr_table expects UCS2BE; host editGetText returns UTF-8.
    LOG("ext call %s()\n", o->name);
    int32_t edit;
    uc_reg_read(uc, UC_ARM_REG_R0, &edit);
#ifdef __EMSCRIPTEN__
    char *str = (char *)js_mr_editGetText(edit);
#else
    char *str = editGetText(edit);
#endif
    br_free_edit_unicode_text();
    if (g_bridgeEditReturnGbk) {
        g_bridgeEditGbkText = br_utf8_to_gbk(str);
        if (g_bridgeEditGbkText == NULL) {
            SET_RET_V((uint32_t)NULL);
            return;
        }
        BR_HILOGI("editGetText bridge mode=gbk");
        SET_RET_V(toMrpMemAddr(g_bridgeEditGbkText));
        return;
    }

    g_bridgeEditUnicodeText = br_utf8_to_ucs2be(str, NULL);
    if (g_bridgeEditUnicodeText == NULL) {
        SET_RET_V((uint32_t)NULL);
        return;
    }
    BR_HILOGI("editGetText bridge mode=ucs2");
    SET_RET_V(toMrpMemAddr(g_bridgeEditUnicodeText));
}

// 偏移量由./mrc/[x]_offsets.c直接从mrp中导出
static BridgeMap mr_table_funcMap[] = {
    BRIDGE_FUNC_MAP(0x0, MAP_FUNC, mr_malloc, NULL, br_mr_malloc, 0),
    BRIDGE_FUNC_MAP(0x4, MAP_FUNC, mr_free, NULL, br_mr_free, 0),
    BRIDGE_FUNC_MAP(0x8, MAP_FUNC, mr_realloc, NULL, br_mr_realloc, 0),
    BRIDGE_FUNC_MAP(0xC, MAP_FUNC, memcpy, NULL, br_memcpy, 0),
    BRIDGE_FUNC_MAP(0x10, MAP_FUNC, memmove, NULL, br_memmove, 0),
    BRIDGE_FUNC_MAP(0x14, MAP_FUNC, strcpy, NULL, br_strcpy, 0),
    BRIDGE_FUNC_MAP(0x18, MAP_FUNC, strncpy, NULL, br_strncpy, 0),
    BRIDGE_FUNC_MAP(0x1C, MAP_FUNC, strcat, NULL, br_strcat, 0),
    BRIDGE_FUNC_MAP(0x20, MAP_FUNC, strncat, NULL, br_strncat, 0),
    BRIDGE_FUNC_MAP(0x24, MAP_FUNC, memcmp, NULL, br_memcmp, 0),
    BRIDGE_FUNC_MAP(0x28, MAP_FUNC, strcmp, NULL, br_strcmp, 0),
    BRIDGE_FUNC_MAP(0x2C, MAP_FUNC, strncmp, NULL, br_strncmp, 0),
    BRIDGE_FUNC_MAP(0x30, MAP_FUNC, strcoll, NULL, br_strcoll, 0),
    BRIDGE_FUNC_MAP(0x34, MAP_FUNC, memchr, NULL, br_memchr, 0),
    BRIDGE_FUNC_MAP(0x38, MAP_FUNC, memset, NULL, br_memset, 0),
    BRIDGE_FUNC_MAP(0x3C, MAP_FUNC, strlen, NULL, br_strlen, 0),
    BRIDGE_FUNC_MAP(0x40, MAP_FUNC, strstr, NULL, br_strstr, 0),
    BRIDGE_FUNC_MAP(0x44, MAP_FUNC, sprintf, NULL, br_sprintf, 0),
    BRIDGE_FUNC_MAP(0x48, MAP_FUNC, atoi, NULL, br_atoi, 0),
    BRIDGE_FUNC_MAP(0x4C, MAP_FUNC, strtoul, NULL, br_strtoul, 0),
    BRIDGE_FUNC_MAP(0x50, MAP_FUNC, rand, NULL, br_rand, 0),
    BRIDGE_FUNC_MAP(0x54, MAP_DATA, reserve0, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x58, MAP_DATA, reserve1, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x5C, MAP_DATA, _mr_c_internal_table, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x60, MAP_DATA, _mr_c_port_table, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x64, MAP_FUNC, _mr_c_function_new, NULL, br__mr_c_function_new, 0),
    BRIDGE_FUNC_MAP(0x68, MAP_FUNC, mr_printf, NULL, br_mr_printf, 0),
    BRIDGE_FUNC_MAP(0x6C, MAP_FUNC, mr_mem_get, NULL, br_mem_get, 0),
    BRIDGE_FUNC_MAP(0x70, MAP_FUNC, mr_mem_free, NULL, br_mem_free, 0),
    BRIDGE_FUNC_MAP(0x74, MAP_FUNC, mr_drawBitmap, NULL, br_mr_drawBitmap, 0),
    BRIDGE_FUNC_MAP(0x78, MAP_FUNC, mr_getCharBitmap, NULL, br_mr_getCharBitmap, 0),
    BRIDGE_FUNC_MAP(0x7C, MAP_FUNC, mr_timerStart, NULL, br_mr_timerStart_alias, 0),
    BRIDGE_FUNC_MAP(0x80, MAP_FUNC, mr_timerStop, NULL, br_mr_timerStop_alias, 0),
    BRIDGE_FUNC_MAP(0x84, MAP_FUNC, mr_getTime, NULL, br_mr_getTime_alias, 0),
    BRIDGE_FUNC_MAP(0x88, MAP_FUNC, mr_getDatetime, NULL, br_mr_getDatetime_alias, 0),
    BRIDGE_FUNC_MAP(0x8C, MAP_FUNC, mr_getUserInfo, NULL, br_mr_getUserInfo, 0),
    BRIDGE_FUNC_MAP(0x90, MAP_FUNC, mr_sleep, NULL, br_mr_sleep_alias, 0),
    BRIDGE_FUNC_MAP(0x94, MAP_FUNC, mr_plat, NULL, br_mr_plat, 0),
    BRIDGE_FUNC_MAP(0x98, MAP_FUNC, mr_platEx, NULL, br_mr_platEx, 0),
    BRIDGE_FUNC_MAP(0x9C, MAP_FUNC, mr_ferrno, NULL, br_mr_ferrno, 0),
    BRIDGE_FUNC_MAP(0xA0, MAP_FUNC, mr_open, NULL, br_mr_open, 0),
    BRIDGE_FUNC_MAP(0xA4, MAP_FUNC, mr_close, NULL, br_mr_close, 0),
    BRIDGE_FUNC_MAP(0xA8, MAP_FUNC, mr_info, NULL, br_info, 0),
    BRIDGE_FUNC_MAP(0xAC, MAP_FUNC, mr_write, NULL, br_mr_write, 0),
    BRIDGE_FUNC_MAP(0xB0, MAP_FUNC, mr_read, NULL, br_mr_read, 0),
    BRIDGE_FUNC_MAP(0xB4, MAP_FUNC, mr_seek, NULL, br_mr_seek, 0),
    BRIDGE_FUNC_MAP(0xB8, MAP_FUNC, mr_getLen, NULL, br_mr_getLen, 0),
    BRIDGE_FUNC_MAP(0xBC, MAP_FUNC, mr_remove, NULL, br_mr_remove, 0),
    BRIDGE_FUNC_MAP(0xC0, MAP_FUNC, mr_rename, NULL, br_mr_rename, 0),
    BRIDGE_FUNC_MAP(0xC4, MAP_FUNC, mr_mkDir, NULL, br_mr_mkDir, 0),
    BRIDGE_FUNC_MAP(0xC8, MAP_FUNC, mr_rmDir, NULL, br_mr_rmDir, 0),
    BRIDGE_FUNC_MAP(0xCC, MAP_FUNC, mr_findStart, NULL, br_mr_findStart, 0),
    BRIDGE_FUNC_MAP(0xD0, MAP_FUNC, mr_findGetNext, NULL, br_mr_findGetNext, 0),
    BRIDGE_FUNC_MAP(0xD4, MAP_FUNC, mr_findStop, NULL, br_mr_findStop, 0),
    BRIDGE_FUNC_MAP(0xD8, MAP_FUNC, mr_exit, NULL, br_exit, 0),
    BRIDGE_FUNC_MAP(0xDC, MAP_FUNC, mr_startShake, NULL, br_mr_startShake, 0),
    BRIDGE_FUNC_MAP(0xE0, MAP_FUNC, mr_stopShake, NULL, br_mr_stopShake, 0),
    BRIDGE_FUNC_MAP(0xE4, MAP_FUNC, mr_playSound, NULL, br_mr_playSound, 0),
    BRIDGE_FUNC_MAP(0xE8, MAP_FUNC, mr_stopSound, NULL, br_mr_stopSound, 0),
    BRIDGE_FUNC_MAP(0xEC, MAP_FUNC, mr_sendSms, NULL, br_mr_sendSms, 0),
    BRIDGE_FUNC_MAP(0xF0, MAP_FUNC, mr_call, NULL, br_mr_call, 0),
    BRIDGE_FUNC_MAP(0xF4, MAP_FUNC, mr_getNetworkID, NULL, br_mr_getNetworkID, 0),
    BRIDGE_FUNC_MAP(0xF8, MAP_FUNC, mr_connectWAP, NULL, br_mr_connectWAP, 0),
    BRIDGE_FUNC_MAP(0xFC, MAP_FUNC, mr_menuCreate, NULL, br_mr_menuCreate, 0),
    BRIDGE_FUNC_MAP(0x100, MAP_FUNC, mr_menuSetItem, NULL, br_mr_menuSetItem, 0),
    BRIDGE_FUNC_MAP(0x104, MAP_FUNC, mr_menuShow, NULL, br_mr_menuShow, 0),
    BRIDGE_FUNC_MAP(0x108, MAP_DATA, reserve, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x10C, MAP_FUNC, mr_menuRelease, NULL, br_mr_menuRelease, 0),
    BRIDGE_FUNC_MAP(0x110, MAP_FUNC, mr_menuRefresh, NULL, br_mr_menuRefresh, 0),
    BRIDGE_FUNC_MAP(0x114, MAP_FUNC, mr_dialogCreate, NULL, br_mr_dialogCreate, 0),
    BRIDGE_FUNC_MAP(0x118, MAP_FUNC, mr_dialogRelease, NULL, br_mr_dialogRelease, 0),
    BRIDGE_FUNC_MAP(0x11C, MAP_FUNC, mr_dialogRefresh, NULL, br_mr_dialogRefresh, 0),
    BRIDGE_FUNC_MAP(0x120, MAP_FUNC, mr_textCreate, NULL, br_mr_textCreate, 0),
    BRIDGE_FUNC_MAP(0x124, MAP_FUNC, mr_textRelease, NULL, br_mr_textRelease, 0),
    BRIDGE_FUNC_MAP(0x128, MAP_FUNC, mr_textRefresh, NULL, br_mr_textRefresh, 0),
    BRIDGE_FUNC_MAP(0x12C, MAP_FUNC, mr_editCreate, NULL, br_mr_editCreate_unicode, 0),
    BRIDGE_FUNC_MAP(0x130, MAP_FUNC, mr_editRelease, NULL, br_mr_editRelease, 0),
    BRIDGE_FUNC_MAP(0x134, MAP_FUNC, mr_editGetText, NULL, br_mr_editGetText_unicode, 0),
    BRIDGE_FUNC_MAP(0x138, MAP_FUNC, mr_winCreate, NULL, br_mr_winCreate, 0),
    BRIDGE_FUNC_MAP(0x13C, MAP_FUNC, mr_winRelease, NULL, br_mr_winRelease, 0),
    BRIDGE_FUNC_MAP(0x140, MAP_FUNC, mr_getScreenInfo, NULL, br_mr_getScreenInfo, 0),
    BRIDGE_FUNC_MAP(0x144, MAP_FUNC, mr_initNetwork, NULL, br_mr_initNetwork, 0),
    BRIDGE_FUNC_MAP(0x148, MAP_FUNC, mr_closeNetwork, NULL, br_mr_closeNetwork, 0),
    BRIDGE_FUNC_MAP(0x14C, MAP_FUNC, mr_getHostByName, NULL, br_mr_getHostByName, 0),
    BRIDGE_FUNC_MAP(0x150, MAP_FUNC, mr_socket, NULL, br_mr_socket, 0),
    BRIDGE_FUNC_MAP(0x154, MAP_FUNC, mr_connect, NULL, br_mr_connect, 0),
    BRIDGE_FUNC_MAP(0x158, MAP_FUNC, mr_closeSocket, NULL, br_mr_closeSocket, 0),
    BRIDGE_FUNC_MAP(0x15C, MAP_FUNC, mr_recv, NULL, br_mr_recv, 0),
    BRIDGE_FUNC_MAP(0x160, MAP_FUNC, mr_recvfrom, NULL, br_mr_recvfrom, 0),
    BRIDGE_FUNC_MAP(0x164, MAP_FUNC, mr_send, NULL, br_mr_send, 0),
    BRIDGE_FUNC_MAP(0x168, MAP_FUNC, mr_sendto, NULL, br_mr_sendto, 0),
    BRIDGE_FUNC_MAP(0x16C, MAP_DATA, mr_screenBuf, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x170, MAP_DATA, mr_screen_w, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x174, MAP_DATA, mr_screen_h, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x178, MAP_DATA, mr_screen_bit, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x17C, MAP_DATA, mr_bitmap, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x180, MAP_DATA, mr_tile, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x184, MAP_DATA, mr_map, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x188, MAP_DATA, mr_sound, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x18C, MAP_DATA, mr_sprite, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x190, MAP_DATA, pack_filename, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x194, MAP_DATA, start_filename, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x198, MAP_DATA, old_pack_filename, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x19C, MAP_DATA, old_start_filename, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x1A0, MAP_DATA, mr_ram_file, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x1A4, MAP_DATA, mr_ram_file_len, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x1A8, MAP_DATA, mr_soundOn, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x1AC, MAP_DATA, mr_shakeOn, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x1B0, MAP_DATA, LG_mem_base, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x1B4, MAP_DATA, LG_mem_len, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x1B8, MAP_DATA, LG_mem_end, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x1BC, MAP_DATA, LG_mem_left, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x1C0, MAP_DATA, mr_sms_cfg_buf, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x1C4, MAP_FUNC, mr_md5_init, NULL, br_mr_md5_init, 0),
    BRIDGE_FUNC_MAP(0x1C8, MAP_FUNC, mr_md5_append, NULL, br_mr_md5_append, 0),
    BRIDGE_FUNC_MAP(0x1CC, MAP_FUNC, mr_md5_finish, NULL, br_mr_md5_finish, 0),
    BRIDGE_FUNC_MAP(0x1D0, MAP_FUNC, _mr_load_sms_cfg, NULL, br__mr_load_sms_cfg, 0),
    BRIDGE_FUNC_MAP(0x1D4, MAP_FUNC, _mr_save_sms_cfg, NULL, br__mr_save_sms_cfg, 0),
    BRIDGE_FUNC_MAP(0x1D8, MAP_FUNC, _DispUpEx, NULL, br__DispUpEx, 0),
    BRIDGE_FUNC_MAP(0x1DC, MAP_FUNC, _DrawPoint, NULL, br__DrawPoint, 0),
    BRIDGE_FUNC_MAP(0x1E0, MAP_FUNC, _DrawBitmap, NULL, br__DrawBitmap, 0),
    BRIDGE_FUNC_MAP(0x1E4, MAP_FUNC, _DrawBitmapEx, NULL, br__DrawBitmapEx, 0),
    BRIDGE_FUNC_MAP(0x1E8, MAP_FUNC, DrawRect, NULL, br_DrawRect, 0),
    BRIDGE_FUNC_MAP(0x1EC, MAP_FUNC, _DrawText, NULL, br__DrawText, 0),
    BRIDGE_FUNC_MAP(0x1F0, MAP_FUNC, _BitmapCheck, NULL, br__BitmapCheck, 0),
    BRIDGE_FUNC_MAP(0x1F4, MAP_FUNC, _mr_readFile, NULL, br__mr_readFile, 0),
    BRIDGE_FUNC_MAP(0x1F8, MAP_FUNC, mr_wstrlen, NULL, br_mr_wstrlen, 0),
    BRIDGE_FUNC_MAP(0x1FC, MAP_FUNC, mr_registerAPP, NULL, br_mr_registerAPP, 0),
    BRIDGE_FUNC_MAP(0x200, MAP_FUNC, _DrawTextEx, NULL, br__DrawTextEx, 0),
    BRIDGE_FUNC_MAP(0x204, MAP_FUNC, _mr_EffSetCon, NULL, br__mr_EffSetCon, 0),
    BRIDGE_FUNC_MAP(0x208, MAP_FUNC, _mr_TestCom, NULL, br__mr_TestCom, 0),
    BRIDGE_FUNC_MAP(0x20C, MAP_FUNC, _mr_TestCom1, NULL, br__mr_TestCom1, 0),
    BRIDGE_FUNC_MAP(0x210, MAP_FUNC, c2u, NULL, br_c2u, 0),
    BRIDGE_FUNC_MAP(0x214, MAP_FUNC, _mr_div, NULL, br_mr_div, 0),
    BRIDGE_FUNC_MAP(0x218, MAP_FUNC, _mr_mod, NULL, br_mr_mod, 0),
    BRIDGE_FUNC_MAP(0x21C, MAP_DATA, LG_mem_min, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x220, MAP_DATA, LG_mem_top, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x224, MAP_DATA, mr_updcrc, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x228, MAP_DATA, start_fileparameter, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x22C, MAP_DATA, mr_sms_return_flag, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x230, MAP_DATA, mr_sms_return_val, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x234, MAP_DATA, mr_unzip, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x238, MAP_DATA, mr_exit_cb, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x23C, MAP_DATA, mr_exit_cb_data, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x240, MAP_DATA, mr_entry, NULL, NULL, 0),
    BRIDGE_FUNC_MAP(0x244, MAP_FUNC, mr_platDrawChar, NULL, br_mr_platDrawChar, 0),
};

static BridgeMap dsm_require_funcs_funcMap[] = {
    BRIDGE_FUNC_MAP(0x0, MAP_FUNC, test, NULL, br_test, 0),
    BRIDGE_FUNC_MAP(0x4, MAP_FUNC, log, NULL, br_log, 0),
    BRIDGE_FUNC_MAP(0x8, MAP_FUNC, exit, NULL, br_exit, 0),
    BRIDGE_FUNC_MAP(0xc, MAP_FUNC, srand, NULL, br_srand, 0),
    BRIDGE_FUNC_MAP(0x10, MAP_FUNC, rand, NULL, br_rand, 0),
    BRIDGE_FUNC_MAP(0x14, MAP_FUNC, mem_get, NULL, br_mem_get, 0),
    BRIDGE_FUNC_MAP(0x18, MAP_FUNC, mem_free, NULL, br_mem_free, 0),
    BRIDGE_FUNC_MAP(0x1c, MAP_FUNC, timerStart, NULL, br_timerStart, 0),
    BRIDGE_FUNC_MAP(0x20, MAP_FUNC, timerStop, NULL, br_timerStop, 0),
    BRIDGE_FUNC_MAP(0x24, MAP_FUNC, get_uptime_ms, br_get_uptime_ms_init, br_get_uptime_ms, 0),
    BRIDGE_FUNC_MAP(0x28, MAP_FUNC, getDatetime, NULL, br_getDatetime, 0),
    BRIDGE_FUNC_MAP(0x2c, MAP_FUNC, sleep, NULL, br_sleep, 0),
    BRIDGE_FUNC_MAP(0x30, MAP_FUNC, open, NULL, br_mr_open, 0),
    BRIDGE_FUNC_MAP(0x34, MAP_FUNC, close, NULL, br_mr_close, 0),
    BRIDGE_FUNC_MAP(0x38, MAP_FUNC, read, NULL, br_mr_read, 0),
    BRIDGE_FUNC_MAP(0x3c, MAP_FUNC, write, NULL, br_mr_write, 0),
    BRIDGE_FUNC_MAP(0x40, MAP_FUNC, seek, NULL, br_mr_seek, 0),
    BRIDGE_FUNC_MAP(0x44, MAP_FUNC, info, NULL, br_info, 0),
    BRIDGE_FUNC_MAP(0x48, MAP_FUNC, remove, NULL, br_mr_remove, 0),
    BRIDGE_FUNC_MAP(0x4c, MAP_FUNC, rename, NULL, br_mr_rename, 0),
    BRIDGE_FUNC_MAP(0x50, MAP_FUNC, mkDir, NULL, br_mr_mkDir, 0),
    BRIDGE_FUNC_MAP(0x54, MAP_FUNC, rmDir, NULL, br_mr_rmDir, 0),
    BRIDGE_FUNC_MAP(0x58, MAP_FUNC, opendir, NULL, br_opendir, 0),
    BRIDGE_FUNC_MAP(0x5c, MAP_FUNC, readdir, br_readdir_init, br_readdir, 0),
    BRIDGE_FUNC_MAP(0x60, MAP_FUNC, closedir, NULL, br_closedir, 0),
    BRIDGE_FUNC_MAP(0x64, MAP_FUNC, getLen, NULL, br_mr_getLen, 0),
    BRIDGE_FUNC_MAP(0x68, MAP_FUNC, drawBitmap, NULL, br_mr_drawBitmap, 0),

    BRIDGE_FUNC_MAP(0x6c, MAP_FUNC, getHostByName, NULL, br_mr_getHostByName, 0),
    BRIDGE_FUNC_MAP(0x70, MAP_FUNC, initNetwork, NULL, br_mr_initNetwork, 0),
    BRIDGE_FUNC_MAP(0x74, MAP_FUNC, mr_closeNetwork, NULL, br_mr_closeNetwork, 0),
    BRIDGE_FUNC_MAP(0x78, MAP_FUNC, mr_socket, NULL, br_mr_socket, 0),
    BRIDGE_FUNC_MAP(0x7c, MAP_FUNC, mr_connect, NULL, br_mr_connect, 0),
    BRIDGE_FUNC_MAP(0x80, MAP_FUNC, mr_getSocketState, NULL, br_mr_getSocketState, 0),
    BRIDGE_FUNC_MAP(0x84, MAP_FUNC, mr_closeSocket, NULL, br_mr_closeSocket, 0),
    BRIDGE_FUNC_MAP(0x88, MAP_FUNC, mr_recv, NULL, br_mr_recv, 0),
    BRIDGE_FUNC_MAP(0x8c, MAP_FUNC, mr_send, NULL, br_mr_send, 0),
    BRIDGE_FUNC_MAP(0x90, MAP_FUNC, mr_recvfrom, NULL, br_mr_recvfrom, 0),
    BRIDGE_FUNC_MAP(0x94, MAP_FUNC, mr_sendto, NULL, br_mr_sendto, 0),

    BRIDGE_FUNC_MAP(0x98, MAP_FUNC, mr_startShake, NULL, br_mr_startShake, 0),
    BRIDGE_FUNC_MAP(0x9c, MAP_FUNC, mr_stopShake, NULL, br_mr_stopShake, 0),
    BRIDGE_FUNC_MAP(0xa0, MAP_FUNC, mr_playSound, NULL, br_mr_playSound, 0),
    BRIDGE_FUNC_MAP(0xa4, MAP_FUNC, mr_stopSound, NULL, br_mr_stopSound, 0),
    BRIDGE_FUNC_MAP(0xa8, MAP_FUNC, mr_dialogCreate, NULL, br_mr_dialogCreate, 0),
    BRIDGE_FUNC_MAP(0xac, MAP_FUNC, mr_dialogRelease, NULL, br_mr_dialogRelease, 0),
    BRIDGE_FUNC_MAP(0xb0, MAP_FUNC, mr_dialogRefresh, NULL, br_mr_dialogRefresh, 0),
    BRIDGE_FUNC_MAP(0xb4, MAP_FUNC, mr_textCreate, NULL, br_mr_textCreate, 0),
    BRIDGE_FUNC_MAP(0xb8, MAP_FUNC, mr_textRelease, NULL, br_mr_textRelease, 0),
    BRIDGE_FUNC_MAP(0xbc, MAP_FUNC, mr_textRefresh, NULL, br_mr_textRefresh, 0),
    BRIDGE_FUNC_MAP(0xc0, MAP_FUNC, mr_editCreate, NULL, br_mr_editCreate_unicode, 1),
    BRIDGE_FUNC_MAP(0xc4, MAP_FUNC, mr_editRelease, NULL, br_mr_editRelease, 0),
    BRIDGE_FUNC_MAP(0xc8, MAP_FUNC, mr_editGetText, NULL, br_mr_editGetText_unicode, 1),
    // BRIDGE_FUNC_MAP(0x98, MAP_FUNC, drawBitmap, NULL, NULL, 0),
};
//////////////////////////////////////////////////////////////////////////////////////////

static void hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
    uIntMap *mobj = uIntMap_search(&root, address);
    if (mobj) {
        BridgeMap *obj = mobj->data;
        if (obj->type == MAP_FUNC) {
            if (obj->fn == NULL) {
                printf("!!! %s() Not yet implemented function !!! \n", obj->name);
                bridgeSetLastErr(UC_ERR_EXCEPTION, "bridge hit not implemented function");
                uc_emu_stop(uc);
                return;
            }
            obj->fn(obj, uc);

            uint32_t _lr;
            uc_reg_read(uc, UC_ARM_REG_LR, &_lr);
            uc_reg_write(uc, UC_ARM_REG_PC, &_lr);
            return;
        }
        printf("!!! unregister function at 0x%" PRIX64 " !!! \n", address);
    }
}

static void *hooks_init(uc_engine *uc, BridgeMap *map, uint32_t mapCount, uint32_t size) {
    uc_err err;
    uc_hook trace;
    BridgeMap *obj;
    uIntMap *mobj;
    uint32_t addr;
    void *ptr = my_mallocExt(size);
    if (ptr == NULL) {
        bridgeSetLastErr(UC_ERR_NOMEM, "bridge hooks_init alloc failed");
        return NULL;
    }
    memset(ptr, 0, size);
    uint32_t startAddress = toMrpMemAddr(ptr);

    err = uc_hook_add(uc, &trace, UC_HOOK_CODE, hook_code, NULL, startAddress, startAddress + size, 0);
    if (err != UC_ERR_OK) {
        printf("add hook err %u (%s)\n", err, uc_strerror(err));
        bridgeSetLastErr(err, "bridge add hook failed");
        goto end;
    }

    for (int i = 0; i < mapCount; i++) {
        obj = &map[i];
        addr = startAddress + obj->pos;
        if (obj->initFn != NULL) {
            obj->initFn(obj, uc, addr);
        } else {
            if (obj->type == MAP_FUNC) {
                // 默认的函数初始化，初始化为地址值，当PC寄存器执行到该地址时拦截下来进入我们的回调函数
                uc_mem_write(uc, addr, &addr, 4);
            }
        }
        mobj = malloc(sizeof(uIntMap));
        if (mobj == NULL) {
            bridgeSetLastErr(UC_ERR_NOMEM, "uIntMap malloc failed");
            goto end;
        }
        mobj->key = addr;
        mobj->data = obj;
        if (uIntMap_insert(&root, mobj)) {
            printf("uIntMap_insert() failed %d exists.\n", addr);
            bridgeSetLastErr(UC_ERR_EXCEPTION, "uIntMap_insert failed");
            free(mobj);
            goto end;
        }
    }
    return ptr;
end:
    my_freeExt(ptr);
    return NULL;
}

static uc_err runCode(uc_engine *uc, uint32_t startAddr, uint32_t stopAddr, bool isThumb) {
    g_bridgeLastErr = UC_ERR_OK;
    printf("runCode start=0x%08X stop=0x%08X thumb=%d\n", startAddr, stopAddr, isThumb ? 1 : 0);
    uc_reg_write(uc, UC_ARM_REG_LR, &stopAddr);  // 当程序执行到这里时停止运行(return)

    // Note we start at ADDRESS | 1 to indicate THUMB mode.
    startAddr = isThumb ? (startAddr | 1) : startAddr;
    uc_err err = uc_emu_start(uc, startAddr, stopAddr, 0, 0);  // 似乎unicorn 1.0.2之前并不会在pc==stopAddr时立即停止
    if (err) {
        printf("Failed on uc_emu_start() with error returned: %u (%s)\n", err, uc_strerror(err));
        dumpREG(uc);
        bridgeSetLastErr(err, "bridge runCode failed");
        return err;
    }
    if (g_bridgeLastErr != UC_ERR_OK) {
        dumpREG(uc);
        return g_bridgeLastErr;
    }
    dumpREG(uc);
    return UC_ERR_OK;
}

uc_err bridge_init(uc_engine *uc) {
    g_bridgeLastErr = UC_ERR_OK;
    bridgeClearHookMap();
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        perror("mutex init fail");
        bridgeSetLastErr(UC_ERR_EXCEPTION, "bridge mutex init failed");
        return UC_ERR_EXCEPTION;
    }
    uint32_t len = 4 * countof(mr_table_funcMap);  // 因为都是指针，所以直接可以算出来总内存大小
    mr_table = hooks_init(uc, mr_table_funcMap, countof(mr_table_funcMap), len);
    if (mr_table == NULL) {
        return g_bridgeLastErr != UC_ERR_OK ? g_bridgeLastErr : UC_ERR_NOMEM;
    }

    dsm_require_funcs = hooks_init(uc, dsm_require_funcs_funcMap, countof(dsm_require_funcs_funcMap), sizeof(DSM_REQUIRE_FUNCS));
    if (dsm_require_funcs == NULL) {
        return g_bridgeLastErr != UC_ERR_OK ? g_bridgeLastErr : UC_ERR_NOMEM;
    }
#ifdef __EMSCRIPTEN__
    ((DSM_REQUIRE_FUNCS *)dsm_require_funcs)->flags = FLAG_USE_UTF8_FS;  // wasm文件系统是UTF8编码
#else
    ((DSM_REQUIRE_FUNCS *)dsm_require_funcs)->flags = 0;
#endif

    mr_c_event = my_mallocExt(sizeof(event_t));
    dsm_event = my_mallocExt(sizeof(event_t));
    mr_start_dsm_param = my_mallocExt(sizeof(start_t));
    return UC_ERR_OK;
}

uc_err bridge_ext_init(uc_engine *uc) {
    uint32_t v = toMrpMemAddr(mr_table);
    uc_err err = uc_mem_write(uc, CODE_ADDRESS, &v, 4);  // 设置mr_table
    if (err != UC_ERR_OK) {
        bridgeSetLastErr(err, "bridge set mr_table failed");
        return err;
    }

    // Upstream notes only document fixR9 coverage for mr_c_function_load(0).
    // On OHOS the code=1 ext/plugin path currently crashes inside Unicorn TCG
    // during bridge_ext_init, so prefer the base helper init path first.
    v = 0;
    err = uc_reg_write(uc, UC_ARM_REG_R0, &v);
    if (err != UC_ERR_OK) {
        bridgeSetLastErr(err, "bridge set ext init arg failed");
        return err;
    }
    printf("bridge_ext_init mr_table=0x%08X code=0 helper_addr=0x%08X\n", toMrpMemAddr(mr_table), mr_extHelper_addr);
    dumpREG(uc);

    // 执行ext内的mr_c_function_load()
    err = runCode(uc, CODE_ADDRESS + 8, CODE_ADDRESS, false);
    if (err != UC_ERR_OK) {
        return err;
    }

    // mr_c_function.start_of_ER_RW 会被写入r9(SB)，指向的内存是用来存放全局变量的
    printf("bridge_ext_init ok helper_addr=0x%08X er_rw=%p er_rw_len=%u ext_type=%d ext_chunk=%p stack=%d\n",
           mr_extHelper_addr,
           mr_c_function_P->start_of_ER_RW,
           (unsigned)mr_c_function_P->ER_RW_Length,
           (int)mr_c_function_P->ext_type,
           mr_c_function_P->mrc_extChunk,
           (int)mr_c_function_P->stack);
    return UC_ERR_OK;
}

static int32_t bridge_mr_extHelper(uc_engine *uc, uint32_t code, uint32_t input, uint32_t input_len) {
    // int32 (*mr_extHelper)(void* P, int32 code, uint8* input, int32 input_len);
    uint32_t v = toMrpMemAddr(mr_c_function_P);
    uc_reg_write(uc, UC_ARM_REG_R0, &v);          // p
    uc_reg_write(uc, UC_ARM_REG_R1, &code);       // code
    uc_reg_write(uc, UC_ARM_REG_R2, &input);      // input
    uc_reg_write(uc, UC_ARM_REG_R3, &input_len);  // input_len

    if (runCode(uc, mr_extHelper_addr, CODE_ADDRESS, false) != UC_ERR_OK) {
        return MR_FAILED;
    }
    uc_reg_read(uc, UC_ARM_REG_R0, &v);
    return v;
}

static inline int32_t bridge_mr_event(uc_engine *uc, int32_t code, int32_t param0, int32_t param1) {
    mr_c_event->code = code;
    mr_c_event->p0 = param0;
    mr_c_event->p1 = param1;
    return bridge_mr_extHelper(uc, 1, toMrpMemAddr(mr_c_event), sizeof(event_t));
}

// 执行网络通信的回调
int32_t bridge_dsm_network_cb(uc_engine *uc, uint32_t addr, int32_t p0, uint32_t p1) {
    if (!bridgeMutexLockOrFail()) {
        return MR_FAILED;
    }
    uint32_t ret, r9;
    uc_reg_read(uc, UC_ARM_REG_R9, &r9);

    // 因为回调不是从mr_extHelper调用，因此需要手动设置r9
    uc_reg_write(uc, UC_ARM_REG_R9, &mr_c_function_P->start_of_ER_RW);
    // 实际上这个r9值被设置成mythroad层的，因为mythroad层的lua部分也会调用
    // 因此由mythroad层去区分是mythroad层的回调函数还是mrp层的回调函数，这就是userData存在的意义

    uc_reg_write(uc, UC_ARM_REG_R0, &p0);
    uc_reg_write(uc, UC_ARM_REG_R1, &p1);
    if (runCode(uc, addr, CODE_ADDRESS, false) != UC_ERR_OK) {
        bridgeMutexUnlockOrFail();
        return MR_FAILED;
    }

    uc_reg_write(uc, UC_ARM_REG_R9, &r9);  // 恢复r9
    uc_reg_read(uc, UC_ARM_REG_R0, &ret);
    if (!bridgeMutexUnlockOrFail()) {
        return MR_FAILED;
    }
    return ret;
}

int32_t bridge_dsm_mr_start_dsm(uc_engine *uc, char *filename, char *ext, char *entry) {
    if (!bridgeMutexLockOrFail()) {
        return MR_FAILED;
    }

    mr_start_dsm_param->filename = (char *)copyStrToMrp(filename);
    mr_start_dsm_param->ext = (char *)copyStrToMrp(ext);
    mr_start_dsm_param->entry = entry ? (char *)copyStrToMrp(entry) : NULL;

    int32_t v = bridge_mr_event(uc, MR_START_DSM, toMrpMemAddr(mr_start_dsm_param), 0);

    my_freeExt(getMrpMemPtr((uint32_t)mr_start_dsm_param->filename));
    mr_start_dsm_param->filename = NULL;

    my_freeExt(getMrpMemPtr((uint32_t)mr_start_dsm_param->ext));
    mr_start_dsm_param->ext = NULL;

    if (entry) {
        my_freeExt(getMrpMemPtr((uint32_t)mr_start_dsm_param->entry));
    }

    if (!bridgeMutexUnlockOrFail()) {
        return MR_FAILED;
    }
    return v;
}

int32_t bridge_dsm_mr_pauseApp(uc_engine *uc) {
    if (!bridgeMutexLockOrFail()) {
        return MR_FAILED;
    }
    int32_t v = bridge_mr_event(uc, MR_PAUSEAPP, 0, 0);
    if (!bridgeMutexUnlockOrFail()) {
        return MR_FAILED;
    }
    return v;
}

int32_t bridge_dsm_mr_resumeApp(uc_engine *uc) {
    if (!bridgeMutexLockOrFail()) {
        return MR_FAILED;
    }
    int32_t v = bridge_mr_event(uc, MR_RESUMEAPP, 0, 0);
    if (!bridgeMutexUnlockOrFail()) {
        return MR_FAILED;
    }
    return v;
}

int32_t bridge_dsm_mr_timer(uc_engine *uc) {
    if (!bridgeMutexLockOrFail()) {
        return MR_FAILED;
    }
    int32_t v = bridge_mr_event(uc, MR_TIMER, 0, 0);
    if (!bridgeMutexUnlockOrFail()) {
        return MR_FAILED;
    }
    return v;
}

int32_t bridge_dsm_mr_event(uc_engine *uc, int32_t code, int32_t p0, int32_t p1) {
    if (!bridgeMutexLockOrFail()) {
        return MR_FAILED;
    }
    dsm_event->code = code;
    dsm_event->p0 = p0;
    dsm_event->p1 = p1;
    int32_t v = bridge_mr_event(uc, MR_EVENT, toMrpMemAddr(dsm_event), 0);
    if (!bridgeMutexUnlockOrFail()) {
        return MR_FAILED;
    }
    return v;
}

int32_t bridge_dsm_init(uc_engine *uc) {
    if (!bridgeMutexLockOrFail()) {
        return MR_FAILED;
    }
    int32_t v = bridge_mr_event(uc, DSM_INIT, toMrpMemAddr(dsm_require_funcs), 0);

    if (!bridgeMutexUnlockOrFail()) {
        return MR_FAILED;
    }
    if (v == VMRP_VER) {
        return MR_SUCCESS;
    } else {
        printf("err: dsm_version got %d expect %d\n", v, VMRP_VER);
    }
    return MR_FAILED;
}
