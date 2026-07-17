/*
 * copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVFORMAT_H
#define AVFORMAT_H

#define LIBAVFORMAT_VERSION_INT ((58<<16)+(77<<8)+100)
#define LIBAVFORMAT_VERSION     58.77.100
#define LIBAVFORMAT_BUILD       LIBAVFORMAT_VERSION_INT

//#define LIBAVFORMAT_IDENT       "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.131 Safari/537.36" //V1 use user_agent.
#define LIBAVFORMAT_IDENT       "Lavf/" AV_STRINGIFY(LIBAVFORMAT_VERSION) //ffmpeg defalut user_agent.

#ifndef UINT_MAX
#define UINT_MAX (0xfffffff)
#endif

#include "avcodec.h"
#include "avio.h"
#include "../avutil/intfloat_readwrite.h"
#include "../avutil/dict.h"
#include "../avutil/opt.h"
#include "gx_system.h"
#include "../avutil/common.h"


/* *
 *  * To specify text track kind (different from subtitles default).
 *   */
#define AV_DISPOSITION_CAPTIONS     0x10000
#define AV_DISPOSITION_DESCRIPTIONS 0x20000
#define AV_DISPOSITION_METADATA     0x40000

#define AV_DISPOSITION_ATTACHED_PIC      0x0400
#define AV_PKT_FLAG_KEY   0x0001

#define AV_PKT_FLAG_CONTENT 0
#define AV_PKT_FLAG_VIDEO_HEADER  1
#define AV_PKT_FLAG_AUDIO_HEADER  1

#define NTP_OFFSET 2208988800ULL
#define NTP_OFFSET_US (NTP_OFFSET * 1000000ULL)

#define CONFIG_SMALL 0
#if CONFIG_SMALL
#   define NULL_IF_CONFIG_SMALL(x) NULL
#else
#   define NULL_IF_CONFIG_SMALL(x) x
#endif

#define SPACE_CHARS " \t\r\n"
#define AVFMTCTX_UNSEEKABLE    0x0002 //signal that the stream is definitely


extern PLAYER_SEEK_CBK lavf_seekcbk;

typedef struct AVMetadataConv AVMetadataConv;
typedef struct AVFormatContext AVFormatContext;

typedef struct AVCodecTag {
	int  id;
	unsigned int tag;
} AVCodecTag;

typedef struct AVSubtitleTag {
	int  id;
	int type;
	char subtitle_codec[32];
} AVSubtitleTag;

typedef struct {
	int discard_idx;
	int retain_idx;
}AvControl;

/**
 * Converts all the metadata sets from ctx according to the source and
 * destination conversion tables.
 * @param d_conv destination tags format conversion table
 * @param s_conv source tags format conversion table
 */
void av_metadata_conv(struct AVFormatContext *ctx,const AVMetadataConv *d_conv,const AVMetadataConv *s_conv);

/**
 * Frees all the memory allocated for an AVMetadata struct.
 */

typedef struct AVPacket {
	int64_t pts;		///< presentation time stamp in time_base units
	int64_t dts;		///< decompression time stamp in time_base units
	uint8_t *data;
	int size;
	int stream_index;
	int flags;
	int duration;		///< presentation duration in time_base units (0 if not available)
	void (*destruct) (struct AVPacket *);
	void *priv;
	int64_t pos;		///< byte position in stream, -1 if unknown
	int64_t convergence_duration;
	struct {
		uint8_t *data;
		int      size;
		enum AVPacketSideDataType type;
	} *side_data;
	int side_data_elems;
	int64_t read_pos;
	int16_t lost_packet;
} AVPacket;
#define PKT_FLAG_KEY   0x0001

void av_destruct_packet_nofree(AVPacket * pkt);

/**
 * Default packet destructor.
 */
void av_destruct_packet(AVPacket * pkt);

/**
 * Initialize optional fields of a packet to default values.
 *
 * @param pkt packet
 */
void av_init_packet(AVPacket * pkt);

/**
 * Allocate the payload of a packet and initialize its fields to default values.
 *
 * @param pkt packet
 * @param size wanted payload size
 * @return 0 if OK. AVERROR_xxx otherwise.
 */
int av_new_packet(AVPacket * pkt, int size);

int av_malloc_packet_patch(AVPacket *pkt);
/**
 * Increase packet size, correctly zeroing padding
 *
 * @param pkt packet
 * @param grow_by number of bytes by which to increase the size of the packet
 */
int av_grow_packet(AVPacket *pkt, int grow_by);


/**
 * Allocate and read the payload of a packet and initialize its fields to default values.
 *
 * @param pkt packet
 * @param size wanted payload size
 * @return >0 (read size) if OK. AVERROR_xxx otherwise.
 */
int av_get_packet(ByteIOContext * s, AVPacket * pkt, int size);

/**
 * @warning This is a hack - the packet memory allocation stuff is broken. The
 * packet is allocated if it was not really allocated
 */
int av_dup_packet(AVPacket * pkt);

int av_append_packet(ByteIOContext *s, AVPacket *pkt, int size);

/**
 * Free a packet
 *
 * @param pkt packet to free
 */
void av_free_packet(AVPacket *pkt);
void av_packet_move_ref(AVPacket *dst, AVPacket *src);

uint8_t *av_packet_get_side_data(AVPacket *pkt, enum AVPacketSideDataType type,
                                 int *size);
uint8_t *av_packet_new_side_data(AVPacket *pkt, enum AVPacketSideDataType type, int size);
/** this structure contains the data a format has to probe a file */
typedef struct AVProbeData {
	const char *filename;
	unsigned char *buf;
	int buf_size;
} AVProbeData;

#define AVPROBE_SCORE_RETRY (AVPROBE_SCORE_MAX/4)
#define AVPROBE_SCORE_STREAM_RETRY (AVPROBE_SCORE_MAX/4-1)
#define AVPROBE_SCORE_EXTENSION  50 ///< score for file extension
#define AVPROBE_SCORE_MAX 100	///< max score, half of that is used for file extension based detection
#define AVPROBE_PADDING_SIZE 32	///< extra allocated bytes at the end of the probe buffer

typedef struct AVFormatParameters {
	AVRational time_base;
	int sample_rate;
	int channels;
	int width;
	int height;
	enum PixelFormat pix_fmt;
	int channel;
	/**< used to select dv channel */
	const char *device;
	/**< video, audio or DV device */
	const char *standard;
	/**< tv standard, NTSC, PAL, SECAM */
	int mpeg2ts_raw:1;
	/**< force raw MPEG2 transport stream output, if possible */
	int mpeg2ts_compute_pcr:1;
	/**< compute exact PCR for each transport
	  stream packet (only meaningful if
	  mpeg2ts_raw is TRUE) */
	int initial_pause:1;   /**< do not begin to play the stream
				 immediately (RTSP only) */
	int prealloced_context:1;
	enum CodecID video_codec_id;
	enum CodecID audio_codec_id;
} AVFormatParameters;

