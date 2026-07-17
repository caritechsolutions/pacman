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

#ifndef __DECODER_H__
#define __DECODER_H__

#include "dvbsub_segments.h"
#include "subtitle/sub_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dvbsub_display_set_s {
    uint16_t                                page_id;
    uint64_t                                PTS; /* milliseconds */
    int                                     ready_for_display; /* 1 if page is complete */

    /* Page composition segment is always present when ready_for_display */
    dvbsub_page_composition_segment_t*      page_composition_segment;
    dvbsub_region_composition_segment_t*    region_composition_segments;
    dvbsub_CLUT_definition_segment_t*       CLUT_definition_segments;
    dvbsub_object_data_segment_t*           object_data_segments;
    struct dvbsub_display_set_s*            ancillary_display_set;

    /* Previous display set has smaller PTS than current one */
    struct dvbsub_display_set_s*            previous_display_set;
    /* Previous display set has greater PTS than current one. This pointer
       isn't used anything other than to give user ability to travel forward in
       time. */
    struct dvbsub_display_set_s*            next_display_set;
} dvbsub_display_set_t;


typedef void (*dvbsub_page_decoder_callback_t)(void *priv, const dvbsub_display_set_t *display_set);
struct dvbsub_decoder_s;

typedef struct dvbsub_page_decoder_s {
    uint16_t                        page_id;
    dvbsub_display_set_t*           display_sets;
    void*                           priv;
} dvbsub_page_decoder_t;

typedef struct drawing_context_s {
    GxSubtitle_Render*  render;
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} drawing_context_t;

typedef struct dvbsub_decoder_s {
    dvbsub_page_decoder_t*      comp_page_decoders;
	dvbsub_page_decoder_t*      anci_page_decoders;
    handle_t                    ready_sem;
    struct gxlist_head          ready_list;
    drawing_context_t*          dc;
    void*                       cntx;
    uint32_t                    time_out;
    int32_t                     sync_milliseconds;
    bool                        stop_flag;
} dvbsub_decoder_t;


/*
 * Create new decoder.
 */
dvbsub_decoder_t *dvbsub_decoder_create(void);


/*
 * Release resources obtained by decoder. Destroys all remaining page decoders
 * also.
 */
void dvbsub_decoder_destroy(dvbsub_decoder_t *decoder);


/*
 * Create new page decoder.
 * Parameters:
 *  decoder: previously created decoder
 *  page_id: ID of a page that this page decoder should handle. There shouldn't
 *           be more than one page decoder per page_id per decoder.
 *  is_ancillary_page_decoder: 1 if this page decoder is an ancillary page
 *                             decoder for some other page decoders
 *  ancillary_page_decoder: Ancillary page decoder for this page or NULL if
 *                          none exists. Note that ancillary page decoder can't
 *                          have ancillary page decoder.
 *  callback: Callback function to call when page is ready to be displayed.
 *            May be NULL, if need for display refreshes is noticed by other
 *            means.
 *  priv: Private data for callback.
 */
dvbsub_page_decoder_t*
dvbsub_page_decoder_create(dvbsub_decoder_t*          		decoder,
                           uint16_t                   		page_id,
                           void*                      		priv);


void dvbsub_page_decoder_destroy(dvbsub_page_decoder_t *decoder);


/*
 * Send PES packet for decoder to handle. Call to callback function of one
 * of the page decoders should be expected during execution of this function.
 */
int32_t dvbsub_push_pes_packet(dvbsub_decoder_t *decoder, const uint8_t* packet);


/*
 * Remove display sets that have PTS pts or lower.
 */
void dvbsub_cleanup_before_PTS(dvbsub_decoder_t *decoder, uint64_t pts);


/*
 * Find display set that should be visible during PTS. Returns NULL if no
 * display set is visible.
 */
dvbsub_display_set_t*
dvbsub_find_visible_display_set(dvbsub_page_decoder_t *decoder, uint64_t pts);


/*
 * Find display set that should be displayed at or after PTS. Returns NULL if
 * page decoder has no display sets that have PTS after pts. Returned display
 * set need not be ready for displaying.
 */
dvbsub_display_set_t*
dvbsub_find_next_display_set(dvbsub_page_decoder_t *decoder, uint64_t pts);


#ifdef __cplusplus
}
#endif


#endif /* __DECODER_H__ */
