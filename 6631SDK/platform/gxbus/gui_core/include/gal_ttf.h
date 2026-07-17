#ifndef __GAL_TTF_H__
#define __GAL_TTF_H__

#ifdef GUI_TTF

#include "ft2build.h"
#ifdef NDS_SUPPORT
#include "freetype.h"
#else
#include "freetype/freetype.h"
#endif
#ifndef NO_OS
#else
#include "gui_hash.h"
#endif /*NO_OS*/
#include "IMG.h"

#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_IDS_H

/* FIXME: Right now we assume the gray-scale renderer Freetype is using
   supports 256 shades of gray, but we should instead key off of num_grays
   in the result FT_Bitmap after the FT_Render_Glyph() call. */
#define NUM_GRAYS       256

/* We'll use SDL for reporting errors */
//#define TTF_SetError	SDL_SetError
//#define TTF_GetError	SDL_GetError

/* Handy routines for converting from fixed point */
#define FT_FLOOR(X)	((X & -64) / 64)
#define FT_CEIL(X)	(((X + 63) & -64) / 64)
#define CACHED_METRICS	0x10
#define CACHED_BITMAP	0x01
#define CACHED_PIXMAP	0x02

/* Set and retrieve the font style
   This font style is implemented by modifying the font glyphs, and
   doesn't reflect any inherent properties of the truetype font file.
*/
#define TTF_STYLE_NORMAL	0x00
#define TTF_STYLE_BOLD		0x01
#define TTF_STYLE_ITALIC	0x02
#define TTF_STYLE_UNDERLINE	0x04

/* ZERO WIDTH NO-BREAKSPACE (Unicode byte order mark) */
#define UNICODE_BOM_NATIVE	0xFEFF
#define UNICODE_BOM_SWAPPED	0xFFFE

#define TTF_HANDLE_STYLE_BOLD(font) (((font)->style & TTF_STYLE_BOLD) && \
									!((font)->face_style & TTF_STYLE_BOLD))

/* Cached glyph information */
typedef struct cached_glyph {
	int stored;
	FT_UInt index;
	FT_Bitmap bitmap;
	FT_Bitmap pixmap;
	int minx;
	int maxx;
	int miny;
	int maxy;
	int yoffset;
	int advance;
	unsigned short cached;
} c_glyph;

/* The structure used to hold internal font information */
struct _TTF_Font {
	/* Freetype2 maintains all sorts of useful info itself */
	FT_Face face;

	/* We'll cache these ourselves */
	int height;
	int ascent;
	int descent;
	int lineskip;

	/* The font style */
	int face_style;
	int style;

	/* Extra width in glyph bounds for text styles */
	int glyph_overhang;
	float glyph_italics;

	/* Information in the font for underlining */
	int underline_offset;
	int underline_height;

	/* Cache for style-transformed glyphs */
	c_glyph *current;
	c_glyph *subscript;
	c_glyph cache[256];
	c_glyph scratch;

	c_glyph extracache[256];
	c_glyph subcache[256];
	c_glyph arabic_base[256];
	c_glyph arabic_present_b[256];
	c_glyph extra_devanagari;
	hash_t  *extra_hash;

	/*CJK Unified Ideographs*/
	hash_t *cjk_hash;

	/*Cache start code */
	unsigned int start_code;
	unsigned int arabic_presentb;

	/* We are responsible for closing the font stream */
	handle_t src;
	int freesrc;
	FT_Open_Args args;

	/*Special font glyph*/
	c_glyph *null_glyph;

	/* For non-scalable formats, we must remember which font index size */
	int font_size_family;
};

/* The internal structure containing font information */
typedef struct _TTF_Font TTF_Font;

int TTF_Init( void );
TTF_Font* TTF_OpenFont( const char *file, int ptsize );
void TTF_CloseFont( TTF_Font* font );
int TTF_SizeSubScript(TTF_Font *font, const unsigned int *text, int *w, int *h, int sub_index);
int TTF_SizeUNICODE(TTF_Font *font, const unsigned int *text, int *w, int *h);
int TTF_SizeNAME(TTF_Font *font, char *text, int *w, int *h);
int TTF_GlyphMetrics(TTF_Font *font, const unsigned int ch, int* minx,
						int* maxx,int* miny, int* maxy, int* advance);
int TTF_GlyphMetrics_Byname(TTF_Font *font, char *ch, int* minx,
						int* maxx,int* miny, int* maxy, int* advance);
image_desc *ttf_draw_char(void *surface,
                          TTF_Font *font,
                          const unsigned int *word,
                          int x,
                          int y,
                          int bpp,
                          int color);
int ttf_get_xoff(unsigned int c);
image_desc *ttf_draw_char_byname(void *surface,
                          TTF_Font *font,
                          char *word,
                          int x,
                          int y,
                          int bpp,
                          int color);
image_desc *ttf_draw_sub_byname(void *surface,
							TTF_Font *font,
							char *word,
							int x,
							int y,
							int bpp,
							int color);
image_desc *ttf_draw_subscript(void *surface,
		TTF_Font *font,
		const unsigned int *word,
		int x,
		int y,
		int bpp,
		int color,
		int sub_index);
void ttf_get_bearingheight(TTF_Font *font, int c, int *yoffset, int *height);
bool TTF_IsExist(TTF_Font *font, unsigned int ch);
void TTF_Quit( void );
#endif

#endif
