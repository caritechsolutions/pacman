#ifndef __VD_MPEG_ES_H__
#define __VD_MPEG_ES_H__



#include "vd.h"
#include "gx_image.h"

#define MP_IMGFIELD_TOP_FIRST 0x02

	typedef struct gx_mpeg1codec {
		GxAVCodec parent;

	} GxMPEG1Codec;

	typedef struct {
		uint8_t *ref[2][3];
		uint8_t **ref2[2];
		int pmv[2][2];
		int f_code[2];
	} motion_t;

	typedef struct mpeg2_sequence_s {
		unsigned int width, height;
		unsigned int chroma_width, chroma_height;
		unsigned int byte_rate;
		unsigned int vbv_buffer_size;
		uint32_t flags;

		unsigned int picture_width, picture_height;
		unsigned int display_width, display_height;
		unsigned int pixel_width, pixel_height;
		unsigned int frame_period;

		uint8_t profile_level_id;
		uint8_t colour_primaries;
		uint8_t transfer_characteristics;
		uint8_t matrix_coefficients;
	} mpeg2_sequence_t;

	typedef struct mpeg2_gop_s {
		uint8_t hours;
		uint8_t minutes;
		uint8_t seconds;
		uint8_t pictures;
		uint32_t flags;
	} mpeg2_gop_t;

	typedef struct mpeg2_picture_s {
		unsigned int temporal_reference;
		unsigned int nb_fields;
		uint32_t tag, tag2;
		uint32_t flags;
		struct {
			int x, y;
		} display_offset[3];
	} mpeg2_picture_t;

	typedef struct mpeg2_fbuf_s {
		uint8_t *buf[3];
		void *id;
	} mpeg2_fbuf_t;

	typedef enum {
		STATE_BUFFER = 0,
		STATE_SEQUENCE = 1,
		STATE_SEQUENCE_REPEATED = 2,
		STATE_GOP = 3,
		STATE_PICTURE = 4,
		STATE_SLICE_1ST = 5,
		STATE_PICTURE_2ND = 6,
		STATE_SLICE = 7,
		STATE_END = 8,
		STATE_INVALID = 9,
		STATE_INVALID_END = 10
	} mpeg2_state_t;

	typedef enum {
		MPEG2_ALLOC_MPEG2DEC = 0,
		MPEG2_ALLOC_CHUNK = 1,
		MPEG2_ALLOC_YUV = 2,
		MPEG2_ALLOC_CONVERT_ID = 3,
		MPEG2_ALLOC_CONVERTED = 4
	} mpeg2_alloc_t;

	typedef enum {
		MPEG2_CONVERT_SET = 0,
		MPEG2_CONVERT_STRIDE = 1,
		MPEG2_CONVERT_START = 2
	} mpeg2_convert_stage_t;

	typedef struct mpeg2_info_s {
		const mpeg2_sequence_t *sequence;
		const mpeg2_gop_t *gop;
		const mpeg2_picture_t *current_picture;
		const mpeg2_picture_t *current_picture_2nd;
		const mpeg2_fbuf_t *current_fbuf;
		const mpeg2_picture_t *display_picture;
		const mpeg2_picture_t *display_picture_2nd;
		const mpeg2_fbuf_t *display_fbuf;
		const mpeg2_fbuf_t *discard_fbuf;
		const uint8_t *user_data;
		unsigned int user_data_len;
	} mpeg2_info_t;

	typedef struct mpeg2dec_s mpeg2dec_t;
	typedef struct mpeg2_decoder_s mpeg2_decoder_t;

	typedef struct mpeg2_convert_init_s {
		unsigned int id_size;
		unsigned int buf_size[3];
		void (*start) (void *id, const mpeg2_fbuf_t * fbuf,
			       const mpeg2_picture_t * picture, const mpeg2_gop_t * gop);
		void (*copy) (void *id, uint8_t * const *src, unsigned int v_offset);
	} mpeg2_convert_init_t;

	typedef void mpeg2_mc_fct(uint8_t *, const uint8_t *, int, int);

	typedef void motion_parser_t(mpeg2_decoder_t * decoder, motion_t * motion, mpeg2_mc_fct * const *table);
	typedef int mpeg2_convert_t(int stage, void *id,
				    const mpeg2_sequence_t * sequence, int stride,
				    uint32_t accel, void *arg, mpeg2_convert_init_t * result);

	struct mpeg2_decoder_s {
		/* first, state that carries information from one macroblock to the */
		/* next inside a slice, and is never used outside of mpeg2_slice() */

		/* bit parsing stuff */
		uint32_t bitstream_buf;	/* current 32 bit working set */
		int bitstream_bits;	/* used bits in working set */
		const uint8_t *bitstream_ptr;	/* buffer with stream data */

		uint8_t *dest[3];

		int offset;
		int stride;
		int uv_stride;
		int slice_stride;
		int slice_uv_stride;
		int stride_frame;
		unsigned int limit_x;
		unsigned int limit_y_16;
		unsigned int limit_y_8;
		unsigned int limit_y;

		/* Motion vectors */
		/* The f_ and b_ correspond to the forward and backward motion */
		/* predictors */
		motion_t b_motion;
		motion_t f_motion;
		motion_parser_t *motion_parser[5];

		/* predictor for DC coefficients in intra blocks */
		int16_t dc_dct_pred[3];

		/* DCT coefficients */
		int16_t DCTblock[64];

		uint8_t *picture_dest[3];
		void (*convert) (void *convert_id, uint8_t * const *src, unsigned int v_offset);
		void *convert_id;

		int dmv_offset;
		unsigned int v_offset;

		/* now non-slice-specific information */

		/* sequence header stuff */
		uint16_t *quantizer_matrix[4];
		 uint16_t(*chroma_quantizer[2])[64];
		uint16_t quantizer_prescale[4][32][64];

		/* The width and height of the picture snapped to macroblock units */
		int width;
		int height;
		int vertical_position_extension;
		int chroma_format;

		/* picture header stuff */

		/* what type of picture this is (I, P, B, D) */
		int coding_type;

		/* picture coding extension stuff */

		/* quantization factor for intra dc coefficients */
		int intra_dc_precision;
		/* top/bottom/both fields */
		int picture_structure;
		/* bool to indicate all predictions are frame based */
		int frame_pred_frame_dct;
		/* bool to indicate whether intra blocks have motion vectors */
		/* (for concealment) */
		int concealment_motion_vectors;
		/* bool to use different vlc tables */
		int intra_vlc_format;
		/* used for DMV MC */
		int top_field_first;

		/* stuff derived from bitstream */

		/* pointer to the zigzag scan we're supposed to be using */
		const uint8_t *scan;

		int second_field;

		int mpeg1;

		/* for MPlayer: */
		int quantizer_scales[32];
		int quantizer_scale;
		char *quant_store;
		int quant_stride;
	};

	typedef struct {
		mpeg2_fbuf_t fbuf;
	} fbuf_alloc_t;

	struct mpeg2dec_s {
		mpeg2_decoder_t decoder;

		mpeg2_info_t info;

		uint32_t shift;
		int is_display_initialized;
		 mpeg2_state_t(*action) (struct mpeg2dec_s * mpeg2dec);
		mpeg2_state_t state;
		uint32_t ext_state;

		/* allocated in init - gcc has problems allocating such big structures */
		uint8_t *chunk_buffer;
		/* pointer to start of the current chunk */
		uint8_t *chunk_start;
		/* pointer to current position in chunk_buffer */
		uint8_t *chunk_ptr;
		/* last start code ? */
		uint8_t code;

		/* picture tags */
		uint32_t tag_current, tag2_current, tag_previous, tag2_previous;
		int num_tags;
		int bytes_since_tag;

		int first;
		int alloc_index_user;
		int alloc_index;
		uint8_t first_decode_slice;
		uint8_t nb_decode_slices;

		unsigned int user_data_len;

		mpeg2_sequence_t new_sequence;
		mpeg2_sequence_t sequence;
		mpeg2_gop_t new_gop;
		mpeg2_gop_t gop;
		mpeg2_picture_t new_picture;
		mpeg2_picture_t pictures[4];
		mpeg2_picture_t *picture;
							/*const */ mpeg2_fbuf_t *fbuf[3];
							/* 0: current fbuf, 1-2: prediction fbufs */

		fbuf_alloc_t fbuf_alloc[3];
		int custom_fbuf;

		uint8_t *yuv_buf[3][3];
		int yuv_index;
		mpeg2_convert_t *convert;
		void *convert_arg;
		unsigned int convert_id_size;
		int convert_stride;
		void (*convert_start) (void *id, const mpeg2_fbuf_t * fbuf,
				       const mpeg2_picture_t * picture, const mpeg2_gop_t * gop);

		uint8_t *buf_start;
		uint8_t *buf_end;

		int16_t display_offset_x, display_offset_y;

		int copy_matrix;
		int8_t q_scale_type, scaled[4];
		uint8_t quantizer_matrix[4][64];
		uint8_t new_quantizer_matrix[4][64];

		/* for MPlayer: */
		unsigned char *pending_buffer;
		int pending_length;
	};

	typedef struct {
		uint8_t delta;
		uint8_t len;
	} MVtab;

	typedef struct {
		int8_t dmv;
		uint8_t len;
	} DMVtab;

	typedef struct {
		uint8_t mba;
		uint8_t len;
	} MBAtab;

	typedef struct {
		uint8_t modes;
		uint8_t len;
	} MBtab;

	typedef struct {
		uint8_t size;
		uint8_t len;
	} DCtab;

	typedef struct {
		uint8_t run;
		uint8_t level;
		uint8_t len;
	} DCTtab;

	typedef struct {
		uint8_t cbp;
		uint8_t len;
	} CBPtab;

	typedef struct {
		mpeg2_mc_fct *put[8];
		mpeg2_mc_fct *avg[8];
	} mpeg2_mc_t;

	typedef struct {
		mpeg2dec_t *mpeg2dec;
		int quant_store_idx;
		char *quant_store[3];
	} vd_libmpeg2_ctx_t;

	typedef struct vf_image_context_s {
		GxImage *static_images[2];
		GxImage *temp_images[1];
		GxImage *export_images[1];
		int static_idx;
	} vf_image_context_t;

