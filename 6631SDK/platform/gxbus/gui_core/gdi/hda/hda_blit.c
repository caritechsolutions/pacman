#include "linklist.h"
#include "stack.h"
#ifndef NO_OS
#include "hd_blit.h"
#include "hd_gal.h"
#include "gui_core.h"
#include "av/avapi.h"
#include "av/gxav.h"
#include "av/gxav_vpu_propertytypes.h"
#include "gui_private.h"
#include "gui.h"
#include "gxavdev.h"
#include "image_cache.h"

#ifdef LINUX_OS
#include <unistd.h> //for mmap

#if (_FILE_OFFSET_BITS==64)
#undef __USE_FILE_OFFSET64
#include <sys/mman.h> //for mmap

#define __USE_FILE_OFFSET64

#else
#include <sys/mman.h> //for mmap
#endif

#endif
static handle_t blit_mutex = -1;

static void _insert_to_clone_list(GuiSurface *source, GuiSurface *cloned)
{
	GuiSurface *surface = source;
	while(surface) {
		if(NULL == surface->next_cloned) {
			cloned->next_cloned = NULL;
			surface->next_cloned = cloned;
			break;
		}
		surface = surface->next_cloned;
	}
}

int hd_change_surface(void **src0, void **src1)
{
	void *p = *src0;
	*src0 = *src1;
	*src1 = p;

	return (0);
}

GuiSurface *hd_surface_clone(GxLayerID layer, GAL_Rect *rect, int bpp)
{
	GuiSurface *surface = NULL, *new_surface = NULL;
	int real_bpp = 0;

	if (rect == NULL)
		return NULL;

	if(GX_LAYER_OSD == layer)
	{
	    if(gui.config.buffer_share) {
		surface = screen;
	    } else {
		hd_set_layer_surface(LAYER_MAIN_OSD_SURFACE);
		if(gui.config.enable_double_buffer)
			hd_set_layer_surface(LAYER_BACK_OSD_SURFACE);
		surface = screen;
	    }
	}
	else if(GX_LAYER_SPP == layer)
	{
		if((gui.config.width >= rect->w) &&
		   (gui.config.height >= rect->h))
		{
			if(gui.config.buffer_share) {
				surface = spp_screen;
			} else {
				hd_set_layer_surface(LAYER_MAIN_SPP_SURFACE);
				surface = spp_screen;
			}
		}
		else
		{
			surface = NULL;
		}
	}

	if((gui.config.buffer_share) && (NULL == surface)) {
		if(layer == GX_LAYER_OSD) {
			surface = spp_screen;
		} else if(layer == GX_LAYER_SPP) {
			surface = screen;
		} else {
			;
		}
	}

	if (surface)
	{
		if ((bpp == 0) || (bpp == 2))
		    real_bpp = 2;
		else if(bpp == 3)
		    real_bpp = 1;
		else if(bpp == 4)
			real_bpp = 2;
		else
			real_bpp = bpp / 8;
		if(surface->buffer_size >= (rect->w * rect->h * real_bpp))
		{
			new_surface = hd_get_surface(rect, bpp, surface->data, 1);
			new_surface->source = surface;
			_insert_to_clone_list(surface, new_surface);
		}
		else
			new_surface = hd_get_surface(rect, bpp, NULL, 1);
	}
	else
	{
		if ((bpp == 0) || (bpp == 2))
		    real_bpp = 16;
		else if(bpp == 3)
		    real_bpp = 8;
		else if(bpp == 4)
			real_bpp = 2;
		new_surface = hd_get_surface(rect, bpp, NULL, 1);
	}

	return new_surface;
}

void *idle_surface = NULL;
static int hd_copy_surface0(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect);

static int main_index = 0;
static int main_surface_flag[2] = {MALLOC_NO_CACHE, MALLOC_NO_CACHE};

GuiSurface *hd_get_surface0(GAL_Rect *rect, int bpp, int color_format, void *buffer, int force)
{
	int ret = 0;
	GuiSurface *surface;
	GuiCore *gui_core = GUI_GetCurrent();
	int real_bpp = 0;
	GxVpuProperty_CreateSurface CreateSurface = {0};

	if (rect == NULL)
		return NULL;

	if (rect->w <= 0 || rect->h <= 0) {
		GUI_Debug_Print("%s error: width: %d, height: %d\n", __func__, rect->w, rect->h);
		return NULL;
	}
	surface = (GuiSurface *)GxCore_Mallocz(sizeof(GuiSurface));
	if (surface == NULL) {
		gxlogd("[GUI] Alloc GuiSurface error.\n");
		return NULL;
	}

	real_bpp = bpp;
	if ((bpp == 0) || (bpp == 2))
	    real_bpp = 16;
	else if(bpp == 3)
	    real_bpp = 8;
	else if(bpp == 4)
		real_bpp = 2;

	surface->bpp             = real_bpp;
	if(bpp == 4)
		surface->buffer_size     = rect->w * rect->h * 3 / 2;
	else
		surface->buffer_size     = rect->w * rect->h * real_bpp / 8;
	surface->sf.width        = rect->w;
	surface->sf.height       = rect->h;
	surface->sf.color_format = color_format;
	surface->hw              = 0;

	if (buffer != NULL) {
#ifndef NO_OS
		surface->sf.buffer = (void*)fb_getphys(&gui.fb, buffer);
		if (surface->sf.buffer > 0) {
			surface->data       = buffer;
			if (force == 0) {
				surface->hw         = GUI_FALSE;
				surface->hw_surface = &surface->sf;
				//surface_set_color_format(surface, color_format);
				return surface;
			}
		}
		else
		{
		    surface->sf.buffer = buffer;
		    surface->data = (uint8_t *)GxCore_Map(g_device_handle, (uint32_t)surface->sf.buffer, surface->buffer_size);
		}
#else
		surface->data = surface->sf.buffer = buffer;
#endif /*NO_OS*/
	}
	else if (gui.fb.mem_mgr) {
#ifndef NO_OS
		if(force && (main_index < (gui.config.enable_double_buffer + 1)) && (bpp))
		{
			surface->vq   = hd_malloc_main(&(surface->vq), surface->buffer_size, main_surface_flag[main_index]);
			main_index++;
		}
		else
#endif /*NO_OS*/
		{
			surface->vq   = hd_malloc_nocache(&(surface->vq), surface->buffer_size);
			main_index = 0;
		}
		surface->data = surface->vq->usr_p;

		if (surface->data != NULL) {
			surface->sf.buffer = (void*)fb_getphys(&gui.fb, surface->data);
			surface->hw |= VBUFFER;
			image_cache_add_data(gui_core->config.image_cache, surface); // SAVE LIST

			if (force == 0) {
				surface->hw_surface = &surface->sf;
				//surface_set_color_format(surface, color_format);
				return surface;
			}
		}
	}

	memset(&CreateSurface, 0, sizeof(CreateSurface));
	CreateSurface.format = color_format;
	CreateSurface.width  = rect->w;
	CreateSurface.height = rect->h;
	CreateSurface.mode   = GX_SURFACE_MODE_IMAGE;
	CreateSurface.buffer = surface->sf.buffer;

	ret |= GxAVGetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_CreateSurface,
			&CreateSurface,
			sizeof(GxVpuProperty_CreateSurface));

	if(ret != 0) {
		gxlogd("[GUI]Cannot create surface : %d\n", ret);
		GxCore_Free(surface);
		return (NULL);
	} else if((CreateSurface.buffer) &&
		  (surface->vq) &&
		  (NULL == surface->vq->usr_p) &&
		  (NULL == surface->vq->kernel_p)) {
			gui_core->fb.overflow_size += surface->vq->size;
			if(gui_core->fb.max_overflow_size < gui_core->fb.overflow_size) {
				gui_core->fb.max_overflow_size = gui_core->fb.overflow_size;
			}
	}

	surface->hw |= KSURFACE;
	surface->hw_surface = CreateSurface.surface;
	if (surface->sf.buffer == 0) {
		surface->sf.buffer = CreateSurface.buffer;
		surface->data = (uint8_t *)GxCore_Map(g_device_handle, (uint32_t)surface->sf.buffer, surface->buffer_size);
		if(surface->vq) {
			hd_free(surface->vq);
			GUI_FREE(surface->vq);
		}
	}

	return surface;
}

void hd_clear(GxLayerID layer)
{
	GuiCore *gui_core = NULL;
	GAL_Rect rect;
	unsigned int color = 0;

	gui_core = GUI_GetCurrent();
	if(GX_LAYER_OSD == layer) {
		rect.x = rect.y = 0;
		rect.w = gui_core->config.width;
		rect.h = gui_core->config.height;

		color = gal_color2index(screen, gui.config.osd_trans);
		hd_fillrect(screen, &rect, color);

		if(gui_core->config.enable_double_buffer)
			hd_fillrect(back_screen, &rect, color);
	}
	else if(GX_LAYER_SPP == layer)
		hd_img_clear();
}

int hd_rotate_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect, GxScrewMode mode)
{
    int ret = 0;
    GxVpuProperty_TurnSurface TurnSurface = {0};

    if((NULL == src) ||
       (NULL == src_rect) ||
       (NULL == dst) ||
       (NULL == dst_rect))
	return 1;

    gdi_begin();
    TurnSurface.src_surface = src->hw_surface;
    TurnSurface.src_is_ksurface = is_ksurface(src);
    TurnSurface.src_rect.x = src_rect->x;
    TurnSurface.src_rect.y = src_rect->y;
    TurnSurface.src_rect.width = src_rect->w;
    TurnSurface.src_rect.height = src_rect->h;
    TurnSurface.dst_surface = dst->hw_surface;
    TurnSurface.dst_is_ksurface = is_ksurface(dst);
    TurnSurface.dst_rect.x = dst_rect->x;
    TurnSurface.dst_rect.y = dst_rect->y;
    TurnSurface.dst_rect.width = dst_rect->w;
    TurnSurface.dst_rect.height = dst_rect->h;
    TurnSurface.screw = mode;
    ret = GxAVSetProperty(g_device_handle,
			  vpu_handle,
			  GxVpuPropertyID_TurnSurface,
			  &TurnSurface,
			  sizeof(GxVpuProperty_TurnSurface));
    gdi_end();

    return ret;
}

