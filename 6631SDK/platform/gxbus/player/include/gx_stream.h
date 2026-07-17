#ifndef _GX_ACCESS_H_
#define _GX_ACCESS_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "gx_common.h"
#include "gx_url.h"
#include "gx_mediafilter.h"
#include "gx_network.h"
#include "gx_fileops.h"
#include "gx_system.h"
#include "gx_options.h"
#include "gxplayer_stream.h"

#define GX_STREAMTYPE_UNKNOWN              -1
#define GX_STREAMTYPE_FILE                 0
#define GX_STREAMTYPE_DVB                  1
#define GX_STREAMTYPE_STREAM               2
#define GX_STREAMTYPE_MEMORY               4      // read data from memory area
#define GX_STREAMTYPE_PLAYLIST             6      // FIXME!!! same as GX_STREAMTYPE_FILE now
#define GX_STREAMTYPE_DS                   8      // read from a demuxer stream
#define GX_STREAMTYPE_SMB                  11      // smb:// url, using libsmbclient (samba)
#define GX_STREAMTYPE_SDP                  15
#define GX_STREAMTYPE_FM                   16

#define GX_STREAMTYPE_MEM                  20
#define GX_STREAMTYPE_FIFO                 21
#define GX_STREAMTYPE_RINGMEM              22
#define GX_STREAMTYPE_DEMUXER              23
#define GX_STREAMTYPE_VODSYSTEM            24

#define GX_STREAMTYPE_MTC                  30

#define GX_STREAM_TRUNC                    1
#define GX_STREAM_APPEND                   2
#define GX_STREAM_READ                     3
#define GX_STREAM_WRITE                    4

#define GX_STREAM_SEEK_BW                  2
#define GX_STREAM_SEEK_FW                  4
#define GX_STREAM_SEEK                     (GX_STREAM_SEEK_BW|GX_STREAM_SEEK_FW)
#define GX_STREAM_SEEK_TIME                8
#define GX_STREAM_NO_SEEK                  0x4000

#define GX_STREAM_REDIRECTED               -2

#define MAX_PROTOCOL_SIZE                  10
#define MAX_PROGRAM_COUNT                  16

#define GX_STREAM_CTRL_RESET               0
#define GX_STREAM_CTRL_GET_SIZE            1
#define GX_STREAM_CTRL_CHANGE_URL          2
#define GX_STREAM_CTRL_SWITCH_AUDIO        3
#define GX_STREAM_CTRL_GET_CAP_SIZE        5
#define GX_STREAM_CTRL_ADD_TRACK           6
#define GX_STREAM_CTRL_DEL_TRACK           7
#define GX_STREAM_CTRL_SEEK_TO_TIME        8
#define GX_STREAM_CTRL_SET_READ_TIMEOUT    9
#define GX_STREAM_CTRL_GET_TOTAL_TIME      10
#define GX_STREAM_CTRL_SET_TIMESTAMP       11
#define GX_STREAM_CTRL_FORCE_STOP_STREAM   12
#define GX_STREAM_CTRL_GET_RES_SIZE        13
#define GX_STREAM_CTRL_SEEK_TO_POS         14
#define GX_STREAM_CTRL_GET_TIME_BYSEQ      15
#define GX_STREAM_CTRL_GET_SEQ_BYTIME      16
#define GX_STREAM_CTRL_IS_FINISHED         17
#define	GX_STREAM_CTRL_CACHE_RESET         18
#define GX_STREAM_CTRL_SEEK_TO_SEQ         19
#define GX_STREAM_CTRL_GET_CUR_SEQ         20
#define GX_STREAM_CTRL_IS_NEW_DATA         21
#define GX_STREAM_CTRL_ROLL_BACK           22
#define GX_STREAM_CTRL_GET_VOL_INFO        23// for shift seeking
#define GX_STREAM_CTRL_SEND_ALIVE          24
#define GX_STREAM_CTRL_TIME_OVER           25
#define GX_STREAM_CTRL_VIDEO_READY         26
#define GX_STREAM_CTRL_TS_CACHE_CONFIG     27
#define GX_STREAM_CTRL_GET_BASE_URL        28//for mpegdash
#define GX_STREAM_CTRL_SEAMLESS_BANDWIDTH_SWITCH        29
#define GX_STREAM_CTRL_STREAM_BUFFERING    30
#define GX_STREAM_CTRL_PROBE_HDR           31
#define GX_STREAM_CTRL_READ_HDR            32
#define GX_STREAM_CTRL_WRITE_HDR           33
#define GX_STREAM_CTRL_READ_BLOCK          34
#define GX_STREAM_CTRL_GET_NEWVOLUME       35
#define GX_STREAM_CTRL_PVR_SET_CONTROL     36
#define GX_STREAM_CTRL_PVR_GET_CONTROL     37
#define GX_STREAM_CTRL_GET_ERROR_CODE      38
#define GX_STREAM_CTRL_SYNC_DATA           39
#define GX_STREAM_CTRL_GET_DATA_INFO       40
#define GX_STREAM_CTRL_SET_SECURITY        41
#define GX_STREAM_CTRL_GET_PROCESS_FLAG    42

