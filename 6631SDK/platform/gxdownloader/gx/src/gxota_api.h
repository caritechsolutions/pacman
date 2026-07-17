/** \addtogroup <label> 
 *  *  @{
 *   */
#ifndef __GXOTA_H__
#define __GXOTA_H__

#include "gxos/gxcore_partition.h"
#include "gxota_msg.h"
#include "gxdlp_api.h"

typedef enum {
	GXUPDATE_GETTING_VERSION,                   ///< 表示版本检测线程正在获取版本信息
	GXUPDATE_GOT_VERSION,                       ///< 表示版本检测线程已经获取到版本信息
} GXUPDATE_DETECT_VERSION_STATUS;

/**
 *  检测版本进度结构体
 */
typedef struct {
	bool version_was_got;                        ///< 表示版本曾经获取过版本信息，但是不代表获取的版本信息有效，
	bool version_is_ok;                          ///< 表示是否检测到新版本, 与 GxOTA_isTrigger 的表达的意思一致
	GXUPDATE_DETECT_VERSION_STATUS status;       ///< 表示版本检测进度状态
} GxUpdateDetectVersionProgress;

/**
 *升级模式控制
 */
typedef enum {
	GX_OTA_NORMAL_UPGRADE,           ///< 正常升级
	GX_OTA_SN_UPGRADE,               ///< 判断 SN 号升级，具体判断由客户自己实现
	GX_OTA_FORCE_UPGRADE,            ///< 强制升级，忽略软件版本号比较, 但厂商ID和硬件版本号必须匹配
	GX_OTA_VERSION_NOTEQ_UPGRADE     ///< 软件版本号不相等升级, 但厂商ID和硬件版本号必须匹配
	/* New values should be put at the end without altering the order */
} GxOTAType;

/**
 * OTA DDTEX升级Frontend和Demod配置结构体
 */
typedef struct {
	/* demux param */
	uint32_t dmx_pid;                  ///< 升级流pid
	enum dmx_input dmx_source;         ///< demux source

	/* frontend param */
	uint32_t Frequency;                ///< Frequency unit:KHz
	uint32_t Symbolrate;               ///< Symbolrate
	uint16_t Modulation;               ///< modulation
	fe_sec_voltage_t Polarity;         ///< polarity (satellite)
	fe_sec_tone_mode_t Sat22k;         ///< Sat22k (satellite)
	int Diseqc10_Port;                 ///< Diseqc10_Port
	int Diseqc11_Port;                 ///< Diseqc11_Port
	fe_bandwidth_t bandwidth;          ///< bandwidth
} ota_ddtex_upgrade_struct;

/**
 * 升级参数配置结构体
 */
typedef struct {
	GxDLP_Download_Type OTA_Type;     ///< 升级类型
	uint32_t OTA_Repeat_Times;        ///< 升级失败重试次数
	union {
		ota_ddtex_upgrade_struct ddtex_upgrade; ///< OTA DDTEX 升级时，相应的frontend和demod的配置
	} ota_u;
} ota_trigger_info;

typedef ota_trigger_info GXOTATriggerInfo;

/**
 * 升级分区信息
 */
typedef struct {
	char     partition_name[FLASH_PARTITION_NAME_SIZE+1];  ///< 升级的分区名
	char     fstype;                                       ///< 升级分区的文件系统类型
	uint32_t size;                                         ///< 需要写入升级分区的数据大小
} GxOTAUpgradePart;

#define MAX_UPGRADE_NUM 32   ///< MAX_UPGRADE_NUM == 32 because of MAX_FILE == 32 which defined at gx/src/common.h
/**
 * 升级包信息
 */
typedef struct {
	uint32_t hwver;                            ///< 硬件版本号，与GxOTA_Get_OTA_HW_Version获取的一样
	uint32_t swver;                            ///< 软件版本号，与GxOTA_Get_OTA_SW_Version获取的一样
	uint32_t mid;                              ///< 厂商ID号，与GxOTA_Get_OTA_Mid获取的一样
	uint32_t is_upgrade_all;                   ///< 升级整bin标志
	uint32_t upgrade_part_num;                 ///< 升级分区个数
	uint32_t total_upgrade_size;               ///< 有效升级数据大小，去除填充数据和升级分区表数据, total_upgrade_size = part[0].size + ... part[upgrade_part_num-1].size
	uint32_t total_size;                       ///< 升级包大小，> total_upgrade_size，包含升级分区表以及填充数据
	GxOTAUpgradePart part[MAX_UPGRADE_NUM];    ///< 升级分区信息 gxdocref GxOTAUpgradePart
} GxOTAUpgradePackageInfo;

