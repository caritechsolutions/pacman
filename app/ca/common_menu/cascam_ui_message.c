/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	cascam_ui_message.c
* Author    :	zhangyg
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  2.0	   2018.10.30		 zhangyg			  Creation
*****************************************************************************/
#include "app.h"
#if CASCAM_SUPPORT
#include "cascam_ui_common.h"
#include "cascam_ui_porting.h"
#if CASCAM_UI_MULTI_LAYER
#include "app_ca_osd_layer.h"
#endif

#define POP_NOTICE_NUM	2// 0, for pop msg; 1, for time pop msg
static CascamUiNoticeClass thiz_notice_record[POP_NOTICE_NUM];
static event_list  *pop_msg_timer = NULL;
#if CASCAM_UI_MULTI_LAYER
static CascamOsdLayerBlock pop_layer[POP_NOTICE_NUM];

static int cascam_osd_layer_config(CascamOsdLayerBlock* layer, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
#define  X_DEFAULT_POS 260*APP_XRES/1024
#define  Y_DEFAULT_POS 180*APP_YRES/576
#define  W_DEFAULT_VAL 500
#define  H_DEFAULT_VAL 140
	uint32_t  len = 0;

	if((x == 0) && (y == 0)){
		layer->x = X_DEFAULT_POS;
		layer->y = Y_DEFAULT_POS;
	}else{
		layer->x = x;
		layer->y = y;
	}
	if((w == 0) || (h == 0)){
		layer->w = W_DEFAULT_VAL;
		layer->h = H_DEFAULT_VAL;
	}else{
		layer->w = w;
		layer->h = h;
	}

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
	layer->layer_type = CASCAM_UI_POP;
	return 0;
}
void cascam_ui_create_layer_msg_dialog(CascamUiNoticeClass *para)
{
	uint8_t pop_type = 0;
	CascamOsdLayerBlock *layer;
	CascamRect rect = {0,};
    if((NULL == para)||(NULL == para->str)){
        printf("\nCAS, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }
	if(para->timeout_sec !=0 ){
		pop_type = 1;
	}else{
		pop_type = 0;
	}
	layer = &pop_layer[pop_type];
    if(layer->layer_name!=NULL){
        if((0 == memcmp(&thiz_notice_record[pop_type], para, sizeof(CascamUiNoticeClass)))
			&& (0 == pop_type)){
			// same
            return ;
        }
		cascam_osd_layer_quit(layer);
	}
    memcpy(&thiz_notice_record[pop_type], para, sizeof(CascamUiNoticeClass));
	cascam_osd_layer_config(layer, para->x, para->y, para->w, para->h);
	cascam_osd_layer_init(layer);
	rect.x = 0;
	rect.y = 0;
	rect.w = layer->w;
	rect.h = layer->h;
	cascam_osd_layer_fillrect(layer, rect);
	cascam_osd_layer_draw_string(layer, para->str, rect, (0<<0)|(0<<2)/*GUI_TA_LEFT | GUI_TA_TOP*/, CASCAM_STRING_PARAGRAPH);
}

void cascam_ui_exit_layer_msg_dialog(uint8_t pop_type)
{
    if(pop_layer[pop_type].layer_name!=NULL)
    {
		cascam_osd_layer_quit(&pop_layer[pop_type]);
    }
    memset(&thiz_notice_record[pop_type], 0, sizeof(CascamUiNoticeClass));
}
#else
// use separate dialog
#define WND_NOTICE                  "ca_notice_prompt" // in style.xml
#define PROMPT_NOTICE_NAME          "prompt_ca_notice"
#define WND_TIME_NOTICE             "ca_time_notice_prompt"
#define PROMPT_TIME_NOTICE_NAME     "prompt_ca_time_notice"
static char* win_pop[POP_NOTICE_NUM] = {WND_NOTICE, WND_TIME_NOTICE};
static char* prompt_pop_name[POP_NOTICE_NUM] = {PROMPT_NOTICE_NAME, PROMPT_TIME_NOTICE_NAME};
void cascam_ui_create_prompt_msg_dialog(CascamUiNoticeClass *para)
{
#define  X_DEFAULT_POS 320*APP_XRES/1024
#define  Y_DEFAULT_POS 152*APP_YRES/576
	uint8_t pop_type = 0;

    if((NULL == para)||(NULL == para->str))
    {
        printf("\nCAS, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }
	if(0 != para->timeout_sec){
		pop_type = 1;
	}else{
		pop_type = 0;
	}
    if(GUI_CheckPrompt(prompt_pop_name[pop_type]) == GXCORE_SUCCESS)
    {
        if((0 == memcmp(&thiz_notice_record[pop_type], para, sizeof(CascamUiNoticeClass)))
			&& pop_type == 0)
        {// same
            return ;
        }
        GUI_EndPrompt(prompt_pop_name[pop_type]);
    }
    memcpy(&thiz_notice_record[pop_type], para, sizeof(CascamUiNoticeClass));
    if((0 == para->x) && (0 == para->y))
    {
        GUI_CreatePrompt(X_DEFAULT_POS, Y_DEFAULT_POS, prompt_pop_name[pop_type], win_pop[pop_type], para->str, "ontop");
    }
    else
    {
        GUI_CreatePrompt(para->x, para->y, prompt_pop_name[pop_type], win_pop[pop_type], para->str, "ontop");
    }
}

void cascam_ui_exit_prompt_msg_dialog(uint8_t pop_type)
{
    if(GUI_CheckPrompt(prompt_pop_name[pop_type]) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(prompt_pop_name[pop_type]);
    }
    memset(&thiz_notice_record[pop_type], 0, sizeof(CascamUiNoticeClass));
}
#endif
static int pop_timeout_cb(void *usrdata)
{
	uint8_t pop_type = 1;
	if(0 != thiz_notice_record[pop_type].timeout_sec){
		thiz_notice_record[pop_type].timeout_sec--;
	}else{
		cascam_timer_stop(pop_msg_timer);
		//cascam_remove_timer(pop_msg_timer);
		//pop_msg_timer = NULL;
#if CASCAM_UI_MULTI_LAYER
		cascam_ui_exit_layer_msg_dialog(pop_type);
#else
		cascam_ui_exit_prompt_msg_dialog(pop_type);
#endif
	}
	return 0;
}

void cascam_ui_clean_msg(void)
{
	uint8_t pop_type = 0;
    memset(&thiz_notice_record[pop_type], 0, sizeof(CascamUiNoticeClass));
}

char cascam_ui_is_notice(void)
{
	uint8_t pop_type = 0;
    if(thiz_notice_record[pop_type].str)
        return 1;
    else
        return 0;
}

unsigned char *cascam_ui_get_notice_str(void)
{
	uint8_t pop_type = 0;
    return (unsigned char *)thiz_notice_record[pop_type].str;
}


void cascam_ui_create_msg_dialog(CascamUiNoticeClass *para)
{
	uint32_t duration = 1000;//ms
	uint32_t x = para->x;
	uint32_t y = para->y;
	uint32_t w = para->w;
	uint32_t h = para->h;
	uint8_t pop_type = 0;
	if(cascam_ui_check_notice_hide_by_video()){
		if(NULL != pop_msg_timer){
			cascam_timer_stop(pop_msg_timer);
			//cascam_remove_timer(pop_msg_timer);
			//pop_msg_timer = NULL;
		}
		//cascam_ui_exit_msg_dialog();
#if CASCAM_UI_MULTI_LAYER
		if(pop_layer[pop_type].layer_name!=NULL)
		{
			cascam_osd_layer_quit(&pop_layer[pop_type]);
		}
#else
		if(GUI_CheckPrompt(prompt_pop_name[pop_type]) == GXCORE_SUCCESS)
		{
			GUI_EndPrompt(prompt_pop_name[pop_type]);
		}
#endif
		memcpy(&thiz_notice_record[pop_type], para, sizeof(CascamUiNoticeClass));
		return;
	}
	cascam_ui_check_popmsg_create_postion(&x, &y, &w, &h);
	para->x = x;
	para->y = y;
	para->w = w;
	para->h = h;
#if CASCAM_UI_MULTI_LAYER
	cascam_ui_create_layer_msg_dialog(para);
#else
	cascam_ui_create_prompt_msg_dialog(para);
#endif
	if(para->timeout_sec){
		//time pop msg
		if(NULL != pop_msg_timer){
			cascam_timer_stop(pop_msg_timer);
			//cascam_remove_timer(pop_msg_timer);
			//pop_msg_timer = NULL;
		}
		if(cascam_reset_timer(pop_msg_timer, duration) != 0)
		{
			pop_msg_timer = cascam_create_timer(pop_timeout_cb, duration, NULL, TIMER_REPEAT);
		}
	}
}

void cascam_ui_exit_msg_dialog(void)
{
	uint8_t pop_type = 0;
#if CASCAM_UI_MULTI_LAYER
	cascam_ui_exit_layer_msg_dialog(pop_type);
#else
	cascam_ui_exit_prompt_msg_dialog(pop_type);
#endif
}

//for app
static char* sMsgString = NULL;
void cascam_ui_create_msg_dialog_for_app(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
	CascamUiNoticeClass para = {0};
	sMsgString = (char*)cascam_ui_get_notice_str();
	if(NULL == sMsgString)
		return;
	para.x = x;
	para.y = y;
	para.w = w;
	para.h = h;
	para.str = sMsgString;
#if CASCAM_UI_MULTI_LAYER
	cascam_ui_exit_layer_msg_dialog(0);
	cascam_ui_create_layer_msg_dialog(&para);
#else
	cascam_ui_exit_prompt_msg_dialog(0);
	cascam_ui_create_prompt_msg_dialog(&para);
#endif
}

void cascam_ui_exit_msg_dialog_for_app(void)
{
	uint8_t pop_type = 0;
	if(NULL != pop_msg_timer){
		sMsgString = NULL;
		return;
	}
	sMsgString = (char*)cascam_ui_get_notice_str();
	if(NULL == sMsgString)
		return;
#if CASCAM_UI_MULTI_LAYER
	//cascam_ui_exit_layer_msg_dialog(pop_type);
	if(pop_layer[pop_type].layer_name!=NULL)
    {
		cascam_osd_layer_quit(&pop_layer[pop_type]);
    }
#else
	//cascam_ui_exit_prompt_msg_dialog(pop_type);
	if(GUI_CheckPrompt(prompt_pop_name[pop_type]) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(prompt_pop_name[pop_type]);
    }
#endif
}
#endif
