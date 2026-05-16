#include "./header/fileLib.h"
#include "./header/utils.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#define MYTHROAD_PATH_MAX 512
#define MYTHROAD_NAME_MAX 256

// mrc_open需要返回0表示失败
static struct rb_root filef_map = RB_ROOT;
static uint32_t filef_count = 0;

// mrc_findStart需要返回-1表示失败
static struct rb_root dirf_map = RB_ROOT;
static uint32_t dirf_count = 0;

typedef struct FileHandle {
    int fd;
    bool writable;
} FileHandle;

typedef struct DirHandle {
    char **names;
    size_t count;
    size_t index;
} DirHandle;

static bool append_part(char *out, size_t outSize, const char *part) {
    if (out == NULL || part == NULL || part[0] == '\0') {
        return true;
    }
    size_t len = strlen(out);
    size_t partLen = strlen(part);
    size_t need = len + partLen + ((len > 0 && strcmp(out, ".") != 0) ? 1U : 0U) + 1U;
    if (need > outSize) {
        return false;
    }
    if (strcmp(out, ".") == 0) {
        out[0] = '\0';
        len = 0;
    }
    if (len > 0) {
        out[len++] = '/';
        out[len] = '\0';
    }
    memcpy(out + len, part, partLen + 1U);
    return true;
}

static void pop_part(char *path) {
    if (path == NULL || path[0] == '\0' || strcmp(path, ".") == 0) {
        return;
    }
    char *slash = strrchr(path, '/');
    if (slash == NULL) {
        path[0] = '\0';
    } else {
        *slash = '\0';
    }
}

static bool sanitize_path(const char *input, char *out, size_t outSize) {
    if (out == NULL || outSize == 0) {
        return false;
    }
    out[0] = '\0';
    if (input == NULL) {
        return false;
    }

    char temp[MYTHROAD_PATH_MAX];
    size_t n = 0;
    const unsigned char *src = (const unsigned char *)input;
    if (((src[0] >= 'A' && src[0] <= 'Z') || (src[0] >= 'a' && src[0] <= 'z')) && src[1] == ':') {
        char drive = (char)(src[0] >= 'A' && src[0] <= 'Z' ? src[0] - 'A' + 'a' : src[0]);
        if (drive == 'a' || drive == 'b' || drive == 'x') {
            n = (size_t)snprintf(temp, sizeof(temp), "mythroad/disk/%c/", drive);
        } else {
            temp[0] = '\0';
        }
        src += 2;
        while (*src == '/' || *src == '\\') {
            ++src;
        }
    } else {
        temp[0] = '\0';
    }

    for (; *src != '\0' && n + 1U < sizeof(temp); ++src) {
        char c = (char)*src;
        temp[n++] = (c == '\\') ? '/' : c;
    }
    temp[n] = '\0';

    const char *cursor = temp;
    while (*cursor == '/') {
        ++cursor;
    }
    char part[MYTHROAD_NAME_MAX];
    while (true) {
        while (*cursor == '/') {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }
        size_t partLen = 0;
        while (cursor[partLen] != '\0' && cursor[partLen] != '/') {
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
            pop_part(out);
        } else if (!append_part(out, outSize, part)) {
            return false;
        }
        cursor += partLen;
    }

    if (out[0] == '\0') {
        snprintf(out, outSize, ".");
    }
    return true;
}

static bool path_exists(const char *path, struct stat *outStat) {
    struct stat st;
    if (stat((path == NULL || path[0] == '\0') ? "." : path, &st) != 0) {
        return false;
    }
    if (outStat != NULL) {
        *outStat = st;
    }
    return true;
}

