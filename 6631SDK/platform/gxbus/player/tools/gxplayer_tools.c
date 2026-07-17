#include "gx_media.h"
#include "gxmsg.h"
#include "gxplayer_internal.h"
#include "../include/module/subtitle/gxsubtitle.h"
#include "../include/module/player/gxplayer_module.h"


status_t GxPlayer_DumpPCM(const char* name,const char* url)
{
	status_t ret=GXCORE_ERROR;                 
	GxPlayer* p;
	GxMediaPara MediaPara;
	
	p = GxPlayer_Create(name);
	if(p == NULL || url == NULL)
		return ret;

	memset(&MediaPara, 0, sizeof(MediaPara));
	MediaPara.pcmdumper.url = url;
	p->media_play = GxMedia_New(GX_MEDIA_PROBER, &MediaPara);

	if(p->media_play)
	{
		GxMedia_Config(p->media_play);
		GxMedia_Run(p->media_play);
	}

	return ret;    
}


