#ifndef _GX_APPLETS_H
#define _GX_APPLETS_H
#include <gx_apps.h>

typedef struct gx_applet_s
{
	char *id;
	char *name;
	char *url;
	char *md5;
	char *icon_url;
	char *version;
	int encryption;
}gx_applet_t;

typedef struct gx_appletlist_s
{
	int applet_num;
	gx_applet_t *applets;
}gx_appletlist_t;

typedef struct gx_applets_builtin_s{
	char *id;
	char *name;
	char *icon_url;
}gx_applets_builtin_t;

typedef struct gx_applets_builtinlist_s
{
	int builtin_num;
	gx_applets_builtin_t *builtin;
}gx_applets_builtinlist_t;

typedef struct gx_applets_install_s{
	char *id;
	int encryption;
	int index;
}gx_applets_install_t;

typedef struct gx_applets_installlist_s
{
	int install_num;
	gx_applets_install_t *install;
}gx_applets_installlist_t;

typedef struct gx_applets_fav_s{
	char *id;
	int  source;//0:cloud; 1:builtin
}gx_applets_fav_t;

typedef struct gx_applets_favlist_s
{
	int fav_num;
	gx_applets_fav_t *fav;
}gx_applets_favlist_t;


typedef struct gx_applets_app_s{
	int need_templet;
	char *id;
	char *file;
	char *category;
	char *version;
	char *title;
	char *icon;
	char *description;
	char *entry_func;
	char *exit_func;
}gx_applets_app_t;

typedef struct gx_applets_applist_s
{
	int app_num;
	gx_applets_app_t *app;
}gx_applets_applist_t;


void gx_applets_service_start(void);

void gx_applets_service_stop(void);

void gx_applets_set_customer_server_url(char *url);

gxapps_ret_t gx_applets_get_appletlist(
							gx_appletlist_t **appletlist);

void gx_applets_free_appletlist(gx_appletlist_t *appletlist);

int gx_applets_aes_key_config(unsigned char *key, unsigned char *iv);

int gx_applets_checksum(unsigned char *src,  int srclen,char *sign);

char * gx_applets_decrypt(char *src,  int srclen);

int gx_applets_unzip_app(char *zip_src,char *path);

int gx_applets_entry_obj(char *obj_path,char *obj_entry,char *obj_exit);

void  gx_applets_destory_obj(void);

gxapps_ret_t gx_applets_get_installlist(gx_applets_installlist_t ** list,char* install_path,gx_appletlist_t *applets_list);

gxapps_ret_t gx_applets_get_usb_installlist(gx_applets_installlist_t ** list,char* install_path,gx_appletlist_t *applets_list);

void gx_applets_free_install_list(gx_applets_installlist_t* data);

int gx_applets_modify_installlist(gx_applets_installlist_t ** data,char *id,int encryption,int add_or_del);

gxapps_ret_t gx_applets_save_installlist(char* root_path,gx_applets_installlist_t *list);

int gx_applets_modify_favlist(gx_applets_favlist_t ** data,char *id,int source,int add_or_del);

gxapps_ret_t gx_applets_get_favlist(gx_applets_favlist_t ** list,gx_appletlist_t *applets_list,gx_applets_builtinlist_t * builtin_list);

void gx_applets_free_fav_list(gx_applets_favlist_t* data);

gxapps_ret_t gx_applets_save_favlist(gx_applets_favlist_t *list);

gxapps_ret_t gx_applets_get_builtinlist(char* path,gx_applets_builtinlist_t ** list);

void gx_applets_free_builtin_list(gx_applets_builtinlist_t* data);

void gx_applets_free_install_app_list(gx_applets_applist_t* data);

void gx_applets_free_install_one_app(gx_applets_app_t* data);

gxapps_ret_t gx_applets_get_one_install_app(char *installpath,gx_applets_app_t *app);

gxapps_ret_t gx_applets_get_recentlist(gx_applets_favlist_t ** list,gx_appletlist_t *applets_list,gx_applets_builtinlist_t * builtin_list);

gxapps_ret_t gx_applets_save_recentlist(gx_applets_favlist_t *list);

#endif
