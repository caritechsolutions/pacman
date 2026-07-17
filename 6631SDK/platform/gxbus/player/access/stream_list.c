#include "gx_common.h"
#include "gx_stream.h"
#include "avstring.h"
#include "hls.h"
#include "stream_m3u8.h"

#define CACHE_BUFFER_SIZE  (3*1024*1024)
#define CACHE_SECTION_SIZE (32*1024)
static int64_t add_cache_id = 0;

typedef struct {
#define MAX_LIST_NUM 5
	int change_index;
	char *list_url[MAX_LIST_NUM];
}slt_compare_t;

typedef struct {
	// constats:
	int64_t  cache_id;
	char *url;
	unsigned char* buffer;
	int buffer_size;
	int back_size;
	int fill_limit;
	int eof;
	off_t min_filepos;
	off_t max_filepos;
	off_t read_filepos;
	off_t offset;
	GxStream* stream;
	GxStream* pstream;
	int control_exit;
	int cache_fill_tid;
}slt_cache_t;

struct streamlist_node{
	struct gxlist_head head;
	slt_cache_t* slt_cache;
};

static slt_compare_t compare_list;

#define STREAM_LIST_COST_TIMES(parameter1)          \
{                                                   \
	static int64_t last_time = -1;              \
	int64_t cur_time = GxStream_GetTimeMs();    \
	if(last_time == -1){                        \
		last_time = cur_time;               \
	} else {                                    \
		gxlogd("\033[31m -- %lld:%lld\033[0m\n", (cur_time-last_time)/1000, (cur_time-last_time)%1000); \
		if(parameter1)                      \
			last_time = -1;             \
		else {                              \
			last_time = cur_time;       \
		}                                   \
	}                                           \
}

static int _interrupt(GxStream* stream)
{
	if(stream->interrupt_cbk && stream->interrupt_cbk())
		return 1;
	return 0;
}

static int streamlist_compare_url(char* url)
{
	int i, def_max_url_size = 0;

	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
	for(i = 0; i < MAX_LIST_NUM; i++){
		if(compare_list.list_url[i] && av_stristr(compare_list.list_url[i], url))
			return 1;
	}
	i = compare_list.change_index;
	if (!(compare_list.list_url[i])) {
		if (!(compare_list.list_url[i] = av_mallocz(def_max_url_size))) {
			return 0;
		}
	}
	snprintf (compare_list.list_url[i], def_max_url_size, "%s", url);
	compare_list.change_index++;
	if(compare_list.change_index >= MAX_LIST_NUM)
		compare_list.change_index = 0;
	return 0;
}

static int streamlist_cache_buffer_init(slt_cache_t* s, int size ,int sector)
{
	int num;
	num=size/sector+1;
	if(num < 16){
		num = 16;
	}
	s->buffer_size=num*sector;
	s->buffer=av_malloc(s->buffer_size);

	if(s->buffer == NULL)
		return -1;
	return 0;
}

static slt_cache_t* streamlist_cache_init(void)
{
	slt_cache_t* s;

	s =av_mallocz(sizeof(slt_cache_t));
	if(s==NULL)
		return NULL;

	s->fill_limit = 0;
	s->back_size  = 0;
	return s;
}

static void streamlist_cache_uinit(slt_cache_t* s)
{
	if(s){
		s->buffer_size = 0;
		if(s->buffer) {
			av_free(s->buffer);
			s->buffer = NULL;
		}
		s->fill_limit=0;
		s->back_size=0;
		av_free(s);
	}
}

static int streamlist_is_exit(slt_cache_t* s)
{
	if (s->eof || s->control_exit
		|| (s->stream && _interrupt(s->stream))
		|| (s->pstream && _interrupt(s->pstream))) {
		return 1;
	}
	return 0;
}

static int streamlist_cache_fill(slt_cache_t* s)
{
	int back,back2,newb,space,len,pos;
	off_t read=s->read_filepos;

	if(read<s->min_filepos)
		return 0;

	back=read - s->min_filepos;
	if(back<0)
		back=0;
	if(back>s->back_size)
		back=s->back_size;

	newb=s->max_filepos - read;
	if(newb<0)
		newb=0;

	space=s->buffer_size - (newb+back);
	pos=s->max_filepos - s->offset;
	if(pos>=s->buffer_size)
		pos-=s->buffer_size; // wrap-around

	if(space<=s->fill_limit){
		return -1;
	}

	if(space>s->buffer_size-pos)
		space=s->buffer_size-pos;

	GxStreamClass* sl = GetGxStreamClassFromObject(s->stream);
	if(sl->read)
		len = sl->read(s->stream, &s->buffer[pos], space);
	else
		len = -1;

	if(len < 0){
		s->eof=1;
		return -1;
	} else if (len == 0) {
		off_t cn_l = -1;

		if (sl->control)
			sl->control(s->stream, GX_STREAM_CTRL_GET_RES_SIZE, &cn_l);

		if (cn_l == 0) {
			s->eof = 1;
			return -1;
		}
	}

	back2=s->buffer_size-(len+newb); // max back size
	if(s->min_filepos<(read-back2))
		s->min_filepos=read-back2;

	s->max_filepos+=len;
	if(pos+len>=s->buffer_size){
		// wrap...
		s->offset+=s->buffer_size;
	}

	return len;
}

