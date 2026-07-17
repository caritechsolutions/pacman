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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "gx_mem.h"
#include "dvbsub_segments.h"
#include "dvbsub_renderer.h"
#include "dvbsub_helpers.h"
#include "clut_tables.h"
#include "sub_debug.h"

/* #define DEBUG_RENDERER */


static inline int get_bit(const uint8_t *data, int index) {
    int byte_idx = index / 8;
    uint8_t mask = 1 << (index & 0x07);
    return (data[byte_idx] & mask) != 0;
}


static inline int get_nibble(const uint8_t *data, int index) {
    int byte_idx = index / 2;
    int result;
    if (index & 0x01)
        result = data[byte_idx] & 0x0f;
    else
        result = data[byte_idx] >> 4;
    return result;
}


static inline void draw_pixels_2bit(dvbsub_rendering_context_t *cntx, int x,
                                    int y, int length, int line_thickness,
                                    uint8_t pseudo_color) {
    uint8_t pcolor;
    if (pseudo_color == 0x01 && cntx->have_holes)
        return;
    pcolor = cntx->CLUT_2bit_to_8bit_map[pseudo_color];
    cntx->hline_callback(cntx->private_data, x, y, length, line_thickness,
                         pcolor);
}


static inline void draw_pixels_4bit(dvbsub_rendering_context_t *cntx, int x,
                                    int y, int length, int line_thickness,
                                    uint8_t pseudo_color)
{
    if (pseudo_color == 0x01 && cntx->have_holes)
        return;
    //pcolor = cntx->CLUT_4bit_to_8bit_map[pseudo_color];
    cntx->hline_callback(cntx->private_data, x, y, length, line_thickness,
                         pseudo_color);
}


static inline void draw_pixels_8bit(dvbsub_rendering_context_t *cntx, int x,
                                    int y, int length, int line_thickness,
                                    uint8_t pseudo_color) {
    if (pseudo_color == 0x01 && cntx->have_holes)
        return;
    cntx->hline_callback(cntx->private_data, x, y, length, line_thickness,
                         pseudo_color);
}


static int parse_2bit_pixel_code_string(dvbsub_rendering_context_t *cntx,
                                        int *x, int y, int line_thickness,
                                        const uint8_t *data, int length) {
    int idx = 0;

    /* Check position just in case if data is corrupted */
    while (idx/8 < length) {
        int length, color;
        if (get_bit(data, idx) != 0 || get_bit(data, idx + 1) != 0) {
            /* 01 - 11 */
            length = 1;
            color = (get_bit(data, idx) << 1) | get_bit(data, idx + 1);
            idx += 2;
        } else {
            idx += 2;
            if (get_bit(data, idx) == 1) { /* switch_1 */
                /* 00 1L LL CC */
                length = ((get_bit(data, idx + 1) << 2) |
                          (get_bit(data, idx + 2) << 1) |
                          get_bit(data, idx + 3)) + 3;
                color = (get_bit(data, idx + 4) << 1) | get_bit(data, idx + 5);
                idx += 6;
            } else {
                ++idx;
                if (get_bit(data, idx) == 0) { /* switch_2 */
                    ++idx;
                    if (get_bit(data, idx) == 1) { /* switch_3_1 */
                        ++idx;
                        if (get_bit(data, idx) == 0) { /* switch_3_2 */
                            /* 00 00 10 LL LL CC */
                            length = ((get_bit(data, idx + 1) << 3) |
                                      (get_bit(data, idx + 2) << 2) |
                                      (get_bit(data, idx + 3) << 1) |
                                      get_bit(data, idx + 4)) + 12;
                            color = (get_bit(data, idx + 5) << 1) |
                                get_bit(data, idx + 6);
                            idx += 7;
                        } else {
                            /* 00 00 11 LL LL LL LL CC */
                            length = ((get_bit(data, idx + 1) << 7) |
                                      (get_bit(data, idx + 2) << 6) |
                                      (get_bit(data, idx + 3) << 5) |
                                      (get_bit(data, idx + 4) << 4) |
                                      (get_bit(data, idx + 5) << 3) |
                                      (get_bit(data, idx + 6) << 2) |
                                      (get_bit(data, idx + 7) << 1) |
                                      get_bit(data, idx + 8)) + 29;
                            color = (get_bit(data, idx + 9) << 1) |
                                get_bit(data, idx + 10);
                            idx += 11;
                        }
                    } else {
                        ++idx;
                        if (get_bit(data, idx) == 1) {
                            /* 00 00 01 */
                            length = 2;
                            color = 0;
                            ++idx;
                        } else {
                            /* 00 00 00 */
                            return (idx + 1 + 7) / 8;
                        }
                    }
                } else {
                    /* 00 01 */
                    length = 1;
                    color = 0;
                    ++idx;
                }
            }
        }
        draw_pixels_2bit(cntx, *x, y, length, line_thickness, color);
        x += length;
    }
    return length;
}


