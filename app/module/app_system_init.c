/*****************************************************************************
* 			  CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009-2010, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_system_init.c
* Author    : 	shenbin
* Project   :	GoXceed -S
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2010.2.27	           shenbin         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "module/app_time.h"
#include "sys_setting_manage.h"
#include "app_utility.h"
#include "full_screen.h"
#include "app_config.h"
#include "gxcore.h"
#include "tree.h"
#include "parser.h"
#include "app_book.h"
#include "app_volume_value.h"
#if !OTT_SUPPORT
#include "module/app_frontend_board_config.h"
#endif
#include "module/pm/gxpm_urlcontrol.h"
#include "devapi/chip_info.h"
#if TKGS_SUPPORT
#include "app_tkgs_upgrade.h"
#endif
#if NETAPPS_SUPPORT
#include <gx_apps.h>
#include <app_netapps_init.h>
#endif

#include <fcntl.h>

#if PDMX_SUPPORT
#include "app_pdmx.h"
#endif

#if MULTISTREAM_SUPPORT
#include "module/app_multistream.h"
#endif
#if DEMOD_DVB_T || DEMOD_ISDBT
#include "app_t2_area_config.h"
#include "app_rf_channels.h"
#endif
#if FM_SUPPORT
#include "app_fm.h"
#endif
#if SECURE_PVR_SUPPORT
#include "gxsecure/gxtfm_api.h"
#include "gxplayer_pvr.h"
#endif

#if (DEMOD_DVB_S > 0)
extern void app_sat_list_del_sel_status(void);
extern void app_sat_list_close_lnb(void);
#endif
extern void app_init_sleep_set(void);
extern VideoOutputMode app_av_get_cvbs_standard(VideoOutputMode vout_mode);
#if SMART_VOLUME_SUPPORT
extern void app_smart_volume_support(int32_t volume_scope);
#endif
#if TKGS_SUPPORT
extern status_t app_tkg_lcn_flush(void);
extern void app_tkgs_sort_channels_by_lcn(void);
extern void app_tkgs_init_operatemode(void);
#endif
extern char *app_sel_to_priority_lang_str(uint32_t lang_1, uint32_t lang_2);
#if NETAPPS_SUPPORT
extern void app_iptv_factory_reset(void);
#endif

/* Private functions ------------------------------------------------------ */
// set the value that the same with player service
#if DEMOD_DVB_T || DEMOD_ISDBT
static int32_t app_get_default_country(void)
{
    int32_t country_index = 0;//reference g_CountryName

    if(app_oem_get_int(OEM_COUNTRY_CONFIG, &country_index) == GXCORE_ERROR)
        country_index = T2_COUNTRY_VALUE;

    return country_index;
}
#endif

#if SECURE_PVR_SUPPORT
static int _pvr_encrypt(PlayerPvrEncryptPara *para)
{
    int ret = -1;
    GxTfmCrypto param = {0};

    if(para->enc_mode != PLAYER_PVR_DVR_ENCRYPT)
    {
        app_log_error("secure pvr enc mode error: %d\n", para->enc_mode);
        return -1;
    }

    memset(&param, 0, sizeof(GxTfmCrypto));
    param.module = TFM_MOD_M2M;
    param.alg           = TFM_ALG_AES128;
    param.opt           = TFM_OPT_ECB;
    param.src.id        = TFM_SRC_MEM;
    param.dst.id        = TFM_DST_MEM;
    param.input.buf     = para->pvr.i_buf;
    param.input.length  = para->pvr.i_size;
    param.output.buf    = para->pvr.o_buf;
    param.output.length = para->pvr.o_size;
    param.even_key.id   = TFM_KEY_RK;

    if ((para->ibuf_type == PLAYER_PVR_BUF_NORMAL) && (para->obuf_type == PLAYER_PVR_BUF_NORMAL))
    {
        param.flags = 0 ;
    }
    else if ((para->ibuf_type == PLAYER_PVR_BUF_SECURE) && (para->obuf_type == PLAYER_PVR_BUF_NORMAL))
    {
        param.input_paddr   = (unsigned int)para->pvr.i_buf;
        param.flags         = TFM_FLAG_CRYPT_HW_PVR_MODE | TFM_FLAG_CRYPT_SRC_HW_ADDR ;
    }
    else
    {
        app_log_error("secure pvr en type = %d, %d \n",para->ibuf_type,para->obuf_type);
        return -1 ;
    }

    ret = GxTfm_Encrypt(&param);
    if (ret < 0)
    {
        app_log_error("GxTfm_Encrypt error, ret = %d\n", ret);
        return -1;
    }
    return 0;
}

static int _pvr_decrypt(PlayerPvrEncryptPara *para)
{
    int ret = -1;
    GxTfmCrypto param = {0};

    if(para->enc_mode != PLAYER_PVR_DVR_ENCRYPT)
    {
        app_log_error("secure pvr enc mode error: %d\n", para->enc_mode);
        return -1;
    }

    memset(&param, 0, sizeof(GxTfmCrypto));
    param.module = TFM_MOD_M2M;
    param.alg           = TFM_ALG_AES128;
    param.opt           = TFM_OPT_ECB;
    param.src.id        = TFM_SRC_MEM;
    param.dst.id        = TFM_DST_MEM;
    param.input.buf     = para->pvr.i_buf;
    param.input.length  = para->pvr.i_size;
    param.output.buf    = para->pvr.o_buf;
    param.output.length = para->pvr.o_size;
    param.even_key.id   = TFM_KEY_RK;

    if ((para->ibuf_type == PLAYER_PVR_BUF_NORMAL) && (para->obuf_type == PLAYER_PVR_BUF_NORMAL))
    {
        param.flags = 0 ;
    }
    else if ((para->ibuf_type == PLAYER_PVR_BUF_NORMAL ) && (para->obuf_type == PLAYER_PVR_BUF_SECURE))
    {
        param.output_paddr  = (unsigned int)para->pvr.o_buf;
        param.flags         = TFM_FLAG_CRYPT_HW_PVR_MODE | TFM_FLAG_CRYPT_SRC_HW_ADDR ;
    }
    else
    {
        app_log_error("secure pvr de type = %d, %d \n",para->ibuf_type,para->obuf_type);
        return -1 ;
    }

    ret = GxTfm_Decrypt(&param);
    if (ret < 0)
    {
        app_log_error("GxTfm_Decrypt error, ret = %d\n", ret);
        return -1;
    }
    return 0;
}

static PlayerPvrEncryptOps s_secure_pvr_ops = {
    .name     = "PVR encrypt/decrypt",
    .author   = "test",
    .enc_mode = PLAYER_PVR_DVR_ENCRYPT,
    .encrpyt  = _pvr_encrypt,
    .decrpyt  = _pvr_decrypt,
};
#endif