int hd_blit_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect)
{
    int ret = 0;
    GuiSurface *tmp_surface0 = NULL, *tmp_surface1 = NULL;
    GAL_Rect tmp_rect = {0};

    if(src == NULL || src_rect == NULL || dst == NULL || dst_rect == NULL)
	return 1;

    if((src_rect->w == dst_rect->w) &&
       (src_rect->h == dst_rect->h))
    {
	ret = hd_stretch_surface(src, src_rect, dst, dst_rect);
	return (ret);
    }
    else
    {
	if(GX_COLOR_FMT_YCBCR422 == src->sf.color_format)
	{
	    tmp_surface0 = hd_get_surface(src_rect, 16, NULL, GUI_TRUE);
	    tmp_rect = *dst_rect;
	    tmp_rect.x = tmp_rect.y = 0;
	    tmp_surface1 = hd_get_surface(&tmp_rect, 16, NULL, GUI_TRUE);
	    ret = hd_copy_surface(src, src_rect, tmp_surface0, src_rect);
	    ret |= hd_zoom_surface(tmp_surface0, src_rect, tmp_surface1, &tmp_rect);
	    ret |= hd_copy_surface(tmp_surface1, &tmp_rect, dst, dst_rect);
	    hd_free_surface(tmp_surface0);
	    hd_free_surface(tmp_surface1);
	    return (ret);
	}
	ret = hd_zoom_surface(src, src_rect, dst, dst_rect);
	return (ret);
    }
}

#define MAX_LIMIT(value)    do {		    \
			    if(value < 0)	    \
				value = 0;	    \
			    else if(value > 0xFF)   \
				value = 0xFF;	    \
}while(0)

static unsigned short _yuv_to_rgb(unsigned char y, unsigned char u, unsigned char v)
{
    short r = 0, g = 0, b = 0;
    short tu = 0, tv = 0;
    unsigned short value = 0;

    tu = u - 128;
    tv = v - 128;

    r = y + (114 * tv) / 100;
    g = y - (39 * tu + 58 * tv) / 100;
    b = y + (203 * tu) / 100;

    MAX_LIMIT(r);
    MAX_LIMIT(g);
    MAX_LIMIT(b);

    value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);

    return value;
}

int hd_video_size(GxLayerID layer, int *width, int *height)
{
    GxVpuProperty_LayerCapture Capture = {0};
    GxVpuProperty_DestroySurface DesSurface = { 0 };
    GxSurface *capture_surface = NULL;
    int ret = 0;

    Capture.layer = layer;
    Capture.surface = NULL;
    Capture.rect.x = 0;
    Capture.rect.y = 0;
    Capture.rect.width = gui.config.width;
    Capture.rect.height = gui.config.height;

    gdi_begin();
    ret = GxAVGetProperty(g_device_handle,
			  vpu_handle,
			  GxVpuPropertyID_LayerCapture,
			  &Capture,
			  sizeof(GxVpuProperty_LayerCapture));
    gdi_end();

    if(ret)
	return 1;
    capture_surface = (GxSurface *)(Capture.surface);
    *width = capture_surface->width;
    *height = capture_surface->height;

    memset(&DesSurface, 0, sizeof(DesSurface));
    DesSurface.surface = Capture.surface;
    ret = GxAVSetProperty(g_device_handle,
			  vpu_handle,
			  GxVpuPropertyID_DestroySurface,
			  &DesSurface,
			  sizeof(GxVpuProperty_DestroySurface));
    if(ret)
	return 1;

    return ret;
}

int hd_capture(GxLayerID layer, GuiSurface *surface, GAL_Rect *rect)
{
    GxVpuProperty_LayerCapture Capture = {0};
    GxVpuProperty_ZoomSurface zoom_surface = {0};
    //GxVpuProperty_DestroySurface DesSurface = { 0 };
	GuiSurface *video_surface = NULL;
    GuiSurface *src_y = NULL, *src_u = NULL, *src_v = NULL;
    GuiSurface *dst_surface_y = NULL, *dst_surface_u = NULL, *dst_surface_v = NULL;
    unsigned char *buffer = NULL;
    unsigned char *buffer_y = NULL, *buffer_u = NULL, *buffer_v = NULL;
    unsigned char *dst_y = NULL, *dst_u = NULL, *dst_v = NULL;
    unsigned char u = 0, v = 0, y = 0;
    unsigned char *dst_buffer = NULL;
	GAL_Rect zoom_rect = {0};
    GAL_Rect src_y_rect = {0}, src_uv_rect = {0};
    GAL_Rect dst_y_rect = {0}, dst_uv_rect = {0};
    int ret = 0, size = 0, uv_size = 0, dst_size = 0, i = 0, j = 0;
    unsigned short rgb_value = 0;

    if((NULL == surface) || (NULL == rect))
	return 1;

	video_surface = hd_get_surface(rect, 4, NULL, GUI_TRUE);
	if(NULL == video_surface)
		return (1);

	memset(video_surface->data, 0, video_surface->buffer_size);

	zoom_rect.w = surface->sf.width;
	zoom_rect.h = surface->sf.height;

    Capture.layer = layer;
    Capture.surface = video_surface->hw_surface;
    Capture.rect.x = 0;
    Capture.rect.y = 0;
    Capture.rect.width = rect->w;
    Capture.rect.height = rect->h;

    gdi_begin();
    ret = GxAVGetProperty(g_device_handle,
			  vpu_handle,
			  GxVpuPropertyID_LayerCapture,
			  &Capture,
			  sizeof(GxVpuProperty_LayerCapture));
    gdi_end();

    if(ret)
    {
	hd_free_surface(video_surface);
	return 1;
    }

    size = video_surface->sf.width * video_surface->sf.height;//video_surface->buffer_size;
    uv_size = size / 4;
    size = size + uv_size * 2;

    buffer = video_surface->data;//(uint8_t *)GxCore_Map(g_device_handle, (uint32_t)video_surface->sf.buffer, size);
    buffer_y = buffer;
    buffer_u = buffer + (video_surface->sf.width * video_surface->sf.height);
    buffer_v = (unsigned char *)GxCore_Mallocz(uv_size);
    for(i = 0; i < uv_size; i++)
    {
	u = buffer_u[i * 2];
	v = buffer_u[i * 2 + 1];
	buffer_u[i] = u;
	buffer_v[i] = v;
    }

    memcpy(buffer_u + uv_size, buffer_v, uv_size);
    GUI_FREE(buffer_v);
    buffer_v = buffer_u + uv_size;

    if((zoom_rect.w <= video_surface->sf.width) &&
       (zoom_rect.h <= video_surface->sf.height))
    {
	/*Zoom*/
	dst_size = zoom_rect.w * zoom_rect.h;

	src_y_rect.x = src_y_rect.y = 0;
	src_y_rect.w = video_surface->sf.width;
	src_y_rect.h = video_surface->sf.height;

	src_uv_rect = src_y_rect;
	src_uv_rect.w = src_uv_rect.w / 2;
	src_uv_rect.h = src_uv_rect.h / 2;

	dst_y_rect.x = dst_y_rect.y = 0;
	dst_y_rect.w = zoom_rect.w;
	dst_y_rect.h = zoom_rect.h;

	dst_uv_rect = dst_y_rect;
	dst_uv_rect.w = dst_uv_rect.w / 2;
	dst_uv_rect.h = dst_uv_rect.h / 2;

	src_y = hd_get_surface(&src_y_rect, 3, buffer_y, GUI_TRUE);
	src_u = hd_get_surface(&src_uv_rect, 3, buffer_u, GUI_TRUE);
	src_v = hd_get_surface(&src_uv_rect, 3, buffer_v, GUI_TRUE);

	dst_surface_y = hd_get_surface(&dst_y_rect, 3, NULL, GUI_TRUE);
	dst_surface_u = hd_get_surface(&dst_uv_rect, 3, NULL, GUI_TRUE);
	dst_surface_v = hd_get_surface(&dst_uv_rect, 3, NULL, GUI_TRUE);
	dst_y = dst_surface_y->data;
	dst_u = dst_surface_u->data;
	dst_v = dst_surface_v->data;

	gdi_begin();
	zoom_surface.src_surface = src_y->hw_surface;
	zoom_surface.src_is_ksurface = is_ksurface(src_y);
	zoom_surface.src_rect.x = zoom_surface.src_rect.y = 0;
	zoom_surface.src_rect.width = src_y_rect.w;
	zoom_surface.src_rect.height = src_y_rect.h;
	zoom_surface.dst_surface = dst_surface_y->hw_surface;
	zoom_surface.dst_is_ksurface = is_ksurface(dst_surface_y);
	zoom_surface.dst_rect.x = zoom_surface.dst_rect.y = 0;
	zoom_surface.dst_rect.width = dst_y_rect.w;
	zoom_surface.dst_rect.height = dst_y_rect.h;
	ret = GxAVSetProperty(g_device_handle,
			      vpu_handle,
			      GxVpuPropertyID_ZoomSurface,
			      &zoom_surface,
			      sizeof(GxVpuProperty_ZoomSurface));
	gdi_end();

	gdi_begin();
	zoom_surface.src_surface = src_u->hw_surface;
	zoom_surface.src_is_ksurface = is_ksurface(src_u);
	zoom_surface.src_rect.x = zoom_surface.src_rect.y = 0;
	zoom_surface.src_rect.width = src_uv_rect.w;
	zoom_surface.src_rect.height = src_uv_rect.h;
	zoom_surface.dst_surface = dst_surface_u->hw_surface;
	zoom_surface.dst_is_ksurface = is_ksurface(dst_surface_u);
	zoom_surface.dst_rect.x = zoom_surface.dst_rect.y = 0;
	zoom_surface.dst_rect.width = dst_uv_rect.w;
	zoom_surface.dst_rect.height = dst_uv_rect.h;
	ret = GxAVSetProperty(g_device_handle,
			      vpu_handle,
			      GxVpuPropertyID_ZoomSurface,
			      &zoom_surface,
			      sizeof(GxVpuProperty_ZoomSurface));
	gdi_end();

	gdi_begin();
	zoom_surface.src_surface = src_v->hw_surface;
	zoom_surface.src_is_ksurface = is_ksurface(src_v);
	zoom_surface.src_rect.x = zoom_surface.src_rect.y = 0;
	zoom_surface.src_rect.width = src_uv_rect.w;
	zoom_surface.src_rect.height = src_uv_rect.h;
	zoom_surface.dst_surface = dst_surface_v->hw_surface;
	zoom_surface.dst_is_ksurface = is_ksurface(dst_surface_v);
	zoom_surface.dst_rect.x = zoom_surface.dst_rect.y = 0;
	zoom_surface.dst_rect.width = dst_uv_rect.w;
	zoom_surface.dst_rect.height = dst_uv_rect.h;
	ret = GxAVSetProperty(g_device_handle,
			      vpu_handle,
			      GxVpuPropertyID_ZoomSurface,
			      &zoom_surface,
			      sizeof(GxVpuProperty_ZoomSurface));
	gdi_end();
    }

    dst_buffer = surface->data;

    for(i = 0; i < surface->sf.height; i++)
    {
	for(j = 0; j < surface->sf.width; j++)
	{
	    y = dst_y[i * dst_y_rect.w + j];
	    u = dst_u[(i / 2) * dst_uv_rect.w + (j / 2)];
	    v = dst_v[(i / 2) * dst_uv_rect.w + (j / 2)];
	    rgb_value = _yuv_to_rgb(y, u, v);
	    dst_buffer[i * surface->sf.width * 2 + j * 2 + 1] = (rgb_value >> 8) & 0x00FF;
	    dst_buffer[i * surface->sf.width * 2 + j * 2] = rgb_value & 0x00FF;
	}
    }

    hd_free_surface(src_y);
    hd_free_surface(src_u);
    hd_free_surface(src_v);
    hd_free_surface(dst_surface_y);
    hd_free_surface(dst_surface_u);
    hd_free_surface(dst_surface_v);
    hd_free_surface(video_surface);

    /*GxCore_UnMap(g_device_handle, buffer, size);
    memset(&DesSurface, 0, sizeof(DesSurface));
    DesSurface.surface = Capture.surface;
    ret = GxAVSetProperty(g_device_handle,
			  vpu_handle,
			  GxVpuPropertyID_DestroySurface,
			  &DesSurface,
			  sizeof(GxVpuProperty_DestroySurface));*/

    return ret;
}
#else /*NO_OS*/
#include "hd_gal.h"
#include "gdi_core.h"
#include "gui_core.h"
#include "gxtype.h"
#include "av/gxav.h"
#include "mini_gui_private.h"
#include "common/gxmalloc.h"

