#ifndef NO_OS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pngapi.h"
#include "gdi.h"
#include "gal.h"
#include "IMG.h"
#include "hd_gal.h"
#include "image_cache.h"
#include "gui_private.h"
#include "gdi_play.h"
#include "gui_config.h"
#include "gui_xml_parse.h"
#include "gx_videomem.h"

#ifdef JPEG_ENCODING
#include "jpeglib.h"
#include "jerror.h"             /* get library error codes too */
#include "cderror.h"
#endif

#include "../../include/gxavdev.h"

int s_pal_index = 0;
static int spp_flag = 0;
static image_desc *back_desc = NULL;
static struct image_register_func s_img_register_func = {0};

static image_file_node bmp_node = {
	.type            = BMP_TYPE,
	.file_type       = {"bmp", NULL},
#ifdef GUI_BMP_INFO
	.img_cb          = IMG_BMP_Draw,
	.img_get_info_cb = IMG_BMP_GetInfo,
#endif
	.img_load_cb     = IMG_BMP_Load,
	.img_free        = NULL,
	.img_stop        = IMG_BMP_STOP,
};

static image_file_node jpg_node = {
	.type            = JPEG_TYPE,
	.file_type       = {"jpg", "jpeg", NULL},
	.img_cb          = IMG_JPEG_Draw,
	.img_load_cb     = IMG_JPEG_Load,
	.img_get_info_cb = IMG_JPEG_GetInfo,
	.img_free        = NULL,
	.img_stop        = IMG_JPEG_STOP,
};

static image_file_node jpg_soft_node = {
#ifdef GUI_SOFT_JPEG
	.type            = JPEG_TYPE,
	.file_type       = {"jpg", "jpeg", NULL},
	.img_cb          = IMG_JPEG_SOFT_Draw,
	.img_load_cb     = IMG_JPEG_SOFT_Load,
	.img_get_info_cb = IMG_JPEG_GetInfo,
	.img_free        = NULL,
	.img_stop        = IMG_JPEG_SOFT_STOP,
#endif /*GUI_SOFT_JPEG*/
};

static image_file_node gif_node = {
	.type            = GIF_TYPE,
	.file_type       = {"gif", NULL},
	.img_cb          = IMG_GIF_Draw,
	.img_load_cb     = IMG_GIF_Load,
	.img_get_info_cb = IMG_GIF_GetInfo,
	.img_free        = IMG_GIF_Free,
	.img_stop        = IMG_GIF_Stop,
};

static image_file_node png_node = {
	.type            = PNG_TYPE,
	.file_type       = {"png", NULL},
	.img_cb          = IMG_PNG_Draw,
	.img_load_cb     = IMG_PNG_Load,
	.img_get_info_cb = IMG_PNG_GetInfo,
	.img_free        = NULL,
	.img_stop        = NULL,
};

static image_file_node tiff_node = {
	.type            = TIFF_TYPE,
	.file_type       = {"tif", "tiff", NULL},
	.img_cb          = IMG_TIFF_Draw,
	.img_load_cb     = IMG_TIFF_Load,
	.img_get_info_cb = IMG_TIFF_GetInfo,
	.img_free        = NULL,
	.img_stop        = NULL,
};

static image_file_node mpeg_node = {
	.type            = MPEG_TYPE,
	.file_type       = {"mpg", NULL},
	.img_cb          = IMG_MPEG_Draw,
	.img_load_cb     = IMG_MPEG_Load,
	.img_get_info_cb = IMG_MPEG_GetInfo,
	.img_free        = NULL,
	.img_stop        = NULL,
};

static image_file_node webp_node = {
	.type            = WEBP_TYPE,
	.file_type       = {"webp", NULL},
	.img_cb          = IMG_WEBP_Draw,
	.img_load_cb     = IMG_WEBP_Load,
	.img_get_info_cb = IMG_WEBP_GetInfo,
	.img_free        = NULL,
	.img_stop        = IMG_WEBP_STOP,
};

struct image_special_node special_jpeg_zoom;
struct image_special_node special_jpeg_cut;
struct image_special_node special_png;

#define IMG_NODE_MAX 12
static image_file_node *img_node[IMG_NODE_MAX] = {
	&bmp_node, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL
};

IMG_Config image_config = {0};

static int register_image(struct image_file_node *node)
{
	int i = 0;

	for(i = 0; i < IMG_NODE_MAX; i++) {
		if (img_node[i] == NULL) {
			img_node[i] = node;
			return 0;
		}
	}

	return -1;
}

int _check_img_decode(struct image_file_node *node)
{
	int i = 0;

	if(NULL == node) {
		return (GUI_FALSE);
	}

	for(i = 0; i < IMG_NODE_MAX; i++) {
		if (img_node[i] == NULL)
			continue;
		if(node == img_node[i]) {
			return (GUI_TRUE);
		}
	}

	return (GUI_FALSE);
}

// 先注册的 JPEG 为解码器，后注册的为备份
int GDI_RegisterJPEG(void)
{
	int i = 0, j = 0;

	if(GUI_TRUE == _check_img_decode(&jpg_node)) {
		return (1);
	}

	for(i = 0; i < IMG_NODE_MAX; i++) {
		if (img_node[i] == NULL)
			continue;
		if (img_node[i]->type == JPEG_TYPE && img_node[i] != &jpg_node) { //找到JPEG 解码器
			for(j = (IMG_NODE_MAX - 1); j > i; j--)
			{
			    img_node[j] = img_node[j-1];
			}
			img_node[i] = &jpg_node;
			img_node[i]->reserve = &jpg_soft_node;
			return (0);
		}
	}

	return register_image(&jpg_node);
}

int GDI_UnRegisterJPEG(void)
{
    int i = 0, j = 0;

    for(i = 0; i < IMG_NODE_MAX; i++) {
	if (img_node[i] == NULL)
		continue;
	if((img_node[i]->type == JPEG_TYPE) && (img_node[i] == &jpg_node))
	{
	    for(j = i; img_node[j]; j++)
	    {
		img_node[j] = img_node[j+1];
	    }
	    return (0);
	}
    }

    return (1);
}

int GDI_RegisterJPEG_SOFT(void)
{
	int i = 0;

	if(GUI_TRUE == _check_img_decode(&jpg_soft_node)) {
		return (1);
	}

	for(i = 0; i < IMG_NODE_MAX; i++) {
		if (img_node[i] == NULL)
			continue;
		if (img_node[i]->type == JPEG_TYPE && img_node[i] != &jpg_soft_node) { //找到JPEG 解码器
			img_node[i]->reserve = &jpg_soft_node;
			break;
		}
	}

	return register_image(&jpg_soft_node);
}

int GDI_RegisterJPEG_SOFT_Special(void)
{
	GDI_RegisterJPEG_SOFT();

	if(NULL == special_jpeg_zoom.img_zoom_cut)
	{
		special_jpeg_zoom.img_zoom_cut = IMG_JPEG_Load_and_Zoom;
	}
	if(NULL == special_jpeg_cut.img_zoom_cut)
	{
		special_jpeg_cut.img_zoom_cut  = IMG_JPEG_Load_and_Cut;
	}

	return register_image(&jpg_soft_node);
}

int GDI_UnRegisterJPEG_SOFT(void)
{
    int i = 0, j = 0;

    for(i = 0; i < IMG_NODE_MAX; i++) {
	if((img_node[i]->type == JPEG_TYPE) && (img_node[i] == &jpg_soft_node))
	{
	    for(j = i; img_node[j]; j++)
	    {
		img_node[j] = img_node[j+1];
	    }
	    return (0);
	}
    }

    return (1);
}

int GDI_RegisterGIF(void)
{
	return register_image(&gif_node);
}

int GDI_RegisterPNG(void)
{
	special_png.img_load_size =	PNG_LoadData;
	special_png.img_check	= PNG_Check;
	return register_image(&png_node);
}

int GDI_RegisterTIFF(void)
{
	return register_image(&tiff_node);
}

int GDI_RegisterMPEG(void)
{
	return register_image(&mpeg_node);
}

int GDI_RegisterWEBP(void)
{
	return register_image(&webp_node);
}

int GDI_RegisterAll(void)
{
	int ret = 0;

	ret = GDI_RegisterJPEG();
	ret |= GDI_RegisterJPEG_SOFT();
	ret |= GDI_RegisterGIF();
	ret |= GDI_RegisterPNG();
	ret |= GDI_RegisterTIFF();
	ret |= GDI_RegisterMPEG();
	ret |= GDI_RegisterWEBP();

	return ret;
}

static struct image_file_node *gal_find_node(const char *filename);

int gal_img_get_width(const char *filename)
{
	Image_Exif img_info;

	struct image_file_node *image_ops = gal_find_node(filename);

	if (image_ops && image_ops->img_get_info_cb)
		if (image_ops->img_get_info_cb(filename, &img_info) == 0)
			return img_info.width;

	return 0;
}

int gal_img_get_height(const char *filename)
{
	Image_Exif img_info;

	struct image_file_node *image_ops = gal_find_node(filename);

	if (image_ops && image_ops->img_get_info_cb)
		if (image_ops->img_get_info_cb(filename, &img_info) == 0)
			return img_info.height;

	return 0;
}

int gal_img_uncache(int flag)
{
	GuiCore *gui_core = NULL;
	int ret = 0;

	gui_core = GUI_GetCurrent();
	ret = gui_core->config.only_gdi;
	gui_core->config.only_gdi = flag;
	return (ret);
}

int gal_img_stop0(const char *filename)
{
	struct image_file_node *image_ops = gal_find_node(filename);
	int ret = 0;

	if (image_ops && image_ops->img_stop) {
		ret = image_ops->img_stop();
	}

	return (ret);
}

int gal_img_stop(const char *filename)
{
	int ret = 0;

	ret = gal_img_stop0(filename);

	gdi_lock();
	hd_img_clear();
	gal_set_default_desc(NULL);
	gdi_commit();
	gdi_unlock();


	return 0;
}

