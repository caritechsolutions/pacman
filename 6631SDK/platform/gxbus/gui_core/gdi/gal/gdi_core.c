#include "gdi_core.h"
#include "gui_private.h"
#include "gdi_font.h"
#include "gui.h"
#include "gxavdev.h"

GXGDI_FontOpt *g_font_opt_fun;
extern void _copy_double_surface(void);

/*OSD surface*/
void *GXGDI_GetMainOSDSurface(void)
{
	return (screen);
}

void *GXGDI_GetBackOSDSurface(void)
{
	return (back_screen);
}

status_t GXGDI_SPP_OnOff(int flag)
{
	return (gal_enable_img(flag));
}

status_t GXGDI_OSD_OnOff(int flag)
{
	return (gdi_enable(flag));
}

/*Color*/
int GXGDI_GetColorKey(void)
{
	return (gal_get_gui_transparent());
}

/*SPP surface*/
void *GXGDI_GetMainSPPSurface(void)
{
	return (spp_screen);
}

void GXGDI_Lock(void)
{
	gdi_lock();
}

void GXGDI_Unlock(void)
{
	gdi_unlock();
}

/*Image*/
status_t GXGDI_DrawImage(void *surface, const void *pdesc, int x, int y)
{
	return (gal_img_desc(surface,
				(const image_desc *)pdesc,
				x,
				y,
				((const image_desc *)pdesc)->width,
				((const image_desc *)pdesc)->height));
}

status_t GXGDI_DrawSPP(void *surface, const void *pdesc)
{
	image_desc *img_desc = NULL;

	img_desc = (image_desc *)pdesc;

	hd_img_copy(surface, (char *)img_desc->data, 0, 0, img_desc->width, img_desc->height, img_desc->bpp / 8);

	hd_add_blit_element(NULL);

	return (GXCORE_SUCCESS);
}

int GXGDI_GetImageWidth(const void *pdesc)
{
	image_desc *img_desc = NULL;

	img_desc = (image_desc *)pdesc;

	return (img_desc->width);
}

int GXGDI_GetImageHeight(const void *pdesc)
{
	image_desc *img_desc = NULL;

	img_desc = (image_desc *)pdesc;

	return (img_desc->height);
}

int GXGDI_GetImageBpp(const void *pdesc)
{
	image_desc *img_desc = NULL;

	img_desc = (image_desc *)pdesc;

	return (img_desc->bpp);
}

status_t GXGDI_ClearSPP(void)
{
	hd_img_clear();

	hd_add_blit_element(NULL);

	return (GXCORE_SUCCESS);
}

/*Rectangle*/
status_t GXGDI_FillRect(void *surface, GXGDI_Rect *rect, int color)
{
	int dst_color = 0;
	int ret = 0;

	dst_color = gal_color2index0(surface, color);

	if((32 == ((GuiSurface *)surface)->bpp) && (0xFF000000 & color))
	{
		((GuiSurface *)surface)->alpha = ((color & 0xFF000000) >> 24);
	}
	ret = hd_fillrect(surface, (GAL_Rect *)rect, dst_color);
	if(ret){
		return (GXCORE_ERROR);
	}

	hd_add_blit_element(NULL);

	return (GXCORE_SUCCESS);
}

int GXGDI_FontLoad(const unsigned char *ID, char* pFile, int Size, int Style)
{
	return (gdi_font_load(&(gui.config), (char *)ID, pFile, Size, Style));
}

/*String & Font*/
void *GXGDI_GetFont(void)
{
	return (gxgdi_get_font());
}

void *GXGDI_SetFont(const unsigned char *name)
{
	return (gxgdi_set_font((char *)name));
}

int GXGDI_GetFontHeight(void* pfont)
{
	struct gdi_font *font = (struct gdi_font *)pfont;

	return (font->ysize);
}

void GXGDI_GetFontBearingHeight(void *pfont, int code, int *yoffset, int *height)
{
	return (gxgdi_get_bearingheight((struct gdi_font *)pfont, code, yoffset, height));
}

int gxgdi_font_thrshold_size(GuiConfig* config, char *fontfile, int size)
{
	int height = 0;

	height = gdi_get_font_height_bysize(fontfile, size);
	config->font_threshold_height = height;
	config->font_threshold_size   = size;
	gxgdi_font_infomation_print();

	return (0);
}

int GXGDI_FontThresholdSize(char *fontfile, int size)
{
	GuiCore *gui_core = NULL;

	gui_core = GUI_GetCurrent();
	return (gxgdi_font_thrshold_size(&(gui_core->config), fontfile, size));
}

int GXGDI_FontDestroy(const unsigned char *name)
{
	struct gdi_font *font = NULL;
	GuiCore *gui_core = GUI_GetCurrent();

	if(NULL == name) {
		return (-1);
	}

	font = gdi_font_get_byname(name);

	if(gui_core && font) {
		return (gdi_font_destroy(&(gui_core->config), font));
	}

	return (-1);
}

void *GXGDI_I18N_String(const char *string)
{
	return ((void *)i18n_get_string(string));
}
status_t GXGDI_DrawString_back_color(void *surface, void *string, GXGDI_Rect *rect, int alignment, int color,int back_color, STRING_TYPE flag)
{
	GAL_Interval interval = {0};
	int dst_color = 0;
	int dst_back_color = 0;

	dst_color = gal_color2index0(surface, color);
	dst_back_color = gal_color2index0(surface, back_color);;

	return (gxgdi_draw_string_back_color(surface, string, (GAL_Rect*)rect, &interval, alignment, dst_color,dst_back_color, flag));

}

int GXGDI_DrawString(void *surface, void *string, GXGDI_Rect *rect, int alignment, int color, STRING_TYPE flag)
{
	GAL_Interval interval = {0};
	int dst_color = 0, text_len = 0, font_height = 0;
	GAL_Rect tmp_rect = {0};

	dst_color = gal_color2index0(surface, color);
	tmp_rect.x = rect->x;
	tmp_rect.y = rect->y;
	tmp_rect.w = rect->w;
	tmp_rect.h = rect->h;

	text_len = gdi_text_len(string);
	font_height = GXGDI_GetFontHeight(GXGDI_GetFont());

	if(GDI_STRING_SINGLE == flag) {
		widget_string_alignment(alignment,
					text_len,
					font_height,
					(GUI_Rect *)(&tmp_rect),
					(GUI_Rect *)(&tmp_rect));
	}

	return (gxgdi_draw_string(surface, string, &tmp_rect, &interval, alignment, dst_color, flag));
}

int GXGDI_GetStringLines(void *string, GXGDI_Rect *rect, GDI_Interval *interval)
{
	GAL_Rect str_rect = {0};
	GAL_Interval str_interval = {0};

	if((NULL == string) || (NULL == string) || (NULL == string)) {
		return (0);
	}

	str_rect.x = rect->x;
	str_rect.y = rect->y;
	str_rect.w = rect->w;
	str_rect.h = rect->h;

	str_interval.horizontal = interval->width;
	str_interval.vertical = interval->height;

	return (gdi_text_get_lines(string, &str_rect, &str_interval));
}

int GXGDI_GetStringPixels(void *string)
{
	int len = 0;

	len = gdi_text_len(string);

	return (len);
}

