#include "gx_media.h"
#include "gx_avout.h"
#include "avstring.h"
#include "../gxplayer_internal.h"
#include "gx_txt_subtitle.h"
#include "gx_pixel_subtitle.h"

#ifdef CONFIG_SUBTITLE_MEDIA

#undef CONFIG_SUBTITLE_PLAYER_DRAW

static void _sub_draw(PISubBlock* subblock, uint32_t* start, uint32_t *end)
{
	GxSubData* sub = (GxSubData*)subblock->subint;

	if (sub->encode == SUB_ENC_SPU) {
		PlayerSubSpuPage spu;
		GxSubReader_GetPacket(sub,NULL,NULL, NULL,NULL);
		if(sub->spu == NULL)
			return;
		memcpy(&spu,sub->spu,sizeof(PlayerSubSpuPage));
		if((sub->vob->spu_streams->packet)&&
				((*end-*start) > sub->vob->spu_streams->packet->pts100))
			*end = *start + sub->vob->spu_streams->packet->pts100;
#ifdef CONFIG_SUBTITLE_PLAYER_DRAW
		GxSubRender_Draw(sub);
#else
		GxPixelSubtitle_Draw(subblock->sub_handle, (GxPixelSubtitleContent *)(&spu), 1, sub->encode);
#endif
	} else {
		GxTxtSubtitle_Draw(subblock->sub_handle,
				(GxTxtSubtitleContent *)(sub->sub_hdr), sub->sub_num, sub->sub_hdr->encode);
	}
	return;
}

static void _sub_clear(PISubBlock* subblock)
{
	GxSubData* sub = (GxSubData*)subblock->subint;

	if(sub->encode == SUB_ENC_SPU) {
#ifdef CONFIG_SUBTITLE_PLAYER_DRAW
		GxSubRender_Clean(NULL);
#else
		GxPixelSubtitle_Clean(subblock->sub_handle);
#endif
	} else {
		GxTxtSubtitle_Clean(subblock->sub_handle);
	}
	return;
}

static uint64_t _sub_timer_gettime(PISubBlock *subblock)
{
	uint64_t u64cur = 0;
	GxPlayer* player = (GxPlayer*)subblock->player;

	if (subblock->subpara.use_ticktime) {
		u64cur  = (uint64_t)av_get_tick(NULL, 0);
		u64cur -= subblock->sub_timer.start_timems;
		u64cur += subblock->sub_timer.goto_timems;
	} else
		gxplayer_get_time(player, &u64cur, NULL, NULL);

	return u64cur;
}

static void _sub_timer_start(PISubBlock *subblock, uint64_t goto_timems)
{
	if (subblock->subpara.use_ticktime) {
		subblock->sub_timer.start_timems = av_get_tick(NULL, 0);
		subblock->sub_timer.goto_timems  = goto_timems;
	}

	return;
}

static void sub_pp_data_text(GxSubPage* sub)
{
	char* so,*de,*start,*pos;
	int lines = 0;

	if (sub == NULL)
		return;

	for (lines = 0; lines < sub->lines; lines++) {
		so=de=pos=sub->text[lines];
		while (*so) {
			if(*so == '<')
			{
				for(start=so; *so && *so!='>'; so++);
				if(*so) so++; else{ so=start;break;}
			}else if(*so == '{' && so[1]=='\\'){
				for(start=so; *so && *so!='}'; so++);
				if(*so) so++; else{ so=start;break;}
			}else if(*so == '\n'){
				*de = '\0';
				de++;
				so++;
			}else if(*so == '\\' && (so[1] == 'N' || so[1] == 'n')){
				*de = '\0';
				de++;
				so += 2;
			}else if(*so) {
				*de=*so;
				so++; de++;
			}
		}
		*de=*so;
		sub->text[lines] = pos;
	}
}