extern int g_device_handle;
extern int vpu_handle;
extern GuiCore gui;
#ifndef MAX_BLIT_COUNT
	#define MAX_BLIT_COUNT			(300)
#endif

GxHwMallocObj *hd_malloc_nocache(GxHwMallocObj **p, size_t size)
{
	if(NULL == *p)
	{
		*p = GxCore_Mallocz(sizeof(GxHwMallocObj));
		if(NULL == *p)
			return (NULL);

		(*p)->size = size;
	}

	*p = fb_malloc_nocache(&gui.fb, *p);

	return *p;
}

GuiSurface *hd_get_surface0(GAL_Rect *rect, int bpp, int color_format, void *buffer, int force)
{
	int ret = 0;
	GuiSurface *surface;
	int real_bpp = 0;
	GxVpuProperty_CreateSurface CreateSurface = {0};

	if (rect == NULL)
		return NULL;

	if (rect->w == 0 || rect->h == 0) {
		gxlogd("%s error: width: %d, height: %d\n", __func__, rect->w, rect->h);
		return NULL;
	}
	surface = (GuiSurface *)GxCore_Mallocz(sizeof(GuiSurface));
	if (surface == NULL) {
		gxlogd("[GUI] Alloc GuiSurface error.\n");
		return NULL;
	}

	real_bpp = bpp;
	if ((bpp == 0) || (bpp == 2))
	    real_bpp = 16;
	else if(bpp == 3)
	    real_bpp = 8;
	else if(bpp == 4)
		real_bpp = 2;

	surface->bpp             = real_bpp;
	if(bpp == 4)
		surface->buffer_size     = rect->w * rect->h * 3 / 2;
	else
		surface->buffer_size     = rect->w * rect->h * real_bpp / 8;
	surface->sf.width        = rect->w;
	surface->sf.height       = rect->h;
	surface->sf.color_format = color_format;
	surface->hw              = 0;

	if (buffer != NULL) {
#ifndef NO_OS
		surface->sf.buffer = (void*)fb_getphys(&gui.fb, buffer);
		if (surface->sf.buffer > 0) {
			surface->data       = buffer;
			if (force == 0) {
				surface->hw         = GUI_FALSE;
				surface->hw_surface = &surface->sf;
				//surface_set_color_format(surface, color_format);
				return surface;
			}
		}
		else
		{
		    surface->sf.buffer = buffer;
		    surface->data = (uint8_t *)GxCore_Map(g_device_handle, (uint32_t)surface->sf.buffer, surface->buffer_size);
		}
#else
		surface->data = surface->sf.buffer = buffer;
#endif /*NO_OS*/
	}
	else if (gui.fb.mem_mgr) {
		surface->vq   = hd_malloc_nocache(&(surface->vq), surface->buffer_size);
		surface->data = surface->vq->usr_p;

		if (surface->data != NULL) {
			surface->sf.buffer = (void*)fb_getphys(&gui.fb, surface->data);
			surface->hw |= VBUFFER;

			if (force == 0) {
				surface->hw_surface = &surface->sf;
				//surface_set_color_format(surface, color_format);
				return surface;
			}
		}
	}

	memset(&CreateSurface, 0, sizeof(CreateSurface));
	CreateSurface.format = color_format;
	CreateSurface.width  = rect->w;
	CreateSurface.height = rect->h;
	CreateSurface.mode   = GX_SURFACE_MODE_IMAGE;
	CreateSurface.buffer = surface->sf.buffer;

	ret |= GxAVGetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_CreateSurface,
			&CreateSurface,
			sizeof(GxVpuProperty_CreateSurface));

	if(ret != 0) {
		gxlogd("[GUI]Cannot create surface : %d\n", ret);
		GxCore_Free(surface);
		return (NULL);
	}

	surface->hw = KSURFACE;
	surface->hw_surface = CreateSurface.surface;
	if (surface->sf.buffer == 0) {
		surface->sf.buffer = CreateSurface.buffer;
		surface->data = (uint8_t *)GxCore_Map(g_device_handle, (uint32_t)surface->sf.buffer, surface->buffer_size);
	}

	return surface;
}

#endif /*NO_OS*/

GuiSurface *hd_get_idle_surface(GuiSurface *surface)
{
#ifndef NO_OS
	if(!gui.config.enable_double_buffer)
		return (surface);

	if((surface != screen) && (surface != back_screen))
		return (surface);

	if(surface == screen)
	{
		int ret = 0;
		GAL_Rect rect = {0};
		GuiSurface tmp_surface;
		GxVpuProperty_GetIdleSurface IdleSurface = {0};

		IdleSurface.layer = GX_LAYER_OSD;
		ret = GxAVGetProperty(g_device_handle,
							  vpu_handle,
							  GxVpuPropertyID_GetIdleSurface,
							  &IdleSurface,
							  sizeof(GxVpuProperty_GetIdleSurface));
		if(0 != ret)
		{
			gxlogd("[GUI]Get Idle Surface Failed!\n");
		}

		idle_surface = IdleSurface.idle_surface;
		if(back_screen->hw_surface == IdleSurface.idle_surface)
		{
			rect.x = rect.y = 0;
			rect.w = screen->sf.width;
			rect.h = screen->sf.height;
			ret |= hd_copy_surface0(surface, &rect, back_screen, &rect);

			tmp_surface = *screen;
			*screen = *back_screen;
			*back_screen = tmp_surface;
		}

		return (surface);
	}
#endif /*NO_OS*/
	return (surface);
}

GuiSurface *hd_get_surface(GAL_Rect *rect, int bpp, void *buffer, int force)
{
	GuiSurface *surface;
	int color_format;

	if (rect == NULL)
		return NULL;

	if (rect->w == 0 || rect->h == 0) {
		gxlogd("%s error: width: %d, height: %d\n", __func__, rect->w, rect->h);
		return NULL;
	}

	color_format = get_color_format(0, bpp);
	surface = hd_get_surface0(rect, bpp, color_format, buffer, force);
	return surface;
}

static int osd_enable;

static quefr_t *g_link_job = NULL;
static gx_stack_t *job_stack = NULL;

int hd_enable_osd(int flag)
{
	int ret = 0, current_state = 0;
#ifdef NO_OS
	GxVpuProperty_LayerEnable property;
#endif /*NO_OS*/

#ifndef NO_OS
	current_state = GxAvdev_GetLayerEnable(g_device_handle, vpu_handle, GX_LAYER_OSD);
#else
	memset(&property, 0, sizeof(property));
	property.layer = GX_LAYER_OSD;

	ret = GxAVGetProperty(g_device_handle,
						  vpu_handle,
						  GxVpuPropertyID_LayerEnable,
						  &property,
						  sizeof(GxVpuProperty_LayerEnable));
	current_state = property.enable;
#endif /*NO_OS*/

	if((osd_enable == flag) && (current_state == flag))
		return ret;

#ifndef NO_OS
	ret = GxAvdev_LayerEnable(g_device_handle, vpu_handle, GX_LAYER_OSD, flag);
#else
	memset(&property, 0, sizeof(property));
	property.layer = GX_LAYER_OSD;
	property.enable = flag ? GUI_TRUE : GUI_FALSE;

	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_LayerEnable,
			&property,
			sizeof(GxVpuProperty_LayerEnable));
