#ifndef __GXAV_VOUT_PROPERTYTYPES_H__
#define __GXAV_VOUT_PROPERTYTYPES_H__

#include "gxav_common.h"

typedef enum {
	GXAV_VOUT_RCA         = (1 << 0),
	GXAV_VOUT_RCA1        = (1 << 1),
	GXAV_VOUT_YUV         = (1 << 2),
	GXAV_VOUT_SCART       = (1 << 3),
	GXAV_VOUT_SVIDEO      = (1 << 4),
	GXAV_VOUT_HDMI        = (1 << 5),
	GXAV_VOUT_ALL         = (0x3F),
}GxVideoOutProperty_Interface;

typedef enum {
	GXAV_VOUT_PAL = 1     ,
	GXAV_VOUT_PAL_M       ,
	GXAV_VOUT_PAL_N       ,
	GXAV_VOUT_PAL_NC      ,
	GXAV_VOUT_NTSC_M      ,
	GXAV_VOUT_NTSC_443    ,

	GXAV_VOUT_480I        ,
	GXAV_VOUT_480P        ,
	GXAV_VOUT_576I        ,
	GXAV_VOUT_576P        ,
	GXAV_VOUT_720P_50HZ   ,
	GXAV_VOUT_720P_60HZ   ,
	GXAV_VOUT_1080I_50HZ  ,
	GXAV_VOUT_1080P_50HZ  ,
	GXAV_VOUT_1080I_60HZ  ,
	GXAV_VOUT_1080P_60HZ  ,

	GXAV_VOUT_NULL_MAX    ,
}GxVideoOutProperty_Mode;

typedef enum {
	QR_LIMIT_RANGE,
	QR_FULL_RANGE,
	QR_AUTO_RANGE,
}GxVideoOutProperty_Quantization_Range;

typedef enum {
	ASPECT_RATIO_NOMAL = 0,
	ASPECT_RATIO_PAN_SCAN,
	ASPECT_RATIO_LETTER_BOX,
	ASPECT_RATIO_COMBINED,    // pan scan and letter box combined
	ASPECT_RATIO_RAW_SIZE,
	ASPECT_RATIO_RAW_RATIO,
	ASPECT_RATIO_4X3_PULL,
	ASPECT_RATIO_4X3_CUT,
	ASPECT_RATIO_16X9_PULL,
	ASPECT_RATIO_16X9_CUT,
	ASPECT_RATIO_4X3,
	ASPECT_RATIO_16X9,
	ASPECT_RATIO_AUTO,
}GxVideoOutProperty_Spec;

typedef enum {
	TV_SCREEN_4X3 = 0,
	TV_SCREEN_16X9
}GxVideoOutProperty_Screen;

typedef enum {
	SCART_CVBS = 0,
	SCART_RGB
} GxScartMode;

typedef enum {
	HDCP_VERSION_NU = 0,
	HDCP_VERSION_14 = 1 << 0,
	HDCP_VERSION_22 = 1 << 1,
} GxHDCPVersion;

typedef struct {
	int enable;
	GxScartMode mode;
} GxVideoOutProperty_ScartConfig;

typedef struct {
	unsigned int enable;
	GxVideoOutProperty_Mode pal;
	GxVideoOutProperty_Mode ntsc;
}GxVideoOutProperty_Auto;

typedef struct {
#define VOUT_BUF_SIZE (720*576*3)
	unsigned int   svpu_enable;
	unsigned char *svpu_buf[3];
	unsigned int   svpu_delayms;

	unsigned int   hdmi_delayms;

	GxVideoOutProperty_Auto vout1_auto;

	unsigned int hdcp_enable __attribute__((deprecated("see GxVideoOutProperty_HdmiHdcpConfig")));
	unsigned int macrovision_enable  __attribute__((deprecated("see GxVideoOutProperty_Macrovision")));
} GxVideoOutProperty_OutputConfig;

typedef struct {
	int selection;
} GxVideoOutProperty_OutputSelect;

typedef struct {
	GxVideoOutProperty_Interface iface;
	GxVideoOutProperty_Mode      mode;
} GxVideoOutProperty_Resolution;

typedef struct {
	GxVideoOutProperty_Interface iface;
	unsigned int value;
} GxVideoOutProperty_Brightness;

