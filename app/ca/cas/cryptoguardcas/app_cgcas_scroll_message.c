/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_cgcas_scroll_message.c
* Author    :	huangwf
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2021.03.30		  huangwf		  Creation
*****************************************************************************/
#include "app_module.h"
#include "app.h"
#ifdef CASCAM_CGCAS_SUPPORT
#include "cascam_cryptoguardcas_data.h"
#include "cascam.h"
#include "cascam_ui_porting.h"
#include "math.h"
#include "cascam_ui_common.h"
#include "app_ca_osd_layer.h"

#define ROLLING_COMPLETE_EXIT						1
#define WND_ROLLING_R2L_TOP                         "ca_rolling_text_r2l_prompt" // in style.xml
#define PROMPT_ROLLING_R2L_TOP_NAME                 "prompt_ca_rolling_r2l"
#define TXT_ROLLING_R2L_TOP                         "txt_ca_rolling_r2l"

#define WND_ROLLING_R2L_BOTTOM                      "ca_rolling_text_r2l_prompt_1" // in style.xml
#define PROMPT_ROLLING_R2L_BOTTOM_NAME              "prompt_ca_rolling_r2l_1"
#define TXT_ROLLING_R2L_BOTTOM                      "txt_ca_rolling_r2l_1"

#define WND_SUPER_OSD_FULLSCREEN                     "ca_force_osd_prompt" // in style.xml
#define PROMPT_SUPER_OSD_FULLSCREEN_NAME             "prompt_ca_force_osd"
#define TXT_SUPER_OSD_FULLSCREEN					 "txt_ca_force_osd"
#define TXT_SUPER_OSD_CARD							 "txt_ca_force_osd_card"

#define X_BOTTOM_POS    25
#define Y_BOTTOM_POS    530
#define ROLLING_WIDTH	970
#define ROLLING_HEIGHT	30

typedef enum _CGCASOSDStyleEnum{
    ROLLING_TOP,
    ROLLING_BOTTOM,
    CGCAS_OSD_FULLSCREEN,
    CGCAS_OSD_HALFSCREEN,
    CGCAS_OSD_STYLE_TOTAL,
}CGCASOSDStyleEnum;

typedef struct _CGCASOSDMenuClass{
    char *prompt_name;
    char *text_name;
    unsigned int repeat_times;
    unsigned int duration;
    CGCASOSDStyleEnum style;
}CGCASOSDMenuClass;

//extern void app_cgcas_set_force_lock_status(int type, char force_flag);
extern int32_t cascam_cgcas_queue_put(uint32_t type, void *buf, uint32_t size, int32_t timeout);

static event_list  *thiz_query_repeat_timer[CGCAS_OSD_STYLE_TOTAL] = {NULL, NULL};
static event_list  *thiz_query_duration_timer[CGCAS_OSD_STYLE_TOTAL] = {NULL, NULL};
static unsigned int thiz_rolling_times[CGCAS_OSD_STYLE_TOTAL] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
static CGCASOSDMenuClass thiz_menu_info[CGCAS_OSD_STYLE_TOTAL];
static Cascam_CGScrollMessage thiz_rolling_control[CGCAS_OSD_STYLE_TOTAL];
static uint8_t thiz_rolling_status[CGCAS_OSD_STYLE_TOTAL];
static CascamOsdLayerBlock osd_layer[CGCAS_OSD_STYLE_TOTAL];
static uint16_t osd_show_time_sec = 0;

static int _cgcas_create_exit_timer(Cascam_CGScrollMessage *control_info);
static void _rolling_osd_exit(int style);

static void _record_rolling_times(unsigned int times, CGCASOSDStyleEnum style)
{
    thiz_rolling_times[style] = times;
}

