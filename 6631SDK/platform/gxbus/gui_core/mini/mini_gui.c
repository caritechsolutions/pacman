#include "mini_gui.h"
#include "gal.h"
#include "mini_gui_private.h"
#include "gui_core.h"

#include "gxos/gxcore_os.h"
#ifndef NO_OS
#include "gui_private.h"
#else
#include "common/get_mem_info.h"
#endif

#include "av/avapi.h"
#include "hd_gal.h"
#include "gdi_font.h"
#ifdef NO_OS
#include "io.h"
#endif

MiniGUIConfig minigui_config = {0};
static GxMiniPartitionTable *flash_table = NULL;

#ifndef NO_OS
extern int g_device_handle;
extern int vpu_handle;;
#else
int g_device_handle;
int vpu_handle;
int jpeg_handle;
#endif
int sdc_handle;
GuiCore gui;

void mini_fb_init(struct framebuffer *fb);
void _minigui_init(void)
{
	g_device_handle = -1;
	vpu_handle = -1;
	g_device_handle = GxAVCreateDevice(0);
	if(g_device_handle < 0) {
		gxlogd("[MINI GUI]Cannot open device, %d!\n", g_device_handle);
		return;
	}

	vpu_handle = GxAVOpenModule(g_device_handle, GXAV_MOD_VPU, 0);
	if(0 > vpu_handle) {
		gxlogd("[MINI GUI]Open VPU module failed!\n");
		return;
	}

	jpeg_handle = GxAVOpenModule(g_device_handle, GXAV_MOD_JPEG_DECODER, 0);
	if(0 > jpeg_handle) {
		gxlogd("[MINI GUI]Open JPEG module failed!\n");
	}

	sdc_handle = GxAVOpenModule(g_device_handle, GXAV_MOD_SDC, 0);
	if(0 > sdc_handle) {
		gxlogd("[MINI GUI]Open SDC module failed!\n");
	}

	gui.config.chip_id = GxChipDetect(g_device_handle);
	mini_fb_init(&(gui.fb));
	return;
}

void mini_fb_init(struct framebuffer *fb)
{
	GxHwInfo vq_info = {0};

	GxCore_HwGetInfo(&vq_info);

	memset(fb, 0, sizeof(struct framebuffer));
	fb->kernel_address = (uint32_t)vq_info.kernel_address;
	fb->size = vq_info.size;
	fb->mmap_address = vq_info.mmp_address;

	fb->mem_mgr = (void *)fb->mmap_address;
}

