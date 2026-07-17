#include "so.h"
#include "gx_subtitle.h"
#include "gx_txt_subtitle.h"

static int32_t s_pid = 0;
static int32_t s_delay_ms = 0;

static void sub_pp_data_text(char* txt, GxSubPage* sub) {
	char* so,*de,*start,*pos;

	so=de=pos=txt;
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
			de++; so++;
			sub->text[sub->lines] = pos;
			sub->lines++;
			pos = de;
		}else if(*so == '\\' && (so[1] == 'N' || so[1] == 'n')){
			*de = '\0';
			de++;
			so += 2;
			sub->text[sub->lines] = pos;
			sub->lines++;
			pos = de;
		}else if(*so) {
			*de=*so;
			so++; de++;
		}
	}
	*de=*so;
	sub->text[sub->lines] = pos;
	sub->lines++;
}

static void sub_text_run_thread(void* filter)
{
	GxSubOut* sub_out = GXSOUT(filter);
	GxMediaFilter* mf = GXMEDIAFILTER(filter);
	GxPin *in_pin;
	GxFifo* fifo = NULL;
	GxSub sf;
	char* buf = NULL;
	size_t len;
	uint64_t cur = 0, last = 0;
	int32_t is_draw = 0;

	in_pin = GxMediaFilter_FindPin(filter, GX_SO_PIN_NAME_IN);

	while (sub_out->running &&
			in_pin && in_pin->source && in_pin->source->fifo) {
		if (mf->status != GX_MFT_STATE_RUNNING) {
			GxCore_ThreadDelay(100);
			continue;
		}

		fifo = in_pin->source->fifo;
		len  = GxFifo_GetLength(fifo);
		if (len <= 0) {
			if (in_pin->source->filter->status == GX_MFT_STATE_STOPPED) {
				GxMediaFilter_Stop(filter);
				GxSubOut_Close(sub_out);
				return  ;
			}
			GxCore_ThreadDelay(100);
			continue;
		}

		//gxlogf("[Player]%s[%d]:%s RUN!",__FILE__,__LINE__,__FUNCTION__);
		GxFifo_Read(fifo, (unsigned char*)(&sf), sizeof(GxSub), -1);
		if (sf.size && (sf.magic==GX_SUB_MAGIC)) {
			buf = av_mallocz(sf.size+1);
			if (buf == NULL) {
				GxFifo_Rollback(fifo, sf.size);
				//gxlogf("[Player]%s[%d]:%s MALLOC FAILED!",__FILE__,__LINE__,__FUNCTION__);
				continue;
			}
			memset(buf,'\0',sf.size+1);
			GxFifo_Read(fifo, (unsigned char*)buf, sf.size, -1);
			if (sf.pid != s_pid) {
				av_free(buf);
				continue;
			}
			if ((sf.endpts == -1) || (sf.startpts == -1)) {
				av_free(buf);
				continue;
			}
			if (s_delay_ms < 0) {
				cur = sub_out->cb(sub_out->args);
				if (((int32_t)cur - s_delay_ms) > (int32_t)sf.startpts) {
					int32_t dis_ms = cur - sf.startpts - s_delay_ms;

					sf.startpts += dis_ms;
					sf.endpts   += dis_ms;
					gxlogi("subtitle max delay :%d\n", -dis_ms);
				}
			}
			while (sub_out->running && mf->status != GX_MFT_STATE_STOPPED) {
				if ((sub_out->cb == NULL)||(sub_out->args == NULL))
					break;
				cur = sub_out->cb(sub_out->args);
				if ((cur == 0) || ((int32_t)cur < s_delay_ms)) {
					GxCore_ThreadDelay(200);
					continue;
				}
				cur = ((int32_t)cur > s_delay_ms) ? (cur - s_delay_ms) : 0;

				if (last > cur) {
					if (is_draw) {
						GxTxtSubtitle_Clean(sub_out->sub_handle);
						is_draw = 0;
					}
					if (!is_draw) {
						GxSubPage sub_pg;
						sub_pg.lines = 0;
						sub_pg.start = sf.startpts;
						sub_pg.end = sf.endpts;
						sub_pg.encode = SUB_ENC_UTF8;
						sub_pp_data_text(buf,&sub_pg);
						sub_pg.next = NULL;
						GxTxtSubtitle_Draw(sub_out->sub_handle, (GxTxtSubtitleContent *)&sub_pg, 1, SUB_ENC_UTF8);
						is_draw = 1;
					}
					GxCore_ThreadDelay(200);
					GxTxtSubtitle_Clean(sub_out->sub_handle);
					is_draw = 0;
					last = cur;
					break;
				}
				last = cur;
				if ((sf.startpts <= cur)&&((cur <= sf.endpts)||(sf.endpts == 0))) {
					if (is_draw == 0) {
						GxSubPage sub_pg;
						sub_pg.lines = 0;
						sub_pg.start = sf.startpts;
						sub_pg.end = sf.endpts;
						sub_pg.encode = SUB_ENC_UTF8;
						sub_pp_data_text(buf,&sub_pg);
						sub_pg.next = NULL;
						GxTxtSubtitle_Draw(sub_out->sub_handle, (GxTxtSubtitleContent *)&sub_pg, 1, SUB_ENC_UTF8);
						//gxlogf("draw -- cur:%d start:%d end:%d text:%s\n",(uint32_t)cur,(uint32_t)sf.startpts,(uint32_t)sf.endpts,buf);
						if(sf.endpts == 0)
							break;
						is_draw = 1;
					}

					if (is_draw && (sf.endpts == 0)) {
						GxTxtSubtitle_Clean(sub_out->sub_handle);
						is_draw = 0;
					}
				} else if((cur > sf.endpts) && (sf.endpts != 0)) {
					if (is_draw) {
						GxTxtSubtitle_Clean(sub_out->sub_handle);
						is_draw = 0;
					}
					break;
				}
				GxCore_ThreadDelay(100);
			}
			av_free(buf);
		}
		else
			gxlogf("[Player]%s[%d]:%s ERROR! magic=0x%x sf.size=0x%x\n",__FILE__,__LINE__,__FUNCTION__,sf.magic,sf.size);
	}

	if (is_draw) {
		GxTxtSubtitle_Clean(sub_out->sub_handle);
		is_draw = 0;
	}
	gxlogf("[Player]%s[%d]:%s EXIT!",__FILE__,__LINE__,__FUNCTION__);
}

