
#include "dvbsource.h"
#include "dvbsource_tsbuff.h"
#include "gx_mediafilter.h"
#include "gx_system.h"

#define PID_VAILD(pid)   (pid >  0 && pid <  0x1FFF)
#define PID_INVAILD(pid) (pid <= 0 || pid >= 0x1FFF)

static TSBuff _tsbuff;
static int _tsbuff_inited = 0;
static PlayerStreamPortCallback tsbuff_stream_inside;

static int tsbuff_slot_find(void* handle, int pid)
{
	int i;
	TSBuff* tsd = handle;

	for(i=0; i<TSBUFF_SLOT_MAX; i++) {
		if(tsd->slot[i].pid == pid)
			return i;
	}

	return -1;
}

static int tsbuff_slot_add(void* handle, GxDemuxProperty_Slot slot)
{
	int i;
	TSBuff* tsd = handle;

	i = tsbuff_slot_find(handle, slot.pid);
	if (i >= 0) {
		tsd->slotusecnt[i]++;
		return 0;
	}

	for(i=0; i<TSBUFF_SLOT_MAX; i++) {
		if(tsd->slotusecnt[i] == 0) {
			tsd->slot[i] = slot;
			tsd->slotusecnt[i]++;
			tsd->slot_nb++;
			return 0;
		}
	}

	return -1;
}

static int tsbuff_slot_del(void* handle, unsigned short pid)
{
	int i;
	TSBuff* tsd = handle;
	GxDemuxProperty_Slot slot;

	i = tsbuff_slot_find(handle, pid);
	if (i >= 0) {
		if(--tsd->slotusecnt[i] == 0) {
			slot = tsd->slot[i];
			GxAVSetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotDisable, &slot, sizeof(slot));
			GxAVSetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotFree, &slot, sizeof(slot));
			memset(&tsd->slot[i], 0, sizeof(slot));
			tsd->slot[i].pid = -1;
			tsd->slot_nb--;
		}
		return 0;
	}

	return -1;
}

static void tsb_cleanup_dmx(TSBuff* tsd)
{
	if(tsd->dmx_r > 0)
		GxAvdev_CloseModule(tsd->dev, tsd->dmx_r);
	if(tsd->dmx_w > 0)
		GxAvdev_CloseModule(tsd->dev, tsd->dmx_w);
	if(tsd->dev > 0)
		GxAvdev_DestroyDevice(tsd->dev);
}

static void tsb_cleanup_fifo(TSBuff* tsd)
{
	GxFifoLink FifoLink;

	if(tsd->fifo_r) {
		GxFifo_Destroy(tsd->fifo_r);
		tsd->fifo_r = NULL;
	}

	if(tsd->fifo_w) {
		FifoLink.module = tsd->dmx_w;
		GxFifo_Destroy(tsd->fifo_w);
		tsd->fifo_w = NULL;
	}

	tsd->stream.Uninit(tsd->priv);
	tsd->priv = 0;
}

static void tsb_cleanup_slot(TSBuff* tsd)
{
	int i;
	GxDemuxProperty_Slot slot;

	for (i=0; i<TSBUFF_SLOT_MAX; i++) {
		if(tsd->slotusecnt[i] > 0) {
			slot = tsd->slot[i];
			GxAVSetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotDisable, &slot, sizeof(slot));
			GxAVSetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotFree, &slot, sizeof(slot));
			tsd->slotusecnt[i] = 0;
			memset(&tsd->slot[i], 0, sizeof(slot));
			tsd->slot[i].pid = -1;
		}
	}

	tsd->slot_nb = 0;
}

#define TSD_CHECK_RET(ret)\
	do {\
		if(ret < 0) \
		{\
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);\
			goto ERR;\
		}\
	}while(0);

#define TSD_CHECK_NULL(p)\
	do {\
		if(p == NULL) \
		{\
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);\
			goto ERR;\
		}\
	}while(0);


