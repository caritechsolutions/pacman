#include "gxcore.h"
#include "gxavdev.h"
#include "gxfrontend_module.h"

extern void FE_INIT_FUNC(int,char *);

int dev;
int demux;

void fe_init(void)
{
	dev = GxAvdev_CreateDevice(0);
	demux = GxAvdev_OpenModule(dev, GXAV_MOD_DEMUX,0);
}

extern int dvbsource_set_tp(int dev, int demux, char* url);
extern void GxUrl_GetItem(const char* url, const char* item, int32_t *retVal);
extern void cyg_time_delay_us(unsigned int delay);

int check_lock(char *url)
{
	int ret;

	if(strncasecmp(url,"dvbts",5) == 0){
		int tsid;
		GxDemuxProperty_ConfigDemux cfg_dmx = {
			.stream_mode = 0,
			.ts_select   = 0,
			.source      = 0,
			.time_gate        = 0xf,
			.sync_lock_gate   = 0x3,
			.sync_loss_gate   = 0x3,
			.byt_cnt_err_gate = 0x3
		};

		GxUrl_GetItem(url,"tsid", &tsid);
		cfg_dmx.source = tsid;

		GxAVSetProperty(dev, demux, GxDemuxPropertyID_Config,
				&cfg_dmx, sizeof(GxDemuxProperty_ConfigDemux));

		GxDemuxProperty_TSLockQuery ts_lock_status = {TS_SYNC_UNLOCKED};

		ret = GxAVGetProperty(dev, demux, GxDemuxPropertyID_TSLockQuery,
				&ts_lock_status, sizeof(GxDemuxProperty_TSLockQuery));
		if(ret < 0)
			return -1;

		return (ts_lock_status.ts_lock==TS_SYNC_LOCKED?0:-1);

	}else{
		ret = 0;
		int tsid;

		GxDemuxProperty_ConfigDemux cfg_dmx = {
			.stream_mode = 0,
			.ts_select   = 0,
			.source      = 0,
			.time_gate        = 0xf,
			.sync_lock_gate   = 0x3,
			.sync_loss_gate   = 0x3,
			.byt_cnt_err_gate = 0x3
		};

		GxUrl_GetItem(url,"tsid", &tsid);
		cfg_dmx.source = tsid;

		GxAVSetProperty(dev, demux, GxDemuxPropertyID_Config,
				&cfg_dmx, sizeof(GxDemuxProperty_ConfigDemux));

		dvbsource_set_tp(dev,demux,url);

		//cyg_time_delay_us(150*1000);
		GxCore_ThreadDelay(1000);
		if (GxFrontend_QueryFrontendStatus(0)==1){
			ret |= (1<<0);
			printf("===============demod lock!!!!\n");
		}else{
			printf("demod unlock!!!!\n");
		}

		if( GxFrontend_QueryStatus(0) ==1){
			ret |= (1<<1);
			printf("---------------demux lock!!!!\n");
		}else{
			printf("demux unlock!!!!\n");
		}
		return (ret == 0x3?0:-1);
	}
}
