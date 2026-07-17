#ifndef GX_DEMUX_H
#define GX_DEMUX_H

#include "gx_stream.h"
#include "gx_subreader.h"
#include "gx_subrender.h"
#include "gx_mempool.h"
#include "gxplayer_pvr.h"

#define MAX_PACKS                          512
#define MAX_PACK_BYTES                     (0x100000 * 2)
#define MEMPOOL_NUM                        4

#define GX_DEMUX_PIN_NAME_IN               "demux_in"
#define GX_DEMUX_PIN_NAME_AO               "demux_ao"
#define GX_DEMUX_PIN_NAME_VO               "demux_vo"
#define GX_DEMUX_PIN_NAME_SO               "demux_so"
#define GX_DEMUX_PIN_NAME_TSO              "demux_tso"
#define GX_DEMUX_PIN_NAME_MUXO             "demux_muxout"
#define GX_DEMUX_PIN_NAME_ADAO             "demux_adao"

#define GX_DEMUXER_FLAG_PLAYER             1
#define GX_DEMUXER_FLAG_RECORDER           2
#define GX_DEMUXER_FLAG_PROBE              4
#define GX_DEMUXER_FLAG_MUXER              8

#define GX_DEMUXER_TYPE_UNKNOWN            0
#define GX_DEMUXER_TYPE_AUDIO              1
#define GX_DEMUXER_TYPE_MPEG_PS            2
#define GX_DEMUXER_TYPE_MPEG_TS            3
#define GX_DEMUXER_TYPE_HW                 4
#define GX_DEMUXER_TYPE_MKV                5
#define GX_DEMUXER_TYPE_MPEG_ES            6
#define GX_DEMUXER_TYPE_MOV                7
#define GX_DEMUXER_TYPE_ASF                8
#define GX_DEMUXER_TYPE_AAC                9
#define GX_DEMUXER_TYPE_OGG                10
#define GX_DEMUXER_TYPE_NSV                11
#define GX_DEMUXER_TYPE_LAVF               12
#define GX_DEMUXER_TYPE_MPEG4_ES           13
#define GX_DEMUXER_TYPE_H264_ES            14
#define GX_DEMUXER_TYPE_MPEG_PES           15
#define GX_DEMUXER_TYPE_REAL               17
#define GX_DEMUXER_TYPE_RMVB               18
#define GX_DEMUXER_TYPE_AVI                19
#define GX_DEMUXER_TYPE_DVB                20
#define GX_DEMUXER_TYPE_RTP                21 //Demux LIVE555 RTP
#define GX_DEMUXER_TYPE_DRA                22 //Demux dra
#define GX_DEMUXER_TYPE_DOLBY              23 //AC3/EC3
#define GX_DEMUXER_TYPE_ES                 24
#define GX_DEMUXER_TYPE_HW_YUV             25
#define GX_DEMUXER_TYPE_HW_TS              26
#define GX_DEMUXER_TYPE_VOD                27
#define GX_DEMUXER_TYPE_MPEG_DASH          28
#define GX_DEMUXER_TYPE_FM                 29
#define GX_DEMUXER_TYPE_ASSOCIATIONLIBRE            30


// A virtual demuxer type for the network code
#define GX_DEMUXER_TYPE_PLAYLIST           (2<<16)

#define MAX_A_STREAMS                      1024
#define MAX_V_STREAMS                      1024
#define MAX_S_STREAMS                      256

#define GX_NOPTS_VALUE                     (-1)

