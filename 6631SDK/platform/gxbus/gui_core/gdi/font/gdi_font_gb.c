#ifndef NO_OS
#include <stdio.h>
#include <stdlib.h>
#include "gui.h"
#else /*NO_OS*/
#include "mini_gui_private.h"
#include "hd_gal.h"
#include "gal.h"
#include "gal_blit.h"

#endif /*NO_OS*/

#include "gdi_char.h"
#include "gdi_font.h"
//#include "IMG.h"

static unsigned short merge_byte(unsigned char Byte0, unsigned char Byte1)
{
	return Byte1 | (((unsigned short)Byte0) << 8);
}

static int _check_english_alpha(unsigned short c);
static int _word_length(char **pStr);
static int font_gb_len(void *string)
{
	int len = 0;
	unsigned char c;
	char *pStr = NULL;

	if (NULL == string) {
		return (0);
	} else {
		pStr = (char *)string;

		while ((*pStr != '\0') && (*pStr != '\n')) {
			c = *pStr++;

			if ((c > 0x7F) && (c < 0xFF)) {
				unsigned char c1 = 0;

				c1 = *pStr++;
				len += gdi_get_charwidth(merge_byte(c, c1));
			} else {
				#if 0
				if(_check_english_alpha(c))
				{
					char *pcur = pStr;
					pcur--;
					len += _word_length(&pcur);
					pStr = pcur;
				}
				else
				{
					len += gdi_get_charwidth(c);
				}
				#endif
				len += gdi_get_charwidth(c);
			}
		}
	}
	return (len);
}

#ifndef NO_OS
#ifdef GUI_ROLL_STR
static int font_gb_lines(void *string, GAL_Rect *rect, GAL_Interval *interval)
{
	int lines = 0, text_width = 0;
	char *pStr = NULL;
	unsigned char c = 0;
	unsigned short wc = 0, horizontal = 0;

	if((NULL == string) || (NULL == rect) || (NULL == interval))
	{
		return (0);
	}

	pStr = (char *)string;
	lines = 1;
	horizontal = interval->horizontal;
	while(*pStr != '\0')
	{
		c = *pStr++;
		if ((c > 0x7F) && (c < 0xFF))
		{
			unsigned char c1 = 0;

			c1 = *pStr++;
			wc = merge_byte(c, c1);
		}
		else
		{
			wc = c;
		}

		if(('\n' == wc))
		{
			lines++;
			text_width = 0;
			continue;
		}

		text_width += (gdi_get_charwidth(wc) + horizontal);
		if((text_width > (rect->w)) && ('\0' != *pStr))
		{
			if((c > 0x7F) && (c < 0xFF))
			{
				pStr--;
			}
			else
			{
				pStr -= 2;
			}
			lines++;
			text_width = 0;

			continue;
		}
	}

	return (lines);
}
#endif /*GUI_ROLL_STR*/
#endif /*NO_OS*/

static int _check_english_alpha(unsigned short c)
{
    if(((c >= 'A') && (c <= 'Z')) ||
       ((c >= 'a') && (c <= 'z')))
	return (1);
    else
	return (0);
}

static int _word_length(char **pStr)
{
    char *ptr = NULL;
    unsigned char c = 0;
    unsigned short wc = 0;
    unsigned int word_len = 0;

    ptr = *pStr;
    while(*ptr)
    {
	c = *ptr++;
	if ((c > 0x7F) && (c < 0xFF)) {
		unsigned char c1 = 0;

		c1 = *ptr++;
		wc = merge_byte(c, c1);
	} else {
		wc = c;
	}

	if((!_check_english_alpha(wc)) || (' ' == wc))
	{
	    if(' ' == wc)
	    {
		word_len += gdi_get_charwidth(wc);
	    }
	    else if(wc > 0xFF)
	    {
		ptr -= 2;
	    }
	    else/*('\n' == wc)*/
	    {
		ptr--;
	    }

	    break;
	}

	word_len += gdi_get_charwidth(wc);
    }

    *pStr = ptr;

    return (word_len);
}

