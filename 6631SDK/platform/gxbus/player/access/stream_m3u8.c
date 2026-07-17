#include "hls.h"
#include "http.h"
#include "stream_m3u8.h"
#include "gx_demux.h"
#include "gx_options.h"

#define HLS_THREAD_COUNT         3
#define STREAM_M3U8_PVR_DEBUG 0

#if STREAM_M3U8_PVR_DEBUG
static FILE* stream_m3u8_pvr_fd = NULL;
static char* stream_m3u8_pvr_file = "/media/sda1/net_pvr/1-m3u8.mp4";
#endif

typedef struct {
	GxStream parent;
	int      prev_download_time; //prev seg download time
	int      prev_update_time;   //prev var update time
	int      prog_target;
	int      stream_buffering;   //buffering data
} GxStreamM3U8;

static void stream_m3u8_close(GxStream*  stream);
static int _interrupt(GxStream* stream)
{
	if(stream->interrupt_cbk && stream->interrupt_cbk())
		return 1;
	return 0;
}

static HLSContext* _new_hls_playlist(GxStream* stream, HLSContext* hls, int id)
{
	char* hls_buf = NULL;
	int ret = 0;
	off_t cn_l = 0;
	off_t rec_z = 0;
	GxStreamClass* sl = GetGxStreamClassFromObject(stream);
	struct variant* var = NULL;

	if(!sl)
		return hls;
	sl->control(stream, GX_STREAM_CTRL_GET_SIZE, &cn_l);
	if(!cn_l){
		gxlogf("error: %s %d\n", __FUNCTION__, __LINE__);
		return hls;
	}

	if(stream->streaming_ctrl &&
			(stream->streaming_ctrl->chk_size >= 0) &&
			(stream->streaming_ctrl->con_len < 0))
		cn_l = -1;

	if(cn_l == -1){
		stream->flags |= GX_STREAM_NO_SEEK;
		cn_l = 256*1024;
	}

	hls_buf = av_malloc(cn_l+1);
	if(!hls_buf){
		gxlogf("error: %s %d\n", __FUNCTION__, __LINE__);
		return hls;
	}

	while(rec_z < cn_l){
		if(_interrupt(stream)){
			av_free(hls_buf);
			return hls;
		}
		ret = sl->read(stream, (uint8_t*)hls_buf+rec_z, cn_l-rec_z);
		if(ret > 0)
			rec_z += ret;
		else if(ret < 0)
			break;
	}
	hls_buf[rec_z] = '\0';

	if (rec_z < GX_HLS_PROBE_LEN) {
		av_free(hls_buf);
		http_closesocket(stream);
		return hls;
	}

	if (hls) {
		var = hls->variants[id];
	} else {
		hls = hls_create();
	}

	if (stream->demuxer_type == GX_DEMUXER_TYPE_PLAYLIST) {
		ret = stream_parse_playlist(hls, var, stream->url, hls_buf);
		if (ret < 0) {
			if (stream->redirect_url)
				ret = hls_parse_playlist(hls, var, stream->redirect_url, hls_buf);
			else
				ret = hls_parse_playlist(hls, var, stream->url, hls_buf);
		}
	} else {
		if (stream->redirect_url)
			ret = hls_parse_playlist(hls, var, stream->redirect_url, hls_buf);
		else
			ret = hls_parse_playlist(hls, var, stream->url, hls_buf);
	}

	if (ret < 0) {
		hls_destory(hls);
		hls = NULL;
	}
	if (hls_buf) {
		av_free(hls_buf);
		hls_buf = NULL;
	}
	http_closesocket(stream);

	return hls;
}

static int _back_cur_seq(HLSContext* hls)
{
	if(!hls) return -1;

	hls->cur_seq_no--;
	return 0;
}

static int _set_start_seq(HLSContext* hls, int id, int seq_no)
{
#define JUMP_FRONT_URL_NO 4
	struct variant* var = (hls!=NULL)?hls->variants[id]:NULL;

	if(!hls || !var) return -1;

	gxlogf("finished %d, no %d, start no %d, segments %d\n",
			var->finished, seq_no,  var->start_seq_no, var->n_segments);
	if(seq_no != -1){
		if ((var->finished) || ((seq_no + 1) >= var->start_seq_no) || (var->n_segments < JUMP_FRONT_URL_NO)) {
			if ((var->start_seq_no + var->n_segments) < seq_no) {
				hls->cur_seq_no = var->start_seq_no+1;
			} else {
				hls->cur_seq_no = seq_no;
			}
		} else {
			hls->cur_seq_no = var->start_seq_no + var->n_segments - JUMP_FRONT_URL_NO;
		}
		return 0;
	}

	if(!var->finished &&
			var->n_segments >= JUMP_FRONT_URL_NO &&
			hls->cur_seq_no < var->start_seq_no)
		hls->cur_seq_no = var->start_seq_no + var->n_segments - JUMP_FRONT_URL_NO;
	return 0;
}

static int _get_hls_seekable(HLSContext* hls, int id)
{
	//EXT-X-ENDLIST标志，可支持seek操作, duration > 0
	struct variant* var = NULL;

	if(!hls) return 0;

	var = hls->variants[id];
	if(var->finished && var->totle_duration > 0)
		return 1;

	return 0;
}