#endif /*NO_OS*/
	if(0 != ret) {
		gxlogd("[GUI]Set OSD enable failed!\n");
		return (1);
	}
	osd_enable = flag;

	return (ret);
}

int hd_new_blit_list(void)
{
	GxVpuProperty_BeginUpdate begin = {0};
	int ret = 0, count = 0, total = 0;

	if(GUI_FALSE == gui.config.enable_ga)
	{
		return (0);
	}

	if(NULL == job_stack)
	{
		job_stack = stack_new(NULL);
		if(NULL == job_stack)
		{
			return (1);
		}
	}

#ifndef NO_OS
	if(-1 == blit_mutex)
	{
		GxCore_MutexCreate(&blit_mutex);
	}
#endif

	if(NULL == g_link_job)
	{
		g_link_job = linklist_new(g_link_job);
		if(NULL == g_link_job)
		{
			return (1);
		}

		gui.config.job_link = g_link_job;
	}

	count = stack_count(job_stack);

#ifndef NO_OS
	GxCore_MutexLock(blit_mutex);
#endif
	if(linklist_get_count(g_link_job))
	{
		ret = stack_push(job_stack, linklist_get_front(g_link_job));
		ret = stack_push(job_stack, linklist_get_rear(g_link_job));
		ret = stack_push(job_stack, (void *)linklist_get_count(g_link_job));

		linklist_clear(g_link_job);
	}
	else if(count)
	{
		ret = stack_push(job_stack, 0);
		ret = stack_push(job_stack, 0);
		ret = stack_push(job_stack, 0);
	}
#ifndef NO_OS
	GxCore_MutexUnlock(blit_mutex);
#endif

	if(gui.config.ga_count)
	{
	    total = gui.config.ga_count;
	}
	else
	{
	    total = MAX_BLIT_COUNT;
	}

	begin.max_job_num = total;

	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_BeginUpdate,
			&begin,
			sizeof(GxVpuProperty_BeginUpdate));

	if(0 != ret)
	{
		gxlogd("[GUI]Set OSD BLIT begin failed!\n");
		return (1);
	}

	return (ret);
}

int hd_commit_blit(void);
int hd_add_blit_element(GuiSurface *surface)
{
	int total = 0;

	if(GUI_FALSE == gui.config.enable_ga)
	{
		if(surface)
		{
		    hd_free_surface(surface);
		}
		return (0);
	}

#ifndef NO_OS
	if(-1 == blit_mutex)
	{
		GxCore_MutexCreate(&blit_mutex);
	}
	GxCore_MutexLock(blit_mutex);
#endif
	if(NULL == g_link_job)
	{
		g_link_job = linklist_new(g_link_job);
		if(NULL == g_link_job)
		{
#ifndef NO_OS
			GxCore_MutexUnlock(blit_mutex);
#endif
			return (1);
		}

		gui.config.job_link = g_link_job;
	}

	linklist_insert(g_link_job, surface);

	if(gui.config.ga_count)
	{
	    total = gui.config.ga_count;
	}
	else
	{
	    total = MAX_BLIT_COUNT;
	}

#ifndef NO_OS
	GxCore_MutexUnlock(blit_mutex);
#endif
	if(total <= linklist_get_count(g_link_job))
	{
	    hd_commit_blit();
	}

	return (0);
}

static void _free_surface(void *data)
{
	if(data)
	{
		hd_free_surface(data);
	}
}

int hd_end_blit_list(void)
{
	GxVpuProperty_EndUpdate end = {0};
	int ret = 0, count = 0, total = 0;

	if(GUI_FALSE == gui.config.enable_ga)
	{
		return (0);
	}

	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_EndUpdate,
			&end,
			sizeof(GxVpuProperty_EndUpdate));

	if(0 != ret)
	{
		gxlogd("[GUI]Set OSD BLIT end failed!\n");
	}

	if(NULL == g_link_job)
	{
		g_link_job = linklist_new(g_link_job);
		if(NULL == g_link_job)
		{
			return (1);
		}

		gui.config.job_link = g_link_job;
	}

#ifndef NO_OS
	if(-1 == blit_mutex)
	{
		ret = GxCore_MutexCreate(&blit_mutex);
	}
	GxCore_MutexLock(blit_mutex);
#endif
	count = linklist_get_count(g_link_job);
	if(gui.config.ga_count)
	{
	    total = gui.config.ga_count;
	}
	else
	{
	    total = MAX_BLIT_COUNT;
	}

	if((0 == count) || (total < count))
	{
		if(NULL == job_stack)
		{
			job_stack = stack_new(NULL);
			if(NULL == job_stack)
			{
#ifndef NO_OS
				GxCore_MutexUnlock(blit_mutex);
#endif
				return (1);
			}
		}

		count = stack_count(job_stack);

		if(count)
		{
			linklist_set_count(g_link_job, (int)stack_pop(job_stack));
			linklist_set_rear(g_link_job, (quenode_t *)stack_pop(job_stack));
			linklist_set_front(g_link_job, (quenode_t *)stack_pop(job_stack));
		}
#ifndef NO_OS
		GxCore_MutexUnlock(blit_mutex);
#endif
		return (0);
	}


	linklist_delete_all(g_link_job, _free_surface);
	linklist_clear(g_link_job);

	if(NULL == job_stack)
	{
		job_stack = stack_new(NULL);
		if(NULL == job_stack)
		{
#ifndef NO_OS
			GxCore_MutexUnlock(blit_mutex);
#endif
			return (1);
		}
	}

	count = stack_count(job_stack);

	if(count)
	{
		linklist_set_count(g_link_job, (int)stack_pop(job_stack));
		linklist_set_rear(g_link_job, (quenode_t *)stack_pop(job_stack));
		linklist_set_front(g_link_job, (quenode_t *)stack_pop(job_stack));
	}
#ifndef NO_OS
	GxCore_MutexUnlock(blit_mutex);
#endif

	return (ret);
}

int hd_commit_blit(void)
{
	int ret = 0;

	ret = hd_end_blit_list();
#ifndef NO_OS
	image_cache_clean(gui.config.image_cache);
#endif
	ret |= hd_new_blit_list();
	if(!gui.config.enable_double_buffer)
		ret |= hd_enable_osd(GUI_TRUE);

	return (ret);
}


int hd_update_blit(void)
{
	int ret = 0, total = 0;
	GxVpuProperty_EndUpdate end = {0};
	GxVpuProperty_BeginUpdate begin = {0};

#ifndef NO_OS
	if(-1 == blit_mutex)
	{
		ret = GxCore_MutexCreate(&blit_mutex);
	}

	GxCore_MutexLock(blit_mutex);
#endif

	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_EndUpdate,
			&end,
			sizeof(GxVpuProperty_EndUpdate));

	if(gui.config.ga_count)
	{
	    total = gui.config.ga_count;
	}
	else
	{
	    total = MAX_BLIT_COUNT;
	}

	begin.max_job_num = total;

	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_BeginUpdate,
			&begin,
			sizeof(GxVpuProperty_BeginUpdate));

	ret = hd_enable_osd(GUI_TRUE);

#ifndef NO_OS
	GxCore_MutexUnlock(blit_mutex);
#endif

	return (ret);
}

static void _delete_from_clone_list(GuiSurface *source, GuiSurface *cloned)
{
	GuiSurface *surface = source;
	while(surface) {
		if(surface->next_cloned == cloned) {
			surface->next_cloned = cloned->next_cloned;
			cloned->next_cloned = NULL;
			break;
		}
		surface = surface->next_cloned;
	}
}

#ifndef NO_OS
static void _print_from_clone_list(GuiSurface *source)
{
	GuiSurface *surface = source;
	gxloge("--------------------------------------------------------------");
	while(surface) {
		if(surface->next_cloned) {
			gxloge("surface address = 0x%x,", surface->next_cloned);
			gxloge("--------------------------------------------------------------");
		}
		surface = surface->next_cloned;
	}
	printf("\n");
}
#endif

void hd_free_surface(GuiSurface *surface)
{
	GuiCore *gui_core = NULL;
	int ret;

	gui_core = GUI_GetCurrent();
	if (surface == NULL)
		return;

#ifndef NO_OS
	if ((surface->source != NULL) && (!is_ksurface(surface))) {
		_delete_from_clone_list(surface->source, surface);
		GxCore_Free(surface);
		return;
	}
	if((surface->next_cloned) &&
	   ((surface->source == NULL) ||
	    (surface->source == surface))) {
		gxloge("The cloned surface:");
		_print_from_clone_list(surface);
		gxloge("is not deleted, please check!\n");
		return;
	}
#endif
	if (surface->hw & KSURFACE) {
		if (!(surface->hw & VBUFFER))
			GxCore_UnMap(g_device_handle, surface->data, surface->buffer_size);
		GxVpuProperty_DestroySurface DesSurface = { 0 };

		memset(&DesSurface, 0, sizeof(DesSurface));
		DesSurface.surface = surface->hw_surface;

		ret = GxAVSetProperty(g_device_handle,
				vpu_handle,
				GxVpuPropertyID_DestroySurface,
				&DesSurface,
				sizeof(GxVpuProperty_DestroySurface));
		if(0 != ret)
			gxlogd("[GUI]Set OSD Destroy surface failed!\n");
		else if((surface->vq) && (0 == surface->vq->id)){
			gui_core->fb.overflow_size -= surface->vq->size;
		}
	}

	if (surface->hw & VBUFFER && surface->source == NULL) {
		hd_free(surface->vq);
		GUI_FREE(surface->vq);
#ifndef NO_OS
		image_cache_clr_data(gui.config.image_cache, surface); // REMOVE LIST
#endif
	} else if(surface->vq){
		hd_free(surface->vq);
	}
	if(surface->source)
		_delete_from_clone_list(surface->source, surface);
	GxCore_Free(surface);
}

