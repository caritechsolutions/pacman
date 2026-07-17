#include "gx_media.h"
#include "gx_avout.h"
#include "../gxplayer_internal.h"
#include "gx_dvb_subtitle.h"
#include "sub_pes.h"
#include "sub_sync.h"

#ifdef CONFIG_SUBTITLE_DVB

static void player_sub_dvb(void* data)
{
#define MAX_READ_SIZE (64*1024 + 6)
	PISubBlock *subblock = (PISubBlock*)data;
	GxPlayer *player = (GxPlayer*)subblock->player;
	uint8_t  *buffer = av_malloc(MAX_READ_SIZE);

	if (buffer == NULL) return;

	while (subblock->substatus != GXPLAYER_SUB_STOPPED)
	{
		int32_t size = 0, use_size = 0, pes_len = 0;
		uint8_t *pes = NULL;
		int64_t pts = -1;

		if (subblock->substatus == GXPLAYER_SUB_PAUSED){
			GxCore_ThreadDelay(100);
			GxDvbSubt_ShowTimer(subblock->sub_handle);
			continue;
		}

		size = GxDemuxer_ReadPesData(player->media_play->demuxer, buffer, MAX_READ_SIZE);
		if (size <= 0) {
			GxCore_ThreadDelay(100);
			GxDvbSubt_ShowTimer(subblock->sub_handle);
			continue;
		} else if (size == -1) {
			gxlogf("%s %d, PES read error\n", __FUNCTION__, __LINE__);
			break;
		}

		while (subblock->substatus == GXPLAYER_SUB_RUNNING) {
			if (sub_parse_pes(buffer+use_size, size-use_size, &pes, &pes_len, &pts) == -1)
				break;

			use_size += pes_len;
			if (use_size > size) {
				gxlogf("%s %d, PES Over Flow\n", __FUNCTION__, __LINE__);
				break;
			}

			while (subblock->substatus == GXPLAYER_SUB_RUNNING) {
				GxSubSyncState state = SUB_ERROR;

				if (!(GX_SPEED_SLOW(player->speed) || GX_SPEED_NORMAL(player->speed))) {
					GxDvbSubt_Clear(subblock->sub_handle);
					break;
				}

				state = sub_pts_sync(player->media_play, pts, subblock->delay);
				if (state == SUB_SKIP) {
					GxDvbSubt_Clear(subblock->sub_handle);
					break;
				} else if ((state == SUB_NORMAL) || (state == SUB_FREERUN)) {
					GxDvbSubt_SendPesData(subblock->sub_handle, pes, pes_len);
					break;
				} else {
					if (state == SUB_REPEAT)
						GxCore_ThreadDelay(100);
					else if (state == SUB_ERROR)
						GxCore_ThreadDelay(10);
				}
				GxDvbSubt_ShowTimer(subblock->sub_handle);
			}

			if (use_size >= size) break;
		}
	}

	av_free(buffer);
	gxlogf("%s %d, dvb subtitle exit\n", __FUNCTION__, __LINE__);
}

static int _start_sub_dvb(PISubBlock* subblock)
{
	GxDvbSubt_Stream sub_stream;
	GxPlayer* player = (GxPlayer*)subblock->player;

	if (player == NULL)
		return GX_PLAYER_ERROR;

	if (subblock->substatus == GXPLAYER_SUB_RUNNING)
		return GX_PLAYER_OK;

	if (player->media_play && player->media_play->demuxer) {
		sub_stream.pid = subblock->subpara.pid.pid;
		sub_stream.comp_page_id = subblock->subpara.pid.major;
		sub_stream.anci_page_id = subblock->subpara.pid.minor;
		GxDvbSubt_StreamSet(subblock->sub_handle, &sub_stream);
		GxDvbSubt_Start(subblock->sub_handle);

		int ret = GxDemuxer_Control(player->media_play->demuxer, GX_DEMUXER_CTRL_SUB_ON, &sub_stream.pid);
		if (ret == GX_PLAYER_OK) {
			subblock->substatus = GXPLAYER_SUB_RUNNING;
			GxCore_ThreadCreate("dvb_sub_thread", \
					&subblock->sub_thread, player_sub_dvb, subblock, 8*1024, GXOS_DEFAULT_PRIORITY);
		} else {
			subblock->substatus = GXPLAYER_SUB_STOPPED;
		}
	}
	return GX_PLAYER_OK;
}

