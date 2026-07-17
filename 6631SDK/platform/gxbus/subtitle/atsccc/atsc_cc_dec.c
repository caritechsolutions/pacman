#include "gx_subtitle_atsccc.h"
#include "gxcore.h"
#include "libzvbi/libzvbi.h"
#include "atsc_cc_dec.h"
#include "gx_mem.h"

#define CC_UNIT_OFFSET (4)
#define CC_UNIT_LENGTH (16*1024)

#define MAX_VBI_CHAR_ROWS    16
#define MAX_VBI_CHAR_COLUMNS 48

/* EIA 608-B Section 4.1. */
#define VBI_CAPTION_CC1 1 /* primary synchronous caption service (F1) */
#define VBI_CAPTION_CC2 2 /* special non-synchronous use captions (F1) */
#define VBI_CAPTION_CC3 3 /* secondary synchronous caption service (F2) */
#define VBI_CAPTION_CC4 4 /* special non-synchronous use captions (F2) */

#define VBI_CAPTION_T1 5 /* first text service (F1) */
#define VBI_CAPTION_T2 6 /* second text service (F1) */
#define VBI_CAPTION_T3 7 /* third text service (F2) */
#define VBI_CAPTION_T4 8 /* fourth text service (F2) */


#define MAX_CC_CHANNELS    8
#define MAX_CC_SERVICE     6
#define UNKNOWN_CC_CHANNEL 0

/* 47 CFR 15.119 (d) Screen format. */
#define CC_FIRST_ROW    0
#define CC_LAST_ROW     14
#define CC_MAX_ROWS     16

#define CC_FIRST_COLUMN  1
#define CC_LAST_COLUMN   32
#define CC_MAX_COLUMNS   32

#define CC_ALL_ROWS_MASK ((1 << CC_MAX_ROWS) - 1)

#define VBI_TRANSLUCENT VBI_SEMI_TRANSPARENT

#define DTV_CC_MAX_ROWS      16
#define DTV_CC_MAX_COLUMNS   64

#undef MIN
#define MIN(x, y) ({         \
		__typeof__ (x) _x = (x); \
		__typeof__ (y) _y = (y); \
		(void)(&_x == &_y); /* warn if types do not match */ \
		/* return */ (_x < _y) ? _x : _y; \
		})

#undef MAX
#define MAX(x, y) ({         \
		__typeof__ (x) _x = (x); \
		__typeof__ (y) _y = (y); \
		(void)(&_x == &_y); /* warn if types do not match */ \
		/* return */ (_x > _y) ? _x : _y; \
		})

#define CLEAR(var) memset (&(var), 0, sizeof (var))

enum cc_type {
	NTSC_F1 = 0,
	NTSC_F2 = 1,
	DTVCC_DATA = 2,
	DTVCC_START = 3,
};

enum cc_attr {
	VBI_UNDERLINE = (1 << 0),
	VBI_ITALIC    = (1 << 2),
	VBI_FLASH     = (1 << 3)
};

/* EIA 608-B decoder. */
enum field_num {
	FIELD_1 = 0,
	FIELD_2,
	MAX_FIELDS
};

enum cc_mode {
	CC_MODE_UNKNOWN,
	CC_MODE_ROLL_UP,
	CC_MODE_POP_ON,
	CC_MODE_PAINT_ON,
	CC_MODE_TEXT
};

enum caption_format {
	FORMAT_PLAIN,
	FORMAT_VT100,
	FORMAT_NTSC_CC
};

/* CEA 708-C decoder. */
enum justify {
	JUSTIFY_LEFT = 0,
	JUSTIFY_RIGHT,
	JUSTIFY_CENTER,
	JUSTIFY_FULL
};

enum direction {
	DIR_LEFT_RIGHT = 0,
	DIR_RIGHT_LEFT,
	DIR_TOP_BOTTOM,
	DIR_BOTTOM_TOP
};

enum display_effect {
	DISPLAY_EFFECT_SNAP = 0,
	DISPLAY_EFFECT_FADE,
	DISPLAY_EFFECT_WIPE
};


enum opacity {
	OPACITY_SOLID = 0,
	OPACITY_FLASH,
	OPACITY_TRANSLUCENT,
	OPACITY_TRANSPARENT
};

enum edge {
	EDGE_NONE = 0,
	EDGE_RAISED,
	EDGE_DEPRESSED,
	EDGE_UNIFORM,
	EDGE_SHADOW_LEFT,
	EDGE_SHADOW_RIGHT
};

enum pen_size {
	PEN_SIZE_SMALL = 0,
	PEN_SIZE_STANDARD,
	PEN_SIZE_LARGE
};

enum font_style {
	FONT_STYLE_DEFAULT = 0,
	FONT_STYLE_MONO_SERIF,
	FONT_STYLE_PROP_SERIF,
	FONT_STYLE_MONO_SANS,
	FONT_STYLE_PROP_SANS,
	FONT_STYLE_CASUAL,
	FONT_STYLE_CURSIVE,
	FONT_STYLE_SMALL_CAPS
};

enum text_tag {
	TEXT_TAG_DIALOG = 0,
	TEXT_TAG_SOURCE_ID,
	TEXT_TAG_DEVICE,
	TEXT_TAG_DIALOG_2,
	TEXT_TAG_VOICEOVER,
	TEXT_TAG_AUDIBLE_TRANSL,
	TEXT_TAG_SUBTITLE_TRANSL,
	TEXT_TAG_VOICE_DESCR,
	TEXT_TAG_LYRICS,
	TEXT_TAG_EFFECT_DESCR,
	TEXT_TAG_SCORE_DESCR,
	TEXT_TAG_EXPLETIVE,
	TEXT_TAG_NOT_DISPLAYABLE = 15
};

enum offset {
	OFFSET_SUBSCRIPT = 0,
	OFFSET_NORMAL,
	OFFSET_SUPERSCRIPT
};


typedef uint8_t cc_dtvcc_window_map;
typedef uint8_t cc_dtvcc_color;

typedef struct {
	/**
	 * [0] and [1] are the displayed and non-displayed buffer as
	 * defined in 47 CFR 15.119, and selected by displayed_buffer
	 * below. [2] is a snapshot of the displayed buffer at the
	 * last stream event.
	 *
	 * XXX Text channels don't need buffer[2] and buffer[3], we're
	 * wasting memory.
	 */
	uint16_t buffer[3][CC_MAX_ROWS][1 + CC_MAX_COLUMNS];

	/**
	 * For buffer[0 ... 2], if bit 1 << row is set this row
	 * contains displayable characters, spacing or non-spacing
	 * attributes. (Special character 0x1139 "transparent space"
	 * is not a displayable character.) This information is
	 * intended to speed up copying, erasing and formatting.
	 */
	unsigned int dirty[3];

	/** Index of displayed buffer, 0 or 1. */
	unsigned int displayed_buffer;

	/**
	 * Cursor position: FIRST_ROW ... LAST_ROW and
	 * FIRST_COLUMN ... LAST_COLUMN.
	 */
	unsigned int curr_row;
	unsigned int curr_column;

	/**
	 * Text window height in CC_MODE_ROLL_UP. The first row of the
	 * window is curr_row - window_rows + 1, the last row is
	 * curr_row.
	 *
	 * Note: curr_row - window_rows + 1 may be < FIRST_ROW, this
	 * must be clipped before using window_rows:
	 *
	 * actual_rows = MIN (curr_row - FIRST_ROW + 1, window_rows);
	 *
	 * We won't do that at the RUx command because usually a PAC
	 * follows which may change curr_row.
	 */
	unsigned int window_rows;

	/* Most recently received PAC command. */
	unsigned int last_pac;

	/**
	 * This variable counts successive transmissions of the
	 * letters A to Z. It is reset to zero on reception of any
	 * letter a to z.
	 *
	 * Some stations do not transmit EIA 608-B extended characters
	 * and except for N with tilde the standard and special
	 * character sets contain only lower case accented
	 * characters. We force these characters to upper case if this
	 * variable indicates live caption, which is usually all upper
	 * case.
	 */
	unsigned int uppercase_predictor;

	/** Current caption mode or CC_MODE_UNKNOWN. */
	enum cc_mode mode;
} cc_channel;

typedef struct {
	/**
	 * Decoder state. We decode all channels in parallel, this way
	 * clients can switch between channels without data loss, or
	 * capture multiple channels with a single decoder instance.
	 *
	 * Also 47 CFR 15.119 and EIA 608-C require us to remember the
	 * cursor position on each channel.
	 */
	cc_channel channel[MAX_CC_CHANNELS];

	/**
	 * Current channel, switched by caption control codes. Can be
	 * one of @c VBI_CAPTION_CC1 ... @c VBI_CAPTION_CC4 or @c
	 * VBI_CAPTION_T1 ... @c VBI_CAPTION_T4 or @c
	 * UNKNOWN_CC_CHANNEL if no channel number was received yet.
	 */
	vbi_pgno curr_ch_num[MAX_FIELDS];

	/**
	 * Caption control codes (two bytes) may repeat once for error
	 * correction. -1 if no repeated control code can be expected.
	 */
	int expect_ctrl[MAX_FIELDS][2];

	/** Receiving XDS data, as opposed to caption / ITV data. */
	vbi_bool in_xds[MAX_FIELDS];

	/**
	 * Pointer into the channel[] array if a display update event
	 * shall be sent at the end of this iteration, %c NULL
	 * otherwise. Purpose is to suppress an event for the first of
	 * two displayable characters in a caption byte pair.
	 */
	cc_channel *event_pending;

	/**
	 * Remembers past parity errors: One bit for each call of
	 * cc_feed(), most recent result in lsb. The idea is to
	 * disable the decoder if we detect too many errors.
	 */
	unsigned int error_history;

	void  *cr;

	AtscccEvent ev;
	void        *priv;
} cc_decoder;

typedef struct {
	/* Test tap. */
	const char *option_cc_data_tap_file_name;

	/* For debugging. */
	int64_t last_pts;
} cc_data_decoder;

typedef struct {
	enum pen_size   pen_size;
	enum font_style font_style;
	enum offset     offset;
	vbi_bool        italics;
	vbi_bool        underline;

	enum edge       edge_type;

	cc_dtvcc_color  fg_color;
	enum opacity    fg_opacity;

	cc_dtvcc_color  bg_color;
	enum opacity    bg_opacity;

	cc_dtvcc_color  edge_color;
} cc_dtvcc_pen_style;

typedef struct {
	enum text_tag      text_tag;
	cc_dtvcc_pen_style style;
} cc_dtvcc_pen;

typedef struct {
	enum justify        justify;
	enum direction      print_direction;
	enum direction      scroll_direction;
	vbi_bool            wordwrap;

	enum display_effect display_effect;
	enum direction effect_direction;
	unsigned int   effect_speed; /* 1/10 sec */

	cc_dtvcc_color fill_color;
	enum opacity   fill_opacity;

	enum edge      border_type;
	cc_dtvcc_color border_color;
} cc_dtvcc_window_style;

typedef struct {
	/* EIA 708-C window state. */

	uint16_t buffer[DTV_CC_MAX_ROWS][DTV_CC_MAX_COLUMNS];

	vbi_bool visible;

	/* 0 = highest ... 7 = lowest. */
	unsigned int priority;
	unsigned int anchor_point;
	unsigned int anchor_horizontal;
	unsigned int anchor_vertical;
	vbi_bool     anchor_relative;

	unsigned int row_count;
	unsigned int column_count;

	vbi_bool row_lock;
	vbi_bool column_lock;

	unsigned int curr_row;
	unsigned int curr_column;

	cc_dtvcc_pen curr_pen;
	cc_dtvcc_pen pen[DTV_CC_MAX_ROWS][DTV_CC_MAX_COLUMNS];

	cc_dtvcc_window_style style;

	/* Our stuff. */
	/**
	 * If bit 1 << row is set we already sent a stream event for
	 * this row.
	 */
	unsigned int			streamed;
} cc_dtvcc_window;

typedef struct {
	/* Interpretation Layer. */

	cc_dtvcc_window  window[8];

	cc_dtvcc_window *curr_window;

	cc_dtvcc_window_map created;

	/* For debugging. */
	unsigned int error_line;

	/* Service Layer. */

	uint8_t service_data[128];
	unsigned int service_data_in;
} cc_dtvcc_service;


typedef struct {
	cc_dtvcc_service *service[MAX_CC_SERVICE];

	/* Packet Layer. */

	uint8_t      packet[128];
	unsigned int packet_size;

	/* Next expected DTVCC packet sequence_number. Only the two
	   most significant bits are valid. < 0 if no sequence_number
	   has been received yet. */
	int  next_sequence_number;

	void *cr;
	AtscccEvent ev;
	void        *priv;
} cc_dtvcc_decoder;

typedef enum {
	/**
	 * 47 CFR Section 15.119 requires caption decoders to roll
	 * caption smoothly: Nominally each character cell has a
	 * height of 13 field lines. When this flag is set the current
	 * caption should be displayed with a vertical offset of 12
	 * field lines, and after every 1001 / 30000 seconds the
	 * caption overlay should move up by one field line until the
	 * offset is zero. The roll rate should be no more than 0.433
	 * seconds/row for other character cell heights.
	 *
	 * The flag may be set again before the offset returned to
	 * zero. The caption overlay should jump to offset 12 in this
	 * case regardless.
	 */
	VBI_START_ROLLING	= (1 << 0)
} cc_page_flags;

typedef struct {
	/* Caption stream filter:
	   NTSC CC1 ... CC4	(1 << 0 ... 1 << 3),
	   NTSC T1 ... T4	(1 << 4 ... 1 << 7),
	   ATSC Service 1 ... 2	(1 << 8 ... 1 << 9). */
	unsigned int option_caption_mask;

	/* Output file name for caption stream. */
	const char *option_caption_file_name[10];

	/* Output file name for XDS data. */
	const char *option_xds_output_file_name;

	vbi_bool option_caption_timestamps;
	vbi_bool option_timestamps_rel;
	vbi_bool option_ucla_prefix;

	enum caption_format option_caption_format;

	/* old options */
	char usexds;
	char usecc;
	char usesen;
	char usewebtv;

	cc_data_decoder cd;
	cc_decoder cc;
	cc_dtvcc_decoder dtvcc;

	//XDSdecode
	unsigned int field;
	struct {
		char    packet[34];
		uint8_t length;
		int     print : 1;
	} info[2][8][25];

	char newinfo[2][8][25][34];
	char *infoptr;
	int  mode;
	int  type;
	char infochecksum;
	const char *xds_info_prefix;
	const char *xds_info_suffix;

	uint16_t *ucs_buffer;
	unsigned int ucs_buffer_length;
	unsigned int ucs_buffer_capacity;

	int minicut_min[10];
} cc_caption_recorder;

typedef struct {
	unsigned int  cc_len;
	unsigned char cc_data[CC_UNIT_OFFSET + CC_UNIT_LENGTH];
	cc_caption_recorder cr;
} cc_context;

static int debug_atsc_cc = 0;

static void atsc_cc_stream_event_if_changed(cc_decoder *cd, cc_channel *ch);
static void atsc_cc_stream_event(cc_decoder *cd,
		cc_channel *ch,
		unsigned int first_row,
		unsigned int last_row);
static void atsc_cc_format_row(cc_decoder *cd,
		struct vbi_char *cp,
		cc_channel      *ch,
		unsigned int    buffer,
		unsigned int    row,
		vbi_bool        to_upper,
		vbi_bool        padding);
static vbi_pgno atsc_cc_channel_num(cc_decoder * cd, cc_channel *ch);

static const char * const ratings[] = {
	"(NOT RATED)","TV-Y","TV-Y7","TV-G",
	"TV-PG","TV-14","TV-MA","(NOT RATED)"};

static const vbi_color cc_color_map [8] = {
	VBI_WHITE, VBI_GREEN, VBI_BLUE, VBI_CYAN,
	VBI_RED, VBI_YELLOW, VBI_MAGENTA, VBI_BLACK
};

#define CC_RGBA(r, g, b) \
	((((r) & 0xFF) << 0) | (((g) & 0xFF) << 8) \
	 | (((b) & 0xFF) << 16) | (0xFF << 24))

