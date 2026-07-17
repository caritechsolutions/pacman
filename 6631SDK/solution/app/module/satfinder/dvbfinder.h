#ifndef __DVBFINDER_H__
#define __DVBFINDER_H__

typedef void (*dvbfinder_qr_callback_t)(char *apk,char *server);

typedef struct
{
    char type;
    char name[32];
}dvbfinder_channel_t;

typedef enum{
	GXMSG_HTTP_DOWNLOAD=0,
	GXMSG_SCAN_FIND_DOWNLOAD,
	GXMSG_HTTP_DOWNLOAD_END
}GxSatFindType;

typedef enum{
	DVB_SAT_FINDE_SERVER_MODE=1133,
	DVB_SAT_FINDE_APK_MODE=1144
}DvbSatFindMode;

typedef struct{
	int para1;
	GxSatFindType type;
	char *pic_patch;
}dream_ui_msg_t;


int dream_dvbfinder_init(void);

int dream_dvbfinder_start(dvbfinder_qr_callback_t cb);

int dream_dvbfinder_stop(void);

int dream_dvbfinder_reconnect(void);

/*****************************************************************************************/
void dvbfinder_get_sn(char *sn,int len);

int dvbfinder_rand(int max);

int dvbfinder_task_create(char *name,void(*entry_func)(void *), void *arg,int stack_size);

void dvbfinder_sleep(unsigned int ms);

unsigned int dvbfinder_get_tick(void);

int dvbfinder_atoi(char *s);

int dvbfinder_sprintf(char *d,char *fmt,...);

void *dvbfinder_malloc(int size);

void dvbfinder_free(void *p);

int dvbfinder_memcmp(void *s,void *d,int l);

void *dvbfinder_memcpy(void *s,void *d,int l);

void *dvbfinder_memset(void *s,int d,int l);

char *dvbfinder_strchr(char *s,char c);

char *dvbfinder_strrchr(char *s,char c);

int dvbfinder_strlen(char *s);

char *dvbfinder_strcpy(char *s,char *d);

char *dvbfinder_strncpy(char *s,char *d,int l);

int dvbfinder_http_sync_download(char *url,char *buf,unsigned int *size);

int dvbfinder_conn_init(char *s,int p,int lp);

void dvbfinder_conn_send(char *buf,int len,int fd);

int dvbfinder_conn_recv(char *buf,int len,int *curr_fd);

int dvbfinder_search_tp(void);

int dvbfinder_get_channels(dvbfinder_channel_t *ch,int max);

void dvbfinder_set_tuner(char pol,char k22,char port,unsigned short frq,unsigned short sym);

int dvbfinder_get_lock_signal(char *l,char *s,char *q);

int dvbfinder_set_tp(unsigned short frq,unsigned short sym,char pol);

int dvbfinder_get_tp(unsigned short *frq,unsigned short *sym,char *pol);

int dvbfinder_set_antenna(unsigned short lnb1,unsigned short lnb2,char k22,char disqec);

int dvbfinder_get_antenna(unsigned short *lnb1,unsigned short *lnb2,char *k22,char *disqec);

extern int db_sat_init(void);

extern int db_sat_release(void);

#endif
