#include "common/gxoem.h"
#include "frontend/frontend.h"
#include "gxdlp_api.h"
#include "ini.h"
#include "common.h"
#include "common/libini.h"
#include "av/gxav_vout_propertytypes.h"
#ifdef NO_OS
#include "minifs/api_minifs.h"
#endif
#include "dlp_internal.h"
#include "config.h"
#include "frontend/frontend.h"
#include "frontend/demod.h"
#include "frontend/tuner.h"

/*以下为OEM中的信息，需要在ini文件中添加选项
用于OTA loader 等跟升级相关的模块交互信息*/

#define OEM_SYSTEM_SECTION                      ("system")
#define OEM_HARDWARE_VERSION                    ("hardware_version")
#define OEM_MANUFACTURE_ID                      ("manufacture_id")
#define OEM_OPERATOR_ID                         ("operator_id")
#define OEM_OUI                                 ("oui")
#define OEM_PLATFORM_ID                         ("platform_Id")
#define OEM_SERIAL_NUMBER                       ("serial_number")
#define OEM_CHIP_ID                             ("chip_id")

#define OEM_SOFTWARE_SECTION                    ("software")
#define OEM_LANGUAGE                            ("language")
#define OEM_APP_VERSION                         ("application_version")
#define OEM_APP_UPDATEVERSION                   ("application_update_version")

#define OEM_UPDATE_SECTION                      ("update")
#define OEM_UPDATE_TYPE                         ("type")
#define OEM_USB_PARTITION_NAME                  ("partition_name")
#define OEM_DMX_ID                              ("dmx_id")
#define OEM_DMX_TIMEOUT                         ("dmx_timeout")
#define OEM_DMX_SOURCE                          ("dmx_source")
#define OEM_DMX_PID                             ("dmx_pid")
#define OEM_DMX_TABLEID                         ("dmx_tableid")
#define OEM_OTA_DDTEX_DOWN_TIMES                ("ota_ddtex_down_times")
#define OEM_OTA_REPEAT_TIMES                    ("ota_repeat_times")
#define OEM_ERASEWIRTE_FLAG                     ("erase_writeflag")
#define OEM_FE_REPEAT_TIMES                     ("fe_repeat_times")
#define OEM_FE_TYPE                             ("fe_type")
#define OEM_FE_SYMBOLRATE                       ("fe_symbolrate")
#define OEM_FE_FREQUENCY                        ("fe_frequency")
#define OEM_FE_MODULATION                       ("fe_modulation")
#define OEM_FE_BANDWIDTH                        ("fe_bandwidth")
#define OEM_FE_POLARITY                         ("fe_polar")
#define OEM_FE_POLARITY_INVERT                  ("fe_polar_invert")
#define OEM_FE_POLARITY_GPIO                    ("fe_polar_gpio")
#define OEM_FE_LNB_INVERT                       ("fe_lnb_invert")
#define OEM_FE_LNBPOWER                         ("fe_lnbpower")
#define OEM_FE_22K                              ("fe_22k")
#define OEM_FE_DISEQC10_PORT                    ("fe_diseqc10_port")
#define OEM_FE_DISEQC11_PORT                    ("fe_diseqc11_port")
#define OEM_FE_TUNER_TYPE                       ("fe_tuner_type")
#define OEM_FE_TUNER_DEV_ID                     ("fe_tuner_dev_id")
#define OEM_FE_TUNER_I2C_ADDR                   ("fe_tuner_i2c_addr")
#define OEM_FE_TUNER_OSC_FRE                    ("fe_tuner_osc_fre")
#define OEM_FE_TUNER_XTAL_CAP                   ("fe_tuner_xtal_cap")
#define OEM_FE_DEMOD_TYPE                       ("fe_demod_type")
#define OEM_FE_DEMOD_DEV_ID                     ("fe_demod_dev_id")
#define OEM_FE_DEMOD_I2C_ADDR                   ("fe_demod_i2c_addr")
#define OEM_FE_DEMOD_TS_MODE                    ("fe_demod_ts_mode")
#define OEM_FE_DEMOD_IQ_MODE                    ("fe_demod_iq_mode")
#define OEM_FE_DEMOD_IS_OSCILLATOR              ("fe_demod_is_oscillator")
#define OEM_FE_DEMOD_TS_PIN                     ("fe_demod_ts_pin")
#define OEM_FE_DEMOD_TS_PINMAP                  ("fe_demod_ts_pinmap")
#define OEM_TFTP_SERVER_IP_ADDR                 ("tftp_server_ip_addr")
#define OEM_TFTP_PORT                           ("tftp_port")
#define OEM_TFTP_HOST_IP_ADDR                   ("tftp_host_ip_addr")
#define OEM_UPGRADE_PATCH_FILE_NAME             ("upgrade_patch_file_name")
#define OEM_FORCE_UPGRADE_FLAG                  ("force_upgrade_flag")

#define OEM_APP_SECTION                         ("app")
#define OEM_TV_STANDARD                         ("tv_standard")

#define OEM_DEBUG_SECTION                       ("debug")
#define OEM_USB_PRINT                           ("usb_print_debug")

#define OEM_PARTITION_STATUS                    ("partition_status")

#define OEM_WIFI_BSSID                          ("wifi_bssid")
#define OEM_WIFI_PSK                            ("wifi_psk")
#define OEM_WIFI_CONNECT_TIMES                  ("wifi_connnect_times")
#define OEM_URL                                 ("url")
#define OEM_HTTP_GET_TIMES                      ("http_get_times")

#define OEM_NET_DEVICE_TYPE                     ("net_device_type")

#define UINT32_STRING_LENGTH                    10 //32bit整形最大值字符串表示需要10个字节

struct modulation_struct {
	const char * fe_mod_type_str;
	fe_modulation_t fe_mod_type;
};

struct fe_type_struct {
	const char * fe_type_str;
	fe_type_t fe_type;
};

struct Sat22k_table_struct {
	const char * Sat22k_str;
	fe_sec_tone_mode_t Sat22k;
};

struct Polar_table_struct {
	const char * Polar_str;
	fe_sec_voltage_t Polar;
};

struct ts_mode_table_struct {
	const char *ts_mode_str;
	ts_mode_t mode;
};

struct ts_pin_table_struct {
	const char *ts_pin_str;
	ts_pin_t ts_pin;
};

struct net_device_type_table_struct {
	const char *net_device_type_str;
	GxDLP_Net_Device_Type net_dev_type;
};

/* 以下为全局的索引值*/
static char *tv_mode[] = {"","GXAV_VOUT_PAL", "GXAV_VOUT_PAL_M", "GXAV_VOUT_PAL_N", "GXAV_VOUT_PAL_NC",
			 "GXAV_VOUT_NTSC_M", "GXAV_VOUT_NTSC_443", "GXAV_VOUT_480I", "GXAV_VOUT_480P",
			 "GXAV_VOUT_576I", "GXAV_VOUT_576P", "GXAV_VOUT_720P_50HZ", "GXAV_VOUT_720P_60HZ",
			 "GXAV_VOUT_1080I_50HZ", "GXAV_VOUT_1080P_50HZ", "GXAV_VOUT_1080I_60HZ", "GXAV_VOUT_1080P_60HZ"
};
static char *update_type[] = {"BOOT", "OTA_FAILED", "OTA_DDT", "OTA_DDTEX", "USB", "NET_TFTP", "NET_HTTP", "UART", "RECOVERY"};

//static char *chip_type[] = {"gx3201", "gx3113c", "gx3115", "gx3113h", "gx3201h", "gx3212"};

static char *language_type[] = { "Chinese" , "English", "Uygur"};
static char *demux_type[] = {"DEMUX_TS1", "DEMUX_TS2", "DEMUX_TS3"};
static struct modulation_struct modulation_table[] =
{
	{"DVBC_16QAM", DVBC_16QAM},
	{"DVBC_32QAM", DVBC_32QAM},
	{"DVBC_64QAM", DVBC_64QAM},
	{"DVBC_128QAM", DVBC_128QAM},
	{"DVBC_256QAM", DVBC_256QAM}
};
static struct fe_type_struct fe_type_table[] =
{
	{"DVB_S",   FE_DVB_S},
	{"DVB_S2",  FE_DVB_S2},
	{"DVB_C",   FE_DVB_C},
	{"DVB_C2",  FE_DVB_C2},
	{"DVB_T",   FE_DVB_T},
	{"DVB_T2",  FE_DVB_T2},
	{"DTMB",    FE_DTMB},
	{"ATSC",    FE_ATSC},
	{"ABS_S",   FE_ABS_S},
	{"DIRECTV", FE_DIRECTV},
	{"ISDB_T",  FE_ISDB_T},
	{"J83B",    FE_J83B}
};

static struct Sat22k_table_struct Sat22k_table[] =
{
	{"ON", SEC_TONE_ON},
	{"OFF", SEC_TONE_OFF}
};
static struct Polar_table_struct Polar_table[] =
{
	{"13V", SEC_VOLTAGE_13},
	{"18V", SEC_VOLTAGE_18},
	{"OFF", SEC_VOLTAGE_OFF}
};

static struct ts_mode_table_struct ts_mode_table[] =
{
	{"PARALLEL", PARALLEL},
	{"SERIAL", SERIAL},
	{"OTHER", OTHER_TS_MODE}
};

#define TS_PIN_NUMBER 12
struct ts_pin_table_struct ts_pin_table[] =
{
	{"DATA0"  , DATA0},
	{"DATA1"  , DATA1},
	{"DATA2"  , DATA2},
	{"DATA3"  , DATA3},
	{"DATA4"  , DATA4},
	{"DATA5"  , DATA5},
	{"DATA6"  , DATA6},
	{"DATA7"  , DATA7},
	{"CLK"    , CLK  },
	{"VALID"  , VALID},
	{"SYNC"   , SYNC },
	{"ERROR"  , ERROR},
	{"DATAS"  , DATAS},
	{"VALIDS" , VALIDS},
	{"SYNCS"  , SYNCS},
	{"ERRS"   , ERRS}
};