static int _check_line_full(const char *string, int width, unsigned short horizontal)
{
	int text_width = 0;
	char *pstr = NULL;
	unsigned char c = 0;
	unsigned short wc = 0;

	if(NULL == string)
	{
		return (0);
	}

	pstr = (char *)string;
	while(*pstr)
	{
		c = *pstr++;
		if ((c > 0x7F) && (c < 0xFF)) {
			unsigned char c1 = 0;

			c1 = *pstr++;
			wc = merge_byte(c, c1);
		} else {
			wc = c;
		}

		if(_check_english_alpha(wc))
		{
			char *pcur = pstr;
			pcur--;
			text_width += _word_length(&pcur);
			if(text_width > width)
			{
				pcur = pstr;
				pcur--;
				text_width -= _word_length(&pcur);
				break;
			}
			else
			{
				pstr = pcur;
			}
		}
		else
		{
			text_width += (gdi_get_charwidth(wc) + horizontal);
		}
		if(text_width >= width)
		{
			break;
		}
		else if('\n' == wc)
		{
			break;
		}
		else if('\0' == wc)
		{
			break;
		}
	}

	if(text_width >= width)
	{
		return (0);
	}

	return (width - text_width);

}

static unsigned short _get_control_code(unsigned char **string)
{
	unsigned char *pstr = NULL;
	unsigned short code = 0;

	if(NULL == string)
	{
		return (0);
	}

	pstr = *string;
	if(NULL == pstr)
	{
		return (0);
	}

	if(0x10 == *pstr)
	{
		pstr++;
		code = (*pstr << 8) | (*(pstr + 1));
		pstr += 2;
	}
	if((*pstr > 0) && (*pstr < 0x20))
	{
		if((0xA != *pstr) && (0xD != *pstr))
		{
			code = *pstr;
			pstr++;
		}
	}

	*string = pstr;
	return (code);
}

static int _draw_word(void *surface, char **pStr, int *xPos, int *yPos, int color, int total_width)
{
    char *ptr = NULL;
    unsigned char c = 0;
    unsigned short wc = 0;
    unsigned int word_len = 0;

    ptr = *pStr;
    while(*ptr)
    {
	c = *ptr++;
	if ((c > 0x7F) && (c < 0xFF)) {
		unsigned char c1 = 0;

		c1 = *ptr++;
		wc = merge_byte(c, c1);
	} else {
		wc = c;
	}

	if((_check_english_alpha(wc)) || (' ' == wc))
	{
		word_len += gdi_get_charwidth(wc);
		if(word_len > total_width)
		{
			ptr--;
			break;
		}
		gdi_draw_char(surface, xPos, yPos, (unsigned short)wc, color);
		*xPos += gdi_get_charwidth(wc);

	}

	if((!_check_english_alpha(wc)) || (' ' == wc))
	{
	    if('\n' == wc)
	    {
		ptr--;
	    }
	    else if((wc <= 0xFF) && (wc != ' '))
	    {
		ptr--;
	    }
	    else if(wc > 0xFF)
	    {
		ptr -= 2;
	    }
	    break;
	}
    }

    *pStr = ptr;

    return (word_len);
}

