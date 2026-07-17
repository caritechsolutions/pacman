#include "gxcore.h"
#include "gx_mem.h"
#include "gx_pixel_subtitle.h"
#include "gx_subtitle_show_pixel.h"
#include "gx_subtitle_config.h"

typedef struct {
	unsigned int show_handle;
	unsigned int def_width;
	unsigned int def_height;
	unsigned int cv_width;
	unsigned int cv_height;
} GxPixelSubtitle;

static unsigned int ayuv8888_to_argb8888(unsigned int ayuv)
{
	unsigned int Y = (ayuv >> 16) & 0xff;
	unsigned int Cr = (ayuv >>  8) & 0xff;
	unsigned int Cb = (ayuv & 0xff);
	unsigned int R = 0, G = 0, B = 0;

	B = ((1164 * (Y - 16) + 2018 * (Cb - 128)) / 1000);
	G = ((1164 * (Y - 16) -  813 * (Cr - 128) -  391 * (Cb - 128)) / 1000);
	R = ((1164 * (Y - 16) + 1596 * (Cr - 128)) / 1000);

	return ((((ayuv >> 24) & 0xff) << 24) | (R << 16) | (G << 8) | B);
}

static void find_right_window(unsigned int width,
		unsigned int height,
		unsigned int *def_width,
		unsigned int *def_height)
{
	if ((*def_width == 0) || (*def_height == 0)) {
		*def_width  = 720;
		*def_height = 576;
	}

	if ((width <= *def_width) && (height <= *def_height))
		return;
	else {
		if ((width > 1280) || (height > 720)) {
			*def_width  = 1920;
			*def_height = 1080;
		} else if ((width > 720) || (height > 576)) {
			*def_width  = 1280;
			*def_height = 720;
		} else {
			*def_width  = 720;
			*def_height = 576;
		}
	}

	return;
}

static int draw_bitmap_0(GxPixelSubtitle *pixel, GxPixelSubtitleContent *content)
{
	unsigned int w = 0, h = 0, i = 0;
	unsigned char *src  = content->image;
	unsigned char *srca = content->aimage;
	GxSubtitleShowDisplay pos;
	int def_yb = 0, def_yt = 0, def_xl = 0;
	unsigned int *drawBuffer = NULL;

	if (!src || !srca) {
		gxloge("src %p, srca %p\n", src, srca);
		return -1;
	}

	if (0 == (content->width * content->height)) {
		gxloge("%s %d, width [%d] height[%d]\n",__FUNCTION__, __LINE__, content->width, content->height);
		return -1;
	}

	drawBuffer = GxSubtitleShowPixelAllocARGB888Buffer(pixel->show_handle, content->width, content->height);
	if (drawBuffer == NULL) {
		gxloge("(%d %d) ARGB8888 buffer alloc fail\n", content->width, content->height);
		return -1;
	}

	for (h = 0; h < content->height; ++h) {
		for (w = 0; w < content->width; ++w) {
			if (srca[w]) {
				if (content->custom) {
					if (src[w]) {
						for (i = 0; i < 4; i++) {
							unsigned char a = 0xff, y = 0x00, u = 0x00, v = 0x00;
							unsigned int ayuv = 0;

							if (src[w] == (content->cuspal[i] >> 16)) {
								y = (content->cuspal[i] >> 16) & 0xff;
								u = (content->cuspal[i] >> 8 ) & 0xff;
								v = (content->cuspal[i]) & 0xff;
								ayuv = ((a << 24) | (y << 16) | (u << 8) | v);
								drawBuffer[(h * content->width + w)] = ayuv8888_to_argb8888(ayuv);
								break;
							}
						}
					}
				} else {
					drawBuffer[(h * content->width + w)] = ((0xff << 24) | content->global_palette[src[w]]);
				}
			}
		}
		src  += content->stride;
		srca += content->stride;
	}

	if (pixel->def_width == 0)
		pixel->def_width = content->orig_frame_width;
	if (pixel->def_height == 0)
		pixel->def_height = content->orig_frame_height;
	find_right_window(content->width, content->height, &pixel->def_width, &pixel->def_height);

	def_yb = subtitleConfig.defDisPos.yb;
	def_yt = (pixel->def_height - (def_yb + content->height));
	def_yt = (def_yt > 0) ? def_yt : 0;
	def_xl = (pixel->def_width - content->width) / 2;

	pos.xl = def_xl * pixel->cv_width  / pixel->def_width;
	pos.xr = pos.xl;
	pos.yb = def_yb * pixel->cv_height / pixel->def_height;
	pos.yt = def_yt * pixel->cv_height / pixel->def_height;
	pos.autoPos = 0;

	GxSubtitleShowPixelSetScreenColor(pixel->show_handle, 0x00000000);
	GxSubtitleShowPixelSetDisplayPos (pixel->show_handle, &pos);
	GxSubtitleShowPixelDrawARGB8888Buffer(pixel->show_handle, drawBuffer, 0);
	GxSubtitleShowPixelFreeARGB8888Bufeer(pixel->show_handle, drawBuffer);

	return 0;
}

