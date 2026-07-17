/** \addtogroup <label>
 *  *  @{
 *   */
#ifndef __DLP_H__
#define __DLP_H__

#include "av/gxav_vout_propertytypes.h"
#include "common/gxoem.h"
#include "frontend/frontend.h"
#include "dvbhal/gxfe_hal.h"

/**
 *  TUNER类型
 */
typedef enum{
	GXDLP_TUNER_AV2018,        ///< TUNER_AV2018
	GXDLP_TUNER_AV2020,        ///< TUNER_AV2020
	GXDLP_TUNER_MXL608_DTMB,   ///< TUNER_MXL608_DTMB
	GXDLP_TUNER_MXL608_T,      ///< TUNER_MXL608_T
	GXDLP_TUNER_MXL608_C,      ///< TUNER_MXL608_C
	GXDLP_TUNER_TDA18250,      ///< TUNER_TDA18250
	GXDLP_TUNER_TDA18250A,     ///< TUNER_TDA18250A
	GXDLP_TUNER_TDA18273_DTMB, ///< TUNER_TDA18273_DTMB
	GXDLP_TUNER_TDA18273_DVBC, ///< TUNER_TDA18273_DVBC
	GXDLP_TUNER_R836_T,        ///< TUNER_R836_T
	GXDLP_TUNER_R836_C,        ///< TUNER_R836_C
	GXDLP_TUNER_R836_DTMB,     ///< TUNER_R836_DTMB
	GXDLP_TUNER_R910_T,        ///< TUNER_R910_T
	GXDLP_TUNER_R910_C,        ///< TUNER_R910_C
	GXDLP_TUNER_SI2141_DTMB,   ///< TUNER_SI2141_DTMB
	GXDLP_TUNER_SI2141_C,      ///< TUNER_SI2141_T
	GXDLP_TUNER_SI2141_T,      ///< TUNER_SI2141_C
	GXDLP_TUNER_R848_S,        ///< TUNER_R848_S
	GXDLP_TUNER_R848_C,        ///< TUNER_R848_C
	GXDLP_TUNER_R848_T,        ///< TUNER_R848_T
	GXDLP_TUNER_R848_DTMB,     ///< TUNER_R848_DTMB
	GXDLP_TUNER_RDA5815M,      ///< TUNER_RDA5815M
	GXDLP_TUNER_ATBM2040,      ///< TUNER_ATBM2040
	GXDLP_TUNER_ATBM2030,      ///< TUNER_ATBM2030
	GXDLP_TUNER_SHARP7306,     ///< TUNER_SHARP7306
	GXDLP_TUNER_ATBM253_DTMB,  ///< TUNER_ATBM253_DTMB
	GXDLP_TUNER_ATBM253_DVBT,
	GXDLP_TUNER_ATBM253_DVBC,
	GXDLP_TUNER_RDA5880U_DTMB,
	GXDLP_TUNER_RDA5880U_T,
	GXDLP_TUNER_RDA5880U_C,
	GXDLP_TUNER_ATBM2040_DVBT,
	GXDLP_TUNER_ATBM2040_DVBC,
	GXDLP_TUNER_ATBM2040_DTMB,
	GXDLP_TUNER_TS6011,
	GXDLP_TUNER_M3031,
	GXDLP_TUNER_RDA5160_DTMB,
	GXDLP_TUNER_RDA5160_C,
	GXDLP_TUNER_RDA5160_T,
	GXDLP_TUNER_RT720,
	GXDLP_TUNER_MTV600,
	GXDLP_TUNER_A3500,
	GXDLP_TUNER_R858_DVBC_1ST,
	GXDLP_TUNER_R858_DVBC_2ND,
	GXDLP_TUNER_MAX
}GxDLP_Tuner_Type;

/**
 *  DEMOD类型
 */
