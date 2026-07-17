#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef NO_OS
#include <sys/stat.h>
#include "gui_private.h"
#else
#include "gui_core.h"
#include "mini_gui_private.h"
extern GuiCore gui;
GuiSurface *screen = NULL;
GuiSurface *back_screen = NULL;
int gui_set_clut(void *surface, Color *colors, int firstcolor, int ncolors);
#endif /*NO_OS*/

#include "gal.h"
#include "hd_gal.h"

#include "IMG.h"

#ifndef NO_OS
static int s_bmp_state;

#ifndef  IMG_MAX_LOG_COLORS
#define  IMG_MAX_LOG_COLORS (256)
#endif

#ifdef SWITCH_ELEB
static int _16BPP_Add_Alpha(const image_desc *pImage)
{
	unsigned short *data;
	int i;

	if (pImage == NULL || pImage->bpp != 16 || pImage->data == NULL)
		return 1;

	data = (unsigned short *)(pImage->data);

	for(i = 0; i < pImage->width * pImage->height; i++)
		data[i] = ((data[i] << 8) & 0xFF00) | ((data[i] >> 8) & 0x00FF);

	return (0);
}
#endif

static int _DrawBitmap_Pal(const int *pPal, const char *pIn, int Width, int Height, int isReversed, unsigned short *pOut, int bpp)
{
	unsigned char *pRGB = NULL;
	unsigned short *pYUV = NULL;
	unsigned int *pClut = NULL;
	unsigned int line_num = 0, index = 0, color_index = 0, byte_per_line = 0, rgb_value = 0;
	int offset = 0;

	if((NULL == pPal) || (NULL == pIn) || (NULL == pOut))
		return (1);

	pClut = (unsigned int *)pPal;
	pRGB = (unsigned char *)pIn;
	pYUV = pOut;

	line_num = 0;
	index = 0;
	byte_per_line = (((Width * bpp)+31)>>5)<<2;

	switch(bpp) {
		case 1:
			while(line_num < Height) {
				index = 0;
				while(index < Width) {
					color_index = (pRGB[index / 8]) >> (7 - (index % 8)) & 0x1;
					color_index = gal_rgb2yuv(pClut[color_index]);

					pYUV[index] = YUV2YUV422(color_index, index);

					index++;
				}
				pYUV += Width;
				pRGB += byte_per_line;
				line_num++;
			}
			break;
		case 4:
			if(Width % 2) {
				Width -= 1;
			}
			while(line_num < Height) {
				index = 0;
				while(index < Width) {
					color_index = (pRGB[index / 2] >> (4 - (index % 2 * 4))) & 0xF;
					rgb_value = pClut[color_index];
					rgb_value = ((rgb_value << 16) & 0xFF0000) |
					             (rgb_value & 0x00FF00) |
						     ((rgb_value >> 16) & 0x0000FF);
					color_index = gal_rgb2yuv(pClut[color_index]);

					pYUV[index] = YUV2YUV422(color_index, index);

					index++;
				}
				pYUV += Width;
				pRGB += byte_per_line;
				line_num++;
			}
			break;
		case 8:
			if(Width % 2) {
				Width -= 1;
			}
			while(line_num < Height) {
				index = 0;
				if(isReversed)
					offset = byte_per_line * line_num;
				else
					offset = byte_per_line * (Height - line_num - 1);
				while(index < Width) {
					color_index = (unsigned int)(pRGB[offset + index]);
					rgb_value = pClut[color_index];
					rgb_value = ((rgb_value << 16) & 0xFF0000) |
					             (rgb_value & 0x00FF00) |
						     ((rgb_value >> 16) & 0x0000FF);
					color_index = gal_rgb2yuv(rgb_value);
					pYUV[index] = YUV2YUV422(color_index, index);

					index++;
				}
				pYUV += Width;
				line_num++;
			}
			break;
		default:
			break;
	}

	return (0);
}

static int _DrawBitmap_24bpp(const char *pIn, int Width, int Height, int isReversed, unsigned short *pOut)
{
	char * pRGB = NULL;
	unsigned short *pYUV = NULL;
	unsigned int line_num = 0, index = 0, byte_per_line = 0, color = 0;
	int offset = 0;

	if(NULL == pIn || NULL == pOut)
		return 1;

	pRGB = (char *)pIn;
	pYUV = pOut;

	line_num = 0;
	byte_per_line = (((Width * 24)+31)>>5)<<2;
	if(Width % 2)
	{
	    Width -= 1;
	}
	while(line_num < Height) {
		index = 0;
		if(isReversed)
			offset = byte_per_line * line_num;
		else
			offset = byte_per_line * (Height - line_num - 1);
		while(index < Width) {
			color = (((int)(pRGB[offset + index * 3])) << 16) |
				(((int)(pRGB[offset + index * 3 + 1])) << 8) |
				(pRGB[offset + index * 3 + 2]);

			color = gal_rgb2yuv(color);
			pYUV[index] = YUV2YUV422(color, index);
			index++;
		}
		pYUV += Width;
		line_num++;
	}

	return 0;
}

static int _DrawBitmap_32bpp(const char *pIn, int Width, int Height, unsigned short *pOut)
{
	char * pRGB = NULL;
	unsigned short *pYUV = NULL;
	unsigned int line_num = 0, index = 0, byte_per_line = 0, color = 0;
	int offset = 0;

	if(NULL == pIn || NULL == pOut)
		return 1;

	pRGB = (char *)pIn;
	pYUV = pOut;

	line_num = 0;
	byte_per_line = (((Width * 32)+31)>>5)<<2;
	while(line_num < Height) {
		index = 0;
		offset = byte_per_line * (Height - line_num - 1);
		while(index < Width) {
			color = (((int)(pRGB[offset + index * 4 + 2])) << 16) |
				(((int)(pRGB[offset + index * 4 + 1])) << 8) |
				(pRGB[offset + index * 4]);

			color = gal_rgb2yuv(color);
			pYUV[index] = YUV2YUV422(color, index);
			index++;
		}
		pYUV += Width;
		line_num++;
	}

	return 0;
}

