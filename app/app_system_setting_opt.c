#include "app.h"
#include "module/app_time.h"
#include "app_wnd_system_setting_opt.h"
#include "module/app_play_control.h"
#include "app_module.h"
#include "app_book.h"
#include "app_channel_info.h"

#define IMG_BAR_HALF_FOCUS_L    "s_bar_half_focus_l"
#define IMG_BAR_HALF_UNFOCUS_L  "s_bar_half_unfocus"

#define TXT_BACK_TOP "text_system_setting_BG_top"
#define TEXT_SYS_TIME "text_system_setting_time"
#define TEXT_SYS_DATE "text_system_setting_ymd"

typedef enum
{
	SETTING_ITEM_1,
	SETTING_ITEM_2,
	SETTING_ITEM_3,
	SETTING_ITEM_4,
	SETTING_ITEM_5,
	SETTING_ITEM_6,
	SETTING_ITEM_7,
	SETTING_ITEM_MAX
}SettingOptSel;

static SystemSettingOpt *s_setting_opt = NULL;
static SettingOptSel s_sel = SETTING_ITEM_1;
static int s_item_total = 0;
static WndStatus s_system_setting_menu_state = WND_CANCLE;
static bool s_wnd_block = false;
static event_list* s_time_update_timer = NULL;

static event_list* s_system_setting_gif_timer = NULL;
static void app_system_setting_load_gif(void);
static void app_system_setting_free_gif(void);
static void app_system_setting_show_gif(void);
static void app_system_setting_hide_gif(void);
#define SYSTEM_SETTING_WIDGET_LEN   40
static char s_system_setting_wgt[SYSTEM_SETTING_WIDGET_LEN];
static int s_cur_page_first_item_index = 0;
static int* s_show_item_index_list = NULL;
static char** s_edit_item_string_list = NULL;
static char** s_btn_item_string_list = NULL;
static void app_system_set_hide_item(uint32_t index);

static int app_system_set_get_show_item_total(void)
{
    int num = 0;
    int sel = 0;

    for(sel=0; sel < s_setting_opt->itemNum; sel++)
    {
        if(s_setting_opt->item[sel].itemStatus != ITEM_HIDE)
            num++;
    }
    return num;
}

static int app_system_set_get_cur_page_show_item_num(void)
{
    int num=0;
    int show_item_num = 0;

    show_item_num = app_system_set_get_show_item_total();
    if((show_item_num - s_cur_page_first_item_index) > SETTING_ITEM_MAX)
        num = SETTING_ITEM_MAX;
    else
        num = show_item_num - s_cur_page_first_item_index;

    return num;
}

static int app_system_set_index_to_sel(uint32_t index)
{
    int i=0,j=0,sel=-1;
    int cur_page_item_num = 0;

    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
    for(i=s_cur_page_first_item_index;i<s_cur_page_first_item_index+cur_page_item_num;i++)
    {
        if(s_show_item_index_list[i] == index)
        {
            sel = j;
            break;
        }
        j++;
    }
    return sel;
}

int app_system_get_sel(void)
{
	return s_show_item_index_list[s_sel+s_cur_page_first_item_index];
}

int app_system_get_item_total_num(void)
{
	return ((s_setting_opt != NULL) ? (s_setting_opt->itemNum) : (0));
}

char *app_system_get_title(void)
{
	return ((s_setting_opt != NULL) ? (s_setting_opt->menuTitle) : (NULL));
}

static char *app_system_get_choice_back(uint32_t index)
{
    int sel = 0;

    sel = app_system_set_index_to_sel(index);
    if(sel != -1)
        snprintf(s_system_setting_wgt, SYSTEM_SETTING_WIDGET_LEN, "%s%u", "img_system_setting_item_choice", sel+1);
	return s_system_setting_wgt;
}

char *app_system_get_combobox(uint32_t index)
{
    int sel = 0;

    sel = app_system_set_index_to_sel(index);
    if(sel != -1)
        snprintf(s_system_setting_wgt, SYSTEM_SETTING_WIDGET_LEN, "%s%u", "cmb_system_setting_opt", sel+1);
	return s_system_setting_wgt;
}

char *app_system_get_edit(uint32_t index)
{
    int sel = 0;

    sel = app_system_set_index_to_sel(index);
    if(sel != -1)
        snprintf(s_system_setting_wgt, SYSTEM_SETTING_WIDGET_LEN, "%s%u", "edit_system_setting_opt", sel+1);
	return s_system_setting_wgt;
}

char *app_system_get_button(uint32_t index)
{
    int sel = 0;

    sel = app_system_set_index_to_sel(index);
    if(sel != -1)
        snprintf(s_system_setting_wgt, SYSTEM_SETTING_WIDGET_LEN, "%s%u", "btn_system_setting_opt", sel+1);
    return s_system_setting_wgt;
}

static char *app_system_get_interval(uint32_t index)
{
    int sel = 0;

    sel = app_system_set_index_to_sel(index);
    if(sel != -1)
        snprintf(s_system_setting_wgt, SYSTEM_SETTING_WIDGET_LEN, "%s%u", "img_system_setting_interval", sel+1);
	return s_system_setting_wgt;
}

char *app_system_get_item_title(uint32_t index)
{
    int sel = 0;

    sel = app_system_set_index_to_sel(index);
    if(sel != -1)
        snprintf(s_system_setting_wgt, SYSTEM_SETTING_WIDGET_LEN, "%s%u", "text_system_setting_item", sel+1);
    return s_system_setting_wgt;
}

static void app_system_set_update_page(void)
{
    int i = 0;
    int cur_page_item_num = 0;
    int cur_page_first_item_index_bak = s_cur_page_first_item_index;

    s_cur_page_first_item_index = 0;
    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
    for(i = 0; i < cur_page_item_num; i++)
    {
        app_system_set_hide_item(s_show_item_index_list[i+s_cur_page_first_item_index]);
    }
    s_cur_page_first_item_index = cur_page_first_item_index_bak;
    cur_page_item_num = app_system_set_get_cur_page_show_item_num();

    for(i = 0; i < cur_page_item_num; i++)
    {
        app_system_set_item_property(s_show_item_index_list[i+s_cur_page_first_item_index], &s_setting_opt->item[s_show_item_index_list[i+s_cur_page_first_item_index]].itemProperty);
        app_system_set_item_state_update(s_show_item_index_list[i+s_cur_page_first_item_index], s_setting_opt->item[s_show_item_index_list[i+s_cur_page_first_item_index]].itemStatus);
    }

    return;
}

static int app_system_set_get_normal_item_sel(int begin_sel, int end_sel)
{
    int sel = -1;
    int i;
    int cur_page_item_num = 0;

    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
    if(begin_sel < 0 || end_sel < 0 || begin_sel > cur_page_item_num-1 || end_sel > cur_page_item_num -1)
        return -1;
    if(begin_sel < end_sel)
    {
        for(i = begin_sel; i <= end_sel; i++)
        {
            if(s_setting_opt->item[s_show_item_index_list[i + s_cur_page_first_item_index]].itemStatus == ITEM_NORMAL)
            {
                sel = i;
                break;
            }
        }
    }
    else
    {
        for(i = begin_sel; i >= end_sel; i--)
        {
            if(s_setting_opt->item[s_show_item_index_list[i + s_cur_page_first_item_index]].itemStatus == ITEM_NORMAL)
            {
                sel = i;
                break;
            }
        }
    }
    return sel;
}

