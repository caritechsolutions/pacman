#ifndef __GX_ARIBCC_H__
#define __GX_ARIBCC_H__

#include "gxcore.h"

__BEGIN_DECLS

typedef struct {
	uint8_t            y;
	uint8_t            cb;
	uint8_t            cr;
	uint8_t            t;
} GxAribcc_YCC;

typedef struct {
	uint8_t            r;
	uint8_t            g;
	uint8_t            b;
	uint8_t            t;
} GxAribcc_RGB;

typedef struct {
	union render_color_format {
		uint32_t       value;
		GxAribcc_YCC ycc;
		GxAribcc_RGB rgb;
	}color;
} GxAribcc_RenderColor;

typedef struct {
	uint8_t                 clut_id;
	GxAribcc_RenderColor  entries_list[256];
} GxAribcc_RenderClut;

typedef struct {
	uint16_t              x;
	uint16_t              y;
	uint16_t          width;
	uint16_t         height;
} GxAribcc_RenderRect;


typedef enum {
	GXARIBCC_SPP,
	GXARIBCC_OSD
} GxAribcc_RenderType;

typedef struct {
	GxAribcc_RenderRect       render_rect;
	GxAribcc_RenderType        render_type;
} GxAribcc_Param;

typedef struct {
	uint32_t                    pid;
	uint16_t                    group_a_id;
	uint16_t                    group_b_id;
} GxAribcc_Stream;


handle_t    GxAribcc_Open         (GxAribcc_Param* param);
int32_t     GxAribcc_StreamSet    (handle_t handle, GxAribcc_Stream* stream);
int32_t     GxAribcc_Synchronize  (handle_t handle, int32_t milliseconds);
int32_t     GxAribcc_Close        (handle_t handle);
int32_t     GxAribcc_Start        (handle_t handle);
int32_t     GxAribcc_SetNtsc      (handle_t handle, bool flag);
int32_t     GxAribcc_Stop         (handle_t handle, int freeze);
int32_t     GxAribcc_Show         (handle_t handle);
int32_t     GxAribcc_Clear        (handle_t handle);
int32_t     GxAribcc_Hide         (handle_t handle);
int32_t     GxAribcc_SendESData  (handle_t handle, uint8_t* packet, int packet_length);


__END_DECLS
#endif