static int _DrawBitmap_16bpp(const char *pIn,
		int Width,
		int Height,
		int isReversed,
		unsigned int Rmask,
		unsigned int Gmask,
		unsigned int Bmask,
		unsigned short *pOut)
{
	unsigned short *pRGB = NULL, *pYUV = NULL;
	int line_num = 0, index = 0, byte_per_line = 0;
	int color = 0, Rshift = 0, Gshift = 0, Bshift = 0;

	if(NULL == pIn || NULL == pOut)
		return 1;

	pRGB = (unsigned short *)pIn;
	pYUV = pOut;

	byte_per_line = (((Width * 16)+31)>>5)<<2;

	if((0xF800 == Rmask) && (0x07E0 == Gmask) && (0x001F == Bmask)) {
		Rshift = 8;
		Gshift = 3;
		Bshift = 3;
	}
	else if((0x7C00 == Rmask) && (0x03E0 == Gmask) && (0x001F == Bmask)) {
		Rshift = 7;
		Gshift = 2;
		Bshift = 3;
	}
	else if((0x0F00 == Rmask) && (0x00F0 == Gmask) && (0x000F == Bmask)) {
		Rshift = 4;
		Gshift = 0;
		Bshift = 4;
	}

	if(Width % 2)
	{
	    Width -= 1;
	}
	line_num = 0;
	while(line_num < Height) {
		index = 0;
		if(isReversed)
			pRGB = (unsigned short *)(pIn + byte_per_line * line_num);
		else
			pRGB = (unsigned short *)(pIn + byte_per_line * (Height - line_num - 1));
		while(index < Width) {
			color = (((pRGB[index] & Bmask) << Bshift) << 16) |
				(((pRGB[index] & Gmask) >> Gshift) << 8) |
				((pRGB[index] & Rmask) >> Rshift);
			color = gal_rgb2yuv(color);
			pYUV[index] = YUV2YUV422(color, index);
			index++;
		}
		pYUV += Width;
		line_num++;
	}

	return 0;
}

void convert_rgb_2_yuv422(void* pin, void* pout, unsigned int width, unsigned int height, unsigned int bpp, void *pclut)
{
	unsigned char *psrc = NULL, *pdst = NULL, *pal = NULL;
	unsigned int i = 0;

	if((NULL == pin) || (NULL == pout))
		return;

	psrc = (unsigned char *)pin;
	pdst = (unsigned char *)pout;
	pal  = (unsigned char *)pclut;

	while(i < height)
	{
		switch(bpp)
		{
			case 1:
			case 4:
			case 8:
				_DrawBitmap_Pal((const int *)pal,
						(const char *)(psrc + i * width * bpp / 8),
						width,
						1,
						false,
						(unsigned short *)(pdst + i * width * 2),
						bpp);
				break;
			case 16:
				_DrawBitmap_16bpp((const char *)(psrc + i * width * bpp / 8),
						width,
						1,
						false,
						0xF800,
						0x07E0,
						0x001F,
						(unsigned short *)(pdst + i * width * 2));
				break;
			case 24:
				_DrawBitmap_24bpp((const char *)(psrc + i * width * bpp / 8),
						width,
						1,
						false,
						(unsigned short *)(pdst + i * width * 2));
				break;
			case 32:
				_DrawBitmap_32bpp((const char *)(psrc + i * width * bpp / 8),
						width,
						1,
						(unsigned short *)(pdst + i * width * 2));
				break;
			default:
				break;
		}
		i++;
	}
}