static void app_system_video_init(void)
{
	#define MAX_COLOR_VALUE 100
	int32_t init_value;
 	int32_t chroma;
	int32_t brightness;
	int32_t contrast;
	int32_t sharpness;
	int32_t hue;
	GxMsgProperty_PlayerDisplayScreen screen_ratio;
	GxMsgProperty_PlayerVideoAspect aspect;

#if SECURE_PVR_SUPPORT
	PlayerRecordEncrypt pvr_encrpyt_config ;
#endif

	// 4:3  16:9, for screen size
	GxBus_ConfigGetInt(TV_RATIO_KEY, &init_value, TV_RATIO);
	screen_ratio.aspect = init_value;
	screen_ratio.xres = APP_THEME_XRES;
	screen_ratio.yres = APP_THEME_YRES;
	GxPlayer_SetDisplayScreen(screen_ratio);

	// Letter / Pilar box  pan scan
	GxBus_ConfigGetInt(ASPECT_MODE_KEY, &init_value, ASPECT_MODE);
	aspect = init_value;
	GxPlayer_SetVideoAspect(aspect);

	// HDMI&RCA color set: saturation brightness contrast
	GxBus_ConfigGetInt(SATURATION_KEY, &chroma, SATURATION);
	GxBus_ConfigGetInt(BRIGHTNESS_KEY, &brightness, BRIGHTNESS);
	GxBus_ConfigGetInt(CONTRAST_KEY, &contrast, CONTRAST);
	GxBus_ConfigGetInt(SHARPNESS_KEY, &sharpness, SHARPNESS);
	GxBus_ConfigGetInt(HUE_KEY, &hue, HUE);
	system_setting_set_screen_color(chroma, brightness, contrast,
            sharpness, hue, true);

	// freeze control
	GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
	GxPlayer_SetFreezeFrameSwitch(init_value);

    // volume init
	GxBus_ConfigGetInt(AUDIO_VOLUME_KEY, &init_value, AUDIO_VOLUME);
	if(init_value > 100)
	{
		init_value = 100;
	}
	app_system_vol_set_val(init_value);
	GxPlayer_SetAudioVolume(init_value);

#if SECURE_PVR_SUPPORT
	GxPVRPlayer_RegisterEncrypt(&s_secure_pvr_ops);

	memset(&pvr_encrpyt_config, 0, sizeof(PlayerRecordEncrypt));
	pvr_encrpyt_config.user_encrypt_enable = 1 ;
	pvr_encrpyt_config.user_encrypt_blocksize = 64*188 ;
	GxPlayer_MediaRecordEncrypt(PLAYER_FOR_REC, &pvr_encrpyt_config);
#endif
	return;
}

status_t app_system_gui_init(void)
{
	status_t ret = GXCORE_SUCCESS;
	GUI_Rect region_size = {0, 0, APP_THEME_XRES, APP_THEME_YRES};
	int32_t init_value;
    GuiPoolBlock block_map = {0};
#define GUI_POOL_BLOCK  12
    int bk_size[GUI_POOL_BLOCK] = {4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};
    int bk_count[GUI_POOL_BLOCK] = {200, 4000, 6800, 2900, 400, 780, 570, 510, 60, 15, 5, 12};

    block_map.total_size = GUI_POOL_BLOCK;
    block_map.size_info_set = bk_size;
    block_map.count_info_set = bk_count;
    GuiPool_SetSize(&block_map);
    // pic decode register
    GDI_RegisterJPEG_SOFT_Special();
	GDI_RegisterGIF();
    GDI_RegisterPNG();
#if FONT_GB2312_TO_UTF8
    gdi_register_gb2312();
#endif

    /*gui init*/
	ret = GUI_Init(WORK_PATH"theme/theme.xml");
	if(GXCORE_SUCCESS != ret)
	{
		app_log_error("[GUI_Init] ERROR\n");
		return GXCORE_ERROR;
	}
#if NETAPPS_SUPPORT
	app_netapps_gui_init();
#endif
	//this is spp region size
	GUI_SetInterface("image_size", &region_size);
	// this is osd region size
	GUI_SetInterface("interface_size", &region_size);

	GxBus_ConfigGetInt(OSD_LANG_KEY, &init_value, OSD_LANG);
	GUI_SetInterface("osd_language", g_LangName[init_value]);
#if (UI_MIRROR_SUPPORT > 0)
    app_menu_mirror_set(g_LangName[init_value]);
#endif

	GxBus_ConfigGetInt(TRANS_LEVEL_KEY, &init_value, TRANS_LEVEL);
	GUI_SetInterface("osd_alpha_global", &init_value);

	return GXCORE_SUCCESS;
}

void app_system_output_config(void)
{
    GxMsgProperty_PlayerVideoInterface Interface;
	GxMsgProperty_PlayerVideoModeConfig  mode_config;
	GxMsgProperty_PlayerVideoAutoAdapt auto_adapt = {0};
	int32_t init_value = 0;

    //printf("\n%s, %d\n", __func__, __LINE__);
// output config
    /// video output auto adapt
    GxBus_ConfigGetInt(TV_ADAPT_KEY, &init_value, TV_ADAPT_NOAUTO);
	auto_adapt.enable = init_value;
	auto_adapt.pal = GX_VIDEO_OUTPUT_PAL;
	auto_adapt.ntsc = GX_VIDEO_OUTPUT_NTSC_M;
	GxPlayer_SetVideoAutoAdapt(&auto_adapt);
	/// interface HDMI & RCA
#ifdef SUPPORT_SCART
	GxBus_ConfigGetInt(VIDEO_INTERFACE_KEY, &Interface, VIDEO_INTERFACE_SCART);
#else
	GxBus_ConfigGetInt(VIDEO_INTERFACE_KEY, &Interface, VIDEO_INTERFACE_MODE);
#endif
	GxPlayer_SetVideoOutputSelect(Interface);

    /// HDMI output
	GxBus_ConfigGetInt(TV_STANDARD_KEY, &init_value, TV_STANDARD);
	mode_config.interface = GX_VIDEO_OUTPUT_HDMI;
	mode_config.mode = init_value;
	GxPlayer_SetVideoOutputConfig(mode_config);

    /// RCA output
  	mode_config.interface = GX_VIDEO_OUTPUT_RCA;
	mode_config.mode = app_av_get_cvbs_standard(init_value);
	if(GX_VIDEO_OUTPUT_MODE_MAX != mode_config.mode)
	{
		GxPlayer_SetVideoOutputConfig(mode_config);
	}
    /// scart output
#ifdef SUPPORT_SCART
    //scart
	GxBus_ConfigGetInt(SCART_CONF_KEY, &init_value, SCART_CONF);
	if(init_value == 1)
    {
        mode_config.interface = GX_VIDEO_OUTPUT_SCART;
        mode_config.mode = app_av_get_cvbs_standard(init_value);
        if(GX_VIDEO_OUTPUT_MODE_MAX != mode_config.mode)
            GxPlayer_SetVideoOutputConfig(mode_config);
    }
    {
        int value = 0;
        GxBus_ConfigGetInt(TV_SCART_KEY, &value,TV_SCART);
        app_gpio_set_scart_rgb(value);

        GxBus_ConfigGetInt(TV_RATIO_KEY, &value,TV_RATIO);
        if(value==1)//16:9
        {
            app_gpio_set_scart_onoff(0);
            app_gpio_set_scart_tv(1);
            app_log_info("==============params int  16:9 ========\n");
        }
        else  if(value==0)//4:3
        {
            app_gpio_set_scart_onoff(0);
            app_gpio_set_scart_tv(0);
            app_log_info("==============params int  4:3 ========\n");
        }
    }
#endif
	app_system_video_init();

#if CVBS_TEST_SUPPORT
{
    extern  void app_cvbs_index_test_para_init(void);
    GxBus_ConfigGetInt(TV_STANDARD_KEY, &init_value, TV_STANDARD);
    if((init_value == GX_VIDEO_OUTPUT_576I)
            || (init_value == GX_VIDEO_OUTPUT_480I))
        app_cvbs_index_test_para_init();
}
#else
  GxBus_ConfigGetInt(VOLUME_MODE_KEY, &init_value, VOLUME_SCOMPE_);
#if SMART_VOLUME_SUPPORT
  app_smart_volume_support(init_value);
#endif
#endif
#if PLAYBACK_SPEED_SUPPORT
    int32_t sel = 0;
    GxBus_ConfigGetInt(AUDIO_SPEED_MODE_KEY, &sel, AUDIO_SPEED_MODE_VALUE);
    GxPlayer_SetAudioSpeedMode(sel);
#endif
    //printf("\n%s, %d\n", __func__, __LINE__);
}

