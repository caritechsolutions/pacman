#ifndef __INSTALLER_H__
#define __INSTALLER_H__

#define PARTITION_NORMAL_STATUS '0'
#define PARTITION_UPDATE_STATUS '1'

#define ROOT_PARTITION_NAME "ROOT"
#define ROOTFS_PARTITION_NAME "ROOTFS"
#define DATA_PARTITION_NAME "DATA"
#define OEM_INI_ORIGIN_PATH       "/dvb/theme/variable_oem.ini"

int check_partition_status(int show_error);
int update_data(struct upgrade_data *udata, int num);
#endif
