#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"

/*
TV_STANDARD_KEY  --  VEDIO_OUT_KEY
TV_RATIO_KEY
ASPECT_MODE_KEY
SOUND_MODE_KEY
SPDIF_OUTPUT_KEY
//TV_SCART


PLAYER_CONFIG_VIDEO_RESOLUTION
PLAYER_CONFIG_VIDEO_DISPLAY_SCREEN
PLAYER_CONFIG_VIDEO_ASPECT
PLAYER_CONFIG_AUDIO_TRACK
PLAYER_CONFIG_AUDIO_SPDIF_SOURCE
*/
#define AUTO_CHANGE_SUPPORT
#define SUPPORT_1080P_OUTPUT
#define SUPPORT_480_I_P_OUTPUT
#ifdef SUPPORT_SCART
#define SUPPORT_TV_SCART
#else
#undef SUPPORT_TV_SCART
#endif

enum ITEM_AV_OUT
{
	ITEM_TV_STANDARD = 0,
	ITEM_TV_RATIO,
	ITEM_ASPECT_MODE,
    ITEM_VIDEO_OUTPUT,
	ITEM_SOUND_MODE,
	ITEM_SPDIF,
#ifdef SUPPORT_TV_SCART
	ITEM_TV_SCART,
#endif
#if AUDIO_DESCRIPTOR_SUPPORT
	ITEM_AD_MODE,
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
	ITEM_AD_VOL_OFFSET,
#endif
#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT
	ITEM_VIDEO_HDR,
#endif
	ITEM_AVOUT_TOTAL
};


typedef enum
{
    AV_STANDARD_AUTO,
#ifdef SUPPORT_480_I_P_OUTPUT
    AV_STANDARD_480I,
    AV_STANDARD_480P,
#endif
    AV_STANDARD_576I,
    AV_STANDARD_576P,
    AV_STANDARD_720P_50HZ,
    AV_STANDARD_720P_60HZ,
    AV_STANDARD_1080I_50HZ,
    AV_STANDARD_1080I_60HZ,
#ifdef SUPPORT_1080P_OUTPUT
    AV_STANDARD_1080P_50HZ,
    AV_STANDARD_1080P_60HZ,
#endif
    AV_STANDARD_TOTAL,
}AvTvStandard;

typedef enum
{
    AV_RATIO_4_3,
    AV_RATIO_16_9,
    AV_RATIO_TOTAL
}AvTvRatio;

typedef enum
{
#if CVBS_TEST_SUPPORT
	AV_ASPECT_AUTO,
#else
	AV_ASPECT_AUTO,
	AV_ASPECT_LETTER_BOX,
	AV_ASPECT_PAN_SCAN,
#endif
    AV_ASPECT_TOTAL
}AvAspectMode;

typedef enum
{
    AV_VOLUME_GLOBAL,
    AV_VOLUME_CHANNEL,
#if SMART_VOLUME_SUPPORT
    AV_VOLUME_SMART,
#endif
   /* AV_SOUND_STEREO,
    AV_SOUND_LEFT,
    AV_SOUND_RIGHT,
    AV_SOUND_MONO,*/
    AV_SOUND_TOTAL
}AvSoundMode;

typedef enum
{
    AV_VIDEO_AUTO,
    AV_VIDEO_HDMI,
    AV_VIDEO_TOTAL
}AvVideoMode;


typedef enum
{
    AV_SPDIF_DECODE,//AC3 decord
    AV_SPDIF_BYPASS,//AV3 BYPASS
    AV_SPDIF_AUTO,
    AV_SPDIF_TOTAL,
}AvSpdif;

static char* spdif[AV_SPDIF_TOTAL] = {
     "AC3 Decode,","AC3 Bypass,","AC3 Auto"
};

#ifdef SUPPORT_TV_SCART
typedef enum
{
    AV_SCART_CVBS,
    AV_SCART_RGB,
    AV_SCART_TOTAL
}AvScart;

static char* scart[AV_SCART_TOTAL] = {
    "CVBS,","RGB,"
};

#endif

static char* item_avout[ITEM_AVOUT_TOTAL] =
{
    STR_ID_TV_STANDARD,
    STR_ID_TV_RATIO,
    STR_ID_ASPECT_MODE,
    STR_ID_VIDEO_OUTPUT,
    STR_ID_VOLUME_SCOPE,// "Sound Mode",
    "SPDIF/HDMI",
    #ifdef SUPPORT_TV_SCART
    STR_ID_SCART,
    #endif
#if AUDIO_DESCRIPTOR_SUPPORT
    STR_ID_AUDIO_DESCRIPTION,
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
    STR_ID_AD_VOL_OFFSET,
#endif
#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT
    STR_ID_HDR_MODE,
#endif
};

static char* tv_ratio[AV_RATIO_TOTAL] = {
   "4:3,","16:9,"
};
static char* aspect_mode[AV_ASPECT_TOTAL] = {
#if CVBS_TEST_SUPPORT
    "Raw",
#else
   "Auto,","Letter Box,","Pan & Scan"
#endif
};
static char* tv_standard[AV_STANDARD_TOTAL] = {
	"Auto,",
#ifdef SUPPORT_480_I_P_OUTPUT
	"480i,","480p,",
#endif
    "576i,","576p,","720p@50Hz,","720p@60Hz,","1080i@50Hz,","1080i@60Hz,",
#ifdef SUPPORT_1080P_OUTPUT
    "1080p@50Hz,","1080p@60Hz,",
#endif
};
static char* sound_mode[AV_SOUND_TOTAL] = {
    "Global,","Channel,",//"Stereo,","Left,","Right,","Mono",
#if SMART_VOLUME_SUPPORT
    "Smart",
#endif
};

static char* video_out_mode[AV_VIDEO_TOTAL] = {
    "HDMI and CVBS,","HDMI or CVBS",
};

#if AUDIO_DESCRIPTOR_SUPPORT
typedef enum
{
	AUDIO_DESCRIPTOR_OFF,
	AUDIO_DESCRIPTOR_ON,
	AUDIO_DESCRIPTOR_TOTAL
}AudioDescriptor;

static char* ad_status[AUDIO_DESCRIPTOR_TOTAL] = {
    "Off,","On,"
};
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
typedef enum
{
    AD_VOL_OFFSET_0 = 0,    /* -3 */
    AD_VOL_OFFSET_1 ,         /* -2 */
    AD_VOL_OFFSET_2 ,         /* -1 */
    AD_VOL_OFFSET_3 ,         /*  0  */
    AD_VOL_OFFSET_4 ,         /*  1 */
    AD_VOL_OFFSET_5 ,         /*  2 */
    AD_VOL_OFFSET_6 ,         /*  3 */
    AD_VOL_OFFSET_MAX,
}AdVolOffset;

static char* sg_ad_vol_offset [AD_VOL_OFFSET_MAX] = {
    "-3,","-2,","-1,","0,","1,","2,","3",
};

#endif
#endif

#if HIGH_DYNAMIC_RANGE_SUPPORT
//If the driver is updated, this content also needs to be updated. kk Label:expand
typedef enum
{
    APP_VIDEO_HDR_MODE_HDR,
    APP_VIDEO_HDR_MODE_SDR,
    APP_VIDEO_HDR_MODE_TOTAL
}AvVideoHdrMode;

static char *hdr_mode[APP_VIDEO_HDR_MODE_TOTAL] = {"Enhanced,","Off"};

static AvVideoHdrMode _hdr_player_para_convert_app(VideoDrcMode mode)
{
    AvVideoHdrMode sel = APP_VIDEO_HDR_MODE_HDR;

    if(GX_VDRC_BYPASS == mode)
        sel = APP_VIDEO_HDR_MODE_SDR;
    else if(GX_VDRC_TOSDR == mode)
        sel = APP_VIDEO_HDR_MODE_HDR;
    else if(GX_VDRC_AUTO == mode)
        app_log_error("App not support yet\n");

    return sel;
}

static VideoDrcMode _hdr_app_para_convert_player(AvVideoHdrMode sel)
{
    VideoDrcMode mode = GX_VDRC_TOSDR;

    if(APP_VIDEO_HDR_MODE_SDR == sel)
        mode = GX_VDRC_BYPASS;
    else if(APP_VIDEO_HDR_MODE_HDR == sel)
        mode = GX_VDRC_TOSDR;
    else
        app_log_error("Param error\n");

    return mode;
}

#endif

typedef enum
{
	AV_ADAPT_NOAUTO,
	AV_ADAPT_AUTO
}AvAutoAdapt;