static int tsb_init_slot(TSBuff* tsd)
{
	int ret;
	GxFifoConfig FifoConfig;
	GxDemuxProperty_Slot slot;

	if (tsd->kernel_mode == 0) {
		memset(&FifoConfig, 0, sizeof(FifoConfig));
		FifoConfig.dev = tsd->dev;
		FifoConfig.dvrid = tsd->dmxid;
		FifoConfig.source = DVR_INPUT_MEM;
		FifoConfig.dest = DVR_OUTPUT_DMX;
		FifoConfig.hw_buffer_size = tsd->fifo_w->size;
		ret = GxFifo_Config(tsd->fifo_w, &FifoConfig);
		TSD_CHECK_RET(ret);
		if (1) {
			GxDvrProperty_TSRFlowControl ctrl = {0};
			ctrl.flags = DVR_FLOW_CONTROL_ES;
			GxAVSetProperty(FifoConfig.dev, FifoConfig.dvrhandle,
					GxDvrPropertyID_TSRFlowControl, &ctrl, sizeof(GxDvrProperty_TSRFlowControl));
		}
	}

	memset(&FifoConfig, 0, sizeof(FifoConfig));
	FifoConfig.dev = tsd->dev;
	FifoConfig.dvrid = tsd->delaydmx;
	FifoConfig.source = DVR_INPUT_DMX;
	FifoConfig.dest = tsd->kernel_mode ? DVR_OUTPUT_DVR2 : DVR_OUTPUT_MEM;
	FifoConfig.sw_buffer_size = tsd->fifo_r->size;
	if (!PID_VAILD(tsd->vpid)) {
		FifoConfig.hw_buffer_size = 188 * 1000;
		FifoConfig.almost_full_gate = 188 *30;
	}
	ret = GxFifo_Config(tsd->fifo_r, &FifoConfig);
	TSD_CHECK_RET(ret);

	slot.flags = (DMX_TSOUT_EN | DMX_CRC_DISABLE | DMX_REPEAT_MODE);
	slot.ts_out_pin = tsd->ts_out_pin = FifoConfig.dvrhandle;
//	slot.pid = 0x1fff;

	if(PID_VAILD(tsd->vpid)) {
		slot.pid = tsd->vpid;
		slot.type = DEMUX_SLOT_VIDEO;
		slot.flags |= DMX_ERR_DISCARD_EN;
		ret = GxAVGetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotAlloc, &slot, sizeof(slot));
		TSD_CHECK_RET(ret);

		ret = GxAVSetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotConfig, &slot, sizeof(slot));
		TSD_CHECK_RET(ret);

		tsbuff_slot_add(tsd, slot);
	}

	if(PID_VAILD(tsd->apid)) {
		slot.pid = tsd->apid;
		slot.type = DEMUX_SLOT_AUDIO;
		slot.flags |= DMX_ERR_DISCARD_EN;
		ret = GxAVGetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotAlloc, &slot, sizeof(slot));
		TSD_CHECK_RET(ret);

		ret = GxAVSetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotConfig, &slot, sizeof(slot));
		TSD_CHECK_RET(ret);

		tsbuff_slot_add(tsd, slot);
	}

	if(PID_VAILD(tsd->pcrpid)) {
		slot.pid = tsd->pcrpid;
		slot.type = DEMUX_SLOT_PES;
		ret = GxAVGetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotAlloc, &slot, sizeof(slot));
		TSD_CHECK_RET(ret);

		ret = GxAVSetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotConfig, &slot, sizeof(slot));
		TSD_CHECK_RET(ret);

		tsbuff_slot_add(tsd, slot);
	}

	return 0;
ERR:
	tsb_cleanup_slot(tsd);
	return -1;
}

