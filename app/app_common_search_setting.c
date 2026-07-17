#include "app.h"
#include "app_windows.h"
#include "module/app_ioctl.h"
#include "app_common_search_setting.h"
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif

#define BOX_CST_OPTIONS         "box_cst_options"
#define IMG_CST_MODULE_BG       "img_cst_module_bg"
#define IMG_CST_STRENGTH_BG     "img_cst_strength_bg"
#define IMG_CST_QUALITY_BG      "img_cst_quality_bg"
#define TXT_CST_TITLE           "text_cst_title"
#define TXT_CST_STRENGTH        "text_cst_strength"
#define TXT_CST_STRENGTH_VALUE  "text_cst_strength_value"
#define TXT_CST_QUALITY_VALUE   "text_cst_quality_value"
#define TXT_CST_QUALITY         "text_cst_quality"
#define PROGBAR_CST_STRENGTH    "progbar_cst_strength"
#define PROGBAR_CST_QUALITY     "progbar_cst_quality"

#define BTN_CST_EXIT            "btn_cst_exit"
#define TXT_CST_EXIT            "txt_cst_exit"
#define BTN_CST_OK              "btn_cst_ok"
#define TXT_CST_OK              "txt_cst_ok"
#define IMG_CST_MOVE            "img_cst_move"
#define TXT_CST_MOVE            "txt_cst_move"
#define IMG_CST_RED             "img_cst_red"
#define TXT_CST_RED             "txt_cst_red"
#define IMG_CST_GREEN           "img_cst_green"
#define TXT_CST_GREEN           "txt_cst_green"
#define IMG_CST_YELLOW          "img_cst_yellow"
#define TXT_CST_YELLOW          "txt_cst_yellow"
#define IMG_CST_BLUE            "img_cst_blue"
#define TXT_CST_BLUE            "txt_cst_blue"
#define	GREEN_KEY_IMG			"s_ts_green"
#define RED_KEY_IMG				"s_ts_red"
#define BLUE_KEY_IMG			"s_ts_blue"
#define YELLOW_KEY_IMG			"s_ts_yellow"

#define IMG_BAR_LONG_FOCUS_OK   "s_bar_long_focus_ok"
#define IMG_BAR_LONG_FOCUS      "s_bar_long_focus"
#define TXT_CST_CONTENT			"text_cst_url_content"

typedef enum
{
    CST_ITEM_1,
    CST_ITEM_2,
    CST_ITEM_3,
    CST_ITEM_4,
    CST_ITEM_5,
    CST_ITEM_6,
    CST_ITEM_7,
    CST_ITEM_MAX
}CSTOptSel;

typedef enum
{
	TIP_KEY = 0,
	COLOR_KEY,
}CSTKeyType;

typedef struct _CSTCtr
{
    CSTOptSel   cur_sel;
    int32_t     item_total;
    CSTOpt      *opt;
    event_list  *timer_signal;
	CSTKeyType	Key_type;
}CSTCtrl;

static CSTCtrl thiz_CSTCtrl = {
    .cur_sel    = CST_ITEM_1,
    .item_total = 0,
    .opt        = NULL,
    .timer_signal = NULL,
	.Key_type = TIP_KEY,
};

static char *s_cst_boxitem[] =
{
    "boxitem_cst1",
    "boxitem_cst2",
    "boxitem_cst3",
    "boxitem_cst4",
    "boxitem_cst5",
    "boxitem_cst6",
    "boxitem_cst7",
};

static char *s_cst_item_txt[] =
{
    "text_cst_item1",
    "text_cst_item2",
    "text_cst_item3",
    "text_cst_item4",
    "text_cst_item5",
    "text_cst_item6",
    "text_cst_item7",
};

static char *s_cst_item_edit[] =
{
    "edit_cst_item1",
    "edit_cst_item2",
    "edit_cst_item3",
    "edit_cst_item4",
    "edit_cst_item5",
    "edit_cst_item6",
    "edit_cst_item7",
};

static char *s_cst_item_cmb[] =
{
    "cmb_cst_item1",
    "cmb_cst_item2",
    "cmb_cst_item3",
    "cmb_cst_item4",
    "cmb_cst_item5",
    "cmb_cst_item6",
    "cmb_cst_item7",
};

static char *s_cst_item_btn[] =
{
    "btn_cst_item1",
    "btn_cst_item2",
    "btn_cst_item3",
    "btn_cst_item4",
    "btn_cst_item5",
    "btn_cst_item6",
    "btn_cst_item7",
};

