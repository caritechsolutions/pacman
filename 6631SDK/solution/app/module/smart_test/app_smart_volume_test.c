
#include "app.h"
#include "app_module.h"
#include "app_utility.h"
#include "app_send_msg.h"
#include "app_pop.h"
#include <stdio.h>
#include <fcntl.h>
#include <math.h>
#include "app_default_params.h"
#include "app_wnd_system_setting_opt.h"

#include "full_screen.h"
#if SMART_VOLUME_TEST_SUPPORT
#define EDIT_MODE_SUPPRT 1

enum SAT_LIST_POPUP_BOX
{
    SMART_VOLUME_BOX_ITEM_STATUS = 0,
    SMART_VOLUME_BOX_ITEM_TIMES,
    SMART_VOLUME_BOX_ITEM_LEVEL,
    SMART_VOLUME_BOX_ITEM_TOTAL
};

typedef enum
{
	SMART_VOLUME_OFF,
	SMART_VOLUME_ON,
	SMART_VOLUME_TOTAL
}SmartVolueStatus;


typedef enum
{
    SMART_GAIN_LEVEL_0 = 0,
    SMART_GAIN_LEVEL_1 ,
    SMART_GAIN_LEVEL_2 ,
    SMART_GAIN_LEVEL_3 ,
    SMART_GAIN_LEVEL_4 ,
    SMART_GAIN_LEVEL_5 ,
    SMART_GAIN_LEVEL_6 ,
    SMART_GAIN_LEVEL_7 ,
    SMART_GAIN_LEVEL_8 ,
    SMART_GAIN_LEVEL_9 ,
    SMART_GAIN_LEVEL_10 ,
    SMART_GAIN_LEVEL_11 ,
    SMART_GAIN_LEVEL_MAX,
}SmGainLevel;

typedef struct
{
    unsigned int        sm_vol_times;/////< 起始播放，检测多长时间数据后，停止检测.
	SmartVolueStatus	sm_status;
#if EDIT_MODE_SUPPRT
	float         sm_level;//< 目标增益值，范围: 0 - 9.
#else
    SmGainLevel   sm_level;
#endif
}SmVolumePara;

#define WIN_EDIT_LOUDNESS_SET "edit_system_setting_opt3"
#define SMART_STATUS_CONTENT  "[Off,On]"
#define SMART_LEVEL_CONTENT    "[-42,-30,-28,-26,-24,-22,-16,-14,-13,-10,-8,-6]"
#define SMART_TIMES_CONTENT    "[always,5s,10s,15s,20s,25s,30s]"
#define SMART_VOLUME_MODE	VOLUME_MODE_KEY
#define SMART_MODE          0
#define MAX_CH_LOUD_LEN     10

static SystemSettingItem s_smart_item[SMART_VOLUME_BOX_ITEM_TOTAL];
static SystemSettingOpt s_smart_volume_opt;
static SmVolumePara thiz_smart_para;
static char thiz_loudness[MAX_CH_LOUD_LEN] = {0};

extern  void app_smart_volume_support(int32_t volume_scope);
extern  void app_channel_info_end_dialog(void);
extern  void app_channel_info_signal_end_dialog(void);
#if EDIT_MODE_SUPPRT
typedef struct _SmartVolumeCtrl
{
	GuiSurface     *surface;
	char           *layer_name;
	GXGDI_Rect      rect;
	uint32_t	fore_color;
	uint32_t	back_color;
	bool            init;
}SmartVolumeCtrl;

static SmartVolumeCtrl s_SmVolueLoudnessCtrl = {
	.surface = NULL,
	.layer_name = NULL,
	.back_color = 0xff00ff,//0x31314a,//202121,//0x0c0c0c,
	.fore_color = 0xf4f4f4,
	.init = false,
	.rect = {816,278,16,30},
};

