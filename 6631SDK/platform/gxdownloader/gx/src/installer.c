#ifdef NO_OS
#include "gxloader.h"
#endif
#include "gxota_msg.h"
#include <string.h>
#include "common.h"
#include "config.h"
#include "gxdlp_api.h"
#include "ini.h"
#include "devapi/gxflash_api.h"
#include "common/gx_crc32.h"
#ifdef LINUX_OS
#include <sys/mount.h>
#include <mntent.h>
#endif
#include "upgrade.h"
#include "installer.h"
#include "progress.h"
#include "parser.h"

struct write_back_data {
	char partition_name[FLASH_PARTITION_NAME_SIZE + 1];
	int  data_len;
	uint8_t *data;
};

static unsigned int totalSize = 0;
static unsigned int writeSize = 0;
#ifndef NO_OS
static int rootfs_is_umount = 0;
#endif

static struct partition flash_table = {
	.magic = 0,
	.count = 0,
};

static char partition_status[FLASH_PARTITION_MAX_NUM+1] = {0};

static int check_data(unsigned char *src, unsigned int size, struct partition_info *partition)
{
	int         ret = 0,ssize=0;
	unsigned char *fdata = NULL;
	int read_real_size = 0;
	unsigned int flash_block_size = 0;

	GxCore_PartitionGetFlashBlockSize(&flash_block_size);
	fdata = GxCore_Malloc(flash_block_size);
	if(fdata == NULL)
		return -1;

	while(ssize<size){
		read_real_size = ((size - ssize) >= flash_block_size ? flash_block_size : (size - ssize));
		if (partition == NULL) {
			ret = GxFlash_Read(ssize, read_real_size, fdata);
			if (ret == -1) {
				GxCore_Free(fdata);
				fdata = NULL;
				gxloge("%s,%d, oem write error!\n", __func__, __LINE__);
				return -1;
			}
		} else {
			ret = GxCore_PartitionRead(partition, (void *)fdata, read_real_size, ssize);
			if (ret == 0) {
				GxCore_Free(fdata);
				fdata = NULL;
				gxloge("%s,%d, oem write error!\n", __func__, __LINE__);
				return -1;
			}
		}
		ret = memcmp(fdata,src+ssize, read_real_size);
		if(ret == 0)
			ssize += read_real_size;
		else {
			gxloge("%s,%d, data check error!\n", __func__, __LINE__);
			GxCore_Free(fdata);
			fdata = NULL;
			return -1;
		}
	}

	GxCore_Free(fdata);
	fdata = NULL;
	return ret;
}

static int write_data(uint8_t *buf, int size, struct partition_info *partition)
{
	int         ret,ssize=0;
	int write_real_size = 0;
	int percent = 0;
	unsigned int flash_block_size = 0;

	GxCore_PartitionGetFlashBlockSize(&flash_block_size);
	if (partition != NULL) {
		if (GxCore_PartitionErase(partition) == 0) {
			gxloge("partition %s erase error! %s,%d\n", partition->name, __func__, __LINE__);
			return -1;
		}
	}

	while (ssize < size) {
		//gxloge("======%s,%d, %d %d %d %d %d\n", __func__, __LINE__,ssize,size,writeSize,totalSize,percent);
		write_real_size = ((size - ssize) >= flash_block_size ? flash_block_size : (size - ssize));
		if (partition == NULL) {
			GxFlash_Erase(ssize, write_real_size);
			ret = GxFlash_Write(ssize, flash_block_size, buf+ssize);
			if (ret == -1) {
				gxloge("%s,%d, oem write error!\n", __func__, __LINE__);
				return -1;
			}
		} else {
			ret = GxCore_PartitionWriteRaw(partition, (void *)(buf+ssize), write_real_size, ssize);
			if (ret == 0) {
				gxloge("%s,%d, oem write error!\n", __func__, __LINE__);
				return -1;
			}
		}
		ssize += write_real_size;
		if (totalSize > 0)
			percent = 50 + ((99- 50)*(writeSize+ssize)/totalSize);
		else
			percent = 99;
		update_progress_percent(percent);

#ifndef NO_OS
		GxCore_ThreadDelay(10);
#endif
		//gxloge("++++++%s,%d, %d %d %d %d %d\n", __func__, __LINE__,ssize,size,writeSize,totalSize,percent);
	}

	writeSize += size;

#ifndef NO_OS
	if (totalSize > 0)
		percent = 50 + ((100 - 50)*(writeSize)/totalSize);
	else
		percent = 100;
	update_progress_percent(percent);
#endif

	return 0;
}

