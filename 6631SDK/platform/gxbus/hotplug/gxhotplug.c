#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gxmsg.h"
#include "gxcore.h"
#include "gxhotplug.h"
#include "module/config/gxconfig.h"

#include "gxeth_hotplug.h"
#include "gxhotplug.h"
#include "gxusb3gplug.h"
#include "gxusbwifiplug.h"
#include "gxusbnetplug.h"
#include "gxusbbtplug.h"
#include "gxusbhidplug.h"
#include "gxusbuvcplug.h"
#include "gxusbaudioplug.h"
#ifdef LINUX_OS
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <errno.h>
#include <net/if_arp.h>
#endif

static HotplugPartitionList hp_unmounted_partition_usb;
static HotplugPartitionList hp_unmounted_partition_mmc;

static HotplugPartitionList hotplug_partition_usb;
static HotplugPartitionList hotplug_partition_mmc;
static struct gxlist_head   hotplug_out_dev_list;
static handle_t             hotplug_out_dev_mutex = -1;
struct hotplug_out_dev {
	struct gxlist_head head;
	char dev_name[16];
};

static char* disk_name_map[26] = {NULL, };
static int bt_cbk_ref;

static int cmp(const void *p1 , const void *p2)
{
	HotplugPartition *d1 = (HotplugPartition*)p1, *d2 = (HotplugPartition*)p2;

	return d1->disk_id - d2->disk_id;
}

void partition_sort(HotplugPartitionList*list)
{
	qsort(list->partition, list->partition_num, sizeof(HotplugPartition), cmp);
}

static char GetDiskName(const char *disk_name) {
	int i;

	// 判断是否已经分配过
	for (i=0; i < 26; i++) {
		if (disk_name_map[i] != NULL && strcmp(disk_name_map[i], disk_name) == 0)
			return 'C' + i;
	}

	// 如果未分配，则分配一个最小的盘符:
	for (i=0; i < 26; i++) {
		if (disk_name_map[i] == NULL) {
			disk_name_map[i] = GxCore_Strdup(disk_name);
			return 'C' + i;
		}
	}

	return '\0';
}

static void FreeDiskName(char d) {
	int id = d - 'C';
	if (id >= 0 && id < 26 && disk_name_map[id] != NULL) {
		GxCore_Free(disk_name_map[id]);
		disk_name_map[id] = NULL;
	}
}

static void partition_update(void)
{
	int major, minor, blocks;
	char name[128];
	char buffer[129];

	FILE *fp = fopen("/proc/partitions", "r");

	if (fp == NULL)
		return;

	while (fgets(buffer, 128, fp) != NULL ) {
		if (sscanf(buffer,"%d %d %d %s", &major, &minor, &blocks, name) != 4)
			continue;

		if (strncmp(name, "sd", 2) == 0 && strlen(name)> 3 && blocks > 1) {
			snprintf(buffer, sizeof(buffer), "/dev/%s", name);
			GetDiskName(buffer);
		}
	}

	fclose(fp);
}
#ifdef LINUX_OS
static void GxNetStatusServiceConsole(handle_t self)
{
	char name[128];
	char buffer[129];
	char *substr;
	static char last_wifi_map = 0;
	char new_wifi_map = 0;
	static char last_eth_map = 0;
	char new_eth_map = 0;
	int i, link;
	int fd = -1;
	FILE *fp;
	fp = fopen("/proc/net/wireless", "r");
	if (fp == NULL)
		goto check_netlink;

	while (fgets(buffer, sizeof(buffer), fp) != NULL ) {
		i = sscanf(buffer," %[^:]: %*d %d %*s", name, &link);
		if (i != 2){
			continue;
		}
		if((strncmp(name, WIFI_DEV_NAME, WIFI_DEV_NAME_LEN) == 0)&&(link > 10)){
			int numb = name[WIFI_DEV_NAME_LEN] - 0x30;
			if(numb >= 0 && numb < 8){
				new_wifi_map |= (1 << numb);
			}
		}
	}
	fclose(fp);
	if(last_wifi_map == new_wifi_map)
		goto check_netlink;
	last_wifi_map = last_wifi_map^new_wifi_map;
	for(i = 0; last_wifi_map != 0; i++){
		if(last_wifi_map & 0x01){
			GxMessage *msg = NULL;
			GxMsgProperty_WifiStatus* plug = NULL;
			msg = GxBus_MessageNew(GXMSG_USBWIFI_STATUS);
			if(NULL == msg)
				break;
			plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_WifiStatus);
			if(NULL == plug)
				break;
			strncpy(plug->dev_name, WIFI_DEV_NAME"0", WIFI_DEV_NAME_LEN + 1);
			plug->dev_name[WIFI_DEV_NAME_LEN] = i + 0x30;
			plug->status = (new_wifi_map>>i)&0x01;
			GxBus_MessageSend(msg);
		}
		last_wifi_map >>= 1;
	}
	last_wifi_map = new_wifi_map;

check_netlink:
	fp = fopen("/proc/net/dev", "r");
	if(fp != NULL)
	{
		fgets(buffer, sizeof(buffer), fp);
		fgets(buffer, sizeof(buffer), fp);

		/* Read each device line */
		while(fgets(buffer, sizeof(buffer), fp))
		{
			if ((substr = strstr(buffer, ETH_DEV_NAME)))
			{
				struct ethtool_value edata;
				struct ifreq ifr;
				edata.cmd = ETHTOOL_GLINK;
				edata.data = 0;
				memset(&ifr, 0, sizeof(ifr));
				strncpy(ifr.ifr_name, substr, ETH_DEV_NAME_LEN + 1);
				ifr.ifr_data = (char *) &edata;
				if ((fd = socket( AF_INET, SOCK_DGRAM, 0 )) >= 0)
				{
					if(ioctl(fd, SIOCETHTOOL, &ifr ) >= 0){
						int numb = substr[ETH_DEV_NAME_LEN] - 0x30;
						new_eth_map |= (edata.data << numb);
					}
					close(fd);
				}
			}
		}
		fclose(fp);
	}
	if(last_eth_map == new_eth_map)
		return;
	last_eth_map = last_eth_map^new_eth_map;
	for(i = 0; last_eth_map != 0; i++){
		if(last_eth_map & 0x01){
			char status = (new_eth_map>>i)&0x01;
			GxMessage *msg = NULL;
			if(status)
				msg = GxBus_MessageNew(GXMSG_ETH_HOTPLUG_IN);
			else
				msg = GxBus_MessageNew(GXMSG_ETH_HOTPLUG_OUT);

			if(NULL == msg)
				break;
			GxMsgProperty_EthHotPlug* plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_EthHotPlug);
			if(NULL == plug)
				break;

			plug->id = i;
			strncpy(plug->dev_name, ETH_DEV_NAME"0", ETH_DEV_NAME_LEN + 1);
			plug->dev_name[ETH_DEV_NAME_LEN] = i + 0x30;
			GxBus_MessageSend(msg);
		}
		last_eth_map >>= 1;
	}
	last_eth_map = new_eth_map;

	GxCore_ThreadDelay(800);
}

