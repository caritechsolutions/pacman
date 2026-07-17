#ifndef ___SPUDEC_H__
#define ___SPUDEC_H__

#include "vobsub.h"
#include "../avutil/common.h"
#include "../avutil/intreadwrite.h"

typedef struct __packet {
  unsigned char *packet;
  unsigned int palette[4];
  unsigned int alpha[4];
  unsigned int control_start;	/* index of start of control data */
  unsigned int current_nibble[2]; /* next data nibble (4 bits) to be
                                     processed (for RLE decoding) for
                                     even and odd lines */
  int deinterlace_oddness;	/* 0 or 1, index into current_nibble */
  unsigned int start_col, end_col;
  unsigned int start_row, end_row;
  unsigned int width, height, stride;
  unsigned int start_pts, end_pts;
  struct __packet *next;
}GxSpuPacket;

typedef struct {
  GxSpuPacket *queue_head;
  GxSpuPacket *queue_tail;
  unsigned int global_palette[16];
  unsigned int orig_frame_width, orig_frame_height;
  unsigned char* packet;
  size_t packet_reserve;	/* size of the memory pointed to by packet */
  unsigned int packet_offset;	/* end of the currently assembled fragment */
  unsigned int packet_size;	/* size of the packet once all fragments are assembled */
  int packet_pts;		/* PTS for this packet */
  unsigned int palette[4];
  unsigned int alpha[4];
  unsigned int cuspal[4];
  unsigned int custom;
  unsigned int now_pts;
  unsigned int start_pts, end_pts;
  unsigned int start_col, end_col;
  unsigned int start_row, end_row;
  unsigned int width, height, stride;
  size_t image_size;		/* Size of the image buffer */
  unsigned char *image;		/* Grayscale value */
  unsigned char *aimage;	/* Alpha value */
  unsigned int scaled_frame_width, scaled_frame_height;
  unsigned int scaled_start_col, scaled_start_row;
  unsigned int scaled_width, scaled_height, scaled_stride;
  size_t scaled_image_size;
  unsigned char *scaled_image;
  unsigned char *scaled_aimage;
  int auto_palette; /* 1 if we lack a palette and must use an heuristic. */
  int font_start_level;  /* Darkest value used for the computed font */
//  const vo_functions_t *hw_spu;
  int spu_changed;
  unsigned int forced_subs_only;     /* flag: 0=display all subtitle, !0 display only forced subtitles */
  unsigned int is_forced_sub;         /* true if current subtitle is a forced subtitle */
  unsigned short has_palette;
  unsigned short bitmap;
} GxSpuDec;


void spudec_heartbeat(void *spudec, unsigned int pts100);
void spudec_assemble(void *spudec, unsigned char *packet, unsigned int len, int pts100);
void spudec_draw(void *spudec, void (*draw_alpha)(int x0,int y0, int w,int h, unsigned char* src, unsigned char *srca, int stride,void* spu));
//void spudec_draw_scaled(void *spudec, unsigned int dxs, unsigned int dys, void (*draw_alpha)(int x0,int y0, int w,int h, unsigned char* src, unsigned char *srca, int stride));
void spudec_update_palette(void *spudec, unsigned int *palette);
void *spudec_new_scaled(unsigned int *palette, unsigned int frame_width, unsigned int frame_height, uint8_t *extradata, int extradata_len);
void *spudec_new(unsigned int *palette);
void spudec_free(void *spudec);
void spudec_reset(void *spudec);	// called after seek
int spudec_visible(void *spudec); // check if spu is visible
void spudec_set_font_factor(void * spudec, double factor); // sets the equivalent to ffactor
//void spudec_set_hw_spu(void *spudec, const vo_functions_t *hw_spu);
int spudec_changed(void *spudec);
void spudec_calc_bbox(void *me, unsigned int dxs, unsigned int dys, unsigned int* bbox);
void spudec_set_forced_subs_only(void * const spudec, const unsigned int flag);
void spudec_free(void* spudec);

#endif /* MPLAYER_SPUDEC_H */