typedef enum{
	GXDLP_DEMOD_GX1121D,           ///< DEMOD_GX1121D,
	GXDLP_DEMOD_GX113X,            ///< DEMOD_GX113X,
	GXDLP_DEMOD_GX3211,            ///< DEMOD_GX3211,
	GXDLP_DEMOD_GX1001,            ///< DEMOD_GX1001,
	GXDLP_DEMOD_GX1503B_DVBC,      ///< DEMOD_GX1503B_DVBC,
	GXDLP_DEMOD_GX1503B_DTMB,      ///< DEMOD_GX1503B_DTMB,
	GXDLP_DEMOD_GX3211_DVBC,       ///< DEMOD_GX3211_DVBC,
	GXDLP_DEMOD_ATBM78X,           ///< DEMOD_ATBM78X,
	GXDLP_DEMOD_ATBM888X_DVBC,     ///< DEMOD_ATBM888X_DVBC,
	GXDLP_DEMOD_ATBM888X_DTMB,     ///< DEMOD_ATBM888X_DTMB,
	GXDLP_DEMOD_ATBM783X_DVBC,     ///< DEMOD_ATBM783X_DVBC,
	GXDLP_DEMOD_ATBM783X_DVBS,     ///< DEMOD_ATBM783X_DVBS,
	GXDLP_DEMOD_GX1133,            ///< DEMOD_GX1133,
	GXDLP_DEMOD_Sirius_DVBS,       ///< DEMOD_Sirius_DVBS,
	GXDLP_DEMOD_Sirius_DVBC,       ///< DEMOD_Sirius_DVBC,
	GXDLP_DEMOD_MXL683,            ///< DEMOD_MXL683,
	GXDLP_DEMOD_Taurus_DVBS,       ///< DEMOD_Taurus_DVBS,
	GXDLP_DEMOD_Taurus_DVBC,       ///< DEMOD_Taurus_DVBC,
	GXDLP_DEMOD_GX6605S_DVBS,      ///< DEMOD_GX6605S_DVBS,
	GXDLP_DEMOD_GX1135_DVBS,       ///< DEMOD_GX1135S_DVBS,
	GXDLP_DEMOD_Gemini_DVBT,       ///< DEMOD_Gemini_DVBT,
	GXDLP_DEMOD_Gemini_DVBC,       ///< DEMOD_Gemini_DVBC,
	GXDLP_DEMOD_Cygnus_DVBT,
	GXDLP_DEMOD_Cygnus_DVBC,
	GXDLP_DEMOD_Sirius_J83B,
	GXDLP_DEMOD_Taurus_J83B,
	GXDLP_DEMOD_Canopus_DVBS,
	GXDLP_DEMOD_Canopus_DVBC,
	GXDLP_DEMOD_Canopus_J83B,
	GXDLP_DEMOD_Canopus_DVBC1,
	GXDLP_DEMOD_ATBM78X_DVBC,
	GXDLP_DEMOD_ATBM78X_REPEAT_DVBT,
	GXDLP_DEMOD_ATBM78X_REPEAT_DVBC,
	GXDLP_DEMOD_Cygnus_FM,
	GXDLP_DEMOD_CXD2856_DVBT,
	GXDLP_DEMOD_CXD2856_DVBC,
	GXDLP_DEMOD_CXD2856_REPEAT_DVBT,
	GXDLP_DEMOD_CXD2856_REPEAT_DVBC,
	GXDLP_DEMOD_SI2183_DVBT,
	GXDLP_DEMOD_SI2183_DVBC,
	GXDLP_DEMOD_Vega_DVBS,
	GXDLP_DEMOD_Vega_DVBC,
	GXDLP_DEMOD_Vega_J83B,
	GXDLP_DEMOD_Vega_DVBC1,
	GXDLP_DEMOD_Scorpio_DVBC,
	GXDLP_DEMOD_MAX
}GxDLP_Demod_Type;

/**
 *  GxDonwloader升级类型
 */
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

typedef enum {
	GXDLP_DOWNLOAD_SEQ_FIRST,      ///< 升级第一顺序类型
	GXDLP_DOWNLOAD_SEQ_SECOND,     ///< 升级第二顺序类型
	GXDLP_DOWNLOAD_SEQ_THIRD,      ///< 升级第三顺序类型
	GXDLP_DOWNLOAD_SEQ_MAX
} GxDLP_Download_Sequence;

/**
 *  GxDonwloader系统语言选择
 */
typedef enum{
	GXDLP_CHINESE ,                ///< 中文
	GXDLP_ENGLISH ,                ///< 英语
	GXDLP_UYGUR   ,                ///< 维吾尔(目前不支持)

	GXDLP_LANGUAGE_MAX
}GxDLP_Language_Type;

/**
 * 网络设备类型
 */
typedef enum {
	GXDLP_NET_DEVICE_ETH,         ///< eth
	GXDLP_NET_DEVICE_USBETH,      ///< usb eth
	GXDLP_NET_DEVICE_WIFI,        ///< wifi
	GXDLP_NET_DEVICE_MAX
} GxDLP_Net_Device_Type;

/**
 * 根据tuner的枚举型号获取tuner型号的名字
 *\param type tuner的型号
 *\return tuner型号名字
 */
char* get_tunername(GxDLP_Tuner_Type type);
/**
 * 根据demod的枚举型号获取demod型号的名字
 *\param type demod的型号
 *\return demod型号名字
 */
