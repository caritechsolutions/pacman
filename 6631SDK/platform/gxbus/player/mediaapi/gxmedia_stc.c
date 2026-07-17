#include "gxmedia_module.h"
#include "gxmedia_common.h"

GxMediaStc* GxMedia_StcOpen(GxMediaStcMod mod)
{
	GxMediaStc* mstc = av_mallocz(sizeof(GxMediaStc));

	if (mstc == NULL) return NULL;

	mstc->stc_dev = -1;
	mstc->stc_mod = -1;
	mstc->valid = 1;
	mstc->sync_mode = -1;
	mstc->freq_HZ = -1;

	mstc->stc_dev = GxAvdev_CreateDevice(0);
	if (mstc->stc_dev < 0) {
		GxMediaAPI_debug("[%s %d]: stc device open failed\n", __FUNCTION__, __LINE__);
		goto open_err;
	}

	mstc->stc_mod = GxAVOpenModule(mstc->stc_dev, GXAV_MOD_STC, mod.stc_modid);
	if (mstc->stc_mod < 0) {
		GxMediaAPI_debug("[%s %d]: stc module open failed\n", __FUNCTION__, __LINE__);
		goto open_err;
	}

	return mstc;

open_err:
	GxMedia_StcClose(mstc);
	return NULL;
}

int GxMedia_StcClose(GxMediaStc* mstc)
{
	CHECK_MEDIA_MOLUDE_VALID(mstc);

	GxMedia_StcStop(mstc);

	if(mstc->stc_mod != -1)
		GxAvdev_CloseModule(mstc->stc_dev, mstc->stc_mod);

	if(mstc->stc_dev != -1)
		GxAvdev_DestroyDevice(mstc->stc_dev);

	mstc->valid = 0;
	mstc->stc_mod = -1;
	mstc->stc_dev = -1;
	av_free(mstc);

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_StcConfig(GxMediaStc* mstc, int freq_HZ, int sync_mode)
{
	CHECK_MEDIA_MOLUDE_VALID(mstc);

	mstc->sync_mode = (sync_mode>=0)?sync_mode:APTS_RECOVER;
	mstc->freq_HZ   = (freq_HZ>0)?freq_HZ:45000;

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_StcStart(GxMediaStc* mstc)
{
	int ret = 0;
	GxSTCProperty_Config stc_config;
	GxSTCProperty_TimeResolution time_resolution;

	CHECK_MEDIA_MOLUDE_VALID(mstc);

	if (mstc->running == 1)
		return GX_MEDIAAPI_SUCCESS;

	if (mstc->sync_mode >= 0) {
		memset(&stc_config, 0, sizeof(GxSTCProperty_Config));
		stc_config.mode = mstc->sync_mode;
		GxAVSetProperty(mstc->stc_dev, mstc->stc_mod,
				GxSTCPropertyID_Config, &stc_config,sizeof(GxSTCProperty_Config));
		if (ret < 0)
			GxMediaAPI_debug("[%s %d]: stc config failed\n", __FUNCTION__, __LINE__);
	}

	if (mstc->freq_HZ > 0) {
		memset(&time_resolution, 0, sizeof(GxSTCPropertyID_TimeResolution));
		time_resolution.freq_HZ = mstc->freq_HZ;
		GxAVSetProperty(mstc->stc_dev, mstc->stc_mod, \
				GxSTCPropertyID_TimeResolution, &time_resolution,sizeof(GxSTCProperty_TimeResolution));
		if (ret < 0)
			GxMediaAPI_debug("[%s %d]: stc time resolution failed\n", __FUNCTION__, __LINE__);
	}

	GxAVSetProperty(mstc->stc_dev, mstc->stc_mod, GxSTCPropertyID_Play, NULL,0);

	mstc->running = 1;
	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_StcStop(GxMediaStc* mstc)
{
	CHECK_MEDIA_MOLUDE_VALID(mstc);

	if (mstc->running == 0)
		return GX_MEDIAAPI_SUCCESS;

	GxAVSetProperty(mstc->stc_dev, mstc->stc_mod, GxSTCPropertyID_Stop, NULL,0);

	mstc->sync_mode = -1;
	mstc->freq_HZ = -1;
	mstc->running = 0;
	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_StcPause(GxMediaStc* mstc)
{
	CHECK_MEDIA_MOLUDE_VALID(mstc);

	GxAVSetProperty(mstc->stc_dev, mstc->stc_mod, GxSTCPropertyID_Pause, NULL,0);

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_StcResume(GxMediaStc* mstc)
{
	CHECK_MEDIA_MOLUDE_VALID(mstc);

	GxAVSetProperty(mstc->stc_dev, mstc->stc_mod, GxSTCPropertyID_Resume, NULL,0);

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_StcSpeed(GxMediaStc* mstc, float speed)
{
	int ret = 0;
	GxSTCProperty_TimeResolution time_resolution;

	CHECK_MEDIA_MOLUDE_VALID(mstc);

	if (speed < 0) return GX_MEDIAAPI_ERROR;

	time_resolution.freq_HZ = (unsigned int)(mstc->freq_HZ * speed);
	ret = GxAVSetProperty(mstc->stc_dev, mstc->stc_mod, \
			GxSTCPropertyID_TimeResolution, &time_resolution,sizeof(GxSTCProperty_TimeResolution));
	if (ret < 0) {
		GxMediaAPI_debug("[%s %d]: stc time resolution failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_StcGetTime(GxMediaStc* mstc, int64_t* stc)
{
	GxSTCProperty_Time Time;
	CHECK_MEDIA_MOLUDE_VALID(mstc);

	GxAVGetProperty(mstc->stc_dev, mstc->stc_mod, GxSTCPropertyID_Time, &Time, sizeof(GxSTCProperty_Time));
	if (stc)
		*stc =(int)Time.time;

	return GX_MEDIAAPI_SUCCESS;
}

