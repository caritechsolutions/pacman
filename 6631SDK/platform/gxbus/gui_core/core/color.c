#include "hd_gal.h"
#ifndef NO_OS
#include "color.h"
#include "gui.h"

GuiClut* gui_set_color_from_pal(const char *filename)
{
	handle_t file = 0;
	GuiClut *pClut = NULL;
	char data[10];
	int file_len = 0, file_off = 0, color_num = 0, i = 0;

	if (NULL == filename) {
		return (NULL);
	}

	file = GxCore_Open(filename, "r");
	if (0 == file) {
		return (NULL);
	}

	GxCore_Read(file, data, 1, 4);
	if (strncasecmp("RIFF", data, 4) != 0)
		goto out;

	GxCore_Read(file, &file_len, 1, 4);
	GxCore_Read(file, data, 1, 8);

	if (strncasecmp("PAL data", data, 8) != 0)
		goto out;

	GxCore_Read(file, &file_off, 1, 4);

	GxCore_Read(file, data, 1, 4);
	//version = (data[0] << 8) | data[1];
	color_num = (data[3] << 8) | data[2];

	pClut = gui.config.clut;
	if (pClut == NULL) {
		pClut = (GuiClut *)GxCore_Malloc(sizeof(GuiClut));
		if (pClut == NULL)
			goto out;

		memset(pClut, 0, sizeof(GuiClut));
	}

	pClut->num = color_num;
	if (pClut->palette == NULL) {
		pClut->palette = (Color *)GxCore_Malloc(color_num * sizeof(Color));
		if (pClut->palette == NULL) {
			GUI_FREE(pClut);
			pClut = NULL;
			goto out;
		}

		memset(pClut->palette, 0, sizeof(color_num * sizeof(Color)));
	}

	for (i = 0; i < color_num; i++)
		GxCore_Read(file, &(pClut->palette[i]), 4, 1);

out:
	GxCore_Close(file);

	return (pClut);
}

int gdi_set_clut(void *data, int number)
{
	if(NULL == data)
		return (1);

	return hd_set_clut(screen, data, 0, number);
}

void *gdi_get_clut(void)
{
	return (hd_get_clut());
}

static void _color_free(void *data)
{
	;
}

int gui_set_osdalpha_color(Color color, int alpha)
{
	int ret = 0;
	int i = 0, num_color = 0;
	char key[10] = {0};
	GuiClut *clut = NULL;
	Color *pcolor = NULL;

	if((color == gui.config.osd_trans)
			||(color == gui.config.gui_trans))
	{
		return (1);
	}

	clut = gui.config.firstclut;

	if(NULL == gui.config.color_hash)
	{
		gui.config.color_hash = hash_new(10, _color_free);
	}

	sprintf(key, "%d", color);
	hash_add(gui.config.color_hash, key, (void *)alpha);

	while(clut)
	{
		num_color = clut->num;
		pcolor = clut->palette;
		if(NULL == pcolor)
		{
			return (1);
		}

		for(i = 0; i < num_color; i++)
		{
			if(color == (pcolor[i]&0xFFFFFF))
			{
				pcolor[i] |= (alpha&0xFF)<<24;
			}
		}

		clut = clut->next;
	}

	return (ret);
}

int gui_set_alpha(unsigned int value)
{
	return hd_set_alpha(screen, value);
}
#else
#include "mini_gui_private.h"
#include "gui_core.h"
#include "color.h"
extern GuiCore gui;
int hd_set_clut(GuiSurface *surface, Color * colors, int firstcolor, int ncolors);
#endif /*NO_OS*/

u8 R565( u16 clr )
{
	return (clr & 0xF800) >> 8;
}

u8 G565( u16 clr )
{
	return (clr & 0x07E0) >> 3;
}

u8 B565( u16 clr )
{
	return (clr & 0x001F) << 3;
}

u32 RGB32( u8 r, u8 g, u8 b )
{
	return (u32)(r) << 16 | (u32)(g) << 8 | (u32)(b) | 0xFF000000;
}

u32 ARGB32( u8 a, u8 r, u8 g, u8 b )
{
	return (u32)(a) << 24 | (u32)(r) << 16 | (u32)(g) << 8 | (u32)(b);
}


