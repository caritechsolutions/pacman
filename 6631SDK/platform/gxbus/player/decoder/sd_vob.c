#include "sd.h"
#include "../avformat/avcodec.h"

static int sub_copy_spu(PlayerSubSpuPage* inspu, unsigned char* outspu)
{
	size_t size;
	unsigned char * pdata;

	if(inspu == NULL)
		return 0;
	memcpy(outspu,inspu,sizeof(PlayerSubSpuPage));
	pdata = outspu + sizeof(PlayerSubSpuPage);
	size = inspu->image_size;
	if(size > 0)
	{
		memcpy(pdata,inspu->image,size);
		memcpy(pdata+size,inspu->aimage,size);
		pdata += 2*size;
	}
	size = inspu->scaled_image_size;
	if(size > 0)
	{
		memcpy(pdata,inspu->scaled_image,size);
		memcpy(pdata+size,inspu->scaled_aimage,size);
	}
	return 0;
}

static size_t vobsub_decode(GxSub* in_sf, void* priv, unsigned char* in_data, unsigned char** out_data)
{
	PlayerSubSpuPage* spudec = priv;
	unsigned char* pdata = NULL;
	unsigned char* pbuf = NULL;
	int size;

	if(spudec == NULL)
		return 0;

	spudec_assemble(spudec,in_data,in_sf->size,in_sf->startpts);
	if (spudec->queue_head)
	{
		spudec_heartbeat(spudec, spudec->queue_head->start_pts);
		if (spudec_changed(spudec))
		{
			size = sizeof(GxSub)+sizeof(PlayerSubSpuPage)+2*(spudec->image_size+spudec->scaled_image_size);
			pdata = av_malloc(size);
			if(pdata == NULL)
				return 0;
			in_sf->size = sizeof(PlayerSubSpuPage);
			if ((spudec->end_pts <= spudec->start_pts) ||
					(spudec->end_pts >= spudec->start_pts + 900000))
				spudec->end_pts = spudec->start_pts + 900000;
			spudec->bitmap = 0;
			in_sf->startpts = spudec->start_pts;
			in_sf->endpts   = in_sf->startpts+(spudec->end_pts-spudec->start_pts)/90;
			memcpy(pdata,in_sf,sizeof(GxSub));
			pbuf = pdata + sizeof(GxSub);
			sub_copy_spu(spudec,pbuf);
			*out_data = pdata;
			return size;
		}
	}
	return 0;
}

static void xsub_right_window(unsigned int width,
		unsigned int height,
		unsigned int *def_width,
		unsigned int *def_height)
{
	if ((*def_width == 0) || (*def_height == 0)) {
		*def_width  = 720;
		*def_height = 576;
	}

	if ((width <= *def_width) && (height <= *def_height))
		return;
	else {
		if ((width > 1280) || (height > 720)) {
			*def_width  = 1920;
			*def_height = 1080;
		} else if ((width > 720) || (height > 576)) {
			*def_width  = 1280;
			*def_height = 720;
		} else {
			*def_width  = 720;
			*def_height = 576;
		}
	}

	return;
}

static size_t xsub_decode(GxSub* in_sf, void* priv, unsigned char* in_data, unsigned char** out_data)
{
	PlayerSubSpuPage* spudec = priv;
	AVSubtitle psubt;
	int got_frame = 0;
	extern int  xsub_decode_frame(int has_alpha, int64_t pts, void *in_data, int in_size, void *data, int *data_size);
	extern void xsub_decode_free(void *data);

	if(spudec == NULL)
		return 0;

	memset(&psubt, 0, sizeof(psubt));
	xsub_decode_frame(0, in_sf->startpts, in_data, in_sf->size, (void *)&psubt, &got_frame);
	if (got_frame) {
		unsigned int i = 0;
		unsigned int w = psubt.rects[0]->w + psubt.rects[0]->x;
		unsigned int h = psubt.rects[0]->h + psubt.rects[0]->y;
		unsigned int l = psubt.rects[0]->w * psubt.rects[0]->h;
		int size = sizeof(GxSub) + sizeof(PlayerSubSpuPage) + 2 * l;
		unsigned char *pdata = av_malloc(size);

		if (pdata) {
			xsub_right_window(w, h, &spudec->orig_frame_width, &spudec->orig_frame_height);
			for (i = 0; i < psubt.rects[0]->nb_colors; i++)
				spudec->global_palette[i] = ((uint32_t *)psubt.rects[0]->data[1])[i];
			spudec->image_size = l;
			spudec->scaled_image_size = 0;
			spudec->bitmap = 1;
			spudec->width  = psubt.rects[0]->w;
			spudec->height = psubt.rects[0]->h;
			in_sf->size = sizeof(PlayerSubSpuPage);
			in_sf->startpts = psubt.start_display_time;
			in_sf->endpts   = psubt.end_display_time;
			memcpy(pdata, in_sf, sizeof(GxSub));
			memcpy(pdata + sizeof(GxSub), spudec, sizeof(PlayerSubSpuPage));
			memcpy(pdata + sizeof(GxSub) + sizeof(PlayerSubSpuPage), psubt.rects[0]->data[0], l);
			*out_data = pdata;
		} else
			size = 0;
		xsub_decode_free((void *)&psubt);

		return size;
	}

	return 0;
}