static void player_sub_outside(void* data)
{
	int32_t ret;
	int32_t is_draw = 0;
	uint32_t start,end;
	uint32_t current=0,last = 0, old_current = 0;

	PISubBlock* subblock = (PISubBlock*)data;
	GxPlayer* player = (GxPlayer*)subblock->player;
	GxSubData* sub = (GxSubData*)subblock->subint;

	if(!sub||!player)
		return;

	while(subblock->substatus != GXPLAYER_SUB_STOPPED)
	{
		if (0) {
delay_again:
			if (subblock->substatus == GXPLAYER_SUB_STOPPED)
				break;
			GxCore_ThreadDelay(100);
		}

		if (subblock->substatus == GXPLAYER_SUB_PAUSED)
			goto delay_again;

		current = (uint32_t)_sub_timer_gettime(subblock);
		if (current == 0)
			goto delay_again;

		current = ((int32_t)current > subblock->delay) ? (current - subblock->delay) : 0;
		if (old_current == current)
			goto delay_again;

		old_current = current;
		if(last > current){
			GxSubReader_Reset_Chr(sub);
			last = current;
		}
		ret = GxSubReader_Get_ByTime(sub, current, 0, &start, &end);
		if (ret != 0) {
			last = current;
			goto delay_again;
		}

		if(sub->encode != SUB_ENC_SPU)
			sub_pp_data_text(sub->sub_hdr);

		while (1) {
			int debug_sub_sync = 0;
			GxPlayer_SystemGet(PSYS_DEBUG_SUBTITLE_SYNC, &debug_sub_sync);

			if (subblock->substatus != GXPLAYER_SUB_RUNNING) {
				GxSubReader_Free_Chr(sub);
				break;
			}

			if (last > current) {
				int speed = abs(player->speed)/1000;

				speed = speed > 0 ? speed : 1;
				if (current + 200 <= start) {
					if (is_draw)
						_sub_clear(subblock);
					GxCore_ThreadDelay(100);
				} else {
					if (!is_draw) {
						_sub_draw(subblock, &start, &end);
					}
					GxCore_ThreadDelay(abs(end-start)/speed/10*10 + 10);
					_sub_clear(subblock);
				}

				GxSubReader_Free_Chr(sub);
				GxSubReader_Reset_Chr(sub);
				is_draw = 0;
				last = current;
				break;
			}

			last = current;
			current = (uint32_t)_sub_timer_gettime(subblock);
			if (current == 0) {
				current = last;
				GxCore_ThreadDelay(10);
				continue;
			}

			current = ((int32_t)current > subblock->delay) ? (current - subblock->delay) : 0;
			if (is_draw && ((end <= current) || (current < start))) {
				is_draw = 0;
				if (debug_sub_sync)
					gxlogi("[subtitle]-[Clear] start:%d end:%d cur:%d\n", start, end, current);
				_sub_clear(subblock);
				GxSubReader_Free_Chr(sub);
				break;
			}else if(!is_draw && (start <= current)){
				is_draw = 1;
				if (debug_sub_sync)
					gxlogi("[subtitle]-[Draw ] start:%d end:%d cur:%d\n", start, end, current);
				_sub_draw(subblock, &start, &end);
			}
			GxCore_ThreadDelay(100);
		}
	}

	if(is_draw)
		_sub_clear(subblock);
	GxSubReader_Free_Chr(sub);

	gxlogf("exit player_sub_draw_thread\n");
}

static PISubBlock* player_sub_open_outside(PISubBlock* subblock, PISubParam* subpara)
{
	int i,ret;
	GxSubData* data=NULL ;
	PlayerWindow window;

	if (subpara == NULL)
		return NULL;

	data = GxSubReader_Open(subblock->lang, subpara->url);
	if(data==NULL)
		return NULL;

	subblock->subint = data;
	subblock->subout->sub_hdr = (PlayerSubTextPage*)data->sub_hdr;
	subblock->subout->sub_num = data->sub_track;

	if(subblock->subout->sub_num > PLAYER_MAX_TRACK_SUB)
		subblock->subout->sub_num = PLAYER_MAX_TRACK_SUB;

	for(i=0;i<subblock->subout->sub_num;i++)
	{
		subblock->subout->track[i].id = i;
		subblock->subout->track[i].pid.pid   = i;
		subblock->subout->track[i].pid.major = 0;
		subblock->subout->track[i].pid.minor = 0;
		if(data->sub_lang[i])
			snprintf (subblock->subout->track[i].lang, sizeof(subblock->subout->track[i].lang), "%s", data->sub_lang[i]);
	}

	if (data->encode == SUB_ENC_SPU) {
		window.x = data->vob->origin_x;
		window.y = data->vob->origin_y;
		window.width = data->vob->orig_frame_width;
		window.height = data->vob->orig_frame_height;
		subblock->delay = data->vob->delay;
	} else {
		subblock->delay = 0;
	}

	if (data->encode == SUB_ENC_SPU) {
#ifdef CONFIG_SUBTITLE_PLAYER_DRAW
		GxSubRender_Init(SUB_RENDER_SPP,&window);
#else
		subblock->sub_handle = GxPixelSubtitle_Init((GxPixelSubtitleWindow *)(&window), GX_PIXEL_RENDER_SPP);
#endif
	} else {
		subblock->sub_handle = GxTxtSubtitle_Init((GxTxtSubtitleWindow *)(&window), GX_TXT_RENDER_OSD);
	}

	if (subpara->use_ticktime) {
	}

	subblock->subpara.use_ticktime = subpara->use_ticktime;
	_sub_timer_start(subblock, 0);
	subblock->substatus = GXPLAYER_SUB_RUNNING;
	ret = GxCore_ThreadCreate("outside_sub_thread", \
			&subblock->sub_thread, player_sub_outside, subblock, 16*1024, GXOS_DEFAULT_PRIORITY);
	if(0 != ret)
	{
		GxSubReader_Close(subblock->subint);
		return NULL;
	}

	return subblock;
}

static void player_sub_close_outside(PISubBlock* subblock)
{
	subblock->substatus = GXPLAYER_SUB_STOPPED;
	GxCore_ThreadJoin(subblock->sub_thread);

	if (subblock->subint->encode == SUB_ENC_SPU) {
#ifdef CONFIG_SUBTITLE_PLAYER_DRAW
		GxSubRender_Destory();
#else
		GxPixelSubtitle_Destory(subblock->sub_handle);
#endif
	} else {
		GxTxtSubtitle_Destory(subblock->sub_handle);
	}

	GxSubReader_Close(subblock->subint);
	return;
}

