#ifndef NO_OS
#include "gui_private.h"
#include "gui.h"
#else
#include "mini_gui.h"
#include "mini_gui_private.h"
#endif /*NO_OS*/
#include "gdi_font.h"
#include "gal_ttf.h"
#include <string.h>
#include <math.h>
//#include <sys/stat.h>
#include "gui_hash.h"
#include "SDL.h"
#include "SDL_endian.h"
#include "freetype/ftmodapi.h"

/* The FreeType font engine/library */
static FT_Library library;
static int TTF_initialized = 0;
static int TTF_byteswapped = 0;

static unsigned long RWread(
		FT_Stream stream,
		unsigned long offset,
		unsigned char* buffer,
		unsigned long count
		)
{
	handle_t src;

	src = (handle_t)stream->descriptor.pointer;
	GxCore_Seek( src, (int)offset, SEEK_SET );//SDL_RWseek( src, (int)offset, SEEK_SET );
	if ( count == 0 ) {
		return (0);
	}

	if(GxCore_Read(src, buffer, (int)count, 1))
	{
		return (count);
	}
	else
	{
		return (0);
	}
}

static void Flush_Glyph( c_glyph* glyph )
{
	glyph->stored = 0;
	glyph->index = 0;
	if( glyph->bitmap.buffer ) {
		GUI_FREE( glyph->bitmap.buffer );
		glyph->bitmap.buffer = 0;
	}
	if( glyph->pixmap.buffer ) {
		GUI_FREE( glyph->pixmap.buffer );
		glyph->pixmap.buffer = 0;
	}
	glyph->cached = 0;
}

void Flush_Cache( TTF_Font* font )
{
	int i;
	int size = sizeof( font->cache ) / sizeof( font->cache[0] );

	for( i = 0; i < size; ++i ) {
		if( font->cache[i].cached ) {
			Flush_Glyph( &font->cache[i] );
		}

	}
	if( font->scratch.cached ) {
		Flush_Glyph( &font->scratch );
	}

	size = sizeof( font->subcache ) / sizeof( font->subcache[0] );
	for(i = 0; i < size; ++i) {
		if( font->subcache[i].cached) {
			Flush_Glyph( &font->subcache[i]);
		}
	}

	if( font->extra_devanagari.cached ) {
		Flush_Glyph( &font->extra_devanagari);
	}
}

static void Flush_ExtraCache( TTF_Font* font )
{
	int i;
	int size = sizeof(font->extracache) / sizeof(font->extracache[0]);
	for( i = 0; i < size; ++i)
	{
		if( font->extracache[i].cached ){
			Flush_Glyph( &font->extracache[i] );
		}
	}
	if( font->scratch.cached ) {
		Flush_Glyph( &font->scratch );
	}
}

static void Flush_ArabicBCache( TTF_Font* font )
{
	int i;
	int size = sizeof(font->arabic_present_b) / sizeof(font->arabic_present_b[0]);
	for( i = 0; i < size; ++i)
	{
		if( font->arabic_present_b[i].cached ){
			Flush_Glyph( &font->arabic_present_b[i] );
		}
	}
	if( font->scratch.cached ) {
		Flush_Glyph( &font->scratch );
	}
}

static void Free_ArabicBCache( TTF_Font* font )
{
	int i;
	int size = sizeof(font->arabic_present_b) / sizeof(font->arabic_present_b[0]);
	if(font->arabic_presentb == 0)
	    return;
	for( i = 0; i < size; ++i)
	{
		if( font->arabic_present_b[i].cached ){
		    GUI_FREE( font->arabic_present_b[i].bitmap.buffer );
		    GUI_FREE( font->arabic_present_b[i].pixmap.buffer );
		    font->arabic_present_b[i].cached = 0;
		    memset(&(font->arabic_present_b[i]), 0, sizeof(font->arabic_present_b[0]));
		}
	}

	font->arabic_presentb = 0;
}

static void Free_ArabicBaseCache( TTF_Font* font )
{
	int i;
	int size = sizeof(font->arabic_base) / sizeof(font->arabic_base[0]);
	if(font->arabic_presentb == 0)
	    return;
	for( i = 0; i < size; ++i)
	{
		if( font->arabic_base[i].cached ){
		    GUI_FREE( font->arabic_base[i].bitmap.buffer );
		    GUI_FREE( font->arabic_base[i].pixmap.buffer );
		    font->arabic_base[i].cached = 0;
		    memset(&(font->arabic_base[i]), 0, sizeof(font->arabic_base[0]));
		}
	}
}

#if 0
static void TTF_SetFTError(const char *msg, FT_Error error)
{
#ifdef USE_FREETYPE_ERRORS
#undef FTERRORS_H
#define FT_ERRORDEF( e, v, s )  { e, s },
	static const struct
	{
		int          err_code;
		const char*  err_msg;
	} ft_errors[] = {
#include <freetype/fterrors.h>
	};
	int i;
	const char *err_msg;
	char buffer[1024];

	err_msg = NULL;
	for ( i=0; i<((sizeof ft_errors)/(sizeof ft_errors[0])); ++i ) {
		if ( error == ft_errors[i].err_code ) {
			err_msg = ft_errors[i].err_msg;
			break;
		}
	}
	if ( ! err_msg ) {
		err_msg = "unknown FreeType error";
	}
	sprintf(buffer, "%s: %s", msg, err_msg);
	//TTF_SetError(buffer);
#else
	//TTF_SetError(msg);
#endif /* USE_FREETYPE_ERRORS */
}
#endif

static int _mem_cmp(void *buffer, char c, size_t size)
{
    int i = 0;
    char *ptr = (char *)buffer;

    for(i = 0; i < size; i++)
    {
	if(c != *(ptr + i))
	    return 0;
    }

    return 1;
}

