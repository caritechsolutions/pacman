#ifndef __GX_ATSC_CC_H__
#define __GX_ATSC_CC_H__

#include "gxcore.h"
#include "module/subtitle/gx_subtitle_atsccc.h"

__BEGIN_DECLS

#define GX_ATSC_608_CC_FLAG          (1<<0)
#define GX_ATSC_708_DTV_SERVICE_FLAG (1<<1)

typedef enum {
	GX_ATSC_608_CC1 = 0, /*atsc cc 608*/
	GX_ATSC_608_CC2,
	GX_ATSC_608_CC3,
	GX_ATSC_608_CC4,
	GX_ATSC_708_DTV_SERVICE1, /*atsc cc 708*/
	GX_ATSC_708_DTV_SERVICE2,
	GX_ATSC_708_DTV_SERVICE3,
	GX_ATSC_708_DTV_SERVICE4,
	GX_ATSC_708_DTV_SERVICE5,
	GX_ATSC_708_DTV_SERVICE6,
	GX_ATSC_CC_AUTO,
	GX_ATSC_CC_INVALID
}GxAtsccc_ServiceID;

typedef struct {
	uint16_t              x;
	uint16_t              y;
	uint16_t          width;
	uint16_t         height;
} GxAtsccc_RenderRect;

typedef enum {
	GXATSCCC_SPP,
	GXATSCCC_OSD
} GxAtsccc_RenderType;

typedef struct {
	GX_SUBTITLE_ATSCCC_SERVICE  id;
	GxAtsccc_RenderRect rect;
	GxAtsccc_RenderType type;
} GxAtsccc_Param;

handle_t    GxAtsccc_Open     (GxAtsccc_Param* param);
int32_t     GxAtsccc_StreamSet(handle_t handle, GX_SUBTITLE_ATSCCC_SERVICE id);
int32_t     GxAtsccc_Close    (handle_t handle);
int32_t     GxAtsccc_Start    (handle_t handle);
int32_t     GxAtsccc_Stop     (handle_t handle);
int32_t     GxAtsccc_Show     (handle_t handle);
int32_t     GxAtsccc_Clear    (handle_t handle);
int32_t     GxAtsccc_Hide     (handle_t handle);
int32_t     GxAtsccc_SendData (handle_t handle, uint8_t* data, int length);

__END_DECLS
#endif
