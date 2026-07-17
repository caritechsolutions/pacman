#include "hls.h"
#include "gx_url.h"
#include "stheader.h"
#include "../avutil/avstring.h"
#include "../avutil/intreadwrite.h"
#include "../avformat/avformat.h"

#define SPACE_CHARS " \t\r\n"
#define MAX_SEGMENTS (8192)

#define TRY_AV_FREE(a) {    \
        if (a) {            \
              av_free(a);   \
              a = NULL;}}

static int gx_isspace(int c)
{
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' ||
           c == '\v';
}

#define gx_dynarray_add(tab, nb_ptr, elem)\
	do {\
		___gx_dynarray_add((unsigned long **)(tab), nb_ptr, (unsigned long)(elem));\
	} while(0)

/* add one element to a dynamic array*/
static void ___gx_dynarray_add(unsigned long* *tab_ptr, int *nb_ptr, unsigned long elem)
{
	int nb, nb_alloc;
	unsigned long* tab;

	nb =* nb_ptr;
	tab =* tab_ptr;
	if ((nb & (nb - 1)) == 0) {
		if (nb == 0)
			nb_alloc = 1;
		else
			nb_alloc = nb*  2;
		tab = av_realloc(tab, nb_alloc*  sizeof(unsigned long));
		*tab_ptr = tab;
	}
	tab[nb++] = elem;
	*nb_ptr = nb;
}

static int str2acodec(char *acodec_type)
{
	int acodec = -1;

	//if (strstr(acodec_type, "mp4a.40.2") != NULL ||
	//	strstr(acodec_type, "mp4a.40.5") != NULL) {

	if (strstr(acodec_type, "mp4a.") != NULL) {
		acodec = GX_FOURCC('M', 'P', '4', 'A');//AAC
	}

	return acodec;
}

static int str2vcodec(char *vcodec_type)
{
	int vcodec = -1;

	//if (strstr(vcodec_type, "avc1.42001e") != NULL ||
	//	strstr(vcodec_type, "avc1.66.30")  != NULL ||
	//	strstr(vcodec_type, "avc1.42001f") != NULL ||
	//	strstr(vcodec_type, "avc1.4d001e") != NULL ||
	//	strstr(vcodec_type, "avc1.77.30")  != NULL ||
	//	strstr(vcodec_type, "avc1.4d001f") != NULL ||
	//	strstr(vcodec_type, "avc1.4d0028") != NULL ||
	//	strstr(vcodec_type, "avc1.64001f") != NULL ||
	//	strstr(vcodec_type, "avc1.640028") != NULL) {

	if (strstr(vcodec_type, "avc1.") != NULL) {
		vcodec = PLAYER_VCODEC_H264;//VIDEO_H264
	}

	return vcodec;
}

static char* hls_get_line(char *byte, char *line, int maxlen)
{
	int i = 0;
	char c;

	do{
		c = *byte;
		if (c && i < maxlen-1)
			line[i++] = c;
		if(c)
			byte++;
	} while (c != '\n' && c);

	if(i == 0)
		return NULL;
	else {
		if ('\n' == line[i-1])
			line[--i] = '\0';
		else
			line[i] = '\0';
		if (line[--i] == '\r')
			line[i] = '\0';
	}
	//gxlogd("line:%s\n", line);
	return byte;
}

static void hls_parse_key_value(char *str, hls_parse_key_val_cb callback_get_buf, void *context)
{
	char *ptr = str;

	/* Parse key=value pairs. */
	for (;;) {
		char *key;
		char *dest = NULL, *dest_end;
		int key_len, dest_len = 0;

		/* Skip whitespace and potential commas. */
		while (*ptr && (gx_isspace(*ptr) || *ptr == ','))
			ptr++;
		if (!*ptr)
			break;

		key = ptr;

		if (!(ptr = strchr(key, '=')))
			break;
		ptr++;
		key_len = ptr - key;

		callback_get_buf(context, key, key_len, &dest, &dest_len);
		dest_end = dest + dest_len - 1;

		if (*ptr == '\"') {
			ptr++;
			while (*ptr && *ptr != '\"') {
				if (*ptr == '\\') {
					if (!ptr[1])
						break;
					if (dest && dest < dest_end)
						*dest++ = ptr[1];
					ptr += 2;
				} else {
					if (dest && dest < dest_end)
						*dest++ = *ptr;
					ptr++;
				}
			}
			if (*ptr == '\"')
				ptr++;
		} else {
			for (; *ptr && !(gx_isspace(*ptr) || *ptr == ','); ptr++)
				if (dest && dest < dest_end)
					*dest++ = *ptr;
		}
		if (dest)
			*dest = 0;
	}
}

