#include "av/avapi.h"
#include "av/gxav.h"
#include "av/gxav_vpu_propertytypes.h"
#include "gal.h"
#include "gxsecure/gxsecure_api.h"
#include "gdi_font.h"
#ifndef NO_OS
#include "gui_private.h"
#include "gxavdev.h"
#else
#include "mini_gui_private.h"
#include "gui_core.h"
extern GuiCore gui;
#endif /*NO_OS*/
#include "hd_gal.h"
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

#include "SDL.h"

void *spare_spp_surface = NULL;
void *spare_surface = NULL;

#ifdef GUI_TTF
static int hw_src_and_dst(GuiSurface *surface, const image_desc *pDesc, int x, int y)
{
	GuiCore *gui_core = NULL;
	GxVpuProperty_Blit Blit;
	GxPalette palette = {0};
	GxColor entries[2];
	GuiSurface *src_surface;
	GAL_Rect rect = {0};
	int ret = 0;
	GxColorFormat color_format = 0;

	gui_core = GUI_GetCurrent();
	surface = hd_get_idle_surface(surface);
	color_format = surface->sf.color_format;

	if(8 == gui_core->config.bpp)
	{
		;
	}
	else if(GX_COLOR_FMT_RGB565 == color_format) {
		hd_color2formatcolor(surface, pDesc->color, &(entries[0]));
		entries[1]   = entries[0];
	}
	else if(GX_COLOR_FMT_ARGB1555 == color_format) {
		hd_color2formatcolor(surface, pDesc->color, &(entries[0]));
		entries[0].a = 0x80;
		entries[1]   = entries[0];
	}
	else if(GX_COLOR_FMT_ARGB4444 == color_format) {
		hd_color2formatcolor(surface, pDesc->color, &(entries[0]));
		entries[0].a = 0xF0;
		entries[1]   = entries[0];
	}
	else if(GX_COLOR_FMT_RGBA8888 == color_format) {
		hd_color2formatcolor(surface, pDesc->color, &(entries[0]));
		entries[0].a = 0xFF;
		entries[1]   = entries[0];
	}
	else if(GX_COLOR_FMT_ARGB8888 == color_format) {
		hd_color2formatcolor(surface, pDesc->color, &(entries[0]));
		entries[0].a = 0xFF;
		entries[1]   = entries[0];
	}

	palette.entries = entries;
	palette.num_entries = 2;

	rect.x = rect.y = 0;
	rect.w = pDesc->width;
	rect.h = pDesc->height;
	src_surface = image_desc_surface(pDesc);
	if (src_surface == NULL)
		return 1;

	memset(&Blit, 0, sizeof(GxVpuProperty_Blit));

	Blit.srca.surface     = src_surface->hw_surface;
	Blit.srca.cct         = (void *)(&palette);
	Blit.srca.is_ksurface = is_ksurface(src_surface);
	Blit.srca.dst_format  = surface->sf.color_format;
	Blit.srca.rect.x      = 0;
	Blit.srca.rect.y      = 0;
	Blit.srca.rect.width  = pDesc->width;
	Blit.srca.rect.height = pDesc->height;

	Blit.srcb.surface     = surface->hw_surface;
	Blit.srcb.is_ksurface = is_ksurface(surface);
	Blit.srcb.rect        = Blit.srca.rect;
	Blit.srcb.rect.x      = (unsigned int)x;
	Blit.srcb.rect.y      = (unsigned int)y;

	Blit.mode             = GX_ALU_VECTOR_FONT_COPY;
	Blit.dst              = Blit.srcb;

	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_Blit,
			&Blit,
			sizeof(GxVpuProperty_Blit));

	hd_add_blit_element(src_surface);

	return (ret);
}