static const vbi_rgba cc_rgba_map[40] = {
	CC_RGBA(0x00, 0x00, 0x00), CC_RGBA(0xFF, 0x00, 0x00),
	CC_RGBA(0x00, 0xFF, 0x00), CC_RGBA(0xFF, 0xFF, 0x00),
	CC_RGBA(0x00, 0x00, 0xFF), CC_RGBA(0xFF, 0x00, 0xFF),
	CC_RGBA(0x00, 0xFF, 0xFF), CC_RGBA(0xFF, 0xFF, 0xFF),
	CC_RGBA(0x00, 0x00, 0x00), CC_RGBA(0x77, 0x00, 0x00),
	CC_RGBA(0x00, 0x77, 0x00), CC_RGBA(0x77, 0x77, 0x00),
	CC_RGBA(0x00, 0x00, 0x77), CC_RGBA(0x77, 0x00, 0x77),
	CC_RGBA(0x00, 0x77, 0x77), CC_RGBA(0x77, 0x77, 0x77),
	CC_RGBA(0xFF, 0x00, 0x55), CC_RGBA(0xFF, 0x77, 0x00),
	CC_RGBA(0x00, 0xFF, 0x77), CC_RGBA(0xFF, 0xFF, 0xBB),
	CC_RGBA(0x00, 0xCC, 0xAA), CC_RGBA(0x55, 0x00, 0x00),
	CC_RGBA(0x66, 0x55, 0x22), CC_RGBA(0xCC, 0x77, 0x77),
	CC_RGBA(0x33, 0x33, 0x33), CC_RGBA(0xFF, 0x77, 0x77),
	CC_RGBA(0x77, 0xFF, 0x77), CC_RGBA(0xFF, 0xFF, 0x77),
	CC_RGBA(0x77, 0x77, 0xFF), CC_RGBA(0xFF, 0x77, 0xFF),
	CC_RGBA(0x77, 0xFF, 0xFF), CC_RGBA(0xDD, 0xDD, 0xDD),

	/* Private colors */

	CC_RGBA(0x00, 0x00, 0x00), CC_RGBA(0xFF, 0xAA, 0x99),
	CC_RGBA(0x44, 0xEE, 0x00), CC_RGBA(0xFF, 0xDD, 0x00),
	CC_RGBA(0xFF, 0xAA, 0x99), CC_RGBA(0xFF, 0x00, 0xFF),
	CC_RGBA(0x00, 0xFF, 0xFF), CC_RGBA(0xEE, 0xEE, 0xEE)
};

static const int8_t cc_pac_row_map [16] = {
	/* 0 */ 10,			/* 0x1040 */
	/* 1 */ -1,			/* no function */
	/* 2 */ 0, 1, 2, 3,		/* 0x1140 ... 0x1260 */
	/* 6 */ 11, 12, 13, 14,		/* 0x1340 ... 0x1460 */
	/* 10 */ 4, 5, 6, 7, 8, 9	/* 0x1540 ... 0x1760 */
};

/* CEA 708-C Digital TV Closed Caption decoder. */
static const uint8_t dtvcc_c0_length [4] = {
	1, 1, 2, 3
};

static const uint8_t dtvcc_c1_length [32] = {
	/* 0x80 CW0 ... CW7 */ 1, 1, 1, 1,  1, 1, 1, 1,
	/* 0x88 CLW */ 2,
	/* 0x89 DSW */ 2,
	/* 0x8A HDW */ 2,
	/* 0x8B TGW */ 2,

	/* 0x8C DLW */ 2,
	/* 0x8D DLY */ 2,
	/* 0x8E DLC */ 1,
	/* 0x8F RST */ 1,

	/* 0x90 SPA */ 3,
	/* 0x91 SPC */ 4,
	/* 0x92 SPL */ 3,
	/* CEA 708-C Section 7.1.5.1: 0x93 ... 0x96 are
	   reserved one byte codes. */ 1, 1, 1, 1,
	/* 0x97 SWA */ 5,
	/* 0x98 DF0 ... DF7 */ 7, 7, 7, 7,  7, 7, 7, 7
};


static const uint16_t dtvcc_g2 [96] = {
	/* Note Unicode defines no transparent spaces. */
	0x0020, /* 0x1020 Transparent space */
	0x00A0, /* 0x1021 Non-breaking transparent space */

	0,      /* 0x1022 reserved */
	0,
	0,
	0x2026, /* 0x1025 Horizontal ellipsis */
	0,
	0,
	0,
	0,
	0x0160, /* 0x102A S with caron */
	0,
	0x0152, /* 0x102C Ligature OE */
	0,
	0,
	0,

	/* CEA 708-C Section 7.1.8: "The character (0x30) is a solid
	   block which fills the entire character position with the
	   text foreground color." */
	0x2588, /* 0x1030 Full block */

	0x2018, /* 0x1031 Left single quotation mark */
	0x2019, /* 0x1032 Right single quotation mark */
	0x201C, /* 0x1033 Left double quotation mark */
	0x201D, /* 0x1034 Right double quotation mark */
	0,
	0,
	0,
	0x2122, /* 0x1039 Trademark sign */
	0x0161, /* 0x103A s with caron */
	0,
	0x0153, /* 0x103C Ligature oe */
	0x2120, /* 0x103D Service mark */
	0,
	0x0178, /* 0x103F Y with diaeresis */

	/* Code points 0x1040 ... 0x106F reserved. */
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,

	0,      /* 0x1070 reserved */
	0,
	0,
	0,
	0,
	0,
	0x215B, /* 0x1076 1/8 */
	0x215C, /* 0x1077 3/8 */
	0x215D, /* 0x1078 5/8 */
	0x215E, /* 0x1079 7/8 */
	0x2502, /* 0x107A Box drawings vertical */
	0x2510, /* 0x107B Box drawings down and left */
	0x2514, /* 0x107C Box drawings up and right */
	0x2500, /* 0x107D Box drawings horizontal */
	0x2518, /* 0x107E Box drawings up and left */
	0x250C  /* 0x107F Box drawings down and right */
};

_vbi_inline void vbi_char_copy_attr(struct vbi_char *cp1,
		struct vbi_char *cp2,
		unsigned int attr)
{
	if (attr & VBI_UNDERLINE)
		cp1->underline = cp2->underline;
	if (attr & VBI_ITALIC)
		cp1->italic = cp2->italic;
	if (attr & VBI_FLASH)
		cp1->flash = cp2->flash;
}

_vbi_inline void vbi_char_clear_attr(struct vbi_char *cp, unsigned int attr)
{
	if (attr & VBI_UNDERLINE)
		cp->underline = 0;
	if (attr & VBI_ITALIC)
		cp->italic = 0;
	if (attr & VBI_FLASH)
		cp->flash = 0;
}

_vbi_inline void vbi_char_set_attr(struct vbi_char *cp, unsigned int attr)
{
	if (attr & VBI_UNDERLINE)
		cp->underline = 1;
	if (attr & VBI_ITALIC)
		cp->italic = 1;
	if (attr & VBI_FLASH)
		cp->flash = 1;
}

_vbi_inline unsigned int vbi_char_has_attr(struct vbi_char *cp, unsigned int attr)
{
	attr &= (VBI_UNDERLINE | VBI_ITALIC | VBI_FLASH);

	if (0 == cp->underline)
		attr &= ~VBI_UNDERLINE;
	if (0 == cp->italic)
		attr &= ~VBI_ITALIC;
	if (0 == cp->flash)
		attr &= ~VBI_FLASH;

	return attr;
}

_vbi_inline unsigned int vbi_char_xor_attr(struct vbi_char *cp1,
		struct vbi_char *cp2,
		unsigned int attr)
{
	attr &= (VBI_UNDERLINE | VBI_ITALIC | VBI_FLASH);

	if (0 == (cp1->underline ^ cp2->underline))
		attr &= ~VBI_UNDERLINE;
	if (0 == (cp1->italic ^ cp2->italic))
		attr &= ~VBI_ITALIC;
	if (0 == (cp1->flash ^ cp2->flash))
		attr &= ~VBI_FLASH;

	return attr;
}

static void cr_grow_buffer(cc_caption_recorder *cr, unsigned int n_chars)
{
	uint16_t *new_buffer;
	size_t min_size;
	size_t new_size;

	if (cr->ucs_buffer_length + n_chars <= cr->ucs_buffer_capacity)
		return;

	min_size = (cr->ucs_buffer_length + n_chars) * 2;
	min_size = MAX ((size_t) 64, min_size);
	new_size = MAX (min_size, (size_t) cr->ucs_buffer_capacity * 4);

	new_buffer = av_realloc(cr->ucs_buffer, new_size);
	if (NULL == new_buffer) {
		gxlogd("warning: %s %d, malloc fail\n", __FUNCTION__, __LINE__);
		return;
	}

	cr->ucs_buffer = new_buffer;
	cr->ucs_buffer_capacity = new_size / 2;
}

static void cr_putuc(cc_caption_recorder *cr, uint16_t uc)
{
	cr_grow_buffer (cr, 1);
	cr->ucs_buffer[cr->ucs_buffer_length++] = uc;
}

static void cr_puts(cc_caption_recorder *cr, const char *s)
{
	while (0 != *s)
		cr_putuc (cr, *s++);
}

static vbi_bool cr_put_attr(cc_caption_recorder *cr, vbi_char *prev, vbi_char curr)
{
	uint16_t *d;

	switch (cr->option_caption_format) {
	case FORMAT_PLAIN:
		return TRUE;

	case FORMAT_NTSC_CC:
		/* Same output as the [zvbi-]ntsc-cc app. */

		curr.opacity = VBI_OPAQUE;

		/* Use the default foreground and background color of
		   the terminal. */
		curr.foreground = -1;
		curr.background = -1;

		if (vbi_char_has_attr (&curr, VBI_ITALIC))
			curr.foreground = VBI_CYAN;

		vbi_char_clear_attr (&curr, VBI_ITALIC | VBI_FLASH);

		break;

	case FORMAT_VT100:
		break;
	}

	cr_grow_buffer (cr, 32);

	d = cr->ucs_buffer + cr->ucs_buffer_length;

	/* Control sequences based on ECMA-48,
http://www.ecma-international.org/ */

	/* SGR sequence */

	d[0] = 27; /* CSI */
	d[1] = '[';
	d += 2;

	switch (curr.opacity) {
	case VBI_TRANSPARENT_SPACE:
		vbi_char_clear_attr (&curr, -1);
		curr.foreground = -1;
		curr.background = -1;
		break;

	case VBI_TRANSPARENT_FULL:
		curr.background = -1;
		break;

	case VBI_SEMI_TRANSPARENT:
	case VBI_OPAQUE:
		break;
	}

	if ((prev->foreground != curr.foreground
				&& (uint8_t) -1 == curr.foreground)
			|| (prev->background != curr.background
				&& (uint8_t) -1 == curr.background)) {
		*d++ = ';'; /* "[0m;" reset */
		vbi_char_clear_attr (prev, -1);
		prev->foreground = -1;
		prev->background = -1;
	}

	if (vbi_char_xor_attr (prev, &curr, VBI_ITALIC)) {
		if (!vbi_char_has_attr (&curr, VBI_ITALIC))
			*d++ = '2'; /* off */
		d[0] = '3'; /* italic */
		d[1] = ';';
		d += 2;
	}

	if (vbi_char_xor_attr (prev, &curr, VBI_UNDERLINE)) {
		if (!vbi_char_has_attr (&curr, VBI_UNDERLINE))
			*d++ = '2'; /* off */
		d[0] = '4'; /* underline */
		d[1] = ';';
		d += 2;
	}

	if (vbi_char_xor_attr (prev, &curr, VBI_FLASH)) {
		if (!vbi_char_has_attr (&curr, VBI_FLASH))
			*d++ = '2'; /* steady */
		d[0] = '5'; /* slowly blinking */
		d[1] = ';';
		d += 2;
	}

	if (prev->foreground != curr.foreground) {
		d[0] = '3';
		d[1] = curr.foreground + '0';
		d[2] = ';';
		d += 3;
	}

	if (prev->background != curr.background) {
		d[0] = '4';
		d[1] = curr.background + '0';
		d[2] = ';';
		d += 3;
	}

	vbi_char_copy_attr (prev, &curr, -1);
	prev->foreground = curr.foreground;
	prev->background = curr.background;

	if ('[' == d[-1])
		d -= 2; /* no change, remove CSI */
	else
		d[-1] = 'm'; /* replace last semicolon */

	cr->ucs_buffer_length = d - cr->ucs_buffer;

	return TRUE;
}

static void cr_new_line(cc_caption_recorder *cr,
		vbi_pgno channel,
		enum cc_mode mode,
		const vbi_char *text,
		unsigned int length)
{
	if (channel >= 10 || length < 32)
		return;

	if (0 == (cr->option_caption_mask & (1 << (channel - 1))))
		return;

	cr->ucs_buffer_length = 0;

	if (cr->usesen) {
		unsigned int uc[3] = {0, 0, 0};
		unsigned int column;
		unsigned int end;
		unsigned int separator;

		end = length;

		/* Eat trailing spaces. */
		while (end > 0 && ' ' == text[end - 1].unicode)
			--end;

		uc[2] = ' ';
		separator = 0;

		for (column = 0; column < end + 1; ++column) {
			uc[0] = uc[1];
			uc[1] = uc[2];
			uc[2] = 0;
			if (column < end)
				uc[2] = text[column].unicode;

			if (0 == separator && ' ' == uc[1])
				continue;
			cr_putuc (cr, uc[1]);
			separator = ' ';

			switch (uc[1]) {
			case '"':
				if ('.' != uc[0] && '!' != uc[0]
						&& '?' != uc[0])
					continue;
				break;

			case '.':
				if ('.' == uc[0] || '.' == uc[2])
					continue;
				/* fall through */

			case '!':
			case '?':
				if ('"' == uc[2])
					continue;
				if (0 != uc[2] && ' ' != uc[2])
					continue;
				break;

			default:
				continue;
			}

			cr_putuc (cr, '\n');
			separator = 0;
		}

		if (0 != separator)
			cr_putuc (cr, separator);
	} else {
		vbi_char prev_char;
		unsigned int column;

		if (cr->option_ucla_prefix) {
			cr_puts (cr, "CC");
			cr_putuc (cr, '0' + channel);

			switch (mode) {
			case CC_MODE_UNKNOWN:
				/* Shouldn't happen. Fall through. */
			case CC_MODE_ROLL_UP:
				cr_puts (cr, "|RU2|");
				break;
			case CC_MODE_POP_ON:
				cr_puts (cr, "|POP|");
				break;
			case CC_MODE_PAINT_ON:
				cr_puts (cr, "|PAI|");
				break;
			case CC_MODE_TEXT:
				cr_puts (cr, "|TXT|");
				break;
			}
		}

		vbi_char_clear_attr (&prev_char, -1);
		prev_char.foreground = -1;
		prev_char.background = -1;

		for (column = 0; column < length; ++column) {
			cr_put_attr (cr, &prev_char, text[column]);
			cr_putuc (cr, text[column].unicode);
		}

		if (0 != vbi_char_has_attr (&prev_char, -1)
				|| (uint8_t) -1 != prev_char.foreground
				|| (uint8_t) -1 != prev_char.background) {
			static const char end_seq[] = {
				27, '[', 'm', '\n', 0
			};

			cr_puts (cr, end_seq);
		} else {
			cr_putuc (cr, '\n');
		}
	}
}

static void atsc_cc_display_event(cc_decoder *cd,
		cc_channel *ch,
		cc_page_flags flags)
{
	if (cd->ev) {
		unsigned int i = 0;
		vbi_page *page = av_mallocz(sizeof(vbi_page));
		ATSC_CC_DISPLAY disp;

		memcpy(page->color_map, cc_rgba_map, sizeof(cc_rgba_map));
		page->rows    = MAX_VBI_CHAR_ROWS;
		page->columns = MAX_VBI_CHAR_COLUMNS;
		page->pgno    = atsc_cc_channel_num (cd, ch);
		for (i = 0; i < CC_MAX_ROWS; i++) {
			vbi_char *cp = &page->text[i * MAX_VBI_CHAR_COLUMNS];
			atsc_cc_format_row(cd, cp, ch, ch->displayed_buffer, i, FALSE, FALSE);
			if (debug_atsc_cc) {
				unsigned char sbuf[CC_MAX_COLUMNS + 1];
				unsigned int j = 0;
				for (j = 0; j < CC_MAX_COLUMNS; j++)
					sbuf[j] = cp[j].unicode;
				sbuf[CC_MAX_COLUMNS] = '\0';
				gxlogi("%02d --> %s\n", i, sbuf);
			}
		}
		disp.xr = disp.xl = 12;
		disp.yt = disp.yb = 10;
		cd->ev(page, cd->priv, &disp);
		av_free(page);
	}

	return;
}