static FT_Error Load_Subscript( TTF_Font* font, Uint32 ch, c_glyph* cached, int want , int sub_index)
{
	FT_Face face;
	FT_Error error;
	FT_GlyphSlot glyph;
	FT_Glyph_Metrics* metrics;
	FT_Outline* outline;

	if ( !font || !font->face ) {
		return FT_Err_Invalid_Handle;
	}
	REPEAT:

	face = font->face;

	/* Load the glyph */
	cached->stored = 0;
	cached->index = 0;
	if ( ! cached->index ) {
		cached->index = FT_Get_Char_Index( face, ch ) + sub_index;
	}
	error = FT_Load_Glyph( face, cached->index, FT_LOAD_DEFAULT );
	if( error ) {
		return error;
	}

	/* Get our glyph shortcuts */
	glyph = face->glyph;
	metrics = &glyph->metrics;
	outline = &glyph->outline;

	/* Get the glyph metrics if desired */
	if ( (want & CACHED_METRICS) && !(cached->stored & CACHED_METRICS) ) {
		if ( FT_IS_SCALABLE( face ) ) {
			/* Get the bounding box */
			cached->minx = FT_FLOOR(metrics->horiBearingX);
			cached->maxx = cached->minx + FT_CEIL(metrics->width);
			cached->maxy = FT_FLOOR(metrics->horiBearingY);
			cached->miny = cached->maxy - FT_CEIL(metrics->height);
			cached->yoffset = font->ascent - cached->maxy;
			cached->advance = FT_CEIL(metrics->horiAdvance);
		} else {
			/* Get the bounding box for non-scalable format.
			 * Again, freetype2 fills in many of the font metrics
			 * with the value of 0, so some of the values we
			 * need must be calculated differently with certain
			 * assumptions about non-scalable formats.
			 * */
			cached->minx = FT_FLOOR(metrics->horiBearingX);
			cached->maxx = cached->minx + FT_CEIL(metrics->horiAdvance);
			cached->maxy = FT_FLOOR(metrics->horiBearingY);
			cached->miny = cached->maxy - FT_CEIL(face->available_sizes[font->font_size_family].height);
			cached->yoffset = 0;
			cached->advance = FT_CEIL(metrics->horiAdvance);
		}

		/* Adjust for bold and italic text */
		if( font->style & TTF_STYLE_BOLD ) {
			cached->maxx += font->glyph_overhang;
		}
		if( font->style & TTF_STYLE_ITALIC ) {
			cached->maxx += (int)ceil(font->glyph_italics);
		}
		cached->stored |= CACHED_METRICS;
	}

	if ( ((want & CACHED_BITMAP) && !(cached->stored & CACHED_BITMAP)) ||
			((want & CACHED_PIXMAP) && !(cached->stored & CACHED_PIXMAP)) ) {
		int mono = (want & CACHED_BITMAP);
		int i;
		FT_Bitmap* src;
		FT_Bitmap* dst;

		/* Handle the italic style */
		if( font->style & TTF_STYLE_ITALIC ) {
			FT_Matrix shear;

			shear.xx = 1 << 16;
			shear.xy = (int) ( font->glyph_italics * ( 1 << 16 ) ) / font->height;
			shear.yx = 0;
			shear.yy = 1 << 16;

			FT_Outline_Transform( outline, &shear );
		}

		/* Render the glyph */
		if ( mono ) {
			error = FT_Render_Glyph( glyph, ft_render_mode_mono );
		} else {
			error = FT_Render_Glyph( glyph, ft_render_mode_normal );
			if(0x20 != ch)
			{
			    if((glyph->bitmap.rows) &&
			       (glyph->bitmap.pitch) &&
			       (glyph->bitmap.buffer) &&
			       (_mem_cmp(glyph->bitmap.buffer, 0, glyph->bitmap.rows * glyph->bitmap.pitch)))
				goto REPEAT;
			}
		}
		if( error ) {
			return error;
		}

		/* Copy over information to cache */
		src = &glyph->bitmap;
		if ( mono ) {
			dst = &cached->bitmap;
		} else {
			dst = &cached->pixmap;
		}
		memcpy( dst, src, sizeof( *dst ) );

		/* FT_Render_Glyph() and .fon fonts always generate a
		 * two-color (black and white) glyphslot surface, even
		 * when rendered in ft_render_mode_normal. */
		/* FT_IS_SCALABLE() means that the font is in outline format,
		 * but does not imply that outline is rendered as 8-bit
		 * grayscale, because embedded bitmap/graymap is preferred
		 * (see FT_LOAD_DEFAULT section of FreeType2 API Reference).
		 * FT_Render_Glyph() canreturn two-color bitmap or 4/16/256-
		 * color graymap according to the format of embedded bitmap/
		 * graymap. */
		if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO ) {
			dst->pitch *= 8;
		} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY2 ) {
			dst->pitch *= 4;
		} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY4 ) {
			dst->pitch *= 2;
		}

		/* Adjust for bold and italic text */
		if( font->style & TTF_STYLE_BOLD ) {
			int bump = font->glyph_overhang;
			dst->pitch += bump;
			dst->width += bump;
		}
		if( font->style & TTF_STYLE_ITALIC ) {
			int bump = (int)ceil(font->glyph_italics);
			dst->pitch += bump;
			dst->width += bump;
		}

		if (dst->rows != 0) {
			dst->buffer = (unsigned char *)GUI_MALLOCZ( dst->pitch * dst->rows );
			if( !dst->buffer ) {
				return FT_Err_Out_Of_Memory;
			}
			memset( dst->buffer, 0, dst->pitch * dst->rows );

			for( i = 0; i < src->rows; i++ ) {
				int soffset = i * src->pitch;
				int doffset = i * dst->pitch;
				if ( mono ) {
					unsigned char *srcp = src->buffer + soffset;
					unsigned char *dstp = dst->buffer + doffset;
					int j;
					if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO ) {
						for ( j = 0; j < src->width; j += 8 ) {
							unsigned char ch = *srcp++;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
						}
					}  else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY2 ) {
						for ( j = 0; j < src->width; j += 4 ) {
							unsigned char ch = *srcp++;
							*dstp++ = (((ch&0xA0) >> 6) >= 0x2) ? 1 : 0;
							ch <<= 2;
							*dstp++ = (((ch&0xA0) >> 6) >= 0x2) ? 1 : 0;
							ch <<= 2;
							*dstp++ = (((ch&0xA0) >> 6) >= 0x2) ? 1 : 0;
							ch <<= 2;
							*dstp++ = (((ch&0xA0) >> 6) >= 0x2) ? 1 : 0;
						}
					} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY4 ) {
						for ( j = 0; j < src->width; j += 2 ) {
							unsigned char ch = *srcp++;
							*dstp++ = (((ch&0xF0) >> 4) >= 0x8) ? 1 : 0;
							ch <<= 4;
							*dstp++ = (((ch&0xF0) >> 4) >= 0x8) ? 1 : 0;
						}
					} else {
						for ( j = 0; j < src->width; j++ ) {
							unsigned char ch = *srcp++;
							*dstp++ = (ch >= 0x80) ? 1 : 0;
						}
					}
				} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO ) {
					/* This special case wouldn't
					 * be here if the FT_Render_Glyph()
					 * function wasn't buggy when it tried
					 * to render a .fon font with 256
					 * shades of gray.  Instead, it
					 * returns a black and white surface
					 * and we have to translate it back
					 * to a 256 gray shaded surface.
					 * */
					unsigned char *srcp = src->buffer + soffset;
					unsigned char *dstp = dst->buffer + doffset;
					unsigned char ch;
					int j, k;
					for ( j = 0; j < src->width; j += 8) {
						ch = *srcp++;
						for (k = 0; k < 8; ++k) {
							if ((ch&0x80) >> 7) {
								*dstp++ = NUM_GRAYS - 1;
							} else {
								*dstp++ = 0x00;
							}
							ch <<= 1;
						}
					}
				} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY2 ) {
					unsigned char *srcp = src->buffer + soffset;
					unsigned char *dstp = dst->buffer + doffset;
					unsigned char ch;
					int j, k;
					for ( j = 0; j < src->width; j += 4 ) {
						ch = *srcp++;
						for ( k = 0; k < 4; ++k ) {
							if ((ch&0xA0) >> 6) {
								*dstp++ = NUM_GRAYS * ((ch&0xA0) >> 6) / 3 - 1;
							} else {
								*dstp++ = 0x00;
							}
							ch <<= 2;
						}
					}
				} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY4 ) {
					unsigned char *srcp = src->buffer + soffset;
					unsigned char *dstp = dst->buffer + doffset;
					unsigned char ch;
					int j, k;
					for ( j = 0; j < src->width; j += 2 ) {
						ch = *srcp++;
						for ( k = 0; k < 2; ++k ) {
							if ((ch&0xF0) >> 4) {
								*dstp++ = NUM_GRAYS * ((ch&0xF0) >> 4) / 15 - 1;
							} else {
								*dstp++ = 0x00;
							}
							ch <<= 4;
						}
					}
				} else {
					memcpy(dst->buffer+doffset,
							src->buffer+soffset, src->pitch);
				}
			}
		}

		/* Handle the bold style */
		if ( font->style & TTF_STYLE_BOLD ) {
			int row;
			int col;
			int offset;
			int pixel;
			Uint8* pixmap;

			/* The pixmap is a little hard, we have to add and clamp */
			for( row = dst->rows - 1; row >= 0; --row ) {
				pixmap = (Uint8*) dst->buffer + row * dst->pitch;
				for( offset=1; offset <= font->glyph_overhang; ++offset ) {
					for( col = dst->width - 1; col > 0; --col ) {
						if( mono ) {
							pixmap[col] |= pixmap[col-1];
						} else {
							pixel = (pixmap[col] + pixmap[col-1]);
							if( pixel > NUM_GRAYS - 1 ) {
								pixel = NUM_GRAYS - 1;
							}
							pixmap[col] = (Uint8) pixel;
						}
					}
				}
			}
		}

		/* Mark that we rendered this format */
		if ( mono ) {
			cached->stored |= CACHED_BITMAP;
		} else {
			cached->stored |= CACHED_PIXMAP;
		}
	}

	/* We're done, mark this glyph cached */
	cached->cached = ch;

	return 0;
}

static FT_Error Load_Glyph( TTF_Font* font, Uint32 ch, c_glyph* cached, int want )
{
	FT_Face face;
	FT_Error error;
	FT_GlyphSlot glyph;
	FT_Glyph_Metrics* metrics;
	FT_Outline* outline;

	if ( !font || !font->face ) {
		return FT_Err_Invalid_Handle;
	}
	REPEAT:

	face = font->face;

	/* Load the glyph */
	if ( ! cached->index ) {
		cached->index = FT_Get_Char_Index( face, ch );
	}
	error = FT_Load_Glyph( face, cached->index, FT_LOAD_DEFAULT );
	if( error ) {
		return error;
	}

	/* Get our glyph shortcuts */
	glyph = face->glyph;
	metrics = &glyph->metrics;
	outline = &glyph->outline;

	/* Get the glyph metrics if desired */
	if ( (want & CACHED_METRICS) && !(cached->stored & CACHED_METRICS) ) {
		if ( FT_IS_SCALABLE( face ) ) {
			/* Get the bounding box */
			cached->minx = FT_FLOOR(metrics->horiBearingX);
			cached->maxx = cached->minx + FT_CEIL(metrics->width);
			cached->maxy = FT_FLOOR(metrics->horiBearingY);
			cached->miny = cached->maxy - FT_CEIL(metrics->height);
			cached->yoffset = font->ascent - cached->maxy;
			cached->advance = FT_CEIL(metrics->horiAdvance);
		} else {
			/* Get the bounding box for non-scalable format.
			 * Again, freetype2 fills in many of the font metrics
			 * with the value of 0, so some of the values we
			 * need must be calculated differently with certain
			 * assumptions about non-scalable formats.
			 * */
			cached->minx = FT_FLOOR(metrics->horiBearingX);
			cached->maxx = cached->minx + FT_CEIL(metrics->horiAdvance);
			cached->maxy = FT_FLOOR(metrics->horiBearingY);
			cached->miny = cached->maxy - FT_CEIL(face->available_sizes[font->font_size_family].height);
			cached->yoffset = 0;
			cached->advance = FT_CEIL(metrics->horiAdvance);
		}

		/* Adjust for bold and italic text */
		if( font->style & TTF_STYLE_BOLD ) {
			cached->maxx += font->glyph_overhang;
		}
		if( font->style & TTF_STYLE_ITALIC ) {
			cached->maxx += (int)ceil(font->glyph_italics);
		}
		cached->stored |= CACHED_METRICS;
	}

	if ( ((want & CACHED_BITMAP) && !(cached->stored & CACHED_BITMAP)) ||
			((want & CACHED_PIXMAP) && !(cached->stored & CACHED_PIXMAP)) ) {
		int mono = (want & CACHED_BITMAP);
		int i;
		FT_Bitmap* src;
		FT_Bitmap* dst;

		/* Handle the italic style */
		if( font->style & TTF_STYLE_ITALIC ) {
			FT_Matrix shear;

			shear.xx = 1 << 16;
			shear.xy = (int) ( font->glyph_italics * ( 1 << 16 ) ) / font->height;
			shear.yx = 0;
			shear.yy = 1 << 16;

			FT_Outline_Transform( outline, &shear );
		}

		/* Render the glyph */
		if ( mono ) {
			error = FT_Render_Glyph( glyph, ft_render_mode_mono );
		} else {
			error = FT_Render_Glyph( glyph, ft_render_mode_normal );
			if(0x20 != ch)
			{
			    if((glyph->bitmap.rows) &&
			       (glyph->bitmap.pitch) &&
			       (glyph->bitmap.buffer) &&
			       (_mem_cmp(glyph->bitmap.buffer, 0, glyph->bitmap.rows * glyph->bitmap.pitch)))
				goto REPEAT;
			}
		}
		if( error ) {
			return error;
		}

		/* Copy over information to cache */
		src = &glyph->bitmap;
		if ( mono ) {
			dst = &cached->bitmap;
		} else {
			dst = &cached->pixmap;
		}
		memcpy( dst, src, sizeof( *dst ) );

		/* FT_Render_Glyph() and .fon fonts always generate a
		 * two-color (black and white) glyphslot surface, even
		 * when rendered in ft_render_mode_normal. */
		/* FT_IS_SCALABLE() means that the font is in outline format,
		 * but does not imply that outline is rendered as 8-bit
		 * grayscale, because embedded bitmap/graymap is preferred
		 * (see FT_LOAD_DEFAULT section of FreeType2 API Reference).
		 * FT_Render_Glyph() canreturn two-color bitmap or 4/16/256-
		 * color graymap according to the format of embedded bitmap/
		 * graymap. */
		if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO ) {
			dst->pitch *= 8;
		} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY2 ) {
			dst->pitch *= 4;
		} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY4 ) {
			dst->pitch *= 2;
		}

		/* Adjust for bold and italic text */
		if( font->style & TTF_STYLE_BOLD ) {
			int bump = font->glyph_overhang;
			dst->pitch += bump;
			dst->width += bump;
		}
		if( font->style & TTF_STYLE_ITALIC ) {
			int bump = (int)ceil(font->glyph_italics);
			dst->pitch += bump;
			dst->width += bump;
		}

		if (dst->rows != 0) {
			int pitch = 0, rows = 0;

			pitch = src->pitch > dst->pitch ? src->pitch : dst->pitch;
			rows = src->rows > dst->rows ? src->rows : dst->rows;
			dst->buffer = (unsigned char *)GUI_MALLOCZ( pitch * rows );
			if( !dst->buffer ) {
				return FT_Err_Out_Of_Memory;
			}
			memset( dst->buffer, 0, dst->pitch * dst->rows );

			for( i = 0; i < src->rows; i++ ) {
				int soffset = i * src->pitch;
				int doffset = i * dst->pitch;
				if ( mono ) {
					unsigned char *srcp = src->buffer + soffset;
					unsigned char *dstp = dst->buffer + doffset;
					int j;
					if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO ) {
						for ( j = 0; j < src->width; j += 8 ) {
							unsigned char ch = *srcp++;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
						}
					}  else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY2 ) {
						for ( j = 0; j < src->width; j += 4 ) {
							unsigned char ch = *srcp++;
							*dstp++ = (((ch&0xA0) >> 6) >= 0x2) ? 1 : 0;
							ch <<= 2;
							*dstp++ = (((ch&0xA0) >> 6) >= 0x2) ? 1 : 0;
							ch <<= 2;
							*dstp++ = (((ch&0xA0) >> 6) >= 0x2) ? 1 : 0;
							ch <<= 2;
							*dstp++ = (((ch&0xA0) >> 6) >= 0x2) ? 1 : 0;
						}
					} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY4 ) {
						for ( j = 0; j < src->width; j += 2 ) {
							unsigned char ch = *srcp++;
							*dstp++ = (((ch&0xF0) >> 4) >= 0x8) ? 1 : 0;
							ch <<= 4;
							*dstp++ = (((ch&0xF0) >> 4) >= 0x8) ? 1 : 0;
						}
					} else {
						for ( j = 0; j < src->width; j++ ) {
							unsigned char ch = *srcp++;
							*dstp++ = (ch >= 0x80) ? 1 : 0;
						}
					}
				} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO ) {
					/* This special case wouldn't
					 * be here if the FT_Render_Glyph()
					 * function wasn't buggy when it tried
					 * to render a .fon font with 256
					 * shades of gray.  Instead, it
					 * returns a black and white surface
					 * and we have to translate it back
					 * to a 256 gray shaded surface.
					 * */
					unsigned char *srcp = src->buffer + soffset;
					unsigned char *dstp = dst->buffer + doffset;
					unsigned char ch;
					int j, k;
					for ( j = 0; j < src->width; j += 8) {
						ch = *srcp++;
						for (k = 0; k < 8; ++k) {
							if ((ch&0x80) >> 7) {
								*dstp++ = NUM_GRAYS - 1;
							} else {
								*dstp++ = 0x00;
							}
							ch <<= 1;
						}
					}
				} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY2 ) {
					unsigned char *srcp = src->buffer + soffset;
					unsigned char *dstp = dst->buffer + doffset;
					unsigned char ch;
					int j, k;
					for ( j = 0; j < src->width; j += 4 ) {
						ch = *srcp++;
						for ( k = 0; k < 4; ++k ) {
							if ((ch&0xA0) >> 6) {
								*dstp++ = NUM_GRAYS * ((ch&0xA0) >> 6) / 3 - 1;
							} else {
								*dstp++ = 0x00;
							}
							ch <<= 2;
						}
					}
				} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY4 ) {
					unsigned char *srcp = src->buffer + soffset;
					unsigned char *dstp = dst->buffer + doffset;
					unsigned char ch;
					int j, k;
					for ( j = 0; j < src->width; j += 2 ) {
						ch = *srcp++;
						for ( k = 0; k < 2; ++k ) {
							if ((ch&0xF0) >> 4) {
								*dstp++ = NUM_GRAYS * ((ch&0xF0) >> 4) / 15 - 1;
							} else {
								*dstp++ = 0x00;
							}
							ch <<= 4;
						}
					}
				} else {
					memcpy(dst->buffer+doffset,
							src->buffer+soffset, src->pitch);
				}
			}
		}

		/* Handle the bold style */
		if ( font->style & TTF_STYLE_BOLD ) {
			int row;
			int col;
			int offset;
			int pixel;
			Uint8* pixmap;

			/* The pixmap is a little hard, we have to add and clamp */
			for( row = dst->rows - 1; row >= 0; --row ) {
				pixmap = (Uint8*) dst->buffer + row * dst->pitch;
				for( offset=1; offset <= font->glyph_overhang; ++offset ) {
					for( col = dst->width - 1; col > 0; --col ) {
						if( mono ) {
							pixmap[col] |= pixmap[col-1];
						} else {
							pixel = (pixmap[col] + pixmap[col-1]);
							if( pixel > NUM_GRAYS - 1 ) {
								pixel = NUM_GRAYS - 1;
							}
							pixmap[col] = (Uint8) pixel;
						}
					}
				}
			}
		}

		/* Mark that we rendered this format */
		if ( mono ) {
			cached->stored |= CACHED_BITMAP;
		} else {
			cached->stored |= CACHED_PIXMAP;
		}
	}

	/* We're done, mark this glyph cached */
	cached->cached = ch;

	return 0;
}