GuiSurface *minigui_init(uint32_t width, uint32_t height, uint32_t bpp)
{
	GAL_Rect rect = {0};
	GxVpuProperty_Alpha Alpha;
	GxVpuProperty_ColorKey ColorKey = { 0 };
	GuiSurface *surface = NULL;
	GxVpuProperty_LayerViewport ViewPort = {0};
#ifdef NO_OS
	struct mem_info mem_para = {0};
#endif
	void *buffer = NULL;
	//GxAvRect viewport_rect;
	int ret = 0, color = 0;

	gui.config.width = minigui_config.width = width;
	gui.config.height = minigui_config.height = height;
	gui.config.bpp = minigui_config.bpp = bpp;
	gui.config.color_format = get_color_format(0, gui.config.bpp);

	_minigui_init();
	rect.x = rect.y = 0;
	rect.w = width;
	rect.h = height;
#ifdef NO_OS
	mem_para.type = OSD_BUF;
	ret = get_mem_info(&mem_para);
	if((0 != ret) || (0 == mem_para.len)) {
		gxlogd("Get OSD_BUF failed, maybe the OSD_BUF_SIZE is 0 in gxloader.\n");
		return (NULL);
	}

	buffer = (void *)mem_para.addr;
#endif
	surface = minigui_config.surface = hd_get_surface(&rect, bpp, buffer, 1);
	if(NULL == minigui_config.surface) {
		return (NULL);
	}

	Alpha.surface = surface->hw_surface;
	if(GX_COLOR_FMT_RGB565 == get_color_format(0, bpp)) {
		Alpha.alpha.type = GX_ALPHA_GLOBAL;
		Alpha.alpha.value = 0x7F;
	}
	else {
		Alpha.alpha.type = GX_ALPHA_PIXEL;
		Alpha.alpha.value = 0xFF;
	}
	ret = GxAVSetProperty(g_device_handle, vpu_handle, GxVpuPropertyID_Alpha, &Alpha, sizeof(GxVpuProperty_Alpha));

	gui.config.osd_trans = 0x00FF00;
	gui.config.gui_trans = 0xFF00FF;
	gal_set_gui_transparent(gui.config.gui_trans);
	gal_set_osd_transparent(gui.config.osd_trans, 0xFF);
	ColorKey.enable = 1;
	color = gui.config.osd_trans;
	if(16 == gui.config.bpp) {
		color = hd_color2index(surface, color);
		ColorKey.color.r = (color & 0xF800) >> 8;
		ColorKey.color.g = (color & 0x07E0) >> 3;
		ColorKey.color.b = (color & 0x001F) << 3;
	}
	else {
		ColorKey.color.r = (color & 0x00FF0000) >> 16;
		ColorKey.color.g = (color & 0x0000FF00) >> 8;
		ColorKey.color.b = color & 0x000000FF;
	}
	ColorKey.color.a = 0;
	ColorKey.surface = surface->hw_surface;
	ret = GxAVSetProperty(g_device_handle,
						  vpu_handle,
						  GxVpuPropertyID_ColorKey,
						  &ColorKey,
						  sizeof(GxVpuProperty_ColorKey));


	hd_enable_osd(GUI_FALSE);
	color = gui.config.osd_trans;
	color = hd_color2index(surface, color);
	ret = hd_fillrect(surface, &rect, color);

	GxVpuProperty_LayerMainSurface sur;
	sur.layer = GX_LAYER_OSD;
	sur.surface = surface->hw_surface;
	ret = GxAVSetProperty(g_device_handle,
						  vpu_handle,
						  GxVpuPropertyID_LayerMainSurface,
						  &sur,
						  sizeof(GxVpuProperty_LayerMainSurface));

	ViewPort.layer = GX_LAYER_OSD;
	ViewPort.rect.x = ViewPort.rect.y = 0;
	ViewPort.rect.width = width;
	ViewPort.rect.height = height;
	ret = GxAVSetProperty(g_device_handle,
						  vpu_handle,
						  GxVpuPropertyID_LayerViewport,
						  &ViewPort,
						  sizeof(GxVpuProperty_LayerViewport));

	hd_enable_osd(GUI_TRUE);

	return (surface);
}

GuiSurface *minigui_get_osd(void)
{
	return (minigui_config.surface);
}

status_t minigui_load_font(const char *name, const char *file, uint32_t size, uint32_t style)
{
	struct gdi_font *font = NULL;

	gdi_font_load(&(gui.config), (char *)name, (char *)file, size, style);
	gxgdi_set_font((char *)name);
	font = gxgdi_get_font();
	if(NULL == font) {
		return (GXCORE_ERROR);
	}

	if(font->family == TTF_FONT_TYPE) {
		gui.config.enable_double_buffer = GUI_TRUE;
	}

	if(NULL == minigui_config.font) {
		minigui_config.font = hash_new(10, NULL);
		if(NULL == minigui_config.font) {
			return (GXCORE_ERROR);
		}
	}

	hash_add(minigui_config.font, name, font);

	return (GXCORE_SUCCESS);
}

char *minigui_get_font(void)
{
	return (minigui_config.current_font);
}

int minigui_get_font_height(void)
{
	int height = 0;
	struct gdi_font *font = NULL;
	font = gxgdi_get_font();
	if(NULL != font) {
		height = font->ysize;
	}
	return(height);
}

status_t minigui_set_font(const char *name)
{
	struct gdi_font *font = NULL;

	if(NULL == name) {
		return (GXCORE_ERROR);
	}

	font = hash_get(minigui_config.font, name);
	if(NULL == font) {
		return (GXCORE_ERROR);
	}

	gxgdi_set_font((char *)name);

	return (GXCORE_SUCCESS);
}