int check_partition_status(int show_error)
{
	int32_t i = 0;
	char *p_save_partition_status = GxDLP_Get_PartitionStatus();

	memset(partition_status, PARTITION_NORMAL_STATUS, FLASH_PARTITION_MAX_NUM);

	memcpy(partition_status, p_save_partition_status, (strlen(p_save_partition_status) > FLASH_PARTITION_MAX_NUM ? FLASH_PARTITION_MAX_NUM : strlen(p_save_partition_status)));

	if (flash_table.count == 0) {
		struct partition *ptable = NULL;

		ptable = GxCore_PartitionFlashInit();
		if(ptable == NULL) {
			gxloge("[Downloader] GxOem_PartitionTableGet  old Error!\n");
			return 0;
		}
		memcpy(&flash_table, ptable, sizeof(struct partition));
	}

	for (i = 0; i < flash_table.count; i++) {
		if (partition_status[i] == PARTITION_UPDATE_STATUS) {
			if (show_error == 1) {
				if (i == 0) {
					gxloge("All bin error, pls contact vendor\n");
				} else {
					gxloge("Partition %s error, pls contact vendor", flash_table.tables[i].name);
				}
				update_progress_error(GXUPDATE_PARTITION_ERROR);
				while(1);
			}
			break;
		}
	}

	if (i == flash_table.count)
		return FLASH_PARTITION_MAX_NUM;

	return i;
}

#ifdef LINUX_OS
#define MTD_DEVICE_NAME_SIZE 20
#define MTD_DEVICE_NAME "/dev/mtdblock"
static int umount_fs(struct upgrade_data *udata, int num)
{
	int i = 0;
	char device_name[MTD_DEVICE_NAME_SIZE] = {0};
	struct partition_info *partition = NULL;
	char command[100] = {0};
	int need_umount_rootfs = 0;
	struct mntent *mnt;
	FILE *fp = NULL;
	int ret = 0;

	fp = setmntent("/proc/mounts", "r");
	if (fp == NULL) {
		gxloge("setmntent failed");
		return -1;
	}

	for (i = 0; i < num; i++) {
		if (0 == strcasecmp(udata[i].name, ROOT_PARTITION_NAME)
				|| 0 == strcasecmp(udata[i].name, ROOTFS_PARTITION_NAME)
				|| true == is_upgrade_all_bin()) {
			need_umount_rootfs = 1;
			break;
		}
	}
	
	/* 卸载根文件系统上面所有挂载的Flash文件系统，最后再卸载根文件系统 */
	if (need_umount_rootfs) {
		/* cat /proc/mounts 中记载的根文件系统挂载在 /dev/root, 其他都挂
		 * 载在 /dev/mtdblock* 上，卸载时根据/dev/mtdblock*找到挂载路径
		 * 即可 */
		while ((mnt = getmntent(fp)) != NULL) {
			gxlogd("mnt_dir %s\n", mnt->mnt_dir);
			if (strncasecmp(mnt->mnt_fsname, MTD_DEVICE_NAME, strlen(MTD_DEVICE_NAME)) == 0) {
				if (strcmp(mnt->mnt_dir, "/dvb") == 0) //dvb目录下有升级进度条图片资源，无法卸载
					continue;
				gxlogd("mnt_fsname in flash %s\n", mnt->mnt_fsname);
				gxlogd("mnt_dir in flash %s\n", mnt->mnt_dir);
				if (umount2(mnt->mnt_dir, MNT_DETACH) != 0) {
					gxloge("umount %s error\n", mnt->mnt_dir);
					perror("");
					ret = -1;
					goto umount_fs_error;
				}
			}
		}
	} else {
		for (i = 0; i < num; i++) {
			if (0 != strcasecmp(udata[i].name, ROOT_PARTITION_NAME)
					&& 0 != strcasecmp(udata[i].name, ROOTFS_PARTITION_NAME)
					&& 0 != strcasecmp(udata[i].name, "APP")) { //dvb目录下有升级进度条图片资源，无法卸载
				partition = GxCore_PartitionGetByName(&flash_table, udata[i].name);
				if (partition->file_system_type != RAW
						&& partition->file_system_type != HIDE) {
					memset(device_name, 0x00, sizeof(device_name));
					sprintf(device_name, "/dev/mtdblock%d", partition->mtd_id);
					sprintf(command, "umount -l %s", device_name);
					while ((mnt = getmntent(fp)) != NULL) {
						if (strcasecmp(device_name, mnt->mnt_fsname) == 0) {
							gxlogd("mnt_dir in flash %s\n", mnt->mnt_dir);
							if (umount2(mnt->mnt_dir, MNT_DETACH) != 0) {
								gxloge("umount %s error\n", mnt->mnt_dir);
								perror("");
								ret = -1;
								goto umount_fs_error;
							}
						}
					}
					fseek(fp, 0, SEEK_SET);
				}
			}
		}
	}

	/* Linux rootfs 无法卸载，只能切rootfs */
	if (need_umount_rootfs == 1) {
		/* FIXME 由于进度条图片资源在root,卸载掉root,会出现进度条显示不正常, 只能升级完成后再卸载，后续BU1解决图片资源问题，再打开 */
#if 0
		if (system("/sbin/switch_ramfs.sh") != 0) {
			gxloge("switch ramfs error\n");
			ret = -1;
			goto umount_fs_error;
		}
#endif
		rootfs_is_umount = 1;
	}

umount_fs_error:
	endmntent(fp);
	return ret;
}

