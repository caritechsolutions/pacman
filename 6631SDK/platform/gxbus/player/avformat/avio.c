/*
 *  Unbuffered io for ffmpeg system
 *  Copyright (c) 2001 Fabrice Bellard
 *
 *  This file is part of FFmpeg.
 *
 *  FFmpeg is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  FFmpeg is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with FFmpeg; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "avformat.h"
#include "network.h"
#include "gx_stream.h"
#include "../avutil/opt.h"

#define URL_SCHEME_CHARS                        \
    "abcdefghijklmnopqrstuvwxyz"                \
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"                \
    "0123456789+-."


URLProtocol* first_protocol = NULL;

URLProtocol *ffurl_protocol_next(URLProtocol *prev)
{
    return prev ? prev->next : first_protocol;
}

const char *avio_enum_protocols(void **opaque, int output)
{
    URLProtocol **p = (URLProtocol **)opaque;
    *p = ffurl_protocol_next(*p);
    if (!*p) return NULL;
    if ((output && (*p)->url_write) || (!output && (*p)->url_read))
        return (*p)->name;
    return avio_enum_protocols(opaque, output);
}

int register_protocol(URLProtocol*  protocol)
{
	URLProtocol* *p;
	p = &first_protocol;
	while (*p != NULL)
		p = &(*p)->next;
	*p = protocol;
	protocol->next = NULL;
	return 0;
}

static int url_alloc_for_protocol (URLContext **puc, struct URLProtocol *up,
                                   const char *filename, int flags)
{
    URLContext *uc;
    int err;

    uc = av_mallocz(sizeof(URLContext) + strlen(filename) + 1);
    if (!uc) {
        err = AVERROR(ENOMEM);
        goto fail;
    }
    uc->filename = (char *) &uc[1];
    av_strlcpy(uc->filename, filename, strlen(filename)+1);
    uc->prot = up;
    uc->flags = flags;
    uc->is_streamed = 0; /* default = not streamed */
    uc->max_packet_size = 0; /* default: stream file */
    uc->debug_r_cost_time = 0;
    uc->debug_r_packet_size = 0;
    if (up->priv_data_size) {
        uc->priv_data = av_mallocz(up->priv_data_size);
            if (!uc->priv_data) {
                av_free(uc);
                err = AVERROR(ENOMEM);
                goto fail;
        }
    }

    *puc = uc;
    return 0;
 fail:
    *puc = NULL;
    return err;
}

int url_connect(URLContext* uc)
{
	int err =
		uc->prot->url_open?uc->prot->url_open(uc, uc->filename, uc->flags):AVERROR(EIO);
	if (err)
		return err;
	uc->is_connected = 1;
	//We must be careful here as ffurl_seek() could be slow, for example for http
	if( (uc->flags != URL_RDONLY)
			|| !strcmp(uc->prot->name, "file"))
		if(!uc->is_streamed && url_seek(uc, 0, SEEK_SET) < 0)
			uc->is_streamed= 1;
	return 0;
}

int url_alloc(URLContext* * puc, const char *filename, int flags)
{
    URLProtocol *up = NULL;
    char proto_str[128]="", proto_nested[128]="", *ptr;
    size_t proto_len = strspn(filename, URL_SCHEME_CHARS);

    if (!first_protocol) {
        gxlogd ("No URL Protocols are registered.\n");
    }

    if (filename[proto_len] != ':' &&  filename[proto_len] != ',')
        av_strlcpy(proto_str, "file", FFMIN(sizeof("file"), sizeof(proto_str)));
    else
        av_strlcpy(proto_str, filename, FFMIN(proto_len+1, sizeof(proto_str)));

    if ((ptr = strchr(proto_str, ',')))
        *ptr = '\0';
    av_strlcpy(proto_nested, proto_str, sizeof(proto_nested));
    if ((ptr = strchr(proto_nested, '+')))
        *ptr = '\0';

    while ((up = ffurl_protocol_next(up))) {
        if (!strcmp(proto_nested, up->name)) {
            return url_alloc_for_protocol (puc, up, filename, flags);
        }
        if (url_check_interrupt_cb())
            return AVERROR_EXIT;
    }
    *puc = NULL;
    gxloge("not find support stream media protocol.\n");
    return AVERROR_PROTOCOL_NOT_FOUND;
}

