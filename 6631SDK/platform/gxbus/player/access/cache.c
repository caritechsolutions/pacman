#include "gx_stream.h"
#include "cache.h"
#include "gx_msg.h"
#include "gx_system.h"

#define READ_USLEEP_TIME 10
#define FILL_USLEEP_TIME 100
#define PREFILL_SLEEP_TIME 200
#define CONTROL_SLEEP_TIME 0

typedef struct {
	// constats:
	unsigned char* buffer;      // base pointer of the alllocated buffer memory
	int buffer_size;            // size of the alllocated buffer memory
	int sector_size;            // size of a single sector (2048/2324)
	int back_size;              // we should keep back_size amount of old bytes for backward seek
	int cache_size;              // we should keep back_size amount of old bytes for backward seek
	int fill_limit;             // we should fill buffer only if space>=fill_limit
	int seek_limit;             // keep filling cache if distanse is less that seek limit
	int reset_pos;
	// filler's pointers:
	int eof;
	off_t min_filepos;          // buffer contain only a part of the file, from min-max pos
	off_t max_filepos;
	off_t offset;               // filepos <-> bufferpos  offset value (filepos of the buffer's first byte)
	// reader's pointers:
	off_t read_filepos;
	// commands/locking:
	// int seek_lock;          // 1 if we will seek/reset buffer, 2 if we are ready for cmd
	// int fifo_flag;          // 1 if we should use FIFO to notice cache about buffer reads.
	// callback
	GxStream* stream;
	volatile int control;
	volatile int control_exit;
	volatile int control_int_arg;
	volatile off_t control_double_arg;
	volatile int control_res;
	volatile off_t stream_time_length;
	volatile off_t stream_time_pos;
	int cache_fill_tid;
	int cache_check_tid;
	int nobuffer;//0:default,login  1:not login cache_check func.
	uint64_t loop_time;
}cache_vars_t;

static int cache_interrupt_cbk(cache_vars_t* s)
{
	if (s && s->stream &&
			s->stream->interrupt_cbk &&
			s->stream->interrupt_cbk())
		return -1;
	return 0;
}

static void _cache_stats(cache_vars_t* s)
{
	gxlogf("(%s -- [%d]) ", s->eof?"EOF":"ING", s->stream->err.need_restart);
	gxlogf("0x%08X  [0x%08X]  0x%08X   ",(int)s->min_filepos,(int)s->read_filepos,(int)s->max_filepos);
	gxlogf("%3d %%\n",100*(int)(s->max_filepos-s->read_filepos)/s->buffer_size);
}

static void _cache_updata(cache_vars_t* s)
{
	if (!s || !s->stream)
		return;

	GxStream *stream = s->stream;

	int ppos = 0;
	int len  = s->stream->buf_len;

	while (len > 0) {
		int pos      = s->max_filepos - s->offset;
		int free_len = s->buffer_size - pos;
		int x        = (len > free_len) ? free_len : len;

		memcpy(&(s->buffer[pos]), stream->buffer+ppos, x);

		s->read_filepos += x;
		s->max_filepos  += x;

		if (pos+x >= s->buffer_size)
			s->offset += s->buffer_size;

		len  -= x;
		ppos += x;
	}

	return;
}

static void _cache_flush(cache_vars_t *s, off_t pos)
{
	s->offset= (pos/s->buffer_size)*s->buffer_size;
	s->min_filepos=s->max_filepos=s->read_filepos=pos; // drop cache content :(
	s->reset_pos = 1;
}

