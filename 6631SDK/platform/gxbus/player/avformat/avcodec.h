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

#ifndef AVCODEC_H
#define AVCODEC_H

/**
 * @file avcodec.h
 * external API header
 */
#include "../avutil/avutil.h"

#define AV_NOPTS_VALUE          (-1)
#define AV_TIME_BASE            1000000
#define AV_TIME_BASE_Q          (AVRational){1, AV_TIME_BASE}

#define AV_EF_EXPLODE   (1<<3)

#define AV_PKT_FLAG_KEY     0x0001 ///< The packet contains a keyframe
#define AV_PKT_FLAG_CORRUPT 0x0002 ///< The packet content is corrupted

enum AVAudioServiceType {
    AV_AUDIO_SERVICE_TYPE_MAIN              = 0,
    AV_AUDIO_SERVICE_TYPE_EFFECTS           = 1,
    AV_AUDIO_SERVICE_TYPE_VISUALLY_IMPAIRED = 2,
    AV_AUDIO_SERVICE_TYPE_HEARING_IMPAIRED  = 3,
    AV_AUDIO_SERVICE_TYPE_DIALOGUE          = 4,
    AV_AUDIO_SERVICE_TYPE_COMMENTARY        = 5,
    AV_AUDIO_SERVICE_TYPE_EMERGENCY         = 6,
    AV_AUDIO_SERVICE_TYPE_VOICE_OVER        = 7,
    AV_AUDIO_SERVICE_TYPE_KARAOKE           = 8,
    AV_AUDIO_SERVICE_TYPE_NB                   , ///< Not part of ABI
};

/* Currently unused, may be used if 24/32 bits samples are ever supported. */
/* all in native-endian format */
enum SampleFormat {
	SAMPLE_FMT_NONE = -1,
	SAMPLE_FMT_U8,		///< unsigned 8 bits
	SAMPLE_FMT_S16,		///< signed 16 bits
	SAMPLE_FMT_S24,		///< signed 24 bits
	SAMPLE_FMT_S32,		///< signed 32 bits
	SAMPLE_FMT_FLT,		///< float

	SAMPLE_FMT_U8P,		///< unsigned 8 bits, planar
	SAMPLE_FMT_S16P,	///< signed 16 bits, planar
	SAMPLE_FMT_S32P,	///< signed 32 bits, planar
	SAMPLE_FMT_FLTP,	///< float, planar
};

/**
 * Required number of additionally allocated bytes at the end of the input bitstream for decoding.
 * This is mainly needed because some optimized bitstream readers read
 * 32 or 64 bit at once and could read over the end.<br>
 * Note: If the first 23 bits of the additional bytes are not 0, then damaged
 * MPEG bitstreams could cause overread and segfault.
 */

enum AVDiscard {
	/* We leave some space between them for extensions (drop some
	 * keyframes for intra-only or drop just some bidir frames). */
	AVDISCARD_NONE = -16,	///< discard nothing
	AVDISCARD_DEFAULT = 0,	///< discard useless packets like 0 size packets in avi
	AVDISCARD_NONREF = 8,	///< discard all non reference
	AVDISCARD_BIDIR = 16,	///< discard all bidirectional frames
	AVDISCARD_NONKEY = 32,	///< discard all frames except keyframes
	AVDISCARD_ALL = 48,	///< discard all
};

/* encoding support
   These flags can be passed in AVCodecContext.flags before initialization.
   Note: Not everything is supported yet.
*/

#define CODEC_FLAG_QSCALE 0x0002	///< Use fixed qscale.
#define CODEC_FLAG_4MV    0x0004	///< 4 MV per MB allowed / advanced prediction for H.263.
#define CODEC_FLAG_QPEL   0x0010	///< Use qpel MC.
#define CODEC_FLAG_GMC    0x0020	///< Use GMC.
#define CODEC_FLAG_MV0    0x0040	///< Always try a MB with MV=<0,0>.
#define CODEC_FLAG_PART   0x0080	///< Use data partitioning.
/* The parent program guarantees that the input for B-frames containing
 * streams is not written to for at least s->max_b_frames+1 frames, if
 * this is not set the input will be copied. */
