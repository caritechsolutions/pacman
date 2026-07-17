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
#include <stdint.h>

#include "gx_mem.h"
#include "dvbsub_decoder.h"
#include "dvbsub_segments.h"
#include "dvbsub_helpers.h"
#include "sub_debug.h"


dvbsub_page_composition_segment_t *dvbsub_page_composition_segment_copy(const dvbsub_page_composition_segment_t* segment);

dvbsub_region_composition_segment_t *dvbsub_region_composition_segment_copy(const dvbsub_region_composition_segment_t* segment);

dvbsub_CLUT_definition_segment_t *dvbsub_CLUT_definition_segment_copy(const dvbsub_CLUT_definition_segment_t* segment);

dvbsub_object_data_segment_t *dvbsub_object_data_segment_copy(const dvbsub_object_data_segment_t* segment);


static void destroy_region_composition_segment_list(dvbsub_display_set_t *display_set) {
    dvbsub_region_composition_segment_t *rcs;

    if (display_set == NULL)
        return;
    rcs = display_set->region_composition_segments;
    while (rcs != NULL) {
        dvbsub_region_composition_segment_t *tmp = rcs;
        rcs = rcs->next_segment;
        dvbsub_region_composition_segment_destroy(tmp);
    }

    display_set->region_composition_segments = NULL;
}


static void destroy_CLUT_definition_segment_list(dvbsub_display_set_t *display_set) {
    dvbsub_CLUT_definition_segment_t *cds;

    if (display_set == NULL)
        return;

    cds = display_set->CLUT_definition_segments;
    while (cds != NULL) {
        dvbsub_CLUT_definition_segment_t *tmp = cds;
        cds = cds->next_segment;
        dvbsub_CLUT_definition_segment_destroy(tmp);
    }

    display_set->CLUT_definition_segments = NULL;
}


static void destroy_object_data_segment_list(dvbsub_display_set_t *display_set) {
    dvbsub_object_data_segment_t *ods;

    if (display_set == NULL)
        return;

    ods = display_set->object_data_segments;
    while (ods != NULL) {
        dvbsub_object_data_segment_t *tmp = ods;
        ods = ods->next_segment;
        dvbsub_object_data_segment_destroy(tmp);
    }

    display_set->object_data_segments = NULL;
}


void dvbsub_update_region_composition_segments(dvbsub_display_set_t *display_set, dvbsub_region_composition_segment_t *segment) {
    dvbsub_region_composition_segment_t *s, *prev;

    if (segment == NULL)
        return;

    if (display_set->region_composition_segments == NULL) {
        display_set->region_composition_segments = segment;
        return;
    }

    s = display_set->region_composition_segments;
    prev = NULL;
    while (s != NULL && s->region_id != segment->region_id) {
        prev = s;
        s = s->next_segment;
    }

    if (s == NULL) {
        /* Note that we don't have to set ready_for_display to 0, because
           it is 0 already, or if 1, this region composition doesn't belong to
           current visible page. */
        segment->next_segment = display_set->region_composition_segments;
        display_set->region_composition_segments = segment;
    } else if (segment->region_version_number != s->region_version_number) {
        segment->next_segment = s->next_segment;
        if (prev == NULL)
            display_set->region_composition_segments = segment;
        else
            prev->next_segment = segment;
        dvbsub_region_composition_segment_destroy(s);
        /* Required object list may have been changed */
        display_set->ready_for_display = 0;
    } else
        dvbsub_region_composition_segment_destroy(segment); /* No update */
}


void dvbsub_update_CLUT_definition_segments(dvbsub_display_set_t *display_set, dvbsub_CLUT_definition_segment_t *segment) {
    dvbsub_CLUT_definition_segment_t *s, *prev;

    if (segment == NULL)
        return;

    if (display_set->CLUT_definition_segments == NULL) {
        display_set->CLUT_definition_segments = segment;
        return;
    }

    s = display_set->CLUT_definition_segments;
    prev = NULL;
    while (s != NULL && s->CLUT_id != segment->CLUT_id) {
        prev = s;
        s = s->next_segment;
    }

    if (s == NULL) {
        segment->next_segment = display_set->CLUT_definition_segments;
        display_set->CLUT_definition_segments = segment;
    } else if (segment->CLUT_version_number != s->CLUT_version_number) {
        segment->next_segment = s->next_segment;
        if (prev == NULL)
            display_set->CLUT_definition_segments = segment;
        else
            prev->next_segment = segment;
        dvbsub_CLUT_definition_segment_destroy(s);
        /* Colors may have been changed */
        display_set->ready_for_display = 0;
    } else
        dvbsub_CLUT_definition_segment_destroy(segment); /* No update */
}


