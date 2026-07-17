/** \addtogroup <label> 
 *  *  @{
 *   */
#ifndef __GXOTA_MSG_H__
#define __GXOTA_MSG_H__

/*The OTA message list*/
#define GXOTA_STATUS_INIT                    (0)
#define GXOTA_STATUS_FRONTEND_ERROR          (1)
#define GXOTA_STATUS_FRONTEND_SETUP          (2)
#define GXOTA_STATUS_FRONTEND_LOCK           (3)
#define GXOTA_STATUS_DMX_ERROR               (4)
#define GXOTA_STATUS_DMX_START_FILTER        (5)
#define GXOTA_STATUS_DMX_DATA_FINISH         (6)
#define GXOTA_STATUS_WRITE_FLASH             (7)
#define GXOTA_STATUS_WRITE_FLASH_OK          (8)
#define GXOTA_STATUS_WRITE_FLASH_FAILED      (9)
#define GXOTA_STATUS_UPDATE_FAILED           (10)
#define GXOTA_STATUS_UPDATE_SUCCESS          (11)
#define GXOTA_STATUS_VERSION_ERROR           (12)
#define GXOTA_STATUS_VERSION_OK              (13)
#define GXOTA_STATUS_DMX_TIMEOUT             (14)
#define GXOTA_STATUS_NO_DATA_UPDATE          (15)

/**
 * OTA升级消息
 */
struct GxOTA_Msg
{
#define GXOTA_GUI_STATUS                    (1)
#define GXOTA_GUI_PROCESS                   (2)
	int            type;	///< 升级消息类型, 0表示传递的是升级状态，1表示传递的是升级进度百分比
	union
	{
		int    percent; ///< 升级进度百分比
		int    msg_id;  ///< 升级状态
	};
};

/**
 * OTA获取升级进度消息
 *\param msg 升级消息结构体地址, 结构体说明请参考gxdocref GxOTA_Msg
 *\return 返回函数执行是否成功
 *\retval 0 成功, msg有效
 *\retval -1 失败, msg无效
 */
extern int GxOTA_Get_Msg(struct GxOTA_Msg *msg);
#endif

/** @}*/
