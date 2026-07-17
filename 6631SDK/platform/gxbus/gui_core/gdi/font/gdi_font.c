#ifndef NO_OS
#include "gui.h"
#include "gui_private.h"
#ifdef GUI_TTF
#include "SDL.h"
#endif /*GUI_TTF*/
#else /*NO_OS*/

#include "mini_gui.h"
#include "gxtype.h"
#include "mini_gui_private.h"
#include "gdi_core.h"
#include "gal_blit.h"
#include "hd_gal.h"
#include "gdi_char.h"
#ifdef GUI_TTF
#include "gal_ttf.h"
#endif /*GUI_TTF*/

void GxCore_GetFileInfo  (const char *path, GxFileInfo *Info)
{
	FILE *pFile = NULL;

	if((NULL == path) || (NULL == Info))
	{
		return;
	}

	pFile = fopen(path, "r");
	if(NULL == pFile)
	{
		return;
	}

	fseek(pFile, 0, SEEK_END);
	Info->size_by_bytes = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	fclose(pFile);
}

#if 0
#define rindex(s, c)			strrchr(s, c)
static char *rindex(const char *s, int c)
{
	char *str = NULL, *ptr = NULL;

	str = (char *)s;
	ptr = str + strlen(str);
	while(ptr != str)
	{
		if(*ptr == c)
			break;
		ptr--;
	}
	if(ptr == s)
		return (NULL);
	else
		return (ptr);
}
#endif
GXGDI_FontOpt *g_font_opt_fun;

#endif /*NO_OS*/
//#include <sys/stat.h>
#include <string.h>
#include "gdi_font.h"
#include "gdi_font_utf8.h"
#include "gdi_font_ascii.h"
#include "gdi_font_gb.h"
#include "gdi_core.h"
#include "gal_ttf.h"
#include "freetype/ftmodapi.h"

#ifndef NO_OS
static struct gdi_font_opt *font_list[] = {
#ifdef GUI_I18N
	&font_ascii_opt,
#else
	NULL,
#endif
	&font_gb_opt,
	&font_utf8_opt,
	&font_utf8_opt,
	&font_utf8_opt,
	&font_utf8_opt,
	&font_utf8_opt,
#ifdef GUI_TTF
	&font_utf8_opt,
#endif
	NULL
};
#else
static struct gdi_font_opt *font_list[] = {
	NULL,
	&font_gb_opt,
#ifdef GUI_I18N
	&font_utf8_opt,
	&font_utf8_opt,
	&font_utf8_opt,
	&font_utf8_opt,
	&font_utf8_opt,
#else
	NULL,//&font_utf8_opt,
	NULL,//&font_utf8_opt,
	NULL,//&font_utf8_opt,
	NULL,//&font_utf8_opt,
	NULL,//&font_utf8_opt,
#endif /*GUI_I18N*/
#ifdef GUI_TTF
	&font_utf8_opt,
#endif /*GUI_TTF*/
	NULL
};
#endif /*NO_OS*/

#ifdef GUI_TTF
static gdi_font_prop g_font_arrange[] = {
	{0x0020, 0x007F, NULL, NULL},
	{0x00A0, 0x00FF, NULL, NULL},
	{0x0100, 0x017F, NULL, NULL},
	{0x0180, 0x023F, NULL, NULL},
	{0x0250, 0x02AF, NULL, NULL},
	{0x02B0, 0x02EF, NULL, NULL},
	{0x0300, 0x036F, NULL, NULL},
	{0x0370, 0x03FF, NULL, NULL},
	{0x0400, 0x04FF, NULL, NULL},
	{0x0500, 0x052F, NULL, NULL},
	{0x0530, 0x058F, NULL, NULL},
	{0x0590, 0x05FF, NULL, NULL},
	{0x0600, 0x06FF, NULL, NULL},
	{0x0700, 0x074F, NULL, NULL},
	{0x0750, 0x077F, NULL, NULL},
	{0x0780, 0x07BF, NULL, NULL},
	{0x07C0, 0x07FF, NULL, NULL},
	{0x0900, 0x097F, NULL, NULL},
	{0x0980, 0x09FF, NULL, NULL},
	{0x0A00, 0x0A7F, NULL, NULL},
	{0x0A80, 0x0AFF, NULL, NULL},
	{0x0B00, 0x0B7F, NULL, NULL},
	{0x0B80, 0x0BFF, NULL, NULL},
	{0x0C00, 0x0C7F, NULL, NULL},
	{0x0C80, 0x0CFF, NULL, NULL},
	{0x0D00, 0x0D7F, NULL, NULL},
	{0x0D80, 0x0DFF, NULL, NULL},
	{0x0E00, 0x0E7F, NULL, NULL},
	{0x0E80, 0x0EFF, NULL, NULL},
	{0x0F00, 0x0FFF, NULL, NULL},
	{0x1000, 0x109F, NULL, NULL},
	{0x10A0, 0x10FF, NULL, NULL},
	{0x1100, 0x11FF, NULL, NULL},
	{0x1200, 0x137F, NULL, NULL},
	{0x1380, 0x139F, NULL, NULL},
	{0x13A0, 0x13FF, NULL, NULL},
	{0x1400, 0x167F, NULL, NULL},
	{0x1680, 0x169F, NULL, NULL},
	{0x16A0, 0x16FF, NULL, NULL},
	{0x1700, 0x171F, NULL, NULL},
	{0x1720, 0x173F, NULL, NULL},
	{0x1740, 0x175F, NULL, NULL},
	{0x1760, 0x177F, NULL, NULL},
	{0x1780, 0x17FF, NULL, NULL},
	{0x1800, 0x18AF, NULL, NULL},
	{0x1900, 0x194F, NULL, NULL},
	{0x1950, 0x197F, NULL, NULL},
	{0x1980, 0x19DF, NULL, NULL},
	{0x19E0, 0x19FF, NULL, NULL},
	{0x1A00, 0x1A1F, NULL, NULL},
	{0x1B00, 0x1B7F, NULL, NULL},
	{0x1D00, 0x1D7F, NULL, NULL},
	{0x1D80, 0x1DBF, NULL, NULL},
	{0x1DC0, 0x1DFF, NULL, NULL},
	{0x1E00, 0x1EFF, NULL, NULL},
	{0x1F00, 0x1FFF, NULL, NULL},
	{0x2000, 0x206F, NULL, NULL},
	{0x2070, 0x209F, NULL, NULL},
	{0x20A0, 0x20CF, NULL, NULL},
	{0x20D0, 0x20FF, NULL, NULL},
	{0x2100, 0x214F, NULL, NULL},
	{0x2150, 0x218F, NULL, NULL},
	{0x2190, 0x21FF, NULL, NULL},
	{0x2200, 0x22FF, NULL, NULL},
	{0x2300, 0x23FF, NULL, NULL},
	{0x2400, 0x243F, NULL, NULL},
	{0x2440, 0x245F, NULL, NULL},
	{0x2460, 0x24FF, NULL, NULL},
	{0x2500, 0x257F, NULL, NULL},
	{0x2580, 0x259F, NULL, NULL},
	{0x25A0, 0x25FF, NULL, NULL},
	{0x2600, 0x26FF, NULL, NULL},
	{0x2700, 0x27BF, NULL, NULL},
	{0x27C0, 0x27EF, NULL, NULL},
	{0x27F0, 0x27FF, NULL, NULL},
	{0x2800, 0x28FF, NULL, NULL},
	{0x2900, 0x297F, NULL, NULL},
	{0x2980, 0x29FF, NULL, NULL},
	{0x2A00, 0x2AFF, NULL, NULL},
	{0x2B00, 0x2BFF, NULL, NULL},
	{0x2C00, 0x2C5F, NULL, NULL},
	{0x2C60, 0x2C7F, NULL, NULL},
	{0x2C80, 0x2CFF, NULL, NULL},
	{0x2D00, 0x2D2F, NULL, NULL},
	{0x2D30, 0x2D7F, NULL, NULL},
	{0x2D80, 0x2DDF, NULL, NULL},
	{0x2E00, 0x2E7F, NULL, NULL},
	{0x2E80, 0x2EFF, NULL, NULL},
	{0x2F00, 0x2FDF, NULL, NULL},
	{0x2FF0, 0x2FFF, NULL, NULL},
	{0x3000, 0x303F, NULL, NULL},
	{0x3040, 0x309F, NULL, NULL},
	{0x30A0, 0x30FF, NULL, NULL},
	{0x3100, 0x312F, NULL, NULL},
	{0x3130, 0x318F, NULL, NULL},
	{0x3190, 0x319F, NULL, NULL},
	{0x31A0, 0x31BF, NULL, NULL},
	{0x31C0, 0x31EF, NULL, NULL},
	{0x31F0, 0x31FF, NULL, NULL},
	{0x3200, 0x32FF, NULL, NULL},
	{0x3300, 0x33FF, NULL, NULL},
	{0x3400, 0x4DBF, NULL, NULL},
	{0x4DC0, 0x4DFF, NULL, NULL},
	{0x4E00, 0x9FFF, NULL, NULL},
	{0xA000, 0xA48F, NULL, NULL},
	{0xA490, 0xA4CF, NULL, NULL},
	{0xA700, 0xA71F, NULL, NULL},
	{0xA720, 0xA7FF, NULL, NULL},
	{0xA800, 0xA82F, NULL, NULL},
	{0xA840, 0xA87F, NULL, NULL},
	{0xAC00, 0xD7AF, NULL, NULL},
	{0xD800, 0xDB7F, NULL, NULL},
	{0xDB80, 0xDBFF, NULL, NULL},
	{0xDC00, 0xDFFF, NULL, NULL},
	{0xE000, 0xF8FF, NULL, NULL},
	{0xF900, 0xFAFF, NULL, NULL},
	{0xFB00, 0xFB4F, NULL, NULL},
	{0xFB50, 0xFDFF, NULL, NULL},
	{0xFE00, 0xFE0F, NULL, NULL},
	{0xFE10, 0xFE1F, NULL, NULL},
	{0xFE20, 0xFE2F, NULL, NULL},
	{0xFE30, 0xFE4F, NULL, NULL},
	{0xFE50, 0xFE6F, NULL, NULL},
	{0xFE70, 0xFEFF, NULL, NULL},
	{0xFF00, 0xFFEF, NULL, NULL},
	{0xFFF0, 0xFFFF, NULL, NULL}
};

#ifdef GXDBG
char *unicode_subset_name[] =
{
	"Basic Latin",
	"Latin-1 Supplement",
	"Latin Extended-A",
	"Latin Extended-B",
	"IPA Extensions",
	"Spacing Modifier Letters",
	"Combining Diacritical Marks",
	"Greek and Coptic",
	"Cyrillic",
	"Cyrillic Supplement",
	"Armenian",
	"Hebrew",
	"Arabic",
	"Syriac",
	"Arabic Supplement",
	"Thaana",
	"N’Ko",
	"Devanagari",
	"Bengali",
	"Gurmukhi",
	"Gujarati",
	"Oriya",
	"Tamil",
	"Telugu",
	"Kannada",
	"Malayalam",
	"Sinhala",
	"Thai",
	"Lao",
	"Tibetan",
	"Myanmar",
	"Georgian",
	"Hangul Jamo",
	"Ethiopic",
	"Ethiopic Supplement",
	"Cherokee",
	"Unified Canadian Aboriginal Syllabics",
	"Ogham",
	"Runic",
	"Tagalog",
	"Hanunoo",
	"Buhid",
	"Tagbanwa",
	"Khmer",
	"Mongolian",
	"Limbu",
	"Tai Le",
	"New Tai Lue",
	"Khmer Symbols",
	"Buginese",
	"Balinese",
	"Phonetic Extensions",
	"Phonetic Extensions Supplement",
	"Combining Diacritical Marks Supplement",
	"Latin Extended Additional",
	"Greek Extended",
	"General Punctuation",
	"Superscripts and Subscripts",
	"Currency Symbols",
	"Combining Diacritical Marks for Symbols",
	"Letterlike Symbols",
	"Number Forms",
	"Arrows",
	"Mathematical Operators",
	"Miscellaneous Technical",
	"Control Pictures",
	"Optical Character Recognition",
	"Enclosed Alphanumerics",
	"Box Drawing",
	"Block Elements",
	"Geometric Shapes",
	"Miscellaneous Symbols",
	"Dingbats",
	"Miscellaneous Mathematical Symbols-A",
	"Supplemental Arrows-A",
	"Braille Patterns",
	"Supplemental Arrows-B",
	"Miscellaneous Mathematical Symbols-B",
	"Supplemental Mathematical Operators",
	"Miscellaneous Symbols and Arrows",
	"Glagolitic",
	"Latin Extended-C",
	"Coptic",
	"Georgian Supplement",
	"Tifinagh",
	"Ethiopic Extended",
	"Supplemental Punctuation",
	"CJK Radicals Supplement",
	"Kangxi Radicals",
	"Ideographic Description Characters",
	"CJK Symbols and Punctuation",
	"Hiragana",
	"Katakana",
	"Bopomofo",
	"Hangul Compatibility Jamo",
	"Kanbun",
	"Bopomofo Extended",
	"CJK Strokes",
	"Katakana Phonetic Extensions",
	"Enclosed CJK Letters and Months",
	"CJK Compatibility",
	"CJK Unified Ideographs Extension A",
	"Yijing Hexagram Symbols",
	"CJK Unified Ideographs",
	"Yi Syllables",
	"Yi Radicals",
	"Modifier Tone Letters",
	"Latin Extended-D",
	"Syloti Nagri",
	"Phags-pa",
	"Hangul Syllables",
	"High Surrogates",
	"High Private Use Surrogates",
	"Low Surrogates",
	"Private Use Area",
	"CJK Compatibility Ideographs",
	"Alphabetic Presentation Forms",
	"Arabic Presentation Forms-A",
	"Variation Selectors",
	"Vertical Forms",
	"Combining HalF",
	"CJK Compatibility Forms",
	"Small Form Variants",
	"Arabic Presentation Forms-B",
	"Halfwidth and Fullwidth Forms",
	"Specials"
};
#endif

