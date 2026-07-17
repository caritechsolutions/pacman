#ifndef __APP_SKIN_FLASH_H_202310121045__
#define __APP_SKIN_FLASH_H_202310121045__

#include "app_skin_xml.h"


typedef enum {
    GXSKIN_FLASH_PARTION_INIT_ERROR        = -1,
	GXSKIN_FLASH_PARTION_GET_NAME_ERROR    = -2,
	GXSKIN_FLASH_ERASE_ERROR               = -3,
	GXSKIN_FLASH_WIRTE_ERROR               = -4,
	GXSKIN_FLASH_READ_ERROR                = -5,
	GXSKIN_FLASH_ADDR_ALIGNMENT_ERROR      = -6,
	GXSKIN_IMAGE_SRC_FILE_MALLOC_ERROR     = -7,
	GXSKIN_IMAGE_DRC_FILE_MALLOC_ERROR     = -8,
	GXSKIN_IMAGE_SRC_FILE_MD5_PARA_ERROR   = -9,  //以下错误有字符串对应
	GXSKIN_IMAGE_DRC_FILE_MD5_PARA_ERROR   = -10, //image desc md5
	GXSKIN_IMAGE_FILE_OPEN_ERROR           = -11,
	GXSKIN_IMAGE_FILE_LEN_ERROR            = -12,
	GXSKIN_IMAGE_FILE_READ_ERROR           = -13,
	GXSKIN_IMAGE_FILE_CHECK_MD5_ERROR      = -14,
	GXSKIN_IMAGE_DESC_FILE_MD5_CHECK_ERROR = -15,
}GxSKINFlashUpdate_Erroe_code;

char* gxapp_skin_flash_get_err_tips(int ret);

int  gxapp_skin_flash_set_update_mode(int mode);
int  gxapp_skin_flash_get_update_mode(void);
int  gxapp_skin_flash_set_update_imgsel(int index);
int  gxapp_skin_flash_get_update_imgsel(void);

int  gxapp_skin_flash_set_update_error_flag(int theme_id);
int  gxapp_skin_flash_get_update_error_flag(void);
int  gxapp_skin_flash_clear_update_error_flag(void);

int  gxapp_skin_flash_recovery_info(void);

int  gxapp_skin_flash_update_partition(int theme_id,char *file_name,char *imd5,char *dmd5);

int  gxapp_skin_flash_update_all(char *file_name);
int  gxapp_skin_flash_update_all_ext(char *buffer,unsigned int size);

int  gxapp_skin_flash_partition_init(void);
int  gxapp_skin_flash_mount(char *parition_name,char *mount_path);
int  gxapp_skin_flash_unmount(char *mount_path);
int  gxapp_skin_flash_other_mount(int parition_id);

int  gxapp_skin_flash_get_data(skin_data_node_t *pdata);

int  gxapp_skin_flash_get_status(int theme_id);
int  gxapp_skin_flash_get_active_partition_id(void);

int  gxapp_skin_flash_set_status(int theme_id, int installed);
int  gxapp_skin_flash_set_active_partition_id(int theme_id);

int  gxapp_skin_flash_get_recovry_status(void);

#endif
