/************************************************************
Copyright (C), 2014, GX S&T Co., Ltd.
FileName   :	gxtkgs_location_manager.c
Author     :
Version    : 	1.0
Date       :
Description:
Version    :
History    :
Date                Author      Modification
2014.06.11     liqg         create
***********************************************************/

/* Includes --------------------------------------------------------------- */
#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "sys_setting_manage.h"
#include "app_utility.h"
#include "app_config.h"
#include "gxcore.h"
#include "tkgs_protocol.h"
#include "gxtkgs.h"
#if TKGS_SUPPORT


#define FILE_MODE  "w+"


#define GX_TKGS_LOCATION_FILE_NAME "/home/gx/tkgs_location_db"

/*初始化标记*/
//static uint32_t gx_tkgs_location_init_flag = 0;

/*互斥量句柄*/
static handle_t gx_tkgs_location_mutex_handle = -1;

/**
 * @brief 		获取保存的location列表，包括visible和invisible location
 * @param   	void
 * @return   	GXCORE_SUCCESS:执行正常
 				GXCORE_ERROR:执行失败
 */
status_t GxBus_TkgsLocationGet(GxTkgsLocation* location)
{
	size_t real_size = 0;
	GxTkgsLocation sys_info = {0};
	handle_t location_file = 0;

    if(gx_tkgs_location_mutex_handle == -1)
    {
        GxCore_MutexCreate(&gx_tkgs_location_mutex_handle);
    }

    GxCore_MutexLock(gx_tkgs_location_mutex_handle);
    if(location == NULL)
    {
		GxCore_MutexUnlock(gx_tkgs_location_mutex_handle);
		return GXCORE_ERROR;
    }

	location_file = GxCore_Open(GX_TKGS_LOCATION_FILE_NAME, "r");
	if(location_file<0)
	{
		GxCore_MutexUnlock(gx_tkgs_location_mutex_handle);
		return GXCORE_ERROR;
	}

	real_size = GxCore_Read(location_file, location, 1,sizeof(GxTkgsLocation));
	if(real_size != sizeof(GxTkgsLocation))
	{
		memset(&sys_info,0,sizeof(GxTkgsLocation));
		sys_info.visible_num = 0;
		sys_info.invisible_num = 0;
		GxCore_Write(location_file, &sys_info, 1,sizeof(GxTkgsLocation));
		memcpy(location,&sys_info,sizeof(GxTkgsLocation));
	}
	GxCore_Close(location_file);
	GxCore_MutexUnlock(gx_tkgs_location_mutex_handle);
	return GXCORE_SUCCESS;
}


/**
 * @brief 		保存的location
 * @param   	void
 * @return   	GXCORE_SUCCESS:执行正常
 				GXCORE_ERROR:执行失败
 */
