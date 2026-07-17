#ifndef __APP_DIR_MODULE_H__
#define __APP_DIR_MODULE_H__
#include <sys/types.h>

typedef int (*file_check_cb_t) (const char *path, int is_dir, size_t dir_count, size_t file_count, ssize_t file_size);
int app_dir_or_file_delete(const char *path, size_t max_depth, file_check_cb_t check_cb);
int app_dir_or_file_tell(const char *path, size_t max_depth, file_check_cb_t check_cb, size_t *dir_count, size_t *file_count);
int app_dir_or_file_copy(const char *src, const char *dst, size_t max_depth, size_t buf_size, file_check_cb_t check_cb);
int app_dir_or_file_check(const char *src, const char *dst, size_t max_depth, file_check_cb_t check_cb);
int app_copy_file_to_file(const char *src, const char *dst, size_t buf_size);

typedef struct {
    FILE *fp;                   // 文件指针
    size_t file_size;           // 文件大小
    size_t file_offset;         // 文件偏移
    size_t buffer_size;         // buffer大小
    size_t start_offset;        // 读取开始偏移
    size_t read_size;           // 读取到的数目
    char *ptr;                  // 文件缓冲buffer
} fcache_t;

int fcache_open(fcache_t *fcache, const char *file_name, size_t buffer_size);
void fcache_close(fcache_t *fcache);
size_t fcache_read(fcache_t *fcache, size_t read_size, char **sstr);
int fcache_seek(fcache_t *fcache, long offset, int whence);
int fcache_tell(fcache_t *fcache);
#endif