/*Blit*/
status_t GXGDI_Blit(void *src, GXGDI_Rect *src_rect, void *dst, GXGDI_Rect *dst_rect, BLIT_FORMAT flag)
{
	if(GDI_BLIT_STRETCH == flag)
	{
		return (gal_stretch_surface(src, (GAL_Rect *)src_rect, dst, (GAL_Rect *)dst_rect));
	}
	else if(GDI_BLIT_COPY == flag)
	{
		return (gal_copy_surface(src, (GAL_Rect *)src_rect, dst, (GAL_Rect *)dst_rect));
	}
	else if(GDI_BLIT_BLEND == flag) {
		return (hd_stretch_surface1(src, (GAL_Rect *)src_rect, dst, (GAL_Rect *)dst_rect));
	}
	else if(GDI_ZOOM == flag) {
		return (hd_zoom_surface(src, (GAL_Rect *)src_rect, dst, (GAL_Rect *)dst_rect));
	}
	else
	{
		return (GXCORE_SUCCESS);
	}
}

status_t GXGDI_Begin(void)
{
	return (gdi_begin());
}

status_t GXGDI_End(void)
{
	return (gdi_end());
}

status_t GXGDI_Flip(void)
{
	return (gdi_commit());
}

status_t GXGDI_Sync(void)
{
	GXGDI_Flip();
	_copy_double_surface();
	return (0);
}

status_t GXGDI_Font_RegisterOpt(GXGDI_FontOpt *opt)
{
	g_font_opt_fun = opt;

	return (GXCORE_SUCCESS);
}

status_t GXGDI_Font_UnRegisterOpt(GXGDI_FontOpt *opt)
{
	g_font_opt_fun = NULL;
	return (GXCORE_SUCCESS);
}

#define LEVEL_MAX(a, b)               (a) > (b) ? (a) : (b)
#define LEVEL_MIN(a, b)               (a) < (b) ? (a) : (b)

int  GDI_FillGradualRect(GuiSurface *surface,
			GXGDI_Rect * rect,
			unsigned int start_color,
			unsigned int end_color,
			GradualFillType type)
{
#if 1
	int r_start = 0, g_start = 0, b_start = 0, a_start = 0;
	int r_end = 0, g_end = 0, b_end = 0, a_end = 0;
	int r_value = 0, g_value = 0, b_value = 0, a_value = 0;
	int r_offset = 0, g_offset = 0, b_offset = 0, a_offset = 0;
	int level[4] = {0}, color_level = 0;
	int ret = 0, i = 0;
	unsigned int color = 0, element_dist = 0;
	GXGDI_Rect element_rect = {0};

	if((NULL == surface) || (NULL == rect)){
		return (-1);
	}

	a_start = (start_color & 0xFF000000) >> 24;
	r_start = (start_color & 0x00FF0000) >> 16;
	g_start = (start_color & 0x0000FF00) >> 8;
	b_start = start_color & 0x000000FF;

	a_end = (end_color & 0xFF000000) >> 24;
	r_end = (end_color & 0x00FF0000) >> 16;
	g_end = (end_color & 0x0000FF00) >> 8;
	b_end = end_color & 0x000000FF;

	a_offset = a_end - a_start;
	r_offset = r_end - r_start;
	g_offset = g_end - g_start;
	b_offset = b_end - b_start;

	level[0] = r_offset > 0 ? r_offset : (-r_offset);
	level[1] = g_offset > 0 ? g_offset : (-g_offset);
	level[2] = b_offset > 0 ? b_offset : (-b_offset);
	level[3] = a_offset > 0 ? a_offset : (-a_offset);

	color_level = LEVEL_MAX(level[0], level[1]);
	color_level = LEVEL_MAX(color_level, level[2]);
	color_level = LEVEL_MAX(color_level, level[3]);

	if(GRADUAL_FILL_VERTICAL == type){
		color_level = LEVEL_MIN(color_level, rect->h);
	}else{
		color_level = LEVEL_MIN(color_level, rect->w);
	}
	if(color_level == 0){
		color_level = 1;
	}

	hd_new_blit_list();

	element_rect = *rect;
	if(GRADUAL_FILL_VERTICAL == type){
		element_dist = rect->h / color_level;
	}else{
		element_dist = rect->w / color_level;
	}
	for(i = 0; i < color_level; i++){
		if(r_offset > 0)
			r_value = LEVEL_MIN((r_start + r_offset * i / color_level), r_end);
		else
			r_value = LEVEL_MAX((r_start + r_offset * i / color_level), r_end);

		if(g_offset > 0)
			g_value = LEVEL_MIN((g_start + g_offset * i / color_level), g_end);
		else
			g_value = LEVEL_MAX((g_start + g_offset * i / color_level), g_end);

		if(b_offset > 0)
			b_value = LEVEL_MIN((b_start + b_offset * i / color_level), b_end);
		else
			b_value = LEVEL_MAX((b_start + b_offset * i / color_level), b_end);

		if(a_offset > 0)
			a_value = LEVEL_MIN((a_start + a_offset * i / color_level), a_end);
		else
			a_value = LEVEL_MAX((a_start + a_offset * i / color_level), a_end);

		color = ((a_value << 24) & 0xFF000000) |
			((r_value << 16) & 0x00FF0000) |
			((g_value << 8)  & 0x0000FF00) |
			(b_value        & 0x000000FF);
		if(GRADUAL_FILL_VERTICAL == type){
			element_rect.h = element_dist;
		}else{
			element_rect.w = element_dist;
		}

		ret = GXGDI_FillRect((void *)surface, &element_rect, color);
		if(GRADUAL_FILL_VERTICAL == type){
			element_rect.y += element_dist;
		}else{
			element_rect.x += element_dist;
		}
	}

	if(GRADUAL_FILL_VERTICAL == type){
		element_rect.h = rect->y + rect->h - element_rect.y;
		if(element_rect.h)
			ret = GXGDI_FillRect((void *)surface, &element_rect, color);
	}else{
		element_rect.w = rect->x + rect->w - element_rect.x;
		if(element_rect.w)
			ret = GXGDI_FillRect((void *)surface, &element_rect, color);
	}

	hd_end_blit_list();

	return (ret);
#else
	GuiSurface *tmp_surface = NULL;
	GAL_Rect src_rect = {0}, dst_rect = {0};
	GXGDI_Rect fill_rect = {0};
	int ret = 0;

	src_rect.x = src_rect.y = 0;
	src_rect.w = rect->w;
	src_rect.h = rect->h;
	if(GRADUAL_FILL_VERTICAL == type){
		src_rect.h = 2;
	}else{
		src_rect.w = 2;
	}
	tmp_surface = hd_get_surface(&src_rect, surface->bpp, NULL, GUI_TRUE);
	if(NULL == tmp_surface){
		return (-1);
	}

	if(GRADUAL_FILL_VERTICAL == type){
		fill_rect.x = src_rect.x;
		fill_rect.y = src_rect.y;
		fill_rect.w = src_rect.w;
		fill_rect.h = 1;
		ret = GXGDI_FillRect(tmp_surface, &fill_rect, start_color);
		fill_rect.y += 1;
		ret = GXGDI_FillRect(tmp_surface, &fill_rect, end_color);
	}else{
		fill_rect.x = src_rect.x;
		fill_rect.y = src_rect.y;
		fill_rect.w = 1;
		fill_rect.h = src_rect.h;
		ret = GXGDI_FillRect(tmp_surface, &fill_rect, start_color);
		fill_rect.x += 1;
		ret = GXGDI_FillRect(tmp_surface, &fill_rect, end_color);
	}

	dst_rect.x = rect->x;
	dst_rect.y = rect->y;
	dst_rect.w = rect->w;
	dst_rect.h = rect->h;
	hd_zoom_surface(tmp_surface, &src_rect, surface, &dst_rect);
	hd_add_blit_element(tmp_surface);
	return (ret);
#endif
}