int gal_img_file(void *screen, const char *filename, int x, int y, int screen_width, int screen_height)
{
	GuiCore *gui_core = NULL;
	void *surface = screen;
	int ret = 0, cache = 0, mode = 0;
	handle_t handle = GXCORE_INVALID_POINTER;
	struct image_file_node *image_ops = gal_find_node(filename);

	if(NULL == image_ops)
	{
	    GUI_Debug_Print("[GUI] Cannot find any image %s decoder.\n", filename);
	}

	handle = GxCore_FileExists(filename);
	if(GXCORE_FILE_UNEXIST == handle)
	{
	    GUI_Debug_Print("[GUI] File %s is not existed.\n", filename);
	}

	gui_core = GUI_GetCurrent();
	if (image_ops && image_ops->img_cb) {
		gdi_lock();
		hd_img_clear();
		gal_set_default_desc(NULL);
		hd_add_blit_element(NULL);

		hd_commit_blit();
		GxAvdev_SppLock();
		cache = gal_img_uncache(GUI_TRUE);
		ret = image_ops->img_cb(surface, filename, x, y, screen_width, screen_height);
		if (ret) { // 错误
			image_ops = image_ops->reserve;
			if (image_ops)
				ret = image_ops->img_cb(surface, filename, x, y, screen_width, screen_height);

		}
		gdi_commit();
		gal_img_uncache(cache);
		GxAvdev_SppUnlock();
		gdi_unlock();
	}
	else
	{
		return (1);
	}

	gal_enable_img(GUI_TRUE);
	hd_enable_video(GUI_FALSE);

	return ret;
}

GuiSurface *gal_image_surface(const char *filepath, int *x, int *y, int width, int height)
{
    char *file_ext;
    static image_desc *img_desc = NULL;
    static int count;
    GIF_Para *gif_src = NULL;
    GIF_Image *pGifImage = NULL;
    Image_Exif exif;
    GuiSurface *dst = NULL;
    GAL_Rect rect = {0};

    file_ext = strrchr(filepath, '.') + 1;

    if (file_ext == NULL)
	return NULL;

    if((0 == strcasecmp("jpeg", file_ext)) ||
       (0 == strcasecmp("jpg", file_ext)))
    {
	IMG_JPEG_GetInfo(filepath, &exif);
	dst = jpeg_surface(filepath, exif.width, exif.height, width, height);
	if(dst) {
		return (dst);
	} else {
		img_desc = gal_img_load(img_desc, filepath);
		if(img_desc)
			dst = image_desc_surface(img_desc);
		return (dst);
	}
    }
    else if((0 == strcasecmp("bmp", file_ext)) ||
	    (0 == strcasecmp("png", file_ext)))
    {
	img_desc = gal_img_load(img_desc, filepath);
	if(NULL == img_desc)
	    return NULL;

	rect.x = rect.y = 0;
	rect.w = img_desc->width;
	rect.h = img_desc->height;
	dst = hd_get_surface(&rect, 0, NULL, GUI_FALSE);

	convert_rgb_2_yuv422((void*)(img_desc->data),
			      dst->data,
			      img_desc->width,
			      img_desc->height,
			      img_desc->bpp,
			      img_desc->pal);

	GUI_FREE(img_desc->filename);
	GUI_FREE(img_desc->pal);
	GUI_FREE(img_desc);
	return dst;
    }
    else if(0 == strcasecmp("gif", file_ext))
    {
	if(filepath)
	{
	    gal_img_cleanup(img_desc);
	    img_desc = NULL;
	    img_desc = gal_img_load(img_desc, filepath);
	}

	gif_src = (GIF_Para *)(img_desc->data);
	if(0 == count)
	    pGifImage = gif_src->cur_image = gif_src->each_image;
	else
	    pGifImage = gif_src->cur_image = gif_src->cur_image->next;

	if(NULL == pGifImage)
	{
	    count = 0;
	    return NULL;
	}
	else
	{
	    if(x)
		*x = pGifImage->img_left;
	    if(y)
		*y = pGifImage->img_top;

	    rect.x = rect.y = 0;
	    rect.w = pGifImage->img_width;
	    rect.h = pGifImage->img_height;
	    dst = hd_get_surface(&rect, 0, NULL, GUI_FALSE);

	    convert_rgb_2_yuv422((void*)(pGifImage->img_data),
				  dst->data,
				  pGifImage->img_width,
				  pGifImage->img_height,
				  8,
				  pGifImage->img_pal);

	}

	count++;
	return dst;
    }
    else
    {
	    img_desc = gal_img_load(img_desc, filepath);
	    if(img_desc)
		    dst = image_desc_surface(img_desc);
	    return (dst);
    }
}

int gal_set_logic_clut(void *screen, const image_desc *pDesc)
{
	int ret = 0, i = 0;
	GuiCore *gui_core = NULL;
	GuiClut *pclut = NULL;

	if(screen == NULL || pDesc == NULL)
		return 1;

	gui_core = GUI_GetCurrent();
	i = 0;
	pclut = gui_core->config.firstclut;
	if(s_pal_index != pDesc->pal_index) {
		while(i < pDesc->pal_index) {
			if(NULL == pclut)
				break;
			pclut = pclut->next;
			i++;
		}

		if(pclut) {
			gui_core->config.clut = pclut;
			s_pal_index = i;
			gui_core->config.logical_clut = i;
		}
	}

	return ret;
}

int gal_set_clut(void *screen, const image_desc *pDesc)
{
	int ret = 0, i = 0;
	GuiCore *gui_core = NULL;
	GuiClut *pclut = NULL;

	if(screen == NULL || pDesc == NULL)
		return 1;

	gui_core = GUI_GetCurrent();
	if(s_pal_index != pDesc->pal_index) {
		i = 0;
		pclut = gui_core->config.firstclut;
		while(i < pDesc->pal_index) {
			if(NULL == pclut)
				break;
			pclut = pclut->next;
			i++;
		}

		if(pclut) {
			gui_core->config.clut = pclut;
			ret = gui_set_clut(screen, pclut->palette, 0, pclut->num);
		}

		s_pal_index = i;
		gui_core->config.logical_clut = i;
	}

	return ret;
}

int gal_get_clut_index(void)
{
	return s_pal_index;
}

int gal_set_clut_index(int pal_index)
{
	s_pal_index = pal_index;
	return 0;
}

int gal_img_desc(void *screen, const image_desc *pDesc, int x, int y, int w, int h)
{
	if(screen == NULL || pDesc == NULL)
		return 1;

	gal_set_clut(screen, pDesc);

	return hd_img_desc(screen, pDesc, x, y, w, h);
}

int gal_img_desc_mirror(void *screen, const image_desc *pDesc, int x, int y, int w, int h, GxReverseMode mode)
{
	if(screen == NULL || pDesc == NULL)
		return 1;

	gal_set_clut(screen, pDesc);

	return hd_img_desc_mirror(screen, pDesc, x, y, w, h, mode);
}

void gal_img_threshod_size(int size)
{
	image_config.threshold_size = size;
}

void gal_img_memory_set_priority(ImageMemorySource level, int size)
{
	image_config.level = level;
	image_config.level_size = size;
}

ImageMemorySource gal_get_memory_type(int size)
{
	ImageMemorySource type = IMAGE_FROM_VIDEO_MEMORY;

	if((0 == image_config.level_size) ||
	   ((image_config.level_size) && (size < image_config.level_size))) {
		type = image_config.level;
	} else {
		type = image_config.level == IMAGE_FROM_VIDEO_MEMORY ? IMAGE_FROM_SYSTEM_MEMORY : IMAGE_FROM_VIDEO_MEMORY;
	}

	return (type);
}

GALMemory *gal_alloc_memory_by_source(int size, ImageMemorySource source)
{
	GALMemory *p = NULL;

	p = GUI_MALLOCZ(sizeof(GALMemory));
	if(NULL == p) {
		return (NULL);
	}

	p->source = source;
	if(p->source == IMAGE_FROM_VIDEO_MEMORY) {
		p->block.vq = hd_malloc(&(p->block.vq), size);
		if(NULL == p->block.vq) {
			p->source = IMAGE_FROM_SYSTEM_MEMORY;
			p->block.ptr = GUI_MALLOCZ(size);
		}
	} else {
		p->block.ptr = GUI_MALLOCZ(size);
		if(NULL == p->block.ptr) {
			p->source = IMAGE_FROM_VIDEO_MEMORY;
			p->block.vq = hd_malloc(&(p->block.vq), size);
		}
	}

	if(p->source == IMAGE_FROM_VIDEO_MEMORY) {
		if(NULL == p->block.vq) {
			goto error;
		}
	} else {
		if(NULL == p->block.ptr) {
			goto error;
		}
	}

	p->size = size;
	return (p);

error:
	GUI_FREE(p);

	return (p);
}

GALMemory *gal_alloc_memory(int size)
{
	GALMemory *p = NULL;
	ImageMemorySource source;

	if((image_config.threshold_size) && (size > image_config.threshold_size)) {
		return (NULL);
	}

	source = gal_get_memory_type(size);

	p = gal_alloc_memory_by_source(size, source);

	return (p);
}

int gal_free_memory(GALMemory *p)
{
	if(NULL == p) {
		return (-1);
	}

	if(p->source == IMAGE_FROM_VIDEO_MEMORY) {
		hd_free(p->block.vq);
		GUI_FREE(p->block.vq);
	} else {
		GUI_FREE(p->block.ptr);
	}

	GUI_FREE(p);

	return (0);
}

int gal_check_image_size(int size)
{
	if(0 == image_config.threshold_size) {
		return (GUI_TRUE);
	}

	if(size < image_config.threshold_size) {
		return (GUI_TRUE);
	} else {
		return (GUI_FALSE);
	}
}

int gal_img_stretch(GuiSurface *surface, const image_desc *pDesc, GAL_Rect *src_rect, GAL_Rect *dst_rect)
{
    GuiSurface *src_surface = NULL;

    if((NULL == surface) ||
       (NULL == pDesc) ||
       (NULL == src_rect) ||
       (NULL == dst_rect))
    {
	return (1);
    }

    src_surface = image_desc_surface(pDesc);
    if(NULL == src_surface)
    {
	return (1);
    }

    if((src_rect->w != dst_rect->w) ||
       (src_rect->h != dst_rect->h))
    {
    hd_free_surface(src_surface);
	return (1);
    }

    hd_stretch_surface(src_surface, (GAL_Rect *)src_rect, surface, (GAL_Rect *)dst_rect);
    hd_free_surface(src_surface);

    return (0);
}