int hd_check_surface(GuiSurface *surface)
{
#ifdef GXDBG
#ifndef NO_OS
	if (is_fb_memory(&gui.fb, surface->data) == 1) {
		hd_check(surface->vq);
	}
#endif /*NO_OS*/
#endif /*GXDBG*/

	return 0;
}

int hd_tile_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect)
{
	int ret = 0, color = 0;
	GuiCore *gui_core = GUI_GetCurrent();
	GAL_Rect tmp_rect = {0};
	GuiSurface *tmp_surface = NULL, *psrc = NULL;
	GxVpuProperty_Blit Blit;

	if((NULL == src) ||
	   (NULL == src_rect) ||
	   (NULL == dst) ||
	   (NULL == dst_rect)) {
		return (1);
	}

	if(24 == src->bpp) {
		tmp_rect.w = src_rect->w;
		tmp_rect.h = src_rect->h;
		psrc = hd_get_surface(&tmp_rect, 32, NULL, GUI_FALSE);
		if(NULL == psrc) {
			return (1);
		}
		hd_copy_surface(src, &tmp_rect, psrc, &tmp_rect);
	} else {
		psrc = src;
	}

	if((src_rect->w < 0) ||
	   (src_rect->h < 0) ||
	   (dst_rect->w < 0) ||
	   (dst_rect->h < 0)) {
		return (1);
	}

	if(NULL == gui_core) {
		return (1);
	}

	color = gui_core->config.gui_trans;

	tmp_rect.x = tmp_rect.y = 0;
	tmp_rect.w = dst_rect->w;
	tmp_rect.h = dst_rect->h;
	tmp_surface = hd_get_surface(&tmp_rect, dst->bpp, NULL, GUI_TRUE);
	if(NULL == tmp_surface) {
		return (1);
	}

	memset(&Blit, 0, sizeof(GxVpuProperty_Blit));

	color = hd_color2index(tmp_surface, color);

	Blit.srca.dst_format  = psrc->sf.color_format;//get_color_format(0, gui.config.bpp);
	Blit.srca.surface     = psrc->hw_surface;
	Blit.srca.is_ksurface = is_ksurface(psrc);
	Blit.srca.alpha       = GUI_TRUE;
	Blit.srca.rect.x      = src_rect->x;
	Blit.srca.rect.y      = src_rect->y;
	Blit.srca.rect.width  = src_rect->w;
	Blit.srca.rect.height = src_rect->h;

	Blit.srcb.surface     = tmp_surface->hw_surface;
	Blit.srcb.dst_format  = tmp_surface->sf.color_format;//get_color_format(0, gui.config.bpp);
	Blit.srcb.is_ksurface = is_ksurface(tmp_surface);
	Blit.srcb.rect        = Blit.srca.rect;
	Blit.srcb.rect.x      = 0;//dst_rect->x;
	Blit.srcb.rect.y      = 0;//dst_rect->y;
	Blit.srcb.rect.width  = dst_rect->w;
	Blit.srcb.rect.height = dst_rect->h;

	Blit.dst = Blit.srcb;
	Blit.mode = GX_ALU_PAVE;
	Blit.colorkey_info.mode = GX_BLIT_COLORKEY_BASIC_MODE;
	Blit.dst.alpha = gui.config.osd_alpha;
	Blit.colorkey_info.src_colorkey_en = GUI_FALSE;

	// FIXME:
	hd_check_surface(psrc);
	hd_check_surface(dst);

	hd_new_blit_list();
	hd_fillrect(tmp_surface, &tmp_rect, color);
	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_Blit,
			&Blit,
			sizeof(GxVpuProperty_Blit));
	if(0 != ret)
	{
		gxlogd("[GUI]----------Tile Failed-----------\n");
	}
	hd_add_blit_element(NULL);
	hd_end_blit_list();

	if((32 == psrc->bpp) && (32 == tmp_surface->bpp)) {
		tmp_surface->alpha = GUI_TRUE;
	}
	if(psrc != src) {
		hd_new_blit_list();
	}
	ret |= hd_stretch_surface(tmp_surface, &tmp_rect, dst, dst_rect);

	hd_add_blit_element(tmp_surface);
	if(psrc != src) {
		hd_end_blit_list();
	}

	// FIXME:
	hd_check_surface(psrc);
	hd_check_surface(dst);

	if(psrc != src) {
		hd_free_surface(psrc);
	}

	return (ret);
}

int hd_copy_surface1(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect)
{
	int ret = 0;
	GxVpuProperty_Blit Blit;

	memset(&Blit, 0, sizeof(GxVpuProperty_Blit));

	Blit.srca.dst_format  = src->sf.color_format;//get_color_format(0, gui.config.bpp);
	Blit.srca.surface     = src->hw_surface;
	Blit.srca.is_ksurface = is_ksurface(src);
	Blit.srca.alpha       = GUI_TRUE;
	Blit.srca.rect.x      = src_rect->x;
	Blit.srca.rect.y      = src_rect->y;
	Blit.srca.rect.width  = src_rect->w;
	Blit.srca.rect.height = src_rect->h;

	Blit.srcb.surface     = dst->hw_surface;
	Blit.srcb.dst_format  = dst->sf.color_format;//get_color_format(0, gui.config.bpp);
	Blit.srcb.is_ksurface = is_ksurface(dst);
	Blit.srcb.rect        = Blit.srca.rect;
	Blit.srcb.rect.x      = dst_rect->x;
	Blit.srcb.rect.y      = dst_rect->y;

	Blit.dst = Blit.srcb;
	Blit.mode = GX_ALU_ROP_COPY;
	Blit.colorkey_info.mode = GX_BLIT_COLORKEY_BASIC_MODE;
	Blit.dst.alpha = gui.config.osd_alpha;
	Blit.colorkey_info.src_colorkey_en = GUI_FALSE;

	if((src->alpha) && (0xFF != src->alpha))
	{
		Blit.mode = GX_ALU_MIX_TRUE;

		Blit.srca.modulator.premultiply_en = GUI_TRUE;
		Blit.srca.modulator.step1_en = GUI_TRUE;
		Blit.srca.modulator.step1_mode = MODULAT_CHANNEL_ALPHA_WITH_REGCOLOR_ALPHA;
		Blit.srca.modulator.reg_color = 0xFF;

		Blit.srcb.modulator.premultiply_en = GUI_TRUE;
		Blit.srcb.modulator.step1_en = GUI_TRUE;
		Blit.srcb.modulator.step1_mode = MODULAT_CHANNEL_ALPHA_WITH_REGCOLOR_ALPHA;
		Blit.srcb.modulator.reg_color = 0xFF;

		Blit.dst.modulator.premultiply_en = GUI_TRUE;
		Blit.dst.modulator.step1_en = GUI_TRUE;
		Blit.dst.modulator.step1_mode = MODULAT_CHANNEL_ALPHA_WITH_REGCOLOR_ALPHA;
		Blit.dst.modulator.reg_color = 0xFF;
		Blit.srca.alpha_en	= GUI_TRUE;
		Blit.srca.alpha		= src->alpha;
	}

	// FIXME:
	hd_check_surface(src);
	hd_check_surface(dst);

	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_Blit,
			&Blit,
			sizeof(GxVpuProperty_Blit));
	if(0 != ret)
	{
		gxlogd("[GUI]----------Copy Failed-----------\n");
	}

	// FIXME:
	hd_check_surface(src);
	hd_check_surface(dst);

	return (ret);
}

static int hd_copy_surface0(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect)
{
	int ret = 0;
	//GxVpuProperty_BeginUpdate Begin = {0};
	//GxVpuProperty_EndUpdate End = {0};

	if(src == NULL || src_rect == NULL || dst == NULL || dst_rect == NULL)
		return 1;

	if((src_rect->w <= 0) || (src_rect->h) <= 0)
		return 1;

	/*ret |= hd_stretch_surface(src, src_rect, dst, dst_rect);*/
	hd_new_blit_list();
	ret = hd_copy_surface1(src, src_rect, dst, dst_rect);
	hd_add_blit_element(NULL);

	hd_end_blit_list();

	return (ret);
}

int hd_copy_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect)
{
	if(src == NULL || src_rect == NULL || dst == NULL || dst_rect == NULL)
		return 1;

	if((src_rect->w <= 0) || (src_rect->h) <= 0)
		return 1;

	dst = hd_get_idle_surface(dst);
	return (hd_copy_surface0(src, src_rect, dst, dst_rect));

}

