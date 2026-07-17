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

#ifndef __DVBSUB_HELPERS_H__
#define __DVBSUB_HELPERS_H__

#include "dvbsub_segments.h"
#include "gxcore.h"

__BEGIN_DECLS

static inline dvbsub_region_composition_segment_t*
dvbsub_find_region_composition_segment(const dvbsub_display_set_t *display_set,
                                       uint8_t region_id) {
    dvbsub_region_composition_segment_t *rcs;

    if (display_set == NULL)
        return NULL;

    rcs = display_set->region_composition_segments;
    while (rcs != NULL && rcs->region_id != region_id) {
        rcs = rcs->next_segment;
    }

    return rcs;
}


static inline dvbsub_CLUT_definition_segment_t*
dvbsub_find_CLUT_definition_segment(const dvbsub_display_set_t *display_set,
                                    uint8_t CLUT_id) {
    dvbsub_CLUT_definition_segment_t *cds;

    if (display_set == NULL)
        return NULL;

    cds = display_set->CLUT_definition_segments;
    while (cds != NULL && cds->CLUT_id != CLUT_id)
        cds = cds->next_segment;

    if (cds == NULL && display_set->ancillary_display_set) {
        cds = display_set->ancillary_display_set->CLUT_definition_segments;
        while (cds != NULL && cds->CLUT_id != CLUT_id)
            cds = cds->next_segment;
    }

    return cds;
}


static inline dvbsub_object_data_segment_t*
dvbsub_find_object_data_segment(const dvbsub_display_set_t *display_set,
                                uint16_t object_id) {
    dvbsub_object_data_segment_t *ods;

    if (display_set == NULL)
        return NULL;

    ods = display_set->object_data_segments;
    while (ods != NULL && ods->object_id != object_id)
        ods = ods->next_segment;

    if (ods == NULL && display_set->ancillary_display_set) {
        ods = display_set->ancillary_display_set->object_data_segments;
        while (ods != NULL && ods->object_id != object_id)
            ods = ods->next_segment;
    }

    return ods;
}


__END_DECLS

#endif /* __DVBSUB_HELPERS_H__ */

