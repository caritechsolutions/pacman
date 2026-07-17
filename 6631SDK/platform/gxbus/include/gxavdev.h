#ifndef __GX_BUS_AVDEV_H__
#define __GX_BUS_AVDEV_H__

#include <av/gxav.h>
#include <gxcore.h>
#ifndef NO_OS
#include <devapi/gxmtc_api.h>
#endif

__BEGIN_DECLS

extern int GxAvdev_CreateDevice(int pid) ;
extern int GxAvdev_DestroyDevice(int dev_id) ;
extern int GxAvdev_OpenModule(int dev, int type, int id) ;
extern int GxAvdev_CloseModule(int dev, int module) ;
extern int GxAvdev_CloseLogo(void);

typedef enum {
	LOGO_LAYER_VPP = GX_LAYER_VPP,
	LOGO_LAYER_SPP = GX_LAYER_SPP,
} LogoLayerID;
extern int GxAvdev_CloseLogo2(LogoLayerID layer);

extern int GxAvdev_AllocDemux(int tsid);
extern int GxAvdev_FreeDemux(int dmxid);
extern int GxAvdev_SppLock(void);
extern int GxAvdev_SppUnlock(void);

int   GxAvdev_LayerEnable         (int dev, int vpu, int layer, int flag);
int   GxAvdev_GetLayerEnable      (int dev, int vpu, int layer);
int   GxAvdev_SetLayerMainSurface (int dev, int vpu, int layer, void *surface);
int   GxAvdev_UnSetLayerMainSurface (int dev, int vpu, int layer, void *surface);
void *GxAvdev_GetLayerMainSurface (int dev, int vpu, int layer);
int   GxAvdev_SetLayerTop         (int dev, int vpu, int layer, int flag);
int   GxAvdev_GetLayerTop         (int dev, int vpu, int layer, int *flag);
int   GxAvdev_SetLayerViewport    (int dev, int vpu, int layer, GxAvRect* rect);
int   GxAvdev_GetLayerViewport    (int dev, int vpu, int layer, GxAvRect* rect);
int   GxAvdev_SetLayerClipport    (int dev, int vpu, int layer, GxAvRect* rect);
int   GxAvdev_SetLayerAntiFlicker (int dev, int vpu, int layer, int flag);
int   GxAvdev_SetSurfaceAlpha     (int dev, int vpu, void *surface, GxAlpha *alpha);
int   GxAvdev_SetVirtualResolution(int dev, int vpu, int width, int height);
int   GxAvdev_Memset(void *buffer, char value, unsigned int width, unsigned int height);

unsigned int GxAvdev_GetKernelVersion(void);
unsigned int GxAvdev_GetFirewallFlag(void);
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))

__END_DECLS

#endif