/*------------------------------------------------------------------------------------
 * 8bpp to 16£¬24£¬32
 *-------------------------------------------------------------------------------------*/

u16 _8_to_565(  const  Color  * pal, u16 index )
{
	int b = 0, g = 0, r = 0;
	u16 value = 0;

	r  =  (pal[index] >> 16) & 0x000000FF;
	g  =  (pal[index] >> 8) & 0x000000FF;
	b  =  (pal[index]) & 0x000000FF;

	value = (((u16)(r >> 3) << 11) | ((u16)(g >> 2) << 5 ) |  ((u16)b >> 3 ));
	return (value);
}

void  _8_to_24(  const  Color  *pal, u16 index, u8 *r, u8 *g, u8 *b )
{
	*b  =  pal[index] >> 16;
	*g  =  pal[index] >> 8;
	*r  =  pal[index];
}

u32 _8_to_32(  const  Color  * pal, u16 index )
{
	return (pal[index] | 0xFF000000);
	//return ((pal[index] >> 16) | ((pal[index] >> 8) << 8) | (pal[index] << 16) | 0xFF000000);
}

/*------------------------------------------------------------------------------------
 *16bpp to 8£¬24£¬32
 *-------------------------------------------------------------------------------------*/
u8 _565_to_8( u16 clr )
{
	return PIXEL_GREY((((clr & 0xF800) >> 11 ) * 0xFF ) / 0x1F ,
			(((clr & 0x7E0) >> 5 )  * 0xFF ) / 0x3F ,
			((clr & 0x1F) * 0xFF ) / 0x1F);
}

void  _565_to_24( u16 clr, u8 *r, u8* g, u8* b )
{
	*r = (u8)((clr & 0xF800 ) >> 11 );
	*g = (u8)((clr & 0x07E0 ) >> 5 );
	*b = (u8)((clr & 0x001F ) << 3 );
}

u32 _565_to_32( u16 clr )
{
	return (((clr & 0x001F) << 3 ) | ((clr & 0x07E0) << 5) | ((clr & 0xF800) << 8) | 0xFF000000);
}

/*------------------------------------------------------------------------------------
 * 24bpp to 8£¬16£¬32
 *-------------------------------------------------------------------------------------*/
u8 _24_to_8( u8 r, u8 g, u8 b )
{
	return PIXEL_GREY( b, g, r );
}

u16 _24_to_565( u8 r, u8 g, u8 b )
{
	return (((u16)(r >> 3) << 11) | ((u16)(g >> 2) << 5 ) |  ((u16)b >> 3 ));
}

u32 _24_to_32( u8 r, u8 g, u8 b )
{
	return (0xFF000000 | ((u16)r << 16) | ((u16)g  << 8 ) | (u16)b);
}

/*------------------------------------------------------------------------------------
  32bpp to 8£¬16£¬24
  -------------------------------------------------------------------------------------*/
u8 _32_to_8( u32 clr )
{
	return PIXEL_GREY((clr & 0xFF), ((clr & 0xFF00) >> 8), ((clr & 0xFF0000) >> 16));
}

u16 _32_to_565( u32 clr )
{
	return (u16)(((clr & 0xF8) >> 3) | ((clr & 0xFC00) >> 5) | ((clr & 0xF80000) >> 8 ));
}

void _32_to_24( u32 clr, u8  *r, u8  *g, u8  *b )
{
	*b = (u8)(clr & 0xFF );
	*g = (u8)((clr & 0xFF00 ) >> 8 );
	*r = (u8)((clr & 0xFF0000 ) >> 16 );
}

int gui_set_clut(void *surface, Color * colors, int firstcolor, int ncolors)
{
	int ret = 0;
	int color = 0;
	GAL_Rect rect = {0};

	hd_enable_osd(GUI_FALSE);
	ret = hd_set_clut(surface, colors, firstcolor, ncolors);

	color = gui.config.osd_trans;
	color = gal_color2index(surface, color);
	rect.x = 0;
	rect.y = 0;
	rect.w = gui.config.width;
	rect.h = gui.config.height;

	hd_fillrect(surface, &rect, color);

	gdi_commit();
	hd_enable_osd(GUI_TRUE);

	return (ret);
}

