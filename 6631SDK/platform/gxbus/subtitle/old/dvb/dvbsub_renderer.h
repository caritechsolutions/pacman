/*
   libdvbsub - DVB subtitle stream parsing and rendering library

   Copyright (C) 2004 Harri Forsgren

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#ifndef __DVBSUB_RENDERER_H__
#define __DVBSUB_RENDERER_H__

#include <stdint.h>
#include "dvbsub_segments.h"
#include "dvbsub_decoder.h"
#include "gxcore.h"

__BEGIN_DECLS


/*
 * To create subtitle renderer you must implement following callback functions
 * and allocate space for 256 pseudo color entries.
 */


/*
 * Function that draws a horizontal line.
 * Parameters:
 *  priv: private data possibly meaningful to called function
 *  x, y: left ending point of a line
 *  length: line length
 *  thickness: line thickness (1 or 2), second line is drawn below the
 *             first line
 *  pseudo_color: references drawing color
 */
typedef void (*dvbsub_hline_function_t)(void *priv, int x, int y, int length,
                                        int thickness, uint8_t pseudo_color);


/*
 * Draw a string.
 * Parameters:
 *  priv: private data
 *  x, y: top left corner?
 *  width, height: maximum string width/height in pixels
 *  str: decoding a string is left to this function
 *  length: string length
 *  fgcolor: foreground color of characters
 *  bgcolor: background color of characters whatever that means
 */
typedef void (*dvbsub_draw_string_function_t)(void *priv, int x, int y, int width, int height, const uint16_t *str, int length, uint8_t fgcolor, uint8_t bgcolor);


/*
 * Maps pseudo color to actual color on target frame buffer.
 * Parameters:
 *  priv: private data possibly meaningful to called function
 *  pseudo_color: color to map
 *  Y, Cb, Cr: color components
 *  T: level of transparency (0=full transparency)
 */
typedef void (*dvbsub_set_color_function_t)(void *priv, uint8_t pseudo_color, uint8_t Y, uint8_t Cb, uint8_t Cr, uint8_t T);


typedef struct dvbsub_rendering_context_s {
    /* Set by user */
    dvbsub_hline_function_t                     hline_callback;
    dvbsub_draw_string_function_t               string_callback;
    dvbsub_set_color_function_t                 set_color_callback;
    void*                                       private_data;
    const dvbsub_display_set_t*                 display_set;
    dvbsub_region_composition_segment_t*        region_composition;
    /* Used by renderer */
    int                                         have_holes; /* non_modifying_colour_flag */
    uint8_t                                     CLUT_2bit_to_8bit_map[4];
    uint8_t                                     CLUT_4bit_to_8bit_map[16];
} dvbsub_rendering_context_t;


void dvbsub_render_region_composition(dvbsub_rendering_context_t *cntx);


/*
 * Create a rendering context.
 * Parameters:
 *  display_set: display set to render
 *  region_id: Region in page to render
 *  hline_callback: function to draw a horizontal line (may be NULL)
 *  string_callback: function to draw a string (may be NULL)
 *  set_color_callback: function to map pseudo color to display color (may be
 *                      NULL)
 *  priv: private data to callback functions
 */
dvbsub_rendering_context_t*
dvbsub_rendering_context_create(dvbsub_hline_function_t hline_callback,
                                dvbsub_draw_string_function_t string_callback,
                                dvbsub_set_color_function_t set_color_callback,
                                void *priv);

void dvbsub_rendering_context_destroy(dvbsub_rendering_context_t *cntx);

void dvbsub_rendering_color_map(const dvbsub_rendering_context_t *cntx);
/*
 * Returns background color for region. Value is either in range 0-255 for
 * pseudo color background, or something else if unspecified/transparent.
 */
int dvbsub_get_region_background_color(const dvbsub_region_composition_segment_t *region);

__END_DECLS

#endif /* __DVBSUB_RENDERER_H__ */
