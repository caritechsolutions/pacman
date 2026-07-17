#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#include "IMG.h"
#include "gal.h"
#include "gdi_play.h"
#include "hd_gal.h"
#include "gui_private.h"
#include "tiffiop.h"
#include "tiffio.h"

image_desc *_TIFF_LoadData(image_desc *desc,  const char *filename)
{
	Image_Exif img_info;
	if (IMG_TIFF_GetInfo(filename, &img_info))
		return NULL;

	if(NULL == desc)
		desc = gal_img_new();

	if(NULL == desc)
		return NULL;

	desc->width = img_info.width;
	desc->height = img_info.height;
	desc->bpp = img_info.bpp;

	return desc;
}

int _TIFF_GetColormap(TIFF *image, image_desc *desc)
{
	int pal_index = 0, i = 0;
	GuiClut *pclut = NULL, *pnewclut = NULL;
	unsigned char *red_orig = NULL, *green_orig = NULL, *blue_orig = NULL;
	GuiCore *gui_core = NULL;

	gui_core = GUI_GetCurrent();
	desc->pal = (void *)GUI_MALLOC(256 * sizeof(int));
	if (desc->pal) {
		TIFFGetField(image, TIFFTAG_COLORMAP, &red_orig, &green_orig, &blue_orig);
		memset(desc->pal, 0, 256 * sizeof(int));

		for (i = 0; i < 256; i++) {
			((int*)(desc->pal))[i] = ((*red_orig) << 16) |
				((*(green_orig)) << 8) | *(blue_orig);
			red_orig += 2;
			green_orig += 2;
			blue_orig += 2;
		}
	} else
		return 1;

	if ((NULL == gui_core->config.clut)) {
		gui_core->config.clut = (GuiClut *)GUI_MALLOC(sizeof(GuiClut));
		if (NULL == gui_core->config.clut)
			goto error;

		memset(gui_core->config.clut, 0, sizeof(GuiClut));
		gui_core->config.clut->num = 256;
		gui_core->config.clut->palette = (Color* )GUI_MALLOC(256 * sizeof(Color));//desc->pal;

		if (gui_core->config.clut->palette) {
			memcpy(gui_core->config.clut->palette,
					desc->pal,
					256 * sizeof(Color));
		} else {
			GUI_FREE(gui_core->config.clut);
			goto error;
		}

		desc->pal_index = 0;
	} else {
		if (0 != memcmp(gui_core->config.clut->palette, desc->pal, 256)) {
			pclut = gui_core->config.clut;
			pal_index = 1;
			while (pclut->next) {
				pclut = pclut->next;
				pal_index++;
			}

			pnewclut = (GuiClut *)GUI_MALLOC(sizeof(GuiClut));
			if (NULL == pnewclut)
				goto error;

			memset(pnewclut, 0, sizeof(GuiClut));
			pnewclut->num = 256;
			pnewclut->palette = (Color* )GUI_MALLOC(256 * sizeof(Color));//desc->pal;
			if (pnewclut->palette) {
				memcpy(pnewclut->palette,
						desc->pal,
						256 * sizeof(Color));
			} else {
				GUI_FREE(pnewclut);
				goto error;
			}

			desc->pal_index = pal_index;
			pclut->next = pnewclut;
		}
	}

	return 0;

error:
	GUI_FREE(desc->pal);
	return 1;

}

image_desc *IMG_TIFF_Load(image_desc *desc, const char *filename)
{
	GuiCore *gui_core = NULL;
	TIFF *image;
	int stripSize, stripMax, stripCount, bufferSize;
	int result = 0, imageOffset = 0;

	if (NULL == filename)
		return (desc);

	desc = _TIFF_LoadData(desc, filename);
	if (NULL == desc)
		return NULL;

	gui_core = GUI_GetCurrent();
	if (desc->width > gui_core->config.width || desc->height > gui_core->config.height) {
		gxlogd("The TIFF img is too big\n");
		return desc;
	}

	if ((image = TIFFOpen(filename, "r")) == NULL)
		return NULL;

	stripSize = TIFFStripSize(image);
	stripMax = TIFFNumberOfStrips(image);

	bufferSize = stripMax * stripSize;

	if ((desc->data = (char *)GUI_MALLOC(sizeof(char) * bufferSize)) == NULL) {
		TIFFClose(image);
		return NULL;
	}

	memset(desc->data, 0, bufferSize);

	if (desc->bpp == 8) {
		int type = 0;
		TIFFGetField(image, TIFFTAG_PHOTOMETRIC, &type);

		if (type == 3) {
			if (_TIFF_GetColormap(image, desc))
				goto error;

			for (stripCount = 0; stripCount < stripMax; stripCount++){
				if((result = TIFFReadEncodedStrip (image, stripCount,
								desc->data + imageOffset, stripSize)) == -1)
					goto error;

				imageOffset += result;
			}
		}
	} else if (desc->bpp == 24 || desc->bpp == 32) {
		uint32 *raster;
		uint32 *row;

		raster = (uint32*)GUI_MALLOC(desc->width * desc->height * sizeof (uint32));
		if (NULL == raster)
			goto error;

		TIFFReadRGBAImage(image, desc->width, desc->height, (uint32*)raster, 1);

		row = &raster[desc->width * (desc->height - 1)];
		char *bits2 = desc->data;
		int x, y;
		for (y = 0; y < desc->height; y++) {
			char *bits = bits2;
			for (x = 0; x < desc->width; x++) {
				if (desc->bpp == 32)
					*bits++ = (char)TIFFGetA(row[x]);
				*bits++ = (char)TIFFGetR(row[x]);
				*bits++ = (char)TIFFGetG(row[x]);
				*bits++ = (char)TIFFGetB(row[x]);
			}

			row -= desc->width;

			if (desc->bpp == 32)
				bits2 += desc->width * 4;
			else
				bits2 += desc->width * 3;
		}

		GUI_FREE(raster);
	} else
		gxlogd("Unsuppopt TIFF Type!\n");

	TIFFClose(image);
	return desc;

error:
	GUI_FREE(desc->data);
	gal_img_release(desc);

	TIFFClose(image);
	return NULL;
}

