/*
 * HTTP protocol for ffmpeg client
 * Copyright (c) 2000, 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "../avutil/avstring.h"
#include "../avutil/bprintf.h"
#include "avformat.h"
#include <unistd.h>
#include "network.h"
#include "http.h"
#include "httpauth.h"
#include "../avutil/opt.h"
#include <zlib.h>

/* XXX: POST protocol is not completely implemented because ffmpeg uses
   only a subset of it. */

/* used for protocol handling */
#define MAX_REDIRECTS 8
#define MAX_DATE_LEN    19
#define WHITESPACES " \n\t\r"
#define HTTP_OFFSET (1024)

/*Only used for streams that support streaming connections (for example:. ts,. flv)*/
#define STREAM_HTTP_PVR_DEBUG 0
#if STREAM_HTTP_PVR_DEBUG
	static char* stream_http_pvr_file = "/mnt/usb01/http_downlod.ts"; /*ecos*/
	//static char* stream_http_pvr_file = "/media/sda1/http_downlod.ts"; /*linux*/
#endif

typedef struct {
	char proto[10]; // 10 byte
	char hostname[HTTP_OFFSET]; // 1KB
	char hoststr[HTTP_OFFSET]; // 1KB
	char auth[HTTP_OFFSET]; // 1KB
	char proxyauth[HTTP_OFFSET]; // 1KB
	char *path1; // 2KB
	char buf[HTTP_OFFSET]; // 1KB
	char *urlbuf; // 2KB
	char *sanitized_path;
}http_open_url_param;

typedef struct {
	char hostname[HTTP_OFFSET];
	char hoststr[HTTP_OFFSET];
	char auth[HTTP_OFFSET];
	char pathbuf[HTTP_OFFSET];
	char lower_url[HTTP_OFFSET+12];
}http_proxy_url_param;

typedef struct {
	URLContext *hd;
	unsigned char *buffer, *buf_ptr, *buf_end;
	int line_count;
	int http_code;
	uint64_t chunksize;      /**< Used if "Transfer-Encoding: chunked" otherwise -1. */
	uint64_t off, filesize, end_off;
	char *uri;
	char *location;
	HTTPAuthState auth_state;
	HTTPAuthState proxy_auth_state;
	char *headers;
	int willclose;          /**< Set if the server correctly handles Connection: close and will close the connection after feeding us the content. */
	int chunked_post;
	int end_chunked_post;   /**< A flag which indicates if the end of chunked encoding has been sent. */
	int end_header;         /**< A flag which indicates we have finished to read POST reply. */
	int multiple_requests;  /**< A flag which indicates if we use persistent connections. */
	uint8_t *post_data;
	int post_datalen;
	int chunkend;
	int is_akamai;
	int is_mediagateway;
	int seekable;           /**< Control seekability, 0 = disable, 1 = enable, -1 = probe. */
	char *http_version;
	char *content_type;
	char *cookies;          ///< holds newline (\n) delimited Set-Cookie header field values (without the "Set-Cookie: " field name)
	int icy;
	int compressed;
	int is_reconnect;
	int is_live;
	unsigned int retry_after;
	z_stream inflate_stream;
#if STREAM_HTTP_PVR_DEBUG
	FILE *download_stream_fp;
#endif
	uint8_t *inflate_buffer;
	/* A dictionary containing cookies keyed by cookie name */
	AVDictionary *cookie_dict;
} HTTPContext;

static int http_connect(URLContext *h, const char *path, const char *local_path,
		const char *hoststr, const char *auth,
		const char *proxyauth, int *new_location);

static void http_printf(char *strings)
{
	int debug_network_stream = 0;

	GxPlayer_SystemGet(PSYS_DEBUG_NETWORK_STREAM, &debug_network_stream);
	if (debug_network_stream)
		gxlogi_raw_l("%s\n", strings);

	return;
}

void ff_http_init_auth_state(URLContext *dest, const URLContext *src)
{
	memcpy(&((HTTPContext*)dest->priv_data)->auth_state,
			&((HTTPContext*)src->priv_data)->auth_state, sizeof(HTTPAuthState));
	memcpy(&((HTTPContext*)dest->priv_data)->proxy_auth_state,
			&((HTTPContext*)src->priv_data)->proxy_auth_state,
			sizeof(HTTPAuthState));
}

int ff_http_averror(int status_code, int default_averror)
{
	switch (status_code) {
	case 400: return AVERROR_HTTP_BAD_REQUEST;
	case 401: return AVERROR_HTTP_UNAUTHORIZED;
	case 403: return AVERROR_HTTP_FORBIDDEN;
	case 404: return AVERROR_HTTP_NOT_FOUND;
	case 429: return AVERROR_HTTP_TOO_MANY_REQUESTS;
	default: break;
	}
	if (status_code >= 400 && status_code <= 499)
		return AVERROR_HTTP_OTHER_4XX;
	else if (status_code >= 500)
		return AVERROR_HTTP_SERVER_ERROR;
	else
		return default_averror;
}

static int http_open_cnx_internal(URLContext *h)
{
	const char *path, *proxy_path = NULL, *lower_proto = "tcp", *local_path;
	char *tmp_path1 = NULL, *tmp_urlbuf = NULL, *hashmark = NULL;
	http_open_url_param *url_param = NULL;
	int port = -1, use_proxy = 0, err = 0, location_changed = 0, def_max_url_size = 0;
	AVDictionaryEntry *e = NULL;
	HTTPContext *s = h->priv_data;

	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);

	if (!(url_param = av_mallocz(sizeof(http_open_url_param)))) {
		err = AVERROR(ENOMEM);
		goto http_fail;
	}

	if (!(tmp_path1 = av_mallocz(def_max_url_size))) {
		err = AVERROR(ENOMEM);
		goto http_fail;
	}
	/* needed in any case to build the host string */
	av_url_split(url_param->proto, sizeof(url_param->proto), url_param->auth, sizeof(url_param->auth), \
		url_param->hostname, sizeof(url_param->hostname), &port, tmp_path1, def_max_url_size-1, s->location);
	ff_url_join(url_param->hoststr, sizeof(url_param->hoststr), NULL, NULL, url_param->hostname, port, NULL);

	if (url_param->path1) {
		av_free(url_param->path1);
		url_param->path1 = NULL;
	}
	if (!(url_param->path1 = av_strdup(tmp_path1))) {
		err = AVERROR(ENOMEM);
		goto http_fail;
	}
	if (tmp_path1) {
		av_free(tmp_path1);
		tmp_path1 = NULL;
	}

	if ((e = av_dict_get(h->url_context_options, "http_proxy", NULL, 0))) {
		proxy_path = e->value;
		use_proxy  = proxy_path && av_strstart(proxy_path, "http://", NULL);
	}

	if (!strcmp(url_param->proto, "https")) {
		lower_proto = "tls";
		use_proxy = 0;
		port = (port < 0)?443:port;
	} else if (!strcmp(url_param->proto, "http")) {
		lower_proto = "tcp";
	}
	port = (port < 0)?80:port;

	hashmark = strchr(url_param->path1, '#');
	if (hashmark)
		*hashmark = '\0';

	if (url_param->path1[0] == '\0') {
		path = "/";
	} else if (url_param->path1[0] == '?') {
		if (!(url_param->sanitized_path = av_mallocz(def_max_url_size))) {
			err = AVERROR(ENOMEM);
			goto http_fail;
		}
		snprintf(url_param->sanitized_path, sizeof(url_param->sanitized_path), "/%s", url_param->path1);
		path = url_param->sanitized_path;
	} else {
		path = url_param->path1;
	}

	local_path = path;
	if (use_proxy) {
		/* Reassemble the request URL without auth string - we don't
		 * want to leak the auth to the proxy. */
		if (!(tmp_urlbuf = av_mallocz(def_max_url_size))) {
			err = AVERROR(ENOMEM);
			goto http_fail;
		}
		ff_url_join(tmp_urlbuf, def_max_url_size-1, url_param->proto, NULL, url_param->hostname, port, "%s", url_param->path1);
		if (url_param->urlbuf) {
			av_free(url_param->urlbuf);
			url_param->urlbuf = NULL;
		}
		if (!(url_param->urlbuf = av_strdup(tmp_urlbuf))) {
			err = AVERROR(ENOMEM);
			goto http_fail;
		}
		if (tmp_urlbuf) {
			av_free(tmp_urlbuf);
			tmp_urlbuf = NULL;
		}
		path = url_param->urlbuf;
		av_url_split(NULL, 0, url_param->proxyauth, sizeof(url_param->proxyauth), url_param->hostname, \
			sizeof(url_param->hostname), &port, NULL, 0, proxy_path);
	}
	ff_url_join(url_param->buf, sizeof(url_param->buf), lower_proto, NULL, url_param->hostname, port, NULL);
	if (!s->hd) {
		err = url_open(&s->hd, url_param->buf, URL_RDWR, &h->url_context_options);
		if (err < 0) {
			goto http_fail;
		}
	}
	err = http_connect(h, path, local_path, url_param->hoststr, url_param->auth, url_param->proxyauth, &location_changed);