static int app_system_set_jump_to_last_page(void)
{
    SettingOptSel sel = SETTING_ITEM_1;
    int show_item_num = 0;
    int cur_page_item_num = 0;

    show_item_num = app_system_set_get_show_item_total();
    if(show_item_num % SETTING_ITEM_MAX)
        s_cur_page_first_item_index = show_item_num - (show_item_num % SETTING_ITEM_MAX);
    else
        s_cur_page_first_item_index = show_item_num - SETTING_ITEM_MAX;
    app_system_set_update_page();
    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
    sel = app_system_set_get_normal_item_sel(cur_page_item_num-1,0);
    sel = (sel == -1 ? SETTING_ITEM_1:sel);

    return sel;
}

static int app_system_set_jump_to_first_page(void)
{
    SettingOptSel sel = SETTING_ITEM_1;
    int cur_page_item_num = 0;

    s_cur_page_first_item_index = 0;
    app_system_set_update_page();
    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
    sel = app_system_set_get_normal_item_sel(0,cur_page_item_num-1);
    sel = (sel == -1 ? SETTING_ITEM_1:sel);

    return sel;
}

static int app_system_set_page_num_arrow(void)
{
    int show_item_num = 0;
    int cur_page_item_num = 0;

    show_item_num = app_system_set_get_show_item_total();
    if (show_item_num <= SETTING_ITEM_MAX)
    {
        return 0 ;
    }

    cur_page_item_num = app_system_set_get_cur_page_show_item_num();

    if ((cur_page_item_num + s_cur_page_first_item_index) <= SETTING_ITEM_MAX)
    {
        GUI_SetProperty("img_system_setting_arrow_up", "state", "hide");
        GUI_SetProperty("img_system_setting_arrow_down", "state", "show");
        return 0;
    }
    if ((cur_page_item_num + s_cur_page_first_item_index) >= show_item_num)
    {
        GUI_SetProperty("img_system_setting_arrow_up", "state", "show");
        GUI_SetProperty("img_system_setting_arrow_down", "state", "hide");
        return 0;
    }

    GUI_SetProperty("img_system_setting_arrow_up", "state", "show");
    GUI_SetProperty("img_system_setting_arrow_down", "state", "show");
    return 0;
}

static SettingOptSel app_system_set_next_sel(SettingOptSel cur_sel)
{
    SettingOptSel sel = SETTING_ITEM_1;
    int show_item_num = 0;
    int cur_page_item_num = 0;

    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
    show_item_num = app_system_set_get_show_item_total();
    if(show_item_num > SETTING_ITEM_MAX)
    {
        if(cur_sel == cur_page_item_num-1)
        {
            if(s_cur_page_first_item_index + 1 + cur_sel == show_item_num)
                sel = app_system_set_jump_to_first_page();
            else
            {
                s_cur_page_first_item_index++;
                app_system_set_update_page();
                sel = app_system_set_get_normal_item_sel(cur_page_item_num-1,0);
                sel = (sel == -1 ? SETTING_ITEM_1:sel);
            }
        }
        else
        {
            if(s_cur_page_first_item_index + cur_sel + 1== show_item_num)
                sel=app_system_set_jump_to_first_page();
            else
            {
                if(show_item_num - s_cur_page_first_item_index-1 >= SETTING_ITEM_MAX)
                {
                    sel = app_system_set_get_normal_item_sel(cur_sel+1,cur_page_item_num-1);
                    if(sel == -1)
                    {
                        s_cur_page_first_item_index++;
                        app_system_set_update_page();
                        sel = app_system_set_get_normal_item_sel(cur_sel,cur_page_item_num-1);
                        if(sel == -1)
                            sel = (cur_sel == 0 ? SETTING_ITEM_1: cur_sel-1);
                    }
                }
                else
                {
                    int item_num = show_item_num - s_cur_page_first_item_index;
                    sel = app_system_set_get_normal_item_sel(cur_sel+1,item_num-1);
                    if(sel == -1)
                        sel=app_system_set_jump_to_first_page();
                }
            }
        }
    }
    else
    {
        if(cur_sel == cur_page_item_num-1)
        {
            sel = app_system_set_get_normal_item_sel(0,cur_sel);
            sel = (sel == -1 ? cur_sel:sel);
        }
        else
        {
            sel = app_system_set_get_normal_item_sel(cur_sel+1,show_item_num-1);
            if(sel == -1)
            {
                sel = app_system_set_get_normal_item_sel(0,cur_sel);
                sel = (sel == -1 ? cur_sel : sel);
            }
        }
    }
    return sel;
}

static SettingOptSel app_system_set_last_sel(SettingOptSel cur_sel)
{
    SettingOptSel sel = SETTING_ITEM_1;
    int show_item_num = 0;
    int cur_page_item_num = 0;

    show_item_num = app_system_set_get_show_item_total();
    if(show_item_num > SETTING_ITEM_MAX)
    {
        if(cur_sel == SETTING_ITEM_1)
        {
            if(s_cur_page_first_item_index == 0)
                sel = app_system_set_jump_to_last_page();
            else
            {
                s_cur_page_first_item_index--;
                app_system_set_update_page();
                cur_page_item_num = app_system_set_get_cur_page_show_item_num();
                sel = app_system_set_get_normal_item_sel(0,cur_page_item_num-1);
                sel = (sel == -1 ? SETTING_ITEM_1:sel);
            }
        }
        else
        {
            sel = app_system_set_get_normal_item_sel(cur_sel-1,0);
            if(sel == -1)
            {
                if(s_cur_page_first_item_index == 0)
                    sel = app_system_set_jump_to_last_page();
                else
                {
                    s_cur_page_first_item_index--;
                    app_system_set_update_page();
                    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
                    sel = app_system_set_get_normal_item_sel(0,cur_page_item_num-1);
                    sel = (sel == -1 ? SETTING_ITEM_1:sel);
                }
            }
        }
    }
    else
    {
        if(cur_sel == SETTING_ITEM_1)
        {
            sel = app_system_set_get_normal_item_sel(show_item_num-1,cur_sel);
            sel = (sel == -1 ? cur_sel:sel);
        }
        else
        {
            sel = app_system_set_get_normal_item_sel(cur_sel-1,0);
            if(sel == -1)
            {
                sel = app_system_set_get_normal_item_sel(show_item_num-1,cur_sel);
                sel = (sel == -1 ? cur_sel:sel);
            }
        }
    }
    return sel;
}

static SettingOptSel app_system_set_last_page_sel(SettingOptSel cur_sel)
{
    SettingOptSel sel = SETTING_ITEM_1;
    int cur_page_item_num = 0;

    if(s_cur_page_first_item_index == 0)
    {
        if(cur_sel == 0)
            sel = app_system_set_jump_to_last_page();
        else
        {
            sel = app_system_set_get_normal_item_sel(0,cur_sel);
            if(sel == -1)
                sel = app_system_set_jump_to_last_page();
        }
    }
    else
    {
        int cur_sel_page = (s_cur_page_first_item_index+cur_sel)/SETTING_ITEM_MAX+1;
        int new_sel = s_cur_page_first_item_index + cur_sel-(cur_sel_page-1)*SETTING_ITEM_MAX;
        int last_page = cur_sel_page-1 > 0 ?cur_sel_page-1:1;
        s_cur_page_first_item_index = (last_page-1)*SETTING_ITEM_MAX;
        app_system_set_update_page();
        cur_page_item_num = app_system_set_get_cur_page_show_item_num();
        if(cur_sel_page >1)
        {
            if(new_sel < cur_page_item_num && s_setting_opt->item[s_show_item_index_list[s_cur_page_first_item_index+new_sel]].itemStatus == ITEM_NORMAL)
                sel = new_sel;
        }
        else
        {
            sel = app_system_set_get_normal_item_sel(0,cur_page_item_num-1);
            sel = (sel == -1 ? SETTING_ITEM_1:sel);
        }
    }

    return sel;
}

