#ifndef __GX_TELETEXT_H__
#define __GX_TELETEXT_H__

#include "gxcore.h"


__BEGIN_DECLS

typedef struct {
	uint16_t                x;
	uint16_t                y;
	uint16_t                width;
	uint16_t                height;
} GxTeletext_RenderRect;

typedef enum {
	GX_TELETEXT_SPP,
	GX_TELETEXT_OSD
} GxTeletext_RenderType;

typedef enum {
	GX_TELETEXT_TTX = 0,
	GX_TELETEXT_MAG,
} GxTeletext_CodeFormat;

typedef enum {
	GX_TELETEXT_SYNC_FREERUN = 0,
	GX_TELETEXT_SYNC_NORMAL,
	GX_TELETEXT_SYNC_SKIP,
	GX_TELETEXT_SYNC_REPEAT,
	GX_TELETEXT_SYNC_ERROR,
	GX_TELETEXT_SYNC_SYNC_MAX
} GxTeletext_SyncState;

typedef struct {
	GxTeletext_RenderRect    rect;
	GxTeletext_RenderType    type;
	GxTeletext_CodeFormat    format;
	GxTeletext_SyncState     (*sync)(void *priv, int64_t pts);
	void                     *priv;
} GxTeletext_Param;

typedef struct {
	uint32_t                    pid;
	uint16_t                    comp_page_id;
	uint16_t                    anci_page_id;
} GxTeletext_Stream;

handle_t    GxTeletext_Open         (GxTeletext_Param* param);
int32_t     GxTeletext_StreamSet    (handle_t handle, GxTeletext_Stream* stream);
int32_t     GxTeletext_Synchronize  (handle_t handle, int32_t milliseconds);
int32_t     GxTeletext_Close        (handle_t handle);
int32_t     GxTeletext_Start        (handle_t handle);
int32_t     GxTeletext_Stop         (handle_t handle, int freeze);
int32_t     GxTeletext_Show         (handle_t handle);
int32_t     GxTeletext_Clear        (handle_t handle);
int32_t     GxTeletext_Hide         (handle_t handle);
int32_t     GxTeletext_SendPesData  (handle_t handle, unsigned char* packet, unsigned int packet_length, int64_t pts);
int32_t     GxTeletext_Timer        (handle_t handle);
int32_t     GxTeletext_FindRegion   (const char *lang);

__END_DECLS

#endif
