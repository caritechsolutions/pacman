#ifndef __SIMPLE_DLP_H__
#define __SIMPLE_DLP_H__

#include "sys/types.h"

typedef enum {
	GXDLP_DOWNLOAD_SEQ_FIRST,      ///< 升级第一顺序类型
	GXDLP_DOWNLOAD_SEQ_SECOND,     ///< 升级第二顺序类型
	GXDLP_DOWNLOAD_SEQ_THIRD,      ///< 升级第三顺序类型
	GXDLP_DOWNLOAD_SEQ_MAX
} GxDLP_Download_Sequence;

typedef enum{
	GXDLP_NONE_DOWNLOAD     ,      ///< 不升级
	GXDLP_OTA_FAILED        ,      ///< 升级失败
	GXDLP_OTA_DDT_DOWNLOAD  ,      ///< DDT协议升级
	GXDLP_OTA_DDTEX_DOWNLOAD,      ///< DDTEX协议升级
	GXDLP_USB_DOWNLOAD      ,      ///< USB升级
	GXDLP_NET_TFTP_DOWNLOAD ,      ///< 网络TFTP升级
	GXDLP_NET_HTTP_DOWNLOAD ,      ///< 网络HTTP升级
	GXDLP_UART_DOWNLOAD     ,      ///< 串口升级
	GXDLP_RECOVERY_DOWNLOAD ,      ///< 进入Recovery模式，自由选择升级方式，包括USB,NET,DDTEX等

	GXDLP_DOWNLOADER_MAX
}GxDLP_Download_Type;

int32_t GxDLP_Init(char *dlp,char *backup_dlp);
int32_t GxDLP_Sync(void); //sync dlp and backup_dlp
int32_t GxDLP_Close(void);
int32_t GxDLP_Get_UpdateType(GxDLP_Download_Type *type);
int32_t GxDLP_Set_UpdateType(GxDLP_Download_Type type);
#endif