#define ALT_BITSTREAM_READER
#define ALT_BITSTREAM_READER_LE

/* bit input */
/* buffer, buffer_end and size_in_bits must be present and used by every reader */
	typedef struct GetBitContext {
		const uint8_t *buffer, *buffer_end;
#ifdef ALT_BITSTREAM_READER
		int index;
#elif defined LIBMPEG2_BITSTREAM_READER
		uint8_t *buffer_ptr;
		uint32_t cache;
		int bit_count;
#elif defined A32_BITSTREAM_READER
		uint32_t *buffer_ptr;
		uint32_t cache0;
		uint32_t cache1;
		int bit_count;
#endif
		int size_in_bits;
	} GetBitContext;

	typedef struct {
#ifdef ARCH_PPC
		uint8_t regv[12 * 16];
#endif
		int dummy;
	} cpu_state_t;

#if defined __GNUC__
# define GNUC_PREREQ(maj, min) \
        ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
# define GNUC_PREREQ(maj, min) 0
#endif

/*#if GNUC_PREREQ(2,96)
# define likely(x)      __builtin_expect((x) != 0, 1)
# define unlikely(x)    __builtin_expect((x) != 0, 0)
#else*/
# define likely(x)      (x)
# define unlikely(x)    (x)
//#endif

#define MPEG2_ACCEL_X86_MMX 1
#define MPEG2_ACCEL_X86_3DNOW 2
#define MPEG2_ACCEL_X86_MMXEXT 4
#define MPEG2_ACCEL_X86_SSE2 8
#define MPEG2_ACCEL_PPC_ALTIVEC 1
#define MPEG2_ACCEL_ALPHA 1
#define MPEG2_ACCEL_ALPHA_MVI 2
#define MPEG2_ACCEL_SPARC_VIS 1
#define MPEG2_ACCEL_SPARC_VIS2 2
#define MPEG2_ACCEL_DETECT 0x80000000

