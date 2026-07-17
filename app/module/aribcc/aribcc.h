#ifndef __ARIBCC_H__
#define __ARIBCC_H__

#include "app_config.h"

#if ARIBCC_SUPPORT
#include <gxcore.h>

#ifndef E_FAILURE
#define E_FAILURE                   -1
#endif
#ifndef E_OK
#define E_OK                        0
#endif
#ifndef E_INVALID_HANDLE
#define E_INVALID_HANDLE            0
#endif

#define ASSERT(exp)                     assert(exp)
#define MALLOC(size)                    GxCore_Malloc(size)
#define CALLOC(n, size)                 GxCore_Calloc(n, size)
#define REALLOC(ptr, size)              GxCore_Realloc(ptr, size)
#define FREE(ptr)                       GxCore_Free(ptr)

typedef void (*aribcc_timer_event) (unsigned int,unsigned int);

typedef struct {
	int      (*start)    (void);
	int      (*stop)     (void);
	handle_t (*create)(aribcc_timer_event func,void *arg,int ms);
	int      (*remove)(handle_t handle);
	void     (*delayms)      (uint32_t ms);
	int32_t  (*exec)         (void);
} GxAribCC_TimerOps;

typedef struct {
	handle_t                handle;
	GxAribCC_TimerOps*  ops;
} GxAribCC_Timer;

typedef struct {
	handle_t    (*open)     (uint32_t demux_id,uint16_t pid);
	int32_t     (*close)    (handle_t handle);
	int32_t     (*start)    (handle_t handle);
	int32_t     (*stop)     (handle_t handle);
	uint32_t    (*get_stc)  (handle_t handle);
	size_t      (*get_pes)  (handle_t handle, uint8_t** ptr, size_t size);
	uint8_t     (*nextbyte) (handle_t handle);
	void        (*skipbytes)(handle_t handle, uint16_t size);
	void        (*copybytes)(handle_t handle, uint8_t* ptr, uint16_t size);
	uint8_t     (*get2bits) (handle_t handle, uint8_t* ptr, uint32_t* index);
	uint32_t    (*alignbyte)(handle_t handle, uint32_t index);
} GxAribCC_DemuxOps;

typedef struct  {
	int			demux;
	handle_t                handle;
	GxAribCC_DemuxOps*    ops;
} GxAribCC_demux;

typedef struct {
	uint16_t                x;
	uint16_t                y;
	uint16_t                width;
	uint16_t                heigth;
} GxAribCC_RenderRect;

typedef struct {
	uint16_t                x;
	uint16_t                y;
} GxAribCC_RenderPoint;

typedef struct {
	GxAribCC_RenderPoint  start_point;
	uint16_t                len;
	uint16_t                thickness;
} GxAribCC_RenderLine;

typedef struct {
	uint8_t            y;
	uint8_t            cb;
	uint8_t            cr;
	uint8_t            t;
} GxAribCC_YCC;

typedef struct {
	uint8_t            r;
	uint8_t            g;
	uint8_t            b;
	uint8_t            t;
} GxAribCC_RGB;

typedef struct {
	union color_format {
		uint32_t       value;
		GxAribCC_YCC ycc;
		GxAribCC_RGB rgb;
	}color;
} GxAribCC_RenderColor;

typedef struct {
	uint8_t                 clut_id;
	GxAribCC_RenderColor  entries_list[256];
} GxAribCC_RenderClut;

typedef struct {
	handle_t    (*open)         (GxAribCC_RenderRect* rect,void *surface,void *buffer);
	int32_t	(*registercb) (handle_t handle,GxAribCC_RenderCB cb);
	int32_t     (*close)        (handle_t handle);
	int32_t     (*clear)        (handle_t handle,int x, int y,int w,int h);
	int32_t     (*show)         (handle_t handle);
	int32_t     (*hide)         (handle_t handle);
	int32_t     (*fill_region)  (handle_t handle, int8_t *data, uint32_t len);
} GxAribCC_RenderOps;