static uint8_t *hls_data_to_hex(uint8_t *buff, const uint8_t *src, int s, int lowercase)
{
	int i;
	static const char hex_table_uc[16] = { '0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'A', 'B',
		'C', 'D', 'E', 'F' };
	static const char hex_table_lc[16] = { '0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'a', 'b',
		'c', 'd', 'e', 'f' };
	const char *hex_table = lowercase ? hex_table_lc : hex_table_uc;

	for(i = 0; i < s; i++) {
		buff[i * 2]     = hex_table[src[i] >> 4];
		buff[i * 2 + 1] = hex_table[src[i] & 0xF];
	}

	return buff;
}

static int hls_hex_to_data(uint8_t *data, const char *p)
{
	int c, len, v;

	len = 0;
	v = 1;
	for (;;) {
		p += strspn(p, SPACE_CHARS);
		if (*p == '\0')
			break;
		c = av_toupper((unsigned char) *p++);
		if (c >= '0' && c <= '9')
			c = c - '0';
		else if (c >= 'A' && c <= 'F')
			c = c - 'A' + 10;
		else
			break;
		v = (v << 4) | c;
		if (v & 0x100) {
			if (data)
				data[len] = v;
			len++;
			v = 1;
		}
	}
	return len;
}

static void hls_make_header_url(char* buf, int size, const char *rel)
{
	char *ptr1 = NULL, *ptr2 = NULL;

	ptr1 = strstr(rel, " -H");
	ptr2 = strstr(buf, " -H");

	if (ptr1 && !ptr2)
		av_strlcat(buf, ptr1, size);

	return;
}

static void hls_make_absolute_url(char *buf, int size, const char *base, const char *rel)
{
	char *sep, *path_query;
	if (!buf || !rel) {
		return;
	}
	/* Absolute path, relative to the current server */
	if (base && rel && strstr(base, "://") && rel[0] == '/') {
		if (base != buf)
			av_strlcpy(buf, base, size);
		sep = strstr(buf, "://");
		if (sep) {
			/* Take scheme from base url */
			if (rel[1] == '/') {
				sep[1] = '\0';
			} else {
				/* Take scheme and host from base url */
				sep += 3;
				sep = strchr(sep, '/');
				if (sep)
					*sep = '\0';
			}
		}
		av_strlcat(buf, rel, size);
		return;
	}
	/* If rel actually is an absolute url, just copy it */
	if (!base || strstr(rel, "://") || rel[0] == '/') {
		av_strlcpy(buf, rel, size);
		return;
	}
	if (base != buf)
		av_strlcpy(buf, base, size);

	/* Strip off any query string from base */
	path_query = strchr(buf, '?');
	if (path_query != NULL)
		*path_query = '\0';

	/* Is relative path just a new query part? */
	if (rel[0] == '?') {
		av_strlcat(buf, rel, size);
		return;
	}

	/* Remove the file name from the base url */
	sep = strrchr(buf, '/');
	if (sep)
		sep[1] = '\0';
	else
		buf[0] = '\0';
	while (av_strstart(rel, "../", (const char**)NULL) && sep) {
		/* Remove the path delimiter at the end */
		sep[0] = '\0';
		sep = strrchr(buf, '/');
		/* If the next directory name to pop off is "..", break here */
		if (!strcmp(sep ? &sep[1] : buf, "..")) {
			/* Readd the slash we just removed */
			av_strlcat(buf, "/", size);
			break;
		}
		/* Cut off the directory name */
		if (sep)
			sep[1] = '\0';
		else
			buf[0] = '\0';
		rel += 3;
	}
	av_strlcat(buf, rel, size);
}