static void cgcas_cas_end_super_osd(int style)
{
#if 1//CASCAM_UI_MULTI_LAYER
	cascam_osd_layer_stop_rolling_string(&osd_layer[style]);
	cascam_osd_layer_quit(&osd_layer[style]);
#endif
}
#if ROLLING_COMPLETE_EXIT
static unsigned int thiz_rolling_complete_exit[CGCAS_OSD_STYLE_TOTAL] = {0, 0, 0, 0};
static unsigned int thiz_rolling_times_exit[CGCAS_OSD_STYLE_TOTAL] = {0, 0, 0, 0};
static int _exit_rolling_osd(int style)
{
    int ret = 0;

/*
	time_t osd_end_time_sec = 0;
	uint32_t sub_id = 0;

	osd_end_time_sec = cascam_ui_get_tick_time() ;
	osd_show_time_sec = osd_end_time_sec - osd_show_time_sec;
	sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_OSD_OVER;
    if(0 != cascam_set(sub_id, (void*)&osd_show_time_sec))
    {
    	printf("CASCAM_CGCAS_OSD_OVER error\n");
    }
*/

	cgcas_cas_end_super_osd(style);

    if(thiz_query_repeat_timer[style])
    {
        cascam_timer_stop(thiz_query_repeat_timer[style]);
        //cascam_remove_timer(thiz_query_repeat_timer[style]);
        //thiz_query_repeat_timer[style] = NULL;
    }

    if(thiz_query_duration_timer[style])
    {
        cascam_timer_stop(thiz_query_duration_timer[style]);
        //cascam_remove_timer(thiz_query_duration_timer[style]);
        //thiz_query_duration_timer[style] = NULL;
    }

    _record_rolling_times(0x01, style);
    memset(&thiz_menu_info[style], 0, sizeof(CGCASOSDMenuClass));

    thiz_rolling_complete_exit[style] = 0;
    thiz_rolling_times_exit[style] = 0;
	thiz_rolling_status[style] = 0;

    return ret;
}

static int _complete_exit_control(int style, unsigned int rolling_times)
{
    int ret = 0;
    if(0 == thiz_rolling_complete_exit[style])
    {
        return 0;
    }
    if(thiz_rolling_times_exit[style] == rolling_times)
    {
        _exit_rolling_osd(style);
    }
    return ret;
}
#endif


static int _query_repeat_timer(void *usrdata)
{
    unsigned int rolling_times = 0;
    CGCASOSDMenuClass *data = (CGCASOSDMenuClass*)usrdata;

    rolling_times = cascam_osd_layer_get_roll_times(osd_layer[data->style].roll_handle);
    _record_rolling_times(rolling_times, data->style);
    if(rolling_times >= data->repeat_times)
    {
        //cmm_cascam_exit_dialog3(data->style);
        //_cgcas_create_exit_timer(&thiz_rolling_control[data->style]);
        _exit_rolling_osd(data->style);
        return 0;
    }
#if ROLLING_COMPLETE_EXIT
    _complete_exit_control(data->style, rolling_times);
#endif
    return 0;
}

static int _create_repeat_timer(CGCASOSDMenuClass *menu_info)
{
    if (cascam_reset_timer(thiz_query_repeat_timer[menu_info->style], 100) != 0)
    {
        thiz_query_repeat_timer[menu_info->style] = cascam_create_timer(_query_repeat_timer, 100,  (void*)menu_info, TIMER_REPEAT);
    }
    return 0;
}

static int _query_duration_timer(void *usrdata)
{
    CGCASOSDMenuClass *data = (CGCASOSDMenuClass*)usrdata;

    _exit_rolling_osd(data->style);
    return 0;
}

static int _create_duration_timer(CGCASOSDMenuClass *menu_info)
{
    if (cascam_reset_timer(thiz_query_duration_timer[menu_info->style], (menu_info->duration * 1000)) != 0)
    {
        thiz_query_duration_timer[menu_info->style] = cascam_create_timer(_query_duration_timer, (menu_info->duration * 1000), (void*)menu_info, TIMER_REPEAT);
    }
    return 0;
}

