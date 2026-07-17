/*--------------------------------------------------------

	COPYRIGHT 2008 (C) Hangzhou Motorola Technology Ltd

	AUTHOR:		wangry@dvnchina.com
	PURPOSE:	coship vod lib
	CREATED:	2008-7-7

	MODIFICATION HISTORY
	Date        By     Details
	----------  -----  -------

--------------------------------------------------------*/
#ifndef _COSHIP_RTSP_H_
#define _COSHIP_RTSP_H_ 


/*----------------------------------------------------------
	本系统函数返回值规则:
	0 为成功
	非0 为失败类型

	该模块只负责控制信息的获取和发送, 为上面策略层提供工具
----------------------------------------------------------*/
#define VERSION "RTSP FOR COSHIP 1.0"

/*------------------------------------------------------------------------------
回调函数的类型

RTSP_EVENT_CLOSE: 服务器主动发的退出消息
RTSP_EVENT_BOS  : 服务器主动发的BOS消息 
RTSP_EVENT_EOS  : 服务器主动发的EOS消息

RTSP_EVENT_BLOCK: 命令阻塞时间过长时, 每0.5产生一个回调, 可以在这里进行界面的进度控制
改回调是调用这个命令的线程产生的, 不是异步线程产生的

RTSP_EVENT_SOCK_ERR:		socket err
RTSP_EVENT_SOCK_TIMEOUT:	socket timeout
RTSP_EVENT_INVALID_RESP:	resp invalid
RTSP_EVENT_MEMORY_ERR:		memory err
这些回调是在异步线程中产生的, 要注意与主线程竞争等问题
回调函数中严禁执行rtsp函数, 涉及锁嵌套等问题
------------------------------------------------------------------------------*/
#define RTSP_EVENT_CLOSE			1
#define RTSP_EVENT_EOS				2
#define RTSP_EVENT_BOS				3
#define RTSP_EVENT_BLOCK			4
#define RTSP_EVENT_SOCK_ERR		5
#define RTSP_EVENT_SOCK_TIMEOUT	6
#define RTSP_EVENT_INVALID_RESP	7
#define RTSP_EVENT_MEMORY_ERR		8
#define RTSP_EVENT_SERVER_CLOSE	9
#define RTSP_EVENT_ERROR_CODE	   10

typedef void (*rtsp_callback_func_t)(int event, int errcode);

typedef struct 
{
	unsigned int start_time;	/*影片开始时间*/
	int		duration;			/*影片长度*/
	int 	position;			/*影片当前位置*/
	int		trickmode;			/*是否可以快进快退, 0表示不行, 2, 4, 8等表示最大倍速*/
	int		program_no;		/*ts的service id*/
	int		videopid;			/*ts流里的video pid*/
	int		audiopid;			/*ts流里的audio pid*/
	int		pcrpid;				/*ts流里的pcr pid*/
	int		freq;				/*ipqam的freq*/
	int		symbrate;			/*ipqam的symbol rate*/
	int		qamsize;			/*ipqam的qam size*/
	char	media_ip[16];		/*iptv的ip*/
	int		media_port;		/*iptv的port*/
	char	keystring[25];		/*iptv的认证字符*/
	char 	session[25];		/*rtsp session*/
	int   error_count;      /*rtsp 出现错误的个数*/
}coship_rtsp_info_t;

/*------------------------------------------------------------------------------
打开媒体, io_type设置服务器按指定的方式输出数据, 1=cable, 2=tcp, 3=udp multicast
------------------------------------------------------------------------------*/
int coship_rtsp_open(char* url, char* hip, int hport, int io_type, int timeout, rtsp_callback_func_t func);

/*------------------------------------------------------------------------------
播放
range用于各种方式的播放位置
rebuild用于tcp断掉后的续播
scale用于trick播放时的步长
例如:
启动播放  Ragne: npt=123-100
定时播放  Range: npt=123-456; time=20051223T153600Z
直播      Range: Clock=20060329T175412Z
直播定时  Range: npt=123-456; time=20051223T153600Z	
直播片段  Range: Clock=20060329T175412Z-20060329T175412Z
------------------------------------------------------------------------------*/
int	coship_rtsp_play(char* range, int rebuild_pos, int scale);

/*------------------------------------------------------------------------------
用于暂停
------------------------------------------------------------------------------*/
int	coship_rtsp_pause();

/*------------------------------------------------------------------------------
获取当前播放位置, 同时应用需要至少60秒调一次,以维持服务器联系
------------------------------------------------------------------------------*/
int coship_rtsp_get_position(int* position);

/*------------------------------------------------------------------------------
关闭rtsp
该函数释放所有资源
------------------------------------------------------------------------------*/
void coship_rtsp_close();

/*------------------------------------------------------------------------------
设置所有rtsp命令中timeout值, 表示rtsp交互命令最长多少时间返回, ms
------------------------------------------------------------------------------*/
void coship_rtsp_set_timeout(int timeout);

/*------------------------------------------------------------------------------
得到服务器信息
------------------------------------------------------------------------------*/
coship_rtsp_info_t* coship_rtsp_get_info();

/*------------------------------------------------------------------------------
返回当前lib的版本信息, 用于库和头文件的版本校对
------------------------------------------------------------------------------*/
char* coship_rtsp_version();

/*------------------------------------------------------------------------------
为配合上层应用, 添加的一些取信息的接口, 与coship_rtsp_get_info冗余了
------------------------------------------------------------------------------*/
int coship_get_trick_mode();
unsigned int coship_get_start_time();
int coship_get_current_pos();
int coship_get_duration();

#endif

