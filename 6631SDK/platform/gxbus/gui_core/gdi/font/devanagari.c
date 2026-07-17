#ifndef NO_OS
#include <stdio.h>
#include <stdlib.h>
#include "gui.h"
#include "gui_private.h"
#else /*NO_OS*/
#include "gal_ttf.h"
#include "mini_gui_private.h"

#endif /*NO_OS*/

#include "gdi_char.h"
#include "gdi_font.h"
#include "gdi_font_utf8.h"

typedef struct _combine {
	unsigned int code[3];
	char name[40];
} combine;

static const combine cb[] = {
	{{0x915, 0x94d, 0x0}, {"kadeva_viramadeva"}},
	{{0x916, 0x94d, 0x0}, {"khadeva_viramadeva"}},
	{{0x917, 0x94d, 0x0}, {"gadeva_viramadeva"}},
	{{0x918, 0x94d, 0x0}, {"ghadeva_viramadeva"}},
	{{0x919, 0x94d, 0x0}, {"ngadeva_viramadeva"}},
	{{0x91a, 0x94d, 0x0}, {"cadeva_viramadeva"}},
	{{0x91b, 0x94d, 0x0}, {"chadeva_viramadeva"}},
	{{0x91c, 0x94d, 0x0}, {"jadeva_viramadeva"}},
	{{0x91d, 0x94d, 0x0}, {"jhadeva_viramadeva"}},
	{{0x91e, 0x94d, 0x0}, {"nyadeva_viramadeva"}},
	{{0x91f, 0x94d, 0x0}, {"ttadeva_viramadeva"}},
	{{0x920, 0x94d, 0x0}, {"tthadeva_viramadeva"}},
	{{0x921, 0x94d, 0x0}, {"ddadeva_viramadeva"}},
	{{0x922, 0x94d, 0x0}, {"ddhadeva_viramadeva"}},
	{{0x923, 0x94d, 0x0}, {"nnadeva_viramadeva"}},
	{{0x924, 0x94d, 0x0}, {"tadeva_viramadeva"}},
	{{0x925, 0x94d, 0x0}, {"thadeva_viramadeva"}},
	{{0x926, 0x94d, 0x0}, {"dadeva_viramadeva"}},
	{{0x927, 0x94d, 0x0}, {"dhadeva_viramadeva"}},
	{{0x928, 0x94d, 0x0}, {"nadeva_viramadeva"}},
	{{0x929, 0x94d, 0x0}, {"nnnadeva_viramadeva"}},
	{{0x92a, 0x94d, 0x0}, {"padeva_viramadeva"}},
	{{0x92b, 0x94d, 0x0}, {"phadeva_viramadeva"}},
	{{0x92c, 0x94d, 0x0}, {"badeva_viramadeva"}},
	{{0x92d, 0x94d, 0x0}, {"bhadeva_viramadeva"}},
	{{0x92e, 0x94d, 0x0}, {"madeva_viramadeva"}},
	{{0x92f, 0x94d, 0x0}, {"yadeva_viramadeva"}},
	{{0x930, 0x93c, 0x094d}, {"radeva_viramadeva"}},  /*{{0x930, 0x94d, 0x0}, {"radeva_viramadeva"}},*/
	{{0x932, 0x94d, 0x0}, {"ladeva_viramadeva"}},
	{{0x933, 0x94d, 0x0}, {"lladeva_viramadeva"}},
	{{0x934, 0x94d, 0x0}, {"llladeva_viramadeva"}},
	{{0x935, 0x94d, 0x0}, {"vadeva_viramadeva"}},
	{{0x936, 0x94d, 0x0}, {"shadeva_viramadeva"}},
	{{0x937, 0x94d, 0x0}, {"ssadeva_viramadeva"}},
	{{0x938, 0x94d, 0x0}, {"sadeva_viramadeva"}},
	{{0x939, 0x94d, 0x0}, {"hadeva_viramadeva"}},
	{{0x958, 0x94d, 0x0}, {"qadeva_viramadeva"}},
	{{0x959, 0x94d, 0x0}, {"khhadeva_viramadeva"}},
	{{0x95a, 0x94d, 0x0}, {"ghhadeva_viramadeva"}},
	{{0x95b, 0x94d, 0x0}, {"zadeva_viramadeva"}},
	{{0x95e, 0x94d, 0x0}, {"fadeva_viramadeva"}},
	{{0x915, 0x94d, 0x930}, {"kadeva_viramadeva_radeva"}},
	{{0x916, 0x94d, 0x930}, {"khadeva_viramadeva_radeva"}},
	{{0x917, 0x94d, 0x930}, {"gadeva_viramadeva_radeva"}},
	{{0x918, 0x94d, 0x930}, {"ghadeva_viramadeva_radeva"}},
	{{0x91a, 0x94d, 0x930}, {"cadeva_viramadeva_radeva"}},
	{{0x91c, 0x94d, 0x930}, {"jadeva_viramadeva_radeva"}},
	{{0x91d, 0x94d, 0x930}, {"jhadeva_viramadeva_radeva"}},
	{{0x91e, 0x94d, 0x930}, {"nyadeva_viramadeva_radeva"}},
	{{0x923, 0x94d, 0x930}, {"nnadeva_viramadeva_radeva"}},
	{{0x924, 0x94d, 0x930}, {"tadeva_viramadeva_radeva"}},
	{{0x925, 0x94d, 0x930}, {"thadeva_viramadeva_radeva"}},
	{{0x926, 0x94d, 0x930}, {"dadeva_viramadeva_radeva"}},
	{{0x927, 0x94d, 0x930}, {"dhadeva_viramadeva_radeva"}},
	{{0x928, 0x94d, 0x930}, {"nadeva_viramadeva_radeva"}},
	{{0x92a, 0x94d, 0x930}, {"padeva_viramadeva_radeva"}},
	{{0x92b, 0x94d, 0x930}, {"phadeva_viramadeva_radeva"}},
	{{0x92c, 0x94d, 0x930}, {"badeva_viramadeva_radeva"}},
	{{0x92d, 0x94d, 0x930}, {"bhadeva_viramadeva_radeva"}},
	{{0x92e, 0x94d, 0x930}, {"madeva_viramadeva_radeva"}},
	{{0x92f, 0x94d, 0x930}, {"yadeva_viramadeva_radeva"}},
	{{0x932, 0x94d, 0x930}, {"ladeva_viramadeva_radeva"}},
	{{0x935, 0x94d, 0x930}, {"vadeva_viramadeva_radeva"}},
	{{0x936, 0x94d, 0x930}, {"shadeva_viramadeva_radeva"}},
	{{0x937, 0x94d, 0x930}, {"ssadeva_viramadeva_radeva"}},
	{{0x938, 0x94d, 0x930}, {"sadeva_viramadeva_radeva"}},
	{{0x939, 0x94d, 0x930}, {"hadeva_viramadeva_radeva"}},
	{{0x915, 0x94d, 0x937}, {"kadeva_viramadeva_ssadeva"}},
	{{0x91c, 0x94d, 0x91e}, {"jadeva_viramadeva_nyadeva"}},
	{{0x936, 0x94d, 0x943}, {"sha_virama_rvocalicvoweldeva"}},
	{{0x915, 0x94d, 0x915}, {"kadeva_viramadeva_kadeva"}},
	{{0x915, 0x94d, 0x932}, {"kadeva_viramadeva_ladeva"}},
	{{0x915, 0x94d, 0x935}, {"kadeva_viramadeva_vadeva"}},
	{{0x917, 0x94d, 0x928}, {"gadeva_viramadeva_nadeva"}},
	{{0x919, 0x94d, 0x915}, {"ngadeva_viramadeva_kadeva"}},
	{{0x919, 0x94d, 0x916}, {"ngadeva_viramadeva_khadeva"}},
	{{0x919, 0x94d, 0x917}, {"ngadeva_viramadeva_gadeva"}},
	{{0x919, 0x94d, 0x918}, {"ngadeva_viramadeva_ghadeva"}},
	{{0x919, 0x94d, 0x92e}, {"ngadeva_viramadeva_madeva"}},
	{{0x91a, 0x94d, 0x91a}, {"cadeva_viramadeva_cadeva"}},
	{{0x91b, 0x94d, 0x935}, {"chadeva_viramadeva_vadeva"}},
	{{0x91c, 0x94d, 0x91c}, {"jadeva_viramadeva_jadeva"}},
	{{0x91e, 0x94d, 0x91a}, {"nyadeva_viramadeva_cadeva"}},
	{{0x91e, 0x94d, 0x91c}, {"nyadeva_viramadeva_jadeva"}},
	{{0x91f, 0x94d, 0x91f}, {"ttadeva_viramadeva_ttadeva"}},
	{{0x91f, 0x94d, 0x920}, {"ttadeva_viramadeva_tthadeva"}},
	{{0x91f, 0x94d, 0x92f}, {"ttadeva_viraamdeva_yadeva"}},
	{{0x91f, 0x94d, 0x935}, {"ttadeva_viramadeva_vadeva"}},
	{{0x920, 0x94d, 0x920}, {"tthadeva_viraamdeva_tthadeva"}},
	{{0x920, 0x94d, 0x92f}, {"tthadeva_viraamdeva_yadeva"}},
	{{0x921, 0x94d, 0x921}, {"ddadeva_viramadeva_ddadeva"}},
	{{0x921, 0x94d, 0x922}, {"ddadeva_viramadeva_ddhadeva"}},
	{{0x921, 0x94d, 0x92f}, {"ddadeva_viramadeva_yadeva"}},
	{{0x922, 0x94d, 0x922}, {"ddhadeva_viramadeva_ddhadeva"}},
	{{0x922, 0x94d, 0x92f}, {"ddhadeva_viramadeva_yadeva"}},
	{{0x926, 0x94d, 0x917}, {"dadeva_viramadeva_gadeva"}},
	{{0x926, 0x94d, 0x918}, {"dadeva_viramadeva_ghadeva"}},
	{{0x926, 0x94d, 0x926}, {"dadeva_viramadeva_dadeva"}},
	{{0x926, 0x94d, 0x927}, {"dadeva_viramadeva_dhadeva"}},
	{{0x926, 0x94d, 0x928}, {"dadeva_viramadeva_nadeva"}},
	{{0x926, 0x94d, 0x92c}, {"dadeva_viramadeva_badeva"}},
	{{0x926, 0x94d, 0x92d}, {"dadeva_viramadeva_bhadeva"}},
	{{0x926, 0x94d, 0x92e}, {"dadeva_viraamdeva_madeva"}},
	{{0x926, 0x94d, 0x92f}, {"dadeva_viraamdeva_yadeva"}},
	{{0x926, 0x94d, 0x935}, {"dadeva_viramadeva_vadeva"}},
	{{0x928, 0x94d, 0x928}, {"nadeva_viramadeva_nadeva"}},
	{{0x92a, 0x94d, 0x924}, {"padeva_viramadeva_tadeva"}},
	{{0x92a, 0x94d, 0x932}, {"padeva_viramadeva_ladeva"}},
	{{0x92b, 0x94d, 0x932}, {"phadeva_viramadeva_ladeva"}},
	{{0x932, 0x94d, 0x932}, {"ladeva_viramadeva_ladeva"}},
	{{0x936, 0x94d, 0x91a}, {"shadeva_viramadeva_cadeva"}},
	{{0x936, 0x94d, 0x928}, {"shadeva_viramadeva_nadeva"}},
	{{0x936, 0x94d, 0x932}, {"shadeva_viramadeva_ladeva"}},
	{{0x936, 0x94d, 0x935}, {"shadeva_viramadeva_vadeva"}},
	{{0x937, 0x94d, 0x91f}, {"ssadeva_viramadeva_ttadeva"}},
	{{0x937, 0x94d, 0x920}, {"ssadeva_viramadeva_tthadeva"}},
	{{0x939, 0x94d, 0x923}, {"hadeva_viramadeva_nnadeva"}},
	{{0x939, 0x94d, 0x928}, {"hadeva_viramadeva_nadeva"}},
	{{0x939, 0x94d, 0x92e}, {"hadeva_viramadeva_madeva"}},
	{{0x939, 0x94d, 0x92f}, {"hadeva_viramadeva_yadeva"}},
	{{0x939, 0x94d, 0x932}, {"hadeva_viramadeva_ladeva"}},
	{{0x939, 0x94d, 0x935}, {"hadeva_viramadeva_vadeva"}}
};