static int sub_vob_init(GxAVCodec*  sd)
{
	int i;
	PlayerSubSpuPage *spudec  = NULL;
	GxStreamSubHeader* header = NULL;
	extern unsigned int spudec_palette_to_yuv(unsigned int pal);
	uint32_t pattle[16]={
		0x6d6d6d, 0x1f1f1f, 0xdadada, 0x000000,
		0x6d6d6d, 0xdadada, 0x000000, 0xea12eb,
		0xfaff1a, 0x000070, 0x067506, 0x731f03,
		0x095f78, 0xdadada, 0x7c950b, 0xffffff};
	if(sd == NULL)
		return GX_PLAYER_ERROR;
	header = sd->header;
	if(header && header->extradata){
		spudec = spudec_new_scaled(pattle, 0, 0, header->extradata, header->extradata_len);
	}else{
		for(i = 0; i < 16; i++)
			pattle[i] = spudec_palette_to_yuv(pattle[i]);
		spudec = spudec_new(pattle);
	}
	if(spudec == NULL)
		return  GX_PLAYER_ERROR;

	sd->priv = (void*)spudec;
	return GX_PLAYER_OK;
}

static int sub_vob_uinit(GxAVCodec*  sd)
{
	if(sd){
		PlayerSubSpuPage* spudec = sd->priv;
		if(spudec)
			spudec_free(spudec);
	}
	return GX_PLAYER_OK;
}

static void sd_vob_run_thread(void* filter)
{
	size_t len;
	unsigned char* in_data = NULL;
	unsigned char* out_data = NULL;
	size_t write_size,ac_size=0;
	GxMediaFilter* mf = GXMEDIAFILTER(filter);
	GxAVCodec* sdec = GXDECODEROBJECT(filter);
	GxPin* in_pin, *out_pin;
	GxSub  sf;

	if((filter == NULL)||(sdec == NULL))
		return  ;

	in_pin = GxMediaFilter_FindPin(filter, GX_SD_PIN_NAME_IN);
	out_pin = GxMediaFilter_FindPin(filter, GX_SD_PIN_NAME_OUT);

	gxlogf("[Player]%s[%d]:%s RUN\n",__FILE__,__LINE__,__FUNCTION__);
	while (mf->status != GX_MFT_STATE_STOPPED && in_pin && in_pin->source && in_pin->source->fifo ) {
		if(mf->status != GX_MFT_STATE_RUNNING)
		{
			GxCore_ThreadDelay(100);
			continue;
		}

		len = GxFifo_GetLength(in_pin->source->fifo);
		if(len <= 0){
			if(in_pin->source->filter->status== GX_MFT_STATE_STOPPED){
				GxMediaFilter_Stop(filter);
				return  ;
			}
			GxCore_ThreadDelay(100);
			continue;
		}
		GxFifo_Read(in_pin->source->fifo,(unsigned char*)(&sf),sizeof(GxSub),-1);
		if((sf.magic==GX_SUB_MAGIC) && sf.size)
		{
			in_data = av_malloc(sf.size);
			if(in_data == NULL)
			{
				gxlogf("[Player]%s[%d]:%s MALLOC ERROR! sf.size=0x%x\n",__FILE__,__LINE__,__FUNCTION__,sf.size);
				continue;
			}

			GxFifo_Read(in_pin->source->fifo,in_data,sf.size,-1);
			if((sf.type=='v')||(sf.type=='V'))
				ac_size = vobsub_decode(&sf,sdec->priv ,in_data,&out_data);
			else if ((sf.type=='x')||(sf.type=='X'))
				ac_size = xsub_decode(&sf,sdec->priv ,in_data,&out_data);
			else
				ac_size = 0;
			if((ac_size>0) && out_pin && out_pin->fifo)
			{
				write_size = 0;
				while((write_size < ac_size)&&(mf->status != GX_MFT_STATE_STOPPED)) {
					write_size += GxFifo_Write(out_pin->fifo, out_data+write_size, ac_size-write_size, 1000);
					GxCore_ThreadDelay(1);
				}
				av_free(out_data);
			}
			av_free(in_data);
		}
		else
			gxlogf("[Player]%s[%d]:%s ERROR! magic=0x%x ac_size=0x%x\n",__FILE__,__LINE__,__FUNCTION__,ac_size,sf.magic);

		GxCore_ThreadDelay(100);
	}
	gxlogf("[Player]%s[%d]:%s EXIT\n",__FILE__,__LINE__,__FUNCTION__);
}