static int _get_hlslist_count(HLSContext* hls, int id)
{
	struct variant* var = NULL;

	if(!hls) return -1;

	var = hls->variants[id];
	if(!var) return -1;

	if(var->n_segments == 0)
		return 0;

	return var->start_seq_no + var->n_segments - hls->cur_seq_no-1;
}

static int _get_hls_thread_flags(HLSContext* hls, int id)
{
	struct variant* var = NULL;

	if(!hls) return 0;
	var = hls->variants[id];
	if (!var) {
		return 0;
	}
	if(!var->finished && var->n_segments > 0 && var->totle_duration >= 0)
		return 1;
	else if(!var->finished && var->n_segments > 0 && var->totle_duration < 0)
		gxlogd("not new stream list, beacuse totle_duration(%d) < 0\n", var->totle_duration);
	return 0;
}

static int _hls_is_thread_data(HLSContext* hls, int id)
{
	struct variant* var = NULL;

	if(!hls) return -1;
	var = hls->variants[id];
	if (!var) {
		return -1;
	}
	if(var->n_segments == 0)
		return 0;
	return 1;
}

static int _get_reload_interval(HLSContext* hls, int id)
{
	struct variant* var = NULL;

	if(!hls)
		return -1;
	var = hls->variants[id];
	if (!var) {
		return -1;
	}

	if(var->finished || (var->n_segments == 0))
		return 0;
	return hls_get_interval_byseq(hls, var, hls->cur_seq_no, 0);
}

static int _get_cur_interval(HLSContext* hls, int id)
{
	struct variant* var = NULL;

	if(!hls)
		return 0;

	var = hls->variants[id];
	if (var->n_segments == 0)
		return 0;

	return hls_get_interval_byseq(hls, var, hls->cur_seq_no, 1);
}

static int _get_playurl(GxStream* stream, HLSContext* hls, char* hls_url, int id)
{
	int ret = 0;
	struct variant* var = NULL;

	if(!hls)
		return -1;
	var = hls->variants[id];
	if (!var) {
		return -1;
	}
	if(var->n_segments == 0)
		ret = hls_get_varurl(hls, var, hls_url);
	else
		ret = hls_get_playurl(stream, hls, var, hls_url, -1);
	if(ret != 0){
		if(hls_is_finished(hls, var) == 1){
			return -2; //url over
		}
		return -1;     //not url
	}
	return 0;
}

static int _hls_is_over(HLSContext* hls, int id)
{
	struct variant* var = hls->variants[id];

	if (!hls || !var)
		return 1;

	if (var->n_segments == 0)
		return 1;

	if ((hls->cur_seq_no < var->start_seq_no) ||
			(hls->cur_seq_no >= var->start_seq_no + var->n_segments - 1))
		return 1;

	if (hls_is_finished(hls, var) != 0)
		return 1;

	return 0;
}

static GxStream* _find_prev_data_stream(GxStream* stream, GxStream* find_stream)
{
	GxStream **p, **p_prev;

	p = p_prev = &stream;
	while((*p) && (*p)->priv){
		if(find_stream && *p==find_stream)
			break;
		p_prev = p;
		p = &((*p)->next);
	}
	return *p_prev;
}

static int _read_data(GxStream* stream, uint8_t* buffer, size_t max_len, int* is_over)
{
	off_t cn_l = 0;
	int rec_z = 0;
	GxStreamClass* sl = GetGxStreamClassFromObject(stream);

	if(!sl){
		*is_over = 1;
		return 0;
	}
	sl->control(stream, GX_STREAM_CTRL_GET_RES_SIZE, &cn_l);
	if(!cn_l){
		*is_over = 1;
		return 0;
	}
	if (cn_l > 0)
		max_len = (size_t)cn_l>max_len?max_len:(size_t)cn_l;
	rec_z = sl->read?sl->read(stream, buffer, max_len):-1;
	if (GX_STREAMTYPE_FILE == stream->file_format && rec_z < 0) {
		*is_over = 1;
		return 0;
	}
	if(stream->err.need_restart){
		*is_over = 1;
	}
	return rec_z;
}

static int _stream_probe(GxStream* stream)
{
	char buffer[GX_HLS_PROBE_LEN+1] = {0};
	int len = 0, ret = 0;
	int retry = 100;
	GxStreamClass* sl = GetGxStreamClassFromObject(stream);

	if(!sl || !sl->read)
		return -1;

	while(retry && (len < GX_HLS_PROBE_LEN)){
		ret =sl->read(stream, (uint8_t*)buffer+len, GX_HLS_PROBE_LEN-len);
		if(ret < 0){
			len = -1;
			break;
		}else{
			len += ret;
			retry--;
		}
	}
	if(len <= 0)
		return -1;
	if(len < GX_HLS_PROBE_LEN || stream->err.need_restart)
		return -2; //recv error
	ret = hls_probe(buffer);

	//不是M3U8格式的，如果对demuxer_type未知，检测是否时ts格式
	if(ret < 0 && stream->demuxer_type == GX_DEMUXER_TYPE_UNKNOWN){
		if(buffer[0] == 0x47 && !(buffer[1]&0x80) && (buffer[3]&0x30))
			stream->demuxer_type = GX_DEMUXER_TYPE_MPEG_TS;
	}

	//探测的数据复位，方便后面数据解析
	if(stream->file_format != GX_STREAMTYPE_FILE){
		if (sl && sl->control){
			GxStream_RollBack rol;
			rol.buffer = buffer;
			rol.size = len;
			sl->control(stream, GX_STREAM_CTRL_ROLL_BACK, &rol);
		}
	}else{
		if (sl && sl->seek)
			sl->seek(stream, 0);
	}

	return (ret==0)?1:0;
}

