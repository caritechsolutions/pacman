#include "../jpeg/gx3201_jpeg.h"
#include "stdio.h"
#include "io.h"
#include "string.h"
#include "time.h"
#include "cpu_config.h"
#include "gx_api.h"
#include "dw_hdmi/dw_hdmi.h"
#include "gx_firewall.h"
#include "../clock/clock.h"
#if defined (CONFIG_ARCH_CKMMU_CYGNUS) || defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA) || defined (CONFIG_ARCH_CKMMU_SCORPIO)
#include "cygnus_vpu_vout.h"
#elif defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
#include "clock_feature.h"
#include "gxav_vout_propertytypes.h"
#include "vpu_feature.h"
#include "gx_mem_info.h"
#endif /*CONFIG_ARCH_CKMMU_CYGNUS*/

#define VIDEOOUT_PRINTF(fmt, args...) \
	do { \
		printf("\n[VIDEOOUT][%s():%d]: ", __func__, __LINE__); \
		printf(fmt, ##args); \
	} while(0)

#if !defined (CONFIG_ARCH_CKMMU_CYGNUS) && !defined(CONFIG_ARCH_ARM_CANOPUS) && !defined(CONFIG_ARCH_ARM_VEGA) && !defined (CONFIG_ARCH_CKMMU_SCORPIO)
#define gx_iowrite32(value, addr)  (*(volatile int *)(addr)=(value))
#endif /*CONFIG_ARCH_CKMMU_CYGNUS*/

#define SVPU_SURFACE_WIDTH         (720)
#define SVPU_SURFACE_HEIGHT        (576)
#define SVPU_SURFACE_BPP           (24)
#define SVPU_SURFACE_NUM           (3)
#if defined (CONFIG_ARCH_ARM_SIRIUS) || defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA)
#define VOUT_HVSYNC_REGS           (0x8a804070)
#define VOUT_BOTTOM_LINE_REGS      (0x8a804098)
#else
#define VOUT_HVSYNC_REGS           (0xa4804070)
#define VOUT_BOTTOM_LINE_REGS      (0xa4804098)
#endif

typedef struct svpu_surface_info {
	unsigned int buf_addrs[SVPU_SURFACE_NUM];
	unsigned int buf_size;
}SvpuSurfaceInfo;

typedef enum {
	SVPU_LAYER_MIX,
	SVPU_LAYER_SPP,
}SvpuLayerSelect;

typedef struct svpu_buf {
	unsigned int y_topfield_addr;
	unsigned int y_bottomfield_addr;
	unsigned int u_topfield_addr;
	unsigned int u_bottomfield_addr;
	unsigned int v_topfield_addr;
	unsigned int v_bottomfield_addr;
}SvpuBuf;

typedef struct gx3211_svpu_reg {
	SvpuBuf      rBUF[SVPU_SURFACE_NUM];//0x0000~0x0044
	unsigned int rRESERVED_0[2];//0x0048~0x004C

	unsigned int rCAP_CTRL;//0x0050
	unsigned int rRESERVED_1[1];
	unsigned int rCAP_H;
	unsigned int rCAP_V;
	unsigned int rSCE_ZOOM;//0x0060
	unsigned int rSCE_ZOOM1;
	unsigned int rREQ_LENGTH;
	unsigned int rRESERVED_2[1];
	unsigned int rZOOM_PHASE;//0x0070
	unsigned int rSYS_PARA;
	unsigned int rPARA_UPDATE;
	unsigned int rRESERVED_3[1];
	unsigned int rDISP_REQ_BLOCK;//0x0080
	unsigned int rDISP_CTRL;
	unsigned int rVIEW_V;
	unsigned int rVIEW_H;
	unsigned int rVPU_FRAME_MODE;//0x0090
	unsigned int rRESERVED_4[3];
	unsigned int rSCE_PHASE_PARA[64];//0x0100~0x01FC
}Gx3211SvpuReg;

typedef struct gx3211_videoout_reg {
	unsigned int   tv_ctrl        ;//00
	unsigned int   tv_dac         ;
	unsigned int   tv_hsync       ;
	unsigned int   tv_active      ;
	unsigned int   tv_fir_y0_2    ;//10
	unsigned int   tv_fir_y3_5    ;
	unsigned int   tv_fir_y6_8    ;
	unsigned int   tv_fir_y9      ;
	unsigned int   tv_fir_c0_2    ;//20
	unsigned int   tv_fir_c3_5    ;
	unsigned int   colr_tran      ;
	unsigned int   line_pixel_num ;
	unsigned int   tv_range       ;//30
	unsigned int   tv_adjust      ;
	unsigned int   tv_test        ;
	unsigned int   color_tran_c   ;
	unsigned int   pposd_delay    ;//40
	unsigned int   sog_enable     ;
	unsigned int   vsync_width    ;
	unsigned int   vbi_cc_ctrl    ;
	unsigned int   vbi_cc_data    ;//50
	unsigned int   vbi_wss_ctrl   ;
	unsigned int   vbi_vps_ctrl   ;
	unsigned int   vbi_cgms_ctrl  ;
	unsigned int   vbi_wss_data   ;//60
	unsigned int   vbi_cgms_data  ;
	unsigned int   vbi_tlx_ctrl1  ;
	unsigned int   vbi_tlx_ctrl2  ;
	unsigned int   scart_ctrl     ;//70
	unsigned int   dgdp_enable    ;
	unsigned int   dgdp_ctrl_reg1 ;
	unsigned int   dgdp_ctrl_reg2 ;
	unsigned int   dgdp_ctrl_reg3 ;//80
	unsigned int   dgdp_ctrl_reg4 ;
	unsigned int   tri_sync_ctrl  ;
	unsigned int   lcd_vsync_width;
	unsigned int   digital_ctrl   ;//90
	unsigned int   bt656_ctrl0    ;
	unsigned int   bt656_ctrl1    ;
	unsigned int   bt656_ctrl2    ;
	unsigned int   lcd_line_ctrl  ;//a0
	unsigned int   dgdp0_enable   ;
	unsigned int   dgdp0_ctrl_reg1;
	unsigned int   dgdp0_ctrl_reg2;
	unsigned int   dgdp0_ctrl_reg3;//b0
	unsigned int   dgdp0_ctrl_reg4;
	unsigned int   bt656_tran_para;
	unsigned int   aps_ctrl0      ;
	unsigned int   aps_ctrl1      ;//c0
	unsigned int   aps_ctrl2      ;
	unsigned int   aps_ctrl3      ;
	unsigned int   aps_reg0       ;
	unsigned int   aps_reg1       ;//d0
	unsigned int   aps_reg2       ;
	unsigned int   aps_reg3       ;
	unsigned int   aps_reg4       ;
	unsigned int   dcs_ctrl4      ;//e0
	unsigned int   aps_ctrl_data_l;
	unsigned int   aps_ctrl_data_h;
} gx3211_videoout_reg;

#define bSVPU_CAP_EN      (0)
#define SVPU_CAP_ENABLE(reg)\
	REG_SET_BIT(&(reg), bSVPU_CAP_EN);
#define SVPU_CAP_DISABLE(reg)\
	REG_CLR_BIT(&(reg), bSVPU_CAP_EN);
#define bSVPU_DISP_EN     (31)
#define SVPU_DISP_ENABLE(reg)\
	REG_SET_BIT(&(reg), bSVPU_DISP_EN);
#define SVPU_DISP_DISABLE(reg)\
	REG_CLR_BIT(&(reg), bSVPU_DISP_EN);

#define bSVPU_CAPMODE     (24)
#define mSVPU_CAPMODE     (0x1f<<bSVPU_CAPMODE)
#define SVPU_SET_CAPMODE(reg, capmode)\
	REG_SET_FIELD(&(reg), mSVPU_CAPMODE, capmode, bSVPU_CAPMODE)

#define bSVPU_CAPMODE_EN  (31)
#define SVPU_SET_CAPMODE_ENABLE(reg)\
	REG_SET_BIT(&(reg), bSVPU_CAPMODE_EN);
#define SVPU_SET_CAPMODE_DISABLE(reg)\
	REG_CLR_BIT(&(reg), bSVPU_CAPMODE_EN);

#define bSVPU_CAPLEVEL    (1)
#define mSVPU_CAPLEVEL    (0x1<<bSVPU_CAPLEVEL)
#define SVPU_SET_CAPLEVEL(reg, caplevel)\
	REG_SET_FIELD(&(reg), mSVPU_CAPLEVEL, caplevel, bSVPU_CAPLEVEL)

#define bSVPU_W_REQBLOCK  (16)
#define mSVPU_W_REQBLOCK  (0x7ff<<bSVPU_W_REQBLOCK)
#define SVPU_SET_BUFS_WRITE_REQBLOCK(reg, reqblock)\
	REG_SET_FIELD(&(reg), mSVPU_W_REQBLOCK, reqblock, bSVPU_W_REQBLOCK)

#define bSVPU_R_REQBLOCK  (16)
#define mSVPU_R_REQBLOCK  (0x7ff<<bSVPU_R_REQBLOCK)
#define SVPU_SET_BUFS_READ_REQBLOCK(reg, reqblock)\
	REG_SET_FIELD(&(reg), mSVPU_R_REQBLOCK, reqblock, bSVPU_R_REQBLOCK)

#define bSVPU_CAP_LEFT    (0)
#define mSVPU_CAP_LEFT    (0x1<<bSVPU_CAP_LEFT)
#define SVPU_SET_CAP_LEFT(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_CAP_LEFT, value, bSVPU_CAP_LEFT)

#define bSVPU_CAP_RIGHT    (16)
#define mSVPU_CAP_RIGHT    (0x1<<bSVPU_CAP_RIGHT)
#define SVPU_SET_CAP_RIGHT(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_CAP_RIGHT, value, bSVPU_CAP_RIGHT)

#define bSVPU_CAP_TOP    (0)
#define mSVPU_CAP_TOP    (0x1<<bSVPU_CAP_TOP)
#define SVPU_SET_CAP_TOP(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_CAP_TOP, value, bSVPU_CAP_TOP)

#define bSVPU_CAP_BOTTOM    (16)
#define mSVPU_CAP_BOTTOM    (0x1<<bSVPU_CAP_BOTTOM)
#define SVPU_SET_CAP_BOTTOM(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_CAP_BOTTOM, value, bSVPU_CAP_BOTTOM)

#define bSVPU_SCE_VZOOM    (0)
#define mSVPU_SCE_VZOOM    (0x1<<bSVPU_SCE_VZOOM)
#define SVPU_SET_SCE_VZOOM(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SCE_VZOOM, value, bSVPU_SCE_VZOOM)

#define bSVPU_SCE_HZOOM    (16)
#define mSVPU_SCE_HZOOM    (0x1<<bSVPU_SCE_HZOOM)
#define SVPU_SET_SCE_HZOOM(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SCE_HZOOM, value, bSVPU_SCE_HZOOM)

#define bSVPU_SCE_ZOOM_LENGTH    (0)
#define mSVPU_SCE_ZOOM_LENGTH    (0x1<<bSVPU_SCE_ZOOM_LENGTH)
#define SVPU_SET_SCE_ZOOM_LENGTH(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SCE_ZOOM_LENGTH, value, bSVPU_SCE_ZOOM_LENGTH)

#define bSVPU_SCE_ZOOM_HEIGHT    (16)
#define mSVPU_SCE_ZOOM_HEIGHT    (0x1<<bSVPU_SCE_ZOOM_HEIGHT)
#define SVPU_SET_SCE_ZOOM_HEIGHT(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SCE_ZOOM_HEIGHT, value, bSVPU_SCE_ZOOM_HEIGHT)

#define bSVPU_SCE_VT_PHASE    (0)
#define mSVPU_SCE_VT_PHASE    (0x1<<bSVPU_SCE_VT_PHASE)
#define SVPU_SET_SCE_VT_PHASE(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SCE_VT_PHASE, value, bSVPU_SCE_VT_PHASE)

#define bSVPU_SCE_VB_PHASE    (16)
#define mSVPU_SCE_VB_PHASE    (0x1<<bSVPU_SCE_VB_PHASE)
#define SVPU_SET_SCE_VB_PHASE(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SCE_VB_PHASE, value, bSVPU_SCE_VB_PHASE)

#define bSVPU_SPP_LEFT    (16)
#define mSVPU_SPP_LEFT    (0x1<<bSVPU_SPP_LEFT)
#define SVPU_SET_SPP_LEFT(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SPP_LEFT, value, bSVPU_SPP_LEFT)

#define bSVPU_SPP_RIGHT    (0)
#define mSVPU_SPP_RIGHT    (0x1<<bSVPU_SPP_RIGHT)
#define SVPU_SET_SPP_RIGHT(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SPP_RIGHT, value, bSVPU_SPP_RIGHT)

#define bSVPU_SPP_TOP    (16)
#define mSVPU_SPP_TOP    (0x1<<bSVPU_SPP_TOP)
#define SVPU_SET_SPP_TOP(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SPP_TOP, value, bSVPU_SPP_TOP)

#define bSVPU_SPP_BOTTOM    (0)
#define mSVPU_SPP_BOTTOM    (0x1<<bSVPU_SPP_BOTTOM)
#define SVPU_SET_SPP_BOTTOM(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_CAP_BOTTOM, value, bSVPU_CAP_BOTTOM)

#define bSVPU_VDOWNSCALE_EN      (4)
#define SVPU_VDOWNSCALE_ENABLE(reg)\
	REG_SET_BIT(&(reg), bSVPU_VDOWNSCALE_EN);
#define SVPU_VDOWNSCALE_DISABLE(reg)\
	REG_CLR_BIT(&(reg), bSVPU_VDOWNSCALE_EN);

typedef enum {
	FRAME_MODE_2 = 0,
	FRAME_MODE_3 = 1,
}SvpuFrameMode;
#define bSVPU_FRAME_MODE (0)
#define SVPU_SET_FRAME_MODE(reg, mode)\
	if(mode == FRAME_MODE_2)\
		REG_CLR_BIT(&(reg), bSVPU_FRAME_MODE);\
	else\
		REG_SET_BIT(&(reg), bSVPU_FRAME_MODE);

#define SVPU_PARA_UPDATE(reg)\
	do {\
		REG_SET_VAL(&(reg), 1);\
		REG_SET_VAL(&(reg), 0);\
	}while(0)

#define bTV_CTRL_TV_FORMAT              (0)
#define bTV_CTRL_FRAME_FRE              (5)
#define bTV_CTRL_DAC_OUT_MODE           (17)
#define bTV_CTRL_DAC_FULLCASE_CHANGE    (25)

#define VOUT_0 (0)
#define VOUT_1 (1)

/* options: hdcp, 3d X, next(mode), modes(all), mode X*/
struct gxav_hdmidev{
	videoParams_t                 video;
	audioParams_t                 audio;
	productParams_t               product;
	u16                           ceacode;
};

struct hdmi_dtd_map{
	u8                      code;
	GxVideoOutProperty_Mode mode;
};

static volatile struct gxav_hdmidev g_hdmidev;
#define HDMI_GET_PDEV() ((struct gxav_hdmidev *)(&g_hdmidev))

static struct hdmi_dtd_map gn_dtd_map[] = {
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	{2  ,GXAV_VOUT_480P        },
	{6  ,GXAV_VOUT_480I        },
	{21 ,GXAV_VOUT_576I        },
	{17 ,GXAV_VOUT_576P        },
	{19 ,GXAV_VOUT_720P_50HZ   },
	{4  ,GXAV_VOUT_720P_60HZ   },
	{20 ,GXAV_VOUT_1080I_50HZ  },
	{5  ,GXAV_VOUT_1080I_60HZ  },
	{31 ,GXAV_VOUT_1080P_50HZ  },
	{16 ,GXAV_VOUT_1080P_60HZ  }
#else /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
	{2  ,GXAV_VOUT_HDMI_480P        },
	{6  ,GXAV_VOUT_HDMI_480I        },
	{21 ,GXAV_VOUT_HDMI_576I        },
	{17 ,GXAV_VOUT_HDMI_576P        },
	{19 ,GXAV_VOUT_HDMI_720P_50HZ   },
	{4  ,GXAV_VOUT_HDMI_720P_60HZ   },
	{20 ,GXAV_VOUT_HDMI_1080I_50HZ  },
	{5  ,GXAV_VOUT_HDMI_1080I_60HZ  },
	{31 ,GXAV_VOUT_HDMI_1080P_50HZ  },
	{16 ,GXAV_VOUT_HDMI_1080P_60HZ  }
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
};

static int gx3201_videoout_pal_config(int id, enum cvbs_mode_enum encoding_mode)
{
	volatile unsigned int vout_addr = 0;

	switch (id) {
	case VOUT_0:
		vout_addr = VPU_VOUT_BASE_ADDR;
		break;
	case VOUT_1:
		vout_addr = SVPU_VOUT_BASE_ADDR;
		break;
	default:
		return -1;
	}

	switch (encoding_mode) {
	case G_PAL:
		REG_SET_FIELD(vout_addr, 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|0, bTV_CTRL_TV_FORMAT);
		REG_SET_BIT(vout_addr, 31);
		break;
	case G_PAL_N:
		REG_SET_FIELD(vout_addr, 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|2, bTV_CTRL_TV_FORMAT);
		REG_SET_BIT(vout_addr, 31);
		break;
	case G_PAL_NC:
		REG_SET_FIELD(vout_addr, 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|3, bTV_CTRL_TV_FORMAT);
		REG_SET_BIT(vout_addr, 31);
		break;
	default:
		VIDEOOUT_PRINTF("unsupport dve in pal\n");
		return -1;
	}

	return 0;
}

static int gx3201_videoout_ntsc_config(int id, enum cvbs_mode_enum encoding_mode)
{
	volatile unsigned int vout_addr = 0;

	switch (id) {
	case VOUT_0:
		vout_addr = VPU_VOUT_BASE_ADDR;
		break;
	case VOUT_1:
		vout_addr = SVPU_VOUT_BASE_ADDR;
		break;
	default:
		return -1;
	}

	switch(encoding_mode) {
	case G_NTSC_M:
		REG_SET_FIELD(vout_addr, 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|4, bTV_CTRL_TV_FORMAT);
		REG_SET_BIT(vout_addr, 31);
		break;
	case G_NTSC_443:
		REG_SET_FIELD(vout_addr, 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|5, bTV_CTRL_TV_FORMAT);
		REG_SET_BIT(vout_addr, 31);
		break;
	case G_PAL_M:
		REG_SET_FIELD(vout_addr, 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|1, bTV_CTRL_TV_FORMAT);
		REG_SET_BIT(vout_addr, 31);
		break;
	default:
		VIDEOOUT_PRINTF("unsupport dve in ntsc\n");
		return -1;
	}

	return 0;
}

static int gx3201_videoout_yuv_config(int id, enum ypbpr_hdmi_mode_enum encoding_mode)
{
	unsigned dac_case = 0;
	volatile unsigned int vout_addr = 0;

	if (id == VOUT_1) {
		VIDEOOUT_PRINTF("vout1 unsupport dve in yuv\n");
		return -1;
	}

	vout_addr = VPU_VOUT_BASE_ADDR;
	dac_case = REG_GET_FIELD(vout_addr, 0x1f<<bTV_CTRL_DAC_FULLCASE_CHANGE, bTV_CTRL_DAC_FULLCASE_CHANGE);

	switch(encoding_mode) {
	case G_YPBPR_HDMI_480I:
		REG_SET_FIELD(vout_addr, 0x7 << bTV_CTRL_FRAME_FRE, 0, bTV_CTRL_FRAME_FRE);
		REG_SET_FIELD(vout_addr, 0xf << bTV_CTRL_TV_FORMAT, 4, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(vout_addr, 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		REG_SET_FIELD(vout_addr, 0x1f<<bTV_CTRL_DAC_FULLCASE_CHANGE, dac_case, bTV_CTRL_DAC_FULLCASE_CHANGE);
		break;
	case G_YPBPR_HDMI_576I:
		REG_SET_FIELD(vout_addr, 0x7 << bTV_CTRL_FRAME_FRE, 0, bTV_CTRL_FRAME_FRE);
		REG_SET_FIELD(vout_addr, 0xf << bTV_CTRL_TV_FORMAT, 0, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(vout_addr, 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		REG_SET_FIELD(vout_addr, 0x1f<<bTV_CTRL_DAC_FULLCASE_CHANGE, dac_case, bTV_CTRL_DAC_FULLCASE_CHANGE);
		break;
	case G_YPBPR_HDMI_480P:
		REG_SET_FIELD(vout_addr, 0x7 << bTV_CTRL_FRAME_FRE, 0, bTV_CTRL_FRAME_FRE);
		REG_SET_FIELD(vout_addr, 0xf << bTV_CTRL_TV_FORMAT, 8, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(vout_addr, 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		REG_SET_FIELD(vout_addr, 0x1f<<bTV_CTRL_DAC_FULLCASE_CHANGE, dac_case, bTV_CTRL_DAC_FULLCASE_CHANGE);
		break;
	case G_YPBPR_HDMI_576P:
		REG_SET_FIELD(vout_addr, 0x7 << bTV_CTRL_FRAME_FRE, 0, bTV_CTRL_FRAME_FRE);
		REG_SET_FIELD(vout_addr, 0xf << bTV_CTRL_TV_FORMAT, 9, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(vout_addr, 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		REG_SET_FIELD(vout_addr, 0x1f<<bTV_CTRL_DAC_FULLCASE_CHANGE, dac_case, bTV_CTRL_DAC_FULLCASE_CHANGE);
		break;
	case G_YPBPR_HDMI_720P_50HZ:
		REG_SET_FIELD(vout_addr, 0x7 << bTV_CTRL_FRAME_FRE, 1, bTV_CTRL_FRAME_FRE);
		REG_SET_FIELD(vout_addr, 0xf << bTV_CTRL_TV_FORMAT, 13, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(vout_addr, 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		REG_SET_FIELD(vout_addr, 0x1f<<bTV_CTRL_DAC_FULLCASE_CHANGE, dac_case, bTV_CTRL_DAC_FULLCASE_CHANGE);
		break;
	case G_YPBPR_HDMI_720P_60HZ:
		REG_SET_FIELD(vout_addr, 0x7 << bTV_CTRL_FRAME_FRE, 0, bTV_CTRL_FRAME_FRE);
		REG_SET_FIELD(vout_addr, 0xf << bTV_CTRL_TV_FORMAT, 13, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(vout_addr, 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		REG_SET_FIELD(vout_addr, 0x1f<<bTV_CTRL_DAC_FULLCASE_CHANGE, dac_case, bTV_CTRL_DAC_FULLCASE_CHANGE);
		break;
	case G_YPBPR_HDMI_1080I_50HZ:
		REG_SET_FIELD(vout_addr, 0x7 << bTV_CTRL_FRAME_FRE, 1, bTV_CTRL_FRAME_FRE);
		REG_SET_FIELD(vout_addr, 0xf << bTV_CTRL_TV_FORMAT, 6, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(vout_addr, 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		REG_SET_FIELD(vout_addr, 0x1f<<bTV_CTRL_DAC_FULLCASE_CHANGE, dac_case, bTV_CTRL_DAC_FULLCASE_CHANGE);
		break;
	case G_YPBPR_HDMI_1080I_60HZ:
		REG_SET_FIELD(vout_addr, 0x7 << bTV_CTRL_FRAME_FRE, 0, bTV_CTRL_FRAME_FRE);
		REG_SET_FIELD(vout_addr, 0xf << bTV_CTRL_TV_FORMAT, 6, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(vout_addr, 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		REG_SET_FIELD(vout_addr, 0x1f<<bTV_CTRL_DAC_FULLCASE_CHANGE, dac_case, bTV_CTRL_DAC_FULLCASE_CHANGE);
		break;
	case G_YPBPR_HDMI_1080P_50HZ:
		REG_SET_FIELD(vout_addr, 0x7 << bTV_CTRL_FRAME_FRE, 1, bTV_CTRL_FRAME_FRE);
		REG_SET_FIELD(vout_addr, 0xf << bTV_CTRL_TV_FORMAT, 14, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(vout_addr, 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		REG_SET_FIELD(vout_addr, 0x1f<<bTV_CTRL_DAC_FULLCASE_CHANGE, dac_case, bTV_CTRL_DAC_FULLCASE_CHANGE);
		break;
	case G_YPBPR_HDMI_1080P_60HZ:
		REG_SET_FIELD(vout_addr, 0x7 << bTV_CTRL_FRAME_FRE, 0, bTV_CTRL_FRAME_FRE);
		REG_SET_FIELD(vout_addr, 0xf << bTV_CTRL_TV_FORMAT, 14, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(vout_addr, 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		REG_SET_FIELD(vout_addr, 0x1f<<bTV_CTRL_DAC_FULLCASE_CHANGE, dac_case, bTV_CTRL_DAC_FULLCASE_CHANGE);
		break;
	default:
		VIDEOOUT_PRINTF("unsupport dve in yuv\n");
		return -1;
	}

	REG_CLR_BIT(vout_addr, 30);
	REG_SET_BIT(vout_addr, 31);

	return 0;
}

extern enum cvbs_mode_enum g_cvbs_mode;
extern enum ypbpr_hdmi_mode_enum g_ypbpr_hdmi_mode;
static GxVideoOutProperty_Mode vpu_hdmi_vout_mode = 0;
#if defined (CONFIG_ARCH_ARM_VEGA)
static volatile struct config_regs *gx3201_config_reg = (void *)CONFIG_CLOCK_BASE;
#else
static volatile struct config_regs *gx3201_config_reg = (void *)CONFIG_BASE_MMU;
#endif /*CONFIG_ARCH_ARM_VEGA*/
#if defined(CONFIG_ARCH_CKMMU_CYGNUS) || defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA) || defined(CONFIG_ARCH_CKMMU_SCORPIO)
volatile CygnusOsdReg *cygnusosd_reg = (CygnusOsdReg *)(VPU_BASE_ADDR + 0x4000);
#else
volatile Gx3201VpuReg *gx3201vpu_reg = (void *)VPU_BASE_ADDR;
#endif /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO*/
static volatile Gx3211SvpuReg *gx3211svpu_reg = (Gx3211SvpuReg *)SVPU_BASE_ADDR;
static volatile gx3211_videoout_reg* gx3211_vout1_reg = (gx3211_videoout_reg *)SVPU_VOUT_BASE_ADDR;
unsigned int g_hdmi_output_w;
unsigned int g_hdmi_output_h;

static unsigned int sPhaseNomal[] = {
	0x10909010,0x00ff0100,0x01ff0200,0x02ff0300,
	0x03ff0400,0x04ff0500,0x05fe0700,0x06fe0800,
	0x07fd0a00,0x07fc0b00,0x08fc0c00,0x09fb0e00,
	0x09fa1001,0x0af91201,0x0bf81401,0x0bf71501,
	0x0cf61701,0x0cf51801,0x0df41b02,0x0df31c02,
	0x0ef11f02,0x0ef02002,0x0fef2303,0x0fed2503,
	0x10ec2703,0x10ea2903,0x10e92b04,0x10e72d04,
	0x11e53004,0x11e33305,0x11e13505,0x11df3705,
	0x12de3a06,0x12dc3c06,0x12da3e06,0x12d74106,
	0x12d54407,0x12d34607,0x12d14807,0x12cf4b08,
	0x12cd4d08,0x12ca5008,0x12c85309,0x12c65509,
	0x12c35809,0x12c15b0a,0x12bf5d0a,0x12bc600a,
	0x12ba630b,0x12b7660b,0x12b5680b,0x12b26b0b,
	0x12b06e0c,0x12ad710c,0x12aa750d,0x11a8760d,
	0x11a5790d,0x11a27d0e,0x11a07f0e,0x119d820e,
	0x109a840e,0x1098870f,0x10958a0f,0x10928d0f,
};

#if 0
static unsigned int gx3201_osd_filter_table[] = {
	0x808000,0x7e8200,0x7c8400,0x7a8600,
	0x788800,0x768a00,0x748c00,0x728e00,
	0x709000,0x6e9200,0x6c9400,0x6a9600,
	0x689800,0x669a00,0x649c00,0x629e00,
	0x60a000,0x5ea200,0x5ca400,0x5aa600,
	0x58a800,0x56aa00,0x54ac00,0x52ae00,
	0x50b000,0x4eb200,0x4cb400,0x4ab600,
	0x48b800,0x46ba00,0x44bc00,0x42be00,
	0x40c000,0x3ec200,0x3cc400,0x3ac600,
	0x38c800,0x36ca00,0x34cc00,0x32ce00,
	0x30d000,0x2ed200,0x2cd400,0x2ad600,
	0x28d800,0x26da00,0x24dc00,0x22de00,
	0x20e000,0x1ee200,0x1ce400,0x1ae600,
	0x18e800,0x16ea00,0x14ec00,0x12ee00,
	0x10f000,0x0ef200,0x0cf400,0x0af600,
	0x08f800,0x06fa00,0x04fc00,0x02fe00,
};
#endif

static void Gx3201_Vpu_SetBufferStateDelay(int v)
{
#if defined(CONFIG_ARCH_CKMMU_CYGNUS) || defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA) || defined(CONFIG_ARCH_CKMMU_SCORPIO)
#else
        VPU_SET_BUFF_STATE_DELAY(gx3201vpu_reg->rBUFF_CTRL2, v);
#endif /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO*/
}

static void config_videoout_ClockDIVConfig(unsigned int clock, unsigned int div)
{
        if (div)
                REG_SET_BIT(&(gx3201_config_reg->cfg_clk), clock);
        else
                REG_CLR_BIT(&(gx3201_config_reg->cfg_clk), clock);
}

#if (defined(CONFIG_ARCH_CKMMU_GEMINI) || defined(CONFIG_ARCH_CKMMU_CYGNUS)) || defined(CONFIG_ARCH_ARM_CANOPUS) ||defined (CONFIG_ARCH_ARM_VEGA) || defined (CONFIG_ARCH_CKMMU_SCORPIO)
static void config_videoout_ClockDIVConfig2(unsigned int clock, unsigned int div)
{
#if (defined(CONFIG_ARCH_CKMMU_GEMINI) || defined(CONFIG_ARCH_CKMMU_CYGNUS) || defined(CONFIG_ARCH_CKMMU_SCORPIO))
	volatile unsigned int * div_config2 = (volatile unsigned int *)&(gx3201_config_reg->clock_div_config2);
#elif defined(CONFIG_ARCH_ARM_CANOPUS)
	volatile unsigned int * div_config2 = (volatile unsigned int *)&(gx3201_config_reg->cfg_clock2);
#elif defined(CONFIG_ARCH_ARM_VEGA)
	volatile unsigned int * div_config2 = (volatile unsigned int *)&(gx3201_config_reg->cfg_clock5);
#endif
        if (div)
                REG_SET_BIT(div_config2, clock);
        else
                REG_CLR_BIT(div_config2, clock);
}

static void config_videoout_ClockDIVConfig3(unsigned int clock, unsigned int div)
{
#if (defined(CONFIG_ARCH_CKMMU_GEMINI) || defined(CONFIG_ARCH_CKMMU_CYGNUS) || defined(CONFIG_ARCH_CKMMU_SCORPIO))
	volatile unsigned int * div_config3 = (volatile unsigned int *)&(gx3201_config_reg->clock_div_config3);
#elif defined(CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA)
	volatile unsigned int * div_config3 = (volatile unsigned int *)&(gx3201_config_reg->cfg_clock3);
#endif
        if (div)
                REG_SET_BIT(div_config3, clock);
        else
                REG_CLR_BIT(div_config3, clock);
}
#endif

static void config_videoout_ClockSourceSel(unsigned int clock, unsigned int source)
{
        if (source)
                REG_SET_BIT(&(gx3201_config_reg->cfg_source_sel), clock);
        else
                REG_CLR_BIT(&(gx3201_config_reg->cfg_source_sel), clock);
}

static void _vpu_reset(void)
{
#ifdef CONFIG_ARCH_ARM_SIRIUS
	REG_SET_BIT(&(gx3201_config_reg->cfg_mpeg_hot_reset), 9);
	REG_CLR_BIT(&(gx3201_config_reg->cfg_mpeg_hot_reset), 9);
#elif defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA)
	REG_SET_BIT(&(gx3201_config_reg->cfg_mpeg_hot_reset_set), 28);
	REG_CLR_BIT(&(gx3201_config_reg->cfg_mpeg_hot_reset_set), 28);
#else
	REG_SET_BIT(&(gx3201_config_reg->cfg_mpeg_hot_reset), 28);
	REG_CLR_BIT(&(gx3201_config_reg->cfg_mpeg_hot_reset), 28);
#endif
	mdelay(20);
}

static void gx3211_config_videoout_Clock(int sub_id, int mode)
{
#if defined(CONFIG_ARCH_CKMMU_SCORPIO)
	if (sub_id == 0) {
		ClockVout vout;
		switch (mode) {
			case GXAV_VOUT_PAL:
			case GXAV_VOUT_PAL_M:
			case GXAV_VOUT_PAL_N:
			case GXAV_VOUT_PAL_NC:
			case GXAV_VOUT_NTSC_M:
			case GXAV_VOUT_NTSC_443:
			case GXAV_VOUT_YCBCR_480I:
			case GXAV_VOUT_YCBCR_576I:
			case GXAV_VOUT_HDMI_480I:
			case GXAV_VOUT_HDMI_576I:
				vout.vpu_up = 27000000;
				vout.vpu    = 13500000;
				vout.dcs    = 27000000;
				vout.hdmi   = 27000000;
				break;
			case GXAV_VOUT_YPBPR_480P:
			case GXAV_VOUT_HDMI_480P:
			case GXAV_VOUT_VGA_480P:
			case GXAV_VOUT_YPBPR_576P:
			case GXAV_VOUT_HDMI_576P:
			case GXAV_VOUT_VGA_576P:
				vout.vpu_up = 54000000;
				vout.vpu    = 27000000;
				vout.dcs    = 27000000;
				vout.hdmi   = 27000000;
				break;
			case GXAV_VOUT_YPBPR_720P_50HZ:
			case GXAV_VOUT_YPBPR_1080I_50HZ:
			case GXAV_VOUT_HDMI_720P_50HZ:
			case GXAV_VOUT_HDMI_1080I_50HZ:
			case GXAV_VOUT_YPBPR_720P_60HZ:
			case GXAV_VOUT_YPBPR_1080I_60HZ:
			case GXAV_VOUT_HDMI_720P_60HZ:
			case GXAV_VOUT_HDMI_1080I_60HZ:
				vout.vpu_up = 148500000;
				vout.vpu    =  74250000;
				vout.dcs    =  74250000;
				vout.hdmi   =  74250000;
				break;
			case GXAV_VOUT_YPBPR_1080P_50HZ:
			case GXAV_VOUT_HDMI_1080P_50HZ:
			case GXAV_VOUT_YPBPR_1080P_60HZ:
			case GXAV_VOUT_HDMI_1080P_60HZ:
			default:
				vout.vpu_up = 148500000;
				vout.vpu    = 148500000;
				vout.dcs    = 148500000;
				vout.hdmi   = 148500000;
				break;
		}
		clock_vout_clk_config(&vout);
	} else if (sub_id == 1) {
		ClockSvpu svpu;
		svpu.svpu_up = 27000000;
		svpu.svpu    = 13500000;
		clock_svpu_clk_config(&svpu);
	}
#else
	if(sub_id == 0){
		switch(mode)
		{
#if (defined(ENABLE_YPBPR_HDMI_480I) || defined(ENABLE_YPBPR_HDMI_576I))
			case GXAV_VOUT_PAL:
			case GXAV_VOUT_PAL_M:
			case GXAV_VOUT_PAL_N:
			case GXAV_VOUT_PAL_NC:
			case GXAV_VOUT_NTSC_M:
			case GXAV_VOUT_NTSC_443:
#if !defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
			case GXAV_VOUT_YCBCR_480I:
			case GXAV_VOUT_YCBCR_576I:
			case GXAV_VOUT_HDMI_480I:
			case GXAV_VOUT_HDMI_576I:
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
#if (defined(CONFIG_ARCH_CKMMU_GEMINI) || defined(CONFIG_ARCH_CKMMU_CYGNUS))
#ifdef CONFIG_ARCH_CKMMU_GEMINI
				Gx3201_Vpu_SetBufferStateDelay(0xc0);
#endif
				config_videoout_ClockSourceSel(3,0); //set vpu clock source -> xtal
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 43;//ratio;
				config_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockSourceSel(3,1); //set vpu clock source -> pll
				config_videoout_ClockDIVConfig3(3,0);
				config_videoout_ClockDIVConfig3(4,0);
				config_videoout_ClockDIVConfig3(5,1);
				config_videoout_ClockDIVConfig3(6,0);
				config_videoout_ClockDIVConfig3(6,1);
#elif defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA)
				config_videoout_ClockSourceSel(9, 0);    // set vpu clock source to xtal
				config_videoout_ClockDIVConfig2(6, 0);  // vpu rst_en  复位有效
				config_videoout_ClockDIVConfig2(7, 0);  // vpu load_en 更新无效
				config_videoout_ClockDIVConfig2(6, 1);  // vpu rst_en  复位无效
				//配置分频系数
#if defined (CONFIG_ARCH_ARM_VEGA)
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock5)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock5)) |= 43;
#else
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock2)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock2)) |= 43;
#endif /*CONFIG_ARCH_ARM_VEGA*/
				config_videoout_ClockDIVConfig2(7, 1);  // load_en 更新有效
				config_videoout_ClockDIVConfig2(7, 0);  // load_en 更新无效
				config_videoout_ClockSourceSel(9, 1);   // set vpu clock source to pll

				config_videoout_ClockDIVConfig2(10, 1);  // VPU pixel 二分频
				config_videoout_ClockDIVConfig2(9, 1);   // VPU DCS 二分频
				config_videoout_ClockDIVConfig2(8, 1);   // HDMI pixel 二分频
				// VPU pixel / DCS HDMI pixel 二分频复位
				config_videoout_ClockDIVConfig2(11, 0);
				config_videoout_ClockDIVConfig2(11, 1);
#else
				Gx3201_Vpu_SetBufferStateDelay(0xc0);
				config_videoout_ClockSourceSel(5,0); //set vpu clock source -> xtal
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 43;//ratio;
				config_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockSourceSel(5,1); //sel vpu clock source -> pll
				config_videoout_ClockDIVConfig(8,0);  //div hdmi
				config_videoout_ClockDIVConfig(9,0);  //div dcs
				config_videoout_ClockDIVConfig(10,1); //div pixel
				config_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				config_videoout_ClockDIVConfig(11,1); //148.5M div rst
#ifndef CONFIG_ARCH_CKMMU_TAURUS
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_resv10[0])) |= (3<<25);
#endif
#endif
				break;
#endif

#if (defined(ENABLE_YPBPR_HDMI_480P))

			case GXAV_VOUT_YPBPR_480P:
			case GXAV_VOUT_HDMI_480P:
			case GXAV_VOUT_VGA_480P:
#if (defined(CONFIG_ARCH_CKMMU_GEMINI) || defined(CONFIG_ARCH_CKMMU_CYGNUS))
#ifdef CONFIG_ARCH_CKMMU_GEMINI
				Gx3201_Vpu_SetBufferStateDelay(0x60);
#endif
				config_videoout_ClockSourceSel(3,0); //set vpu clock source -> xtal
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 21;//ratio;
				config_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockSourceSel(3,1); //set vpu clock source -> pll
				config_videoout_ClockDIVConfig3(3,1);
				config_videoout_ClockDIVConfig3(4,1);
				config_videoout_ClockDIVConfig3(5,1);
				config_videoout_ClockDIVConfig3(6,0);
				config_videoout_ClockDIVConfig3(6,1);
#elif defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA)
				config_videoout_ClockSourceSel(9, 0);    // set vpu clock source to xtal
				config_videoout_ClockDIVConfig2(6, 0);  // vpu rst_en  复位有效
				config_videoout_ClockDIVConfig2(7, 0);  // vpu load_en 更新无效
				config_videoout_ClockDIVConfig2(6, 1);  // vpu rst_en  复位无效
				//配置分频系数
#if defined (CONFIG_ARCH_ARM_VEGA)
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock5)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock5)) |= 21;
#else
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock2)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock2)) |= 21;
#endif /*CONFIG_ARCH_ARM_VEGA*/
				config_videoout_ClockDIVConfig2(7, 1);  // load_en 更新有效
				config_videoout_ClockDIVConfig2(7, 0);  // load_en 更新无效
				config_videoout_ClockSourceSel(9, 1);   // set vpu clock source to pll

				config_videoout_ClockDIVConfig2(10, 1);  // VPU pixel 二分频
				config_videoout_ClockDIVConfig2(9, 1);   // VPU DCS 二分频
				config_videoout_ClockDIVConfig2(8, 1);   // HDMI pixel 二分频
				// VPU pixel / DCS HDMI pixel 二分频复位
				config_videoout_ClockDIVConfig2(11, 0);
				config_videoout_ClockDIVConfig2(11, 1);
#else
				Gx3201_Vpu_SetBufferStateDelay(0x60);
				config_videoout_ClockSourceSel(5,0); //set vpu clock source -> xtal
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) &= ~(0x3f);
#ifdef CONFIG_ARCH_CKMMU_GX6605S
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 43;//ratio;
				config_videoout_ClockDIVConfig(8,0);  //div hdmi
				config_videoout_ClockDIVConfig(9,0);  //div dcs
				config_videoout_ClockDIVConfig(10,0); //div pixel
#else
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 21;//ratio;
				config_videoout_ClockDIVConfig(8,1);  //div hdmi
				config_videoout_ClockDIVConfig(9,1);  //div dcs
				config_videoout_ClockDIVConfig(10,1); //div pixel
#endif
				config_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockSourceSel(5,1); //sel vpu clock source -> pll
				config_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				config_videoout_ClockDIVConfig(11,1); //148.5M div rst

#ifndef CONFIG_ARCH_CKMMU_TAURUS
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_resv10[0])) |= (3<<25);
#endif
#endif
				break;
#endif

#if !defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
#if (defined(ENABLE_YPBPR_HDMI_576P))
			case GXAV_VOUT_YPBPR_576P:
			case GXAV_VOUT_HDMI_576P:
			case GXAV_VOUT_VGA_576P:
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
#if (defined(CONFIG_ARCH_CKMMU_GEMINI) || defined(CONFIG_ARCH_CKMMU_CYGNUS))
#ifdef CONFIG_ARCH_CKMMU_GEMINI
				Gx3201_Vpu_SetBufferStateDelay(0x60);
#endif
				config_videoout_ClockSourceSel(3,0); //set vpu clock source -> xtal
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 21;//ratio;
				config_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockSourceSel(3,1); //set vpu clock source -> pll
				config_videoout_ClockDIVConfig3(3,1);
				config_videoout_ClockDIVConfig3(4,1);
				config_videoout_ClockDIVConfig3(5,1);
				config_videoout_ClockDIVConfig3(6,0);
				config_videoout_ClockDIVConfig3(6,1);
#elif defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA)
				config_videoout_ClockSourceSel(9, 0);    // set vpu clock source to xtal
				config_videoout_ClockDIVConfig2(6, 0);  // vpu rst_en  复位有效
				config_videoout_ClockDIVConfig2(7, 0);  // vpu load_en 更新无效
				config_videoout_ClockDIVConfig2(6, 1);  // vpu rst_en  复位无效
				//配置分频系数
#if defined (CONFIG_ARCH_ARM_VEGA)
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock5)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock5)) |= 21;
#else
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock2)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock2)) |= 21;
#endif /*CONFIG_ARCH_ARM_VEGA*/
				config_videoout_ClockDIVConfig2(7, 1);  // load_en 更新有效
				config_videoout_ClockDIVConfig2(7, 0);  // load_en 更新无效
				config_videoout_ClockSourceSel(9, 1);   // set vpu clock source to pll

				config_videoout_ClockDIVConfig2(10, 1);  // VPU pixel 二分频
				config_videoout_ClockDIVConfig2(9, 1);   // VPU DCS 二分频
				config_videoout_ClockDIVConfig2(8, 1);   // HDMI pixel 二分频
				// VPU pixel / DCS HDMI pixel 二分频复位
				config_videoout_ClockDIVConfig2(11, 0);
				config_videoout_ClockDIVConfig2(11, 1);
#else
				Gx3201_Vpu_SetBufferStateDelay(0x60);
				config_videoout_ClockSourceSel(5,0); //set vpu clock source -> xtal
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) &= ~(0x3f);
#ifdef CONFIG_ARCH_CKMMU_GX6605S
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 43;//ratio;
				config_videoout_ClockDIVConfig(8,0);  //div hdmi
				config_videoout_ClockDIVConfig(9,0);  //div dcs
				config_videoout_ClockDIVConfig(10,0); //div pixel
