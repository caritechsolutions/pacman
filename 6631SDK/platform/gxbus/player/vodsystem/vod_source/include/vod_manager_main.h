
#ifndef __VOD_MANAGER_MAIN_H
#define __VOD_MANAGER_MAIN_H

#include "vod_in_common_def.h"
#include "vod_in_typedef.h"

typedef  struct  tagVodMsg_e
{
	uint32_t ulCommand;
	uint32_t ulmsg1;
	uint32_t ulmsg2;
	uint32_t used;
}VodMsg_t;

typedef  struct
{
	uint8_t url[2048];                //当前进入点播或者时移地url
	uint8_t url_add[2048];            //进入点播和时移地第二个url，目前用途在上海电信的iptv项目，直播url＆时移url分开的情况
	int32_t  apptype;                //选择vodmachine的业务应用类型
	int32_t  regionid;               //cable线中获取到的regionid
	int32_t  init_scale;             //进入时移或者点播初始状态的scale，允许以快退或者快进方式直接进入时移
	uint8_t init_pause;               //进入时移时，是否通过暂停dvb方式进入
	uint8_t enter_place;              //进入时移或者点播前一种状态，0 为浏览器  1 为 dvb,此标志位跟vod本身没有关系
	uint32_t srmip;			//used for moto dmx srm ip address
	uint16_t srmport;			//srm ip port number
	uint32_t gateway;		//bmi ip address
	uint16_t groupid;			// stb group id
	char assertid[21];		// program assert id
	uint32_t startnpt;
	int io_type;
	int service_type;
	char multicast_ip[16];
	int multicast_port;
	int64_t start_position;    //进入点播相对于起始时间的位置
	int64_t reference_time;        //进入时移起始UTC时间
	char hostip[64];
	int  hostport;
}
vod_start_param_t;



/* 消息返回代码 */
enum
{
	VOD_MANAGER_MID_MSGCODE_SUCCEED = 200 , /* 成功消息 */

	VOD_MANAGER_MID_MSGCODE_ERROR = 100, /* 失败消息 400 */

	VOD_MANAGER_MID_MSGCODE_END
};

/*
 *描述了ISMA中间件的消息分类
 */
enum
{
	VOD_MANAGER_MID_CLASS_CTRL_MSG = 0x00 ,/* 
											  播放控制消息：
											  对播放中的媒体进行一系列的控制处理，
											  该消息仅仅给出命令指示，不附带命令参数 
											  */

	VOD_MANAGER_MID_CLASS_PLAYLIST_MSG  ,  /* 对内容控制消息：
											  表示对播放列表的控制或者其它需求，
											  后续版本将给予明确，该消息需要消息体，
											  消息体的定义需要特殊说明
											  */

	VOD_MANAGER_MID_CLASS_SYSTEM_MSG  ,    /* 系统消息：
											  主要表明系统管理类消息，例如心跳信息 
											  */

	VOD_MANAGER_MID_CLASS_MSG_END   
};

/*
 *系统消息类型
 */
enum
{
	VOD_MANAGER_MID_SYSTEM_PING = 0x00 ,

	VOD_MANAGER_MID_SYSTEM_END 
};

/*
 *描述了ISMA中间件所经历的状态
 */
typedef enum
{
	VOD_MANAGER_ENGIN_IDEL = 0X00,			/* 空闲 */
	VOD_MANAGER_ENGIN_PREROLL ,		/* 缓存 */
	VOD_MANAGER_ENGIN_RUNNING ,		/* 正常运行*/
	VOD_MANAGER_ENGIN_TRICKING ,		/* 快进快退 */
	VOD_MANAGER_ENGIN_PAUSED ,		/* 暂停 */
	VOD_MANAGER_ENGIN_BUFFERING ,
	VOD_MANAGER_ENGIN_END
}VOD_MANAGER_STATUS_t;


/*
 *描述了ISMA中间件所接收的控制命令操作
 */