static void atsc_cc_format_row(cc_decoder *cd,
		struct vbi_char *cp,
		cc_channel      *ch,
		unsigned int    buffer,
		unsigned int    row,
		vbi_bool        to_upper,
		vbi_bool        padding)
{
	struct vbi_char ac;
	unsigned int i;

	cd = cd; /* unused */

	/* 47 CFR 15.119 (h)(1). EIA 608-B Section 6.4. */
	CLEAR (ac);
	ac.foreground = VBI_WHITE;
	ac.background = VBI_BLACK;

	/* Shortcut. */
	if (0 == (ch->dirty[buffer] & (1 << row))) {
		vbi_char *end;

		ac.unicode = 0x20;
		ac.opacity = VBI_TRANSPARENT_SPACE;

		end = cp + CC_MAX_COLUMNS;
		if (padding)
			end += 2;

		while (cp < end)
			*cp++ = ac;

		return;
	}

	if (padding) {
		ac.unicode = 0x20;
		ac.opacity = VBI_TRANSPARENT_SPACE;
		*cp++ = ac;
	}

	/* EIA 608-B Section 6.4. */
	ac.opacity = VBI_OPAQUE;

	for (i = CC_FIRST_COLUMN - 1; i <= CC_LAST_COLUMN; ++i) {
		unsigned int color;
		unsigned int c;

		ac.unicode = 0x20;

		c = ch->buffer[buffer][row][i];
		if (0 == c) {
			if (padding
					&& VBI_TRANSPARENT_SPACE != cp[-1].opacity
					&& 0x20 != cp[-1].unicode) {
				/* Append a space with the same colors
				   and opacity (opaque or
				   transp. backgr.) as the text to the
				   left of it. */
				*cp++ = ac;
				/* We don't underline spaces, see
				   below. */
				vbi_char_clear_attr (cp - 1, -1);
			} else if (i > 0) {
				*cp++ = ac;
				cp[-1].opacity = VBI_TRANSPARENT_SPACE;
			}

			continue;
		} else if (c < 0x1020) {
			if (padding
					&& VBI_TRANSPARENT_SPACE == cp[-1].opacity) {
				/* Prepend a space with the same
				   colors and opacity (opaque or
				   transp. backgr.) as the text to the
				   right of it. */
				cp[-1] = ac;
				/* We don't underline spaces, see
				   below. */
				vbi_char_clear_attr (cp - 1, -1);
			}

			if ((c >= 'a' && c <= 'z')
					|| 0x7E == c /* n with tilde */) {
				/* We do not force these characters to
				   upper case because the standard
				   character set includes upper case
				   versions of these characters and
				   lower case was probably
				   deliberately transmitted. */
				ac.unicode = vbi_caption_unicode
					(c, /* to_upper */ FALSE);
			} else {
				ac.unicode = vbi_caption_unicode
					(c, to_upper);
			}
		} else if (c < 0x1040) {
			unsigned int color;

			/* Backgr. Attr. Codes -- 001 c000  010 xxxt */
			/* EIA 608-B Section 6.2. */

			/* This is a set-at spacing attribute. */

			color = (c >> 1) & 7;
			ac.background = cc_color_map[color];

			if (c & 0x0001) {
				ac.opacity = VBI_SEMI_TRANSPARENT;
			} else {
				ac.opacity = VBI_OPAQUE;
			}
		} else if (c < 0x1120) {
			/* Preamble Address Codes -- 001 crrr  1ri xxxu */

			/* PAC is a non-spacing attribute and only
			   stored in the buffer at the addressed
			   column minus one if it replaces a
			   transparent space (EIA 608-B Annex C.7,
			   C.14). There's always a transparent space
			   to the left of the first column but we show
			   this zeroth column only if padding is
			   enabled. */
			if (padding
					&& VBI_TRANSPARENT_SPACE != cp[-1].opacity
					&& 0x20 != cp[-1].unicode) {
				/* See 0 == c. */
				*cp++ = ac;
				vbi_char_clear_attr (cp - 1, -1);
			} else if (i > 0) {
				*cp++ = ac;
				cp[-1].opacity = VBI_TRANSPARENT_SPACE;
			}

			vbi_char_clear_attr (&ac, VBI_UNDERLINE | VBI_ITALIC);
			if (c & 0x0001)
				vbi_char_set_attr (&ac, VBI_UNDERLINE);
			if (c & 0x0010) {
				ac.foreground = VBI_WHITE;
			} else {
				color = (c >> 1) & 7;
				if (7 == color) {
					ac.foreground = VBI_WHITE;
					vbi_char_set_attr (&ac, VBI_ITALIC);
				} else {
					ac.foreground = cc_color_map[color];
				}
			}

			continue;
		} else if (c < 0x1130) {
			/* Mid-Row Codes -- 001 c001  010 xxxu */
			/* 47 CFR 15.119 Mid-Row Codes table,
			   (h)(1)(ii), (h)(1)(iii). */

			/* 47 CFR 15.119 (h)(1)(i), EIA 608-B Section
			   6.2: Mid-Row codes, FON, BT, FA and FAU are
			   set-at spacing attributes. */

			vbi_char_clear_attr (&ac, -1);
			if (c & 0x0001)
				vbi_char_set_attr (&ac, VBI_UNDERLINE);
			color = (c >> 1) & 7;
			if (7 == color) {
				vbi_char_set_attr (&ac, VBI_ITALIC);
			} else {
				ac.foreground = cc_color_map[color];
			}
		} else if (c < 0x1220) {
			/* Special Characters -- 001 c001  011 xxxx */
			/* 47 CFR 15.119 Character Set Table. */

			if (padding
					&& VBI_TRANSPARENT_SPACE == cp[-1].opacity) {
				cp[-1] = ac;
				vbi_char_clear_attr (cp - 1, -1);
			}

			/* Note we already stored 0 instead of 0x1139
			   (transparent space) in the ch->buffer. */
			ac.unicode = vbi_caption_unicode (c, to_upper);
		} else if (c < 0x1428) {
			/* Extended Character Set -- 001 c01x  01x xxxx */
			/* EIA 608-B Section 6.4.2 */

			if (padding
					&& VBI_TRANSPARENT_SPACE == cp[-1].opacity) {
				cp[-1] = ac;
				vbi_char_clear_attr (cp - 1, -1);
			}

			/* We do not force these characters to upper
			   case because the extended character set
			   includes upper case versions of all letters
			   and lower case was probably deliberately
			   transmitted. */
			ac.unicode = vbi_caption_unicode
				(c, /* to_upper */ FALSE);
		} else if (c < 0x172D) {
			/* FON Flash On -- 001 c10f  010 1000 */
			/* 47 CFR 15.119 (h)(1)(iii). */

			vbi_char_set_attr (&ac, VBI_FLASH);
		} else if (c < 0x172E) {
			/* BT Background Transparent -- 001 c111  010 1101 */
			/* EIA 608-B Section 6.4. */

			ac.opacity = VBI_TRANSPARENT_FULL;
		} else if (c <= 0x172F) {
			/* FA Foreground Black -- 001 c111  010 111u */
			/* EIA 608-B Section 6.4. */

			if (c & 0x0001)
				vbi_char_set_attr (&ac, VBI_UNDERLINE);
			ac.foreground = VBI_BLACK;
		}

		*cp++ = ac;

		/* 47 CFR 15.119 and EIA 608-B are silent about
		   underlined spaces, but considering the example in
		   47 CFR (h)(1)(iv) which would produce something
		   ugly like "__text" I suppose we should not
		   underline them. For good measure we also clear the
		   invisible italic and flash attribute. */
		if (0x20 == ac.unicode)
			vbi_char_clear_attr (cp - 1, -1);
	}

	if (padding) {
		ac.unicode = 0x20;
		vbi_char_clear_attr (&ac, -1);

		if (VBI_TRANSPARENT_SPACE != cp[-1].opacity
				&& 0x20 != cp[-1].unicode) {
			*cp = ac;
		} else {
			ac.opacity = VBI_TRANSPARENT_SPACE;
			*cp = ac;
		}
	}
}

static void atsc_cc_put_char(cc_decoder *cd,
		cc_channel *ch,
		int c,
		vbi_bool displayable,
		vbi_bool backspace)
{
	uint16_t *text;
	unsigned int curr_buffer;
	unsigned int row;
	unsigned int column;

	/* 47 CFR Section 15.119 (f)(1), (f)(2), (f)(3). */
	curr_buffer = ch->displayed_buffer
		^ (CC_MODE_POP_ON == ch->mode);

	row = ch->curr_row;
	column = ch->curr_column;

	if (backspace) {
		/* 47 CFR 15.119 (f)(1)(vi), (f)(2)(ii),
		   (f)(3)(i). EIA 608-B Section 6.4.2, 7.4. */
		if (column > CC_FIRST_COLUMN)
			--column;
	} else {
		/* 47 CFR 15.119 (f)(1)(v), (f)(1)(vi), (f)(2)(ii),
		   (f)(3)(i). EIA 608-B Section 7.4. */
		if (column < CC_LAST_COLUMN)
			ch->curr_column = column + 1;
	}

	text = &ch->buffer[curr_buffer][row][0];
	text[column] = c;

	/* Send a display update event when the displayed buffer of
	   the current channel changed, but no more than once for each
	   pair of Closed Caption bytes. */
	/* XXX This may not be a visible change, but such cases are
	   rare and we'd need something close to format_row() to be
	   sure. */
	if (CC_MODE_POP_ON != ch->mode) {
		cd->event_pending = ch;
	}

	if (displayable) {
		/* Note EIA 608-B Annex C.7, C.14. */
		if (CC_FIRST_COLUMN == column
				|| 0 == text[column - 1]) {
			/* Note last_pac may be 0 as well. */
			text[column - 1] = ch->last_pac;
		}

		if (c >= 'a' && c <= 'z') {
			ch->uppercase_predictor = 0;
		} else if (c >= 'A' && c <= 'Z') {
			unsigned int up;

			up = ch->uppercase_predictor + 1;
			if (up > 0)
				ch->uppercase_predictor = up;
		}
	} else if (0 == c) {
		unsigned int i;

		/* This is Special Character "Transparent space". */

		for (i = CC_FIRST_COLUMN; i <= CC_LAST_COLUMN; ++i)
			c |= ch->buffer[curr_buffer][row][i];

		ch->dirty[curr_buffer] &= ~((0 == c) << row);

		return;
	}

	if ((row >= (sizeof(ch->dirty[0]) * 8)) || (column >= CC_MAX_ROWS))
		return;

	ch->dirty[curr_buffer] |= 1 << row;
}

static void atsc_xds_print_info(cc_caption_recorder *cr,
		unsigned int mode,
		unsigned int type)
{
	const char *infoptr;

	if (!cr->info[0][mode][type].print)
		return;

	infoptr = cr->info[cr->field][mode][type].packet;

	switch ((mode << 8) + type) {
	case 0x0101:
		gxlogd("%s TIMECODE: %d/%02d %d:%02d%s\n",
				cr->xds_info_prefix,
				infoptr[3]&0x0f,infoptr[2]&0x1f,
				infoptr[1]&0x1f,infoptr[0]&0x3f,
				cr->xds_info_suffix);
	case 0x0102:
		if ((infoptr[1]&0x3f)>5)
			break;
		gxlogd("%s  LENGTH: %d:%02d:%02d of %d:%02d:00%s\n",
				cr->xds_info_prefix,
				infoptr[3]&0x3f,infoptr[2]&0x3f,
				infoptr[4]&0x3f,infoptr[1]&0x3f,
				infoptr[0]&0x3f,
				cr->xds_info_suffix);
		break;
	case 0x0103:
		gxlogd("%s   TITLE: %s%s\n",
				cr->xds_info_prefix,
				infoptr,
				cr->xds_info_suffix);
		break;
	case 0x0105:
		gxlogd("%s  RATING: %s (%d)",
				cr->xds_info_prefix,
				ratings[infoptr[0]&0x07],infoptr[0]);
		if ((infoptr[0]&0x07)>0) {
			if (infoptr[0]&0x20) gxlogd(" VIOLENCE");
			if (infoptr[0]&0x10) gxlogd(" SEXUAL");
			if (infoptr[0]&0x08) gxlogd(" LANGUAGE");
		}
		gxlogd(" %s\n", cr->xds_info_suffix);
		break;
	case 0x0501:
		gxlogd("%s NETWORK: %s%s\n",
				cr->xds_info_prefix,
				infoptr,
				cr->xds_info_suffix);
		break;
	case 0x0502:
		gxlogd("%s    CALL: %s%s",
				cr->xds_info_prefix,
				infoptr,
				cr->xds_info_suffix);
		break;
	case 0x0701:
		gxlogd("%sCUR.TIME: %d:%02d %d/%02d/%04d UTC%s",
				cr->xds_info_prefix,
				infoptr[1]&0x1F,infoptr[0]&0x3f,
				infoptr[3]&0x0f,infoptr[2]&0x1f,
				(infoptr[5]&0x3f)+1990,
				cr->xds_info_suffix);
		break;
	case 0x0704: //timezone
		gxlogd("%sTIMEZONE: UTC-%d%s",
				cr->xds_info_prefix,
				infoptr[0]&0x1f,
				cr->xds_info_suffix);
		break;
	case 0x0104: //program genere
		break;
	case 0x0110:
	case 0x0111:
	case 0x0112:
	case 0x0113:
	case 0x0114:
	case 0x0115:
	case 0x0116:
	case 0x0117:
		gxlogd("%s    DESC: %s%s",
				cr->xds_info_prefix,
				infoptr,
				cr->xds_info_suffix);
		break;
	}
}

static int atsc_xds_decode(cc_caption_recorder *cr, int data)
{
	static vbi_bool in_xds[2];
	int b1, b2, length;

	if (data == -1)
		return -1;

	b1 = data & 0x7F;
	b2 = (data>>8) & 0x7F;

	if (0 == b1) {
		/* Filler, discard. */
		return -1;
	} else if (b1 < 15) {// start packet
		cr->mode = b1;
		cr->type = b2;
		cr->infochecksum = b1 + b2 + 15;
		if (cr->mode > 8 || cr->type > 20) {
			cr->mode=0; cr->type=0;
		}
		cr->infoptr = cr->newinfo[cr->field][cr->mode][cr->type];
		in_xds[cr->field] = TRUE;
	} else if (b1 == 15) { // eof (next byte is checksum)
		if (cr->mode == 0) return 0;
		if (b2 != 128-((cr->infochecksum%128)&0x7F)) return 0;

		length = cr->infoptr - cr->newinfo[cr->field][cr->mode][cr->type];

		//don't bug the user with repeated data
		//only parse it if it's different
		if (cr->info[cr->field][cr->mode][cr->type].length != length
				|| 0 != memcmp (cr->info[cr->field][cr->mode][cr->type].packet,
					cr->newinfo[cr->field][cr->mode][cr->type], length)) {
			memcpy (cr->info[cr->field][cr->mode][cr->type].packet,
					cr->newinfo[cr->field][cr->mode][cr->type], 32);
			cr->info[cr->field][cr->mode][cr->type].packet[length] = 0;
			cr->info[cr->field][cr->mode][cr->type].length = length;
			if (debug_atsc_cc)
				atsc_xds_print_info (cr, cr->mode, cr->type);
		}
		cr->mode = 0; cr->type = 0;
		in_xds[cr->field] = FALSE;
	} else if (b1 <= 31) {
		/* Caption control code. */
		in_xds[cr->field] = FALSE;
	} else if (in_xds[cr->field]) {
		if (cr->infoptr >= &cr->newinfo[cr->field][cr->mode][cr->type][32]) {
			/* Bad packet. */
			cr->mode = 0;
			cr->type = 0;
			in_xds[cr->field] = 0;
		} else {
			cr->infoptr[0] = b1; cr->infoptr++;
			cr->infoptr[0] = b2; cr->infoptr++;
			cr->infochecksum += b1 + b2;
		}
	}

	return 0;
}

static void atsc_cc_erase_memory(cc_decoder *cd, cc_channel *ch, unsigned int buffer)
{
	if (0 != ch->dirty[buffer]) {
		if (debug_atsc_cc)
			gxlogi("erase: buffer %d\n", buffer);
		CLEAR (ch->buffer[buffer]);

		ch->dirty[buffer] = 0;
		if (buffer == ch->displayed_buffer)
			atsc_cc_display_event (cd, ch, 0);
	}
}

static void atsc_cc_text_restart(cc_decoder *cd, cc_channel *ch)
{
	unsigned int curr_buffer;
	unsigned int row;

	/* TR Text Restart -- 001 c10f  010 1010 */

	curr_buffer = ch->displayed_buffer;
	row = ch->curr_row;

	/* ch->mode is invariably CC_MODE_TEXT. */

	if (0 != (ch->dirty[curr_buffer] & (1 << row))) {
		atsc_cc_stream_event (cd, ch, row, row);
	}

	/* EIA 608-B Section 7.4. */
	/* May send a display event. */
	atsc_cc_erase_memory (cd, ch, ch->displayed_buffer);

	/* EIA 608-B Section 7.4. */
	ch->curr_row = CC_FIRST_ROW;
	ch->curr_column = CC_FIRST_COLUMN;
}

static void atsc_cc_erase_displayed_memory(cc_decoder *cd, cc_channel *ch)
{
	unsigned int row;

	/* EDM Erase Displayed Memory -- 001 c10f  010 1100 */

	switch (ch->mode) {
	case CC_MODE_UNKNOWN:
		/* We have not received EOC, RCL, RDC or RUx yet, but
		   ch is valid. */
		break;

	case CC_MODE_ROLL_UP:
		row = ch->curr_row;
		if (0 != (ch->dirty[ch->displayed_buffer] & (1 << row)))
			atsc_cc_stream_event (cd, ch, row, row);
		break;

	case CC_MODE_PAINT_ON:
		atsc_cc_stream_event_if_changed (cd, ch);
		break;

	case CC_MODE_POP_ON:
		/* Nothing to do. */
		break;

	case CC_MODE_TEXT:
		/* Not reached. (ch is a caption channel.) */
		return;
	}

	/* May send a display event. */
	atsc_cc_erase_memory (cd, ch, ch->displayed_buffer);
}