http_fail:
	if (tmp_path1) {
		av_free(tmp_path1);
		tmp_path1 = NULL;
	}
	if (url_param->path1) {
		av_free(url_param->path1);
		url_param->path1 = NULL;
	}
	if (url_param->sanitized_path) {
		av_free(url_param->sanitized_path);
		url_param->sanitized_path = NULL;
	}
	if (tmp_urlbuf) {
		av_free(tmp_urlbuf);
		tmp_urlbuf = NULL;
	}
	if (url_param->urlbuf) {
		av_free(url_param->urlbuf);
		url_param->urlbuf = NULL;
	}
	if (url_param)
		av_free(url_param);
	if (err < 0)
		return err;
	return location_changed;
}

static int http_is_reconnect(int err)
{
    int ret = 0;
    switch (err) {
    case AVERROR_HTTP_BAD_REQUEST:
    case AVERROR_HTTP_UNAUTHORIZED:
    case AVERROR_HTTP_FORBIDDEN:
    case AVERROR_HTTP_NOT_FOUND:
    case AVERROR_HTTP_OTHER_4XX:
    case AVERROR_HTTP_SERVER_ERROR:
        ret = 1;
        break;
    case AVERROR_EXIT:
    case AVERROR_NOMEM:
    case AVERROR_EOF:
        ret = 1;
        break;
    case AVERROR_HTTP_TOO_MANY_REQUESTS:
        ret = 0;
        break;
    default:
        ret = 0;
        break;
    }
    return ret;
}

/* return non zero if error */
static int http_open_cnx(URLContext *h)
{
	HTTPContext *s = h->priv_data;
	HTTPAuthType cur_auth_type = HTTP_AUTH_NONE, cur_proxy_auth_type = HTTP_AUTH_NONE;
	int auth_attempts = 0, redirects = 0, ret = 0, conn_attempts = 0;
	int reconnect_delay = 0, reconnect_delay_max = 20, reconnect_delay_total = 0, reconnect_delay_total_max = 45;
	int64_t off;
	if (!s)
		return AVERROR(EINVAL);

redo:
	cur_auth_type = s->auth_state.auth_type;
	cur_proxy_auth_type = s->auth_state.auth_type;
	off = s->off;
	ret = http_open_cnx_internal(h);
	if (ret < 0) {
		if (http_is_reconnect(ret) || (reconnect_delay > reconnect_delay_max) || (reconnect_delay_total > reconnect_delay_total_max))
			goto fail;
		/* Both fields here are in seconds. */
		if (s->retry_after > 0) {
			reconnect_delay = s->retry_after;
			if (reconnect_delay > reconnect_delay_max)/*>20S,goto fail.*/
				goto fail;
			s->retry_after = 0;
			ret = ff_network_sleep_interruptible(1000U*1000*reconnect_delay);
			if (ret != AVERROR(ETIMEDOUT))
				goto fail;
			reconnect_delay_total += reconnect_delay;
			reconnect_delay = 1 + 2 * reconnect_delay;
			conn_attempts+=1;
			gxlogi_raw_l ("http.c open fail.conn_attempts=%d. ret=%d[%s].\n", conn_attempts, ret, av_err2str(ret));
        	} else {
			conn_attempts+=1;
			GxCore_ThreadDelay(20);
			gxlogi_raw_l ("http.c open fail.conn_attempts=%d. ret=%d[%s].\n", conn_attempts, ret, av_err2str(ret));
			if (conn_attempts > 1)
				goto fail;
        	}
		if (s->hd)
			url_closep(&s->hd);
		s->off = off;
		goto redo;
	}

	conn_attempts = 0;
	auth_attempts++;
	gxlogd ("http.c...request http stauts=%d\n", s->http_code);
	if (s->http_code == 401) {
		gxlogi_raw_l ("Server returned 401 Unauthorized (authorization failed)\n");
		if ((cur_auth_type == HTTP_AUTH_NONE || s->auth_state.stale) &&
				s->auth_state.auth_type != HTTP_AUTH_NONE && auth_attempts < 4) {
			url_closep(&s->hd);
			goto redo;
		} else
			goto fail;
	}
	if (s->http_code == 407) {
		gxlogi_raw_l ("Server returned 407 Unauthorized (authorization failed)\n");
		if ((cur_proxy_auth_type == HTTP_AUTH_NONE || s->proxy_auth_state.stale) &&
				s->proxy_auth_state.auth_type != HTTP_AUTH_NONE && auth_attempts < 4) {
			url_closep(&s->hd);
			goto redo;
		} else
			goto fail;
	}
	if ((s->http_code == 301
		|| s->http_code == 302
		|| s->http_code == 303
		|| s->http_code == 307
		|| s->http_code == 308) &&
        ret == 1) {
		/* url moved, get next */
		url_closep(&s->hd);
		if (redirects++ >= MAX_REDIRECTS) {
			return AVERROR(EIO);
		}
		/* Restart the authentication process with the new target, which
		 * might use a different auth mechanism. */
		memset(&s->auth_state, 0, sizeof(s->auth_state));
		auth_attempts = 0;
		ret = 0;
		goto redo;
	}
	return 0;
fail:
	if (s->hd)
		url_closep(&s->hd);
	if (ret < 0)
		return ret;
	return ff_http_averror(s->http_code, AVERROR(EIO));
}

int ff_http_do_new_request(URLContext *h, const char *uri, AVDictionary **opts)
{
	HTTPContext *s = h->priv_data;
	int ret = -1, port1 = -1, port2 = -1;
	char *tmp_buf = av_mallocz(2 * HTTP_OFFSET);
	char *hostname1 = NULL, *hostname2 = NULL;
	char proto1[10], proto2[10];

	if (!tmp_buf)
		return AVERROR(ENOMEM);
	if (!s) {
		av_freep(&tmp_buf);
		return AVERROR(EIO);
	}
	hostname1 = tmp_buf;
	hostname2 = tmp_buf + HTTP_OFFSET;

	if (!h->prot || !(!strcmp(h->prot->name, "http") || !strcmp(h->prot->name, "https"))) {
		if (tmp_buf)
			av_freep(&tmp_buf);
		return AVERROR(EINVAL);
	}

	av_url_split(proto1, sizeof(proto1), NULL, 0,
			hostname1, HTTP_OFFSET, &port1, NULL, 0, s->location);
	av_url_split(proto2, sizeof(proto2), NULL, 0,
			hostname2, HTTP_OFFSET, &port2, NULL, 0, uri);
	if (port1 != port2 || strncmp(hostname1, hostname2, HTTP_OFFSET) != 0) {
		av_freep(&tmp_buf);
		return AVERROR(EINVAL);
	}
	if (tmp_buf)
		av_freep(&tmp_buf);

	s->off = 0;
	s->chunkend      = 0;
	if (s->location)
		av_free(s->location);
	s->location = av_strdup(uri);
	if (!s->location)
		return AVERROR(ENOMEM);
	if (s->uri)
		av_free(s->uri);
	s->uri = av_strdup(uri);
	if (!s->uri) {
		av_free(s->location);
		return AVERROR(ENOMEM);
	}

	if ((ret = av_opt_set_dict(s, opts)) < 0) {
		return ret;
	}
	if (opts && *opts)
		av_dict_copy(&h->url_context_options, *opts, 0);
	if (opts && *opts)
		av_opt_free(*opts);
	return http_open_cnx(h);
}

