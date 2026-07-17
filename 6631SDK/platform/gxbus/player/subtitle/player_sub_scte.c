#include "gx_media.h"
#include "gx_avout.h"
#include "gx_scte_subtitle.h"
#include "sub_sync.h"
#include "../gxplayer_internal.h"

static int _scte_start(PISubBlock* subblock)
{
	GxPlayer* player = subblock->player;

	if (!player || !player->media_play)
		return GX_PLAYER_ERROR;

	if (subblock->substatus == GXPLAYER_SUB_RUNNING)
		return GX_PLAYER_OK;

	if (player->media_play->demuxer) {
		GxDemuxer_Control(player->media_play->demuxer, GX_DEMUXER_CTRL_SUB_PSI_ON, &subblock->subpara.pid);
		subblock->substatus = GXPLAYER_SUB_RUNNING;
		GxScteSubtitle_Start(subblock->sub_handle);
	}

	return GX_PLAYER_OK;
}

static int _scte_stop(PISubBlock* subblock)
{
	GxPlayer* player = subblock->player;

	if (!player || !player->media_play)
		return GX_PLAYER_ERROR;

	if (subblock->substatus == GXPLAYER_SUB_STOPPED)
		return GX_PLAYER_OK;

	if (player->media_play->demuxer) {
		subblock->substatus = GXPLAYER_SUB_STOPPED;
		GxScteSubtitle_Stop(subblock->sub_handle);
		GxDemuxer_Control(player->media_play->demuxer, GX_DEMUXER_CTRL_SUB_PSI_OFF, NULL);
	}

	return GX_PLAYER_OK;
}

static int _scte_read_packet(void *priv, unsigned char *packet, unsigned int packet_length)
{
	PISubBlock *subblock = (PISubBlock *)priv;
	GxPlayer   *player   = subblock->player;

	if (!player || !player->media_play || !player->media_play->demuxer)
		return -1;

	return GxDemuxer_ReadPsiData(player->media_play->demuxer, packet, packet_length);
}

int _pts_probe(GxMedia *media, int64_t pts)
{
#define MAX_DISTANCE (100000)
	int32_t start = 0, current = 0;

	current = GxMedia_GetCurrentPts(media);
	if (current == 0)
		return -1;

	if (pts == -1)
		return -1;

	if (media->demuxer->type == GX_DEMUXER_TYPE_HW ||
			media->demuxer->type == GX_DEMUXER_TYPE_HW_TS) {
		start  = (int32_t)(pts>>1);
		start /= 45;
	} else {
		start = (int32_t)(pts/90.0f);
	}

	if (abs(start-current) >= MAX_DISTANCE)
		return 1;

	return 0;
}

static GxScteSubtitleSyncState _scte_sync_pts(void *priv, int64_t pts)
{
	int ret = 0;
	PISubBlock *subblock = (PISubBlock *)priv;
	GxPlayer   *player   = subblock->player;

	if (!player || !player->media_play)
		return GX_SCTE_SYNC_ERROR;

	ret = _pts_probe(player->media_play, pts);
	if (ret < 0)
		return GX_SCTE_SYNC_ERROR;

	if (ret > 0)
		pts += 0x100000000;

	return (GxScteSubtitleSyncState)sub_pts_sync(player->media_play, pts, subblock->delay);
}

static PISubBlock* scte_open(PISubBlock* subblock, PISubParam* subpara)
{
	GxScteSubtitleParam param;

	if (!subblock || !subpara)
		return NULL;

	param.priv        = (void*)(subblock);
	param.read_packet = _scte_read_packet;
	param.sync_pts    = _scte_sync_pts;
	subblock->sub_handle = GxScteSubtitle_Open(&param);
	memcpy(&subblock->subpara, subpara, sizeof(PISubParam));

	if (_scte_start(subblock) != GX_PLAYER_OK)
		return NULL;

	return subblock;
}

static void scte_close(PISubBlock* subblock)
{
	_scte_stop(subblock);
	GxScteSubtitle_Close(subblock->sub_handle);
	subblock->sub_handle = 0;

	return;
}

static void scte_hide(PISubBlock* subblock)
{
	GxScteSubtitle_Hide(subblock->sub_handle);
}

static void scte_show(PISubBlock* subblock)
{
	GxScteSubtitle_Show(subblock->sub_handle);
}

static void scte_sync(PISubBlock* subblock,int32_t timems)
{
	subblock->delay = timems;
}

static void scte_pause(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		_scte_stop(subblock);

	return;
}

static void scte_resume(PISubBlock* subblock)
{
	if (subblock && subblock->sub_handle)
		_scte_start(subblock);

	return;
}

static void scte_switch_stream(PISubBlock* subblock, PlayerSubPID pid)
{
}

static void scte_switch_render(PISubBlock* subblock,PlayerSubRender render)
{

}

static void scte_switch_resolution(PISubBlock* subblock,VideoOutputMode mode)
{

}

PISubCtrl subctrl_scte = {
	.type =  PLAYER_SUB_TYPE_SCTE,
	.name = "SCTE SUBTITLE",

	.open   = scte_open,
	.close  = scte_close,
	.hide   = scte_hide,
	.show   = scte_show,
	.sync   = scte_sync,
	.pause  = scte_pause,
	.stop   = scte_pause,
	.resume = scte_resume,
	.switch_stream     = scte_switch_stream,
	.switch_render     = scte_switch_render,
	.switch_resolution = scte_switch_resolution,
};