void app_boot_pvr_config(void)
{
	GxMsgProperty_PlayerPVRConfig  pvrConfig = {0};
    int init_value = 0;

    // PVR config
	GxBus_ConfigGetInt(PVR_FILE_SIZE_KEY, &init_value, PVR_FILE_SIZE_VALUE);
	//GxBus_ConfigSetInt(PVR_FILE_SIZE_KEY, init_value);
	GxPlayer_GetPVRConfig(&pvrConfig);
	pvrConfig.volume_sizemb = init_value;
	GxPlayer_SetPVRConfig(&pvrConfig);
}


static int app_language_create_default_config(void)
{
    int32_t init_value;

    if(app_oem_get_int(OEM_LANG_OSD, &init_value) == GXCORE_ERROR)
    {
        init_value = OSD_LANG;
    }
    GxBus_ConfigSetInt(OSD_LANG_KEY, init_value);
    if(app_oem_get_int(OEM_LANG_AUD1, &init_value) == GXCORE_ERROR)
    {
        init_value = AUDIO1_LANG;
    }
    GxBus_ConfigSetInt(AUDIO_1ST_KEY, init_value);
    if(app_oem_get_int(OEM_LANG_AUD2, &init_value) == GXCORE_ERROR)
    {
        init_value = AUDIO2_LANG;
    }
    GxBus_ConfigSetInt(AUDIO_2ND_KEY, init_value);
    if(app_oem_get_int(OEM_LANG_SUBT, &init_value) == GXCORE_ERROR)
    {
        init_value = SUB_LANG;
    }
    GxBus_ConfigSetInt(SUBTILE_1ST_KEY, init_value);
    if(app_oem_get_int(OEM_LANG_TTX, &init_value) == GXCORE_ERROR)
    {
        init_value = TTX_LANG;
    }
    GxBus_ConfigSetInt(TTX_LANG_KEY, init_value);
    if(app_oem_get_int(OEM_LANG_EPG, &init_value) == GXCORE_ERROR)
    {
        init_value = EPG_LANG;
    }
    GxBus_ConfigSetInt(EPG_LANG_KEY, init_value);

    char *priority_lang = NULL;
    int s_audio_1st = 0;
    int s_audio_2nd = 0;
    GxBus_ConfigGetInt(AUDIO_1ST_KEY, &s_audio_1st, OSD_LANG);
    GxBus_ConfigGetInt(AUDIO_2ND_KEY, &s_audio_2nd, OSD_LANG);
    priority_lang = app_sel_to_priority_lang_str(s_audio_1st, s_audio_2nd);
    if (priority_lang != NULL)
    {
        GxBus_ConfigSet(PRIORITY_LANG_KEY, priority_lang);
        GxCore_Free(priority_lang);
        priority_lang = NULL;
    }
    else
    {
        GxBus_ConfigSet(PRIORITY_LANG_KEY, PRIORITY_LANG_VALUE);
    }

    return 0;
}

static void app_system_set_player_cache_size(void)
{
    uint32_t mem_size = 0;
    int esasize = 0;
    int pcmsize = 0;
    struct Gx_Mem_Info info = {0};

    GxCore_MemInfoGet("mem", &info);
    mem_size = info.size;

#ifdef LINUX_OS
    // linux play开启了多线程下载，需要更多内存，限定值要设置的更大
    if (mem_size >= 100 * SIZE_UNIT_M)
    {
        GxBus_ConfigSetInt(PLAY_RESERVED_MEMSIZE, 30 * SIZE_UNIT_M);
    }
    else if (mem_size >= 50 * SIZE_UNIT_M)
    {
        GxBus_ConfigSetInt(PLAY_RESERVED_MEMSIZE, 20 * SIZE_UNIT_M);
    }
#else
    if (mem_size >= 100 * SIZE_UNIT_M)
    {
        GxBus_ConfigSetInt(PLAY_RESERVED_MEMSIZE, 20 * SIZE_UNIT_M);
    }
    else if (mem_size >= 50 * SIZE_UNIT_M)
    {
        GxBus_ConfigSetInt(PLAY_RESERVED_MEMSIZE, 10 * SIZE_UNIT_M);
    }
#endif
    else
    {
#if (defined GEMINI || defined CYGNUS)
        GxBus_ConfigSetInt(PLAY_RESERVED_MEMSIZE, 4 * SIZE_UNIT_M);
#else
        GxBus_ConfigSetInt(PLAY_RESERVED_MEMSIZE, 6 * SIZE_UNIT_M);
#endif
    }

    // 252140: NETWORK_CACHE is distributed from RESERVED_MEM.
    // PLAYER_CONFIG_NETWORK_CACHED_MEMSIZE must be smaller than PLAY_RESERVED_MEMSIZE
#if NETAPPS_SUPPORT
    if (mem_size >= 100 * SIZE_UNIT_M)
    {
        GxBus_ConfigSetInt(PLAYER_CONFIG_NETWORK_CACHED_MEMSIZE, 8 * SIZE_UNIT_M);
    }
    else if (mem_size >= 50 * SIZE_UNIT_M)
    {
        GxBus_ConfigSetInt(PLAYER_CONFIG_NETWORK_CACHED_MEMSIZE, 4 * SIZE_UNIT_M);
    }
    else
    {
        GxBus_ConfigSetInt(PLAYER_CONFIG_NETWORK_CACHED_MEMSIZE, 512 * SIZE_UNIT_K);
    }
    GxBus_ConfigSetInt(PLAYER_CONFIG_NETWORK_TIMEOUT, 60 * 1000);
    /*
    * TCP/UDP socket RCVBUF/SNDBUF.
    * For Linux, Kernel default is rmem_default/max in ./buildroot/template/public/etc/sysctl.conf
        net.core.rmem_default=87380
        net.core.rmem_max=4194304
    * For Linux, We setsocketopt default by PLAYER_CONFIG_NETWORK_RECVBUF 256KB in gxplayer_config.h
    * For ECOS, accroding to #261747, change from 32K to 64K.
    */
    #ifdef ECOS_OS
        GxBus_ConfigSetInt(PLAYER_CONFIG_NETWORK_RECVBUF, 64 * SIZE_UNIT_K);
    #endif
#endif

    // Other caches are not distributed from RESERVED_MEM.
#if MINI_MEM_SUPPORT
    GxBus_ConfigSetInt(PLAYER_CONFIG_RECORD_CACHED_MEMSIZE, 1880*1024);
    GxBus_ConfigSetInt(PLAYER_CONFIG_PLAY_RESERVED_HOLESIZE, 0xa00000);
    GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_SUPPORT_PPZOOM, 0);
    GxBus_ConfigSetInt(PLAYER_CONFIG_AUDIO_DTS_ENABLE, 0);
    GxBus_ConfigSetInt(PLAYER_CONFIG_AUDIO_DOWNMIX, 1);
#else
    GxBus_ConfigSetInt(PLAYER_CONFIG_NETWORK_HLS_MULIPLE_LIST, 1);
#endif

    // ESV, ESA, PCM 解码空间调整要慎重，确保loader的CMD中没有独立配置，只要loader中配置了，方案上配置无效。
    esasize = 0;
    pcmsize = 0;
#if (FLAC_SUPPORT||OGG_SUPPORT)
    esasize = 128*1024;//esa
    pcmsize = 256*1024;//pcm
#endif
#if QUICK_SWITCH_SUPPORT
    esasize = esasize > 0x40000? esasize: 0x40000;// esa
    pcmsize = pcmsize > 0x80000? pcmsize: 0x80000;//pcm