typedef enum
{
	AV_VOLUME_SCOPE_GLOBAL,
	AV_VOLEME_SCOPE_CHANNEL,
#if SMART_VOLUME_SUPPORT
	AV_VOLEME_SCOPE_SMART
#endif
}AvVolumeScope;

typedef struct
{
	VideoOutputMode         vout_mode;// HDMI 1080  YPBPR 1080...output config. standard
	DisplayScreen           ratio;  // DisplayScreen . aspect
	VideoAspect             aspect;  // 4:3 letterbox ...
	AvAutoAdapt          display_adapt;
	AvVolumeScope	volume_scope;
	AudioTrack              audio_track;
#ifdef SUPPORT_TV_SCART
    int		scart_source;// 0:CVBS; 1: RGB
    int     scart_screen;// 0:4:3;  1:16:9
#endif
	AvSpdif         spdif;             //SPDIF_OUTPUT_KEY
	VideoOutputInterface    vout_interface;// RCA, YUV, HDMI ...output config.UI needn't input
	#if AUDIO_DESCRIPTOR_SUPPORT
	AudioDescriptor ad_status;
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
    int ad_vol_offset;
#endif
	#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT
    AvVideoHdrMode video_hdr_mode_sel;
#endif
    AvVideoMode video_out_mode;
}AvPlayerPara;

typedef struct
{
    //AvAutoAdapt     adapt;
    AvTvStandard    tv_standard;       //TV_STANDARD_KEY  --  VEDIO_OUT_KEY
    AvTvRatio       tv_ratio;          //TV_RATIO_KEY
    AvAspectMode    aspect_mode;       //ASPECT_MODE_KEY
    AvSoundMode     volume_mode;        //SOUND_MODE_KEY  sound_mode
    AvSpdif         spdif;             //SPDIF_OUTPUT_KEY
    #ifdef SUPPORT_TV_SCART
    AvScart         tv_scart;
	#endif
	#if AUDIO_DESCRIPTOR_SUPPORT
	AudioDescriptor ad_mode;
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
    int ad_vol_offset;
#endif
	#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT
    AvVideoHdrMode hdr_mode;
#endif
    AvVideoMode video_out_mode;
}AvAppPara;


static uint32_t s_av_item_map[ITEM_AVOUT_TOTAL];
static SystemSettingOpt s_av_setting_opt;
static SystemSettingItem s_av_item[ITEM_AVOUT_TOTAL];
static AvPlayerPara s_AvPlayerPara;
static AvAppPara s_AvAppPara;
static VideoOutputMode s_vout_mode;
#ifdef SUPPORT_TV_SCART
static int32_t s_scart_enable = 0;
#endif
static int32_t s_spdif_enable = 0;
static bool s_av_menu_flag = false;
static bool s_av_power_flag = false;
static bool isTVModeChange = false;
static  PopList pop_list;

static void av_player_to_app_para(AvPlayerPara *player, AvAppPara *app)
{
    if (player == NULL || app == NULL)
        return;
    if (player->display_adapt != AV_ADAPT_AUTO)
    {
        // standard
        switch (player->vout_mode)
        {

#ifdef SUPPORT_480_I_P_OUTPUT
            case GX_VIDEO_OUTPUT_480I:
                app->tv_standard = AV_STANDARD_480I;
                break;
            case GX_VIDEO_OUTPUT_480P:
                app->tv_standard = AV_STANDARD_480P;
                break;
#endif
            case GX_VIDEO_OUTPUT_576I:
                app->tv_standard = AV_STANDARD_576I;
                break;
            case GX_VIDEO_OUTPUT_576P:
                app->tv_standard = AV_STANDARD_576P;
                break;
            case GX_VIDEO_OUTPUT_720P_50HZ:
                app->tv_standard = AV_STANDARD_720P_50HZ;
                break;
			case GX_VIDEO_OUTPUT_720P_60HZ:
                app->tv_standard = AV_STANDARD_720P_60HZ;
                break;
            case GX_VIDEO_OUTPUT_1080I_50HZ:
                app->tv_standard = AV_STANDARD_1080I_50HZ;
                break;
			case GX_VIDEO_OUTPUT_1080I_60HZ:
                app->tv_standard = AV_STANDARD_1080I_60HZ;
                break;
#ifdef SUPPORT_1080P_OUTPUT
            case GX_VIDEO_OUTPUT_1080P_50HZ:
                app->tv_standard = AV_STANDARD_1080P_50HZ;
                break;
			case GX_VIDEO_OUTPUT_1080P_60HZ:
                app->tv_standard = AV_STANDARD_1080P_60HZ;
                break;
#endif
            default:
                break;
        }
    }
    else
    {
        // standard - adapt
        app->tv_standard = AV_STANDARD_AUTO;
    }
    // ratio
    switch (player->ratio.aspect)
    {
        case DISPLAY_SCREEN_4X3:
            app->tv_ratio = AV_RATIO_4_3;
            break;
        case DISPLAY_SCREEN_16X9:
            app->tv_ratio = AV_RATIO_16_9;
            break;
        default:
            break;
    }

    // aspect
    switch (player->aspect)
    {
#if CVBS_TEST_SUPPORT
        case ASPECT_NORMAL:
            app->aspect_mode = AV_ASPECT_AUTO;
            break;
#else
        case ASPECT_NORMAL:
            app->aspect_mode = AV_ASPECT_AUTO;
            break;
        case ASPECT_LETTER_BOX:
            app->aspect_mode = AV_ASPECT_LETTER_BOX;
            break;
        case ASPECT_PAN_SCAN:
            app->aspect_mode = AV_ASPECT_PAN_SCAN;
            break;
#endif
        default:
            break;
    }

#if 0
    // sound mode
    switch (player->audio_track)
    {
        case AUDIO_TRACK_STEREO:
            app->sound_mode = AV_SOUND_STEREO;
            break;
        case AUDIO_TRACK_LEFT:
            app->sound_mode = AV_SOUND_LEFT;
            break;
        case AUDIO_TRACK_RIGHT:
            app->sound_mode = AV_SOUND_RIGHT;
            break;
        case AUDIO_TRACK_MONO:
            app->sound_mode = AV_SOUND_MONO;
            break;
        default:
            break;
    }
   #else
            app->volume_mode = player->volume_scope;
    #endif

// scart output
#ifdef SUPPORT_TV_SCART
    if(s_scart_enable == 1)
    {
        switch(player->scart_source)
        {
            case AV_SCART_CVBS:
                app->tv_scart = AV_SCART_CVBS;
                break;

            case AV_SCART_RGB:
                app->tv_scart = AV_SCART_RGB;
                break;

            default:
                break;
        }
    }
#endif

#if AUDIO_DESCRIPTOR_SUPPORT
    int ad_flag = 0;
    GxPlayer_GetAudioDescriptorVaild(&ad_flag);
    if(ad_flag == 1)
    {
        app->ad_mode = player->ad_status ;
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
        app->ad_vol_offset = player->ad_vol_offset ;
#endif
    }
#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT
    app->hdr_mode = player->video_hdr_mode_sel;
#endif
//SPDIF output
    if(s_spdif_enable == 1)
    {
        switch(player->spdif)
        {
            case AV_SPDIF_DECODE:
                app->spdif = AV_SPDIF_DECODE;
                break;
            case AV_SPDIF_BYPASS:
                app->spdif = AV_SPDIF_BYPASS;
                break;
            case AV_SPDIF_AUTO:
                app->spdif = AV_SPDIF_AUTO;
                break;

            default:
                break;

        }
    }
    app->video_out_mode = player->video_out_mode;

}