static int _stream_control(GxStream* stream, int cmd, void*arg)
{
	HLSContext* hls = stream->priv;
	struct variant* var = NULL;

	if(!hls) return GX_PLAYER_ERROR;
	var = hls->variants[stream->prog_now];
	if(!var) return GX_PLAYER_ERROR;

	if(_interrupt(stream))
		return GX_PLAYER_ERROR;

	switch(cmd)
	{
	case GX_STREAM_CTRL_IS_FINISHED:
		if(hls_is_finished(hls, var) == 1)
			*(int*)arg = 1;
		else
			*(int*)arg = 0;
		break;
	case GX_STREAM_CTRL_IS_NEW_DATA:
		if(hls_is_new_data(hls, var) == 1)
			*(int*)arg = 1;
		else
			*(int*)arg = 0;
		break;
	case GX_STREAM_CTRL_GET_TOTAL_TIME:
		if(var->finished)
			*(int*)arg = var->totle_duration/1000;
		else
			*(int*)arg = 0;
		break;
	case GX_STREAM_CTRL_GET_TIME_BYSEQ:
		{
			GxStreamSeek* seq_info = arg;
			seq_info->seek_ms = hls_get_time_byseq(hls, var, seq_info->seek_seq);
			break;
		}
	case GX_STREAM_CTRL_GET_SEQ_BYTIME:
		{
			GxStreamSeek* seek_info = arg;
			seek_info->seek_seq = hls_get_seq_bytime(hls, var, seek_info->seek_ms);
			seek_info->seek_ms = hls_get_time_byseq(hls, var, seek_info->seek_seq);
			break;
		}
	case GX_STREAM_CTRL_GET_CUR_SEQ:
		{
			*(int*)arg = hls->cur_seq_no;
			break;
		}
	default:
		return GX_PLAYER_ERROR;
	}
	return GX_PLAYER_OK;
}

static int _stream_bandwidth_switch(GxStream* stream)
{
	GxStreamM3U8 *s = (GxStreamM3U8 *)stream;

	if (stream->prog_max > 1) {
		if (stream->prog_now != s->prog_target) {
			GxStream *p = stream->next;
			while (p) {
				GxStream *next = p->next;

				if (p->priv) {
					hls_destory(p->priv);
					p->priv = NULL;
				}
				GxStream_Close(p);
				p = next;
			}
			stream->next = NULL;
			stream->prog_now = s->prog_target;
		}
	}

	return 0;
}

static int _stream_open(GxStream* stream, GxStream* prev, int delay)
{
	int ret  = 0, def_max_url_size = 0;
	char *hls_url = NULL;
	int is_main_url = 0;
	int reload_interval = 0;
	int64_t dis_time = 0;
	GxStreamM3U8 *s = (GxStreamM3U8 *)stream;

	if ((!s->stream_buffering) && (stream->prog_max > 1)) {
		reload_interval = _get_cur_interval(prev->priv, prev->prog_now);
		dis_time = GxStream_GetTimeMs() - s->prev_download_time;
		if(delay && (dis_time >= 0) && (dis_time < reload_interval)) {
			GxCore_ThreadDelay(10);
			return 0;
		}
	}

	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
	if (!(hls_url = av_malloc(def_max_url_size)))
		return -1;
	ret = _get_playurl(stream, prev->priv, hls_url, prev->prog_now);
	if(ret == -2){
		if (hls_url)
			av_free(hls_url);
		return -1;//stream is over
	}else if(ret == -1){
		//upate hlis list, we use interval time to update once
		reload_interval = _get_reload_interval(prev->priv, prev->prog_now);
		dis_time = GxStream_GetTimeMs() - s->prev_update_time;
		if(delay && (dis_time >= 0) && (dis_time < reload_interval)) {
			GxCore_ThreadDelay(10);
			if (hls_url)
				av_free(hls_url);
			return 0;
		}
		char *new_url = NULL;
		new_url = GxOptions_Get_By_Name(stream->options, " -H", "location:");
		url_str_copy(new_url?new_url:prev->url, hls_url, def_max_url_size-1);
		is_main_url = 1;
	}

	if (prev->next) {
		GxStream *p = prev->next;
		prev->next = NULL;
		GxStream_Close(p);
	}

	GxStreamParam param;
	param.mode     = stream->mode;
	param.prog     = 0;
	param.flags    = stream->flags;
	param.options  = stream->options;
	param.protocol = NULL;
	param.priv     = NULL;

	if (!(prev->next = GxStream_OpenByParam(hls_url, &param))) {
		if(prev->priv){
			stream->err.open_err++;
			if(stream->err.open_err >= 2){
				prev = _find_prev_data_stream(stream, prev);
				if(prev->next){
					hls_destory(prev->next->priv);
					GxStream* p = prev->next;
					prev->next = NULL;
					GxStream_Close(p);
				}
				stream->err.open_err = 0;
				if (_hls_is_over(prev->priv, prev->prog_now)) {
					stream->err.need_restart = 1;
					if (hls_url)
						av_free(hls_url);
					return -1;
				}
			}else{
				if(!is_main_url && prev->priv)
					_back_cur_seq(prev->priv);
			}
		}else{
			stream->err.open_err = 0;
			stream->err.need_restart = 1;
			if (hls_url)
				av_free(hls_url);
			return -1;
		}
		if (hls_url)
			av_free(hls_url);
		return 0;
	}
	prev->next->priv = NULL;
	prev->next->flags = stream->flags;
	stream->err.open_err = 0;
	if (hls_url)
		av_free(hls_url);
	ret = _stream_probe(prev->next);
	if (ret < 0) {
		GxStream *p = prev->next;
		prev->next = NULL;
		GxStream_Close(p);
		return -1;
	} else if (ret == 0) {
		s->prev_update_time   = GxStream_GetTimeMs();
		s->prev_download_time = GxStream_GetTimeMs();
		stream->end_pos = prev->next->end_pos;
		return 1;
	}
	if(is_main_url){
		prev->priv =_new_hls_playlist(prev->next, prev->priv, prev->prog_now);
		if(prev->next){
			GxStream* p = prev->next;
			prev->next = NULL;
			GxStream_Close(p);
		}
		s->prev_update_time = GxStream_GetTimeMs();
	}else{
		prev->next->priv = _new_hls_playlist(prev->next, NULL, prev->prog_now);
		_set_start_seq(prev->next->priv, prev->next->prog_now, stream->cur_seq_no);
		s->prev_update_time = 0;
	}
	return 0;
}