static int tsb_init_fifo(TSBuff* tsd)
{
	int rcachesize;

	memcpy(&tsd->stream, &tsbuff_stream_inside, sizeof(PlayerStreamPortCallback));
	GxPlayer_SystemGet(PSYS_CBK_STREAM_PORT, &tsd->stream);
	GxPlayer_SystemGet(PSYS_DELAY_CACHE_HW_SIZE, &rcachesize);

	tsd->fifo_r = GxFifo_Create(rcachesize/(188*8)*(188*8), GX_PINFLAG_MUXTS);
	TSD_CHECK_NULL(tsd->fifo_r);
	if (tsd->kernel_mode == 0) {
		tsd->fifo_w = GxFifo_Create(TSBUFF_FIFO_W_SIZE, GX_PINFLAG_MUXTS);
		TSD_CHECK_NULL(tsd->fifo_w);

		tsd->priv = tsd->stream.Init(TSBUFF_BLOCK_SIZE, tsd->delayms);
		if (tsd->priv != 0)
			return 0;
	}

ERR:
	tsb_cleanup_fifo(tsd);
	return -1;
}

static int tsb_init_dmx(TSBuff* tsd, const char* url, GxOptions *op)
{
	int ret;

	GxDemuxProperty_ConfigDemux cfg_dmx = {
		.stream_mode = 0,
		.ts_select   = 0,
		.source      = 0,
		.time_gate        = 0xf,
		.sync_lock_gate   = 0x3,
		.sync_loss_gate   = 0x3,
		.byt_cnt_err_gate = 0x3
	};

	tsd->dev = GxAvdev_CreateDevice(0);

	tsd->dmx_r = GxAvdev_OpenModule(tsd->dev, GXAV_MOD_DEMUX, tsd->delaydmx);
	TSD_CHECK_RET(tsd->dmx_r);
	tsd->dmx_w = GxAvdev_OpenModule(tsd->dev, GXAV_MOD_DEMUX, tsd->dmxid);
	TSD_CHECK_RET(tsd->dmx_w);

	cfg_dmx.source = tsd->tsid;
	ret = GxAVSetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_Config, &cfg_dmx, sizeof(cfg_dmx));
	TSD_CHECK_RET(ret);

	cfg_dmx.source = 3;//DEMUX_SDRAM;
	ret = GxAVSetProperty(tsd->dev, tsd->dmx_w, GxDemuxPropertyID_Config, &cfg_dmx, sizeof(cfg_dmx));
	TSD_CHECK_RET(ret);

	ret = dvbsource_set_tp(tsd->dev, tsd->dmx_r, url, op);
	TSD_CHECK_RET(ret);

	return 0;

ERR:
	tsb_cleanup_dmx(tsd);
	return -1;
}

static void tsb_run_slot(TSBuff *tsd)
{
	int i;
	GxDemuxProperty_Slot slot;

	for (i=0; i< tsd->slot_nb; i++) {
		slot = tsd->slot[i];
		GxAVSetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotEnable, &slot, sizeof(slot));
	}
}

static void tsb_stop_slot(TSBuff* tsd)
{
	int i;
	GxDemuxProperty_Slot slot;

	for (i=0; i< tsd->slot_nb; i++) {
		slot = tsd->slot[i];
		GxAVSetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotDisable, &slot, sizeof(slot));
	}
}

static uint32_t tsb_current_timems(void)
{
	GxTime Time;

	GxCore_GetTickTime(&Time);

	return Time.seconds*1000 + Time.microsecs/1000;
}

static void tsb_wthread(void* data)
{
	TSBuff *tsd = data;

	while(tsd->running) {
		int ret;
		GxPlayerDataPacket pkt;
		ret = tsd->stream.GetPacket(tsd->priv, &pkt);
		if (ret == 0) {
			ssize_t wsize, size = pkt.size;
			wsize = GxFifo_Write(tsd->fifo_w, pkt.data, size, 100000);
			while(wsize < size && tsd->running) {
				wsize += GxFifo_Write(tsd->fifo_w, pkt.data + wsize,  size - wsize, 100000);
				GxCore_ThreadDelay(10);
			}
			tsd->stream.DelPacket(tsd->priv, &pkt);
		}
		else {
			GxCore_ThreadDelay(10);
		}
	}
}