static int parse_4bit_pixel_code_string(dvbsub_rendering_context_t *cntx,
                                        int *x, int y, int line_thickness,
                                        const uint8_t *data, int length) {
    int idx = 0;

    int t1 = 0, t2 = 0, t3 = 0, t4 = 0, t5 = 0, t6 = 0, t7 = 0;

    /* Check position just in case if data is corrupted */
    while (idx/2 < length) {
        int length, color;
        if (get_nibble(data, idx) > 0x0) { /* 0001 - 1111 */
            length = 1;
            color = get_nibble(data, idx);
            ++idx;
            ++t1;
        } else { /* 0000 */
            int nibble = get_nibble(data, idx + 1);
            idx += 2;
            if (nibble == 0x0) { /* 0000 0000 */
/*              gxlogd ( "%d %d %d %d %d %d %d\n", t1, t2, t3, t4, t5, t6, t7); */
                return (idx + 1)/2;
            } else if (nibble == 0xc) { /* 0000 1100 */
                length = 1;
                color = 0;
                ++t2;
            } else if (nibble == 0xd) { /* 0000 1101 */
                length = 2;
                color = 0;
                ++t3;
            } else if (nibble == 0xe) { /* 0000 1110 LLLL CCCC */
                length = get_nibble(data, idx) + 9;
                color = get_nibble(data, idx + 1);
                idx += 2;
                ++t4;
            } else if (nibble == 0xf) { /* 0000 1111 LLLL LLLL CCCC */
                length = (get_nibble(data, idx) << 4 |
                    get_nibble(data, idx + 1)) + 25;
                color = get_nibble(data, idx + 2);
                idx += 3;
                ++t5;
            } else if (nibble & 0x8) { /* 0000 10LL CCCC */
                length = (nibble & 0x3) + 4;
                color = get_nibble(data, idx);
                ++idx;
                ++t6;
            } else { /* 0000 0LLL */
                length = nibble + 2; /* Specs say +3 */
                color = 0;
                ++t7;
            }
        }
        draw_pixels_4bit(cntx, *x, y, length, line_thickness, color);
        *x += length;
    }
    return length;
}

static int parse_8bit_pixel_code_string(dvbsub_rendering_context_t *cntx,
                                        int *x, int y, int line_thickness,
                                        const uint8_t *data, int length) {
    int idx = 0;

/*     int debug_total_length = 0; */

    /* Check position just in case if data is corrupted */
    while (idx < length) {
        if (data[idx] != 0x00) {
            draw_pixels_8bit(cntx, *x, y, 1, line_thickness, data[idx]);
            ++*x;
/*             debug_total_length += 1; */
        } else {
            ++idx;
            if (data[idx] == 0x00) {
/*              gxlogd ( "debug_total_length=%d\n", debug_total_length); */
                return idx + 1;
            } else if (data[idx] & 0x80) {
                int length = (data[idx] & 0x7f) + 3;
                draw_pixels_8bit(cntx, *x, y, length, line_thickness,
                                 data[idx + 1]);
                *x += length;
/*                 debug_total_length += (data[idx] & 0x7f) + 3; */
                ++idx;
            } else {
                int length = data[idx] + 1;
                draw_pixels_8bit(cntx, *x, y, length, line_thickness, 0x00);
                *x += length;
/*                 debug_total_length += data[idx] + 1; */
            }
        }
        ++idx;
    }
/*  gxlogd ( "debug_total_length=%d\n", debug_total_length); */
    return length;
}