static int http_open(URLContext *h, const char *uri, int flags)
{
	int ret = 0;
	HTTPContext *s = h->priv_data;
	if (!uri || !s)
		return AVERROR_UNKNOWN;

	h->is_streamed = 1;
	s->filesize = UINT64_MAX;
	s->end_off = 0;
	s->location = av_strdup(uri);
	s->http_version = NULL;
	s->seekable = 1;
	s->retry_after = 0;
	if (!s->location) {
		return AVERROR(ENOMEM);
	}
	s->uri = av_strdup(uri);
	if (!s->uri) {
		if (s->location)
			av_free(s->location);
		return AVERROR(ENOMEM);
	}
	if (s->headers) {
		int len = strlen(s->headers);
		if (len < 2 || strcmp("\r\n", s->headers + len - 2))
			gxlogd ("No trailing CRLF found in HTTP header.\n");
	}

#if STREAM_HTTP_PVR_DEBUG
	if(s->download_stream_fp)
		fclose(s->download_stream_fp);
	s->download_stream_fp = fopen(stream_http_pvr_file,"wb+");
#endif

	if ((ret = http_open_cnx(h)) < 0) {
		if (s->location) {
			av_free(s->location);
			s->location = NULL;
		}
		if (s->uri) {
			av_free(s->uri);
			s->uri = NULL;
		}
		if (s->cookies) {
			av_free(s->cookies);
			s->cookies= NULL;
		}
		if (s->buffer) {
			av_free(s->buffer);
			s->buffer = NULL;
		}
	}
	return ret;
}

static int http_getc(HTTPContext *s)
{
	int len = -1, def_max_url_size = 0;

	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
	if (s->buf_ptr >= s->buf_end) {
		len = url_read(s->hd, s->buffer, def_max_url_size);
		if (len < 0) {
			return len;
		} else if (len == 0) {
			return AVERROR_EOF;
		} else {
			s->buf_ptr = s->buffer;
			s->buf_end = s->buffer + len;
		}
	}
	return *s->buf_ptr++;
}

static int http_get_line(HTTPContext *s, char *line, int line_size)
{
	int ch = -1;
	char *q = NULL;

	q = line;
	for(;;) {
		ch = http_getc(s);
		if (ch < 0)
			return ch;
		if (ch == '\n') {
			/* process line */
			if (q > line && q[-1] == '\r')
				q--;
			*q = '\0';

			return 0;
		} else {
			if ((q - line) < line_size - 1)
				*q++ = ch;
		}
	}
}

static int check_http_code(URLContext *h, int http_code, const char *end)
{
	HTTPContext *s = h->priv_data;
	/* error codes are 4xx and 5xx, but regard 401 as a success, so we
	* don't abort until all headers have been parsed. */
	if (http_code >= 400 && http_code < 600 &&
	(http_code != 401 || s->auth_state.auth_type != HTTP_AUTH_NONE) &&
	(http_code != 407 || s->proxy_auth_state.auth_type != HTTP_AUTH_NONE)) {
		end += strspn(end, SPACE_CHARS);
		gxlogd ("HTTP error %d %s\n", http_code, end);
		return ff_http_averror(http_code, AVERROR(EIO));
	}
	return 0;
}

static int parse_location(URLContext *h, HTTPContext *s, const char *p)
{
	char *redirected_location = NULL, *new_loc = NULL;
	int def_max_url_size = 0;

	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
	if (!(redirected_location = av_mallocz(def_max_url_size)))
		return AVERROR(ENOMEM);
	ff_make_absolute_url(redirected_location, def_max_url_size - 1, s->location, p);
	new_loc = av_strdup(redirected_location);
	if (redirected_location)
		av_freep(&redirected_location);
	if (!new_loc)
		return AVERROR(ENOMEM);
	if (s->location)
		av_freep(&s->location);
	s->location = new_loc;
	if (s->location)
	    av_dict_set(&(h->url_context_options), "location", s->location, 0);
	return 0;
}

/* "bytes $from-$to/$document_size" */
static void parse_content_range(URLContext *h, const char *p)
{
	HTTPContext *s = h->priv_data;
	const char *slash;

	if (!strncmp(p, "bytes ", 6)) {
		p     += 6;
		s->off = strtoull(p, NULL, 10);
		if ((slash = strchr(p, '/')) && strlen(slash) > 0)
			s->filesize = strtoull(slash + 1, NULL, 10);
	}
	if (s->seekable == 1 && (!s->is_akamai || s->filesize != 2147483647))
		h->is_streamed = 0; /* we _can_ in fact seek */
}

static int parse_http_date(const char *date_str, struct tm *buf)
{
	char date_buf[MAX_DATE_LEN];
	int i, j, date_buf_len = MAX_DATE_LEN-1;
	char *date;

	// strip off any punctuation or whitespace
	for (i = 0, j = 0; date_str[i] != '\0' && j < date_buf_len; i++) {
		if ((date_str[i] >= '0' && date_str[i] <= '9') ||
		(date_str[i] >= 'A' && date_str[i] <= 'Z') ||
		(date_str[i] >= 'a' && date_str[i] <= 'z')) {
			date_buf[j] = date_str[i];
			j++;
		}
	}
	date_buf[j] = '\0';
	date = date_buf;
	// move the string beyond the day of week
	while ((*date < '0' || *date > '9') && *date != '\0')
		date++;
	return av_small_strptime(date, "%d%b%Y%H%M%S", buf) ? 0 : AVERROR(EINVAL);
}

static int parse_set_cookie(const char *set_cookie, AVDictionary **dict)
{
	char *param, *next_param, *cstr, *back;

	if (!set_cookie[0])
		return 0;

	if (!(cstr = av_strdup(set_cookie)))
		return AVERROR(EINVAL);

	// strip any trailing whitespace
	back = &cstr[strlen(cstr)-1];
	while (strchr(WHITESPACES, *back)) {
		*back='\0';
		if (back == cstr)
			break;
		back--;
	}

	next_param = cstr;
	while ((param = av_strtok(next_param, ";", &next_param))) {
		char *name = NULL, *value = NULL;
		param += strspn(param, WHITESPACES);
		if ((name = av_strtok(param, "=", &value))) {
			if (av_dict_set(dict, name, value, 0) < 0) {
				av_free(cstr);
				return -1;
			}
		}
	}

	av_free(cstr);
	return 0;
}

static int parse_cookie(HTTPContext *s, const char *p, AVDictionary **cookies)
{
	AVDictionary *new_params = NULL;
	AVDictionaryEntry *e = NULL, *cookie_entry = NULL;
	char *eql = NULL, *name = NULL;

	// ensure the cookie is parsable
	if (parse_set_cookie(p, &new_params))
		return -1;

	// if there is no cookie value there is nothing to parse
	cookie_entry = av_dict_get(new_params, "", NULL, AV_DICT_IGNORE_SUFFIX);
	if (!cookie_entry || !cookie_entry->value) {
		av_dict_free(&new_params);
		return -1;
	}

	// ensure the cookie is not expired or older than an existing value
	if ((e = av_dict_get(new_params, "expires", NULL, 0)) && e->value) {
		struct tm new_tm = {0};
		if (!parse_http_date(e->value, &new_tm)) {
			AVDictionaryEntry *e2 = NULL;

			// if the cookie has already expired ignore it
			if (av_timegm(&new_tm) < av_gettime() / 1000000) {
				av_dict_free(&new_params);
				return 0;
			}

			// only replace an older cookie with the same name
			e2 = av_dict_get(*cookies, cookie_entry->key, NULL, 0);
			if (e2 && e2->value) {
				AVDictionary *old_params = NULL;
				if (!parse_set_cookie(p, &old_params)) {
					e2 = av_dict_get(old_params, "expires", NULL, 0);
					if (e2 && e2->value) {
						struct tm old_tm = {0};
						if (!parse_http_date(e->value, &old_tm)) {
							if (av_timegm(&new_tm) < av_timegm(&old_tm)) {
								av_dict_free(&new_params);
								av_dict_free(&old_params);
								return -1;
							}
						}
					}
				}
				av_dict_free(&old_params);
			}
		}
	}
	av_dict_free(&new_params);

	// duplicate the cookie name (dict will dupe the value)
	if (!(eql = strchr(p, '=')))
		return AVERROR(EINVAL);
	if (!(name = av_strndup(p, eql - p)))
		return AVERROR(ENOMEM);

	// add the cookie to the dictionary
	av_dict_set(cookies, name, eql, 0);
	if (name) {
		av_free(name);
	}

	return 0;
}