static SettingOptSel app_system_set_next_page_sel(SettingOptSel cur_sel)
{
    SettingOptSel sel = SETTING_ITEM_1;
    int show_item_num = 0;
    int cur_page_item_num = 0;

    show_item_num = app_system_set_get_show_item_total();
    if(s_cur_page_first_item_index+SETTING_ITEM_MAX+1 > show_item_num)
    {
        if(s_cur_page_first_item_index + cur_sel+1 == show_item_num)
            sel=app_system_set_jump_to_first_page();
        else
        {
            cur_page_item_num = app_system_set_get_cur_page_show_item_num();
            sel = app_system_set_get_normal_item_sel(cur_page_item_num-1,cur_sel+1);
            if(sel == -1)
                sel=app_system_set_jump_to_first_page();
        }
    }
    else
    {
        int cur_sel_page = (s_cur_page_first_item_index+cur_sel)/SETTING_ITEM_MAX+1;
        int new_sel = s_cur_page_first_item_index + cur_sel-(cur_sel_page-1)*SETTING_ITEM_MAX;
        s_cur_page_first_item_index = cur_sel_page*SETTING_ITEM_MAX;
        if(s_cur_page_first_item_index > show_item_num - 1)
            s_cur_page_first_item_index = (cur_sel_page-1)*SETTING_ITEM_MAX;
        app_system_set_update_page();
        cur_page_item_num = app_system_set_get_cur_page_show_item_num();
        if(new_sel < cur_page_item_num && s_setting_opt->item[s_show_item_index_list[s_cur_page_first_item_index+new_sel]].itemStatus == ITEM_NORMAL)
        {
            sel = new_sel;
        }
        else
        {
            sel = app_system_set_get_normal_item_sel(0,cur_page_item_num-1);
            sel = (sel == -1 ? SETTING_ITEM_1:sel);
        }
    }
    return sel;
}


char *app_system_get_item_value(uint32_t index)
{
    int cur_page_item_num = 0;

    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
    if(index < s_show_item_index_list[s_cur_page_first_item_index] || index  > s_show_item_index_list[s_cur_page_first_item_index + cur_page_item_num - 1])
        return NULL;
	if(s_setting_opt->item[index].itemType == ITEM_CHOICE
			|| s_setting_opt->item[index].itemType == ITEM_POP_CHOICE)
	{
		return app_system_get_combobox(index);
	}
	else if(s_setting_opt->item[index].itemType == ITEM_EDIT)
	{
		return app_system_get_edit(index);
	}
	else if(s_setting_opt->item[index].itemType == ITEM_PUSH)
	{
		return app_system_get_button(index);
	}
	return NULL;
}

static void app_system_set_show_item(uint32_t index)
{
#define NORMAL_FORE_COLOR "[text_color,text_color,text_color]"
#define DISABLE_FORE_COLOR "[text_disable_color,text_disable_color,text_disable_color]"
	char *state = NULL;
	char *fore_color = NULL;
	char *item_widget = NULL;

	if(s_setting_opt->item[index].itemStatus == ITEM_DISABLE)
	{
		state = "disable";
		fore_color = DISABLE_FORE_COLOR;
	}
	else
	{
		state = "enable";
		fore_color = NORMAL_FORE_COLOR;
	}
	GUI_SetProperty(app_system_get_choice_back(index), "state", "show");
	GUI_SetProperty(app_system_get_item_title(index), "state", "show");
	GUI_SetProperty(app_system_get_interval(index), "state", "show");
	item_widget = app_system_get_item_value(index);
	GUI_SetProperty(item_widget, "state",  "show");
	GUI_SetProperty(item_widget, "state", state);
	GUI_SetProperty(item_widget, "forecolor", fore_color);
	GUI_SetProperty(app_system_get_item_title(index), "forecolor", fore_color);
	GUI_SetProperty(app_system_get_item_title(index), "string", s_setting_opt->item[index].itemTitle);
	GUI_SetProperty(app_system_get_choice_back(index), "state", "show");
	GUI_SetProperty(app_system_get_choice_back(index), "img", IMG_BAR_HALF_UNFOCUS_L);
}

static void app_system_set_hide_item(uint32_t index)
{
	GUI_SetProperty(app_system_get_item_title(index), "state", "hide");
	GUI_SetProperty(app_system_get_combobox(index), "state", "hide");
	GUI_SetProperty(app_system_get_edit(index), "state", "hide");
	GUI_SetProperty(app_system_get_button(index), "state", "hide");
	GUI_SetProperty(app_system_get_choice_back(index), "state", "hide");
	GUI_SetProperty(app_system_get_interval(index), "state", "hide");
}


static void app_system_set_show_funckey(void)
{
	if(s_setting_opt->menuFuncKey.redKey.keyPressFun != NULL)
	{
		GUI_SetProperty("text_system_setting_red", "string", s_setting_opt->menuFuncKey.redKey.keyName);
		GUI_SetProperty("text_system_setting_red", "state", "show");
		GUI_SetProperty("img_system_setting_red", "state", "show");
	}

	if(s_setting_opt->menuFuncKey.greenKey.keyPressFun != NULL)
	{
		GUI_SetProperty("text_system_setting_green", "string", s_setting_opt->menuFuncKey.greenKey.keyName);
		GUI_SetProperty("text_system_setting_green", "state", "show");
		GUI_SetProperty("img_system_setting_green", "state", "show");
	}

	if(s_setting_opt->menuFuncKey.blueKey.keyPressFun != NULL)
	{
		GUI_SetProperty("text_system_setting_blue", "string", s_setting_opt->menuFuncKey.blueKey.keyName);
		GUI_SetProperty("text_system_setting_blue", "state", "show");
		GUI_SetProperty("img_system_setting_blue", "state", "show");
	}

	if(s_setting_opt->menuFuncKey.yellowKey.keyPressFun != NULL)
	{
		GUI_SetProperty("text_system_setting_yellow", "string", s_setting_opt->menuFuncKey.yellowKey.keyName);
		GUI_SetProperty("text_system_setting_yellow", "state", "show");
		GUI_SetProperty("img_system_setting_yellow", "state", "show");
	}
}

static int app_system_set_update_timer(void* usrdata)
{
	char sys_time_str[24] = {0};
	char time_str_temp[24] = {0};

	g_AppTime.time_get(&g_AppTime, sys_time_str, sizeof(sys_time_str));

	// time
	if((strlen(sys_time_str) > 24) || (strlen(sys_time_str) < 5))
	{
		app_log_error("\nlenth error\n");
		return 1;
	}
	memcpy(time_str_temp,&sys_time_str[strlen(sys_time_str)-5],5);// 00:00
	GUI_SetProperty(TEXT_SYS_TIME, "string", time_str_temp);

	// ymd
	memcpy(time_str_temp,sys_time_str,strlen(sys_time_str));// 0000/00/00
	time_str_temp[strlen(sys_time_str)-5] = '\0';
	GUI_SetProperty(TEXT_SYS_DATE, "string", time_str_temp);

	return 0;
}

static void _system_setting_ctrl_free(void)
{
    int i;
    APP_FREE(s_show_item_index_list)
    for(i = 0; i < s_item_total; i++)
    {
        APP_FREE(s_edit_item_string_list[i]);
    }
    APP_FREE(s_edit_item_string_list);
    for(i = 0; i < s_item_total; i++)
    {
        APP_FREE(s_btn_item_string_list[i]);
    }
    APP_FREE(s_btn_item_string_list);
}