static int parse_sub_block(dvbsub_rendering_context_t *cntx,
                           int start_x, int y, int line_thickness,
                           const uint8_t *data, int length) {
    int x = start_x;
    int idx = 0;


/*     int debug_data_type = 0; */
    while (idx < length) {
        switch (data[idx++]) { /* Note: idx++ */
        case 0x10:
/*          gxlogd ( "2bit\n"); */
            idx += parse_2bit_pixel_code_string(cntx, &x, y, line_thickness,
                                                data + idx, length - idx);
            break;
        case 0x11:
/*          gxlogd ( "4bit\n"); */
            idx += parse_4bit_pixel_code_string(cntx, &x, y, line_thickness,
                                                data + idx, length - idx);
            break;
        case 0x12:
/*          gxlogd ( "8bit\n"); */
            idx += parse_8bit_pixel_code_string(cntx, &x, y, line_thickness,
                                                data + idx, length - idx);
            break;
        case 0x20: /* We don't care about 2 to 4 bit conversions */
            idx += 2;
            break;
        case 0x21: /* 2 to 8 bit */
/*          gxlogd ( "2 to 8bit\n"); */
            cntx->CLUT_2bit_to_8bit_map[0] = data[idx++];
            cntx->CLUT_2bit_to_8bit_map[1] = data[idx++];
            cntx->CLUT_2bit_to_8bit_map[2] = data[idx++];
            cntx->CLUT_2bit_to_8bit_map[3] = data[idx++];
            break;
        case 0x22: { /* 4 to 8 bit */
            int i;
/*          gxlogd ( "4 to 8bit\n"); */
            for (i = 0; i < 16; ++i)
                cntx->CLUT_4bit_to_8bit_map[i] = data[idx++];
            break;
        }
        case 0xf0: /* end of object line code */
/*          gxlogd ( "Invalid sub-block data type found %d times\n", debug_data_type); */
/*          gxlogd ( "Line %d width=%d\n", y, x); */
            return idx;
        default:
/*             ++debug_data_type; */
/*          gxlogd ( "invalid sub-block data type: %x\n", (int)data[idx]); */
            break;
        }
    }
/*  gxlogd ( "Invalid sub-block data type found %d times\n", debug_data_type); */
    return length;
}


static void dvbsub_render_pixel_data(dvbsub_rendering_context_t *cntx,
                                     int start_x, int start_y,
                                     const dvbsub_object_data_pixels_t *data) {
/*  gxlogd ( "dvbsub_render_pixel_data\n"); */
    int top_idx = 0, bottom_idx = 0;
    int top_length = data->top_field_data_block_length;
    int bottom_length = data->bottom_field_data_block_length;
    int line_thickness = bottom_length == 0 ? 2 : 1;
    int y = 0;

/*     int i; */
/*     for (i = 0; i < top_length; ++i) */
/*         gxlogi ( "%02x", data->sub_blocks[i]); */
/*     gxlogi ( "\n"); */

    while (start_y + y < cntx->region_composition->region_height &&
           (top_idx < top_length || bottom_idx < bottom_length)) {
        if (top_idx < top_length) {
            top_idx += parse_sub_block(cntx, start_x, start_y + y,
                                       line_thickness,
                                       data->sub_blocks + top_idx,
                                       top_length - top_idx);
            y += line_thickness;
        }
        if (bottom_idx < bottom_length) {
            bottom_idx += parse_sub_block(cntx, start_x, start_y + y, 1,
                                          data->sub_blocks + top_length +
                                          bottom_idx,
                                          bottom_length - bottom_idx);
            ++y;
        }
    }
/*  gxlogd ( "top_idx=%d, bottom_idx=%d, y=%d\n", top_idx, bottom_idx, y); */
}


