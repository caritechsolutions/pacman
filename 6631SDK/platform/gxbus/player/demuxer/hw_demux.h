#ifndef __HW_DEMUX_H__
#define __HW_DEMUX_H__

#include "gx_common.h"
#include "gx_mediafilter.h"
#include "gx_system.h"
#include "gx_demux.h"

#define PES_READ_SIZE (64*1024+6)

#define HW_DEMUX_MOD_MAX       (16)
#define HW_DEMUX_PROG_MAX      (32)
#define HW_DEMUX_SLOT_MAX      (64)
#define HW_DEMUX_MUX_MAX       (1)
#define HW_DEMUX_FILTER_MAX    (64)

#define HW_DEMUX_FUNC_DEMUX    (0)
#define HW_DEMUX_FUNC_MUXER    (1)

#define HW_DEMUX_VIDEO         (1)
#define HW_DEMUX_AUDIO         (2)
#define HW_DEMUX_PES_AUDIO     (4)
#define HW_DEMUX_PES_SUBT      (8)
#define HW_DEMUX_PSI_SUBT      (16)

#define VAILD_PID(pid)          ((pid>0)&&(pid<0x1fff))
#define CHECK_PID(pid)          if((pid>=0x1fff)) return GX_PLAYER_ERROR;
#define CHECK_MODID(index)      if(index <0 || index>=HW_DEMUX_MOD_MAX) return GX_PLAYER_ERROR;
#define CHECK_PROGID(index)     if(index <0 || index>=HW_DEMUX_PROG_MAX) return GX_PLAYER_ERROR;
#define CHECK_SLOTID(index)     if(index <0 || index>=HW_DEMUX_SLOT_MAX) return GX_PLAYER_ERROR;

#define AUDIO_CONFIG_FAILED	 (1<<0)
#define VIDEO_CONFIG_FAILED	 (1<<1)
#define MUXER_CONFIG_FAILED	 (1<<2)

typedef struct {
	int a_quiet;
	int v_quiet;
	int a_id;
	int v_id;
	int pcr_id;
	int pmt_id;
	int sync;
	int afmt;
	int vfmt;
	GxFifo* vfifo;
	GxFifo* afifo;
	GxFifo* tsfifo;
	int type;
	int progid;
	int stcadjust;
	GxDemuxerPrivateTSInfo  ts_info;

	int bind_descr;
}HwDemuxConf;

typedef struct {
	GxDemuxProperty_Slot slot;
	GxFifo* fifo;
	int usecnt;
}HwDemuxSlot;

typedef struct {
	GxDemuxProperty_Filter filter;
	GxFifo* fifo;
	int usecnt;
}HwDemuxFilter;

typedef struct {
	int slotidx;
	int filteridx;
}HwDemuxPesFilter;

typedef struct {
	GxFifo* vfifo ;
	GxFifo* afifo ;
	int vslotidx ;
	int aslotidx ;
	int used;
	int is_encrypt;
	int	ext_slot_num;
	int	ext_slot_idx[GX_DEMUXER_MAX_EXTPID_NUM];
	int muxfiltercnt;
	int outpin[HW_DEMUX_MUX_MAX];
	HwDemuxPesFilter audio;
	HwDemuxPesFilter subt;
}HwDemuxProg;

typedef struct {
	int opened;
	int opencnt;
	handle_t handle;
	handle_t stc;
	HwDemuxProg prog[HW_DEMUX_PROG_MAX];
	HwDemuxSlot slot[HW_DEMUX_SLOT_MAX];
	HwDemuxFilter filter[HW_DEMUX_FILTER_MAX];
	int slotnum;
	int filternum;
	int prognum;
}HwDemuxMod;

typedef struct {
	int dev   ;
	int opened;
	int opencnt;
	HwDemuxMod *module[HW_DEMUX_MOD_MAX] ;
}HwDemuxDev;

int GxHwDemux_Init(void);

int GxHwDemux_OpenModule(int index);

int GxHwDemux_CloseModule(int index);

int GxHwDemux_SetSource(int index,int source);

int GxHwDemux_GetSource(int index,int *source);

int GxHwDemux_GetDMXMod(int index);

int GxHwDemux_GetSlotID(int index, int pid);

int GxHwDemux_GetDVRMod(int index, int progid);

int GxHwDemux_StopStream(int index,int progid,int type);

int GxHwDemux_RunStream(int index,int progid,int type,int pid, int bind_descr);

int GxHwDemux_ResetStream(int index, int progid, int type);

int GxHwDemux_SwitchStream(int index, int progid, int type, int pid, int bind_descr);

int GxHwDemux_SaveStream(int index,int progid,int type);

int GxHwDemux_RestoreStream(int index,int progid,int type);

int GxHwDemux_Config(int index,HwDemuxConf* conf);

int GxHwDemux_Run(int index,int progid,int type);

int GxHwDemux_Stop(int index,int progid,int type);

int GxHwDemux_AllocProg(int index);

int GxHwDemux_FreeProg(int index,int progid);

int GxHwDemux_Reset(int index, int progid, int type);

int GxHwDemux_ReadFilterData(int index, int progid, int type, uint8_t* buffer, int size);

int GxHwDemux_CheckRecordOverflow(int index, int progid);
#endif