char* get_demodname(GxDLP_Demod_Type type);
/**
 * 版本号字符串转为整型
 *\param str 版本号字符串
 *\return 版本号整型
 */
uint32_t convert_version_str(const char *str);
/**
 * 版本号整型转为字符串
 *\param num 版本号整型
 *\return 版本号字符串形式
 */
char *convert_version_num_to_char(uint32_t num);

/**
 *\ 根据DLP分区初始化DLP参数,
 *\ the format of partition path of dlp and backup_dlp is not include "/", like "OEM"
 *\ oppositely, the format of minifs path of dlp and backup_dlp_path is include "/" and must be prefixed with "/home/gx", like "/home/gx/oem.ini"
 *\ using different path format to determine how to read and write dlp and backup_dlp file
 *\ NOTE: path name length not longer than 40 bytes
 *\param DLP分区路径
 *\param 备份DLP分区路径
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Init(char *dlp,char *backup_dlp);
/**
 * 关闭DLP, 释放DLP使用的资源
 *\param void
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Close(void);
/**
 * DLP同步，把DLP新的数据写回到DLP分区
 *\param void
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Sync(void); //sync dlp and backup_dlp
/**
 * 获取设备硬件版本号
 *\param void
 *\return 硬件版本号
 *\retval 非-1 成功, 获取的硬件版本号有效
 *\retval -1 失败，DLP分区未定义硬件版本号字段
 */
uint32_t GxDLP_Get_HardwareVersion(void);
/**
 * 获取设备当前的软件版本号
 *\param void
 *\return 软件版本号
 *\retval 非-1 成功, 获取的软件版本号有效
 *\retval -1 失败，DLP分区未定义软件版本号字段
 */
uint32_t GxDLP_Get_SoftwareVersion(void);
/**
 * 获取要升级的新软件的版本号,只在升级的过程中获得的新软件版本号有效
 *\param void
 *\return 获取要升级的新软件的版本号
 *\retval 非-1 成功, 获取的要升级的新软件的版本号有效
 *\retval -1 失败，DLP分区未定义新软件的版本号字段
 */
uint32_t GxDLP_Get_UpdateSoftVersion(void);
/**
 * 获取设备厂商ID
 *\param void
 *\return 设备厂商ID
 */
uint32_t GxDLP_Get_ManufactureId(void);
/**
 * 获取组织唯一标识符 Organizationally unique identifier
 *\param oui
 *\retval 0  成功
 *\retval -1 获取失败
 *\retval -2 参数为 NULL
 */
int32_t GxDLP_Get_OUI(uint32_t *oui);
/**
 * 获取运营商ID
 *\param operator_id
 *\retval 0  成功
 *\retval -1 获取失败
 *\retval -2 参数为 NULL
 */
int32_t GxDLP_Get_OperatorID(uint32_t *operator_id);
/**
 * 获取设备序列号，目前未使用
 *\param void
 *\return 设备序列号
 */
int32_t GxDLP_Get_SerialNumber(void);
/**
 * 获取TV显示制式，目前未使用
 *\param type 指向TV显示制式类型的指针
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Get_TvStandard(GxVideoOutProperty_Mode *type);
/**
 * 获取GxDownloader的系统语言
 *\param type 指向系统语言的指针, 枚举系统语言信息请参考gxdocref GxDLP_Language_Type
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Get_Language(GxDLP_Language_Type *type);
/**
 * 获取GxDonwloader的第一升级类型
 *\param type 指向系统语言的指针, 枚举系统语言信息请参考gxdocref GxDLP_Download_Type
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Get_UpdateType(GxDLP_Download_Type *type);
/**
 * -根据index获取GxDownloader的各个顺序的升级类型
 * -index == GXDLP_DOWNLOAD_SEQ_FIRST 时该函数等于GxDLP_Get_UpdateType
 *
 *\param type 升级类型，具体请参考gxdocref GxDLP_Download_Type
 *\param index 升级顺序，具体请参考gxdocref GxDLP_Download_Sequence
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Get_UpdateTypeByIndex(GxDLP_Download_Type *type, GxDLP_Download_Sequence index);
/**
 * 获取升级包的文件名
 *\param void
 *\return 返回升级包的文件名
 */
char   *GxDLP_Get_UpgradeFileName(void);

