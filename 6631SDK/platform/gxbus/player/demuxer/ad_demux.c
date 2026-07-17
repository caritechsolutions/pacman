#include "gx_config.h"
#include "gx_common.h"
#include "hw_demux.h"
#include "stheader.h"
#include "ad_demux.h"

AdDemuxer* GxAdDemux_Open(int pid, int source)
{
	AdDemuxer *ad_dmx   = NULL;

	gxlogf("[Demuxer]: %s %d\n", __func__, __LINE__);
	if (!VAILD_PID(pid)) {
		gxloge("[Demuxer]: %s %d (pid %d)\n", __func__, __LINE__, pid);
		return NULL;
	}

	ad_dmx = av_mallocz(sizeof(AdDemuxer));
	if (ad_dmx == NULL) {
		gxloge("[Demuxer]: %s %d malloc failed\n", __func__, __LINE__);
		return NULL;
	}

	ad_dmx->module = -1;
	ad_dmx->progid = -1;
	//ad demux same with tsr，example：
	//dvb play 0 ** dvb record 0 ** file play 1 ** ad play 1
	//dvb play 0 ** dvb record 1 ** file play 0 ** ad play 0
	GxPlayer_SystemGet(PSYS_PVR_PLAY_DEMUX_ID, &ad_dmx->module);
	if(ad_dmx->module < 0) {
		gxloge("[error]: AdDemux dmxid not defined!\n");
		return NULL;
	}

	GxHwDemux_OpenModule(ad_dmx->module);
	GxHwDemux_GetSource(ad_dmx->module, &ad_dmx->default_source);
	GxHwDemux_SetSource(ad_dmx->module, source);

	ad_dmx->pid    = pid;
	ad_dmx->progid = GxHwDemux_AllocProg(ad_dmx->module);

	return ad_dmx;
}

int GxAdDemux_Config(AdDemuxer *ad_dmx, GxFifo *fifo, int bind_descr)
{
	gxlogf("[Demuxer]: %s %d\n", __func__, __LINE__);

	if (ad_dmx && fifo && !ad_dmx->running) {
		ad_dmx->conf.bind_descr = bind_descr;
		ad_dmx->conf.progid = ad_dmx->progid;
		ad_dmx->conf.type   = HW_DEMUX_FUNC_MUXER;
		ad_dmx->conf.afifo  = 0;
		ad_dmx->conf.vfifo  = 0;
		ad_dmx->conf.a_id   = -1;
		ad_dmx->conf.v_id   = -1;
		ad_dmx->conf.tsfifo = fifo;
		ad_dmx->conf.ts_info.ext_info.ext_num      = 1;
		ad_dmx->conf.ts_info.ext_info.ext_pids [0] = ad_dmx->pid;
		ad_dmx->conf.ts_info.ext_info.ext_types[0] = DEMUX_SLOT_AUDIO;

		if (GxHwDemux_Config(ad_dmx->module, &ad_dmx->conf) == GX_PLAYER_OK) {
			GxFifoLink FifoLink;

			FifoLink.module = GxHwDemux_GetDVRMod(ad_dmx->module, ad_dmx->progid);
			FifoLink.pin_id = GxHwDemux_GetSlotID(ad_dmx->module, ad_dmx->pid);
			FifoLink.direction = GXAV_PIN_OUTPUT;
			GxFifo_Reset(fifo);
			GxFifo_Link (fifo, &FifoLink);

			ad_dmx->fifo = fifo;
			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_ERROR;
}

int GxAdDemux_Run(AdDemuxer *ad_dmx)
{
	gxlogf("[Demuxer]: %s %d\n", __func__, __LINE__);

	if (ad_dmx && !ad_dmx->running) {
		GxHwDemux_Run(ad_dmx->module, ad_dmx->progid, HW_DEMUX_FUNC_MUXER);
		ad_dmx->running = 1;
	}
	return GX_PLAYER_OK;
}

int GxAdDemux_Stop(AdDemuxer *ad_dmx)
{
	gxlogf("[Demuxer]: %s %d\n", __func__, __LINE__);

	if (ad_dmx && ad_dmx->running) {
		GxHwDemux_Stop(ad_dmx->module, ad_dmx->progid, HW_DEMUX_FUNC_MUXER);
		if (ad_dmx->fifo) {
			GxFifoLink FifoLink;

			FifoLink.module = GxHwDemux_GetDVRMod(ad_dmx->module, ad_dmx->progid);
			FifoLink.pin_id = GxHwDemux_GetSlotID(ad_dmx->module, ad_dmx->pid);
			FifoLink.direction = GXAV_PIN_OUTPUT;
			GxFifo_Unlink(ad_dmx->fifo, &FifoLink);
		}
		ad_dmx->running = 0;
	}

	return GX_PLAYER_OK;
}

void GxAdDemux_Close(AdDemuxer* ad_dmx)
{
	gxlogf("[Demuxer]: %s %d\n", __func__, __LINE__);

	if (ad_dmx) {
		if (ad_dmx->progid >= 0) {
			GxHwDemux_FreeProg(ad_dmx->module, ad_dmx->progid);
			ad_dmx->progid = -1;
		}

		if (ad_dmx->module >= 0) {
			GxHwDemux_SetSource(ad_dmx->module, ad_dmx->default_source);
			GxHwDemux_CloseModule(ad_dmx->module);
			ad_dmx->module = -1;
		}

		av_free(ad_dmx);
	}

	return;
}
