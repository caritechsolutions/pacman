
#include <stdio.h>
#include <unistd.h>

#include "components.h"
#include "parse_mp4.h" // .MP4 specific stuff
#include "gx_subreader.h"
#include "gx_demux.h"
#include "../avformat/avformat.h"
#include "../avutil/bswap.h"
#include "../avutil/intreadwrite.h"
#include "../avcodec/mpeg4audio.h"
#include "../decoder/vd_mpeg_es.h"

#ifndef _FCNTL_H
#include <fcntl.h>
#endif

#define ENABLE_AAC_FRAME 1
#define ENABLE_AVC_FRAME 1

#define char2short(x,y) AV_RB16(&(x)[(y)])
#define char2int(x,y)   AV_RB32(&(x)[(y)])

#define CHECK_POINT(p) if(p==NULL) {\
	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);\
	return -1;}
#define CHECK_RET(ret) if(ret<0) {\
	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);\
	return ret;}
#define MOV_FREE(ptr) {if(ptr) av_free(ptr); ptr=NULL;}

static const uint8_t start_code[4] = { 0x00, 0x00, 0x00, 0x01 };

static void* realloc_struct(void *ptr, size_t nmemb, size_t size)
{
	if (nmemb > SIZE_MAX / size) {
		MOV_FREE(ptr);
		return NULL;
	}
	return av_realloc(ptr, nmemb*  size);
}

typedef struct {
	// double pts; // duration
	unsigned int size;
	//  off_t pos;
} mov_sample_t;

typedef struct {
	double pts; // duration
	off_t pos;
} mov_sample2_t;

typedef struct {
	double pts; // duration
	off_t  pos;
	unsigned int sampleno;
	unsigned int durmap_index;
	unsigned int durmap_size;
	unsigned int chunk_index;
	unsigned int chunk_size;
} mov_sample3_t;

typedef struct {
	unsigned int sample; // number of the first sample in the chunk
	unsigned int size;   // number of samples in the chunk
	off_t pos;
} mov_chunk_t;

typedef struct {
	unsigned int first;
	unsigned int spc;
	unsigned int sdid;
} mov_chunkmap_t;

typedef struct {
	unsigned int num;
	unsigned int dur;
} mov_durmap_t;

typedef struct {
	unsigned int dur;
	unsigned int pos;
	int speed;
	//
	int frames;
	int start_sample;
	int start_frame;
	int pts_offset;
} mov_editlist_t;

#define MOV_TRAK_UNKNOWN 0
#define MOV_TRAK_VIDEO 1
#define MOV_TRAK_AUDIO 2
#define MOV_TRAK_FLASH 3
#define MOV_TRAK_GENERIC 4
#define MOV_TRAK_CODE 5

#define MOV_SAMP2_COUNT 1024
#define MOV_SAMP3_UNIT  4096

typedef struct {
	int id;
	int type;
	off_t pos;
	//
	unsigned int media_handler;
	unsigned int data_handler;
	//
	int timescale;
	unsigned int length;
	int samplesize;  // 0 = variable
	int duration;    // 0 = variable
	int width,height; // for video
	unsigned int fourcc;
	unsigned int nchannels;
	unsigned int samplebytes;
	//
	int tkdata_len;  // track data
	unsigned char* tkdata;
	int stdata_len;  // stream data
	unsigned char* stdata;
	//
	unsigned char* stream_header;
	int stream_header_len; // if >0, this header should be sent before the 1st frame
	//
	int samples_size;
	mov_sample_t* samples;
	int chunks_size;
	mov_chunk_t* chunks;
	int chunkmap_size;
	mov_chunkmap_t* chunkmap;
	int durmap_size;
	mov_durmap_t* durmap;
	int keyframes_size;
	unsigned int* keyframes;
	int editlist_size;
	mov_editlist_t* editlist;
	int editlist_pos;

	mov_sample2_t  samples2[MOV_SAMP2_COUNT];
	mov_sample3_t  *samples3;
	unsigned int samples3_num;
	unsigned int samples2_start;
	unsigned int samples2_end;
} mov_track_t;

static int mov_build_samples3(mov_track_t* trak)
{
	int i,j,k;
	int s=0;
	double pts=0;

	trak->samples3_num = trak->samples_size/MOV_SAMP3_UNIT + 1;
	trak->samples3 = av_malloc(trak->samples3_num*sizeof(mov_sample3_t));
	CHECK_POINT(trak->samples3);

	// calc pts:
	s=0;
	k=0;
	for(j=0;j<trak->durmap_size;j++)
	{
		for(i=0;i<trak->durmap[j].num;i++)
		{
			if (s >= trak->samples_size)
				return -1;
			if(s%MOV_SAMP3_UNIT==0)
			{
				trak->samples3[k].sampleno = s;
				trak->samples3[k].pts = pts;
				trak->samples3[k].durmap_index = j;
				trak->samples3[k].durmap_size = i;
				k++;
			}
			pts+=trak->durmap[j].dur;
			++s;
		}
	}

	// calc sample offsets
	s=0;
	k=0;
	for(j=0;j<trak->chunks_size;j++)
	{
		off_t pos=trak->chunks[j].pos;
		for(i=0;i<trak->chunks[j].size;i++)
		{
			if (s >= trak->samples_size)
				return -1;
			if(s%MOV_SAMP3_UNIT==0)
			{
				trak->samples3[k].pos = pos;
				trak->samples3[k].chunk_index = j;
				trak->samples3[k].chunk_size = i;
				k++;
			}
			pos+=trak->samples[s].size;
			++s;
		}
	}

	return 0;
}

static int mov_build_pos_pts(mov_track_t* trak,int start)
{
	int i,j,sample3=0;
	int s=0;
	double pts=0;
	off_t pos;

	if(start >= trak->samples_size)
		return -1;

	//find nearest sample
	if(start != 0)
	{
		for(sample3=0;sample3<trak->samples3_num;sample3++)
		{
			if(trak->samples3[sample3].sampleno > start)
				break;
		}
	}
	if(sample3 > 0)
		sample3=sample3-1;

	// calc pts:

	j = trak->samples3[sample3].durmap_index;
	i = trak->samples3[sample3].durmap_size;
	s = trak->samples3[sample3].sampleno;
	pts = trak->samples3[sample3].pts;

	goto skip1pts;

	for(;j<trak->durmap_size;j++)
	{
		i=0;
skip1pts:
		for(;i<trak->durmap[j].num;i++)
		{
			if (s >= trak->samples_size)
				return -1;
			if(s<start)
			{
				pts+=trak->durmap[j].dur;
				s++;
				continue;
			}
			if(s-start >= MOV_SAMP2_COUNT)
				goto skippts;
			trak->samples2[s-start].pts=pts;
			pts+=trak->durmap[j].dur;
			++s;
		}
	}

skippts:
	// calc sample offsets
	j = trak->samples3[sample3].chunk_index;
	i = trak->samples3[sample3].chunk_size;
	s = trak->samples3[sample3].sampleno;
	pos = trak->samples3[sample3].pos;

	goto skip1pos;

	for(;j<trak->chunks_size;j++)
	{
		pos=trak->chunks[j].pos;
		i=0;
skip1pos:
		for(;i<trak->chunks[j].size;i++)
		{
			if (s >= trak->samples_size)
				return -1;
			if(s<start)
			{
				pos+=trak->samples[s].size;
				s++;
				continue;
			}
			if(s-start >= MOV_SAMP2_COUNT)
				goto skippos;
			trak->samples2[s-start].pos=pos;
			pos+=trak->samples[s].size;
			++s;
		}
	}

skippos:
	trak->samples2_start = start;
	trak->samples2_end = trak->samples2_start+MOV_SAMP2_COUNT;
	if(trak->samples2_end >= trak->samples_size)
		trak->samples2_end = trak->samples_size;

	return 0;
}

static int mov_get_pos_pts(mov_track_t* trak,int s,off_t* pos,double* pts)
{
	if(s >= trak->samples_size)
		return -1;

	if(s < trak->samples2_start || s >= trak->samples2_end)
	{
		mov_build_pos_pts(trak,s);
	}

	*pos = trak->samples2[s-trak->samples2_start].pos;
	*pts = trak->samples2[s-trak->samples2_start].pts;
	return 0;
}

int mov_build_index(mov_track_t* trak,int timescale)
{
	int i,j,s;
	int last=trak->chunks_size;
	//unsigned int pts=0;

	gxlogf( "MOV track #%d: %d chunks, %d samples\n",trak->id,trak->chunks_size,trak->samples_size);
	gxlogf( "pts=%d  scale=%d  time=%5.3f\n",trak->length,trak->timescale,(float)trak->length/(float)trak->timescale);

	// process chunkmap:
	i=trak->chunkmap_size;
	while(i>0){
		--i;
		j=trak->chunkmap[i].first;
		for(;j>=0 && j<last;j++){
			trak->chunks[j].size=trak->chunkmap[i].spc;
		}
		last=GXMIN(trak->chunkmap[i].first, trak->chunks_size);
	}

	// calc pts of chunks:
	s=0;
	for(j=0;j<trak->chunks_size;j++){
		trak->chunks[j].sample=s;
		s+=trak->chunks[j].size;
	}
	i = 0;
	for (j = 0; j < trak->durmap_size; j++)
		i += trak->durmap[j].num;
	if (i != s) {
		gxlogf("MOV: durmap and chunkmap sample count differ (%i vs %i)\n", i, s);
		if (i > s) s = i;
	}

	// workaround for fixed-size video frames (dv and uncompressed)
	if(!trak->samples_size && trak->type!=MOV_TRAK_AUDIO){
		trak->samples=av_calloc(s, sizeof(mov_sample_t));
		CHECK_POINT(trak->samples);
		trak->samples_size=trak->samples ? s : 0;
		for(i=0;i<trak->samples_size;i++)
			trak->samples[i].size=trak->samplesize;
		trak->samplesize=0;
	}

	if(!trak->samples_size){
		// constant sampesize
		if(trak->durmap_size==1 || (trak->durmap_size==2 && trak->durmap[1].num==1))
		{
			trak->duration=trak->durmap[0].dur;
		}
		else
			gxlogd ("*** constant samplesize & variable duration not yet supported!* **\nContact the author if you have such sample file!\n");
		return 0;
	}

	if (trak->samples_size < s) {
		gxlogf( "MOV: durmap or chunkmap bigger than sample count (%i vs %i)\n", s, trak->samples_size);
		trak->samples = realloc_struct(trak->samples, s, sizeof(mov_sample_t));
		CHECK_POINT(trak->samples);
		trak->samples_size = trak->samples ? s : 0;
	}

#if 0
	// calc pts:
	s=0;
	for(j=0;j<trak->durmap_size;j++){
		for(i=0;i<trak->durmap[j].num;i++){
			if (s >= trak->samples_size)
				break;
			trak->samples[s].pts=pts;
			++s;
			pts+=trak->durmap[j].dur;
		}
	}

	// calc sample offsets
	s=0;
	for(j=0;j<trak->chunks_size;j++){
		off_t pos=trak->chunks[j].pos;
		for(i=0;i<trak->chunks[j].size;i++){
			if (s >= trak->samples_size)
				break;
			trak->samples[s].pos=pos;
			// gxlogf("Sample %5d: pts=%8d  off=0x%08X  size=%d\n",s,trak->samples[s].pts,(int)trak->samples[s].pos,trak->samples[s].size);
			pos+=trak->samples[s].size;
			++s;
		}
	}
#else
	mov_build_samples3(trak);
	mov_build_pos_pts(trak,0);
#endif

	// precalc editlist entries
	if(trak->editlist_size>0)
	{
		int frame=0;
		int e_pts=0;
		double spts;
		off_t spos;
		for(i=0;i<trak->editlist_size;i++)
		{
			mov_editlist_t* el=&trak->editlist[i];
			int sample=0;
			int pts=el->pos;
			el->start_frame=frame;
			if(pts<0){
				// skip!
				el->frames=0;
				continue;
			}
			// find start sample
			for(;sample<trak->samples_size;sample++)
			{
				mov_get_pos_pts(trak,sample,&spos,&spts);
				if(pts<=spts)
					break;
			}
			el->start_sample=sample;
			el->pts_offset=((long long)e_pts*(long long)trak->timescale)/(long long)timescale-spts;
			pts+=((long long)el->dur*(long long)trak->timescale)/(long long)timescale;
			e_pts+=el->dur;
			// find end sample
			for(;sample<trak->samples_size;sample++)
			{
				mov_get_pos_pts(trak,sample,&spos,&spts);
				if(pts<spts)
					break;
			}
			el->frames=sample-el->start_sample;
			frame+=el->frames;
			gxlogf("EL#%d: pts=%d  1st_sample=%d  frames=%d (%5.3fs)  pts_offs=%d\n",i,
					el->pos,el->start_sample, el->frames,(float)(el->dur)/(float)timescale, el->pts_offset);
		}
	}

	return 0;
}