int GxStream_UpdateM3U8(GxStream* stream)
{
	int hls_muliple_list_flg = 0, def_max_url_size = 0;
	GxPlayer_SystemGet(PSYS_NETWORK_HLS_MULTIPLE_LIST, &hls_muliple_list_flg);
	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
	if(hls_muliple_list_flg && stream->hls_thread){
		int ret = 0;
		int is_main_url = 0;
		char *hls_url = NULL;
		GxStream* prev = _find_prev_data_stream(stream, NULL);
		GxStreamM3U8 *s = (GxStreamM3U8 *)stream;

		int64_t reload_interval = _get_reload_interval(prev->priv, prev->prog_now);
		int m3u8_count = _get_hlslist_count(prev->priv, prev->prog_now);
		int list_count = GxStream_StreamList_Get_Count(stream->hls_list);
		gxlogf("m3u8 count %d, thread count %d\n", m3u8_count, list_count);
		if (!(hls_url = av_mallocz(def_max_url_size)))
			return -1;
		if((m3u8_count > 0) && (list_count < HLS_THREAD_COUNT)){
			while(!_get_playurl(stream, prev->priv, hls_url, prev->prog_now)){
				if(GxStream_StreamList_Get_Count(stream->hls_list) >= HLS_THREAD_COUNT){
					_back_cur_seq(prev->priv);
					break;
				}
				GxStream_StreamList_Add(stream->hls_list, hls_url);
				_stream_control(prev, GX_STREAM_CTRL_GET_CUR_SEQ, &stream->cur_seq_no);
				s->prev_update_time = GxStream_GetTimeMs();
				m3u8_count--;
				list_count++;
			}
		}

		if (((m3u8_count <= 0) && (list_count <= 0)) ||
				((GxStream_GetTimeMs() - s->prev_update_time)>reload_interval)) {
			ret = _hls_is_thread_data(prev->priv, prev->prog_now);
			if(ret == -1){
				if (hls_url)
					av_free(hls_url);
				return -1;
			}else if(ret == 1 || ret == 0){
				url_str_copy(prev->url, hls_url, def_max_url_size);
				is_main_url = (ret == 1)?1:2;
			}

re_update:
			if(is_main_url){
				if (prev->next) {
					GxStream *p = prev->next;
					prev->next = NULL;
					GxStream_Close(p);
				}

				GxStreamParam param = {0};
				param.mode     = stream->mode;
				param.prog     = (is_main_url == 2)?stream->prog_now:0;
				param.flags    = stream->flags;
				param.options  = stream->options;
				param.protocol = NULL;
				param.priv     = NULL;

				if (!(prev->next = GxStream_OpenByParam(hls_url, &param))) {
					stream->err.open_err++;
					if(prev != stream){
						prev = _find_prev_data_stream(stream, prev);
						GxStream* p = prev->next;
						prev->next = NULL;
						_stream_control(p, GX_STREAM_CTRL_GET_CUR_SEQ, &stream->cur_seq_no);
						hls_destory(p->priv);
						GxStream_Close(p);
						if (hls_url)
							av_free(hls_url);
						return 0;
					}
					if (hls_url)
						av_free(hls_url);
					return -1;
				}else{
					prev->next->flags = stream->flags;
					if(is_main_url == 3){
						GxStream* p = prev->next;
						p->priv =_new_hls_playlist(p, p->priv, p->prog_now);
						_set_start_seq(p->priv, p->prog_now, stream->cur_seq_no);
						s->prev_update_time = GxStream_GetTimeMs();
						if (hls_url)
							av_free(hls_url);
						return 0;
					}else{
						GxStream* p = prev->next;
						prev->priv =_new_hls_playlist(prev->next, prev->priv, prev->prog_now);
						prev->next = NULL;
						GxStream_Close(p);
						if(is_main_url == 1) {
							HLSContext* hls = prev->priv;
							stream->err.open_err = 0;
							_set_start_seq(prev->priv, prev->prog_now, hls->cur_seq_no);
							s->prev_update_time = GxStream_GetTimeMs();
						}else if(is_main_url == 2){
							_get_playurl(stream, prev->priv, hls_url, prev->prog_now);
							is_main_url = 3;
							goto re_update;
						}
					}
				}
			}
		}
		if (hls_url)
			av_free(hls_url);
	}
	return 0;
}