#define I_TYPE 1
#define P_TYPE 2
#define B_TYPE 3
#define D_TYPE 4

#define PIC_CODING_EXT 0x100

#define PIC_MASK_CODING_TYPE 7
#define PIC_FLAG_CODING_TYPE_I 1
#define PIC_FLAG_CODING_TYPE_P 2
#define PIC_FLAG_CODING_TYPE_B 3
#define PIC_FLAG_CODING_TYPE_D 4

/* picture structure */
#define TOP_FIELD 1
#define BOTTOM_FIELD 2
#define FRAME_PICTURE 3

#define MPEG2_VERSION(a,b,c) (((a)<<16)|((b)<<8)|(c))
#define MPEG2_RELEASE MPEG2_VERSION (0, 4, 1)	/* 0.4.1 */

#define SEQ_FLAG_MPEG2 1
#define SEQ_FLAG_CONSTRAINED_PARAMETERS 2
#define SEQ_FLAG_PROGRESSIVE_SEQUENCE 4
#define SEQ_FLAG_LOW_DELAY 8
#define SEQ_FLAG_COLOUR_DESCRIPTION 16

#define SEQ_MASK_VIDEO_FORMAT 0xe0
#define SEQ_VIDEO_FORMAT_COMPONENT 0
#define SEQ_VIDEO_FORMAT_PAL 0x20
#define SEQ_VIDEO_FORMAT_NTSC 0x40
#define SEQ_VIDEO_FORMAT_SECAM 0x60
#define SEQ_VIDEO_FORMAT_MAC 0x80
#define SEQ_VIDEO_FORMAT_UNSPECIFIED 0xa0

#define SEQ_EXT 2
#define SEQ_DISPLAY_EXT 4
#define QUANT_MATRIX_EXT 8
#define COPYRIGHT_EXT 0x10
#define PIC_DISPLAY_EXT 0x80
#define PIC_CODING_EXT 0x100

#define PIC_MASK_CODING_TYPE 7
#define PIC_FLAG_CODING_TYPE_I 1
#define PIC_FLAG_CODING_TYPE_P 2
#define PIC_FLAG_CODING_TYPE_B 3
#define PIC_FLAG_CODING_TYPE_D 4

#define PIC_FLAG_TOP_FIELD_FIRST 8
#define PIC_FLAG_PROGRESSIVE_FRAME 16
#define PIC_FLAG_COMPOSITE_DISPLAY 32
#define PIC_FLAG_SKIP 64
#define PIC_FLAG_TAGS 128
#define PIC_FLAG_REPEAT_FIRST_FIELD 256
#define PIC_MASK_COMPOSITE_DISPLAY 0xfffff000

#define BUFFER_SIZE (1194 * 1024)

//  clear it in get_image() if draw_slice() not implemented]
#define MP_IMGFLAG_DRAW_CALLBACK 0x1000

// I+P+B type, requires 2+ independent static R/W and 1+ temp WO buffers
#define MP_IMGTYPE_IPB 4

//--------- codec's requirements (filled by the codec/vf) ---------

//--- buffer content restrictions:
// set if buffer content shouldn't be modified:
#define MP_IMGFLAG_PRESERVE 0x01
// set if buffer content will be READ for next frame's MC: (I/P mpeg frames)
#define MP_IMGFLAG_READABLE 0x02

//--- buffer width/stride/plane restrictions: (used for direct rendering)
// stride _have_to_ be aligned to MB boundary:  [for DR restrictions]
#define MP_IMGFLAG_ACCEPT_ALIGNED_STRIDE 0x4
// stride should be aligned to MB boundary:     [for buffer allocation]
#define MP_IMGFLAG_PREFER_ALIGNED_STRIDE 0x8
// codec accept any stride (>=width):
#define MP_IMGFLAG_ACCEPT_STRIDE 0x10
// codec accept any width (width*bpp=stride -> stride%bpp==0) (>=width):
#define MP_IMGFLAG_ACCEPT_WIDTH 0x20
//--- for planar formats only:
// uses only stride[0], and stride[1]=stride[2]=stride[0]>>mpi->chroma_x_shift
#define MP_IMGFLAG_COMMON_STRIDE 0x40
// uses only planes[0], and calculates planes[1,2] from width,height,imgfmt
#define MP_IMGFLAG_COMMON_PLANE 0x80

#define MP_IMGFLAGMASK_RESTRICTIONS 0xFF

//--------- color info (filled by mp_image_setfmt() ) -----------
// set if number of planes > 1
#define MP_IMGFLAG_PLANAR 0x100
// set if it's YUV colorspace
#define MP_IMGFLAG_YUV 0x200
// set if it's swapped (BGR or YVU) plane/byteorder
#define MP_IMGFLAG_SWAPPED 0x400
// using palette for RGB data
#define MP_IMGFLAG_RGB_PALETTE 0x800

#define MP_IMGFLAGMASK_COLORS 0xF00

// codec uses drawing/rendering callbacks (draw_slice()-like thing, DR method 2)
// [the codec will set this flag if it supports callbacks, and the vo _may_
//  clear it in get_image() if draw_slice() not implemented]
#define MP_IMGFLAG_DRAW_CALLBACK 0x1000
// set if it's in video buffer/memory: [set by vo/vf's get_image() !!!]
#define MP_IMGFLAG_DIRECT 0x2000
// set if buffer is allocated (used in destination images):
#define MP_IMGFLAG_ALLOCATED 0x4000

// buffer type was printed (do NOT set this flag - it's for INTERNAL USE!!!)
#define MP_IMGFLAG_TYPE_DISPLAYED 0x8000

