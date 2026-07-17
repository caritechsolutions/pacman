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


dvbsub_page_composition_segment_t* dvbsub_page_composition_segment_parse(const uint8_t* segment_data)
{
    int i;
    uint16_t pos;
    uint16_t region_compositions_length =
        ((uint16_t)segment_data[4] << 8) + segment_data[5] - 2;

    uint16_t number_of_region_compositions = region_compositions_length / 6;

    if (number_of_region_compositions == 0) {
        return NULL;
    }
    
    dvbsub_page_composition_segment_t* segment =
        (dvbsub_page_composition_segment_t*)av_malloc(sizeof(dvbsub_page_composition_segment_t) + number_of_region_compositions*sizeof(dvbsub_page_composition_region_t));

    if (segment == NULL)
        return NULL;

    segment->page_time_out = segment_data[6];
    segment->page_version_number = segment_data[7] >> 4;
    segment->page_state = (segment_data[7] & 0x0f) >> 2;

    //if (segment->page_state == 0) {
    //    gxlogd("segment->page_state=%d\n", segment->page_state);
    //}

    segment->number_of_region_compositions = number_of_region_compositions;

    for (pos=8, i=0; i < number_of_region_compositions; pos+=6, ++i) {
        segment->regions[i].region_id = segment_data[pos];
        segment->regions[i].region_horizontal_address =
            ((uint16_t)segment_data[pos + 2] << 8) | segment_data[pos + 3];
        segment->regions[i].region_vertical_address =
            ((uint16_t)segment_data[pos + 4] << 8) | segment_data[pos + 5];
    }

    return segment;
}


void dvbsub_page_composition_segment_destroy(dvbsub_page_composition_segment_t* segment) {
    if (segment == NULL)
        return;

/*     av_free(segment->regions); */
    av_free(segment);
}


dvbsub_page_composition_segment_t *dvbsub_page_composition_segment_copy(const dvbsub_page_composition_segment_t* segment) {
    dvbsub_page_composition_segment_t *s;

    if (segment == NULL)
        return NULL;

/*     dvbsub_page_composition_region_t *regions = (dvbsub_page_composition_region_t*)av_malloc(sizeof(dvbsub_page_composition_region_t)*segment->number_of_region_compositions); */
/*     if (regions == NULL) */
/*         return NULL; */

    s = (dvbsub_page_composition_segment_t*)av_malloc(sizeof(dvbsub_page_composition_segment_t) + segment->number_of_region_compositions*sizeof(dvbsub_page_composition_region_t));
    if (s == NULL)
        return NULL;

    memcpy(s, segment, sizeof(dvbsub_page_composition_segment_t) + segment->number_of_region_compositions*sizeof(dvbsub_page_composition_region_t));


/*     dvbsub_page_composition_segment_t *s = (dvbsub_page_composition_segment_t*)av_malloc(sizeof(dvbsub_page_composition_segment_t)); */
/*     if (s == NULL) */
/*         return NULL; */

/*     memcpy(s, segment, sizeof(dvbsub_page_composition_segment_t)); */
/*     memcpy(regions, segment->regions, sizeof(dvbsub_page_composition_region_t)*segment->number_of_region_compositions); */
/*     s->regions = regions; */
    return s;
}
