#ifndef NO_OS
#include <stdio.h>
#include <stdlib.h>
#include "gui_private.h"
#else /*NO_OS*/
#include "gdi_core.h"
#include "mini_gui_private.h"
#include "gal.h"
int gdi_get_charwidth(unsigned int c);
extern GuiSurface *screen;
#endif /*NO_OS*/
#include "hd_gal.h"

#include "gdi_font.h"
#ifdef GUI_TTF
#include "gal_ttf.h"
#endif

gdi_font_prop *gdi_check_char(unsigned short c)
{
	struct gdi_font *pFont = NULL;
	gdi_font_prop *pFontProp;

	pFont = gxgdi_get_font();
	if (NULL == pFont) {
		gxlogd("[GUI]Illegal Font\n");
		return (NULL);
	} else {
		if(TTF_FONT_TYPE == pFont->family)
		{
			return (NULL);
		}
		pFontProp = (gdi_font_prop *) (pFont->font_data);
		if(NULL == pFontProp)
		{
			return (NULL);
		}
		for (; pFontProp; pFontProp = (gdi_font_prop *) (pFontProp->pNext)) {
			if (((c >= pFontProp->First) && (c <= pFontProp->Last)))
				break;
		}
		return (pFontProp);
	}
}

int gdi_check_iso6937(unsigned short c, unsigned short accent)
{
	if((accent > 0xC0) && (accent <= 0xCF) &&
	   (c > 0) && (c < 0x80))
	{
		return (GUI_TRUE);
	}
	else
	{
		return (GUI_FALSE);
	}
}

int gdi_get_charwidth(unsigned int c);
static int _other_font_width(unsigned short c)
{
	struct gdi_font *pFont = NULL, *pOldfont = NULL;
	gdi_font_prop *pFontProp = NULL;
	gdi_charinfo *pCharInfo = NULL;
	int xsize = 0;

	pOldfont = gxgdi_get_font();

	pFont = gdi_get_first_font();

	while(pFont)
	{
		if((pFont != pOldfont) && (pFont->family == pOldfont->family))
		{
			gxgdi_set_font(pFont->font_name);

			pFontProp = gdi_check_char(c);
			if(NULL == pFontProp)
			{
			    gxgdi_set_font(pOldfont->font_name);
			    return (0);
			}
			pCharInfo = (gdi_charinfo *)(&(pFontProp->paCharInfo[c - (pFontProp->First)]));

			if(pCharInfo)
			{
				xsize = gdi_get_charwidth(c);
				break;
			}
		}
		pFont = pFont->next;
	}

	gxgdi_set_font(pOldfont->font_name);
	return (xsize);
}

int gdi_get_subscriptwidth(unsigned int c, int sub_index)
{
	gdi_font_prop *pFontProp = NULL;
	struct gdi_font *pFont = NULL;
	int width = 0;
#ifdef GUI_TTF
	int height = 0;
#endif

	if(0x20 > c)
	{
		return (0);
	}

	pFontProp = gdi_check_char(c);
	if (NULL == pFontProp) {
		pFont = gxgdi_get_font();
		if(TTF_FONT_TYPE == pFont->family)
		{
#ifdef GUI_TTF
			unsigned int unicode[2] = {0};
			unicode[0] = c;
			unicode[1] = 0;
			if(0x17D2 == c)
			    return (0);
			TTF_SizeSubScript((TTF_Font *)(pFont->font_data),
					unicode,
					&width,
					&height,
					sub_index);
#endif
			return (width);
		}
		else
		{
			return (_other_font_width(c));
		}
	} else {
		return (pFontProp->paCharInfo[c - (pFontProp->First)].XDist);
	}
}
int gdi_get_charwidth_byname(char *name)
{
	struct gdi_font *pFont = NULL;
	int width = 0;
#ifdef GUI_TTF
	int height = 0;
#endif

	pFont = gxgdi_get_font();

#ifdef GUI_TTF
	if(TTF_SizeNAME(pFont->font_data, name, (int *)&width, (int *)&height) < 0)
	{
		return (0);
	}
#endif

	return (width);
}

