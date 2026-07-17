#ifndef _VOD_PORTING_ALL_H_
#define _VOD_PORTING_ALL_H_

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
int vod_porting_heap_init ( unsigned int * pHandle,  void * pStart, unsigned int heapsize );
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
int vod_porting_heap_term ( unsigned int * pHandle);
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
void * vod_porting_mem_malloc (unsigned int heaphandle, unsigned int len);
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
void vod_porting_mem_free (unsigned int heaphandle, void * pindex);
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
int vod_porting_msgq_create (unsigned int msgsize, unsigned int msgmaxnum, unsigned int *msgid);


/*************************************************************
说明：
接收消息队列
定义：
int vod_porting_msgq_wait(unsigned int msgid, void * recvmsg, int size,  unsigned int timeoutms);
参数：
Msgid:消息队列句柄
Recvmsg：存放接收消息的地址
timeoutms：等待接收超时时间
返回：
0：成功
非0：失败.
**************************************************************/
int vod_porting_msgq_wait(unsigned int msgid, void * recvmsg, int size, unsigned int timeoutms);


int vod_porting_msgq_reset(unsigned int msgid);
/*************************************************************
说明：
发送消息
定义：
int vod_porting_msgq_send(unsigned int msgid, void * sendmsg,int size, unsigned int timeoutms);
参数：
Msgid:消息队列句柄
sendmsg：发送的消息结构体地址
timeoutms：等待发送超时时间
返回：
0：成功
非0：失败.
**************************************************************/
int vod_porting_msgq_send(unsigned int msgid, void * sendmsg, int size, unsigned int timeoutms);


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
int vod_porting_msgq_delete(unsigned int msgid);
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
int vod_porting_sem_create(unsigned int initvalue, unsigned int * semid);

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
int vod_porting_sem_wait(unsigned int semid, unsigned int timeoutms);


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
int vod_porting_sem_send(unsigned int semid);


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
int vod_porting_sem_delete(unsigned int semid);



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
                           );


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
int vod_porting_task_kill(unsigned int taskid);

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
void vod_porting_task_exit(int exitcode);
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
int vod_porting_task_suspend(int taskid);


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
int vod_porting_task_resume(int taskid);


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
int vod_porting_task_waitdel (unsigned int taskid, unsigned int timeoutms);
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
void vod_porting_task_delay (unsigned int delayms);
/*************************************************************
说明：
获取系统当前时间，毫秒数
定义：
unsigned vod_porting_get_ms (void);
参数：

返回：
系统当前毫秒数.
**************************************************************/
unsigned int vod_porting_get_ms (void);

/*************************************************************
说明：
获取系统ip地址
定义：
const char * vod_porting_get_ipaddr (void);
参数：

返回：
字符串形式返回ip地址. 格式为如 10.7.1.105
**************************************************************/
const char * vod_porting_get_ipaddr (void);

/*************************************************************
说明：
获取系统mac地址
定义：
const char * vod_porting_get_mac (void);
参数：

返回：
字符串形式返回mac地址.格式为xx:xx:xx:xx:xx:xx
**************************************************************/
const char * vod_porting_get_mac (void);


/*************************************************
说明：
连接服务器socket
定义：
int vod_porting_socket_open (int ip, int port, int type)
参数：
Ip: 服务器ip地址
Port：服务器端口
Type：socket类型
返回：
<0：失败
>=0: 成功.
**************************************************/

/*************************************************
说明：
连接服务器socket
定义：
int vod_porting_tcp_socket_create ( void )
参数：
domain: AF_INET or AF_UNSPEC
type：SOCK_STREAM or SOCK_DGRAM
protocol：TCP or UDP 0 for default
返回：
<0：失败
>=0: 成功.
**************************************************/
int vod_porting_tcp_socket_create(void);
/*************************************************
说明：
连接服务器socket
定义：
int vod_porting_udp_socket_create ( void )
参数：
domain: AF_INET or AF_UNSPEC
type：SOCK_STREAM or SOCK_DGRAM
protocol：TCP or UDP 0 for default
返回：
<0：失败
>=0: 成功.
**************************************************/
int vod_porting_udp_socket_create(void);

/***************************************************
说明:
TCP用于连接SOCKET
parameter:
sock:socket id
name:server param:ip & port
namelen: sizeof(*name)
return:
>=0:ok
< 0:fail
***************************************************/
//int vod_porting_socket_connect(int sock, struct sockaddr * name, int namelen)
int vod_porting_socket_connect(int sock, unsigned short sport, unsigned long saddr);

