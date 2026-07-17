#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
//#include <linux/fs.h>
#ifdef MAIN
#include "app_dir_module.h"
typedef int (*file_check_cb_t) (const char *path, int is_dir, size_t dir_count, size_t file_count, ssize_t file_size);
#define _fmalloc            malloc
#define _frealloc           realloc
#define _fstrdup            strdup
#define _ffree              free
#define _FERR(fmt, args...) do { printf("[FERR][%s:%d] ", __func__, __LINE__); printf(fmt, ##args); } while(0)
#define _FINFO(fmt, args...) do { printf("[FINFO][%s:%d] ", __func__, __LINE__); printf(fmt, ##args); } while(0)
#else
#include "app.h"
#include "module/app_dir_module.h"
#define _fmalloc            GxCore_Malloc
#define _frealloc           GxCore_Realloc
#define _fstrdup            GxCore_Strdup
#define _ffree              GxCore_Free
#define _FERR               app_log_error
#define _FINFO              app_log_info
#endif

#define FILE_NAME_MAX       1024
#define _free_ptr(ptr)      do { if (ptr) _ffree(ptr); ptr = NULL; } while(0)

typedef enum {
    FILE_NONE = 0,
    FILE_DIR,
    FILE_REG,
    FILE_UNKNOWN
} ftype_t;

typedef struct {
    int is_del;
    int max_depth;
    int cur_depth;
    char *dir_name;
    size_t dir_len;
    file_check_cb_t check_cb;
} dir_del_t;

typedef struct {
    int is_copy;
    size_t max_depth;
    size_t cur_depth;
    char *src_name;
    size_t src_len;
    char *dst_name;
    size_t dst_len;
    char *buf;
    size_t buf_size;
    file_check_cb_t check_cb;
} dir_copy_t;

static ftype_t _get_ftype(const char *path)
{
    struct stat st;

    if (stat((char *)path, &st) != 0)
        return FILE_NONE;
    if (S_ISDIR(st.st_mode))
        return FILE_DIR;
    else if (S_ISREG(st.st_mode))
        return FILE_REG;
    else
        return FILE_UNKNOWN;
}

#if 0
static ftype_t _get_ftype2(struct dirent *ddir)
{
    /* The implementation of ECOS is not standard, It is wrong to use DT_xxx. */
    if (!ddir)
        return FILE_NONE;
#ifdef ECOS_OS
    if (S_ISDIR(ddir->d_type))
        return FILE_DIR;
    else if (S_ISREG(ddir->d_type))
        return FILE_REG;
    else
        return FILE_UNKNOWN;
#else
    if (ddir->d_type == DT_DIR)
        return FILE_DIR;
    else if (ddir->d_type == DT_REG)
        return FILE_REG;
    else
        return FILE_UNKNOWN;
#endif
}
#endif

