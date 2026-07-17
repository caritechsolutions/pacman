
#include "vo.h"


static int vo_dummy_open(GxVideoOut* vo)
{
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int vo_dummy_control(GxVideoOut*  vo, int cmd, void* args)
{
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static FILE*  vo_dummy_fp = NULL;

static int vo_dummy_close(GxVideoOut*  vo)
{
	fclose(vo_dummy_fp);
	vo_dummy_fp = NULL;
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int vo_dummy_draw_frame(GxVideoOut* vo, uint8_t *frame, int len, int type)
{
	if(vo_dummy_fp == NULL)
	{
#ifdef I386_LINUX
		{
			char Name[128] = {0};
			char* dot = vo->header->ds->demuxer->stream->url;
			char* postfixptr = NULL;

			if(strstr(dot,"/"))
			{
				while(dot)
				{
					postfixptr = ++dot;
					dot = strchr(dot,'/');
				}
			}
			else
				postfixptr = dot;

			snprintf(Name, sizeof(Name), "%s", postfixptr);

			dot = strstr(Name,".");
			*dot = '\0';

			strcat(Name,".m2v", strlen(".m2v"));

			vo_dummy_fp = fopen(Name,"wb+");
		}
#endif

	}

	if(vo_dummy_fp){
		fwrite((unsigned char* )frame,len,1,vo_dummy_fp);
		return GX_PLAYER_OK;
	}

	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}


GxVideoOutClass gx_vout_dummy = {
	._inherit = {
		._inherit = {
			.name    = "DummyVo",
			.parent  = (void* )&gx_VideoOutBase,
			.size    = sizeof(GxVideoOut),
		},
		.run    = NULL,
		.pause  = NULL,
		.resume = NULL,
		.stop   = NULL,
	},
	DEF_AUTHOR("VideoOut","Dummy","No description","L.F","No comment"),

	.name       = "Dummy Video Player",
	.open       = vo_dummy_open,
	.close      = vo_dummy_close,
	.control    = vo_dummy_control,
	.draw_frame = vo_dummy_draw_frame,
	.draw_sub   = NULL,
};