static void app_av_app_to_player_para(AvAppPara *app, AvPlayerPara *player)
{
    if (player == NULL || app == NULL)  return;

    uint32_t standard[AV_STANDARD_TOTAL] = {
#ifdef SUPPORT_480_I_P_OUTPUT
            GX_VIDEO_OUTPUT_480I,
            GX_VIDEO_OUTPUT_480P,
#endif
            GX_VIDEO_OUTPUT_576I,
            GX_VIDEO_OUTPUT_576P,
            GX_VIDEO_OUTPUT_720P_50HZ,
            GX_VIDEO_OUTPUT_720P_60HZ,
            GX_VIDEO_OUTPUT_1080I_50HZ,
            GX_VIDEO_OUTPUT_1080I_60HZ,
#ifdef SUPPORT_1080P_OUTPUT
            GX_VIDEO_OUTPUT_1080P_50HZ,
            GX_VIDEO_OUTPUT_1080P_60HZ,
#endif
    };
    uint32_t ratio[AV_RATIO_TOTAL] = {DISPLAY_SCREEN_4X3, DISPLAY_SCREEN_16X9};
    uint32_t aspect[AV_ASPECT_TOTAL] = {
#if CVBS_TEST_SUPPORT
        ASPECT_RAW_SIZE,
#else
        ASPECT_NORMAL, ASPECT_LETTER_BOX, ASPECT_PAN_SCAN
#endif
    };
    uint32_t sound[AV_SOUND_TOTAL] =  {AV_VOLUME_SCOPE_GLOBAL,AV_VOLEME_SCOPE_CHANNEL,
#if SMART_VOLUME_SUPPORT
        AV_VOLUME_SMART
#endif
    };//{AUDIO_TRACK_STEREO, AUDIO_TRACK_LEFT, AUDIO_TRACK_RIGHT, AUDIO_TRACK_MONO};
    #ifdef SUPPORT_TV_SCART
    uint32_t scart[AV_SCART_TOTAL] = {AV_SCART_CVBS, AV_SCART_RGB};
	#endif
	#if AUDIO_DESCRIPTOR_SUPPORT
	//uint32_t ad_status[AUDIO_DESCRIPTOR_TOTAL] = {AUDIO_DESCRIPTOR_OFF, AUDIO_DESCRIPTOR_ON};
	#endif
    uint32_t spdif[AV_SPDIF_TOTAL] = {AV_SPDIF_DECODE,AV_SPDIF_BYPASS,AV_SPDIF_AUTO};
    // standard
    if (app->tv_standard > AV_STANDARD_AUTO)
    {
        player->vout_mode = standard[app->tv_standard - 1];
        player->display_adapt = AV_ADAPT_NOAUTO;
    }
    else
    {
    // modify by zhouzhr 20120317
    	player->display_adapt = AV_ADAPT_AUTO;
#if 0
        // TODO: ×Ô¶¯Ä£Ê½ÈçºÎÉèÖÃÊÓÆµÊä³ö
        player->vout_mode = standard[AV_STANDARD_1080I_50HZ];
#endif
    }
    // ratio
    player->ratio.aspect = ratio[app->tv_ratio];
#ifdef SUPPORT_TV_SCART
    if(s_scart_enable == 1)// 0:4:3; 1: 16:9
    {
        player->scart_screen = ratio[app->tv_ratio];
        player->scart_source = scart[app->tv_scart];
    }
#endif

    if (s_spdif_enable == 1)
        player->spdif = spdif[app->spdif];

    player->ratio.xres = APP_THEME_XRES;
    player->ratio.yres = APP_THEME_YRES;

    // aspect
    player->aspect = aspect[app->aspect_mode];

    // sound mode
  //  player->audio_track = sound[app->volume_mode];
    player->volume_scope= sound[app->volume_mode];

#if AUDIO_DESCRIPTOR_SUPPORT
    int ad_flag = 0;
    GxPlayer_GetAudioDescriptorVaild(&ad_flag);
    if(ad_flag == 1)
    {
        player->ad_status = app->ad_mode;
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
        player->ad_vol_offset = app->ad_vol_offset;
#endif
    }
#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT
    player->video_hdr_mode_sel = app->hdr_mode;
#endif
    player->video_out_mode = app->video_out_mode;
}

static void app_av_player_para_get(AvPlayerPara *player)
{
    int init_value = 0;

    //auto adapt
    GxBus_ConfigGetInt(TV_ADAPT_KEY, &init_value, TV_ADAPT_NOAUTO);
    player->display_adapt = init_value;

   // tv standard
    GxBus_ConfigGetInt(TV_STANDARD_KEY, &init_value, TV_STANDARD);
	s_vout_mode = init_value;
    player->vout_mode = init_value;

    // ratio
    GxBus_ConfigGetInt(TV_RATIO_KEY, &init_value, TV_RATIO);
    player->ratio.aspect = init_value;

    // ascept
    GxBus_ConfigGetInt(ASPECT_MODE_KEY, &init_value, ASPECT_MODE);
    player->aspect = init_value;

    // sound mode
    GxBus_ConfigGetInt(SOUND_MODE_KEY, &init_value, SOUND_MODE);
    player->audio_track = init_value;

   GxBus_ConfigGetInt(VOLUME_MODE_KEY, &init_value, VOLUME_SCOMPE_);
    player->volume_scope= init_value;

#ifdef SUPPORT_TV_SCART
    if(s_scart_enable == 1)
    {
        /*SCART OUTPUT*/
        GxBus_ConfigGetInt(TV_SCART_KEY, &init_value, TV_SCART);
        player->scart_source = init_value;//  0:CVBS ,1:RGB
        player->scart_screen = player->aspect;// 0: 4:3; 1: 16:9
    }
#endif

    if(s_spdif_enable == 1)
    {
        /*SPDIF OUTPUT*/
        GxBus_ConfigGetInt(TV_SPDIF_KEY, &init_value, AC3_BYPASS);
        if (init_value == DECODE_MODE)
            player->spdif = AV_SPDIF_DECODE;
        else if(init_value == BYPASS_MODE)
            player->spdif = AV_SPDIF_BYPASS;
        else if(init_value == (DECODE_MODE|BYPASS_MODE))
            player->spdif = AV_SPDIF_AUTO;
    }
#if AUDIO_DESCRIPTOR_SUPPORT
    int ad_flag = 0;
    GxPlayer_GetAudioDescriptorVaild(&ad_flag);
    if(ad_flag == 1)
    {
        GxBus_ConfigGetInt(AD_CONF_KEY, &init_value, AD_CONFIG);
        player->ad_status= init_value;
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
        GxBus_ConfigGetInt(AD_VOL_OFFSET_KEY, &init_value, AD_VOL_OFFSET_VALUE);
        player->ad_vol_offset= init_value+3;
#endif
    }
#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT
    GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_DRCMODE, &init_value, _hdr_app_para_convert_player(APP_VIDEO_HDR_MODE_HDR));
    player->video_hdr_mode_sel = _hdr_player_para_convert_app(init_value);
#endif
    init_value = 0;
    GxBus_ConfigGetInt(AV_VIDEO_OUT_MODE, &init_value, VIDEO_OUT_MODE_DEFAULT_VALUE);
    player->video_out_mode = init_value;
}

static void app_av_app_para_get(AvAppPara *app)
{
    app->tv_standard   = s_av_item[s_av_item_map[ITEM_TV_STANDARD]].itemProperty.itemPropertyCmb.sel;
    app->tv_ratio      = s_av_item[s_av_item_map[ITEM_TV_RATIO]].itemProperty.itemPropertyCmb.sel;
    app->aspect_mode   = s_av_item[s_av_item_map[ITEM_ASPECT_MODE]].itemProperty.itemPropertyCmb.sel;
    app->volume_mode    = s_av_item[s_av_item_map[ITEM_SOUND_MODE]].itemProperty.itemPropertyCmb.sel;
#ifdef SUPPORT_TV_SCART
	if(s_scart_enable == 1)
    {
        app->tv_scart = s_av_item[s_av_item_map[ITEM_TV_SCART]].itemProperty.itemPropertyCmb.sel;
    }
    else
    {
    	  app->tv_scart = s_AvAppPara.tv_scart;
    }
#endif

#if AUDIO_DESCRIPTOR_SUPPORT
    int ad_flag = 0;
    GxPlayer_GetAudioDescriptorVaild(&ad_flag);
    if(ad_flag == 1)
    {
        app->ad_mode= s_av_item[s_av_item_map[ITEM_AD_MODE]].itemProperty.itemPropertyCmb.sel;
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
        app->ad_vol_offset= s_av_item[s_av_item_map[ITEM_AD_VOL_OFFSET]].itemProperty.itemPropertyCmb.sel;
#endif
    }
    else
    {
         app->ad_mode = s_AvAppPara.ad_mode;
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
         app->ad_vol_offset= s_AvAppPara.ad_vol_offset;
#endif
    }
#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT
    app->hdr_mode = s_av_item[s_av_item_map[ITEM_VIDEO_HDR]].itemProperty.itemPropertyCmb.sel;
#endif

    if(s_spdif_enable == 1)
        app->spdif = s_av_item[s_av_item_map[ITEM_SPDIF]].itemProperty.itemPropertyCmb.sel;
    else
        app->spdif = s_AvAppPara.spdif;
    app->video_out_mode = s_av_item[s_av_item_map[ITEM_VIDEO_OUTPUT]].itemProperty.itemPropertyCmb.sel;
}

