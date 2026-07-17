#ifndef __GX_MUXER_H__
#define __GX_MUXER_H__

#include "gx_avutil.h"
#include "gx_stream.h"
#include "page_cache.h"

#define GX_MUXER_PIN_NAME_INPUT  "mux_in"
#define GX_MUXER_PIN_NAME_OUTPUT "mux_out"

#define MUXER_ERROR   (-1)
#define MUXER_OK      (0)
#define MUXER_OOM     (1)
#define MUXER_FULL    (2)
#define MUXER_FLUSHED (3)

#define MUX_IO_BUFFER_SIZE    (0x10000)
#define MUXER_OUT_FIFO_SIZE   (0x100000)
#define MUXER_CACHE_SIZE_MAX  (0x400000)

#define GX_MUXER_PIN_OUTPUT  "mux_out"

typedef struct gx_muxer GxMuxer;
typedef struct gx_muxer_class  GxMuxerClass;
typedef struct gx_muxer_packet GxMuxerPacket;

extern GxMuxerClass gx_MuxerBase;

#define GetGxMuxerClassFromObject(s) ((GxMuxerClass*)(GetGxClassFromObject(s)))
#define GXMUXER(s)                   ((GxMuxer*)(s))
#define GXMUXERCLASS(s)              ((GxMuxerClass *)(s))

typedef enum {
	GX_MUXER_TYPE_UNKNOWN = 0,
	GX_MUXER_TYPE_FFMPEG,
}GxMuxerType;

typedef enum {
	GX_MUXER_CTRL_SET_VIDEO_HEADER = 0,
	GX_MUXER_CTRL_SET_AUDIO_HEADER,
}GxMuxerCtrl;

struct gx_muxer_packet {
	int     stream_index;
	uint8_t *data;
	size_t  size;
	int     pts;
};

typedef struct {
	char *format;
#define MUX_STREAM_MAX (8)
	struct {
		int stream_index;
		enum CodecType codec_type;
		enum CodecID codec_id;
	}streams[MUX_STREAM_MAX];

	int nb_streams;
}GxMuxerOpenInfo;

typedef struct {
	int stream_index;
	int extradata_size;
	void *extradata;
}GxMuxerHeaderInfo;

struct gx_muxer {
	GxMediaFilter filter;

	GxMuxerOpenInfo info;
	int have_error;
	int flushed;
	handle_t thread_mux;
	GxPin *inpin;
	GxPin *outpin;
	void  *priv;
};

struct gx_muxer_class {
	GxMediaFilterClass  _inherit;
	AUTHER;

	int                   type;
	const char*           name;

	GxMuxer *(*open)      (GxMuxer *muxer);
	void (*close)         (GxMuxer *muxer);
	int  (*control)       (GxMuxer *muxer, int cmd, void *args);
	int  (*write_header)  (GxMuxer *muxer, int stream_index, uint8_t *data, size_t size);
	int  (*write_frame)   (GxMuxer *muxer, int stream_index, uint8_t *data, size_t size, int pts);
	int  (*write_trailer) (GxMuxer *muxer);
};

extern GxMuxer *GxMuxer_Open(GxMuxerOpenInfo *info);
extern void   GxMuxer_Close(GxMuxer *muxer);
extern void GxMuxerPacket_Destroy(GxMuxerPacket *pkt);
extern GxMuxerPacket *GxMuxerPacket_New(int size);
extern int GxMuxer_Control(GxMuxer *muxer, int cmd, void *args);

#endif


