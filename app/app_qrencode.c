#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdi_core.h"
#include "gui_core.h"
#include "gxcore.h"
#include "gxbus.h"
#include "app_config.h"
#include "app_qrencode.h"
#include "module/app_log.h"
#if QR_POP_SUPPORT

typedef struct qrencode_handle_s
{
	GuiSurface *surface;
	int margin;
	int size;
}qrencode_handle_t;

static unsigned char fg_color[4] = {0, 0, 0, 0};
static unsigned char bg_color[4] = {255, 255, 255, 255};

static void fillRow(unsigned char *row, int num, const unsigned char color[],int bpp)
{
	int i;
	for(i = 0; i < num; i++)
	{
		memcpy(row, color, bpp);
		row += bpp;
	}
}

unsigned int qren_encode_init(char *url, int bpp)
{
	QRcode *qrcode;
	GAL_Rect rect = {0, 0, 164, 164};
	unsigned char* png_ptr =NULL;
	unsigned char *row, *p;
	int x, y, xx, yy;
	int realwidth;
	int s_bpp = bpp/8;
	qrencode_handle_t *handle = 0;
	GuiSurface *surface =NULL;

	handle = (qrencode_handle_t *)malloc(sizeof(qrencode_handle_t));
	memset(handle, 0, sizeof(qrencode_handle_t));
	handle->margin = 4;
	handle->size = 2;

	/* must be version 0, QR_ECLEVEL_H and case sensitive */
	qrcode = QRcode_encodeString(url, 0, QR_ECLEVEL_H, QR_MODE_8, 1);
	realwidth = (qrcode->width + handle->margin * s_bpp) * handle->size;
	//DEBUG("realwidth=%d\n",realwidth);

	row = (unsigned char *)malloc(realwidth * s_bpp);
	rect.w = realwidth;
	rect.h = realwidth;
	surface = hd_get_surface(&rect, bpp, NULL, 0);
	if(!surface)
	{
		app_log_error("error!\n");
		free(handle);
		handle = 0;
		goto end;
	}
	handle->surface = surface;
	png_ptr = surface->data;

	GXGDI_Begin();
	/* top margin */
	fillRow(row, realwidth, bg_color,s_bpp);
	for(y = 0; y < handle->margin * handle->size; y++)
	{
		memcpy(png_ptr, row,realwidth*s_bpp);
		png_ptr =png_ptr+(realwidth*s_bpp);
	}

	/* data */
	p = qrcode->data;
	for(y = 0; y < qrcode->width; y++)
	{
		fillRow(row, realwidth, bg_color,s_bpp);
		for(x = 0; x < qrcode->width; x++)
		{
			for(xx = 0; xx < handle->size; xx++)
			{
				if(*p & 1)
				{
					memcpy(&row[((handle->margin + x) * handle->size + xx) * s_bpp], fg_color, s_bpp);
				}
			}
			p++;
		}

		for(yy = 0; yy < handle->size; yy++)
		{
			memcpy(png_ptr, row,realwidth*s_bpp);
			png_ptr = png_ptr+(realwidth*s_bpp);
		}
	}

	/* bottom margin */
	fillRow(row, realwidth, bg_color,s_bpp);
	for(y = 0; y < handle->margin * handle->size; y++)
	{
		memcpy(png_ptr, row,realwidth*s_bpp);
		png_ptr =png_ptr+(realwidth*s_bpp);
	}
	GXGDI_End();

end:
	free(row);
	QRcode_free(qrcode);
	QRcode_clearCache();

	return (unsigned int)handle;
}

void qren_encode_show(unsigned int handle, const char* img_fd)
{
	qrencode_handle_t *qr_handle = NULL;
	status_t ret = 0;

	qr_handle = (qrencode_handle_t *)handle;
	if((!qr_handle) || (!qr_handle->surface))
	{
		return;
	}
	ret = GUI_SetProperty(img_fd, "surface", qr_handle->surface);
	GUI_SetInterface("flush", NULL);
	if(ret != GXCORE_SUCCESS)
	{
		app_log_error("GUI_SetProperty failed\n");
	}
}

void qren_encode_close(unsigned int handle)
{
	qrencode_handle_t *qr_handle = NULL;

	if(qr_handle)
	{
		if(qr_handle->surface)
		{
			hd_free_surface(qr_handle->surface);
			qr_handle->surface = NULL;
		}
		free(qr_handle);
		qr_handle = NULL;
	}
}

#endif