static bool resolve_child_case(const char *parent, const char *name, char *out, size_t outSize) {
    char exact[MYTHROAD_PATH_MAX];
    exact[0] = '\0';
    if (parent != NULL && parent[0] != '\0' && strcmp(parent, ".") != 0) {
        snprintf(exact, sizeof(exact), "%s", parent);
    }
    if (!append_part(exact, sizeof(exact), name)) {
        return false;
    }
    if (path_exists(exact, NULL)) {
        snprintf(out, outSize, "%s", exact);
        return true;
    }

    const char *dirName = (parent == NULL || parent[0] == '\0') ? "." : parent;
    DIR *dir = opendir(dirName);
    if (dir == NULL) {
        return false;
    }
    struct dirent *entry;
    bool found = false;
    while ((entry = readdir(dir)) != NULL) {
        if (strcasecmp(entry->d_name, name) == 0) {
            char candidate[MYTHROAD_PATH_MAX];
            candidate[0] = '\0';
            if (parent != NULL && parent[0] != '\0' && strcmp(parent, ".") != 0) {
                snprintf(candidate, sizeof(candidate), "%s", parent);
            }
            if (append_part(candidate, sizeof(candidate), entry->d_name)) {
                snprintf(out, outSize, "%s", candidate);
                found = true;
            }
            break;
        }
    }
    closedir(dir);
    return found;
}

static bool resolve_existing_path(const char *input, char *out, size_t outSize) {
    char clean[MYTHROAD_PATH_MAX];
    if (!sanitize_path(input, clean, sizeof(clean))) {
        return false;
    }
    if (strcmp(clean, ".") == 0) {
        snprintf(out, outSize, ".");
        return path_exists(out, NULL);
    }

    char current[MYTHROAD_PATH_MAX] = "";
    const char *cursor = clean;
    char part[MYTHROAD_NAME_MAX];
    while (*cursor != '\0') {
        size_t partLen = 0;
        while (cursor[partLen] != '\0' && cursor[partLen] != '/') {
            if (partLen + 1U >= sizeof(part)) {
                return false;
            }
            part[partLen] = cursor[partLen];
            ++partLen;
        }
        part[partLen] = '\0';
        char next[MYTHROAD_PATH_MAX];
        if (!resolve_child_case(current[0] ? current : ".", part, next, sizeof(next))) {
            return false;
        }
        snprintf(current, sizeof(current), "%s", next);
        cursor += partLen;
        if (*cursor == '/') {
            ++cursor;
        }
    }
    snprintf(out, outSize, "%s", current[0] ? current : ".");
    return true;
}

static bool split_parent_name(const char *path, char *parent, size_t parentSize, char *name, size_t nameSize) {
    char clean[MYTHROAD_PATH_MAX];
    if (!sanitize_path(path, clean, sizeof(clean)) || strcmp(clean, ".") == 0) {
        return false;
    }
    char *slash = strrchr(clean, '/');
    if (slash == NULL) {
        snprintf(parent, parentSize, ".");
        snprintf(name, nameSize, "%s", clean);
    } else {
        *slash = '\0';
        snprintf(parent, parentSize, "%s", clean[0] ? clean : ".");
        snprintf(name, nameSize, "%s", slash + 1);
    }
    return name[0] != '\0';
}

static int mkdir_one(const char *path) {
    if (mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) == 0 || errno == EEXIST) {
        return 0;
    }
    return -1;
}

static bool resolve_parent_for_create(const char *parent, char *out, size_t outSize) {
    char clean[MYTHROAD_PATH_MAX];
    if (!sanitize_path(parent, clean, sizeof(clean))) {
        return false;
    }
    if (strcmp(clean, ".") == 0) {
        snprintf(out, outSize, ".");
        return true;
    }

    char current[MYTHROAD_PATH_MAX] = "";
    const char *cursor = clean;
    char part[MYTHROAD_NAME_MAX];
    while (*cursor != '\0') {
        size_t partLen = 0;
        while (cursor[partLen] != '\0' && cursor[partLen] != '/') {
            if (partLen + 1U >= sizeof(part)) {
                return false;
            }
            part[partLen] = cursor[partLen];
            ++partLen;
        }
        part[partLen] = '\0';

        char next[MYTHROAD_PATH_MAX];
        if (resolve_child_case(current[0] ? current : ".", part, next, sizeof(next))) {
            struct stat st;
            if (!path_exists(next, &st) || !S_ISDIR(st.st_mode)) {
                return false;
            }
        } else {
            next[0] = '\0';
            if (current[0] != '\0' && strcmp(current, ".") != 0) {
                snprintf(next, sizeof(next), "%s", current);
            }
            if (!append_part(next, sizeof(next), part) || mkdir_one(next) != 0) {
                return false;
            }
        }
        snprintf(current, sizeof(current), "%s", next);
        cursor += partLen;
        if (*cursor == '/') {
            ++cursor;
        }
    }

    snprintf(out, outSize, "%s", current[0] ? current : ".");
    return true;
}

