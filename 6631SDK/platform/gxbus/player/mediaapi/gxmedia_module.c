#include "gxmedia_module.h"
#include "gxmedia_common.h"

GxMediaModule* GxMedia_ModuleOpen(GxMediaModuleMod mod, GxMediaModuleModPara mpara)
{
	GxMediaModule *module = av_mallocz(sizeof(GxMediaModule));

	if (module == NULL) return NULL;

	module->sourcetype = mpara.source_type;
	module->valid = 1;
	if (module->sourcetype == GXMEDIA_SOURCE_TS ||
			module->sourcetype == GXMEDIA_SOURCE_DVBTS) {
		module->mdemux = GxMedia_DemuxOpen(mod.demux, mpara);
		if (module->mdemux == NULL) {
			GxMediaAPI_debug("[%s %d]: demux module open failed\n", __FUNCTION__, __LINE__);
			goto open_error;
		}
	}

	module->mstc = GxMedia_StcOpen(mod.stc);
	if (module->mstc == NULL) {
		GxMediaAPI_debug("[%s %d]: stc module open failed\n", __FUNCTION__, __LINE__);
		goto open_error;
	}

	module->mvideo = GxMedia_VideoOpen(mod.video);
	if (module->mvideo == NULL) {
		GxMediaAPI_debug("[%s %d]: video module open failed\n", __FUNCTION__, __LINE__);
		goto open_error;
	}

	module->maudio = GxMedia_AudioOpen(mod.audio);
	if (module->maudio == NULL) {
		GxMediaAPI_debug("[%s %d]: audio module open failed\n", __FUNCTION__, __LINE__);
		goto open_error;
	}

	if (module->mdemux) {
		if (module->mvideo)
			GxMedia_VideoBindDemux(module->mvideo, module->mdemux);

		if (module->maudio)
			GxMedia_AudioBindDemux(module->maudio, module->mdemux);
	}

	return module;

open_error:
	GxMedia_ModuleClose(module);
	return NULL;
}