#else
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 21;//ratio;
				config_videoout_ClockDIVConfig(8,1);  //div hdmi
				config_videoout_ClockDIVConfig(9,1);  //div dcs
				config_videoout_ClockDIVConfig(10,1); //div pixel
#endif
				config_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockSourceSel(5,1); //sel vpu clock source -> pll
				config_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				config_videoout_ClockDIVConfig(11,1); //148.5M div rst

#ifndef CONFIG_ARCH_CKMMU_TAURUS
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_resv10[0])) |= (3<<25);
#endif
#endif
				break;
#endif

#if !defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
#if (defined(ENABLE_YPBPR_HDMI_720P_50HZ) || defined(ENABLE_YPBPR_HDMI_1080I_50HZ))
			case GXAV_VOUT_YPBPR_720P_50HZ:
			case GXAV_VOUT_YPBPR_1080I_50HZ:
			case GXAV_VOUT_HDMI_720P_50HZ:
			case GXAV_VOUT_HDMI_1080I_50HZ:
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
#if (defined(CONFIG_ARCH_CKMMU_GEMINI) || defined(CONFIG_ARCH_CKMMU_CYGNUS))
#ifdef CONFIG_ARCH_CKMMU_GEMINI
				Gx3201_Vpu_SetBufferStateDelay(0x30);
