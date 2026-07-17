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
#include "dvbsub_segments.h"
#include "gxcore.h"


dvbsub_object_data_segment_t*
dvbsub_object_data_segment_parse(const uint8_t *segment_data)
{
    uint8_t     number_of_codes      = 0;
    uint16_t    top_field_length     = 0;
    uint16_t    bottom_field_length  = 0;
    uint8_t     object_coding_method = (segment_data[8] & 0x0f) >> 2;
    uint16_t    segment_length       = sizeof(dvbsub_object_data_segment_t);

    dvbsub_object_data_segment_t *segment;


    if (object_coding_method == 0x00) {
        top_field_length    = ((uint16_t)segment_data[9] << 8)|segment_data[10];
        bottom_field_length = ((uint16_t)segment_data[11] << 8)|segment_data[12];
        segment_length      += top_field_length + bottom_field_length;
    } else if (object_coding_method == 0x01) {
        number_of_codes = segment_data[9];
        segment_length += 2 * number_of_codes;
    }

    segment = (dvbsub_object_data_segment_t*)av_malloc(segment_length);
    if (segment == NULL) {
        return NULL;
    }
    segment->object_id = ((uint16_t)segment_data[6] << 8) | segment_data[7];
    segment->object_version_number = segment_data[8] >> 4;
    segment->object_coding_method = object_coding_method;
    segment->non_modifying_colour_flag = (segment_data[8] & 0x02) != 0;

    if (segment->object_coding_method == 0x00) { /* pixels */
        segment->data.pixels.top_field_data_block_length =
            top_field_length;
        segment->data.pixels.bottom_field_data_block_length =
            bottom_field_length;
        memcpy(segment->data.pixels.sub_blocks, segment_data + 13,
               top_field_length + bottom_field_length);
    } else if (segment->object_coding_method == 0x01) { /* string */
        segment->data.string.number_of_codes = number_of_codes;
        memcpy(segment->data.string.character_codes, segment_data + 10,
               segment->data.string.number_of_codes * 2);

    }

    segment->next_segment = NULL;

    return segment;
}


void dvbsub_object_data_segment_destroy(dvbsub_object_data_segment_t* segment)
{
    if (segment == NULL) {
        return;
    }
    av_free(segment);
}


dvbsub_object_data_segment_t*
dvbsub_object_data_segment_copy(const dvbsub_object_data_segment_t* segment)
{
    uint16_t                        segment_length;
    dvbsub_object_data_segment_t*   s;

    if (segment == NULL) {
        return NULL;
    }

    segment_length = sizeof(dvbsub_object_data_segment_t);
    if (segment->object_coding_method == 0x00) {
        segment_length +=
            segment->data.pixels.top_field_data_block_length +
            segment->data.pixels.bottom_field_data_block_length;
    }
    else {
        segment_length += 2 * segment->data.string.number_of_codes;
    }
    s = (dvbsub_object_data_segment_t*)av_malloc(segment_length);
    if (s == NULL) {
        return NULL;
    }
    memcpy(s, segment, segment_length);
    s->next_segment = NULL;

    return s;
}