#endif

extern GXGDI_FontOpt *g_font_opt_fun;

static encoding_node *p_default_encoding = NULL;
static encoding_node *p_current_encoding = NULL;

extern unsigned int GBK_Unicode[][2];
extern unsigned int BIG5_Unicode[][2];

encoding_node *EncodingPara[50] = {NULL};

int _get_first_encoding_node(void)
{
	int i = 0;
	while (i < (sizeof(EncodingPara) / sizeof(encoding_node *)))
	{
		if (EncodingPara[i] == NULL)
		{
			return (i);
		}
		i++;
	}
	return (-1);
}

static unsigned int *_get_iso_table(char *name)
{
	char* subset = NULL;
	int index = 0;

	if (0 == strcasecmp("iso6937", name))
	{
		return (get_iso2unicode_table(0));
	}
	else
	{
		subset = &(name[strlen(name) - 2]);
		index = atoi(subset);
		return (get_iso2unicode_table(index));
	}
}

static void _iso_register(char *name)
{
	int index = 0;
	encoding_node *p_encoding = NULL;

	index = _get_first_encoding_node();
	if (-1 == index)
	{
		return;
	}

	p_encoding = GxCore_Mallocz(sizeof(encoding_node));
	if (NULL == p_encoding)
	{
		return;
	}
	EncodingPara[index] = p_encoding;
	p_encoding->name = GxCore_Strdup(name);
	p_encoding->single_table = _get_iso_table(name);
	p_encoding->src_single_word = GUI_TRUE;
	p_encoding->start = 0;
	p_encoding->end = 0x7F;
}

int gdi_encoding_init(void)
{
	unsigned int i = 0;

	_iso_register("iso6937");
	_iso_register("ucs2");
	for(i = 0; i < 16; i++)
	{
		char enc_name[50] = {0};
		memset(enc_name, 0, 50);
		sprintf(enc_name, "iso8859-%02d", (i + 1));
		_iso_register(enc_name);
	}
	_iso_register("utf8");

	return (0);
}

#ifdef CONFIG_FONT_GB
#include "gbk_table.h"
int gdi_register_gb2312(void)
{
	int index = 0;
	encoding_node *p_encoding = NULL;

	index = _get_first_encoding_node();
	if (-1 == index)
	{
		return (1);
	}

	p_encoding = GxCore_Mallocz(sizeof(encoding_node));
	if (NULL == p_encoding)
	{
		return (1);
	}

	EncodingPara[index] = p_encoding;
	p_encoding->name = GxCore_Strdup("gb2312");
	p_encoding->table = GBK_Unicode;
	p_encoding->start = 0;
	p_encoding->end = sizeof(GBK_Unicode) / 8 - 1;
	return (0);
}

int gdi_register_gbk(void)
{
	int index = 0;
	encoding_node *p_encoding = NULL;

	index = _get_first_encoding_node();
	if (-1 == index)
	{
		return (1);
	}

	p_encoding = GxCore_Mallocz(sizeof(encoding_node));
	if (NULL == p_encoding)
	{
		return (1);
	}

	EncodingPara[index] = p_encoding;
	p_encoding->name = GxCore_Strdup("gbk");
	p_encoding->table = GBK_Unicode;
	p_encoding->start = 0;
	p_encoding->end = sizeof(GBK_Unicode) / 8 - 1;
	return (0);
}
#endif

#ifdef CONFIG_FONT_BIG5
#include "big5_table.h"
int gdi_register_big5(void)
{
	int index = 0;
	encoding_node *p_encoding = NULL;

	index = _get_first_encoding_node();
	if (-1 == index)
	{
		return (1);
	}

	p_encoding = GxCore_Mallocz(sizeof(encoding_node));
	if (NULL == p_encoding)
	{
		return (1);
	}

	EncodingPara[index] = p_encoding;
	p_encoding->name = GxCore_Strdup("big5");
	p_encoding->table = BIG5_Unicode;
	p_encoding->start = 0;
	p_encoding->end = sizeof(BIG5_Unicode) / 8 - 1;
	return (0);
}
#endif

#ifdef CONFIG_FONT_BIG5
#include "ksx_table.h"
int gdi_register_ksx(void)
{
	int index = 0;
	encoding_node *p_encoding = NULL;

	index = _get_first_encoding_node();
	if (-1 == index)
	{
		return (1);
	}

	p_encoding = GxCore_Mallocz(sizeof(encoding_node));
	if (NULL == p_encoding)
	{
		return (1);
	}

	EncodingPara[index] = p_encoding;
	p_encoding->name = GxCore_Strdup("ksx");
	p_encoding->table = KSX_Unicode;
	p_encoding->start = 0;
	p_encoding->end = sizeof(KSX_Unicode) / 8 - 1;
	return (0);
}
#endif

int gdi_set_default_local(char *name)
{
	int i = 0;
	encoding_node *p_encoding = NULL;

	if (NULL == name)
	{
		p_default_encoding = NULL;
		return (0);
	}

	while (i < (sizeof(EncodingPara) / sizeof(encoding_node *)))
	{
		p_encoding = EncodingPara[i];
		if (p_encoding == NULL)
			break;
		if (0 == strcasecmp(name, p_encoding->name))
			break;
		i++;
	}

	p_current_encoding = p_default_encoding = p_encoding;
	return (0);
}

static encoding_node *gdi_find_encoding(char *name)
{
	int i = 0;
	encoding_node *p_encoding = NULL;

	while(i < (sizeof(EncodingPara) / sizeof(encoding_node *)))
	{
		p_encoding = EncodingPara[i];
		if(p_encoding == NULL)
			break;
		if(0 == strcasecmp(name, p_encoding->name))
			break;
		i++;
	}

	if(p_encoding)
		return (p_encoding);
	else
		return (p_current_encoding);
}

int gdi_register_encoding(char *name, unsigned int (*table)[2], unsigned int size)
{
	int index = 0;
	encoding_node *node = NULL;

	if((NULL == name) || (NULL == table))
		return (1);

	node = gdi_find_encoding(name);
	if(NULL == node)
	{
		node = GxCore_Mallocz(sizeof(encoding_node));
		if(NULL == node)
			return (1);

		index = _get_first_encoding_node();
		EncodingPara[index] = node;
	}

	node->name = GxCore_Strdup(name);
	node->table = table;
	node->start = 0;
	node->end = size;
	return (0);
}

int gdi_unregister_encoding(char *name)
{
	encoding_node *node = NULL;
	int i = 0;

	if(NULL == name)
		return (1);

	node = gdi_find_encoding(name);
	if(NULL == node)
		return (1);

	while (i < (sizeof(EncodingPara) / sizeof(encoding_node *)))
	{
		if(EncodingPara[i] == node)
		{
			GUI_FREE(node->name);
			GUI_FREE(EncodingPara[i]);
			return (0);
		}
	}

	return (1);
}

int gdi_set_local(char *name)
{
	int i = 0;
	encoding_node *p_encoding = NULL;

	if (NULL == name)
	{
		p_current_encoding = p_default_encoding;
		return (0);
	}

	while (i < (sizeof(EncodingPara) / sizeof(encoding_node *)))
	{
		p_encoding = EncodingPara[i];
		if (p_encoding == NULL)
			break;
		if (0 == strcasecmp(name, p_encoding->name))
			break;
		i++;
	}

	if (p_encoding)
	{
		p_current_encoding = p_encoding;
	}
	return (0);
}

encoding_node *gdi_get_local(void)
{
	return p_current_encoding;
}

#ifdef GUI_STR_ICONV
static int UnicodeBinarySearch(unsigned int key, unsigned int lookup_table[][2], int start, int end)
{
	while (start <= end)
	{
		int mid = (start + end)/2;
		if ( key >  lookup_table[mid][0])
		{
			start = mid+1;
		}
		else if (key < lookup_table[mid][0])
		{
			end = mid - 1;
		}
		else //found
		{
			return mid;
		}
	}
	return -1;
}

static unsigned int _get_encoding_dst(encoding_node *encoding, unsigned int code)
{
	if(NULL == encoding)
		encoding = p_current_encoding;

	unsigned int (*table)[2] = encoding->table;
	unsigned int i = 0;

	if(code < 0x80)
		return (code);
	else if (code == 0x8A)
		return ('\n');

	if(encoding->src_single_word)
	{
		if(NULL == encoding->single_table)
			return (code);
		if(code >= 0xA0) {
			code = encoding->single_table[code - 0xA0];
		}
		return (code);
	}
	else
	{
		i = UnicodeBinarySearch(code,
				table,
				encoding->start,
				encoding->end);
		if(i < 0)
		{
			return (code);
		}

		if (table[i][1] != 0)
		{
			return (table[i][1]);
		}
		else
		{
			return (code);
		}
	}
}

static int enc_unicode_to_utf8_one(unsigned long unic, char *pOutput)
{
	if (pOutput == NULL)
	{
		return (0);
	}

	if ( unic <= 0x0000007F )
	{
		// * U-00000000 - U-0000007F:  0xxxxxxx
		*pOutput     = (unic & 0x7F);
		return 1;
	}
	else if ( unic >= 0x00000080 && unic <= 0x000007FF )
	{
		// * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
		*(pOutput+1) = (unic & 0x3F) | 0x80;
		*pOutput     = ((unic >> 6) & 0x1F) | 0xC0;
		return 2;
	}
	else if ( unic >= 0x00000800 && unic <= 0x0000FFFF )
	{
		// * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
		*(pOutput+2) = (unic & 0x3F) | 0x80;
		*(pOutput+1) = ((unic >>  6) & 0x3F) | 0x80;
		*pOutput     = ((unic >> 12) & 0x0F) | 0xE0;
		return 3;
	}
	else if ( unic >= 0x00010000 && unic <= 0x001FFFFF )
	{
		// * U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		*(pOutput+3) = (unic & 0x3F) | 0x80;
		*(pOutput+2) = ((unic >>  6) & 0x3F) | 0x80;
		*(pOutput+1) = ((unic >> 12) & 0x3F) | 0x80;
		*pOutput     = ((unic >> 18) & 0x07) | 0xF0;
		return 4;
	}
	else if ( unic >= 0x00200000 && unic <= 0x03FFFFFF )
	{
		// * U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
		*(pOutput+4) = (unic & 0x3F) | 0x80;
		*(pOutput+3) = ((unic >>  6) & 0x3F) | 0x80;
		*(pOutput+2) = ((unic >> 12) & 0x3F) | 0x80;
		*(pOutput+1) = ((unic >> 18) & 0x3F) | 0x80;
		*pOutput     = ((unic >> 24) & 0x03) | 0xF8;
		return 5;
	}
	else if ( unic >= 0x04000000 && unic <= 0x7FFFFFFF )
	{
		// * U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
		*(pOutput+5) = (unic & 0x3F) | 0x80;
		*(pOutput+4) = ((unic >>  6) & 0x3F) | 0x80;
		*(pOutput+3) = ((unic >> 12) & 0x3F) | 0x80;
		*(pOutput+2) = ((unic >> 18) & 0x3F) | 0x80;
		*(pOutput+1) = ((unic >> 24) & 0x3F) | 0x80;
		*pOutput     = ((unic >> 30) & 0x01) | 0xFC;
		return 6;
	}

	return 0;
}

