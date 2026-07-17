#include "sd.h"
#include "../avformat/ttml_parse.h"

#define LINE_NUM 2048

static void check_is_contiguous_identical_string(TtmlContext *c)
{
	int i = 1;
	if ((!c) || (c->n_ttml_body < 1))
		return ;
	for (i = 1; i < c->n_ttml_body; i++) {
		ttml_body_element *body = c->ttml_body[i];
		ttml_body_element *pre_body = c->ttml_body[i-1];
		if (body && pre_body) {
			if (body->text && pre_body->text && !av_strcasecmp(body->text, pre_body->text)) {
				body->cont_mark = body->text[0];
				pre_body->cont_mark = body->text[0];
			}
		}
	}
}

static int sub_read_data_ttml(GxMediaFilter* mf, GxPin *out_pin, GxSub* in_sf, char* in_data)
{
	int ret = 0, i = 0, newsize = 0;
	int64_t startpts = 0, cont_time = 0;
	unsigned char *pdata = NULL;
	TtmlContext *c = NULL;
	GxSub  sf;

	if (!(c = av_mallocz(sizeof(TtmlContext)))) {
		newsize = AVERROR_NOMEM;
		goto end;
	}
	ret = ttml_parse(c, in_data, in_sf->size);
	if (ret >= 0) {
		startpts = in_sf->startpts;
		check_is_contiguous_identical_string(c);
		for (i = 0; i < c->n_ttml_body; i++) {
			ttml_body_element *body = NULL, *next_body = NULL;
			body = c->ttml_body[i];
			next_body = ((i+1) < c->n_ttml_body)?c->ttml_body[i+1]:NULL;
			if (body) {
				sf.startpts = startpts;
				sf.magic = GX_SUB_MAGIC;
				sf.pid = in_sf->pid;
				sf.endpts = sf.startpts + (body->end - body->begin)+cont_time;
				sf.type = in_sf->type;
				if (body->cont_mark && next_body && next_body->cont_mark) {
					cont_time = cont_time + (body->end - body->begin);
					continue;
				}
				if (body->text) {
					newsize = sizeof(GxSub) + strlen(body->text) + 1;
					if(!(pdata = av_mallocz(newsize))) {
						gxloge ("sd_ttml.c malloc fail.\n");
						newsize = AVERROR_NOMEM;
						goto end;
					}
					sf.size = newsize - sizeof(GxSub);
					memcpy(pdata, &sf, sizeof(GxSub));
					memcpy(pdata+sizeof(GxSub), body->text, sf.size);
					pdata[newsize-1] = '\0';
				}
				if((newsize > 0) && out_pin && out_pin->fifo) {
					size_t write_size = GxFifo_Write(out_pin->fifo, pdata, newsize, 1000);
					while((write_size < newsize)&&(mf->status != GX_MFT_STATE_STOPPED)) {
						GxCore_ThreadDelay(10);
						write_size += GxFifo_Write(out_pin->fifo, pdata+write_size, newsize-write_size, 1000);
					}
				}
				newsize = 0;
				cont_time = 0;
				startpts = sf.endpts;
				if (pdata) {
					av_free(pdata);
					pdata = NULL;
				}
				//GxCore_ThreadDelay(100);
			}
		}
	}
end:
	close_ttml_context(c);
	return newsize;
}