#define MOV_MAX_TRACKS 256
#define MOV_MAX_SUBLEN 1024

typedef struct {
	off_t moov_start;
	off_t moov_end;
	off_t mdat_start;
	off_t mdat_end;
	int track_db;
	mov_track_t* tracks[MOV_MAX_TRACKS];
	int timescale; // movie timescale
	int duration;  // movie duration (in movie timescale units)
	GxSubPage subs;
	char subtext[MOV_MAX_SUBLEN + 1];
	int current_sub;
	int nal_size_size;
	int isom;
} DemuxMovPriv;

#define MOV_FOURCC(a,b,c,d) ((a<<24)|(b<<16)|(c<<8)|(d))

static int demux_mov_check_file(GxDemuxer* demuxer){
	int flags=0;
	int no=0;
	int minor_version=0;
	DemuxMovPriv* priv=av_malloc(sizeof(DemuxMovPriv));

	memset(priv,0,sizeof(DemuxMovPriv));
	priv->current_sub = -1;

	while(1){
		int i;
		int skipped=8;
		off_t len=GxStream_ReadDword(demuxer->stream);
		unsigned int id=GxStream_ReadDword(demuxer->stream);
		if(GxStream_Eof(demuxer->stream)) break; // EOF
		if (len == 1) /* real size is 64bits - cjb*/
		{
			len = GxStream_ReadQword(demuxer->stream);
			skipped += 8;
		}
		else if(len<8)
			break; // invalid chunk

		switch(id){
			case MOV_FOURCC('f','t','y','p'):
				{
					unsigned int tmp;
					// File Type Box (ftyp):
					// char[4]  major_brand	   (eg. 'isom')
					// int      minor_version	   (eg. 0x00000000)
					// char[4]  compatible_brands[]  (eg. 'mp41')
					// compatible_brands list spans to the end of box
#if 1
					tmp = GxStream_ReadDword(demuxer->stream);
					switch(tmp)
					{
						case MOV_FOURCC('i','s','o','m'):
							gxlogf("ISO: File Type Major Brand: ISO Base Media\n");
							break;
						case MOV_FOURCC('m','p','4','1'):
							gxlogf("ISO: File Type Major Brand: ISO/IEC 14496-1 (MPEG-4 system) v1\n");
							break;
						case MOV_FOURCC('m','p','4','2'):
							gxlogf("ISO: File Type Major Brand: ISO/IEC 14496-1 (MPEG-4 system) v2\n");
							break;
						case MOV_FOURCC('M','4','A',' '):
							gxlogf("ISO: File Type Major Brand: Apple iTunes AAC-LC Audio\n");
							break;
						case MOV_FOURCC('M','4','P',' '):
							gxlogf("ISO: File Type Major Brand: Apple iTunes AAC-LC Protected Audio\n");
							break;
						case MOV_FOURCC('q','t',' ',' '):
							priv->isom = 1;
							gxlogf("ISO: File Type Major Brand: Original QuickTime\n");
							break;
						case MOV_FOURCC('3','g','p','1'):
							gxlogf("ISO: File Type Major Brand: 3GPP Profile 1\n");
							break;
						case MOV_FOURCC('3','g','p','2'):
						case MOV_FOURCC('3','g','2','a'):
							gxlogf("ISO: File Type Major Brand: 3GPP Profile 2\n");
							break;
						case MOV_FOURCC('3','g','p','3'):
							gxlogf("ISO: File Type Major Brand: 3GPP Profile 3\n");
							break;
						case MOV_FOURCC('3','g','p','4'):
							gxlogf("ISO: File Type Major Brand: 3GPP Profile 4\n");
							break;
						case MOV_FOURCC('3','g','p','5'):
							gxlogf("ISO: File Type Major Brand: 3GPP Profile 5\n");
							break;
						case MOV_FOURCC('m','m','p','4'):
							gxlogf("ISO: File Type Major Brand: Mobile ISO/IEC 14496-1 (MPEG-4 system)\n");
							break;
						default:
							tmp = be2me_32(tmp);
							gxlogf("ISO: Unknown File Type Major Brand: %.4s\n",(char* )&tmp);
					}
					minor_version = GxStream_ReadDword(demuxer->stream);
					gxlogf("ISO: File Type Minor Version: %d\n", minor_version);
					skipped += 8;
					// List all compatible brands
					for(i = 0; i < ((len-16)/4); i++) {
						tmp = GxStream_ReadDword(demuxer->stream);
						tmp = be2me_32(tmp);
						skipped += 4;
					}
#endif
				} break;
			case MOV_FOURCC('m','o','o','v'):
				//	case MOV_FOURCC('c','m','o','v'):
				gxlogf("MOV: Movie header found!\n");
				priv->moov_start=(off_t)GxStream_Tell(demuxer->stream);
				priv->moov_end=(off_t)priv->moov_start+len-skipped;
				gxlogf("MOV: Movie header: start: %lld end: %lld\n",
						(int64_t)priv->moov_start, (int64_t)priv->moov_end);
				skipped+=8;
				i = GxStream_ReadDword(demuxer->stream)-8;
				if(GxStream_ReadDword(demuxer->stream)==MOV_FOURCC('r','m','r','a')){
					skipped+=i;
					demuxer->type=GX_DEMUXER_TYPE_PLAYLIST;
					while(i>0){
						int len=GxStream_ReadDword(demuxer->stream)-8;
						int fcc=GxStream_ReadDword(demuxer->stream);
						if(len<0) break; // EOF!?
						i-=8;
						//		  gxlogd("i=%d  len=%d\n",i,len);
						switch(fcc){
							case MOV_FOURCC('r','m','d','a'):
								continue;
							case MOV_FOURCC('r','d','r','f'):
								{
									av_unused int tmp=GxStream_ReadDword(demuxer->stream);
									av_unused int type=GxStream_ReadDword_Le(demuxer->stream);
									int slen=GxStream_ReadDword(demuxer->stream);
									GxDemuxStream_ReadPacket(demuxer->video,demuxer->stream,slen,0,GxStream_Tell(demuxer->stream),0);
									flags|=4;
									gxlogf("Added reference to playlist\n");
									//s[slen]=0;
									//gxlogf("REF: [%.4s] %s\n",&type,s);
									len-=12+slen;i-=12+slen; break;
								}
							case MOV_FOURCC('r','m','d','r'):
								{
									av_unused int flags=GxStream_ReadDword(demuxer->stream);
									av_unused int rate=GxStream_ReadDword(demuxer->stream);
									gxlogf("  min. data rate: %d bits/sec\n",rate);
									len-=8; i-=8; break;
								}
							case MOV_FOURCC('r','m','q','u'):
								{
									av_unused int q=GxStream_ReadDword(demuxer->stream);
									len-=4; i-=4; break;
								}
						}
						i-=len;GxStream_Skip(demuxer->stream,len);
					}
				}
				flags|=1;
				break;
			case MOV_FOURCC('w','i','d','e'):
				gxlogf("MOV: 'WIDE' chunk found!\n");
				if(flags&2) break;
			case MOV_FOURCC('m','d','a','t'):
				gxlogf("MOV: Movie DATA found!\n");
				priv->mdat_start=GxStream_Tell(demuxer->stream);
				priv->mdat_end=priv->mdat_start+len-skipped;
				gxlogf("MOV: Movie data: start: %lld end: %lld\n",
						(int64_t)priv->mdat_start, (int64_t)priv->mdat_end);
				flags|=2;
				if(flags==3){
					// if we're over the headers, then we can stop parsing here!
					demuxer->priv=priv;
					return GX_DEMUXER_TYPE_MOV;
				}
				break;
			case MOV_FOURCC('f','r','e','e'):
			case MOV_FOURCC('s','k','i','p'):
			case MOV_FOURCC('j','u','n','k'):
				gxlogf("MOV: MOV_FREE space (len: %lld)\n", (int64_t)len);
				/* unused, if you edit a mov, you can use space provided by MOV_FREE atoms (redefining it)*/
				break;
			case MOV_FOURCC('p','n','o','t'):
			case MOV_FOURCC('P','I','C','T'):
				/* dunno what, but we shoudl ignore it*/
				break;
			default:
				if(no==0){ MOV_FREE(priv); return 0;} // first chunk is bad!
				id = be2me_32(id);
				gxlogf("MOV: unknown chunk: %.4s %d\n",(char* )&id,(int)len);
		}
		//skip_chunk:
		if(GxStream_Skip(demuxer->stream,len-skipped)!= GX_PLAYER_OK) break;
		++no;
	}

	if(flags==3){
		demuxer->priv=priv;
		return GX_DEMUXER_TYPE_MOV;
	}
	MOV_FREE(demuxer->priv);

	if ((flags==5) || (flags==7)) // reference & header sent
		return GX_DEMUXER_TYPE_PLAYLIST;

	return 0;
}

static void demux_mov_close(GxDemuxer* demuxer) {
	DemuxMovPriv* priv = demuxer->priv;
	int i;

	if (!priv)
		return;
	for (i = 0; i < MOV_MAX_TRACKS; i++) {
		mov_track_t* track = priv->tracks[i];
		if (track) {
			MOV_FREE(track->tkdata);
			MOV_FREE(track->stdata);
			MOV_FREE(track->stream_header);
			MOV_FREE(track->samples);
			MOV_FREE(track->chunks);
			MOV_FREE(track->chunkmap);
			MOV_FREE(track->durmap);
			MOV_FREE(track->keyframes);
			MOV_FREE(track->editlist);
			MOV_FREE(track->samples3);
			MOV_FREE(track);
		}
	}

	MOV_FREE(demuxer->priv);
}

unsigned int store_ughvlc(unsigned char* s, unsigned int v){
	unsigned int n = 0;

	while(v >= 0xff) {
		* s++ = 0xff;
		v -= 0xff;
		n++;
	}
	* s = v;
	n++;

	return n;
}

static void init_vobsub(GxStreamSubHeader* sh, mov_track_t *trak) {
	sh->type = 'v';
	if (trak->stdata_len < 106)
		return;
	sh->extradata_len = 16*4;
	sh->extradata = av_malloc(sh->extradata_len);
	memcpy(sh->extradata, trak->stdata + 42, sh->extradata_len);
}

static int lschunks_intrak(GxDemuxer* demuxer, int level, unsigned int id,
		off_t pos, off_t len, mov_track_t* trak);

