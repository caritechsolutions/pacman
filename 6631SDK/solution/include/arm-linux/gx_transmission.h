#ifndef _GX_TRANSMISSION_H
#define _GX_TRANSMISSION_H
#include <gx_apps.h>

typedef struct gx_transmission_torrent_s
{
	int torrent_id;
	char *torrent_name;
	int torrent_status;
	int is_finish;
	int error;
	char *err_str;
	int connected_peers_num;
	int active_peers_num;
	int download_speed;
	int upload_speed;
	long long download_size;
	long long total_size;
	float download_percent;
}gx_transmission_torrent_t;

typedef struct gx_transmission_torrentlist_s
{
	int torrent_num;
	gx_transmission_torrent_t *torrents;
}gx_transmission_torrentlist_t;

gxapps_ret_t gx_transmission_login(void);

void gx_transmission_logout(void);

void gx_transmission_set_upspeed_limit(int speed);

void gx_transmission_set_downspeed_limit(int speed);

gxapps_ret_t gx_transmission_setting_exec(void);

gxapps_ret_t gx_transmission_get_torrents(
				gx_transmission_torrentlist_t **torrentlist);

void gx_transmission_free_torrents(gx_transmission_torrentlist_t *torrentlist);

gxapps_ret_t gx_transmission_add_torrent(char *path);

gxapps_ret_t gx_transmission_remove_torrent(int torrent_id, int delete_local_data);

gxapps_ret_t gx_transmission_start_torrent(int torrent_id);

gxapps_ret_t gx_transmission_stop_torrent(int torrent_id);

#endif