static bool resolve_create_path(const char *input, char *out, size_t outSize) {
    char parent[MYTHROAD_PATH_MAX];
    char name[MYTHROAD_NAME_MAX];
    if (!split_parent_name(input, parent, sizeof(parent), name, sizeof(name))) {
        return false;
    }
    char resolvedParent[MYTHROAD_PATH_MAX];
    if (!resolve_parent_for_create(parent, resolvedParent, sizeof(resolvedParent))) {
        return false;
    }
    char existing[MYTHROAD_PATH_MAX];
    if (resolve_child_case(resolvedParent, name, existing, sizeof(existing))) {
        snprintf(out, outSize, "%s", existing);
        return true;
    }
    out[0] = '\0';
    if (resolvedParent[0] != '\0' && strcmp(resolvedParent, ".") != 0) {
        snprintf(out, outSize, "%s", resolvedParent);
    }
    return append_part(out, outSize, name);
}

static int compare_names_ci(const void *a, const void *b) {
    const char *sa = *(const char *const *)a;
    const char *sb = *(const char *const *)b;
    int ci = strcasecmp(sa, sb);
    if (ci != 0) {
        return ci;
    }
    return strcmp(sa, sb);
}

static void free_dir_handle(DirHandle *handle) {
    if (handle == NULL) {
        return;
    }
    for (size_t i = 0; i < handle->count; ++i) {
        free(handle->names[i]);
    }
    free(handle->names);
    free(handle);
}

int32_t my_open(const char *filename, uint32_t mode) {
    int f;
    int new_mode = 0;
    bool writable = false;

    if (filename == NULL || filename[0] == '\0') {
        return 0;
    }

    char resolved[MYTHROAD_PATH_MAX];
    bool recreate = (mode & MR_FILE_RECREATE) && (mode & (MR_FILE_WRONLY | MR_FILE_RDWR | MR_FILE_CREATE));

    if (mode & MR_FILE_RDWR) {
        new_mode = O_RDWR;
        writable = true;
    } else if (mode & MR_FILE_WRONLY) {
        new_mode = O_WRONLY;
        writable = true;
    } else {
        new_mode = O_RDONLY;
    }
    if ((mode & MR_FILE_CREATE || recreate) && !writable) {
        new_mode = O_RDWR;
        writable = true;
    }
    if (mode & MR_FILE_CREATE) new_mode |= O_CREAT;
    if (recreate) new_mode |= O_CREAT | O_TRUNC;

#ifdef _WIN32
    new_mode |= O_RAW;
#endif

    if (writable || (new_mode & O_CREAT)) {
        if (!resolve_create_path(filename, resolved, sizeof(resolved))) {
            return 0;
        }
    } else {
        if (!resolve_existing_path(filename, resolved, sizeof(resolved))) {
            return 0;
        }
    }

    f = open(resolved, new_mode, S_IRWXU | S_IRWXG | S_IRWXO);
    if (f == -1) {
        return 0;
    }

    filef_count++;
    FileHandle *handle = malloc(sizeof(FileHandle));
    if (handle == NULL) {
        close(f);
        return 0;
    }
    handle->fd = f;
    handle->writable = writable;

    uIntMap *obj = malloc(sizeof(uIntMap));
    if (obj == NULL) {
        close(f);
        free(handle);
        return 0;
    }
    obj->key = filef_count;
    obj->data = handle;
    uIntMap_insert(&filef_map, obj);
    return filef_count;
}

int32_t my_close(int32_t f) {
    uIntMap *obj = uIntMap_delete(&filef_map, f);
    if (obj == NULL) {
        return MR_FAILED;
    }
    if (f == filef_count) {
        filef_count--;
    }
    FileHandle *handle = (FileHandle *)obj->data;
    int fh = handle->fd;
    bool writable = handle->writable;
    if (writable) {
        fsync(fh);
    }
    free(handle);
    free(obj);
    if (close(fh) != 0) {
        return MR_FAILED;
    }
    return MR_SUCCESS;
}