static handle_t layer_mutex_id = 0;

void layer_lock(void)
{
	if(0 == layer_mutex_id)
	{
		GxCore_MutexCreate(&layer_mutex_id);
		if(0 == layer_mutex_id)
		{
			return;
		}
	}

	GxCore_MutexLock(layer_mutex_id);
}

void layer_unlock(void)
{
	if(0 == layer_mutex_id)
	{
		GxCore_MutexCreate(&layer_mutex_id);
		if(0 == layer_mutex_id)
		{
			return;
		}
	}

	GxCore_MutexUnlock(layer_mutex_id);
}

static gx_stack_t* _get_layer_stack(void)
{
	if(NULL == gui.layer_stack)
	{
		gui.layer_stack = stack_new(NULL);
	}

	return (gui.layer_stack);
}

static void _free_layer(void *data)
{
	VirtualLayer *layer = NULL;
	GuiSurface *surface = NULL;

	layer = (VirtualLayer *)data;
	if(NULL == layer)
		return;

	surface = layer->surface;
	if(NULL == surface)
	{
		GUI_FREE(layer);
		return;
	}

	GUI_FREE(layer->name);
	// app new surface, app free surface for 89627
	//hd_free_surface(surface);
	GUI_FREE(layer);
}

static hash_t* _get_layer_table(void)
{
	if(NULL == gui.layer_table)
	{
		gui.layer_table = hash_new(10, _free_layer);
	}

	return (gui.layer_table);
}

static unsigned int s_layer_index;
/*Virtual Layer*/
char *GxGDI_LayerRegister(GuiSurface *surface, unsigned int x, unsigned int y)
{
	char *name = NULL;
	VirtualLayer *layer = NULL;
	gx_stack_t *stack = NULL;
	hash_t *table = NULL;
	GuiSurface *tmp_surface = NULL;
	GAL_Rect rect = {0};

	if(NULL == surface)
	{
		return (NULL);
	}

	if(((x + surface->sf.width) > gui.config.width) ||
	   ((y + surface->sf.height) > gui.config.height)){
		gxlogd("Register surface to virtual layer is out of bound!\n");
		return (NULL);
	}
	else if((x > gui.config.width) || (y > gui.config.height)){
		return (NULL);
	}

	name = (char *)GUI_MALLOCZ(strlen("Virtual Layer") + 10);
	if(NULL == name)
	{
		return (NULL);
	}

	layer_lock();
	snprintf(name, strlen("Virtual Layer") + 10, "Virtual Layer %d", s_layer_index);
	s_layer_index++;
	layer = (VirtualLayer *)GUI_MALLOCZ(sizeof(VirtualLayer));
	if(NULL == layer)
	{
		GUI_FREE(name);
		layer_unlock();
		return (NULL);
	}

	stack = _get_layer_stack();
	if(NULL == stack)
	{
		GUI_FREE(name);
		GUI_FREE(layer);
		layer_unlock();
		return (NULL);
	}

	table = _get_layer_table();
	if(NULL == table)
	{
		GUI_FREE(name);
		GUI_FREE(layer);
		layer_unlock();
		return (NULL);
	}

	layer->x = x;
	layer->y = y;
	layer->name = GUI_STRDUP(name);
	layer->surface = surface;
	layer->enable = GUI_TRUE;
	stack_push(stack, layer);
	hash_add(table, name, layer);
	layer_unlock();

	gdi_lock();
	layer_lock();
	gui.layer_mode = GUI_HAS_VIRTUAL_LAYER;
	if(1 == stack_count(stack))
	{
		rect.x = rect.y = 0;
		rect.w = gui.config.width;
		rect.h = gui.config.height;
		tmp_surface = hd_get_idle_surface(screen);
		if(tmp_surface != screen)
		{
			tmp_surface = screen;
			screen = back_screen;
			back_screen = tmp_surface;
		}
		gal_copy_surface(back_screen, &rect, screen, &rect);
	}
	layer_unlock();
	gdi_unlock();
	GUI_FREE(name);

	return (layer->name);
}

char *GxGDI_LayerRegisterNolock(GuiSurface *surface, unsigned int x, unsigned int y)
{
	char *name = NULL;
	VirtualLayer *layer = NULL;
	gx_stack_t *stack = NULL;
	hash_t *table = NULL;
	GuiSurface *tmp_surface = NULL;
	GAL_Rect rect = {0};
	LayerMode prev_mode = gui.layer_mode;

	if(NULL == surface)
	{
		return (NULL);
	}

	if(((x + surface->sf.width) > gui.config.width) ||
	   ((y + surface->sf.height) > gui.config.height)){
		gxlogd("Register surface to virtual layer is out of bound!\n");
		return (NULL);
	}
	else if((x > gui.config.width) || (y > gui.config.height)){
		return (NULL);
	}

	name = (char *)GUI_MALLOCZ(strlen("Virtual Layer") + 10);
	if(NULL == name)
	{
		return (NULL);
	}

	layer_lock();
	snprintf(name, strlen("Virtual Layer") + 10, "Virtual Layer %d", s_layer_index);
	s_layer_index++;
	layer = (VirtualLayer *)GUI_MALLOCZ(sizeof(VirtualLayer));
	if(NULL == layer)
	{
		GUI_FREE(name);
		layer_unlock();
		return (NULL);
	}

	stack = _get_layer_stack();
	if(NULL == stack)
	{
		GUI_FREE(name);
		GUI_FREE(layer);
		layer_unlock();
		return (NULL);
	}

	table = _get_layer_table();
	if(NULL == table)
	{
		GUI_FREE(name);
		GUI_FREE(layer);
		layer_unlock();
		return (NULL);
	}

	layer->x = x;
	layer->y = y;
	layer->name = GUI_STRDUP(name);
	layer->surface = surface;
	layer->enable = GUI_TRUE;
	stack_push(stack, layer);
	hash_add(table, name, layer);
	layer_unlock();

	if(prev_mode != GUI_HAS_VIRTUAL_LAYER) {
		gui_delay(30);
	}
	gui.layer_mode = GUI_HAS_VIRTUAL_LAYER;
	if(1 == stack_count(stack))
	{
		rect.x = rect.y = 0;
		rect.w = gui.config.width;
		rect.h = gui.config.height;
		tmp_surface = hd_get_idle_surface(screen);
		if(tmp_surface != screen)
		{
			tmp_surface = screen;
			screen = back_screen;
			back_screen = tmp_surface;
		}
		gal_copy_surface(back_screen, &rect, screen, &rect);
	}
	GUI_FREE(name);

	return (layer->name);
}

