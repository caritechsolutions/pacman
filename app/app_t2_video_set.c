#include "app.h"
#include "app_default_params.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "sys_setting_manage.h"
#include "app_windows.h"
#include "app_config.h"

#define T2_SUPPORT_1080P_OUTPUT
#define RESOLUTION_MAX_NUM	5
#define VIDEO_CHANGE_POP_DIAG
#ifdef VIDEO_CHANGE_POP_DIAG
#define VIDEO_CHANGE_POP_SEC (15)
#endif
#undef DEBUG_T2_AVSETTING
#ifdef DEBUG_T2_AVSETTING
#define DEBUG_AVSETTING app_log_debug
#else
#define DEBUG_AVSETTING(...)   ((void)0)
#endif

extern VideoOutputMode app_av_get_cvbs_standard(VideoOutputMode vout_mode);
extern void app_main_menu_listview_update(void);

typedef enum
{
    VIDEO_V_FORMAT_PAL,
    VIDEO_V_FORMAT_NTSC,
    VIDEO_V_FORMAT_TOTAL
} WinVFormat;

typedef enum
{
    ITEM_VIDEO_RF_CH_STEP,
    ITEM_VIDEO_RF_CH_NO,
    ITEM_VIDEO_RF_SOUND_MOD,
    ITEM_VIDEO_RF_MAX
} RfSearch;

typedef enum
{
    ITEM_VIDEO_OSD_TRANS,
    ITEM_VIDEO_OSD_BRIGHTNESS,
    ITEM_VIDEO_OSD_SATURATION,
    ITEM_VIDEO_OSD_CONTRAST,
    ITEM_VIDEO_OSD_TIMEOUT,
    ITEM_VIDEO_OSD_MAX,
} OSDSetting;

typedef enum
{
    VIDEO_OUTPUT_AUTO,
    VIDEO_OUTPUT_HDMI,
    VIDEO_OUTPUT_TOTAL
}VideoOutMode;

#ifdef SUPPORT_SCART
typedef enum
{
    VIDEO_SCART_RGB,
    VIDEO_SCART_CVBS,
    VIDEO_SCART_TOTAL
} ScartSel;
#endif
typedef enum
{
    ITEM_LED_BRIGTHNESS,
    ITEM_LED_DISPLAY_MODE,

    ITEM_LED_DISPLAY_MAX
} PanelSetting;

typedef enum
{
#if CVBS_TEST_SUPPORT
    VIDEO_ASPECT_RATIO_AUTO = 0,
#else
    VIDEO_ASPECT_RATIO_AUTO = 0,
    VIDEO_ASPECT_RATIO_16X9_WIDE_SCREEN,
    VIDEO_ASPECT_RATIO_16X9_PILLAR_BOX,
    VIDEO_ASPECT_RATIO_16X9_PAN_SCAN,
    VIDEO_ASPECT_RATIO_4X3_LETTER_BOX,
    VIDEO_ASPECT_RATIO_4X3_PAN_SCAN,
    VIDEO_ASPECT_RATIO_4X3_FULL,
#endif
    VIDEO_ASPECT_RATIO_TOTAL
} AspectRatioSel;

typedef enum
{
    OSD_TIMEOUT_3S,
    OSD_TIMEOUT_5S,
    OSD_TIMEOUT_8S,
    OSD_TIMEOUT_MAX
} OsdTimeOutSel;
typedef enum
{
    PANEL_LED_BRIGHTNESS_LOW,
    PANEL_LED_BRIGHTNESS_MOD,
    PANEL_LED_BRIGHTNESS_HIG,
    PANEL_LED_BRIGHTNESS_MAX
} LedBrightnessSel;
typedef enum
{
    LED_DISPLAY_CHNO = 0, /*LED Display*/
    LED_DISPLAY_TIME,
    LED_DISPLAY_MAX
} Led_State_t;

static  uint32_t s_resolution[VIDEO_V_FORMAT_TOTAL][RESOLUTION_MAX_NUM] =
{
    {
        GX_VIDEO_OUTPUT_576I,
        GX_VIDEO_OUTPUT_576P,
        GX_VIDEO_OUTPUT_720P_50HZ,
        GX_VIDEO_OUTPUT_1080I_50HZ,
#ifdef T2_SUPPORT_1080P_OUTPUT
        GX_VIDEO_OUTPUT_1080P_50HZ,
#endif
    },
    {
        GX_VIDEO_OUTPUT_480I,
        GX_VIDEO_OUTPUT_480P,
        GX_VIDEO_OUTPUT_720P_60HZ,
        GX_VIDEO_OUTPUT_1080I_60HZ,
#ifdef T2_SUPPORT_1080P_OUTPUT
        GX_VIDEO_OUTPUT_1080P_60HZ
#endif
    }
};

#if HIGH_DYNAMIC_RANGE_SUPPORT
//If the driver is updated, this content also needs to be updated. kk Label:expand
typedef enum
{
    APP_VIDEO_HDR_MODE_HDR,
    APP_VIDEO_HDR_MODE_SDR,
    APP_VIDEO_HDR_MODE_TOTAL
} App_video_hdr_mode_Sel;
#endif