static int app_system_set_init(SystemSettingOpt *settingOpt)
{
	int sel;
    int cur_page_item_num = 0;
    int i=0,j=0;

	if(settingOpt == NULL)
		return -1;

    s_sel = SETTING_ITEM_1;
	s_setting_opt = settingOpt;
    s_cur_page_first_item_index = 0;
    s_item_total = s_setting_opt->itemNum;
    s_show_item_index_list = (int *)GxCore_Mallocz(s_item_total*sizeof(int));
    s_edit_item_string_list = (char**)GxCore_Mallocz(s_item_total*sizeof(char*));
    s_btn_item_string_list = (char**)GxCore_Mallocz(s_item_total*sizeof(char*));
    if(s_show_item_index_list == NULL || s_edit_item_string_list == NULL || s_btn_item_string_list == NULL)
        goto err;
    for(i = 0; i < s_item_total; i++)
    {
        if(s_setting_opt->item[i].itemStatus != ITEM_HIDE)
        {
            s_show_item_index_list[j] = i;
            j++;
        }
    }
    for(i = 0; i < s_setting_opt->itemNum; i++)
    {
        if(s_setting_opt->item[i].itemStatus != ITEM_HIDE)
        {
            if(s_setting_opt->item[i].itemType == ITEM_EDIT)
            {
                int len = atoi(s_setting_opt->item[i].itemProperty.itemPropertyEdit.maxlen);
                s_edit_item_string_list[i] = (char*)GxCore_Mallocz(sizeof(char)*(len+1));
                if(NULL == s_edit_item_string_list[i])
                    goto err;
                if(s_setting_opt->item[i].itemProperty.itemPropertyEdit.string)
                    memcpy(s_edit_item_string_list[i],s_setting_opt->item[i].itemProperty.itemPropertyEdit.string,len);
            }
            else if(s_setting_opt->item[i].itemType == ITEM_PUSH)
            {
                int len = strlen(s_setting_opt->item[i].itemProperty.itemPropertyBtn.string);
                s_btn_item_string_list[i] = (char*)GxCore_Mallocz(sizeof(char)*(len+1));
                if(NULL == s_btn_item_string_list[i])
                    goto err;
                if(s_setting_opt->item[i].itemProperty.itemPropertyBtn.string)
                    memcpy(s_btn_item_string_list[i],s_setting_opt->item[i].itemProperty.itemPropertyBtn.string,len);
            }
        }
    }
	//s_wnd_open_count++;
	if(GXCORE_SUCCESS != GUI_CheckDialog(WND_SYSTEM_SETTING))
	{
		GUI_CreateDialog(WND_SYSTEM_SETTING);
		app_system_reset_unfocus_image();
		GUI_SetProperty("text_system_setting_timezone","state","hide"/*"osd_trans_hide"*/);
		GUI_SetProperty("img_system_setting_timezone_back","state","hide"/*"osd_trans_hide"*/);
        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_MAIN_MENU))
        {
            if(GUI_CheckDialog(WND_WEATHER) == GXCORE_SUCCESS
                    || GUI_CheckDialog(WND_COMMON_VIEW) == GXCORE_SUCCESS
                    || GUI_CheckDialog(WND_APPLETS) == GXCORE_SUCCESS
                    || GUI_CheckDialog(WND_IPTV_BROWSER) == GXCORE_SUCCESS)
            {
                uint32_t y = APP_WND_SYSTEM_SETTING_POS_Y;
                GUI_SetProperty(WND_SYSTEM_SETTING, "move_window_y", &y);
                GUI_SetProperty("img_system_setting_opt_b1", "state", "show");
            }
            else if(app_channel_epg_check_dialog() == GXCORE_SUCCESS)
            {
                GUI_SetProperty("img_system_setting_opt_b1", "state", "show");
            }
            else
            {
                GUI_SetProperty("img_system_setting_opt_b1", "state", "hide");
            }
        }
        else
        {
            if(app_channel_epg_check_dialog() == GXCORE_SUCCESS || GXCORE_SUCCESS == GUI_CheckDialog(WND_MERGE_DB))
                GUI_SetProperty("img_system_setting_opt_b1", "state", "show");
            else
                GUI_SetProperty("img_system_setting_opt_b1", "state", "hide");
        }
    }
	else
	{
		for(sel = 0; sel < SETTING_ITEM_MAX; sel++)
		{
            app_system_set_hide_item(s_show_item_index_list[sel+s_cur_page_first_item_index]);
		}
	}

	if(s_setting_opt->titleImage != NULL)
	{
		GUI_SetProperty("img_system_setting_title", "img", s_setting_opt->titleImage);
	}
	else
	{
        GUI_SetProperty("img_system_setting_title", "img", "s_title_left_setting");
	}
	if(s_setting_opt->backGround == BG_PLAY_RPOG)
	{
        GUI_SetProperty(TXT_BACK_TOP, "backcolor", "[#3a3d4a,#3a3d4a,#3a3d4a]");
	}
	else
	{
        GUI_SetProperty(TXT_BACK_TOP, "backcolor", "[#17174A,#17174A,#17174A]");
	}
	if(s_setting_opt->topTipDisplay == TIP_SHOW)
	{
		GUI_SetProperty("text_system_setting_title", "string", s_setting_opt->menuTitle);
		GUI_SetProperty("text_system_setting_title", "state", "show");
        if(TIP_SHOW == s_setting_opt->topTipImgDisplay)
            GUI_SetProperty("img_system_setting_title", "state", "show");
	}
    else
    {
		GUI_SetProperty("text_system_setting_title", "state", "hide");
		GUI_SetProperty("img_system_setting_title", "state", "hide");
    }
	if(s_setting_opt->bottmTipDisplay == TIP_SHOW)
	{
		GUI_SetProperty("img_system_setting_bottom", "state", "show");
	}
    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
	for(sel = 0; sel < cur_page_item_num; sel++)
	{
		app_system_set_item_property(s_show_item_index_list[sel+s_cur_page_first_item_index], &s_setting_opt->item[s_show_item_index_list[sel+s_cur_page_first_item_index]].itemProperty);
        app_system_set_item_state_update(s_show_item_index_list[sel+s_cur_page_first_item_index], s_setting_opt->item[s_show_item_index_list[sel+s_cur_page_first_item_index]].itemStatus);
	}
	app_system_set_focus_item(s_show_item_index_list[s_sel+s_cur_page_first_item_index]);
	app_system_set_show_funckey();
	if(s_setting_opt->timeDisplay == TIP_SHOW)
	{
		app_system_set_update_timer(NULL);
		GUI_SetProperty(TEXT_SYS_TIME, "state", "show");
		GUI_SetProperty(TEXT_SYS_DATE, "state", "show");
		s_time_update_timer = create_timer(app_system_set_update_timer, 1000, NULL, TIMER_REPEAT);
	}
	app_system_set_page_num_arrow();

	if(s_setting_opt->gifDisplay == GIF_SHOW)
	{
		app_system_setting_load_gif();
	}
	return 0;
err:
    _system_setting_ctrl_free();
    return -1;
}

static void app_system_setting_load_gif(void)
{
#define GIF_PATH WORK_PATH"theme/image/netapps/loading.gif"
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty("img_system_setting_gif", "load_img", GIF_PATH);
	GUI_SetProperty("img_system_setting_gif", "init_gif_alu_mode", &alu);
}

static void app_system_setting_free_gif(void)
{
	GUI_SetProperty("img_system_setting_gif", "load_img", NULL);
	GUI_SetProperty("img_system_setting_gif", "state", "hide");
	if(s_system_setting_gif_timer != NULL)
	{
		remove_timer(s_system_setting_gif_timer);
		s_system_setting_gif_timer = NULL;
	}
}