status_t  GxBus_TkgsLocationSet(GxTkgsLocation* location)
{
	handle_t location_file = 0;

    if(location == NULL)
    {
		return GXCORE_ERROR;
    }

    if(gx_tkgs_location_mutex_handle == -1)
    {
        GxCore_MutexCreate(&gx_tkgs_location_mutex_handle);
    }

	GxCore_MutexLock(gx_tkgs_location_mutex_handle);
	location_file = GxCore_Open(GX_TKGS_LOCATION_FILE_NAME, FILE_MODE);
	if(location_file<0)
	{
		GxCore_MutexUnlock(gx_tkgs_location_mutex_handle);
		return GXCORE_ERROR;
	}

	GxCore_Write(location_file, location, 1,sizeof(GxTkgsLocation));
	GxCore_Close(location_file);
	GxCore_MutexUnlock(gx_tkgs_location_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 		检查参数频点是否是location 频点
 * @param   	void
 * @return   	GXCORE_SUCCESS:执行正常
 				GXCORE_ERROR:执行失败
 */
status_t  GxBus_TkgsLocationCheckByTp(GxTkgsTpCheckParm *parm)
{
	GxTkgsLocation tkgs_location = {0};
	int i;

	if (parm == NULL)
	{
		return GXCORE_ERROR;
	}

	if (GxBus_TkgsLocationGet(&tkgs_location) != GXCORE_SUCCESS)
	{
		parm->resault = TKGS_LOCATION_NOFOUNE;
		//get location error
		return GXCORE_ERROR;
	}

	for (i=0; i<tkgs_location.visible_num; i++)
	{
		if (parm->frequency == tkgs_location.visible_location[i].frequency)
		{
			// found in visible location
			parm->resault = TKGS_LOCATION_FOUND;
			parm->pid = tkgs_location.visible_location[i].pid;
			return GXCORE_SUCCESS;
		}
	}

	for (i=0; i<tkgs_location.invisible_num; i++)
	{
		if (parm->frequency == tkgs_location.invisible_location[i].frequency)
		{
			// found in visible location
			parm->resault = TKGS_LOCATION_FOUND;
			parm->pid = tkgs_location.invisible_location[i].pid;
			return GXCORE_SUCCESS;
		}
	}
	parm->resault = TKGS_LOCATION_NOFOUNE;
	return GXCORE_SUCCESS;
}

/**
 * @brief 		检查参数PMT是否包含TKGS component descriptor
 * @param   	void
 * @return   	GXCORE_SUCCESS:执行正常
 				GXCORE_ERROR:执行失败
 */
status_t  GxBus_TkgsLocationCheckByPmt(GxTkgsPmtCheckParm *parm)
{
	int32_t section_length = 0;
	int32_t loop1Len = 0;
	int32_t loop2Len = 0;
	int32_t loop2DescLen = 0;
	int32_t tempLen = 0;
	uint8_t* p1 = NULL;
	uint8_t* p2 = NULL;
	uint8_t* temp_p = NULL;
	uint16_t elementary_pid = 0;

	if (parm == NULL)
	{
		return GXCORE_ERROR;
	}

	parm->resault = TKGS_LOCATION_NOFOUNE;

	p1 = parm->pPmtSec;
	section_length = (((p1[1] & 0x0f) << 8 ) | p1[2]) + 3;
	loop1Len = ((p1[10] & 0x0f) << 8 ) | p1[11];
	p1+= 12;
	loop2Len = section_length - 12 - loop1Len - 4;
	p2 = parm->pPmtSec + 12 + loop1Len;

	while(loop1Len >= 7)
    {
        if ((p1[0] == TKGS_COMPONENT_TAG) && ((p1[6] & 0x80) == 0x80)
			&& (0 == strncmp((const char*)(p1+2), "TKGS", 4)))
        {
			parm->resault = TKGS_LOCATION_FOUND;
			//found TKGS COMPONENT TAG
		}
		loop1Len -= (2+p1[1]);
		p1 += (2+p1[1]);
	}

	while(loop2Len > 5)
    {
    	elementary_pid = ((p2[1]<<8)|p2[2]) & 0x1fff;
    	loop2DescLen = ((p2[3] & 0x0f) << 8 ) |p2[4];

		temp_p = p2+5;
		tempLen = loop2DescLen;
		while (tempLen >= 7)
		{

	        if ((temp_p[0] == TKGS_COMPONENT_TAG) && ((temp_p[6]&0x80) == 0x80)
				&& (0 == strncmp((const char*)&temp_p[2], "TKGS", 4)))
	        {
				parm->resault = TKGS_LOCATION_FOUND;
				parm->pid = elementary_pid;
				return GXCORE_SUCCESS;
				//found TKGS COMPONENT TAG
			}
			tempLen -= (2+temp_p[1]);
			temp_p += (2+temp_p[1]);
		}

		loop2Len -= (5+loop2DescLen);
		p2 += (5+loop2DescLen);
	}

	return GXCORE_SUCCESS;

}


/**
 * @brief 		获取所有的location  ，按优先级顺序排列
 * @param   	void
 * @return   	GXCORE_SUCCESS:执行正常
 				GXCORE_ERROR:执行失败
 */
status_t gx_TkgsGetLocationInOrder(uint32_t *num, GxTkgsLocationParm *locationTp, uint16_t *pid)
{
	GxTkgsLocation location;
	int i;

	if (GXCORE_ERROR == GxBus_TkgsLocationGet(&location))
	{
		return GXCORE_ERROR;
	}

	for (i=0; i<location.invisible_num; i++)
	{
		locationTp[i] = location.invisible_location[i];
		pid[i] = location.invisible_location[i].pid;
	}

	for (i=0; i<location.visible_num; i++)
	{
		locationTp[location.invisible_num+i] = location.visible_location[i];
		pid[location.invisible_num+i] = location.visible_location[i].pid;
	}

	*num = location.invisible_num+location.visible_num;
	return GXCORE_SUCCESS;
}


/**
 * @brief 		保存location (当前升级成功的或从流里解析出来的)
 * @param   	void
 * @return   	GXCORE_SUCCESS:执行正常
 				GXCORE_ERROR:执行失败
 */
status_t gx_TkgsSaveLocation(GxTkgsLocationParm *locationTp)
{
	GxTkgsLocation location;
	int i;
	uint8_t flag = 0;

	if (NULL == locationTp)
	{
		app_log_error("====%s, %d, err\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}

	if (GXCORE_ERROR == GxBus_TkgsLocationGet(&location))
	{
		app_log_error("====%s, %d, err\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}

	if (location.invisible_num > 0)
	{
		if (0 == memcmp(locationTp, &(location.invisible_location[0]), sizeof(GxTkgsLocationParm)))
		{
			return GXCORE_SUCCESS;
		}

		//检查要保存的location是否在invisible location列表中
		for (i=1; i<location.invisible_num; i++)
		{
			if (0 == memcmp(locationTp, &(location.invisible_location[i]), sizeof(GxTkgsLocationParm)))
			{
				flag = i;
				break;
			}
		}

		//处理要保存的location不在invisible location列表中且列表已满的情况
		if ((location.invisible_num == TKGS_MAX_INVISIBLE_LOCATION) && (flag == 0))
		{
			flag = TKGS_MAX_INVISIBLE_LOCATION -1;
		}

		if (flag == 0)
		{
			for (i=location.invisible_num; i>0; i--)
			{
				location.invisible_location[i] = location.invisible_location[i-1];
			}
			location.invisible_num += 1;
		}
		else
		{
			for (i=flag; i>0; i--)
			{
				location.invisible_location[i] = location.invisible_location[i-1];
			}
		}
	}
	else
	{
		location.invisible_num = 1;
	}

	location.invisible_location[0] = *locationTp;

	if (GXCORE_ERROR == GxBus_TkgsLocationSet(&location))
	{
	    app_log_error("====%s, %d, err\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}

	return GXCORE_SUCCESS;
}
#endif