static FT_Error Load_Glyph_Byname( TTF_Font* font, char *ch, c_glyph *cached, int want )
{
	FT_Face face;
	FT_Error error;
	FT_GlyphSlot glyph;
	FT_Glyph_Metrics* metrics;
	FT_Outline* outline;

	if ( !font || !font->face ) {
		return FT_Err_Invalid_Handle;
	}

	face = font->face;

	/* Load the glyph */
	cached->index = FT_Get_Name_Index( face, ch );
	error = FT_Load_Glyph( face, cached->index, FT_LOAD_DEFAULT );
	if( error ) {
		return error;
	}

	/* Get our glyph shortcuts */
	glyph = face->glyph;
	metrics = &glyph->metrics;
	outline = &glyph->outline;

	/* Get the glyph metrics if desired */
	if ( (want & CACHED_METRICS) && !(cached->stored & CACHED_METRICS) ) {
		if ( FT_IS_SCALABLE( face ) ) {
			/* Get the bounding box */
			cached->minx = FT_FLOOR(metrics->horiBearingX);
			cached->maxx = cached->minx + FT_CEIL(metrics->width);
			cached->maxy = FT_FLOOR(metrics->horiBearingY);
			cached->miny = cached->maxy - FT_CEIL(metrics->height);
			cached->yoffset = font->ascent - cached->maxy;
			cached->advance = FT_CEIL(metrics->horiAdvance);
		} else {
			/* Get the bounding box for non-scalable format.
			 * Again, freetype2 fills in many of the font metrics
			 * with the value of 0, so some of the values we
			 * need must be calculated differently with certain
			 * assumptions about non-scalable formats.
			 * */
			cached->minx = FT_FLOOR(metrics->horiBearingX);
			cached->maxx = cached->minx + FT_CEIL(metrics->horiAdvance);
			cached->maxy = FT_FLOOR(metrics->horiBearingY);
			cached->miny = cached->maxy - FT_CEIL(face->available_sizes[font->font_size_family].height);
			cached->yoffset = 0;
			cached->advance = FT_CEIL(metrics->horiAdvance);
		}

		/* Adjust for bold and italic text */
		if( font->style & TTF_STYLE_BOLD ) {
			cached->maxx += font->glyph_overhang;
		}
		if( font->style & TTF_STYLE_ITALIC ) {
			cached->maxx += (int)ceil(font->glyph_italics);
		}
		cached->stored |= CACHED_METRICS;
	}

	if ( ((want & CACHED_BITMAP) && !(cached->stored & CACHED_BITMAP)) ||
			((want & CACHED_PIXMAP) && !(cached->stored & CACHED_PIXMAP)) ) {
		int mono = (want & CACHED_BITMAP);
		int i;
		FT_Bitmap* src;
		FT_Bitmap* dst;

		/* Handle the italic style */
		if( font->style & TTF_STYLE_ITALIC ) {
			FT_Matrix shear;

			shear.xx = 1 << 16;
			shear.xy = (int) ( font->glyph_italics * ( 1 << 16 ) ) / font->height;
			shear.yx = 0;
			shear.yy = 1 << 16;

			FT_Outline_Transform( outline, &shear );
		}

		/* Render the glyph */
		if ( mono ) {
			error = FT_Render_Glyph( glyph, ft_render_mode_mono );
		} else {
			error = FT_Render_Glyph( glyph, ft_render_mode_normal );
		}
		if( error ) {
			return error;
		}

		/* Copy over information to cache */
		src = &glyph->bitmap;
		if ( mono ) {
			dst = &cached->bitmap;
		} else {
			dst = &cached->pixmap;
		}
		memcpy( dst, src, sizeof( *dst ) );

		/* FT_Render_Glyph() and .fon fonts always generate a
		 * two-color (black and white) glyphslot surface, even
		 * when rendered in ft_render_mode_normal. */
		/* FT_IS_SCALABLE() means that the font is in outline format,
		 * but does not imply that outline is rendered as 8-bit
		 * grayscale, because embedded bitmap/graymap is preferred
		 * (see FT_LOAD_DEFAULT section of FreeType2 API Reference).
		 * FT_Render_Glyph() canreturn two-color bitmap or 4/16/256-
		 * color graymap according to the format of embedded bitmap/
		 * graymap. */
		if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO ) {
			dst->pitch *= 8;
		} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY2 ) {
			dst->pitch *= 4;
		} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY4 ) {
			dst->pitch *= 2;
		}

		/* Adjust for bold and italic text */
		if( font->style & TTF_STYLE_BOLD ) {
			int bump = font->glyph_overhang;
			dst->pitch += bump;
			dst->width += bump;
		}
		if( font->style & TTF_STYLE_ITALIC ) {
			int bump = (int)ceil(font->glyph_italics);
			dst->pitch += bump;
			dst->width += bump;
		}

		if (dst->rows != 0) {
			dst->buffer = (unsigned char *)GUI_MALLOCZ( dst->pitch * dst->rows );
			if( !dst->buffer ) {
				return FT_Err_Out_Of_Memory;
			}
			memset( dst->buffer, 0, dst->pitch * dst->rows );

			for( i = 0; i < src->rows; i++ ) {
				int soffset = i * src->pitch;
				int doffset = i * dst->pitch;
				if ( mono ) {
					unsigned char *srcp = src->buffer + soffset;
					unsigned char *dstp = dst->buffer + doffset;
					int j;
					if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO ) {
						for ( j = 0; j < src->width; j += 8 ) {
							unsigned char ch = *srcp++;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
						}
					}  else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY2 ) {
						for ( j = 0; j < src->width; j += 4 ) {
							unsigned char ch = *srcp++;
							*dstp++ = (((ch&0xA0) >> 6) >= 0x2) ? 1 : 0;
							ch <<= 2;
							*dstp++ = (((ch&0xA0) >> 6) >= 0x2) ? 1 : 0;
							ch <<= 2;
							*dstp++ = (((ch&0xA0) >> 6) >= 0x2) ? 1 : 0;
							ch <<= 2;
							*dstp++ = (((ch&0xA0) >> 6) >= 0x2) ? 1 : 0;
						}
					} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY4 ) {
						for ( j = 0; j < src->width; j += 2 ) {
							unsigned char ch = *srcp++;
							*dstp++ = (((ch&0xF0) >> 4) >= 0x8) ? 1 : 0;
							ch <<= 4;
							*dstp++ = (((ch&0xF0) >> 4) >= 0x8) ? 1 : 0;
						}
					} else {
						for ( j = 0; j < src->width; j++ ) {
							unsigned char ch = *srcp++;
							*dstp++ = (ch >= 0x80) ? 1 : 0;
						}
					}
				} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO ) {
					/* This special case wouldn't
					 * be here if the FT_Render_Glyph()
					 * function wasn't buggy when it tried
					 * to render a .fon font with 256
					 * shades of gray.  Instead, it
					 * returns a black and white surface
					 * and we have to translate it back
					 * to a 256 gray shaded surface.
					 * */
					unsigned char *srcp = src->buffer + soffset;
					unsigned char *dstp = dst->buffer + doffset;
					unsigned char ch;
					int j, k;
					for ( j = 0; j < src->width; j += 8) {
						ch = *srcp++;
						for (k = 0; k < 8; ++k) {
							if ((ch&0x80) >> 7) {
								*dstp++ = NUM_GRAYS - 1;
							} else {
								*dstp++ = 0x00;
							}
							ch <<= 1;
						}
					}
				} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY2 ) {
					unsigned char *srcp = src->buffer + soffset;
					unsigned char *dstp = dst->buffer + doffset;
					unsigned char ch;
					int j, k;
					for ( j = 0; j < src->width; j += 4 ) {
						ch = *srcp++;
						for ( k = 0; k < 4; ++k ) {
							if ((ch&0xA0) >> 6) {
								*dstp++ = NUM_GRAYS * ((ch&0xA0) >> 6) / 3 - 1;
							} else {
								*dstp++ = 0x00;
							}
							ch <<= 2;
						}
					}
				} else if ( glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY4 ) {
					unsigned char *srcp = src->buffer + soffset;
					unsigned char *dstp = dst->buffer + doffset;
					unsigned char ch;
					int j, k;
					for ( j = 0; j < src->width; j += 2 ) {
						ch = *srcp++;
						for ( k = 0; k < 2; ++k ) {
							if ((ch&0xF0) >> 4) {
								*dstp++ = NUM_GRAYS * ((ch&0xF0) >> 4) / 15 - 1;
							} else {
								*dstp++ = 0x00;
							}
							ch <<= 4;
						}
					}
				} else {
					memcpy(dst->buffer+doffset,
							src->buffer+soffset, src->pitch);
				}
			}
		}

		/* Handle the bold style */
		if ( font->style & TTF_STYLE_BOLD ) {
			int row;
			int col;
			int offset;
			int pixel;
			Uint8* pixmap;

			/* The pixmap is a little hard, we have to add and clamp */
			for( row = dst->rows - 1; row >= 0; --row ) {
				pixmap = (Uint8*) dst->buffer + row * dst->pitch;
				for( offset=1; offset <= font->glyph_overhang; ++offset ) {
					for( col = dst->width - 1; col > 0; --col ) {
						if( mono ) {
							pixmap[col] |= pixmap[col-1];
						} else {
							pixel = (pixmap[col] + pixmap[col-1]);
							if( pixel > NUM_GRAYS - 1 ) {
								pixel = NUM_GRAYS - 1;
							}
							pixmap[col] = (Uint8) pixel;
						}
					}
				}
			}
		}

		/* Mark that we rendered this format */
		if ( mono ) {
			cached->stored |= CACHED_BITMAP;
		} else {
			cached->stored |= CACHED_PIXMAP;
		}
	}

	return 0;
}

