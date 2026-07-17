#include "gx_demux.h"
#include "gx_options.h"
#include "http.h"
#include "cookie.h"
#include "gx_url.h"
#include "tcp.h"
#include "avstring.h"

#ifdef CONFIG_STREAM_HTTP
/* Variables for the command line option -user, -passwd, -bandwidth,
   -user-agent and -nocookies*/

char* network_username = NULL;
char* network_password = NULL;
int network_bandwidth = 0;

/* IPv6 options*/
int network_ipv4_only_proxy = 0;

GxMimeStruct GxMime_Type_Table[] = {
	// MP3 streaming, some MP3 streaming server answer with audio/mpeg
	{"audio/mpeg", GX_DEMUXER_TYPE_AUDIO},
	// MPEG streaming
	{"video/mpeg", GX_DEMUXER_TYPE_UNKNOWN},
	{"video/x-mpeg", GX_DEMUXER_TYPE_UNKNOWN},
	{"video/x-mpeg2", GX_DEMUXER_TYPE_UNKNOWN},
	// AVI ??? => video/x-msvideo
	{"video/x-msvideo", GX_DEMUXER_TYPE_LAVF},
	// MOV => video/quicktime
	{"video/quicktime", GX_DEMUXER_TYPE_LAVF},
	// ASF
	{"audio/x-ms-wax", GX_DEMUXER_TYPE_ASF},
	{"audio/x-ms-wma", GX_DEMUXER_TYPE_ASF},
	{"video/x-ms-asf", GX_DEMUXER_TYPE_ASF},
	{"video/x-ms-afs", GX_DEMUXER_TYPE_ASF},
	{"video/x-ms-wmv", GX_DEMUXER_TYPE_ASF},
	{"video/x-ms-wma", GX_DEMUXER_TYPE_ASF},
	{"application/x-mms-framed", GX_DEMUXER_TYPE_ASF},
	{"application/vnd.ms.wms-hdr.asfv1", GX_DEMUXER_TYPE_ASF},
	{"application/octet-stream", GX_DEMUXER_TYPE_UNKNOWN},
	// Playlists
	{"video/x-ms-wmx", GX_DEMUXER_TYPE_PLAYLIST},
	{"video/x-ms-wvx", GX_DEMUXER_TYPE_PLAYLIST},
	{"audio/x-scpls", GX_DEMUXER_TYPE_PLAYLIST},
	{"audio/x-mpegurl", GX_DEMUXER_TYPE_MPEG_TS},
	{"audio/x-pls", GX_DEMUXER_TYPE_PLAYLIST},
	// Real Media
	{"audio/x-pn-realaudio", GX_DEMUXER_TYPE_REAL},
	// OGG Streaming
	{ "application/ogg", GX_DEMUXER_TYPE_OGG },
	{"application/x-ogg", GX_DEMUXER_TYPE_OGG},
	// NullSoft Streaming Video
	{"video/nsv", GX_DEMUXER_TYPE_NSV},
	{"misc/ultravox", GX_DEMUXER_TYPE_NSV},

	// Flash Video
	{"video/flv", GX_DEMUXER_TYPE_LAVF},
	{"video/x-flv", GX_DEMUXER_TYPE_UNKNOWN},

	{"video/mp4", GX_DEMUXER_TYPE_LAVF},
	{"video/MP2T", GX_DEMUXER_TYPE_MPEG_TS},
	{"audio/x-aac", GX_DEMUXER_TYPE_AAC},
	{NULL, GX_DEMUXER_TYPE_UNKNOWN},
};

#include <pthread.h>
#ifdef CONFIG_DEMUX_TLS
#ifdef CONFIG_HAVE_WOLFSSL
#include <cyassl/openssl/bio.h>
#include <cyassl/openssl/ssl.h>
#include <cyassl/openssl/err.h>
#include <cyassl/openssl/crypto.h>
#elif defined(CONFIG_HAVE_OPENSSL)
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#endif
#include "../avformat/gx_poll.h"
static int openssl_init;
#endif

void https_init(void)
{
#ifdef CONFIG_DEMUX_TLS
	if (!openssl_init) {
		SSL_library_init();
		SSL_load_error_strings();
	}
	openssl_init = 1;
#endif
	return;
}

void https_deinit(void)
{
	return;
}