/**
 * 设置TV显示制式 (未使用)
 *\param type TV显示制式
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_TvStandard(GxVideoOutProperty_Mode type);
/**
 * 设置GxDownloader的系统语言
 *\param type 系统语言，具体请参考gxdocref GxDLP_Language_Type
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_Language(GxDLP_Language_Type type);
/**
 * 设置GxDownloader的第一升级类型
 *\param type 升级类型，具体请参考gxdocref GxDLP_Download_Type
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_UpdateType(GxDLP_Download_Type type);
/**
 * -根据index设置GxDownloader的各个顺序的升级类型
 * -index == GXDLP_DOWNLOAD_SEQ_FIRST 时该函数等于GxDLP_Set_UpdateType
 *
 *\param type 升级类型，具体请参考gxdocref GxDLP_Download_Type
 *\param index 升级顺序，具体请参考gxdocref GxDLP_Download_Sequence
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_UpdateTypeByIndex(GxDLP_Download_Type type, GxDLP_Download_Sequence index);
/**
 * 清除升级类型
 *\param void
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Clear_UpdateType(void);
/**
 * 设置升级包的文件名
 *\param value 升级包的文件名
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */

int32_t GxDLP_Set_UpgradeFileName(char* Value);
/**
 * 设置设备的软件版本号
 *\param value 软件版本号
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_SoftwareVersion(char *value);
/**
 * 设置要升级的新软件版本号
 *\param value 新软件版本号
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_UpdateSoftVersion(char *value);
/**
 * 获取OTA DDTEX升级时的demux id
 *\param void
 *\return 返回dmx id
 */
int32_t GxDLP_Get_DmxId(void);
/**
 * 获取OTA DDTEX升级时demux获取数据失败的超时时间,单位为秒
 *\param void
 *\return 返回超时时间，不会小于30秒的限制
 */
int32_t GxDLP_Get_DmxTimeout(void);
/**
 * 获取 OTA DDTEX 下载次数
 *
 *\param void
 *\return 返回 OTA DDTEX 下载次数
 */
int32_t GxDLP_Get_OTADDTEXDownTimes(void);
/**
 * 设置 OTA DDTEX 下载次数
 *\param times 次数
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_OTADDTEXDownTimes(int32_t times);
/**
 * 获取OTA DSMCC升级时的demux source
 *\param void
 *\return 返回函数执行是否成功
 *\retval  demux source
 *\retval -1 失败
 */
enum dmx_input GxDLP_Get_DmxSource(void);
/**
 * 获取OTA DDTEX升级时的demux要过滤的TS流的pid号
 *\param void
 *\return 返回pid号
 */
int32_t GxDLP_Get_DmxPid(void);
/**
 * 获取OTA DDT升级时的demux要过滤的TS流的table id号
 *\param void
 *\return 返回table id号
 */
int32_t GxDLP_Get_DmxTableId(void);
/**
 * 设置OTA DDTEX升级时的demux source
 *\param type demux的source选择
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_DmxSource(enum dmx_input type);
/**
 * 设置OTA DDTEX升级时的demux要过滤的TS流的pid
 *\param value 字符串形式的pid,如十进制的2003,这里传"2003"，十六进制的0x07d3, 这里传"0x07d3"
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_DmxPid(char* value);
/**
 * 设置OTA DDT升级时的demux要过滤的TS流的table id
 *\param value 字符串形式的table id,如十进制的2003,value应为"2003"，十六进制的0x07d3, value应为"0x07d3"
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_DmxTableId(char* value);
/**
 * 设置OTA DDTEX升级时的dmx id
 *\param value 字符串形式的dmx id,如十进制的1,value应为"1"，十六进制的0x1, value应为"0x1"
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_Dmxid(char* value);
/**
 * 设置OTA DDTEX升级时的demux获取数据失败的超时时间,单位为秒,不能小于30秒的限制
 *\param value 字符串形式的超时时间,如十进制的30,value应为"30"，十六进制的0x100, value应为"0x100"
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_DmxTimeout(char* value);
/**
 * 获取OTA DDTEX升级时的升级失败重试次数
 *\param void
 *\return 返回升级失败重试次数
 */
int32_t GxDLP_Get_OtaRepeatTimes(void);
/**
 * 设置OTA DDTEX升级时的升级失败重试次数
 *\param value 字符串形式的升级重试次数,如十进制形式3,value应为"3"，十六进制形式0x3, value应为"0x3"
 *\return 返回升级失败重试次数
 */
int32_t GxDLP_Set_OtaRepeatTimes(char *value);
/**
 * 获取升级时的升级失败重试次数
 *\param void
 *\return 返回升级失败重试次数
 */
int32_t GxDLP_Get_UpdateRepeatTimes(void);
/**
 * 设置升级时的升级失败重试次数
 *\param repeat_times 重试次数
 *\return 返回升级失败重试次数
 */
int32_t GxDLP_Set_UpdateRepeatTimes(int32_t repeat_times);
/**
 * 获取OTA DDTEX升级时前端frontend锁定超时时间,单位:秒
 *\param void
 *\return 返回前段frontend锁定超时时间
 */
