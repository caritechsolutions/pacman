
#include "gx_mediamanager.h"

static int mediamgr_inited = -1;

#define MEDIA_MAX_MASTER 2

struct mediamgr_node {
	GxMedia*          media;
	int               cloneidx;
};

static struct mediamgr_node mediamgr[MEDIA_MAX_MASTER];

static int mediamgr_init(void)
{
	if(mediamgr_inited <= 0)
	{
		memset(mediamgr, 0, sizeof(mediamgr));
		mediamgr_inited = 1;
	}

	return 0;
}

static int issameprog(char* url1, char* url2)
{
	int fre[2], symb[2], vpid[2], apid[2], tsid[2], dmxid[2];

	if(url1 && url2)
	{
		GxUrl_GetItem(url1,GX_URL_KEY_FRE,  &fre[0]);
		GxUrl_GetItem(url1,GX_URL_KEY_SYM,  &symb[0]);
		GxUrl_GetItem(url1,GX_URL_KEY_VPID, &vpid[0]);
		GxUrl_GetItem(url1,GX_URL_KEY_APID, &apid[0]);
		GxUrl_GetItem(url1,GX_URL_KEY_TSID, &tsid[0]);
		GxUrl_GetItem(url1,GX_URL_KEY_DMXID, &dmxid[0]);
		tsid[0] = (tsid[0] < 0) ? 0 : tsid[0];
		dmxid[0] = (dmxid[0] < 0) ? 0 : dmxid[0];

		GxUrl_GetItem(url2,GX_URL_KEY_FRE,  &fre[1]);
		GxUrl_GetItem(url2,GX_URL_KEY_SYM,  &symb[1]);
		GxUrl_GetItem(url2,GX_URL_KEY_VPID, &vpid[1]);
		GxUrl_GetItem(url2,GX_URL_KEY_APID, &apid[1]);
		GxUrl_GetItem(url2,GX_URL_KEY_TSID, &tsid[1]);
		GxUrl_GetItem(url2,GX_URL_KEY_DMXID, &dmxid[1]);
		tsid[1] = (tsid[1] < 0) ? 0 : tsid[1];
		dmxid[1] = (dmxid[1] < 0) ? 0 : dmxid[1];

		if(fre[0] == fre[1] &&
		   symb[0] == symb[1] &&
		   tsid[0] == tsid[1] &&
		   (apid[0] == apid[1] ||vpid[0] == vpid[1]))
		return 1;
	}

	return 0;
}

static GxMedia* mediamgr_media_new(GxMediaType type, GxMediaPara* para)
{
	int i;
	GxMedia* media = NULL;

	for(i=0 ; i<MEDIA_MAX_MASTER; i++)
	{
		if(mediamgr[i].media &&
		   mediamgr[i].media->type == type &&
		   mediamgr[i].media->cloned < MEDIA_MAX_CLONE &&
		   issameprog(mediamgr[i].media->stream->url, (char*)para->recorder.srcurl))
		{
		   media = mediamgr[i].media;
		   break;
		}
	}

	if(media)
	{
		media->cloned++;
		for(i=0; i<MEDIA_MAX_CLONE+1; i++)
		{
			if(media->event[i].func == NULL)
			{
				media->event[i] = para->recorder.event;
				mediamgr[i].cloneidx = i;
				break;
			}
		}
		return media;
	}
	else
	{
		media = GxMedia_New(type, para);
		if(media)
		{
			for(i=0 ; i<MEDIA_MAX_MASTER; i++)
			{
				if(mediamgr[i].media == NULL)
				{
					mediamgr[i].media = media;
					break;
				}
			}
		}

		return media;
	}
}

static void mediamgr_media_destroy(GxMedia* media, int freeze)
{
	int i;

	for(i=0 ; i<MEDIA_MAX_MASTER; i++)
	{
		if(mediamgr[i].media == media)
		{
			if(media->cloned-- == 0)
			{
				GxMedia_Destroy(media, freeze);
				mediamgr[i].media = NULL;
			}
			else
			{
				media->event[mediamgr[i].cloneidx].func = NULL;
				media->event[mediamgr[i].cloneidx].priv = NULL;
			}

			return;
		}
	}

	GxMedia_Destroy(media, freeze);
}

GxMedia* GxMediaManager_MediaCreate(GxMediaType type, GxMediaPara* para)
{
	mediamgr_init();

	if(para)
	{
		if(type == GX_MEDIA_RECORDER)
		{
			return mediamgr_media_new(type, para);
		}
		else
		{
			return GxMedia_New(type, para);
		}
	}

	return NULL;
}

void GxMediaManager_MediaDestroy(GxMedia* media, int freeze)
{
	mediamgr_init();

	if(media)
	{
		if(media->type == GX_MEDIA_RECORDER)
			mediamgr_media_destroy(media, freeze);
		else
			GxMedia_Destroy(media, freeze);
	}
}

GxMedia* GxMediaManager_MediaRestart(GxMedia* media, GxMediaPara* para)
{
	mediamgr_init();

	if(para)
	{
		if(media->type == GX_MEDIA_RECORDER)
		{
			return media;
		}
		else
		{
			return GxMedia_Restart(media, para);
		}
	}

	return NULL;
}
