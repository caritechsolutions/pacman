#ifndef _GX_WEBAPI_H
#define _GX_WEBAPI_H
#include <gx_apps.h>

void gx_webapi_start(
					char*ip_addr,
					int port,
					int version,
					char *device_name);

void gx_webapi_stop(void);

char *gx_webapi_str_decrypt(char *src);

int gx_webapi_aes_key_config(unsigned char *key, unsigned char *iv);

#endif
