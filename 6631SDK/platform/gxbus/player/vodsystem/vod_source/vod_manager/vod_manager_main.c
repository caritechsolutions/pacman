#include "../include/vod_in_common_def.h"
#include "../include/vod_manager_main.h"
#include "../include/vod_porting_all.h"
#include "gx_common.h"

#define VOD_MANAGER_MESSAGE_TIMEOUT 50

extern int32_t vod_manager_msg_init( void );
extern int32_t  vod_manager_msg_destroy(void);
extern int32_t  vod_manager_msg_recv(uint32_t timeout_ms ,int32_t* event ,int32_t* lParams ,int32_t* wParams);
extern int32_t  vod_manager_msg_send(uint32_t timeout_ms ,int32_t event ,int32_t lParams ,int32_t wParams);
extern int32_t  vod_manager_msg_reset(void);
extern int32_t vod_api_msg_init(void);
extern int32_t  vod_api_msg_destroy(void);
extern int32_t  vod_api_msg_reset(void);

vod_manager_info_t * vod_manager_info = NULL;         /*播放描述体*/
static VOD_MANAGER_MIDDLE_ACTION_MACHINE ** gPStatMachine_Acting = NULL;
extern VOD_MANAGER_MIDDLE_ACTION_MACHINE * gPStatMachine_coship_living[VOD_MANAGER_ENGIN_END];
extern VOD_MANAGER_MIDDLE_ACTION_MACHINE * gPStatMachine_coship[VOD_MANAGER_ENGIN_END];
extern VOD_MANAGER_MIDDLE_ACTION_MACHINE * gPStatMachine_wfd[VOD_MANAGER_ENGIN_END];
static uint8_t g_vod_task_exit_flag = 0;
static uint32_t g_vod_manager_task_id= 0;
static uint8_t g_vod_manager_main_readyflag = 0;

void vod_manager_set_state ( uint8_t state )
{
	if(vod_manager_info)
	{
		vod_manager_info->last_state = vod_manager_info->state;
		vod_manager_info->state = state;
		vod_manager_info->state_change_time = vod_porting_get_ms();
	}
}

uint8_t vod_manager_get_state ( void )
{
	if(vod_manager_info)
	{
		return vod_manager_info->state;
	}

	return VOD_MANAGER_ENGIN_END;
}


VOD_MANAGER_MIDDLE_ACTION_MACHINE * vod_get_gPStateMachine(void)
{
	if(gPStatMachine_Acting == NULL)
	{
		return NULL;
	}

	if(vod_manager_get_state() >= 0 && vod_manager_get_state() < VOD_MANAGER_ENGIN_END)
	{
		return gPStatMachine_Acting[vod_manager_get_state()];
	}

	return NULL;
}

int32_t vod_manager_select_machine(VOD_APP_TYPE_t apptype)
{
	int32_t err = -1;

	switch(apptype)
	{
		case VOD_APP_UNKNOWN:
			break;
		case VOD_APP_SIHUA_VOD:
			break;
		case VOD_APP_SIHUA_SHIFT:
			break;
		case VOD_APP_SIHUA_SINGLE:
			break;
		case VOD_APP_SIHUA_MP3:
			break;
		case VOD_APP_IPTV2_VOD:
			break;
		case VOD_APP_IPTV2_SINGLE:
			break;
		case VOD_APP_IPTV2_GROUP:
			break;
		case VOD_APP_IPTV2_MP3:
			break;
		case VOD_APP_MOTO_LSCP_VOD:
			break;
		case VOD_APP_MS_MMS_VOD:
			break;
		case VOD_APP_COSHIP:
			gPStatMachine_Acting = gPStatMachine_coship;
			GxVod_debug("select the vod machine for coship server\n");
			break;
		case VOD_APP_COSHIP_LIVING:
			gPStatMachine_Acting = gPStatMachine_coship_living;
			GxVod_debug("select the vod machine for coship living server\n");
			break;
		case VOD_APP_WFD:
			gPStatMachine_Acting = gPStatMachine_wfd;
			GxVod_debug("select the vod machine for coship living server\n");
			break;
		default:

			break;
	}

	return err;
}