#define CODEC_FLAG_INPUT_PRESERVED 0x0100
#define CODEC_FLAG_PASS1           0x0200	///< Use internal 2pass ratecontrol in first pass mode.
#define CODEC_FLAG_PASS2           0x0400	///< Use internal 2pass ratecontrol in second pass mode.
#define CODEC_FLAG_EXTERN_HUFF     0x1000	///< Use external Huffman table (for MJPEG).
#define CODEC_FLAG_GRAY            0x2000	///< Only decode/encode grayscale.
#define CODEC_FLAG_EMU_EDGE        0x4000	///< Don't draw edges.
#define CODEC_FLAG_PSNR            0x8000	///< error[?] variables will be set during encoding.
#define CODEC_FLAG_TRUNCATED       0x00010000 /** Input bitstream might be truncated at a random
                                                  location instead of only at frame boundaries. */
#define CODEC_FLAG_NORMALIZE_AQP  0x00020000	///< Normalize adaptive quantization.
#define CODEC_FLAG_INTERLACED_DCT 0x00040000	///< Use interlaced DCT.
#define CODEC_FLAG_LOW_DELAY      0x00080000	///< Force low delay.
#define CODEC_FLAG_ALT_SCAN       0x00100000	///< Use alternate scan.
#define CODEC_FLAG_TRELLIS_QUANT  0x00200000	///< Use trellis quantization.
#define CODEC_FLAG_GLOBAL_HEADER  0x00400000	///< Place global headers in extradata instead of every keyframe.
#define CODEC_FLAG_BITEXACT       0x00800000	///< Use only bitexact stuff (except (I)DCT).

#define CODEC_FLAG_AC_PRED        0x01000000	///< H.263 advanced intra coding / MPEG-4 AC prediction
#define CODEC_FLAG_H263P_UMV      0x02000000	///< unlimited motion vector
#define CODEC_FLAG_CBP_RD         0x04000000	///< Use rate distortion optimization for cbp.
#define CODEC_FLAG_QP_RD          0x08000000	///< Use rate distortion optimization for qp selectioon.
#define CODEC_FLAG_H263P_AIV      0x00000008	///< H.263 alternative inter VLC
#define CODEC_FLAG_OBMC           0x00000001	///< OBMC
#define CODEC_FLAG_LOOP_FILTER    0x00000800	///< loop filter
#define CODEC_FLAG_H263P_SLICE_STRUCT 0x10000000
#define CODEC_FLAG_INTERLACED_ME  0x20000000	///< interlaced motion estimation
#define CODEC_FLAG_SVCD_SCAN_OFFSET 0x40000000	///< Will reserve space for SVCD scan offset user data.
#define CODEC_FLAG_CLOSED_GOP     ((int)0x80000000)
#define CODEC_FLAG2_FAST          0x00000001	///< Allow non spec compliant speedup tricks.
#define CODEC_FLAG2_STRICT_GOP    0x00000002	///< Strictly enforce GOP size.
#define CODEC_FLAG2_NO_OUTPUT     0x00000004	///< Skip bitstream encoding.
#define CODEC_FLAG2_LOCAL_HEADER  0x00000008	///< Place global headers at every keyframe instead of in extradata.
#define CODEC_FLAG2_BPYRAMID      0x00000010	///< H.264 allow B-frames to be used as references.
#define CODEC_FLAG2_WPRED         0x00000020	///< H.264 weighted biprediction for B-frames
#define CODEC_FLAG2_MIXED_REFS    0x00000040	///< H.264 one reference per partition, as opposed to one reference per macroblock
#define CODEC_FLAG2_8X8DCT        0x00000080	///< H.264 high profile 8x8 transform
#define CODEC_FLAG2_FASTPSKIP     0x00000100	///< H.264 fast pskip
#define CODEC_FLAG2_AUD           0x00000200	///< H.264 access unit delimiters
#define CODEC_FLAG2_BRDO          0x00000400	///< B-frame rate-distortion optimization
#define CODEC_FLAG2_INTRA_VLC     0x00000800	///< Use MPEG-2 intra VLC table.
#define CODEC_FLAG2_MEMC_ONLY     0x00001000	///< Only do ME/MC (I frames -> ref, P frame -> ME+MC).
#define CODEC_FLAG2_DROP_FRAME_TIMECODE 0x00002000	///< timecode is in drop frame format.
#define CODEC_FLAG2_SKIP_RD       0x00004000	///< RD optimal MB level residual skipping
#define CODEC_FLAG2_CHUNKS        0x00008000	///< Input bitstream might be truncated at a packet boundaries instead of only at frame boundaries.
#define CODEC_FLAG2_NON_LINEAR_QUANT 0x00010000	///< Use MPEG-2 nonlinear quantizer.