status_t minigui_init_byxml(char *path)
{
	status_t ret = GXCORE_SUCCESS;

	widget_ops_init();
	if(NULL == minigui_config.signal_hash) {
		minigui_config.signal_hash = hash_new(10, NULL);
		if(NULL == minigui_config.signal_hash) {
			ret = GXCORE_ERROR;
			return (ret);
		}
	}
	ret = minigui_xml_parse(path);
	minigui_config.surface = minigui_init(minigui_config.width, minigui_config.height, minigui_config.bpp);
	if(minigui_config.surface == NULL) {
		ret = GXCORE_ERROR;
		return (ret);
	}

	return (ret);
}

status_t minigui_create_dialog(char *name)
{
	GuiWidget *widget = NULL;
	status_t ret = GXCORE_ERROR;
	GuiWidgetSignal *create_signal = NULL;
	minigui_signal_func func;

	widget = gui_get_widget(minigui_config.root_widget, name);
	if(NULL == widget) {
		return (ret);
	}

	if(widget_create(widget) == 0) {
		if(NULL == minigui_config.win_stack) {
			minigui_config.win_stack = stack_new(NULL);
		}
		if(NULL == minigui_config.win_stack) {
			return (ret);
		}

		stack_push(minigui_config.win_stack, (void *)widget);
	}

	if(widget->signal) {
		create_signal = hash_get(widget->signal, "create");
		if(create_signal) {
			func = hash_get(minigui_config.signal_hash, create_signal->handler_name);
			func((void *)widget);
		}
	}

	widget_set_update(widget, GUI_TRUE);

	ret = GXCORE_SUCCESS;
	return (ret);
}

status_t minigui_end_dialog(char *name)
{
	GuiWidget *widget = NULL, *new_widget = NULL;
	status_t ret = GXCORE_ERROR;
	GuiWidgetSignal *destroy_signal = NULL;
	minigui_signal_func func = NULL;
	int id = -1;

	widget = gui_get_widget(minigui_config.root_widget, name);
	if(NULL == widget) {
		return (ret);
	}

	id = stack_get_id(minigui_config.win_stack, (void *)widget);
	if(-1 == id) {
		return (ret);
	}

	destroy_signal = hash_get(widget->signal, "destroy");
	if(destroy_signal) {
		func = hash_get(minigui_config.signal_hash, destroy_signal->handler_name);
	}
	new_widget = stack_del(minigui_config.win_stack, id);
	widget_release(widget);
	if(func) {
		func((void *)widget);
	}

	ret = GXCORE_SUCCESS;
	return (ret);
}

status_t minigui_widget_set(char *name, char *property, void *data)
{
	GuiWidget *widget = NULL;
	status_t ret = GXCORE_SUCCESS;

	widget = gui_get_widget(minigui_config.root_widget, name);
	if(NULL == widget) {
		return (ret);
	}

	widget_set_property(widget, property, data);

	if (widget->ops && widget->ops->set) {
		ret = widget->ops->set(widget, (char*)property, data);
	}
	widget_set_update(widget, GUI_TRUE);

	return (ret);
}

void *minigui_widget_get(char *name, char *property)
{
	GuiWidget *widget = NULL;
	void *data = NULL;

	status_t ret = GXCORE_SUCCESS;

	widget = gui_get_widget(minigui_config.root_widget, name);
	if(NULL == widget) {
		return (NULL);
	}

	data = (void *)widget_get_property(widget, property);
	if((NULL == data) && (widget->ops && widget->ops->set)) {
		ret = widget->ops->get(widget, (char *)property, &data);
	}

	return (data);
}

status_t minigui_widget_add_data(char *name, void *data)
{
	status_t ret = GXCORE_SUCCESS;

	minigui_widget_set(name, "add", data);

	return (ret);
}

status_t minigui_widget_clear_data(char *name)
{
	status_t ret = GXCORE_SUCCESS;

	minigui_widget_set(name, "clear", NULL);

	return (ret);
}

