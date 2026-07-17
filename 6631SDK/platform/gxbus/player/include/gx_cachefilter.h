#ifndef _GX_CACHE_FILTER_H_
#define _GX_CACHE_FILTER_H_

#include "gx_mediafilter.h"
#include "gx_config.h"

#define CACHE_FILTER_CAP (4*1024*1024)

typedef struct gx_cachefilter_class {
	GxMediaFilterClass  _inherit;

}GxCacheFilterClass;

typedef struct gx_cachefilter{
	GxMediaFilter     filter;

    int32_t           pthread;
	unsigned char     buf[CACHE_FILTER_CAP];
}GxCacheFilter;


#define GX_CACHEFILTER_PIN_NAME_IN       "cachefilter_in"
#define GX_CACHEFILTER_PIN_NAME_OUT      "cachefilter_out"

#define GetCacheFilterClassFromObject(s) ((GxCacheFilterClass*)(GetGxClassFromObject(s)))
#define GXCACHEFILTER(s)                 ((GxCacheFilter*)(s))
#define GXCACHEFILTERCLASS(s)            ((GxCacheFilterClass *)(s))

extern GxCacheFilter *GxCacheFilter_Open(void);
extern void           GxCacheFilter_Close(GxCacheFilter * cf);



#endif