#endif
				config_videoout_ClockSourceSel(3,0); //set vpu clock source -> xtal
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 7;//ratio;
				config_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockSourceSel(3,1); //set vpu clock source -> pll
				config_videoout_ClockDIVConfig3(3,1);
				config_videoout_ClockDIVConfig3(4,1);
				config_videoout_ClockDIVConfig3(5,1);
				config_videoout_ClockDIVConfig3(6,0);
				config_videoout_ClockDIVConfig3(6,1);
#elif defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA)
				config_videoout_ClockSourceSel(9, 0);    // set vpu clock source to xtal
				config_videoout_ClockDIVConfig2(6, 0);  // vpu rst_en  复位有效
				config_videoout_ClockDIVConfig2(7, 0);  // vpu load_en 更新无效
				config_videoout_ClockDIVConfig2(6, 1);  // vpu rst_en  复位无效
				//配置分频系数
#if defined (CONFIG_ARCH_ARM_VEGA)
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock5)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock5)) |= 7;
#else
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock2)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock2)) |= 7;
#endif /*CONFIG_ARCH_ARM_VEGA*/
				config_videoout_ClockDIVConfig2(7, 1);  // load_en 更新有效
				config_videoout_ClockDIVConfig2(7, 0);  // load_en 更新无效
				config_videoout_ClockSourceSel(9, 1);   // set vpu clock source to pll

				config_videoout_ClockDIVConfig2(10, 1);  // VPU pixel 二分频
				config_videoout_ClockDIVConfig2(9, 1);   // VPU DCS 二分频
				config_videoout_ClockDIVConfig2(8, 1);   // HDMI pixel 二分频
				// VPU pixel / DCS HDMI pixel 二分频复位
				config_videoout_ClockDIVConfig2(11, 0);
				config_videoout_ClockDIVConfig2(11, 1);
