#include "hw_demux.h"
#include "stheader.h"
#include "gx_config.h"

static HwDemuxDev hw_demux = {0};

#define MUX_SLOT_NUM (2)
#define DMX_SLOT_NUM (2)
#define DMX_PESFILTER_NUM (2)

static handle_t hwdemux_mutex = -1;

#define GxHwDemux_Lock()   GxCore_MutexLock(hwdemux_mutex)
#define GxHwDemux_Unlock() GxCore_MutexUnlock(hwdemux_mutex)

int GxHwDemux_Init(void)
{
	if(hwdemux_mutex == -1)
		GxCore_MutexCreate(&hwdemux_mutex);

	return 0;
}

static int find_slot_idx(int index, int progid, int type)
{
	int slotidx = -1;

	switch(type) {
	case HW_DEMUX_VIDEO:
		slotidx = hw_demux.module[index]->prog[progid].vslotidx;
		break;
	case HW_DEMUX_AUDIO:
		slotidx = hw_demux.module[index]->prog[progid].aslotidx;
		break;
	case HW_DEMUX_PES_AUDIO:
		slotidx   = hw_demux.module[index]->prog[progid].audio.slotidx;
		break;
	case HW_DEMUX_PES_SUBT:
	case HW_DEMUX_PSI_SUBT:
		slotidx   = hw_demux.module[index]->prog[progid].subt.slotidx;
		break;
	default:
		break;
	}

	return slotidx;
}

static int find_filter_idx(int index, int progid, int type)
{
	int filteridx = -1;

	switch(type) {
	case HW_DEMUX_VIDEO:
	case HW_DEMUX_AUDIO:
		break;
	case HW_DEMUX_PES_AUDIO:
		filteridx = hw_demux.module[index]->prog[progid].audio.filteridx;
		break;
	case HW_DEMUX_PES_SUBT:
	case HW_DEMUX_PSI_SUBT:
		filteridx = hw_demux.module[index]->prog[progid].subt.filteridx;
		break;
	default:
		break;
	}

	return filteridx;
}

static GxFifo *find_slot_fifo(int index, int progid, int type)
{
	GxFifo *fifo = NULL;

	switch(type) {
	case HW_DEMUX_VIDEO:
		fifo = hw_demux.module[index]->prog[progid].vfifo;
		break;
	case HW_DEMUX_AUDIO:
		fifo = hw_demux.module[index]->prog[progid].afifo;
		break;
	case HW_DEMUX_PES_AUDIO:
	case HW_DEMUX_PES_SUBT:
	case HW_DEMUX_PSI_SUBT:
		break;
	default:
		break;
	}

	return fifo;
}

static int hw_slot_alloc(int index, GxDemuxProperty_Slot *slot)
{
	return GxAVGetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
			GxDemuxPropertyID_SlotAlloc, slot, sizeof(GxDemuxProperty_Slot));
}

static int hw_slot_free(int index, GxDemuxProperty_Slot *slot)
{
	GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
			GxDemuxPropertyID_SlotDisable, slot, sizeof(GxDemuxProperty_Slot));

	return GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
			GxDemuxPropertyID_SlotFree, slot, sizeof(GxDemuxProperty_Slot));
}

static int hw_slot_config(int index, GxDemuxProperty_Slot *slot)
{
	gxlogd(">> pid: %4d, type %9s, TS_EN %d, AV_EN %d, DES_EN %d, AUTO_BIND %d\n",
			slot->pid,
			((slot->type == DEMUX_SLOT_VIDEO) ? "video":
			 ((slot->type == DEMUX_SLOT_AUDIO) ? "audio" :
			  ((slot->type == DEMUX_SLOT_PSI) ? "psi" :
			   ((slot->type == DEMUX_SLOT_PES_AUDIO) ? "pes_audio" :
				((slot->type == DEMUX_SLOT_PES)? "pes" : "unkown"))))),
			(slot->flags & DMX_TSOUT_EN) ? 1 : 0,
			(slot->flags & DMX_AVOUT_EN) ? 1 : 0,
			(slot->flags & DMX_DES_EN) ? 1 : 0,
			(slot->flags & DMX_DES_AUTO_BIND) ? 1 : 0);
	return GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
			GxDemuxPropertyID_SlotConfig, slot, sizeof(GxDemuxProperty_Slot));
}

static int hw_query_flags_by_pid(int index, int pid)
{
	int ret = -1;
	GxDemuxProperty_Slot slot;
	GxDemuxProperty_SlotQueryByPid query_slot;

	query_slot.pid = (unsigned short)pid;
	ret = GxAVGetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
			GxDemuxPropertyID_SlotQueryByPid, &query_slot, sizeof(GxDemuxProperty_SlotQueryByPid));
	if (ret != 0 || query_slot.slot_id == -1) return 0;

	slot.slot_id = query_slot.slot_id;
	ret = GxAVGetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
			GxDemuxPropertyID_SlotConfig, &slot, sizeof(GxDemuxProperty_Slot));
	if (ret != 0) return 0;

	return slot.flags;
}

static int find_slot_in_module(int index, int slotpid)
{
	int i;

	CHECK_MODID(index);

	for (i=0; i<HW_DEMUX_SLOT_MAX; i++) {
		if (slotpid == hw_demux.module[index]->slot[i].slot.pid)
			return i;
	}

	return -1;
}

static int find_free_slot(int index)
{
	int i;

	CHECK_MODID(index);

	for (i=0; i<HW_DEMUX_SLOT_MAX; i++) {
		if (hw_demux.module[index]->slot[i].usecnt == 0) {
			hw_demux.module[index]->slot[i].usecnt = 1;
			return i;
		}
	}

	return -1;
}

static int update_slot_in_module(int index,int slotidx, GxDemuxProperty_Slot slot)
{
	CHECK_MODID(index);

	if (slotidx < 0 || slotidx >= HW_DEMUX_SLOT_MAX)
		return 0;

	hw_demux.module[index]->slot[slotidx].slot = slot;
	hw_demux.module[index]->slot[slotidx].usecnt++;

	return 0;
}

static int add_slot_to_module(int index,GxDemuxProperty_Slot slot)
{
	int slotidx;

	CHECK_MODID(index);
	CHECK_PID(slot.pid);

	if (hw_demux.module[index]->slotnum >= HW_DEMUX_SLOT_MAX)
		return -1;

	slotidx = find_free_slot(index);

	hw_demux.module[index]->slot[slotidx].slot = slot;
	hw_demux.module[index]->slotnum ++;

	gxlogf("[Player]: %s[%d]:%s:slotnum=%d\n", __FILE__, __LINE__, __FUNCTION__,hw_demux.module[index]->slotnum);
	return slotidx;
}

static int remove_slot_from_module(int index, GxDemuxProperty_Slot slot)
{
	int slotidx;

	CHECK_MODID(index);
	CHECK_PID(slot.pid);

	slotidx = find_slot_in_module(index,slot.pid);
	if (slotidx >= 0) {
		hw_demux.module[index]->slot[slotidx].usecnt--;
		if (hw_demux.module[index]->slot[slotidx].usecnt <= 0) {
			hw_slot_free(index, &slot);
			memset(&hw_demux.module[index]->slot[slotidx],0,sizeof(HwDemuxSlot));
			hw_demux.module[index]->slot[slotidx].slot.pid = -1;
			hw_demux.module[index]->slotnum --;
			gxlogf("[Player]: %s[%d]:%s:slotnum=%d\n", __FILE__, __LINE__, __FUNCTION__,hw_demux.module[index]->slotnum);
		}
	}
	return 0;
}

static int find_filter_in_module(int index, int slot_id)
{
	int i;

	CHECK_MODID(index);

	for (i=0;i<HW_DEMUX_FILTER_MAX;i++) {
		if (slot_id == hw_demux.module[index]->filter[i].filter.slot_id)
			return i;
	}

	return -1;
}

static int find_free_filter(int index)
{
	int i;

	CHECK_MODID(index);

	for (i=0;i<HW_DEMUX_FILTER_MAX;i++) {
		if (hw_demux.module[index]->filter[i].usecnt == 0) {
			hw_demux.module[index]->filter[i].usecnt = 1;
			return i;
		}
	}

	return -1;
}