typedef struct {
	GxVideoOutProperty_Interface iface;
	unsigned int value;
} GxVideoOutProperty_Saturation;

typedef struct {
	GxVideoOutProperty_Interface iface;
	unsigned int value;
} GxVideoOutProperty_Contrast;

typedef struct {
	GxVideoOutProperty_Interface iface;
	unsigned int value;
} GxVideoOutProperty_Sharpness;

typedef struct {
	GxVideoOutProperty_Interface iface;
	unsigned int value;
} GxVideoOutProperty_Hue;

typedef struct {
	GxVideoOutProperty_Spec spec;
}GxVideoOutProperty_AspectRatio;

typedef struct {
	GxVideoOutProperty_Screen screen;
}GxVideoOutProperty_TvScreen;

typedef struct {
	GxVideoOutProperty_Interface iface;
	unsigned int enable;
} GxVideoOutProperty_OutDefault;

typedef enum {
	VOUT_POWER_MODE_DEFAULT = 0,
	VOUT_POWER_MODE_DEEP_SLEEP = 1,
} GxVideoOutPowerMode;

typedef struct {
	GxVideoOutProperty_Interface selection;
	GxVideoOutPowerMode mode;
} GxVideoOutProperty_PowerOff;

typedef struct {
	GxVideoOutProperty_Interface selection;
} GxVideoOutProperty_PowerOn;

typedef struct {
	int status;
#define HDMI_PLUG_OUT	(0)
#define HDMI_PLUG_IN	(1)
} GxVideoOutProperty_OutHdmiStatus;

typedef struct {
	/*
	 * tv_cap is the mask of all resolutions TV support, which is get from TV EDID
	 *
	 * exam :
	 *	tv_cap = (1<<GXAV_VOUT_480I) | (1<<GXAV_VOUT_576I) | ... | (GXAV_VOUT_1080I_50HZ);
	 *
	 **/
	unsigned tv_cap;
	unsigned native_resolution;
	GxAvHdmiEdid audio_codes;

	/* raw edid data */
	unsigned int  data_len;
	unsigned char data[512];
} GxVideoOutProperty_EdidInfo;

typedef struct {
	int enable;
} GxVideoOutProperty_HdmiHdcpEnable;

typedef enum {
	SYSTEM_625 = 0,
	SYSTEM_525,
	SYSTEM_MAX
} GxFormatSystem;

typedef struct {
	/* CPC0 at [0:3], CPC1 at [4:7] */
	unsigned char cpc;
	/* CPS0 at cps[0][0:3], CPS1 at cps[0][4:7], CPS2 at cps[1][0:3]... */
	unsigned char cps[17];
} GxMacrovisionParam;

/** CNcomment:显示输出Macrovision模式枚举定义*/
typedef enum vout_macrovision_mode {
	GX_MACROVISION_MODE_TYPE0,     /*<type 0:no protect process *//**<CNcomment:无保护处理*/
	GX_MACROVISION_MODE_TYPE1,     /*<type 1:AGC (automatic gain control) process only *//**<CNcomment:仅自动增益控制 */
	GX_MACROVISION_MODE_TYPE2,     /*<type 2:AGC + 2-line color stripe *//**<CNcomment:自动增益控制和两线色 度干扰*/
	GX_MACROVISION_MODE_TYPE3,     /*<type 3:AGC + aggressive 4-line color stripe *//**<CNcomment:自动增益和四线强色度干扰*/
	GX_MACROVISION_MODE_TEST1,     /*认证 1 参数*/
	GX_MACROVISION_MODE_TEST2,     /*认证 2 参数*/
	GX_MACROVISION_MODE_CUSTOM,    /*<type of configure by user *//**<CNcomment:用户自定义配置 */
	GX_MACROVISION_MODE_BUTT
} GxMacrovisionMode;

typedef struct {
	GxMacrovisionMode mode;
	GxMacrovisionParam custom_param;/* 仅当选择GX_MACROVISION_MODE_CUSTOM时填写 */
}GxVideOutMacrovision;

typedef struct {
	GxVideOutMacrovision data[SYSTEM_MAX];
} GxVideoOutProperty_Macrovision;

typedef enum {
	GX_DCS_MODE_DISABLE,
	GX_DCS_MODE_A35,
	GX_DCS_MODE_A36,
} GxVideoOutDCSMode;