static int parse_content_encoding(URLContext *h, const char *p)
{
    if (!av_strncasecmp(p, "gzip", 4) ||
        !av_strncasecmp(p, "deflate", 7)) {
        HTTPContext *s = h->priv_data;
        s->compressed = 1;
        inflateEnd(&s->inflate_stream);
        if (inflateInit2(&s->inflate_stream, 32 + 15) != Z_OK) {
            gxloge("Error during zlib initialisation:%s\n", s->inflate_stream.msg);
            return AVERROR(ENOSYS);
        }
        if (zlibCompileFlags() & (1 << 17)) {
            gxloge("Your zlib was compiled without gzip support.\n");
            return AVERROR(ENOSYS);
        }
    } else if (!av_strncasecmp(p, "identity", 8)) {
        // The normal, no-encoding case (although servers shouldn't include
        // the header at all if this is the case).
    } else {
        gxloge("Unknown content coding:%s\n", p);
    }
    return 0;
}

static int process_line(URLContext *h, char *line, int line_count,
		int *new_location, int *parsed_http_code)
{
	HTTPContext *s = h->priv_data;
	char *tag = NULL, *p = NULL, *end = NULL;
	int ret = 0;

	/* end of header */
	if (line[0] == '\0') {
		s->end_header = 1;
		return 0;
	}

	p = line;
	if (line_count == 0) {
		if (av_strncasecmp(p, "HTTP/1.0", 8) == 0)
			s->willclose = 1;
		while (*p != '/' && *p != '\0')
			p++;
		while (*p == '/')
			p++;
		if (s->http_version) {
			av_free(s->http_version);
			s->http_version = NULL;
		}
		s->http_version = av_strndup(p, 3);
		if (!s->http_version)
			return AVERROR(ENOMEM);
		av_dict_set(&h->url_context_options, "http_version", s->http_version, 0);
		if (s->http_version) {
			av_free(s->http_version);
			s->http_version = NULL;
		}

		while (!av_isspace(*p) && *p != '\0')
			p++;
		while (av_isspace(*p))
			p++;
		s->http_code = strtol(p, &end, 10);

		gxlogd("http_code=%d\n", s->http_code);
		 *parsed_http_code = 1;
		if ((ret = check_http_code(h, s->http_code, end)) < 0)
			return ret;
	} else {
		while (*p != '\0' && *p != ':')
			p++;
		if (*p != ':')
			return 1;

		*p = '\0';
		tag = line;
		p++;
		while (av_isspace(*p))
			p++;
		if (!av_strcasecmp(tag, "Location")) {
			if ((ret = parse_location(h, s, p)) < 0)
				return ret;
			*new_location = 1;
		} else if (!av_strcasecmp (tag, "Content-Length") && s->filesize == UINT64_MAX) {
			s->filesize = atoll(p);
		} else if (!av_strcasecmp (tag, "Content-Range")) {
			parse_content_range(h, p);
		} else if (!av_strcasecmp(tag, "Accept-Ranges") && !strncmp(p, "bytes", 5)) {
			h->is_streamed = 0;
		} else if (!av_strcasecmp (tag, "Transfer-Encoding") && !av_strncasecmp(p, "chunked", 7)) {
			s->filesize = UINT64_MAX;
			s->chunksize = 0;
		} else if (!av_strcasecmp (tag, "WWW-Authenticate")) {
			ff_http_auth_handle_header(&s->auth_state, tag, p);
		} else if (!av_strcasecmp (tag, "Authentication-Info")) {
			ff_http_auth_handle_header(&s->auth_state, tag, p);
		} else if (!av_strcasecmp (tag, "Proxy-Authenticate")) {
			ff_http_auth_handle_header(&s->proxy_auth_state, tag, p);
		} else if (!av_strcasecmp(tag, "Server")) {
			if (!av_strcasecmp(p, "AkamaiGHost")) {
				s->is_akamai = 1;
			} else if (!av_strncasecmp(p, "MediaGateway", 12)) {
				s->is_mediagateway = 1;
			}
		} else if (!av_strcasecmp (tag, "Connection")) {
			if (!strcmp(p, "close"))
				s->willclose = 1;
		} else if (!av_strcasecmp(tag, "Set-Cookie")) {
			if (parse_cookie(s, p, &s->cookie_dict))
				gxlogd("Unable to parse '%s'\n", p);
		} else if (!av_strcasecmp(tag, "Content-Encoding")) {
			if ((ret = parse_content_encoding(h, p)) < 0) {
                return ret;
            }
		} else if (!av_strcasecmp(tag, "Retry-After")) {
			/* The header can be either an integer that represents seconds, or a date. */
			struct tm tm;
			int date_ret = parse_http_date(p, &tm);
			if (!date_ret) {
				time_t retry   = av_timegm(&tm);
				int64_t now    = av_gettime() / 1000000;
				int64_t diff   = ((int64_t) retry) - now;
				s->retry_after = (unsigned int) FFMAX(0, diff);
			} else {
				s->retry_after = strtoul(p, NULL, 10);
			}
		}
	}
	return 1;
}


/**
 * Create a string containing cookie values for use as a HTTP cookie header
 * field value for a particular path and domain from the cookie values stored in
 * the HTTP protocol context. The cookie string is stored in *cookies, and may
 * be NULL if there are no valid cookies.
 *
 * @return a negative value if an error condition occurred, 0 otherwise
 */
static int get_cookies(HTTPContext *s, char **cookies, const char *path,
		const char *domain)
{
	// cookie strings will look like Set-Cookie header field values.  Multiple
	// Set-Cookie fields will result in multiple values delimited by a newline
	int ret = 0;
	char *cookie = NULL, *set_cookies = NULL, *next = NULL;

	// destroy any cookies in the dictionary.
	av_dict_free(&s->cookie_dict);

	if (!s->cookies)
		return 0;

	next = set_cookies = av_strdup(s->cookies);
	if (!next)
		return AVERROR(ENOMEM);

	*cookies = NULL;
	while ((cookie = av_strtok(next, "\n", &next)) && !ret) {
		AVDictionary *cookie_params = NULL;
		AVDictionaryEntry *cookie_entry, *e;

		// store the cookie in a dict in case it is updated in the response
		if (parse_cookie(s, cookie, &s->cookie_dict))
			gxloge("Unable to parse '%s'\n", cookie);

		// continue on to the next cookie if this one cannot be parsed
		if (parse_set_cookie(cookie, &cookie_params))
			goto skip_cookie;

		// if the cookie has no value, skip it
		cookie_entry = av_dict_get(cookie_params, "", NULL, AV_DICT_IGNORE_SUFFIX);
		if (!cookie_entry || !cookie_entry->value)
			goto skip_cookie;

		// if the cookie has expired, don't add it
		if ((e = av_dict_get(cookie_params, "expires", NULL, 0)) && e->value) {
			struct tm tm_buf = {0};
			if (!parse_http_date(e->value, &tm_buf)) {
				if (av_timegm(&tm_buf) < av_gettime() / 1000000)
					goto skip_cookie;
			}
		}

		// if no domain in the cookie assume it appied to this request
		if ((e = av_dict_get(cookie_params, "domain", NULL, 0)) && e->value) {
			// find the offset comparison is on the min domain (b.com, not a.b.com)
			int domain_offset = strlen(domain) - strlen(e->value);
			if (domain_offset < 0)
				goto skip_cookie;

			// match the cookie domain
			if (av_strcasecmp(&domain[domain_offset], e->value))
				goto skip_cookie;
		}

		// ensure this cookie matches the path
		e = av_dict_get(cookie_params, "path", NULL, 0);
		if (!e || av_strncasecmp(path, e->value, strlen(e->value)))
			goto skip_cookie;

		// cookie parameters match, so copy the value
		if (!*cookies) {
			*cookies = av_asprintf("%s=%s", cookie_entry->key, cookie_entry->value);
		} else {
			char *tmp = *cookies;
			*cookies = av_asprintf("%s; %s=%s", tmp, cookie_entry->key, cookie_entry->value);
			av_free(tmp);
		}
		if (!*cookies)
			ret = AVERROR(ENOMEM);

skip_cookie:
		av_dict_free(&cookie_params);
	}

	if (set_cookies)
		av_freep(&set_cookies);

	return ret;
}

