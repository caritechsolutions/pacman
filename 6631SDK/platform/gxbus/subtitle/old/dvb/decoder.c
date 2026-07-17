
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "gx_mem.h"
#include "dvbsub_segments.h"
#include "dvbsub_decoder.h"
#include "sub_debug.h"
#include "display_set.h"

#define GET_STC(decoder)    ((decoder)->demux->ops->get_stc((decoder)->demux->handle))

static int dvbsub_verify_packet(const uint8_t* packet)
{
    int substart;
    int packet_length;

    /* Packet header == 0x000001 */
    if (packet[0] != 0x00 || packet[1] != 0x00 || packet[2] != 0x01) {
        DBG_SUB("Invalid header\n");
        return 0;
    }

    /* stream_id == private_stream_1 */
    if (packet[3] != 0xbd) {
        DBG_SUB("Not a private_stream_1 stream (%x)\n", packet[3]);
        return 0;
    }

    /* data_alignment_indicator == 1 */
    if ((packet[6] & 0x04) == 0) {
        DBG_SUB("Data alignment indicator == 0\n");
        return 0;
    }

    /* PTS is present */
    if ((packet[7] & 0x80) == 0) {
        DBG_SUB("No PTS\n");
        return 0;
    }

    substart        = 9 + packet[8];
    packet_length   = ((uint16_t)packet[4] << 8) + packet[5] + 6;

    /*  data_identifier  == 0x20,*/
    if (packet[substart] != 0x20) {
        DBG_SUB("Data identifier != 0x20\n");
        return 0;
    }
    /*  subtitle_stream_id == 0x00,*/
    if (packet[substart + 1] != 0x00) {
        DBG_SUB( "Subtitle stream ID %x != 0x00\n", packet[substart+1]);
        return 0;
    }
    /*  end_of_PES_data_field_marker    == 0xff*/
    if (packet[packet_length - 1] != 0xff) {
        DBG_SUB("Wrong packet length\n");
        return 0;
    }

    return 1;
}


static void dvbsub_parse_segment(dvbsub_decoder_t*     decoder,
                                 const uint8_t*        segment_data,
                                 uint64_t              pts)
{
    dvbsub_display_set_t*   ds;
    uint16_t                page_id;
    dvbsub_page_decoder_t*  pd = NULL;
    
    page_id = ((uint16_t)segment_data[2] << 8) + segment_data[3];

    if (decoder->comp_page_decoders != NULL 
        && decoder->comp_page_decoders->page_id == page_id) {
        pd = decoder->comp_page_decoders;   
    } else if (decoder->anci_page_decoders != NULL 
               && decoder->anci_page_decoders->page_id == page_id) {
        pd = decoder->anci_page_decoders;   
    }
    
    if (pd == NULL) {
        return;
    }

    ds = pd->display_sets;
    while (ds != NULL && ds->PTS != pts) {
        ds = ds->previous_display_set;
    }
    if (ds == NULL) {
        ds = dvbsub_display_set_create(page_id, pts);
        if (ds == NULL) {
            return;
        }
        dvbsub_display_set_insert(pd, ds);
    }

    switch (segment_data[1]) {
    case 0x10: { /* Page composition segment */
        dvbsub_page_composition_segment_t *segment =
            dvbsub_page_composition_segment_parse(segment_data);
        if (segment == NULL) {
            break;
        }
        dvbsub_update_page_composition_segment(ds, segment);
        DBG_SUB("Got a new page\n");
        DBG_SUB("display_set=%p,pts=%lld\n", ds, ds->PTS);
        DBG_SUB("page_id=%d,page_state=%d, version=%d\n", ds->page_id, ds->page_composition_segment->page_state, ds->page_composition_segment->page_version_number);
        //DBG_SUB("region_number=%d\n", ds->page_composition_segment->number_of_region_compositions);
        break;
    }
    case 0x11: { /* Region composition segment */
        dvbsub_region_composition_segment_t *segment =
            dvbsub_region_composition_segment_parse(segment_data);
        if (segment == NULL) {
            break;
        }
        dvbsub_update_region_composition_segments(ds, segment);
        break;
    }
    case 0x12: { /* CLUT definition segment */
        dvbsub_CLUT_definition_segment_t *segment =
            dvbsub_CLUT_definition_segment_parse(segment_data);
        if (segment == NULL) {
            break;
        }
        dvbsub_update_CLUT_definition_segments(ds, segment);
        DBG_SUB("Got a new CLUT\n");
        DBG_SUB("display_set=%p,pts=%lld\n", ds, ds->PTS);
        DBG_SUB("clut_id=%d, version=%d\n", segment->CLUT_id, segment->CLUT_version_number);
        break;
    }
    case 0x13: {/* Object data segment */
        dvbsub_object_data_segment_t *segment =
            dvbsub_object_data_segment_parse(segment_data);
        if (segment == NULL) {
            break;
        }
        dvbsub_update_object_data_segments(ds, segment);
        break;
    }
    case 0x80: { /* End of display set segment */
        break;
    }
    default:
        break;
    }
    if (dvbsub_check_if_display_set_ready(decoder, pd, ds) == 1) {
        GxCore_SemPost(decoder->ready_sem);
        DBG_SUB("display_set=%p,PTS=%lld is ready\n", ds, ds->PTS);
    }    
}