static FT_Error Find_Glyph( TTF_Font* font, Uint32 ch, int want )
{
	int retval = 0;

	if( ch < 256 ) {
		font->current = &font->cache[ch];
	} else {
		if ( font->scratch.cached != ch ) {
			Flush_Glyph( &font->scratch );
		}
		font->current = &font->scratch;
	}
	if ( (font->current->stored & want) != want ) {
		retval = Load_Glyph( font, ch, font->current, want );
	}
	return retval;
}

static unsigned int _get_start_code(unsigned int code)
{
	return (code & 0xFFFFFF00);
}

static void _free_cjk(void *p)
{
	c_glyph *p_glyph = NULL;

	p_glyph = (c_glyph *)p;

	Flush_Glyph(p_glyph);

	GUI_FREE(p);
}

static unsigned int _cache_cjk_glyph(TTF_Font* font, Uint32 ch, int want)
{
	char num_value[10] = {0};
	c_glyph *p_glyph = NULL;
	int retval = 0;

	sprintf(num_value, "0x%x", ch);

	if(NULL == font->cjk_hash)
	{
		font->cjk_hash = hash_new(100, _free_cjk);
		if(NULL == font->cjk_hash)
		{
			return (1);
		}
	}

	if(0 == FT_Get_Char_Index( font->face, ch ))
	{
		if(font->null_glyph)
		{
			font->current = font->null_glyph;
			//return (0);
		}
		else
		{
			font->null_glyph = (c_glyph *)GUI_MALLOCZ(sizeof(c_glyph));
			if(NULL == font->null_glyph)
			{
				return (1);
			}
			memset(font->null_glyph, 0, sizeof(c_glyph));
			font->current = font->null_glyph;
		}
	}
	else
	{
		font->current = hash_get(font->cjk_hash, num_value);
		if(NULL == font->current)
		{
			p_glyph = (c_glyph *)GUI_MALLOCZ(sizeof(c_glyph));
			if(NULL == p_glyph)
			{
				return (1);
			}
			memset(p_glyph, 0, sizeof(c_glyph));

			font->current = p_glyph;

			hash_add(font->cjk_hash, num_value, p_glyph);
		}
	}

	if( (font->current->stored & want) != want )
	{
		retval = Load_Glyph( font, ch, font->current, want );
	}

	return (retval);
}

static FT_Error Cache_Subscript(TTF_Font* font, Uint32 ch, int want, int sub_index)
{
	int retval = 0;

	if( ch < 256 ){
		font->subscript = &font->cache[ch];
	}else {
		if(0 == FT_Get_Char_Index( font->face, ch ))
		{
			if(font->null_glyph)
			{
				font->subscript = font->null_glyph;
				//return (0);
			}
			else
			{
				font->null_glyph = (c_glyph *)GUI_MALLOCZ(sizeof(c_glyph));
				if(NULL == font->null_glyph)
				{
					return (1);
				}
				memset(font->null_glyph, 0, sizeof(c_glyph));
				font->subscript = font->null_glyph;
			}
		}
		else
		{
			if((ch >= 0x3000) && (ch <= 0x9FFF))
			{
				/*CJK Unified Ideographs*/
				return (_cache_cjk_glyph(font, ch, want));
			}
			retval = _get_start_code(ch);
			/*Arabic base shape*/
			if((retval >= 0x0600) && (retval <= 0x06FF))
			{
				font->subscript = &font->arabic_base[ch - 0x0600];
			}
			else if(retval != font->start_code){
				font->start_code = retval;
				Flush_ExtraCache(font);
				font->subscript = &font->subcache[ch - font->start_code];
			}
			else
			{
				font->subscript = &font->subcache[ch - font->start_code];
			}
			retval = 0;
		}
	}
	//if( (font->subscript->stored & want) != want ) {
	if( font->subscript ) {
		retval = Load_Subscript( font, ch, font->subscript, want , sub_index);
	}
	return retval;
}

FT_Error Cache_Glyph(TTF_Font* font, Uint32 ch, int want)
{
	int retval = 0;

	if( ch < 256 ){
		font->current = &font->cache[ch];
	}else {
		if(0 == FT_Get_Char_Index( font->face, ch ))
		{
			if(font->null_glyph)
			{
				font->current = font->null_glyph;
				//return (0);
			}
			else
			{
				font->null_glyph = (c_glyph *)GUI_MALLOCZ(sizeof(c_glyph));
				if(NULL == font->null_glyph)
				{
					return (1);
				}
				memset(font->null_glyph, 0, sizeof(c_glyph));
				font->current = font->null_glyph;
			}
		}
		else
		{
			if((ch >= 0x3000) && (ch <= 0x9FFF))
			{
				/*CJK Unified Ideographs*/
				return (_cache_cjk_glyph(font, ch, want));
			}
			retval = _get_start_code(ch);
			/*Arabic base shape*/
			if((retval >= 0x0600) && (retval <= 0x06FF))
			{
				font->current = &font->arabic_base[ch - 0x0600];
			}
			else if(((ch >= 0xFE00) && (ch <= 0xFEFF)) ||
					((ch >= 0x1E00) && (ch <= 0x1EFF)))
			{
			    if(retval == font->arabic_presentb)
			    {
				font->current = &font->arabic_present_b[ch - font->arabic_presentb];
			    }
			    else
			    {
				font->arabic_presentb = retval;
				Flush_ArabicBCache(font);
				font->current = &font->arabic_present_b[ch - font->arabic_presentb];
			    }
			}
			else if(retval != font->start_code){
				font->start_code = retval;
				Flush_ExtraCache(font);
				font->current = &font->extracache[ch - font->start_code];
				if((retval != 0xFB00) && (retval != 0x1E00))
				    Free_ArabicBCache(font);
				if(font->extra_hash) {
					hash_release(font->extra_hash);
					font->extra_hash = NULL;
				}
			}
			else
			{
				font->current = &font->extracache[ch - font->start_code];
			}
			retval = 0;
		}
	}
	if( (font->current->stored & want) != want ) {
		retval = Load_Glyph( font, ch, font->current, want );
	}
	return retval;
}

static void extra_hash_free(void *p)
{
	c_glyph *glyph = NULL;

	if(NULL == p)
		return;

	glyph = (c_glyph *)p;
	GUI_FREE(glyph->pixmap.buffer);
	GUI_FREE(p);
}

