#include "gal.h"
#include "hd_gal.h"
#include "gui_private.h"
#include "pngapi.h"
#include "png.h"
#ifdef NDS_SUPPORT
#include "pngstruct.h"
#include "pnginfo.h"
#endif
#include <sys/stat.h>
#include <setjmp.h>
#include "hd_system.h"

//#define HFILE

typedef struct tagMEMFILE
{
	unsigned char *pbyData; //data
	unsigned char *pbyCur;	//cur data
	unsigned int  dwRemain;	    //valid data size
	handle_t hfile;
}MEMFILE,*PMEMFILE;

static jmp_buf png_jmp_buf;

extern struct image_special_node special_png;
image_desc *IMG_PNG_Load(image_desc *png_desc, const char *pFile)
{
	GuiCore *gui_core = NULL;
	GuiClut *pclut = NULL, *pnewclut = NULL;
#ifdef HFILE
	handle_t pSrc = 0;
#else
	FILE *pSrc = NULL;
#endif
	struct stat buf = {0};
	int ret = 0, pal_index = 0;
	char *filename = NULL;
	GxHwMallocObj *vq = NULL;
	GALMemory *img_obj = NULL;
	image_desc *original_img = png_desc;

	if ((NULL == pFile) || (NULL == special_png.img_check))
		return (NULL);

	if(!(special_png.img_check(pFile)))
	{
	    return (NULL);
	}

	ret = stat(pFile, &buf);
	if (ret < 0)
		return (png_desc);

#ifdef HFILE
	pSrc = GxCore_Open((char *)pFile, "r");
#else
	pSrc = fopen(pFile, "r");
#endif
	gui_core = GUI_GetCurrent();

	if (pSrc) {
		if(special_png.img_load_size)
		{
			png_desc = special_png.img_load_size(png_desc, (handle_t)pSrc, buf.st_size);
		}
		if(NULL == png_desc) {
#ifdef HFILE
			GxCore_Close(pSrc);
#else
			fclose(pSrc);
#endif
			return (png_desc);
		} else if(32 < png_desc->bpp){
#ifdef HFILE
			GxCore_Close(pSrc);
#else
			fclose(pSrc);
#endif
			if(NULL == original_img) {
				gal_img_release(png_desc);
			}
			return (NULL);
		}

		if((NULL == gui_core->config.clut) && (png_desc->pal))
		{
			gui_core->config.clut = (GuiClut *)GUI_MALLOC(sizeof(GuiClut));
			memset(gui_core->config.clut, 0, sizeof(GuiClut));
			gui_core->config.clut->num = 256;
			gui_core->config.clut->palette = (Color* )GUI_MALLOC(256 * sizeof(Color));//desc->pal;
			if(gui_core->config.clut->palette) {
				memcpy(gui_core->config.clut->palette,
						png_desc->pal,
						256 * sizeof(Color));
			}

			png_desc->pal_index = 0;
		}
		else if(png_desc->pal) {
			if(0 != memcmp(gui_core->config.clut->palette, png_desc->pal, 256)) {
				pclut = gui_core->config.clut;
				pal_index = 1;
				while(pclut->next) {
					pclut = pclut->next;
					pal_index++;
				}

				pnewclut = (GuiClut *)GUI_MALLOC(sizeof(GuiClut));
				memset(pnewclut, 0, sizeof(GuiClut));
				pnewclut->num = 256;
				pnewclut->palette = (Color* )GUI_MALLOCZ(256 * sizeof(Color));//desc->pal;
				if(pnewclut->palette) {
					memcpy(pnewclut->palette,
							png_desc->pal,
							256 * sizeof(Color));
				}
				png_desc->pal_index = pal_index;
				pclut->next = pnewclut;
			}
		}
		else
			png_desc->pal_index = 0;

		if(pFile)
		{
		    filename = GUI_STRDUP(pFile);
		    GUI_FREE(png_desc->filename);
		    png_desc->filename = filename;
		}

	}

#ifdef HFILE
	GxCore_Close(pSrc);
#else
	fclose(pSrc);
#endif
	if((png_desc) &&
	   ((png_desc->bpp == 24) ||
	   ((png_desc->bpp == 8) && (NULL != png_desc->pal))))
	{
		image_desc tmp_desc = {0};

		if((GXAV_ID_GX3211 == gui.config.chip_id) ||
		   (GXAV_ID_GX6605S == gui.config.chip_id) ||
		   (8 == png_desc->bpp)) {
			image_convert2current(png_desc);
			tmp_desc = *png_desc;
			png_desc->data = gal_img_alloc_memory(png_desc);
			memcpy(png_desc->data, tmp_desc.data, png_desc->size);
			gal_img_free_memory(&tmp_desc);
		}
	}
	else if((8 == png_desc->bpp) && (NULL == png_desc->pal))
	{
		// MONO gray image
		char *mono_data = NULL, *rgb_data = NULL;
		int i = 0;

		img_obj = gal_alloc_memory(png_desc->size);
		if(img_obj) {
			if(IMAGE_FROM_VIDEO_MEMORY == img_obj->source) {
				mono_data = (char *)img_obj->block.vq->usr_p;
			} else {
				mono_data = (char *)img_obj->block.ptr;
			}
		}
		if(NULL == mono_data)
			return (NULL);

		memcpy(mono_data, png_desc->data, png_desc->size);
		gal_img_free_memory(png_desc);
		png_desc->bpp = 32;
		png_desc->data = gal_img_alloc_memory(png_desc);
		rgb_data = (char *)(png_desc->data);
		for(i = 0; i < (png_desc->size / 4); i++)
		{
			rgb_data[i * 4] = mono_data[i];
			rgb_data[i * 4 + 1] = mono_data[i];
			rgb_data[i * 4 + 2] = mono_data[i];
			rgb_data[i * 4 + 3] = 0xFF;
		}

		if(img_obj) {
			gal_free_memory(img_obj);
			img_obj = NULL;
		}
	}

	return (png_desc);
}