int _check_color_len(const char *str)
{
    const char *pstr = NULL;

    if(NULL == str)
    {
	return (0);
    }

    pstr = str;
    while(*pstr)
    {
	if((',' == *pstr) || (']' == *pstr))
	{
	    break;
	}

	pstr++;
    }

    return (pstr - str);
}

int gui_get_color(const char *color_name, int default_color)
{
	int color = 0, ret_len = 0, i = 0, count = 0;
	GuiCore *gui_core = NULL;
	GuiConfig *config = NULL;
	const char *value = NULL;

	if(NULL == color_name)
	{
		return (default_color);
	}

	gui_core = GUI_GetCurrent();
	if('#' == *color_name)
	{
		color_name++;
		color = hexToInt(color_name);
	}
	else
	{
#ifndef NO_OS
		if((gui_core->resource_config_table) && (gui_core->resource_config_stack)) {
			count = stack_count(gui_core->resource_config_stack);
			for(i = 0; i < count; i++) {
				config = (GuiConfig *)stack_get(gui_core->resource_config_stack, i);
				if(config && config->colors.hash_colors && !config->invalid) {
					value = (const char *)hash_get(config->colors.hash_colors, color_name);
					if(value) {
						color = hexToInt(value);
						ret_len = _check_color_len(value);
						if((gui_core->config.bpp==32) &&
								(ret_len == 6)) {
							color |= 0xFF000000;
						}
						return (color);
					}
				}
			}
		}
		if(gui_core->config.colors.hash_colors)
		{
			value = (const char *)hash_get(gui_core->config.colors.hash_colors, color_name);
			if(value)
			{
				color = hexToInt(value);
				ret_len = _check_color_len(value);
				if((gui_core->config.bpp==32) &&
				   (ret_len == 6)) {
					color |= 0xFF000000;
				}
			}
		}
#endif /*NO_OS*/
	}

	return (color);
}

int hexToInt(const char *string)
{
	int ret = 0;
	char *pstr = NULL;

	if (NULL == string) {
		return (0);
	}

	pstr = (char *)string;

	while (*pstr) {
		if (*pstr >= '0' && *pstr <= '9') {
			ret = ret * 16 + ((int)*pstr - '0');
		} else {
			if (*pstr >= 'A' && *pstr <= 'F') {
				ret = ret * 16 + ((int)*pstr - 'A' + 10);
			} else if (*pstr >= 'a' && *pstr <= 'f') {
				ret = ret * 16 + ((int)*pstr - 'a' + 10);
			}
		}

		pstr++;
	}

	return (ret);
}

int getColorFromString(const char *input, int *color)
{
	GuiCore *gui_core = NULL;
	char *pstr = NULL, *pval = NULL;
	char value[30] = { 0 }, *name_value = NULL;
	int i = 0, namelen = 0;
	int ret_len = 0;

	gui_core = GUI_GetCurrent();
	pstr = (char *)input;

	namelen = strlen(input);
	name_value = GUI_MALLOCZ(namelen + 1);
	if(NULL == name_value)
		return (0);
	while(*pstr)
	{
		if((',' == *pstr) || ('[' == *pstr))
		{
			pstr++;
		}
		else if((']' == *pstr) || ('\0' == *pstr))
		{
			break;
		}
		else if('#' == *pstr)
		{
			pstr++;
			ret_len = _check_color_len(pstr);
			strncpy(value, pstr, ret_len);
			pstr += ret_len;
			color[i] = hexToInt(value);
			if((gui_core->config.bpp==32) &&
			   (ret_len == 6)) {
				color[i] |= 0xFF000000;
			}
			i++;
		}
		else
		{
			pval = name_value;
			while((',' != *pstr) && (']' != *pstr) && ('\0' != *pstr))
			{
				*pval++ = *pstr++;
			}
			*pval = '\0';
			color[i] = gui_get_color(name_value, 0);
			i++;
		}
		memset(name_value, 0, namelen + 1);
	}
	GUI_FREE(name_value);

	return (0);
}