static int gen_sh_audio(GxStreamAudioHeader* sh, mov_track_t* trak, int timescale) {
	int version=0, adjust;
	int is_vorbis = 0;
	sh->format=trak->fourcc;

	// crude audio delay from editlist0 hack ::atm
	if(trak->editlist_size>=1) {
		if(trak->editlist[0].pos == -1) {
			sh->stream_delay = (float)trak->editlist[0].dur/(float)timescale;
			gxlogf("MOV: Initial Audio-Delay: %.3f sec\n", sh->stream_delay);
		}
	}


	switch( sh->format ) {
		case 0x726D6173: /* samr*/
			/* amr narrowband*/
			trak->samplebytes=sh->samplesize=1;
			trak->nchannels=sh->channels=1;
			sh->samplerate=8000;
			break;

		case 0x62776173: /* sawb*/
			/* amr wideband*/
			trak->samplebytes=sh->samplesize=1;
			trak->nchannels=sh->channels=1;
			sh->samplerate=16000;
			break;

		default:

			// assumptions for below table: short is 16bit, int is 32bit, intfp is 16bit
			// XXX: 32bit fixed point numbers (intfp) are only 2 Byte!
			// short values are usually one byte leftpadded by zero
			//   int values are usually two byte leftpadded by zero
			//  stdata[]:
			//	8   short	version
			//	10  short	revision
			//	12  int		vendor_id
			//	16  short	channels
			//	18  short	samplesize
			//	20  short	compression_id
			//	22  short	packet_size (==0)
			//	24  intfp	sample_rate
			//     (26  short)	unknown (==0)
			//    ---- qt3.0+ (version>=1)
			//	28  int		samples_per_packet
			//	32  int		bytes_per_packet
			//	36  int		bytes_per_frame
			//	40  int		bytes_per_sample
			// there may be additional atoms following at 28 (version 0)
			// or 44 (version 1), eg. esds atom of .MP4 files
			// esds atom:
			//      28  int		atom size (bytes of int size, int type and data)
			//      32  char[4]	atom type (fourc charater code -> esds)
			//      36  char[]  	atom data (len=size-8)

			// TODO: fix parsing for files using version 2.
			if (trak->stdata_len < 26) {
				gxlogf( "MOV: broken (too small) sound atom!\n");
				return 0;
			}
			version=char2short(trak->stdata,8);
			if (version > 1)
				gxlogf( "MOV: version %d sound atom may not parse correctly!\n", version);
			trak->samplebytes=sh->samplesize=char2short(trak->stdata,18)/8;

			/* I can't find documentation, but so far this is the case. -Corey*/
			switch (char2short(trak->stdata,16)) {
				case 1:
					trak->nchannels = 1; break;
				case 2:
					trak->nchannels = 2; break;
				case 3:
					trak->nchannels = 6; break;
				default:
					gxlogf("MOV: unable to determine audio channels, assuming 2 (got %d)\n",
							char2short(trak->stdata,16));
					trak->nchannels = 2;
			}
			sh->channels = trak->nchannels;

			sh->samplerate=char2short(trak->stdata,24);
			if((sh->samplerate < 7000) && trak->durmap && trak->durmap[0].dur > 1) {
				switch(char2short(trak->stdata,24)/trak->durmap[0].dur) {
					// TODO: add more cases.
					case 31:
						sh->samplerate = 32000; break;
					case 43:
						sh->samplerate = 44100; break;
					case 47:
						sh->samplerate = 48000; break;
					default:
						gxlogf(
								"MOV: unable to determine audio samplerate, "
								"assuming 44.1kHz (got %d)\n",
								char2short(trak->stdata,24)/trak->durmap[0].dur);
						sh->samplerate = 44100;
				}
		}
	}

	gxlogf( "Audio bits: %d  chans: %d  rate: %d\n",sh->samplesize*8,sh->channels,sh->samplerate);

	if(trak->stdata_len >= 44 && trak->stdata[9]>=1){
		gxlogf("Audio header: samp/pack=%d bytes/pack=%d bytes/frame=%d bytes/samp=%d  \n",
				char2int(trak->stdata,28),
				char2int(trak->stdata,32),
				char2int(trak->stdata,36),
				char2int(trak->stdata,40));
		if(trak->stdata_len>=44+8){
			int len=char2int(trak->stdata,44);
			int fcc=char2int(trak->stdata,48);
			fcc = fcc;
			// we have extra audio headers!!!
			gxlogf("Audio extra header: len=%d  fcc=0x%X\n",len,fcc);
			if((len >= 4) &&
					(char2int(trak->stdata,52) >= 12) &&
					(char2int(trak->stdata,52+4) == MOV_FOURCC('f','r','m','a'))) {
				switch(char2int(trak->stdata,52+8)) {
					case MOV_FOURCC('a','l','a','c'):
						if (len >= 36 + char2int(trak->stdata,52)) {
							sh->codecdata_len = char2int(trak->stdata,52+char2int(trak->stdata,52));
							gxlogf( "MOV: Found alac atom (%d)!\n", sh->codecdata_len);
							sh->codecdata = av_malloc(sh->codecdata_len);
							CHECK_POINT(sh->codecdata);
							memcpy(sh->codecdata, &trak->stdata[52+char2int(trak->stdata,52)], sh->codecdata_len);
						}
						break;
					case MOV_FOURCC('i','n','2','4'):
					case MOV_FOURCC('i','n','3','2'):
					case MOV_FOURCC('f','l','3','2'):
					case MOV_FOURCC('f','l','6','4'):
						if ((len >= 22) &&
								(char2int(trak->stdata,52+16)==MOV_FOURCC('e','n','d','a')) &&
								(char2short(trak->stdata,52+20))) {
							sh->format=char2int(trak->stdata,52+8);
							gxlogf( "MOV: Found little endian PCM data, reversed fourcc:%04x\n", sh->format);
						}
						break;
					default:
						if (len > 8 && len + 44 <= trak->stdata_len) {
							sh->codecdata_len = len-8;
							sh->codecdata = av_malloc(sh->codecdata_len);
							CHECK_POINT(sh->codecdata);
							memcpy(sh->codecdata, trak->stdata+44+8, sh->codecdata_len);
						}
				}
			} else {
				if (len > 8 && len + 44 <= trak->stdata_len) {
					sh->codecdata_len = len-8;
					sh->codecdata = av_malloc(sh->codecdata_len);
					CHECK_POINT(sh->codecdata);
					memcpy(sh->codecdata, trak->stdata+44+8, sh->codecdata_len);
				}
			}
		}
	}

	switch (version) {
		case 0:
			adjust = 0; break;
		case 1:
			adjust = 48; break;
		case 2:
			adjust = 68; break;
		default:
			gxlogf( "MOV: unknown sound atom version (%d); may not work!\n", version);
			adjust = 68;
	}
	if (trak->stdata_len >= 36 + adjust) {
		int atom_len = char2int(trak->stdata,28+adjust);
		if (atom_len < 0 || atom_len > trak->stdata_len - 28 - adjust) atom_len = trak->stdata_len - 28 - adjust;
		switch(char2int(trak->stdata,32+adjust)) { // atom type
			case MOV_FOURCC('e','s','d','s'):
				{
					gxlogf( "MOV: Found MPEG4 audio Elementary Stream Descriptor atom (%d)!\n", atom_len);
					if(atom_len > 8) {
						esds_t esds;
						if(!mp4_parse_esds(&trak->stdata[36+adjust], atom_len-8, &esds)) {
							/* 0xdd is a "user private" id, not an official allocated id (see http://www.mp4ra.org/object.html),
							   so perform some extra checks to be sure that this is really vorbis audio*/
							if(esds.objectTypeId==0xdd && esds.streamType==0x15 && sh->format==0x6134706D && esds.decoderConfigLen > 8)
							{
								//vorbis audio
								unsigned char* buf[3];
								unsigned short sizes[3];
								int offset, len, k;
								unsigned char* ptr = esds.decoderConfig;

								if(ptr[0] != 0 || ptr[1] != 30) goto quit_vorbis_block; //wrong extradata layout

								offset = len = 0;
								for(k = 0; k < 3; k++)
								{
									sizes[k] = (ptr[offset]<<8) | ptr[offset+1];
									len += sizes[k];
									offset += 2;
									if(offset + sizes[k] > esds.decoderConfigLen)
									{
										gxlogd( "MOV: ERROR!, not enough vorbis extradata to read: offset = %d, k=%d, size=%d, len: %d\n", offset, k, sizes[k], esds.decoderConfigLen);
										goto quit_vorbis_block;
									}
									buf[k] = av_malloc(sizes[k]);
									if(!buf[k]) goto quit_vorbis_block;
									memcpy(buf[k], &ptr[offset], sizes[k]);
									offset += sizes[k];
								}

								sh->codecdata_len = len + len/255 + 64;
								sh->codecdata = av_malloc(sh->codecdata_len);
								CHECK_POINT(sh->codecdata);
								ptr = sh->codecdata;

								ptr[0] = 2;
								offset = 1;
								offset += store_ughvlc(&ptr[offset], sizes[0]);
								offset += store_ughvlc(&ptr[offset], sizes[1]);
								for(k = 0; k < 3; k++)
								{
									memcpy(&ptr[offset], buf[k], sizes[k]);
									offset += sizes[k];
								}

								sh->codecdata_len = offset;
								sh->codecdata = av_realloc(sh->codecdata, offset);
								gxlogf( "demux_mov, vorbis extradata size: %d\n", offset);
								is_vorbis = 1;
quit_vorbis_block:
								sh->format = GX_FOURCC('v', 'r', 'b', 's');
							}
							sh->i_bps = esds.avgBitrate/8;

							if(esds.objectTypeId==MP4OTI_MPEG1Audio || esds.objectTypeId==MP4OTI_MPEG2AudioPart3)
								sh->format=0x55; // .mp3

							if(esds.objectTypeId==MP4OTI_13kVoice) { // 13K Voice, defined by 3GPP2
								sh->format=GX_FOURCC('Q', 'c', 'l', 'p');
								trak->nchannels=sh->channels=1;
								trak->samplebytes=sh->samplesize=1;
							}

							// dump away the codec specific configuration for the AAC decoder
							if(esds.decoderConfigLen){
								if( (esds.decoderConfig[0]>>3) == 29 )
									sh->format = 0x1d61346d; // request multi-channel mp3 decoder
								if(!is_vorbis)
								{
									sh->codecdata_len = esds.decoderConfigLen;
									sh->codecdata = av_malloc(sh->codecdata_len);
									CHECK_POINT(sh->codecdata);
									memcpy(sh->codecdata, esds.decoderConfig, sh->codecdata_len);
								}
							}
						}
						mp4_free_esds(&esds); // freeup esds mem

						if (sh->format == GX_FOURCC('M', 'P', '4', 'A')||
								sh->format == GX_FOURCC('m', 'p', '4', 'a'))
						{
			                MPEG4AudioConfig cfg;
			                mpeg4audio_get_config(&cfg, sh->codecdata,
			                                         sh->codecdata_len);
			                if (cfg.chan_config > 7)
			                    return -1;
			                sh->channels = mpeg4audio_channels[cfg.chan_config];
			                sh->samplerate = cfg.sample_rate;
			            }
					}
				} break;
			case MOV_FOURCC('a','l','a','c'):
				{
					gxlogf( "MOV: Found alac atom (%d)!\n", atom_len);
					if(atom_len > 8) {
						// copy all the atom (not only payload) for lavc alac decoder
						sh->codecdata_len = atom_len;
						sh->codecdata = av_malloc(sh->codecdata_len);
						CHECK_POINT(sh->codecdata);
						memcpy(sh->codecdata, &trak->stdata[28], sh->codecdata_len);
					}
				} break;
			case MOV_FOURCC('d','a','m','r'):
				gxlogf( "MOV: Found AMR audio atom %c%c%c%c (%d)!\n", trak->stdata[32+adjust],trak->stdata[33+adjust],trak->stdata[34+adjust],trak->stdata[35+adjust], atom_len);
				if (atom_len>14) {
					gxlogf( "mov: vendor: %c%c%c%c Version: %d\n",trak->stdata[36+adjust],trak->stdata[37+adjust],trak->stdata[38+adjust], trak->stdata[39+adjust],trak->stdata[40+adjust]);
					gxlogf( "MOV: Modes set: %02x%02x\n",trak->stdata[41+adjust],trak->stdata[42+adjust]);
					gxlogf( "MOV: Mode change period: %d Frames per sample: %d\n",trak->stdata[43+adjust],trak->stdata[44+adjust]);
				}
				break;
			default:
				gxlogf( "MOV: Found unknown audio atom %c%c%c%c (%d)!\n",
						trak->stdata[32+adjust],trak->stdata[33+adjust],trak->stdata[34+adjust],trak->stdata[35+adjust],
						atom_len);
		}
	}
	gxlogf( "Fourcc: %.4s\n",(char* )&trak->fourcc);

	// Emulate WAVEFORMATEX struct:
	sh->wf=av_malloc(sizeof(WAVEFORMATEX) + (is_vorbis ? sh->codecdata_len : 0));
	CHECK_POINT(sh->wf);
	memset(sh->wf,0,sizeof(WAVEFORMATEX));
	sh->wf->nChannels=sh->channels;
	sh->wf->wBitsPerSample=(trak->stdata[18]<<8)+trak->stdata[19];
	// sh->wf->nSamplesPerSec=trak->timescale;
	sh->wf->nSamplesPerSec=sh->samplerate;
	if(trak->stdata_len >= 44 && trak->stdata[9]>=1 && char2int(trak->stdata,28)>0){
		sh->wf->nAvgBytesPerSec=(sh->wf->nChannels*sh->wf->nSamplesPerSec*
				char2int(trak->stdata,32)+char2int(trak->stdata,28)/2)
			/char2int(trak->stdata,28)/8;
		sh->wf->nBlockAlign=char2int(trak->stdata,36);
	} else {
		sh->wf->nAvgBytesPerSec=sh->wf->nChannels*sh->wf->wBitsPerSample*sh->wf->nSamplesPerSec/8/8;
		// workaround for ms11 ima4
		if (sh->format == 0x1100736d && trak->stdata_len >= 36)
			sh->wf->nBlockAlign=char2int(trak->stdata,36);
	}

	if(is_vorbis && sh->codecdata_len)
	{
		memcpy(sh->wf+1, sh->codecdata, sh->codecdata_len);
		sh->wf->cbSize = sh->codecdata_len;
	}

	if(trak->fourcc){
		strncpy(sh->priv.codec, (char*)&trak->fourcc, sizeof(sh->priv.codec) - 1);
	}

	return 1;
}