static int sw_src_and_dst(GuiSurface *screen, const image_desc * pDesc, int x, int y)
{
	int i = 0, j = 0, alpha = 0;
	SDL_Surface *sw_surface = NULL, *text_surface = NULL;
	SDL_Color fg_color = {0};
	GuiCore *gui_core = NULL;
	char *PixelBuffer;
	unsigned int surface_width = 0, surface_height = 0;
	unsigned int pixel = 0, *textbuf = NULL, palcolor = 0, pitch = 0;
	unsigned short color16 = 0;
	int Rmask = 0, Gmask = 0, Bmask = 0;
	SDL_Rect dst_rect = {0}, src_rect = {0};
	GxColorFormat color_format = screen->sf.color_format;
	gui_core = GUI_GetCurrent();

	surface_width = screen->sf.width;
	surface_height = screen->sf.height;
	if(8 == screen->bpp) {
		palcolor = gui_core->config.clut->palette[pDesc->color];
		fg_color.r = (palcolor >> 16) & 0x000000FF;
		fg_color.g = (palcolor >> 8) & 0x000000FF;
		fg_color.b = palcolor & 0x000000FF;
		Rmask = 0x0000FF;
		Gmask = 0x00FF00;
		Bmask = 0xFF0000;
	}
	else if(16 == screen->bpp) {
		if (GX_COLOR_FMT_RGB565 == color_format) {
			color16 = (Uint16)pDesc->color;
			Rmask = 0xF800;
			Gmask = 0x07E0;
			Bmask = 0x001F;
			fg_color.r = (color16 & Rmask) >> 8;
			fg_color.g = (color16 & Gmask) >> 3;
			fg_color.b = (color16 & Bmask) << 3;
		}
		else if(GX_COLOR_FMT_ARGB1555 == color_format) {
			color16 = (Uint16)pDesc->color;
			Rmask = 0x7C00;
			Gmask = 0x03E0;
			Bmask = 0x001F;
			fg_color.r = (color16 & Rmask) >> 7;
			fg_color.g = (color16 & Gmask) >> 2;
			fg_color.b = (color16 & Bmask) << 3;
		}
		else if(GX_COLOR_FMT_ARGB4444 == color_format){
			Rmask = 0x0F00;
			Gmask = 0x00F0;
			Bmask = 0x000F;
			fg_color.r = (pDesc->color & Rmask) >> 4;
			fg_color.g = (pDesc->color & Gmask);
			fg_color.b = (pDesc->color & Bmask) << 4;
		}
	}
	else {
			if(GX_COLOR_FMT_RGBA8888 == color_format){
				Rmask = 0xFF000000;
				Gmask = 0x00FF0000;
				Bmask = 0x0000FF00;
				fg_color.r = (pDesc->color & Rmask) >> 24;
				fg_color.g = (pDesc->color & Gmask) >> 16;
				fg_color.b = (pDesc->color & Bmask) >> 8;
			}
			else {
				Rmask = 0xFF0000;
				Gmask = 0x00FF00;
				Bmask = 0x0000FF;
				fg_color.r = (pDesc->color  & Rmask) >> 16;
				fg_color.g = (pDesc->color  & Gmask) >> 8;
				fg_color.b = pDesc->color & Bmask;
			}
	}

	pitch = surface_width * screen->bpp / 8;
	sw_surface = SDL_CreateRGBSurfaceFrom(screen->data,
			surface_width,
			surface_height,
			screen->bpp,
			pitch,
			Rmask,
			Gmask,
			Bmask,
			0);

	text_surface = SDL_AllocSurface(SDL_SWSURFACE, pDesc->width, pDesc->height, 32,
			0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	if(NULL == text_surface)
		return 1;

	pixel = (fg_color.r<<16) | (fg_color.g<<8) | fg_color.b;
	SDL_FillRect(text_surface, NULL, pixel);
	PixelBuffer = (char *)(pDesc->data);
	textbuf = (unsigned int *)(text_surface->pixels);

	for(i = 0; i < pDesc->height; i++) { textbuf = (unsigned int *)text_surface->pixels + i * text_surface->pitch / 4;
		for(j = 0; j < pDesc->width; j++)
		{
			alpha = PixelBuffer[i * pDesc->width + j];
			*textbuf = (alpha << 24) | *textbuf;
			textbuf++;
		}
	}

	src_rect.w = dst_rect.w = pDesc->width;
	src_rect.h = dst_rect.h = pDesc->height;
	dst_rect.x = x;
	dst_rect.y = y;
	src_rect.x = src_rect.y = 0;
	SDL_BlitSurface(text_surface, &src_rect, sw_surface, &dst_rect);

	SDL_FreeSurface(text_surface);
	SDL_FreeSurface(sw_surface);

	return (0);
}
#endif

int hd_soft_img_draw(GuiSurface *surface, const image_desc *pDesc, int x, int y)
{
#ifdef GUI_TTF
    size_t size = 0;
    unsigned int i = 0, offset = 0, component = 0, ret = 0;
    unsigned int pitch = 0, Rmask = 0xFF0000, Gmask = 0x00FF00, Bmask = 0x0000FF;
    unsigned char *data = NULL, *source = NULL;
    SDL_Surface *dst_surface = NULL, *src_surface = NULL;
    SDL_Rect dst_rect = {0}, src_rect = {0};
	GuiCore *gui_core = NULL;

    if((NULL == surface) || (NULL == pDesc))
	return 1;

    if((x + pDesc->width) > surface->sf.width)
	return 1;

    if((y + pDesc->height) > surface->sf.height)
	return 1;

	gui_core = GUI_GetCurrent();

    if(JPEG_TYPE == pDesc->type)
	component = 2;
    else
	component = pDesc->bpp / 8;

    if(surface->bpp != pDesc->bpp)
    {
	pitch = (((surface->sf.width * gui_core->config.bpp) + 31) >> 5) << 2;
	dst_surface = SDL_CreateRGBSurfaceFrom(surface->data,
					      surface->sf.width,
					      surface->sf.height,
					      gui_core->config.bpp,
					      pitch,
					      Rmask,
					      Gmask,
					      Bmask,
					      0);
	if(NULL == dst_surface)
	    return (1);

	pitch = (((pDesc->width * pDesc->bpp) + 31) >> 5) << 2;
	data = (unsigned char *)GxCore_Mallocz(pitch * pDesc->height * sizeof(char));
	if(NULL == data)
	{
	    SDL_FreeSurface(dst_surface);
	    return (1);
	}

	if((pDesc->width * pDesc->bpp / 8) == pitch)
	{
	    memcpy(data, pDesc->data, pitch * pDesc->height * sizeof(char));
	}
	else
	{
	    for(i = 0; i < pDesc->height; i++)
	    {
		memcpy(data + i * pitch,
		       pDesc->data + i * pDesc->width * pDesc->bpp / 8,
		       pDesc->width * pDesc->bpp / 8);
	    }
	}
	src_surface = SDL_CreateRGBSurfaceFrom(data,
					       pDesc->width,
					       pDesc->height,
					       pDesc->bpp,
					       pitch,
					       Rmask,
					       Gmask,
					       Bmask,
					       0);
	if(NULL == src_surface)
	{
	    SDL_FreeSurface(dst_surface);
	    GUI_FREE(data);
	    return (1);
	}

	src_rect.w = dst_rect.w = pDesc->width;
	src_rect.h = dst_rect.h = pDesc->height;
	dst_rect.x = x;
	dst_rect.y = y;
	src_rect.x = src_rect.y = 0;
	ret = SDL_BlitSurface(src_surface, &src_rect, dst_surface, &dst_rect);

	SDL_FreeSurface(src_surface);
	SDL_FreeSurface(dst_surface);
	GUI_FREE(data);
	return (ret);
    }

    if(pDesc->width == surface->sf.width)
    {
	size = (surface->buffer_size < pDesc->size) ? surface->buffer_size : pDesc->size;
	memcpy(surface->data, pDesc->data, size);
    }
    else
    {
	data = (unsigned char *)surface->data;
	source = (unsigned char *)pDesc->data;
	offset = y * surface->sf.width * component + x * component;
	for(i = 0; i < pDesc->height; i++)
	    memcpy(data + offset + i * surface->sf.width * component, source + i * pDesc->width * component, pDesc->width * component);
    }
#endif /*GUI_TTF*/

    return 0;
}

int hd_img_desc(GuiSurface *surface, const image_desc *pDesc, int x, int y, int w, int h)
{
	int ret = 0, i = 0;
	int color = 0, num_colors = 0;
	GxVpuProperty_Blit Blit;
	GAL_Rect rect = {0}, dst_rect = {0}, temp_rect = {0};
	GuiSurface *src_surface, *temp_surface = NULL;
	GxPalette palette = {0};
	GxColor entries[256];

	/*A property for displaying TTF vector font designedly.*/
#ifdef GUI_TTF
	if((EFFECT_SRC_AND_DST == pDesc->effect) &&
	   (8 != surface->bpp)) {
		if(((GXAV_ID_GX3211 == gui.config.chip_id) ||
		   (GXAV_ID_GX6605S == gui.config.chip_id)) &&
		   (gui.config.is_align)) {
			memset(&rect, 0, sizeof(GAL_Rect));
			rect.h = h;
			if(w % 4) {
				rect.w = (w / 4 + 1) * 4;
			} else {
				rect.w = w;
			}
			temp_surface = hd_get_surface0(&rect,
						       surface->bpp,
						       surface->sf.color_format,
						       NULL,
						       GUI_FALSE);
			temp_rect = rect;
			temp_rect.x = x;
			temp_rect.y = y;
			rect.w = temp_rect.w = w;
			ret = hd_copy_surface(surface, &temp_rect, temp_surface, &rect);
			hd_new_blit_list();
			ret |= hw_src_and_dst(temp_surface, pDesc, 0, 0);
			hd_end_blit_list();
			ret |= hd_copy_surface(temp_surface, &rect, surface, &temp_rect);
			hd_free_surface(temp_surface);
		} else {
			memset(&rect, 0, sizeof(GAL_Rect));
			rect.w = w;
			rect.h = h;
			if(gxgdi_font_is_hwdraw())
				ret = hw_src_and_dst(surface, pDesc, x, y);
			else
				ret = sw_src_and_dst(surface, pDesc, x, y);
		}
		if(ret)
			ret = sw_src_and_dst(surface, pDesc, x, y);

		return ret;
	}
#endif

	surface = hd_get_idle_surface(surface);
	rect.x = rect.y = 0;
	rect.w = pDesc->width;
	rect.h = pDesc->height;

	if(24 == pDesc->bpp) {
		GAL_Rect desc_rect = {0};
		GuiSurface *argb_surface = {0};
		int i = 0;

		desc_rect.x = desc_rect.y = 0;
		if(pDesc->width % 8) {
			desc_rect.w = (pDesc->width / 8 + 1) * 8;
		} else {
			desc_rect.w = pDesc->width;
		}
		desc_rect.h = pDesc->height;

		src_surface = hd_get_surface(&desc_rect, pDesc->bpp, NULL, GUI_FALSE);
		if(src_surface) {
			memset(src_surface->data, 0, src_surface->buffer_size);
			for(i = 0; i < pDesc->height; i++) {
				memcpy(&(((char *)src_surface->data)[i * src_surface->sf.width * 3]),
				       &(((char *)pDesc->data)[i * pDesc->width * 3]),
				       pDesc->width * 3);
			}
		}
		argb_surface = hd_get_surface(&desc_rect, 32, NULL, GUI_FALSE);
		hd_new_blit_list();
		hd_copy_surface(src_surface, &desc_rect, argb_surface, &desc_rect);
		hd_end_blit_list();
		hd_free_surface(src_surface);
		src_surface = argb_surface;
	} else {
		src_surface = image_desc_surface(pDesc);
	}

	if (src_surface == NULL)
	{
	    return (hd_soft_img_draw(surface, pDesc, x, y));
	}

	dst_rect.x = x;
	dst_rect.y = y;
	dst_rect.w = w;
	dst_rect.h = h;

	if((pDesc->width != w) ||
	   (pDesc->height != h))
	{
		GuiSurface *src_surface0 = NULL;
		GAL_Rect rect0 = {0};

		if((8 >= pDesc->bpp) &&
		   (pDesc->bpp != surface->bpp)) {
			hd_free_surface(src_surface);
			temp_rect.x = temp_rect.y = 0;
			temp_rect.w = pDesc->width;
			temp_rect.h = pDesc->height;
			src_surface = hd_get_surface(&temp_rect, 32, NULL, GUI_FALSE);
			if(NULL == src_surface) {
				return (1);
			}
			hd_img_desc(src_surface, pDesc, 0, 0, pDesc->width, pDesc->height);
		}
		temp_rect = dst_rect;
		temp_rect.x = temp_rect.y = 0;
		if(temp_rect.w == (src_surface->sf.width / 4))
			temp_rect.w += (src_surface->sf.width % 4);
		if(temp_rect.h == (src_surface->sf.height / 4))
			temp_rect.h += (src_surface->sf.height % 4);
		temp_surface = hd_get_surface(&temp_rect, 32, NULL, GUI_FALSE);
		if(NULL == temp_surface)
		{
			return (1);
		}

		rect0.x = rect0.y = 0;
		rect0.w = src_surface->sf.width;
		rect0.h = src_surface->sf.height;
		src_surface0 = hd_get_surface0(&rect0,
					       src_surface->bpp,
					       src_surface->sf.color_format,
					       NULL,
					       GUI_FALSE);
		if(NULL == src_surface0) {
			src_surface0 = src_surface;
		} else {
			memcpy(src_surface0->data, src_surface->data, src_surface->buffer_size);
		}
		ret = hd_zoom_surface(src_surface0, &rect, temp_surface, &temp_rect);
		if(ret) {
			if(src_surface0) {
				hd_free_surface(src_surface0);
			}
			if(temp_surface) {
				hd_free_surface(temp_surface);
			}
			return (1);
		}
		temp_rect = dst_rect;
		temp_rect.x = temp_rect.y = 0;
		if(((GXAV_ID_GX3211 == gui.config.chip_id) ||
		   (GXAV_ID_GX6605S == gui.config.chip_id)) &&
		   (gui.config.is_align)) {
			GuiSurface *align_surface = NULL;
			GAL_Rect align_rect = {0};

			align_rect = temp_rect;
			if(align_rect.w % 4) {
				align_rect.w = (align_rect.w / 4 + 1) * 4;
			}
			align_surface = hd_get_surface0(&align_rect,
							surface->bpp,
							surface->sf.color_format,
							NULL,
							GUI_FALSE);
			if(NULL == align_surface) {
				if(src_surface0) {
					hd_free_surface(src_surface0);
				}
				hd_free_surface(temp_surface);
				return (1);
			}
			hd_copy_surface(temp_surface, &temp_rect, align_surface, &temp_rect);
			hd_copy_surface(align_surface, &temp_rect, surface, &dst_rect);
			hd_free_surface(align_surface);
		} else {
			if(32 == pDesc->bpp) {
				temp_surface->alpha = 0xFF;
				hd_new_blit_list();
				hd_stretch_surface(temp_surface, &temp_rect, surface, &dst_rect);
				hd_end_blit_list();
			} else {
				hd_copy_surface(temp_surface, &temp_rect, surface, &dst_rect);
			}
		}
		hd_free_surface(temp_surface);
		if(src_surface0 != src_surface)
			hd_add_blit_element(src_surface0);
		hd_add_blit_element(src_surface);
		return 0;
	}

	color = gal_get_gui_transparent();
	color = hd_color2index(src_surface, color);

	memset(&Blit, 0, sizeof(GxVpuProperty_Blit));

	if((8 >= pDesc->bpp) && (pDesc->bpp != surface->bpp)) {
		GuiClut *clut = NULL;
		int clut_index = 0;
		image_desc *desc = NULL;

		clut = gui.config.clut;
		while(clut_index < pDesc->pal_index) {
			clut = clut->next;
			clut_index++;
		}

		if(clut) {
			desc = (image_desc *)pDesc;
			desc->pal = clut->palette;
		}

		if(pDesc->pal) {
			num_colors = 1 << pDesc->bpp;
			palette.entries = entries;
			for(i = 0; i < num_colors; i++) {
				color = ((int *)pDesc->pal)[i];
				if(16 == surface->bpp) {
					entries[i].r = (color >> 16) & 0xF8;
					entries[i].g = (color >> 8) & 0xFC;
					entries[i].b = color & 0xF8;
					entries[i].a = 0xFF;
				}
				else if((8 == surface->bpp) || (32 == surface->bpp)) {
					entries[i].r = (color >> 16) & 0xFF;
					entries[i].g = (color >> 8) & 0xFF;
					entries[i].b = color & 0xFF;
					entries[i].a = 0xFF;
				}
			}
			if(GIF_TYPE == pDesc->type) {
				color = pDesc->color;
			}
			palette.num_entries = num_colors;
			Blit.srca.cct = (void *)(&palette);
			Blit.srca.is_kcct = GUI_FALSE;
		}

		if(desc) {
			desc->pal = NULL;
		}
		Blit.srca.dst_format = get_color_format(0, surface->bpp);
	}

	Blit.srca.surface     = src_surface->hw_surface;
	Blit.srca.is_ksurface = is_ksurface(src_surface);
	Blit.srca.rect.x      = 0;
	Blit.srca.rect.y      = 0;
	Blit.srca.rect.width  = pDesc->width;	//((int)(r_width / 4)) * 4;
	Blit.srca.rect.height = pDesc->height;	//((int)(r_height / 4)) * 4;
	Blit.srca.dst_format  = pDesc->fmt;

	Blit.srcb.surface     = surface->hw_surface;
	Blit.srcb.is_ksurface = is_ksurface(surface);
	Blit.srcb.rect        = Blit.srca.rect;
	Blit.srcb.rect.x      = x;
	Blit.srcb.rect.y      = y;
	Blit.srcb.dst_format  = surface->sf.color_format;

	Blit.dst       = Blit.srcb;
	Blit.dst.alpha = gui.config.osd_alpha;
	Blit.mode      = GX_ALU_ROP_COPY;
	Blit.colorkey_info.mode = GX_BLIT_COLORKEY_BASIC_MODE;
	if(EFFECT_COPY == pDesc->effect)
		Blit.colorkey_info.src_colorkey_en = GUI_FALSE;
	else if(EFFECT_SRC_TO_DST == pDesc->effect) {
		Blit.colorkey_info.src_colorkey_en = GUI_TRUE;
		Blit.colorkey_info.src_colorkey = color;
	} else if(EFFECT_SRC_AND_DST == pDesc->effect) {
		Blit.mode      = GX_ALU_ROP_OR;
	}

	if(pDesc->alpha)
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

		Blit.srca.alpha_en = GUI_TRUE;
		Blit.srca.alpha = pDesc->alpha;
	}

	if(32 == pDesc->bpp)
	{
	    //Blit.dst.alpha_en =GUI_TRUE;
	    Blit.colorkey_info.src_colorkey_en = GUI_FALSE;
	    Blit.colorkey_info.dst_colorkey_en = GUI_TRUE;
	    //Blit.colorkey_info.src_colorkey = gui.config.osd_trans;
	    Blit.colorkey_info.dst_colorkey = gui.config.osd_trans;
	}

	ret = GxAVSetProperty(g_device_handle, vpu_handle, GxVpuPropertyID_Blit, &Blit, sizeof(GxVpuProperty_Blit));
	if(0 != ret) {
		hd_free_surface(src_surface);
		gxlogd("%s: [GUI]Image BLIT failed!\n", __func__);
		return (1);
	}

	if(32 == pDesc->bpp)
	{
	    hd_add_blit_element(NULL);

	    memset(&Blit, 0, sizeof(GxVpuProperty_Blit));
	    if(palette.entries)
	    {
		Blit.srca.cct = (void *)(&palette);
		Blit.srca.is_kcct = GUI_FALSE;
	    }

	    Blit.srca.surface     = src_surface->hw_surface;
	    Blit.srca.is_ksurface = is_ksurface(src_surface);
	    Blit.srca.rect.x      = 0;
	    Blit.srca.rect.y      = 0;
	    Blit.srca.rect.width  = pDesc->width;	//((int)(r_width / 4)) * 4;
	    Blit.srca.rect.height = pDesc->height;	//((int)(r_height / 4)) * 4;

	    Blit.srcb.surface     = surface->hw_surface;
	    Blit.srcb.is_ksurface = is_ksurface(surface);
	    Blit.srcb.rect        = Blit.srca.rect;
	    Blit.srcb.rect.x      = x;
	    Blit.srcb.rect.y      = y;

	    Blit.dst       = Blit.srcb;
	    Blit.dst.alpha = gui.config.osd_alpha;
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

	    ret = GxAVSetProperty(g_device_handle, vpu_handle, GxVpuPropertyID_Blit, &Blit, sizeof(GxVpuProperty_Blit));
	    if(0 != ret) {
		    hd_free_surface(src_surface);
		    gxlogd("%s: [GUI]Image BLIT failed!\n", __func__);
		    return (1);
	    }
	    hd_add_blit_element(src_surface);
	}
	else
	{
	    hd_add_blit_element(src_surface);
	}

	return (ret);
}

