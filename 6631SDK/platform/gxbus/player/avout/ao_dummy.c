
#include "ao.h"

static int ao_dummy_open(GxAudioOut* ao)
{
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int ao_dummy_control(GxAudioOut*  ao, int cmd, void* args)
{
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static FILE*  ao_dummy_fp = NULL;

static int ao_dummy_close(GxAudioOut*  ao , int immed)
{
	fclose(ao_dummy_fp);
	ao_dummy_fp = NULL;
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int ao_dummy_play(GxAudioOut* ao, void* data, int len, int flags)
{
	if(ao_dummy_fp == NULL)
	{
		char Name[128] = {0};
		char* dot = ao->header->ds->demuxer->stream->url;
#ifdef I386_LINUX
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
#else
		snprintf(Name, sizeof(Name), "%s", dot);
#endif
		strncat(Name,".m2a", strlen(".m2a"));

		ao_dummy_fp = fopen(Name,"wb+");
	}
	if(ao_dummy_fp){
		fwrite((unsigned char* )data,len,1,ao_dummy_fp);
		return GX_PLAYER_OK;
	}

	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_ERROR;
}

GxAudioOutClass gx_aout_dummy = {
	._inherit = {
		._inherit = {
			.name    = "DummyAo",
			.parent  = (void* )&gx_AudioOutBase,
			.size    = sizeof(GxAudioOut),
		},
		.run    = NULL,
		.pause  = NULL,
		.resume = NULL,
		.stop   = NULL,
	},
	DEF_AUTHOR("AudioOut","dummy","No description","L.F","No comment"),

	.name    = "Dummy Audio Player",
	.open    = ao_dummy_open,
	.close   = ao_dummy_close,
	.control = ao_dummy_control,
	.play    = ao_dummy_play,
};