static void atsc_cc_end_of_caption(cc_decoder *cd, cc_channel *ch)
{
	unsigned int curr_buffer;
	unsigned int row;

	/* EOC End Of Caption -- 001 c10f  010 1111 */

	curr_buffer = ch->displayed_buffer;

	switch (ch->mode) {
	case CC_MODE_UNKNOWN:
	case CC_MODE_POP_ON:
		break;

	case CC_MODE_ROLL_UP:
		row = ch->curr_row;
		if (0 != (ch->dirty[curr_buffer] & (1 << row)))
			atsc_cc_stream_event (cd, ch, row, row);
		break;

	case CC_MODE_PAINT_ON:
		atsc_cc_stream_event_if_changed (cd, ch);
		break;

	case CC_MODE_TEXT:
		/* Not reached. (ch is a caption channel.) */
		return;
	}

	ch->displayed_buffer = curr_buffer ^= 1;

	/* 47 CFR Section 15.119 (f)(2). */
	ch->mode = CC_MODE_POP_ON;

	if (debug_atsc_cc)
		gxlogi("display: buffer %d\n", ch->displayed_buffer);

	if (0 != ch->dirty[curr_buffer]) {
		atsc_cc_stream_event (cd, ch,
				CC_FIRST_ROW,
				CC_LAST_ROW);

		atsc_cc_display_event (cd, ch, 0);
		CLEAR(ch->buffer[ch->displayed_buffer]);
	}
}

static void atsc_cc_carriage_return(cc_decoder *cd, cc_channel *ch)
{
	unsigned int curr_buffer;
	unsigned int row;
	unsigned int window_rows;
	unsigned int first_row;

	/* CR Carriage Return -- 001 c10f  010 1101 */

	curr_buffer = ch->displayed_buffer;
	row = ch->curr_row;

	switch (ch->mode) {
	case CC_MODE_UNKNOWN:
		return;

	case CC_MODE_ROLL_UP:
		/* 47 CFR Section 15.119 (f)(1)(iii). */
		ch->curr_column = CC_FIRST_COLUMN;

		/* 47 CFR 15.119 (f)(1): "The cursor always remains on
		   the base row." */

		/* XXX Spec? */
		ch->last_pac = 0;

		/* No event if the buffer contains only
		   TRANSPARENT_SPACEs. */
		if (0 == ch->dirty[curr_buffer])
			return;

		window_rows = MIN (row + 1 - CC_FIRST_ROW,
				ch->window_rows);
		break;

	case CC_MODE_POP_ON:
	case CC_MODE_PAINT_ON:
		/* 47 CFR 15.119 (f)(2)(i), (f)(3)(i): No effect. */
		return;

	case CC_MODE_TEXT:
		/* 47 CFR Section 15.119 (f)(1)(iii). */
		ch->curr_column = CC_FIRST_COLUMN;

		/* XXX Spec? */
		ch->last_pac = 0;

		/* EIA 608-B Section 7.4: "When Text Mode has
		   initially been selected and the specified Text
		   memory is empty, the cursor starts at the topmost
		   row, Column 1, and moves down to Column 1 on the
		   next row each time a Carriage Return is received
		   until the last available row is reached. A variety
		   of methods may be used to accomplish the scrolling,
		   provided that the text is legible while moving. For
		   example, as soon as all of the available rows of
		   text are on the screen, Text Mode switches to the
		   standard roll-up type of presentation." */

		if (CC_LAST_ROW != row) {
			if (0 != (ch->dirty[curr_buffer] & (1 << row))) {
				atsc_cc_stream_event (cd, ch, row, row);
			}

			ch->curr_row = row + 1;

			return;
		}

		/* No event if the buffer contains all
		   TRANSPARENT_SPACEs. */
		if (0 == ch->dirty[curr_buffer])
			return;

		window_rows = CC_MAX_ROWS;

		break;
	}

	/* 47 CFR Section 15.119 (f)(1)(iii). In roll-up mode: "Each
	   time a Carriage Return is received, the text in the top row
	   of the window is erased from memory and from the display or
	   scrolled off the top of the window. The remaining rows of
	   text are each rolled up into the next highest row in the
	   window, leaving the base row blank and ready to accept new
	   text." */

	if (0 != (ch->dirty[curr_buffer] & (1 << row))) {
		atsc_cc_stream_event (cd, ch, row, row);
	}

	first_row = row + 1 - window_rows;
	memmove (ch->buffer[curr_buffer][first_row],
			ch->buffer[curr_buffer][first_row + 1],
			(window_rows - 1) * sizeof (ch->buffer[0][0]));

	ch->dirty[curr_buffer] >>= 1;

	memset (ch->buffer[curr_buffer][row], 0,
			sizeof (ch->buffer[0][0]));

	atsc_cc_display_event (cd, ch, VBI_START_ROLLING);
}

static void atsc_cc_resume_direct_captioning(cc_decoder *cd, cc_channel *ch)
{
	unsigned int curr_buffer;
	unsigned int row;

	/* RDC Resume Direct Captioning -- 001 c10f  010 1001 */

	/* 47 CFR 15.119 (f)(1)(x), (f)(2)(vi) and EIA 608-B Annex
	   B.7: Does not erase memory, does not move the cursor when
	   resuming after a Text transmission.

	   XXX If ch->mode is unknown, roll-up or pop-on, what shall
	   we do if no PAC is received between RDC and the text? */

	curr_buffer = ch->displayed_buffer;
	row = ch->curr_row;

	switch (ch->mode) {
	case CC_MODE_ROLL_UP:
		if (0 != (ch->dirty[curr_buffer] & (1 << row)))
			atsc_cc_stream_event (cd, ch, row, row);

		/* fall through */

	case CC_MODE_UNKNOWN:
	case CC_MODE_POP_ON:
		/* No change since last stream_event(). */
		memcpy (ch->buffer[2], ch->buffer[curr_buffer],
				sizeof (ch->buffer[2]));
		break;

	case CC_MODE_PAINT_ON:
		/* Mode continues. */
		break;

	case CC_MODE_TEXT:
		/* Not reached. (ch is a caption channel.) */
		return;
	}

	ch->mode = CC_MODE_PAINT_ON;
}

static void atsc_cc_resize_window(cc_decoder *cd, cc_channel *ch, unsigned int new_rows)
{
	unsigned int curr_buffer;
	unsigned int max_rows;
	unsigned int old_rows;
	unsigned int row1;

	curr_buffer = ch->displayed_buffer;

	/* No event if the buffer contains all TRANSPARENT_SPACEs. */
	if (0 == ch->dirty[curr_buffer])
		return;

	row1 = ch->curr_row + 1;
	max_rows = row1 - CC_FIRST_ROW;
	old_rows = MIN (ch->window_rows, max_rows);
	new_rows = MIN (new_rows, max_rows);

	/* Nothing to do unless the window shrinks. */
	if (0 == new_rows || new_rows >= old_rows)
		return;

	memset (&ch->buffer[curr_buffer][row1 - old_rows][0], 0,
			(old_rows - new_rows)
			* sizeof (ch->buffer[0][0]));

	ch->dirty[curr_buffer] &= -1 << (row1 - new_rows);

	atsc_cc_display_event (cd, ch, 0);
}

static void atsc_cc_roll_up_caption(cc_decoder *cd, cc_channel *ch, unsigned int c2)
{
	unsigned int window_rows;

	/* Roll-Up Captions -- 001 c10f  010 01xx */

	window_rows = (c2 & 7) - 3; /* 2, 3, 4 */

	switch (ch->mode) {
	case CC_MODE_ROLL_UP:
		/* 47 CFR 15.119 (f)(1)(iv). */
		/* May send a display event. */
		atsc_cc_resize_window (cd, ch, window_rows);

		/* fall through */

	case CC_MODE_UNKNOWN:
		ch->mode = CC_MODE_ROLL_UP;
		ch->window_rows = window_rows;

		/* 47 CFR 15.119 (f)(1)(ix): No cursor movements,
		   no memory erasing. */

		break;

	case CC_MODE_PAINT_ON:
		atsc_cc_stream_event_if_changed (cd, ch);

		/* fall through */

	case CC_MODE_POP_ON:
		ch->mode = CC_MODE_ROLL_UP;
		ch->window_rows = window_rows;

		/* 47 CFR 15.119 (f)(1)(ii). */
		ch->curr_row = CC_LAST_ROW;
		ch->curr_column = CC_FIRST_COLUMN;

		/* 47 CFR 15.119 (f)(1)(x). */
		/* May send a display event. */
		atsc_cc_erase_memory (cd, ch, ch->displayed_buffer);
		atsc_cc_erase_memory (cd, ch, ch->displayed_buffer ^ 1);

		break;

	case CC_MODE_TEXT:
		/* Not reached. (ch is a caption channel.) */
		return;
	}
}

static void atsc_cc_delete_to_end_of_row(cc_decoder *cd, cc_channel *ch)
{
	unsigned int curr_buffer;
	unsigned int row;

	/* DER Delete To End Of Row -- 001 c10f  010 0100 */

	/* 47 CFR 15.119 (f)(1)(vii), (f)(2)(iii), (f)(3)(ii) and EIA
	   608-B Section 7.4: In all caption modes and Text mode
	   "[the] Delete to End of Row command will erase from memory
	   any characters or control codes starting at the current
	   cursor location and in all columns to its right on the same
	   row." */

	curr_buffer = ch->displayed_buffer
		^ (CC_MODE_POP_ON == ch->mode);

	row = ch->curr_row;

	/* No event if the row contains only TRANSPARENT_SPACEs. */
	if (0 != (ch->dirty[curr_buffer] & (1 << row))) {
		unsigned int column;
		unsigned int i;
		uint16_t c;

		column = ch->curr_column;

		memset (&ch->buffer[curr_buffer][row][column], 0,
				(CC_LAST_COLUMN - column + 1)
				* sizeof (ch->buffer[0][0][0]));

		c = 0;
		for (i = CC_FIRST_COLUMN; i < column; ++i)
			c |= ch->buffer[curr_buffer][row][i];

		ch->dirty[curr_buffer] &= ~((0 == c) << row);

		atsc_cc_display_event (cd, ch, 0);
	}
}

static void atsc_cc_backspace(cc_decoder *cd,  cc_channel *ch)
{
	unsigned int curr_buffer;
	unsigned int row;
	unsigned int column;

	/* BS Backspace -- 001 c10f  010 0001 */

	/* 47 CFR Section 15.119 (f)(1)(vi), (f)(2)(ii), (f)(3)(i) and
	   EIA 608-B Section 7.4. */
	column = ch->curr_column;
	if (column <= CC_FIRST_COLUMN)
		return;

	ch->curr_column = --column;

	curr_buffer = ch->displayed_buffer
		^ (CC_MODE_POP_ON == ch->mode);

	row = ch->curr_row;

	/* No event if there's no visible effect. */
	if (0 != ch->buffer[curr_buffer][row][column]) {
		unsigned int i;
		uint16_t c;

		/* 47 CFR 15.119 (f), (f)(1)(vi), (f)(2)(ii) and EIA
		   608-B Section 7.4. */
		ch->buffer[curr_buffer][row][column] = 0;

		c = 0;
		for (i = CC_FIRST_COLUMN; i <= CC_LAST_COLUMN; ++i)
			c |= ch->buffer[curr_buffer][row][i];

		ch->dirty[curr_buffer] &= ~((0 == c) << row);

		atsc_cc_display_event (cd, ch, 0);
	}
}

static void atsc_cc_move_window(cc_decoder *cd, cc_channel *ch, unsigned int new_base_row)
{
	uint8_t *base;
	unsigned int curr_buffer;
	unsigned int bytes_per_row;
	unsigned int old_max_rows;
	unsigned int new_max_rows;
	unsigned int copy_bytes;
	unsigned int erase_begin;
	unsigned int erase_end;

	curr_buffer = ch->displayed_buffer;

	/* No event if we do not move the window or the buffer
	   contains only TRANSPARENT_SPACEs. */
	if (new_base_row == ch->curr_row
			|| 0 == ch->dirty[curr_buffer])
		return;

	base = (void *) &ch->buffer[curr_buffer][CC_FIRST_ROW][0];
	bytes_per_row = sizeof (ch->buffer[0][0]);

	old_max_rows = ch->curr_row + 1 - CC_FIRST_ROW;
	new_max_rows = new_base_row + 1 - CC_FIRST_ROW;
	copy_bytes = MIN (MIN (old_max_rows, new_max_rows),
			ch->window_rows) * bytes_per_row;

	if (new_base_row < ch->curr_row) {
		erase_begin = (new_base_row + 1) * bytes_per_row;
		erase_end = (ch->curr_row + 1) * bytes_per_row;

		memmove (base + erase_begin - copy_bytes,
				base + erase_end - copy_bytes, copy_bytes);

		ch->dirty[curr_buffer] >>= ch->curr_row - new_base_row;
	} else {
		erase_begin = (ch->curr_row + 1) * bytes_per_row
			- copy_bytes;
		erase_end = (new_base_row + 1) * bytes_per_row
			- copy_bytes;

		memmove (base + erase_end,
				base + erase_begin, copy_bytes);

		ch->dirty[curr_buffer] <<= new_base_row - ch->curr_row;
		ch->dirty[curr_buffer] &= CC_ALL_ROWS_MASK;
	}

	memset (base + erase_begin, 0, erase_end - erase_begin);

	atsc_cc_display_event (cd, ch, 0);
}

static vbi_pgno atsc_cc_channel_num(cc_decoder * cd, cc_channel *ch)
{
	return (ch - cd->channel) + 1;
}

static void atsc_cc_stream_event(cc_decoder *cd,
		cc_channel *ch,
		unsigned int first_row,
		unsigned int last_row)
{
	vbi_pgno channel;
	unsigned int row;

	channel = atsc_cc_channel_num (cd, ch);

	for (row = first_row; row <= last_row; ++row) {
		struct vbi_char text[MAX_VBI_CHAR_COLUMNS];
		unsigned int end;

		atsc_cc_format_row (cd, text, ch,
				ch->displayed_buffer,
				row, /* to_upper */ FALSE,
				/* padding */ FALSE);

		for (end = 32; end > 0; --end) {
			if (VBI_TRANSPARENT_SPACE != text[end - 1].opacity)
				break;
		}

		if (0 == end)
			continue;
		{
			cr_new_line ((cc_caption_recorder *)cd->cr, channel, ch->mode, text, /* length */ 32);
		}
	}
}

/* Send a stream event if the current row has changed since the last
   stream event. This is necessary in paint-on mode where CR has no
   function and captioners can freely position the cursor to erase or
   overwrite (parts of) rows. */
static void atsc_cc_stream_event_if_changed(cc_decoder *cd, cc_channel *ch)
{
	unsigned int curr_buffer;
	unsigned int row;
	unsigned int i;

	curr_buffer = ch->displayed_buffer;
	row = ch->curr_row;

	if (0 == (ch->dirty[curr_buffer] & (1 << row)))
		return;

	for (i = CC_FIRST_COLUMN; i <= CC_LAST_COLUMN; ++i) {
		unsigned int c1;
		unsigned int c2;

		c1 = ch->buffer[curr_buffer][row][i];
		if (c1 >= 0x1040) {
			if (c1 < 0x1120) {
				c1 = 0; /* PAC -- non-spacing */
			} else if (c1 < 0x1130 || c1 >= 0x1428) {
				/* MR, FON, BT, FA, FAU -- spacing */
				c1 = 0x20;
			}
		}

		c2 = ch->buffer[2][row][i];
		if (c2 >= 0x1040) {
			if (c2 < 0x1120) {
				c2 = 0;
			} else if (c2 < 0x1130 || c2 >= 0x1428) {
				c1 = 0x20;
			}
		}

		if (c1 != c2) {
			atsc_cc_stream_event (cd, ch, row, row);

			memcpy (ch->buffer[2][row],
					ch->buffer[curr_buffer][row],
					sizeof (ch->buffer[0][0]));

			ch->dirty[2] = ch->dirty[curr_buffer];

			return;
		}
	}
}

static void atsc_cc_preamble_address_code(cc_decoder *cd,
		cc_channel *ch,
		unsigned int c1,
		unsigned int c2)
{
	unsigned int row;

	/* PAC Preamble Address Codes -- 001 crrr  1ri xxxu */

	row = cc_pac_row_map[(c1 & 7) * 2 + ((c2 >> 5) & 1)];
	if ((int) row < 0)
		return;

	switch (ch->mode) {
	case CC_MODE_UNKNOWN:
		return;

	case CC_MODE_ROLL_UP:
		/* EIA 608-B Annex C.4. */
		if (ch->window_rows > row + 1)
			row = ch->window_rows - 1;

		/* 47 CFR Section 15.119 (f)(1)(ii). */
		/* May send a display event. */
		atsc_cc_move_window (cd, ch, row);

		ch->curr_row = row;

		break;

	case CC_MODE_PAINT_ON:
		atsc_cc_stream_event_if_changed (cd, ch);

		/* fall through */

	case CC_MODE_POP_ON:
		/* XXX 47 CFR 15.119 (f)(2)(i), (f)(3)(i): In Pop-on
		   and paint-on mode "Preamble Address Codes can be
		   used to move the cursor around the screen in random
		   order to place captions on Rows 1 to 15." We do not
		   have a limit on the number of displayable rows, but
		   as EIA 608-B Annex C.6 points out, if more than
		   four rows must be displayed they were probably
		   received in error and we should respond
		   accordingly. */

		/* 47 CFR Section 15.119 (d)(1)(i) and EIA 608-B Annex
		   C.7. */
		ch->curr_row = row;

		break;

	case CC_MODE_TEXT:
		/* 47 CFR 15.119 (e)(1) and EIA 608-B Section 7.4:
		   Does not change the cursor row. */
		break;
	}

	if (c2 & 0x10) {
		/* 47 CFR 15.119 (e)(1)(i) and EIA 608-B Table 71. */
		ch->curr_column = CC_FIRST_COLUMN + (c2 & 0x0E) * 2;
	}

	/* PAC is a non-spacing attribute for the next character, see
	   cc_put_char(). */
	ch->last_pac = 0x1000 | c2;
}