static const combine special_cb[] = {
	{{0x926, 0x943, 0x0}, {"dadeva_rvocalicvowelsigndeva"}},
	{{0x930, 0x941, 0x0}, {"radeva_uvowelsigndeva"}},
	{{0x930, 0x942, 0x0}, {"radeva_uuvowelsigndeva"}},
	{{0x92b, 0x930, 0x93c}, {"phadeva_radeva_nuktadeva"}},
	{{0x924, 0x930, 0x924}, {"tadeva_viramadeva_tadeva"}}
};

static int _check_devanagari_word(unsigned int unicode)
{
	if ((unicode >= 0x0900) && (unicode <= 0x097F))
		return (GUI_TRUE);
	else
		return (GUI_FALSE);
}


int check_devanagari(const unsigned int *unicode)
{
	unsigned int *pstr = NULL;

	if(NULL == unicode)
		return (GUI_FALSE);

	pstr = (unsigned int *)unicode;
	while (*pstr) {
		if(_check_devanagari_word(*pstr))
			return (GUI_TRUE);
		pstr++;
	}
	return (GUI_FALSE);
}

static int _devanagari_is_cjk_symbol(unsigned int unicode)
{
	    if((unicode >= 0x2E00) && (unicode <= 0xFE4F))
			    return GUI_TRUE;
		    else
				    return GUI_FALSE;
}