int32_t GxDLP_Get_FeRepeatTimes(void);
/**
 * 获取OTA DDTEX升级时的前端frontend的类型,如DVB-S2,DVB-T
 *\param type 指向fe_type_t枚举的指针
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Get_FeType(fe_type_t *type);
/**
 * 获取OTA DDTEX升级时的前端frontend的tuner类型
 *\param type 指向GxDLP_Tuner_Type枚举的指针 gxdocref GxDLP_Tuner_Type
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Get_FeTunerType(GxDLP_Tuner_Type *type);
/**
 * 获取OTA DDTEX升级时前端的tuner设备id
 *\param void
 *\return 返回tuner设备id
 *\retval 非-1 成功，返回的tuner设备id值有效
 *\retval -1 失败
 */
int32_t GxDLP_Get_FeTunerDevId(void);
/**
 * 设置OTA DDTEX升级时前端的tuner设备id
 *\param dev_id tuner设备id
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeTunerDevId(uint32_t dev_id);
/**
 * 获取OTA DDTEX升级时前端的tuner的i2c地址
 *\param void
 *\return 返回tuner的i2c地址
 *\retval 非-1 成功，返回的tuner i2c addr值有效
 *\retval -1 失败
 */
int32_t GxDLP_Get_FeTunerI2cAddr(void);
/**
 * 设置OTA DDTEX升级时前端tuner的i2c地址
 *\param addr tuner的i2c地址
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeTunerI2cAddr(uint32_t addr);
/**
 * 获取OTA DDTEX升级时的前端的tuner的晶振频率
 *\param void
 *\return 返回tuner的晶振频率
 *\retval 非-1 成功，返回的tuner的晶振频率值有效
 *\retval -1 失败
 */
int32_t GxDLP_Get_FeTunerOscFre(void);
/**
 * 设置OTA DDTEX升级时前端tuner的晶振频率
 *\param fre tuner的晶振频率
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeTunerOscFre(uint32_t fre);

/**
 * 获取OTA DDTEX升级时的前端tuner的晶振电容, 单位是 pf
 *\param void
 *\return 返回tuner的晶振频率
 *\retval >= 0，返回的tuner的晶振电容有效
 *\retval < 0 失败
 */
int32_t GxDLP_Get_FeTunerXtalCap(void);

/**
 * 设置OTA DDTEX升级时的前端tuner的晶振电容, 单位是 pf
 *\param void
 *\return 返回tuner的晶振频率
 *\retval = 0，设置 tuner的晶振电容有效
 *\retval != 0 失败
 */
int32_t GxDLP_Set_FeTunerXtalCap(int32_t xtal_cap_pf);

/**
 * 获取OTA DDTEX升级时前端的demod类型
 *\param type 指向GxDLP_Demod_Type枚举的指针 gxdocref GxDLP_Demod_Type
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Get_FeDemodType(GxDLP_Demod_Type *type);
/**
 * 获取OTA DDTEX升级时前端的demod设备id
 *\param void
 *\return 返回demod设备id
 *\retval 非-1 成功，返回的demod设备id值有效
 *\retval -1 失败
 */
int32_t GxDLP_Get_FeDemodDevId(void);
/**
 * 设置OTA DDTEX升级时前端的demod设备id
 *\param dev_id demod设备id
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeDemodDevId(uint32_t dev_id);
/**
 * 获取OTA DDTEX升级时前端的demod的i2c地址
 *\param void
 *\return 返回demod的i2c地址
 *\retval 非-1 成功，返回的demod i2c addr值有效
 *\retval -1 失败
 */
int32_t GxDLP_Get_FeDemodI2cAddr(void);
/**
 * 设置OTA DDTEX升级时前端demod的i2c地址
 *\param addr demod的i2c地址
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeDemodI2cAddr(uint32_t addr);
/**
 * 获取OTA DDTEX升级时前端的demod的串并行配置
 *\param mode demod串并行配置
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Get_FeDemodTsMode(ts_mode_t *mode);
/**
 * 设置OTA DDTEX升级时前端的demod串并行配置
 *\param mode demod串并行配置
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeDemodTsMode(ts_mode_t mode);
/**
 * 获取OTA DDTEX升级时前端的demod的iq swap
 *\param void
 *\return 返回demod的iq swap
 *\retval 非-1 成功，返回的demod iq swap值有效
 *\retval -1 失败
 */
