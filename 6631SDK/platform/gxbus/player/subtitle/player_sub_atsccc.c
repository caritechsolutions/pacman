#include "gx_media.h"
#include "gx_avout.h"
#include "../gxplayer_internal.h"
#include "gx_atsc_cc.h"

static void _atsc_cc_thread(void* data)
{
#define MAX_READ_SIZE (64*1024 + 6)
	PISubBlock *subblock = (PISubBlock*)data;
	GxPlayer *player = (GxPlayer*)subblock->player;
	uint8_t  *buffer = av_malloc(MAX_READ_SIZE);
	unsigned int len = 0;

	if (buffer == NULL) return;

	while (subblock->substatus != GXPLAYER_SUB_STOPPED) {
		if (subblock->substatus == GXPLAYER_SUB_PAUSED){
			GxCore_ThreadDelay(100);
			continue;
		}

		len = GxMedia_Read(player->media_play, PLAYER_READ_VDECODER_SUBCC, buffer, MAX_READ_SIZE);
		if (len == 0) {
			GxCore_ThreadDelay(100);
			continue;
		}
		GxAtsccc_SendData(subblock->sub_handle, buffer, len);
		GxCore_ThreadDelay(10);
	}

	av_free(buffer);
	gxlogf("%s %d, cc subtitle exit\n", __FUNCTION__, __LINE__);
}

static int _atsc_cc_start(PISubBlock* subblock)
{
	GxPlayer* player = (GxPlayer*)subblock->player;

	if (player == NULL || player->media_play == NULL)
		return GX_PLAYER_ERROR;

	if (subblock->substatus == GXPLAYER_SUB_RUNNING)
		return GX_PLAYER_OK;

	GxAtsccc_StreamSet(subblock->sub_handle, subblock->subpara.service_id);
	GxAtsccc_Start(subblock->sub_handle);
	subblock->substatus = GXPLAYER_SUB_RUNNING;

	GxCore_ThreadCreate("atsc_cc_thread", \
			&subblock->sub_thread, _atsc_cc_thread, subblock, 16*1024, GXOS_DEFAULT_PRIORITY);

	return GX_PLAYER_OK;
}

static int _atsc_cc_stop(PISubBlock* subblock)
{
	GxPlayer* player = (GxPlayer*)subblock->player;

	if (player == NULL || player->media_play == NULL)
		return GX_PLAYER_ERROR;

	if (subblock->substatus == GXPLAYER_SUB_STOPPED)
		return GX_PLAYER_OK;

	subblock->substatus = GXPLAYER_SUB_STOPPED;
	GxCore_ThreadJoin(subblock->sub_thread);
	GxAtsccc_Stop(subblock->sub_handle);

	return GX_PLAYER_OK;
}

static PISubBlock* atsc_cc_open(PISubBlock* subblock,  PISubParam* subpara)
{
	GxAtsccc_Param param;

	if (!subblock || !subpara)
		return NULL;

	param.id   = subpara->service_id;
	subblock->sub_handle = GxAtsccc_Open(&param);
	memcpy(&subblock->subpara, subpara, sizeof(PISubParam));

	if (_atsc_cc_start(subblock) != GX_PLAYER_OK)
		return NULL;

	return subblock;
}

static void atsc_cc_close(PISubBlock* subblock)
{
	if (!subblock)
		return;

	if (_atsc_cc_stop(subblock) != GX_PLAYER_OK)
		return;

	GxAtsccc_Close(subblock->sub_handle);
	subblock->sub_handle = 0;

	return;
}

static void atsc_cc_hide(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		GxAtsccc_Hide(subblock->sub_handle);

	return;
}

static void atsc_cc_show(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		GxAtsccc_Show(subblock->sub_handle);

	return;
}

static void atsc_cc_sync(PISubBlock* subblock,int32_t timems)
{
	subblock->delay = timems;
}

static void atsc_cc_pause(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		subblock->substatus = GXPLAYER_SUB_PAUSED;

	return;
}

static void atsc_cc_stop(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		_atsc_cc_stop(subblock);

	return;
}

static void atsc_cc_resume(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle) {
		if (subblock->substatus == GXPLAYER_SUB_PAUSED)
			subblock->substatus = GXPLAYER_SUB_RUNNING;
		else if (subblock->substatus == GXPLAYER_SUB_STOPPED)
			_atsc_cc_start(subblock);
	}

	return;
}

static void atsc_cc_switch_stream(PISubBlock* subblock, PlayerSubPID pid)
{
	return;
}

static void atsc_cc_switch_render(PISubBlock* subblock,PlayerSubRender render)
{
	return;
}

static void atsc_cc_switch_resolution(PISubBlock* subblock,VideoOutputMode mode)
{
	return;
}

PISubCtrl subctrl_atsccc = {
	.type =  PLAYER_SUB_TYPE_ATSC_CC,
	.name = "MPEG-2 Video Closed Caption",

	.open   = atsc_cc_open,
	.close  = atsc_cc_close,
	.hide   = atsc_cc_hide,
	.show   = atsc_cc_show,
	.sync   = atsc_cc_sync,
	.pause  = atsc_cc_pause,
	.stop   = atsc_cc_stop,
	.resume = atsc_cc_resume,
	.switch_stream     = atsc_cc_switch_stream,
	.switch_render     = atsc_cc_switch_render,
	.switch_resolution = atsc_cc_switch_resolution,
};
