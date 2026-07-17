#ifndef __GX_MEDIA_MODULE_H__
#define __GX_MEDIA_MODULE_H__

#include "gxmedia_api.h"
#include "av/gxav_module_property.h"

typedef struct {
#define DEMUX_TSBUF_SIZE  (2*188*1024)
	int valid;
	int mdmx_dev;
	int mdmx_mod;
	int idvr_mod;
	int odvr_mod;
	int ts_fifo;
	int esa_fifo;
	int esv_fifo;
	int running;
	unsigned int   ts_fill_size;
	unsigned char* ts_buffer;
	unsigned char* esa_buffer;
	unsigned char* esv_buffer;
	GxDemuxProperty_Slot esa_slot;
	GxDemuxProperty_Slot esv_slot;
	GxMediaProtectMode  encrpt_mode;
	GxMediaAVStreamType astream_type;
} GxMediaDemux;

typedef struct {
	int valid;
	int stc_dev;
	int stc_mod;
	int running;
	int sync_mode;
	int freq_HZ;
} GxMediaStc;

typedef struct {
#define AUDIO_ESA_SIZE (128*1024)
#define AUDIO_PCM_SIZE (512*1024)
	int valid;
	int dec_dev;
	int dec_mod;
	int out_dev;
	int out_mod;
	int es_fifo;
	int pcm_fifo;
	int running;
	unsigned char *es_buffer;
	unsigned char *pcm_buffer;
	unsigned char *work_mem;
	unsigned int  work_size;
	GxMediaDemux  *mdemux;
	GxMediaAudioPara params;
	int dsp_modid;
	int dsp_outdata;
} GxMediaAudio;

typedef struct {
#define VIDEO_ESV_SIZE (0x200000)
#define VIDEO_MAX_RUN_INFO (2)
	int valid;
	int dec_dev;
	int out_dev;
	int dec_mod;
	int out_mod;
	int vpu_mod;
	int es_fifo;
	int running;
	int waiting;
	int waitthread;
	int mod_id;
	unsigned char* es_buffer;
	unsigned char* work_mem;
	GxMediaDemux  *mdemux;
	int                        run_info_id;
	GxVideoDecoderProperty_Run run_info[VIDEO_MAX_RUN_INFO];//use to change video size
	GxMediaVideoPara params;
} GxMediaVideo;

typedef struct {
	uint8_t          *buf;
	int               len;
	void             *priv;
	GxMediaDecryptCB  decrypt_cb;
} GxMediaDemuxInject;

typedef struct {
	int valid;
	GxMediaAudio *maudio;
	GxMediaVideo *mvideo;
	GxMediaDemux *mdemux;
	GxMediaStc   *mstc;
	GxMediaSourceType sourcetype;
} GxMediaModule;

GxMediaModule* GxMedia_ModuleOpen(GxMediaModuleMod mod, GxMediaModuleModPara mpara);
int GxMedia_ModuleClose(GxMediaModule* module);
int GxMedia_ModuleConfig(GxMediaModule* module, GxMediaModulePara params);
int GxMedia_ModuleStart (GxMediaModule* module);
int GxMedia_ModuleStop  (GxMediaModule* module, int freeze);
int GxMedia_ModulePause (GxMediaModule* module);
int GxMedia_ModuleResume(GxMediaModule* module);
int GxMedia_ModuleInjectData(GxMediaModule* module, GxMediaModuleInject inject);
int GxMedia_ModuleSpeed   (GxMediaModule* module, float speed);
int GxMedia_ModuleGetInfo (GxMediaModule* module, GxMediaModuleInfo  *info);
int GxMedia_ModuleGetState(GxMediaModule* module, GxMediaModuleState *state);
int GxMedia_ModuleFinished(GxMediaModule* module);

