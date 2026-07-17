#ifndef _GXAV_COMMON_H_
#define _GXAV_COMMON_H_

#ifndef NULL
#define NULL 0
#endif

#define CRYSTAL_FREQ 27000000  /* HZ */

/*
 * GX_MIN()/GX_MAX() macros that also do strict
 * type-checking. See the pointer comparison
 * [(void) (&_min1 == &_min2);]that do this.
 */
#define GX_MIN(x, y) ({                   \
		typeof(x) _min1 = (x);            \
		typeof(y) _min2 = (y);            \
		(void) (&_min1 == &_min2);        \
		_min1 < _min2 ? _min1 : _min2; })

#define GX_MAX(x, y) ({                   \
		typeof(x) _max1 = (x);            \
		typeof(y) _max2 = (y);            \
		(void) (&_max1 == &_max2);        \
		_max1 > _max2 ? _max1 : _max2; })

#define GX_ABS(x) ({                      \
		int __x = (x);                    \
		(__x < 0) ? -__x : __x;           \
		})

#define GX_ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))

typedef enum  gxav_chip_id {
	GXAV_ID_GX3211  = 0x3211,
	GXAV_ID_GX3201  = 0x3201,
	GXAV_ID_GX3113C = 0x6131,
	GXAV_ID_GX6605S = 0x6605,
	GXAV_ID_SIRIUS  = 0x6612,
	GXAV_ID_TAURUS  = 0x6616,
	GXAV_ID_GEMINI  = 0x6701,
	GXAV_ID_CYGNUS  = 0x6705,
	GXAV_ID_CANOPUS = 0x6631,
	GXAV_ID_VEGA    = 0x6633,
	GXAV_ID_SCORPIO = 0x3113,
}GxAvChipId;

typedef enum {
	GXAV_SDC_READ,
	GXAV_SDC_WRITE
}GxAvSdcOPMode;

/*
 * GxAvModuleType 枚举和其他驱动仓库复用，所以修改该枚举时要注意是否产生冲突
 *  - gxfrontend : frontend.h 中的 GxFEModule 枚举
 *  - gxsecure : gxse_debug_level.h 中的 GxSeLogModule 枚举
 */
typedef enum gxav_module_type {
	GXAV_MOD_SDC           = 0x00,
	GXAV_MOD_CLOCK         = 0x01,
	GXAV_MOD_HDMI          = 0x02,
	GXAV_MOD_DEMUX         = 0x03,
	GXAV_MOD_VIDEO_DECODE  = 0x04,
	GXAV_MOD_AUDIO_DECODE  = 0x05,
	GXAV_MOD_VPU           = 0x06,
	GXAV_MOD_AUDIO_OUT     = 0x07,
	GXAV_MOD_VIDEO_OUT     = 0x08,
	GXAV_MOD_JPEG_DECODER  = 0x09,
	GXAV_MOD_STC           = 0x0a,
	GXAV_MOD_MTC           = 0x0b,
	GXAV_MOD_ICAM          = 0x0c,
	GXAV_MOD_IEMM          = 0x0d,
	GXAV_MOD_DESCRAMBLER   = 0x0e,
	GXAV_MOD_GP            = 0x0f,
	GXAV_MOD_DVR           = 0x10,
	GXAV_MOD_SECURE        = 0x11,
	GXAV_MOD_NPU           = 0x12,
	GXAV_MOD_HDCP14        = 0x13,
	GXAV_MOD_HDCP22        = 0x14,
	GXAV_MOD_MAX           = 0x15,
} GxAvModuleType;

typedef enum  gxav_module_state {
	GXAV_MOD_RUNNING,
	GXAV_MOD_STOP,
	GXAV_MOD_PAUSE,
} GxAvModuleState;

typedef struct gxav_point {
	unsigned int x;
	unsigned int y;
} GxAvPoint;

typedef struct gxav_rect {
	unsigned int x;
	unsigned int y;
	unsigned int width;
	unsigned int height;
} GxAvRect;

typedef enum  gxav_direction {
	GXAV_PIN_INPUT  = 0x1,
	GXAV_PIN_OUTPUT = 0x2
} GxAvDirection;

typedef enum  gxav_pin_id {
	GXAV_PIN_ESA   = 0x1,
	GXAV_PIN_PCM   = 0x2,
	GXAV_PIN_AC3   = 0x3, //直接过滤的AC3数据
	GXAV_PIN_EAC3  = 0x4, //降级处理的AC3数据
	GXAV_PIN_ADESA = 0x5,
	GXAV_PIN_ADPCM = 0x6,
	GXAV_PIN_DTS   = 0x7,
	GXAV_PIN_AAC   = 0x8,
	GXAV_PIN_MAX   = 0xff
} GxAvPinId;

typedef struct gxav_channel_info {
	unsigned int pin_id;
	void *channel;
	GxAvDirection dir;
} GxAvChanInfo;

