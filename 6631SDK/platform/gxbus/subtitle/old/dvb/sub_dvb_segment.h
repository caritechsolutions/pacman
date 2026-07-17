/**
 *
 * @file        dvb_segment.h
 * @brief
 * @version:    1.1.0
 * @date        04/30/2010 03:05:24 PM
 * @author      Li Xiaobin (lixb), lixb@nationalchip.com
 *
 */
#ifndef __SUB_DVB_SEGMENT_H__
#define __SUB_DVB_SEGMENT_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "gxcore.h"

typedef enum {
    SUBTITLE_PAGE_COMPOSITION_SEGMENT       = 0x10,
    SUBTITLE_REGION_COMPOSITION_SEGEMNT     = 0x11,
    SUBTITLE_CLUT_DEFINITION_SEGMENT        = 0x12,
    SUBTITLE_OBJECT_DATA_SEGMENT            = 0x13,
    SUBTITLE_DISPLAY_DEFINITION_SEGMENT     = 0x14,
    SUBTITLE_END_OF_DISPLAY_SET_SEGMENT     = 0x80,
    SUBTITLE_STUFFING_SEGMENT               = 0xFF
} GxSubtitle_SegmetType;

typedef enum {
    SUBTITLE_PAGE_NORMAL_CASE               = 0x00,
    SUBTITLE_PAGE_ACQUISITION_POINT         = 0x01,
    SUBTITLE_PAGE_MODE_CHANGE               = 0x02
} dvbsub_page_state;

typedef enum {
    PIXEL_2B_CODE_STRING                    = 0x10,
    PIXEL_4B_CODE_STRING                    = 0x11,
    PIXEL_8B_CODE_STRING                    = 0x12,
    MAP_TABLE_2_TO_4                        = 0x20,
    MAP_TABLE_2_TO_8                        = 0x21,
    MAP_TABLE_4_TO_8                        = 0x22,
    END_LINE_CODE                           = 0xF0
} GxSubtitle_PixelType;

typedef enum {
    SUBTITLE_BASIC_OBJECT_BITMAP            = 0,
    SUBTITLE_BASIC_OBJECT_CHARACTER         = 1,
    SUBTITLE_COMPOSITE_OJBETC_STRING        = 2
} GxSubtitle_ObjectType;

typedef struct {
    uint16_t                                width                   :16;
    uint16_t                                height                  :16;
    uint16_t                                window_h_position_min   :16;
    uint16_t                                window_h_position_max   :16;
    uint16_t                                window_v_position_min   :16;
    uint16_t                                window_v_position_max   :16;
    uint8_t                                 version                 :4;
    uint8_t                                 window_flag             :1;
} GxSubtitle_Dds;


typedef struct {
    uint16_t                                object_id                :16;
    GxSubtitle_ObjectType                   type                     :2;
    uint16_t                                provider_flag            :2;
    uint16_t                                horizontal_position      :12;
    uint16_t                                                         :4;
    uint16_t                                vertical_position        :12;
    uint16_t                                forground_pixel_code     :8;
    uint16_t                                background_pixel_code    :8;

} dvbsub_object_ref;

typedef struct {
    uint16_t                                top_field_length;
    uint16_t                                bottom_field_length;
    uint8_t*                                sub_blocks;
} dvbsub_object_pixels;

typedef struct {
    uint8_t                                 codes_num;
    uint16_t*                               string;
} dvbsub_boject_strings;

typedef struct {
    uint16_t                                id;
    int8_t                                  version;
    uint8_t                                 coding_method;
    uint8_t                                 non_modifying_color_flag;
    union {
        dvbsub_object_pixels                pixels;
        dvbsub_boject_strings               string;
    };
} dvbsub_ods;

typedef struct dvbsub_rcs_s {
    struct dvbsub_rcs_s*                    next;
    uint8_t                                 id                       :8;
    int8_t                                  version                  :8;
    uint8_t                                 fill_flag                :1;
    uint8_t                                 level_compatibility      :3;
    uint16_t                                width                    :16;
    uint16_t                                height                   :16;
    uint8_t                                 clut_id                  :8;
    uint8_t                                 pixel_code_8bit;
    uint8_t                                 pixel_code_4bit;
    uint8_t                                 pixel_code_2bit;
    uint8_t                                 depth                    :3;
    uint32_t                                objects_num              :32;
    dvbsub_object_ref*                      objects_list;
} dvbsub_rcs;

typedef struct dvbsub_clut_entry_s {
    uint8_t                                 id;
    uint8_t                                 entry_2bit_flag;
    uint8_t                                 entry_4bit_flag;
    uint8_t                                 entry_8bit_flag;
    uint8_t                                 y;
    uint8_t                                 cr;
    uint8_t                                 cb;
    uint8_t                                 t;
} dvbsub_clut_entry;

typedef struct dvbsub_cds_s {
    struct dvbsub_cds_s*                    next;
    uint8_t                                 id             :8;
    int8_t                                  version             :8;
    uint16_t                                entries_num           :16;
    dvbsub_clut_entry*                      entries_list;
} dvbsub_cds;

typedef struct {
    uint8_t         id;
    uint16_t        horizontal_add;
    uint16_t        vertical_add;
} dvbsub_region_ref;

typedef struct {
    uint8_t                                 end_time;
    int8_t                                  version;
    dvbsub_page_state                       state;
    int32_t                                 regions_num;
    dvbsub_region_ref*                      regions_list;
} dvbsub_pcs;

#ifdef __cplusplus
}
#endif
#endif
