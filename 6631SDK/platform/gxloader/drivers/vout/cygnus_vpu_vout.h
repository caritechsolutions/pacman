#ifndef __CYGNUS_VPU_VOUT_H__
#define __CYGNUS_VPU_VOUT_H__

#include "../gdi/gx3201_gdi.h"

#if defined (CONFIG_ARCH_CKMMU_CYGNUS) || defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA) || defined (CONFIG_ARCH_CKMMU_SCORPIO)
#define PIC_BASE_ADDR              (VPU_BASE_ADDR + 0x3000)
#endif /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO*/

// SYS
int vpu_init(void);

// VPP2(SVPU)
int vpp2_enable(int en);
int vpp2_zoom(int x, int y, int width, int height);
int vpp2_update(void);

// CE
int ce_init(void);
int ce_enable(int en);
int ce_set_format(GxColorFormat format);
int ce_update(void);
int ce_set_auto(void);
int ce_set_layer(int layer_sel);
int ce_set_buf(unsigned buf, int width, int height, int color_format);
int ce_bind_to_vpp2(int en);


#endif /*__CYGNUS_VPU_VOUT_H__*/