void gxgdi_updatelayers(GuiSurface *swap_surface)
{
	VirtualLayer *layer = NULL;
	GuiSurface *surface = NULL;
	gx_stack_t *stack = NULL;
	int i = 0, count = 0;
	GAL_Rect src_rect = {0}, dst_rect = {0};

	layer_lock();
	stack = _get_layer_stack();
	if(NULL == stack)
	{
		layer_unlock();
		return;
	}

	count = stack_count(stack);
	for(i = 0; i < count; i++)
	{
		layer = stack_get(stack, i);
		if((NULL == layer) || (GUI_FALSE == layer->enable))
		{
			continue;
		}
		surface = layer->surface;
		if(NULL == surface)
		{
			continue;
		}

		src_rect.x = src_rect.y = 0;
		src_rect.w = surface->sf.width;
		src_rect.h = surface->sf.height;

		if((swap_surface->sf.width >= surface->sf.width) &&
		   (swap_surface->sf.height >= surface->sf.height))
		{
			dst_rect.w = src_rect.w;
			dst_rect.h = src_rect.h;
		}
		else
		{
			dst_rect.w = swap_surface->sf.width;
			dst_rect.h = swap_surface->sf.height;
		}

		if(layer->x >= 0) {
			dst_rect.x = layer->x;
		} else {
			src_rect.x -= layer->x;
			src_rect.w += layer->x;
			dst_rect.w += layer->x;
		}

		if(layer->y >= 0) {
			dst_rect.y = layer->y;
		} else {
			src_rect.y -= layer->y;
			src_rect.h += layer->y;
			dst_rect.h += layer->y;
		}

		if((dst_rect.x + dst_rect.w) > swap_surface->sf.width) {
			src_rect.w = dst_rect.w = swap_surface->sf.width - dst_rect.x;
		}

		if((dst_rect.y + dst_rect.h) > gui.config.height) {
			src_rect.h = dst_rect.h = swap_surface->sf.height - dst_rect.y;
		}
		
		hd_blit_surface(surface, &src_rect, swap_surface, &dst_rect);
		hd_add_blit_element(NULL);
	}
	gui.layer_need_update = GUI_FALSE;
	layer_unlock();
}

status_t GxGDI_LayerUnregister(const char *name)
{
	VirtualLayer *layer = NULL;
	gx_stack_t *stack = NULL;
	hash_t *table = NULL;
	GAL_Rect rect = {0};
	int i = 0, count= 0;

	if(NULL == name)
	{
		return (GXCORE_ERROR);
	}

	layer_lock();
	stack = _get_layer_stack();
	if(NULL == stack)
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}

	table = _get_layer_table();
	if(NULL == table)
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}

	layer = hash_get(table, name);
	if(NULL == layer)
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}

	count = stack_count(stack);
	if(0 == count)
	{
		gdi_lock();
		gui.layer_mode = GUI_NO_VIRTUAL_LAYER;
		gui.layer_need_update = GUI_FALSE;
		rect.x = rect.y = 0;
		rect.w = gui.config.width;
		rect.h = gui.config.height;
		if(screen == hd_get_idle_surface(screen)) {
			hd_new_blit_list();
			hd_copy_surface1(screen, &rect, back_screen, &rect);
			hd_end_blit_list();
		}
		gdi_unlock();
	}else{
		gui.layer_need_update = GUI_TRUE;
	}

	for(i = 0; i < count; i++)
	{
		if(layer == stack_get(stack, i))
		{
			stack_del(stack, i);
			break;
		}
	}
	count = stack_count(stack);
	hash_drop(table, name);
	layer_unlock();

	if(0 == count)
	{
		gdi_lock();
		GxGDI_LayerUpdate(name);
		gui.layer_mode = GUI_NO_VIRTUAL_LAYER;
		rect.x = rect.y = 0;
		rect.w = gui.config.width;
		rect.h = gui.config.height;
		if(screen == hd_get_idle_surface(screen)) {
			hd_new_blit_list();
			hd_copy_surface1(screen, &rect, back_screen, &rect);
			hd_end_blit_list();
		}
		gdi_unlock();
	}

	return (GXCORE_SUCCESS);
}

status_t GxGDI_LayerUnregisterNolock(const char *name)
{
	VirtualLayer *layer = NULL;
	gx_stack_t *stack = NULL;
	hash_t *table = NULL;
	GAL_Rect rect = {0};
	int i = 0, count= 0;

	if(NULL == name)
	{
		return (GXCORE_ERROR);
	}

	layer_lock();
	stack = _get_layer_stack();
	if(NULL == stack)
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}

	table = _get_layer_table();
	if(NULL == table)
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}

	layer = hash_get(table, name);
	if(NULL == layer)
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}

	count = stack_count(stack);
	if(0 == count)
	{
		gui.layer_mode = GUI_NO_VIRTUAL_LAYER;
		gui.layer_need_update = GUI_FALSE;
		rect.x = rect.y = 0;
		rect.w = gui.config.width;
		rect.h = gui.config.height;
		if(screen == hd_get_idle_surface(screen)) {
			hd_new_blit_list();
			hd_copy_surface1(screen, &rect, back_screen, &rect);
			hd_end_blit_list();
		}
	}else{
		gui.layer_need_update = GUI_TRUE;
	}

	for(i = 0; i < count; i++)
	{
		if(layer == stack_get(stack, i))
		{
			stack_del(stack, i);
			break;
		}
	}
	count = stack_count(stack);
	hash_drop(table, name);
	layer_unlock();

	if(0 == count)
	{
		GxGDI_LayerUpdate(name);
		gui.layer_mode = GUI_NO_VIRTUAL_LAYER;
		rect.x = rect.y = 0;
		rect.w = gui.config.width;
		rect.h = gui.config.height;
		if(screen == hd_get_idle_surface(screen)) {
			hd_new_blit_list();
			hd_copy_surface1(screen, &rect, back_screen, &rect);
			hd_end_blit_list();
		}
	}

	return (GXCORE_SUCCESS);
}

status_t GxGDI_LayerUpdate(const char *name)
{
	hash_t *table = NULL;
	VirtualLayer *layer = NULL;

	layer_lock();
	if(gui.layer_mode == GUI_NO_VIRTUAL_LAYER)
	{
		gui.layer_need_update = GUI_FALSE;
		layer_unlock();
		return (GXCORE_ERROR);
	}

	table = _get_layer_table();
	if(NULL == table)
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}

	layer = hash_get(table, name);
	if(NULL == layer)
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}

	//gdi_lock();
	gui.layer_need_update = GUI_TRUE;
	//gdi_unlock();
	layer_unlock();

	return (GXCORE_SUCCESS);
}

status_t GxGDI_LayerUpdateSync(void)
{

	layer_lock();
	gui.layer_need_update = GUI_TRUE;
	layer_unlock();

	gdi_wait();
	return (GXCORE_SUCCESS);
}

status_t GXGDI_LayerClear(const char *name, GXGDI_Rect *rect)
{
	GuiSurface *surface = NULL;
	int color = 0;
	GXGDI_Rect tmp_rect = {0};

	if(NULL == name)
	{
		return (GXCORE_ERROR);
	}

	surface = GxGDI_LayerGetSurface(name);
	if(NULL == surface)
	{
		return (GXCORE_ERROR);
	}

	if(NULL == rect)
	{
		tmp_rect.x = tmp_rect.y = 0;
		tmp_rect.w = surface->sf.width;
		tmp_rect.h = surface->sf.height;
	}
	else
	{
		tmp_rect = *rect;
	}
	color = gui.config.gui_trans;
	return (GXGDI_FillRect(surface, &tmp_rect, color));
}

GuiSurface *GxGDI_LayerGetSurface(const char *name)
{
	hash_t *table = NULL;
	VirtualLayer *layer = NULL;

	layer_lock();
	table = _get_layer_table();
	if(NULL == table)
	{
		layer_unlock();
		return (NULL);
	}

	layer = hash_get(table, name);
	if(NULL == layer)
	{
		layer_unlock();
		return (NULL);
	}

	layer_unlock();

	return (layer->surface);
}

