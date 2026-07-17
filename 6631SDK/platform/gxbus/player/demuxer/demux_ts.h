#ifndef _DEMUX_TS_H_
#define _DEMUX_TS_H_

#include "gx_demux.h"
#include "stheader.h"

#include "mpeg_header.h"
#include "parse_es.h"
#include "demux_hw.h"

#define TYPE_VIDEO                     1
#define TYPE_AUDIO                     2
#define TYPE_SUBT                      3

#define EXTRA_CLEAN_EFFECTS     0x1  /**< stream without voice */
#define EXTRA_HEARING_IMPAIRED  0x2  /**< stream for hearing impaired audiences */
#define EXTRA_VISUAL_IMPAIRED   0x3  /**< stream for visual impaired audiences */

#define TS_WUNIT_SIZE                  64*188

#define TS_PH_PACKET_SIZE              192
#define TS_FEC_PACKET_SIZE             204
#define TS_PACKET_SIZE                 188
#define NB_PID_MAX                     8192

#define MAX_HEADER_SIZE                6
#define MAX_CHECK_SIZE                 65535
#define MAX_PES_PAYLOAD_SIZE           65535
#define TS_MAX_PROBE_SIZE              4000000
#define NUM_CONSECUTIVE_TS_PACKETS     32
#define NUM_CONSECUTIVE_AUDIO_PACKETS  348
#define MAX_A52_FRAME_SIZE             3840

#define MAX_EXTRADATA_SIZE             64*1024
#define MAX_PES_SIZE                   64*1024

#define IS_AUDIO(x) (((x) == AUDIO_MP2) || ((x) == AUDIO_A52) || \
		((x) == AUDIO_LPCM_BE) || ((x) == AUDIO_AAC) || \
		((x) == AUDIO_ADTS) || ((x) == AUDIO_DTS) || \
		((x) == AUDIO_DRA1) || ((x) == AUDIO_A53) || ((x) == AUDIO_OPUS))
#define IS_VIDEO(x) (((x) == VIDEO_MPEG1) || ((x) == VIDEO_MPEG2) ||\
		             ((x) == VIDEO_MPEG4) || ((x) == VIDEO_H264)  ||\
		             ((x) == VIDEO_AVC)   || ((x) == VIDEO_VC1)   ||\
		             ((x) == VIDEO_AVS)   || ((x) == VIDEO_HEVC))
#define IS_SUBT(x)  (((x) == SPU_DVD) || ((x) == SPU_DVB) || \
		((x) == SPU_TXT) || ((x) == SPU_SCTE) || \
		((x) == SPU_ARIB_A) || ((x) == SPU_ARIB_C))

typedef enum {
	UNKNOWN = -1,
	VIDEO_MPEG1 = 0x10000001,
	VIDEO_MPEG2 = 0x10000002,
	VIDEO_MPEG4 = 0X7634706D,
	VIDEO_H264 = 0x10000005,
	VIDEO_AVC = GX_FOURCC('a', 'v', 'c', '1'),
	VIDEO_VC1 = GX_FOURCC('W', 'V', 'C', '1'),
	VIDEO_AVS = 0x42,
	AUDIO_MP2 = 0x50,
	AUDIO_A52 = 0x2000,
	AUDIO_A53 = 0X64623731,
	AUDIO_DTS = 0x2001,
	AUDIO_LPCM_BE = 0x10001,
	AUDIO_AAC = GX_FOURCC('M', 'P', '4', 'A'),
	AUDIO_ADTS = GX_FOURCC('A', 'D', 'T', 'S'),
	SPU_DVD = 0x3000000,
	SPU_DVB = 0x3000001,
	SPU_TXT = 0x3000002,
	SPU_SCTE = 0x3000003,
	SPU_ARIB_A = 0x3000004,
	SPU_ARIB_C = 0x3000005,
	PES_PRIVATE1 = 0xBD00000,
	SL_PES_STREAM = 0xD000000,
	SL_SECTION = 0xD100000,
	MP4_OD = 0xD200000,
	AUDIO_DRA1 = 0X6472615A,
	AUDIO_OPUS = 0X4f707573,
	VIDEO_HEVC = GX_FOURCC('H', 'E', 'V', 'C'),
} ESStreamType;

typedef struct {
	uint8_t *buffer;
	uint16_t buffer_len;
} TSSection;

typedef struct es_stream{
	int size;
	unsigned char *start;
	uint16_t payload_size;
	ESStreamType type, subtype;
	float pts, last_pts;
	int ad_tags;
	int ad_fade_byte;
	int ad_pan_byte;
	int pid;
	char lang[4];
	int last_cc;		// last cc code (-1 if first packet)
	int is_synced;
	TSSection section;
	uint8_t *extradata;
	int extradata_alloc, extradata_len;
	struct {
		uint8_t au_start, au_end, last_au_end;
	} sl;
	struct es_stream *next;
	struct es_stream *tail;
	uint8_t extended_stream_id;
} ESStream;

typedef struct stream_header{
	void *sh;
	int id;
	int type;
} StreamAVHeader;

typedef struct MpegTSContext {
	int packet_size;	// raw packet size, including FEC if present e.g. 188 bytes
	ESStream *pids_hdr;
	StreamAVHeader  streams[NB_PID_MAX];
} MpegTSContext;

typedef struct {
	GxDemuxStream *ds;
	uint8_t buffer[MAX_PES_SIZE];
	int64_t pts;
	int32_t ad_tags;
	int32_t ad_fade_byte;
	int32_t ad_pan_byte;
	uint8_t extended_stream;
	off_t pos;
	int offset;
	int buffer_size;
	int flag;
	int synced;
} AVFIFO;