int32_t GxDLP_Get_FeDemodIqMode(void);
/**
 * 设置OTA DDTEX升级时前端的demod的iq swap
 *\param mode demod的iq swap
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeDemodIqMode(uint32_t mode);
/**
 * 获取OTA DDTEX升级时前端的demod的is oscillator
 *\param void
 *\return 返回demod的is oscillator
 *\retval 非-1 成功，返回的demod is oscillator值有效
 *\retval -1 失败
 */
int32_t GxDLP_Get_FeDemodIsOscillator(void);
/**
 * 设置OTA DDTEX升级时前端的demod的is oscillator
 *\param mode demod的is oscillator
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeDemodIsOscillator(uint32_t is_oscillator);
/**
 * 获取OTA DDTEX升级时前端的demod的ts pin
 *\param void
 *\return 返回demod的ts pin
 *\retval 非-1 成功，返回的demod ts pin值有效
 *\retval -1 失败
 */
int32_t GxDLP_Get_FeDemodTsPin(void);
/**
 * 设置OTA DDTEX升级时前端的demod的ts pin
 *\param mode demod的ts pin
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeDemodTsPin(uint32_t ts_pin);
/**
 * 获取OTA DDTEX升级时前端的demod的ts pin map
 * - 获取到的 demod_pin_map, 调用 ioctl(fe_handle, FE_SET_TSPIN_MAP, &(demod_pin_map)); 设置前端
 * - ini 格式: fe_demod_ts_pinmap=ERRS|ERRS|ERRS|ERRS|VALID|SYNC|CLK|DATA0|ERRS|ERRS|ERRS|ERRS
 *\param  demod_pin_map 指向 struct pin_config 的指针 gxdocref struct pin_config
 *\return 返回函数是否执行成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Get_FeDemodTsPinMap(struct pin_config *demod_pin_map);
/**
 * 设置OTA DDTEX升级时前端的demod的ts pin map
 * - ini 格式: fe_demod_ts_pinmap=ERRS|ERRS|ERRS|ERRS|VALID|SYNC|CLK|DATA0|ERRS|ERRS|ERRS|ERRS
 *\param  demod_pin_map 指向 struct pin_config 的指针 gxdocref struct pin_config
 *\return 返回函数是否执行成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeDemodTsPinMap(struct pin_config *demod_pin_map);
/**
 * 获取OTA DDTEX升级时的前端frontend的symbolrate
 *
 * - unit:Kbps
 *
 *\param void
 *\return 返回symbolrate的值
 */
int32_t GxDLP_Get_FeSymbolrate(void);
/**
 * 获取OTA DDTEX升级时的前端frontend的frequency
 *
 * - unit:KHz
 *
 *\param void
 *\return 返回frequency的值
 */
int32_t GxDLP_Get_FeFrequency(void);
/**
 * 获取OTA DDTEX升级时的前端frontend的modulation
 *\param type 指向fe_modulation_t枚举的指针 gxdocref fe_modulation_t
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Get_FeModulation(fe_modulation_t *type);
/**
 * 获取OTA DDTEX升级时的前端frontend的bandwidth
 *\param void
 *\return 返回bandwidth的值
 */
int32_t GxDLP_Get_FeBandwidth(void);
/**
 * 获取OTA DDTEX升级时的前端frontend的polarity
 *\param type 指向fe_sec_voltage_t枚举的指针 gxdocref fe_sec_voltage_t
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Get_FePolarity(fe_sec_voltage_t *polar);
/**
 * 获取OTA DDTEX升级时的前端frontend的polar invert
 *\param void
 *\return 返回polar invert的值
 */
int32_t GxDLP_Get_FePolarInvert(void);

/**
 * 获取OTA DDTEX升级时的前端frontend的polar gpio
 *\param void
 *\return 返回polar gpio的值
 *\retval 非-1 成功，返回的polar gpio值有效
 *\retval -1 失败
 */
int32_t GxDLP_Get_FePolarGpio(void);
/**
 * 设置OTA DDTEX升级时的前端frontend的polar gpio
 *\param void
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FePolarGpio(uint32_t gpio);
/**
 * 获取OTA DDTEX升级时的前端frontend的lnb invert
 *\param void
 *\return 返回lnb invert的值
 *\retval 非-1 成功，返回的lnb invert值有效
 *\retval -1 失败
 */
int32_t GxDLP_Get_FeLnbInvert(void);
/**
 * 设置OTA DDTEX升级时的前端frontend的lnb invert
 *\param void
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeLnbInvert(uint32_t lnb_invert);

/**
 * 获取OTA DDTEX升级时的前端frontend的sat 22k的配置
 *\param type 指向fe_sec_tone_mode_t枚举的指针 gxdocref fe_sec_tone_mode_t
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Get_FeSat22k(fe_sec_tone_mode_t *type);
/**
 * 获取OTA DDTEX升级时的前端frontend的diseqc10_port配置
 *\param void
 *\return 返回diseqc10_port的值
 */
