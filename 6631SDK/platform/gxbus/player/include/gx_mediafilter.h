#ifndef _GX_MEDIAFILTER_H_
#define _GX_MEDIAFILTER_H_

#include "gx_common.h"
#include "gx_fifo.h"

typedef struct gx_media_filter GxMediaFilter;
typedef struct gx_pin          GxPin;

typedef enum _MediaFitlerType {
	GX_MFT_TYPE_ACCESS,
	GX_MFT_TYPE_DEMUXER,
	GX_MFT_TYPE_DECODER,
	GX_MFT_TYPE_RENDERER,
	GX_MFT_TYPE_UNKNOWN
}GxMediaFilterType;

typedef enum {
	GX_MFT_STATE_STOPPED,
	GX_MFT_STATE_PAUSED,
	GX_MFT_STATE_RUNNING,
	GX_MFT_STATE_UNKNOWN,
}GxMediaFilterStatus;

typedef enum _PinDirection {
	GX_PINDIR_INPUT,
	GX_PINDIR_OUTPUT
}GxPinDirection;

#define GX_PINSIZE_TSA (188 * 1024)
typedef enum _PinFlag {
	GX_PINFLAG_NO_PTS_FIFO = GXAV_NO_PTS_FIFO,
	GX_PINFLAG_PTS_FIFO = GXAV_PTS_FIFO,
	GX_PINFLAG_WPROTECT_FIFO = GXAV_WPROTECT_FIFO,
	GX_PINFLAG_SW,
	GX_PINFLAG_MUXTS,
	GX_PINFLAG_MUXER,
	GX_PINFLAG_TSA,
	GX_PINFLAG_ESV  = (1 << 8 )|GXAV_PTS_FIFO,
	GX_PINFLAG_WKV  = (1 << 9 )|GXAV_NO_PTS_FIFO,
	GX_PINFLAG_FBV  = (1 << 10)|GXAV_NO_PTS_FIFO,
	GX_PINFLAG_FBVZ = (1 << 11)|GXAV_NO_PTS_FIFO,
	GX_PINFLAG_FCOL = (1 << 12)|GXAV_NO_PTS_FIFO,
	GX_PINFLAG_ESA  = (1 << 13)|GXAV_PTS_FIFO,
	GX_PINFLAG_ESA1 = (1 << 14)|GXAV_PTS_FIFO,
	GX_PINFLAG_WKA  = (1 << 15)|GXAV_NO_PTS_FIFO,
	GX_PINFLAG_TSR  = (1 << 16)|GXAV_NO_PTS_FIFO|GXAV_WPROTECT_FIFO,
	GX_PINFLAG_TSW  = (1 << 17)|GXAV_NO_PTS_FIFO,
	GX_PINFLAG_PCM  = (1 << 18)|GXAV_NO_PTS_FIFO,
	GX_PINFLAG_PCM1 = (1 << 19)|GXAV_NO_PTS_FIFO,
	GX_PINFLAG_AC3  = (1 << 20)|GXAV_NO_PTS_FIFO,
	GX_PINFLAG_EAC3 = (1 << 21)|GXAV_NO_PTS_FIFO,
	GX_PINFLAG_DTS  = (1 << 22)|GXAV_NO_PTS_FIFO,
	GX_PINFLAG_AAC  = (1 << 23)|GXAV_NO_PTS_FIFO,
	GX_PINFLAG_SESV = (1 << 24)|GXAV_PTS_FIFO,
	GX_PINFLAG_SESA = (1 << 25)|GXAV_PTS_FIFO,
	GX_PINFLAG_SVPU = (1 << 26)|GXAV_NO_PTS_FIFO,
	GX_PINFLAG_MASK = 0xffffff00
}GxPinFlag;

#ifndef NO_OS
struct gx_pin {
	struct gxlist_head   head;
	char*                name;
	GxMediaFilter*       filter;
	GxPinDirection       dir;
	GxPin*               link;
	GxPin*               source;
	GxFifo*              fifo;
};

typedef enum  {
	GX_MFT_EVENT_PLAY_END        ,
	GX_MFT_EVENT_RECORD_END      ,
	GX_MFT_EVENT_RECORD_FULL     ,
	GX_MFT_EVENT_START_FILL_BUF  ,
	GX_MFT_EVENT_END_FILL_BUF    ,
	GX_MFT_EVENT_SERVER_ERROR    ,
	GX_MFT_EVENT_MEDIA_PAUSE     ,
	GX_MFT_EVENT_MEDIA_RESUME    ,
}GxMediaFilterEventType;

typedef struct {
	GxMediaFilterEventType  type;
	void*                   arg;
}GxMediaFilterEventPara;

typedef struct  {
	void*                priv;
	void                 (*func)(void* priv,void* args);
}GxMediaFilterEvent;

struct gx_media_filter {
	GxObject             obj;
	GxMediaFilterType    type;
	GxMediaFilterStatus  status;

	GxPin                in_pin;
	GxPin                out_pin;
	GxMediaFilterEvent   event;
};

typedef struct gx_media_filter_class {
	GxClass             _inherit;

	int (*run)          (GxMediaFilter *filter);
	int (*pause)        (GxMediaFilter *filter);
	int (*resume)       (GxMediaFilter *filter);
	int (*stop)         (GxMediaFilter *filter);
	int (*config)       (GxMediaFilter *filter);
	int (*abort)        (GxMediaFilter *filter);
	int (*reset)        (GxMediaFilter *filter);
	int (*wait)         (GxMediaFilter *filter);
}GxMediaFilterClass;

extern GxMediaFilterClass gx_mediafilter_base;

#define  GetGxMediaFilterClassFromObject(s) ((GxMediaFilterClass*)(GetGxClassFromObject(s)))
#define  GXMEDIAFILTER(s)                   ((GxMediaFilter*)(s))
#define  GXMEDIAFILTERCLASS(s)              ((GxMediaFilterClass*)(s))

void     GxMediaFilter_Init      (void);
void     GxMediaFilter_Destroy   (void);

GxPin*   GxMediaFilter_FindPin   (void *obj, const char *pin_name);

int      GxMediaFilter_Run       (void *obj);
int      GxMediaFilter_Pause     (void *obj);
int      GxMediaFilter_Resume    (void *obj);
int      GxMediaFilter_Stop      (void *obj);
int      GxMediaFilter_Config    (void *obj);
int      GxMediaFilter_Abort     (void *obj);
int      GxMediaFilter_Wait      (void *obj);
int      GxMediaFilter_Reset     (void *obj);
void     GxMediaFilter_SetEvent(void* obj, void (*func)(void* priv, void* args), void* priv);

int      GxMediaFilter_Connect   (void *source,const char *source_pin_name,void* link,const char *link_pin_name);
int      GxMediaFilter_Disconnect   (void *source,const char *source_pin_name,void* link,const char *link_pin_name);

GxMediaFilterStatus GxMediaFilter_Status(void *obj);

GxPin*  GxPin_Create       (const char *pin_name, void *obj, GxPinDirection dir,unsigned int size,int subflag);
void    GxPin_Destroy      (GxPin *self);
void    GxPin_Connect      (GxPin *self, GxPin *pin_out);
void    GxPin_Disconnect   (GxPin *self);
int     GxPin_Write        (GxPin * pin, void *data, size_t size);
int     GxPin_Read         (GxPin * pin, void *data, size_t size);
int     GxPin_Peek         (GxPin * pin, void *data, size_t size);
#endif

#endif