static int add_filter_to_module(int index, GxDemuxProperty_Filter filter)
{
	int filteridx;

	CHECK_MODID(index);

	if (hw_demux.module[index]->filternum >= HW_DEMUX_FILTER_MAX)
		return -1;

	filteridx = find_free_filter(index);

	hw_demux.module[index]->filter[filteridx].filter = filter;
	hw_demux.module[index]->filternum ++;

	gxlogf("[Player]: %s[%d]:%s:filternum=%d\n", __FILE__, __LINE__, __FUNCTION__,hw_demux.module[index]->filternum);
	return filteridx;
}

static int remove_filter_from_module(int index, GxDemuxProperty_Filter filter)
{
	int filteridx;

	CHECK_MODID(index);

	filteridx = find_filter_in_module(index, filter.slot_id);
	if (filteridx >= 0) {
		hw_demux.module[index]->filter[filteridx].usecnt--;
		if (hw_demux.module[index]->filter[filteridx].usecnt <= 0) {
			GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
					GxDemuxPropertyID_FilterFree, &filter, sizeof(GxDemuxProperty_Filter));
			memset(&hw_demux.module[index]->filter[filteridx],0,sizeof(HwDemuxFilter));
			hw_demux.module[index]->filter[filteridx].filter.slot_id = -1;
			hw_demux.module[index]->filternum --;
			gxlogf("[Player]: %s[%d]:%s:filternum=%d\n", __FILE__, __LINE__, __FUNCTION__,hw_demux.module[index]->filternum);
		}
	}
	return 0;
}

