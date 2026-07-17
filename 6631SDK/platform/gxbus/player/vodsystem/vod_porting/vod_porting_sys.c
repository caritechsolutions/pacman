#include "gx_common.h"
#include "gx_mediafilter.h"
#include "gx_fifo.h"

/*************************************************************
  说明：
  申请内存
  定义：
  int vod_porting_heap_init ( unsigned int * pHandle,  void * pStart, unsigned int heapsize )
  参数：
pHandle:堆栈句柄返回地址

返回：
0：成功
非0：失败.
 **************************************************************/
int vod_porting_heap_init ( unsigned int * pHandle,  void * pStart, unsigned int heapsize )
{
	return 0;
}


/*************************************************************
  说明：
  申请内存
  定义：
  int vod_porting_heap_term ( unsigned int * pHandle)
  参数：
pHandle:堆栈句柄
返回：
0：成功
非0：失败.
 **************************************************************/
int vod_porting_heap_term ( unsigned int * pHandle)
{
	return 0;
}

/*************************************************************
  说明：
  申请内存
  定义：
  int vod_porting_mem_malloc (unsigned int len)
  参数：
Len:申请内存大小
返回：
0：成功
非0：失败.
 **************************************************************/
void * vod_porting_mem_malloc (unsigned int heaphandle, unsigned int len)
{
	return av_malloc(len);
}

/*************************************************************
  说明：
  释放内存
  定义：
  int vod_porting_mem_free (void * pindex)
  参数：
Len:申请内存大小
返回：
0：成功
非0：失败.
 **************************************************************/
void vod_porting_mem_free (unsigned int heaphandle, void * pindex)
{
	av_free(pindex);
}

/*************************************************************
  说明：
  创建消息队列
  定义：
  int vod_porting_msgq_create (unsigned int msgsize, unsigned int msgmaxnum, unsigned int *msgid)
  参数：
Msgsize:消息结构体大小
Msgmaxnum：消息队列最大个数
Msgid：消息队列句柄
返回：
0：成功
非0：失败.
 **************************************************************/
int vod_porting_msgq_create (unsigned int msgsize, unsigned int msgmaxnum, unsigned int *msgid)
{
	GxFifo* fifo = NULL;

	fifo = GxFifo_Create(msgsize*msgmaxnum, GX_PINFLAG_SW);
	if(fifo == NULL){
		return -1;
	}
	*msgid = (unsigned int)fifo;
	return 0;
}


/*************************************************************
  说明：
  接收消息队列
  定义：
  int vod_porting_msgq_wait(unsigned int msgid, void * recvmsg, int size, unsigned int timeoutms);
  参数：
Msgid:消息队列句柄
Recvmsg：存放接收消息的地址
timeoutms：等待接收超时时间
返回：
0：成功
非0：失败.
 **************************************************************/
int vod_porting_msgq_wait(unsigned int msgid, void * recvmsg, int size, unsigned int timeoutms)
{
	int count = 0;
	GxFifo* fifo = (GxFifo*)msgid;

	if(timeoutms <= 0){
		timeoutms = 0;
		count = 1;
	}else{
		count = timeoutms/10+1;
		count = (count<2)?2:count;
	}

	while(count){
		if(GxFifo_GetLength(fifo) >= size){
			GxFifo_Read(fifo, (unsigned char *)recvmsg, (unsigned int)size, 0);
			break;
		}
		count--;
		if(count > 0)
			GxCore_ThreadDelay(10);
	}

	if(count == 0){
		return -1;
	}
	return 0;
}


int vod_porting_msgq_reset(unsigned int msgid)
{
	GxFifo* fifo = (GxFifo*)msgid;

	GxFifo_Reset(fifo);
	return 0;
}

/*************************************************************
  说明：
  发送消息
  定义：
  int vod_porting_msgq_send(unsigned int msgid, void * sendmsg, int size, unsigned int timeoutms);
  参数：
Msgid:消息队列句柄
sendmsg：发送的消息结构体地址
timeoutms：等待发送超时时间
返回：
0：成功
非0：失败.
 **************************************************************/
int vod_porting_msgq_send(unsigned int msgid, void * sendmsg, int size, unsigned int timeoutms)
{
	int len = 0;
	GxFifo* fifo = (GxFifo*)msgid;

	len = GxFifo_Write(fifo, (unsigned char *)sendmsg, (unsigned int)size, 0);
	if(len != size){
		return -1;
	}
	return 0;
}

/*************************************************************
  说明：
  删除消息队列
  定义：
  int vod_porting_ msgq_delete(unsigned int msgid);
  参数：
Msgid:消息队列句柄
返回：
0：成功
非0：失败.
 **************************************************************/