static FT_Error Cache_Glyph_Byname(TTF_Font* font, char *ch, int want)
{
	int retval = 0;
	c_glyph *extra_glyph = NULL, *current_glyph = NULL;

	if(NULL == font->extra_hash) {
		font->extra_hash = hash_new(10, extra_hash_free);
	}

	if(font->extra_hash) {
		current_glyph = hash_get(font->extra_hash, ch);
		if(NULL == current_glyph) {
			extra_glyph = GUI_MALLOCZ(sizeof(c_glyph));
		} else {
			if(current_glyph->pixmap.buffer) {
				font->subscript = current_glyph;
				return (0);
			}
		}
		if(extra_glyph) {
			hash_add(font->extra_hash, ch, extra_glyph);
			current_glyph = extra_glyph;
		}
	}

	if(0 == FT_Get_Name_Index( font->face, ch ))
	{
		if(font->null_glyph)
		{
			font->subscript = font->null_glyph;
		}
		else
		{
			font->null_glyph = (c_glyph *)GUI_MALLOCZ(sizeof(c_glyph));
			if(NULL == font->null_glyph)
			{
				return (1);
			}
			memset(font->null_glyph, 0, sizeof(c_glyph));
			font->subscript = font->null_glyph;
		}
	} else if((current_glyph) &&
		  (FT_Get_Name_Index( font->face, ch ) == current_glyph->index)) {
		if((CACHED_PIXMAP & want) &&
		   (current_glyph->pixmap.buffer)) {
			memcpy(font->subscript, current_glyph, sizeof(c_glyph));
			return retval;
		}
	} else {
	}

	memset(&font->extra_devanagari, 0, sizeof(c_glyph));
	font->subscript = &font->extra_devanagari;
	retval = Load_Glyph_Byname( font, ch, font->subscript, want );
	if(current_glyph) {
		if(font->subscript->pixmap.buffer) {
			memcpy(current_glyph, font->subscript, sizeof(c_glyph));
			current_glyph->pixmap.buffer = GUI_MALLOCZ(current_glyph->pixmap.pitch * current_glyph->pixmap.rows);
			memcpy(current_glyph->pixmap.buffer, font->subscript->pixmap.buffer,
			       current_glyph->pixmap.pitch * current_glyph->pixmap.rows);
		} else {
			font->subscript = &(font->extra_devanagari);
		}
	}
	return retval;
}


void TTF_CloseFont( TTF_Font* font )
{
	if ( font ) {
		Flush_Cache( font );
		Flush_ExtraCache( font );
		Free_ArabicBCache(font);
		Free_ArabicBaseCache(font);
		if ( font->face ) {
			FT_Done_Face( font->face );
		}
		if ( font->args.stream ) {
			GUI_FREE( font->args.stream );
		}
		if ( font->freesrc ) {
			GxCore_Close( font->src );//SDL_RWclose( font->src );
		}
		if(font->cjk_hash) {
			hash_release(font->cjk_hash);
		}
		GUI_FREE(font->null_glyph);
		GUI_FREE( font );
	}
}

TTF_Font* TTF_OpenFontIndexRW( handle_t src, int freesrc, int ptsize, long index )
{
	TTF_Font* font;
	FT_Error error;
	FT_Face face;
	FT_Fixed scale;
	FT_Stream stream;
	int position;

	if ( ! TTF_initialized ) {
		//TTF_SetError( "Library not initialized" );
		return NULL;
	}

	/* Check to make sure we can seek in this stream */
	position = GxCore_Seek(src, 0, SEEK_CUR);//SDL_RWtell(src);
	if ( position < 0 ) {
		//TTF_SetError( "Can't seek in stream" );
		return NULL;
	}

	font = (TTF_Font*) GUI_MALLOCZ(sizeof *font);
	if ( font == NULL ) {
		//TTF_SetError( "Out of memory" );
		return NULL;
	}
	memset(font, 0, sizeof(*font));

	font->src = src;
	font->freesrc = freesrc;

	stream = (FT_Stream)GUI_MALLOCZ(sizeof(*stream));
	if ( stream == NULL ) {
		//TTF_SetError( "Out of memory" );
		TTF_CloseFont( font );
		return NULL;
	}
	memset(stream, 0, sizeof(*stream));

	stream->read = RWread;
	stream->descriptor.pointer = (void*)src;
	stream->pos = (unsigned long)position;
	GxCore_Seek(src, 0, SEEK_END);//SDL_RWseek(src, 0, SEEK_END);
	stream->size = (unsigned long)(GxCore_Seek(src, 0, SEEK_CUR) - position);
	GxCore_Seek(src, position, SEEK_SET);//SDL_RWseek(src, position, SEEK_SET);

	font->args.flags = FT_OPEN_STREAM;
	font->args.stream = stream;

	error = FT_Open_Face( library, &font->args, index, &font->face );
	if( error ) {
		//TTF_SetFTError( "Couldn't load font file", error );
		TTF_CloseFont( font );
		return NULL;
	}
	face = font->face;

	/* Make sure that our font face is scalable (global metrics) */
	if ( FT_IS_SCALABLE(face) ) {

		/* Set the character size and use default DPI (72) */
		error = FT_Set_Char_Size( font->face, 0, ptsize * 64, 0, 0 );
		if( error ) {
			//TTF_SetFTError( "Couldn't set font size", error );
			TTF_CloseFont( font );
			return NULL;
		}

		/* Get the scalable font metrics for this font */
		scale = face->size->metrics.y_scale;
		font->ascent  = FT_CEIL(FT_MulFix(face->ascender, scale));
		font->descent = FT_CEIL(FT_MulFix(face->descender, scale));
		font->height  = font->ascent - font->descent + /* baseline */ 1;
		font->lineskip = FT_CEIL(FT_MulFix(face->height, scale));
		font->underline_offset = FT_FLOOR(FT_MulFix(face->underline_position, scale));
		font->underline_height = FT_FLOOR(FT_MulFix(face->underline_thickness, scale));

	} else {
		/* Non-scalable font case.  ptsize determines which family
		 * or series of fonts to grab from the non-scalable format.
		 * It is not the point size of the font.
		 * */
		if ( ptsize >= font->face->num_fixed_sizes )
			ptsize = font->face->num_fixed_sizes - 1;
		font->font_size_family = ptsize;
		error = FT_Set_Pixel_Sizes( face,
				face->available_sizes[ptsize].height,
				face->available_sizes[ptsize].width );
		/* With non-scalale fonts, Freetype2 likes to fill many of the
		 * font metrics with the value of 0.  The size of the
		 * non-scalable fonts must be determined differently
		 * or sometimes cannot be determined.
		 * */
		font->ascent = face->available_sizes[ptsize].height;
		font->descent = 0;
		font->height = face->available_sizes[ptsize].height;
		font->lineskip = FT_CEIL(font->ascent);
		font->underline_offset = FT_FLOOR(face->underline_position);
		font->underline_height = FT_FLOOR(face->underline_thickness);
	}

	if ( font->underline_height < 1 ) {
		font->underline_height = 1;
	}

	/* Set the default font style */
	font->style = TTF_STYLE_NORMAL;
	font->glyph_overhang = face->size->metrics.y_ppem / 10;
	/* x offset = cos(((90.0-12)/360)*2*M_PI), or 12 degree angle */
	font->glyph_italics = 0.207f;
	font->glyph_italics *= font->height;

	return font;
}

static void *_ft_malloc(FT_Memory  memory, long       size)
{
	return ((void *)GUI_MALLOCZ(size));
}

static void _ft_free(FT_Memory  memory, void *block)
{
	GUI_FREE(block);
}

static void *_ft_realloc(FT_Memory  memory, long cur_size, long new_size, void *block)
{
	return (GUI_REALLOC(block, new_size));
}

static void _set_ft_memory(FT_Library library)
{
	struct FT_MemoryRec_ memory = {0};

	if(gui.config.font_in_pool) {
		memory.alloc = _ft_malloc;
		memory.free = _ft_free;
		memory.realloc = _ft_realloc;
		FT_Add_MemFun(library, &memory);
	}
}

int TTF_Init( void )
{
	int status = 0;

	if ( ! TTF_initialized ) {
		FT_Error error = FT_Init_FreeType( &library );
		if ( error ) {
			//TTF_SetFTError("Couldn't init FreeType engine", error);
			status = -1;
		}
		_set_ft_memory(library);
	}
	if ( status == 0 ) {
		++TTF_initialized;
	}
	return status;
}


TTF_Font* TTF_OpenFontIndex( const char *file, int ptsize, long index )
{
	/*SDL_RWops *rw = SDL_RWFromFile(file, "rb");
	  if ( rw == NULL ) {
	  TTF_SetError(SDL_GetError());
	  return NULL;
	  }*/
	handle_t hfile = 0;
	TTF_Font *data_font = NULL;

	hfile = GxCore_Open((char *)file, "rb");
	if(0 >= hfile)
	{
		return NULL;
	}

	data_font = TTF_OpenFontIndexRW(hfile, 1, ptsize, index);
	if(NULL == data_font) {
		GxCore_Close(hfile);
	}
	return (data_font);
}

TTF_Font* TTF_OpenFont( const char *file, int ptsize )
{
	return TTF_OpenFontIndex(file, ptsize, 0);
}

static int _CheckIsExist(struct gdi_font *pfont, unsigned int code)
{
    gdi_font_prop *table_ptr = NULL;

    if(pfont->index_table)
    {
	table_ptr = pfont->index_table;
	while(table_ptr)
	{
	    if((code >= table_ptr->First) && (code <= table_ptr->Last))
	    {
		return (GUI_TRUE);
	    }
	    table_ptr = (gdi_font_prop *)(table_ptr->pNext);
	}
	return (GUI_FALSE);
    }
    else
    {
	return (GUI_FALSE);
    }
}

static TTF_Font* _OtherTTF_Blended(const Uint32 *text)
{
	TTF_Font *font = NULL;
	struct gdi_font *pfont = NULL;
	FT_Error error;
	const Uint32 *ch;
	c_glyph *glyph;

	pfont = gdi_get_first_font();
	while(pfont)
	{
		if(TTF_FONT_TYPE == pfont->family)
		{
			font = (TTF_Font *)(pfont->font_data);
			ch = text;
			if(GUI_FALSE == _CheckIsExist(pfont, *ch))
			{
			    pfont = pfont->next;
			    continue;
			}
			error = Cache_Glyph(font, *ch, CACHED_METRICS|CACHED_PIXMAP);
			if( error ) {
				return (NULL);
			}

			glyph = font->current;
			if(0 != glyph->index)
			{
				break;
			}
		}
		pfont = pfont->next;
	}

	if(NULL == pfont)
	{
		font = NULL;
	}

	return (font);
}

