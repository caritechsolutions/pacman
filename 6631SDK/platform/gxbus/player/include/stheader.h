#ifndef STHEADER_H
#define STHEADER_H

#include "gx_demux.h"

#define CONFIG_ASS
#ifdef CONFIG_ASS
#include "../libass/ass.h"
#endif

typedef struct   {
	short    top;
	short    left;
	short    bottom;
	short    right;
}Rect;

#define PLAYER_ACODEC_EAC3       0X2002
#define PLAYER_ACODEC_DTS        0x2001
#define PLAYER_ACODEC_AC31       0x2000
#define PLAYER_ACODEC_AC32       0X332D6361

#define PLAYER_ACODEC_DVDPCM     0X10001
#define PLAYER_ACODEC_LATM       0X4C41544D
#define PLAYER_ACODEC_AAAC       0X4134504D
#define PLAYER_ACODEC_RA8B       0X6B6F6F63
#define PLAYER_ACODEC_RA8        0X636F6F6B
#define PLAYER_ACODEC_RAAC1      0X43414152
#define PLAYER_ACODEC_RAAC       0X72616000
#define PLAYER_ACODEC_MPG1       0X50
#define PLAYER_ACODEC_MPG2       0X55
#define PLAYER_ACODEC_MPG3       0X5500736D
#define PLAYER_ACODEC_MP4A       0X6134706D
#define PLAYER_ACODEC_VORBIS     0X73627276
#define PLAYER_ACODEC_VORBIS1    0X566F
#define PLAYER_ACODEC_OPUS       0X6F707573
#define PLAYER_ACODEC_DRA        0X6472615A
#define PLAYER_ACODEC_DB71       0X64623731
#define PLAYER_ACODEC_DB51       0X64623531
#define PLAYER_ACODEC_DBCV       0X64626376
#define PLAYER_ACODEC_FLAC       0X43614C66
#define PLAYER_ACODEC_APE        0X20657061
#define PLAYER_ACODEC_DUMMY      0X5A5A5A5A

#define PLAYER_VCODEC_RV40       0X30345652
#define PLAYER_VCODEC_RV1        0X5649444F
#define PLAYER_VCODEC_H264       0X10000005
#define PLAYER_VCODEC_H263       0X31564C46
#define PLAYER_VCODEC_XVID       0X58564944
#define PLAYER_VCODEC_DIVX       0X44495658
#define PLAYER_VCODEC_XVIDD      0X64697678
#define PLAYER_VCODEC_DIVXD      0X78766964
#define PLAYER_VCODEC_DIV3       0X33564944
#define PLAYER_VCODEC_DX50       0X30355844
#define PLAYER_VCODEC_AVS        0X42
#define PLAYER_VCODEC_MPG1       0X10000001
#define PLAYER_VCODEC_MPG2       0X10000002
#define PLAYER_VCODEC_MPEG       0X4745504D
#define PLAYER_VCODEC_MPG3       0X3267706D
#define PLAYER_VCODEC_MPG11      0X3167706D
#define PLAYER_VCODEC_MP4V       0X7634706D
#define PLAYER_VCODEC_FMP4       0X34504D46
#define PLAYER_VCODEC_WVC1       0X31435657
#define PLAYER_VCODEC_HEVC       0x43564548
#define PLAYER_VCODEC_WMV3       0x33564d57
#define PLAYER_VCODEC_MJPEG      0x47504a4d
#define PLAYER_VCODEC_DUMMY      0X5A5A5A5A

#define ADTS_HEADER_SIZE         7
#define SUBT_HEADER_SIZE         5
#define OGGS_HEADER_SIZE         27
#define OGGS_HEADER_NUM          3



#ifndef WIN32
typedef struct wave_format_ex{
	short wFormatTag; // value that identifies compression format
	short nChannels;
	long  nSamplesPerSec;
	long  nAvgBytesPerSec;
	short nBlockAlign; // size of a data sample
	short wBitsPerSample;
	short cbSize;    // size of format-specific data
} WAVEFORMATEX;