static void atsc_cc_resume_caption_loading(cc_decoder *cd, cc_channel *ch)
{
	unsigned int row;

	/* RCL Resume Caption Loading -- 001 c10f  010 0000 */

	switch (ch->mode) {
	case CC_MODE_UNKNOWN:
	case CC_MODE_POP_ON:
		break;

	case CC_MODE_ROLL_UP:
		row = ch->curr_row;
		if (0 != (ch->dirty[ch->displayed_buffer] & (1 << row)))
			atsc_cc_stream_event (cd, ch, row, row);
		break;

	case CC_MODE_PAINT_ON:
		atsc_cc_stream_event_if_changed (cd, ch);
		break;

	case CC_MODE_TEXT:
		/* Not reached. (ch is a caption channel.) */
		return;
	}

	/* 47 CFR 15.119 (f)(1)(x): Does not erase memory.
	   (f)(2)(iv): Cursor position remains unchanged. */

	ch->mode = CC_MODE_POP_ON;
}

/* Note curr_ch is invalid if UNKNOWN_CC_CHANNEL == cd->cc.curr_ch_num. */
static cc_channel *atsc_cc_switch_channel(cc_decoder *cd,
		cc_channel *curr_ch,
		vbi_pgno new_ch_num,
		enum field_num f)
{
	cc_channel *new_ch;

	if (UNKNOWN_CC_CHANNEL != cd->curr_ch_num[f]
			&& CC_MODE_UNKNOWN != curr_ch->mode) {
		/* XXX Force a display update if we do not send events
		   on every display change. */
	}

	cd->curr_ch_num[f] = new_ch_num;
	new_ch = &cd->channel[new_ch_num - VBI_CAPTION_CC1];

	return new_ch;
}

static void atsc_cc_ext_control_code(cc_decoder *cd, cc_channel *ch, unsigned int c2)
{
	unsigned int column;

	switch (c2) {
	case 0x21: /* TO1 */
	case 0x22: /* TO2 */
	case 0x23: /* TO3 Tab Offset -- 001 c111  010 00xx */
		/* 47 CFR 15.119 (e)(1)(ii). EIA 608-B Section 7.4,
		   Annex C.7. */
		column = ch->curr_column + (c2 & 3);
		ch->curr_column = MIN (column,
				(unsigned int) CC_LAST_COLUMN);
		break;

	case 0x24: /* Select standard character set in normal size */
	case 0x25: /* Select standard character set in double size */
	case 0x26: /* Select first private character set */
	case 0x27: /* Select second private character set */
	case 0x28: /* Select character set GB 2312-80 (Chinese) */
	case 0x29: /* Select character set KSC 5601-1987 (Korean) */
	case 0x2A: /* Select first registered character set. */
		/* EIA 608-B Section 6.3 Closed Group Extensions. */
		break;

	case 0x2D: /* BT Background Transparent -- 001 c111  010 1101 */
	case 0x2E: /* FA Foreground Black -- 001 c111  010 1110 */
	case 0x2F: /* FAU Foregr. Black Underl. -- 001 c111  010 1111 */
		/* EIA 608-B Section 6.2. */
		atsc_cc_put_char (cd, ch, 0x1700 | c2,
				/* displayable */ FALSE,
				/* backspace */ TRUE);
		break;

	default:
		/* 47 CFR Section 15.119 (j): Ignore. */
		break;
	}
}

static void atsc_cc_misc_control_code(cc_decoder *cd,
		cc_channel *ch,
		unsigned int c2,
		unsigned int ch_num0,
		enum field_num f)
{
	unsigned int new_ch_num;

	/* Misc Control Codes -- 001 c10f  010 xxxx */

	/* c = channel (0 -> CC1/CC3/T1/T3, 1 -> CC2/CC4/T2/T4)
	   -- 47 CFR Section 15.119, EIA 608-B Section 7.7.
	   f = field (0 -> F1, 1 -> F2)
	   -- EIA 608-B Section 8.4, 8.5. */

	/* XXX The f flag is intended to detect accidential field
	   swapping and we should use it for that purpose. */

	switch (c2 & 15) {
	case 0:	/* RCL Resume Caption Loading -- 001 c10f  010 0000 */
		/* 47 CFR 15.119 (f)(2) and EIA 608-B Section 7.7. */
		new_ch_num = VBI_CAPTION_CC1 + (ch_num0 & 3);
		ch = atsc_cc_switch_channel (cd, ch, new_ch_num, f);
		atsc_cc_resume_caption_loading (cd, ch);
		break;

	case 1: /* BS Backspace -- 001 c10f  010 0001 */
		if (UNKNOWN_CC_CHANNEL == cd->curr_ch_num[f]
				|| CC_MODE_UNKNOWN == ch->mode)
			break;
		atsc_cc_backspace (cd, ch);
		break;

	case 2: /* reserved (formerly AOF Alarm Off) */
	case 3: /* reserved (formerly AON Alarm On) */
		break;

	case 4: /* DER Delete To End Of Row -- 001 c10f  010 0100 */
		if (UNKNOWN_CC_CHANNEL == cd->curr_ch_num[f]
				|| CC_MODE_UNKNOWN == ch->mode)
			break;
		atsc_cc_delete_to_end_of_row (cd, ch);
		break;

	case 5: /* RU2 */
	case 6: /* RU3 */
	case 7: /* RU4 Roll-Up Captions -- 001 c10f  010 01xx */
		/* 47 CFR 15.119 (f)(1) and EIA 608-B Section 7.7. */
		new_ch_num = VBI_CAPTION_CC1 + (ch_num0 & 3);
		ch = atsc_cc_switch_channel (cd, ch, new_ch_num, f);
		atsc_cc_roll_up_caption (cd, ch, c2);
		break;

	case 8: /* FON Flash On -- 001 c10f  010 1000 */
		if (UNKNOWN_CC_CHANNEL == cd->curr_ch_num[f]
				|| CC_MODE_UNKNOWN == ch->mode)
			break;

		/* 47 CFR 15.119 (h)(1)(i): Spacing attribute. */
		atsc_cc_put_char (cd, ch, 0x1428,
				/* displayable */ FALSE,
				/* backspace */ FALSE);
		break;

	case 9: /* RDC Resume Direct Captioning -- 001 c10f  010 1001 */
		/* 47 CFR 15.119 (f)(3) and EIA 608-B Section 7.7. */
		new_ch_num = VBI_CAPTION_CC1 + (ch_num0 & 3);
		ch = atsc_cc_switch_channel (cd, ch, new_ch_num, f);
		atsc_cc_resume_direct_captioning (cd, ch);
		break;

	case 10: /* TR Text Restart -- 001 c10f  010 1010 */
		/* EIA 608-B Section 7.4. */
		new_ch_num = VBI_CAPTION_T1 + (ch_num0 & 3);
		ch = atsc_cc_switch_channel (cd, ch, new_ch_num, f);
		atsc_cc_text_restart (cd, ch);
		break;

	case 11: /* RTD Resume Text Display -- 001 c10f  010 1011 */
		/* EIA 608-B Section 7.4. */
		new_ch_num = VBI_CAPTION_T1 + (ch_num0 & 3);
		ch = atsc_cc_switch_channel (cd, ch, new_ch_num, f);
		/* ch->mode is invariably CC_MODE_TEXT. */
		break;

	case 12: /* EDM Erase Displayed Memory -- 001 c10f  010 1100 */
		/* 47 CFR 15.119 (f). EIA 608-B Section 7.7 and Annex
		   B.7: "[The] command shall be acted upon as
		   appropriate for caption processing without
		   terminating the Text Mode data stream." */

		/* We need not check cd->curr_ch_num because bit 2 is
		   implied, bit 1 is the known field number and bit 0
		   is coded in the control code. */
		ch = &cd->channel[ch_num0 & 3];

		atsc_cc_erase_displayed_memory (cd, ch);

		break;

	case 13: /* CR Carriage Return -- 001 c10f  010 1101 */
		if (UNKNOWN_CC_CHANNEL == cd->curr_ch_num[f])
			break;
		atsc_cc_carriage_return (cd, ch);
		break;

	case 14: /* ENM Erase Non-Displayed Memory -- 001 c10f  010 1110 */
		/* 47 CFR 15.119 (f)(2)(v). EIA 608-B Section 7.7 and
		   Annex B.7: "[The] command shall be acted upon as
		   appropriate for caption processing without
		   terminating the Text Mode data stream." */

		/* See EDM. */
		ch = &cd->channel[ch_num0 & 3];

		atsc_cc_erase_memory (cd, ch, ch->displayed_buffer ^ 1);

		break;

	case 15: /* EOC End Of Caption -- 001 c10f  010 1111 */
		/* 47 CFR 15.119 (f), (f)(2), (f)(3)(iv) and EIA 608-B
		   Section 7.7, Annex C.11. */
		new_ch_num = VBI_CAPTION_CC1 + (ch_num0 & 3);
		ch = atsc_cc_switch_channel (cd, ch, new_ch_num, f);
		atsc_cc_end_of_caption (cd, ch);
		break;
	}
}

static void atsc_cc_control_code(cc_decoder *cd,
		unsigned int c1,
		unsigned int c2,
		enum field_num f)
{
	cc_channel *ch;
	unsigned int ch_num0;

	if (debug_atsc_cc)
		gxlogi("%s %02x %02x %d\n",  __FUNCTION__, c1, c2, f);

	/* Caption / text, field 1 / 2, primary / secondary channel. */
	ch_num0 = (((cd->curr_ch_num[f] - VBI_CAPTION_CC1) & 4)
			+ f * 2
			+ ((c1 >> 3) & 1));

	/* Note ch is invalid if UNKNOWN_CC_CHANNEL ==
	   cd->curr_ch_num[f]. */
	ch = &cd->channel[ch_num0];

	if (c2 >= 0x40) {
		/* Preamble Address Codes -- 001 crrr  1ri xxxu */
		if (UNKNOWN_CC_CHANNEL != cd->curr_ch_num[f])
			atsc_cc_preamble_address_code (cd, ch, c1, c2);
		return;
	}

	switch (c1 & 7) {
	case 0:
		if (UNKNOWN_CC_CHANNEL == cd->curr_ch_num[f]
				|| CC_MODE_UNKNOWN == ch->mode)
			break;

		if (c2 < 0x30) {
			/* Backgr. Attr. Codes -- 001 c000  010 xxxt */
			/* EIA 608-B Section 6.2. */
			atsc_cc_put_char (cd, ch, 0x1000 | c2,
					/* displayable */ FALSE,
					/* backspace */ TRUE);
		} else {
			/* Undefined. */
		}

		break;

	case 1:
		if (UNKNOWN_CC_CHANNEL == cd->curr_ch_num[f]
				|| CC_MODE_UNKNOWN == ch->mode)
			break;

		if (c2 < 0x30) {
			/* Mid-Row Codes -- 001 c001  010 xxxu */
			/* 47 CFR 15.119 (h)(1)(i): Spacing attribute. */
			atsc_cc_put_char (cd, ch, 0x1100 | c2,
					/* displayable */ FALSE,
					/* backspace */ FALSE);
		} else {
			/* Special Characters -- 001 c001  011 xxxx */
			if (0x39 == c2) {
				/* Transparent space. */
				atsc_cc_put_char (cd, ch, 0,
						/* displayable */ FALSE,
						/* backspace */ FALSE);
			} else {
				atsc_cc_put_char (cd, ch, 0x1100 | c2,
						/* displayable */ TRUE,
						/* backspace */ FALSE);
			}
		}

		break;

	case 2:
	case 3: /* Extended Character Set -- 001 c01x  01x xxxx */
		if (UNKNOWN_CC_CHANNEL == cd->curr_ch_num[f]
				|| CC_MODE_UNKNOWN == ch->mode)
			break;

		/* EIA 608-B Section 6.4.2. */
		atsc_cc_put_char (cd, ch, (c1 * 256 + c2) & 0x777F,
				/* displayable */ TRUE,
				/* backspace */ TRUE);
		break;

	case 4:
	case 5:
		if (c2 < 0x30) {
			/* Misc. Control Codes -- 001 c10f  010 xxxx */
			atsc_cc_misc_control_code (cd, ch, c2, ch_num0, f);
		} else {
			/* Undefined. */
		}

		break;

	case 6: /* reserved */
		break;

	case 7:	/* Extended control codes -- 001 c111  01x xxxx */
		if (UNKNOWN_CC_CHANNEL == cd->curr_ch_num[f]
				|| CC_MODE_UNKNOWN == ch->mode)
			break;

		atsc_cc_ext_control_code (cd, ch, c2);

		break;
	}
}

static vbi_bool atsc_cc_characters(cc_decoder *cd, cc_channel *ch, int c)
{
	if (0 == c) {
		if (CC_MODE_UNKNOWN == ch->mode)
			return TRUE;

		/* XXX After x NUL characters (presumably a caption
		   pause), force a display update if we do not send
		   events on every display change. */

		return TRUE;
	}

	if (debug_atsc_cc) {
		char s[2];
		char row     = (char)ch->curr_row;
		char column  = (char)ch->curr_column;
		char display = (char)(ch->displayed_buffer ^ (CC_MODE_POP_ON == ch->mode));

		s[0] = c;
		s[1] = 0;
		gxlogi("%s: %d %02d %02d, %02x %s\n", __FUNCTION__, display, row, column, c, s);
	}

	if (c < 0x20) {
		/* Parity error or invalid data. */

		if (c < 0 && CC_MODE_UNKNOWN != ch->mode) {
			/* 47 CFR Section 15.119 (j)(1). */
			atsc_cc_put_char (cd, ch, 0x7F,
					/* displayable */ TRUE,
					/* backspace */ FALSE);
		}

		return FALSE;
	}

	if (CC_MODE_UNKNOWN != ch->mode) {
		atsc_cc_put_char (cd, ch, c,
				/* displayable */ TRUE,
				/* backspace */ FALSE);
	}

	return TRUE;
}

static vbi_bool atsc_cc_feed(cc_decoder *cd,
		const uint8_t buffer[2], unsigned int line)
{
	int c1, c2;
	enum field_num f;
	vbi_bool all_successful;

	if (debug_atsc_cc) {
		gxlogd("%s: %d %02x %02x\n", __FUNCTION__,
				buffer[0] & 0x7F, buffer[1] & 0x7F, line);
	}

	f = FIELD_1;

	switch (line) {
	case 21: /* NTSC */
	case 22: /* PAL/SECAM */
		break;

	case 284: /* NTSC */
		f = FIELD_2;
		break;

	default:
		return FALSE;
	}

	/* FIXME deferred reset here */

	c1 = vbi_unpar8 (buffer[0]);
	c2 = vbi_unpar8 (buffer[1]);

	all_successful = TRUE;

	/* 47 CFR 15.119 (2)(i)(4): "If the first transmission of a
	   control code pair passes parity, it is acted upon within
	   one video frame. If the next frame contains a perfect
	   repeat of the same pair, the redundant code is ignored. If,
	   however, the next frame contains a different but also valid
	   control code pair, this pair, too, will be acted upon (and
	   the receiver will expect a repeat of this second pair in
	   the next frame).  If the first byte of the expected
	   redundant control code pair fails the parity check and the
	   second byte is identical to the second byte in the
	   immediately preceding pair, then the expected redundant
	   code is ignored. If there are printing characters in place
	   of the redundant code, they will be processed normally."

	   EIA 608-B Section 8.3: Caption control codes on field 2 may
	   repeat as on field 1. Section 8.6.2: XDS control codes
	   shall not repeat. */

	if (c1 < 0) {
		goto parity_error;
	} else if (c1 == cd->expect_ctrl[f][0]
			&& c2 == cd->expect_ctrl[f][1]) {
		/* Already acted upon. */
		cd->expect_ctrl[f][0] = -1;
		goto finish;
	}

	if (c1 >= 0x10 && c1 < 0x20) {
		/* Caption control code. */

		/* There's no XDS on field 1, we just
		   use an array to save a branch. */
		cd->in_xds[f] = FALSE;

		/* 47 CFR Section 15.119 (i)(1), (i)(2). */
		if (c2 < 0x20) {
			/* Parity error or invalid control code.
			   Let's hope it repeats. */
			goto parity_error;
		}

		atsc_cc_control_code (cd, c1, c2, f);

		if (cd->event_pending) {
			atsc_cc_display_event (cd, cd->event_pending, 0);
			cd->event_pending = NULL;
		}

		cd->expect_ctrl[f][0] = c1;
		cd->expect_ctrl[f][1] = c2;
	} else {
		cd->expect_ctrl[f][0] = -1;

		if (c1 < 0x10) {
			if (FIELD_1 == f) {
				/* 47 CFR Section 15.119 (i)(1): "If the
				   non-printing character in the pair is
				   in the range 00h to 0Fh, that character
				   alone will be ignored and the second
				   character will be treated normally." */
				c1 = 0;
			} else if (0x0F == c1) {
				/* XDS packet terminator. */
				cd->in_xds[FIELD_2] = FALSE;
				goto finish;
			} else if (c1 >= 0x01) {
				/* XDS packet start or continuation.
				   EIA 608-B Section 7.7, 8.5: Also
				   interrupts a Text mode
				   transmission. */
				cd->in_xds[FIELD_2] = TRUE;
				goto finish;
			}
		}

		if (!cd->in_xds[f]) {
			cc_channel *ch;
			vbi_pgno ch_num;

			ch_num = cd->curr_ch_num[f];
			if (UNKNOWN_CC_CHANNEL == ch_num)
				goto finish;

			ch_num = ((ch_num - VBI_CAPTION_CC1) & 5) + f * 2;
			ch = &cd->channel[ch_num];

			all_successful &= atsc_cc_characters (cd, ch, c1);
			all_successful &= atsc_cc_characters (cd, ch, c2);

			if (cd->event_pending) {
				atsc_cc_display_event (cd, cd->event_pending, 0);
				cd->event_pending = NULL;
			}
		}
	}

finish:
	cd->error_history = cd->error_history * 2 + all_successful;

	return all_successful;

parity_error:
	cd->expect_ctrl[f][0] = -1;

	/* XXX Some networks stupidly transmit 0x0000 instead of
	   0x8080 as filler. Perhaps we shouldn't take that as a
	   serious parity error. */
	cd->error_history *= 2;

	return FALSE;
}