static bool app_av_setting_para_change(AvAppPara *app, AvAppPara *bak)
{
	bool ret = FALSE;

    if (0 != memcmp(app, bak, sizeof(AvAppPara)))
    {
        ret = TRUE;
    }
	if(s_vout_mode != s_AvPlayerPara.vout_mode)
	{
		ret = TRUE;
	}

	return ret;
}


//add for HD
VideoOutputMode app_av_get_cvbs_standard(VideoOutputMode vout_mode)
{
	VideoOutputMode OutMode = GX_VIDEO_OUTPUT_MODE_MAX;
    switch (vout_mode)
    {
#ifdef SUPPORT_480_I_P_OUTPUT
		case GX_VIDEO_OUTPUT_480I:
		case GX_VIDEO_OUTPUT_480P:
#endif
		case GX_VIDEO_OUTPUT_720P_60HZ:
		case GX_VIDEO_OUTPUT_1080I_60HZ:
#ifdef SUPPORT_1080P_OUTPUT
		case GX_VIDEO_OUTPUT_1080P_60HZ:
#endif
				OutMode = NTSC_TYPE;//VIDEO_OUTPUT_PAL_M,VIDEO_OUTPUT_NTSC_443
				break;
		case GX_VIDEO_OUTPUT_576I:
		case GX_VIDEO_OUTPUT_576P:
		case GX_VIDEO_OUTPUT_720P_50HZ:
		case GX_VIDEO_OUTPUT_1080P_50HZ:
#ifdef SUPPORT_1080P_OUTPUT
		case GX_VIDEO_OUTPUT_1080I_50HZ:
#endif
			OutMode = GX_VIDEO_OUTPUT_PAL; //GX_VIDEO_OUTPUT_PAL_N, GX_VIDEO_OUTPUT_PAL_NC
                break;
            default:
				app_log_error("\n---wrong format for RCA1----\n");
                break;
    }
	return OutMode;
}

static int app_av_set_standard(AvPlayerPara *player)
{
    GxMsgProperty_PlayerVideoModeConfig video_mode;
    GxMsgProperty_PlayerVideoAutoAdapt display_adapt;
    int init_value = 0;
    // adapt
    display_adapt.enable = (int)player->display_adapt;
    display_adapt.pal = GX_VIDEO_OUTPUT_PAL;
    display_adapt.ntsc = NTSC_TYPE;
    app_send_msg_exec(GXMSG_PLAYER_VIDEO_AUTO_ADAPT, &display_adapt);

    //standard
    if(s_av_power_flag == true)
    {
        s_av_power_flag=false;
        GxBus_ConfigSetInt(TV_STANDARD_KEY, player->vout_mode);
    }
    if (player->display_adapt != AV_ADAPT_AUTO)
    {
        //HDMI - standard
        video_mode.interface = GX_VIDEO_OUTPUT_HDMI;
        video_mode.mode = player->vout_mode;
        app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG,&video_mode);

#if 0 //def SUPPORT_TV_SCART
        if (s_scart_enable == 1)
        {
        //SCART - standard
            video_mode.interface = GX_VIDEO_OUTPUT_SCART;// if use this configure, yuv is not support
            video_mode.config.scart.screen = player->scart_screen;
            video_mode.config.scart.source = player->scart_source;
        }
        else
#endif
        {
        //RCA - standard
            video_mode.interface = GX_VIDEO_OUTPUT_RCA;
        }
        video_mode.mode = app_av_get_cvbs_standard(player->vout_mode);
        if(GX_VIDEO_OUTPUT_MODE_MAX != video_mode.mode)
            app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG,&video_mode);
    }
    else //auto
    {
        //HDMI - standard
        video_mode.interface = GX_VIDEO_OUTPUT_HDMI;
        video_mode.mode = player->vout_mode;
        app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG,&video_mode);

#if 0//def SUPPORT_TV_SCART
        if (s_scart_enable == 1)
        {
        //SCART - standard
            video_mode.interface = GX_VIDEO_OUTPUT_SCART;
            video_mode.config.scart.screen = player->scart_screen;
            video_mode.config.scart.source = player->scart_source;
        }
#endif
    }
#if CVBS_TEST_SUPPORT
    {
        extern void app_cvbs_index_test_para_init(void);
        if((GX_VIDEO_OUTPUT_576I == player->vout_mode)
#ifdef SUPPORT_480_I_P_OUTPUT
                ||(GX_VIDEO_OUTPUT_480I == player->vout_mode)
#endif
          )
        {
            app_cvbs_index_test_para_init();
        }
    }
#endif
    GxBus_ConfigGetInt(AV_VIDEO_OUT_MODE, &init_value, VIDEO_OUT_MODE_DEFAULT_VALUE);
    app_av_set_videoout_mode(init_value);
    return 0;
}

static int app_av_set_ratio(AvPlayerPara *player)
{
//    GxMsgProperty_PlayerDisplayScreen screen_ratio;

 //   screen_ratio.aspect = player->ratio.aspect;
//    screen_ratio.xres = player->ratio.xres;
//    screen_ratio.yres = player->ratio.yres;

    GxBus_ConfigSetInt(TV_RATIO_KEY,player->ratio.aspect);
//    app_send_msg_exec(GXMSG_PLAYER_DISPLAY_SCREEN, &screen_ratio);

    return 0;
}

static int app_av_set_aspect(AvPlayerPara *player)
{
//    GxMsgProperty_PlayerVideoAspect video_aspect = player->aspect;

    GxBus_ConfigSetInt(ASPECT_MODE_KEY,player->aspect);
//    app_send_msg_exec(GXMSG_PLAYER_VIDEO_ASPECT, &video_aspect);

    return 0;
}

void app_av_setting_init(void)
{
	int  init_value = 0;
    GxBus_ConfigGetInt(TV_RATIO_KEY, &init_value, TV_RATIO);
	GxMsgProperty_PlayerDisplayScreen screen_ratio;
	screen_ratio.aspect = init_value;
	screen_ratio.xres = APP_THEME_XRES;
	screen_ratio.yres = APP_THEME_YRES;
	app_send_msg_exec(GXMSG_PLAYER_DISPLAY_SCREEN, &screen_ratio);
    GxBus_ConfigGetInt(ASPECT_MODE_KEY, &init_value, ASPECT_MODE);
	GxMsgProperty_PlayerVideoAspect video_aspect = init_value;
	app_send_msg_exec(GXMSG_PLAYER_VIDEO_ASPECT, &video_aspect);
}
#if SMART_VOLUME_SUPPORT
void app_smart_volume_support(int32_t volume_scope)
{
    AudioSmartConfig config;
    int gain_timems = 0;
    float loudness = -16;
    if(volume_scope == AV_VOLEME_SCOPE_SMART)
    {
        //GxPlayer_GetAudioSmartVolumeLoudness(&loudness);
        gain_timems = SMART_GAIN_TIME;
        gain_timems = gain_timems*5000;
        config.target_timems = gain_timems;
        config.target_loudness = loudness;
        //app_log_info("enable line:%d,>>>gain level:%d,time:%d ms\n",__LINE__,config.gain_level,config.gain_timems);
        GxPlayer_SetAudioSmartVolume(1,&config);
    }
    else
    {
        config.target_timems = 0;
        config.target_loudness = 0;
        //app_log_info("disable line:%d,>>>gain level:%d,time:%d ms\n",__LINE__,config.gain_level,config.gain_timems);
        GxPlayer_SetAudioSmartVolume(0,&config);
    }
}

