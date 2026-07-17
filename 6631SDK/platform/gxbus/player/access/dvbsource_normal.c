#include "dvbsource.h"

typedef struct {
	handle_t dev;
	handle_t dmx;
}dvbnormal;

void* dvbnormal_open(const char* url)
{
	dvbnormal* dn = av_mallocz(sizeof(dvbnormal));

	if (dn) {
		dn->dev = GxAvdev_CreateDevice(0);
	}

	return dn;
}

static int dvbnormal_config(void *handle, const char* url, GxOptions *op)
{
	int ret;
	int tsid, dmxid, tsmode;
	dvbnormal* dn = handle;
	GxDemuxProperty_ConfigDemux cfg_dmx = {
		.stream_mode = 0,
		.ts_select   = 0,
		.source      = 0,
		.time_gate        = 0xf,
		.sync_lock_gate   = 0x3,
		.sync_loss_gate   = 0x3,
		.byt_cnt_err_gate = 0x3
	};

	GxUrl_GetItem(url,GX_URL_KEY_TSID, &tsid);
	GxUrl_GetItem(url,GX_URL_KEY_DMXID, &dmxid);
	GxUrl_GetItem(url,GX_URL_KEY_TSMODE, &tsmode);

	tsid = tsid<0 ? 0 : tsid;
	dmxid = dmxid<0 ? 0 :dmxid;
	tsmode = tsmode<0 ? 0 :tsmode;

	dn->dmx = GxAvdev_OpenModule(dn->dev, GXAV_MOD_DEMUX, dmxid);

	cfg_dmx.source = tsid;
	cfg_dmx.stream_mode = tsmode;
	GxAVSetProperty(dn->dev, dn->dmx, GxDemuxPropertyID_Config,
			&cfg_dmx, sizeof(GxDemuxProperty_ConfigDemux));

	ret = dvbsource_set_tp(dn->dev, dn->dmx, url, op);
	if(ret < 0) {
		if (dn->dmx > 0) {
			GxAvdev_CloseModule(dn->dev, dn->dmx);
			dn->dmx = -1;
		}
		GxAvdev_DestroyDevice(dn->dev);
		av_free(dn);
		return -1;
	}

	return 0;
}

static int dvbnormal_run(void* handle)
{
	return 0;
}

static int dvbnormal_disconnect(void* handle, const char* url)
{
	dvbnormal* dn = handle;

	if(dn) {
		dvbsource_disconnect_l(dn->dev, dn->dmx, url);
	}

	return 0;
}

static int dvbnormal_stop(void* handle)
{
	dvbnormal* dn = handle;

	if(dn) {
		if (dn->dmx > 0) {
			GxAvdev_CloseModule(dn->dev, dn->dmx);
			dn->dmx = -1;
		}
	}

	return 0;
}

static int dvbnormal_close(void* handle)
{
	dvbnormal* dn = handle;

	if(dn) {
		if (dn->dmx > 0) {
			GxAvdev_CloseModule(dn->dev, dn->dmx);
			dn->dmx = -1;
		}
		GxAvdev_DestroyDevice(dn->dev);
		av_free(dn);
	}

	return 0;
}

DVBSourceClass dvbsource_normal= {
	.type  = DVBSOURCE_NORMAL,
	.open = dvbnormal_open,
	.config = dvbnormal_config,
	.pause = NULL,
	.resume = NULL,
	.run = dvbnormal_run,
	.stop = dvbnormal_stop,
	.close = dvbnormal_close,
	.disconnect = dvbnormal_disconnect,
	.track_add = NULL,
	.track_del = NULL
};