static void write_nal(GxDemuxPacket*  dp, const uint8_t * data, int *ppos, int data_size, int nal_size_size)
{
	int i;
	int nal_size = 0;

	int pos =* ppos;

	if(nal_size_size)
	{
		for (i = 0; i < nal_size_size; ++i)
			nal_size = (nal_size << 8) | data[pos++];

		if ((pos + nal_size) > data_size){
			gxlogd("Track %d: nal too big\n", 0);
			//GxDemuxPacket_Resize(dp,dp->len + pos + nal_size - data_size);
			nal_size = data_size - pos - 4;
			//return ;
		}
		GxDemuxPacket_Write(dp, start_code, 4);
	}
	else
		nal_size = data_size-pos;

	GxDemuxPacket_Write(dp, data + pos, nal_size);

	pos += nal_size;

	*ppos = pos;
}

static void handle_firstframe(DemuxMovPriv*  mov, mov_track_t * track, GxDemuxPacket * dp)
{
	int priv_size = track->stream_header_len;
	uint8_t* buf = (uint8_t *) track->stream_header;

	int i, pos = 6, numsps, numpps;

	if (buf == NULL){
		return ;
	}

	mov->nal_size_size = 1 + (buf[4] & 3);

	numsps = buf[5] & 0x1f;
	gxlogf("priva_size=%d, nal_size_size=%d, numsps=%d\n", priv_size, mov->nal_size_size, numsps);

	for (i = 0; (i < numsps) && (priv_size > pos); i++)
		write_nal(dp, buf, &pos, priv_size, 2);

	if (priv_size <= pos)
		return;

	numpps = buf[pos++];

	for (i = 0; (i < numpps) && (priv_size > pos); i++)
		write_nal(dp, buf, &pos, priv_size, 2);
}


static int gen_sh_video(GxStreamVideoHeader* sh, mov_track_t* trak, GxDemuxer* demuxer) {
	int depth;
	int flag, start, count_flag, end, palette_count, gray;
	int hdr_ptr = 76;  // the byte just after depth
	unsigned char* palette_map;
	DemuxMovPriv*  priv = demuxer->priv;

	int timescale = priv->timescale;

	sh->format=trak->fourcc;

	// crude video delay from editlist0 hack ::atm
	if(trak->editlist_size>=1) {
		if(trak->editlist[0].pos == -1) {
			sh->stream_delay = (float)trak->editlist[0].dur/(float)timescale;
			gxlogf("MOV: Initial Video-Delay: %.3f sec\n", sh->stream_delay);
		}
	}


	if (trak->stdata_len < 78) {
		gxlogf("MOV: Invalid (%d bytes instead of >= 78) video trak desc\n",trak->stdata_len);
		return 0;
	}

	depth = trak->stdata[75] | (trak->stdata[74] << 8);
	if (trak->fourcc == GX_FOURCC('r', 'a', 'w', ' '))
		sh->format = IMGFMT_RGB | depth;

	//  stdata[]:
	//	8   short	version
	//	10  short	revision
	//	12  int		vendor_id
	//	16  int		temporal_quality
	//	20  int		spatial_quality
	//	24  short	width
	//	26  short	height
	//	28  int		h_dpi
	//	32  int		v_dpi
	//	36  int		0
	//	40  short	frames_per_sample
	//	42  char[4]	compressor_name
	//	74  short	depth
	//	76  short	color_table_id
	// additional atoms may follow,
	// eg esds atom from .MP4 files
	//      78  int		atom size
	//      82  char[4]	atom type
	//	86  ...		atom data
#if 0
	{	ImageDescription* id=av_malloc(8+trak->stdata_len);  // safe
		trak->desc=id;
		id->idSize=8+trak->stdata_len;
		//		id->cType=bswap_32(trak->fourcc);
		id->cType=le2me_32(trak->fourcc);
		id->version=char2short(trak->stdata,8);
		id->revisionLevel=char2short(trak->stdata,10);
		id->vendor=char2int(trak->stdata,12);
		id->temporalQuality=char2int(trak->stdata,16);
		id->spatialQuality=char2int(trak->stdata,20);
		id->width=char2short(trak->stdata,24);
		id->height=char2short(trak->stdata,26);
		id->hRes=char2int(trak->stdata,28);
		id->vRes=char2int(trak->stdata,32);
		id->dataSize=char2int(trak->stdata,36);
		id->frameCount=char2short(trak->stdata,40);
		memcpy(&id->name,trak->stdata+42,32);
		id->depth=char2short(trak->stdata,74);
		id->clutID=char2short(trak->stdata,76);
		if(trak->stdata_len>78)	memcpy(((char*)&id->clutID)+2,trak->stdata+78,trak->stdata_len-78);
		sh->ImageDesc=id;
	}
#endif
	if(trak->stdata_len >= 86) { // extra atoms found
		int pos=78;
		int atom_len;
		while(pos+8<=trak->stdata_len &&
				(pos+(atom_len=char2int(trak->stdata,pos)))<=trak->stdata_len){
			switch(char2int(trak->stdata,pos+4)) { // switch atom type
				case MOV_FOURCC('g','a','m','a'):
					// intfp with gamma value at which movie was captured
					// can be used to gamma correct movie display
					gxlogf( "MOV: Found unsupported Gamma-Correction movie atom (%d)!\n",
							atom_len);
					break;
				case MOV_FOURCC('f','i','e','l'):
					// 2 char-values (8bit int) that specify field handling
					// see the Apple's QuickTime Fileformat PDF for more info
					gxlogf( "MOV: Found unsupported Field-Handling movie atom (%d)!\n",
							atom_len);
					break;
				case MOV_FOURCC('m','j','q','t'):
					// Motion-JPEG default quantization table
					gxlogf( "MOV: Found unsupported MJPEG-Quantization movie atom (%d)!\n",
							atom_len);
					break;
				case MOV_FOURCC('m','j','h','t'):
					// Motion-JPEG default huffman table
					gxlogf( "MOV: Found unsupported MJPEG-Huffman movie atom (%d)!\n",
							atom_len);
					break;
				case MOV_FOURCC('e','s','d','s'):
					// MPEG4 Elementary Stream Descriptor header
					gxlogf( "MOV: Found MPEG4 movie Elementary Stream Descriptor atom (%d)!\n", atom_len);
					// add code here to save esds header of length atom_len-8
					// beginning at stdata[86] to some variable to pass it
					// on to the decoder ::atmos
					if(atom_len > 8) {
						esds_t esds;
						if(!mp4_parse_esds(trak->stdata+pos+8, atom_len-8, &esds)) {

							if(esds.objectTypeId==MP4OTI_MPEG2VisualSimple || esds.objectTypeId==MP4OTI_MPEG2VisualMain ||
									esds.objectTypeId==MP4OTI_MPEG2VisualSNR || esds.objectTypeId==MP4OTI_MPEG2VisualSpatial ||
									esds.objectTypeId==MP4OTI_MPEG2VisualHigh || esds.objectTypeId==MP4OTI_MPEG2Visual422)
								sh->format=GX_FOURCC('m', 'p', 'g', '2');
							else if(esds.objectTypeId==MP4OTI_MPEG1Visual)
								sh->format=GX_FOURCC('m', 'p', 'g', '1');

							// dump away the codec specific configuration for the AAC decoder
							trak->stream_header_len = esds.decoderConfigLen;
							trak->stream_header = av_malloc(trak->stream_header_len);
							CHECK_POINT(trak->stream_header);
							memcpy(trak->stream_header, esds.decoderConfig, trak->stream_header_len);
						}
						mp4_free_esds(&esds); // freeup esds mem
					}
					break;
				case MOV_FOURCC('a','v','c','C'):
					{
						// AVC decoder configuration record
						gxlogf( "MOV: AVC decoder configuration record atom (%d)!\n", atom_len);
						if(atom_len > 8) {
							int i, poffs, cnt=0;
							// Parse some parts of avcC, just for fun :)
							// real parsing is done by avc1 decoder
							gxlogf( "MOV: avcC version: %d\n",* (trak->stdata+pos+8));
							if (*(trak->stdata+pos+8) != 1)
								gxlogd ("MOV: unknown avcC version (%d). Expexct problems.\n",* (trak->stdata+pos+9));
							gxlogf( "MOV: avcC profile: %d\n",* (trak->stdata+pos+9));
							gxlogf( "MOV: avcC profile compatibility: %d\n",* (trak->stdata+pos+10));
							gxlogf( "MOV: avcC level: %d\n",* (trak->stdata+pos+11));
							gxlogf( "MOV: avcC nal length size: %d\n", ((*(trak->stdata+pos+12))&0x03)+1);
							priv->nal_size_size = ((*(trak->stdata+pos+12))&0x03)+1;
							gxlogf( "MOV: avcC number of sequence param sets: %d\n", cnt = (*(trak->stdata+pos+13) & 0x1f));
							poffs = pos + 14;
							for (i = 0; i < cnt; i++) {
								gxlogf( "MOV: avcC sps %d have length %d\n", i, AV_RB16(trak->stdata+poffs));
								poffs += AV_RB16(trak->stdata+poffs) + 2;
							}
							gxlogf( "MOV: avcC number of picture param sets: %d\n",* (trak->stdata+poffs));
							poffs++;
							for (i = 0; i < cnt; i++) {
								gxlogf( "MOV: avcC pps %d have length %d\n", i, AV_RB16(trak->stdata+poffs));
								poffs += AV_RB16(trak->stdata+poffs) + 2;
							}
							// Copy avcC for the AVC decoder
							// This data will be put in extradata below, where BITMAPINFOHEADER is created
							trak->stream_header_len = atom_len-8;
							trak->stream_header = av_malloc(trak->stream_header_len);
							CHECK_POINT(trak->stream_header);
							memcpy(trak->stream_header, trak->stdata+pos+8, trak->stream_header_len);
							//TODO liufei
							GxDemuxPacket* dp = GxDemuxPacket_Create(demuxer, NULL, trak->stream_header_len);
							if (dp) {
								handle_firstframe(priv, trak, dp);
								if(dp->write_size != dp->len);
								dp = GxDemuxPacket_Resize(demuxer, dp,dp->write_size);
								sh->priv.header.data = av_malloc(dp->write_size);
								sh->priv.header.len  = dp->write_size;
								if(sh->priv.header.data != NULL)
								{
									memcpy(sh->priv.header.data,dp->buffer,dp->write_size);
									GxDemuxPacket_Destroy(dp);
								}
								else
								{
									GxDemuxPacket_Destroy(dp);
									return 0;
								}
							}

						}
					}
					break;
				case MOV_FOURCC('d','2','6','3'):
					gxlogf( "MOV: Found H.263 decoder atom %c%c%c%c (%d)!\n", trak->stdata[pos+4],trak->stdata[pos+5],trak->stdata[pos+6],trak->stdata[pos+7],atom_len);
					if (atom_len>10)
						gxlogf( "MOV: Vendor: %c%c%c%c H.263 level: %d H.263 profile: %d \n", trak->stdata[pos+8],trak->stdata[pos+9],trak->stdata[pos+10],trak->stdata[pos+11],trak->stdata[pos+12],trak->stdata[pos+13]);
					break;
				case 0:
					break;
				default:
					gxlogf( "MOV: Found unknown movie atom %c%c%c%c (%d)!\n",
							trak->stdata[pos+4],trak->stdata[pos+5],trak->stdata[pos+6],trak->stdata[pos+7],
							atom_len);
			}
			if(atom_len<8) break;
			pos+=atom_len;
			//		   gxlogd("pos=%d max=%d\n",pos,trak->stdata_len);
		}
	}
	sh->fps=trak->timescale/((trak->durmap_size>=1)?(float)trak->durmap[0].dur:1);
	sh->frametime=1.0f/sh->fps;

	sh->disp_w=trak->stdata[25]|(trak->stdata[24]<<8);
	sh->disp_h=trak->stdata[27]|(trak->stdata[26]<<8);
	if(trak->tkdata_len>81) {
		// if image size is zero, fallback to display size
		if(!sh->disp_w && !sh->disp_h) {
			sh->disp_w=trak->tkdata[77]|(trak->tkdata[76]<<8);
			sh->disp_h=trak->tkdata[81]|(trak->tkdata[80]<<8);
		} else if(sh->disp_w!=(trak->tkdata[77]|(trak->tkdata[76]<<8))){
			// codec and display width differ... use display one for aspect
			sh->aspect=trak->tkdata[77]|(trak->tkdata[76]<<8);
			sh->aspect/=trak->tkdata[81]|(trak->tkdata[80]<<8);
		}
	}
	if(!sh->aspect)
		sh->aspect = (float)sh->disp_w/sh->disp_h;

	if(depth>32+8) gxlogf("*** depth = 0x%X\n",depth);

	// palettized?
	gray = 0;
	if (depth > 32) { depth&=31; gray = 1; } // depth > 32 means grayscale
	if ((depth == 2) || (depth == 4) || (depth == 8))
		palette_count = (1 << depth);
	else
		palette_count = 0;

	// emulate BITMAPINFOHEADER:
	if (palette_count)
	{
		sh->bih=av_malloc(sizeof(BITMAPINFOHEADER) + palette_count*  4);
		CHECK_POINT(sh->bih);
		memset(sh->bih,0,sizeof(BITMAPINFOHEADER) + palette_count*  4);
		sh->bih->biSize=40 + palette_count*  4;
		// fetch the relevant fields
		flag = AV_RB16(&trak->stdata[hdr_ptr]);
		hdr_ptr += 2;
		start = AV_RB32(&trak->stdata[hdr_ptr]);
		hdr_ptr += 4;
		count_flag = AV_RB16(&trak->stdata[hdr_ptr]);
		hdr_ptr += 2;
		end = AV_RB16(&trak->stdata[hdr_ptr]);
		hdr_ptr += 2;
		palette_map = (unsigned char* )sh->bih + 40;
		gxlogf( "Allocated %d entries for palette\n",
				palette_count);
		gxlogf( "QT palette: start: %x, end: %x, count flag: %d, flags: %x\n",
				start, end, count_flag, flag);
	}
	else
	{
		if (trak->fourcc == GX_FOURCC('a','v','c','1'))
		{
			if (trak->stream_header_len > 0xffffffff - sizeof(BITMAPINFOHEADER))
			{
				gxlogd( "Invalid extradata size %d, skipping\n",trak->stream_header_len);
				trak->stream_header_len = 0;
			}
			sh->bih=av_malloc(sizeof(BITMAPINFOHEADER) + trak->stream_header_len);
			CHECK_POINT(sh->bih);
			memset(sh->bih,0,sizeof(BITMAPINFOHEADER) + trak->stream_header_len);
			sh->bih->biSize=40  + trak->stream_header_len;
			memcpy(((unsigned char* )sh->bih)+40,  trak->stream_header, trak->stream_header_len);
			MOV_FREE (trak->stream_header);
			trak->stream_header_len = 0;
			trak->stream_header = NULL;
		}
		else
		{
			sh->bih=av_malloc(sizeof(BITMAPINFOHEADER));
			CHECK_POINT(sh->bih);
			memset(sh->bih,0,sizeof(BITMAPINFOHEADER));
			sh->bih->biSize=40;
		}
	}
	sh->bih->biWidth=sh->disp_w;
	sh->bih->biHeight=sh->disp_h;
	sh->bih->biPlanes=0;
	sh->bih->biBitCount=depth;
	sh->bih->biCompression=trak->fourcc;
	sh->bih->biSizeImage=sh->bih->biWidth*sh->bih->biHeight;

	gxlogf( "Image size: %d x %d (%d bpp)\n",sh->disp_w,sh->disp_h,sh->bih->biBitCount);
	if(trak->tkdata_len>81)
		gxlogf( "Display size: %d x %d\n",trak->tkdata[77]|(trak->tkdata[76]<<8),trak->tkdata[81]|(trak->tkdata[80]<<8));
	gxlogf( "Fourcc: %.4s  Codec: '%.*s'\n",(char* )&trak->fourcc,trak->stdata[42]&31,trak->stdata+43);

	if(trak->fourcc){
		strncpy(sh->priv.codec, (char*)&trak->fourcc, sizeof(sh->priv.codec) - 1);
	}

	return 1;
}