static void tsb_rthread(void* data)
{
	TSBuff *tsd = data;
	GxPlayerDataPacket pkt;

	memset(&pkt, 0, sizeof(pkt));

	while(tsd->running) {
		size_t size, rsize;
		if (pkt.data == NULL) {
			int ret = tsd->stream.NewPacket(tsd->priv, &pkt, TSBUFF_BLOCK_SIZE);
			if (ret != 0) {
				gxlogd("[tsbuff] OOM  #########\n");
				GxCore_ThreadDelay(10);
				continue;
			}
		}
		size = pkt.size;
		rsize = GxFifo_Read(tsd->fifo_r, pkt.data, size, 10000);
		while(rsize < size && tsd->running) {
			rsize += GxFifo_Read(tsd->fifo_r, pkt.data + rsize, size - rsize, 10000);
			GxCore_ThreadDelay(10);
		}
		if (rsize > 0) {
			pkt.size = rsize;
			tsd->stream.PutPacket(tsd->priv, &pkt);
			pkt.data = NULL;
		}
		else {
			GxCore_ThreadDelay(10);
		}
	}
}

static void tsb_run_thread(TSBuff *tsd)
{
	GxCore_ThreadCreate("tsbuff read thread", &tsd->pthread_r,
			tsb_rthread, tsd, 1024*16, TSBUFF_PTHREAD_PRIORITY);

	GxCore_ThreadCreate("tsbuff write thread", &tsd->pthread_w,
			tsb_wthread, tsd, 1024*16, TSBUFF_PTHREAD_PRIORITY);
}

static void tsb_stop_thread(TSBuff* tsd)
{
	tsd->stream.Exit(tsd->priv);
	GxCore_ThreadJoin(tsd->pthread_r);
	GxCore_ThreadJoin(tsd->pthread_w);
}

static int tsbuff_run(void* handle)
{
	TSBuff* tsd = handle;

	if(tsd) {
		if(tsd->running++ > 0) {
			return 0;
		}
		if (tsd->kernel_mode == 0) {
			tsb_run_thread(tsd);
			tsb_run_slot(tsd);
		}
	}

	return 0;
}

static int tsbuff_stop(void* handle)
{
	TSBuff* tsd = handle;

	if(tsd) {
		if(--tsd->running == 0) {
			if (tsd->kernel_mode == 0) {
				tsb_stop_slot(tsd);
				tsb_stop_thread(tsd);
			}
		}

		if(--tsd->opencnt == 0) {
			tsb_cleanup_slot(tsd);
			tsb_cleanup_fifo(tsd);
			tsb_cleanup_dmx(tsd);
		}

		gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	}

	return 0;
}

static int tsbuff_close(void* handle)
{
	return 0;
}

static int tsbuff_disconnect(void* handle, const char* url)
{
	TSBuff* tsd = handle;

	if(tsd) {
		dvbsource_disconnect_l(tsd->dev, tsd->dmx_r, url);
	}

	return 0;
}

static void* tsbuff_open(const char* url)
{
	TSBuff* tsd = &_tsbuff;

	if(_tsbuff_inited == 0) {
		int i;
		memset(tsd, 0, sizeof(TSBuff));
		for(i=0; i<TSBUFF_SLOT_MAX; i++)
			tsd->slot[i].pid = -1;
		_tsbuff_inited = 1;
	}

	return tsd;
}

