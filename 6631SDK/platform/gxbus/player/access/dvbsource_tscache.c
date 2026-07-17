#include "dvbsource.h"
#include "gx_mediafilter.h"
#include "gx_system.h"
#include "gx_common.h"
#include "gx_mediafilter.h"
#include "pack_cache.h"
#include "file_cache.h"
#include "gx_avtick.h"

enum {
	MODE_USER          ,
	MODE_KERNEL_NORMAL ,
	MODE_KERNEL_SHARED ,
};

#define TSCACHE_DEFAULT_MS      (600)
#define TSCACHE_DEFAULT_MODE    (MODE_KERNEL_SHARED)
#define TSCACHE_DEFAULT_DMXID   (1)
#define TSCACHE_DEFAULT_BUFSIZE (188*1024*50)

#define PID_VAILD(pid)          (pid >  0 && pid <  0x1FFF)
#define PID_INVAILD(pid)        (pid <= 0 || pid >= 0x1FFF)

#define TSCACHE_BLOCK_SIZE      (188*300)
#define TSCACHE_SLOT_MAX        (64)

#define TSCACHE_PTHREAD_PRIORITY (GXOS_DEFAULT_PRIORITY-1)

static int tscache_track_cache(void* handle, GxStreamTrackCache* ptrack);
static int tscache_close(void* handle);

typedef struct {
	int running;
	int cachesize, cache2mem;
	char* cachedir;
	unsigned char* fcache_r, *fcache_w;

	int blksize;
	unsigned int cache_ms;

	handle_t sem;
	struct pack_cache cache;
	struct file_cache fcache;
} TSCacheStreamPort;

typedef struct {
	int opencnt;
	int runcnt;

	int cache_ms;
	int cache_mode;
	int cache_dmxid;
	int cache_bufsize;

	handle_t dev;
	handle_t dmx_r;
	handle_t dmx_w;

	int vpid;
	int apid;
	int pcrpid;
	int tsid;
	int dmxid;

	int mutex;
	int running;
	int paused;
	int pthread_r;
	int pthread_w;

	GxFifo* fifo_r;
	GxFifo* fifo_w;
	int pin_r;
	int pin_w;

	int slot_nb;
	GxDemuxProperty_Slot slot[TSCACHE_SLOT_MAX];
	int slotusecnt[TSCACHE_SLOT_MAX];

	handle_t priv;
	PlayerStreamPortCallback stream;
} TSCache;


static TSCache _tscache = { 0 };
static PlayerStreamPortCallback tscache_stream_inside;
static GxStreamTrackCache _track_cache = { 0 };

static uint32_t tsc_current_timems(void)
{
	GxTime Time;

	GxCore_GetTickTime(&Time);

	return Time.seconds*1000 + Time.microsecs/1000;
}

static int tscache_slot_find(void* handle, int pid)
{
	int i;
	TSCache* tsc = handle;

	for(i=0; i<TSCACHE_SLOT_MAX; i++) {
		if (tsc->slot[i].pid == pid)
			return i;
	}

	return -1;
}

static int tscache_slot_add(void* handle, GxDemuxProperty_Slot slot)
{
	int i;
	TSCache* tsc = handle;

	i = tscache_slot_find(handle, slot.pid);
	if (i >= 0) {
		tsc->slotusecnt[i]++;
		return 0;
	}

	for(i=0; i<TSCACHE_SLOT_MAX; i++) {
		if (tsc->slotusecnt[i] == 0) {
			tsc->slot[i] = slot;
			tsc->slotusecnt[i]++;
			tsc->slot_nb++;
			return 0;
		}
	}

	return -1;
}

static int tscache_slot_del(void* handle, unsigned short pid)
{
	int i;
	TSCache* tsc = handle;
	GxDemuxProperty_Slot slot;

	i = tscache_slot_find(handle, pid);
	if (i >= 0) {
		if (--tsc->slotusecnt[i] == 0) {
			slot = tsc->slot[i];
			GxAVSetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotDisable, &slot, sizeof(slot));
			GxAVSetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotFree, &slot, sizeof(slot));
			memset(&tsc->slot[i], 0, sizeof(slot));
			tsc->slot[i].pid = -1;
			tsc->slot_nb--;
		}
		return 0;
	}

	return -1;
}

static void tsc_cleanup_dmx(TSCache* tsc)
{
	GxDemuxProperty_ConfigDemux cfg_dmx = {
		.stream_mode = 0,
		.ts_select   = 0,
		.source      = 0,
		.time_gate        = 0xf,
		.sync_lock_gate   = 0x3,
		.sync_loss_gate   = 0x3,
		.byt_cnt_err_gate = 0x3
	};

	if (tsc->dmx_r > 0)
		GxAvdev_CloseModule(tsc->dev, tsc->dmx_r);

	if (tsc->dmx_w > 0) {
		cfg_dmx.source = tsc->tsid;
		GxAVSetProperty(tsc->dev, tsc->dmx_w, GxDemuxPropertyID_Config, &cfg_dmx, sizeof(cfg_dmx));
		GxAvdev_CloseModule(tsc->dev, tsc->dmx_w);
	}

	if (tsc->dev > 0)
		GxAvdev_DestroyDevice(tsc->dev);
}