static int font_gb_text(void *screen, void *string, GAL_Rect *rect, GAL_Interval *interval, int alignment,  int color, int flag)
{
	char *pStr = NULL;
	gdi_font_prop *pFontProp;
	int text_width = 0;
	unsigned char c = 0;
	struct gdi_font *pFont;
	int xPos, yPos;
	int text_off = 0;
	int x = 0, y = 0, width = 0, height = 0;
	unsigned short wc = 0, horizontal = 0, vertical = 0;
	void *surface = screen;

	if (NULL == surface) {
		gxlogd("[GUI]Illegal SDL surface\n");
		return (1);
	} else {
		if (NULL == string) {
			return (-1);
		}

		if(NULL == rect)
		{
			return (-1);
		}

		if(NULL == interval)
		{
			return (-1);
		}

		pStr = (char *)string;
		x = rect->x;
		y = rect->y;
		width = rect->w;
		height = rect->h;

		_get_control_code((unsigned char **)&pStr);

		horizontal = interval->horizontal;
		vertical = interval->vertical;

		if(PARAGRAPH_MODE == flag)
		{
			if(GUI_TA_HCENTRE & alignment)
			{
				text_off = _check_line_full(pStr, width, horizontal);
				xPos = text_off / 2 + x;
			}
			else if(GUI_TA_RIGHT & alignment)
			{
				text_off = _check_line_full(pStr, width, horizontal);
				xPos = text_off + x;
			}
			else
			{
				xPos = x;
			}
		}
		else
		{
			xPos = x;
		}
		yPos = y;

		while (*pStr != '\0') {
			c = *pStr++;
			if ((c > 0x7F) && (c < 0xFF)) {
				unsigned char c1 = 0;

				c1 = *pStr++;

				if(c1 == '\0')
					break;

				wc = merge_byte(c, c1);
			} else {
				wc = c;

				if ((PARAGRAPH_MODE) == flag && ('\n' == wc)) {
					pFont = gxgdi_get_font();
					if (NULL == pFont) {
						return (-1);
					} else {
						yPos += (pFont->ysize + vertical);
						height -= pFont->ysize;

						if ((yPos + pFont->ysize + vertical) > (rect->y + rect->h) ) {
							return (-1);
						} else {
							if((_check_english_alpha(wc)) && (PARAGRAPH_MODE == flag))
							{
							    char *pcur = pStr;
							    pcur--;
							    text_width = _word_length(&pcur);
							}
							else
							{
							    text_width = gdi_get_charwidth(wc);
							}

							if(GUI_TA_HCENTRE & alignment)
							{
								text_off = _check_line_full(pStr, width, horizontal);
								xPos = text_off / 2 + x;
							}
							else if(GUI_TA_RIGHT & alignment)
							{
								text_off = _check_line_full(pStr, width, horizontal);
								xPos = text_off + x;
							}
							else
							{
								xPos = x;
							}
						}
					}
				}
				else if('\n' == wc)
				{
					return (pStr - (char *)string);
				}
			}

			if((_check_english_alpha(wc)) && (PARAGRAPH_MODE == flag))
			{
			    char *pcur = pStr;
			    pcur--;
			    text_width += _word_length(&pcur);
			}
			else
			{
			    text_width += gdi_get_charwidth(wc);
			}

			pFontProp = gdi_check_char(wc);
			if (NULL != pFontProp) {

				if (text_width > width) {
					if (PARAGRAPH_MODE == flag) {
						int line_full = 0;
						pFont = gxgdi_get_font();

						if (NULL == pFont) {
							return (1);
						} else {
							if(xPos != rect->x)
							    yPos += (pFont->ysize + vertical);
							height -= pFont->ysize;
							if ((yPos + pFont->ysize + vertical) > (rect->y + rect->h) ) {
								return (pStr - (char *)string - 1);
							} else {
								if((_check_english_alpha(wc)) && (PARAGRAPH_MODE == flag))
								{
								    char *pcur = pStr;
								    pcur--;
								    text_width = _word_length(&pcur);
								}
								else
								{
								    text_width = gdi_get_charwidth(wc);
								}
								if ((wc > 0x7F) && (wc < 0xFF))
								{
									pStr--;
								}
								else
								{
									pStr -= 2;
								}

								if('\n' == *pStr)
								{
									pStr++;
									line_full = GUI_TRUE;
								}

								if(GUI_TA_HCENTRE & alignment)
								{
									text_off = _check_line_full(pStr, width, horizontal);
									xPos = text_off / 2 + x;
								}
								else if(GUI_TA_RIGHT & alignment)
								{
									text_off = _check_line_full(pStr, width, horizontal);
									xPos = text_off + x;
								}
								else
								{
									xPos = x;
								}

								if((line_full) && ('\n' == *(pStr - 1)))
								{
									pStr--;
								}
								if ((wc > 0x7F) && (wc < 0xFF))
								{
									pStr++;
								}
								else
								{
									pStr += 2;
								}
								line_full = GUI_FALSE;
							}
						}
					} else {
						return (pStr - (char *)string - 1);
					}
				}

				if((_check_english_alpha(wc)) && (PARAGRAPH_MODE == flag))
				{
				    pStr--;
				    _draw_word(surface, &pStr, &xPos, &yPos, color, x + width);
				}
				else
				{
				    gdi_draw_char(surface, &xPos, &yPos, (unsigned short)wc, color);
				    xPos += (gdi_get_charwidth(wc) + horizontal);
				}
			}
		}
	}

	return (pStr - (char *)string);
}