static struct net_device_type_table_struct net_device_type_table[] =
{
	{"ETH", GXDLP_NET_DEVICE_ETH},
	{"USBETH", GXDLP_NET_DEVICE_USBETH},
	{"WIFI", GXDLP_NET_DEVICE_WIFI}
};

static char *bandwidth_type[] = {"BANDWIDTH_8_MHZ", "BANDWIDTH_7_MHZ", "BANDWIDTH_6_MHZ"};

#define TUNER_ARRAY_SIZE sizeof(tuner_array)/sizeof(struct tuner)
struct tuner tuner_array[] = {
#ifdef CONFIG_TUNER_AV2018
	{GXDLP_TUNER_AV2018       , "TUNER_AV2018"       , AV2018_ATTACH       , 0xC0},
#endif
#ifdef CONFIG_TUNER_AV2020
	{GXDLP_TUNER_AV2020       , "TUNER_AV2020"       , AV2020_ATTACH       , 0xC0},
#endif
#ifdef CONFIG_TUNER_MXL608_DTMB
	{GXDLP_TUNER_MXL608_DTMB  , "TUNER_MXL608_DTMB"  , MXL608_DTMB_ATTACH  , 0xC0},
#endif
#ifdef CONFIG_TUNER_MXL608_DVBT
	{GXDLP_TUNER_MXL608_T     , "TUNER_MXL608_T"     , MXL608_T_ATTACH     , 0xC0},
#endif
#ifdef CONFIG_TUNER_MXL608_DVBC
	{GXDLP_TUNER_MXL608_C     , "TUNER_MXL608_C"     , MXL608_C_ATTACH     , 0xC0},
#endif
#ifdef CONFIG_TUNER_TDA18250
	{GXDLP_TUNER_TDA18250     , "TUNER_TDA18250"     , TDA18250_ATTACH     , 0xC0},
#endif
#ifdef CONFIG_TUNER_TDA18250A
	{GXDLP_TUNER_TDA18250A    , "TUNER_TDA18250A"    , TDA18250A_ATTACH    , 0xC0},
#endif
#ifdef CONFIG_TUNER_TDA18273_DTMB
	{GXDLP_TUNER_TDA18273_DTMB, "TUNER_TDA18273_DTMB", TDA18273_DTMB_ATTACH, 0xC0},
#endif
#ifdef CONFIG_TUNER_TDA18273_DVBC
	{GXDLP_TUNER_TDA18273_DVBC, "TUNER_TDA18273_DVBC", TDA18273_DVBC_ATTACH, 0xC0},
#endif
#ifdef CONFIG_TUNER_R850_R836_DVBT
	{GXDLP_TUNER_R836_T       , "TUNER_R836_T"       , R836_T_ATTACH       , 0xF8},
#endif
#ifdef CONFIG_TUNER_R850_R836_DVBC
	{GXDLP_TUNER_R836_C       , "TUNER_R836_C"       , R836_C_ATTACH       , 0xF8},
#endif
#ifdef CONFIG_TUNER_R850_R836_DTMB
	{GXDLP_TUNER_R836_DTMB    , "TUNER_R836_DTMB"    , R836_DTMB_ATTACH    , 0xF8},
#endif
#ifdef CONFIG_TUNER_R910_DTMB
	{GXDLP_TUNER_R910_T       , "TUNER_R910_T"       , R910_T_ATTACH       , 0x38},
#endif
#ifdef CONFIG_TUNER_R910_DVBC
	{GXDLP_TUNER_R910_C       , "TUNER_R910_C"       , R910_C_ATTACH       , 0x38},
#endif
#ifdef CONFIG_TUNER_SI2141_DTMB
	{GXDLP_TUNER_SI2141_DTMB  , "TUNER_SI2141_DTMB"  , SI2141_DTMB_ATTACH  , 0xC0},
#endif
#ifdef CONFIG_TUNER_SI2141_DVBC
	{GXDLP_TUNER_SI2141_C     , "TUNER_SI2141_C"     , SI2141_C_ATTACH     , 0xC0},
#endif
#ifdef CONFIG_TUNER_SI2141_DVBT
	{GXDLP_TUNER_SI2141_T     , "TUNER_SI2141_T"     , SI2141_T_ATTACH     , 0xC0},
#endif
#ifdef CONFIG_TUNER_R848_DVBS
	{GXDLP_TUNER_R848_S       , "TUNER_R848_S"       , R848_S_ATTACH       , 0xF8},
#endif
#ifdef CONFIG_TUNER_R848_DVBC
	{GXDLP_TUNER_R848_C       , "TUNER_R848_C"       , R848_C_ATTACH       , 0xF8},
#endif
#ifdef CONFIG_TUNER_R848_DVBT
	{GXDLP_TUNER_R848_T       , "TUNER_R848_T"       , R848_T_ATTACH       , 0xF8},
#endif
#ifdef CONFIG_TUNER_R848_DTMB
	{GXDLP_TUNER_R848_DTMB    , "TUNER_R848_DTMB"    , R848_DTMB_ATTACH    , 0xF8},
#endif
#ifdef CONFIG_TUNER_RDA5815M
	{GXDLP_TUNER_RDA5815M     , "TUNER_RDA5815M"     , RDA5815M_ATTACH     , 0x18},
#endif
#ifdef CONFIG_TUNER_ATBM2040
	{GXDLP_TUNER_ATBM2040     , "TUNER_ATBM2040"     , ATBM2040_ATTACH     , 0xC0},
#endif
#ifdef CONFIG_TUNER_ATBM2030
	{GXDLP_TUNER_ATBM2030     , "TUNER_ATBM2030"     , ATBM2030_ATTACH     , 0xC0},
#endif
#ifdef CONFIG_TUNER_SHARP7306
	{GXDLP_TUNER_SHARP7306    , "TUNER_SHARP7306"    , SHARP7306_ATTACH    , 0xC0},
#endif
#ifdef CONFIG_TUNER_ATBM253_DTMB
	{GXDLP_TUNER_ATBM253_DTMB , "TUNER_ATBM253_DTMB" , ATBM253_DTMB_ATTACH , 0xC0},
#endif
#ifdef CONFIG_TUNER_ATBM253_DVBT
	{GXDLP_TUNER_ATBM253_DVBT , "TUNER_ATBM253_DVBT" , ATBM253_DVBT_ATTACH , 0xC0},
#endif
#ifdef CONFIG_TUNER_ATBM253_DVBC
	{GXDLP_TUNER_ATBM253_DVBC , "TUNER_ATBM253_DVBC" , ATBM253_DVBC_ATTACH , 0xC0},
#endif
#ifdef CONFIG_TUNER_RDA5880U_DTMB
	{GXDLP_TUNER_RDA5880U_DTMB, "TUNER_RDA5880U_DTMB", RDA5880U_DTMB_ATTACH, 0xC0},
#endif
#ifdef CONFIG_TUNER_RDA5880U_DVBT
	{GXDLP_TUNER_RDA5880U_T   , "TUNER_RDA5880U_T"   , RDA5880U_T_ATTACH   , 0xC0},
#endif
#ifdef CONFIG_TUNER_RDA5880U_DVBC
	{GXDLP_TUNER_RDA5880U_C   , "TUNER_RDA5880U_C"   , RDA5880U_C_ATTACH   , 0xC0},
#endif
#ifdef CONFIG_TUNER_ATBM2040_DVBT
	{GXDLP_TUNER_ATBM2040_DVBT, "TUNER_ATBM2040_DVBT", ATBM2040_DVBT_ATTACH, 0xC0},
#endif
#ifdef CONFIG_TUNER_ATBM2040_DVBC
	{GXDLP_TUNER_ATBM2040_DVBC, "TUNER_ATBM2040_DVBC", ATBM2040_DVBC_ATTACH, 0xC0},
#endif
#ifdef CONFIG_TUNER_ATBM2040_DTMB
	{GXDLP_TUNER_ATBM2040_DTMB, "TUNER_ATBM2040_DTMB", ATBM2040_DTMB_ATTACH, 0xC0},
#endif
#ifdef CONFIG_TUNER_TS6011
	{GXDLP_TUNER_TS6011       , "TUNER_TS6011"       , TS6011_ATTACH       , 0x58},
#endif
#ifdef CONFIG_TUNER_M3031
	{GXDLP_TUNER_M3031        , "TUNER_M3031"        , M3031_ATTACH        , 0x40},
#endif
#ifdef CONFIG_TUNER_RDA5160_DTMB
	{GXDLP_TUNER_RDA5160_DTMB , "TUNER_RDA5160_DTMB" , RDA5160_DTMB_ATTACH , 0xC0},
#endif
#ifdef CONFIG_TUNER_RDA5160_DVBC
	{GXDLP_TUNER_RDA5160_C    , "TUNER_RDA5160_C"    , RDA5160_C_ATTACH    , 0xC0},
#endif
#ifdef CONFIG_TUNER_RDA5160_DVBT
	{GXDLP_TUNER_RDA5160_T    , "TUNER_RDA5160_T"    , RDA5160_T_ATTACH    , 0xC0},
#endif
#ifdef CONFIG_TUNER_RT720
	{GXDLP_TUNER_RT720        , "TUNER_RT720"        , RT720_ATTACH        , 0x34},
#endif
#ifdef CONFIG_TUNER_MTV600
	{GXDLP_TUNER_MTV600       , "TUNER_MTV600"       , MTV600_ATTACH       , 0x34},
#endif
#ifdef CONFIG_TUNER_A3500
	{GXDLP_TUNER_A3500        , "TUNER_A3500"        , A3500_ATTACH        , 0x34},
#endif
#ifdef CONFIG_TUNER_R858_DVBC_1ST
	{GXDLP_TUNER_R858_DVBC_1ST        , "TUNER_R858_DVBC_1ST"        , R858_DVBC_1ST_ATTACH        , 0x34},
#endif
#ifdef CONFIG_TUNER_R858_DVBC_2ND
	{GXDLP_TUNER_R858_DVBC_2ND        , "TUNER_R858_DVBC_2ND"        , R858_DVBC_2ND_ATTACH        , 0x34},
#endif
};