static void hls_handle_variant_args(struct variant_info *info, char *key,
		int key_len, char **dest, int *dest_len)
{
	if (!strncmp(key, "BANDWIDTH=", key_len)) {
		*dest     =        info->bandwidth;
		*dest_len = sizeof(info->bandwidth);
	} else if (!strncmp(key, "CODECS=", key_len)) {
		*dest     =        info->codec_type;
		*dest_len = sizeof(info->codec_type);
	}
}

static void hls_handle_key_args(struct key_info *info, char *key,
		int key_len, char **dest, int *dest_len)
{
	int def_max_url_size = 0;
	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
	if (!strncmp(key, "METHOD=", key_len)) {
		*dest     =        info->method;
		*dest_len = sizeof(info->method);
	} else if (!strncmp(key, "URI=", key_len)) {
		*dest     =        info->uri;
		*dest_len = def_max_url_size;
	} else if (!strncmp(key, "IV=", key_len)) {
		*dest     =        info->iv;
		*dest_len = sizeof(info->iv);
	}
}

static void free_segment_list(struct variant *var)
{
	int i;
	for (i = 0; i < var->n_segments; i++){
		if (var->segments && var->segments[i].key) {
			av_free(var->segments[i].key);
		}
		if (var->segments && var->segments[i].url) {
			av_free(var->segments[i].url);
		}
	}

	if (var->n_segments > 0) {
		if (var->segments) {
			av_freep(&var->segments);
		}
	}
	TRY_AV_FREE(var->url);
	TRY_AV_FREE(var->key_url);
	var->n_segments = 0;
}

static struct variant *new_variant(HLSContext *c, struct variant_info *info, char *url, char *base)
{
	int i = 0, def_max_url_size = 0;
	int bandwidth = 0;
	char *tmp_url = NULL;
	struct variant *var = NULL;
	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);

	if (info != NULL)
		bandwidth = atoi(info->bandwidth);

	for(i = 0; i < c->n_variants; i++){
		if(bandwidth != 0 && c->variants[i]->bandwidth == bandwidth){
			var = c->variants[i];
			if (!(tmp_url = av_mallocz(def_max_url_size))) {
				return NULL;
			}
			hls_make_absolute_url(tmp_url, def_max_url_size-1, base, url);
			TRY_AV_FREE(var->url);
			if (!(var->url = av_strdup(tmp_url))) {
				TRY_AV_FREE(tmp_url);
				return NULL;
			}
			TRY_AV_FREE(tmp_url);
			return var;
		}
	}

	var = av_mallocz(sizeof(struct variant));
	if (!var)
		return NULL;

	if (info != NULL) {
		var->acodec_type = str2acodec(info->codec_type);
		var->vcodec_type = str2vcodec(info->codec_type);
		var->bandwidth   = bandwidth;
	}

	if (!(tmp_url = av_mallocz(def_max_url_size))) {
		TRY_AV_FREE(var);
		return NULL;
	}

	hls_make_absolute_url(tmp_url, def_max_url_size-1, base, url);

	if (!(var->url = av_strdup(tmp_url))) {
		TRY_AV_FREE(tmp_url);
		TRY_AV_FREE(var);
		return NULL;
	}
	TRY_AV_FREE(tmp_url);
	if (!(var->key_url = av_mallocz(def_max_url_size))) {
		TRY_AV_FREE(var->url);
		TRY_AV_FREE(var);
		return NULL;
	}

	gx_dynarray_add(&c->variants, &c->n_variants, var);
	return var;
}

static void free_variant_list(HLSContext *c)
{
	int i;
	for (i = 0; i < c->n_variants; i++) {
		struct variant *var = c->variants[i];
		free_segment_list(var);
		av_free(var);
	}
	if(c->variants > 0)
		av_freep(&c->variants);
	c->n_variants = 0;
}

int stream_parse_playlist(HLSContext* hls, struct variant* var, char* url, char *hls_buf)
{
	int ret = 0, is_right = 0, def_max_url_size = 0;
	char *line = NULL;
	char *p = hls_buf;

	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
	if (!(line=av_mallocz(def_max_url_size)))
		return -1;
	while ((p = hls_get_line(p, line, def_max_url_size-1)) != NULL) {
		if (av_stristr(line, "://")) {
			if (!(var=new_variant(hls, NULL, line, url))) {
				ret = -1;
				goto fail;
			}
			var->totle_duration  = -1;
			var->finished  = 1;
			is_right = 1;
		}
	}
    ret = is_right?0:-1;

fail:
    TRY_AV_FREE(line);
	return ret;
}