#define URL_QUIET_SUFFIX "?quiet=1"

#define GX_STREAM_PIN_NAME_OUTPUT "stream_output"

typedef struct {
	int          security_flag;
	int          crypt_enable;
	unsigned int block_size;
} GxStreamSecurity;

typedef struct {
	int    mode;
	int    prog;
	int    flags;
	char  *protocol;
	void  *options;
	void  *priv;
}GxStreamParam;

typedef struct {
	unsigned int bandwidth;
	unsigned int acodec;
	unsigned int vcodec;
	unsigned int width;
	unsigned int height;
	unsigned int frameRate;
	char sar[16];
}GxStreamProgram;

typedef struct {
	int recv_err;
	int need_restart;
	int open_err;
	int need_length;
}GxStreamError;

typedef struct {
	int buf_percent;
	int net_speed;
	int cache_buf_size;
	int cache_buf_percent;
	int cache_back_size;
	int cache_back_percent;
	int cache_data_len;
	int64_t restart_time;
}GxStreamInfo;

typedef struct {
	int64_t seek_pos;
	int32_t seek_ms;
	int32_t seek_seq;
}GxStreamSeek;

typedef struct {
	unsigned short pid;
	int type;
} GxStreamStreamSwitch;

typedef struct gx_stream_list{
	struct gxlist_head  streamlist;
	handle_t            mutex;
	int                 count;
	int                 list_update_id;
	int                 list_update_exit;
	void                *priv;
}GxStreamList;

typedef struct {
	//pcm编码
	unsigned int samplerate;
	unsigned int channels;
	unsigned int endian;
	unsigned int bitwidth;
}GxStreamDataInfo;

typedef struct gx_stream {
	GxMediaFilter    filter;

	char             mime_type[PLAYER_MIME_TYPE_LONG];
	int              fd;
	int              file_format;
	int              flags;
	int              hls_thread;
	GxStreamList*    hls_list;
	int              sector_size;
	unsigned int     buf_pos, buf_len;
	off_t            pos, start_pos, end_pos;
	char             isdvr;
	char             ispvr;
	int              eof;
	int              mode;
	void             *priv;
	char             *url;
	char             *original_url;
	char             *redirect_url;
	int              demuxer_type;
	int              encode;
#ifdef CONFIG_STREAM_CACHE
	int              cache_pid;
	void*            cache_data;
#endif
	int              nobuffer;//0:off 1:on
	int              screen_to_dmx_pts_lms;
	int              screen_to_dmx_pts_hms;
	GxStreamingCtrl* streaming_ctrl;
	int              read_timeoutms; //default 500ms; 0 no timeout ; -1 endless;
	GxFileOps*       fops;
	uint8_t*         buffer;
	int              prog_now;
	int              prog_target;
	int              prog_max;
	GxStreamProgram  prog[MAX_PROGRAM_COUNT];

	PLAYER_INTERRUPT_CBK interrupt_cbk;
	GxStreamInfo      info;
	GxStreamError     err;
	int               cur_seq_no;
	int               no_restart_stream;
	int               isquiet;
	int               audio_disable;
	int               video_disable;
	int               subtitle_disable;
	int               no_check_priv_packet;
	struct gx_stream* next;       // children stream, not readir stream
	GxPin             *outpin;
	GxOptions         *options;
#define DEBUG_STREAM_TIME_MS      (1000)
#define DEBUG_ONCE_STREAM_TIME_MS (100)
	unsigned int      debug_r_cost_time;
	unsigned int      debug_r_data_size;
	unsigned int      debug_w_cost_time;
	unsigned int      debug_w_data_size;
	int               is_hls_flg;
	int               is_hls_av_separate;
}GxStream;