typedef struct {
	GxVideoOutDCSMode mode;
} GxVideoOutProperty_DCS;

typedef struct {
	int enable[SYSTEM_MAX];
} GxVideoOutProperty_CGMSEnable;

typedef enum vout_cgms_a_copy_e {
    VOUT_CGMS_A_COPY_PERMITTED = 0,          /**< copying is permitted without restriction*//**< CNcomment:无限制拷贝*/
    VOUT_CGMS_A_COPY_ONE_TIME_BEEN_MADE = 1, /**< no more copies, one generation copy has been made*//**< CNcomment:已经拷贝一次*/
    VOUT_CGMS_A_COPY_ONE_TIME  = 2,          /**< one generation of copied may be made*//**< CNcomment:只能拷贝一次*/
    VOUT_CGMS_A_COPY_FORBIDDEN = 3,          /**< no copying is permitted*//**< CNcomment:禁止拷贝*/
    VOUT_CGMS_A_BUTT,
} GxVideoOutCGMSACopyPermission;

typedef struct {
	GxVideoOutCGMSACopyPermission CGMS_A_control;
	int ASP_control;
	int ASB_control;
} GxVideoOutCGMS;

typedef struct {
	GxVideoOutCGMS data[SYSTEM_MAX];
} GxVideoOutProperty_CGMSConfig;

typedef struct {
	int enable[SYSTEM_MAX];
} GxVideoOutProperty_WSSEnable;

typedef enum vout_wss_copy_e {
	VOUT_VBI_WSS_COPY_PERMITTED  = 0, /*< copying not restricted >*/
	VOUT_VBI_WSS_COPY_RESTRICTED = 1, /*< copying restricted >*/
	VOUT_VBI_WSS_COPY_ASSERTED   = 2, /*< copyright asserted >*/
	VOUT_VBI_WSS_COPY_FORBIDDEN  = 3, /*< copyright asserted,copying restricted >*/
	VOUT_VBI_WSS_BUTT,
} GxVideoOutWSSCopyPermission;

typedef struct {
	GxVideoOutWSSCopyPermission WSS_control[SYSTEM_MAX];
} GxVideoOutProperty_WSSConfig;

typedef struct {
	int enable;
} GxVideoOutProperty_HdmiCecEnable;

