#include "app.h"
#if CASCAM_SUPPORT
#include "cascam_ui_porting.h"
#include "app_ca_osd_layer.h"
#include "gui_core/gdi_core.h"

#if CASCAM_UI_MULTI_LAYER
static CascamLayerCtrl s_layer_ctrl;
static handle_t s_layer_ctrl_mutex = -1;

static void _layer_block_add(CascamLayerBlock* pBlock)
{
	CascamLayerBlock* temp = NULL;
	CascamLayerBlock* p = s_layer_ctrl.layer_block;

	if(NULL == pBlock){
		printf("----error-----,_layer_block_add\n");
		return;
	}
	if(NULL == p){
		temp = GxCore_Malloc(sizeof(CascamLayerBlock));
		memset(temp, 0, sizeof(CascamLayerBlock));
		memcpy(temp, pBlock, sizeof(CascamLayerBlock));
		s_layer_ctrl.layer_block = temp;
		s_layer_ctrl.num ++;
		return;
	}

	while(NULL != p->next_ptr){
		p = p->next_ptr;
	}
	temp = GxCore_Malloc(sizeof(CascamLayerBlock));
	memset(temp, 0, sizeof(CascamLayerBlock));
	memcpy(temp, pBlock, sizeof(CascamLayerBlock));
	p->next_ptr = temp;
	s_layer_ctrl.num ++;
	return;
}
static void _layer_block_remove(CascamLayerBlock* pBlock)
{
	CascamLayerBlock* temp = NULL;
	CascamLayerBlock* p = s_layer_ctrl.layer_block;

	if(NULL == pBlock){
		printf("----error-----,_layer_block_remove\n");
		return;
	}

	if(NULL == p){
		printf("-----error-----, layer block list is NULL\n");
		return;
	}
	if((!strcmp(pBlock->layer_name, p->layer_name))&&(pBlock->layer_type == p->layer_type)){
		if(NULL == p->next_ptr){
			GxCore_Free(p);
			s_layer_ctrl.layer_block = NULL;
		}else{
			temp = p->next_ptr;
			GxCore_Free(p);
			s_layer_ctrl.layer_block = temp;
		}
		s_layer_ctrl.num --;
		return;
	}
	while(NULL != p->next_ptr){
		if((!strcmp(pBlock->layer_name, ((CascamLayerBlock*)(p->next_ptr))->layer_name))
				&&(pBlock->layer_type == ((CascamLayerBlock*)(p->next_ptr))->layer_type)){
			temp = p->next_ptr;
			p->next_ptr = ((CascamLayerBlock*)(p->next_ptr))->next_ptr;
			GxCore_Free(temp);
			s_layer_ctrl.num --;
			return;
		}
		p = p->next_ptr;
	}
	printf("no needed layer block found to remove\n");
	return;
}
static void _set_layer_priority(void)
{
	char* layer_finger_name = NULL;
	char* layer_pop_name = NULL;
	int num = s_layer_ctrl.num;
	CascamLayerBlock* p = s_layer_ctrl.layer_block;

	//finger first
	while(NULL!=p){
		if(p->layer_type == CASCAM_UI_FINGER)
			layer_finger_name = p->layer_name;
		if(p->layer_type == CASCAM_UI_POP)
			layer_pop_name = p->layer_name;
		p = p->next_ptr;
	}
	if((layer_finger_name!=NULL)&&(layer_pop_name!=NULL)){
		GXGDI_SetLayerPriority(layer_finger_name, num-1);
		GXGDI_SetLayerPriority(layer_pop_name, num-2);
	}else if(layer_finger_name!=NULL){
		GXGDI_SetLayerPriority(layer_finger_name, num-1);
	}else if(layer_pop_name!=NULL){
		GXGDI_SetLayerPriority(layer_pop_name, num-1);
	}
}

