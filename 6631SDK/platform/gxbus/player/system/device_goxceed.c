#include "gx_device.h"
#include "gx_mediafilter.h"

#ifdef MULTIPROCESS
#define device_goxceed_config_set    (k, v) GxBus_ConfigSet   (k, v)
#define device_goxceed_config_set_int(k, v) GxBus_ConfigSetInt(k, v)
#define device_goxceed_config_get    (k, v, d) GxBus_ConfigGet   (k, v, d)
#define device_goxceed_config_get_int(k, v, d) GxBus_ConfigGetInt(k, v, d)
#else
#define device_goxceed_config_set    (k, v) ((void)0)
#define device_goxceed_config_set_int(k, v) ((void)0)
#define device_goxceed_config_get    (k, v, d) strcpy(v, d)
#define device_goxceed_config_get_int(k, v, d) *(int*)(v) = d
#endif

#define VOUT_FREE_BUF(buf) \
{ \
	if(buf){\
		av_memhole_free(buf, GX_PINFLAG_SVPU);\
		buf = NULL;\
	}\
}

#define CHECK_IFACE(iface) \
	do {\
		if (iface != GX_VIDEO_OUTPUT_RCA &&\
				iface != GX_VIDEO_OUTPUT_RCA1 &&\
				iface != GX_VIDEO_OUTPUT_YUV &&\
				iface != GX_VIDEO_OUTPUT_SCART &&\
				iface != GX_VIDEO_OUTPUT_SVIDEO &&\
				iface != GX_VIDEO_OUTPUT_HDMI) {\
			return -1;\
		}\
	} while(0)

#define HAVE_SVPU_SRC(select) \
	 (select& GX_VIDEO_OUTPUT_RCA ||\
	  select & GX_VIDEO_OUTPUT_RCA1 ||\
	  select & GX_VIDEO_OUTPUT_SCART ||\
	  select & GX_VIDEO_OUTPUT_SVIDEO)

#define IS_SVPU_SRC(iface) \
	(iface == GX_VIDEO_OUTPUT_RCA ||\
	 iface == GX_VIDEO_OUTPUT_RCA1 ||\
	 iface == GX_VIDEO_OUTPUT_SCART ||\
	 iface == GX_VIDEO_OUTPUT_SVIDEO)

#define VOUT_AUTO_CONFIG(select, iface, fr_type) \
	do {\
		if (select & iface)\
		vout_autoconfig(iface, fr_type);\
	} while(0)

#define CHECK_SELECT_IFACE(vout_select, iface) \
	do {\
		if (iface & (~vout_select)) {\
			return -1;\
		}\
	} while(0)

#define VOUT_CONFIG_MODE(select, iface, res) \
	do {\
		if (select & iface) {\
			VideoOutputConfig config;\
			config.interface = iface;\
			config.mode = res;\
			set_VideoOutputConfig(config);\
		}\
	} while(0)

#define VOUT_CONFIG_COLORS(select, iface, b, c, s) \
	do {\
		if (select & iface) {\
			VideoColor color;\
			color.interface = iface;\
			color.brightness = b;\
			color.contrast   = c;\
			color.saturation = s;\
			set_VideoColors(color);\
		}\
	} while(0)


#ifndef I386_LINUX

typedef struct {
	GxAvChipId chip;
	int        dev;
	handle_t   ao;
	handle_t   vo;
	handle_t   vpu;
	handle_t   vdec;
	handle_t   adec;
	handle_t   adec1;
	handle_t   stc;
	handle_t   jpeg;
	handle_t   hdmi;

	int default_ao;
	int default_vo;
	int default_vmode;
	int indexcache;
	int packetcache;
	int recordcache;
	int enable_ac3;
	int enable_aac;
	int enable_dts;
	int enable_adpcm;
	int esv_size;
	int esa_size;
	int ess_size;
	int pcm_size;
	int ac3_size;
	int eac3_size;
	int dts_size;
	int aac_size;
	int vol_right_now;
	int vol_step;
	int support_pp_zoom;
	int support_deinterlace;
	unsigned int freeze_size;
	unsigned int max_fb_size;
	GxPlayerSystem *psystem;
	GxVideoOutProperty_OutputConfig vout_save;
	GxAVDeviceCapability chipcap;
}GoXceedDevice;

static GoXceedDevice gx_device;

static status_t set_VideoAutoAdapt(PlayerVideoAutoAdapt config);

enum {
	FR_TYPE_PAL,
	FR_TYPE_NTSC,
	FR_TYPE_UNKNOWN
};

#define IS_P_RESOLUTION(mode) \
	((mode==GXAV_VOUT_PAL)\
	 ||(mode==GXAV_VOUT_PAL_N)\
	 ||(mode==GXAV_VOUT_PAL_NC)\
	 ||(mode==GXAV_VOUT_576I)\
	 ||(mode==GXAV_VOUT_576P)\
	 ||(mode==GXAV_VOUT_720P_50HZ)\
	 ||(mode==GXAV_VOUT_1080I_50HZ) \
	 ||(mode==GXAV_VOUT_1080P_50HZ))

#define IS_N_RESOLUTION(mode) \
	((mode==GXAV_VOUT_NTSC_443)\
	 ||(mode==GXAV_VOUT_NTSC_M)\
	 ||(mode==GXAV_VOUT_PAL_M)\
	 ||(mode==GXAV_VOUT_480I)\
	 ||(mode==GXAV_VOUT_480P)	\
	 ||(mode==GXAV_VOUT_720P_60HZ)\
	 ||(mode==GXAV_VOUT_1080I_60HZ)\
	 ||(mode==GXAV_VOUT_1080P_60HZ))

static int vout_resolution_reverse(GxVideoOutProperty_Mode *mode, unsigned int fr_n, unsigned int fr_p)
{
	if((*mode==GXAV_VOUT_PAL)
			||(*mode==GXAV_VOUT_PAL_N)
			||(*mode==GXAV_VOUT_PAL_NC)){
		*mode= fr_n;
	}else if((*mode==GXAV_VOUT_NTSC_M)
			||(*mode==GXAV_VOUT_PAL_M)
			||(*mode==GXAV_VOUT_NTSC_443)) {
		*mode= fr_p;
	}else if(*mode==GXAV_VOUT_480I) {
		*mode=GXAV_VOUT_576I;
	}else if(*mode==GXAV_VOUT_480P) {
		*mode=GXAV_VOUT_576P;
	}else if(*mode==GXAV_VOUT_576I) {
		*mode=GXAV_VOUT_480I;
	}else if(*mode==GXAV_VOUT_576P) {
		*mode=GXAV_VOUT_480P;
	}else if(*mode==GXAV_VOUT_720P_50HZ) {
		*mode=GXAV_VOUT_720P_60HZ;
	}else if(*mode==GXAV_VOUT_720P_60HZ) {
		*mode=GXAV_VOUT_720P_50HZ;
	}else if(*mode==GXAV_VOUT_1080I_50HZ) {
		*mode=GXAV_VOUT_1080I_60HZ;
	}else if(*mode==GXAV_VOUT_1080I_60HZ) {
		*mode=GXAV_VOUT_1080I_50HZ;
	}else if(*mode==GXAV_VOUT_1080P_50HZ) {
		*mode =GXAV_VOUT_1080P_60HZ;
	}else if(*mode==GXAV_VOUT_1080P_60HZ) {
		*mode =GXAV_VOUT_1080P_50HZ;
	}else {
		return -1;
	}
	return 0;
}

static status_t vout_autoconfig(VideoOutputInterface iface, int fr_type)
{
	int ret = 0;

	GxVideoOutProperty_Resolution Resolution;
	Resolution.iface = iface;
	ret = GxAVGetProperty(gx_device.dev, gx_device.vo,   \
			GxVideoOutPropertyID_Resolution, (void*)&Resolution, sizeof(GxVideoOutProperty_Resolution));

	if (ret == 0) {
		if ((fr_type == FR_TYPE_PAL  && IS_N_RESOLUTION(Resolution.mode)) ||
				(fr_type == FR_TYPE_NTSC && IS_P_RESOLUTION(Resolution.mode))) {
			vout_resolution_reverse(&(Resolution.mode), gx_device.psystem->vout.autontsc, gx_device.psystem->vout.autopal);
			ret = GxAVSetProperty(gx_device.dev, gx_device.vo,   \
					GxVideoOutPropertyID_Resolution, (void*)&Resolution, sizeof(GxVideoOutProperty_Resolution));
		}
	}
	else {
		if (fr_type == FR_TYPE_PAL)
			Resolution.mode = gx_device.psystem->vout.autopal;
		else
			Resolution.mode = gx_device.psystem->vout.autontsc;
		ret = GxAVSetProperty(gx_device.dev, gx_device.vo,   \
				GxVideoOutPropertyID_Resolution, (void*)&Resolution, sizeof(GxVideoOutProperty_Resolution));
	}

	return ret;
}

static status_t freeze_frame(int flag)
{
	static int freeze_flag = 0;

	if(flag == 0 && freeze_flag == 0)
	{
		PLAYER_FREEZE_FRAME freeze;
		GxPlayer_SystemGet(PSYS_CBK_FREEZE, &freeze);

		if(freeze)
		{
			int ret = freeze();

			if(ret != 0)
			{
				return GX_PLAYER_ERROR;
			}

			freeze_flag = 1;

			return GX_PLAYER_OK;
		}

		return GX_PLAYER_ERROR;
	}
	else if(flag == 1 && freeze_flag == 1)
	{
		PLAYER_UNFREEZE_FRAME unfreeze;
		GxPlayer_SystemGet(PSYS_CBK_UNFREEZE, &unfreeze);

		if(unfreeze)
		{
			unfreeze();
			freeze_flag = 0;
			return GX_PLAYER_OK;
		}

		return GX_PLAYER_ERROR;
	}

	return GX_PLAYER_OK;
}

////////////////////////////set///////////////////////////////////