//! demuxer will use url_fopen, no opened file should be provided by the caller
#define AVFMT_NOFILE        0x0001
#define AVFMT_NEEDNUMBER    0x0002 /**< needs '%d' in filename */
#define AVFMT_SHOW_IDS      0x0008 /**< show format stream IDs numbers */
#define AVFMT_TS_DISCONT    0x0200 /**< Format allows timestamp discontinuities. Note, muxers always require valid (monotone) timestamps */
#define AVFMT_RAWPICTURE    0x0020 /**< format wants AVPicture structure for
				     raw picture data */
#define AVFMT_GLOBALHEADER  0x0040 /**< format wants global header */
#define AVFMT_NOTIMESTAMPS  0x0080 /**< format does not need / have any timestamps */
#define AVFMT_GENERIC_INDEX 0x0100 /**< use generic index building code */
#define AVFMT_TS_DISCONT    0x0200 /**< Format allows timestamp discontinuities. Note, muxers always require valid (monotone) timestamps */
#define AVFMT_VARIABLE_FPS  0x0400 /**< Format allows variable fps. */
#define AVFMT_NODIMENSIONS  0x0800 /**< Format does not need width/height */
#define AVFMT_NOSTREAMS     0x1000 /**< Format does not require any streams */
#define AVFMT_NOBINSEARCH   0x2000 /**< Format does not allow to fallback to binary search via read_timestamp */
#define AVFMT_NOGENSEARCH   0x4000 /**< Format does not allow to fallback to generic search */
#define AVFMT_NO_BYTE_SEEK  0x8000 /**< Format does not allow seeking by bytes */
#define AVFMT_ALLOW_FLUSH  0x10000 /**< Format allows flushing. If not set, the muxer will not receive a NULL packet in the write_packet function. */
#define AVFMT_TS_NONSTRICT 0x20000
/**< Format does not require strictly
  increasing timestamps, but they must
  still be monotonic */

#define AVFMT_SEEK_TO_PTS   0x4000000 /**< Seeking is based on PTS */


typedef struct AVOutputFormat {
	const char *name;
	const char *mime_type;
	const char *extensions; /**< comma-separated filename extensions */
	/** size of private data so that it can be allocated in the wrapper */
	int priv_data_size;
	/* output support */
	enum CodecID audio_codec; /**< default audio codec */
	enum CodecID video_codec; /**< default video codec */
	int (*write_header)(struct AVFormatContext *);
	int (*write_packet)(struct AVFormatContext *, AVPacket *pkt);
	int (*write_trailer)(struct AVFormatContext *);
	/** can use flags: AVFMT_NOFILE, AVFMT_NEEDNUMBER, AVFMT_GLOBALHEADER */
	int flags;
	/** Currently only used to set pixel format if not YUV420P. */
	int (*set_parameters)(struct AVFormatContext *, AVFormatParameters *);
	int (*interleave_packet)(struct AVFormatContext *, AVPacket *out,
			AVPacket *in, int flush);

	/**
	 * List of supported codec_id-codec_tag pairs, ordered by "better
	 * choice first". The arrays are all terminated by CODEC_ID_NONE.
	 */
	const struct AVCodecTag * const *codec_tag;

	enum CodecID subtitle_codec; /**< default subtitle codec */

	const AVMetadataConv *metadata_conv;

	const AVClass* priv_class;
	/* private fields */
	struct AVOutputFormat *next;
} AVOutputFormat;

typedef struct AVInputFormat {
	const char *name;
	/** Size of private data so that it can be allocated in the wrapper. */
	int priv_data_size;
	/**
	 * Tell if a given file has a chance of being parsed as this format.
	 * The buffer provided is guaranteed to be AVPROBE_PADDING_SIZE bytes
	 * big so you do not have to check for that unless you need more.
	 */
	int (*read_probe)(AVProbeData *);
	/** Read the format header and initialize the AVFormatContext
	  structure. Return 0 if OK. 'ap' if non-NULL contains
	  additional parameters. Only used in raw format right
	  now. 'av_new_stream' should be called to create new streams.  */
	int (*read_header)(struct AVFormatContext *,
			AVFormatParameters *ap);
	/** Read one packet and put it in 'pkt'. pts and flags are also
	  set. 'av_new_stream' can be called only if the flag
	  AVFMTCTX_NOHEADER is used.
	  @return 0 on success, < 0 on error.
	  When returning an error, pkt must not have been allocated
	  or must be freed before returning */
	int (*read_packet)(struct AVFormatContext *, AVPacket *pkt);
	/** Close the stream. The AVFormatContext and AVStreams are not
	  freed by this function */
	int (*read_close)(struct AVFormatContext *);

	//#if LIBAVFORMAT_VERSION_MAJOR < 53
	/**
	 * Seek to a given timestamp relative to the frames in
	 * stream component stream_index.
	 * @param stream_index Must not be -1.
	 * @param flags Selects which direction should be preferred if no exact
	 *              match is available.
	 * @return >= 0 on success (but not necessarily the new offset)
	 */
	int (*read_seek)(struct AVFormatContext *,
			int stream_index, int64_t timestamp, int flags);
	int (*read_control)(AVFormatContext *s, int cmd, void *arg);
	//#endif
	/**
	 * Gets the next timestamp in stream[stream_index].time_base units.
	 * @return the timestamp or AV_NOPTS_VALUE if an error occurred
	 */
	int64_t (*read_timestamp)(struct AVFormatContext *s, int stream_index, int64_t *pos, int64_t pos_limit);
	/** Can use flags: AVFMT_NOFILE, AVFMT_NEEDNUMBER. */
	int flags;
	/** If extensions are defined, then no probe is done. You should
	  usually not use extension format guessing because it is not
	  reliable enough */
	const char *extensions;
	/** General purpose read-only value that the format can use. */
	int value;

	/** Starts/resumes playing - only meaningful if using a network-based format
	  (RTSP). */
	int (*read_play)(struct AVFormatContext *);

	/** Pauses playing - only meaningful if using a network-based format
	  (RTSP). */
	int (*read_pause)(struct AVFormatContext *);

	const struct AVCodecTag * const *codec_tag;

	/**
	 * Seeks to timestamp ts.
	 * Seeking will be done so that the point from which all active streams
	 * can be presented successfully will be closest to ts and within min/max_ts.
	 * Active streams are all streams that have AVStream.discard < AVDISCARD_ALL.
	 */
	int (*read_seek2)(struct AVFormatContext *s, int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags);

	const AVMetadataConv *metadata_conv;

	/* private fields */
	struct AVInputFormat *next;
	int raw_codec_id;
} AVInputFormat;