static int app_system_setting_draw_gif(void* usrdata)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty("img_system_setting_gif", "draw_gif", &alu);
	return 0;
}

static void app_system_setting_show_gif(void)
{
	GUI_SetProperty("img_system_setting_gif", "state", "show");
	if(0 != reset_timer(s_system_setting_gif_timer))
	{
		s_system_setting_gif_timer = create_timer(app_system_setting_draw_gif, 100, NULL, TIMER_REPEAT);
	}
}

static void app_system_setting_hide_gif(void)
{
	//remove_timer(s_system_setting_gif_timer);
	//s_system_setting_gif_timer = NULL;

	if(s_system_setting_gif_timer != NULL)
		timer_stop(s_system_setting_gif_timer);

	GUI_SetProperty("img_system_setting_gif", "state", "hide");
}

void app_system_setting_gif_show(void)
{
	app_system_setting_show_gif();
}

void app_system_setting_gif_hide(void)
{
	app_system_setting_hide_gif();
}

void app_system_set_item_property(int index, ItemProPerty *property)
{
    char *item_widget = NULL;
    int cur_page_item_num = 0;
    int edit_maxlen_src = 0;
    int btn_maxlen_src = 0;

// 配合“GUI_EndDialog("after xxxx")”功能，阻塞菜单后续的代码有可能会滞后执行，如果指针操作异常
// 后续公版去掉阻塞菜单后，该部分的处理需要重新评估
    if (GUI_CheckDialog(WND_SYSTEM_SETTING) != GXCORE_SUCCESS)
        return ;

    if(s_setting_opt->item[index].itemType == ITEM_EDIT)
        edit_maxlen_src = atoi(s_setting_opt->item[index].itemProperty.itemPropertyEdit.maxlen);
    if(s_setting_opt->item[index].itemType == ITEM_PUSH)
        btn_maxlen_src = strlen(s_btn_item_string_list[index]);
    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
    memmove(&s_setting_opt->item[index].itemProperty, property, sizeof(ItemProPerty));
    if(index < s_show_item_index_list[s_cur_page_first_item_index] || index  > s_show_item_index_list[s_cur_page_first_item_index + cur_page_item_num - 1])
        return;
    item_widget = app_system_get_item_value(index);
	if((s_setting_opt->item[index].itemType == ITEM_CHOICE)||(s_setting_opt->item[index].itemType == ITEM_POP_CHOICE))
	{
		if(property->itemPropertyCmb.content != NULL)
		{
			GUI_SetProperty(item_widget, "content", property->itemPropertyCmb.content);
		}
        GUI_SetProperty(item_widget, "select", &(property->itemPropertyCmb.sel));
	}
	else if(s_setting_opt->item[index].itemType == ITEM_EDIT)
	{
		GUI_SetProperty(item_widget, "clear", NULL);

		if(property->itemPropertyEdit.format != NULL)
		{
			GUI_SetProperty(item_widget, "format", property->itemPropertyEdit.format);
		}

		if(property->itemPropertyEdit.maxlen != NULL)
		{
			GUI_SetProperty(item_widget, "maxlen", property->itemPropertyEdit.maxlen);
            int len = atoi(property->itemPropertyEdit.maxlen);
            if(len > edit_maxlen_src)
            {
                s_edit_item_string_list[index] = GxCore_Realloc(s_edit_item_string_list[index],len+1);
                if(NULL == s_edit_item_string_list[index])
                    return;
                edit_maxlen_src = len;
            }
		}

		if(property->itemPropertyEdit.intaglio != NULL)
		{
			GUI_SetProperty(item_widget, "intaglio", property->itemPropertyEdit.intaglio);
		}
		else
		{
			GUI_SetProperty(item_widget, "intaglio", "");
		}

		if(property->itemPropertyEdit.default_intaglio != NULL)
		{
			GUI_SetProperty(item_widget, "default_intaglio", property->itemPropertyEdit.default_intaglio);
		}
		else
		{
			GUI_SetProperty(item_widget, "default_intaglio", "");
		}
		if(property->itemPropertyEdit.string != NULL)
		{
            if(strcmp(property->itemPropertyEdit.string, s_edit_item_string_list[index]))
            {
                int len = atoi(property->itemPropertyEdit.maxlen);
                if(len > edit_maxlen_src)
                {
                    APP_FREE(s_edit_item_string_list[index]);
                    s_edit_item_string_list[index] =(char*)GxCore_Mallocz(sizeof(char)*(len+1));
                    if(NULL == s_edit_item_string_list[index])
                    {
                        app_log_error("malloc failed\n");
                        return;
                    }
                }
                memset(s_edit_item_string_list[index],0,edit_maxlen_src);
                memcpy(s_edit_item_string_list[index],property->itemPropertyEdit.string,len);
            }
            s_setting_opt->item[index].itemProperty.itemPropertyEdit.string = s_edit_item_string_list[index];
            GUI_SetProperty(item_widget, "string", s_edit_item_string_list[index]);
		}
	}
	else if(s_setting_opt->item[index].itemType == ITEM_PUSH)
	{
		if(property->itemPropertyBtn.string != NULL)
		{
            if(strcmp(property->itemPropertyBtn.string,s_btn_item_string_list[index]))
            {
                int len = strlen(property->itemPropertyBtn.string);
                if(len > btn_maxlen_src)
                {
                    APP_FREE(s_btn_item_string_list[index]);
                    s_btn_item_string_list[index] =(char*)GxCore_Mallocz(sizeof(char)*(len+1));
                    if(NULL == s_btn_item_string_list[index])
                    {
                        app_log_error("malloc failed\n");
                        return;
                    }
                }
                memset(s_btn_item_string_list[index],0,btn_maxlen_src);
                memcpy(s_btn_item_string_list[index],property->itemPropertyBtn.string,len);
            }
            s_setting_opt->item[index].itemProperty.itemPropertyBtn.string = s_btn_item_string_list[index];
            GUI_SetProperty(item_widget, "string", property->itemPropertyBtn.string);
        }
    }
}

void app_system_set_item_title(int index, char *new_title)
{
    s_setting_opt->item[index].itemTitle = new_title;
    if(index < s_show_item_index_list[s_cur_page_first_item_index] || index > s_show_item_index_list[s_cur_page_first_item_index + app_system_set_get_cur_page_show_item_num() - 1])
        return;
    GUI_SetProperty(app_system_get_item_title(index), "string", s_setting_opt->item[index].itemTitle);
}

void app_system_set_item_data_update(int index)
{
    if(index < s_show_item_index_list[s_cur_page_first_item_index] || index > s_show_item_index_list[s_cur_page_first_item_index + app_system_set_get_cur_page_show_item_num() - 1])
        return;
    char *str = NULL;
    int len = 0;

    char *item_widget = app_system_get_item_value(index);
    if((s_setting_opt->item[index].itemType == ITEM_CHOICE)||(s_setting_opt->item[index].itemType == ITEM_POP_CHOICE))
    {
        GUI_GetProperty(item_widget, "select", &s_setting_opt->item[index].itemProperty.itemPropertyCmb.sel);
    }
    else if(s_setting_opt->item[index].itemType == ITEM_EDIT)
    {
        GUI_GetProperty(item_widget, "string", &str);
        if(s_edit_item_string_list[index] != NULL && str != NULL)
        {
            len = atoi(s_setting_opt->item[index].itemProperty.itemPropertyEdit.maxlen);
            memset(s_edit_item_string_list[index],0,len);
            memcpy(s_edit_item_string_list[index], str, strlen(str));
            s_setting_opt->item[index].itemProperty.itemPropertyEdit.string = s_edit_item_string_list[index];
        }
    }
    else if(s_setting_opt->item[index].itemType == ITEM_PUSH)
    {
        GUI_GetProperty(item_widget, "string", &str);
        if(s_btn_item_string_list[index] != NULL && str != NULL)
        {
            memset(s_btn_item_string_list[index],0,strlen(s_btn_item_string_list[index]));
            memcpy(s_btn_item_string_list[index],str,strlen(str));
            s_setting_opt->item[index].itemProperty.itemPropertyBtn.string = s_btn_item_string_list[index];
        }
    }
}