int gdi_image_data(void *data, int x, int y, int w, int h, int bpp)
{
	image_desc image = {0};

	if (data == NULL)
		return 1;

	image.data   = data;
	image.type   = BMP_TYPE;
	image.width  = w;
	image.height = h;
	image.bpp    = bpp;

	return hd_img_desc(screen, &image, x, y, w, h);
}

image_desc *gal_get_background(void)
{
	return back_desc;
}

void gal_set_default_desc(const image_desc * pDesc)
{
    back_desc = (image_desc *)pDesc;
}

int gal_background_desc(void *screen, const image_desc * pDesc)
{
	GuiCore *gui_core = NULL;
	int x = 0, y = 0, ret = 0;

	gal_set_default_desc((const image_desc *)pDesc);
	if(screen == NULL|| pDesc == NULL) {
		hd_img_clear();
		gdi_commit();
		return 1;
	}

	x = 0;// (gui.config.width - pDesc->width) / 2;
	y = 0;// (gui.config.height - pDesc->height) / 2;

	gui_core = GUI_GetCurrent();
	ret = hd_img_desc(screen, pDesc, x, y, gui_core->config.width, gui_core->config.height);

	return (ret);
}

int gal_get_img_flag(void)
{
	return spp_flag;
}

int gal_enable_img(int flag)
{
	spp_flag = flag == GUI_TRUE ? 1 : 0;

	return hd_enable_img(flag);
}

int gdi_image_enable(int flag)
{
	return gal_enable_img(flag);
}

int gal_rgb2yuv(int color)
{
	int y = 0, cb = 0, cr = 0;
	int r = 0, g = 0, b = 0;

	r = (color >> 16) & 0x000000FF;
	g = (color >>  8) & 0x000000FF;
	b = (color >>  0) & 0x000000FF;

	y  = (299 * r + 578 * g + 114 * b) / 1000;
	cb = (500 * r - 419 * g - 81  * b) / 1000 + 128;
	cr = (500 * b - 169 * r - 331 * g) / 1000 + 128;

	return ((y << 16) & 0x00FF0000) | ((cb << 8) & 0x0000FF00) | (cr & 0x000000FF);
}

int image_free(image_desc *desc)
{
	return 0;
}

void images_release_data(void *data)
{
	if (data)
		gal_img_release((image_desc *)data);
}

void logo_Free(void *data)
{
	GUI_FREE(data);
}

int GDI_PlayImage(const char *filename, int screen_width, int screen_height)
{
	GuiCore *gui_core = NULL;

	if (filename == NULL)
		return 1;

	gui_core = GUI_GetCurrent();
	if (screen_width <= 0)
		screen_width = gui_core->config.width;

	if (screen_height <= 0)
		screen_height = gui_core->config.height;

	gal_set_default_desc(NULL);

	hd_set_layer_surface(LAYER_MAIN_SPP_SURFACE);
	return gal_img_file(spp_screen, filename, 0, 0, screen_width, screen_height);
}

int GDI_PlayImage_Cordinate(const char *filename, unsigned int x, unsigned int y)
{
	image_desc *image = NULL;
	int ret = 0;
	unsigned char *pout = NULL;
	GIF_Para *gif_image = NULL;
	GIF_Image *gif_slice = NULL;

	image = gal_img_load(NULL, filename);
	if(image == NULL)
		return 1;

	if (BMP_TYPE == image->type || PNG_TYPE == image->type) {
		pout = (unsigned char *)GUI_MALLOC(image->width * image->height * 2);
		if(NULL == pout)
			return 1;

		memset(pout, 0, image->width * image->height * 2);
		convert_rgb_2_yuv422((void*)(image->data), pout, image->width, image->height, image->bpp, image->pal);
		GUI_FREE(image->data);
		GUI_FREE(pout);
	}
	else if(GIF_TYPE == image->type) {
		gif_image = (GIF_Para *)(image->data);
		if(NULL == gif_image)
			return 1;

		pout = (unsigned char *)GUI_MALLOC(image->width * image->height * 2);
		if(NULL == pout)
			return 1;

		memset(pout, 0, image->width * image->height * 2);

		gif_slice = gif_image->each_image;
		convert_rgb_2_yuv422((void*)(gif_slice->img_data),
				pout, image->width,
				image->height,
				image->bpp,
				gif_slice->img_pal);
		GUI_FREE(image->data);
		GUI_FREE(pout);
	}

    hd_set_layer_surface(LAYER_MAIN_SPP_SURFACE);
	ret = hd_img_desc(spp_screen, image, x, y, image->width, image->height);
	hd_commit_blit();

	gal_img_release(image);

	return ret;
}

int GDI_StopImageNoClear(const char *filename)
{
    int ret = 0;

    ret = gal_img_stop0(filename);

    return (ret);
}

int GDI_StopImage(const char *filename)
{
    int ret = 0;

    ret = gal_img_stop(filename);

    return (ret);
}

int GDI_ImageInformation(const char *filename, Image_Exif *img_info)
{
	struct image_file_node *image_ops;

	if((NULL == filename) || (NULL == img_info))
		return 1;

	image_ops = gal_find_node(filename);
	if (image_ops && image_ops->img_get_info_cb)
		image_ops->img_get_info_cb(filename, img_info);

	return 0;
}

static unsigned char GetYUVR(int nY,int nU,int nV)
{
	int nR;    //V=nV-128
	nR = nY + (nV - 128) + ((nV -128) * 103 >> 8);
	//overflow?
	if( nR > 255 )
		nR = 255;
	if( nR < 0 )
		nR = 0;
	return nR;
}

static unsigned char GetYUVG(int nY,int nU,int nV)
{
	int nG;    //U = nU-128
	nG = nY - (((nU -128) * 88 >> 8) + ((nV - 128) * 183 >> 8));
	//overflow?
	if( nG > 255 )
		nG = 255;
	if( nG < 0 )
		nG = 0;
	return nG;
}

static unsigned char GetYUVB(int nY,int nU,int nV)
{
	int nB;
	nB = nY + (nU - 128) + ((nU - 128) * 198 >> 8);
	//overflow?
	if( nB > 255 )
		nB = 255;
	if( nB < 0 )
		nB = 0;
	return nB;
}

static unsigned short EB2EL(unsigned short pixel)
{
    return (((pixel & 0xFF00) >> 8) | ((pixel & 0x00FF) << 8));
}

int GDI_ColorSpaceConvert(image_desc *pDesc)
{
	unsigned char *src = NULL, *dst = NULL;
	unsigned int i = 0, j = 0, width = 0, height = 0, size = 0;
	unsigned int y0_vector = 0, y1_vector = 0, u_vector = 0, v_vector = 0;
	unsigned char r_vector = 0, g_vector = 0, b_vector = 0;
	unsigned short *ptr = NULL;
	GuiCore *gui_core = NULL;

	if (pDesc == NULL)
		return 1;

	src = pDesc->data;
	width = pDesc->width;
	height = pDesc->height;
	gui_core = GUI_GetCurrent();
	size = width * height * gui_core->config.bpp / 8;
	dst = (unsigned char *)GUI_MALLOC(size);
	if(dst == NULL)
		return 1;

	memset(dst, 0, size);
	switch(gui_core->config.bpp) {
		case 8:
			break;
		case 16:
			ptr = (unsigned short *)dst;
			for(i = 0; i < height; i++) {
				for(j = 0; j < width * 2;) {
					y0_vector = src[i * width * 2 + j + 1];
					u_vector = src[i * width * 2 + j];
					j += 2;
					if(j < (width * 2)) {
						v_vector = src[i * width * 2 + j];
						y1_vector = src[i * width * 2 + j + 1];
						j += 2;
					}
					else {
						v_vector = src[i * width * 2 + j - 3];
						y1_vector = y0_vector;
					}

					b_vector = GetYUVB(y0_vector, u_vector, v_vector);
					g_vector = GetYUVG(y0_vector, u_vector, v_vector);
					r_vector = GetYUVR(y0_vector, u_vector, v_vector);

					*ptr = (((r_vector & 0xF8) << 8) |
						((g_vector & 0xFC) << 3) |
						(b_vector) >> 3);

					if(!check_cpu())
					{
					    *ptr = EB2EL((*ptr));
					}

					ptr++;

					if(j < (width * gui_core->config.bpp / 8 + 2)) {
						b_vector = GetYUVB(y1_vector, u_vector, v_vector);
						g_vector = GetYUVG(y1_vector, u_vector, v_vector);
						r_vector = GetYUVR(y1_vector, u_vector, v_vector);

						*ptr = (((r_vector & 0xF8) << 8) |
						        ((g_vector & 0xFC) << 3) |
							(b_vector) >> 3);

						if(!check_cpu())
						{
						    *ptr = EB2EL((*ptr));
						}

						ptr++;
					}
				}
			}

			pDesc->bpp = 16;
			break;
		case 24:
			break;
		case 32:
			break;
		default:
			break;
	}

	GUI_FREE(pDesc->data);
	pDesc->data = dst;

	return 0;
}

int GDI_Load_Image(const char *path,
		unsigned int *width,
		unsigned int *height,
		unsigned char *bpp,
		unsigned char **data)
{
	image_desc *image = NULL;
	unsigned char *pout = NULL;
	GIF_Para *gif_image = NULL;
	GIF_Image *gif_slice = NULL;


	if((NULL == path) || (NULL == width) || (NULL == height) || (NULL == bpp))
		return 0;

	image = gal_img_load(NULL, path);
	if(NULL == image)
		return 0;

	*width = image->width;
	*height = image->height;

	if(JPEG_TYPE == image->type)
		*bpp = 16;
	else if((BMP_TYPE == image->type) || (PNG_TYPE == image->type)) {
		pout = (unsigned char *)GUI_MALLOC(image->width * image->height * 2);
		if(NULL == pout)
			return 1;

		memset(pout, 0, image->width * image->height * 2);

		convert_rgb_2_yuv422((void*)(image->data), pout, image->width, image->height, image->bpp, image->pal);
		gal_img_free_memory(image);
		image->data = pout;
	}
	else if(GIF_TYPE == image->type) {
		gif_image = (GIF_Para *)(image->data);
		if(NULL == gif_image)
			return 1;

		pout = (unsigned char *)GUI_MALLOC(image->width * image->height * 2);
		if(NULL == pout)
			return 1;

		memset(pout, 0, image->width * image->height * 2);

		gif_slice = gif_image->each_image;
		convert_rgb_2_yuv422((void*)(gif_slice->img_data),
				pout,
				image->width,
				image->height,
				image->bpp,
				gif_slice->img_pal);
		gal_img_free_memory(image);
		image->data = pout;
	}
	else
		*bpp = image->bpp;

	*data = (unsigned char *)image->data;

	GUI_FREE(image->filename);
	GUI_FREE(image->pal);
	GUI_FREE(image);

	return 0;

}

