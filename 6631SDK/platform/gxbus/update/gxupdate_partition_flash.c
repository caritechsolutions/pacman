/**
 *
 * @file        gxupdate_partition_flash.c
 * @brief
 * @version     1.1.0
 * @date        03/23/2010 03:54:25 PM
 * @author      Li Xiaobin (lixb), lixb@nationalchip.com
 *
 */
#include <sys/ioctl.h>
#include <fcntl.h>
#include "update/gxupdate_partition_flash.h"
#include "update/gxupdate_def.h"
#include "gxupdate_debug.h"

#define IO_GET_CONFIG_FLASH_DEVSIZE             0x605
#define IO_GET_CONFIG_FLASH_BLOCKSIZE           0x606
#define IO_GET_CONFIG_FLASH_ERASE               0x600
#define IO_SET_CONFIG_FLASH_SYNC                0x630
#define IO_GET_CONFIG_FLASH_DEVADDR             0x607
#define IO_GET_CONFIG_FLASH_UNLOCK              0x603

struct flash_addr{
    uint32_t    addr;
};

struct gxupdate_flash_block {
    uint32_t    start_offset;
    size_t      block_size;
    uint8_t*    blk_buf;
};

struct gxupdate_flash_block_size {
    uint32_t    offset;
    size_t      block_size;
};

typedef struct gxupdate_flash_erase {
    uint32_t    offset;
    size_t      len;
    int         flasherr;
    uint32_t    err_address;
}gxupdate_flash_erase_t;

typedef gxupdate_flash_erase_t gxupdate_flash_unlock_t;

struct gxupdate_flash {
    char*                           name;
    int                             fd;
    uint32_t                        start_offset;
    size_t                          size;
    size_t                          read_offset;
    size_t                          write_offset;
    struct gxupdate_flash_block*    blk;
};

static struct gxupdate_flash  flash[GXUPDATE_MAX_NUM_OPEN_FLASH] = {{0,},};
static bool flashlock = false;

static int32_t flash_config(struct gxupdate_flash* fs, GxUpdate_ConfigFlash* config)
{
    int32_t             ret;
    struct flash_addr   flash;

    ASSERT(fs != NULL);
    ASSERT(config != NULL);

    ret = ioctl(fs->fd, IO_GET_CONFIG_FLASH_DEVADDR, &flash, sizeof(struct flash_addr));
    ASSERT(ret >= 0);

    fs->start_offset    = config->start_addr - flash.addr;
    fs->size            = config->size;
    fs->write_offset    = fs->start_offset;
    fs->read_offset     = fs->start_offset;

    lseek(fs->fd, fs->start_offset, SEEK_SET);
    ASSERT(ret >= 0);

    return ret;
}

static void get_blokc_start_addr(struct gxupdate_flash*         fs,
                                 uint32_t                       offset,
                                 struct gxupdate_flash_block*   blk)
{
    int32_t                             ret;
    struct gxupdate_flash_block_size    p;

    p.offset = offset;
    ret = ioctl(fs->fd, IO_GET_CONFIG_FLASH_BLOCKSIZE,
                    &p, sizeof(struct gxupdate_flash_block_size));
    ASSERT(ret >= 0);

    blk->block_size = p.block_size;
    blk->start_offset = (offset/blk->block_size)*blk->block_size;
}

static ssize_t block_write(struct gxupdate_flash*   fs,
                           const uint8_t*           ptr,
                           size_t                   size)
{
    int32_t                     ret;
    size_t                      unchanged_size;
    size_t                      changed_size;
    ssize_t                     do_size;
    ssize_t                     len = size;
    uint8_t*                    data = (uint8_t*)ptr;
    struct gxupdate_flash_erase erase = {0,};

    if (fs == NULL || (ptr == NULL && size != 0) || fs->size == 0) {
        return -1;
    }

    do {
        if (fs->blk == NULL) {
            fs->blk = GxCore_Calloc(1, sizeof(struct gxupdate_flash_block));
            if (fs->blk == NULL) {
                return -1;
            }
            get_blokc_start_addr(fs, fs->write_offset, fs->blk);
            fs->blk->blk_buf = GxCore_Realloc(fs->blk->blk_buf, fs->blk->block_size);
            if (fs->blk->blk_buf == NULL) {
                GxCore_Free(fs->blk);
                return -1;
            }
            ret = lseek(fs->fd, fs->blk->start_offset, SEEK_SET);
            ASSERT(ret >= 0);
            do_size = read(fs->fd, fs->blk->blk_buf, fs->blk->block_size);
            ASSERT(do_size == fs->blk->block_size);
        }

        unchanged_size  = fs->write_offset - fs->blk->start_offset;
        changed_size    = fs->blk->block_size - unchanged_size;
        if (len >= changed_size) {
            memcpy(fs->blk->blk_buf + unchanged_size,
                   data, changed_size);
            data            += changed_size;
            erase.offset    = fs->blk->start_offset;
            erase.len       = fs->blk->block_size;

            ret = ioctl(fs->fd, IO_GET_CONFIG_FLASH_ERASE,
                    &erase, sizeof(struct gxupdate_flash_erase));
            ASSERT(ret >= 0);
            ret = lseek(fs->fd, fs->blk->start_offset, SEEK_SET);
            ASSERT(ret >= 0);
            do_size = write(fs->fd, fs->blk->blk_buf, fs->blk->block_size);
            ASSERT(do_size == fs->blk->block_size);

            fs->write_offset += changed_size;

            len -= changed_size;

            /*open a new block*/
            get_blokc_start_addr(fs, fs->write_offset, fs->blk);
            ret = lseek(fs->fd, fs->blk->start_offset, SEEK_SET);
            ASSERT(ret >= 0);

            do_size = read(fs->fd, fs->blk->blk_buf, fs->blk->block_size);
            ASSERT(do_size == fs->blk->block_size);

        } else {
            memcpy(fs->blk->blk_buf + unchanged_size,
                   data, len);
            fs->write_offset += len;
            return size;
        }
    } while(len > 0);
    return -1;
}

