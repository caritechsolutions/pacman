/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_cgcas_fingerprint.c
* Author    :	huangwf
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2021.03.25		huangwf			  Creation
*****************************************************************************/
#include "app.h"
#include "app_module.h"
#ifdef CASCAM_CGCAS_SUPPORT
#include "cascam.h"
#include "cascam_ui_common.h"
#include "cascam_ui_porting.h"
#include "cascam_cryptoguardcas_data.h"
#include "app_ca_osd_layer.h"

//extern void app_cgcas_set_force_lock_status(int type, char force_flag);

#define FINGER_WIDTH	860
#define FINGER_HEIGHT	560
#define X_RATE			1024*100/1024
#define Y_RATE			576*100/576
#define RATE			100

#define FINGER_HORIZONTAL_OFFSET    40
#define FINGER_VERTICAL_OFFSET      46
#define SCREEN_BOTTOM_ROLLING_Y		516
#define FINGER_AVOID_H				65

static char thiz_fingerprint_content[50] = {0};
static unsigned char thiz_fingerprint_inner_pos[17] = {0};
static Cascam_CGFingerInfo thiz_cg_finger_info = {{0},0};
#define CASCAM_UI_MULTI_LAYER 1
#if CASCAM_UI_MULTI_LAYER
static CascamOsdLayerBlock cgcas_finger_layer;
#endif
#define PROMPT_FINGER_NAME          "prompt_ca_fingerprint"
#define TXT_FINGER                  "txt_fingerprint"

#define PROMPT_SUPER_FP_NAME          "prompt_ca_fingerprint"
#define WND_SUPER_FP                  "ca_fingerprint_prompt"

