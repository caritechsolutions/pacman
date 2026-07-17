#include "gxmedia_api.h"
#include "gxmedia_module.h"
#include "gxmedia_common.h"
#include "gx_avflow.h"

#define CHECK_MEDIA_API_HANDLE(h) if (h == -1) return GX_MEDIAAPI_ERROR;

handle_t GxMediaApi_ModuleOpen(GxMediaSourceType type)
{
	GxMediaModule* module = NULL;
	GxMediaModuleMod     mod;
	GxMediaModuleModPara mpara;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	memset(&mod, 0, sizeof(GxMediaModuleMod));
	mpara.source_type   = type;
	mpara.protect_mode  = GXMEDIA_OUTPUT_NORMAL;
	mpara.a_stream_type = GXMEDIA_AVSTREAM_ES;
	module = GxMedia_ModuleOpen(mod, mpara);
	if (module == NULL) return -1;

	return (handle_t)module;
}

handle_t GxMediaApi_ModuleOpen2(GxMediaSourceType type, GxMediaProtectMode mode)
{
	GxMediaModule* module = NULL;
	GxMediaModuleMod     mod;
	GxMediaModuleModPara mpara;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	memset(&mod, 0, sizeof(GxMediaModuleMod));
	mpara.source_type   = type;
	mpara.protect_mode  = mode;
	mpara.a_stream_type = GXMEDIA_AVSTREAM_ES;
	module = GxMedia_ModuleOpen(mod, mpara);
	if (module == NULL) return -1;

	return (handle_t)module;
}

handle_t GxMediaApi_ModuleOpen3(GxMediaModuleMod mod, GxMediaModuleModPara mpara)
{
	GxMediaModule* module = NULL;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	module = GxMedia_ModuleOpen(mod, mpara);
	if (module == NULL) return -1;

	return (handle_t)module;
}