#define DEMOD_ARRAY_SIZE sizeof(demod_array)/sizeof(struct demod)
struct demod demod_array[] = {
#ifdef CONFIG_DEMOD_GX1121D
	{GXDLP_DEMOD_GX1121D            , "DEMOD_GX1121D"            , GX1121D_ATTACH            , 0xC0},
#endif
#ifdef CONFIG_DEMOD_GX3211
	{GXDLP_DEMOD_GX113X             , "DEMOD_GX113X"             , GX113X_ATTACH             , 0xC0},
	{GXDLP_DEMOD_GX3211             , "DEMOD_GX3211"             , GX3211_ATTACH             , 0xE4},
	/* There is no GX6605S_DVBs_ATTACH, use GX3211_ATTACH */
	{GXDLP_DEMOD_GX6605S_DVBS       , "DEMOD_GX6605S_DVBS"       , GX3211_ATTACH             , 0xE4},
#endif
#ifdef CONFIG_DEMOD_GX1001
	{GXDLP_DEMOD_GX1001             , "DEMOD_GX1001"             , GX1001_ATTACH             , 0xC0},
#endif
#ifdef CONFIG_DEMOD_GX1503B_DVBC
	{GXDLP_DEMOD_GX1503B_DVBC       , "DEMOD_GX1503B_DVBC"       , GX1503B_DVBC_ATTACH       , 0x60},
#endif
#ifdef CONFIG_DEMOD_GX1503B_DTMB
	{GXDLP_DEMOD_GX1503B_DTMB       , "DEMOD_GX1503B_DTMB"       , GX1503B_DTMB_ATTACH       , 0xC0},
#endif
#ifdef CONFIG_DEMOD_GX3211_DVBC
	{GXDLP_DEMOD_GX3211_DVBC        , "DEMOD_GX3211_DVBC"        , GX3211_DVBC_ATTACH        , 0xC0},
#endif
#ifdef CONFIG_DEMOD_ATBM78X_DVBT
	{GXDLP_DEMOD_ATBM78X            , "DEMOD_ATBM78X"            , ATBM78X_ATTACH            , 0x80},
#endif
#ifdef CONFIG_DEMOD_ATBM888X_DVBC
	{GXDLP_DEMOD_ATBM888X_DVBC      , "DEMOD_ATBM888X_DVBC"      , ATBM888X_DVBC_ATTACH      , 0xC0},
#endif
#ifdef CONFIG_DEMOD_ATBM888X_DTMB
	{GXDLP_DEMOD_ATBM888X_DTMB      , "DEMOD_ATBM888X_DTMB"      , ATBM888X_DTMB_ATTACH      , 0x80},
#endif
#ifdef CONFIG_DEMOD_ATBM783X_DVBC
	{GXDLP_DEMOD_ATBM783X_DVBC      , "DEMOD_ATBM783X_DVBC"      , ATBM783X_DVBC_ATTACH      , 0xC0},
#endif
#ifdef CONFIG_DEMOD_ATBM783X_DVBS
	{GXDLP_DEMOD_ATBM783X_DVBS      , "DEMOD_ATBM783X_DVBS"      , ATBM783X_DVBS_ATTACH      , 0x80},
#endif
#ifdef CONFIG_DEMOD_GX1133
	{GXDLP_DEMOD_GX1133             , "DEMOD_GX1133"             , GX1133_ATTACH             , 0xB4},
#endif
#ifdef CONFIG_DEMOD_Sirius_DVBS
	{GXDLP_DEMOD_Sirius_DVBS        , "DEMOD_Sirius_DVBS"        , Sirius_DVBS_ATTACH        , 0xA4},
#endif
#ifdef CONFIG_DEMOD_Sirius_DVBC
	{GXDLP_DEMOD_Sirius_DVBC        , "DEMOD_Sirius_DVBC"        , Sirius_DVBC_ATTACH        , 0xA4},
#endif
#ifdef CONFIG_DEMOD_MXL683
	{GXDLP_DEMOD_MXL683             , "DEMOD_MXL683"             , MXL683_ATTACH             , 0xC0},
#endif
#ifdef CONFIG_DEMOD_Taurus_DVBS
	{GXDLP_DEMOD_Taurus_DVBS        , "DEMOD_Taurus_DVBS"        , Taurus_DVBS_ATTACH        , 0xA4},
#endif
#ifdef CONFIG_DEMOD_Taurus_DVBC
	{GXDLP_DEMOD_Taurus_DVBC        , "DEMOD_Taurus_DVBC"        , Taurus_DVBC_ATTACH        , 0xA4},
#endif
#ifdef CONFIG_DEMOD_GX1135_DVBS
	{GXDLP_DEMOD_GX1135_DVBS        , "DEMOD_GX1135_DVBS"        , GX1135_DVBS_ATTACH        , 0xC0},
#endif
#ifdef CONFIG_DEMOD_Gemini_DVBT
	{GXDLP_DEMOD_Gemini_DVBT        , "DEMOD_Gemini_DVBT"        , Gemini_DVBT_ATTACH        , 0xE4},
#endif
#ifdef CONFIG_DEMOD_Gemini_DVBC
	{GXDLP_DEMOD_Gemini_DVBC        , "DEMOD_Gemini_DVBC"        , Gemini_DVBC_ATTACH        , 0xE4},
#endif
#ifdef CONFIG_DEMOD_Cygnus_DVBT
	{GXDLP_DEMOD_Cygnus_DVBT        , "DEMOD_Cygnus_DVBT"        , Cygnus_DVBT_ATTACH        , 0xE4},
#endif
#ifdef CONFIG_DEMOD_Cygnus_DVBC
	{GXDLP_DEMOD_Cygnus_DVBC        , "DEMOD_Cygnus_DVBC"        , Cygnus_DVBC_ATTACH        , 0xE4},
#endif
#ifdef CONFIG_DEMOD_Sirius_J83B
	{GXDLP_DEMOD_Sirius_J83B        , "DEMOD_Sirius_J83B"        , Sirius_J83B_ATTACH        , 0xC0},
#endif
#ifdef CONFIG_DEMOD_Taurus_J83B
	{GXDLP_DEMOD_Taurus_J83B        , "DEMOD_Taurus_J83B"        , Taurus_J83B_ATTACH        , 0xC0},
#endif
#ifdef CONFIG_DEMOD_Canopus_DVBS
	{GXDLP_DEMOD_Canopus_DVBS       , "DEMOD_Canopus_DVBS"       , Canopus_DVBS_ATTACH       , 0xA4},
#endif
#ifdef CONFIG_DEMOD_Canopus_DVBC
	{GXDLP_DEMOD_Canopus_DVBC       , "DEMOD_Canopus_DVBC"       , Canopus_DVBC_ATTACH       , 0xC0},
#endif
#ifdef CONFIG_DEMOD_Canopus_J83B
	{GXDLP_DEMOD_Canopus_J83B       , "DEMOD_Canopus_J83B"       , Canopus_J83B_ATTACH       , 0xC0},
#endif
#ifdef CONFIG_DEMOD_Canopus_DVBC1
	{GXDLP_DEMOD_Canopus_DVBC1      , "DEMOD_Canopus_DVBC1"      , Canopus_DVBC1_ATTACH      , 0xC0},
#endif
#ifdef CONFIG_DEMOD_ATBM78X_DVBC
	{GXDLP_DEMOD_ATBM78X_DVBC       , "DEMOD_ATBM78X_DVBC"       , ATBM78X_DVBC_ATTACH       , 0x80},
#endif
#ifdef CONFIG_DEMOD_ATBM78X_REPEAT_DVBT
	{GXDLP_DEMOD_ATBM78X_REPEAT_DVBT, "DEMOD_ATBM78X_REPEAT_DVBT", ATBM78X_REPEAT_DVBT_ATTACH, 0xC0},
#endif
#ifdef CONFIG_DEMOD_ATBM78X_REPEAT_DVBC
	{GXDLP_DEMOD_ATBM78X_REPEAT_DVBC, "DEMOD_ATBM78X_REPEAT_DVBC", ATBM78X_REPEAT_DVBC_ATTACH, 0xC0},
#endif
#ifdef CONFIG_DEMOD_Cygnus_FM
	{GXDLP_DEMOD_Cygnus_FM          , "DEMOD_Cygnus_FM"          , Cygnus_FM_ATTACH          , 0xE4},
#endif
#ifdef CONFIG_DEMOD_CXD2856_DVBT
	{GXDLP_DEMOD_CXD2856_DVBT       , "DEMOD_CXD2856_DVBT"       , CXD2856_DVBT_ATTACH       , 0xC8},
#endif
#ifdef CONFIG_DEMOD_CXD2856_DVBC
	{GXDLP_DEMOD_CXD2856_DVBC       , "DEMOD_CXD2856_DVBC"       , CXD2856_DVBC_ATTACH       , 0xC8},
#endif
#ifdef CONFIG_DEMOD_CXD2856_REPEAT_DVBT
	{GXDLP_DEMOD_CXD2856_REPEAT_DVBT, "DEMOD_CXD2856_REPEAT_DVBT", CXD2856_REPEAT_DVBT_ATTACH, 0xC0},
#endif
#ifdef CONFIG_DEMOD_CXD2856_REPEAT_DVBC
	{GXDLP_DEMOD_CXD2856_REPEAT_DVBC, "DEMOD_CXD2856_REPEAT_DVBC", CXD2856_REPEAT_DVBC_ATTACH, 0xC0},
#endif
#ifdef CONFIG_DEMOD_SI2183_DVBT
	{GXDLP_DEMOD_SI2183_DVBT        , "DEMOD_SI2183_DVBT"        , SI2183_DVBT_ATTACH        , 0xC0},
#endif
#ifdef CONFIG_DEMOD_SI2183_DVBC
	{GXDLP_DEMOD_SI2183_DVBC        , "DEMOD_SI2183_DVBC"        , SI2183_DVBC_ATTACH        , 0xC0},
#endif
#ifdef CONFIG_DEMOD_Vega_DVBS
	{GXDLP_DEMOD_Vega_DVBS          , "DEMOD_Vega_DVBS"          , Vega_DVBS_ATTACH          , 0xA4},
#endif
#ifdef CONFIG_DEMOD_Vega_DVBC
	{GXDLP_DEMOD_Vega_DVBC          , "DEMOD_Vega_DVBC"          , Vega_DVBC_ATTACH          , 0xC0},
#endif
#ifdef CONFIG_DEMOD_Vega_J83B
	{GXDLP_DEMOD_Vega_J83B          , "DEMOD_Vega_J83B"          , Vega_J83B_ATTACH          , 0xC0},
#endif
#ifdef CONFIG_DEMOD_Vega_DVBC1
	{GXDLP_DEMOD_Vega_DVBC1         , "DEMOD_Vega_DVBC1"         , Vega_DVBC1_ATTACH         , 0xC0},
#endif
#ifdef CONFIG_DEMOD_Scorpio_DVBC
	{GXDLP_DEMOD_Scorpio_DVBC       , "DEMOD_Scorpio_DVBC"       , Scorpio_DVBC_ATTACH       , 0xA4},
#endif
};

