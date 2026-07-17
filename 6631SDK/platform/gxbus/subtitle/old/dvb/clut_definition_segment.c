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
#include "gxcore.h"

#include "gx_mem.h"
#include "dvbsub_segments.h"


dvbsub_CLUT_definition_segment_t *dvbsub_CLUT_definition_segment_parse(const uint8_t* segment_data) {
    int i;
    dvbsub_CLUT_definition_segment_t *segment;
    uint16_t CLUT_entries_length =
        ((uint16_t)segment_data[4] << 8) + segment_data[5] - 2;

    int number_of_CLUT_entries = 0;
    uint16_t pos = 8;
    while (pos < 8 + CLUT_entries_length) {
        ++number_of_CLUT_entries;
        if (segment_data[pos + 1] & 0x01)
            pos += 6;
        else
            pos += 4;
    }

    segment = (dvbsub_CLUT_definition_segment_t*)av_malloc(sizeof(dvbsub_CLUT_definition_segment_t) + number_of_CLUT_entries*sizeof(dvbsub_CLUT_entry_t));

    if (segment == NULL)
        return NULL;
    segment->next_segment = NULL;
    segment->CLUT_id = segment_data[6];
    segment->CLUT_version_number = segment_data[7] >> 4;
    segment->number_of_CLUT_entries = number_of_CLUT_entries;

/*  gxlogd ( "CLUT definition segment: id=%d, version=%d\n ", segment->CLUT_id, segment->CLUT_version_number); */

    for (pos=8, i=0; i < segment->number_of_CLUT_entries; pos+=2, ++i) {
        segment->CLUT_entries[i].CLUT_entry_id = segment_data[pos];
        segment->CLUT_entries[i].CLUT_entry_2bit_flag =
            (segment_data[pos + 1] & 0x80) != 0;
        segment->CLUT_entries[i].CLUT_entry_4bit_flag =
            (segment_data[pos + 1] & 0x40) != 0;
        segment->CLUT_entries[i].CLUT_entry_8bit_flag =
            (segment_data[pos + 1] & 0x20) != 0;
        if (segment_data[pos + 1] & 0x01) { /* full_range_flag */
            segment->CLUT_entries[i].Y_value = segment_data[pos + 2];
            segment->CLUT_entries[i].Cr_value = segment_data[pos + 3];
            segment->CLUT_entries[i].Cb_value = segment_data[pos + 4];
            segment->CLUT_entries[i].T_value = segment_data[pos + 5];
            pos += 4;
        } else {
            segment->CLUT_entries[i].Y_value = segment_data[pos + 2] & 0xfc;
            segment->CLUT_entries[i].Cr_value =
                ((segment_data[pos + 2] & 0x03) << 6) |
                ((segment_data[pos + 3] & 0xc0) >> 2);
            segment->CLUT_entries[i].Cb_value = (segment_data[pos + 3] & 0x3c) << 2;
            segment->CLUT_entries[i].T_value = (segment_data[pos + 3] & 0x3) << 6;
            pos += 2;
        }
/*      gxlogd ( " (entry_id=%d, 2bit=%d, 4bit=%d, 8bit=%d, Y=%d, Cr=%d, Cb=%d, T=%d)", segment->CLUT_entries[i].CLUT_entry_id, segment->CLUT_entries[i].CLUT_entry_2bit_flag, segment->CLUT_entries[i].CLUT_entry_4bit_flag, segment->CLUT_entries[i].CLUT_entry_8bit_flag, segment->CLUT_entries[i].Y_value, segment->CLUT_entries[i].Cr_value, segment->CLUT_entries[i].Cb_value, segment->CLUT_entries[i].T_value); */
    }


    return segment;
}


void dvbsub_CLUT_definition_segment_destroy(dvbsub_CLUT_definition_segment_t* segment) {
    if (segment == NULL)
        return;

    av_free(segment);
}


dvbsub_CLUT_definition_segment_t *dvbsub_CLUT_definition_segment_copy(const dvbsub_CLUT_definition_segment_t* segment) {
    dvbsub_CLUT_definition_segment_t *s;

    if (segment == NULL)
        return NULL;

    s = (dvbsub_CLUT_definition_segment_t*)av_malloc(sizeof(dvbsub_CLUT_definition_segment_t) + segment->number_of_CLUT_entries*sizeof(dvbsub_CLUT_entry_t));
    if (s == NULL)
        return NULL;

    memcpy(s, segment, sizeof(dvbsub_CLUT_definition_segment_t) + segment->number_of_CLUT_entries*sizeof(dvbsub_CLUT_entry_t));
    s->next_segment = NULL;
    return s;
}
