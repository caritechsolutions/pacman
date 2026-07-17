/*
 * TLS/SSL Protocol
 * Copyright (c) 2011 Martin Storsjo
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

#include "avformat.h"
#include "avio.h"
#include "../avutil/avstring.h"
#include "../avutil/opt.h"
#include "network.h"
#if defined(CONFIG_HAVE_WOLFSSL)
#include <cyassl/openssl/bio.h>
#include <cyassl/openssl/ssl.h>
#include <cyassl/openssl/err.h>
#elif defined(CONFIG_HAVE_OPENSSL)
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif
#define TLS_read(c, buf, size)  SSL_read(c->ssl,  buf, size)
#define TLS_write(c, buf, size) SSL_write(c->ssl, buf, size)
#define TLS_shutdown(c)         SSL_shutdown(c->ssl)
#define TLS_free(c) do { \
	if (c->ssl) \
	SSL_free(c->ssl); \
	if (c->ctx) \
	SSL_CTX_free(c->ctx); \
} while (0)
#include "gx_poll.h"

typedef struct TlsSupCipherSuiteInfo {
	int64_t cipherSuite;
	const char* name;
} TlsSupCipherSuiteInfo;

typedef struct {
	URLContext *tcp;
	SSL_CTX *ctx;
	SSL *ssl;
	int fd;
	char *ca_file;
	int verify;
	char *cert_file;
	char *key_file;
	char *host;
	char cipher_name[2048];
	int listen;
	int numerichost;
} TLSContext;

#if defined(CONFIG_HAVE_WOLFSSL)
/*a_li cipher suite configue.*/
///#define DEFAULT_CIPHER_SUITE_LIST "ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA:ECDHE-ECDSA-DES-CBC3-SHA:ECDHE-RSA-AES128-SHA:ECDHE-RSA-DES-CBC3-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:EDH-RSA-DES-CBC3-SHA:AES128-SHA256:AES128-SHA:DES-CBC3-SHA:AES256-SHA:AES256-SHA256:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-GCM-SHA256:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-ECDSA-RC4-SHA:ECDHE-RSA-RC4-SHA:DHE-RSA-AES256-SHA256:DHE-RSA-AES256-SHA:RC4-SHA:RC4-MD5:HC128-MD5:HC128-SHA:RABBIT-SHA"
/*s_unplus cipher suite configue.*/
#define DEFAULT_CIPHER_SUITE_LIST "AES128-SHA:AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA256:AES256-SHA:AES256-SHA256:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:ECDHE-ECDSA-DES-CBC3-SHA:ECDHE-RSA-DES-CBC3-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:EDH-RSA-DES-CBC3-SHA:DES-CBC3-SHA:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-GCM-SHA256:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-ECDSA-RC4-SHA:ECDHE-RSA-RC4-SHA:DHE-RSA-AES256-SHA256:DHE-RSA-AES256-SHA:RC4-SHA:RC4-MD5:HC128-MD5:HC128-SHA:RABBIT-SHA"
#endif

