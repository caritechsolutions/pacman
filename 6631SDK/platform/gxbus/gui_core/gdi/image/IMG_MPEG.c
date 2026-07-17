#include "IMG.h"
#include "gdi_play.h"
 #include "hd_gal.h"
#include "gui_private.h"
#include "libmpeg2/mpeg2.h"
#include "libmpeg2/mpeg2convert.h"

#define BUFFER_SIZE 4096

image_desc *IMG_MPEG_Load(image_desc *mpg_desc, const char *pFile)
{
	uint8_t buffer[BUFFER_SIZE];
	handle_t mpgfile = 0;
	mpeg2dec_t * decoder = NULL;
	const mpeg2_info_t * info;
	const mpeg2_sequence_t * sequence;
	mpeg2_state_t state;
	size_t size = 1;
	int bufferSize = 0;

	if (NULL == pFile)
		return (mpg_desc);

	if(NULL == mpg_desc)
		mpg_desc = gal_img_new();

	if(NULL == mpg_desc)
		return NULL;

	mpgfile = GxCore_Open((char *)pFile, "r");
	if(!mpgfile)
		goto error;

	decoder = mpeg2_init();
	if(!decoder)
		goto error;

	info = mpeg2_info(decoder);

	while(size) {
		state = mpeg2_parse (decoder);
		sequence = info->sequence;
		switch (state) {
			case STATE_BUFFER:
				if((GxCore_Read (mpgfile, buffer, BUFFER_SIZE, 1)) == 1)
					size = BUFFER_SIZE;
				mpeg2_buffer(decoder, buffer, buffer + size);
				break;
			case STATE_SEQUENCE:
				mpeg2_convert (decoder, mpeg2convert_uyvy, NULL);
				break;
			case STATE_SLICE:
			case STATE_END:
				if (!info->display_fbuf)
					continue;

				mpg_desc->width = sequence->width;
				mpg_desc->height = sequence->height;
				mpg_desc->bpp = 16;
				mpg_desc->fmt = GX_COLOR_FMT_YCBCR422;
				mpg_desc->type = IMAGE_MPEG;
				mpg_desc->filename = GUI_STRDUP(pFile);
				bufferSize = sequence->width * sequence->height * 2;

				if((!mpg_desc->data) && ((mpg_desc->data = (unsigned char *)GUI_MALLOC(bufferSize)) == NULL))
					goto error;
				memset(mpg_desc->data, 0, bufferSize);
				memcpy(mpg_desc->data, info->display_fbuf->buf[0], bufferSize);

				size = 0;
				break;
			default:
				break;
		}
	} while(size);

	mpeg2_close(decoder);
	GxCore_Close(mpgfile);
	return (mpg_desc);

error:
	if(!decoder)
		mpeg2_close(decoder);
	if(!mpgfile)
		GxCore_Close(mpgfile);
	GUI_FREE(mpg_desc);
	return NULL;
}

int IMG_MPEG_Draw(void *screen, const char *pFile, int x0, int y0, int screen_width, int screen_height)
{
	GuiCore *gui_core = NULL;
	image_desc *mpg_desc = NULL;
	unsigned short *display_buffer = NULL;
	int x_pos = 0, y_pos = 0;
	int buffer_width = 0, buffer_height = 0;
	size_t size = 0;

	if(NULL == screen || NULL == pFile) {
		hd_img_clear();
		return (1);
	}

	gui_core = GUI_GetCurrent();
	mpg_desc = IMG_MPEG_Load(NULL, pFile);
	if(NULL == mpg_desc || mpg_desc->width > gui_core->config.width || mpg_desc->height > gui_core->config.height) {
		if (mpg_desc)
			gal_img_release(mpg_desc);
		hd_img_clear();
		return (1);
	}
	size = mpg_desc->width * mpg_desc->height * 2;

	display_buffer = (unsigned short *)GUI_MALLOC(size);
	if(NULL == display_buffer) {
		hd_img_clear();
		return (1);
	}

	memset(display_buffer, 0, size);

	memcpy(display_buffer, mpg_desc->data, size);

	if(-1 != screen_width)
		buffer_width = screen_width;
	else
		buffer_width = gui_core->config.width;

	if(-1 != screen_height)
		buffer_height = screen_height;
	else
		buffer_height = gui_core->config.height;

	if(mpg_desc->width < buffer_width) {
		x_pos = x0 + (buffer_width - mpg_desc->width) / 2;
		x_pos = x_pos - (x_pos % 2);
	}

	if(mpg_desc->height < buffer_height) {
		y_pos =y0 + (buffer_height - mpg_desc->height) / 2;
		y_pos = y_pos - (y_pos % 2);
	}

	mpg_desc->type = JPEG_TYPE;
	mpg_desc->effect = EFFECT_COPY;

	gal_img_free_memory(mpg_desc);
	mpg_desc->bpp = 16;
	mpg_desc->data = gal_img_alloc_memory(mpg_desc);

	memcpy(mpg_desc->data, display_buffer, mpg_desc->size);
	GUI_FREE(display_buffer);

	hd_img_desc_gradiant(screen, mpg_desc, x_pos, y_pos, mpg_desc->width, mpg_desc->height, gui_core->config.play_image_mode);

	gal_img_release(mpg_desc);
	return 0;
}

int IMG_MPEG_GetInfo(const char *pFile, Image_Exif *img_info)
{
	uint8_t buffer[BUFFER_SIZE];
	handle_t mpgfile = 0;
	mpeg2dec_t * decoder;
	const mpeg2_info_t * info;
	const mpeg2_sequence_t * sequence;
	mpeg2_state_t state;
	size_t size = 1;

	if((NULL == pFile) || (NULL == img_info))
		return 1;

	mpgfile = GxCore_Open((char *)pFile, "r");
	if(!mpgfile)
		return 1;

	decoder = mpeg2_init();
	if(!decoder) {
		GxCore_Close(mpgfile);
		return 1;
	}
	info = mpeg2_info(decoder);

	while(size) {
		state = mpeg2_parse (decoder);
		sequence = info->sequence;
		switch (state) {
			case STATE_BUFFER:
				if((GxCore_Read (mpgfile, buffer, BUFFER_SIZE, 1)) == 1)
					size = BUFFER_SIZE;
				mpeg2_buffer (decoder, buffer, buffer + size);
				break;
			case STATE_SLICE:
			case STATE_END:
			case STATE_INVALID_END:
				img_info->width = sequence->width;
				img_info->height = sequence->height;
				img_info->horizontal_pixel = sequence->pixel_width;
				img_info->vertical_pixel = sequence->pixel_height;
				img_info->bpp = 16;
				size = 0;
				break;
			default:
				break;
		}
	}
	img_info->type = IMAGE_MPEG;
	img_info->frame_num = 1;

	mpeg2_close(decoder);
	GxCore_Close(mpgfile);
	return 0;
}