static unsigned short _enc_merge_byte(unsigned char byte0, unsigned char byte1)
{
	return byte1 | (((unsigned short)byte0) << 8);
}

static unsigned int _get_string_code(encoding_node * encoding, char **str)
{
	unsigned int code = 0;
	char *pstr = *str;

	if(encoding == NULL)
		encoding = p_current_encoding;
	if (((*pstr) == 0x86) || ((*pstr) == 0x87))
	{
		pstr++;
	}

	if(encoding->src_single_word)
	{
		code = *pstr;
		pstr++;
	}
	else
	{
		if ((*pstr) < 0x80)
		{
			code = *pstr;
			pstr++;
		}
		else
		{
			if(*(pstr + 1))
			{
				code = _enc_merge_byte(*pstr, *(pstr + 1));
				pstr += 2;
			}
			else
			{
				code = *pstr;
				pstr++;
			}
		}
	}

	*str = pstr;
	return (code);
}

static int _ucs_len(char *string)
{
	char *pstr = NULL;
	unsigned short code = 0;
	int len = 0;

	pstr = string;
	code = _enc_merge_byte(*pstr, *(pstr + 1));
	while (code)
	{
		pstr += 2;
		len++;
		code = _enc_merge_byte(*pstr, *(pstr + 1));
	}

	return (len);
}

static void _iso10646_to_utf8(const unsigned char *src, unsigned char *dst, unsigned int src_size, unsigned int *dst_size)
{
	unsigned char utf8_code[10] = {0};
	unsigned short usc = 0;
	unsigned int i = 0, dst_index = 0;

	if ((NULL == src) || (NULL == dst))
	{
		return;
	}

	i = 0;
	dst_index = 0;
	while (i < src_size * 2)
	{
		usc = src[i++];
		usc = (usc << 8) | src[i++];
		if (0xE08A == usc)
		{
			dst[dst_index++] = '\n';
		}
		else if ((0x7F >= usc) && (usc >= 0x20))
		{
			dst[dst_index++] = usc;
		}
		else if ((0x80 <= usc) && (0x7FF >= usc))
		{
			utf8_code[0] = 0xC0 | ((usc & 0x07C0) >> 6);
			utf8_code[1] = 0x80 | (usc & 0x003F);
			dst[dst_index++] = utf8_code[0];
			dst[dst_index++] = utf8_code[1];
		}
		else if ((0x800 <= usc) && (0xFFFF >= usc))
		{
			if ((0xE086 != usc) && (0xE087 != usc))
			{
				utf8_code[0] = 0xE0 | ((usc & 0xF000) >> 12);
				utf8_code[1] = 0x80 | ((usc & 0x0FC0) >> 6);
				utf8_code[2] = 0x80 | ((usc & 0x003F));
				dst[dst_index++] = utf8_code[0];
				dst[dst_index++] = utf8_code[1];
				dst[dst_index++] = utf8_code[2];
			}
		}
	}
	dst[dst_index] = 0;
	*dst_size += dst_index;
}

static void _dvb_utf8(const unsigned char *src_str, unsigned char *dst_str, unsigned int size)
{
	while ((*src_str) && size)
	{
		if ((0xC2 == *src_str) && (0x8A == *(src_str + 1)))
		{
			*dst_str = '\n';
			src_str += 2;
			size -= 2;
		}
		else if (((0xC2 == *src_str) && (0x86 == *(src_str + 1))) ||
				((0xC2 == *src_str) && (0x87 == *(src_str + 1))))
		{
			src_str += 2;
			size -= 2;
			continue;
		}
		else
		{
			*dst_str = *src_str;
		}

		dst_str++;
		src_str++;
		size--;
	}
}

char *gdi_iconv_bylocal(encoding_node *encoding, char *string)
{
	unsigned int len = 0, utf8_size = 0, out_len = 0;
	char *pstr = NULL, *ptr = NULL;
	char utf8_array[10] = {0};
	unsigned int src_code = 0, dst_code = 0;

	if(NULL == encoding)
		encoding = p_current_encoding;

	if (NULL == string)
	{
		return (NULL);
	}
	else if ((NULL == encoding) ||
			 (0 == strcasecmp("utf8", encoding->name)))
	{
		// utf8
		len = strlen(string);
		pstr = GxCore_Mallocz(len + 10);
		_dvb_utf8((unsigned char*)string, (unsigned char *)pstr, (unsigned int)len);
		return (pstr);
	}

	if(0 == strcasecmp("iso6937", encoding->name))
	{
		// ISO6937 switch 0xC1~0xCf(Accent) and next letter
		;//_process_iso6937(string);
	}
	else if(0 == strcasecmp("ucs2", encoding->name))
	{
		// ISO/IEC 10646
		len = _ucs_len(string);
		pstr = GxCore_Mallocz((len + 1) * 8);
		if (NULL == pstr)
		{
			return (NULL);
		}
		_iso10646_to_utf8((const unsigned char *)string,
						  (unsigned char *)pstr,
						  len,
						  &out_len);
		return (pstr);
	}

	len = strlen(string) + 1;
	len = len * 8;
	pstr = GxCore_Mallocz(len);
	if (NULL == pstr)
	{
		return (NULL);
	}

	ptr = pstr;
	while (*string)
	{
		src_code = _get_string_code(encoding, &string);
		dst_code = _get_encoding_dst(encoding, src_code);
		utf8_size = enc_unicode_to_utf8_one(dst_code, utf8_array);
		memcpy(ptr, utf8_array, utf8_size);
		ptr += utf8_size;
		memset(utf8_array, 0, 10);
	}
	return (pstr);
}
#endif /*GUI_STR_ICONV*/

static unsigned char FILE__Read8(char **pFile)
{
	unsigned char Value = 0;

	if (NULL == pFile || NULL == *pFile)
	{
		return (0);
	}

	Value = **pFile;
	*pFile += 1;

	return Value;
}

static unsigned short FILE__Read16(char **pFile)
{
	unsigned char pData[2] = { 0 };
	unsigned short Value = 0;

	if (NULL == pFile || NULL == *pFile)
	{
		return (0);
	}

	memcpy(pData, *pFile, 2);
	*pFile += 2;

	Value = *pData;
	Value |= (unsigned short)(*(pData + 1) << 8);

	return Value;
}

static unsigned int FILE__Read32(char **pFile)
{
	unsigned char pData[4] = { 0 };
	unsigned int Value;

	if (NULL == pFile || NULL == *pFile)
	{
		return (0);
	}

	memcpy(pData, *pFile, 4);
	*pFile += 4;

	Value = *pData;
	Value |= (*(pData + 1) << 8);
	Value |= ((unsigned int)*(pData + 2) << 16);
	Value |= ((unsigned int)*(pData + 3) << 24);

	return Value;
}

static int gdi_set_xsize(struct gdi_font *pFont, int xsize)
{
	if (NULL == pFont)
	{
		return (1);
	}

	pFont->xsize = xsize;

	return (0);
}

static gdi_charinfo *gdi_char_information(char **pFile, const struct gdi_font *pFont, int Range, int Offset)
{
	int i = 0, max_size = 0;
	gdi_charinfo *pCharInfo = NULL;

	if (NULL == pFile || NULL == *pFile || NULL == pFont)
	{
		return (0);
	}

	pCharInfo = (gdi_charinfo *) GxCore_Malloc(Range * sizeof(gdi_charinfo));
	if (NULL == pCharInfo) {
		gxlogd("[GUI]Cannot MALLOC!\n");
		return (NULL);
	}

	memset(pCharInfo, 0, sizeof(gdi_charinfo));

	//GxCore_Seek(pFile, Offset, SEEK_SET);
	*pFile += Offset;
	while (i < Range) {
		FILE__Read8(pFile);
		FILE__Read8(pFile);
		pCharInfo[i].MinX  = 0;
		pCharInfo[i].MinY  = 0;
		pCharInfo[i].XSize = FILE__Read8(pFile);
		if (pCharInfo[i].XSize > max_size)
		{
			max_size = pCharInfo[i].XSize;
		}
		pCharInfo[i].XDist = FILE__Read8(pFile);
		pCharInfo[i].BytesPerLine = FILE__Read8(pFile);
		pCharInfo[i].pData = (unsigned char *)GxCore_Malloc(pFont->ysize * pCharInfo[i].BytesPerLine);
		FILE_READ_BUF(*pFile, pFont->ysize * pCharInfo[i].BytesPerLine, (void *)pCharInfo[i].pData);
		i++;
	}

	gdi_set_xsize((struct gdi_font *)pFont, max_size);

	return (pCharInfo);
}

static gdi_font_prop *gdi_font_propertion(char **pFile, const struct gdi_font *pFont, int Offset)
{
	gdi_font_prop *pFontProp = NULL;
	int InfoOff, NextOff;
	char *pStart = NULL;

	if (0 == Offset) {
		return (NULL);
	}

	if (0 == Offset || NULL == pFile || NULL == *pFile || NULL == pFont)
	{
		return (NULL);
	}

	//GxCore_Seek(pFile, Offset, SEEK_SET);
	pStart = *pFile;
	*pFile += Offset;
	pFontProp = (gdi_font_prop *) GxCore_Malloc(sizeof(gdi_font_prop));
	memset(pFontProp, 0, sizeof(gdi_font_prop));
	if (NULL == pFontProp)
	{
		gxlogd("[GUI]Cannot MALLOC!\n");
		return (NULL);
	}
	else
	{
		pFontProp->First = FILE__Read16(pFile);
		pFontProp->Last = FILE__Read16(pFile);
		InfoOff = FILE__Read32(pFile);
		NextOff = FILE__Read32(pFile);
		// TODO:
		*pFile = pStart;
		pFontProp->paCharInfo = gdi_char_information(pFile,
				pFont, pFontProp->Last - pFontProp->First + 1, InfoOff);
		//gxlogd("gdi_font_propertion\n");

		*pFile = pStart;
		pFontProp->pNext = gdi_font_propertion(pFile, pFont, NextOff);
	}

	return (pFontProp);
}

char *_get_font_ext_name(const char *file_name)
{
	char *p;

	if (NULL == file_name)
		return (NULL);

	p = rindex(file_name, '.');

	if (p)
		return GxCore_Strdup(p + 1);

	return NULL;
}

int gdi_strlen(const char *string)
{
	int len = 0;
	unsigned short c = 0;

	if (0x10 == *string)
	{
		len = strlen(string + 3) + 3;
	}
	else if (0x11 == *string)
	{
		len = 1;
		string++;
		do{
			c = ((*string) << 8) | (*(string + 1));
			string += 2;
			len += 2;
		}while (c);
	}
	else
	{
		len = strlen(string);
	}

	return (len);
}

char* gdi_strcpy(char *dest, const char *src)
{
	int len = 0;

	if (NULL == dest || NULL == src)
	{
		return (NULL);
	}

	if (0x10 == *src)
	{
		memcpy(dest, src, 3);
		strcpy(dest + 3, src + 3);
	}
	else if (0x11 == *src)
	{
		len = gdi_strlen(src);
		memcpy(dest, src, len);
		dest[len - 1] = 0;
		dest[len - 2] = 0;
	}
	else
	{
		strcpy(dest, src);
	}

	return ((char *)src);
}

#ifdef GUI_TTF

#define MAX_TTF_TABLE		    (30)

static ttf_statistic_talbe ttf_table[MAX_TTF_TABLE];

static int _ttf_is_exist(char *filename)
{
	int i = 0;

	for(i = 0; i < MAX_TTF_TABLE; i++)
	{
		if ((ttf_table[i].filename) && (0 == strcasecmp(ttf_table[i].filename, filename)))
		{
			return (i);
		}
	}
	return (MAX_TTF_TABLE);
}

static int _ttf_empty_table(void)
{
	int i = 0;

	for(i = 0; i < MAX_TTF_TABLE; i++)
	{
		if ((NULL == ttf_table[i].filename) && (NULL == ttf_table[i].table))
			return (i);
	}

	return (MAX_TTF_TABLE);
}