void app_system_set_item_state_update(int index, ItemDisplayStatus state)
{
    int i=0,j=0;
    int cur_page_item_num = 0;

    if((s_setting_opt->item[index].itemStatus == ITEM_HIDE && state != ITEM_HIDE)||(s_setting_opt->item[index].itemStatus != ITEM_HIDE && state == ITEM_HIDE))
    {
        cur_page_item_num = app_system_set_get_cur_page_show_item_num();
        for(i = 0; i < cur_page_item_num; i++)
        {
            app_system_set_hide_item(s_show_item_index_list[i+s_cur_page_first_item_index]);
        }
        s_setting_opt->item[index].itemStatus = state;
        memset(s_show_item_index_list,0,s_item_total*sizeof(int));
        for(i = 0; i < s_item_total; i++)
        {
            if(s_setting_opt->item[i].itemStatus != ITEM_HIDE)
            {
                s_show_item_index_list[j] = i;
                j++;
            }
        }
        cur_page_item_num = app_system_set_get_cur_page_show_item_num();
        for(i = 0; i < cur_page_item_num; i++)
        {
            app_system_set_item_property(s_show_item_index_list[i+s_cur_page_first_item_index], &s_setting_opt->item[s_show_item_index_list[i+s_cur_page_first_item_index]].itemProperty);
            if(s_setting_opt->item[s_show_item_index_list[i+s_cur_page_first_item_index]].itemStatus == ITEM_HIDE)
            {
                app_system_set_hide_item(s_show_item_index_list[i+s_cur_page_first_item_index]);
            }
            else
            {
                app_system_set_show_item(s_show_item_index_list[i+s_cur_page_first_item_index]);
            }
        }
        return;
    }
    s_setting_opt->item[index].itemStatus = state;
    if(index < s_show_item_index_list[s_cur_page_first_item_index] || index > s_show_item_index_list[s_cur_page_first_item_index + app_system_set_get_cur_page_show_item_num() - 1])
        return;

    if(state == ITEM_HIDE)
    {
        app_system_set_hide_item(index);
    }
    else
    {
        app_system_set_show_item(index);
    }
}

void app_system_set_focus_item(int index)
{
    if(index < s_show_item_index_list[s_cur_page_first_item_index] || index > s_show_item_index_list[s_cur_page_first_item_index + app_system_set_get_cur_page_show_item_num() - 1])
        return;
#define SS_FOCUS_FORE_COLOR "[text_focus_color,text_focus_color,text_disable_color]"
	if(s_setting_opt->item[index].itemStatus == ITEM_NORMAL)
	{
		char *item_widget = app_system_get_item_value(index);
		GUI_SetProperty(item_widget, "state", "focus");
		GUI_SetProperty(item_widget, "forecolor", SS_FOCUS_FORE_COLOR);
		GUI_SetProperty(app_system_get_item_title(index), "string", s_setting_opt->item[index].itemTitle);
		GUI_SetProperty(app_system_get_item_title(index), "forecolor", SS_FOCUS_FORE_COLOR);
		GUI_SetProperty(app_system_get_choice_back(index), "img", IMG_BAR_HALF_FOCUS_L);
		s_sel = app_system_set_index_to_sel(index);
	}
}

void app_system_set_unfocus_item(int index)
{
    if(index < s_show_item_index_list[s_cur_page_first_item_index] || index > s_show_item_index_list[s_cur_page_first_item_index + app_system_set_get_cur_page_show_item_num() - 1])
        return;
#define SS_UNFOCUS_FORE_COLOR "[text_color,text_color,text_disable_color]"
	if(s_setting_opt->item[index].itemStatus == ITEM_NORMAL)
	{
		GUI_SetProperty(app_system_get_item_value(index), "forecolor", SS_UNFOCUS_FORE_COLOR);
		GUI_SetProperty(app_system_get_item_title(index), "string", s_setting_opt->item[index].itemTitle);
		GUI_SetProperty(app_system_get_item_title(index), "forecolor", SS_UNFOCUS_FORE_COLOR);
		GUI_SetProperty(app_system_get_choice_back(index), "img", IMG_BAR_HALF_UNFOCUS_L);
	}
}

void app_system_set_wnd_update(void)
{
	GUI_SetProperty(WND_SYSTEM_SETTING, "update", NULL);
}

void app_system_set_item_event(int index, GUI_Event *event)
{
	static GUI_Event eve = {0};
	char *item_widget = app_system_get_item_value(index);
	if(event == NULL || item_widget == NULL)
	{
		memset(&eve, 0, sizeof(GUI_Event));
		return;
	}
	memcpy(&eve, event, sizeof(GUI_Event));
	GUI_SendEvent(item_widget, &eve);
}

void app_system_set_wnd_event(GUI_Event *event)
{
	static GUI_Event eve = {0};

	if(event == NULL)
	{
		memset(&eve, 0, sizeof(GUI_Event));
		return;
	}

	memcpy(&eve, event, sizeof(GUI_Event));
	GUI_SendEvent(WND_SYSTEM_SETTING, &eve);
}

//gui 阻塞方式，适合需要返回值的情况
static bool s_sys_block_book = false;
WndStatus app_system_set_block_create(SystemSettingOpt *settingOpt)
{
	if(app_system_set_init(settingOpt) < 0)
	{
		if(s_time_update_timer != NULL)
		{
			remove_timer(s_time_update_timer);
			s_time_update_timer = NULL;
		}
		return WND_CANCLE;
	}
	s_wnd_block = true;
	s_system_setting_menu_state = WND_EXEC;

	app_block_msg_destroy(g_app_msg_self);
	while(s_system_setting_menu_state == WND_EXEC)
	{
		GUI_LoopEvent();
		GxCore_ThreadDelay(50);
	}
	GUI_StartSchedule();
	if(s_sys_block_book == true)
	{
		s_sys_block_book = false;
	}
	else
	{
		app_block_msg_init(g_app_msg_self);
		app_system_reset_unfocus_image();
		GUI_EndDialog(WND_SYSTEM_SETTING);
		GUI_SetInterface("flush",NULL);
	}
	return s_system_setting_menu_state;
}
//IMG_BAR_HALF_UNFOCUS_R
static char* _app_system_get_img_bar_har_unfocus_r(void)
{
    if( APP_THEME_CLASSIC_PURPLE || APP_THEME_SKY_BLUE || APP_THEME_GRID_PURPLE ){
        return "[s_button_unfocus,s_button_unfocus,s_button_unfocus]" ;
    }
    else{
        return "s_bar_half_unfocus" ;
    }
}

void app_system_reset_unfocus_image(void)
{
	char *pImg = _app_system_get_img_bar_har_unfocus_r() ;

	GUI_SetProperty(app_system_get_item_value(s_show_item_index_list[s_sel+s_cur_page_first_item_index]), "unfocus_img", pImg);
}