int cascam_osd_layer_init(CascamOsdLayerBlock* layer)
{
	GuiSurface *surface = NULL;
    char *layer_name = NULL;
	GAL_Rect rect = {0,};
	CascamLayerBlock layer_block;
	GXGDI_Rect fill_rect = {0};

	if (-1 == s_layer_ctrl_mutex)
		GxCore_MutexCreate(&s_layer_ctrl_mutex);

	GxCore_MutexLock(s_layer_ctrl_mutex);
	rect.x = layer->x;
	rect.y = layer->y;
	rect.w = layer->w;
	rect.h = layer->h;
	surface = hd_get_surface(&rect, DEFAULT_BPP, NULL, FALSE);
    if(NULL == surface)
    {
		printf("---error-----!Get surface failed.\n");
		GxCore_MutexUnlock(s_layer_ctrl_mutex);
		return -1;
    }
	fill_rect.x = 0;
	fill_rect.y = 0;
	fill_rect.w = layer->w;
	fill_rect.h = layer->h;
	GXGDI_Lock();
	GXGDI_Begin();
    GXGDI_FillRect(surface, &fill_rect, layer->back_color);
	GXGDI_End();
	GXGDI_Unlock();

	layer_name = GxGDI_LayerRegister(surface, rect.x, rect.y);
    if(NULL == layer_name)
    {
		printf("---error-----!Register surface failed.\n");
		GxCore_MutexUnlock(s_layer_ctrl_mutex);
		return -1;
    }
	layer->surface = surface;
	layer->layer_name = layer_name;
	memset(&layer_block, 0, sizeof(CascamLayerBlock));
	layer_block.layer_name = layer->layer_name;
	layer_block.layer_type = layer->layer_type;
	_layer_block_add(&layer_block);
	_set_layer_priority();
	GxCore_MutexUnlock(s_layer_ctrl_mutex);
	return 0;
}
int cascam_osd_layer_fillrect(CascamOsdLayerBlock* layer, CascamRect rect)
{
	GXGDI_Rect fill_rect = {0};

	GxCore_MutexLock(s_layer_ctrl_mutex);
    if(NULL == layer->surface)
    {
		printf("---error-----!Fill NULL surface.\n");
		GxCore_MutexUnlock(s_layer_ctrl_mutex);
		return -1;
    }

    fill_rect.x = rect.x;
    fill_rect.y = rect.y;
    fill_rect.w = rect.w;
    fill_rect.h = rect.h;
	GXGDI_Lock();
	GXGDI_Begin();
    GXGDI_FillRect(layer->surface, &fill_rect, layer->back_color);
	GXGDI_End();
    GxGDI_LayerUpdate(layer->layer_name);
	GXGDI_Unlock();
	GxCore_MutexUnlock(s_layer_ctrl_mutex);
	return 0;
}
int cascam_osd_layer_draw_string(CascamOsdLayerBlock* layer, void* string, CascamRect rect, int alignment, CascamStringType flag)
{// alignment: GUI_TA_HCENTRE | GUI_TA_VCENTRE; flag: GDI_STRING_PARAGRAPH or GDI_STRING_SINGLE
	GXGDI_Rect draw_rect = {0};
	uint8_t* font_name = NULL;
    char *i18n_string = NULL;

	GxCore_MutexLock(s_layer_ctrl_mutex);
    if(NULL == layer->surface)
    {
		printf("---error-----!Fill NULL surface.\n");
		GxCore_MutexUnlock(s_layer_ctrl_mutex);
		return -1;
    }

    draw_rect.x = rect.x;
    draw_rect.y = rect.y;
    draw_rect.w = rect.w;
    draw_rect.h = rect.h;
    i18n_string = (char *)GXGDI_I18N_String(string);
	GXGDI_Lock();
	GXGDI_Begin();
	font_name = (uint8_t*)GXGDI_SetFont(layer->font_name);
    if(i18n_string)
        GXGDI_DrawString(layer->surface, i18n_string, &draw_rect, alignment, layer->fore_color, flag);
    else
        GXGDI_DrawString(layer->surface, string, &draw_rect, alignment, layer->fore_color, flag);
	GXGDI_End();
	GXGDI_SetFont(font_name);
    GxGDI_LayerUpdate(layer->layer_name);
	GXGDI_Unlock();
	GxCore_MutexUnlock(s_layer_ctrl_mutex);
	return 0;
}

static int delay = DEFAULT_ROLL_DELAY;
int cascam_osd_layer_set_rolling_delay(uint32_t delay_time)
{
   if(delay_time > 0){
       delay = delay_time;
   }
   return 0;
}