int hd_img_desc_mirror(GuiSurface *surface, const image_desc *pDesc, int x, int y, int w, int h, GxReverseMode mode)
{
	int ret = 0;
	GAL_Rect src_rect = {0}, dst_rect = {0};
	GuiSurface *tmp_surface = NULL, *src_surface = NULL;

	src_rect.x = src_rect.y = 0;
	src_rect.w = pDesc->width;
	src_rect.h = pDesc->height;
	tmp_surface = hd_get_surface0(&src_rect, pDesc->bpp, pDesc->fmt, NULL, GUI_FALSE);
	if(NULL == tmp_surface) {
		return (1);
	}

	src_surface = image_desc_surface(pDesc);
	if(NULL == src_surface) {
		hd_free_surface(tmp_surface);
		return (1);
	}
	ret = hd_mirror_image_surface(src_surface, &src_rect, tmp_surface, &src_rect, mode);
	if(ret) {
		hd_free_surface(tmp_surface);
		return (1);
	}

	dst_rect.x = x;
	dst_rect.y = y;
	dst_rect.w = w;
	dst_rect.h = h;
	if((tmp_surface->sf.width != w) ||
	   (tmp_surface->sf.height != h)) {
		ret = hd_zoom_surface(tmp_surface, &src_rect, surface, &dst_rect);
	} else {
		ret = hd_stretch_surface(tmp_surface, &src_rect, surface, &dst_rect);
	}
	hd_add_blit_element(tmp_surface);

	return (ret);
}