static int sd_vob_run(GxMediaFilter*  filter)
{
	GxAVCodec* sd = GXDECODEROBJECT(filter);

	if(sd){
		if(sub_vob_init(sd) != GX_PLAYER_OK)
			return GX_PLAYER_ERROR;
		filter->status = GX_MFT_STATE_RUNNING;
		if (GxCore_ThreadCreate("sd_vob_run_thread", &sd->run_pthread,
				sd_vob_run_thread, filter, 1024*16, GXOS_DEFAULT_PRIORITY) == 0){
			gxlogf("[Player]%s[%d]:%s SUCCESS\n",__FILE__,__LINE__,__FUNCTION__);
			return GX_PLAYER_OK;
		}
	}
	gxlogf("[Player]%s[%d]:%s FAILED\n",__FILE__,__LINE__,__FUNCTION__);
	return GX_PLAYER_ERROR;
}

static int sd_vob_resume(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int sd_vob_config(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int sd_vob_pause(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int sd_vob_stop(GxMediaFilter*  filter)
{
	GxAVCodec* sd = GXDECODEROBJECT(filter);
	GxPin* in_pin;

	if(sd){
		GxCore_ThreadJoin(sd->run_pthread);
		in_pin = GxMediaFilter_FindPin(filter, GX_SD_PIN_NAME_IN);
		if(in_pin && in_pin->source && in_pin->source->fifo)
			GxFifo_Reset(in_pin->source->fifo);
		if(sub_vob_uinit(sd) != GX_PLAYER_OK)
			return GX_PLAYER_ERROR;
	}
	gxlogf("[Player]%s[%d]:%s SUCCESS\n",__FILE__,__LINE__,__FUNCTION__);
	return GX_PLAYER_OK;
}

static int sd_vob_control(GxAVCodec*  sd, int cmd, void* args)
{
	switch(cmd){
		case GX_SD_CTRL_SWITCH_HEADER:
			sd->header = args;
			break;
		default:
			break;
	}
	return GX_PLAYER_OK;
}

static int sd_vob_open(GxAVCodec*  sd)
{
	return GX_PLAYER_OK;
}

static int sd_vob_close(GxAVCodec*  sd)
{
	return GX_PLAYER_OK;
}

static int sd_vob_decode(GxAVCodec* sd, void **outdata, int *outdata_size, uint8_t * buf, int buf_size)
{
	return GX_PLAYER_OK;
}

GxAVCodecClass gx_sub_decoder_vob = {
	._inherit = {
		._inherit = {
			.name    = "Vob Sd",
			.parent  = (void* )&gx_SubDecoderBase,
			.size    = sizeof(GxSubCodec),
		},
		.run    = sd_vob_run,
		.pause  = sd_vob_pause,
		.resume = sd_vob_resume,
		.config = sd_vob_config,
		.stop   = sd_vob_stop,
	},
	DEF_AUTHOR("VobDecoder","Vob","No description","L.F","No comment"),

	.name    = "Vob Sub Decoder",
	.format  = {'v','V','x','X',-1},
	.open    = sd_vob_open,
	.close   = sd_vob_close,
	.control = sd_vob_control,
	.decode  = sd_vob_decode,
};