#define FF_COMMON_FRAME \
    /**\
     * pointer to the picture planes.\
     * This might be different from the first allocated byte\
     * - encoding: \
     * - decoding: \
     */\
    uint8_t *data[4];\
    int linesize[4];\
    /**\
     * pointer to the first allocated byte of the picture. Can be used in get_buffer/release_buffer.\
     * This isn't used by libavcodec unless the default get/release_buffer() is used.\
     * - encoding: \
     * - decoding: \
     */\
    uint8_t *base[4];\
    /**\
     * 1 -> keyframe, 0-> not\
     * - encoding: Set by libavcodec.\
     * - decoding: Set by libavcodec.\
     */\
    int key_frame;\
\
    /**\
     * Picture type of the frame, see ?_TYPE below.\
     * - encoding: Set by libavcodec. for coded_picture (and set by user for input).\
     * - decoding: Set by libavcodec.\
     */\
    int pict_type;\
\
    /**\
     * presentation timestamp in time_base units (time when frame should be shown to user)\
     * If AV_NOPTS_VALUE then frame_rate = 1/time_base will be assumed.\
     * - encoding: MUST be set by user.\
     * - decoding: Set by libavcodec.\
     */\
    int64_t pts;\
\
    /**\
     * picture number in bitstream order\
     * - encoding: set by\
     * - decoding: Set by libavcodec.\
     */\
    int coded_picture_number;\
    /**\
     * picture number in display order\
     * - encoding: set by\
     * - decoding: Set by libavcodec.\
     */\
    int display_picture_number;\
\
    /**\
     * quality (between 1 (good) and FF_LAMBDA_MAX (bad)) \
     * - encoding: Set by libavcodec. for coded_picture (and set by user for input).\
     * - decoding: Set by libavcodec.\
     */\
    int quality; \
\
    /**\
     * buffer age (1->was last buffer and dint change, 2->..., ...).\
     * Set to INT_MAX if the buffer has not been used yet.\
     * - encoding: unused\
     * - decoding: MUST be set by get_buffer().\
     */\
    int age;\
\
    /**\
     * is this picture used as reference\
     * - encoding: unused\
     * - decoding: Set by libavcodec. (before get_buffer() call)).\
     */\
    int reference;\
\
    /**\
     * QP table\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */\
    int8_t *qscale_table;\
    /**\
     * QP store stride\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */\
    int qstride;\
\
    /**\
     * mbskip_table[mb]>=1 if MB didn't change\
     * stride= mb_width = (width+15)>>4\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */\
    uint8_t *mbskip_table;\
\
    /**\
     * motion vector table\
     * @code\
     * example:\
     * int mv_sample_log2= 4 - motion_subsample_log2;\
     * int mb_width= (width+15)>>4;\
     * int mv_stride= (mb_width << mv_sample_log2) + 1;\
     * motion_val[direction][x + y*mv_stride][0->mv_x, 1->mv_y];\
     * @endcode\
     * - encoding: Set by user.\
     * - decoding: Set by libavcodec.\
     */\
    int16_t (*motion_val[2])[2];\
\
    /**\
     * macroblock type table\
     * mb_type_base + mb_width + 2\
     * - encoding: Set by user.\
     * - decoding: Set by libavcodec.\
     */\
    uint32_t *mb_type;\
\
    /**\
     * log2 of the size of the block which a single vector in motion_val represents: \
     * (4->16x16, 3->8x8, 2-> 4x4, 1-> 2x2)\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */\
    uint8_t motion_subsample_log2;\
\
    /**\
     * for some private data of the user\
     * - encoding: unused\
     * - decoding: Set by user.\
     */\
    void *opaque;\
\
    /**\
     * error\
     * - encoding: Set by libavcodec. if flags&CODEC_FLAG_PSNR.\
     * - decoding: unused\
     */\
    uint64_t error[4];\
\
    /**\
     * type of the buffer (to keep track of who has to deallocate data[*])\
     * - encoding: Set by the one who allocates it.\
     * - decoding: Set by the one who allocates it.\
     * Note: User allocated (direct rendering) & internal buffers cannot coexist currently.\
     */\
    int type;\
    \
    /**\
     * When decoding, this signals how much the picture must be delayed.\
     * extra_delay = repeat_pict / (2*fps)\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */\
    int repeat_pict;\
    \
    /**\
     * \
     */\
    int qscale_type;\
    \
    /**\
     * The content of the picture is interlaced.\
     * - encoding: Set by user.\
     * - decoding: Set by libavcodec. (default 0)\
     */\
    int interlaced_frame;\
    \
    /**\
     * If the content is interlaced, is top field displayed first.\
     * - encoding: Set by user.\
     * - decoding: Set by libavcodec.\
     */\
    int top_field_first;\
    \
    \
    /**\
     * Tell user application that palette has changed from previous frame.\
     * - encoding: ??? (no palette-enabled encoder yet)\
     * - decoding: Set by libavcodec. (default 0).\
     */\
    int palette_has_changed;\
    \
    /**\
     * codec suggestion on buffer type if != 0\
     * - encoding: unused\
     * - decoding: Set by libavcodec. (before get_buffer() call)).\
     */\
    int buffer_hints;\