int url_open(URLContext* * puc, const char *filename, int flags, AVDictionary **options)
{
    int ret = -1;
    ret = url_alloc(puc, filename, flags);

    if (ret)
        return ret;

    if (puc && *puc && *options) {
        av_dict_copy(&(*puc)->url_context_options, *options, 0);
    }
    ret = url_connect(*puc);
    if (!ret)
        return 0;

    if (*puc) {
        url_close(*puc);
        *puc = NULL;
    }
    return ret;
}

static inline int retry_transfer_wrapper(URLContext *h, unsigned char *buf, int size, int size_min,
		int (*transfer_func)(URLContext *h, unsigned char *buf, int size))
{
	int ret, len = 0;
	/*Patch Start*/
	int fast_retries = 200;/*default:20S.*/
	AVDictionaryEntry *e = NULL;
	if ((e = av_dict_get(h->url_context_options, "rw_timeout", NULL, 0))) {
		fast_retries = AV_MAX((atoi(e->value)*10), 10);/*timeout min:1s.*/
	}
	/*Patch End*/
	while (len < size_min) {
		if (url_check_interrupt_cb())
			return AVERROR_EXIT;
		ret = transfer_func(h, buf+len, size-len);
		if (ret == AVERROR(EINTR))
			continue;
		if (h->flags & AVIO_FLAG_NONBLOCK)
			return ret;
		if (ret == AVERROR(EAGAIN)) {
			ret = 0;
			fast_retries -= 1;
		/*Patch Start*/
			if (fast_retries <= 0) {
				gxlogi_raw_l ("retry 200*100ms. transfer fail. return EIO..\n");
				return AVERROR(ETIMEDOUT);
			}
		/*Patch End*/
		} else if (ret == AVERROR_EOF) {
			return (len > 0) ? len : AVERROR_EOF;
		} else if (ret < 1) {
			return ret < 0 ? ret : len;
		}
		len += ret;
	}
	return len;
}

int url_read(URLContext*  h, unsigned char *buf, int size)
{
	int ret = 0;
	if (!h || !(h->flags & AVIO_FLAG_READ)) {
		return AVERROR(EIO);
	}
	unsigned int debug_stream_speed = 0;
	unsigned int debug_cost_ms = 0;
	GxPlayer_SystemGet(PSYS_DEBUG_STREAM_SPEED, &debug_stream_speed);
	if (debug_stream_speed)
		debug_cost_ms = av_get_tick_ms(NULL, 0);

	ret = retry_transfer_wrapper(h, buf, size, 1, h->prot->url_read);
	if (debug_stream_speed && ret > 0) {
		debug_cost_ms = (av_get_tick_ms(NULL, 0) - debug_cost_ms);
		h->debug_r_cost_time += debug_cost_ms;
		h->debug_r_packet_size += ret;
		if (h->debug_r_cost_time > 2*DEBUG_STREAM_TIME_MS) {
			gxloge("avio.c read data:: %7.2f KB/s\n", ((double)h->debug_r_packet_size/1024.0*1000)/h->debug_r_cost_time);
			h->debug_r_cost_time = 0;
			h->debug_r_packet_size = 0;
		}
	}
	return ret;
}