int vod_porting_msgq_delete(unsigned int msgid)
{
	int err;
	GxFifo* fifo = (GxFifo*)msgid;

	err = GxFifo_Destroy(fifo);

	return err;
}


/*************************************************************
  说明：
  信号量创建
  定义：
  int vod_porting_sem_create(unsigned int initvalue, unsigned int * semid);
  参数：
Initvalue: 信号量的初始值
Semid：信号两句柄
返回：
0：成功
非0：失败.
 **************************************************************/
int vod_porting_sem_create(unsigned int initvalue, unsigned int * semid)
{
	int err;

	err = GxCore_SemCreate((handle_t*)semid, initvalue);

	return err;
}


/*************************************************************
  说明：
  等待信号量
  定义：
  int vod_porting_ sem_wait(unsigned int semid, unsigned int timeoutms);
  参数：
  Semid：信号两句柄
Timeoutms: 等待超时时间，单位毫秒
返回：
0：成功
非0：失败.
 **************************************************************/
int vod_porting_sem_wait(unsigned int semid, unsigned int timeoutms)
{
	int err;

	err = GxCore_SemTimedWait((handle_t)semid, timeoutms);

	return err;
}


/*************************************************************
  说明：
  释放信号量
  定义：
  int vod_porting_ sem_send(unsigned int semid);
  参数：
  Semid：信号两句柄
  返回：
  0：成功
  非0：失败.
 **************************************************************/
int vod_porting_sem_send(unsigned int semid)
{
	int err;

	err = GxCore_SemPost((handle_t)semid);

	return err;
}



/*************************************************************
  说明：
  删除信号量
  定义：
  int vod_porting_sem_delete(unsigned int semid);
  参数：
  Semid：信号两句柄
  返回：
  0：成功
  非0：失败.
 **************************************************************/
int vod_porting_sem_delete(unsigned int semid)
{
	int err;

	err = GxCore_SemDelete((handle_t)semid);

	return err;
}



/*************************************************************
  说明：
  创建一个任务
  定义：
  int vod_porting_task_create( void ( * Function ) ( void * ),
  void* Param,
  unsigned int StackSize,
  unsigned int Priority,
  unsigned int* Task_ID,
  const unsigned char* Name,
  )

  参数：
  Function：任务函数
  Param：传入任务的参数
  Stacksize：任务堆栈大小
  Priority：任务优先级
Task_id: 任务句柄
返回：
0：成功
非0：失败.
 **************************************************************/
int vod_porting_task_create( void ( * Function ) ( void * ),
		void* Param,
		unsigned int StackSize,
		unsigned int Priority,
		unsigned int* Task_ID,
		const unsigned char* Name
		)
{
	int err;

	err = GxCore_ThreadCreate((const char*)Name, (handle_t*)Task_ID, Function,Param, StackSize, Priority);

	return err;
}



/*************************************************************
  说明：
  强制杀任务
  定义：
  int vod_porting_task_kill(unsigned int taskid);
  参数：
Tasked: 任务句柄
返回：
0：成功
非0：失败.
 **************************************************************/
int vod_porting_task_kill(unsigned int taskid)
{
	int err;

	err = GxCore_ThreadJoin((handle_t)taskid);

	return err;
}


/*************************************************************
  说明：
  退出任务
  定义：
  void vod_porting_ task_exit();
  参数：

  返回：
  0：成功
  非0：失败.
 **************************************************************/
void vod_porting_task_exit(int exitcode)
{
	return;
}



/*************************************************************
  说明：
  任务挂起
  定义：
  int vod_porting_task_suspend(int taskid);
  参数：
Tasked: 任务句柄
返回：
0：成功
非0：失败.
 **************************************************************/
int vod_porting_task_suspend(int taskid)
{
	return 0;
}


/*************************************************************
  说明：
  任务恢复
  定义：
  int vod_porting_ task_resume(int taskid);
  参数：
Tasked: 任务句柄
返回：
0：成功
非0：失败.
 **************************************************************/
int vod_porting_task_resume(int taskid)
{
	return 0;
}


/*************************************************************
  说明：
  等待任务退出
  定义：
  int vod_porting_task_waitdel (int taskid, unsigned int timeoutms);
  参数：
Tasked: 任务句柄
Timeoutms: 等待超时时间，单位毫秒
返回：
0：成功
非0：失败.
 **************************************************************/
