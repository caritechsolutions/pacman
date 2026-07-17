#ifndef __GX_DVB_SUBTITLE_H__
#define __GX_DVB_SUBTITLE_H__

#include "gxcore.h"


__BEGIN_DECLS

typedef struct {
	uint16_t                x;
	uint16_t                y;
	uint16_t                width;
	uint16_t                height;
} GxDvbSubt_RenderRect;

typedef enum {
	GX_DVB_SUBT_SPP,
	GX_DVB_SUBT_OSD
} GxDvbSubt_RenderType;

typedef struct {
	GxDvbSubt_RenderRect    rect;
	GxDvbSubt_RenderType    type;
} GxDvbSubt_Param;

typedef struct {
	uint32_t                    pid;
	uint16_t                    comp_page_id;
	uint16_t                    anci_page_id;
} GxDvbSubt_Stream;

handle_t    GxDvbSubt_Open       (GxDvbSubt_Param* param);
int32_t     GxDvbSubt_StreamSet  (handle_t handle, GxDvbSubt_Stream* stream);
int32_t     GxDvbSubt_Synchronize(handle_t handle, int32_t milliseconds);
int32_t     GxDvbSubt_Close      (handle_t handle);
int32_t     GxDvbSubt_Start      (handle_t handle);
int32_t     GxDvbSubt_Stop       (handle_t handle, int freeze);
int32_t     GxDvbSubt_Show       (handle_t handle);
int32_t     GxDvbSubt_Clear      (handle_t handle);
int32_t     GxDvbSubt_Hide       (handle_t handle);
int32_t     GxDvbSubt_SendPesData(handle_t handle, unsigned char* packet, unsigned int packet_length);
int32_t     GxDvbSubt_ShowTimer  (handle_t handle);

__END_DECLS

#endif