int _TIFF_RGB2YUV(const image_desc *src, unsigned short *dst)
{
	int i = 0, j = 0, rgb = 0, bpp = 0;
	int yuv = 0, width = 0, height = 0;
	char *data = NULL;
	int *pal = NULL;

	if((NULL == src) || (NULL == dst))
	{
		return (1);
	}

	width = src->width;
	height = src->height;

	data = (char *)(src->data);

	bpp = src->bpp;
	pal = (int *)(src->pal);

	for(i = 0; i < height; i++)
	{
		for(j = 0; j < width; j++)
		{
			if(8 == bpp)
			{
				if(NULL == pal)
					return 1;
				rgb = data[i * width + j];
				rgb = pal[rgb];
			}
			else if(24 == bpp)
			{
				rgb = (data[i * width * 3 + j * 3] << 16) |
					(data[i * width * 3 + j * 3 + 1] << 8) |
					(data[i * width * 3 + j * 3 + 2]);
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

			dst[i * width + j] = yuv;
		}
	}

	return (0);
}

int IMG_TIFF_Draw(void *screen, const char *filename, int x0, int y0, int screen_width, int screen_height)
{
	GuiCore *gui_core = NULL;
	image_desc *desc = NULL;
	unsigned short *display_buffer = NULL;
	int ret = 0, x_pos = 0, y_pos = 0;
	int buffer_width = 0, buffer_height = 0;

	if (NULL == screen || NULL == filename) {
		hd_img_clear();
		return (1);
	}

	gui_core = GUI_GetCurrent();
	desc = IMG_TIFF_Load(NULL, filename);
	if (NULL == desc || desc->width > gui_core->config.width || desc->height > gui_core->config.height) {
		if (desc)
			gal_img_release(desc);
		hd_img_clear();
		return (1);
	}

	display_buffer = (unsigned short *)GUI_MALLOC(desc->width * desc->height * 2);
	if (NULL == display_buffer) {
		hd_img_clear();
		return (1);
	}

	memset(display_buffer, 0, desc->width * desc->height * 2);

	ret = _TIFF_RGB2YUV(desc, display_buffer);
	GUI_FREE(desc->pal);

	if(-1 != screen_width)
		buffer_width = screen_width;
	else
		buffer_width = gui_core->config.width;

	if(-1 != screen_height)
		buffer_height = screen_height;
	else
		buffer_height = gui_core->config.height;

	if (desc->width < buffer_width) {
		x_pos = x0 + (buffer_width - desc->width) / 2;
		x_pos = x_pos - (x_pos % 2);
	}

	if (desc->height < buffer_height) {
		y_pos = y0 + (buffer_height - desc->height) / 2;
		y_pos = y_pos - (y_pos % 2);
	}

	desc->type = JPEG_TYPE;
	desc->effect = EFFECT_COPY;

	gal_img_free_memory(desc);
	desc->bpp = 16;
	desc->data = gal_img_alloc_memory(desc);

	memcpy(desc->data, display_buffer, desc->size);
	GUI_FREE(display_buffer);

	hd_img_desc(screen, desc, x_pos, y_pos, desc->width, desc->height);

	gal_img_release(desc);
	return 0;
}

int IMG_TIFF_GetInfo(const char *filename, Image_Exif *img_info)
{
	TIFF *image;
	int Width = 0, Height = 0, BitsPerSample = 0, SamplesPerPixel = 0;

	if (NULL == filename || NULL == img_info)
		return 1;
	if ((image = TIFFOpen(filename, "r")) == NULL)
		return 1;

	TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &Width);
	TIFFGetField(image, TIFFTAG_IMAGELENGTH, &Height);
	TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &BitsPerSample);
	TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &SamplesPerPixel);

	int dircount = 0;
	do {
		dircount++;
	} while (TIFFReadDirectory(image));

	img_info->bpp = BitsPerSample * SamplesPerPixel;
	img_info->frame_num = dircount;
	img_info->width = Width;
	img_info->height = Height;
	img_info->horizontal_pixel = -1;
	img_info->vertical_pixel = -1;
	img_info->type = IMAGE_TIFF;

	TIFFClose(image);
	return 0;
}