static void tsc_cleanup_fifo(TSCache* tsc)
{
	GxFifoLink FifoLink;

	if (tsc->fifo_r) {
		GxFifo_Destroy(tsc->fifo_r);
		tsc->fifo_r = NULL;
	}

	if (tsc->fifo_w) {
		FifoLink.module = tsc->dmx_w;
		GxFifo_Unlink(tsc->fifo_w, &FifoLink);
		GxFifo_Destroy(tsc->fifo_w);
		tsc->fifo_w = NULL;
	}

	if (tsc->cache_mode == MODE_USER) {
		tsc->stream.Uninit(tsc->priv);
		tsc->priv = 0;
	}
	else {
		struct gxav_dvr2dvr_protocol protocol;
		protocol.dvrid = tsc->cache_dmxid;
		protocol.cmd = GXAV_DVR2DVR_PROTOCOL_CLOSE;
		GxAVDVR2DVRProtocolControl(tsc->dev, &protocol);
	}
}

static void tsc_cleanup_slot(TSCache* tsc)
{
	int i;
	GxDemuxProperty_Slot slot;

	for (i=0; i<TSCACHE_SLOT_MAX; i++) {
		if (tsc->slotusecnt[i] > 0) {
			slot = tsc->slot[i];
			GxAVSetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotDisable, &slot, sizeof(slot));
			GxAVSetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotFree, &slot, sizeof(slot));
			tsc->slotusecnt[i] = 0;
			memset(&tsc->slot[i], 0, sizeof(slot));
			tsc->slot[i].pid = -1;
		}
	}

	tsc->slot_nb = 0;
}

#define TSC_CHECK_RET(ret)\
	do {\
		if (ret < 0) \
		{\
			gxloge("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);\
			goto ERR;\
		}\
	}while(0);

#define TSC_CHECK_NULL(p)\
	do {\
		if (p == NULL) \
		{\
			gxloge("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);\
			goto ERR;\
		}\
	}while(0);


static int tsc_init_slot(TSCache* tsc)
{
	int ret;
	GxFifoConfig FifoConfig;
	GxDemuxProperty_Slot slot;

	//TSR
	memset(&FifoConfig, 0, sizeof(FifoConfig));
	FifoConfig.dev = tsc->dev;
	FifoConfig.dvrid = tsc->dmxid;
	FifoConfig.dest = DVR_OUTPUT_DMX;
	if (tsc->cache_mode == MODE_USER) {
		FifoConfig.source = DVR_INPUT_MEM;
		FifoConfig.hw_buffer_size = tsc->fifo_w->size;
	}
	else {
		switch(tsc->cache_dmxid) {
		case 0:
		case 8:
			FifoConfig.source = DVR_INPUT_DVR0;
			break;
		case 1:
			FifoConfig.source = DVR_INPUT_DVR1;
			break;
		case 2:
		case 4:
			FifoConfig.source = DVR_INPUT_DVR2;
			break;
		case 3:
		case 5:
			FifoConfig.source = DVR_INPUT_DVR3;
			break;
		default:
			return -1;
		}
		if (tsc->cache_mode == MODE_KERNEL_SHARED)
			FifoConfig.hw_buffer_size = 0;
		else
			FifoConfig.hw_buffer_size = tsc->fifo_w->size;
	}
	ret = GxFifo_Config(tsc->fifo_w, &FifoConfig);
	TSC_CHECK_RET(ret);
	tsc->pin_r = FifoConfig.dvrhandle;

	//TSW
	memset(&FifoConfig, 0, sizeof(FifoConfig));
	FifoConfig.dev = tsc->dev;
	FifoConfig.dvrid = tsc->cache_dmxid;
	FifoConfig.sw_buffer_size = tsc->fifo_r->size;
	FifoConfig.hw_buffer_size = tsc->fifo_r->size;
	if (_track_cache.num == 0)
		FifoConfig.source = DVR_INPUT_TSPORT;
	else
		FifoConfig.source = DVR_INPUT_DMX;
	if (tsc->cache_mode == MODE_USER)
		FifoConfig.dest = DVR_OUTPUT_MEM;
	else {
		switch(tsc->dmxid) {
		case 0:
			FifoConfig.dest = DVR_OUTPUT_DVR0;
			break;
		case 1:
			FifoConfig.dest = DVR_OUTPUT_DVR1;
			break;
		default:
			return -1;
		}
	}
	ret = GxFifo_Config(tsc->fifo_r, &FifoConfig);
	TSC_CHECK_RET(ret);
	tsc->pin_w = FifoConfig.dvrhandle;
	if (1) {
		GxDvrProperty_TSRFlowControl ctrl = {0};
		ctrl.flags = DVR_FLOW_CONTROL_ES;
		GxAVSetProperty(FifoConfig.dev, FifoConfig.dvrhandle,
				GxDvrPropertyID_TSRFlowControl, &ctrl, sizeof(GxDvrProperty_TSRFlowControl));
	}

	if (tsc->cache_mode != MODE_USER) {
		struct gxav_dvr2dvr_protocol protocol;
		protocol.dvrid = tsc->cache_dmxid;
		protocol.cmd = GXAV_DVR2DVR_PROTOCOL_CONFIG;
		protocol.params.config.handle = FifoConfig.dvrhandle;
		GxAVDVR2DVRProtocolControl(tsc->dev, &protocol);
	}

	if (_track_cache.num == 0)
		return 0;

	if (PID_VAILD(tsc->vpid)) {
		slot.pid = tsc->vpid;
		slot.type = DEMUX_SLOT_VIDEO;
		slot.flags |= DMX_ERR_DISCARD_EN;
		ret = GxAVGetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotAlloc, &slot, sizeof(slot));
		TSC_CHECK_RET(ret);

		ret = GxAVSetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotConfig, &slot, sizeof(slot));
		TSC_CHECK_RET(ret);

		tscache_slot_add(tsc, slot);
	}

	if (PID_VAILD(tsc->apid)) {
		slot.pid = tsc->apid;
		slot.type = DEMUX_SLOT_AUDIO;
		slot.flags |= DMX_ERR_DISCARD_EN;
		ret = GxAVGetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotAlloc, &slot, sizeof(slot));
		TSC_CHECK_RET(ret);

		ret = GxAVSetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotConfig, &slot, sizeof(slot));
		TSC_CHECK_RET(ret);

		tscache_slot_add(tsc, slot);
	}

	if (PID_VAILD(tsc->pcrpid)) {
		slot.pid = tsc->pcrpid;
		ret = GxAVGetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotAlloc, &slot, sizeof(slot));
		TSC_CHECK_RET(ret);

		ret = GxAVSetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotConfig, &slot, sizeof(slot));
		TSC_CHECK_RET(ret);

		tscache_slot_add(tsc, slot);
	}

	return 0;
ERR:
	tsc_cleanup_slot(tsc);
	return -1;
}