static int lschunks(GxDemuxer* demuxer,int level,off_t endpos,mov_track_t* trak){
	DemuxMovPriv* priv=demuxer->priv;

	while(1){
		off_t pos;
		off_t len;
		unsigned int id;
		//
		pos=GxStream_Tell(demuxer->stream);

		if(pos>=endpos) return -1; // END
		len=GxStream_ReadDword(demuxer->stream);

		if(len<8) return -1; // error
		len-=8;
		id=GxStream_ReadDword(demuxer->stream);

		if(trak){
			if (lschunks_intrak(demuxer, level, id, pos, len, trak) < 0)
				return -1;
		}
		else
		{ /* not in track*/
			switch(id) {
				case MOV_FOURCC('m','v','h','d'):
					{
						int version = GxStream_ReadChar(demuxer->stream);
						GxStream_Skip(demuxer->stream, (version == 1) ? 19 : 11);
						priv->timescale=GxStream_ReadDword(demuxer->stream);
						if (version == 1)
							priv->duration=GxStream_ReadQword(demuxer->stream);
						else
							priv->duration=GxStream_ReadDword(demuxer->stream);
						gxlogf("MOV: %*sMovie header (%d bytes): tscale=%d  dur=%d\n",level,"",(int)len,
								(int)priv->timescale,(int)priv->duration);
						break;
					}

				case MOV_FOURCC('t','r','a','k'):
					{
						if(priv->track_db>=MOV_MAX_TRACKS){
							return -1;
						}
						if(!priv->track_db)
							gxlogf( "--------------\n");
						trak=av_malloc(sizeof(mov_track_t));
						CHECK_POINT(trak);
						memset(trak,0,sizeof(mov_track_t));
						gxlogf("MOV: Track #%d:\n",priv->track_db);
						trak->id=priv->track_db;
						priv->tracks[priv->track_db]=trak;
						if(lschunks(demuxer,level+1,pos+len,trak)<0)
							return -1;
						if(mov_build_index(trak,priv->timescale)<0)
							return -1;
						switch(trak->type){
							case MOV_TRAK_AUDIO:
								{
									GxStreamAudioHeader* sh=GxStreamHeader_AudioNew(demuxer,priv->track_db,priv->track_db);
									gen_sh_audio(sh, trak, priv->timescale);
									break;
								}

							case MOV_TRAK_VIDEO:
								{
									GxStreamVideoHeader* sh=GxStreamHeader_VideoNew(demuxer,priv->track_db,priv->track_db);
									gen_sh_video(sh, trak, demuxer);
									break;
								}
							case MOV_TRAK_GENERIC:
								if (trak->fourcc == GX_FOURCC('m','p','4','s') ||
										trak->fourcc == GX_FOURCC('t','x','3','g') ||
										trak->fourcc == GX_FOURCC('t','e','x','t')) {
									GxStreamSubHeader* sh = GxStreamHeader_SubNew(demuxer, priv->track_db,priv->track_db);
									if (trak->fourcc == GX_FOURCC('m','p','4','s'))
										init_vobsub(sh, trak);
									else {
										sh->type = 'm';
										//sub_utf8 = 1;
									}
								} else
									gxlogf( "Generic track - not completely understood! (id: %d)\n",
											trak->id);
								/* XXX: Also this contains the FLASH data*/

								break;
							default:
								gxlogf( "Unknown track type found (type: %d)\n", trak->type);
								break;
						}
						gxlogf( "--------------\n");
						priv->track_db++;
						trak=NULL;
						break;
					}
#ifndef CONFIG_ZLIB
				case MOV_FOURCC('c','m','o','v'):
					{
						return -1;
					}
#else
				case MOV_FOURCC('m','o','o','v'):
				case MOV_FOURCC('c','m','o','v'):
					{
						ret = lschunks(demuxer,level+1,pos+len,NULL);
						CHECK_RET(ret);
						break;
					}
				case MOV_FOURCC('d','c','o','m'):
					{
						unsigned int algo=GxStream_ReadDword(demuxer->stream);
						algo = be2me_32(algo);
						gxlogf( "Compressed header uses %.4s algo!\n",(char* )&algo);
						break;
					}
				case MOV_FOURCC('c','m','v','d'):
					{
						unsigned int moov_sz=GxStream_ReadDword(demuxer->stream);
						unsigned int cmov_sz=len-4;
						unsigned char* cmov_buf;
						unsigned char* moov_buf;
						int zret;
						z_stream zstrm;
						GxStream* backup;

						if (moov_sz > UINT_MAX - 16) {
							gxlogd( "Invalid cmvd atom size %d\n", moov_sz);
							break;
						}
						cmov_buf=av_malloc(cmov_sz);
						moov_buf=av_malloc(moov_sz+16);
						CHECK_POINT(moov_buf);
						CHECK_POINT(cmov_buf);
						gxlogf( "Compressed header size: %d / %d\n",cmov_sz,moov_sz);

						GxStream_Read(demuxer->stream,cmov_buf,cmov_sz);

						zstrm.zalloc          = (alloc_func)0;
						zstrm.zfree           = (free_func)0;
						zstrm.opaque          = (voidpf)0;
						zstrm.next_in         = cmov_buf;
						zstrm.avail_in        = cmov_sz;
						zstrm.next_out        = moov_buf;
						zstrm.avail_out       = moov_sz;

						zret = inflateInit(&zstrm);
						if (zret != Z_OK)
						{ gxlogd( "QT cmov: inflateInit err %d\n",zret);
							return;
						}
						zret = inflate(&zstrm, Z_NO_FLUSH);
						if ((zret != Z_OK) && (zret != Z_STREAM_END))
						{ gxlogd( "QT cmov inflate: ERR %d\n",zret);
							return;
						}
#if 0
						else {
							FILE* DecOut;
							DecOut = fopen("Out.bin", "w");
							fwrite(moov_buf, 1, moov_sz, DecOut);
							fclose(DecOut);
						}
#endif
						if(moov_sz != zstrm.total_out)
							gxlogf( "Warning! moov size differs cmov: %d  zlib: %ld\n",moov_sz,zstrm.total_out);
						zret = inflateEnd(&zstrm);

						backup=demuxer->stream;
						demuxer->stream=new_memory_stream(moov_buf,moov_sz);
						GxStream_Skip(demuxer->stream,8);
						lschunks(demuxer,level+1,moov_sz,NULL); // parse uncompr. 'moov'
						CHECK_RET(ret);
						demuxer->stream=backup;
						MOV_FREE(cmov_buf);
						MOV_FREE(moov_buf);
						break;
					}
#endif
				case MOV_FOURCC('u','d','t','a'):
					{
						unsigned int udta_id;
						off_t udta_len;
						off_t udta_size = len;

						gxlogf( "mov: user data record found\n");
						gxlogf( "Quicktime Clip Info:\n");

						while((len > 8) && (udta_size > 8))
						{
							udta_len = GxStream_ReadDword(demuxer->stream);
							udta_id = GxStream_ReadDword(demuxer->stream);
							udta_size -= 8;
							gxlogf( "udta_id: %.4s (len: %lld)\n", (char* )&udta_id, (int64_t)udta_len);
							switch (udta_id)
							{
								case MOV_FOURCC(0xa9,'c','p','y'):
								case MOV_FOURCC(0xa9,'d','a','y'):
								case MOV_FOURCC(0xa9,'d','i','r'):
									/* 0xa9,'e','d','1' - '9' : edit timestamps*/
								case MOV_FOURCC(0xa9,'f','m','t'):
								case MOV_FOURCC(0xa9,'i','n','f'):
								case MOV_FOURCC(0xa9,'p','r','d'):
								case MOV_FOURCC(0xa9,'p','r','f'):
								case MOV_FOURCC(0xa9,'r','e','q'):
								case MOV_FOURCC(0xa9,'s','r','c'):
								case MOV_FOURCC('n','a','m','e'):
								case MOV_FOURCC(0xa9,'n','a','m'):
								case MOV_FOURCC(0xa9,'A','R','T'):
								case MOV_FOURCC(0xa9,'c','m','t'):
								case MOV_FOURCC(0xa9,'a','u','t'):
								case MOV_FOURCC(0xa9,'s','w','r'):
									{
										off_t text_len = GxStream_ReadWord(demuxer->stream);
										char text[text_len+2+1];
										GxStream_Read(demuxer->stream, (uint8_t* )&text, text_len+2);
										text[text_len+2] = 0x0;
										switch(udta_id)
										{
											case MOV_FOURCC(0xa9,'a','u','t'):
												gxlogf( " Author: %s\n", &text[2]);
												break;
											case MOV_FOURCC(0xa9,'c','p','y'):
												gxlogf( " Copyright: %s\n", &text[2]);
												break;
											case MOV_FOURCC(0xa9,'i','n','f'):
												gxlogf( " Info: %s\n", &text[2]);
												break;
											case MOV_FOURCC('n','a','m','e'):
											case MOV_FOURCC(0xa9,'n','a','m'):
												gxlogf("title :%s\n", &text[2]);
												gxlogf( " Name: %s\n", &text[2]);
												break;
											case MOV_FOURCC(0xa9,'A','R','T'):
												gxlogf( " Artist: %s\n", &text[2]);
												break;
											case MOV_FOURCC(0xa9,'d','i','r'):
												gxlogf( " Director: %s\n", &text[2]);
												break;
											case MOV_FOURCC(0xa9,'c','m','t'):
												gxlogf( " Comment: %s\n", &text[2]);
												break;
											case MOV_FOURCC(0xa9,'r','e','q'):
												gxlogf( " Requirements: %s\n", &text[2]);
												break;
											case MOV_FOURCC(0xa9,'s','w','r'):
												gxlogf("encoder :%s\n", &text[2]);
												gxlogf( " Software: %s\n", &text[2]);
												break;
											case MOV_FOURCC(0xa9,'d','a','y'):
												gxlogf( " Creation timestamp: %s\n", &text[2]);
												break;
											case MOV_FOURCC(0xa9,'f','m','t'):
												gxlogf( " Format: %s\n", &text[2]);
												break;
											case MOV_FOURCC(0xa9,'p','r','d'):
												gxlogf( " Producer: %s\n", &text[2]);
												break;
											case MOV_FOURCC(0xa9,'p','r','f'):
												gxlogf( " Performer(s): %s\n", &text[2]);
												break;
											case MOV_FOURCC(0xa9,'s','r','c'):
												gxlogf( " Source providers: %s\n", &text[2]);
												break;
										}
										udta_size -= 4+text_len;
										break;
									}
									/* some other shits:    WLOC - window location,
									   LOOP - looping style,
									   SelO - play only selected frames
									   AllF - play all frames
									   */
								case MOV_FOURCC('W','L','O','C'):
								case MOV_FOURCC('L','O','O','P'):
								case MOV_FOURCC('S','e','l','O'):
								case MOV_FOURCC('A','l','l','F'):
								default:
									{
										if( udta_len>udta_size)
											udta_len=udta_size;
										{
											GxStream_Skip(demuxer->stream, udta_len-4-4);
											udta_size -= udta_len;
										}
									}
							}
						}
						break;
					} /* eof udta*/
				default:
					id = be2me_32(id);
					gxlogf("MOV: unknown chunk: %.4s %d\n",(char* )&id,(int)len);
			} /* endof switch*/
		} /* endof else*/

		pos+=len+8;
		if(pos>=endpos)
			break;
		if(GxStream_Seek(demuxer->stream,pos)!=GX_PLAYER_OK)
			break;
	}

	return 0;
}

