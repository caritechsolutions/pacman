#ifndef NO_OS
#include <stdio.h>
#include <stdlib.h>
#include "gui.h"
#include "gui_private.h"

#else  /*NO_OS*/

#include "mini_gui_private.h"

#endif /*NO_OS*/

#include "gdi_char.h"
#include "gdi_font.h"
#include "gdi_font_utf8.h"

static int _check_khmer_word(unsigned int unicode)
{
	if((unicode >= 0x1780) && (unicode <= 0x17FF))
	{
		return (GUI_TRUE);
	}
	else
	{
		return (GUI_FALSE);
	}
}

int check_khmer(const unsigned int *unicode)
{
	unsigned int *pstr = NULL;

	if(NULL == unicode)
	{
		return (GUI_FALSE);
	}

	pstr = (unsigned int *)unicode;
	while(*pstr)
	{
		if(_check_khmer_word(*pstr))
		{
			return (GUI_TRUE);
		}
		pstr++;
	}
	return (GUI_FALSE);
}

static int _khmer_is_cjk_symbol(unsigned int unicode)
{
    if((unicode >= 0x2E00) && (unicode <= 0xFE4F))
	return GUI_TRUE;
    else
	return GUI_FALSE;
}

static int _khmer_is_vowel(unsigned int unicode)
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