void dvbsub_render_region_composition(dvbsub_rendering_context_t *cntx) {
    dvbsub_object_data_segment_t *ods;
    const dvbsub_region_composition_segment_t *rcs = cntx->region_composition;
    int i;

    for (i = 0; i < rcs->number_of_regions; ++i) {
        const dvbsub_region_composition_region_t *region = &(rcs->regions[i]);
        if (region->object_provider_flag !=
            DVBSUB_OBJECT_PROVIDER_SUBTITLING_STREAM)
            continue;

        ods = dvbsub_find_object_data_segment(cntx->display_set,
                                              region->object_id);
        if (ods == NULL)
            continue;

        //DBG_SUB("Object %d\n", ods->object_id);

        cntx->have_holes = ods->non_modifying_colour_flag;
        switch (ods->object_coding_method) {
        case DVBSUB_OBJECT_CODING_PIXELS:
            if (cntx->hline_callback)
                dvbsub_render_pixel_data(cntx,
                                         region->object_horizontal_position,
                                         region->object_vertical_position,
                                         &ods->data.pixels);
            break;
        case DVBSUB_OBJECT_CODING_STRING:
            if (cntx->string_callback)
                cntx->string_callback(cntx->private_data,
                                      region->object_horizontal_position,
                                      region->object_vertical_position,
                                      rcs->region_width - region->object_horizontal_position,
                                      rcs->region_height - region->object_vertical_position,
                                      ods->data.string.character_codes,
                                      ods->data.string.number_of_codes,
                                      region->foreground_pixel_code,
                                      region->background_pixel_code);
            break;
        }
    }
}


void dvbsub_rendering_color_map(const dvbsub_rendering_context_t *cntx)
{
    dvbsub_CLUT_definition_segment_t *cds;
    int i;

    cds = cntx->display_set->CLUT_definition_segments;
    if (cds->CLUT_entries[0].CLUT_entry_8bit_flag) {
        for (i = 0; i < 256; ++i)
            cntx->set_color_callback(cntx->private_data, i,
                                     dvbsub_default_CLUT_8bit_YCbCrT[i][0],
                                     dvbsub_default_CLUT_8bit_YCbCrT[i][1],
                                     dvbsub_default_CLUT_8bit_YCbCrT[i][2],
                                     dvbsub_default_CLUT_8bit_YCbCrT[i][3]);
    } else if (cds->CLUT_entries[0].CLUT_entry_4bit_flag) {

        for (i = 0; i < 16; ++i) {
            cntx->set_color_callback(cntx->private_data, i,
                                     dvbsub_default_CLUT_4bit_YCbCrT[i][0],
                                     dvbsub_default_CLUT_4bit_YCbCrT[i][1],
                                     dvbsub_default_CLUT_4bit_YCbCrT[i][2],
                                     dvbsub_default_CLUT_4bit_YCbCrT[i][3]);
        }
    } else if (cds->CLUT_entries[0].CLUT_entry_2bit_flag) {
        for (i = 0; i < 4; ++i)
            cntx->set_color_callback(cntx->private_data, i,
                                     dvbsub_default_CLUT_2bit_YCbCrT[i][0],
                                     dvbsub_default_CLUT_2bit_YCbCrT[i][1],
                                     dvbsub_default_CLUT_2bit_YCbCrT[i][2],
                                     dvbsub_default_CLUT_2bit_YCbCrT[i][3]);
    }

    while (cds != NULL) {
        if (cds->CLUT_id == cntx->region_composition->CLUT_id) {
            for (i = 0; i < cds->number_of_CLUT_entries; ++i) {
                cntx->set_color_callback(cntx->private_data,
                                         cds->CLUT_entries[i].CLUT_entry_id,
                                         cds->CLUT_entries[i].Y_value,
                                         cds->CLUT_entries[i].Cb_value,
                                         cds->CLUT_entries[i].Cr_value,
                                         cds->CLUT_entries[i].T_value);

            }
        }
        cds = cds->next_segment;
    }
}


dvbsub_rendering_context_t*
dvbsub_rendering_context_create(dvbsub_hline_function_t hline_callback,
                                dvbsub_draw_string_function_t string_callback,
                                dvbsub_set_color_function_t set_color_callback,
                                void *priv) {
    dvbsub_rendering_context_t *cntx;


    cntx = (dvbsub_rendering_context_t*)av_malloc(sizeof(dvbsub_rendering_context_t));
    if (cntx == NULL)
        return NULL;

    cntx->hline_callback = hline_callback;
    cntx->string_callback = string_callback;
    cntx->set_color_callback = set_color_callback;
    cntx->private_data = priv;

    cntx->display_set = NULL;
    cntx->region_composition = NULL;

    return cntx;
}