static int _slow_display_bmp(void *surface, const char *pBMP, int x0, int y0)
{
	unsigned short BitCount = 0;
	unsigned short Type = 0;
	int Width = 0, Height = 0, Offset = 0, HeadSize = 0, isReversed = false;
	unsigned int ClrUsed = 0, Compression = 0, Clut[256] = {0};
	unsigned int real_width = 0, real_height = 0, real_offset = 0;
	int NumColors = 0, ret = 0, x_pos = 0, y_pos = 0, i = 0;
	unsigned char *pBuf = NULL, *pData = NULL, *pTr = NULL;
	GuiCore *gui_core = NULL;
	FILE* fbmp = NULL;

	if((NULL == surface) || (NULL == pBMP))
		return (1);

	fbmp = fopen((const char *)pBMP, "r");
	if(NULL == fbmp)
		return (1);

	gui_core = GUI_GetCurrent();
	fseek(fbmp, 0, SEEK_SET);
	Type = FILE_Read16((const FILE *)fbmp);

	fseek(fbmp, 0xA, SEEK_SET);
	Offset = FILE_Read32((const FILE *)fbmp);

	fseek(fbmp, 0xE, SEEK_SET);
	HeadSize = FILE_Read32((const FILE *)fbmp);

	fseek(fbmp, 0x12, SEEK_SET);
	Width = FILE_Read32((const FILE *)fbmp);

	fseek(fbmp, 0x16, SEEK_SET);
	Height = FILE_Read32((const FILE *)fbmp);
	if(0 > Height) {
		Height = 0 - Height;
		isReversed = true;
	}

	fseek(fbmp, 0x1C, SEEK_SET);
	BitCount = FILE_Read16((const FILE *)fbmp);

	fseek(fbmp, 0x1E, SEEK_SET);
	Compression = FILE_Read32((const FILE *)fbmp);

	fseek(fbmp, 0x22, SEEK_SET);
	FILE_Read32((const FILE *)fbmp);

	fseek(fbmp, 0x2E, SEEK_SET);
	ClrUsed = FILE_Read32((const FILE *)fbmp);

	switch(BitCount) {
		case 0:
			fclose(fbmp);
			return (1);
		case 1:
			NumColors = 2;
		case 4:
			NumColors = 16;
		case 8:
			NumColors = 256;
			fseek(fbmp, HeadSize + 0xE, SEEK_SET);
			ret = fread(Clut, NumColors * 4, 1, fbmp);
			if (0 == ret)
				return (1);
			break;
		case 16:
		case 24:
			NumColors = 0;
			break;
		default:
			hd_img_clear();
			fclose(fbmp);
			return (1);
	}

	/* check validity of bmp */
	if (NumColors && ClrUsed) {
		NumColors = ClrUsed;
	}

	if ((NumColors > IMG_MAX_LOG_COLORS) ||
			(Type != 0x4D42) ||	/* 'BM' */
			(Compression) ||	/* only uncompressed bitmaps */
			(Width > gui_core->config.width) || (Height > gui_core->config.height)) {
		fclose(fbmp);
		hd_img_clear();
		return 1;
	}

	real_width = (Width > 720) ? 720 : Width;
	real_height = (Height > 576) ? 576 : Height;

	pBuf = (unsigned char *)GUI_MALLOC(Width * (BitCount >> 3));
	if(NULL == pBuf) {
		fclose(fbmp);
		hd_img_clear();
		return 1;
	}

	pData = (unsigned char *)GUI_MALLOC(real_width * real_height * 2);
	if(NULL == pData) {
		GUI_FREE(pBuf);
		fclose(fbmp);
		hd_img_clear();
		return 1;
	}

	/* Show bitmap */
	i = 0;
	real_offset = (((Width * BitCount)+31)>>5)<<2;
	fseek(fbmp, Offset, SEEK_SET);
	while(i < real_height) {
		ret = fread(pBuf, real_offset, 1, fbmp);
		if(0 == ret) {
			GUI_FREE(pBuf);
			GUI_FREE(pData);
			fclose(fbmp);
			return (1);
		}

		if(isReversed)
			pTr = (pData + i * real_width * 2);
		else
			pTr = pData + (real_height - i - 1) * real_width * 2;
		switch (BitCount) {
			case 1:
			case 4:
			case 8:
				ret = _DrawBitmap_Pal((const int *)Clut,
						(const char *)pBuf,
						real_width,
						1,
						false,
						(unsigned short *)pTr,
						BitCount);
				break;
			case 16:
				ret = _DrawBitmap_16bpp((const char *)pBuf,
						real_width,
						1,
						false,
						0xF800,
						0x07E0,
						0x001F,
						(unsigned short *)pTr);
				break;
			case 24:
				ret = _DrawBitmap_24bpp((const char *)pBuf,
						real_width,
						1,
						false,
						(unsigned short *)pTr);
				break;
			case 32:
				ret = _DrawBitmap_32bpp((const char *)pBuf,
						real_width,
						1,
						(unsigned short *)pTr);
				break;
			default:
				break;
		}
		i++;
	}

	if(real_width < gui_core->config.width) {
		x_pos = x0 + (gui_core->config.width - real_width) / 2;
		x_pos = x_pos - (x_pos % 2);
	}

	if(real_height < gui_core->config.height) {
		y_pos =y0 + (gui_core->config.height - real_width) / 2;
		y_pos = y_pos - (y_pos % 2);
	}

	hd_img_copy(surface,
			(char *)pData,
			x_pos,
			y_pos,
			real_width,
			real_height,
			2);

	GUI_FREE(pBuf);
	GUI_FREE(pData);

	return (0);
}

static void bitmap_revert(unsigned char *BMP_buffer, int Width, int Height, int BitCount)
{
	unsigned char *tmp_buffer = NULL, *src = NULL, *dst = NULL;
	int i = 0, size = 0;

	size = Width * BitCount / 8;
	if(Width % 2) {
		size++;
	}
	tmp_buffer = GUI_MALLOCZ(size);

	for(i = 0; i < Height / 2; i++) {
		src = BMP_buffer + i * size;
		dst = BMP_buffer + (Height - i - 1) * size;
		memcpy(tmp_buffer, src, size);
		memcpy(src, dst, size);
		memcpy(dst, tmp_buffer, size);
	}

	GUI_FREE(tmp_buffer);
}