/* Linux mtd 设备无法卸载,当升级整bin后，有可能分区和之前的不一样时，目前升级整bin统一不做重新挂载的操作 */
/* linux 中执行该函数表示已经在 ramfs */
static int mount_fs(struct upgrade_data *udata, int num)
{
	struct partition_info *partition = NULL;
	char device_name[MTD_DEVICE_NAME_SIZE] = {0};
	/* 由于当前主进程在原来的rootfs中，所以rootfs无法被卸载也无法被重新挂载 */
#define RAMFS_MOUNT_ROOTFS_PATH  "/mnt"  //重新把Flash中的 rootfs 挂载到当前ramfs的挂载点

	if (rootfs_is_umount) {
		/* FIXME 升级了ROOT或者整bin，卸载掉之前的ROOT，再挂新的ROOT 理
		 * 论应该是升级前卸载，升级后挂载，这里把卸载和挂载做到一起的原
		 * 因在于进度条图片资源在root,卸载掉root,会出现进度条显示不正常,后续BU1解决图片资源问题，再去掉这里的卸载操作 */
		if (system("/sbin/switch_ramfs.sh") != 0) {
			gxloge("switch ramfs error\n");
			return -1;
		}
#if 0
		partition = GxCore_PartitionGetByName(&flash_table, ROOTFS_PARTITION_NAME);
		if (partition == NULL)
			partition = GxCore_PartitionGetByName(&flash_table, ROOT_PARTITION_NAME);
		if (partition)
		{
			memset(device_name, 0x00, sizeof(device_name));
			sprintf(device_name, "/dev/mtdblock%d", partition->mtd_id);
			gxlogd("mount rootfs device_name %s\n", device_name);
			if (mount(device_name, RAMFS_MOUNT_ROOTFS_PATH, GxCore_PartitionGetType(partition), 0, NULL) != 0) {
				gxloge("mount rootfs failed\n");
				perror("");
				return -1;
			}

			gxloge("ls /mnt\n");
			system("ls /mnt");

			if (pivot_root(RAMFS_MOUNT_ROOTFS_PATH, RAMFS_MOUNT_ROOTFS_PATH"/mnt") != 0) {
				gxloge("switch rootfs failed\n");
				perror("");
				return -1;
			}

			gxloge("ls\n");
			system("ls");

			gxloge("ls /dev\n");
			system("ls /dev");
			if (mount(RAMFS_MOUNT_ROOTFS_PATH"/dev", "/dev", NULL, 0, NULL) != 0) {
				gxloge("mount /devdev failed\n");
				perror("");
				return -1;
			}
			gxloge("ls /dev\n");
			system("ls /dev");
			gxloge("ls /home/gx\n");
			system("ls /home/gx");

		}
#endif
	}
	partition = NULL;
	partition = GxCore_PartitionGetByName(&flash_table, DATA_PARTITION_NAME);
	if (partition)
	{
		memset(device_name, 0x00, sizeof(device_name));
		sprintf(device_name, "/dev/mtdblock%d", partition->mtd_id);
		gxlogd("mount DATA device_name %s\n", device_name);
		if (system("mkdir -p /home/gx") != 0) {
			gxloge("mkdir /home/gx at ramfs error\n");
			return -1;
		}
		if (mount(device_name, "/home/gx", GxCore_PartitionGetType(partition), 0, NULL) != 0) {
			gxloge("mount DATA failed\n");
			return -1;
		}
	} else {
		gxloge("can't find DATA partition\n");
		return -1;
	}
	
	return 0;
}
#endif
#ifdef ECOS_OS
/* Structure describing a mount table entry.  */
struct mntent {
	char *mnt_fsname;	/* Device or server for filesystem.  */
	char *mnt_dir;		/* Directory mounted on.             */
	char *mnt_type;		/* Type of filesystem: ufs, nfs, etc.*/
	char *mnt_opts;		/* Comma-separated options for fs.   */
	int mnt_freq;		/* Dump frequency (in days).         */
	int mnt_passno;		/* Pass number for `fsck'.           */
};