static unsigned int _htoi(const char *str)
{
	const char *cp;
	unsigned int data = 0, bdata = 0;

	for (cp = str, data = 0; *cp != 0; ++cp) {
		if (*cp >= '0' && *cp <= '9')
			bdata = *cp - '0';
		else if (*cp >= 'A' && *cp <= 'F')
			bdata = *cp - 'A' + 10;
		else if (*cp >= 'a' && *cp <= 'f')
			bdata = *cp - 'a' + 10;
		else
			break;
		data = (data << 4) | bdata;
	}

	return data;
}
static unsigned int _atoi(const char *str)
{
	const char *cp;
	unsigned int data = 0;

	if (str[0] == '0' && (str[1] == 'X' || str[1] == 'x'))
		return _htoi(str + 2);

	for (cp = str, data = 0; *cp != 0; ++cp) {
		if (*cp >= '0' && *cp <= '9')
			data = data * 10 + *cp - '0';
		else
			break;
	}

	return data;
}

/*
* xx.xx.xx.xx格式字符串转化成数值
*/
uint32_t convert_version_str(const char *str)
{
	const char *cp;
	unsigned int data, bdata;

	for (cp = str, data = 0; *cp != 0; ++cp) {
		if (*cp >= '0' && *cp <= '9')
			bdata = *cp - '0';
		else if (*cp >= 'A' && *cp <= 'F')
			bdata = *cp - 'A' + 10;
		else if (*cp >= 'a' && *cp <= 'f')
			bdata = *cp - 'a' + 10;
		else if ('.' == *cp)
			continue;
		else
			break;
		data = (data << 4) | bdata;
	}

	return data;
}

/*
   将版本数值转换成字符串
   应用显示的格式为00.00.00.01
*/
char *convert_version_num_to_char(uint32_t num)
{
	/* 索引表 */
	static char str[12];
	sprintf(str, "%02x.%02x.%02x.%02x", (num >> 24) & 0xff, (num >> 16) & 0xff, (num >> 8) & 0xff, num & 0xff);

	return str;
}

int32_t GxDLP_Init(char *dlp,char *backup_dlp)
{
	return Ini_Oem_Init(dlp,backup_dlp);
}
/*
   获取机顶盒硬件版本号，用于跟升级流中的编号
   进行对比，确认是否需要升级。两者需要严格一致，才
   会进行升级
   */
uint32_t GxDLP_Get_HardwareVersion(void)
{
	char* str;

	str = Ini_GetValue(OEM_SYSTEM_SECTION, OEM_HARDWARE_VERSION);
	if (str == NULL)
		return -1;

	return convert_version_str(str);
}
/*
   获取机顶盒厂商的编号，用于跟升级流中的编号
   进行对比，确认是否需要升级。两者需要严格一致，才
   会进行升级
   */
uint32_t GxDLP_Get_ManufactureId(void)
{
	char* str;

	str = Ini_GetValue(OEM_SYSTEM_SECTION, OEM_MANUFACTURE_ID);
	if (str == NULL)
		return -1;

	return convert_version_str(str);
}

int32_t GxDLP_Get_OperatorID(uint32_t *operator_id)
{
	char* str;

	if (operator_id == NULL)
		return -2;
	str = Ini_GetValue(OEM_SYSTEM_SECTION, OEM_OPERATOR_ID);
	if (str == NULL)
		return -1;
	*operator_id = convert_version_str(str);

	return 0;
}

int32_t GxDLP_Get_OUI(uint32_t *oui)
{
	char* str;

	if (oui == NULL)
		return -2;
	str = Ini_GetValue(OEM_SYSTEM_SECTION, OEM_OUI);
	if (str == NULL)
		return -1;
	*oui = convert_version_str(str);

	return 0;
}
/*
   获取机顶盒序列号，暂时没用到。可以根据此值圈定升级范围
   机顶盒序列号可能需要生产时写进flash。升级时读取对比。
   此函数需要用户根据协议自行扩展
   */
int32_t GxDLP_Get_SerialNumber(void)
{
	char* str;

	str = Ini_GetValue(OEM_SYSTEM_SECTION, OEM_SERIAL_NUMBER);
	if (str == NULL)
		return -1;

	return _atoi(str);
}

int32_t GxDLP_Get_TvStandard(GxVideoOutProperty_Mode *type)
{
	char *rc_str = NULL;
	int32_t i = 0;

	rc_str =  Ini_GetValue(OEM_APP_SECTION, OEM_TV_STANDARD);
	if (NULL == rc_str)
		return -1;

	for (i = 0; i < GXAV_VOUT_NULL_MAX; i++)
	{
		if (0 == strcmp(rc_str, tv_mode[i]))
		{
			*type = i;
			break;
		}
	}

	return 0;
}

int32_t GxDLP_Set_TvStandard(GxVideoOutProperty_Mode type)
{
	if (GXAV_VOUT_NULL_MAX <= type)
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_APP_SECTION, OEM_TV_STANDARD, tv_mode[type]);
	if(ret == -1)
		gxloge("Ini_SetValue Tv Standard error\n");

	return 0;
}

int32_t GxDLP_Get_Language(GxDLP_Language_Type *type)
{
	char *language = NULL;
	int32_t i = 0;

	language =  Ini_GetValue(OEM_SOFTWARE_SECTION, OEM_LANGUAGE);
	if (NULL ==language)
		return -1;

	for (i = 0; i < GXDLP_LANGUAGE_MAX; i++)
	{
		if (0 == strcmp(language, language_type[i]))
		{
			*type = i;
			break;
		}
	}
	return 0;
}

int32_t GxDLP_Set_Language(GxDLP_Language_Type type)
{
	if (GXDLP_LANGUAGE_MAX <= type)
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_SOFTWARE_SECTION, OEM_LANGUAGE, language_type[type]);
	if(ret == -1)
		gxloge("Ini_SetValue Language error\n");


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
		gxloge("Ini_SetValue Update type by index error\n");

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

int32_t GxDLP_Clear_UpdateType(void)
{
	int32_t i = 0;

	for (i = 0; i < GXDLP_DOWNLOAD_SEQ_MAX; i++) {
		if (GxDLP_Set_UpdateTypeByIndex(GXDLP_NONE_DOWNLOAD, i) != 0)
			return -1;
	}

	return 0;
}

char *GxDLP_Get_UpgradeFileName(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_UPGRADE_PATCH_FILE_NAME);
	if (NULL == str)
		return NULL;

	return str;
}

int32_t GxDLP_Set_UpgradeFileName(char* Value)
{
	if (Value == NULL)
		return -1;

	int32_t ret = 0;
	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_UPGRADE_PATCH_FILE_NAME, Value);
	if(ret == -1)
		gxloge("Ini_SetValue Upgrade file name error\n");


	return 0;
}

int32_t GxDLP_Get_DmxId(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_DMX_ID);
	if (str == NULL)
		return -1;

	return _atoi(str);
}

int32_t GxDLP_Set_Dmxid(char* value)
{
	if (NULL == value)
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_DMX_ID, value);
	if(ret == -1)
		gxloge("Ini_SetValue dmx id error\n");

	return 0;
}
/*
   DEMUX过滤数据的超时值。
   用于没有收到数据时推出OTA,建议值5000-0xffffffff
   码率小时过滤不到数据时，请增加此值尝试
   */
int32_t GxDLP_Get_DmxTimeout(void)
{
	char* str;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_DMX_TIMEOUT);
	if ((str == NULL) || (_atoi(str) < 30))
		return 30; //unit: s

	return _atoi(str);
}