#else
				Gx3201_Vpu_SetBufferStateDelay(0x30);
				config_videoout_ClockSourceSel(5,0); //set vpu clock source -> xtal
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) &= ~(0x3f);
#ifdef CONFIG_ARCH_CKMMU_GX6605S
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 15;//ratio;
				config_videoout_ClockDIVConfig(8,0);  //div hdmi
				config_videoout_ClockDIVConfig(9,0);  //div dcs
				config_videoout_ClockDIVConfig(10,0); //div pixel
#else
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 7;//ratio;
				config_videoout_ClockDIVConfig(8,1);  //div hdmi
				config_videoout_ClockDIVConfig(9,1);  //div dcs
				config_videoout_ClockDIVConfig(10,1); //div pixel
#endif
				config_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockSourceSel(5,1); //sel vpu clock source -> pll
				config_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				config_videoout_ClockDIVConfig(11,1); //148.5M div rst
#ifndef CONFIG_ARCH_CKMMU_TAURUS
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_resv10[0])) |= (3<<25);
#endif
#endif
				break;
#endif

#if !defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
#if (defined(ENABLE_YPBPR_HDMI_720P_60HZ) || defined(ENABLE_YPBPR_HDMI_1080I_60HZ))
			case GXAV_VOUT_YPBPR_720P_60HZ:
			case GXAV_VOUT_YPBPR_1080I_60HZ:
			case GXAV_VOUT_HDMI_720P_60HZ:
			case GXAV_VOUT_HDMI_1080I_60HZ:
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
#if (defined(CONFIG_ARCH_CKMMU_GEMINI) || defined(CONFIG_ARCH_CKMMU_CYGNUS))
#ifdef CONFIG_ARCH_CKMMU_GEMINI
				Gx3201_Vpu_SetBufferStateDelay(0x30);
#endif
				config_videoout_ClockSourceSel(3,0); //set vpu clock source -> xtal
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 7;//ratio;
				config_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockSourceSel(3,1); //set vpu clock source -> xtal
				config_videoout_ClockDIVConfig3(3,1);
				config_videoout_ClockDIVConfig3(4,1);
				config_videoout_ClockDIVConfig3(5,1);
				config_videoout_ClockDIVConfig3(6,0);
				config_videoout_ClockDIVConfig3(6,1);
#elif defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA)
				config_videoout_ClockSourceSel(9, 0);    // set vpu clock source to xtal
				config_videoout_ClockDIVConfig2(6, 0);  // vpu rst_en  复位有效
				config_videoout_ClockDIVConfig2(7, 0);  // vpu load_en 更新无效
				config_videoout_ClockDIVConfig2(6, 1);  // vpu rst_en  复位无效
				//配置分频系数
#if defined (CONFIG_ARCH_ARM_VEGA)
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock5)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock5)) |= 7;
#else
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock2)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock2)) |= 7;
#endif /*CONFIG_ARCH_ARM_VEGA*/
				config_videoout_ClockDIVConfig2(7, 1);  // load_en 更新有效
				config_videoout_ClockDIVConfig2(7, 0);  // load_en 更新无效
				config_videoout_ClockSourceSel(9, 1);   // set vpu clock source to pll

				config_videoout_ClockDIVConfig2(10, 1);  // VPU pixel 二分频
				config_videoout_ClockDIVConfig2(9, 1);   // VPU DCS 二分频
				config_videoout_ClockDIVConfig2(8, 1);   // HDMI pixel 二分频
				// VPU pixel / DCS HDMI pixel 二分频复位
				config_videoout_ClockDIVConfig2(11, 0);
				config_videoout_ClockDIVConfig2(11, 1);
#else
				Gx3201_Vpu_SetBufferStateDelay(0x30);
				config_videoout_ClockSourceSel(5,0); //set vpu clock source -> xtal
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) &= ~(0x3f);
#ifdef CONFIG_ARCH_CKMMU_GX6605S
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 15;//ratio;
				config_videoout_ClockDIVConfig(8,0);  //div hdmi
				config_videoout_ClockDIVConfig(9,0);  //div dcs
				config_videoout_ClockDIVConfig(10,0); //div pixel
#else
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 7;//ratio;
				config_videoout_ClockDIVConfig(8,1);  //div hdmi
				config_videoout_ClockDIVConfig(9,1);  //div dcs
				config_videoout_ClockDIVConfig(10,1); //div pixel
#endif
				config_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockSourceSel(5,1); //sel vpu clock source -> pll
				config_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				config_videoout_ClockDIVConfig(11,1); //148.5M div rst

#ifndef CONFIG_ARCH_CKMMU_TAURUS
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_resv10[0])) |= (3<<25);
#endif
#endif
				break;
#endif

#if !defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
#if (defined(ENABLE_YPBPR_HDMI_1080P_50HZ))
			case GXAV_VOUT_YPBPR_1080P_50HZ:
			case GXAV_VOUT_HDMI_1080P_50HZ:
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
#if (defined(CONFIG_ARCH_CKMMU_GEMINI) || defined(CONFIG_ARCH_CKMMU_CYGNUS))
#ifdef CONFIG_ARCH_CKMMU_GEMINI
				Gx3201_Vpu_SetBufferStateDelay(0x18);
#endif
				config_videoout_ClockSourceSel(3,0); //set vpu clock source -> xtal
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 7;//ratio;
				config_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockSourceSel(3,1); //set vpu clock source -> xtal
				config_videoout_ClockDIVConfig3(3,0);
				config_videoout_ClockDIVConfig3(4,0);
				config_videoout_ClockDIVConfig3(5,0);
				config_videoout_ClockDIVConfig3(6,0);
				config_videoout_ClockDIVConfig3(6,1);
#elif defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA)
				config_videoout_ClockSourceSel(9, 0);    // set vpu clock source to xtal
				config_videoout_ClockDIVConfig2(6, 0);  // vpu rst_en  复位有效
				config_videoout_ClockDIVConfig2(7, 0);  // vpu load_en 更新无效
				config_videoout_ClockDIVConfig2(6, 1);  // vpu rst_en  复位无效
				//配置分频系数
#if defined (CONFIG_ARCH_ARM_VEGA)
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock5)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock5)) |= 7;
#else
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock2)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock2)) |= 7;
#endif /*CONFIG_ARCH_ARM_VEGA*/
				config_videoout_ClockDIVConfig2(7, 1);  // load_en 更新有效
				config_videoout_ClockDIVConfig2(7, 0);  // load_en 更新无效
				config_videoout_ClockSourceSel(9, 1);   // set vpu clock source to pll

				config_videoout_ClockDIVConfig2(10, 0);  // VPU pixel 二分频
				config_videoout_ClockDIVConfig2(9, 0);   // VPU DCS 二分频
				config_videoout_ClockDIVConfig2(8, 0);   // HDMI pixel 二分频
				// VPU pixel / DCS HDMI pixel 二分频复位
				config_videoout_ClockDIVConfig2(11, 0);
				config_videoout_ClockDIVConfig2(11, 1);