FILE *setmntent(const char *name, const char *mode);
int endmntent(FILE * filep);
struct mntent *getmntent(FILE * filep);
struct mntent *getmntent_r(FILE *filep, struct mntent *mnt, char *buff, int bufsize);
extern int mount(const char *devname,
		const char *dir,
		const char *fsname,
		unsigned long mountflags,
		const void *data);
extern int umount(const char *name);
static int umount_fs(struct upgrade_data *udata, int num)
{
#define DEVICE_NAME_SIZE 20
#define MAX_FS_NAME_SIZE 50
	int i = 0;
	int j = 0;
	struct partition_info *partition = NULL;
	struct mntent mnt;
	const char *fstabname = "/etc/fstab";
	FILE *fstab = NULL;
	GxFileInfo info;
	char *getmntent_buf = NULL;
	char buff[MAX_FS_NAME_SIZE + 1];
	char *buff1 = NULL;
	int ret = 0;
	char *p[5];
	int id = 0;
	int need_umount_rootfs = 0;

	GxCore_GetFileInfo(fstabname, &info);
	if (info.type == GX_FILE_NONE) {
		gxloge("The system has not mounted any flash file system\n", fstabname);
		return 0;
	}

	getmntent_buf = (char *)GxCore_Malloc(info.size_by_bytes + 1);
	if (NULL == getmntent_buf) {
		gxloge("error : alloc getmntent_buf failed\n");
		return -1;
	}

	fstab = setmntent(fstabname, "r");
	if (fstab == NULL) {
		GxCore_Free(getmntent_buf);
		gxloge("setmntent failed %s\n", fstabname);
		ret = -1;
		goto umount_fs_error;
	}

	for (i = 0; i < num; i++) {
		if (0 == strcasecmp(udata[i].name, ROOT_PARTITION_NAME)
				|| 0 == strcasecmp(udata[i].name, ROOTFS_PARTITION_NAME)
				|| true == is_upgrade_all_bin()) {
			need_umount_rootfs = 1;
			break;
		}
	}

	/* 卸载根文件系统上面所有挂载的文件系统，最后再卸载根文件系统 */
	if (need_umount_rootfs) {
		/* ecos的/etc/fstab文件不会描述rootfs, 只描述其他文件系统 */
		while (getmntent_r(fstab, &mnt, getmntent_buf, info.size_by_bytes + 1)) {
			gxlogd("mnt_dir %s\n", mnt.mnt_dir);
			if (strcasecmp(mnt.mnt_fsname, "NONE") != 0) {
				gxlogd("mnt_dir in flash %s\n", mnt.mnt_dir);
				if (umount(mnt.mnt_dir) != 0) {
					gxloge("umount %s error\n", mnt.mnt_dir);
					ret = -1;
					goto umount_fs_error;
				}
			}
		}
	} else {
		for (i = 0; i < num; i++) {
			if (0 != strcasecmp(udata[i].name, ROOT_PARTITION_NAME)
					&& 0 != strcasecmp(udata[i].name, ROOTFS_PARTITION_NAME)) {
				partition = GxCore_PartitionGetByName(&flash_table, udata[i].name);
				if (partition->file_system_type != RAW
						&& partition->file_system_type != HIDE) {
					/* this code copy from api/os/ecos/app_start.cpp:mount_main */
					while (getmntent_r(fstab, &mnt, getmntent_buf, info.size_by_bytes + 1)) {
						gxlogd("mnt_dir %s\n", mnt.mnt_dir);
						if (strcasecmp(mnt.mnt_fsname, "NONE") != 0) {
							gxlogd("mnt_dir in flash %s\n", mnt.mnt_dir);
							strncpy(buff, mnt.mnt_fsname, MAX_FS_NAME_SIZE);
							buff1 = buff;
							j = 0;
							while(j < 5 && (p[j] = strtok(buff1, " /"))!= NULL) {
								j++;
								buff1 = NULL;
							}
							if (p[0] != NULL && p[1] != NULL && p[2] != NULL && p[3] != NULL)
								id = atoi(p[3]);
							else
								continue;
							if (id == partition->id) {
								if (umount(mnt.mnt_dir) != 0) {
									gxloge("umount %s error\n", mnt.mnt_dir);
									ret = -1;
									goto umount_fs_error;
								}
							}
						}
					}
					fseek(fstab, 0, SEEK_SET);
				}
			}
		}
	}

	if (need_umount_rootfs) {
		/* FIXME 由于进度条图片资源在root,卸载掉root,会出现进度条显示不正常, 只能升级完成后再卸载，后续BU1解决图片资源问题，再打开 */
#if 0
		if (umount("/") != 0) {
			gxloge("umount root error\n");
			ret = -1;
			goto umount_fs_error;
		}
#endif
		rootfs_is_umount = 1;
	}

umount_fs_error:
	if(getmntent_buf != NULL)
		GxCore_Free(getmntent_buf);
	endmntent(fstab);
	return ret;
}