typedef enum {
	CEC_OP_STANDBY                       = (0x36),
	CEC_OP_IMAGE_VIEW_ON                 = (0x04),

	CEC_OP_ACTIVE_SOURCE                 = (0x82),
	CEC_OP_TEXT_VIEW_ON                  = (0x0D),
	CEC_OP_INACTIVE_SOURCE               = (0x9D),
	CEC_OP_REQUEST_ACTIVE_SOURCE         = (0x85),
	CEC_OP_ROUTING_CHANGE                = (0x80),
	CEC_OP_ROUTING_INFORMATION           = (0x81),
	CEC_OP_SET_STREAM_PATH               = (0x86),
	CEC_OP_RECORD_OFF                    = (0x0B),
	CEC_OP_RECORD_ON                     = (0x09),
	CEC_OP_RECORD_STATUS                 = (0x0A),
	CEC_OP_RECORD_TV_SCREEN              = (0x0F),
	CEC_OP_CLEAR_ANALOGUE_TIMER          = (0x33),
	CEC_OP_CLEAR_DIGITAL_TIMER           = (0x99),
	CEC_OP_CLEAR_EXTERNAL_TIMER          = (0xA1),
	CEC_OP_SET_ANALOGUE_TIMER            = (0x34),
	CEC_OP_SET_DIGITAL_TIMER             = (0x97),
	CEC_OP_SET_EXTERNAL_TIMER            = (0xA2),
	CEC_OP_SET_TIMER_PROGRAM_TITLE       = (0x67),
	CEC_OP_TIMER_CLEARED_STATUS          = (0x43),
	CEC_OP_TIMER_STATUS                  = (0x35),
	CEC_OP_CEC_VERSION                   = (0x9E),
	CEC_OP_GET_CEC_VERSION               = (0x9F),
	CEC_OP_GIVE_PHYSICAL_ADDRESS         = (0x83),
	CEC_OP_GET_MENU_LANGUAGE             = (0x91),
	CEC_OP_REPORT_PHYSICAL_ADDRESS       = (0x84),
	CEC_OP_SET_MENU_LANGUAGE             = (0x32),
	CEC_OP_DECK_CONTROL                  = (0x42),
	CEC_OP_DECK_STATUS                   = (0x1B),
	CEC_OP_GIVE_DECK_STATUS              = (0x1A),
	CEC_OP_PLAY                          = (0x41),
	CEC_OP_GIVE_TUNER_DEVICE_STATUS      = (0x08),
	CEC_OP_SELECT_ANALOGUE_SERVICE       = (0x92),
	CEC_OP_SELECT_DIGITAL_SERVICE        = (0x93),
	CEC_OP_TUNER_DEVICE_STATUS           = (0x07),
	CEC_OP_TUNER_STEP_DECREMENT          = (0x06),
	CEC_OP_TUNER_STEP_INCREMENT          = (0x05),
	CEC_OP_DEVICE_VENDOR_ID              = (0x87),
	CEC_OP_GIVE_DEVICE_VENDOR_ID         = (0x8C),
	CEC_OP_VENDOR_COMMAND                = (0x89),
	CEC_OP_VENDOR_COMMAND_WITH_ID        = (0xA0),
	CEC_OP_VENDOR_REMOTE_BUTTON_DOWN     = (0x8A),
	CEC_OP_VENDOR_REMOTE_BUTTON_UP       = (0x8B),
	CEC_OP_SET_OSD_STRING                = (0x64),
	CEC_OP_GIVE_OSD_NAME                 = (0x46),
	CEC_OP_SET_OSD_NAME                  = (0x47),
	CEC_OP_MENU_REQUEST                  = (0x8D),
	CEC_OP_MENU_STATUS                   = (0x8E),
	CEC_OP_USER_CONTROL_PRESSED          = (0x44),
	CEC_OP_USER_CONTROL_RELEASED         = (0x45),
	CEC_OP_GIVE_DEVICE_POWER_STATUS      = (0x8F),
	CEC_OP_REPORT_POWER_STATUS           = (0x90),
	CEC_OP_FEATURE_ABORT                 = (0x00),
	CEC_OP_ABORT                         = (0xFF),
	CEC_OP_GIVE_AUDIO_STATUS             = (0x71),
	CEC_OP_GIVE_SYSTEM_AUDIO_MODE_STATUS = (0x7D),
	CEC_OP_REPORT_AUDIO_STATUS           = (0x7A),
	CEC_OP_SET_SYSTEM_AUDIO_MODE         = (0x72),
	CEC_OP_SYSTEM_AUDIO_MODE_REQUEST     = (0x70),
	CEC_OP_SYSTEM_AUDIO_MODE_STATUS      = (0x7E),
	CEC_OP_SET_AUDIO_RATE                = (0x9A),
} GxHdmiCecOpcode;

typedef struct {
	GxHdmiCecOpcode code;
} GxVideoOutProperty_HdmiCecCmd;

typedef struct {
	unsigned int hdmi_major;     /* major in number */
	unsigned int hdmi_minor;     /* minor in number */
	unsigned int hdmi_revision;  /* revision in number */
	unsigned int hdcp_major;
	unsigned int hdcp_minor;
	unsigned int hdcp_revision;
} GxVideoOutProperty_HdmiVersion;

typedef struct {
	GxAvHdmiEncodingOut encoding;
} GxVideoOutProperty_HdmiEncodingOut;

typedef struct {
	GxAvHdmiColorDepth color_depth;
} GxVideoOutProperty_HdmiColorDepth;

typedef struct {
	GxAvHdmiSSCMode ssc_mode;
} GxVideoOutProperty_HdmiSSCMode;

typedef struct {
	int mute;
} GxVideoOutProperty_HdmiAudioMute;

typedef struct {
	int mute;
} GxVideoOutProperty_HdmiAvMute;

typedef struct {
	unsigned char *payload;
	unsigned char  payload_len;
} GxVideoOutProperty_HdmiVSIInfo;

typedef struct {
	unsigned char vender_name[8];
	unsigned char product_description[16];
	unsigned char source_device_type;
} GxVideoOutProperty_HdmiSPDInfo;

