#ifndef _GX_STALKER_H
#define _GX_STALKER_H
#include <gx_apps.h>

typedef struct gx_stalker_userinfo_s
{
	char *mac;
	char *create_timestamp;
	char *expire_timestamp;
}gx_stalker_userinfo_t;

typedef struct gx_stalker_category_s
{
	char *category_name;
	int category_id;
	int censored;
}gx_stalker_category_t;

typedef struct gx_stalker_categorylist_s
{
	int category_num;
	gx_stalker_category_t *categories;
}gx_stalker_categorylist_t;

typedef struct gx_stalker_channel_s
{
	char *channel_name;
	int category_id;
	int stream_id;
	char *pic_url;
	char *play_cmd;
	int archive;
	int lock;
}gx_stalker_channel_t;

typedef struct gx_stalker_channellist_s
{
	int channel_num;
	int total_channel_num;
	gx_stalker_channel_t *channellist;
	gx_stalker_categorylist_t *categorylist;
}gx_stalker_channellist_t;

typedef struct gx_stalker_day_s
{
	char *id;
	char *title;
	int is_today;
}gx_stalker_day_t;

typedef struct gx_stalker_daylist_s
{
	int day_num;
	gx_stalker_day_t *days;
}gx_stalker_daylist_t;

typedef struct gx_stalker_epg_s
{
	char *id;
	char *title;
	char *description;
	char *start_timestamp;
	char *end_timestamp;
	int mark_archive;
}gx_stalker_epg_t;

typedef struct gx_stalker_epglist_s
{
	int epg_num;
	int epg_total_num;
	gx_stalker_epg_t *epgs;
}gx_stalker_epglist_t;

typedef struct gx_stalker_movie_s
{
	char *movie_name;
	char *description;
	int duration;
	char *pic_url;
	char *genre;
	char *date;
	char *rating;
	char *age;
	char *year;
	char *director;
	char *actors;
	char *play_cmd;
	int episode_num;
	char **episodes;
}gx_stalker_movie_t;

typedef struct gx_stalker_movielist_s
{
	int movie_num;
	int page_movie_num;;
	int movie_total_num;
	gx_stalker_movie_t *movielist;
}gx_stalker_movielist_t;

typedef struct gx_stalker_series_s
{
	int is_hd;
	char *series_id;
	char *category_id;
	char *name;
	char *description;
	char *pic_url;
	char *genre;
	char *date;
	char *rating;
	char *age;
	char *year;
	char *director;
	char *actors;
}gx_stalker_series_t;

typedef struct gx_stalker_serieslist_s
{
	int series_num;
	int page_series_num;;
	int series_total_num;
	gx_stalker_series_t *series;
}gx_stalker_serieslist_t;

typedef struct gx_stalker_season_s
{
	int is_hd;
	char *name;
	char *description;
	char *pic_url;
	char *genre;
	char *date;
	char *rating;
	char *age;
	char *year;
	char *director;
	char *actors;
	char *play_cmd;
	int episode_num;
	char **episodes;
}gx_stalker_season_t;

typedef struct gx_stalker_seasonlist_s
{
	int season_num;
	int page_season_num;;
	int season_total_num;
	gx_stalker_season_t *seasons;
}gx_stalker_seasonlist_t;

gxapps_ret_t gx_stalker_login(char *url, char *mac, char *user, char *pass);

void gx_stalker_logout(void);

gx_stalker_userinfo_t *gx_stalker_get_userinfo(void);

void gx_stalker_free_userinfo(gx_stalker_userinfo_t *userinfo);

void gx_stalker_set_channel_max(int max);

void gx_stalker_set_channel_buffer_size(int size);

gxapps_ret_t gx_stalker_get_channels(
							gx_stalker_channellist_t **channellist);

gxapps_ret_t gx_stalker_get_category_channels(
							int category_id,
							gx_stalker_channellist_t **channellist);

void gx_stalker_free_channellist(gx_stalker_channellist_t *channellist);

gxapps_ret_t gx_stalker_get_live_playurl(
							char *cmd,
							char **video_url);

void gx_stalker_free_live_playurl(
							char *video_url);

gxapps_ret_t gx_stalker_get_short_epg(
							int stream_id,
							int limit_num,
							gx_stalker_epglist_t **epglist);

void gx_stalker_free_epglist(gx_stalker_epglist_t *epglist);

gxapps_ret_t gx_stalker_get_epg_days(
							gx_stalker_daylist_t **daylist);

void gx_stalker_free_daylist(gx_stalker_daylist_t *daylist);


gxapps_ret_t gx_stalker_get_full_epg(
							int stream_id,
							char *day_id,
							int page,
							gx_stalker_epglist_t **epglist);

gxapps_ret_t gx_stalker_get_epg_archive_playurl(
							char *id,
							char **video_url);

void gx_stalker_free_epg_archive_playurl(
							char *video_url);

gxapps_ret_t gx_stalker_get_movie_categories(
							gx_stalker_categorylist_t **categorylist);

void gx_stalker_free_categorylist(gx_stalker_categorylist_t *categorylist);

gxapps_ret_t gx_stalker_get_category_movies(
							int category_id,
							int page,
							gx_stalker_movielist_t **movielist);

void gx_stalker_free_movielist(gx_stalker_movielist_t *movielist);

gxapps_ret_t gx_stalker_search_movies(
							char *keywords,
							int page,
							gx_stalker_movielist_t **movielist);

gxapps_ret_t gx_stalker_get_movie_playurl(
							char *cmd,
							int series_index,
							char **video_url);

void gx_stalker_free_movie_playurl(
							char *video_url);

gxapps_ret_t gx_stalker_get_series_categories(
							gx_stalker_categorylist_t **categorylist);

gxapps_ret_t gx_stalker_get_category_series(
							int category_id,
							int page,
							gx_stalker_serieslist_t **serieslist);

void gx_stalker_free_serieslist(gx_stalker_serieslist_t *serieslist);

gxapps_ret_t gx_stalker_search_series(
							char *keywords,
							int page,
							gx_stalker_serieslist_t **serieslist);

gxapps_ret_t gx_stalker_get_series_seasons(
							char *category_id,
							char *series_id,
							int page,
							gx_stalker_seasonlist_t **seasonlist);

void gx_stalker_free_seasonlist(gx_stalker_seasonlist_t *seasonlist);

gxapps_ret_t gx_stalker_get_episode_playurl(
							char *cmd,
							char *episode,
							char **video_url);

void gx_stalker_free_episode_playurl(
							char *video_url);


#endif