static int tsbuff_config(void *handle, const char* url, GxOptions *op)
{
	int ret;
	int delayms, delaydmx, delayhole;
	int vpid, apid, pcrpid, tsid, dmxid;
	TSBuff* tsd = handle;

	if(tsd->opencnt++ > 0) {
		return 0;
	}

	GxUrl_GetItem(url, GX_URL_KEY_TSBUFF_MS, &delayms);
	//delayms = 3000;
	if(delayms <= 0)
		return -1;

	GxUrl_GetItem(url,GX_URL_KEY_VPID, &vpid);
	GxUrl_GetItem(url,GX_URL_KEY_APID, &apid);
	GxUrl_GetItem(url,GX_URL_KEY_PCRPID, &pcrpid);
	GxUrl_GetItem(url,GX_URL_KEY_TSID, &tsid);
	GxUrl_GetItem(url,GX_URL_KEY_DMXID, &dmxid);
	GxUrl_GetItem(url,GX_URL_KEY_TSBUFF_DMXID, &delaydmx);
	GxUrl_GetItem(url,GX_URL_KEY_TSBUFF_MEMHOLE, &delayhole);

	if((PID_INVAILD(apid) && PID_INVAILD(vpid)) || (apid == vpid))
		return -1;

	if(delaydmx < 0) {
		gxloge("[error]: delay dmxid not defined!\n");
		return -1;
	}

	delayhole = delayhole<0 ? 0 : delayhole;
	GxPlayer_SystemSet(PSYS_DELAY_CACHE_HOLE_SIZE, &delayhole);

	if(PID_VAILD(vpid))
		tsd->vpid = vpid;
	if(PID_VAILD(apid))
		tsd->apid = apid;
	if(PID_VAILD(pcrpid) && pcrpid != vpid && pcrpid != apid)
		tsd->pcrpid = pcrpid;

	tsd->tsid = tsid<0 ? 0 : tsid;
	tsd->dmxid = dmxid<0 ? 0 : dmxid;
	tsd->delayms = delayms;
	tsd->delaydmx = delaydmx;

	// Todo:
	tsd->kernel_mode = 0; //?

	ret = tsb_init_dmx(tsd, url, op);
	TSD_CHECK_RET(ret);

	ret = tsb_init_fifo(tsd);
	TSD_CHECK_RET(ret);

	ret = tsb_init_slot(tsd);
	TSD_CHECK_RET(ret);

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return 0;

ERR:
	tsbuff_close((void*)tsd);
	return -1;
}

static int tsbuff_track_add(void* handle, GxStreamTrackAdd* ptrack)
{
	int i, j, ret, num;
	GxDemuxProperty_Slot slot;
	TSBuff* tsd = handle;

	slot.flags = (DMX_PTS_TO_SDRAM | DMX_TSOUT_EN | DMX_CRC_DISABLE);
	slot.ts_out_pin = tsd->ts_out_pin;
	slot.pid = 0x1fff;

	num = TSBUFF_SLOT_MAX - tsd->slot_nb;
	num = GX_MIN(ptrack->num, num);

	for(i=0; i<num; i++) {
		slot.type = DEMUX_SLOT_PSI;
		slot.pid = ptrack->pid[i];
		if(slot.pid < 0x1fff) {
			j = tsbuff_slot_find(handle, slot.pid);
			if(j >= 0) {
				tsd->slotusecnt[j]++;
				continue;
			}

			ret = GxAVGetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotAlloc, &slot, sizeof(slot));
			TSD_CHECK_RET(ret);

			tsbuff_slot_add(handle, slot);

			ret = GxAVSetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotConfig, &slot, sizeof(slot));
			TSD_CHECK_RET(ret);

			GxAVSetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotEnable, &slot, sizeof(slot));
		}
	}

ERR:
	gxlogf("[Player]: %s[%d]:%s [%d:%d]\n", __FILE__, __LINE__, __FUNCTION__, ptrack->num, i);
	return 0;
}

static int tsbuff_track_del(void* handle, GxStreamTrackDel* ptrack)
{
	int i;

	for(i=0; i<ptrack->num; i++) {
		tsbuff_slot_del(handle, ptrack->pid[i]);
	}

	gxlogf("[Player]: %s[%d]:%s [%d:%d]\n", __FILE__, __LINE__, __FUNCTION__, ptrack->num, i);
	return 0;
}