int hls_parse_playlist(HLSContext* hls, struct variant* var, char* url, char *hls_buf)
{
	int ret = -1, duration = 0, is_segment = 0, is_variant = 0, is_right = 0, has_iv = 0, def_max_url_size = 0;
	int64_t seg_offset = 0, seg_size = -1;
	enum KeyType key_type = KEY_NONE;
	uint8_t iv[16] = "";
	char *line = NULL, *tmp_str = NULL, *key = NULL, *ptr = NULL, *p = hls_buf;
	struct segment *seg = NULL, *tmp_seg = NULL;
	struct variant_info var_info;

	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
	line = av_mallocz(def_max_url_size);
	if (!line)
		goto fail;
	tmp_str = av_malloc(def_max_url_size);
	if (!tmp_str)
		goto fail;
	p = hls_get_line(p, line, def_max_url_size-1);
	if (strncmp(line, "#EXTM3U", 7)) {
		goto fail;
	}

	if(var){
		free_segment_list(var);
		var->finished = 0;
		var->playlist_type = PLS_TYPE_UNSPECIFIED;
		var->totle_duration = 0;
	}

	while ((p = hls_get_line(p, line, def_max_url_size-1)) != NULL) {
		if (av_strstart(line, "#EXT-X-STREAM-INF:", (const char**)&ptr)) {
			is_variant = 1;
			memset(&var_info, 0, sizeof(struct variant_info));
			hls_parse_key_value(ptr, (hls_parse_key_val_cb) hls_handle_variant_args, &var_info);
		} else if (av_strstart(line, "#EXT-X-KEY:", (const char**)&ptr)) {
			struct key_info info = {0};
			if (!(info.uri = av_mallocz(def_max_url_size))) {
				ret = -1;
				goto fail;
			}
			hls_parse_key_value(ptr, (hls_parse_key_val_cb) hls_handle_key_args, &info);
			key_type = KEY_NONE;
			has_iv = 0;
			if (!strcmp(info.method, "AES-128"))
				key_type = KEY_AES_128;
			if (!strncmp(info.iv, "0x", 2) || !strncmp(info.iv, "0X", 2)) {
				hls_hex_to_data(iv, info.iv + 2);
				has_iv = 1;
			}

			TRY_AV_FREE(key);
			key = av_strdup(info.uri);
			TRY_AV_FREE(info.uri);
			if (!key) {
				ret = -1;
				goto fail;
			}
		} else if (av_strstart(line, "#EXT-X-TARGETDURATION:", (const char**)&ptr)) {
			if (!var) {
				var = new_variant(hls, NULL, url, NULL);
				if (!var) {
					ret = -1;
					goto fail;
				}
			}
			var->target_duration = atoi(ptr);
		} else if (av_strstart(line, "#EXT-X-MEDIA-SEQUENCE:", (const char**)&ptr)) {
			if (!var) {
				var = new_variant(hls, NULL, url, NULL);
				if (!var) {
					ret = -1;
					goto fail;
				}
			}
			var->start_seq_no = atoi(ptr);
			//	gxlogd("start_seq_no: %d\n",var->start_seq_no);
		} else if (av_strstart(line, "#EXT-X-PLAYLIST-TYPE:", (const char**)&ptr)) {
			if (!var) {
				var = new_variant(hls, NULL, url, NULL);
				if (!var) {
					ret = -1;
					goto fail;
				}
			}
			if (!strcmp(ptr, "EVENT"))
				var->playlist_type = PLS_TYPE_EVENT;
			else if (!strcmp(ptr, "VOD"))
				var->playlist_type = PLS_TYPE_VOD;
        } else if (av_strstart(line, "#EXT-X-ENDLIST", (const char**)&ptr)) {
			if (var)
				var->finished = 1;
		} else if (av_strstart(line, "#EXTINF:", (const char**)&ptr)) {
			is_segment = 1;
			duration   = (int)(atof(ptr)*1000);
		} else if (av_strstart(line, "#EXT-X-DISCONTINUITY", (const char**)&ptr)) {
			if(seg)
				seg->new_starttime = 1;
		} else if (av_strstart(line, "#EXT-X-BYTERANGE:", (const char**)&ptr)) {
            seg_size = strtoll(ptr, NULL, 10);
            ptr = strchr(ptr, '@');
            if (ptr)
                seg_offset = strtoll(ptr+1, NULL, 10);
        } else if (av_strstart(line, "#", NULL)) {
			continue;
		} else if (line[0]) {
			if (is_variant) {
				if (!new_variant(hls, &var_info, line, url)) {
					ret = -1;
					goto fail;
				}
				is_variant = 0;
				is_right = 1;
			}
			if (is_segment) {
				if (!var) {
					var = new_variant(hls, NULL, url, NULL);
					if (!var) {
						ret = -1;
						goto fail;
					}
				}
				if (!var->segments) {
					var->totle_segments = 16;
					var->segments = av_malloc(var->totle_segments*sizeof(struct segment));
					if (!var->segments) {
						ret = -1;
						goto fail;
					}
				} else if ((var->n_segments + 1) >= var->totle_segments) {
					if(var->totle_segments * 2 > MAX_SEGMENTS) {
						gxlogi("hls segments > 8K.Only support 8K seg url.\n");
						break;
					}
					tmp_seg = var->segments;
					var->totle_segments *= 2;
					var->segments = av_realloc(var->segments, var->totle_segments*sizeof(struct segment));
					if (!var->segments) {
						ret = -1;
						var->segments = tmp_seg;
						var->totle_segments /= 2;
						goto fail;
					}
				}

				if (!var->segments) {
					ret = -1;
					goto fail;
				}

				seg = &var->segments[var->n_segments];
				if (!seg) {
					ret = -1;
					goto fail;
				}
				seg->duration = duration;
				seg->key_type = key_type;
				seg->key      = NULL;
				seg->url      = NULL;
				if (has_iv) {
					memcpy(seg->iv, iv, sizeof(iv));
				} else {
					int seq = var->start_seq_no + var->n_segments;
					AV_WB32(seg->iv + 12, seq);
				}

				seg->size = seg_size;
				if (seg_size >= 0) {
					seg->url_offset = seg_offset;
					seg_offset += seg_size;
					seg_size = -1;
				} else {
					seg->url_offset = 0;
					seg_offset = 0;
				}

				if (key_type != KEY_NONE) {
					memset (tmp_str, 0, def_max_url_size);
					hls_make_absolute_url(tmp_str, def_max_url_size-1, url, key);
					hls_make_header_url(tmp_str, def_max_url_size-1, url);
					seg->key = av_strdup(tmp_str);
					if (!seg->key) {
						ret = -1;
						goto fail;
					}
				}
				memset (tmp_str, 0, def_max_url_size);
				hls_make_absolute_url(tmp_str, def_max_url_size-1, url, line);
				hls_make_header_url(tmp_str, def_max_url_size-1, url);
				seg->url = av_strdup(tmp_str);
				if (!seg->url) {
					ret = -1;
					goto fail;
				}
				var->totle_duration += duration;
				var->n_segments++;
				is_segment = 0;
				is_right = 1;
			}
		}
	}
	ret = is_right?0:-1;
	if (var && PLS_TYPE_VOD == var->playlist_type && 0 == var->finished) {
		var->finished = 1;
	}

fail:
	TRY_AV_FREE(key);
	TRY_AV_FREE(line);
	TRY_AV_FREE(tmp_str);
	return ret;
}