static int smart_volume_osd_layer_init(void)
{
    SmartVolumeCtrl *ctrl = &s_SmVolueLoudnessCtrl;

    if(true == ctrl->init)
	{
		app_log_error(">>>>>please deinit first\n");
		return -1;
	}

	ctrl->surface = hd_get_surface((GAL_Rect *)&ctrl->rect, UI_THEME_BPP, NULL, false);
	if(NULL == ctrl->surface)
	{
		app_log_error(">>>>>Create surface failed.\n");
		goto err;
	}

	ctrl->layer_name = GxGDI_LayerRegister(ctrl->surface, ctrl->rect.x, ctrl->rect.y);
	if(NULL == ctrl->layer_name)
	{
		app_log_error(">>>Register surface failed.\n");
		goto err;
	}

	ctrl->init = true;
	return 0;

err:
	if (ctrl->layer_name)
	{
		GxGDI_LayerUnregister(ctrl->layer_name);
		ctrl->layer_name = NULL;
	}
	if(ctrl->surface)
	{
		hd_free_surface(ctrl->surface);
		ctrl->surface = NULL;
	}

	return -1;

}

static int smart_volume_osd_layer_draw_string(void* string)
{
	SmartVolumeCtrl *ctrl = &s_SmVolueLoudnessCtrl;
	GXGDI_Rect draw_rect = {0};

	if(NULL == ctrl->surface)
	{
		app_log_error(">>>error-----!Fill NULL surface.\n");
		return -1;
	}
	if(APP_THEME_CLASSIC_DARK)
		GxGDI_LayerSetPosition(ctrl->layer_name,654,204);
	else if(APP_THEME_SKY_BLUE)
		GxGDI_LayerSetPosition(ctrl->layer_name,686,312);
	/*if(strcmp(GUI_GetFocusWidget(),WIN_EDIT_LOUDNESS_SET) == 0){
		ctrl->fore_color = 0x96C1F6;
	}*/
	draw_rect.x = 0;
	draw_rect.y = 0;
	draw_rect.w = ctrl->rect.w;
	draw_rect.h = ctrl->rect.h;
	GXGDI_Lock();
	GXGDI_Begin();
	GXGDI_FillRect(ctrl->surface, &draw_rect, ctrl->back_color);
	GXGDI_DrawString(ctrl->surface, string, &draw_rect, ALIGNMENT_HCENTRE | ALIGNMENT_VCENTRE, ctrl->fore_color, GDI_STRING_PARAGRAPH);

	GXGDI_End();
	GxGDI_LayerUpdate(ctrl->layer_name);
	GXGDI_Unlock();

	return 0;
}

static int smart_volume_osd_layer_quit(void)
{
	SmartVolumeCtrl *ctrl = &s_SmVolueLoudnessCtrl;

	if(false == ctrl->init)
	{
		app_log_debug(">>> Have been quited\n");
		return -1;
	}
	GxGDI_LayerUnregister(ctrl->layer_name);
	ctrl->layer_name = NULL;
	hd_free_surface(ctrl->surface);
	ctrl->surface = NULL;
	ctrl->init = false;

	return 0;
}


static int float_to_int(float value)
{
    float scaledValue = value * 10;

    // 转换为整数
    return (int)scaledValue;
    //return (int)(value < 0 ? ceil(value) : floor(value + 0.5))*10; // 负数向上取整，正数四舍五入
    //return (int)round(value * 10); // 四舍五入
}
static float _float_edit_str_to_value(const char *str)
{
    const char *cp;
    float data = 0.0f;
    float decimal_place = 1.0f; // 用于处理小数部分
    char isNegative = 0;
    char isDecimal = 0; // 标记是否遇到小数点

    if (NULL == str)
        return 0.0f;

    for (cp = str; *cp != 0; ++cp)
    {
        if (*cp == '-')
        {
            isNegative = 1; // 记录负号
        }
        else if (*cp >= '0' && *cp <= '9')
        {
            if (isDecimal)
            {
                // 处理小数部分
                data = data + (*cp - '0') / (decimal_place *= 10);
            }
            else
            {
                // 处理整数部分
                data = data * 10 + (*cp - '0');
            }
        }
        else if (*cp == '.')
        {
            if (!isDecimal) // 只允许出现一个小数点
            {
                isDecimal = 1;
            }
            else
            {
                break; // 遇到第二个小数点，停止处理
            }
        }
        else
        {
            break; // 遇到非数字字符，停止处理
        }
    }

    return isNegative ? -data : data; // 根据负号返回结果
}
#endif
static void app_smart_volume_para_get(SmVolumePara *smart)
{
    int init_value = 0;

    GxBus_ConfigGetInt(SMART_VOLUME_MODE, &init_value, SMART_MODE);
    smart->sm_status = init_value;
#if EDIT_MODE_SUPPRT
    GxBus_ConfigGetInt(SMART_VOLUME_LEVEL, &init_value, SMART_GAIN_LEVEL);
    smart->sm_level= init_value/10.0f;
#else
    GxBus_ConfigGetInt(SMART_VOLUME_LEVEL, &init_value, SMART_GAIN_LEVEL);
    smart->sm_level= init_value;
#endif
    GxBus_ConfigGetInt(SMART_VOLUME_TIMES, &init_value, SMART_GAIN_TIME);
    smart->sm_vol_times= init_value;
}