static void _generat_random_pos(unsigned int *x, unsigned int *y, unsigned int *w, unsigned int *h,
                                  unsigned int *b_x, unsigned int *b_y, unsigned int *b_w, unsigned int *b_h)
{
// 区域控制，排除ROLLING信息的影???
#define SCREEN_WIDTH                860
#define SCREEN_HEIGHT               420
#define SCREEN_HORIZONTAL_OFFSET    40
#define SCREEN_VERTICAL_OFFSET      78

#define CENTRE_RECT_LX 149
#define CENTRE_RECT_RX 701
#define CENTRE_RECT_TY 134
#define CENTRE_RECT_BY 286

#define FINGERPRINT_DEFAULT_WIDTH 220
#define FINGERPRINT_DEFAULT_HEIGHT 30

    unsigned int fingerprint_width = 0;
    unsigned int fingerprint_height = 0;
    unsigned int tmp_x = 0;
    unsigned int tmp_y = 0;
#if CASCAM_UI_MULTI_LAYER
	fingerprint_width = FINGERPRINT_DEFAULT_WIDTH;
    fingerprint_height = FINGERPRINT_DEFAULT_HEIGHT;
#else
	GuiWidget *widget = NULL;
    widget = gui_get_window("prompt_ca_fingerprint");
	//widget = gui_get_widget(NULL, WND_SUPER_FP);
    if(widget == NULL)
    {
        printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }

    fingerprint_width = widget->rect.w;
    fingerprint_height = widget->rect.h;
#endif
    tmp_x = (rand() % (SCREEN_WIDTH - fingerprint_width - SCREEN_HORIZONTAL_OFFSET)) + SCREEN_HORIZONTAL_OFFSET;
    tmp_y = (rand() % (SCREEN_HEIGHT - fingerprint_height - SCREEN_VERTICAL_OFFSET)) + SCREEN_VERTICAL_OFFSET;

    if (tmp_y > CENTRE_RECT_TY && tmp_y < CENTRE_RECT_BY && tmp_x > CENTRE_RECT_LX && tmp_y < CENTRE_RECT_RX)
    {
        if (tmp_y > (SCREEN_HEIGHT - fingerprint_height)/2)
        {
            tmp_y = (rand() % (SCREEN_HEIGHT - CENTRE_RECT_BY)) + CENTRE_RECT_BY;
        }
        else
        {
            tmp_y = (rand() % (CENTRE_RECT_TY - SCREEN_HORIZONTAL_OFFSET)) + SCREEN_HORIZONTAL_OFFSET;
        }
    }

    *x = tmp_x;
    *y = tmp_y;
    *w = fingerprint_width;
    *h = fingerprint_height;

#if !CASCAM_UI_MULTI_LAYER
    *b_x =  widget->rect.x;
    *b_y =  widget->rect.y;
    *b_w =  widget->rect.w;
    *b_h =  widget->rect.h;
#endif
}
#if 0
static uint32_t _cas_change_color(uint32_t icolor)
{
	uint32_t ocolor = 0;
	uint8_t color_A	= 0;
	uint8_t color_R	= 0;
	uint8_t color_G = 0;
	uint8_t color_B = 0;

	color_A = (icolor & 0xff000000) >> 24;
	color_R = (icolor & 0xff0000) >> 16;
	color_G = (icolor & 0xff00) >> 8;
	color_B = icolor & 0xff;
	ocolor = (color_R << 16)|(color_G << 8)|color_B ;
	if (ocolor == 0xFF00FF)
	{
		ocolor = 0xEF00EF;
	}
	else if (ocolor == 0x00FF00)
	{
		ocolor = 0x04F004;
	}
	ocolor |= (color_A << 24);
	return ocolor;
}
#endif
#if CASCAM_UI_MULTI_LAYER
static int cgcas_finger_osd_layer_config(CascamOsdLayerBlock* layer)
{
    uint32_t  len = 0;
    unsigned int x = 0;
    unsigned int y = 0;
    unsigned int w = 0;
    unsigned int h = 0;
    unsigned int b_x = 0;
    unsigned int b_y = 0;
    unsigned int b_w = 0;
    unsigned int b_h = 0;

    _generat_random_pos(&x, &y, &w, &h, &b_x, &b_y, &b_w, &b_h);
    if ((thiz_cg_finger_info.X >= 0) && (thiz_cg_finger_info.X <= 15))
        layer->x = 20+thiz_cg_finger_info.X * APP_XRES / 16;
    else
        layer->x = x;

    if((thiz_cg_finger_info.Y >= 0) && (thiz_cg_finger_info.Y <= 15))
        layer->y = 20+thiz_cg_finger_info.Y * APP_YRES / 16;
    else
        layer->y = y;
    layer->w = 13*strlen(thiz_cg_finger_info.DisplayText);//font_size*num
    layer->h = h;
    layer->x = (layer->x + layer->w)<(APP_XRES-20)? layer->x:(APP_XRES-20-layer->w);
    layer->y = (layer->y + layer->h)<(APP_YRES-20)? layer->y:(APP_YRES-20-layer->h);

    layer->fore_color = thiz_cg_finger_info.FontColor;
    layer->back_color = thiz_cg_finger_info.BackgroundColor;


    if (layer->font_name !=NULL) {
        GxCore_Free(layer->font_name);
        layer->font_name = NULL;
    }

    len = strlen(DEFAULT_FONT);
    layer->font_name = (uint8_t*)GxCore_Malloc(len+1);
    memset(layer->font_name, 0, len + 1);
    memcpy(layer->font_name, DEFAULT_FONT, len);
    layer->layer_type = CASCAM_UI_FINGER;

    return 0;
}
#endif

int _cgcas_finger_ready(void)
{
	GuiWidget *emm_fp_prompt_window = NULL, *emm_fp_prompt_text = NULL;
	emm_fp_prompt_window = gui_get_widget(NULL, WND_SUPER_FP);
	if(emm_fp_prompt_window == NULL)
	{
		printf("[SUPER FINGER]--------ERROR--------\n");
		return -1;
	}

	widget_set_property(emm_fp_prompt_window, "backcolor","[#ff00ff,#ff00ff,#808080]");
	emm_fp_prompt_text = widget_new_child(emm_fp_prompt_window, "emm_fp_prompt_text00", "text", "default");

	widget_set_property(emm_fp_prompt_text, "font", "font_prog");
	widget_set_property(emm_fp_prompt_text, "backcolor", "[#ff00ff,#ff00ff,#408DD2]");
	widget_set_property(emm_fp_prompt_text, "forecolor", "[#CEE8FF,#FFFFFF,#CEE8FF]");

	widget_set_property(emm_fp_prompt_text, "string", thiz_fingerprint_content);

	widget_set_property(emm_fp_prompt_text, "rect", "[0,0,150,30]");
	emm_fp_prompt_text->init = TRUE;

	widget_set_property(emm_fp_prompt_window, "rect", "[0,0,150,30]");
	return 0;
}