//typedef struct AVOutputFormat {
//	const char *name;
//	const char *mime_type;
//	const char *extensions;
//			    /**< comma separated filename extensions */
//    /** size of private data so that it can be allocated in the wrapper */
//	int priv_data_size;
//	/* output support */
//	enum CodecID audio_codec;
//			      /**< default audio codec */
//	enum CodecID video_codec;
//			      /**< default video codec */
//	int (*write_header) (struct AVFormatContext *);
//	int (*write_packet) (struct AVFormatContext *, AVPacket * pkt);
//	int (*write_trailer) (struct AVFormatContext *);
//    /** can use flags: AVFMT_NOFILE, AVFMT_NEEDNUMBER, AVFMT_GLOBALHEADER */
//	int flags;
//    /** currently only used to set pixel format if not YUV420P */
//	int (*set_parameters) (struct AVFormatContext *, AVFormatParameters *);
//	int (*interleave_packet) (struct AVFormatContext *, AVPacket * out, AVPacket * in, int flush);
//
//    /**
//     * list of supported codec_id-codec_tag pairs, ordered by "better choice first"
//     * the arrays are all CODEC_ID_NONE terminated
//     */
//	const struct AVCodecTag **codec_tag;
//    const AVMetadataConv *metadata_conv;
//	enum CodecID subtitle_codec;
//				 /**< default subtitle codec */
//
//	/* private fields */
//	struct AVOutputFormat *next;
//} AVOutputFormat;
//
//typedef struct AVInputFormat {
//	const char *name;
//    /** size of private data so that it can be allocated in the wrapper */
//	int priv_data_size;
//    /**
//     * tell if a given file has a chance of being parsed by this format.
//     * The buffer provided is guranteed to be AVPROBE_PADDING_SIZE bytes big
//     * so you dont have to check for that unless you need more.
//     */
//	int (*read_probe) (AVProbeData *);
//    /** read the format header and initialize the AVFormatContext
//       structure. Return 0 if OK. 'ap' if non NULL contains
//       additional paramters. Only used in raw format right
//       now. 'av_new_stream' should be called to create new streams.  */
//	int (*read_header) (struct AVFormatContext *, AVFormatParameters * ap);
//    /** read one packet and put it in 'pkt'. pts and flags are also
//       set. 'av_new_stream' can be called only if the flag
//       AVFMTCTX_NOHEADER is used. */
//	int (*read_packet) (struct AVFormatContext *, AVPacket * pkt);
//    /** close the stream. The AVFormatContext and AVStreams are not
//       freed by this function */
//	int (*read_close) (struct AVFormatContext *);
//    /**
//     * seek to a given timestamp relative to the frames in
//     * stream component stream_index
//     * @param stream_index must not be -1
//     * @param flags selects which direction should be preferred if no exact
//     *              match is available
//     * @return >= 0 on success (but not necessarily the new offset)
//     */
//	int (*read_seek) (struct AVFormatContext *, int stream_index, int64_t timestamp, int flags);
//    /**
//     * gets the next timestamp in AV_TIME_BASE units.
//     */
//	 int64_t(*read_timestamp) (struct AVFormatContext * s, int stream_index, int64_t * pos, int64_t pos_limit);
//    /** can use flags: AVFMT_NOFILE, AVFMT_NEEDNUMBER */
//	int flags;
//    /** if extensions are defined, then no probe is done. You should
//       usually not use extension format guessing because it is not
//       reliable enough */
//	const char *extensions;
//    /** general purpose read only value that the format can use */
//	int value;
//
//    /** start/resume playing - only meaningful if using a network based format
//       (RTSP) */
//	int (*read_play) (struct AVFormatContext *);
//
//    /** pause playing - only meaningful if using a network based format
//       (RTSP) */
//	int (*read_pause) (struct AVFormatContext *);
//
//	const struct AVCodecTag **codec_tag;
//
//	/* private fields */
//	struct AVInputFormat *next;
//} AVInputFormat;

enum AVStreamParseType {
	AVSTREAM_PARSE_NONE,
	AVSTREAM_PARSE_FULL,   /**< full parsing and repack */
	AVSTREAM_PARSE_HEADERS,/**< only parse headers, don't repack */
	AVSTREAM_PARSE_TIMESTAMPS,
	/**< full parsing and interpolation of timestamps for frames not starting on packet boundary */
	AVSTREAM_PARSE_FULL_ONCE,   /**< once full parsing and repack */
	AVSTREAM_PARSE_FULL_RAW=MKTAG(0,'R','A','W'),
	AVSTREAM_PARSE_HEVC,
};

typedef struct AVIndexEntry {
	int64_t pos;
	int64_t timestamp;
#define AVINDEX_KEYFRAME 0x0001
	int flags:2;
	int size:30;		//Yeah, trying to keep the size of this small to reduce memory requirements (it is 24 vs 32 byte due to possible 8byte align).
	int min_distance;     /**< min distance between this and the previous keyframe, used to avoid unneeded searching */
} AVIndexEntry;

#define AV_DISPOSITION_DEFAULT   0x0001
#define AV_DISPOSITION_DUB       0x0002
#define AV_DISPOSITION_ORIGINAL  0x0004
#define AV_DISPOSITION_COMMENT   0x0008
#define AV_DISPOSITION_LYRICS    0x0010
#define AV_DISPOSITION_KARAOKE   0x0020

/**
 *  * Track should be used during playback by default.
 *   * Useful for subtitle track that should be displayed
 *    * even when user did not explicitly ask for subtitles.
 *     */
#define AV_DISPOSITION_FORCED    0x0040
#define AV_DISPOSITION_HEARING_IMPAIRED  0x0080  /**< stream for hearing impaired audiences */
#define AV_DISPOSITION_VISUAL_IMPAIRED   0x0100  /**< stream for visual impaired audiences */
#define AV_DISPOSITION_CLEAN_EFFECTS     0x0200  /**< stream without voice */
/**
 * The stream is stored in the file as an attached picture/"cover art" (e.g.
 * APIC frame in ID3v2). The single packet associated with it will be returned
 * among the first few packets read from the file unless seeking takes place.
 * It can also be accessed at any time in AVStream.attached_pic.
 */
#define AV_DISPOSITION_ATTACHED_PIC      0x0400

#define AV_DISPOSITION_DEPENDENT    0x80000 ///< dependent audio stream (mix_type=0 in mpegts)


typedef struct AVFrac {
	int64_t val, num, den;
} AVFrac;

