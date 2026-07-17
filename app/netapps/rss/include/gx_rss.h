#ifndef _GX_RSS_H
#define _GX_RSS_H
#include <gx_apps.h>

typedef struct gx_rss_img_s {
	char *url;
}gx_rss_img_t;

typedef struct gx_rss_imglist_s {
	int img_num;
	gx_rss_img_t *imgs;
}gx_rss_imglist_t;

typedef struct gx_rss_news_s {
	char *title;
	char *description;
	gx_rss_imglist_t *imglist;
}gx_rss_news_t;

typedef struct gx_rss_newslist_s {
	int news_num;
	gx_rss_news_t *newses;
}gx_rss_newslist_t;

gxapps_ret_t gx_rss_get_news(char *url, gx_rss_newslist_t **newslist);

void gx_rss_free_newslist(gx_rss_newslist_t *newslist);

void gx_rss_set_buffer_size(int size);

#endif