void dvbsub_update_object_data_segments(dvbsub_display_set_t *display_set, dvbsub_object_data_segment_t *segment) {
    dvbsub_object_data_segment_t *s, *prev;

    if (segment == NULL)
        return;

    if (display_set->object_data_segments == NULL) {
        display_set->object_data_segments = segment;
        return;
    }

    s = display_set->object_data_segments;
    prev = NULL;
    while (s != NULL && s->object_id != segment->object_id) {
        prev = s;
        s = s->next_segment;
    }

    if (s == NULL) {
        segment->next_segment = display_set->object_data_segments;
        display_set->object_data_segments = segment;
    } else if (segment->object_version_number != s->object_version_number) {
        segment->next_segment = s->next_segment;
        if (prev == NULL)
            display_set->object_data_segments = segment;
        else
            prev->next_segment = segment;
        dvbsub_object_data_segment_destroy(s);
        /* Object has probably changed */
        display_set->ready_for_display = 0;
    } else
        dvbsub_object_data_segment_destroy(segment); /* No update */
}



void dvbsub_update_page_composition_segment(dvbsub_display_set_t *display_set, dvbsub_page_composition_segment_t *segment) {
    switch (segment->page_state) {
    case DVBSUB_PAGE_STATE_NORMAL_CASE: {
        dvbsub_display_set_t *old_ds;
        dvbsub_region_composition_segment_t *old_rcs;
        dvbsub_CLUT_definition_segment_t *old_cds;
        dvbsub_object_data_segment_t *old_ods;

        if (display_set->page_composition_segment == NULL) {
            display_set->page_composition_segment = segment;
        } else if (segment->page_version_number !=
                   display_set->page_composition_segment->page_version_number) {
            dvbsub_page_composition_segment_destroy(display_set->page_composition_segment);
            display_set->page_composition_segment = segment;
        } else {
            dvbsub_page_composition_segment_destroy(segment);
            return;
        }

        old_ds = display_set->previous_display_set;
        if (old_ds == NULL)
            return;
        
        old_rcs = old_ds->region_composition_segments;
        while (old_rcs != NULL) {
            dvbsub_region_composition_segment_t *copy =
                dvbsub_region_composition_segment_copy(old_rcs);
            dvbsub_update_region_composition_segments(display_set, copy);
            old_rcs = old_rcs->next_segment;
        }
        
        old_cds = old_ds->CLUT_definition_segments;
        while (old_cds != NULL) {
            dvbsub_CLUT_definition_segment_t *copy =
                dvbsub_CLUT_definition_segment_copy(old_cds);
            dvbsub_update_CLUT_definition_segments(display_set, copy);
            old_cds = old_cds->next_segment;
            //gxlogd("display_set=%p, clut_id=%d, version=%d\n", display_set, copy->CLUT_id, copy->CLUT_version_number);
        }
        
        old_ods = old_ds->object_data_segments;
        while (old_ods != NULL) {
            dvbsub_object_data_segment_t *copy =
                dvbsub_object_data_segment_copy(old_ods);
            dvbsub_update_object_data_segments(display_set, copy);
            old_ods = old_ods->next_segment;
        }
        
        break;
    }
    case DVBSUB_PAGE_STATE_ACQUISITION_POINT:
    case DVBSUB_PAGE_STATE_MODE_CHANGE:
        dvbsub_page_composition_segment_destroy(display_set->page_composition_segment);
        destroy_region_composition_segment_list(display_set);
        destroy_CLUT_definition_segment_list(display_set);
        destroy_object_data_segment_list(display_set);
        display_set->page_composition_segment = segment;
        break;
    }
    /* If page has been updated, redraw is required */
    display_set->ready_for_display = 0;
}