static void _check_ttf_font_arrange(struct gdi_font *pFont, TTF_Font *font, char *filename)
{
	FT_Face face;
	int first_index = 0, last_index = 0, i = 0, count = 0, table_index = 0;
	gdi_font_prop *pindex_table = NULL;

	if (NULL == font || NULL == font->face)
	{
		return;
	}

	face = font->face;
	table_index = _ttf_is_exist(filename);
	if (MAX_TTF_TABLE != table_index)
	{
		pFont->index_table	= ttf_table[table_index].table;
		return;
	}

	count = sizeof(g_font_arrange) / sizeof(g_font_arrange[0]);

#ifdef GXDBG
	GUI_Printf("##############################################################\n");
	GUI_Printf("[GUI][Font]The file %s support subset is :\n", filename);
#endif
	for(i = 0; i < count; i++)
	{
		first_index = FT_Get_Char_Index(face, (g_font_arrange[i].First + 1));
		last_index = FT_Get_Char_Index(face, (g_font_arrange[i].Last - 1));
		if (first_index || last_index)
		{
			pindex_table = (gdi_font_prop *)GxCore_Malloc(sizeof(gdi_font_prop));
			if (NULL == pindex_table)
			{
				return;
			}
			memset(pindex_table, 0, sizeof(gdi_font_prop));

			pindex_table->First = g_font_arrange[i].First;
			pindex_table->Last  = g_font_arrange[i].Last;

			pindex_table->pNext = pFont->index_table;
			pFont->index_table	= pindex_table;
#ifdef GXDBG
			GUI_Printf("%s\n", unicode_subset_name[i]);
#endif
		}
	}
#ifdef GXDBG
	GUI_Printf("##############################################################\n");
#endif

	table_index = _ttf_empty_table();
	if (table_index < MAX_TTF_TABLE)
	{
		ttf_table[table_index].filename = GxCore_Strdup(filename);
		ttf_table[table_index].table = pindex_table;
	}
}

static void _release_ttf_index_table(struct gdi_font *pFont)
{
	gdi_font_prop *pindex_table = NULL, *next_table = NULL;
	int i = 0;

	pindex_table = pFont->index_table;
	if (NULL == pindex_table)
	{
		return;
	}

	while(ttf_table[i].table) {
		if(ttf_table[i].table == pindex_table) {
			GUI_FREE(ttf_table[i].filename);
			ttf_table[i].table = NULL;
			break;
		}
		i++;
	}

	while (pindex_table)
	{
		next_table = (gdi_font_prop *)(pindex_table->pNext);
		GUI_FREE(pindex_table);
		pindex_table = next_table;
	}
}
#endif

#ifdef GUI_TTF
static struct gdi_font *_find_font_byfile(char *file)
{
	GuiCore *gui_core = NULL;
	struct gdi_font *pFont = NULL;

	gui_core = GUI_GetCurrent();
	pFont = gui_core->config.pFirstFont;
	while(pFont)
	{
		if((file) &&
		   (pFont->filename) &&
		   (0 == strcasecmp(file, pFont->filename)))
		{
			return (pFont);
		}
		pFont = pFont->next;
	}

	return (NULL);
}
#endif

int gdi_font_load(GuiConfig *config, char * id, char *pFileName, int size, int style)
{
#ifdef GUI_TTF
	struct gdi_font *pOldFont = NULL;
#endif
	handle_t pFileFont = 0;
	GxFileInfo fileinfo = {0};
	int Offset = 0;
	char *ext_name = NULL;
	char *font_buffer = NULL, *ptr = NULL;
#ifdef GUI_TTF
	TTF_Font *ttf_font = NULL;
#endif

	if((NULL == config) || (NULL == id)) {
		return (-1);
	}

	struct gdi_font *pFont = config->pLastFont;

	if (NULL == pFileName) {
#ifndef NO_OS
		gxlogf ( "[GUI]There is no font file\n");
#else
		gxlogf ("[GUI]There is no font file\n");
#endif /*NO_OS*/
		return (-1);
	}
	else
	{
		ext_name = _get_font_ext_name(pFileName);
		if (ext_name)
		{
			if (0 == strcasecmp(ext_name, "ttf"))
			{
#ifdef GUI_TTF
				TTF_Init();
#endif
			}
		}
		if (NULL == pFont) {
			pFont = (struct gdi_font *)GxCore_Malloc(sizeof(struct gdi_font));
			if (NULL == pFont) {
				return (1);
			}
			memset(pFont, 0, sizeof(struct gdi_font));
			config->pFirstFont = pFont;
			config->pLastFont = pFont;
		} else {
			struct gdi_font *pNewFont = NULL;

			pNewFont = (struct gdi_font *)GxCore_Malloc(sizeof(struct gdi_font));
			if (NULL == pNewFont) {
				return (1);
			}

			memset(pNewFont, 0, sizeof(struct gdi_font));
			//strcpy(pNewFont->font_name, pFileName);
			pFont->next = pNewFont;
			pFont = config->pLastFont = pFont->next;
		}

#ifdef GUI_TTF
		if (ext_name)
		{
			if (0 == strcasecmp(ext_name, "ttf"))
			{
				// TODO:
				unsigned int unicode[2] = {0};

				unicode[0] = 0x4D;
				unicode[1] = 0x0;
				ttf_font = TTF_OpenFont(pFileName, size);
				if (NULL == ttf_font) {
					gxlogd("[GUI] Load %s font file failed.\n", pFileName);
					return (1);
				}
				_check_ttf_font_arrange(pFont, ttf_font, pFileName);
				TTF_SizeUNICODE(ttf_font, unicode, &(pFont->xsize), &(pFont->ysize));
				pFont->font_data = (void *)ttf_font;
				pFont->family = TTF_FONT_TYPE;
				ttf_font->style = style;
				if (id)
				{
					strcpy(pFont->font_name, id);
				}

				pOldFont = _find_font_byfile(pFileName);
				if(pOldFont)
				{
#ifndef NO_OS
					if(config->gsub_opt) {
						pFont->gsub_opt = (GSUB_Opt *)config->gsub_opt;
					}
					pFont->info = pOldFont->info;
#else
					;
#endif /*NO_OS*/
				}
				else if(gui.config.esoteric_language == GUI_TRUE)
				{
#ifndef NO_OS
					if(config->gsub_opt) {
						pFont->gsub_opt = (GSUB_Opt *)config->gsub_opt;
						pFont->info = pFont->gsub_opt->read_info(pFileName, ttf_font->face);
					}
#else
					;
#endif /*NO_OS*/
				}
#ifndef NO_OS
				if((pFont->info) && (config->gsub_opt))
				{
					pFont->gsub_opt = (GSUB_Opt *)config->gsub_opt;
					pFont->gsub_opt->arrange(pFont->info);
#ifdef GXDBG
					pFont->gsub_opt->print(pFont->info);
#endif /*GXDBG*/
				}
#endif /*NO_OS*/
			}
			GUI_FREE(ext_name);
			if (0 == pFont->xsize)
			{
				pFont->xsize = pFont->ysize;
			}
			pFont->font_size = size;
			gxgdi_set_default_font(config, pFont);

			pFont->filename = GxCore_Strdup(pFileName);
			return (0);
		}
#else
		GUI_FREE(ext_name);
#endif

		GxCore_GetFileInfo(pFileName, &fileinfo);

		pFileFont = GxCore_Open(pFileName, "r");
		if (0 == pFileFont) {
#ifndef NO_OS
			gxloge ( "[GUI]Cannot open file %s normally!\n", pFileName);
#else
			gxloge ("[GUI]Cannot open file %s normally!\n", pFileName);
#endif
			return (-1);
		}

		font_buffer = (char *)GxCore_Malloc(fileinfo.size_by_bytes + 100);
		if (NULL == font_buffer)
		{
			GxCore_Close(pFileFont);
			return (1);
		}

		memset(font_buffer, 0, fileinfo.size_by_bytes + 100);
		GxCore_Read(pFileFont, font_buffer, fileinfo.size_by_bytes, 1);

		ptr = font_buffer;

		memset(pFont->font_name, 0, MAX_FONT_NAME);
		//FILE_READ_BUF(ptr, MAX_FONT_NAME, pFont->font_name);
		ptr += MAX_FONT_NAME;
		strcpy(pFont->font_name, id);
		if (strlen(pFont->font_name) >= MAX_FONT_NAME) {
			gxlogd("[GUI]The font name is incorrect!\n");
			GUI_FREE(pFont);
			GUI_FREE(font_buffer);
			GxCore_Close(pFileFont);
			return (1);
		}

		pFont->family = FILE__Read32(&ptr);
		pFont->ysize = FILE__Read32(&ptr);
		//strcpy(pFont->font_name, pFileName);
		Offset = FILE__Read32(&ptr);

		//gxlogd("Family = 0x%x, YSize = 0x%x, Offset = 0x%x\n", pFont->family, pFont->ysize, Offset);

		// TODO:
		ptr = font_buffer;
		pFont->font_data = (void *)gdi_font_propertion(&ptr, pFont, Offset);
		pFont->next = NULL;

		GUI_FREE(font_buffer);

		GxCore_Close(pFileFont);
	}

	pFont->filename = GxCore_Strdup(pFileName);
	config->pDefaultFont = pFont;
	return (0);
}

struct gdi_font *gdi_get_first_font(void)
{
	GuiCore *gui_core = NULL;

	gui_core = GUI_GetCurrent();

	if(gui_core->config.pFirstFont)
		return (gui_core->config.pFirstFont);
	else
		return (gui.config.pFirstFont);
}

static void _resume_font_table(struct gdi_font *pFont, struct gdi_font *pPrev) {
	GuiCore *gui_core = GUI_GetCurrent();

	if (pFont == gui_core->config.pLastFont)
	{
		gui_core->config.pLastFont = pPrev;
	}
	if (pFont == gui_core->config.pDefaultFont)
	{
		gui_core->config.pDefaultFont = pPrev;
	}
}

int gdi_get_font_height_bysize(char *fontfile, int size)
{
#ifdef GUI_TTF
	TTF_Font *ttf_font = NULL;
	unsigned int unicode[2] = {0};
	int width = 0, height = 0;

	unicode[0] = 0x4D;
	unicode[1] = 0x0;
	ttf_font = TTF_OpenFont(fontfile, size);
	if(NULL == ttf_font) {
		return (-1);
	}

	TTF_SizeUNICODE(ttf_font, unicode, &(width), &(height));
	TTF_CloseFont(ttf_font);

	return (height);
#else
	return (0);
#endif /*GUI_TTF*/
}

struct gdi_font *gdi_font_get_byname(const unsigned char *name)
{
#ifndef NO_OS
	GuiCore *gui_core = GUI_GetCurrent();

	struct gdi_font *font = gui_core->config.pFirstFont;
	while(font) {
		if((font) && ((0 == strcasecmp((const char *)name, font->font_name))))
			return (font);
		font = font->next;
	}

	return (NULL);
#else
	return (NULL);
#endif /*NO_OSF*/
}

int gdi_font_destroy(GuiConfig *config, struct gdi_font *pFont)
{
	struct gdi_font *pPrev = config->pFirstFont;
	gdi_font_prop *pFontProp = NULL;
	gdi_font_prop *pFontNext = NULL;
	gdi_charinfo *pCharInfo = NULL;
	int i = 0, range = 0;

	if((NULL == config) || (NULL == pFont)) {
		return (1);
	}

	if (TTF_FONT_TYPE == pFont->family) {
#ifdef GUI_TTF
		if(pFont == config->pLastFont) {
			_release_ttf_index_table(pFont);
		}
		TTF_CloseFont((TTF_Font *)pFont->font_data);
		TTF_Quit();
#endif /*GUI_TTF*/
	} else {
		pFontProp = (gdi_font_prop *) (pFont->font_data);
		while (NULL != pFontProp) {
			pFontNext = (gdi_font_prop *) (pFontProp->pNext);
			pCharInfo = (gdi_charinfo *) (pFontProp->paCharInfo);
			range = pFontProp->Last - pFontProp->First + 1;
			for (i = 0; i < range; i++) {
				GxCore_Free((char *)(pCharInfo[i].pData));
			}
			GUI_FREE(pCharInfo);
			GUI_FREE(pFontProp);
			pFontProp = pFontNext;
		}
	}

	if(config->pFirstFont == pFont) {
		config->pFirstFont = pFont->next;
	} else {
		pPrev = config->pFirstFont;
		while(pPrev->next != pFont) {
			pPrev = pPrev->next;
			if (NULL == pPrev) {
				break;
			}
		}
		if (NULL != pPrev) {
			pPrev->next = pFont->next;
		}
	}
	_resume_font_table(pFont, pPrev);
	GUI_FREE(pFont->filename);
	GUI_FREE(pFont);
	pFont = NULL;

	return (0);
}

