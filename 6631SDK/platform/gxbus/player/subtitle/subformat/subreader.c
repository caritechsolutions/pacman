
#include "gx_subreader.h"

extern GxSubReaderClass subreader_chr;
extern GxSubReaderClass subreader_spu;

static GxSubReaderClass* gx_sub_reader_class[] = {
	&subreader_chr,
	&subreader_spu,
	NULL
};

static GxSubReaderClass* get_classs_from_postfix(const char* filename)
{
	int i,j;

	GxSubReaderClass* cls = NULL;

	for (i = 0; gx_sub_reader_class[i] != NULL; i++) {
		for (j = 0; j < GX_SUBTITLE_POSTFIX_MAX; j++) {
			if (gx_sub_reader_class[i]->extensions[j] == NULL)
				break;
			else
			{
				if (strstr(filename,gx_sub_reader_class[i]->extensions[j]))
				{
					cls = gx_sub_reader_class[i];
					return cls;
				}
			}
		}
	}

	return cls;
}

GxSubData* GxSubReader_Open(const char* lang,const char* filename)
{
	GxSubData* subd = NULL;
	GxSubReaderClass* cls;

	if(filename == NULL)
		return NULL;

	cls = get_classs_from_postfix(filename);

	if(cls && cls->open)
	{
		gxlogf("[Subt]:Subreader ---> %s\n",cls->name);
		subd = cls->open(lang,filename);
	}

	if(subd)
	{
		subd->cls = cls;
	}

	return subd;
}


void GxSubReader_Close( GxSubData*  subd)
{
	if(subd && subd->cls && subd->cls->close)
	{
		subd->cls->close(subd);
	}
}

int GxSubReader_GetPacket(GxSubData* subd,void** pkt,int* len,int64_t* startpts ,int64_t* endpts)
{
	if(subd && subd->cls && subd->cls->get_packet)
	{
		return subd->cls->get_packet(subd,pkt,len,startpts,endpts);
	}

	return -1;
}

void GxSubReader_SwitchStream(GxSubData* subd,int id)
{
	if(subd && subd->cls && subd->cls->close)
	{
		subd->cls->switch_stream(subd,id);
	}
}

void GxSubReader_GetInfo(GxSubData* subd, void* sub_info)
{
	if(subd && subd->cls && subd->cls->get_info)
	{
		subd->cls->get_info(subd,sub_info);
	}
}

int GxSubReader_Alloc_Chr(GxSubData* sub, int sublines)
{
	if(sub && sub->cls && sub->cls->alloc_chr)
	{
		return sub->cls->alloc_chr(sub, sublines);
	}
	return 0;
}

int  GxSubReader_Get_ByTime(GxSubData* sub, uint32_t cur, int delay, uint32_t* start, uint32_t* end)
{
	int ret;
	uint32_t udelay = (uint32_t)abs(delay);
	while(1){
		ret = GxSubReader_Alloc_Chr(sub, 1);
		if(ret != 0){
			ret = -1;
			break;
		}

		if(sub->encode == SUB_ENC_SPU){
			*start = sub->vob->spu_streams->startpts;
			*end   = sub->vob->spu_streams->endpts;
		}else{
			*start = sub->sub_hdr->start;
			*end   = sub->sub_hdr->end;
		}
		if((*end == 0)||(*end < *start))
			*end = *start + 200;

		if(delay >= 0){
			*start += udelay;
			*end += udelay;
		}else{
			if(*end <= udelay){
				GxSubReader_Free_Chr(sub);
				continue;
			}else{
				*end -= udelay;
				if(*start <= udelay)
					*start = 0;
				else
					*start -= udelay;
			}
		}

		if((cur < *end) && (*start < *end)){
			ret = 0;
			break;
		}else if(*start >= *end){
			GxSubReader_Free_Chr(sub);
			ret = -1;
			break;
		}
		GxSubReader_Free_Chr(sub);
	}
	return ret;
}

void GxSubReader_Free_Chr(GxSubData* sub)
{
	if(sub && sub->cls && sub->cls->free_chr)
	{
		sub->cls->free_chr(sub);
	}
}

int GxSubReader_Reset_Chr(GxSubData* sub)
{
	if(sub && sub->cls && sub->cls->reset_chr)
	{
		return sub->cls->reset_chr(sub);
	}
	return 0;
}