typedef enum {
	GX_DEMUXER_CTRL_OK ,/*0*/
	GX_DEMUXER_CTRL_ERROR ,
	GX_DEMUXER_CTRL_NOTIMPL ,
	GX_DEMUXER_CTRL_GET_CURRENT_TIME ,
	GX_DEMUXER_CTRL_GET_SEEKMIN_TIME ,
	GX_DEMUXER_CTRL_GET_CURRENT_PERCENT ,
	GX_DEMUXER_CTRL_GET_TIME_LENGTH ,
	GX_DEMUXER_CTRL_GET_PERCENT_POS ,
	GX_DEMUXER_CTRL_GET_BASE_URL,
	GX_DEMUXER_CTRL_IDENTIFY_PROGRAM ,
	GX_DEMUXER_CTRL_RESET_AUDIO ,/*10*/
	GX_DEMUXER_CTRL_RESET_VIDEO ,
	GX_DEMUXER_CTRL_SWITCH_AUDIO ,
	GX_DEMUXER_CTRL_SWITCH_VIDEO ,
	GX_DEMUXER_CTRL_SWITCH_SUB ,
	GX_DEMUXER_CTRL_SET_SPEED ,
	GX_DEMUXER_CTRL_SYNC_AUDIO ,
	GX_DEMUXER_CTRL_SYNC_SUB ,
	GX_DEMUXER_CTRL_FOURC_STOP ,
	GX_DEMUXER_CTRL_SET_SUB_DROPMODE ,
	GX_DEMUXER_CTRL_SET_AUDIO_DROPMODE ,/*20*/
	GX_DEMUXER_CTRL_SET_AD_DROPMODE ,
	GX_DEMUXER_CTRL_SET_VIDEO_DROPMODE ,
	GX_DEMUXER_CTRL_ADD_TRACK,
	GX_DEMUXER_CTRL_DEL_TRACK,
	GX_DEMUXER_CTRL_SET_RECORD_CONFIG,
	GX_DEMUXER_CTRL_PRIVATE_RESET,
	GX_DEMUXER_CTRL_SUB_ON,
	GX_DEMUXER_CTRL_SUB_OFF,
	GX_DEMUXER_CTRL_SUB_READ_DATA,
	GX_DEMUXER_CTRL_SUB_PSI_ON,/*30*/
	GX_DEMUXER_CTRL_SUB_PSI_OFF,
	GX_DEMUXER_CTRL_SUB_PSI_READ_DATA,
	GX_DEMUXER_CTRL_SET_BASE_TIME,
	GX_DEMUXER_CTRL_GET_VIDEO_LENGTH,
	GX_DEMUXER_CTRL_GET_AUDIO_LENGTH,
	GX_DEMUXER_CTRL_GET_TSRECORDINFO,
	GX_DEMUXER_CTRL_SET_RECORD_USER_INFO,
	GX_DEMUXER_CTRL_GET_RECORD_HEADER_INFO,
	GX_DEMUXER_CTRL_GET_RECORD_USER_INFO,
	GX_DEMUXER_CTRL_SET_PLAY_TS_CONFIG,/*40*/
	GX_DEMUXER_CTRL_CHECK_RECORD_OVERFLOW,
	GX_DEMUXER_CTRL_OPEN_AD_AUDIO,
	GX_DEMUXER_CTRL_RUN_AD_AUDIO,
	GX_DEMUXER_CTRL_STOP_AD_AUDIO,
	GX_DEMUXER_CTRL_CLOSE_AD_AUDIO,
	GX_DEMUXER_CTRL_RESET_AD_AUDIO,
	GX_DEMUXER_CTRL_SAVE_AUDIO,
	GX_DEMUXER_CTRL_SAVE_VIDEO,
	GX_DEMUXER_CTRL_RESTORE_AUDIO,
	GX_DEMUXER_CTRL_RESTORE_VIDEO,/*50*/
	GX_DEMUXER_CTRL_START_MUX,
	GX_DEMUXER_CTRL_STOP_MUX,
	GX_DEMUXER_CTRL_START_PLAY,
	GX_DEMUXER_CTRL_STOP_PLAY,
	GX_DEMUXER_CTRL_GET_CURRENT_AUDIO_TRACK_INFO,
	GX_DEMUXER_CTRL_SEAMLESS_BANDWIDTH_SWITCH,
	GX_DEMUXER_CTRL_GET_PROG_STREAM_INFO,
	GX_DEMUXER_CTRL_LAVF_CONTINUE,
	GX_DEMUXER_CTRL_PAUSE_FILL_DATA,
	GX_DEMUXER_CTRL_RESUME_FILL_DATA,
	GX_DEMUXER_CTRL_UPDATE_TIMEMS,
	GX_DEMUXER_CTRL_SET_RECORD_ENCRYPT,
	GX_DEMUXER_CTRL_GET_RECORD_CTRL_INFO,
}GxDemuxerCtrl;