int IMG_BMP_Draw(void *screen, const char *pBMP, int x0, int y0, int screen_width, int screen_height)
{
	int Ret = 0;
	unsigned short BitCount;
	int Width, Height, isReversed;
	unsigned int Offset, HeadSize = 0;
	unsigned int x_pos = 0, y_pos = 0, is_os2 = 0;
	unsigned int ClrUsed = 0, Compression = 0;
	unsigned int *pClut = NULL;
	int buffer_width = 0, buffer_height = 0;
	unsigned char *BMP_buffer = NULL, *pCursor = NULL, *pData = NULL;
	GxHwMallocObj *bmp_vq = NULL, *data_vq = NULL;
	unsigned short Type = 0;
	image_desc img_desc = {0}, *pdesc = NULL;
	int NumColors = 0, ret = 0, real_width = 0, real_height = 0, read_size = 0;
	unsigned int RMask = 0, GMask = 0, BMask = 0;
	struct stat buf;
	handle_t pSrc = 0;
	void *surface = screen;
	GuiCore *gui_core = NULL;

	if (NULL == surface) {
		gxloge ( "[GUI]There is no surface\n");
		hd_img_clear();
		return (1);
	}

	ret = stat((char *)pBMP, &buf);
	if (ret < 0) {
		hd_img_clear();
		return (1);
	}

	gui_core = GUI_GetCurrent();
	pSrc = GxCore_Open((char *)pBMP, "rb");
	if(0 == pSrc) {
		hd_img_clear();
		return (1);
	}

	s_bmp_state = 1;

	BMP_buffer = (unsigned char *)GUI_MALLOC(0x80);
	if(NULL == BMP_buffer) {
		//hd_img_clear();
		GxCore_Close(pSrc);
		// TODO:
		Ret = _slow_display_bmp(surface, pBMP, x0, y0);
		return (Ret);
	}

	memset(BMP_buffer, 0, 0x80);
	GxCore_Read(pSrc, BMP_buffer, 0x80, 1);

	memcpy((char *)&Type, BMP_buffer, 2);

	pCursor = &(BMP_buffer[0xA]);
	Offset = (((unsigned int)pCursor[3]) << 24) |
		(((unsigned int)pCursor[2]) << 16) |
		(((unsigned int)pCursor[1]) << 8) |
		((unsigned int)pCursor[0]);

	pCursor = &(BMP_buffer[0xE]);
	HeadSize = (((unsigned int)pCursor[3]) << 24) |
		(((unsigned int)pCursor[2]) << 16) |
		(((unsigned int)pCursor[1]) << 8) |
		((unsigned int)pCursor[0]);
	if(0xC == HeadSize) {
		is_os2 = GUI_TRUE;
	}

	pCursor = &(BMP_buffer[0x12]);
	if(is_os2) {
		Width = (((unsigned int)pCursor[1]) << 8) |
			((unsigned int)pCursor[0]);
	} else {
		Width = (((unsigned int)pCursor[3]) << 24) |
			(((unsigned int)pCursor[2]) << 16) |
			(((unsigned int)pCursor[1]) << 8) |
			((unsigned int)pCursor[0]);
	}

	if(Width < 0) {
		Width = 0 - Width;
	}

	if(is_os2) {
		pCursor = &(BMP_buffer[0x14]);
		Height = (((unsigned int)pCursor[1]) << 8) |
			 ((unsigned int)pCursor[0]);
	} else {
		pCursor = &(BMP_buffer[0x16]);
		Height = (((unsigned int)pCursor[3]) << 24) |
			(((unsigned int)pCursor[2]) << 16) |
			(((unsigned int)pCursor[1]) << 8) |
			((unsigned int)pCursor[0]);
	}

	if(Height < 0) {
		Height = 0 - Height;
		isReversed = true;
	} else {
		isReversed = false;
	}

	if(is_os2) {
		pCursor = &(BMP_buffer[0x18]);
	} else {
		pCursor = &(BMP_buffer[0x1C]);
	}
	BitCount = ((unsigned short)(pCursor[1]) << 8) |
		   ((unsigned short)(pCursor[0]));

	if(!is_os2) {
		pCursor = &(BMP_buffer[0x1E]);
		Compression = (((unsigned int)pCursor[3]) << 24) |
			(((unsigned int)pCursor[2]) << 16) |
			(((unsigned int)pCursor[1]) << 8) |
			((unsigned int)pCursor[0]);

		pCursor = &(BMP_buffer[0x2E]);
		ClrUsed = (((unsigned int)pCursor[3]) << 24) |
			(((unsigned int)pCursor[2]) << 16) |
			(((unsigned int)pCursor[1]) << 8) |
			((unsigned int)pCursor[0]);
	}

	switch(BitCount)
	{
		case 0:
			GUI_FREE(BMP_buffer);
			GxCore_Close(pSrc);
			return (1);
		case 1:
			NumColors = 2;
		case 4:
			NumColors = 16;
			/*Get palette*/
			GxCore_Seek(pSrc, HeadSize + 0xE, SEEK_SET);
			pClut = (unsigned int *)GUI_MALLOCZ(NumColors * 4);
			if(NULL == pClut) {
				GUI_FREE(BMP_buffer);
				GxCore_Close(pSrc);
				hd_img_clear();
				return (1);
			}
			GxCore_Read(pSrc, pClut, NumColors * 4, 1);
			break;
		case 8:
			NumColors = 256;
			/*Get palette*/
			GxCore_Seek(pSrc, HeadSize + 0xE, SEEK_SET);
			pClut = (unsigned int *)GUI_MALLOCZ(NumColors * 4);
			if(NULL == pClut) {
				GUI_FREE(BMP_buffer);
				GxCore_Close(pSrc);
				hd_img_clear();
				return (1);
			}
			GxCore_Read(pSrc, pClut, NumColors * 4, 1);
			break;
		case 16:
			NumColors = 0;
			break;
		case 24:
		default:
			img_desc.filename = (char *)pBMP;
			pdesc = IMG_BMP_Load(&img_desc, pBMP);
			if(NULL == pdesc)
			{
				GUI_FREE(BMP_buffer);
				hd_img_clear();
				GxCore_Close(pSrc);
				return (1);
			}
			else
			{
				goto DISPLAY;
			}
	}

	/* check validity of bmp */
	if (NumColors && ClrUsed)
		NumColors = ClrUsed;

	if ((NumColors > IMG_MAX_LOG_COLORS) ||
			(Type != 0x4D42) ||	/* 'BM' */
			(Compression && (3 != Compression))) {
		GUI_FREE(BMP_buffer);
		GxCore_Close(pSrc);
		hd_img_clear();
		return 1;
	}

	if(3 == Compression) {
		pCursor = BMP_buffer + 0x36;
		RMask = (((unsigned int)pCursor[3]) << 24) |
			(((unsigned int)pCursor[2]) << 16) |
			(((unsigned int)pCursor[1]) << 8) |
			((unsigned int)pCursor[0]);
		pCursor = BMP_buffer + 0x3A;
		GMask = (((unsigned int)pCursor[3]) << 24) |
			(((unsigned int)pCursor[2]) << 16) |
			(((unsigned int)pCursor[1]) << 8) |
			((unsigned int)pCursor[0]);
		pCursor = BMP_buffer + 0x3E;
		BMask = (((unsigned int)pCursor[3]) << 24) |
			(((unsigned int)pCursor[2]) << 16) |
			(((unsigned int)pCursor[1]) << 8) |
			((unsigned int)pCursor[0]);

	}
	else if(16 == BitCount)
	{
	    RMask = 0x7C00;
	    GMask = 0x03E0;
	    BMask = 0x001F;
	}

	GUI_FREE(BMP_buffer);
	bmp_vq = hd_malloc_nocache(&bmp_vq, buf.st_size);
	if(bmp_vq) {
		BMP_buffer = bmp_vq->usr_p;
		if(BMP_buffer) {
			memset(BMP_buffer, 0, buf.st_size);
		}
	} else {
		BMP_buffer = (unsigned char *)GUI_MALLOCZ(buf.st_size);
	}
	if(NULL == BMP_buffer) {
		//hd_img_clear();
		GxCore_Close(pSrc);
		return (1);
	}

	GxCore_Seek(pSrc, 0, SEEK_SET);
	read_size = GxCore_Read(pSrc, BMP_buffer, 1, buf.st_size);
	if(read_size <= 0) {
		if(bmp_vq) {
			hd_free(bmp_vq);
			bmp_vq = NULL;
			BMP_buffer = NULL;
		} else {
			GUI_FREE(BMP_buffer);
		}
		GxCore_Close(pSrc);
		return (1);
	}
	pCursor = &(BMP_buffer[Offset]);
	data_vq = hd_malloc_nocache(&data_vq, Width * Height * 2);
	if(data_vq) {
		pData = (unsigned char *)(data_vq->usr_p);
	} else {
		pData = (unsigned char *)GUI_MALLOC(Width * Height * 2);
	}
	if(NULL == pData) {
		if(bmp_vq) {
			hd_free(bmp_vq);
			GUI_FREE(bmp_vq);
			bmp_vq = NULL;
			BMP_buffer = NULL;
		} else {
			GUI_FREE(BMP_buffer);
		}
		if(pClut)
			GUI_FREE(pClut);

		GxCore_Close(pSrc);
		hd_img_clear();
		return (1);
	}
	memset(pData, 0, Width * Height * 2);

	/* Show bitmap */
	switch (BitCount) {
		case 1:
		case 4:
		case 8:
			if(BitCount < 8) {
				bitmap_revert(BMP_buffer, Width, Height, BitCount);
			}
			Ret = _DrawBitmap_Pal((const int *)pClut,
					(const char *)&(BMP_buffer[Offset]),
					Width,
					Height,
					isReversed,
					(unsigned short *)pData,
					BitCount);
			break;
		case 16:
			Ret = _DrawBitmap_16bpp((const char *)&(BMP_buffer[Offset]),
					Width,
					Height,
					isReversed,
					RMask,
					GMask,
					BMask,
					(unsigned short *)pData);
			break;
		case 24:
			Ret = _DrawBitmap_24bpp((const char *)&(BMP_buffer[Offset]),
					Width,
					Height,
					isReversed,
					(unsigned short *)pData);
			break;
	}

	if(screen_width != -1)
		buffer_width = screen_width;
	else
		buffer_width = gui_core->config.width;

	if(screen_height != -1)
		buffer_height = screen_height;
	else
		buffer_height = gui_core->config.height;

	if(Width % 2)
	{
	    Width -= 1;
	}

	if(Width < buffer_width) {
		x_pos = x0 + (buffer_width - Width) / 2;
		x_pos = x_pos - (x_pos % 2);
	}

	if(Height < buffer_height) {
		y_pos =y0 + (buffer_height - Height) / 2;
		y_pos = y_pos - (y_pos % 2);
	}

DISPLAY:
	if(s_bmp_state) {
		if(NULL == pdesc)
		{
			img_desc.width = Width;
			img_desc.height = Height;
			img_desc.bpp = 16;
			img_desc.size = Width * Height * 2;
			img_desc.type = JPEG_TYPE;
			img_desc.effect = EFFECT_COPY;
			img_desc.data = pData;
		}

		if(Width < gui_core->config.width)
		{
		    x0 += (gui_core->config.width - Width) / 2;
		    if((0 != x0 % 2) && x0)
			x0 -= 1;
		}

		if(Height < gui_core->config.height)
		    y0 += (gui_core->config.height - Height) / 2;

		real_width = img_desc.width;
		real_height = img_desc.height;
		if(real_width > gui_core->config.width)
			real_width = gui_core->config.width;
		if(real_height > gui_core->config.height)
			real_height = gui_core->config.height;
		if(bmp_vq) {
			hd_free(bmp_vq);
			GUI_FREE(bmp_vq);
			bmp_vq = NULL;
			BMP_buffer = NULL;
		} else {
			GUI_FREE(BMP_buffer);
		}
		ret = hd_img_desc_gradiant(screen, &img_desc, x0, y0, real_width, real_height, gui_core->config.play_image_mode);
		if(ret) {
			if(pdesc)
			{
				gal_img_cleanup(pdesc);
				GUI_FREE(pdesc->vq);
			}
			if(pClut)
				GUI_FREE(pClut);
			GxCore_Close(pSrc);
			hd_img_clear();
			return (1);
		}
		hd_commit_blit();
		//hd_img_copy(surface, (char *)pData, x_pos, y_pos, Width, Height, 2);
		hd_enable_video(GUI_FALSE);
		gal_enable_img(GUI_TRUE);
		hd_flip();
	}

	/*Patch*/
	if(!s_bmp_state) {
		widget_set_update(stack_peek(gui_core->win_stack), GUI_TRUE);
		hd_flip();
	}

	s_bmp_state = 1;
	if(pdesc)
	{
		gal_img_cleanup(pdesc);
		GUI_FREE(pdesc->vq);
	}

	if(pClut)
		GUI_FREE(pClut);

	if(data_vq) {
		hd_free(data_vq);
		GUI_FREE(data_vq);
		data_vq = NULL;
	} else {
		GUI_FREE(pData);
	}
	if(bmp_vq) {
		hd_free(bmp_vq);
		GUI_FREE(bmp_vq);
		bmp_vq = NULL;
	} else {
		GUI_FREE(BMP_buffer);
	}
	GxCore_Close(pSrc);
	//UNLOCK();
	return Ret;
}


