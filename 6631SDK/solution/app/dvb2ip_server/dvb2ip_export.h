#ifndef __DVB2IP_EXPORT_H___
#define __DVB2IP_EXPORT_H___
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define DVB2IP_IPADDR_LEN   32

typedef struct {
    int *tv_array;
    int tv_num;
    int *ra_array;
    int ra_num;
    int *fm_array;
    int fm_num;
} dvb2ip_prog_list_t;

typedef struct {
    int (*ids_dump)(dvb2ip_prog_list_t *list);      // 获取节目id数组
    // 获取当前TP的节目id数组, play_num是播放的个数. 得到play_num为0时可以不用填写list
    int (*cur_ids_dump)(dvb2ip_prog_list_t *list, int *play_num);
    int (*ids_free)(dvb2ip_prog_list_t *list);      // 释放节目id数组
    int (*des_get)(int id, char *buf, int size);    // 获取节目描述字符串(节目名等)
    int (*data_read)(int pos, char *buf, int size); // 读取节目录制数据
    int (*send_start)(int id, const char *ip_addr); // 节目推送开始
    int (*send_stop)(int pos);                      // 节目推送结束

} dvb2ip_callback_t;

typedef struct {
    int server_num;                 // 文件服务器监听数目
    int stream_num;                 // 码流推送服务器监听数目
    char ip_addr[32];               // ip地址
    unsigned short server_port;     // 文件服务器监听端口
    unsigned short stream_port;     // 码流推送服务器监听端口
    int stream_prio;                // 码流推送服务器线程优先级，取负值优先级变高
    int send_cache_size;            // 录制数据的缓冲大小
    int send_min_size;              // 一次调用send的最小发送大小
    int send_max_size;              // 一次调用send的最大发送大小
    int read_min_size;              // 一次从demux或fifo读取最小大小
    const char *vir_play_file;      // 客户端请求此字符串发送当前播放TP的所有prog ids
    const char *dev_name;           // 客户端发现设备显示的设备名称
} dvb2ip_info_t;

typedef struct {
    dvb2ip_info_t info;
    dvb2ip_callback_t prog_cb;
    dvb2ip_callback_t fm_prog_cb;
} dvb2ip_start_t;

extern int s_http_printf_level;     // 0, 关闭；1-5, 打印级别(errno, error, info, warn, debug)
int dvb2ip_service_start(dvb2ip_start_t *para);
int dvb2ip_service_stop(void);
int dvb2ip_service_reset(void);
int dvb2ip_service_run_check(void);
int dvb2ip_stream_buffer_reset(void);
#endif