\
    /**\
     * DCT coefficients\
     * - encoding: unused\
     * - decoding: Set by libavcodec.\
     */\
    short *dct_coeff;\
\
    /**\
     * motion referece frame index\
     * - encoding: Set by user.\
     * - decoding: Set by libavcodec.\
     */\
    int8_t *ref_index[2];

#define FF_QSCALE_TYPE_MPEG1 0
#define FF_QSCALE_TYPE_MPEG2 1
#define FF_QSCALE_TYPE_H264  2

#define FF_BUFFER_TYPE_INTERNAL 1
#define FF_BUFFER_TYPE_USER     2	///< direct rendering buffers (image is (de)allocated by user)
#define FF_BUFFER_TYPE_SHARED   4	///< Buffer from somewhere else; don't deallocate image (data/base), all other tables are not shared.
#define FF_BUFFER_TYPE_COPY     8	///< Just a (modified) copy of some other buffer, don't deallocate anything.

#define FF_I_TYPE  1		// Intra
#define FF_P_TYPE  2		// Predicted
#define FF_B_TYPE  3		// Bi-dir predicted
#define FF_S_TYPE  4		// S(GMC)-VOP MPEG4
#define FF_SI_TYPE 5
#define FF_SP_TYPE 6

#define FF_BUFFER_HINTS_VALID    0x01	// Buffer hints value is meaningful (if 0 ignore).
#define FF_BUFFER_HINTS_READABLE 0x02	// Codec will read from buffer.
#define FF_BUFFER_HINTS_PRESERVE 0x04	// User must not alter buffer content.
#define FF_BUFFER_HINTS_REUSABLE 0x08	// Codec will reuse the buffer (update).

/**
 * Audio Video Frame.
 */
typedef struct AVFrame {
	FF_COMMON_FRAME
    AVRational sample_aspect_ratio;
	int nb_samples;
} AVFrame;

enum AVSideDataParamChangeFlags {
    AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_COUNT  = 0x0001,
    AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_LAYOUT = 0x0002,
    AV_SIDE_DATA_PARAM_CHANGE_SAMPLE_RATE    = 0x0004,
    AV_SIDE_DATA_PARAM_CHANGE_DIMENSIONS     = 0x0008,
};

#define DEFAULT_FRAME_RATE_BASE 1001000
#define AV_EF_CRCCHECK  (1<<0)
#define AV_EF_BITSTREAM (1<<1)
#define AV_EF_BUFFER    (1<<2)
#define AV_EF_EXPLODE   (1<<3)

#define AV_EF_CAREFUL    (1<<16)
#define AV_EF_COMPLIANT  (1<<17)
#define AV_EF_AGGRESSIVE (1<<18)


/**
 * main external API structure
 */