static void app_smart_volume_app_para_get(SmVolumePara *app)
{
#if EDIT_MODE_SUPPRT
    app->sm_level = _float_edit_str_to_value(s_smart_item[SMART_VOLUME_BOX_ITEM_LEVEL].itemProperty.itemPropertyEdit.string);
#else
    app->sm_level = s_smart_item[SMART_VOLUME_BOX_ITEM_LEVEL].itemProperty.itemPropertyCmb.sel;
#endif
	app->sm_status = s_smart_item[SMART_VOLUME_BOX_ITEM_STATUS].itemProperty.itemPropertyCmb.sel;
    app->sm_vol_times = s_smart_item[SMART_VOLUME_BOX_ITEM_TIMES].itemProperty.itemPropertyCmb.sel;
}

static int app_smart_volume_para_save(SmVolumePara *result)
{
    SmVolumePara *app = &thiz_smart_para;
#if 0
    if (app->sm_status != result->sm_status)
    {
        GxBus_ConfigSetInt(SMART_VOLUME_MODE,result->sm_status);
    }
#endif
#if EDIT_MODE_SUPPRT
{
    int loudness = float_to_int(result->sm_level);
	if (app->sm_level!= result->sm_status)
	{
		GxBus_ConfigSetInt(SMART_VOLUME_LEVEL,loudness);
	}
}
#else
	if (app->sm_level!= result->sm_level)
	{
		GxBus_ConfigSetInt(SMART_VOLUME_LEVEL,result->sm_level);
	}
#endif
    if (app->sm_vol_times != result->sm_vol_times)
    {
        GxBus_ConfigSetInt(SMART_VOLUME_TIMES,result->sm_vol_times);
    }

    app_smart_volume_support(0);
#if EDIT_MODE_SUPPRT
{
    if(thiz_smart_para.sm_status == VOLUME_SMART)
    {

        AudioSmartConfig config;
        int gain_timems = 0;
        float loudness = -16;
        loudness = (float)(-result->sm_level);

        gain_timems = result->sm_vol_times*5000;
        config.target_timems = gain_timems;
        config.target_loudness = loudness;
        GxPlayer_SetAudioSmartVolume(1,&config);
    }
}
#else
      if(result->sm_status == VOLUME_SMART){
        app_smart_volume_support(VOLUME_SMART);
      }
#endif

	return 0;
}

static bool app_smart_test_para_change(SmVolumePara *ret_para)
{
    SmVolumePara *app = &thiz_smart_para;
	bool ret = FALSE;
#if 0
   	if(app->sm_status != ret_para->sm_status)
	{
		ret = TRUE;
	}
#endif
	if(app->sm_level != ret_para->sm_level)
	{
		ret = TRUE;
	}

	if(app->sm_vol_times != ret_para->sm_vol_times)
	{
		ret = TRUE;
	}

	return ret;
}

static int app_smart_volume_test_popdlg_exit_cb(PopDlgRet ret)
{
	SmVolumePara para_ret;

	if (POP_VAL_OK == ret)
	{
		memset(&para_ret,0,sizeof(para_ret));
		app_smart_volume_app_para_get(&para_ret);
		app_smart_volume_para_save(&para_ret);
	}
    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
	return 0 ;
}


static bool app_smart_volume_check_cb(void)
{
    bool ret = false;
    SmVolumePara para_ret = {0};

    app_smart_volume_app_para_get(&para_ret);
    ret = app_smart_test_para_change(&para_ret);

    return ret;
}

static int app_smart_volume_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if(false != app_system_set_callback_handler(app_smart_volume_check_cb, app_smart_volume_test_popdlg_exit_cb))
    {
         ret = EXIT_RET_POP;
    }
#if EDIT_MODE_SUPPRT
    smart_volume_osd_layer_quit();
