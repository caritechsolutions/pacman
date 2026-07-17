#include "gx_media.h"
#include "gx_avout.h"
#include "../gxplayer_internal.h"
#include "gx_teletext.h"
#include "sub_pes.h"
#include "sub_sync.h"

#ifdef CONFIG_SUBTITLE_TELETEXT

static void player_teletext(void* data)
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
					GxTeletext_Clear(subblock->sub_handle);
					break;
				}

				state = sub_pts_sync(player->media_play, pts, subblock->delay);
				if (state == SUB_SKIP) {
					GxTeletext_Clear(subblock->sub_handle);
					break;
				} else if ((state == SUB_NORMAL) || (state == SUB_FREERUN)) {
					GxTeletext_SendPesData(subblock->sub_handle, pes, pes_len, -1);
					break;
				} else {
					if (state == SUB_REPEAT)
						GxCore_ThreadDelay(100);
					else if (state == SUB_ERROR)
						GxCore_ThreadDelay(10);
				}
			}

			if (use_size >= size) break;
		}
	}

	av_free(buffer);
	gxlogf("%s %d, teletext subtitle exit\n", __FUNCTION__, __LINE__);
}

static int _start_teletext(PISubBlock* subblock)
{
	GxTeletext_Stream sub_stream;
	GxPlayer* player = (GxPlayer*)subblock->player;

	if (player == NULL)
		return GX_PLAYER_ERROR;

	if (subblock->substatus == GXPLAYER_SUB_RUNNING)
		return GX_PLAYER_OK;

	if (player->media_play && player->media_play->demuxer) {
		sub_stream.pid = subblock->subpara.pid.pid;
		sub_stream.comp_page_id = subblock->subpara.pid.major;
		sub_stream.anci_page_id = subblock->subpara.pid.minor;
		GxTeletext_StreamSet(subblock->sub_handle, &sub_stream);
		GxTeletext_Start(subblock->sub_handle);

		int ret = GxDemuxer_Control(player->media_play->demuxer, GX_DEMUXER_CTRL_SUB_ON, &subblock->subpara.pid.pid);
		if (ret == GX_PLAYER_OK) {
			subblock->substatus = GXPLAYER_SUB_RUNNING;
			GxCore_ThreadCreate("teletext_thread", \
					&subblock->sub_thread, player_teletext, subblock, 32*1024, GXOS_DEFAULT_PRIORITY);
		} else {
			subblock->substatus = GXPLAYER_SUB_STOPPED;
		}
	}
	return GX_PLAYER_OK;
}

static int _stop_teletext(PISubBlock* subblock, int freeze)
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
		GxTeletext_Stop(subblock->sub_handle, freeze);
	}
	return GX_PLAYER_OK;
}

static PISubBlock* player_open_teletext(PISubBlock* subblock,  PISubParam* subpara)
{
	GxTeletext_Param  param;

	if (!subblock || !subpara)
		return NULL;

	if (subpara->pid.pid >= 0x1fff || subpara->pid.pid <= 0)
		return NULL;

	param.type   = GXSUBTITLE_SPP;
	param.format = GX_TELETEXT_TTX;
	param.sync   = NULL;
	param.priv   = NULL;

	subblock->sub_handle = GxTeletext_Open(&param);
	memcpy(&subblock->subpara, subpara, sizeof(PISubParam));

	if (_start_teletext(subblock) == GX_PLAYER_ERROR)
		return NULL;

	return subblock;
}

static void player_close_teletext(PISubBlock* subblock)
{
	GxPlayer* player = (GxPlayer*)subblock->player;

	if (player == NULL)
		return;

	_stop_teletext(subblock, 0);
	GxTeletext_Close(subblock->sub_handle);
	subblock->sub_handle = 0;
}

static void player_hide_teletext(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		GxTeletext_Hide(subblock->sub_handle);
}

static void player_show_teletext(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		GxTeletext_Show(subblock->sub_handle);
}

static void player_sync_teletext(PISubBlock* subblock,int32_t timems)
{
	subblock->delay = timems;
}

static void player_pause_teletext(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		subblock->substatus = GXPLAYER_SUB_PAUSED;
}

static void player_stop_teletext(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		_stop_teletext(subblock, 1);
}

static void player_resume_teletext(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle) {
		if (subblock->substatus == GXPLAYER_SUB_PAUSED)
			subblock->substatus = GXPLAYER_SUB_RUNNING;
		else if (subblock->substatus == GXPLAYER_SUB_STOPPED)
			_start_teletext(subblock);
	}
}

static void player_switch_stream_teletext(PISubBlock* subblock, PlayerSubPID pid)
{
	if (pid.pid >= 0x1fff || pid.pid <= 0)
		return;

	if(subblock && subblock->sub_handle)
	{
		if (subblock->substatus == GXPLAYER_SUB_RUNNING) {
			player_stop_teletext(subblock);
			subblock->subpara.pid = pid;
			player_resume_teletext(subblock);
		} else
			subblock->subpara.pid = pid;
	}
}

static void player_switch_render_teletext(PISubBlock* subblock,PlayerSubRender render)
{

}

static void player_reset_source_teletext(PISubBlock* subblock)
{
	if ((subblock->substatus == GXPLAYER_SUB_RUNNING) ||
			(subblock->substatus == GXPLAYER_SUB_PAUSED)) {
		GxPlayer* player = (GxPlayer*)subblock->player;

		if (player->media_play && player->media_play->demuxer) {
			GxDemuxer_Control(player->media_play->demuxer, GX_DEMUXER_CTRL_SUB_OFF, NULL);
			GxDemuxer_Control(player->media_play->demuxer, GX_DEMUXER_CTRL_SUB_ON, &subblock->subpara.pid.pid);
		}
	}
}

PISubCtrl subctrl_teletext = {
	.type = PLAYER_SUB_TYPE_DVB_TTX,
	.name = "PLAYER_SUB_TYPE_DVB_TTX",

	.open    = player_open_teletext,
	.close   = player_close_teletext,
	.hide    = player_hide_teletext,
	.show    = player_show_teletext,
	.sync    = player_sync_teletext,
	.pause   = player_pause_teletext,
	.stop    = player_stop_teletext,
	.resume  = player_resume_teletext,
	.switch_stream      = player_switch_stream_teletext,
	.switch_render      = player_switch_render_teletext,
	.reset_source       = player_reset_source_teletext,
};
#endif