typedef struct AVPacketSideData {
    uint8_t *data;
    int      size;
    enum AVPacketSideDataType type;
} AVPacketSideData;

typedef struct AVStream {
	int index;/**< stream index in AVFormatContext */
	int id;	  /**< format specific stream id */
	AVCodecContext *codec;
	/**< codec context */
	/**
	 * real base frame rate of the stream.
	 * this is the lowest framerate with which all timestamps can be
	 * represented accurately (it is the least common multiple of all
	 * framerates in the stream), Note, this value is just a guess!
	 * for example if the timebase is 1/90000 and all frames have either
	 * approximately 3600 or 1800 timer ticks then r_frame_rate will be 50/1
	 */
	AVRational r_frame_rate;
	void *priv_data;
	/**
	 *      * Average framerate
	 *           */
	AVRational avg_frame_rate;

	int64_t first_dts;
	int64_t seek_tolerance;
	int codec_info_nb_frames;
	AVDictionary *metadata;

	/**
	 * sample aspect ratio (0 if unknown)
	 * - encoding: Set by user.
	 * - decoding: Set by libavformat.
	 */
	AVRational sample_aspect_ratio;
	/**
	 * this is the fundamental unit of time (in seconds) in terms
	 * of which frame timestamps are represented. for fixed-fps content,
	 * timebase should be 1/framerate and timestamp increments should be
	 * identically 1.
	 */
	AVRational time_base;
	float  time_base_sec;
	int pts_wrap_bits;
	/**< number of bits in pts (used for wrapping control) */
	/* ffmpeg.c private use */
	int stream_copy;
	/**< if set, just copy stream */
	enum AVDiscard discard;	///< selects which packets can be discarded at will and do not need to be demuxed
	//FIXME move stuff to a flags field?
	/** quality, as it has been removed from AVCodecContext and put in AVVideoFrame
	 * MN: dunno if that is the right place for it */
	float quality;
	/**
	 * decoding: pts of the first frame of the stream, in stream time base.
	 * only set this if you are absolutely 100% sure that the value you set
	 * it to really is the pts of the first frame
	 * This may be undefined (AV_NOPTS_VALUE).
	 * @note the ASF header does NOT contain a correct start_time the ASF
	 * demuxer must NOT set this
	 */
	int64_t start_time;
	/**
	 * decoding: duration of the stream, in stream time base.
	 * If a source file does not specify a duration, but does specify
	 * a bitrate, this value will be estimates from bit rate and file size.
	 */
	int64_t duration;

	char name[32];
	char language[4];
	/** ISO 639 3-letter language code (empty string if undefined) */

	/* av_read_frame() support */
	enum AVStreamParseType need_parsing;
	struct AVCodecParserContext *parser;
	/**
	 * For streams with AV_DISPOSITION_ATTACHED_PIC disposition, this packet
	 * will contain the attached picture.
	 *
	 * decoding: set by libavformat, must not be modified by the caller.
	 * encoding: unused
	 */
	AVPacket attached_pic;

#define MAX_STD_TIMEBASES (60*12+6)
	struct {
		int64_t last_dts;
		int64_t duration_gcd;
		int duration_count;
		double duration_error[2][2][MAX_STD_TIMEBASES];
		int64_t codec_info_duration;
		int found_decoder;

		/**
		 * Those are used for average framerate estimation.
		 */
		int64_t fps_first_dts;
		int     fps_first_dts_idx;
		int64_t fps_last_dts;
		int     fps_last_dts_idx;
		int     nb_decoded_frames;
	} *info;
	int64_t reference_dts;
	int64_t cur_dts;
	int last_IP_duration;
	int64_t last_IP_pts;
	const uint8_t *cur_ptr;
	int cur_len;
	int probe_packets;
	AVPacket cur_pkt;
	/* av_seek_frame() support */
	AVIndexEntry *index_entries;
	/**< only used if the format does not
	  support seeking natively */
	int nb_index_entries;
	int ab_index_entries;
	int index_entries_start;
	int index_entries_end;
	int time_scale_mov;
	unsigned int index_entries_allocated;
	int request_probe;
	int64_t nb_frames;	///< number of frames in this stream if known or 0
	int disposition; /**< AV_DISPOSITION_* bit field */

	/**
	 * An array of side data that applies to the whole stream (i.e. the
	 * container does not allow it to change between packets).
	 *
	 * There may be no overlap between the side data in this array and side data
	 * in the packets. I.e. a given side data is either exported by the muxer
	 * (demuxing) / set by the caller (muxing) in this array, then it never
	 * appears in the packets, or the side data is exported / sent through
	 * the packets (always in the first packet where the value becomes known or
	 * changes), then it does not appear in this array.
	 *
	 * - demuxing: Set by libavformat when the stream is created.
	 * - muxing: May be set by the caller before avformat_write_header().
	 *
	 * Freed by libavformat in avformat_free_context().
	 *
	 * @see av_format_inject_global_side_data()
	 */
	AVPacketSideData *side_data;
	/**
	 * The number of elements in the AVStream.side_data array.
	 */
	int            nb_side_data;

	/**
	 * Flags for the user to detect events happening on the stream. Flags must
	 * be cleared by the user once the event has been handled.
	 * A combination of AVSTREAM_EVENT_FLAG_*.
	 */
	int event_flags;
#define AVSTREAM_EVENT_FLAG_METADATA_UPDATED 0x0001 ///< The call resulted in updated metadata.

#define MAX_REORDER_DELAY 4
	int64_t pts_buffer[MAX_REORDER_DELAY + 1];

	struct AVPacketList* last_in_packet_buffer;
	int64_t interleaver_chunk_size;
	int64_t interleaver_chunk_duration;
	AVProbeData probe_data;
	int skip_to_keyframe;
	struct AVFrac pts;
	unsigned int index_entries_allocated_size;
	/* *
	 *      * Internal data to analyze DTS and detect faulty mpeg streams
	 *           */
	int64_t last_dts_for_order_check;
	uint8_t dts_ordered;
	uint8_t dts_misordered;
    /**
     * If not 0, the number of samples that should be skipped from the start of
     * the stream (the samples are removed from packets with pts==0, which also
     * assumes negative timestamps do not happen).
     * Intended for use with formats such as mp3 with ad-hoc gapless audio
     * support.
     */
    int64_t start_skip_samples;
    /**
     * If not 0, the first audio sample that should be discarded from the stream.
     * This is broken by design (needs global sample count), but can't be
     * avoided for broken by design formats such as mp3 with ad-hoc gapless
     * audio support.
     */
    int64_t first_discard_sample;

    /**
     * The sample after last sample that is intended to be discarded after
     * first_discard_sample. Works on frame boundaries only. Used to prevent
     * early EOF if the gapless info is broken (considered concatenated mp3s).
     */
    int64_t last_discard_sample;
} AVStream;

