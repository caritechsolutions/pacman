#ifndef __AD_DEMUXER_H__
#define __AD_DEMUXER_H__

typedef struct {
	int chip;
	int dev;
	int module;
	int progid;
	int dmxid;
	int pid;
	HwDemuxConf conf;
	GxFifo *fifo;
	int running;
	int default_source;
} AdDemuxer;

extern AdDemuxer* GxAdDemux_Open(int pid, int source);
extern int  GxAdDemux_Config(AdDemuxer *ad_dmx, GxFifo *fifo, int bind_descr);
extern int  GxAdDemux_Run   (AdDemuxer *ad_dmx);
extern int  GxAdDemux_Stop  (AdDemuxer *ad_dmx);
extern void GxAdDemux_Close (AdDemuxer *ad_dmx);

#endif
