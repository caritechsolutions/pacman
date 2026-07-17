#ifndef __GX_VOBSUB_H__
#define __GX_VOBSUB_H__

#include "../avutil/common.h"

/**********************************************************************
 * Vobsub
 **********************************************************************/

/**********************************************************************
 * Packet queue
 **********************************************************************/
#define MAX_LANG_NUM    8

typedef struct vob_packet{
	int id;
    unsigned int pts100;
    unsigned int size;
    unsigned char *data;
	struct vob_packet* next;
} vob_packet_t;

typedef struct vob_stream{
	unsigned int filepos;
	unsigned int startpts;
	unsigned int endpos;
	unsigned int endpts;
	vob_packet_t* packet;
	struct vob_stream* next;
}vob_stream_t;

typedef struct vob_lang{
	int  cur_id;
	int  num;
	int  id[MAX_LANG_NUM];
	char lang[MAX_LANG_NUM][16];
}vob_lang_t;

typedef struct vobsub {
    unsigned int palette[16];
    int delay;
    unsigned int have_palette;
    unsigned int orig_frame_width, orig_frame_height;
    unsigned int origin_x, origin_y;
    /* index */
    vob_stream_t* spu_streams;
} vobsub_t;

int vobsub_open(const char* const filename, void** spu, void** vob);
void vobsub_close(int fd, void* spu, void* vob);
vob_stream_t* vobsub_alloc_oneline_chr(int fd);
vob_packet_t* vobsub_alloc_oneline_packet(const char* const filename, vob_stream_t* stream, void* spudec);
void vobsub_free_oneline_chr(vob_stream_t* stream);
void vobsub_free_oneline_packet(vob_packet_t* packet);
void vobsub_switch_stream_spu(int id);
void vobsub_reset_oneline_chr(int fd);
void vobsub_get_info_spu(vob_lang_t* subinfo);
#endif /* MPLAYER_VOBSUB_H */