static void set_MainSurface(GxLayerID layer, GxColorFormat format, GxSurfaceMode mode,
		unsigned int xres, unsigned yres, void *buffer)
{
	GxVpuProperty_LayerMainSurface MainSurface = {0};
	GxVpuProperty_CreateSurface CreateSurface = {0};
	GxVpuProperty_ModifySurface ModifySurface = {0};

	MainSurface.layer   = layer;
	GxAVGetProperty(gx_device.dev,gx_device.vpu,GxVpuPropertyID_LayerMainSurface,
			(void*)&MainSurface, sizeof(GxVpuProperty_LayerMainSurface));

	if(NULL == MainSurface.surface) {
		CreateSurface.width  = xres;
		CreateSurface.height = yres;
		CreateSurface.buffer = buffer;
		CreateSurface.format = format;
		CreateSurface.mode   = mode;
		CreateSurface.ext_param.colordepth = 8;
		GxAVGetProperty(gx_device.dev,
				gx_device.vpu,
				GxVpuPropertyID_CreateSurface,
				(void*)&CreateSurface,
				sizeof(GxVpuProperty_CreateSurface));

		MainSurface.layer   = layer;
		MainSurface.surface = CreateSurface.surface;
		GxAVSetProperty(gx_device.dev,
				gx_device.vpu,
				GxVpuPropertyID_LayerMainSurface,
				(void*)&MainSurface,
				sizeof(GxVpuProperty_LayerMainSurface));
	} else {
		ModifySurface.surface = MainSurface.surface;
		ModifySurface.color_format = format;
		ModifySurface.mode = mode;
		ModifySurface.width = xres;
		ModifySurface.height = yres;
		ModifySurface.buffer = buffer;
		GxAVSetProperty(gx_device.dev,
				gx_device.vpu,
				GxVpuPropertyID_ModifySurface,
				(void *)&ModifySurface,
				sizeof(GxVpuProperty_ModifySurface));
	}
}

static status_t set_VideoDisplayScreen(DisplayScreen scr)
{
	int interface = gx_device.psystem->vout.select;
	GxVpuProperty_LayerClipport cp = {0};
	GxVideoOutProperty_Screen screen;
	GxVpuProperty_VirtualResolution Resolution;

	screen = scr.aspect;

	Resolution.xres = scr.xres;
	Resolution.yres = scr.yres;

	gx_device.psystem->screen = scr;

	if (interface) {
		GxAVSetProperty(gx_device.dev, gx_device.vo, \
				GxVideoOutPropertyID_TvScreen, (void*)&screen, sizeof(GxVideoOutProperty_Screen));
	}

	GxAVSetProperty(gx_device.dev, gx_device.vpu, \
			GxVpuPropertyID_VirtualResolution, (void*)&Resolution, sizeof(GxVpuProperty_VirtualResolution));

	if (gx_device.psystem->clip.width && gx_device.psystem->clip.height) {
		cp.layer = GX_LAYER_VPP;
		cp.rect.x      = gx_device.psystem->clip.x;
		cp.rect.y      = gx_device.psystem->clip.y;
		cp.rect.width  = gx_device.psystem->clip.width;
		cp.rect.height = gx_device.psystem->clip.height;
		GxAVSetProperty(gx_device.dev, gx_device.vpu, \
				GxVpuPropertyID_LayerClipport, (void*)&cp, sizeof(GxVpuProperty_LayerClipport));
	}

	set_MainSurface(GX_LAYER_VPP, GX_COLOR_FMT_YCBCR420, GX_SURFACE_MODE_VIDEO, scr.xres, scr.yres, NULL);

	return GXCORE_SUCCESS;
}

static status_t set_VideoAspect(VideoAspect aspect )
{
	int interface = gx_device.psystem->vout.select;
	GxVideoOutProperty_AspectRatio AspectRatio;

	AspectRatio.spec = aspect;
	gx_device.psystem->aspect = aspect;

	if(interface)
	{
		GxAVSetProperty(gx_device.dev, gx_device.vo, \
				GxVideoOutPropertyID_AspectRatio, (void*)&AspectRatio, sizeof(GxVideoOutProperty_AspectRatio));
	}

	return GXCORE_SUCCESS;
}

static status_t set_VideoDrcMode(VideoDrcMode mode)
{
	GxVpuProperty_DrcMode drc = {0};

	gx_device.psystem->drc_mode = mode;

	drc.mode = mode;
	return GxAVSetProperty(gx_device.dev, gx_device.vpu, \
			GxVpuPropertyID_DrcMode, (void*)&drc, sizeof(GxVpuProperty_DrcMode));
}

static status_t set_VideoColors(VideoColor color)
{
	GxVideoOutProperty_Brightness bright;
	GxVideoOutProperty_Contrast   contrast;
	GxVideoOutProperty_Saturation saturation;
	GxVideoOutProperty_Sharpness  sharp;
	GxVideoOutProperty_Hue        hue;
	handle_t vo = gx_device.vo;

	CHECK_IFACE(color.interface);
	CHECK_SELECT_IFACE(gx_device.psystem->vout.select, color.interface);

	if (vo) {
		hue.value        = color.hue;
		sharp.value      = color.sharpness;
		bright.value     = color.brightness;
		contrast.value   = color.contrast;
		saturation.value = color.saturation;

		hue.iface =
			sharp.iface =
			bright.iface =
			contrast.iface =
			saturation.iface = color.interface;

		GxAVSetProperty(gx_device.dev, vo, \
				GxVideoOutPropertyID_Brightness, (void*)&bright, sizeof(GxVideoOutProperty_Brightness));
		GxAVSetProperty(gx_device.dev, vo, \
				GxVideoOutPropertyID_Contrast,(void*)&contrast, sizeof(GxVideoOutProperty_Contrast));
		GxAVSetProperty(gx_device.dev, vo, \
				GxVideoOutPropertyID_Saturation,(void*)&saturation, sizeof(GxVideoOutProperty_Saturation));

		if (gx_device.psystem->vout.sharpness_en == 1) {
			GxAVSetProperty(gx_device.dev, vo, \
					GxVideoOutPropertyID_Sharpness, (void*)&sharp, sizeof(GxVideoOutProperty_Sharpness));
		}
		if (gx_device.psystem->vout.hue_en == 1) {
			GxAVSetProperty(gx_device.dev, vo, \
					GxVideoOutPropertyID_Hue, (void*)&hue, sizeof(GxVideoOutProperty_Hue));
		}
	}

	return GXCORE_SUCCESS;
}

static status_t set_VideoOutputSelect(int select)
{
	int ret = GXCORE_SUCCESS;

	GxVideoOutProperty_OutputSelect output = {0};
	GxVideoOutProperty_OutputConfig config = {0};
	PlayerVideoAutoAdapt autoadapt = {0};

	autoadapt.enable = gx_device.psystem->vout.autoadapt;
	autoadapt.pal = gx_device.psystem->vout.autopal;
	autoadapt.ntsc = gx_device.psystem->vout.autontsc;
	set_VideoAutoAdapt(autoadapt);


	if (HAVE_SVPU_SRC(select)) {
		if (gx_device.vout_save.svpu_enable == 0) {
			if (gx_device.chip != GXAV_ID_GX3211)
				gx_device.vout_save.svpu_buf[0] = av_memhole_malloc(VOUT_BUF_SIZE*2/3, GX_PINFLAG_SVPU);
			else
				gx_device.vout_save.svpu_buf[0] = av_memhole_malloc(VOUT_BUF_SIZE, GX_PINFLAG_SVPU);
			gx_device.vout_save.svpu_buf[1] = gx_device.vout_save.svpu_buf[0];
			gx_device.vout_save.svpu_buf[2] = gx_device.vout_save.svpu_buf[0];
			if (gx_device.vout_save.svpu_buf[0] == NULL) {
				VOUT_FREE_BUF(gx_device.vout_save.svpu_buf[0]);
				return GXCORE_ERROR;
			}

			config.svpu_enable = 1;
			GxBus_ConfigGetInt(PLAYER_CONFIG_HDMI_DELAY,      (int32_t*)&config.hdmi_delayms, 40);
			GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_OUT_DELAY, (int32_t*)&config.svpu_delayms, 0);
			memcpy(config.svpu_buf, gx_device.vout_save.svpu_buf, sizeof(gx_device.vout_save.svpu_buf));

			if (gx_device.psystem->vout.autoadapt1) {
				config.vout1_auto.enable = 1;
				config.vout1_auto.pal    = gx_device.psystem->vout.autopal;
				config.vout1_auto.ntsc   = gx_device.psystem->vout.autontsc;
			}

			ret = GxAVSetProperty(gx_device.dev, gx_device.vo, \
					GxVideoOutPropertyID_OutputConfig, (void*)&config, sizeof(GxVideoOutProperty_OutputConfig));

			if(ret != GXCORE_SUCCESS) {
				VOUT_FREE_BUF(gx_device.vout_save.svpu_buf[0]);
				gx_device.vout_save.svpu_enable = 0;
			}else{
				gx_device.vout_save.svpu_enable = 1;
			}
		}
	} else {
		config.svpu_enable = 0;
		GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_OUT_DELAY, (int32_t*)&config.svpu_delayms, 0);
		ret = GxAVSetProperty(gx_device.dev, gx_device.vo, \
				GxVideoOutPropertyID_OutputConfig, (void*)&config, sizeof(GxVideoOutProperty_OutputConfig));
		if(gx_device.vout_save.svpu_enable) {// free the space for svpu before
			gx_device.vout_save.svpu_enable = 0;
			VOUT_FREE_BUF(gx_device.vout_save.svpu_buf[0]);
		}
	}

	if (select) {
		output.selection = select;
		ret = GxAVSetProperty(gx_device.dev, gx_device.vo, \
				GxVideoOutPropertyID_OutputSelect, (void*)&output, sizeof(GxVideoOutProperty_OutputSelect));
		if (GXCORE_SUCCESS == ret)
			gx_device.psystem->vout.select = select;
		else
			return ret;
	}

	return ret;
}

static status_t set_VideoOutputDef(VideoOutDefault outdef)
{
	int ret = GXCORE_ERROR;
	GxVideoOutProperty_OutDefault def;
	int select = gx_device.psystem->vout.select;

	def.enable = outdef.enable;
	def.iface = outdef.interface;

	CHECK_IFACE(outdef.interface);
	if(def.iface & select)
	{
		ret = GxAVSetProperty(gx_device.dev, gx_device.vo, \
				GxVideoOutPropertyID_OutDefault, (void*)&def, sizeof(GxVideoOutProperty_OutDefault));
	}

	return ret;
}