static int lschunks_intrak(GxDemuxer* demuxer, int level, unsigned int id,
		off_t pos, off_t len, mov_track_t* trak)
{
	int ret;
	switch(id)
	{
		case MOV_FOURCC('m','d','a','t'):
			return -1;
		case MOV_FOURCC('f','r','e','e'):
		case MOV_FOURCC('u','d','t','a'):
			/* here not supported :p*/
			break;
		case MOV_FOURCC('t','k','h','d'):
			{
				// read codec data
				trak->tkdata_len = len;
				trak->tkdata = av_malloc(trak->tkdata_len);
				CHECK_POINT(trak->tkdata);
				GxStream_Read(demuxer->stream, trak->tkdata, trak->tkdata_len);
				/*
				   0  1 Version
				   1  3 Flags
				   4  4 Creation time
				   8  4 Modification time
				   12 4 Track ID
				   16 4 Reserved
				   20 4 Duration
				   24 8 Reserved
				   32 2 Layer
				   34 2 Alternate group
				   36 2 Volume
				   38 2 Reserved
				   40 36 Matrix structure
				   76 4 Track width
				   80 4 Track height
				   */
				gxlogf(
						"tkhd len=%d ver=%d flags=0x%X id=%d dur=%d lay=%d vol=%d\n",
						trak->tkdata_len, trak->tkdata[0], trak->tkdata[1],
						char2int(trak->tkdata, 12), // id
						char2int(trak->tkdata, 20), // duration
						char2short(trak->tkdata, 32), // layer
						char2short(trak->tkdata, 36)); // volume
				break;
			}
		case MOV_FOURCC('m','d','h','d'):
			{
				int version = GxStream_ReadChar(demuxer->stream);
				GxStream_Skip(demuxer->stream, (version == 1) ? 19 : 11);
				// read timescale
				trak->timescale = GxStream_ReadDword(demuxer->stream);
				// read length
				if (version == 1)
					trak->length = GxStream_ReadQword(demuxer->stream);
				else
					trak->length = GxStream_ReadDword(demuxer->stream);
				break;
			}
		case MOV_FOURCC('h','d','l','r'):
			{
				av_unused unsigned int tmp = GxStream_ReadDword(demuxer->stream);
				unsigned int type = GxStream_ReadDword_Le(demuxer->stream);
				unsigned int subtype = GxStream_ReadDword_Le(demuxer->stream);
				av_unused unsigned int manufact = GxStream_ReadDword_Le(demuxer->stream);
				av_unused unsigned int comp_flags = GxStream_ReadDword(demuxer->stream);
				av_unused unsigned int comp_mask = GxStream_ReadDword(demuxer->stream);
				int len = GxStream_ReadChar(demuxer->stream);
				char* str = av_malloc(len + 1);
				CHECK_POINT(str);
				GxStream_Read(demuxer->stream, (uint8_t*)str, len);
				str[len] = 0;
				MOV_FREE(str);
				switch(bswap_32(type)) {
					case MOV_FOURCC('m','h','l','r'):
						trak->media_handler = bswap_32(subtype);
						break;
					case MOV_FOURCC('d','h','l','r'):
						trak->data_handler = bswap_32(subtype);
						break;
					default:
						gxlogf( "MOV: unknown handler class: 0x%X (%.4s)\n",
								bswap_32(type), (char* )&type);
				}
				break;
			}
		case MOV_FOURCC('v','m','h','d'):
			{
				trak->type = MOV_TRAK_VIDEO;
				// read video data
				break;
			}
		case MOV_FOURCC('s','m','h','d'):
			{
				trak->type = MOV_TRAK_AUDIO;
				// read audio data
				break;
			}
		case MOV_FOURCC('g','m','h','d'):
			{
				trak->type = MOV_TRAK_GENERIC;
				break;
			}
		case MOV_FOURCC('n','m','h','d'):
			{
				trak->type = MOV_TRAK_GENERIC;
				break;
			}
		case MOV_FOURCC('s','t','s','d'):
			{
				int i = GxStream_ReadDword(demuxer->stream); // temp!
				int count = GxStream_ReadDword(demuxer->stream);

				for (i = 0; i < count; i++) {
					off_t pos = GxStream_Tell(demuxer->stream);
					off_t len = GxStream_ReadDword(demuxer->stream);
					unsigned int fourcc = GxStream_ReadDword_Le(demuxer->stream);
					/* some files created with Broadcast 2000 (e.g. ilacetest.mov)
					   contain raw I420 video but have a yv12 fourcc*/
					if (fourcc == GX_FOURCC('y','v','1','2'))
						fourcc = GX_FOURCC('I','4','2','0');
					if (len < 8)
						break; // error

					//      if(!i)
					{
						trak->fourcc = fourcc;
						// read type specific (audio/video/time/text etc) header
						// NOTE: trak type is not yet known at this point :(((
						trak->stdata_len = len - 8;
						trak->stdata = av_malloc(trak->stdata_len);
						CHECK_POINT(trak->stdata);
						GxStream_Read(demuxer->stream, trak->stdata, trak->stdata_len);
					}
					if (GxStream_Seek(demuxer->stream, pos + len)!=GX_PLAYER_OK)
						break;
				}
				break;
			}
		case MOV_FOURCC('s','t','t','s'):
			{
				av_unused int temp = GxStream_ReadDword(demuxer->stream);
				int len = GxStream_ReadDword(demuxer->stream);
				int i;
				unsigned int pts = 0;
				trak->durmap = av_calloc(len, sizeof(mov_durmap_t));
				CHECK_POINT(trak->durmap);
				trak->durmap_size = trak->durmap ? len : 0;
				for (i = 0; i < trak->durmap_size; i++) {
					trak->durmap[i].num = GxStream_ReadDword(demuxer->stream);
					trak->durmap[i].dur = GxStream_ReadDword(demuxer->stream);
					pts += trak->durmap[i].num*  trak->durmap[i].dur;
				}
				if (trak->length != pts)
					gxlogf( "Warning! pts=%d  length=%d\n", pts, trak->length);
				break;
			}
		case MOV_FOURCC('s','t','s','c'):
			{
				av_unused int temp = GxStream_ReadDword(demuxer->stream);
				int len = GxStream_ReadDword(demuxer->stream);
				int i;
				// read data:
				trak->chunkmap = av_calloc(len, sizeof(mov_chunkmap_t));
				CHECK_POINT(trak->chunkmap);
				trak->chunkmap_size = trak->chunkmap ? len : 0;
				for (i = 0; i < trak->chunkmap_size; i++) {
					trak->chunkmap[i].first = GxStream_ReadDword(demuxer->stream) - 1;
					trak->chunkmap[i].spc = GxStream_ReadDword(demuxer->stream);
					trak->chunkmap[i].sdid = GxStream_ReadDword(demuxer->stream);
				}
				break;
			}
		case MOV_FOURCC('s','t','s','z'):
			{
				av_unused int temp = GxStream_ReadDword(demuxer->stream);
				int ss=GxStream_ReadDword(demuxer->stream);
				int entries = GxStream_ReadDword(demuxer->stream);
				int i;
				trak->samplesize = ss;
				if (!ss) {
					// variable samplesize
					trak->samples = realloc_struct(trak->samples, entries, sizeof(mov_sample_t));
					CHECK_POINT(trak->samples);
					trak->samples_size = entries;
					for (i = 0; i < trak->samples_size; i++)
						trak->samples[i].size = GxStream_ReadDword(demuxer->stream);
				}
				break;
			}
		case MOV_FOURCC('s','t','c','o'):
			{
				av_unused int temp = GxStream_ReadDword(demuxer->stream);
				int len = GxStream_ReadDword(demuxer->stream);
				int i;

				if (len > trak->chunks_size) {
					trak->chunks = realloc_struct(trak->chunks, len, sizeof(mov_chunk_t));
					CHECK_POINT(trak->chunks);
					trak->chunks_size = trak->chunks ? len : 0;
				}
				// read elements:
				for(i = 0; i < trak->chunks_size; i++)
					trak->chunks[i].pos = GxStream_ReadDword(demuxer->stream);
				break;
			}
		case MOV_FOURCC('c','o','6','4'):
			{
				av_unused int temp = GxStream_ReadDword(demuxer->stream);
				int len = GxStream_ReadDword(demuxer->stream);
				int i;
				// extend array if needed:
				if (len > trak->chunks_size) {
					trak->chunks = realloc_struct(trak->chunks, len, sizeof(mov_chunk_t));
					CHECK_POINT(trak->chunks);
					trak->chunks_size = trak->chunks ? len : 0;
				}
				// read elements:
				for (i = 0; i < trak->chunks_size; i++) {
#ifndef	_LARGEFILE_SOURCE
					if (GxStream_ReadDword(demuxer->stream) != 0)
						gxlogf( "Chunk %d has got 64bit address, but you've MPlayer compiled without LARGEFILE support!\n", i);
					trak->chunks[i].pos = GxStream_ReadDword(demuxer->stream);
#else
					trak->chunks[i].pos = GxStream_ReadQword(demuxer->stream);
#endif
				}
				break;
			}
		case MOV_FOURCC('s','t','s','s'):
			{
				av_unused int temp = GxStream_ReadDword(demuxer->stream);
				int entries = GxStream_ReadDword(demuxer->stream);
				int i;
				trak->keyframes = av_calloc(entries, sizeof(unsigned int));
				CHECK_POINT(trak->keyframes);
				trak->keyframes_size = trak->keyframes ? entries : 0;
				for (i = 0; i < trak->keyframes_size; i++)
					trak->keyframes[i] = GxStream_ReadDword(demuxer->stream) - 1;
				break;
			}
		case MOV_FOURCC('m','d','i','a'):
			{
				ret = lschunks(demuxer, level + 1, pos + len, trak);
				CHECK_RET(ret);
				break;
			}
		case MOV_FOURCC('m','i','n','f'):
			{
				ret = lschunks(demuxer, level + 1 ,pos + len, trak);
				CHECK_RET(ret);
				break;
			}
		case MOV_FOURCC('s','t','b','l'):
			{
				ret = lschunks(demuxer, level + 1, pos + len, trak);
				CHECK_RET(ret);
				break;
			}
		case MOV_FOURCC('e','d','t','s'):
			{
				ret = lschunks(demuxer, level + 1, pos + len, trak);
				CHECK_RET(ret);
				break;
			}
		case MOV_FOURCC('e','l','s','t'):
			{
				av_unused int temp = GxStream_ReadDword(demuxer->stream);
				int entries = GxStream_ReadDword(demuxer->stream);
				int i;
#if 1
				trak->editlist = av_calloc(entries, sizeof(mov_editlist_t));
				CHECK_POINT(trak->editlist);
				trak->editlist_size = trak->editlist ? entries : 0;
				for (i = 0; i < trak->editlist_size; i++) {
					int dur = GxStream_ReadDword(demuxer->stream);
					int mt = GxStream_ReadDword(demuxer->stream);
					int mr = GxStream_ReadDword(demuxer->stream); // 16.16fp
					trak->editlist[i].dur = dur;
					trak->editlist[i].pos = mt;
					trak->editlist[i].speed = mr;
				}
#endif
				break;
			}
		case MOV_FOURCC('c','o','d','e'):
			{
				/* XXX: Implement atom 'code' for FLASH support*/
				break;
			}
		default:
			id = be2me_32(id);
			break;
	}//switch(id)
	return 0;
}