int cgcas_finger_func(void);
int cgcas_finger_stop(void);

static event_list  *thiz_stage_timer = NULL;

static int _finger_display(void)
{
#if CASCAM_UI_MULTI_LAYER
	CascamRect rect = {0,};
#else
    unsigned int x = 0;
    unsigned int y = 0;
    unsigned int w = 0;
    unsigned int h = 0;
    unsigned int b_x = 0;
    unsigned int b_y = 0;
    unsigned int b_w = 0;
    unsigned int b_h = 0;
#endif
#if CASCAM_UI_MULTI_LAYER
	memset(&cgcas_finger_layer, 0, sizeof(CascamOsdLayerBlock));
	cascam_osd_layer_quit(&cgcas_finger_layer);
	cgcas_finger_osd_layer_config(&cgcas_finger_layer);
	cascam_osd_layer_init(&cgcas_finger_layer);
	rect.x = 0;
	rect.y = 0;
	rect.w = cgcas_finger_layer.w;
	rect.h = cgcas_finger_layer.h;
	cascam_osd_layer_fillrect(&cgcas_finger_layer, rect);

	cascam_osd_layer_draw_string(&cgcas_finger_layer, thiz_fingerprint_content, rect, (2<<0)|(3<<2)/*GUI_TA_HCENTRE | GUI_TA_VCENTRE*/, CASCAM_STRING_SINGLE);
#else
    _generat_random_pos(&x, &y, &w, &h, &b_x, &b_y, &b_w, &b_h);
	if (GUI_CheckPrompt(PROMPT_FINGER_NAME) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(PROMPT_FINGER_NAME);
    }
    //GUI_CreatePrompt(x, y, PROMPT_FINGER_NAME, WND_FINGER, NULL, "ontop");
#endif
    return 0;
}

static int _finger_display_timer(void *usrdata)
{
	cascam_timer_stop(thiz_stage_timer);
	//cascam_remove_timer(thiz_stage_timer);
	//thiz_stage_timer = NULL;
	if (0 == cascam_ui_check_notice_hide())
		_finger_display();
#if CASCAM_UI_MULTI_LAYER
	else
		cascam_osd_layer_quit(&cgcas_finger_layer);
#endif
    return 0;
}

static int _destroy_stage_timer(void)
{
    int ret = 0;
    if(thiz_stage_timer != NULL)
    {
        cascam_timer_stop(thiz_stage_timer);
        //cascam_remove_timer(thiz_stage_timer);
        //thiz_stage_timer = NULL;
    }
    return ret;
}

static int _create_normal_finger_timer(unsigned int duration_ms)
{
    if(thiz_stage_timer)
    {
        cascam_timer_stop(thiz_stage_timer);
        //cascam_remove_timer(thiz_stage_timer);
        //thiz_stage_timer = NULL;
    }
	//if(0 == cascam_ui_check_notice_hide())
    if(cascam_reset_timer(thiz_stage_timer, duration_ms) != 0)
    {
        thiz_stage_timer = cascam_create_timer(_finger_display_timer, duration_ms, NULL, TIMER_REPEAT_NOW);
    }
    return 0;
}

// normal finger function
int cgcas_finger_func(void)
{
#if CASCAM_UI_MULTI_LAYER
	//CascamRect rect = {0,};
#else
    unsigned int x = 100;
    unsigned int y = 100;
#endif

    _destroy_stage_timer();
    _create_normal_finger_timer(1000);

    return 0;
}

int cgcas_finger_stop(void)
{
#if CASCAM_UI_MULTI_LAYER
	cascam_osd_layer_quit(&cgcas_finger_layer);
#else
    if (GUI_CheckPrompt(PROMPT_FINGER_NAME) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(PROMPT_FINGER_NAME);
       // GUI_SetInterface("flush", NULL);
    }
#endif
	return 0;
}