int32_t GxDLP_Get_FeDiseqc10_Port(void);
/**
 * 获取OTA DDTEX升级时的前端frontend的diseqc11_port配置
 *\param void
 *\return 返回diseqc11_port的值
 */
int32_t GxDLP_Get_FeDiseqc11_Port(void);
/**
 * 设置OTA DDTEX升级时前端frontend锁定超时时间,单位:秒
 *\param value 字符串形式的升级重试次数,如十进制形式10,value应为"10"，十六进制形式0xA, value应为"0xA"
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeRepeatTimes(char* value);
/**
 * 设置OTA DDTEX升级时的前端frontend的类型,如DVB-S2,DVB-T
 *\param type 前段frontend的类型 gxdocref fe_type_t
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeType(fe_type_t type);
/**
 * 设置OTA DDTEX升级时的前端frontend的tuner类型
 *\param type 前段frontend的tuner类型 gxdocref GxDLP_Tuner_Type
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeTunerType(GxDLP_Tuner_Type type);
/**
 * 通过 attach 设置OTA DDTEX升级时的前端frontend的tuner类型
 *\param tuner_attach 见 gxfrontend/include/tuner.h
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeTunerTypeByAttach(uint32_t tuner_attach);
/**
 * 设置OTA DDTEX升级时的前端frontend的demod类型
 *\param type 前段frontend的demod类型 gxdocref GxDLP_Demod_Type
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeDemodType(GxDLP_Demod_Type type);
/**
 * 通过 attach 设置设置OTA DDTEX升级时的前端frontend的demod类型
 *\param demod_attach 见 gxfrontend/include/demod.h
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeDemodTypeByAttach(uint32_t demod_attach);
/**
 * 设置OTA DDTEX升级时的前端frontend的symbolrate配置
 *
 * - unit:Kbps
 *
 *\param value 字符串形式的symbolrate值,如十进制形式27500,value应为"27500"，十六进制形式0x6b6c, value应为"0x6b6c"
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeSymbolrate(char* value);
/**
 * 设置OTA DDTEX升级时的前端frontend的frequency配置
 *
 * - unit:KHz
 *
 *\param value 字符串形式的frequency值,如十进制形式1150,value应为"1150"，十六进制形式0x47e, value应为"0x47e"
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeFrequency(char* value);
/**
 * 设置OTA DDTEX升级时的前端frontend的modulation配置
 *\param type 前段frontend的modulation配置 gxdocref fe_modulation_t
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeModulation(fe_modulation_t type);
/**
 * 设置OTA DDTEX升级时的前端frontend的bandwidth配置
 *\param type 前段frontend的bandwidth配置 gxdocref fe_bandwidth_t
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeBandwidth(fe_bandwidth_t type);
/**
 * 设置OTA DDTEX升级时的前端frontend的polar配置
 *\param polar 前段frontend的polar配置 gxdocref fe_sec_voltage_t
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FePolarity(fe_sec_voltage_t polar);
/**
 * 设置OTA DDTEX升级时的前端frontend的polar invert配置
 *\param value 字符串形式的polar invert值,如十进制形式1,value应为"1"，十六进制形式0x1, value应为"0x1"
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FePolarInvert(char* value);
/**
 * 设置OTA DDTEX升级时的前端frontend的sat 22k配置
 *\param type 前段frontend的sat 22k配置 gxdocref fe_sec_tone_mode_t
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeSat22k(fe_sec_tone_mode_t type);
/**
 * 设置OTA DDTEX升级时的前端frontend的diseqc10_port配置
 *\param port 前段frontend的diseqc10_port配置
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeDiseqc10_Port(unsigned int port);
/**
 * 设置OTA DDTEX升级时的前端frontend的diseqc11_port配置
 *\param port 前段frontend的diseqc11_port配置
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_FeDiseqc11_Port(unsigned int port);
/**
 * 设置网络TFTP升级时的端口号配置
 *\param tftp_port 端口号, 字符串形式传入，如十进制形式69,value应为"69"，十六进制形式0x45, value应为"0x45"
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_TFTP_PORT(char* tftp_port);
/**
 * 获取网络TFTP升级时的端口号
 *\param void
 *\return 返回TFTP端口
 */
int32_t GxDLP_Get_TFTP_PORT(void);
/**
 * 设置网络TFTP升级时的设备的ip地址
 *\param ip ip地址, 字符串形式传入，如192.168.1.78
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_HOST_IP_ADDR(char* Value);
/**
 * 获取网络TFTP升级时的设备的ip地址
 *\param void
 *\return 返回ip地址
 */