int hd_stretch_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect)
{
	int ret = 0, color = 0;;
	GxVpuProperty_Blit Blit;
	GxVpuProperty_ColorKey ColorKey = { 0 };

	if (src == NULL || src_rect == NULL || dst == NULL || dst_rect == NULL)
		return 1;

	if (src_rect->w == 0 || src_rect->h == 0 || dst_rect->w == 0 || dst_rect->h == 0)
		return 1;

	dst = hd_get_idle_surface(dst);

	ColorKey.enable = 1;
	color = gal_get_gui_transparent();
	color = gal_color2index(src, color);
	hd_color2formatcolor(dst, color, &(ColorKey.color));

	memset(&Blit, 0, sizeof(GxVpuProperty_Blit));

	Blit.srca.dst_format  = src->sf.color_format;//get_color_format(0, gui.config.bpp);
	Blit.srca.surface     = src->hw_surface;
	Blit.srca.is_ksurface = is_ksurface(src);
	Blit.srca.rect.x      = src_rect->x;
	Blit.srca.rect.y      = src_rect->y;
	Blit.srca.rect.width  = src_rect->w;
	Blit.srca.rect.height = src_rect->h;

	Blit.srcb.dst_format  = dst->sf.color_format;//get_color_format(0, gui.config.bpp);
	Blit.srcb.surface     = dst->hw_surface;
	Blit.srcb.is_ksurface = is_ksurface(dst);
	Blit.srcb.rect        = Blit.srca.rect;
	Blit.srcb.rect.x      = dst_rect->x;
	Blit.srcb.rect.y      = dst_rect->y;

	Blit.dst                           = Blit.srcb;
	Blit.mode                          = GX_ALU_ROP_COPY;
	Blit.colorkey_info.mode            = GX_BLIT_COLORKEY_BASIC_MODE;
	Blit.colorkey_info.src_colorkey_en = GUI_TRUE;

	Blit.colorkey_info.src_colorkey = color;

	if(src->alpha)
	{
		if((GX_COLOR_FMT_ARGB4444 == src->sf.color_format) ||
				(GX_COLOR_FMT_RGBA4444 == src->sf.color_format) ||
				(GX_COLOR_FMT_ARGB1555 == src->sf.color_format)) {
			Blit.colorkey_info.dst_colorkey_en = GUI_FALSE;
		} else {
			Blit.colorkey_info.dst_colorkey_en = GUI_TRUE;
		}
		Blit.colorkey_info.dst_colorkey = gui.config.osd_trans;
		Blit.colorkey_info.src_colorkey_en = GUI_FALSE;
		Blit.mode = GX_ALU_MIX_TRUE;

		Blit.srca.modulator.premultiply_en = GUI_TRUE;
		Blit.srca.modulator.step1_en = GUI_TRUE;
		Blit.srca.modulator.step1_mode = MODULAT_CHANNEL_ALPHA_WITH_REGCOLOR_ALPHA;
		Blit.srca.modulator.reg_color = 0xFF;

		Blit.srcb.modulator.premultiply_en = GUI_TRUE;
		Blit.srcb.modulator.step1_en = GUI_TRUE;
		Blit.srcb.modulator.step1_mode = MODULAT_CHANNEL_ALPHA_WITH_REGCOLOR_ALPHA;
		Blit.srcb.modulator.reg_color = 0xFF;

		Blit.dst.modulator.premultiply_en = GUI_TRUE;
		Blit.dst.modulator.step1_en = GUI_TRUE;
		Blit.dst.modulator.step1_mode = MODULAT_CHANNEL_ALPHA_WITH_REGCOLOR_ALPHA;
		Blit.dst.modulator.reg_color = 0xFF;

		Blit.srca.alpha_en = GUI_TRUE;
		Blit.srca.alpha = src->alpha;
		Blit.srcb.alpha = 0xFF;
	}

	hd_check_surface(src);
	hd_check_surface(dst);
	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_Blit,
			&Blit,
			sizeof(GxVpuProperty_Blit));

	if(src->bpp == 32)
	{
		hd_add_blit_element(NULL);

		memset(&Blit, 0, sizeof(GxVpuProperty_Blit));

		Blit.srca.dst_format  = src->sf.color_format;//get_color_format(0, gui.config.bpp);
		Blit.srca.surface     = src->hw_surface;
		Blit.srca.is_ksurface = is_ksurface(src);
		Blit.srca.rect.x      = src_rect->x;
		Blit.srca.rect.y      = src_rect->y;
		Blit.srca.rect.width  = src_rect->w;
		Blit.srca.rect.height = src_rect->h;

		Blit.srcb.dst_format  = dst->sf.color_format;//get_color_format(0, gui.config.bpp);
		Blit.srcb.surface     = dst->hw_surface;
		Blit.srcb.is_ksurface = is_ksurface(dst);
		Blit.srcb.rect        = Blit.srca.rect;
		Blit.srcb.rect.x      = dst_rect->x;
		Blit.srcb.rect.y      = dst_rect->y;

		Blit.dst                           = Blit.srcb;
		Blit.mode                          = GX_ALU_ROP_COPY;
		Blit.colorkey_info.mode            = GX_BLIT_COLORKEY_BASIC_MODE;
		Blit.colorkey_info.src_colorkey_en = GUI_TRUE;

		Blit.colorkey_info.src_colorkey = color;

		if(src->alpha)
		{
			Blit.colorkey_info.src_colorkey_en = GUI_FALSE;
			Blit.mode = GX_ALU_MIX_TRUE;

			Blit.srca.modulator.premultiply_en = GUI_TRUE;
			Blit.srca.modulator.step1_en = GUI_TRUE;
			Blit.srca.modulator.step1_mode = MODULAT_CHANNEL_ALPHA_WITH_REGCOLOR_ALPHA;
			Blit.srca.modulator.reg_color = 0xFF;

			Blit.srcb.modulator.premultiply_en = GUI_TRUE;
			Blit.srcb.modulator.step1_en = GUI_TRUE;
			Blit.srcb.modulator.step1_mode = MODULAT_CHANNEL_ALPHA_WITH_REGCOLOR_ALPHA;
			Blit.srcb.modulator.reg_color = 0xFF;

			Blit.dst.modulator.premultiply_en = GUI_TRUE;
			Blit.dst.modulator.step1_en = GUI_TRUE;
			Blit.dst.modulator.step1_mode = MODULAT_CHANNEL_ALPHA_WITH_REGCOLOR_ALPHA;
			Blit.dst.modulator.reg_color = 0xFF;

			Blit.srca.alpha_en = GUI_TRUE;
			Blit.srca.alpha = src->alpha;
			Blit.srcb.alpha = 0xFF;
		}

		hd_check_surface(src);
		hd_check_surface(dst);
		ret = GxAVSetProperty(g_device_handle,
				vpu_handle,
				GxVpuPropertyID_Blit,
				&Blit,
				sizeof(GxVpuProperty_Blit));
	}
	hd_check_surface(src);
	hd_check_surface(dst);

	return (ret);
}

int hd_stretch_surface1(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect)
{
	int ret = 0, color = 0;;
	GxVpuProperty_Blit Blit;
	GxVpuProperty_ColorKey ColorKey = { 0 };

	if (src == NULL || src_rect == NULL || dst == NULL || dst_rect == NULL)
		return 1;

	if (src_rect->w == 0 || src_rect->h == 0 || dst_rect->w == 0 || dst_rect->h == 0)
		return 1;

	dst = hd_get_idle_surface(dst);

	ColorKey.enable = 1;
	color = gal_get_gui_transparent();
	color = gal_color2index(src, color);
	hd_color2formatcolor(dst, color, &(ColorKey.color));

	memset(&Blit, 0, sizeof(GxVpuProperty_Blit));

	Blit.srca.dst_format  = src->sf.color_format;//get_color_format(0, gui.config.bpp);
	Blit.srca.surface     = src->hw_surface;
	Blit.srca.is_ksurface = is_ksurface(src);
	Blit.srca.rect.x      = src_rect->x;
	Blit.srca.rect.y      = src_rect->y;
	Blit.srca.rect.width  = src_rect->w;
	Blit.srca.rect.height = src_rect->h;

	Blit.srcb.dst_format  = dst->sf.color_format;//get_color_format(0, gui.config.bpp);
	Blit.srcb.surface     = dst->hw_surface;
	Blit.srcb.is_ksurface = is_ksurface(dst);
	Blit.srcb.rect        = Blit.srca.rect;
	Blit.srcb.rect.x      = dst_rect->x;
	Blit.srcb.rect.y      = dst_rect->y;
	Blit.srcb.rect.width  = dst_rect->w;
	Blit.srcb.rect.height = dst_rect->h;

	Blit.dst                           = Blit.srcb;
	Blit.mode                          = GX_ALU_ROP_COPY;
	Blit.colorkey_info.mode            = GX_BLIT_COLORKEY_BASIC_MODE;
	Blit.colorkey_info.src_colorkey_en = GUI_FALSE;//GUI_TRUE;

	Blit.colorkey_info.src_colorkey = color;

	if(src->alpha)
	{
		Blit.colorkey_info.dst_colorkey = gui.config.osd_trans;
		Blit.colorkey_info.src_colorkey_en = GUI_FALSE;
		Blit.mode = GX_ALU_MIX_TRUE;

		Blit.srca.modulator.premultiply_en = GUI_TRUE;
		Blit.srca.modulator.step1_en = GUI_TRUE;
		Blit.srca.modulator.step1_mode = MODULAT_CHANNEL_ALPHA_WITH_REGCOLOR_ALPHA;
		Blit.srca.modulator.reg_color = 0xFF;

		Blit.srcb.modulator.premultiply_en = GUI_TRUE;
		Blit.srcb.modulator.step1_en = GUI_TRUE;
		Blit.srcb.modulator.step1_mode = MODULAT_CHANNEL_ALPHA_WITH_REGCOLOR_ALPHA;
		Blit.srcb.modulator.reg_color = 0xFF;

		Blit.dst.modulator.premultiply_en = GUI_TRUE;
		Blit.dst.modulator.step1_en = GUI_TRUE;
		Blit.dst.modulator.step1_mode = MODULAT_CHANNEL_ALPHA_WITH_REGCOLOR_ALPHA;
		Blit.dst.modulator.reg_color = 0xFF;

		Blit.srca.alpha_en = GUI_TRUE;
		Blit.srca.alpha = src->alpha;
		Blit.srcb.alpha = 0xFF;
	}

	hd_check_surface(src);
	hd_check_surface(dst);
	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_Blit,
			&Blit,
			sizeof(GxVpuProperty_Blit));
	hd_add_blit_element(NULL);
	hd_check_surface(src);
	hd_check_surface(dst);

	return (ret);
}