static int demux_alloc_av_slot(int index, int progid, int pid, int slotflags, GxFifo* fifo, int slottype)
{
	int ret, slotidx;
	GxFifoLink FifoLink;
	GxDemuxProperty_Slot slot;

	memset(&slot,0,sizeof(GxDemuxProperty_Slot));
	slot.slot_id = -1;

	if (VAILD_PID(pid)) {
		slotidx = find_slot_in_module(index, pid);
		if (slotidx < 0) {
			slot.pid = pid;
			slot.flags = slotflags;
			slot.type = slottype;
			ret = hw_slot_alloc(index, &slot);
			CHECK_RETURN_VALUE(ret);

			slotidx = add_slot_to_module(index,slot);
		}
		else {
			slot = hw_demux.module[index]->slot[slotidx].slot;
			slot.flags |= slotflags;

			update_slot_in_module(index,slotidx,slot);
		}

		switch(slottype)
		{
		case DEMUX_SLOT_VIDEO:
			hw_demux.module[index]->prog[progid].vslotidx = slotidx;
			break;
		case DEMUX_SLOT_AUDIO:
			hw_demux.module[index]->prog[progid].aslotidx = slotidx;
			break;
		case DEMUX_SLOT_PES_AUDIO:
			hw_demux.module[index]->prog[progid].audio.slotidx = slotidx;
			break;
		case DEMUX_SLOT_PES:
		case DEMUX_SLOT_PSI:
			hw_demux.module[index]->prog[progid].subt.slotidx = slotidx;
			break;
		default:
			break;
		}

		if (slotflags & DMX_TSOUT_EN) {
			slot.ts_out_pin = hw_demux.module[index]->prog[progid].outpin[0];
			hw_demux.module[index]->slot[slotidx].slot.ts_out_pin = slot.ts_out_pin;
		}

		if (fifo) {
			FifoLink.module = hw_demux.module[index]->handle;
			FifoLink.pin_id = slot.slot_id;
			FifoLink.direction = GXAV_PIN_OUTPUT;
			ret = GxFifo_Link(fifo, &FifoLink);
			CHECK_RETURN_VALUE(ret);

			hw_demux.module[index]->slot[slotidx].fifo = fifo;

			switch(slottype)
			{
			case DEMUX_SLOT_VIDEO:
				hw_demux.module[index]->prog[progid].vfifo = fifo;
				break;
			case DEMUX_SLOT_AUDIO:
				hw_demux.module[index]->prog[progid].afifo = fifo;
				break;
			default:
				break;
			}
		}

		ret = hw_slot_config(index, &slot);
		CHECK_RETURN_VALUE(ret);

		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

static int demux_alloc_av_filter(int index, int progid, int slotidx, int filter_size)
{
	int ret, filteridx;
	GxDemuxProperty_Slot slot;
	GxDemuxProperty_Filter filter;

	slot = hw_demux.module[index]->slot[slotidx].slot;
	memset(&filter,0,sizeof(GxDemuxProperty_Filter));

	if (VAILD_PID(slot.pid)) {
		filteridx = find_filter_in_module(index, slot.slot_id);
		if (filteridx < 0) {
			filter.slot_id  = slot.slot_id;
			filter.sw_buffer_size = filter_size;
			ret = GxAVGetProperty(hw_demux.dev, hw_demux.module[index]->handle,
					GxDemuxPropertyID_FilterAlloc, &filter, sizeof(GxDemuxProperty_Filter));
			CHECK_RETURN_VALUE(ret);

			filteridx = add_filter_to_module(index, filter);
		}else
			filter = hw_demux.module[index]->filter[filteridx].filter;

		if (slotidx == hw_demux.module[index]->prog[progid].audio.slotidx) {
			hw_demux.module[index]->prog[progid].audio.filteridx = filteridx;
			filter.depth   = 0;
		} else if (slotidx == hw_demux.module[index]->prog[progid].subt.slotidx){
			hw_demux.module[index]->prog[progid].subt.filteridx = filteridx;
			if (DEMUX_SLOT_PSI == hw_demux.module[index]->slot[slotidx].slot.type) {
				filter.key[0].value = 0xc6;
				filter.key[0].mask  = 0xff;
			} else {
				filter.key[0].value = 0xbd;
				filter.key[0].mask  = 0xff;
			}
			filter.depth        = 1;
		} else
			filter.depth   = 0;

		filter.slot_id = slot.slot_id;
		filter.flags   = DMX_EQ;
		ret = GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle,
				GxDemuxPropertyID_FilterConfig, &filter, sizeof(GxDemuxProperty_Filter));
		CHECK_RETURN_VALUE(ret);

		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

static int hw_demux_stop_demux(int index,int progid)
{
	int i;
	int slotidx[DMX_SLOT_NUM];
	HwDemuxPesFilter pes[DMX_PESFILTER_NUM];
	GxFifoLink FifoLink;

	CHECK_MODID(index);
	CHECK_PROGID(progid);

	pes[0] = hw_demux.module[index]->prog[progid].audio;
	pes[1] = hw_demux.module[index]->prog[progid].subt;
	for (i = 0; i < DMX_PESFILTER_NUM; i++) {
		if ((pes[i].slotidx >= 0) && (pes[i].filteridx >= 0) &&
				VAILD_PID(hw_demux.module[index]->slot[pes[i].slotidx].slot.pid)) {
			GxDemuxProperty_Slot   *slot   = &hw_demux.module[index]->slot[pes[i].slotidx].slot;
			GxDemuxProperty_Filter *filter = &hw_demux.module[index]->filter[pes[i].filteridx].filter;

			GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
					GxDemuxPropertyID_FilterDisable, filter, sizeof(GxDemuxProperty_Filter));
			remove_filter_from_module(index, *filter);

			slot->flags &= ~(DMX_AVOUT_EN);
			hw_slot_config(index, slot);
			remove_slot_from_module(index, *slot);
		}
	}

	slotidx[0] = hw_demux.module[index]->prog[progid].aslotidx;
	slotidx[1] = hw_demux.module[index]->prog[progid].vslotidx;
	for (i = 0; i < DMX_SLOT_NUM; i++) {
		if (slotidx[i] >=0 && VAILD_PID(hw_demux.module[index]->slot[slotidx[i]].slot.pid)) {
			GxDemuxProperty_Slot *slot = &hw_demux.module[index]->slot[slotidx[i]].slot;

			slot->flags &= (~DMX_AVOUT_EN);
			hw_slot_config(index, slot);
			FifoLink.module = hw_demux.module[index]->handle;
			GxFifo_Unlink(hw_demux.module[index]->slot[slotidx[i]].fifo, &FifoLink);
			GxFifo_Reset(hw_demux.module[index]->slot[slotidx[i]].fifo);
			remove_slot_from_module(index, *slot);
		}
	}

	hw_demux.module[index]->prog[progid].aslotidx     = -1;
	hw_demux.module[index]->prog[progid].vslotidx     = -1;
	hw_demux.module[index]->prog[progid].audio.slotidx   = -1;
	hw_demux.module[index]->prog[progid].audio.filteridx = -1;
	hw_demux.module[index]->prog[progid].subt.slotidx    = -1;
	hw_demux.module[index]->prog[progid].subt.filteridx  = -1;

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int hw_demux_config_demux(int index, HwDemuxConf* conf)
{
	int ret, progid;
	GxDemuxProperty_Pcr Pcr ;
	GxSTCProperty_Config Config;
	GxSTCProperty_TimeResolution TimeResolution;
	unsigned int vslotflags, aslotflags;
	int ret_val = VIDEO_CONFIG_FAILED|AUDIO_CONFIG_FAILED;

	if (conf == NULL)
		return GX_PLAYER_ERROR;

	CHECK_MODID(index);
	CHECK_PROGID(conf->progid);

	progid = conf->progid;

	Pcr.pcr_pid = conf->pcr_id;
	GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
			GxDemuxPropertyID_Pcr, &Pcr,sizeof(GxDemuxProperty_Pcr));

	switch(conf->sync)
	{
	case NO_RECOVER:
	case PCR_RECOVER:
	case VPTS_RECOVER:
	case APTS_RECOVER:
	case AVPTS_RECOVER:
	case PURE_APTS_RECOVER:
	case FIXD_APTS_RECOVER:
	case FIXD_VPTS_RECOVER:
		Config.mode = conf->sync;
		break;
	default:
		Config.mode = AVPTS_RECOVER;
		break;
	}

	if (!VAILD_PID(conf->v_id))
		Config.mode = (conf->sync == -1) ? APTS_RECOVER : Config.mode;

	GxAVSetProperty(hw_demux.dev,  hw_demux.module[index]->stc, \
			GxSTCPropertyID_Config, &Config,sizeof(GxSTCProperty_Config));

	TimeResolution.freq_HZ = 45000;
	GxAVSetProperty(hw_demux.dev,  hw_demux.module[index]->stc, \
			GxSTCPropertyID_TimeResolution, &TimeResolution,sizeof(GxSTCProperty_TimeResolution));

	GxAVSetProperty(hw_demux.dev,  hw_demux.module[index]->stc, \
			GxSTCPropertyID_Play, NULL, 0);

	aslotflags  = (DMX_REPEAT_MODE|DMX_PTS_TO_SDRAM|DMX_AVOUT_EN|DMX_ERR_DISCARD_EN);
	vslotflags  = (DMX_REPEAT_MODE|DMX_PTS_TO_SDRAM|DMX_AVOUT_EN);
	if (conf->bind_descr) {
		aslotflags |= DMX_DES_AUTO_BIND;
		vslotflags |= DMX_DES_AUTO_BIND;
	}

	if (VAILD_PID(conf->v_id) && !conf->v_quiet) {
		ret = demux_alloc_av_slot(index, progid, conf->v_id, vslotflags, conf->vfifo, DEMUX_SLOT_VIDEO);
		if (ret == 0) {
			ret_val &= ~(VIDEO_CONFIG_FAILED);
		}
	}

	if (VAILD_PID(conf->a_id) && !conf->a_quiet) {
		ret = demux_alloc_av_slot(index, progid, conf->a_id, aslotflags, conf->afifo, DEMUX_SLOT_AUDIO);
		if (ret == 0) {
			ret_val &= ~(AUDIO_CONFIG_FAILED);
		}
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	if (ret_val == (AUDIO_CONFIG_FAILED|VIDEO_CONFIG_FAILED)) {
		hw_demux_stop_demux(index, progid);
		return GX_PLAYER_ERROR;
	}

	return ret_val;
}

static int hw_demux_run_demux(int index, int progid)
{
	int i, ret = 0;
	int slotidx[DMX_SLOT_NUM];
	HwDemuxPesFilter pes[DMX_PESFILTER_NUM];

	CHECK_MODID(index);
	CHECK_PROGID(progid);

	slotidx[0] = hw_demux.module[index]->prog[progid].aslotidx;
	slotidx[1] = hw_demux.module[index]->prog[progid].vslotidx;
	for (i = 0; i < DMX_SLOT_NUM; i++) {
		if (slotidx[i] >= 0) {
			if (VAILD_PID(hw_demux.module[index]->slot[slotidx[i]].slot.pid)) {
				ret = GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
						GxDemuxPropertyID_SlotEnable, &hw_demux.module[index]->slot[slotidx[i]].slot, sizeof(GxDemuxProperty_Slot));
				CHECK_RETURN_VALUE(ret);
			}
		}
	}

	pes[0] = hw_demux.module[index]->prog[progid].audio;
	pes[1] = hw_demux.module[index]->prog[progid].subt;
	for (i = 0; i < DMX_PESFILTER_NUM; i++) {
		if ((pes[i].slotidx >= 0) && (pes[i].filteridx >= 0) &&
				VAILD_PID(hw_demux.module[index]->slot[pes[i].slotidx].slot.pid)) {
			GxDemuxProperty_Slot   *slot   = &hw_demux.module[index]->slot[pes[i].slotidx].slot;
			GxDemuxProperty_Filter *filter = &hw_demux.module[index]->filter[pes[i].filteridx].filter;
			GxDemuxProperty_FilterFifoReset fifo;

			ret = GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
					GxDemuxPropertyID_SlotEnable, slot, sizeof(GxDemuxProperty_Slot));
			CHECK_RETURN_VALUE(ret);

			fifo.filter_id = filter->filter_id;
			ret = GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
					GxDemuxPropertyID_FilterFIFOReset, &fifo, sizeof(GxDemuxProperty_FilterFifoReset));
			CHECK_RETURN_VALUE(ret);

			ret = GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
					GxDemuxPropertyID_FilterEnable, filter, sizeof(GxDemuxProperty_Filter));
			CHECK_RETURN_VALUE(ret);
		}
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int hw_demux_reset_demux(int index, int progid)
{
	int i;
	int slotidx[DMX_SLOT_NUM];
	HwDemuxPesFilter pes[DMX_PESFILTER_NUM];
	GxFifoLink FifoLink;

	CHECK_MODID(index);
	CHECK_PROGID(progid);

	slotidx[0] = hw_demux.module[index]->prog[progid].aslotidx;
	slotidx[1] = hw_demux.module[index]->prog[progid].vslotidx;
	pes[0] = hw_demux.module[index]->prog[progid].audio;
	pes[1] = hw_demux.module[index]->prog[progid].subt;
	for (i = 0; i < DMX_PESFILTER_NUM; i++) {
		if ((pes[i].slotidx >= 0) && (pes[i].filteridx >= 0) &&
				VAILD_PID(hw_demux.module[index]->slot[pes[i].slotidx].slot.pid)) {
			GxDemuxProperty_Slot   *slot   = &hw_demux.module[index]->slot[pes[i].slotidx].slot;
			GxDemuxProperty_Filter *filter = &hw_demux.module[index]->filter[pes[i].filteridx].filter;

			GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
					GxDemuxPropertyID_FilterDisable, filter, sizeof(GxDemuxProperty_Filter));

			slot->flags &= ~(DMX_AVOUT_EN);
			hw_slot_config(index, slot);
		}
	}

	for (i = 0; i < DMX_SLOT_NUM; i++) {
		if (VAILD_PID(hw_demux.module[index]->slot[slotidx[i]].slot.pid)) {
			GxDemuxProperty_Slot *slot = &hw_demux.module[index]->slot[slotidx[i]].slot;

			slot->flags &= ~(DMX_AVOUT_EN);
			hw_slot_config(index, slot);

			FifoLink.module = hw_demux.module[index]->handle;
			GxFifo_Unlink(hw_demux.module[index]->slot[slotidx[i]].fifo, &FifoLink);
		}
	}

	for (i = 0; i < DMX_SLOT_NUM; i++) {
		if (VAILD_PID(hw_demux.module[index]->slot[slotidx[i]].slot.pid)) {
			GxDemuxProperty_Slot *slot = &hw_demux.module[index]->slot[slotidx[i]].slot;

			GxFifo_Reset(hw_demux.module[index]->slot[slotidx[i]].fifo);
			FifoLink.module = hw_demux.module[index]->handle;
			FifoLink.pin_id = hw_demux.module[index]->slot[slotidx[i]].slot.slot_id;
			FifoLink.direction = GXAV_PIN_OUTPUT;
			GxFifo_Link(hw_demux.module[index]->slot[slotidx[i]].fifo, &FifoLink);

			slot->flags |= DMX_AVOUT_EN;
			hw_slot_config(index, slot);
			GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
					GxDemuxPropertyID_SlotEnable, slot, sizeof(GxDemuxProperty_Slot));
		}
	}

	for (i = 0; i < DMX_PESFILTER_NUM; i++) {
		if ((pes[i].slotidx >= 0) && (pes[i].filteridx >= 0) &&
				VAILD_PID(hw_demux.module[index]->slot[pes[i].slotidx].slot.pid)) {
			GxDemuxProperty_Slot   *slot   = &hw_demux.module[index]->slot[pes[i].slotidx].slot;
			GxDemuxProperty_Filter *filter = &hw_demux.module[index]->filter[pes[i].filteridx].filter;
			GxDemuxProperty_FilterFifoReset fifo;

			slot->flags |= DMX_AVOUT_EN;
			hw_slot_config(index, slot);
			GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
					GxDemuxPropertyID_SlotEnable, slot, sizeof(GxDemuxProperty_Slot));

			fifo.filter_id = filter->filter_id;
			GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
					GxDemuxPropertyID_FilterFIFOReset, &fifo, sizeof(GxDemuxProperty_FilterFifoReset));

			GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
					GxDemuxPropertyID_FilterEnable, filter, sizeof(GxDemuxProperty_Filter));
		}
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}