int TTF_SizeSubScript(TTF_Font *font, const unsigned int *text, int *w, int *h, int sub_index)
{
	int status;
	const unsigned int *ch;
	int swapped;
	int x, z;
	int minx, maxx;
	int miny, maxy;
	c_glyph *glyph = NULL;
	FT_Error error;
	FT_Long use_kerning;
	FT_UInt prev_index = 0;
	TTF_Font *other_font = NULL;

	/* Initialize everything to 0 */
	if ( ! TTF_initialized ) {
		gxlogd( "Library not initialized" );
		return -1;
	}
	status = 0;
	minx = maxx = 0;
	miny = maxy = 0;
	swapped = TTF_byteswapped;

	/* check kerning */
	use_kerning = FT_HAS_KERNING( font->face );

	/* Load each character and sum it's bounding box */
	x= 0;
	for ( ch=text; *ch; ++ch ) {
		unsigned int c = *ch;
		if ( c == UNICODE_BOM_NATIVE ) {
			swapped = 0;
			if ( text == ch ) {
				++text;
			}
			continue;
		}
		if ( c == UNICODE_BOM_SWAPPED ) {
			swapped = 1;
			if ( text == ch ) {
				++text;
			}
			continue;
		}
		if ( swapped ) {
			c = SDL_Swap16(c);
		}

		error = Cache_Subscript(font, c, CACHED_METRICS, sub_index);
		if ( error ) {
			return -1;
		}
		glyph = font->subscript;

		/*Undefined character code*/
		if(0 == glyph->index)
		{
			other_font = _OtherTTF_Blended(text);
			if(other_font)
			{
				return (TTF_SizeSubScript(other_font, text, w, h, sub_index));
			}
		}

		/* handle kerning */
		if ( use_kerning && prev_index && glyph->index ) {
			FT_Vector delta;
			FT_Get_Kerning( font->face, prev_index, glyph->index, ft_kerning_default, &delta );
			x += delta.x >> 6;
		}

#if 0
		if ( (ch == text) && (glyph->minx < 0) ) {
			/* Fixes the texture wrapping bug when the first letter
			 * has a negative minx value or horibearing value.  The entire
			 * bounding box must be adjusted to be bigger so the entire
			 * letter can fit without any texture corruption or wrapping.
			 *
			 * Effects: First enlarges bounding box.
			 * Second, xstart has to start ahead of its normal spot in the
			 * negative direction of the negative minx value.
			 * (pushes everything to the right).
			 *
			 * This will make the memory copy of the glyph bitmap data
			 * work out correctly.
			 * */
			z -= glyph->minx;

		}
#endif

		z = x + glyph->minx;
		if ( minx > z ) {
			minx = z;
		}
		if ( font->style & TTF_STYLE_BOLD ) {
			x += font->glyph_overhang;
		}
		if ( glyph->advance > glyph->maxx ) {
			z = x + glyph->advance;
		} else {
			z = x + glyph->maxx;
		}
		if ( maxx < z ) {
			maxx = z;
		}
		x += glyph->advance;

		if ( glyph->miny < miny ) {
			miny = glyph->miny;
		}
		if ( glyph->maxy > maxy ) {
			maxy = glyph->maxy;
		}
		prev_index = glyph->index;
	}

	if(NULL == glyph)
		return status;

	/* Fill the bounds rectangle */
	if ( w ) {
		//*w = (glyph->advance - minx);
		//shencz modify 2014.12.15
		*w = (maxx - minx);
	}
	if ( h ) {
#if 0 /* This is correct, but breaks many applications */
		*h = (maxy - miny);
#else
		*h = font->height;
#endif
	}
	return status;
}

int TTF_SizeUNICODE(TTF_Font *font, const unsigned int *text, int *w, int *h)
{
	int status;
	const unsigned int *ch;
	int swapped;
	int x, z;
	int minx, maxx;
	int miny, maxy;
	c_glyph *glyph = NULL;
	FT_Error error;
	FT_Long use_kerning;
	FT_UInt prev_index = 0;
	TTF_Font *other_font = NULL;

	/* Initialize everything to 0 */
	if ( ! TTF_initialized ) {
		gxlogd( "Library not initialized" );
		return -1;
	}
	status = 0;
	minx = maxx = 0;
	miny = maxy = 0;
	swapped = TTF_byteswapped;

	/* check kerning */
	use_kerning = FT_HAS_KERNING( font->face );

	/* Load each character and sum it's bounding box */
	x= 0;
	for ( ch=text; *ch; ++ch ) {
		unsigned int c = *ch;
		if ( c == UNICODE_BOM_NATIVE ) {
			swapped = 0;
			if ( text == ch ) {
				++text;
			}
			continue;
		}
		if ( c == UNICODE_BOM_SWAPPED ) {
			swapped = 1;
			if ( text == ch ) {
				++text;
			}
			continue;
		}
		if ( swapped ) {
			c = SDL_Swap16(c);
		}

		error = Cache_Glyph(font, c, CACHED_METRICS);
		if ( error ) {
			return -1;
		}
		glyph = font->current;

		/*Undefined character code*/
		if(0 == glyph->index)
		{
			other_font = _OtherTTF_Blended(text);
			if(other_font)
			{
				return (TTF_SizeUNICODE(other_font, text, w, h));
			}
		}

		/* handle kerning */
		if ( use_kerning && prev_index && glyph->index ) {
			FT_Vector delta;
			FT_Get_Kerning( font->face, prev_index, glyph->index, ft_kerning_default, &delta );
			x += delta.x >> 6;
		}

#if 0
		if ( (ch == text) && (glyph->minx < 0) ) {
			/* Fixes the texture wrapping bug when the first letter
			 * has a negative minx value or horibearing value.  The entire
			 * bounding box must be adjusted to be bigger so the entire
			 * letter can fit without any texture corruption or wrapping.
			 *
			 * Effects: First enlarges bounding box.
			 * Second, xstart has to start ahead of its normal spot in the
			 * negative direction of the negative minx value.
			 * (pushes everything to the right).
			 *
			 * This will make the memory copy of the glyph bitmap data
			 * work out correctly.
			 * */
			z -= glyph->minx;

		}
#endif

		z = x + glyph->minx;
		if ( minx > z ) {
			minx = z;
		}
		if ( font->style & TTF_STYLE_BOLD ) {
			x += font->glyph_overhang;
		}
		if ( glyph->advance > glyph->maxx ) {
			z = x + glyph->advance;
		} else {
			z = x + glyph->maxx;
		}
		if ( maxx < z ) {
			maxx = z;
		}
		x += glyph->advance;

		if ( glyph->miny < miny ) {
			miny = glyph->miny;
		}
		if ( glyph->maxy > maxy ) {
			maxy = glyph->maxy;
		}
		prev_index = glyph->index;
	}

	if(NULL == glyph)
		return status;

	/* Fill the bounds rectangle */
	if ( w ) {
		//*w = (glyph->advance - minx);
		//shencz modify 2014.12.15
		*w = (maxx - minx);
	}
	if ( h ) {
#if 0 /* This is correct, but breaks many applications */
		*h = (maxy - miny);
#else
		*h = font->height;
#endif
	}
	return status;
}

int TTF_SizeNAME(TTF_Font *font, char *text, int *w, int *h)
{
	int status;
	int x, z;
	int minx, maxx;
	int miny, maxy;
	c_glyph *glyph = NULL;
	FT_Error error;
	FT_Long use_kerning;
	FT_UInt prev_index = 0;

	/* Initialize everything to 0 */
	if ( ! TTF_initialized ) {
		gxlogd( "Library not initialized" );
		return -1;
	}
	status = 0;
	minx = maxx = 0;
	miny = maxy = 0;

	/* check kerning */
	use_kerning = FT_HAS_KERNING( font->face );

	/* Load each character and sum it's bounding box */
	x= 0;

	error = Cache_Glyph_Byname(font, text, CACHED_METRICS);
	if ( error ) {
		return -1;
	}
	glyph = font->subscript;

	/*Undefined character code*/
	if(0 == glyph->index) {
		return -1;
	}

	/* handle kerning */
	if ( use_kerning && prev_index && glyph->index ) {
		FT_Vector delta;
		FT_Get_Kerning( font->face, prev_index, glyph->index, ft_kerning_default, &delta );
		x += delta.x >> 6;
	}

	z = x + glyph->minx;
	if ( minx > z ) {
		minx = z;
	}
	if ( font->style & TTF_STYLE_BOLD ) {
		x += font->glyph_overhang;
	}
	if ( glyph->advance > glyph->maxx ) {
		z = x + glyph->advance;
	} else {
		z = x + glyph->maxx;
	}
	if ( maxx < z ) {
		maxx = z;
	}
	x += glyph->advance;

	if ( glyph->miny < miny ) {
		miny = glyph->miny;
	}
	if ( glyph->maxy > maxy ) {
		maxy = glyph->maxy;
	}
	prev_index = glyph->index;

	if(NULL == glyph)
		return status;

	/* Fill the bounds rectangle */
	if ( w ) {
		//*w = (glyph->advance - minx);
		//shencz modify 2014.12.15
		*w = (maxx - minx);
	}
	if ( h ) {
#if 0 /* This is correct, but breaks many applications */
		*h = (maxy - miny);
#else
		*h = font->height;
#endif
	}
	return status;
}

int TTF_GlyphMetrics(TTF_Font *font, const unsigned int ch,
		int* minx, int* maxx, int* miny, int* maxy, int* advance)
{
	FT_Error error;

	error = Find_Glyph(font, ch, CACHED_METRICS);
	if ( error ) {
		return -1;
	}

	if ( minx ) {
		*minx = font->current->minx;
	}
	if ( maxx ) {
		*maxx = font->current->maxx;
		if ( TTF_HANDLE_STYLE_BOLD(font) ) {
			*maxx += font->glyph_overhang;
		}
	}
	if ( miny ) {
		*miny = font->current->miny;
	}
	if ( maxy ) {
		*maxy = font->current->maxy;
	}
	if ( advance ) {
		*advance = font->current->advance;
		if ( TTF_HANDLE_STYLE_BOLD(font) ) {
			*advance += font->glyph_overhang;
		}
	}
	return 0;
}

int TTF_GlyphMetrics_Byname(TTF_Font *font, char *ch, int* minx,
			int* maxx, int* miny, int* maxy, int* advance)
{
	FT_Error error;

	if(NULL == ch)
		return -1;

	memset(&font->extra_devanagari, 0, sizeof(c_glyph));
	font->subscript = &font->extra_devanagari;

	error = Load_Glyph_Byname( font, ch, font->subscript, CACHED_METRICS);
	if ( error ) {
		return -1;
	}

	if ( minx ) {
		*minx = font->subscript->minx;
	}
	if ( maxx ) {
		*maxx = font->subscript->maxx;
		if ( TTF_HANDLE_STYLE_BOLD(font) ) {
			*maxx += font->glyph_overhang;
		}
	}
	if ( miny ) {
		*miny = font->subscript->miny;
	}
	if ( maxy ) {
		*maxy = font->subscript->maxy;
	}
	if ( advance ) {
		*advance = font->subscript->advance;
		if ( TTF_HANDLE_STYLE_BOLD(font) ) {
			*advance += font->glyph_overhang;
		}
	}
	return 0;
}