int url_read_complete(URLContext*  h, unsigned char *buf, int size)
{
	int ret = 0;
	if (!h || !(h->flags & AVIO_FLAG_READ))
		return AVERROR(EIO);
	unsigned int debug_stream_speed = 0;
	unsigned int debug_cost_ms = 0;
	GxPlayer_SystemGet(PSYS_DEBUG_STREAM_SPEED, &debug_stream_speed);
	if (debug_stream_speed)
		debug_cost_ms = av_get_tick_ms(NULL, 0);

	ret = retry_transfer_wrapper(h, buf, size, size, h->prot->url_read);
	if (debug_stream_speed && ret > 0) {
		debug_cost_ms = (av_get_tick_ms(NULL, 0) - debug_cost_ms);
		h->debug_r_cost_time += debug_cost_ms;
		h->debug_r_packet_size += ret;
		if (h->debug_r_cost_time > 2*DEBUG_STREAM_TIME_MS) {
			gxloge("avio.c read data:: %7.2f KB/s\n", ((double)h->debug_r_packet_size/1024.0*1000)/h->debug_r_cost_time);
			h->debug_r_cost_time = 0;
			h->debug_r_packet_size = 0;
		}
	}

	return ret;
}

int url_write(URLContext*  h, unsigned char *buf, int size)
{
	if (!h || !(h->flags & AVIO_FLAG_WRITE))
		return AVERROR(EIO);
	/* avoid sending too big packets */
	if (h->max_packet_size && size > h->max_packet_size)
		return AVERROR(EIO);

	return retry_transfer_wrapper(h, buf, size, size, (void*)h->prot->url_write);
}

int64_t url_size(URLContext *h)
{
    int64_t pos, size;
    if (!h)
        return AVERROR(EIO);

    size = url_seek(h, 0, AVSEEK_SIZE);
    if (size < 0) {
        pos = url_seek(h, 0, SEEK_CUR);
        if ((size = url_seek(h, -1, SEEK_END)) < 0)
            return size;
        size++;
        url_seek(h, pos, SEEK_SET);
    }
    return size;
}

offset_t url_seek(URLContext*  h, offset_t pos, int whence)
{
	offset_t ret;
	if (!h)
		return AVERROR(EIO);

	if (!h->prot->url_seek)
		return AVERROR(ENOSYS);
	ret = h->prot->url_seek(h, pos, whence & ~AVSEEK_FORCE);
	return ret;
}

int url_closep(URLContext **hh)
{
	URLContext *h= *hh;
	int ret = 0;
	if (!h) return 0; /* can happen when ffurl_open fails */

	if (h->is_connected && h->prot->url_close)
		ret = h->prot->url_close(h);
	if (h->prot->priv_data_size) {
		av_freep(&h->priv_data);
    }
	if (h->url_context_options) {
		av_dict_free(&h->url_context_options);
		h->url_context_options = NULL;
	}
	av_freep(hh);
	return ret;
}

int url_close(URLContext*  h)
{
	return url_closep(&h);
}

int url_exist(const char* filename)
{
	URLContext* h;
	if (url_open(&h, filename, URL_RDONLY, NULL) < 0)
		return 0;
	url_close(h);
	return 1;
}

offset_t url_filesize(URLContext*  h)
{
	offset_t pos, size;

	size = url_seek(h, 0, AVSEEK_SIZE);
	if (size < 0) {
		pos = url_seek(h, 0, SEEK_CUR);
		if ((size = url_seek(h, -1, SEEK_END)) < 0)
			return size;
		size++;
		url_seek(h, pos, SEEK_SET);
	}
	return size;
}

int url_get_file_handle(URLContext *h)
{
	if (!h || !h->prot || !h->prot->url_get_file_handle) {
		return -1;
	}
	return h->prot->url_get_file_handle(h);
}

int url_get_max_packet_size(URLContext*  h)
{
	return h->max_packet_size;
}

void url_get_filename(URLContext*  h, char *buf, int buf_size)
{
	av_strlcpy(buf, h->filename, buf_size);
}

int url_check_interrupt_cb(void)
{
	extern PLAYER_INTERRUPT_CBK lavf_interruptcbk;
	if(lavf_interruptcbk && lavf_interruptcbk())
		return 1;
	return 0;
}

