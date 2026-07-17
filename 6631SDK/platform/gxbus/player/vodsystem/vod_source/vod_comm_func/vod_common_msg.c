#include "../include/vod_in_common_def.h"
#include "../include/vod_porting_all.h"
#include "../include/vod_manager_main.h"
#include "gx_common.h"


#define SYS_TIMEOUT_IMMEDIATE 0x00000000
static uint32_t vod_manager_msg_id = 0;
static uint32_t vod_api_msg_id = 0;

/****************************************************************
  vod manager模块的msg队列初始化
 ****************************************************************/
int32_t vod_manager_msg_init( void )
{
	int32_t ret = 0;

	if (0 == vod_manager_msg_id)
	{
		ret = vod_porting_msgq_create(sizeof(VodMsg_t), 50, &vod_manager_msg_id);
	}

	GxVod_debug("%s %d  msgid %d\n", __FUNCTION__, __LINE__, vod_manager_msg_id);
	return ret;
}

/****************************************************************
  vod manager模块的msg队列接收
 ****************************************************************/
int32_t  vod_manager_msg_recv(uint32_t timeout_ms ,int32_t * event ,int32_t * lParams ,int32_t * wParams )
{
	int32_t err;
	VodMsg_t msg = {0};

	if (0 == vod_manager_msg_id)
	{
		return -1;
	}

	err = vod_porting_msgq_wait(vod_manager_msg_id, (void*)&msg, sizeof(VodMsg_t), timeout_ms);

	if(err==0)
	{
		*event = msg.ulCommand;
		*lParams = msg.ulmsg1;
		*wParams = msg.ulmsg2;
		GxVod_debug("%s %d  msgid %d [%d ][%d][%d]\n", __FUNCTION__, __LINE__, vod_manager_msg_id, *event, *lParams, *wParams);
	}

	return err;
}

/****************************************************************
  vod manager模块的msg队列发送，发送到api层
 ****************************************************************/
int32_t  vod_manager_msg_send(uint32_t timeout_ms ,int32_t event ,int32_t lParams ,int32_t wParams )
{
	int32_t err;
	VodMsg_t msg = {0};

	if (0 == vod_api_msg_id)
	{
		return -1;
	}

	msg.ulCommand = event;
	msg.ulmsg1 = lParams;
	msg.ulmsg2 = wParams;

	err = vod_porting_msgq_send(vod_api_msg_id, (void*)&msg, sizeof(VodMsg_t), timeout_ms);
	if(err == 0){
		GxVod_debug("%s %d  msgid %d [%d ][%d][%d]\n", __FUNCTION__, __LINE__, vod_api_msg_id, event, lParams, wParams);
	}

	return err;
}

/****************************************************************
  vod manager模块的msg队列清空
 ****************************************************************/
int32_t  vod_manager_msg_reset(void)
{
	int32_t err;

	if (0 == vod_manager_msg_id)
	{
		return -1;
	}

	err = vod_porting_msgq_reset(vod_manager_msg_id);

	return err;
}

/****************************************************************
  vod manager模块的msg队列删除
 ****************************************************************/
int32_t  vod_manager_msg_destroy(void)
{
	if (vod_manager_msg_id)
	{
		vod_porting_msgq_delete(vod_manager_msg_id);
	}

	vod_manager_msg_id = 0;

	return 0;
}


/****************************************************************
  vod api 模块的msg队列初始化
 ****************************************************************/
int32_t vod_api_msg_init( void )
{
	int32_t ret = 0;

	if (0 == vod_api_msg_id)
	{
		ret = vod_porting_msgq_create(sizeof(VodMsg_t), 50, &vod_api_msg_id);
	}

	return ret;
}

/****************************************************************
  vod api模块的msg队列接收
 ****************************************************************/
int32_t  vod_api_msg_recv(uint32_t timeout_ms ,int32_t * event ,int32_t * lParams ,int32_t * wParams )
{
	int32_t err;
	VodMsg_t msg = {0};

	if (0 == vod_api_msg_id)
	{
		return -1;
	}

	err = vod_porting_msgq_wait(vod_api_msg_id, (void*)&msg, sizeof(VodMsg_t), timeout_ms);

	if(err==0)
	{
		*event = msg.ulCommand;
		*lParams = msg.ulmsg1;
		*wParams = msg.ulmsg2;
		GxVod_debug("%s %d  msgid %d [%d ][%d][%d]\n", __FUNCTION__, __LINE__, vod_api_msg_id, *event, *lParams, *wParams);
	}

	return err;
}

/****************************************************************
  vod api模块的msg队列发送，发送到vod manager
 ****************************************************************/
int32_t  vod_api_msg_send(uint32_t timeout_ms ,int32_t event ,int32_t lParams ,int32_t wParams )
{
	int32_t err;
	VodMsg_t msg = {0};

	if (0 == vod_manager_msg_id)
	{
		return -1;
	}

	msg.ulCommand = event;
	msg.ulmsg1 = lParams;
	msg.ulmsg2 = wParams;

	err = vod_porting_msgq_send(vod_manager_msg_id, (void*)&msg, sizeof(VodMsg_t), timeout_ms);
	if(err == 0){
		GxVod_debug("%s %d  msgid %d [%d ][%d][%d]\n", __FUNCTION__, __LINE__, vod_manager_msg_id, event, lParams, wParams);
	}

	return err;
}

/****************************************************************
  vod api模块的msg队列清空
 ****************************************************************/
int32_t  vod_api_msg_reset(void)
{
	int32_t err;

	if (0 == vod_api_msg_id)
	{
		return -1;
	}

	err = vod_porting_msgq_reset(vod_api_msg_id);

	return err;
}
/****************************************************************
  vod api模块的msg队列删除
 ****************************************************************/

int32_t  vod_api_msg_destroy(void)
{
	if (vod_api_msg_id)
	{
		vod_porting_msgq_delete(vod_api_msg_id);
	}

	vod_api_msg_id = 0;

	return 0;
}