#endif
	return ret;
}

static void app_smart_volume_support_item_init(void)
{
#if 1
    char *status_str = (thiz_smart_para.sm_status == VOLUME_SMART) ? "On" : "Off";
    s_smart_item[SMART_VOLUME_BOX_ITEM_STATUS].itemTitle = "Smart";
    s_smart_item[SMART_VOLUME_BOX_ITEM_STATUS].itemType = ITEM_PUSH;
    s_smart_item[SMART_VOLUME_BOX_ITEM_STATUS].itemProperty.itemPropertyBtn.string= status_str;
    s_smart_item[SMART_VOLUME_BOX_ITEM_STATUS].itemCallback.btnCallback.BtnPress= NULL;
    s_smart_item[SMART_VOLUME_BOX_ITEM_STATUS].itemStatus = ITEM_NORMAL;
#else
	s_smart_item[SMART_VOLUME_BOX_ITEM_STATUS].itemTitle = "Smart";
	s_smart_item[SMART_VOLUME_BOX_ITEM_STATUS].itemType = ITEM_CHOICE;
	s_smart_item[SMART_VOLUME_BOX_ITEM_STATUS].itemProperty.itemPropertyCmb.content= SMART_STATUS_CONTENT;
	s_smart_item[SMART_VOLUME_BOX_ITEM_STATUS].itemProperty.itemPropertyCmb.sel = thiz_smart_para.sm_status;
	s_smart_item[SMART_VOLUME_BOX_ITEM_STATUS].itemCallback.cmbCallback.CmbChange= NULL;
	s_smart_item[SMART_VOLUME_BOX_ITEM_STATUS].itemStatus = ITEM_NORMAL;
#endif
}

#if EDIT_MODE_SUPPRT

static int _smart_volume_draw_line(void)
{
    smart_volume_osd_layer_init();
    smart_volume_osd_layer_draw_string("-");
    return 0;
}

static int app_smart_volume_level_edit_reach_end(char *str)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define SMART_VOLUME_TIME_LEN 4
    GUI_GetProperty(WIN_EDIT_LOUDNESS_SET, "select", &cur_sel);
    sel = (cur_sel == 0 ? SMART_VOLUME_TIME_LEN - 1 : 0);
    GUI_SetProperty(WIN_EDIT_LOUDNESS_SET, "select", &sel);
    return EVENT_TRANSFER_STOP;
}
#if 0
static int app_smart_volume_edit_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_STOP;
    int sel;
    unsigned int lastsel = 3;

    switch (key)
    {
        case STBK_OK:
            break;
        case STBK_0:
        case STBK_1:
        case STBK_2:
        case STBK_3:
        case STBK_4:
        case STBK_5:
        case STBK_6:
        case STBK_7:
        case STBK_8:
        case STBK_9:
            ret = EVENT_TRANSFER_KEEPON;
            break;
        case STBK_LEFT:
            {
                GUI_GetProperty(WIN_EDIT_LOUDNESS_SET, "select", &sel);
                if(sel == 1)
                {
                    sel=lastsel;
                    GUI_SetProperty(WIN_EDIT_LOUDNESS_SET, "select", &sel);
                }
                else
                    ret = EVENT_TRANSFER_KEEPON;
            }
            break;
        case STBK_RIGHT:
            {
                GUI_GetProperty(WIN_EDIT_LOUDNESS_SET, "select", &sel);
                if(sel == lastsel)
                {
                    sel=1;
                    GUI_SetProperty(WIN_EDIT_LOUDNESS_SET, "select", &sel);
                }
                else
                    ret = EVENT_TRANSFER_KEEPON;
            }
            break;
        case STBK_EXIT:
        case STBK_DOWN:
        case STBK_UP:
            break;
        default:
            break;
    }
    return ret;
}
#endif
#endif
static void app_smart_volume_gain_level_item_init(void)
{
    s_smart_item[SMART_VOLUME_BOX_ITEM_LEVEL].itemTitle = "Loudness";
#if EDIT_MODE_SUPPRT
    int sm_level = (int)(thiz_smart_para.sm_level*10+0.5);
    memset(thiz_loudness, 0, sizeof(thiz_loudness));
    sprintf(thiz_loudness, "%02d.%d", sm_level/10,sm_level%10);
   // sprintf(thiz_loudness, "%02.1f", thiz_smart_para.sm_level);
    _smart_volume_draw_line();
    s_smart_item[SMART_VOLUME_BOX_ITEM_LEVEL].itemType = ITEM_EDIT;
	s_smart_item[SMART_VOLUME_BOX_ITEM_LEVEL].itemProperty.itemPropertyEdit.format = "edit_float";
	s_smart_item[SMART_VOLUME_BOX_ITEM_LEVEL].itemProperty.itemPropertyEdit.maxlen = "4";
	s_smart_item[SMART_VOLUME_BOX_ITEM_LEVEL].itemProperty.itemPropertyEdit.intaglio = NULL;
	s_smart_item[SMART_VOLUME_BOX_ITEM_LEVEL].itemProperty.itemPropertyEdit.default_intaglio = NULL;
	s_smart_item[SMART_VOLUME_BOX_ITEM_LEVEL].itemProperty.itemPropertyEdit.string = thiz_loudness;
	s_smart_item[SMART_VOLUME_BOX_ITEM_LEVEL].itemCallback.editCallback.EditReachEnd= app_smart_volume_level_edit_reach_end;
	s_smart_item[SMART_VOLUME_BOX_ITEM_LEVEL].itemCallback.editCallback.EditPress = NULL;//app_smart_volume_edit_press_callback;
#else
	s_smart_item[SMART_VOLUME_BOX_ITEM_LEVEL].itemType = ITEM_CHOICE;
	s_smart_item[SMART_VOLUME_BOX_ITEM_LEVEL].itemProperty.itemPropertyCmb.content= SMART_LEVEL_CONTENT;
	s_smart_item[SMART_VOLUME_BOX_ITEM_LEVEL].itemProperty.itemPropertyCmb.sel = thiz_smart_para.sm_level;
	s_smart_item[SMART_VOLUME_BOX_ITEM_LEVEL].itemCallback.cmbCallback.CmbChange= NULL;
#endif
    if(thiz_smart_para.sm_status == VOLUME_SMART)
    {
        s_smart_item[SMART_VOLUME_BOX_ITEM_LEVEL].itemStatus = ITEM_NORMAL;
    }
    else
    {
        s_smart_item[SMART_VOLUME_BOX_ITEM_LEVEL].itemStatus = ITEM_DISABLE;
    }

}