#endif
    if(esasize > 0)
        app_player_mem_config("esamem", esasize);
    if(pcmsize > 0)
        app_player_mem_config("pcmmem", pcmsize);

#if MINI_MEM_SUPPORT
    app_player_mem_config("esvmem", 0x3C0000);
#endif

}

static void _player_video_extra_fb_config(void)
{
#define CYGNUS_CHIPNAME_6706S5 "6706S5-"
#define CYGNUS_CHIPNAME_6706H5 "6706H5-"
    struct Gx_Mem_Info info = {0};
    char chipname[16] = {0};
    int fb = 0, ret = 0;

    if(app_chip_name_get(chipname, 16) < 0)
        goto end;

    ret = GxCore_MemInfoGet("videomem", &info);
    if(0 == ret && info.size >= 0x3c00000)
        fb = 0xFFFFFFFF;
    else if(GXCHIP_IS_CANOPUS
            || GXCHIP_IS_VEGA
            || GXCHIP_IS_SCORPIO
            || (GXCHIP_IS_CYGNUS && (strstr(chipname, CYGNUS_CHIPNAME_6706S5) || strstr(chipname, CYGNUS_CHIPNAME_6706H5))))
        fb = VIDEO_CODEC_H264;

end:
    GxBus_ConfigSetInt(APP_VIDEO_EXTRA_FB_NUM, fb);
}

static int app_system_create_default_config(void)
{
	int32_t init_value;
	int32_t ret_value = 0;
    int32_t zone, half_zone, summer;

    // player
    /// audio
    GxBus_ConfigSetInt(PLAYER_CONFIG_AUDIO_ANTI_ERROR_CODE, 0);//音频解码抗误码级别: 0 - 不开启；1 - 级别1; 2 - 级别2; 3 - 防水印
    //// volume
    ret_value = app_oem_get_int(OEM_VOLUME, &init_value);
    init_value *=VOLUME_NUM;
    if(ret_value == GXCORE_ERROR)
    {
        init_value = AUDIO_VOLUME;
    }
    app_system_vol_save(init_value);
    GxBus_ConfigSetInt(PLAYER_CONFIG_AUDIO_VOL_STEP,AUDIO_STEP);// 音频输出音量缓慢变化步长

    GxBus_ConfigSetInt(AUDIO_LEVEL_KEY, AUDIO_LEVEL);
    GxBus_ConfigSetInt(MUTE_KEY, MUTE);
    GxBus_ConfigSetInt(SOUND_MODE_KEY, SOUND_MODE);
    //// app control
    GxBus_ConfigSetInt(VOLUME_MODE_KEY, VOLUME_SCOMPE_);

#if AUDIO_DESCRIPTOR_SUPPORT
    /*aac mode shoud be decode For AD function,value  1--decode 2 bypass 4 convert*/
    GxBus_ConfigSetInt(PLAYER_CONFIG_AUDIO_AAC_MODE, 1);
    GxBus_ConfigSetInt(AD_CONF_KEY, AD_CONFIG);
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
    GxBus_ConfigSetInt(AD_VOL_OFFSET_KEY, AD_VOL_OFFSET_VALUE);
#endif
#endif
#if BOOT_AD_SUPPORT
    GxBus_ConfigSet(PLAYER_CONFIG_AUTOPLAY_VIDEO_NAME, AD_PLAYER);
    GxBus_ConfigSet(PLAYER_CONFIG_AUTOPLAY_VIDEO_URL, AD_GX_VIDEO_PATH);
#endif
    /// video
    //// convans
    GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_SCREEN_XRES,APP_THEME_XRES);
    GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_SCREEN_YRES,APP_THEME_YRES);
    ////
    _player_video_extra_fb_config();
    GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_DEC_MOSAIC_GATE, VIDEO_MOSAIC_GATE);// 100: 不屏蔽马赛克

    //// ATSC-CC
#if ATSCCC_SUPPORT
    GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_DEC_CC_ENABLE, 1);
    GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_DEC_CC_DISPLAY, 0);
#endif

    //// HDR
#if HIGH_DYNAMIC_RANGE_SUPPORT
    extern int app_get_hdr_enable(void);
    if(app_get_hdr_enable())
    {
        GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_DEC_HDR_ENABLE, 1);
        GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_DRCMODE, GX_VDRC_TOSDR);
    }
#endif
//#if QUICK_SWITCH_SUPPORT
    GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_DEC_SYNC_FLAG, VIDEO_SYNCMODE_SLOWLY);
//#endif

    /// output
    //// PP control
#ifdef FULL_HD_PP_MODE_ENABLE
    GxBus_ConfigSetInt(PLAY_PP_MODE_MAX_WIDTH, DEFAULT_PP_SUPPORT_WIDTH);
    GxBus_ConfigSetInt(PLAY_PP_MODE_MAX_HEIGHT, DEFAULT_PP_SUPPORT_HEIGHT);
#endif
    //// color
#if APP_SHARPNESS_SUPPORT
    GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_SHARPNESS_EN, 1);
#endif
#if APP_HUE_SUPPORT
    GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_HUE_EN      , 1);
#endif
    GxBus_ConfigSetInt(SATURATION_KEY, SATURATION);
    GxBus_ConfigSetInt(BRIGHTNESS_KEY, BRIGHTNESS);
    GxBus_ConfigSetInt(CONTRAST_KEY, CONTRAST);
    ///// app control
    ret_value = app_oem_get_int(OEM_TRANS, &init_value);
    if( ret_value== GXCORE_ERROR)
    {
        init_value = TRANS_LEVEL;
    }
    GxBus_ConfigSetInt(TRANS_LEVEL_KEY, init_value);

    //// Switch program style
    GxBus_ConfigSetInt(FREEZE_SWITCH, FREEZE_SWITCH_VALUE);

    //// HDMI & CVBS
    if(app_oem_get_int(OEM_TVSTANARD_CONF, &init_value) == GXCORE_ERROR)
        init_value = TV_STANDARD;
    GxBus_ConfigSetInt(TV_STANDARD_KEY, init_value);
    GxBus_ConfigSetInt(TV_RATIO_KEY, TV_RATIO);
    GxBus_ConfigSetInt(TV_ADAPT_KEY, TV_ADAPT_NOAUTO);
    GxBus_ConfigSetInt(ASPECT_MODE_KEY, ASPECT_MODE);
    GxBus_ConfigSetInt(NTSC_TYPE_KEY, NTSC_TYPE);
    GxBus_ConfigSetInt(VIDEO_INTERFACE_KEY, VIDEO_INTERFACE_MODE);
    GxBus_ConfigSetInt(AV_VIDEO_OUT_MODE, VIDEO_OUT_MODE_DEFAULT_VALUE);
    //// scart
#ifdef SUPPORT_SCART
	//scart
	if(app_oem_get_int(OEM_SCART_CONF, &init_value) == GXCORE_ERROR)
	{
		init_value = SCART_CONF;
		GxBus_ConfigSetInt(SCART_CONF_KEY, init_value);
		init_value = TV_SCART;
		GxBus_ConfigSetInt(TV_SCART_KEY, init_value);
		GxBus_ConfigSetInt(VIDEO_INTERFACE_KEY, VIDEO_INTERFACE_MODE);
	}
	else
	{
		if(init_value == -1)
		{
			init_value = 0;
			GxBus_ConfigSetInt(SCART_CONF_KEY, init_value);
		}
		else
		{
			GxBus_ConfigSetInt(TV_SCART_KEY, init_value);
			init_value = 1;
			GxBus_ConfigSetInt(SCART_CONF_KEY, init_value);
		}
	}
	GxBus_ConfigSetInt(VIDEO_INTERFACE_KEY, VIDEO_INTERFACE_SCART);