static int _stop_sub_dvb(PISubBlock* subblock, int freeze)
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
		GxDvbSubt_Stop(subblock->sub_handle, freeze);
	}
	return GX_PLAYER_OK;
}

static PISubBlock* player_sub_open_dvb(PISubBlock* subblock,  PISubParam* subpara)
{
	GxDvbSubt_Param  param;

	if (!subblock || !subpara)
		return NULL;

	if (subpara->pid.pid >= 0x1fff || subpara->pid.pid <= 0)
		return NULL;

#if 0
	param.render_type = GXSUBTITLE_SPP;
	memcpy(&param.render_rect,     &subpara->rect,     sizeof(PlayerSubRect));
	memcpy(&param.render_rect_max, &subpara->rect_max, sizeof(PlayerSubRect));
#endif

	subblock->sub_handle = GxDvbSubt_Open(&param);
	if (subblock->sub_handle == 0)
		return NULL;

	memcpy(&subblock->subpara, subpara, sizeof(PISubParam));

	if (_start_sub_dvb(subblock) == GX_PLAYER_ERROR)
		return NULL;

	return subblock;
}

static void player_sub_close_dvb(PISubBlock* subblock)
{
	GxPlayer* player = (GxPlayer*)subblock->player;

	if (player == NULL)
		return;

	_stop_sub_dvb(subblock, 0);
	GxDvbSubt_Close(subblock->sub_handle);
	subblock->sub_handle = 0;
}

static void player_sub_hide_dvb(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		GxDvbSubt_Hide(subblock->sub_handle);
}

static void player_sub_show_dvb(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		GxDvbSubt_Show(subblock->sub_handle);
}

static void player_sub_sync_dvb(PISubBlock* subblock,int32_t timems)
{
	subblock->delay = timems;
}

static void player_sub_pause_dvb(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		subblock->substatus = GXPLAYER_SUB_PAUSED;
}

static void player_sub_stop_dvb(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		_stop_sub_dvb(subblock, 1);
}

static void player_sub_resume_dvb(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle) {
		if (subblock->substatus == GXPLAYER_SUB_PAUSED)
			subblock->substatus = GXPLAYER_SUB_RUNNING;
		else if (subblock->substatus == GXPLAYER_SUB_STOPPED)
			_start_sub_dvb(subblock);
	}
}

static void player_sub_switch_stream_dvb(PISubBlock* subblock, PlayerSubPID pid)
{
	if (pid.pid >= 0x1fff || pid.pid <= 0)
		return;

	if(subblock && subblock->sub_handle)
	{
		if (subblock->substatus == GXPLAYER_SUB_RUNNING) {
			player_sub_stop_dvb(subblock);
			subblock->subpara.pid = pid;
			player_sub_resume_dvb(subblock);
		} else
			subblock->subpara.pid = pid;
	}
}

static void player_sub_switch_render_dvb(PISubBlock* subblock,PlayerSubRender render)
{

}

static void player_sub_switch_resolution_dvb(PISubBlock* subblock,VideoOutputMode mode)
{
	if(subblock->sub_handle)
	{
#if 0
		if(mode  == GX_VIDEO_OUTPUT_NTSC_M ||
				mode == GX_VIDEO_OUTPUT_NTSC_443 ||
				mode == GX_VIDEO_OUTPUT_PAL_M ||
				mode == GX_VIDEO_OUTPUT_480I ||
				mode == GX_VIDEO_OUTPUT_480P
		  )
			GxDvbSubt_SetNtsc((handle_t)subblock->sub_handle, 1);
		else
			GxDvbSubt_SetNtsc((handle_t)subblock->sub_handle, 0);
#endif
	}
}

PISubCtrl subctrl_dvb = {
	.type = PLAYER_SUB_TYPE_DVB,
	.name = "PLAYER_SUB_TYPE_DVB",

	.open  = player_sub_open_dvb,
	.close = player_sub_close_dvb,
	.hide  = player_sub_hide_dvb,
	.show  = player_sub_show_dvb,
	.sync  = player_sub_sync_dvb,
	.pause = player_sub_pause_dvb,
	.stop  = player_sub_stop_dvb,
	.resume = player_sub_resume_dvb,
	.switch_stream  = player_sub_switch_stream_dvb,
	.switch_render  = player_sub_switch_render_dvb,
	.switch_resolution  = player_sub_switch_resolution_dvb,
};
#endif

