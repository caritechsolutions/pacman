#ifndef __APP_TS_H__
#define __APP_TS_H__

#include "app_audio.h"
#include "module/app_ttx_subt.h"

typedef struct
{
    int pack_num;
    char **pack_data;
}AppTsData;

typedef enum
{
    TS_PARSE_CONTINUE,
    TS_PARSE_OVER,
}TsParseRet;

typedef struct
{
    void *cb_data;
    TsParseRet (*cb_proc)(char*, void *cb_data);
}SectionParseCb;

typedef struct
{
    unsigned short service_id;
    unsigned short pmt_pid;
}PATProgData;

typedef struct
{
    unsigned short ts_id;
    unsigned short network_id;
    PATProgData *prog_data;
    int prog_num;
}PATInfo;

typedef struct descriptor_info DescriptorInfo;
struct descriptor_info
{
    unsigned char tag;
    unsigned char length;
    char *data;
    DescriptorInfo *next;
};

typedef struct pmt_stream_info PmtStreamInfo;
struct pmt_stream_info
{
    unsigned char type;
    unsigned short pid;
    DescriptorInfo *es_info;
    PmtStreamInfo *next;
};

typedef struct
{
  unsigned short service_id;
  unsigned short pcr_pid;
  DescriptorInfo *prog_info;
  PmtStreamInfo *stream_info;
}PMTInfo;

#define PMTINFO_PRIVATE_TAG 0xe9

status_t app_ts_data_free(AppTsData *ts_data);
status_t app_ts_pack(unsigned short pid, char *payload_data, int data_length, AppTsData *ts_data);
status_t app_tsfile_section_parse(const char *ts_file, unsigned short pid, SectionParseCb *section_cb);

TsParseRet pat_section_proc(char *section_data, void *ret_data);
void pat_data_release(PATInfo *pat_info);

TsParseRet pmt_section_proc(char *section_data, void *ret_data);
void pmt_data_release(PMTInfo *pmt_info);

status_t app_parse_pmt_video(PMTInfo *pmt_info, unsigned short *video_pid, VideoCodecType *video_type);
status_t app_parse_pmt_audio(PMTInfo *pmt_info, AudioLang_t *audio);
status_t app_parse_pmt_subt(PMTInfo *pmt_info, ttx_subt **subt, int *subt_num);
status_t app_parse_pmt_ttx(PMTInfo *pmt_info, ttx_subt **ttx, int *ttx_num);
status_t app_parse_pmt_private(PMTInfo *pmt_info, void *private_data);

//make pat table section, plz release result data if valid
char *app_pat_generate(unsigned short *porg_id_array, unsigned int prog_count);
//make pmt table section, plz release result data if valid
char *app_pmt_generate(unsigned short porg_id, DescriptorInfo *descriptorinfo);


extern unsigned int dvb_crc32(unsigned char* pBuffer, unsigned int nSize);

#endif