static inline int has_header(const char *str, const char *header)
{
	/* header + 2 to skip over CRLF prefix. (make sure you have one!) */
	if (!str)
		return 0;
	return av_stristart(str, header + 2, NULL) || av_stristr(str, header);
}

static int cookie_string(AVDictionary *dict, char **cookies)
{
	AVDictionaryEntry *e = NULL;
	int len = 1;

	// determine how much memory is needed for the cookies string
	while ((e = av_dict_get(dict, "", e, AV_DICT_IGNORE_SUFFIX)))
		len += strlen(e->key) + strlen(e->value) + 1;

	if (len > 1) {
		// reallocate the cookies
		e = NULL;
		if (*cookies) av_freep(cookies);
		*cookies = av_malloc(len);
		if (!*cookies)
			return AVERROR(ENOMEM);
		*cookies[0] = '\0';

		// write out the cookies
		while ((e = av_dict_get(dict, "", e, AV_DICT_IGNORE_SUFFIX)))
			av_strlcatf(*cookies, len, "%s%s\n", e->key, e->value);
	}
	return 0;
}

static int http_read_header(URLContext *h, int *new_location)
{
	HTTPContext *s = h->priv_data;
	char *line = NULL;
	int err = 0, http_err = 0, def_max_url_size = 0;

	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
	if (!(line = av_mallocz(def_max_url_size))) {
		return AVERROR(ENOMEM);
	}
	http_printf("<<<<<<<<<<");
	for (;;) {
		int parsed_http_code = 0;
		if ((err = http_get_line(s, line, def_max_url_size)) < 0) {
			av_free(line);
			return err;
		}

		http_printf(line);
		err = process_line(h, line, s->line_count, new_location, &parsed_http_code);
		if (err < 0) {
			if (parsed_http_code) {
				http_err = err;
			} else {
				av_free(line);
				/* Prefer to return HTTP code error if we've already seen one. */
				return (http_err?http_err:err);
			}
		}
		if (err == 0) {
			break;
		}
		s->line_count++;
	}

	if (http_err) {
		av_free(line);
		return http_err;
	}

	if (s->seekable == 1 && s->is_mediagateway && s->filesize == 2000000000)
		h->is_streamed = 1; /* we can in fact _not_ seek */

	// add any new cookies into the existing cookie string
	cookie_string(s->cookie_dict, &s->cookies);
	if (s->cookies) {
	    av_dict_set(&(h->url_context_options), "ffcookies", s->cookies, 0);
	}
	if (s->cookie_dict)
		av_dict_free(&s->cookie_dict);
	av_free(line);
	return err;
}

/**
 * Escape unsafe characters in path in order to pass them safely to the HTTP
 * request. Insipred by the algorithm in GNU wget:
 * - escape "%" characters not followed by two hex digits
 * - escape all "unsafe" characters except which are also "reserved"
 * - pass through everything else
 */
static void bprint_escaped_path(AVBPrint *bp, const char *path)
{
    int len = 0, def_max_url_size = 0;
	char *buf = NULL;
#define NEEDS_ESCAPE(ch) \
    ((ch) <= ' ' || (ch) >= '\x7f' || \
     (ch) == '"' || (ch) == '%' || (ch) == '<' || (ch) == '>' || (ch) == '\\' || \
     (ch) == '^' || (ch) == '`' || (ch) == '{' || (ch) == '}' || (ch) == '|')

	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
	if (!(buf = av_mallocz(def_max_url_size)))
		return ;
    while (*path) {
        memset (buf, 0, def_max_url_size);
        char *q = buf;
        while (*path && q - buf < def_max_url_size - 4) {
            if (path[0] == '%' && av_isxdigit(path[1]) && av_isxdigit(path[2])) {
                *q++ = *path++;
                *q++ = *path++;
                *q++ = *path++;
            } else if (NEEDS_ESCAPE(*path)) {
                q += snprintf(q, 4, "%%%02X", (uint8_t)*path++);
            } else {
                *q++ = *path++;
            }
        }
		av_bprint_append_data(bp, buf, q - buf);
    }
	if (buf) {
		av_free(buf);
		buf = NULL;
	}
}

static int custom_http_headers_callback(AVBPrint *bp, int *len)
{
#define DEFINE_CUSTOM_FLAGS "http"
#define DEFINE_MAX_CUSTOM_NUM 32
	extern GxStreamOptionCallback public_stream_callback;

	if (public_stream_callback) {
		int i = 0, ret = 0, max_custom = 0;;
		GxStreamOptionCallbackParam param;
		if (!(param.custom_str = av_mallocz(DEFINE_MAX_CUSTOM_NUM)))
			return AVERROR(ENOMEM);
		if ((ret = public_stream_callback(&param)) == 0) {
			max_custom = FFMIN(param.custom_num, DEFINE_MAX_CUSTOM_NUM);
			if (!av_strncasecmp(param.custom_flag, DEFINE_CUSTOM_FLAGS, strlen(DEFINE_CUSTOM_FLAGS))) {
				for (i = 0; i < max_custom; i++) {
					av_bprintf(bp, "%s\r\n", param.custom_str[i]);
				}
			}
		}
		av_free(param.custom_str);
		param.custom_str = NULL;
		return 0;
	}
	return AVERROR(EINVAL);
}