int hd_copy_surface_gradiant(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect, ImagePlayMode mode, unsigned int steps)
{
	int ret = 0;
#ifndef NO_OS
	int i = 0, j = 0, step_width = 0, step_height = 0;
	GAL_Rect step_rect = {0};

	if((NULL == src) || (NULL == src_rect) || (NULL == dst) || (NULL == dst_rect))
		return (-1);
	if((src_rect->x != dst_rect->x) ||
	   (src_rect->y != dst_rect->y) ||
	   (src_rect->w != dst_rect->w) ||
	   (src_rect->h != dst_rect->h))
		return (-1);

	switch(mode) {
		case IMAGE_SLIDE_LEFT:
			step_width = 2 * ((dst->sf.width / steps) / 2);
			step_height = dst->sf.height;
			step_rect.x = src_rect->w - step_width;
			step_rect.y = 0;
			step_rect.w = step_width;
			step_rect.h = step_height;
			for(i = 0; i < steps; i++) {
				hd_copy_surface(src, &step_rect, dst, &step_rect);
				gui_delay(20);
				if((step_rect.x - step_width) >= 0)
					step_rect.x -= step_width;
				else
					step_rect.x = 0;
			}
			hd_copy_surface(src, src_rect, dst, dst_rect);
			break;
		case IMAGE_SLIDE_RIGHT:
			step_width = 2 * ((dst->sf.width / steps) / 2);
			step_height = dst->sf.height;
			step_rect.x = 0;
			step_rect.y = 0;
			step_rect.w = step_width;
			step_rect.h = step_height;
			for(i = 0; i < steps; i++) {
				hd_copy_surface(src, &step_rect, dst, &step_rect);
				gui_delay(20);
				if((step_rect.x + step_width) <= dst->sf.width)
					step_rect.x += step_width;
				else
					step_rect.x = dst->sf.width - step_width;
			}
			hd_copy_surface(src, dst_rect, dst, dst_rect);
			break;
		case IMAGE_SLIDE_TOP:
			step_width = dst->sf.width;
			step_height = 2 * ((dst->sf.height / steps) / 2);
			step_rect.x = 0;
			step_rect.y = dst->sf.height - step_height;
			step_rect.w = step_width;
			step_rect.h = step_height;
			for(i = 0; i < steps; i++) {
				hd_copy_surface(src, &step_rect, dst, &step_rect);
				gui_delay(20);
				if((step_rect.y - step_height) >= 0)
					step_rect.y -= step_height;
				else
					step_rect.y = 0;
			}
			hd_copy_surface(src, src_rect, dst, dst_rect);
			break;
		case IMAGE_SLIDE_BOTTOM:
			step_width = dst->sf.width;
			step_height = 2 * ((dst->sf.height / steps) / 2);
			step_rect.x = 0;
			step_rect.y = 0;
			step_rect.w = step_width;
			step_rect.h = step_height;
			for(i = 0; i < steps; i++) {
				hd_copy_surface(src, &step_rect, dst, &step_rect);
				gui_delay(20);
				if((step_rect.y + step_height) <= dst->sf.height)
					step_rect.y += step_height;
				else
					step_rect.y = dst->sf.height - step_height;
			}
			hd_copy_surface(src, src_rect, dst, dst_rect);
			break;
		case IMAGE_DIAGONAL_TOP_LEFT:
			step_width = 2 * ((dst->sf.width / steps) / 2);
			step_height = 2 * ((dst->sf.height / steps) / 2);
			step_rect.x = dst->sf.width - step_width;
			step_rect.y = dst->sf.height - step_height;
			step_rect.w = step_width;
			step_rect.h = step_height;
			for(i = 0; i < steps; i++) {
				hd_copy_surface(src, &step_rect, dst, &step_rect);
				gui_delay(20);
				if((step_rect.y - step_height) >= 0) {
					step_rect.y -= step_height;
					step_rect.h += step_height;
				} else {
					step_rect.y = 0;
				}
				if((step_rect.x - step_width) >= 0) {
					step_rect.x -= step_width;
					step_rect.w += step_width;
				} else {
					step_rect.x = 0;
				}
			}
			hd_copy_surface(src, src_rect, dst, dst_rect);
			break;
		case IMAGE_DIAGONAL_TOP_RIGHT:
			step_width = 2 * ((dst->sf.width / steps) / 2);
			step_height = 2 * ((dst->sf.height / steps) / 2);
			step_rect.x = 0;
			step_rect.y = dst->sf.height - step_height;
			step_rect.w = step_width;
			step_rect.h = step_height;
			for(i = 0; i < steps; i++) {
				hd_copy_surface(src, &step_rect, dst, &step_rect);
				gui_delay(20);
				if((step_rect.y - step_height) >= 0) {
					step_rect.y -= step_height;
					step_rect.h += step_height;
				} else {
					step_rect.y = 0;
				}
				if((step_rect.x + step_width) <= dst->sf.width) {
					step_rect.w += step_width;
				} else {
					step_rect.x = dst->sf.width - step_width;;
				}
			}
			hd_copy_surface(src, src_rect, dst, dst_rect);
			break;
		case IMAGE_DIAGONAL_BOTTOM_LEFT:
			step_width = 2 * ((dst->sf.width / steps) / 2);
			step_height = 2 * ((dst->sf.height / steps) / 2);
			step_rect.x = dst->sf.width - step_width;
			step_rect.y = step_height;
			step_rect.w = step_width;
			step_rect.h = step_height;
			for(i = 0; i < steps; i++) {
				hd_copy_surface(src, &step_rect, dst, &step_rect);
				gui_delay(20);
				if((step_rect.y + step_height) <= dst->sf.height) {
					step_rect.h += step_height;
				} else {
					step_rect.y = dst->sf.height - step_height;
				}
				if((step_rect.x - step_width) >= 0) {
					step_rect.x -= step_width;
					step_rect.w += step_width;
				} else {
					step_rect.x = 0;
				}
			}
			hd_copy_surface(src, src_rect, dst, dst_rect);
			break;
		case IMAGE_DIAGONAL_BOTTOM_RIGHT:
			step_width = 2 * ((dst->sf.width / steps) / 2);
			step_height = 2 * ((dst->sf.height / steps) / 2);
			step_rect.x = 0;
			step_rect.y = 0;
			step_rect.w = step_width;
			step_rect.h = step_height;
			for(i = 0; i < steps; i++) {
				hd_copy_surface(src, &step_rect, dst, &step_rect);
				gui_delay(20);
				step_rect.w += step_width;
				step_rect.h += step_height;
			}
			hd_copy_surface(src, src_rect, dst, dst_rect);
			break;
		case IMAGE_HSTEP_TOP_LEFT:
			step_width = 2 * ((dst->sf.width / steps) / 2);
			step_height = dst->sf.height / steps;
			step_rect.w = step_width;
			step_rect.h = step_height;
			for(i = 1; i <= steps; i++) {
				step_rect.x = dst->sf.width - step_width * i;
				step_rect.y = dst->sf.height - step_height;
				for(j = 0; j < i; j++) {
					hd_copy_surface(src, &step_rect, dst, &step_rect);
					step_rect.x += step_width;
					step_rect.y -= step_height;
				}
				gui_delay(20);
			}
			for(i = (steps - 1); i > 0; i--) {
				step_rect.x = dst->sf.width - step_width * steps;
				if(step_rect.x < 0)
					step_rect.x = 0;
				step_rect.y = dst->sf.height - step_height * (steps - i + 1);
				for(j = 0; j < i; j++) {
					if(step_rect.y == (dst->sf.height % steps)) {
						step_rect.y = 0;
						step_rect.h = step_height + (dst->sf.height % steps);
					} else {
						step_rect.h = step_height;
					}
					hd_copy_surface(src, &step_rect, dst, &step_rect);
					step_rect.x += step_width;
					step_rect.y -= step_height;
				}
				gui_delay(20);
			}
			hd_copy_surface(src, src_rect, dst, dst_rect);
			break;
		case IMAGE_HSTEP_TOP_RIGHT:
			step_width = 2 * ((dst->sf.width / steps) / 2);
			step_height = (dst->sf.height - (dst->sf.height % steps)) / steps;
			step_rect.w = step_width;
			step_rect.h = step_height;
			for(i = 1; i <= steps; i++) {
				step_rect.x = step_width * (i - 1);
				step_rect.y = dst->sf.height - step_height;
				for(j = 0; j < i; j++) {
					hd_copy_surface(src, &step_rect, dst, &step_rect);
					step_rect.x -= step_width;
					step_rect.y -= step_height;
				}
				gui_delay(20);
			}
			for(i = (steps - 1); i > 0; i--) {
				step_rect.x = step_width * (steps - i);
				step_rect.y = 0;
				for(j = 0; j < i; j++) {
					if(step_rect.y == 0) {
						step_rect.h = step_height + dst->sf.height % steps;
					} else {
						step_rect.h = step_height;
					}
					hd_copy_surface(src, &step_rect, dst, &step_rect);
					step_rect.x += step_width;
					step_rect.y += step_rect.h;
				}
				gui_delay(20);
			}
			hd_copy_surface(src, src_rect, dst, dst_rect);
			break;
		case IMAGE_HSTEP_BOTTOM_LEFT:
			step_width = 2 * ((dst->sf.width / steps) / 2);
			step_height = dst->sf.height / steps;
			step_rect.w = step_width;
			step_rect.h = step_height;
			for(i = 1; i <= steps; i++) {
				step_rect.x = dst->sf.width - step_width * i;
				step_rect.y = 0;
				for(j = 0; j < i; j++) {
					hd_copy_surface(src, &step_rect, dst, &step_rect);
					step_rect.x += step_width;
					step_rect.y += step_height;
				}
				gui_delay(20);
			}
			for(i = (steps - 1); i > 0; i--) {
				step_rect.x = dst->sf.width - step_width * steps;
				if(step_rect.x < 0)
					step_rect.x = 0;
				step_rect.y = step_height * (steps - i);
				for(j = 0; j < i; j++) {
					if(step_rect.y == (dst->sf.height - step_height - (dst->sf.height % steps))) {
						step_rect.h = step_height + (dst->sf.height % steps);
					} else {
						step_rect.h = step_height;
					}
					hd_copy_surface(src, &step_rect, dst, &step_rect);
					step_rect.x += step_width;
					step_rect.y += step_height;
				}
				gui_delay(20);
			}
			hd_copy_surface(src, src_rect, dst, dst_rect);
			break;
		case IMAGE_HSTEP_BOTTOM_RIGHT:
			step_width = 2 * ((dst->sf.width / steps) / 2);
			step_height = (dst->sf.height - (dst->sf.height % steps)) / steps;
			step_rect.w = step_width;
			step_rect.h = step_height;
			for(i = 1; i <= steps; i++) {
				step_rect.x = (i - 1) * step_width;
				step_rect.y = 0;
				for(j = 0; j < i; j++) {
					hd_copy_surface(src, &step_rect, dst, &step_rect);
					step_rect.x -= step_width;
					step_rect.y += step_height;
				}
				gui_delay(20);
			}
			for(i = (steps - 1); i > 0; i--) {
				step_rect.x = step_width * (steps - i);
				step_rect.y = (dst->sf.height - (dst->sf.height % steps)) - step_height;
				for(j = 0; j < i; j++) {
					if(step_rect.y == (dst->sf.height - (dst->sf.height % steps)) - step_height) {
						step_rect.h = step_height + dst->sf.height % steps;
					} else {
						step_rect.h = step_height;
					}
					hd_copy_surface(src, &step_rect, dst, &step_rect);
					step_rect.x += step_width;
					step_rect.y -= step_height;
				}
				gui_delay(20);
			}
			hd_copy_surface(src, src_rect, dst, dst_rect);
			break;
		case IMAGE_CHECKER_BOARD:
			step_width = 2 * ((dst->sf.width / steps) / 2);
			step_height = dst->sf.height / steps;
			step_rect.w = step_width;
			step_rect.h = step_height;
			for(i = 0; i < steps; i++) {
				step_rect.x = 0;
				step_rect.y = i * step_height;
				for(j = 0; j < steps; j++) {
					step_rect.x = j * step_width;
					if((0 == (i % 2)) && (0 == (j % 2))) {
						hd_copy_surface(src, &step_rect, dst, &step_rect);
					} else if((i % 2) && (j % 2)) {
						hd_copy_surface(src, &step_rect, dst, &step_rect);
					}
				}
				gui_delay(20);
			}
			for(i = 0; i < steps; i++) {
				step_rect.x = 0;
				step_rect.y = i * step_height;
				for(j = 0; j < steps; j++) {
					step_rect.x = j * step_width;
					if((0 == i % 2) && (j % 2)) {
						hd_copy_surface(src, &step_rect, dst, &step_rect);
					} else if((i % 2) && (0 == (j % 2))) {
						hd_copy_surface(src, &step_rect, dst, &step_rect);
					}
				}
				gui_delay(20);
			}
			hd_copy_surface(src, src_rect, dst, dst_rect);
			break;
		case IMAGE_EFFECT_NONE:
		default:
			ret |= hd_copy_surface(src, src_rect, dst, dst_rect);
			break;
	}
#endif /* NO_OS */

	return (ret);
}