static uint32_t s_tv_format_mode[VIDEO_V_FORMAT_TOTAL] = {GX_VIDEO_OUTPUT_PAL, GX_VIDEO_OUTPUT_NTSC_M};
static char *s_tv_format_content[VIDEO_V_FORMAT_TOTAL] = {"PAL", "NTSC"};
static char *s_videooutput_content[VIDEO_OUTPUT_TOTAL] = {"HDMI and CVBS","HDMI or CVBS"};
#ifdef SUPPORT_SCART
static char *s_video_scart_content[VIDEO_SCART_TOTAL] = {"CVBS","RGB"};
#endif
static char *s_resolution_content[VIDEO_V_FORMAT_TOTAL][RESOLUTION_MAX_NUM] =
{
    {"576i", "576p", "720p", "1080i", "1080p"},
    {"480i", "480p", "720p", "1080i", "1080p"}
};

static char *s_aspect_ratio_content[VIDEO_ASPECT_RATIO_TOTAL] =
{
#if CVBS_TEST_SUPPORT
    "Raw"
#else
    "Auto", "16:9 Wide Screen", "16:9 Pillar Box", "16:9 Pan Scan",
    "4:3 Letter Box", "4:3 Pan Scan", "4:3 Full"
#endif
};

#if HIGH_DYNAMIC_RANGE_SUPPORT
static char *s_video_hdr_mode_content[APP_VIDEO_HDR_MODE_TOTAL] = {"Enhanced","Off"};
static int   s_video_hdr_mode_sel = APP_VIDEO_HDR_MODE_HDR;
#endif

static int s_resolution_sel = 0;
static int t2_ratio_sel = TV_RATIO;
static int s_aspect_ratio_sel = ASPECT_MODE;
static int t2_aspect_mode_sel = ASPECT_MODE;
static int s_tv_format_sel = 0;
static int32_t s_videooutput_sel = 0;
#ifdef SUPPORT_SCART
static int s_video_scart_sel = 0;
#endif

static void app_video_aspect_ratio_set_para(int *VideoAspect, int *VideoRatio)
{
    int32_t RatioValue = 0;
    int32_t AspectValue = 0;
    GxBus_ConfigGetInt(TV_RATIO_KEY, &RatioValue, TV_RATIO);
    GxBus_ConfigGetInt(ASPECT_MODE_KEY, &AspectValue, ASPECT_MODE);
    switch (s_aspect_ratio_sel)
    {
#if CVBS_TEST_SUPPORT
        case VIDEO_ASPECT_RATIO_AUTO:
        {
            AspectValue = ASPECT_RAW_SIZE;
        }
        break;
#else
        case VIDEO_ASPECT_RATIO_AUTO:
        {
            AspectValue = ASPECT_NORMAL;
        }
        break;
        case VIDEO_ASPECT_RATIO_16X9_WIDE_SCREEN:
        {
            AspectValue = ASPECT_16X9;
            RatioValue = DISPLAY_SCREEN_16X9;
        }
        break;
        case VIDEO_ASPECT_RATIO_16X9_PILLAR_BOX:
        {
            AspectValue = ASPECT_LETTER_BOX;
            RatioValue = DISPLAY_SCREEN_16X9;
        }
        break;
        case VIDEO_ASPECT_RATIO_16X9_PAN_SCAN:
        {
            AspectValue = ASPECT_PAN_SCAN;
            RatioValue = DISPLAY_SCREEN_16X9;
        }
        break;
        case VIDEO_ASPECT_RATIO_4X3_LETTER_BOX:
        {
            AspectValue = ASPECT_LETTER_BOX;
            RatioValue = DISPLAY_SCREEN_4X3;
        }
        break;
        case VIDEO_ASPECT_RATIO_4X3_PAN_SCAN:
        {
            AspectValue = ASPECT_PAN_SCAN;
            RatioValue = DISPLAY_SCREEN_4X3;
        }
        break;
        case VIDEO_ASPECT_RATIO_4X3_FULL:
        {
            AspectValue = ASPECT_4X3;
            RatioValue = DISPLAY_SCREEN_4X3;
        }
        break;
#endif
        default:
            break;
    }
    *VideoAspect = AspectValue;
    *VideoRatio = RatioValue;

#ifdef SUPPORT_SCART
    if (RatioValue == DISPLAY_SCREEN_4X3)
    {
        app_gpio_set_scart_onoff(0);
        app_gpio_set_scart_tv(0);
    }
    else if (RatioValue == DISPLAY_SCREEN_16X9)
    {
        app_gpio_set_scart_onoff(0);
        app_gpio_set_scart_tv(1);
    }
#endif
}

