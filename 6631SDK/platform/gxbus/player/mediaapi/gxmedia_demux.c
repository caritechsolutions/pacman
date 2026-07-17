#include "gxmedia_module.h"
#include "gxmedia_common.h"

static int _alloc_esfifo_solt(GxMediaDemux *mdemux, GxDemuxProperty_Slot* slot, int fifo)
{
	int ret = 0;

	slot->slot_id = -1;
	ret = GxAVGetProperty(mdemux->mdmx_dev, mdemux->mdmx_mod,
			GxDemuxPropertyID_SlotAlloc, slot, sizeof(GxDemuxProperty_Slot));
	if (ret < 0) {
		GxMediaAPI_debug("alloc slot[%d, %d, %d] get config error\n", fifo, slot->type, slot->pid);
		return -1;
	}

	GxAVFifoReset(mdemux->mdmx_dev, fifo);

	if (mdemux->astream_type == GXMEDIA_AVSTREAM_TS)
		GxAVLinkFifo(mdemux->mdmx_dev, mdemux->odvr_mod, fifo, slot->slot_id, GXAV_PIN_OUTPUT);
	else
		GxAVLinkFifo(mdemux->mdmx_dev, mdemux->mdmx_mod, fifo, slot->slot_id, GXAV_PIN_OUTPUT);

	ret = GxAVSetProperty(mdemux->mdmx_dev, mdemux->mdmx_mod,
			GxDemuxPropertyID_SlotConfig, slot, sizeof(GxDemuxProperty_Slot));
	if (ret < 0) {
		GxMediaAPI_debug("alloc slot[%d, %d, %d] set config error\n", fifo, slot->type, slot->pid);
		goto alloc_slot_err;
	}

	return 0;

alloc_slot_err:
	if (mdemux->astream_type == GXMEDIA_AVSTREAM_TS)
		GxAVUnLinkFifo(mdemux->mdmx_dev, mdemux->odvr_mod, fifo);
	else
		GxAVUnLinkFifo(mdemux->mdmx_dev, mdemux->mdmx_mod, fifo);

	GxAVSetProperty(mdemux->mdmx_dev, mdemux->mdmx_mod,
			GxDemuxPropertyID_SlotFree, slot, sizeof(GxDemuxProperty_Slot));

	return -1;
}

static int _free_esfifo_slot(GxMediaDemux *mdemux, GxDemuxProperty_Slot* slot, int fifo)
{
	int ret = 0;

	GxAVUnLinkFifo(mdemux->mdmx_dev, mdemux->mdmx_mod, fifo);
	ret = GxAVSetProperty(mdemux->mdmx_dev, mdemux->mdmx_mod,
				GxDemuxPropertyID_SlotFree, slot,sizeof(GxDemuxProperty_Slot));
	if (ret < 0) {
		GxMediaAPI_debug("free slot[%d, %d, %d] set config error\n", fifo, slot->type, slot->pid);
		return -1;
	}

	return 0;
}