URLProtocol **ffurl_get_protocols(const char *whitelist,
                                        const char *blacklist)
{
	return NULL;
}

static inline int is_dos_path(char *path)
{
    if (path[0] && path[1] == ':')
        return 1;
    return 0;
}

char *avio_find_protocol_name(char *url)
{
	char *pProtocol = NULL, *pBuff=NULL;
	int proto_len = strspn(url, URL_SCHEME_CHARS);
	pBuff = strstr(url, "://");
	if (!pBuff) {
		if ((((url[proto_len] != ':') && (strncmp(url, "subfile,", 8))) || (!strchr(url + proto_len + 1, ':'))) || is_dos_path(url)) {
			pProtocol = av_strdup("file");
			return pProtocol;
		}
		return NULL;
	}

	pProtocol = av_malloc (pBuff-url+1);
	if (!pProtocol)
		return NULL;
	memset (pProtocol, 0, pBuff-url+1);
	memcpy (pProtocol, url, pBuff-url);
	return pProtocol;
}

int av_match_list(const char *name, const char *list, char separator)
{
    const char *p, *q;

    for (p = name; p && *p; ) {
        for (q = list; q && *q; ) {
            int k;
            for (k = 0; p[k] == q[k] || (p[k]*q[k] == 0 && p[k]+q[k] == separator); k++)
                if (k && (!p[k] || p[k] == separator))
                    return 1;
            q = strchr(q, separator);
            q += !!q;
        }
        p = strchr(p, separator);
        p += !!p;
    }

    return 0;
}

int ffurl_connect(URLContext *uc, AVDictionary **options)
{
    int err = 0;
    AVDictionary *tmp_opts = NULL;

    if (!options)
        options = &tmp_opts;
    if (*options)
        av_dict_copy(&uc->url_context_options, *options, 0);
	err = uc->prot->url_open(uc, uc->filename, uc->flags);
    if (err)
        return err;
    uc->is_connected = 1;
    /* We must be careful here as ffurl_seek() could be slow,
     * for example for http */
    if ((uc->flags & AVIO_FLAG_WRITE) || !strcmp(uc->prot->name, "file")) {
        if (!uc->is_streamed && url_seek(uc, 0, SEEK_SET) < 0) {
            uc->is_streamed = 1;
        }
    }
    return 0;
}

int ffurl_open_whitelist(URLContext **puc, const char *filename, int flags, AVDictionary **options,
                         const char *whitelist, const char* blacklist, URLContext *parent)
{
    AVDictionary *tmp_opts = NULL;
    AVDictionaryEntry *e = NULL;
    int ret = url_alloc(puc, filename, flags);
    if (ret < 0)
        return ret;
    if (parent) {
        ret = av_opt_copy(*puc, parent);
        if (ret < 0)
            goto fail;
    }
    if (options &&
        (ret = av_opt_set_dict(*puc, options)) < 0)
        goto fail;
    if (!options)
        options = &tmp_opts;

    ret = ffurl_connect(*puc, options);

    if (!ret)
        return 0;
fail:
    //ffurl_close(*puc);
    url_close(*puc);
    *puc = NULL;
    return ret;
}

int ffurl_close(URLContext *h)
{
    return url_closep(&h);
}

int ffurl_get_multi_file_handle(URLContext *h, int **handles, int *numhandles)
{
    if (!h || !h->prot)
        return AVERROR_UNKNOWN;
    if (!h->prot->url_get_multi_file_handle) {
        if (!h->prot->url_get_file_handle)
            return AVERROR_UNKNOWN;
        *handles = av_malloc(sizeof(**handles));
        if (!*handles)
            return AVERROR(ENOMEM);
        *numhandles = 1;
        *handles[0] = h->prot->url_get_file_handle(h);
        return 0;
    }
    return h->prot->url_get_multi_file_handle(h, handles, numhandles);
}