static status_t set_VideoOutputPower(unsigned int enable)
{
	int ret = GXCORE_SUCCESS;
	int interface = gx_device.psystem->vout.select;

	if(enable == 0){
		GxVideoOutProperty_PowerOff off;

		if(interface) {
			off.selection = interface;
			ret = GxAVSetProperty(gx_device.dev, gx_device.vo, \
					GxVideoOutPropertyID_PowerOff, (void*)&off, sizeof(GxVideoOutProperty_PowerOff));
		}
	}else{
		GxVideoOutProperty_PowerOn on;

		if(interface) {
			on.selection = interface;
			ret = GxAVSetProperty(gx_device.dev, gx_device.vo, \
					GxVideoOutPropertyID_PowerOn, (void*)&on, sizeof(GxVideoOutProperty_PowerOn));
		}
	}

	return ret;
}

static status_t set_VideoOutputPower2(VideoOutPower power)
{
	int ret = GXCORE_SUCCESS;

	CHECK_IFACE(power.interface);
	CHECK_SELECT_IFACE(gx_device.psystem->vout.select, power.interface);

	if(power.enable == 0){
		GxVideoOutProperty_PowerOff off;

		off.selection = power.interface;
		ret = GxAVSetProperty(gx_device.dev, gx_device.vo, \
				GxVideoOutPropertyID_PowerOff, (void*)&off, sizeof(GxVideoOutProperty_PowerOff));
	}else{
		GxVideoOutProperty_PowerOn on;

		on.selection = power.interface;
		ret = GxAVSetProperty(gx_device.dev, gx_device.vo, \
				GxVideoOutPropertyID_PowerOn, (void*)&on, sizeof(GxVideoOutProperty_PowerOn));
	}

	return ret;
}

static status_t set_VideoOutputScartMode(VideoOutputScartMode mode)
{
	int ret = GX_PLAYER_OK;
	GxVideoOutProperty_ScartConfig scart_config = {0};

	scart_config.enable = 1;
	scart_config.mode   = (mode == GX_SCART_CVBS ? SCART_CVBS : SCART_RGB);
	ret = GxAVSetProperty(gx_device.dev, gx_device.vo, \
			GxVideoOutPropertyID_ScartConfig, (void*)&scart_config, sizeof(GxVideoOutProperty_ScartConfig));

	return ret;
}

static status_t set_VideoOutputConfig(VideoOutputConfig config)
{
	int ret = GX_PLAYER_OK;
	int select = gx_device.psystem->vout.select;
	GxVideoOutProperty_Resolution Resolution = {0};

	CHECK_IFACE(config.interface);
	CHECK_SELECT_IFACE(select, config.interface);

	Resolution.iface = config.interface;
	Resolution.mode = config.mode;
	ret = GxAVSetProperty(gx_device.dev, gx_device.vo, \
			GxVideoOutPropertyID_Resolution, (void*)&Resolution, sizeof(GxVideoOutProperty_Resolution));

#if 0
	if (ret == GXCORE_SUCCESS && gx_device.psystem->vout.autoadapt1) {
		if (HAVE_SVPU_SRC(gx_device.psystem->vout.select) && !IS_SVPU_SRC(config.interface)) {
			int fr_type = FR_TYPE_UNKNOWN;
			VideoOutputInterface iface = GX_VIDEO_OUTPUT_RCA;

			if (IS_P_RESOLUTION((int)config.mode))
				fr_type = FR_TYPE_PAL;
			else if (IS_N_RESOLUTION((int)config.mode))
				fr_type = FR_TYPE_NTSC;

			if (select & GX_VIDEO_OUTPUT_RCA)
				iface = GX_VIDEO_OUTPUT_RCA;
			else if (select & GX_VIDEO_OUTPUT_RCA1)
				iface = GX_VIDEO_OUTPUT_RCA1;
			else if (select & GX_VIDEO_OUTPUT_SCART)
				iface = GX_VIDEO_OUTPUT_SCART;
			else if (select & GX_VIDEO_OUTPUT_SVIDEO)
				iface = GX_VIDEO_OUTPUT_SVIDEO;

			VOUT_AUTO_CONFIG(select, iface, fr_type);
		}
	}
#endif

	return ret;
}

#ifdef NO_OS
#define abs(x) ((x) < 0 ? (0-(x)):(x))
#endif
static status_t set_VideoAutoConfig(float ffps)
{
	if (gx_device.psystem->vout.autoadapt) {
		int fps = 1000*ffps;
		int fr_type = FR_TYPE_UNKNOWN;
		int select = gx_device.psystem->vout.select;

		if (abs(fps-25000) <= 2000 || abs(fps-50000) <= 2000) {
			fr_type = FR_TYPE_PAL;
		}
		else if (abs(fps-30000) <= 2000 || abs(fps-60000) <= 2000) {
			fr_type = FR_TYPE_NTSC;
		}

		if (select & GX_VIDEO_OUTPUT_HDMI) {
			VOUT_AUTO_CONFIG(select, GX_VIDEO_OUTPUT_HDMI, fr_type);
		}
		else if (select & GX_VIDEO_OUTPUT_YUV) {
			VOUT_AUTO_CONFIG(select, GX_VIDEO_OUTPUT_YUV, fr_type);
		}

		VOUT_AUTO_CONFIG(select, GX_VIDEO_OUTPUT_RCA, fr_type);
		VOUT_AUTO_CONFIG(select, GX_VIDEO_OUTPUT_RCA1, fr_type);
		VOUT_AUTO_CONFIG(select, GX_VIDEO_OUTPUT_SVIDEO, fr_type);
		VOUT_AUTO_CONFIG(select, GX_VIDEO_OUTPUT_SCART, fr_type);
	}

	return GXCORE_SUCCESS;
}

static status_t set_VideoAutoAdapt(PlayerVideoAutoAdapt config)
{
	config.enable = (config.enable == 0) ? 0: 1;

	if(config.pal != (int)GXAV_VOUT_PAL &&
			config.pal != (int)GXAV_VOUT_PAL_NC &&
			config.pal != (int)GXAV_VOUT_PAL_N) {
		config.pal = GXAV_VOUT_PAL;
	}

	if(config.ntsc != (int)GXAV_VOUT_PAL_M &&
			config.ntsc != (int)GXAV_VOUT_NTSC_M &&
			config.ntsc != (int)GXAV_VOUT_NTSC_443) {
		config.ntsc = GXAV_VOUT_NTSC_M;
	}

	gx_device.psystem->vout.autoadapt = config.enable;
	gx_device.psystem->vout.autopal = config.pal;
	gx_device.psystem->vout.autontsc = config.ntsc;

	return GXCORE_SUCCESS;
}

static status_t set_VideoView(PlayerWindow window)
{
	GxVpuProperty_LayerViewport vp;

	vp.layer = GX_LAYER_VPP;
	vp.rect.x = window.x;
	vp.rect.y = window.y;
	vp.rect.width  = window.width;
	vp.rect.height = window.height;

	gx_device.psystem->view = window;

	return GxAVSetProperty(gx_device.dev, gx_device.vpu, \
			GxVpuPropertyID_LayerViewport, (void*)&vp, sizeof(GxVpuProperty_LayerViewport));
}

static status_t set_VideoClip(PlayerWindow window)
{
	GxVpuProperty_LayerClipport vp;

	vp.layer = GX_LAYER_VPP;
	vp.rect.x = window.x;
	vp.rect.y = window.y;
	vp.rect.width  = window.width;
	vp.rect.height = window.height;

	gx_device.psystem->clip = window;

	return GxAVSetProperty(gx_device.dev, gx_device.vpu, \
			GxVpuPropertyID_LayerClipport, (void*)&vp, sizeof(GxVpuProperty_LayerClipport));
}

static status_t set_CvbsViewport(PlayerWindow window)
{
	GxVpuProperty_SVPUConfig config = {0};

	config.x_margin = (gx_device.psystem->screen.xres - window.width)/2;
	config.y_margin = (gx_device.psystem->screen.yres - window.height)/2;
	config.x_shift  = window.x;
	config.y_shift  = window.y;
	return GxAVSetProperty(gx_device.dev, gx_device.vpu, \
			GxVpuPropertyID_SVPUConfig, (void*)&config, sizeof(GxVpuProperty_SVPUConfig));
}

static status_t set_VideoHide(PlayerLayer layer)
{
	GxVpuProperty_LayerEnable LayerEnable;

	LayerEnable.layer  = layer;
	LayerEnable.enable = 0;

	return GxAVSetProperty(gx_device.dev, gx_device.vpu, \
			GxVpuPropertyID_LayerEnable, (void*)&LayerEnable, sizeof(GxVpuProperty_LayerEnable));
}

static status_t set_Background(PlayerLayer layer);
static status_t _set_VideoShow(PlayerLayer layer)
{
	GxVpuProperty_LayerEnable LayerEnable;

	LayerEnable.layer  = layer;
	LayerEnable.enable = 1;

	return GxAVSetProperty(gx_device.dev, gx_device.vpu, \
			GxVpuPropertyID_LayerEnable, (void*)&LayerEnable, sizeof(GxVpuProperty_LayerEnable));
}

static status_t set_VideoShow(PlayerVideoShow show)
{
	if (show.force) {
		 return _set_VideoShow(show.layer);
	}
	else {
		int ret;
		GxVideoDecoderProperty_FrameInfo frame;

		ret = GxAVGetProperty(gx_device.dev, gx_device.vdec, \
				GxVideoDecoderPropertyID_FrameInfo, &frame,sizeof(GxVideoDecoderProperty_FrameInfo));
		if (ret == GXCORE_SUCCESS && frame.stat.play_frame_cnt > 0) {
			return _set_VideoShow(show.layer);
		}
		else {
			return set_Background(show.layer);
		}
	}
}

static status_t set_StreamWidth(int width)
{
	if (width <= 1920)
		gx_device.psystem->background.last_stream_width = width;
	return GXCORE_SUCCESS;
}

static status_t set_StreamHeight(int height)
{
	if (height <= 1088)
		gx_device.psystem->background.last_stream_height = height;
	return GXCORE_SUCCESS;
}

static status_t set_StreamStride(int stride)
{
	if (stride <= 1920)
		gx_device.psystem->background.last_stream_stride = stride;
	return GXCORE_SUCCESS;
}