int gdi_get_charwidth(unsigned int c)
{
	gdi_font_prop *pFontProp = NULL;
	struct gdi_font *pFont = NULL;
	int width = 0;
#ifdef GUI_TTF
	int height = 0;
#endif

	if(0x20 > c)
	{
		return (0);
	}

	pFontProp = gdi_check_char(c);
	if (NULL == pFontProp) {
		pFont = gxgdi_get_font();
		if(TTF_FONT_TYPE == pFont->family)
		{
#ifdef GUI_TTF
			unsigned int unicode[2] = {0};
			unicode[0] = c;
			unicode[1] = 0;
			if(0x17D2 == c)
			    return (0);
			TTF_SizeUNICODE((TTF_Font *)(pFont->font_data),
					unicode,
					&width,
					&height);
#endif
			return (width);
		}
		else
		{
			return (_other_font_width(c));
		}
	} else {
		return (pFontProp->paCharInfo[c - (pFontProp->First)].XDist);
	}
}

int gdi_get_charheight(void)
{
	struct gdi_font *pfont;

	pfont = gxgdi_get_font();
	if(NULL == pfont)
	{
		return (0);
	}

	return (pfont->ysize);
}


int gdi_get_charwidth_with_accent(unsigned int c, unsigned int accent)
{
	gdi_font_prop *pFontProp0 = NULL, *pFontProp1 = NULL;
	int width = 0, accent_width = 0;
	struct gdi_font *pFont = NULL;
#ifdef GUI_TTF
	int height = 0;
	unsigned int unicode[2] = {0};
#endif

	if(0x20 > c)
	{
		return (0);
	}

	pFont = gxgdi_get_font();
	if(TTF_FONT_TYPE == pFont->family)
	{
#ifdef GUI_TTF
		unicode[0] = c;
		unicode[1] = 0;
		TTF_SizeUNICODE((TTF_Font *)(pFont->font_data),
				unicode,
				&width,
				&height);

		unicode[0] = accent;
		unicode[1] = 0;
		TTF_SizeUNICODE((TTF_Font *)(pFont->font_data),
				unicode,
				&accent_width,
				&height);
#endif
		return ((width > accent_width) ? (width) : (accent_width));
	}

	pFontProp0 = gdi_check_char(c);
	pFontProp1 = gdi_check_char(accent);
	if((NULL == pFontProp0) || (NULL == pFontProp1))
	{
		if(pFontProp0)
		{
			return (pFontProp0->paCharInfo[c - (pFontProp0->First)].XDist);
		}
		else if(pFontProp1)
		{
			return (pFontProp1->paCharInfo[accent - (pFontProp1->First)].XDist);
		}
		else
		{
			return (0);
		}
	}
	else
	{
		width = (((pFontProp0->paCharInfo[c - (pFontProp0->First)].XDist)) >
				(pFontProp1->paCharInfo[accent - (pFontProp1->First)].XDist)) ?
			((pFontProp0->paCharInfo[c - (pFontProp0->First)].XDist)) :
			(pFontProp1->paCharInfo[accent - (pFontProp1->First)].XDist);
		return (width);
	}
}

int gdi_draw_char_xpos(void* surface, int x, int y, int w, int h, unsigned int c, int color, int pos)
{
	int i = 0, j = 0;
	int diff = 0;
	int xsize = 0, ysize = 0;
	char *pData = NULL, *pPixel = NULL;
	gdi_font_prop *pFontProp;
	struct gdi_font *pFont = NULL;

	if(NULL == surface)
	{
		return (1);
	}

	pPixel = (char *)surface;

	pFont = gxgdi_get_font();

	if(0x20 > c)
	{
		return (0);
	}

	pFontProp = gdi_check_char(c);
	if(NULL == pFontProp)
	{
		return (1);
	}
	pData = (char *)(pFontProp->paCharInfo[c - (pFontProp->First)].pData);

	if (NULL == pData || NULL == pFont)
	{
		return (1);
	}
	else
	{
		xsize = gdi_get_charwidth(c);
		ysize = pFont->ysize;

		/*Draw Char in temporary buffer*/
		for(i = 0; i < ysize; i++)
		{
			for(j = 0; j < xsize; j++)
			{
				if ((*pData) & (0x80 >> diff)) {
					if(j >= pos)
					{
						if((x + j - pos) < w)
						{
							pPixel[(y + i) * w + x + j - pos] = color;
						}
					}
				}
				diff++;

				if (8 == diff) {
					pData++;
					diff = 0;
				}
			}
			if(diff)
			{
				pData++;
			}
			diff = 0;
		}
	}

	return (0);
}