#else
				Gx3201_Vpu_SetBufferStateDelay(0x18);
				config_videoout_ClockSourceSel(5,0); //set vpu clock source -> xtal
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 7;//ratio;
				config_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockSourceSel(5,1); //sel vpu clock source -> pll
				config_videoout_ClockDIVConfig(8,0);  //div hdmi
				config_videoout_ClockDIVConfig(9,0);  //div dcs
				config_videoout_ClockDIVConfig(10,0); //div pixel
				config_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				config_videoout_ClockDIVConfig(11,1); //148.5M div rst
#ifndef CONFIG_ARCH_CKMMU_TAURUS
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_resv10[0])) |= (3<<25);
#endif
#endif
				break;
#endif

#if (defined(ENABLE_YPBPR_HDMI_1080P_60HZ))
#if (defined(CONFIG_ARCH_CKMMU_GEMINI) || defined(CONFIG_ARCH_CKMMU_CYGNUS))
#ifdef CONFIG_ARCH_CKMMU_GEMINI
				Gx3201_Vpu_SetBufferStateDelay(0x18);
#endif
				config_videoout_ClockSourceSel(3,0); //set vpu clock source -> xtal
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 7;//ratio;
				config_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockSourceSel(3,1); //set vpu clock source -> xtal
				config_videoout_ClockDIVConfig3(3,0);
				config_videoout_ClockDIVConfig3(4,0);
				config_videoout_ClockDIVConfig3(5,0);
				config_videoout_ClockDIVConfig3(6,0);
				config_videoout_ClockDIVConfig3(6,1);
#else
				Gx3201_Vpu_SetBufferStateDelay(0x18);
				config_videoout_ClockSourceSel(5,0); //set vpu clock source -> xtal
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 7;//ratio;
				config_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				config_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				config_videoout_ClockSourceSel(5,1); //sel vpu clock source -> pll
				config_videoout_ClockDIVConfig(8,0);  //div hdmi
				config_videoout_ClockDIVConfig(9,0);  //div dcs
				config_videoout_ClockDIVConfig(10,0); //div pixel
				config_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				config_videoout_ClockDIVConfig(11,1); //148.5M div rst
#ifndef CONFIG_ARCH_CKMMU_TAURUS
				*(volatile unsigned int *)(&(gx3201_config_reg->cfg_resv10[0])) |= (3<<25);
#endif
#endif
				break;
#endif
			default:
				printf("un-supported.\n");
				break;
		}
	}
	else {
#if defined(CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA)
		config_videoout_ClockSourceSel(10, 0);    // set svpu clock source to xtal
		config_videoout_ClockDIVConfig2(19, 0);   // rst_en  复位有效
		config_videoout_ClockDIVConfig2(18, 0);   // load_en 更新无效
		config_videoout_ClockDIVConfig2(19, 1);   // rst_en  复位无效
		//配置分频系数
#if defined (CONFIG_ARCH_ARM_VEGA)
		*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock5)) &= ~(0x3f000);
		*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock5)) |= (43 << 12);
#else
		*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock2)) &= ~(0x3f000);
		*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clock2)) |= (43 << 12);
#endif /*CONFIG_ARCH_ARM_VEGA*/
		config_videoout_ClockDIVConfig2(18, 1);    // load_en 更新有效
		config_videoout_ClockDIVConfig2(18, 0);    // load_en 更新无效

		config_videoout_ClockDIVConfig2(21, 1);    // svpu 分频系数
		config_videoout_ClockDIVConfig2(23, 0);    // rst_en gate复位有效
		config_videoout_ClockDIVConfig2(23, 1);    // rst_en gate复位无效
#elif (defined(CONFIG_ARCH_CKMMU_GEMINI) || defined(CONFIG_ARCH_CKMMU_CYGNUS))
		config_videoout_ClockSourceSel(4,0);   //1: pll 0: xtal
#else
		config_videoout_ClockSourceSel(6,0);   //1: pll 0: xtal
#endif
		config_videoout_ClockDIVConfig(20,0);  //rst -> 0
		config_videoout_ClockDIVConfig(19,0);  //load_en -> 0
		config_videoout_ClockDIVConfig(20,1);  //rst -> 1
		config_videoout_ClockDIVConfig(18,0);  //bypass -> 0
		*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) &= ~(0x3f000);
		*(volatile unsigned int *)(&(gx3201_config_reg->cfg_clk)) |= 43<<12;
		config_videoout_ClockDIVConfig(19,1);  //load_en -> 1
		config_videoout_ClockDIVConfig(19,0);  //load_en -> 0
#if (defined(CONFIG_ARCH_CKMMU_GEMINI) || defined(CONFIG_ARCH_CKMMU_CYGNUS))
		config_videoout_ClockSourceSel(4,1);   //1: pll 0: xtal
#else
		config_videoout_ClockSourceSel(6,1);   //1: pll 0: xtal
#endif
	}

#if (defined(CONFIG_ARCH_CKMMU_GX3211) || defined(CONFIG_ARCH_ARM_SIRIUS) || defined(CONFIG_ARCH_CKMMU_GEMINI))
	config_videoout_ClockSourceSel(6,0);
	config_videoout_ClockSourceSel(7,0);
	config_videoout_ClockSourceSel(8,0);
#endif
	_vpu_reset();
#endif
}

static void gx3201_hdmi_cold_reset_set(void)
{
	gx3201_config_reg->cfg_mpeg_cold_reset |= (1<<7);
}

static void gx3201_hdmi_cold_reset_clr(void)
{
	gx3201_config_reg->cfg_mpeg_cold_reset &= ~(1<<7);
}

static int _hdmi_get_code(GxVideoOutProperty_Mode vout_mode, u8 *code)
{
	int i;
	for(i = 0; i < (sizeof(gn_dtd_map) / sizeof(gn_dtd_map[0])); i++){
		if(gn_dtd_map[i].mode == vout_mode){
			*code = gn_dtd_map[i].code;
			return 0;
		}
	}
	return -1;
}

static void gx3211_hdmi_open(void)
{
	gx3201_hdmi_cold_reset_set();
	mdelay(2);
	gx3201_hdmi_cold_reset_clr();
	mdelay(2);
	//标示不完整的hdmi初始化
#if defined (CONFIG_ARCH_CKMMU_CYGNUS) || defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA) || defined(CONFIG_ARCH_CKMMU_SCORPIO)
	*(volatile unsigned int *)(VPU_VOUT_BASE_ADDR + 0x0004) = 0x5a;
#elif defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	vout_set_init_val(0, 0x5a);
#else
	*(volatile unsigned int *)(VPU_BASE_ADDR + 0x4004) = 0x5a;
#endif /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO*/
}

#define HDMI_GET_AUDIO(pdev) ((audioParams_t *)(&pdev->audio))
#define HDMI_GET_VIDEO(pdev) ((videoParams_t *)(&pdev->video))
static void gx3211_hdmi_videoout_set(GxVideoOutProperty_Mode vout_mode)
{
	u8 code;
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	if(_hdmi_get_code(vout_mode, &code))
		return;
	pdev->ceacode = code;
	dw_hdmi_configure(HDMI_GET_VIDEO(pdev),HDMI_GET_AUDIO(pdev),pdev->ceacode);
}

int gx3211_svpu_do_run(void)
{
	SVPU_CAP_ENABLE(gx3211svpu_reg->rCAP_CTRL);
	SVPU_DISP_ENABLE(gx3211svpu_reg->rDISP_CTRL);
	SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
	return 0;
}

static int gx3211_svpu_stop(void)
{
	SVPU_CAP_DISABLE(gx3211svpu_reg->rCAP_CTRL);
	SVPU_DISP_DISABLE(gx3211svpu_reg->rDISP_CTRL);
	SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
	return 0;
}

#define IS_PAL_MODE(mode)\
	((mode)==GXAV_VOUT_PAL || (mode)==GXAV_VOUT_PAL_N || (mode)==GXAV_VOUT_PAL_NC)
static int gx3211_svpu_get_capmod(GxVideoOutProperty_Mode mode_vout0, GxVideoOutProperty_Mode mode_vout1)
{
	int capmode;

	switch(mode_vout0)
	{
	case GXAV_VOUT_PAL_M:
	case GXAV_VOUT_NTSC_M:
	case GXAV_VOUT_NTSC_443:
#if !defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	case GXAV_VOUT_HDMI_480I:
	case GXAV_VOUT_YCBCR_480I:
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
		capmode = IS_PAL_MODE(mode_vout1) ? -1 : 9;
		break;
	case GXAV_VOUT_PAL:
	case GXAV_VOUT_PAL_N:
	case GXAV_VOUT_PAL_NC:
#if !defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	case GXAV_VOUT_HDMI_576I:
	case GXAV_VOUT_YCBCR_576I:
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
		capmode = IS_PAL_MODE(mode_vout1) ? 8 : -1;
		break;
#if !defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	case GXAV_VOUT_VGA_480P:
	case GXAV_VOUT_HDMI_480P:
	case GXAV_VOUT_YPBPR_480P:
		capmode = IS_PAL_MODE(mode_vout1) ? -1 : 7;
		break;
	case GXAV_VOUT_VGA_576P:
	case GXAV_VOUT_HDMI_576P:
	case GXAV_VOUT_YPBPR_576P:
		capmode = IS_PAL_MODE(mode_vout1) ? 6 : -1;
		break;
	case GXAV_VOUT_HDMI_720P_50HZ:
	case GXAV_VOUT_YPBPR_720P_50HZ:
	case GXAV_VOUT_HDMI_720P_60HZ:
	case GXAV_VOUT_YPBPR_720P_60HZ:
		capmode = IS_PAL_MODE(mode_vout1) ? 5 : 4;
		break;
	case GXAV_VOUT_HDMI_1080I_50HZ:
	case GXAV_VOUT_YPBPR_1080I_50HZ:
	case GXAV_VOUT_HDMI_1080I_60HZ:
	case GXAV_VOUT_YPBPR_1080I_60HZ:
		capmode = IS_PAL_MODE(mode_vout1) ? 3 : 2;
		break;
	case GXAV_VOUT_HDMI_1080P_50HZ:
	case GXAV_VOUT_YPBPR_1080P_50HZ:
	case GXAV_VOUT_HDMI_1080P_60HZ:
	case GXAV_VOUT_YPBPR_1080P_60HZ:
		capmode = IS_PAL_MODE(mode_vout1) ? 1 : 0;
		break;
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
	default:
		capmode = -1;
		break;
	}

	return capmode;
}

static int gx3211_svpu_get_capindex(GxVideoOutProperty_Mode mode_vout0, GxVideoOutProperty_Mode mode_vout1)
{
	int capindex;

	switch(mode_vout0)
	{
	case GXAV_VOUT_PAL_M:
	case GXAV_VOUT_NTSC_M:
	case GXAV_VOUT_NTSC_443:
#if !defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	case GXAV_VOUT_HDMI_480I:
	case GXAV_VOUT_YCBCR_480I:
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
		capindex = IS_PAL_MODE(mode_vout1) ? 3 : -1;
		break;
	case GXAV_VOUT_PAL:
	case GXAV_VOUT_PAL_N:
	case GXAV_VOUT_PAL_NC:
#if !defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	case GXAV_VOUT_HDMI_576I:
	case GXAV_VOUT_YCBCR_576I:
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
		capindex = IS_PAL_MODE(mode_vout1) ? -1 : 2;
		break;
#if !defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	case GXAV_VOUT_VGA_480P:
	case GXAV_VOUT_HDMI_480P:
	case GXAV_VOUT_YPBPR_480P:
		capindex = IS_PAL_MODE(mode_vout1) ? 1 : -1;
		break;
	case GXAV_VOUT_VGA_576P:
	case GXAV_VOUT_HDMI_576P:
	case GXAV_VOUT_YPBPR_576P:
		capindex = IS_PAL_MODE(mode_vout1) ? -1 : 0;
		break;
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
	default:
		capindex = -1;
		break;
	}

	return capindex;
}

static unsigned int gx_virt_to_phys(unsigned int addr)
{
#if defined (CONFIG_ARCH_ARM_SIRIUS) || defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA)
	return addr;
#else
	return ((unsigned int)addr - 0x80000000);
#endif
}

static unsigned int gx_phys_to_virt(unsigned int addr)
{
#ifdef CONFIG_ARCH_ARM_SIRIUS
	return addr;
#else
	return ((unsigned int)addr + 0x80000000);
#endif
}

static int gx3211_svpu_config_buf(SvpuSurfaceInfo *buf_info)
{
	int i;
	unsigned buf_block_size = SVPU_SURFACE_WIDTH*SVPU_SURFACE_HEIGHT;

	if(gx3211svpu_reg && buf_info) {
#ifdef CONFIG_ENABLE_FIREWALL
		firewall_config_filter(gx_virt_to_phys(buf_info->buf_addrs[0]), buf_info->buf_size, MASTER_VPU, MASTER_VPU, 0);
#endif
		for(i = 0; i < SVPU_SURFACE_NUM; i ++) {
			unsigned buf_base_addr = gx_virt_to_phys(buf_info->buf_addrs[i]);
			REG_SET_VAL(&gx3211svpu_reg->rBUF[i].y_topfield_addr,    buf_base_addr+buf_block_size*0);
			REG_SET_VAL(&gx3211svpu_reg->rBUF[i].y_bottomfield_addr, buf_base_addr+buf_block_size*0+SVPU_SURFACE_WIDTH);
			REG_SET_VAL(&gx3211svpu_reg->rBUF[i].u_topfield_addr,    buf_base_addr+buf_block_size*1);
			REG_SET_VAL(&gx3211svpu_reg->rBUF[i].u_bottomfield_addr, buf_base_addr+buf_block_size*1+SVPU_SURFACE_WIDTH);
#ifdef CONFIG_ARCH_CKMMU_GX6605S
			// 4:2:2 模式 v_addr 不用配置
#else
			REG_SET_VAL(&gx3211svpu_reg->rBUF[i].v_topfield_addr,    buf_base_addr+buf_block_size*2);
			REG_SET_VAL(&gx3211svpu_reg->rBUF[i].v_bottomfield_addr, buf_base_addr+buf_block_size*2+SVPU_SURFACE_WIDTH);
#endif
		}
	}

	return 0;
}