static int http_connect(URLContext *h, const char *path, const char *local_path,
		const char *hoststr, const char *auth, const char *proxyauth, int *new_location)
{
	HTTPContext *s = h->priv_data;
	int post=0, err = 0, len = 0, def_max_url_size = 0;
	char *authstr = NULL, *proxyauthstr = NULL;
	int64_t off = s->off;
	const char *method;
	AVDictionaryEntry *e = NULL;
	AVBPrint request;

	/* send http header */
	post = h->flags & AVIO_FLAG_WRITE;

	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
	if (s->buffer) {
		av_freep(&s->buffer);
	}
	if (!(s->buffer = av_mallocz((def_max_url_size+HTTP_HEADERS_MAX_SIZE))))
		return AVERROR(ENOMEM);
	av_bprint_init_for_buffer(&request, (char *)s->buffer, def_max_url_size+HTTP_HEADERS_MAX_SIZE);

	if (s->post_data) {
		/* force POST method and disable chunked encoding when custom HTTP post data is set */
		post = 1;
		s->chunked_post = 0;
	}

	method = post ? "POST" : "GET";
	authstr = ff_http_auth_create_response(&s->auth_state, auth, local_path, method);
	proxyauthstr = ff_http_auth_create_response(&s->proxy_auth_state, proxyauth, local_path, method);

    av_bprintf(&request, "%s ", method);
    bprint_escaped_path(&request, path);
    av_bprintf(&request, " HTTP/1.1\r\n");

	s->is_reconnect = 1;
	if ((e = av_dict_get(h->url_context_options, "is_reconnect", NULL, 0))) {
		s->is_reconnect = strtoll(e->value, NULL, 10);
	}
	s->is_live = 0;
	if ((e = av_dict_get(h->url_context_options, "is_live", NULL, 0))) {
		s->is_live = strtoll(e->value, NULL, 10);
	}

	/* set default headers if needed */
	if (!has_header(s->headers, "\r\nUser-Agent: ")) {
		if ((e = av_dict_get(h->url_context_options, "user_agent", NULL, 0))) {
			av_bprintf(&request, "User-Agent: %s\r\n", e->value);
		} else {
			av_bprintf(&request, "User-Agent: %s\r\n", LIBAVFORMAT_IDENT);
		}
	}

	if (!has_header(s->headers, "\r\nxClientGUID: ")) {
		if ((e = av_dict_get(h->url_context_options, "xClientGUID", NULL, 0))) {
			av_bprintf(&request, "Pragma: xClientGUID={%s}\r\n", e->value);
			av_bprintf(&request, "X-Accept-Authentication: Negotiate, NTLM, Digest, Basic\r\n");
			av_bprintf(&request, "Pragma: no-cache,rate=1.000000,stream-time=0,stream-offset=0:0,request-context=1,max-duration=0\r\n");
			av_bprintf(&request, "Supported: com.microsoft.wm.srvppair, com.microsoft.wm.sswitch, com.microsoft.wm.predstrm\r\n");
		}
	}

	if (!has_header(s->headers, "\r\nReferer: ")) {
		if ((e = av_dict_get(h->url_context_options, "referer", NULL, 0))) {
			av_bprintf(&request, "Referer: %s\r\n", e->value);
		}
	}

	if ((e = av_dict_get(h->url_context_options, "disable_accept", NULL, 0))) {
		if (0 == strtoll(e->value, NULL, 10)) {
			av_bprintf(&request, "Accept: */*\r\n");
		}
	} else {
		if (!has_header(s->headers, "\r\nAccept: "))
			av_bprintf(&request, "Accept: */*\r\n");
	}

	if ((e = av_dict_get(h->url_context_options, "offset", NULL, 0))) {
		s->off = strtoll(e->value, NULL, 10);
		off = s->off;
	}

	if ((e = av_dict_get(h->url_context_options, "end_offset", NULL, 0))) {
		s->end_off = strtoll(e->value, NULL, 10);
	}

	if ((e = av_dict_get(h->url_context_options, "seekable", NULL, 0))) {
		s->seekable = atoi(e->value);
	}
	if (!has_header(s->headers, "\r\nRange: ") && !post && (s->off > 0 || s->end_off || s->seekable != 0)) {
		av_bprintf(&request, "Range: bytes=%lld-", s->off);
		if (s->end_off)
			av_bprintf(&request, "%lld", s->end_off - 1);
		av_bprintf(&request, "\r\n");
	}
	if (!has_header(s->headers, "\r\nConnection: ")) {
		if ((e = av_dict_get(h->url_context_options, "Connection", NULL, 0))) {
			av_bprintf(&request, "Connection: %s\r\n", e->value);
		} else {
			if ((e = av_dict_get(h->url_context_options, "multiple_requests", NULL, 0))) {
				av_bprintf(&request, "Connection: %s\r\n", (atoi(e->value)) ? "keep-alive" : "close");
				s->multiple_requests = atoi(e->value)?1:0;
			} else {
				av_bprintf(&request, "Connection: %s\r\n", "close");
			}
		}
	}

	if (!has_header(s->headers, "\r\nHost: "))
		av_bprintf(&request, "Host: %s\r\n", hoststr);
	if (!has_header(s->headers, "\r\nContent-Length: ") && s->post_data)
		av_bprintf(&request, "Content-Length: %d\r\n", s->post_datalen);

	if (!has_header(s->headers, "\r\nContent-Type: ") && s->content_type)
		av_bprintf(&request, "Content-Type: %s\r\n", s->content_type);

	if (!has_header(s->headers, "\r\nCookie: ")) {
		char *cookies = NULL;
		int is_ffcookies = 0;
		if ((e = av_dict_get(h->url_context_options, "ffcookies", NULL, 0))) {
			if (s->cookies)
				av_freep(&s->cookies);
			s->cookies = av_strdup(e->value);
			if (!s->cookies) {
				av_freep(&authstr);
				av_freep(&proxyauthstr);
				return AVERROR(ENOMEM);
			}
			is_ffcookies = 1;
		}
		if (s->cookies && !get_cookies(s, &cookies, path, hoststr) && cookies) {
			av_bprintf(&request, "Cookie: %s\r\n", cookies);
			if (cookies)
				av_free(cookies);
			if (is_ffcookies) {
				av_freep(&s->cookies);
			}
		} else {
			/*non-standard cookies, use defalut cookies.*/
			if (is_ffcookies) {
				av_bprintf(&request, "Cookie: %s\r\n", s->cookies);
				if (s->cookies)
					av_freep(&s->cookies);
			}
		}
	}
	if (!has_header(s->headers, "\r\nIcy-MetaData: ") && s->icy)
		av_bprintf(&request, "Icy-MetaData: %d\r\n", 1);

	if ((e = av_dict_get(h->url_context_options, "Authorization", NULL, 0))) {
		av_bprintf(&request, "Authorization: %s\r\n", e->value);
	}

	/* now add in custom headers */
	if ((e = av_dict_get(h->url_context_options, "CustomHeaders", NULL, 0))) {
		av_bprintf(&request, "%s\r\n", e->value);
	}

	custom_http_headers_callback(&request, &len);

    if (authstr)
        av_bprintf(&request, "%s", authstr);
    if (proxyauthstr)
        av_bprintf(&request, "Proxy-%s", proxyauthstr);

	av_bprintf(&request, "\r\n");
	av_freep(&authstr);
	av_freep(&proxyauthstr);

    if (!av_bprint_is_complete(&request)) {
        gxloge ("overlong headers\n");
		if (s->cookie_dict)
			av_dict_free(&s->cookie_dict);
		return AVERROR(EINVAL);
    }

	http_printf(">>>>>>>>>>");
	http_printf(request.str);
	if ((err = url_write(s->hd, (unsigned char*)request.str, request.len)) < 0) {
		if (s->cookie_dict)
			av_dict_free(&s->cookie_dict);
		if (s->buffer) {
			av_freep(&s->buffer);
		}
		return err;
	}
	if (s->post_data) {
		if ((err = url_write(s->hd, s->post_data, s->post_datalen)) < 0) {
			if (s->cookie_dict)
				av_dict_free(&s->cookie_dict);
			if (s->buffer) {
				av_freep(&s->buffer);
			}
			return err;
		}
	}

	/* init input buffer */
	s->buf_ptr = s->buffer;
	s->buf_end = s->buffer;
	s->line_count = 0;
	s->off = 0;
	s->filesize = UINT64_MAX;
	s->willclose = 0;
	s->end_chunked_post = 0;
	s->end_header = 0;
	s->compressed = 0;
	if (post && !s->post_data) {
		/* Pretend that it did work. We didn't read any header yet, since
		 * we've still to send the POST data, but the code calling this
		 * function will check http_code after we return. */
		s->http_code = 200;
		return 0;
	}
	s->chunksize = UINT64_MAX;

	/* wait for header */
	err = http_read_header(h, new_location);
	if (err < 0) {
		if (s->cookie_dict)
			av_dict_free(&s->cookie_dict);
		if (s->buffer) {
			av_freep(&s->buffer);
		}
		return err;
	}
	if (*new_location)
		s->off = off;
	return (off == s->off) ? 0 : -1;
}


