#ifndef __BACKSTAGE_SEARCH_H__
#define __BACKSTAGE_SEARCH_H__

#define PAT_TIMEOUT (5)
#define PMT_TIMEOUT (7)
#define SDT_TIMEOUT (7)
#define NIT_TIMEOUT (10)

#include "gxsearch.h"
#include "module/pm/gxpm_manage.h"

#define MAX_FILTER (64)

//stream type
#define  MPEG_1_VIDEO       0x01
#define  MPEG_2_VIDEO       0x02
#define  MPEG_4_VIDEO       0x10
#define  MPEG_1_AUDIO       0x03
#define  MPEG_2_AUDIO       0x04
#define  H264               0x1b
#define  H265               0x24
#define  AAC_ADTS           0xf
#define  AAC_LATM           0x11
#define  AVS                0x42
#define  PRIVATE_PES_STREAM 0x06//出现在pmt中一般是EAC3
#define  LPCM               0x80
#define  AC3                0x81
#define  DTS                0x82
#define  DOLBY_TRUEHD       0x83
#define  AC3_PLUS           0x84
#define  DTS_HD             0x85
#define  DTS_MA             0x86
#define  AC3_PLUS_SEC       0xa1
#define  DTS_HD_SEC         0xa2

/*pat表的内容*/
typedef struct {
    uint16_t                prog_number;    ///<相当于service id
    uint16_t                pmt_pid;
    uint16_t                ts_id;
    uint32_t                pat_version;
}SearchPatBody;

/*pmt表的内容*/
typedef struct {
    uint32_t                ttx_desc_valid:1;
    uint32_t                cc_desc_valid:1;
    uint32_t                teletext_type:5;
    uint32_t                teletext_magazine_number:3;
    uint32_t                teletext_page_number:8;
    uint32_t                ttx_pid:13;
    uint32_t                reserved0:1;
}SearchPmtTeletextDesc;

typedef struct {
    uint8_t                 desc_valid:1;
    uint8_t                 reserved:7;
    uint8_t                 subtitling_type;
    uint16_t                composition_page_id;
    uint16_t                ancillary_page_id;
}SearchPmtSubtitlingDesc;

typedef struct {
    uint8_t                 desc_valid:1;
    uint8_t                 reserved:7;
    uint16_t                new_original_network_id;
    uint16_t                new_transport_streamId;
    uint16_t                new_service_id;
}SearchPmtServiceMoveDesc;

typedef struct {
    uint8_t                 desc_valid;
    uint16_t                cas_id;
    uint16_t                ecm_id;
}SearchPmtCaDesc;

typedef struct {
    uint16_t                desc_valid:1;
    uint16_t                ac3_pid:13;
    uint16_t                reserved0:2;
}SearchPmtAc3Desc;
typedef struct {
    uint16_t                 video_pid;
    uint16_t                 ecm_pid_video;
    uint16_t                 audio_count;
    uint16_t                 video_count;
    uint16_t                 pcr_pid;
    uint16_t                 service_id;
    GxBusPmDataProgVideoType service_type;  //avs or mpeg

    SearchPmtTeletextDesc    pmt_ttx_desc;
    SearchPmtSubtitlingDesc  pmt_subt_desc;
    SearchPmtServiceMoveDesc pmt_service_move_desc;
    SearchPmtCaDesc          pmt_ca_desc;
    SearchPmtAc3Desc         pmt_ac3_desc;
}SearchPmtBody;

typedef struct _ServiceInfo
{
    uint16_t prog_id;
    uint16_t servicd_id;
}ServiceInfo;

typedef enum
{
    PAT_TABLE = 0,
    PMT_TABLE,
    SDT_TABLE,
    NIT_TABLE,
    BAT_TABLE,
}TableType;

typedef enum
{
    SECTION_OK = 0,
    SECTION_REPEAT,
    SUBTABLE_OK,
}TableStatus;

typedef enum
{
    SEARCH_START = 0,
    SEARCH_CONTINUE,
    SEARCH_FINISH,
}SearchStatus;

typedef struct _TableInfo
{
    handle_t          handle;
    TableType         type;
    uint8_t           section_state[32];
    uint32_t          section_count;
    bool              section_deduplication;
    private_table_cfg table_cfg;
}TableInfo;

typedef struct _RichEpgSearchCtrl
{
    TableInfo               table[MAX_FILTER];
    handle_t                mutex;
    handle_t                thread;
    SearchStatus            status;

    ServiceInfo            *service_info;
    int32_t                 service_num;

    SearchPatBody          *pat_body;
    uint32_t                pat_body_count;
    int32_t                 pat_timeout;

    SearchPmtBody           pmt_body;
    uint32_t                pmt_audio_stream_body_count;
    GxBusPmDataStream      *pmt_audio_stream_body;
    GxBusPmDataVideoStream *pmt_video_stream_body ;
    uint32_t                pmt_filter_count;
    uint32_t                pmt_finish_count;
    int32_t                 pmt_timeout;

    int32_t                 dmx_id;
    bool                    inited;
}BackStageSearchCtrl;

#define SEARCH_FREE(x)  if(x){GxCore_Free(x);x=NULL;}

#endif