static int tsc_init_fifo(TSCache* tsc)
{
	int rcachesize;

	memcpy(&tsc->stream, &tscache_stream_inside, sizeof(PlayerStreamPortCallback));
	GxPlayer_SystemGet(PSYS_CBK_STREAM_PORT, &tsc->stream);

	tsc->fifo_r = GxFifo_Create(tsc->cache_bufsize, GX_PINFLAG_MUXTS);
	TSC_CHECK_NULL(tsc->fifo_r);

	tsc->fifo_w = GxFifo_Create(tsc->cache_bufsize, GX_PINFLAG_MUXTS);
	TSC_CHECK_NULL(tsc->fifo_w);

	if (tsc->cache_mode == MODE_USER) {
		tsc->priv = tsc->stream.Init(TSCACHE_BLOCK_SIZE, tsc->cache_ms);
		if (tsc->priv != 0)
			return 0;
	}
	else {
		struct gxav_dvr2dvr_protocol protocol;
		protocol.dvrid = tsc->cache_dmxid;
		protocol.cmd = GXAV_DVR2DVR_PROTOCOL_OPEN;
		protocol.params.open.cachems = tsc->cache_ms;
		protocol.params.open.shared = (tsc->cache_mode == MODE_KERNEL_SHARED) ? 1 : 0;
		GxAVDVR2DVRProtocolControl(tsc->dev, &protocol);
		return 0;
	}
ERR:
	tsc_cleanup_fifo(tsc);
	return -1;
}

static int tsc_init_dmx(TSCache* tsc, const char* url, GxOptions *op)
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

	tsc->dev = GxAvdev_CreateDevice(0);

	tsc->dmx_r = GxAvdev_OpenModule(tsc->dev, GXAV_MOD_DEMUX, tsc->cache_dmxid);
	TSC_CHECK_RET(tsc->dmx_r);
	tsc->dmx_w = GxAvdev_OpenModule(tsc->dev, GXAV_MOD_DEMUX, tsc->dmxid);
	TSC_CHECK_RET(tsc->dmx_w);

	cfg_dmx.source = tsc->tsid;
	ret = GxAVSetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_Config, &cfg_dmx, sizeof(cfg_dmx));
	TSC_CHECK_RET(ret);

	cfg_dmx.source = DEMUX_SDRAM;
	ret = GxAVSetProperty(tsc->dev, tsc->dmx_w, GxDemuxPropertyID_Config, &cfg_dmx, sizeof(cfg_dmx));
	TSC_CHECK_RET(ret);

	ret = dvbsource_set_tp(tsc->dev, tsc->dmx_r, url, op);
	TSC_CHECK_RET(ret);

	return 0;

ERR:
	tsc_cleanup_dmx(tsc);
	return -1;
}

static void tsc_run_slot(TSCache *tsc)
{
	int i;
	GxDemuxProperty_Slot slot;

	tscache_track_cache(tsc, &_track_cache);

	for (i=0; i< tsc->slot_nb; i++) {
		slot = tsc->slot[i];
		GxAVSetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotEnable, &slot, sizeof(slot));
	}
}

static void tsc_stop_slot(TSCache* tsc)
{
	int i;
	GxDemuxProperty_Slot slot;

	for (i=0; i< tsc->slot_nb; i++) {
		slot = tsc->slot[i];
		GxAVSetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotDisable, &slot, sizeof(slot));
	}
}