static int http_buf_read(URLContext *h, uint8_t *buf, int size)
{
	HTTPContext *s = h->priv_data;
	int len = 0;

	if (s->chunksize != UINT64_MAX) {
		if (s->chunkend) {
			gxlogd ("http.c...read fail fail fail...\n");
			return AVERROR_EOF;
		}
		if (!s->chunksize) {
			char line[32];
			int err;
			do {
				if ((err = http_get_line(s, line, sizeof(line))) < 0) {
					return err;
				}
			} while (!*line);	 /* skip CR LF from last chunk */
			s->chunksize = strtoull(line, NULL, 16);
			if (!s->chunksize && s->multiple_requests) {
				http_get_line(s, line, sizeof(line)); // read empty chunk
				s->chunkend = 1;
				return 0;
			} else if (!s->chunksize) {
				gxlogd ("Last chunk received, closing conn.line:[%s]\n", line);
				url_closep(&s->hd);
				return 0;
			} else if (s->chunksize == UINT64_MAX) {
				gxlogd ("Invalid chunk size %lld.\n", s->chunksize);
				return AVERROR(EINVAL);
			}
		}
		size = FFMIN(size, s->chunksize);
	}

	/* read bytes from input buffer first */
	len = s->buf_end - s->buf_ptr;
	if (len > 0) {
		if (len > size)
			len = size;
		memcpy(buf, s->buf_ptr, len);
		s->buf_ptr += len;
	} else {
		uint64_t target_end = s->end_off ? s->end_off : s->filesize;
		if ((!s->willclose || s->chunksize == UINT64_MAX) && s->off >= target_end) {
			return AVERROR_EOF;
		}
		len = url_read(s->hd, buf, size);
		if ((!len || len == AVERROR_EOF) && (!s->willclose || s->chunksize == UINT64_MAX) && (s->off < target_end)) {
			gxlogi_raw_l ("Stream ends prematurely at %lld,should be %lld.len:%d(%s).wilclose:%d.chunksize:%lld.\n", 
				s->off, target_end, len, av_err2str(len), s->willclose, s->chunksize);
			return AVERROR(EIO);
		}

	}
	if (len > 0) {
		s->off += len;
		if (s->chunksize > 0  && s->chunksize != UINT64_MAX)
			s->chunksize -= len;
	}
	return len;
}

#define DECOMPRESS_BUF_SIZE (128 * 1024)
static int http_buf_read_compressed(URLContext *h, uint8_t *buf, int size)
{
    HTTPContext *s = h->priv_data;
    int ret;

    if (!s->inflate_buffer) {
        s->inflate_buffer = av_malloc(DECOMPRESS_BUF_SIZE);
        if (!s->inflate_buffer)
            return AVERROR(ENOMEM);
    }

    if (s->inflate_stream.avail_in == 0) {
        int read = http_buf_read(h, s->inflate_buffer, DECOMPRESS_BUF_SIZE);
        if (read <= 0)
            return read;
        s->inflate_stream.next_in  = s->inflate_buffer;
        s->inflate_stream.avail_in = read;
    }

    s->inflate_stream.avail_out = size;
    s->inflate_stream.next_out  = buf;

    ret = inflate(&s->inflate_stream, Z_SYNC_FLUSH);
    if (ret != Z_OK && ret != Z_STREAM_END) {
        gxlogd("inflate return value:%d,%s\n", ret, s->inflate_stream.msg);
    }

    return size - s->inflate_stream.avail_out;
}

static int64_t http_seek_internal(URLContext *h, int64_t off, int whence, int force_reconnect);
static int http_read(URLContext *h, uint8_t *buf, int size)
{
	HTTPContext *s = h->priv_data;
	int err, new_location, read_ret = 0;

	if (!s->hd) {
		return AVERROR_EOF;
	}

	if (s->end_chunked_post) {
		if (!s->end_header) {
			err = http_read_header(h, &new_location);
			if (err < 0) {
				if (s->cookie_dict)
					av_dict_free(&s->cookie_dict);
				return err;
			}
		}
		return http_buf_read(h, buf, size);
	}

	if (s->compressed) {
 		return http_buf_read_compressed(h, buf, size);
	}
	read_ret = http_buf_read(h, buf, size);
	if (read_ret < 0) {
		uint64_t target = h->is_streamed ? 0 : s->off;
		int64_t seek_ret;
		if (!s->is_reconnect || http_is_reconnect(read_ret)) {
			return read_ret;
		}
#if 0
		if ((read_ret == AVERROR(EIO)) && s->is_reconnect && s->willclose) {
			return read_ret;
		}
#endif
		if (!s->is_live && !h->is_streamed) {
			/*stream is vod.*/
			seek_ret = http_seek_internal(h, target, SEEK_SET, 1);
			if (seek_ret >= 0 && seek_ret != target) {
				gxlogd ("Failed to reconnect at target=%lld.\n", target);
				return read_ret;
			}
		} else {
			/*stream is live.*/
			s->off = 0;
			/* if the location changed (redirect), revert to the original uri */
			if (strcmp(s->uri, s->location)) {
				if (s->location) {
					av_free(s->location);
					s->location = NULL;
				}
				s->location = av_strdup(s->uri);
				if (!s->location)
					return AVERROR(ENOMEM);
			}
			if (s->hd) {
				url_close(s->hd);
				s->hd = NULL;
			}
			if ((read_ret = http_open_cnx(h)) < 0) {
				return read_ret;
			}
		}
		read_ret = http_buf_read(h, buf, size);
	}
#if STREAM_HTTP_PVR_DEBUG
	if (read_ret > 0 && s->download_stream_fp)
		fwrite(buf, read_ret, 1, s->download_stream_fp);
#endif
	return read_ret;
}

/* used only when posting data */
static int http_write(URLContext *h, uint8_t *buf, int size)
{
	char temp[11] = "";  /* 32-bit hex + CRLF + nul */
	int ret;
	char crlf[] = "\r\n";
	HTTPContext *s = h->priv_data;

	if (!s->chunked_post) {
		/* non-chunked data is sent without any special encoding */
		return url_write(s->hd, (unsigned char*)buf, size);
	}

	/* silently ignore zero-size data since chunk encoding that would
	 * signal EOF */
	if (size > 0) {
		/* upload data using chunked encoding */
		snprintf(temp, sizeof(temp), "%x\r\n", size);

		if ((ret = url_write(s->hd, (unsigned char*)temp, strlen(temp))) < 0 ||
				(ret = url_write(s->hd, (unsigned char*)buf, size)) < 0 ||
				(ret = url_write(s->hd, (unsigned char*)crlf, sizeof(crlf) - 1)) < 0)
			return ret;
	}
	return size;
}

static int http_shutdown(URLContext *h, int flags)
{
	int ret = 0;
	char footer[] = "0\r\n\r\n";
	HTTPContext *s = h->priv_data;

	/* signal end of chunked encoding if used */
	if ((flags != URL_RDONLY) && s->chunked_post) {
		ret = url_write(s->hd, (unsigned char*)footer, sizeof(footer) - 1);
		ret = ret > 0 ? 0 : ret;
		s->end_chunked_post = 1;
	}

	return ret;
}

static int http_close(URLContext *h)
{
	int ret = 0;
	HTTPContext *s = h->priv_data;
#if STREAM_HTTP_PVR_DEBUG
	if (s->download_stream_fp)
		fclose(s->download_stream_fp);
#endif
	inflateEnd(&s->inflate_stream);
	if (s->inflate_buffer) {
		av_freep(&s->inflate_buffer);
	}

	if (!s->end_chunked_post) {
		/* Close the write direction by sending the end of chunked encoding. */
		ret = http_shutdown(h, h->flags);
	}

	if (s->hd)
		url_closep(&s->hd);
	if (s->location) {
		av_free(s->location);
		s->location = NULL;
	}
	if (s->uri) {
		av_free(s->uri);
		s->uri = NULL;
	}
	if (s->cookie_dict)
		av_dict_free(&s->cookie_dict);
	if (s->http_version) {
		av_free(s->http_version);
		s->http_version = NULL;
	}
	if (s->cookies) {
		av_free(s->cookies);
		s->cookies= NULL;
	}
	if (s->buffer) {
		av_free(s->buffer);
		s->buffer = NULL;
	}
	return ret;
}