int32_t GxDLP_Set_DmxTimeout(char* value)
{
	if (NULL == value || (_atoi(value) < 30))
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_DMX_TIMEOUT, value);
	if(ret == -1)
		gxloge("Ini_SetValue dmx timeout error\n");

	return 0;
}

int32_t GxDLP_Get_OTADDTEXDownTimes(void)
{
	char* str;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_OTA_DDTEX_DOWN_TIMES);
	if (str == NULL)
		return 0;
	if (_atoi(str) > 3)
		return 3;

	return _atoi(str);
}

int32_t GxDLP_Set_OTADDTEXDownTimes(int32_t times)
{
	int32_t ret = 0;

	char str[UINT32_STRING_LENGTH+1] = {0};

	if (times > 3)
		times = 3;
	snprintf(str, UINT32_STRING_LENGTH+1, "%u", times);

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_OTA_DDTEX_DOWN_TIMES, str);
	if(ret == -1)
		gxloge("Ini_SetValue ota_ddtex_down_times error\n");

	return ret;
}


/* 标注demux使用哪一路ts输入。用于配置OTA属性 */
enum dmx_input GxDLP_Get_DmxSource(void)
{
	char* str;
	int32_t i = 0;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_DMX_SOURCE);
	if (str == NULL)
		return -1;

	for (i = 0; i < DEMUX_SDRAM; i++)
	{
		if (0 == strcmp(str, demux_type[i]))
		{
			return i;
		}
	}

	return -1;
}
int32_t GxDLP_Set_DmxSource(enum dmx_input type)
{
	if (DEMUX_SDRAM <= type)
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_DMX_SOURCE, demux_type[type]);
	if(ret == -1)
		gxloge("Ini_SetValue dmx source error\n");


	return 0;
}
/*
   获取承载升级数据的pid。
   可以默认值，也可以根据升级描述符获取
   */
int32_t GxDLP_Get_DmxPid(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_DMX_PID);
	if (str == NULL)
		return -1;

	return _atoi(str);
}
int32_t GxDLP_Set_DmxPid(char* value)
{
	if (NULL == value)
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_DMX_PID, value);
	if(ret == -1)
		gxloge("Ini_SetValue dmx pid error\n");


	return 0;
}

/*
   获取承载升级数据的table id。
   可以默认值，也可以根据升级描述符获取
   */
int32_t GxDLP_Get_DmxTableId(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_DMX_TABLEID);
	if (str == NULL)
		return -1;
	return _atoi(str);
}
int32_t GxDLP_Set_DmxTableId(char* value)
{
	if (NULL == value)
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_DMX_TABLEID, value);
	if(ret == -1)
		gxloge("Ini_SetValue dmx table id error\n");


	return 0;
}

/*
   ota升级失败重复进入的次数，如果重复次数超过此值
   将不进行此次升级。等待下次设置或者检测到升级描述符
   */
int32_t GxDLP_Get_OtaRepeatTimes(void)
{
	char* str;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_OTA_REPEAT_TIMES);
	if (str == NULL)
		return 0;

	return _atoi(str);
}
int32_t GxDLP_Set_OtaRepeatTimes(char *value)
{
	if (NULL == value)
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_OTA_REPEAT_TIMES, value);
	if(ret == -1)
		gxloge("Ini_SetValue  ota repeat times error\n");

	return 0;
}
/*
   升级失败重复进入的次数，如果重复次数超过此值
   将不进行此次升级。等待下次设置或者检测到升级描述符
   */
int32_t GxDLP_Get_UpdateRepeatTimes(void)
{
	return GxDLP_Get_OtaRepeatTimes();
}
int32_t GxDLP_Set_UpdateRepeatTimes(int32_t repeat_times)
{
	char str[UINT32_STRING_LENGTH+1] = {0};

	snprintf(str, UINT32_STRING_LENGTH+1, "%u", repeat_times);

	return GxDLP_Set_OtaRepeatTimes(str);
}

int32_t GxDLP_Get_ErasewriteFlag(void)
{
	char* str;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_ERASEWIRTE_FLAG);
	if (str == NULL)
		return -1;

	return _atoi(str);
}
int32_t GxDLP_Set_ErasewriteFlag(char *value)
{
	if (NULL == value)
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_ERASEWIRTE_FLAG, value);
	if(ret == -1)
		gxloge("Ini_SetValue erase write flag error\n");


	return 0;
}
/*
   前端解调查询是否锁定的延时。
   配置这个值，用于调整性能。建议值5000.
   */
int32_t GxDLP_Get_FeRepeatTimes(void)
{
	char* str;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_REPEAT_TIMES);
	if ((str == NULL) || (_atoi(str) < 10))
		return 10;

	return _atoi(str);
}

int32_t GxDLP_Set_FeRepeatTimes(char* value)
{
	if (NULL == value || (_atoi(value) < 10))
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_REPEAT_TIMES, value);
	if(ret == -1)
		gxloge("Ini_SetValue fe repeat times error\n");

	return 0;
}

struct tuner *tuner_name_to_tuner(char *name)
{
	int32_t i = 0;

	for (i = 0; i < TUNER_ARRAY_SIZE; i++)
	{
		if (0 == strcmp(name, tuner_array[i].name))
		{
			return &tuner_array[i];
		}
	}

	return NULL;
}

struct tuner *tuner_type_to_tuner(GxDLP_Tuner_Type type)
{
	int32_t i = 0;

	for (i = 0; i < TUNER_ARRAY_SIZE; i++)
	{
		if (tuner_array[i].type == type)
		{
			return &tuner_array[i];
		}
	}

	return NULL;
}

struct tuner *tuner_attach_to_tuner(uint32_t attach)
{
	int32_t i = 0;

	for (i = 0; i < TUNER_ARRAY_SIZE; i++)
	{
		if ((uint32_t)tuner_array[i].attach == attach)
		{
			return &tuner_array[i];
		}
	}

	return NULL;
}

/*
 * get tuner name
 */
char* get_tunername(GxDLP_Tuner_Type type)
{
	struct tuner *p_tuner = tuner_type_to_tuner(type);

	if (p_tuner == NULL)
		return NULL;

	return (char *)p_tuner->name;
}
/*
 * get tuner type
 */
int32_t GxDLP_Get_FeTunerType(GxDLP_Tuner_Type *type)
{
	char* str;
	struct tuner *p_tuner;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_TUNER_TYPE);
	if (str == NULL)
		return -1;

	p_tuner = tuner_name_to_tuner(str);
	if (p_tuner == NULL)
		return -1;
	*type = p_tuner->type;

	return 0;
}
int32_t GxDLP_Set_FeTunerType(GxDLP_Tuner_Type type)
{
	if (GXDLP_TUNER_MAX <= type)
		return -1;

	int32_t ret = 0;
	struct tuner *p_tuner;

	p_tuner = tuner_type_to_tuner(type);
	if (p_tuner == NULL)
		return -1;
	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_TUNER_TYPE, p_tuner->name);
	if(ret == -1)
		gxloge("Ini_SetValue fe tuner type error\n");

	return 0;
}

int32_t GxDLP_Set_FeTunerTypeByAttach(uint32_t tuner_attach)
{
	int32_t ret = 0;
	struct tuner *p_tuner;

	p_tuner = tuner_attach_to_tuner(tuner_attach);
	if (p_tuner == NULL)
		return -1;
	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_TUNER_TYPE, p_tuner->name);
	if(ret == -1)
		gxloge("Ini_SetValue fe tuner type by attach error\n");

	return 0;
}

int32_t GxDLP_Get_FeTunerDevId(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_TUNER_DEV_ID);
	if (str == NULL)
		return -1;

	return _atoi(str);
}

int32_t GxDLP_Set_FeTunerDevId(uint32_t dev_id)
{
	int32_t ret = 0;
	char str[UINT32_STRING_LENGTH+1] = {0};

	snprintf(str, UINT32_STRING_LENGTH+1, "%u", dev_id);

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_TUNER_DEV_ID, str);
	if(ret == -1)
		gxloge("Ini_SetValue tuner dev id error\n");

	return ret;
}

int32_t GxDLP_Get_FeTunerI2cAddr(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_TUNER_I2C_ADDR);
	if (str == NULL)
		return -1;

	return _atoi(str);
}

int32_t GxDLP_Set_FeTunerI2cAddr(uint32_t addr)
{
	int32_t ret = 0;
	char str[UINT32_STRING_LENGTH+1] = {0};

	snprintf(str, UINT32_STRING_LENGTH+1, "%u", addr);

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_TUNER_I2C_ADDR, str);
	if(ret == -1)
		gxloge("Ini_SetValue tuner i2c addr error\n");

	return ret;
}

int32_t GxDLP_Get_FeTunerOscFre(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_TUNER_OSC_FRE);
	if (str == NULL)
		return -1;

	return _atoi(str);
}

int32_t GxDLP_Set_FeTunerOscFre(uint32_t fre)
{
	int32_t ret = 0;
	char str[UINT32_STRING_LENGTH+1] = {0};

	snprintf(str, UINT32_STRING_LENGTH+1, "%u", fre);

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_TUNER_OSC_FRE, str);
	if(ret == -1)
		gxloge("Ini_SetValue tuner osc fre error\n");

	return ret;
}

int32_t GxDLP_Get_FeTunerXtalCap(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_TUNER_XTAL_CAP);
	if (str == NULL)
		return -1;

	return _atoi(str);
}

