#include "app.h"

#define MAX_USB_NUM    (3)

typedef struct
{
    char *pos;
    char *dev;  //1-1,1-2,1-3
}UsbPosMap;

/////depend on hardware/////
static UsbPosMap s_pos_map[MAX_USB_NUM] =
{
    {"back-1", "1-1"},
    {"back-2", "1-2"},
    {"front", "1-3"},
};

static int _parse_usbdev_name(char *src, char *ret_buf)
{
    char *p = NULL;
    int ret = 0;

    if((src == NULL) || (ret_buf == NULL))
        return 0;

    if((p = strstr(src, "usb")) != NULL)
    {
        if((p = strchr(p, '/')) != NULL)
        {
            char *q = NULL;
            p++;
            if((q = strchr(p, '/')) != NULL)
            {
                ret = q-p;
                memcpy(ret_buf, p, ret);
            }
        }
    }

    return ret;
}

static char *_get_usbdev_name(char *dev_name)
{
    char *dev = NULL;
    char cmdline[128] = {0};
    char buffer[128] = {0};
    FILE *read_fp = NULL;
    int chars_read = 0;
    char *usbdev = NULL;
    static char usbdev_buf[10] = {0};

    if(dev_name == NULL)
        return NULL;

    dev = strrchr(dev_name, '/');
    if(dev == NULL)
        return NULL;

    dev++;
    dev[3] = 0;
    sprintf(cmdline, "readlink /sys/block/%s/device", dev);
    //app_log_debug("###[%s]%d cmdline: %s ###\n", __func__, __LINE__, cmdline);

    read_fp = popen(cmdline, "r");
    if (read_fp != NULL)
    {
        chars_read = fread(buffer, sizeof(char), sizeof(buffer) - 1, read_fp);
        if (chars_read > 0)
        {
            //app_log_debug("[%s]%d, %s", __func__, __LINE__, buffer);
            memset(usbdev_buf, 0, sizeof(usbdev_buf));
            if(_parse_usbdev_name(buffer, usbdev_buf) > 0)
            {
                usbdev = usbdev_buf;
                //app_log_debug("[%s]%d %s\n", __func__, __LINE__, usbdev);
            }
        }
        pclose(read_fp);
    }

    return usbdev;
}

static char *_get_usbdev_pos(char *usb_dev)
{
    int i = 0;

    if(usb_dev == NULL)
        return NULL;

    for(i = 0; i < MAX_USB_NUM; i++)
    {
        if(strcmp(usb_dev, s_pos_map[i].dev) == 0)
        {
            return s_pos_map[i].pos;
        }
    }

    return NULL;
}

void app_check_usb_pos(void)
{
    int i = 0;
    char *ss = NULL;
    HotplugPartitionList* partition_list = NULL;

    partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
    if(partition_list == NULL)
        return;

    for(i = 0; i < partition_list->partition_num; i++)
    {
        ss = _get_usbdev_name(partition_list->partition[i].dev_name);
        if(ss != NULL)
        {
            ss = _get_usbdev_pos(ss);
            if(ss != NULL)
            {
                app_log_min("\n##################################\n");
                app_log_min("%s, %s, %s\n", partition_list->partition[i].partition_name, partition_list->partition[i].dev_name, ss);
                app_log_min("##################################\n");
            }
        }
    }
}

