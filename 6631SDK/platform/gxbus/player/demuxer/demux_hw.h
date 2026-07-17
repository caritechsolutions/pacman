#ifndef __DEMUX_HW_H__
#define __DEMUX_HW_H__

#include "hw_demux.h"
#include "gx_demux.h"
#include "ad_demux.h"

typedef struct {
	int chip;
	int dev;
	int module;
	int progid;
	int index;
	int tsid;
	HwDemuxConf conf;
	GxDemuxerPrivateTSInfo ts_info;
	AdDemuxer   *ad_dmx;
}DemuxHwPriv;

#endif