static const TlsSupCipherSuiteInfo tls_cipher_list[] = {
{/*TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384*/(0xc02c), "ECDHE-ECDSA-AES256-GCM-SHA384"},
{/*TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256*/(0xc02b), "ECDHE-ECDSA-AES128-GCM-SHA256"},
{/*TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384*/	(0xc030), "ECDHE-RSA-AES256-GCM-SHA384"},
{/*TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256*/	(0xc02f), "ECDHE-RSA-AES128-GCM-SHA256"},
{/*TLS_DHE_RSA_WITH_AES_256_GCM_SHA384*/	(0x009f), "DHE-RSA-AES256-GCM-SHA384"},
{/*TLS_DHE_RSA_WITH_AES_128_GCM_SHA256*/	(0x009e), "DHE-RSA-AES128-GCM-SHA256"},
{/*TLS_RSA_WITH_AES_256_GCM_SHA384*/		(0x009d), "AES256-GCM-SHA384"},
{/*TLS_RSA_WITH_AES_128_GCM_SHA256*/		(0x009c), "AES128-GCM-SHA256"},
{/*TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384*/ (0xc02e), "ECDH-ECDSA-AES256-GCM-SHA384"},
{/*TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256*/ (0xc02d), "ECDH-ECDSA-AES128-GCM-SHA256"},
{/*TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384*/	(0xc032), "ECDH-RSA-AES256-GCM-SHA384"},
{/*TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256*/	(0xc031), "ECDH-RSA-AES128-GCM-SHA256"},
{/*TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256*/	(0xc027), "ECDHE-RSA-AES128-SHA256"},
{/*TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256*/(0xc023), "ECDHE-ECDSA-AES128-SHA256"},
{/*TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256*/	(0xc029), "ECDH-RSA-AES128-SHA256"},
{/*TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256*/ (0xc025), "ECDH-ECDSA-AES128-SHA256"},
{/*TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384*/	(0xc028), "ECDHE-RSA-AES256-SHA384"},
{/*TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384*/(0xc024), "ECDHE-ECDSA-AES256-SHA384"},
{/*TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384*/	(0xc02a), "ECDH-RSA-AES256-SHA384"},
{/*TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384*/ (0xc026), "ECDH-ECDSA-AES256-SHA384"},
{/*TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA*/	(0xc00a), "ECDHE-ECDSA-AES256-SHA"},
{/*TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA*/	(0xc005), "ECDH-ECDSA-AES256-SHA"},
{/*TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA*/	(0xc009), "ECDHE-ECDSA-AES128-SHA"},
{/*TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA*/	(0xc004), "ECDH-ECDSA-AES128-SHA"},
{/*TLS_ECDHE_ECDSA_WITH_RC4_128_SHA*/		(0xc007), "ECDHE-ECDSA-RC4-SHA"},
{/*TLS_ECDH_ECDSA_WITH_RC4_128_SHA*/		(0xc002), "ECDH-ECDSA-RC4-SHA"},
{/*TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA*/	(0xc008), "ECDHE-ECDSA-DES-CBC3-SHA"},
{/*TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA*/	(0xc003), "ECDH-ECDSA-DES-CBC3-SHA"},
{/*TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA*/ 	(0xc014), "ECDHE-RSA-AES256-SHA"},
{/*TLS_ECDH_RSA_WITH_AES_256_CBC_SHA*/		(0xc00f), "ECDH-RSA-AES256-SHA"},
{/*TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA*/ 	(0xc013), "ECDHE-RSA-AES128-SHA"},
{/*TLS_ECDH_RSA_WITH_AES_128_CBC_SHA*/		(0xc00e), "ECDH-RSA-AES128-SHA"},
{/*TLS_ECDHE_RSA_WITH_RC4_128_SHA*/ 		(0xc011), "ECDHE-RSA-RC4-SHA"},
{/*TLS_ECDH_RSA_WITH_RC4_128_SHA*/			(0xc00c), "ECDH-RSA-RC4-SHA"},
{/*TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA*/	(0xc012), "ECDHE-RSA-DES-CBC3-SHA"},
{/*TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA*/ 	(0xc00d), "ECDH-RSA-DES-CBC3-SHA"},
{/*TLS_DHE_RSA_WITH_AES_256_CBC_SHA256*/	(0x006b), "DHE-RSA-AES256-SHA256"},
{/*TLS_DHE_RSA_WITH_AES_128_CBC_SHA256*/	(0x0067), "DHE-RSA-AES128-SHA256"},
{/*TLS_DHE_RSA_WITH_AES_256_CBC_SHA*/		(0x0039), "DHE-RSA-AES256-SHA"},
{/*TLS_DHE_RSA_WITH_AES_128_CBC_SHA*/		(0x0033), "DHE-RSA-AES128-SHA"},
{/*TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA*/		(0x0016), "EDH-RSA-DES-CBC3-SHA"},
{/*TLS_RSA_WITH_AES_256_CBC_SHA256*/		(0x003d), "AES256-SHA256"},
{/*TLS_RSA_WITH_AES_128_CBC_SHA256*/		(0x003c), "AES128-SHA256"},
{/*TLS_RSA_WITH_AES_256_CBC_SHA*/			(0x0035), "AES256-SHA"},
{/*TLS_RSA_WITH_AES_128_CBC_SHA*/			(0x002f), "AES128-SHA"},
{/*TLS_RSA_WITH_RC4_128_SHA*/				(0x0005), "RC4-SHA"},
{/*TLS_RSA_WITH_RC4_128_MD5*/				(0x0004), "RC4-MD5"},
{/*TLS_RSA_WITH_3DES_EDE_CBC_SHA*/			(0x000a), "DES-CBC3-SHA"},
{/*TLS_RSA_WITH_HC_128_MD5*/				(0x00fb), "HC128-MD5"},
{/*TLS_RSA_WITH_HC_128_SHA*/				(0x00fc), "HC128-SHA"},
{/*TLS_RSA_WITH_RABBIT_SHA*/				(0x00fd), "RABBIT-SHA"},
};

