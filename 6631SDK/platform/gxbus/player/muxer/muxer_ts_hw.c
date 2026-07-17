
#include "gx_muxer.h"
#include "../demuxer/hw_demux.h"

typedef struct {
	int                  index   ;
}MuxerHwPriv;


extern GxMuxerClass gx_MuxerBase;

#if 0
int muxer_hw_run(GxMediaFilter*  filter)
{
	MuxerHwPriv* priv ;
	GxDemuxer* muxer = GXDEMUXER(filter);

	if(muxer)
	{
		priv = (MuxerHwPriv*)muxer->priv;
		if(priv)
		{
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			return 	GxHwDemux_Run(priv->index,HW_DEMUX_FUNC_MUXER);
		}
	}
	return GX_PLAYER_ERROR;
}

int muxer_hw_pause(GxMediaFilter*  filter)
{
	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

int muxer_hw_resume(GxMediaFilter*  filter)
{
	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

int muxer_hw_stop(GxMediaFilter*  filter)
{
	MuxerHwPriv* priv ;
	GxDemuxer* muxer = GXDEMUXER(filter);

	if(muxer)
	{
		priv = (MuxerHwPriv*)muxer->priv;
		if(priv)
		{
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			return GxHwDemux_Stop(priv->index,HW_DEMUX_FUNC_MUXER);
		}
	}
	return GX_PLAYER_ERROR;
}

int muxer_hw_config(GxMediaFilter*  filter)
{
	MuxerHwPriv* priv = NULL;
	GxMuxer* muxer = GXMUXER(filter);
	HwDemuxConf conf;
	int v_id = -1;
	int a_id = -1;

	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	if(muxer)
	{
		GxUrl_GetItem(muxer->stream->url,GX_URL_KEY_VPID, &v_id );
		GxUrl_GetItem(muxer->stream->url,GX_URL_KEY_APID, &a_id );

		if(INVAILD_PID(a_id)||INVAILD_PID(v_id))
			return GX_PLAYER_ERROR;

		priv = (MuxerHwPriv*)muxer->priv;
		if(priv)
		{
			conf.a_id   = a_id;
			conf.v_id   = v_id;
			conf.tspin  = GxMediaFilter_FindPin(filter,GX_MUXER_PIN_NAME_FO);
			conf.apin   = GxMediaFilter_FindPin(filter,GX_MUXER_PIN_NAME_AI);
			conf.vpin   = GxMediaFilter_FindPin(filter,GX_MUXER_PIN_NAME_VI);
			conf.type   = HW_DEMUX_FUNC_MUXER;
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

			return GxHwDemux_Config(priv->index,&conf);
		}
	}
	return GX_PLAYER_ERROR;
}

#endif

static void* muxer_hw_create(GxObject * obj)
{
	return obj;
}

static void muxer_hw_release(GxObject*  obj)
{
	return;
}

static int  muxer_ts_hw_open(GxMuxer* m){
/*	MuxerHwPriv* priv = NULL;

	priv = (void*)av_malloc(sizeof(MuxerHwPriv));
	if (priv == NULL)
		return GX_PLAYER_ERROR ;

	m->priv = (void* )priv;
	memset(m->priv, 0, sizeof(MuxerHwPriv));

	priv->index = m->index;

	GxHwDemux_OpenModule(priv->index);
	GxHwDemux_SetSource(priv->index,DEMUX_TS1);

	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);*/
	return GX_PLAYER_OK;
}

static void muxer_ts_hw_fix_stream_parameters (GxMuxerStream* ms){

}

static void muxer_ts_hw_write_chunk(GxMuxerStream*ms,size_t size,unsigned int len, double dts, double pts){

}
static void muxer_ts_hw_write_header (GxMuxer* m){

}
static void muxer_ts_hw_write_index (GxMuxer* m){

}
static GxMuxerStream* muxer_ts_hw_new_stream(GxMuxer*m,int type){

	return NULL;
}

GxMuxerClass gx_muxer_ts_hw = {
	._inherit = {
		._inherit = {
			.name    = "Muxer TS",
			.parent  = &gx_MuxerBase,
			.size    = sizeof(GxMuxer),
			.create  = muxer_hw_create,
			.release = muxer_hw_release,
			.event   = NULL,
		},
		.run    = NULL,
		.pause  = NULL,
		.resume = NULL,
		.config = NULL,
		.stop   = NULL,
	},
	DEF_AUTHOR("Muxer","ts","No description","L.F","No comment"),

	.name           = "Muxer TS",
	.type           = GX_MUXER_TYPE_TSHW,
	.open           = muxer_ts_hw_open,
	.fix_parameters = muxer_ts_hw_fix_stream_parameters,
	.write_chunk    = muxer_ts_hw_write_chunk,
	.write_header   = muxer_ts_hw_write_header,
	.write_index    = muxer_ts_hw_write_index,
	.new_stream     = muxer_ts_hw_new_stream,
};