static int _delete_or_tell_dir(dir_del_t p, size_t *dir_count, size_t *file_count)
{
    DIR *dir = NULL;
    struct dirent *ddir = NULL;
    size_t fname_len = 0;
    ftype_t type = FILE_NONE;
    dir_del_t np = {0};

    if (p.is_del == 0 && p.cur_depth > p.max_depth) {
        //_FERR("depth %u > %u overflow!\n", (unsigned)p.cur_depth, (unsigned)p.max_depth);
        return 0;
    }

    p.dir_name[p.dir_len] = 0;
    dir = opendir(p.dir_name);
    if (!dir) {
        _FERR("opendir %s failed!\n", p.dir_name);
        return -1;
    }

    while ((ddir = readdir(dir))) {
        if (strcmp (ddir->d_name, ".") == 0 || strcmp (ddir->d_name, "..") == 0)
            continue;
        fname_len = strlen(ddir->d_name);
        if (p.dir_len + fname_len + 1 >= FILE_NAME_MAX) {
            _FERR("fname overflow!\n");
            goto end;
        }
        p.dir_name[p.dir_len] = '/';
        memcpy(p.dir_name + p.dir_len + 1, ddir->d_name, fname_len);
        p.dir_name[p.dir_len + fname_len + 1] = 0;
        type = _get_ftype(p.dir_name);
        if (type == FILE_DIR) {
            np = p;
            np.cur_depth = p.cur_depth + 1;
            np.dir_len = p.dir_len + fname_len + 1;
            if (_delete_or_tell_dir(np, dir_count, file_count) < 0)
                goto end;
        } else {
            if (p.is_del) {
                if (unlink(p.dir_name) < 0) {
                    _FERR("unlink %s failed!\n", p.dir_name);
                    goto end;
                }
            }
            ++(*file_count);
            if (p.check_cb && p.check_cb(p.dir_name, 0, *dir_count, *file_count, 0) < 0) {
                _FERR("force stop!\n");
                goto end;
            }
        }
    }

    closedir(dir);
    p.dir_name[p.dir_len] = 0;
    if (p.is_del) {
        if (rmdir(p.dir_name) < 0) {
            _FERR("rmdir %s failed!\n", p.dir_name);
            return -1;
        }
    }
    ++(*dir_count);
    if (p.check_cb && p.check_cb(p.dir_name, 1, *dir_count, *file_count, 0) < 0) {
        _FERR("force stop!\n");
        goto end;
    }
    return 0;
end:
    closedir(dir);
    return -1;
}

static int _copy_file(const char *src, const char *dst, char *buf, size_t buf_size)
{
    int ret = -1;
    int rfd = -1, wfd = -1;
    ssize_t size = 0;
    ssize_t total = 0;

    if ((rfd = open(src, O_RDONLY)) < 0) {
        _FERR("open %s failed!\n", src);
        return -1;
    }
    if ((wfd = open(dst, O_CREAT | O_TRUNC | O_WRONLY, 0666)) < 0) {
        _FERR("open %s failed!\n", dst);
        close(rfd);
        return -1;
    }

    while ((size = read(rfd, buf, buf_size)) > 0) {
        if (size != write(wfd, buf, size)) {
            _FERR("write %s failed!\n", dst);
            goto end;
        }
        total += size;
    }

    ret = (int)total;
end:
    close(rfd);
    close(wfd);
    return ret;
}

static int _prepare_dir(char *dir_name)
{
    char *tmp = dir_name;
    while ((tmp = strchr(tmp+1, '/'))) {
        *tmp = 0;
        if (access(dir_name, F_OK) != 0) {
            if (mkdir(dir_name, 0777) < 0) {
                _FERR("mkdir %s failed!\n", dir_name);
                *tmp = '/';
                return -1;
            }
        }
        *tmp = '/';
    }

    if (access(dir_name, F_OK) != 0) {
        if (mkdir(dir_name, 0777) < 0) {
            _FERR("mkdir %s failed!\n", dir_name);
            return -1;
        }
    }

    return 0;
}