typedef enum {
	GX_HDCP_STATUS_UNCONNECTED = 0,
	GX_HDCP_STATUS_CONNECTED,
	GX_HDCP_STATUS_ENGAGED,
	GX_HDCP_STATUS_FAILED,
	GX_HDCP_STATUS_I2C_NACK,
	GX_HDCP_STATUS_AUTHING,
	GX_HDCP_STATUS_NOT_CAPABLE,
} GxVideoOutProperty_HdmiHdcpStatus;

typedef struct {
	unsigned char header;
	unsigned char message_len;
	unsigned char message[16];
} GxVideoOutProperty_HdmiCecRW;

typedef struct {
	unsigned short logical_addr;   ///< 逻辑地址
	unsigned short physical_addr;  ///< 物理地址
} GxVideoOutProperty_HdmiCecAddr;

typedef enum {
	DOT_SDR_8BIT = 0,
	DOT_SDR_10BIT,
	DOT_HLG,
	DOT_HDR10,
	DOT_HDR10PLUS,
}GxVideoOutProperty_HdmiDisplayOutputType;

#define GX_VBI_MODULATION_EN300294 (0x00000001)
#define GX_VBI_MODULATION_EN300231 (0x00000002)
#define GX_VBI_MODULATION_EN300706 (0x00000004)
#define GX_VBI_MODULATION_EIA608   (0x00000008)
#define GX_VBI_MODULATION_IEC61880 (0x00000010)
#define GX_VBI_MODULATION_IEC62375 (0x00000020)
#define GX_VBI_MODULATION_CEA805_TYPE_A (0x00000040)
#define GX_VBI_MODULATION_CEA805_TYPE_B (0x00000080)

typedef struct {
	unsigned short    line;
	unsigned int      modulation;
	unsigned char     *data;
} GxVideoOutProperty_VbiLineConfigure;

typedef struct {
	unsigned short numlines;
	GxVideoOutProperty_VbiLineConfigure *lines;
} GxVideoOutProperty_VbiConfigure;

typedef struct {
	unsigned int    modulation;
	unsigned char   control;
} GxVideoOutProperty_VbiControl;

typedef struct {
	unsigned int field_parity;
	unsigned int line_offset;
} GxVBILineData;

typedef struct {
	unsigned int num;
	GxVBILineData data[24];
} GxVideoOutProperty_VbiLine;

/**
 * VOUT 典型缩放系数（内置）
 */
typedef enum {
	GXVOUT_SCALE_STANDARD ,       ///< 指标测试专用
	GXVOUT_SCALE_AUTO     ,       ///< 根据制式自动调整系数
	GXVOUT_SCALE_CUSTOM   ,       ///< 客户自定义缩放系数
	GXVOUT_SCALE_MAX      ,
} GxVideoOut_ScaleParam;

#define GXVOUT_ZOOM_COEF_COUNT            (18)

/**
 * VOUT 缩放配置
 */
typedef struct {
	GxVideoOut_ScaleParam             param_h_index;  /// 典型水平缩放系数
	GxVideoOut_ScaleParam             param_v_index;  /// 典型垂直缩放系数
	unsigned int                  custom_h_param[GXVOUT_ZOOM_COEF_COUNT];  /// 客户自定义水平缩放系数
	unsigned int                  custom_v_param[GXVOUT_ZOOM_COEF_COUNT];  /// 客户自定义垂直缩放系数
} GxVideoOutProperty_ScaleConfig;

/**
 * VOUT 指标测试参数
 */