int32_t my_seek(int32_t f, int32_t pos, int method) {
    uIntMap *obj = uIntMap_search(&filef_map, f);
    if (obj == NULL) {
        return MR_FAILED;
    }
    FileHandle *handle = (FileHandle *)obj->data;
    off_t ret = lseek(handle->fd, (off_t)pos, method);
    if (ret == -1) {
        return MR_FAILED;
    }
    return MR_SUCCESS;
}

int32_t my_read(int32_t f, void *p, uint32_t l) {
    uIntMap *obj = uIntMap_search(&filef_map, f);
    if (obj == NULL) {
        return MR_FAILED;
    }
    FileHandle *handle = (FileHandle *)obj->data;
    if (p == NULL) {
        return MR_FAILED;
    }
    uint32_t total = 0;
    while (total < l) {
        ssize_t n = read(handle->fd, (uint8_t *)p + total, (size_t)(l - total));
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return total > 0 ? (int32_t)total : MR_FAILED;
        }
        if (n == 0) {
            break;
        }
        total += (uint32_t)n;
    }
    return (int32_t)total;
}

int32_t my_write(int32_t f, void *p, uint32_t l) {
    uIntMap *obj = uIntMap_search(&filef_map, f);
    if (obj == NULL) {
        return MR_FAILED;
    }
    FileHandle *handle = (FileHandle *)obj->data;
    if (p == NULL) {
        return MR_FAILED;
    }
    uint32_t total = 0;
    while (total < l) {
        ssize_t n = write(handle->fd, (uint8_t *)p + total, (size_t)(l - total));
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return total > 0 ? (int32_t)total : MR_FAILED;
        }
        if (n == 0) {
            break;
        }
        total += (uint32_t)n;
    }
    return (int32_t)total;
}

int32_t my_rename(const char *oldname, const char *newname) {
    if (oldname == NULL || newname == NULL) {
        return MR_FAILED;
    }
    char oldPath[MYTHROAD_PATH_MAX];
    char newPath[MYTHROAD_PATH_MAX];
    if (!resolve_existing_path(oldname, oldPath, sizeof(oldPath)) || !resolve_create_path(newname, newPath, sizeof(newPath))) {
        return MR_FAILED;
    }
    if (strcmp(oldPath, newPath) != 0 && path_exists(newPath, NULL)) {
        struct stat st;
        if (!path_exists(newPath, &st) || !S_ISREG(st.st_mode) || remove(newPath) != 0) {
            return MR_FAILED;
        }
    }
    int ret = rename(oldPath, newPath);
    if (ret != 0) {
        return MR_FAILED;
    }
    return MR_SUCCESS;
}

int32_t my_remove(const char *filename) {
    if (filename == NULL) {
        return MR_FAILED;
    }
    char resolved[MYTHROAD_PATH_MAX];
    if (!resolve_existing_path(filename, resolved, sizeof(resolved))) {
        return MR_FAILED;
    }
    int ret = remove(resolved);
    if (ret != 0) {
        return MR_FAILED;
    }
    return MR_SUCCESS;
}

int32_t my_getLen(const char *filename) {
    if (filename == NULL) {
        return -1;
    }
    char resolved[MYTHROAD_PATH_MAX];
    if (!resolve_existing_path(filename, resolved, sizeof(resolved))) {
        return -1;
    }
    struct stat s1;
    int ret = stat(resolved, &s1);
    if (ret != 0)
        return -1;
    return s1.st_size;
}

int32_t my_mkDir(const char *name) {
    if (name == NULL) {
        return MR_FAILED;
    }
    char resolved[MYTHROAD_PATH_MAX];
    if (!resolve_create_path(name, resolved, sizeof(resolved))) {
        return MR_FAILED;
    }
    struct stat st;
    if (path_exists(resolved, &st)) {
        return S_ISDIR(st.st_mode) ? MR_SUCCESS : MR_FAILED;
    }
    if (mkdir_one(resolved) != 0) {
        return MR_FAILED;
    }
    return MR_SUCCESS;
}

int32_t my_rmDir(const char *name) {
    if (name == NULL) {
        return MR_FAILED;
    }
    char resolved[MYTHROAD_PATH_MAX];
    if (!resolve_existing_path(name, resolved, sizeof(resolved))) {
        return MR_FAILED;
    }
    int ret = rmdir(resolved);
    if (ret != 0) {
        return MR_FAILED;
    }
    return MR_SUCCESS;
}

