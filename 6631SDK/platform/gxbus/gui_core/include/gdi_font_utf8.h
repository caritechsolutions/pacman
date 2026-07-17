#ifndef __GDI_FONT_UTF8_H__
#define __GDI_FONT_UTF8_H__
#include "gdi_font.h"

typedef struct
{
	unsigned char Start;
	unsigned short End;
}ArabicMap;

typedef struct
{
	unsigned int Key;
	unsigned int Isolated;
	unsigned int Final;
	unsigned int Initial;
	unsigned int Medial;
}KEY_INFO;


extern struct gdi_font_opt font_utf8_opt;
extern struct gdi_font_opt font_turki_opt;

unsigned int *get_iso2unicode_table(int code);

#endif