typedef struct {
	///< 测试指标：视频DAC整体增益
	struct {
		int gdac0_ips:8;       ///< 第0路视频DAC增益控制
		int gdac1_ips:8;       ///< 第1路视频DAC增益控制
		int gdac2_ips:8;       ///< 第2路视频DAC增益控制
		int gdac3_ips:8;       ///< 第3路视频DAC增益控制
	};
	///< 测试指标：Frequency Response/Multi-burst
	struct {
		unsigned int tv_fir_val0;
		unsigned int tv_fir_val1;
		unsigned int tv_fir_val2;
		unsigned int tv_fir_val3;
		unsigned int tv_fir_val4;
	};
	///< 测试指标：Bar Level
	struct {
		unsigned int m0_scale0;        ///< Y转换系数。数值 ＝ 转换系数 × 256。用于将亮度信息压缩到相应的数字段
	};
	///< 测试指标：Chroma Gain
	struct {
		unsigned int m1_scale1:10;     ///< Cb转换系数。数值 ＝ 转换系数 × 256。用于将亮度信息压缩到相应的数字段
		unsigned int m2_scale2:10;     ///< Cr转换系数。数值 ＝ 转换系数 × 256。用于将亮度信息压缩到相应的数字段
	};
	///< 测试指标：Burst Amplitude (Chroma sync level) 色度同步幅度
	struct {
		unsigned int burst_amp_a;       ///< 红色差同步幅度设置
		unsigned int burst_amp_b;       ///< 蓝色差同步幅度设置
	};
	///< 测试指标：Sync Level
	struct {
		unsigned int blank_amp;         ///< 空白电平幅度
		unsigned int black_amp;         ///< 黑电平幅度
		unsigned int y_shift;           ///< 亮度偏移电平幅
	};
} GxVideoOutProperty_PerformanceFixer;

typedef struct {
	int enable;
} GxVideoOutProperty_AutoCapture;

/**
 *
 * \brief                NexGuard 使能
 */
typedef struct {
	int enable;
} GxVideoOutProperty_NexGuardEnable;

/**
 *
 * \brief                NexGuard 时间信息
 */
typedef struct {
	unsigned short timeval;
} GxVideoOutProperty_NexGuardTimeStamp;

/**
 *
 * \brief               NexGuard  水印显示区域
 */
typedef struct {
	int left;
	int top;
	int right;
	int bottom;
} GxVideoOutProperty_NexGuardRect;

/**
 *
 * \brief                NexGuard 当前处理的帧数据类型
 */
typedef enum {
	VOUT_NG_UNSUPPORT,        ///< 不支持
	VOUT_NG_R_FRAME,          ///< 右半帧
	VOUT_NG_L_FRAME,          ///< 左半帧
	VOUT_NG_WHOLE_FRAME,      ///< 全帧
	VOUT_NG_R_R_FRAME,        ///< 右右帧
	VOUT_NG_M_R_FRAME,        ///< 中右帧
	VOUT_NG_M_L_FRAME,        ///< 中左帧
	VOUT_NG_L_L_FRAME,        ///< 左左帧
} GxVoutNGFrameType;

typedef enum {
	VOUT_NG_PAYLOAD_56BIT,     ///< 56bits Payload
	VOUT_NG_PAYLOAD_24BIT,     ///< 24bits Payload
} GxVoutNGPayLoadBitWidth;

/**
 *
 * \brief               NexGuard 视频帧参数
 */
typedef struct {
	GxVoutNGFrameType type;                   ///< gxdocref GxVoutNGFrameType
	int  rate;                                ///< gxdocref GXVoutNGFrameRate
	int  payload_en;                          ///< payload 使能
	GxVoutNGPayLoadBitWidth payload_bitwidth;                     ///< payload 位宽  gxdocref GxVoutNGPayLoadBitWidth
	char payload_word[5];                     ///< payload 参数
} GxVideoOutProperty_NexGuardFrameConfig;

/**
 *
 * \brief               NexGuard 内核参数
 */
typedef struct {
	int corein;     ///< 水印 Core 编号
	int keyin;      ///< 水印 Key 信息
	int settingword[6];  ///< 水印 Settingword
} GxVideoOutProperty_NexGuardParam;

/**
 *
 * \brief              NexGuard 版本号
 */
typedef struct {
	int version;
} GxVideoOutProperty_NexGuardVersion;

/**
 *
 * \brief              NexGuard 错误码
 */
typedef struct {
	int code;
} GxVideoOutProperty_NexGuardErrorCode;


/**
 *
 * \brief              NexGuard 调试模式开启
 */
typedef struct {
	int enable;
} GxVideoOutProperty_NexGuardDebug;

/**
 *
 * \brief        输出安全限制配置
 */
typedef struct {
	int enable;                 ///< 输出安全色使能
	GxColor color;              ///< 限制输出使用单色颜色值（YUV格式）
	int width;                  ///< 限制输出分辨率宽度
	int height;                 ///< 限制输出分辨率高度
} GxLayerLimitConfig;

typedef struct {
	GxLayerID layer;
	GxLayerLimitConfig config;
} GxVideoOutProperty_LayerSecureConfig;

#endif