int hls_get_varurl(HLSContext* c, struct variant *var, char* hls_url)
{
	int def_max_url_size = 0;
	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
	snprintf (hls_url, def_max_url_size-1, "%s", var->url);
	return 0;
}

int hls_get_interval_byseq(HLSContext* c, struct variant* var, int cur_seq_no, int only_cur)
{
	struct segment* seg = NULL;

	if (cur_seq_no < var->start_seq_no) {
		if (only_cur) return 0;
		cur_seq_no = var->start_seq_no;
	}

	if (cur_seq_no >= var->start_seq_no+var->n_segments) {
		if (only_cur) return 0;
		cur_seq_no = var->start_seq_no+var->n_segments -1;
	}

	seg = &var->segments[cur_seq_no - var->start_seq_no];
	return seg->duration;
}

int hls_get_time_byseq(HLSContext* c, struct variant* var, int cur_seq_no)
{
	int i, seq_time = 0;

	if(!var->finished || cur_seq_no<var->start_seq_no)
		return 0;

	if(cur_seq_no >= var->start_seq_no+var->n_segments)
		return var->totle_duration;

	for(i = 0; var->start_seq_no + i < cur_seq_no; i++){
		struct segment *seg = &var->segments[i];
		seq_time += seg->duration;
	}

	return seq_time;
}