#define SIZE_ALIGN(val, align) ((val+align-1)&~(align-1))
static status_t set_Background(PlayerLayer layer)
{
	int ret = GX_PLAYER_ERROR;
	int stride = 1920, height = 1088;
	unsigned int kbuf = 0, ubuf = 0;
	unsigned int y_size = 0, uv_size = 0;

	PlayerWindow window = {0};

	ret = GxPlayer_SystemFreezeFrameBufferAlloc(0, &kbuf, NULL);
	if (ret != 0)
		return ret;

	ubuf = (unsigned int)GxCore_MemholeMap((unsigned char*)kbuf);

	if (ubuf && kbuf) {
		/* fill background */
		if (gx_device.psystem->background.last_stream_width && gx_device.psystem->background.last_stream_height) {
			stride = gx_device.psystem->background.last_stream_stride;
			stride = stride > 1920 ? 1920 : stride;
			height = gx_device.psystem->background.last_stream_height;
			height = height > 1088 ? 1088 : height;
		}
		stride = SIZE_ALIGN(stride, 8);
		height = SIZE_ALIGN(height, 8);

		y_size  = stride*height;
		uv_size = stride*height/2;
#if 0
		memset((void*)ubuf,          0x10,  y_size);
		memset((void*)(ubuf+y_size), 0x80, uv_size);
#else
		GxAvdev_Memset((void *)kbuf, 0x10, stride, height);
		GxAvdev_Memset((void *)(kbuf+y_size), 0x80, stride, height/2);
#endif

		/* show background */
		set_MainSurface(GX_LAYER_VPP, GX_COLOR_FMT_YCBCR420_Y_UV, GX_SURFACE_MODE_IMAGE, stride, height, (void*)kbuf);

		window = gx_device.psystem->clip;
		set_VideoClip(window);

		window = gx_device.psystem->view;
		set_VideoView(window);

		if (gx_device.psystem->vout.closed == 0)
			_set_VideoShow(layer);
	}

	return ret;
}

static status_t set_AudioOutputSelect(int interface)
{
	gx_device.psystem->aout.interface = interface;
	return 0;
}

static status_t set_AudioOutputConfigPort(AudioOutputConfig para)
{
	GxAudioOutProperty_ConfigPort config;

	config.port_type = para.output_port;
	config.data_type = para.output_data;

	return GxAVSetProperty(gx_device.dev, gx_device.ao,   \
			GxAudioOutPropertyID_ConfigPort, (void*)&config, sizeof(GxAudioOutProperty_ConfigPort));
}

static status_t set_AudioVolumeTable(unsigned int table_idx)
{
	GxAudioOutProperty_VolumeTable table;

	table.table_idx    = table_idx;

	return GxAVSetProperty(gx_device.dev, gx_device.ao,   \
			GxAudioOutPropertyID_VolumeTable, (void*)&table, sizeof(GxAudioOutProperty_VolumeTable));
}

static status_t set_AudioVolume(unsigned int vol)
{
	GxAudioOutProperty_Volume  volume;

	volume.volume    = vol;
	volume.right_now = gx_device.vol_right_now;
	volume.step      = gx_device.vol_step;

	gx_device.psystem->aout.volume = vol;

	device_goxceed_config_set_int(PLAYER_CONFIG_AUDIO_VOLUME, vol);

	return GxAVSetProperty(gx_device.dev, gx_device.ao,   \
			GxAudioOutPropertyID_Volume, (void*)&volume,sizeof(GxAudioOutProperty_Volume));
}

static status_t set_AudioTrack(AudioTrack track)
{
	int ret=0;
	GxAudioOutProperty_Channel channel;

	channel.mono = 0;

	switch(track){
	case AUDIO_TRACK_MONO:
		channel.mono = 1;
	case AUDIO_TRACK_LEFT:
		channel.left   = AUDIO_CHANNEL_0;
		channel.right  = AUDIO_CHANNEL_0;
		channel.lfe    = AUDIO_CHANNEL_0;
		channel.center = AUDIO_CHANNEL_0;
		channel.left_surround  = AUDIO_CHANNEL_0;
		channel.right_surround = AUDIO_CHANNEL_0;
		channel.left_backsurround = AUDIO_CHANNEL_0;
		channel.right_backsurround = AUDIO_CHANNEL_0;
		break;
	case AUDIO_TRACK_RIGHT:
		channel.left   = AUDIO_CHANNEL_1;
		channel.right  = AUDIO_CHANNEL_1;
		channel.lfe    = AUDIO_CHANNEL_1;
		channel.center = AUDIO_CHANNEL_1;
		channel.left_surround  = AUDIO_CHANNEL_1;
		channel.right_surround = AUDIO_CHANNEL_1;
		channel.left_backsurround = AUDIO_CHANNEL_1;
		channel.right_backsurround = AUDIO_CHANNEL_1;
		break;
	case AUDIO_TRACK_STEREO:
		channel.left   = AUDIO_CHANNEL_0;
		channel.right  = AUDIO_CHANNEL_1;
		channel.lfe    = AUDIO_CHANNEL_2;
		channel.center = AUDIO_CHANNEL_3;
		channel.left_surround  = AUDIO_CHANNEL_4;
		channel.right_surround = AUDIO_CHANNEL_5;
		channel.left_backsurround = AUDIO_CHANNEL_6;
		channel.right_backsurround = AUDIO_CHANNEL_7;
		break;
	case AUDIO_TRACK_MONO_MUTE_LEFT:
		channel.mono = 1;
	case AUDIO_TRACK_RIGHT_MUTE_LEFT:
		channel.left   = AUDIO_CHANNEL_M;
		channel.right  = AUDIO_CHANNEL_1;
		channel.lfe    = AUDIO_CHANNEL_M;
		channel.center = AUDIO_CHANNEL_M;
		channel.left_surround  = AUDIO_CHANNEL_M;
		channel.right_surround = AUDIO_CHANNEL_M;
		channel.left_backsurround = AUDIO_CHANNEL_M;
		channel.right_backsurround = AUDIO_CHANNEL_M;
		break;
	case AUDIO_TRACK_MONO_MUTE_RIGHT:
		channel.mono = 1;
	case AUDIO_TRACK_LEFT_MUTE_RIGHT:
		channel.left   = AUDIO_CHANNEL_0;
		channel.right  = AUDIO_CHANNEL_M;
		channel.lfe    = AUDIO_CHANNEL_M;
		channel.center = AUDIO_CHANNEL_M;
		channel.left_surround  = AUDIO_CHANNEL_M;
		channel.right_surround = AUDIO_CHANNEL_M;
		channel.left_backsurround = AUDIO_CHANNEL_M;
		channel.right_backsurround = AUDIO_CHANNEL_M;
		break;
	case AUDIO_TRACK_SPDIF_LR:
		channel.left   = AUDIO_CHANNEL_0;
		channel.right  = AUDIO_CHANNEL_2;
		break;
	case AUDIO_TRACK_SPDIF_LsRs:
		channel.left   = AUDIO_CHANNEL_3;
		channel.right  = AUDIO_CHANNEL_4;
		break;
	case AUDIO_TRACK_SPDIF_CS:
		channel.left   = AUDIO_CHANNEL_1;
		channel.right  = AUDIO_CHANNEL_5;
		break;
	case AUDIO_TRACK_SPDIF_X1X2:
		channel.left   = AUDIO_CHANNEL_6;
		channel.right  = AUDIO_CHANNEL_7;
		break;
	default:
		return GXCORE_ERROR;
	}

	gx_device.psystem->aout.track = track;

	ret |= GxAVSetProperty(gx_device.dev, gx_device.ao,   \
			GxAudioOutPropertyID_Channel, (void*)&channel,sizeof(GxAudioOutProperty_Channel));

	return ret;
}

static status_t set_AudioDescriptorTrack(AudioDecriptorTrack track)
{
	int ret=0;
	GxAudioOutProperty_PcmMix pcm_mix;

	if (track == AUDIO_PRIMARY_TRACK)
		pcm_mix.pcm_mix = MIX_PCM;
	else if (track == AUDIO_DESCRIPTOR_TRACK)
		pcm_mix.pcm_mix = MIX_ADPCM;
	else
		pcm_mix.pcm_mix = MIX_PCM|MIX_ADPCM;

	gx_device.psystem->aout.descriptor_track = track;

	ret |= GxAVSetProperty(gx_device.dev, gx_device.ao,   \
			GxAudioOutPropertyID_PcmMix, (void*)&pcm_mix, sizeof(GxAudioOutProperty_PcmMix));

	return ret;
}

static status_t set_AudioBoostVolume(unsigned int vol)
{
	GxAudioDecProperty_BoostVolume  boost;

	boost.index = vol;

	gx_device.psystem->audio_boost_volume = vol;

	return GxAVSetProperty(gx_device.dev, gx_device.adec,   \
			GxAudioDecoderPropertyID_BoostVolume, (void*)&boost, sizeof(GxAudioDecProperty_BoostVolume));
}

static status_t set_AudioDescriptorBoostVolume(unsigned int vol)
{
	GxAudioDecProperty_BoostVolume  boost;

	if (!gx_device.adec1) {
		gx_device.adec1 = GxAvdev_OpenModule(gx_device.dev, GXAV_MOD_AUDIO_DECODE, 1);
		if (!gx_device.adec1)
			return GX_PLAYER_ERROR;
	}

	boost.index = vol;
	gx_device.psystem->audio_decriptor_boost_volume = vol;

	return GxAVSetProperty(gx_device.dev, gx_device.adec1,   \
			GxAudioDecoderPropertyID_BoostVolume, (void*)&boost, sizeof(GxAudioDecProperty_BoostVolume));
}

static status_t set_AudioMute(int enable)
{
	GxAudioOutProperty_Mute mute;

	gx_device.psystem->aout.mute = enable;

	mute.mute = enable ? 1 : 0;

	device_goxceed_config_set_int(PLAYER_CONFIG_AUDIO_MUTE, enable);

	return GxAVSetProperty(gx_device.dev, gx_device.ao,   \
			GxAudioOutPropertyID_Mute,(void*)&mute, sizeof(GxAudioOutProperty_Mute));
}

static status_t set_AudioPowerMute(int enable)
{
	GxAudioOutProperty_PowerMute power_mute;

	power_mute.enable = enable;

	return GxAVSetProperty(gx_device.dev, gx_device.ao,   \
			GxAudioOutPropertyID_PowerMute,(void*)&power_mute, sizeof(GxAudioOutProperty_PowerMute));
}