// 1: need create; 0: need not create
static int _create_dialog_check(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    if((1 == cascam_ui_check_window_exist("wnd_epg", CASCAM_MSG_CHECK_WIN))
            || (1 == cascam_ui_check_window_exist("wnd_main_menu", CASCAM_MSG_CHECK_WIN))
            /*|| (1 == cascam_ui_check_window_exist("wnd_channel_info", CASCAM_MSG_CHECK_WIN))*/)
    {
        return 0;
    }
	return 1;
}

void app_cgcas_osd_get_rolling_times(unsigned int *times, int style)
{
    if( (NULL == times) || (style > CGCAS_OSD_STYLE_TOTAL))
    {
        printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }
    *times = thiz_rolling_times[style];
}

static int _get_rolling_osd_rect(unsigned int *x, unsigned int *y, unsigned int *w, unsigned int *h, char *name)
{
    GuiWidget *widget = NULL;

    if((NULL == x) || (NULL == y) || (NULL == w) || (NULL == h) || (NULL == name))
    {
        return -1;
    }
    widget = gui_get_window(name);
    if(NULL == widget)
    {
        return -1;
    }
    *x = widget->rect.x;
    *y = widget->rect.y;
    *w = widget->rect.w;
    *h = widget->rect.h;
    return 0;
}

static int _check_rect_overlap(unsigned int x, unsigned int y, unsigned int w, unsigned int h,
                                unsigned int x1, unsigned int y1, unsigned int w1, unsigned int h1)
{
    if(((x + w) < x1)
            || ((y + h) < y1)
            || (y > (y1 + h1))
            || (x > (x1 + w1)))
    {
        return 0;// no overlap
    }
    else
    {
        return 1;// operlap
    }
}

void app_cgcas_osd_check_rolling_osd(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    unsigned int x_pos = 0, y_pos = 0, width = 0, height = 0;
    if(cascam_ui_check_gui_prompt(PROMPT_ROLLING_R2L_TOP_NAME) == GXCORE_SUCCESS)
    {
        if(_get_rolling_osd_rect(&x_pos, &y_pos, &width, &height, PROMPT_ROLLING_R2L_TOP_NAME) == 0)
        {
            if(_check_rect_overlap(x, y, w, h, x_pos, y_pos, width, height))
            {
                //cmm_cascam_exit_dialog3(ROLLING_TOP);
            }
        }
    }

    if(cascam_ui_check_gui_prompt(PROMPT_ROLLING_R2L_BOTTOM_NAME) == GXCORE_SUCCESS)
    {
        if(_get_rolling_osd_rect(&x_pos, &y_pos, &width, &height, PROMPT_ROLLING_R2L_BOTTOM_NAME) == 0)
        {
            if(_check_rect_overlap(x, y, w, h, x_pos, y_pos, width, height))
            {
                //cmm_cascam_exit_dialog3(ROLLING_BOTTOM);
            }
        }
    }
}
static int cascam_osd_layer_osd_config(CascamOsdLayerBlock* layer, CGCASOSDStyleEnum byStyle)
{
	uint32_t  len = 0;
	layer->x = X_BOTTOM_POS;
	layer->y = Y_BOTTOM_POS;
	layer->w = ROLLING_WIDTH;
	layer->h = ROLLING_HEIGHT;

	layer->fore_color = FORE_COLOR;
	layer->back_color = BACK_COLOR;

	if(layer->font_name !=NULL){
		GxCore_Free(layer->font_name);
		layer->font_name = NULL;
	}
	len = strlen(DEFAULT_FONT);
	layer->font_name = (uint8_t*)GxCore_Malloc(len+1);
	memset(layer->font_name, 0, len+1);
	memcpy(layer->font_name, DEFAULT_FONT, len);
	layer->layer_type = CASCAM_UI_OSD;

	return 0;
}

static event_list  *thiz_cgcas_create_exit_timer[CGCAS_OSD_STYLE_TOTAL] = {NULL, NULL, NULL, NULL};

