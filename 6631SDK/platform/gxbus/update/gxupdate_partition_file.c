/**
 *
 * @file        gxupdate_file.c
 * @brief       升级datafs普通文件分区实现
 * @version     1.1.0
 * @date        03/23/2010 03:55:38 PM
 * @author      Li Xiaobin (lixb), lixb@nationalchip.com
 *
 */
#include "update/gxupdate_partition_file.h"

#include "gxupdate_debug.h"

struct gxupdate_file {
    char*   name;
    char*   file_name;
    FILE*   stream;
};

static struct gxupdate_file file[GXUPDATE_MAX_NUM_OPEN_FILE] = {{0,},};

static handle_t file_open(const char* name)
{
    int32_t i;

    if (name == NULL) {
        return (handle_t)E_INVALID_HANDLE;
    }

    for (i = 0; i < GXUPDATE_MAX_NUM_OPEN_FILE; i++) {
        if (file[i].name != NULL && strcmp(file[i].name, name) == 0) {
            return (handle_t)&file[i];
        }
    }

    for (i = 0; i < GXUPDATE_MAX_NUM_OPEN_FILE; i++) {
        if (file[i].name == NULL) {
            memset(&file[i], 0, sizeof(struct gxupdate_file));
            file[i].name = strdup(name);
            if (file[i].name == NULL) {
                return (handle_t)E_INVALID_HANDLE;
            }
            return (handle_t)&file[i];
        }
    }

    return (handle_t)E_INVALID_HANDLE;
}

static int32_t file_ioctl(handle_t handle, int32_t key, void* buf, size_t size)
{
    struct gxupdate_file* fs = (struct gxupdate_file*)handle;

    if (fs == NULL) {
        return E_INPUT;
    }
    switch(key) {
    case GXUPDATE_CONFIG_FILE:
        if (size == sizeof(GxUpdate_ConfigFile)) {
            fs->stream = fopen(((GxUpdate_ConfigFile*)buf)->file_name, "r+");
            if (fs->stream == NULL) {
                break;
            }
            return E_OK;
        }
        break;
    default:
    	break;
    }

    return E_FAILURE;
}

static ssize_t file_get_size(handle_t handle)
{
    struct gxupdate_file* fs = (struct gxupdate_file*)handle;

    if (fs == NULL && fs->stream == NULL) {
        return E_INPUT;
    }

    fseek(fs->stream, 0, SEEK_END);

    return ftell(fs->stream);
}

static ssize_t file_read(handle_t handle, uint8_t* buf, size_t size)
{
    struct gxupdate_file* fs = (struct gxupdate_file*)handle;

    if (fs == NULL && fs->stream == NULL) {
        return E_INPUT;
    }

    return fread(buf, 1, size, fs->stream);
}
static ssize_t file_write(handle_t handle, const uint8_t* ptr, size_t size)
{
    struct gxupdate_file* fs = (struct gxupdate_file*)handle;

    if (fs == NULL && fs->stream == NULL) {
        return E_INPUT;
    }

    return fwrite(ptr, 1, size, fs->stream);
}
static int32_t file_close(handle_t handle)
{
    struct gxupdate_file* fs = (struct gxupdate_file*)handle;

    if (fs == NULL) {
        return E_INPUT;
    }

    GxCore_Free(fs->file_name);
    fs->name = NULL;
    fclose(fs->stream);
    fs->stream = NULL;

    return E_OK;
}

GxUpdate_PartitionOps gxupdate_partition_file =
{
    GXUPDATE_PARTITION_FILE,
    file_open,
    file_ioctl,
    file_get_size,
    file_read,
    file_write,
    file_close
};