static GuiSurface *_any_to_32bpp_surface(image_desc *pdesc)
{
	GuiSurface *src_surface = NULL, *surface = NULL;
	void *buffer = NULL;
	uint8_t *data = NULL, r = 0, g = 0, b = 0, a = 0;
	int i = 0, size = 0, color = 0;
	GAL_Rect rect = {0};

	rect.x = rect.y = 0;
	rect.w = pdesc->width;
	rect.h = pdesc->height;
	buffer = Gx_VideoMemoryMalloc(rect.w * rect.h * 32 / 8, 0);
	surface = hd_get_surface(&rect,
							 32,
							 Gx_VideoMemoryMapToKernel(buffer),
							 GUI_TRUE);
	surface->data = buffer;
	size = pdesc->width * pdesc->height * pdesc->bpp / 8;
	if(NULL == surface)
		return (NULL);

	if(pdesc->bpp == 8)
	{
		int *pal = NULL, index = 0;

		pal = pdesc->pal;
		if(NULL == pal)
			return (NULL);

		data = surface->data;
		for(i = 0; i < size; i++)
		{
			index = ((uint8_t *)pdesc->data)[i];
			color = pal[index];
			if(pdesc->type == BMP_TYPE) {
				b = (color >> 16) & 0x000000FF;
				g = (color >> 8) & 0x000000FF;
				r = color & 0x000000FF;
			} else {
				r = (color >> 16) & 0x000000FF;
				g = (color >> 8) & 0x000000FF;
				b = color & 0x000000FF;
			}
			a = 0xFF;
			data[i * 4] = r;
			data[i * 4 + 1] = g;
			data[i * 4 + 2] = b;
			data[i * 4 + 3] = a;
		}
	}
	else if(pdesc->bpp == 24)
	{
		data = surface->data;
		for(i = 0; i < size;)
		{
			a = 0xFF;
			r = ((uint8_t *)pdesc->data)[i];
			g = ((uint8_t *)pdesc->data)[i + 1];
			b = ((uint8_t *)pdesc->data)[i + 2];
			data[i / 3 * 4] = r;
			data[i / 3 * 4 + 1] = g;
			data[i / 3 * 4 + 2] = b;
			data[i / 3 * 4 + 3] = a;
			i += 3;
		}
	}
	else
	{
		GAL_Rect src_rect = {0}, dst_rect = {0};

		src_surface = image_desc_surface(pdesc);
		src_rect.x = src_rect.y = dst_rect.x = dst_rect.y = 0;
		src_rect.w = dst_rect.w = pdesc->width;
		src_rect.h = dst_rect.h = pdesc->height;
		hd_copy_surface(src_surface, &src_rect, surface, &dst_rect);
		hd_free_surface(src_surface);
	}

	return (surface);
}

static int _get_gif_desc(image_desc *src_desc, image_desc *dst_desc, int frame_index)
{
	GIF_Para *gif_para = NULL;
	GIF_Image *each_image = NULL;
	int i = 0;

	gif_para = (GIF_Para *)src_desc->data;
	if(NULL == gif_para)
		return -1;

	each_image = gif_para->each_image;
	while(each_image)
	{
		if(i == frame_index)
			break;
		each_image = each_image->next;
		i++;
	}

	if(i == frame_index)
	{
		dst_desc->type = GIF_TYPE;
		dst_desc->width = each_image->img_width;
		dst_desc->height = each_image->img_height;
		dst_desc->bpp = 8;  //GIF default
		if(each_image->img_pal)
		{
			dst_desc->pal = each_image->img_pal;
		}
		else
		{
			dst_desc->pal = gif_para->pal;
		}
		dst_desc->data = each_image->img_data;
		return 0;
	}
	else
		return -1;
}

static void _init_device(void)
{
	if(0 == g_device_handle)
	{
		g_device_handle = GxAvdev_CreateDevice(0);
		if(g_device_handle <= 0) {
			return;
		}
	}

	if(0 == vpu_handle)
	{
		vpu_handle = GxAvdev_OpenModule(g_device_handle, GXAV_MOD_VPU, 0);
		if(0 == vpu_handle) {
			return;
		}
	}

	if(0 == jpeg_handle)
	{
		jpeg_handle = GxAvdev_OpenModule(g_device_handle, GXAV_MOD_JPEG_DECODER, 0);
		if(0 == jpeg_handle) {
			return;
		}
	}
}

int GDI_DecodeImage(const char *path,
		unsigned int index,
		unsigned int width,
		unsigned int height,
		unsigned int pitch,
		GxColorFormat color_type,
		unsigned char *data)
{
	image_desc *p_img_desc = NULL, tmp_desc = {0};
	GuiSurface *surface = NULL, *zoom_surface = NULL, *dst_surface = NULL;
	void *zoom_buffer = NULL, *dst_buffer = NULL;
	GAL_Rect src_rect = {0}, dst_rect = {0};
	int ret = 0;

	_init_device();

	p_img_desc = gal_img_load(NULL, path);
	if(NULL == p_img_desc)
		return (1);

	// 高色位一律不向8bpp索引色转，没法转
	if((8 != p_img_desc->bpp) && (GX_COLOR_FMT_CLUT8 == color_type))
		return (1);

	memset(&tmp_desc, 0, sizeof(image_desc));
	//Anyformat=>32bpp
	if(GIF_TYPE == p_img_desc->type)
	{
		ret = _get_gif_desc(p_img_desc, &tmp_desc, index);
		if(ret)
			return -1;
	}
	else
	{
		tmp_desc = *p_img_desc;
	}

	surface = _any_to_32bpp_surface(&tmp_desc);
	if(NULL == surface)
		return (1);
	//32bpp=>zoom
	src_rect.x = src_rect.y = dst_rect.x = dst_rect.y = 0;
	src_rect.w = tmp_desc.width;
	src_rect.h = tmp_desc.height;
	dst_rect.w = width;
	dst_rect.h = height;
	if((p_img_desc->width != width) ||
	   (p_img_desc->height != height))
	{
		gdi_begin();
		zoom_buffer = Gx_VideoMemoryMalloc(dst_rect.w * dst_rect.h * 32 / 8, 0);
		zoom_surface = hd_get_surface(&dst_rect,
									  32,
									  Gx_VideoMemoryMapToKernel(zoom_buffer),
									  GUI_TRUE);
		if(NULL == zoom_surface)
		{
			gal_img_release(p_img_desc);
			return (1);
		}
		zoom_surface->data = zoom_buffer;
		ret = hd_zoom_surface(surface, &src_rect, zoom_surface, &dst_rect);
		gdi_end();
		Gx_VideoMemoryFree(surface->data);
		hd_free_surface(surface);
		surface = zoom_surface;
	}
	//Blit=>color_type
	dst_buffer = Gx_VideoMemoryMalloc(pitch * dst_rect.h, 0);
	dst_surface = hd_get_surface0(&dst_rect,
								  8 * (pitch / width),
								  color_type,
								  Gx_VideoMemoryMapToKernel(dst_buffer),
								  GUI_TRUE);
	if(NULL == dst_surface)
	{
		if(surface)
		{
			Gx_VideoMemoryFree(surface->data);
			hd_free_surface(surface);
		}
		gal_img_release(p_img_desc);
		return (1);
	}
	dst_surface->data = dst_buffer;
	ret = hd_copy_surface(surface, &dst_rect, dst_surface, &dst_rect);
	//Get data
	memcpy(data, dst_surface->data, pitch * height);
	//Free
	Gx_VideoMemoryFree(surface->data);
	hd_free_surface(surface);
	Gx_VideoMemoryFree(dst_surface->data);
	hd_free_surface(dst_surface);
	gal_img_release(p_img_desc);

	return (ret);
}

status_t gal_add_key_path(const char *key, const char *path)
{
	GuiCore *gui_core = NULL;

    if((NULL == key) || (NULL == path))
	return GXCORE_ERROR;

	gui_core = GUI_GetCurrent();
    path_image_cache_put(gui_core->config.image_cache, key, path, GUI_FALSE);

    return GXCORE_SUCCESS;
}

status_t gal_check_imagekey(const char *key)
{
	GuiCore *gui_core = NULL;
	GuiConfig *config = NULL;
	int count = 0, i = 0;

	if(NULL == key)
		return GXCORE_ERROR;

	gui_core = GUI_GetCurrent();
	if(gui_core) {
		if((gui_core->resource_config_table) && (gui_core->resource_config_stack)) {
			count = stack_count(gui_core->resource_config_stack);
			for(i = 0; i < count; i++) {
				config = (GuiConfig *)stack_get(gui_core->resource_config_stack, i);
				if(config && !config->invalid && config->image_cache) {
					if(hash_get(((ImageCache *)config->image_cache)->hash_imgs, key))
						return GXCORE_SUCCESS;
				}
			}
		} else {
			config = &(gui_core->config);
			if(config->image_cache) {
				if(hash_get(((ImageCache *)config->image_cache)->hash_imgs, key))
					return GXCORE_SUCCESS;
			}
		}
	}

	return GXCORE_ERROR;
}

status_t gal_add_image(const char *key, const char *path)
{
	GuiCore *gui_core = NULL;

    if((NULL == key) || (NULL == path))
	return GXCORE_ERROR;

	gui_core = GUI_GetCurrent();
    image_cache_put(&(gui_core->config), key, path, GUI_FALSE);

    return GXCORE_SUCCESS;
}