static void _cgcas_scroll_msg_create(int pos, Cascam_CGScrollMessage *data)
{
	CascamRect rect = {0,};
    char *prompt_name = PROMPT_ROLLING_R2L_TOP_NAME;
   // char *window_name = WND_ROLLING_R2L_TOP;
    char *text_name = TXT_ROLLING_R2L_TOP;
    unsigned int x_default_pos = X_BOTTOM_POS;
    unsigned int y_default_pos = Y_BOTTOM_POS;

    if(NULL == data->msgText)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        return ;
    }
#if 0
	if(1 == cascam_ui_check_notice_hide())
	{
        printf("\n not in fullscreen, %s, %d\n", __FUNCTION__, __LINE__);
		return;
	}
#endif
	cgcas_cas_end_super_osd(pos);

    x_default_pos = X_BOTTOM_POS;
    y_default_pos = Y_BOTTOM_POS;
    prompt_name = PROMPT_ROLLING_R2L_BOTTOM_NAME;
    //window_name = WND_ROLLING_R2L_BOTTOM;
    text_name = TXT_ROLLING_R2L_BOTTOM;
	_create_dialog_check(x_default_pos, y_default_pos, 1024, 30);
    if(1)//(_create_dialog_check(x_default_pos, y_default_pos, 1024, 30))
    {
		cascam_osd_layer_osd_config(&osd_layer[pos], pos);
        osd_layer[pos].fore_color = data->msgFontColor;
        osd_layer[pos].back_color = data->msgBackgroundColor;
		cascam_osd_layer_init(&osd_layer[pos]);
		rect.x = 0;
		rect.y = 0;
		rect.w = osd_layer[pos].w;
		rect.h = osd_layer[pos].h;
        cascam_osd_layer_set_rolling_delay(data->msgScrollSpeed / 4);
		cascam_osd_layer_start_rolling_string(&osd_layer[pos], data->msgText, rect, CASCAM_ROLL_RIGHT_LEFT);
    }
    else
    {
        return ;
    }

	osd_show_time_sec = cascam_ui_get_tick_time();
    _record_rolling_times(0, pos);
#if ROLLING_COMPLETE_EXIT
    thiz_rolling_times_exit[pos] = 0;
    thiz_rolling_complete_exit[pos] = 0;
#endif
    thiz_menu_info[pos].prompt_name = prompt_name;
    thiz_menu_info[pos].text_name = text_name;
    // here, need create the repeat timer, for complete exit or create..., so
    // use the max value, 0xffffffff;
    thiz_menu_info[pos].repeat_times = 0;
    thiz_menu_info[pos].duration = data->msgDuration;
    thiz_menu_info[pos].style = pos;
    if(thiz_menu_info[pos].repeat_times > 0)
    {
        _create_repeat_timer(&thiz_menu_info[pos]);
    }

    if(thiz_menu_info[pos].duration > 0)
    {
        _create_duration_timer(&thiz_menu_info[pos]);
    }
}

