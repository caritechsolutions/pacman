
#ifndef COOKIES_H
#define COOKIES_H

#include "http.h"

extern void cookies_set(GxHttpHeader * http_hdr, const char *hostname,
			const char *url);

#endif