typedef enum  gxav_channel_flag {
	N  = 0x0,
	W  = 0x1,
	R  = 0x2,
	RW = 0x3
} GxAvChannelFlag;

typedef enum gxav_channel_type {
	GXAV_NO_PTS_FIFO   = (1 << 0),
	GXAV_PTS_FIFO      = (1 << 1),
	GXAV_WPROTECT_FIFO = (1 << 2),
	GXAV_RPROTECT_FIFO = (1 << 3),
} GxAvChannelType;

typedef enum {
	GXAV_FRAME_PLAY_NORMAL,
	GXAV_FRAME_PLAY_REPEAT,
	GXAV_FRAME_PLAY_SKIP,
	GXAV_FRAME_PLAY_FREERUN,
} GxAvSyncFrameState;

typedef struct gxav_gate_info {
	unsigned int almost_empty;
	unsigned int almost_full;
	unsigned int cache_max;
} GxAvGateInfo;

typedef struct gxav_sync_params {
	unsigned int pcr_err_gate;
	unsigned int pcr_err_time;
	unsigned int apts_err_gate;
	unsigned int apts_err_time;
	unsigned int audio_low_tolerance;
	unsigned int audio_high_tolerance;
	int stc_offset;
} GxAvSyncParams;

typedef enum {
	GXAV_VIDEO_SD = 0,
	GXAV_VIDEO_HD    ,
	GXAV_VIDEO_FHD   ,
	GXAV_VIDEO_UHD   ,
	GXAV_VIDEO_FUHD  ,
} GxAvVideoDefinition;

typedef struct gxav_device_capability {
	struct {
		int support_10bit;
		GxAvVideoDefinition definition;
	} video;
} GxAVDeviceCapability;

typedef struct gxav_frame_stat {
	unsigned long long play_frame_cnt;
	unsigned long long error_frame_cnt;
	unsigned long long filter_frame_cnt;
	unsigned long long lose_sync_cnt;
	unsigned long long decode_frame_cnt;
	unsigned int       synced_flag;
	GxAvSyncFrameState synced_state;
	struct {
		unsigned int enable;
		unsigned int code;
	} AFD;
} GxAvFrameStat;

typedef enum {
	PCM_TYPE     = (1<<0),
	AC3_TYPE     = (1<<1),
	EAC3_TYPE    = (1<<2),
	ADPCM_TYPE   = (1<<3),
	DTS_TYPE     = (1<<4),
	AAC_TYPE     = (1<<5),
} GxAvAudioDataType;

typedef enum {
	PCM_ERROR     = (1<<0),
	AC3_ERROR     = (1<<1),
	EAC3_ERROR    = (1<<2),
	ADPCM_ERROR   = (1<<3),
	DTS_ERROR     = (1<<4),
	AAC_ERROR     = (1<<5),
} GxAvAudioDataEvent;

typedef enum {
	AS_NONE = 0,
	AS_ACCESS_RESTRICT = (1<<0),
	AS_ACCESS_ALIGN    = (1<<1),
} GxAvAdvancedSecurity;

typedef struct {
	unsigned int addr;
	unsigned int size;
} GxAvASDataBlock;

typedef enum {
	EDID_AUDIO_LINE_PCM = (1<<0),
	EDID_AUDIO_AC3      = (1<<1),
	EDID_AUDIO_EAC3     = (1<<2),
	EDID_AUDIO_DTS      = (1<<3),
	EDID_AUDIO_AAC      = (1<<4),
} GxAvHdmiEdid;

typedef enum videoout_hdmi_type {
	HDMI_RGB_OUT = 1,
	HDMI_YUV_422 = 2,
	HDMI_YUV_444 = 4,
	HDMI_AUTO_OUT = 8,
} GxAvHdmiEncodingOut;

typedef enum videoout_hdmi_color_depth {
	HDMI_COLOR_DEPTH_8BIT  = 1,
	HDMI_COLOR_DEPTH_10BIT = 2,
	HDMI_COLOR_DEPTH_12BIT = 3,
} GxAvHdmiColorDepth;

typedef enum videoout_hdmi_ssc_mode {
	HDMI_CLOSE_SPREAD,
	HDMI_CENTER_SPREAD,
	HDMI_DOWN_SPREAD,
} GxAvHdmiSSCMode;

typedef struct gxav_rational {
    int num; ///< numerator
    int den; ///< denominator
} GxAvRational;

enum av_hdcp_state {
	AV_HDCP_IDLE = -1,
	AV_HDCP_SUCCESS = 0,
	AV_HDCP_FAILED = 1,
};

typedef struct gxav_hdcp_state {
	enum av_hdcp_state state;
} GxAvHDCPState;