int32_t GxDLP_Set_FeTunerXtalCap(int32_t xtal_cap_pf)
{
	int32_t ret = 0;
	char str[UINT32_STRING_LENGTH+1] = {0};

	if (xtal_cap_pf < 0) {
		gxloge("xtal_cap_pf can't < 0\n");
		return -2;
	}
	snprintf(str, UINT32_STRING_LENGTH+1, "%d", xtal_cap_pf);

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_TUNER_XTAL_CAP, str);
	if(ret == -1)
		gxloge("Ini_SetValue tuner xtal cap pf error\n");

	return ret;
}

struct demod *demod_name_to_demod(char *name)
{
	int32_t i = 0;

	for (i = 0; i < DEMOD_ARRAY_SIZE; i++)
	{
		if (0 == strcmp(name, demod_array[i].name))
		{
			return &demod_array[i];
		}
	}

	return NULL;
}

struct demod *demod_type_to_demod(GxDLP_Demod_Type type)
{
	int32_t i = 0;

	for (i = 0; i < DEMOD_ARRAY_SIZE; i++)
	{
		if (demod_array[i].type == type)
		{
			return &demod_array[i];
		}
	}

	return NULL;
}

struct demod *demod_attach_to_demod(uint32_t attach)
{
	int32_t i = 0;

	for (i = 0; i < DEMOD_ARRAY_SIZE; i++)
	{
		if ((uint32_t)demod_array[i].attach == attach)
		{
			return &demod_array[i];
		}
	}

	return NULL;
}

/*
 * get demod name
 */
char* get_demodname(GxDLP_Demod_Type type)
{
	struct demod *p_demod = demod_type_to_demod(type);

	if (p_demod == NULL)
		return NULL;

	return (char *)p_demod->name;
}
/*
 * get demod type
 * 0 -- gx1001, 1 -- gx1503
 */
int32_t GxDLP_Get_FeDemodType(GxDLP_Demod_Type *type)
{
	char* str;
	struct demod *p_demod;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_DEMOD_TYPE);
	if (str == NULL)
		return -1;

	p_demod = demod_name_to_demod(str);
	if (p_demod == NULL)
		return -1;
	*type = p_demod->type;

	return 0;
}
int32_t GxDLP_Set_FeDemodType(GxDLP_Demod_Type type)
{
	if (GXDLP_DEMOD_MAX <= type)
		return -1;

	int32_t ret = 0;
	struct demod *p_demod;

	p_demod = demod_type_to_demod(type);
	if (p_demod == NULL)
		return -1;
	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_DEMOD_TYPE, p_demod->name);
	if(ret == -1)
		gxloge("Ini_SetValue fe demod type error\n");

	return 0;
}

int32_t GxDLP_Set_FeDemodTypeByAttach(uint32_t demod_attach)
{
	int32_t ret = 0;
	struct demod *p_demod;

	p_demod = demod_attach_to_demod(demod_attach);
	if (p_demod == NULL)
		return -1;
	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_DEMOD_TYPE, p_demod->name);
	if(ret == -1)
		gxloge("Ini_SetValue fe demod type by attach error\n");

	return 0;
}

int32_t GxDLP_Get_FeDemodDevId(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_DEMOD_DEV_ID);
	if (str == NULL)
		return -1;

	return _atoi(str);
}

int32_t GxDLP_Set_FeDemodDevId(uint32_t dev_id)
{
	int32_t ret = 0;
	char str[UINT32_STRING_LENGTH+1] = {0};

	snprintf(str, UINT32_STRING_LENGTH+1, "%u", dev_id);

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_DEMOD_DEV_ID, str);
	if(ret == -1)
		gxloge("Ini_SetValue demod dev id error\n");

	return ret;
}

int32_t GxDLP_Get_FeDemodI2cAddr(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_DEMOD_I2C_ADDR);
	if (str == NULL)
		return -1;

	return _atoi(str);
}

int32_t GxDLP_Set_FeDemodI2cAddr(uint32_t addr)
{
	int32_t ret = 0;
	char str[UINT32_STRING_LENGTH+1] = {0};

	snprintf(str, UINT32_STRING_LENGTH+1, "%u", addr);

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_DEMOD_I2C_ADDR, str);
	if(ret == -1)
		gxloge("Ini_SetValue demod i2c addr error\n");

	return ret;
}

int32_t GxDLP_Get_FeDemodTsMode(ts_mode_t *mode)
{
	char* str;
	int32_t i = 0;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_DEMOD_TS_MODE);
	if (str == NULL)
		return -1;

	for (i = 0; i < sizeof(ts_mode_table) / sizeof(struct ts_mode_table_struct); i++)
	{
		if (0 == strcmp(str, ts_mode_table[i].ts_mode_str))
		{
			*mode = ts_mode_table[i].mode;
			break;
		}
	}

	if(i == sizeof(ts_mode_table) / sizeof(struct ts_mode_table_struct))
		return -1;

	return 0;
}

int32_t GxDLP_Set_FeDemodTsMode(ts_mode_t mode)
{
	int32_t ret = 0;
	int32_t i;

	for (i = 0; i < sizeof(ts_mode_table) / sizeof(struct ts_mode_table_struct); i++)
	{
		if (mode == ts_mode_table[i].mode)
		{
			ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_DEMOD_TS_MODE, ts_mode_table[i].ts_mode_str);
			break;
		}
	}

	if(ret == -1 || (i == sizeof(ts_mode_table) / sizeof(struct ts_mode_table_struct))) {
		ret = -1;
		gxloge("Ini_SetValue demod ts mode error\n");
	}

	return ret;
}

int32_t GxDLP_Get_FeDemodIqMode(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_DEMOD_IQ_MODE);
	if (str == NULL)
		return -1;

	return _atoi(str);
}

int32_t GxDLP_Set_FeDemodIqMode(uint32_t mode)
{
	int32_t ret = 0;
	char str[UINT32_STRING_LENGTH+1] = {0};

	snprintf(str, UINT32_STRING_LENGTH+1, "%u", mode);

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_DEMOD_IQ_MODE, str);
	if(ret == -1)
		gxloge("Ini_SetValue demod iq error\n");

	return ret;
}

int32_t GxDLP_Get_FeDemodIsOscillator(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_DEMOD_IS_OSCILLATOR);
	if (str == NULL)
		return -1;

	return _atoi(str);
}

int32_t GxDLP_Set_FeDemodIsOscillator(uint32_t is_oscillator)
{
	int32_t ret = 0;
	char str[UINT32_STRING_LENGTH+1] = {0};

	snprintf(str, UINT32_STRING_LENGTH+1, "%u", is_oscillator);

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_DEMOD_IS_OSCILLATOR, str);
	if(ret == -1)
		gxloge("Ini_SetValue demod is oscillator error\n");

	return ret;
}

int32_t GxDLP_Get_FeDemodTsPin(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_DEMOD_TS_PIN);
	if (str == NULL)
		return -1;

	return _atoi(str);
}

int32_t GxDLP_Set_FeDemodTsPin(uint32_t ts_pin)
{
	int32_t ret = 0;
	char str[UINT32_STRING_LENGTH+1] = {0};

	snprintf(str, UINT32_STRING_LENGTH+1, "%u", ts_pin);

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_DEMOD_TS_PIN, str);
	if(ret == -1)
		gxloge("Ini_SetValue ts pin error\n");

	return ret;
}

#define DEMOD_TS_PINMAP_DELIM "|"
int32_t GxDLP_Get_FeDemodTsPinMap(struct pin_config *demod_pin_map)
{
	char* str;
	int32_t i = 0;
	int32_t j = 0;
	char pin_map_buf[128] = {0};
	char *p_pin_map_buf = pin_map_buf;
	ts_pin_t ts_pin_map[TS_PIN_NUMBER];

	if (demod_pin_map == NULL)
		return -2;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_DEMOD_TS_PINMAP);
	if (str == NULL)
		return -1;

	strncpy(pin_map_buf, str, 128 - 1);

	p_pin_map_buf = pin_map_buf;
	for (i = 0; i < TS_PIN_NUMBER; i++) {
		ts_pin_map[i] = ERRS;
		str = strsep(&p_pin_map_buf, DEMOD_TS_PINMAP_DELIM);
		if (str != NULL) {
			for (j = 0; j < sizeof(ts_pin_table) / sizeof(struct ts_pin_table_struct); j++) {
				if (0 == strcmp(str, ts_pin_table[j].ts_pin_str)) {
					ts_pin_map[i] = ts_pin_table[j].ts_pin;
					break;
				}
			}
			if (j == sizeof(ts_pin_table) / sizeof(struct ts_pin_table_struct)) {
				gxloge("ts pin type in ini file is error: %s\n", str);
				return -1;
			}
		} else {
			gxloge("str is NULL\n");
			return -1;
		}
	}

	demod_pin_map->TS00 = ts_pin_map[0];
	demod_pin_map->TS01 = ts_pin_map[1];
	demod_pin_map->TS02 = ts_pin_map[2];
	demod_pin_map->TS03 = ts_pin_map[3];
	demod_pin_map->TS04 = ts_pin_map[4];
	demod_pin_map->TS05 = ts_pin_map[5];
	demod_pin_map->TS06 = ts_pin_map[6];
	demod_pin_map->TS07 = ts_pin_map[7];
	demod_pin_map->TS08 = ts_pin_map[8];
	demod_pin_map->TS09 = ts_pin_map[9];
	demod_pin_map->TS10 = ts_pin_map[10];
	demod_pin_map->TS11 = ts_pin_map[11];

	return 0;
}