#define AV_PROGRAM_RUNNING 1

typedef struct AVProgram {
	int id;
	char *provider_name;	///< Network name for DVB streams
	char *name;		///< Service name for DVB streams
	int flags;
	enum AVDiscard discard;	///< selects which program to discard and which to feed to the caller
	unsigned int* stream_index;
	unsigned int nb_stream_indexes;
	AVDictionary *metadata;
} AVProgram;

#define AVFMTCTX_NOHEADER      0x0001 /**< signal that no header is present
					(streams are added dynamically) */
/* format I/O context */


typedef struct AVChapter {
	int id;                 ///< unique ID to identify the chapter
	AVRational time_base;   ///< time base in which the start/end timestamps are specified
	int64_t start, end;     ///< chapter start/end time in time_base units
	char *title;            ///< chapter title
	AVDictionary *metadata;
} AVChapter;

struct AVFormatContext {
	/* can only be iformat or oformat, not both at the same time */
	struct AVInputFormat *iformat;
	struct AVOutputFormat *oformat;
	void *priv_data;
	ByteIOContext *pb;
	unsigned int nb_streams;
	AVStream **streams;/*max support: MAX_STREAMS */
	char *url;
	/**< input or output filename */
	char file_format;
	/* stream info */
	int64_t timestamp;
#if 0
	char title[512];
	char author[512];
	char copyright[512];
	char comment[512];
	char album[512];
	int year;
	/**< ID3 year, 0 if none */
	int track;
	/**< track number, 0 if none */
	char genre[32];
#endif
	/**< ID3 genre */

	int ctx_flags;

	/** decoding: position of the first frame of the component, in
	  AV_TIME_BASE fractional seconds. NEVER set this value directly:
	  it is deduced from the AVStream values.  */
	int64_t start_time;

	int64_t lastseekpos;
	int64_t lastseekpts;

	int64_t target_time;
	int64_t seek_tolerance_ms;
	/** decoding: duration of the stream, in AV_TIME_BASE fractional
	  seconds. NEVER set this value directly: it is deduced from the
	  AVStream values.  */
	int64_t duration;
	/** decoding: total file size. 0 if unknown */
	int64_t file_size;
	/** decoding: total stream bitrate in bit/s, 0 if not
	  available. Never set it directly if the file_size and the
	  duration are known as ffmpeg can compute it automatically. */
	int bit_rate;

	/* av_read_frame() support */
	AVStream *cur_st;
	const uint8_t *cur_ptr;
	int cur_len;
	AVPacket cur_pkt;

	/** offset of the first packet */
	int index_built;

	int mux_rate;
	int packet_size;
	int preload;
	int max_delay;

#define AVFMT_NOOUTPUTLOOP -1
#define AVFMT_INFINITEOUTPUTLOOP 0
	/** number of times to loop output in formats that support it */
	int loop_output;

	int flags;
#define AVFMT_FLAG_GENPTS       0x0001 ///< Generate missing pts even if it requires parsing future frames.
#define AVFMT_FLAG_IGNIDX       0x0002 ///< Ignore index.
#define AVFMT_FLAG_NONBLOCK     0x0004 ///< Do not block when reading packets from input.
#define AVFMT_FLAG_IGNDTS       0x0008 ///< Ignore DTS on frames that contain both DTS & PTS
#define AVFMT_FLAG_NOFILLIN     0x0010 ///< Do not infer any values from other values, just return what is stored in the container
#define AVFMT_FLAG_NOPARSE      0x0020 ///< Do not use AVParsers, you also must set AVFMT_FLAG_NOFILLIN as the fillin code works on frames and no parsing -> no frames. Also seeking to frames can not work if parsing to find frame boundaries has been disabled
#define AVFMT_FLAG_NOBUFFER     0x0040 ///< Do not buffer frames when possible
#define AVFMT_FLAG_CUSTOM_IO    0x0080 ///< The caller has supplied a custom AVIOContext, don't avio_close() it.
#define AVFMT_FLAG_DISCARD_CORRUPT  0x0100 ///< Discard frames marked corrupted
#define AVFMT_FLAG_FLUSH_PACKETS    0x0200 ///< Flush the AVIOContext every packet.
#define AVFMT_FLAG_QUICK_START     0x0400 ///< quick start,use iptv live.
/**
 * When muxing, try to avoid writing any random/volatile data to the output.
 * This includes any random IDs, real-time timestamps/dates, muxer version, etc.
 *
 * This flag is mainly intended for testing.
 */
#define AVFMT_FLAG_BITEXACT         0x0400
#define AVFMT_FLAG_MP4A_LATM    0x8000 ///< Enable RTP MP4A-LATM payload
#define AVFMT_FLAG_SORT_DTS    0x10000 ///< try to interleave outputted packets by dts (using this flag can slow demuxing down)
#define AVFMT_FLAG_PRIV_OPT    0x20000 ///< Enable use of private options by delaying codec open (this could be made default once all code is converted)
#define AVFMT_FLAG_KEEP_SIDE_DATA 0x40000 ///< Don't merge side data but keep it separate.
#define AVFMT_FLAG_PTS_REORDER 0x80000
#define AVFMT_FLAG_FAST_SEEK   0x81000 ///< Enable fast, but inaccurate seeks for some formats

	//int loop_input;//not use.
	/** decoding: size of data to probe; encoding unused */
	unsigned int probesize;
	/**
	 * Forced video codec_id.
	 * Demuxing: Set by user.
	 */
	enum CodecID video_codec_id;
	/**
	 * Forced audio codec_id.
	 * Demuxing: Set by user.
	 */
	enum CodecID audio_codec_id;
	/**
	 * Forced subtitle codec_id.
	 * Demuxing: Set by user.
	 */
	enum CodecID subtitle_codec_id;

	/**
	 * maximum duration in AV_TIME_BASE units over which the input should be analyzed in av_find_stream_info()
	 */
	int64_t max_analyze_duration;

	const uint8_t *key;
	int keylen;

	unsigned int nb_programs;
	AVProgram **programs;
	AVDictionary *metadata;
	unsigned int nb_chapters;
	AVChapter **chapters;

	/**
	 * Avoid negative timestamps during muxing.
	 *  0 -> allow negative timestamps
	 *  1 -> avoid negative timestamps
	 * -1 -> choose automatically (default)
	 * Note, this only works when interleave_packet_per_dts is in use.
	 * - encoding: Set by user via AVOptions (NO direct access)
	 * - decoding: unused
	 */
	//int avoid_negative_ts;//not use.