static status_t set_AudioOutPort(AudioOutPortSet port)
{
	GxAudioOutProperty_SetPort output_port;

	if (port.output_port == AUDIO_OUTPUT_HDMI) {
		output_port.port_type      = AUDIO_OUT_HDMI;
		output_port.hdmi.samplefre = port.hdmi.samplefre;
		output_port.hdmi.channel   = port.hdmi.channels;

		return GxAVSetProperty(gx_device.dev, gx_device.ao,   \
				GxAudioOutPropertyID_SetPort,(void*)&output_port, sizeof(GxAudioOutProperty_SetPort));
	}

	return -1;
}

static status_t set_AudioOutPortTurn(AudioOutPortTurn port)
{
	GxAudioOutProperty_TurnPort output_port = {0, 0};

	if ((port.output_port & AUDIO_OUTPUT_DAC_IN) == AUDIO_OUTPUT_DAC_IN)
		output_port.port_type |= AUDIO_OUT_I2S_IN_DAC;
	if ((port.output_port & AUDIO_OUTPUT_DAC_OUT) == AUDIO_OUTPUT_DAC_OUT)
		output_port.port_type |= AUDIO_OUT_I2S_OUT_DAC;
	if ((port.output_port & AUDIO_OUTPUT_SPDIF) == AUDIO_OUTPUT_SPDIF)
		output_port.port_type |= AUDIO_OUT_SPDIF;
	if ((port.output_port & AUDIO_OUTPUT_HDMI) == AUDIO_OUTPUT_HDMI)
		output_port.port_type |= AUDIO_OUT_HDMI;
	output_port.onoff     = port.onoff;

	return GxAVSetProperty(gx_device.dev, gx_device.ao,   \
			GxAudioOutPropertyID_TurnPort,(void*)&output_port, sizeof(GxAudioOutProperty_TurnPort));
}

static status_t set_AudioDACVolume(int voldb)
{
	GxAudioOutProperty_DACVolume vol;

	vol.voldb = voldb;
	return GxAVSetProperty(gx_device.dev, gx_device.ao,   \
			GxAudioOutPropertyID_DAC,(void*)&vol, sizeof(GxAudioOutProperty_DACVolume));
}

static status_t set_AudioVolumeFactor(unsigned int factor)
{
	GxAPlayProperty_Control ctrl;

	ctrl.cmd   = GX_APLAY_CONTROL_SET_VOLUME_FACTOR;
	ctrl.arg13 = factor;
	return GxAVSetProperty(gx_device.dev, gx_device.ao,   \
			GxAPlayPropertyID_Control, (void*)&ctrl, sizeof(GxAPlayProperty_Control));
}

static status_t set_VideoDeinterlaceEnable(int enable)
{
	GxVideoDecoderProperty_DeinterlaceEnable param = {0};

	param.enable = enable;
	gx_device.psystem->vdec.enable_deinterlace = enable;

	return GxAVSetProperty(gx_device.dev, gx_device.vdec,   \
			GxVideoDecoderPropertyID_DeinterlaceEnable, (void*)&param, sizeof(param));
}

static status_t set_VideoMosaicDropGate(int drop_gate)
{
	GxVideoDecoderProperty_MosaicDropGate param = {0};
	gx_device.psystem->vdec.mosaic_gate = drop_gate;

	param.err_gate = drop_gate;
	return GxAVSetProperty(gx_device.dev, gx_device.vdec,   \
			GxVideoDecoderPropertyID_MosaicDropGate, (void*)&param, sizeof(param));
}

static status_t set_VideoPlayBypass(int enable)
{
	GxVideoDecoderProperty_PlayBypass param = {0};

	param.enable = enable;
	return GxAVSetProperty(gx_device.dev, gx_device.vdec,   \
			GxVideoDecoderPropertyID_PlayBypass, (void*)&param, sizeof(param));
}

static status_t set_DebugAdecConfig(int level)
{
	struct gxav_debug_config config;

	config.module = GXAV_MOD_AUDIO_DECODE;
	config.level  = level;
	return GxAVConfigDebug(gx_device.dev, &config);
}

static status_t set_DebugAdecContent(int content)
{
	GxAudioDecProperty_Debug config;

	config.value = content;
	return GxAVSetProperty(gx_device.dev, gx_device.adec,
			GxAudioDecoderPropertyID_Debug, &config, sizeof(GxAudioDecProperty_Debug));
}

static status_t set_DebugAoutConfig(int level)
{
	struct gxav_debug_config config;

	config.module = GXAV_MOD_AUDIO_OUT;
	config.level  = level;
	return GxAVConfigDebug(gx_device.dev, &config);
}

static status_t set_DebugAoutContent(int content)
{
	GxAudioOutProperty_Debug config;

	config.value = content;
	return GxAVSetProperty(gx_device.dev, gx_device.ao,
			GxAudioOutPropertyID_Debug, &config, sizeof(GxAudioOutProperty_Debug));
}

static status_t set_AudioDolbyCertify(int enable)
{
	return GxAVDolbyCertifyEnable(gx_device.dev, enable);
}

static status_t set_AudioSpeedMode(AudioOutputSpeedMode mode)
{
	GxADSPProperty_Control actrl;

	actrl.cmd  = GX_ADSP_CONTROL_SET_SPEED_MODE;
	actrl.arg6 = (mode == AUDIO_OUTPUT_FAST) ? 2 : 1;
	return GxAVSetProperty(gx_device.dev, gx_device.adec,   \
			GxADSPPropertyID_Control, (void*)&actrl, sizeof(GxADSPProperty_Control));
}

static status_t get_SystemVersion(unsigned int* version)
{
	*version = GxAvdev_GetKernelVersion();

	return GX_PLAYER_OK;
}

static status_t get_VideoDisplayScreen(DisplayScreen* scr)
{
	*scr = gx_device.psystem->screen;

	return GX_PLAYER_OK;
}

static status_t get_VideoAspect(VideoAspect* aspect)
{
	*aspect = gx_device.psystem->aspect;

	return GX_PLAYER_OK;
}

static status_t get_VideoDrcMode(VideoDrcMode* mode)
{
	*mode = gx_device.psystem->drc_mode;

	return GX_PLAYER_OK;
}

static status_t get_VideoColors(VideoColor* color)
{
	GxVideoOutProperty_Brightness bright;
	GxVideoOutProperty_Contrast   contrast;
	GxVideoOutProperty_Saturation saturation;
	GxVideoOutProperty_Sharpness  sharp;
	GxVideoOutProperty_Hue        hue;
	handle_t vo = gx_device.vo;

	CHECK_IFACE(color->interface);
	CHECK_SELECT_IFACE(gx_device.psystem->vout.select, color->interface);

	if (vo) {

		hue.iface = contrast.iface = bright.iface = sharp.iface = saturation.iface = color->interface;

		GxAVGetProperty(gx_device.dev, vo, \
				GxVideoOutPropertyID_Brightness, (void*)&bright, sizeof(GxVideoOutProperty_Brightness));
		GxAVGetProperty(gx_device.dev, vo, \
				GxVideoOutPropertyID_Contrast,(void*)&contrast, sizeof(GxVideoOutProperty_Contrast));
		GxAVGetProperty(gx_device.dev, vo, \
				GxVideoOutPropertyID_Saturation,(void*)&saturation, sizeof(GxVideoOutProperty_Saturation));
		GxAVGetProperty(gx_device.dev, vo, \
				GxVideoOutPropertyID_Sharpness,(void*)&sharp, sizeof(GxVideoOutProperty_Sharpness));
		GxAVGetProperty(gx_device.dev, vo, \
				GxVideoOutPropertyID_Hue, (void*)&hue, sizeof(GxVideoOutProperty_Hue));

		color->brightness = bright.value;
		color->saturation = saturation.value;
		color->contrast   = contrast.value;

		if (gx_device.psystem->vout.sharpness_en == 1)
			color->sharpness  = sharp.value;
		if (gx_device.psystem->vout.hue_en == 1)
			color->hue        = hue.value;
	}


	return GX_PLAYER_OK;
}

static status_t get_VideoOutputSelect(int* interface)
{
	*interface = gx_device.psystem->vout.select;

	return GX_PLAYER_OK;
}

static status_t get_VideoOutputConfig(VideoOutputConfig* config)
{
	int ret = GX_PLAYER_OK;
	GxVideoOutProperty_Resolution Resolution;

	CHECK_IFACE(config->interface);
	CHECK_SELECT_IFACE(gx_device.psystem->vout.select, config->interface);

	Resolution.iface = config->interface;
	ret = GxAVGetProperty(gx_device.dev, gx_device.vo,   \
			GxVideoOutPropertyID_Resolution, (void*)&Resolution, sizeof(GxVideoOutProperty_Resolution));

	if(ret == GXCORE_SUCCESS)
		config->mode = Resolution.mode;

	return ret;
}


static status_t get_VideoAutoAdapt(PlayerVideoAutoAdapt* Auto)
{
	Auto->enable = gx_device.psystem->vout.autoadapt;
	Auto->pal = gx_device.psystem->vout.autopal;
	Auto->ntsc = gx_device.psystem->vout.autontsc;

	return GX_PLAYER_OK;
}

static status_t get_VideoView(PlayerWindow* window)
{
	*window = gx_device.psystem->view;

	return GX_PLAYER_OK;
}

static status_t get_VideoClip(PlayerWindow* window)
{
	*window = gx_device.psystem->clip;

	return GX_PLAYER_OK;
}

static status_t get_VideoUsableIface(int *iface)
{
	*iface = GX_VIDEO_OUTPUT_RCA | GX_VIDEO_OUTPUT_RCA1 | GX_VIDEO_OUTPUT_YUV | GX_VIDEO_OUTPUT_SCART |
		GX_VIDEO_OUTPUT_SVIDEO | GX_VIDEO_OUTPUT_HDMI;

	return GX_PLAYER_OK;
}