typedef struct bitmap_info_header{
	int   biSize;
	int   biWidth;
	int   biHeight;
	short biPlanes;
	short biBitCount;
	int   biCompression;
	int   biSizeImage;
	int   biXPelsPerMeter;
	int   biYPelsPerMeter;
	int   biClrUsed;
	int   biClrImportant;
}BITMAPINFOHEADER;
#else
#include <windows.h>
#endif

typedef struct avi_stream_hdr{
	uint32_t fccType;
	uint32_t fccHandler;
	uint32_t dwFlags;	/* Contains AVITF_* flags */
	uint16_t wPriority;
	uint16_t wLanguage;
	uint32_t dwInitialFrames;
	uint32_t dwScale;
	uint32_t dwRate;	/* dwRate / dwScale == samples/second */
	uint32_t dwStart;
	uint32_t dwLength; /* In units above... */
	uint32_t dwSuggestedBufferSize;
	uint32_t dwQuality;
	uint32_t dwSampleSize;
	Rect     rcFrame;
} AVIStreamHeader;

typedef struct ogg_stream_hdr {
	uint8_t  pageLogo[4];
	uint8_t  version;
	uint8_t  headerType;
	uint8_t  granulePostion[8];
	uint8_t  serialNumber[4];
	uint8_t  pageSequence[4];
	uint8_t  crcCheckSum[4];
	uint8_t  numSegments;
	uint8_t  segmentTable[255];
	uint8_t  hdrLen;
} OGGStreamHeader;

typedef struct ape_stream_hdr {
	uint8_t  tags[4];
	uint8_t  header_len[4];
	uint16_t file_version;
	uint16_t compress_level;
	uint16_t format_flags;
	uint16_t bits_per_sample;
	uint16_t channels_num;
	uint16_t reserved;
	uint32_t sample_rate;
} APEStreamHeader;

typedef struct {
	void              *data;
	unsigned int       len;
	unsigned int       flag;
}HeaderUnit;

typedef struct {
#define MAX_SUB_STREAM_NUM 32
	uint16_t           major;
	uint16_t           minor;
	char               lang[8];
	unsigned int       type;
}GxStreamSubStream;

typedef struct {
	HeaderUnit         header;
	HeaderUnit         para;
	unsigned int       pts_sync;
	char               name[32];
	char               codec[32];
	char               lang[8];
	int                id;
	int                sub_num;
	GxStreamSubStream  sub_stream[MAX_SUB_STREAM_NUM];
}GxStreamHeadPriv;

typedef struct {
	GxStreamHeadPriv  priv;

	GxDemuxStream     *ds;
	GxAvFrameStat     stat;
	int               format;
	float             stream_delay;       // number of seconds stream should be delayed (according to dwStart or similar)
	int               uncodecdata_len;

	// output format:
	int               sample_format;
	int               samplerate;
	int               samplesize;
	int               channels;
	int               channel_layout;
	int               o_bps;              // == samplerate*samplesize*channels   (uncompr. bytes/sec)
	int               i_bps;              // == bitrate  (compressed bytes/sec)
	int               bits_per_coded_sample;
	int               big_endian;

	// codec-specific:
	void              *context;           // codec-specific stuff (usually HANDLE or struct pointer)
	unsigned char     *codecdata;         // extra header data passed from demuxer to codec
	int                codecdata_len;
	double             pts;               // last known pts value in output from decoder
	int                pts_bytes;         // bytes output by decoder after last known pts
	int                default_track;
	int                data_type;
	int                audio_id;
	int                dec_dev;
	int                dec_mod;
	int                raw_adts;
	int                stream_type;
	int                stream_pid;
	int                dual_mono;

	AVIStreamHeader    audio;
	OGGStreamHeader    *osh;
	WAVEFORMATEX 	   *wf;

} GxStreamAudioHeader;