struct gdi_font *gxgdi_get_font(void)
{
	GuiCore *gui_core = NULL;
	GuiConfig *config = NULL;
	int i = 0, count = 0;

	gui_core = GUI_GetCurrent();
#ifndef NO_OS
	if(gui_core->resource_config_stack && gui_core->resource_config_table) {
		count = stack_count(gui_core->resource_config_stack);
		for(i = 0; i < count; i++) {
			config = stack_get(gui_core->resource_config_stack, i);
			if(config && !config->invalid) {
				if(config->pCurrentFont)
					return (config->pCurrentFont);
			}
		}
	}
#endif /*NO_OS*/

	if(gui_core->config.pCurrentFont)
		return (gui_core->config.pCurrentFont);
	else
	{
		if((NULL == gui_core->config.pFirstFont) &&
		   (NULL == gui_core->config.pLastFont) &&
		   (NULL == gui_core->config.pDefaultFont))
		{
			gui_core->config.pDefaultFont = gui.config.pDefaultFont;
		}
		return (gui.config.pCurrentFont);
	}
}

void gxgdi_get_bearingheight(struct gdi_font *pfont, int code, int *yoffset, int *height)
{
	if((NULL == pfont) || (NULL == yoffset) || (NULL == height)) {
		return;
	}
	if(pfont->family == TTF_FONT_TYPE) {
#ifdef GUI_TTF
#ifndef NO_OS
		return (ttf_get_bearingheight((TTF_Font *)pfont->font_data, code, yoffset, height));
#endif /*NO_OS*/
#endif /*GUI_TTF*/
	} else {
		*yoffset = 0;
		*height = pfont->ysize;
	}
}

bool gxgdi_is_exist(unsigned short ch)
{
	struct gdi_font *font = gxgdi_get_font();

	if(font->family == TTF_FONT_TYPE) {
#ifdef GUI_TTF
#ifndef NO_OS
		if(TTF_IsExist(font->font_data, ch)) {
			return (GUI_TRUE);
		}
#endif /*NO_OS*/
#endif /*GUI_TTF*/
	} else {
		if(gdi_check_char(ch)) {
			return (GUI_TRUE);
		}
	}

	return (GUI_FALSE);
}

void gxgdi_set_default_font(GuiConfig *config, struct gdi_font *pFont)
{
	config->pDefaultFont = pFont;
}

struct gdi_font *gxgdi_set_font(char *pFontName)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiConfig *config = NULL;
	struct gdi_font *pFont = gui_core->config.pFirstFont;
	struct gdi_font *pOld = gxgdi_get_font();
	int count = 0, i = 0;

	if (NULL == pFontName) {
		gui_core->config.pCurrentFont = gui_core->config.pDefaultFont;
		return (pOld);
	}

	// Get resource font
#ifndef NO_OS
	if(gui_core->resource_config_stack && gui_core->resource_config_table) {
		count = stack_count(gui_core->resource_config_stack);
		for(i = 0; i < count; i++) {
			config = stack_get(gui_core->resource_config_stack, i);
			if(config && !config->invalid) {
				pFont = config->pFirstFont;
				while(pFont) {
					if (0 == strcasecmp(pFontName, pFont->font_name)) {
						break;
					}
					pFont = pFont->next;
				}
				config->pCurrentFont = pFont;
				if(pFont) {
					return (pOld);
				}
			}
		}
	}
#endif /*NO_OS*/

	pFont = gui_core->config.pFirstFont;
	if(NULL == pFont)
	{
		pFont = gui.config.pFirstFont;
	}

	while (pFont) {
		if (0 == strcmp(pFontName, pFont->font_name)) {
			break;
		} else {
			pFont = pFont->next;
		}
	}

	// Get public font
	if((NULL == pFont) && (gui_core != &gui))
	{
		pFont = gui.config.pFirstFont;
		while (pFont) {
			if (0 == strcmp(pFontName, pFont->font_name)) {
				break;
			} else {
				pFont = pFont->next;
			}
		}
	}
	if (NULL != pFont) {
		if(pFont->ysize > gui_core->config.font_threshold_height) {
			pFont->hw_draw = GUI_TRUE;
		}
		gui_core->config.pCurrentFont = pFont;
		return (pOld);
	} else {
		return (NULL);
	}
}

void gxgdi_font_infomation_print(void)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiConfig *config = NULL;
	struct gdi_font *pFont = NULL;
	int i = 0, count = 0;

	if(NULL == gui_core) {
		return;
	}

	gxlogd("************************Font Information ************************\n");
	gxlogd("Font threshold Size = %d, Height = %d (Pixels) \n", gui_core->config.font_threshold_size, gui_core->config.font_threshold_height);
	gxlogd("-----------------------------------------------------------------\n");
#ifndef NO_OS
	if(gui_core->resource_config_stack) {
		count = stack_count(gui_core->resource_config_stack);
		for(i = 0; i < count; i++) {
			config = stack_get(gui_core->resource_config_stack, i);
			gxlogd("Font threshold Size = %d, Height = %d (Pixels) \n", config->font_threshold_size, config->font_threshold_height);
		}
	}
#endif /*NO_OS*/
	gxlogd("-----------------------------------------------------------------\n");
	gxlogd("Solution fonts list:\n");
	pFont = gui_core->config.pFirstFont;
	gxlogd("-----------------------------------------------------------------\n");
	while(pFont) {
		gxlogd("Name : %s, \tSize = %d, \tHeight = %d(Pixels)\n", pFont->font_name, pFont->font_size, pFont->ysize);
		pFont = pFont->next;
	}
	gxlogd("-----------------------------------------------------------------\n");
#ifndef NO_OS
	gxlogd("-----------------------------------------------------------------\n");
	gxlogd("Package fonts list:\n");
	if(gui_core->resource_config_stack) {
		count = stack_count(gui_core->resource_config_stack);
		for(i = 0; i < count; i++) {
			pFont = config->pFirstFont;
			while(pFont) {
				gxlogd("Name : %s, \tSize = %d, \tHeight = %d(Pixels)\n", pFont->font_name, pFont->font_size, pFont->ysize);
				pFont = pFont->next;
			}
		}
	}
	gxlogd("-----------------------------------------------------------------\n");
#endif /*NO_OS*/
	gxlogd("*****************************************************************\n");
}

bool gxgdi_font_is_hwdraw(void)
{
	struct gdi_font *pFont = NULL;

	pFont = gxgdi_get_font();
	if(pFont) {
		if(TTF_FONT_TYPE != pFont->family)
			return (GUI_TRUE);
		else
			return (pFont->hw_draw);
	} else {
		return (GUI_TRUE);
	}
}

int gxgdi_draw_string_back_color(void *screen, void *string, GAL_Rect *rect, GAL_Interval *interval, int alignment, int color, int back_color, int flag)
{
	GuiCore *gui_core = GUI_GetCurrent();
	struct gdi_font *pFont = NULL;
	struct gdi_font_opt *FontOpt = NULL;
	int height = 0, ret = 0;
	GuiSurface *surface = (GuiSurface *)screen, *src_surface = NULL;
	GAL_Rect src_rect = {0}, trans_rect = {0}, dst_rect = {0};
	unsigned int buffer_width = 0, buffer_height = 0, i = 0;
	char *read_buffer = NULL;
	void *p_convert_str = NULL;
	//unsigned short *word_buffer = NULL;

	if (NULL == surface || NULL == string || NULL == rect || NULL == interval)
	{
		//gxlogd("[GUI]Illegal surface\n");
		return (-1);
	}

	//x = rect->x;
	//y = rect->y;
	//width = rect->w;
	height = rect->h;

	pFont = gxgdi_get_font();

	if (NULL == pFont) {
		gxlogd("[GUI]Illegal font::::Please reset the font\n");
		return (-1);
	} else {
		if (pFont->ysize > height) {
			return (-1);
		}
		if (g_font_opt_fun)
		{
			FontOpt = (struct gdi_font_opt *)g_font_opt_fun;
		}
		else
		{
			FontOpt = font_list[pFont->family];
		}

		if (NULL == FontOpt) {
			gxlogd("[GUI]Illegal FontOpt\n");
			return (-1);
		} else {
			src_rect = *rect;
			if ((gui.config.enable_double_buffer) &&
					(TTF_FONT_TYPE == gui_core->config.pCurrentFont->family))
			{
				src_surface = surface;
			}
			else
			{
				src_rect.x = src_rect.y = 0;
				src_rect.w += (gui_core->config.pCurrentFont->xsize);

				src_surface = gal_get_surface(&src_rect, surface->bpp);
				if (NULL == src_surface)
				{
					gxlogd("[GUI] Create a surface [%d, %d, %d, %d] failed.\n",
							src_rect.x,
							src_rect.y,
							src_rect.w,
							src_rect.h);
					return (-1);
				}

				src_rect.w -= gui_core->config.pCurrentFont->xsize;
			}

			if ((gui.config.enable_double_buffer) &&
					(TTF_FONT_TYPE == gui_core->config.pCurrentFont->family))
			{
				gdi_commit();  /*For ttf blend*/
			}
			else
			{
				if (src_rect.w < MAX_STRING_SURFACE_WIDTH)
				{
					gdi_begin();
					hd_fillrect(src_surface, &src_rect, back_color);
#ifdef GUI_TTF
					if ((gui.config.enable_double_buffer) &&
							(TTF_FONT_TYPE == gui_core->config.pCurrentFont->family))
					{
						gdi_commit();  /*For ttf blend*/
						gal_copy_surface(screen, rect, src_surface, &src_rect);
					}
#endif
					gdi_end();
				}
				else
				{
					hd_read_surface(src_surface,
							(unsigned int *)&buffer_width,
							(unsigned int *)&buffer_height,
							(void **)&read_buffer);
					if (8 == gui.config.bpp)
					{
						memset(read_buffer,
								back_color,
								buffer_width * buffer_height * gui.config.bpp / 8);
					}
					else
					{
#if 0
						unsigned short color_16 = 0;

						color_16 = surface_color;
						color_16 = (color_16 << 8) | ((color_16 >> 8) & 0x00FF);
						word_buffer = (unsigned short *)read_buffer;
						for(i = 0;i < buffer_width * buffer_height; i++)
						{
							*(word_buffer + i) = color_16;
						}
#endif
						gdi_begin();
						hd_fillrect(src_surface, &src_rect, back_color);
						gdi_end();
					}
				}
			}

			gdi_begin();
			//src_rect.w -= gui_core->config.pCurrentFont->xsize;
			if (pFont->family != GB_FAMILY)
			{
#ifdef GUI_STR_ICONV
                                p_convert_str = (void *)gdi_iconv_bylocal(NULL, (char *)string);
#else
                                p_convert_str = (void *)string;
#endif
			}
			else
			{
				p_convert_str = string;
			}
			if(!gxgdi_font_is_hwdraw()) {
				gdi_commit();
			}
			ret = (FontOpt->pDrawText)(src_surface,
					p_convert_str,
					&src_rect,
					interval,
					alignment,
					color,
					flag);
			if(!gxgdi_font_is_hwdraw()) {
				hd_sync();
			}
			if (p_convert_str != string)
			{
				GUI_FREE(p_convert_str);
			}
			gdi_end();

			if (src_rect.w < MAX_STRING_SURFACE_WIDTH)
			{
				if ((gui.config.enable_double_buffer) &&
						(TTF_FONT_TYPE == gui_core->config.pCurrentFont->family))
				{
					;
				}
				else
				{
					hd_stretch_surface(src_surface, &src_rect, surface, rect);
					hd_add_blit_element(src_surface);
				}
			}
			else
			{
				buffer_width = src_rect.w / MAX_STRING_SURFACE_WIDTH;
				i = 0;
				trans_rect = src_rect;
				dst_rect = *rect;
				dst_rect.w = trans_rect.w = MAX_STRING_SURFACE_WIDTH;

				while (i < buffer_width)
				{
					hd_stretch_surface(src_surface,
							&trans_rect,
							surface,
							&dst_rect);
					hd_add_blit_element(NULL);

					trans_rect.x += MAX_STRING_SURFACE_WIDTH;
					dst_rect.x += MAX_STRING_SURFACE_WIDTH;
					i++;
				}

				trans_rect.w = dst_rect.w = src_rect.w % MAX_STRING_SURFACE_WIDTH;
				if (trans_rect.w)
				{
					hd_stretch_surface(src_surface,
							&trans_rect,
							surface,
							&dst_rect);
				}
				if (src_surface != surface)
					hd_add_blit_element(src_surface);
			}

			return (ret);
		}
	}

	return (0);
}