int32_t dvbsub_push_pes_packet(dvbsub_decoder_t*   decoder,
                               const uint8_t*      packet)
{
    uint64_t            pts;
//    uint64_t            stc;
    int32_t             packet_length;
    int32_t             len;
    int32_t             pos;

    packet_length = ((uint16_t)packet[4] << 8) + packet[5] + 6;

    if (!dvbsub_verify_packet(packet)) {
        return -1;
    }

    pts  = (int64_t)(packet[9] & 0x0E)  << 29;
    pts |= (int64_t) packet[10]         << 22;
    pts |= (int64_t)(packet[11] & 0xFE) << 14;
    pts |= (int64_t) packet[12]         << 7;
    pts |= (int64_t)(packet[13] & 0xFE) >> 1;
    pts /= 90; /* millisecond resolution is enough */  

    //GET_STC(decoder)/45;
    //if (stc > pts || ((pts - stc) > 99999)) {
    //    DBG_SUB("Error! stc=%lld, pts=%lld\n", stc, pts);
    //    return packet_length;
    //}

    pts += decoder->sync_milliseconds;

    pos = 9 + (uint16_t)packet[8] + 2;
    len = packet_length - 7;

    while (pos < len && packet[pos] == 0x0F) {
        dvbsub_parse_segment(decoder, packet + pos, pts);
        pos += 6 + ((uint16_t)packet[pos + 4] << 8) + packet[pos + 5];
    }    
    return packet_length;
}


void dvbsub_cleanup_before_PTS(dvbsub_decoder_t *decoder, uint64_t pts)
{
    int                     i;
    dvbsub_page_decoder_t*  pd = NULL;
    dvbsub_display_set_t*   ds = NULL;

    for (i = 0; i < 2; i++) {
        if (i == 0) {
            pd = decoder->comp_page_decoders;
        } else {
            pd = decoder->anci_page_decoders;
        }
        if (pd != NULL) {
            ds = pd->display_sets;
            while (ds != NULL && ds->PTS > pts) {
                ds = ds->next_display_set;
            }
            if (pd->display_sets != NULL && ds == pd->display_sets) {
                ds = ds->previous_display_set; /* Keep latest display set */
            }

            while (ds != NULL) {
                dvbsub_display_set_t *ds2 = ds;
                ds = ds->previous_display_set;
                dvbsub_display_set_destroy(pd, ds2);
            }
        }
    }
}


dvbsub_display_set_t*
dvbsub_find_visible_display_set(dvbsub_page_decoder_t *decoder,
                                uint64_t pts)
{
    dvbsub_display_set_t*               ds = decoder->display_sets;
    dvbsub_page_composition_segment_t*  pcs = ds->page_composition_segment;
    while (ds != NULL) {
        if (ds->ready_for_display == 1) {
            //DBG_SUB("????????display PTS=%lld, stc=%lld\n", ds->PTS, pts);
            if (ds->PTS <= pts &&
                ds->PTS + pcs->page_time_out*1000 >= pts) {
                ds->ready_for_display = 2;
                return ds;
            } /*else if (ds->PTS < pts &&
                ds->PTS + pcs->page_time_out*1000 < pts){
                dvbsub_display_set_destroy(decoder, ds);
            }*/
        }
        ds = ds->previous_display_set;
    }
    return NULL;
}


dvbsub_display_set_t*
dvbsub_find_next_display_set(dvbsub_page_decoder_t* decoder,
                             uint64_t               pts)
{
    dvbsub_display_set_t *ds = decoder->display_sets;
    while (ds != NULL && ds->PTS > pts) {
        ds = ds->previous_display_set;
    }
    if (ds != NULL) {
        ds = ds->next_display_set;
    }
    return ds;
}



dvbsub_decoder_t *dvbsub_decoder_create(void)
{
    dvbsub_decoder_t *dc;

    dc = (dvbsub_decoder_t *)av_malloc(sizeof(dvbsub_decoder_t));
    if (dc == NULL) {
        return NULL;
    }
    
    dc->comp_page_decoders = NULL;
    dc->anci_page_decoders = NULL;
    
    return dc;
}


void dvbsub_decoder_destroy(dvbsub_decoder_t *decoder)
{
    int                     i;
    dvbsub_page_decoder_t*  pd;
    for (i = 0; i < 2; i++) {
        if (i == 0) {
            pd = decoder->comp_page_decoders;
        } else {
            pd = decoder->anci_page_decoders;
        }
        
        if (pd != NULL) {
            dvbsub_page_decoder_destroy(pd);
        }
    }
    av_free(decoder);
}


dvbsub_page_decoder_t*
dvbsub_page_decoder_create(dvbsub_decoder_t*                decoder,
                           uint16_t                         page_id,
                           void*                            priv)
{
    dvbsub_page_decoder_t *pd;

    pd = (dvbsub_page_decoder_t *)av_malloc(sizeof(dvbsub_page_decoder_t));
    if (pd == NULL) {
        return NULL;
    }

    pd->page_id                 = page_id;
    pd->display_sets            = NULL;
    pd->priv                    = priv;

    return pd;
}


void dvbsub_page_decoder_destroy(dvbsub_page_decoder_t *decoder) 
{
    dvbsub_display_set_t*   ds;

    /* Destroy all display sets. */
    ds = decoder->display_sets;

    while (ds != NULL) {
        dvbsub_display_set_t *ds2 = ds;
        ds = ds->previous_display_set;
        DBG_SUB("%s%d, ds=%p\n", __FUNCTION__, __LINE__, ds);
        dvbsub_display_set_destroy(decoder, ds2);
    }

    av_free(decoder);
}