typedef struct {
	GxStreamHeadPriv  priv;

	GxDemuxStream      *ds;
	GxAvFrameStat      stat;
	GxAvRational       dar, sar;
	GxVideoDecoderProperty_AspectRatio ratio;
	int                format;
	float              timer;             // absolute time in video stream, since last start/seek
	float              stream_delay;      // number of seconds stream should be delayed (according to dwStart or similar)
	unsigned long      bitrate;

	// frame counters:
	float              num_frames;        // number of frames played
	int                num_frames_decoded;// number of frames decoded

	// timing (mostly for mpeg):
	double             pts;               // predicted/interpolated PTS of the current frame
	double             i_pts;             // PTS for the _next_ I/P frame
	float              next_frame_time;
	double             last_pts;
	double             buffered_pts[20];
	int                num_buffered_pts;

	// output format: (set by demuxer)
	float              fps;               // frames per second (set only if constant fps)
	float              frametime;         // 1/fps
	float              aspect;            // aspect ratio stored in the file (for prescaling)
	float              stream_aspect;     // aspect ratio stored in the media headers (e.g. in DVD IFO files)
	int                i_bps;             // == bitrate  (compressed bytes/sec)
	int                disp_w, disp_h;    // display size (filled by fileformat parser)
	int                bpp;
	struct {
		int inited;
		int w, h;
		int cnt;
	} wh_check;

	int                interlace;

	AVIStreamHeader    video;
	BITMAPINFOHEADER   *bih;
	int                isquiet;
	int                mjpeg_needmodify;
	int                mjpeg_unsupport;

	//below is for mpeg4 incompleted
	int mpeg4_needmodify;
	struct{
		int max_b_frames;
		int quarter_sample;
		int ratio_num, ratio_den;       //for sample_aspect_ratio
		int timebase_num, timebase_den;  //for time_base
		int data_partitioning;
		int low_delay;
		int vo_type;
		int mpeg_quant;
		int width, height;
		int workaround_bugs;
		int progressive_sequence;
		int rtp_mode;
		int level;
		int profile;
	}mpeg4;                               //this is for mpeg4 which is lost the header

	VideoDynamicRangeType  drt;
	int                    dolby_flag;
} GxStreamVideoHeader;

typedef enum {
	SUB_CODEC_DVB_DESCRIPTOR = 'b',
	SUB_CODEC_TXT_DESCRIPTOR = 'd',
	SUB_CODEC_SRT = 't',
	SUB_CODEC_MOV_TEXT = 'm',
	SUB_CODEC_SSA = 'a',
	SUB_CODEC_DVD = 'v',
	SUB_CODEC_VOB = 'x',
	SUB_CODEC_PGS = 'p',
	SUB_CODEC_SCTE = 's',
	SUB_CODEC_ARIB = 'r',
	SUB_CODEC_TTML = 'l',
} GxStreamSubCodecType;

typedef struct {
	GxStreamHeadPriv     priv;
	GxStreamSubCodecType type;            // t = text, v = VobSub, a = SSA/ASS
	unsigned char*       extradata;       // extra header data passed from demuxer
	int                  extradata_len;
#ifdef CONFIG_ASS
	ass_track_t*         ass_track;  // for SSA/ASS streams (type == 'a')
#endif
	int                  default_track;
} GxStreamSubHeader;

GxStreamAudioHeader *GxStreamHeader_AudioNew(GxDemuxer *demuxer, int id, int aid);
GxStreamAudioHeader *GxStreamHeader_Audio1New(GxDemuxer *demuxer, int id, int aid);
GxStreamVideoHeader *GxStreamHeader_VideoNew(GxDemuxer *demuxer, int id, int vid);
GxStreamSubHeader   *GxStreamHeader_SubNew  (GxDemuxer *demuxer, int id, int sid);

void GxStreamHeader_AudioFree(GxStreamAudioHeader *sh);
void GxStreamHeader_VideoFree(GxStreamVideoHeader *sh);
void GxStreamHeader_SubFree(GxStreamSubHeader *sh);

int video_read_properties(GxStreamVideoHeader *sh_video);

#endif

