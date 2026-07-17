#ifndef __GX_SUB_OUT_H__
#define __GX_SUB_OUT_H__

#include "gx_subreader.h"


typedef enum {
	SUB_RENDER_SPP,  
	SUB_RENDER_OSD,
	SUB_RENDER_STD,
}GxSubRenderType;

typedef struct  {
    int        (*init)(void* args);
    void       (*destroy)(void);
    void       (*draw)(int x0,int y0, int w,int h, unsigned char* src, unsigned char *srca, int stride,void* spu);
    void       (*clean)(void);
    void       (*hide)(void);
    void       (*show)(void);
}GxSubRender;

extern int GxSubRender_Init(int type,void* args);

extern void GxSubRender_Destory(void);

extern void GxSubRender_Draw(GxSubData *subdate);

extern void GxSubRender_Clean(GxSubData *sub);

extern void GxSubRender_Hide(GxSubData *sub);

extern void GxSubRender_Show(GxSubData *sub);




#endif
