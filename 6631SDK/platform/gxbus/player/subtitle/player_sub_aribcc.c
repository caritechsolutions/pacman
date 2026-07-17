#include "gx_media.h"
#include "gx_avout.h"
#include "../gxplayer_internal.h"
#include "gx_arib_cc.h"
#include "sub_pes.h"
#include "sub_sync.h"
#include "../demuxer/demux_pes.h"

static void _arib_cc_thread(void* data)
{
#define MAX_READ_SIZE (64*1024 + 6)
	PISubBlock *subblock = (PISubBlock*)data;
	GxPlayer *player = (GxPlayer*)subblock->player;
	uint8_t  *buffer = av_malloc(MAX_READ_SIZE);
	handle_t handle  = MpegPesCreate();

	if (buffer == NULL) {
		MpegPesDestory(handle);
		return;
	}

	while (subblock->substatus != GXPLAYER_SUB_STOPPED) {
		int32_t size = 0;

		if (subblock->substatus == GXPLAYER_SUB_PAUSED){
			GxCore_ThreadDelay(100);
			continue;
		}

		size = GxDemuxer_ReadPesData(player->media_play->demuxer, buffer, MAX_READ_SIZE);
		if (size <= 0) {
			GxCore_ThreadDelay(100);
			continue;
		} else if (size == -1) {
			gxlogf("%s %d, PES read error\n", __FUNCTION__, __LINE__);
			break;
		}
		MpegPesInsertBuffer(handle, buffer, size);

		while (subblock->substatus == GXPLAYER_SUB_RUNNING) {
			int64_t pts = -1;

			size = MpegPesReadEs(handle, buffer, MAX_READ_SIZE, &pts, NULL);
			if (size < 0)
				break;
			else if (size == 0)
				continue;

			while (subblock->substatus == GXPLAYER_SUB_RUNNING) {
				GxSubSyncState state = SUB_ERROR;

				if (!(GX_SPEED_SLOW(player->speed) || GX_SPEED_NORMAL(player->speed))) {
					GxAribcc_Clear(subblock->sub_handle);
					break;
				}

				state = sub_pts_sync(player->media_play, pts, subblock->delay);
				if (state == SUB_SKIP) {
					GxAribcc_Clear(subblock->sub_handle);
					break;
				} else if ((state == SUB_NORMAL) || (state == SUB_FREERUN)) {
					GxAribcc_SendESData(subblock->sub_handle, buffer, size);
					break;
				} else {
					if (state == SUB_REPEAT)
						GxCore_ThreadDelay(100);
					else if (state == SUB_ERROR)
						GxCore_ThreadDelay(10);
				}
			}
		}
	}

	MpegPesDestory(handle);
	av_free(buffer);
	gxlogf("%s %d, cc subtitle exit\n", __FUNCTION__, __LINE__);
}

static int _arib_cc_start(PISubBlock* subblock)
{
	GxAribcc_Stream sub_stream;
	GxPlayer* player = (GxPlayer*)subblock->player;

	if (player == NULL)
		return GX_PLAYER_ERROR;

	if (subblock->substatus == GXPLAYER_SUB_RUNNING)
		return GX_PLAYER_OK;

	if (player->media_play && player->media_play->demuxer) {
		sub_stream.pid = subblock->subpara.pid.pid;
		sub_stream.group_a_id = subblock->subpara.pid.major;
		sub_stream.group_b_id = subblock->subpara.pid.minor;
		GxAribcc_StreamSet(subblock->sub_handle, &sub_stream);
		GxAribcc_Start(subblock->sub_handle);
		GxDemuxer_Control(player->media_play->demuxer, GX_DEMUXER_CTRL_SUB_ON, &sub_stream.pid);
		subblock->substatus = GXPLAYER_SUB_RUNNING;

		GxCore_ThreadCreate("cc_sub_thread", \
				&subblock->sub_thread, _arib_cc_thread, subblock, 16*1024, GXOS_DEFAULT_PRIORITY);
	}
	return GX_PLAYER_OK;
}