static void tsc_wthread(void* data)
{
	TSCache *tsc = data;

	while(tsc->running) {
		int ret;
		GxPlayerDataPacket pkt;
		if (tsc->running == 0x99) {
			tsc->paused = 1;
			GxCore_ThreadDelay(10);
			continue;
		}

		tsc->paused = 0;
		ret = tsc->stream.GetPacket(tsc->priv, &pkt);
		if (ret == 0) {
			ssize_t wsize, size = pkt.size;
			wsize = GxFifo_Write(tsc->fifo_w, pkt.data, size, 100000);
			while(wsize < size && tsc->running == 1) {
				wsize += GxFifo_Write(tsc->fifo_w, pkt.data + wsize,  size - wsize, 100000);
				GxCore_ThreadDelay(10);
			}
			tsc->stream.DelPacket(tsc->priv, &pkt);
		}
		else {
			GxCore_ThreadDelay(10);
		}
	}
}

static void tsc_rthread(void* data)
{
	TSCache *tsc = data;
	GxPlayerDataPacket pkt;

	memset(&pkt, 0, sizeof(pkt));

	while(tsc->running) {
		size_t size, rsize;
		if (pkt.data == NULL) {
			int ret = tsc->stream.NewPacket(tsc->priv, &pkt, TSCACHE_BLOCK_SIZE);
			if (ret != 0) {
				gxlogi("[tscache] OOM  #########\n");
				GxCore_ThreadDelay(10);
				continue;
			}
		}

		size = pkt.size;
		rsize = GxFifo_Read(tsc->fifo_r, pkt.data, size, 10000);
		while(rsize < size && tsc->running) {
			rsize += GxFifo_Read(tsc->fifo_r, pkt.data + rsize, size - rsize, 10000);
			GxCore_ThreadDelay(10);
		}
		if (rsize > 0) {
			pkt.size = rsize;
			tsc->stream.PutPacket(tsc->priv, &pkt);
			pkt.data = NULL;
		}
		else {
			GxCore_ThreadDelay(10);
		}
	}

	if (pkt.data != NULL)
		tsc->stream.DelPacket(tsc->priv, &pkt);
}

static void tsc_run_thread(TSCache *tsc)
{
	if (tsc->cache_mode != MODE_USER)
		return;

	GxCore_ThreadCreate("tscache read thread", &tsc->pthread_r, tsc_rthread, tsc, 1024*16, TSCACHE_PTHREAD_PRIORITY);
	GxCore_ThreadCreate("tscache write thread", &tsc->pthread_w, tsc_wthread, tsc, 1024*16, TSCACHE_PTHREAD_PRIORITY);
}

static void tsc_stop_thread(TSCache* tsc)
{
	if (tsc->cache_mode != MODE_USER)
		return;

	tsc->stream.Exit(tsc->priv);
	GxCore_ThreadJoin(tsc->pthread_r);
	GxCore_ThreadJoin(tsc->pthread_w);
}

static int tscache_run(void* handle)
{
	TSCache* tsc = handle;

	if (tsc && tsc->runcnt++ == 0) {
		if (tsc->cache_mode == MODE_USER) {
			if (tsc->running == 0x99) {
				if (tsc->stream.Reset)
					tsc->stream.Reset(tsc->priv);
				tsc->running = 1;
			}
			else if (tsc->running == 0) {
				tsc->running = 1;
				tsc_run_thread(tsc);
				tsc_run_slot(tsc);
			}
		}
		else {
			struct gxav_dvr2dvr_protocol protocol;
			protocol.dvrid = tsc->cache_dmxid;
			protocol.cmd = GXAV_DVR2DVR_PROTOCOL_START;
			GxAVDVR2DVRProtocolControl(tsc->dev, &protocol);
			if (tsc->running == 0)
				tsc_run_slot(tsc);
			tsc->running = 1;
		}
	}

	return 0;
}

