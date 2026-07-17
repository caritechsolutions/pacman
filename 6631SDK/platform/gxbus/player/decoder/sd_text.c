#include "sd.h"

#define LINE_NUM 2048

static size_t sub_read_mov_text(GxSub* in_sf, char* in_data, unsigned char** out_data)
{
	unsigned char *pdata;
	int newsize = 0;

	if (in_sf->size <= 2)
		return 0;

	newsize = (sizeof(GxSub) + in_sf->size + 1) - 2;
	pdata=av_malloc(newsize);
	if(pdata == NULL)
		return 0;
	in_sf->size = newsize - sizeof(GxSub);
	memcpy(pdata, in_sf, sizeof(GxSub));
	memcpy(pdata + sizeof(GxSub), (in_data + 2), (in_sf->size - 2));
	pdata[newsize - 1] = '\0';
	*out_data = pdata;
	return newsize;
}

static size_t sub_read_data_srt(GxSub* in_sf, char* in_data, unsigned char** out_data)
{
	unsigned char *pdata;
	int newsize = sizeof(GxSub) + in_sf->size + 1;

	pdata = av_malloc(newsize);
	if(pdata == NULL)
		return 0;
	in_sf->size = newsize - sizeof(GxSub);
	memcpy(pdata,in_sf,sizeof(GxSub));
	memcpy(pdata+sizeof(GxSub),in_data,in_sf->size);
	pdata[newsize-1] = '\0';
	*out_data = pdata;
	return newsize;
}

static size_t sub_read_data_ass(GxSub* in_sf, char* in_data, unsigned char** out_data)
{
	int comma;
	int hour1, min1, sec1, hunsec1,
		hour2, min2, sec2, hunsec2, nothing;
	char line3[LINE_NUM], *line2;
	char* tmp;
	int64_t start = 0;
	int64_t end = 0;
	unsigned char* pdata;
	int32_t tmpsize = 0;
	int32_t newsize = 0;
	int32_t slen = 0;
	int32_t slen3 = 0;
	int32_t lines = 0;

	if(in_sf->size > LINE_NUM)
		return 0;
	newsize = sizeof(GxSub)+1;
	pdata = av_malloc(newsize);
	if(pdata == NULL)
		return 0;
	memset(pdata,'\0',newsize);
	while(tmpsize < in_sf->size)
	{
		memset(line3,'\0',LINE_NUM);
		if(sscanf(in_data,"%[^\n\r]",line3)==0)
			break;
		slen3 = strlen(line3)+2;
		memset(line3,'\0',LINE_NUM);
		if(sscanf(in_data, "Dialogue: Marked=%d,%d:%d:%d.%d,%d:%d:%d.%d,"
					"%[^\n\r]", &nothing,
					&hour1, &min1, &sec1, &hunsec1,
					&hour2, &min2, &sec2, &hunsec2,
					line3) < 9
				&&
				sscanf(in_data, "Dialogue: %d,%d:%d:%d.%d,%d:%d:%d.%d,"
					"%[^\n\r]", &nothing,
					&hour1, &min1, &sec1, &hunsec1,
					&hour2, &min2, &sec2, &hunsec2,
					line3) < 9
				&&
				sscanf(in_data, "Dialogue: ,%d:%d:%d.%d,%d:%d:%d.%d,"
					"%[^\n\r]",
					&hour1, &min1, &sec1, &hunsec1,
					&hour2, &min2, &sec2, &hunsec2,
					line3) < 9)
			break;
		line2=strchr(line3, ',');
		if(line2 == NULL)
		{
			av_free(pdata);
			return 0;
		}
		line2++;
		for (comma = 4; comma < 9; comma++)
		{
			tmp = line2;
			if(!(tmp=strchr(tmp, ','))) break;
			tmp++;
			if(*tmp == ' ') break;
			line2 = tmp;
		}
		if(*line2 == ',')
			line2++;
		start = 3600000*hour1 + 60000*min1 + 1000*sec1 + hunsec1*10;
		end   = 3600000*hour2 + 60000*min2 + 1000*sec2 + hunsec2*10;
		if (lines > 0) {
			pdata[newsize-1]='\n';
			newsize++;
		}
		slen = strlen(line2);
		newsize += slen;
		pdata = av_realloc(pdata,newsize);
		if(pdata == NULL)
			return 0;
		memcpy(pdata+newsize-slen-1,line2,slen);
		pdata[newsize-1]='\0';
		in_data += slen3;
		tmpsize += slen3;
		lines++;
	}
	in_sf->size   = newsize - sizeof(GxSub);
	in_sf->endpts = in_sf->startpts + end - start;
	memcpy(pdata,in_sf,sizeof(GxSub));
	*out_data = pdata;
	return newsize;
}