static int do_tls_poll(URLContext *h, int ret, int retry)
{
	TLSContext *c = h->priv_data;
	struct pollfd p = { c->fd, 0, 0 };

	AVDictionaryEntry *e = NULL;
	if ((e = av_dict_get(h->url_context_options, "rw_timeout", NULL, 0))) {
		retry = AV_MAX((atoi(e->value)*10), 50);/*timeout min:5S*/
	}

	ret = SSL_get_error(c->ssl, ret);
	if (ret == SSL_ERROR_WANT_READ) {
		p.events = POLLIN;
	} else if (ret == SSL_ERROR_WANT_WRITE) {
		p.events = POLLOUT;
	} else {
		return AVERROR(EIO);
	}
	if (h->flags & AVIO_FLAG_NONBLOCK)
		return AVERROR(EAGAIN);
	while (retry > 0) {
		int n = poll(&p, 1, AV_POLLING_TIME);
        if (n == -1)
			return AVERROR(EIO);
		else if (n > 0)
			break;
		else if (n == 0)
			retry --;
		if (url_check_interrupt_cb())
			return AVERROR_EXIT;
	}
	if (retry <= 0)
		return AVERROR_UNKNOWN;
	return 0;
}

static void set_options(URLContext *h, const char *uri)
{
	TLSContext *c = h->priv_data;
	char buf[1024] = {0};
	const char *p = strchr(uri, '?');
	if (!p)
		return;

	if (!c->ca_file && av_find_info_tag(buf, sizeof(buf), "cafile", p))
		c->ca_file = av_strdup(buf);

	if (!c->verify && av_find_info_tag(buf, sizeof(buf), "verify", p)) {
		char *endptr = NULL;
		c->verify = strtol(buf, &endptr, 10);
		if (buf == endptr)
			c->verify = 1;
	}

	if (!c->cert_file && av_find_info_tag(buf, sizeof(buf), "cert", p)) {
		if (strlen(buf) > 0)
			c->cert_file = av_strdup(buf);
	}

	if (!c->key_file && av_find_info_tag(buf, sizeof(buf), "key", p)) {
		if (strlen(buf) > 0)
			c->key_file = av_strdup(buf);
	}
}

#if 0//defined(CONFIG_HAVE_WOLFSSL)
static int print_tls_error(URLContext *h, int ret, int flags)
{
	return AVERROR(EIO);
}
#endif

int ff_tls_open_underlying(URLContext *h, const char *uri)
{
	TLSContext *c = h->priv_data;
	int ret= -1, port = -1, use_proxy = 0;
	char buf[200], host[200], opts[50] = "";
	struct addrinfo hints = { 0 }, *ai = NULL;
	char *proxy_path = NULL;
	AVDictionaryEntry *e = NULL;
	const char *p = strchr(uri, '?');

	if(p && av_find_info_tag(buf, sizeof(buf), "listen", p))
		c->listen = 1;

	if (c->listen)
		snprintf(opts, sizeof(opts), "?listen=1");

	av_url_split(NULL, 0, NULL, 0, host, sizeof(host), &port, NULL, 0, uri);
	ff_url_join(buf, sizeof(buf), "tcp", NULL, host, port, "%s", opts);

	hints.ai_flags = AI_NUMERICHOST;
	if (!getaddrinfo(host, NULL, &hints, &ai)) {
		c->numerichost = 1;
		freeaddrinfo(ai);
	}
	set_options(h, uri);

	if ((e = av_dict_get(h->url_context_options, "http_proxy", NULL, 0))) {
		proxy_path = e->value;
		use_proxy  = proxy_path && av_strstart(proxy_path, "http://", NULL);
	}

	if (use_proxy) {
		char proxy_host[200], proxy_auth[200], dest[200];
		int proxy_port;
		av_url_split(NULL, 0, proxy_auth, sizeof(proxy_auth),
				proxy_host, sizeof(proxy_host), &proxy_port, NULL, 0, proxy_path);
		ff_url_join(dest, sizeof(dest), NULL, NULL, host, port, NULL);
		ff_url_join(buf, sizeof(buf), "httpproxy", proxy_auth, proxy_host,
				proxy_port, "/%s", dest);
	}

	ret = url_open(&c->tcp, buf, URL_RDWR, &h->url_context_options);
	if (ret)
		goto fail;
	c->fd = url_get_file_handle(c->tcp);
	if (!c->host && !(c->host = av_strdup(host)))
		ret = AVERROR(ENOMEM);
fail:
	return ret;
}