static int _copy_or_check_dir(dir_copy_t p, size_t *dir_count, size_t *file_count)
{
    DIR *dir = NULL;
    struct dirent *ddir = NULL;
    size_t fname_len = 0;
    int ret = -1;
    ftype_t type = FILE_NONE;
    dir_copy_t np = {0};
    ssize_t fsize = 0;

    if (p.cur_depth > p.max_depth) {
        _FERR("depth %u > %u overflow!\n", (unsigned)p.cur_depth, (unsigned)p.max_depth);
        return 0;
    }

    p.dst_name[p.dst_len] = 0;
    if (access(p.dst_name, F_OK) != 0) {
        if (p.is_copy) {
            if (mkdir(p.dst_name, 0777) < 0) {
                _FERR("mkdir %s failed!\n", p.dst_name);
                return -1;
            }
        } else {
            _FERR("%s not existed!\n", p.dst_name);
            return -1;
        }
    }
    ++(*dir_count);
    if (p.check_cb && p.check_cb(p.dst_name, 1, *dir_count, *file_count, 0) < 0) {
        _FERR("force stop!\n");
        return -1;
    }

    p.src_name[p.src_len] = 0;
    dir = opendir(p.src_name);
    if (!dir) {
        _FERR("opendir %s failed!\n", p.src_name);
        return -1;
    }

    while ((ddir = readdir(dir))) {
        if (strcmp(ddir->d_name, ".") == 0 || strcmp(ddir->d_name, "..") == 0)
            continue;
        fname_len = strlen(ddir->d_name);
        if (p.src_len + fname_len + 1 >= FILE_NAME_MAX
                || p.dst_len + fname_len + 1 >= FILE_NAME_MAX) {
            _FERR("fname length overflow!\n");
            continue;
        }
        p.src_name[p.src_len] = '/';
        memcpy(p.src_name + p.src_len + 1, ddir->d_name, fname_len);
        p.src_name[p.src_len + fname_len + 1] = 0;
        p.dst_name[p.dst_len] = '/';
        memcpy(p.dst_name + p.dst_len + 1, ddir->d_name, fname_len);
        p.dst_name[p.dst_len + fname_len + 1] = 0;
        type = _get_ftype(p.src_name);
        if (type != FILE_DIR && type != FILE_REG) {
            p.src_name[p.src_len] = 0;
            _FERR("%s/%s ftype not supported!\n", p.src_name, ddir->d_name);
            continue;
        }

        if (type == FILE_DIR) {
            np = p;
            np.cur_depth = p.cur_depth + 1;
            np.src_len = p.src_len + fname_len + 1;
            np.dst_len = p.dst_len + fname_len + 1;
            if (_copy_or_check_dir(np, dir_count, file_count) < 0) {
                goto end;
            }
        } else {
            if (p.is_copy) {
                if ((fsize = _copy_file(p.src_name, p.dst_name, p.buf, p.buf_size)) < 0) {
                    _FERR("rm %s failed!\n", p.src_name);
                    goto end;
                }
            } else {
                fsize = 0;
                if (access(p.dst_name, F_OK) != 0) {
                    _FERR("%s not existed!\n", p.dst_name);
                    goto end;
                }
            }
            ++(*file_count);
            if (p.check_cb && p.check_cb(p.dst_name, 0, *dir_count, *file_count, fsize) < 0) {
                _FERR("force stop!\n");
                goto end;
            }
        }
    }

    p.dst_name[p.dst_len] = 0;
    ret = 0;
end:
    closedir(dir);
    return ret;
}

static char *_dir_path_dup(const char *path, size_t *len)
{
    size_t dir_len = 0;
    char *dir_name = NULL;

    dir_len = strlen(path);
    if (dir_len > 0 && path[dir_len-1] == '/')
        dir_len -= 1;
    if (dir_len == 0 || dir_len >= FILE_NAME_MAX) {
        _FERR("invalid path %s!\n", path);
        return NULL;
    }

    dir_name = _fmalloc(FILE_NAME_MAX);
    if (!dir_name) {
        _FERR("malloc failed!\n");
        return NULL;
    }
    memcpy(dir_name, path, dir_len);
    dir_name[dir_len] = 0;
    if (len)
        *len = dir_len;

    return dir_name;
}

static void _dir_path_free(char *dir_name)
{
    if (dir_name)
        _ffree(dir_name);
}

int app_dir_or_file_delete(const char *path, size_t max_depth, file_check_cb_t check_cb)
{
    int ret = -1;
    char *dir_name = NULL;
    size_t dir_len = 0;
    dir_del_t p = {0};
    size_t dir_count = 0, file_count = 0;

    if (!path) {
        _FERR("null path!\n");
        return -1;
    }
    if (max_depth == 0) max_depth = 0xff;

    switch (_get_ftype(path)) {
        case FILE_NONE:
            ret = 0;
            break;
        case FILE_DIR:
            dir_name = _dir_path_dup(path, &dir_len);
            if (!dir_name)
                break;
            p.is_del = 1;
            p.max_depth = max_depth;
            p.cur_depth = 0;
            p.dir_name = dir_name;
            p.dir_len = dir_len;
            p.check_cb = check_cb;
            ret = _delete_or_tell_dir(p, &dir_count, &file_count);
            _dir_path_free(dir_name);
            break;
        case FILE_REG:
            ret = unlink(path);
            if (check_cb) check_cb(path, 0, 0, 1, 0);
            break;
        case FILE_UNKNOWN:
            break;
        default:
            break;
    }

    return ret;
}