/*************************************************
说明：
查询socket状态
定义：
int vod_porting_socket_select (int nfds, myfd_set *readfds, myfd_set *writefds, 
							myfd_set *exceptfds, struct timeval_t *timeout)
参数：
参见LINUX下select
返回：
等同于标准函数select.
**************************************************/
int vod_porting_socket_select (int sock, unsigned int timeout_ms);
/*************************************************
说明：
Socket接收数据
定义：
int vod_porting_socket_recv (int socketid, void * recvbuf, int recvlen)
参数：
Socketid : 句柄
Recvbuf：数据存放地址
Recvlen：数据接收最大长度
返回：
Recv数据长度.
**************************************************/
int vod_porting_socket_recv (int sock, void * recvbuf, int recvlen);
/*************************************************
说明：
Socket发送数据
定义：
int vod_porting_socket_send (int socketid, void * sendbuf, int sendlen)
参数：
Socketid : 句柄
sendbuf：数据发送地址
sendlen：数据发送长度
返回：
Send成功数据长度.
**************************************************/
int vod_porting_socket_send (int sock, void * sendbuf, int sendlen);

/*************************************************
说明：
关闭socket
定义：
int vod_porting_ socket_close (int socketid)
参数：
Socketid : 句柄
返回：
等同于标准函数close
**************************************************/
int vod_porting_socket_close (int socketid);
/*************************************************
说明：
关闭socket
定义：
int vod_porting_server_name_to_addr(char * servername)
参数：
Socketid : 句柄
返回：
等同于标准函数close
**************************************************/
int vod_porting_socket_bind(int sock, unsigned long addr, unsigned short port);
int vod_porting_socket_join_multicast(int sock, unsigned long local_addr, unsigned long mcast_addr);
int vod_porting_socket_leave_multicast(int sock, unsigned long local_addr, unsigned long  mcast_addr);
/*************************************************************
说明：
定义：
const char * vod_porting_inet_addr (void);
参数：

返回：
**************************************************************/
unsigned long vod_porting_inet_addr ( char *dotted );
unsigned long vod_porting_server_name_to_addr(char * servername);
int vod_porting_socket_sendto(int sock, unsigned short port, unsigned long addr,void * sendbuf, int sendlen);
int vod_porting_socket_recvfrom (int sock, unsigned short *port, unsigned long* addr,void * recvbuf, int recvlen);
unsigned int vod_porting_htonl(unsigned int data);
unsigned short vod_porting_htons(unsigned short data);
unsigned int vod_porting_ntohs(unsigned int invalue);
unsigned int vod_porting_ntohl(unsigned int invalue);
void vod_porting_event_seturl(const char* player, const char* src_url, void* cb);
int vod_porting_event_notify(int eventid, int errorcode);
void vod_porting_get_arecode(char* areaCode);

int vod_porting_dvb_play_by_pid_sync(
			unsigned int frequency,
			unsigned int symborate,
			unsigned int qam_mode,
			unsigned int u32VideoPid,
			unsigned int u32AudioPid,
			unsigned int u32PcrPid,
			unsigned int u32EmmPid,
			unsigned int u32AudioEcmPid,
			unsigned int u32VideoEcmPid
			);
int vod_porting_dvb_play_by_pid(
			unsigned int frequency,
			unsigned int symborate,
			unsigned int qam_mode,
			unsigned int u32VideoPid,
			unsigned int u32AudioPid,
			unsigned int u32PcrPid,
			unsigned int u32EmmPid,
			unsigned int u32AudioEcmPid,
			unsigned int u32VideoEcmPid
			);

int vod_porting_dvb_play_by_pmtpid_sync(
		unsigned int frequency,
		unsigned int symborate,
		unsigned int qam_mode,
		unsigned int pmtpid
		);

int vod_porting_dvb_play_by_pmtpid(
		unsigned int frequency,
		unsigned int symborate,
		unsigned int qam_mode,
		unsigned int pmtpid
		);

int vod_porting_dvb_play_by_serviceid_sync(
		unsigned int frequency,
		unsigned int symborate,
		unsigned int qam_mode,
		unsigned short serviceid
		);

int vod_porting_dvb_play_by_serviceid(
		unsigned int frequency,
		unsigned int symborate,
		unsigned int qam_mode,
		unsigned short serviceid
		);

int vod_porting_dvb_stop(int needclear);
int vod_porting_dvb_pause(void);
int vod_porting_dvb_resume(void);
#endif


