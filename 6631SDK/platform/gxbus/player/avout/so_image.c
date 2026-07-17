#include "so.h"
#include "gx_pixel_subtitle.h"

static int32_t s_pid = 0;
static int32_t s_delay_ms = 0;

#undef CONFIG_SUBTITLE_PLAYER_DRAW

static void so_image_run_thread(void* filter);
static int sub_image_init(GxSubOut* so)
{
	PlayerWindow window;
	window.x = 0;
	window.y = 0;
	window.width = 720;
	window.height = 576;
	if(so->header->extradata){
		sscanf((const char*)so->header->extradata, "size: %dx%d", &window.width, &window.height);
	}

	if (!so->running) {
#ifdef CONFIG_SUBTITLE_PLAYER_DRAW
		GxSubRender_Init(SUB_RENDER_SPP,&window);
#else
		so->sub_handle = GxPixelSubtitle_Init(NULL, GX_PIXEL_RENDER_SPP);
#endif
		so->running = 1;
		GxCore_ThreadCreate("so_image_thread", &so->run_pthread,
				so_image_run_thread, (void*)so, 8*1024, GXOS_DEFAULT_PRIORITY);
	}


	return GX_PLAYER_OK;
}

static int sub_image_uinit(GxSubOut* so)
{
	if (so->running) {
		GxPin* in_pin = NULL;
		GxMediaFilter* filter = &so->filter;

		so->running = 0;
		GxCore_ThreadJoin(so->run_pthread);

		in_pin = GxMediaFilter_FindPin(filter, GX_SO_PIN_NAME_IN);
		if(in_pin && in_pin->source && in_pin->source->fifo)
			GxFifo_Reset(in_pin->source->fifo);

		GxPixelSubtitle_Clean(so->sub_handle);

#ifdef CONFIG_SUBTITLE_PLAYER_DRAW
		GxSubRender_Destory();
#else
		GxPixelSubtitle_Destory(so->sub_handle);
#endif
	}

	return GX_PLAYER_OK;
}

