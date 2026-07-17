/*****************************************************************************
* 			  CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009-2010, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_player_manage.c
* Author    : 	shenbin
* Project   :	GoXceed -S
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2010.4.15	           shenbin         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "app_module.h"
#include "app_send_msg.h"
#include "gxplayer.h"

#define _DUMMY	(-1)
#define _NORMAL	(0)
#define _PIP		(1)
#define _REC		(2)
#define _LOGO		(3)
#define _TIMESHIFT	(4)
#define _TOTAL	(5)

const char * player_usage[_TOTAL] = {PLAYER_FOR_NORMAL,
						PLAYER_FOR_PIP,
						PLAYER_FOR_REC,
						PLAYER_FOR_JPEG,
						PLAYER_FOR_TIMESHIFT};

// 3110 support
#define _ACCESS	(3)
#define _OPS		(3)

const int8_t player_logic[_ACCESS][_OPS] ={
		{_NORMAL, _REC, _DUMMY},
		{_NORMAL, _LOGO, _DUMMY},
		{_NORMAL, _TIMESHIFT, _DUMMY}
};


int8_t player_logic_record[_OPS] = {_DUMMY,_DUMMY,_DUMMY};

// 从第一步开始，Normal play
int8_t player_ops_step = 0;

/* Private functions ------------------------------------------------------ */
/* normal play only can open once, call close "app_player_close" only stop player, not decrease count
 normal play  只能打开一次，打开后调用"app_player_close" 只是关闭play
 再次播放时不用再调用"app_player_open"，其他任何player调用"app_player_close"
 后再次播放必须调用"app_player_open"，返回PLAYER_OPEN_OK才可使用*/
// must check the return value.
AppPlayerStatus app_player_open(char* player_name)
{
	uint32_t player_find = 0;
	uint32_t player_ops;
	uint32_t i;

	// 大画面播放默认成功，如果第一次播放，将player_ops_step自加
	if (strcmp(PLAYER_FOR_NORMAL, player_name) == 0)
	{
		if (player_ops_step == 0)
		{
			// 记录第一个操作状态为播放普通视频
			player_logic_record[player_ops_step] = _NORMAL;

			player_ops_step++;
		}

		return PLAYER_OPEN_OK;
	}

	// check the player name exist, normal default ok, so find from the second "pip"
	for (player_ops=_PIP; player_ops<_TOTAL; player_ops++)
	{
		if (strcmp(player_usage[player_ops],player_name) == 0)
		{
			player_find = 1;
			break;
		}
	}

	if (player_find == 0) {
		return PLAYER_NAME_ERROR;
	}

	// 记录当前在access的第几个ops step上
	player_logic_record[player_ops_step] = player_ops;
	player_ops_step++;

	for (i=0; i<_ACCESS; i++)
	{
		if (memcmp(&(player_logic[i][0]), player_logic_record, player_ops_step) == 0)
		{
			return PLAYER_OPEN_OK;
		}
	}

	// 还原当前不可打开player的ops
	player_ops_step--;
	player_logic_record[player_ops_step] = _DUMMY;

	return PLAYER_OPS_FORBID;
}

// after player_close, only normal play needn't call player_open again
AppPlayerStatus app_player_close(char* player_name)
{
	uint32_t player_ops;
	uint8_t player_find = 0;
	int8_t player_record = 0;
	GxMsgProperty_PlayerStop player_stop;

	if (strcmp(PLAYER_FOR_NORMAL,player_name) == 0)
	{
		player_stop.player = player_name;

		app_send_msg_exec(GXMSG_PLAYER_STOP, &player_stop);

		// TODO: FOR EPG CONTROL
		return PLAYER_CLOSE_OK;
	}

	// 查询当前player是第几个
	for (player_ops=0; player_ops<_TOTAL; player_ops++)
	{
		if (strcmp(player_usage[player_ops],player_name) == 0)
		{
			player_record = player_ops;
			player_find = 1;
		}
	}

	if (player_find == 0) {
		return PLAYER_NAME_ERROR;
	}

	// 当前查询到的player是否在逻辑链中
	for (player_ops=0; player_ops<_OPS; player_ops++)
	{
		if (player_logic_record[player_ops] == player_record)
		{
			// 当前player关闭，把后续的logic往前移动一个位置
			for (;player_ops<player_ops_step-1; player_ops++)
			{
				player_logic_record[player_ops] = player_logic_record[player_ops+1];
			}

			player_ops_step--;
			//logic最后一个位置置为_DUMMY，形成新的logic_record
			player_logic_record[_OPS-1] = _DUMMY;

			if (player_ops == _TIMESHIFT) {
				// time shift ust the same player with normal play, needn't stop player
				return PLAYER_CLOSE_OK;
			}

			// close player
			player_stop.player = player_name;
			app_send_msg_exec(GXMSG_PLAYER_STOP, &player_stop);

			return PLAYER_CLOSE_OK;
		}
	}

	return PLAYER_NAME_ERROR;
}

/* Exported functions ----------------------------------------------------- */

/* End of file -------------------------------------------------------------*/