status_t check_layer_enable(void)
{
	gx_stack_t *stack = NULL;
	VirtualLayer *layer = NULL;
	uint32_t count = 0, i = 0;

	stack = _get_layer_stack();
	if(NULL == stack)
	{
		return (GXCORE_ERROR);
	}
	count = stack_count(stack);
	for(i = 0; i < count; i++)
	{
		layer = stack_get(stack, i);
		if((layer) && (GUI_TRUE == layer->enable))
		{
			return (GXCORE_SUCCESS);
		}
	}
	return (GXCORE_ERROR);
}

status_t GxGDI_LayerEnable(const char *name, bool flag)
{
	hash_t *table = NULL;
	VirtualLayer *layer = NULL;
	int has_layer = 0;
	const char *focus_window = NULL;

	layer_lock();

	has_layer = check_layer_enable();

	table = _get_layer_table();
	if(NULL == table)
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}

	layer = hash_get(table, name);
	if(NULL == layer)
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}

	layer->enable = flag;
	if(flag){
		gui.layer_mode = GUI_HAS_VIRTUAL_LAYER;
	}

	if((has_layer == check_layer_enable()) &&
	   ((GxCore_ThreadGetId() == gui.console_id) ||
	    (GxCore_ThreadGetId() == gui.rcv_msg_id))) {
		focus_window = GUI_GetFocusWindow();
		GUI_SetProperty(focus_window, "update", NULL);
		has_layer = check_layer_enable();
	}

	if(GXCORE_ERROR == has_layer) {
		gui.layer_mode = GUI_NO_VIRTUAL_LAYER;
	}

	layer_unlock();
	GxGDI_LayerUpdate(name);

	return (GXCORE_SUCCESS);
}

void GxGDI_LayerInfomation(void)
{
	gx_stack_t *stack = NULL;
	VirtualLayer *layer = NULL;
	int count = 0, i = 0;
	char value[30] = {0};

	layer_lock();
	stack = _get_layer_stack();
	if(NULL == stack)
	{
		layer_unlock();
		return;
	}
	gxlogd("================================================================================================================================================\n");
	count = stack_count(stack);
	for(i = 0; i < count; i++)
	{
		layer = stack_get(stack, i);
		if((NULL == layer) || (NULL == layer->surface))
		{
			continue;
		}
		if(layer->enable)
		{
			strcpy(value, "Enabled");
		}
		else
		{
			strcpy(value, "Disabled");
		}
		gxlogd("*	Name = %s	*	x = %d, y = %d, w = %d, h = %d	*	Surface = 0x%x	*	%s	*\n",
				layer->name, layer->x, layer->y, layer->surface->sf.width, layer->surface->sf.height,
				(int)layer->surface, value);

	}
	gxlogd("================================================================================================================================================\n");
	layer_unlock();
}

status_t GxGDI_LayerSetPosition(const char *name, int32_t x, int32_t y)
{
	hash_t *table = NULL;
	VirtualLayer *layer = NULL;
	GuiSurface *surface = NULL;

	layer_lock();
	table = _get_layer_table();
	if(NULL == table)
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}

	layer = hash_get(table, name);
	if(NULL == layer)
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}

	surface = layer->surface;
	if(NULL == surface)
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}
#if 0
	if(((x + surface->sf.width) > gui.config.width) ||
	   ((y + surface->sf.height) > gui.config.height))
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}
	else if((x > gui.config.width) || (y > gui.config.height)){
		layer_unlock();
		return (GXCORE_ERROR);
	}
#endif

	layer->x = x;
	layer->y = y;

	layer_unlock();
	GxGDI_LayerUpdate(name);

	return (GXCORE_SUCCESS);
}

int32_t GXGDI_GetLayerPriority(const char *name)
{
	hash_t *table = NULL;
	VirtualLayer *layer = NULL;
	gx_stack_t *stack = NULL;
	uint32_t i = 0, count = 0;

	if(NULL == name)
	{
		return (-1);
	}
	layer_lock();
	stack = _get_layer_stack();
	if(NULL == stack)
	{
		layer_unlock();
		return (-1);
	}

	table = _get_layer_table();
	if(NULL == table)
	{
		layer_unlock();
		return (-1);
	}

	layer = hash_get(table, name);
	if(NULL == layer)
	{
		layer_unlock();
		return (-1);
	}

	count = stack_count(stack);
	for(i = 0; i < count; i++)
	{
		layer = stack_get(stack, i);
		if((NULL == layer) || (NULL == layer->surface))
		{
			continue;
		}

		if(0 == strcasecmp(name, layer->name))
		{
			layer_unlock();
			return (i);
		}
	}
	layer_unlock();
	return (-1);
}

status_t GXGDI_SetLayerPriority(const char *name, uint32_t priority)
{
	hash_t *table = NULL;
	VirtualLayer *layer = NULL, *tmp_layer = NULL;
	gx_stack_t *stack = NULL, *tmp_stack = NULL;
	uint32_t i = 0, count = 0;

	if(NULL == name)
	{
		return (GXCORE_ERROR);
	}
	layer_lock();
	stack = _get_layer_stack();
	if(NULL == stack)
	{
		layer_unlock();
		return GXCORE_ERROR;
	}

	table = _get_layer_table();
	if(NULL == table)
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}

	tmp_layer = layer = hash_get(table, name);
	if(NULL == layer)
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}

	count = stack_count(stack);
	if(priority >= count)
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}

	tmp_stack = stack_new(NULL);
	if(NULL == tmp_stack)
	{
		layer_unlock();
		return (GXCORE_ERROR);
	}

	for(i = 0; i < count; i++)
	{
		if((count - priority - 1) == i)
		{
			stack_push(tmp_stack, tmp_layer);
		}

		layer = stack_pop(stack);
		if((NULL == layer) && (stack_count(stack)))
		{
			count = stack_count(tmp_stack);
			for(i = 0; i < count; i++)
			{
				layer = stack_pop(tmp_stack);//stack_get(tmp_stack, i);
				stack_push(stack, layer);
			}
			stack_release(tmp_stack);
			layer_unlock();
			return (GXCORE_ERROR);
		}

		if(layer){
			if(layer != tmp_layer)
			{
				stack_push(tmp_stack, layer);
			}
			else{
				layer = stack_pop(stack);
				if(NULL != layer)
				{
					stack_push(tmp_stack, layer);
				}
			}
		}
	}

	for(i = 0; i < count; i++)
	{
		layer = stack_pop(tmp_stack);//stack_get(tmp_stack, i);
		stack_push(stack, layer);
	}
	stack_release(tmp_stack);

	layer_unlock();
	GxGDI_LayerUpdate(name);

	return (GXCORE_SUCCESS);
}

typedef struct _GxRollBlock
{
	GuiSurface *dst_surface;
	GuiSurface *src_surface;
	char *font;
	GAL_Rect rect;
	uint32_t fore_color;
	uint32_t back_color;
	uint32_t delay;
	uint32_t pos;
	uint32_t step;
	handle_t handle;
	handle_t roll_mutex;
	bool exit;
	bool pause;
	int times;
	ROLL_DIRECT direct;
}GxRollBlock;

static hash_t *roll_table;

static void _free_roll_block(void *data)
{
	GxRollBlock *block = NULL;

	block = (GxRollBlock *)data;
	if(NULL == block)
	{
		return;
	}

	GxCore_MutexDelete(block->roll_mutex);
	GUI_FREE(block);
}

static hash_t *_get_roll_table(void)
{
	if(NULL == roll_table)
	{
		roll_table = hash_new(40, _free_roll_block);
	}

	return (roll_table);
}

static int rolling_count;