static int mount_fs(struct upgrade_data *udata, int num)
{
	struct partition_info *partition = NULL;
	char *fs_type = NULL;
	char dir_cur[64] = {0};

	if (rootfs_is_umount) {
		/* FIXME 升级了ROOT或者整bin，卸载掉之前的ROOT，再挂新的ROOT 理
		 * 论应该是升级前卸载，升级后挂载，这里把卸载和挂载做到一起的原
		 * 因在于进度条图片资源在root,卸载掉root,会出现进度条显示不正常,后续BU1解决图片资源问题，再去掉这里的卸载操作 */
		if (umount("/") != 0) {
			gxloge("umount root error\n");
			return -1;
		}
		/* this is code copy from solution/app/app_low_power.c:app_remount_rootfs */
		partition = GxCore_PartitionGetByName(&flash_table, ROOT_PARTITION_NAME);
		if (partition == NULL)
			partition = GxCore_PartitionGetByName(&flash_table, ROOTFS_PARTITION_NAME);
		if (partition)
		{
			if (partition->file_system_type < MINIFS)
			{
				fs_type = GxCore_PartitionGetType(partition);
				memset(dir_cur, 0x00, sizeof(dir_cur));
				sprintf(dir_cur, "/dev/flash/0/0x%x,0x%x", partition->start_addr, partition->total_size);

				if (mount(dir_cur, "/", fs_type, 0, 0) != 0) {
					gxloge("mount ROOT failed\n");
					return -1;
				}
			}
		} else {
			gxloge("can't find ROOT or ROOTFS partition\n");
			return -1;
		}
	}
	partition = NULL;
	partition = GxCore_PartitionGetByName(&flash_table, DATA_PARTITION_NAME);
	if (partition)
	{
		fs_type = GxCore_PartitionGetType(partition);
		memset(dir_cur, 0x00, sizeof(dir_cur));
		sprintf(dir_cur, "/dev/flash/0/0x%x,0x%x", partition->start_addr, partition->total_size);
		if (mount(dir_cur, "/home/gx", fs_type, 0, 0) != 0) {
			gxloge("mount DATA failed\n");
			return -1;
		}
	} else {
		gxloge("can't find DATA partition\n");
		return -1;
	}
	return 0;
}