static int _cache_read(cache_vars_t* s,unsigned char* buf,int size)
{
	int pos,newb,len;
	_cache_stats(s);
	while (s->read_filepos>=s->max_filepos || s->read_filepos<s->min_filepos) {
		// eof?
		if (s->eof || s->control == -2) return -1;
		if (cache_interrupt_cbk(s)) return -1;
		// waiting for buffer fill...
		GxCore_ThreadDelay(READ_USLEEP_TIME); // 10ms
		continue; // try again...
	}

	newb=s->max_filepos-s->read_filepos; // new bytes in the buffer

	pos=s->read_filepos - s->offset;
	if (pos<0)
		pos+=s->buffer_size;
	else if (pos>=s->buffer_size)
		pos-=s->buffer_size;

	if (newb>s->buffer_size-pos)
		newb=s->buffer_size-pos; // handle wrap...
	if (newb>size)
		newb=size;

	// check:
	if (s->read_filepos<s->min_filepos)
		gxlogd("Ehh. s->read_filepos<s->min_filepos !!! Report bug...\n");

	// len=write(mem,newb)
	//gxlogd("Buffer read: %d bytes\n",newb);
	memcpy(buf,&s->buffer[pos],newb);
	len=newb;

	s->read_filepos+=len;

	return len;
}

static void _cache_msg(GxStream* s, int cmd)
{
	GxMediaFilter* mf = GXMEDIAFILTER(s);
	GxMediaFilterEventPara EventPara = {0};

	if (mf == NULL)
		return;

	if (mf->event.func) {
		EventPara.type = cmd;
		EventPara.arg  = NULL;
		mf->event.func(mf->event.priv, &EventPara);
	}
	return;
}

static int _cache_fill(cache_vars_t* s)
{
	int back,back2,newb,space,len,pos;
	off_t read=s->read_filepos;

	if (read<s->min_filepos) {
		// streaming: drop cache contents only if seeking backward or too much fwd:
		return -1;
	}

	// calc number of back-bytes:
	back=read - s->min_filepos;
	if (back<0)
		back=0; // strange...
	if (back>s->back_size)
		back=s->back_size;

	// calc number of new bytes:
	newb=s->max_filepos - read;
	if (newb<0)
		newb=0; // strange...

	// calc free buffer space:
	space=s->buffer_size - (newb+back);

	// calc bufferpos:
	pos=s->max_filepos - s->offset;
	if (pos>=s->buffer_size)
		pos-=s->buffer_size; // wrap-around

	if (space<=s->fill_limit) {
		//	gxlogd("Buffer is full (%d bytes free, limit: %d)\n",space,s->fill_limit);
		return -1; // no fill...
	}

	//gxlogd("### read=0x%llX  back=%d  newb=%d  space=%d  pos=%d\n",read,back,newb,space,pos);

	// reduce space if needed:
	if (space>s->buffer_size-pos)
		space=s->buffer_size-pos;

	//  if (space>32768) space=32768; // limit one-time block size
	if (space > 16*s->sector_size)
		space = 16*s->sector_size;

	//  if (s->seek_lock) return 0; // FIXME
	// ....
	//gxlogd("Buffer fill: %d bytes of %d\n",space,s->buffer_size);
	//len=stream_fill_buffer(s->stream);
	//memcpy(&s->buffer[pos],s->stream->buffer,len); // avoid this extra copy!
	// ....
	//len=GxStream_Read(s->stream,&s->buffer[pos],space);
	GxStreamClass* sl = GetGxStreamClassFromObject(s->stream);
	len = sl->read ? sl->read(s->stream, &s->buffer[pos], space) : -1;

	if (len < 0) {
		s->eof=1;
		return -1;
	}

	back2=s->buffer_size-(len+newb); // max back size
	if (s->min_filepos<(read-back2))
		s->min_filepos=read-back2;

	s->max_filepos+=len;
	if (pos+len>=s->buffer_size) {
		// wrap...
		s->offset+=s->buffer_size;
	}

	return len;
}

static void _cache_check(cache_vars_t* s) {
	int newb = s->max_filepos -s->read_filepos;

	if (s->stream->err.need_restart) {
		_cache_msg(s->stream, GX_MFT_EVENT_END_FILL_BUF);
		return;
	}

	if (!s->eof && newb <= 0) {
		_cache_msg(s->stream, GX_MFT_EVENT_START_FILL_BUF);
	}else if (s->eof || newb >= s->cache_size) {
		_cache_msg(s->stream, GX_MFT_EVENT_END_FILL_BUF);
	}
}