static unsigned int atsc_dtvcc_unicode(unsigned int c)
{
	if (0 == (c & 0x60)) {
		/* C0, C1, C2, C3 */
		return 0;
	} else if (c < 0x100) {
		/* G0, G1 */
		if (0x7F == c)
			return 0x266A; /* music note */
		else
			return c;
	} else if (c < 0x1080) {
		if (c < 0x1020)
			return 0;
		else
			return dtvcc_g2[c - 0x1020];
	} else if (0x10A0 == c) {
		/* We map all G2/G3 characters which are not
		   representable in Unicode to private code U+E900
		   ... U+E9FF. */
		return 0xE9A0; /* caption icon */
	}

	return 0;
}
static unsigned int atsc_dtvcc_window_id(cc_dtvcc_service *ds, cc_dtvcc_window *dw)
{
	return dw - ds->window;
}

static unsigned int atsc_dtvcc_service_num(cc_dtvcc_decoder *dc, cc_dtvcc_service *ds)
{
	int count = -1;
	int i     =  0;

	for(i = 0; i < MAX_CC_SERVICE; i++) {
		if(ds == dc->service[i]) {
			count = i;
		}
	}

	if (count != -1) {
		return count + 1;
	} else {
		return 0;
	}
}

/* Up to eight windows can be visible at once, so which one displays
   the caption? Let's take a guess. */
static cc_dtvcc_window *atsc_dtvcc_caption_window(cc_dtvcc_service *ds)
{
	cc_dtvcc_window *dw;
	unsigned int max_priority;
	unsigned int window_id;

	dw = NULL;
	max_priority = 8;

	for (window_id = 0; window_id < 8; ++window_id) {
		if (0 == (ds->created & (1 << window_id)))
			continue;
		if (!ds->window[window_id].visible)
			continue;
		if (DIR_BOTTOM_TOP
				!= ds->window[window_id].style.scroll_direction)
			continue;
		if (ds->window[window_id].priority < max_priority) {
			dw = &ds->window[window_id];
			max_priority = ds->window[window_id].priority;
		}
	}

	return dw;
}

static int atsc_dtvcc_color(int color)
{
	unsigned char red   = ((color >> 4) & 0x02);
	unsigned char green = ((color >> 2) & 0x02);
	unsigned char blue  = ((color) & 0x02);

	if ((0 == red) && (0 == green) && (0 == blue))
		return VBI_BLACK;
	else if ((2 == red) && (2 == green) && (2 == blue))
		return VBI_WHITE;
	else if ((2 == red) && (0 == green) && (0 == blue))
		return VBI_RED;
	else if ((0 == red) && (2 == green) && (0 == blue))
		return VBI_GREEN;
	else if ((0 == red) && (0 == green) && (2 == blue))
		return VBI_BLUE;
	else if ((2 == red) && (2 == green) && (0 == blue))
		return VBI_YELLOW;
	else if ((2 == red) && (0 == green) && (2 == blue))
		return VBI_MAGENTA;
	else if ((0 == red) && (2 == green) && (2 == blue))
		return VBI_CYAN;

	return VBI_BLACK;
}

static void atsc_dtvcc_position(cc_dtvcc_window *dw, unsigned int *row, unsigned int *column)
{
	unsigned int row_start, column_start;

	if (!dw->anchor_relative) {
		row_start    = (DTV_CC_MAX_ROWS * dw->anchor_vertical) / 75;
		column_start = (DTV_CC_MAX_COLUMNS * dw->anchor_horizontal) / 210;
	} else {
		row_start    = (DTV_CC_MAX_ROWS * dw->anchor_vertical) / 100;
		column_start = (DTV_CC_MAX_COLUMNS * dw->anchor_horizontal) / 100;
	}

	switch (dw->anchor_point) {
	case 0:
		row_start    = row_start;
		column_start = column_start;
		break;
	case 1:
		{
			unsigned int half_column = dw->column_count / 2;
			row_start    = row_start;
			column_start = (column_start > half_column) ? (column_start - half_column) : 0;
		}
		break;
	case 2:
		{
			unsigned int full_column = dw->column_count;
			row_start    = row_start;
			column_start = (column_start > full_column) ? (column_start - full_column) : 0;
		}
		break;
	case 3:
		{
			unsigned int half_row = dw->row_count / 2;
			row_start    = (row_start > half_row) ? (row_start - half_row) : 0;
			column_start = column_start;
		}
		break;
	case 4:
		{
			unsigned int half_row = dw->row_count / 2;
			unsigned int half_column = dw->column_count / 2;
			row_start    = (row_start > half_row) ? (row_start - half_row) : 0;
			column_start = (column_start > half_column) ? (column_start - half_column) : 0;
		}
		break;
	case 5:
		{
			unsigned int half_row = dw->row_count / 2;
			unsigned int full_column = dw->column_count;
			row_start    = (row_start > half_row) ? (row_start - half_row) : 0;
			column_start = (column_start > full_column) ? (column_start - full_column) : 0;
		}
	case 6:
		{
			unsigned int full_row = dw->row_count;
			row_start    = (row_start > full_row) ? (row_start - full_row) : 0;
			column_start = column_start;
		}
		break;
	case 7:
		{
			unsigned int full_row = dw->row_count;
			unsigned int half_column = dw->column_count / 2;
			row_start    = (row_start > full_row) ? (row_start - full_row) : 0;
			column_start = (column_start > half_column) ? (column_start - half_column) : 0;
		}
		break;
	case 8:
		{
			unsigned int full_row = dw->row_count;
			unsigned int full_column = dw->column_count;
			row_start    = (row_start > full_row) ? (row_start - full_row) : 0;
			column_start = (column_start > full_column) ? (column_start - full_column) : 0;
		}
		break;
	default:
		row_start    = row_start;
		column_start = column_start;
		break;
	}

	*row = row_start;
	*column = column_start;
	return;
}

static int atsc_dtvcc_display_event(cc_dtvcc_decoder *dc,
		cc_dtvcc_service *ds,
		cc_dtvcc_window *dw)
{
	unsigned int row, column;

	if (NULL == dw || dw != atsc_dtvcc_caption_window (ds))
		return FALSE;

	if (dc->ev) {
		vbi_page *page = av_mallocz(sizeof(vbi_page));
		ATSC_CC_DISPLAY disp;
		unsigned int row_start, column_start;

		memcpy(page->color_map, cc_rgba_map, sizeof(cc_rgba_map));
		page->rows    = MAX_VBI_CHAR_ROWS;
		page->columns = MAX_VBI_CHAR_COLUMNS;
		page->pgno    = atsc_dtvcc_service_num (dc, ds) + 8;
		atsc_dtvcc_position(dw, &row_start, &column_start);

		for (row = 0; row < DTV_CC_MAX_ROWS; row++) {
			for (column = 0; column < DTV_CC_MAX_COLUMNS; column++) {
				vbi_char *cp = &page->text[row * MAX_VBI_CHAR_COLUMNS + column];

				if ((row >= row_start) &&
						(row < (row_start + dw->row_count)) &&
						(column >= column_start) &&
						(column_start < (column_start + dw->column_count))) {
					unsigned int c = dw->buffer[row - row_start][column - column_start];

					cp->foreground = VBI_WHITE;
					cp->background = VBI_BLACK;
					cp->opacity = VBI_OPAQUE;
					if (c == 0) {
						cp->unicode = 0x20;
						cp->opacity = VBI_TRANSPARENT_SPACE;
					} else {
						cc_dtvcc_pen *curr_pen = &dw->pen[row - row_start][column - column_start];
						cp->unicode    = atsc_dtvcc_unicode(c);
						cp->italic     = curr_pen->style.italics;
						cp->underline  = curr_pen->style.underline;
						cp->foreground = atsc_dtvcc_color(curr_pen->style.fg_color);
						cp->background = atsc_dtvcc_color(curr_pen->style.bg_color);
					}
				}
			}
			if (debug_atsc_cc) {
				if ((row >= row_start) && (row < (row_start + dw->row_count))) {
					unsigned int j = 0;
					vbi_char *cp = &page->text[row * MAX_VBI_CHAR_COLUMNS + column_start];

					gxlogi("------> %d\n", row);
					gxlogi("unicode: ");
					for (j = 0; j < dw->column_count; j++)
						gxlogi("%c ", cp[j].unicode);
					gxlogi("\n");
					gxlogi("frcolor: ");
					for (j = 0; j < dw->column_count; j++)
						gxlogi("%d ", cp[j].foreground);
					gxlogi("\n");
					gxlogi("bgcolor: ");
					for (j = 0; j < dw->column_count; j++)
						gxlogi("%d ", cp[j].background);
					gxlogi("\n");
					gxlogi("opacity: ");
					for (j = 0; j < dw->column_count; j++)
						gxlogi("%d ", cp[j].opacity);
					gxlogi("\n");
				}
			}
		}

		disp.xr = disp.xl = 12;
		disp.yt = disp.yb = 10;
		dc->ev(page, dc->priv, &disp);
		av_free(page);
	}

	return TRUE;
}

static void atsc_dtvcc_stream_event(cc_dtvcc_decoder *dc,
		cc_dtvcc_service *ds,
		cc_dtvcc_window *dw,
		unsigned int row)
{
	vbi_char text[MAX_VBI_CHAR_COLUMNS];
	vbi_char ac;
	unsigned int column;

	if (NULL == dw || dw != atsc_dtvcc_caption_window (ds))
		return;

	if (debug_atsc_cc) {
		gxlogi("%s row=%u streamed=%08x\n", __FUNCTION__, row, dw->streamed);
	}

	/* Note we only stream windows with scroll direction
	   upwards. */
	if (0 != (dw->streamed & (1 << row)))
		return;

	dw->streamed |= 1 << row;

	for (column = 0; column < dw->column_count; ++column) {
		if (0 != dw->buffer[row][column])
			break;
	}

	/* Row contains only transparent spaces. */
	if (column >= dw->column_count)
		return;

	/* TO DO. */
	CLEAR (ac);
	ac.foreground = VBI_WHITE;
	ac.background = VBI_BLACK;
	ac.opacity = VBI_OPAQUE;

	for (column = 0; column < dw->column_count; ++column) {
		unsigned int c;

		c = dw->buffer[row][column];
		if (0 == c) {
			ac.unicode = 0x20;
		} else {
			ac.unicode = atsc_dtvcc_unicode (c);
			if (0 == ac.unicode) {
				ac.unicode = 0x20;
			}
		}
		text[column] = ac;
	}

	{
		cr_new_line ((cc_caption_recorder *)dc->cr,
				/* channel */ atsc_dtvcc_service_num (dc, ds) + 8,
				/* mode */ CC_MODE_ROLL_UP, /* FIXME */
				text, /* length */ dw->column_count);
	}
}

static vbi_bool atsc_dtvcc_put_char(cc_dtvcc_decoder *dc,
		cc_dtvcc_service *ds,
		unsigned int c)
{
	cc_dtvcc_window *dw;
	unsigned int row;
	unsigned int column;

	dc = dc; /* unused */

	dw = ds->curr_window;
	if (NULL == dw) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	row = dw->curr_row;
	column = dw->curr_column;

	/* FIXME how should we handle TEXT_TAG_NOT_DISPLAYABLE? */

	dw->buffer[row][column] = c;
	dw->pen[row][column] = dw->curr_pen;

	if (debug_atsc_cc) {
		gxlogi("%s row=%u/%u column=%u/%u, %c\n",
				__FUNCTION__,
				row, dw->row_count,
				column, dw->column_count, c);
	}

	switch (dw->style.print_direction) {
	case DIR_LEFT_RIGHT:
		dw->streamed &= ~(1 << row);
		if (++column >= dw->column_count)
			return TRUE;
		break;

	case DIR_RIGHT_LEFT:
		dw->streamed &= ~(1 << row);
		if (column-- <= 0)
			return TRUE;
		break;

	case DIR_TOP_BOTTOM:
		dw->streamed &= ~(1 << column);
		if (++row >= dw->row_count)
			return TRUE;
		break;

	case DIR_BOTTOM_TOP:
		dw->streamed &= ~(1 << column);
		if (row-- <= 0)
			return TRUE;
		break;
	}

	dw->curr_row = row;
	dw->curr_column = column;

	return TRUE;
}

static vbi_bool atsc_dtvcc_set_pen_location(cc_dtvcc_decoder *dc,
		cc_dtvcc_service *ds,
		const uint8_t *buf)
{
	cc_dtvcc_window *dw;
	unsigned int row;
	unsigned int column;

	dw = ds->curr_window;
	if (NULL == dw) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	row = buf[1];
	/* We check the top four zero bits. */
	if (row >= 16) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	column = buf[2];
	/* We also check the top two zero bits. */
	if (column >= 42) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	if (row > dw->row_count)
		row = dw->row_count - 1;
	if (column > dw->column_count)
		column = dw->column_count - 1;

	if (row != dw->curr_row) {
		atsc_dtvcc_stream_event (dc, ds, dw, dw->curr_row);
	}

	/* FIXME there's more. */
	dw->curr_row = row;
	dw->curr_column = column;

	return TRUE;
}

static vbi_bool atsc_dtvcc_set_pen_color(cc_dtvcc_service *ds, const uint8_t *buf)
{
	cc_dtvcc_window *dw;
	unsigned int c;

	dw = ds->curr_window;
	if (NULL == dw) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	c = buf[3];
	if (0 != (c & 0xC0)) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	dw->curr_pen.style.edge_color = c;
	c = buf[1];
	dw->curr_pen.style.fg_opacity = c >> 6;
	dw->curr_pen.style.fg_color = c & 0x3F;
	c = buf[2];
	dw->curr_pen.style.bg_opacity = c >> 6;
	dw->curr_pen.style.bg_color = c & 0x3F;

	if (debug_atsc_cc)
		gxlogi("font: %x %x\n",
				dw->curr_pen.style.fg_color, dw->curr_pen.style.bg_color);
	return TRUE;
}

static vbi_bool atsc_dtvcc_set_pen_attributes(cc_dtvcc_service *ds, const uint8_t *buf)
{
	cc_dtvcc_window *dw;
	unsigned int c;
	enum pen_size pen_size;
	enum offset offset;
	enum edge edge_type;

	dw = ds->curr_window;
	if (NULL == dw) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	c = buf[1];
	offset = (c >> 2) & 3;
	pen_size = c & 3;
	if ((offset | pen_size) >= 3) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	c = buf[2];
	edge_type = (c >> 3) & 7;
	if (edge_type >= 6) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	c = buf[1];
	dw->curr_pen.text_tag = c >> 4;
	dw->curr_pen.style.offset = offset;
	dw->curr_pen.style.pen_size = pen_size;
	c = buf[2];
	dw->curr_pen.style.italics = c >> 7;
	dw->curr_pen.style.underline = (c >> 6) & 1;
	dw->curr_pen.style.edge_type = edge_type;
	dw->curr_pen.style.font_style = c & 7;

	if (debug_atsc_cc)
		gxlogi("style: %d, %d\n",
				dw->curr_pen.style.italics, dw->curr_pen.style.underline);

	return TRUE;
}

static vbi_bool atsc_dtvcc_set_window_attributes(cc_dtvcc_service *ds, const uint8_t *buf)
{
	cc_dtvcc_window *dw;
	unsigned int c;
	enum edge border_type;
	enum display_effect display_effect;

	dw = ds->curr_window;
	if (NULL == dw)
		return FALSE;

	c = buf[2];
	border_type = ((buf[3] >> 5) & 0x04) | (c >> 6);
	if (border_type >= 6)
		return FALSE;

	c = buf[4];
	display_effect = c & 3;
	if (display_effect >= 3)
		return FALSE;

	c = buf[1];
	dw->style.fill_opacity = c >> 6;
	dw->style.fill_color = c & 0x3F;
	c = buf[2];
	dw->style.border_type = border_type;
	dw->style.border_color = c & 0x3F;
	c = buf[3];
	dw->style.wordwrap = (c >> 6) & 1;
	dw->style.print_direction = (c >> 4) & 3;
	dw->style.scroll_direction = (c >> 2) & 3;
	dw->style.justify = c & 3;
	c = buf[4];
	dw->style.effect_speed = c >> 4;
	dw->style.effect_direction = (c >> 2) & 3;
	dw->style.display_effect = display_effect;

	return TRUE;
}