int vod_porting_task_waitdel (unsigned int taskid, unsigned int timeoutms)
{
	int err;

	err = GxCore_ThreadJoin((handle_t)taskid);

	return err;
}
/*************************************************************
  说明：
  等待任务退出
  定义：
  void vod_porting_task_delay (unsigned int delayms);
  参数：
delayms: 等待超时时间，单位毫秒
返回：
0：成功
非0：失败.
 **************************************************************/
void vod_porting_task_delay (unsigned int delayms)
{
	GxCore_ThreadDelay(delayms);
}


/*************************************************************
  说明：
  获取系统当前时间，毫秒数
  定义：
  unsigned vod_porting_get_ms (void);
  参数：

  返回：
  系统当前毫秒数.
 **************************************************************/
unsigned int vod_porting_get_ms (void)
{
	struct timeval tv;
	//  float s;
	gettimeofday(&tv, NULL);
	//  s=tv.tv_usec;s*=0.000001;s+=tv.tv_sec;
	return (uint32_t)(tv.tv_sec*  1000 + tv.tv_usec / 1000);
}

/*************************************************************
  说明：
  获取系统ip地址
  定义：
  const char * vod_porting_get_ipaddr (void);
  参数：

  返回：
  字符串形式返回ip地址. 格式为如 10.7.1.105
 **************************************************************/
const char * vod_porting_get_ipaddr (void)
{
	static char _vod_ipaddr_str[16] = "0";


	return _vod_ipaddr_str;
}
/*************************************************************
  说明：
  获取系统ip地址
  定义：
  const char * vod_porting_get_longipaddr (void);
  参数：

  返回：
  字符串形式返回ip地址. 格式为如 10.7.1.105
 **************************************************************/
unsigned int  vod_porting_get_longipaddr (void)
{
	unsigned int addr = (10<<24)|(7<< 16)|(1<<8)|105;
	return addr;
}


/*************************************************************
  说明：
  获取系统mac地址
  定义：
  const char * vod_porting_get_mac (void);
  参数：

  返回：
  字符串形式返回mac地址.格式为xx:xx:xx:xx:xx:xx
 **************************************************************/
const char * vod_porting_get_mac (void)
{
	static char _vod_mac_str[18] = "0";


	return _vod_mac_str;
}

static int SYS_GetTimeByMJD_MY( unsigned short int usMJD, unsigned int *y, unsigned char *m, unsigned char *d )
{
	unsigned int Y1, Y;
	unsigned char M1, D, M, K;
	int temp;

	/*	计算公式
		Y1 = int[(MJD - 15078.2)/365.25];
		M1 = int[(MJD - 14956.1 - int(Y1*365.25))/30.6001];
		D = MJD - 14956 - int(Y1*365.25) - int(M1*30.6001);
		if( M1 == 14 || M1 == 15 )
			K = 1;
		else
			K = 0;
		Y = Y1 + K;
		M = M1 - 1 - K*12;
	*/
	Y1 = ( usMJD * 100 - 1507820 ) / 36525;
	temp = usMJD * 10 - 149561 - ( ( int ) ( Y1 * 365 + Y1 / 4 ) ) * 10;
	M1 = ( temp * 1000 ) / 306001;
	D = usMJD - 14956 - ( int ) ( Y1 * 365 + Y1 / 4 ) - ( int ) ( M1 * 30 + ( M1 * 6001 ) / 10000 );
	if ( M1 == 14 || M1 == 15 )
	{
		K = 1;
	}
	else
	{
		K = 0;
	}
	Y = Y1 + K;
	M = M1 - 1 - K * 12;
	*y = Y + 2000;
	*m = M;
	*d = D;
	return 0;
}

static unsigned short int SYS_GetMJD_MY ( int year, int month, int day )
{
	unsigned short	uMJD = 0;
	int	L, temp;

	/*	计算公式
		if( M == 1 || M == 2 )
		L = 1;
		else
		L = 0;
		MJD = 14956 + D + int[(Y - L)*365.25] + int[(M + 1 + L*12)*30.6001];
		*/
	if ( month == 1 || month == 2 )
	{
		L = 1;
	}
	else
	{
		L = 0;
	}
	uMJD = 14956 + day;

	temp = year - 2000 - L;
	uMJD += ( int ) ( temp * 365 + temp / 4 );

	temp = month + 1 + L * 12;
	uMJD += ( int ) ( temp * 30 + ( temp * 6001 ) / 10000 );
	return uMJD;

}

/*************************************************************
  说明：
  转化时间字符串range clock形式到npt形式
  定义：
  int vod_porting_mktime_clock_to_npt(unsigned char * timestr, unsigned int * npt)
  参数：
  timestr? 输入的时间字符串,格式为如20080515T010101.00Z
  返回：
  0 成功
  非0  失败
 **************************************************************/
