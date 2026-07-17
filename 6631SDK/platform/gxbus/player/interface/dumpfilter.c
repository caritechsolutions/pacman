#include <stdio.h>
#include "gx_dumpfilter.h"
#include <devapi/gxmtc_api.h>

#define TICKTIME_MINUS_MS(end, start)\
	(((end.seconds*1000 + end.microsecs/1000)-(start.seconds*1000 + start.microsecs/1000)))

#define GX_DUMPER_CLASS_MAX (8)
static int dumper_class_num = 0;
static GxDumpFilterClass* gx_dumpfilter_classes[GX_DUMPER_CLASS_MAX];
void GxDumpFilterRegister(GxDumpFilterClass* cls)
{
	int i;
	if (dumper_class_num == 0)
		memset(&gx_dumpfilter_classes, 0, sizeof(gx_dumpfilter_classes));

	if (dumper_class_num >= GX_DUMPER_CLASS_MAX)
		return;

	for (i=0; i<dumper_class_num; i++) {
		if (gx_dumpfilter_classes[i] == cls)
			return;
	}

	gx_dumpfilter_classes[dumper_class_num++] = cls;
}

static GxDumpFilterClass* dumpfilter_get_class_from_type(int type)
{
	int i;

	for (i = 0; gx_dumpfilter_classes[i]; i++)
		if (type == gx_dumpfilter_classes[i]->type)
			return gx_dumpfilter_classes[i];

	return NULL;
}

static void* dumpfilter_create(GxObject* obj)
{
	return obj;
}

static void dumpfilter_release(GxObject* obj)
{
	GxDumpFilter* df = GXDUMPFILTER(obj);

	if (df && df->s)
	{
		gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
		GxStream_Close(df->s);
		df->s = NULL;
	}

	return;
}

static int dumpfilter_pause(GxMediaFilter* filter)
{
	GxDumpFilter* df = GXDUMPFILTER(filter);
	if (df) {
		GxCore_GetTickTime(&df->pause_ticktime);
		gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	}
	return 0;
}

static int dumpfilter_resume(GxMediaFilter* filter)
{
	GxDumpFilter* df = GXDUMPFILTER(filter);
	if (df) {
		GxTime cur_time;
		GxCore_GetTickTime(&cur_time);
		df->pause_timems += TICKTIME_MINUS_MS(cur_time, df->pause_ticktime);
		gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	}
	return 0;
}