int32_t GxDLP_Set_FeDemodTsPinMap(struct pin_config *demod_pin_map)
{
	int32_t i = 0;
	int32_t j = 0;
	char pin_map_buf[128] = {0};
	char *p_pin_map_buf = pin_map_buf;
	ts_pin_t ts_pin_map[TS_PIN_NUMBER];
	int32_t ret = 0;

	if (demod_pin_map == NULL)
		return -2;

	ts_pin_map[0] = demod_pin_map->TS00;
	ts_pin_map[1] = demod_pin_map->TS01;
	ts_pin_map[2] = demod_pin_map->TS02;
	ts_pin_map[3] = demod_pin_map->TS03;
	ts_pin_map[4] = demod_pin_map->TS04;
	ts_pin_map[5] = demod_pin_map->TS05;
	ts_pin_map[6] = demod_pin_map->TS06;
	ts_pin_map[7] = demod_pin_map->TS07;
	ts_pin_map[8] = demod_pin_map->TS08;
	ts_pin_map[9] = demod_pin_map->TS09;
	ts_pin_map[10] = demod_pin_map->TS10;
	ts_pin_map[11] = demod_pin_map->TS11;

	for (i = 0; i < TS_PIN_NUMBER; i++) {
		for (j = 0; j < sizeof(ts_pin_table) / sizeof(struct ts_pin_table_struct); j++) {
			if (ts_pin_map[i] == ts_pin_table[j].ts_pin) {
				p_pin_map_buf += snprintf(p_pin_map_buf, (sizeof(pin_map_buf) - 1 - strlen(pin_map_buf)), "%s"DEMOD_TS_PINMAP_DELIM, ts_pin_table[j].ts_pin_str);
				break;
			}
		}
		if (j == sizeof(ts_pin_table) / sizeof(struct ts_pin_table_struct)) {
			gxloge("ts pin %d error : %d\n", i, ts_pin_map[i]);
			return -1;
		}
	}

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_DEMOD_TS_PINMAP, pin_map_buf);
	if(ret == -1)
		gxloge("Ini_SetValue demod ts pin map error\n");

	return 0;
}

int32_t GxDLP_Get_FeType(fe_type_t *type)
{
	char* str;
	int32_t i = 0;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_TYPE);
	if (str == NULL)
		return -1;

	for (i = 0; i < sizeof(fe_type_table) / sizeof(struct fe_type_struct); i++)
	{
		if (0 == strcmp(str, fe_type_table[i].fe_type_str))
		{
			*type = fe_type_table[i].fe_type;
			break;
		}
	}

	return 0;
}
int32_t GxDLP_Set_FeType(fe_type_t type)
{
	int32_t ret = 0;
	int32_t i;

	for (i = 0; i < sizeof(fe_type_table) / sizeof(struct fe_type_struct); i++)
	{
		if (type == fe_type_table[i].fe_type)
		{
			ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_TYPE, fe_type_table[i].fe_type_str);
			break;
		}
	}

	if(ret == -1 || (i == sizeof(fe_type_table) / sizeof(struct fe_type_struct)))
		gxloge("Ini_SetValue fe type error\n");

	return 0;
}

/*
   获取升级信息所在的频点的符号率
   频点信息可以默认写在VOEM中，也可以通过下载描述符获得。
   通过下载描述符获得时，需要更新VOEM
   */
int32_t GxDLP_Get_FeSymbolrate(void)
{
	char* str;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_SYMBOLRATE);
	if (str == NULL)
		return 6875;

	return _atoi(str);
}

int32_t GxDLP_Set_FeSymbolrate(char* value)
{
	if (NULL == value)
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_SYMBOLRATE, value);
	if(ret == -1)
		gxloge("Ini_SetValue fe symbolrate error\n");


	return 0;
}
/*
   获取升级信息所在的频点频率。
   频点信息可以默认写在VOEM中，也可以通过下载描述符获得。
   通过下载描述符获得时，需要更新VOEM
   */
int32_t GxDLP_Get_FeFrequency(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_FREQUENCY);
	if (str == NULL)
		return 858000;

	return _atoi(str);
}
int32_t GxDLP_Set_FeFrequency(char* value)
{
	if (NULL == value)
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_FREQUENCY, value);
	if(ret == -1)
		gxloge("Ini_SetValue fe frequency error\n");


	return 0;
}
/*
   获取升级信息所在的频点的调制方式
   频点信息可以默认写在VOEM中，也可以通过下载描述符获得。
   通过下载描述符获得时，需要更新VOEM
   */
int32_t GxDLP_Get_FeModulation(fe_modulation_t *type)
{
	char* str;
	int32_t i = 0;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_MODULATION);
	if (str == NULL)
		return -1;

	for (i = 0; i < sizeof(modulation_table) / sizeof(struct modulation_struct); i++)
	{
		if (0 == strcmp(str, modulation_table[i].fe_mod_type_str))
		{
			*type = modulation_table[i].fe_mod_type;
			break;
		}
	}

	return 0;
}
int32_t GxDLP_Set_FeModulation(fe_modulation_t type)
{
	int32_t ret = 0;
	int32_t i;

	for (i = 0; i < sizeof(modulation_table) / sizeof(struct modulation_struct); i++)
	{
		if (type == modulation_table[i].fe_mod_type)
		{
			ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_MODULATION, modulation_table[i].fe_mod_type_str);
			break;
		}
	}

	if(ret == -1 || (i == sizeof(modulation_table) / sizeof(struct modulation_struct)))
		gxloge("Ini_SetValue fe modulation error\n");

	return 0;
}

int32_t GxDLP_Get_FeDiseqc10_Port(void)
{
	char* str;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_DISEQC10_PORT);
	if (str == NULL)
		return 0;

	return _atoi(str);
}

int32_t GxDLP_Set_FeDiseqc10_Port(unsigned int port)
{
	int32_t ret = 0;
	char value[10+1] = {0};

	snprintf(value, 10, "%d", port);

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_DISEQC10_PORT, value);
	if(ret == -1) {
		gxloge("Ini_SetValue fe diseqc10_port error\n");
		return ret;
	}

	return ret;
}

int32_t GxDLP_Get_FeDiseqc11_Port(void)
{
	char* str;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_DISEQC11_PORT);
	if (str == NULL)
		return 0;

	return _atoi(str);
}

int32_t GxDLP_Set_FeDiseqc11_Port(unsigned int port)
{
	int32_t ret = 0;
	char value[10+1] = {0};

	snprintf(value, 10, "%d", port);

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_DISEQC11_PORT, value);
	if(ret == -1) {
		gxloge("Ini_SetValue fe diseqc11_port error\n");
		return ret;
	}

	return ret;
}

int32_t GxDLP_Get_FeSat22k(fe_sec_tone_mode_t *type)
{
	char* str;
	int32_t i = 0;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_22K);
	if (str == NULL)
		return -1;

	for (i = 0; i < sizeof(Sat22k_table) / sizeof(struct Sat22k_table_struct); i++)
	{
		if (0 == strcmp(str, Sat22k_table[i].Sat22k_str))
		{
			*type = Sat22k_table[i].Sat22k;
			break;
		}
	}

	return 0;
}

int32_t GxDLP_Set_FeSat22k(fe_sec_tone_mode_t type)
{
	int32_t ret = 0;
	int32_t i;

	for (i = 0; i < sizeof(Sat22k_table) / sizeof(struct Sat22k_table_struct); i++)
	{
		if (type == Sat22k_table[i].Sat22k)
		{
			ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_22K, Sat22k_table[i].Sat22k_str);
			break;
		}
	}

	if(ret == -1 || (i == sizeof(Sat22k_table) / sizeof(struct Sat22k_table_struct)))
		gxloge("Ini_SetValue fe sat22k error\n");

	return 0;
}

int32_t GxDLP_Get_FeBandwidth(void)
{
	char* str;
	fe_bandwidth_t ret = BANDWIDTH_8_MHZ;
	int32_t i = 0;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_BANDWIDTH);
	if (str == NULL)
		return -1;

	for (i = 0; i < BANDWIDTH_AUTO; i++)
	{
		if (0 == strcmp(str, bandwidth_type[i]))
		{
			ret = i;
			break;
		}
	}

	return ret;
}

int32_t GxDLP_Set_FeBandwidth(fe_bandwidth_t type)
{
	if (4 <= type)
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_BANDWIDTH, bandwidth_type[type]);
	if(ret == -1)
		gxloge("Ini_SetValue fe bandwidth error\n");


	return 0;
}

int32_t GxDLP_Get_FePolarity(fe_sec_voltage_t *polar)
{
	char* str;
	int32_t i = 0;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_POLARITY);
	if (str == NULL)
		return -1;

	for (i = 0; i < sizeof(Polar_table) / sizeof(struct Polar_table_struct); i++)
	{
		if (0 == strcmp(str, Polar_table[i].Polar_str))
		{
			*polar = Polar_table[i].Polar;
			break;
		}
	}

	return 0;
}

int32_t GxDLP_Set_FePolarity(fe_sec_voltage_t polar)
{
	int32_t ret = 0;
	int32_t i;

	for (i = 0; i < sizeof(Polar_table) / sizeof(struct Polar_table_struct); i++)
	{
		if (polar == Polar_table[i].Polar)
		{
			ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_POLARITY, Polar_table[i].Polar_str);
			break;
		}
	}

	if(ret == -1 || (i == sizeof(Polar_table) / sizeof(struct Polar_table_struct)))
		gxloge("Ini_SetValue fe polarity error\n");

	return 0;
}

int32_t GxDLP_Get_FePolarInvert(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_POLARITY_INVERT);
	if (str == NULL)
		return 0;

	if (_atoi(str) != 0 && _atoi(str) != 1)
		return 0;
	else
		return _atoi(str);
}

int32_t GxDLP_Set_FePolarInvert(char* value)
{
	if (NULL == value || (_atoi(value) != 0 && _atoi(value) != 1))
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_POLARITY_INVERT, value);
	if(ret == -1)
		gxloge("Ini_SetValue fe polar invert error\n");

	return 0;
}