static void _cst_boxitem_show(uint32_t sel, CSTItemType type)
{
    if(sel >= CST_ITEM_MAX)
        return;

    GUI_SetProperty(s_cst_boxitem[sel], "state", "show");
    GUI_SetProperty(s_cst_item_txt[sel], "state", "show");

    if(CST_ITEM_CHOICE == type || CST_ITEM_POP_CHOICE == type)
    {
        GUI_SetProperty(s_cst_item_cmb[sel], "state", "show");
        GUI_SetProperty(s_cst_item_edit[sel], "state", "hide");
        GUI_SetProperty(s_cst_item_btn[sel], "state", "hide");
    }
    else if(CST_ITEM_EDIT == type)
    {
        GUI_SetProperty(s_cst_item_edit[sel], "state", "show");
        GUI_SetProperty(s_cst_item_cmb[sel], "state", "hide");
        GUI_SetProperty(s_cst_item_btn[sel], "state", "hide");
    }
    else
    {
        GUI_SetProperty(s_cst_item_btn[sel], "state", "show");
        GUI_SetProperty(s_cst_item_cmb[sel], "state", "hide");
        GUI_SetProperty(s_cst_item_edit[sel], "state", "hide");
    }
    //focus child of boxitem
    GUI_SetProperty(s_cst_boxitem[sel], "disable", NULL);
    GUI_SetProperty(s_cst_boxitem[sel], "enable", NULL);
}

static void _cst_boxitem_hide(uint32_t sel)
{
    if(sel >= CST_ITEM_MAX)
        return;

    GUI_SetProperty(s_cst_boxitem[sel], "state", "hide");
    GUI_SetProperty(s_cst_item_txt[sel], "state", "hide");
    GUI_SetProperty(s_cst_item_edit[sel], "state", "hide");
    GUI_SetProperty(s_cst_item_cmb[sel], "state", "hide");
    GUI_SetProperty(s_cst_item_btn[sel], "state", "hide");
}

static void _cst_boxitem_disable(uint32_t sel, CSTItemType type)
{
    if(sel >= CST_ITEM_MAX)
        return;

    if(CST_ITEM_CHOICE == type || CST_ITEM_POP_CHOICE == type)
    {
        GUI_SetProperty(s_cst_item_cmb[sel], "state", "show");
        GUI_SetProperty(s_cst_item_edit[sel], "state", "hide");
        GUI_SetProperty(s_cst_item_btn[sel], "state", "hide");
    }
    else if(CST_ITEM_EDIT == type)
    {
        GUI_SetProperty(s_cst_item_edit[sel], "state", "show");
        GUI_SetProperty(s_cst_item_cmb[sel], "state", "hide");
        GUI_SetProperty(s_cst_item_btn[sel], "state", "hide");
    }
    else
    {
        GUI_SetProperty(s_cst_item_btn[sel], "state", "show");
        GUI_SetProperty(s_cst_item_cmb[sel], "state", "hide");
        GUI_SetProperty(s_cst_item_edit[sel], "state", "hide");
    }
    GUI_SetProperty(s_cst_boxitem[sel], "state", "disable");
}

static void _cst_item_init(uint32_t sel, CSTItem *item)
{
    if(NULL == item || sel >= CST_ITEM_MAX)
        return;

    if(item->itemTitle)
        GUI_SetProperty(s_cst_item_txt[sel], "string", item->itemTitle);

    if(CST_ITEM_NORMAL == item->itemStatus)
        _cst_boxitem_show(sel, item->itemType);
    else if(CST_ITEM_DISABLE == item->itemStatus)
        _cst_boxitem_disable(sel, item->itemType);
    else if(CST_ITEM_HIDE == item->itemStatus)
        _cst_boxitem_hide(sel);

    if( APP_THEME_CLASSIC_BLUE || APP_THEME_CLASSIC_DARK ){
    if(CST_ITEM_POP_CHOICE == item->itemType)
        GUI_SetProperty(s_cst_boxitem[sel], "focus_image", IMG_BAR_LONG_FOCUS_OK);
    else if(CST_ITEM_CHOICE == item->itemType)
        GUI_SetProperty(s_cst_boxitem[sel], "focus_image", IMG_BAR_LONG_FOCUS);
    }

    if(CST_ITEM_CHOICE == item->itemType || CST_ITEM_POP_CHOICE == item->itemType)
    {
        if(item->itemProperty.itemPropertyCmb.content != NULL)
            GUI_SetProperty(s_cst_item_cmb[sel], "content", item->itemProperty.itemPropertyCmb.content);

        GUI_SetProperty(s_cst_item_cmb[sel], "select", &(item->itemProperty.itemPropertyCmb.sel));
    }
    else if(CST_ITEM_EDIT == item->itemType)
    {
        GUI_SetProperty(s_cst_item_edit[sel], "clear", NULL);

        if(item->itemProperty.itemPropertyEdit.format != NULL)
            GUI_SetProperty(s_cst_item_edit[sel], "format", item->itemProperty.itemPropertyEdit.format);

        if(item->itemProperty.itemPropertyEdit.maxlen != NULL)
            GUI_SetProperty(s_cst_item_edit[sel], "maxlen", item->itemProperty.itemPropertyEdit.maxlen);

        if(item->itemProperty.itemPropertyEdit.intaglio != NULL)
            GUI_SetProperty(s_cst_item_edit[sel], "intaglio", item->itemProperty.itemPropertyEdit.intaglio);
        else
            GUI_SetProperty(s_cst_item_edit[sel], "intaglio", "");

        if(item->itemProperty.itemPropertyEdit.default_intaglio != NULL)
            GUI_SetProperty(s_cst_item_edit[sel], "default_intaglio", item->itemProperty.itemPropertyEdit.default_intaglio);
        else
            GUI_SetProperty(s_cst_item_edit[sel], "default_intaglio", "");

        if(item->itemProperty.itemPropertyEdit.string != NULL)
            GUI_SetProperty(s_cst_item_edit[sel], "string", item->itemProperty.itemPropertyEdit.string);
    }
    else if(CST_ITEM_PUSH == item->itemType)
    {
        if(item->itemProperty.itemPropertyBtn.string != NULL)
            GUI_SetProperty(s_cst_item_btn[sel], "string", item->itemProperty.itemPropertyBtn.string);
    }

}