#endif
    //// Spdif
    if(app_oem_get_int(OEM_SPDIF_CONF, &init_value) == GXCORE_ERROR)
    {
        init_value = SPDIF_CONF;
        GxBus_ConfigSetInt(SPDIF_CONF_KEY, init_value);
        init_value = BYPASS_MODE;//AC3_BYPASS;
        GxBus_ConfigSetInt(PLAYER_CONFIG_AUDIO_AC3_ENABLE, 1);
        GxBus_ConfigSetInt(TV_SPDIF_KEY, init_value);
    }
    else
    {
        GxBus_ConfigSetInt(PLAYER_CONFIG_AUDIO_AC3_ENABLE, 1);
        GxBus_ConfigSetInt(TV_SPDIF_KEY, init_value);
        init_value = 1;
        GxBus_ConfigSetInt(SPDIF_CONF_KEY, init_value);
    }

    /// PVR
    GxBus_ConfigSetInt(PVR_FORCE_HWTS_KEY, PVR_FORCE_HWTS_FLAG);
    GxBus_ConfigSet(PVR_PARTITION_KEY, PVR_PARTITION);
    GxBus_ConfigSetInt(PLAYER_CONFIG_PVR_PLAYBACK_HWTS_SYNCMODE, FIXD_VPTS_RECOVER);

    //// app control
    GxBus_ConfigSetInt(PVR_DURATION_KEY, PVR_DURATION_VALUE);
    GxBus_ConfigSetInt(PVR_TIMESHIFT_KEY, PVR_TIMESHIFT_FLAG);
    GxBus_ConfigSetInt(PVR_SECTIONRECORD_KEY,PVR_SECTIONRECORD_FLAG);
    GxBus_ConfigSetInt(PVR_FILE_SIZE_KEY, PVR_FILE_SIZE_VALUE);
    GxBus_ConfigSet(PVR_PARTITION_KEY, PVR_PARTITION);

    /// Player memory control
    app_system_set_player_cache_size();

    // APP control
    ///// log output control
    //GxBus_ConfigSetInt(LOG_OUT_PUT_TYPE, 0);//初始为串口输出

    /// prog id for boot play
    GxBus_ConfigSetInt(ALL_TV_KEY, ALL_TV_ID);
    GxBus_ConfigSetInt(ALL_RADIO_KEY, ALL_RADIO_ID);
    /// IPTV LITE boot play mode
#if IPTV_LITE_SUPPORT
	GxBus_ConfigSetInt(POWERON_PLAY_KEY, POWERON_PLAY_VALUE);
#endif
    /// ui display
#if NO_SIGNAL_AUTO_STANDBY_SUPPORT
    GxBus_ConfigSetInt(NO_SIGNAL_AUTO_STANDBY_KEY,NO_SIGNAL_AUTO_STANDBY_VALUE);
#endif
#if NO_SIGNAL_ALERT_TIP_SUPPORT
    GxBus_ConfigSetInt(NO_SIGNAL_ALERT_TIP_KEY, NO_SIGNAL_ALERT_TIP_VALUE);
#endif
#if PVR_STANDBY_TIP_SUPPORT
    GxBus_ConfigSetInt(PVR_END_STANDBY_KEY,PVR_END_STANDBY_VALUE);
#endif
    GxBus_ConfigSetInt(INFOBAR_TIMEOUT_KEY, INFOBAR_TIMEOUT);

    /// Power on control
#if POWER_ON_CONTROL_SUPPORT
	GxBus_ConfigSetInt(POWER_CONTROL_KEY, POWER_CONTROL);
	GxBus_ConfigSetInt(POWER_STANDBY_KEY, POWER_STANDBY);
#endif

    /// search
    //// main frequence control
#if DEMOD_DVB_C | DEMOD_DTMB
    GxBus_ConfigSetInt(MAIN_FREQ_KEY, MAIN_FREQ_VALUE);
    GxBus_ConfigSetInt(MAIN_FREQ_SYM_KEY, MAIN_FREQ_SYM_VALUE);
    GxBus_ConfigSetInt(MAIN_FREQ_QAM_KEY, MAIN_FREQ_QAM_VALUE);
#endif
    //// DTS search control
    GxBus_ConfigSetInt(APP_SEARCH_DTS_SUPPORT, APP_GXBUS_SEARCH_DTS_NO);
    //// Search program name code change.[change to utf8]
#if FONT_GB2312_TO_UTF8
    GxBus_ConfigSetInt(SEARCH_PROGRAM_CODE,GXBUS_SEARCH_UTF8_NO);
#else
    GxBus_ConfigSetInt(SEARCH_PROGRAM_CODE,SEARCH_PROGRAM_CODE_UTF8);
#endif
    //// Default UI display
#if DEMOD_DVB_T || DEMOD_ISDBT || DEMOD_DVB_C
    GxBus_ConfigSetInt(SEARCH_MODE_TYPE_KEY, SEARCH_MODE_TYPE_VALUE);
#endif
    //// Default UI display
    GxBus_ConfigSetInt(SEARCH_FTA_CAS_KEY, SEARCH_FTA_CAS_VALUE);

    /// T2 region default
#if DEMOD_DVB_T
    GxBus_ConfigSetInt(T2_REGION_KEY, T2_REGION_VALUE);
#endif
    /// country and antenna power control
#if DEMOD_DVB_T || DEMOD_ISDBT
    init_value = app_get_default_country();
    GxBus_ConfigSetInt(T2_COUNTRY, init_value);

    //Antenna power
    if(GXCORE_ERROR == app_oem_get_int(OEM_ANTENNA_P0WER, &init_value))
        init_value = T2_ANTENNA_POWER_VALUE;
    GxBus_ConfigSetInt(T2_ANTENNA_POWER_KEY, init_value);
#endif

    /// System time
    init_value = TIME_GMT;
    GxBus_ConfigSetInt(TIME_MODE, init_value);
    if(app_oem_get_int(OEM_TIME_ZONE, &zone) == GXCORE_ERROR)
    {
        zone = TIME_ZONE_VALUE;
    }
    half_zone = TIME_HALF_ZONE_VALUE;
    if(app_oem_get_int(OEM_SUMMER_TIME, &summer) == GXCORE_ERROR)
    {
        summer = TIME_SUMMER_VALUE;
    }
    app_time_zone_config_set(&zone, &half_zone, &summer);

    /// language: OSD, Audio 1th&2th，EPG，TTX，Subtitle
    app_language_create_default_config();

    /// Lock
    //// switch control
    GxBus_ConfigSetInt(MENU_LOCK_KEY, MENU_LOCK);
    GxBus_ConfigSetInt(CHANNEL_LOCK_KEY, CHANNEL_LOCK);
    //GxBus_ConfigSetInt(MENU_BOOT_KEY, BOOT_LOCK);
    GxBus_ConfigSetInt(MENU_STAND3HOURS_KEY, STAND3HOUR_LOCK);
#if PARENTAL_LOCK_SUPPORT
    GxBus_ConfigSetInt(PARENTAL_GUIDANCE_KEY,PARENTAL_GUIDANCE_VALUE);
#endif
    //// Passwd control
    GxBus_ConfigSetInt(PASSWORD_KEY0, PASSWORD_1);
    GxBus_ConfigSetInt(PASSWORD_KEY1, PASSWORD_2);
    GxBus_ConfigSetInt(PASSWORD_KEY2, PASSWORD_3);
    GxBus_ConfigSetInt(PASSWORD_KEY3, PASSWORD_4);