status_t minigui_widget_set_focus(char *name, bool flag)
{
	status_t ret = GXCORE_ERROR;
	GuiWidget *widget = NULL;

	widget = gui_get_widget(minigui_config.root_widget, name);
	if(NULL == widget) {
		return (ret);
	}

	if(GUI_TRUE == flag) {
		if(minigui_config.focus_widget) {
			minigui_config.focus_widget->state &= ~WIDGET_STATE_FOCUSED;
		}

		minigui_config.focus_widget = widget;
		minigui_config.focus_widget->state |= WIDGET_STATE_FOCUSED;
	}
	else {
		if(widget == minigui_config.focus_widget) {
			minigui_config.focus_widget = NULL;
		}
		widget->state &= ~WIDGET_STATE_FOCUSED;
	}

	ret = GXCORE_SUCCESS;
	return (ret);
}

char *minigui_widget_get_focus(void)
{
	char *focus_widget = NULL;

	if(NULL == minigui_config.focus_widget)
		return (NULL);

	focus_widget = minigui_config.focus_widget->name;

	return (focus_widget);
}

static void _minigui_draw_widget(GuiWidget *widget)
{
	GuiWidget *pnext = NULL;

	pnext = widget;
	while(pnext) {
		if(pnext->need_update) {
			widget_draw(pnext);
		}
		else if(pnext->first_child) {
			_minigui_draw_widget(pnext->first_child);
		}
		pnext = pnext->next_sibling;
	}

	if(0 == stack_count(minigui_config.win_stack)) {
		MiniGUI_Rect rect = {0};

		rect.x = rect.y = 0;
		rect.w = minigui_config.width;
		rect.h = minigui_config.height;
		minigui_fillrect(minigui_config.surface, &rect, 0x00FF00);
	}
}

void minigui_flush(void)
{
	_minigui_draw_widget(minigui_config.root_widget);
}

status_t minigui_connect_signal(const char *handlename, minigui_signal_func func)
{
	status_t ret = GXCORE_ERROR;

	if(NULL == minigui_config.signal_hash) {
		minigui_config.signal_hash = hash_new(10, NULL);
		if(NULL == minigui_config.signal_hash)
			return (ret);
	}

	hash_add(minigui_config.signal_hash, handlename, func);

	ret = GXCORE_SUCCESS;
	return (ret);
}

static int _colorformt_to_bpp(GxColorFormat format)
{
	switch(format)
	{
		case GX_COLOR_FMT_YCBCR422:
			return (0);
		case GX_COLOR_FMT_CLUT1:
			return (1);
		case GX_COLOR_FMT_YCBCRA6442:
			return (2);
		case GX_COLOR_FMT_Y8:
			return (3);
		case GX_COLOR_FMT_CLUT8:
			return (8);
		case GX_COLOR_FMT_RGB565:
			return (16);
		case GX_COLOR_FMT_RGB888:
			return (24);
		case GX_COLOR_FMT_ARGB8888:
			return (32);
		default:
			return (16);
	}
}

GuiSurface *minigui_get_surface(MiniGUI_Rect *rect, GxColorFormat format)
{
	int bpp = _colorformt_to_bpp(format);

	if(NULL == rect) {
		return (NULL);
	}

	GAL_Rect inner_rect = {0};

	inner_rect.x = rect->x;
	inner_rect.y = rect->y;
	inner_rect.w = rect->w;
	inner_rect.h = rect->h;
	return (hd_get_surface0(&inner_rect, bpp, format, NULL, 1));
}

static GuiSurface *minigui_get_spp_mainsurface(MiniGUI_Rect *rect, GxColorFormat format)
{
	int bpp = _colorformt_to_bpp(format), ret = 0;
#ifdef NO_OS
	struct mem_info mem_para = {0};
#endif
	void *buffer = NULL;

	if(NULL == rect) {
		return (NULL);
	}

#ifdef NO_OS
	mem_para.type = SPP_BUF;

	ret = get_mem_info(&mem_para);
	if((0 != ret) || (0 == mem_para.len)) {
		gxlogd("Get SPP_BUF failed, maybe the SPP_BUF_SIZE is 0 in gxloader.\n");
		return (NULL);
	}

	buffer = (void *)mem_para.addr;
#endif

	GAL_Rect inner_rect = {0};

	inner_rect.x = rect->x;
	inner_rect.y = rect->y;
	inner_rect.w = rect->w;
	inner_rect.h = rect->h;
	return (hd_get_surface0(&inner_rect, bpp, format, buffer, 1));
}