static void _cst_signal_clean(void)
{
    CSTCtrl *ctrl = &thiz_CSTCtrl;
    uint32_t value = 0;
    SignalBarWidget siganl_bar = {0};
    SignalValue siganl_val = {0};
    char buffer[10] = {0};

    // progbar strength
    siganl_bar.strength_bar = PROGBAR_CST_STRENGTH;
    siganl_val.strength_val = value;

    // strength value
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d%%", value);
    GUI_SetProperty(TXT_CST_STRENGTH_VALUE, "string", buffer);

    // progbar quality
    siganl_bar.quality_bar = PROGBAR_CST_QUALITY;
    siganl_val.quality_val = value;

    // quality value
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d%%", value);
    GUI_SetProperty(TXT_CST_QUALITY_VALUE, "string", buffer);

    app_signal_progbar_update(ctrl->opt->cur_tuner, &siganl_bar, &siganl_val);
    app_panel_ioctl_handle(PANEL_SHOW_QUALITY, &value);
    return;
}

static void _cst_signal_widget_show(void)
{
    GUI_SetProperty(IMG_CST_MODULE_BG, "state", "show");
    GUI_SetProperty(IMG_CST_STRENGTH_BG, "state", "show");
    GUI_SetProperty(IMG_CST_QUALITY_BG, "state", "show");
    GUI_SetProperty(TXT_CST_STRENGTH, "state", "show");
    GUI_SetProperty(TXT_CST_STRENGTH_VALUE, "state", "show");
    GUI_SetProperty(TXT_CST_QUALITY_VALUE, "state", "show");
    GUI_SetProperty(TXT_CST_QUALITY, "state", "show");
    GUI_SetProperty(PROGBAR_CST_STRENGTH, "state", "show");
    GUI_SetProperty(PROGBAR_CST_QUALITY, "state", "show");
    _cst_signal_clean();
    return;
}

static void _cst_signal_widget_hide(void)
{
    GUI_SetProperty(IMG_CST_MODULE_BG, "state", "hide");
    GUI_SetProperty(IMG_CST_STRENGTH_BG, "state", "hide");
    GUI_SetProperty(IMG_CST_QUALITY_BG, "state", "hide");
    GUI_SetProperty(TXT_CST_STRENGTH, "state", "hide");
    GUI_SetProperty(TXT_CST_QUALITY, "state", "hide");
    GUI_SetProperty(TXT_CST_STRENGTH_VALUE, "state", "hide");
    GUI_SetProperty(TXT_CST_QUALITY_VALUE, "state", "hide");
    GUI_SetProperty(PROGBAR_CST_STRENGTH, "state", "hide");
    GUI_SetProperty(PROGBAR_CST_QUALITY, "state", "hide");
    return;
}