static void _rolling_fun(void *data)
{
	GxRollBlock *block = (GxRollBlock *)data;
	hash_t *table = NULL;
	char value[100] = {0};
	unsigned int pos = 0, bound = 0;
	int color = 0;
	GAL_Rect src_rect = {0}, dst_rect = {0};

	if(NULL == block)
		return;

	GxCore_ThreadDetach();
	bound = (block->src_surface->sf.width > block->dst_surface->sf.width) ?
			block->src_surface->sf.width : block->dst_surface->sf.width;
	while(GUI_FALSE == block->exit)
	{
		GxCore_MutexLock(block->roll_mutex);
		gdi_lock();
		if(pos <= block->dst_surface->sf.width)
		{
			if(GDI_ROLL_RIGHT_LEFT == block->direct)
			{
				src_rect.x = src_rect.y = 0;
				src_rect.w = (pos < block->src_surface->sf.width) ? pos : block->src_surface->sf.width;
				src_rect.h = block->rect.h;
				dst_rect.x = block->rect.x + block->rect.w - pos;
				dst_rect.w = pos;
			}
			else if(GDI_ROLL_LEFT_RIGHT == block->direct)
			{
				if(block->src_surface->sf.width > pos)
				{
					src_rect.x = block->src_surface->sf.width - pos;
					dst_rect.x = 0;
					src_rect.w = pos;
					dst_rect.w = pos;
				}
				else
				{
					src_rect.x = 0;
					src_rect.w = block->src_surface->sf.width;
					dst_rect.x = pos - block->src_surface->sf.width;
					dst_rect.w = src_rect.w;
				}
				src_rect.h = block->rect.h;
			}

			dst_rect.y = block->rect.y;
			src_rect.h = dst_rect.h = block->rect.h;
		}
		else
		{
			if(GDI_ROLL_RIGHT_LEFT == block->direct)
			{
				src_rect.x = pos - block->rect.w;
				src_rect.y = 0;
				dst_rect.x = 0;
				if((src_rect.x + block->rect.w) < block->src_surface->sf.width)
					src_rect.w = block->rect.w;
				else
					src_rect.w = block->src_surface->sf.width - src_rect.x;
				src_rect.h = block->rect.h;
			}
			else if(GDI_ROLL_LEFT_RIGHT == block->direct)
			{
				if(block->src_surface->sf.width >= pos)
				{
					src_rect.x = block->src_surface->sf.width - pos;
					src_rect.w = block->rect.w;
					dst_rect.x = 0;
				}
				else
				{
					src_rect.x = 0;
					src_rect.w = block->rect.w - (pos - block->src_surface->sf.width);
					dst_rect.x = pos- block->src_surface->sf.width;;
				}
				src_rect.h = block->rect.h;
			}

			dst_rect.y = 0;
			dst_rect.w = src_rect.w;
			dst_rect.h = src_rect.h;
		}
		if(GUI_FALSE == block->pause)
		{
			gdi_begin();
			color = hd_color2index0(block->dst_surface, block->back_color);
			hd_fillrect(block->dst_surface, &(block->rect), color);
			hd_stretch_surface(block->src_surface,
								&src_rect,
								block->dst_surface,
								&dst_rect);
			gdi_end();
			pos += block->step;
		}
		if(pos > (block->src_surface->sf.width + block->dst_surface->sf.width))
		{
			gdi_begin();
			color = hd_color2index0(block->dst_surface, block->back_color);
			hd_fillrect(block->dst_surface, &(block->rect), color);
			gdi_end();
			pos = 0;
			block->times++;
		}
		block->pos = pos;
		if((gui.layer_stack) && (stack_count(gui.layer_stack)))
			gui.layer_need_update = GUI_TRUE;
		gdi_unlock();
		GxCore_MutexUnlock(block->roll_mutex);
		// For smooth rolling.
		if(rolling_count <= 1)
			wm_copy_main_surface();
		GxCore_ThreadDelay(block->delay);
	}

	gdi_begin();
	color = hd_color2index0(block->dst_surface, block->back_color);
	hd_fillrect(block->dst_surface, &(block->rect), color);
	gdi_end();

	GxCore_MutexLock(block->roll_mutex);
	snprintf(value, sizeof(value), "%d", block->handle);
	table = _get_roll_table();
	if(NULL == table)
	{
		hd_free_surface(block->src_surface);
		block->src_surface = NULL;
		GxCore_MutexUnlock(block->roll_mutex);
		return;
	}
	hd_free_surface(block->src_surface);
	block->src_surface = NULL;

	GxCore_MutexUnlock(block->roll_mutex);
}

handle_t GXGDI_StartRollString(void *surface,
							   const char *font,
							   const char *string,
							   GXGDI_Rect *rect,
							   uint32_t fore_color,
							   uint32_t back_color,
							   uint32_t delay,
							   ROLL_DIRECT direct)
{
	hash_t *table = NULL;
	GxRollBlock *block = NULL;
	char value[100] = {0};
	int color = 0;
	GAL_Rect valid_rect = {0};
	GAL_Interval interval = {0};
	struct gdi_font* old_font = NULL;

	if((NULL == surface) ||
	   (NULL == font) ||
	   (NULL == rect))
	{
		return (0);
	}

	table = _get_roll_table();
	if(NULL == table)
	{
		return (0);
	}

	block = GUI_MALLOCZ(sizeof(GxRollBlock));
	if(NULL == block)
	{
		return (0);
	}

	block->dst_surface = (GuiSurface *)surface;
	block->font = (char *)font;
	block->rect.x = rect->x;
	block->rect.y = rect->y;
	block->rect.w = rect->w;
	block->rect.h = rect->h;
	block->fore_color = fore_color;
	block->back_color = back_color;
	block->delay = delay;
	block->step = 2;

	if(GDI_ROLL_AUTO == direct)
	{
		if(gxgdi_is_right2left((const unsigned char *)string))
		{
			block->direct = GDI_ROLL_LEFT_RIGHT;
		}
		else
		{
			block->direct = GDI_ROLL_RIGHT_LEFT;
		}
	}
	else
	{
		block->direct = direct;
	}

	if(0 == block->roll_mutex)
	{
		GxCore_MutexCreate(&(block->roll_mutex));
		if(0 == block->roll_mutex)
		{
			return (0);
		}
	}
	block->exit = GUI_FALSE;

	GxCore_MutexLock(block->roll_mutex);

	valid_rect.x = valid_rect.y = 0;
	old_font = gxgdi_set_font((char *)font);
	gdi_lock();
	valid_rect.w = gdi_text_surfacewidth((char *)string);
	gdi_unlock();
	valid_rect.h = block->rect.h;
	gxgdi_set_font((char *)old_font);
	block->src_surface = hd_get_surface(&valid_rect, gui.config.bpp, NULL, GUI_FALSE);
	if(NULL == block->src_surface)
	{
		GxCore_MutexUnlock(block->roll_mutex);
		return (0);
	}

	gdi_begin();
	color = hd_color2index0(block->dst_surface, block->back_color);
	hd_fillrect(block->dst_surface, &(block->rect), color);

	color = hd_color2index0(block->src_surface, block->back_color);
	gdi_lock();
	hd_fillrect(block->src_surface, &(valid_rect), color);
	gdi_unlock();
	gdi_end();
	gdi_begin();

	color = hd_color2index0(block->src_surface, block->fore_color);
	old_font = gxgdi_set_font((char *)font);
	gdi_lock();
	gxgdi_draw_string(block->src_surface,
					  (char *)string,
					  &valid_rect,
					  &interval,
					  GUI_TA_LEFT | GUI_TA_VCENTRE,
					  color,
					  SINGLELINE_MODE);
	gxgdi_set_font((char *)old_font);
	gdi_unlock();
	gdi_end();
	GxCore_ThreadCreate("roll_thread",
						&(block->handle),
						_rolling_fun,
						block,
						16 * 1024,
						GXOS_DEFAULT_PRIORITY - 2);
	if(0 == block->handle)
	{
		GxCore_MutexUnlock(block->roll_mutex);
		return (0);
	}
	snprintf(value, sizeof(value), "%d", block->handle);
	hash_add(table, value, block);

	rolling_count++;
	GxCore_MutexUnlock(block->roll_mutex);
	return (block->handle);
}