static int _devanagari_is_vowel(unsigned int unicode)
{
	if(((0x0E33 < unicode) && (unicode < 0x0E3F)) ||
			(0x0E31 == unicode) ||
			((0x0E46 < unicode) && (unicode < 0x0E4F)) ||
			((unicode >= 0x0300) && (unicode <= 0x036F)) ||
			(0x102B == unicode) ||
			((0x102C < unicode) && (0x1031 > unicode)) ||
			((0x1031 < unicode) && (0x1038 > unicode)) ||
			((0x1038 < unicode) && (0x103F > unicode)) ||
			((0x1057 < unicode) && (0x105A > unicode)) ||
			((0x105D < unicode) && (0x1061 > unicode)) ||
			((0x1070 < unicode) && (0x1075 > unicode)) ||
			(0x1082 == unicode) ||
			((0x1084 < unicode) && (0x1087 > unicode)) ||
			(0x108D == unicode) ||
			((0x17B5 < unicode) && (0x17D4 > unicode)))
	{
		return (GUI_TRUE);
	}
	else
	{
		return (GUI_FALSE);
	}
}

static unsigned int _devanagari_double_word_width(unsigned int *pstr)
{
	unsigned int *ptr = NULL;
	unsigned int word = 0;
	unsigned int word_width = 0;

	ptr = pstr;
	while(' ' != *ptr)
	{
		if('\n' == *ptr)
		{
			break;
		}
		else if('\0' == *ptr)
		{
			break;
		}
		else if(_devanagari_is_cjk_symbol(*ptr))
		{
			break;
		}
		else
		{
			word = *ptr;
			if(_devanagari_is_vowel(word))
			{
				;
			}
			else
			{
				word_width += gdi_get_charwidth(word);
			}
		}
		ptr++;
	}

	return (word_width);
}