static int _arib_cc_stop(PISubBlock* subblock, int freeze)
{
	GxPlayer* player = (GxPlayer*)subblock->player;

	if (player == NULL)
		return GX_PLAYER_ERROR;

	if (subblock->substatus == GXPLAYER_SUB_STOPPED)
		return GX_PLAYER_OK;

	if (player->media_play && player->media_play->demuxer) {
		subblock->substatus = GXPLAYER_SUB_STOPPED;
		GxCore_ThreadJoin(subblock->sub_thread);
		GxDemuxer_Control(player->media_play->demuxer, GX_DEMUXER_CTRL_SUB_OFF, NULL);
		GxAribcc_Stop(subblock->sub_handle, freeze);
	}
	return GX_PLAYER_OK;
}

static PISubBlock* arib_cc_open(PISubBlock* subblock,  PISubParam* subpara)
{
	GxAribcc_Param param;

	if (!subblock || !subpara)
		return NULL;

	if (subpara->pid.pid >= 0x1fff || subpara->pid.pid <= 0)
		return NULL;

	param.render_type = GXSUBTITLE_SPP;
	subblock->sub_handle = GxAribcc_Open(&param);
	memcpy(&subblock->subpara, subpara, sizeof(PISubParam));

	if (_arib_cc_start(subblock) == GX_PLAYER_ERROR)
		return NULL;

	return subblock;
}

static void arib_cc_close(PISubBlock* subblock)
{
	GxPlayer* player = (GxPlayer*)subblock->player;

	if (player == NULL)
		return;

	_arib_cc_stop(subblock, 0);
	GxAribcc_Close(subblock->sub_handle);
	subblock->sub_handle = 0;

	return;
}

static void arib_cc_hide(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		GxAribcc_Hide(subblock->sub_handle);
	return;
}

static void arib_cc_show(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		GxAribcc_Show(subblock->sub_handle);
	return;

}

static void arib_cc_sync(PISubBlock* subblock,int32_t timems)
{
	subblock->delay = timems;
}

static void arib_cc_pause(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		subblock->substatus = GXPLAYER_SUB_PAUSED;

	return;
}

static void arib_cc_stop(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		_arib_cc_stop(subblock, 1);

	return;
}

static void arib_cc_resume(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle) {
		if (subblock->substatus == GXPLAYER_SUB_PAUSED)
			subblock->substatus = GXPLAYER_SUB_RUNNING;
		else if (subblock->substatus == GXPLAYER_SUB_STOPPED)
			_arib_cc_start(subblock);
	}

	return;
}

static void arib_cc_switch_stream(PISubBlock* subblock, PlayerSubPID pid)
{
	if (pid.pid >= 0x1fff || pid.pid <= 0)
		return;

	if(subblock && subblock->sub_handle)
	{
		if (subblock->substatus == GXPLAYER_SUB_RUNNING) {
			arib_cc_stop(subblock);
			subblock->subpara.pid = pid;
			arib_cc_resume(subblock);
		} else
			subblock->subpara.pid = pid;
	}

	return;
}

static void arib_cc_switch_render(PISubBlock* subblock,PlayerSubRender render)
{

}

static void arib_cc_switch_resolution(PISubBlock* subblock,VideoOutputMode mode)
{

}

PISubCtrl subctrl_aribcc = {
	.type =  PLAYER_SUB_TYPE_ARIB_CC,
	.name = "ARIB STD-B24 Closed Caption",

	.open   = arib_cc_open,
	.close  = arib_cc_close,
	.hide   = arib_cc_hide,
	.show   = arib_cc_show,
	.sync   = arib_cc_sync,
	.stop   = arib_cc_stop,
	.pause  = arib_cc_pause,
	.resume = arib_cc_resume,
	.switch_stream     = arib_cc_switch_stream,
	.switch_render     = arib_cc_switch_render,
	.switch_resolution = arib_cc_switch_resolution,
};