typedef struct gx_stream_class {
#define GX_STREAM_NEED_PROBE  (1<<0)
#define GX_STREAM_NOT_CACHE   (1<<1)
	GxMediaFilterClass  _inherit;
	AUTHER;

	const char*         protocols[MAX_PROTOCOL_SIZE];
	const char*         name;

	int  flags;
	int  (*probe)       (GxStream *stream, uint8_t *buffer, size_t size, void **priv);
	int  (*open)        (GxStream *stream, int mode);
	int  (*read)        (GxStream *stream, uint8_t* buffer, size_t max_len);
	int  (*write)       (GxStream *stream, uint8_t* buffer, size_t len);
	int  (*seek)        (GxStream *stream, off_t pos);
	int  (*control)     (GxStream *stream, int cmd, void* arg);
	void (*close)       (GxStream *stream);
}GxStreamClass;

#ifdef  CONFIG_STREAM_CACHE
int GxStream_CacheFillBuffer(GxStream *s);
int GxStream_CacheSeekLong(GxStream *s, off_t pos, off_t time);
int GxStream_CacheEnable(GxStream *stream, int size, int seek_limit);
int GxStream_CacheControl(GxStream *stream, int cmd, void *arg);
void GxStream_CacheDisable(GxStream *s);
extern int stream_cache_size  ;
extern float stream_cache_min_percent  ;
extern float stream_cache_seek_min_percent ;

#else
#define GxStream_CacheFillBuffer(x) GxStream_FillBuffer(x)
#define GxStream_CacheSeekLong(x,y) GxStream_SeekLong(x,y)
#endif

extern GxStreamClass gx_streambase;

void GxStreamClass_Init(void);
void GxStreamClass_Destroy(void);

GxStream* GxStream_Open(const char* url, int32_t mode, int prog);
GxStream* GxStream_OpenByParam(const char* url, GxStreamParam *param);
GxStream* GxStream_NewMemory(unsigned char* data,int len);
uint64_t GxStream_GetTimeMs(void);
int GxStream_URLISSupported(const char* url);

void GxStream_Close         (GxStream *stream);
size_t  GxStream_Write      (GxStream *stream, uint8_t *buffer, size_t size);
size_t  GxStream_Read       (GxStream *stream, uint8_t *buffer, size_t size);
size_t  GxStream_FillBuffer (GxStream *stream);
int  GxStream_Seek          (GxStream *stream, off_t pos);
int  GxStream_SeekTime      (GxStream *stream, off_t time);
int  GxStream_Control       (GxStream *stream, int cmd, void *args);

void GxStream_Reset   (GxStream *s);
void GxStream_SetTime(GxStream *s, uint64_t time_ms);
int  GxStream_Skip    (GxStream *s, off_t len);
int  GxStream_SeekLong(GxStream *s, off_t pos);


uint8_t      GxStream_ReadChar (GxStream *s);
uint16_t     GxStream_ReadWord (GxStream *s);
unsigned int GxStream_ReadDword(GxStream *s);

uint16_t GxStream_ReadWord_Le  (GxStream *s);
uint32_t GxStream_ReadDword_Le (GxStream *s);
uint64_t GxStream_ReadQword    (GxStream *s);
uint64_t GxStream_ReadQword_Le (GxStream *s);
uint32_t GxStream_Read_Int24   (GxStream *s);
char*        GxStream_ReadLine (GxStream *s, char* mem, int max);
int   GxStream_Eof      (GxStream *s);
off_t GxStream_Tell     (GxStream *s);

#define GetGxStreamClassFromObject(s) ((GxStreamClass*)(GetGxClassFromObject(s)))
#define GXSTREAM(s)                   ((GxStream*)(s))
#define GXSTREAMCLASS(s)              ((GxStreamClass *)(s))

extern void GxStreamRegister(GxStreamClass *cls);
extern int GxStream_RecordEnable(GxStream *stream, int size);
char* stream_url_opts_info_to_lavf(const char *url, char *opts_type, char *opts_value, int opts_size);

#ifdef __cplusplus
}
#endif

#endif