static void app_video_aspect_ratio_get_para(void)
{
    int32_t VideoAspect, VideoRatio;
    GxBus_ConfigGetInt(TV_RATIO_KEY, &VideoRatio, TV_RATIO);
    GxBus_ConfigGetInt(ASPECT_MODE_KEY, &VideoAspect, ASPECT_MODE);
    if (VideoAspect == ASPECT_RATIO_NOMAL) //for auto
    {
        s_aspect_ratio_sel = VIDEO_ASPECT_RATIO_AUTO;
    }
    else
    {
#if CVBS_TEST_SUPPORT
        s_aspect_ratio_sel = VIDEO_ASPECT_RATIO_AUTO;
#else
        switch (VideoRatio)
        {
            case DISPLAY_SCREEN_4X3:
            {
                switch (VideoAspect)
                {
                    case ASPECT_RATIO_LETTER_BOX://VIDEO_ASPECT_RATIO_4X3_LETTER_BOX
                    {
                        s_aspect_ratio_sel = VIDEO_ASPECT_RATIO_4X3_LETTER_BOX;
                    }
                    break;
                    case ASPECT_RATIO_PAN_SCAN:
                    {
                        s_aspect_ratio_sel = VIDEO_ASPECT_RATIO_4X3_PAN_SCAN;
                    }
                    break;
                    case ASPECT_4X3:
                    {
                        s_aspect_ratio_sel = VIDEO_ASPECT_RATIO_4X3_FULL;
                    }
                    break;
                    default:
                    {
                        s_aspect_ratio_sel = VIDEO_ASPECT_RATIO_AUTO;
                    }
                    break;
                }
            }
            break;
            case DISPLAY_SCREEN_16X9:
            {
                switch (VideoAspect)
                {
                    case ASPECT_RATIO_LETTER_BOX://VIDEO_ASPECT_RATIO_16X9_PILLAR_BOX
                    {
                        s_aspect_ratio_sel = VIDEO_ASPECT_RATIO_16X9_PILLAR_BOX;
                    }
                    break;
                    case ASPECT_RATIO_PAN_SCAN:
                    {
                        s_aspect_ratio_sel = VIDEO_ASPECT_RATIO_16X9_PAN_SCAN;
                    }
                    break;
                    case ASPECT_RATIO_16X9://
                    {
                        s_aspect_ratio_sel = VIDEO_ASPECT_RATIO_16X9_WIDE_SCREEN;
                    }
                    break;
                    default:
                    {
                        s_aspect_ratio_sel = VIDEO_ASPECT_RATIO_AUTO;
                    }
                    break;
                }
            }
            default:
                break;
        }
#endif
    }
}

static void app_video_get_tv_format_sel(VideoOutputMode CurTvFormat)
{
    switch (CurTvFormat)
    {
        case GX_VIDEO_OUTPUT_PAL:
        {
            s_tv_format_sel = VIDEO_V_FORMAT_PAL;
        }
        break;
        case GX_VIDEO_OUTPUT_NTSC_M:
        {
            s_tv_format_sel = VIDEO_V_FORMAT_NTSC;
        }
        break;
        default:
            break;
    }
}

static void app_video_get_resolution_sel(VideoOutputMode CurResolution)
{
    int i = 0;
    s_resolution_sel = i;
    for (i = 0; i < RESOLUTION_MAX_NUM; i++)
    {
        if (CurResolution == s_resolution[s_tv_format_sel][i])
        {
            s_resolution_sel = i;
            break;
        }
    }
}

static void app_t2_video_reset_resolution(VideoOutputMode ResolutionMode)
{
    int32_t 		init_value;
    GxMsgProperty_PlayerVideoModeConfig video_mode;
    GxMsgProperty_PlayerVideoAutoAdapt display_adapt;
    GxBus_ConfigGetInt(TV_ADAPT_KEY, &init_value, TV_ADAPT_NOAUTO);
    display_adapt.enable = init_value;//TV_ADAPT_AUTO;//set outo
    display_adapt.pal = GX_VIDEO_OUTPUT_PAL;
    display_adapt.ntsc = GX_VIDEO_OUTPUT_NTSC_M;
    GxBus_ConfigSetInt(TV_ADAPT_KEY, display_adapt.enable);

    app_send_msg_exec(GXMSG_PLAYER_VIDEO_AUTO_ADAPT, &display_adapt);
    if (ResolutionMode < GX_VIDEO_OUTPUT_PAL || ResolutionMode >= GX_VIDEO_OUTPUT_MODE_MAX)
        ResolutionMode = 0;
    if (init_value == TV_ADAPT_AUTO)	//auto
    {
        video_mode.interface = GX_VIDEO_OUTPUT_HDMI;
        video_mode.mode = ResolutionMode;
        app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG, &video_mode);

        video_mode.interface = GX_VIDEO_OUTPUT_RCA;
        video_mode.mode = GX_VIDEO_OUTPUT_PAL;//
        app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG, &video_mode);

        video_mode.interface = GX_VIDEO_OUTPUT_YUV;
        video_mode.mode = GX_VIDEO_OUTPUT_576I;//
        app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG, &video_mode);
    }
    else
    {
        //  GxPlayer_SetVideoOutputPower(0);

        video_mode.interface = GX_VIDEO_OUTPUT_HDMI;
        video_mode.mode = ResolutionMode;
        app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG, &video_mode);

        video_mode.interface = GX_VIDEO_OUTPUT_RCA;
        video_mode.mode = app_av_get_cvbs_standard(ResolutionMode);

       // GxBus_ConfigSetInt(TV_FORMAT_KEY, video_mode.mode);

        //GxBus_ConfigSetInt(TV_STANDARD_KEY,video_mode.mode);
        if (GX_VIDEO_OUTPUT_MODE_MAX != video_mode.mode)
            app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG, &video_mode);

        //  GxPlayer_SetVideoOutputPower(1);

        //GxBus_ConfigGetInt(TV_FORMAT_KEY, &init_value, GX_VIDEO_OUTPUT_PAL);
    }