#ifndef NO_OS
#ifdef GUI_ROLL_STR
static int _word_cut_by_width(char **pStr, int width)
{
	char *ptr = NULL;
	unsigned char c = 0;
	unsigned short wc = 0;
	unsigned int word_len = 0;
	int offset = 0;

	ptr = *pStr;
	while(*ptr)
	{
		c = *ptr++;
		offset += 1;
		if ((c > 0x7F) && (c < 0xFF)) {
			unsigned char c1 = 0;

			c1 = *ptr++;
			offset += 1;
			wc = merge_byte(c, c1);
		} else {
			wc = c;
		}

		if((!_check_english_alpha(wc)) || (' ' == wc))
		{
			if(' ' == wc)
			{
				word_len += gdi_get_charwidth(wc);
			}
			else if(wc > 0xFF)
			{
				ptr -= 2;
				offset -= 2;
			}
			else/*('\n' == wc)*/
			{
				ptr--;
				offset -= 1;
			}

			break;
		}

		word_len += gdi_get_charwidth(wc);
		if(word_len > width)
		{
			ptr--;
			offset -= 1;
			break;
		}

	}

	*pStr = ptr;

	return (offset);
}

static int font_gb_skip_line(void *p_start, void *string, GAL_Interval *interval, int line_width, int alignment)
{
	int width = 0, offset = 0, line = 0, char_width = 0;
	unsigned char * p_str = NULL, *p_head = NULL, *ptr = NULL, *p_cursor = NULL;
	unsigned short wc = 0, horizontal = 0;

	if(NULL == string || NULL == p_start || NULL == interval)
	{
		return (0);
	}

	p_str = (unsigned char *)string;
	p_head = p_start;
	horizontal = interval->horizontal;
	if(line_width < 0)
	{
		width = -line_width;
	}
	else
	{
		width = line_width;
	}

	line = 0;
	if(line_width > 0)
	{
		do
		{

			if('\0' == *(p_str))
			{
				return (offset);
			}
			if((*(p_str) > 0x7F) && (*(p_str) < 0xFF))
			{
				wc = merge_byte(*(p_str ), *(p_str + 1));

				char_width = gdi_get_charwidth(wc) + horizontal;
				line += char_width;

				if(line > width)
				{
					break;
				}

				p_str += 2;
				offset += 2;
			}
			else
			{
				wc = *p_str;

				if(_check_english_alpha(wc))
				{
					p_cursor = p_str;
					char_width = _word_length((char**)&p_cursor) + horizontal;
					if((line + char_width) > width)
					{
						if(line == 0)
						{
							p_cursor = p_str;
							offset += _word_cut_by_width((char**)&p_cursor, width);
						}
						break;
					}
					line += char_width;
					offset += (p_cursor - p_str);
					p_str = p_cursor;
				}
				else
				{
					char_width = gdi_get_charwidth(wc) + horizontal;
					line += char_width;
					p_str++;
					offset++;
				}

				if(line > width)
				{
					if(wc > 0x80)
					{
						offset -= 2;
					}
					else
					{
						offset--;
					}
					break;
				}

				if('\n' == wc)
				{
					break;
				}
			}

			char_width = 0;
		}while(line < width);
	}
	else if(p_head <= p_str)
	{
		do{
			if(*(p_str - 1) > 0x80)
			{
				wc = merge_byte(*(p_str - 2), *(p_str - 1));

				char_width = gdi_get_charwidth(wc) + horizontal;
				line += char_width;

				if(line > width)
				{
					break;
				}

				p_str -= 2;
				offset += 2;
			}
			else
			{
				wc = *(p_str - 1);

				char_width = gdi_get_charwidth(wc) + horizontal;
				line += char_width;

				if(line > width)
				{
					break;
				}

				p_str--;
				offset++;
				if(('\n' == wc) && ('\n' == *(p_str - 2)))
				{
					break;
				}
				else if('\n' == wc)
				{
					ptr = p_str;

					while(('\n' != *(ptr - 1)) && (p_head != ptr))
					{
						if(0x80 < *(ptr - 1))
						{
							ptr -= 2;
						}
						else
						{
							ptr--;
						}
					}

					p_cursor = ptr;
					char_width = 0;

					while(*ptr != *p_str)
					{
						if(char_width > width)
						{
							p_cursor = ptr;
							if(0x80 < *p_cursor)
							{
								p_cursor -= 2;
							}
							else
							{
								p_cursor--;
							}

							ptr = p_cursor;
							char_width = 0;
						}

						if(0x80 < *ptr)
						{
							wc = merge_byte(*ptr, *(ptr + 1));
							char_width += (gdi_get_charwidth(wc) + horizontal);

							ptr += 2;
						}
						else
						{
							wc = *ptr;
							char_width += (gdi_get_charwidth(wc) + horizontal);

							ptr++;
						}
					}

					p_str = (unsigned char *)string;
					offset = p_str - p_cursor;

					return (offset);
				}
			}
		}while(line < width);
	}

	return (offset);
}
#endif /*GUI_ROLL_STR*/
#endif /*NO_OS*/

