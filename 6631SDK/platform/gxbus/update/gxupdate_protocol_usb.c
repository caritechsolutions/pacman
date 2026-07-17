/**
 *
 * @file        gxupdate_protocol_usb.c
 * @brief
 * @version     1.1.0
 * @date        05/10/2010 08:46:18 AM
 * @author      Li Xiaobin (lixb), lixb@nationalchip.com
 *
 */
#include <fcntl.h>
#include "update/gxupdate_protocol_usb.h"
#include "gxupdate_debug.h"
#include "gxos/gxcore_partition.h"
#include "gxos/gxcore_partition.h"
#include "gxcore.h"
#ifdef LINUX_OS
#include <zlib.h>
#else
#include <linux/zlib.h>
#endif

#define CONTENT_SIZE (64*1024)//flash bank 64K

enum gxupdate_ubs_cre_ctr{
    GXUPDATE_USB_CRC_DISABLE = 0,
    GXUPDATE_USB_CRC_ENABLE,
    GXUPDATE_USB_MEM_MODE,
};

struct gxupdate_usb {
    char*               name;
    uint8_t*            membuf;
    uint8_t*            buf_end;
    char                file[GXUPDATE_MAX_FILE_PATH_LENGTH];
    FILE*               stream;
    GxUpdate_Terminal   type;
    uint32_t            crc_enable;
    uint32_t            update_len;
};

struct gxupdate_usb_crc_list {
    uint32_t  content_size;
    uint8_t   content[CONTENT_SIZE];
    void*  next;
};


#ifdef LINUX_OS
#else
static struct partition usb_update_flash_partition;

static struct partition * usb_update_flash_part = NULL;

static struct gxupdate_usb usb[GXUPDATE_MAX_NUM_USB_OPEN];


#endif
uint32_t gxupdata_usb_file_read_crc(uint32_t partition, uint8_t* p_buf)
{
#ifdef LINUX_OS
#else
    int32_t new_crc = 0;
    uint32_t total_size;
    int32_t used_size;
    uint32_t start_addr;

    start_addr = usb_update_flash_part->tables[partition].start_addr;
    total_size = usb_update_flash_part->tables[partition].total_size;
    used_size = usb_update_flash_part->tables[partition].used_size;
    /*crc*/
    new_crc = crc32(0, (void*)(p_buf +start_addr), used_size);
    if(usb_update_flash_part->tables[partition].crc32_enable == 1 &&
       new_crc != usb_update_flash_part->tables[partition].crc_verify)
    {
        gxlogd("[update] %s  crc fail!!!\n",usb_update_flash_part->tables[partition].name);
        return 0;
    }
    else
    {
        gxlogd("[update] %s  crc sucess!!!\n",usb_update_flash_part->tables[partition].name);
	    return 1;
    }
#endif
    return 1;
}

uint32_t gxupdata_usb_file_open_crc(struct gxupdate_usb * usb)
{
#ifdef LINUX_OS
#else
    handle_t file = 0;
    uint32_t i = 0;

    usb_update_flash_part = GxCore_PartitionInit(&usb_update_flash_partition, NULL, usb->file);
    if(usb_update_flash_part == NULL ||
       usb_update_flash_part->count > FLASH_PARTITION_V2_NUM ||
       usb_update_flash_part->crc32_enable != 1) {
        gxlogd("[update] partiton table err partition = %p  count = %d  crc = %d!!\n",usb_update_flash_part,usb_update_flash_part->count,usb_update_flash_part->crc32_enable);
        return 0;
    }

    /*crc*/
    if(GxCore_PartitionVerity(usb_update_flash_part) == 0)
    {
        gxlogd("[update] TABLE crc fail!!\n");
        return 0;
    }

    /*crc*/
    file = GxCore_Open(usb->file, "r");
    if(file < 0)
    {
        return 0;
    }
    for(i=0; i<usb_update_flash_part->count; i++)
    {
        if(gxupdata_usb_file_read_crc(i,usb->membuf) != 1)
        {
			GxCore_Close(file);
            return 0;
        }
    }
	GxCore_Close(file);
#endif
	return 1;
}

uint32_t gxupdate_usb_read_crc(struct gxupdate_usb* usb,uint8_t* buf, ssize_t* size)
{
#ifdef LINUX_OS
	return 0;
#else
    int32_t ret = GXUPDATE_STREAM_CONTINUE;

    if (usb->membuf == NULL || buf == NULL)
        return -1;

    if (usb->membuf >= usb->buf_end)
	    return -1;

    if(*size >= (usb->buf_end - usb->membuf))
    {
	    *size = usb->buf_end - usb->membuf;
        ret = GXUPDATE_STREAM_FINISH;
    }

    memcpy(buf, usb->membuf, *size);
    usb->membuf += *size;
    return ret;
#endif
}