int hd_img_copy(GuiSurface *surface, char *buffer, int x, int y, int x_size, int y_size, int component)
{

	return 0;
}

#ifndef NO_OS
int hd_img_clear(void)
{
	GuiCore *gui_core = NULL;
	GxVpuProperty_FillRect FillRect = {0};
	int ret = 0;
	int spp_width = 0;
	int spp_height =0;

	if(NULL == spp_screen)
	{
		return (0);
	}

	gui_core = GUI_GetCurrent();
	spp_width =  gui_core->config.width;
	spp_height = gui_core->config.height;

	FillRect.color.y = 0x10;
	FillRect.color.cb = 0x80;
	FillRect.color.cr = 0x80;
	FillRect.color.alpha = 0xFF;
	FillRect.rect.x = 0;
	FillRect.rect.y = 0;
	FillRect.is_ksurface = is_ksurface(spp_screen);
	FillRect.rect.width = spp_width;
	FillRect.rect.height = spp_height;
	FillRect.surface = spp_screen->hw_surface;
	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_FillRect,
			&FillRect,
			sizeof(GxVpuProperty_FillRect));

	if(0 != ret)
	{
		gxlogd("[GUI]Fill Rectangle failed\n");
		return (1);
	}
	hd_add_blit_element(NULL);

	return (0);
}

int hd_enable_img(int flag)
{
	int ret = GxAvdev_LayerEnable(g_device_handle, vpu_handle, GX_LAYER_SPP, flag);
	if(0 != ret) {
		gxlogd("[GUI]Set Layer enable failed!\n");
		return (1);
	}

	return ret;
}
#endif /*NO_OS*/