int hls_get_base_time(HLSContext* c, struct variant *var, int cur_seq_no)
{
	int seq_no = 0;
	struct segment* seg = NULL;

	if(cur_seq_no<var->start_seq_no ||
			cur_seq_no >= var->start_seq_no+var->n_segments)
		return 0;
	seq_no = cur_seq_no - 1;
	while(seq_no >= var->start_seq_no){
		seg = &var->segments[seq_no-var->start_seq_no];
		if(seg->new_starttime)
			break;
		seq_no--;
	}
	seq_no++;
	return hls_get_time_byseq(c, var, seq_no);
}

int hls_get_seq_bytime(HLSContext*c, struct variant* var, int time)
{
	int second = 0;
	int i = var->start_seq_no;
	while((i-var->start_seq_no)<var->n_segments){
		second += var->segments[i].duration;
		if(second > time)
			break;
		i++;
	}
	return i;
}

static void select_cur_seq_no(GxStream* stream, HLSContext *c, struct variant *var, int64_t ms)
{
	int live_start_index = -3;
	char *live_start;
	if ((live_start = GxOptions_Get_By_Name(stream->options, " -H", "live_start_index:"))) {
		live_start_index = atoi(live_start);
	}

	if(ms < 0){
		if(c->cur_seq_no >= 0) {
			if(c->cur_seq_no < var->start_seq_no) {
				if (!var->finished)//live
					c->cur_seq_no = var->start_seq_no + FFMAX(var->n_segments + live_start_index, 0) - 1;
				else
					c->cur_seq_no = var->start_seq_no - 1;
			}
			var->cur_seq_no = c->cur_seq_no+1;
			if(var->cur_seq_no < 0) {
				if (!var->finished)//live
					var->cur_seq_no = var->start_seq_no + FFMAX(var->n_segments + live_start_index, 0);
				else //vod
					var->cur_seq_no = var->start_seq_no;
			} else if (var->cur_seq_no > (var->n_segments+var->start_seq_no)) {
				if (!var->finished)//live
					c->cur_seq_no = var->start_seq_no + FFMAX(var->n_segments + live_start_index, 0);
				else
					c->cur_seq_no = var->start_seq_no;
				var->cur_seq_no = c->cur_seq_no+1;
			}
		} else {
			if (!var->finished)//live
				var->cur_seq_no = var->start_seq_no + FFMAX(var->n_segments + live_start_index, 0);
			else //vod
				var->cur_seq_no = var->start_seq_no;
		}
	} else {
		var->cur_seq_no = hls_get_seq_bytime(c, var, (int)ms);
	}

}