#define GX_DEMUXER_SEEK_RELATIVE           1
#define GX_DEMUXER_SEEK_ABSOLUTE           2
#define GX_DEMUXER_SEEK_PERCENT            4
#define GX_DEMUXER_SEEK_ANY                8
#define GX_DEMUXER_SEEK_FRAME             16
#define GX_DEMUXER_SEEK_BACKWARD          32

typedef struct gx_demuxer       GxDemuxer;
typedef struct gx_demux_packet  GxDemuxPacket;
typedef struct gx_demux_chapter GxDemuxChapter;
typedef struct gx_demux_stream  GxDemuxStream;
typedef struct gx_demuxer_class GxDemuxerClass;

#define GetGxDemuxerClassFromObject(s)  ((GxDemuxerClass*)(GetGxClassFromObject(s)))
#define GXDEMUXER(s)                    ((GxDemuxer*)(s))
#define GXDEMUXERCLASS(s)               ((GxDemuxerClass *)(s))

typedef enum {
	GX_DEMUXER_FLAG_PKT_HEADER = 0,
	GX_DEMUXER_FLAG_PKT_CONTENT,
}GxDemuxerPktType;
#define GX_DEMUXER_PKT_CONTENT            0x0001
#define GX_DEMUXER_PKT_HEADER             0x0002

struct gx_ctrl_info {
	int              ad_tags;
	int              ad_fade_byte;
	int              ad_pan_byte;
};

struct gx_demux_packet {
	int              len;
	int              pool_size;
#ifdef CONFIG_MEM_POOL
	gx_mempool *     pool;
#endif
	int64_t          pts;
	int64_t          endpts;
	off_t            pos;           // position in index (AVI) or file (MPG)
	uint8_t*         buffer;
	int              buffer_len;
	uint8_t*         buffer2;
	int              buffer2_len;
	int              write_size;
	int              flags;         // keyframe, etc
	int              is_start_switch_dp;

	GxDemuxPacket*   next;
	GxDemuxer*       demuxer;
	GxFifo*          fifo;
	struct gx_ctrl_info ctrlinfo;
};

enum {
	DROPMODE_NONE,
	DROPMODE_FFFB,
	DROPMODE_UNSUPPORT,
	DROPMODE_AUDIOSWITCH,
};

typedef enum {
	DEMUX_STREAM_VIDEO    = (1 << 0),
	DEMUX_STREAM_AUDIO    = (1 << 1),
	DEMUX_STREAM_SUBTITLE = (1 << 2),
} GxDemuxStreamType;

#define NEEDDROP(dropmode) (dropmode == DROPMODE_FFFB || dropmode == DROPMODE_UNSUPPORT)
#define NEEDDROPSWITCH(dropmode) (dropmode == DROPMODE_AUDIOSWITCH)
#define CHANGE_DROPMODE(dropmode, newmode) if(dropmode != DROPMODE_UNSUPPORT) dropmode = newmode;
struct gx_demux_stream {
	int                 buffer_pos;   // current buffer position
	int                 buffer_size;  // current buffer size
	uint8_t*            buffer;       // current buffer, never free() it, always use free_demux_packet(buffer_ref);
	int64_t             pts;          // current buffer's pts
	int                 pts_bytes;    // number of bytes read after last pts stamp
	int                 eof;          // end of demuxed stream? (true if all buffer empty)
	off_t               pos;          // position in the input stream (file)
	off_t               dpos;         // position in the demuxed stream
	int64_t             wbytes;       // serial number of packet
	int                 flags;        // flags of current packet (keyframe etc)
	GxPin*              pin;