static int _cache_is_exit(cache_vars_t* s)
{
	int quit = s->control == -2;

	if (quit || s->control_exit || cache_interrupt_cbk(s)) {
		s->stream_time_length = 0;
		s->stream_time_pos = -1;
		s->control_res = -1;
		s->control = -2;
		s->eof = 1;
		return 0;
	}
	return 1;
}

static void _cache_net_speed(cache_vars_t* s, int reset)
{
	int64_t ms = 0;
	static int64_t rec_ms = 0;
	static int64_t last_filepos=0;
	int64_t max_filepos = s->max_filepos;
	int64_t read_filepos = s->read_filepos;

	if (reset) {
		rec_ms = last_filepos = 0;
		return;
	}
	if ((rec_ms==0) || s->reset_pos) {
		last_filepos = max_filepos;
		s->reset_pos = 0;
		rec_ms = GxStream_GetTimeMs();
		return;
	}
	ms = GxStream_GetTimeMs();
	if ((ms - rec_ms) >= 2000) {
		GxStreamInfo *info = &s->stream->info;
		int net_speed          = (max_filepos-last_filepos)*1000/(ms-rec_ms)/1024;
		int buf_percent        = 100*(max_filepos-read_filepos)/s->cache_size;
		int cache_buf_size     = (s->buffer_size-s->back_size);
		int cache_buf_percent  = 100*(max_filepos-read_filepos)/(cache_buf_size-s->fill_limit);
		int cache_back_size    = s->back_size;
		int cache_back_percent = 100*(read_filepos-s->min_filepos)/cache_back_size;
		int cache_data_len     = (max_filepos-read_filepos);

		info->net_speed          = net_speed;
		info->buf_percent        = (buf_percent>100)?100:((buf_percent<0)?0:buf_percent);
		info->cache_buf_size     = cache_buf_size;
		info->cache_buf_percent  = (cache_buf_percent>100)?100:((cache_buf_percent<0)?0:cache_buf_percent);
		info->cache_data_len     = (cache_data_len > 0) ? cache_data_len : 0;
		info->cache_back_size    = cache_back_size;
		info->cache_back_percent = (cache_back_percent>100)?100:((cache_back_percent<0)?0:cache_back_percent);
		last_filepos = max_filepos;
		rec_ms = ms;
		gxlogf("net speed:%d(KB/s) per:%d%%\ncache size:%d per:%d%%, back size:%d per:%d%%\n",
				info->net_speed, info->buf_percent,
				info->cache_buf_size, info->cache_buf_percent,
				info->cache_back_size, info->cache_back_percent);
	}
}

static int _cache_execute_control(cache_vars_t *s, int rec_len)
{
	off_t double_res = 0;
	int   int_res = 0;
	GxStreamClass* sl = GetGxStreamClassFromObject(s->stream);

	if (!_cache_is_exit(s))
		return 0;

	double_res = GxStream_GetTimeMs();
	if ((double_res - s->loop_time) > 60*1000) {
		if (sl->control)
			sl->control(s->stream, GX_STREAM_CTRL_SEND_ALIVE, NULL);
		s->loop_time = double_res;
	}

	if (s->control == -1) return 1;
	switch (s->control) {
	case GX_STREAM_CTRL_SEEK_TO_POS:
		double_res = s->control_double_arg;
		if (sl->seek && (s->control_res = sl->seek(s->stream, double_res)) == GX_PLAYER_OK) {
			s->control_double_arg = double_res;
			_cache_flush(s, double_res);
			s->eof = 0;
		}
		break;
	case GX_STREAM_CTRL_SEEK_TO_TIME:
		{
			double_res = s->control_double_arg;
			if (sl->control &&
					(sl->control(s->stream, GX_STREAM_CTRL_SEEK_TO_TIME, &double_res) == GX_PLAYER_OK)) {
				_cache_flush(s, 0);
				s->eof = 0;
			}
			s->control_res = GX_PLAYER_OK;
		}
		break;
	case GX_STREAM_CTRL_CACHE_RESET:
		{
			int_res = s->control_int_arg;
			_cache_flush(s, 0);
			if (s->control_int_arg >= 0)
				s->control_res = sl->control?sl->control(s->stream, GX_STREAM_CTRL_SEEK_TO_SEQ, &int_res):GX_PLAYER_ERROR;
			else
				s->control_res = GX_PLAYER_OK;
			s->eof = 0;
			break;
		}
	case GX_STREAM_CTRL_SEAMLESS_BANDWIDTH_SWITCH:
		{
			double_res = s->control_double_arg;
			if (sl->control){
				sl->control(s->stream, GX_STREAM_CTRL_SEAMLESS_BANDWIDTH_SWITCH, &double_res);
			}
			s->control_res = GX_PLAYER_OK;
		}
		break;
	default:
		s->control_res = GX_PLAYER_ERROR;
		break;
	}
	s->control = -1;
	return 1;
}