int cascam_osd_layer_start_rolling_string(CascamOsdLayerBlock* layer, void* string, CascamRect rect, CascamRollDirect flag)
{
	GXGDI_Rect draw_rect = {0};

	GxCore_MutexLock(s_layer_ctrl_mutex);
    if(NULL == layer->surface)
    {
		printf("---error-----!Fill NULL surface.\n");
		GxCore_MutexUnlock(s_layer_ctrl_mutex);
		return -1;
    }
    draw_rect.x = rect.x;
    draw_rect.y = rect.y;
    draw_rect.w = rect.w;
    draw_rect.h = rect.h;
	layer->roll_handle = GXGDI_StartRollString(layer->surface,
											(char*)layer->font_name,
											string,
											&draw_rect,
											layer->fore_color,
											layer->back_color,
											delay,
											flag);
    delay = DEFAULT_ROLL_DELAY;
	GxCore_MutexUnlock(s_layer_ctrl_mutex);
	return 0;
}
int cascam_osd_layer_stop_rolling_string(CascamOsdLayerBlock* layer)
{
	GxCore_MutexLock(s_layer_ctrl_mutex);
    if(layer->roll_handle <= 0)
    {
		printf("--%s,--%d---error-----!Roll handle error.\n", __func__, __LINE__);
		GxCore_MutexUnlock(s_layer_ctrl_mutex);
		return -1;
    }

	GXGDI_StopRollString(layer->roll_handle);
	layer->roll_handle = 0;
//	GxCore_ThreadDelay(100);
	GxCore_MutexUnlock(s_layer_ctrl_mutex);
	return 0;
}
int cascam_osd_layer_resume_rolling_string(int handle)
{
    if(handle <= 0)
    {
		printf("--%s,--%d---error-----!Roll handle error.\n", __func__, __LINE__);
		return -1;
    }

	GxCore_MutexLock(s_layer_ctrl_mutex);
	GXGDI_ResetRollString(handle);
	GxCore_MutexUnlock(s_layer_ctrl_mutex);
	return 0;
}
int cascam_osd_layer_pause_rolling_string(int handle)
{
    if(handle <= 0)
    {
		printf("--%s,--%d---error-----!Roll handle error.\n", __func__, __LINE__);
		return -1;
    }

	GxCore_MutexLock(s_layer_ctrl_mutex);
	GXGDI_PauseRollString(handle);
	GxCore_MutexUnlock(s_layer_ctrl_mutex);
	return 0;
}
int cascam_osd_layer_get_roll_times(int handle)
{
	int value = 0;
    if(handle <= 0)
    {
		printf("--%s,--%d---error-----!Roll handle error.\n", __func__, __LINE__);
		return -1;
    }

	GxCore_MutexLock(s_layer_ctrl_mutex);
	value = GXGDI_GetRollTimes(handle);
	GxCore_MutexUnlock(s_layer_ctrl_mutex);
	return value;
}
int cascam_osd_layer_get_image_width(char* filename)
{
	int value = 0;
	GXGDI_Lock();
	value = gal_img_get_width(filename);
	GXGDI_Unlock();
	return value;
}
int cascam_osd_layer_get_image_height(char* filename)
{
	int value = 0;
	GXGDI_Lock();
	value = gal_img_get_height(filename);
	GXGDI_Unlock();
	return value;
}
int cascam_osd_layer_start_draw_image(CascamOsdLayerBlock* layer, uint32_t x_pos, uint32_t y_pos, char* filename)
{
	image_desc *pdesc = NULL;

	GxCore_MutexLock(s_layer_ctrl_mutex);
	GXGDI_Lock();
	pdesc = gal_img_load(NULL, filename);
	if(NULL == pdesc){
		printf("---error-----!load img error.\n");
//		GXGDI_End();
		GXGDI_Unlock();
		GxCore_MutexUnlock(s_layer_ctrl_mutex);
		return -1;
    }
	layer->img_desc = (void*)pdesc;
	GXGDI_Begin();
	GXGDI_DrawImage(layer->surface, pdesc, x_pos, y_pos);
	GXGDI_End();
    GxGDI_LayerUpdate(layer->layer_name);
	GXGDI_Unlock();
	GxCore_MutexUnlock(s_layer_ctrl_mutex);
	return 0;
}
int cascam_osd_layer_stop_draw_image(CascamOsdLayerBlock* layer)
{
	if(NULL != layer->img_desc){
		GxCore_MutexLock(s_layer_ctrl_mutex);
		GXGDI_Lock();
		gal_img_release((image_desc*)(layer->img_desc));
		GXGDI_Unlock();
		layer->img_desc = NULL;
		GxCore_MutexUnlock(s_layer_ctrl_mutex);
	}
	return 0;
}
int cascam_osd_layer_quit(CascamOsdLayerBlock* layer)
{
	status_t ret = 0;
	CascamLayerBlock layer_block;

	if (-1 == s_layer_ctrl_mutex)
		GxCore_MutexCreate(&s_layer_ctrl_mutex);

	GxCore_MutexLock(s_layer_ctrl_mutex);
	if(NULL == layer->layer_name)
    {
    	GxCore_MutexUnlock(s_layer_ctrl_mutex);
		return 0;
    }
	memset(&layer_block, 0, sizeof(CascamLayerBlock));
	layer_block.layer_name = layer->layer_name;
	layer_block.layer_type = layer->layer_type;
	_layer_block_remove(&layer_block);
	ret = GxGDI_LayerUnregister(layer->layer_name);
    if(GXCORE_SUCCESS != ret)
    {
		printf("---error-----!GxGDI_LayerUnregister failed.\n");
    }
	hd_free_surface(layer->surface);
	layer->surface = NULL;
	layer->layer_type = 0;
	if (layer->layer_name != NULL) {
		//GUI_FREE(layer->layer_name);
		layer->layer_name = NULL;
	}
	if (layer->font_name != NULL) {
		GxCore_Free(layer->font_name);
		layer->font_name = NULL;
	}
	GxCore_MutexUnlock(s_layer_ctrl_mutex);
	return 0;
}
int cascam_osd_layer_get_font_height(void* font)
{
	int len = 0;
	uint8_t* font_name = NULL;
    if(font == NULL)
    {
		printf("---error-----!font is NULL.\n");
		return -1;
    }

	GXGDI_Lock();
	font_name = (uint8_t*)GXGDI_SetFont(font);
	len = GXGDI_GetFontHeight(GXGDI_GetFont());
	GXGDI_SetFont(font_name);
	GXGDI_Unlock();
	return len;
}
int cascam_osd_layer_get_string_pixels(void* string, uint8_t* font)
{
	int len = 0;
	uint8_t* font_name = NULL;
    if(string == NULL)
    {
		printf("---error-----!string is NULL.\n");
		return -1;
    }

	GXGDI_Lock();
	font_name = (uint8_t*)GXGDI_SetFont(font);
	len = GXGDI_GetStringPixels(string);
	GXGDI_SetFont(font_name);
	GXGDI_Unlock();
	return len;
}
extern int hd_zoom_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect);
// zoom surface
int cascam_osd_layer_zoom(CascamOsdLayerBlock* layer, CascamRect src_rect, CascamRect dst_rect, void* string, int alignment, CascamStringType flag)
{
	int32_t ret = 0;
	uint8_t* font_name = NULL;
	GuiSurface *surface = NULL;
    char *i18n_string = NULL;

	GxCore_MutexLock(s_layer_ctrl_mutex);
    if(NULL == layer->surface)
    {
		printf("---error-----!Fill NULL surface.\n");
		GxCore_MutexUnlock(s_layer_ctrl_mutex);
		return -1;
    }
	surface = hd_get_surface((GAL_Rect*)&src_rect, DEFAULT_BPP, NULL, FALSE);
    if(NULL == surface)
    {
		printf("---error-----!Get surface failed.\n");
		GxCore_MutexUnlock(s_layer_ctrl_mutex);
		return -1;
    }
    i18n_string = (char *)GXGDI_I18N_String(string);
	GXGDI_Lock();
	GXGDI_Begin();
	font_name = (uint8_t*)GXGDI_SetFont(layer->font_name);
    GXGDI_FillRect(surface, (GXGDI_Rect*)&src_rect, layer->back_color);
    if(i18n_string)
        GXGDI_DrawString(surface, i18n_string, (GXGDI_Rect*)&src_rect, alignment, layer->fore_color, flag);
    else
        GXGDI_DrawString(surface, string, (GXGDI_Rect*)&src_rect, alignment, layer->fore_color, flag);
	ret =  hd_zoom_surface(surface, (GAL_Rect*)&src_rect, layer->surface, (GAL_Rect*)&dst_rect);
	if (GXCORE_SUCCESS != ret)
		{
			printf("---error----!!hd_zoom_surface  error ret=%d src_rect x=%d y=%d w=%d h=%d dst_rect x=%d y=%d w=%d h=%d\n",
				ret,src_rect.x,src_rect.y,src_rect.w,src_rect.h,dst_rect.x,dst_rect.y,dst_rect.w,dst_rect.h);
			GXGDI_End();
			GXGDI_Unlock();
			GxCore_MutexUnlock(s_layer_ctrl_mutex);
			return -1;
		}
	//zoom
	GXGDI_End();
	GXGDI_SetFont(font_name);
    GxGDI_LayerUpdate(layer->layer_name);
	GXGDI_Unlock();
	hd_free_surface(surface);
	GxCore_MutexUnlock(s_layer_ctrl_mutex);
	return 0;
}
#endif
#endif