static vbi_bool atsc_dtvcc_clear_windows(cc_dtvcc_decoder *dc,
		cc_dtvcc_service *ds,
		cc_dtvcc_window_map window_map)
{
	unsigned int i;

	window_map &= ds->created;

	for (i = 0; i < 8; ++i) {
		cc_dtvcc_window *dw;

		if (0 == (window_map & (1 << i)))
			continue;

		dw = &ds->window[i];

		atsc_dtvcc_stream_event (dc, ds, dw, dw->curr_row);

		memset (dw->buffer, 0, sizeof (dw->buffer));

		dw->streamed = 0;

		/* FIXME CEA 708-C Section 7.1.4 (Form Feed)
		   and 8.10.5.3 confuse me. */
		if (0) {
			dw->curr_column = 0;
			dw->curr_row = 0;
		}
	}

	return TRUE;
}

static vbi_bool atsc_dtvcc_define_window(cc_dtvcc_decoder *dc,
		cc_dtvcc_service *ds,
		uint8_t *buf)
{
	static const cc_dtvcc_window_style window_styles [7] = {
		{
			JUSTIFY_LEFT, DIR_LEFT_RIGHT, DIR_BOTTOM_TOP,
			FALSE, DISPLAY_EFFECT_SNAP, 0, 0, 0,
			OPACITY_SOLID, EDGE_NONE, 0
		}, {
			JUSTIFY_LEFT, DIR_LEFT_RIGHT, DIR_BOTTOM_TOP,
			FALSE, DISPLAY_EFFECT_SNAP, 0, 0, 0,
			OPACITY_TRANSPARENT, EDGE_NONE, 0
		}, {
			JUSTIFY_CENTER, DIR_LEFT_RIGHT, DIR_BOTTOM_TOP,
			FALSE, DISPLAY_EFFECT_SNAP, 0, 0, 0,
			OPACITY_SOLID, EDGE_NONE, 0
		}, {
			JUSTIFY_LEFT, DIR_LEFT_RIGHT, DIR_BOTTOM_TOP,
			TRUE, DISPLAY_EFFECT_SNAP, 0, 0, 0,
			OPACITY_SOLID, EDGE_NONE, 0
		}, {
			JUSTIFY_LEFT, DIR_LEFT_RIGHT, DIR_BOTTOM_TOP,
			TRUE, DISPLAY_EFFECT_SNAP, 0, 0, 0,
			OPACITY_TRANSPARENT, EDGE_NONE, 0
		}, {
			JUSTIFY_CENTER, DIR_LEFT_RIGHT, DIR_BOTTOM_TOP,
				TRUE, DISPLAY_EFFECT_SNAP, 0, 0, 0,
				OPACITY_SOLID, EDGE_NONE, 0
		}, {
			JUSTIFY_LEFT, DIR_TOP_BOTTOM, DIR_RIGHT_LEFT,
				FALSE, DISPLAY_EFFECT_SNAP, 0, 0, 0,
				OPACITY_SOLID, EDGE_NONE, 0
		}
	};
	static const cc_dtvcc_pen_style pen_styles [7] = {
		{
			PEN_SIZE_STANDARD, 0, OFFSET_NORMAL, FALSE,
			FALSE, EDGE_NONE, 0x3F, OPACITY_SOLID,
			0x00, OPACITY_SOLID, 0
		}, {
			PEN_SIZE_STANDARD, 1, OFFSET_NORMAL, FALSE,
			FALSE, EDGE_NONE, 0x3F, OPACITY_SOLID,
			0x00, OPACITY_SOLID, 0
		}, {
			PEN_SIZE_STANDARD, 2, OFFSET_NORMAL, FALSE,
			FALSE, EDGE_NONE, 0x3F, OPACITY_SOLID,
			0x00, OPACITY_SOLID, 0
		}, {
			PEN_SIZE_STANDARD, 3, OFFSET_NORMAL, FALSE,
			FALSE, EDGE_NONE, 0x3F, OPACITY_SOLID,
			0x00, OPACITY_SOLID, 0
		}, {
			PEN_SIZE_STANDARD, 4, OFFSET_NORMAL, FALSE,
			FALSE, EDGE_NONE, 0x3F, OPACITY_SOLID,
			0x00, OPACITY_SOLID, 0
		}, {
			PEN_SIZE_STANDARD, 3, OFFSET_NORMAL, FALSE,
				FALSE, EDGE_UNIFORM, 0x3F, OPACITY_SOLID,
				0, OPACITY_TRANSPARENT, 0x00
		}, {
			PEN_SIZE_STANDARD, 4, OFFSET_NORMAL, FALSE,
				FALSE, EDGE_UNIFORM, 0x3F, OPACITY_SOLID,
				0, OPACITY_TRANSPARENT, 0x00
		}
	};
	cc_dtvcc_window *dw;
	cc_dtvcc_window_map window_map;
	vbi_bool anchor_relative;
	unsigned int anchor_vertical;
	unsigned int anchor_horizontal;
	unsigned int anchor_point;
	unsigned int column_count_m1;
	unsigned int window_id;
	unsigned int window_style_id;
	unsigned int pen_style_id;
	unsigned int c;

	if (0 != ((buf[1] | buf[6]) & 0xC0)) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	c = buf[2];
	anchor_relative = (c >> 7) & 1;
	anchor_vertical = c & 0x7F;
	anchor_horizontal = buf[3];
	if (0 == anchor_relative) {
		if ((anchor_vertical >= 75
					|| anchor_horizontal >= 210)) {
			ds->error_line = __LINE__;
			return FALSE;
		}
	} else {
		if ((anchor_vertical >= 100
					|| anchor_horizontal >= 100)) {
			ds->error_line = __LINE__;
			return FALSE;
		}
	}

	c = buf[4];
	anchor_point = c >> 4;
	if ((anchor_point >= 9)) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	column_count_m1 = buf[5] & 0x3f;
	/* We also check the top two zero bits. */
	if ((column_count_m1 > MAX_VBI_CHAR_COLUMNS)) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	window_id = buf[0] & 7;
	dw = &ds->window[window_id];
	window_map = 1 << window_id;

	ds->curr_window = dw;

	c = buf[1];
	dw->visible = (c >> 5) & 1;
	dw->row_lock = (c >> 4) & 1;
	dw->column_lock = (c >> 4) & 1;
	dw->priority = c & 7;

	dw->anchor_relative = anchor_relative;
	dw->anchor_vertical = anchor_vertical;
	dw->anchor_horizontal = anchor_horizontal;
	dw->anchor_point = anchor_point;

	c = buf[4];
	dw->row_count = (c & 15) + 1;
	dw->column_count = column_count_m1 + 1;

	c = buf[6];
	window_style_id = (c >> 3) & 7;
	pen_style_id = c & 7;

	if (window_style_id > 0) {
		dw->style = window_styles[window_style_id];
	} else if (0 == (ds->created & window_map)) {
		dw->style = window_styles[1];
	}

	if (pen_style_id > 0) {
		dw->curr_pen.style = pen_styles[pen_style_id];
	} else if (0 == (ds->created & window_map)) {
		dw->curr_pen.style = pen_styles[1];
	}

	if (0 != (ds->created & window_map))
		return TRUE;

	/* Has to be something, no? */
	dw->curr_pen.text_tag = TEXT_TAG_NOT_DISPLAYABLE;

	dw->curr_column = 0;
	dw->curr_row = 0;

	dw->streamed = 0;

	ds->created |= window_map;

	return atsc_dtvcc_clear_windows (dc, ds, window_map);
}

static vbi_bool atsc_dtvcc_display_windows(cc_dtvcc_decoder *dc,
		cc_dtvcc_service *ds,
		unsigned int     c,
		cc_dtvcc_window_map window_map)
{
	unsigned int i;

	window_map &= ds->created;

	for (i = 0; i < 8; ++i) {
		cc_dtvcc_window *dw;
		vbi_bool was_visible;

		if (0 == (window_map & (1 << i)))
			continue;

		dw = &ds->window[i];
		was_visible = dw->visible;

		switch (c) {
		case 0x89: /* DSW DisplayWindows */
			dw->visible = TRUE;
			break;

		case 0x8A: /* HDW HideWindows */
			dw->visible = FALSE;
			break;

		case 0x8B: /* TGW ToggleWindows */
			dw->visible = was_visible ^ TRUE;
			break;
		}

		if (!was_visible) {
			unsigned int row;

			for (row = 0; row < dw->row_count; ++row) {
				atsc_dtvcc_stream_event (dc, ds, dw, row);
			}

			atsc_dtvcc_display_event(dc,ds,dw);
		}
	}

	return TRUE;
}

static vbi_bool atsc_dtvcc_carriage_return(cc_dtvcc_decoder *dc, cc_dtvcc_service *ds)
{
	cc_dtvcc_window *dw;
	unsigned int row;
	unsigned int column;

	dw = ds->curr_window;
	if (NULL == dw) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	atsc_dtvcc_stream_event (dc, ds, dw, dw->curr_row);
	atsc_dtvcc_display_event(dc,ds,dw);

	row = dw->curr_row;
	column = dw->curr_column;

	switch (dw->style.scroll_direction) {
	case DIR_LEFT_RIGHT:
		dw->curr_row = 0;
		if (column > 0) {
			dw->curr_column = column - 1;
			break;
		}
		dw->streamed = (dw->streamed << 1)
			& ~(1 << dw->column_count);
		for (row = 0; row < dw->row_count; ++row) {
			for (column = dw->column_count - 1;
					column > 0; --column) {
				dw->buffer[row][column] =
					dw->buffer[row][column - 1];
			}
			dw->buffer[row][column] = 0;
		}
		break;

	case DIR_RIGHT_LEFT:
		dw->curr_row = 0;
		if (column + 1 < dw->row_count) {
			dw->curr_column = column + 1;
			break;
		}
		dw->streamed >>= 1;
		for (row = 0; row < dw->row_count; ++row) {
			for (column = 0;
					column < dw->column_count - 1; ++column) {
				dw->buffer[row][column] =
					dw->buffer[row][column + 1];
			}
			dw->buffer[row][column] = 0;
		}
		break;

	case DIR_TOP_BOTTOM:
		dw->curr_column = 0;
		if (row > 0) {
			dw->curr_row = row - 1;
			break;
		}
		dw->streamed = (dw->streamed << 1)
			& ~(1 << dw->row_count);
		memmove (&dw->buffer[1], &dw->buffer[0],
				sizeof (dw->buffer[0]) * (dw->row_count - 1));
		memset (&dw->buffer[0], 0, sizeof (dw->buffer[0]));
		break;

	case DIR_BOTTOM_TOP:
		dw->curr_column = 0;
		if (row + 1 < dw->row_count) {
			dw->curr_row = row + 1;
			break;
		}
		dw->streamed >>= 1;
		memmove (&dw->buffer[0], &dw->buffer[1],
				sizeof (dw->buffer[0]) * (dw->row_count - 1));
		memset (&dw->buffer[row], 0, sizeof (dw->buffer[0]));
		break;
	}

	return TRUE;
}

static vbi_bool atsc_dtvcc_form_feed(cc_dtvcc_decoder *dc, cc_dtvcc_service *ds)
{
	cc_dtvcc_window *dw;
	cc_dtvcc_window_map window_map;

	dw = ds->curr_window;
	if (NULL == dw) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	window_map = 1 << atsc_dtvcc_window_id (ds, dw);

	if (!atsc_dtvcc_clear_windows (dc, ds, window_map))
		return FALSE;

	dw->curr_row = 0;
	dw->curr_column = 0;

	return TRUE;
}

static vbi_bool atsc_dtvcc_backspace(cc_dtvcc_decoder *dc, cc_dtvcc_service *ds)
{
	cc_dtvcc_window *dw;
	unsigned int row;
	unsigned int column;
	unsigned int mask;

	dc = dc; /* unused */

	dw = ds->curr_window;
	if (NULL == dw) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	row = dw->curr_row;
	column = dw->curr_column;

	switch (dw->style.print_direction) {
	case DIR_LEFT_RIGHT:
		mask = 1 << row;
		if (column-- <= 0)
			return TRUE;
		break;

	case DIR_RIGHT_LEFT:
		mask = 1 << row;
		if (++column >= dw->column_count)
			return TRUE;
		break;

	case DIR_TOP_BOTTOM:
		mask = 1 << column;
		if (row-- <= 0)
			return TRUE;
		break;

	case DIR_BOTTOM_TOP:
		mask = 1 << column;
		if (++row >= dw->row_count)
			return TRUE;
		break;
	}

	if (0 != dw->buffer[row][column]) {
		dw->streamed &= ~mask;
		dw->buffer[row][column] = 0;
	}

	dw->curr_row = row;
	dw->curr_column = column;

	return TRUE;
}

static int atsc_dtvcc_decode_p16(cc_dtvcc_decoder *dc, cc_dtvcc_service *ds, uint16_t ucs2)
{
	cc_dtvcc_window *dw;
	uint8_t out[4] = {'?', 0, 0, 0};

	dw = ds->curr_window;
	if (NULL == dw) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	if (ucs2 <= 0x7F) {
		out[0] = ucs2;
	} else if (ucs2 <= 0x7FF) {
		out[0] = 0xC0 | (ucs2 >> 6);
		out[1] = 0x80 | (ucs2 & 0x3F);
	} else {
		out[0] = 0xE0 | (ucs2 >> 12);
		out[1] = 0x80 | ((ucs2 >> 6) & 0x3F);
		out[2] = 0x80 | (ucs2 & 0x3F);
	}

	atsc_dtvcc_put_char(dc, ds, ((out[3]<<24)|(out[2]<<16)|(out[1]<<8)|out[0]));

	return TRUE;
}

static vbi_bool atsc_dtvcc_hor_carriage_return(cc_dtvcc_decoder *dc, cc_dtvcc_service *ds)
{
	cc_dtvcc_window *dw;
	unsigned int row;
	unsigned int column;
	unsigned int mask;

	dc = dc; /* unused */

	dw = ds->curr_window;
	if (NULL == dw) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	row = dw->curr_row;
	column = dw->curr_column;

	switch (dw->style.print_direction) {
	case DIR_LEFT_RIGHT:
	case DIR_RIGHT_LEFT:
		mask = 1 << row;
		memset (&dw->buffer[row][0], 0,
				sizeof (dw->buffer[0]));
		if (DIR_LEFT_RIGHT == dw->style.print_direction)
			dw->curr_column = 0;
		else
			dw->curr_column = dw->column_count - 1;
		break;

	case DIR_TOP_BOTTOM:
	case DIR_BOTTOM_TOP:
		mask = 1 << column;
		for (row = 0; row < dw->column_count; ++row)
			dw->buffer[row][column] = 0;
		if (DIR_TOP_BOTTOM == dw->style.print_direction)
			dw->curr_row = 0;
		else
			dw->curr_row = dw->row_count - 1;
		break;
	}

	dw->streamed &= ~mask;

	return TRUE;
}

static vbi_bool atsc_dtvcc_delete_windows(cc_dtvcc_decoder *dc,
		cc_dtvcc_service *ds,
		cc_dtvcc_window_map window_map)
{
	cc_dtvcc_window *dw;

	dw = ds->curr_window;
	if (NULL != dw) {
		unsigned int window_id;

		window_id = atsc_dtvcc_window_id (ds, dw);
		if (0 != (window_map & (1 << window_id))) {
			atsc_dtvcc_stream_event (dc, ds, dw, dw->curr_row);
			atsc_dtvcc_display_event(dc,ds,dw);
			ds->curr_window = NULL;
		}
	}

	ds->created &= ~window_map;

	return TRUE;
}

static void atsc_dtvcc_reset_service(cc_dtvcc_service *ds)
{
	ds->curr_window = NULL;
	ds->created = 0;
}

