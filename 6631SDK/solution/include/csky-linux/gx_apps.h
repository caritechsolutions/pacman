#ifndef _GX_APPS_H
#define _GX_APPS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gx_apps_type.h>
#include <curl/curl.h>

typedef enum gx_curl_httpfunc_e
{
	GX_CURL_HTTP_GET = 0,
	GX_CURL_HTTP_POST,
	GX_CURL_HTTP_DELETE,
	GX_CURL_HTTP_MAX,
} gx_curl_httpfunc_t;

typedef enum gx_curl_state_e
{
	GXCURL_STATE_START = 0,
	GXCURL_STATE_PROCESS,
	GXCURL_STATE_COMPLETE,
}gx_curl_state_t;

typedef struct gx_curl_data_s
{
	char *buffer;
	unsigned int used_size;
	unsigned int buffer_size;
	unsigned int max_size;
}gx_curl_data_t;

typedef struct gx_curl_http_options_s
{
	struct curl_slist *header;//http header
	char *poststr; //http post content
	char *user_agent; //http user agent
	char *cookies; //http cookies
	char *doh_url; //secure dns url
	char *ssl_cipher; //ssl cipher list
	int debug;
}gx_curl_http_options_t;

typedef struct gx_curl_start_s {
	unsigned int reserved;
}gx_curl_start_t;

typedef struct gx_curl_process_s {
	unsigned int content_length;
	unsigned int download_size;
}gx_curl_process_t;

typedef struct gx_curl_complete_s {
	gxapps_ret_t retcode;
	char *buffer;
	unsigned int size;
}gx_curl_complete_t;

typedef struct gxcurl_callback_data_s {
	unsigned int handle;
	unsigned int index;
	union{
		gx_curl_start_t  start_data;
		gx_curl_process_t process_data;
		gx_curl_complete_t complete_data;
	};
}gxcurl_callback_data_t;

typedef void(* gxapps_callback)(int, int);

typedef struct gx_item_s
{
	char *key;
	char *value;
	char *type;
}gx_item_t;

typedef struct gx_list_s {
	int item_num;
	gx_item_t *items;
}gx_list_t;

typedef struct gx_location_s
{
	char *ipAddress;
	char *countryCode;
	char *countryName;
	char *regionName;
	char *cityName;
	char *latitude;
	char *longitude;
	char *timeZone;
}gx_location_t;

typedef struct gx_net_time_s
{
    int tm_sec;      // seconds after the minute - [0..59]
    int tm_min;      // minutes after the hour - [0..59]
    int tm_hour;     // hours since midnight - [0..23]
    int tm_mday;     // days of the month - [1..31]
    int tm_mon;      // months since January - [0..11]
    int tm_year;     // years since 1900
}gx_net_time_t;

unsigned int gx_os_thread_create(void*(*entry_func)(void *), void *arg);

void  gx_os_thread_sleep(unsigned int ms);

unsigned int gx_os_mutex_create(void);

void gx_os_mutex_delete(unsigned int mutex_id);

void gx_os_mutex_lock(unsigned int mutex_id);

void gx_os_mutex_unlock(unsigned int mutex_id);

unsigned int gx_get_tick_time(void);


//url functions
char *gx_url_encode(char const *s, int len, int *new_length);

int gx_url_decode(char *str, int len);

void gx_time_itoa(unsigned int seconds, char *timestr);

void gxapps_strncpy(char *dest, const char * source, int len);

char* gx_strndup(const char* src, int len);

char* gx_strrstr(char *buffer, const char *str, size_t length);

void gx_strrev(char *s);

char *gx_strtok_r(char *s, const char *delim, char **save_ptr);

void gx_hex2str(const unsigned char *src,  char *dest, int srclen);

unsigned char *gx_str2hex(unsigned char* str);

unsigned int gx_crc32(const unsigned char *buffer, int len);

char *gx_base64_encode(const unsigned char *src, int srclen);

void gx_base64_decode(const char *src, unsigned char **outptr, int *outlen);