static void _rolling_osd_exit(int style)
{
#if ROLLING_COMPLETE_EXIT
    if(style >= 0)
    {
        thiz_rolling_complete_exit[style] = 1;
        if(thiz_rolling_times[style] != 0xffffffff)
        {
            thiz_rolling_times_exit[style] = thiz_rolling_times[style] + 1;
        }
        else
        {
            thiz_rolling_times_exit[style] = 1;
        }
    }
    else
    {
        int i = 0;
        for(i = 0; i < CGCAS_OSD_STYLE_TOTAL; i++)
        {
            thiz_rolling_complete_exit[i] = 1;
            if(thiz_rolling_times[i] != 0xffffffff)
            {
                thiz_rolling_times_exit[i] = thiz_rolling_times[i] + 1;
            }
            else
            {
                thiz_rolling_times_exit[i] = 1;
            }
        }
    }
#else
    if(style >= 0)
    {
		cgcas_cas_end_super_osd(style);
        // timer
        if(thiz_query_repeat_timer[style])
        {
            cascam_timer_stop(thiz_query_repeat_timer[style]);
            //cascam_remove_timer(thiz_query_repeat_timer[style]);
            //thiz_query_repeat_timer[style] = NULL;
        }
        if(thiz_query_duration_timer[style])
        {
            cascam_timer_stop(thiz_query_duration_timer[style]);
            //cascam_remove_timer(thiz_query_duration_timer[style]);
            //thiz_query_duration_timer[style] = NULL;
        }
        _record_rolling_times(0xffffffff, style);
        memset(&thiz_menu_info[style], 0, sizeof(CGCASOSDMenuClass));
    }
    else
    {
        int i = 0;
        for(i = 0; i < CGCAS_OSD_STYLE_TOTAL; i++)
        {
			cgcas_cas_end_super_osd(style);
            // timer
            if(thiz_query_repeat_timer[i])
            {
                cascam_timer_stop(thiz_query_repeat_timer[i]);
                //cascam_remove_timer(thiz_query_repeat_timer[i]);
                //thiz_query_repeat_timer[i] = NULL;
            }
            if(thiz_query_duration_timer[i])
            {
                cascam_timer_stop(thiz_query_duration_timer[i]);
                //cascam_remove_timer(thiz_query_duration_timer[i]);
                //thiz_query_duration_timer[i] = NULL;
            }
            _record_rolling_times(0xffffffff, i);
            memset(&thiz_menu_info[i], 0, sizeof(CGCASOSDMenuClass));
        }
    }
#endif
}

static int _cgcas_create_exit_control(void *usrdata)
{
	CGCASOSDStyleEnum type = 0;

    if (NULL == usrdata)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    Cascam_CGScrollMessage *data = (Cascam_CGScrollMessage*)usrdata;
	type = ROLLING_BOTTOM;

	if(thiz_cgcas_create_exit_timer[type])
    {
        cascam_timer_stop(thiz_cgcas_create_exit_timer[type]);
        //cascam_remove_timer(thiz_cgcas_create_exit_timer[type]);
        //thiz_cgcas_create_exit_timer[type] = NULL;
    }

    if (0)//(CASCAM_CGCAS_HIDE  == data->display_type)
    {
    	_rolling_osd_exit(type);
    }
    else
    {
       _cgcas_scroll_msg_create(type, data);
    }

    return 0;
}

static int _cgcas_create_exit_timer(Cascam_CGScrollMessage *control_info)
{
	CGCASOSDStyleEnum type = 0;
	static Cascam_CGScrollMessage control;

	if (control_info == NULL)
	{
		printf("\nCA, error, %s, %d\n", __FUNCTION__,__LINE__);
   	 	return -1;
	}

	memcpy(&control, control_info, sizeof(Cascam_CGScrollMessage));
	type = ROLLING_BOTTOM;

    if(thiz_cgcas_create_exit_timer[type])
    {
        cascam_timer_stop(thiz_cgcas_create_exit_timer[type]);
        //cascam_remove_timer(thiz_cgcas_create_exit_timer[type]);
        //thiz_cgcas_create_exit_timer[type] = NULL;
    }

    if (cascam_reset_timer(thiz_cgcas_create_exit_timer[type], 1000) != 0)
    {
        thiz_cgcas_create_exit_timer[type] = cascam_create_timer(_cgcas_create_exit_control, 1000, (void*)&control, TIMER_REPEAT_NOW);
    }

    return 0;
}