void vod_manager_deselect_machine(void)
{
	gPStatMachine_Acting = NULL;
}

void vod_manager_main_set_readyflag ( uint8_t flag )
{
	g_vod_manager_main_readyflag = flag;
}

uint8_t vod_manager_main_get_readyflag ( void )
{
	return g_vod_manager_main_readyflag;
}

int32_t vod_manager_task_doexit(BOOLEAN needstop, BOOLEAN needdestroy)
{
	VOD_MANAGER_PFUNAction pAction = NULL ;
	VOD_MANAGER_MIDDLE_ACTION_MACHINE * pActionMachine = NULL;

	if(needstop)
	{
		pActionMachine = vod_get_gPStateMachine();
		if ( pActionMachine != NULL )
		{
			pAction = pActionMachine->pActionMachine.pAction[VOD_MANAGER_MID_MSG_STOP] ;
			if ( pAction != NULL )
			{
				pAction ( vod_manager_info , 0 );
			}
		}
	}
	vod_manager_info->state = VOD_MANAGER_ENGIN_IDEL ;
	if(needdestroy)
	{
		pActionMachine = vod_get_gPStateMachine();
		if ( pActionMachine != NULL )
		{
			pAction = pActionMachine->pActionMachine.pAction[VOD_MANAGER_MID_MSG_DESTORY] ;
			if ( pAction != NULL )
			{
				pAction ( vod_manager_info , 0 );
			}
		}
	}
	return 0;
}

int32_t vod_manager_middle_create(void)
{
	int32_t  ulReturn = ERRNO_VOD_NO_ERROR;


	/*************************初始化操作**************************/
	/* 创建总体框架对象 */
	/* add by sun 6-18 */
	vod_manager_info = (vod_manager_info_t*)xmalloc(sizeof(vod_manager_info_t));
	if( NULL ==  vod_manager_info )
	{
		GxVod_debug("%s %d VOD : create object fail\n", __FILE__, __LINE__);
		return ERR_OBJECT_FAIL ;
	}
	memset(vod_manager_info, 0, sizeof(vod_manager_info_t));

	ulReturn = vod_manager_msg_init();
	if( ulReturn != ERRNO_VOD_NO_ERROR )
	{
		GxVod_debug("%s %d VOD : vod_manager_msg_init Fail\n", __FILE__, __LINE__);
		return ERR_MSG_INIT_FAIL;
	}
	vod_manager_msg_reset();

	ulReturn = vod_api_msg_init();
	if( ulReturn != ERRNO_VOD_NO_ERROR )
	{
		GxVod_debug("%s %d VOD : vod_api_msg_init Fail\n",__FILE__, __LINE__);
		return ERR_MSG_INIT_FAIL;
	}
	vod_api_msg_reset();
	vod_manager_timer_init();

	vod_manager_set_state(VOD_MANAGER_ENGIN_IDEL);

	return 0 ;
}
int32_t vod_manager_middle_destory()
{
	int32_t  ulReturn = ERRNO_VOD_NO_ERROR;


	if(vod_manager_info)
	{
		xfree(vod_manager_info);
	}
	vod_manager_info = NULL;

	//ulReturn |= vod_decode_destroy();

	ulReturn = vod_manager_msg_destroy();
	if( ulReturn != ERRNO_VOD_NO_ERROR )
	{
		GxVod_debug("%s %d VOD : vod_manager_msg_destroy Fail\n", __FILE__, __LINE__);
		return ERR_MSG_INIT_FAIL;
	}

	ulReturn = vod_api_msg_destroy();
	if( ulReturn != ERRNO_VOD_NO_ERROR )
	{
		GxVod_debug("%s %d VOD : vod_api_msg_destroy Fail\n", __FILE__, __LINE__);
		return ERR_MSG_INIT_FAIL;
	}

	vod_manager_timer_init();

	vod_manager_set_state(VOD_MANAGER_ENGIN_IDEL);

	return 0 ;
}
/****************************************************************
 *功能 : ISMA对根据所在的状态对外部的命令处理，调用ISMA状态机制
 *输入 : 无
 *输出 : 无
 *返回 : 返回成功失败标志
 *       DH_ISMA_MID_ERROR
 *       ERRNO_VOD_NO_ERROR
 ****************************************************************
 *全局变量
 *smr:   ISMA管理框架
 ****************************************************************/
