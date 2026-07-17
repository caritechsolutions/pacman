#include "gx_media.h"
#include "gx_avout.h"
#include "../gxplayer_internal.h"
#include "gx_txt_subtitle.h"

#ifdef CONFIG_SUBTITLE_MEDIA

static PISubBlock* player_sub_open_inside(PISubBlock* subblock, PISubParam* subpara)
{
	GxPlayer* player=(GxPlayer*)subblock->player;

	if(player->media_play)
	{
		subblock->subout->sub_hdr = av_mallocz(sizeof(GxSubPage));
		GxSubOut_Control(player->media_play->sout,     GX_SO_CTRL_EXE_INIT_FUNC, NULL);
		GxSubOut_Control(player->media_play->sout,     GX_SO_CTRL_SET_PID,       &subpara->pid.pid);
		GxDemuxer_Control(player->media_play->demuxer, GX_DEMUXER_CTRL_SUB_ON,   &subpara->pid.pid);
		if (player->media_play) {
			GxMedia_SubSwitch(player->media_play, subpara->pid.pid);
		}

		return subblock;
	}
	return NULL;
}

static void player_sub_close_inside(PISubBlock* subblock)
{
	GxPlayer*  player=(GxPlayer*)subblock->player;

	if(player->media_play)
	{
		if(subblock->subout->sub_hdr) {
			av_free(subblock->subout->sub_hdr);
			subblock->subout->sub_hdr = NULL;
		}
		GxDemuxer_Control(player->media_play->demuxer, GX_DEMUXER_CTRL_SUB_OFF,     NULL);
		GxSubOut_Control(player->media_play->sout,     GX_SO_CTRL_EXE_DESTORY_FUNC, NULL);
	}
}

static void player_sub_hide_inside(PISubBlock* subblock)
{
	GxPlayer*  p=(GxPlayer*)subblock->player;
	if (p->media_play) {
		GxSubOut_Control(p->media_play->sout,GX_SO_CTRL_EXE_HIDE_FUNC,NULL);
	}
}

static void player_sub_show_inside(PISubBlock* subblock)
{
	GxPlayer*  p=(GxPlayer*)subblock->player;
	if (p->media_play) {
		GxSubOut_Control(p->media_play->sout,GX_SO_CTRL_EXE_SHOW_FUNC,NULL);
	}
}

static void player_sub_sync_inside(PISubBlock* subblock,int32_t timems)
{
	GxPlayer*  p=(GxPlayer*)subblock->player;
	if (p->media_play) {
		GxSubOut_Control(p->media_play->sout,GX_SO_CTRL_SET_SYNC_TIME,&timems);
	}
}

static void player_sub_pause_inside(PISubBlock* subblock)
{
	GxPlayer*  p=(GxPlayer*)subblock->player;
	if(p->media_play)
	{
		GxDemuxer_Control(p->media_play->demuxer,GX_DEMUXER_CTRL_SUB_OFF, NULL);
	}
}

static void player_sub_resume_inside(PISubBlock* subblock)
{
	GxPlayer*  p=(GxPlayer*)subblock->player;
	if(p->media_play)
	{
		GxDemuxer_Control(p->media_play->demuxer,GX_DEMUXER_CTRL_SUB_ON, &subblock->subpara.pid.pid);
	}
}

static void player_sub_switch_stream_inside(PISubBlock* subblock,PlayerSubPID pid)
{
	GxPlayer*  p=(GxPlayer*)subblock->player;
	if(p->media_play)
	{
		GxMedia_SubSwitch(p->media_play,pid.pid);
	}
}

static void player_sub_switch_render_inside(PISubBlock* subblock,PlayerSubRender render)
{

}

PISubCtrl subctrl_inside = {
	.type = PLAYER_SUB_TYPE_INSIDE,
	.name = "PLAYER_SUB_TYPE_INSIDE",

	.open  = player_sub_open_inside,
	.close = player_sub_close_inside,
	.hide  = player_sub_hide_inside,
	.show  = player_sub_show_inside,
	.sync  = player_sub_sync_inside,
	.pause = player_sub_pause_inside,
	.stop  = player_sub_pause_inside,
	.resume = player_sub_resume_inside,
	.switch_stream  = player_sub_switch_stream_inside,
	.switch_render  = player_sub_switch_render_inside,
	.get_info = NULL,
};
#endif