static void so_image_run_thread(void* filter)
{
	GxSubOut* sub_out = GXSOUT(filter);
	GxMediaFilter* mf = GXMEDIAFILTER(filter);
	GxPin *in_pin;
	GxFifo* fifo = NULL;
	GxSub sf;
	size_t len,pdata_len;
	uint64_t cur = 0, last = 0;
	int32_t is_draw = 0;
	unsigned char* pdata = NULL;
	PlayerSubSpuPage spu;
	GxSubData sub;

	sub.spu =(void*)(&spu);
	sub.encode = SUB_ENC_SPU;
	in_pin = GxMediaFilter_FindPin(filter, GX_SO_PIN_NAME_IN);

	while (sub_out->running &&
			in_pin && in_pin->source && in_pin->source->fifo ) {
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
				return;
			}
			GxCore_ThreadDelay(100);
			continue;
		}

		gxlogf("[Player]%s[%d]:%s RUN!",__FILE__,__LINE__,__FUNCTION__);
		GxFifo_Read(in_pin->source->fifo, (unsigned char*)(&sf), sizeof(GxSub), -1);
		if ((sf.size==sizeof(PlayerSubSpuPage)) && (sf.magic==GX_SUB_MAGIC)) {
			GxFifo_Read(in_pin->source->fifo,(unsigned char*)(&spu), sf.size, -1);
			pdata_len = 2*(spu.image_size + spu.scaled_image_size);
			if (pdata_len > 0) {
				unsigned int read_size = 0;

				pdata = av_malloc(pdata_len);
				if (pdata == NULL) {
					GxFifo_Rollback(in_pin->source->fifo,pdata_len);
					gxlogf("[Player]%s[%d]:%s MALLOC FAILED!",__FILE__,__LINE__,__FUNCTION__);
					continue;
				}
				while (read_size < pdata_len
						&& sub_out->running &&
						mf->status != GX_MFT_STATE_STOPPED) {
					read_size += GxFifo_Read(in_pin->source->fifo,
							pdata + read_size, pdata_len - read_size, -1);
					GxCore_ThreadDelay(1);
				}

				if (spu.image_size) {
					spu.image = pdata;
					spu.aimage = spu.image+spu.image_size;
				} else{
					spu.image = NULL;
					spu.aimage = NULL;
				}

				if(spu.scaled_image_size) {
					spu.scaled_image = pdata + 2*spu.image_size;
					spu.scaled_aimage = spu.scaled_image + spu.scaled_image_size;
				} else{
						spu.scaled_image = NULL;
						spu.scaled_aimage = NULL;
				}
			} else {
				spu.image = NULL;
				spu.aimage = NULL;
				spu.scaled_image = NULL;
				spu.scaled_aimage = NULL;
			}
			if (sf.pid != s_pid) {
				if(pdata)
					av_free(pdata);
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
				if((sub_out->cb == NULL)||(sub_out->args == NULL))
					break;
				cur = sub_out->cb(sub_out->args);
				if ((cur == 0) || ((int32_t)cur < s_delay_ms)) {
					GxCore_ThreadDelay(200);
					continue;
				}
				cur = ((int32_t)cur > s_delay_ms) ? (cur - s_delay_ms) : 0;

				if (last > cur) {
					if (is_draw) {
#ifdef CONFIG_SUBTITLE_PLAYER_DRAW
						GxSubRender_Clean(&sub);
#else
						GxPixelSubtitle_Clean(sub_out->sub_handle);
#endif
						is_draw = 0;
					}
					if (!is_draw) {
#ifdef CONFIG_SUBTITLE_PLAYER_DRAW
						GxSubRender_Draw(sub);
#else
						GxPixelSubtitle_Draw(sub_out->sub_handle, (GxPixelSubtitleContent *)(&spu), 1, SUB_ENC_SPU);
#endif
						is_draw = 1;
					}
					GxCore_ThreadDelay(200);
#ifdef CONFIG_SUBTITLE_PLAYER_DRAW
					GxSubRender_Clean(&sub);
#else
					GxPixelSubtitle_Clean(sub_out->sub_handle);
#endif
					is_draw = 0;
					last = cur;
					break;
				}
				last = cur;

				if ((sf.startpts <= cur)&&((cur <= sf.endpts)||(sf.endpts == 0))) {
					if (is_draw == 0) {
						gxlogf("%s %d -------------- draw\n",__FUNCTION__,__LINE__);
#ifdef CONFIG_SUBTITLE_PLAYER_DRAW
						GxSubRender_Draw(sub);
#else
						GxPixelSubtitle_Draw(sub_out->sub_handle, (GxPixelSubtitleContent *)(&spu), 1, SUB_ENC_SPU);
#endif
						if(sf.endpts == 0)
							break;
						is_draw = 1;
					}

					if (is_draw && (sf.endpts == 0)) {
#ifdef CONFIG_SUBTITLE_PLAYER_DRAW
						GxSubRender_Clean(&sub);
#else
						GxPixelSubtitle_Clean(sub_out->sub_handle);
#endif
						is_draw = 0;
					}
				} else if((cur > sf.endpts) && (sf.endpts != 0)) {
					if (is_draw) {
#ifdef CONFIG_SUBTITLE_PLAYER_DRAW
						GxSubRender_Clean(&sub);
#else
						GxPixelSubtitle_Clean(sub_out->sub_handle);
#endif
						is_draw = 0;
					} else {
						if ((cur - sf.endpts) < 2000) {
							sf.endpts = cur + 500;
							continue;
						}
					}
					break;
				}
				GxCore_ThreadDelay(100);
			}
			if (pdata_len && pdata)
				av_free(pdata);
		}
		else
			gxlogf("[Player]%s[%d]:%s ERROR! magic=0x%x sf.size=0x%x\n",__FILE__,__LINE__,__FUNCTION__,sf.magic,sf.size);
	}

	if (is_draw) {
#ifdef CONFIG_SUBTITLE_PLAYER_DRAW
		GxSubRender_Clean(&sub);
#else
		GxPixelSubtitle_Clean(sub_out->sub_handle);
#endif
		is_draw = 0;
	}
	gxlogf("[Player]%s[%d]:%s EXIT!",__FILE__,__LINE__,__FUNCTION__);
}