static int64_t http_seek_internal(URLContext *h, int64_t off, int whence, int force_reconnect)
{
	HTTPContext *s = h->priv_data;
	URLContext *old_hd = s->hd;
	uint64_t old_off = s->off;
	uint8_t *old_buf;
	int old_buf_size, ret = 0, buf_size = 0;

	if (whence == AVSEEK_SIZE) {
		return s->filesize;
	} else if (!force_reconnect &&
		((whence == SEEK_CUR && off == 0) || (whence == SEEK_SET && off == s->off))) {
		return s->off;
	} else if ((s->filesize == UINT64_MAX && whence == SEEK_END) || h->is_streamed) {
		if (h->is_streamed)
			return -1;
		else
			return AVERROR(ENOSYS);
	}

	if (whence == SEEK_CUR)
		off += s->off;
	else if (whence == SEEK_END)
		off += s->filesize;
	else if (whence != SEEK_SET)
		return AVERROR(EINVAL);
	if (off < 0)
		return AVERROR(EINVAL);
	s->off = off;
	/* do not try to make a new connection if seeking past the end of the file */
	if (s->end_off || s->filesize != UINT64_MAX) {
		uint64_t end_pos = s->end_off ? s->end_off : s->filesize;
		if (s->off >= end_pos) {
			return s->off;
		}
	}

	/* if the location changed (redirect), revert to the original uri */
	if (strcmp(s->uri, s->location)) {
		if (s->location) {
			av_free(s->location);
			s->location = NULL;
		}
		s->location = av_strdup(s->uri);
		if (!s->location)
			return AVERROR(ENOMEM);
	}
	/* we save the old context in case the seek fails */
	old_buf_size = s->buf_end - s->buf_ptr;
	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &buf_size);
	buf_size = (old_buf_size <= 0)?(buf_size+HTTP_HEADERS_MAX_SIZE):old_buf_size;
	old_buf = av_mallocz(buf_size*sizeof(uint8_t)+1);
	if (!old_buf) {
		if (s->hd) {
			url_close(s->hd);
			s->hd = NULL;
		}
		return AVERROR(ENOMEM);
	}
	memcpy(old_buf, s->buf_ptr, old_buf_size);
	s->hd = NULL;
	/* if it fails, continue on old connection */
	if ((ret = http_open_cnx(h)) < 0) {
		gxloge_raw_l ("http seek error::%d(%s).err string:(%s).\n", ret, av_fourcc2str(abs(ret)), av_err2str(ret));
		if (s->buffer)
			memcpy(s->buffer, old_buf, old_buf_size);
		s->buf_ptr = s->buffer;
		s->buf_end = s->buffer + old_buf_size;
		s->hd      = old_hd;
		s->off     = old_off;
		if (old_buf) {
			av_free(old_buf);
			old_buf = NULL;
		}
		return ret;
	}
	if (old_hd) {
		url_close(old_hd);
		old_hd = NULL;
	}
 	if (old_buf) {
		av_free(old_buf);
		old_buf = NULL;
	}
	return off;
}

static int64_t http_seek(URLContext *h, int64_t off, int whence)
{
	return http_seek_internal(h, off, whence, 0);
}

static int http_get_file_handle(URLContext *h)
{
	HTTPContext *s = h->priv_data;
	return url_get_file_handle(s->hd);
}

URLProtocol gx_http_protocol = {
	.name                = "http",
	.url_open            = http_open,
	.url_read            = http_read,
	.url_write           = http_write,
	.url_seek            = http_seek,
	.url_close           = http_close,
	.url_get_file_handle = http_get_file_handle,
	.url_shutdown        = http_shutdown,
	.priv_data_size      = sizeof(HTTPContext),
};

URLProtocol gx_https_protocol = {
	.name                = "https",
	.url_open            = http_open,
	.url_read            = http_read,
	.url_write           = http_write,
	.url_seek            = http_seek,
	.url_close           = http_close,
	.url_get_file_handle = http_get_file_handle,
	.url_shutdown        = http_shutdown,
	.priv_data_size      = sizeof(HTTPContext),
};

static int http_proxy_close(URLContext *h)
{
	HTTPContext *s = h->priv_data;
	if (s->hd)
		url_closep(&s->hd);
	return 0;
}

static int http_proxy_open(URLContext *h, const char *uri, int flags)
{
	HTTPContext *s = h->priv_data;
	char *path = NULL, *authstr = NULL;
	int port = 0, ret = 0, attempts = 0, new_loc = 0, def_max_url_size = 0;
	HTTPAuthType cur_auth_type = HTTP_AUTH_NONE;
	http_proxy_url_param *url_param = NULL;
	AVBPrint request;
	GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);

	if (!uri) {
		ret = AVERROR(EINVAL);
		goto fail2;
	}

	if (!(s->buffer = av_mallocz((def_max_url_size+HTTP_HEADERS_MAX_SIZE)))) {
		ret = AVERROR(ENOMEM);
		goto fail2;
	}
	av_bprint_init_for_buffer(&request, (char *)s->buffer, def_max_url_size+HTTP_HEADERS_MAX_SIZE);

	if (!(url_param = av_mallocz(sizeof(http_open_url_param)))) {
		ret = AVERROR(ENOMEM);
		goto fail2;
	}

	h->is_streamed = 1;
	av_url_split(NULL, 0, url_param->auth, sizeof(url_param->auth)-1, url_param->hostname, sizeof(url_param->hostname)-1, &port,
			url_param->pathbuf, sizeof(url_param->pathbuf)-1, uri);

	ff_url_join(url_param->hoststr, HTTP_OFFSET-1, NULL, NULL, url_param->hostname, port, NULL);

	path = url_param->pathbuf;
	if (*path == '/')
		path++;

	ff_url_join(url_param->lower_url, sizeof(url_param->lower_url)-1, "tcp", NULL, url_param->hostname, port, NULL);
redo:
	ret = url_open(&s->hd, url_param->lower_url, URL_RDWR, &h->url_context_options);
	if (ret < 0) {
		goto fail2;
	}
	authstr = ff_http_auth_create_response(&s->proxy_auth_state, url_param->auth, path, "CONNECT");
	av_bprintf(&request, "CONNECT %s HTTP/1.1\r\n"
			"Host: %s\r\n"
			"Connection: close\r\n"
			"%s%s"
			"\r\n",
			path, url_param->hoststr,
			authstr ? "Proxy-" : "", authstr ? authstr : "");
	if (authstr) {
		av_freep(&authstr);
	}
	http_printf(">>>>>>>>>>");
	http_printf(request.str);
	if ((ret = url_write(s->hd, (unsigned char*)request.str, request.len)) < 0)
		goto fail;

	s->buf_ptr = s->buffer;
	s->buf_end = s->buffer;
	s->line_count = 0;
	s->filesize = UINT64_MAX;
	cur_auth_type = s->proxy_auth_state.auth_type;

	/* Note: This uses buffering, potentially reading more than the
	 * HTTP header. If tunneling a protocol where the server starts
	 * the conversation, we might buffer part of that here, too.
	 * Reading that requires using the proper url_read() function
	 * on this URLContext, not using the fd directly (as the tls
	 * protocol does). This shouldn't be an issue for tls though,
	 * since the client starts the conversation there, so there
	 * is no extra data that we might buffer up here.
	 */
	ret = http_read_header(h, &new_loc);
	if (ret < 0) {
		if (s->cookie_dict)
			av_dict_free(&s->cookie_dict);
		goto fail;
	}

	attempts++;
	if (s->http_code == 407 &&
			(cur_auth_type == HTTP_AUTH_NONE || s->proxy_auth_state.stale) &&
			s->proxy_auth_state.auth_type != HTTP_AUTH_NONE && attempts < 2) {
		url_closep(&s->hd);
		goto redo;
	}

	if (s->http_code < 400) {
		ret = 0;
		goto fail2;
	}
	ret = ff_http_averror(s->http_code, AVERROR(EIO));
fail:
	http_proxy_close(h);
fail2:
	if (url_param) {
		av_free(url_param);
		url_param = NULL;
	}
	if (s->buffer) {
		av_free(s->buffer);
		s->buffer = NULL;
	}
	return ret;
}

static int http_proxy_write(URLContext *h, uint8_t *buf, int size)
{
	HTTPContext *s = h->priv_data;
	return url_write(s->hd, buf, size);
}

URLProtocol gx_httpproxy_protocol = {
	.name                = "httpproxy",
	.url_open            = http_proxy_open,
	.url_read            = http_buf_read,
	.url_write           = http_proxy_write,
	.url_close           = http_proxy_close,
	.url_get_file_handle = http_get_file_handle,
	.priv_data_size      = sizeof(HTTPContext),
};