static int streamlist_cache_read(slt_cache_t* s, unsigned char* buf, int size)
{
	int pos,newb,len;
	while(s->read_filepos>=s->max_filepos || s->read_filepos<s->min_filepos){
		if(streamlist_is_exit(s)) return -1;
		GxCore_ThreadDelay(10);
		return 0;
	}

	newb = s->max_filepos - s->read_filepos;
	pos  = s->read_filepos - s->offset;
	if(pos<0)
		pos+=s->buffer_size;
	else if(pos>=s->buffer_size)
		pos-=s->buffer_size;

	if(newb>s->buffer_size-pos)
		newb=s->buffer_size-pos;
	if(newb>size)
		newb=size;

	if(s->read_filepos<s->min_filepos){
		gxlogd("Ehh. s->read_filepos<s->min_filepos !!! Report bug...\n");
		s->eof = 1;
		return -1;
	}

	memcpy(buf,&s->buffer[pos],newb);
	len=newb;

	s->read_filepos+=len;

	return len;
}

static void streamlist_fill_loop(void* data)
{
	int cache_size, retry = 0, len = 0;
	GxStreamParam param;
	slt_cache_t* s = data;

	param.mode     = GX_STREAM_READ;
	param.prog     = 0;
	param.priv     = NULL;
	param.flags    = 0;
	param.options  = (s->pstream!=NULL)?s->pstream->options:NULL;
	param.protocol = NULL;

	s->stream = GxStream_OpenByParam(s->url, &param);
	if(!s->stream){
		gxlogf("failed:  open [id %lld], url [%s]\n", s->cache_id, s->url);
		goto cache_exit;
	}
	cache_size = (s->stream->end_pos<=0)?CACHE_BUFFER_SIZE:s->stream->end_pos;
	cache_size = (cache_size<CACHE_BUFFER_SIZE)?cache_size:CACHE_BUFFER_SIZE;
	if(streamlist_cache_buffer_init(s, cache_size, CACHE_SECTION_SIZE)){
		GxStream_Close(s->stream);
		goto cache_exit;
	}

	gxlogf("open success [id %lld, fd = %d, con_len = %lld, cache_size = %d]\n",
			s->cache_id, s->stream->fd, s->stream->end_pos, cache_size);
	while (!streamlist_is_exit(s)) {
		if((len = streamlist_cache_fill(s)) <= 0) {
			if (0 == len) {
				if (++retry > 500) {/*read ts data size is 0..retry 500,exit player.set s->eof=1.*/
					s->eof = 1;
					gxlogf ("stream_list.c..read size=0. retry 500.player eof.\n");
				}
			}
			GxCore_ThreadDelay(10);
		}
	}
	gxlogf("close success id %lld, fd = %d\n", s->cache_id, s->stream->fd);
	GxStream_Close(s->stream);
cache_exit:
	s->stream = NULL;
	s->eof = 1;
}

static slt_cache_t* streamlist_enable_cache(char* url, int64_t cache_id, void* data)
{
	int ret = GX_PLAYER_ERROR, def_max_url_size = 0;
	slt_cache_t* s;

	s = streamlist_cache_init();
	if(s == NULL)
		return NULL;

	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
	if (!(s->url = av_mallocz(def_max_url_size))) {
		return NULL;
	}
	snprintf (s->url, def_max_url_size-1, "%s", url);
	s->pstream  = (GxStream*)data;
	s->cache_id = cache_id;
	ret = GxCore_ThreadCreate("streamlist_fill_loop",
			&s->cache_fill_tid, streamlist_fill_loop, s, 16*1024, GXOS_DEFAULT_PRIORITY);
	if(ret != GX_PLAYER_OK){
		streamlist_cache_uinit(s);
		if (s->url) {
			av_free(s->url);
			s->url = NULL;
		}
		return NULL;
	}

	return s;
}

static void streamlist_disable_cache(slt_cache_t* s)
{
	s->control_exit = 1;
	if (s->url) {
		av_free(s->url);
		s->url = NULL;
	}
	GxCore_ThreadJoin(s->cache_fill_tid);
	streamlist_cache_uinit(s);
}

int GxStream_StreamList_Get_Count(GxStreamList* s)
{
	if(!s) return 0;
	return s->count;
}