	/**
	 * Error recognition; higher values will detect more errors but may
	 * misdetect some more or less valid parts as errors.
	 * - encoding: unused
	 * - decoding: Set by user.
	 */
	int error_recognition;

	/*****************************************************************
	 * All fields below this line are not part of the public API. They
	 * may not be used outside of libavformat and can be changed and
	 * removed at will.
	 * New public fields should be added right above.
	 *****************************************************************
	 */

	/**
	 * This buffer is only needed when packets were already buffered but
	 * not decoded, for example to get the codec parameters in MPEG
	 * streams.
	 */
	struct AVPacketList *packet_buffer;
	struct AVPacketList *packet_buffer_end;

	/* av_seek_frame() support */
	int64_t data_offset; /**< offset of the first packet */

	/**
	 * Raw packets from the demuxer, prior to parsing and decoding.
	 * This buffer is used for buffering packets until the codec can
	 * be identified, as parsing cannot be done without knowing the
	 * codec.
	 */
	struct AVPacketList *raw_packet_buffer;
	struct AVPacketList *raw_packet_buffer_end;
	/**
	 * Packets split by the parser get queued here.
	 */
	struct AVPacketList *parse_queue;
	struct AVPacketList *parse_queue_end;
	/**
	 * Remaining size available for raw_packet_buffer, in bytes.
	 */
#define RAW_PACKET_BUFFER_SIZE 2500000
	int raw_packet_buffer_remaining_size;
	int audio_preload;
	int max_chunk_size;
	int max_chunk_duration;
	unsigned int max_index_size;
	int fps_probe_size;
	int seek_by_time;

	/**
	 * IO repositioned flag.
	 * This is set by avformat when the underlaying IO context read pointer
	 * is repositioned, for example when doing byte based seeking.
	 * Demuxers can use the flag to detect such changes.
	 */
	int io_repositioned;

	int index_space;
	int index_limit;
	/**
	* Flags for the user to detect events happening on the file. Flags must
	* be cleared by the user once the event has been handled.
	* A combination of AVFMT_EVENT_FLAG_*.
	*/
    int event_flags;
#define AVFMT_EVENT_FLAG_METADATA_UPDATED 0x0001 ///< The call resulted in updated metadata.
	//int strict_std_compliance;//no use.
	int select_index;
	int nobuffer;
	int set_muliti_bandwidth_flag;//1:on 0:off,default:0
	int is_hls_av_separate;
	int concat_eof;
	/**
	 * avio flags, used to force AVIO_FLAG_DIRECT.
	 * - encoding: unused
	 * - decoding: Set by user
	 */
	int avio_flags;

	/**
	 * ID3v2 tag useful for MP3 demuxing
	 */
	AVDictionary *id3v2_meta;
	AVDictionary *url_options;/*not login ByteIOContext,use rtsp protocol*/
	int get_packet_bypts;
	int is_switch_av_track;
	int ass_is_forward;
} ;

typedef struct AVPacketList {
	AVPacket pkt;
	unsigned int         seq;
	struct AVPacketList *next;
} AVPacketList;

extern AVInputFormat *first_iformat;
extern AVOutputFormat *first_oformat;

/* utils.c */
void av_register_input_format(AVInputFormat * format);
void av_register_output_format(AVOutputFormat * format);

/** codec tag <-> codec id */
enum CodecID ff_codec_get_id(const AVCodecTag *tags, unsigned int tag);
enum CodecID av_codec_get_id(const AVCodecTag* const* tags, unsigned int tag);
enum CodecID ff_codec_get_id(const AVCodecTag*  tags, unsigned int tag);
unsigned int av_codec_get_tag(const AVCodecTag* const* tags, enum CodecID id);

/* media file input */

/**
 * finds AVInputFormat based on input format's short name.
 */
AVInputFormat *av_find_input_format(const char *short_name);

/**
 * Guess file format.
 *
 * @param is_opened whether the file is already opened, determines whether
 *                  demuxers with or without AVFMT_NOFILE are probed
 */
AVInputFormat *av_probe_input_format(AVProbeData * pd, int is_opened);

/**
 * Allocates all the structures needed to read an input stream.
 *        This does not open the needed codecs for decoding the stream[s].
 */
int avformat_open_input(AVFormatContext **ps, const char *filename, AVInputFormat *fmt, AVDictionary** options);

AVFormatContext *avformat_alloc_context(void);

void avformat_free_context(AVFormatContext *s);

int avformat_open_input(AVFormatContext **ps,
		const char *filename,AVInputFormat *fmt, AVDictionary** options);

void avformat_close_input(AVFormatContext **ps);

/** no av_open for output, so applications will need this: */
AVFormatContext *av_alloc_format_context(void);

/**
 * Read packets of a media file to get stream information. This
 * is useful for file formats with no headers such as MPEG. This
 * function also computes the real frame rate in case of mpeg2 repeat
 * frame mode.
 * The logical file position is not changed by this function;
 * examined packets may be buffered for later processing.
 *
 * @param ic media file handle
 * @return >=0 if OK. AVERROR_xxx if error.
 * @todo Let user decide somehow what information is needed so we do not waste time geting stuff the user does not need.
 */
int avformat_find_stream_info(AVFormatContext *ic, AVDictionary **options);

/**
 * Read a transport packet from a media file.
 *
 * This function is obsolete and should never be used.
 * Use av_read_frame() instead.
 *
 * @param s media file handle
 * @param pkt is filled
 * @return 0 if OK. AVERROR_xxx if error.
 */
int av_read_packet(AVFormatContext * s, AVPacket * pkt);

/**
 * Return the next frame of a stream.
 *
 * The returned packet is valid
 * until the next av_read_frame() or until av_close_input_file() and
 * must be freed with av_free_packet. For video, the packet contains
 * exactly one frame. For audio, it contains an integer number of
 * frames if each frame has a known fixed size (e.g. PCM or ADPCM
 * data). If the audio frames have a variable size (e.g. MPEG audio),
 * then it contains one frame.
 *
 * pkt->pts, pkt->dts and pkt->duration are always set to correct
 * values in AVStream.timebase units (and guessed if the format cannot
 * provided them). pkt->pts can be AV_NOPTS_VALUE if the video format
 * has B frames, so it is better to rely on pkt->dts if you do not
 * decompress the payload.
 *
 * @return 0 if OK, < 0 if error or end of file.
 */
int av_read_frame(AVFormatContext * s, AVPacket * pkt);

/**
 * Seek to the key frame at timestamp.
 * 'timestamp' in 'stream_index'.
 * @param stream_index If stream_index is (-1), a default
 * stream is selected, and timestamp is automatically converted
 * from AV_TIME_BASE units to the stream specific time_base.
 * @param timestamp timestamp in AVStream.time_base units
 *        or if there is no stream specified then in AV_TIME_BASE units
 * @param flags flags which select direction and seeking mode
 * @return >= 0 on success
 */