int hls_get_playurl(GxStream* stream, HLSContext *c, struct variant *var, char* hls_url, int64_t ms)
{
	int ret, def_max_url_size = 0;
	struct segment *seg;

	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
	select_cur_seq_no(stream, c, var, ms);
	if((var->cur_seq_no < var->start_seq_no)
			||(var->cur_seq_no >= var->n_segments+var->start_seq_no))
		return -1;

	seg = &var->segments[var->cur_seq_no - var->start_seq_no];
	gxlogf("cur seq no: %d [%d]\n", var->cur_seq_no, seg->duration);
	gxlogf("nxt seq no: %d -- %d\n", var->start_seq_no, var->start_seq_no + var->n_segments - 1);
	if (seg->key_type == KEY_NONE) {
		url_str_copy(seg->url, hls_url, def_max_url_size);
		if (seg->size > 0) {
			char off[16] = "";
			snprintf(off, sizeof(off), "%lld", seg->url_offset);
			GxOptions_Set(stream->options, " -H", "offset:", off);
			snprintf(off, sizeof(off), "%lld", seg->url_offset + seg->size);
			GxOptions_Set(stream->options, " -H", "end_offset:", off);
		}
		c->cur_seq_no = var->cur_seq_no;
		ret = 0;
	} else if (seg->key_type == KEY_AES_128) {
		extern GxOptions *public_stream_options;
		if (NULL != GxOptions_Get_By_Id(public_stream_options, " -H", OPTIONS_ENC_DRM)) {
			uint8_t iv[33];

			hls_data_to_hex(iv,  (const uint8_t*)seg->iv,  sizeof(seg->iv),  0);
			iv[32] = '\0';
			snprintf(hls_url, strlen(hls_url), "%s -H enc:aes-128-cbc enc_key_url:%s enc_iv:%s", seg->url, seg->key, iv);
			c->cur_seq_no = var->cur_seq_no;
			ret = 0;
		} else {
			uint8_t key[33], iv[33];

			if (var->key_url && seg->key && strcmp(seg->key, var->key_url)) {
				GxStream* key_stream = NULL;

				GxStreamParam param;
				param.mode     = GX_STREAM_READ;
				param.prog     = 0;
				param.flags    = 0;
				param.priv     = NULL;
				param.options  = NULL;
				param.protocol = NULL;

				key_stream = GxStream_OpenByParam(seg->key, &param);
				if (key_stream) {
					int retry = 50;
					GxStreamClass* sl = GetGxStreamClassFromObject(key_stream);

					while (retry) {
						int ret = 0;
						if(sl && sl->read)
							ret = sl->read(key_stream, var->key, sizeof(var->key));
						if(ret > 0)
							break;
						retry--;
					}
					GxStream_Close(key_stream);
					snprintf (var->key_url, def_max_url_size-1, "%s", seg->key);
				}
			}

			memcpy(var->iv, seg->iv, sizeof(seg->iv));
			hls_data_to_hex(iv,  (const uint8_t*)var->iv,  sizeof(var->iv),  0);
			hls_data_to_hex(key, (const uint8_t*)var->key, sizeof(var->key), 0);
			iv[32] = '\0';
			key[32] = '\0';

			snprintf(hls_url, strlen(hls_url), "%s -H enc:aes-128-cbc enc_key:%s enc_iv:%s", seg->url, key, iv);
			if (seg->size > 0) {
				char off[16] = "";
				snprintf(off, sizeof(off), "%lld", seg->url_offset);
				GxOptions_Set(stream->options, " -H", "offset:", off);
				snprintf(off, sizeof(off), "%lld", seg->url_offset + seg->size);
				GxOptions_Set(stream->options, " -H", "end_offset:", off);
			}
			c->cur_seq_no = var->cur_seq_no;
			ret = 0;
		}
	} else
		ret = -1;

	return ret;
}

int hls_is_finished(HLSContext *c, struct variant *var)
{
	if((c->cur_seq_no >= var->n_segments+var->start_seq_no-1)&&
			var->finished)
		return 1;
	else if(var->finished)
		return -1;
	else
		return 0;
}

int hls_is_new_data(HLSContext *c, struct variant *var)
{
	struct segment* seg = NULL;
	if((c->cur_seq_no < var->start_seq_no)
			||(c->cur_seq_no >= var->n_segments+var->start_seq_no))
		return 0;
	seg = &var->segments[c->cur_seq_no - var->start_seq_no];
//	gxlogd("cur_seq_no %d new_starttime %d \n", c->cur_seq_no, seg->new_starttime);
	return seg->new_starttime;
}

int hls_get_probe_len(void)
{
	return strlen("#EXTM3U");
}

int hls_probe(const char* buf)
{
	/* Require #EXTM3U at the start, and either one of the ones below
	 * somewhere for a proper match. */
	if (strncmp(buf, "#EXTM3U", GX_HLS_PROBE_LEN))
		return -1;
	return 0;
}

HLSContext* hls_create(void)
{
	HLSContext* hls;
	hls = av_mallocz(sizeof(HLSContext));
	if(hls == NULL)
		return NULL;
	hls->cur_seq_no = -1;
	return hls;
}

void hls_destory(HLSContext* hls)
{
	if(hls == NULL)
		return;
	free_variant_list(hls);
	av_free(hls);
	hls = NULL;
	return;
}

