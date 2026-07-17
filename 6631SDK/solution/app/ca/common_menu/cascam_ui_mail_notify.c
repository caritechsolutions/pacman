/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	cascam_ui_mail_notify.c
* Author    :	zhangyg
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  2.0	   2018.10.29		 zhangyg			 Creation
*****************************************************************************/
#include "app.h"
#include "app_module.h"
#if CASCAM_SUPPORT
//#include "app_config.h"
#include "cascam_ui_common.h"
#include "cascam_ui_porting.h"
#if CASCAM_UI_MULTI_LAYER
#include "app_ca_osd_layer.h"
#endif

#if 1
// for notice
#define PROMPT_EMAIL_NOTICE_NEW_NAME    "prompt_ca_email_new"
#define WND_EMAIL_NOTICE_NEW            "ca_email_new_prompt"
#define PROMPT_EMAIL_NOTICE_FULL_NAME   "prompt_ca_email_full"
#define WND_EMAIL_NOTICE_FULL           "ca_email_full_prompt"

#define MAX_MAIL_STATE		2
static event_list  *thiz_query_flicker_timer = NULL;

typedef struct{
    char index;// new or full
    char flicker;// 0: static; 1:fliker
    char *prompt_name;
    char *wnd_name;
    int x_pos;
    int y_pos;
    char show_state;
}CascamUiMailPara;
static CascamUiMailPara thiz_mail_param[MAX_MAIL_STATE];

#if CASCAM_UI_MULTI_LAYER
#define NEW_MAIL_IMAGE_FILE	"/dvb/theme/image/ca/icon_mail.bmp"
#define FULL_MAIL_IMAGE_FILE	"/dvb/theme/image/ca/icon_mailfull.bmp"
static uint8_t s_icon_hide_flag = 0;
static CascamOsdLayerBlock mail_layer[MAX_MAIL_STATE];
static char* sMailStatusFile[MAX_MAIL_STATE] = {NEW_MAIL_IMAGE_FILE, FULL_MAIL_IMAGE_FILE};

static int cascam_osd_layer_config(CascamOsdLayerBlock* layer, uint8_t index)
{
	layer->x = thiz_mail_param[index].x_pos;
	layer->y = thiz_mail_param[index].y_pos;
	layer->w = cascam_osd_layer_get_image_width(sMailStatusFile[index]);
	layer->h = cascam_osd_layer_get_image_height(sMailStatusFile[index]);

	layer->layer_type = CASCAM_UI_OTHER;
	return 0;
}

