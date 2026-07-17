#include "gx_stream.h"
#include "gx_subreader.h"
#include "spudec.h"
#include "vobsub.h"

GxSubData* subreader_open_spu(const char* lang,const char* filename)
{
	int fd;
	GxSubData* subd=NULL;
	GxSpuDec* spu = NULL;
	vobsub_t* vob = NULL;

	if(filename==NULL)
		return NULL;

	fd = vobsub_open(filename, (void**)&spu, (void**)&vob);
	if(fd == -1)
		return NULL;

	subd = (GxSubData*)av_malloc(sizeof(GxSubData));
	if(!subd)
	{
		vobsub_close(fd,spu,vob);
		return NULL;
	}

	memset(subd, 0, sizeof(GxSubData));
	subd->filename      = av_strdup(filename);
	subd->sub_track      = 1;
	subd->sub_hdr       = NULL;
	subd->encode        = SUB_ENC_SPU;
	subd->vob           = vob;
	subd->spu           = spu;
	subd->fd            = fd;

	return subd;
}

void subreader_close_spu( GxSubData*  subd )
{
	if(subd)
	{
		if(subd->filename)
			av_free(subd->filename);
		vobsub_close(subd->fd,subd->spu,subd->vob);
		av_free(subd);
	}
}

int subreader_alloc_spuchr(GxSubData* subd, uint32_t sublines)
{
	vob_stream_t** p;
	int i;

	if(!subd || !(subd->vob))
		return -1;

	p = &(subd->vob->spu_streams);
	for(i = 0; i < sublines; i++)
	{
		*p = vobsub_alloc_oneline_chr(subd->fd);
		if(*p == NULL)
			break;
		p = &((*p)->next);
	}
	if(i == 0)
	{
		return -1;
	}
	subd->sub_num = i;
	return 0;
}

int subreader_reset_spuchr(GxSubData* subd)
{
	vobsub_reset_oneline_chr(subd->fd);
	return 0;
}

void subreader_free_spuchr(GxSubData* subd)
{
	int i;
	vob_stream_t* p;
	vob_stream_t* p_next;

	if(!subd || !(subd->vob) || !(subd->vob->spu_streams))
		return;

	p = subd->vob->spu_streams;
	for(i = 0;(i<subd->sub_num) && p; i++)
	{
		p_next = p->next;
		vobsub_free_oneline_packet(p->packet);
		vobsub_free_oneline_chr(p);
		p = p_next;
	}
	subd->sub_num = 0;
	subd->vob->spu_streams = NULL;
	return;
}

int subreader_get_packet_spu(GxSubData* subd,void** pkt,int* len,int64_t* startpts ,int64_t* endpts)
{
	int i;
	vob_stream_t** pst;
	char sub_file[256];
	char* pos;
	char* idx_file = subd->filename;

	if(!subd || !(subd->vob) || !(subd->vob->spu_streams))
		return -1;

	if(!(idx_file)||(!(pos = strstr(idx_file, ".idx")) && !(pos = strstr(idx_file, ".IDX"))))
		return -1;

	memcpy(sub_file,idx_file,pos-idx_file);
	sub_file[pos-idx_file]='\0';
	strncat(sub_file,".sub", sizeof(sub_file)-1);

	pst = &(subd->vob->spu_streams);
	for(i = 0; (i<subd->sub_num)&&(pst!=NULL); i++)
	{
		(*pst)->packet = vobsub_alloc_oneline_packet(sub_file,*pst,subd->spu);
		pst = &((*pst)->next);
	}
	return 0;
}

void subreader_switch_stream_spu(GxSubData* sub,int id)
{
	vobsub_switch_stream_spu(id);
}

int subreader_gets_spu_info(GxSubData* sub, GxSubInfo*subinfo)
{
	int i;
	vob_lang_t vobinfo;

	memset(&vobinfo,0,sizeof(vob_lang_t));
	vobsub_get_info_spu(&vobinfo);
	if(vobinfo.num)
	{
		subinfo->cur_id = vobinfo.cur_id;
		subinfo->num    = vobinfo.num;
		subinfo->encode = SUB_ENC_SPU;
		for(i = 0; i < subinfo->num; i++)
		{
			subinfo->id[i] = vobinfo.id[i];
			memcpy(subinfo->lang[i],vobinfo.lang[i],16);
			strncpy(subinfo->codec_id[i],"VobSub", strlen(subinfo->codec_id[i]));
		}
	}
	return 0;
}

GxSubReaderClass subreader_spu = {
	.name = "SPU Subtitle",
	.extensions = {".idx",".IDX",NULL},

	.open  = subreader_open_spu,
	.close = subreader_close_spu,
	.get_packet = subreader_get_packet_spu,
	.switch_stream = subreader_switch_stream_spu,
	.alloc_chr = subreader_alloc_spuchr,
	.free_chr = subreader_free_spuchr,
	.reset_chr = subreader_reset_spuchr,
	.get_info = subreader_gets_spu_info,
};
