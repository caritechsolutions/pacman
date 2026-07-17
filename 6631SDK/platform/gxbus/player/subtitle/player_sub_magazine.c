#include "gx_media.h"
#include "gx_avout.h"
#include "../gxplayer_internal.h"
#include "gx_teletext.h"
#include "sub_pes.h"
#include "sub_sync.h"

#ifdef CONFIG_SUBTITLE_TELETEXT

static GxTeletext_SyncState _magazine_sync_pts(void *priv, int64_t pts)
{
	PISubBlock *subblock = (PISubBlock *)priv;
	GxPlayer   *player   = subblock->player;

	if (!player || !player->media_play)
		return GX_TELETEXT_SYNC_ERROR;

	if (subblock->substatus != GXPLAYER_SUB_RUNNING)
		return GX_TELETEXT_SYNC_ERROR;

	return (GxTeletext_SyncState)sub_pts_sync(player->media_play, pts, subblock->delay);
}

static void player_magazine(void* data)
{
#define MAX_READ_SIZE (64*1024 + 6)
	PISubBlock *subblock = (PISubBlock*)data;
	GxPlayer *player = (GxPlayer*)subblock->player;
	uint8_t  *buffer = av_malloc(MAX_READ_SIZE);

	if (buffer == NULL) {
		subblock->pause_ack = 1;
		return;
	}

	subblock->pause_ack = 0;
	while (subblock->substatus != GXPLAYER_SUB_STOPPED)
	{
		int32_t size = 0, use_size = 0, pes_len = 0;
		uint8_t *pes = NULL;
		int64_t pts = -1;

		GxTeletext_Timer(subblock->sub_handle);

		if (subblock->substatus == GXPLAYER_SUB_PAUSED){
			subblock->pause_ack = 1;
			GxCore_ThreadDelay(100);
			continue;
		} else
			subblock->pause_ack = 0;

		if (player->media_play == NULL) {
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

			GxTeletext_SendPesData(subblock->sub_handle, pes, pes_len, pts);
			GxTeletext_Timer(subblock->sub_handle);
		}
	}

	av_free(buffer);
	subblock->pause_ack = 1;
	gxlogf("%s %d, teletext subtitle exit\n", __FUNCTION__, __LINE__);
}

static int _start_magazine(PISubBlock* subblock)
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
			GxCore_ThreadCreate("magazine_thread", \
					&subblock->sub_thread, player_magazine, subblock, 32*1024, GXOS_DEFAULT_PRIORITY);
		} else {
			subblock->substatus = GXPLAYER_SUB_STOPPED;
		}
	}
	return GX_PLAYER_OK;
}

static int _stop_magazine(PISubBlock* subblock, int freeze)
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

static PISubBlock* player_open_magazine(PISubBlock* subblock,  PISubParam* subpara)
{
	GxTeletext_Param  param;

	if (!subblock || !subpara)
		return NULL;

	if (subpara->pid.pid >= 0x1fff || subpara->pid.pid <= 0)
		return NULL;

	param.type   = GXSUBTITLE_SPP;
	param.format = GX_TELETEXT_MAG;
	param.sync   = _magazine_sync_pts;
	param.priv   = (void*)subblock;

	subblock->sub_handle = GxTeletext_Open(&param);
	memcpy(&subblock->subpara, subpara, sizeof(PISubParam));

	if (_start_magazine(subblock) == GX_PLAYER_ERROR)
		return NULL;

	return subblock;
}

static void player_close_magazine(PISubBlock* subblock)
{
	GxPlayer* player = (GxPlayer*)subblock->player;

	if (player == NULL)
		return;

	_stop_magazine(subblock, 0);
	GxTeletext_Close(subblock->sub_handle);
	subblock->sub_handle = 0;
}

static void player_hide_magazine(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		GxTeletext_Hide(subblock->sub_handle);
}

static void player_show_magazine(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		GxTeletext_Show(subblock->sub_handle);
}

static void player_sync_magazine(PISubBlock* subblock,int32_t timems)
{
	subblock->delay = timems;
}

static void player_pause_magazine(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		subblock->substatus = GXPLAYER_SUB_PAUSED;
}

static void player_stop_magazine(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle) {
		int retry = 100;
		subblock->substatus = GXPLAYER_SUB_PAUSED;

		while ((retry--) > 0) {
			if (subblock->pause_ack)
				break;
			GxCore_ThreadDelay(10);
		}
		if (retry == 0)
			gxloge("%s %d ...\n", __func__, __LINE__);
	}
}

static void player_resume_magazine(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle) {
		if (subblock->substatus == GXPLAYER_SUB_PAUSED)
			subblock->substatus = GXPLAYER_SUB_RUNNING;
		else if (subblock->substatus == GXPLAYER_SUB_STOPPED)
			_start_magazine(subblock);
	}
}

static void player_switch_stream_magazine(PISubBlock* subblock, PlayerSubPID pid)
{
	if (pid.pid >= 0x1fff || pid.pid <= 0)
		return;

	if(subblock && subblock->sub_handle)
	{
		if (subblock->substatus == GXPLAYER_SUB_RUNNING) {
			_stop_magazine(subblock, 1);
			subblock->subpara.pid = pid;
			_start_magazine(subblock);
		} else
			subblock->subpara.pid = pid;
	}
}

static void player_switch_render_magazine(PISubBlock* subblock,PlayerSubRender render)
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

PISubCtrl subctrl_magazine = {
	.type = PLAYER_SUB_TYPE_DVB_MAG,
	.name = "PLAYER_SUB_TYPE_DVB_MAG",

	.open    = player_open_magazine,
	.close   = player_close_magazine,
	.hide    = player_hide_magazine,
	.show    = player_show_magazine,
	.sync    = player_sync_magazine,
	.pause   = player_pause_magazine,
	.stop    = player_stop_magazine,
	.resume  = player_resume_magazine,
	.switch_stream      = player_switch_stream_magazine,
	.switch_render      = player_switch_render_magazine,
	.reset_source       = player_reset_source_teletext,
};
#endif