static int stream_streamlist_read(GxStream*  stream, uint8_t * buffer, size_t max_len)
{
	int hls_muliple_list_flg = 0;
	GxPlayer_SystemGet(PSYS_NETWORK_HLS_MULTIPLE_LIST, &hls_muliple_list_flg);
	if (1 == hls_muliple_list_flg) {
		if(!stream->hls_thread)
			return -2;
		return GxStream_StreamList_Read(stream->hls_list, buffer, max_len);
	}
	return -2;
}

static int stream_streamlist_open(GxStream*  stream)
{
	int hls_muliple_list_flg = 0, def_max_url_size = 0;
	GxPlayer_SystemGet(PSYS_NETWORK_HLS_MULTIPLE_LIST, &hls_muliple_list_flg);
	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
	if (1 == hls_muliple_list_flg) {
		GxStream* prev = _find_prev_data_stream(stream, NULL);

		if(prev){
			stream->hls_thread  = _get_hls_thread_flags(prev->priv, prev->prog_now);
			//seamless switch bandwidth, we will not use multi thread download
			stream->hls_thread &= (stream->prog_max > 1) ? 0 : 1;
			if(stream->hls_thread){
				char *hls_url = NULL;

				if (!(hls_url = av_mallocz(def_max_url_size))) {
					return GX_PLAYER_ERROR;
				}
				if(prev->next){
					GxStream *p = prev->next;
					prev->next = NULL;
					GxStream_Close(p);
				}
				_back_cur_seq(prev->priv);
				_get_playurl(stream, prev->priv, hls_url, prev->prog_now);
				stream->hls_list = GxStream_StreamList_Create(stream, hls_url);
				if(!stream->hls_list) {
					if (hls_url)
						av_free(hls_url);
					return GX_PLAYER_ERROR;
				}
				if (hls_url)
					av_free(hls_url);
			}
		}
	}
	return GX_PLAYER_OK;
}

static void stream_streamlist_close(GxStream* stream)
{
	int hls_muliple_list_flg = 0;
	GxPlayer_SystemGet(PSYS_NETWORK_HLS_MULTIPLE_LIST, &hls_muliple_list_flg);
	if(hls_muliple_list_flg && stream->hls_thread){
		GxStream_StreamList_Destory(stream->hls_list);
		stream->hls_list = NULL;
	}
	return;
}

static int stream_m3u8_read(GxStream*  stream, uint8_t * buffer, size_t max_len)
{
	int is_over = 0;
	int rec_z = 0;
	GxStream* prev = NULL;

	if(_interrupt(stream))
		return -1;

	rec_z = stream_streamlist_read(stream, buffer, max_len);
	if(rec_z != -2){
		goto m3u8_read_out;
	}

	prev = _find_prev_data_stream(stream, NULL);
	if(!prev->next){
		int ret = 0;
		if((ret = _stream_open(stream, prev, 1)) <= 0) {
			if (ret == 0)
				_stream_bandwidth_switch(stream);
			return ret;
		}
		_stream_control(prev, GX_STREAM_CTRL_GET_CUR_SEQ, &stream->cur_seq_no);
	}

	rec_z = _read_data(prev->next, buffer, max_len, &is_over);
	if(is_over){
		GxStream* p = prev->next;
		prev->next = NULL;
		GxStream_Close(p);
		rec_z = 0;
		//check restart demuxer or not
		{
			int ret_args = 0;
			int ret = _stream_control(prev, GX_STREAM_CTRL_IS_NEW_DATA, &ret_args);
			if((ret==GX_PLAYER_OK && ret_args)||
					(stream->demuxer_type == GX_DEMUXER_TYPE_LAVF)){
				ret_args = 1;
				ret = _stream_control(prev, GX_STREAM_CTRL_IS_FINISHED, &ret_args);
				if(ret==GX_PLAYER_OK && !ret_args){
					stream->no_restart_stream = 1;
				}
				rec_z = -1;
			}
		}

		//hls one segment recv over, we judge switch new prog
		_stream_bandwidth_switch(stream);
	}

m3u8_read_out:
#if STREAM_M3U8_PVR_DEBUG
	if(stream_m3u8_pvr_fd && rec_z > 0){
		fwrite(buffer, rec_z, 1, stream_m3u8_pvr_fd);
	}
#endif
	return rec_z;
}