cache_vars_t* _cache_init(int size,int sector)
{
	int num;
	cache_vars_t* s=av_malloc(sizeof(cache_vars_t));
	if (s==NULL)
		return NULL;

	memset(s,0,sizeof(cache_vars_t));
	num=size/sector;
	if (num < 16) {
		num = 16;
	}//32kb min_size
	s->buffer_size=num*sector;
	s->sector_size=sector;
	s->buffer=av_malloc(s->buffer_size);

	if (s->buffer == NULL) {
		av_free(s);
		return NULL;
	}

	s->fill_limit=sector;
	s->back_size=s->buffer_size/4;
	s->cache_size=(s->buffer_size-s->back_size)/48;
	s->loop_time = GxStream_GetTimeMs();
	return s;
}

void _cache_uinit(cache_vars_t* s)
{
	if (s) {
		s->buffer_size = 0;
		s->sector_size = 0;
		if (s->buffer)
			av_free(s->buffer);
		s->buffer = NULL;
		s->fill_limit=0;
		s->back_size=0;
		s->cache_size=0;
		av_free(s);
	}
}

static void _cache_fill_loop(void* data)
{
	int ret = 0;

	cache_vars_t*s = data;
	do {
		ret = 0;
		if (s->eof || (ret = _cache_fill(s)) < 0) {
			GxCore_ThreadDelay(FILL_USLEEP_TIME/10); // idle
		}
	} while (_cache_execute_control(s, ret));
	_cache_stats(s);

	gxlogf("exit cache fill thread...\n");
}

static void _cache_check_loop(void* data)
{
	cache_vars_t*s = data;
	do {
		if (!s->nobuffer)
			_cache_check(s);
		_cache_net_speed(s,0);
		GxCore_ThreadDelay(FILL_USLEEP_TIME/10); // idle
	} while (_cache_is_exit(s));
	_cache_net_speed(s,1);

	gxlogf("exit cache check thread...\n");
}

int GxStream_CacheEnable(GxStream* stream, int size, int seek_limit)
{
	cache_vars_t* s;
	int retry = 0, prefill;
	int ss = stream->sector_size ? stream->sector_size : 32*1024;

	if (stream->file_format != GX_STREAMTYPE_STREAM) {
		return GX_PLAYER_ERROR;
	}

	if (stream->cache_data)
		return GX_PLAYER_OK;

	s = _cache_init(size, ss);
	if (s == NULL) return GX_PLAYER_ERROR;

	prefill = stream->nobuffer?0:size/32;
	stream->cache_data = s;
	s->stream = stream;
	s->seek_limit = size;
	s->control = -1;
	s->nobuffer = stream->nobuffer;

	_cache_updata(s);

	//make sure that we won't wait from _cache_fill
	//more data than it is alowed to fill
	if (s->seek_limit > s->buffer_size-(s->back_size+s->fill_limit)) {
		s->seek_limit = s->buffer_size-(s->back_size+s->fill_limit);
	}

	if (prefill > s->buffer_size - s->fill_limit) {
		prefill = s->buffer_size - s->fill_limit;
	}
	if (GxCore_ThreadCreate("cache_fill_loop", &s->cache_fill_tid, _cache_fill_loop, s, 64*1024, GXOS_DEFAULT_PRIORITY-1) != GX_PLAYER_OK) {
		_cache_uinit(s);
		return GX_PLAYER_ERROR;
	}

	if (GxCore_ThreadCreate("cache_check_loop", &s->cache_check_tid, _cache_check_loop, s, 8*1024, GXOS_DEFAULT_PRIORITY) != GX_PLAYER_OK) {
		_cache_uinit(s);
		return GX_PLAYER_ERROR;
	}

	stream->cache_pid = 1;
	while (s->read_filepos < s->min_filepos || s->max_filepos - s->read_filepos < prefill) {
		if (cache_interrupt_cbk(s))
			return GX_PLAYER_ERROR;
		if (s->eof || retry > 10)
			break; // file is smaller than prefill size
		retry++;
		GxCore_ThreadDelay(FILL_USLEEP_TIME);
	}
	return GX_PLAYER_OK;
}