int32_t my_info(const char *filename) {
    if (filename == NULL) {
        return MR_IS_INVALID;
    }
    char resolved[MYTHROAD_PATH_MAX];
    if (!resolve_existing_path(filename, resolved, sizeof(resolved))) {
        return MR_IS_INVALID;
    }
    struct stat s1;
    int ret = stat(resolved, &s1);

    if (ret != 0) {
        return MR_IS_INVALID;
    }
    if (S_ISDIR(s1.st_mode)) {
        return MR_IS_DIR;
    } else if (S_ISREG(s1.st_mode)) {
        return MR_IS_FILE;
    }
    return MR_IS_INVALID;
}

int32_t my_opendir(const char *name) {
    if (name == NULL) {
        return MR_FAILED;
    }
    char resolved[MYTHROAD_PATH_MAX];
    if (!resolve_existing_path(name[0] == '\0' ? "." : name, resolved, sizeof(resolved))) {
        return MR_FAILED;
    }
    DIR *pDir = opendir(resolved);
    if (pDir == NULL) {
        return MR_FAILED;
    }

    DirHandle *handle = calloc(1, sizeof(DirHandle));
    if (handle == NULL) {
        closedir(pDir);
        return MR_FAILED;
    }
    struct dirent *entry;
    while ((entry = readdir(pDir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char **newNames = realloc(handle->names, sizeof(char *) * (handle->count + 1U));
        if (newNames == NULL) {
            closedir(pDir);
            free_dir_handle(handle);
            return MR_FAILED;
        }
        handle->names = newNames;
        handle->names[handle->count] = strdup(entry->d_name);
        if (handle->names[handle->count] == NULL) {
            closedir(pDir);
            free_dir_handle(handle);
            return MR_FAILED;
        }
        handle->count++;
    }
    closedir(pDir);
    if (handle->count > 1U) {
        qsort(handle->names, handle->count, sizeof(char *), compare_names_ci);
    }

    dirf_count++;
    uIntMap *obj = malloc(sizeof(uIntMap));
    if (obj == NULL) {
        free_dir_handle(handle);
        return MR_FAILED;
    }
    obj->key = dirf_count;
    obj->data = (void *)handle;
    uIntMap_insert(&dirf_map, obj);
    return dirf_count;
}

char *my_readdir(int32_t f) {
    uIntMap *obj = uIntMap_search(&dirf_map, f);
    if (obj == NULL) {
        return NULL;
    }
    DirHandle *handle = (DirHandle *)obj->data;
    if (handle != NULL && handle->index < handle->count) {
        return handle->names[handle->index++];
    }
    return NULL;
}

int32_t my_closedir(int32_t f) {
    uIntMap *obj = uIntMap_delete(&dirf_map, f);
    if (obj == NULL) {
        return MR_FAILED;
    }
    if (f == dirf_count) {
        dirf_count--;
    }
    DirHandle *handle = (DirHandle *)obj->data;
    free(obj);
    free_dir_handle(handle);
    return MR_SUCCESS;
}

void writeFile(const char *filename, void *data, uint32 length) {
    int fh = my_open(filename, MR_FILE_RECREATE | MR_FILE_RDWR);
    if (fh == 0) {
        return;
    }
    int32_t wLen = 0;
    char *ptr = (char *)data;
    do {
        ptr += wLen;
        wLen = my_write(fh, ptr, length < 1000 ? length : 1000);
        if (wLen <= 0) {
            break;
        }
        length -= wLen;
    } while (length > 0);
    my_close(fh);
}

char *readFile(const char *filename) {
    int32_t len = my_getLen(filename);
    if (len < 0) {
        return NULL;
    }
    char *p = malloc(len);
    if (p == NULL) {
        return NULL;
    }
    int32_t fh = my_open(filename, MR_FILE_RDONLY);
    if (fh) {
        int32_t rLen = 0;
        char *ptr = p;
        do {
            ptr += rLen;
            rLen = my_read(fh, ptr, len);
            if (rLen <= 0) {
                free(p);
                my_close(fh);
                return NULL;
            }
            len -= rLen;
        } while (len > 0);
        my_close(fh);
        return p;
    }
    free(p);
    return NULL;
}
