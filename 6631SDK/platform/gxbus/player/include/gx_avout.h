#ifndef __GX_AVOUT_H__
#define __GX_AVOUT_H__

#include "gx_mediafilter.h"
#include "stheader.h"
#include "gx_subtitle.h"

#define GX_VO_PIN_NAME_IN   "gx_vo_pin_in"
#define GX_VO_PIN_NAME_OUT  "gx_vo_pin_out"

enum
{
	GX_VO_CTRL_SET_LOCATION,
	GX_VO_CTRL_HIDE_VIDEO,
	GX_VO_CTRL_SHOW_VIDEO,
	GX_VO_CTRL_FREEZE_VIDEO,
};

typedef struct gx_video_out {
	GxMediaFilter         filter;

	char                  *name;
	int32_t               run_pthread;
	GxStreamVideoHeader   *header;

	int                   index;
	void                  *priv;
} GxVideoOut;

typedef struct gx_vo_class {
	GxMediaFilterClass    _inherit;
	AUTHER;
	const char*            name;

	int   (*open)         (GxVideoOut *vo);
	int   (*close)        (GxVideoOut *vo);
	int   (*control)      (GxVideoOut *vo, int cmd, void *arg);
	int   (*draw_frame)   (GxVideoOut *vo, uint8_t *frame, int len, int type);
	int   (*draw_sub)     (GxVideoOut *vo, uint8_t *data, int len, int type);
}GxVideoOutClass;


void GxVideoOutClass_Init    (void);
void GxVideoOutClass_Destroy (void);

void GxVideoOut_PrintList    (void);
GxVideoOut *GxVideoOut_Open  (GxStreamVideoHeader *header,int index);
void GxVideoOut_Close        (GxVideoOut *vo);
int  GxVideoOut_DrawFrame    (GxVideoOut *vo, uint8_t *frame,int len, int type);
int  GxVideoOut_DrawSub      (GxVideoOut *vo, uint8_t *data,int len, int type);
int  GxVideoOut_Control      (GxVideoOut *vo, int cmd, void *arg);

/*=======================================audioout============================*/


#define GX_AO_PIN_NAME_PCM_IN   "ao_pcm_in"
#define GX_AO_PIN_NAME_ADPCM_IN "ao_adpcm_in"
#define GX_AO_PIN_NAME_AC3_IN   "ao_ac3_in"
#define GX_AO_PIN_NAME_EAC3_IN  "ao_eac3_in"
#define GX_AO_PIN_NAME_DTS_IN   "ao_dts_in"
#define GX_AO_PIN_NAME_AAC_IN   "ao_aac_in"

enum
{
	GX_AO_CTRL_RESET,
	GX_AO_CTRL_SET_SPEED,
	GX_AO_CTRL_SET_SYNC,
	GX_AO_CTRL_SWITCH_HEADER,
	GX_AO_CTRL_GET_FRAME_INFO,
	GX_AO_CTRL_GET_CUR_PTS,
	GX_AO_CTRL_SET_PCM_MIX,
	GX_AO_CTRL_SET_DELAY,
	GX_AO_CTRL_DRAIN_WAIT,
};

typedef struct gx_audio_out {
	GxMediaFilter       filter;
	char                *name;

	int32_t             run_pthread;
	GxStreamAudioHeader *header;
	GxStreamAudioHeader *header1;

	int                 index;
	void*               priv;
} GxAudioOut;

typedef struct gx_ao_class {
	GxMediaFilterClass  _inherit;
	AUTHER;
	const char*         name;

	int   (*open)       (GxAudioOut *ao);
	int   (*close)      (GxAudioOut *ao, int immed);
	int   (*control)    (GxAudioOut *ao, int cmd, void *arg);
	int   (*play)       (GxAudioOut *ao, void* data, int len, int flags);
}GxAudioOutClass;

void GxAudioOutClass_Init   (void);
void GxAudioOutClass_Destroy(void);

void GxAudioOut_PrintList   (void);
GxAudioOut *GxAudioOut_Open (GxStreamAudioHeader * header, GxStreamAudioHeader * header1);
void  GxAudioOut_Close      (GxAudioOut *ao, int immed);
int   GxAudioOut_Control    (GxAudioOut *ao, int cmd,void *args);
int   GxAudioOut_Play       (GxAudioOut *ao, void *data, int len, int flags);

/*=======================================audioout============================*/


#define GX_SO_PIN_NAME_IN  	"gx_so_pin_in"

enum
{
	GX_SO_CTRL_RESET,
	GX_SO_CTRL_SET_SPEED,
	GX_SO_CTRL_SET_SYNC_TIME,
	GX_SO_CTRL_EXE_INIT_FUNC,
	GX_SO_CTRL_EXE_DESTORY_FUNC,
	GX_SO_CTRL_EXE_SHOW_FUNC,
	GX_SO_CTRL_EXE_HIDE_FUNC,
	GX_SO_CTRL_SET_PID
};

typedef struct gx_sub_out {
	GxMediaFilter       filter;
	char                *name;
	char                type;
	int32_t             run_pthread;
	GxStreamSubHeader   *header;

	int                 index;
	void*               priv;
	uint64_t             (*cb)(void* args);
	void                *args;
	int                 sub_handle;
	int                 running;
} GxSubOut;

typedef struct gx_so_class {
	GxMediaFilterClass  _inherit;
	AUTHER;
	const char*         name;

	int   (*open)       (GxSubOut *so);
	int   (*close)      (GxSubOut *so);
	int   (*control)    (GxSubOut *so, int cmd, void *arg);
	int   (*play)       (GxSubOut *so, void* data, int len, int flags);
}GxSubOutClass;

void GxSubOutClass_Init   (void);
void GxSubOutClass_Destroy(void);

void GxSubOut_PrintList   (void);
GxSubOut *GxSubOut_Open (GxStreamSubHeader * header, uint64_t (*cb)(void* args), void* args);
void  GxSubOut_Close      (GxSubOut *so);
int   GxSubOut_Control    (GxSubOut *so, int cmd,void *args);
int   GxSubOut_Play       (GxSubOut *so, void *data, int len, int flags);

/*=======================================audioout============================*/

#endif

