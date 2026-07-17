#ifndef _GX_FREEIPTV_H
#define _GX_FREEIPTV_H
#include <gx_apps.h>

typedef struct gx_freeiptv_category_s
{
	int category_id;
	char *category_name;
}gx_freeiptv_category_t;

typedef struct gx_freeiptv_categorylist_s
{
	int category_num;
	gx_freeiptv_category_t *categories;
}gx_freeiptv_categorylist_t;

typedef struct gx_freeiptv_channel_s
{
	int category_id;
	char *channel_name;
	char *play_url;
}gx_freeiptv_channel_t;

typedef struct gx_freeiptv_channellist_s
{
	int channel_num;
	gx_freeiptv_channel_t *channels;
	gx_freeiptv_categorylist_t *categorylist;
}gx_freeiptv_channellist_t;

gxapps_ret_t gx_freeiptv_get_channels(
							gx_freeiptv_channellist_t **channellist);

void gx_freeiptv_free_channellist(gx_freeiptv_channellist_t *channellist);

#endif