	//----------------  -------------------------
	int                 packs;        // number of packets in buffer
	int                 bytes;        // total bytes of packets in buffer
	GxDemuxPacket*      first;        // read to current buffer from here
	GxDemuxPacket*      last;         // append new packets from input stream to here
	GxDemuxPacket*      current;      // needed for refcounting of the buffer

	int                 id;           // stream audio/video/subtitle index
	GxDemuxStreamType   type;         // stream type eg:audio/video/subtitle
	int                 mux_index;    // same means with ffmpeg for muxing
	GxDemuxer*          demuxer;      // parent demuxer structure (stream handler)
	int                 running;      // not eof
	int                 paused;       // not writing
	int64_t             start_pts;
	uint8_t*            resvbuffer;
	int                 resvsize;
	int64_t             resvpts;
	int                 dropmode;

	int32_t             mutex;
	// -------------- stream header --------------
	void* sh;
	void *st;
	// -------------- asf ------------------------
	GxDemuxPacket*      asf_packet;
	int                 asf_seq;
#define FILL_PACKET_LESSER  (1<<0)
#define FILL_PACKET_GREATER (1<<1)
	int                 fill_flag;
	int                 fill_error;
	int                 fill_pkt_sync;     //切音轨，视频同步包标记，音频pts同步
	int64_t             fill_pkt_sync_pos; //切音轨，视频同步包位置
	int32_t             fill_pkt_pts;      //记录填包时间，用于计算dv+esv/da+esa的总时间
	int                 compute_pkt_count; //计算当前流的码流，统计个数
	int32_t             compute_first_pts; //保存第一次时间，达到统计个数重新赋值
	int64_t             compute_first_pos; //保存第一次位置，达到统计个数重新赋值
	int                 compute_stream_bitrate;//当前流码率
	int                 compute_jump_cnt;
	int64_t             compute_es_size;
	int                 compute_es_bitrate;
	int                 fill_full_flags;
	int                 is_fdr_switch_flag;//full detection rate,is switch flag.
	int                 write_first_switch_pack_flag;
};

struct gx_demux_chapter {
	float               start;
	float               end;
	char*               name;
	GxDemuxChapter      *next;
};

typedef struct {
	int pid;
	int type;
	int runflag;
}GxDemuxerStreamSwitch;

typedef struct {
	uint8_t* buffer;
	int      size;
}GxDemuxerPesData, GxDemuxerPsiData;

typedef struct {
#define RECODER_HEADER_MAX_LEN (188 * 12)      ///< 最大支持 12 个 TS 包输入.
	unsigned int len;                          ///< TS 包长度.
	char         data[RECODER_HEADER_MAX_LEN]; ///< TS 包地址.
}GxDemuxerRecordHeaderInfo;

typedef struct {
#define RECODER_BLOCK_SIZE (256*188)
	unsigned int        vaild;
	unsigned int        encrypt_enable;
	unsigned int        tsw_security;
	unsigned int        block_size;
	unsigned int        tsw_ptr_en;
}GxDemuxerRecordCtrlInfo;

typedef struct {
	unsigned int               a_pid;
	unsigned int               v_pid;
	unsigned int               a_pid1;
	unsigned int               a_fmt;
	unsigned int               v_fmt;
	unsigned int               a_fmt1;
	unsigned int               pcr_pid;
	unsigned int               hdr_len;
	unsigned int               timestamp_pid;
	unsigned int               autogen_pmt_pid;
	unsigned int               pmt_pid;
	GxDemuxerRecordExtInfo     ext_info;
	GxDemuxerRecordHeaderInfo  header_info;
	GxDemuxerRecordUserInfo    user_info;
	GxDemuxerRecordCtrlInfo    ctrl_info;
}GxDemuxerPrivateTSInfo;