static void _mail_notify_image_layer_show(void)
{
	if(cascam_ui_check_notice_hide())
	{
		if (thiz_mail_param[0].show_state == 1)
		{
			if(mail_layer[0].layer_name)
			{
				cascam_osd_layer_stop_draw_image(&mail_layer[0]);
				cascam_osd_layer_quit(&mail_layer[0]);
			}
		}
		if (thiz_mail_param[1].show_state == 1)
		{
			if(mail_layer[1].layer_name)
			{
				cascam_osd_layer_stop_draw_image(&mail_layer[1]);
				cascam_osd_layer_quit(&mail_layer[1]);
			}
		}
	}
	else
	{
		if (thiz_mail_param[0].show_state == 1)
		{
			if(thiz_mail_param[0].flicker)
			{
				if(mail_layer[0].layer_name)
				{
					cascam_osd_layer_stop_draw_image(&mail_layer[0]);
					cascam_osd_layer_quit(&mail_layer[0]);
				}
				else
				{
					cascam_osd_layer_config(&mail_layer[0], 0);
					cascam_osd_layer_init(&mail_layer[0]);
					cascam_osd_layer_start_draw_image(&mail_layer[0], 0, 0, sMailStatusFile[0]);
				}
			}
			else
			{
				if(mail_layer[0].layer_name)
				{
					;
				}
				else
				{
					cascam_osd_layer_config(&mail_layer[0], 0);
					cascam_osd_layer_init(&mail_layer[0]);
					cascam_osd_layer_start_draw_image(&mail_layer[0], 0, 0, sMailStatusFile[0]);
				}
			}
		}
		if (thiz_mail_param[1].show_state == 1)
		{
			if(thiz_mail_param[1].flicker)
			{
				if(mail_layer[1].layer_name)
				{
					cascam_osd_layer_stop_draw_image(&mail_layer[1]);
					cascam_osd_layer_quit(&mail_layer[1]);
				}
				else
				{
					cascam_osd_layer_config(&mail_layer[1], 1);
					cascam_osd_layer_init(&mail_layer[1]);
					cascam_osd_layer_start_draw_image(&mail_layer[1], 0, 0, sMailStatusFile[1]);
				}
			}
			else
			{
				if(mail_layer[1].layer_name)
				{
					;
				}
				else
				{
					cascam_osd_layer_config(&mail_layer[1], 1);
					cascam_osd_layer_init(&mail_layer[1]);
					cascam_osd_layer_start_draw_image(&mail_layer[1], 0, 0, sMailStatusFile[1]);
				}
			}
		}
	}
}
#else
static void _mail_notify_image_prompt_show(void)
{
	if(cascam_ui_check_notice_hide())
	{
		if (thiz_mail_param[0].show_state == 1)
		{
			if(GUI_CheckPrompt(PROMPT_EMAIL_NOTICE_NEW_NAME) == GXCORE_SUCCESS)
			{
				GUI_EndPrompt(PROMPT_EMAIL_NOTICE_NEW_NAME);
			}
		}
		if (thiz_mail_param[1].show_state == 1)
		{
			if(GUI_CheckPrompt(PROMPT_EMAIL_NOTICE_FULL_NAME) == GXCORE_SUCCESS)
			{
				GUI_EndPrompt(PROMPT_EMAIL_NOTICE_FULL_NAME);
			}
		}
	}
	else
	{
		if (thiz_mail_param[0].show_state == 1)
		{
			if(thiz_mail_param[0].flicker)
			{
				if(GUI_CheckPrompt(PROMPT_EMAIL_NOTICE_NEW_NAME) == GXCORE_SUCCESS)
				{
					GUI_EndPrompt(PROMPT_EMAIL_NOTICE_NEW_NAME);
				}
				else
				{
					GUI_CreatePrompt(thiz_mail_param[0].x_pos, thiz_mail_param[0].y_pos, thiz_mail_param[0].prompt_name, thiz_mail_param[0].wnd_name, NULL, "ontop");
				}
			}
			else
			{
				if(GUI_CheckPrompt(PROMPT_EMAIL_NOTICE_NEW_NAME) == GXCORE_SUCCESS)
				{
					;
				}
				else
				{
					GUI_CreatePrompt(thiz_mail_param[0].x_pos, thiz_mail_param[0].y_pos, thiz_mail_param[0].prompt_name, thiz_mail_param[0].wnd_name, NULL, "ontop");
				}
			}
		}
		if (thiz_mail_param[1].show_state == 1)
		{
			if(thiz_mail_param[1].flicker)
			{
				if(GUI_CheckPrompt(PROMPT_EMAIL_NOTICE_FULL_NAME) == GXCORE_SUCCESS)
				{
					GUI_EndPrompt(PROMPT_EMAIL_NOTICE_FULL_NAME);
				}
				else
				{
					GUI_CreatePrompt(thiz_mail_param[1].x_pos, thiz_mail_param[1].y_pos, thiz_mail_param[1].prompt_name, thiz_mail_param[1].wnd_name, NULL, "ontop");
				}
			}
			else
			{
				if(GUI_CheckPrompt(PROMPT_EMAIL_NOTICE_FULL_NAME) == GXCORE_SUCCESS)
				{
					;
				}
				else
				{
					GUI_CreatePrompt(thiz_mail_param[1].x_pos, thiz_mail_param[1].y_pos, thiz_mail_param[1].prompt_name, thiz_mail_param[1].wnd_name, NULL, "ontop");
				}
			}
		}
	}
}
#endif

static int _destroy_flicker_timer(void)
{
    int ret = 0;
    if(thiz_query_flicker_timer != NULL)
    {
        cascam_timer_stop(thiz_query_flicker_timer);
        //cascam_remove_timer(thiz_query_flicker_timer);
        //thiz_query_flicker_timer = NULL;
    }
#if CASCAM_UI_MULTI_LAYER
	if(mail_layer[0].layer_name)
	{
		cascam_osd_layer_stop_draw_image(&mail_layer[0]);
		cascam_osd_layer_quit(&mail_layer[0]);
	}
	if(mail_layer[1].layer_name)
	{
		cascam_osd_layer_stop_draw_image(&mail_layer[1]);
		cascam_osd_layer_quit(&mail_layer[1]);
	}
#else
	if(GUI_CheckPrompt(PROMPT_EMAIL_NOTICE_NEW_NAME) == GXCORE_SUCCESS)
	{
		GUI_EndPrompt(PROMPT_EMAIL_NOTICE_NEW_NAME);
	}
	if(GUI_CheckPrompt(PROMPT_EMAIL_NOTICE_FULL_NAME) == GXCORE_SUCCESS)
	{
		GUI_EndPrompt(PROMPT_EMAIL_NOTICE_FULL_NAME);
	}
#endif
	memset(&thiz_mail_param, 0, sizeof(CascamUiMailPara)*MAX_MAIL_STATE);

    return ret;
}