static void player_sub_pause_outside(PISubBlock* subblock)
{
	subblock->substatus = GXPLAYER_SUB_PAUSED;
}

static void player_sub_resume_outside(PISubBlock* subblock)
{
	subblock->substatus = GXPLAYER_SUB_RUNNING;
}

static void player_sub_hide_outside(PISubBlock* subblock)
{
	GxSubData* data = subblock->subint;

	if (data->encode == SUB_ENC_SPU) {
#ifdef CONFIG_SUBTITLE_PLAYER_DRAW
		GxSubRender_Hide(data);
#else
		GxPixelSubtitle_Hide(subblock->sub_handle);
#endif
	} else {
		GxTxtSubtitle_Hide(subblock->sub_handle);
	}
}

static void player_sub_show_outside(PISubBlock* subblock)
{
	GxSubData* data = subblock->subint;

	if (data->encode == SUB_ENC_SPU) {
#ifdef CONFIG_SUBTITLE_PLAYER_DRAW
		GxSubRender_Show(data);
#else
		GxPixelSubtitle_Show(subblock->sub_handle);
#endif
	} else {
		GxTxtSubtitle_Show(subblock->sub_handle);
	}
}

static void player_sub_sync_outside(PISubBlock* sb,int32_t timems)
{
	sb->delay = timems;
}

static void player_sub_switch_stream_outside(PISubBlock* subblock,PlayerSubPID pid)
{
	GxSubData* data=subblock->subint ;

	if(data->encode == SUB_ENC_SPU){
		GxSubReader_SwitchStream(data,pid.pid);
	}
}

static void player_sub_switch_render_outside(PISubBlock* sb,PlayerSubRender render)
{

}

static void player_sub_config_outside(PISubBlock* sb, void* config)
{
}

static void player_sub_get_info_outside(PISubBlock* sb, PlayerProgInfo* info)
{
	int i;
	GxSubInfo sub_info;
	GxSubData* data = sb->subint;
	memset(&sub_info,0,sizeof(GxSubInfo));
	if(data)
	{
		GxSubReader_GetInfo(data,&sub_info);
		info->cur_sub_id = sub_info.cur_id;
		info->sub_num = sub_info.num;
		for(i=0;i<sub_info.num;i++)
		{
			info->sub[i].id        = sub_info.id[i];
			info->sub[i].pid.pid   = sub_info.id[i];
			info->sub[i].pid.major = 0;
			info->sub[i].pid.minor = 0;
			info->sub[i].encode = sub_info.encode;
			info->sub[i].type   = PLAYER_SUB_TYPE_FILE;
			snprintf (info->sub[i].codec_id, sizeof(info->sub[i].codec_id), "%s", sub_info.codec_id[i]);
			snprintf (info->sub[i].track_name, sizeof(info->sub[i].track_name), "%s", sub_info.track_name[i]);
			snprintf (info->sub[i].lang, sizeof(info->sub[i].lang), "%s", sub_info.lang[i]);
		}
	}
}

void player_sub_get_timeinfo(PISubBlock* sb, uint64_t *curtime, uint64_t *duration)
{
	GxSubData* sub = (GxSubData*)sb->subint;

	if (duration)
		*duration = (uint64_t)sub->sub_duration;

	if (curtime)
		*curtime = _sub_timer_gettime(sb);
}

void player_sub_goto_localtime(PISubBlock* sb, uint64_t timems)
{
	GxSubData* sub = (GxSubData*)sb->subint;

	sb->substatus = GXPLAYER_SUB_STOPPED;
	GxCore_ThreadJoin(sb->sub_thread);
	GxSubReader_Reset_Chr(sub);
	_sub_timer_start(sb, timems);
	sb->substatus = GXPLAYER_SUB_RUNNING;
	GxCore_ThreadCreate("outside_sub_thread", \
			&sb->sub_thread, player_sub_outside, sb, 16*1024, GXOS_DEFAULT_PRIORITY);
}

PISubCtrl subctrl_outside = {
	.type = PLAYER_SUB_TYPE_FILE,
	.name = "PLAYER_SUB_TYPE_OUTSIDE",

	.open  = player_sub_open_outside,
	.close = player_sub_close_outside,
	.hide  = player_sub_hide_outside,
	.show  = player_sub_show_outside,
	.sync  = player_sub_sync_outside,
	.pause = player_sub_pause_outside,
	.stop  = player_sub_pause_outside,
	.resume = player_sub_resume_outside,
	.switch_stream  = player_sub_switch_stream_outside,
	.switch_render  = player_sub_switch_render_outside,
	.config         = player_sub_config_outside,
	.get_info       = player_sub_get_info_outside,
	.get_timeinfo   = player_sub_get_timeinfo,
	.goto_localtime = player_sub_goto_localtime,
};
#endif