static void sd_ttml_run_thread(void* filter)
{
	size_t len, ac_size=0;
	unsigned char* in_data;
	GxMediaFilter* mf = GXMEDIAFILTER(filter);
	GxPin* in_pin, *out_pin;
	GxSub  sf;

	if(filter == NULL)
		return;

	in_pin = GxMediaFilter_FindPin(filter, GX_SD_PIN_NAME_IN);
	out_pin = GxMediaFilter_FindPin(filter, GX_SD_PIN_NAME_OUT);

	gxlogd ("[Player]sd_ttml.c sd_ttml_run_thread RUN\n");
	while (mf->status != GX_MFT_STATE_STOPPED && in_pin && in_pin->source && in_pin->source->fifo ) {
		if(mf->status != GX_MFT_STATE_RUNNING) {
			GxCore_ThreadDelay(100);
			continue;
		}
		if((len = GxFifo_GetLength(in_pin->source->fifo)) <= 0){
			if(in_pin->source->filter->status== GX_MFT_STATE_STOPPED){
				GxMediaFilter_Stop(filter);
				return ;
			}
			GxCore_ThreadDelay(100);
			continue;
		}
		GxFifo_Read(in_pin->source->fifo, (unsigned char*)(&sf), sizeof(GxSub), -1);
		if((sf.magic==GX_SUB_MAGIC) && sf.size) {
			in_data = av_mallocz(sf.size);
			if(in_data == NULL) {
				gxlogd ("[Player] MALLOC ERROR! sf.size=0x%x\n", sf.size);
				continue;
			}

			GxFifo_Read(in_pin->source->fifo, in_data, sf.size,-1);
			if((sf.type == 'l')||(sf.type == 'L')) {
				ac_size = sub_read_data_ttml(mf, out_pin, &sf, (char *)in_data);
			}
			if (in_data) {
				av_free(in_data);
				in_data = NULL;
			}
			if (ac_size < 0)
				continue;
		} else {
			gxlogd ("[Player] ERROR! magic=0x%x ac_size=0x%x\n", ac_size,sf.magic);
		}
		GxCore_ThreadDelay(100);
	}
}

static int sd_ttml_run(GxMediaFilter*  filter)
{
	GxAVCodec* sd = GXDECODEROBJECT(filter);

	if(sd){
		filter->status = GX_MFT_STATE_RUNNING;
		if (GxCore_ThreadCreate("sd_ttml_run_thread", &sd->run_pthread,
					sd_ttml_run_thread, filter, 1024*8, GXOS_DEFAULT_PRIORITY) == 0){
			gxlogd("[Player] SUCCESS\n");
			return GX_PLAYER_OK;
		}
	}
	gxlogd("[Player] FAILED\n");
	return GX_PLAYER_ERROR;
}

static int sd_ttml_pause(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int sd_ttml_stop(GxMediaFilter*  filter)
{
	GxAVCodec* sd = GXDECODEROBJECT(filter);
	GxPin* in_pin;

	if(sd){
		GxCore_ThreadJoin(sd->run_pthread);
		in_pin = GxMediaFilter_FindPin(filter, GX_SD_PIN_NAME_IN);
		if(in_pin && in_pin->source && in_pin->source->fifo)
			GxFifo_Reset(in_pin->source->fifo);
	}
	return GX_PLAYER_OK;
}

static int sd_ttml_resume(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int sd_ttml_config(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}
static int sd_ttml_open(GxAVCodec*  sd)
{
	return GX_PLAYER_OK;
}

static int sd_ttml_control(GxAVCodec*  sd, int cmd, void* args)
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

static int sd_ttml_close(GxAVCodec*  sd)
{
	return GX_PLAYER_OK;
}

static int sd_ttml_decode(GxAVCodec* sd, void **outdata, int *outdata_size, uint8_t * buf, int buf_size)
{
	return GX_PLAYER_OK;
}

GxAVCodecClass gx_sub_decoder_ttml = {
	._inherit = {
		._inherit = {
			.name    = "TTMLSd",
			.parent  = (void* )&gx_SubDecoderBase,
			.size    = sizeof(GxSubCodec),
		},
		.run    = sd_ttml_run,
		.pause  = sd_ttml_pause,
		.resume = sd_ttml_resume,
		.config = sd_ttml_config,
		.stop   = sd_ttml_stop,
	},
	DEF_AUTHOR("TTMLDecoder","Ttml","No description","L.F","No comment"),

	.name    = "TTML Sub Decoder",
	.format  = {'l', 'L', -1},
	.open    = sd_ttml_open,
	.close   = sd_ttml_close,
	.control = sd_ttml_control,
	.decode  = sd_ttml_decode,
};
