#ifndef __GX_GDI_LAYER_SPP_H__
#define __GX_GDI_LAYER_SPP_H__

int GXGDI_LayerSppCreate(GxSubtitleShowCanvas *cv, GxSubtitleShowScreen *sr);
int GXGDI_LayerSppDestory(void);
int GXGDI_LayerSppDraw(GuiSurface *surface, GXGDI_Rect *srcRect, GXGDI_Rect *dstRect);
int GXGDI_LayerSppClean (GXGDI_Rect *dstRect);
int GXGDI_LayerSppScreen(GXGDI_Rect *rect);
int GXGDI_LayerSppShow(void);
int GXGDI_LayerSppHide(void);
int GXGDI_LayerSppGetModule(int *dev, int *module);

#endif