typedef struct  {
	handle_t                handle;
	GxAribCC_RenderOps*   ops;
} GxAribCC_Render;

typedef struct {
	handle_t    (*open)         (GxAribCC_demux* demux, GxAribCC_Render* render, GxAribCC_Timer* timer);
	int32_t     (*close)        (handle_t handle);
	int32_t     (*start)         (handle_t handle);
	int32_t     (*stop)         (handle_t handle);
	int32_t     (*set_stream)   (handle_t handle, uint16_t comp_page_id, uint16_t anci_page_id);
	int32_t     (*switch_lang)   (handle_t handle, GxAribCC_LangID langid);
	int32_t     (*decode)       (handle_t handle);
} GxAribCC_DecoderOps;

typedef struct {
	handle_t                handle;
	GxAribCC_DecoderOps*  ops;
} GxAribCC_Decoder;

typedef struct {
	volatile bool           decoding;
	volatile bool           timing;
	handle_t                decode_thread;
	handle_t                timer_thread;
	GxAribCC_demux        demux;
	GxAribCC_Render       render;
	GxAribCC_Decoder      decoder;
	GxAribCC_Timer        timer;
} GxAribCC_Info;

extern GxAribCC_DemuxOps aribcc_demux;
extern GxAribCC_RenderOps aribcc_render_spp;
extern GxAribCC_RenderOps aribcc_render_osd;
extern GxAribCC_DecoderOps aribcc_decoder;
extern GxAribCC_TimerOps aribcc_timer;

#define AV_RB16(x)                           \
    ((((const uint8_t*)(x))[0] << 8) |          \
      ((const uint8_t*)(x))[1])

typedef struct SLConfigDescr {
	int use_au_start;
	int use_au_end;
	int use_rand_acc_pt;
	int use_padding;
	int use_timestamps;
	int use_idle;
	int timestamp_res;
	int timestamp_len;
	int ocr_len;
	int au_len;
	int inst_bitrate_len;
	int degr_prior_len;
	int au_seq_num_len;
	int packet_seq_num_len;
} SLConfigDescr;

/* TS stream handling */
enum MpegTSState {
	MPEGTS_HEADER = 0,
	MPEGTS_PESHEADER,
	MPEGTS_PESHEADER_FILL,
	MPEGTS_PAYLOAD,
	MPEGTS_SKIP,
};

#define MAX_PES_PAYLOAD 200 * 1024

/* enough for PES header + length */
#define PES_START_SIZE  6
#define PES_HEADER_SIZE 9
#define MAX_PES_HEADER_SIZE (9 + 255)
#define BUFFER_PADDING_SIZE 32

typedef struct PESContext {
	int pcr_pid; /**< if -1 then all packets containing PCR are considered */
	int stream_type;
	uint8_t stream_id;
	enum MpegTSState state;
	/* used to get the format */
	int data_index;
	int flags; /**< copied to the AVPacket flags */
	int total_size;
	int pes_header_size;
	int extended_stream_id;
	int64_t pts, dts;
	int64_t ts_packet_pos; /**< position of first TS packet of this PES packet */
	uint8_t header[MAX_PES_HEADER_SIZE];
	uint8_t *data_buffer;
	SLConfigDescr sl;
	void *priv;/*pointer to API */
} PESContext;

extern int arib_init_caption(void);
extern char *arib_set_langid(int langid);
extern int arib_get_cur_lang(void);
extern char *arib_get_cur_caption_lang(void);
extern uint8_t dvbprivate_wait_pcr(PESContext *pes);
extern void aribcc_decoder_draw_region(void *arib_dec,int8_t *data, int32_t len);
extern int process_dvbs_pes_cc(PESContext *pes,uint8_t *buf, int buf_size, int is_start,int64_t pos);

#endif
#endif