#if CVBS_TEST_SUPPORT
    {
        extern void app_cvbs_index_test_para_init(void);
        if((GX_VIDEO_OUTPUT_576I == ResolutionMode)
            ||(GX_VIDEO_OUTPUT_480I == ResolutionMode)
          )
        {
            app_cvbs_index_test_para_init();
        }
    }
#endif
    init_value = 0;
    GxBus_ConfigGetInt(AV_VIDEO_OUT_MODE, &init_value, VIDEO_OUT_MODE_DEFAULT_VALUE);
    app_av_set_videoout_mode(init_value);
}

static int _app_t2_video_AspectRatio_poplist_cb(int32_t ret_sel, unsigned short key)
{
    GxMsgProperty_PlayerDisplayScreen screen_ratio;
    GxMsgProperty_PlayerVideoAspect video_aspect;

    if(s_aspect_ratio_sel != ret_sel)
    {
        s_aspect_ratio_sel = ret_sel ;
        app_video_aspect_ratio_set_para(&t2_aspect_mode_sel, &t2_ratio_sel);
        video_aspect = t2_aspect_mode_sel;

        screen_ratio.aspect = t2_ratio_sel;
        screen_ratio.xres = APP_THEME_XRES;
        screen_ratio.yres = APP_THEME_YRES;
        GxBus_ConfigSetInt(ASPECT_MODE_KEY, t2_aspect_mode_sel);
        GxBus_ConfigSetInt(TV_RATIO_KEY, t2_ratio_sel);
        app_send_msg_exec(GXMSG_PLAYER_VIDEO_ASPECT, &video_aspect);
        app_send_msg_exec(GXMSG_PLAYER_DISPLAY_SCREEN, &screen_ratio);
    }
    else
    {
        if(GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS)
            return -1;
    }

    return 0;
}

static int  app_t2_video_AspectRatio_pop(int sel)
{
    PopList pop_list;

    memset(&pop_list, 0, sizeof(PopList));
    pop_list.title = "Aspect Ratio";
    pop_list.pos.x = APP_MAINMENU_POP_LIST_X;
    pop_list.pos.y = APP_MAINMENU_POP_LIST_Y;
    pop_list.item_num = VIDEO_ASPECT_RATIO_TOTAL;
    pop_list.item_content = s_aspect_ratio_content;
    pop_list.sel = sel;
    pop_list.show_num = false;
    pop_list.mode = POP_MODE_UNBLOCK;
    pop_list.exit_cb = _app_t2_video_AspectRatio_poplist_cb;
    poplist_create(&pop_list);

    return 1;
}

static int _app_t2_video_Resolution_poplist_cb(int32_t ret_sel, unsigned short key);

static int  app_t2_video_Resolution_pop(int sel)
{
    PopList pop_list;
    int ret = -1;

    memset(&pop_list, 0, sizeof(PopList));
    pop_list.title = "Resolution";
    pop_list.pos.x = APP_MAINMENU_POP_LIST_X;
    pop_list.pos.y = APP_MAINMENU_POP_LIST_Y;
    pop_list.item_num = RESOLUTION_MAX_NUM;
    pop_list.item_content = s_resolution_content[s_tv_format_sel];
    pop_list.sel = sel;
    pop_list.show_num = false;
    pop_list.mode = POP_MODE_UNBLOCK;
    pop_list.exit_cb = _app_t2_video_Resolution_poplist_cb;
    poplist_create(&pop_list);

    return ret;
}