#define SLOT_ALLOC_AND_CONFIG(slot)\
	do {\
		ret = hw_slot_alloc(index, &slot);\
		CHECK_RETURN_VALUE(ret);\
		ret = hw_slot_config(index, &slot);\
		CHECK_RETURN_VALUE(ret);\
	}while (0)

static int demux_alloc_ext_slot(int index, int progid, HwDemuxConf *conf)
{
	int ret, i=0, slot_cnt=0, slotidx;
	GxDemuxProperty_Slot slot;
	GxDemuxerRecordExtInfo *ext_info = &conf->ts_info.ext_info;

	for (i = 0; i < ext_info->ext_num; i++) {
		unsigned int pid = ext_info->ext_pids[i];

		if (VAILD_PID(pid) && (pid != conf->a_id) && (pid != conf->v_id)) {
			int encrypt = 0;
			encrypt |= (conf->ts_info.ext_info.is_encrypt) ? 0 : DMX_DES_EN;
			encrypt |= (conf->bind_descr) ? DMX_DES_AUTO_BIND : 0;

			slotidx = find_slot_in_module(index, pid);
			if (slotidx < 0) {
				int flags = hw_query_flags_by_pid(index, pid);

				slot.pid   = pid;
				slot.type  = ext_info->is_reconly ? DEMUX_SLOT_PSI: ext_info->ext_types[i];
				slot.flags = DMX_REPEAT_MODE|DMX_TSOUT_EN|encrypt|flags;
				slot.ts_out_pin = hw_demux.module[index]->prog[progid].outpin[0];
				SLOT_ALLOC_AND_CONFIG(slot);
				hw_demux.module[index]->prog[progid].ext_slot_idx[slot_cnt++] = add_slot_to_module(index, slot);
				hw_demux.module[index]->prog[progid].ext_slot_num++;
			}
			else {
				slot = hw_demux.module[index]->slot[slotidx].slot;
				slot.flags |= (DMX_TSOUT_EN|encrypt);
				slot.ts_out_pin = hw_demux.module[index]->prog[progid].outpin[0];
				ret = hw_slot_config(index, &slot);
				CHECK_RETURN_VALUE(ret);
				update_slot_in_module(index,slotidx,slot);
				hw_demux.module[index]->prog[progid].ext_slot_idx[slot_cnt++] = slotidx;
				hw_demux.module[index]->prog[progid].ext_slot_num++;
			}
		}
	}

	if (conf->ts_info.user_info.have_pmt == 0 && VAILD_PID(conf->pmt_id)) {
		/* PAT */
		int flags = 0;

		flags = hw_query_flags_by_pid(index, 0);
		slot.pid   = 0;
		slot.type  = DEMUX_SLOT_PSI;
		slot.flags = DMX_REPEAT_MODE|DMX_TSOUT_EN|flags;
		slot.ts_out_pin = hw_demux.module[index]->prog[progid].outpin[0];
		SLOT_ALLOC_AND_CONFIG(slot);
		hw_demux.module[index]->prog[progid].ext_slot_idx[slot_cnt++] = add_slot_to_module(index, slot);
		hw_demux.module[index]->prog[progid].ext_slot_num++;

		/* PMT */
		flags = hw_query_flags_by_pid(index, conf->pmt_id);
		slot.pid   = conf->pmt_id;
		slot.type  = DEMUX_SLOT_PSI;
		slot.flags = DMX_REPEAT_MODE|DMX_TSOUT_EN|flags;
		slot.ts_out_pin = hw_demux.module[index]->prog[progid].outpin[0];
		SLOT_ALLOC_AND_CONFIG(slot);
		hw_demux.module[index]->prog[progid].ext_slot_idx[slot_cnt++] = add_slot_to_module(index, slot);
		hw_demux.module[index]->prog[progid].ext_slot_num++;
	}

	/* PCR */
	if (VAILD_PID(conf->pcr_id) &&
			(conf->pcr_id != conf->v_id) &&
			(conf->pcr_id != conf->a_id) &&
			(conf->pcr_id != 0)) {
		int flags = hw_query_flags_by_pid(index, conf->pcr_id);

		slot.pid   = conf->pcr_id;
		slot.type  = DEMUX_SLOT_PSI;
		slot.flags = DMX_REPEAT_MODE|DMX_TSOUT_EN|flags;
		slot.ts_out_pin = hw_demux.module[index]->prog[progid].outpin[0];
		SLOT_ALLOC_AND_CONFIG(slot);
		hw_demux.module[index]->prog[progid].ext_slot_idx[slot_cnt++] = add_slot_to_module(index, slot);
		hw_demux.module[index]->prog[progid].ext_slot_num++;
	}

	return GX_PLAYER_OK;
}