static GxDemuxer* demux_mov_open(GxDemuxer* demuxer){
	DemuxMovPriv* priv=demuxer->priv;
	int t_no;
	int best_a_id=-1, best_a_len=0;
	int best_v_id=-1, best_v_len=0;

	gxlogf( "demux_mov_open!\n");

	// Parse header:
	GxStream_Reset(demuxer->stream);
	if(GxStream_Seek(demuxer->stream,priv->moov_start)!=GX_PLAYER_OK)
	{
		gxlogd("MOV: Cannot seek to the beginning of the Movie header (0x%lld)\n",(int64_t)priv->moov_start);
		return 0;
	}

	if(lschunks(demuxer, 0, priv->moov_end, NULL) <0)
		return NULL;
	// just in case we have hit eof while parsing...
	demuxer->stream->eof = 0;

	// find the best (longest) streams:
	for(t_no=0;t_no<priv->track_db;t_no++){
		mov_track_t* trak=priv->tracks[t_no];
		int len=(trak->samplesize) ? trak->chunks_size : trak->samples_size;
		if(demuxer->a_streams[t_no]){ // need audio
			if(len>best_a_len){	best_a_len=len; best_a_id=t_no; }
		}
		if(demuxer->v_streams[t_no]){ // need video
			if(len>best_v_len){	best_v_len=len; best_v_id=t_no; }
		}
	}
	gxlogf( "MOV: longest streams: A: #%d (%d samples)  V: #%d (%d samples)\n",
			best_a_id,best_a_len,best_v_id,best_v_len);
	if(demuxer->audio->id==-1 && best_a_id>=0) demuxer->audio->id=best_a_id;
	if(demuxer->video->id==-1 && best_v_id>=0) demuxer->video->id=best_v_id;

	// setup sh pointers:
	if(demuxer->audio->id>=0){
		GxStreamAudioHeader* sh=demuxer->a_streams[demuxer->audio->id];
		if(sh){
			demuxer->audio->sh=sh;
			sh->ds=demuxer->audio;
		} else {
			gxlogd( "MOV: selected audio stream (%d) does not exist\n",demuxer->audio->id);
			demuxer->audio->id=-2;
		}
	}
	if(demuxer->video->id>=0){
		GxStreamVideoHeader* sh=demuxer->v_streams[demuxer->video->id];
		if(sh){
			demuxer->video->sh=sh; sh->ds=demuxer->video;
		} else {
			gxlogd( "MOV: selected video stream (%d) does not exist\n",demuxer->video->id);
			demuxer->video->id=-2;
		}
	}
	if(demuxer->sub->id>=0){
		GxStreamSubHeader* sh=demuxer->s_streams[demuxer->sub->id];
		if(sh){
			demuxer->sub->sh=sh;
		} else {
			gxlogd( "MOV: selected subtitle stream (%d) does not exist\n",demuxer->sub->id);
			demuxer->sub->id=-2;
		}
	}

	if(demuxer->video->id<0 && demuxer->audio->id<0) {
		gxlogd( "MOV:  No AV streams found\n");
	}

	return demuxer;
}

/**
 *  \brief return the mov track that belongs to a demuxer stream
 *  \param ds the demuxer stream, may be NULL
 *  \return the mov track info structure belonging to the stream,
 *           NULL if not found
 */
static mov_track_t* stream_track(DemuxMovPriv *priv, GxDemuxStream *ds) {
	if (ds && (ds->id >= 0) && (ds->id < priv->track_db))
		return priv->tracks[ds->id];
	return NULL;
}