int IMG_BMP_GetInfo(const char *pFile, Image_Exif *img_info)
{
	int ret = 0;
	unsigned char *buffer =NULL, *pCursor = NULL;
	struct stat buf = {0};
	handle_t hfile = 0;

	if((NULL == pFile) || (NULL == img_info))
		return (1);

	hfile = GxCore_Open((char *)pFile, "r");
	if(0 == hfile)
		return (1);

	ret = stat((char *)pFile, &buf);
	if (ret < 0) {
		GxCore_Close(hfile);
		return (1);
	}

	buffer = (unsigned char *)GUI_MALLOC(0x80);
	if(NULL == buffer) {
		GxCore_Close(hfile);
		return (1);
	}

	memset(buffer, 0, 0x80);

	GxCore_Read(hfile, buffer, 0x80, 1);

	pCursor = buffer + 0x12;
	img_info->width = (((unsigned int)pCursor[3]) << 24) |
		(((unsigned int)pCursor[2]) << 16) |
		(((unsigned int)pCursor[1]) << 8) |
		((unsigned int)pCursor[0]);
	pCursor = buffer + 0x16;
	img_info->height = (((unsigned int)pCursor[3]) << 24) |
		(((unsigned int)pCursor[2]) << 16) |
		(((unsigned int)pCursor[1]) << 8) |
		((unsigned int)pCursor[0]);

	pCursor = buffer + 0x1A;
	img_info->frame_num = (((unsigned short)pCursor[1]) << 8) | ((unsigned short)pCursor[0]);

	pCursor = buffer + 0x1C;
	img_info->bpp = (((unsigned short)pCursor[1]) << 8) | ((unsigned short)pCursor[0]);

	pCursor = buffer + 0x26;
	img_info->horizontal_pixel = (((unsigned int)pCursor[3]) << 24) |
		(((unsigned int)pCursor[2]) << 16) |
		(((unsigned int)pCursor[1]) << 8) |
		((unsigned int)pCursor[0]);

	pCursor = buffer + 0x2A;
	img_info->vertical_pixel = (((unsigned int)pCursor[3]) << 24) |
		(((unsigned int)pCursor[2]) << 16) |
		(((unsigned int)pCursor[1]) << 8) |
		((unsigned int)pCursor[0]);

	img_info->type = IMAGE_BMP;

	GUI_FREE(buffer);

	GxCore_Close(hfile);

	return (ret);
}