void dvbsub_rendering_context_destroy(dvbsub_rendering_context_t *cntx) {
    av_free(cntx);
    cntx = NULL;
}




int dvbsub_get_region_background_color(const dvbsub_region_composition_segment_t *region) {
    int bkcolor;
    if (region == NULL)
        bkcolor = -1;
    else if (region->region_fill_flag) {
        switch (region->region_depth) {
        case 0x01:
            bkcolor = dvbsub_default_CLUT_2bit_to_8bit_map[region->region_2bit_pixel_code];
            break;
        case 0x02:
            bkcolor = dvbsub_default_CLUT_4bit_to_8bit_map[region->region_4bit_pixel_code];
            break;
        case 0x03:
            bkcolor = region->region_8bit_pixel_code;
            break;
        default:
            bkcolor = -1;
        }
    } else
        bkcolor = -1;

    return bkcolor;
}


#ifdef DEBUG_RENDERER
static uint8_t pixel_data_8bit[] = { 0x12, 0x00, 0x00,
                                     0x12, 0x01, 0x02, 0x56, 0xff,
                                     0x00, 0x05,
                                     0x34,
                                     0x00, 0x82, 0x0a,
                                     0x00, 0x00,
                                     0xf0 };

static uint8_t pixel_data_4bit[] = { 0x11, 0x12,
                                     0x01,
                                     0x56,
                                     0x0e, 0x02,
                                     0x00,
                                     0xf0 };


int main(int argc, char *argv[]) {
    int width = 100;

    dvbsub_region_composition_segment_t *rcs =
        (dvbsub_region_composition_segment_t*)av_malloc(sizeof(dvbsub_region_composition_segment_t) + sizeof(dvbsub_region_composition_region_t));
    rcs->next_segment = NULL;
    rcs->region_id = 0;
    rcs->region_fill_flag = 0;
    rcs->region_width = width;
    rcs->region_height = 20;
    rcs->CLUT_id = 0;
    rcs->number_of_regions = 1;
    rcs->regions[0].object_id = 0;
    rcs->regions[0].object_type = DVBSUB_OBJECT_TYPE_BASIC_BITMAP;
    rcs->regions[0].object_provider_flag =
        DVBSUB_OBJECT_PROVIDER_SUBTITLING_STREAM;
    rcs->regions[0].object_horizontal_position = 0;
    rcs->regions[0].object_vertical_position = 0;

    dvbsub_object_data_segment_t *ods =
        (dvbsub_object_data_segment_t*)av_malloc(sizeof(dvbsub_object_data_segment_t) + sizeof(pixel_data_8bit) + sizeof(pixel_data_4bit));
    ods->object_id = 0;
    ods->object_coding_method = DVBSUB_OBJECT_CODING_PIXELS;
    ods->non_modifying_colour_flag = 0;
    ods->data.pixels.top_field_data_block_length =
        sizeof(pixel_data_8bit) + sizeof(pixel_data_4bit);
    ods->data.pixels.bottom_field_data_block_length = 0;
    memcpy(ods->data.pixels.sub_blocks, pixel_data_8bit, sizeof(pixel_data_8bit));
    memcpy(ods->data.pixels.sub_blocks+sizeof(pixel_data_8bit), pixel_data_4bit, sizeof(pixel_data_4bit));

    dvbsub_display_set_t ds;
    ds.page_composition_segment = NULL;
    ds.region_composition_segments = rcs;
    ds.CLUT_definition_segments = NULL;
    ds.object_data_segments = ods;

    dvbsub_rendering_context_t *cntx =
        dvbsub_rendering_context_create(&ds, 0, DVBSUB_COLOR_MODEL_RGBA);

    dvbsub_render_region_composition(cntx);

    int i, j;
    for (j = 0; j < 2; ++j) {
        for (i = 0; i < 4*30; i += 4)
            gxlogi ( "%02x ", cntx->buffer[2*j*width*4 + i]);
        gxlogi ( "\n");
    }

    return 0;
}
#endif