int32_t vod_manager_command_process( int32_t ulPlayCommand , int32_t ulPlayParam)
{
	int32_t ulReturn = ERRNO_VOD_NO_ERROR ;

	VOD_MANAGER_MIDDLE_ACTION_MACHINE * pActionMachine = NULL;
	VOD_MANAGER_PFUNAction pAction = NULL ;


	pActionMachine = vod_get_gPStateMachine();

	if ( ulPlayCommand < 0 || ulPlayCommand >= VOD_MANAGER_MID_MSG_END )
	{
		GxVod_debug("%s %d Isma recv error command %d\n",__FILE__, __LINE__, ulPlayCommand);
		return  ERRNO_VOD_NO_ERROR;
	}
	/******************************命令处理******************************/
	/*******************根据状态选择常态数据的执行方式*******************/
	/******************************命令处理******************************/
	if ( pActionMachine != NULL )
	{
		pAction = pActionMachine->pActionMachine.pAction[ulPlayCommand] ;
		if ( pAction != NULL )
		{
			ulReturn = pAction( vod_manager_info , ulPlayParam );
		}
		else
		{
			GxVod_debug("%s %d Isma_status: command %d\n",__FILE__, __LINE__,  ulPlayCommand );
		}
	}

	return ulReturn ;
}

int32_t vod_manager_playlist_process( int32_t ulPlayListCommand , int32_t ulPlayParam)
{
	return 0;
}

int32_t vod_manager_system_process( int32_t ulSystemCommand , int32_t ulPlayParam)
{
	return 0 ;
}