int hd_mirror_image_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect, GxReverseMode mode)
{
	int ret = 0;
	GxVpuProperty_TurnSurface TurnSurface = {0};

	if (src == NULL || src_rect == NULL || dst == NULL || dst_rect == NULL)
		return 1;

	if (src_rect->w == 0 || src_rect->h == 0 || dst_rect->w == 0 || dst_rect->h == 0)
		return 1;

	dst = hd_get_idle_surface(dst);

	TurnSurface.src_surface = src->hw_surface;
	TurnSurface.src_is_ksurface = is_ksurface(src);
	TurnSurface.src_rect.x = src_rect->x;
	TurnSurface.src_rect.y = src_rect->y;
	TurnSurface.src_rect.width = src_rect->w;
	TurnSurface.src_rect.height = src_rect->h;
	TurnSurface.dst_surface = dst->hw_surface;
	TurnSurface.dst_is_ksurface = is_ksurface(dst);
	TurnSurface.dst_rect.x = dst_rect->x;
	TurnSurface.dst_rect.y = dst_rect->y;
	TurnSurface.dst_rect.width = dst_rect->w;
	TurnSurface.dst_rect.height = dst_rect->h;
	TurnSurface.screw = SCREW_0;
	TurnSurface.reverse = mode;

	hd_check_surface(src);
	hd_check_surface(dst);
	hd_new_blit_list();
	ret = GxAVSetProperty(g_device_handle,
			      vpu_handle,
			      GxVpuPropertyID_TurnSurface,
			      &TurnSurface,
			      sizeof(GxVpuProperty_TurnSurface));
	hd_end_blit_list();

	hd_check_surface(src);
	hd_check_surface(dst);

	return (ret);
}

int hd_read_surface(GuiSurface *surface, unsigned int *width, unsigned int *height, void **buffer)
{
	if (surface == NULL)
		return 1;

	if (buffer)
		*buffer = surface->data;
	if (width)
		*width = surface->sf.width;
	if (height)
		*height = surface->sf.height;

	return 0;
}

int hd_zoom_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect)
{
    int ret = 0;
    unsigned hcoefficent = 0, vcoefficent = 0;
    GuiSurface *temp_surface = NULL, *new_surface = NULL, *swap_s_surface = NULL, *swap_d_surface = NULL;
	GuiSurface *dst_tmp_surface = NULL, *src_tmp_surface = NULL;
    GAL_Rect s_rect = {0}, tmp_rect = {0};
    GxVpuProperty_ZoomSurface zoom_surface = {0};

    if(src == NULL || src_rect == NULL || dst == NULL || dst_rect == NULL)
	return 1;

    if((src_rect->w <= 0) ||
       (src_rect->h <= 0) ||
       (dst_rect->w <= 0) ||
       (dst_rect->h <= 0))
	return 1;

    swap_d_surface = dst;
    swap_s_surface = src;
    if(SURFACE_FORMAT_ANY == src->format)
	hcoefficent = vcoefficent = 1;
    else
    {
	switch(src->format)
	{
	    case GX_COLOR_FMT_YCBCR420:
		hcoefficent = vcoefficent = 2;
		break;
	    case GX_COLOR_FMT_YCBCR422v:
		hcoefficent = 2;
		vcoefficent = 1;
		break;
	    case GX_COLOR_FMT_YCBCR422h:
		hcoefficent = 1;
		vcoefficent = 2;
		break;
	    case GX_COLOR_FMT_YCBCR400:
		hcoefficent = vcoefficent = 0;
		break;
	    default:
		hcoefficent = vcoefficent = 1;
		break;
	}
    }

    if(SURFACE_FORMAT_ANY == src->format)
    {
	if(GX_COLOR_FMT_YCBCR422 == src->sf.color_format)
	{
	    s_rect = *src_rect;
	    temp_surface = hd_get_surface(&s_rect, 16, NULL, GUI_TRUE);
	    if(NULL == temp_surface)
		return 1;

	    new_surface = temp_surface;
	    ret = hd_stretch_surface(src, src_rect, temp_surface, &s_rect);
	    if(ret)
		return 1;
	    src = temp_surface;
	}
	else
	    temp_surface = src;

	while(((src_rect->w > dst_rect->w) &&
		((src_rect->w / 4) > dst_rect->w)) ||
	   ((src_rect->h > dst_rect->h) &&
	    ((src_rect->h / 4) > dst_rect->h)))
	{
	    tmp_rect = *dst_rect;
		tmp_rect.x = tmp_rect.y = 0;
	    if((src_rect->w / 4) > dst_rect->w)
	    {
		tmp_rect.w = (src_rect->w / 4) + (src_rect->w % 4);
	    }

	    if((src_rect->h / 4) > dst_rect->h)
	    {
		tmp_rect.h = (src_rect->h / 4) + (src_rect->h % 4);
	    }
	    hd_zoom_surface(src, src_rect, src, &tmp_rect);
	    *src_rect = tmp_rect;
	    //temp_surface = src = dst;
	}

	hd_check_surface(src);
	hd_check_surface(dst);
	if(gui.config.enable_double_buffer)
	    gdi_commit();
	gdi_begin();
	zoom_surface.src_surface = temp_surface->hw_surface;
	zoom_surface.src_is_ksurface = is_ksurface(temp_surface);
	zoom_surface.src_rect.x = src_rect->x;
	zoom_surface.src_rect.y = src_rect->y;
	zoom_surface.src_rect.width = src_rect->w;
	zoom_surface.src_rect.height= src_rect->h;
	zoom_surface.dst_surface = dst->hw_surface;
	zoom_surface.dst_is_ksurface = is_ksurface(dst);
	if(dst != swap_d_surface)
	{
	    zoom_surface.dst_rect.x = 0;
	    zoom_surface.dst_rect.y = 0;
	}
	else
	{
	    zoom_surface.dst_rect.x = dst_rect->x;
	    zoom_surface.dst_rect.y = dst_rect->y;
	}
	zoom_surface.dst_rect.width = dst_rect->w;
	zoom_surface.dst_rect.height = dst_rect->h;
	ret = GxAVSetProperty(g_device_handle,
			      vpu_handle,
			      GxVpuPropertyID_ZoomSurface,
			      &zoom_surface,
			      sizeof(GxVpuProperty_ZoomSurface));
	gdi_end();
	hd_check_surface(src);
	hd_check_surface(dst);
	if(dst && swap_d_surface && (swap_d_surface != dst))
	{
		GAL_Rect rect0 = {0}, rect1 = {0};
		rect0.x = rect0.y = 0;
		rect0.w = dst_rect->w;
		rect0.h = dst_rect->h;

		//rect1.x = (swap_d_surface->sf.width - rect0.w) / 2;
		//rect1.y = (swap_d_surface->sf.height - rect0.h) / 2;
		rect1.x = dst_rect->x;
		rect1.y = dst_rect->y;
		rect1.w = rect0.w;
		rect1.h = rect0.h;
		hd_copy_surface(dst, &rect0, swap_d_surface, &rect1);
	}

	if(dst_tmp_surface)
	{
		hd_free_surface(dst_tmp_surface);
		dst_tmp_surface = NULL;
	}

	if(new_surface)
	{
	    hd_free_surface(new_surface);
	}
    }
    else
    {
	gxlogd("[GUI] Unsupport color format!\n");
    }

    return (ret);
}

int hd_sync(void)
{
	int ret = 0;
	GxVpuProperty_Sync sync = {0};

	ret = GxAVSetProperty(g_device_handle,
			      vpu_handle,
			      GxVpuPropertyID_Sync,
			      &sync,
			      sizeof(GxVpuProperty_Sync));

	return (ret);
}