enum
{
	VOD_MANAGER_MID_MSG_OPEN  ,			/* 打开 */
	VOD_MANAGER_MID_MSG_START   ,		/* 启动*/
	VOD_MANAGER_MID_MSG_STOP    ,		/* 停止*/
	VOD_MANAGER_MID_MSG_PAUSE  ,		/* 暂停 */
	VOD_MANAGER_MID_MSG_SEEK  ,			/* 拖动 */
	VOD_MANAGER_MID_MSG_RESTART ,		/* 唤醒 */
	VOD_MANAGER_MID_MSG_TRICK  ,			/* 快进快退*/
	VOD_MANAGER_MID_MSG_BUFFERING, 		/* 播放过程中缓存数据*/
	VOD_MANAGER_MID_MSG_GET_PARAMETER ,	/* 时移状态下，通过OPTION 获取时间*/
	VOD_MANAGER_MID_MSG_DESTORY ,		/* 销毁退出*/

	VOD_MANAGER_MID_MSG_END   
};

typedef struct vod_manager_info_t_
{
	uint8_t   state;
	uint8_t   last_state;
	uint32_t media_start_time;
	int32_t audio_volume;
	uint8_t mute;
	uint8_t buffering;
	uint32_t state_change_time;
	uint32_t v_pts ;
	uint32_t a_pts ;

	uint8_t last_action;
	uint32_t last_pts;
	uint32_t old_time;
	int32_t justseeked;
	char service_type;

	vod_start_param_t start_param;
}vod_manager_info_t;


enum{
	m_start,
	m_seek,
	m_pause,
	m_restart,
	m_stop,

	m_end
};



/*
 *状态操作原子定义
 */
typedef  int32_t ( * VOD_MANAGER_PFUNAction ) ( vod_manager_info_t *, int32_t );

/*
 *状态机制定义
 */
typedef struct tag_vod_act_mackine
{
	union
	{
		VOD_MANAGER_PFUNAction pAction[VOD_MANAGER_MID_MSG_END];

		struct
		{
			VOD_MANAGER_PFUNAction pActionOpen;
			VOD_MANAGER_PFUNAction pActionStart;
			VOD_MANAGER_PFUNAction pActionStop;
			VOD_MANAGER_PFUNAction pActionPause;
			VOD_MANAGER_PFUNAction pActionSeek;
			VOD_MANAGER_PFUNAction pActionResume;
			VOD_MANAGER_PFUNAction pActionTrick;
			VOD_MANAGER_PFUNAction pActionBuffing; /* 新增加*/
			VOD_MANAGER_PFUNAction pActionGetparameter;
			VOD_MANAGER_PFUNAction pActionDestory;
		}pActionFun;
	}pActionMachine;
}VOD_MANAGER_MIDDLE_ACTION_MACHINE;

/*定时器类型*/
enum
{
	VOD_MANAGER_TIMER_NONE = 0x00 ,
	VOD_MANAGER_TIMER_ALWAYS ,
	VOD_MANAGER_TIMER_OPTION ,
	VOD_MANAGER_TIMER_IPQAM_OPTION ,

	VOD_MANAGER_TIMER_GET_PARAMETER,

	VOD_MANAGER_TIMER_AUTO_SEEK,

	VOD_MANAGER_TIMER_EXIT ,

	VOD_MANAGER_TIMER_KIND_END
};

enum
{
	VOD_MANAGER_TIMER_UNUSED = 0X00 ,
	VOD_MANAGER_TIMER_USED ,

	VOD_MANAGER_TIMER_END 
};

enum
{
	VOD_MANAGER_TIMER_MOD_UNLOOP = 0X00 ,
	VOD_MANAGER_TIMER_MOD_LOOP ,

	VOD_MANAGER_TIMER_MOD_END 
};

/*退出类型*/
enum
{
	VOD_MANAGER_EXIT_CODE_NONE =0X00 ,
	VOD_MANAGER_EXIT_CODE_NORMAL ,
	VOD_MANAGER_EXIT_CODE_DSPFAILE ,
	VOD_MANAGER_EXIT_CODE_NETFAIL ,
	VOD_MANAGER_EXIT_CODE_BYE ,

	VOD_MANAGER_EXIT_CODE_END 
};


void vod_manager_timer_init(void);
int32_t vod_manager_timer_set(int interval , int timer , int used , int flag );
int32_t vod_manager_timer_tigger(void);
void vod_manager_set_state(uint8_t state);
uint8_t vod_manager_get_state ( void );
#endif