static void _add_palette(GuiConfig *config, const char *path)
{
    handle_t fhandle = 0;
    char temp_str[10] = {0}, a = 0, r = 0, g = 0, b = 0;
    unsigned short length = 0;
    int pal_data[256] = {0}, i = 0;

    fhandle =  GxCore_Open(path, "r");
    if(fhandle < 0)
	return;

    GxCore_Read(fhandle, temp_str, 4, 1);
    temp_str[4] = '\0';
    if(strcasecmp("RIFF", temp_str))
	return;

    GxCore_Seek(fhandle, 4, GX_SEEK_CUR);
    GxCore_Read(fhandle, temp_str, 8, 1);
    temp_str[8] = '\0';
    if(strcasecmp("PAL data", temp_str))
	return;

    GxCore_Seek(fhandle, 6, GX_SEEK_CUR);
    GxCore_Read(fhandle, &length, 2, 1);
    GxCore_Read(fhandle, pal_data, length * sizeof(int), 1);

    if(NULL == config->clut)
    {
	config->firstclut = config->clut = (GuiClut *)GUI_MALLOCZ(sizeof(GuiClut));
	if(NULL == config->clut)
	{
	    GxCore_Close(fhandle);
	    return;
	}

	config->clut->num = length;
	config->clut->palette = (Color* )GUI_MALLOCZ(length * sizeof(Color));
	if(NULL == config->clut->palette)
	{
	    GxCore_Close(fhandle);
	    return;
	}
	/*Convert For Photoshop standard*/
	for(i = 0; i < length; i++)
	{
	    a = ((pal_data[i] & 0xFF000000) >> 24) & 0x000000FF;
	    b = ((pal_data[i] & 0x00FF0000) >> 16) & 0x000000FF;
	    g = ((pal_data[i] & 0x0000FF00) >> 8) & 0x000000FF;
	    r = (pal_data[i] & 0x000000FF) & 0x000000FF;
	    pal_data[i] = (a << 24) | (r << 16) | (g << 8) | b;
	}

	memcpy(config->clut->palette, pal_data, length * sizeof(int));
    }

    GxCore_Close(fhandle);
}

int gal_add_palette(GuiConfig* config, const char *pal_name)
{
    char *full_name = NULL;

    if((NULL == config) || (NULL == pal_name))
    {
	return (1);
    }

    full_name = gui_mallocpath_swapfile(config->files.file_xml_image, pal_name);
    if(NULL == full_name)
    {
	return (1);
    }

    _add_palette(config, full_name);

    GUI_FREE(full_name);

    return (0);
}

static void _yuv2rgb(unsigned char y, unsigned char u, unsigned char v, int *r, int *g, int *b)
{
	short tu = 0, tv = 0;

	tu = u - 128;
	tv = v - 128;

	*r = y + (114 * tv) / 100;
	*g = y - (39 * tu + 58 * tv) / 100;
	*b = y + (203 * tu) / 100;
}

static void _ycbcr2rgb(unsigned char y, unsigned char cb, unsigned char cr, int *r, int *g, int *b)
{
	short tcr = 0, tcb = 0;

	tcb = cb - 128;
	tcr = cr - 128;

	*r = y + (137 * tcr) / 100;
	*g = y - (69 * tcr + 33 * tcb) / 100;
	*b = y + (173 * tcb) / 100;

	return;
}

static status_t _create_capture_surface(GxVpuProperty_CreateSurface *CreateSurface, GAL_Rect *rect, GxLayerID layer)
{
	uint32_t size = 0;
	unsigned char *temp_buffer = NULL;

	if(NULL == CreateSurface || NULL == rect)
		return GXCORE_ERROR;

	memset(CreateSurface, 0, sizeof(GxVpuProperty_CreateSurface));

	CreateSurface->format = GX_COLOR_FMT_YCBCR422;
	size = rect->w * rect->h * 2;

	CreateSurface->width = rect->w;
	CreateSurface->height = rect->h;
	CreateSurface->mode = GX_SURFACE_MODE_IMAGE;
	CreateSurface->buffer = NULL;

	if(GxAVGetProperty(g_device_handle, vpu_handle, GxVpuPropertyID_CreateSurface, CreateSurface, sizeof(GxVpuProperty_CreateSurface)) < 0)
	{
		gxlogd("Create surface error\n");
		return GXCORE_ERROR;
	}

	temp_buffer = (uint8_t *)GxCore_Map(g_device_handle, (uint32_t)CreateSurface->buffer, size);
	memset(temp_buffer, 0, size);
	GxCore_UnMap(g_device_handle, temp_buffer, size);

	return GXCORE_SUCCESS;
}

static void _convert_yuv420_to_argb(GuiSurface *surface, unsigned char *src_buffer, unsigned char *dst_buffer, int is_bgr)
{
	int i = 0, j = 0;
	int r = 0, g = 0, b = 0;
	unsigned char y = 0, u = 0, v = 0;
	unsigned char *y_buffer = NULL, *uv_buffer = NULL;

	if(NULL == surface || NULL == src_buffer || NULL == dst_buffer)
		return;

	y_buffer = src_buffer;
	uv_buffer = y_buffer + surface->sf.width * surface->sf.height;

	for(i = 0; i < surface->sf.height; i++)
	{
		for(j = 0; j < surface->sf.width; j++)
		{
			y = y_buffer[i * surface->sf.width + j];
			u = uv_buffer[(i / 2) * surface->sf.width + (j / 2) * 2];
			v = uv_buffer[(i / 2) * surface->sf.width + (j / 2) * 2 + 1];
			_yuv2rgb(y, u, v, &r, &g, &b);
			if(is_bgr) {
				dst_buffer[(i * surface->sf.width + j) * 4] = b;
				dst_buffer[(i * surface->sf.width + j) * 4 + 1] = g;
				dst_buffer[(i * surface->sf.width + j) * 4 + 2] = r;
				dst_buffer[(i * surface->sf.width + j) * 4 + 3] = 0xFF;
			} else {
				dst_buffer[(i * surface->sf.width + j) * 4] = 0xFF;
				dst_buffer[(i * surface->sf.width + j) * 4 + 1] = r;
				dst_buffer[(i * surface->sf.width + j) * 4 + 2] = g;
				dst_buffer[(i * surface->sf.width + j) * 4 + 3] = b;
			}
		}
	}


	return;
}

static void _convert_ycbcr422_to_argb(GuiSurface *surface, unsigned char *src_buffer, unsigned char *dst_buffer, int is_bgr)
{
	int i = 0, j = 0, z = 0;
	int r = 0, g = 0, b = 0;
	unsigned char *uyuv_buffer = NULL;
	unsigned char y1 = 0, y2 = 0, cb = 0, cr = 0;

	if(NULL == surface || NULL == src_buffer || NULL == dst_buffer)
		return;

	for(i = 0; i < surface->sf.height; i++)
	{
		for(j = 0; j < surface->sf.width / 2; j++)
		{
			uyuv_buffer = src_buffer + (i * surface->sf.width * 2 + 4 * j);
			y1 = uyuv_buffer[1];
			y2 = uyuv_buffer[3];
			cb = uyuv_buffer[0];
			cr = uyuv_buffer[2];

			_ycbcr2rgb(y1, cb, cr, &r, &g, &b);
			if(is_bgr) {
				dst_buffer[4 * z] = b;
				dst_buffer[4 * z + 1] = g;
				dst_buffer[4 * z + 2] = r;
				dst_buffer[4 * z + 3] = 0xFF;
			} else {
				dst_buffer[4 * z] = 0xFF;
				dst_buffer[4 * z + 1] = r;
				dst_buffer[4 * z + 2] = g;
				dst_buffer[4 * z + 3] = b;
			}
			z++;

			_ycbcr2rgb(y2, cb, cr, &r, &g, &b);
			if(is_bgr) {
				dst_buffer[4 * z] = b;
				dst_buffer[4 * z + 1] = g;
				dst_buffer[4 * z + 2] = r;
				dst_buffer[4 * z + 3] = 0xFF;
			} else {
				dst_buffer[4 * z] = 0xFF;
				dst_buffer[4 * z + 1] = r;
				dst_buffer[4 * z + 2] = g;
				dst_buffer[4 * z + 3] = b;
			}
			z++;
		}
	}

	return;
}

static GuiSurface *_capture_surface(GAL_Rect *rect, GxLayerID layer, int svpu_cap)
{
	int size = 0, ret = 0;
	GuiCore *gui_core = NULL;
	GuiSurface *surface = NULL;
	GxVpuProperty_LayerCapture Capture = {0};
	GxVpuProperty_CreateSurface CreateSurface = {0};
	GxVpuProperty_DestroySurface DesSurface = {0};
	unsigned char *src_buffer = NULL, *dst_buffer = NULL;


	if(GXCORE_ERROR == _create_capture_surface(&CreateSurface, rect, layer))
		return NULL;

	gui_core = GUI_GetCurrent();

	Capture.layer = layer;
	Capture.surface = CreateSurface.surface;
	Capture.rect.x = 0;
	Capture.rect.y = 0;
	if(svpu_cap) {
		Capture.rect.width  = 720;
		Capture.rect.height = 576;
		Capture.svpu_cap    = GUI_TRUE;
	} else {
		Capture.rect.width  = gui_core->config.width;
		Capture.rect.height = gui_core->config.height;
		Capture.svpu_cap    = GUI_FALSE;
	}

	ret = GxAVGetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_LayerCapture,
			&Capture,
			sizeof(GxVpuProperty_LayerCapture));
	if(ret)
		return (NULL);

	surface = hd_get_surface(rect, 32, NULL, GUI_TRUE);
	if(NULL == surface)
	{
		memset(&DesSurface, 0, sizeof(DesSurface));
		DesSurface.surface = CreateSurface.surface;
		ret = GxAVSetProperty(g_device_handle,
				vpu_handle,
				GxVpuPropertyID_DestroySurface,
				&DesSurface,
				sizeof(GxVpuProperty_DestroySurface));
		return (NULL);
	}

	if(GX_COLOR_FMT_YCBCR422 == CreateSurface.format)
		size = CreateSurface.width * CreateSurface.height * 2;
	else if(GX_COLOR_FMT_YCBCR420_Y_UV == CreateSurface.format)
		size = CreateSurface.width * CreateSurface.height + CreateSurface.width * CreateSurface.height / 2;
	else
		return NULL;

	src_buffer = (uint8_t *)GxCore_Map(g_device_handle, (uint32_t)CreateSurface.buffer, size);
	dst_buffer = (uint8_t *)(surface->data);

	if(GX_COLOR_FMT_YCBCR422 == CreateSurface.format)
		_convert_ycbcr422_to_argb(surface, src_buffer, dst_buffer, GUI_TRUE);
	else if(GX_COLOR_FMT_YCBCR420_Y_UV == CreateSurface.format)
		_convert_yuv420_to_argb(surface, src_buffer, dst_buffer, GUI_TRUE);
	else
		return NULL;

	GxCore_UnMap(g_device_handle, src_buffer, size);

	memset(&DesSurface, 0, sizeof(DesSurface));
	DesSurface.surface = CreateSurface.surface;
	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_DestroySurface,
			&DesSurface,
			sizeof(GxVpuProperty_DestroySurface));
	if(ret)
	{
		hd_free_surface(surface);
		return (NULL);
	}

	return (surface);
}