static void _surface_to_image(GuiSurface *surface, image_desc *desc)
{
	desc->hw_accel = GUI_TRUE;
	desc->data = surface->data;
	desc->xoff = 0;
	desc->yoff = 0;
	desc->width = surface->sf.width;
	desc->height = surface->sf.height;
	desc->bpp = surface->bpp;
	desc->fmt = surface->sf.color_format;
	desc->type = BMP_TYPE;
	desc->size = surface->buffer_size;
}

int gxgdi_draw_string(void *screen, void *string, GAL_Rect *rect, GAL_Interval *interval, int alignment, int color, int flag)
{
	GuiCore *gui_core = GUI_GetCurrent();

	struct gdi_font *pFont = NULL;
	struct gdi_font_opt *FontOpt = NULL;
	int height = 0, ret = 0, surface_color = 0, is_right2left = 0, text_len = 0;
	GuiSurface *surface = (GuiSurface *)screen, *src_surface = NULL;
	GuiSurface *tmp_surface = NULL, *tmp_surface0 = NULL, *tmp_surface1 = NULL;
	GAL_Rect src_rect = {0}, trans_rect = {0}, dst_rect = {0}, tmp_rect = {0};
	unsigned int buffer_width = 0, buffer_height = 0, i = 0;
	char *read_buffer = NULL;
	void *p_convert_str = NULL;
	//unsigned short *word_buffer = NULL;

	if (NULL == surface || NULL == string || NULL == rect || NULL == interval)
	{
		//gxlogd("[GUI]Illegal surface\n");
		return (-1);
	}

	//x = rect->x;
	//y = rect->y;
	//width = rect->w;
	height = rect->h;

	pFont = gxgdi_get_font();

	if (NULL == pFont) {
		gxlogd("[GUI]Illegal font::::Please reset the font\n");
		return (-1);
	} else {
		if (pFont->ysize > height) {
			return (-1);
		}
		if (g_font_opt_fun)
		{
			FontOpt = (struct gdi_font_opt *)g_font_opt_fun;
		}
		else
		{
			FontOpt = font_list[pFont->family];
		}

		if (NULL == FontOpt) {
			gxlogd("[GUI]Illegal FontOpt\n");
			return (-1);
		} else {
			surface_color = gal_get_gui_transparent();
			surface_color = gal_color2index(surface, surface_color);
			src_rect = *rect;
			if ((gui.config.enable_double_buffer) &&
				(TTF_FONT_TYPE == pFont->family))
			{
				src_surface = surface;
			}
			else
			{
				src_rect.x = src_rect.y = 0;
				src_rect.w += (pFont->xsize);

				src_surface = gal_get_surface(&src_rect, surface->bpp);
				if (NULL == src_surface)
				{
					gxlogd("[GUI] Create a surface [%d, %d, %d, %d] failed.\n",
							src_rect.x,
							src_rect.y,
							src_rect.w,
							src_rect.h);
					return (-1);
				}

				src_rect.w -= pFont->xsize;
			}

			if ((gui.config.enable_double_buffer) &&
					(TTF_FONT_TYPE == pFont->family))
			{
				gdi_commit();  /*For ttf blend*/
			}
			else
			{
				if (src_rect.w < MAX_STRING_SURFACE_WIDTH)
				{
					gdi_begin();
					hd_fillrect(src_surface, &src_rect, surface_color);
#ifdef GUI_TTF
					if ((gui.config.enable_double_buffer) &&
							(TTF_FONT_TYPE == pFont->family))
					{
						gdi_commit();  /*For ttf blend*/
						gal_copy_surface(screen, rect, src_surface, &src_rect);
					}
#endif
					gdi_end();
				}
				else
				{
					hd_read_surface(src_surface,
							(unsigned int *)&buffer_width,
							(unsigned int *)&buffer_height,
							(void **)&read_buffer);
					if (8 == gui_core->config.bpp)
					{
						memset(read_buffer,
								surface_color,
								buffer_width * buffer_height * gui_core->config.bpp / 8);
					}
					else
					{
#if 0
						unsigned short color_16 = 0;

						color_16 = surface_color;
						color_16 = (color_16 << 8) | ((color_16 >> 8) & 0x00FF);
						word_buffer = (unsigned short *)read_buffer;
						for(i = 0;i < buffer_width * buffer_height; i++)
						{
							*(word_buffer + i) = color_16;
						}
#endif
						hd_fillrect(src_surface, &src_rect, surface_color);
					}
				}
			}

			gdi_begin();
			//src_rect.w -= pFont->xsize;
			if (pFont->family != GB_FAMILY)
			{
#ifdef GUI_STR_ICONV
				p_convert_str = (void *)gdi_iconv_bylocal(NULL, (char *)string);
#else
				p_convert_str = (void *)string;
#endif
			}
			else
			{
				p_convert_str = string;
			}
			if((gui_core->config.font_overlay) &&
			   (32 == src_surface->bpp)) {
				tmp_surface = src_surface;
				src_rect.x = src_rect.y = 0;
				src_surface = hd_get_surface(&src_rect, 8, NULL, GUI_FALSE);
				memset(src_surface->data, 0, src_surface->buffer_size);
				gdi_begin();
			}

			text_len = gdi_text_len(p_convert_str);
			if((SINGLELINE_MODE == flag) && (text_len > src_rect.w)) {
				GAL_Rect rect0 = {0}, rect1 = {0};

				is_right2left = gxgdi_is_right2left(p_convert_str);
				if(is_right2left) {
					tmp_rect = src_rect;

					tmp_rect.x = tmp_rect.y = 0;
					tmp_rect.w = text_len;
					tmp_surface1 = src_surface;
					src_surface = hd_get_surface(&tmp_rect, tmp_surface1->bpp, NULL, GUI_FALSE);
					memset(src_surface->data, 0, src_surface->buffer_size);

					rect0 = src_rect;
					rect1 = rect0;
					rect1.y = 0;
					rect1.x = src_surface->sf.width - rect1.w;
					gdi_begin();
					hd_copy_surface(tmp_surface1, &rect0, src_surface, &rect1);
					gdi_end();
					tmp_rect= src_rect;
					src_rect = rect1;
					src_rect.x = 0;
					src_rect.w = src_surface->sf.width;
					gdi_begin();
				}
			}
			if(!gxgdi_font_is_hwdraw()) {
				gdi_commit();
			}
			ret = (FontOpt->pDrawText)(src_surface,
					p_convert_str,
					&src_rect,
					interval,
					alignment,
					color,
					flag);
			if(!gxgdi_font_is_hwdraw()) {
				hd_sync();
			}
			if(tmp_surface1) {
				GAL_Rect rect0 = {9}, rect1 = {0};

				rect0 = tmp_rect;
				rect1 = rect0;
				rect1.y = 0;
				rect1.x = src_surface->sf.width - rect1.w;
				gdi_end();
				gdi_begin();
				hd_copy_surface(src_surface, &rect1, tmp_surface1, &rect0);
				gdi_end();

				hd_free_surface(src_surface);
				src_surface = tmp_surface1;
				tmp_surface1 = NULL;
			}

			if((gui_core->config.font_overlay) &&
			   (tmp_surface) &&
			   (32 == tmp_surface->bpp)) {
				image_desc img_desc = {0};
				gdi_end();
				tmp_surface0 = hd_get_surface(&src_rect, 32, NULL, GUI_FALSE);
				gdi_begin();
				hd_fillrect(tmp_surface0, &src_rect, 0x00FFFFFF & color);
				_surface_to_image(src_surface, &img_desc);
				img_desc.color = 0xFF000000 | color;
				img_desc.effect = EFFECT_SRC_AND_DST;
				hd_img_desc(tmp_surface0, &img_desc, 0, 0, img_desc.width, img_desc.height);
				dst_rect = src_rect;
				dst_rect.x = rect->x;
				dst_rect.y = rect->y;
				hd_free_surface(src_surface);
				src_surface = tmp_surface;
				tmp_surface0->alpha = 0xFF;
				hd_stretch_surface(tmp_surface0, &src_rect, src_surface, &dst_rect);
				gdi_end();
				hd_free_surface(tmp_surface0);
			}
			if (p_convert_str != string)
			{
				GUI_FREE(p_convert_str);
			}
			gdi_end();

			if (src_rect.w < MAX_STRING_SURFACE_WIDTH)
			{
				if ((gui.config.enable_double_buffer) &&
						(TTF_FONT_TYPE == pFont->family))
				{
					;
				}
				else
				{
					hd_stretch_surface(src_surface, &src_rect, surface, rect);
					hd_add_blit_element(src_surface);
				}
			}
			else
			{
				buffer_width = src_rect.w / MAX_STRING_SURFACE_WIDTH;
				i = 0;
				trans_rect = src_rect;
				dst_rect = *rect;
				dst_rect.w = trans_rect.w = MAX_STRING_SURFACE_WIDTH;

				while (i < buffer_width)
				{
					hd_stretch_surface(src_surface,
							&trans_rect,
							surface,
							&dst_rect);
					hd_add_blit_element(NULL);

					trans_rect.x += MAX_STRING_SURFACE_WIDTH;
					dst_rect.x += MAX_STRING_SURFACE_WIDTH;
					i++;
				}

				trans_rect.w = dst_rect.w = src_rect.w % MAX_STRING_SURFACE_WIDTH;
				if (trans_rect.w)
				{
					hd_stretch_surface(src_surface,
							&trans_rect,
							surface,
							&dst_rect);
				}
				if (src_surface != surface)
					hd_add_blit_element(src_surface);
			}

			return (ret);
		}
	}

	return (0);
}

int gxgdi_is_right2left(const unsigned char *str)
{
	unsigned char *pstr = NULL;
	unsigned int unicode = 0, accent = 0;
	unsigned char is_iso6937 = 0;

	if (NULL == str)
		return GUI_FALSE;

	pstr = (unsigned char *)str;
	while (*pstr)
	{
		gdi_font_utf82unicode(&pstr, &unicode, &accent, &is_iso6937);
		if ((0x0600 <= unicode) && (unicode <= 0x065F))
			return GUI_TRUE;
		else if ((0x0670 <= unicode) && (unicode <= 0x06FF))
			return GUI_TRUE;
		else if ((0x0590 < unicode) && (unicode < 0x05FF))
			return GUI_TRUE;
		else if ((0x0780 < unicode) && (unicode < 0x07BF))
			return GUI_TRUE;
	}

	return GUI_FALSE;
}

int gxgdi_is_arabic(const unsigned char *str)
{
	unsigned char *pstr = NULL;
	unsigned int unicode = 0, accent = 0;
	unsigned char is_iso6937 = 0;

	if (NULL == str)
		return GUI_FALSE;

	pstr = (unsigned char *)str;
	while (*pstr)
	{
		gdi_font_utf82unicode(&pstr, &unicode, &accent, &is_iso6937);
		if ((0x0600 < unicode) && (unicode < 0x06FF))
			return GUI_TRUE;
	}

	return GUI_FALSE;
}

char *gxgdi_string_move(char *string, unsigned int offset, unsigned int *real_offset)
{
	char *pstr = NULL;
	unsigned char is_iso6937 = 0;
	unsigned int unicode = 0, accent = 0;
	int family = 0, i = 0;
	struct gdi_font *pFont = NULL;

	if (0 == offset)
	{
		*real_offset = 0;
		return (string);
	}

	pstr = string;
	pFont = gxgdi_get_font();
	family = pFont->family;
	while (*pstr)
	{
		if (GB_FAMILY == family)
		{
			if (*pstr < 0x80)
				pstr++;
			else
				pstr += 2;
			i++;
		}
		else
		{
			gdi_font_utf82unicode((unsigned char **)&pstr, &unicode, &accent, &is_iso6937);
			i++;
		}

		if (i == offset)
		{
			*real_offset = i;
			break;
		}
	}
	if(i != offset)
	{
		pstr = string;
	}

	return (pstr);
}

int gxgdi_string_num(char *string)
{
	char *pstr = NULL;
	unsigned char is_iso6937 = 0;
	unsigned int unicode = 0, accent = 0;
	int family = 0, i = 0;
	struct gdi_font *pFont = NULL;

	pstr = string;
	pFont = gxgdi_get_font();
	family = pFont->family;
	while (*pstr)
	{
		if (GB_FAMILY == family)
		{
			if (*pstr < 0x80)
				pstr++;
			else
				pstr += 2;
			i++;
		}
		else
		{
			gdi_font_utf82unicode((unsigned char **)&pstr, &unicode, &accent, &is_iso6937);
			i++;
		}
	}

	return (i);
}

