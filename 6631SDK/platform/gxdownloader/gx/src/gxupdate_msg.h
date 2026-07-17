#ifndef __GXUPDATE_MSG_H__
#define __GXUPDATE_MSG_H__

#include "gxdlp_api.h"

typedef enum {
	GXUPDATE_MSG_STATUS = 1,    ///< 表示此次消息类型为更新状态类型
	GXUPDATE_MSG_PROCESS,       ///< 表示此次消息类型为更新升级进度百分比
} GXUPDATE_MSG_TYPE;

typedef enum {
	GXUPDATE_IDLE,              ///< 菜单升级不需要管
	GXUPDATE_START,             ///< 菜单升级不需要管
	GXUPDATE_CONFIG,            ///< 菜单升级不需要管
	GXUPDATE_CONFIG_OK,         ///< 菜单升级不需要管
	GXUPDATE_DOWNLOAD_DATA,     ///< 开始下载镜像
	GXUPDATE_DOWNLOAD_DATA_OK,  ///< 镜像下载完成
	GXUPDATE_PARSE,             ///< 开始解析镜像
	GXUPDATE_PARSE_OK,          ///< 解析镜像成功
	GXUPDATE_INSTALL,           ///< 开始把镜像写入Flash
	GXUPDATE_INSTALL_OK,        ///< 镜像写入 Flash 成功
	GXUPDATE_SUCCESS,           ///< 升级成功
	GXUPDATE_FAIL               ///< 升级失败
} GXUPDATE_STATUS;

typedef enum {
	GXUPDATE_NO_ERROR,                 ///< 没有错误
	/* OTA download data error */
	GXUPDATE_FRONTEND_ERROR,           ///< 前端接收消息
	GXUPDATE_FRONTEND_UNLOCK,          ///< 前端不锁定
	GXUPDATE_DMX_CONFIG_ERROR,         ///< demux 配置错误
	GXUPDATE_DMX_FILTER_TIMEOUT,       ///< demux 下载数据超时
	/* HTTP download data error */
	GXUPDATE_WIFI_CONNECT_ERROR,       ///< wifi 链接失败
	GXUPDATE_NETWORK_INIT_ERROR,       ///< 网络初始化失败
	GXUPDATE_HTTP_GET_ERROR,           ///< http get 失败
	/* USB downloader data error */
	GXUPDATE_USB_GET_ERROR,            ///< usb 下载失败
	/* parse upgrade image error */
	GXUPDATE_IMAGE_HEADER_ERROR,       ///< 升级镜像头错误
	GXUPDATE_IMAGE_VERSION_ERROR,      ///< 升级镜像版本错误
	GXUPDATE_IMAGE_CRC_ERROR,          ///< 升级镜像crc校验错误
	GXUPDATE_IMAGE_NO_DATA_ERROR,      ///< 升级镜像没有数据可以升级
	/*  install error */
	GXUPDATE_WRITE_FLASH_FAILED,       ///< 写 Flash 失败

	/* PARTITION error */
	GXUPDATE_PARTITION_ERROR           ///< 有分区是在某次升级该分区的时候断电，该分区数据有错
} GXUPDATE_ERROR_CODE;

typedef struct {
	GXUPDATE_MSG_TYPE type;               ///< 消息类型
	char percent;                         ///< 升级进度百分比
	GXUPDATE_STATUS status;               ///< 当前升级状态,当升级状态为GXUPDATE_FAIL,根据error_code判断错误类型
	GXUPDATE_ERROR_CODE error_code;       ///< 升级错误码
	GxDLP_Download_Type download_type;    ///< 升级类型
} GxUpdateMsg;

/**
 * 接收升级消息
 *\param msg
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int GxUpdate_ReceiveMsg(GxUpdateMsg *msg);

#endif