#if 0//defined(CONFIG_HAVE_WOLFSSL)
static int wolfssl_recv_callback(WOLFSSL* ssl, char* buf, int sz, void* ctx)
{
    URLContext *h = (URLContext*) ctx;
    int ret = url_read(h, (unsigned char *)buf, sz);
    if (ret >= 0)
        return ret;
    if (ret == AVERROR_EXIT)
        return WOLFSSL_CBIO_ERR_GENERAL;
    errno = EIO;
    gxlogd ("tls.c..recv callback read fail..ret=%d.\n", ret);
    return WOLFSSL_CBIO_ERR_GENERAL;
}

static int wolfssl_send_callback(WOLFSSL* ssl, char* buf, int sz, void* ctx)
{
    URLContext *h = (URLContext*) ctx;
    int ret = url_write(h, (unsigned char *)buf, sz);
    if (ret >= 0)
        return ret;
    if (ret == AVERROR_EXIT)
        return WOLFSSL_CBIO_ERR_GENERAL;
    errno = EIO;
    gxlogd ("tls.c..send send fail..ret=%d.\n", ret);
    return WOLFSSL_CBIO_ERR_GENERAL;
}
#endif

static int tls_open(URLContext *h, const char *uri, int flags)
{
	TLSContext *c = h->priv_data;
	PLAYER_INTERRUPT_CBK interrupt_cbk;
	AVDictionaryEntry *e = NULL;
	int ret= -1;

	ff_tls_init();

	if ((ret = ff_tls_open_underlying(h, uri)))
		goto fail;

	c->ctx = SSL_CTX_new(c->listen ? SSLv23_server_method() : SSLv23_client_method());
	if (!c->ctx) {
		gxloge ("tls.c fail.error:%s", ERR_error_string(ERR_get_error(), NULL));
		ret = AVERROR(EIO);
		goto fail;
	}
#if defined(CONFIG_HAVE_WOLFSSL)
	if ((e = av_dict_get(h->url_context_options, "ssl_cipher_list", NULL, 0)) && e->value) {
		int len = 0, i = 0;
		char *token = strtok(e->value, ":");
		memset (c->cipher_name, 0, sizeof(c->cipher_name));
		while (token != NULL) {
			int64_t cipherSuite = strtoll(token, NULL, 16);
			for (i = 0; i < sizeof(tls_cipher_list) / sizeof(tls_cipher_list[0]); i++) {
				if (tls_cipher_list[i].cipherSuite == cipherSuite) {
					len += av_strlcatf(c->cipher_name + len, sizeof(c->cipher_name) - len, "%s", tls_cipher_list[i].name);
					break;
				}
			}
			if (len >= sizeof(c->cipher_name))
				break;
			if ((token = strtok(NULL, ":")))
				len += av_strlcatf(c->cipher_name + len, sizeof(c->cipher_name) - len, "%s", ":");
		}
		SSL_CTX_set_cipher_list(c->ctx, c->cipher_name);
	} else {
		CUSTOM_SSL_CHPHER_SUITE_CBK ssl_chipher_suite = NULL;
		char *custom_ssl_cipher = NULL;
		GxPlayer_SystemGet(PSYS_NETWORK_CUSTOM_SSL_CIPHER_SUITE, &ssl_chipher_suite);
		if (ssl_chipher_suite && ssl_chipher_suite(&custom_ssl_cipher)) {
			SSL_CTX_set_cipher_list(c->ctx, custom_ssl_cipher);
		} else {
			SSL_CTX_set_cipher_list(c->ctx, DEFAULT_CIPHER_SUITE_LIST);
		}
	}
#endif
	SSL_CTX_set_options(c->ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
	if (c->ca_file) {
		if (!SSL_CTX_load_verify_locations(c->ctx, c->ca_file, NULL))
			gxlogd ("SSL_CTX_load_verify_locations %s\n", ERR_error_string(ERR_get_error(), NULL));
	}
	if (c->cert_file && !SSL_CTX_use_certificate_chain_file(c->ctx, c->cert_file)) {
		ret = AVERROR(EIO);
		goto fail;
	}
	if (c->key_file && !SSL_CTX_use_PrivateKey_file(c->ctx, c->key_file, SSL_FILETYPE_PEM)) {
		ret = AVERROR(EIO);
		goto fail;
	}
	SSL_CTX_set_timeout(c->ctx, 10);
	// Note, this doesn't check that the peer certificate actually matches the requested hostname.
	SSL_CTX_set_verify(c->ctx, (c->verify)?(SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT):SSL_VERIFY_NONE, NULL);
#if 0//defined(CONFIG_HAVE_WOLFSSL)
	wolfSSL_CTX_SetIORecv(c->ctx, wolfssl_recv_callback);
	wolfSSL_CTX_SetIOSend(c->ctx, wolfssl_send_callback);
#endif
	c->ssl = SSL_new(c->ctx);
	if (!c->ssl) {
		ret = AVERROR(EIO);
		goto fail;
	}
	if (!SSL_set_fd(c->ssl, c->fd)) {
		ret = AVERROR(EIO);
		goto fail;
	}

	if (!c->listen && !c->numerichost)
		SSL_set_tlsext_host_name(c->ssl, c->host);

	GxPlayer_SystemGet(PSYS_CBK_INTERRUPT, &interrupt_cbk);
	SSL_check_interrupt_cbk(c->ssl,interrupt_cbk);

#if 0//defined(CONFIG_HAVE_WOLFSSL)
	wolfSSL_SetIOReadCtx(c->ssl, c->tcp);
	wolfSSL_SetIOWriteCtx(c->ssl, c->tcp);
#endif
	while (1) {
		ret = c->listen ? SSL_accept(c->ssl) : SSL_connect(c->ssl);
		if (ret > 0)
			break;
		if (ret == 0) {
			ret = AVERROR(EIO);
			goto fail;
		}
		if (ret == WOLFSSL_ERROR_EXIT) {
			ret = AVERROR_EXIT;
			goto fail;
		}
		if ((ret = do_tls_poll(h, ret, 500)) < 0)
			goto fail;
	}
	if (c->host) {
		av_free(c->host);
		c->host = NULL;
	}
	return 0;
fail:
	TLS_free(c);
	if (c->host) {
		av_free(c->host);
		c->host = NULL;
	}
	if (c->tcp)
		url_close(c->tcp);
	ff_tls_deinit();
	return ret;
}

static int tls_read(URLContext *h, uint8_t *buf, int size)
{
#if 0//defined(CONFIG_HAVE_WOLFSSL)
	TLSContext *c = h->priv_data;
	int ret = TLS_read(c, buf, size);
	if (ret > 0)
		return ret;
	if (ret == 0)
		return AVERROR_EOF;
	return print_tls_error(h, ret, 1);
#else
	TLSContext *c = h->priv_data;
	while (1) {
		int ret = TLS_read(c, buf, size);
		if (ret > 0){
			return ret;
		}
		if (ret == 0)
			return AVERROR_EOF;
		if ((ret = do_tls_poll(h, ret, 150)) < 0)
			return ret;
	}
	return 0;
#endif
}

static int tls_write(URLContext *h, uint8_t *buf, int size)
{
#if 0//defined(CONFIG_HAVE_WOLFSSL)
	TLSContext *c = h->priv_data;
	int ret = TLS_write(c, buf, size);
	if (ret > 0)
		return ret;
	if (ret == 0)
		return AVERROR_EOF;
	return print_tls_error(h, ret, 2);
#else
	TLSContext *c = h->priv_data;
	while (1) {
		int ret = TLS_write(c, buf, size);
		if (ret > 0)
			return ret;
		if (ret == 0)
			return AVERROR_EOF;
		if ((ret = do_tls_poll(h, ret, 200)) < 0)
			return ret;
	}
	return 0;
#endif
}

static int tls_close(URLContext *h)
{
	TLSContext *c = h->priv_data;
	TLS_shutdown(c);
	TLS_free(c);
	url_close(c->tcp);
	ff_tls_deinit();
	return 0;
}

URLProtocol gx_tls_protocol = {
	.name           = "tls",
	.url_open       = tls_open,
	.url_read       = tls_read,
	.url_write      = tls_write,
	.url_close      = tls_close,
	.priv_data_size = sizeof(TLSContext),
};