static void _save_as_bmp(const unsigned char *temp_path, GuiSurface *surface)
{
    unsigned char *temp_file = NULL;
    FILE *file = NULL;
    unsigned char *fill = NULL;
    unsigned short word = 0;
    unsigned int size = 0, dword = 0, i = 0, offset = 0;

    if((NULL == temp_path) || (NULL == surface))
    {
	return;
    }

    temp_file = (unsigned char *)temp_path;

    file = fopen((char *)temp_file, "w");
    if(NULL == file)
    {
	return;
    }

    fwrite("BM", 2, 1, file);
    offset = (((surface->sf.width * 24)+31)>>5)<<2;
    size = 0x36 + offset * surface->sf.height;
    fwrite(&size, 4, 1, file);
    dword = 0;
    fwrite(&dword, 4, 1, file);
    dword = 0x36;
    fwrite(&dword, 4, 1, file);
    dword = 0x28;
    fwrite(&dword, 4, 1, file);
    dword = surface->sf.width;
    fwrite(&dword, 4, 1, file);
    dword = surface->sf.height;
    fwrite(&dword, 4, 1, file);
    word = 1;
    fwrite(&word, 2, 1, file);
    word = 24;
    fwrite(&word, 2, 1, file);
    dword = 0;
    fwrite(&dword, 4, 1, file);
    dword = (((surface->sf.width * 24)+31)>>5)<<2;
    dword = dword * surface->sf.height;
    fwrite(&dword, 4, 1, file);
    dword = surface->sf.width;
    fwrite(&dword, 4, 1, file);
    dword = surface->sf.height;
    fwrite(&dword, 4, 1, file);
    dword = 2 << 24;
    fwrite(&dword, 4, 1, file);
    dword = 0;
    fwrite(&dword, 4, 1, file);

    i = 0;
    offset = (((surface->sf.width * 24)+31)>>5)<<2;
    offset = offset - surface->sf.width * 3;
    if(offset)
    {
	fill = (unsigned char *)GUI_MALLOC(offset);
	if(NULL == fill)
	{
	    fclose(file);
	    return;
	}
	memset(fill, 0xFF, offset);
    }
    while(i < surface->sf.height)
    {
	fwrite(surface->data + (surface->sf.width * 3) * (surface->sf.height - i - 1), surface->sf.width * 3, 1, file);
	if(offset)
	{
	    fwrite(fill, offset, 1, file);
	}
	i++;
    }
    GUI_FREE(fill);

    fclose(file);
}

#ifdef JPEG_ENCODING
typedef struct cjpeg_source_struct * cjpeg_source_ptr;

struct cjpeg_source_struct {
    JMETHOD(void, start_input, (j_compress_ptr cinfo,
                                cjpeg_source_ptr sinfo));
    JMETHOD(JDIMENSION, get_pixel_rows, (j_compress_ptr cinfo,
                                         cjpeg_source_ptr sinfo));
    JMETHOD(void, finish_input, (j_compress_ptr cinfo,
                                 cjpeg_source_ptr sinfo));

    FILE *input_file;

    JSAMPARRAY buffer;
    JDIMENSION buffer_height;
};

static const char * const cdjpeg_message_table[] = {
    #include "cderror.h"
    NULL
};

//#define jinit_read_bmp          jIRdBMP
EXTERN(cjpeg_source_ptr) jinit_read_bmp JPP((j_compress_ptr cinfo));

LOCAL(cjpeg_source_ptr)
select_file_type (j_compress_ptr cinfo, FILE * infile)
{
    int c;

    if ((c = getc(infile)) == EOF)
	ERREXIT(cinfo, JERR_INPUT_EMPTY);
    if (ungetc(c, infile) == EOF)
        ERREXIT(cinfo, JERR_UNGETC_FAILED);

    switch (c) {
	case 'B':
	    return jinit_read_bmp(cinfo);
	default:
	    ERREXIT(cinfo, JERR_UNKNOWN_FORMAT);
	    break;
    }

    return NULL;                  /* suppress compiler warnings */
}

static void _save_as_jpeg(const unsigned char *temp_file, const unsigned char *output_file)
{
    FILE *infile = NULL, *outfile = NULL;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cjpeg_source_ptr src_mgr;
    JDIMENSION num_scanlines;

    if((NULL == temp_file) || (NULL == output_file))
    {
	return;
    }

    infile = fopen((char *)temp_file, "r");
    if(NULL == infile)
    {
	return;
    }

    outfile = fopen((char *)output_file, "w");
    if(NULL == outfile)
    {
	fclose(infile);
	return;
    }

    cinfo.err = jpeg_std_error(&jerr);
    cinfo.optimize_coding = TRUE;
    jpeg_create_compress(&cinfo);
    jerr.addon_message_table = cdjpeg_message_table;
    jerr.first_addon_message = JMSG_FIRSTADDONCODE;
    jerr.last_addon_message = JMSG_LASTADDONCODE;

    cinfo.in_color_space = JCS_RGB; /* arbitrary guess */
    jpeg_set_defaults(&cinfo);

    src_mgr = select_file_type(&cinfo, infile);
    src_mgr->input_file = infile;
    (*src_mgr->start_input) (&cinfo, src_mgr);
    jpeg_default_colorspace(&cinfo);

    jpeg_stdio_dest(&cinfo, outfile);
    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height){
	num_scanlines = (*src_mgr->get_pixel_rows) (&cinfo, src_mgr);
	(void) jpeg_write_scanlines(&cinfo, src_mgr->buffer, num_scanlines);
    }

    (*src_mgr->finish_input) (&cinfo, src_mgr);
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    fclose(infile);
    fclose(outfile);
}

status_t gal_save_bmp_to_jpeg(const unsigned char *bmp_path, const unsigned char *jpeg_path)
{
	_save_as_jpeg(bmp_path, jpeg_path);
	return (GXCORE_SUCCESS);
}

status_t gal_capture_jpeg(const unsigned char *temp_dir, const unsigned char *output_file, GAL_Rect *rect, GxLayerID layer)
{
	unsigned char *temp_file = NULL;
	GuiSurface *surface = NULL;

	if((NULL == temp_dir) || (NULL == output_file))
	{
		return (GXCORE_ERROR);
	}

	surface = _capture_surface(rect, layer, GUI_FALSE);
	if(NULL == surface)
	{
		return (GXCORE_ERROR);
	}

	temp_file = (unsigned char *)GUI_MALLOCZ(strlen((char *)temp_dir) + 20);
	if(NULL == temp_file)
	{
		return (GXCORE_ERROR);
	}

	sprintf((char *)temp_file, "%s1.bmp", temp_dir);
	_save_as_bmp(temp_file, surface);
	hd_free_surface(surface);

	_save_as_jpeg(temp_file, output_file);

	GxCore_FileDelete((char *)temp_file);
	GUI_FREE(temp_file);

	return (GXCORE_SUCCESS);
}

#endif /* JPEG_ENCODING */

GuiSurface * gal_capture_surface(int layer_id, GAL_Rect *src_rect, GAL_Rect *dst_rect)
{
	GuiSurface *surface = NULL;

	if(NULL == src_rect) {
		return (NULL);
	}

	surface = _capture_surface(src_rect, layer_id, GUI_FALSE);
	if(NULL == surface) {
		return (NULL);
	}

	if(dst_rect) {
		GuiSurface *temp_surface = NULL, *dst_surface = NULL;
		GAL_Rect temp_rect = {0};


		temp_rect.x = temp_rect.y = 0;
		temp_rect.w = src_rect->w;
		temp_rect.h = src_rect->h;

		temp_surface = hd_get_surface(&temp_rect, 32, NULL, GUI_TRUE);
		if(NULL == temp_surface) {
			return (surface);
		}
		hd_copy_surface(surface, &temp_rect, temp_surface, &temp_rect);

		dst_surface = hd_get_surface(dst_rect, 32, NULL, GUI_TRUE);
		if(NULL == dst_surface) {
			hd_free_surface(temp_surface);
			return (surface);
		}
		gdi_begin();
		hd_zoom_surface(temp_surface, &temp_rect, dst_surface, dst_rect);
		gdi_end();
		hd_free_surface(temp_surface);
		hd_free_surface(surface);
		surface = dst_surface;
	}

	return (surface);
}

GuiSurface * gal_capture_surface_sd(GAL_Rect *src_rect, GAL_Rect *dst_rect)
{
	GuiSurface *surface = NULL;

	if(NULL == src_rect) {
		return (NULL);
	}

	surface = _capture_surface(src_rect, GX_LAYER_VPP, GUI_TRUE);
	if(NULL == surface) {
		return (NULL);
	}

	if(dst_rect) {
		GuiSurface *temp_surface = NULL, *dst_surface = NULL;
		GAL_Rect temp_rect = {0};


		temp_rect.x = temp_rect.y = 0;
		temp_rect.w = src_rect->w;
		temp_rect.h = src_rect->h;

		temp_surface = hd_get_surface(&temp_rect, 32, NULL, GUI_TRUE);
		if(NULL == temp_surface) {
			return (surface);
		}
		hd_copy_surface(surface, &temp_rect, temp_surface, &temp_rect);

		dst_surface = hd_get_surface(dst_rect, 32, NULL, GUI_TRUE);
		if(NULL == dst_surface) {
			hd_free_surface(temp_surface);
			return (surface);
		}
		gdi_begin();
		hd_zoom_surface(temp_surface, &temp_rect, dst_surface, dst_rect);
		gdi_end();
		hd_free_surface(temp_surface);
		hd_free_surface(surface);
		surface = dst_surface;
	}

	return (surface);
}

