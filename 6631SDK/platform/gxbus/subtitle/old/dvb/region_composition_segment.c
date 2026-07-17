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

#include "gx_mem.h"
#include "sub_debug.h"
#include "dvbsub_segments.h"


dvbsub_region_composition_segment_t *dvbsub_region_composition_segment_parse(const uint8_t* segment_data) {
    int i;
    dvbsub_region_composition_segment_t *segment;

    uint16_t regions_length =
        ((uint16_t)segment_data[4] << 8) + segment_data[5] - 10;

    int number_of_regions = 0;
    uint16_t pos = 16;
    while (pos < 16 + regions_length) {
        ++number_of_regions;
        if (segment_data[pos + 2] == 0x01 ||
            segment_data[pos + 2] == 0x02)
            pos += 8;
        else
            pos += 6;
    }

/*     dvbsub_region_composition_segment_t *segment = */
/*         (dvbsub_region_composition_segment_t*)av_malloc(sizeof(dvbsub_region_composition_segment_t)); */

    segment = (dvbsub_region_composition_segment_t*)av_malloc(sizeof(dvbsub_region_composition_segment_t) + number_of_regions*sizeof(dvbsub_region_composition_region_t));

    if (segment == NULL)
        return NULL;

    segment->region_id = segment_data[6];
    segment->region_version_number = segment_data[7] >> 4;
    segment->region_fill_flag = (segment_data[7] & 0x08) != 0;
    segment->region_width =
        ((uint16_t)segment_data[8] << 8) | segment_data[9];
    segment->region_height =
        ((uint16_t)segment_data[10] << 8) | segment_data[11];
    segment->region_level_of_compatibility = segment_data[12] >> 5;
    segment->region_depth = (segment_data[12] & 0x0c) >> 2;
    segment->CLUT_id = segment_data[13];
    segment->region_8bit_pixel_code = segment_data[14];
    segment->region_4bit_pixel_code = segment_data[15] >> 4;
    segment->region_2bit_pixel_code = (segment_data[15] & 0x0c) >> 2;
    segment->number_of_regions = number_of_regions;

/*     gxlogd ( "Region composition segment: region_id=%d, version=%d, fill=%d, w=%d, h=%d, compatibility=%d, depth=%d, CLUT id=%d, 8bit=%d, 4bit=%d, 2bit=%d\n ", segment->region_id, segment->region_version_number, segment->region_fill_flag, segment->region_width, segment->region_height, segment->region_level_of_compatibility, segment->region_depth, segment->CLUT_id, segment->region_8bit_pixel_code, segment->region_4bit_pixel_code, segment->region_2bit_pixel_code); */

    for (pos=16, i=0; i < segment->number_of_regions; pos+=6, ++i) {
        segment->regions[i].object_id =
            ((uint16_t)segment_data[pos] << 8) | segment_data[pos + 1];
        segment->regions[i].object_type = segment_data[pos + 2] >> 6;
        segment->regions[i].object_provider_flag =
            (segment_data[pos + 2] & 0x30) >> 4;
        segment->regions[i].object_horizontal_position =
            (((uint16_t)segment_data[pos + 2] & 0x0f) << 8) |
            segment_data[pos + 3];
        segment->regions[i].object_vertical_position =
            (((uint16_t)segment_data[pos + 4] & 0x0f) << 8) |
            segment_data[pos + 5];
        if (segment->regions[i].object_type == 0x01 ||
            segment->regions[i].object_type == 0x02) {
            segment->regions[i].foreground_pixel_code = segment_data[pos + 6];
            segment->regions[i].background_pixel_code = segment_data[pos + 7];
            pos += 2;
        } else {
            segment->regions[i].foreground_pixel_code = 0;
            segment->regions[i].background_pixel_code = 0;
        }

/*      gxlogd ( " (object_id=%d, type=%d, provider=%d, x=%d, y=%d, foreground=%d, background=%d)", segment->regions[i].object_id, segment->regions[i].object_type, segment->regions[i].object_provider_flag, segment->regions[i].object_horizontal_position, segment->regions[i].object_vertical_position, segment->regions[i].foreground_pixel_code, segment->regions[i].background_pixel_code); */
    }


    segment->next_segment = NULL;

    return segment;
}


void dvbsub_region_composition_segment_destroy(dvbsub_region_composition_segment_t* segment) {
    if (segment == NULL)
        return;
    av_free(segment);
}


dvbsub_region_composition_segment_t *dvbsub_region_composition_segment_copy(const dvbsub_region_composition_segment_t* segment) {
    dvbsub_region_composition_segment_t *s;

    if (segment == NULL)
        return NULL;

    s = (dvbsub_region_composition_segment_t*)av_malloc(sizeof(dvbsub_region_composition_segment_t) + segment->number_of_regions*sizeof(dvbsub_region_composition_region_t));
    if (s == NULL)
        return NULL;

    memcpy(s, segment, sizeof(dvbsub_region_composition_segment_t) + segment->number_of_regions*sizeof(dvbsub_region_composition_region_t));
    s->next_segment = NULL;

    return s;
}