char *encoding_name[] =
{
	"iso6937",
	"iso8859-01",
	"iso8859-02",
	"iso8859-03",
	"iso8859-04",
	"iso8859-05",
	"iso8859-06",
	"iso8859-07",
	"iso8859-08",
	"iso8859-09",
	"iso8859-10",
	"iso8859-11",
	"iso8859-12",
	"iso8859-13",
	"iso8859-14",
	"iso8859-15",
	"iso8859-16",
	"ucs2",
	"ksx",
	"gb2312",
	"big5",
	"utf8"
};

static char *_get_standard_name(char *string)
{
	char *pstr = NULL;
	int code = 0;

	pstr = string;
	if(*pstr >= 0x20)
	{
		//iso6937
		code = 0;
	}
	else if(*pstr == 0x10)
	{
		code = *(pstr + 2);
	}
	else if((*pstr > 0) && (*pstr < 0x0F))
	{
		code = (*pstr) + 4;
	}
	else
	{
		//ucs2, ksx, gb2312, big5, utf8
		code = *pstr;
	}

	/*Unstandard data, default is utf8*/
	if(code >= (sizeof(encoding_name) / sizeof(char *)))
	{
		code = sizeof(encoding_name) / sizeof(char *) - 1;
	}

	return (encoding_name[code]);
}

static char *encoding_name_convert[50][2];

static int _get_first_enc_name_index(void)
{
	int i = 0;
	while (i < sizeof(encoding_name_convert) / (sizeof(char *) * 2))
	{
		if (encoding_name_convert[i][0] == NULL)
		{
			break;
		}
		i++;
	}

	return (i);
}

static int _clear_encoding_name(char *enc)
{
	int i = 0;

	while (i < sizeof(encoding_name_convert) / (sizeof(char *) * 2))
	{
		if (encoding_name_convert[i][0])
		{
			GUI_FREE(encoding_name_convert[i][0]);
		}
		if (encoding_name_convert[i][1])
		{
			GUI_FREE(encoding_name_convert[i][1]);
		}

		if (enc)
		{
			encoding_name_convert[i][0] = GxCore_Strdup(encoding_name[i]);
			encoding_name_convert[i][1] = GxCore_Strdup(enc);
		}

		i++;
		if (i >= (sizeof(encoding_name) / sizeof(char *)))
		{
			break;
		}
	}
	return (0);
}

int gdi_encoding_convert(char *src_enc, char *dst_enc)
{
	int i = 0;

	if (src_enc == NULL)
		return (1);

	if (0 == strcasecmp("all", src_enc))
	{
		return (_clear_encoding_name(dst_enc));
	}

	while (i < sizeof(encoding_name_convert) / (sizeof(char *) * 2))
	{
		if ((encoding_name_convert[i][0]) &&
		   (0 == strcasecmp(src_enc, encoding_name_convert[i][0])))
		{
			if (dst_enc == NULL)
			{
				GUI_FREE(encoding_name_convert[i][0]);
				GUI_FREE(encoding_name_convert[i][1]);
			}
			return (0);
		}
		i++;
	}

	if (dst_enc == NULL)
		return (1);

	i = _get_first_enc_name_index();
	if (i >= sizeof(encoding_name_convert) / (sizeof(char *) * 2))
	{
		return (1);
	}

	encoding_name_convert[i][0] = GxCore_Strdup(src_enc);
	encoding_name_convert[i][1] = GxCore_Strdup(dst_enc);

	return (0);
}

char *_get_convert_encoding(char *enc)
{
	int i = 0;

	if (enc == NULL)
		return (NULL);

	while (i < sizeof(encoding_name_convert) / (sizeof(char *) * 2))
	{
		if ((encoding_name_convert[i][0]) && (0 == strcasecmp(enc, encoding_name_convert[i][0])))
		{
			return (encoding_name_convert[i][1]);
		}
		i++;
	}

	return (NULL);
}

int gxgdi_iconv(const unsigned char *from_str,
		unsigned char **to_str,
		unsigned int from_size,
		unsigned int *to_size,
		unsigned char *iso_str)
{
	unsigned char *tmp_str = NULL;
	char *encoding_name = NULL;
	char *standard_str = NULL;
	char *standard_tmp_str = NULL;
	encoding_node *encoding_node = NULL;

	if ((NULL == from_str) || (NULL == to_str))
	{
		return (1);
	}

	standard_str = GxCore_Mallocz(from_size + 2);
	if (NULL == standard_str)
	{
		return (1);
	}

	memcpy(standard_str, from_str, from_size);
	standard_tmp_str = standard_str;

	encoding_name = _get_standard_name((char *)standard_str);
	if (_get_convert_encoding(encoding_name))
	{
		encoding_name = _get_convert_encoding(encoding_name);
	}

	if((0 != strcasecmp("ucs2", encoding_name)) &&
	   (0 != strcasecmp("gb2312", encoding_name)) &&
	   (0 != strcasecmp("big5", encoding_name)) &&
	   (0 != strcasecmp("ksx", encoding_name)))
	{
		if (*standard_str == 0x10)
			standard_tmp_str += 3;
		else if (*standard_str < 0x20)
			standard_tmp_str++;
	} else if(*standard_tmp_str < 0x20) {
		if(*standard_tmp_str == 0x10)
			standard_tmp_str += 3;
		else
			standard_tmp_str++;
	}

	encoding_node = gdi_find_encoding(encoding_name);
	tmp_str = (unsigned char *)gdi_iconv_bylocal(encoding_node, ((char *)standard_tmp_str));
	*to_str = tmp_str;
	*to_size = strlen((char *)tmp_str) + 1;
	GxCore_Free(standard_str);

	return (0);
}

int gdi_roll_string(void *screen, void *string, GAL_Rect* rect, int pos, int color, int flag)
{
	GuiCore *gui_core = NULL;
	struct gdi_font *pFont = NULL;
	struct gdi_font_opt *FontOpt = NULL;
	GuiSurface *surface = (GuiSurface *)screen, *src_surface = NULL;
	void *p_convert_str = NULL;
	GAL_Rect src_rect = {0};
	int ret = 0;

	if (NULL == surface || NULL == string)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();
	pFont = gxgdi_get_font();

	if (NULL == pFont)
	{
		gxlogd("[GUI]Illegal font::::Please reset the font\n");
		return (1);
	}
	else
	{
		if (pFont->ysize > rect->h) {
			return (1);
		}

		if (g_font_opt_fun)
		{
			FontOpt = (struct gdi_font_opt*)g_font_opt_fun;
		}
		else
		{
			FontOpt = font_list[pFont->family];
		}

		if (NULL == FontOpt) {
			gxlogd("[GUI]Illegal FontOpt\n");
			return (1);
		} else {
			if ((gui.config.enable_double_buffer) &&
					(TTF_FONT_TYPE == gui_core->config.pCurrentFont->family))
			{
				src_rect = *rect;
				src_rect.x = src_rect.y = 0;
				src_rect.w += (gui_core->config.pCurrentFont->xsize);
				src_surface = gal_get_surface(&src_rect, surface->bpp);
				if (NULL == src_surface)
				{
					gxlogd("[GUI] Create a surface [%d, %d, %d, %d] failed.\n",
							src_rect.x,
							src_rect.y,
							src_rect.w,
							src_rect.h);
					return (-1);
				}
				src_rect.w -= gui_core->config.pCurrentFont->xsize;

				gdi_commit();
				gal_copy_surface(surface,
						(GAL_Rect *)rect,
						src_surface,
						(GAL_Rect *)&src_rect);

				gdi_begin();
				if (pFont->family != GB_FAMILY)
				{
					p_convert_str = (void *)gdi_iconv_bylocal(NULL, (char *)string);
				}
				else
				{
					p_convert_str = string;
				}
				ret = (FontOpt->pRollText)(src_surface, p_convert_str, &src_rect, pos, color, flag);
				if (p_convert_str != string)
				{
					GUI_FREE(p_convert_str);
				}
				gdi_end();
				hd_stretch_surface(src_surface, &src_rect, surface, rect);
				hd_add_blit_element(src_surface);
			}
			else
			{
				if (pFont->family != GB_FAMILY)
				{
					p_convert_str = (void *)gdi_iconv_bylocal(NULL, (char *)string);
				}
				else
				{
					p_convert_str = string;
				}
				ret = ((FontOpt->pRollText) (surface, p_convert_str, rect, pos, color, flag));
				if (p_convert_str != string)
				{
					GUI_FREE(p_convert_str);
				}
			}
		}
	}

	return (ret);
}

int gdi_text_len(void *string)
{
	struct gdi_font *pFont = NULL;
	struct gdi_font_opt *FontOpt = NULL;
	int ret = 0;
	void *p_convert_str = NULL;

	if (NULL == string) {
		return (1);
	}

	pFont = gxgdi_get_font();
	if (NULL == pFont) {
		gxlogd("[GUI]Illegal font::::Please reset the font\n");
		return (1);
	} else {
		if (g_font_opt_fun)
		{
			FontOpt = (struct gdi_font_opt*)g_font_opt_fun;
		}
		else
		{
			FontOpt = font_list[pFont->family];
		}

		if (NULL == FontOpt) {
			return (1);
		} else {
			if (pFont->family != GB_FAMILY)
			{
#ifdef GUI_STR_ICONV
				p_convert_str = (void *)gdi_iconv_bylocal(NULL, (char *)string);
#else
				p_convert_str = (void *)string;
#endif /*GUI_STR_ICONV*/
			}
			else
			{
				p_convert_str = string;
			}
			ret = FontOpt->pGetLen(p_convert_str);
			if (p_convert_str != string)
			{
				GUI_FREE(p_convert_str);
			}
			return (ret);
		}
	}
}

int gdi_text_surfacewidth(void *string)
{
	int ret = 0;
	struct gdi_font *pFont = NULL;

	pFont = gxgdi_get_font();
	if(NULL == pFont) {
		return (0);
	}

	ret = gdi_text_len(string);

	if(gxgdi_is_right2left(string) && (gxgdi_string_num(string) > 1)) {
		ret += pFont->xsize;
	}

	return (ret);
}

static int _convert_offset(void *pstart, void* pend, void *orig)
{
	char *p_str0 = NULL, *p_str1 = NULL, *p_orig = NULL;
	int count = 0;
	unsigned int unicode = 0, accent = 0;
	unsigned char is_iso = 0;

	if((p_current_encoding) && (p_current_encoding->name) &&
	   (0 == strcasecmp(p_current_encoding->name, "gb2312")))
	{
		p_str0 = (char *)pstart;
		p_str1 = (char *)pend;
		p_orig = (char *)orig;
		count = 0;
		while(p_str0 < p_str1)
		{
			gdi_font_utf82unicode((unsigned char **)&p_str0, &unicode, &accent, &is_iso);
			if(p_str0 > p_str1)
				break;

			if(unicode < 0x80)
				count++;
			else
				count += 2;
		}
		return (count);
	}
	else
	{
		return (pend - pstart);
	}
}

int gdi_text_skip_line(void *p_start, void *string, GAL_Interval *interval, int line_width, int alignment)
{
	struct gdi_font *pFont = NULL;
	struct gdi_font_opt *FontOpt = NULL;
	int ret = 0;
	void *p_convert_str = NULL;

	if (NULL == string || NULL == interval)
	{
		return (0);
	}

	pFont = gxgdi_get_font();
	if (NULL == pFont)
	{
		gxlogd("[GUI]Illegal font::::Please reset the font\n");
		return (0);
	}
	else
	{
		if (g_font_opt_fun)
		{
			FontOpt = (struct gdi_font_opt *)g_font_opt_fun;
		}
		else
		{
			FontOpt = font_list[pFont->family];
		}

		if (NULL == FontOpt)
		{
			return (0);
		}
		else
		{
			if (FontOpt->pSkipLine)
			{
				if (pFont->family != GB_FAMILY)
				{
					p_convert_str = (void *)gdi_iconv_bylocal(NULL, (char *)string);
				}
				else
				{
					p_convert_str = string;
				}
				ret = (FontOpt->pSkipLine(p_start, p_convert_str, interval, line_width, alignment));
				ret = _convert_offset(p_convert_str, p_convert_str + ret, string);
				if(p_convert_str != string)
				{
					GUI_FREE(p_convert_str);
				}
				return (ret);
			}
			else
			{
				return (0);
			}
		}
	}
}