static GuiSurface* _convert_color_32_to_24(GuiSurface *surface)
{
	GuiSurface *dst_surface = NULL;
	GAL_Rect rect = {0};
	int *data = NULL, i = 0, j = 0;
	unsigned char r = 0, g = 0, b = 0;

	rect.w = surface->sf.width;
	rect.h = surface->sf.height;
	dst_surface = hd_get_surface(&rect, 24, NULL, GUI_FALSE);
	if(NULL == dst_surface) {
		return (surface);
	}
	data = (int *)(surface->data);

	for(i = 0; i < surface->sf.height; i++) {
		for(j = 0; j < surface->sf.width; j++) {
			_32_to_24(data[i * surface->sf.width + j], &r, &g, &b);
			dst_surface->data[(i * surface->sf.width + j) * 3] = b;
			dst_surface->data[(i * surface->sf.width + j) * 3 + 1] = g;
			dst_surface->data[(i * surface->sf.width + j) * 3 + 2] = r;
		}
	}

	return (dst_surface);
}

status_t gal_save_surface_to_bmp(GuiSurface *surface, const unsigned char *path)
{
	GuiSurface *dst_surface = NULL;

	if(32 == surface->bpp) {
		dst_surface = _convert_color_32_to_24(surface);
	} else {
		dst_surface = surface;
	}
	_save_as_bmp(path, dst_surface);
	if(32 == surface->bpp)
		hd_free_surface(dst_surface);
	return (GXCORE_SUCCESS);
}

#else
#include "mini_gui_private.h"
#include "IMG.h"
#include "gui_core.h"
#include "color.h"
extern GuiCore gui;
static struct image_register_func s_img_register_func = {0};

static image_file_node nos_bmp_node = {
	.type            = BMP_TYPE,
	.file_type       = {"bmp", NULL},
	.img_cb          = NULL,
	.img_load_cb     = IMG_BMP_Load,
	.img_get_info_cb = NULL,
	.img_free        = NULL,
	.img_stop        = NULL,
};

static image_file_node nos_jpg_node = {
	.type            = JPEG_TYPE,
	.file_type       = {"jpg", "jpeg", NULL},
	.img_cb          = NULL,
	.img_load_cb     = IMG_JPEG_Load,
	.img_get_info_cb = NULL,
	.img_free        = NULL,
	.img_stop        = NULL,
};
#define IMG_NODE_MAX 10
static image_file_node *img_node[IMG_NODE_MAX] = {
	&nos_bmp_node, &nos_jpg_node, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL
};
#endif /*NO_OS*/

image_desc *gal_img_new(void)
{
	image_desc *desc = GUI_MALLOCZ(sizeof(image_desc));

	if (desc) {
		desc->type = UNKOWN_TYPE;
		GX_INIT_LIST_HEAD(&desc->head);
	}

	return desc;
}

static int _convert_pixel_4_to_16(const image_desc *image, int i, int j)
{
	GuiCore *gui_core = NULL;
	unsigned char index, icur;
	GuiClut *clut = NULL;
	Color *palette = NULL;
	static int flag;

	gui_core = GUI_GetCurrent();
	index = ((char *)(image->data))[(i * image->width + j) / 2];
	if(flag % 2)
		index = index & 0x0F;
	else
		index = ((index & 0xF0) >> 4);

	flag ^= 1;

	clut = gui_core->config.firstclut;

	icur = 0;
	while(icur < image->pal_index) {
		clut = clut->next;
		icur++;
	}

	if(clut)
		palette = clut->palette;
	else
		return 0;

	if(palette)
		return _8_to_565(palette, index);
	else
		return 0;
}

static int _convert_pixel_8_to_16(const image_desc *image, int i, int j)
{
	GuiCore *gui_core = NULL;
	unsigned char index, icur;
	GuiClut *clut = NULL;
	Color *palette = NULL;

	gui_core = GUI_GetCurrent();
	index = ((char *)(image->data))[i * image->width + j];
	clut = gui_core->config.firstclut;

	icur = 0;
	while(icur < image->pal_index) {
		clut = clut->next;
		icur++;
	}

	if(clut)
		palette = clut->palette;
	else
	{
		return (_24_to_32(index, index, index));
	}

	if(palette)
		return _8_to_565(palette, index);
	else
		return 0;
}

static int _convert_pixel_24_to_16(const image_desc *image, int i, int j)
{
	unsigned char r, g, b;
	unsigned char *data = (unsigned char *)image->data;

	r = data[i * image->width * 3 + j * 3 + 0];
	g = data[i * image->width * 3 + j * 3 + 1];
	b = data[i * image->width * 3 + j * 3 + 2];

	return _24_to_565(r, g, b);
}

static int _convert_pixel_32_to_16(const image_desc *image, int i, int j)
{
	unsigned int *data = (unsigned int *)image->data;

	return _32_to_565(data[i * image->width + j]);
}

static int _convert_16bpp(image_desc *image)
{
	unsigned short *pData = NULL;
	unsigned int size, i, j;
	image_covert func;

	size = image->width * image->height * sizeof(unsigned short);

	pData = (unsigned short *)GUI_MALLOC(size);
	if(NULL == pData)
	{
		return (0);
	}

	memset(pData, 0, size);

	switch(image->bpp) {
		case IMAGE_4_BPP:
			func = _convert_pixel_4_to_16;
			break;
		case IMAGE_8_BPP:
			func = _convert_pixel_8_to_16;
			break;
		case IMAGE_24_BPP:
			func = _convert_pixel_24_to_16;
			break;
		case IMAGE_32_BPP:
			func = _convert_pixel_32_to_16;
			break;
		default:
			break;
	}

	if(NULL == func)
		return 0;

	for(i = 0; i < image->height; i++)
		for(j = 0; j < image->width; j++)
			pData[i * image->width + j] = func(image, i, j);

	gal_img_free_memory(image);
	image->data = pData;
	image->size = size;

	return 0;
}

static int _convert_pixel_8_to_24(const image_desc *image, int i, int j)
{
	unsigned int icur;
	unsigned char index, r = 0, g = 0, b = 0;
	GuiCore *gui_core = NULL;
	GuiClut *clut = NULL;
	Color *palette = NULL;
	unsigned char *data = (unsigned char *)image->data;

	gui_core = GUI_GetCurrent();
	index = data[i * image->width + j];
	clut = gui_core->config.firstclut;

	icur = 0;
	while(icur < image->pal_index) {
		clut = clut->next;
		icur++;
	}

	if(clut)
		palette = clut->palette;
	else
		return 0;

	_8_to_24(palette, index, &r, &g, &b);

	return RGB32(r, g, b);
}

static int _convert_pixel_16_to_24(const image_desc *image, int i, int j)
{
	unsigned char r = 0, g = 0, b = 0;
	unsigned short *data = (unsigned short *)image->data;

	_565_to_24(data[i * image->width + j], &r, &g, &b);

	return RGB32(r, g, b);
}

static int _convert_pixel_32_to_24(const image_desc *image, int i, int j)
{
	unsigned char r = 0, g = 0, b = 0;
	unsigned int *data = (unsigned int*) image->data;

	_32_to_24(data[i * image->width + j], &r, &g, &b);

	return RGB32(r, g, b);
}

static int _convert_24bpp(image_desc *image)
{
	unsigned char *pData = NULL;
	unsigned int size = 0, i = 0, j = 0, color = 0;
	image_covert func;

	size = image->width * image->height * 3;

	pData = (unsigned char *)GUI_MALLOC(size);
	if(NULL == pData)
		return 1;

	memset(pData, 0, size);

	switch(image->bpp) {
		case IMAGE_8_BPP:
			func = _convert_pixel_8_to_24;
			break;
		case IMAGE_16_BPP:
			func = _convert_pixel_16_to_24;
			break;
		case IMAGE_32_BPP:
			func = _convert_pixel_32_to_24;
			break;
	}

	for(i = 0; i < image->height; i++) {
		for(j = 0; j < image->width * 3; j+= 3) {
			color = func(image, i, j);
			pData[i * image->width * 3 + j + 0] = color >> 16;
			pData[i * image->width * 3 + j + 1] = color >> 8;
			pData[i * image->width * 3 + j + 2] = color >> 0;
		}
	}

	GUI_FREE(image->data);
	image->data = pData;

	return 0;
}

static int _convert_pixel_4_to_32(const image_desc *image, int i, int j)
{
	unsigned char index = 0;
	GuiCore *gui_core = NULL;
	GuiClut *clut = NULL;
	Color *palette = NULL;
	unsigned char *data = (unsigned char *)image->data;
	static int flag;

	gui_core = GUI_GetCurrent();
	index = ((char *)(data))[(i * image->width + j) / 2];
	if(flag % 2)
		index = index & 0x0F;
	else
		index = ((index & 0xF0) >> 4);

	flag ^= 1;

	clut = gui_core->config.firstclut;
	while(i < image->pal_index) {
		clut = clut->next;
		i++;
	}

	if(clut)
		palette = clut->palette;
	else
		return 0;

	return _8_to_32(palette, index);
}

static int _convert_pixel_8_to_32(const image_desc *image, int i, int j)
{
	unsigned char index = 0;
	GuiCore *gui_core = NULL;
	GuiClut *clut = NULL;
	Color *palette = NULL;
	unsigned char *data = (unsigned char *)image->data;

	gui_core = GUI_GetCurrent();
	index = data[i * image->width + j];
	clut = gui_core->config.firstclut;
	while(i < image->pal_index) {
		clut = clut->next;
		i++;
	}

	if(clut)
		palette = clut->palette;
	else
		return 0;

	return _8_to_32(palette, index);
}

static int _convert_pixel_16_to_32(const image_desc *image, int i, int j)
{
	unsigned short *data = image->data;

	return _565_to_32(data[i * image->width +j]);
}

static int _convert_pixel_24_to_32(const image_desc *image, int i, int j)
{
	unsigned char *data = image->data;

	return _24_to_32(data[i * image->width * 3 + j * 3 + 0],
			 data[i * image->width * 3 + j * 3 + 1],
			 data[i * image->width * 3 + j * 3 + 2]);
}

static int _convert_32bpp(image_desc *image)
{
	unsigned int *pData = NULL;
	unsigned int size = 0, i = 0, j = 0;
	image_covert func;

	size = image->width * image->height * sizeof(unsigned int);

	pData = (unsigned int *)GUI_MALLOC(size);
	if(NULL == pData)
		return 1;

	memset(pData, 0, size);

	switch(image->bpp) {
		case IMAGE_4_BPP:
			func = _convert_pixel_4_to_32;
			break;
		case IMAGE_8_BPP:
			func = _convert_pixel_8_to_32;
			break;
		case IMAGE_16_BPP:
			func = _convert_pixel_16_to_32;
			break;
		case IMAGE_24_BPP:
			func = _convert_pixel_24_to_32;
			break;
	}

	for(i = 0; i < image->height; i++)
		for(j = 0; j < image->width; j++)
			pData[i * image->width + j] = func(image, i, j);

	gal_img_free_memory(image);
	image->data = (void *)pData;
	image->size = size;

	return 0;
}