status_t minigui_free_surface(GuiSurface *surface)
{
	hd_free_surface(surface);
	return (GXCORE_SUCCESS);
}

status_t minigui_fillrect(GuiSurface *surface, MiniGUI_Rect *rect, MiniGUIColor color)
{
	int ret = 0;
	GAL_Rect fill_rect = {0};
	int fill_color = 0;

	if((NULL == surface) || (NULL == rect)) {
		return (GXCORE_ERROR);
	}

	fill_rect.x = rect->x;
	fill_rect.y = rect->y;
	fill_rect.w = rect->w;
	fill_rect.h = rect->h;
	fill_color = hd_color2index(surface, color);
	ret = hd_fillrect(surface, &fill_rect, fill_color);
	if(0 == ret)
		return (GXCORE_SUCCESS);
	else
		return (GXCORE_ERROR);
}

status_t minigui_copy(GuiSurface *src, MiniGUI_Rect *src_rect, GuiSurface *dst, MiniGUI_Rect *dst_rect)
{
	int ret = 0;
	GAL_Rect src_rect0 = {0}, dst_rect0 = {0};

	if((NULL == src) || (NULL == dst)) {
		return (GXCORE_ERROR);
	}

	src_rect0.x = src_rect->x;
	src_rect0.y = src_rect->y;
	src_rect0.w = src_rect->w;
	src_rect0.h = src_rect->h;

	dst_rect0.x = dst_rect->x;
	dst_rect0.y = dst_rect->y;
	dst_rect0.w = dst_rect->w;
	dst_rect0.h = dst_rect->h;

	ret = hd_copy_surface(src, &src_rect0, dst, &dst_rect0);

	return (ret);
}

int32_t minigui_draw_string(GuiSurface *surface,
							char *string,
							MiniGUI_Rect *rect,
							MiniGUIColor color,
							int32_t alignment)
{
	int fore_color =  hd_color2index(surface, color);
	GAL_Interval interval = {0};
	GAL_Rect area_rect = {0};

	if((NULL == surface) ||
	   (NULL == string) ||
	   (NULL == rect))
	{
		return (0);
	}

	area_rect.x = rect->x;
	area_rect.y = rect->y;
	area_rect.w = rect->w;
	area_rect.h = rect->h;
	return (gxgdi_draw_string(surface, string, &area_rect, &interval, alignment, fore_color, PARAGRAPH_MODE));
}

int32_t minigui_get_string_pixel(const char *string)
{
	return (gdi_text_len((void *)string));
}

image_desc *minigui_load_image(const char *file)
{
	return (gal_img_load(NULL, file));
}

int32_t minigui_draw_image(GuiSurface *surface, const image_desc *pDesc, int x, int y, int w, int h)
{
	return (hd_img_desc(surface, pDesc, x, y, w, h));
}

int32_t minigui_config_logo_surface(void)
{
	MiniGUI_Rect rect = {0};
	int ret = 0;
	if(NULL == minigui_config.logo_surface) {
		rect.x = rect.y = 0;
		rect.w = minigui_config.width;
		rect.h = minigui_config.height;
		minigui_config.logo_surface = minigui_get_spp_mainsurface(&rect, GX_COLOR_FMT_YCBCR422);
		if(NULL == minigui_config.logo_surface)
			return (-1);

		GxVpuProperty_LayerMainSurface sur;
		sur.layer = GX_LAYER_SPP;
		sur.surface = minigui_config.logo_surface->hw_surface;
		ret = GxAVSetProperty(g_device_handle,
							  vpu_handle,
							  GxVpuPropertyID_LayerMainSurface,
							  &sur,
							  sizeof(GxVpuProperty_LayerMainSurface));

		GxVpuProperty_FillRect FillRect = {0};
		FillRect.color.y = 0x10;
		FillRect.color.cb = 0x80;
		FillRect.color.cr = 0x80;
		FillRect.color.alpha = 0xFF;
		FillRect.rect.x = 0;
		FillRect.rect.y = 0;
		FillRect.is_ksurface = is_ksurface(minigui_config.logo_surface);
		FillRect.rect.width = rect.w;
		FillRect.rect.height = rect.h;
		FillRect.surface = minigui_config.logo_surface->hw_surface;
		ret = GxAVSetProperty(g_device_handle,
							  vpu_handle,
							  GxVpuPropertyID_FillRect,
							  &FillRect,
							  sizeof(GxVpuProperty_FillRect));


		GxVpuProperty_LayerViewport ViewPort = {0};
		ViewPort.layer = GX_LAYER_SPP;
		ViewPort.rect.x = ViewPort.rect.y = 0;
		ViewPort.rect.width = minigui_config.width;
		ViewPort.rect.height = minigui_config.height;
		ret = GxAVSetProperty(g_device_handle,
							vpu_handle,
							GxVpuPropertyID_LayerViewport,
							&ViewPort,
							sizeof(GxVpuProperty_LayerViewport));
	}

	return (ret);
}

