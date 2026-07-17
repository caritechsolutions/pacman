#include "libini.h"
#include "simple_ini.h"
#include "sys/types.h"
#include "simple_gxdlp_api.h"
#include <string.h>
#include <stdio.h>

#define OEM_SYSTEM_SECTION                      ("system")
#define OEM_HARDWARE_VERSION                    ("hardware_version")
#define OEM_MANUFACTURE_ID                      ("manufacture_id")
#define OEM_PLATFORM_ID                         ("platform_Id")
#define OEM_SERIAL_NUMBER                       ("serial_number")
#define OEM_CHIP_ID                             ("chip_id")

#define OEM_SOFTWARE_SECTION                    ("software")
#define OEM_LANGUAGE                            ("language")
#define OEM_APP_VERSION                         ("application_version")
#define OEM_APP_UPDATEVERSION                   ("application_update_version")

#define OEM_UPDATE_SECTION                      ("update")
#define OEM_UPDATE_TYPE                         ("type")

static char *update_type[] = {"BOOT", "OTA_FAILED", "OTA_DDT", "OTA_DDTEX", "USB", "NET_TFTP", "NET_HTTP", "UART", "RECOVERY"};

int32_t GxDLP_Init(char *dlp,char *backup_dlp)
{
	return Ini_Oem_Init(dlp,backup_dlp);
}

int32_t GxDLP_Sync(void)
{
	return Ini_Sync();
}

int32_t GxDLP_Close(void)
{
	Ini_Oem_Close();
	return 0;
}

int32_t GxDLP_Get_UpdateTypeByIndex(GxDLP_Download_Type *type, GxDLP_Download_Sequence index)
{
	char* str;
	int32_t i = 0;
	int32_t j = 0;
	char type_list_buf[128] = {0};
	char *p_type_list_buf = type_list_buf;
	GxDLP_Download_Type type_list[GXDLP_DOWNLOAD_SEQ_MAX] = {GXDLP_NONE_DOWNLOAD};
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_UPDATE_TYPE);
	if (NULL == str || index > GXDLP_DOWNLOAD_SEQ_MAX)
		return -1;

	strncpy(type_list_buf, str, sizeof(type_list_buf) - 1);

	for (i = 0; i < GXDLP_DOWNLOAD_SEQ_MAX; i++) {
		str = strsep(&p_type_list_buf, ":");
		if (str != NULL) {
			for (j = 0; j < GXDLP_DOWNLOADER_MAX; j++) {
				if (0 == strcmp(str, update_type[j])) {
					type_list[i] = j;
					break;
				}
			}
		} else
			break;
	}

	*type = type_list[index];

	return 0;
}

int32_t GxDLP_Set_UpdateTypeByIndex(GxDLP_Download_Type type, GxDLP_Download_Sequence index)
{
	if (GXDLP_DOWNLOADER_MAX <= type)
		return -1;

	int32_t ret = 0;
	int32_t i = 0;
	char type_list_buf[128] = {0};
	char *p_type_list_buf = type_list_buf;
	GxDLP_Download_Type type_list[GXDLP_DOWNLOAD_SEQ_MAX] = {GXDLP_NONE_DOWNLOAD};

	if (index > GXDLP_DOWNLOAD_SEQ_MAX)
		return -1;

	for (i = 0; i < GXDLP_DOWNLOAD_SEQ_MAX; i++) {
		GxDLP_Get_UpdateTypeByIndex(&type_list[i], i);
		if (i == index)
			type_list[i] = type;
		p_type_list_buf += snprintf(p_type_list_buf, (sizeof(type_list_buf) - 1 - strlen(type_list_buf)), "%s:", update_type[type_list[i]]);
	}
	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_UPDATE_TYPE, type_list_buf);
	if(ret == -1)
		printf("Ini_SetValue Update type by index error\n");

	return ret;
}

int32_t GxDLP_Get_UpdateType(GxDLP_Download_Type *type)
{
	if (GxDLP_Get_UpdateTypeByIndex(type, GXDLP_DOWNLOAD_SEQ_FIRST) != 0)
		return -1;

	return 0;
}

int32_t GxDLP_Set_UpdateType(GxDLP_Download_Type type)
{
	if (GxDLP_Set_UpdateTypeByIndex(type, GXDLP_DOWNLOAD_SEQ_FIRST) != 0)
		return -1;

	return 0;
}
