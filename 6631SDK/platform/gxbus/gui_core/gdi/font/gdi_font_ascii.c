#include "gdi_font.h"
#include "gdi_char.h"
//#include "IMG.h"
#ifndef NO_OS
#include <stdlib.h>
#endif /*NO_OS*/
#include <stdio.h>
#include <string.h>

static int _check_first_byte(char **pstr)
{
	char *str = NULL;
	int first_code = 0;

	if(NULL == pstr)
	{
		return (0);
	}

	str = *pstr;
	if(NULL == str)
	{
		return (0);
	}

	if((*str > 0) && (*str < 0x5))
	{
		first_code = *str;
		first_code += 4;
		str++;
	}
	else if(0x10 == *str)
	{
		str++;
		first_code = ((*str) << 8) | (*(str + 1));
		str += 2;
	}
	else if(0x11 == *str)
	{
		first_code = *str;
		str++;
	}
	else
	{
		first_code = 0;
	}

	*pstr = str;

	return (first_code);

}

static int font_ascii_len(void *string)
{
	int len, str_leng, i;
	char *p = NULL;

	if (NULL == string) {
		return (0);
	}

	p = (char *)string;

	_check_first_byte(&p);

	str_leng = strlen(p);

	i = 0;
	len = 0;

	while (i < str_leng) {
		len += gdi_get_charwidth(p[i]);
		i++;
	}

	return (len);
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
		wc = c;

		text_width += (gdi_get_charwidth(wc) + horizontal);
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

static int font_ascii_text(void *screen, void *string, GAL_Rect *rect, GAL_Interval *interval, int alignment, int color, int flag)
{
	char *pStr = NULL;
	gdi_font_prop *pFontProp;
	int text_width = 0;
	int x = 0, y = 0, width = 0, height = 0;
	unsigned short c = 0;
	struct gdi_font *pFont;
	int xPos, yPos;
	int text_off = 0, class_code = 0;
	unsigned short horizontal = 0, vertical = 0;
	void *surface =  screen;

	if (NULL == surface) {
		gxlogd("[GUI]Illegal SDL surface\n");
		return (1);
	} else {
		if (NULL == string) {
			return (1);
		}

		if(NULL == rect)
		{
			return (1);
		}

		if(NULL == interval)
		{
			return (1);
		}

		x = rect->x;
		y = rect->y;
		width = rect->w;
		height = rect->h;

		yPos = y;
		pStr = (char *)string;

		class_code = _check_first_byte(&pStr);
		/*ISO 8859 standard*/

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

		horizontal = interval->horizontal;
		vertical = interval->vertical;

		while (*pStr != '\0') {
			c = (*pStr) & 0x00FF;
			c = (class_code << 8) | c;
			text_width += (gdi_get_charwidth(c) + horizontal);
			pFontProp = gdi_check_char(c);
			if (NULL == pFontProp) {
				pStr++;
				continue;
			}
			if ((text_width > width) || ('\n' == c) ||
					((xPos + gdi_get_charwidth(c) + horizontal) > (x + width))) {
				if (PARAGRAPH_MODE == flag) {
					pFont = gxgdi_get_font();
					if (NULL == pFont) {
						return (1);
					} else {
						yPos += (pFont->ysize + vertical);
						if (pFont->ysize > (y + height - yPos)) {
							return (1);
						} else {
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
							text_width = 0;
						}
					}
				} else {
					return (0);
				}
			}

			gdi_draw_char(surface, &xPos, &yPos, (unsigned short)c, color);
			xPos += (gdi_get_charwidth(c) + horizontal);
			pStr++;
		}
	}
	return (0);
}

#ifndef NO_OS
static int _font_ascii_text_surface(void *screen, void *string, void *buffer, GAL_Rect *rect, int pos, int color)
{
	GuiCore *gui_core = GUI_GetCurrent();
	int ret = 0, char_size = 0, i = 0, offset = 0;
	int bpp = 0, back_color = 0;
	int xpos = 0, ypos = 0;
	char *pstr = NULL;
	unsigned short wc = 0;

	if((NULL == string) || (NULL == buffer))
	{
		return (1);
	}

	bpp = gui_core->config.bpp;

	pstr = (char *)string;
	back_color = gal_color2index(screen, gui_core->config.gui_trans);
	offset = pos;

	switch(bpp)
	{
		case 8:
			memset(buffer, back_color, rect->w * rect->h * gui_core->config.bpp / 8);

			while (*pstr != '\0')
			{
				// TODO:
				wc = *pstr++;

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
				wc = *pstr++;

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

static int _font_ascii_paragraph_surface(void *screen, void *string, void *buffer, GAL_Rect *rect, int pos, int color)
{
	GuiCore *gui_core = GUI_GetCurrent();
	int ret = 0, char_size = 0, offset = 0, skip = 0, line_num = 0;
	int bpp = 0, back_color = 0, x_pos = 0, y_pos = 0;
	unsigned short wc = 0;
	struct gdi_font* font = NULL;
	char *pstr = NULL;

	if((NULL == string) || (NULL == buffer))
	{
		return (1);
	}

	bpp = gui_core->config.bpp;
	back_color = gal_color2index(screen, gui_core->config.gui_trans);

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
				// TODO:
				wc = *pstr++;

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
#endif /*NO_OS*/

static int font_ascii_roll_text(void *screen, void *string, GAL_Rect* rect, int pos, int color, int flag)
{
#ifndef NO_OS
	GuiCore *gui_core = GUI_GetCurrent();
	int width = 0, height = 0, byteperline = 0;
	char *buffer = NULL;
	image_desc img_desc = {0};

	if((NULL == screen) || (NULL == string) || (NULL == rect))
	{
		return (1);
	}

	width = rect->w;
	height = rect->h;
	byteperline = (((width * gui_core->config.bpp) + 31) >> 5) << 2;

	buffer = (char *)GxCore_Malloc(byteperline * height * gui_core->config.bpp / 8);
	if(NULL == buffer)
	{
		return (1);
	}

	memset(buffer, 0, byteperline * height * gui_core->config.bpp / 8);

	// TODO: Draw temporary buffer

	if(SINGLELINE_MODE == flag)
	{
		_font_ascii_text_surface(screen, string, buffer, rect, pos, color);
	}
	else
	{
		_font_ascii_paragraph_surface(screen, string, buffer, rect, pos, color);
	}
	// TODO: BLIT the temporary buffer to screen
	img_desc.bpp = gui_core->config.bpp;
	img_desc.width = width;
	img_desc.height = height;
	img_desc.type = BMP_TYPE;
	img_desc.data = buffer;
	img_desc.pal_index = gal_get_clut_index();

	gal_img_desc(screen, &img_desc, rect->x, rect->y, img_desc.width, img_desc.height);

	/*Release the buffer*/
	GUI_FREE(buffer);

#endif
	return (0);
}

static int font_ascii_lines(void *string, GAL_Rect *rect, GAL_Interval *interval)
{
	int lines = 0, text_width = 0;
	char *pStr = NULL;
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
		wc = *pStr++;

		if(('\n' == wc))
		{
			lines++;
			text_width = 0;
			continue;
		}

		text_width += (gdi_get_charwidth(wc) + horizontal);
		if(text_width >= (rect->w))
		{
			lines++;
			text_width = 0;
		}
	}

	return (lines);
}

static int font_ascii_skip_line(void *p_start, void *string, GAL_Interval *interval, int line_width, int alignment)
{
	int width = 0, offset = 0, line = 0, char_width = 0, line_num = 0, line_end = 0;
	unsigned char * p_str = NULL, *p_head = NULL;
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
				return (0);
			}
			wc = *p_str;

			char_width = gdi_get_charwidth(wc) + horizontal;
			line += char_width;

			if(line > width)
			{
				break;
			}

			p_str++;
			offset++;
			if('\n' == wc)
			{
				break;
			}

			char_width = 0;
		}while(line < width);
	}
	else if(p_head <= p_str)
	{
		line_num = 0;
		do
		{
			wc = *p_head;

			char_width = gdi_get_charwidth(wc) + horizontal;

			line += char_width;

			p_head++;
			offset++;

			if(line > width)
			{
				p_head --;
				offset --;
			}

			if((line >= width) || ('\n' == wc))
			{
				line_end = line;
				line_num++;
				line = 0;
			}
		}while(p_head < (p_str ));

		if(line)
		{
			line_end = line;
		}

		char_width = 0;
		offset = 0;

		if('\n' == *(p_str - 1))
		{
			p_str--;
			offset++;
		}

		do
		{
			wc = *(p_str - 1);

			char_width += (gdi_get_charwidth(wc) + horizontal);

			if(char_width > line_end)
			{
				break;
			}

			if('\n' == wc)
			{
				//p_str--;
				//offset++;
				break;
			}

			p_str--;
			offset++;
		}while(char_width < line_end);

	}

	return (offset);
}

struct gdi_font_opt font_ascii_opt = {
	"ascii",
	font_ascii_len,
	font_ascii_text,
	font_ascii_skip_line,
	font_ascii_roll_text,
	font_ascii_lines
};