void app_t2_video_set_aspect_shortcuts(void)
{
    GxMsgProperty_PlayerDisplayScreen screen_ratio;
    GxMsgProperty_PlayerVideoAspect video_aspect;

    app_video_aspect_ratio_get_para();
    if (s_aspect_ratio_sel++ >= VIDEO_ASPECT_RATIO_TOTAL - 1)
    {
        s_aspect_ratio_sel = VIDEO_ASPECT_RATIO_AUTO;
    }
    app_video_aspect_ratio_set_para(&t2_aspect_mode_sel, &t2_ratio_sel);
    video_aspect = t2_aspect_mode_sel;

    screen_ratio.aspect = t2_ratio_sel;
    screen_ratio.xres = APP_THEME_XRES;
    screen_ratio.yres = APP_THEME_YRES;
    GxBus_ConfigSetInt(ASPECT_MODE_KEY, t2_aspect_mode_sel);
    GxBus_ConfigSetInt(TV_RATIO_KEY, t2_ratio_sel);
    GUI_SetProperty("txt_full_screen_shortcuts", "state", "show");
    GUI_SetProperty("txt_full_screen_shortcuts", "string", s_aspect_ratio_content[s_aspect_ratio_sel]);
    app_send_msg_exec(GXMSG_PLAYER_VIDEO_ASPECT, &video_aspect);
    app_send_msg_exec(GXMSG_PLAYER_DISPLAY_SCREEN, &screen_ratio);
}
#ifdef VIDEO_CHANGE_POP_DIAG
static int app_video_change_verify(ExitCb  exit_cb)
{
    PopDlg pop;

    popdlg_destroy();
    memset(&pop, 0, sizeof(PopDlg));
    pop.format = POP_FORMAT_DLG;
    pop.type = POP_TYPE_YES_NO;
    pop.mode = POP_MODE_UNBLOCK;
    pop.str = STR_ID_SAVE_INFO;
    pop.timeout_sec = VIDEO_CHANGE_POP_SEC;
    pop.default_ret = POP_VAL_CANCEL;
    pop.show_time = true;
    pop.exit_cb = exit_cb ;
    popdlg_create(&pop);
    return 0 ;
}
#endif

#ifdef VIDEO_CHANGE_POP_DIAG
static int _t2_video_set_resolution_popdlg_exit_cb(PopDlgRet ret)
{
    int video_standard = 0;
    int tv_format = 0;
    VideoOutputMode ResolutionMode;

    ResolutionMode = s_resolution[s_tv_format_sel][s_resolution_sel];

    if ( ret != POP_VAL_OK )
    {
        GxBus_ConfigGetInt(TV_STANDARD_KEY, &video_standard, TV_STANDARD);
        app_video_get_resolution_sel(video_standard);
        tv_format = (int)app_av_get_cvbs_standard((VideoOutputMode)video_standard);
        app_video_get_tv_format_sel(tv_format);

        app_t2_video_reset_resolution(s_resolution[s_tv_format_sel][s_resolution_sel]);
    }
    else
    {
        if (ResolutionMode < GX_VIDEO_OUTPUT_PAL || ResolutionMode >= GX_VIDEO_OUTPUT_MODE_MAX)
            ResolutionMode = 0;
        GxBus_ConfigSetInt(TV_STANDARD_KEY, (int32_t)ResolutionMode);
    }
    return 0 ;
}
#endif

static int _app_t2_video_Resolution_poplist_cb(int32_t ret_sel, unsigned short key)
{
    VideoOutputMode ResolutionMode;
#ifdef VIDEO_CHANGE_POP_DIAG
    int resolution_sel_bak = 0;
#endif

    if ( ret_sel == s_resolution_sel)
    {
        return -1 ;
    }
#ifdef VIDEO_CHANGE_POP_DIAG
    resolution_sel_bak = s_resolution_sel;
#endif
    s_resolution_sel = ret_sel ;
    ResolutionMode = s_resolution[s_tv_format_sel][s_resolution_sel];
    app_t2_video_reset_resolution(ResolutionMode);
#ifdef VIDEO_CHANGE_POP_DIAG
    if(s_resolution_sel!=resolution_sel_bak)
    {
        app_video_change_verify(_t2_video_set_resolution_popdlg_exit_cb);
    }
#else
    if (ResolutionMode < GX_VIDEO_OUTPUT_PAL || ResolutionMode >= GX_VIDEO_OUTPUT_MODE_MAX)
        ResolutionMode = 0;
    GxBus_ConfigSetInt(TV_STANDARD_KEY, (int32_t)ResolutionMode);
#endif
    return 0 ;
}


void app_t2_video_set_standard(void)
{
    int *first = NULL, end = 0;
    VideoOutputMode ResolutionMode;
#ifdef VIDEO_CHANGE_POP_DIAG
	int	resolution_sel_bak = s_resolution_sel;
#endif
    first = &s_resolution_sel;
    end = RESOLUTION_MAX_NUM;

    if ((*first) == (end - 1))
    {
        *first = 0;
    }
    else
    {
        (*first)++;
    }
    ResolutionMode = s_resolution[s_tv_format_sel][s_resolution_sel];
    app_t2_video_reset_resolution(ResolutionMode);
    DEBUG_AVSETTING("-------video_mode.mode=%d---------\n", s_resolution[s_tv_format_sel][s_resolution_sel]);
#ifdef VIDEO_CHANGE_POP_DIAG
    GUI_SetProperty("txt_full_screen_shortcuts", "state", "show");
    GUI_SetProperty("txt_full_screen_shortcuts", "string", s_resolution_content[s_tv_format_sel][s_resolution_sel]);
	if(s_resolution_sel!=resolution_sel_bak)
	{
		app_video_change_verify(_t2_video_set_resolution_popdlg_exit_cb);
	}
#else
    if (ResolutionMode < GX_VIDEO_OUTPUT_PAL || ResolutionMode >= GX_VIDEO_OUTPUT_MODE_MAX)
        ResolutionMode = 0;
    GxBus_ConfigSetInt(TV_STANDARD_KEY, (int32_t)ResolutionMode);
#endif


    return;
}

