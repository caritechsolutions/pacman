#include <stdio.h>
#include <wifi/gxwifi.h>

#define MAX_P2P_DEVICE 4
#define MAX_P2P_DEVICE_NAME_LEN 32

#ifdef ECOS_OS
#define SIOCIWFIRSTPRIV 0x8BE0
#define RTPRIV_IOCTL_GET_P2P	 (SIOCIWFIRSTPRIV + 0x06)
extern signed int ls_dev_p2p(char **dev_list, signed int cmd);
#elif defined LINUX_OS
extern int get_p2p_device(struct p2p_device_list *device_list);
extern void free_p2p_device(struct p2p_device_list *device_list);
#endif

int GxWiFi_GetP2PDevice(struct p2p_device_list *list)
{
#ifdef LINUX_OS
    return get_p2p_device(list);
#elif defined ECOS_OS
    int dev_num = 0;
    char *p2p_dev[MAX_P2P_DEVICE];
    int i = 0;

    if (!list) {
        return -1;
    }

    /* 初始化设备列表 */
    list->dev_names = NULL;
    list->count = 0;

    /* 分配临时设备指针数组的内存 */
    for (i = 0; i < MAX_P2P_DEVICE; i++) {
        p2p_dev[i] = GxCore_Mallocz(MAX_P2P_DEVICE_NAME_LEN);
        if (p2p_dev[i] == NULL) {
            goto cleanup;
        }
    }

    /* 获取P2P设备 */
    dev_num = ls_dev_p2p(p2p_dev, RTPRIV_IOCTL_GET_P2P);
    if (dev_num <= 0) {
        goto cleanup;
    }

    /* 分配设备名称指针数组 */
    list->dev_names = (char **)GxCore_Mallocz(dev_num * sizeof(char *));
    if (!list->dev_names) {
        goto cleanup;
    }

    /* 分配并复制设备名称 */
    for (i = 0; i < dev_num; i++) {
        list->dev_names[i] = (char *)GxCore_Mallocz(MAX_P2P_DEVICE_NAME_LEN);
        if (!list->dev_names[i]) {
            /* 释放已分配的内存 */
            while (--i >= 0) {
                GxCore_Free(list->dev_names[i]);
            }
            GxCore_Free(list->dev_names);
            list->dev_names = NULL;
            goto cleanup;
        }
        memcpy(list->dev_names[i], p2p_dev[i], MAX_P2P_DEVICE_NAME_LEN);
    }

    list->count = dev_num;

cleanup:
    /* 释放临时内存 */
    for (i = 0; i < MAX_P2P_DEVICE; i++) {
        if (p2p_dev[i] != NULL) {
            GxCore_Free(p2p_dev[i]);
            p2p_dev[i] = NULL;
        }
    }

    return (list->count > 0) ? 0 : -1;
#else
    return 0;
#endif
}

int GxWiFi_PutP2PDevice(struct p2p_device_list *list)
{
#ifdef LINUX_OS
    free_p2p_device(list);
    return 0;
#elif defined ECOS_OS
    int i;

    if (!list || !list->dev_names) {
        return 0;
    }

    /* 释放每个设备名称 */
    for (i = 0; i < list->count; i++) {
        if (list->dev_names[i]) {
            GxCore_Free(list->dev_names[i]);
            list->dev_names[i] = NULL;
        }
    }

    /* 释放设备名称指针数组 */
    GxCore_Free(list->dev_names);
    list->dev_names = NULL;
    list->count = 0;

    return 0;
#else
    return 0;
#endif
}