int IMG_BMP_STOP(void)
{
	s_bmp_state = 0;
	return (0);
}
#endif /*NO_OS*/

union w
{
    int a;
    char b;
};

int check_cpu(void)
{
    union w c;

    c.a = 1;
    return (c.b == 1);
}

static void _convert_pal(Color *src, Color *dst, int num)
{
    int i = 0;

    for(i = 0; i < num; i++)
    {
	dst[i] = src[i] & 0x00FFFFFF;
    }
}

static int _32BPP_Add_Alpha(const image_desc *pImage, int rmask, int gmask, int bmask)
{
	unsigned char *data = NULL;
	int i;

	if (pImage == NULL || pImage->bpp != 32 || pImage->data == NULL)
		return 1;

	data = (unsigned char *)(pImage->data);

	for(i = 0; i < pImage->width * pImage->height * pImage->bpp / 8;)
	{
		if((rmask == 0xFF000000) &&
		   (gmask == 0x00FF0000) &&
		   (bmask == 0x0000FF00)) {
			memcpy(data + i, data + i + 1, 3);
			data[i + 3] = 0xFF;//data[i];
		} else if((rmask == 0x0000FF00) &&
			  (gmask == 0x00FF0000) &&
			  (bmask == 0xFF000000)) {

		} else if((rmask == 0x00FF0000) &&
			  (gmask == 0x0000FF00) &&
			  (bmask == 0x000000FF)) {
		} else if((rmask == 0x000000FF) &&
			  (gmask == 0x0000FF00) &&
			  (bmask == 0x00FF0000)) {
		} else {
			data[i + 3] = 0xFF;
		}
		i += (pImage->bpp / 8);
	}

	return (0);
}

static GxColorFormat _get_color_format_by_mask(int rmask, int gmask, int bmask){
	if((0x7C00 == rmask) &&
	   (0x03E0 == gmask) &&
	   (0x001F == bmask)){
		return (GX_COLOR_FMT_ARGB1555);
	}else if((0xF800 == rmask) &&
		 (0x07E0 == gmask) &&
		 (0x001F == bmask)){
		return (GX_COLOR_FMT_RGB565);
	}else if((0x0F00 == rmask) &&
		 (0x00F0 == gmask) &&
		 (0x000F == bmask)){
		return (GX_COLOR_FMT_ARGB4444);
	}else{
		return (GX_COLOR_FMT_RGB565);
	}
}