static int _query_flicker_timer(void *usrdata)
{
	if(s_icon_hide_flag == 1)
	{
		_destroy_flicker_timer();
		s_icon_hide_flag = 0;
	}
	else
	{
#if CASCAM_UI_MULTI_LAYER
		_mail_notify_image_layer_show();
#else
		_mail_notify_image_prompt_show();
#endif
	}
	return 0;
}

static int _create_flicker_timer(unsigned int duration_sec)
{
    if (cascam_reset_timer(thiz_query_flicker_timer, duration_sec*1000) != 0)
    {
        thiz_query_flicker_timer = cascam_create_timer(_query_flicker_timer, duration_sec*1000, NULL, TIMER_REPEAT);
    }
    return 0;
}

void cascam_ui_mail_notify(CascamUiMailNotify* p )
{
#define X1_DEFAUT 900*APP_XRES/1024
#define Y1_DEFAULT 100*APP_YRES/576
#define X2_DEFAUT 900*APP_XRES/1024
#define Y2_DEFAULT 150*APP_YRES/576
    unsigned int x_pos = 0;
    unsigned int y_pos = 0;
    char *prompt_name = NULL;
    char *wnd_name = NULL;
	CascamUiMailNotify type = 0;

	if(p == NULL)
	{
		printf("%s,%d, data is NULL!", __func__, __LINE__);
		return ;
	}
	type = *(CascamUiMailNotify*)p;


	if(type ==  CASCAM_UI_EMAIL_ICONHIDE){
		s_icon_hide_flag = 1;
		//_destroy_flicker_timer();
		//return;
	}else if(type ==  CASCAM_UI_EMAIL_NEW){
		prompt_name = PROMPT_EMAIL_NOTICE_NEW_NAME;
		wnd_name = WND_EMAIL_NOTICE_NEW;
		x_pos = X1_DEFAUT;
		y_pos = Y1_DEFAULT;
	}else if(type == CASCAM_UI_EMAIL_SPACE_FULL){
#if 1
		prompt_name = PROMPT_EMAIL_NOTICE_NEW_NAME;
		wnd_name = WND_EMAIL_NOTICE_NEW;
		x_pos = X1_DEFAUT;
		y_pos = Y1_DEFAULT;
#else
		prompt_name = PROMPT_EMAIL_NOTICE_FULL_NAME;
		wnd_name = WND_EMAIL_NOTICE_FULL;
		x_pos = X2_DEFAUT;
		y_pos = Y2_DEFAULT;
#endif
	}

	if(s_icon_hide_flag == 0)
	{
#if 1
		thiz_mail_param[0].index = type - 1;
		thiz_mail_param[0].show_state = 1;
		if(type == CASCAM_UI_EMAIL_NEW)
			thiz_mail_param[0].flicker = 0;
		else
			thiz_mail_param[0].flicker = 1;
		thiz_mail_param[0].x_pos = x_pos;
		thiz_mail_param[0].y_pos = y_pos;
		thiz_mail_param[0].prompt_name = prompt_name;
		thiz_mail_param[0].wnd_name = wnd_name;
#else
		thiz_mail_param[type-1].index = type - 1;
		thiz_mail_param[type-1].show_state = 1;
		if(type == CASCAM_UI_EMAIL_NEW)
			thiz_mail_param[type-1].flicker = 0;
		else
			thiz_mail_param[type-1].flicker = 1;
		thiz_mail_param[type-1].x_pos = x_pos;
		thiz_mail_param[type-1].y_pos = y_pos;
		thiz_mail_param[type-1].prompt_name = prompt_name;
		thiz_mail_param[type-1].wnd_name = wnd_name;
#endif
	}
	_create_flicker_timer(2);
}
#endif
#endif
