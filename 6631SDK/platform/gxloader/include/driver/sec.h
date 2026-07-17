#ifndef __SEC_H__
#define __SEC_H__

#include "gxav_vout_propertytypes.h"
struct active_code {
	int dcs_l[2];
	int dcs_h[2];
};

typedef struct {
	GxVideoOutDCSMode mode;
	struct active_code enable;
	struct active_code disable;
} DcsParam;

int vout_secure_init(unsigned base_addr);

void vout_secure_uninit(void);

int vout_secure_otp_get_macrovision_status(void);

int vout_secure_otp_get_dcs_status(void);

int vout_secure_macrovision_nootp_set(void);

int vout_secure_macrovision_setmode(GxFormatSystem system_mode);

int vout_secure_macrovision_set(GxFormatSystem system_mode, GxMacrovisionParam *param);

int vout_secure_dcs_init(int is_cvbs, int is_50hz);

DcsParam *vout_secure_dcs_get_param(GxVideoOutDCSMode mode);

int vout_secure_dcs_set(DcsParam *param, int enable, int is_cvbs, int is_50hz);

int vout_secure_cgms_enable(int *en);

int vout_secure_cgms_set(GxVideoOutProperty_CGMSConfig *property);

int vout_secure_wss_enable(int *en, unsigned int wss_line1, unsigned int wss_line2);

int vout_secure_wss_set(GxVideoOutProperty_WSSConfig *property);

int vout_secure_layer_secure_color_set(GxLayerID layer, int en, GxColor color);

int vout_secure_layer_secure_view_set(GxLayerID layer, int width, int height);

int vout_secure_layer_secure_color_get(int layer, int *en, GxColor *color);

int vout_secure_layer_secure_view_get(GxLayerID layer, int *width, int *height);

int vout_secure_pic_keepout_buffer(int addr, int size);

int vout_secure_ce_permission_buffer(int addr, int size);

#endif /*__SEC_H__*/

