#ifndef __GX_SCTE_SUBTITLE_H__
#define __GX_SCTE_SUBTITLE_H__

typedef struct {
	uint16_t    x;
	uint16_t    y;
	uint16_t    width;
	uint16_t    height;
} GxScteSubtitleWindow;

typedef enum {
	GX_SCTE_SUB_SPP,
	GX_SCTE_SUB_OSD
} GxScteSubtitleRender;

typedef enum {
	GX_SCTE_SYNC_FREERUN = 0,
	GX_SCTE_SYNC_NORMAL,
	GX_SCTE_SYNC_SKIP,
	GX_SCTE_SYNC_REPEAT,
	GX_SCTE_SYNC_ERROR,
	GX_SCTE_SYNC_SYNC_MAX
} GxScteSubtitleSyncState;

typedef struct {
	GxScteSubtitleWindow    win;
	GxScteSubtitleRender    render;
	int                     (*read_packet)(void *priv, unsigned char *packet, unsigned int packet_length);
	GxScteSubtitleSyncState (*sync_pts)   (void *priv, int64_t pts);
	void                    *priv;
} GxScteSubtitleParam;

extern handle_t GxScteSubtitle_Open (GxScteSubtitleParam *param);
extern int32_t  GxScteSubtitle_Close(handle_t handle);
extern int32_t  GxScteSubtitle_Start(handle_t handle);
extern int32_t  GxScteSubtitle_Clean(handle_t handle);
extern int32_t  GxScteSubtitle_Stop (handle_t handle);
extern int32_t  GxScteSubtitle_Show (handle_t handle);
extern int32_t  GxScteSubtitle_Hide (handle_t handle);

#endif