static status_t get_VideoCapture(PlayerSnapshot* snap)
{
	int ret;
	GxVpuProperty_LayerCapture  capture;
	GxVpuProperty_CreateSurface surface;

	if(snap->surface == NULL)
	{
		surface.buffer = NULL;
		surface.format = GX_COLOR_FMT_YCBCR422;
		surface.width  = snap->rect.width;
		surface.height = snap->rect.height;
		surface.mode   = GX_SURFACE_MODE_IMAGE;

		ret = GxAVGetProperty(gx_device.dev, gx_device.vpu, \
				GxVpuPropertyID_CreateSurface, (void*)&surface, sizeof(GxVpuProperty_CreateSurface));

		if(ret)
		{
			return GXCORE_ERROR;
		}
		snap->surface = surface.surface;
	}

	capture.layer   = snap->layer;
	capture.rect.x    = snap->rect.x;
	capture.rect.y    = snap->rect.y;
	capture.rect.width  = snap->rect.width;
	capture.rect.height = snap->rect.height;
	capture.surface = snap->surface;
	ret = GxAVGetProperty(gx_device.dev, gx_device.vpu, \
			GxVpuPropertyID_LayerCapture, (void*)&capture, sizeof(GxVpuProperty_LayerCapture));

	return ret;
}


static status_t get_AudioOutputSelect(int* interface)
{
	*interface = gx_device.psystem->aout.interface;

	return GX_PLAYER_OK;
}

static status_t get_AudioOutputConfig(AudioOutputConfig* para)
{
	int ret = GX_PLAYER_OK;
	GxAudioOutProperty_ConfigPort config;

	config.port_type = para->output_port;

	ret = GxAVGetProperty(gx_device.dev, gx_device.ao,   \
			GxAudioOutPropertyID_ConfigPort, (void*)&config, sizeof(GxAudioOutProperty_ConfigPort));

	para->output_data = config.data_type;

	return ret;
}

static status_t get_AudioVolume(unsigned int* vol)
{
#if 0
	int ret = GX_PLAYER_OK;
	GxAudioOutProperty_Volume  volume;

	ret = GxAVGetProperty(gx_device.dev, gx_device.ao,   \
			GxAudioOutPropertyID_Volume, (void*)&volume,sizeof(GxAudioOutProperty_Volume));

	if(ret == GXCORE_SUCCESS)
		gx_device.psystem->aout.volume = volume.volume;
#endif

	*vol = gx_device.psystem->aout.volume;

	return GX_PLAYER_OK;
}

static status_t get_AudioTrack(AudioTrack* track)
{
	*track = gx_device.psystem->aout.track;

	return GX_PLAYER_OK;
}

static status_t get_AudioDescriptorVaild(PlayerVaildAD* vaild)
{
	GxAudioDecProperty_SupportAD support;

	support.type    = vaild->codec;
	support.mode    = vaild->mode;
	support.support = 0;
	GxAVGetProperty(gx_device.dev, gx_device.adec,
			GxAudioDecoderPropertyID_SupportAD, &support, sizeof(GxAudioDecProperty_SupportAD));
	vaild->support = support.support;
	return GX_PLAYER_OK;
}

static status_t get_AudioDescriptorTrack(AudioDecriptorTrack* track)
{
	*track = gx_device.psystem->aout.descriptor_track;

	return GX_PLAYER_OK;
}

static status_t get_AudioBoostVolume(unsigned int* vol)
{
	*vol = gx_device.psystem->audio_boost_volume;

	return GX_PLAYER_OK;
}

static status_t get_AudioDescriptorBoostVolume(unsigned int* vol)
{
	*vol = gx_device.psystem->audio_decriptor_boost_volume;

	return GX_PLAYER_OK;
}

static status_t get_AudioMute(int* enable)
{
	int ret = GX_PLAYER_OK;
	int smute = gx_device.psystem->aout.mute;
	GxAudioOutProperty_Mute mute;

	device_goxceed_config_get_int(PLAYER_CONFIG_AUDIO_MUTE, &smute, gx_device.psystem->aout.mute);

	if(smute != -1)
	{
		ret = GxAVGetProperty(gx_device.dev, gx_device.ao,   \
				GxAudioOutPropertyID_Mute, (void*)&mute,sizeof(GxAudioOutProperty_Mute));

		if(ret == GXCORE_SUCCESS)
			gx_device.psystem->aout.mute = mute.mute;
	}

	*enable = smute;

	return GX_PLAYER_OK;
}

static status_t get_FirewallFlag(int *flag)
{
	*flag = GxAvdev_GetFirewallFlag();
	return 0;
}

static status_t encrypt_Key(EncryptKey *key)
{
#define GX_MEMCPY(dst, src) do {                                \
	int l = sizeof(dst)>sizeof(src)?sizeof(src):sizeof(dst);    \
	memcpy(dst, src, l);                                        \
} while(0)

	int ret = GX_PLAYER_OK;
	uint32_t event;
	GxAudioDecProperty_DecodeKey dec_key;
	GxAudioDecProperty_GetKey get_key;

	GX_MEMCPY(dec_key.map,       key->map);
	GX_MEMCPY(dec_key.map_key,   key->map_key);
	GX_MEMCPY(dec_key.rand_data, key->rand_data);
	GX_MEMCPY(dec_key.chip_id,   key->chip_id);
	GX_MEMCPY(dec_key.scr_key,   key->scr_key);
	GxAVSetProperty(gx_device.dev, gx_device.adec,   \
			GxAudioDecoderPropertyID_DecodeKey, (void*)&dec_key, sizeof(GxAudioDecProperty_DecodeKey));

	ret = GxAVWaitEvents(gx_device.dev, gx_device.adec, EVENT_AUDIO_KEY, key->timeout_ms*1000, &event);
	if (ret < 0)
		return GX_PLAYER_ERROR;

	if ((event&EVENT_AUDIO_KEY) != EVENT_AUDIO_KEY)
		return GX_PLAYER_ERROR;

	GxAVGetProperty(gx_device.dev, gx_device.adec,   \
			GxAudioDecoderPropertyID_GetKey, (void*)&get_key, sizeof(GxAudioDecProperty_GetKey));
	memcpy(key->dec_key, get_key.dec_key, sizeof(get_key.dec_key));

	return GX_PLAYER_OK;
}

static status_t get_VideoDeinterlaceEnable(int* enable)
{
	*enable = gx_device.psystem->vdec.enable_deinterlace;
	return GX_PLAYER_OK;
}

static status_t get_VideoMosaicDropGate(int* err_gate)
{
	*err_gate = gx_device.psystem->vdec.mosaic_gate;
	return GX_PLAYER_OK;
}

static int set_sysinit(void* arg)
{
	GxPlayerSystem *pSys = arg;

	CHECK_NULL(pSys);

	gx_device.psystem = pSys;

	get_FirewallFlag(&pSys->firewall_flag);
	set_VideoDisplayScreen(pSys->screen);
	set_VideoDrcMode(pSys->drc_mode);

	if(pSys->autoplay)
	{
		int select;
		PlayerVideoAutoAdapt autoadapt;
		VideoOut vout = pSys->vout;

		GxAvdev_CloseLogo();

		autoadapt.enable                 = pSys->vout.autoadapt;
		autoadapt.pal                    = pSys->vout.autopal;
		autoadapt.ntsc                   = pSys->vout.autontsc;
		gx_device.psystem->vout.autoadapt1 = pSys->vout.autoadapt1;
		set_VideoAutoAdapt(autoadapt);

		select = pSys->vout.select;
		set_VideoOutputSelect(select);

		VOUT_CONFIG_MODE(select, GX_VIDEO_OUTPUT_HDMI    , vout.mode_hdmi   );
		VOUT_CONFIG_MODE(select, GX_VIDEO_OUTPUT_YUV     , vout.mode_yuv    );
		VOUT_CONFIG_MODE(select, GX_VIDEO_OUTPUT_RCA     , vout.mode_rca    );
		VOUT_CONFIG_MODE(select, GX_VIDEO_OUTPUT_RCA1    , vout.mode_rca1   );
		VOUT_CONFIG_MODE(select, GX_VIDEO_OUTPUT_SVIDEO  , vout.mode_scart  );
		VOUT_CONFIG_MODE(select, GX_VIDEO_OUTPUT_SCART   , vout.mode_svideo );

		VOUT_CONFIG_COLORS(select, GX_VIDEO_OUTPUT_HDMI    , vout.b_hdmi  , vout.c_hdmi  , vout.s_hdmi  );
		VOUT_CONFIG_COLORS(select, GX_VIDEO_OUTPUT_YUV     , vout.b_yuv   , vout.c_yuv   , vout.s_yuv   );
		VOUT_CONFIG_COLORS(select, GX_VIDEO_OUTPUT_RCA     , vout.b_rca   , vout.c_rca   , vout.s_rca   );
		VOUT_CONFIG_COLORS(select, GX_VIDEO_OUTPUT_RCA1    , vout.b_rca1  , vout.c_rca1  , vout.s_rca1  );
		VOUT_CONFIG_COLORS(select, GX_VIDEO_OUTPUT_SVIDEO  , vout.b_scart , vout.c_scart , vout.s_scart );
		VOUT_CONFIG_COLORS(select, GX_VIDEO_OUTPUT_SCART   , vout.b_svideo, vout.c_svideo, vout.s_svideo);

		set_VideoAspect(pSys->aspect);
	}

	if(1)
	{
		int i;
		AudioOutputConfig ao_config;

		int aoutput[] = {
			AUDIO_OUTPUT_DAC_IN,
			AUDIO_OUTPUT_DAC_OUT,
			AUDIO_OUTPUT_SPDIF,
			AUDIO_OUTPUT_HDMI,
		};

		int ainput[] = {
			AUDIO_OUTPUT_PCM,
			AUDIO_OUTPUT_PCM,
			pSys->aout.spdsource,
			pSys->aout.hdmisource,
		};

		set_AudioOutputSelect(pSys->aout.interface);
		set_AudioVolumeTable(pSys->aout.volume_table);
		set_AudioVolume(pSys->aout.volume);
		set_AudioTrack(pSys->aout.track);

		for(i=0;i<sizeof(aoutput)/sizeof(int);i++)
		{
			if(pSys->aout.interface & aoutput[i])
			{
				ao_config.output_data = ainput[i];
				ao_config.output_port = aoutput[i];
				set_AudioOutputConfigPort(ao_config);
			}
		}
	}

	return GX_PLAYER_OK;

}
///////////////////////////////////////////////////////////////////