static int sub_text_init(GxSubOut* so)
{
	if (!so->running) {
		so->sub_handle = GxTxtSubtitle_Init(NULL, GX_TXT_RENDER_SPP);
		so->running = 1;
		GxCore_ThreadCreate("so_text_thread", &so->run_pthread,
				sub_text_run_thread, (void*)so, 8*1024, GXOS_DEFAULT_PRIORITY);
	}

	return GX_PLAYER_OK;
}

static int sub_text_uinit(GxSubOut* so)
{
	if (so->running) {
		GxPin* in_pin = NULL;
		GxMediaFilter* filter = &so->filter;

		so->running = 0;
		GxCore_ThreadJoin(so->run_pthread);

		in_pin = GxMediaFilter_FindPin(filter, GX_SO_PIN_NAME_IN);
		if(in_pin && in_pin->source && in_pin->source->fifo)
			GxFifo_Reset(in_pin->source->fifo);

		GxTxtSubtitle_Clean(so->sub_handle);

		GxTxtSubtitle_Destory(so->sub_handle);
	}

	return GX_PLAYER_OK;
}

static int so_text_run(GxMediaFilter*  filter)
{
	GxSubOut* sub_out = GXSOUT(filter);
	gxlogf("%s %d\n",__FUNCTION__,__LINE__);

	if (sub_out) {
		sub_text_init(sub_out);
		filter->status = GX_MFT_STATE_RUNNING;
		return 0;
	}

	return -1;
}

static int so_text_stop(GxMediaFilter*  filter)
{
	GxSubOut* sub_out = GXSOUT(filter);
	gxlogf("%s %d\n",__FUNCTION__,__LINE__);

	if(sub_out){
		sub_text_uinit(sub_out);
		return GX_PLAYER_OK;
	}
	return GX_PLAYER_ERROR;
}

static int so_text_pause(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int so_text_resume(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int so_text_config(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int so_text_open(GxSubOut*  so)
{
	if(so&&so->header){
		char type = so->header->type;
		gxlogf("[Playe]:%s[%d]:%s, type = '%c'\n",__FILE__,__LINE__,__FUNCTION__,type);
		if((type == 'a') || (type == 't') ||
				(type == 'A') || (type == 'T') ||
				(type == 'm') || (type == 'M') ||
				(type == 'l') || (type == 'L'))
			return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

static int so_text_close(GxSubOut*  so)
{
	if(so){
		char type = so->type;
		gxlogf("[Playe]:%s[%d]:%s, type = '%c'\n",__FILE__,__LINE__,__FUNCTION__,type);
		if((type == 'a')||(type == 't')||(type == 'A')||(type == 'T') ||(type == 'l')||(type == 'L'))
			return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

static int so_text_control(GxSubOut*  so, int cmd, void *args)
{
	gxlogf("%s %d\n",__FUNCTION__,__LINE__);
	switch(cmd){
		case GX_SO_CTRL_SET_SYNC_TIME:
		{
			s_delay_ms = *(int32_t*)args;
			break;
		}
		case GX_SO_CTRL_EXE_INIT_FUNC:
		{
			s_delay_ms = 0;
			sub_text_init(so);
			break;
		}
		case GX_SO_CTRL_EXE_DESTORY_FUNC:
		{
			sub_text_uinit(so);
			break;
		}
		case GX_SO_CTRL_EXE_SHOW_FUNC:
		{
			GxTxtSubtitle_Show(so->sub_handle);
			break;
		}
		case GX_SO_CTRL_EXE_HIDE_FUNC:
		{
			GxTxtSubtitle_Hide(so->sub_handle);
			break;
		}
		case GX_SO_CTRL_SET_PID:
		{
			s_pid = *(int32_t*)args;
			break;
		}
		default:
			break;
	}
	return GX_PLAYER_OK;
}

static int so_text_play(GxSubOut*  so, void *data, int len, int flags)
{
	return GX_PLAYER_OK;
}

GxSubOutClass gx_sout_text = {
	._inherit = {          // GxMediaFilter
		._inherit = {  // GxObject
			.name    = "Sout Text",
			.parent  = &gx_SubOutBase,
			.size    = sizeof(GxSubOut),
			.create  = NULL,
			.release = NULL,
		},
		.run          = so_text_run,
		.pause        = so_text_pause,
		.resume       = so_text_resume,
		.config       = so_text_config,
		.stop         = so_text_stop,
	},
	DEF_AUTHOR("Sub Text Out","sw","No description","L.F","No comment"),

	.name    = "SW Sub Text Out",
	.open    = so_text_open,
	.close   = so_text_close,
	.control = so_text_control,
	.play    = so_text_play,
};