int dvbsub_check_if_display_set_ready(dvbsub_decoder_t* 		decoder,
									  dvbsub_page_decoder_t*	page_decoder,
                                      dvbsub_display_set_t*		display_set) 
{
    int 									i;
    int 									j;
    dvbsub_page_composition_segment_t*		pcs;
    dvbsub_page_decoder_t*					pd;
    dvbsub_region_composition_segment_t*	rcs;
    dvbsub_object_data_segment_t*			ods;
    dvbsub_CLUT_definition_segment_t*		cds;

    if (display_set == NULL || display_set->page_composition_segment == NULL)
        return 0;

    if (display_set->ready_for_display == 1) {
        return 1;
    }

    if (display_set->ready_for_display == 2) {
        return 0;
    }

    /* Find ancillary display set */
    if (display_set->ancillary_display_set == NULL 
        && decoder->anci_page_decoders != NULL) {
        pd = decoder->anci_page_decoders;
        if (pd != page_decoder) {
            dvbsub_display_set_t *ds = pd->display_sets;
            while (ds != NULL) {
                if (ds->PTS == display_set->PTS) {
                    display_set->ancillary_display_set = ds;
                    break;
                }
                ds = ds->previous_display_set;
            }
        }
    }

    pcs = display_set->page_composition_segment;

    for (i = 0; i < pcs->number_of_region_compositions; ++i) {
        rcs = dvbsub_find_region_composition_segment(display_set,
                                                   pcs->regions[i].region_id);
        if (rcs == NULL) {
            //if (pcs->page_state != 0) {
                return 0;
            //} else {
            //    gxlogd("display set=%p, region_num=%d, Leak a region id=%d\n", 
            //        display_set, pcs->number_of_region_compositions, pcs->regions[i].region_id);
            //}
        }
        for (j = 0; j < rcs->number_of_regions; ++j) {
            ods = dvbsub_find_object_data_segment(display_set,
                                                rcs->regions[j].object_id);

            if (ods == NULL) {
            //    if (pcs->page_state != 0) {
                    return 0;
            //    } else {
                    //gxlogd("display_set=%p, object_num=%d, Leak a object id=%d\n", display_set, pcs->number_of_region_compositions, rcs->regions[j].object_id);
            //    }
            }

        }
        cds = dvbsub_find_CLUT_definition_segment(display_set, rcs->CLUT_id);
        if (cds == NULL) {
            //if (pcs->page_state != 0) {
                return 0;
            //} else {
            //    gxlogd("display_set=%p, Leak a Clut id=%d\n", display_set, rcs->CLUT_id);
            //}
        }

    }
    display_set->ready_for_display = 1;
    //{
    //    static int ready_display_set_number = 0;
    //    gxlogd("ready_display_set_number=%d\n", ++ready_display_set_number);
    //}
    
    return 1;
}


dvbsub_display_set_t *dvbsub_display_set_create(uint16_t page_id, uint64_t pts) {
    dvbsub_display_set_t *display_set = (dvbsub_display_set_t*)av_malloc(sizeof(dvbsub_display_set_t));
    if (display_set == NULL)
        return NULL;

    memset(display_set, 0, sizeof(dvbsub_display_set_t));
    display_set->page_id = page_id;
    display_set->PTS = pts; 

    //{
    //    static int create_display_set_number = 0;
    //    gxlogd("create_display_set_number=%d\n", ++create_display_set_number);
    //}

    return display_set;
}


void dvbsub_display_set_insert(dvbsub_page_decoder_t *decoder,
                               dvbsub_display_set_t *display_set) {
    dvbsub_display_set_t *dset;

    if (decoder == NULL) {
        return;
    }

    if (display_set == NULL) {
        return;
    }

    if (decoder->display_sets == NULL) {
        decoder->display_sets = display_set;
        return;
    } else if (decoder->display_sets->PTS < display_set->PTS) {
        decoder->display_sets->next_display_set = display_set;
        display_set->previous_display_set = decoder->display_sets;
        decoder->display_sets = display_set;
        return;
    }

    dset = decoder->display_sets;
    while (dset->previous_display_set != NULL &&
           dset->previous_display_set->PTS > display_set->PTS)
        dset = dset->previous_display_set;

    if (dset->previous_display_set == NULL) {
        dset->previous_display_set = display_set;
        display_set->next_display_set = dset;
    } else {
        display_set->previous_display_set = dset->previous_display_set;
        display_set->previous_display_set->next_display_set = display_set;
        dset->previous_display_set = display_set;
        display_set->next_display_set = dset;
    }
}


void dvbsub_display_set_destroy(dvbsub_page_decoder_t *decoder,
                                dvbsub_display_set_t *display_set)
{
    /* Assume that display sets pointing to this ancillary display set
       won't be used anymore and so we don't have to modify
       ancillary_display_set pointers. */

    if (decoder == NULL) {
        return;
    }

    if (display_set == NULL) {
        return;
    }
	
    if (display_set->previous_display_set) {		
		display_set->previous_display_set->next_display_set =
            display_set->next_display_set;
    }

    if (display_set->next_display_set) {		
        display_set->next_display_set->previous_display_set =
            display_set->previous_display_set;
    }
	
    if (display_set == decoder->display_sets) {
        decoder->display_sets = display_set->previous_display_set;
    }

    dvbsub_page_composition_segment_destroy(display_set->page_composition_segment);
    destroy_region_composition_segment_list(display_set);
    destroy_CLUT_definition_segment_list(display_set);
    destroy_object_data_segment_list(display_set);
    av_free(display_set);

    //{
    //    static int destroy_display_set_number = 0;
    //    gxlogd("destroy_display_set_number=%d\n", ++destroy_display_set_number);
    //}
}