int av_seek_frame(AVFormatContext * s, int stream_index, int64_t timestamp, int flags);

/**
 * start playing a network based stream (e.g. RTSP stream) at the
 * current position
 */
int av_read_play(AVFormatContext * s);

/**
 * Pause a network based stream (e.g. RTSP stream).
 *
 * Use av_read_play() to resume it.
 */
int av_read_pause(AVFormatContext * s);

/**
 * Add a new stream to a media file.
 *
 * Can only be called in the read_header() function. If the flag
 * AVFMTCTX_NOHEADER is in the format context, then new streams
 * can be added in read_packet too.
 *
 * @param s media file handle
 * @param id file format dependent stream id
 */
AVStream *av_new_stream(AVFormatContext * s, int id);

AVStream *avformat_new_stream(AVFormatContext *s, void *c);

AVProgram *av_new_program(AVFormatContext * s, int id);

void av_program_add_stream_index(AVFormatContext *ac, int progid, unsigned int idx);

AVChapter *new_chapter(AVFormatContext *s, int id, AVRational time_base, int64_t start, int64_t end, const char *title);
/**
 * Set the pts for a given stream.
 *
 * @param s stream
 * @param pts_wrap_bits number of bits effectively used by the pts
 *        (used for wrap control, 33 is the value for MPEG)
 * @param pts_num numerator to convert to seconds (MPEG: 1)
 * @param pts_den denominator to convert to seconds (MPEG: 90000)
 */
void av_set_pts_info(AVStream * s, int pts_wrap_bits, int pts_num, int pts_den);

void avpriv_set_pts_info(AVStream *s, int pts_wrap_bits, unsigned int pts_num, unsigned int pts_den);
#define AVSEEK_FLAG_BACKWARD   1 ///< seek backward
#define AVSEEK_FLAG_BYTE       2 ///< seeking based on position in bytes
#define AVSEEK_FLAG_ANY        4 ///< seek to any frame, even non keyframes
#define AVSEEK_FLAG_FRAME      8 ///< seeking based on frame number
#define AVSEEK_FLAG_TOLERANCE 16 ///< seeking based on frame number

int av_find_default_stream_index(AVFormatContext * s);

/**
 * Gets the index for a specific timestamp.
 * @param flags if AVSEEK_FLAG_BACKWARD then the returned index will correspond to
 *                 the timestamp which is <= the requested one, if backward is 0
 *                 then it will be >=
 *              if AVSEEK_FLAG_ANY seek to any frame, only keyframes otherwise
 * @return < 0 if no such timestamp could be found
 */
int av_index_search_timestamp(AVStream * st, int64_t timestamp, int flags);
int av_index_search_pos(AVStream*  st, int64_t pos, int flags);

/**
 * Add a index entry into a sorted list updateing if it is already there.
 *
 * @param timestamp timestamp in the timebase of the given stream
 */
int av_add_index_entry(AVStream * st, int64_t pos, int64_t timestamp, int size, int distance, int flags);

/**
 * Does a binary search using av_index_search_timestamp() and .read_timestamp().
 * This is not supposed to be called directly by a user application, but by demuxers.
 * @param target_ts target timestamp in the time base of the given stream
 * @param stream_index stream number
 */

int av_seek_frame_binary(AVFormatContext * s, int stream_index, int64_t target_ts, int flags);

/**
 * Updates cur_dts of all streams based on given timestamp and AVStream.
 *
 * Stream ref_st unchanged, others set cur_dts in their native timebase
 * only needed for timestamp wrapping or if (dts not set and pts!=dts).
 * @param timestamp new dts expressed in time_base of param ref_st
 * @param ref_st reference stream giving time_base of param timestamp
 */
void av_update_cur_dts(AVFormatContext * s, AVStream * ref_st, int64_t timestamp);

/**
 * Does a binary search using read_timestamp().
 * This is not supposed to be called directly by a user application, but by demuxers.
 * @param target_ts target timestamp in the time base of the given stream
 * @param stream_index stream number
 */
int64_t av_gen_search(AVFormatContext * s, int stream_index, int64_t target_ts, int64_t pos_min, int64_t pos_max,
		int64_t pos_limit, int64_t ts_min, int64_t ts_max, int flags, int64_t * ts_ret,
		int64_t(*read_timestamp) (struct AVFormatContext *, int, int64_t *, int64_t));

/** media file output */
int av_set_parameters(AVFormatContext * s, AVFormatParameters * ap);

/**
 * Returns in 'buf' the path with '%d' replaced by number.

 * Also handles the '%0nd' format where 'n' is the total number
 * of digits and '%%'.
 *
 * @param buf destination buffer
 * @param buf_size destination buffer size
 * @param path numbered sequence string
 * @number frame number
 * @return 0 if OK, -1 if format error.
 */
int av_get_frame_filename(char *buf, int buf_size, const char *path, int number);

/**
 * Check whether filename actually is a numbered sequence generator.
 *
 * @param filename possible numbered sequence string
 * @return 1 if a valid numbered sequence string, 0 otherwise.
 */
int av_filename_number_test(const char *filename);

int av_write_trailer(AVFormatContext *s);
/**
 * Generate an SDP for an RTP session.
 *
 * @param ac array of AVFormatContexts describing the RTP streams. If the
 *           array is composed by only one context, such context can contain
 *           multiple AVStreams (one AVStream per RTP stream). Otherwise,
 *           all the contexts in the array (an AVCodecContext per RTP stream)
 *           must contain only one AVStream
 * @param n_files number of AVCodecContexts contained in ac
 * @param buff buffer where the SDP will be stored (must be allocated by
 *             the caller
 * @param size the size of the buffer
 * @return 0 if OK. AVERROR_xxx if error.
 */
int avf_sdp_create(AVFormatContext * ac[], int n_files, char *buff, int size);

void av_dynarray_add(void *tab_ptr, int *nb_ptr, void *elem);
int av_dynarray_add_nofree(void *tab_ptr, int *nb_ptr, void *elem);

#ifdef __GNUC__
#define dynarray_add(tab, nb_ptr, elem)\
	do {\
	    typeof(tab) _tab = (tab);\
	    typeof(elem) _elem = (elem);\
	    (void)sizeof(**_tab == _elem); /* check that types are compatible */\
	    av_dynarray_add(_tab, nb_ptr, _elem);\
	} while(0)
#else
#define dynarray_add(tab, nb_ptr, elem)\
	do {\
		av_dynarray_add((tab), nb_ptr, (elem));\
	} while(0)
#endif