typedef struct {
	unsigned int ext_pids;
	unsigned int ext_types;
	unsigned int ext_encrypt;
	unsigned int ext_pids_content;
	unsigned int ext_pids_content_type;
	unsigned int ext_pids_major;
	unsigned int ext_pids_minor;
	unsigned int ext_pids_codec;
	unsigned int ext_pids_name;
	unsigned int ext_pids_lang;
	unsigned int ext_resv[4];
} GxDemuxerPrivatePacketExtV01;

typedef struct {
	//desc info
	unsigned int priv_desc;
	unsigned int priv_length;
	unsigned int ts_packets;
	unsigned int ts_length;
	unsigned int es_length;
	unsigned int desc_resv[4];
	//prog info
	unsigned int a_pid;
	unsigned int v_pid;
	unsigned int a_fmt;
	unsigned int v_fmt;
	unsigned int pcr_pid;
	unsigned int hdr_len;
	unsigned int a_pid1;
	unsigned int a_fmt1;
	unsigned int timestamp_pid;
	unsigned int autogen_pmt_pid;
	unsigned int pmt_pid;
	unsigned int prog_resv[4];
	//pvr info
	unsigned int volume_sizemb;
	unsigned int volume_maxnum;
	unsigned int volume_fullstop;
	unsigned int reserve_sizemb;
	unsigned int volume_maxtimes;
	unsigned int pvr_resv[4];
	//ext info
	unsigned int is_rawts;
	unsigned int is_encrypt;
	unsigned int is_reconly;
	unsigned int service_id;
	unsigned int ext_data[4];
	unsigned int ext_num;
	unsigned int ext_resv[4];
	//user info
	unsigned int have_pmt;
	unsigned int user_len;
	unsigned int user_resv[4];
	//resv
	unsigned int version;
} GxDemuxerPrivatePacketV01;

typedef struct {
	//ctrl info
	unsigned int user_encrypt_enable;
	unsigned int user_encrypt_blocksize;
	unsigned int tsw_security;
	unsigned int tsw_ptr_en;
	unsigned int resv[12];
} GxDemuxerPrivatePacketV02;

typedef struct {
	int demuxer_type;
	int clock_en;
	int video_pid; // -1 :auto, -2 not play
	int audio_pid;
	int sub_pid;
	int ad_pid;
	int ad_codec;
	void *debug;
	void *media;
	unsigned int concat_eof;/*1:concat url eof*/
} GxDemuxerOpenInfo;

struct _swdmx {
	int            dev ;
	int            chip_type ;
	handle_t       mod_stc   ;
	unsigned int   cachesize ;
	unsigned int   esvsize ;
	unsigned int   esasize ;
	unsigned int   esssize ;
	unsigned int   poolsize ;
	unsigned int   dumpsize ;
	unsigned int   security_flag;
	unsigned int   security_es;
	unsigned int   security_ts;
};

typedef struct demux_pool {
	int         init;
	gx_mempool* packet_pool;
	int         size[MEMPOOL_NUM];
	int         count[MEMPOOL_NUM];
	gx_mempool* data_pool[MEMPOOL_NUM];
} GxDemuxerPool;

typedef struct {
	int    defalut_index;//1:default 0:no def.
	int    codec_id;
	char   language[PLAYER_TARCK_LANG_LONG];
}GxDemuxerMulitiAudioTrack;

struct gx_demuxer {
	GxMediaFilter       filter;

	uint64_t            filepos;         // input stream current pos.
	uint64_t            movi_start;
	uint64_t            movi_end;
	GxStream            *stream;
	GxStream            *durstream;
	int64_t             stream_pts;      // current stream pts, if applicable (e.g. dvd)
	int64_t             start_pts;
	int64_t             last_pts;
	uint64_t            duration;
	int64_t             cur_timems;
	uint64_t            last_seekms;     // for speed seek
	off_t               last_seekpos;    // for speed seek
	uint64_t            tell_pos;        // just used for get time
	char                *filename;       // Needed by avs_check_file
	int                 synced;          // stream synced (used by mpeg)
	int                 type;            // demuxer type: mpeg PS, mpeg ES, avi, avi-ni, avi-nini, asf
	int                 index_mode;