static int demux_alloc_ts_slot(int index, int progid, HwDemuxConf *conf)
{
	int ret, slotidx;
	GxDemuxProperty_Slot slot;
	GxDemuxerRecordExtInfo *ext_info = &conf->ts_info.ext_info;

	if (VAILD_PID(conf->v_id)) {
		int encrypt = 0;

		encrypt |= (conf->ts_info.ext_info.is_encrypt) ? 0 : DMX_DES_EN;
		encrypt |= (conf->bind_descr) ? DMX_DES_AUTO_BIND : 0;

		slotidx = find_slot_in_module(index, conf->v_id);
		if (slotidx < 0) {
			slot.pid   = conf->v_id;
			slot.type  = DEMUX_SLOT_PSI;
			slot.flags = DMX_REPEAT_MODE|DMX_TSOUT_EN|encrypt;
			slot.ts_out_pin = hw_demux.module[index]->prog[progid].outpin[0];
			SLOT_ALLOC_AND_CONFIG(slot);
			hw_demux.module[index]->prog[progid].vslotidx = add_slot_to_module(index, slot);
		}
		else {
			slot = hw_demux.module[index]->slot[slotidx].slot;
			slot.flags |= (DMX_TSOUT_EN|encrypt);
			slot.ts_out_pin = hw_demux.module[index]->prog[progid].outpin[0];
			ret = hw_slot_config(index, &slot);
			CHECK_RETURN_VALUE(ret);
			update_slot_in_module(index,slotidx,slot);
			hw_demux.module[index]->prog[progid].vslotidx = add_slot_to_module(index, slot);
		}
	}

	if (VAILD_PID(conf->a_id)) {
		int encrypt = 0;

		encrypt |= (conf->ts_info.ext_info.is_encrypt) ? 0 : DMX_DES_EN;
		encrypt |= (conf->bind_descr) ? DMX_DES_AUTO_BIND : 0;

		slotidx = find_slot_in_module(index, conf->a_id);
		if (slotidx < 0) {
			slot.pid   = conf->a_id;
			slot.type  = DEMUX_SLOT_PSI;
			slot.flags = DMX_REPEAT_MODE|DMX_TSOUT_EN|encrypt;
			slot.ts_out_pin = hw_demux.module[index]->prog[progid].outpin[0];
			SLOT_ALLOC_AND_CONFIG(slot);
			hw_demux.module[index]->prog[progid].aslotidx = add_slot_to_module(index, slot);
		}
		else {
			slot = hw_demux.module[index]->slot[slotidx].slot;
			slot.flags |= (DMX_TSOUT_EN|encrypt);
			slot.ts_out_pin = hw_demux.module[index]->prog[progid].outpin[0];
			ret = hw_slot_config(index, &slot);
			CHECK_RETURN_VALUE(ret);
			update_slot_in_module(index,slotidx,slot);
			hw_demux.module[index]->prog[progid].aslotidx = add_slot_to_module(index, slot);
		}
	}

	return GX_PLAYER_OK;
}

static int hw_demux_stop_muxer(int index,int progid)
{
	int i;
	int slotidx[MUX_SLOT_NUM];
	int ext_slot_num, *ext_slot_idx;

	CHECK_MODID(index);
	CHECK_PROGID(progid);

	slotidx[0] = hw_demux.module[index]->prog[progid].vslotidx;
	slotidx[1] = hw_demux.module[index]->prog[progid].aslotidx;

	for (i=0; i<MUX_SLOT_NUM; i++) {
		if (slotidx[i]>=0 && slotidx[i]<HW_DEMUX_SLOT_MAX) {
			GxDemuxProperty_Slot *slot = &hw_demux.module[index]->slot[slotidx[i]].slot;
			if (VAILD_PID(slot->pid)) {
				GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
						GxDemuxPropertyID_SlotDisable, slot, sizeof(GxDemuxProperty_Slot));
				slot->flags &=(~DMX_TSOUT_EN);
				hw_slot_config(index, slot);
				remove_slot_from_module(index, *slot);
			}
		}
	}

	ext_slot_num = hw_demux.module[index]->prog[progid].ext_slot_num;
	ext_slot_idx = hw_demux.module[index]->prog[progid].ext_slot_idx;
	if (ext_slot_num>0) {
		for (i=0 ; i<ext_slot_num; i++) {
			GxDemuxProperty_Slot *slot = &hw_demux.module[index]->slot[*(ext_slot_idx+i)].slot;
			if (VAILD_PID(slot->pid)) {
				slot->flags &=(~DMX_TSOUT_EN);
				hw_slot_config(index, slot);
				remove_slot_from_module(index, *slot);
			}
		}
	}

	hw_demux.module[index]->prog[progid].aslotidx     = -1;
	hw_demux.module[index]->prog[progid].vslotidx     = -1;
	hw_demux.module[index]->prog[progid].ext_slot_num = 0;

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int hw_demux_config_muxer(int index, HwDemuxConf* conf)
{
	int ret, progid;
	GxFifoConfig FifoConfig;
	unsigned int vslotflags, aslotflags;
	int ret_val = MUXER_CONFIG_FAILED;
	GxFifoShallowInfo info;

	if (conf == NULL)
		return ret_val;

	CHECK_MODID(index);
	CHECK_PROGID(conf->progid);

	progid = conf->progid;

	memset(&FifoConfig, 0, sizeof(FifoConfig));

	FifoConfig.dev = hw_demux.dev;
	FifoConfig.dvrid = index;
	FifoConfig.source = DVR_INPUT_DMX;
	FifoConfig.dest = DVR_OUTPUT_MEM;
	FifoConfig.sw_buffer_size = conf->tsfifo->size;
	GxPlayer_SystemGet(PSYS_RECORD_TSW_MEMSIZE, &FifoConfig.hw_buffer_size);

	if (conf->ts_info.ext_info.is_rawts)
		FifoConfig.source = DVR_INPUT_TSPORT;

	if (!VAILD_PID(conf->v_id)) {
		FifoConfig.hw_buffer_size = 188 * 1024;
		FifoConfig.almost_full_gate = 188 *30;
	}

	if (conf->ts_info.ctrl_info.encrypt_enable)
		FifoConfig.blocklen = conf->ts_info.ctrl_info.block_size;
	else
		FifoConfig.blocklen = RECODER_BLOCK_SIZE;

	ret = GxFifo_Config(conf->tsfifo, &FifoConfig);
	CHECK_RETURN_VALUE(ret);

	GxFifo_GetShallowInfo(conf->tsfifo, &info);
	if (info.block_size > 0)
		conf->ts_info.ctrl_info.block_size = info.block_size;
	conf->ts_info.ctrl_info.tsw_security = info.security_flag;
	conf->ts_info.ctrl_info.tsw_ptr_en   = info.ptr_enable;

	hw_demux.module[index]->prog[progid].outpin[0] = FifoConfig.dvrhandle;
	if (conf->ts_info.ext_info.is_rawts)
		goto out;

	aslotflags = (DMX_REPEAT_MODE|DMX_PTS_TO_SDRAM|DMX_TSOUT_EN);
	vslotflags = (DMX_REPEAT_MODE|DMX_PTS_TO_SDRAM|DMX_TSOUT_EN);


	if (conf->ts_info.ext_info.is_encrypt == 0) {
		vslotflags |= (DMX_DES_EN);
		aslotflags |= (DMX_DES_EN);
	}

	if (conf->bind_descr) {
		vslotflags |= (DMX_DES_AUTO_BIND);
		aslotflags |= (DMX_DES_AUTO_BIND);
	}

	if (conf->ts_info.ext_info.is_reconly == 0) {
		if (VAILD_PID(conf->v_id)) {
			ret = demux_alloc_av_slot(index, progid, conf->v_id, vslotflags, conf->vfifo, DEMUX_SLOT_VIDEO);
			CHECK_RETURN_VALUE(ret);
		}

		if (VAILD_PID(conf->a_id)) {
			ret = demux_alloc_av_slot(index, progid, conf->a_id, aslotflags, conf->afifo, DEMUX_SLOT_AUDIO);
			CHECK_RETURN_VALUE(ret);
		}
	}
	else {
		ret = demux_alloc_ts_slot(index, progid, conf);
		CHECK_RETURN_VALUE(ret);
	}

	ret = demux_alloc_ext_slot(index, progid, conf);
	if (ret != 0) {
		hw_demux_stop_muxer(index, progid);
		return ret_val;
	}

out:
	ret_val &= ~(MUXER_CONFIG_FAILED);
	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return ret_val;
}