static int tscache_stop(void* handle)
{
	TSCache* tsc = handle;

	if (tsc && tsc->runcnt-- == 1) {
		tsc->running = 0x99;
		if (tsc->cache_mode == MODE_USER) {
			while (tsc->paused != 1)
				GxCore_ThreadDelay(10);
			GxFifo_Reset(tsc->fifo_w);
		}
		else {
			struct gxav_dvr2dvr_protocol protocol;
			protocol.dvrid = tsc->cache_dmxid;
			protocol.cmd = GXAV_DVR2DVR_PROTOCOL_STOP;
			GxAVDVR2DVRProtocolControl(tsc->dev, &protocol);
			if (tsc->cache_mode == MODE_KERNEL_NORMAL)
				GxFifo_Reset(tsc->fifo_w);
		}
	}

	gxlogi("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return 0;
}

static int tscache_pause(void* handle)
{
	TSCache* tsc = handle;
	GxDemuxProperty_ConfigDemux cfg_dmx = {
		.stream_mode = 0,
		.ts_select   = 0,
		.source      = 0,
		.time_gate        = 0xf,
		.sync_lock_gate   = 0x3,
		.sync_loss_gate   = 0x3,
		.byt_cnt_err_gate = 0x3
	};

	tsc_cleanup_fifo(tsc);

	if (tsc->dmx_w > 0) {
		cfg_dmx.source = tsc->tsid;
		GxAVSetProperty(tsc->dev, tsc->dmx_w, GxDemuxPropertyID_Config, &cfg_dmx, sizeof(cfg_dmx));
	}

	gxlogi("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return 0;
}

static int tscache_resume(void* handle)
{
	TSCache* tsc = handle;
	GxDemuxProperty_ConfigDemux cfg_dmx = {
		.stream_mode = 0,
		.ts_select   = 0,
		.source      = 0,
		.time_gate        = 0xf,
		.sync_lock_gate   = 0x3,
		.sync_loss_gate   = 0x3,
		.byt_cnt_err_gate = 0x3
	};

	tsc_init_fifo(tsc);

	if (tsc->dmx_w > 0) {
		cfg_dmx.source = DEMUX_SDRAM;
		GxAVSetProperty(tsc->dev, tsc->dmx_w, GxDemuxPropertyID_Config, &cfg_dmx, sizeof(cfg_dmx));
	}

	gxlogi("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return 0;
}

static int tscache_close(void* handle)
{
	TSCache* tsc = handle;

	if (tsc && --tsc->opencnt == 0) {
		GxCore_MutexLock(tsc->mutex);
		tsc->running = 0;
		GxCore_MutexUnlock(tsc->mutex);
		tsc_stop_slot(tsc);
		tsc_stop_thread(tsc);
		tsc_cleanup_slot(tsc);
		tsc_cleanup_fifo(tsc);
		tsc_cleanup_dmx(tsc);
		if (tsc->mutex) {
			GxCore_MutexDelete(tsc->mutex);
			tsc->mutex = 0;
		}
		gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	}

	return 0;
}

static int tscache_disconnect(void* handle, const char* url)
{
	TSCache* tsc = handle;

	if (tsc) {
		dvbsource_disconnect_l(tsc->dev, tsc->dmx_r, url);
	}

	return 0;
}

static int tscache_sametp(void* handle, const char* url)
{
#ifdef CONFIG_DVB_FE
	extern int32_t GxFrontend_SameTp(GxFrontend *para);
	TSCache* tsc = handle;
	int tuner,type;
	GxFrontend frontend;
	int fre,symb,qam,polar,bandwidth,sat22k,workmode;
	int data_plp_id, common_plp_exist,common_plp_id;
	int if_channel_index =0;
	int lnb_index =0x20;
	int centre_fre =0;
	int if_fre =0;

	if (strncasecmp(url,"dvbc",4) == 0)
		type = FRONTEND_DVB_C;
	else if (strncasecmp(url,"dvbs",4) == 0)
		type = FRONTEND_DVB_S;
	else if (strncasecmp(url,"dvbt2",5) == 0)
		type = FRONTEND_DVB_T2;
	else if (strncasecmp(url,"dvbt",4) == 0)
		type = FRONTEND_DVB_T;
	else if (strncasecmp(url,"dtmb",4) == 0)
		type = FRONTEND_DTMB;
	else
		return 0;

	GxUrl_GetItem(url,GX_URL_KEY_TUNER,&tuner);
	GxUrl_GetItem(url,GX_URL_KEY_FRE,&fre);
	GxUrl_GetItem(url,GX_URL_KEY_SYM,&symb);
	GxUrl_GetItem(url,GX_URL_KEY_QAM,&qam);
	GxUrl_GetItem(url,GX_URL_KEY_POLAR,&polar);
	GxUrl_GetItem(url,GX_URL_KEY_BANDW,&bandwidth);
	GxUrl_GetItem(url,GX_URL_KEY_22K,&sat22k);
	GxUrl_GetItem(url,GX_URL_KEY_CWMODE,&workmode);
	GxUrl_GetItem(url,GX_URL_KEY_DATA_PLPID,&data_plp_id);
	GxUrl_GetItem(url,GX_URL_KEY_COMM_PLPID_EXIST,&common_plp_exist);
	GxUrl_GetItem(url,GX_URL_KEY_COMM_PLPID,&common_plp_id);
	GxUrl_GetItem(url,GX_URL_KEY_UNICABLE_LNB_INDEX,&lnb_index);
	if (lnb_index == -1)
	{
		if_channel_index =0;
		lnb_index =0x20;
		centre_fre =0;
		if_fre =0;
	}
	else
	{
		GxUrl_GetItem(url,GX_URL_KEY_UNICABLE_IF_INDEX,&if_channel_index);
		GxUrl_GetItem(url,GX_URL_KEY_UNICABLE_CENTRE_FRE,&centre_fre);
		GxUrl_GetItem(url,GX_URL_KEY_UNICABLE_IF_FRE,&if_fre);
	}

	if (fre > 0) {
		frontend.fre        = fre;
		frontend.symb       = symb;
		frontend.qam        = qam;
		frontend.sat22k     = sat22k;
		frontend.polar      = polar;
		frontend.bandwidth  = bandwidth;
		frontend.type_1501  = workmode;
		frontend.data_plp_id = data_plp_id;
		frontend.common_plp_exist = common_plp_exist;
		frontend.common_plp_id = common_plp_id;

		frontend.dev        = tsc->dev;
		frontend.demux      = tsc->dmx_r;
		frontend.tuner      = (tuner < 0) ? 0 : tuner;
		frontend.type       = type;

		frontend.if_channel_index= if_channel_index;
		frontend.lnb_index= lnb_index;

		frontend.centre_fre= centre_fre;
		frontend.if_fre= if_fre;
		if (frontend.lnb_index < 0x10&&frontend.centre_fre!=0 &&frontend.if_fre!=0)
		{
			frontend.fre = frontend.if_fre;

		}
		return GxFrontend_SameTp(&frontend);
	}
#endif
	return 0;
}

static void* tscache_open(const char* url)
{
	int i = 0;
	int tsid, dmxid;
	int cachems, cachemode, cachebufsize, cachedmxid;
	TSCache* tsc = &_tscache;

	if (tsc->opencnt++ > 0) {
		return tsc;
	}

	memset(tsc, 0, sizeof(TSCache));
	for(i=0; i<TSCACHE_SLOT_MAX; i++)
		tsc->slot[i].pid = -1;
	tsc->opencnt = 1;

	GxUrl_GetItem(url, GX_URL_KEY_TSCACHE_MS, &cachems);
	GxUrl_GetItem(url, GX_URL_KEY_TSCACHE_MODE, &cachemode);
	GxUrl_GetItem(url, GX_URL_KEY_TSCACHE_DMXID, &cachedmxid);
	GxUrl_GetItem(url, GX_URL_KEY_TSCACHE_BUFSIZE, &cachebufsize);

	GxUrl_GetItem(url,GX_URL_KEY_TSID, &tsid);
	GxUrl_GetItem(url,GX_URL_KEY_DMXID, &dmxid);

	tsc->tsid = tsid<0 ? 0 : tsid;
	tsc->dmxid = dmxid<0 ? 0 : dmxid;
	tsc->cache_ms = cachems<0 ? TSCACHE_DEFAULT_MS: cachems;
	tsc->cache_mode = cachemode<0 ? TSCACHE_DEFAULT_MODE: cachemode;
	tsc->cache_dmxid = cachedmxid<0 ? TSCACHE_DEFAULT_DMXID: cachedmxid;
	tsc->cache_bufsize = cachebufsize<0 ? TSCACHE_DEFAULT_BUFSIZE: cachebufsize;
	tsc->cache_bufsize = tsc->cache_bufsize/(188*1024)*(188*1024);

	gxlogi("==========================================\n");
	gxlogi("%s. CacheConfig:\n", __func__);
	gxlogi("	tsc->tsid = %d\n", tsc->tsid);
	gxlogi("	tsc->dmxid = %d\n", tsc->dmxid);
	gxlogi("	tsc->cache_ms = %d\n", tsc->cache_ms);
	gxlogi("	tsc->cache_mode = %d\n", tsc->cache_mode);
	gxlogi("	tsc->cache_dmxid = %d\n", tsc->cache_dmxid);
	gxlogi("	tsc->cache_bufsize = %d\n", tsc->cache_bufsize);
	gxlogi("==========================================\n");

	GxCore_MutexCreate(&tsc->mutex);
	return tsc;
}

static int tscache_config(void *handle, const char* url, GxOptions *op)
{
	int ret;
	int cachems, cachemode, cachebufsize, cachedmxid;
	int vpid, apid, pcrpid, tsid, dmxid;
	TSCache* tsc = handle;

	GxUrl_GetItem(url,GX_URL_KEY_VPID, &vpid);
	GxUrl_GetItem(url,GX_URL_KEY_APID, &apid);
	GxUrl_GetItem(url,GX_URL_KEY_PCRPID, &pcrpid);

	if (PID_VAILD(vpid)) tsc->vpid = vpid;
	if (PID_VAILD(apid)) tsc->apid = apid;
	if (PID_VAILD(pcrpid) && pcrpid != vpid && pcrpid != apid) tsc->pcrpid = pcrpid;

	if (tscache_sametp(tsc, url)) {
		return 0;
	}

	gxlogi("[Player] TP Change !\n");

	tsc->running = 0;

	tsc_stop_slot(tsc);
	tsc_stop_thread(tsc);
	tsc_cleanup_slot(tsc);
	tsc_cleanup_fifo(tsc);
	tsc_cleanup_dmx(tsc);

	ret = tsc_init_dmx(tsc, url, op);
	TSC_CHECK_RET(ret);

	ret = tsc_init_fifo(tsc);
	TSC_CHECK_RET(ret);

	ret = tsc_init_slot(tsc);
	TSC_CHECK_RET(ret);

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return 0;

ERR:
	tscache_close((void*)tsc);
	return -1;
}

static int tscache_track_cache(void* handle, GxStreamTrackCache* ptrack)
{
	int i, j, ret, num;
	GxDemuxProperty_Slot slot;
	TSCache* tsc = handle;

	slot.flags = (DMX_PTS_TO_SDRAM | DMX_TSOUT_EN | DMX_CRC_DISABLE);
	slot.ts_out_pin = tsc->pin_r;
	slot.pid = 0x1fff;

	num = TSCACHE_SLOT_MAX - tsc->slot_nb;
	num = GX_MIN(ptrack->num, num);

	for(i=0; i<num; i++) {
		slot.type = DEMUX_SLOT_PSI;
		slot.pid = ptrack->pid[i];
		if (slot.pid < 0x1fff) {
			j = tscache_slot_find(handle, slot.pid);
			if (j >= 0) {
				tsc->slotusecnt[j]++;
				continue;
			}

			ret = GxAVGetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotAlloc, &slot, sizeof(slot));
			TSC_CHECK_RET(ret);

			tscache_slot_add(handle, slot);

			ret = GxAVSetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotConfig, &slot, sizeof(slot));
			TSC_CHECK_RET(ret);
		}
	}

ERR:
	gxlogf("[Player]: %s[%d]:%s [%d:%d]\n", __FILE__, __LINE__, __FUNCTION__, ptrack->num, i);
	return 0;
}

static int tscache_track_add(void* handle, GxStreamTrackAdd* ptrack)
{
	int i, j, ret, num;
	GxDemuxProperty_Slot slot;
	TSCache* tsc = handle;

	if (_track_cache.num == 0)
		return 0;

	slot.flags = (DMX_PTS_TO_SDRAM | DMX_TSOUT_EN | DMX_CRC_DISABLE);
	slot.ts_out_pin = tsc->pin_r;
	slot.pid = 0x1fff;

	num = TSCACHE_SLOT_MAX - tsc->slot_nb;
	num = GX_MIN(ptrack->num, num);

	for(i=0; i<num; i++) {
		slot.type = DEMUX_SLOT_PSI;
		slot.pid = ptrack->pid[i];
		if (slot.pid < 0x1fff) {
			j = tscache_slot_find(handle, slot.pid);
			if (j >= 0) {
				tsc->slotusecnt[j]++;
				continue;
			}

			ret = GxAVGetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotAlloc, &slot, sizeof(slot));
			TSC_CHECK_RET(ret);

			tscache_slot_add(handle, slot);

			ret = GxAVSetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotConfig, &slot, sizeof(slot));
			TSC_CHECK_RET(ret);

			GxAVSetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotEnable, &slot, sizeof(slot));
		}
	}