int app_dir_or_file_tell(const char *path, size_t max_depth, file_check_cb_t check_cb, size_t *dir_count, size_t *file_count)
{
    int ret = -1;
    char *dir_name = NULL;
    size_t dir_len = 0;
    dir_del_t p = {0};

    if (!path || !dir_count || !file_count) {
        _FERR("null path or null count!\n");
        return -1;
    }
    if (max_depth == 0) max_depth = 0xff;
    *dir_count = 0;
    *file_count = 0;

    switch (_get_ftype(path)) {
        case FILE_NONE:
            break;
        case FILE_DIR:
            dir_name = _dir_path_dup(path, &dir_len);
            if (!dir_name)
                break;
            p.is_del = 0;
            p.max_depth = max_depth;
            p.cur_depth = 0;
            p.dir_name = dir_name;
            p.dir_len = dir_len;
            p.check_cb = check_cb;
            ret = _delete_or_tell_dir(p, dir_count, file_count);
            _dir_path_free(dir_name);
            break;
        case FILE_REG:
            ret = 0;
            *file_count = 1;
            if (check_cb) check_cb(path, 0, 0, 1, 0);
            break;
        case FILE_UNKNOWN:
            break;
        default:
            break;
    }

    return ret;
}

int app_dir_or_file_copy(const char *src, const char *dst, size_t max_depth, size_t buf_size, file_check_cb_t check_cb)
{
    int ret = -1;
    ftype_t src_type, dst_type;
    size_t src_len = 0, dst_len = 0;
    char *src_name = NULL, *dst_name = NULL, *buf = NULL;
    dir_copy_t p = {0};
    size_t dir_count = 0, file_count = 0;
    ssize_t fsize = 0;

    if (!src || !dst) {
        _FERR("null path!\n");
        return -1;
    }
    src_len = strlen(src);
    if (strncmp(src, dst, src_len) == 0 && (dst[src_len] == '/' || dst[src_len] == 0)) {
        _FERR("invalid copy %s to %s!\n", src, dst);
        return -1;
    }
    if (max_depth == 0) max_depth = 0xff;
    if (buf_size < 512) buf_size = 512;

    src_type = _get_ftype(src);
    dst_type = _get_ftype(dst);
    if (src_type == FILE_NONE || src_type == FILE_UNKNOWN) {
        _FERR("%s unknown ftype!\n", src);
        return -1;
    }
    if (dst_type == FILE_UNKNOWN) {
        _FERR("%s unknown ftype!\n", dst);
        return -1;
    }

    buf = _fmalloc(buf_size);
    if (!buf) {
        _FERR("malloc failed!\n");
        return -1;
    }
    if (app_dir_or_file_delete(dst, 0, NULL) < 0) {
        _FERR("delete %s failed!\n", dst);
        goto end;
    }
    if (src_type == FILE_DIR) {
        src_name = _dir_path_dup(src, &src_len);
        dst_name = _dir_path_dup(dst, &dst_len);
        if (!src_name || !dst_name) {
            _FERR("malloc failed!\n");
            goto end;
        }
        if (_prepare_dir(dst_name) < 0) {
            _FERR("malloc failed!\n");
            goto end;
        }

        p.is_copy = 1;
        p.max_depth = max_depth;
        p.cur_depth = 0;
        p.src_name = src_name;
        p.src_len = src_len;
        p.dst_name = dst_name;
        p.dst_len = dst_len;
        p.buf = buf;
        p.buf_size = buf_size;
        p.check_cb = check_cb;
        ret = _copy_or_check_dir(p, &dir_count, &file_count);
    } else {
        if ((fsize = _copy_file(src, dst, buf, buf_size)) > 0) {
            if (check_cb) check_cb(dst, 0, 0, 1, fsize);
            ret = 0;
        }
    }

    if (ret < 0) {
        _FERR("\033[31mcopy %s to %s failed!\033[0m\n", src, dst);
    } else {
        _FINFO("\033[32mcopy %s to %s succeed!\033[0m\n", src, dst);
    }
end:
    _dir_path_free(src_name);
    _dir_path_free(dst_name);
    _free_ptr(buf);
    return ret;
}