static int32_t _cst_signal_timer(void *userdata)
{
    unsigned int value = 0;
    char Buffer[20] = {0};
    SignalValue siganl_val = {0};
    SignalBarWidget siganl_bar = {0};
    CSTCtrl *ctrl = &thiz_CSTCtrl;
    char* focus_Window = (char*)GUI_GetFocusWindow();

    if(NULL != focus_Window && strcasecmp(WND_COMMON_SEARCH_SETTING, focus_Window) != 0)
        return 0;

    app_ioctl(ctrl->opt->cur_tuner, FRONTEND_STRENGTH_GET, &value);
    siganl_bar.strength_bar = PROGBAR_CST_STRENGTH;
    siganl_val.strength_val = value;

    memset(Buffer, 0, sizeof(Buffer));
    sprintf(Buffer, "%d%%", value);
    GUI_SetProperty(TXT_CST_STRENGTH_VALUE, "string", Buffer);

    app_ioctl(ctrl->opt->cur_tuner, FRONTEND_QUALITY_GET, &value);
    siganl_bar.quality_bar = PROGBAR_CST_QUALITY;
    siganl_val.quality_val = value;

    memset(Buffer, 0, sizeof(Buffer));
    sprintf(Buffer, "%d%%", value);
    GUI_SetProperty(TXT_CST_QUALITY_VALUE, "string", Buffer);

    app_signal_progbar_update(ctrl->opt->cur_tuner, &siganl_bar, &siganl_val);
    app_panel_ioctl_handle(PANEL_SHOW_QUALITY, &value);

    return 0;
}

static void _cst_set_show_funckey(void)
{
    CSTCtrl *ctrl = &thiz_CSTCtrl;
    if(ctrl->opt->funckey.redKey.keyPressFun != NULL || ctrl->opt->funckey.redKey.keyName != NULL)
	{
		GUI_SetProperty(TXT_CST_RED, "string", ctrl->opt->funckey.redKey.keyName);
		GUI_SetProperty(IMG_CST_RED, "img", RED_KEY_IMG);
        ctrl->Key_type = COLOR_KEY;
	}
	else
	{
		GUI_SetProperty(TXT_CST_RED, "string", " ");
		GUI_SetProperty(IMG_CST_RED, "img", "null");
	}
    if(ctrl->opt->funckey.yellowKey.keyPressFun != NULL || ctrl->opt->funckey.yellowKey.keyName != NULL)
	{
		GUI_SetProperty(TXT_CST_YELLOW, "string", ctrl->opt->funckey.yellowKey.keyName);
		GUI_SetProperty(IMG_CST_YELLOW, "img", YELLOW_KEY_IMG);
        ctrl->Key_type = COLOR_KEY;
	}
	else
	{
		GUI_SetProperty(TXT_CST_YELLOW, "string", " ");
		GUI_SetProperty(IMG_CST_YELLOW, "img", "null");
	}
    if(ctrl->opt->funckey.greenKey.keyPressFun != NULL || ctrl->opt->funckey.greenKey.keyName != NULL)
	{
		GUI_SetProperty(TXT_CST_GREEN, "string", ctrl->opt->funckey.greenKey.keyName);
		GUI_SetProperty(IMG_CST_GREEN, "img", GREEN_KEY_IMG);
        ctrl->Key_type = COLOR_KEY;
	}
	else
	{
		GUI_SetProperty(TXT_CST_GREEN, "string", " ");
		GUI_SetProperty(IMG_CST_GREEN, "img", "null");
	}
    if(ctrl->opt->funckey.blueKey.keyPressFun != NULL || ctrl->opt->funckey.blueKey.keyName != NULL)
	{
		GUI_SetProperty(TXT_CST_BLUE, "string", ctrl->opt->funckey.blueKey.keyName);
		GUI_SetProperty(IMG_CST_BLUE, "img", BLUE_KEY_IMG);
        ctrl->Key_type = COLOR_KEY;
	}
	else
	{
		GUI_SetProperty(TXT_CST_BLUE, "string", " ");
		GUI_SetProperty(IMG_CST_BLUE, "img", "null");
	}
    if(ctrl->Key_type == COLOR_KEY)
    {
		GUI_SetProperty(TXT_CST_EXIT, "state", "hide");
		GUI_SetProperty(BTN_CST_EXIT, "state", "hide");
		GUI_SetProperty(TXT_CST_OK, "state", "hide");
		GUI_SetProperty(BTN_CST_OK, "state", "hide");
		GUI_SetProperty(TXT_CST_MOVE, "state", "hide");
		GUI_SetProperty(IMG_CST_MOVE, "state", "hide");
    }
}