void GxStream_CacheDisable(GxStream* stream)
{
	cache_vars_t* s = stream->cache_data;
	if (!stream->cache_pid)
		return;

	GxStream_CacheControl(stream, -2, NULL);
	GxCore_ThreadJoin(s->cache_fill_tid);
	GxCore_ThreadJoin(s->cache_check_tid);
	stream->cache_pid = 0;
	_cache_uinit(s);
}

int GxStream_CacheFillBuffer(GxStream* stream)
{
	int len, rlen, stream_buffer_size;
	uint8_t* rbuffer;
	cache_vars_t* s = stream->cache_data;

	GxPlayer_SystemGet(PSYS_STREAM_BUFFER_SIZE, &stream_buffer_size);
	if (!stream->cache_pid)
		return GxStream_FillBuffer(stream);

	if (s->control == -2) {
		stream->buf_pos=stream->buf_len=0;
		stream->eof = 1;
		return 0;
	}

	if (stream->pos != s->read_filepos) {
		gxlogf("stream->pos %lld read_filepos %lld\n", stream->pos, s->read_filepos);
		gxlogf("!!! read_filepos differs!!! report this bug...\n");
		return 0;
	}

	stream->buf_pos %= stream_buffer_size;
	stream->buf_len %= stream_buffer_size;
	rlen = stream_buffer_size - stream->buf_len;
	rbuffer = stream->buffer + stream->buf_len;
	len = _cache_read(stream->cache_data, rbuffer, rlen);

	if (len < 0) {
		stream->eof = 1;
		stream->buf_pos = stream->buf_len = 0;
		return 0;
	}

	stream->buf_len+=len;
	stream->pos+=len;

	return len;
}

int GxStream_CacheSeekLong(GxStream* stream, off_t pos, off_t time)
{
	int stream_buffer_size = 0;
	cache_vars_t* s;
	off_t newpos;
	if (!stream->cache_pid)
		return GxStream_SeekLong(stream,pos);

	s = stream->cache_data;
	if (pos < 0 || cache_interrupt_cbk(s)) {
		gxlogf("warning --- pos [%lld] error\n", pos);
		return GX_PLAYER_ERROR;
	}

	GxPlayer_SystemGet(PSYS_STREAM_BUFFER_SIZE, &stream_buffer_size);
	newpos = pos/stream_buffer_size*stream_buffer_size;
	pos -= newpos;

	if (time == -1) {
		if ((newpos < s->min_filepos)||(newpos > (s->max_filepos+s->seek_limit))) {
			gxlogf("new 0x%llx, min 0x%llx, max 0x%llx, seek 0x%x\n",
					newpos, s->min_filepos, s->max_filepos, s->seek_limit);
			//need to seek long pos
			int ret = 0;
			if (time == -1) {
				ret = GxStream_CacheControl(stream, GX_STREAM_CTRL_SEEK_TO_POS, &newpos);
				if (ret != GX_PLAYER_OK) {
					gxlogf("seek error: \033[032m [%d]-{%08llX, %08llX, %08llX, %08llX}\033[0m\n",
							stream->eof, stream->end_pos, stream->pos, newpos, pos);
					return GX_PLAYER_ERROR;
				}
			}
		}
	}else if (time >= 0) {
		int ret = 0;
		ret = GxStream_CacheControl(stream, GX_STREAM_CTRL_SEEK_TO_TIME, &time);
		if (ret != GX_PLAYER_OK) {
			gxlogf("seek time(%lld) pos(%lld) error\n", time, newpos);
			return GX_PLAYER_ERROR;
		}
	}else
		return GX_PLAYER_ERROR;

	stream->buf_pos = stream->buf_len = 0;
	stream->pos=s->read_filepos=newpos;
	s->reset_pos = 1;
	while (GxStream_CacheFillBuffer(stream) > 0 && pos >= 0) {
		if (pos <= stream->buf_len) {
			stream->buf_pos = pos;
			return GX_PLAYER_OK;
		}
	}
	return GX_PLAYER_ERROR;
}