char *gx_str_enc(char *src, const char *key);

char *gx_str_dec(char *src, const char *key);

char *gx_md5_sign(const unsigned char *src,  int srclen);

char *gx_md5_sign_file(const char *path);

char *gx_hmac_sha_sign(const unsigned char *src,  int srclen, const unsigned char* key, unsigned int keylen);

char *gx_hmac_sha256_sign(const unsigned char *src,  int srclen, const unsigned char* key, unsigned int keylen);

char *gx_sha_sign(const unsigned char *src,  int srclen);

char *gx_sha256_sign(const unsigned char *src,  int srclen);

void gx_aes_cbc_encrypt(const unsigned char* key, unsigned int keylen, const unsigned char* iv, const unsigned char *in, unsigned char* out, unsigned int sz);

void gx_aes_cbc_decrypt(const unsigned char* key, unsigned int keylen, const unsigned char* iv, const unsigned char *in, unsigned char* out, unsigned int sz);

void gx_des_ecb_encrypt(const unsigned char* key, const unsigned char *in, unsigned char* out, unsigned int sz);

void gx_des_ecb_decrypt(const unsigned char* key, const unsigned char *in, unsigned char* out, unsigned int sz);

void gx_des_cbc_encrypt(const unsigned char* key, const unsigned char* iv, const unsigned char *in, unsigned char* out, unsigned int sz);

void gx_des_cbc_decrypt(const unsigned char* key, const unsigned char* iv, const unsigned char *in, unsigned char* out, unsigned int sz);

void gxapps_curl_set_timeout(int connect_timeout, int low_speed_timeout);

void gxapps_curl_enable_compress(int enable);

void gxapps_curl_set_proxy(const char *url, int proxy_type, const char *username, const char *password);

gxapps_ret_t gxapps_curl_download(const char *url, gx_curl_data_t *curldata);

void gxapps_curl_free(gx_curl_data_t *curldata);

gxapps_ret_t gxapps_curl_httpfunc(const char *url,
								gx_curl_httpfunc_t func,
								gx_curl_http_options_t *options,
								gx_curl_data_t *curldata);

unsigned int gxapps_curl_async_download(
								const char *url,
								const char *path, //save path, set to NULL if download to mem
								gxapps_callback callback);

void gxapps_curl_async_abort(unsigned int handle);

unsigned int gxapps_curl_async_download_ext(
								const char *url,
								gx_curl_http_options_t *options,
								const char *path,
								const char *buffer,
								unsigned int buffer_size,
								gxapps_callback callback);

unsigned int gxapps_curl_multi_async_download(
								const char **urls,
								int url_num,
								gx_curl_http_options_t *options,
								const char **paths,
								gxapps_callback callback);

gx_list_t *gx_list_new(void);

void gx_list_add_item(gx_list_t *list, char *key, char *value, char *type);

void gx_list_edit_item(gx_list_t *list, int index, char *key, char *value, char *type);

void gx_list_delete_item(gx_list_t *list, int index);

gx_list_t *gx_get_list_from_file(char *path);

gx_list_t *gx_search_list_type(gx_list_t *list, char *keywords);

gx_list_t *gx_list_get_mode(gx_list_t *list);

gx_list_t *gx_list_get_mode_method(gx_list_t *list, char *mode);

gx_list_t *gx_list_get_method(gx_list_t *list);

void gx_list_free(gx_list_t *list);

void gx_merge_list(gx_list_t *dstlist, gx_list_t *srclist);

void gx_list_save_to_file(
					gx_list_t  *list,
					char *path
					);

gxapps_ret_t gx_net_get_location(gx_location_t ** location);

void gx_net_free_location(gx_location_t * location);

//failed, return NULL; success, return a pointer,you must free it yourself
gx_net_time_t *gx_net_get_time(void);

//global functions
void gxapps_init(char *platform_ver, char *app_ver);

void gxapps_customer_init(char *name, char *model, char *sign);

void gxapps_service_start(void);

void gxapps_service_stop(void);


#endif