#if LONG_PASSWORD_SUPPORT
    GxBus_ConfigSetInt(PASSWORD_KEY4, PASSWORD_5);
    GxBus_ConfigSetInt(PASSWORD_KEY5, PASSWORD_6);
#endif

    /// Panel
#if PANEL_SETTING_SUPPORT
    GxBus_ConfigSetInt(PANEL_LED_BRIGHTNESS_KEY, PANEL_LED_BRIGHTNESS);
    GxBus_ConfigSetInt(PANEL_LED_DISPLAY_KEY, PANEL_LED_DISPLAY);
#endif
    //// Standby for panel
#if STANDBY_SHOW_TIME_SHPPORT
    ret_value = app_oem_get_int(OEM_STANDBY_SHOWTIME, &init_value);
    if(ret_value == GXCORE_ERROR)
        init_value = PANEL_STANDBY_TIME;
    GxBus_ConfigSetInt(PANEL_LED_STANDBY_TIME_KEY, init_value);
#endif

    /// Sleep control
#if AUTO_STANDBY_SUPPORT
    GxBus_ConfigSetInt(SLEEP_KEY, SLEEP_VALUE);
    GxBus_ConfigSetInt(TIME_AUTO_STANDBY, TIME_AUTO_STANDBY_VALUE);
#endif

    /// local position
#if DEMOD_DVB_S
    GxBus_ConfigSetInt(LONGITUDE_KEY, LONGITUDE);
    GxBus_ConfigSetInt(LONGITUDE_DIRECT_KEY, LONGITUDE_DIRECT);
    GxBus_ConfigSetInt(LATITUDE_KEY, LATITUDE);
    GxBus_ConfigSetInt(LATITUDE_DIRECT_KEY, LATITUDE_DIRECT);
#endif

    /// Subtitle
    //GxBus_ConfigSetInt(SUBTITLE_MODE_KEY, SUBTITLE_MODE_VALUE);
    //GxBus_ConfigSetInt(SUBTITLE_LANG_KEY, SUBTITLE_LANG_VALUE);

    //#ifndef COLOR_16BIT_TTX_SUPPORT
    //// GA or CPU, GXBUS_TTX_SHOWMEAN_WRITE: CPU; GXBUS_TTX_SHOWMEAN_BLIT: GA，默认即是GXBUS_TTX_SHOWMEAN_WRITE。
    //    GxBus_ConfigSetInt(GXBUS_TTX_SHOWMEAN,GXBUS_TTX_SHOWMEAN_WRITE);
    //#endif

    /// frontend
    //// monitor
    GxBus_ConfigSetInt(FRONTEND_WATCH_TIME_KEY, FRONTEND_WATCH_TIME_VALUE);
	GxBus_ConfigSetInt(FRONTEND_CONFIG_WATCH_COUNT,FRONTEND_WATCH_TIME_COUNT);
    //// unicable
#if UNICABLE_SUPPORT
    GxBus_ConfigSetInt(GXBUS_FRONTEND_UNICABLE, GXBUS_FRONTEND_UNICABLE_YES);
#endif
    //// Mult-stream
#if MULTISTREAM_SUPPORT
    GxBus_ConfigSetInt(GXBUS_FRONTEND_MULTISTREAM, GXBUS_FRONTEND_MULTISTREAM_ON);
#endif
    //// Multi-Point LNBF #359333
#if LNBF_SUPPORT
    GxBus_ConfigSetInt(GXBUS_FRONTEND_LNBF, GXBUS_FRONTEND_LNBF_ON);
#endif
    //// PDMX
#if PDMX_SUPPORT
    GxBus_ConfigSetInt(GXBUS_FRONTEND_PDMX, GXBUS_FRONTEND_PDMX_AUTO);
#endif
    //// Tuner adapt
#if TUNER_AUTO_ADAPT_SUPPORT
    GxBus_ConfigSetInt(FE_TUNER1_ADAPT, FE_TUNER_ADAPT_INVALID_INDEX);
    GxBus_ConfigSetInt(FE_TUNER2_ADAPT, FE_TUNER_ADAPT_INVALID_INDEX);
    GxBus_ConfigSetInt(FE_TUNER3_ADAPT, FE_TUNER_ADAPT_INVALID_INDEX);
    GxBus_ConfigSetInt(FE_TUNER4_ADAPT, FE_TUNER_ADAPT_INVALID_INDEX);
    GxBus_ConfigSetInt(FE_TUNER1_ADDR_ADAPT, FE_TUNER_ADDR_ADAPT_INVALID_INDEX);
    GxBus_ConfigSetInt(FE_TUNER2_ADDR_ADAPT, FE_TUNER_ADDR_ADAPT_INVALID_INDEX);
    GxBus_ConfigSetInt(FE_TUNER3_ADDR_ADAPT, FE_TUNER_ADDR_ADAPT_INVALID_INDEX);
    GxBus_ConfigSetInt(FE_TUNER4_ADDR_ADAPT, FE_TUNER_ADDR_ADAPT_INVALID_INDEX);
#endif
#if DEMOD_AUTO_ADAPT_SUPPORT
    GxBus_ConfigSetInt(FE_DEMOD1_ADAPT, FE_DEMOD_ADAPT_INVALID_INDEX);
    GxBus_ConfigSetInt(FE_DEMOD2_ADAPT, FE_DEMOD_ADAPT_INVALID_INDEX);
    GxBus_ConfigSetInt(FE_DEMOD3_ADAPT, FE_DEMOD_ADAPT_INVALID_INDEX);
    GxBus_ConfigSetInt(FE_DEMOD4_ADAPT, FE_DEMOD_ADAPT_INVALID_INDEX);
    GxBus_ConfigSetInt(FE_DEMOD1_ADDR_ADAPT, FE_DEMOD_ADDR_ADAPT_INVALID_INDEX);
    GxBus_ConfigSetInt(FE_DEMOD2_ADDR_ADAPT, FE_DEMOD_ADDR_ADAPT_INVALID_INDEX);
    GxBus_ConfigSetInt(FE_DEMOD3_ADDR_ADAPT, FE_DEMOD_ADDR_ADAPT_INVALID_INDEX);
    GxBus_ConfigSetInt(FE_DEMOD4_ADDR_ADAPT, FE_DEMOD_ADDR_ADAPT_INVALID_INDEX);
#endif
#if LNBF_SUPPORT
    GxBus_ConfigSetInt(GXBUS_FRONTEND_LNBF, GXBUS_FRONTEND_LNBF_ON);
#endif
    //// FM
#if FM_SUPPORT
    GxBus_ConfigSetInt(FM_STEP, FM_STEP_VAL);
    GxBus_ConfigSetInt(FM_PLAY_INDEX, FM_PLAY_INDEX_VAL);
#endif

    /// LCN
#if LCN_SUPPORT
    if(app_oem_get_int(OEM_LCN_CONFIG, &init_value) == GXCORE_ERROR)
        init_value = LCN_VALUE;
    GxBus_ConfigSetInt(LCN_KEY, init_value);
    init_value = PROGRAM_LCN_SORT_VALUE;
    GxBus_ConfigSetInt(PROGRAM_LCN_SORT_KEY, init_value);
#endif
    GxBus_ConfigSetInt(T2_SORT_FLAG, T2_SORT_FLAG_VALUE);

    /// ACU
#if AUTO_UPDATE_SUPPORT
	GxBus_ConfigSetInt(T2_ACU_KEY, T2_ACU_VALUE);
#endif
    /// RF Modulator
#if ((RF_MODULATOR_SUPPORT > 0)&&(RF_RT500 > 0))
		GxBus_ConfigSetInt(RF_CH_NUMER_KEY, RF_CH_NUMER);
		GxBus_ConfigSetInt(RF_SOUND_MODE_KEY, RF_SOUND_MODE);
