#ifndef __DEMUX_HWTS_H__
#define __DEMUX_NWTS_H__

#include "gx_demux.h"
#include "stheader.h"
#include "hw_demux.h"
#include "demux_ts.h"
#include "recfs.h"

#define ENTRY_BLOCK_SIZE  ((int)sizeof(PlayerRecIndexEntry))
#define ENTRY_BLOCK_NUM   (512)

#define HW_TSR_BUFFER_SIZE        (GX_PINSIZE_TSA)
#define HW_TS_BLOCK_SIZE          (64*188*8)
#define HW_TS_MAX_CHECK_SIZE      (64*1024) //dumpfilter time packet interval/2
#define HW_TS_PACKET_SIZE         (188)

#define HW_TS_BACKWARD            (1)
#define HW_TS_RELATIVE            (2)

typedef struct {
	GxFifo *infifo;
	GxPin *apin;
	GxPin *vpin;
	unsigned char buffer[HW_TS_BLOCK_SIZE];

	struct record_file *fd;
	PlayerRecIndexEntry entries[ENTRY_BLOCK_NUM];
	int32_t nb_entries;
	off_t entry_pos;

	int32_t dev;
	int32_t module;
	int32_t progid;
	int32_t abort;
	int32_t paused;
	int32_t config;
	int32_t speed;

	int32_t mutex;

	off_t    duration_file_size;
	int64_t  duration;

	int     default_source;
	void    *timer;
	int64_t  timer_basems;
	int      running;
	AdDemuxer *ad_dmx;
	int      bind_descr;
	int      tsr_security;
	int      tsr_ptr_en;
	int      block_size;
	int      decrypt_enable;
	int      pause_fill_data;
	int64_t  fill_cur_timems;
	int      block_pos;
	int      kBps;
	GxDemuxerPrivateTSInfo tsinfo;
	PlayerPVRConfig pvr_config;
	unsigned int    process_flag;
}DemuxHwTSPriv;

#endif