// codec doesn't support any form of direct rendering - it has own buffer
// allocation. so we just export its buffer pointers:
#define MP_IMGTYPE_EXPORT 0
// codec requires a static WO buffer, but it does only partial updates later:
#define MP_IMGTYPE_STATIC 1
// codec just needs some WO memory, where it writes/copies the whole frame to:
#define MP_IMGTYPE_TEMP 2
// I+P type, requires 2+ independent static R/W buffers
#define MP_IMGTYPE_IP 3

#define IMGFMT_YUY2 0x32595559

/* Compressed Formats */
#define IMGFMT_MPEGPES (('M'<<24)|('P'<<16)|('E'<<8)|('S'))
#define IMGFMT_MJPEG (('M')|('J'<<8)|('P'<<16)|('G'<<24))
/* Formats that are understood by zoran chips, we include
 * non-interlaced, interlaced top-first, interlaced bottom-first */
#define IMGFMT_ZRMJPEGNI  (('Z'<<24)|('R'<<16)|('N'<<8)|('I'))
#define IMGFMT_ZRMJPEGIT (('Z'<<24)|('R'<<16)|('I'<<8)|('T'))
#define IMGFMT_ZRMJPEGIB (('Z'<<24)|('R'<<16)|('I'<<8)|('B'))

#define IMGFMT_RGB_MASK 0xFFFFFF00
#define IMGFMT_RGB (('R'<<24)|('G'<<16)|('B'<<8))
#define IMGFMT_RGB1  (IMGFMT_RGB|1)
#define IMGFMT_RGB4  (IMGFMT_RGB|4)
#define IMGFMT_RGB4_CHAR  (IMGFMT_RGB|4|128)	// RGB4 with 1 pixel per byte
#define IMGFMT_RGB8  (IMGFMT_RGB|8)
#define IMGFMT_RGB15 (IMGFMT_RGB|15)
#define IMGFMT_RGB16 (IMGFMT_RGB|16)
#define IMGFMT_RGB24 (IMGFMT_RGB|24)
#define IMGFMT_RGB32 (IMGFMT_RGB|32)

#define IMGFMT_BGR_MASK 0xFFFFFF00
#define IMGFMT_BGR (('B'<<24)|('G'<<16)|('R'<<8))
#define IMGFMT_BGR1 (IMGFMT_BGR|1)
#define IMGFMT_BGR4 (IMGFMT_BGR|4)
#define IMGFMT_BGR4_CHAR (IMGFMT_BGR|4|128)	// BGR4 with 1 pixel per byte
#define IMGFMT_BGR8 (IMGFMT_BGR|8)
#define IMGFMT_BGR15 (IMGFMT_BGR|15)
#define IMGFMT_BGR16 (IMGFMT_BGR|16)
#define IMGFMT_BGR24 (IMGFMT_BGR|24)
#define IMGFMT_BGR32 (IMGFMT_BGR|32)

#define IMGFMT_RG4B  IMGFMT_RGB4_CHAR
#define IMGFMT_BG4B  IMGFMT_BGR4_CHAR

#define IMGFMT_IS_RGB(fmt) (((fmt)&IMGFMT_RGB_MASK)==IMGFMT_RGB)
#define IMGFMT_IS_BGR(fmt) (((fmt)&IMGFMT_BGR_MASK)==IMGFMT_BGR)

#define IMGFMT_RGB_DEPTH(fmt) ((fmt)&0x3F)
#define IMGFMT_BGR_DEPTH(fmt) ((fmt)&0x3F)

// I think that this code could not be used by any other codec/format
#define IMGFMT_XVMC 0x1DC70000
#define IMGFMT_XVMC_MASK 0xFFFF0000
#define IMGFMT_IS_XVMC(fmt) (((fmt)&IMGFMT_XVMC_MASK)==IMGFMT_XVMC)
//these are chroma420
#define IMGFMT_XVMC_MOCO_MPEG2 (IMGFMT_XVMC|0x02)
#define IMGFMT_XVMC_IDCT_MPEG2 (IMGFMT_XVMC|0x82)

// vf filter: accepts stride (put_image)
// vo driver: has draw_slice() support for the given csp
#define VFCAP_ACCEPT_STRIDE 0x400

/* Planar YUV Formats */

#define IMGFMT_YVU9 0x39555659
#define IMGFMT_IF09 0x39304649
#define IMGFMT_YV12 0x32315659
#define IMGFMT_I420 0x30323449
#define IMGFMT_IYUV 0x56555949
#define IMGFMT_CLPL 0x4C504C43
#define IMGFMT_Y800 0x30303859
#define IMGFMT_Y8   0x20203859
#define IMGFMT_NV12 0x3231564E
#define IMGFMT_NV21 0x3132564E

/* unofficial Planar Formats, FIXME if official 4CC exists */
#define IMGFMT_444P 0x50343434
#define IMGFMT_422P 0x50323234
#define IMGFMT_411P 0x50313134
#define IMGFMT_HM12 0x32314D48

#define IMGFMT_UYVY 0x59565955

#define MP_IMGFIELD_ORDERED 0x01
#define MP_IMGFIELD_TOP_FIRST 0x02
#define MP_IMGFIELD_REPEAT_FIRST 0x04
#define MP_IMGFIELD_TOP 0x08
#define MP_IMGFIELD_BOTTOM 0x10
#define MP_IMGFIELD_INTERLACED 0x20

#define MPEG12_POSTPROC

#define RECEIVED(code,state) (((state) << 8) + (code))

#define GETWORD(bit_buf,shift,bit_ptr)				\
do {								\
    bit_buf |= ((bit_ptr[0] << 8) | bit_ptr[1]) << (shift);	\
    bit_ptr += 2;						\
} while (0)

#define NEEDBITS(bit_buf,bits,bit_ptr)		\
do {						\
    if (unlikely (bits > 0)) {			\
	GETWORD (bit_buf, bits, bit_ptr);	\
	bits -= 16;				\
    }						\
} while (0)