#ifdef CONFIG_DEMUX_TLS
static int https_select (int sock, unsigned int timeout_ms, int type)
{
	int ret = 0;
	fd_set data_set;
	fd_set expt_set;
	struct timeval timeout;

	if (timeout_ms != 0) {
		FD_ZERO(&data_set);
		FD_ZERO(&expt_set);
		FD_SET(sock, &expt_set);
		FD_SET(sock, &data_set);
		timeout.tv_sec = (unsigned int)(((long)timeout_ms)/(long)1000);
		timeout.tv_usec = (unsigned int)((long)timeout_ms % 1000) * (long)1000;
	} else {
		return 1;
	}

	if (type == POLLIN)
		ret = select(sock+1, &data_set, NULL, &expt_set, &timeout);

	if (type == POLLOUT)
		ret = select(sock+1, NULL, &data_set, &expt_set, &timeout);

	if (ret > 0) {
		if (FD_ISSET(sock, &expt_set)) {
			gxlogd("%s %d errno %d....\n", __FUNCTION__, __LINE__, errno);
			return -1;
		}

		if (FD_ISSET(sock, &data_set))
			return ret;

		return 0;
	}

	return ret;
}

static int https_poll(GxStreamHttps* c, int ret)
{
	struct pollfd p = { c->fd, 0, 0 };
	char error_buf[1024];
	int retry = 100;

	ret = SSL_get_error(c->ssl, ret);
	if (ret == SSL_ERROR_WANT_READ) {
		p.events = POLLIN;
	} else if (ret == SSL_ERROR_WANT_WRITE) {
		p.events = POLLOUT;
	} else {
		gxlogd ("[%s-%d] ret = %d, %s\n",__FUNCTION__, __LINE__, ret, ERR_error_string(ret, error_buf));
		c->https_connect   = 0;/*SSL connect fail.set https_connect=1*/
		return -1;
	}

	while (retry) {
		int n = poll(&p, 1, 100);
		if (n == -1)
			return -1;
		else if (n > 0)
			break;
		else if (n == 0)
			retry --;

		if (c->interrupt_cbk && c->interrupt_cbk())
			return -1;
	}

	return ((retry>0)?0:-1);
}
#endif