static int dumpfilter_stop(GxMediaFilter* filter)
{
	GxDumpFilter* df = GXDUMPFILTER(filter);

	if (df->s) {
		if (df->thread) {
			GxCore_ThreadJoin(df->thread);
			df->thread = 0;
		}
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

int GxDumpfilter_Heartbeats(GxDumpFilter* df)
{
	int time_val_ms;
	GxTime cur_time;

#define CHECK_EMPTY_TIME (3000)
	/* init start ticktime */
	if (df->start_ticktime.seconds == 0)
		GxCore_GetTickTime(&df->start_ticktime);

	/* insert timestamp */
	GxCore_GetTickTime(&cur_time);
	time_val_ms  = TICKTIME_MINUS_MS(cur_time, df->start_ticktime);
	time_val_ms -= df->pause_timems;
	time_val_ms -= df->empty_timems;

	if ((time_val_ms - df->timems[1]) > CHECK_EMPTY_TIME) {
		df->empty_timems += (time_val_ms - df->timems[1]);
		time_val_ms -= (time_val_ms - df->timems[1]);
	}

	df->timems[0] = df->timems[1];
	df->timems[1] = time_val_ms;

	return GX_PLAYER_OK;
}

static void dumpfilter_sendmsg(void* filter, GxMediaFilterEventType msg)
{
	GxMediaFilter* mf = GXMEDIAFILTER(filter);

	if (mf->event.func) {
		GxMediaFilterEventPara EventPara;
		EventPara.type = msg;
		EventPara.arg  = NULL;
		mf->event.func(mf->event.priv, &EventPara);
	}
}

static void dumpfilter_thread(void* filter)
{
	uint64_t totle_size = 0;
	int write_size = 0, read_size = 0;
	GxDumpFilter* df = GXDUMPFILTER(filter);
	GxMediaFilter* mf = GXMEDIAFILTER(filter);
	GxDumpFilterClass* cls = GetDumpFilterClassFromObject(mf);

	while ((mf->status == GX_MFT_STATE_RUNNING || mf->status == GX_MFT_STATE_PAUSED) && !df->eof) {
		read_size = cls->read(df);
		if (mf->status == GX_MFT_STATE_RUNNING) {
			if (read_size > 0) {
				if (df->write_full == 0) {
					GxTime cur0_time, cur1_time;
					int write_timems = 0;

					GxCore_GetTickTime(&cur0_time);
					write_size = cls->write(df);
					GxCore_GetTickTime(&cur1_time);
					write_timems = TICKTIME_MINUS_MS(cur1_time, cur0_time);
					if (write_timems > 800)
						gxlogi("[Record]-[%d]: write over time %d(ms)\n", __LINE__, write_timems);
#ifdef DEBUG_DUMPFILTER_WRITE_SPEED
					df->write_bksize[df->idx] += write_size;
					df->write_costms[df->idx] += write_timems;
					if (df->write_costms[df->idx] > 1000) {
						unsigned int i = 0;
						unsigned int t = 0, b = 0;
						unsigned int c = sizeof(df->write_bksize)/sizeof(df->write_bksize[0]);

						if (df->cnt < c)
							df->cnt++;
						for (i = 0; i < df->cnt; i++) {
							b += df->write_bksize[i];
							t += df->write_costms[i];
						}
						gxlogd ("write speed: %.2f(KB/s)\n", ((float)b)*1000/1024/t);
						df->idx++;
						df->idx %= c;
						df->write_bksize[df->idx] = 0;
						df->write_costms[df->idx] = 0;
					}
#endif
				} else
					write_size = 0;

				if (write_size <= 0) {
					int64_t space_size = 0;
					int64_t res_size = 0;
					int keep_size = 0;

					GxPlayer_SystemGet(PSYS_PVR_RESERVE_SIZEMB, &keep_size);
					res_size = keep_size;
					res_size <<= 20;
					GxStream_Control(df->s, GX_STREAM_CTRL_GET_CAP_SIZE, (void *)&space_size);
					if (space_size <= res_size) {
						if (df->write_full == 0) {
							gxlogi("\n[Record]-[%d]: Disk Full!... \n", __LINE__);
							dumpfilter_sendmsg(filter, GX_MFT_EVENT_RECORD_FULL);
						}
						df->write_full = 1;
					} else {
						if (df->write_full == 0) {
							gxlogi("\n[Record]: Disk Access Failed !.. wsize:%d\n", write_size);
							dumpfilter_sendmsg(filter, GX_MFT_EVENT_RECORD_END);
							break;
						}
					}
				}
				else {
					struct vol_info info;
					GxStream_Control(df->s, GX_STREAM_CTRL_GET_VOL_INFO, (void *)&info);
					totle_size = info.truesize;
					totle_size += write_size;
					if (df->cap > 0 && totle_size >= df->cap) {
						if (df->write_full == 0) {
							gxlogi("\n[Record]-[%d]: Disk Full!... \n", __LINE__);
							dumpfilter_sendmsg(filter, GX_MFT_EVENT_RECORD_FULL);
						}
						df->write_full = 1;
					} else
						df->write_full = 0;
				}
			}
			GxStream_Control(df->s, GX_STREAM_CTRL_SYNC_DATA, NULL);
		}

		if ((read_size <= 0) || (write_size == 0))
			GxCore_ThreadDelay(10);
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
}

int dumpfilter_run(GxMediaFilter* filter)
{
	GxDumpFilter* df = GXDUMPFILTER(filter);
	GxMediaFilter* mf = GXMEDIAFILTER(filter);
	GxDumpFilterClass* cls = GetDumpFilterClassFromObject(mf);

	if (cls->type != DUMPFILTER_FM) {
		df->inpin = GxMediaFilter_FindPin(filter, GX_DUMPFILTER_PIN_NAME_IN);
		if (df->inpin == NULL || df->inpin->source == NULL || df->inpin->source->fifo == NULL) {
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			return -1;
		}
	}

	filter->status = GX_MFT_STATE_RUNNING;

	GxCore_ThreadCreate("dumpfilter_thread", &df->thread, dumpfilter_thread,
			filter, 12*1024, GXOS_DEFAULT_PRIORITY-1);

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	return 0;
}

GxDumpFilter* GxDumpFilter_Open(const char* dsturl, GxDumpFilterType type)
{
	GxDumpFilter* df;
	GxDumpFilterClass* cls = dumpfilter_get_class_from_type(type);
	if(cls == NULL)
		return NULL;

	df = GxObjectNew(NULL, cls);
	if (df) {
		if (cls->type != DUMPFILTER_FM)
			GxPin_Create(GX_DUMPFILTER_PIN_NAME_IN, df, GX_PINDIR_INPUT, 0, GX_PINFLAG_NO_PTS_FIFO);

		df->s = GxStream_Open(dsturl, GX_STREAM_TRUNC, 0);
		if (df->s == NULL) {
			gxlogf("[Player]: Dump File Open Failed !!!\n");
			goto errout;
		}

		df->write_full = 0;
		df->cap = -1;
#ifdef DEBUG_DUMPFILTER_WRITE_SPEED
		df->idx = 0;
		df->cnt = 0;
#endif
		GxStream_Control(df->s, GX_STREAM_CTRL_GET_CAP_SIZE, &df->cap);
		if (df->cap != -1) {
			int64_t ressize = 0;
			int keepsize = 0;

			GxPlayer_SystemGet(PSYS_PVR_RESERVE_SIZEMB, &keepsize);
			ressize = keepsize;
			ressize <<= 20;
			if (df->cap <= ressize) {
				gxlogf("[Player]: Dump Disk Full !!!\n");
				goto errout;
			} else {
				df->cap -= ressize;
				gxlogf("[Player]: Dump Disk Free Size = %lld Byte\n",df->cap);
			}
		}

		/* init ticktime */
		df->pause_timems = 0;
		df->empty_timems = 0;
		df->start_ticktime.seconds = 0;
		df->start_ticktime.microsecs = 0;
		df->pause_ticktime.seconds = 0;
		df->pause_ticktime.microsecs = 0;

		if (cls->open && cls->open(df) != 0)
			goto errout;
	}

	return df;

errout:
	gxlogf("[Player]: Dump Page malloc fail !!!\n");
	GxDumpFilter_Close(df);
	return NULL;
}

int GxDumpFilter_Read(GxDumpFilter *df, uint8_t *buffer, int len)
{
	int ret = -1;
	GxMediaFilter* mf = GXMEDIAFILTER(df);

	if (df == NULL || mf == NULL || df->s == NULL)
		return -1;

	if (mf->status == GX_MFT_STATE_RUNNING || mf->status == GX_MFT_STATE_PAUSED) {
		if (df->r == NULL)
			df->r = GxStream_Open(df->s->url, GX_STREAM_TRUNC, 0);
		ret = GxStream_Read(df->r, buffer, len);
	}

	return ret;
}

void GxDumpFilter_Close(GxDumpFilter* df)
{
	if (df) {
		GxDumpFilterClass* cls = GetDumpFilterClassFromObject(df);

		dumpfilter_stop(GXMEDIAFILTER(df));

		if (df->r)
			GxStream_Close(df->r);

		if (cls && cls->close) {
			cls->close(df);
		}

		GxObjectDestroy((void* )df);
	}
}

int GxDumpFilter_Control(GxDumpFilter* df, int cmd, void* args)
{
	if (df) {
		GxDumpFilterClass* cls = GetDumpFilterClassFromObject(df);

		if (cls && cls->control && cls->control(df, cmd, args) == 0)
			return 0;

		switch (cmd) {
		case GX_DUMPER_GET_DURATION:
			*(uint64_t*)args = df->timems[0];
			break;
		case GX_DUMPER_GET_FILESIZE:
			GxStream_Control(df->s, GX_STREAM_CTRL_GET_SIZE, args);
			break;
		case GX_DUMPER_GET_VOLINFO:
			GxStream_Control(df->s, GX_STREAM_CTRL_GET_VOL_INFO, args);
			break;
		case GX_DUMPER_SET_PVR_CONTROL:
			if (df->s->ispvr)
				return GxStream_Control(df->s, GX_STREAM_CTRL_PVR_SET_CONTROL, args);
			break;
		case GX_DUMPER_GET_PVR_CONTROL:
			if (df->s->ispvr)
				return GxStream_Control(df->s, GX_STREAM_CTRL_PVR_GET_CONTROL, args);
			break;
		default:
			return -1;
		}
		return 0;
	}

	return -1;
}

GxDumpFilterClass gx_dumpfilter_base = {
	._inherit = {
		._inherit = {
			.name    = "DumpFilterBase",
			.parent  = &gx_mediafilter_base,
			.size    = sizeof(GxDumpFilter),
			.create  = dumpfilter_create,
			.release = dumpfilter_release,
		},
		.run   = dumpfilter_run,
		.pause = dumpfilter_pause,
		.resume= dumpfilter_resume,
		.stop  = dumpfilter_stop,
	}
	,
};