static int draw_bitmap_1(GxPixelSubtitle *pixel, GxPixelSubtitleContent *content)
{
	unsigned int w = 0, h = 0;
	unsigned char *src  = content->image;
	unsigned char *srca = content->aimage;
	GxSubtitleShowDisplay pos;
	int def_yb = 0, def_yt = 0, def_xl = 0;
	unsigned int *drawBuffer = NULL;

	if (!src || !srca) {
		gxloge("src %p, srca %p\n", src, srca);
		return -1;
	}

	if (0 == (content->width * content->height)) {
		gxloge("%s %d, width [%d] height[%d]\n",__FUNCTION__, __LINE__, content->width, content->height);
		return -1;
	}

	drawBuffer = GxSubtitleShowPixelAllocARGB888Buffer(pixel->show_handle, content->width, content->height);
	if (drawBuffer == NULL) {
		gxloge("(%d %d) ARGB8888 buffer alloc fail\n", content->width, content->height);
		return -1;
	}

	for (h = 0; h < content->height; ++h) {
		for (w = 0; w < content->width; ++w) {
			int idx = h * content->width + w;
			drawBuffer[idx] = content->global_palette[src[idx]];
		}
	}

	if (pixel->def_width == 0)
		pixel->def_width = content->orig_frame_width;
	if (pixel->def_height == 0)
		pixel->def_height = content->orig_frame_height;
	find_right_window(content->width, content->height, &pixel->def_width, &pixel->def_height);

	def_yb = subtitleConfig.defDisPos.yb;
	def_yt = (pixel->def_height - (def_yb + content->height));
	def_yt = (def_yt > 0) ? def_yt : 0;
	def_xl = (pixel->def_width - content->width) / 2;

	pos.xl = def_xl * pixel->cv_width  / pixel->def_width;
	pos.xr = pos.xl;
	pos.yb = def_yb * pixel->cv_height / pixel->def_height;
	pos.yt = def_yt * pixel->cv_height / pixel->def_height;
	pos.autoPos = 0;

	GxSubtitleShowPixelSetScreenColor(pixel->show_handle, 0x00000000);
	GxSubtitleShowPixelSetDisplayPos (pixel->show_handle, &pos);
	GxSubtitleShowPixelDrawARGB8888Buffer(pixel->show_handle, drawBuffer, 0);
	GxSubtitleShowPixelFreeARGB8888Bufeer(pixel->show_handle, drawBuffer);

	return 0;
}

int GxPixelSubtitle_Init(GxPixelSubtitleWindow *rect, GxPixelSubtitleRender render)
{
	GxPixelSubtitle *pixel = av_mallocz(sizeof(GxPixelSubtitle));
	GxSubtitleShowCanvas cv;
	GxSubtitleShowScreen sr;

	if (pixel == NULL) {
		gxloge("pixel subtitle malloc failed\n");
		return 0;
	}

	memcpy(&cv, &subtitleConfig.canvas, sizeof(GxSubtitleShowCanvas));
	memcpy(&sr, &subtitleConfig.screen, sizeof(GxSubtitleShowScreen));
	pixel->show_handle = GxSubtitleShowPixelOpen(&cv, &sr);
	pixel->cv_width  = cv.w;
	pixel->cv_height = cv.h;;

	return (int)(pixel);
}

void GxPixelSubtitle_Destory(int handle)
{
	GxPixelSubtitle *pixel = (GxPixelSubtitle *)handle;

	if (pixel == NULL) {
		gxlogd("%s %d\n",__FUNCTION__, __LINE__);
		return;
	}

	GxSubtitleShowPixelClose(pixel->show_handle);
	av_free(pixel);
	return;
}

int GxPixelSubtitle_Show(int handle)
{
	GxSubtitleShowPixelShow();
	return 0;
}

int GxPixelSubtitle_Hide(int handle)
{
	GxSubtitleShowPixelHide();
	return 0;
}

int GxPixelSubtitle_Draw(int handle, GxPixelSubtitleContent *content, int track, GxPixelSubtitleEncode encode)
{
	GxPixelSubtitle *pixel = (GxPixelSubtitle *)handle;

	if (pixel == NULL) {
		gxlogd("%s %d\n",__FUNCTION__, __LINE__);
		return -1;
	}

	if (content->bitmap)
		draw_bitmap_1(pixel, content);
	else
		draw_bitmap_0(pixel, content);

	return 0;
}

void GxPixelSubtitle_Clean(int handle)
{
	GxPixelSubtitle *pixel = (GxPixelSubtitle *)handle;

	if (pixel == NULL) {
		gxlogd("%s %d\n",__FUNCTION__, __LINE__);
		return;
	}

	GxSubtitleShowPixelClean(pixel->show_handle);
	return;
}