char   *GxDLP_Get_HOST_IP_ADDR(void);
/**
 * 设置网络TFTP升级时的服务器的ip地址
 *\param ip ip地址, 字符串形式传入，如192.168.1.79
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_SERVER_IP_ADDR(char* Value);
/**
 * 获取网络TFTP升级时的服务器的ip地址
 *\param void
 *\return 返回ip地址
 */
char   *GxDLP_Get_SERVER_IP_ADDR(void);
/**
 * 获取OTA DDTEX升级时的强制升级标志，1: 忽略软件版本号，只要硬件版本号和厂商ID
 * 一样就可以升级 其他值: 硬件版本号和厂商ID一样的情况下，接收到的升级包的软件版
 * 本号必须大于新的软件版本号才可以升级
 *\param void
 *\return 返回强制升级标志
 */
int32_t GxDLP_Get_ForceUpgradeFLag(void);
/**
 * 设置OTA DDTEX升级时的强制升级标志
 *\param value 强制升级标志，字符串形式传入，如十进制形式1,value应为"1"，十六进制形式0x1, value应为"0x1"
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_ForceUpgradeFlag(char* value);
/**
 * 获取当前各个分区的状态, 返回的值是多个字节组成的字符串，每个字节表示一个分区的状态，其中'0'表示该分区正常， '1'表示该分区正在升级,已经被擦掉或者正在写入新的数据
 *\param void
 *\return 返回当前各个分区的状态
 */
char   *GxDLP_Get_PartitionStatus(void);
/**
 * 设置当前各个分区的状态
 *\param partition_status 分区状态，字符串形式传入,如"00000"，表示只有5个分区，都是正常状态, "00100"表示第三个分区在升级状态
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_PartitionStatus(const char* partition_status);
/**
 * 获取 WIFI 的BSSID,BSSID即无线路由的MAC地址
 *\param void
 *\return 返回const char *指向WIFI_BSSID的指针
 *\retval 非NULL 成功
 *\retval NULL 失败
 */
const char *GxDLP_Get_WIFI_BSSID(void);

/**
 * 设置 WIFI的BSSID，BSSID即无线路由的MAC地址
 *\param wifi_bssid 无线路由的MAC地址
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_WIFI_BSSID(const char *wifi_bssid);

/**
 * 获取 WIFI 的PSK即无线路由的密码
 *\param void
 *\return 返回const char *指向WIFI_PSK的指针
 *\retval 非NULL 成功
 *\retval NULL 失败
 */
const char *GxDLP_Get_WIFI_PSK(void);

/**
 * 设置 WIFI的PSK即无线路由的密码
 *\param wifi_psk 无线路由的密码
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_WIFI_PSK(const char *wifi_psk);
/**
 * 获取下载升级文件的url地址
 *\param void
 *\return 返回const char *指向url的指针
 *\retval 非NULL 成功
 *\retval NULL 失败
 */
const char *GxDLP_Get_URL(void);

/**
 * 设置下载升级文件的url地址
 *\param url
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_URL(const char *url);
/**
 * 获取 wifi 连接次数
 *
 *\param void
 *\return 返回 wifi 连接次数
 */
int32_t GxDLP_Get_WIFIConnectTimes(void);
/**
 * 设置 wifi 连接次数
 *\param times 次数
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_WIFIConnectTimes(int32_t times);
/**
 * 获取 http 下载次数
 *
 *\param void
 *\return 返回 http 下载次数
 */
int32_t GxDLP_Get_HTTPGetTimes(void);
/**
 * 设置 http 下载次数
 *\param times 次数
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_HTTPGetTimes(int32_t times);

/**
 * 获取升级需要配置的最高优先级网络设备类型
 * - 如果没有设置则按照默认优先级
 * - 默认优先级按照 gxdocref GxDLP_Net_Device_Type 枚举的排列顺序
 *\param type 指向fe_type_t枚举的指针
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Set_HighestPriNetDevice(GxDLP_Net_Device_Type type);
/**
 * 设置升级需要配置的最高优先级网络设备类型，如果没有设置按照默认优先级
 * - 如果没有设置则按照默认优先级
 * - 默认优先级按照 gxdocref GxDLP_Net_Device_Type 枚举的排列顺序
 *\param type 指向 GxDLP_Net_Device_Type 枚举的指针 gxdocref GxDLP_Net_Device_Type
 *\return 返回函数执行是否成功
 *\retval 0 成功
 *\retval -1 失败
 */
int32_t GxDLP_Get_HighestPriNetDevice(GxDLP_Net_Device_Type *type);
#endif

/** @}*/