static void app_smart_volume_times_item_init(void)
{
	s_smart_item[SMART_VOLUME_BOX_ITEM_TIMES].itemTitle = "Gain Time";
	s_smart_item[SMART_VOLUME_BOX_ITEM_TIMES].itemType = ITEM_CHOICE;
	s_smart_item[SMART_VOLUME_BOX_ITEM_TIMES].itemProperty.itemPropertyCmb.content= SMART_TIMES_CONTENT;
	s_smart_item[SMART_VOLUME_BOX_ITEM_TIMES].itemProperty.itemPropertyCmb.sel = thiz_smart_para.sm_vol_times;
	s_smart_item[SMART_VOLUME_BOX_ITEM_TIMES].itemCallback.cmbCallback.CmbChange= NULL;
    if(thiz_smart_para.sm_status == VOLUME_SMART)
    {
        s_smart_item[SMART_VOLUME_BOX_ITEM_TIMES].itemStatus = ITEM_NORMAL;
    }
    else
    {
        s_smart_item[SMART_VOLUME_BOX_ITEM_TIMES].itemStatus = ITEM_DISABLE;
    }
}

void app_smart_volume_test_menu_exec(void)
{
    app_channel_info_end_dialog();
    app_channel_info_signal_end_dialog();
    memset(&s_smart_volume_opt, 0 ,sizeof(s_smart_volume_opt));

	app_smart_volume_para_get(&thiz_smart_para);
	s_smart_volume_opt.menuTitle = "Smart Volume";
	s_smart_volume_opt.itemNum = SMART_VOLUME_BOX_ITEM_TOTAL;
	s_smart_volume_opt.timeDisplay = TIP_HIDE;
	s_smart_volume_opt.item = s_smart_item;

    app_smart_volume_support_item_init();
	app_smart_volume_gain_level_item_init();
	app_smart_volume_times_item_init();

	s_smart_volume_opt.exit = app_smart_volume_exit_callback;

	app_system_set_create(&s_smart_volume_opt);

}


#endif