status_t GxMediaApi_ModuleClose(handle_t handle)
{
	GxMediaModule* module = (GxMediaModule*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_ModuleClose(module);
}

status_t GxMediaApi_ModuleConfig(handle_t handle, GxMediaModulePara para)
{
	GxMediaModule* module = (GxMediaModule*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_ModuleConfig(module, para);
}

status_t GxMediaApi_ModuleStart(handle_t handle)
{
	GxMediaModule* module = (GxMediaModule*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_ModuleStart(module);
}

status_t GxMediaApi_ModuleStop(handle_t handle, int freeze)
{
	GxMediaModule* module = (GxMediaModule*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_ModuleStop(module, freeze);
}

status_t GxMediaApi_ModulePause(handle_t handle)
{
	GxMediaModule* module = (GxMediaModule*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_ModulePause(module);
}

status_t GxMediaApi_ModuleResume(handle_t handle)
{
	GxMediaModule* module = (GxMediaModule*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_ModuleResume(module);
}

status_t GxMediaApi_ModuleInfo(handle_t handle, GxMediaModuleInfo *info)
{
	GxMediaModule* module = (GxMediaModule*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);
	if(info == NULL) return GX_MEDIAAPI_ERROR;

	return GxMedia_ModuleGetInfo(module, info);
}

status_t GxMediaApi_ModuleState(handle_t handle, GxMediaModuleState *state)
{
	GxMediaModule* module = (GxMediaModule*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);
	if(state == NULL) return GX_MEDIAAPI_ERROR;

	return GxMedia_ModuleGetState(module, state);
}

status_t GxMediaApi_ModuleFinished(handle_t handle)
{
	GxMediaModule* module = (GxMediaModule*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_ModuleFinished(module);
}

int GxMediaApi_ModuleInjectData(handle_t handle, GxMediaModuleInject inject)
{
	GxMediaModule* module = (GxMediaModule*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	if (handle == -1) return -1;

	return GxMedia_ModuleInjectData(module, inject);
}

int GxMediaApi_ModuleSpeed(handle_t handle, float speed)
{
	GxMediaModule* module = (GxMediaModule*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_ModuleSpeed(module, speed);
}

handle_t GxMediaApi_AudioOpen(handle_t demux_handle)
{
	GxMediaAudio* audio_module = NULL;
	GxMediaDemux* demux_module = (GxMediaDemux*)demux_handle;
	GxMediaAudioMod mod;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	memset(&mod, 0, sizeof(GxMediaAudioMod));
	audio_module = GxMedia_AudioOpen(mod);

	if (audio_module == NULL) return -1;

	GxMedia_AudioBindDemux(audio_module, demux_module);

	return (handle_t)audio_module;
}

handle_t GxMediaApi_AudioOpen3(GxMediaAudioMod mod)
{
	GxMediaAudio* audio_module = NULL;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	audio_module = GxMedia_AudioOpen(mod);

	if (audio_module == NULL) return -1;

	return (handle_t)audio_module;
}

status_t GxMediaApi_AudioBindDemux(handle_t handle, handle_t demux_handle)
{
	GxMediaAudio* audio_module = (GxMediaAudio*)handle;
	GxMediaDemux* demux_module = (GxMediaDemux*)demux_handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_AudioBindDemux(audio_module, demux_module);
}

status_t GxMediaApi_AudioClose(handle_t handle)
{
	GxMediaAudio* audio_module = (GxMediaAudio*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_AudioClose(audio_module);
}

status_t GxMediaApi_AudioConfig(handle_t handle, GxMediaAudioPara param)
{
	GxMediaAudio* audio_module = (GxMediaAudio*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_AudioConfig(audio_module, param);
}

status_t GxMediaApi_AudioStart(handle_t handle)
{
	GxMediaAudio* audio_module = (GxMediaAudio*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_AudioStart(audio_module);
}

status_t GxMediaApi_AudioStop(handle_t handle)
{
	GxMediaAudio* audio_module = (GxMediaAudio*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_AudioStop(audio_module);
}

status_t GxMediaApi_AudioPause(handle_t handle)
{
	GxMediaAudio* audio_module = (GxMediaAudio*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_AudioPause(audio_module);
}

status_t GxMediaApi_AudioResume(handle_t handle)
{
	GxMediaAudio* audio_module = (GxMediaAudio*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_AudioResume(audio_module);
}

status_t GxMediaApi_AudioFinished(handle_t handle)
{
	GxMediaAudio* audio_module = (GxMediaAudio*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_AudioFinished(audio_module);
}

status_t GxMediaApi_AudioEos(handle_t handle)
{
	GxMediaAudio* audio_module = (GxMediaAudio*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_AudioEos(audio_module);
}

status_t GxMediaApi_AudioGetFifoInfo(handle_t handle, GxMediaModuleAudioFifo *info)
{
	GxMediaAudio* audio_module = (GxMediaAudio*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_AudioGetFifoInfo(audio_module, info);
}

status_t GxMediaApi_AudioGetDecState(handle_t handle, GxMediaModuleDecState *state)
{
	GxMediaAudio* audio_module = (GxMediaAudio*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	if (state == NULL)
		return GX_MEDIAAPI_ERROR;

	return GxMedia_AudioGetDecState(audio_module, &(state->state), &(state->err_code));
}

status_t GxMediaApi_AudioGetPts(handle_t handle, int64_t* apts)
{
	GxMediaAudio* audio_module = (GxMediaAudio*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_AudioGetPts(audio_module, apts);
}

int GxMediaApi_AudioInjectData(handle_t handle, GxMediaSourceType type, uint8_t* buffer, int len, int pts)
{
	GxMediaAudio* audio_module = (GxMediaAudio*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	if (handle == -1) return -1;

	return GxMedia_AudioInjectData(audio_module, type, buffer, len, pts);
}

status_t GxMediaApi_AudioSpeed(handle_t handle, float speed)
{
	GxMediaAudio* audio_module = (GxMediaAudio*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_AudioSpeed(audio_module, speed);
}

handle_t GxMediaApi_VideoOpen(handle_t demux_handle)
{
	GxMediaVideo* video_module = NULL;
	GxMediaDemux* demux_module = (GxMediaDemux*)demux_handle;
	GxMediaVideoMod mod;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	memset(&mod, 0, sizeof(GxMediaVideoMod));
	video_module = GxMedia_VideoOpen(mod);

	if (video_module == NULL) return -1;

	GxMedia_VideoBindDemux(video_module, demux_module);

	return (handle_t)video_module;
}

handle_t GxMediaApi_VideoOpen3(GxMediaVideoMod mod)
{
	GxMediaVideo* video_module = NULL;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	video_module = GxMedia_VideoOpen(mod);

	if (video_module == NULL) return -1;

	return (handle_t)video_module;
}

status_t GxMediaApi_VideoBindDemux(handle_t handle, handle_t demux_handle)
{
	GxMediaVideo* video_module = (GxMediaVideo*)handle;
	GxMediaDemux* demux_module = (GxMediaDemux*)demux_handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_VideoBindDemux(video_module, demux_module);
}

status_t GxMediaApi_VideoClose(handle_t handle)
{
	GxMediaVideo* video_module = (GxMediaVideo*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_VideoClose(video_module);
}

status_t GxMediaApi_VideoConfig(handle_t handle, GxMediaVideoPara param)
{
	GxMediaVideo* video_module = (GxMediaVideo*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_VideoConfig(video_module, param);
}

status_t GxMediaApi_VideoSetWindow(handle_t handle, GxMediaVideoWindow win)
{
	GxMediaVideo* video_module = (GxMediaVideo*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_VideoSetWindow(video_module, win);
}

status_t GxMediaApi_VideoStart(handle_t handle)
{
	GxMediaVideo* video_module = (GxMediaVideo*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_VideoStart(video_module);
}

status_t GxMediaApi_VideoStop(handle_t handle, int freeze)
{
	GxMediaVideo* video_module = (GxMediaVideo*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_VideoStop(video_module, freeze);
}

status_t GxMediaApi_VideoPause(handle_t handle)
{
	GxMediaVideo* video_module = (GxMediaVideo*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_VideoPause(video_module);
}

status_t GxMediaApi_VideoResume(handle_t handle)
{
	GxMediaVideo* video_module = (GxMediaVideo*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_VideoResume(video_module);
}

status_t GxMediaApi_VideoGetFifoInfo(handle_t handle, GxMediaModuleVideoFifo *info)
{
	GxMediaVideo* video_module = (GxMediaVideo*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_VideoGetFifoInfo(video_module, info);
}

status_t GxMediaApi_VideoGetDecState(handle_t handle, GxMediaModuleDecState *state)
{
	GxMediaVideo* video_module = (GxMediaVideo*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	if (state == NULL)
		return GX_MEDIAAPI_ERROR;

	return GxMedia_VideoGetDecState(video_module, &(state->state), &(state->err_code));
}

status_t GxMediaApi_VideoGetFrameInfo(handle_t handle, GxMediaVideoFrameInfo *info)
{
	GxMediaVideo* video_module = (GxMediaVideo*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	if (info == NULL)
		return GX_MEDIAAPI_ERROR;

	return GxMedia_VideoGetFrameInfo(video_module, info);
}

status_t GxMediaApi_VideoGetPts(handle_t handle, int64_t* vpts)
{
	GxMediaVideo* video_module = (GxMediaVideo*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_VideoGetPts(video_module, vpts);
}

status_t GxMediaApi_VideoGetHash(handle_t handle, GxMediaVideoFrameHash *hash)
{
	GxMediaVideo* video_module = (GxMediaVideo*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_VideoGetHash(video_module, hash);
}

status_t GxMediaApi_VideoSpeed(handle_t handle, float speed)
{
	GxMediaVideo* video_module = (GxMediaVideo*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_VideoSpeed(video_module, speed);
}

status_t GxMediaApi_VideoFinished(handle_t handle)
{
	GxMediaVideo* video_module = (GxMediaVideo*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_VideoFinished(video_module);
}

status_t GxMediaApi_VideoEos(handle_t handle)
{
	GxMediaVideo* video_module = (GxMediaVideo*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_VideoEos(video_module);
}

int  GxMediaApi_VideoInjectData(handle_t handle, GxMediaSourceType type, uint8_t* buffer, int len, int pts)
{
	GxMediaVideo* video_module = (GxMediaVideo*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	if (handle == -1) return -1;

	return GxMedia_VideoInjectData(video_module, type, buffer, len, pts);
}

handle_t GxMediaApi_DemuxOpen(GxMediaSourceType SourceType)
{
	GxMediaDemux* demux_module = NULL;
	GxMediaDemuxMod mod;
	GxMediaModuleModPara mpara;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	memset(&mod, 0, sizeof(GxMediaDemuxMod));
	mpara.source_type   = SourceType;
	mpara.protect_mode  = GXMEDIA_OUTPUT_NORMAL;
	mpara.a_stream_type = GXMEDIA_AVSTREAM_ES;
	demux_module = GxMedia_DemuxOpen(mod, mpara);
	if (demux_module == NULL) return -1;

	return (handle_t)demux_module;
}

handle_t GxMediaApi_DemuxOpen3(GxMediaDemuxMod mod, GxMediaSourceType SourceType)
{
	GxMediaDemux* demux_module = NULL;
	GxMediaModuleModPara mpara;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	mpara.source_type   = SourceType;
	mpara.protect_mode  = GXMEDIA_OUTPUT_NORMAL;
	mpara.a_stream_type = GXMEDIA_AVSTREAM_ES;
	demux_module = GxMedia_DemuxOpen(mod, mpara);
	if (demux_module == NULL) return -1;

	return (handle_t)demux_module;
}

status_t GxMediaApi_DemuxClose(handle_t handle)
{
	GxMediaDemux* demux_module = (GxMediaDemux*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_DemuxClose(demux_module);
}

status_t GxMediaApi_DemuxConfig(handle_t handle, int Tsid, int AudPid, int VidPid, int PcrPid)
{
	GxMediaDemux* demux_module = (GxMediaDemux*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_DemuxConfig(demux_module, Tsid, AudPid, VidPid, PcrPid);
}

status_t GxMediaApi_DemuxStart(handle_t handle)
{
	GxMediaDemux* demux_module = (GxMediaDemux*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_DemuxStart(demux_module);
}

status_t GxMediaApi_DemuxStop(handle_t handle)
{
	GxMediaDemux* demux_module = (GxMediaDemux*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_DemuxStop(demux_module);
}

status_t GxMediaApi_DemuxGetFifoInfo(handle_t handle, GxMediaModuleDemuxFifo *info)
{
	GxMediaDemux* demux_module = (GxMediaDemux*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_DemuxGetFifoInfo(demux_module, info);
}

int GxMediaApi_DemuxInjectData(handle_t handle, uint8_t* buffer, int len)
{
	GxMediaDemux* demux_module = (GxMediaDemux*)handle;
	GxMediaDemuxInject dmx_inject;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	if (handle == -1) return -1;

	dmx_inject.buf        = buffer;
	dmx_inject.len        = len;
	dmx_inject.priv       = NULL;
	dmx_inject.decrypt_cb = NULL;
	return GxMedia_DemuxInjectData(demux_module, &dmx_inject);
}

handle_t GxMediaApi_StcOpen(void)
{
	GxMediaStc* stc_module = NULL;
	GxMediaStcMod mod;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	memset(&mod, 0, sizeof(GxMediaStcMod));
	stc_module = GxMedia_StcOpen(mod);
	if (stc_module == NULL) return -1;

	return (handle_t)stc_module;
}

handle_t GxMediaApi_StcOpen3(GxMediaStcMod mod)
{
	GxMediaStc* stc_module = NULL;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	stc_module = GxMedia_StcOpen(mod);
	if (stc_module == NULL) return -1;

	return (handle_t)stc_module;
}

status_t GxMediaApi_StcConfig(handle_t handle, int freq_HZ, int sync_mode)
{
	GxMediaStc* stc_module = (GxMediaStc*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_StcConfig(stc_module, freq_HZ, sync_mode);
}

status_t GxMediaApi_StcStart(handle_t handle)
{
	GxMediaStc* stc_module = (GxMediaStc*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_StcStart(stc_module);
}

status_t GxMediaApi_StcStop(handle_t handle)
{
	GxMediaStc* stc_module = (GxMediaStc*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_StcStop(stc_module);
}

status_t GxMediaApi_StcPause(handle_t handle)
{
	GxMediaStc* stc_module = (GxMediaStc*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_StcPause(stc_module);
}

status_t GxMediaApi_StcResume(handle_t handle)
{
	GxMediaStc* stc_module = (GxMediaStc*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_StcResume(stc_module);
}

status_t GxMediaApi_StcSpeed(handle_t handle, float speed)
{
	GxMediaStc* stc_module = (GxMediaStc*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_StcSpeed(stc_module, speed);
}

status_t GxMediaApi_StcClose(handle_t handle)
{
	GxMediaStc* stc_module = (GxMediaStc*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_StcClose(stc_module);
}

status_t GxMediaApi_StcGetTime(handle_t handle, int64_t* stc)
{
	GxMediaStc* stc_module = (GxMediaStc*)handle;

	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	CHECK_MEDIA_API_HANDLE(handle);

	return GxMedia_StcGetTime(stc_module, stc);
}

status_t GxMediaApi_Init(void)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	GxPlayer_SystemInit();

	GxPlayer_HeapInit();

	return 0;
}