/**
* Chromaticity coordinates of the source primaries.
* These values match the ones defined by ISO/IEC 23001-8_2013 § 7.1.
430   */
enum GxColorPrimaries {
	GXCOL_PRI_RESERVED0    = 0,
	GXCOL_PRI_BT709        = 1,  ///< also ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B
	GXCOL_PRI_UNSPECIFIED  = 2,
	GXCOL_PRI_RESERVED     = 3,
	GXCOL_PRI_BT470M       = 4,  ///< also FCC Title 47 Code of Federal Regulations 73.682 (a)(20)
	GXCOL_PRI_BT470BG      = 5,  ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM
	GXCOL_PRI_SMPTE170M    = 6,  ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
	GXCOL_PRI_SMPTE240M    = 7,  ///< functionally identical to above
	GXCOL_PRI_FILM         = 8,  ///< colour filters using Illuminant C
	GXCOL_PRI_BT2020       = 9,  ///< ITU-R BT2020
	GXCOL_PRI_SMPTE428     = 10, ///< SMPTE ST 428-1 (CIE 1931 XYZ)
	GXCOL_PRI_SMPTEST428_1 = GXCOL_PRI_SMPTE428,
	GXCOL_PRI_SMPTE431     = 11, ///< SMPTE ST 431-2 (2011) / DCI P3
	GXCOL_PRI_SMPTE432     = 12, ///< SMPTE ST 432-1 (2010) / P3 D65 / Display P3
	GXCOL_PRI_JEDEC_P22    = 22, ///< JEDEC P22 phosphors
	GXCOL_PRI_NB                ///< Not part of ABI
};

/**
 * Color Transfer Characteristic.
 * These values match the ones defined by ISO/IEC 23001-8_2013 § 7.2.
 */
enum GxColorTransferCharacteristic {
	GXCOL_TRC_RESERVED0    = 0,
	GXCOL_TRC_BT709        = 1,  ///< also ITU-R BT1361
	GXCOL_TRC_UNSPECIFIED  = 2,
	GXCOL_TRC_RESERVED     = 3,
	GXCOL_TRC_GAMMA22      = 4,  ///< also ITU-R BT470M / ITU-R BT1700 625 PAL & SECAM
	GXCOL_TRC_GAMMA28      = 5,  ///< also ITU-R BT470BG
	GXCOL_TRC_SMPTE170M    = 6,  ///< also ITU-R BT601-6 525 or 625 / ITU-R BT1358 525 or 625 / ITU-R BT1700 NTSC
	GXCOL_TRC_SMPTE240M    = 7,
	GXCOL_TRC_LINEAR       = 8,  ///< "Linear transfer characteristics"
	GXCOL_TRC_LOG          = 9,  ///< "Logarithmic transfer characteristic (100:1 range)"
	GXCOL_TRC_LOG_SQRT     = 10, ///< "Logarithmic transfer characteristic (100 * Sqrt(10) : 1 range)"
	GXCOL_TRC_IEC61966_2_4 = 11, ///< IEC 61966-2-4
	GXCOL_TRC_BT1361_ECG   = 12, ///< ITU-R BT1361 Extended Colour Gamut
	GXCOL_TRC_IEC61966_2_1 = 13, ///< IEC 61966-2-1 (sRGB or sYCC)
	GXCOL_TRC_BT2020_10    = 14, ///< ITU-R BT2020 for 10-bit system
	GXCOL_TRC_BT2020_12    = 15, ///< ITU-R BT2020 for 12-bit system
	GXCOL_TRC_SMPTE2084    = 16, ///< SMPTE ST 2084 for 10-, 12-, 14- and 16-bit systems
	GXCOL_TRC_SMPTEST2084  = GXCOL_TRC_SMPTE2084,
	GXCOL_TRC_SMPTE428     = 17, ///< SMPTE ST 428-1
	GXCOL_TRC_SMPTEST428_1 = GXCOL_TRC_SMPTE428,
	GXCOL_TRC_ARIB_STD_B67 = 18, ///< ARIB STD-B67, known as "Hybrid log-gamma"
	GXCOL_TRC_NB                 ///< Not part of ABI
};

typedef enum {
	GX_LAYER_OSD  ,
	GX_LAYER_VPP  ,
	GX_LAYER_VPP1 ,
	GX_LAYER_VPP2 ,
	GX_LAYER_SPP  ,
	GX_LAYER_SOSD ,
	GX_LAYER_SOSD2,
	GX_LAYER_BKG  ,
	GX_LAYER_MAX  ,
	/*just for capture*/
	GX_LAYER_MIX_VPP_BKG,
	GX_LAYER_MIX_SPP_BKG,
	GX_LAYER_MIX_ALL,
	GX_LAYER_NONE,
}GxLayerID;

typedef union  {
	struct {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
	};
	struct {
		unsigned char y;
		unsigned char cb;
		unsigned char cr;
		unsigned char alpha;
	};
	unsigned char entry;
} GxColor;

struct gxav_module_resource {
	unsigned int regs[6];
	unsigned int irqs[4];
	unsigned int clks[1];
};

#define __maybe_unused  __attribute__((unused))

#endif