int app_dir_or_file_check(const char *src, const char *dst, size_t max_depth, file_check_cb_t check_cb)
{
    int ret = -1;
    ftype_t src_type, dst_type;
    size_t src_len = 0, dst_len = 0;
    char *src_name = NULL, *dst_name = NULL;
    dir_copy_t p = {0};
    size_t dir_count = 0, file_count = 0;

    if (!src || !dst) {
        _FERR("null path!\n");
        return -1;
    }
    src_len = strlen(src);
    if (strncmp(src, dst, src_len) == 0 && (dst[src_len] == '/' || dst[src_len] == 0)) {
        _FERR("invalid check %s to %s!\n", src, dst);
        return -1;
    }
    if (max_depth == 0) max_depth = 0xff;

    src_type = _get_ftype(src);
    dst_type = _get_ftype(dst);

    if (src_type != dst_type) {
        _FERR("ftype not same!\n");
        return -1;
    }
    if (src_type == FILE_NONE || src_type == FILE_UNKNOWN) {
        _FERR("%s unknown ftype!\n", src);
        return -1;
    }
    if (dst_type == FILE_NONE || dst_type == FILE_UNKNOWN) {
        _FERR("%s unknown ftype!\n", dst);
        return -1;
    }

    if (src_type == FILE_DIR) {
        src_name = _dir_path_dup(src, &src_len);
        dst_name = _dir_path_dup(dst, &dst_len);
        if (!src_name || !dst_name) {
            _FERR("malloc failed!\n");
            goto end;
        }
        p.is_copy = 0;
        p.max_depth = max_depth;
        p.cur_depth = 0;
        p.src_name = src_name;
        p.src_len = src_len;
        p.dst_name = dst_name;
        p.dst_len = dst_len;
        p.buf = NULL;
        p.buf_size = 0;
        p.check_cb = check_cb;
        ret = _copy_or_check_dir(p, &dir_count, &file_count);
    } else {
        if (access(dst, F_OK) == 0) {
            ret = 0;
            if (check_cb) check_cb(dst, 0, 0, 1, 0);
        }
    }

    if (ret < 0) {
        _FERR("\033[31mcheck %s between %s failed!\033[0m\n", src, dst);
    } else {
        _FINFO("\033[32mcheck %s between %s succeed!\033[0m\n", src, dst);
    }
end:
    _dir_path_free(src_name);
    _dir_path_free(dst_name);
    return ret;
}

int app_copy_file_to_file(const char *src, const char *dst, size_t buf_size)
{
    char *buf = NULL;
    int ret = 0;

    if (!src || !dst)
        return -1;
    if (buf_size == 0)
        buf_size = 8192;
    if ((buf = _fmalloc(buf_size)) == NULL) {
        _FERR("\033[31mmalloc failed!\033[0m\n");
        return -1;
    }
    ret = _copy_file(src, dst, buf, buf_size);
    _free_ptr(buf);

    return ret;
}

int fcache_open(fcache_t *fcache, const char *file_name, size_t buffer_size)
{
    if((fcache == NULL)||(file_name == NULL))
        return -1;
    memset(fcache, 0, sizeof(fcache_t));
    if ((fcache->fp = fopen(file_name, "r")) == NULL)
        return -1;
    if ((fcache->ptr = _fmalloc(buffer_size)) == NULL)
    {
        fclose(fcache->fp);
        fcache->fp = NULL;
        return -1;
    }
    fcache->buffer_size = buffer_size;
    fseek(fcache->fp, 0, SEEK_END);
    fcache->file_size = ftell(fcache->fp);
    fseek(fcache->fp, 0, SEEK_SET);
    return 0;
}