static handle_t flash_open(const char* name)
{
    int32_t i;

    if (name == NULL) {
        return (handle_t)E_INVALID_HANDLE;
    }

    for (i = 0; i < GXUPDATE_MAX_NUM_OPEN_FLASH; i++) {
        if (flash[i].name != NULL && strcmp(flash[i].name, name) == 0) {
            return (handle_t)&flash[i];
        }
    }

    for (i = 0; i < GXUPDATE_MAX_NUM_OPEN_FLASH; i++) {
        if (flash[i].name == NULL) {
            memset(&flash[i], 0, sizeof(struct gxupdate_flash));
            flash[i].name = GxCore_Strdup(name);
            if (flash[i].name == NULL) {
                return (handle_t)E_INVALID_HANDLE;
            }
            flash[i].fd = open(FLASH_DEVICE_NAME, O_RDWR);
            if (flash[i].fd < 0) {
                GxCore_Free(flash[i].name);
                flash[i].name = NULL;
                return (handle_t)E_INVALID_HANDLE;
            }

            lseek(flash[i].fd, flash[i].start_offset, SEEK_SET);
            return (handle_t)&flash[i];
        }
    }

    return (handle_t)E_INVALID_HANDLE;
}



static int32_t flash_ioctl(handle_t handle, int32_t key, void* buf, size_t size)
{
    struct gxupdate_flash* fs = (struct gxupdate_flash*)handle;
    if (fs == NULL || buf == NULL) {
        return 0;
    }

    switch(key) {
    case GXUPDATE_CONFIG_FLASH:
        ASSERT(size == sizeof(GxUpdate_ConfigFlash));
        return flash_config(fs, (GxUpdate_ConfigFlash*)buf);
    default:
        break;
    }

    return E_FAILURE;
}

static ssize_t flash_get_size(handle_t handle)
{
    struct gxupdate_flash* fs = (struct gxupdate_flash*)handle;
    if (fs == NULL) {
        return 0;
    }

    return fs->size;
}


static ssize_t flash_read(handle_t handle, uint8_t* buf, size_t size)
{
    struct gxupdate_flash* fs = (struct gxupdate_flash*)handle;
    ssize_t                 read_size;
    ssize_t                 do_size;

    if (fs == NULL) {
        return 0;
    }

    read_size = fs->size - fs->read_offset + fs->start_offset;
    read_size = read_size > size ? size : read_size;

    do_size = read(fs->fd, buf, read_size);
    ASSERT(read_size == do_size);

    fs->read_offset += read_size;

    return read_size;
}



static ssize_t flash_write(handle_t handle, const uint8_t* ptr, size_t size)
{
    struct gxupdate_flash* fs = (struct gxupdate_flash*)handle;
    size_t                  write_size = size;

    if (fs == NULL || ptr == NULL  || fs->size == 0) {
        return 0;
    }

    if(flashlock == false)
    {
        /*写flash前去除flash写保护*/
        gxupdate_flash_unlock_t flash_lock = {0};
        flash_lock.offset = 0;
        flash_lock.len = 0;
        ioctl(fs->fd, IO_GET_CONFIG_FLASH_UNLOCK, &flash_lock, sizeof(gxupdate_flash_unlock_t));
        flashlock = true;
    }

    write_size = fs->size - fs->write_offset + fs->start_offset;
    write_size = write_size > size ? size : write_size;

    return block_write(fs, ptr, write_size);
}

static int32_t flash_close(handle_t handle)
{
    int32_t                     ret;
    ssize_t                     do_size;
    struct gxupdate_flash_erase erase = {0,};
    struct gxupdate_flash*     fs = (struct gxupdate_flash*)handle;

    if ((fs->blk != NULL) && (fs->write_offset - fs->blk->start_offset != 0)) {
        erase.offset    = fs->blk->start_offset;
        erase.len       = fs->blk->block_size;
        ret = ioctl(fs->fd, IO_GET_CONFIG_FLASH_ERASE,
                &erase, sizeof(struct gxupdate_flash_erase));
        ASSERT(ret >= 0);

        ret = lseek(fs->fd, fs->blk->start_offset, SEEK_SET);
        ASSERT(ret >= 0);

        do_size = write(fs->fd, fs->blk->blk_buf, fs->blk->block_size);
        ASSERT(do_size == fs->blk->block_size);

        GxCore_Free(fs->blk->blk_buf);
        GxCore_Free(fs->blk);
    }
    GxCore_Free(fs->name);
    fs->name = NULL;

    ioctl(fs->fd, IO_SET_CONFIG_FLASH_SYNC,NULL,0);

    close(fs->fd);
    return E_OK;
}

GxUpdate_PartitionOps gxupdate_partition_flash =
{
    GXUPDATE_PARTITION_FLASH,
    flash_open,
    flash_ioctl,
    flash_get_size,
    flash_read,
    flash_write,
    flash_close
};