void app_cgcas_scroll_msg_notify(void *param)
{
	CGCASOSDStyleEnum type = 0;
	Cascam_CGScrollMessage *p = NULL;

    if (NULL == param)
    {
        printf("\nCAS, error, %s, %d\n", __FUNCTION__,__LINE__);
        return;
    }

	p = (Cascam_CGScrollMessage*)param;
	type = ROLLING_BOTTOM;
	if (1)//(p->display_type == CASCAM_CGCAS_SHOW)
	{
		if (thiz_query_repeat_timer[type])
		{
			cascam_timer_stop(thiz_query_repeat_timer[type]);
			//cascam_remove_timer(thiz_query_repeat_timer[type]);
			//thiz_query_repeat_timer[type] = NULL;
		}

		if (thiz_query_duration_timer[type])
		{
			cascam_timer_stop(thiz_query_duration_timer[type]);
			//cascam_remove_timer(thiz_query_duration_timer[type]);
			//thiz_query_duration_timer[type] = NULL;
		}
	}

	memcpy(&thiz_rolling_control[type], p, sizeof(Cascam_CGScrollMessage));
	thiz_rolling_status[type] = 1;
    _cgcas_create_exit_timer(&thiz_rolling_control[type]);
}

void app_cgcas_scroll_msg_destroy_for_app(void)
{
	uint8_t i = 0;

	for(i = 0; i < CGCAS_OSD_STYLE_TOTAL; i++)
	{
		if (1)//(thiz_rolling_control[i].display_type == CASCAM_CGCAS_SHOW)
		{
			_rolling_osd_exit(ROLLING_BOTTOM);
		}
	}
}

void app_cgcas_scroll_msg_show(int style)
{
    if(style >= CGCAS_OSD_STYLE_TOTAL)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        return ;
    }
    if(style >= 0)
    {
        if((thiz_menu_info[style].prompt_name)
                && cascam_ui_check_gui_prompt(thiz_menu_info[style].prompt_name) == GXCORE_SUCCESS)
        {
            GUI_SetProperty(thiz_menu_info[style].text_name, "continue_rolling", NULL);
            GUI_SetProperty(thiz_menu_info[style].prompt_name, "state", "show");
        }
        // timer
        if(thiz_query_repeat_timer[style])
        {
            cascam_reset_timer(thiz_query_repeat_timer[style], 0);
        }
        if(thiz_query_duration_timer[style])
        {
            cascam_reset_timer(thiz_query_duration_timer[style], 0);
        }
    }
    else
    {
        int i = 0;
        for(i = 0; i < CGCAS_OSD_STYLE_TOTAL; i++)
        {
            if((thiz_menu_info[i].prompt_name)
                    && cascam_ui_check_gui_prompt(thiz_menu_info[i].prompt_name) == GXCORE_SUCCESS)
            {
                GUI_SetProperty(thiz_menu_info[i].text_name, "continue_rolling", NULL);
                GUI_SetProperty(thiz_menu_info[i].prompt_name, "state", "show");
            }
            // timer
            if(thiz_query_repeat_timer[i])
            {
                cascam_reset_timer(thiz_query_repeat_timer[i], 0);
            }
            if(thiz_query_duration_timer[i])
            {
                cascam_reset_timer(thiz_query_duration_timer[i], 0);
            }
        }
    }
}

void app_cgcas_scroll_msg_hide(int style)
{
    if(style >= CGCAS_OSD_STYLE_TOTAL)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        return ;
    }
    if(style >= 0)
    {
        if((thiz_menu_info[style].prompt_name)
                && cascam_ui_check_gui_prompt(thiz_menu_info[style].prompt_name) == GXCORE_SUCCESS)
        {
            GUI_SetProperty(thiz_menu_info[style].text_name, "rolling_stop_soon", NULL);
            GUI_SetProperty(thiz_menu_info[style].prompt_name, "state", "hide");
        }
        // timer
        if(thiz_query_repeat_timer[style])
        {
            cascam_timer_stop(thiz_query_repeat_timer[style]);
        }
        if(thiz_query_duration_timer[style])
        {
            cascam_timer_stop(thiz_query_duration_timer[style]);
        }
    }
    else
    {

		int i = 0;
        for(i = 0; i < CGCAS_OSD_STYLE_TOTAL; i++)
        {
            if((thiz_menu_info[i].prompt_name)
                    && cascam_ui_check_gui_prompt(thiz_menu_info[i].prompt_name) == GXCORE_SUCCESS)
            {
                GUI_SetProperty(thiz_menu_info[i].text_name, "rolling_stop_soon", NULL);
                GUI_SetProperty(thiz_menu_info[i].prompt_name, "state", "hide");
            }
            // timer
            if(thiz_query_repeat_timer[i])
            {
                cascam_timer_stop(thiz_query_repeat_timer[i]);
            }
            if(thiz_query_duration_timer[i])
            {
                cascam_timer_stop(thiz_query_duration_timer[i]);
            }
        }
    }
}