/* remove num valid bits from bit_buf */
#define DUMPBITS(bit_buf,bits,num)	\
do {					\
    bit_buf <<= (num);			\
    bits += (num);			\
} while (0)

/* take num bits from the high part of bit_buf and zero extend them */
#define UBITS(bit_buf,num) (((uint32_t)(bit_buf)) >> (32 - (num)))

/* take num bits from the high part of bit_buf and sign extend them */
#define SBITS(bit_buf,num) (((int32_t)(bit_buf)) >> (32 - (num)))

#define MPEG2_MC_EXTERN(x) mpeg2_mc_t mpeg2_mc_##x = {			  \
    {MC_put_o_16_##x, MC_put_x_16_##x, MC_put_y_16_##x, MC_put_xy_16_##x, \
     MC_put_o_8_##x,  MC_put_x_8_##x,  MC_put_y_8_##x,  MC_put_xy_8_##x}, \
    {MC_avg_o_16_##x, MC_avg_x_16_##x, MC_avg_y_16_##x, MC_avg_xy_16_##x, \
     MC_avg_o_8_##x,  MC_avg_x_8_##x,  MC_avg_y_8_##x,  MC_avg_xy_8_##x}  \
};

	static const DMVtab DMV_2[] = {
		{0, 1}, {0, 1}, {1, 2}, {-1, 2}
	};

	static const MVtab MV_4[] = {
		{3, 6}, {2, 4}, {1, 3}, {1, 3}, {0, 2}, {0, 2}, {0, 2}, {0, 2}
	};

	static const MVtab MV_10[] = {
		{0, 10}, {0, 10}, {0, 10}, {0, 10}, {0, 10}, {0, 10}, {0, 10}, {0, 10},
		{0, 10}, {0, 10}, {0, 10}, {0, 10}, {15, 10}, {14, 10}, {13, 10}, {12, 10},
		{11, 10}, {10, 10}, {9, 9}, {9, 9}, {8, 9}, {8, 9}, {7, 9}, {7, 9},
		{6, 7}, {6, 7}, {6, 7}, {6, 7}, {6, 7}, {6, 7}, {6, 7}, {6, 7},
		{5, 7}, {5, 7}, {5, 7}, {5, 7}, {5, 7}, {5, 7}, {5, 7}, {5, 7},
		{4, 7}, {4, 7}, {4, 7}, {4, 7}, {4, 7}, {4, 7}, {4, 7}, {4, 7}
	};

	static const MBAtab MBA_5[] = {
		{6, 5}, {5, 5}, {4, 4}, {4, 4}, {3, 4}, {3, 4},
		{2, 3}, {2, 3}, {2, 3}, {2, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
		{0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
		{0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}
	};

	static const MBAtab MBA_11[] = {
		{32, 11}, {31, 11}, {30, 11}, {29, 11},
		{28, 11}, {27, 11}, {26, 11}, {25, 11},
		{24, 11}, {23, 11}, {22, 11}, {21, 11},
		{20, 10}, {20, 10}, {19, 10}, {19, 10},
		{18, 10}, {18, 10}, {17, 10}, {17, 10},
		{16, 10}, {16, 10}, {15, 10}, {15, 10},
		{14, 8}, {14, 8}, {14, 8}, {14, 8},
		{14, 8}, {14, 8}, {14, 8}, {14, 8},
		{13, 8}, {13, 8}, {13, 8}, {13, 8},
		{13, 8}, {13, 8}, {13, 8}, {13, 8},
		{12, 8}, {12, 8}, {12, 8}, {12, 8},
		{12, 8}, {12, 8}, {12, 8}, {12, 8},
		{11, 8}, {11, 8}, {11, 8}, {11, 8},
		{11, 8}, {11, 8}, {11, 8}, {11, 8},
		{10, 8}, {10, 8}, {10, 8}, {10, 8},
		{10, 8}, {10, 8}, {10, 8}, {10, 8},
		{9, 8}, {9, 8}, {9, 8}, {9, 8},
		{9, 8}, {9, 8}, {9, 8}, {9, 8},
		{8, 7}, {8, 7}, {8, 7}, {8, 7},
		{8, 7}, {8, 7}, {8, 7}, {8, 7},
		{8, 7}, {8, 7}, {8, 7}, {8, 7},
		{8, 7}, {8, 7}, {8, 7}, {8, 7},
		{7, 7}, {7, 7}, {7, 7}, {7, 7},
		{7, 7}, {7, 7}, {7, 7}, {7, 7},
		{7, 7}, {7, 7}, {7, 7}, {7, 7},
		{7, 7}, {7, 7}, {7, 7}, {7, 7}
	};

/* macroblock modes */
#define MACROBLOCK_INTRA 1
#define MACROBLOCK_PATTERN 2
#define MACROBLOCK_MOTION_BACKWARD 4
#define MACROBLOCK_MOTION_FORWARD 8
#define MACROBLOCK_QUANT 16
#define DCT_TYPE_INTERLACED 32

#define INTRA MACROBLOCK_INTRA
#define QUANT MACROBLOCK_QUANT

	static const MBtab MB_I[] = {
		{INTRA | QUANT, 2}, {INTRA, 1}
	};

#define MC MACROBLOCK_MOTION_FORWARD
#define CODED MACROBLOCK_PATTERN

	static const MBtab MB_P[] = {
		{INTRA | QUANT, 6}, {CODED | QUANT, 5}, {MC | CODED | QUANT, 5}, {INTRA, 5},
		{MC, 3}, {MC, 3}, {MC, 3}, {MC, 3},
		{CODED, 2}, {CODED, 2}, {CODED, 2}, {CODED, 2},
		{CODED, 2}, {CODED, 2}, {CODED, 2}, {CODED, 2},
		{MC | CODED, 1}, {MC | CODED, 1}, {MC | CODED, 1}, {MC | CODED, 1},
		{MC | CODED, 1}, {MC | CODED, 1}, {MC | CODED, 1}, {MC | CODED, 1},
		{MC | CODED, 1}, {MC | CODED, 1}, {MC | CODED, 1}, {MC | CODED, 1},
		{MC | CODED, 1}, {MC | CODED, 1}, {MC | CODED, 1}, {MC | CODED, 1}
	};

#define FWD MACROBLOCK_MOTION_FORWARD
#define BWD MACROBLOCK_MOTION_BACKWARD
#define INTER MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD

	static const MBtab MB_B[] = {
		{0, 0}, {INTRA | QUANT, 6},
		{BWD | CODED | QUANT, 6}, {FWD | CODED | QUANT, 6},
		{INTER | CODED | QUANT, 5}, {INTER | CODED | QUANT, 5},
		{INTRA, 5}, {INTRA, 5},
		{FWD, 4}, {FWD, 4}, {FWD, 4}, {FWD, 4},
		{FWD | CODED, 4}, {FWD | CODED, 4}, {FWD | CODED, 4}, {FWD | CODED, 4},
		{BWD, 3}, {BWD, 3}, {BWD, 3}, {BWD, 3},
		{BWD, 3}, {BWD, 3}, {BWD, 3}, {BWD, 3},
		{BWD | CODED, 3}, {BWD | CODED, 3}, {BWD | CODED, 3}, {BWD | CODED, 3},
		{BWD | CODED, 3}, {BWD | CODED, 3}, {BWD | CODED, 3}, {BWD | CODED, 3},
		{INTER, 2}, {INTER, 2}, {INTER, 2}, {INTER, 2},
		{INTER, 2}, {INTER, 2}, {INTER, 2}, {INTER, 2},
		{INTER, 2}, {INTER, 2}, {INTER, 2}, {INTER, 2},
		{INTER, 2}, {INTER, 2}, {INTER, 2}, {INTER, 2},
		{INTER | CODED, 2}, {INTER | CODED, 2}, {INTER | CODED, 2}, {INTER | CODED, 2},
		{INTER | CODED, 2}, {INTER | CODED, 2}, {INTER | CODED, 2}, {INTER | CODED, 2},
		{INTER | CODED, 2}, {INTER | CODED, 2}, {INTER | CODED, 2}, {INTER | CODED, 2},
		{INTER | CODED, 2}, {INTER | CODED, 2}, {INTER | CODED, 2}, {INTER | CODED, 2}
	};

	static const DCtab DC_lum_5[] = {
		{1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
		{2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
		{0, 3}, {0, 3}, {0, 3}, {0, 3}, {3, 3}, {3, 3}, {3, 3}, {3, 3},
		{4, 3}, {4, 3}, {4, 3}, {4, 3}, {5, 4}, {5, 4}, {6, 5}
	};

	static const DCtab DC_long[] = {
		{6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5},
		{6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5},
		{7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6},
		{8, 7}, {8, 7}, {8, 7}, {8, 7}, {9, 8}, {9, 8}, {10, 9}, {11, 9}
	};

	static const DCtab DC_chrom_5[] = {
		{0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2},
		{1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
		{2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
		{3, 3}, {3, 3}, {3, 3}, {3, 3}, {4, 4}, {4, 4}, {5, 5}
	};

	static const DCTtab DCT_B14AC_5[] = {
		{1, 3, 5}, {5, 1, 5}, {4, 1, 5},
		{1, 2, 4}, {1, 2, 4}, {3, 1, 4}, {3, 1, 4},
		{2, 1, 3}, {2, 1, 3}, {2, 1, 3}, {2, 1, 3},
		{129, 0, 2}, {129, 0, 2}, {129, 0, 2}, {129, 0, 2},
		{129, 0, 2}, {129, 0, 2}, {129, 0, 2}, {129, 0, 2},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2}
	};

	static const DCTtab DCT_B14_8[] = {
		{65, 0, 6}, {65, 0, 6}, {65, 0, 6}, {65, 0, 6},
		{3, 2, 7}, {3, 2, 7}, {10, 1, 7}, {10, 1, 7},
		{1, 4, 7}, {1, 4, 7}, {9, 1, 7}, {9, 1, 7},
		{8, 1, 6}, {8, 1, 6}, {8, 1, 6}, {8, 1, 6},
		{7, 1, 6}, {7, 1, 6}, {7, 1, 6}, {7, 1, 6},
		{2, 2, 6}, {2, 2, 6}, {2, 2, 6}, {2, 2, 6},
		{6, 1, 6}, {6, 1, 6}, {6, 1, 6}, {6, 1, 6},
		{14, 1, 8}, {1, 6, 8}, {13, 1, 8}, {12, 1, 8},
		{4, 2, 8}, {2, 3, 8}, {1, 5, 8}, {11, 1, 8}
	};

	static const DCTtab DCT_B14_10[] = {
		{17, 1, 10}, {6, 2, 10}, {1, 7, 10}, {3, 3, 10},
		{2, 4, 10}, {16, 1, 10}, {15, 1, 10}, {5, 2, 10}
	};

	static const DCTtab DCT_13[] = {
		{11, 2, 13}, {10, 2, 13}, {6, 3, 13}, {4, 4, 13},
		{3, 5, 13}, {2, 7, 13}, {2, 6, 13}, {1, 15, 13},
		{1, 14, 13}, {1, 13, 13}, {1, 12, 13}, {27, 1, 13},
		{26, 1, 13}, {25, 1, 13}, {24, 1, 13}, {23, 1, 13},
		{1, 11, 12}, {1, 11, 12}, {9, 2, 12}, {9, 2, 12},
		{5, 3, 12}, {5, 3, 12}, {1, 10, 12}, {1, 10, 12},
		{3, 4, 12}, {3, 4, 12}, {8, 2, 12}, {8, 2, 12},
		{22, 1, 12}, {22, 1, 12}, {21, 1, 12}, {21, 1, 12},
		{1, 9, 12}, {1, 9, 12}, {20, 1, 12}, {20, 1, 12},
		{19, 1, 12}, {19, 1, 12}, {2, 5, 12}, {2, 5, 12},
		{4, 3, 12}, {4, 3, 12}, {1, 8, 12}, {1, 8, 12},
		{7, 2, 12}, {7, 2, 12}, {18, 1, 12}, {18, 1, 12}
	};

	static const DCTtab DCT_15[] = {
		{1, 40, 15}, {1, 39, 15}, {1, 38, 15}, {1, 37, 15},
		{1, 36, 15}, {1, 35, 15}, {1, 34, 15}, {1, 33, 15},
		{1, 32, 15}, {2, 14, 15}, {2, 13, 15}, {2, 12, 15},
		{2, 11, 15}, {2, 10, 15}, {2, 9, 15}, {2, 8, 15},
		{1, 31, 14}, {1, 31, 14}, {1, 30, 14}, {1, 30, 14},
		{1, 29, 14}, {1, 29, 14}, {1, 28, 14}, {1, 28, 14},
		{1, 27, 14}, {1, 27, 14}, {1, 26, 14}, {1, 26, 14},
		{1, 25, 14}, {1, 25, 14}, {1, 24, 14}, {1, 24, 14},
		{1, 23, 14}, {1, 23, 14}, {1, 22, 14}, {1, 22, 14},
		{1, 21, 14}, {1, 21, 14}, {1, 20, 14}, {1, 20, 14},
		{1, 19, 14}, {1, 19, 14}, {1, 18, 14}, {1, 18, 14},
		{1, 17, 14}, {1, 17, 14}, {1, 16, 14}, {1, 16, 14}
	};

	static const DCTtab DCT_16[] = {
		{129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
		{129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
		{129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
		{129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
		{2, 18, 0}, {2, 17, 0}, {2, 16, 0}, {2, 15, 0},
		{7, 3, 0}, {17, 2, 0}, {16, 2, 0}, {15, 2, 0},
		{14, 2, 0}, {13, 2, 0}, {12, 2, 0}, {32, 1, 0},
		{31, 1, 0}, {30, 1, 0}, {29, 1, 0}, {28, 1, 0}
	};

	static const DCTtab DCT_B15_10[] = {
		{6, 2, 9}, {6, 2, 9}, {15, 1, 9}, {15, 1, 9},
		{3, 4, 10}, {17, 1, 10}, {16, 1, 9}, {16, 1, 9}
	};

	static const CBPtab CBP_7[] = {
		{0x11, 7}, {0x12, 7}, {0x14, 7}, {0x18, 7},
		{0x21, 7}, {0x22, 7}, {0x24, 7}, {0x28, 7},
		{0x3f, 6}, {0x3f, 6}, {0x30, 6}, {0x30, 6},
		{0x09, 6}, {0x09, 6}, {0x06, 6}, {0x06, 6},
		{0x1f, 5}, {0x1f, 5}, {0x1f, 5}, {0x1f, 5},
		{0x10, 5}, {0x10, 5}, {0x10, 5}, {0x10, 5},
		{0x2f, 5}, {0x2f, 5}, {0x2f, 5}, {0x2f, 5},
		{0x20, 5}, {0x20, 5}, {0x20, 5}, {0x20, 5},
		{0x07, 5}, {0x07, 5}, {0x07, 5}, {0x07, 5},
		{0x0b, 5}, {0x0b, 5}, {0x0b, 5}, {0x0b, 5},
		{0x0d, 5}, {0x0d, 5}, {0x0d, 5}, {0x0d, 5},
		{0x0e, 5}, {0x0e, 5}, {0x0e, 5}, {0x0e, 5},
		{0x05, 5}, {0x05, 5}, {0x05, 5}, {0x05, 5},
		{0x0a, 5}, {0x0a, 5}, {0x0a, 5}, {0x0a, 5},
		{0x03, 5}, {0x03, 5}, {0x03, 5}, {0x03, 5},
		{0x0c, 5}, {0x0c, 5}, {0x0c, 5}, {0x0c, 5},
		{0x01, 4}, {0x01, 4}, {0x01, 4}, {0x01, 4},
		{0x01, 4}, {0x01, 4}, {0x01, 4}, {0x01, 4},
		{0x02, 4}, {0x02, 4}, {0x02, 4}, {0x02, 4},
		{0x02, 4}, {0x02, 4}, {0x02, 4}, {0x02, 4},
		{0x04, 4}, {0x04, 4}, {0x04, 4}, {0x04, 4},
		{0x04, 4}, {0x04, 4}, {0x04, 4}, {0x04, 4},
		{0x08, 4}, {0x08, 4}, {0x08, 4}, {0x08, 4},
		{0x08, 4}, {0x08, 4}, {0x08, 4}, {0x08, 4},
		{0x0f, 3}, {0x0f, 3}, {0x0f, 3}, {0x0f, 3},
		{0x0f, 3}, {0x0f, 3}, {0x0f, 3}, {0x0f, 3},
		{0x0f, 3}, {0x0f, 3}, {0x0f, 3}, {0x0f, 3},
		{0x0f, 3}, {0x0f, 3}, {0x0f, 3}, {0x0f, 3}
	};

	static const CBPtab CBP_9[] = {
		{0, 0}, {0x00, 9}, {0x39, 9}, {0x36, 9},
		{0x37, 9}, {0x3b, 9}, {0x3d, 9}, {0x3e, 9},
		{0x17, 8}, {0x17, 8}, {0x1b, 8}, {0x1b, 8},
		{0x1d, 8}, {0x1d, 8}, {0x1e, 8}, {0x1e, 8},
		{0x27, 8}, {0x27, 8}, {0x2b, 8}, {0x2b, 8},
		{0x2d, 8}, {0x2d, 8}, {0x2e, 8}, {0x2e, 8},
		{0x19, 8}, {0x19, 8}, {0x16, 8}, {0x16, 8},
		{0x29, 8}, {0x29, 8}, {0x26, 8}, {0x26, 8},
		{0x35, 8}, {0x35, 8}, {0x3a, 8}, {0x3a, 8},
		{0x33, 8}, {0x33, 8}, {0x3c, 8}, {0x3c, 8},
		{0x15, 8}, {0x15, 8}, {0x1a, 8}, {0x1a, 8},
		{0x13, 8}, {0x13, 8}, {0x1c, 8}, {0x1c, 8},
		{0x25, 8}, {0x25, 8}, {0x2a, 8}, {0x2a, 8},
		{0x23, 8}, {0x23, 8}, {0x2c, 8}, {0x2c, 8},
		{0x31, 8}, {0x31, 8}, {0x32, 8}, {0x32, 8},
		{0x34, 8}, {0x34, 8}, {0x38, 8}, {0x38, 8}
	};

	static const DCTtab DCT_B14DC_5[] = {
		{1, 3, 5}, {5, 1, 5}, {4, 1, 5},
		{1, 2, 4}, {1, 2, 4}, {3, 1, 4}, {3, 1, 4},
		{2, 1, 3}, {2, 1, 3}, {2, 1, 3}, {2, 1, 3},
		{1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1},
		{1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1},
		{1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1},
		{1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}
	};

	static const DCTtab DCT_B15_8[] = {
		{65, 0, 6}, {65, 0, 6}, {65, 0, 6}, {65, 0, 6},
		{8, 1, 7}, {8, 1, 7}, {9, 1, 7}, {9, 1, 7},
		{7, 1, 7}, {7, 1, 7}, {3, 2, 7}, {3, 2, 7},
		{1, 7, 6}, {1, 7, 6}, {1, 7, 6}, {1, 7, 6},
		{1, 6, 6}, {1, 6, 6}, {1, 6, 6}, {1, 6, 6},
		{5, 1, 6}, {5, 1, 6}, {5, 1, 6}, {5, 1, 6},
		{6, 1, 6}, {6, 1, 6}, {6, 1, 6}, {6, 1, 6},
		{2, 5, 8}, {12, 1, 8}, {1, 11, 8}, {1, 10, 8},
		{14, 1, 8}, {13, 1, 8}, {4, 2, 8}, {2, 4, 8},
		{3, 1, 5}, {3, 1, 5}, {3, 1, 5}, {3, 1, 5},
		{3, 1, 5}, {3, 1, 5}, {3, 1, 5}, {3, 1, 5},
		{2, 2, 5}, {2, 2, 5}, {2, 2, 5}, {2, 2, 5},
		{2, 2, 5}, {2, 2, 5}, {2, 2, 5}, {2, 2, 5},
		{4, 1, 5}, {4, 1, 5}, {4, 1, 5}, {4, 1, 5},
		{4, 1, 5}, {4, 1, 5}, {4, 1, 5}, {4, 1, 5},
		{2, 1, 3}, {2, 1, 3}, {2, 1, 3}, {2, 1, 3},
		{2, 1, 3}, {2, 1, 3}, {2, 1, 3}, {2, 1, 3},
		{2, 1, 3}, {2, 1, 3}, {2, 1, 3}, {2, 1, 3},
		{2, 1, 3}, {2, 1, 3}, {2, 1, 3}, {2, 1, 3},
		{2, 1, 3}, {2, 1, 3}, {2, 1, 3}, {2, 1, 3},
		{2, 1, 3}, {2, 1, 3}, {2, 1, 3}, {2, 1, 3},
		{2, 1, 3}, {2, 1, 3}, {2, 1, 3}, {2, 1, 3},
		{2, 1, 3}, {2, 1, 3}, {2, 1, 3}, {2, 1, 3},
		{129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
		{129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
		{129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
		{129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
		{1, 3, 4}, {1, 3, 4}, {1, 3, 4}, {1, 3, 4},
		{1, 3, 4}, {1, 3, 4}, {1, 3, 4}, {1, 3, 4},
		{1, 3, 4}, {1, 3, 4}, {1, 3, 4}, {1, 3, 4},
		{1, 3, 4}, {1, 3, 4}, {1, 3, 4}, {1, 3, 4},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
		{1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
		{1, 2, 3}, {1, 2, 3}, {1, 2, 3}, {1, 2, 3},
		{1, 2, 3}, {1, 2, 3}, {1, 2, 3}, {1, 2, 3},
		{1, 2, 3}, {1, 2, 3}, {1, 2, 3}, {1, 2, 3},
		{1, 2, 3}, {1, 2, 3}, {1, 2, 3}, {1, 2, 3},
		{1, 2, 3}, {1, 2, 3}, {1, 2, 3}, {1, 2, 3},
		{1, 2, 3}, {1, 2, 3}, {1, 2, 3}, {1, 2, 3},
		{1, 2, 3}, {1, 2, 3}, {1, 2, 3}, {1, 2, 3},
		{1, 2, 3}, {1, 2, 3}, {1, 2, 3}, {1, 2, 3},
		{1, 4, 5}, {1, 4, 5}, {1, 4, 5}, {1, 4, 5},
		{1, 4, 5}, {1, 4, 5}, {1, 4, 5}, {1, 4, 5},
		{1, 5, 5}, {1, 5, 5}, {1, 5, 5}, {1, 5, 5},
		{1, 5, 5}, {1, 5, 5}, {1, 5, 5}, {1, 5, 5},
		{10, 1, 7}, {10, 1, 7}, {2, 3, 7}, {2, 3, 7},
		{11, 1, 7}, {11, 1, 7}, {1, 8, 7}, {1, 8, 7},
		{1, 9, 7}, {1, 9, 7}, {1, 12, 8}, {1, 13, 8},
		{3, 3, 8}, {5, 2, 8}, {1, 14, 8}, {1, 15, 8}
	};


#endif