ERR:
	gxlogf("[Player]: %s[%d]:%s [%d:%d]\n", __FILE__, __LINE__, __FUNCTION__, ptrack->num, i);
	return 0;
}

static int tscache_track_del(void* handle, GxStreamTrackDel* ptrack)
{
	int i;

	if (_track_cache.num == 0)
		return 0;

	for(i=0; i<ptrack->num; i++)
		tscache_slot_del(handle, ptrack->pid[i]);

	gxlogf("[Player]: %s[%d]:%s [%d:%d]\n", __FILE__, __LINE__, __FUNCTION__, ptrack->num, i);
	return 0;
}

static int tscache_audio_switch(void* handle, unsigned short pid)
{
	int ret;
	TSCache* tsc = handle;
	GxDemuxProperty_Slot slot;

	if (PID_VAILD(tsc->apid)) {
		tscache_slot_del(handle, tsc->apid);
	}

	tsc->apid = pid;
	if (tsc->slot_nb >= TSCACHE_SLOT_MAX || !PID_VAILD(pid))
		return -1;

	if (_track_cache.num == 0)
		return 0;

	slot.pid = pid;
	slot.type = DEMUX_SLOT_AUDIO;
	slot.ts_out_pin = tsc->pin_r;
	slot.flags = (DMX_TSOUT_EN | DMX_CRC_DISABLE | DMX_REPEAT_MODE | DMX_ERR_DISCARD_EN);

	ret = GxAVGetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotAlloc, &slot, sizeof(slot));
	TSC_CHECK_RET(ret);

	tscache_slot_add(handle, slot);

	ret = GxAVSetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotConfig, &slot, sizeof(slot));
	TSC_CHECK_RET(ret);

	GxAVSetProperty(tsc->dev, tsc->dmx_r, GxDemuxPropertyID_SlotEnable, &slot, sizeof(slot));