static int get_stream_m3u8_type(GxStream* stream, HLSContext* hls)
{
	/*max_bandwidth:        max bandwidth.         *
	 *second_max_bandwidth: second max bandwidth.  */
	int i = -1, max_bandwidth = -1, second_max_bandwidth = -1;
	if(!hls || hls->n_variants == 0){
		gxlogf("error: hls list not found\n");
		return GX_PLAYER_ERROR;
	}

	stream->prog_max = (hls->n_variants>MAX_PROGRAM_COUNT)?MAX_PROGRAM_COUNT:hls->n_variants;
	stream->is_hls_flg = 1;
	memset (stream->prog, 0, sizeof(stream->prog));

	/*find m3u8 list mulite bandwidth second max value.*/
	max_bandwidth = (stream->prog_max>1)?AV_MAX(hls->variants[0]->bandwidth, hls->variants[1]->bandwidth):-1;
	second_max_bandwidth = (stream->prog_max>1)?AV_MIN(hls->variants[0]->bandwidth, hls->variants[1]->bandwidth):-1;
	for(i = 0; i < stream->prog_max; i++){
		stream->prog[i].bandwidth = hls->variants[i]->bandwidth;
		stream->prog[i].acodec    = hls->variants[i]->acodec_type;
		stream->prog[i].vcodec    = hls->variants[i]->vcodec_type;
		if (stream->prog_max > 1) {
			if(hls->variants[i]->bandwidth > max_bandwidth) {
				second_max_bandwidth = max_bandwidth;
				max_bandwidth = hls->variants[i]->bandwidth;
			}
		} else {
			second_max_bandwidth = max_bandwidth = hls->variants[i]->bandwidth;
		}
	}

	if(stream->prog_now == -1){
		for(i = 0; i < stream->prog_max; i++){
			if(stream->prog[i].bandwidth == second_max_bandwidth){
				stream->prog_now = i;
				break;
			}
		}
	}
	if((stream->prog_now < 0)||(stream->prog_now >= stream->prog_max))
		stream->prog_now = 0;

	return GX_PLAYER_OK;
}

/**************************************************************************************
 *** request mastar m3u8 list.
 *** (for example: get mulit bandwidth list.)
 ***  hls_url: upper input player url.
 **************************************************************************************/
static int _restart_open_m3u8_master_directory_request(GxStream **stream, char *hls_url)
{
	int ret = GX_PLAYER_OK;
	GxStreamParam param;
	GxStream *s = NULL;
	HLSContext* hls = NULL;
	GxStreamClass* st;

	param.mode     = (*stream)->mode;
	param.prog     = 0;
	param.flags    = (*stream)->flags;
	param.options  = (*stream)->options;
	param.protocol = NULL;
	param.priv     = NULL;

	s  = GxStream_OpenByParam((const char*)hls_url, &param);
	if (!s) {
		ret = GX_PLAYER_ERROR;
		goto err_m3u8_open;
	}

	ret = _stream_probe(s);
	if (ret < 0) {
		goto err_m3u8_open;
	}

	hls = _new_hls_playlist(s, hls, -1);
	if(!hls) {
		ret = GX_PLAYER_ERROR;
		goto err_m3u8_open;
	}

	if ((*stream)->priv) {
		hls_destory((*stream)->priv);
		(*stream)->priv = NULL;
	}
	(*stream)->priv = hls;

	if (s->url)
		av_free(s->url);
	if (s->original_url)
		av_free(s->original_url);

	st = GetGxStreamClassFromObject(s);
	if (st && st->close)
		st->close(s);
	if (s->buffer) {
		GxCore_PageFree(s->buffer);
		s->buffer = NULL;
	}
	GxOptions_Destory(s->options);

err_m3u8_open:
	return ret;
}

/**************************************************************************************
 *** request mastar m3u8 list.
 *** (for example: get one bandwidth url list.)
 **************************************************************************************/
static GxStream* _open_m3u8_sub_categories_request(GxStream* stream, HLSContext* hls, int *ret)
{
	int i = -1, retry = 0, sub_cate_re_flag = 0, def_max_url_size = 0;
	char *hls_url = NULL;
	GxStream**p = &stream;

	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
	if (get_stream_m3u8_type(stream, hls)) {
		goto err_m3u8_open;
	}
	if (!(hls_url = av_mallocz(def_max_url_size))) {
		goto err_m3u8_open;
	}

	i = stream->prog_now;
	while(1){
		if(_interrupt(stream) || (retry > 3)) {
			if (hls->n_variants > 1 && !sub_cate_re_flag) {
				struct variant* var = NULL;
				var = hls->variants[(*p)->prog_now];
				if (var) {
					var->n_segments = 0;
					sub_cate_re_flag = 1;/*sub categorites retry request flag.*/
				}
			} else {
				*ret = GX_PLAYER_ERROR;
				goto err_m3u8_open;
			}
		}

		*ret = _get_playurl(stream, (*p)->priv, hls_url, (*p)->prog_now);
		if(*ret < 0){
			gxlogf("url:%s cannot open\n", stream->url);
			goto err_m3u8_open;
		}

		GxStreamParam param;
		param.mode     = stream->mode;
		param.prog     = 0;
		param.flags    = stream->flags;
		param.options  = stream->options;
		param.protocol = NULL;
		param.priv     = NULL;
		(*p)->next     = GxStream_OpenByParam(hls_url, &param);

		if(!(*p)->next){
			if(*p == stream){
				if(stream->prog_max == 1){
					if(stream->prog[0].bandwidth != 0) {/*only one bandwidth of m3u8, so not connect, must break*/
						*ret = -2; /*one bandwidth url.retry mastr url.request m3u8 list*/
						goto err_m3u8_open;
					}
				}else if(stream->prog_max > 1){
					stream->prog_now++;//connect each bandwidth in m3u8
					if(stream->prog_now >= stream->prog_max)
						stream->prog_now = 0;
					if(i == stream->prog_now) {
						*ret = GX_PLAYER_ERROR;
						goto err_m3u8_open;
					}
				}
			}
			retry++;
			continue;
		}
		p = &(*p)->next;
		(*p)->priv = NULL;
		*ret = _stream_probe(*p);
		if (*ret < 0) {
			gxlogf("stream_m3u8.c probe fail.\n");
			goto err_m3u8_open;
		} else if (*ret == 0) {
			break;
		}

		hls = _new_hls_playlist(*p, NULL, (*p)->prog_now);
		if(!hls){
			gxlogf("error: url(%s) open failed\n", hls_url);
			*ret = GX_PLAYER_ERROR;
			goto err_m3u8_open;
		}
		(*p)->priv = hls;
	}
err_m3u8_open:
	if (hls_url)
		av_free(hls_url);
	return *p;
}