int gdi_draw_char_ypos(void* surface, int x, int y, int w, int h, unsigned int c, int color, int pos)
{
	int byte_line = 0, diff = 0;
	int xsize = 0, ysize = 0, position = 0;
	char *pData = NULL, *pPixel = NULL;
	gdi_font_prop *pFontProp;
	struct gdi_font *pFont = NULL;

	if(NULL == surface)
	{
		return (1);
	}

	if(0x20 > c)
	{
		return (0);
	}

	pPixel = (char *)surface;
	pFont = gxgdi_get_font();

	pFontProp = gdi_check_char(c);
	if(NULL == pFontProp)
	{
		return (1);
	}

	pData = (char *)(pFontProp->paCharInfo[c - (pFontProp->First)].pData);

	if (NULL == pData || NULL == pFont)
	{
		return (1);
	}
	else
	{
		int i, j;
		xsize = gdi_get_charwidth(c);
		ysize = pFont->ysize;

		byte_line = pFontProp->paCharInfo[c - pFontProp->First].BytesPerLine;

		/*Draw Char in temporary buffer*/
		pData += byte_line * pos;

		for(i = pos; i < ysize; i++)
		{
			for(j = 0; j < xsize; j++)
			{
				if ((*pData) & (0x80 >> diff))
				{
					position = (i + y - pos) * w + x + j;
					if(position > (w * h))
					{
						return (0);
					}
					pPixel[position] = color;
				}
				diff++;

				if (8 == diff) {
					pData++;
					diff = 0;
				}
			}
			pData++;
			diff = 0;
		}
	}

	return (0);
}