void app_smart_volume_save_extinfo(int32_t prog_pos)
{
    int ret = 0;
    float loudness = 0;
    AppProgExtInfo prog_ext_info;
    GxMsgProperty_NodeByPosGet node = {0};

    memset(&prog_ext_info, 0, sizeof(AppProgExtInfo));
    node.node_type = NODE_PROG;
    node.pos = prog_pos;
    if (GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
    {
        GxBus_PmProgExtInfoGet(node.prog_data.id, sizeof(AppProgExtInfo), &prog_ext_info );
        ret =GxPlayer_GetAudioSmartVolumeLoudness(&loudness);
        if((loudness !=0)&& (ret == 0))
        {
            prog_ext_info.loudness = loudness;
            GxBus_PmProgExtInfoAdd(node.prog_data.id, sizeof(AppProgExtInfo), &prog_ext_info);
            GxBus_PmSync(GXBUS_PM_SYNC_EXT_PROG);
        }
    }

    return;
}

void app_smart_volume_set_loudness(int32_t prog_id)
{
    AppProgExtInfo prog_ext_info;

    memset(&prog_ext_info, 0, sizeof(AppProgExtInfo));

    GxBus_PmProgExtInfoGet(prog_id, sizeof ( AppProgExtInfo ), &prog_ext_info );
    if(prog_ext_info.loudness != 0){
        GxPlayer_SetAudioSmartVolumeLoudness(prog_ext_info.loudness);
    }
    return;
}
#endif
static int app_av_set_audio_track(AvPlayerPara *player)
{

	#if 0
	GxMsgProperty_PlayerAudioTrack track = player->audio_track;

    	GxBus_ConfigSetInt(SOUND_MODE_KEY,player->audio_track);
	app_send_msg_exec(GXMSG_PLAYER_AUDIO_TRACK, &track);
	#else
	GxBus_ConfigSetInt(VOLUME_MODE_KEY,player->volume_scope);
#if SMART_VOLUME_SUPPORT
    app_smart_volume_support(player->volume_scope);
#endif
    #endif

	return 0;
}

#ifdef SUPPORT_TV_SCART
static int app_av_set_scart_source(AvPlayerPara *player)
{
	GxBus_ConfigSetInt(TV_SCART_KEY, player->scart_source);
	app_gpio_set_scart_rgb(player->scart_source);
	return 0;
}

static int app_av_set_scart_ratio(AvPlayerPara *player)
{
	if(player->scart_screen==DISPLAY_SCREEN_4X3)
	{
		app_gpio_set_scart_onoff(0);
		app_gpio_set_scart_tv(0);
	}
	else if(player->scart_screen==DISPLAY_SCREEN_16X9)
	{
		app_gpio_set_scart_onoff(0);
		app_gpio_set_scart_tv(1);
	}
	return 0;
}
#endif

#if AUDIO_DESCRIPTOR_SUPPORT
static int app_audio_descriptor_set_mode(AvPlayerPara *player)
{
	GxBus_ConfigSetInt(AD_CONF_KEY, player->ad_status);
	return 0;
}
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
static int app_ad_vol_offset_set(AvPlayerPara *player)
{
	GxBus_ConfigSetInt(AD_VOL_OFFSET_KEY, player->ad_vol_offset-3);
	return 0;
}
#endif
#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT

static int app_av_set_hdr_mode(AvPlayerPara *player)
{
    GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_DRCMODE, _hdr_app_para_convert_player(player->video_hdr_mode_sel));
    GxPlayer_SetVideoDrcMode(_hdr_app_para_convert_player(player->video_hdr_mode_sel));
    return 0;
}
#endif

extern status_t GxPlayer_SetAudioAC3Enable(int enable);
static int app_av_set_spdif_output(AvPlayerPara *player)
{
	GxMsgProperty_PlayerAudioAC3Mode  mode = {0};//0: decord; 2.bypas 3.auto

	if(player->spdif ==  AV_SPDIF_DECODE)
		mode.mode = AUDIO_AC3_DECODE_MODE;
	else if(player->spdif == AV_SPDIF_BYPASS)
		mode.mode = AUDIO_AC3_BYPASS_MODE;
	else if(player->spdif == AV_SPDIF_AUTO)
		mode.mode = AUDIO_AC3_BYPASS_MODE|AUDIO_AC3_DECODE_MODE;
	else
		mode.mode = AUDIO_AC3_DECODE_MODE;

	GxBus_ConfigSetInt(TV_SPDIF_KEY, mode.mode);

	app_send_msg_exec(GXMSG_PLAYER_AUDIO_AC3_MODE,(void*)&mode);
	return 0;
}

static int app_av_setting_para_save(AvAppPara *result)
{
    AvAppPara *app = &s_AvAppPara;
    AvPlayerPara *player = &s_AvPlayerPara;

    if (app->tv_standard != result->tv_standard)
    {
		s_av_power_flag = true;
        app_av_set_standard(player);
    }
	else if(s_vout_mode != player->vout_mode)
	{
		app_av_set_standard(player);
	}
    if (app->tv_ratio != result->tv_ratio)
    {
        app_av_set_ratio(player);
		isTVModeChange = true;
#ifdef SUPPORT_TV_SCART
        if(s_scart_enable == 1)
        {
            app_av_set_scart_ratio(player);
        }
#endif
    }

    if (app->aspect_mode != result->aspect_mode)
    {
        app_av_set_aspect(player);
		isTVModeChange = true;
    }
    if (app->volume_mode != result->volume_mode)
    {
        app_av_set_audio_track(player);
    }

    if (app->video_out_mode != result->video_out_mode)
        app_av_set_videoout_mode(player->video_out_mode);

    if ((s_spdif_enable == 1) && (app->spdif != result->spdif))
    {
		app_av_set_spdif_output(player);
    }
#ifdef SUPPORT_TV_SCART
    if((s_scart_enable == 1) && (app->tv_scart != result->tv_scart))
    {
		app_av_set_scart_source(player);
    }
#endif

#if AUDIO_DESCRIPTOR_SUPPORT
	if (app->ad_mode!= result->ad_mode)
	{
		app_audio_descriptor_set_mode(player);
	}
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
        if (app->ad_vol_offset!= result->ad_vol_offset)
        {
            app_ad_vol_offset_set(player);
        }
#endif
#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT
    if(app->hdr_mode != result->hdr_mode)
    {
        app_av_set_hdr_mode(player);
    }
#endif
    // TODO:
//    init_value = ret_para->display_adapt;
//    GxBus_ConfigSetInt(TV_TYPE_IS_AUTO_KEY,init_value);

	if(isTVModeChange == true)
		app_av_setting_init();

	return 0;
}

static int _av_setting_save_pop_cb(PopDlgRet ret)
{
    AvAppPara result;

    app_av_app_para_get(&result);
    if(POP_VAL_OK == ret)
    {
        app_av_app_to_player_para(&result, &s_AvPlayerPara);
        GxBus_ConfigSetInt(TV_ADAPT_KEY, s_AvPlayerPara.display_adapt);
        if (s_AvAppPara.tv_standard == result.tv_standard)
        {
            s_AvPlayerPara.vout_mode = s_vout_mode;
            GxBus_ConfigSetInt(TV_STANDARD_KEY, s_vout_mode);
        }
        app_av_setting_para_save(&result);
    }
#ifdef AUTO_CHANGE_SUPPORT
    else
    {
        app_av_app_to_player_para(&s_AvAppPara, &s_AvPlayerPara);
        app_av_setting_para_save(&result);
    }
#endif
    s_av_menu_flag = false;
    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
    return 0;
}

static bool _av_setting_check_cb(void)
{
    bool ret = false;
    AvAppPara result;

    app_av_app_para_get(&result);
    ret = app_av_setting_para_change(&s_AvAppPara, &result);

    return ret;
}

static int app_av_setting_exit_callback(ExitType exit_type)
{
    AvAppPara result;
    int ret = EXIT_RET_DEFAULT;

    if(EXIT_ABANDON == exit_type)
    {
        app_av_app_para_get(&result);
        app_av_app_to_player_para(&s_AvAppPara, &s_AvPlayerPara);
        app_av_setting_para_save(&result);
        s_av_menu_flag = false;
    }
    else
    {
        if(false == app_system_set_callback_handler(_av_setting_check_cb, _av_setting_save_pop_cb))
        {
            s_av_menu_flag = false;
        }
        ret = EXIT_RET_POP;
    }

    return ret;
}

static void av_param_init(uint32_t item, uint32_t sel,char* cmb_content, uint32_t content_total, char** content, int (*func)(int),int max_len)
{
    uint32_t i;
    uint32_t num = 0;

	if(s_av_item_map[item] >= s_av_setting_opt.itemNum)
		return;

	s_av_item[s_av_item_map[item]].itemTitle = item_avout[item];
	if(item == ITEM_TV_STANDARD)
	{
		s_av_item[s_av_item_map[item]].itemType = ITEM_POP_CHOICE;
	}
	else
	{
		s_av_item[s_av_item_map[item]].itemType = ITEM_CHOICE;
	}

    // cmb content create
    strcpy(cmb_content, "[");
    num++;
    for (i=0; i<content_total; i++)
    {
        num= num+strlen(content[i]);
        if(num >= (max_len-1))
        {
            app_log_error("cmb_content Buffer overflow\n");
            return;
        }
        strcat(cmb_content, content[i]);
    }
    strcat(cmb_content, "]");
	s_av_item[s_av_item_map[item]].itemProperty.itemPropertyCmb.content = cmb_content;
	s_av_item[s_av_item_map[item]].itemProperty.itemPropertyCmb.sel = sel;
	s_av_item[s_av_item_map[item]].itemCallback.cmbCallback.CmbChange= func;
	s_av_item[s_av_item_map[item]].itemStatus = ITEM_NORMAL;
}