static int _alloc_fifo_buffer(GxMediaDemux *mdemux, unsigned char** buffer, int* fifo, int size, int flags)
{
	uint8_t* memhole;
	int memfifo;
	int subflags;

	memhole = GxMediaAPI_MemAlloc(size, flags);
	if (memhole == NULL) {
		GxMediaAPI_debug("[%s %d] demux create memhole failed\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (flags == GX_PINFLAG_TSA)
		subflags = GX_PINFLAG_NO_PTS_FIFO;
	else
		subflags = flags&(~GX_PINFLAG_MASK);
	memfifo = GxAVFifoCreate(mdemux->mdmx_dev, memhole, size, subflags);
	if (memfifo == -1) {
		GxMediaAPI_debug("[%s %d] demux create fifo failed\n", __FUNCTION__, __LINE__);
		GxMediaAPI_MemFree(memhole, flags);
		return -1;
	}

	*buffer = memhole;
	*fifo = memfifo;

	return 0;
}

static void _free_fifo_buffer(GxMediaDemux *mdemux, unsigned char** buffer, int* fifo, int flags)
{
	if(*fifo != -1)
		GxAVFifoDestroy(mdemux->mdmx_dev, *fifo);

	if(*buffer)
		GxMediaAPI_MemFree(*buffer, flags);

	*buffer = NULL;
	*fifo = -1;

	return;
}

GxMediaDemux* GxMedia_DemuxOpen(GxMediaDemuxMod mod, GxMediaModuleModPara mpara)
{
	int ret = 0;
	GxDvrProperty_Config dvrconf;
	GxMediaDemux* mdemux = NULL;
	int esvsize, esasize;

	mdemux = av_mallocz(sizeof(GxMediaDemux));
	if (mdemux == NULL) return NULL;

	mdemux->mdmx_dev = -1;
	mdemux->mdmx_mod = -1;
	mdemux->idvr_mod = -1;
	mdemux->odvr_mod = -1;
	mdemux->esa_fifo = -1;
	mdemux->esv_fifo = -1;
	mdemux->valid = 1;
	mdemux->encrpt_mode  = mpara.protect_mode;
	mdemux->astream_type = mpara.a_stream_type;

	GxPlayer_SystemGet(PSYS_BufSizeESV, &esvsize);
	GxPlayer_SystemGet(PSYS_BufSizeESA, &esasize);

	mdemux->mdmx_dev = GxAvdev_CreateDevice(0);
	if (mdemux->mdmx_dev < 0) {
		GxMediaAPI_debug("[%s %d] demux create device failed\n", __FUNCTION__, __LINE__);
		goto open_error;
	}

	mdemux->mdmx_mod = GxAvdev_OpenModule(mdemux->mdmx_dev, GXAV_MOD_DEMUX, mod.demux_modid);
	if (mdemux->mdmx_mod < 0) {
		GxMediaAPI_debug("[%s %d] demux create module failed\n", __FUNCTION__, __LINE__);
		goto open_error;
	}

	if (mpara.source_type == GXMEDIA_SOURCE_TS) {
		mdemux->idvr_mod = GxAvdev_OpenModule(mdemux->mdmx_dev, GXAV_MOD_DVR, 0);
		memset(&dvrconf, 0, sizeof(dvrconf));
		dvrconf.src = DVR_INPUT_MEM;
		dvrconf.dst = DVR_OUTPUT_DMX;
		dvrconf.src_buf.hw_buffer_size = DEMUX_TSBUF_SIZE;
		dvrconf.flags |= ((mpara.protect_mode == GXMEDIA_OUTPUT_PROTECT) ? 0 : DVR_FLAG_MEM_NOT_PROTECTED);
		dvrconf.flags |= DVR_FLAG_PTR_MODE_EN;

		GxAVSetProperty(mdemux->mdmx_dev, mdemux->idvr_mod, GxDvrPropertyID_Config, &dvrconf, sizeof(dvrconf));
	}

	if (mpara.a_stream_type == GXMEDIA_AVSTREAM_TS) {
		mdemux->odvr_mod = GxAvdev_OpenModule(mdemux->mdmx_dev, GXAV_MOD_DVR, 0);
		memset(&dvrconf, 0, sizeof(dvrconf));
		dvrconf.src = DVR_INPUT_DMX;
		dvrconf.dst = DVR_OUTPUT_DSP;
		dvrconf.dst_buf.hw_buffer_size   = esasize;
		dvrconf.dst_buf.sw_buffer_size   = esasize;
		dvrconf.dst_buf.almost_full_gate = esasize/10;

		GxAVSetProperty(mdemux->mdmx_dev, mdemux->odvr_mod, GxDvrPropertyID_Config, &dvrconf, sizeof(dvrconf));
	}

	ret = _alloc_fifo_buffer(mdemux, &mdemux->esv_buffer, &mdemux->esv_fifo, esvsize, GX_PINFLAG_ESV);
	if (ret < 0) {
		GxMediaAPI_debug("[%s %d] demux alloc esa fifo failed\n", __FUNCTION__, __LINE__);
		goto open_error;
	}

	if (mdemux->astream_type == GXMEDIA_AVSTREAM_TS)
		ret = _alloc_fifo_buffer(mdemux, &mdemux->esa_buffer, &mdemux->esa_fifo, esasize, GX_PINFLAG_TSA);
	else
		ret = _alloc_fifo_buffer(mdemux, &mdemux->esa_buffer, &mdemux->esa_fifo, esasize, GX_PINFLAG_ESA);
	if (ret < 0) {
		GxMediaAPI_debug("[%s %d] demux alloc esv fifo failed\n", __FUNCTION__, __LINE__);
		goto open_error;
	}

	return mdemux;

open_error:
	GxMedia_DemuxClose(mdemux);
	return NULL;
}

int GxMedia_DemuxClose(GxMediaDemux* mdemux)
{
	CHECK_MEDIA_MOLUDE_VALID(mdemux);

	GxMedia_DemuxStop(mdemux);
	if (mdemux->astream_type == GXMEDIA_AVSTREAM_TS)
		_free_fifo_buffer(mdemux, &mdemux->esa_buffer, &mdemux->esa_fifo, GX_PINFLAG_TSA);
	else
		_free_fifo_buffer(mdemux, &mdemux->esa_buffer, &mdemux->esa_fifo, GX_PINFLAG_ESA);
	_free_fifo_buffer(mdemux, &mdemux->esv_buffer, &mdemux->esv_fifo, GX_PINFLAG_ESV);

	if (mdemux->odvr_mod != -1)
		GxAvdev_CloseModule(mdemux->mdmx_dev, mdemux->odvr_mod);

	if (mdemux->idvr_mod != -1)
		GxAvdev_CloseModule(mdemux->mdmx_dev, mdemux->idvr_mod);

	if (mdemux->mdmx_mod != -1)
		GxAvdev_CloseModule(mdemux->mdmx_dev, mdemux->mdmx_mod);

	if (mdemux->mdmx_dev != -1)
		GxAvdev_DestroyDevice(mdemux->mdmx_dev);

	mdemux->mdmx_mod = -1;
	mdemux->odvr_mod = -1;
	mdemux->idvr_mod = -1;
	mdemux->mdmx_dev = -1;
	mdemux->valid = 0;
	av_free(mdemux);

	return GX_MEDIAAPI_SUCCESS;
}


int GxMedia_DemuxConfig(GxMediaDemux* mdemux, int Tsid, int AudPid, int VidPid, int PcrPid)
{
	int ret = 0;
	GxDemuxProperty_Pcr  pcr;
	GxDemuxProperty_ConfigDemux dmx_config;

	CHECK_MEDIA_MOLUDE_VALID(mdemux);

	dmx_config.ts_select = 0;
	dmx_config.time_gate = 0xf;
	dmx_config.sync_lock_gate = 0x3;
	dmx_config.sync_loss_gate = 0x3;
	dmx_config.byt_cnt_err_gate = 0x3;
	dmx_config.stream_mode = 0;
	dmx_config.source = (Tsid == -1)?DEMUX_SDRAM:Tsid;
	ret = GxAVSetProperty(mdemux->mdmx_dev, mdemux->mdmx_mod,
			GxDemuxPropertyID_Config, &dmx_config, sizeof(GxDemuxProperty_ConfigDemux));
	if (ret < 0) {
		GxMediaAPI_debug("[%s %d] demux config failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	if (PcrPid > 0) {
		pcr.pcr_pid = PcrPid;
		ret = GxAVSetProperty(mdemux->mdmx_dev, mdemux->mdmx_mod,
				GxDemuxPropertyID_Pcr, &pcr, sizeof(GxDemuxProperty_Pcr));
		if (ret < 0) {
			GxMediaAPI_debug("[%s %d] demux config pcr failed\n", __FUNCTION__, __LINE__);
			return GX_MEDIAAPI_ERROR;
		}
	}

	if (VidPid > 0) {
		mdemux->esv_slot.pid   = VidPid;
		mdemux->esv_slot.flags = (DMX_REPEAT_MODE|DMX_PTS_TO_SDRAM|DMX_AVOUT_EN);;
		mdemux->esv_slot.type  = DEMUX_SLOT_VIDEO;
		ret = _alloc_esfifo_solt(mdemux, &mdemux->esv_slot, mdemux->esv_fifo);
		if (ret < 0) {
			GxMediaAPI_debug("[%s %d] demux config esv failed\n", __FUNCTION__, __LINE__);
			return GX_MEDIAAPI_ERROR;
		}
	}

	if (AudPid > 0) {
		mdemux->esa_slot.pid   = AudPid;
		if (mdemux->astream_type == GXMEDIA_AVSTREAM_TS)
			mdemux->esa_slot.flags = (DMX_REPEAT_MODE|DMX_TSOUT_EN);
		else
			mdemux->esa_slot.flags = (DMX_REPEAT_MODE|DMX_PTS_TO_SDRAM|DMX_AVOUT_EN);
		mdemux->esa_slot.type  = DEMUX_SLOT_AUDIO;
		ret = _alloc_esfifo_solt(mdemux, &mdemux->esa_slot, mdemux->esa_fifo);
		if (ret < 0) {
			GxMediaAPI_debug("[%s %d] demux config esa failed\n", __FUNCTION__, __LINE__);
			return GX_MEDIAAPI_ERROR;
		}
	}

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_DemuxStart(GxMediaDemux* mdemux)
{
	int ret = 0;

	CHECK_MEDIA_MOLUDE_VALID(mdemux);

	if (mdemux->running == 1)
		return GX_MEDIAAPI_SUCCESS;

	if (mdemux->esv_slot.pid > 0) {
		GxAVSetProperty(mdemux->mdmx_dev, mdemux->mdmx_mod,
				GxDemuxPropertyID_SlotEnable, &mdemux->esv_slot, sizeof(GxDemuxProperty_Slot));
		if (ret < 0)
			GxMediaAPI_debug("[%s %d] demux enable esv slot failed\n", __FUNCTION__, __LINE__);
	}

	if (mdemux->esa_slot.pid > 0) {
		ret = GxAVSetProperty(mdemux->mdmx_dev, mdemux->mdmx_mod,
				GxDemuxPropertyID_SlotEnable, &mdemux->esa_slot, sizeof(GxDemuxProperty_Slot));
		if (ret < 0)
			GxMediaAPI_debug("[%s %d] demux enable esa slot failed\n", __FUNCTION__, __LINE__);
	}

	if (mdemux->idvr_mod != -1)
		GxAVSetProperty(mdemux->mdmx_dev, mdemux->idvr_mod, GxDvrPropertyID_Run, NULL, 0);

	if (mdemux->odvr_mod != -1)
		GxAVSetProperty(mdemux->mdmx_dev, mdemux->odvr_mod, GxDvrPropertyID_Run, NULL, 0);

	mdemux->running = 1;
	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_DemuxStop(GxMediaDemux* mdemux)
{
	int ret = 0;

	CHECK_MEDIA_MOLUDE_VALID(mdemux);

	if (mdemux->running == 1) {
		if (mdemux->esa_slot.pid > 0) {
			ret = GxAVSetProperty(mdemux->mdmx_dev, mdemux->mdmx_mod, \
					GxDemuxPropertyID_SlotDisable, &mdemux->esa_slot, sizeof(GxDemuxProperty_Slot));
			if (ret < 0)
				GxMediaAPI_debug("[%s %d] demux disable esa slot failed\n", __FUNCTION__, __LINE__);
		}

		if (mdemux->esv_slot.pid > 0) {
			ret = GxAVSetProperty(mdemux->mdmx_dev, mdemux->mdmx_mod, \
					GxDemuxPropertyID_SlotDisable, &mdemux->esv_slot, sizeof(GxDemuxProperty_Slot));
			if (ret < 0)
				GxMediaAPI_debug("[%s %d] demux disable esv slot failed\n", __FUNCTION__, __LINE__);
		}

		if (mdemux->idvr_mod != -1)
			GxAVSetProperty(mdemux->mdmx_dev, mdemux->idvr_mod, GxDvrPropertyID_Stop, NULL, 0);

		if (mdemux->odvr_mod != -1)
			GxAVSetProperty(mdemux->mdmx_dev, mdemux->odvr_mod, GxDvrPropertyID_Stop, NULL, 0);

		mdemux->running = 0;
	}

	if (mdemux->esa_slot.pid > 0) {
		_free_esfifo_slot(mdemux, &mdemux->esa_slot, mdemux->esa_fifo);
		mdemux->esa_slot.pid = 0;
	}

	if (mdemux->esv_slot.pid > 0) {
		_free_esfifo_slot(mdemux, &mdemux->esv_slot, mdemux->esv_fifo);
		mdemux->esv_slot.pid = 0;
	}

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_DemuxGetAfifo(GxMediaDemux* mdemux)
{
	CHECK_MEDIA_MOLUDE_VALID(mdemux);

	return mdemux->esa_fifo;
}

int GxMedia_DemuxGetVfifo(GxMediaDemux* mdemux)
{
	CHECK_MEDIA_MOLUDE_VALID(mdemux);

	return mdemux->esv_fifo;
}

int GxMedia_DemuxInjectData(GxMediaDemux* mdemux, GxMediaDemuxInject *inject)
{
	int write_size = 0;
	int esa_used_size = 0;
	int esa_fifo_size = 0;
	int esv_used_size = 0;
	int esv_fifo_size = 0;

	CHECK_MEDIA_MOLUDE_VALID(mdemux);

	if (mdemux->esa_fifo != -1) {
		esa_fifo_size = GxAVFifoGetCap(mdemux->mdmx_dev,    mdemux->esa_fifo);
		esa_used_size = GxAVFifoGetLength(mdemux->mdmx_dev, mdemux->esa_fifo);
	}

	if (mdemux->esv_fifo != -1) {
		esv_fifo_size = GxAVFifoGetCap(mdemux->mdmx_dev,    mdemux->esv_fifo);
		esv_used_size = GxAVFifoGetLength(mdemux->mdmx_dev, mdemux->esv_fifo);
	}

	if ((esa_used_size > esa_fifo_size*7/8) || (esv_used_size > esv_fifo_size*7/8))
		return 0;

	if (mdemux->idvr_mod != -1) {
		GxDvrProperty_Config dvrconf = {0};
		int ret = 0, dvr_len = 0;

		ret = GxAVGetProperty(mdemux->mdmx_dev, mdemux->idvr_mod,
				GxDvrPropertyID_Config, &dvrconf, sizeof(GxDvrProperty_Config));
		if (ret < 0)
			return -1;

		dvr_len =  dvrconf.src_buf.buffer_len;
		if (esa_fifo_size > 0) {
			if (dvr_len + inject->len > (esa_fifo_size - esa_used_size))
				return 0;
		}
		if (esv_fifo_size > 0) {
			if (dvr_len + inject->len > (esv_fifo_size - esv_used_size))
				return 0;
		}
	}

	if (inject->decrypt_cb) {
		GxDvrProperty_PtrInfo info = {0};
		unsigned int buffer = 0;
		int ret = 0;

		info.blocklen = inject->len;
		ret = GxAVGetProperty(mdemux->mdmx_dev,
				mdemux->idvr_mod,
				GxDvrPropertyID_PtrInfo, &info, sizeof(info));
		if (ret < 0)
			return -1;

		buffer = info.paddr;
		if ((mdemux->ts_fill_size + inject->len) <= DEMUX_TSBUF_SIZE) {
			inject->decrypt_cb(inject->priv, inject->buf, (unsigned char*)buffer, inject->len);
		} else {
			unsigned int copy_size = (DEMUX_TSBUF_SIZE - mdemux->ts_fill_size);

			inject->decrypt_cb(inject->priv, inject->buf, (unsigned char*)buffer, copy_size);
			buffer    = buffer - mdemux->ts_fill_size;
			copy_size = inject->len - copy_size;
			inject->decrypt_cb(inject->priv, inject->buf, (unsigned char*)buffer, copy_size);
		}

		info.paddr    = (unsigned int)buffer;
		info.size     = inject->len;
		info.blocklen = inject->len;
		ret = GxAVSetProperty(mdemux->mdmx_dev,
				mdemux->idvr_mod,
				GxDvrPropertyID_PtrInfo, &info, sizeof(info));
		if (ret < 0)
			return -1;

		mdemux->ts_fill_size += inject->len;
		mdemux->ts_fill_size %= DEMUX_TSBUF_SIZE;
		return inject->len;
	} else {
		if (mdemux->encrpt_mode == GXMEDIA_OUTPUT_PROTECT) {
			GxMediaAPI_debug("Not Support protect buffer\n");
			return -1;
		}
		write_size = GxAVModuleWrite(mdemux->mdmx_dev, mdemux->idvr_mod, inject->buf, inject->len, -1);
		if (write_size != inject->len) {
			GxMediaAPI_debug("Ts fifo write_size %d, len %d\n", write_size, inject->len);
			return -1;
		}
	}

	return write_size;
}

int GxMedia_DemuxGetFifoInfo(GxMediaDemux* mdemux, GxMediaModuleDemuxFifo *info)
{
	int ret = 0;
	GxDvrProperty_Config dvrconf = {0};

	CHECK_MEDIA_MOLUDE_VALID(mdemux);

	ret = GxAVGetProperty(mdemux->mdmx_dev, mdemux->idvr_mod,
			GxDvrPropertyID_Config, &dvrconf, sizeof(GxDvrProperty_Config));
	if (ret < 0)
		return GX_MEDIAAPI_ERROR;

	if (info) {
		info->ts_used_size = dvrconf.src_buf.buffer_len;
		info->ts_free_size = dvrconf.src_buf.buffer_freelen;
	}

	return GX_MEDIAAPI_SUCCESS;
}
