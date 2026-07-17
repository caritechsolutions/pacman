#ifndef __DISPLAY_SET_H__
#define __DISPLAY_SET_H__



void dvbsub_update_region_composition_segments(dvbsub_display_set_t *display_set, 
								dvbsub_region_composition_segment_t *segment);


void dvbsub_update_CLUT_definition_segments(dvbsub_display_set_t *display_set, 
									dvbsub_CLUT_definition_segment_t *segment);


void dvbsub_update_object_data_segments(dvbsub_display_set_t *display_set, 
										dvbsub_object_data_segment_t *segment);


void dvbsub_update_page_composition_segment(dvbsub_display_set_t *display_set, 
								dvbsub_page_composition_segment_t *segment);


int dvbsub_check_if_display_set_ready(dvbsub_decoder_t* 	 decoder,
									  dvbsub_page_decoder_t* page_decoder,
                                      dvbsub_display_set_t*	 display_set);

dvbsub_display_set_t*
dvbsub_display_set_create(uint16_t page_id, uint64_t pts);


void dvbsub_display_set_insert(dvbsub_page_decoder_t *decoder,
                               dvbsub_display_set_t *display_set);


void dvbsub_display_set_destroy(dvbsub_page_decoder_t *decoder,
                                dvbsub_display_set_t *display_set);


#endif /* __DISPLAY_SET_H__ */
