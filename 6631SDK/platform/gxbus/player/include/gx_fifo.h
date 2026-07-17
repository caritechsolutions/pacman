#ifndef __GX_FIFO_H__
#define __GX_FIFO_H__

#include "gx_common.h"

typedef struct gx_fifo {
	unsigned int         size;
	void*                priv;
	char*                extdata;
	unsigned int         extsize;
	unsigned char        *buffer;  /* the buffer holding the data              */
	unsigned int         in;       /* data is added at offset (in % size)      */
	unsigned int         out;      /* data is extracted from off. (out % size) */
	unsigned int         alloc_out;
	void*                cls;
}GxFifo;

typedef struct {
	handle_t             dev;
	unsigned int         empty_gate;
	unsigned int         full_gate;
	unsigned int         cache_size;

	unsigned int         dvrid;
	unsigned int         source;
	unsigned int         dest;
	unsigned int         dvrhandle;
	unsigned int         sw_buffer_size;
	unsigned int         hw_buffer_size;
	unsigned int         almost_full_gate;
	unsigned int         blocklen;
	unsigned int         flags;
}GxFifoConfig;

typedef struct {
	handle_t             module;
	unsigned int         pin_id;
	GxAvDirection        direction;
}GxFifoLink;

typedef struct {
	unsigned int         security_flag;
	unsigned int         block_size;
	unsigned int         ptr_enable;
}GxFifoShallowInfo;

GxFifo*GxFifo_Create        (size_t size, int subflag);
int    GxFifo_Destroy       (GxFifo *fifo);
int    GxFifo_Config        (GxFifo *fifo,GxFifoConfig*configure);
int    GxFifo_Link          (GxFifo *fifo, GxFifoLink* configure);
int    GxFifo_Unlink        (GxFifo *fifo, GxFifoLink* configure);
int    GxFifo_Reset         (GxFifo *fifo);
size_t GxFifo_Read          (GxFifo *fifo, unsigned char *buffer, unsigned int len, int timeout_us);
size_t GxFifo_ReadWithPts   (GxFifo *fifo, unsigned char *buffer, unsigned int len, int *pts, int timeout_us);
size_t GxFifo_Write         (GxFifo *fifo, unsigned char *buffer, unsigned int len, int timeout_us);
size_t GxFifo_WriteWithPts  (GxFifo *fifo, unsigned char *buffer, unsigned int len, int pts,int timeout_us);
size_t GxFifo_Peek          (GxFifo *fifo, unsigned char *buffer, unsigned int len);
size_t GxFifo_Skip          (GxFifo *fifo, unsigned int len);
size_t GxFifo_GetLength     (GxFifo *fifo);
size_t GxFifo_GetFreeSize   (GxFifo *fifo);
size_t GxFifo_GetPtsLength  (GxFifo *fifo);
size_t GxFifo_GetPtsFreeSize(GxFifo *fifo);
size_t GxFifo_GetCap        (GxFifo *fifo);
size_t GxFifo_GetPtsCap     (GxFifo *fifo);
size_t GxFifo_Rollback      (GxFifo *fifo, int len);
int    GxFifo_GetShallowInfo(GxFifo *fifo, GxFifoShallowInfo *info);
size_t GxFifo_ShallowRead   (GxFifo *fifo, unsigned char **start, unsigned int len, int timeout_us);
int    GxFifo_ShallowWrite  (GxFifo *fifo, unsigned char *start,  unsigned int len);
int    GxFifo_PtsIsFull     (GxFifo *fifo);

typedef struct gx_fifo_class {
	GxFifo* (*create)       (size_t size, int subflag);
	int     (*destroy)      (GxFifo *fifo);
	int     (*config)       (GxFifo *fifo, void*configure);
	int     (*link)         (GxFifo *fifo, void*configure);
	int     (*unlink)       (GxFifo *fifo, void*configure);
	int     (*reset)        (GxFifo *fifo);
	size_t  (*write)        (GxFifo *fifo, unsigned char *buffer, unsigned int len, int timeout_us);
	size_t  (*writepts)     (GxFifo *fifo, unsigned char *buffer, unsigned int len, int pts, int timeout_us);
	size_t  (*read)         (GxFifo *fifo, unsigned char *buffer, unsigned int len, int timeout_us);
	size_t  (*readpts)      (GxFifo *fifo, unsigned char *buffer, unsigned int len, int *pts, int timeout_us);
	size_t  (*peek)         (GxFifo *fifo, unsigned char *buffer, unsigned int len);
	size_t  (*skip)         (GxFifo *fifo,unsigned int len);
	size_t  (*get_len)      (GxFifo *fifo);
	size_t  (*get_free)     (GxFifo *fifo);
	size_t  (*get_cap)      (GxFifo *fifo);
	size_t  (*get_pts_len)  (GxFifo *fifo);
	size_t  (*get_pts_free) (GxFifo *fifo);
	size_t  (*get_pts_cap)  (GxFifo *fifo);
	int     (*rollback)     (GxFifo *fifo, int len);
	size_t  (*shallow_read) (GxFifo *fifo, unsigned char **start, unsigned int len, int timeout_us);
	int     (*shallow_write)(GxFifo *fifo, unsigned char *start,  unsigned int len);
	int     (*shallow_info) (GxFifo *fifo, GxFifoShallowInfo *info);
	int     (*pts_isfull)   (GxFifo *fifo);

//	size_t  (*get_write_addr)(GxFifo *fifo);
//	size_t  (*set_write_addr)(GxFifo *fifo);
//	size_t  (*get_read_addr) (GxFifo *fifo);
//	size_t  (*set_read_addr) (GxFifo *fifo);

#if 0
	GxDemuxPacket* (*alloc_packet) (GxFifo *fifo, size_t size);
	int            (*resize_packet)(GxFifo *fifo, GxDemuxPacket *packet, size_t size);
	size_t         (*apply_packet) (GxFifo *fifo, GxDemuxPacket *packet);
#endif
}GxFifoClass;

#endif