int check_and_exit_non_force_scroll_msg(void)
{
	int i = 0;
	int ret = 0;
	const char *wnd = NULL;
	wnd = GUI_GetFocusWindow();

	if((wnd == NULL) || (0 != strcasecmp("wnd_full_screen", wnd)))
	{
		return 0;
	}

	for(i=0;i<CGCAS_OSD_STYLE_TOTAL;i++)
	{
		if(thiz_rolling_status[i] == 1)
		{
			if(thiz_rolling_control[i].msgForced == 0)
			{
				_exit_rolling_osd(i);
				ret = 1;
			}
		}
	}
	return ret;
}

void test_scroll_message(void)
{
    Cascam_CGScrollMessage scroll_msg_info;

    unsigned char scroll_msg[] =
    {
        0x41, 0x20, 0x53, 0x63, 0x72, 0x6f, 0x6c, 0x6c, 0x69, 0x6e, 0x67, 0x20, 0x6d, 0x65, 0x73, 0x73,
        0x61, 0x67, 0x65, 0x2c, 0x20, 0x72, 0x65, 0x64, 0x20, 0x74, 0x65, 0x78, 0x74, 0x20, 0x6f, 0x6e,
        0x20, 0x77, 0x68, 0x69, 0x74, 0x65, 0x20, 0x62, 0x61, 0x63, 0x6b, 0x67, 0x72, 0x6f, 0x75, 0x6e,
        0x64, 0x2e, 0x20, 0x74, 0x6f, 0x20, 0x62, 0x65, 0x20, 0x73, 0x68, 0x6f, 0x77, 0x6e, 0x20, 0x74,
        0x77, 0x69, 0x63, 0x65, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20, 0x36, 0x30, 0x20, 0x73, 0x65, 0x63,
        0x20, 0x64, 0x75, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x31, 0x20,
        0x6d, 0x69, 0x6e, 0x20, 0x70, 0x61, 0x75, 0x73, 0x65, 0x2e, 0x20, 0x39, 0x30, 0x6d, 0x53, 0x20,
        0x73, 0x63, 0x72, 0x6f, 0x6c, 0x6c, 0x20, 0x73, 0x70, 0x65, 0x65, 0x64
    };

    memset(&scroll_msg_info, 0, sizeof(Cascam_CGScrollMessage));

    scroll_msg_info.encoding           = 0;
    scroll_msg_info.msgDuration        = 40;
    scroll_msg_info.msgY               = 14;
    scroll_msg_info.msgFontColor       = 0xFFFF0000;
    scroll_msg_info.msgBackgroundColor = 0xFFFFFFFF;
    scroll_msg_info.msgScrollSpeed     = 90; /* 90ms 250ms 500ms */
    scroll_msg_info.msgForced          = 0;
    scroll_msg_info.msgFontSize        = 0;
    memset(scroll_msg_info.msgText, '\0', 256);
    memcpy(scroll_msg_info.msgText, scroll_msg, sizeof(scroll_msg));

    if (cascam_cgcas_queue_put(CASCAM_CGCAS_SCROLL_MESSAGE, (void *)&scroll_msg_info, sizeof(Cascam_CGScrollMessage), 0) != 0)
        printf("CASCAM_MSG CASCAM_CGCAS_SCROLL_MESSAGE Error\n");

    return;
}

#endif