ERR:
	return 0;
}

static int tscache_control(void* ds, int cmd, void* args)
{
	TSCache* tsc = ds;

	switch (cmd) {
	case GX_STREAM_CTRL_TS_CACHE_CONFIG:
		if (args) {
			memcpy(&_track_cache, args, sizeof(_track_cache));
		}
		break;
	case GX_STREAM_CTRL_VIDEO_READY:
		if (tsc->cache_mode == MODE_KERNEL_SHARED) {
			struct gxav_dvr2dvr_protocol protocol;
			protocol.dvrid = tsc->cache_dmxid;
			protocol.cmd = GXAV_DVR2DVR_PROTOCOL_START_CACHE;
			GxAVDVR2DVRProtocolControl(tsc->dev, &protocol);
		}
		break;
	default:
		break;
	}

	return 0;
}


DVBSourceClass dvbsource_tscache = {
	.type  = DVBSOURCE_TSCACHE,
	.open = tscache_open,
	.config = tscache_config,
	.pause = tscache_pause,
	.resume = tscache_resume,
	.run = tscache_run,
	.stop = tscache_stop,
	.close = tscache_close,
	.disconnect = tscache_disconnect,
	.track_add = tscache_track_add,
	.track_del = tscache_track_del,
	.audio_switch = tscache_audio_switch,
	.control = tscache_control
};