int GxMedia_ModuleClose(GxMediaModule* module)
{
	CHECK_MEDIA_MOLUDE_VALID(module);

	GxMedia_AudioClose(module->maudio);
	GxMedia_VideoClose(module->mvideo);
	GxMedia_StcClose(module->mstc);
	if (module->sourcetype == GXMEDIA_SOURCE_TS ||
			module->sourcetype == GXMEDIA_SOURCE_DVBTS)
		GxMedia_DemuxClose(module->mdemux);
	module->valid = 0;
	av_free(module);

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_ModuleConfig(GxMediaModule* module, GxMediaModulePara params)
{
	CHECK_MEDIA_MOLUDE_VALID(module);

	if (module->sourcetype == GXMEDIA_SOURCE_DVBTS)
		params.tsid = (params.tsid > 0)?params.tsid:0;
	else
		params.tsid = -1;
	params.freq_HZ   = (params.freq_HZ>0)?params.freq_HZ:45000;
	params.sync_mode = (params.sync_mode!=-1)?params.sync_mode:2;
	GxMedia_StcConfig(module->mstc, params.freq_HZ, params.sync_mode);
	GxMedia_DemuxConfig(module->mdemux, params.tsid, params.audPid, params.vidPid, params.pcrPid);
	if (params.vidPid > 0)
		GxMedia_VideoConfig(module->mvideo, params.vparam);
	else {
		GxMedia_VideoClose(module->mvideo);
		module->mvideo = NULL;
	}
	if (params.audPid > 0)
		GxMedia_AudioConfig(module->maudio, params.aparam);
	else {
		GxMedia_AudioClose(module->maudio);
		module->maudio = NULL;
	}

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_ModuleStart(GxMediaModule* module)
{
	CHECK_MEDIA_MOLUDE_VALID(module);

	GxMedia_StcStart(module->mstc);
	GxMedia_DemuxStart(module->mdemux);
	GxMedia_AudioStart(module->maudio);
	GxMedia_VideoStart(module->mvideo);

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_ModuleStop(GxMediaModule* module, int freeze)
{
	CHECK_MEDIA_MOLUDE_VALID(module);

	GxMedia_VideoStop(module->mvideo, freeze);
	GxMedia_AudioStop(module->maudio);
	GxMedia_DemuxStop(module->mdemux);
	GxMedia_StcStop(module->mstc);

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_ModulePause(GxMediaModule* module)
{
	CHECK_MEDIA_MOLUDE_VALID(module);

	GxMedia_StcPause(module->mstc);
	GxMedia_VideoPause(module->mvideo);
	GxMedia_AudioPause(module->maudio);

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_ModuleResume(GxMediaModule* module)
{
	CHECK_MEDIA_MOLUDE_VALID(module);

	GxMedia_StcResume(module->mstc);
	GxMedia_VideoResume(module->mvideo);
	GxMedia_AudioResume(module->maudio);

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_ModuleInjectData(GxMediaModule* module, GxMediaModuleInject inject)
{
	int ret = 0;

	CHECK_MEDIA_MOLUDE_VALID(module);

	if ((module->sourcetype == GXMEDIA_SOURCE_DVBTS) ||
			(inject.source_type == GXMEDIA_SOURCE_DVBTS))
		return GX_MEDIAAPI_ERROR;

	if ((module->sourcetype == GXMEDIA_SOURCE_TS &&
				inject.source_type != GXMEDIA_SOURCE_TS) ||
			(module->sourcetype != GXMEDIA_SOURCE_TS &&
			 inject.source_type == GXMEDIA_SOURCE_TS))
		return GX_MEDIAAPI_ERROR;

	if (inject.source_type == GXMEDIA_SOURCE_TS) {
		GxMediaDemuxInject dmx_inject;

		dmx_inject.buf        = inject.buf;
		dmx_inject.len        = inject.len;
		dmx_inject.priv       = inject.priv;
		dmx_inject.decrypt_cb = inject.decrypt_cb;
		ret = GxMedia_DemuxInjectData(module->mdemux, &dmx_inject);
	} else {
		if (inject.stream_type == GXMEDIA_STREAM_VIDEO)
			ret = GxMedia_VideoInjectData(module->mvideo, inject.source_type, inject.buf, inject.len, inject.pts);
		else if (inject.stream_type == GXMEDIA_STREAM_AUDIO)
			ret = GxMedia_AudioInjectData(module->maudio, inject.source_type, inject.buf, inject.len, inject.pts);
		else
			ret = -1;
	}

	return ret;
}

int GxMedia_ModuleSpeed(GxMediaModule* module, float speed)
{
	CHECK_MEDIA_MOLUDE_VALID(module);

	if (speed < 0) return GX_MEDIAAPI_ERROR;

	GxMedia_StcSpeed  (module->mstc,   speed);
	GxMedia_VideoSpeed(module->mvideo, speed);
	GxMedia_AudioSpeed(module->maudio, speed);

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_ModuleGetInfo(GxMediaModule* module, GxMediaModuleInfo *info)
{
	CHECK_MEDIA_MOLUDE_VALID(module);

	if (info == NULL) return GX_MEDIAAPI_ERROR;

	if (module->sourcetype == GXMEDIA_SOURCE_TS)
		GxMedia_DemuxGetFifoInfo(module->mdemux, &info->demux);

	GxMedia_AudioGetFifoInfo(module->maudio, &info->audio);
	GxMedia_VideoGetFifoInfo(module->mvideo, &info->video);
	GxMedia_StcGetTime(module->mstc, &info->stc);
	GxMedia_AudioGetPts(module->maudio, &info->apts);
	GxMedia_VideoGetPts(module->mvideo, &info->vpts);

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_ModuleGetState(GxMediaModule* module, GxMediaModuleState *state)
{
	CHECK_MEDIA_MOLUDE_VALID(module);

	if (state == NULL) return GX_MEDIAAPI_ERROR;

	state->audio.state    = GXMEDIA_STATE_NO_DEC;
	state->audio.err_code = GXMEDIA_ERR_NO_ERROR;
	state->video.state    = GXMEDIA_STATE_NO_DEC;
	state->video.err_code = GXMEDIA_ERR_NO_ERROR;
	GxMedia_AudioGetDecState(module->maudio, &(state->audio.state), &(state->audio.err_code));
	GxMedia_VideoGetDecState(module->mvideo, &(state->video.state), &(state->video.err_code));

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_ModuleFinished(GxMediaModule* module)
{
	CHECK_MEDIA_MOLUDE_VALID(module);

	GxMedia_AudioFinished(module->maudio);
	GxMedia_VideoFinished(module->mvideo);

	return GX_MEDIAAPI_SUCCESS;
}
