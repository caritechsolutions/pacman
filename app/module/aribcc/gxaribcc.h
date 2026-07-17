#ifndef __GX_ARIBCC_H__
#define __GX_ARIBCC_H__

#include "app_config.h"

#if ARIBCC_SUPPORT
#include "gxcore.h"

typedef enum {
	ARIB_LAN_ID_ENG = 0x01,
	ARIB_LAN_ID_CHN,
	ARIB_LAN_ID_ARB,
	ARIB_LAN_ID_DEU,
	ARIB_LAN_ID_FRH,
	ARIB_LAN_ID_SPA,
	ARIB_LAN_ID_ITA,
	ARIB_LAN_ID_POR,
	ARIB_LAN_ID_RUS,
	ARIB_LAN_ID_GRK,
	ARIB_LAN_ID_UNKNOW,
} GxAribCC_LangID;

typedef enum {
	GXARIBCC_OSD,
	GXARIBCC_SPP,
} GxAribCC_RenderType;

typedef struct {
	uint16_t        width;
	uint16_t        height;
	void *	   surface;
	void *	    buffer;
} GxAribCC_RenderSize;

typedef struct {
	int32_t     (*draw_data)  (GxAribCC_LangID langid,GxAribCC_RenderSize *render,int8_t *data, uint32_t len);
	int32_t     (*clean_data)  (GxAribCC_RenderSize *render);
} GxAribCC_RenderCB;

typedef struct {
	uint32_t                    demux_id;
	GxAribCC_RenderType			render_type;
	GxAribCC_RenderSize			render_size;
	GxAribCC_RenderCB			render_cb;
} GxAribCC_Param;

typedef struct {
	uint16_t                    pid;
	uint16_t                    comp_page_id;
	uint16_t                    anci_page_id;
} GxAribCC_Stream;

handle_t GxAribCC_Open         (GxAribCC_Param* param);
int32_t     GxAribCC_Close        (handle_t handle);
int32_t     GxAribCC_StreamSet    (handle_t handle, GxAribCC_Stream* stream);
int32_t     GxAribCC_SwitchLang	(handle_t handle, GxAribCC_LangID langid);
int32_t     GxAribCC_Start        (handle_t handle);
int32_t     GxAribCC_Stop         (handle_t handle);
int32_t     GxAribCC_Show         (handle_t handle);
int32_t     GxAribCC_Hide         (handle_t handle);

#endif
#endif