int gdi_draw_char_with_accent(void *screen,
		int x,
		int y,
		unsigned int c,
		unsigned int accent,
		int color)
{
	gdi_font_prop *pFontProp0 = NULL, *pFontProp1 = NULL;
	gdi_charinfo * pCharInfo0 = NULL, *pCharInfo1 = NULL;
#ifdef GUI_TTF
	GuiCore *gui_core = GUI_GetCurrent();
	image_desc *pCharImage0 = NULL, *pCharImage1 = NULL;
#endif
	struct gdi_font *pFont = NULL;
	void *surface = NULL;
#ifdef GUI_TTF
	unsigned int unicode[2] = {0};
#endif
	int ret = 0,  byte_line0 = 0, byte_line1 = 0;

	surface = screen;
	if(NULL == surface)
	{
		gxlogd("[GUI]Illegal Surface\n");
		return (1);
	}
	else
	{
		char *pData0 = NULL, *pData1 = NULL, *pData = NULL, *pTr = NULL;
		int xsize0 = 0, ysize = 0, xsize1 = 0, xsize = 0, offset = 0;
		int max_byte = 0, mini_byte = 0, size = 0, i = 0, j = 0;

		pFont = gxgdi_get_font();
	//	bpp = gui_core->config.bpp;
		pFontProp0 = gdi_check_char((unsigned short)c);
		pFontProp1 = gdi_check_char((unsigned short)accent);
		if((NULL == pFontProp0) || (NULL == pFontProp1))
		{
			return (1);
		}

		if(c < 0x20)
		{
			return (0);
		}

		if(TTF_FONT_TYPE == pFont->family)
		{
#ifdef GUI_TTF
			unicode[0] = c;
			unicode[1] = 0;
			pCharImage0 = (image_desc *)ttf_draw_char(surface,
					(TTF_Font *)pFont->font_data,
					unicode,
					x,
					y,
					gui_core->config.bpp,
					color);
			unicode[0] = accent;
			unicode[1] = 0;
			pCharImage1 = (image_desc *)ttf_draw_char(surface,
					(TTF_Font *)pFont->font_data,
					unicode,
					x,
					y,
					gui_core->config.bpp,
					color);
			hd_img_desc(screen,
				    pCharImage0,
				    x + pCharImage0->xoff,
				    y + pCharImage0->yoff,
				    pCharImage0->width,
				    pCharImage0->height);
			hd_img_desc(screen,
				    pCharImage1,
				    x + pCharImage1->xoff,
				    y + pCharImage1->yoff,
				    pCharImage1->width,
				    pCharImage1->height);

			GUI_FREE(pCharImage0->data);
			GUI_FREE(pCharImage0);
			GUI_FREE(pCharImage1->data);
			GUI_FREE(pCharImage1);
			return (0);
#endif
		}
		else
		{
			pCharInfo0 = (gdi_charinfo *)(&(pFontProp0->paCharInfo[c - (pFontProp0->First)]));
			pCharInfo1 = (gdi_charinfo *)(&(pFontProp1->paCharInfo[accent- (pFontProp1->First)]));
		}

		pData0 = (char *)(pCharInfo0->pData);
		pData1 = (char *)(pCharInfo1->pData);

		if((NULL == pData0) || (NULL == pData1) || (NULL == pFont))
		{
			return (1);
		}
		else
		{
			xsize0 = gdi_get_charwidth(c);
			xsize1 = gdi_get_charwidth(accent);
			ysize = pFont->ysize;

			byte_line0 = pCharInfo0->BytesPerLine;
			byte_line1 = pCharInfo1->BytesPerLine;

			max_byte = (byte_line0 >= byte_line1) ? (byte_line0) : (byte_line1);
			mini_byte = (byte_line0 >= byte_line1) ? (byte_line1) : (byte_line0);
			size = max_byte * ysize;
			pData = (char *)GxCore_Malloc(size);
			if(NULL == pData)
			{
				return (1);
			}
			memset(pData, 0, size);

			/*Mix character c with the accent*/
			if(byte_line0 > byte_line1)
			{
				memcpy(pData, pData0, size);
				xsize = xsize0;
				offset = (xsize - xsize1) / 2;
				pTr = pData1;
			}
			else
			{
				memcpy(pData, pData1, size);
				xsize = xsize1;
				offset = (xsize - xsize0) / 2;
				pTr = pData0;
			}

			for(i = 0; i < ysize; i++)
			{
				for(j = 0; j < mini_byte; j++)
				{
					pData[i * max_byte + j] |= (pTr[i * mini_byte + j] >> offset);
				}
			}

			ret = hd_char(screen,
					pData,
					x,
					y,
					xsize,
					ysize,
					max_byte,
					color);

			GUI_FREE(pData);
		}
	}

	return (ret);
}

static int _change_other_font(void *screen, int x, int y, unsigned short c, int color)
{
	struct gdi_font *pFont = NULL, *pOldfont = NULL;
	gdi_font_prop *pFontProp = NULL;
	gdi_charinfo *pCharInfo = NULL;
	char *pData = NULL;
	int xsize = 0, ysize = 0, byte_line = 0, ret = 0;

	pOldfont = gxgdi_get_font();

	pFont = gdi_get_first_font();

	while(pFont)
	{
		if((pFont != pOldfont) && (pFont->family == pOldfont->family))
		{
			gxgdi_set_font(pFont->font_name);

			pFontProp = gdi_check_char(c);
			if(NULL == pFontProp)
			{
			    gxgdi_set_font(pOldfont->font_name);
			    return (0);
			}

			pCharInfo = (gdi_charinfo *)(&(pFontProp->paCharInfo[c - (pFontProp->First)]));

			if(pCharInfo)
			{
				pData = (char *)(pCharInfo->pData);

				xsize = gdi_get_charwidth(c);
				ysize = pFont->ysize;
				byte_line = pCharInfo->BytesPerLine;

				ret = hd_char(screen, pData, x, y, xsize, ysize, byte_line, color);
				break;
			}
		}
		pFont = pFont->next;
	}

	gxgdi_set_font(pOldfont->font_name);

	return (ret);
}