typedef struct AVCodecContext {
    /**
     * the average bitrate
     * - encoding: Set by user; unused for constant quantizer encoding.
     * - decoding: Set by libavcodec. 0 or some bitrate if this info is available in the stream.
     */
	int bit_rate;

	enum CodecType codec_type;	/* see CODEC_TYPE_xxx */
	enum CodecID codec_id;	/* see CODEC_ID_xxx */

    /**
     * fourcc (LSB first, so "ABCD" -> ('D'<<24) + ('C'<<16) + ('B'<<8) + 'A').
     * This is used to work around some encoder bugs.
     * A demuxer should set this to what is stored in the field used to identify the codec.
     * If there are multiple such fields in a container then the demuxer should choose the one
     * which maximizes the information about the used codec.
     * If the codec tag field in a container is larger then 32 bits then the demuxer should
     * remap the longer ID to 32 bits with a table or other structure. Alternatively a new
     * extra_codec_tag + size could be added but for this a clear advantage must be demonstrated
     * first.
     * - encoding: Set by user, if not then the default based on codec_id will be used.
     * - decoding: Set by user, will be converted to uppercase by libavcodec during init.
     */
	unsigned int codec_tag;
	/* audio only */
	int sample_rate;	///< samples per second
	int channels;
    /**
     * audio sample format
     * - encoding: Set by user.
     * - decoding: Set by libavcodec.
     */
	enum SampleFormat sample_fmt;	///< sample format, currently unused
    char codec_name[32];
    int color_table_id;

	int frame_rate;
	int frame_size;
	    /**
     * some codecs need / can use extradata like Huffman tables.
     * mjpeg: Huffman tables
     * rv10: additional flags
     * mpeg4: global headers (they can be in the bitstream or here)
     * The allocated memory should be FF_INPUT_BUFFER_PADDING_SIZE bytes larger
     * than extradata_size to avoid prolems if it is read with the bitstream reader.
     * The bytewise contents of extradata must not depend on the architecture or CPU endianness.
     * - encoding: Set/allocated/freed by libavcodec.
     * - decoding: Set/allocated/freed by user.
     */
	uint8_t *extradata;
	int extradata_size;
	int is_reset_extradata;
	int nal_handle;

	    /**
     * bits per sample/pixel from the demuxer (needed for huffyuv).
     * - encoding: Set by libavcodec.
     * - decoding: Set by user.
     */
	int bits_per_sample;

    /**
     * rate control equation
     * - encoding: Set by user
     * - decoding: unused
     */
	char *rc_eq;
	/* video only */
    /**
     * picture width / height.
     * - encoding: MUST be set by user.
     * - decoding: Set by libavcodec.
     * Note: For compatibility it is possible to set this instead of
     * coded_width/height before decoding.
     */
	int width, height;

	/**
	 * Audio only. The number of "priming" samples (padding) inserted by the
	 * encoder at the beginning of the audio. I.e. this number of leading
	 * decoded samples must be discarded by the caller to get the original audio
	 * without leading padding.
	 *
	 * - decoding: unused
	 * - encoding: Set by libavcodec. The timestamps on the output packets are
	 *             adjusted by the encoder so that they always refer to the
	 *             first sample of the data actually contained in the packet,
	 *             including any added padding.  E.g. if the timebase is
	 *             1/samplerate and the timestamp of the first input sample is
	 *             0, the timestamp of the first output packet will be
	 *             -initial_padding.
	 */
	int initial_padding;

    /**
     * This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identically 1.
     * - encoding: MUST be set by user.
     * - decoding: Set by libavcodec.
     */

	AVRational time_base;
    /**
     * sample aspect ratio (0 if unknown)
     * Numerator and denominator must be relatively prime and smaller than 256 for some video standards.
     * - encoding: Set by user.
     * - decoding: Set by libavcodec.
     */
	AVRational sample_aspect_ratio;
	    /**
     * Pixel format, see PIX_FMT_xxx.
     * - encoding: Set by user.
     * - decoding: Set by libavcodec.
     */
	enum PixelFormat pix_fmt;

    /**
     * palette control structure
     * - encoding: ??? (no palette-enabled encoder yet)
     * - decoding: Set by user.
     */
	struct AVPaletteControl *palctrl;
    /**
     * CODEC_FLAG_*.
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
	int flags;

	  /**
     * CODEC_FLAG2_*
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
	int flags2;
	    /**
     * number of bytes per packet if constant and known or 0
     * Used by some WAV based audio codecs.
     */
	int block_align;
	int has_b_frames;
   /**
     * maximum number of B-frames between non-B-frames
     * Note: The output will be delayed by max_b_frames+1 relative to the input.
     * - encoding: Set by user.
     * - decoding: unused
     */
	int max_b_frames;

       /**
     * fourcc from the AVI stream header (LSB first, so "ABCD" -> ('D'<<24) + ('C'<<16) + ('B'<<8) + 'A').
     * This is used to work around some encoder bugs.
     * - encoding: unused
     * - decoding: Set by user, will be converted to uppercase by libavcodec during init.
     */
    unsigned int stream_codec_tag;


    /**
     * bits per sample/pixel from the demuxer (needed for huffyuv).
     * - encoding: Set by libavcodec.
     * - decoding: Set by user.
     */
     int bits_per_coded_sample;

    /**
     * Bits per sample/pixel of internal libavcodec pixel/sample format.
     * This field is applicable only when sample_fmt is SAMPLE_FMT_S32.
     * - encoding: set by user.
     * - decoding: set by libavcodec.
     */
    int bits_per_raw_sample;


    /**
     * Channel layout of the audio data.
     *
     * - encoding: unused
     * - decoding: read by user.
     */
    uint64_t channel_layout;

    int ticks_per_frame;

    /**
     * Codec delay.
     *
     * Encoding: Number of frames delay there will be from the encoder input to
     *           the decoder output. (we assume the decoder matches the spec)
     * Decoding: Number of frames delay in addition to what a standard decoder
     *           as specified in the spec would produce.
     *
     * Video:
     *   Number of frames the decoded output will be delayed relative to the
     *   encoded input.
     *
     * Audio:
     *   For encoding, this is the number of "priming" samples added to the
     *   beginning of the stream. The decoded output will be delayed by this
     *   many samples relative to the input to the encoder. Note that this
     *   field is purely informational and does not directly affect the pts
     *   output by the encoder, which should always be based on the actual
     *   presentation time, including any delay.
     *   For decoding, this is the number of samples the decoder needs to
     *   output before the decoder's output is valid. When seeking, you should
     *   start decoding this many samples prior to your desired seek point.
     *
     * - encoding: Set by libavcodec.
     * - decoding: Set by libavcodec.
     */
    int delay;


    /**
     * strictly follow the standard (MPEG4, ...).
     * - encoding: Set by user.
     * - decoding: Set by user.
     * Setting this to STRICT or higher means the encoder and decoder will
     * generally do stupid things, whereas setting it to unofficial or lower
     * will mean the encoder might produce output that is not supported by all
     * spec-compliant decoders. Decoders don't differentiate between normal,
     * unofficial and experimental (that is, they always try to decode things
     * when they can) unless they are explicitly asked to behave stupidly
     * (=strictly conform to the specs)
     */
    //int strict_std_compliance;//no use.
#define FF_COMPLIANCE_VERY_STRICT   2 ///< Strictly conform to an older more strict version of the spec or reference software.
#define FF_COMPLIANCE_STRICT        1 ///< Strictly conform to all the things in the spec no matter what consequences.
#define FF_COMPLIANCE_NORMAL        0
#define FF_COMPLIANCE_UNOFFICIAL   -1 ///< Allow unofficial extensions
#define FF_COMPLIANCE_EXPERIMENTAL -2 ///< Allow nonstandardized experimental things.


	enum AVAudioServiceType audio_service_type;

	struct {
		int channels;
		int big_endian;
		int sample_rate;
		int sample_size;
	} bluray_pcm;

	void* priv_data;
} AVCodecContext;