int vod_porting_mktime_clock_to_npt(unsigned char * timestr, unsigned int * npt)
{
	char tmpstr[6] = {0};
	int year, month, day, hour, minute, second;
	unsigned short int mjd;

	if(timestr == NULL)
	{
		return -1;
	}
	if(npt == NULL)
	{
		return -1;
	}
	if(strlen((char*)timestr) < 16)
	{
		return -2;
	}
	memset(tmpstr, 0, sizeof(tmpstr));
	memcpy(tmpstr, timestr, 4);
	year = (int)strtoul(tmpstr, NULL, 10);

	memset(tmpstr, 0, sizeof(tmpstr));
	memcpy(tmpstr, &timestr[4], 2);
	month = (int)strtoul(tmpstr, NULL, 10);

	memset(tmpstr, 0, sizeof(tmpstr));
	memcpy(tmpstr, &timestr[6], 2);
	day = (int)strtoul(tmpstr, NULL, 10);

	memset(tmpstr, 0, sizeof(tmpstr));
	memcpy(tmpstr, &timestr[9], 2);
	hour = (int)strtoul(tmpstr, NULL, 10);

	memset(tmpstr, 0, sizeof(tmpstr));
	memcpy(tmpstr, &timestr[11], 2);
	minute = (int)strtoul(tmpstr, NULL, 10);

	memset(tmpstr, 0, sizeof(tmpstr));
	memcpy(tmpstr, &timestr[13], 2);
	second = (int)strtoul(tmpstr, NULL, 10);

	mjd = SYS_GetMJD_MY(year, month, day);
	mjd = mjd;

	*npt = mjd*24*3600 + hour*3600 + minute*60 + second;
	return 0;
}

/*************************************************************
  说明：
  转化npt形式到clock形式
  定义：
  int vod_porting_mktime_npt_to_clock(unsigned int npt, unsigned char * timestr)
  参数：
npt:npt时间
timestr: 输出格式为 20080515T010101.00Z
返回：
0 成功
非0  失败
 **************************************************************/
int vod_porting_mktime_npt_to_clock(unsigned int npt, unsigned char * timestr)
{
	int mjd, year, hour, minute, second;
	unsigned char month, day;
	if(timestr == NULL)
	{
		return -1;
	}
	mjd = npt / (24*3600);
	mjd = mjd;

	SYS_GetTimeByMJD_MY(mjd, (unsigned int*)&year, (unsigned char*)&month, (unsigned char*)&day);

	hour = (npt / 3600) % 24;
	minute = (npt / 60) % 60;
	second = npt % 60;

	memset(timestr, 0, sizeof(timestr));
	sprintf((char*)timestr, "%4d%02d%02dT%02d%02d%02d.00Z", year, month, day, hour, minute, second);
	return 0;
}

/*************************************************************
  说明：
  转化时间字符串range clock形式到npt形式
  定义：
  int vod_porting_mktime_showtime_to_npt(unsigned char * timestr, unsigned int * npt)
  参数：
  timestr? 输入的时间字符串,格式为如2008-05-15 08:34:22
  返回：
  0 成功
  非0  失败
 **************************************************************/
int vod_porting_mktime_showtime_to_npt(unsigned char * timestr, unsigned int * npt)
{
	return 0;
}

/*************************************************************
  说明：
  转化npt形式到clock形式
  定义：
  int vod_porting_mktime_npt_to_showtime(unsigned int npt, unsigned char * timestr)
  参数：
npt:npt时间
timestr: 输出格式为 2008-05-15 08:34:22
返回：
0 成功
非0  失败
 **************************************************************/
int vod_porting_mktime_npt_to_showtime(unsigned int npt, unsigned char * timestr)
{
	return 0;
}

/*************************************************************
  说明：
  转化时间字符串range clock形式到npt形式
  定义：
  int vod_porting_mktime_timestr_to_npt(unsigned char * timestr, unsigned int * npt)
  参数：
  timestr? 输入的时间字符串,格式为如2008051501010100
  返回：
  0 成功
  非0  失败
 **************************************************************/

int vod_porting_mktime_timestr_to_npt(char * timestr, unsigned int * npt)
{
	return 0;
}

/*************************************************************
  说明：
  转化npt形式到clock形式
  定义：
  int vod_porting_mktime_npt_to_timestr(unsigned int npt, unsigned char * timestr)
  参数：
npt:npt时间
timestr: 输出格式为 2008051501010100Z
返回：
0 成功
非0  失败
 **************************************************************/

int vod_porting_mktime_npt_to_timestr(unsigned int npt, char * timestr)
{
	return 0;
}