static int tsbuff_audio_switch(void* handle, unsigned short pid)
{
	int ret;
	TSBuff* tsd = handle;
	GxDemuxProperty_Slot slot;

	if(PID_VAILD(tsd->apid)) {
		tsbuff_slot_del(handle, tsd->apid);
	}

	tsd->apid = pid;
	if (tsd->slot_nb >= TSBUFF_SLOT_MAX || !PID_VAILD(pid))
		return -1;

	slot.pid = pid;
	slot.type = DEMUX_SLOT_AUDIO;
	slot.ts_out_pin = tsd->ts_out_pin;
	slot.flags = (DMX_TSOUT_EN | DMX_CRC_DISABLE | DMX_REPEAT_MODE | DMX_ERR_DISCARD_EN);

	ret = GxAVGetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotAlloc, &slot, sizeof(slot));
	TSD_CHECK_RET(ret);

	tsbuff_slot_add(handle, slot);

	ret = GxAVSetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotConfig, &slot, sizeof(slot));
	TSD_CHECK_RET(ret);

	GxAVSetProperty(tsd->dev, tsd->dmx_r, GxDemuxPropertyID_SlotEnable, &slot, sizeof(slot));

ERR:
	return 0;
}

static int tsbuff_control(void* ds, int cmd, void* args)
{

	return 0;
}

DVBSourceClass dvbsource_tsbuff = {
	.type  = DVBSOURCE_TSBUFF,
	.open = tsbuff_open,
	.config = tsbuff_config,
	.pause = NULL,
	.resume = NULL,
	.run = tsbuff_run,
	.stop = tsbuff_stop,
	.close = tsbuff_close,
	.disconnect = tsbuff_disconnect,
	.track_add = tsbuff_track_add,
	.track_del = tsbuff_track_del,
	.audio_switch = tsbuff_audio_switch,
	.control = tsbuff_control
};

static handle_t tsb_stream_init(size_t blksize, unsigned int delayms)
{
	int ret, holesize;
	TSBuffStreamPort* priv = av_mallocz(sizeof(TSBuffStreamPort));
	if (priv == NULL)
		return 0;

	priv->blksize = blksize;
	priv->delayms = delayms;
	GxPlayer_SystemGet(PSYS_DELAY_CACHE_DIR,    &priv->cachedir);
	GxPlayer_SystemGet(PSYS_DELAY_CACHE_SIZE,   &priv->cachesize);
	GxPlayer_SystemGet(PSYS_DELAY_CACHE_TO_MEM, &priv->cache2mem);
	GxPlayer_SystemGet(PSYS_DELAY_CACHE_HOLE_SIZE, &holesize);

	//priv->cache2mem = 0;
	//priv->cachedir = "/mnt/usb01/12345/";
	//priv->cachesize = 0x80000;

	gxlogf("[Player]: %s[%d]:%s, %d, %d, %s\n", __FILE__, __LINE__, __FUNCTION__,
			priv->cachesize, priv->cache2mem, priv->cachedir);

	if (priv->cache2mem) {
		ret = page_cache_init(&priv->cache, priv->blksize,
				priv->cachesize/priv->blksize, priv->cachesize*3/priv->blksize/4, holesize);
		TSD_CHECK_RET(ret);
	}
	else {
		ret = file_cache_init(&priv->fcache, priv->cachedir, priv->cachesize);
		TSD_CHECK_RET(ret);

		priv->fcache_r = av_malloc(priv->blksize);
		priv->fcache_w = av_malloc(priv->blksize);
		if (priv->fcache_r == NULL || priv->fcache_w == NULL) {
			av_free(priv->fcache_r);
			av_free(priv->fcache_w);
			file_cache_uninit(&priv->fcache);
			goto ERR;
		}
		GxCore_SemCreate(&priv->sem, 0);
	}

	priv->running = 1;
	return (handle_t)priv;
ERR:
	av_free(priv);
	return 0;
}