/**
 * 升级流 SN 号范围
 */
typedef struct {
	uint32_t start;  ///< 起始值
	uint32_t end;    ///< 结束值
}GxOTASNRange;

/**
 * OTA检测新版本
 *\param dmx_id 选择使用哪个demux 取值[0-3]
 *\param dmx_source 选择demux 使用哪个端口的数据取值[0-3]
 *\param pid 升级流所在表的pid
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
extern int GxOTA_Start_Detect_Version(uint32_t dmx_id, uint32_t dmx_source, uint16_t pid);
/**
 * OTA停止检测新版本
 *\param void
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
extern int GxOTA_Stop_Detect_Version(void);
/**
 * OTA判断检测到新版本
 *\param void
 *\return 返回函数执行是否成功
 *\retval true 检测到新版本
 *\retval false 没检测到新版本
 */
extern bool GxOTA_isTrigger(void);
/**
 * 获取OTA版本检测进度信息
 *\param progress
 *\return int 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 *\retval -2 参数错误
 */
int GxOTA_GetDetectVersionProgress(GxUpdateDetectVersionProgress *progress);
/**
 * OTA设置loader ota需要配置的参数，根据参数配置frontend,demod，新版本检测判断条件等
 *\param ota_trigger 指向升级配置结构体指针，结构体信息请参考 gxdocref ota_trigger_info
 *\return 无
 */
extern void GxOTA_SetTrigger(ota_trigger_info *ota_trigger);
/**
 * OTA获取升级包信息
 *\param info 传入存放升级信息变量的地址 gxdocref GxOTAUpgradePackageInfo
 *\return 无
 */
extern int GxOTA_Get_UpgradePackageInfo(GxOTAUpgradePackageInfo *info);
/**
 * OTA获取新版本的软件版本号
 *\param sw 指向用于存放新的软件版本号的uint32_t指针
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
extern int GxOTA_Get_OTA_SW_Version(uint32_t *sw);
/**
 * OTA获取新版本的硬件版本号
 *\param hw 指向用于存放新的硬件版本号的uint32_t指针
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
extern int GxOTA_Get_OTA_HW_Version(uint32_t *hw);
/**
 * OTA获取新版本的厂商ID
 * \param did 指向用于存放新的厂商ID的uint32_t指针
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
extern int GxOTA_Get_OTA_sid(uint32_t *aid);
/**
 * OTA获取升级流的SN号范围
 * param range 指向用于存放升级流的SN号范围的GxOTASNRange指针
 * return 返回函数执行是否成功
 * retval 0 成功
 * retval -1 失败
 */
extern int GxOTA_GetSNRange(GxOTASNRange *range);
/**
 * OTA获取升级流的类型
 * param type 指向用于存放升级流的类型的GxOTAType指针
 * return 返回函数执行是否成功
 * retval 0 成功
 * retval -1 失败
 */
extern int GxOTA_GetType(GxOTAType *type);
/**
 * OTA启动升级新版本线程
 *\param dmx_id 选择使用哪个demux 取值[0-3]
 *\param dmx_source 选择demux 使用哪个端口的数据取值[0-3]
 *\param pid 升级流所在表的pid
 *\return 返回函数执行是否成功
 *\retval 0 成功, 表示OTA升级线程启动成功
 *\retval -1 失败, 表示OTA升级线程启动失败, 有可能是线程已经在运行了
 */
extern int GxOTA_Start_Upgrade(uint32_t dmx_id, uint32_t dmx_source, uint16_t pid);
/**
 * OTA停止升级新版本线程，执行该函数后，只是通知线程要停止，线程不会立即停止，线程需要自己释放掉资源后，停止
 *\param void
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
extern int GxOTA_Stop_Upgrade(void);
/**
 * OTA用于与上层应用进行同步，只有上层应用调用该函数后，升级线程才会把接收到的新版本软件写到flash
 *\param void
 *\return 无
 */
extern void GxOTA_Start_Write_Flash(void);
#endif
/** @}*/