void app_t2_video_set_tv_format_shortcuts(void)
{
	app_t2_video_set_standard();
}

int app_aspect_ratio_setting_keypress(int key)
{
    int ratio_sel = ASPECT_MODE;
    int aspect_mode_sel = ASPECT_MODE;
    int ret = EVENT_TRANSFER_KEEPON;
    GxMsgProperty_PlayerDisplayScreen screen_ratio;
    GxMsgProperty_PlayerVideoAspect video_aspect;

    app_video_aspect_ratio_get_para();

    switch(key)
    {
        case STBK_LEFT:
        case STBK_RIGHT:
            if(STBK_LEFT == key)
            {
                if(s_aspect_ratio_sel > VIDEO_ASPECT_RATIO_AUTO)
                    s_aspect_ratio_sel--;
                else if(VIDEO_ASPECT_RATIO_AUTO == s_aspect_ratio_sel)
                    s_aspect_ratio_sel = VIDEO_ASPECT_RATIO_TOTAL - 1;
            }
            else
            {
                if(s_aspect_ratio_sel < VIDEO_ASPECT_RATIO_TOTAL - 1)
                    s_aspect_ratio_sel++;
                else if(VIDEO_ASPECT_RATIO_TOTAL - 1 == s_aspect_ratio_sel)
                    s_aspect_ratio_sel = VIDEO_ASPECT_RATIO_AUTO;
            }

            app_video_aspect_ratio_set_para(&aspect_mode_sel, &ratio_sel);
            video_aspect = aspect_mode_sel;

            screen_ratio.aspect = ratio_sel;
            screen_ratio.xres = APP_THEME_XRES;
            screen_ratio.yres = APP_THEME_YRES;
            GxBus_ConfigSetInt(ASPECT_MODE_KEY, aspect_mode_sel);
            GxBus_ConfigSetInt(TV_RATIO_KEY, ratio_sel);
            app_send_msg_exec(GXMSG_PLAYER_VIDEO_ASPECT, &video_aspect);
            app_send_msg_exec(GXMSG_PLAYER_DISPLAY_SCREEN, &screen_ratio);
            app_main_menu_listview_update();
            break;

        case STBK_OK:
            if(app_t2_video_AspectRatio_pop(s_aspect_ratio_sel))
                ret = EVENT_TRANSFER_STOP;
            break;

        default:
            break;
    }

    return ret;
}

char *app_aspect_ratio_setting_content_get(void)
{
    app_video_aspect_ratio_get_para();
    return s_aspect_ratio_content[s_aspect_ratio_sel];
}

int app_resolution_setting_keypress(int key)
{
    int video_standard = 0;
    int tv_format = 0;
    int ret = EVENT_TRANSFER_KEEPON;
    VideoOutputMode ResolutionMode;
#ifdef VIDEO_CHANGE_POP_DIAG
    int resolution_sel_bak = 0;
#endif

    if(STBK_LEFT != key && STBK_RIGHT != key && STBK_OK != key)
        return ret;

    GxBus_ConfigGetInt(TV_STANDARD_KEY, &video_standard, TV_STANDARD);
    app_video_get_resolution_sel(video_standard);
    tv_format = (int)app_av_get_cvbs_standard((VideoOutputMode)video_standard);
    app_video_get_tv_format_sel(tv_format);

#ifdef VIDEO_CHANGE_POP_DIAG
    resolution_sel_bak = s_resolution_sel;
#endif
    if(STBK_LEFT == key)
    {
        if(s_resolution_sel > 0)
            s_resolution_sel--;
        else if(0 == s_resolution_sel)
            s_resolution_sel = RESOLUTION_MAX_NUM - 1;
    }
    else if(STBK_RIGHT == key)
    {
        if(s_resolution_sel < RESOLUTION_MAX_NUM - 1)
            s_resolution_sel++;
        else if(RESOLUTION_MAX_NUM - 1 == s_resolution_sel)
            s_resolution_sel = 0;
    }
    else
    {
        app_t2_video_Resolution_pop(s_resolution_sel);

        ret = EVENT_TRANSFER_STOP;
        return ret ;
    }
    ResolutionMode = s_resolution[s_tv_format_sel][s_resolution_sel];
    app_t2_video_reset_resolution(ResolutionMode);
#ifdef VIDEO_CHANGE_POP_DIAG
    if(s_resolution_sel!=resolution_sel_bak)
    {
        app_video_change_verify(_t2_video_set_resolution_popdlg_exit_cb);
    }
#else
    if (ResolutionMode < GX_VIDEO_OUTPUT_PAL || ResolutionMode >= GX_VIDEO_OUTPUT_MODE_MAX)
        ResolutionMode = 0;
    GxBus_ConfigSetInt(TV_STANDARD_KEY, (int32_t)ResolutionMode);
#endif

    return ret;
}