static int hw_demux_run_muxer(int index,int progid)
{
	int i;
	int ret = 0;
	int slotidx[MUX_SLOT_NUM];
	int ext_slot_num, *ext_slot_idx;

	CHECK_MODID(index);
	CHECK_PROGID(progid);

	slotidx[0] = hw_demux.module[index]->prog[progid].aslotidx;
	slotidx[1] = hw_demux.module[index]->prog[progid].vslotidx;

	for (i=0; i<MUX_SLOT_NUM; i++) {
		if (slotidx[i] >= 0) {
			if (VAILD_PID(hw_demux.module[index]->slot[slotidx[i]].slot.pid) ||
					hw_demux.module[index]->slot[slotidx[i]].slot.pid == 0) {
				ret = GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
						GxDemuxPropertyID_SlotEnable, &hw_demux.module[index]->slot[slotidx[i]].slot, sizeof(GxDemuxProperty_Slot));
				CHECK_RETURN_VALUE(ret);
			}
		}
	}

	ext_slot_num = hw_demux.module[index]->prog[progid].ext_slot_num;
	ext_slot_idx = hw_demux.module[index]->prog[progid].ext_slot_idx;
	if (ext_slot_num > 0) {
		for (i=0; i<ext_slot_num; i++) {
			ret = GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
					GxDemuxPropertyID_SlotEnable, &hw_demux.module[index]->slot[*(ext_slot_idx+i)].slot, sizeof(GxDemuxProperty_Slot));
			CHECK_RETURN_VALUE(ret);
		}
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int hw_demux_module_init(int index)
{
	int i;

	CHECK_MODID(index);

	hw_demux.module[index] = av_mallocz(sizeof(HwDemuxMod));
	if (hw_demux.module[index] == NULL)
		return -1;

	for (i=0; i<HW_DEMUX_SLOT_MAX; i++) {
		hw_demux.module[index]->slot[i].slot.pid = -1;
	}

	for (i=0; i<HW_DEMUX_FILTER_MAX; i++) {
		hw_demux.module[index]->filter[i].filter.slot_id = -1;
	}

	return 0;
}

static int hw_demux_module_uninit(int index)
{
	int i;

	CHECK_MODID(index);

	if (hw_demux.module[index] == NULL)
		return -1;

	av_freep(&hw_demux.module[index]);

	return 0;
}


static int hw_demux_device_init(void)
{
	int i,j;

	memset(&hw_demux,0,sizeof(hw_demux));

	return 0;
}

static int hw_demux_open_device(int dev)
{
	GxHwDemux_Lock();

	if (hw_demux.opened == 0) {
		hw_demux_device_init();

		hw_demux.dev = GxAvdev_CreateDevice(dev);
		if (hw_demux.dev < 0) {
			GxHwDemux_Unlock();
			return GX_PLAYER_ERROR;
		}

		hw_demux.opened = 1;
	}

	hw_demux.opencnt++;

	GxHwDemux_Unlock();

	return GX_PLAYER_OK;
}

static int hw_demux_close_device(void)
{
	GxHwDemux_Lock();

	if (hw_demux.opencnt == 1)
		GxAvdev_DestroyDevice(hw_demux.dev);
	else
		hw_demux.opencnt--;

	GxHwDemux_Unlock();

	return GX_PLAYER_OK;
}

int GxHwDemux_OpenModule(int index)
{
	CHECK_MODID(index);

	if (hw_demux_open_device(0) != GX_PLAYER_OK)
		return GX_PLAYER_ERROR;

	GxHwDemux_Lock();

	if (hw_demux.module[index] == NULL || hw_demux.module[index]->opened == 0) {
		if (hw_demux_module_init(index) != 0) {
			GxHwDemux_Unlock();
			return GX_PLAYER_ERROR;
		}

		hw_demux.module[index]->handle = GxAvdev_OpenModule(hw_demux.dev, GXAV_MOD_DEMUX,index);
		hw_demux.module[index]->stc = GxAvdev_OpenModule(hw_demux.dev, GXAV_MOD_STC, 0);
		if (hw_demux.module[index]->handle < 0) {
			GxHwDemux_Unlock();
			return GX_PLAYER_ERROR;
		}

		hw_demux.module[index]->opened    = 1;
		hw_demux.module[index]->opencnt   = 1;
	}
	else
		hw_demux.module[index]->opencnt++;

	GxHwDemux_Unlock();

	return GX_PLAYER_OK;
};

int GxHwDemux_CloseModule(int index)
{
	CHECK_MODID(index);

	GxHwDemux_Lock();

	if (hw_demux.module[index]->opencnt == 1) {
		GxAvdev_CloseModule(hw_demux.dev, hw_demux.module[index]->handle);
		GxAvdev_CloseModule(hw_demux.dev, hw_demux.module[index]->stc);
		hw_demux_module_uninit(index);
	}
	else
		hw_demux.module[index]->opencnt--;

	GxHwDemux_Unlock();

	hw_demux_close_device();

	return GX_PLAYER_OK;
}

int GxHwDemux_Config(int index, HwDemuxConf *conf)
{
	int ret = GX_PLAYER_ERROR;

	if (conf == NULL)
		return ret;

	switch(conf->type) {
	case HW_DEMUX_FUNC_DEMUX:
		ret = hw_demux_config_demux(index, conf);
		break;
	case HW_DEMUX_FUNC_MUXER:
		ret = hw_demux_config_muxer(index, conf);
		break;
	default:
		break;
	}
	return ret;
}

int GxHwDemux_Run(int index, int progid, int type)
{
	int ret = GX_PLAYER_ERROR;

	switch(type) {
	case HW_DEMUX_FUNC_DEMUX:
		ret = hw_demux_run_demux(index,progid);
		break;
	case HW_DEMUX_FUNC_MUXER:
		ret = hw_demux_run_muxer(index,progid);
		break;
	default:
		break;
	}
	return ret;
}

int GxHwDemux_Stop(int index, int progid, int type)
{
	int ret = GX_PLAYER_ERROR;

	switch(type) {
	case HW_DEMUX_FUNC_DEMUX:
		ret = hw_demux_stop_demux(index,progid);
		break;
	case HW_DEMUX_FUNC_MUXER:
		ret = hw_demux_stop_muxer(index,progid);
		break;
	default:
		break;
	}
	return ret;
}

int GxHwDemux_Reset(int index, int progid, int type)
{
	int ret = GX_PLAYER_ERROR;

	switch(type) {
	case HW_DEMUX_FUNC_DEMUX:
		ret = hw_demux_reset_demux(index, progid);
		break;
	default:
		break;
	}
	return ret;
}

int GxHwDemux_CheckRecordOverflow(int index, int progid)
{
	unsigned int event = 0;

	if (index ==0) {
		GxAVWaitEvents(hw_demux.dev, hw_demux.module[index]->handle, EVENT_DEMUX0_TS_OVERFLOW, 1000, &event);
		if (event == EVENT_DEMUX0_TS_OVERFLOW) {
			return 1;
		}
	}
	else {
		GxAVWaitEvents(hw_demux.dev, hw_demux.module[index]->handle, EVENT_DEMUX1_TS_OVERFLOW, 1000, &event);
		if (event == EVENT_DEMUX1_TS_OVERFLOW) {
			return 1;
		}
	}

	return 0;
}

int GxHwDemux_SaveStream(int index, int progid, int type)
{
	int slotidx   = -1;
	int filteridx = -1;
	GxFifo* fifo = 0;

	CHECK_MODID(index);
	CHECK_PROGID(progid);

	slotidx   = find_slot_idx  (index, progid, type);
	filteridx = find_filter_idx(index, progid, type);
	fifo      = find_slot_fifo (index, progid, type);

	CHECK_SLOTID(slotidx);
	CHECK_PID(hw_demux.module[index]->slot[slotidx].slot.pid);

	if (filteridx >= 0) {
		GxDemuxProperty_Filter *filter = &hw_demux.module[index]->filter[filteridx].filter;
		GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
				GxDemuxPropertyID_FilterDisable, filter, sizeof(GxDemuxProperty_Filter));
	}

	hw_demux.module[index]->slot[slotidx].slot.flags &=(~DMX_AVOUT_EN);
	hw_slot_config(index, &hw_demux.module[index]->slot[slotidx].slot);
	if (fifo) {
		GxFifoLink FifoLink;
		FifoLink.module = hw_demux.module[index]->handle;
		GxFifo_Unlink(fifo, &FifoLink);
		GxFifo_Reset(fifo);
	}

	GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
			GxDemuxPropertyID_SlotDisable, &hw_demux.module[index]->slot[slotidx].slot, sizeof(GxDemuxProperty_Slot));

	return GX_PLAYER_OK;
}