int https_open(GxStreamHttps* c, const char *hostname)
{
#ifdef CONFIG_DEMUX_TLS
	char error_buf[1024];

	c->ctx = SSL_CTX_new(SSLv23_client_method());
	if (!c->ctx) {
		gxlogd ("[%s-%d] %s\n",__FUNCTION__, __LINE__, ERR_error_string(ERR_get_error(), error_buf));
		goto fail;
	}
	SSL_CTX_set_options(c->ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
	if (c->ca_file) {
		if (!SSL_CTX_load_verify_locations(c->ctx, c->ca_file, NULL))
			gxlogd ("[%s-%d] %s\n",__FUNCTION__, __LINE__, ERR_error_string(ERR_get_error(), error_buf));
	}
	if (c->cert_file && !SSL_CTX_use_certificate_chain_file(c->ctx, c->cert_file)) {
		gxlogd ("[%s-%d] %s\n",__FUNCTION__, __LINE__, ERR_error_string(ERR_get_error(), error_buf));
		goto fail;
	}

	if (c->key_file && !SSL_CTX_use_PrivateKey_file(c->ctx, c->key_file, SSL_FILETYPE_PEM)) {
		gxlogd ("[%s-%d] %s\n",__FUNCTION__, __LINE__, ERR_error_string(ERR_get_error(), error_buf));
		goto fail;
	}

	SSL_CTX_set_timeout(c->ctx, 10);

	if (c->verify)
		SSL_CTX_set_verify(c->ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
	else
		SSL_CTX_set_verify(c->ctx, SSL_VERIFY_NONE, NULL);

	c->ssl = SSL_new(c->ctx);
	if (!c->ssl) {
		gxlogd ("[%s-%d] %s\n",__FUNCTION__, __LINE__, ERR_error_string(ERR_get_error(), error_buf));
		goto fail;
	}

	SSL_set_tlsext_host_name(c->ssl, hostname);

	{
//		BIO *bio = BIO_new(BIO_f_buffer());
//		SSL_set_bio(c->ssl, bio, bio);
		c->https_connect   = 0;
		c->https_open_init = 1;
	}

	return 0;

fail:
	if (c->ssl)
		SSL_free(c->ssl);
	if (c->ctx)
		SSL_CTX_free(c->ctx);
	c->ssl = NULL;
	c->ctx = NULL;
#endif
	return -1;
}

int https_set_fd(GxStreamHttps* c, int fd)
{
#ifdef CONFIG_DEMUX_TLS
	int ret;

	c->fd = fd;

	if (!SSL_set_fd(c->ssl, c->fd))
		return -1;

    c->https_connect   = 1;
	while (1) {
		ret = SSL_connect(c->ssl);
		if (ret > 0)
			break;

		if ((ret = https_poll(c, ret)) < 0)
			return -1;
	}
	return 0;
#else
	return -1;
#endif
}

void https_shutdown(GxStreamHttps* c)
{
#ifdef CONFIG_DEMUX_TLS
	if (c && c->ssl) {
        if (c->https_connect)
            SSL_shutdown(c->ssl);
    }
#endif
	return;
}

int https_read(GxStreamHttps* c, uint8_t *buf, int size)
{
#ifdef CONFIG_DEMUX_TLS
	int ret = SSL_read(c->ssl, buf, size);
	if (ret > 0)
		return ret;

	if (ret == 0)
		return -1;

	if (ret < 0) {
		int err = SSL_get_error(c->ssl, ret);
		if ((err == SSL_ERROR_WANT_READ) || (err == SSL_ERROR_WANT_WRITE)) {
			if (https_select(c->fd, 100, POLLIN) >= 0)
				return 0;
		}
	}

	return -1;
#else
	return -1;
#endif
}

int https_write(GxStreamHttps* c, uint8_t *buf, int size)
{
#ifdef CONFIG_DEMUX_TLS
	int ret = SSL_write(c->ssl, buf, size);
	if (ret > 0)
		return ret;

	if (ret == 0)
		return -1;

	if (ret < 0) {
		int err = SSL_get_error(c->ssl, ret);
		if ((err == SSL_ERROR_WANT_READ) || (err == SSL_ERROR_WANT_WRITE)) {
			if (https_select(c->fd, 100, POLLOUT) >= 0)
				return 0;
		}
	}

	return -1;
#else
	return -1;
#endif
}

void https_close(GxStreamHttps* c)
{
#ifdef CONFIG_DEMUX_TLS
    if (c) {
        if (c->ssl) {
            if (c->https_connect)
                SSL_shutdown(c->ssl);
            SSL_free(c->ssl);
        }
        if (c->ctx)
            SSL_CTX_free(c->ctx);
        c->ssl = NULL;
        c->ctx = NULL;
        c->https_connect   = 0;
        c->https_open_init = 0;
    }
#endif
	return;
}

static int http_recv(int fd, uint8_t* buffer, int size)
{
	int ret;
	fd_set fds_red;
	struct timeval tv;

	FD_ZERO(&fds_red);
	FD_SET(fd,&fds_red);
	tv.tv_sec = 0;
	tv.tv_usec = 100*1000;
	ret = select(fd+1, &fds_red, NULL, NULL, &tv);
	if (ret == -1) {
		gxlogd ("sock error (%s)\n", strerror(errno));
		return -1;
	}else if (ret > 0) {
		if (FD_ISSET(fd, &fds_red)) {
			ret = recv(fd, buffer, size, 0);
			if (ret < 0) {
				if ((errno != EINTR)&&(errno != EAGAIN)&&(errno != EWOULDBLOCK)) {
					gxlogd ("recv error (%s)\n", strerror(errno));
					return -1;
				}
			}else if (ret == 0) {
				if (errno != EINPROGRESS) {
					gxlogd ("recv over (%d:%s)\n", errno, strerror(errno));
					return -1;
				}
			}else{
				return ret;
			}
		}
	}
	return 0;
}

static int http_write(GxStreamHttp* s, int fd, uint8_t* buffer, int size)
{
	if (s && s->https_protocol)
		return https_write(s->https, buffer, size);
	else
		return send(fd, buffer, size, 0);
}

static int http_read(GxStreamHttp* s, int fd, uint8_t* buffer, int size)
{
	if (s && s->https_protocol)
		return https_read(s->https, buffer, size);
	else
		return http_recv(fd, buffer, size);
}

GxURL* check4proxies(GxURL * url)
{
	GxURL* url_out = NULL;
	if (url == NULL)
		return NULL;
	url_out = url_new(url->url);
	if (!strcasecmp(url->protocol, "http_proxy")) {
		gx_msg(MSGT_NETWORK, MSGL_V, "Using HTTP proxy: http://%s:%d\n", url->hostname, url->port);
		return url_out;
	}
	// Check if the http_proxy environment variable is set.
	if (!strcasecmp(url->protocol, "http")) {
		char* proxy;
		proxy = getenv("http_proxy");
		if (proxy != NULL) {
			// We got a proxy, build the URL to use it
			char* new_url;
			GxURL* tmp_url;
			GxURL* proxy_url = url_new(proxy);

			if (proxy_url == NULL) {
				gx_msg(MSGT_NETWORK, MSGL_WARN, "Invalid proxy setting... Trying without proxy.\n");
				return url_out;
			}

#ifdef HAVE_AF_INET6
			struct sockaddr sa;
			struct addrinfo hints, *res;
			int ret = 0;
			hints.ai_flags = 0;
			hints.ai_family = AF_INET6;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = 0;
			hints.ai_addrlen = 0;
			hints.ai_addr = NULL;
			hints.ai_canonname = NULL;
			hints.ai_next = NULL;
		    ret = getaddrinfo(url->hostname, NULL, &hints, &res);
			if (network_ipv4_only_proxy && ret) {
				gx_msg(MSGT_NETWORK, MSGL_WARN,
						"Could not resolve remote hostname for AF_INET. Trying without proxy.\n");
				url_free(proxy_url);
				return url_out;
			}
#endif

			gx_msg(MSGT_NETWORK, MSGL_V, "Using HTTP proxy: %s\n", proxy_url->url);
			new_url = get_http_proxy_url(proxy_url, url->url);
			if (new_url == NULL) {
				gx_msg(MSGT_NETWORK, MSGL_FATAL, "Memory allocation failed.\n");
				url_free(proxy_url);
				return url_out;
			}
			tmp_url = url_new(new_url);
			if (tmp_url == NULL) {
				av_free(new_url);
				url_free(proxy_url);
				return url_out;
			}
			url_free(url_out);
			url_out = tmp_url;
			av_free(new_url);
			url_free(proxy_url);
		}
	}
	return url_out;
}

int ParseHttpsGetPort(const char *pcUrl)
{
	#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
	const char *p, *ls, *ls2, *at, *at2, *col, *brk;
	int iRetuFlg = 0;

	/* parse protocol */
	if ((p = strchr(pcUrl, ':'))) {
		p++; /* skip ':' */
		if (*p == '/')
			p++;
		if (*p == '/')
			p++;
	}

	/* separate path from hostname */
	ls = strchr(p, '/');
	ls2 = strchr(p, '?');
	if (!ls)
		ls = ls2;
	else if (ls && ls2)
		ls = FFMIN(ls, ls2);
	if (NULL == ls)
		ls = &p[strlen(p)];  // XXX

	/* the rest is hostname, use that to parse auth/port */
	if (ls != p) {
		/* authorization (user[:pass]@hostname) */
		at2 = p;
		while ((at = strchr(p, '@')) && at < ls) {
		p = at + 1; /* skip '@' */
		}

		if (*p == '[' && (brk = strchr(p, ']')) && brk < ls) {
			/* [host]:port */
			if (brk[1] == ':')
				iRetuFlg = 1;
		} else if ((col = strchr(p, ':')) && col < ls)
			iRetuFlg = 1;
	}
	return iRetuFlg;
}

GxHttpHeader* http_build_header(GxStreamHttp* s, GxURL* url1, GxURL* url2, char* cookies, off_t pos, int proxy)
{
	char str[512] = {0};
	char *user_agent  = NULL;
	char *xclientguid = NULL;
	char *connection  = NULL;
	char *referer     = NULL;
	char *user_cookie = NULL;
	GxHttpHeader* http_hdr;

	http_hdr = http_new_header();
	if (http_hdr == NULL)
		return NULL;

	if (proxy)
		http_set_uri(http_hdr, url2->noauth_url );
	else
		http_set_uri(http_hdr, url1->file);

	if (url2->port && url2->port != 80) {
		if (s->https_protocol && 443 == url2->port && 0 == ParseHttpsGetPort(url1->url))
			snprintf (str, sizeof(str), "Host: %s", url2->hostname);
		else
			snprintf (str, sizeof(str), "Host: %s:%d", url2->hostname, url2->port);
	} else {
		snprintf (str, sizeof(str), "Host: %s", url2->hostname);
	}
	http_set_field( http_hdr, str);
	http_set_field(http_hdr, "Accept: */*");

	if ((user_agent = GxOptions_Get_By_Name(((GxStream*)s)->options, " -H", "User-Agent:"))) {
		const char *user_agent_str[2] = {"User-Agent: ", user_agent};
		http_set_fields(http_hdr, user_agent_str, 2);
	} else
		http_set_field(http_hdr, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.131 Safari/537.36");

	if (pos>=0) {
		// Extend http_send_request with possibility to do partial content retrieval
		char *offset = NULL, *end_offset = NULL;
		int len = 0;
		memset (str, 0, sizeof(str));
		if ((offset = GxOptions_Get_By_Name(((GxStream*)s)->options, " -H", "offset:"))) {
			len += av_strlcatf(str + len, sizeof(str) - len, "Range: bytes=%lld-", strtoll(offset, NULL, 10));
		}
		if ((end_offset = GxOptions_Get_By_Name(((GxStream*)s)->options, " -H", "end_offset:"))) {
			len += av_strlcatf(str + len, sizeof(str) - len, "%lld", strtoll(end_offset, NULL, 10) - 1);
		}
		if (len <= 0) {
			memset (str, 0, sizeof(str));
			snprintf (str, sizeof(str), "Range: bytes=%lld-", (int64_t)pos);
		}
		http_set_field(http_hdr, str);
	}

	if ((xclientguid = GxOptions_Get_By_Name(((GxStream*)s)->options, " -H", "xClientGUID:"))) {
		const char *xclientguid_str[3] = {"Pragma: xClientGUID={", xclientguid, "}"};
		http_set_fields(http_hdr, xclientguid_str, 3);
		http_set_field(http_hdr, "X-Accept-Authentication: Negotiate, NTLM, Digest, Basic");
		http_set_field(http_hdr, "Pragma: no-cache,rate=1.000000,stream-time=0,stream-offset=0:0,request-context=1,max-duration=0");
		http_set_field(http_hdr, "Supported: com.microsoft.wm.srvppair, com.microsoft.wm.sswitch, com.microsoft.wm.predstrm");
	}

	if ((connection = GxOptions_Get_By_Name(((GxStream*)s)->options, " -H", "Connection:"))) {
		const char *connection_str[2] = {"Connection: ", connection};
		http_set_fields(http_hdr, connection_str, 2);
	} else
		http_set_field( http_hdr, "Connection: keep-alive");

	//http_set_field(http_hdr, "Accept-Encoding: gzip,deflate,sdch");
	//http_set_field(http_hdr, "Accept-Language: zh-CN,zh;q=0.8");
	//http_set_field(http_hdr, "Accept-Charset: GBK,utf-8;q=0.7,*;q=0.3");
	//http_set_field(http_hdr, "Range: bytes=0-");
	//http_set_field(http_hdr, "Cookie: VISITOR_INFO1_LIVE=gJiiH2tqK1s; PREF=f1=10000000&f4=200000&fv=11.5.31");
	if ((referer = GxOptions_Get_By_Name(((GxStream*)s)->options, " -H", "Referer:"))) {
		const char *referer_str[2] = {"Referer: ", referer};
		http_set_fields(http_hdr, referer_str, 2);
	}
	if (cookies) {
		snprintf (str, sizeof(str), "Cookie: %s", cookies);
		http_set_field( http_hdr, str);
	} else if ((user_cookie = GxOptions_Get_By_Name(((GxStream*)s)->options, " -H", "Set-Cookie:"))) {
		const char *user_cookie_str[2] = {"Cookie: ", user_cookie};
		if (user_cookie && *user_cookie) {
			http_set_fields(http_hdr, user_cookie_str, 2);
		}
	}
	cookies_set( http_hdr, url2->hostname, url2->url );
	if (proxy)
		http_add_basic_proxy_authentication(http_hdr, url1->username, url1->password);
	http_add_basic_authentication(http_hdr, url2->username, url2->password);
	return http_hdr;
}

int http_send_request(GxStreamHttp* s, GxURL*  url, off_t pos, PLAYER_INTERRUPT_CBK interrupt_cbk)
{
	GxURL* server_url;
	GxHttpHeader* http_hdr = NULL;
	int fd = -1;
	int ret;
	int proxy = 0;		// Boolean

	if (!strcasecmp(url->protocol, "http_proxy")) {
		proxy = 1;
		server_url = url_new((url->file) + 1);
		if (!server_url) {
			gxlogf("Invalid URL '%s' to proxify\n", url->file+1);
			goto err_out;
		}
	}else
		server_url = url;

	if ((http_hdr=http_build_header(s, url, server_url, NULL, pos,  proxy))==NULL)
		goto err_out;

	if (http_build_request(http_hdr)==NULL)
		goto err_out;

	if (proxy) {
		if (url->port==0) url->port = 8080;			// Default port for the proxy server
		fd = connect_2_server(url->hostname, url->port, 1, interrupt_cbk);
		url_free( server_url);
		server_url = NULL;
	} else {
		if (url->port==0) url->port = 80;	// Default port for the web server
		fd = connect_2_server(url->hostname, url->port, 1, interrupt_cbk);
	}
	if (fd < 0) {
		goto err_out;
	}

	{
		int debug_network_stream = 0;
		GxPlayer_SystemGet(PSYS_DEBUG_NETWORK_STREAM, &debug_network_stream);
		if (debug_network_stream)
			gxlogi_raw_l("\033[33m>>>>> %s\033[0m\n", http_hdr->buffer);
	}

	ret = http_write(NULL, fd, (uint8_t*)http_hdr->buffer, http_hdr->buffer_size);
	if (ret != (int)http_hdr->buffer_size) {
		gxlogf("Error while sending HTTP request: Didn't sent all the request.\n");
		goto err_out;
	}
	http_freep(&http_hdr);
	return fd;

err_out:
	if (fd > 0)
		closesocket(fd);
	http_freep(&http_hdr);
	if (proxy && server_url)
		url_free(server_url);
	return -1;
}

int http_send_delete(GxStreamHttp* s, int fd, GxURL*  url)
{
	int ret;
	GxHttpHeader* http_hdr;

	if ((fd < 0)||(url == NULL))
		return -1;

	http_hdr = http_new_header();
	http_set_method(http_hdr, "DELETE");
	http_set_uri(http_hdr, url->file);
	http_set_field( http_hdr, "Connection: keep-alive");
	if ( http_build_request( http_hdr )==NULL ) {
		http_freep(&http_hdr);
		return -1;
	}
	ret = http_write(s, fd, (uint8_t*)http_hdr->buffer, http_hdr->buffer_size);
	if (ret != (int)http_hdr->buffer_size) {
		http_freep(&http_hdr);
		return -1;
	}
	http_freep(&http_hdr);
	return 0;
}

GxHttpHeader* http_read_response(int fd, PLAYER_INTERRUPT_CBK interrupt_cbk)
{
	GxHttpHeader* http_hdr;
	char response[2048]={0};
	int i = 0;

	http_hdr = http_new_header();
	if (http_hdr == NULL) {
		return NULL;
	}

	int retry = 100;
	do {
		if (interrupt_cbk && interrupt_cbk()) {
			goto error_response;
		}
		i = http_read(NULL, fd, (uint8_t*)response, 2048);
		if (i < 0) {
			goto error_response;
		}else if (i == 0) {
			retry--;
		}else{
			if (http_response_append(http_hdr, response, i) < 0)
				goto error_response;
		}
	} while (retry && !http_is_header_entire(http_hdr));

	if (http_response_parse( http_hdr ) < 0) {
		goto error_response;
	}

	{
		int debug_network_stream = 0;
		GxPlayer_SystemGet(PSYS_DEBUG_NETWORK_STREAM, &debug_network_stream);
		if (debug_network_stream) {
			if (http_hdr->buffer_size > http_hdr->body_size) {
				char* response1 = av_malloc(http_hdr->buffer_size - http_hdr->body_size + 1);
				memcpy(response1, http_hdr->buffer, http_hdr->buffer_size-http_hdr->body_size);
				response1[http_hdr->buffer_size-http_hdr->body_size] = '\0';
				gxlogi_raw_l("\033[33m<<<<< %s\033[0m\n", response1);
				av_free(response1);
			}
		}
	}

	return http_hdr;

error_response:
	http_freep(&http_hdr);
	return NULL;
}

int http_send_request2(GxStreamHttp* s, GxURL* url, off_t pos, PLAYER_INTERRUPT_CBK interrupt_cbk)
{
	GxURL* server_url = NULL;
	GxHttpHeader* http_hdr = NULL;
	int ret = 0, fd = -1;
	int proxy = 0;		// Boolean

#ifdef CONFIG_DEMUX_TLS
	if (s->https_protocol) {
		if (s->https && s->https->https_open_init)
			https_close(s->https);
		if (https_open(s->https, url->hostname) != 0)
			return -1;
	}
#endif


	if (!strcasecmp(url->protocol, "http_proxy")) {
		proxy = 1;
		server_url = url_new((url->file) + 1);
		if (!server_url) {
			gxlogf("Invalid URL '%s' to proxify\n", url->file+1);
			goto request2_out;
		}
	}else
		server_url = url;

	if (s->https_protocol && url->port == 0)
		url->port = 443;

	if ((http_hdr=http_build_header(s, url, server_url, s->streaming_ctrl->cookies, pos,  proxy))==NULL)
		goto request2_out;

	if (http_build_request(http_hdr)==NULL)
		goto request2_out;

	if(proxy){
		if(url->port==0) url->port = 8080;			// Default port for the proxy server
		fd = connect_2_server(url->hostname, url->port, 1, interrupt_cbk);
		url_free(server_url);
		server_url = NULL;
	} else {
		if (url->port==0) url->port = 80;	// Default port for the web server
		fd = connect_2_server(url->hostname, url->port, 1, interrupt_cbk);
	}

	if (fd < 0)
		goto request2_out;

	{
		int debug_network_stream = 0;
		GxPlayer_SystemGet(PSYS_DEBUG_NETWORK_STREAM, &debug_network_stream);
		if (debug_network_stream)
			gxlogi_raw_l("\033[33m>>>>> %s\033[0m\n", http_hdr->buffer);
	}

	if (s->https_protocol) {
		ret = https_set_fd(s->https, fd);
		if (ret < 0)/*https SSL_connect fail.goto fail*/
			goto request2_out;
	}

	ret = http_write(s, fd, (uint8_t*)http_hdr->buffer, http_hdr->buffer_size);
	if (ret != (int)http_hdr->buffer_size)
		goto request2_out;

	http_freep(&http_hdr);
	return fd;

request2_out:
	if (fd > 0)
		closesocket(fd);
	http_freep(&http_hdr);
	if (proxy && server_url)
		url_free(server_url);
	return -1;
}

GxHttpHeader* http_read_response2(GxStreamHttp* s, int fd, PLAYER_INTERRUPT_CBK interrupt_cbk)
{
	GxHttpHeader* http_hdr;
	char response[2048]={0};
	int  retry = 0;

	http_hdr = http_new_header();
	if (http_hdr == NULL)
		return NULL;

	do {
		if (interrupt_cbk && interrupt_cbk()) {
			goto error_response;
		}
		int i = http_read(s, fd, (uint8_t*)response, 2048);
		if (i < 0) {
			goto error_response;
		}else if (i == 0) {
			retry++;
			if (retry >= 60)
				goto error_response;
		}else{
			if (http_response_append(http_hdr, response, i) < 0)
				goto error_response;
		}
	} while (!http_is_header_entire(http_hdr));

	if (http_response_parse( http_hdr ) < 0) {
		goto error_response;
	}

	{
		int debug_network_stream = 0;
		GxPlayer_SystemGet(PSYS_DEBUG_NETWORK_STREAM, &debug_network_stream);
		if (debug_network_stream) {
			if (http_hdr->buffer_size >= http_hdr->body_size) {
				if (http_hdr->buffer_size != http_hdr->body_size) {
					char* response1 = av_malloc(http_hdr->buffer_size - http_hdr->body_size + 1);
					memcpy(response1, http_hdr->buffer, http_hdr->buffer_size-http_hdr->body_size);
					response1[http_hdr->buffer_size-http_hdr->body_size] = '\0';
					gxlogi_raw_l("\033[33m<<<<< %s\033[0m\n", response1);
					av_free(response1);
				}else{
					gxlogi_raw_l("\033[33m<<<<< %s\033[0m\n", http_hdr->buffer);
				}
			}
		}
	}

	return http_hdr;

error_response:
	http_freep(&http_hdr);
	return NULL;
}

int http_authenticate(GxHttpHeader*  http_hdr, GxURL * url, int *auth_retry)
{
	char* aut;

	if (*auth_retry == 1) {
		gx_msg(MSGT_NETWORK, MSGL_ERR,
				"Authentication failed. Please use the -user and -passwd options to provide your\n"
				"username/password for a list of URLs, or form an URL like:\n"
				"http://username:password@hostname/file\n");
		return -1;
	}
	if (*auth_retry > 0) {
		if (url->username) {
			av_free(url->username);
			url->username = NULL;
		}
		if (url->password) {
			av_free(url->password);
			url->password = NULL;
		}
	}

	aut = http_get_field(http_hdr, "WWW-Authenticate");
	if (aut != NULL) {
		char* aut_space;
		aut_space = strstr(aut, "realm=");
		if (aut_space != NULL)
			aut_space += 6;
		gx_msg(MSGT_NETWORK, MSGL_INFO, "Authentication required for %s\n", aut_space);
	} else {
		gx_msg(MSGT_NETWORK, MSGL_INFO, "Authentication required.\n");
	}
	if (network_username) {
		url->username = av_strdup(network_username);
		if (url->username == NULL) {
			gx_msg(MSGT_NETWORK, MSGL_FATAL, "Memory allocation failed.\n");
			return -1;
		}
	} else {
		gx_msg(MSGT_NETWORK, MSGL_ERR,
				"Authentication failed. Please use the -user and -passwd options to provide your\n"
				"username/password for a list of URLs, or form an URL like:\n"
				"http://username:password@hostname/file\n");
		return -1;
	}
	if (network_password) {
		url->password = av_strdup(network_password);
		if (url->password == NULL) {
			gx_msg(MSGT_NETWORK, MSGL_FATAL, "Memory allocation failed.\n");
			return -1;
		}
	} else {
		gx_msg(MSGT_NETWORK, MSGL_INFO, "No password provided, trying blank password.\n");
	}
	(*auth_retry)++;
	return 0;
}

int nop_streaming_seek(int fd, off_t pos, GxStreamingCtrl*  stream_ctrl)
{
	return -1;
}

int nop_streaming_read(int fd, uint8_t*  buffer, size_t size, GxStreamingCtrl* stream_ctrl)
{
	int len = 0;
	int ret = 0;

	if ((size == 0)||(fd < 0))
		return -1;

	if (stream_ctrl->buffer_size != 0) {
		int buffer_len = stream_ctrl->buffer_size - stream_ctrl->buffer_pos;
		len = (size < buffer_len) ? size : buffer_len;
		memcpy(buffer, (stream_ctrl->buffer) + (stream_ctrl->buffer_pos), len);
		stream_ctrl->buffer_pos += len;
		if (stream_ctrl->buffer_pos >= stream_ctrl->buffer_size) {
			streaming_buffer_free(stream_ctrl);
		}
		return len;
	}

	ret = http_read(stream_ctrl->priv, fd, buffer+len, size-len);
	if (ret < 0) {
		len = -1;
	}else{
		len += ret;
	}
	return len;
}
#endif

GxStreamingCtrl* streaming_ctrl_new(void)
{
	GxStreamingCtrl* streaming_ctrl;
	streaming_ctrl = av_malloc(sizeof(GxStreamingCtrl));
	if (streaming_ctrl == NULL) {
		gx_msg(MSGT_NETWORK, MSGL_FATAL, "Malloc Memory Failed\n");
		return NULL;
	}
	memset(streaming_ctrl, 0, sizeof(GxStreamingCtrl));
	return streaming_ctrl;
}

void streaming_ctrl_free(GxStreamingCtrl*  streaming_ctrl)
{
	if (streaming_ctrl == NULL)
		return;
	if (streaming_ctrl->url)
		url_free(streaming_ctrl->url);
	if (streaming_ctrl->buffer)
		av_free(streaming_ctrl->buffer);
	if (streaming_ctrl->cookies)
		av_free(streaming_ctrl->cookies);
	av_free(streaming_ctrl);
}

int streaming_bufferize(GxStreamingCtrl*  streaming_ctrl, char *buffer, int size)
{
	if (size == 0) {
		av_free(streaming_ctrl->buffer);
		streaming_ctrl->buffer = NULL;
		streaming_ctrl->buffer_size = 0;
		streaming_ctrl->buffer_pos = 0;
		return 0;
	}

	if (streaming_ctrl->buffer_size >= size) {
		av_free(streaming_ctrl->buffer);
		streaming_ctrl->buffer = NULL;
	}

	if (streaming_ctrl->buffer)
		streaming_ctrl->buffer = av_realloc(streaming_ctrl->buffer, size);
	else
		streaming_ctrl->buffer = av_malloc(size);
	if (streaming_ctrl->buffer == NULL) {
		gx_msg(MSGT_NETWORK, MSGL_FATAL, "Memory allocation failed.\n");
		return -1;
	}
	memcpy(streaming_ctrl->buffer, buffer, size);
	streaming_ctrl->buffer_size = size;
	return size;
}

void streaming_buffer_free(GxStreamingCtrl* streaming_ctrl)
{
	if (streaming_ctrl->buffer) {
		av_free(streaming_ctrl->buffer);
		streaming_ctrl->buffer = NULL;
		streaming_ctrl->buffer_size = 0;
		streaming_ctrl->buffer_pos = 0;
	}
	return;
}