int image_convert2current(image_desc *image)
{
	GuiCore *gui_core = NULL;
	int ret = 0;

	if(NULL == image || image->data == NULL)
		return 1;

	gui_core = GUI_GetCurrent();
	if(gui_core->config.bpp == image->bpp)
		return 0;

	switch(gui_core->config.bpp) {
		case IMAGE_8_BPP:
			/*No routine*/
			break;
		case IMAGE_16_BPP:
			/*8 To 16*/
			/*24 To 16*/
			/*32 To 16*/
			_convert_16bpp(image);
			image->bpp = 16;
			break;
		case IMAGE_24_BPP:
			/*8 To 24*/
			/*16 To 24*/
			/*32 To 24*/
			_convert_24bpp(image);
			break;
		case IMAGE_32_BPP:
			/*8 To 32*/
			/*16 To 32*/
			/*24 To 32*/
			_convert_32bpp(image);
			image->bpp = 32;
			break;
		default:
			break;
	}

	return ret;
}
#ifndef NO_OS

#else
#include "mini_gui_private.h"
#endif /*NO_OS*/

#include "IMG.h"
#include "hd_gal.h"

int gal_img_cleanup(image_desc *img)
{
	if (NULL == img)
		return -1;

	if ((img->pal) && (JPEG_TYPE != img->type)) {
		GUI_FREE(img->pal);
		img->pal = NULL;
	}

	if (img->data) {
		if (img->ops && img->ops->img_free)
			img->ops->img_free(img->data);
		gal_img_free_memory(img);
	}

	return 0;
}

int gal_img_release(image_desc *img)
{
	gal_img_cleanup(img);

	if(img->filename)
		GUI_FREE(img->filename);
	if(img->vq != NULL)
	{
		GUI_FREE(img->vq);
	}
	GUI_FREE(img);

	return 0;
}

static void _correct_type(const char *filename, char *file_ext)
{
    handle_t hfile = 0;
    char buffer[20] = {0};
    int ret = 0;

    if(3 > strlen(file_ext))
	return;

#ifndef NO_OS
    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(filename))
	    return;
#endif /*NO_OS*/
    hfile = GxCore_Open(filename, "r");
    if(hfile == 0)
	    return;

    ret = GxCore_Read(hfile, buffer, 20, 1);
    if(ret <= 0)
	{
		GxCore_Close(hfile);
		return;
	}

    if(('P' == buffer[1]) && ('N' == buffer[2]) && ('G' == buffer[3]))
    {
	memset(file_ext, 0, 4);
	strcpy(file_ext, "png");
    }
    else if(('G' == buffer[0]) && ('I' == buffer[1]) && ('F' == buffer[2]))
    {
	memset(file_ext, 0, 4);
	strcpy(file_ext, "gif");
    }
    else if(('B' == buffer[0]) && ('M' == buffer[1]))
    {
	memset(file_ext, 0, 4);
	strcpy(file_ext, "bmp");
    }
    else if(('J' == buffer[6]) && ('F' == buffer[7]) && ('I' == buffer[8]) && ('F' == buffer[9]))
    {
	memset(file_ext, 0, 4);
	strcpy(file_ext, "jpg");
    }
	else if(('*' == buffer[2]))
    {
	memset(file_ext, 0, 4);
	strcpy(file_ext, "tif");
    }
	else if((buffer[0] == 0x0) && (buffer[1] == 0x0) && (buffer[2] == 0x01))
	{
	memset(file_ext, 0, 4);
	strcpy(file_ext, "mpg");
	}
	else if((buffer[0] == 0xFF) && (buffer[1] == 0xD8) && (buffer[2] == 0xFF))
    {
	memset(file_ext, 0, 4);
	strcpy(file_ext, "jpg");
    }
    else if(('R' == buffer[0]) && ('I' == buffer[1]) && ('F' == buffer[2]) && ('F' == buffer[3])) {
	memset(file_ext, 0, 4);
	strcpy(file_ext, "webp");
    }

    GxCore_Close(hfile);
}

static struct image_file_node *gal_find_node(const char *filename)
{
#define MAX_SUFFIX_LEN 10
	struct image_file_node *image_ops = NULL;
	char *file_ext = NULL, *suffix = NULL;
	int i, j;

	if (filename == NULL)
		return NULL;

	file_ext = strrchr(filename, '.') + 1;
	if (file_ext == NULL)
		return NULL;

	suffix = GUI_MALLOCZ(MAX_SUFFIX_LEN);
	if (suffix == NULL)
		return NULL;
	strncpy(suffix, file_ext, MAX_SUFFIX_LEN);

	_correct_type(filename, suffix);

	for (i = 0; img_node[i] != NULL && image_ops == NULL; i++) {
		for (j = 0; img_node[i]->file_type[j] != NULL; j++) {
			if (strcasecmp(img_node[i]->file_type[j], suffix) == 0) {
				image_ops = img_node[i];
				break;
			}
		}
	}

	GUI_FREE(suffix);

	return image_ops;
}

int gal_img_processing_func_register(struct image_register_func *func)
{
	if(NULL == func || NULL == func->pre || NULL == func->post) {
		gxlogd("Params error\n");
		return -1;
	}

	s_img_register_func = *func;
	return 0;
}

void gal_img_processing_func_unregister(void)
{
	memset(&s_img_register_func, 0, sizeof(struct image_register_func));
}

image_desc *gal_img_load(image_desc *desc, const char *filename)
{
	struct image_file_node *image_ops = gal_find_node(filename);
	image_desc *tmp_desc = NULL;
	char *new_path = NULL, *filename_bak = NULL;

	if(NULL == image_ops)
	{
	    gxlogd("[GUI] Cannot find any image %s decoder.\n", filename);
	}

	// From user memory.
	if((desc) &&
	   (desc->vq) &&
#ifndef NO_OS
	   (NULL == desc->vq->usr_p) &&
	   (NULL == desc->vq->kernel_p) &&
#endif /*NO_OS*/
	   (desc->data))
	{
		return (desc);
	}

	if(s_img_register_func.pre){
		new_path = s_img_register_func.pre(filename);
		filename_bak = GUI_STRDUP(filename);
	} else {
		new_path = (char *)filename;
	}

#ifndef NO_OS
	if(GXCORE_FILE_EXIST != GxCore_FileExists(new_path)) {
		if(s_img_register_func.post) {
			s_img_register_func.post(filename_bak, new_path);
			GUI_FREE(filename_bak);
		}
		gxlogd("error : file %s is not existed.\n", desc->filename);
		return (NULL);
	}
#endif /*NO_OS*/

	while (image_ops) {
		if(desc) {
			desc->fmt = 0;
		}

		tmp_desc = image_ops->img_load_cb(desc, new_path);

		if (tmp_desc) {
			if (tmp_desc->data == NULL) {
				gxlogd("A: error : func=%s:%d, desc->filename=%s, filename=%s\n", __func__, __LINE__,
						desc->filename, filename);
			}
		}
		if ((tmp_desc == NULL) ||
				((NULL != tmp_desc) && (NULL == tmp_desc->data))) {
#ifndef NO_OS
			image_ops = image_ops->reserve;
			if(NULL == image_ops)
#endif /*NO_OS*/
			{
				if(s_img_register_func.post) {
					s_img_register_func.post(filename_bak, new_path);
					if(tmp_desc && tmp_desc->filename && strcmp(tmp_desc->filename, filename_bak)) {
						GUI_FREE(tmp_desc->filename);
						tmp_desc->filename = filename_bak;
					} else {
						GUI_FREE(filename_bak);
					}
				}
				return (NULL);
			}
		}
		else
			break;
	}
	if(s_img_register_func.post) {
		s_img_register_func.post(filename_bak, new_path);
		if(tmp_desc && tmp_desc->filename && strcmp(tmp_desc->filename, filename_bak)) {
			GUI_FREE(tmp_desc->filename);
			tmp_desc->filename = filename_bak;
		} else {
			GUI_FREE(filename_bak);
		}
	}

	desc = tmp_desc;

	if(desc) {
		desc->ref = 0;
		desc->size = desc->width * desc->height * desc->bpp / 8;
		desc->ops = image_ops;
	}

//	gxlogd("%s: disc=%p: desc->data = %p, filename=%s\n", __func__, desc, desc->data, filename);
	return desc;
}

void *gal_img_alloc_memory(image_desc *desc)
{
	void *mem = NULL;
	size_t size = desc->width * desc->height * desc->bpp / 8;

#ifndef NO_OS
	if (gui.fb.mem_mgr) {
		if((desc->vq) && (desc->size != desc->vq->size))
		{
			hd_free(desc->vq);
			GUI_FREE(desc->vq);
		}
		desc->vq = hd_malloc(&(desc->vq), size);
		mem = desc->vq->usr_p;
	}
#endif /*NO_OS*/

	if (mem == NULL) {
		GUI_FREE(desc->vq);
		desc->hw_accel = 0;
		mem = GUI_MALLOC(size);
	}
	else
		desc->hw_accel = 1;

	desc->size = size;
	desc->data = mem;

	return mem;
}

void gal_img_free_memory(image_desc *desc)
{
	if (desc->data) {
#ifndef NO_OS
		if (desc->hw_accel)
			hd_free(desc->vq);
		else
			GUI_FREE(desc->data);
#endif /*NO_OS*/

		desc->data = NULL;
		desc->size = 0;
		desc->hw_accel = 0;
	}
}

#ifndef NO_OS
int IMG_Set_PlayMode(ImagePlayMode mode)
{
	GuiCore *gui_core = GUI_GetCurrent();

	if(gui_core) {
		gui_core->config.play_image_mode = mode;
	}

	return (-1);
}

ImagePlayMode IMG_Get_PlayMode(void)
{
	GuiCore *gui_core = GUI_GetCurrent();

	if(gui_core) {
		return (gui_core->config.play_image_mode);
	}

	return (gui.config.play_image_mode);
}
#endif /*NO_OS*/