static event_list  *thiz_finger_timer = NULL;
static event_list  *thiz_finger_hide_timer = NULL;

static char thiz_finger_create_status = 0;// 1: create; 0:destroy
static int _cgcas_finger_timer(void *usrdata)
{
    cascam_timer_stop(thiz_finger_timer);
    //cascam_remove_timer(thiz_finger_timer);
    //thiz_finger_timer = NULL;
    if (thiz_finger_create_status)
    {
        _destroy_stage_timer();
        cgcas_finger_func();
    }
    else
    {
        _destroy_stage_timer();
        cgcas_finger_stop();
    }
    return 0;
}

static int _cgcas_finger_hide_timer(void *usrdata)
{
	if(thiz_finger_create_status)
	{
		_destroy_stage_timer();
		cgcas_finger_stop();
	}

    return 0;
}

static int _cgcas_create_finger_timer(char flag, Cascam_CGFingerInfo *para)
{
    memset(&thiz_cg_finger_info, 0, sizeof(Cascam_CGFingerInfo));
    if (flag)
    {
        if (para == NULL)
        {
            printf("\nCA, error, %s, %d\n", __FUNCTION__,__LINE__);
            return -1;
        }
        else
        {
            memcpy(&thiz_cg_finger_info, para, sizeof(Cascam_CGFingerInfo));
        }
    }

    thiz_finger_create_status = flag;
    if(thiz_finger_timer)
    {
        cascam_timer_stop(thiz_finger_timer);
        //cascam_remove_timer(thiz_finger_timer);
        //thiz_finger_timer = NULL;
    }
    if(thiz_finger_hide_timer)
    {
        cascam_timer_stop(thiz_finger_hide_timer);
        //cascam_remove_timer(thiz_finger_hide_timer);
        //thiz_finger_hide_timer = NULL;

    }
    if(cascam_reset_timer(thiz_finger_hide_timer, thiz_cg_finger_info.DurationMs) != 0){
        thiz_finger_hide_timer = cascam_create_timer(_cgcas_finger_hide_timer, thiz_cg_finger_info.DurationMs, NULL, TIMER_ONCE);
    }

    if(cascam_reset_timer(thiz_finger_timer, 100) != 0)
    {
        thiz_finger_timer = cascam_create_timer(_cgcas_finger_timer, 100, (void*)&thiz_cg_finger_info, TIMER_REPEAT_NOW);
    }

    return 0;
}

void app_cgcas_fingerprint_create(Cascam_CGFingerInfo *para)
{
    unsigned int len = 0;

    if (NULL == para)
    {
        printf("\nCA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return;
    }

    len = strlen(para->DisplayText);
    if (len > sizeof(thiz_fingerprint_content))
    {
        printf("\nCA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return;
    }

    memset(thiz_fingerprint_content, 0, sizeof(thiz_fingerprint_content));
    memcpy(thiz_fingerprint_content, para->DisplayText, len);

    _destroy_stage_timer();
    cgcas_finger_stop();

    _cgcas_create_finger_timer(1, para);
}

void app_cgcas_fingerprint_exit(void)
{
    memset(thiz_fingerprint_content, 0, sizeof(thiz_fingerprint_content));
    memset(thiz_fingerprint_inner_pos, 0, sizeof(thiz_fingerprint_inner_pos));
    //_cgcas_create_finger_timer(0,NULL);
    cascam_timer_stop(thiz_finger_timer);
    //cascam_remove_timer(thiz_finger_timer);
    //thiz_finger_timer = NULL;

    GxCore_ThreadDelay(100);
    _destroy_stage_timer();
    cgcas_finger_stop();

    //app_cgcas_set_force_lock_status(1,0);
}

void app_cgcas_finger_notify(void *para)
{
	Cascam_CGFingerInfo *p = NULL;

    if (NULL == para)
    {
        printf("\nCA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return;
    }

	p = (Cascam_CGFingerInfo *)para;
	app_cgcas_fingerprint_create(p);
}

void app_cgcas_finger_destroy_for_app(void)
{
	if(thiz_finger_create_status)
	{
        _destroy_stage_timer();
        cgcas_finger_stop();
	}
}

#endif
