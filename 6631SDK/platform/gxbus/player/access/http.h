#ifndef STREAM_HTTP_H
#define STREAM_HTTP_H

#include "gx_stream.h"
#include "gx_system.h"
#include "tcp.h"

typedef struct gxhttp_field_type {
	char *field_name;
	struct gxhttp_field_type *next;
}GxHttpField;

#ifdef CONFIG_DEMUX_TLS
#if defined(CONFIG_HAVE_WOLFSSL)
#include <cyassl/openssl/ssl.h>
#elif defined(CONFIG_HAVE_OPENSSL)
#include <openssl/ssl.h>
#endif
#endif
typedef struct gx_stream_https{
	PLAYER_INTERRUPT_CBK interrupt_cbk;
	int https_open_init;
	int https_connect;
#ifdef CONFIG_DEMUX_TLS
	SSL_CTX *ctx;
	SSL *ssl;
#endif
	int fd;
	char *ca_file;
	int verify;
	char *cert_file;
	char *key_file;
}GxStreamHttps;

typedef struct gx_stream_http{
        GxStream parent;
        GxStreamingCtrl *streaming_ctrl;
		int https_protocol;
		int timeout;
		GxStreamHttps *https;
}GxStreamHttp;

#define WHITESPACES " \n\t\r"

#define AV_DICT_MATCH_CASE      1   /**< Only get an entry with exact-case key match. Only relevant in av_dict_get(). */
#define AV_DICT_IGNORE_SUFFIX   2   /**< Return first entry in a dictionary whose first part corresponds to the search key,
                                         ignoring the suffix of the found key string. Only relevant in av_dict_get(). */
#define AV_DICT_DONT_STRDUP_KEY 4   /**< Take ownership of a key that's been
                                         allocated with av_malloc() or another memory allocation function. */
#define AV_DICT_DONT_STRDUP_VAL 8   /**< Take ownership of a value that's been
                                         allocated with av_malloc() or another memory allocation function. */
#define AV_DICT_DONT_OVERWRITE 16   ///< Don't overwrite existing entries.
#define AV_DICT_APPEND         32   /**< If the entry already exists, append to it.  Note that no
                                      delimiter is added, the strings are simply concatenated. */
#define AV_DICT_MULTIKEY       64   /**< Allow to store several equal keys in the dictionary */

typedef struct http_head_t{
	char *protocol;
	char *method;
	char *uri;
	unsigned int status_code;
	char *reason_phrase;
	unsigned int http_minor_version;
	// Field variables
	GxHttpField *first_field;
	GxHttpField *last_field;
	unsigned int field_nb;
	char *field_search;
	GxHttpField *field_search_pos;
	// Body variables
	char *body;
	size_t body_size;
	char *buffer;
	size_t buffer_size;
	unsigned int is_parsed;
	char acSavCkBuf[1024];
	size_t iCookieTotal;
} GxHttpHeader;


void http_closesocket(GxStream* stream);
GxHttpHeader*	http_new_header(void);
void http_free( GxHttpHeader *http_hdr );
void http_freep(GxHttpHeader** http_hdr);
int	http_response_append( GxHttpHeader *http_hdr, char *data, int length );
int	http_response_parse( GxHttpHeader *http_hdr );
int	http_is_header_entire( GxHttpHeader *http_hdr );
char* http_build_request(GxHttpHeader *http_hdr );
char* http_get_field(GxHttpHeader *http_hdr, const char *field_name );
char* http_get_next_field( GxHttpHeader *http_hdr );
void http_set_field( GxHttpHeader *http_hdr, const char *field_name );
void http_set_fields( GxHttpHeader *http_hdr, const char **field_name, unsigned int count);
void http_set_method( GxHttpHeader *http_hdr, const char *method );
void http_set_uri(GxHttpHeader *http_hdr, const char *uri );
int	 http_add_basic_authentication( GxHttpHeader *http_hdr, const char *username, const char *password );
void http_debug_hdr( GxHttpHeader *http_hdr );
int http_send_request(GxStreamHttp* s, GxURL *url, off_t pos, PLAYER_INTERRUPT_CBK interrupt_cbk);
int  http_send_request2(GxStreamHttp* s, GxURL*  url, off_t pos, PLAYER_INTERRUPT_CBK interrupt_cbk);
GxHttpHeader* http_read_response(int fd, PLAYER_INTERRUPT_CBK cbk);
GxHttpHeader* http_read_response2(GxStreamHttp* s, int fd, PLAYER_INTERRUPT_CBK interrupt_cbk);
int http_authenticate(GxHttpHeader *http_hdr, GxURL *url, int *auth_retry);
int http_add_basic_proxy_authentication( GxHttpHeader *http_hdr, const char *username, const char *password );
int http_send_delete(GxStreamHttp* s, int fd, GxURL*  url);
void https_init(void);
void https_deinit(void);
void https_shutdown(GxStreamHttps* c);
int https_open(GxStreamHttps* c, const char *hostname);
void https_close(GxStreamHttps* c);
int https_read(GxStreamHttps* c, uint8_t *buf, int size);
int https_write(GxStreamHttps* c, uint8_t *buf, int size);
#endif /* HTTP_H */