status_t GXGDI_SetRollStep(handle_t handle, uint32_t step)
{
	hash_t *table = NULL;
	GxRollBlock *block = NULL;
	char value[100] = {0};

	if(0 == handle)
	{
		return (GXCORE_ERROR);
	}

	table = _get_roll_table();
	if(NULL == table)
	{
		return (GXCORE_ERROR);
	}

	snprintf(value, sizeof(value), "%d", handle);
	block = hash_get(table, value);
	if(NULL == block)
	{
		return (GXCORE_ERROR);
	}
	GxCore_MutexLock(block->roll_mutex);
	block->step = step;
	GxCore_MutexUnlock(block->roll_mutex);

	return (GXCORE_SUCCESS);
}

status_t GXGDI_StopRollString(handle_t handle)
{
	hash_t *table = NULL;
	GxRollBlock *block = NULL;
	char value[100] = {0};

	if(0 == handle)
	{
		return (GXCORE_ERROR);
	}

	table = _get_roll_table();
	if(NULL == table)
	{
		return (GXCORE_ERROR);
	}

	snprintf(value, sizeof(value), "%d", handle);
	block = hash_get(table, value);
	if(NULL == block)
	{
		return (GXCORE_ERROR);
	}
	GxCore_MutexLock(block->roll_mutex);
	block->exit = GUI_TRUE;
	GxCore_MutexUnlock(block->roll_mutex);

	while(block->src_surface) {
		GxCore_ThreadDelay(block->delay);
	}

	GxCore_MutexLock(block->roll_mutex);
	rolling_count--;
	GxCore_MutexUnlock(block->roll_mutex);
	hash_drop(table, value);
	return (GXCORE_SUCCESS);
}

status_t GXGDI_PauseRollString(handle_t handle)
{
	hash_t *table = NULL;
	GxRollBlock *block = NULL;
	char value[100] = {0};

	if(0 == handle)
	{
		return (GXCORE_ERROR);
	}

	table = _get_roll_table();
	if(NULL == table)
	{
		return (GXCORE_ERROR);
	}

	snprintf(value, sizeof(value), "%d", handle);
	block = hash_get(table, value);
	if(NULL == block)
	{
		return (GXCORE_ERROR);
	}
	GxCore_MutexLock(block->roll_mutex);
	block->pause = GUI_TRUE;
	rolling_count--;
	GxCore_MutexUnlock(block->roll_mutex);

	return (GXCORE_SUCCESS);
}

status_t GXGDI_ResetRollString(handle_t handle)
{
	hash_t *table = NULL;
	GxRollBlock *block = NULL;
	char value[100] = {0};

	if(0 == handle)
	{
		return (GXCORE_ERROR);
	}

	table = _get_roll_table();
	if(NULL == table)
	{
		return (GXCORE_ERROR);
	}

	snprintf(value, sizeof(value), "%d", handle);
	block = hash_get(table, value);
	if(NULL == block)
	{
		return (GXCORE_ERROR);
	}
	GxCore_MutexLock(block->roll_mutex);
	block->pause = GUI_FALSE;
	rolling_count++;
	GxCore_MutexUnlock(block->roll_mutex);

	return (GXCORE_SUCCESS);
}

uint32_t GXGDI_GetRollStringPos(handle_t handle)
{
	hash_t *table = NULL;
	GxRollBlock *block = NULL;
	char value[100] = {0};
	uint32_t pos = 0;

	if(0 == handle)
	{
		return (0);
	}

	table = _get_roll_table();
	if(NULL == table)
	{
		return (0);
	}

	snprintf(value, sizeof(value), "%d", handle);
	block = hash_get(table, value);
	if(NULL == block)
	{
		return (0);
	}
	GxCore_MutexLock(block->roll_mutex);
	pos = block->pos;
	GxCore_MutexUnlock(block->roll_mutex);

	return (pos);
}

uint32_t GXGDI_GetRollTimes(handle_t handle)
{
	hash_t *table = NULL;
	GxRollBlock *block = NULL;
	char value[100] = {0};
	uint32_t times = 0;

	if(0 == handle)
	{
		return (0);
	}

	table = _get_roll_table();
	if(NULL == table)
	{
		return (0);
	}

	snprintf(value, sizeof(value), "%d", handle);
	block = hash_get(table, value);
	if(NULL == block)
	{
		return (0);
	}
	GxCore_MutexLock(block->roll_mutex);
	times = block->times;
	GxCore_MutexUnlock(block->roll_mutex);

	return (times);
}

static char *hw_layer_name[] = {
	"osd",
	"vpp",
	"spp",
	"sosd",
	"sosd2"
};

int GxGDI_HardwareLayerRegister(GxLayerID layer, GuiSurface *surface)
{
	int ret = 0;
	hash_t *table = NULL;
	VirtualLayer *layer_node = NULL;

	if(GXAV_ID_SIRIUS != gui.config.chip_id) {
		return (-1);
	}

	ret = GxAvdev_LayerEnable(g_device_handle, vpu_handle, layer, GUI_FALSE);
	if(0 != ret) {
		return (ret);
	}

	ret |= GxAvdev_SetLayerMainSurface(g_device_handle, vpu_handle, layer, surface->hw_surface);
	if(0 != ret) {
		return (ret);
	}

	layer_lock();

	layer_node = (VirtualLayer *)GUI_MALLOCZ(sizeof(VirtualLayer));
	if(NULL == layer_node) {
		layer_unlock();
		return (-1);
	}

	table = _get_layer_table();
	if(NULL == table)
	{
		layer_unlock();
		return (-1);
	}

	layer_node->x = 0;
	layer_node->y = 0;
	layer_node->name = GUI_STRDUP(hw_layer_name[layer]);
	layer_node->surface = surface;
	layer_node->enable = GUI_TRUE;
	hash_add(table, hw_layer_name[layer], layer_node);

	layer_unlock();

	ret = GxAvdev_LayerEnable(g_device_handle, vpu_handle, layer, GUI_TRUE);
	if(0 != ret) {
		return (ret);
	}

	return (ret);
}

GuiSurface *GxGDI_HardwareLayerGet(GxLayerID layer)
{
	GuiSurface *surface = NULL;
	void *hw_surface = NULL;
	hash_t *table = NULL;
	VirtualLayer *layer_node = NULL;

	if(GXAV_ID_SIRIUS != gui.config.chip_id) {
		return (NULL);
	}

	hw_surface = GxAvdev_GetLayerMainSurface(g_device_handle, vpu_handle, layer);
	if(NULL == hw_surface) {
		return (NULL);
	}

	layer_lock();

	table = _get_layer_table();
	if(NULL == table) {
		layer_unlock();
		return (NULL);
	}

	layer_node = hash_get(table, hw_layer_name[layer]);
	if(NULL == layer_node) {
		layer_unlock();
		return (NULL);
	}

	surface = layer_node->surface;
	if(NULL == surface) {
		layer_unlock();
		return (NULL);
	}

	if(hw_surface != surface->hw_surface) {
		layer_unlock();
		return (NULL);
	}

	layer_unlock();
	return (surface);
}