#endif
#if ((RF_MODULATOR_SUPPORT > 0) && (RF_RT534 > 0))
		GxBus_ConfigSetInt(RF_ON_OFF_KEY, RF_POWER_DEFAULT);
		GxBus_ConfigSetInt(RF_MODE_KEY, RF_MODE_DEFAULT);
#endif

    // SI engine
#if MINI_MEM_SUPPORT
    GxBus_ConfigSetInt(SI_CONFIG_PER_READ_SIZE, APP_SI_PER_READ_SIZE_DEF);
    GxBus_ConfigSetInt(APP_SI_SMALL_BUF_SIZE, APP_SI_SMALL_BUF_SIZE_DEF);
    GxBus_ConfigSetInt(APP_SI_SMALL_BUF_COUNT,APP_SI_SMALL_BUF_NUM);
#endif
#if PLAYBACK_SPEED_SUPPORT
    GxBus_ConfigSetInt(AUDIO_SPEED_MODE_KEY, ADVANCED_AUDIO_REMEMBER_VALUE);
#endif

    GxBus_ConfigSetInt(POWER_ON_KEY, 1);
    return 0;
}

void app_system_params_init(void)
{
	int32_t init_value;
    int32_t passwd_verify_value;
    int32_t xres = 0;
    int32_t yres = 0;

	GxBus_ConfigGetInt(POWER_ON_KEY, &init_value, POWER_ON);
    if (init_value == POWER_ON)  //first boot
	{
        app_system_create_default_config();
    }
    else
    {
        GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_SCREEN_XRES , &xres, 1920);
        GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_SCREEN_YRES , &yres, 1080);
        if( (xres != APP_THEME_XRES) || (yres != APP_THEME_YRES ))
        {
            GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_SCREEN_XRES,APP_THEME_XRES);
            GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_SCREEN_YRES,APP_THEME_YRES);
        }

        //passwd
        GxBus_ConfigGetInt(PASSWORD_VERIFY, &passwd_verify_value, PASSWD_VERIFY_OK);
        if(PASSWD_VERIFY_ERR == passwd_verify_value)
        {
            GxBus_ConfigSetInt(PASSWORD_KEY0, PASSWORD_1);
            GxBus_ConfigSetInt(PASSWORD_KEY1, PASSWORD_2);
            GxBus_ConfigSetInt(PASSWORD_KEY2, PASSWORD_3);
            GxBus_ConfigSetInt(PASSWORD_KEY3, PASSWORD_4);
#if LONG_PASSWORD_SUPPORT
            GxBus_ConfigSetInt(PASSWORD_KEY4, PASSWORD_5);
            GxBus_ConfigSetInt(PASSWORD_KEY5, PASSWORD_6);
#endif
        }
    }
#if BOOT_AD_SUPPORT
	int ad_sel = 0;
	GxBus_ConfigGetInt(WIDGET_TEXT_VIDEO_AD_KEY, &ad_sel, WIDGET_TEXT_VIDEO_AD_DEFAULT_KEY);
	if (1 == ad_sel)
	{
		GxBus_ConfigSetInt(PLAYER_CONFIG_AUTOPLAY_PLAY_FLAG, 1);
	}
	else
	{
		GxBus_ConfigSetInt(PLAYER_CONFIG_AUTOPLAY_PLAY_FLAG, 0);
	}
#endif
#if PVR_STANDBY_SUPPORT
    extern void app_load_pvr_standby_flag(void);
    app_load_pvr_standby_flag();
#endif
}

// set the value that the same with player service
void app_system_service_init(void)
{
	int32_t init_value;
 	int32_t chroma;
	int32_t brightness;
	int32_t contrast;
	int32_t sharpness;
	int32_t hue;

	GxMsgProperty_PlayerVideoAutoAdapt auto_adapt = {0};
	GUI_Rect region_size = {0, 0, APP_THEME_XRES, APP_THEME_YRES};
	GxMsgProperty_PlayerVideoInterface Interface;
	GxMsgProperty_PlayerVideoModeConfig  mode_config;

	// 4:3  16:9, for screen size
	GxBus_ConfigGetInt(TV_RATIO_KEY, &init_value, TV_RATIO);
	system_setting_set_screen_ratio(init_value);

	GxBus_ConfigGetInt(TV_ADAPT_KEY, &init_value, TV_ADAPT_NOAUTO);
	auto_adapt.enable = init_value;
	auto_adapt.pal = GX_VIDEO_OUTPUT_PAL;
	auto_adapt.ntsc = GX_VIDEO_OUTPUT_NTSC_M;
	app_send_msg_exec(GXMSG_PLAYER_VIDEO_AUTO_ADAPT,&auto_adapt);

	//interface HDMI & RCA
#ifdef SUPPORT_SCART
    GxBus_ConfigGetInt(VIDEO_INTERFACE_KEY, &Interface, VIDEO_INTERFACE_SCART);
#else
	GxBus_ConfigGetInt(VIDEO_INTERFACE_KEY, &Interface, VIDEO_INTERFACE_MODE);
#endif
	app_send_msg_exec(GXMSG_PLAYER_VIDEO_INTERFACE, &Interface);

    //standard
	GxBus_ConfigGetInt(TV_STANDARD_KEY, &init_value, TV_STANDARD);
    //HDMI
	mode_config.interface = GX_VIDEO_OUTPUT_HDMI;
	mode_config.mode = init_value;
	app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG,&mode_config);

#ifdef SUPPORT_SCART
	GxBus_ConfigGetInt(TV_SCART_KEY, &init_value,TV_SCART);
	app_gpio_set_scart_rgb(init_value);
#endif

    //RCA
    mode_config.interface = GX_VIDEO_OUTPUT_RCA;
	mode_config.mode = app_av_get_cvbs_standard(init_value);
	if(GX_VIDEO_OUTPUT_MODE_MAX != mode_config.mode)
		app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG,&mode_config);

	// Letter / Pilar box  pan scan
	GxBus_ConfigGetInt(ASPECT_MODE_KEY, &init_value, ASPECT_MODE);
	system_setting_set_screen_type(init_value);

	//saturation brightness contrast
	GxBus_ConfigGetInt(SATURATION_KEY, &chroma, SATURATION);
	GxBus_ConfigGetInt(BRIGHTNESS_KEY, &brightness, BRIGHTNESS);
	GxBus_ConfigGetInt(CONTRAST_KEY, &contrast, CONTRAST);
	GxBus_ConfigGetInt(SHARPNESS_KEY, &sharpness, SHARPNESS);
	GxBus_ConfigGetInt(HUE_KEY, &hue, HUE);
	system_setting_set_screen_color(chroma, brightness, contrast,
            sharpness, hue, false);

	// freezon
	GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
	app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);

	GxBus_ConfigGetInt(AUDIO_VOLUME_KEY, &init_value, AUDIO_VOLUME);
	app_system_vol_set(init_value);

	//this is spp region size
	GUI_SetInterface("image_size", &region_size);
	// this is osd region size
	GUI_SetInterface("interface_size", &region_size);

	GxBus_ConfigGetInt(OSD_LANG_KEY, &init_value, OSD_LANG);
	GUI_SetInterface("osd_language", g_LangName[init_value]);
#if (UI_MIRROR_SUPPORT > 0)
            app_menu_mirror_set(g_LangName[init_value]);
#endif

	GxBus_ConfigGetInt(PVR_FILE_SIZE_KEY, &init_value, PVR_FILE_SIZE_VALUE);
	system_setting_set_full_gate(init_value);

	GxBus_ConfigGetInt(TRANS_LEVEL_KEY, &init_value, TRANS_LEVEL);
	GUI_SetInterface("osd_alpha_global", &init_value);
}