static int gx3211_svpu_get_reqblock(int width, int bpp)
{
	int ret = 32;
	int min = 32, max = 896;
	int step = 8, align = 128, times = 4;
	int line_byte = ((width*bpp)>>3);
	int request   = (line_byte/times)/align*align;

	if(request < min)
		ret = min;
	else {
		while(request<max && (line_byte%request && line_byte%request<min)) {
			request += step;
		}
		if(request>=max || (line_byte%request && line_byte%request<min)) {
			ret = min;
			printf("\n%s:%d, get svpu reqblock failed!\n", __func__, __LINE__);
		}
		else
			ret = request;
	}

	return ret;
}

static unsigned int svpuPhase[4][15] = {
  { 0, 0, 719, 0, 575, 9830, 4096, 720, 480, 0,   2867, 0, 719, 0, 477 },
  { 0, 0, 719, 0, 479, 6836, 4096, 720, 576, 0,   1365, 0, 719, 0, 573 },
  { 0, 0, 719, 0, 575, 4915, 4096, 720, 480, 0,   409,  0, 719, 0, 477 },
  { 0, 0, 719, 0, 479, 3413, 4096, 720, 576, 341, 0,    0, 719, 0, 573 }
};

static int gx3211_svpu_config(GxVideoOutProperty_Mode mode_vout0, GxVideoOutProperty_Mode mode_vout1, SvpuSurfaceInfo *buf_info)
{
	int capmode, reqblock;

	if(!gx3211svpu_reg)
		return -1;

	//set buffers
	gx3211_svpu_config_buf(buf_info);
	//set capmode
	capmode = gx3211_svpu_get_capmod(mode_vout0, mode_vout1);
	if(capmode < 0) {
		int capindex = gx3211_svpu_get_capindex(mode_vout0, mode_vout1);
		if (capindex < 0) {
			printf("\n%s:%d, config svpu error!", __func__, __LINE__);
			return -1;
		}
		SVPU_SET_CAPMODE_DISABLE(gx3211svpu_reg->rCAP_CTRL);
		if (svpuPhase[capindex][0] != 0) {
			SVPU_VDOWNSCALE_ENABLE(gx3211svpu_reg->rCAP_CTRL);
		}
		else {
			SVPU_VDOWNSCALE_DISABLE(gx3211svpu_reg->rCAP_CTRL);
		}
		SVPU_SET_CAP_LEFT(gx3211svpu_reg->rCAP_H, svpuPhase[capindex][1]);
		SVPU_SET_CAP_RIGHT(gx3211svpu_reg->rCAP_H, svpuPhase[capindex][2]);
		SVPU_SET_CAP_TOP(gx3211svpu_reg->rCAP_V, svpuPhase[capindex][3]);
		SVPU_SET_CAP_BOTTOM(gx3211svpu_reg->rCAP_V, svpuPhase[capindex][4]);
		SVPU_SET_SCE_VZOOM(gx3211svpu_reg->rSCE_ZOOM, svpuPhase[capindex][5]);
		SVPU_SET_SCE_HZOOM(gx3211svpu_reg->rSCE_ZOOM, svpuPhase[capindex][6]);
		SVPU_SET_SCE_ZOOM_LENGTH(gx3211svpu_reg->rSCE_ZOOM1, svpuPhase[capindex][7]);
		SVPU_SET_SCE_ZOOM_HEIGHT(gx3211svpu_reg->rSCE_ZOOM1, svpuPhase[capindex][8]);
		SVPU_SET_SCE_VT_PHASE(gx3211svpu_reg->rZOOM_PHASE, svpuPhase[capindex][9]);
		SVPU_SET_SCE_VB_PHASE(gx3211svpu_reg->rZOOM_PHASE, svpuPhase[capindex][10]);
		SVPU_SET_SPP_LEFT(gx3211svpu_reg->rVIEW_H, svpuPhase[capindex][11]);

		SVPU_SET_SPP_RIGHT(gx3211svpu_reg->rVIEW_H, svpuPhase[capindex][12]);
		SVPU_SET_SPP_TOP(gx3211svpu_reg->rVIEW_V, svpuPhase[capindex][13]);
		SVPU_SET_SPP_BOTTOM(gx3211svpu_reg->rVIEW_V, svpuPhase[capindex][14]);
	}
	else {
		SVPU_SET_CAPMODE(gx3211svpu_reg->rCAP_CTRL, capmode);
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
		SVPU_SET_CAPMODE_ENABLE(gx3211svpu_reg->rCAP_CTRL);
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
	}
	//set caplevel
	SVPU_SET_CAPLEVEL(gx3211svpu_reg->rCAP_CTRL, SVPU_LAYER_MIX);
	//set buf R/W reqblock
	reqblock = gx3211_svpu_get_reqblock(SVPU_SURFACE_WIDTH, SVPU_SURFACE_BPP);
	SVPU_SET_BUFS_READ_REQBLOCK (gx3211svpu_reg->rDISP_REQ_BLOCK, reqblock);
	SVPU_SET_BUFS_WRITE_REQBLOCK(gx3211svpu_reg->rREQ_LENGTH,     reqblock);
	SVPU_SET_FRAME_MODE(gx3211svpu_reg->rVPU_FRAME_MODE, FRAME_MODE_3);
	//para update
	SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
	return 0;
}

static void vout_macrovision_disable(void)
{
	REG_CLR_BIT(&(gx3211_vout1_reg->aps_reg4), 12);
	REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg0), 0xff, 0, 0);
	REG_SET_FIELD(&(gx3211_vout1_reg->aps_ctrl2), (0xff<< 0),0, 0);
}

#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
static void _vout_init_size(GxVideoOutProperty_Mode mode)
{
	switch(mode) {
		case GXAV_VOUT_480I:
		case GXAV_VOUT_480P:
			g_hdmi_output_w = 720;
			g_hdmi_output_h = 480;
			break;
		case GXAV_VOUT_576I:
		case GXAV_VOUT_576P:
			g_hdmi_output_w = 720;
			g_hdmi_output_h = 576;
			break;
		case GXAV_VOUT_720P_50HZ:
		case GXAV_VOUT_720P_60HZ:
			g_hdmi_output_w = 1280;
			g_hdmi_output_h = 720;
			break;
		case GXAV_VOUT_1080I_50HZ:
		case GXAV_VOUT_1080I_60HZ:
		case GXAV_VOUT_1080P_50HZ:
		case GXAV_VOUT_1080P_60HZ:
			g_hdmi_output_w = 1920;
			g_hdmi_output_h = 1080;
			break;
		default:
			break;
	}
}
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/

void vpu_svpu_hdmi_init(void)
{
	static int init_flag = 0;
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	int ret = 0;
	GxVpuMixerLayers mixer = {0};
	SvpuSurfaceInfo svpu_fb = {0};
	GxAvRect rect = {0, 0, 720, 576};
	struct gx_mem_info info = {0};
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
	enum cvbs_mode_enum cvbs_mode = g_cvbs_mode;
	enum ypbpr_hdmi_mode_enum ypbpr_hdmi_mode = g_ypbpr_hdmi_mode;
	GxVideoOutProperty_Mode cvbs;

	if(init_flag){
		return;
	}else{
		init_flag = 1;
	}
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)

	// install
	vout_secure_init(VPU_BASE_ADDR + 0x200);

	// clock
	switch(ypbpr_hdmi_mode) {
		case GXAV_VOUT_480I:
		case GXAV_VOUT_576I:
			vout_set_main_clock_freq(ypbpr_hdmi_mode, 27000000, 13500000, 27000000, 27000000);
			break;
		case GXAV_VOUT_480P:
		case GXAV_VOUT_576P:
			vout_set_main_clock_freq(ypbpr_hdmi_mode, 54000000, 27000000, 27000000, 27000000);
			break;
		case GXAV_VOUT_720P_50HZ:
		case GXAV_VOUT_720P_60HZ:
		case GXAV_VOUT_1080I_50HZ:
		case GXAV_VOUT_1080I_60HZ:
			vout_set_main_clock_freq(ypbpr_hdmi_mode, 148500000, 74250000, 74250000, 74250000);
			break;
		case GXAV_VOUT_1080P_50HZ:
		case GXAV_VOUT_1080P_60HZ:
			vout_set_main_clock_freq(ypbpr_hdmi_mode, 148500000, 148500000, 148500000, 148500000);
			break;
		default:
			break;
	}
	vout_set_second_clock_freq(cvbs_mode, 27000000, 13500000);
	
	// set mode
	ret |= vout_hd_set_mode(0, ypbpr_hdmi_mode);
	ret |= vout_rca_set_mode(1, cvbs_mode);

	// init size
	_vout_init_size(ypbpr_hdmi_mode);

	// set dac mode
	ret |= vout_set_dac_mode(0, 3);   // G/R/X/B
	ret |= vout_set_dac_mode(1, 0);   // G/B/R/X

	// set ext coef
	ret |= vout_set_ext_coef_en(0, 0);
	ret |= vout_set_ext_coef_en(1, 0);

	// hot reset vpu
	ret |= vout_reset_clock();
	clock_set_hdmi_enable(1);
	mdelay(30);

	// hvsync
	vout_set_bt656_hpos(0, 0x48);

	// patch for tv_active
	if((GXAV_VOUT_720P_50HZ == ypbpr_hdmi_mode) ||
	   (GXAV_VOUT_720P_60HZ == ypbpr_hdmi_mode)) {
		unsigned int active_on = 0, active_off = 0;
		ret |= vout_get_tv_active(0, &active_on, &active_off);
		active_off += 6;
		ret |= vout_set_tv_active(0, active_on, active_off);
	}

	// TODO: HDMI
	gx3211_hdmi_open();
	gx3211_hdmi_videoout_set(ypbpr_hdmi_mode);
	// cvbs
	vpu_sys_all_layer_en(1);
	clock_set_dac_src(0, VIDEO_DAC_SRC_SVPU);
	mixer.id = HD_MIXER;
	mixer.layers[0] = GX_LAYER_VPP;
	mixer.layers[1] = GX_LAYER_SPP;
	mixer.layers[2] = GX_LAYER_OSD;
	mixer.layers[3] = GX_LAYER_NONE;
	mixer.layers[4] = GX_LAYER_NONE;
	vpu_sys_set_mixer(&mixer);

	vpu_ce_init();
	vpu_vpp2_enable(0);
	vpu_ce_enable(0);
	vpu_ce_set_format(GX_COLOR_FMT_YCBCR422);
	vpu_ce_set_mode(0);
	vpu_ce_set_layer((1 << GX_LAYER_OSD) | (1 << GX_LAYER_SPP));
#if defined(CMDLINE_SVPUMEM_START) && defined(CMDLINE_SVPUMEM_SIZE)
	gx_mem_info_get("svpumem", &info);
	svpu_fb.buf_addrs[0] = info.start;
	svpu_fb.buf_size = info.size;
#else /*defined(CMDLINE_SVPUMEM_START) && defined(CMDLINE_SVPUMEM_SIZE)*/
#ifdef CONFIG_ENABLE_FIREWALL
	printf("Check config svpumem and size in cmdline.\n");
#endif /*CONFIG_ENABLE_FIREWALL*/
	svpu_fb.buf_addrs[0] = gx_virt_to_phys(SVPU_BUF_START_ADDR); //vout buf
	svpu_fb.buf_size = SVPU_BUF_SIZE;
#endif  /* defined(CMDLINE_SVPUMEM_START) && defined(CMDLINE_SVPUMEM_SIZE) */
	if((0 == svpu_fb.buf_addrs[0]) || (0 == svpu_fb.buf_size)) {
		printf("No SVPUMEM and size configured.\n");
		return;
	}
#ifdef CONFIG_ENABLE_FIREWALL
		firewall_config_filter(gx_virt_to_phys(buf_info->buf_addrs[0]), buf_info->buf_size, MASTER_VPU, MASTER_VPU, 0);
#endif
	vpu_ce_set_buf_addr(gx_phys_to_virt(svpu_fb.buf_addrs[0]), 720, 576, GX_COLOR_FMT_YCBCR422);
	vout_secure_ce_permission_buffer(svpu_fb.buf_addrs[0], svpu_fb.buf_size);
	vpu_ce_set_bind_to_vpp2(1);
	vpu_vpp2_zoom(&rect, &rect);
	vpu_ce_enable(1);
	vpu_vpp2_enable(1);
#else /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/

#if defined(CONFIG_ARCH_CKMMU_CYGNUS) || defined(CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA) || defined(CONFIG_ARCH_CKMMU_SCORPIO)
	*(volatile unsigned int *)VOUT_HVSYNC_REGS &= ~(0xff << 16);
	*(volatile unsigned int *)VOUT_HVSYNC_REGS |=  (0x48 << 16);