static int device_init(void)
{
	gx_device.dev   = GxAvdev_CreateDevice(0);
	gx_device.vo    = GxAvdev_OpenModule(gx_device.dev, GXAV_MOD_VIDEO_OUT, 0);
	gx_device.ao    = GxAvdev_OpenModule(gx_device.dev, GXAV_MOD_AUDIO_OUT, 0);
	gx_device.vpu   = GxAvdev_OpenModule(gx_device.dev, GXAV_MOD_VPU, 0);
	gx_device.vdec  = GxAvdev_OpenModule(gx_device.dev, GXAV_MOD_VIDEO_DECODE, 0);
	gx_device.adec  = GxAvdev_OpenModule(gx_device.dev, GXAV_MOD_AUDIO_DECODE, 0);
	gx_device.stc   = GxAvdev_OpenModule(gx_device.dev, GXAV_MOD_STC, 0);
	gx_device.jpeg  = GxAvdev_OpenModule(gx_device.dev, GXAV_MOD_JPEG_DECODER, 0);
	gx_device.hdmi  = GxAvdev_OpenModule(gx_device.dev, GXAV_MOD_HDMI, 0);
	gx_device.chip  = GxChipDetect(gx_device.dev);
	
	GxAVGetDeviceCapability(gx_device.dev, &gx_device.chipcap);

	if (gx_device.chipcap.video.support_10bit) {
		int enable_10bit = 1;
		GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_SUPPORT_10BIT, (int32_t *)&enable_10bit, 1);
		GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_FB_MAX_SIZE, (int32_t *)&gx_device.max_fb_size, VIDEO_FB_SIZE);
		if (enable_10bit) {
			GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_FREEZE_BUFFER, (int32_t*)&gx_device.freeze_size, PLAY_FREEZE_FB_HD_10BIT);
		}
		else {
			GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_FREEZE_BUFFER, (int32_t*)&gx_device.freeze_size, PLAY_FREEZE_FB_HD);
		}
	}
	else {
		GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_FREEZE_BUFFER, (int32_t*)&gx_device.freeze_size, PLAY_FREEZE_FB_HD);
		GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_FB_MAX_SIZE, (int32_t*)&gx_device.max_fb_size,
				((unsigned int)((1920 * 1088 * 1.75 + 16)* 12 + 8 * 1024)));
	}

	switch(gx_device.chipcap.video.definition)
	{
	case GXAV_VIDEO_SD:
		gx_device.ess_size = SUBTITLE_ESS_SD;
		gx_device.ac3_size = AUDIO_AC3_SD;
		gx_device.eac3_size = AUDIO_DDP_SD;
		gx_device.dts_size  = AUDIO_DTS_SD;
		gx_device.aac_size  = AUDIO_AAC_SD;
		gx_device.enable_ac3 = AUDIO_ENABLE_AC3_SD;
		gx_device.enable_aac = 1;
		gx_device.indexcache = INDEX_CACHE_SIZE_SD;
		gx_device.packetcache = PLAY_CACHE_SIZE_SD;
		gx_device.recordcache = PVR_REC_CACHE_SD;
		gx_device.default_vo  = GX_VIDEO_OUTPUT_RCA;
		gx_device.default_vmode = GX_VIDEO_OUTPUT_PAL;
		gx_device.default_ao = AUDIO_OUTPUT_DAC_OUT|AUDIO_OUTPUT_SPDIF;
		gx_device.support_deinterlace = 0;
		gx_device.support_pp_zoom = 0;
		GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_ESA_SIZE,   &gx_device.esa_size, AUDIO_ESA_SD);
		GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_PCM_SIZE,   &gx_device.pcm_size, AUDIO_PCM_SD);
		GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_ESV_SIZE,   &gx_device.esv_size, VIDEO_ESV_SD);
		GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_AC3_ENABLE, &gx_device.enable_ac3, AUDIO_ENABLE_AC3_SD);
		GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_DTS_ENABLE, &gx_device.enable_dts, AUDIO_ENABLE_DTS_SD);
		GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_AAC_ENABLE, &gx_device.enable_aac, AUDIO_ENABLE_AAC_SD);
		GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_AD_ENABLE,  &gx_device.enable_adpcm, 0);
		GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_FREEZE_BUFFER, (int32_t*)&gx_device.freeze_size, PLAY_FREEZE_FB_SD);
		GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_VOL_RIGHT_NOW,  &gx_device.vol_right_now,  AUDIO_VOL_RIGHT_NOW);
		GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_VOL_STEP,       &gx_device.vol_step,       AUDIO_VOL_STEP);
		break;
	default:
		gx_device.ess_size = SUBTITLE_ESS_HD;
		gx_device.ac3_size = AUDIO_AC3_HD;
		gx_device.eac3_size = AUDIO_DDP_HD;
		gx_device.dts_size  = AUDIO_DTS_HD;
		gx_device.aac_size  = AUDIO_AAC_HD;
		gx_device.indexcache = INDEX_CACHE_SIZE_HD;
		gx_device.packetcache = PLAY_CACHE_SIZE_HD;
		gx_device.recordcache = PVR_REC_CACHE_HD;
		gx_device.default_vo = GX_VIDEO_OUTPUT_HDMI|GX_VIDEO_OUTPUT_RCA;
		gx_device.default_vmode = GX_VIDEO_OUTPUT_1080I_50HZ;
		gx_device.default_ao = AUDIO_OUTPUT_DAC_OUT|AUDIO_OUTPUT_SPDIF|AUDIO_OUTPUT_HDMI;
		GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_ESA_SIZE,   &gx_device.esa_size, AUDIO_ESA_HD);
		GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_PCM_SIZE,   &gx_device.pcm_size, AUDIO_PCM_HD);
		GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_ESV_SIZE,   &gx_device.esv_size, VIDEO_ESV_HD);
		GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_AC3_ENABLE, &gx_device.enable_ac3, AUDIO_ENABLE_AC3_HD);
		GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_AAC_ENABLE, &gx_device.enable_aac, AUDIO_ENABLE_AAC_HD);
		GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_DTS_ENABLE, &gx_device.enable_dts, AUDIO_ENABLE_DTS_HD);
		GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_AD_ENABLE,  &gx_device.enable_adpcm, 1);
		GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_SUPPORT_DEINTERLACE,  &gx_device.support_deinterlace, VIDEO_SUPPORT_DEINTERLACE);
		GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_SUPPORT_PPZOOM, &gx_device.support_pp_zoom, VIDEO_SUPPORT_PPZOOM);
		GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_VOL_RIGHT_NOW,  &gx_device.vol_right_now,   AUDIO_VOL_RIGHT_NOW);
		GxBus_ConfigGetInt(PLAYER_CONFIG_AUDIO_VOL_STEP,       &gx_device.vol_step,       AUDIO_VOL_STEP);
		break;
	}
	av_memhole_init();

	return GX_PLAYER_OK;
}

static void device_uninit(void* priv)
{
	if (gx_device.vpu)
		GxAvdev_CloseModule(gx_device.dev, gx_device.vpu);
	if (gx_device.ao)
		GxAvdev_CloseModule(gx_device.dev, gx_device.ao);
	if (gx_device.vo)
		GxAvdev_CloseModule(gx_device.dev, gx_device.vo);
	if (gx_device.vdec)
		GxAvdev_CloseModule(gx_device.dev, gx_device.vdec);
	if (gx_device.adec)
		GxAvdev_CloseModule(gx_device.dev, gx_device.adec);
	if (gx_device.stc)
		GxAvdev_CloseModule(gx_device.dev, gx_device.stc);
	if (gx_device.jpeg)
		GxAvdev_CloseModule(gx_device.dev, gx_device.jpeg);
	if (gx_device.adec1)
		GxAvdev_CloseModule(gx_device.dev, gx_device.adec1);
	GxAvdev_DestroyDevice(gx_device.dev);
	memset(&gx_device,0,sizeof(GoXceedDevice));
}


#define DEVICE_GET(type,func,arg) {\
	type* newarg = arg;\
	ret = func(newarg);\
	break;\
}