void fcache_close(fcache_t *fcache)
{
    if(fcache == NULL)
        return;
    if (fcache->fp)
        fclose(fcache->fp);
    if (fcache->ptr)
        _ffree(fcache->ptr);
    memset(fcache, 0, sizeof(fcache_t));
}

size_t fcache_read(fcache_t *fcache, size_t read_size, char **sstr)
{
    size_t size = 0;

    if ((fcache->file_offset >= fcache->start_offset)
            && (fcache->file_offset + read_size <= fcache->start_offset + fcache->read_size))
    {
        *sstr = fcache->ptr + fcache->file_offset - fcache->start_offset;
        size = fcache->start_offset + fcache->read_size - fcache->file_offset;
    } else {
        if (read_size > fcache->buffer_size-1) {
            fcache->buffer_size = 0;
            fcache->start_offset = 0;
            fcache->read_size = 0;
            if ((fcache->ptr = _frealloc(fcache->ptr, read_size+1)) == NULL)
                return 0;
            fcache->buffer_size = read_size+1;
        }

        fseek(fcache->fp, fcache->file_offset, SEEK_SET);
        size = fread(fcache->ptr, 1, fcache->buffer_size-1, fcache->fp);
        fcache->ptr[size] = 0;
        fcache->start_offset = fcache->file_offset;
        fcache->read_size = size;
        *sstr = fcache->ptr;
    }

    if (size > read_size)
        size = read_size;
    fcache->file_offset += size;

    return size;
}

int fcache_seek(fcache_t *fcache, long offset, int whence)
{
    size_t offset_use = fcache->file_offset;
    switch (whence)
    {
        case SEEK_SET:
            offset_use = offset;
            break;
        case SEEK_CUR:
            offset_use += offset;
            break;
        case SEEK_END:
            offset_use = offset + fcache->file_size;
            break;
        default:
            return -1;
    }

    if (offset_use <= fcache->file_size)
    {
        fcache->file_offset = offset_use;
        return 0;
    }

    return -1;
}

int fcache_tell(fcache_t *fcache)
{
    return fcache->file_offset;
}

#ifdef MAIN
// gcc -o copy app_dir_module.c -lm -O0 -g -W -Wall -DMAIN
// gcc -o copy app_dir_module.c -lm -O2 -ffunction-sections -fdata-sections -W -Wall -DMAIN
static size_t s_test_file_total = 0;
static int _test_check(const char *path, int is_dir, size_t dir_count, size_t file_count, ssize_t file_size)
{
    int percent = (int)((dir_count + file_count) * 100 / s_test_file_total);
    if (is_dir)
        _FINFO("\033[34m[%2d%%][%s] %s\033[0m\n", percent, "DIR", path);
    //else
    //    _FINFO("[%2d%%][%s] %s(%d)\n", percent, "REG", path, (int)file_size);
    return 0;
}

int main(int argc, char *argv[])
{
    int i = 0, total = 0;
    size_t dir_count = 0, file_count = 0;
    if (argc != 3 && argc != 4) {
        _FINFO("Usage: %s <src> <dst>\n", argv[0]);
        _FINFO("Usage: %s <src> <dst> <count>\n", argv[0]);
        return -1;
    }

    if (argc == 4) total = atoi(argv[3]);
    if (total <= 0) total = 1;

    for (i = 0; i < total; i++) {
        _FINFO("\033[31m--------%d/%d--------\033[0m\n", i+1, total);
        if (app_dir_or_file_tell(argv[1], 0, NULL, &dir_count, &file_count) < 0)
            return -1;
        s_test_file_total = dir_count + file_count;
        if (app_dir_or_file_copy(argv[1], argv[2], 0, 8192, _test_check) < 0)
            return -1;
        if (app_dir_or_file_check(argv[1], argv[2], 0, NULL) < 0)
            return -1;
        sync();
    }
    return 0;
}
#endif