static void streamlist_update_list(void* data)
{
	int ret = 0, retry = 0;
	GxStreamList* slt = data;
	GxStream* stream = slt->priv;

	do{
		if(_interrupt(stream))
			break;
		ret = GxStream_UpdateM3U8(stream);
		if((ret < 0) || (stream->err.open_err > 30)){
			retry++;
			if(retry > 5){
				gxlogd("\033[33mHTTP M3U8 Restart ....\033[0m\n");
				retry = 0;
				stream->err.open_err = 0;
				stream->err.need_restart = 1;
				break;
			}
		}else{
			retry = 0;
		}
		GxCore_ThreadDelay(100);
	}while(!slt->list_update_exit);

	slt->list_update_exit = 1;
	gxlogf("exit list upadte thread...\n");
}

int GxStream_StreamList_Add(GxStreamList* s, char* url)
{
	struct streamlist_node* node = NULL;
	if(s && url){
		if(streamlist_compare_url(url))
			return GX_PLAYER_OK;
		node = av_mallocz(sizeof(struct streamlist_node));
		if(node){
			GxCore_MutexLock(s->mutex);
			add_cache_id++;
			node->slt_cache = streamlist_enable_cache(url, add_cache_id, s->priv);
			if(!node->slt_cache){
				add_cache_id--;
				av_free(node);
				GxCore_MutexUnlock(s->mutex);
				return GX_PLAYER_ERROR;
			}
			s->count++;
			gxlogf("add list slt_cache %p(count %d)\n", node->slt_cache, s->count);
			gxlist_add_tail((struct gxlist_head *)node, &s->streamlist);
			GxCore_MutexUnlock(s->mutex);
			return GX_PLAYER_OK;
		}
	}
	return GX_PLAYER_ERROR;
}

int GxStream_StreamList_Read(GxStreamList* s, uint8_t* buffer, int len)
{
	struct streamlist_node* node = NULL;
	struct gxlist_head* pos;
	struct gxlist_head* n;

	if(s && s->list_update_exit){
		return -1;
	}
	if(s){
		gxlist_for_each_safe(pos, n, &s->streamlist){
			node = gxlist_entry(pos, struct streamlist_node, head);
			if(node){
				int ret = streamlist_cache_read(node->slt_cache, buffer, len);
				if(ret < 0){
					STREAM_LIST_COST_TIMES(0);
					GxCore_MutexLock(s->mutex);
					gxlist_del(pos);
					s->count--;
					gxlogf("del list slt_cache %p(count %d)\n", node->slt_cache, s->count);
					GxCore_MutexUnlock(s->mutex);
					STREAM_LIST_COST_TIMES(0);
					streamlist_disable_cache(node->slt_cache);
					av_free(node);
					ret = 0;
				}
				return ret;
			}
		}
		GxCore_ThreadDelay(10);
		return 0;
	}

	return -1;
}

GxStreamList* GxStream_StreamList_Create(GxStream* s, char* hls_url)
{
	GxStreamList* slt = NULL;

	if(!s)
		return NULL;

	slt = av_mallocz(sizeof(GxStreamList));
	if(!slt)
		return NULL;

	memset(&compare_list, 0, sizeof(slt_compare_t));
	GxCore_MutexCreate(&slt->mutex);
	GX_INIT_LIST_HEAD(&slt->streamlist);
	slt->priv = s;

	s->hls_list = slt;
	add_cache_id = 0;
	GxStream_StreamList_Add(slt, hls_url);

	if(GxCore_ThreadCreate("list_update", &slt->list_update_id,
				streamlist_update_list, slt, 16*1024, GXOS_DEFAULT_PRIORITY) != GX_PLAYER_OK){
		av_free(slt);
		GxCore_MutexDelete(slt->mutex);
		return NULL;
	}
	return slt;
}

void GxStream_StreamList_Destory(GxStreamList* s)
{
	struct streamlist_node* node = NULL;
	struct gxlist_head* pos;
	struct gxlist_head* n;
	int i = 0;

	if(s){
		s->list_update_exit = 1;
		for(i = 0; i < MAX_LIST_NUM; i++){
			if(compare_list.list_url[i]) {
				av_free(compare_list.list_url[i]);
				compare_list.list_url[i] = NULL;
			}
		}
		GxCore_ThreadJoin(s->list_update_id);
		gxlist_for_each_safe(pos, n, &s->streamlist) {
			node = gxlist_entry(pos, struct streamlist_node, head);
			if(node){
				GxCore_MutexLock(s->mutex);
				gxlist_del(pos);
				s->count--;
				gxlogf("del list slt_cache %p(count %d)\n", node->slt_cache, s->count);
				GxCore_MutexUnlock(s->mutex);
				streamlist_disable_cache(node->slt_cache);
				av_free(node);
			}
		}
		GxCore_MutexDelete(s->mutex);
		av_free(s);
	}
	return;
}