static void sd_text_run_thread(void* filter)
{
	size_t len;
	size_t write_size,ac_size=0;
	unsigned char* in_data,*out_data;
	GxMediaFilter* mf = GXMEDIAFILTER(filter);
	GxPin* in_pin, *out_pin;
	GxSub  sf;

	if(filter == NULL)
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
			if((sf.type == 'a')||(sf.type == 'A'))
				ac_size = sub_read_data_ass(&sf,(char*)in_data,&out_data);
			else if((sf.type == 't')||(sf.type == 'T'))
				ac_size = sub_read_data_srt(&sf,(char*)in_data,&out_data);
			else if((sf.type == 'm')||(sf.type == 'M'))
				ac_size = sub_read_mov_text(&sf,(char*)in_data,&out_data);
			else
				ac_size = 0;

			if((ac_size>0) && out_pin && out_pin->fifo)
			{
				write_size = GxFifo_Write(out_pin->fifo, out_data, ac_size, 1000);
				while((write_size < ac_size)&&(mf->status != GX_MFT_STATE_STOPPED)) {
					GxCore_ThreadDelay(10);
					write_size += GxFifo_Write(out_pin->fifo, out_data+write_size, ac_size-write_size, 1000);
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

static int sd_text_run(GxMediaFilter*  filter)
{
	GxAVCodec* sd = GXDECODEROBJECT(filter);

	if(sd){
		filter->status = GX_MFT_STATE_RUNNING;
		if (GxCore_ThreadCreate("sd_text_run_thread", &sd->run_pthread,
					sd_text_run_thread, filter, 1024*8, GXOS_DEFAULT_PRIORITY) == 0){
			gxlogf("[Player]%s[%d]:%s SUCCESS\n",__FILE__,__LINE__,__FUNCTION__);
			return GX_PLAYER_OK;
		}
	}
	gxlogf("[Player]%s[%d]:%s FAILED\n",__FILE__,__LINE__,__FUNCTION__);
	return GX_PLAYER_ERROR;
}

static int sd_text_pause(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int sd_text_stop(GxMediaFilter*  filter)
{
	GxAVCodec* sd = GXDECODEROBJECT(filter);
	GxPin* in_pin;

	if(sd){
		GxCore_ThreadJoin(sd->run_pthread);
		in_pin = GxMediaFilter_FindPin(filter, GX_SD_PIN_NAME_IN);
		if(in_pin && in_pin->source && in_pin->source->fifo)
			GxFifo_Reset(in_pin->source->fifo);
	}

	gxlogf("[Player]%s[%d]:%s SUCCESS\n",__FILE__,__LINE__,__FUNCTION__);
	return GX_PLAYER_OK;
}

static int sd_text_resume(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int sd_text_config(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}
static int sd_text_open(GxAVCodec*  sd)
{
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int sd_text_control(GxAVCodec*  sd, int cmd, void* args)
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

static int sd_text_close(GxAVCodec*  sd)
{
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int sd_text_decode(GxAVCodec* sd, void **outdata, int *outdata_size, uint8_t * buf, int buf_size)
{
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

GxAVCodecClass gx_sub_decoder_text = {
	._inherit = {
		._inherit = {
			.name    = "TextSd",
			.parent  = (void* )&gx_SubDecoderBase,
			.size    = sizeof(GxSubCodec),
		},
		.run    = sd_text_run,
		.pause  = sd_text_pause,
		.resume = sd_text_resume,
		.config = sd_text_config,
		.stop   = sd_text_stop,
	},
	DEF_AUTHOR("TextDecoder","Text","No description","L.F","No comment"),

	.name    = "Text Sub Decoder",
	.format  = {'t','T','a','A','m','M' -1},
	.open    = sd_text_open,
	.close   = sd_text_close,
	.control = sd_text_control,
	.decode  = sd_text_decode,
};