int app_resolution_init(void)
{
    int video_standard = 0;
    int tv_format = 0;

    GxBus_ConfigGetInt(TV_STANDARD_KEY, &video_standard, TV_STANDARD);
    tv_format = (int)app_av_get_cvbs_standard((VideoOutputMode)video_standard);
    app_video_get_tv_format_sel(tv_format);
    app_video_get_resolution_sel(video_standard);

    return 0;
}

char *app_resolution_setting_content_get(void)
{
    return s_resolution_content[s_tv_format_sel][s_resolution_sel];
}

#ifdef VIDEO_CHANGE_POP_DIAG
static int _t2_video_set_tv_format_popdlg_exit_cb(PopDlgRet ret)
{
    GxMsgProperty_PlayerVideoModeConfig video_mode;
    int video_standard = 0;
    int tv_format = 0;
    VideoOutputMode ResolutionMode;

    ResolutionMode = s_resolution[s_tv_format_sel][s_resolution_sel];

    if ( ret != POP_VAL_OK )
    {
        GxBus_ConfigGetInt(TV_STANDARD_KEY, &video_standard, TV_STANDARD);
        tv_format = (int)app_av_get_cvbs_standard((VideoOutputMode)video_standard);
        app_video_get_tv_format_sel(tv_format);
        app_video_get_resolution_sel(video_standard);

        app_t2_video_reset_resolution(s_resolution[s_tv_format_sel][s_resolution_sel]);
        video_mode.interface = GX_VIDEO_OUTPUT_RCA;
        video_mode.mode = s_tv_format_mode[s_tv_format_sel];
        app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG, &video_mode);
    }
    else
    {
        if (ResolutionMode < GX_VIDEO_OUTPUT_PAL || ResolutionMode >= GX_VIDEO_OUTPUT_MODE_MAX)
            ResolutionMode = 0;
        GxBus_ConfigSetInt(TV_STANDARD_KEY, (int32_t)ResolutionMode);
    }
    return 0 ;
}
#endif

int app_tv_format_setting_keypress(int key)
{
    GxMsgProperty_PlayerVideoModeConfig video_mode;
    int video_standard = 0;
    int tv_format = 0;
    VideoOutputMode ResolutionMode;
#ifdef VIDEO_CHANGE_POP_DIAG
    int format_sel_bak = 0;
#endif

    if(key != STBK_LEFT && key != STBK_RIGHT)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(TV_STANDARD_KEY, &video_standard, TV_STANDARD);
    app_video_get_resolution_sel(video_standard);
    tv_format = (int)app_av_get_cvbs_standard((VideoOutputMode)video_standard);
    app_video_get_tv_format_sel(tv_format);

#ifdef VIDEO_CHANGE_POP_DIAG
    format_sel_bak = s_tv_format_sel;
#endif
    if(STBK_LEFT == key)
    {
        if(s_tv_format_sel > 0)
            s_tv_format_sel--;
        else if(0 == s_tv_format_sel)
            s_tv_format_sel = VIDEO_V_FORMAT_TOTAL - 1;
    }
    else
    {
        if(s_tv_format_sel < VIDEO_V_FORMAT_TOTAL - 1)
            s_tv_format_sel++;
        else if(VIDEO_V_FORMAT_TOTAL - 1 == s_tv_format_sel)
            s_tv_format_sel = 0;
    }

    ResolutionMode = s_resolution[s_tv_format_sel][s_resolution_sel];
    app_t2_video_reset_resolution(ResolutionMode);
    video_mode.interface = GX_VIDEO_OUTPUT_RCA;
    video_mode.mode = s_tv_format_mode[s_tv_format_sel];
    app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG, &video_mode);
    app_main_menu_listview_update();

#ifdef VIDEO_CHANGE_POP_DIAG
    if(s_tv_format_sel!=format_sel_bak)
    {
        app_video_change_verify(_t2_video_set_tv_format_popdlg_exit_cb);
    }
#else
    if (ResolutionMode < GX_VIDEO_OUTPUT_PAL || ResolutionMode >= GX_VIDEO_OUTPUT_MODE_MAX)
        ResolutionMode = 0;
    GxBus_ConfigSetInt(TV_STANDARD_KEY, (int32_t)ResolutionMode);
#endif

    return EVENT_TRANSFER_KEEPON;
}

char *app_tv_format_setting_content_get(void)
{
    int video_standard = 0;
    int tv_format = 0;
    video_standard = (int32_t)s_resolution[s_tv_format_sel][s_resolution_sel];
    app_video_get_resolution_sel(video_standard);
    tv_format = (int)app_av_get_cvbs_standard((VideoOutputMode)video_standard);
    app_video_get_tv_format_sel(tv_format);

    return s_tv_format_content[s_tv_format_sel];
}