static void tsb_stream_uninit(handle_t handle)
{
	TSBuffStreamPort* priv = (TSBuffStreamPort*)handle;

	if (priv) {
		if (priv->cache2mem) {
			page_cache_uninit(&priv->cache);
		}
		else {
			av_free(priv->fcache_r);
			av_free(priv->fcache_w);
			file_cache_uninit(&priv->fcache);
			GxCore_SemDelete(priv->sem);
		}

		av_free(priv);
	}
}

static void tsb_stream_exit(handle_t handle)
{
	TSBuffStreamPort* priv = (TSBuffStreamPort*)handle;

	priv->running = 0;

	if (priv->cache2mem)
		page_cache_exit(&priv->cache);
	else
		GxCore_SemPost(priv->sem);
}

static int tsb_stream_new_packet(handle_t handle, GxPlayerDataPacket* pkt, ssize_t size)
{
	TSBuffStreamPort* priv = (TSBuffStreamPort*)handle;

	if (priv->cache2mem) {
		struct cache_page* page = page_cache_alloc_page(&priv->cache);
		if(page == NULL)
			return -1;
		priv->cur_page_r = page;
		pkt->data = page->buffer;
		pkt->size = page->size;
	}
	else {
		pkt->data = priv->fcache_r;
		pkt->size = TSBUFF_BLOCK_SIZE;
	}

	return 0;
}

static void tsb_stream_del_packet(handle_t handle, GxPlayerDataPacket* pkt)
{
	TSBuffStreamPort* priv = (TSBuffStreamPort*)handle;

	if (pkt && pkt->data) {
		if (priv->cache2mem) {
			page_cache_free_page(&priv->cache, priv->cur_page_w);
			priv->cur_page_w = NULL;
		}
	}

	memset(pkt, 0, sizeof(GxPlayerDataPacket));
}


static int tsb_stream_get_packet(handle_t handle, GxPlayerDataPacket* pkt)
{
	uint32_t pts;
	int32_t delayms;
	TSBuffStreamPort* priv = (TSBuffStreamPort*)handle;

	if (priv->cache2mem) {
		struct cache_page* page = page_cache_get_page(&priv->cache);
		if(page == NULL)
			return -1;
		priv->cur_page_w = page;
		pkt->data = page->buffer;
		pkt->size = page->size;
		pts = page->pts;
	}
	else {
		ssize_t size;
		GxCore_SemWait(priv->sem);
		size = file_cache_read(&priv->fcache, priv->fcache_w, TSBUFF_BLOCK_SIZE, &pts);
		if(size <= 0)
			return -1;
		pkt->data = priv->fcache_w;
		pkt->size = size;
	}

	while (priv->running) {
		delayms = tsb_current_timems() - pts - priv->delayms;
		if(delayms >= -10)
			break;
		GxCore_ThreadDelay(10);
	}

	return 0;
}

static int tsb_stream_put_packet(handle_t handle, GxPlayerDataPacket* pkt)
{
	TSBuffStreamPort* priv = (TSBuffStreamPort*)handle;

	if (priv->cache2mem) {
		priv->cur_page_r->size = pkt->size;
		priv->cur_page_r->pts = tsb_current_timems();
		page_cache_put_page(&priv->cache, priv->cur_page_r);
		priv->cur_page_r = NULL;
	}
	else {
		uint32_t pts = tsb_current_timems();
		file_cache_write(&priv->fcache, priv->fcache_r, pkt->size, pts);
		GxCore_SemPost(priv->sem);
	}

	return 0;
}

static PlayerStreamPortCallback tsbuff_stream_inside = {
	.Init = tsb_stream_init,
	.Uninit = tsb_stream_uninit,
	.Exit = tsb_stream_exit,
	.Reset = NULL,
	.NewPacket = tsb_stream_new_packet,
	.DelPacket = tsb_stream_del_packet,
	.GetPacket = tsb_stream_get_packet,
	.PutPacket = tsb_stream_put_packet,
};