	int                 seekable;        // flag
	int                 speedable;       // flag
	GxDemuxStream       *audio;          // audio buffer/demuxer
	GxDemuxStream       *video;          // video buffer/demuxer
	GxDemuxStream       *sub;            // dvd subtitle buffer/demuxer
	GxDemuxStream       *audio1;      // audio buffer/demuxer

	int                 num_audio;

	GxPin               *tsout;
	GxPin               *muxout;

	// stream headers:
	void                *a_streams[MAX_A_STREAMS];  // audio streams (sh_audio_t)
	void                *v_streams[MAX_V_STREAMS];  // video sterams (sh_video_t)
	void                *s_streams[MAX_S_STREAMS];  // dvd subtitles (flag)

	GxDemuxChapter      *chapters;
	int                 num_chapters;

	int                 pcr;
	int                 flag; // player /recoder/
	int                 audio_sync; // sync ms
	int                 sub_sync; // sync ms
	int                 stcfreq;
	int                 stcsync;
	int                 stc_factor;
	int                 speed;
	int64_t             stc_start_timems;
	int                 pts_reorder;

	void                *priv;
	int32_t             pthread_depack;
	int32_t             pause_depack; //make pushdata exit faster
	int32_t             pause_fill_fifo;
	int32_t             pthread_check;
	handle_t            semaphore_exit;

	int32_t             mutex;
	int                 base_time;
	int                 fbeof;

	struct _swdmx       *swdmx;

	GxDemuxerPool       pool;
	char*               poolbuf;

	char*               audio_lang;
	char*               video_lang;
	char*               sub_lang;
	GxDemuxerOpenInfo   info;
	int                 muxing;
	int                 playing;
	int                 index_limit;
	int                 audio_total;
	int                 fill_pkt_speed;
	int                 fill_pkt_time;
	int64_t             fill_pkt_size;
	int64_t             last_fill_pkt_size;
	#define DEBUG_VIDEO_PACKET_TIME_MS (2000)
	#define DEBUG_AUDIO_PACKET_TIME_MS (200)
	#define DEBUG_MAX_PACKET_TIME_MS   (100)
	unsigned int        debug_fill_start_time;
	unsigned int        debug_audio_cost_time;
	unsigned int        debug_audio_data_size;
	unsigned int        debug_video_cost_time;
	unsigned int        debug_video_data_size;
	void               *debug_dumper;
	void               *media_priv;
	int                 hwts_sync_mode;
};

struct gx_demuxer_class {
	GxMediaFilterClass  _inherit;
	AUTHER;

	int                 type;
	const char*         name;

	GxDemuxer *(*open)  (GxDemuxer *demuxer);
	void (*close)       (GxDemuxer *demuxer);
	int  (*fill_buffer) (GxDemuxer *demuxer,GxDemuxStream* ds);
	int  (*seek)        (GxDemuxer *demuxer,int64_t rel_seek_ms,int32_t audio_delay,int flags);
	int  (*control)     (GxDemuxer *demuxer,int query, void* args);
	int  (*check_file)  (GxDemuxer *demuxer);
	int  (*sub_run)     (GxDemuxer *demuxer);
	int  (*sub_stop)    (GxDemuxer *demuxer);
};

#define PRINT_DEMUX 0

extern GxDemuxerClass gx_DemuxerBase;

void GxDemuxerClass_Init   (void);
void GxDemuxerClass_Destroy(void);

void GxDemuxPool_Init(GxDemuxerPool* pool, char* buffer, size_t size);
void GxDemuxPool_Destroy(GxDemuxerPool* pool);
GxDemuxPacket *GxDemuxPool_PacketNew(GxDemuxerPool* pool);
void GxDemuxPool_PacketAllocBuffer(GxDemuxerPool* pool, GxDemuxPacket *dp);
void GxDemuxPool_PacketFree(GxDemuxerPool* pool, GxDemuxPacket *dp);