int app_videooutput_setting_keypress(int key)
{
    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(AV_VIDEO_OUT_MODE, &s_videooutput_sel, VIDEO_OUT_MODE_DEFAULT_VALUE);
    if(STBK_LEFT == key)
    {
        if(s_videooutput_sel > 0)
            s_videooutput_sel--;
        else if(0 == s_videooutput_sel)
            s_videooutput_sel = VIDEO_OUTPUT_TOTAL - 1;

    }
    else
    {
        if(s_videooutput_sel < VIDEO_OUTPUT_TOTAL - 1)
            s_videooutput_sel++;
        else if(VIDEO_OUTPUT_TOTAL - 1 == s_videooutput_sel)
            s_videooutput_sel = 0;
    }
    app_av_set_videoout_mode(s_videooutput_sel);
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_videooutpu_setting_content_get(void)
{
    GxBus_ConfigGetInt(AV_VIDEO_OUT_MODE, &s_videooutput_sel, VIDEO_OUT_MODE_DEFAULT_VALUE);
    return s_videooutput_content[s_videooutput_sel];
}

#ifdef SUPPORT_SCART
int app_scart_setting_keypress(int key)
{
    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(TV_SCART_KEY, &s_video_scart_sel, TV_SCART);

    if(STBK_LEFT == key)
    {
        if(s_video_scart_sel > 0)
            s_video_scart_sel--;
        else if(0 == s_video_scart_sel)
            s_video_scart_sel = VIDEO_SCART_TOTAL - 1;

    }
    else
    {
        if(s_video_scart_sel < VIDEO_SCART_TOTAL - 1)
            s_video_scart_sel++;
        else if(VIDEO_SCART_TOTAL - 1 == s_video_scart_sel)
            s_video_scart_sel = 0;
    }

    GxBus_ConfigSetInt(TV_SCART_KEY, s_video_scart_sel);
    app_gpio_set_scart_rgb(s_video_scart_sel);
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_scart_setting_content_get(void)
{
    GxBus_ConfigGetInt(TV_SCART_KEY, &s_video_scart_sel, TV_SCART);
    return s_video_scart_content[s_video_scart_sel];
}
#endif

#if HIGH_DYNAMIC_RANGE_SUPPORT

static App_video_hdr_mode_Sel _hdr_player_para_convert_app(VideoDrcMode mode)
{
    App_video_hdr_mode_Sel sel = APP_VIDEO_HDR_MODE_HDR;

    if(GX_VDRC_BYPASS == mode)
        sel = APP_VIDEO_HDR_MODE_SDR;
    else if(GX_VDRC_TOSDR == mode)
        sel = APP_VIDEO_HDR_MODE_HDR;
    else if(GX_VDRC_AUTO == mode)
        app_log_error("App not support yet\n");

    return sel;
}

static VideoDrcMode _hdr_app_para_convert_player(App_video_hdr_mode_Sel sel)
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

int app_hdr_mode_setting_keypress(int key)
{
    int hdr_mode = 0;

    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_DRCMODE, &hdr_mode, _hdr_app_para_convert_player(APP_VIDEO_HDR_MODE_HDR));
    s_video_hdr_mode_sel = _hdr_player_para_convert_app(hdr_mode);
    //printf("#################### get hdr mode = %d \n",s_video_hdr_mode_sel);

    if(STBK_LEFT == key)
    {
        if(s_video_hdr_mode_sel > 0)
            s_video_hdr_mode_sel--;
        else if(0 == s_video_hdr_mode_sel)
            s_video_hdr_mode_sel = APP_VIDEO_HDR_MODE_TOTAL - 1;

    }
    else
    {
        if(s_video_hdr_mode_sel < (APP_VIDEO_HDR_MODE_TOTAL - 1))
            s_video_hdr_mode_sel++;
        else if((APP_VIDEO_HDR_MODE_TOTAL - 1) == s_video_hdr_mode_sel)
            s_video_hdr_mode_sel = 0;
    }

    GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_DRCMODE, _hdr_app_para_convert_player(s_video_hdr_mode_sel));
    GxPlayer_SetVideoDrcMode(_hdr_app_para_convert_player(s_video_hdr_mode_sel));
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_hdr_mode_setting_content_get(void)
{
    int hdr_mode = 0;

    GxBus_ConfigGetInt(PLAYER_CONFIG_VIDEO_DRCMODE, &hdr_mode, _hdr_app_para_convert_player(APP_VIDEO_HDR_MODE_HDR));
    s_video_hdr_mode_sel = _hdr_player_para_convert_app(hdr_mode);
    //printf("################## get hdr mode str , mode = %d \n",s_video_hdr_mode_sel);
    if ( (s_video_hdr_mode_sel < APP_VIDEO_HDR_MODE_HDR) || (s_video_hdr_mode_sel >= APP_VIDEO_HDR_MODE_TOTAL) )
    {
        //printf("Config Get Int PLAYER_CONFIG_VIDEO_DRCMODE = %d \n",s_video_hdr_mode_sel);
        s_video_hdr_mode_sel = APP_VIDEO_HDR_MODE_HDR ;
    }
    return s_video_hdr_mode_content[s_video_hdr_mode_sel];
}

int app_hdr_mode_setting_check(void)
{
    extern int app_get_hdr_enable(void);
    return app_get_hdr_enable();
}

#endif