// delete the "," for display

static int _app_av_standard_poplist_cb(int32_t ret_sel, unsigned short key)
{
    GUI_SetProperty(app_system_get_combobox(ITEM_TV_STANDARD), "select", &ret_sel);
    poplist_destory_cb_free_item_content();
    return 0;
}

static int _app_av_standard_poplist_get_info(char **p_get_info)
{
    int i,j;
    char **p_info = NULL ;

    if(NULL == p_get_info)
    {
        app_log_error("\n_app_av_standard_poplist_get_info failed, param...%s,%d\n",__FILE__,__LINE__);
        return -1;
    }

    p_info = (char**)GxCore_Calloc(AV_STANDARD_TOTAL, sizeof(char*));
    if(NULL == p_info)
    {
        app_log_error("\033[31m!!!!!![%s, %d]:GxCore_Calloc failed!!!!!!!!\033[0m\n", __func__, __LINE__);
        return -1;
    }

    for(i = 0; i < AV_STANDARD_TOTAL; i++)
    {
        p_info[i] = (char*)GxCore_Malloc(15);// 15--must long than "1080p@60Hz"
        if(NULL == p_info[i])
        {
            app_log_error("\033[31m!!!![%s, %d]:GxCore_Calloc failed!!!!!\033[0m\n", __func__, __LINE__);
            if(p_info != NULL)
            {
                for(j = 0; j < i; j++)
                {
                    if(p_info[j] != NULL)
                        GxCore_Free(p_info[j]);
                }
                GxCore_Free(p_info);
            }
            return -1;
        }

        memcpy(p_info[i],tv_standard[i],strlen(tv_standard[i])>1?strlen(tv_standard[i])-1:0);
        p_info[i][strlen(tv_standard[i])>1?strlen(tv_standard[i])-1:0] = '\0';
    }

    *p_get_info = (char*)p_info ;
    return 0;
}


static int app_av_standard_cmb_press_callback(int key)
{
	char *p_item_content = NULL ;
	AvAppPara appara = {0};

	if(key == STBK_OK)
	{
		app_av_app_para_get(&appara);
		memset(&pop_list, 0, sizeof(PopList));
		if ( _app_av_standard_poplist_get_info(&p_item_content) < 0 )
			return EVENT_TRANSFER_STOP ;
		pop_list.title = item_avout[0];
		pop_list.item_num = AV_STANDARD_TOTAL;
		pop_list.item_content = (char **)p_item_content;//tv_standard;
		pop_list.sel = appara.tv_standard;//s_AvAppPara.tv_standard;
		pop_list.show_num= false;
		pop_list.mode = POP_MODE_UNBLOCK;
		pop_list.exit_cb = _app_av_standard_poplist_cb;
		poplist_create(&pop_list);

	}

	return EVENT_TRANSFER_KEEPON;
}
//#endif
static int _av_standard_change(int sel)
{
#ifdef AUTO_CHANGE_SUPPORT
	AvAppPara AVPara;
	AvPlayerPara player;
    uint32_t standard[AV_STANDARD_TOTAL] = {

#ifdef SUPPORT_480_I_P_OUTPUT
            GX_VIDEO_OUTPUT_480I,
			GX_VIDEO_OUTPUT_480P,
#endif
            GX_VIDEO_OUTPUT_576I,
            GX_VIDEO_OUTPUT_576P,
            GX_VIDEO_OUTPUT_720P_50HZ,
            GX_VIDEO_OUTPUT_720P_60HZ,
            GX_VIDEO_OUTPUT_1080I_50HZ,
            GX_VIDEO_OUTPUT_1080I_60HZ,
#ifdef SUPPORT_1080P_OUTPUT
            GX_VIDEO_OUTPUT_1080P_50HZ,
            GX_VIDEO_OUTPUT_1080P_60HZ,
#endif
    };


	app_system_set_item_data_update(s_av_item_map[ITEM_TV_STANDARD]);
	app_av_app_para_get(&AVPara);
	app_av_app_to_player_para(&AVPara, &player);
	if (sel > AV_STANDARD_AUTO)
    {
        player.vout_mode = standard[sel - 1];
		s_vout_mode = player.vout_mode;
        player.display_adapt = AV_ADAPT_NOAUTO;
    }
    else
    {
    // modify by zhouzhr 20120317
        player.vout_mode = 0;
    	player.display_adapt = AV_ADAPT_AUTO;
    }
	app_av_set_standard(&player);
#endif
	return 0;
}

static void app_av_standard_init(void)
{
#define STANDARD_MAX    (100)
    static char cmb_standard[STANDARD_MAX];

    memset(cmb_standard, 0, STANDARD_MAX);

    av_param_init(ITEM_TV_STANDARD, s_AvAppPara.tv_standard,
            cmb_standard, AV_STANDARD_TOTAL, tv_standard,_av_standard_change,STANDARD_MAX);
    s_av_item[s_av_item_map[ITEM_TV_STANDARD]].itemCallback.cmbCallback.CmbPress= app_av_standard_cmb_press_callback;
}

static int _av_ratio_change(int sel)
{
#ifdef AUTO_CHANGE_SUPPORT
#ifdef SUPPORT_TV_SCART
	if (s_scart_enable == 1)
	{
	//	AvAppPara AVPara;
		AvPlayerPara player;
		uint32_t ratio[AV_RATIO_TOTAL] = {DISPLAY_SCREEN_4X3, DISPLAY_SCREEN_16X9};
	//	app_system_set_item_data_update(s_av_item_map[ITEM_TV_RATIO]);
		//app_av_app_para_get(&AVPara);
		//app_av_app_to_player_para(&AVPara, &player);
		player.scart_screen = ratio[sel];
		app_av_set_scart_ratio(&player);
	}
#endif
#endif
	return 0;
}

static void app_av_ratio_init(void)
{
#define RATIO_MAX    (12)
    static char cmb_ratio[RATIO_MAX];

    memset(cmb_ratio, 0, RATIO_MAX);

    av_param_init(ITEM_TV_RATIO, s_AvAppPara.tv_ratio,
            cmb_ratio, AV_RATIO_TOTAL, tv_ratio,_av_ratio_change,RATIO_MAX);
}

static void app_av_aspect_init(void)
{
#define ASPECT_MAX    (36)
    static char cmb_aspect[ASPECT_MAX];
    memset(cmb_aspect, 0, ASPECT_MAX);

    av_param_init(ITEM_ASPECT_MODE, s_AvAppPara.aspect_mode,
            cmb_aspect, AV_ASPECT_TOTAL, aspect_mode,NULL,ASPECT_MAX);
}

static void app_av_voutput_init(void)
{
#define VOUTPUT_MAX    (36)
    static char cmb_voutput[VOUTPUT_MAX];
    memset(cmb_voutput, 0, VOUTPUT_MAX);

    av_param_init(ITEM_VIDEO_OUTPUT, s_AvAppPara.video_out_mode,
            cmb_voutput, AV_VIDEO_TOTAL, video_out_mode,NULL,VOUTPUT_MAX);
}

static void app_av_sound_init(void)
{
#define SOUND_MAX    (36)
    static char cmb_sound[SOUND_MAX];
    memset(cmb_sound, 0, SOUND_MAX);

    av_param_init(ITEM_SOUND_MODE, s_AvAppPara.volume_mode,
            cmb_sound, AV_SOUND_TOTAL, sound_mode,NULL,SOUND_MAX);

}

#ifdef SUPPORT_TV_SCART
static int _av_scart_change(int sel)
{
	AvAppPara AVPara;
	AvPlayerPara player;
	uint32_t scart[AV_SCART_TOTAL] = {AV_SCART_CVBS, AV_SCART_RGB};
	app_system_set_item_data_update(s_av_item_map[ITEM_TV_SCART]);
	app_av_app_para_get(&AVPara);
	app_av_app_to_player_para(&AVPara, &player);
	player.scart_source = scart[sel];
	app_av_set_scart_source(&player);
	return 0;
}