int gdi_draw_subscript(void *screen, int *x, int *y, unsigned int c, int color, int sub_index)
{
	gdi_font_prop *pFontProp;
	//unsigned char* pData = NULL;
	struct gdi_font *pFont = NULL;
	gdi_charinfo *pCharInfo = NULL;
#ifdef GUI_TTF
	GuiCore *gui_core = GUI_GetCurrent();
	image_desc *pCharImage = NULL;
#endif
	unsigned char *ptr = NULL;
	int byte_line = 0, ret = 0;
	unsigned int unicode[2] = {0};
	void *surface = screen;

	if (NULL == surface) {
		gxlogd("[GUI]Illegal SDL surface\n");
		return (1);
	} else {
		int xsize = 0, ysize = 0;
		char *pData = NULL;

		pFont = gxgdi_get_font();

		if(c < 0x20)
		{
			return (0);
		}

		if(TTF_FONT_TYPE == pFont->family)
		{
			unicode[0] = c;
			unicode[1] = 0;
#ifdef GUI_TTF
			pCharImage = ttf_draw_subscript(surface,
					(TTF_Font *)pFont->font_data,
					unicode,
					*x,
					*y,
					gui_core->config.bpp,
					color,
					sub_index);
			if(pCharImage)
			{
				hd_img_desc(surface,
						pCharImage,
						*x + pCharImage->xoff,
						*y + pCharImage->yoff,
						pCharImage->width,
						pCharImage->height);
				*x = *x + pCharImage->xoff;
				*y = *y + pCharImage->yoff;
				gal_img_release(pCharImage);
			}

			return (0);
#endif
		}
		else
		{
//			bpp = gui_core->config.bpp;
			pFontProp = gdi_check_char(c);

			if(NULL == pFontProp)
			{
				ret = _change_other_font(screen, *x, *y, c, color);
				return (ret);
			}

			pCharInfo = (gdi_charinfo *)(&(pFontProp->paCharInfo[c - (pFontProp->First)]));

			pData = (char *)(pCharInfo->pData);
		}

		if (NULL == pData || NULL == pFont) {
			return (1);
		} else {
			xsize = gdi_get_charwidth(c);
			ysize = pFont->ysize;

			byte_line = pCharInfo->BytesPerLine;

			ret = hd_char(screen, pData,
				      *x + pCharInfo->MinX,
				      *y + pCharInfo->MinY,
				      xsize,
				      ysize,
				      byte_line,
				      color);
			if((*x + pCharInfo->MinX) >= 0)
			    *x = *x + pCharInfo->MinX;
			if((*y + pCharInfo->MinY) >= 0)
			    *y = *y + pCharInfo->MinY;
		}

		if(TTF_FONT_TYPE == pFont->family)
		{
			ptr = (unsigned char *)pCharInfo->pData;
			GUI_FREE(ptr);
			GUI_FREE(pCharInfo);
		}
	}
	return (ret);
}