GxMediaAudio* GxMedia_AudioOpen(GxMediaAudioMod mod);
int  GxMedia_AudioBindDemux  (GxMediaAudio* media_audio, GxMediaDemux *media_demux);
int  GxMedia_AudioClose      (GxMediaAudio* media_audio);
int  GxMedia_AudioConfig     (GxMediaAudio* media_audio, GxMediaAudioPara param);
int  GxMedia_AudioStart      (GxMediaAudio* media_audio);
int  GxMedia_AudioStop       (GxMediaAudio* media_audio);
int  GxMedia_AudioPause      (GxMediaAudio* media_audio);
int  GxMedia_AudioResume     (GxMediaAudio* media_audio);
int  GxMedia_AudioInjectData (GxMediaAudio* media_audio, GxMediaSourceType type, uint8_t* buffer, int len, int pts);
int  GxMedia_AudioFinished   (GxMediaAudio* media_audio);
int  GxMedia_AudioEos        (GxMediaAudio* media_audio);
int  GxMedia_AudioGetFifoInfo(GxMediaAudio* media_audio, GxMediaModuleAudioFifo *info);
int  GxMedia_AudioGetDecState(GxMediaAudio* media_audio, GxMediaDecState *state, GxMediaDecErrCode *err_code);
int  GxMedia_AudioGetPts     (GxMediaAudio* media_audio, int64_t* apts);
int  GxMedia_AudioSpeed      (GxMediaAudio* media_audio, float speed);

GxMediaVideo* GxMedia_VideoOpen(GxMediaVideoMod mod);
int  GxMedia_VideoBindDemux  (GxMediaVideo* media_video, GxMediaDemux *media_demux);
int  GxMedia_VideoClose      (GxMediaVideo* media_video);
int  GxMedia_VideoConfig     (GxMediaVideo* media_video, GxMediaVideoPara param);
int  GxMedia_VideoSetWindow  (GxMediaVideo* media_video, GxMediaVideoWindow win);
int  GxMedia_VideoStart      (GxMediaVideo* media_video);
int  GxMedia_VideoStop       (GxMediaVideo* media_video, int freeze);
int  GxMedia_VideoPause      (GxMediaVideo* media_video);
int  GxMedia_VideoResume     (GxMediaVideo* media_video);
int  GxMedia_VideoInjectData (GxMediaVideo* media_video, GxMediaSourceType type, uint8_t* buffer, int len, int pts);
int  GxMedia_VideoGetFifoInfo(GxMediaVideo* media_video, GxMediaModuleVideoFifo *info);
int  GxMedia_VideoFinished   (GxMediaVideo* media_video);
int  GxMedia_VideoEos        (GxMediaVideo* media_video);
int  GxMedia_VideoGetDecState(GxMediaVideo* media_video, GxMediaDecState *state, GxMediaDecErrCode *err_code);
int  GxMedia_VideoGetFrameInfo(GxMediaVideo* media_video, GxMediaVideoFrameInfo *info);
int  GxMedia_VideoGetPts(GxMediaVideo* media_video, int64_t* vpts);
int  GxMedia_VideoGetHash(GxMediaVideo* media_video, GxMediaVideoFrameHash *hash);
int  GxMedia_VideoSpeed  (GxMediaVideo* media_video, float speed);

GxMediaDemux* GxMedia_DemuxOpen(GxMediaDemuxMod mod, GxMediaModuleModPara mpara);
int GxMedia_DemuxConfig(GxMediaDemux* mdemux, int Tsid, int AudPid, int VidPid, int PcrPid);
int GxMedia_DemuxStart (GxMediaDemux* mdemux);
int GxMedia_DemuxStop  (GxMediaDemux* mdemux);
int GxMedia_DemuxClose (GxMediaDemux* mdemux);
int GxMedia_DemuxInjectData(GxMediaDemux* mdemux, GxMediaDemuxInject *inject);
int GxMedia_DemuxGetAfifo(GxMediaDemux* mdemux);
int GxMedia_DemuxGetVfifo(GxMediaDemux* mdemux);
int GxMedia_DemuxGetFifoInfo(GxMediaDemux* mdemux, GxMediaModuleDemuxFifo *info);

GxMediaStc* GxMedia_StcOpen(GxMediaStcMod mod);
int GxMedia_StcConfig(GxMediaStc* mstc, int freq_HZ, int sync_mode);
int GxMedia_StcStart (GxMediaStc* mstc);
int GxMedia_StcStop  (GxMediaStc* mstc);
int GxMedia_StcPause (GxMediaStc* mstc);
int GxMedia_StcResume(GxMediaStc* mstc);
int GxMedia_StcClose (GxMediaStc* mstc);
int GxMedia_StcGetTime(GxMediaStc* mstc, int64_t* stc);
int GxMedia_StcSpeed  (GxMediaStc* mstc, float speed);

#endif