#if 0
static int _font_gb_text_surface(void *screen, void *string, void *buffer, GAL_Rect *rect, int pos, int color)
{
	int ret = 0, char_size = 0, i = 0, offset = 0;
	int bpp = 0, back_color = 0, fore_color = 0;
	int xpos = 0, ypos = 0;
	char *pstr = NULL;
	unsigned char c = 0;
	unsigned short wc = 0;

	if((NULL == string) || (NULL == buffer))
	{
		return (1);
	}

	bpp = gui.config.bpp;

	pstr = (char *)string;
	fore_color = color;
	back_color = gal_color2index(screen, gui.config.gui_trans);
	offset = pos;

	switch(bpp)
	{
		case 8:
			memset(buffer, back_color, rect->w * rect->h * gui.config.bpp / 8);

			while (*pstr != '\0')
			{
				c = *pstr++;
				i++;
				if ((c > 0x7F) && (c < 0xFF))
				{
					unsigned char c1 = 0;

					c1 = *pstr++;
					i++;
					wc = merge_byte(c, c1);
				}
				else
				{
					wc = c;
				}

				char_size += gdi_get_charwidth(wc);
				if(char_size >= pos)
				{
					pstr -= i;
					offset -= char_size;
					break;
				}
				i = 0;
			}

			offset += gdi_get_charwidth(wc);
			i = 0;
			while (*pstr != '\0')
			{
				c = *pstr++;
				if ((c > 0x7F) && (c < 0xFF))
				{
					unsigned char c1 = 0;

					c1 = *pstr++;
					wc = merge_byte(c, c1);
				}
				else
				{
					wc = c;
				}

				if(rect->w < xpos)
				{
					break;
				}

				i ++;
				if(1 == i)
				{
					gdi_draw_char_xpos(buffer, xpos, ypos, rect->w, rect->h, wc, color, offset);
					xpos += (gdi_get_charwidth(wc) - offset);
				}
				else
				{
					gdi_draw_char_xpos(buffer, xpos, ypos, rect->w, rect->h, wc, color, 0);
					xpos += gdi_get_charwidth(wc);
				}

			}

			break;
		case 16:
			break;
		case 24:
			break;
		case 32:
			break;
		default:
			break;
	}

	return (ret);
}

static int _font_gb_paragraph_surface(void *screen, void *string, void *buffer, GAL_Rect *rect, int pos, int color)
{
	GuiCore *gui_core = GUI_GetCurrent();
	int ret = 0, char_size = 0, offset = 0, skip = 0, line_num = 0;
	int bpp = 0, back_color = 0, fore_color = 0, x_pos = 0, y_pos = 0;
	unsigned char c = 0;
	unsigned short wc = 0;
	struct gdi_font* font = NULL;
	char *pstr = NULL;

	if((NULL == string) || (NULL == buffer))
	{
		return (1);
	}

	bpp = gui_core->config.bpp;
	fore_color = color;
	back_color = gal_color2index(screen, gui.config.gui_trans);

	pstr = (char *)string;
	font = gxgdi_get_font();
	if(NULL == font)
	{
		return (1);
	}
	char_size = font->ysize;

	switch(bpp)
	{
		case 8:
			memset(buffer, back_color, rect->w * rect->h * gui_core->config.bpp / 8);

			offset = pos % char_size;
			skip = pos / char_size;

			while (*pstr != '\0')
			{
				c = *pstr++;
				if ((c > 0x7F) && (c < 0xFF))
				{
					unsigned char c1 = 0;

					c1 = *pstr++;
					wc = merge_byte(c, c1);
				}
				else
				{
					wc = c;
				}

				if(rect->w < (x_pos + gdi_get_charwidth(wc)))
				{
					x_pos = 0;
					line_num++;
					if(line_num > skip + 1)
					{
						y_pos += char_size;
					}
				}

				if(line_num > skip)
				{
					if(y_pos >= rect->h)
					{
						return (ret);
					}
					gdi_draw_char_ypos(buffer, x_pos, y_pos, rect->w, rect->h, wc, color, 0);
				}
				else if(line_num == skip)
				{
					y_pos = 0;
					gdi_draw_char_ypos(buffer, x_pos, y_pos, rect->w, rect->h, wc, color, offset);
					y_pos = char_size - offset;
				}

				x_pos += gdi_get_charwidth(wc);
			}

			break;
		case 16:
			break;
		case 24:
			break;
		case 32:
			break;
		default:
			break;
	}


	return (ret);
}
#endif

