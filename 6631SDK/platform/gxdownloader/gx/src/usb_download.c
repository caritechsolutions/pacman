#ifdef NO_OS
#include "fat.h"
#include "gxloader.h"
#include "usb.h"
#endif

#include "common.h"
#include "gui.h"
#include "gxdlp_api.h"
#include "upgrade.h"
#ifndef NO_OS
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#include "gxupdate_msg.h"
#include "progress.h"

#ifdef NO_OS
#define USB_MAX_PROBE_TIME 1375
static block_dev_desc_t *usb_dev=NULL;
#endif

#define USB_DISK_PATH "/mnt/usb01/"

static int scan_usb(void)
{
#ifdef NO_OS
	unsigned int  start_time;
	int ota_usb_stor_curr_dev = -1;

	/* start USB */
	if (usb_stop() < 0) {
		gxloge("usb_stop failed\n");
		return -1;
	}

	if (usb_init() < 0) {
		gxloge("usb_init failed\n");
		return -1;
	}

	/*
	 * check whether a storage device is attached (assume that it's
	 * a USB memory stick, since nothing else should be attached).
	 */
	start_time = get_timer(0);
	do {
		ota_usb_stor_curr_dev = usb_stor_scan(0);
		if (ota_usb_stor_curr_dev != -1)
			break;

		mdelay(100);
	} while (get_timer(start_time) < USB_MAX_PROBE_TIME);

	if (-1 == ota_usb_stor_curr_dev) {
		gxloge("usb_stor_scan timeout\n");
		return -1;
	}
	/* check whether it has a partition table */
	usb_dev = usb_stor_get_dev(0);
	if (usb_dev == NULL) {
		gxloge("uknown device type\n");
		return -1;
	}

	gxlogd("scan_usb success\n");
	return 0;
#else
	GxCore_ThreadDelay(5000); //wait usb hotplug detect
	return 0;
#endif

}

static int usb_read_file(char *filename, void *pbuf, unsigned int size, unsigned int* real_read_size)
{
#ifdef NO_OS
	unsigned int partition = 0;
	if (NULL == usb_dev)
		return -1;
	for(partition = 1; partition <= USB_MAX_PARTITON; partition++) {
		if (fat_register_device(usb_dev, partition) == 0) {
			if((file_fat_read(filename, pbuf, size)) != -1) {
				file_get_size(filename, real_read_size);
				return 0;
			}
		}
	}
	return 0;
#else
	int fd;
	char pathname[128] = {0};

	snprintf(pathname, 128 - 1, USB_DISK_PATH"%s", filename);
	fd = open(pathname, O_RDONLY);
	if (fd < 0) {
		printf("open failed\n");
		return -1;
	}
	*real_read_size = read(fd, pbuf, size);

	close(fd);

	return 0;
#endif
}

static int usb_get_file_size(char *filename, unsigned int *size)
{
#ifdef NO_OS
	char *usb_pbuf = NULL;

	usb_pbuf = (char*)malloc(1024);
	if (NULL == usb_pbuf) {
		gxloge("pbuf malloc failed!\n");
		return -1;
	}

	if (0 != usb_read_file(filename, usb_pbuf, 1024, size)) {
		gxloge("read_usb_file failed\n");
		return -1;
	}

	free(usb_pbuf);

	return 0;
#else
	int fd;
	struct stat stat;
	char pathname[128] = {0};

	snprintf(pathname, 128 - 1, USB_DISK_PATH"%s", filename);
	fd = open(pathname, O_RDONLY);
	if (fd < 0) {
		printf("open failed\n");
		return -1;
	}
	fstat(fd, &stat);
	close(fd);
	*size = stat.st_size;
	return 0;
#endif
}

int usb_get_dir(char* dir,void* pbuf,unsigned long maxsize,unsigned long* file_len)
{
#ifdef NO_OS
	unsigned int partition = 0;
	if (0 != scan_usb())
		return -1;
	if (NULL == usb_dev)
		return -1;
	for(partition = 1; partition <= USB_MAX_PARTITON; partition++) {
		if (fat_register_device(usb_dev, partition) == 0) {
			file_fat_read_dir(dir, pbuf, maxsize, file_len);
			return 0;
		}
	}
#endif
	return 0;
}

int usb_download_data(struct download_data *udata)
{
	char *UPDATE_FILE = NULL;
	char *usb_pbuf = NULL;
	unsigned int filesize = 0;
	unsigned int binlength = 0;

	if (0 != scan_usb())
		return -1;

	update_progress_percent(0);
	update_progress_status(GXUPDATE_DOWNLOAD_DATA);

	UPDATE_FILE = GxDLP_Get_UpgradeFileName();
	if(UPDATE_FILE == NULL)
		UPDATE_FILE = "update.bin";

	if (0 != usb_get_file_size(UPDATE_FILE, &binlength)) {
		gxloge("read_usb_file failed\n");
		goto err;
	}

	if (binlength < 64*1024) {
		gxloge("file_get_size binlength=%d failed!\n",binlength);
		goto err;
	}

	usb_pbuf = (char *)malloc(binlength);
	if (NULL == usb_pbuf) {
		gxloge("pbuf malloc failed!\n");
		goto err;
	}

	if (0 == usb_read_file(UPDATE_FILE, usb_pbuf, binlength, &filesize)) {
		update_progress_percent(49);
		gxloge("read_usb_file success\n");
	} else {
		gxloge("read_usb_file failed\n");
		update_progress_error(GXUPDATE_USB_GET_ERROR);
		goto err;
	}

	udata->data = (unsigned char*)usb_pbuf;
	udata->len = filesize;
#ifdef NO_OS
	usb_stop();
#endif
	update_progress_status(GXUPDATE_DOWNLOAD_DATA_OK);
	return 0;
err:
	free(usb_pbuf);
	udata->data = NULL;
	udata->len = 0;
#ifdef NO_OS
	usb_stop();
#endif
	return -1;
}