int32_t minigui_show_logo(const char *id, const char *file)
{
	image_desc *desc = NULL;
	int ret = 0;

	ret = minigui_config_logo_surface();
	if(ret != 0) {
		return (-1);
	}

	if(id && minigui_config.image) {
		desc = hash_get(minigui_config.image, id);
	}
	else {
		desc = minigui_load_image(file);
	}
	if(NULL == desc) {
		return (-1);
	}

	ret = minigui_draw_image(minigui_config.logo_surface,
                             desc,
                             0,
                             0,
                             minigui_config.width,
                             minigui_config.height);

	GxVpuProperty_LayerEnable property;

	memset(&property, 0, sizeof(property));
	property.layer = GX_LAYER_SPP;
	property.enable = GUI_TRUE;

	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_LayerEnable,
			&property,
			sizeof(GxVpuProperty_LayerEnable));
	return (ret);
}

int32_t minigui_show_logo_from_partition(const char *partition_name, int size)
{
	GxMiniPartition *partition;
	char *buf = NULL;
	int        ret;
	if(partition_name == NULL) {
		gxlogd("[partiton]open parameter err!! %s  %d\n",__FILE__,__LINE__);
		return (0);
	}

	buf = (char *)malloc(size);
	if(buf == NULL) {
		gxlogd("malloc failed\n");
		return (-1);
	}

	memset(buf, 0, size);

	if(flash_table == NULL) {
		flash_table = GxCore_PartitionFlashInit();
	}

	partition = GxCore_PartitionGetByName((GxMiniPartitionTable *)flash_table, GX_PARTITION_LOGO);
	if(partition == NULL) {
		free(buf);
		return (-1);
	}

	ret = GxCore_PartitionRead(partition, (void *)buf, size, 0);
	if(ret == 0) {
		gxlogd("%s,%d, logo read error!\n", __func__, __LINE__);
		free(buf);
		return (-1);
	}

#ifdef NO_OS
	file_add("logo.jpg", buf, size);
#endif
	minigui_show_logo(NULL, "logo.jpg");
#ifdef NO_OS
	file_del("logo.jpg");
#endif

	free(buf);

	return (ret);
}


int32_t minigui_show_logo_from_data(void *data, int width, int height)
{
    image_desc desc = {0};
	int ret = 0;

	ret = minigui_config_logo_surface();
	if(ret != 0) {
		return (-1);
	}
	desc.data = data;
	desc.width = width;
	desc.height = height;
	desc.bpp = 16;
	desc.size = width * height * desc.bpp / 8;
	desc.type = JPEG_TYPE;
	desc.color = GX_COLOR_FMT_YCBCR422;
	ret = minigui_draw_image(minigui_config.logo_surface,
                             &desc,
                             0,
                             0,
                             minigui_config.width,
                             minigui_config.height);
	GxVpuProperty_LayerEnable property;

	memset(&property, 0, sizeof(property));
	property.layer = GX_LAYER_SPP;
	property.enable = GUI_TRUE;

	ret = GxAVSetProperty(g_device_handle,
            vpu_handle,
            GxVpuPropertyID_LayerEnable,
            &property,
            sizeof(GxVpuProperty_LayerEnable));

	return (ret);
}

int minigui_free_image(image_desc *image)
{
	return (gal_img_release(image));
}

#ifdef NO_OS
GuiCore *GUI_GetCurrent(void)
{
	return (&gui);
}
#endif /*NO_OS*/