int GxHwDemux_RestoreStream(int index, int progid, int type)
{
	int ret;
	int slotidx   = -1;
	int filteridx = -1;
	GxFifo* fifo  = NULL;

	CHECK_MODID(index);
	CHECK_PROGID(progid);

	slotidx   = find_slot_idx  (index, progid, type);
	filteridx = find_filter_idx(index, progid, type);
	fifo      = find_slot_fifo (index, progid, type);

	CHECK_SLOTID(slotidx);
	CHECK_PID(hw_demux.module[index]->slot[slotidx].slot.pid);

	hw_demux.module[index]->slot[slotidx].slot.flags |= DMX_AVOUT_EN;
	ret = GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
			GxDemuxPropertyID_SlotConfig, &hw_demux.module[index]->slot[slotidx].slot, sizeof(GxDemuxProperty_Slot));
	CHECK_RETURN_VALUE(ret);

	if (fifo) {
		GxFifoLink FifoLink;
		FifoLink.module = hw_demux.module[index]->handle;
		FifoLink.pin_id = hw_demux.module[index]->slot[slotidx].slot.slot_id;
		FifoLink.direction = GXAV_PIN_OUTPUT;
		GxFifo_Reset(fifo);
		ret = GxFifo_Link(fifo, &FifoLink);
		CHECK_RETURN_VALUE(ret);
	}

	ret = GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
			GxDemuxPropertyID_SlotEnable, &hw_demux.module[index]->slot[slotidx].slot, sizeof(GxDemuxProperty_Slot));
	CHECK_RETURN_VALUE(ret);

	if ((type == HW_DEMUX_PES_AUDIO) || (type == HW_DEMUX_PES_SUBT)
		||(type == HW_DEMUX_PSI_SUBT)) {
		GxDemuxProperty_FilterFifoReset filterfifo;

		filterfifo.filter_id = hw_demux.module[index]->filter[filteridx].filter.filter_id;
		ret = GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
				GxDemuxPropertyID_FilterFIFOReset, &filterfifo, sizeof(GxDemuxProperty_FilterFifoReset));
		CHECK_RETURN_VALUE(ret);

		ret = GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
				GxDemuxPropertyID_FilterEnable, &hw_demux.module[index]->filter[filteridx].filter, sizeof(GxDemuxProperty_Filter));
		CHECK_RETURN_VALUE(ret);
	}

	return GX_PLAYER_OK;
}

static int hw_demux_switch_stream(int index, int progid, int type, int pid)
{
	int slotidx = -1;
	int filteridx = -1;
	GxFifo* fifo = 0;

	CHECK_MODID(index);
	CHECK_PROGID(progid);

	slotidx   = find_slot_idx  (index, progid, type);
	filteridx = find_filter_idx(index, progid, type);
	fifo      = find_slot_fifo (index, progid, type);

	CHECK_SLOTID(slotidx);
	CHECK_PID(hw_demux.module[index]->slot[slotidx].slot.pid);

	if (filteridx >= 0) {
		GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
				GxDemuxPropertyID_FilterDisable, &hw_demux.module[index]->filter[filteridx].filter, sizeof(GxDemuxProperty_Filter));
	}

	GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
			GxDemuxPropertyID_SlotDisable, &hw_demux.module[index]->slot[slotidx].slot, sizeof(GxDemuxProperty_Slot));
	if (fifo) {
		GxFifoLink FifoLink;
		FifoLink.pin_id = hw_demux.module[index]->slot[slotidx].slot.slot_id;
		FifoLink.direction = GXAV_PIN_OUTPUT;
		FifoLink.module = hw_demux.module[index]->handle;
		GxFifo_Unlink(fifo, &FifoLink);
		GxFifo_Reset(fifo);
		GxFifo_Link(fifo, &FifoLink);
	}

	if (VAILD_PID(pid)) {
		hw_demux.module[index]->slot[slotidx].slot.pid = pid;
		hw_slot_config(index, &hw_demux.module[index]->slot[slotidx].slot);
	}

	GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
			GxDemuxPropertyID_SlotEnable, &hw_demux.module[index]->slot[slotidx].slot, sizeof(GxDemuxProperty_Slot));

	if (filteridx >= 0) {
		GxDemuxProperty_FilterFifoReset fifo;

		fifo.filter_id = hw_demux.module[index]->filter[filteridx].filter.filter_id;
		GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
				GxDemuxPropertyID_FilterFIFOReset, &fifo, sizeof(GxDemuxProperty_FilterFifoReset));

		GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
				GxDemuxPropertyID_FilterEnable, &hw_demux.module[index]->filter[filteridx].filter, sizeof(GxDemuxProperty_Filter));
	}

	return GX_PLAYER_OK;
}

int GxHwDemux_SwitchStream(int index, int progid, int type, int pid, int bind_descr)
{
	int slotidx = -1, filteridx = -1;

	CHECK_MODID(index);
	CHECK_PROGID(progid);

	slotidx   = find_slot_idx  (index, progid, type);
	filteridx = find_filter_idx(index, progid, type);

	if ((slotidx < 0) ||
			(hw_demux.module[index]->slot[slotidx].usecnt > 1) ||
			(find_slot_in_module(index, pid) >= 0) ||
			((filteridx < 0) &&
				(type == HW_DEMUX_PES_SUBT ||
				 type == HW_DEMUX_PSI_SUBT ||
				 type == HW_DEMUX_PES_AUDIO))) {
		GxHwDemux_StopStream(index, progid, type);
		GxHwDemux_RunStream(index, progid, type, pid, bind_descr);
		return 0;
	} else {
		return hw_demux_switch_stream(index, progid, type, pid);
	}
}

int GxHwDemux_ResetStream(int index, int progid, int type)
{
	return hw_demux_switch_stream(index, progid, type, -1);
}

int GxHwDemux_StopStream(int index,int progid,int type)
{
	int slotidx   = -1;
	int filteridx = -1;
	GxFifo* fifo = 0;

	CHECK_MODID(index);
	CHECK_PROGID(progid);

	slotidx   = find_slot_idx  (index, progid, type);
	filteridx = find_filter_idx(index, progid, type);
	fifo      = find_slot_fifo (index, progid, type);

	switch(type) {
	case HW_DEMUX_VIDEO:
		hw_demux.module[index]->prog[progid].vslotidx = -1;
		break;
	case HW_DEMUX_AUDIO:
		hw_demux.module[index]->prog[progid].aslotidx = -1;
		break;
	case HW_DEMUX_PES_AUDIO:
		hw_demux.module[index]->prog[progid].audio.slotidx = -1;
		break;
	case HW_DEMUX_PES_SUBT:
	case HW_DEMUX_PSI_SUBT:
		hw_demux.module[index]->prog[progid].subt.slotidx = -1;
		break;
	default:
		return GX_PLAYER_ERROR;
	}

	CHECK_SLOTID(slotidx);
	CHECK_PID(hw_demux.module[index]->slot[slotidx].slot.pid);

	if (filteridx >= 0) {
		GxDemuxProperty_Filter *filter = &hw_demux.module[index]->filter[filteridx].filter;
		GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
				GxDemuxPropertyID_FilterDisable, filter, sizeof(GxDemuxProperty_Filter));
		remove_filter_from_module(index, *filter);
	}

	hw_demux.module[index]->slot[slotidx].slot.flags &=(~DMX_AVOUT_EN);
	hw_slot_config(index, &hw_demux.module[index]->slot[slotidx].slot);
	if (fifo) {
		GxFifoLink FifoLink;
		FifoLink.module = hw_demux.module[index]->handle;
		GxFifo_Unlink(fifo, &FifoLink);
		GxFifo_Reset(fifo);
	}
	remove_slot_from_module(index,hw_demux.module[index]->slot[slotidx].slot);

	return GX_PLAYER_OK;
}