typedef struct {
	int32_t object_type;	//aka codec used
	int32_t stream_type;	//video, audio etc.
	uint8_t buf[MAX_EXTRADATA_SIZE];
	uint16_t buf_size;
	uint8_t szm1;
} Mp4DecoderConfig;

typedef struct {
	//flags
	uint8_t flags;
	uint8_t au_start;
	uint8_t au_end;
	uint8_t random_accesspoint;
	uint8_t random_accesspoint_only;
	uint8_t padding;
	uint8_t use_ts;
	uint8_t idle;
	uint8_t duration;

	uint32_t ts_resolution, ocr_resolution;
	uint8_t ts_len, ocr_len, au_len, instant_bitrate_len, degr_len, au_seqnum_len, packet_seqnum_len;
	uint32_t timescale;
	uint16_t au_duration, cts_duration;
	uint64_t ocr, dts, cts;
} Mp4SlConfig;

typedef struct {
	uint16_t id;
	uint8_t flags;
	Mp4DecoderConfig decoder;
	Mp4SlConfig sl;
} Mp4ESDescriptor;

typedef struct {
	uint16_t id;
	uint8_t flags;
	Mp4ESDescriptor *es;
	uint16_t es_cnt;
} Mp4Od;

typedef struct pat_progs_t {
	uint16_t id;
	uint16_t pmt_pid;
} TSPATProgs;

typedef struct {
	uint8_t skip;
	uint8_t table_id;
	uint8_t ssi;
	uint16_t section_length;
	uint16_t ts_id;
	uint8_t version_number;
	uint8_t curr_next;
	uint8_t section_number;
	uint8_t last_section_number;
	TSPATProgs *progs;
	uint16_t progs_cnt;
	TSSection section;
} TSPAT;

typedef struct pmt_es_t {
	uint16_t pid;
	uint32_t type;		//it's 8 bit long, but cast to the right type as FOURCC
	uint8_t  stream_type;
	uint16_t descr_length;
	uint8_t format_descriptor[5];
	uint8_t lang[4];
	uint16_t mp4_es_id;
	uint8_t            sub_es_num;
	GxStreamSubStream  sub_es_stream[MAX_SUB_STREAM_NUM];
	uint8_t  extra_type;
} TSPMTES;

typedef struct {
	uint16_t find_prog_id;
	uint16_t progid;
	uint8_t skip;
	uint8_t table_id;
	uint8_t ssi;
	uint16_t section_length;
	uint8_t version_number;
	uint8_t curr_next;
	uint8_t section_number;
	uint8_t last_section_number;
	uint16_t PCR_PID;
	uint16_t prog_descr_length;
	TSSection section;
	uint16_t es_cnt;
	TSPMTES *es;
	Mp4Od iod, *od;
	Mp4ESDescriptor *mp4es;
	int od_cnt, mp4es_cnt;
} TSPMT;

typedef struct {
	uint64_t size;
	uint64_t duration;
	int64_t first_pts;
	int64_t last_pts;
} TSStreamInfo;

typedef struct{
	int pthread_id;
	int running;
	GxFifo* fifo;
	struct{
		int32_t           using;
		int32_t           pid;
		GxDemuxerDataType type;
		int32_t           crc;
		int32_t           finish;
		int32_t           p_copy;
		int32_t           p_len;
		uint8_t*          p_data;
		int32_t           bck_len;
		uint8_t*          bck_data;
		int32_t (*writeback)(int pid, GxDemuxerDataType type, unsigned char *buffer, unsigned int size);
	}data[PLAYER_FILTER_MAX];
}DemuxDataFilter;

typedef struct{
	GxFifo*  fifo;
	int32_t  pid;
	int32_t  right;
	int32_t  total_len;
	int32_t  len;
	uint8_t* data;
}DemuxSubFilter;

typedef struct {
	int64_t last_pts;
	GxParseES es_parser;
	MpegTSContext ts;
	int last_pid;
	AVFIFO fifo[4];		//0 for audio, 1 for video, 2 for subs, 3 for ext audio
	TSPAT pat;
	TSPMT *pmt;
	uint16_t pmt_cnt;
	uint32_t prog;
	int pmt_index;
	uint32_t vbitrate;
	int keep_broken;
	int last_aid;
	int last_vid;
	int last_sid;
	uint8_t packet[TS_FEC_PACKET_SIZE];
	TSStreamInfo vstr, astr;
	int abort;
	float   bps;
	int64_t lastseekpts;
	int64_t lastseekpos;
	int64_t audio_lastseekbytes;
	int64_t video_lastseekbytes;

	int ts_start_pts;
	int ts_start_v_pts;
	int ts_start_a_pts;
	int ts_a_pid;
	int ts_v_pid;
	int ts_dur_pid;
	int ts_dur_probe_size;
	off_t end_pos;
	uint8_t wbuffer[TS_WUNIT_SIZE];
	int time_insert;
	uint32_t timetick;
	DemuxHwPriv hw;
	DemuxDataFilter filter;
	DemuxSubFilter  sub_filter;
	int audio1_running;

	ESStreamType atype, vtype;
	PlayerPVRConfig pvr_config;
} DemuxTSPriv;

typedef struct {
	ESStreamType type;
	TSSection section;
} TSPIDS;

typedef struct {
	int32_t atype, vtype, stype, ad_atype;	//types
	int32_t apid, vpid, spid, ad_apid;	//stream ids
	char slang[4], alang[4];	//languages
	uint16_t prog;
	off_t probe;
} TSDemuxInit;

typedef struct ts_program {
	int progid;		//program id
	int aid, vid, sid;	//audio, video and subtitle id
} TSProgram;

#endif