int _PNG_RGB2YUV(image_desc *src, unsigned short *dst)
{
	int i = 0, j = 0, rgb = 0, bpp = 0;
	int yuv = 0, real_width = 0, width = 0, height = 0;
	char *data = NULL;
	int *pal = NULL;
	int start_ms = 0, end_ms = 0;

	if((NULL == src) || (NULL == dst))
	{
		return (1);
	}

	real_width = width = src->width;
	if(real_width % 2)
		real_width = (real_width >> 1) << 1;
	height = src->height;

	data = (char *)(src->data);

	bpp = src->bpp;
	pal = (int *)(src->pal);

	for(i = 0; i < height; i++)
	{
		if(0 == start_ms) {
			start_ms = hd_get_tick();
		}
		for(j = 0; j < real_width; j++)
		{
			if(8 == bpp)
			{
				rgb = data[i * width + j];
				rgb = pal[rgb];
			}
			else if(16 == bpp)
			{

			}
			else if(24 == bpp)
			{
				rgb = (data[i * width * 3 + j * 3 + 2] << 16) |
					(data[i * width * 3 + j * 3 + 1] << 8) |
					(data[i * width * 3 + j * 3]);
			}
			else if(32 == bpp)
			{
				rgb = (data[i * width * 4 + j * 4] << 16) |
					(data[i * width * 4 + j * 4 + 1] << 8) |
					(data[i * width * 4 + j * 4 + 2]);
			}
			yuv = gal_rgb2yuv(rgb);
			if(j % 2)
			{
				yuv = (((yuv >> 8) & 0x0000FF00) | (yuv & 0x000000FF));
			}
			else
			{
				yuv = (yuv >> 8);
			}

			dst[i * real_width + j] = yuv;
		}
		end_ms = hd_get_tick();
		if((end_ms - start_ms) >= 500) {
			gui_delay(10);
			end_ms = start_ms = 0;
		}
	}

	src->width = real_width;

	return (0);
}