#ifndef NO_OS
#ifdef GUI_ROLL_STR
static int font_gb_roll_text(void *screen, void *string, GAL_Rect* rect, int pos, int color, int flag)
{
	GuiCore *gui_core = GUI_GetCurrent();;
	void *surface = NULL;
	unsigned char *pstr = NULL;
	unsigned int line = 0, fore_color = 0;
	GAL_Interval interval = {0};
	GAL_Rect src_rect = {0}, dst_rect = {0};
	struct gdi_font *pFont = NULL;

	pstr = (unsigned char *)string;

	src_rect = *rect;
	src_rect.x = src_rect.y = 0;

	pFont = gxgdi_get_font();

	if(SINGLELINE_MODE == flag)
	{
		line = src_rect.w = font_gb_len(pstr) + pFont->xsize * 3;
	}
	else
	{
		src_rect.h += gdi_get_charheight();
	}

	src_rect.w = ((((src_rect.w * gui_core->config.bpp) + 31) >> 5) << 2) / (gui_core->config.bpp / 8);
	surface = gal_get_surface(&src_rect, gui_core->config.bpp);
	src_rect.w = line;

	fore_color = gui.config.gui_trans;
	fore_color = gal_color2index(screen, fore_color);
	gdi_begin();
	if(gui.config.enable_double_buffer)
	{
		dst_rect = *rect;
		src_rect.w = rect->w;
		src_rect.x += pos;
		if((src_rect.x + src_rect.w) > line)
		{
			dst_rect.w = src_rect.w = line - src_rect.x;
		}
		gal_copy_surface(screen, &dst_rect, surface, &src_rect);
		src_rect.x = 0;
		src_rect.w = line;
	}
	else
	{
		hd_fillrect(surface, &src_rect, fore_color);
	}

	font_gb_text(surface,
			pstr,
			&src_rect,
			&interval,
			GUI_TA_LEFT | GUI_TA_VCENTRE,
			color,
			flag);
	gdi_end();

	if((SINGLELINE_MODE == flag) && (pos >= 0))
	{
		src_rect.x += pos;
		src_rect.w = rect->w;

		if((src_rect.x + src_rect.w) > line)
		{
			src_rect.w = line - src_rect.x;
		}
	}
	else if((PARAGRAPH_MODE == flag) && (pos >= 0))
	{
		src_rect.y += (pos % gdi_get_charheight());
	}

	dst_rect = *rect;
	if(pos < 0)
	{
		dst_rect.x = dst_rect.x - pos;
		src_rect.w = dst_rect.w = dst_rect.w + pos;
	}

	gal_stretch_surface(surface, &src_rect, screen, &dst_rect);

	gdi_commit();

	gal_free_surface(surface);

	return (0);
}
#endif /*GUI_ROLL_STR*/
#endif /*NO_OS*/

#ifndef NO_OS
struct gdi_font_opt font_gb_opt = {
	"gb",
	font_gb_len,
	font_gb_text,
#ifdef GUI_ROLL_STR
	font_gb_skip_line,
	font_gb_roll_text,
	font_gb_lines
#else
	NULL,
	NULL,
	NULL
#endif /*GUI_ROLL_STR*/
};
#else
struct gdi_font_opt font_gb_opt = {
	"gb",
	font_gb_len,
	font_gb_text,
	NULL,
	NULL,
	NULL
};
#endif /*NO_OS*/