static int stream_m3u8_open(GxStream* stream, int mode)
{
	int ret = GX_PLAYER_ERROR, retry = 3;
	HLSContext* hls = stream->priv;
	GxStreamM3U8 *s = (GxStreamM3U8 *)stream;
	GxStream* p = NULL;

	do {
		p = _open_m3u8_sub_categories_request(stream, hls, &ret);/*get hls list(list is ***.ts)*/
		if (ret == -2) { /*get hls list fail(get ***.ts list fail))*/
			retry -= 1;
			if (!(stream->prog_max==1 && hls->variants[0]->bandwidth > 0 && ret == -2)) {
				goto err_m3u8_open;
			}
			ret = _restart_open_m3u8_master_directory_request(&stream, stream->original_url);/*get mastr m3u8 list.(get mulit bandwidth ***.m3u8 list)*/
			s = (GxStreamM3U8 *)stream;
			hls = stream->priv;
		} else {
			if (ret < 0) {
				goto err_m3u8_open;
			}
			break;
		}
	} while(retry > 0);

	if (retry <= 0) {
		ret = GX_PLAYER_ERROR;
		goto err_m3u8_open;
	}
	s->prev_update_time   = 0;
	s->prev_download_time = 0;

	s->prog_target      = stream->prog_now;
	s->stream_buffering = 0;
	stream->file_format = GX_STREAMTYPE_STREAM;
	stream->flags = p->flags;
	stream->end_pos = p->end_pos;
	stream->demuxer_type = p->demuxer_type;
	if(stream_streamlist_open(stream) == GX_PLAYER_ERROR)
		goto err_m3u8_open;

	{
		GxStream* prev = _find_prev_data_stream(stream, NULL);
		if (_get_hls_seekable(prev->priv, prev->prog_now))
			stream->flags |= GX_STREAM_SEEK;
		else
			stream->flags &= ~GX_STREAM_SEEK; //not support seek function.
	}

#if STREAM_M3U8_PVR_DEBUG
	if(stream_m3u8_pvr_fd)
		fclose(stream_m3u8_pvr_fd);
	stream_m3u8_pvr_fd = fopen(stream_m3u8_pvr_file, "w+");
#endif
	return GX_PLAYER_OK;

err_m3u8_open:
	stream_m3u8_close(stream);
	return GX_PLAYER_ERROR;
}

static int stream_m3u8_seek(GxStream*  stream, off_t newpos)
{
	GxStream* prev = _find_prev_data_stream(stream, NULL);
	GxStreamClass* sl = NULL;

	if(!prev->next){
		HLSContext* hls = prev->priv;
		hls->cur_seq_no--;
		if(_stream_open(stream, prev, 0) <= 0)
			return GX_PLAYER_ERROR;
	}

	sl = GetGxStreamClassFromObject(prev->next);
	if(!sl || sl->seek(prev->next, newpos)!=GX_PLAYER_OK)
		return GX_PLAYER_ERROR;

	return GX_PLAYER_OK;
}

