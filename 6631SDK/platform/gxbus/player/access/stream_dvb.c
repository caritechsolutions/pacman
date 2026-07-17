#include "gx_stream.h"
#include "gx_media.h"
#include "dvbsource.h"

typedef struct {
	DVBSource* source;
	int running;
}StreamDvbPriv;

static int dvb_open(GxStream* s, int mode)
{
	StreamDvbPriv* priv = NULL;

	priv = (void* )av_mallocz(sizeof(StreamDvbPriv));
	if (priv == NULL)
		return GX_PLAYER_ERROR;

	priv->source = dvbsource_open(s->url);

	s->priv = (void* )priv;
	s->file_format = GX_STREAMTYPE_DVB;

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int dvb_control(GxStream *s, int cmd, void *arg)
{
	int ret = GX_PLAYER_OK;

	switch (cmd) {
	case GX_STREAM_CTRL_SWITCH_AUDIO:
		{
			if (s && s->priv) {
				StreamDvbPriv* priv = s->priv;
				GxStreamStreamSwitch* StreamSwitch = (GxStreamStreamSwitch*)arg;

				dvbsource_audio_switch(priv->source, StreamSwitch->pid);
			}
			break;
		}
	case GX_STREAM_CTRL_ADD_TRACK:
		{
			if (s && s->priv){
				StreamDvbPriv* priv = s->priv;
				ret = dvbsource_track_add(priv->source, arg);
			}
			break;
		}
	case GX_STREAM_CTRL_DEL_TRACK:
		{
			if (s && s->priv){
				StreamDvbPriv* priv = s->priv;
				ret = dvbsource_track_del(priv->source, arg);
			}
			break;
		}
	case GX_STREAM_CTRL_CHANGE_URL:
		{
			if (s) {
				av_free(s->url);
				s->url = av_strdup(arg);
				if (s && s->original_url) {
					av_free(s->original_url);
					s->original_url = av_strdup(arg);
				}
			}
			return GX_PLAYER_OK;
		}
	default:
		{
			if (s && s->priv){
				StreamDvbPriv* priv = s->priv;
				dvbsource_getcls(priv->source, s->url);
				ret = dvbsource_control(priv->source, cmd, arg);
			}
			break;
		}
	}

	return ret;
}

static int dvb_stop(GxMediaFilter *filter)
{
	GxStream* s = GXSTREAM(filter);

	if (s && s->priv){
		StreamDvbPriv* priv = s->priv;
		if(priv->running)
			dvbsource_stop(priv->source);
		priv->running = 0;
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static void dvb_close(GxStream *s)
{
	StreamDvbPriv* priv ;
	if(s)
	{
		priv = (StreamDvbPriv*)s->priv;
		dvb_stop((GxMediaFilter*)s);
		if(priv)
		{
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			dvbsource_close(priv->source);
			av_free(priv);
		}
		if (s->url){
			av_free(s->url);
			s->url = NULL;
		}
	}
	return ;
}

static int dvb_config(GxMediaFilter *filter)
{
	GxStream* s = GXSTREAM(filter);

	if (s && s->priv){
		DVBSource ds = { 0 };
		StreamDvbPriv* priv = s->priv;
		dvbsource_getcls(&ds, s->url);
		if (ds.type != ((DVBSource*)(priv->source))->type) {
			dvbsource_close(priv->source);
			priv->source = dvbsource_open(s->url);
		}
		else {
			dvbsource_getcls(priv->source, s->url);
		}

		if(dvbsource_config(priv->source, s->url, s->options) == GX_PLAYER_ERROR)
			return GX_PLAYER_ERROR;
		priv->running = 1;
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int dvb_run(GxMediaFilter *filter)
{
	GxStream* s = GXSTREAM(filter);

	if (s && s->priv){
		StreamDvbPriv* priv = s->priv;
		if(priv->running)
			dvbsource_run(priv->source);
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int dvb_pause(GxMediaFilter *filter)
{
	GxStream* s = GXSTREAM(filter);

	if (s && s->priv){
		StreamDvbPriv* priv = s->priv;
		dvbsource_pause(priv->source);
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int dvb_resume(GxMediaFilter *filter)
{
	GxStream* s = GXSTREAM(filter);

	if (s && s->priv){
		StreamDvbPriv* priv = s->priv;
		dvbsource_resume(priv->source);
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

GxStreamClass gx_stream_dvb = {
	._inherit = {
		._inherit     = {
			.name    = "StreamDVB",
			.parent  = &gx_streambase,
			.size    = sizeof(GxStream),
		},
		.run     = dvb_run,
		.stop    = dvb_stop,
		.pause   = dvb_pause,
		.resume  = dvb_resume,
		.config  = dvb_config,
	},
	.protocols = {"dvbc", "dvbt","dvbs", "dtmb", "dvbt2", "dvbnet", NULL},
	DEF_AUTHOR("Stream","dvb","No description","L.F","No comment"),

	.flags   = 0,
	.open    = dvb_open,
	.read    = NULL,
	.write   = NULL,
	.seek    = NULL,
	.control = dvb_control,
	.close   = dvb_close,
};