int vod_manager_msg_process( void )
{
	int32_t ulReturn = 0 ;
	static int32_t back_ulCommandType = 0 , back_ulCommandCode = 0 , back_ulCommandParam = 0;/*用来过滤过多的reopen操作*/

	int32_t ulCommandType = 0 , ulCommandCode = 0 , ulCommandParam = 0 ,
			ulCommandResponse = VOD_MANAGER_MID_MSGCODE_SUCCEED ;


	if(back_ulCommandType || back_ulCommandCode || back_ulCommandParam)
	{
		ulCommandType = back_ulCommandType;
		ulCommandCode = back_ulCommandCode;
		ulCommandParam = back_ulCommandParam;

		back_ulCommandType = 0;
		back_ulCommandCode = 0;
		back_ulCommandParam = 0;
	}
	else
	{
		while(1)
		{
			if ( vod_manager_get_state() != VOD_MANAGER_ENGIN_RUNNING  && vod_manager_get_state() != VOD_MANAGER_ENGIN_TRICKING) 
			{
				ulReturn = vod_manager_msg_recv ( VOD_MANAGER_MESSAGE_TIMEOUT , &ulCommandType , 
						&ulCommandCode , &ulCommandParam );
			}
			else
			{
				ulReturn = vod_manager_msg_recv ( 30 , &ulCommandType , 
						&ulCommandCode , &ulCommandParam );
			}
			if ( ulReturn != ERRNO_VOD_NO_ERROR )
			{
				if(back_ulCommandType || back_ulCommandCode || back_ulCommandParam)
				{
					ulCommandType = back_ulCommandType;
					ulCommandCode = back_ulCommandCode;
					ulCommandParam = back_ulCommandParam;

					back_ulCommandType = 0;
					back_ulCommandCode = 0;
					back_ulCommandParam = 0;
					break;
				}
				else
				{
					return ulReturn;
				}
			}

			if(VOD_MANAGER_MID_MSG_OPEN == ulCommandCode)
			{
				back_ulCommandType = ulCommandType;
				back_ulCommandCode = ulCommandCode;
				back_ulCommandParam = ulCommandParam;

				ulCommandType = 0;
				ulCommandCode = 0;
				ulCommandParam = 0;
			}
			else
			{
				if(back_ulCommandType || back_ulCommandCode || back_ulCommandParam)
				{
					int tmp_ulCommandType = 0 , tmp_ulCommandCode = 0 , tmp_ulCommandParam = 0;

					tmp_ulCommandType = ulCommandType;
					tmp_ulCommandCode = ulCommandCode;
					tmp_ulCommandParam = ulCommandParam;

					ulCommandType = back_ulCommandType;
					ulCommandCode = back_ulCommandCode;
					ulCommandParam = back_ulCommandParam;

					back_ulCommandType = tmp_ulCommandType;
					back_ulCommandCode = tmp_ulCommandCode;
					back_ulCommandParam = tmp_ulCommandParam;
				}
				else
				{
					//do nothing;
				}
				break;
			}
		}
	}
	/* 消息处理 */
	switch ( ulCommandType )
	{
		case VOD_MANAGER_MID_CLASS_CTRL_MSG:	/* 命令控制消息 */
			ulReturn = vod_manager_command_process( ulCommandCode ,ulCommandParam ) ;
			if ( ulReturn != ERRNO_VOD_NO_ERROR )
			{
				GxVod_debug("%s %d smr play command process failed[%d],ulCommandType=%d\n",__FILE__, __LINE__, ulReturn, ulCommandType );
				ulCommandResponse = VOD_MANAGER_MID_MSGCODE_ERROR ;
			}

			break;


		case VOD_MANAGER_MID_CLASS_PLAYLIST_MSG:	/* 播放列表消息 */
			ulReturn = vod_manager_playlist_process( ulCommandCode , ulCommandParam ) ;
			if ( ulReturn != ERRNO_VOD_NO_ERROR )
			{
				GxVod_debug("%s %d smr play command process failed[%d],ulCommandType=%d\n", __FILE__, __LINE__, ulReturn, ulCommandType );
				ulCommandResponse = VOD_MANAGER_MID_MSGCODE_ERROR ;
			}
			break;

		case VOD_MANAGER_MID_CLASS_SYSTEM_MSG:	/* 系统监听消息 */
			ulReturn = vod_manager_system_process( ulCommandCode , ulCommandParam ) ;
			if ( ulReturn != ERRNO_VOD_NO_ERROR )
			{
				GxVod_debug("%s %d smr play command process failed[%d],ulCommandType=%d\n", __FILE__, __LINE__, ulReturn, ulCommandType );
				ulCommandResponse = VOD_MANAGER_MID_MSGCODE_ERROR ;
			}
			break;
	}
	/* 返回响应 */
	/* ulCommandResponse 对应 返回操作是否成功 VOD_MANAGER_MID_MSGCODE_SUCCEED VOD_MANAGER_MID_MSGCODE_ERROR
	   ulCommandCode 对应 此次操作是何动作 VOD_MANAGER_MID_MSG_STOP
	   smr->state 对应 此次操作后，播放器转为何种状态 */
	if(ulCommandType < VOD_MANAGER_MID_CLASS_MSG_END && ulCommandType >= 0)
	{
		ulReturn = vod_manager_msg_send(20 ,ulCommandResponse, ulCommandCode , vod_manager_get_state() );
		if ( ulReturn != ERRNO_VOD_NO_ERROR )
		{
			GxVod_debug("%s %d Isma : response failed\n", __FILE__, __LINE__);
			/* 保留错误处理的桩 */
		}
	}
	return ulReturn ;
}