static vbi_bool atsc_dtvcc_command(cc_dtvcc_decoder *dc,
		cc_dtvcc_service *ds,
		unsigned int *se_length,
		uint8_t *buf,
		unsigned int n_bytes)
{
	unsigned int c;
	unsigned int window_id;

	c = buf[0];
	if ((int8_t) c < 0) {
		*se_length = dtvcc_c1_length[c - 0x80];
	} else {
		*se_length = dtvcc_c0_length[c >> 3];
	}

	if (*se_length > n_bytes) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	switch (c) {
	case 0x18:
		return atsc_dtvcc_decode_p16(dc, ds, ((buf[1]<<8)|buf[2]));

	case 0x08: /* BS Backspace */
		return atsc_dtvcc_backspace (dc, ds);

	case 0x0C: /* FF Form Feed */
		return atsc_dtvcc_form_feed (dc, ds);

	case 0x0D: /* CR Carriage Return */
		return atsc_dtvcc_carriage_return (dc, ds);

	case 0x0E: /* HCR Horizontal Carriage Return */
		return atsc_dtvcc_hor_carriage_return (dc, ds);

	case 0x80 ... 0x87: /* CWx SetCurrentWindow */
		window_id = c & 7;
		if (0 == (ds->created & (1 << window_id))) {
			ds->error_line = __LINE__;
			return FALSE;
		}
		ds->curr_window = &ds->window[window_id];
		return TRUE;

	case 0x88: /* CLW ClearWindows */
		return atsc_dtvcc_clear_windows (dc, ds, buf[1]);

	case 0x89: /* DSW DisplayWindows */
		return atsc_dtvcc_display_windows (dc, ds, c, buf[1]);

	case 0x8A: /* HDW HideWindows */
		return atsc_dtvcc_display_windows (dc, ds, c, buf[1]);

	case 0x8B: /* TGW ToggleWindows */
		return atsc_dtvcc_display_windows (dc, ds, c, buf[1]);

	case 0x8C: /* DLW DeleteWindows */
		return atsc_dtvcc_delete_windows (dc, ds, buf[1]);

	case 0x8F: /* RST Reset */
		atsc_dtvcc_reset_service (ds);
		return TRUE;

	case 0x90: /* SPA SetPenAttributes */
		return atsc_dtvcc_set_pen_attributes (ds, buf);

	case 0x91: /* SPC SetPenColor */
		return atsc_dtvcc_set_pen_color (ds, buf);

	case 0x92: /* SPL SetPenLocation */
		return atsc_dtvcc_set_pen_location (dc, ds, buf);

	case 0x97: /* SWA SetWindowAttributes */
		return atsc_dtvcc_set_window_attributes (ds, buf);

	case 0x98 ... 0x9F: /* DFx DefineWindow */
		return atsc_dtvcc_define_window (dc, ds, buf);

	default:
		return TRUE;
	}
}

static void atsc_dtvcc_reset(cc_dtvcc_decoder *dc)
{
	int i = 0;

	for(i = 0; i < MAX_CC_SERVICE; i++) {
		if(dc->service[i]) {
			atsc_dtvcc_reset_service(dc->service[i]);
		}
	}
	dc->packet_size = 0;
	dc->next_sequence_number = -1;
}

static vbi_bool atsc_dtvcc_decode_se(cc_dtvcc_decoder *dc,
		cc_dtvcc_service *ds,
		unsigned int *se_length,
		uint8_t *buf,
		unsigned int n_bytes)
{
	unsigned int c;

	c = buf[0];
	if (0 != (c & 0x60)) {
		/* G0/G1 character. */
		*se_length = 1;
		return atsc_dtvcc_put_char (dc, ds, c);
	}

	if (0x10 != c) {
		/* C0/C1 control code. */
		return atsc_dtvcc_command (dc, ds, se_length,
				buf, n_bytes);
	}

	if (n_bytes < 2) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	c = buf[1];
	if (0 != (c & 0x60)) {
		/* G2/G3 character. */
		*se_length = 2;
		return atsc_dtvcc_put_char (dc, ds, 0x1000 | c);
	}

	/* CEA 708-C defines no C2 or C3 commands. */

	if ((int8_t) c >= 0) {
		/* C2 code. */
		*se_length = (c >> 3) + 2;
	} else if (c < 0x90) {
		/* C3 Fixed Length Commands. */
		*se_length = (c >> 3) - 10;
	} else {
		/* C3 Variable Length Commands. */

		if (n_bytes < 3) {
			ds->error_line = __LINE__;
			return FALSE;
		}

		/* type [2], zero_bit [1],
		   length [5] */
		*se_length = (buf[2] & 0x1F) + 3;
	}

	if (n_bytes < *se_length) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	return TRUE;
}

static vbi_bool atsc_dtvcc_decode_syntactic_elements(cc_dtvcc_decoder *dc,
		cc_dtvcc_service *ds,
		uint8_t *buf,
		unsigned int n_bytes)
{
	while (n_bytes > 0) {
		unsigned int se_length;

		if (0x8D /* DLY */ == *buf
				|| 0x8E /* DLC */ == *buf) {
			/* FIXME ignored for now. */
			++buf;
			--n_bytes;
			continue;
		}

		if (!atsc_dtvcc_decode_se (dc, ds,
					&se_length,
					buf, n_bytes)) {
			return FALSE;
		}

		buf += se_length;
		n_bytes -= se_length;
	}

	return TRUE;
}
static void atsc_dtvcc_decode_packet(cc_dtvcc_decoder *dc)
{
	unsigned int packet_size_code;
	unsigned int packet_size;
	unsigned int i;

	/* Packet Layer. */

	/* sequence_number [2], packet_size_code [6],
	   packet_data [n * 8] */

	if (dc->next_sequence_number >= 0
			&& 0 != ((dc->packet[0] ^ dc->next_sequence_number) & 0xC0)) {
		atsc_dtvcc_reset (dc);
		return;
	}

	dc->next_sequence_number = dc->packet[0] + 0x40;

	packet_size_code = dc->packet[0] & 0x3F;
	packet_size = 128;
	if (packet_size_code > 0)
		packet_size = packet_size_code * 2;

	if (debug_atsc_cc) {
		unsigned int sequence_number;

		sequence_number = (dc->packet[0] >> 6) & 3;
		gxlogi("DTVCC packet packet_size=%u "
				"(transmitted %u), sequence_number %u\n",
				packet_size, dc->packet_size,
				sequence_number);
	}

	/* CEA 708-C Section 5: Apparently packet_size need not be
	   equal to the actually transmitted amount of data. */
	if (packet_size > dc->packet_size) {
		atsc_dtvcc_reset (dc);
		return;
	}

	/* Service Layer. */

	/* CEA 708-C Section 6.2.5, 6.3: Service Blocks and syntactic
	   elements must not cross Caption Channel Packet
	   boundaries. */

	for (i = 1; i < packet_size;) {
		unsigned int service_number;
		unsigned int block_size;
		unsigned int header_size;
		unsigned int c;

		header_size = 1;

		/* service_number [3], block_size [5],
		   (null_fill [2], extended_service_number [6]),
		   (Block_data [n * 8]) */

		c = dc->packet[i]; 
		service_number = (c & 0xE0) >> 5;

		/* CEA 708-C Section 6.3: Ignore block_size if
		   service_number is zero. */
		if (0 == service_number) {
			/* NULL Service Block Header, no more data in
			   this Caption Channel Packet. */
			break;
		}

		/* CEA 708-C Section 6.2.1: Apparently block_size zero
		   is valid, although properly it should only occur in
		   NULL Service Block Headers. */
		block_size = c & 0x1F;

		if (7 == service_number) {
			if (i + 1 > packet_size)
				goto service_block_incomplete;

			header_size = 2;
			c = dc->packet[i + 1];

			/* We also check the null_fill bits. */
			if (c < 7 || c > 63)
				goto invalid_service_block;

			service_number = c;
		}

		if (i + header_size + block_size > packet_size)
			goto service_block_incomplete;

		if (service_number <= MAX_CC_SERVICE) {
			cc_dtvcc_service *ds;
			unsigned int in;

			if(!dc->service[service_number - 1]) {
				cc_dtvcc_service *service_1 = av_mallocz(sizeof(cc_dtvcc_service));
				atsc_dtvcc_reset_service (service_1);
				dc->service[service_number - 1] = service_1;
			}

			ds = dc->service[service_number - 1];
			in = ds->service_data_in;
			memcpy (ds->service_data + in,
					dc->packet + i + header_size,
					block_size);
			ds->service_data_in = in + block_size;
		}

		i += header_size + block_size;
	}

	for (i = 0; i < MAX_CC_SERVICE; ++i) {
		cc_dtvcc_service *ds;
		vbi_bool success;

		if (!dc->service[i])
			continue;

		ds = dc->service[i];
		if (0 == ds->service_data_in)
			continue;

		success = atsc_dtvcc_decode_syntactic_elements
			(dc, ds, ds->service_data, ds->service_data_in);

		ds->service_data_in = 0;

		if (success)
			continue;

		if (debug_atsc_cc) {
			gxlogi("Packet (%d/%d):\n", packet_size, dc->packet_size);
			gxlogi("Service Data:\n");
		}

		atsc_dtvcc_reset_service (ds);
	}

	return;

invalid_service_block:
	{
		if (debug_atsc_cc) {
			gxlogi("Packet (%d/%d):\n", packet_size, dc->packet_size);
		}
		atsc_dtvcc_reset (dc);
		return;
	}

service_block_incomplete:
	{
		if (debug_atsc_cc) {
			gxlogi("Packet (%d/%d):\n", packet_size, dc->packet_size);
		}
		atsc_dtvcc_reset (dc);
		return;
	}
}

static void atsc_cc_decode(cc_context *ctx, unsigned char *buf, unsigned int len)
{
	unsigned int cc_flag;
	unsigned int cc_count;
	unsigned int i;
	vbi_bool dtvcc;

	if (NULL == buf || len < 10)
		return;

	cc_flag = buf[9] & 0x40;
	if (!cc_flag)
		return;

	cc_count = buf[9] & 0x1F;
	dtvcc = FALSE;

	for (i = 0; i < cc_count; ++i) {
		unsigned int b0;
		unsigned int cc_valid;
		enum cc_type cc_type;
		unsigned int cc_data_1;
		unsigned int cc_data_2;
		unsigned int j;

		b0 = buf[11 + i * 3];
		cc_valid = b0 & 4;
		cc_type = (enum cc_type)(b0 & 3);
		cc_data_1 = buf[12 + i * 3];
		cc_data_2 = buf[13 + i * 3];

		switch (cc_type) {
		case NTSC_F1:
		case NTSC_F2:
			/* Note CEA 708-C Table 4: Only one NTSC pair
			   will be present in field picture user_data
			   or in progressive video pictures, and up to
			   three can occur if the frame rate < 30 Hz
			   or repeat_first_field = 1. */
			if (!cc_valid || i >= 3 || dtvcc) {
				/* Illegal, invalid or filler. */
				break;
			}

			atsc_cc_feed (&ctx->cr.cc, &buf[12 + i * 3],
					/* line */ (NTSC_F1 == cc_type) ? 21 : 284);

			/* XXX replace me. */
			if (NTSC_F1 == cc_type) {
				ctx->cr.field = 0;
				if (ctx->cr.usexds) /* fields swapped? */
					atsc_xds_decode(&ctx->cr, cc_data_1 + cc_data_2 * 256);
			} else {
				ctx->cr.field = 1;
				if (ctx->cr.usexds)
					atsc_xds_decode(&ctx->cr, cc_data_1 + cc_data_2 * 256);
			}
			break;

		case DTVCC_DATA:
			j = ctx->cr.dtvcc.packet_size;
			if (j <= 0) {
				/* Missed packet start. */
				break;
			} else if (!cc_valid) {
				/* End of DTVCC packet. */
				atsc_dtvcc_decode_packet (&ctx->cr.dtvcc);
				ctx->cr.dtvcc.packet_size = 0;
			} else if (j >= 128) {
				/* Packet buffer overflow. */
				atsc_dtvcc_reset (&ctx->cr.dtvcc);
				ctx->cr.dtvcc.packet_size = 0;
			} else {
				ctx->cr.dtvcc.packet[j] = cc_data_1;
				ctx->cr.dtvcc.packet[j + 1] = cc_data_2;
				ctx->cr.dtvcc.packet_size = j + 2;
			}
			break;

		case DTVCC_START:
			dtvcc = TRUE;
			j = ctx->cr.dtvcc.packet_size;
			if (j > 0) {
				/* End of DTVCC packet. */
				atsc_dtvcc_decode_packet (&ctx->cr.dtvcc);
			}
			if (!cc_valid) {
				/* No new data. */
				ctx->cr.dtvcc.packet_size = 0;
			} else {
				ctx->cr.dtvcc.packet[0] = cc_data_1;
				ctx->cr.dtvcc.packet[1] = cc_data_2;
				ctx->cr.dtvcc.packet_size = 2;
			}
			break;
		}
	}
}

static void atsc_cc_reset(cc_decoder *cd)
{
	unsigned int ch_num;

	for (ch_num = 0; ch_num < MAX_CC_CHANNELS; ++ch_num) {
		cc_channel *ch;

		ch = &cd->channel[ch_num];

		if (ch_num <= 3) {
			ch->mode = CC_MODE_UNKNOWN;

			/* Something suitable for roll-up mode. */
			ch->curr_row = CC_LAST_ROW;
			ch->curr_column = CC_FIRST_COLUMN;
			ch->window_rows = 4;
		} else {
			ch->mode = CC_MODE_TEXT; /* invariable */

			/* EIA 608-B Section 7.4: "When Text Mode has
			   initially been selected and the specified
			   Text memory is empty, the cursor starts at
			   the topmost row, Column 1." */
			ch->curr_row = CC_FIRST_ROW;
			ch->curr_column = CC_FIRST_COLUMN;
			ch->window_rows = 0; /* n/a */
		}

		ch->displayed_buffer = 0;

		ch->last_pac = 0;

		CLEAR (ch->buffer);
		CLEAR (ch->dirty);
	}

	cd->curr_ch_num[0] = UNKNOWN_CC_CHANNEL;
	cd->curr_ch_num[1] = UNKNOWN_CC_CHANNEL;

	memset (cd->expect_ctrl, -1, sizeof (cd->expect_ctrl));

	CLEAR (cd->in_xds);

	cd->event_pending = NULL;
}

static void init_cc_data_decoder(cc_data_decoder *cd)
{
	CLEAR (*cd);
}

static void init_cc_decoder(cc_decoder *cd)
{
	atsc_cc_reset (cd);

	cd->error_history = 0;
}

static void init_dtvcc_decoder(cc_dtvcc_decoder *dc)
{
	atsc_dtvcc_reset (dc);
}

static void init_caption_recorder(cc_caption_recorder *cr)
{
	CLEAR (*cr);

	cr->option_xds_output_file_name = "-";

	init_cc_data_decoder (&cr->cd);
	init_cc_decoder (&cr->cc);
	init_dtvcc_decoder (&cr->dtvcc);

	cr->infoptr = cr->newinfo[0][0][0];
	cr->xds_info_prefix = "\33[33m% ";
	cr->xds_info_suffix = "\33[0m\n";
	cr->usewebtv = 1;
	cr->cc.cr = (void*)cr;
	cr->dtvcc.cr = (void*)cr;

	memset (cr->minicut_min, -1, sizeof (cr->minicut_min));
}

handle_t AtscccCreate(void)
{
	cc_context *ctx = av_mallocz(sizeof(cc_context));

	init_caption_recorder(&ctx->cr);

	return (handle_t)ctx;
}

void AtscccDestory(handle_t handle)
{
	int i = 0;
	cc_context *ctx = (cc_context *)handle;

	if (ctx->cr.ucs_buffer)
		av_free(ctx->cr.ucs_buffer);

	for(i = 0; i < MAX_CC_SERVICE; i++) {
		if (ctx->cr.dtvcc.service[i]) {
			av_free(ctx->cr.dtvcc.service[i]);
		}
	}

	av_free(ctx);
}

int AtscccDecode(handle_t handle, unsigned char *buf, unsigned int len)
{
	cc_context *ctx = (cc_context *)handle;
	int pos = 0;

	if ((NULL == buf) || (0 == len)) return -1;

	if ((len + ctx->cc_len) <= CC_UNIT_LENGTH) {
		memcpy(ctx->cc_data + ctx->cc_len + 4, buf, len);
		ctx->cc_len += len;
	} else {
		gxlogd("warning: %s %d (bytes %d)\n", __FUNCTION__, __LINE__, len);
	}

	while (ctx->cc_len > 5) {
		for (pos = 0; pos < (ctx->cc_len - 4); pos++) {
			unsigned char *ptr = ctx->cc_data + (CC_UNIT_OFFSET + pos);

			if (0x47413934 == ((ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3])) {
				if (0x03 == ptr[4]) {
					break;
				}
			}
		}

		if (0 != pos) {
			memmove(ctx->cc_data + CC_UNIT_OFFSET,
					ctx->cc_data + (CC_UNIT_OFFSET + pos), ctx->cc_len - pos);
			ctx->cc_len -= pos;
		}

		if (ctx->cc_len > 5) {
			unsigned char *ptr = ctx->cc_data;
			unsigned int   len = (3 * (ptr[9] & 0x1F) + 11);

			if (len > (ctx->cc_len + CC_UNIT_OFFSET))
				break;

			atsc_cc_decode(ctx, ptr, len);
			len -= CC_UNIT_OFFSET;
			memmove(ctx->cc_data + CC_UNIT_OFFSET,
					ctx->cc_data + (CC_UNIT_OFFSET + len), ctx->cc_len - len);
			ctx->cc_len -= len;
		}
	}

	return 0;
}

void AtscccRegisterEvent(handle_t handle, AtscccEvent ev, void *priv)
{
	cc_context *ctx = (cc_context *)handle;

	if (!ctx)
		return;

	ctx->cr.cc.ev   = ev;
	ctx->cr.cc.priv = priv;
	ctx->cr.dtvcc.ev   = ev;
	ctx->cr.dtvcc.priv = priv;
}

int AtscccGetSubInfo(handle_t handle, GX_SUBTITLE_ATSCCC_INFO *info)
{
	cc_context *ctx = (cc_context *)handle;

	if (!ctx) return -1;

	return 0;
}