static int device_get(void* priv, int id, void* arg)
{
	int ret = GX_PLAYER_ERROR;

	if(id < 0 || id > PDEV_MAX)
		return ret;

	CHECK_NULL(arg);

	switch(id)
	{
	case PDEV_SysVersion:
		{
			DEVICE_GET(unsigned int,get_SystemVersion,arg);
		}
	case PDEV_VideoDeInterlace:
		{
			*(int*)arg = gx_device.support_deinterlace;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_VideoPPZoom:
		{
			*(int*)arg = gx_device.support_pp_zoom;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_BufSizeESV:
		{
			*(int*)arg = gx_device.esv_size;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_BufSizeESA:
		{
			*(int*)arg = gx_device.esa_size;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_BufSizeESS:
		{
			*(int*)arg = gx_device.ess_size;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_BufSizeAC3:
		{
			*(int*)arg = gx_device.ac3_size;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_BufSizeEAC3:
		{
			*(int*)arg = gx_device.eac3_size;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_BufSizeDTS:
		{
			*(int*)arg = gx_device.dts_size;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_BufSizeAAC:
		{
			*(int*)arg = gx_device.aac_size;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_BufSizePCM:
		{
			*(int*)arg = gx_device.pcm_size;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_EnableAc3:
		{
			*(int*)arg = gx_device.enable_ac3;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_EnableAac:
		{
			*(int*)arg = gx_device.enable_aac;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_EnableDts:
		{
			*(int*)arg = gx_device.enable_dts;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_EnableADPCM:
		{
			*(int*)arg = gx_device.enable_adpcm;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_IndexCacheSize:
		{
			*(int*)arg = gx_device.indexcache;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_PacketCacheSize:
		{
			*(int*)arg = gx_device.packetcache;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_RecordCacheSize:
		{
			*(int*)arg = gx_device.recordcache;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_AudioOutputDefInterface:
		{
			*(int*)arg = gx_device.default_ao;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_VideoOutputDefInterface:
		{
			*(int*)arg = gx_device.default_vo;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_VideoOutputDefConfig:
		{
			*(int*)arg = gx_device.default_vmode;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_FreezeFrameSize:
		{
			*(int*)arg = gx_device.freeze_size;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_VideoColors:
		{
			DEVICE_GET(VideoColor,get_VideoColors,arg);
		}
	case PDEV_VideoAspect:
		{
			DEVICE_GET(VideoAspect,get_VideoAspect,arg);
		}
	case PDEV_VideoDrcMode:
		{
			DEVICE_GET(VideoDrcMode,get_VideoDrcMode,arg);
		}
	case PDEV_VideoView:
		{
			DEVICE_GET(PlayerWindow,get_VideoView,arg);
		}
	case PDEV_VideoClip:
		{
			DEVICE_GET(PlayerWindow,get_VideoClip,arg);
		}
	case PDEV_VideoOutputInterface:
		{
			DEVICE_GET(int,get_VideoOutputSelect,arg);
		}
	case PDEV_VideoOutputUsableIface:
		{
			DEVICE_GET(int,get_VideoUsableIface,arg);
		}
	case PDEV_VideoOutputConfig:
		{
			DEVICE_GET(VideoOutputConfig,get_VideoOutputConfig,arg);
		}
	case PDEV_VideoDisplayScreen:
		{
			DEVICE_GET(DisplayScreen,get_VideoDisplayScreen,arg);
		}
	case PDEV_VideoAutoAdapt:
		{
			DEVICE_GET(PlayerVideoAutoAdapt,get_VideoAutoAdapt,arg);
		}

	case PDEV_AudioOutputInterface:
		{
			DEVICE_GET(int,get_AudioOutputSelect,arg);
		}
	case PDEV_AudioOutputConfigPort:
		{
			DEVICE_GET(AudioOutputConfig,get_AudioOutputConfig,arg);
		}
	case PDEV_AudioVolume:
		{
			DEVICE_GET(unsigned int,get_AudioVolume,arg);
		}
	case PDEV_AudioTrack:
		{
			DEVICE_GET(AudioTrack,get_AudioTrack,arg);
		}
	case PDEV_AudioDescriptorVaild:
		{
			DEVICE_GET(PlayerVaildAD,get_AudioDescriptorVaild,arg);
		}
	case PDEV_AudioDescriptorTrack:
		{
			DEVICE_GET(AudioDecriptorTrack,get_AudioDescriptorTrack,arg);
		}
	case PDEV_AudioBoostVolume:
		{
			DEVICE_GET(unsigned int, get_AudioBoostVolume, arg);
		}
	case PDEV_AudioDescriptorBoostVolume:
		{
			DEVICE_GET(unsigned int, get_AudioDescriptorBoostVolume,arg);
		}
	case PDEV_AudioMute:
		{
			DEVICE_GET(int,get_AudioMute,arg);
		}
	case PDEV_VideoCapure:
		{
			DEVICE_GET(PlayerSnapshot, get_VideoCapture,arg);
		}
	case PDEV_EncryptKey:
		{
			DEVICE_GET(EncryptKey, encrypt_Key, arg);
		}
	case PDEV_VideoDeinterlaceEnable:
		{
			DEVICE_GET(int, get_VideoDeinterlaceEnable, arg);
		}
	case PDEV_VideoMosaicDropGate:
		{
			DEVICE_GET(int, get_VideoMosaicDropGate, arg);
		}
	default:
		gxlogf("[PDEV] id = %d, Not Support Get !...\n",id);
		break;
	}

	if(ret < 0)
		gxlogf("[PDEV] DEVICE_GET %d ERROR!...\n",id);

	return ret;
}

#define DEVICE_SET(type,func,arg) {\
	CHECK_NULL(arg);\
	type newarg = *(type*)arg;\
	ret = func(newarg);\
	break;\
};

static int device_set(void* priv, int id, void* arg)
{
	int ret = GX_PLAYER_ERROR;

	if(id < 0 || id > PDEV_MAX)
		return ret;

	switch(id)
	{
	case PDEV_SysInit:
		{
			ret = set_sysinit(arg);
			break;
		}
	case PDEV_BufSizeESA:
		{
			gx_device.esa_size = *(int*)arg;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_BufSizeESV:
		{
			gx_device.esv_size = *(int*)arg;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_BufSizePCM:
		{
			gx_device.pcm_size = *(int*)arg;
			ret = GX_PLAYER_OK;
			break;
		}
	case PDEV_VideoColors:
		{
			DEVICE_SET(VideoColor,set_VideoColors,arg);
		}
	case PDEV_VideoAspect:
		{
			DEVICE_SET(VideoAspect,set_VideoAspect,arg);
		}
	case PDEV_VideoDrcMode:
		{
			DEVICE_SET(VideoDrcMode,set_VideoDrcMode,arg);
		}
	case PDEV_VideoView:
		{
			DEVICE_SET(PlayerWindow,set_VideoView,arg);
		}
	case PDEV_VideoClip:
		{
			DEVICE_SET(PlayerWindow,set_VideoClip,arg);
		}
	case PDEV_CvbsViewport:
		{
			DEVICE_SET(PlayerWindow, set_CvbsViewport, arg);
		}
	case PDEV_VideoOutputInterface:
		{
			DEVICE_SET(int,set_VideoOutputSelect,arg);
		}
	case PDEV_VideoOutputConfig:
		{
			DEVICE_SET(VideoOutputConfig,set_VideoOutputConfig,arg);
		}
	case PDEV_VideoOutputDef:
		{
			DEVICE_SET(VideoOutDefault, set_VideoOutputDef,arg);
		}
	case PDEV_VideoOutputPower:
		{
			DEVICE_SET(unsigned int, set_VideoOutputPower,arg);
		}
	case PDEV_VideoOutputPower2:
		{
			DEVICE_SET(VideoOutPower, set_VideoOutputPower2,arg);
		}
	case PDEV_VideoDisplayScreen:
		{
			DEVICE_SET(DisplayScreen,set_VideoDisplayScreen,arg);
		}
	case PDEV_VideoAutoAdapt:
		{
			DEVICE_SET(PlayerVideoAutoAdapt, set_VideoAutoAdapt, arg);
		}
	case PDEV_VideoAutoConfig:
		{
			DEVICE_SET(float, set_VideoAutoConfig, arg);
		}
	case PDEV_VideoHide:
		{
			DEVICE_SET(PlayerLayer,set_VideoHide,arg);
		}
	case PDEV_VideoShow:
		{
			DEVICE_SET(PlayerVideoShow,set_VideoShow,arg);
		}
	case PDEV_Background:
		{
			DEVICE_SET(PlayerLayer, set_Background, arg);
		}
	case PDEV_StreamWidth:
		{
			DEVICE_SET(int, set_StreamWidth, arg);
		}
	case PDEV_StreamHeight:
		{
			DEVICE_SET(int, set_StreamHeight, arg);
		}
	case PDEV_StreamStride:
		{
			DEVICE_SET(int, set_StreamStride, arg);
		}
	case PDEV_AudioOutputInterface:
		{
			DEVICE_SET(int, set_AudioOutputSelect,arg);
		}
	case PDEV_AudioOutputConfigPort:
		{
			DEVICE_SET(AudioOutputConfig,set_AudioOutputConfigPort,arg);
		}
	case PDEV_AudioVolume:
		{
			DEVICE_SET(unsigned int,set_AudioVolume,arg);
		}
	case PDEV_AudioTrack:
		{
			DEVICE_SET(AudioTrack,set_AudioTrack,arg);
		}
	case PDEV_AudioDescriptorTrack:
		{
			DEVICE_SET(AudioDecriptorTrack,set_AudioDescriptorTrack,arg);
		}
	case PDEV_AudioBoostVolume:
		{
			DEVICE_SET(unsigned int,set_AudioBoostVolume,arg);
		}
	case PDEV_AudioDescriptorBoostVolume:
		{
			DEVICE_SET(unsigned int,set_AudioDescriptorBoostVolume,arg);
		}
	case PDEV_AudioMute:
		{
			DEVICE_SET(int,set_AudioMute,arg);
		}
	case PDEV_AudioPowerMute:
		{
			DEVICE_SET(int,set_AudioPowerMute,arg);
		}
	case PDEV_AudioSetPort:
		{
			DEVICE_SET(AudioOutPortSet,set_AudioOutPort,arg);
		}
	case PDEV_AudioTurnPort:
		{
			DEVICE_SET(AudioOutPortTurn,set_AudioOutPortTurn,arg);
		}
	case PDEV_AudioSetDACVolume:
		{
			DEVICE_SET(int,set_AudioDACVolume,arg);
		}
	case PDEV_AudioVolumeFactor:
		{
			DEVICE_SET(unsigned int,set_AudioVolumeFactor,arg);
		}
	case PDEV_FreezeFrameStart:
		{
			ret = freeze_frame(0);
			break;
		}
	case PDEV_FreezeFrameStop:
		{
			ret = freeze_frame(1);
			break;
		}
	case PDEV_VideoDeinterlaceEnable:
		{
			DEVICE_SET(int, set_VideoDeinterlaceEnable, arg);
		}
	case PDEV_VideoMosaicDropGate:
		{
			DEVICE_SET(int, set_VideoMosaicDropGate, arg);
		}
	case PDEV_VideoPlayBypass:
		{
			DEVICE_SET(int, set_VideoPlayBypass, arg);
		}
	case PDEV_VideoOutScartMode:
		{
			DEVICE_SET(VideoOutputScartMode, set_VideoOutputScartMode, arg);
		}
	case PDEV_AudioDebugAdecConfig:
		{
			DEVICE_SET(int, set_DebugAdecConfig, arg);
		}
	case PDEV_AudioDebugAoutConfig:
		{
			DEVICE_SET(int, set_DebugAoutConfig, arg);
		}
	case PDEV_AudioDebugAdecContent:
		{
			DEVICE_SET(int, set_DebugAdecContent, arg);
		}
	case PDEV_AudioDebugAoutContent:
		{
			DEVICE_SET(int, set_DebugAoutContent, arg);
		}
	case PDEV_AudioDolbyCertify:
		{
			DEVICE_SET(int, set_AudioDolbyCertify, arg);
		}
	case PDEV_AudioSpeedMode:
		{
			DEVICE_SET(AudioOutputSpeedMode, set_AudioSpeedMode, arg);
		}
	default:
		gxlogf("[PDEV] id = %d, Not Support Set !...\n",id);
		break;
	}

	if(ret < 0)
		gxlogf("[PDEV] DEVICE_SET %d ERROR!...\n",id);

	return ret;
}

GxDevice gx_device_goxceed = {

	.priv = &gx_device,

	.init   = device_init,
	.uninit = device_uninit,
	.set    = device_set,
	.get    = device_get,
};

#endif
