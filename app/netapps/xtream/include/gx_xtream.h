#ifndef _GX_XTREAM_H
#define _GX_XTREAM_H
#include <gx_apps.h>

typedef enum gx_xtream_format_e{
	XTREAM_FORMAT_AUTO = 0,
	XTREAM_FORMAT_M3U8,
	XTREAM_FORMAT_TS,
	XTREAM_FORMAT_RTMP,
	XTREAM_FORMAT_MAX,
}gx_xtream_format_t;

typedef struct gx_xtream_userinfo_s
{
	char *status;
	char *create_timestamp;
	char *expire_timestamp;
}gx_xtream_userinfo_t;

typedef struct gx_xtream_category_s
{
	int category_id;
	char *category_name;
}gx_xtream_category_t;

typedef struct gx_xtream_categorylist_s
{
	int category_num;
	gx_xtream_category_t *categories;
}gx_xtream_categorylist_t;

typedef struct gx_xtream_channel_s
{
	char *channel_name;
	int category_id;
	int stream_id;
	char *pic_url;
}gx_xtream_channel_t;

typedef struct gx_xtream_channellist_s
{
	int channel_num;
	gx_xtream_channel_t *channels;
}gx_xtream_channellist_t;

typedef struct gx_xtream_livedata_s
{
	gx_xtream_categorylist_t *categorylist;
	gx_xtream_channellist_t *channellist;
}gx_xtream_livedata_t;

typedef struct gx_xtream_movie_s
{
	char *movie_name;
	char *pic_url;
	char *stream_ext;
	int category_id;
	int stream_id;
}gx_xtream_movie_t;

typedef struct gx_xtream_movielist_s
{
	int movie_num;
	gx_xtream_movie_t *movies;
}gx_xtream_movielist_t;

typedef struct gx_xtream_movieinfo_s
{
	char *movie_name;
	char *description;
	char *pic_url;
	char *genre;
	char *added_timestamp;
	char *director;
	char *actors;
}gx_xtream_movieinfo_t;

typedef struct gx_xtream_epg_s
{
	char *title;
	char *description;
	char *start_timestamp;
	char *end_timestamp;
}gx_xtream_epg_t;

typedef struct gx_xtream_epglist_s
{
	int epg_num;
	gx_xtream_epg_t *epgs;
}gx_xtream_epglist_t;

typedef struct gx_xtream_series_s
{
	int series_id;
	char *series_name;
	char *pic_url;
}gx_xtream_series_t;

typedef struct gx_xtream_serieslist_s
{
	int series_num;
	gx_xtream_series_t *series;
}gx_xtream_serieslist_t;

typedef struct gx_xtream_seriesinfo_s
{
	char *description;
	char *genre;
	char *date;
	char *director;
	char *actors;
}gx_xtream_seriesinfo_t;

typedef struct gx_xtream_episode_s
{
	char *season_name;
	char *episode_name;
	char *pic_url;
	char *stream_ext;
}gx_xtream_episode_t;

typedef struct gx_xtream_episodelist_s
{
	int episode_num;
	gx_xtream_episode_t *episodes;
	gx_xtream_seriesinfo_t *seriesinfo;
}gx_xtream_episodelist_t;

void gx_xtream_set_channel_format(gx_xtream_format_t format);

void gx_xtream_set_channel_max(int max);

void gx_xtream_enable_channel_logo(int enable);

gxapps_ret_t gx_xtream_login(char *url,char *username,char *password);

void gx_xtream_logout(void);

gx_xtream_userinfo_t *gx_xtream_get_userinfo(void);

void gx_xtream_free_userinfo(gx_xtream_userinfo_t *userinfo);

gxapps_ret_t gx_xtream_get_channels(
							gx_xtream_livedata_t **livedata);

void gx_xtream_free_channellist(gx_xtream_livedata_t *livedata);

gxapps_ret_t gx_xtream_get_live_playurl(
							int stream_id,
							char **video_url);

void gx_xtream_free_live_playurl(char *video_url);

gxapps_ret_t gx_xtream_get_short_epg(
							int stream_id,
							int limit_num,
							gx_xtream_epglist_t **epglist);

gxapps_ret_t gx_xtream_get_full_epg(
							int stream_id,
							gx_xtream_epglist_t **epglist);

void gx_xtream_free_epglist(gx_xtream_epglist_t *epglist);

gxapps_ret_t gx_xtream_get_movie_categories(
							gx_xtream_categorylist_t **categorylist);

gxapps_ret_t gx_xtream_get_series_categories(
							gx_xtream_categorylist_t **categorylist);

void gx_xtream_free_categorylist(
							gx_xtream_categorylist_t *categorylist);

gxapps_ret_t gx_xtream_get_category_movies(
							int category_id,
							gx_xtream_movielist_t **movielist);

void gx_xtream_free_movielist(
							gx_xtream_movielist_t *movielist);

gxapps_ret_t gx_xtream_get_movie_info(
							int stream_id,
							gx_xtream_movieinfo_t **movieinfo);

void gx_xtream_free_movieinfo(
							gx_xtream_movieinfo_t *movieinfo);

gxapps_ret_t gx_xtream_get_movie_playurl(
							char *stream_ext,
							char **video_url);

void gx_xtream_free_movie_playurl(char *video_url);

gxapps_ret_t gx_xtream_get_category_series(
							int category_id,
							gx_xtream_serieslist_t **serieslist);

void gx_xtream_free_serieslist(
							gx_xtream_serieslist_t *serieslist);

gxapps_ret_t gx_xtream_get_series_episodes(
							int series_id,
							gx_xtream_episodelist_t **episodelist);

void gx_xtream_free_episodelist(
							gx_xtream_episodelist_t *episodelist);

gxapps_ret_t gx_xtream_get_episode_playurl(
							char *stream_ext,
							char **video_url);

void gx_xtream_free_episode_playurl(char *video_url);


#endif