static handle_t tsc_stream_init(size_t blksize, unsigned int cache_ms)
{
	int ret;
	TSCacheStreamPort* priv = av_mallocz(sizeof(TSCacheStreamPort));
	if (priv == NULL)
		return 0;

	priv->blksize = blksize;
	priv->cache_ms = cache_ms;
	GxPlayer_SystemGet(PSYS_DELAY_CACHE_DIR,    &priv->cachedir);
	GxPlayer_SystemGet(PSYS_DELAY_CACHE_SIZE,   &priv->cachesize);
	GxPlayer_SystemGet(PSYS_DELAY_CACHE_TO_MEM, &priv->cache2mem);

	//priv->cache2mem = 0;
	//priv->cachedir = "/mnt/usb01/12345/";

	gxlogf("[Player]: %s[%d]:%s, %d, %d, %s\n", __FILE__, __LINE__, __FUNCTION__,
			priv->cachesize, priv->cache2mem, priv->cachedir);

	if (priv->cache2mem) {
		ret = pack_cache_init(&priv->cache, priv->blksize, priv->cachesize/priv->blksize, priv->cache_ms/10);
		TSC_CHECK_RET(ret);
	}
	else {
		ret = file_cache_init(&priv->fcache, priv->cachedir, priv->cachesize);
		TSC_CHECK_RET(ret);

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

static void tsc_stream_uninit(handle_t handle)
{
	TSCacheStreamPort* priv = (TSCacheStreamPort*)handle;

	if (priv) {
		if (priv->cache2mem) {
			pack_cache_uninit(&priv->cache);
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

static void tsc_stream_reset(handle_t handle)
{
	TSCacheStreamPort* priv = (TSCacheStreamPort*)handle;

	if (priv && priv->cache2mem)
		pack_cache_reset(&priv->cache);
}

static void tsc_stream_exit(handle_t handle)
{
	TSCacheStreamPort* priv = (TSCacheStreamPort*)handle;

	if (priv) {
		priv->running = 0;

		if (priv->cache2mem)
			pack_cache_exit(&priv->cache);
		else
			GxCore_SemPost(priv->sem);
	}
}

static int tsc_stream_new_packet(handle_t handle, GxPlayerDataPacket* pkt, ssize_t size)
{
	TSCacheStreamPort* priv = (TSCacheStreamPort*)handle;

	if (priv->cache2mem) {
		struct cache_pack* pack = pack_cache_alloc_pack(&priv->cache);
		if (pack == NULL)
			return -1;
		pkt->data = pack->buffer;
		pkt->size = pack->size;
		pkt->priv = pack;
	}
	else {
		pkt->data = priv->fcache_r;
		pkt->size = TSCACHE_BLOCK_SIZE;
	}

	return 0;
}

static void tsc_stream_del_packet(handle_t handle, GxPlayerDataPacket* pkt)
{
	TSCacheStreamPort* priv = (TSCacheStreamPort*)handle;

	if (pkt && pkt->data) {
		if (priv->cache2mem) {
			if (pkt->priv)
				pack_cache_free_pack(&priv->cache, pkt->priv);
		}
	}
}


static int tsc_stream_get_packet(handle_t handle, GxPlayerDataPacket* pkt)
{
	uint32_t pts;
	int32_t cache_ms;
	TSCacheStreamPort* priv = (TSCacheStreamPort*)handle;

	if (priv->cache2mem) {
		struct cache_pack* pack = pack_cache_get_pack(&priv->cache);
		if (pack == NULL) {
			GxCore_ThreadDelay(10);
			return -1;
		}
		pkt->data = pack->buffer;
		pkt->size = pack->size;
		pkt->pts  = pack->pts;
		pkt->priv = pack;

		return 0;
	}
	else {
		uint32_t size;
		GxCore_SemWait(priv->sem);
		size = file_cache_read(&priv->fcache, priv->fcache_w, TSCACHE_BLOCK_SIZE, &pts);
		if (size <= 0)
			return -1;
		pkt->data = priv->fcache_w;
		pkt->size = size;
		pkt->pts  = pts ;
	}

	while (priv->running) {
		cache_ms = tsc_current_timems() - pts - priv->cache_ms;
		if (cache_ms >= -10)
			break;
		GxCore_ThreadDelay(10);
	}

	return 0;
}

static int tsc_stream_put_packet(handle_t handle, GxPlayerDataPacket* pkt)
{
	TSCacheStreamPort* priv = (TSCacheStreamPort*)handle;

	if (priv->cache2mem) {
		struct cache_pack *pack = pkt->priv;
		pack->size = pkt->size;
		pack->pts = tsc_current_timems() / 10;
		pack_cache_put_pack(&priv->cache, pack);
	}
	else {
		uint32_t pts = tsc_current_timems();
		file_cache_write(&priv->fcache, priv->fcache_r, pkt->size, pts);
		GxCore_SemPost(priv->sem);
	}

	return 0;
}

static PlayerStreamPortCallback tscache_stream_inside = {
	.Init = tsc_stream_init,
	.Uninit = tsc_stream_uninit,
	.Exit = tsc_stream_exit,
	.Reset = tsc_stream_reset,
	.NewPacket = tsc_stream_new_packet,
	.DelPacket = tsc_stream_del_packet,
	.GetPacket = tsc_stream_get_packet,
	.PutPacket = tsc_stream_put_packet,
};

