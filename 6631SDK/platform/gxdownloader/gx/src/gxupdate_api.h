/** \addtogroup <label>
 *  *  @{
 *   */
#ifndef __GXUPDATE_API_H__
#define __GXUPDATE_API_H__

#include "gxdlp_api.h"
#include "gxota_api.h"

//typedef enum {
//	GXUPDATE_NORMAL_UPGRADE,  ///< 正常升级
//	GXUPDATE_SN_UPGRADE,      ///< 判断 SN 号升级，具体判断由客户自己实现
//} GXUPDATE_UPGRADE_MODE;

typedef GxOTAType GXUPDATE_UPGRADE_MODE;  ///< 升级模式

/**
 * WIFI 参数
 */
typedef struct {
	int  is_highestpri;               ///< 是否设置 WIFI 为最高优先级的网络设备 0:不是，按照默认优先级 1: 设置为最高优先级
	char *bssid;                      ///< WIFI ap的mac地址,格式必须符合mac地址 例如 "02:42:eb:ef:c3:ed"
	char *psk;                        ///< WIFI 密码, 最大长度 32 字节
	char connect_times;               ///< WIFI 连接次数
} GxUpdateWifiParam;

/**
 * http 升级配置结构体
 */
typedef struct {
	char *url;                               ///< 升级 bin 的 http 地址, 最大长度2048字节
	char http_get_times;                     ///< HTTP 下载次数
} GxUpdateHTTPCfg;

/**
 * 升级配置结构体
 */
typedef struct {
	GxDLP_Download_Type type;       ///< 升级类型
	union {
		GxUpdateHTTPCfg http;   ///< http 升级配置
	};
} GxUpdateCfg;

/**
 * 升级镜像信息
 */
typedef struct {
	GxDLP_Download_Type download_type;    ///< 升级类型
	GXUPDATE_UPGRADE_MODE upgrade_mode;   ///< 升级模式
	uint32_t mid;                         ///< 厂商 ID
	uint32_t hwver;                       ///< 硬件版本号
	uint32_t swver;                       ///< 软件版本号
	uint32_t sn_start;                    ///< SN号范围起始值
	uint32_t sn_end;                      ///< SN号范围结束值
	uint32_t oui;                         ///< 组织唯一ID
	uint32_t operator_id;                 ///< 运营商ID
} GxUpdateImageInfo;

/**
 * OTA 镜像数据
 */
typedef struct {
	void *image_data;                 ///< OTA 镜像内存数据
	uint32_t image_size;              ///< OTA 镜像内存大小
} GxUpdateOTAImg;

/**
 * loader ota 升级顺序列表
 */
typedef struct {
	GxDLP_Download_Type type_list[GXDLP_DOWNLOAD_SEQ_MAX];  ///< 升级顺序列表
} GxUpdateTypeListCfg;

/**
 * 设置 loader ota 升级时的升级顺序列表
 *\param list_cfg 结构体信息请参考 gxdocref GxUpdateTypeListCfg
 *\return 无
 */
int GxUpdate_SetUpgradeTypeList(GxUpdateTypeListCfg *list_cfg);

/**
 * 设置 loader ota 升级需要配置的参数
 *\param cfg 指向升级配置结构体指针，结构体信息请参考 gxdocref GxUpdateCfg
 *\return 无
 */
int GxUpdate_SetConfig(GxUpdateCfg *cfg);

/**
 * 设置 loader ota 升级需要配置的最高优先级网络设备类型
 *
 * - 如果没有设置则按照默认优先级
 * - 该接口实际调用 gxdocref GxDLP_Set_HighestPriNetDevice 设置
 * - 默认优先级按照 gxdocref GxDLP_Net_Device_Type 枚举的排列顺序
 *\param type 网络设备类型 gxdocref GxDLP_Net_Device_Type
 *\return 无
 */
int GxUpdate_SetHighesetPriNetDevice(GxDLP_Net_Device_Type type);

/**
 * 设置 loader ota 升级需要的 wifi 参数
 *\param wifi 指向 wifi 参数指针，结构体信息请参考 gxdocref GxUpdateWifiParam
 *\return 无
 */
int GxUpdate_SetWifiParam(GxUpdateWifiParam *wifi);
/**
 * 该函数用于应用通知升级线程把下载的升级镜像写到实际的 Flash 中, 该函数必须在
 * 收到消息 GXUPDATE_PARSE_OK 后调用. 设计该函数的目的是为了在应用中下载完升级镜
 * 像，应用也可以取消升级
 *\param void
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int GxUpdate_StartInstall(void);

/**
 * 获取升级镜像信息
 *\param info 升级镜像信息
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 *\retval -2 参数错误
 */
int GxUpdate_GetImageInfo(GxUpdateImageInfo *info);

/**
 * 获取 OTA 镜像信息
 * - 该接口只能在收到消息 GXOTA_STATUS_VERSION_OK 或者 GXUPDATE_PARSE_OK 时调用才会返回成功
 *\param img OTA 镜像数据
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败, 升级镜像不包含 OTA 或者当前未收到 GXOTA_STATUS_VERSION_OK 和 GXUPDATE_PARSE_OK
 *\retval -2 参数错误
 */
int GxUpdate_GetOTAImg(GxUpdateOTAImg *img);
/**
 * 获取HTTP版本检测进度信息
 *\param progress
 *\return int 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 *\retval -2 参数错误
 */
int GxUpdate_GetHTTPDetectVersionProgress(GxUpdateDetectVersionProgress *progress);
/**
 * HTTP升级判断检测到新版本
 *\param void
 *\return 返回函数执行是否成功
 *\retval true 检测到新版本
 *\retval false 没检测到新版本
 */
bool GxUpdate_HTTPIsTrigger(void);
/**
 * HTTP检测新版本
 *\param url 检测版本的url地址
 *\param 检测周期, 单位: 秒
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int GxUpdate_StartHTTPDetectVersion(const char *url, int detect_cycle);
/**
 * HTTP停止检测新版本
 *\param void
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int GxUpdate_StopHTTPDetectVersion(void);
/**
 * HTTP启动升级新版本线程
 *\param url 升级镜像的url地址
 *\return 返回函数执行是否成功
 *\retval 0 成功, 表示HTTP升级线程启动成功
 *\retval -1 失败, 表示HTTP升级线程启动失败, 有可能是线程已经在运行了
 */
int GxUpdate_StartHTTPUpgrade(const char *url);
/**
 * HTTP停止升级新版本线程，执行该函数后，只是通知线程要停止，线程不会立即停止，线程需要自己释放掉资源后，停止
 *\param void
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int GxUpdate_StopHTTPUpgrade(void);
#endif /* __GXDOWNLOADER_API_H__ */
/** @}*/