time_t mktimegm(struct tm *tm);
struct tm *brktimegm(time_t secs, struct tm *tm);
const char *small_strptime(const char *p, const char *fmt, struct tm *dt);
time_t av_timegm(struct tm *tm);
int av_parse_time(int64_t *timeval, const char *timestr, int duration);
int av_find_info_tag(char *arg, int arg_size, const char *tag1, const char *info);
char *av_small_strptime(const char *p, const char *fmt, struct tm *dt);

struct in_addr;
int resolve_host(struct in_addr *sin_addr, const char *hostname);

uint64_t av_gettime(void);

/**
 * Split a URL string into components.
 *
 * The pointers to buffers for storing individual components may be null,
 * in order to ignore that component. Buffers for components not found are
 * set to empty strings. If the port is not found, it is set to a negative
 * value.
 *
 * @param proto the buffer for the protocol
 * @param proto_size the size of the proto buffer
 * @param authorization the buffer for the authorization
 * @param authorization_size the size of the authorization buffer
 * @param hostname the buffer for the host name
 * @param hostname_size the size of the hostname buffer
 * @param port_ptr a pointer to store the port number in
 * @param path the buffer for the path
 * @param path_size the size of the path buffer
 * @param url the URL to split
 */
void av_url_split(char *proto, int proto_size,
		char *authorization, int authorization_size,
		char *hostname, int hostname_size, int *port_ptr, char *path, int path_size, const char *url);

/**
 * Assemble a URL string from components. This is the reverse operation
 * of av_url_split.
 *
 * Note, this requires networking to be initialized, so the caller must
 * ensure ff_network_init has been called.
 *
 * @see av_url_split
 *
 * @param str the buffer to fill with the url
 * @param size the size of the str buffer
 * @param proto the protocol identifier, if null, the separator
 *              after the identifier is left out, too
 * @param authorization an optional authorization string, may be null.
 *                      An empty string is treated the same as a null string.
 * @param hostname the host name string
 * @param port the port number, left out from the string if negative
 * @param fmt a generic format string for everything to add after the
 *            host/port, may be null
 * @return the number of characters written to the destination buffer
 */
int ff_url_join(char *str, int size, const char *proto,
		const char *authorization, const char *hostname,
		int port, const char *fmt, ...);

int av_match_ext(const char *filename, const char *extensions);


int aac_get_sample_rate_index(uint32_t sample_rate);

char *ff_data_to_hex(char *buff, const uint8_t *src, int s, int lowercase);

int ff_hex_to_data(uint8_t *data, const char *p);

void ff_reduce_index(AVFormatContext *s, int stream_index);

int ff_index_search_timestamp(AVStream* st, const AVIndexEntry *entries, int nb_entries,
		int64_t wanted_timestamp, int flags, int64_t tolerance, int ass_is_forward);
/**
 * Callback function type for ff_parse_key_value.
 *
 * @param key a pointer to the key
 * @param key_len the number of bytes that belong to the key, including the '='
 *                char
 * @param dest return the destination pointer for the value in *dest, may
 *             be null to ignore the value
 * @param dest_len the length of the *dest buffer
 */
typedef void (*ff_parse_key_val_cb)(void *context, const char *key,
		int key_len, char **dest, int *dest_len);
/**
 * Parse a string with comma-separated key=value pairs. The value strings
 * may be quoted and may contain escaped characters within quoted strings.
 *
 * @param str the string to parse
 * @param callback_get_buf function that returns where to store the
 *                         unescaped value string.
 * @param context the opaque context pointer to pass to callback_get_buf
 */
void ff_parse_key_value(const char *str, ff_parse_key_val_cb callback_get_buf,
		void *context);

AVInputFormat* av_probe_input_format2(AVProbeData * pd, int is_opened, int *score_max);
int avformat_seek_file(AVFormatContext *s, int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags);
AVInputFormat  *av_iformat_next(AVInputFormat  *f);
AVOutputFormat *av_oformat_next(AVOutputFormat *f);

void av_shrink_packet(AVPacket *pkt, int size);

int av_packet_merge_side_data(AVPacket *pkt);
int av_copy_packet_side_data(AVPacket *pkt, AVPacket *src);
int av_packet_unpack_dictionary(const uint8_t *data, int size, AVDictionary **dict);
uint8_t *av_packet_pack_dictionary(AVDictionary *dict, int *size);

int av_get_audio_frame_duration(AVCodecContext *avctx, int frame_bytes);
const char *avcodec_get_name(enum CodecID id);
void av_dump_format(AVFormatContext *ic, int index, const char *url, int is_output);
const char *av_get_media_type_string(enum AVMediaType media_type);
const char *av_default_item_name(void *ptr);
AVOutputFormat *av_guess_format(const char *short_name, const char *filename,
		const char *mime_type);
int avformat_write_header(AVFormatContext *s, AVDictionary **options);
const uint8_t *avpriv_find_start_code(const uint8_t *p, const uint8_t *end, uint32_t *state);
int av_write_frame(AVFormatContext *s, AVPacket *pkt);
int avcodec_copy_context(AVCodecContext *dest, const AVCodecContext *src);
int avformat_alloc_output_context2(AVFormatContext **avctx, AVOutputFormat *oformat,
		const char *format, const char *filename);
unsigned int av_xiphlacing(unsigned char *s, unsigned int v);
uint8_t *av_stream_new_side_data(AVStream *st, enum AVPacketSideDataType type,
                                 int size);

extern int ff_alloc_extradata(AVCodecContext *avctx, int size);
void ff_read_frame_flush(AVFormatContext *s);
int av_probe_input_buffer(ByteIOContext *pb, AVInputFormat **fmt,
		const char *filename, void *logctx,
		unsigned int offset, unsigned int max_probe_size);
void ff_format_io_close(AVFormatContext *s, AVIOContext **pb);
void ff_make_absolute_url(char *buf, int size, const char *base,
		const char *rel);
AVProgram *av_find_program_from_stream(AVFormatContext *ic, AVProgram *last, int s);
int avformat_control(AVFormatContext **ps, int cmd, void *arg);

int64_t gx_av_gettime_relative(void);
int gx_av_get_random_seed(void);
int avcodec_hls_context_copy(AVCodecContext *dst, AVCodecContext *src);
void codec_hls_context_reset(AVCodecContext *par);
void ff_update_cur_dts(AVFormatContext *s, AVStream *ref_st, int64_t timestamp);
AVChapter *avpriv_new_chapter(AVFormatContext *s, int id, AVRational time_base,
                              int64_t start, int64_t end, const char *title);
int av_stream_add_side_data(AVStream *st, enum AVPacketSideDataType type,
		                            uint8_t *data, size_t size);
#endif				/* AVFORMAT_H */