SIGNAL_HANDLER int32_t app_edit_cst_item1_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_CST_ITEM1_LEN 10
    GUI_GetProperty("edit_cst_item1", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_CST_ITEM1_LEN - 1 : 0);
    GUI_SetProperty("edit_cst_item1", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int32_t app_edit_cst_item2_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_CST_ITEM2_LEN 10
    GUI_GetProperty("edit_cst_item2", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_CST_ITEM2_LEN - 1 : 0);
    GUI_SetProperty("edit_cst_item2", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int32_t app_edit_cst_item3_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_CST_ITEM3_LEN 10
    GUI_GetProperty("edit_cst_item3", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_CST_ITEM3_LEN - 1 : 0);
    GUI_SetProperty("edit_cst_item3", "select", &sel);
    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int32_t app_edit_cst_item4_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_CST_ITEM4_LEN 10
    GUI_GetProperty("edit_cst_item4", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_CST_ITEM4_LEN - 1 : 0);
    GUI_SetProperty("edit_cst_item4", "select", &sel);
    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int32_t app_edit_cst_item5_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_CST_ITEM5_LEN 10
    GUI_GetProperty("edit_cst_item5", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_CST_ITEM5_LEN - 1 : 0);
    GUI_SetProperty("edit_cst_item5", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int32_t app_edit_cst_item6_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_CST_ITEM6_LEN 10
    GUI_GetProperty("edit_cst_item6", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_CST_ITEM6_LEN - 1 : 0);
    GUI_SetProperty("edit_cst_item6", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int32_t app_edit_cst_item7_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_CST_ITEM7_LEN 10
    GUI_GetProperty("edit_cst_item7", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_CST_ITEM7_LEN - 1 : 0);
    GUI_SetProperty("edit_cst_item7", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int32_t app_cst_box_keypress(GuiWidget *widget, void *usrdata)
{
	int32_t cur_sel = 0;
    GUI_Event *event = NULL;
    CSTCtrl *ctrl = &thiz_CSTCtrl;
    int32_t ret = EVENT_TRANSFER_KEEPON;

	GUI_GetProperty(BOX_CST_OPTIONS, "select", &cur_sel);
    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN != event->type)
        return ret;

    switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
    {
        case STBK_UP:
            if(cur_sel >= ctrl->item_total || cur_sel < 0)
                break;
            if(CST_ITEM_1 == cur_sel)
            {
                cur_sel = ctrl->item_total - 1;
                GUI_SetProperty(BOX_CST_OPTIONS, "select", &cur_sel);
				ret = EVENT_TRANSFER_STOP;
            }
			break;

        case STBK_DOWN:
            if(cur_sel >= ctrl->item_total || cur_sel < 0)
                break;
            if(ctrl->item_total - 1 == cur_sel)
            {
                cur_sel = 0;
                GUI_SetProperty(BOX_CST_OPTIONS, "select", &cur_sel);
                ret = EVENT_TRANSFER_STOP;
            }
			break;

        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int32_t app_cst_box_change(GuiWidget *widget, void *usrdata)
{
    CSTCtrl *ctrl = &thiz_CSTCtrl;
    int32_t cur_sel = 0;

    GUI_GetProperty(BOX_CST_OPTIONS, "select", &cur_sel);
	if(ctrl->opt->signal.item_change)
	{
		ctrl->opt->signal.item_change(cur_sel);
	}
    return  EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int32_t app_cst_create(GuiWidget *widget, void *usrdata)
{
    int32_t i = 0;
    CSTCtrl *ctrl = &thiz_CSTCtrl;

#if SAT2IP_SERVER_SUPPORT
    app_sat2ip_set_dialog_pause();
#endif
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(0);
#endif
	ctrl->Key_type = TIP_KEY;
    if(ctrl->opt->menuTitle)
        GUI_SetProperty(TXT_CST_TITLE, "string", ctrl->opt->menuTitle);

	if(true == ctrl->opt->content_show)
	{
		if(ctrl->opt->Content)
		{
			GUI_SetProperty(TXT_CST_CONTENT, "string", ctrl->opt->Content);
			GUI_SetProperty(TXT_CST_CONTENT, "state", "show");
		}
		else
		{
			GUI_SetProperty(TXT_CST_CONTENT, "state", "hide");
		}
	}
	else
	{
		GUI_SetProperty(TXT_CST_CONTENT, "state", "hide");
	}

    if(true == ctrl->opt->signal_show)
    {
        _cst_signal_widget_show();
		GUI_SetProperty(TXT_CST_CONTENT, "state", "hide");
        APP_TIMER_ADD(ctrl->timer_signal, _cst_signal_timer, 200, TIMER_REPEAT);
    }
    else
    {
        _cst_signal_widget_hide();
    }

    for(i = 0; i < CST_ITEM_MAX; i++)
    {
        if(i < ctrl->item_total)
            _cst_item_init(i, &ctrl->opt->item[i]);
        else
            _cst_boxitem_hide(i);
    }
    _cst_set_show_funckey();
    GUI_SetProperty(BOX_CST_OPTIONS, "select", &ctrl->cur_sel);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int32_t app_cst_got_focus(GuiWidget *widget, void *usrdata)
{
    CSTCtrl *ctrl = &thiz_CSTCtrl;

    if(ctrl->timer_signal)
        reset_timer(ctrl->timer_signal);

    if(ctrl->opt->signal.got_focus)
        ctrl->opt->signal.got_focus();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int32_t app_cst_lost_focus(GuiWidget *widget, void *usrdata)
{
    CSTCtrl *ctrl = &thiz_CSTCtrl;

    if(ctrl->timer_signal)
        timer_stop(ctrl->timer_signal);

    if(ctrl->opt->signal.lost_focus)
        ctrl->opt->signal.lost_focus();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int32_t app_cst_destroy(GuiWidget *widget, void *usrdata)
{
    CSTCtrl *ctrl = &thiz_CSTCtrl;
    APP_TIMER_REMOVE(ctrl->timer_signal);
	ctrl->Key_type = TIP_KEY;
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(1);
#endif
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int32_t app_cst_keypress(GuiWidget *widget, void *usrdata)
{
    int32_t cur_sel = 0;
    CSTItem *item = NULL;
    GUI_Event *event = NULL;
    int32_t ret = EVENT_TRANSFER_STOP;
    int32_t exit_ret = CST_EXIT_RET_DEFAULT;
    CSTCtrl *ctrl = &thiz_CSTCtrl;

    event = (GUI_Event *)usrdata;
    GUI_GetProperty(BOX_CST_OPTIONS, "select", &cur_sel);
    if(GUI_KEYDOWN == event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                if(ctrl->opt->signal.exit_cb)
                    ctrl->opt->signal.exit_cb(CST_EXIT_ABANDON);
                GUI_EndDialog("after wnd_full_screen");
                GUI_SetInterface("flush", NULL);
                break;

            case STBK_1:
            case STBK_2:
            case STBK_3:
            case STBK_4:
            case STBK_5:
            case STBK_6:
            case STBK_7:
            case STBK_8:
            case STBK_9:
            case STBK_0:
                if(cur_sel >= ctrl->item_total || cur_sel < 0)
                    break;
                item = &ctrl->opt->item[cur_sel];
                if(CST_ITEM_EDIT == item->itemType && item->itemCb.editCb.EditPressEnd != NULL)
                    item->itemCb.editCb.EditPressEnd();
                break;

            case STBK_EXIT:
            case STBK_MENU:
                if(ctrl->opt->signal.exit_cb)
                    exit_ret = ctrl->opt->signal.exit_cb(CST_EXIT_NORMAL);

                if(exit_ret != CST_EXIT_RET_POP)
                {
                    GUI_EndDialog(WND_COMMON_SEARCH_SETTING);
                    GUI_SetInterface("flush", NULL);
                }
                break;

            case STBK_OK:
                if(cur_sel >= ctrl->item_total || cur_sel < 0)
                    break;
                item = &ctrl->opt->item[cur_sel];
                if(CST_ITEM_PUSH == item->itemType && item->itemCb.btnCb.BtnPress != NULL)
                    item->itemCb.btnCb.BtnPress();
                else if(CST_ITEM_POP_CHOICE == item->itemType && item->itemCb.cmbCb.CmbPress != NULL)
                    item->itemCb.cmbCb.CmbPress();
                break;
            case STBK_RED:
                if(ctrl->Key_type == COLOR_KEY && ctrl->opt->funckey.redKey.keyPressFun != NULL)
                    exit_ret = ctrl->opt->funckey.redKey.keyPressFun();
                break;
            case STBK_GREEN:
                if(ctrl->Key_type == COLOR_KEY && ctrl->opt->funckey.greenKey.keyPressFun != NULL)
                    exit_ret = ctrl->opt->funckey.greenKey.keyPressFun();
                break;
            case STBK_YELLOW:
                if(ctrl->Key_type == COLOR_KEY && ctrl->opt->funckey.yellowKey.keyPressFun != NULL)
                    exit_ret = ctrl->opt->funckey.yellowKey.keyPressFun();
                break;
            case STBK_BLUE:
                if(ctrl->Key_type == COLOR_KEY && ctrl->opt->funckey.blueKey.keyPressFun != NULL)
                    exit_ret = ctrl->opt->funckey.blueKey.keyPressFun();
                break;
			case STBK_F1:
                if(ctrl->Key_type == COLOR_KEY && ctrl->opt->funckey.hideKey.keyPressFun != NULL)
                    exit_ret = ctrl->opt->funckey.hideKey.keyPressFun();
				break;
#if DEMOD_INFO_SUPPORT
            case STBK_INFO:
                if(GUI_CheckDialog("wnd_demod_info") == GXCORE_ERROR)
                    GUI_CreateDialog("wnd_demod_info");
                break;
#endif
        }
    }
    return ret;
}

SIGNAL_HANDLER int32_t app_cst_cmb1_change(GuiWidget *widget, void *usrdata)
{
    CSTCtrl *ctrl = &thiz_CSTCtrl;
    int32_t ret = EVENT_TRANSFER_KEEPON;

    if((ctrl->item_total > CST_ITEM_1) && (ctrl->opt->item[CST_ITEM_1].itemCb.cmbCb.CmbChange != NULL))
    {
        int32_t sel = 0;
        GUI_GetProperty(s_cst_item_cmb[CST_ITEM_1], "select", &sel);
        ret = ctrl->opt->item[CST_ITEM_1].itemCb.cmbCb.CmbChange(sel);
    }

    return ret;
}

SIGNAL_HANDLER int32_t app_cst_cmb2_change(GuiWidget *widget, void *usrdata)
{
    CSTCtrl *ctrl = &thiz_CSTCtrl;
    int32_t ret = EVENT_TRANSFER_KEEPON;

    if((ctrl->item_total > CST_ITEM_2) && (ctrl->opt->item[CST_ITEM_2].itemCb.cmbCb.CmbChange != NULL))
    {
        int32_t sel = 0;
        GUI_GetProperty(s_cst_item_cmb[CST_ITEM_2], "select", &sel);
        ret = ctrl->opt->item[CST_ITEM_2].itemCb.cmbCb.CmbChange(sel);
    }

    return ret;
}

SIGNAL_HANDLER int32_t app_cst_cmb3_change(GuiWidget *widget, void *usrdata)
{
    CSTCtrl *ctrl = &thiz_CSTCtrl;
    int32_t ret = EVENT_TRANSFER_KEEPON;

    if((ctrl->item_total > CST_ITEM_3) && (ctrl->opt->item[CST_ITEM_3].itemCb.cmbCb.CmbChange != NULL))
    {
        int32_t sel = 0;
        GUI_GetProperty(s_cst_item_cmb[CST_ITEM_3], "select", &sel);
        ret = ctrl->opt->item[CST_ITEM_3].itemCb.cmbCb.CmbChange(sel);
    }

    return ret;
}

SIGNAL_HANDLER int32_t app_cst_cmb4_change(GuiWidget *widget, void *usrdata)
{
    CSTCtrl *ctrl = &thiz_CSTCtrl;
    int32_t ret = EVENT_TRANSFER_KEEPON;

    if((ctrl->item_total > CST_ITEM_4) && (ctrl->opt->item[CST_ITEM_4].itemCb.cmbCb.CmbChange != NULL))
    {
        int32_t sel = 0;
        GUI_GetProperty(s_cst_item_cmb[CST_ITEM_4], "select", &sel);
        ret = ctrl->opt->item[CST_ITEM_4].itemCb.cmbCb.CmbChange(sel);
    }

    return ret;
}

SIGNAL_HANDLER int32_t app_cst_cmb5_change(GuiWidget *widget, void *usrdata)
{
    CSTCtrl *ctrl = &thiz_CSTCtrl;
    int32_t ret = EVENT_TRANSFER_KEEPON;

    if((ctrl->item_total > CST_ITEM_5) && (ctrl->opt->item[CST_ITEM_5].itemCb.cmbCb.CmbChange != NULL))
    {
        int32_t sel = 0;
        GUI_GetProperty(s_cst_item_cmb[CST_ITEM_5], "select", &sel);
        ret = ctrl->opt->item[CST_ITEM_5].itemCb.cmbCb.CmbChange(sel);
    }

    return ret;
}

SIGNAL_HANDLER int32_t app_cst_cmb6_change(GuiWidget *widget, void *usrdata)
{
    CSTCtrl *ctrl = &thiz_CSTCtrl;
    int32_t ret = EVENT_TRANSFER_KEEPON;

    if((ctrl->item_total > CST_ITEM_6) && (ctrl->opt->item[CST_ITEM_6].itemCb.cmbCb.CmbChange != NULL))
    {
        int32_t sel = 0;
        GUI_GetProperty(s_cst_item_cmb[CST_ITEM_6], "select", &sel);
        ret = ctrl->opt->item[CST_ITEM_6].itemCb.cmbCb.CmbChange(sel);
    }

    return ret;
}

SIGNAL_HANDLER int32_t app_cst_cmb7_change(GuiWidget *widget, void *usrdata)
{
    CSTCtrl *ctrl = &thiz_CSTCtrl;
    int32_t ret = EVENT_TRANSFER_KEEPON;

    if((ctrl->item_total > CST_ITEM_7) && (ctrl->opt->item[CST_ITEM_7].itemCb.cmbCb.CmbChange != NULL))
    {
        int32_t sel = 0;
        GUI_GetProperty(s_cst_item_cmb[CST_ITEM_7], "select", &sel);
        ret = ctrl->opt->item[CST_ITEM_7].itemCb.cmbCb.CmbChange(sel);
    }

    return ret;
}

int32_t app_cst_item_data_get(uint32_t sel, CSTItemData *data)
{
    int32_t ret = -1;
    CSTCtrl *ctrl = &thiz_CSTCtrl;

    if(sel >= CST_ITEM_MAX || NULL == data)
        return -1;

    if(CST_ITEM_CHOICE == ctrl->opt->item[sel].itemType || CST_ITEM_POP_CHOICE == ctrl->opt->item[sel].itemType)
        ret = GUI_GetProperty(s_cst_item_cmb[sel], "select", &data->value);
    else if(CST_ITEM_EDIT == ctrl->opt->item[sel].itemType)
        ret = GUI_GetProperty(s_cst_item_edit[sel], "string", &data->string);
    else
        ret = GUI_GetProperty(s_cst_item_btn[sel], "string", &data->string);

    return ret;
}

int32_t app_cst_choice_item_sel_set(uint32_t sel, int32_t value)
{
    CSTCtrl *ctrl = &thiz_CSTCtrl;

    if(sel >= CST_ITEM_MAX)
        return -1;

    if(CST_ITEM_CHOICE == ctrl->opt->item[sel].itemType || CST_ITEM_POP_CHOICE == ctrl->opt->item[sel].itemType)
        ctrl->opt->item[sel].itemProperty.itemPropertyCmb.sel = value;

    return 0;
}

int32_t app_cst_item_data_update(uint32_t sel)
{
    int32_t ret = -1;
    CSTCtrl *ctrl = &thiz_CSTCtrl;

    if(sel >= CST_ITEM_MAX)
        return -1;

    if(CST_ITEM_CHOICE == ctrl->opt->item[sel].itemType || CST_ITEM_POP_CHOICE == ctrl->opt->item[sel].itemType)
	{
        if(ctrl->opt->item[sel].itemProperty.itemPropertyCmb.content != NULL)
            GUI_SetProperty(s_cst_item_cmb[sel], "content", ctrl->opt->item[sel].itemProperty.itemPropertyCmb.content);
        ret = GUI_SetProperty(s_cst_item_cmb[sel], "select", &ctrl->opt->item[sel].itemProperty.itemPropertyCmb.sel);
	}
    else if(CST_ITEM_EDIT == ctrl->opt->item[sel].itemType)
        ret = GUI_SetProperty(s_cst_item_edit[sel], "string", ctrl->opt->item[sel].itemProperty.itemPropertyEdit.string);
    else
        ret = GUI_SetProperty(s_cst_item_btn[sel], "string", ctrl->opt->item[sel].itemProperty.itemPropertyBtn.string);

    app_update_pop_dlg(WND_POP_BOOK);
    return ret;
}

void app_cst_key_data_updata(void)
{
    _cst_set_show_funckey();
}

int32_t app_cst_content_data_update(void)
{
    CSTCtrl *ctrl = &thiz_CSTCtrl;
    int32_t ret = -1;

	if(true == ctrl->opt->content_show)
	{
		if(ctrl->opt->Content)
		{
			ret = GUI_SetProperty(TXT_CST_CONTENT, "string", ctrl->opt->Content);
			GUI_SetProperty(TXT_CST_CONTENT, "state", "show");
		}
		else
		{
			GUI_SetProperty(TXT_CST_CONTENT, "state", "hide");
		}
	}
	else
	{
		GUI_SetProperty(TXT_CST_CONTENT, "state", "hide");
	}
	GUI_SetProperty(WND_COMMON_SEARCH_SETTING, "update", NULL);
	return ret;
}

bool app_cst_callback_handler(CheckCb check_cb, ExitCb exit_cb)
{
    bool ret = 0;

    ret = check_cb();
    if(true == ret)
    {
#if SYS_SETTING_SAVE_DIRECT
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = STR_ID_SAVE_INFO;
        pop.exit_cb = exit_cb;
        popdlg_create(&pop);
#else
        exit_cb(POP_VAL_OK);
#endif
    }

    return ret;
}

int32_t app_cst_dialog_create(CSTOpt *opt)
{
    int32_t ret = -1;
    CSTCtrl *ctrl = &thiz_CSTCtrl;

    if(NULL == opt)
        return -1;

    ctrl->opt = opt;
    if(opt->itemNum > CST_ITEM_MAX)
        ctrl->item_total = CST_ITEM_MAX;
    else
        ctrl->item_total = opt->itemNum;

    ret = GUI_CreateDialog(WND_COMMON_SEARCH_SETTING);

    if(ctrl->opt->signal.create_cb)
        ret = ctrl->opt->signal.create_cb();
    return ret;
}