image_desc *IMG_BMP_Load(image_desc *desc, const char *pFile)
{
	GuiCore *gui_core = NULL;
	int *pal = NULL, compression = 0, is_os2 = 0;
	int rmask = 0, gmask = 0, bmask = 0, headsize = 0;//, amask = 0;
	char *data = NULL, *filename = NULL, *ptr = NULL;
	GuiClut *pclut = NULL, *pnewclut = NULL;
	unsigned char *BMP_buffer = NULL, *temp = NULL;
#ifndef NO_OS
	struct stat buf;
#endif /*NO_OS*/
	int ret = 0, offset = 0, i = 0, delta = 0, height = 0, width = 0, bpp = 0;
	int color_num = 0, pal_index = 0, from_file = 0, is_reversed = false;
	handle_t pSrc = 0;

	gui_core = GUI_GetCurrent();

#define BMP_READ_CELL_SIZE    (8*1024) //兼顾速度和内存消耗，至少大于1行数据的长度(宽度*bpp/8)
	if (desc) {
		pFile = desc->filename;
#ifndef NO_OS
		ret = stat(desc->filename, &buf);
		if (ret < 0)
			return NULL;
#endif /*NO_OS*/
	}
	else {
#ifndef NO_OS
		ret = stat(pFile, &buf);
		if (ret < 0)
			return NULL;
#endif /*NO_OS*/
		desc = gal_img_new();
		if(pFile)
		{
		    filename = GUI_STRDUP(pFile);
		    GUI_FREE(desc->filename);
		    desc->filename = filename;
		}
		from_file = GUI_TRUE;
	}

	pSrc = GxCore_Open(desc->filename, "r");

#ifndef NO_OS
	if (pSrc < 0)
#else
	if (pSrc == 0)
#endif /*NO_OS*/
		return NULL;

	desc->type = BMP_TYPE;

	BMP_buffer = (unsigned char *)GUI_MALLOC(BMP_READ_CELL_SIZE);
	if(NULL == BMP_buffer) {
		GxCore_Close(pSrc);
		return NULL;
	}

	ret = GxCore_Read(pSrc, BMP_buffer, 1, BMP_READ_CELL_SIZE);
	if(0 >= ret) {
		GUI_FREE(BMP_buffer);
		GxCore_Close(pSrc);
		return NULL;
	}

	temp = &BMP_buffer[14];
	headsize = temp[0] | (temp[1] << 8) | (temp[2] << 16) | (temp[3] << 24);
	if(0xC == headsize) {
		is_os2 = GUI_TRUE;
	}

	temp = &BMP_buffer[18];
	if(is_os2) {
		width = desc->width = temp[0] | (temp[1] << 8);
	} else {
		width = desc->width = temp[0] | (temp[1] << 8) | (temp[2] << 16) | (temp[3] << 24);
	}
	if(width < 0) {
		width = 0 - width;
	}

	if(is_os2) {
		temp = &BMP_buffer[20];
		height = desc->height = temp[0] | (temp[1] << 8);
	} else {
		temp = &BMP_buffer[22];
		height = desc->height = temp[0] | (temp[1] << 8) | (temp[2] << 16) | (temp[3] << 24);
		if(height < 0) {
			desc->height = height = 0 - height;
			is_reversed = true;
		}
	}

	if(is_os2) {
		temp = &BMP_buffer[0x18];
		bpp = desc->bpp = temp[0] | (temp[1] << 8);
	} else {
		temp = &BMP_buffer[0x1C];
		bpp = desc->bpp = temp[0] | (temp[1] << 8);
	}

	if((0 == width) || (0 == height) || (0 == bpp)) {
		GUI_FREE(BMP_buffer);
		GxCore_Close(pSrc);
		return NULL;
	}

	if(!is_os2) {
		temp = &(BMP_buffer[0x1E]);
		compression = (((unsigned int)temp[3]) << 24) |
			(((unsigned int)temp[2]) << 16) |
			(((unsigned int)temp[1]) << 8) |
			((unsigned int)temp[0]);

		temp = &BMP_buffer[0x2E];
		color_num = temp[0] | (temp[1] << 8) | (temp[2] << 16) | (temp[3] << 24);
	}

	if(0 == color_num)
		color_num = 1 << bpp;

	if (8 >= desc->bpp) {
		temp = &BMP_buffer[headsize + 0xE];
		pal = (int *)GUI_MALLOC(color_num * sizeof(int));
		if(('B' == temp[0]) &&
		   ('G' == temp[1]) &&
		   ('R' == temp[2]) &&
		   ('s' == temp[3]))
		{
			temp += 0x44;
		}
		memcpy(pal, temp, color_num * sizeof(int));

		if(!check_cpu())
		{
		    for(i = 0; i < color_num; i++)
		    {
			pal[i] = ((pal[i] << 24)&0xFF000000)
				| ((pal[i] << 8) & 0x00FF0000)
				| ((pal[i] >> 8) & 0x0000FF00)
				| ((pal[i] >> 24)&0x000000FF);
		    }
		}

		desc->pal = pal;
		if(NULL == gui_core->config.clut) {
			//gxlogd("[CLUT] create first clut, img: %s\n", desc->filename);
			gui_core->config.firstclut = gui_core->config.clut = (GuiClut *)GUI_MALLOC(sizeof(GuiClut));
			memset(gui_core->config.clut, 0, sizeof(GuiClut));
			gui_core->config.clut->num = color_num;
			gui_core->config.clut->palette = (Color* )GUI_MALLOC(color_num * sizeof(Color));//desc->pal;
			if(gui_core->config.clut->palette)
			{
				memcpy(gui_core->config.clut->palette,
						desc->pal,
						color_num * sizeof(Color));
			}

			if(8 == gui_core->config.bpp)
			{
			    if(screen)
			    {
				gui_set_clut(screen, gui_core->config.clut->palette, 0, gui_core->config.clut->num);
			    }
			    if(back_screen)
			    {
				gui_set_clut(back_screen, gui_core->config.clut->palette, 0, gui_core->config.clut->num);
			    }
			}

			desc->pal_index = 0;
		}
		else {
			GuiClut* plast_clut = NULL;
			Color tmp_pal[256] = {0};

			pclut = gui_core->config.firstclut;
			pal_index = 0;
			while(pclut) {
				_convert_pal(pclut->palette, tmp_pal, color_num);
				if(0 == memcmp(tmp_pal, desc->pal, color_num * sizeof(Color))) {
					//gxlogd("[CLUT] clut already exsit, index :%d\n", pal_index);
					desc->pal_index = pal_index;
					break;
				}
				else {
					pal_index++;
					plast_clut = pclut;
					pclut = pclut->next;
				}
			}

			if(NULL == pclut) {
				gxlogd("[GUI][CLUT] add new clut, img: %s, index :%d\n", desc->filename,  pal_index);

				pnewclut = (GuiClut *)GUI_MALLOC(sizeof(GuiClut));
				memset(pnewclut, 0, sizeof(GuiClut));
				pnewclut->num = color_num;
				pnewclut->palette = (Color* )GUI_MALLOC(color_num * sizeof(Color));//desc->pal;

				if(pnewclut->palette) {
					memcpy(pnewclut->palette,
							desc->pal,
							color_num * sizeof(Color));
				}
				pnewclut->next = NULL;
				plast_clut->next = pnewclut;

				desc->pal_index = pal_index;
			}
		}

		if(!from_file)
		    GUI_FREE(desc->pal);
	}
	else if((32 == desc->bpp) || (16 == desc->bpp))
	{
		if(3 == compression) {
			temp = BMP_buffer + 0x36;
			rmask = (((unsigned int)temp[3]) << 24) |
				(((unsigned int)temp[2]) << 16) |
				(((unsigned int)temp[1]) << 8) |
				((unsigned int)temp[0]);
			temp = BMP_buffer + 0x3A;
			gmask = (((unsigned int)temp[3]) << 24) |
				(((unsigned int)temp[2]) << 16) |
				(((unsigned int)temp[1]) << 8) |
				((unsigned int)temp[0]);
			temp = BMP_buffer + 0x3E;
			bmask = (((unsigned int)temp[3]) << 24) |
				(((unsigned int)temp[2]) << 16) |
				(((unsigned int)temp[1]) << 8) |
				((unsigned int)temp[0]);
		}
	}

	temp = &BMP_buffer[0xA];
	ret = temp[0] | (temp[1] << 8) | (temp[2] << 16) | (temp[3] << 24);
	temp = &BMP_buffer[0x22];
	offset = temp[0] | (temp[1] << 8) | (temp[2] << 16) | (temp[3] << 24);
	delta = (((width * bpp) + 31) >> 5) << 2;
	if (0 == offset) {
		offset = delta * height * bpp / 8;
	}

	int cur_cell_lines = 0;
	int total_read_lines = 0;
	int adjust_width = 0;
	int max_lines_per_cell = BMP_READ_CELL_SIZE/delta;

	if (delta > BMP_READ_CELL_SIZE){
		GUI_FREE(BMP_buffer);
		BMP_buffer = GUI_MALLOC(delta * 5);
		max_lines_per_cell = 5;
	}

	temp = BMP_buffer;
	desc->size = desc->width * desc->height *desc->bpp / 8;
	data = gal_img_alloc_memory(desc);
	if(NULL == data) {
		GUI_FREE(BMP_buffer);
		GxCore_Close(pSrc);
		return (NULL);
	}

	for (i = 0; i < height; i++) {
		if(bpp < 8)
			adjust_width = (width >> 1) << 1;
		else
			adjust_width = width;

		if (cur_cell_lines == 0){
			if (total_read_lines + max_lines_per_cell > height)
				max_lines_per_cell = height - total_read_lines;

			GxCore_Seek(pSrc,ret+height*delta-((total_read_lines+max_lines_per_cell)*delta),GX_SEEK_SET);
			GxCore_Read(pSrc, BMP_buffer,delta, max_lines_per_cell);
			cur_cell_lines = max_lines_per_cell;
			total_read_lines += cur_cell_lines;
		}
		if(is_reversed)
			ptr = &(data[(height - i - 1) * adjust_width * bpp / 8]);
		else
			ptr = &(data[i * adjust_width * bpp / 8]);
		memcpy((void *)ptr,
			(void *)(&(temp[(cur_cell_lines - 1) * delta])),
			adjust_width * bpp / 8);
		cur_cell_lines --;
	}
	desc->width = adjust_width;


#ifdef SWITCH_ELEB
	_16BPP_Add_Alpha(desc);
#endif

	if(check_cpu())
	{
	    _32BPP_Add_Alpha(desc, rmask, gmask, bmask);
	}

	if(24 == desc->bpp)
	{
		int hw_accel = 0;
#ifndef NO_OS
		GxHwMallocObj *tmp_vq = NULL;
#endif /*NO_OS*/
		char *tmp_data = NULL;

		hw_accel = desc->hw_accel;
		desc->bpp = 32;
		desc->size = desc->width * desc->height *desc->bpp / 8;
#ifndef NO_OS
		tmp_vq = hd_malloc_nocache(&tmp_vq, desc->width * desc->height * 3);
		if(NULL == tmp_vq) {
#else
			tmp_data = (char *)GxCore_Mallocz(desc->width * desc->height * 3);
		if(NULL == tmp_data) {
#endif /*NO_OS*/
			GUI_FREE(BMP_buffer);
			GxCore_Close(pSrc);
			gal_img_cleanup(desc);
			return (NULL);
		}
#ifndef NO_OS
		tmp_data = tmp_vq->usr_p;
#endif /*NO_OS*/
		memcpy(tmp_data, data, desc->width * desc->height * 3);
		gal_img_alloc_memory(desc);
		if(NULL == desc->data) {
			GUI_FREE(BMP_buffer);
			GxCore_Close(pSrc);
			gal_img_cleanup(desc);
			return (NULL);
		}
		for(i = 0; i < height; i++)
		{
			int j = 0;
			for(j = 0; j < width; j++)
			{
				((char *)(desc->data))[i * width * 4 + j * 4] = tmp_data[i * width * 3 + j * 3];
				((char *)(desc->data))[i * width * 4 + j * 4 + 1] = tmp_data[i * width * 3 + j * 3 + 1];
				((char *)(desc->data))[i * width * 4 + j * 4 + 2] = tmp_data[i * width * 3 + j * 3 + 2];
				((char *)(desc->data))[i * width * 4 + j * 4 + 3] = 0xFF;
			}
		}

		if (!hw_accel)
		{
			GUI_FREE(data);
		}
#ifndef NO_OS
		hd_free(tmp_vq);
		tmp_vq = NULL;
#else
		GUI_FREE(tmp_data);
#endif /*NO_OS*/
	}
	else if(16 == desc->bpp){
		desc->fmt = _get_color_format_by_mask(rmask, gmask, bmask);
	}
	else if((8 >= desc->bpp) && (8 != gui_core->config.bpp))
	{
		;//image_convert2current(desc);
	}


	GUI_FREE(BMP_buffer);

	GxCore_Close(pSrc);

	desc->effect = EFFECT_SRC_TO_DST;

	while(desc->data == NULL)
		gxlogd("here %s:%d\n", __func__, __LINE__);

	return desc;
}

