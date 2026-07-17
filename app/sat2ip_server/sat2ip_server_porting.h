#ifndef __SATIP_SERVER_PORTING_H__
#define __SATIP_SERVER_PORTING_H__
#include "gxtype.h"
#include <errno.h>
#include <assert.h>

#define SATIP_IPADDR_LEN                32

extern int s_satip_printf_en;

//#define SAT2IP_DEBUG

#ifdef SAT2IP_DEBUG
#define SATIP_ENUM(fmt, args...)  do {                              \
	printf("[SATIP_ENUM][%s:%d]: ", __func__, __LINE__);            \
	printf("[errno = %d:%s]: ", errno, strerror(errno));            \
	printf(fmt, ##args);                                            \
} while(0)

#define SATIP_ERR(fmt, args...)  do {                               \
	printf("[SATIP_ERR][%s:%d]: ", __func__, __LINE__);             \
	printf(fmt, ##args);                                            \
} while(0)

#define SATIP_INFO(fmt, args...)  do {                              \
	printf("[SATIP_INFO][%s:%d]: ", __func__, __LINE__);            \
	printf(fmt, ##args);                                            \
} while(0)

#define SATIP_DBG(fmt, args...)  do {                               \
	if (s_satip_printf_en) {                                        \
		printf("[SATIP_DBG][%s:%d]: ", __func__, __LINE__);         \
		printf(fmt, ##args);                                        \
	}                                                               \
} while(0)
#else
#define SATIP_ENUM(fmt, args...)     {;}
#define SATIP_ERR(fmt, args...)      {;}
#define SATIP_INFO(fmt, args...)     {;}
#define SATIP_DBG(fmt, args...)      {;}
#endif

#define SATIP_MEM_FREE(ptr) do { if (ptr) GxCore_Free(ptr); ptr = NULL; } while(0)
#define SATIP_FD_CLOSE(fd)  do { if (fd >= 0) close(fd); fd = -1; } while(0)
#define SATIP_FP_CLOSE(fp)  do { if (fp != NULL) fclose(fp); fp = NULL; } while(0)

typedef struct {
	char ip_addr[SATIP_IPADDR_LEN];     // 服务端的ip地址
	uint8_t build_flag;                 // 是否强制重建文件
	uint8_t start_ok;                   // 是否成功启动
} SatipStartMsg;

typedef struct {
	int max_http_server;
	int max_rtsp_stream;
	int max_http_stream;
	int max_data_cnt;
	int (*lock_state_get)(int tuner, int *lock, int *quality);
	int (*start_play_send)(int prog_id);
	int (*stop_play_send)(int prog_id);
	int (*record_read)(uint8_t *buf, int size);

} SatipCallback;

int satip_clear_cache_files(void);
int satip_proglist_total_num_get(void);
int app_satip_task(void);
int app_satip_ctrl_init(SatipCallback *callback);
int app_satip_service_start(SatipStartMsg *msg);
int app_satip_service_stop(int file_rm_flag);
int app_satip_dialog_start(void);
int app_satip_dialog_stop(void);
int app_satip_dialog_pause(int sec);
int app_satip_dialog_resume(void);
int app_satip_multi_client_flag_change(int flag);
int app_satip_dms_rtsp_use_change(int flag);
int app_satip_data_prio_change(int flag);
int app_satip_fast_close_set(int flag);
int app_satip_task_state_get(void);
char *app_satip_get_prog_url(int prog_id);
#endif