SDL_Surface *TTF_RenderSubScript_Blended(TTF_Font *font,
		const Uint32 *text, SDL_Color fg, int sub_index)
{
	int xstart;
	int width, height;
	SDL_Surface *textbuf;
	const Uint32 *ch;
	Uint8 *src;
	Uint8 *dst;
	int swapped;
	int row;
	c_glyph *glyph = NULL;
	FT_Error error;
	FT_Long use_kerning;
	FT_UInt prev_index = 0;
	TTF_Font *other_font = NULL;

	/* Get the dimensions of the text surface */
	if ( (TTF_SizeSubScript(font, text, &width, NULL, sub_index) < 0) || !width ) {
		gxlogd("Text has zero width");
		return(NULL);
	}
	height = font->height;

	textbuf = SDL_AllocSurface(SDL_SWSURFACE, width, height, 8,
			0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	if ( textbuf == NULL ) {
		return(NULL);
	}

	/* Adding bound checking to avoid all kinds of memory corruption errors
	   that may occur. */
	memset(textbuf->pixels, 0, textbuf->pitch * textbuf->h);
	//dst_check = (Uint8*)textbuf->pixels + textbuf->pitch * textbuf->h;

	/* check kerning */
	use_kerning = FT_HAS_KERNING( font->face );

	/* Load and render each character */
	xstart = 0;
	swapped = TTF_byteswapped;

	for ( ch=text; *ch; ++ch ) {
		Uint32 c = *ch;
		if ( c == UNICODE_BOM_NATIVE ) {
			swapped = 0;
			if ( text == ch ) {
				++text;
			}
			continue;
		}
		if ( c == UNICODE_BOM_SWAPPED ) {
			swapped = 1;
			if ( text == ch ) {
				++text;
			}
			continue;
		}
		if ( swapped ) {
			c = SDL_Swap16(c);
		}
		error = Cache_Subscript(font, c, CACHED_METRICS|CACHED_PIXMAP, sub_index);
		if( error ) {
			SDL_FreeSurface( textbuf );
			return NULL;
		}
		glyph = font->subscript;

		/*Undefined character code*/
		if(0 == glyph->index)
		{
			other_font = _OtherTTF_Blended(text);
			if(other_font)
			{
				SDL_FreeSurface( textbuf );
				return (TTF_RenderSubScript_Blended(other_font, text, fg, sub_index));
			}
		}

		/* Ensure the width of the pixmap is correct. On some cases,
		 * freetype may report a larger pixmap than possible.*/
		width = glyph->pixmap.width;
		if ((width > glyph->maxx - glyph->minx) && !(font->style & TTF_STYLE_BOLD)) {
			width = glyph->maxx - glyph->minx;
		}
		/* do kerning, if possible AC-Patch */
		if ( use_kerning && prev_index && glyph->index ) {
			FT_Vector delta;
			FT_Get_Kerning( font->face, prev_index, glyph->index, ft_kerning_default, &delta );
			xstart += delta.x >> 6;
		}

		/* Compensate for the wrap around bug with negative minx's */
		if ( (ch == text) && (glyph->minx < 0) ) {
			xstart -= glyph->minx;
		}

		for ( row = 0; row < glyph->pixmap.rows; ++row ) {
			/* Make sure we don't go either over, or under the
			 * limit */
			if ( row+glyph->yoffset < 0 ) {
				continue;
			}
			if ( row+glyph->yoffset >= textbuf->h ) {
				continue;
			}
			dst = (Uint8*) textbuf->pixels +
				(row+glyph->yoffset) * textbuf->pitch +
				xstart + glyph->minx;

			/* Added code to adjust src pointer for pixmaps to
			 * account for pitch.
			 * */
			src = (Uint8*) (glyph->pixmap.buffer + glyph->pixmap.pitch * row);
			memcpy(dst, src, width);
		}

		xstart += glyph->advance;
		if ( font->style & TTF_STYLE_BOLD ) {
			xstart += font->glyph_overhang;
		}
		prev_index = glyph->index;
	}

	if(NULL == glyph)
	{
		SDL_FreeSurface( textbuf );
		return NULL;
	}

	/* Handle the underline style */
	if( font->style & TTF_STYLE_UNDERLINE ) {
		row = font->ascent - font->underline_offset - 1;
		if ( row >= textbuf->h) {
			row = (textbuf->h-1) - font->underline_height;
		}
		dst = (Uint8 *)textbuf->pixels + row * textbuf->pitch;
		for ( row=font->underline_height; row>0; --row ) {
			memset(dst, 0xFF, textbuf->w);
			dst += textbuf->pitch;
		}
	}
	textbuf->minx = glyph->minx;
	//textbuf->miny = glyph->miny;
	GUI_FREE(glyph->pixmap.buffer);
	return(textbuf);
}

SDL_Surface *TTF_RenderUNICODE_Blended(TTF_Font *font,
		const Uint32 *text, SDL_Color fg)
{
	int xstart;
	int width, height;
	SDL_Surface *textbuf;
	const Uint32 *ch;
	Uint8 *src;
	Uint8 *dst;
	int swapped;
	int row;
	c_glyph *glyph = NULL;
	FT_Error error;
	FT_Long use_kerning;
	FT_UInt prev_index = 0;
	TTF_Font *other_font = NULL;

	/* Get the dimensions of the text surface */
	if ( (TTF_SizeUNICODE(font, text, &width, NULL) < 0) || !width ) {
		gxlogd("Text has zero width");
		return(NULL);
	}
	height = font->height;

	textbuf = SDL_AllocSurface(SDL_SWSURFACE, width, height, 8,
			0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	if ( textbuf == NULL ) {
		return(NULL);
	}

	/* Adding bound checking to avoid all kinds of memory corruption errors
	   that may occur. */
	//dst_check = (Uint8*)textbuf->pixels + textbuf->pitch * textbuf->h;

	/* check kerning */
	use_kerning = FT_HAS_KERNING( font->face );

	/* Load and render each character */
	xstart = 0;
	swapped = TTF_byteswapped;

	for ( ch=text; *ch; ++ch ) {
		Uint32 c = *ch;
		if ( c == UNICODE_BOM_NATIVE ) {
			swapped = 0;
			if ( text == ch ) {
				++text;
			}
			continue;
		}
		if ( c == UNICODE_BOM_SWAPPED ) {
			swapped = 1;
			if ( text == ch ) {
				++text;
			}
			continue;
		}
		if ( swapped ) {
			c = SDL_Swap16(c);
		}
		error = Cache_Glyph(font, c, CACHED_METRICS|CACHED_PIXMAP);
		if( error ) {
			SDL_FreeSurface( textbuf );
			return NULL;
		}
		glyph = font->current;

		/*Undefined character code*/
		if(0 == glyph->index)
		{
			other_font = _OtherTTF_Blended(text);
			if(other_font)
			{
				SDL_FreeSurface( textbuf );
				return (TTF_RenderUNICODE_Blended(other_font, text, fg));
			}
		}

		/* Ensure the width of the pixmap is correct. On some cases,
		 * freetype may report a larger pixmap than possible.*/
		width = glyph->pixmap.width;
		if ((width > glyph->maxx - glyph->minx) && !(font->style & TTF_STYLE_BOLD)) {
			width = glyph->maxx - glyph->minx;
		}
		/* do kerning, if possible AC-Patch */
		if ( use_kerning && prev_index && glyph->index ) {
			FT_Vector delta;
			FT_Get_Kerning( font->face, prev_index, glyph->index, ft_kerning_default, &delta );
			xstart += delta.x >> 6;
		}

		/* Compensate for the wrap around bug with negative minx's */
		if ( (ch == text) && (glyph->minx < 0) ) {
			xstart -= glyph->minx;
		}

		int offset = xstart + glyph->minx;
		for (row = 0; row < glyph->pixmap.rows; ++row) {
			int y = row + glyph->yoffset;

			if (y < 0 || y >= textbuf->h) {
				continue;
			}

			dst = (Uint8*) textbuf->pixels + y * textbuf->pitch + offset;

			src = (Uint8*) (glyph->pixmap.buffer + glyph->pixmap.pitch * row);

			memcpy(dst, src, width);
		}

		xstart += glyph->advance;
		if ( font->style & TTF_STYLE_BOLD ) {
			xstart += font->glyph_overhang;
		}
		prev_index = glyph->index;
	}

	if(NULL == glyph)
	{
		SDL_FreeSurface( textbuf );
		return NULL;
	}

	/* Handle the underline style */
	if( font->style & TTF_STYLE_UNDERLINE ) {
		row = font->ascent - font->underline_offset - 1;
		if ( row >= textbuf->h) {
			row = (textbuf->h-1) - font->underline_height;
		}
		dst = (Uint8 *)textbuf->pixels + row * textbuf->pitch;
		for ( row=font->underline_height; row>0; --row ) {
			memset(dst, 0xFF, textbuf->w);
			dst += textbuf->pitch;
		}
	}
	textbuf->minx = glyph->minx;
	//textbuf->miny = glyph->miny;
	return(textbuf);
}

SDL_Surface *TTF_RenderNAME_Blended(TTF_Font *font,
		char *text, SDL_Color fg)
{
	int xstart;
	int width, height;
	SDL_Surface *textbuf;
	Uint8 *src;
	Uint8 *dst;
	int row;
	c_glyph *glyph = NULL;
	FT_Error error;
	FT_Long use_kerning;
	FT_UInt prev_index = 0;

	/* Get the dimensions of the text surface */
	if ( (TTF_SizeNAME(font, text, &width, NULL) < 0) || !width ) {
		gxlogd("Text has zero width");
		return(NULL);
	}
	height = font->height;

	textbuf = SDL_AllocSurface(SDL_SWSURFACE, width, height, 8,
			0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	if ( textbuf == NULL ) {
		return(NULL);
	}

	/* Adding bound checking to avoid all kinds of memory corruption errors
	   that may occur. */
	memset(textbuf->pixels, 0, textbuf->pitch * textbuf->h);
	//dst_check = (Uint8*)textbuf->pixels + textbuf->pitch * textbuf->h;

	/* check kerning */
	use_kerning = FT_HAS_KERNING( font->face );

	/* Load and render each character */
	xstart = 0;

		error = //Load_Glyph_Byname( font, text, font->subscript, CACHED_METRICS|CACHED_PIXMAP);
		Cache_Glyph_Byname(font, text, CACHED_METRICS|CACHED_PIXMAP);
		if( error ) {
			SDL_FreeSurface( textbuf );
			return NULL;
		}
		glyph = font->subscript;

		/*Undefined character code*/
		if(0 == glyph->index)
		{
			return NULL;
		}

		/* Ensure the width of the pixmap is correct. On some cases,
		 * freetype may report a larger pixmap than possible.*/
		width = glyph->pixmap.width;
		if ((width > glyph->maxx - glyph->minx) && !(font->style & TTF_STYLE_BOLD)) {
			width = glyph->maxx - glyph->minx;
		}
		/* do kerning, if possible AC-Patch */
		if ( use_kerning && prev_index && glyph->index ) {
			FT_Vector delta;
			FT_Get_Kerning( font->face, prev_index, glyph->index, ft_kerning_default, &delta );
			xstart += delta.x >> 6;
		}

		/* Compensate for the wrap around bug with negative minx's */
		if ( glyph->minx < 0 ) {
			xstart -= glyph->minx;
		}

		for ( row = 0; row < glyph->pixmap.rows; ++row ) {
			/* Make sure we don't go either over, or under the
			 * limit */
			if ( row+glyph->yoffset < 0 ) {
				continue;
			}
			if ( row+glyph->yoffset >= textbuf->h ) {
				continue;
			}
			dst = (Uint8*) textbuf->pixels +
				(row+glyph->yoffset) * textbuf->pitch +
				xstart + glyph->minx;

			/* Added code to adjust src pointer for pixmaps to
			 * account for pitch.
			 * */
			src = (Uint8*) (glyph->pixmap.buffer + glyph->pixmap.pitch * row);
			memcpy(dst, src, width);
		}

		xstart += glyph->advance;
		if ( font->style & TTF_STYLE_BOLD ) {
			xstart += font->glyph_overhang;
		}
		prev_index = glyph->index;

	if(NULL == glyph)
	{
		SDL_FreeSurface( textbuf );
		return NULL;
	}

	/* Handle the underline style */
	if( font->style & TTF_STYLE_UNDERLINE ) {
		row = font->ascent - font->underline_offset - 1;
		if ( row >= textbuf->h) {
			row = (textbuf->h-1) - font->underline_height;
		}
		dst = (Uint8 *)textbuf->pixels + row * textbuf->pitch;
		for ( row=font->underline_height; row>0; --row ) {
			memset(dst, 0xFF, textbuf->w);
			dst += textbuf->pitch;
		}
	}
	textbuf->minx = glyph->minx;
	//textbuf->miny = glyph->miny;
	return(textbuf);

}

image_desc *ttf_draw_subscript(void *surface,
		TTF_Font *font,
		const unsigned int *word,
		int x,
		int y,
		int bpp,
		int color,
		int sub_index)
{
	unsigned int width = 0, height = 0;
	unsigned int i = 0;
	image_desc *char_image = NULL;
	unsigned char *data = NULL;
	SDL_Color fg_color;
	SDL_Surface *sdl_surface = NULL;

	if(TTF_SizeSubScript(font, word, (int *)&width, (int *)&height, sub_index) < 0)
	{
		return (NULL);
	}

	sdl_surface = TTF_RenderSubScript_Blended(font, word, fg_color, sub_index);
	if(NULL == sdl_surface)
	{
		return (NULL);
	}

	char_image = (image_desc *)GUI_MALLOCZ(sizeof(image_desc));
	if(NULL == char_image) {
		SDL_FreeSurface(sdl_surface);
		return (NULL);
	}
	memset(char_image, 0, sizeof(image_desc));

	char_image->bpp    = 8;
	char_image->width  = width;
	char_image->height = height;
	char_image->color  = color;
	char_image->effect = EFFECT_SRC_AND_DST;
	char_image->size   = width * height * char_image->bpp / 8;

	char_image->hw_accel = 0;
	char_image->data = data = GUI_MALLOCZ(char_image->size);

	if (NULL == data) {
		GUI_FREE(char_image);
		return (NULL);
	}

	for(i = 0; i < sdl_surface->h; i++) {
		memcpy(data + i * width,
				((char *)sdl_surface->pixels) + i * sdl_surface->pitch,
				width);
	}

	if((sdl_surface->minx < 0) && (*word > 0x0500))
	    char_image->xoff = sdl_surface->minx;
	else
	    char_image->xoff = 0;

	SDL_FreeSurface(sdl_surface);

	return (char_image);
}

int ttf_get_xoff(unsigned int c)
{
	unsigned int word[2] = {0};
	SDL_Surface *sdl_surface = NULL;
	struct gdi_font *pFont = NULL;
	TTF_Font *font_data;
	SDL_Color fg_color;
	int xoff = 0;

	word[0] = c;
	word[1] = 0;
	pFont = gxgdi_get_font();
	if(NULL == pFont)
	{
		return (0);
	}

	if(TTF_FONT_TYPE != pFont->family)
		return (0);

	font_data = (TTF_Font *)(pFont->font_data);
	if(NULL == font_data)
	{
		return (0);
	}

	sdl_surface = TTF_RenderUNICODE_Blended(font_data, word, fg_color);
	if(NULL == sdl_surface)
	{
		return (0);
	}

	if((sdl_surface->minx < 0) && (*word > 0x0500))
	    xoff = sdl_surface->minx;
	else
	    xoff = 0;

	SDL_FreeSurface(sdl_surface);

	return (xoff);
}

image_desc *ttf_draw_char(void *surface,
		TTF_Font *font,
		const unsigned int *word,
		int x,
		int y,
		int bpp,
		int color)
{
	GuiCore *gui_core = GUI_GetCurrent();
	unsigned int width = 0, height = 0;
	unsigned int i = 0;
	image_desc *char_image = NULL;
	unsigned char *data = NULL;
	SDL_Color fg_color;
	SDL_Surface *sdl_surface = NULL;

	if(TTF_SizeUNICODE(font, word, (int *)&width, (int *)&height) < 0)
	{
		return (NULL);
	}

	sdl_surface = TTF_RenderUNICODE_Blended(font, word, fg_color);
	if(NULL == sdl_surface)
	{
		return (NULL);
	}

	char_image = (image_desc *)GUI_MALLOCZ(sizeof(image_desc));
	if(NULL == char_image) {
		SDL_FreeSurface(sdl_surface);
		return (NULL);
	}

	char_image->bpp    = 8;
	char_image->effect = EFFECT_SRC_AND_DST;
	char_image->width  = width;
	char_image->height = height;
	char_image->color  = color;
	char_image->size   = width * height * char_image->bpp / 8;

	char_image->hw_accel = 0;
	char_image->data = data = GUI_MALLOCZ(char_image->size);

	if (NULL == data) {
		GUI_FREE(char_image);
		return (NULL);
	}

	for(i = 0; i < sdl_surface->h; i++) {
		memcpy(data + i * width,
		       ((char *)sdl_surface->pixels) + i * sdl_surface->pitch,
		       width);
	}

	if((sdl_surface->minx < 0) && (*word > 0x0500))
	    char_image->xoff = sdl_surface->minx;
	else
	    char_image->xoff = 0;

	SDL_FreeSurface(sdl_surface);

	return (char_image);
}

image_desc *ttf_draw_char_byname(void *surface,
		TTF_Font *font,
		char *word,
		int x,
		int y,
		int bpp,
		int color)
{
	GuiCore *gui_core = GUI_GetCurrent();
	unsigned int width = 0, height = 0;
	unsigned int i = 0;
	image_desc *char_image = NULL;
	unsigned char *data = NULL;
	SDL_Color fg_color;
	SDL_Surface *sdl_surface = NULL;

	if(TTF_SizeNAME(font, word, (int *)&width, (int *)&height) < 0)
	{
		return (NULL);
	}

	sdl_surface = TTF_RenderNAME_Blended(font, word, fg_color);
	if(NULL == sdl_surface)
	{
		return (NULL);
	}

	char_image = (image_desc *)GUI_MALLOCZ(sizeof(image_desc));
	if(NULL == char_image) {
		SDL_FreeSurface(sdl_surface);
		return (NULL);
	}
	memset(char_image, 0, sizeof(image_desc));

	char_image->bpp    = 8;
	char_image->effect = EFFECT_SRC_AND_DST;

	char_image->width  = width;
	char_image->height = height;
	char_image->color  = color;
	char_image->size   = width * height * char_image->bpp / 8;

	char_image->hw_accel = 0;
	char_image->data = data = GUI_MALLOCZ(char_image->size);

	if (NULL == data) {
		GUI_FREE(char_image);
		return (NULL);
	}

	for(i = 0; i < sdl_surface->h; i++) {
		memcpy(data + i * width,
				((char *)sdl_surface->pixels) + i * sdl_surface->pitch,
				width);
	}

	if((sdl_surface->minx < 0) && (*word > 0x0500))
	    char_image->xoff = sdl_surface->minx;
	else
	    char_image->xoff = 0;

	SDL_FreeSurface(sdl_surface);

	return (char_image);

}

image_desc *ttf_draw_sub_byname(void *surface,
		TTF_Font *font,
		char *word,
		int x,
		int y,
		int bpp,
		int color)
{
	unsigned int width = 0, height = 0;
	unsigned int i = 0;
	image_desc *char_image = NULL;
	unsigned char *data = NULL;
	SDL_Color fg_color;
	SDL_Surface *sdl_surface = NULL;

	if(TTF_SizeNAME(font, word, (int *)&width, (int *)&height) < 0)
	{
		return (NULL);
	}

	sdl_surface = TTF_RenderNAME_Blended(font, word, fg_color);
	if(NULL == sdl_surface)
	{
		return (NULL);
	}

	char_image = (image_desc *)GUI_MALLOCZ(sizeof(image_desc));
	if(NULL == char_image) {
		SDL_FreeSurface(sdl_surface);
		return (NULL);
	}
	memset(char_image, 0, sizeof(image_desc));

	char_image->bpp    = 8;
	char_image->effect = EFFECT_SRC_AND_DST;

	char_image->width  = width;
	char_image->height = height;
	char_image->color  = color;
	char_image->size   = width * height * char_image->bpp / 8;

	char_image->hw_accel = 0;
	char_image->data = data = GUI_MALLOCZ(char_image->size);

	if (NULL == data) {
		GUI_FREE(char_image);
		return (NULL);
	}

	for(i = 0; i < sdl_surface->h; i++) {
		memcpy(data + i * width,
				((char *)sdl_surface->pixels) + i * sdl_surface->pitch,
				width);
	}

	if((sdl_surface->minx < 0) && (*word > 0x0500))
	    char_image->xoff = sdl_surface->minx;
	else
	    char_image->xoff = 0;
	char_image->xoff = sdl_surface->minx;

	SDL_FreeSurface(sdl_surface);

	return (char_image);

}

void ttf_get_bearingheight(TTF_Font *font, int c, int *yoffset, int *height)
{
	c_glyph *glyph = NULL;
	FT_Error error;

	error = Cache_Glyph(font, c, CACHED_METRICS|CACHED_PIXMAP);
	if( error ) {
		return;
	}

	glyph = font->current;
	if(glyph) {
		*yoffset = glyph->yoffset;
		*height = glyph->pixmap.rows;
	}
}

bool TTF_IsExist(TTF_Font *font, unsigned int ch)
{
	if(FT_Get_Char_Index( font->face, ch )) {
		return (GUI_TRUE);
	} else {
		return (GUI_FALSE);
	}
}

void TTF_Quit( void )
{
	if ( TTF_initialized ) {
		if ( --TTF_initialized == 0 ) {
			FT_Done_FreeType( library );
		}
	}
}