static int _devanagari_check_line_full(const unsigned char *string, int width, unsigned short horizontal)
{
	int text_width = 0;
	char *pstr = NULL;
	unsigned int wc = 0, accent = 0;

	if(NULL == string)
	{
		return (0);
	}

	pstr = (char *)string;
	while(*(unsigned int*)pstr)
	{
		wc = ((*(pstr+1)) << 24) | ((*(pstr+1)) << 16) | ((*(pstr+1)) << 8) | (*pstr);

		accent = ((*(pstr + 7)) << 24) | ((*(pstr + 6)) << 16) | ((*(pstr + 5)) << 8) | (*(pstr + 4));

		text_width += (gdi_get_charwidth(wc) + horizontal);
		if(_devanagari_is_vowel(accent))
		{
			pstr += 8;
		}
		else
		{
			pstr += 4;
		}

		if(text_width >= width)
		{
			break;
		}
		else if((' ' == wc) || _devanagari_is_cjk_symbol(wc))
		{
			if((' ' != *pstr) || (_devanagari_is_cjk_symbol(*pstr)))
			{
				if((text_width + _devanagari_double_word_width((unsigned int *)pstr)) > width)
				{
					break;
				}
			}
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

static char * _find_cb(unsigned int cur, unsigned int next, unsigned int third)
{
	int i = 0;
	int size = sizeof(cb) / sizeof(cb[0]);

	for(i = 0; i < size; i++) {
		if(cb[i].code[0] == cur &&
		   cb[i].code[1] == next &&
		   cb[i].code[2] == third)
			return (char *)cb[i].name;
	}

	return NULL;
}

static char * _find_special_cb(unsigned int cur, unsigned int next, unsigned int third)
{
	int i = 0;
	int size = sizeof(special_cb) / sizeof(special_cb[0]);

	for(i = 0; i < size; i++) {
		if(special_cb[i].code[0] == cur &&
		   special_cb[i].code[1] == next &&
		   special_cb[i].code[2] == third)
			return (char *)special_cb[i].name;
	}

	return NULL;
}

int devanagari_display_presentation(void *surface, const unsigned int *unicode, GAL_Rect *rect, int size, int color, int alignment, int flag)
{
	int ret = 0, ignore_next = 0;
	unsigned int *p_uc = NULL;
	unsigned int prev_char = 0, cur_char = 0, next_char = 0, third_char = 0;
	unsigned int map_value = 0, accent = 0, iso6937_flag = 0, prev_map = 0;
	int  yPos = 0, xoff = 0, xdelta = 0;
#ifdef GUI_TTF
	int advance = 0;
#endif
	int minx = 0, maxx = 0;
	int text_off = 0;
	struct gdi_font *pFont;

	static int xPos = 0;
	xPos = 0;

	if(NULL == unicode)
	{
		return (-1);
	}

	pFont = gxgdi_get_font();
	if (NULL == pFont)
	{
		return (-1);
	}

	p_uc = (unsigned int *)unicode;

	if(PARAGRAPH_MODE == flag)
	{
		if(GUI_TA_HCENTRE & alignment)
		{
			text_off = _devanagari_check_line_full((const unsigned char *)p_uc, rect->w, 0);
			xdelta = text_off / 2;
			xPos = xdelta + rect->x;
		}
		else if(GUI_TA_RIGHT & alignment)
		{
			text_off = _devanagari_check_line_full((const unsigned char *)p_uc, rect->w, 0);
			xdelta = text_off;
			xPos = xdelta + rect->x;
		}
		else
		{
			xdelta = 0;
			xPos = rect->x;
		}

		yPos = rect->y;
	}
	else
	{
		xPos = rect->x;
		yPos = rect->y;
		xdelta = 0;
	}

	if(0xFEFF == *p_uc)
	{
		p_uc++;
	}

	while(*p_uc)
	{
		cur_char = *p_uc;
		if(*(p_uc + 1))
			next_char = *(p_uc + 1);
		else
			next_char = 0;

		if(*(p_uc + 2))
			third_char = *(p_uc + 2);
		else
			third_char = 0;

		prev_char = cur_char;

		if((cur_char >= 0x0300) && (cur_char <= 0x036F))
		{
			accent = cur_char;
			cur_char = *(p_uc + 1);

			p_uc++;
			iso6937_flag = 1;
		}

		map_value = cur_char;
		p_uc++;

		if((map_value >= 0xfef5) && (map_value <= 0xfefc))
		{
			prev_char = next_char;
			ignore_next = 0;
		}
		else if(0 == map_value)
		{
			ret = ((unsigned int *)p_uc - (unsigned int *)unicode + 1);
			break;
		}

		if(((xPos + xoff + gdi_get_charwidth(map_value)) > (rect->x + rect->w)) ||
				('\n' == map_value))
		{
			if(PARAGRAPH_MODE == flag)
			{
				yPos += pFont->ysize;
				if(GUI_TA_HCENTRE & alignment)
				{
					text_off = _devanagari_check_line_full((const unsigned char *)p_uc, rect->w, 0);
					xdelta = text_off / 2;
					xPos = xdelta + rect->x;
				}
				else if(GUI_TA_RIGHT & alignment)
				{
					text_off = _devanagari_check_line_full((const unsigned char *)p_uc, rect->w, 0);
					xdelta = text_off;
					xPos = xdelta + rect->x;
				}
				else
				{
					xPos = rect->x;
					xdelta = 0;
				}

				if(yPos > (rect->y + rect->h - pFont->ysize))
				{
					ret = ((unsigned int *)p_uc - (unsigned int *)unicode + 1);
					break;
				}
			}
			else
			{
				xdelta = 0;
				ret = ((unsigned int *)p_uc - (unsigned int *)unicode + 1);
				break;
			}

			xoff = 0;
		}

		if(0xFEFF != map_value)
		{
			if(iso6937_flag)
			{
				xPos += xoff;
				gdi_draw_char(surface, &xPos, &yPos, map_value, color);
				gdi_draw_char(surface, &xPos, &yPos, accent, color);
				prev_map = accent;
				xPos += (gdi_get_charwidth_with_accent(map_value, accent));
				xoff = 0;
			}

			xoff = gdi_get_charwidth(map_value);

			char *name = NULL;


			/*special combine*/
			if (third_char)
				name = _find_special_cb(map_value, next_char, third_char);

			if (name) {
#ifdef GUI_TTF
				TTF_GlyphMetrics_Byname((TTF_Font *)(pFont->font_data), name, &minx, &maxx, NULL, NULL, &advance);
#endif

				gdi_draw_char_byname(surface, &xPos, &yPos, name, color);
				xPos += (maxx - minx);

				p_uc += 2;
				continue;
			}

			name = _find_special_cb(map_value, next_char, 0x0);
			if (name){
#ifdef GUI_TTF
				TTF_GlyphMetrics_Byname((TTF_Font *)(pFont->font_data), name, &minx, &maxx, NULL, NULL, &advance);
#endif

				gdi_draw_char_byname(surface, &xPos, &yPos, name, color);
				xPos += (maxx - minx);

				p_uc++;
				continue;
			}

			/*
			 * combine 1.(0x915~0x935) + ox94d
			 *		   2.(0x915~0x935) + ox94d + 0x930
			 *		   3.ox930 + ox94d + ?
			 */
			if (next_char == 0x94d && map_value >= 0x915 && map_value <= 0x935) {
				name = NULL;

				if (third_char) {
					if (map_value == 0x930) {
						gdi_draw_char(surface, &xPos, &yPos, third_char, color);

						int width = gdi_get_charwidth(third_char);
						xPos += width * 0.6;

						gdi_draw_char_byname(surface, &xPos, &yPos, "rphfdeva", color);

						xPos += width * 0.4;
						p_uc += 2;
						continue;
					}
					name = _find_cb(map_value, next_char, third_char);
				}

				if (name) {
#ifdef GUI_TTF
					TTF_GlyphMetrics_Byname((TTF_Font *)(pFont->font_data), name,
							&minx, &maxx, NULL, NULL, &advance);
#endif

					gdi_draw_char_byname(surface, &xPos, &yPos, name, color);
					p_uc++;
				} else {
					name = _find_cb(map_value, next_char, 0x0);
#ifdef GUI_TTF
					TTF_GlyphMetrics_Byname((TTF_Font *)(pFont->font_data), name,
							&minx, &maxx, NULL, NULL, &advance);
#endif

					gdi_draw_char_byname(surface, &xPos, &yPos, name, color);
				}

				xPos += (maxx - minx);
				p_uc++;
				continue;

			} else if (next_char == 0x93f && map_value >= 0x915 && map_value <= 0x935) {
				/* (0x915~0x935) + 0x93f */
				int advance = 0;
#ifdef GUI_TTF
				TTF_GlyphMetrics((TTF_Font *)(pFont->font_data), next_char, NULL, NULL, NULL, NULL, &advance);
#endif

				if (map_value == 0x916 || map_value == 0x91c ||
					map_value == 0x91d || map_value == 0x91e || map_value == 0x932)
					gdi_draw_char_byname(surface, &xPos, &yPos, "iextendedsigndeva", color);
				else
					gdi_draw_char(surface, &xPos, &yPos, next_char, color);

				xPos += advance;
				gdi_draw_char(surface, &xPos, &yPos, map_value, color);
				xPos += xoff;

				p_uc++;
				continue;
			} else if (next_char == 0x940 && map_value >= 0x915 && map_value <= 0x935) {
				/* (0x915~0x935) + 0x940 */

				gdi_draw_char(surface, &xPos, &yPos, map_value, color);

				int advance = 0, minx = 0, maxx = 0;

				if (map_value == 0x915 || map_value == 0x92b) {
#ifdef GUI_TTF
					TTF_GlyphMetrics_Byname((TTF_Font *)(pFont->font_data), "iiextendeddeva",
												&minx, &maxx, NULL, NULL, &advance);
#endif
					xPos += advance;

					gdi_draw_char_byname(surface, &xPos, &yPos, "iiextendeddeva", color);
				} else {
					xPos += xoff;

#ifdef GUI_TTF
					TTF_GlyphMetrics((TTF_Font *)(pFont->font_data), 0x940, &minx, &maxx, NULL, NULL, &advance);
#endif
					gdi_draw_char(surface, &xPos, &yPos, next_char, color);
				}

				xPos += (maxx - minx);

				p_uc++;
				continue;
			} else if ((next_char == 0x941 || next_char == 0x942 ||
					   next_char == 0x947 || next_char == 0x948 ||
					   next_char == 0x902) && map_value >= 0x915 && map_value <= 0x935) {
				/* (0x915~0x935) + 0x941/0x942/0x947/0x948/0x902 */

				gdi_draw_char(surface, &xPos, &yPos, map_value, color);
				if (map_value == 0x915 || map_value == 0x92b)
					xPos += xoff * 0.7;
				else
					xPos += xoff;

#ifdef GUI_TTF
				TTF_GlyphMetrics((TTF_Font *)(pFont->font_data), next_char, &minx, &maxx, NULL, NULL, &advance);
#endif

				gdi_draw_char(surface, &xPos, &yPos, next_char, color);

				if (map_value == 0x915 || map_value == 0x92b)
					xPos += xoff * 0.3;
				xPos += gdi_get_charwidth(next_char);

				p_uc++;
				continue;
			}

			gdi_draw_char(surface, &xPos, &yPos, map_value, color);
			xPos += xoff;
		}
	}
	if(*p_uc)
		ret = p_uc - unicode;
	else
		ret = size;
	return ret;
}