status_t GxNetStatusServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;

	GxBus_MessageRegister(GXMSG_USBWIFI_STATUS, sizeof(GxMsgProperty_WifiStatus));
	GxBus_MessageRegister(GXMSG_ETH_HOTPLUG_IN, sizeof(GxMsgProperty_EthHotPlug));
	GxBus_MessageRegister(GXMSG_ETH_HOTPLUG_OUT, sizeof(GxMsgProperty_EthHotPlug));

	sch = GxBus_SchedulerCreate("WifiStatusConsoleScheduler", 1, 1024 * 8, GXOS_DEFAULT_PRIORITY +priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

void GxNetStatusServiceDestroy(handle_t self)
{
	GxBus_MessageUnregister(GXMSG_USBWIFI_STATUS);
	GxBus_MessageUnregister(GXMSG_ETH_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_ETH_HOTPLUG_OUT);

	GxBus_ServiceUnlink(self);
}
#endif

static int DiskExists(struct gxfs_hot_device* device)
{
	int i;
	int len = 0;

	for (i = 0; i < hotplug_partition_usb.partition_num; i++) {
		len = strlen(device->dev_name);
		if (strncmp(device->dev_name, hotplug_partition_usb.partition[i].dev_name, len) == 0)
			return 0;
	}
	for (i = 0; i < hp_unmounted_partition_usb.partition_num; i++) {
		len = strlen(device->dev_name);
		if (strncmp(device->dev_name, hp_unmounted_partition_usb.partition[i].dev_name, len) == 0)
			return 0;
	}

	for (i = 0; i < hotplug_partition_mmc.partition_num; i++) {
		len = strlen(device->dev_name);
		if (strncmp(device->dev_name, hotplug_partition_mmc.partition[i].dev_name, len) == 0)
			return 0;
	}
	for (i = 0; i < hp_unmounted_partition_mmc.partition_num; i++) {
		len = strlen(device->dev_name);
		if (strncmp(device->dev_name, hp_unmounted_partition_mmc.partition[i].dev_name, len) == 0)
			return 0;
	}

	return -1;
}

static void HotplugOutDeviceAdd(const char *dev_name)
{
	struct hotplug_out_dev *dev = GxCore_Mallocz(sizeof(struct hotplug_out_dev));
	if (dev != NULL) {
		strncpy(dev->dev_name, dev_name, sizeof(dev->dev_name) - 1);
		gxlist_add_tail((struct gxlist_head *)dev, &hotplug_out_dev_list);
	}
}

static void HotplugOutDeviceDel(const char *dev_name)
{
	struct gxlist_head* pos;
	struct gxlist_head* n;

	if (dev_name != NULL) {
		gxlist_for_each_safe(pos, n, &hotplug_out_dev_list) {
			struct hotplug_out_dev *dev = gxlist_entry(pos, struct hotplug_out_dev, head);
			if (strcmp(dev->dev_name, dev_name) == 0) {
				gxlist_del(pos);
				GxCore_Free(dev);
				return;
			}
		}
	}
}

static void HotplugOutDeviceDelAll(void)
{
	struct gxlist_head* pos;
	struct gxlist_head* n;

	gxlist_for_each_safe(pos, n, &hotplug_out_dev_list) {
		struct hotplug_out_dev *dev = gxlist_entry(pos, struct hotplug_out_dev, head);
		gxlist_del(pos);
		GxCore_Free(dev);
	}
}

static int HotplugOutDeviceCheck(const char *dev_name)
{
	struct gxlist_head* pos;
	struct gxlist_head* n;

	if (dev_name != NULL) {
		gxlist_for_each_safe(pos, n, &hotplug_out_dev_list) {
			struct hotplug_out_dev *dev = gxlist_entry(pos, struct hotplug_out_dev, head);
			if (strcmp(dev->dev_name, dev_name) == 0) {
				return 1;
			}
		}
	}

	return 0;
}

static status_t HotplugPartitionAdd(HotplugType type, struct gxfs_hot_device* device)
{
	struct gxfs_partition* partition = NULL;
	HotplugPartition *hot_partition;
	HotplugPartitionList* list = NULL;
	HotplugPartitionList* unmounted_list = NULL;
	int count = 0;
	int is_mounted = 0;

	if ((HOTPLUG_TYPE_USB != type && HOTPLUG_TYPE_MMC != type) || device == NULL)
		return GXCORE_ERROR;

	if (DiskExists(device) == 0)
		return GXCORE_SUCCESS;

	gxlogd("[HOTPLUG] add device %s %s\n", device->dev_name, device->dev_path);

	partition_update(); // 检查 /proc/partitions, 事先分配好盘符

	if(HOTPLUG_TYPE_USB == type) {
		list = &hotplug_partition_usb;
		unmounted_list = &hp_unmounted_partition_usb;
	} else if(HOTPLUG_TYPE_MMC == type) {
		list = &hotplug_partition_mmc;
		unmounted_list = &hp_unmounted_partition_mmc;
	}

	unmounted_list->type = list->type = type;

	count = list->partition_num + unmounted_list->partition_num + device->partition_count;
	unmounted_list->max_partition_num = list->max_partition_num = count;

	list->partition           = GxCore_Realloc(list->partition, count * sizeof(HotplugPartition));
	unmounted_list->partition = GxCore_Realloc(unmounted_list->partition, count * sizeof(HotplugPartition));

	gxlist_for_each_entry(partition, &device->partition_head, head) {
		gxlogd("[HOTPLUG] add partition %s\n", partition->path_name);

		is_mounted = partition->mount_status == MS_MOUNTED ? 1 : 0;

		if (is_mounted)
			hot_partition = list->partition + list->partition_num;
		else
			hot_partition = unmounted_list->partition + unmounted_list->partition_num;

		memset (hot_partition, 0, sizeof(HotplugPartition));

		if (is_mounted) {
			hot_partition->disk_id = GetDiskName(partition->dev_name);
			if(HOTPLUG_TYPE_USB == type)
				snprintf(hot_partition->partition_name, sizeof(hot_partition->partition_name),
						"USB (%c:)", hot_partition->disk_id);
			else if(HOTPLUG_TYPE_MMC == type)
				snprintf(hot_partition->partition_name, sizeof(hot_partition->partition_name),
						"MMC (%c:)", hot_partition->disk_id);
		}

		strncpy(hot_partition->partition_entry, partition->path_name, sizeof(hot_partition->partition_entry) - 1);
		strncpy(hot_partition->dev_name,        partition->dev_name,  sizeof(hot_partition->dev_name) - 1);
		strncpy(hot_partition->fs_type,         partition->fs_type,   sizeof(hot_partition->fs_type) - 1);
		strncpy(hot_partition->dev_path,        device->dev_path,     sizeof(hot_partition->dev_path) - 1);

		if (is_mounted)
			list->partition_num++;
		else
			unmounted_list->partition_num++;
	}

	return GXCORE_SUCCESS;
}

HotplugPartitionList* GxHotplugUmountedPartitionGet(HotplugType type)
{
	if(HOTPLUG_TYPE_USB != type && HOTPLUG_TYPE_MMC != type)
		return NULL;

	if(HOTPLUG_TYPE_USB == type)
		return &hp_unmounted_partition_usb;
	else if(HOTPLUG_TYPE_MMC == type)
		return &hp_unmounted_partition_mmc;

	return NULL;
}

static inline int partition_is_in_list (HotplugPartition *part, HotplugPartitionList *list)
{
	int i;
	int found = 0;

	if (!list || !part)
		return 0;

	for (i = 0; i < list->partition_num; i++)
		if (part == (list->partition + i)) {
			found = 1;
			break;
		}

	return found;
}

status_t HotplugPartitionMount(HotplugPartition *part, HotplugType type)
{
	int part_num_to_move;
	int ret;
	HotplugPartitionList *unmounted_list = ((type == HOTPLUG_TYPE_USB) ? &hp_unmounted_partition_usb : &hp_unmounted_partition_mmc);
	HotplugPartitionList *mounted_list = ((type == HOTPLUG_TYPE_USB) ? &hotplug_partition_usb : &hotplug_partition_mmc);

	if (!part)
		return GXCORE_INVALID_POINTER;

	if (!partition_is_in_list (part, unmounted_list))
		return GXCORE_ERROR;

	ret = GxCore_HotplugMountPartition(part->dev_name, part->partition_entry, part->fs_type);
	if (ret != GXCORE_SUCCESS)
		return ret;

	part->disk_id = GetDiskName(part->dev_name);
	if(HOTPLUG_TYPE_USB == type)
		snprintf(part->partition_name, sizeof(part->partition_name),
				"USB (%c:)", part->disk_id);
	else if(HOTPLUG_TYPE_MMC == type)
		snprintf(part->partition_name, sizeof(part->partition_name),
				"MMC (%c:)", part->disk_id);

	// 从未挂载列表中剔除，并加入到已挂载列表
	memcpy(mounted_list->partition + mounted_list->partition_num, part, sizeof(HotplugPartition));
	part_num_to_move = (unmounted_list->partition + unmounted_list->partition_num) - (part + 1);
	if (part_num_to_move > 0)
		memmove(part, part + 1, part_num_to_move * sizeof(HotplugPartition));
	mounted_list->partition_num++;
	unmounted_list->partition_num--;
	memset(unmounted_list->partition + unmounted_list->partition_num, 0, \
			(unmounted_list->max_partition_num - unmounted_list->partition_num) * sizeof(HotplugPartition));

	partition_sort(mounted_list);

	return ret;
}

status_t HotplugPartitionUmount(HotplugPartition *part, HotplugType type)
{
	int part_num_to_move;
	HotplugPartitionList *unmounted_list = ((type == HOTPLUG_TYPE_USB) ? &hp_unmounted_partition_usb : &hp_unmounted_partition_mmc);
	HotplugPartitionList *mounted_list = ((type == HOTPLUG_TYPE_USB) ? &hotplug_partition_usb : &hotplug_partition_mmc);
	int ret;

	if (!part)
		return GXCORE_INVALID_POINTER;

	if (!partition_is_in_list (part, mounted_list))
		return GXCORE_ERROR;

	ret = GxCore_HotplugUmountPartition(part->partition_entry);
	if (ret != GXCORE_SUCCESS)
		return ret;

	FreeDiskName(part->disk_id);

	// 从已挂载列表中去除，加入到未挂载列表
	memcpy (unmounted_list->partition + unmounted_list->partition_num, part, sizeof(HotplugPartition));
	part_num_to_move = (mounted_list->partition + mounted_list->partition_num) - (part + 1);
	if (part_num_to_move > 0)
		memmove(part, part + 1, part_num_to_move * sizeof(HotplugPartition));
	mounted_list->partition_num--;
	unmounted_list->partition_num++;
	memset(mounted_list->partition + mounted_list->partition_num, 0, \
			(mounted_list->max_partition_num - mounted_list->partition_num) * sizeof(HotplugPartition));

	return ret;
}

status_t HotplugPartitionDestroy(struct gxfs_hot_device* device)
{
	int i = 0, len;

	if(device == NULL) {
		return GXCORE_SUCCESS;
	}

	len = strlen(device->dev_name);
	/*USB*/
	if (device->type == USB) {
		while (i < hotplug_partition_usb.partition_num) {
			if (strncmp(device->dev_name, hotplug_partition_usb.partition[i].dev_name,len) == 0) {
				FreeDiskName(hotplug_partition_usb.partition[i].disk_id);
				memmove(hotplug_partition_usb.partition + i, hotplug_partition_usb.partition + i + 1,
						sizeof(HotplugPartition) * (hotplug_partition_usb.partition_num - i - 1));

				hotplug_partition_usb.partition_num--;
			} else
				i++;
		}

		i = 0;
		while (i < hp_unmounted_partition_usb.partition_num) {
			if (strncmp(device->dev_name, hp_unmounted_partition_usb.partition[i].dev_name,len) == 0) {
				//FreeDiskName(hp_unmounted_partition_usb.partition[i].disk_id);
				memmove(hp_unmounted_partition_usb.partition + i, hp_unmounted_partition_usb.partition + i + 1,
						sizeof(HotplugPartition) * (hp_unmounted_partition_usb.partition_num - i - 1));
				hp_unmounted_partition_usb.partition_num--;
			} else
				i++;
		}
	}
	else if (device->type == SD || device->type == CF) {
		while (i < hotplug_partition_mmc.partition_num) {
			if (strncmp(device->dev_name, hotplug_partition_mmc.partition[i].dev_name,len) == 0) {
				FreeDiskName(hotplug_partition_mmc.partition[i].disk_id);
				memmove(hotplug_partition_mmc.partition + i, hotplug_partition_mmc.partition + i + 1,
						sizeof(HotplugPartition) * (hotplug_partition_mmc.partition_num - i - 1));

				hotplug_partition_mmc.partition_num--;
			} else
				i++;
		}

		i = 0;
		while (i < hp_unmounted_partition_mmc.partition_num) {
			if (strncmp(device->dev_name, hp_unmounted_partition_mmc.partition[i].dev_name,len) == 0) {
				//FreeDiskName(hp_unmounted_partition_mmc.partition[i].disk_id);
				memmove(hp_unmounted_partition_mmc.partition + i, hp_unmounted_partition_mmc.partition + i + 1,
						sizeof(HotplugPartition) * (hp_unmounted_partition_mmc.partition_num - i - 1));
				hp_unmounted_partition_mmc.partition_num--;
			} else
				i++;
		}
	}

	return GXCORE_SUCCESS;
}


static status_t HotplugPartitionCreate(void)
{
	struct gxfs_hot_device* dev;

	GxCore_MutexLock(hotplug_out_dev_mutex);

	dev = GxCore_HotplugGetFirst();
	while(dev) {
		if (dev->type == USB && dev->subtype == USB_STORAGE) {
			if (HotplugOutDeviceCheck(dev->dev_name) == 0) {
				HotplugPartitionAdd(HOTPLUG_TYPE_USB, dev);
				partition_sort(&hotplug_partition_usb);
			}
		}
		else if (dev->type == SD || dev->type == CF) {
			HotplugPartitionAdd(HOTPLUG_TYPE_MMC, dev);
			partition_sort(&hotplug_partition_mmc);
		}

		dev = GxCore_HotplugGetNext(dev);
	}

	GxCore_MutexUnlock(hotplug_out_dev_mutex);

	return GXCORE_SUCCESS;
}

HotplugPartitionList* GxHotplugPartitionGet(HotplugType type)
{
	if(HOTPLUG_TYPE_USB != type && HOTPLUG_TYPE_MMC != type)
		return NULL;

	if(HOTPLUG_TYPE_USB == type)
		return &hotplug_partition_usb;
	else if(HOTPLUG_TYPE_MMC == type)
		return &hotplug_partition_mmc;

	return NULL;
}

#ifdef LINUX_OS
static void event_callback(GxBTCallbackEvent event, void *packet, int size)
{
	GxMessage *msg;
	GxMsgProperty_UsbBtScanResult *result;
	GxMsgProperty_UsbBtLinkKey    *key;
	GxMsgProperty_UsbBtPlayingInfo *pi;

	switch(event)
	{
		case GXBT_SCAN_RESULT:
			msg = GxBus_MessageNew(GXMSG_USBBT_SCAN_RESULT);
			if(NULL == msg)
				break;

			result = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbBtScanResult);
			if(NULL == result)
				break;

			memcpy(&result->info, packet, sizeof(GxBTDev));
			gxlogd ("device name: %s addr: \"%s\"\n", result->info.name, result->info.bd_addr_str);
			GxBus_MessageSend(msg);
			break;
		case GXBT_SCAN_FINISH:
			gxlogd (" <Status Changed> scan finish\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SCAN_FINISH);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_INIT_SUCCESSFUL:
			gxlogd (" <Status Changed> init success\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_INIT_SUCCESS);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_LINK_KEY_NOTIFY:
			msg = GxBus_MessageNew(GXMSG_USBBT_LINK_KEY_NOTIFY);
			if(NULL == msg)
				break;

			key = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbBtLinkKey);
			if(NULL == key)
				break;

			memcpy(&key->key, packet, sizeof(GxBTLinkKey));
			GxBus_MessageSend(msg);
			break;

		// sink
		case GXBT_A2DP_SINK_CONNECT:
			gxlogd (" <Status Changed> SINK CONNECT\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SINK_CONNECT);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SINK_CONNECT_FAILED:
			gxlogd (" <Status Changed> SINK CONNECT FAILED\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SINK_CONNECT_FAILED);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SINK_DISCONNECT:
			gxlogd (" <Status Changed> SINK DISCONNECT\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SINK_DISCONNECT);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SINK_PLAY:
			gxlogd (" <Status Changed> SINK PLAY\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SINK_PLAY);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SINK_PAUSE:
			gxlogd (" <Status Changed> SINK PAUSE\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SINK_PAUSE);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SINK_PLAYING_TITLE_INFO:
		case GXBT_A2DP_SINK_PLAYING_ARTIST_INFO:
		case GXBT_A2DP_SINK_PLAYING_ALBUM_INFO:
			if (event == GXBT_A2DP_SINK_PLAYING_TITLE_INFO)
				msg = GxBus_MessageNew(GXMSG_USBBT_SINK_PLAYING_TITLE_INFO);
			else if (event == GXBT_A2DP_SINK_PLAYING_ARTIST_INFO)
				msg = GxBus_MessageNew(GXMSG_USBBT_SINK_PLAYING_ARTIST_INFO);
			else if (event == GXBT_A2DP_SINK_PLAYING_ALBUM_INFO)
				msg = GxBus_MessageNew(GXMSG_USBBT_SINK_PLAYING_ALBUM_INFO);
			else
				msg = NULL;

			if(NULL == msg)
				break;

			pi = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbBtPlayingInfo);
			if(NULL == result)
				break;

			memcpy(pi->string, packet, size);
			if (event == GXBT_A2DP_SINK_PLAYING_TITLE_INFO)
				gxlogd ("Sink Audio Playing Title Info %s\n", pi->string);
			else if (event == GXBT_A2DP_SINK_PLAYING_ARTIST_INFO)
				gxlogd ("Sink Audio Playing Artist Info %s\n", pi->string);
			else if (event == GXBT_A2DP_SINK_PLAYING_ALBUM_INFO)
				gxlogd ("Sink Audio Playing Album Info %s\n", pi->string);
			GxBus_MessageSend(msg);
			break;

		// source
		case GXBT_A2DP_SOURCE_CONNECT:
			gxlogd (" <Status Changed> SOURCE CONNECT\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SOURCE_CONNECT);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SOURCE_CONNECT_FAILED:
			gxlogd (" <Status Changed> SOURCE CONNECT FAILED\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SOURCE_CONNECT_FAILED);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SOURCE_DISCONNECT:
			gxlogd (" <Status Changed> SOURCE DISCONNECT\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SOURCE_DISCONNECT);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SOURCE_PLAY:
			gxlogd (" <Status Changed> SOURCE PLAY\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SOURCE_PLAY);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SOURCE_PAUSE:
			gxlogd (" <Status Changed> SOURCE PAUSE\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SOURCE_PAUSE);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SOURCE_STREAM_ESTABLISHED:
			gxlogd (" <Status Changed> SOURCE STREAM ESTABLISHED\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SOURCE_STREAM_ESTABLISHED);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SOURCE_STREAM_RELEASED:
			gxlogd (" <Status Changed> SOURCE STREAM RELEASED\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SOURCE_STREAM_RELEASED);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		default:
			gxlogd ("%s Unknow msg : %d\n", __func__, event);
			break;
	}

	return ;
}
#endif

status_t GxHotPlugServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;

	GxCore_MutexCreate(&hotplug_out_dev_mutex);
	GX_INIT_LIST_HEAD(&hotplug_out_dev_list);

	GxBus_MessageRegister(GXMSG_HOTPLUG_DETECT, sizeof(GxMsgProperty_HotPlug));
	GxBus_MessageRegister(GXMSG_HOTPLUG_IN_FAILED_NO_PARTITION, sizeof(GxMsgProperty_HotPlug));
	GxBus_MessageRegister(GXMSG_HOTPLUG_IN_FAILED_MOUNT_FAILED, sizeof(GxMsgProperty_HotPlug));
	GxBus_MessageRegister(GXMSG_HOTPLUG_IN, sizeof(GxMsgProperty_HotPlug));
	GxBus_MessageRegister(GXMSG_HOTPLUG_OUT, sizeof(GxMsgProperty_HotPlug));
	GxBus_MessageRegister(GXMSG_HOTPLUG_CLEAN, sizeof(GxMsgProperty_HotPlug));
#ifdef LINUX_OS
	GxBus_MessageRegister(GXMSG_USB3G_HOTPLUG_IN, sizeof(GxMsgProperty_Usb3gHotPlug));
	GxBus_MessageRegister(GXMSG_USB3G_HOTPLUG_OUT, sizeof(GxMsgProperty_Usb3gHotPlug));
	GxBus_MessageRegister(GXMSG_USBWIFI_HOTPLUG_IN, sizeof(GxMsgProperty_UsbwifiHotPlug));
	GxBus_MessageRegister(GXMSG_USBWIFI_HOTPLUG_OUT, sizeof(GxMsgProperty_UsbwifiHotPlug));
	GxBus_MessageRegister(GXMSG_USBNET_HOTPLUG_IN, sizeof(GxMsgProperty_UsbnetHotPlug));
	GxBus_MessageRegister(GXMSG_USBNET_HOTPLUG_OUT, sizeof(GxMsgProperty_UsbnetHotPlug));
	GxBus_MessageRegister(GXMSG_USBHID_HOTPLUG_IN, sizeof(GxMsgProperty_UsbHidHotPlug));
	GxBus_MessageRegister(GXMSG_USBHID_HOTPLUG_OUT, sizeof(GxMsgProperty_UsbHidHotPlug));
	GxBus_MessageRegister(GXMSG_USBUVC_HOTPLUG_IN, sizeof(GxMsgProperty_UsbUvcHotPlug));
	GxBus_MessageRegister(GXMSG_USBUVC_HOTPLUG_OUT, sizeof(GxMsgProperty_UsbUvcHotPlug));
	GxBus_MessageRegister(GXMSG_USBAUDIO_HOTPLUG_IN, sizeof(GxMsgProperty_UsbaudioHotPlug));
	GxBus_MessageRegister(GXMSG_USBAUDIO_HOTPLUG_OUT, sizeof(GxMsgProperty_UsbaudioHotPlug));

	// bt
	GxBus_MessageRegister(GXMSG_USBBT_HOTPLUG_IN, sizeof(GxMsgProperty_UsbnetHotPlug));
	GxBus_MessageRegister(GXMSG_USBBT_HOTPLUG_OUT, sizeof(GxMsgProperty_UsbnetHotPlug));
	GxBus_MessageRegister(GXMSG_USBBT_INIT_SUCCESS,          0);
	GxBus_MessageRegister(GXMSG_USBBT_SCAN_RESULT,           sizeof(GxMsgProperty_UsbBtScanResult));
	GxBus_MessageRegister(GXMSG_USBBT_SCAN_FINISH,           0);
	GxBus_MessageRegister(GXMSG_USBBT_LINK_KEY_NOTIFY,       sizeof(GxMsgProperty_UsbBtLinkKey));
	GxBus_MessageRegister(GXMSG_USBBT_SINK_CONNECT,          0);
	GxBus_MessageRegister(GXMSG_USBBT_SINK_CONNECT_FAILED,   0);
	GxBus_MessageRegister(GXMSG_USBBT_SINK_DISCONNECT,       0);
	GxBus_MessageRegister(GXMSG_USBBT_SINK_PLAY,             0);
	GxBus_MessageRegister(GXMSG_USBBT_SINK_PAUSE,            0);
	GxBus_MessageRegister(GXMSG_USBBT_SINK_PLAYING_TITLE_INFO,  sizeof(GxMsgProperty_UsbBtPlayingInfo));
	GxBus_MessageRegister(GXMSG_USBBT_SINK_PLAYING_ARTIST_INFO, sizeof(GxMsgProperty_UsbBtPlayingInfo));
	GxBus_MessageRegister(GXMSG_USBBT_SINK_PLAYING_ALBUM_INFO,  sizeof(GxMsgProperty_UsbBtPlayingInfo));

	GxBus_MessageRegister(GXMSG_USBBT_SOURCE_CONNECT,        0);
	GxBus_MessageRegister(GXMSG_USBBT_SOURCE_CONNECT_FAILED, 0);
	GxBus_MessageRegister(GXMSG_USBBT_SOURCE_DISCONNECT,     0);
	GxBus_MessageRegister(GXMSG_USBBT_SOURCE_PLAY,           0);
	GxBus_MessageRegister(GXMSG_USBBT_SOURCE_PAUSE,          0);
	GxBus_MessageRegister(GXMSG_USBBT_SOURCE_STREAM_ESTABLISHED, 0);
	GxBus_MessageRegister(GXMSG_USBBT_SOURCE_STREAM_RELEASED,    0);
#endif

	sch = GxBus_SchedulerCreate("HotPlugConsoleScheduler", 1, 1024 * 8, GXOS_DEFAULT_PRIORITY -1 +priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

void GxHotPlugServiceDestroy(handle_t self)
{
	GxBus_MessageUnregister(GXMSG_HOTPLUG_DETECT);
	GxBus_MessageUnregister(GXMSG_HOTPLUG_IN_FAILED_NO_PARTITION);
	GxBus_MessageUnregister(GXMSG_HOTPLUG_IN_FAILED_MOUNT_FAILED);
	GxBus_MessageUnregister(GXMSG_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_HOTPLUG_OUT);
	GxBus_MessageUnregister(GXMSG_HOTPLUG_CLEAN);
#ifdef LINUX_OS
	GxBus_MessageUnregister(GXMSG_USB3G_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_USB3G_HOTPLUG_OUT);
	GxBus_MessageUnregister(GXMSG_USBWIFI_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_USBWIFI_HOTPLUG_OUT);
	GxBus_MessageUnregister(GXMSG_USBNET_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_USBNET_HOTPLUG_OUT);
	GxBus_MessageUnregister(GXMSG_USBHID_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_USBHID_HOTPLUG_OUT);
	GxBus_MessageUnregister(GXMSG_USBUVC_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_USBUVC_HOTPLUG_OUT);
	GxBus_MessageUnregister(GXMSG_USBAUDIO_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_USBAUDIO_HOTPLUG_OUT);

	// bt
	GxBus_MessageUnregister(GXMSG_USBBT_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_USBBT_HOTPLUG_OUT);
	GxBus_MessageUnregister(GXMSG_USBBT_INIT_SUCCESS);
	GxBus_MessageUnregister(GXMSG_USBBT_SCAN_RESULT);
	GxBus_MessageUnregister(GXMSG_USBBT_SCAN_FINISH);
	GxBus_MessageUnregister(GXMSG_USBBT_LINK_KEY_NOTIFY);
	GxBus_MessageUnregister(GXMSG_USBBT_SINK_CONNECT);
	GxBus_MessageUnregister(GXMSG_USBBT_SINK_CONNECT_FAILED);
	GxBus_MessageUnregister(GXMSG_USBBT_SINK_DISCONNECT);
	GxBus_MessageUnregister(GXMSG_USBBT_SINK_PLAY);
	GxBus_MessageUnregister(GXMSG_USBBT_SINK_PAUSE);
	GxBus_MessageUnregister(GXMSG_USBBT_SINK_PLAYING_TITLE_INFO);
	GxBus_MessageUnregister(GXMSG_USBBT_SINK_PLAYING_ARTIST_INFO);
	GxBus_MessageUnregister(GXMSG_USBBT_SINK_PLAYING_ALBUM_INFO);
	GxBus_MessageUnregister(GXMSG_USBBT_SOURCE_CONNECT);
	GxBus_MessageUnregister(GXMSG_USBBT_SOURCE_CONNECT_FAILED);
	GxBus_MessageUnregister(GXMSG_USBBT_SOURCE_DISCONNECT);
	GxBus_MessageUnregister(GXMSG_USBBT_SOURCE_PLAY);
	GxBus_MessageUnregister(GXMSG_USBBT_SOURCE_PAUSE);
	GxBus_MessageUnregister(GXMSG_USBBT_SOURCE_STREAM_ESTABLISHED);
	GxBus_MessageUnregister(GXMSG_USBBT_SOURCE_STREAM_RELEASED);
#endif

	GxBus_ServiceUnlink(self);
	GxCore_MutexDelete(hotplug_out_dev_mutex);
	HotplugOutDeviceDelAll();
}

void GxHotPlugServiceConsole(handle_t self)
{
	struct gxfs_hot_device* dev;
	int len;
	char key_value[10] = {0};

	GxBus_ConfigGet("prior_gui", key_value, 9, "false");
	if(0 == strcasecmp("false", key_value)) {
		static uint8_t first = 0;

		if(0 == first) {
			GxCore_ThreadDelay(5000);
			first = 1;
		}
	}

	dev = GxCore_HotplugWait();
	while(dev) {
		GxMessage *msg = NULL;
		int result;

		if (dev->action == PLUG_DETECT) {
			GxMsgProperty_HotPlug* plug = NULL;
			/*MSG out*/
			msg = GxBus_MessageNew(GXMSG_HOTPLUG_DETECT);
			if(NULL == msg)
				break;
			plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_HotPlug);
			if(NULL == plug)
				break;
			strncpy(plug->dev_name, dev->dev_name, HOTPLUG_DEV_NAME_LEN - 1);
			plug->port      = dev->port;
			plug->error     = dev->error;
			plug->removable = dev->removable;
			plug->index     = dev->id;
			GxBus_MessageSend(msg);
		} else if (dev->action == PLUG_IN) {

			result = -1;
			switch (dev->type) {
			case USB:
				if (dev->subtype != USB_STORAGE) {
					switch (dev->subtype) {
#ifdef LINUX_OS
					case USB_WIFI:
						{
							gxlogd("\n[USBWIFI HOTPLUG] PLUG_IN: %d\n", dev->id);

							GxMsgProperty_UsbwifiHotPlug* plug;
							msg = GxBus_MessageNew(GXMSG_USBWIFI_HOTPLUG_IN);
							if(NULL == msg)
								break;
							plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbwifiHotPlug);
							if(NULL == plug)
								break;
							len = strlen(dev->dev_name) > (WIFI_DEV_NAME_MAX_LEN - 1) ? (WIFI_DEV_NAME_MAX_LEN - 1) : strlen(dev->dev_name);
							strncpy(plug->dev_name, dev->dev_name, len);
							plug->id = dev->id;
							plug->error = dev->error;
							GxBus_MessageSend(msg);

							result = 0;
						}

						break;
					case USB_3G:
						{
							gxlogd("\n[USB3G HOTPLUG] PLUG_IN: %d\n", dev->id);

							GxMsgProperty_Usb3gHotPlug* plug;
							msg = GxBus_MessageNew(GXMSG_USB3G_HOTPLUG_IN);
							if(NULL == msg)
								break;
							plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_Usb3gHotPlug);
							if(NULL == plug)
								break;

							len = strlen(dev->dev_name) > (USB3G_DEV_NAME_MAX_LEN - 1) ? (USB3G_DEV_NAME_MAX_LEN - 1) : strlen(dev->dev_name);
							strncpy(plug->dev_name, dev->dev_name, len);
							plug->id = dev->id;
							plug->error = dev->error;
							GxBus_MessageSend(msg);

							result = 0;
						}
						break;
					case USB_ETH:
						{
							GxMsgProperty_UsbnetHotPlug* plug = NULL;

							gxlogd ("\n[USBNET HOTPLUG] PLUG_IN: %s\n", dev->dev_name);
							/*MSG out*/
							msg = GxBus_MessageNew(GXMSG_USBNET_HOTPLUG_IN);
							if(NULL == msg)
								break;
							plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbnetHotPlug);
							if(NULL == plug)
								break;

							len = strlen(dev->dev_name) > (USBETH_DEV_NAME_MAX_LEN - 1) ? (USBETH_DEV_NAME_MAX_LEN - 1) : strlen(dev->dev_name);
							strncpy(plug->dev_name, dev->dev_name, len);
							plug->id = dev->id;
							plug->error = dev->error;
							GxBus_MessageSend(msg);
						}
						break;
					case USB_BT:
						{
							GxMsgProperty_UsbBtHotPlug* plug = NULL;

							gxlogd ("\n[USBBT HOTPLUG] PLUG_IN: %s\n", dev->dev_name);

							if (!bt_cbk_ref)
								GxCore_BTRegisterCallback(event_callback);

							bt_cbk_ref++;

							/*MSG out*/
							msg = GxBus_MessageNew(GXMSG_USBBT_HOTPLUG_IN);
							if(NULL == msg)
								break;
							plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbBtHotPlug);
							if(NULL == plug)
								break;

							plug->id = dev->id;
							plug->error = dev->error;
							GxBus_MessageSend(msg);
						}
						break;
					case USB_HID:
						{
							GxMsgProperty_UsbHidHotPlug* plug = NULL;

							gxlogd ("\n[USBHID HOTPLUG] PLUG_IN: %s\n", dev->dev_name);
							/*MSG out*/
							msg = GxBus_MessageNew(GXMSG_USBHID_HOTPLUG_IN);
							if(NULL == msg)
								break;
							plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbHidHotPlug);
							if(NULL == plug)
								break;

							len = strlen(dev->dev_name) > (HID_DEV_NAME_MAX_LEN - 1) ? (HID_DEV_NAME_MAX_LEN - 1) : strlen(dev->dev_name);
							strncpy(plug->dev_name, dev->dev_name, len);
							plug->id = dev->id;
							plug->error = dev->error;
							GxBus_MessageSend(msg);
						}
						break;
					case USB_UVC:
						{
							GxMsgProperty_UsbUvcHotPlug* plug = NULL;

							gxlogd ("\n[USBUVC HOTPLUG] PLUG_IN: %s\n", dev->dev_name);
							/*MSG out*/
							msg = GxBus_MessageNew(GXMSG_USBUVC_HOTPLUG_IN);
							if(NULL == msg)
								break;
							plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbUvcHotPlug);
							if(NULL == plug)
								break;

							len = strlen(dev->dev_name) > (UVC_DEV_NAME_MAX_LEN - 1) ? (UVC_DEV_NAME_MAX_LEN - 1) : strlen(dev->dev_name);
							strncpy(plug->dev_name, dev->dev_name, len);
							plug->id = dev->id;
							plug->error = dev->error;
							GxBus_MessageSend(msg);
						}
						break;
					case USB_UAC:
						{
							GxMsgProperty_UsbaudioHotPlug* plug = NULL;

							gxlogd ("\n[USBUAC HOTPLUG] PLUG_IN: %s\n", dev->dev_name);
							/*MSG out*/
							msg = GxBus_MessageNew(GXMSG_USBAUDIO_HOTPLUG_IN);
							if(NULL == msg)
								break;
							plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbaudioHotPlug);
							if(NULL == plug)
								break;

							len = strlen(dev->dev_name) > (USBAUDIO_DEV_NAME_MAX_LEN - 1) ?
								  (USBAUDIO_DEV_NAME_MAX_LEN - 1) : strlen(dev->dev_name);
							strncpy(plug->dev_name, dev->dev_name, len);
							plug->id = dev->id;
							plug->error = dev->error;
							GxBus_MessageSend(msg);
						}
						break;

#endif
					default:
						break;
					}

					break; // switch (dev->type)
				}
				// USB_STORAGE fall through
			case SD:
			case CF:
				{
					GxMsgProperty_HotPlug* plug = NULL;

					if (dev->error) {
						if (dev->error == PLUG_ERROR_MOUNT_FAILED) {
							gxlogd("\n[HOTPLUG] PLUG_IN FAILED : mount failed\n");
							msg = GxBus_MessageNew(GXMSG_HOTPLUG_IN_FAILED_MOUNT_FAILED);
						} else if (dev->error == PLUG_ERROR_NO_PARTITION) {
							gxlogd("\n[HOTPLUG] PLUG_IN FAILED : no partition\n");
							msg = GxBus_MessageNew(GXMSG_HOTPLUG_IN_FAILED_NO_PARTITION);
						}

						if(NULL == msg)
							break;
						plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_HotPlug);
						if(NULL == plug)
							break;
						strncpy(plug->dev_name, dev->dev_name, HOTPLUG_DEV_NAME_LEN - 1);
						plug->port      = dev->port;
						plug->error     = dev->error;
						plug->removable = dev->removable;
						plug->index     = dev->id;
						GxBus_MessageSend(msg);
					} else {
						gxlogd("\n[HOTPLUG] PLUG_IN: %s, %d partition %s\n", dev->dev_name, dev->partition_count, dev->dev_path);

						/*Partition list create*/
						HotplugPartitionDestroy(dev);
						HotplugOutDeviceDel(dev->dev_name);
						HotplugPartitionCreate();

						/*MSG out*/
						msg = GxBus_MessageNew(GXMSG_HOTPLUG_IN);
						if(NULL == msg)
							break;
						plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_HotPlug);
						if(NULL == plug)
							break;
						strncpy(plug->dev_name, dev->dev_name, HOTPLUG_DEV_NAME_LEN - 1);
						plug->port      = dev->port;
						plug->error     = dev->error;
						plug->removable = dev->removable;
						plug->index     = dev->id;
						GxBus_MessageSend(msg);
					}
				}

				result = 0;
				break;
			default:
				break;
			}

			if (result != 0)
				break;
			/*Add diskinfo get*/
		//	HotplugPartitionInfo();

		}
		else if (dev->action == PLUG_OUT) {

			result = -1;
			switch (dev->type) {
			case USB:
				if (dev->subtype != USB_STORAGE) {
					switch (dev->subtype) {
#ifdef LINUX_OS
					case USB_WIFI:
						{
							gxlogd("\n[USBWIFI HOTPLUG] PLUG_OUT: %d\n", dev->id);

							GxMsgProperty_UsbwifiHotPlug* plug;
							msg = GxBus_MessageNew(GXMSG_USBWIFI_HOTPLUG_OUT);
							if(NULL == msg)
								break;
							plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbwifiHotPlug);
							if(NULL == plug)
								break;
							len = strlen(dev->dev_name) > (WIFI_DEV_NAME_MAX_LEN - 1) ? (WIFI_DEV_NAME_MAX_LEN - 1) : strlen(dev->dev_name);
							strncpy(plug->dev_name, dev->dev_name, len);
							plug->id = dev->id;
							plug->error = dev->error;
							GxBus_MessageSend(msg);

							result = 0;
						}
						break;
					case USB_3G:
						{
							gxlogd("\n[USB3G HOTPLUG] PLUG_OUT: %d\n", dev->id);

							GxMsgProperty_Usb3gHotPlug* plug;
							msg = GxBus_MessageNew(GXMSG_USB3G_HOTPLUG_OUT);
							if(NULL == msg)
								break;
							plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_Usb3gHotPlug);
							if(NULL == plug)
								break;

							len = strlen(dev->dev_name) > (USB3G_DEV_NAME_MAX_LEN - 1) ? (USB3G_DEV_NAME_MAX_LEN - 1) : strlen(dev->dev_name);
							strncpy(plug->dev_name, dev->dev_name, len);
							plug->id = dev->id;
							plug->error = dev->error;
							GxBus_MessageSend(msg);

							result = 0;
						}
						break;
					case USB_ETH:
						{
							GxMsgProperty_UsbnetHotPlug* plug = NULL;

							gxlogd ("\n[USBNET HOTPLUG] PLUG_OUT: %s\n", dev->dev_name);
							/*MSG out*/
							msg = GxBus_MessageNew(GXMSG_USBNET_HOTPLUG_OUT);
							if(NULL == msg)
								break;
							plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbnetHotPlug);
							if(NULL == plug)
								break;

							len = strlen(dev->dev_name) > (USBETH_DEV_NAME_MAX_LEN - 1) ? (USBETH_DEV_NAME_MAX_LEN - 1) : strlen(dev->dev_name);
							strncpy(plug->dev_name, dev->dev_name, len);
							plug->id = dev->id;
							plug->error = dev->error;
							GxBus_MessageSend(msg);
						}
						break;
					case USB_BT:
						{
							GxMsgProperty_UsbBtHotPlug* plug = NULL;

							gxlogd ("\n[USBBT HOTPLUG] PLUG_OUT: %s\n", dev->dev_name);

							--bt_cbk_ref;

							if (bt_cbk_ref == 0)
								GxCore_BTRegisterCallback(NULL);

							/*MSG out*/
							msg = GxBus_MessageNew(GXMSG_USBBT_HOTPLUG_OUT);
							if(NULL == msg)
								break;
							plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbBtHotPlug);
							if(NULL == plug)
								break;

							plug->id = dev->id;
							plug->error = dev->error;
							GxBus_MessageSend(msg);
						}
						break;
					case USB_HID:
						{
							GxMsgProperty_UsbHidHotPlug* plug = NULL;

							gxlogd ("\n[USBHID HOTPLUG] PLUG_OUT: %s\n", dev->dev_name);
							/*MSG out*/
							msg = GxBus_MessageNew(GXMSG_USBHID_HOTPLUG_OUT);
							if(NULL == msg)
								break;
							plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbHidHotPlug);
							if(NULL == plug)
								break;

							len = strlen(dev->dev_name) > (USBETH_DEV_NAME_MAX_LEN - 1) ? (USBETH_DEV_NAME_MAX_LEN - 1) : strlen(dev->dev_name);
							strncpy(plug->dev_name, dev->dev_name, len);
							plug->id = dev->id;
							plug->error = dev->error;
							GxBus_MessageSend(msg);
						}
						break;
					case USB_UVC:
						{
							GxMsgProperty_UsbUvcHotPlug* plug = NULL;

							gxlogd ("\n[USBUVC HOTPLUG] PLUG_OUT: %s\n", dev->dev_name);
							/*MSG out*/
							msg = GxBus_MessageNew(GXMSG_USBUVC_HOTPLUG_OUT);
							if(NULL == msg)
								break;
							plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbUvcHotPlug);
							if(NULL == plug)
								break;

							len = strlen(dev->dev_name) > (UVC_DEV_NAME_MAX_LEN - 1) ? (UVC_DEV_NAME_MAX_LEN - 1) : strlen(dev->dev_name);
							strncpy(plug->dev_name, dev->dev_name, len);
							plug->id = dev->id;
							plug->error = dev->error;
							GxBus_MessageSend(msg);
						}
						break;
					case USB_UAC:
						{
							GxMsgProperty_UsbaudioHotPlug* plug = NULL;

							gxlogd ("\n[USBUAC HOTPLUG] PLUG_OUT: %s\n", dev->dev_name);
							/*MSG out*/
							msg = GxBus_MessageNew(GXMSG_USBAUDIO_HOTPLUG_OUT);
							if(NULL == msg)
								break;
							plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbaudioHotPlug);
							if(NULL == plug)
								break;

							len = strlen(dev->dev_name) > (USBAUDIO_DEV_NAME_MAX_LEN - 1) ?
								  (USBAUDIO_DEV_NAME_MAX_LEN - 1) : strlen(dev->dev_name);
							strncpy(plug->dev_name, dev->dev_name, len);
							plug->id = dev->id;
							plug->error = dev->error;
							GxBus_MessageSend(msg);
						}
						break;
#endif
					default:
						break;
					}

					break; // switch (dev->type)
				}
				// USB_STORAGE fall through
			case SD:
			case CF:
				{
					GxMsgProperty_HotPlug* plug = NULL;

					gxlogd("\n[HOTPLUG] PLUG_OUT: %s, %d partition %s\n", dev->dev_name, dev->partition_count, dev->dev_path);

					/*Partition list create*/
					HotplugPartitionDestroy(dev);
					HotplugOutDeviceAdd(dev->dev_name);
					HotplugPartitionCreate();

					/*MSG out*/
					msg = GxBus_MessageNew(GXMSG_HOTPLUG_OUT);
					if(NULL == msg)
						break;
					plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_HotPlug);
					if(NULL == plug)
						break;
					strncpy(plug->dev_name, dev->dev_name, HOTPLUG_DEV_NAME_LEN - 1);
					plug->port      = dev->port;
					plug->error     = dev->error;
					plug->removable = dev->removable;
					plug->index     = dev->id;
					GxBus_MessageSend(msg);
				}

				result = 0;
				break;
			default:
				break;
			}

			if (result != 0)
				break;
		}
		else if (dev->action == PLUG_CLEAN)
		{
			GxMsgProperty_HotPlug* plug = NULL;

			gxlogd("\n[HOTPLUG] PLUG_CLEAN: %s, %d partition\n", dev->dev_name, dev->partition_count);

			/*Partition list create*/
			HotplugPartitionDestroy(dev);
			GxCore_MutexLock(hotplug_out_dev_mutex);
			HotplugOutDeviceDel(dev->dev_name);
			GxCore_MutexUnlock(hotplug_out_dev_mutex);
			HotplugPartitionCreate();
			/*MSG clean*/
			msg = GxBus_MessageNew(GXMSG_HOTPLUG_CLEAN);
			if(NULL == msg)
				break;
			plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_HotPlug);
			if(NULL == plug)
				break;
			strncpy(plug->dev_name, dev->dev_name, HOTPLUG_DEV_NAME_LEN - 1);
			plug->error = dev->error;
			GxBus_MessageSend(msg);
		}

		dev = GxCore_HotplugGetNext(dev);
	}
	GxCore_HotplugClean();
}

GxServiceClass hotplug_service = {
	"hotplug service",
	GxHotPlugServiceInit,
	GxHotPlugServiceDestroy,
	NULL,
	GxHotPlugServiceConsole,
	.priority_offset = 0,
};
#ifdef LINUX_OS
GxServiceClass net_status_service = {
	"wifi status service",
	GxNetStatusServiceInit,
	GxNetStatusServiceDestroy,
	NULL,
	GxNetStatusServiceConsole,
	.priority_offset = 0,
};
#endif