void default_program_load(void)
{
	if(GXCORE_FILE_EXIST == GxCore_FileExists(DEFAULT_DB_PATH))
		GxBus_PmDbaseInit(SYS_MAX_SAT, SYS_MAX_TP, SYS_MAX_PROG, (uint8_t *)DEFAULT_DB_PATH);
	else
		GxBus_PmDbaseInit(SYS_MAX_SAT, SYS_MAX_TP, SYS_MAX_PROG, NULL);
#if NET_SEARCH_SUPPORT
	GxNetPm_ModuleRegister();
	GxBus_PmNetUrlLoad();
#endif

}

/* Exported functions ----------------------------------------------------- */
#if !OTT_SUPPORT
void app_frontend_init(void)
{
	AppFrontend_Open open;
	unsigned char i = 0;
	enum dmx_input ts_src_list[APP_MULTI_DEMOD_NUM] = APP_FRONTEND_DEMUX_TS_SRC;
	unsigned char demod_type_list[APP_MULTI_DEMOD_NUM] = APP_FRONTEND_MODE_CONFIG;
#if DEMOD_DVB_T || DEMOD_ISDBT
	unsigned int mode = 0;
#endif

	for(i=0; i<APP_MULTI_DEMOD_NUM; i++){
		open.tuner = i;
		open.ts_src = ts_src_list[i];
#if INDEPEND_RECORD
		open.dmx_id = i;
#else
		open.dmx_id = 0;
#endif
		app_mod_init(APP_FRONTEND_BASE, open.tuner, demod_type_list[i]);
		app_ioctl(open.tuner, FRONTEND_OPEN, &open);
#if DEMOD_DVB_T
		if(demod_type_list[i] == DEMOD_TYPE_DVBT){
			if(app_get_country_type() == COUNTRY_TYPE_INA){//for 322712
				GxFrontend_GetFrontendTuneMode(i, &mode);
				GxFrontend_SetFrontendTuneMode(i, mode|FE_INA_SEARCH);
			}
		}
#endif
	}

#if FM_SUPPORT
	open.tuner = i;
	open.ts_src = DEMUX_TS1;
	open.dmx_id = 0;
	app_mod_init(APP_FRONTEND_BASE, open.tuner, DEMOD_TYPE_FM);
	app_ioctl(open.tuner, FRONTEND_OPEN, &open);
    i++;
#endif

#if NET_SEARCH_SUPPORT
	GxFrontend_RegisterNet(i, PLAYER_FOR_NORMAL);
	open.tuner = i;
	open.ts_src = 3;
	open.dmx_id = 0;
	app_mod_init(APP_FRONTEND_BASE, open.tuner, DEMOD_TYPE_NET);
	app_ioctl(open.tuner, FRONTEND_OPEN, &open);
#endif
	// start frontend service
    //app_all_frontend_monitor_control(FRONTEND_MONITOR_ON, false);
}
#endif

#if NETWORK_SUPPORT
//reverse etc/preS.d/S02defdata
static void _system_reset_defdata(void)
{
#ifdef LINUX_OS
    system("rm /home/gx/etc -rf");
    system("rm /home/gx/local -rf");
    system("rm /home/gx/sbin -rf");
    system("rm /home/gx/bin -rf");
    system("rm /home/gx/camconf -rf");
    system("rm /home/gx/plugincfg -rf");
    system("rm /home/gx/plugin_env -rf");
#endif
}
#endif

void app_close_irr(void)
{
    int fd = 0;
	fd = open("/dev/gxirr0", O_RDWR);
	ioctl(fd, 0x4, NULL);
    close(fd);
}

void app_open_irr(void)
{
    int fd = 0;
	fd = open("/dev/gxirr0", O_RDWR);
	ioctl(fd, 0x5, NULL);
    close(fd);
}

void app_system_reset(void)
{
    uint32_t led_lock_state = 0;

    // close irr
    app_close_irr();
#if NETWORK_SUPPORT
	app_send_msg_exec(GXMSG_NETWORK_STOP, NULL);
#ifdef LINUX_OS
    system("rm /tmp/ethernet -rf");
#else

#if WIFI_SUPPORT
    if(GxCore_FileExists("/home/gx/ap_psk_record") == GXCORE_FILE_EXIST)
        GxCore_FileDelete("/home/gx/ap_psk_record");
#endif
#if MOD_3G_SUPPORT
    if(GxCore_FileExists("/home/gx/3g_connect_record") == GXCORE_FILE_EXIST)
        GxCore_FileDelete("/home/gx/3g_connect_record");
#endif
#if VPN_SUPPORT
    if(GxCore_FileExists("/home/gx/openvpn") == GXCORE_FILE_EXIST)
    {
        GxCore_Rmdir("/home/gx/openvpn");
    }
#endif
#if NETAPPS_SUPPORT
    app_iptv_factory_reset();
#endif
#endif
    _system_reset_defdata();
#endif

#if LEARN_RCU_SUPPORT
extern void app_remote_learn_rcu_result_remove(void);
    app_remote_learn_rcu_result_remove();
#endif

#if (DEMOD_DVB_S > 0)
    app_sat_list_del_sel_status();
	app_sat_list_close_lnb();
#endif
#if AUTO_STANDBY_SUPPORT
    app_init_sleep_set();
#endif
#if FM_SUPPORT
    if(GxCore_FileExists(FM_FREQ_DATA) == GXCORE_FILE_EXIST)
        GxCore_FileDelete(FM_FREQ_DATA);
    if(GxCore_FileExists(FM_USER_FREQ_NAME_LIST_PATH) == GXCORE_FILE_EXIST)
        GxCore_FileDelete(FM_USER_FREQ_NAME_LIST_PATH);
#endif
    //app_pvr_rec_stop();
    // program reset
    app_send_msg_exec(GXMSG_PM_LOAD_DEFAULT, NULL);

    app_fuse_spec_system_reset();

    // play status init
    g_AppPlayOps.program_play_init();

    // book reset
	app_send_msg_exec(GXMSG_BOOK_RESET, NULL);

    // config ini reset
    GxBus_ConfigLoadDefault();
    app_system_create_default_config();
    pmpset_factory_default();
	app_system_service_init();

#if BOUQUET_SUPPORT
	app_bouquet_clear_all();
#endif

	if (g_AppFullArb.state.mute == STATE_ON)
	{
		GxMsgProperty_PlayerAudioMute player_mute = 0;
		app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
	}

	g_AppFullArb.init(&g_AppFullArb);
	#if TKGS_SUPPORT
       GxCore_ThreadDelay(20);
        #endif
	g_AppTime.stop(&g_AppTime);

    app_panel_show_prog_num(PANEL_CLEAN_PROG_NUM);
	app_panel_ioctl_handle(PANEL_SHOW_LOCK, &led_lock_state);
	#if TKGS_SUPPORT
    app_tkgs_init_operatemode();

	//给默认数据库中的节目指定lcn
	//app_tkg_lcn_flush();
	//app_tkgs_sort_channels_by_lcn();

	app_tkgs_set_upgrade_mode(TKGS_UPGRADE_MODE_FACTORY);
	app_tkgs_upgrade_process_menu_exec();
	#endif

    g_AppTime.sync(&g_AppTime, SYS_DEFAULT_TIME_UTC);
extern int app_time_store_reset(void);
	app_time_store_reset();
#if VPN_SUPPORT
	extern void vpn_factory_reset(void);
	vpn_factory_reset();
#endif
}

/* End of file -------------------------------------------------------------*/