static void app_av_scart_init(void)
{
#define SCART_MAX    (12)
    static char cmb_scart[SCART_MAX];
    memset(cmb_scart, 0, SCART_MAX);

    av_param_init(ITEM_TV_SCART, s_AvAppPara.tv_scart,
            cmb_scart, AV_SCART_TOTAL, scart,_av_scart_change,SCART_MAX);
}
#endif

static void app_av_spdif_init(void)
{
#define SPDIF_MAX    (36)
    static char cmb_spdif[SPDIF_MAX];
    memset(cmb_spdif, 0, SPDIF_MAX);

    av_param_init(ITEM_SPDIF, s_AvAppPara.spdif,
            cmb_spdif, AV_SPDIF_TOTAL, spdif,NULL,SPDIF_MAX);
}

#if AUDIO_DESCRIPTOR_SUPPORT

static int _ad_mode_change(int sel)
{
    app_system_set_item_data_update(s_av_item_map[ITEM_AD_MODE]);

#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
    if(sel == 1)
    {
        app_system_set_item_state_update(ITEM_AD_VOL_OFFSET, ITEM_NORMAL);
    }
    else
    {
        app_system_set_item_state_update(ITEM_AD_VOL_OFFSET, ITEM_HIDE);
    }
    app_system_set_focus_item(ITEM_AD_MODE);
 #endif
    return 0;
}

static void app_audio_descriptor_item_init(void)
{
	#define AD_MAX    (10)
    static char cmb_ad[AD_MAX];
    memset(cmb_ad, 0, AD_MAX);

    av_param_init(ITEM_AD_MODE, s_AvAppPara.ad_mode,
            cmb_ad, AUDIO_DESCRIPTOR_TOTAL, ad_status,_ad_mode_change,AD_MAX);
}
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
static int _ad_vol_offset_change(int sel)
{
    app_system_set_item_data_update(s_av_item_map[ITEM_AD_VOL_OFFSET]);
    return 0;
}
static void app_ad_vol_offset_item_init(void)
{
#define AD_VOL_OFFSET_BUF_MAX    (24)

    int init_value = 0;
    static char cmb_ad_vol_offset[AD_VOL_OFFSET_BUF_MAX];
    memset(cmb_ad_vol_offset, 0, AD_VOL_OFFSET_BUF_MAX);

    av_param_init(ITEM_AD_VOL_OFFSET, s_AvAppPara.ad_vol_offset,
            cmb_ad_vol_offset, AD_VOL_OFFSET_MAX, sg_ad_vol_offset,_ad_vol_offset_change,AD_VOL_OFFSET_BUF_MAX);

    GxBus_ConfigGetInt(AD_CONF_KEY, &init_value, AD_CONFIG);
    if(init_value == 1)
    {
        s_av_item[s_av_item_map[ITEM_AD_VOL_OFFSET]].itemStatus = ITEM_NORMAL;
    }
    else
    {
        s_av_item[s_av_item_map[ITEM_AD_VOL_OFFSET]].itemStatus = ITEM_HIDE;
    }
}
#endif
#endif

#if HIGH_DYNAMIC_RANGE_SUPPORT
static void  app_video_hdr_init(void)
{
    #define HDR_MAX    (16)
    static char cmb_hdr[HDR_MAX];
    memset(cmb_hdr, 0, HDR_MAX);

    av_param_init(ITEM_VIDEO_HDR, s_AvAppPara.hdr_mode,
            cmb_hdr, APP_VIDEO_HDR_MODE_TOTAL, hdr_mode,NULL,HDR_MAX);
}
#endif
void app_av_setting_menu_exec(void)
{
	uint8_t i;
	isTVModeChange = false;

	GxBus_ConfigGetInt(SPDIF_CONF_KEY, &s_spdif_enable, SPDIF_CONF);
#ifdef SUPPORT_TV_SCART
	GxBus_ConfigGetInt(SCART_CONF_KEY, &s_scart_enable, SCART_CONF);
#endif

	memset(&s_av_item_map, ITEM_AVOUT_TOTAL ,sizeof(s_av_item_map));
	for(i = ITEM_TV_STANDARD; i <= ITEM_SOUND_MODE; i++)
	{
		s_av_item_map[i] = i;
	}

	memset(&s_av_setting_opt, 0 ,sizeof(s_av_setting_opt));

	app_av_player_para_get(&s_AvPlayerPara);
	av_player_to_app_para(&s_AvPlayerPara, &s_AvAppPara);

	s_av_setting_opt.menuTitle = STR_ID_AV_SET;
	s_av_setting_opt.timeDisplay = TIP_HIDE;
	s_av_setting_opt.itemNum = ITEM_SOUND_MODE + 1;

	if(s_spdif_enable == 1)
	{
		s_av_setting_opt.itemNum++;
		s_av_item_map[ITEM_SPDIF] = i++;
	}
#ifdef SUPPORT_TV_SCART
	if(s_scart_enable == 1)
	{
		s_av_setting_opt.itemNum++;
		s_av_item_map[ITEM_TV_SCART] = i++;
	}
#endif
#if AUDIO_DESCRIPTOR_SUPPORT
    int ad_flag = 0;
    GxPlayer_GetAudioDescriptorVaild(&ad_flag);
    if(ad_flag == 1)
    {
        s_av_setting_opt.itemNum++;
        s_av_item_map[ITEM_AD_MODE] = i++;
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
        s_av_setting_opt.itemNum++;
        s_av_item_map[ITEM_AD_VOL_OFFSET] = i++;
#endif
    }
#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT
        s_av_setting_opt.itemNum++;
        s_av_item_map[ITEM_VIDEO_HDR] = i++;
#endif
	s_av_setting_opt.item = s_av_item;


	app_av_standard_init();
	app_av_ratio_init();
	app_av_aspect_init();
    app_av_voutput_init();
	app_av_sound_init();

	if(s_spdif_enable == 1)
	{
		app_av_spdif_init();
	}
#ifdef SUPPORT_TV_SCART
	if(s_scart_enable == 1)
	{
		app_av_scart_init();
	}
#endif
#if AUDIO_DESCRIPTOR_SUPPORT
    if(ad_flag == 1)
    {
        app_audio_descriptor_item_init();
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
        app_ad_vol_offset_item_init();
#endif
    }
#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT
    app_video_hdr_init();
#endif
	s_av_setting_opt.exit = app_av_setting_exit_callback;
	app_system_set_create(&s_av_setting_opt);
    s_av_menu_flag = true;
}

status_t app_video_standard_hotkey(void)
{
#if 0
    if(s_av_menu_flag == true)
    {
        GUI_Event event = {0};

        event.type = GUI_KEYDOWN;
		event.key.sym = STBK_RIGHT;

        app_system_set_item_event(ITEM_TV_STANDARD, &event);
    }
    else
#endif
    {
        GxBus_ConfigGetInt(SPDIF_CONF_KEY, &s_spdif_enable, SPDIF_CONF);
#ifdef SUPPORT_TV_SCART
	    GxBus_ConfigGetInt(SCART_CONF_KEY, &s_scart_enable, SCART_CONF);
#endif
        app_av_player_para_get(&s_AvPlayerPara);
		s_vout_mode = s_AvPlayerPara.vout_mode;
        av_player_to_app_para(&s_AvPlayerPara, &s_AvAppPara);

        s_AvAppPara.tv_standard += 1;
        if(s_AvAppPara.tv_standard >= AV_STANDARD_TOTAL)
            s_AvAppPara.tv_standard = 1;

		s_av_power_flag = true;
        app_av_app_to_player_para(&s_AvAppPara,&s_AvPlayerPara);
        app_av_set_standard(&s_AvPlayerPara);
        GxBus_ConfigSetInt(TV_ADAPT_KEY, s_AvPlayerPara.display_adapt);
        if(s_av_menu_flag == true)
        {
            s_av_item[s_av_item_map[ITEM_TV_STANDARD]].itemProperty.itemPropertyCmb.sel = s_AvAppPara.tv_standard;
            app_system_set_item_property(ITEM_TV_STANDARD, &s_av_item[s_av_item_map[ITEM_TV_STANDARD]].itemProperty);
			if (GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_OPTION_LIST))
			{
				GUI_SetProperty("listview_pop_option", "select",&(s_AvAppPara.tv_standard));
				pop_list.sel = s_AvAppPara.tv_standard;
				GUI_SetProperty(WND_POP_OPTION_LIST, "update_all", NULL);
			}
        }
    }

    return GXCORE_SUCCESS;
}