int GxStream_CacheControl(GxStream* stream, int cmd, void *arg)
{
	cache_vars_t* s = stream->cache_data;
	GxStreamClass* sl = GetGxStreamClassFromObject(stream);

	if (!s || !sl)
		return GX_PLAYER_ERROR;

	switch(cmd) {
	case GX_STREAM_CTRL_CACHE_RESET:
		stream->eof = stream->buf_pos = stream->buf_len = stream->pos = 0;
		memset(&stream->err, 0, sizeof(GxStreamError));
		memset(&stream->info, 0, sizeof(GxStreamInfo));
		if (arg)
			s->control_int_arg = *(int*)arg;
		else
			s->control_int_arg = -1;
		s->control = cmd;
		break;
	case GX_STREAM_CTRL_SEEK_TO_TIME:
		if (arg) {
			s->control_double_arg = *(off_t*)arg;
		}
		s->control = cmd;
		break;
	case GX_STREAM_CTRL_SEEK_TO_POS:
		if (arg) s->control_double_arg = *(off_t*)arg;
		s->control = cmd;
		break;
	case GX_STREAM_CTRL_SEAMLESS_BANDWIDTH_SWITCH:
		if (arg) s->control_double_arg = *(off_t*)arg;
		s->control = cmd;
		return GX_PLAYER_OK;//not add break:deleay 30s
	case GX_STREAM_CTRL_RESET:
	case GX_STREAM_CTRL_GET_SIZE:
	case GX_STREAM_CTRL_GET_TOTAL_TIME:
	case GX_STREAM_CTRL_GET_SEQ_BYTIME:
	case GX_STREAM_CTRL_GET_TIME_BYSEQ:
	case GX_STREAM_CTRL_GET_CUR_SEQ:
	case GX_STREAM_CTRL_IS_FINISHED:
	case GX_STREAM_CTRL_IS_NEW_DATA:
	case GX_STREAM_CTRL_TIME_OVER:
	case GX_STREAM_CTRL_STREAM_BUFFERING:
		{
			int ret = GX_PLAYER_ERROR;
			if (cache_interrupt_cbk(s)) {
				return GX_PLAYER_ERROR;
			}
			ret = sl->control?sl->control(stream, cmd, arg):GX_PLAYER_ERROR;
			return ret;
		}
	case GX_STREAM_CTRL_FORCE_STOP_STREAM:
	case -2:
		s->eof = 1;
		s->control = -2;
		s->control_exit = 1;
		break;
	default:
		return GX_PLAYER_ERROR;
	}

	int retry = 300;
	s->control_res = GX_PLAYER_OK;
	while (s->control != -1 && s->control != -2) {
		if (cache_interrupt_cbk(s)) {
			return GX_PLAYER_ERROR;
		}
		GxCore_ThreadDelay(FILL_USLEEP_TIME);
		if (retry <= 0) {
			gxlogf("error: %s %d\n", __FUNCTION__, __LINE__);
			return GX_PLAYER_ERROR;
		}
		retry--;
	}
	return s->control_res;
}

