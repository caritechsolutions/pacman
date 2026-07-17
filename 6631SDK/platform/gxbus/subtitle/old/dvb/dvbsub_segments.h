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

#ifndef __DVBSUB_SEGMENTS_H__
#define __DVBSUB_SEGMENTS_H__

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef enum dvbsub_page_state_e {
    DVBSUB_PAGE_STATE_NORMAL_CASE = 0x00,
    DVBSUB_PAGE_STATE_ACQUISITION_POINT = 0x01,
    DVBSUB_PAGE_STATE_MODE_CHANGE = 0x02
} dvbsub_page_state_t;


typedef struct dvbsub_page_composition_region_s {
    uint8_t region_id;
    /* Horizontal region composition position in page 0..719 */
    uint16_t region_horizontal_address;
    /* Vertical region composition position in page 0..575 */
    uint16_t region_vertical_address;
} dvbsub_page_composition_region_t;


/* Page composition segment (ETSI EN 300 743 section 7.2.1) */
typedef struct dvbsub_page_composition_segment_s {
    /* Maximum time to show this page in seconds */
    uint8_t page_time_out;
    uint8_t page_version_number;
    dvbsub_page_state_t page_state;

    int number_of_region_compositions;
    dvbsub_page_composition_region_t regions[0];
} dvbsub_page_composition_segment_t;


/* Parse page composition segment */
dvbsub_page_composition_segment_t *dvbsub_page_composition_segment_parse(const uint8_t* segment_data);

/* Destroy page composition segment structure */
void dvbsub_page_composition_segment_destroy(dvbsub_page_composition_segment_t* segment);


/**************************************/


typedef enum dvbsub_region_level_compatibility_e {
    DVBSUB_REGION_LEVEL_COMPABILITY_2BIT_CLUT = 0x01,
    DVBSUB_REGION_LEVEL_COMPABILITY_4BIT_CLUT = 0x02,
    DVBSUB_REGION_LEVEL_COMPABILITY_5BIT_CLUT = 0x03,
} dvbsub_region_level_compatibility_t;


typedef enum dvbsub_region_depth_e {
    DBSUB_REGION_DEPTH_2BIT = 0x1,
    DBSUB_REGION_DEPTH_4BIT = 0x2,
    DBSUB_REGION_DEPTH_8BIT = 0x3
} dvbsub_region_depth_t;


typedef enum dvbsub_object_type_e {
    DVBSUB_OBJECT_TYPE_BASIC_BITMAP = 0x00,
    DVBSUB_OBJECT_TYPE_BASIC_CHARACTER = 0x01,
    DVBSUB_OBJECT_TYPE_COMPOSITE_STRING = 0x02
} dvbsub_object_type_t;


typedef enum dvbsub_object_provider_e {
    DVBSUB_OBJECT_PROVIDER_SUBTITLING_STREAM = 0x00,
    DVBSUB_OBJECT_PROVIDER_ROM = 0x01
} dvbsub_object_provider_t;


typedef struct dvbsub_region_composition_region_s {
    uint16_t        object_id;
    dvbsub_object_type_t object_type;
    dvbsub_object_provider_t object_provider_flag;
    /* Object position inside region */
    uint16_t        object_horizontal_position;
    uint16_t        object_vertical_position;
    uint8_t         foreground_pixel_code;
    uint8_t         background_pixel_code;
} dvbsub_region_composition_region_t;


/* Region composition segment (ETSI EN 300 743 section 7.2.2) */
typedef struct dvbsub_region_composition_segment_s {
    struct dvbsub_region_composition_segment_s *next_segment;

    uint8_t         region_id;
    uint8_t         region_version_number;
    int             region_fill_flag;
    uint16_t        region_width;
    uint16_t        region_height;
    dvbsub_region_level_compatibility_t region_level_of_compatibility;
    dvbsub_region_depth_t region_depth;
    uint8_t         CLUT_id;
    uint8_t         region_8bit_pixel_code;
    uint8_t         region_4bit_pixel_code;
    uint8_t         region_2bit_pixel_code;
    uint16_t        number_of_regions;
    dvbsub_region_composition_region_t regions[0];
} dvbsub_region_composition_segment_t;


dvbsub_region_composition_segment_t *dvbsub_region_composition_segment_parse(const uint8_t* segment_data);
void dvbsub_region_composition_segment_destroy(dvbsub_region_composition_segment_t* segment);


/**************************************/


typedef struct dvbsub_CLUT_entry_s {
    uint8_t         CLUT_entry_id;
    int             CLUT_entry_2bit_flag;
    int             CLUT_entry_4bit_flag;
    int             CLUT_entry_8bit_flag;
    uint8_t         Y_value;
    uint8_t         Cr_value;
    uint8_t         Cb_value;
    uint8_t         T_value;
} dvbsub_CLUT_entry_t;


/* CLUT definition segment (ETSI EN 300 743 section 7.2.3) */
typedef struct dvbsub_CLUT_definition_segment_s {
    struct dvbsub_CLUT_definition_segment_s *next_segment;

    uint8_t         CLUT_id;
    uint8_t         CLUT_version_number;
    uint16_t        number_of_CLUT_entries;
    dvbsub_CLUT_entry_t CLUT_entries[0];
} dvbsub_CLUT_definition_segment_t;


dvbsub_CLUT_definition_segment_t *dvbsub_CLUT_definition_segment_parse(const uint8_t* segment_data);
void dvbsub_CLUT_definition_segment_destroy(dvbsub_CLUT_definition_segment_t* segment);


/**************************************/


typedef enum dvbsub_object_coding_method_e {
    DVBSUB_OBJECT_CODING_PIXELS = 0x00,
    DVBSUB_OBJECT_CODING_STRING = 0x01
} dvbsub_object_coding_t;


typedef struct dvbsub_object_data_pixels_s {
    uint16_t        top_field_data_block_length;
    uint16_t        bottom_field_data_block_length;
    uint8_t         sub_blocks[0];
} dvbsub_object_data_pixels_t;


typedef struct dvbsub_object_data_string_s {
    uint8_t         number_of_codes;
    uint16_t        character_codes[0];
} dvbsub_object_data_string_t;


/* Object data segment (ETSI EN 300 743 section 7.2.4) */
typedef struct dvbsub_object_data_segment_s {
    struct dvbsub_object_data_segment_s *next_segment;

    uint16_t        object_id;
    uint8_t         object_version_number;
    dvbsub_object_coding_t object_coding_method;
    int             non_modifying_colour_flag;
    union {
        dvbsub_object_data_pixels_t pixels;
        dvbsub_object_data_string_t string;
    } data;
} dvbsub_object_data_segment_t;


dvbsub_object_data_segment_t *dvbsub_object_data_segment_parse(const uint8_t* segment_data);
void dvbsub_object_data_segment_destroy(dvbsub_object_data_segment_t* segment);


#ifdef __cplusplus
}
#endif


#endif /* __DVBSUB_SEGMENTS_H__ */