int gdi_draw_mark(void *screen, int x, int y, unsigned int c, int color)
{
	gdi_font_prop *pFontProp;
	//unsigned char* pData = NULL;
	struct gdi_font *pFont = NULL;
	gdi_charinfo *pCharInfo = NULL;
#ifdef GUI_TTF
	image_desc *pCharImage = NULL;
#endif
	unsigned char *ptr = NULL;
	int byte_line = 0, ret = 0;
	unsigned int unicode[2] = {0};
	void *surface = screen;

	if (NULL == surface) {
		gxlogd("[GUI]Illegal SDL surface\n");
		return (1);
	} else {
		int xsize = 0, ysize = 0;
		char *pData = NULL;

		pFont = gxgdi_get_font();

		if(c < 0x20)
		{
			return (0);
		}

		if(TTF_FONT_TYPE == pFont->family)
		{
			unicode[0] = c;
			unicode[1] = 0;
#ifdef GUI_TTF
			pCharImage = ttf_draw_char(surface,
					(TTF_Font *)pFont->font_data,
					unicode,
					x,
					y,
					gui.config.bpp,
					color);
			if(NULL == pCharImage)
			    return (0);
			hd_img_desc(surface,
				    pCharImage,
				    x,
				    y,
				    pCharImage->width,
				    pCharImage->height);
			gal_img_release(pCharImage);

			return (0);
#endif
		}
		else
		{
//			bpp = gui.config.bpp;
			pFontProp = gdi_check_char(c);

			if(NULL == pFontProp)
			{
				ret = _change_other_font(screen, x, y, c, color);
				return (ret);
			}

			pCharInfo = (gdi_charinfo *)(&(pFontProp->paCharInfo[c - (pFontProp->First)]));

			pData = (char *)(pCharInfo->pData);
		}

		if (NULL == pData || NULL == pFont) {
			return (1);
		} else {
			xsize = gdi_get_charwidth(c);
			ysize = pFont->ysize;

			byte_line = pCharInfo->BytesPerLine;

			ret = hd_char(screen, pData,
				      x,
				      y,
				      xsize,
				      ysize,
				      byte_line,
				      color);
		}

		if(TTF_FONT_TYPE == pFont->family)
		{
			ptr = (unsigned char *)pCharInfo->pData;
			GUI_FREE(ptr);
			GUI_FREE(pCharInfo);
		}
	}
	return (ret);
}

int gdi_draw_char(void *screen, int *x, int *y, unsigned int c, int color)
{
	gdi_font_prop *pFontProp;
	//unsigned char* pData = NULL;
	struct gdi_font *pFont = NULL;
	gdi_charinfo *pCharInfo = NULL;
#ifdef GUI_TTF
	GuiCore *gui_core = GUI_GetCurrent();
	image_desc *pCharImage = NULL;
#endif
	unsigned char *ptr = NULL;
	int byte_line = 0, ret = 0;
	unsigned int unicode[2] = {0};
	void *surface = screen;

	if (NULL == surface) {
		gxlogd("[GUI]Illegal SDL surface\n");
		return (1);
	} else {
		int xsize = 0, ysize = 0;
		char *pData = NULL;

		pFont = gxgdi_get_font();

		if(c < 0x20)
		{
			return (0);
		}

		if(TTF_FONT_TYPE == pFont->family)
		{
			unicode[0] = c;
			unicode[1] = 0;
#ifdef GUI_TTF
			pCharImage = ttf_draw_char(surface,
					(TTF_Font *)pFont->font_data,
					unicode,
					*x,
					*y,
					gui_core->config.bpp,
					color);
			if(NULL == pCharImage)
			    return (0);
			if((*y + pCharImage->yoff + pCharImage->height) > ((GuiSurface*)surface)->sf.height)
			    *y = ((GuiSurface *)surface)->sf.height - pCharImage->height - pCharImage->yoff;
			if((*x + pCharImage->xoff) < 0)
				*x = -pCharImage->xoff;
			hd_img_desc(surface,
				    pCharImage,
				    *x + pCharImage->xoff,
				    *y + pCharImage->yoff,
				    pCharImage->width,
				    pCharImage->height);
			*x = *x + pCharImage->xoff;
			*y = *y + pCharImage->yoff;
			gal_img_release(pCharImage);

			return (0);
#endif
		}
		else
		{
//			bpp = gui_core->config.bpp;
			pFontProp = gdi_check_char(c);

			if(NULL == pFontProp)
			{
				ret = _change_other_font(screen, *x, *y, c, color);
				return (ret);
			}

			pCharInfo = (gdi_charinfo *)(&(pFontProp->paCharInfo[c - (pFontProp->First)]));

			pData = (char *)(pCharInfo->pData);
		}

		if (NULL == pData || NULL == pFont) {
			return (1);
		} else {
			xsize = gdi_get_charwidth(c);
			ysize = pFont->ysize;

			byte_line = pCharInfo->BytesPerLine;

			if((*y + pCharInfo->MinY + ysize) > ((GuiSurface *)surface)->sf.height)
			    *y = ((GuiSurface *)surface)->sf.height - ysize - pCharInfo->MinY;
			ret = hd_char(screen, pData,
				      *x + pCharInfo->MinX,
				      *y + pCharInfo->MinY,
				      xsize,
				      ysize,
				      byte_line,
				      color);
			if((*x + pCharInfo->MinX) >= 0)
			    *x = *x + pCharInfo->MinX;
			if((*y + pCharInfo->MinY) >= 0)
			    *y = *y + pCharInfo->MinY;
		}

		if(TTF_FONT_TYPE == pFont->family)
		{
			ptr = (unsigned char *)pCharInfo->pData;
			GUI_FREE(ptr);
			GUI_FREE(pCharInfo);
		}
	}
	return (ret);
}