static handle_t gxupdate_usb_open(const char* name)
{
#ifdef LINUX_OS
#else
    int32_t     i;

    if (name == NULL) {
        return E_INVALID_HANDLE;
    }

    for (i = 0; i < GXUPDATE_MAX_NUM_USB_OPEN; i++) {
        if (usb[i].name == NULL) {
            memset(usb, 0, sizeof(struct gxupdate_usb));
            return (handle_t)&usb[i];
        }
    }
#endif
    return E_INVALID_HANDLE;
}

static GxUpdate_Terminal gxupdate_usb_get_type(handle_t handle)
{
    struct gxupdate_usb* usb = (struct gxupdate_usb*)handle;

    return usb->type;
}

static int32_t gxupdate_usb_set_size(handle_t handle, size_t size)
{
    return E_OK;
}

static int32_t gxupdate_usb_ioctl(handle_t handle,
                                  int32_t key, void* buf, size_t size)
{
#ifdef LINUX_OS
#else
    struct gxupdate_usb* usb = (struct gxupdate_usb*)handle;

    switch(key) {
    case GXUPDATE_USB_SELECT_TERMINAL_TYPE:
        if (size != sizeof(GxUpdate_ConfigUsbTerminalType)) {
             return E_FAILURE;
        }
        usb->type = ((GxUpdate_ConfigUsbTerminalType*)buf)->type;
        break;
    case GXUPDATE_SELECT_FILE_NAME:
        if (size != sizeof(GxUpdate_SelectUsbFile)) {
            return E_FAILURE;
        }
        strncpy(usb->file, buf, sizeof(usb->file)-1);
        if (usb->type == GXUPDATE_CLIENT) {
            if(usb->crc_enable == GXUPDATE_USB_CRC_ENABLE) {
                if(gxupdata_usb_file_open_crc(usb)) {
                    break;
                }
                else {
                    return E_FAILURE;
                }
            }
            else {
                usb->stream = fopen(buf, "r+");
            }
        } else {
            usb->stream = fopen(buf, "w+");
        }
        if (usb->stream == NULL) {
            return E_FAILURE;
        }

        break;
    case GXUPDATE_USB_SET_CRC:
        usb->crc_enable = GXUPDATE_USB_CRC_ENABLE;
	    usb->update_len = ((GxUpdate_ConfigUsbMemData*)buf)->update_length;
	    usb->membuf = ((GxUpdate_ConfigUsbMemData*)buf)->protocol_membuf;
	    usb->buf_end = usb->membuf + usb->update_len;
        break;
    case GXUPDATE_USB_USER_MEM_MODE:
        usb->crc_enable = GXUPDATE_USB_MEM_MODE;
        usb->update_len = ((GxUpdate_ConfigUsbMemData*)buf)->update_length;
        usb->membuf = ((GxUpdate_ConfigUsbMemData*)buf)->protocol_membuf;
        break;
    default:
        break;
    }
#endif
    return E_OK;
}
static uint32_t gxupdate_usb_read(handle_t handle, uint8_t* buf, ssize_t* size)
{
#ifdef LINUX_OS
#else
    static int read_size = 0;
    struct gxupdate_usb* usb = (struct gxupdate_usb*)handle;
    if(usb->crc_enable == GXUPDATE_USB_CRC_DISABLE)
    {
        ASSERT(usb->stream != NULL);

        read_size =  fread(buf, 1, *size, usb->stream);
        if(read_size < *size)
        {
            *size = read_size;
            return GXUPDATE_STREAM_FINISH;
        }
        return GXUPDATE_STREAM_CONTINUE;
    }
    else if(usb->crc_enable == GXUPDATE_USB_MEM_MODE)
    {
        memcpy(buf,usb->membuf+read_size,*size);
        read_size += *size;
        if(read_size == usb->update_len)
        {
			read_size = 0;
            return GXUPDATE_STREAM_FINISH;
        }
        return GXUPDATE_STREAM_CONTINUE;
    }
    else
    {
        return gxupdate_usb_read_crc(usb,buf,size);
    }
#endif
	return 0;
}

static ssize_t gxupdate_usb_write(handle_t handle,
                                  const uint8_t* ptr, size_t size)
{
    struct gxupdate_usb* usb = (struct gxupdate_usb*)handle;

    ASSERT(usb->stream != NULL);

    return fwrite(ptr, 1, size, usb->stream);

}

static int32_t gxupdate_usb_close(handle_t handle)
{
    struct gxupdate_usb* usb = (struct gxupdate_usb*)handle;
    ASSERT(usb->stream != NULL);
    fclose(usb->stream);
    usb->name = NULL;
    return E_OK;
}

GxUpdate_ProtocolOps gxupdate_protocol_usb = {
    GXUPDATE_PROTOCOL_USB,
    gxupdate_usb_open,
    gxupdate_usb_get_type,
    gxupdate_usb_set_size,
    gxupdate_usb_ioctl,
    gxupdate_usb_read,
    gxupdate_usb_write,
    gxupdate_usb_close,
};