int32_t GxDLP_Get_FePolarGpio(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_POLARITY_GPIO);
	if (str == NULL)
		return -1;

	return _atoi(str);
}

int32_t GxDLP_Set_FePolarGpio(uint32_t gpio)
{
	int32_t ret = 0;
	char str[UINT32_STRING_LENGTH+1] = {0};

	snprintf(str, UINT32_STRING_LENGTH+1, "%u", gpio);

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_POLARITY_GPIO, str);
	if(ret == -1)
		gxloge("Ini_SetValue polar gpio error\n");

	return ret;
}

int32_t GxDLP_Get_FeLnbInvert(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_LNB_INVERT);
	if (str == NULL)
		return -1;

	return _atoi(str);
}

int32_t GxDLP_Set_FeLnbInvert(uint32_t lnb_invert)
{
	int32_t ret = 0;
	char str[UINT32_STRING_LENGTH+1] = {0};

	snprintf(str, UINT32_STRING_LENGTH+1, "%u", lnb_invert);

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_LNB_INVERT, str);
	if(ret == -1)
		gxloge("Ini_SetValue lnb invert error\n");

	return ret;
}

fe_sec_tone_mode_t GxDLP_Get_Fe22k(void)
{
	char* str;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FE_22K);
	if (str == NULL)
		return 0;

	return _atoi(str);
}

int32_t GxDLP_Set_Fe22k(char* value)
{
	if (NULL == value)
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FE_22K, value);
	if(ret == -1)
		gxloge("Ini_SetValue fe 22k error\n");

	return 0;
}

char* GxDLP_Get_SERVER_IP_ADDR(void)
{
	char* str;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_TFTP_SERVER_IP_ADDR);
	if (str == NULL)
		return NULL;

	return str;
}

int32_t GxDLP_Set_SERVER_IP_ADDR(char* Value)
{
	if (Value == NULL)
		return -1;

	int32_t ret = 0;
	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_TFTP_SERVER_IP_ADDR, Value);
	if(ret == -1)
		gxloge("Ini_SetValue server ip addr error\n");

	return 0;
}

char* GxDLP_Get_HOST_IP_ADDR(void)
{
	char* str;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_TFTP_HOST_IP_ADDR);
	if (str == NULL)
		return NULL;

	return str;
}

int32_t GxDLP_Set_HOST_IP_ADDR(char* Value)
{
	if (Value == NULL)
		return -1;

	int32_t ret = 0;
	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_TFTP_HOST_IP_ADDR, Value);
	if(ret == -1)
		gxloge("Ini_SetValue  host ip addr error\n");

	return 0;
}

int32_t GxDLP_Get_TFTP_PORT(void)
{
	char* str;
	int tftp_port = 0;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_TFTP_PORT);
	if (str == NULL)
		return -1;

	tftp_port = _atoi(str);

	return tftp_port;
}

int32_t GxDLP_Set_TFTP_PORT(char* tftp_port)
{
	if (tftp_port == NULL)
		return -1;

	int32_t ret = 0;
	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_TFTP_PORT, tftp_port);
	if(ret == -1)
		gxloge("Ini_SetValue tftp port error\n");

	return 0;
}

/*
   获取当前使用的软件版本号，用于跟升级流中的软件版本
   进行对比，确认是否需要升级
   */
uint32_t GxDLP_Get_SoftwareVersion(void)
{
	char* str;

	str = Ini_GetValue(OEM_SOFTWARE_SECTION, OEM_APP_VERSION);
	if (str == NULL)
		return -1;

	return convert_version_str(str);
}

int32_t GxDLP_Set_SoftwareVersion(char *value)
{
	if (NULL == value)
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_SOFTWARE_SECTION,OEM_APP_VERSION, value);
	if(ret == -1)
		gxloge("Ini_SetValue software version error\n");


	return 0;
}

/*
   升级流软件版本号，保存升级的版本信息
     */
uint32_t GxDLP_Get_UpdateSoftVersion(void)
{
	char* str;

	str = Ini_GetValue(OEM_SOFTWARE_SECTION, OEM_APP_UPDATEVERSION);
	if (str == NULL)
		return -1;

	return convert_version_str(str);
}

int32_t GxDLP_Set_UpdateSoftVersion(char *value)
{
	if (NULL == value)
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_SOFTWARE_SECTION,OEM_APP_UPDATEVERSION, value);
	if(ret == -1)
		gxloge("Ini_SetValue update software version error\n");

	return 0;
}

int32_t GxDLP_Get_ForceUpgradeFLag(void)
{
	char* str;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_FORCE_UPGRADE_FLAG);
	if (str == NULL)
		return 0;

	return _atoi(str);
}

int32_t GxDLP_Set_ForceUpgradeFlag(char* value)
{
	if (NULL == value)
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_FORCE_UPGRADE_FLAG, value);
	if(ret == -1)
		gxloge("Ini_SetValue force_upgrade_flag error\n");

	return 0;
}

char* GxDLP_Get_PartitionStatus(void)
{
	char* str;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_PARTITION_STATUS);
	if (str == NULL)
		return "0";

	return str;
}

int32_t GxDLP_Set_PartitionStatus(const char* partition_status)
{
	if (NULL == partition_status || (strlen(partition_status) > FLASH_PARTITION_MAX_NUM))
		return -1;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_PARTITION_STATUS, partition_status);
	if(ret == -1)
		gxloge("Ini_SetValue partition status error\n");

	return 0;
}

const char *GxDLP_Get_WIFI_BSSID(void)
{
	char *str;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_WIFI_BSSID);
	if (str == NULL)
		return "0";

	return str;
}

int32_t GxDLP_Set_WIFI_BSSID(const char *wifi_bssid)
{
	if (NULL == wifi_bssid || (strlen(wifi_bssid) != 17))
		return -2;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_WIFI_BSSID, wifi_bssid);
	if(ret == -1)
		gxloge("Ini_SetValue wifi bssid error\n");

	return ret;
}

const char *GxDLP_Get_WIFI_PSK(void)
{
	char *str;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_WIFI_PSK);
	if (str == NULL)
		return "0";

	return str;
}

int32_t GxDLP_Set_WIFI_PSK(const char *wifi_psk)
{
	if (NULL == wifi_psk)
		return -2;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_WIFI_PSK, wifi_psk);
	if(ret == -1)
		gxloge("Ini_SetValue wifi psk error\n");

	return ret;
}

int32_t GxDLP_Get_WIFIConnectTimes(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_WIFI_CONNECT_TIMES);
	if (str == NULL)
		return 0;
	if (_atoi(str) > 5)
		return 5;

	return _atoi(str);
}

int32_t GxDLP_Set_WIFIConnectTimes(int32_t times)
{
	int32_t ret = 0;
	char str[UINT32_STRING_LENGTH+1] = {0};

	if (times > 5)
		times = 5;
	snprintf(str, UINT32_STRING_LENGTH+1, "%u", times);

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_WIFI_CONNECT_TIMES, str);
	if(ret == -1)
		gxloge("Ini_SetValue Wifi_times error\n");

	return ret;
}

int32_t GxDLP_Get_HTTPGetTimes(void)
{
	char* str;
	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_HTTP_GET_TIMES);
	if (str == NULL)
		return 0;
	if (_atoi(str) > 3)
		return 3;

	return _atoi(str);
}

int32_t GxDLP_Set_HTTPGetTimes(int32_t times)
{
	int32_t ret = 0;
	char str[UINT32_STRING_LENGTH+1] = {0};

	if (times > 3)
		times = 3;
	snprintf(str, UINT32_STRING_LENGTH+1, "%u", times);

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_HTTP_GET_TIMES, str);
	if(ret == -1)
		gxloge("Ini_SetValue http_get_times error\n");

	return ret;
}

const char *GxDLP_Get_URL(void)
{
	char *str;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_URL);
	if (str == NULL)
		return "0";

	return str;
}

int32_t GxDLP_Set_URL(const char *url)
{
	if (NULL == url)
		return -2;

	int32_t ret = 0;

	ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_URL, url);
	if(ret == -1)
		gxloge("Ini_SetValue url error\n");

	return ret;
}

int32_t GxDLP_Set_HighestPriNetDevice(GxDLP_Net_Device_Type type)
{
	int32_t ret = 0;
	int32_t i;

	for (i = 0; i < sizeof(net_device_type_table) / sizeof(struct net_device_type_table_struct); i++)
	{
		if (type == net_device_type_table[i].net_dev_type)
		{
			ret = Ini_SetValue(OEM_UPDATE_SECTION, OEM_NET_DEVICE_TYPE, net_device_type_table[i].net_device_type_str);
			break;
		}
	}

	if(ret == -1 || (i == sizeof(net_device_type_table) / sizeof(struct net_device_type_table_struct))) {
		ret = -1;
		gxloge("%s error\n", __func__);
	}

	return ret;
}

int32_t GxDLP_Get_HighestPriNetDevice(GxDLP_Net_Device_Type *type)
{
	char *str;
	int32_t i = 0;

	str = Ini_GetValue(OEM_UPDATE_SECTION, OEM_NET_DEVICE_TYPE);
	if (str == NULL)
		return -1;

	for (i = 0; i < sizeof(net_device_type_table) / sizeof(struct net_device_type_table_struct); i++)
	{
		if (0 == strcmp(str, net_device_type_table[i].net_device_type_str))
		{
			*type = net_device_type_table[i].net_dev_type;
			break;
		}
	}

	if(i == sizeof(net_device_type_table) / sizeof(struct net_device_type_table_struct)) {
		gxloge("%s error\n", __func__);
		return -1;
	}

	return 0;
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