#endif

	switch(ypbpr_hdmi_mode){

#ifdef ENABLE_YPBPR_HDMI_480I
		case G_YPBPR_HDMI_480I:
			vpu_hdmi_vout_mode = GXAV_VOUT_HDMI_480I;
			gx3201_videoout_yuv_config(VOUT_0, ypbpr_hdmi_mode);
			g_hdmi_output_w = 720;
			g_hdmi_output_h = 480;
			*(volatile unsigned int *)VOUT_HVSYNC_REGS &= ~(3 << 3);
			break;
#endif

#ifdef ENABLE_YPBPR_HDMI_480P
		case G_YPBPR_HDMI_480P:
			vpu_hdmi_vout_mode = GXAV_VOUT_HDMI_480P;
			gx3201_videoout_yuv_config(VOUT_0, ypbpr_hdmi_mode);
			g_hdmi_output_w = 720;
			g_hdmi_output_h = 480;
			*(volatile unsigned int *)VOUT_HVSYNC_REGS  &= ~(3 << 3);
			break;
#endif

#ifdef ENABLE_YPBPR_HDMI_576I
		case G_YPBPR_HDMI_576I:
			vpu_hdmi_vout_mode = GXAV_VOUT_HDMI_576I;
			gx3201_videoout_yuv_config(VOUT_0, ypbpr_hdmi_mode);
			g_hdmi_output_w = 720;
			g_hdmi_output_h = 576;
			*(volatile unsigned int *)VOUT_HVSYNC_REGS &= ~(3 << 3);
			break;
#endif

#ifdef ENABLE_YPBPR_HDMI_576P
		case G_YPBPR_HDMI_576P:
			vpu_hdmi_vout_mode = GXAV_VOUT_HDMI_576P;
			gx3201_videoout_yuv_config(VOUT_0, ypbpr_hdmi_mode);
			g_hdmi_output_w = 720;
			g_hdmi_output_h = 576;
			*(volatile unsigned int *)VOUT_HVSYNC_REGS &= ~(3 << 3);
			*(volatile unsigned int *)VOUT_BOTTOM_LINE_REGS = 0x0001626b;
			break;
#endif

#ifdef ENABLE_YPBPR_HDMI_720P_50HZ
		case G_YPBPR_HDMI_720P_50HZ:
			vpu_hdmi_vout_mode = GXAV_VOUT_HDMI_720P_50HZ;
			gx3201_videoout_yuv_config(VOUT_0, ypbpr_hdmi_mode);
			g_hdmi_output_w = 1280;
			g_hdmi_output_h = 720;
			*(volatile unsigned int *)VOUT_HVSYNC_REGS |= 3 << 3;
			*(volatile unsigned int *)VOUT_BOTTOM_LINE_REGS = 0x0000cae8;
			break;
#endif

#ifdef ENABLE_YPBPR_HDMI_720P_60HZ
		case G_YPBPR_HDMI_720P_60HZ:
			vpu_hdmi_vout_mode = GXAV_VOUT_HDMI_720P_60HZ;
			gx3201_videoout_yuv_config(VOUT_0, ypbpr_hdmi_mode);
			g_hdmi_output_w = 1280;
			g_hdmi_output_h = 720;
			*(volatile unsigned int *)VOUT_HVSYNC_REGS |= 3 << 3;
			*(volatile unsigned int *)VOUT_BOTTOM_LINE_REGS = 0x0000cae8;
			break;
#endif

#ifdef ENABLE_YPBPR_HDMI_1080I_50HZ
		case G_YPBPR_HDMI_1080I_50HZ:
			vpu_hdmi_vout_mode = GXAV_VOUT_HDMI_1080I_50HZ;
			gx3201_videoout_yuv_config(VOUT_0, ypbpr_hdmi_mode);
			g_hdmi_output_w = 1920;
			g_hdmi_output_h = 1080;
			*(volatile unsigned int *)VOUT_HVSYNC_REGS |= (3 << 3);
			break;
#endif

#ifdef ENABLE_YPBPR_HDMI_1080I_60HZ
		case G_YPBPR_HDMI_1080I_60HZ:
			vpu_hdmi_vout_mode = GXAV_VOUT_HDMI_1080I_60HZ;
			gx3201_videoout_yuv_config(VOUT_0, ypbpr_hdmi_mode);
			g_hdmi_output_w = 1920;
			g_hdmi_output_h = 1080;
			*(volatile unsigned int *)VOUT_HVSYNC_REGS |= (3 << 3);
			break;
#endif

#ifdef ENABLE_YPBPR_HDMI_1080P_50HZ
		case G_YPBPR_HDMI_1080P_50HZ:
			vpu_hdmi_vout_mode = GXAV_VOUT_HDMI_1080P_50HZ;
			gx3201_videoout_yuv_config(VOUT_0, ypbpr_hdmi_mode);
			g_hdmi_output_w = 1920;
			g_hdmi_output_h = 1080;
			*(volatile unsigned int *)VOUT_HVSYNC_REGS |= (3 << 3);
			break;
#endif

#ifdef ENABLE_YPBPR_HDMI_1080P_60HZ
		case G_YPBPR_HDMI_1080P_60HZ:
			vpu_hdmi_vout_mode = GXAV_VOUT_HDMI_1080P_60HZ;
			gx3201_videoout_yuv_config(VOUT_0, ypbpr_hdmi_mode);
			g_hdmi_output_w = 1920;
			g_hdmi_output_h = 1080;
			*(volatile unsigned int *)VOUT_HVSYNC_REGS |= (3 << 3);
			break;
#endif

		default:
			printf("error: ypbyr_hdmi mode not support. please modify .config to enable it. \n");
			break;
	}


	switch(cvbs_mode){

#ifdef ENABLE_PAL
		case G_PAL:
			cvbs = GXAV_VOUT_PAL;
			gx3201_videoout_pal_config(VOUT_1, cvbs_mode);
			break;
#endif

#ifdef ENABLE_PAL_M
		case G_PAL_M:
			cvbs = GXAV_VOUT_PAL_M;
			gx3201_videoout_ntsc_config(VOUT_1, cvbs_mode);
			break;
#endif

#ifdef ENABLE_PAL_N
		case G_PAL_N:
			cvbs = GXAV_VOUT_PAL_N;
			gx3201_videoout_pal_config(VOUT_1, cvbs_mode);
			break;
#endif

#ifdef ENABLE_PAL_NC
		case G_PAL_NC:
			cvbs = GXAV_VOUT_PAL_NC;
			gx3201_videoout_pal_config(VOUT_1, cvbs_mode);
			break;
#endif

#ifdef ENABLE_NTSC_M
		case G_NTSC_M:
			cvbs = GXAV_VOUT_NTSC_M;
			gx3201_videoout_ntsc_config(VOUT_1, cvbs_mode);
			break;
#endif

#ifdef ENABLE_NTSC_443
		case G_NTSC_443:
			cvbs = GXAV_VOUT_NTSC_443;
			gx3201_videoout_ntsc_config(VOUT_1, cvbs_mode);
			break;
#endif
		default:
			printf("error: cvbs mode not support. please modify .config to enable it. \n");
			break;
	}
	REG_CLR_BIT(&gx3211_vout1_reg->tv_ctrl, 30);

	if(((ypbpr_hdmi_mode == G_YPBPR_HDMI_720P_50HZ) ||
		(ypbpr_hdmi_mode == G_YPBPR_HDMI_1080I_50HZ) ||
		(ypbpr_hdmi_mode == G_YPBPR_HDMI_1080P_50HZ)) &&
			((cvbs_mode == G_NTSC_M)  || (cvbs_mode == G_NTSC_443))){
		printf("error: can't support 50HZ with NTSC.\n");
	}
	if(((ypbpr_hdmi_mode == G_YPBPR_HDMI_720P_60HZ) ||
				(ypbpr_hdmi_mode == G_YPBPR_HDMI_1080I_60HZ) ||
				(ypbpr_hdmi_mode == G_YPBPR_HDMI_1080P_60HZ)) &&
			((cvbs_mode == G_PAL)  || (cvbs_mode == G_PAL_M) ||
			 (cvbs_mode == G_PAL_N) || (cvbs_mode == G_PAL_NC))){
		printf("error: can't support 60HZ with PAL.\n");
	}

	unsigned int val = REG_GET_VAL(&gx3211_vout1_reg->line_pixel_num);
	REG_SET_VAL(&gx3211_vout1_reg->line_pixel_num, (val|(0x7<<29)));

	//vpu&hdmi clk config
	gx3211_config_videoout_Clock(0, vpu_hdmi_vout_mode);

	gx3211_hdmi_open();
	gx3211_hdmi_videoout_set(vpu_hdmi_vout_mode);

#if defined (CONFIG_ARCH_CKMMU_CYGNUS) || defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA) || defined(CONFIG_ARCH_CKMMU_SCORPIO)
	// TODO:
	vpu_init();
	ce_init();
	vpp2_enable(0);
	vpp2_update();
	ce_enable(0);
	ce_set_format(13);
	ce_set_auto();
	ce_set_layer(0);
	ce_set_layer(3);
	ce_update();
	SvpuSurfaceInfo svpu_fb;

#if defined(CMDLINE_SVPUMEM_START) && defined(CMDLINE_SVPUMEM_SIZE)
	svpu_fb.buf_addrs[0] = CMDLINE_SVPUMEM_START;
	svpu_fb.buf_size = CMDLINE_SVPUMEM_SIZE;
#else
#ifdef CONFIG_ENABLE_FIREWALL
	printf("Check config svpumem and size in cmdline.\n");
#endif
	svpu_fb.buf_addrs[0] = gx_virt_to_phys(SVPU_BUF_START_ADDR); //vout buf
	svpu_fb.buf_size = SVPU_BUF_SIZE;
#endif
	svpu_fb.buf_addrs[1] = svpu_fb.buf_addrs[0];
	svpu_fb.buf_addrs[2] = svpu_fb.buf_addrs[0];
	ce_set_buf(svpu_fb.buf_addrs[0], 720, 576, 13);
	ce_bind_to_vpp2(1);
	vpp2_zoom(0, 0, 720, 576);
	ce_update();
	ce_enable(1);
	vpp2_enable(1);
	ce_update();
#else /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO*/
	//video dac config
	extern enum video_dac_mode dac_config_mode;
	switch (dac_config_mode) {
	case DAC_NOT_CONFIG:
		printf("warning: board-init not call function enable_dac, will open cvbs & ypbpr default.\n");
		*(volatile unsigned int *)(VPU_BASE_ADDR + 0x134) = 0x1f;
		break;
	case ENABLE_CVBS:
		*(volatile unsigned int *)(VPU_BASE_ADDR + 0x134) = 0x18;
		break;
	case ENABLE_CVBS_AND_YPBPR:
		*(volatile unsigned int *)(VPU_BASE_ADDR + 0x134) = 0x1f;
		break;
	default:
		printf("warning: video_dac_mode = %d is not valid.\n", dac_config_mode);
		break;
	}
	*(volatile unsigned int *)(SVPU_BASE_ADDR + 0x80) = 0xffff0000;

#ifdef CONFIG_ARCH_CKMMU_GX6605S
	*(volatile unsigned int *)(VPU_BASE_ADDR + 0x138) = 0x1;
	*(volatile unsigned int *)(SVPU_BASE_ADDR + 0x50) |= 3<<8 ;

	*(volatile unsigned int *)(SVPU_BASE_ADDR + 0x74) = 0x23; //set svpu capture delay for k2t error
	*(volatile unsigned int *)(SVPU_BASE_ADDR + 0x78) = 0x0;
	*(volatile unsigned int *)(SVPU_BASE_ADDR + 0x78) = 0x1;
	*(volatile unsigned int *)(SVPU_BASE_ADDR + 0x78) = 0x0;

	SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
#elif defined(CONFIG_ARCH_CKMMU_GX3211)
	*(volatile unsigned int *)(VPU_BASE_ADDR + 0x138) = 0x8;

	*(volatile unsigned int *)(SVPU_BASE_ADDR + 0x74) = 0x23; //set svpu capture delay for k2t error
	*(volatile unsigned int *)(SVPU_BASE_ADDR + 0x78) = 0x0;
	*(volatile unsigned int *)(SVPU_BASE_ADDR + 0x78) = 0x1;
	*(volatile unsigned int *)(SVPU_BASE_ADDR + 0x78) = 0x0;
#elif defined(CONFIG_ARCH_CKMMU_TAURUS)
    *(volatile unsigned int *)(VPU_BASE_ADDR + 0x138) = 0x1;
    *(volatile unsigned int *)(SVPU_VOUT_BASE_ADDR + 0x00C) = 0xa0320;
	*(volatile unsigned int *)(SVPU_VOUT_BASE_ADDR + 0x040) = 0x200e;
#else
	*(volatile unsigned int *)(VPU_BASE_ADDR + 0x138) = 0x8;
#ifdef CONFIG_ARCH_ARM_SIRIUS
	*(volatile unsigned int *)(SVPU_VOUT_BASE_ADDR + 0x00C) = 0xd0338;
#endif
#endif
	SvpuSurfaceInfo svpu_fb;

#if defined(CMDLINE_SVPUMEM_START) && defined(CMDLINE_SVPUMEM_SIZE)
	svpu_fb.buf_addrs[0] = gx_phys_to_virt(CMDLINE_SVPUMEM_START);
	svpu_fb.buf_size = CMDLINE_SVPUMEM_SIZE;
#else
	svpu_fb.buf_addrs[0] = SVPU_BUF_START_ADDR; //vout buf
	svpu_fb.buf_size = SVPU_BUF_SIZE;
#endif
	svpu_fb.buf_addrs[1] = svpu_fb.buf_addrs[0];
	svpu_fb.buf_addrs[2] = svpu_fb.buf_addrs[0];
	gx3211_svpu_stop();
	gx3211_svpu_config(vpu_hdmi_vout_mode, cvbs, &svpu_fb);
	vout_macrovision_disable();