int GxHwDemux_RunStream(int index,int progid,int type, int pid, int bind_descr)
{
	int ret, slotidx;
	GxFifo* fifo = 0;
	unsigned int slotflags,slottype;

	CHECK_PID(pid);
	CHECK_MODID(index);
	CHECK_PROGID(progid);

	switch(type)
	{
	case HW_DEMUX_VIDEO:
		slotflags = (DMX_REPEAT_MODE|DMX_PTS_TO_SDRAM|DMX_AVOUT_EN|DMX_ERR_DISCARD_EN);
		slottype  = DEMUX_SLOT_VIDEO;
		break;
	case HW_DEMUX_AUDIO:
		slotflags = (DMX_REPEAT_MODE|DMX_PTS_TO_SDRAM|DMX_AVOUT_EN);
		slottype  = DEMUX_SLOT_AUDIO;
		break;
	case HW_DEMUX_PES_AUDIO:
		slotflags  = (DMX_REPEAT_MODE|DMX_AVOUT_EN);
		slottype   = DEMUX_SLOT_PES_AUDIO;
		break;
	case HW_DEMUX_PES_SUBT:
		slotflags  = (DMX_REPEAT_MODE|DMX_AVOUT_EN);
		slottype   = DEMUX_SLOT_PES;
		break;
	case HW_DEMUX_PSI_SUBT:
		slotflags  = (DMX_REPEAT_MODE|DMX_AVOUT_EN);
		slottype   = DEMUX_SLOT_PSI;
		break;
	default:
		return GX_PLAYER_ERROR;
	}

	if (bind_descr)
		slotflags |= (DMX_DES_AUTO_BIND);

	fifo = find_slot_fifo(index, progid, type);
	ret = demux_alloc_av_slot(index, progid, pid, slotflags, fifo, slottype);
	if (ret != 0) {
		hw_demux_stop_demux(index, progid);
		return GX_PLAYER_ERROR;
	}

	slotidx = find_slot_idx(index, progid, type);
	ret = GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
			GxDemuxPropertyID_SlotEnable, &hw_demux.module[index]->slot[slotidx].slot, sizeof(GxDemuxProperty_Slot));
	CHECK_RETURN_VALUE(ret);

	if ((type == HW_DEMUX_PES_AUDIO) || (type == HW_DEMUX_PES_SUBT)
		||(type == HW_DEMUX_PSI_SUBT)) {
		GxDemuxProperty_FilterFifoReset fifo;
		int filteridx = -1;

		ret = demux_alloc_av_filter(index, progid, slotidx, 256*1024);
		if (ret != 0) {
			hw_demux_stop_demux(index, progid);
			return GX_PLAYER_ERROR;
		}

		if (type == HW_DEMUX_PES_AUDIO)
			filteridx = hw_demux.module[index]->prog[progid].audio.filteridx;
		else if ((type == HW_DEMUX_PES_SUBT)||(type == HW_DEMUX_PSI_SUBT))
			filteridx = hw_demux.module[index]->prog[progid].subt.filteridx;

		fifo.filter_id = hw_demux.module[index]->filter[filteridx].filter.filter_id;
		ret = GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
				GxDemuxPropertyID_FilterFIFOReset, &fifo, sizeof(GxDemuxProperty_FilterFifoReset));
		CHECK_RETURN_VALUE(ret);

		ret = GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
				GxDemuxPropertyID_FilterEnable, &hw_demux.module[index]->filter[filteridx].filter, sizeof(GxDemuxProperty_Filter));
		CHECK_RETURN_VALUE(ret);
	}

	return GX_PLAYER_OK;
}

int GxHwDemux_AllocProg(int index)
{
	int i;

	CHECK_MODID(index);

	for (i=0;i<HW_DEMUX_PROG_MAX;i++) {
		if (hw_demux.module[index]->prog[i].used == 0) {
			hw_demux.module[index]->prog[i].used         = 1;
			hw_demux.module[index]->prog[i].vslotidx     = -1;
			hw_demux.module[index]->prog[i].aslotidx     = -1;
			hw_demux.module[index]->prog[i].audio.slotidx   = -1;
			hw_demux.module[index]->prog[i].audio.filteridx = -1;
			hw_demux.module[index]->prog[i].subt.slotidx    = -1;
			hw_demux.module[index]->prog[i].subt.filteridx  = -1;
			hw_demux.module[index]->prog[i].ext_slot_num = 0;
			//hw_demux.module[index]->prog[i].outpin[0] = PIN_ID_SDRAM_OUTPUT+i;
			hw_demux.module[index]->prognum++;
			return i;
		}
	}

	return -1;
}

int GxHwDemux_FreeProg(int index,int progid)
{
	CHECK_MODID(index);
	CHECK_PROGID(progid);

	if (progid >= HW_DEMUX_PROG_MAX)
		return -1;

	hw_demux.module[index]->prognum-- ;

	memset(&hw_demux.module[index]->prog[progid],0,sizeof(HwDemuxProg));

	return 0;
}

int GxHwDemux_GetSource(int index, int *source)
{
	GxDemuxProperty_ConfigDemux cfg_dmx;

	CHECK_MODID(index);

	GxAVGetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
			GxDemuxPropertyID_Config, &cfg_dmx, sizeof(GxDemuxProperty_ConfigDemux));

	*source = cfg_dmx.source;

	return GX_PLAYER_OK;
}

int GxHwDemux_GetDMXMod(int index)
{
	CHECK_MODID(index);

	return hw_demux.module[index]->handle;
}

int GxHwDemux_GetSlotID(int index, int pid)
{
	int slotidx = -1;
	GxDemuxProperty_Slot *slot = NULL;

	CHECK_MODID(index);

	slotidx = find_slot_in_module(index, pid);
	if (slotidx >= 0)
	   slot = &hw_demux.module[index]->slot[slotidx].slot;

	return ((slot != NULL) ? slot->slot_id : -1);
}

int GxHwDemux_GetDVRMod(int index, int progid)
{
	CHECK_MODID(index);
	CHECK_PROGID(progid);

	return hw_demux.module[index]->prog[progid].outpin[0];
}

int GxHwDemux_SetSource(int index, int source)
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

	cfg_dmx.source = source;

	CHECK_MODID(index);
	gxlogf("[Player]: %s[%d]:%s---->source:ts%d\n", __FILE__, __LINE__, __FUNCTION__,source);

	GxAVSetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
			GxDemuxPropertyID_Config, &cfg_dmx, sizeof(GxDemuxProperty_ConfigDemux));

	return GX_PLAYER_OK;
}

#define CHECK_RETURN_ZERO(ret);  \
	if (ret != 0) {\
		gxlogf("%s[%d]:%s###################error##########\n", __FILE__, __LINE__, __FUNCTION__);\
		return 0; \
	}

int GxHwDemux_ReadFilterData(int index, int progid, int type, uint8_t* buffer, int size)
{
	int ret = 0;
	int filteridx = -1;
	GxDemuxProperty_Filter          filter;
	GxDemuxProperty_FilterFifoQuery query;
	GxDemuxProperty_FilterRead      filter_read;

	if (type == HW_DEMUX_PES_AUDIO)
		filteridx = hw_demux.module[index]->prog[progid].audio.filteridx;
	else if (type == HW_DEMUX_PES_SUBT)
		filteridx = hw_demux.module[index]->prog[progid].subt.filteridx;
	else if (type == HW_DEMUX_PSI_SUBT)
		filteridx = hw_demux.module[index]->prog[progid].subt.filteridx;

	if (filteridx < 0)
		return -1;

	filter = hw_demux.module[index]->filter[filteridx].filter;
	ret = GxAVGetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
			GxDemuxPropertyID_FilterFIFOQuery, &query, sizeof(GxDemuxProperty_FilterFifoQuery));
	CHECK_RETURN_ZERO(ret);

	if (((query.state >> filter.filter_id) & 1) == 1) {
		filter_read.filter_id  = filter.filter_id;
		filter_read.max_size   = size;
		filter_read.buffer     = buffer;

		ret = GxAVGetProperty(hw_demux.dev, hw_demux.module[index]->handle, \
				GxDemuxPropertyID_FilterRead, &filter_read, sizeof(GxDemuxProperty_FilterRead));
		CHECK_RETURN_ZERO(ret);

		return (filter_read.read_size>0)?filter_read.read_size:0;
	}
	return 0;
}