static int demux_mov_fill_buffer(GxDemuxer* demuxer,GxDemuxStream* ds){
	DemuxMovPriv* priv=demuxer->priv;
	mov_track_t* trak=NULL;
	double pts;
	int x;
	off_t pos;
	int clock = 0;

	if (ds->eof){
		return GX_PLAYER_ERROR;
	}
	trak = stream_track(priv, ds);
	if (!trak)
		return GX_PLAYER_ERROR;

	clock = trak->timescale;
	clock = (clock==0)?1:clock;

	if(trak->samplesize)
	{
		// read chunk:
		if(trak->pos>=trak->chunks_size)
			return GX_PLAYER_ERROR; // EOF
		GxStream_Seek(demuxer->stream,trak->chunks[trak->pos].pos);
		pts=(double)((trak->chunks[trak->pos].sample*trak->duration)*1000/clock);
		if(trak->samplesize!=1)
		{
			gxlogf( "WARNING! Samplesize(%d) != 1\n",trak->samplesize);
			if((trak->fourcc != MOV_FOURCC('t','w','o','s')) && (trak->fourcc != MOV_FOURCC('s','o','w','t')))
				x=trak->chunks[trak->pos].size*trak->samplesize;
			else
				x=trak->chunks[trak->pos].size;
		}
		else
			x=trak->chunks[trak->pos].size;

		/* the following stuff is audio related*/
		if (trak->type == MOV_TRAK_AUDIO)
		{
			if(trak->stdata_len>=44 && trak->stdata[9]>=1 && char2int(trak->stdata,28)>0)
			{
				// stsd version 1 - we have audio compression ratio info:
				x/=char2int(trak->stdata,28); // samples/packet
				//	x*=char2int(trak->stdata,32); // bytes/packet
				x*=char2int(trak->stdata,36); // bytes/frame
			}
			else
			{
				x*=trak->nchannels;
				x*=trak->samplebytes;
			}
		} /* MOV_TRAK_AUDIO*/
		pos=trak->chunks[trak->pos].pos;
	}
	else
	{
		int frame=trak->pos;
		// editlist support:
		if(trak->type == MOV_TRAK_VIDEO && trak->editlist_size>=1)
		{
			// find the right editlist entry:
			if(frame<trak->editlist[trak->editlist_pos].start_frame)
				trak->editlist_pos=0;
			if(frame>=trak->editlist[trak->editlist_pos].start_frame+trak->editlist[trak->editlist_pos].frames)
				return GX_PLAYER_ERROR; // EOF
			// calc real frame index:
			frame-=trak->editlist[trak->editlist_pos].start_frame;
			//frame+=trak->editlist[trak->editlist_pos].start_sample;
			// calc pts:
			mov_get_pos_pts(trak,frame,&pos,&pts);
			pts=(double)((pts+trak->editlist[trak->editlist_pos].pts_offset)*1000/clock) ;
		}
		else
		{
			if(frame>=trak->samples_size)
				return GX_PLAYER_ERROR; // EOF
			mov_get_pos_pts(trak,frame,&pos,&pts);
			pts=(double)(pts*1000/clock);
		}
		// read sample:
		GxStream_Seek(demuxer->stream,pos);
		x=trak->samples[frame].size;
	}

	if(trak->pos==0 && trak->stream_header_len>0)
	{
		// we have to append the stream header...
		GxStreamHeadPriv* hdr;
		hdr = ds->sh;
		if(hdr)
		{
			hdr->header.data = av_malloc(trak->stream_header_len);
			hdr->header.len  = trak->stream_header_len;
			if(hdr->header.data)
			{
				memcpy(hdr->header.data,trak->stream_header,trak->stream_header_len);
			}
		}
		//TODO liufei
		GxDemuxPacket* dp=GxDemuxPacket_Create(demuxer, NULL, x);
		GxStream_Read(demuxer->stream,dp->buffer,x);
		MOV_FREE(trak->stream_header);
		trak->stream_header = NULL;
		trak->stream_header_len = 0;
		dp->pts=pts ;
		dp->flags=0;
		dp->pos=pos; // FIXME?
		GxDemuxStream_AddPacket(ds,dp);
	}
	else
	{
		if(trak->type == MOV_TRAK_AUDIO)
		{
			GxStreamAudioHeader* sh=(GxStreamAudioHeader*)ds->sh;
			int priv_len = 0;
#if ENABLE_AAC_FRAME
			if (sh->format == GX_FOURCC('M', 'P', '4', 'A')||
					sh->format == GX_FOURCC('m', 'p', '4', 'a'))
				priv_len = ADTS_HEADER_SIZE;
#endif
			//TODO liufei
			GxDemuxPacket*dp = GxDemuxPacket_Create(demuxer, NULL, x + priv_len);
			GxDemuxPacket_Skip(dp, priv_len);
#if ENABLE_AAC_FRAME
			if (sh->format == GX_FOURCC('M', 'P', '4', 'A')||
					sh->format == GX_FOURCC('m', 'p', '4', 'a')) {
				sh->codecdata_len = aac_get_sample_rate_index(sh->samplerate);
				dp->buffer[0] = 0xFF;
				dp->buffer[1] = 0xF1;
				dp->buffer[2] = (0x01<<6)|(sh->codecdata_len<<2)|((sh->channels>>2)&1);
				dp->buffer[3] = (sh->channels << 6) | ((x+7) >>11);
				dp->buffer[4] = (((x + 7) & 0x7FF) >> 3) & 0xff;
				dp->buffer[5] = (((x + 7) & 0x07 ) << 5) | 0x1f;
				dp->buffer[6] = 0xFC;
			}
#endif
			GxStream_Read(demuxer->stream,dp->buffer+priv_len,x);
			dp->pos = pos;
			dp->pts = pts;
			dp->flags = 0;
			GxDemuxStream_AddPacket(ds,dp);
		}
		else if(trak->type == MOV_TRAK_VIDEO)
		{
#if ENABLE_AVC_FRAME
			if(priv->nal_size_size)
			{
				int i,numnal=0,ppos=0, nownal=16;
				unsigned int nal_size = 0;
				GxDemuxPacket* dp = GxDemuxPacket_Create(demuxer, NULL, x + nownal*(4-priv->nal_size_size)); // TODO liufei

				while (x > ppos)
				{
					for (i = 0; i < priv->nal_size_size; ++i)
						nal_size = (nal_size << 8) | (GxStream_ReadChar(demuxer->stream)&0xff);
					ppos += priv->nal_size_size;
					if((numnal++) > nownal){
						nownal += 16;
						dp = GxDemuxPacket_Resize(demuxer, dp, x+nownal*(4-priv->nal_size_size));
						if(dp == NULL)
							return GX_PLAYER_ERROR;
					}
					GxDemuxPacket_Write(dp, start_code, 4);
					if(nal_size > x-ppos){
						gxlogd("nal_error !]#####%d: %d \n", nal_size, priv->nal_size_size);
						nal_size = x-ppos;
					}
					GxStream_Read(demuxer->stream, dp->buffer+dp->write_size, nal_size);
					dp->write_size += nal_size;
					ppos += nal_size;
				}
				dp->pts = pts;
				dp->pos = pos;
				dp->flags = 0;
				GxDemuxStream_AddPacket(ds, dp);
			}
			else
				GxDemuxStream_ReadPacket(ds,demuxer->stream,x,pts,pos,0);
#else
			GxDemuxStream_ReadPacket(ds,demuxer->stream,x,pts,pos,0);
#endif
		}
	}
	++trak->pos;

	trak = NULL;
	if (demuxer->sub->id >= 0 && demuxer->sub->id < priv->track_db)
		trak = priv->tracks[demuxer->sub->id];
	if (trak)
	{
		int samplenr = 0;
		double subpts;
		off_t spos;

		while (samplenr < trak->samples_size)
		{
			mov_get_pos_pts(trak,samplenr,&spos,&subpts);
			if (subpts >= pts)
				break;
			samplenr++;
		}
		samplenr--;
		if (samplenr < 0)
		{
			//vo_sub = NULL;
		}
		else if (samplenr != priv->current_sub)
		{
			int len = trak->samples[samplenr].size;
			mov_get_pos_pts(trak,samplenr,&pos,&subpts);
			GxStream_Seek(demuxer->stream, pos);
			GxDemuxStream_ReadPacket(demuxer->sub, demuxer->stream, len, subpts, pos, 0);
			priv->current_sub = samplenr;
		}
	}

	return GX_PLAYER_OK;

}

static float mov_seek_track(mov_track_t* trak,float pts,int flags)
{
	int clock = 0;

	clock = trak->timescale;
	clock = (clock==0)?1:clock;

	if(flags&GX_DEMUXER_SEEK_PERCENT) pts*=trak->length;

	if(trak->samplesize)
	{
		int sample=pts*(clock/1000)/trak->duration;

		if(!(flags&GX_DEMUXER_SEEK_ABSOLUTE))
			sample+=trak->chunks[trak->pos].sample; // relative
		trak->pos=0;
		while(trak->pos<trak->chunks_size && trak->chunks[trak->pos].sample<sample)
			++trak->pos;
		if (trak->pos == trak->chunks_size)
			return -1;
		pts=(uint32_t)(trak->chunks[trak->pos].sample*trak->duration) ;
	}
	else
	{
		unsigned int ipts;
		double spts;
		off_t spos;
		int i, step, tmp_step;
		double tmp_pts1, tmp_pts2;
		int counter = 0;

		if(!(flags&GX_DEMUXER_SEEK_ABSOLUTE))
		{
			mov_get_pos_pts(trak,trak->pos, &spos, &spts);
			pts+=spts;
		}
		if(pts<0)
			pts=0;
		ipts=pts;

		//get step of pts
		mov_get_pos_pts(trak, 1, &spos, &tmp_pts1);
		mov_get_pos_pts(trak, 0, &spos, &tmp_pts2);
		step = (int)(tmp_pts1 -  tmp_pts2);
		for(i = 1; i < 10; i++)
		{
			mov_get_pos_pts(trak, 1, &spos, &tmp_pts1);
			mov_get_pos_pts(trak, 0, &spos, &tmp_pts2);
			tmp_step = (int)(tmp_pts1 -  tmp_pts2);
			if(tmp_step == step)
				counter ++;
		}
		if(counter == 9)
		{
			trak->pos = (off_t)(((float)ipts/1000.0)*clock/(float)step);
			mov_get_pos_pts(trak,trak->pos, &spos, &spts);
			//while(spts*1000.0/clock<ipts)
			//{
			//    mov_get_pos_pts(trak,++trak->pos, &spos, &spts);
			//    //gxlogf("\n*********trak->pos = %d, ipts = %ld, spts*1000/clock = %ld, %ld\n",
			//    //        (int)(trak->pos), (long)ipts, (long)(spts*1000/clock), (long)((spts*1000.0)/(float)clock));
			//}
		}
		else //can not synchronization
		{
			int precision = 2;
			int step = (0x1<<16);

			trak->pos = 0;
			while(labs((long)(spts*1000/clock-ipts)) > precision)
			{
				mov_get_pos_pts(trak,trak->pos, &spos, &spts);

				if(spts*1000/clock >= ipts)
				{
					trak->pos -= step;
					step /= 2;
					if(step == 0)
						break;
				}
				else
				{
					while(trak->pos + step > trak->samples_size)
						step /= 2;
				}

				trak->pos += step;
			}
		}

		if (trak->pos == trak->samples_size)
			return -1;
		if(trak->keyframes_size)
		{
			// find nearest keyframe
			int i;
			for(i=0;i<trak->keyframes_size;i++)
			{
				if(trak->keyframes[i]>=trak->pos)
					break;
			}
			if (i == trak->keyframes_size)
				return -1;
			if(i>0 && (trak->keyframes[i]-trak->pos) > (trak->pos-trak->keyframes[i-1]))
				--i;
			trak->pos=trak->keyframes[i];
		}
		mov_get_pos_pts(trak,trak->pos, &spos, &spts);
		pts=spts;
	}

	return pts;
}

static int demux_mov_seek(GxDemuxer* demuxer,int64_t pts,int32_t audio_delay,int flags)
{
	int ret = GX_PLAYER_OK;
	DemuxMovPriv* priv=demuxer->priv;
	GxDemuxStream* ds;
	mov_track_t* trak;

	ds=demuxer->video;
	trak = stream_track(priv, ds);
	if (trak) {
		demux_mov_fill_buffer(demuxer,ds);
		ds->pts=mov_seek_track(trak,pts,flags);
		if (ds->pts < 0)
			ds->eof = 1;
	}

	ds=demuxer->audio;
	trak = stream_track(priv, ds);
	if (trak) {
		demux_mov_fill_buffer(demuxer,ds);
		ds->pts=mov_seek_track(trak,pts,flags);
		if (ds->pts < 0)
			ds->eof = 1;
	}

	GxDemuxStream_FreePacks(demuxer->video);
	GxDemuxStream_FreePacks(demuxer->audio);
	GxDemuxStream_FreePacks(demuxer->sub);

	return ret;
}

static int demux_mov_control(GxDemuxer* demuxer, int cmd, void *arg)
{
	mov_track_t* track;

	// try the video track
	track = stream_track(demuxer->priv, demuxer->video);

	if (!track || !track->length)
		// otherwise try to get the info from the audio track
		track = stream_track(demuxer->priv, demuxer->audio);

	if (!track || !track->length)
		return GX_DEMUXER_CTRL_ERROR;

	switch(cmd)
	{
		case GX_DEMUXER_CTRL_GET_TIME_LENGTH:
			if (!track->timescale)
				return GX_DEMUXER_CTRL_ERROR;
			* ((int64_t *)arg) = (int64_t)(track->length/track->timescale)*1000;
			return GX_DEMUXER_CTRL_OK;

		case GX_DEMUXER_CTRL_GET_PERCENT_POS:
			{
				off_t pos = track->pos;
				if (track->durmap_size >= 1)
					pos*= track->durmap[0].dur;
				* ((int *)arg) = (int)(100 * pos / track->length);
				return GX_DEMUXER_CTRL_OK;
			}
		default:
			return GX_DEMUXER_CTRL_NOTIMPL;
	}
}

GxDemuxerClass gx_demux_mov = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "MP4 demuxer",
			.parent  = &gx_DemuxerBase,
			.size    = sizeof(GxDemuxer),
			.create  = NULL,
			.release = NULL,
		},
		.run    = NULL,
		.pause  = NULL,
		.resume = NULL,
		.stop   = NULL,
	},
	DEF_AUTHOR("demuxer","mp4","No description","L.F","No comment"),

	.name        = "Demuxer MP4",
	.type        = GX_DEMUXER_TYPE_MOV,
	.check_file  = demux_mov_check_file,
	.open        = demux_mov_open,
	.close       = demux_mov_close,
	.fill_buffer = demux_mov_fill_buffer,
	.seek        = demux_mov_seek,
	.control     = demux_mov_control,
};