static unsigned int _khmer_double_word_width(unsigned int *pstr)
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
		else if(_khmer_is_cjk_symbol(*ptr))
		{
		    break;
		}
		else
		{
			word = *ptr;
			if(_khmer_is_vowel(word))
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

static int _khmer_check_line_full(const unsigned char *string, int width, unsigned short horizontal)
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
		wc = ((*(pstr + 3)) << 24) | ((*(pstr + 2)) << 16) | ((*(pstr + 1)) << 8) | (*pstr);

		accent = ((*(pstr + 7)) << 24) | ((*(pstr + 6)) << 16) | ((*(pstr + 5)) << 8) | (*(pstr + 4));

		text_width += (gdi_get_charwidth(wc) + horizontal);
		if(_khmer_is_vowel(accent))
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
		else if((' ' == wc) || _khmer_is_cjk_symbol(wc))
		{
			if((' ' != *pstr) || (_khmer_is_cjk_symbol(*pstr)))
			{
				if((text_width + _khmer_double_word_width((unsigned int *)pstr)) > width)
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

static unsigned int _get_subindex(unsigned int next_char, unsigned int value)
{
    unsigned int index = 0;

    if((value >= 0x1780) && (value <= 0x17FF))
    {
	switch(value)
	{
	    case 0x1780:
	    case 0x1781:
	    case 0x1782:
	    case 0x1784:
	    case 0x1785:
	    case 0x1786:
	    case 0x1787:
	    case 0x178A:
	    case 0x178B:
	    case 0x178C:
	    case 0x178E:
	    case 0x178F:
	    case 0x1790:
	    case 0x1792:
	    case 0x1793:
	    case 0x1795:
	    case 0x1796:
	    case 0x1797:
	    case 0x1798:
	    case 0x179A:
	    case 0x179B:
	    case 0x179C:
	    case 0x179D:
	    case 0x17A0:
	    case 0x17A1:
	    case 0x17A2:
		if(0x17B6 == next_char)
		    index = 3;
		else if(0x17C5 == next_char)
		    index = 4;
		break;
	    case 0x1783:
	    case 0x1788:
	    case 0x178D:
	    case 0x1794:
	    case 0x1799:
	    case 0x179E:
	    case 0x179F:
		if(0x17B6 == next_char)
		    index = 3;
		else if(0x17C5 == next_char)
		    index = 6;
		break;

	}
    }
    return (index);
}

int khmer_display_presentation(void *surface, const unsigned int *unicode, GAL_Rect *rect, int size, int color, int alignment, int flag)
{
	int ret = 0, ignore_next = 0;
	unsigned int *p_uc = NULL;
	unsigned int prev_char = 0, cur_char = 0, next_char = 0, khmer_char = 0;
	unsigned int map_value = 0, accent = 0, iso6937_flag = 0, prev_map = 0;
	 int  yPos = 0, xoff = 0, xdelta = 0;
	int text_off = 0, subscript = GUI_FALSE;
	struct gdi_font *pFont;
	//unsigned int *penglish_char = NULL;

	static int xPos = 0;
	xPos = 0;
	static volatile int huangbc_test_enable = 1;

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
			text_off = _khmer_check_line_full((const unsigned char *)p_uc, rect->w, 0);
			xdelta = text_off / 2;
			xPos = xdelta + rect->x;
		}
		else if(GUI_TA_RIGHT & alignment)
		{
			text_off = _khmer_check_line_full((const unsigned char *)p_uc, rect->w, 0);
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
		{
			next_char = *(p_uc + 1);
		}
		else
		{
			next_char = 0;
		}

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

		// TODO:
		if(((xPos + xoff + gdi_get_charwidth(map_value)) > (rect->x + rect->w)) ||
				('\n' == map_value))
		{
			if(PARAGRAPH_MODE == flag)
			{
				yPos += pFont->ysize;
				if(GUI_TA_HCENTRE & alignment)
				{
					text_off = _khmer_check_line_full((const unsigned char *)p_uc, rect->w, 0);
					xdelta = text_off / 2;
					xPos = xdelta + rect->x;

				}
				else if(GUI_TA_RIGHT & alignment)
				{
					text_off = _khmer_check_line_full((const unsigned char *)p_uc, rect->w, 0);
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
			else
			{
				int sub_index = 0;

				if(_khmer_is_vowel(map_value))
				{
					xPos += xoff;
					if(prev_map == 0x17D2) //Khmer Subscript
					{
					    if(map_value == 0x1789)
						sub_index = 4;
					    else
						sub_index = 1;
					    xoff = gdi_get_subscriptwidth(map_value, sub_index);
					}
					else
					{
					    xoff = gdi_get_charwidth(map_value);
					}
				}
				else
				{
					xPos += xoff;
					if(prev_map == 0x17D2) //Khmer Subscript
					{
					    if(map_value == 0x1789)
						sub_index = 4;
					    else
						sub_index = 1;
					    xoff = gdi_get_subscriptwidth(map_value, sub_index);
					}
					else
					{
					    xoff = gdi_get_charwidth(map_value);
					}
				}
				/*For Khmer language modified in Mar 25th, 2015 temporarily by shencz.*/
				if(0x17D2 != map_value)
				{
				    if(prev_map == 0x17D2) //Khmer Subscript
				    {
					if(map_value == 0x1789)
					    sub_index = 4;
					else
					    sub_index = 1;
					if((next_char == 0x17BE) ||
					   (next_char == 0x17BF) ||
					   (next_char == 0x17C0) ||
					   (next_char == 0x17C4) ||
					   (next_char == 0x17C5))
					{
					    gdi_draw_char(surface, &xPos, &yPos, 0x17C1, color);
					    xPos += gdi_get_charwidth(0x17C1);
					    gdi_draw_subscript(surface, &xPos, &yPos, map_value, color, sub_index);
					    xPos += gdi_get_subscriptwidth(map_value, sub_index);
					    gdi_draw_subscript(surface, &xPos, &yPos, next_char, color, 1);
					    xPos += gdi_get_subscriptwidth(next_char, 1);
					    xoff = 0;
					    map_value = next_char;
					    p_uc++;
					}
					else if((next_char == 0x17C1) ||
					   (next_char == 0x17C2) ||
					   (next_char == 0x17C3))
					{
					    gdi_draw_char(surface, &xPos, &yPos, next_char, color);
					    xPos += gdi_get_charwidth(next_char);
					    gdi_draw_subscript(surface, &xPos, &yPos, map_value, color, sub_index);
					    map_value = next_char;
					    p_uc++;
					}
					else
					{
					    khmer_char = *(p_uc + 1);
					    if(
					       ((next_char == 0x17D2) && (khmer_char == 0x179A)))
					    {
						if(next_char == 0x17D2)
						{
						    next_char = 0x179A;
						    sub_index = 1;
						    p_uc += 2;
						}
						else
						{
						    sub_index = 0;
						    p_uc++;
						}
						gdi_draw_subscript(surface, &xPos, &yPos, next_char, color, sub_index);
						xPos += gdi_get_subscriptwidth(next_char, sub_index);
					    }
					    gdi_draw_subscript(surface, &xPos, &yPos, map_value, color, sub_index);
					    xPos += gdi_get_subscriptwidth(map_value, sub_index);
					    xoff = 0;
					}
					subscript = GUI_TRUE;
				    }
				    else
				    {
					if((next_char == 0x17D2) && (map_value == 0x1789))
					{
					    if((next_char == 0x17BE) ||
					       (next_char == 0x17BF) ||
					       (next_char == 0x17C0) ||
					       (next_char == 0x17C4) ||
					       (next_char == 0x17C5))
					    {
						gdi_draw_char(surface, &xPos, &yPos, 0x17C1, color);
						xPos += gdi_get_charwidth(0x17C1);
						gdi_draw_subscript(surface, &xPos, &yPos, map_value, color, sub_index);
						xPos += gdi_get_subscriptwidth(map_value, sub_index);
						gdi_draw_subscript(surface, &xPos, &yPos, next_char, color, 1);
						xPos += gdi_get_subscriptwidth(next_char, 1);
						xoff = 0;
						map_value = next_char;
						p_uc++;
					    }
					    else if((next_char == 0x17C1) ||
					       (next_char == 0x17C2) ||
					       (next_char == 0x17C3))
					    {
						gdi_draw_char(surface, &xPos, &yPos, next_char, color);
						xPos += gdi_get_charwidth(next_char);
						gdi_draw_subscript(surface, &xPos, &yPos, map_value, color, 1);
						map_value = next_char;
						p_uc++;
					    }
					    else
					    {
						khmer_char = *(p_uc + 1);
						if(/*(next_char == 0x179A) || */
						   ((next_char == 0x17D2) && (khmer_char == 0x179A)))
						{
						    if(next_char == 0x17D2)
						    {
							next_char = 0x179A;
							sub_index = 1;
							p_uc += 2;
						    }
						    else
						    {
							sub_index = 0;
							p_uc++;
						    }

						    gdi_draw_subscript(surface, &xPos, &yPos, next_char, color, sub_index);
						    xPos += gdi_get_subscriptwidth(next_char, sub_index);
						}
						gdi_draw_subscript(surface, &xPos, &yPos, map_value, color, 1);
					    }
					}
					else
					{
					    if((next_char == 0x17BE) ||
					       (next_char == 0x17BF) ||
					       (next_char == 0x17C0) ||
					       (next_char == 0x17C4) ||
					       (next_char == 0x17C5) ||
					       ((next_char == 0x17D2) && (*(p_uc + 2) == 0x17BE)) ||
					       ((next_char == 0x17D2) && (*(p_uc + 2) == 0x17BF)) ||
					       ((next_char == 0x17D2) && (*(p_uc + 2) == 0x17C0)) ||
					       ((next_char == 0x17D2) && (*(p_uc + 2) == 0x17C4)) ||
					       ((next_char == 0x17D2) && (*(p_uc + 2) == 0x17C5)))
					    {
						gdi_draw_char(surface, &xPos, &yPos, 0x17C1, color);
						xPos += gdi_get_charwidth(0x17C1);
						if(/*(*(p_uc) == 0x179A) || */
						   ((*(p_uc) == 0x17D2) && (*(p_uc + 1) == 0x179A)))
						{
						    unsigned int khmer_map = 0;
						    if(*(p_uc) == 0x17D2)
						    {
							khmer_map = 0x179A;
							sub_index = 1;
							p_uc += 2;
						    }
						    else
						    {
							sub_index = 0;
							p_uc++;
						    }
						    gdi_draw_subscript(surface, &xPos, &yPos, khmer_map, color, sub_index);
						    xPos += gdi_get_subscriptwidth(khmer_map, sub_index);
						    next_char = *p_uc;
						}
						gdi_draw_char(surface, &xPos, &yPos, map_value, color);
						xPos += gdi_get_charwidth(map_value);
						if(next_char == 0x17D2)
						{
						    int xtmp = 0;

						    xtmp = xPos;
						    gdi_draw_subscript(surface, &xPos, &yPos, *(p_uc + 1), color, 1);
						    next_char = *(p_uc + 2);
						    xPos = xtmp;
						    p_uc += 2;
						}
						gdi_draw_subscript(surface, &xPos, &yPos, next_char, color, 1);
						xPos += gdi_get_subscriptwidth(next_char, 1);
						xoff = 0;
						map_value = next_char;
						p_uc++;
					    }
					    else if((next_char == 0x17C1) ||
					       (next_char == 0x17C2) ||
					       (next_char == 0x17C3) ||
					       ((next_char == 0x17D2) && (*(p_uc + 2) == 0x17C1)) ||
					       ((next_char == 0x17D2) && (*(p_uc + 2) == 0x17C2)) ||
					       ((next_char == 0x17D2) && (*(p_uc + 2) == 0x17C3)))
					    {
						if(next_char == 0x17D2)
						{
						    gdi_draw_char(surface, &xPos, &yPos, *(p_uc + 2), color);
						    xPos += gdi_get_charwidth(*(p_uc + 2));
						    gdi_draw_char(surface, &xPos, &yPos, map_value, color);
						    xPos += gdi_get_charwidth(map_value);
						    gdi_draw_subscript(surface, &xPos, &yPos, *(p_uc + 1), color, 1);
						    xPos += gdi_get_subscriptwidth(*(p_uc + 1), 1);
						    xoff = 0;
						    p_uc += 2;
						    next_char = *(p_uc+3);
						}
						else
						{
						    gdi_draw_char(surface, &xPos, &yPos, next_char, color);
						    xPos += gdi_get_charwidth(next_char);
						    gdi_draw_char(surface, &xPos, &yPos, map_value, color);
						}
						map_value = next_char;
						p_uc++;
					    }
					    else if((next_char == 0x17B6) || (next_char == 0x17C5))
					    {
						sub_index = 0;
						sub_index = _get_subindex(next_char, map_value);
						if(sub_index)
						{
						    gdi_draw_subscript(surface, &xPos, &yPos, map_value, color, sub_index);
						    xPos += gdi_get_subscriptwidth(map_value, sub_index);
						    xoff = 0;
						    map_value = next_char;
						    p_uc++;
						}
						else
						{
						    gdi_draw_char(surface, &xPos, &yPos, map_value, color);
						}
					    }
					    else
					    {

						khmer_char = *(p_uc + 1);
					       if((khmer_char >= 0x17B7)  &&
						     (khmer_char <= 0x17BA) )
					       {
						    if((*p_uc == 0x17C9) || (*p_uc == 0x17CA))
						    {
							    *p_uc = 0x17BB;
						    }
					       }
					       if(huangbc_test_enable)
					       {
						       if(((khmer_char == 0x17C2) ) &&
							    (_check_khmer_word(map_value)))
							    {
								    gdi_draw_char(surface, &xPos, &yPos, khmer_char, color);
								     xPos += gdi_get_charwidth(khmer_char);
								     gdi_draw_char(surface, &xPos, &yPos, map_value, color);
								     xPos += gdi_get_charwidth(map_value);
								     map_value = next_char;
								     p_uc+=2;
							    }
					       }

						if(/*(next_char == 0x179A) || */
						   ((next_char == 0x17D2) && (khmer_char == 0x179A)))
						{
						    if(next_char == 0x17D2)
						    {
							next_char = 0x179A;
							sub_index = 1;
							p_uc += 2;
						    }
						    else
						    {
							sub_index = 0;
							p_uc++;
						    }
						    gdi_draw_subscript(surface, &xPos, &yPos, next_char, color, sub_index);
						    xPos += gdi_get_subscriptwidth(next_char, sub_index);

						    if((khmer_char == 0x17BE) ||
						       (khmer_char == 0x17BF) ||
						       (khmer_char == 0x17C0) ||
						       (khmer_char == 0x17C4) ||
						       (khmer_char == 0x17C5))
						    {
							    gdi_draw_char(surface, &xPos, &yPos, 0x17C1, color);
							    xPos += gdi_get_charwidth(0x17C1);
							    gdi_draw_subscript(surface,&xPos,&yPos,khmer_char,color,1);
							    //xPos += gdi_get_subscriptwidth(khmer_char, 1);
						    }


						}

						   if((map_value == 0x17BE) ||
						       (map_value == 0x17BF) ||
						       (map_value == 0x17C0) ||
						       (map_value == 0x17C4) ||
						       (map_value == 0x17C5) )
						    {

						    }

						    else
						    {
							      if(((map_value == 0x17BB) ||
							      (map_value == 0x17BC) ||
							      (map_value == 0x17BD)) &&
							      subscript)
							     {
								     yPos +=7;
							     }
							    gdi_draw_char(surface, &xPos, &yPos, map_value, color);
							    if((map_value == 0x17BB) ||
							      (map_value == 0x17BC) ||
							      (map_value == 0x17BD) ||
							      ((map_value >= 0x17C9) && (map_value <= 0x17D1)))
							    {
								    if(((map_value == 0x17BB) ||
								      (map_value == 0x17BC) ||
								      (map_value == 0x17BD)) &&
								      subscript)
								    {
									    yPos -= 7;
									 subscript = GUI_FALSE;
								    }
								    xPos += gdi_get_charwidth(map_value);
								  xoff = 0;
							    }
						    }
					    }
					}
					subscript = GUI_FALSE;
				    }
				}
				else
				{
				    //xPos -= gdi_get_charwidth(prev_map);
				    //gxlogd("------------coeng-------------\n");
				}
				prev_map = map_value;

				if((' ' == map_value) || (_khmer_is_cjk_symbol(map_value)))
				{
					if((' ' != *p_uc) || (_khmer_is_cjk_symbol(*p_uc)))
					{
						if((xPos + xoff + _khmer_double_word_width(p_uc)) > (rect->x + rect->w))
						{
							if(PARAGRAPH_MODE == flag)
							{
								yPos += pFont->ysize;
								if(GUI_TA_HCENTRE & alignment)
								{
									text_off = _khmer_check_line_full((const unsigned char *)p_uc, rect->w, 0);
									xdelta = text_off / 2;
									xPos = xdelta + rect->x;
								}
								else if(GUI_TA_RIGHT & alignment)
								{
									text_off = _khmer_check_line_full((const unsigned char *)p_uc, rect->w, 0);
									xdelta = text_off;
									xPos = xdelta + rect->x;
								}
								else
								{
									xdelta = 0;
									xPos = rect->x;
								}
								if(yPos > (rect->y + rect->h - pFont->ysize))
								{
									ret = ((unsigned int *)p_uc - (unsigned int *)unicode + 1);
									break;
								}
								xoff = 0;
							}
							else
							{
								//xdelta = 0;
								//break;
								gdi_draw_char(surface, &xPos, &yPos, map_value, color);
								prev_map = map_value;
								xoff = gdi_get_charwidth(map_value);
								continue;
							}
						}
					}
				}
			}

			iso6937_flag = 0;
		}

	}

	if(*p_uc)
		ret = p_uc - unicode;
	else
		ret = size;
	return (ret);
}