/* Demuxer */
GxDemuxer* GxDemuxer_Restart  (GxDemuxer *demuxer, void* args);
GxDemuxer *GxDemuxer_Open     (GxStream * stream,int flag, GxDemuxerOpenInfo *info);
void GxDemuxer_Close          (GxDemuxer *demuxer) ;
void GxDemuxer_DebugData      (GxDemuxer *demuxer);

int GxDemuxer_Seek            (GxDemuxer *demuxer, int64_t time, int flag);
int GxDemuxer_Speed           (GxDemuxer* demuxer, int speed, int flush, int have_audio, int need_audio);
int GxDemuxer_M3U8_Seek       (GxDemuxer* demuxer, int64_t* seek_time);
int GxDemuxer_Control         (GxDemuxer *demuxer, int cmd, void*);
int GxDemuxer_ReadPesData     (GxDemuxer *demuxer, uint8_t* buffer, int size);
int GxDemuxer_ReadPsiData     (GxDemuxer *demuxer, uint8_t* buffer, int size);
uint64_t GxDemuxer_GetCurrentTime  (GxDemuxer *demuxer);
uint64_t GxDemuxer_GetSeekMinTime(GxDemuxer *demuxer);
int32_t GxDemuxer_GetCurrentSTC(GxDemuxer *demuxer);
int32_t GxDemuxer_GetCurrentVpts(GxDemuxer *demuxer);
uint64_t GxDemuxer_GetTimeLength(GxDemuxer *demuxer);
uint8_t GxDemuxer_GetCurrentPercent(GxDemuxer *demuxer);
unsigned int  GxDemuxer_GetInBufSize(GxDemuxer *demuxer);
unsigned int  GxDemuxer_GetInBufPercent(GxDemuxer *demuxer);
unsigned char GxDemuxer_AlmostEmpty(GxDemuxer *demuxer);
unsigned char GxDemuxer_AlmostFull(GxDemuxer *demuxer);
void GxDemuxer_STCReset(GxDemuxer *demuxer);
void GxDemuxer_STCPause(GxDemuxer *demuxer);
void GxDemuxer_STCResume(GxDemuxer *demuxer);
int GxDemuxer_GetEsBufPercent(GxDemuxer *demuxer, int *esa_percent, int *esv_percent);
int GxDemuxer_GetEsBufSize   (GxDemuxer *demuxer, int *esa_size, int *esv_size);
int GxDemuxer_GetSTCFreq     (GxDemuxer *demuxer);
int GxDemuxer_GetSTCSyncMode (GxDemuxer *demuxer);

int GxDemuxer_ChapterAdd      (GxDemuxer *demuxer, const char *name, uint64_t start, uint64_t end);
int GxDemuxer_ChapterRemove   (GxDemuxer *demuxer, const char *name) ;
int GxDemuxer_ChapterRemoveAll(GxDemuxer *demuxer) ;
GxDemuxChapter* GxDemuxer_ChapterFind (GxDemuxer *demuxer, const char *name);

/* Demux stream */
GxDemuxStream *GxDemuxStream_Create(GxDemuxer *demuxer, int id);
int GxDemuxStream_CheckFillPacket  (GxDemuxer *demuxer);
void GxDemuxStream_Destroy         (GxDemuxStream *ds);
void GxDemuxStream_AddPacket       (GxDemuxStream *ds, GxDemuxPacket * dp);
void GxDemuxStream_ReadPacket      (GxDemuxStream *ds, GxStream * stream, int len, uint32_t pts, off_t pos, int flags);
int  GxDemuxStream_FillBuffer      (GxDemuxStream *ds);
int  GxDemuxStream_Pattern3        (GxDemuxStream *ds, unsigned char *mem, int maxlen, int *read, uint32_t pattern);
int  GxDemuxStream_CachePacket     (GxDemuxStream *ds);