#endif /*CONFIG_ARCH_CKMMU_CYGNUS*/
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
}


int gx_layer_zoom(int layer, int source_width, int source_height, int dest_width, int dest_height, int start_x, int start_y)
{

	int hzoom = 4096, vzoom = 4096;
	int hdownscale = 0, vdownscale = 0;
	int start_x_here = 0, start_y_here = 0;
	int vtbais = 0, vbbais = 0;
	int i = 0;
	int vpu_mode = 1;
	enum ypbpr_hdmi_mode_enum ypbpr_hdmi_mode = g_ypbpr_hdmi_mode;

	source_height = (source_height/2) * 2;
	source_width = (source_width/8) * 8;
	dest_height = (dest_height/2) * 2;
	dest_width = (dest_width/8) * 8;

	switch(ypbpr_hdmi_mode)
	{
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	case    GXAV_VOUT_480I:
	case    GXAV_VOUT_480P:
#else /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
		case	G_YPBPR_HDMI_480I:
		case	G_YPBPR_HDMI_480P:
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
		{
			if((dest_height + start_y) > 480)
				return 1;
			if((dest_width + start_x)>720)
				return 1;
			break;
		}
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
		case    GXAV_VOUT_576I:
		case    GXAV_VOUT_576P:
#else /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
		case	G_YPBPR_HDMI_576I:
		case	G_YPBPR_HDMI_576P:
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
		{
			if((dest_height + start_y) > 576)
				return 1;
			if((dest_width + start_x)>720)
				return 1;
			break;
		}
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
		case    GXAV_VOUT_720P_50HZ:
		case    GXAV_VOUT_720P_60HZ:
#else /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
		case	G_YPBPR_HDMI_720P_50HZ:
		case	G_YPBPR_HDMI_720P_60HZ:
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
		{
			if((dest_height + start_y) > 720)
				return 1;
			if((dest_width + start_x)>1280)
				return 1;
			break;
		}
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
		case    GXAV_VOUT_1080I_50HZ:
		case    GXAV_VOUT_1080I_60HZ:
		case    GXAV_VOUT_1080P_50HZ:
		case    GXAV_VOUT_1080P_60HZ:
#else /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
		case	G_YPBPR_HDMI_1080I_50HZ:
		case	G_YPBPR_HDMI_1080I_60HZ:
		case	G_YPBPR_HDMI_1080P_50HZ:
		case	G_YPBPR_HDMI_1080P_60HZ:
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
		{
			if((dest_height + start_y) > 1080)
				return 1;
			if((dest_width + start_x)>1920)
				return 1;
			break;
		}
		default:
			break;
	}
	if(source_width	!=	dest_width)
		hzoom = (source_width - 1)*4096/( dest_width - 1 ) + 1;
	if(hzoom > 4096*4)
	{
		hdownscale = 1;
		hzoom = ((source_width/2) - 1)*4096/ (dest_width - 1 ) + 1;
		if(hzoom > 4096*4)
			return 1;
	}

	if((dest_width-1) > ((source_width/(1+ hdownscale)-1)*4096/hzoom))
		start_x_here = ((dest_width-1) - ((source_width/(1+ hdownscale)-1)*4096/hzoom)) /2;

	start_y_here = 0;

	switch(ypbpr_hdmi_mode) {
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
		case    GXAV_VOUT_480I:
		case    GXAV_VOUT_576I:
		case    GXAV_VOUT_1080I_50HZ:
		case    GXAV_VOUT_1080I_60HZ:
#else /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
		case G_YPBPR_HDMI_480I:
		case G_YPBPR_HDMI_576I:
		case G_YPBPR_HDMI_1080I_50HZ:
		case G_YPBPR_HDMI_1080I_60HZ:
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
			if (source_height != dest_height)
				vzoom = (source_height - 1) * 4096 / (dest_height - 1 ) + 1;

			if(vzoom >= 4096) {
				vpu_mode = 0;
				if(vzoom > 4096 * 4) {
					vdownscale = 1;
					vzoom = ((source_height / 2) - 1) * 4096 / (dest_height - 1 ) + 1;
					if(vzoom > 4096 * 4)
						return 1;
				}
			}
			else {
				vtbais = 0;
				vbbais = vzoom/2;
			}

			if((dest_height-1) > (source_height*(1+vpu_mode)/(1+ vdownscale)-1)*4096/(vzoom*(1+vpu_mode)) )
				start_y_here = ((dest_height-1) - (source_height*(1+vpu_mode)/(1+ vdownscale)-1)*4096/(vzoom*(1+vpu_mode))) /2;

			break;
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
		case    GXAV_VOUT_480P:
		case    GXAV_VOUT_576P:
		case    GXAV_VOUT_720P_50HZ:
		case    GXAV_VOUT_720P_60HZ:
		case    GXAV_VOUT_1080P_50HZ:
		case    GXAV_VOUT_1080P_60HZ:
#else /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
		case G_YPBPR_HDMI_480P:
		case G_YPBPR_HDMI_576P:
		case G_YPBPR_HDMI_720P_50HZ:
		case G_YPBPR_HDMI_720P_60HZ:
		case G_YPBPR_HDMI_1080P_50HZ:
		case G_YPBPR_HDMI_1080P_60HZ:
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
			vzoom = (source_height - 1) * 4096/ (dest_height - 1 ) + 1;
			if(vzoom >= 4096) {
				if(vzoom > 4096 * 4) {
					vdownscale = 1;
					vzoom = ((source_height/2) - 1)*4096/ (dest_height - 1 ) + 1;
					if(vzoom > 4096*4)
						return 1;
				}
			}
			vtbais = 0;
			vbbais = 0;

			if((dest_height-1) > (source_height*(1+vpu_mode)/(1+ vdownscale)-1)*4096/(vzoom*(1+vpu_mode)/2))
				start_y_here = 0;
			break;
		default:
			break;
	}

	if(layer == 0){	//spp
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
		vpu_pic_set_down_scale(vdownscale, hdownscale);
		vpu_pic_set_v_phase(vbbais, vtbais);
		vpu_pic_set_pos(start_x + start_x_here, start_y + start_y_here);
		vpu_pic_set_clip_size(source_width, source_height);
		vpu_pic_set_view_size(dest_width, dest_height);
		vpu_pic_set_zoom(vzoom, hzoom);
		vpu_pic_set_filter_sign(0x00998080);
		vpu_pic_set_zoom_para(sPhaseNomal, 64);
#elif defined (CONFIG_ARCH_CKMMU_CYGNUS) || defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA) || defined (CONFIG_ARCH_CKMMU_SCORPIO)
		if(vdownscale)
			*(volatile u32*)(PIC_BASE_ADDR + 0x3c) |= (1<<2);	//JPG_CTRL
		else
			*(volatile u32*)(PIC_BASE_ADDR + 0x3c) &= ~(1<<2);

		if(hdownscale)
			*(volatile u32*)(PIC_BASE_ADDR + 0x3c) |= (1<<3);
		else
			*(volatile u32*)(PIC_BASE_ADDR + 0x3c) &= ~(1<<3);

		*(volatile u32*)(PIC_BASE_ADDR + 0x04)  = vtbais | vbbais << 16;	//JPG_V_PHASE
		*(volatile u32*)(PIC_BASE_ADDR + 0x08)  = (start_x + start_x_here) | (start_y + start_y_here)<<16;	//JPG_POSTITION
		*(volatile u32*)(PIC_BASE_ADDR + 0x0c)  = source_width | source_height<<16;	//JPG_SOURCE_SIZE
		*(volatile u32*)(PIC_BASE_ADDR + 0x10)  = dest_width | dest_height<<16;	//JPG_VIEW_SIZE
		*(volatile u32*)(PIC_BASE_ADDR + 0x14)  = hzoom | vzoom<<16;	//JPG_ZOOM
		*(volatile u32*)(PIC_BASE_ADDR + 0x18) &= ~(0x7ff<<16);	//JPG_PARA
		*(volatile u32*)(PIC_BASE_ADDR + 0x18) |= (((source_width>>2)>>3)<<3)<<16 ;
		*(volatile u32*)(PIC_BASE_ADDR + 0x18) &= ~(0x1<<29);
		*(volatile u32*)(PIC_BASE_ADDR + 0x18) |= vpu_mode<<29;
		if (hzoom == 4096)
			*(volatile u32*)(PIC_BASE_ADDR + 0x20) = 0x00ff0000;
		else
			*(volatile u32*)(PIC_BASE_ADDR + 0x20) = 0x00ff0100;

		if (vzoom == 4096)
			*(volatile u32*)(PIC_BASE_ADDR + 0x24) = 0x00ff0000;
		else
			*(volatile u32*)(PIC_BASE_ADDR + 0x24) = 0x00ff0100;

		for(i=0;i<64;i++)
			*(volatile u32*)( PIC_BASE_ADDR +0x0100 + i * 4) = sPhaseNomal[i];	//JPG_PARA_H_0~63
		for(i=0;i<64;i++)
			*(volatile u32*)( PIC_BASE_ADDR +0x0200 + i * 4) = sPhaseNomal[i];	//JPG_PARA_V_0~63
#else /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
		if(vdownscale)
			*(volatile u32*)(VPU_BASE_ADDR + 0x40) |= (1<<2);	//JPG_CTRL
		else
			*(volatile u32*)(VPU_BASE_ADDR + 0x40) &= ~(1<<2);

		if(hdownscale)
			*(volatile u32*)(VPU_BASE_ADDR + 0x40) |= (1<<3);
		else
			*(volatile u32*)(VPU_BASE_ADDR + 0x40) &= ~(1<<3);

		*(volatile u32*)(VPU_BASE_ADDR + 0x44)  = vtbais | vbbais<<16;	//JPG_V_PHASE
		*(volatile u32*)(VPU_BASE_ADDR + 0x48)  = (start_x + start_x_here) | (start_y + start_y_here)<<16;	//JPG_POSTITION
		*(volatile u32*)(VPU_BASE_ADDR + 0x4c)  = source_width | source_height<<16;	//JPG_SOURCE_SIZE
		*(volatile u32*)(VPU_BASE_ADDR + 0x50)  = dest_width | dest_height<<16;	//JPG_VIEW_SIZE
		*(volatile u32*)(VPU_BASE_ADDR + 0x54)  = hzoom | vzoom<<16;	//JPG_ZOOM
		*(volatile u32*)(VPU_BASE_ADDR + 0x58) &= ~(0x7ff<<16);	//JPG_PARA
		*(volatile u32*)(VPU_BASE_ADDR + 0x58) |= (((source_width>>2)>>3)<<3)<<16 ;
		*(volatile u32*)(VPU_BASE_ADDR + 0x58) &= ~(0x1<<29);
		*(volatile u32*)(VPU_BASE_ADDR + 0x58) |= vpu_mode<<29;

		if (hzoom == 4096)
			*(volatile u32*)(VPU_BASE_ADDR + 0x60) = 0x00ff0000;
		else
			*(volatile u32*)(VPU_BASE_ADDR + 0x60) = 0x00ff0100;

		if (vzoom == 4096)
			*(volatile u32*)(VPU_BASE_ADDR + 0x64) = 0x00ff0000;
		else
			*(volatile u32*)(VPU_BASE_ADDR + 0x64) = 0x00ff0100;

		for(i=0;i<64;i++)
			*(volatile u32*)( VPU_BASE_ADDR +0x0600 + i * 4) = sPhaseNomal[i];	//JPG_PARA_H_0~63
		for(i=0;i<64;i++)
			*(volatile u32*)( VPU_BASE_ADDR +0x0700 + i * 4) = sPhaseNomal[i];	//JPG_PARA_V_0~63
#endif /*CONFIG_ARCH_CKMMU_CYGNUS*/
	}else{ //osd
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
		vpu_osd_set_view_size(GX_LAYER_OSD, dest_width, dest_height);
		vpu_osd_set_zoom(GX_LAYER_OSD, vzoom, hzoom);
		vpu_osd_set_vtop_phase(GX_LAYER_OSD, 0);
		vpu_osd_set_zoom_para(GX_LAYER_OSD, sPhaseNomal, 64);
#elif defined (CONFIG_ARCH_CKMMU_CYGNUS) || defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA) || defined(CONFIG_ARCH_CKMMU_SCORPIO)
		*(volatile u32*)(VPU_BASE_ADDR + 0x4000 + 0x20)  = (start_x + start_x_here) | (start_y + start_y_here)<<16;	//OSD_POSTITION
		*(volatile u32*)(VPU_BASE_ADDR + 0x4000 + 0x8)   = dest_width | dest_height<<16;	//OSD_VIEW_SIZE
		*(volatile u32*)(VPU_BASE_ADDR + 0x4000 + 0xc)   = hzoom | vzoom<<16;	//OSD_ZOOM

		for(i=0;i<64;i++)
			*(volatile u32*)( VPU_BASE_ADDR +0x4100 + i * 4) = sPhaseNomal[i];	//OSD_PARA_0~63
#else
		*(volatile u32*)(VPU_BASE_ADDR + 0xac)  = (start_x + start_x_here) | (start_y + start_y_here)<<16;	//OSD_POSTITION
		*(volatile u32*)(VPU_BASE_ADDR + 0x98)  = dest_width | dest_height<<16;	//OSD_VIEW_SIZE
		*(volatile u32*)(VPU_BASE_ADDR + 0x9c)  = hzoom | vzoom<<16;	//OSD_ZOOM

		for(i=0;i<64;i++)
			*(volatile u32*)( VPU_BASE_ADDR +0x0400 + i * 4) = sPhaseNomal[i];	//OSD_PARA_0~63
//			*(volatile u32*)( VPU_BASE_ADDR +0x0400 + i * 4) = gx3201_osd_filter_table[i];	//OSD_PARA_0~63
#endif /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO*/
	}
	return 0;
}