int gdi_draw_char_byname(void *screen, int *x, int *y, char *name, int color)
{
	struct gdi_font *pFont = NULL;
#ifdef GUI_TTF
	GuiCore *gui_core = GUI_GetCurrent();
	image_desc *pCharImage = NULL;
#endif
	void *surface = screen;

	if (NULL == surface) {
		gxlogd("[GUI]Illegal SDL surface\n");
		return (1);
	}
	if(NULL == name)
		return (1);

	pFont = gxgdi_get_font();
	if(TTF_FONT_TYPE != pFont->family)
		return(1);

#ifdef GUI_TTF
	pCharImage = ttf_draw_char_byname(surface,
			(TTF_Font *)pFont->font_data,
			name,
			*x,
			*y,
			gui_core->config.bpp,
			color);
	if(NULL == pCharImage)
		return (0);

	hd_img_desc(surface,
			pCharImage,
			*x + pCharImage->xoff,
			*y + pCharImage->yoff,
			pCharImage->width,
			pCharImage->height);
	*x = *x + pCharImage->xoff;
	*y = *y + pCharImage->yoff;
	gal_img_release(pCharImage);
#endif
	return (0);

}

int gdi_draw_sub_byname(void *screen, int *x, int *y, char *name, int color)
{
	struct gdi_font *pFont = NULL;
#ifdef GUI_TTF
	image_desc *pCharImage = NULL;
#endif
	void *surface = screen;

	if (NULL == surface) {
		gxlogd("[GUI]Illegal SDL surface\n");
		return (1);
	}
	if(NULL == name)
		return (1);

	pFont = gxgdi_get_font();
	if(TTF_FONT_TYPE != pFont->family)
		return(1);

#ifdef GUI_TTF
	pCharImage = ttf_draw_sub_byname(surface,
			(TTF_Font *)pFont->font_data,
			name,
			*x,
			*y,
			gui.config.bpp,
			color);
	if(NULL == pCharImage)
		return (0);

	hd_img_desc(surface,
			pCharImage,
			*x + pCharImage->xoff,
			*y + pCharImage->yoff,
			pCharImage->width,
			pCharImage->height);
	*x = *x + pCharImage->xoff;
	*y = *y + pCharImage->yoff;
	gal_img_release(pCharImage);
#endif
	return (0);

}

int gdi_draw_matrix(void *data, void *rect, int byteperline, int color)
{
	int ret = 0;
#ifndef NO_OS
	GAL_Rect *font_rect = NULL;

	if((NULL == data) || (NULL == rect))
	{
		return (1);
	}

	font_rect = (GAL_Rect *)rect;

	ret = hd_char(screen,
			data,
			font_rect->x,
			font_rect->y,
			font_rect->w,
			font_rect->h,
			byteperline,
			color);
#endif

	return (ret);
}