size_t GxDemuxStream_ProbePacketPts(GxDemuxStream *ds, unsigned char **start, int64_t *pts, int depack);
size_t GxDemuxStream_GetPacket     (GxDemuxStream *ds, unsigned char **start);
size_t GxDemuxStream_GetPacketPts  (GxDemuxStream *ds,
		unsigned char **start, int64_t *pts, int64_t *pos, struct gx_ctrl_info* ctrlinfo);
size_t GxDemuxStream_GetPacketSub  (GxDemuxStream *ds, unsigned char **start, int64_t *pts, int64_t *endpts);

void   GxDemuxStream_FreePacks     (GxDemuxStream *ds);
double GxDemuxStream_GetNextPts    (GxDemuxStream *ds);
int    GxDemuxStream_PushData      (GxDemuxStream *ds);
int    GxDemuxStream_PushSub       (GxDemuxStream *ds);
void   GxDemuxStream_Reset         (GxDemuxStream *ds);
int    GxDemuxStream_GetEsBufSize  (GxDemuxStream *ds);

/* packet */
GxDemuxPacket *GxDemuxPacket_Create(GxDemuxer *demuxer, GxFifo *fifo, int len);
GxDemuxPacket *GxDemuxPacket_Resize(GxDemuxer *demuxer, GxDemuxPacket *pack, int len);
void GxDemuxPacket_Destroy(GxDemuxPacket *dp);
int GxDemuxPacket_Write(GxDemuxPacket *dp, const uint8_t *data, int len);
int GxDemuxPacket_Skip(GxDemuxPacket *dp, int len);

status_t GxDemuxer_SeamlessBandwidthSwitch(GxDemuxer* demuxer, unsigned int bandwidth);

/* GX private ts_info interface */
/*
 * malloc a block of memory then compress the 'priv_info' into it,
 *  return this memory with 'buf' and 'buf_size'
 */
	int GxDemuxer_PrivatePacketCreate(char **ret_buf, unsigned int *ret_buflen, GxDemuxerPrivateTSInfo *priv_info);
	int GxDemuxer_PrivatePacketDestroy(char *buf);

	/*if it's GX private packet
	 *then
	 *  analysis it and signed in the param 'priv_info'
	 *else
	 *  seek to the original pos
	 */
int GxDemuxer_PrivatePacketTryAnalysis(GxStream *stream,
		GxDemuxerPrivateTSInfo *priv_info, PlayerPVRConfig *pvr_config);

int GxDemuxer_PrivatePacketSyncExtInfo(GxStream *stream);

int GxDemuxer_PrivateAnalysis(unsigned char *buffer, unsigned int size,
		GxDemuxerPrivateTSInfo *priv_info, PlayerPVRConfig *pvr_config);

#define GX_DEMUX_PEEKC(ds) (                                                   \
		(ds->buffer_pos<ds->buffer_size) ? ds->buffer[ds->buffer_pos]             \
		:(((GxDemuxStream_FillBuffer(ds))!= GX_PLAYER_OK)? (-1) : ds->buffer[ds->buffer_pos]))

#define GX_DEMUX_GETC(ds) (                                                    \
		(ds->buffer_pos<ds->buffer_size) ? ds->buffer[ds->buffer_pos++]           \
		:(((GxDemuxStream_FillBuffer(ds))!= GX_PLAYER_OK)? (-1) : ds->buffer[ds->buffer_pos++]))

#define HAVE_AUDIO(d)  (d->audio  && d->audio->sh)
#define HAVE_VIDEO(d)  (d->video  && d->video->sh)
#define HAVE_SUB(d)    (d->sub    && d->sub->sh)
#define HAVE_AUDIO1(d) (d->audio1 && d->audio1->sh)

extern void GxDemuxerRegister(GxDemuxerClass *cls);
void GxDemuxer_Reset(GxDemuxer* demuxer);

#endif