int IMG_PNG_Draw(void *screen, const char *pFile, int x0, int y0, int screen_width, int screen_height)
{
	GuiCore *gui_core = NULL;
	image_desc *png_desc = NULL, desc = {0};
	GALMemory *img_obj = NULL;
	unsigned short *display_buffer = NULL;
	int ret = 0, x_pos = 0, y_pos = 0;
	int buffer_width = 0, buffer_height = 0;

	if(NULL == screen || NULL == pFile) {
		hd_img_clear();
		return (1);
	}

	gui_core = GUI_GetCurrent();
	png_desc = IMG_PNG_Load(&desc, pFile);
	if(NULL == png_desc) {
		if (png_desc && (png_desc != &desc))
			gal_img_release(png_desc);
		hd_img_clear();
		return (1);
	}

	if(24 == png_desc->bpp)
	{
		img_obj = gal_alloc_memory(png_desc->width * png_desc->height * 2);
		if(img_obj) {
			if(IMAGE_FROM_VIDEO_MEMORY == img_obj->source) {
				display_buffer = (unsigned short *)img_obj->block.vq->usr_p;
			} else {
				display_buffer = (unsigned short *)img_obj->block.ptr;
			}
		}
		if(NULL == display_buffer) {
			hd_img_clear();
			return (1);
		}

		memset(display_buffer, 0, png_desc->width * png_desc->height * 2);
		ret = _PNG_RGB2YUV(png_desc, display_buffer);
	}

	if(-1 != screen_width)
		buffer_width = screen_width;
	else
		buffer_width = gui_core->config.width;

	if(-1 != screen_height)
		buffer_height = screen_height;
	else
		buffer_height = gui_core->config.height;

	if(png_desc->width < buffer_width) {
		x_pos = x0 + (buffer_width - png_desc->width) / 2;
		x_pos = x_pos - (x_pos % 2);
		buffer_width = png_desc->width;
	}

	if(png_desc->height < buffer_height) {
		y_pos =y0 + (buffer_height - png_desc->height) / 2;
		y_pos = y_pos - (y_pos % 2);
		buffer_height = png_desc->height;
	}

	png_desc->effect = EFFECT_COPY;

	if(24 == png_desc->bpp)
	{
		png_desc->type = JPEG_TYPE;
		gal_img_free_memory(png_desc);
		png_desc->bpp = 16;
		png_desc->data = gal_img_alloc_memory(png_desc);
		memcpy(png_desc->data, display_buffer, png_desc->size);
		if(img_obj) {
			gal_free_memory(img_obj);
			img_obj = NULL;
		}
	}


	ret |= hd_img_desc_gradiant(screen, png_desc, x_pos, y_pos, buffer_width, buffer_height, gui_core->config.play_image_mode);

	if (png_desc && (png_desc != &desc)) {
		gal_img_release(png_desc);
	} else {
		gal_img_cleanup(&desc);
		if(desc.filename) {
			GUI_FREE(desc.filename);
		}
		if(desc.vq != NULL) {
			GUI_FREE(desc.vq);
		}
	}

	return ret;
}

int IMG_PNG_GetInfo(const char *pFile, Image_Exif *img_info)
{
	int ret = 0, value = 0, bpp = 0, colortype = 0;
	handle_t hfile = 0;
	char buf[100] = {0}, *ptr = NULL;

	if((NULL == pFile) || (NULL == img_info)) {
		return (1);
	}

	hfile = GxCore_Open((char *)pFile, "r");
	if(0 == hfile) {
		return (1);
	}

	ret = GxCore_Read(hfile, buf, 1, 100);
	if(ret == 0) {
		GxCore_Close(hfile);
		return (ret);
	}
	ret = 0;
	ptr = buf;
	ptr++;

	if(('P' != ptr[0]) || ('N' != ptr[1]) || ('G' != ptr[2])) {
		GxCore_Close(hfile);
		return (1);
	}

	ptr += 7;
	value = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
	ptr += 4;
	if((13 == value) &&
	   ('I' == ptr[0]) &&
	   ('H' == ptr[1]) &&
	   ('D' == ptr[2]) &&
	   ('R' == ptr[3])) {
		ptr += 4;
		img_info->width = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
		ptr += 4;
		img_info->height = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
		ptr += 4;
		bpp = ptr[0];
		colortype = ptr[1];
		if((3 == colortype) &&
		   ((1 == bpp) ||
		    (2 == bpp) ||
		    (4 == bpp) ||
		    (8 == bpp))) {
			img_info->bpp = 8;
		} else if (2 == colortype) {
			img_info->bpp = 24;
		} else if (6 == colortype) {
			img_info->bpp = 32;
		}
	} else {
		GxCore_Close(hfile);
		return (1);
	}

	GxCore_Close(hfile);
	return (ret);
}