/**
 * four components are given, that's all.
 * the last component is alpha
 */
typedef struct AVPicture {
	uint8_t *data[4];
	int linesize[4];	///< number of bytes per line
} AVPicture;

/**
 * AVPaletteControl
 * This structure defines a method for communicating palette changes
 * between and demuxer and a decoder.
 *
 * @deprecated Use AVPacket to send palette changes instead.
 * This is totally broken.
 */
#define AVPALETTE_SIZE 1024
#define AVPALETTE_COUNT 256
typedef struct AVPaletteControl {

	/* Demuxer sets this to 1 to indicate the palette has changed;
	 * decoder resets to 0. */
	int palette_changed;

	/* 4-byte ARGB palette entries, stored in native byte order; note that
	 * the individual palette components should be on a 8-bit scale; if
	 * the palette data comes from an IBM VGA native format, the component
	 * data is probably 6 bits in size and needs to be scaled. */
	unsigned int palette[AVPALETTE_COUNT];

} AVPaletteControl ;

/**
 * Sets the fields of the given AVCodecContext to default values.
 *
 * @param s The AVCodecContext of which the fields should be set to default values.
 */
void avcodec_get_context_defaults(AVCodecContext * s);

/** THIS FUNCTION IS NOT YET PART OF THE PUBLIC API!
 *  we WILL change its arguments and name a few times! */
void avcodec_get_context_defaults2(AVCodecContext * s, enum CodecType);

/**
 * Allocates an AVCodecContext and sets its fields to default values.  The
 * resulting struct can be deallocated by simply calling av_free().
 *
 * @return An AVCodecContext filled with default values or NULL on failure.
 * @see avcodec_get_context_defaults
 */
AVCodecContext *avcodec_alloc_context(void);

/** THIS FUNCTION IS NOT YET PART OF THE PUBLIC API!
 *  we WILL change its arguments and name a few times! */
AVCodecContext *avcodec_alloc_context2(enum CodecType);

/**
 * Returns codec bits per sample.
 *
 * @param[in] codec_id the codec
 * @return Number of bits per sample or zero if unknown for the given codec.
 */
int av_get_bits_per_sample(enum CodecID codec_id);