int GxGDI_HardwareLayerEnable(GxLayerID layer, bool flag)
{
	int ret = 0;
	hash_t *table = NULL;
	VirtualLayer *layer_node = NULL;

	if(GXAV_ID_SIRIUS != gui.config.chip_id) {
		return (-1);
	}

	ret = GxAvdev_LayerEnable(g_device_handle, vpu_handle, layer, flag);
	layer_lock();

	table = _get_layer_table();
	if(NULL == table) {
		layer_unlock();
		return (-1);
	}

	layer_node = hash_get(table, hw_layer_name[layer]);
	if(NULL == layer_node) {
		layer_unlock();
		return (-1);
	}

	layer_node->enable = flag;
	layer_unlock();

	return (ret);
}

int GxGDI_HardwareLayerAlpha(GxLayerID layer, GxAlphaType type, int alpha)
{
	int ret = 0;
	hash_t *table = NULL;
	VirtualLayer *layer_node = NULL;
	GuiSurface *surface = NULL;
	GxAlpha alpha_para = {0};

	if(GXAV_ID_SIRIUS != gui.config.chip_id) {
		return (-1);
	}

	layer_lock();

	table = _get_layer_table();
	if(NULL == table) {
		layer_unlock();
		return (-1);
	}

	layer_node = hash_get(table, hw_layer_name[layer]);
	if(NULL == layer_node) {
		layer_unlock();
		return (-1);
	}

	surface = layer_node->surface;
	if(NULL == surface) {
		layer_unlock();
		return (-1);
	}

	layer_unlock();

	alpha_para.type = type;
	alpha_para.value = alpha;
	ret = GxAvdev_SetSurfaceAlpha(g_device_handle, vpu_handle, surface->hw_surface, &alpha_para);

	return (ret);
}

int GxGDI_HardwareLayerColorKey(GxLayerID layer, int color)
{
	int ret = 0;
	hash_t *table = NULL;
	VirtualLayer *layer_node = NULL;
	GuiSurface *surface = NULL;
	GxVpuProperty_ColorKey ColorKey = { 0 };

	layer_lock();

	table = _get_layer_table();
	if(NULL == table) {
		layer_unlock();
		return (-1);
	}

	layer_node = hash_get(table, hw_layer_name[layer]);
	if(NULL == layer_node) {
		layer_unlock();
		return (-1);
	}

	surface = layer_node->surface;
	if(NULL == surface) {
		layer_unlock();
		return (-1);
	}

	layer_unlock();

	ColorKey.enable = GUI_TRUE;
	color = hd_color2index0(surface, color);
	hd_color2formatcolor(surface, color, &(ColorKey.color));

	ColorKey.color.a = 0;
	ColorKey.surface = surface->hw_surface;
	ret = GxAVSetProperty(g_device_handle,
			      vpu_handle,
			      GxVpuPropertyID_ColorKey,
			      &ColorKey,
			      sizeof(GxVpuProperty_ColorKey));
	return (ret);
}

int GxGDI_HardwareLayerSetPriority(GxLayerID *layer_array, int count)
{
	int ret = 0;
	GxVpuProperty_OsdOverlay overlay = {{0,}, };

	if(GXAV_ID_SIRIUS != gui.config.chip_id) {
		return (-1);
	}

	if(NULL == layer_array) {
		return (-1);
	}

	if(count >= OSD_ID_MAX) {
		return (-1);
	}

	memcpy(overlay.sel, layer_array, count);
	ret = GxAVSetProperty(g_device_handle,
			      vpu_handle,
			      GxVpuPropertyID_OsdOverlay,
			      &overlay,
			      sizeof(GxVpuProperty_OsdOverlay));

	return (ret);
}


int GxGDI_HardwareLayerGetPriority(GxLayerID layer)
{
	int ret = -1;
	GxVpuProperty_OsdOverlay overlay = {{0,}, };

	if(GXAV_ID_SIRIUS != gui.config.chip_id) {
		return (-1);
	}

	if(layer > GX_LAYER_MIX_ALL) {
		return (-1);
	}
	ret = GxAVGetProperty(g_device_handle,
			      vpu_handle,
			      GxVpuPropertyID_OsdOverlay,
			      &overlay,
			      sizeof(GxVpuProperty_OsdOverlay));

	for(ret = 0; ret < OSD_ID_MAX; ret++) {
		if(overlay.sel[ret] == layer) {
			return (ret);
		}
	}

	return (ret);
}

int32_t GXGDI_LayerSetSequence(int channel, int *layers, int count)
{
	int ret = 0;
	GxVpuProperty_MixerLayers Mixer = {0};

	if((NULL == layers) || (count < 0) || (5 < count)) {
		return (-1);
	}
	Mixer.id = channel;
	if(HD_MIXER == Mixer.id) {
		memset(Mixer.layers, GX_LAYER_NONE, 5);
	}else {
		memset(Mixer.layers, GX_LAYER_NONE, 3);
	}
	memcpy(Mixer.layers, layers, count);
	ret = GxAVSetProperty(g_device_handle,
			      vpu_handle,
			      GxVpuPropertyID_MixerLayers,
			      &Mixer,
			      sizeof(GxVpuProperty_MixerLayers));

	return (ret);
}

int32_t GXGDI_LayerGetSequence(int channel, int *layers, int* count)
{
	int ret = 0, i = 0, j = 0, sum = 0;
	GxVpuProperty_MixerLayers Mixer = {0};

	if((NULL == layers) || (NULL == count)) {
		return (-1);
	}
	Mixer.id = channel;
	if(HD_MIXER == Mixer.id) {
		memset(Mixer.layers, GX_LAYER_NONE, 5);
		sum = 5;
	}else {
		memset(Mixer.layers, GX_LAYER_NONE, 3);
		sum = 3;
	}
	ret = GxAVGetProperty(g_device_handle,
			      vpu_handle,
			      GxVpuPropertyID_MixerLayers,
			      &Mixer,
			      sizeof(GxVpuProperty_MixerLayers));

	for(i = 0; i < sum; i++) {
		if(GX_LAYER_NONE != layers[i]) {
			layers[j] = layers[i];
			j++;
		}
	}

	*count = j;

	return (ret);
}

int32_t GXGDI_SetArabicMode(ARABIC_STR_PRINT_MODE mode)
{
	gui.config.arabic_mode = mode;

	return (GXCORE_SUCCESS);
}

ARABIC_STR_PRINT_MODE GXGDI_GetArabicMode(void)
{
	return (gui.config.arabic_mode);
}

int32_t GXGDI_SetArabicPunctuationMode(ARABIC_STR_PUNCTUATION_MODE mode)
{
	gui.config.arabic_punctuation_mode = mode;

	return (0);
}

ARABIC_STR_PUNCTUATION_MODE GXGDI_GetArabicPunctuationMode(void)
{
	return (gui.config.arabic_punctuation_mode);
}

int32_t GXGDI_SetArabicVaryMode(ARABIC_MODE mode)
{
	gui.config.arabic_vary_mode = mode;
	return (GXCORE_SUCCESS);
}

ARABIC_MODE GXGDI_GetArabicVaryMode(void)
{
	return (gui.config.arabic_vary_mode);
}