status_t app_video_aspect_hotkey(void)
{
    app_av_player_para_get(&s_AvPlayerPara);
    s_vout_mode = s_AvPlayerPara.vout_mode;
    av_player_to_app_para(&s_AvPlayerPara, &s_AvAppPara);

    s_AvAppPara.tv_ratio++;
    if(s_AvAppPara.tv_ratio >= AV_RATIO_TOTAL)
        s_AvAppPara.tv_ratio = AV_RATIO_16_9;

    if(s_av_menu_flag == true)
    {
    	s_av_power_flag = true;
        s_av_item[s_av_item_map[ITEM_TV_RATIO]].itemProperty.itemPropertyCmb.sel = s_AvAppPara.tv_ratio;
        app_system_set_item_property(ITEM_TV_RATIO, &s_av_item[s_av_item_map[ITEM_TV_RATIO]].itemProperty);
    	if (GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_OPTION_LIST))
    	{
    		GUI_SetProperty("listview_pop_option", "select",&(s_AvAppPara.tv_standard));
    		pop_list.sel = s_AvAppPara.tv_ratio;
    		GUI_SetProperty(WND_POP_OPTION_LIST, "update_all", NULL);
    	}
    }
    else
    {
        app_av_app_to_player_para(&s_AvAppPara,&s_AvPlayerPara);
    	s_av_power_flag = true;
        app_av_set_standard(&s_AvPlayerPara);
    }
    return GXCORE_SUCCESS;
}

#if WEBAPI_SUPPORT
#include <cJSON.h>
int app_webapi_av_setting_busy(void)
{
	return s_av_menu_flag;
}

void app_webapi_get_av_settings(int *standard, int *ratio, int *aspect, int *sound, int *spdif, int *ad, int *hdr)
{
	GxBus_ConfigGetInt(SPDIF_CONF_KEY, &s_spdif_enable, SPDIF_CONF);
#ifdef SUPPORT_TV_SCART
	GxBus_ConfigGetInt(SCART_CONF_KEY, &s_scart_enable, SCART_CONF);
#endif
	app_av_player_para_get(&s_AvPlayerPara);
	av_player_to_app_para(&s_AvPlayerPara, &s_AvAppPara);

	*standard = s_AvAppPara.tv_standard;
	*ratio = s_AvAppPara.tv_ratio;
	*aspect = s_AvAppPara.aspect_mode;
	*sound = s_AvAppPara.volume_mode;
	*spdif = s_AvAppPara.spdif;
#if AUDIO_DESCRIPTOR_SUPPORT
	*ad = s_AvAppPara.ad_mode;
#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT
    *hdr= s_AvAppPara.hdr_mode;
#endif
}

void app_webapi_make_av_data(cJSON *standard, cJSON *ratio, cJSON *aspect, cJSON *sound, cJSON *spd, cJSON *ad, cJSON *hdr)
{
	int i = 0;

	for(i=0; i<AV_STANDARD_TOTAL; i++)
	{
		cJSON_AddItemToArray(standard, cJSON_CreateString(tv_standard[i]));
	}

	for(i=0; i<AV_RATIO_TOTAL; i++)
	{
		cJSON_AddItemToArray(ratio, cJSON_CreateString(tv_ratio[i]));
	}

	for(i=0; i<AV_ASPECT_TOTAL; i++)
	{
		cJSON_AddItemToArray(aspect, cJSON_CreateString(aspect_mode[i]));
	}

	for(i=0; i<AV_SOUND_TOTAL; i++)
	{
		cJSON_AddItemToArray(sound, cJSON_CreateString(sound_mode[i]));
	}

	for(i=0; i<AV_SPDIF_TOTAL; i++)
	{
		cJSON_AddItemToArray(spd, cJSON_CreateString(spdif[i]));
	}

#if AUDIO_DESCRIPTOR_SUPPORT
	for(i=0; i<AUDIO_DESCRIPTOR_TOTAL; i++)
	{
		cJSON_AddItemToArray(ad, cJSON_CreateString(ad_status[i]));
	}
#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT
	for(i=0; i<APP_VIDEO_HDR_MODE_TOTAL; i++)
	{
		cJSON_AddItemToArray(hdr, cJSON_CreateString(hdr_mode[i]));
    }
#endif
}

int app_webapi_save_av_settings(int standard, int ratio, int aspect, int sound, int spdif, int ad, int hdr)
{
	int ret = 0;
	AvAppPara result;

	GxBus_ConfigGetInt(SPDIF_CONF_KEY, &s_spdif_enable, SPDIF_CONF);
#ifdef SUPPORT_TV_SCART
	GxBus_ConfigGetInt(SCART_CONF_KEY, &s_scart_enable, SCART_CONF);
#endif
	app_av_player_para_get(&s_AvPlayerPara);
	s_vout_mode = s_AvPlayerPara.vout_mode;
	av_player_to_app_para(&s_AvPlayerPara, &s_AvAppPara);

	memcpy(&result, &s_AvAppPara, sizeof(AvAppPara));
	if((standard>=0)&&standard<AV_STANDARD_TOTAL)
	{
		s_AvAppPara.tv_standard = standard;
		ret++;
	}
	if((ratio>=0)&&ratio<AV_RATIO_TOTAL)
	{
		s_AvAppPara.tv_ratio = ratio;
		ret++;
	}
	if((aspect>=0)&&aspect<AV_ASPECT_TOTAL)
	{
		s_AvAppPara.aspect_mode = aspect;
		ret++;
	}
	if((sound>=0)&&sound<AV_SOUND_TOTAL)
	{
		s_AvAppPara.volume_mode = sound;
		ret++;
	}
	if((spdif>=0)&&spdif<AV_SPDIF_TOTAL)
	{
		s_AvAppPara.spdif = spdif;
		ret++;
	}
#if AUDIO_DESCRIPTOR_SUPPORT
	if((ad>=0)&&ad<AUDIO_DESCRIPTOR_TOTAL)
	{
		s_AvAppPara.ad_mode = ad;
		ret++;
	}
#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT
	if((hdr>=0)&&hdr<APP_VIDEO_HDR_MODE_TOTAL)
	{
		s_AvAppPara.hdr_mode = hdr;
		ret++;
	}
#endif
	app_av_app_to_player_para(&s_AvAppPara,&s_AvPlayerPara);
	s_av_power_flag = true;

	GxBus_ConfigSetInt(TV_ADAPT_KEY, s_AvPlayerPara.display_adapt);
	if(s_AvAppPara.tv_standard != result.tv_standard)
	{
		s_av_power_flag = true;
		app_av_set_standard(&s_AvPlayerPara);
	}
	else if(s_vout_mode != s_AvPlayerPara.vout_mode)
	{
		app_av_set_standard(&s_AvPlayerPara);
	}

	if(s_AvAppPara.tv_ratio != result.tv_ratio)
	{
		app_av_set_ratio(&s_AvPlayerPara);
		isTVModeChange = true;
#ifdef SUPPORT_TV_SCART
		if(s_scart_enable == 1)
		{
			app_av_set_scart_ratio(&s_AvPlayerPara);
		}
#endif
	}

	if(s_AvAppPara.aspect_mode != result.aspect_mode)
	{
		app_av_set_aspect(&s_AvPlayerPara);
		isTVModeChange = true;
	}

	if(s_AvAppPara.volume_mode != result.volume_mode)
	{
		app_av_set_audio_track(&s_AvPlayerPara);
	}

	if((s_spdif_enable == 1) && (s_AvAppPara.spdif != result.spdif))
	{
		app_av_set_spdif_output(&s_AvPlayerPara);
	}

#ifdef SUPPORT_TV_SCART
	if((s_scart_enable == 1) && (s_AvAppPara.tv_scart != result.tv_scart))
	{
		app_av_set_scart_source(&s_AvPlayerPara);
	}
#endif

#if AUDIO_DESCRIPTOR_SUPPORT
	if (s_AvAppPara.ad_mode!= result.ad_mode)
	{
		app_audio_descriptor_set_mode(&s_AvPlayerPara);
	}
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
    if (s_AvAppPara.ad_vol_offset!= result.ad_vol_offset)
    {
        app_ad_vol_offset_set(&s_AvPlayerPara);
    }
#endif
#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT
	if (s_AvAppPara.hdr_mode!= result.hdr_mode)
	{
        GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_DRCMODE, _hdr_app_para_convert_player(s_AvPlayerPara.video_hdr_mode_sel));
	}

#endif
	if(isTVModeChange == true)
		app_av_setting_init();

	return ret;
}

#endif