static int so_image_run(GxMediaFilter*  filter)
{
	GxSubOut* sub_out = GXSOUT(filter);
	gxlogf("%s %d\n",__FUNCTION__,__LINE__);

	if(sub_out){
		sub_image_init(sub_out);
		filter->status = GX_MFT_STATE_RUNNING;
		return 0;
	}

	return -1;
}

static int so_image_stop(GxMediaFilter*  filter)
{
	GxSubOut* sub_out = GXSOUT(filter);
	gxlogf("%s %d\n",__FUNCTION__,__LINE__);

	if(sub_out){
		sub_image_uinit(sub_out);
		return GX_PLAYER_OK;
	}
	return GX_PLAYER_ERROR;
}

static int so_image_pause(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int so_image_resume(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int so_image_config(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int so_image_open(GxSubOut*  so)
{
	if(so&&so->header){
		char type = so->header->type;
		gxlogf("[Playe]:%s[%d]:%s, type = '%c'\n",__FILE__,__LINE__,__FUNCTION__,type);
		if((type == 'x')||(type == 'X')||(type == 'v')||(type == 'V'))
			return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

static int so_image_close(GxSubOut*  so)
{
	if(so){
		char type = so->type;
		gxlogf("[Playe]:%s[%d]:%s, type = '%c'\n",__FILE__,__LINE__,__FUNCTION__,type);
		if((type == 'x')||(type == 'X')||(type == 'v')||(type == 'V'))
			return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

static int so_image_control(GxSubOut*  so, int cmd, void *args)
{
	gxlogf("%s %d\n",__FUNCTION__,__LINE__);
	switch(cmd)
	{
		case GX_SO_CTRL_SET_SYNC_TIME:
		{
			s_delay_ms = *(int32_t*)args;
			break;
		}
		case GX_SO_CTRL_EXE_INIT_FUNC:
		{
			s_delay_ms = 0;
			sub_image_init(so);
			break;
		}
		case GX_SO_CTRL_EXE_DESTORY_FUNC:
		{
			sub_image_uinit(so);
			break;
		}
		case GX_SO_CTRL_EXE_SHOW_FUNC:
		{
#ifdef CONFIG_SUBTITLE_PLAYER_DRAW
			GxSubRender_Show(data);
#else
			GxPixelSubtitle_Show(so->sub_handle);
#endif
			break;
		}
		case GX_SO_CTRL_EXE_HIDE_FUNC:
		{
#ifdef CONFIG_SUBTITLE_PLAYER_DRAW
			GxSubRender_Hide(data);
#else
			GxPixelSubtitle_Hide(so->sub_handle);
#endif
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

static int so_image_play(GxSubOut*  so, void *data, int len, int flags)
{
	return GX_PLAYER_OK;
}

GxSubOutClass gx_sout_image = {
	._inherit = {		// GxMediaFilter
		._inherit     = {	// GxObject
			.name    = "Sout Image",
			.parent  = &gx_SubOutBase,
			.size    = sizeof(GxSubOut),
			.create  = NULL,
			.release = NULL,
		},
		.run          = so_image_run,
		.pause        = so_image_pause,
		.resume       = so_image_resume,
		.config       = so_image_config,
		.stop         = so_image_stop,
	},
	DEF_AUTHOR("Sub Image Out","sw","No description","L.F","No comment"),

	.name    = "SW Sub Image Out",
	.open    = so_image_open,
	.close   = so_image_close,
	.control = so_image_control,
	.play    = so_image_play,
};