void app_system_reset_focus_image(void)
{
	char *pImg = NULL ;
	if( APP_THEME_CLASSIC_PURPLE || APP_THEME_SKY_BLUE || APP_THEME_GRID_PURPLE ){
		pImg = "[T2_64_button_menu_m,T2_64_button_menu_m,T2_64_button_menu_r]" ;
	}
	else{
		pImg = "s_bar_half_focus_r" ;
	}

	GUI_SetProperty(app_system_get_item_value(s_show_item_index_list[s_sel+s_cur_page_first_item_index]), "unfocus_img", pImg);
}

bool app_system_set_callback_handler(CheckCb check_cb, ExitCb exit_cb)
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
    else
    {
        app_system_reset_unfocus_image();
        GUI_EndDialog(WND_SYSTEM_SETTING);
        GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
    }

    return ret;
}

WndStatus app_system_set_create(SystemSettingOpt *settingOpt)
{
	WndStatus ret = WND_OK;

	if(app_system_set_init(settingOpt) < 0)
	{
		if(s_time_update_timer != NULL)
		{
			remove_timer(s_time_update_timer);
			s_time_update_timer = NULL;
		}
		ret = WND_CANCLE;
	}
	s_wnd_block = false;
	return ret;
}

void app_system_set_destroy(ExitType exit_type)
{
	WndStatus menu_ret = WND_CANCLE;
    int exit_ret = EXIT_RET_DEFAULT;
	int i;
    int cur_page_item_num = 0;
    char *unfocus_item = NULL;

    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
	for(i = 0; i < cur_page_item_num; i++)
	{
		app_system_set_item_data_update(s_show_item_index_list[s_cur_page_first_item_index+i]);
	}
	menu_ret = WND_CANCLE;
    unfocus_item = app_system_get_item_value(s_show_item_index_list[s_sel+s_cur_page_first_item_index]);
	if(s_setting_opt->exit != NULL)
	{
        exit_ret = s_setting_opt->exit(exit_type);
		if(EXIT_RET_BLOCK == exit_ret)
		{
			menu_ret = WND_OK;
		}
	}
	if(s_wnd_block)
	{
		s_system_setting_menu_state = menu_ret;
	}
	else
	{
        if(exit_ret != EXIT_RET_POP)
        {
            GUI_SetProperty(unfocus_item, "unfocus_img", _app_system_get_img_bar_har_unfocus_r());
            GUI_EndDialog(WND_SYSTEM_SETTING);
            GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
        }
	}
    app_system_setting_free_gif();
}

void app_system_trriger_func(void)
{
    int cur_page_item_num = 0;
    int i = 0;
    char *unfocus_item = NULL;

    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
    for(i = 0; i < cur_page_item_num; i++)
    {
        app_system_set_item_data_update(s_show_item_index_list[s_cur_page_first_item_index+i]);
    }
    unfocus_item = app_system_get_item_value(s_show_item_index_list[s_sel+s_cur_page_first_item_index]);
    if(s_setting_opt->exit != NULL)
    {
        s_setting_opt->exit(EXIT_ABANDON);
    }
    if(s_wnd_block)
    {
        app_block_msg_init(g_app_msg_self);
        s_sys_block_book = true;
        s_system_setting_menu_state = WND_CANCLE;
        GUI_SetProperty(unfocus_item, "unfocus_img", _app_system_get_img_bar_har_unfocus_r());
        if(app_channel_epg_check_dialog() != GXCORE_SUCCESS)
        {
            GUI_EndDialog("after wnd_full_screen");
            GUI_SetInterface("flush", NULL);
        }
        else
        {
            GUI_EndDialog("after wnd_full_screen");
            PlayerWindow video_wnd = {0, 0, APP_THEME_XRES, APP_THEME_YRES};
            GUI_SetInterface("flush", NULL);
            g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
            GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
        }
    }
    else
    {
        GUI_SetProperty(unfocus_item, "unfocus_img", _app_system_get_img_bar_har_unfocus_r());
        GUI_EndDialog("after wnd_full_screen");
        GUI_SetInterface("flush", NULL);
    }
}

