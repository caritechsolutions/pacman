#ifndef __GX_PIXEL_SUBTITLE_H__
#define __GX_PIXEL_SUBTITLE_H__

typedef enum {
	GX_PIXEL_RENDER_SPP,
	GX_PIXEL_RENDER_OSD,
} GxPixelSubtitleRender;

typedef enum {
	GX_PIXEL_SUB_ENC_UTF8     ,
	GX_PIXEL_SUB_ENC_ANSI     ,
	GX_PIXEL_SUB_ENC_UCS      ,
	GX_PIXEL_SUB_ENC_SPU
} GxPixelSubtitleEncode;

typedef struct {
	unsigned int        x;
	unsigned int        y;
	unsigned int        width;
	unsigned int        height;
} GxPixelSubtitleWindow;

typedef struct gx_pixel_sub_packet {
	unsigned char *packet;
	unsigned int  palette[4];
	unsigned int  alpha[4];
	unsigned int  control_start;	  /* index of start of control data */
	unsigned int  current_nibble[2];  /* next data nibble (4 bits) to be
									    processed (for RLE decoding) for
									    even and odd lines */
	int          deinterlace_oddness; /* 0 or 1, index into current_nibble */
	unsigned int start_col, end_col;
	unsigned int start_row, end_row;
	unsigned int width, height, stride;
	unsigned int start_pts, end_pts;
	struct gx_pixel_sub_packet *next;
} GxPixelSubtitlePacket;

typedef struct {
	GxPixelSubtitlePacket *queue_head;
	GxPixelSubtitlePacket *queue_tail;
	unsigned int        global_palette[16];
	unsigned int        orig_frame_width, orig_frame_height;
	unsigned char       *packet;
	size_t              packet_reserve;	/* size of the memory pointed to by packet */
	unsigned int        packet_offset;	/* end of the currently assembled fragment */
	unsigned int        packet_size;	/* size of the packet once all fragments are assembled */
	int                 packet_pts;		/* PTS for this packet */
	unsigned int        palette[4];
	unsigned int        alpha[4];
	unsigned int        cuspal[4];
	unsigned int        custom;
	unsigned int        now_pts;
	unsigned int        start_pts, end_pts;
	unsigned int        start_col, end_col;
	unsigned int        start_row, end_row;
	unsigned int        width, height, stride;
	size_t              image_size;		/* Size of the image buffer */
	unsigned char       *image;		    /* Grayscale value */
	unsigned char       *aimage;	    /* Alpha value */
	unsigned int        scaled_frame_width, scaled_frame_height;
	unsigned int        scaled_start_col, scaled_start_row;
	unsigned int        scaled_width, scaled_height, scaled_stride;
	size_t              scaled_image_size;
	unsigned char       *scaled_image;
	unsigned char       *scaled_aimage;
	int                 auto_palette;      /* 1 if we lack a palette and must use an heuristic. */
	int                 font_start_level;  /* Darkest value used for the computed font */
	//  const vo_functions_t *hw_spu;
	int                 spu_changed;
	unsigned int        forced_subs_only;  /* flag: 0=display all subtitle, !0 display only forced subtitles */
	unsigned int        is_forced_sub;     /* true if current subtitle is a forced subtitle */
	unsigned short      has_palette;
	unsigned short      bitmap;
} GxPixelSubtitleContent;

extern int  GxPixelSubtitle_Init   (GxPixelSubtitleWindow *rect, GxPixelSubtitleRender render);
extern void GxPixelSubtitle_Destory(int handle);
extern int  GxPixelSubtitle_Show   (int handle);
extern int  GxPixelSubtitle_Hide   (int handle);
extern int  GxPixelSubtitle_Draw   (int handle, GxPixelSubtitleContent *content, int track, GxPixelSubtitleEncode encode);
extern void GxPixelSubtitle_Clean  (int handle);

#endif