int av_get_audio_frame_duration(AVCodecContext *avctx, int frame_bytes);

/* frame parsing */
typedef struct AVCodecParserContext {
	void *priv_data;
	struct AVCodecParser *parser;
	int64_t frame_offset;	/* offset of the current frame */
	int64_t cur_offset;	/* current offset
				   (incremented by each av_parser_parse()) */
	int64_t next_frame_offset; /*  offset of the next frame */
	int64_t last_frame_offset;	/* offset of the last frame */
	/* video info */
	int pict_type;		/* XXX: Put it back in AVCodecContext. */
	int repeat_pict;	/* XXX: Put it back in AVCodecContext. */
	int64_t pts;		/* pts of the current frame */
	int64_t dts;		/* dts of the current frame */

	/* private data */
	int64_t last_pts;
	int64_t last_dts;
	int fetch_timestamp;

#define AV_PARSER_PTS_NB 4
	int cur_frame_start_index;
	int64_t cur_frame_offset[AV_PARSER_PTS_NB];
	int64_t cur_frame_pts[AV_PARSER_PTS_NB];
	int64_t cur_frame_dts[AV_PARSER_PTS_NB];

	int flags;
#define PARSER_FLAG_COMPLETE_FRAMES           0x0001

#define PARSER_FLAG_ONCE                      0x0002
/// Set if the parser has a valid file offset
#define PARSER_FLAG_FETCHED_OFFSET            0x0004
#define PARSER_FLAG_USE_CODEC_TS              0x1000
	int64_t offset;		///< byte offset from starting packet start
	int64_t cur_frame_end[AV_PARSER_PTS_NB];
	int64_t last_offset;

    int64_t convergence_duration;
    int dts_sync_point;
    int dts_ref_dts_delta;
    int pts_dts_delta;
	int64_t cur_frame_pos[AV_PARSER_PTS_NB];
    int64_t pos;
	int64_t last_pos;
    int duration;
    int key_frame;
} AVCodecParserContext;

typedef struct AVCodecParser {
	int codec_ids[5];	/* several codec IDs are permitted */
	int priv_data_size;
	int (*parser_init) (AVCodecParserContext * s);
	int (*parser_parse) (AVCodecParserContext * s,
			     AVCodecContext * avctx,
			     const uint8_t ** poutbuf, int *poutbuf_size, const uint8_t * buf, int buf_size);
	void (*parser_close) (AVCodecParserContext * s);
	int (*split) (AVCodecContext * avctx, const uint8_t * buf, int buf_size);
	struct AVCodecParser *next;
} AVCodecParser;

extern AVCodecParser *av_first_parser;

void av_register_codec_parser(AVCodecParser * parser);
AVCodecParserContext *av_parser_init(int codec_id);
int av_parser_parse(AVCodecParserContext * s,
		    AVCodecContext * avctx,
		    uint8_t ** poutbuf, int *poutbuf_size, const uint8_t * buf, int buf_size, int64_t pts, int64_t dts);
int av_parser_parse2(AVCodecParserContext * s,
		    AVCodecContext * avctx,
		    uint8_t ** poutbuf, int *poutbuf_size, const uint8_t * buf, int buf_size, int64_t pts, int64_t dts,int64_t pos);
void ff_fetch_timestamp(AVCodecParserContext *s, int off, int remove);
int av_parser_change(AVCodecParserContext * s,
		     AVCodecContext * avctx,
		     uint8_t ** poutbuf, int *poutbuf_size, const uint8_t * buf, int buf_size, int keyframe);
void av_parser_close(AVCodecParserContext * s);

/**
 * @defgroup lavc_packet AVPacket
 *
 * Types and functions for working with AVPacket.
 * @{
 */
enum AVPacketSideDataType {
    AV_PKT_DATA_PALETTE,
    AV_PKT_DATA_NEW_EXTRADATA,

    /**
     * An AV_PKT_DATA_PARAM_CHANGE side data packet is laid out as follows:
     * @code
     * u32le param_flags
     * if (param_flags & AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_COUNT)
     *     s32le channel_count
     * if (param_flags & AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_LAYOUT)
     *     u64le channel_layout
     * if (param_flags & AV_SIDE_DATA_PARAM_CHANGE_SAMPLE_RATE)
     *     s32le sample_rate
     * if (param_flags & AV_SIDE_DATA_PARAM_CHANGE_DIMENSIONS)
     *     s32le width
     *     s32le height
     * @endcode
     */
    AV_PKT_DATA_PARAM_CHANGE,

