#ifndef __GX_URL_H__
#define __GX_URL_H__

#include <stdint.h>

typedef struct {
	char *url;
	char *noauth_url;
	char *protocol;
	char *hostname;
	char *file;
	unsigned int port;
	char *username;
	char *password;
	char *user_agent;
	char *referer;
} GxURL;

int url_is_protocol(const GxURL *url, const char *proto);
void url_set_protocol(GxURL *url, const char *proto);
char *get_http_proxy_url(const GxURL *proxy, const char *host_url);

GxURL* url_new(const char* url);
void   url_free(GxURL* url);
GxURL *url_redirect(GxURL **url, const char *redir);
GxURL* url_new(const char* url);
void   url_free(GxURL* url);
int url_str_copy(char* src_url, char* dst_url, int dst_len);
char* url_new_nooptions(const char* src_url);

void url_unescape_string(char *outbuf, const char *inbuf);
void url_escape_string(char *outbuf, const char *inbuf);

char *data_to_hex(char *buff, const uint8_t *src, int s, int lowercase);
int hex_to_data(unsigned char *data, const char *p);
void protocol_by_url(char *url, char *proto, int proto_len);
int parse_by_url(char *src, char *keystring, char *buffer);

#ifdef URL_DEBUG
void GxURL_Debug(const GxURL* url);
#endif /* URL_DEBUG */

#endif /* URL_H */