SIGNAL_HANDLER int app_system_set_real_destroy(GuiWidget *widget, void *usrdata)
{
    s_sel = SETTING_ITEM_1;
	if(s_time_update_timer != NULL)
	{
		remove_timer(s_time_update_timer);
		s_time_update_timer = NULL;
	}
    _system_setting_ctrl_free();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_system_set_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_STOP;
	int i;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
            case STBK_UP:
                {
                    int cur_page_item_num = 0;

                    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
                    app_system_set_unfocus_item(s_show_item_index_list[s_sel+s_cur_page_first_item_index]);
                    for(i = 0; i < cur_page_item_num; i++)
                    {
                        app_system_set_item_data_update(s_show_item_index_list[s_cur_page_first_item_index+i]);
                    }
                    s_sel = app_system_set_last_sel(s_sel);
                    app_system_set_focus_item(s_show_item_index_list[s_sel+s_cur_page_first_item_index]);
                    app_system_set_page_num_arrow();
                }
                break;
            case STBK_DOWN:
                {
                    int cur_page_item_num = 0;

                    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
                    app_system_set_unfocus_item(s_show_item_index_list[s_sel+s_cur_page_first_item_index]);
                    for(i = 0; i < cur_page_item_num; i++)
                    {
                    app_system_set_item_data_update(s_show_item_index_list[s_cur_page_first_item_index+i]);
                    }
                    s_sel = app_system_set_next_sel(s_sel);
                    app_system_set_focus_item(s_show_item_index_list[s_sel+s_cur_page_first_item_index]);
                    app_system_set_page_num_arrow();
                }
                break;
            case STBK_PAGE_UP:
                {
                    if(app_system_set_get_show_item_total() > SETTING_ITEM_MAX)
                    {
                        int cur_page_item_num = 0;

                        cur_page_item_num = app_system_set_get_cur_page_show_item_num();
                        app_system_set_unfocus_item(s_show_item_index_list[s_sel+s_cur_page_first_item_index]);
                        for(i = 0; i < cur_page_item_num; i++)
                        {
                    app_system_set_item_data_update(s_show_item_index_list[s_cur_page_first_item_index+i]);
                        }
                        s_sel = app_system_set_last_page_sel(s_sel);
                        app_system_set_focus_item(s_show_item_index_list[s_sel+s_cur_page_first_item_index]);
                        app_system_set_page_num_arrow();
                    }
                }
                break;
            case STBK_PAGE_DOWN:
                {
                    if(app_system_set_get_show_item_total() > SETTING_ITEM_MAX)
                    {
                        int cur_page_item_num = 0;

                        cur_page_item_num = app_system_set_get_cur_page_show_item_num();
                        app_system_set_unfocus_item(s_show_item_index_list[s_sel+s_cur_page_first_item_index]);
                        for(i = 0; i < cur_page_item_num; i++)
                        {
                    app_system_set_item_data_update(s_show_item_index_list[s_cur_page_first_item_index+i]);
                        }
                        s_sel = app_system_set_next_page_sel(s_sel);
                        app_system_set_focus_item(s_show_item_index_list[s_sel+s_cur_page_first_item_index]);
                        app_system_set_page_num_arrow();
                    }
                }
                break;
            case VK_BOOK_TRIGGER:
                {
                    app_system_trriger_func();
                }
                break;
			case STBK_EXIT:
			case STBK_MENU:
				if (s_setting_opt->forbidExitKey)
					break;
				app_system_set_destroy(EXIT_NORMAL);
				break;

			case STBK_RED:
				if(s_setting_opt->menuFuncKey.redKey.keyPressFun != NULL)
				{
					s_setting_opt->menuFuncKey.redKey.keyPressFun();
				}
				break;

			case STBK_GREEN:
				if(s_setting_opt->menuFuncKey.greenKey.keyPressFun != NULL)
				{
					s_setting_opt->menuFuncKey.greenKey.keyPressFun();
				}
				break;

			case STBK_BLUE:
				if(s_setting_opt->menuFuncKey.blueKey.keyPressFun != NULL)
				{
					s_setting_opt->menuFuncKey.blueKey.keyPressFun();
				}
				break;

			case STBK_YELLOW:
				if(s_setting_opt->menuFuncKey.yellowKey.keyPressFun != NULL)
				{
					s_setting_opt->menuFuncKey.yellowKey.keyPressFun();
				}
				break;

			default:
				break;
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_got_focus(GuiWidget *widget, void *usrdata)
{
	app_system_reset_unfocus_image();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_system_set_lost_focus(GuiWidget *widget, void *usrdata)
{
	app_system_reset_focus_image();
	return EVENT_TRANSFER_STOP;
}

static int _system_set_btn_keypress(SettingOptSel optsel, GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
    int cur_page_item_num = 0;

    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((cur_page_item_num > optsel) && (s_setting_opt->item[s_show_item_index_list[optsel+s_cur_page_first_item_index]].itemCallback.btnCallback.BtnPress != NULL))
		{
			ret = s_setting_opt->item[s_show_item_index_list[optsel+s_cur_page_first_item_index]].itemCallback.btnCallback.BtnPress(event->key.sym);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_btn1_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_btn_keypress(SETTING_ITEM_1, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_btn2_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_btn_keypress(SETTING_ITEM_2, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_btn3_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_btn_keypress(SETTING_ITEM_3, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_btn4_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_btn_keypress(SETTING_ITEM_4, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_btn5_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_btn_keypress(SETTING_ITEM_5, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_btn6_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_btn_keypress(SETTING_ITEM_6, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_btn7_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_btn_keypress(SETTING_ITEM_7, widget, usrdata);
}

static int _system_set_edit_keypress(SettingOptSel optsel, GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
    int cur_page_item_num = 0;

    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((cur_page_item_num > optsel) && (s_setting_opt->item[s_show_item_index_list[optsel+s_cur_page_first_item_index]].itemCallback.editCallback.EditPress!= NULL))
		{
			ret = s_setting_opt->item[s_show_item_index_list[optsel+s_cur_page_first_item_index]].itemCallback.editCallback.EditPress(find_virtualkey_ex(event->key.scancode,event->key.sym));
		}
	}

	return ret;
}

SIGNAL_HANDLER int app_system_set_edit1_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_edit_keypress(SETTING_ITEM_1, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_edit2_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_edit_keypress(SETTING_ITEM_2, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_edit3_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_edit_keypress(SETTING_ITEM_3, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_edit4_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_edit_keypress(SETTING_ITEM_4, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_edit5_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_edit_keypress(SETTING_ITEM_5, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_edit6_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_edit_keypress(SETTING_ITEM_6, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_edit7_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_edit_keypress(SETTING_ITEM_7, widget, usrdata);
}

static int _system_set_edit_reach_end(SettingOptSel optsel, GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    int cur_page_item_num = 0;

    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
	if((cur_page_item_num > optsel) && (s_setting_opt->item[s_show_item_index_list[optsel+s_cur_page_first_item_index]].itemCallback.editCallback.EditReachEnd!= NULL))
	{
		char *s = NULL;
		GUI_GetProperty(app_system_get_edit(s_show_item_index_list[optsel+s_cur_page_first_item_index]), "string", &s);
		ret = s_setting_opt->item[s_show_item_index_list[optsel+s_cur_page_first_item_index]].itemCallback.editCallback.EditReachEnd(s);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_edit1_reach_end(GuiWidget *widget, void *usrdata)
{
	return _system_set_edit_reach_end(SETTING_ITEM_1, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_edit2_reach_end(GuiWidget *widget, void *usrdata)
{
	return _system_set_edit_reach_end(SETTING_ITEM_2, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_edit3_reach_end(GuiWidget *widget, void *usrdata)
{
	return _system_set_edit_reach_end(SETTING_ITEM_3, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_edit4_reach_end(GuiWidget *widget, void *usrdata)
{
	return _system_set_edit_reach_end(SETTING_ITEM_4, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_edit5_reach_end(GuiWidget *widget, void *usrdata)
{
	return _system_set_edit_reach_end(SETTING_ITEM_5, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_edit6_reach_end(GuiWidget *widget, void *usrdata)
{
	return _system_set_edit_reach_end(SETTING_ITEM_6, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_edit7_reach_end(GuiWidget *widget, void *usrdata)
{
	return _system_set_edit_reach_end(SETTING_ITEM_7, widget, usrdata);
}

static  int _system_set_cmb_change(SettingOptSel optsel, GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
    int cur_page_item_num = 0;

    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
	if((cur_page_item_num > optsel) && (s_setting_opt->item[s_show_item_index_list[optsel+s_cur_page_first_item_index]].itemCallback.cmbCallback.CmbChange!= NULL))
	{
		int sel = 0;
		GUI_GetProperty(app_system_get_combobox(s_show_item_index_list[optsel+s_cur_page_first_item_index]), "select", &sel);
		ret = s_setting_opt->item[s_show_item_index_list[optsel+s_cur_page_first_item_index]].itemCallback.cmbCallback.CmbChange(sel);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb1_change(GuiWidget *widget, void *usrdata)
{
	return _system_set_cmb_change(SETTING_ITEM_1, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_cmb2_change(GuiWidget *widget, void *usrdata)
{
	return _system_set_cmb_change(SETTING_ITEM_2, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_cmb3_change(GuiWidget *widget, void *usrdata)
{
	return _system_set_cmb_change(SETTING_ITEM_3, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_cmb4_change(GuiWidget *widget, void *usrdata)
{
	return _system_set_cmb_change(SETTING_ITEM_4, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_cmb5_change(GuiWidget *widget, void *usrdata)
{
	return _system_set_cmb_change(SETTING_ITEM_5, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_cmb6_change(GuiWidget *widget, void *usrdata)
{
	return _system_set_cmb_change(SETTING_ITEM_6, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_cmb7_change(GuiWidget *widget, void *usrdata)
{
	return _system_set_cmb_change(SETTING_ITEM_7, widget, usrdata);
}
static int _system_set_cmb_keypress(SettingOptSel optsel, GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
    int cur_page_item_num = 0;

    cur_page_item_num = app_system_set_get_cur_page_show_item_num();
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((cur_page_item_num > optsel) && (s_setting_opt->item[s_show_item_index_list[optsel+s_cur_page_first_item_index]].itemCallback.cmbCallback.CmbPress != NULL))
		{
			ret = s_setting_opt->item[s_show_item_index_list[optsel+s_cur_page_first_item_index]].itemCallback.cmbCallback.CmbPress(event->key.sym);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb1_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_cmb_keypress(SETTING_ITEM_1, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_cmb2_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_cmb_keypress(SETTING_ITEM_2, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_cmb3_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_cmb_keypress(SETTING_ITEM_3, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_cmb4_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_cmb_keypress(SETTING_ITEM_4, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_cmb5_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_cmb_keypress(SETTING_ITEM_5, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_cmb6_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_cmb_keypress(SETTING_ITEM_6, widget, usrdata);
}
SIGNAL_HANDLER int app_system_set_cmb7_keypress(GuiWidget *widget, void *usrdata)
{
	return _system_set_cmb_keypress(SETTING_ITEM_7, widget, usrdata);
}