#endif
#ifndef NO_OS
static int copy_file(const char *pSrc, const char *pDest)
{
#define BUFFSIZE 1024
	FILE *fp1 = NULL; //指向源文件
	FILE *fp2 = NULL; //指向目的文件
	char *byBuff = NULL; //缓存区指针
	uint32_t fileBytes = 0; //文件大小

	if((pSrc == NULL) || (pDest == NULL))
	{
		return -1;
	}

	byBuff = (char *)GxCore_Malloc(BUFFSIZE);
	if(byBuff == NULL)
	{
		gxloge("malloc failed!!!!!!!!!\n");
		return -1;
	}

	if((fp1 = fopen(pSrc, "r")) != NULL)
	{
		if((fp2 = fopen(pDest, "w")) != NULL)
		{
			while((fileBytes = fread(byBuff, 1, BUFFSIZE, fp1)) > 0)
			{
				fwrite(byBuff, fileBytes, 1, fp2);
				memset(byBuff, 0, BUFFSIZE);
			}

			fclose(fp1);
			fclose(fp2);
		}
	}
	GxCore_Free(byBuff);
	return 0;
}
#endif

int update_data(struct upgrade_data *udata, int num)
{
	int i;
	unsigned int version = 0;
	char *version_str = NULL;
	char *p_save_partition_status = GxDLP_Get_PartitionStatus();
#ifndef NO_OS
	int dlp_close = 0;
	int oem_use_minifs = Ini_oem_use_minifs();
#endif
	struct partition *ptable = NULL;
	struct upgrade_data *oem_udata = NULL;
	struct upgrade_data all_udata = {0};
	enum GXPARTITION_FLASH_TYPE partition_flash_type = GXPARTITION_NORFLASH;
	int write_num = num;
	struct write_back_data *wb_data = NULL;
	int wb_count = 0;
	int ret = 0;

	GxFlash_Open();
	GxCore_PartitionGetFlashType(&partition_flash_type);

	memset(partition_status, PARTITION_NORMAL_STATUS, FLASH_PARTITION_MAX_NUM);

	memcpy(partition_status, p_save_partition_status, (strlen(p_save_partition_status) > FLASH_PARTITION_MAX_NUM ? FLASH_PARTITION_MAX_NUM : strlen(p_save_partition_status)));

	update_progress_status(GXUPDATE_INSTALL);

	totalSize = 0;
	writeSize = 0;

	wb_data = GxCore_Malloc(sizeof(struct write_back_data) * num);
	if (wb_data == NULL) {
		gxloge("malloc wb_data failed\n");
		ret = -1;
		goto install_exit;
	}
	memset(wb_data, 0x00, sizeof(struct write_back_data) * num);

	ptable = GxCore_PartitionFlashInit();
	if(ptable == NULL) {
		gxloge("[Downloader] get flash table failed!\n");
		ret = -1;
		goto install_exit;
	}
	memcpy(&flash_table, ptable, sizeof(struct partition));

	version = GxDLP_Get_UpdateSoftVersion();

	for (i = 0; i < num; i++)
		totalSize += udata[i].size;

	if (is_upgrade_all_bin() &&
	    partition_flash_type == GXPARTITION_NORFLASH) {
		all_udata.dst = 0x0;
		all_udata.src = udata[0].src;
		all_udata.size = totalSize;
		all_udata.partition_pos = 0;
		all_udata.partition = NULL;
		write_num = 1;
	}

#if defined(NO_OS) || defined(ECOS_OS)
	/* save need write back part */
	if (true == is_upgrade_all_bin()) {
		for (i = 0; i < num; i++) {
			if (udata[i].partition == NULL) {
				gxloge("udata partition must be not null \n");
				ret = -1;
				goto install_exit;
			}

			if (udata[i].write_back == true) {
				struct partition_info *write_back_part = NULL;
				write_back_part = GxCore_PartitionGetByName(ptable, udata[i].partition->name);
				if (write_back_part == NULL) {
					gxloge("can't find write back part\n");
					ret = -1;
					goto install_exit;
				}
				memcpy(wb_data[wb_count].partition_name, write_back_part->name, FLASH_PARTITION_NAME_SIZE);
				wb_data[wb_count].data_len = write_back_part->partition_size;
				wb_data[wb_count].data = GxCore_Malloc(wb_data[wb_count].data_len);
				if (wb_data[wb_count].data == NULL) {
					gxloge("malloc wb_data[%d].data is NULL\n", wb_count);
					ret = -1;
					goto install_exit;
				}
				if (GxCore_PartitionRead(write_back_part, wb_data[wb_count].data, wb_data[wb_count].data_len, 0) == 0) {
					gxloge("read write_back_part falied\n");
					ret = -1;
					goto install_exit;
				}

				totalSize += wb_data[wb_count].data_len;
				wb_count++;
			}
		}
	}
#endif

	GxCore_Partition_Unprotect();

#ifndef NO_OS
	if (oem_use_minifs) {
		if (true == is_upgrade_all_bin()) {
			GxDLP_Close();
			dlp_close = 1;
		} else {
			for (i = 0; i < num; i++) {
				if (0 == strcasecmp(udata[i].name, ROOT_PARTITION_NAME)
					|| 0 == strcasecmp(udata[i].name, ROOTFS_PARTITION_NAME)
					|| 0 == strcasecmp(udata[i].name, DATA_PARTITION_NAME)) {
					GxDLP_Close();
					dlp_close = 1;
					break;
				}
			}
		}
	}

	if (umount_fs(udata, num) == -1) {
		gxloge("umount_fs error \n");
		ret = -1;
		goto install_exit;
	}
#endif

	for (i = 0; i < write_num || oem_udata; i++) {
		struct upgrade_data *up_data = NULL;

		if (i < write_num) {
			up_data = &udata[i];
			if (is_upgrade_all_bin() &&
			    partition_flash_type == GXPARTITION_NORFLASH) {
				up_data = &all_udata;
			}
		}
		else {
			if (oem_udata == NULL)
				break;
			up_data = oem_udata;
			oem_udata = NULL;
		}

		gxlogd("[GXOTA] Pa0rt :| %s |Write Flash Addr=0x%08x, From Dram=%p,size:%d \n",
				up_data->name,up_data->dst, up_data->src, up_data->size);
		if (true != is_upgrade_all_bin()) {
			/* OEM partition must upgrade at last */
			if ((strncasecmp(up_data->partition->name, Ini_oem_name(), FLASH_PARTITION_NAME_SIZE) == 0 &&
			    i < write_num - 1)) {
				oem_udata = up_data;
				continue;
			}
			partition_status[up_data->partition_pos] = PARTITION_UPDATE_STATUS;
			GxDLP_Set_PartitionStatus(partition_status);
			GxDLP_Sync();
		}
		if (0 != write_data(up_data->src, up_data->size, up_data->partition)) {
#ifdef NO_OS
			update_progress_error(GXUPDATE_WRITE_FLASH_FAILED);
#else
			update_progress_error(GXUPDATE_WRITE_FLASH_FAILED);
			GxCore_ThreadDelay(100);
#endif
			ret = -1;
			goto install_exit;
		}

		if (0 != check_data(up_data->src, up_data->size, up_data->partition)) {
#ifdef NO_OS
			update_progress_error(GXUPDATE_WRITE_FLASH_FAILED);
#else
			update_progress_error(GXUPDATE_WRITE_FLASH_FAILED);
			GxCore_ThreadDelay(100);
#endif
		}

		if (true != is_upgrade_all_bin()) {
			if (strncasecmp(up_data->partition->name, Ini_oem_name(), FLASH_PARTITION_NAME_SIZE) == 0) {
				/* We should reinit DLP after OEM partition was
				 * upgraded */
				if (Ini_backoem_name() != NULL) {
					struct partition_info *backoem_part = NULL;
					backoem_part = GxCore_PartitionGetByName(&flash_table, Ini_backoem_name());
					/* OEM was upgraded, but data in BACKOEM
					 * is outdated than OEM, so erase data
					 * in BACKOEM, GxDLP_Init will copy data
					 * in OEM to BACKOEM
					 * */
					if (backoem_part != NULL) {
						if (GxCore_PartitionErase(backoem_part) == 0) {
							gxloge("partition %s erase error! %s,%d\n", backoem_part->name, __func__, __LINE__);
							ret = -1;
							goto install_exit;
						}
					}
				}
				GxDLP_Close();
				GxDLP_Init((char *)Ini_oem_name(), (char *)Ini_backoem_name());
				oem_udata = NULL;
			}
			partition_status[up_data->partition_pos] = PARTITION_NORMAL_STATUS;
			GxDLP_Set_PartitionStatus(partition_status);
			GxDLP_Sync();
		}
#ifndef NO_OS
		GxCore_ThreadDelay(100);
#endif

	}

#if defined(NO_OS) || defined(ECOS_OS)
	/* write back */
	if (true == is_upgrade_all_bin()) {
		ptable = get_new_partition();

		for (i = 0; i < wb_count; i++) {
			struct partition_info *write_back_part = NULL;
			write_back_part = GxCore_PartitionGetByName(ptable, wb_data[i].partition_name);
			if (write_back_part == NULL) {
				gxloge("can't find write back part\n");
				ret = -1;
				goto install_exit;
			}
			if (wb_data[i].data == NULL) {
				gxloge("wb_data[%d].data is NULL\n", i);
				ret = -1;
				goto install_exit;
			}
			if (0 != write_data(wb_data[i].data, wb_data[i].data_len, write_back_part)) {
#ifdef NO_OS
				update_progress_error(GXUPDATE_WRITE_FLASH_FAILED);
#else
				update_progress_error(GXUPDATE_WRITE_FLASH_FAILED);
				GxCore_ThreadDelay(100);
#endif
				ret = -1;
				goto install_exit;
			}
		}
	}
#endif

	update_progress_status(GXUPDATE_INSTALL_OK);

	gxlogd("GXOTA_STATUS_WRITE_FLASH_OK \n");

#ifdef LINUX_OS
	if (true != is_upgrade_all_bin()) {
		if (oem_use_minifs && dlp_close == 1) {
			mount_fs(udata, num);
			if (GxCore_FileExists(Ini_oem_name()) == GXCORE_FILE_UNEXIST)
			{
				copy_file(OEM_INI_ORIGIN_PATH, Ini_oem_name());
			}
			GxDLP_Init((char *)Ini_oem_name(), (char *)Ini_backoem_name());
		}
		if (check_partition_status(0) == FLASH_PARTITION_MAX_NUM) {
			GxDLP_Clear_UpdateType();
			GxDLP_Set_ForceUpgradeFlag("0");

			version_str = convert_version_num_to_char(version);
			GxDLP_Set_SoftwareVersion(version_str);
			GxDLP_Sync();
		}
	}
#elif defined(ECOS_OS)
	if (true == is_upgrade_all_bin()) {
		ptable = get_new_partition();
		memcpy(&flash_table, ptable, sizeof(struct partition));
	}

	if (oem_use_minifs && dlp_close == 1) {
		if (mount_fs(udata, num) == 0) {
			if (GxCore_FileExists(Ini_oem_name()) == GXCORE_FILE_UNEXIST)
			{
				copy_file(OEM_INI_ORIGIN_PATH, Ini_oem_name());
			}
			GxDLP_Init((char *)Ini_oem_name(), (char *)Ini_backoem_name());
		} else {
			gxloge("remount fs error and dlp init failed \n");
			ret = -1;
			goto install_exit;
		}
	} else {
		if (true == is_upgrade_all_bin()) {
			if (dlp_close == 0)
				GxDLP_Close();
			GxDLP_Init((char *)Ini_oem_name(), (char *)Ini_backoem_name());
		}
	}

	if (check_partition_status(0) == FLASH_PARTITION_MAX_NUM
		|| true == is_upgrade_all_bin()) {
		GxDLP_Clear_UpdateType();
		GxDLP_Set_ForceUpgradeFlag("0");

		version_str = convert_version_num_to_char(version);
		GxDLP_Set_SoftwareVersion(version_str);
		GxDLP_Sync();
	}
#else
	if (true == is_upgrade_all_bin()) {
		GxDLP_Close();
		GxDLP_Init((char *)Ini_oem_name(), (char *)Ini_backoem_name());
	}

	if (check_partition_status(0) == FLASH_PARTITION_MAX_NUM
		|| true == is_upgrade_all_bin()) {
		GxDLP_Clear_UpdateType();
		GxDLP_Set_ForceUpgradeFlag("0");

		version_str = convert_version_num_to_char(version);
		GxDLP_Set_SoftwareVersion(version_str);
		GxDLP_Sync();
	}
#endif
install_exit:
	if (wb_data != NULL) {
		for (i = 0; i < wb_count; i++)
			GxCore_Free(wb_data[i].data);
		GxCore_Free(wb_data);
	}

	return ret;
}

