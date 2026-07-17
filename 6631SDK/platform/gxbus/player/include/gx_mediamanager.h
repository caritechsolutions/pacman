#ifndef __GX_MEDIA_MANAGER_H__
#define __GX_MEDIA_MANAGER_H__

#include "gx_media.h"

typedef struct {

}GxMediaManager;

GxMedia* GxMediaManager_MediaCreate(GxMediaType type, GxMediaPara* para);

GxMedia* GxMediaManager_MediaRestart(GxMedia* media, GxMediaPara* para);

void GxMediaManager_MediaDestroy(GxMedia* media, int freeze);


#endif