int32_t vod_manager_main_task ( char *url )
{
	int32_t ulReturn ;

	VOD_MANAGER_PFUNAction pAction = NULL ;
	VOD_MANAGER_MIDDLE_ACTION_MACHINE * pActionMachine = NULL;


	ulReturn = vod_manager_middle_create();
	if ( ulReturn != 0 )
	{
		GxVod_debug("%s %d VOD : create isma framwork failed\n", __FILE__, __LINE__);
		goto exit_vod_task;
	}

	vod_manager_main_set_readyflag(1);

	/*********************进入系统消息循环*************************/

	while ( g_vod_task_exit_flag == 0 )
	{
		if ( NULL != vod_manager_info )
		{
			/* 对外的消息处理 */
			ulReturn = vod_manager_msg_process();
			if ( ulReturn != ERRNO_VOD_NO_ERROR )
			{
				/*GxVod_debug("%s %d player : Message process error\n\r", __FILE__, __LINE__); */
				/* 保留状态跳转接口 */
			}
		}
		/******************************运行处理******************************/
		/*******************根据状态选择常态数据的执行方式*******************/
		/******************************运行处理******************************/
		if ( NULL != vod_manager_info)
		{
			pActionMachine = vod_get_gPStateMachine();
			if ( pActionMachine != NULL )
			{
				pAction = pActionMachine->pActionMachine.pAction[VOD_MANAGER_MID_MSG_START];
				if ( pAction != NULL )
				{
					//GxVod_debug("%s %d vod_manager_get_state()=%d\n",__FILE__, __LINE__,  vod_manager_get_state() );
					pAction ( vod_manager_info , 0 );
				}
			}
		}
	}

	GxVod_debug("\n%s %d player: exit player\n",__FILE__, __LINE__);
	vod_manager_task_doexit(1, 1);

exit_vod_task:
	g_vod_task_exit_flag = 0;

	vod_manager_middle_destory();

	vod_porting_task_exit(0);

	return 0;
}

int32_t vod_manager_init (void)
{
	int32_t err;

	GxVod_debug("\n%s %d here in vod_manager_Init\n", __FILE__, __LINE__);
	vod_manager_main_set_readyflag ( 0 );

	g_vod_task_exit_flag = 0;
	err = vod_porting_task_create( ( void (*) ( void *)) vod_manager_main_task, NULL, 128 * 1024, 9,
			&g_vod_manager_task_id, (const unsigned char*)"vod_manager_main_task");
	if (err){
		GxVod_debug("%s %d err:%x\n", __FILE__, __LINE__, err);
		return -1 ;
	}

	while(vod_manager_main_get_readyflag() == 0)
	{
		vod_porting_task_delay(10);
	}
	GxVod_debug("%s %d here exit vod_manager_Init\n", __FILE__, __LINE__);

	return 0 ;
}

void vod_manager_exit(void)
{
	int32_t err;

	if(g_vod_manager_task_id)
	{
		g_vod_task_exit_flag = 1;

		err = vod_porting_task_waitdel( g_vod_manager_task_id , 6000 );
		/*
		   在网线拔掉超过一段时间后，网络
		   发送数据超时时间比较长，这里需要等大约4.7S
		   后任务才能退出，但是并没有死索，
		   建议这里等待6S
		   */
		if ( err != ERRNO_VOD_NO_ERROR)
		{
			vod_manager_task_doexit(0, 1);
			vod_porting_task_kill( g_vod_manager_task_id );

			vod_manager_middle_destory();
		}
		vod_manager_main_set_readyflag ( 0 );
		g_vod_manager_task_id = 0;
	}

	g_vod_task_exit_flag = 0;
}