int gdi_text_get_lines(void *string, GAL_Rect *rect, GAL_Interval *interval)
{
	struct gdi_font *pFont = NULL;
	struct gdi_font_opt *FontOpt = NULL;
	int ret = 0;
	void *p_convert_str = NULL;

	if ((NULL == string) || (NULL == rect))
	{
		return (0);
	}

	pFont = gxgdi_get_font();
	if (NULL == pFont)
	{
		gxlogd("[GUI]Illegal font::::Please reset the font\n");
		return (0);
	}
	else
	{
		if (g_font_opt_fun)
		{
			FontOpt = (struct gdi_font_opt *)g_font_opt_fun;
		}
		else
		{
			FontOpt = font_list[pFont->family];
		}

		if (NULL == FontOpt)
		{
			return (0);
		}
		else
		{
			if (FontOpt->pGetLines)
			{
				if (pFont->family != GB_FAMILY)
				{
					p_convert_str = (void *)gdi_iconv_bylocal(NULL, (char *)string);
				}
				else
				{
					p_convert_str = string;
				}
				ret = (FontOpt->pGetLines(string, rect, interval));
				if (p_convert_str != string)
				{
					GUI_FREE(p_convert_str);
				}
				return (ret);
			}
			else
			{
				return (0);
			}
		}
	}
}

extern unsigned short table_iso6937[];
int _get_latin_letter_by_mark(int mark, int letter);

int gdi_font_utf82unicode(unsigned char **pstr, unsigned int *unicode, unsigned int *accent, unsigned char *is_iso6937)
{
	unsigned char *p_ustr = NULL;
	unsigned char temp_char[6] = {0};

	if ((NULL == pstr) || (NULL == unicode) || (NULL == accent) || (NULL == is_iso6937))
	{
		return (1);
	}

	p_ustr = (unsigned char *)(*pstr);
	if (NULL == p_ustr)
	{
		return (1);
	}

	if ('\0' == *p_ustr)
	{
		return (1);
	}

	if (((*p_ustr) & 0x80) == UTF8_0XXXXXXX)
	{
		*unicode = *p_ustr;
		p_ustr++;
	}
	else if (((*p_ustr) & 0xE0) == UTF8_110XXXXX)
	{
		temp_char[0] = *p_ustr & 0x1F;
		temp_char[1] = *(p_ustr + 1);

		if (UTF8_10XXXXXX != (temp_char[1] & UTF8_10XXXXXX))
		{
			if (gdi_check_iso6937(*(p_ustr + 1), *p_ustr))
			{
				*is_iso6937 = GUI_TRUE;
				*accent = table_iso6937[(*p_ustr) - 0xC0];
				*unicode = *(p_ustr + 1);
				p_ustr += 2;
			}
			else
			{
				*unicode = *p_ustr;
				p_ustr++;
			}
		}
		else
		{
			*unicode = (((*p_ustr & 0x1F) << 6) |
					(*(p_ustr + 1) & 0x3F));

			if (('\0' == *(p_ustr + 1)) || ('\0' == *p_ustr))
			{
				p_ustr += 1;
			}
			else
			{
				p_ustr += 2;
			}
		}

		if((*unicode >= 0x0300) && (*unicode <= 0x032F) && (*p_ustr < 0x80)) {
			int convert_code = _get_latin_letter_by_mark((int)*unicode, (int)*p_ustr);
			if(convert_code) {
				if(gxgdi_is_exist(convert_code)) {
					*unicode = convert_code;
					p_ustr++;
				}
			}
		}
	}
	else if (((*p_ustr) & 0xF0) == UTF8_1110XXXX)
	{
		temp_char[1] = *(p_ustr + 1);
		temp_char[2] = *(p_ustr + 2);
		if ((UTF8_10XXXXXX != (temp_char[1] & UTF8_110XXXXX)) ||
				(UTF8_10XXXXXX != (temp_char[2] & UTF8_110XXXXX)))
		{
			if (gdi_check_iso6937(*(p_ustr + 1), *p_ustr))
			{
				*is_iso6937 = GUI_TRUE;
				*accent = table_iso6937[(*p_ustr) - 0xC0];
				*unicode = *(p_ustr + 1);
				p_ustr += 2;
			}
			else
			{
				*unicode = *p_ustr;
				p_ustr++;
			}
		}
		else
		{
			*unicode = (((*p_ustr & 0x0F) << 12) |
					((*(p_ustr + 1) & 0x3F) << 6) |
					(*(p_ustr + 2) & 0x3F));
			p_ustr += 3;
		}
	}
	else if (((*p_ustr) & 0xF8) == UTF8_11110XXX)
	{
		temp_char[1] = *(p_ustr + 1);
		temp_char[2] = *(p_ustr + 2);
		temp_char[3] = *(p_ustr + 3);
		if ((UTF8_10XXXXXX != (temp_char[1] & UTF8_110XXXXX)) ||
				(UTF8_10XXXXXX != (temp_char[2] & UTF8_110XXXXX)) ||
				(UTF8_10XXXXXX != (temp_char[3] & UTF8_110XXXXX)))
		{
			if (gdi_check_iso6937(*(p_ustr + 1), *p_ustr))
			{
				*is_iso6937 = GUI_TRUE;
				*accent = table_iso6937[(*p_ustr) - 0xC0];
				*unicode = *(p_ustr + 1);
				p_ustr += 2;
			}
			else
			{
				*unicode = *p_ustr;
				p_ustr++;
			}
		}
		else
		{
			*unicode = (((*p_ustr & 0x07) << 18) |
					((*(p_ustr + 1) & 0x3F) << 12) |
					((*(p_ustr + 2) & 0x3F) << 6) |
					(*(p_ustr + 3) & 0x3F));

			p_ustr += 4;
		}
	}
	else if (((*p_ustr) & 0xFC) == UTF8_111110XX)
	{
		temp_char[1] = *(p_ustr + 1);
		temp_char[2] = *(p_ustr + 2);
		temp_char[3] = *(p_ustr + 3);
		temp_char[4] = *(p_ustr + 4);
		if ((UTF8_10XXXXXX != (temp_char[1] & UTF8_110XXXXX)) ||
				(UTF8_10XXXXXX != (temp_char[2] & UTF8_110XXXXX)) ||
				(UTF8_10XXXXXX != (temp_char[3] & UTF8_110XXXXX)) ||
				(UTF8_10XXXXXX != (temp_char[4] & UTF8_110XXXXX)))
		{
			if (gdi_check_iso6937(*(p_ustr + 1), *p_ustr))
			{
				*is_iso6937 = GUI_TRUE;
				*accent = table_iso6937[(*p_ustr) - 0xC0];
				*unicode = *(p_ustr + 1);
				p_ustr += 2;
			}
			else
			{
				*unicode = *p_ustr;
				p_ustr++;
			}
		}
		else
		{
			*unicode = (((*p_ustr & 0x03) << 24) |
					((*(p_ustr + 1) & 0x3F) << 18) |
					((*(p_ustr + 2) & 0x3F) << 12) |
					((*(p_ustr + 3) & 0x3F) << 6) |
					(*(p_ustr + 4) & 0x3F));

			p_ustr += 5;
		}
	}
	else if (((*p_ustr) & 0xFC) == UTF8_1111110X)
	{
		temp_char[1] = *(p_ustr + 1);
		temp_char[2] = *(p_ustr + 2);
		temp_char[3] = *(p_ustr + 3);
		temp_char[4] = *(p_ustr + 4);
		temp_char[5] = *(p_ustr + 5);
		if ((UTF8_10XXXXXX != (temp_char[1] & UTF8_110XXXXX)) ||
				(UTF8_10XXXXXX != (temp_char[2] & UTF8_110XXXXX)) ||
				(UTF8_10XXXXXX != (temp_char[3] & UTF8_110XXXXX)) ||
				(UTF8_10XXXXXX != (temp_char[4] & UTF8_110XXXXX)) ||
				(UTF8_10XXXXXX != (temp_char[5] & UTF8_110XXXXX)))
		{
			if (gdi_check_iso6937(*(p_ustr + 1), *p_ustr))
			{
				*is_iso6937 = 1;
				*accent = table_iso6937[(*p_ustr) - 0xC0];
				*unicode = *(p_ustr + 1);
				p_ustr += 2;
			}
			else
			{
				*unicode = *p_ustr;
				p_ustr++;
			}
		}
		else
		{
			*unicode = (((*p_ustr & 0x01) << 30) |
					((*(p_ustr + 1) & 0x3F) << 24) |
					((*(p_ustr + 2) & 0x3F) << 18) |
					((*(p_ustr + 3) & 0x3F) << 12) |
					((*(p_ustr + 4) & 0x3F) << 6) |
					(*(p_ustr + 5) & 0x3F));

			p_ustr += 6;
		}
	}
	else
	{
		*unicode = *p_ustr;//((*p_ustr) << 8) | (*(p_ustr + 1));

		if ((*(p_ustr + 1) == '\0') || (*(p_ustr + 1) == '\n'))
		{
			p_ustr++;
		}
		else
		{
			p_ustr += 2;
		}
	}

	*pstr = p_ustr;

	return (0);
}

static int _find_subset_index(char *fontname)
{
#ifdef GUI_TTF
#ifdef GXDBG
	int count = sizeof(unicode_subset_name) / sizeof(char *);
	int i = 0;

	if(NULL == fontname)
		return (-1);

	for(i = 0; i < count; i++)
	{
		if(0 == strcmp(unicode_subset_name[i], fontname))
			return (i);
	}

#endif /*GXDBG*/
#endif /*GUI_TTF*/
	return (-1);
}

void gxgdi_str_del_char(char *src, char c)
{
	int i = 0, j = 0;

	for(i = 0, j = 0; *(src + i) != '\0'; i++) {
		if(*(src + i) == c) {
			continue;
		} else {
			*(src + j) = *(src + i);
			j++;
		}
	}
	*(src + j)='\0';
}

#ifndef NO_OS
#ifdef GUI_TTF
FT_Error Cache_Glyph(TTF_Font* font, Uint16 ch, int want);
#endif /*GUI_TTF*/
#endif /*NO_OS*/
int GDI_FontCache(char *fontname, char *subset)
{
	int index = 0, first = 0, last = 0, i = 0;
	struct gdi_font *old_font = NULL, *new_font = NULL;

	if((NULL == fontname) || (NULL == subset))
	{
		return (1);
	}

	index = _find_subset_index(subset);
	if(index < 0)
	{
		return (1);
	}

#ifdef GUI_TTF
	first = g_font_arrange[index].First;
	last = g_font_arrange[index].Last;
	if((last - first + 1) > 255)
	{
		return (1);
	}
#endif /*GUI_TTF*/

	old_font = gxgdi_set_font(fontname);
	if(NULL == old_font)
	{
		return (1);
	}

	new_font = gxgdi_get_font();
	if(NULL == new_font)
	{
		return (1);
	}

	if(TTF_FONT_TYPE != new_font->family)
	{
		return (1);
	}

	for(i = first; i <= last; i++)
	{
#ifndef NO_OS
#ifdef GUI_TTF
		Cache_Glyph((TTF_Font *)(new_font->font_data),
					i,
					CACHED_METRICS|CACHED_PIXMAP);
#endif /*GUI_TTF*/
#endif /*NO_OS*/
	}

	gxgdi_set_font(old_font->font_name);

	return (0);
}

#ifdef GUI_TTF
void Flush_Cache( TTF_Font* font );
#endif /*GUI_TTF*/
int GDI_FontUnCache(char *fontname)
{
#ifdef GUI_TTF
	struct gdi_font *old_font = NULL, *new_font = NULL;

	if(NULL == fontname)
	{
		return (1);
	}

	old_font = gxgdi_set_font(fontname);
	if(NULL == old_font)
	{
		return (1);
	}

	new_font = gxgdi_get_font();
	if(NULL == new_font)
	{
		return (1);
	}

	if(TTF_FONT_TYPE != new_font->family)
	{
		return (1);
	}

	Flush_Cache((TTF_Font *)(new_font->font_data));

	gxgdi_set_font(old_font->font_name);

#endif /*GUI_TTF*/
	return (0);
}

#ifndef NO_OS
void GXGDI_FontRegisterGSUB(void)
{
#ifdef GUI_TTF
	FT_LoadTraditional_Library();
	gui.config.gsub_opt = (void *)&g_gsub_opt;
#endif /*GUI_TTF*/
}
#endif /*NO_OS*/


