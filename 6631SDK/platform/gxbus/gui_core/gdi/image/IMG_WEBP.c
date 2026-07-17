#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "gui_debug.h"
#include "gal.h"
#include <setjmp.h>
#include "hd_gal.h"
#include "SDL.h"

#include "webpdec.h"
#include "webp/decode.h"

static void _argb_scan_line(unsigned char *in, unsigned char *out, int width)
{
	int i = 0;

	for(i = 0; i < width; i++) {
		out[i * 4] = in[i * 4 + 2];
		out[i * 4 + 1] = in[i * 4 + 1];
		out[i * 4 + 2] = in[i * 4];
		out[i * 4 + 3] = in[i * 4 + 3];
	}
}

static void _save_out_data(WebPDecBuffer *buffer, image_desc *pdesc)
{
	unsigned char *rgba_data = buffer->u.RGBA.rgba, *out_data = pdesc->data;
	unsigned int stride = buffer->u.RGBA.stride;
	unsigned int i = 0;

	for(i = 0; i < pdesc->height; i++) {
		_argb_scan_line(rgba_data, out_data, pdesc->width);
		rgba_data += stride;
		out_data += stride;
	}
}

image_desc *IMG_WEBP_Load(image_desc *webp_desc, const char *pFile)
{
	image_desc *pdesc = NULL;
	const unsigned char *data = NULL;
	char *out_data = NULL;
	size_t data_size = 0;
	WebPDecoderConfig config;
	WebPDecBuffer* const output_buffer = &config.output;
	WebPBitstreamFeatures* const bitstream = &config.input;

	if(NULL == pFile) {
		return (NULL);
	}

	if(GXCORE_FILE_EXIST != GxCore_FileExists(pFile)) {
		return (NULL);
	}

	memset(&config, 0, sizeof(WebPDecoderConfig));
	if (!WebPInitDecoderConfig(&config)) {
		gxlogd("Init WebP decoder failed (%s). \n", pFile);
		return (NULL);
	}

	if (!LoadWebP(pFile, &data, &data_size, bitstream)) {
		gxlogd("Load WEBP image failed (%s). \n", pFile);
		WebPFreeDecBuffer(output_buffer);
		return (NULL);
	}

	output_buffer->colorspace = MODE_RGBA;
	if(VP8_STATUS_OK != DecodeWebP(data, data_size, &config)) {
		gxlogd("Decode WEBP image failed (%s). \n", pFile);
		WebPFreeDecBuffer(output_buffer);
		free((void *)data);
		return (NULL);
	}
	
	if(NULL == webp_desc) {
		pdesc = webp_desc = gal_img_new();
	}

	if(NULL == pdesc) {
		goto OUT;
	}

	pdesc->width = output_buffer->width;
	pdesc->height = output_buffer->height;
	pdesc->bpp = 32;
	pdesc->size = pdesc->width * pdesc->height * pdesc->bpp / 8;
	GUI_FREE(pdesc->filename);
	pdesc->filename = GUI_STRDUP(pFile);
	pdesc->type = WEBP_TYPE;
	out_data = gal_img_alloc_memory(pdesc);
	pdesc->effect = EFFECT_SRC_TO_DST;

	_save_out_data(output_buffer, pdesc);
OUT:
	WebPFreeDecBuffer(output_buffer);
	free((void *)data);
	return (pdesc);
}

int IMG_WEBP_Draw(void *screen, const char *pWEBP, int x0, int y0, int screen_width, int screen_height)
{
	int ret = 0;
	unsigned int xpos = 0, ypos = 0, xsize = 0, ysize = 0;
	image_desc *pdesc = NULL;
	 GuiCore *gui_core = NULL;
	
	if((NULL == screen) || (NULL == pWEBP)) {
		return (1);
	}

	gui_core = GUI_GetCurrent();
	pdesc = IMG_WEBP_Load(NULL, pWEBP);
	if(NULL == pdesc) {
		return (1);
	}

	if((pdesc->width < screen_width) && (pdesc->height < screen_height)) {
		xsize = pdesc->width;
		ysize = pdesc->height;
	} else {
		xsize = pdesc->width;
		ysize = pdesc->height;
		if(xsize > screen_width) {
			xsize = screen_width;
			ysize = pdesc->height * xsize / pdesc->width;
		}

		if(ysize > screen_height) {
			ysize = screen_height;
			xsize = pdesc->width * ysize / pdesc->width;
		}
	}

	xpos = x0 + (screen_width - xsize) / 2;
	xpos &= 0xFFFFFFFE;
	ypos = y0 + (screen_height - ysize) / 2;
	pdesc->effect = EFFECT_COPY;
	ret = hd_img_desc_gradiant(screen, pdesc, xpos, ypos, xsize, ysize, gui_core->config.play_image_mode);

	gal_img_release(pdesc);

	return (ret);
}

int IMG_WEBP_GetInfo(const char *pFile, Image_Exif *img_info)
{
	int ret = 0;

	return (ret);
}

int IMG_WEBP_STOP(void)
{
	int ret = 0;

	return (ret);
}