static int stream_m3u8_control(GxStream *stream, int cmd, void *arg)
{
	int ret = GX_PLAYER_ERROR;
	GxStream* prev = _find_prev_data_stream(stream, NULL);
	GxStreamM3U8 *s = (GxStreamM3U8 *)stream;

	switch(cmd){
	case GX_STREAM_CTRL_GET_TOTAL_TIME:
	case GX_STREAM_CTRL_GET_TIME_BYSEQ:
	case GX_STREAM_CTRL_GET_SEQ_BYTIME:
	case GX_STREAM_CTRL_GET_CUR_SEQ:
	case GX_STREAM_CTRL_IS_FINISHED:
	case GX_STREAM_CTRL_IS_NEW_DATA:
		ret = _stream_control(prev, cmd, arg);
		break;
	case GX_STREAM_CTRL_SEEK_TO_SEQ:
		{
			HLSContext* hls = prev->priv;
			if(prev->next){
				GxStream *p = prev->next;
				prev->next = NULL;
				GxStream_Close(p);
			}
			hls->cur_seq_no = *(int*)arg-1;//open next seq
			if(_stream_open(stream, prev, 0) <= 0){
				gxlogf("error : %s %d...\n", __FUNCTION__, __LINE__);
				ret = GX_PLAYER_ERROR;
			}
			break;
		}
	case GX_STREAM_CTRL_SEAMLESS_BANDWIDTH_SWITCH:
		{
			if ((*(int*)arg) < stream->prog_max) {
				s->prog_target = *(int*)arg;
				ret = GX_PLAYER_OK;
			} else {
				ret = GX_PLAYER_ERROR;
			}
			break;
		}
	case GX_STREAM_CTRL_STREAM_BUFFERING:
		{
			s->stream_buffering = *(int*)arg;
		}
	default:
		break;
	}
	return ret;
}

static void stream_m3u8_close(GxStream*  stream)
{
	GxStream* p = NULL, *next = NULL;
#if STREAM_M3U8_PVR_DEBUG
	if(stream_m3u8_pvr_fd)
		fclose(stream_m3u8_pvr_fd);
	stream_m3u8_pvr_fd = NULL;
#endif
	stream_streamlist_close(stream);
	p = stream->next;
	while(p){
		next = p->next;
		hls_destory(p->priv);
		GxStream_Close(p);
		p = next;
	}
	hls_destory(stream->priv);
	stream->priv = NULL;
	stream->interrupt_cbk = NULL;
	stream->next = NULL;
}

int stream_m3u8_probe(GxStream *stream, uint8_t* buffer, size_t size, void **priv)
{
	int ret = 0;
	uint8_t *hls_buf = NULL;
	off_t  hls_size = 0;
	off_t  con_size = 0;
	GxStreamClass* sl = GetGxStreamClassFromObject(stream);
	HLSContext* hls = NULL;
	struct variant* var = NULL;

	if (!sl->control)
		return GX_PLAYER_ERROR;

	if (size < GX_HLS_PROBE_LEN)
		return GX_PLAYER_ERROR;

	if (stream->demuxer_type != GX_DEMUXER_TYPE_PLAYLIST) {
		if (hls_probe((const char*)buffer) < 0)
			return GX_PLAYER_ERROR;
	} else if(stream->demuxer_type == GX_DEMUXER_TYPE_PLAYLIST) { //m3u8 and is play list
		stream->demuxer_type = GX_DEMUXER_TYPE_UNKNOWN;
	}

	GxStream_Control(stream, GX_STREAM_CTRL_GET_SIZE, &con_size);

	if (stream->streaming_ctrl && (0 < stream->streaming_ctrl->chk_size) && (0 > stream->streaming_ctrl->con_len))
		con_size = -1;

	con_size = (con_size <= 0) ? 512*1024 : con_size;
	con_size = GXMAX(con_size, size);//add at:if con_size < size, coredump. default 512K,is probe success,read max data 512*1024.

	hls_buf  = av_mallocz((con_size + 1));
	if (!hls_buf)
		return GX_PLAYER_ERROR;

	memcpy(hls_buf, buffer, size);
	hls_size = size;
	while (hls_size < con_size) {
		if (_interrupt(stream)) {
			av_free(hls_buf);
			return GX_PLAYER_ERROR;
		}

		ret = sl->read(stream, hls_buf + hls_size, con_size - hls_size);
		if (ret < 0)
			break;
		hls_size += ret;
	}
	hls_buf[hls_size] = '\0';

	hls = hls_create();
	if (stream->demuxer_type == GX_DEMUXER_TYPE_PLAYLIST) {
		ret = stream_parse_playlist(hls, var, stream->url, (char *)hls_buf);
		if (ret < 0) {
			if (stream->redirect_url)
				ret = hls_parse_playlist(hls, var, stream->redirect_url, (char *)hls_buf);
			else
				ret = hls_parse_playlist(hls, var, stream->url, (char *)hls_buf);
		}
	} else {
		if (stream->redirect_url)
			ret = hls_parse_playlist(hls, var, stream->redirect_url, (char *)hls_buf);
		else
			ret = hls_parse_playlist(hls, var, stream->url, (char *)hls_buf);
	}

	if (ret < 0) {
		hls_destory(hls);
		av_free(hls_buf);
		return GX_PLAYER_ERROR;
	}

	*priv = hls;
	av_free(hls_buf);
	return GX_PLAYER_OK;
}

GxStreamClass gx_stream_m3u8 = {
	._inherit = {
		._inherit = {
			.name = "StreamM3U8",
			.parent = &gx_streambase,
			.size = sizeof(GxStreamM3U8),
		},
	},
	.protocols = {"m3u8", NULL},
	DEF_AUTHOR("Stream","m3u8","No description","L.F","No comment"),

	.flags   = 0,
	.probe   = stream_m3u8_probe,
	.open    = stream_m3u8_open,
	.read    = stream_m3u8_read,
	.write   = NULL,
	.seek    = stream_m3u8_seek,
	.control = stream_m3u8_control,
	.close   = stream_m3u8_close,
};