    /**
     * An AV_PKT_DATA_H263_MB_INFO side data packet contains a number of
     * structures with info about macroblocks relevant to splitting the
     * packet into smaller packets on macroblock edges (e.g. as for RFC 2190).
     * That is, it does not necessarily contain info about all macroblocks,
     * as long as the distance between macroblocks in the info is smaller
     * than the target payload size.
     * Each MB info structure is 12 bytes, and is laid out as follows:
     * @code
     * u32le bit offset from the start of the packet
     * u8    current quantizer at the start of the macroblock
     * u8    GOB number
     * u16le macroblock address within the GOB
     * u8    horizontal MV predictor
     * u8    vertical MV predictor
     * u8    horizontal MV predictor for block number 3
     * u8    vertical MV predictor for block number 3
     * @endcode
     */
    AV_PKT_DATA_H263_MB_INFO,

	/**
	 * This side data should be associated with an audio stream and contains
	 * ReplayGain information in form of the AVReplayGain struct.
	 */
	AV_PKT_DATA_REPLAYGAIN,

    /**
     * A list of zero terminated key/value strings. There is no end marker for
     * the list, so it is required to rely on the side data size to stop.
     */
    AV_PKT_DATA_STRINGS_METADATA,

    /**
     * Recommmends skipping the specified number of samples
     * @code
     * u32le number of samples to skip from start of this packet
     * u32le number of samples to skip from end of this packet
     * u8    reason for start skip
     * u8    reason for end   skip (0=padding silence, 1=convergence)
     * @endcode
     */
    AV_PKT_DATA_SKIP_SAMPLES=70,

    /**
     * An AV_PKT_DATA_JP_DUALMONO side data packet indicates that
     * the packet may contain "dual mono" audio specific to Japanese DTV
     * and if it is true, recommends only the selected channel to be used.
     * @code
     * u8    selected channels (0=mail/left, 1=sub/right, 2=both)
     * @endcode
     */
    AV_PKT_DATA_JP_DUALMONO,
	AV_PKT_DATA_WEBVTT_IDENTIFIER,
	AV_PKT_DATA_WEBVTT_SETTINGS,
	AV_PKT_DATA_MATROSKA_BLOCKADDITIONAL,
	AV_PKT_DATA_METADATA_UPDATE,

    /**     
     * DOVI configuration
     * ref: 
     * dolby-vision-bitstreams-within-the-iso-base-media-file-format-v2.1.2, section 2.2
     * dolby-vision-bitstreams-in-mpeg-2-transport-stream-multiplex-v1.2, section 3.3
     * Tags are stored in struct AVDOVIDecoderConfigurationRecord.
     */
    AV_PKT_DATA_DOVI_CONF,
};

enum AVSubtitleType {
	SUBTITLE_NONE,
	SUBTITLE_BITMAP,                ///< A bitmap, pict will be set
	/**
	 * Plain text, the text field must be set by the decoder and is
	 * authoritative. ass and pict fields may contain approximations.
	 */
	SUBTITLE_TEXT,

	/**
	 * Formatted text, the ass field must be set by the decoder and is
	 * authoritative. pict and text fields may contain approximations.
	 */
	SUBTITLE_ASS,
};

#define AV_SUBTITLE_FLAG_FORCED 0x00000001

typedef struct AVSubtitleRect {
	int x;         ///< top left corner  of pict, undefined when pict is not set
	int y;         ///< top left corner  of pict, undefined when pict is not set
	int w;         ///< width            of pict, undefined when pict is not set
	int h;         ///< height           of pict, undefined when pict is not set
	int nb_colors; ///< number of colors in pict, undefined when pict is not set
	/**
	* data+linesize for the bitmap of this subtitle.
	* Can be set for text/ass as well once they are rendered.
	*/
	uint8_t *data[4];
	int linesize[4];
	enum AVSubtitleType type;
	char *text;                     ///< 0 terminated plain UTF-8 text
	/**
	 * 0 terminated ASS/SSA compatible event line.
	 * The presentation of this is unaffected by the other values in this struct.
	 */
	char *ass;
	int flags;
} AVSubtitleRect;

typedef struct AVSubtitle {
	uint16_t format; /* 0 = graphics */
	uint32_t start_display_time; /* relative to packet pts, in ms */
	uint32_t end_display_time; /* relative to packet pts, in ms */
	unsigned num_rects;
	AVSubtitleRect **rects;
	int64_t pts;    ///< Same as packet pts, in AV_TIME_BASE
} AVSubtitle;

#endif				/* AVCODEC_H */
