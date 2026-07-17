/*****************************************************************************
*             CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :   xxx.c
* Author    :   xxx
* Project   :   GoXceed -S
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION   Date              AUTHOR         Description
   0.0      2009.12.04              xxx         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */

/* Private types/constants ------------------------------------------------ */

/* Private Functions ----------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */
#include "app.h"

#include <gxmsg.h>
#include <gxbus.h>
#include <service/gxbook.h>
#include "gxupdate.h"
#include "app_send_msg.h"
#include "app_data_type.h"
#include "app_msg.h"
#include "app_default_params.h"
#include "gxavdev.h"
#include "module/app_network_service.h"
#if NETAPPS_SUPPORT
#include "netapps/iptv/app_iptv_list_service.h"
#endif
#if NETAPPS_SUPPORT
#include "netapps/app_netvideo_play.h"
#endif

#if VPN_SUPPORT
#include "module/app_vpn_service.h"
#endif

#if TKGS_SUPPORT
#include "module/tkgs/gxtkgs.h"
#endif

#if SAT_FINDER_SUPPORT
#include "module/satfinder/dvbfinder.h"
#endif
#if PLUGINSTORE_SUPPORT
#include "netapps/plugin/app_plugin_server.h"
#endif
#if WEBLET_SUPPORT
#include "netapps/weblet/app_weblet_service.h"
#endif
#if WEBAPI_SUPPORT
#include "app_webapi.h"
#endif
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif

#if DLNADMR_SUPPORT || DLNADMP_SUPPORT || DLNASAT2IP_SUPPORT || DLNADMS_SUPPORT
#include "netapps/dlna/app_dlna.h"
#endif
#ifdef SUBTTRANS_SUPPORT
#include "gx_subttrans.h"
#endif
#if USB_TEST_SUPPORT
#include "app_usb_test.h"
#endif
#if HDMI_CEC_SUPPORT
#include "av/hal/gxav_hal_vout.h"
#include "hdmi_cec/app_cec_link.h"
#endif

extern status_t GxBus_PmLock(void);

#define CHK_RESULT(num)\
((num)==-1?GXCORE_ERROR:GXCORE_SUCCESS)

#define TUNER_GET_COUNT 5

typedef status_t (*msg_func)(uint32_t msg_id, void* params);
typedef struct
{
    unsigned int message_id;
    msg_func message_func;
}MsgIdFunc;

static  status_t  gxmsg_pm_open(uint32_t msg_id,void* params)
{
    status_t ret = GXCORE_ERROR;

    if(GXMSG_PM_OPEN !=  msg_id)
    {
        APP_PRINT("msg id is incorrect! msg_id = %d",msg_id);
        return GXCORE_ERROR;
    }
    GxBus_PmLock();

    ret = GxBus_PmDbaseInit(SYS_MAX_SAT, SYS_MAX_TP, SYS_MAX_PROG, (uint8_t*)DEFAULT_DB_PATH);

    GxBus_PmUnlock();
    return ret;
}

static  status_t  gxmsg_pm_close(uint32_t msg_id,void* params)
{
    status_t ret = GXCORE_ERROR;

    if(GXMSG_PM_CLOSE !=  msg_id)
    {
        APP_PRINT("msg id is incorrect! msg_id = %d",msg_id);
        return GXCORE_ERROR;
    }

    GxBus_PmLock();
    ret = GxBus_PmDbaseClose();
    GxBus_PmUnlock();

    return ret;
}

static  status_t  gxmsg_node_num_get(uint32_t msg_id,void* params)
{
    GxMsgProperty_NodeNumGet *numGet = NULL;
    int32_t number = -1;

    if(NULL == params ||GXMSG_PM_NODE_NUM_GET !=  msg_id)
    {
        APP_PRINT("msg id or params is incorrect! msg_id = %d\n",msg_id);
        return GXCORE_ERROR;
    }

    numGet = (GxMsgProperty_NodeNumGet*)params;

	GxBus_PmLock();

    switch(numGet->node_type)
    {
        case NODE_SAT:
            {
                number = GxBus_PmSatNumGet();
                break;
            }
        case NODE_TP:
            {
                number = GxBus_PmTpNumGetBySat(numGet->sat_id);
                break;
            }
        case NODE_TP_ALL:
            {
                number = GxBus_PmTpNumGet();
                break;
            }
        case NODE_PROG:
            {
                number = GxBus_PmProgNumGet();
                break;
            }
        case NODE_CAS:
            {
                number = GxBus_PmCasNumGet();
                break;
            }
        case NODE_FAV_GROUP:
            {
                number = GxBus_PmFavGroupNumGet();
                break;
            }
        case NODE_HD_GROUP:
            {
                number = GxBus_PmHdProgNumGet();
                break;
            }
        default:
            {
                number = -1;
                break;
            }
    }

    GxBus_PmUnlock();

    if(-1 == number)
    {
        APP_PRINT("can not get the number of PM successfully  numGet->node_type = %d\n",numGet->node_type);
        numGet->node_num = 0;
        return GXCORE_ERROR;
    }

    else
    {
        numGet->node_num = number;
        return GXCORE_SUCCESS;
    }
}

static  status_t  gxmsg_node_by_pos_get(uint32_t msg_id,void* params)
{
    GxMsgProperty_NodeByPosGet *nodeGet = NULL;
    status_t ret  = GXCORE_ERROR;

    if(NULL == params ||GXMSG_PM_NODE_BY_POS_GET !=  msg_id)
    {
        APP_PRINT("msg id or params is incorrect! msg_id = %d\n",msg_id);
        return GXCORE_ERROR;
    }

    nodeGet = (GxMsgProperty_NodeByPosGet*)params;
    GxBus_PmLock();

    switch(nodeGet->node_type)
    {
        case NODE_SAT:
            {
                int32_t number = -1;
                number =  GxBus_PmSatsGetByPos(nodeGet->pos,1,&(nodeGet->sat_data));
                ret = CHK_RESULT(number);
                break;
            }
        case NODE_TP:
            {
                int32_t number = -1;
                number = GxBus_PmTpGetByPosInSat(nodeGet->pos,1,&(nodeGet->tp_data));
                ret = CHK_RESULT(number);
                break;
            }
        case NODE_TP_ALL:
            {
                int32_t number = -1;
                number = GxBus_PmTpGetByPos(nodeGet->pos,1,&(nodeGet->tp_data));
                ret = CHK_RESULT(number);
                break;
            }
        case NODE_PROG:
            {
                int32_t number = -1;
                number = GxBus_PmProgGetByPos(nodeGet->pos,1,&(nodeGet->prog_data));
                ret = CHK_RESULT(number);
                break;
            }
        case NODE_CAS:
            {
                ret = GxBus_PmCasGetByPos(nodeGet->pos,&(nodeGet->cas_info));
                break;
            }
        case NODE_FAV_GROUP:
            {
                int32_t number = -1;
                number = GxBus_PmFavGroupGetByPos(nodeGet->pos,1,&(nodeGet->fav_item));
                ret = CHK_RESULT(number);
                break;
            }
        default:
            {
                ret = GXCORE_ERROR;
                break;
            }
    }

    GxBus_PmUnlock();

    if(GXCORE_ERROR == ret)
    {
        GxBusPmViewInfo pmview_cur = {0};
        app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&pmview_cur));
        APP_PRINT("[%s] Cann't get Node[T:%d P:%d] in View[G:%d T:%d S:%d Skip:%d Sat:%d Tp:%d Fav:%d L:%s]\n",
                __FUNCTION__,
                nodeGet->node_type,
                nodeGet->pos,
                pmview_cur.group_mode,
                pmview_cur.taxis_mode,
                pmview_cur.stream_type,
                pmview_cur.skip_view_switch,
                pmview_cur.sat_id,
                pmview_cur.tp_id,
                pmview_cur.fav_id,
                pmview_cur.letter);
    }

    return ret;
}

static  status_t  gxmsg_multi_node_by_pos_get(uint32_t msg_id,void* params)
{
    GxMsgProperty_MultiNodeByPosGet *nodeGet = NULL;
    status_t ret  = GXCORE_ERROR;
    int32_t number = -1;

    if(NULL == params ||GXMSG_PM_MULTI_NODE_BY_POS_GET !=  msg_id)
    {
        APP_PRINT("msg id or params is incorrect! msg_id = %d\n",msg_id);
        return GXCORE_ERROR;
    }

    nodeGet = (GxMsgProperty_MultiNodeByPosGet*)params;

    GxBus_PmLock();

    switch(nodeGet->node_type)
    {
        case NODE_SAT:
            {
                number =  GxBus_PmSatsGetByPos(nodeGet->pos,nodeGet->number,nodeGet->sat_array);
                ret = CHK_RESULT(number);
                break;
            }
        case NODE_TP:
            {
                number = GxBus_PmTpGetByPosInSat(nodeGet->pos,nodeGet->number,nodeGet->tp_array);
                ret = CHK_RESULT(number);
                break;
            }
        case NODE_TP_ALL:
            {
                number = GxBus_PmTpGetByPos(nodeGet->pos,nodeGet->number,nodeGet->tp_array);
                ret = CHK_RESULT(number);
                break;
            }
        case NODE_PROG:
            {
                number = GxBus_PmProgGetByPos(nodeGet->pos,nodeGet->number,nodeGet->prog_array);
                ret = CHK_RESULT(number);
                break;
            }
        case NODE_CAS:
            {
                ret = GxBus_PmCasGetByPos(nodeGet->pos,nodeGet->cas_array);
                if(GXCORE_SUCCESS == ret)
                {
                    number = 1;
                }
                break;
            }
        case NODE_FAV_GROUP:
            {
                number = GxBus_PmFavGroupGetByPos(nodeGet->pos,nodeGet->number,nodeGet->fav_array);
                ret = CHK_RESULT(number);
                break;
            }
        default:
            {
                ret = GXCORE_ERROR;
                break;
            }
    }
    GxBus_PmUnlock();

    if(GXCORE_ERROR == ret)
    {
        APP_PRINT("can not get PM node by pos! nodeGet->node_type = %d\n",nodeGet->node_type);
    }
    else
    {
        nodeGet->number = number;
    }

    return ret;
}

static  status_t  gxmsg_node_by_id_get(uint32_t msg_id,void* params)
{
    GxMsgProperty_NodeByIdGet *nodeGet = NULL;
    status_t ret  = GXCORE_ERROR;

    if(NULL == params ||GXMSG_PM_NODE_BY_ID_GET !=  msg_id)
    {
        APP_PRINT("msg id or params is incorrect! msg_id = %d\n",msg_id);
        return GXCORE_ERROR;
    }

    nodeGet = (GxMsgProperty_NodeByIdGet*)params;
    GxBus_PmLock();
    switch(nodeGet->node_type)
    {
        case NODE_SAT:
            {
                ret =  GxBus_PmSatGetById(nodeGet->id,&(nodeGet->sat_data));
                break;
            }
        case NODE_TP:
            {
                ret =  GxBus_PmTpGetById(nodeGet->id,&(nodeGet->tp_data));
                break;
            }
        case NODE_PROG:
            {
                ret =  GxBus_PmProgGetById(nodeGet->id,&(nodeGet->prog_data));
                break;
            }
        case NODE_CAS:
            {
                ret =  GxBus_PmCasGetById(nodeGet->id,&(nodeGet->cas_info));
                break;
            }
        case NODE_FAV_GROUP:
            {
                ret =  GxBus_PmFavGroupGetById(nodeGet->id,&(nodeGet->fav_item));
                break;
            }
        default:
            {
                ret = GXCORE_ERROR;
                break;
            }
    }
    GxBus_PmUnlock();
    if(GXCORE_ERROR == ret)
    {
        APP_PRINT("can not get PM node by id! nodeGet->node_type = %d\n",nodeGet->node_type);
    }

    return ret;
}

static  status_t  gxmsg_node_add(uint32_t msg_id,void* params)
{
    GxMsgProperty_NodeAdd *nodeAdd = NULL;
    GxBusPmSyncType syncType = 0;
    status_t ret  = GXCORE_ERROR;

    if(NULL == params ||GXMSG_PM_NODE_ADD !=  msg_id)
    {
        APP_PRINT("msg id or params is incorrect! msg_id = %d\n",msg_id);
        return GXCORE_ERROR;
    }
    nodeAdd = (GxMsgProperty_NodeAdd*)params;

    GxBus_PmLock();

    switch(nodeAdd->node_type)
    {
        case NODE_SAT:
            {
                syncType = GXBUS_PM_SYNC_SAT;
                ret = GxBus_PmSatAdd(&(nodeAdd->sat_data));
                break;
            }
        case NODE_TP:
            {
                syncType = GXBUS_PM_SYNC_TP;
                ret = GxBus_PmTpAdd(&(nodeAdd->tp_data));
                break;
            }
        case NODE_PROG:
            {
                syncType = GXBUS_PM_SYNC_PROG;
                ret = GxBus_PmProgAdd(&(nodeAdd->prog_data));
                break;
            }
        default:
            {
                ret = GXCORE_ERROR;
                break;
            }
    }
    GxBus_PmUnlock();
    if(GXCORE_ERROR == ret)
    {
        APP_PRINT("can not add node into PM! nodeAdd->node_type = %d\n",nodeAdd->node_type);
    }
    else
    {
        ret = GxBus_PmSync(syncType);
    }

    return ret;
}

static  status_t  gxmsg_node_modify(uint32_t msg_id,void* params)
{
    GxMsgProperty_NodeModify *nodeModify = NULL;
    GxBusPmSyncType syncType = 0;
    status_t ret  = GXCORE_ERROR;

    if(NULL == params ||GXMSG_PM_NODE_MODIFY !=  msg_id)
    {
        APP_PRINT("msg id or params is incorrect! msg_id = %d\n",msg_id);
        return GXCORE_ERROR;
    }
    nodeModify = (GxMsgProperty_NodeModify*)params;
    GxBus_PmLock();

    switch(nodeModify->node_type)
    {
        case NODE_SAT:
            {
                syncType = GXBUS_PM_SYNC_SAT;
                ret = GxBus_PmSatModify(&(nodeModify->sat_data));
                break;
            }
        case NODE_TP:
            {
                syncType = GXBUS_PM_SYNC_TP;
                ret = GxBus_PmTpModify(&(nodeModify->tp_data));
                break;
            }
        case NODE_PROG:
            {
                syncType = GXBUS_PM_SYNC_PROG;
                ret = GxBus_PmProgInfoModify(&(nodeModify->prog_data));
                break;
            }
        case NODE_FAV_GROUP:
            {
                syncType = GXBUS_PM_SYNC_FAV;
                ret = GxBus_PmFavGroupModify(&(nodeModify->fav_item));
                break;
            }
        default:
            {
                ret = GXCORE_ERROR;
                break;
            }
    }
    GxBus_PmUnlock();

    if(GXCORE_ERROR == ret)
    {
        APP_PRINT("can not modify the node of PM! nodeModify->node_type = %d\n",nodeModify->node_type);
    }
    else
    {
        ret = GxBus_PmSync(syncType);
    }

    return ret;
}

static  status_t  gxmsg_node_delete(uint32_t msg_id,void* params)
{
    GxMsgProperty_NodeDelete *nodeDel = NULL;
    GxBusPmSyncType syncType = 0;
    status_t ret  = GXCORE_ERROR;

    if(NULL == params ||GXMSG_PM_NODE_DELETE !=  msg_id)
    {
        APP_PRINT("msg id or params is incorrect! msg_id = %d\n",msg_id);
        return GXCORE_ERROR;
    }
    nodeDel = (GxMsgProperty_NodeDelete*)params;
    GxBus_PmLock();

    switch(nodeDel->node_type)
    {
        case NODE_SAT:
            {
                syncType = GXBUS_PM_SYNC_SAT;
                ret = GxBus_PmSatDelete(nodeDel->id_array,nodeDel->num);
                break;
            }
        case NODE_TP:
            {
                syncType = GXBUS_PM_SYNC_TP;
                ret = GxBus_PmTpDelete(nodeDel->id_array,nodeDel->num);
                break;
            }
        case NODE_PROG:
            {
                syncType = GXBUS_PM_SYNC_PROG;
                ret = GxBus_PmProgDelete(nodeDel->id_array,nodeDel->num);
                break;
            }
        default:
            {
                ret = GXCORE_ERROR;
                break;
            }
    }
    GxBus_PmUnlock();

    if(GXCORE_ERROR == ret)
    {
        APP_PRINT("can not delete the node of PM! nodeDel->node_type = %d\n",nodeDel->node_type);
    }
    else
    {
        ret = GxBus_PmSync(syncType);
    }

    return ret;
}

static  status_t  gxmsg_node_get_pm_url(uint32_t msg_id,void* params)
{
    GxMsgProperty_GetUrl *getUrl = NULL;
    status_t ret  = GXCORE_ERROR;

    if(NULL == params ||GXMSG_PM_NODE_GET_URL !=  msg_id)
    {
        APP_PRINT("msg id or params is incorrect! msg_id = %d\n",msg_id);
        return GXCORE_ERROR;
    }
    getUrl = (GxMsgProperty_GetUrl*)params;
    GxBus_PmLock();
    ret = GxBus_PmProgUrlGet(&(getUrl->prog_info),getUrl->url,getUrl->size);
    GxBus_PmUnlock();
    if(GXCORE_ERROR == ret)
    {
        APP_PRINT("can not get pm url\n");
    }

    return ret;
}

static status_t gxmsg_pm_load_default(uint32_t msg_id,void* params)
{
    if(GXMSG_PM_LOAD_DEFAULT !=  msg_id)
    {
        APP_PRINT("msg id is error! msg_id = %d",msg_id);
        return GXCORE_ERROR;
    }
    GxBus_PmLock();
    GxBus_PmLoadDefault(SYS_MAX_SAT, SYS_MAX_TP, SYS_MAX_PROG, (uint8_t*)DEFAULT_DB_PATH);
    GxBus_PmUnlock();
    return GXCORE_SUCCESS;
}

static status_t gxmsg_cpy_stream_for_prog(uint32_t srcProgId,uint32_t dstProgId)
{
    int32_t number = -1;
    // the follow 3 line del by shenbin, inorder to compiler ok
    //  number = GxBus_PmStreamNumGet(srcProgId);
    if(0 >= number)
    {
        APP_PRINT("get stream num by progid error!\n");
        return GXCORE_ERROR;
    }
    else
    {
        GxBusPmDataStream *stream = NULL;
        int32_t retNum = -1;

        stream = GxCore_Malloc(number*sizeof(GxBusPmDataStream));
        if(NULL == stream)
        {
            APP_PRINT("create memory error!\n");
            return GXCORE_ERROR;
        }
        memset(stream,0,number*sizeof(GxBusPmDataStream));
        GxBus_PmLock();
        //      retNum = GxBus_PmStreamGet(srcProgId,number,stream);
        if(-1 == retNum)
        {
            GxBus_PmUnlock();
            GxCore_Free(stream);
            APP_PRINT("get stream info by progid error!\n");
            return GXCORE_ERROR;
        }
        else
        {
            int32_t i = 0;
            status_t addResult = GXCORE_ERROR;

            for(i=0; i<retNum;i++)
            {
                stream[i].prog_id = dstProgId;
            }

            //          addResult = GxBus_PmStreamAdd(dstProgId,retNum,stream);
            if(GXCORE_ERROR == addResult)
            {
                GxBus_PmUnlock();
                GxCore_Free(stream);
                APP_PRINT("add stream info by progid error!\n");
                return GXCORE_ERROR;
            }
        }
        GxBus_PmUnlock();
        GxCore_Free(stream);
        return GXCORE_SUCCESS;
    }
}

static status_t  gxmsg_cpy_prog_for_tuner(uint32_t count,GxBusPmDataSatTuner dstTuner)
{
    uint32_t start_pos = 0;
    status_t ret  = GXCORE_ERROR;
    uint32_t num = TUNER_GET_COUNT;

    if(TUNER_GET_COUNT > count)
    {
        num = count;
    }
    GxBus_PmLock();
    while(start_pos < count)
    {
        int32_t number = -1;
        GxBusPmDataProg prog_data[TUNER_GET_COUNT];

        number = GxBus_PmProgGetByPos(start_pos ,num,prog_data);
        if(-1 == number)
        {
            GxBus_PmUnlock();
            APP_PRINT("prog get by pos error!\n");
            return GXCORE_ERROR;
        }
        else
        {
            int i = 0;
            if(num >= number)
            {
                start_pos += number;
            }
            else
            {
                GxBus_PmUnlock();
                APP_PRINT("prog get by pos error!number = %d\n",number);
                return GXCORE_ERROR;
            }

            for(i=0; i<number; i++)
            {
                // temp by shenbin 100227
#define TP_ID        1
#define ORIG_NET_ID  1
                int32_t checkExist = -1;
                checkExist = GxBus_PmProgExistChek(TP_ID,prog_data[i].ts_id,prog_data[i].service_id,ORIG_NET_ID,dstTuner,0xff,0xff,0xff);
                if(0 == checkExist)
                {
                    uint32_t srcProgId = prog_data[i].id;
                    prog_data[i].tuner = dstTuner;
                    ret = GxBus_PmProgAdd(&prog_data[i]);
                    if(GXCORE_ERROR == ret)
                    {
                        APP_PRINT("add porg error ! prog_data[i].id = %d\n",prog_data[i].id);
                    }
                    else
                    {
                        ret = gxmsg_cpy_stream_for_prog(srcProgId,prog_data[i].id);
                        if(GXCORE_ERROR == ret)
                        {
                            APP_PRINT("copy the the steam info of program which src id = %d\n",srcProgId);
                        }
                    }
                }
            }
        }
    }

	GxBus_PmUnlock();

    return GXCORE_SUCCESS;
}

static  status_t  gxmsg_tuner_copy(uint32_t msg_id,void* params)
{
    uint32_t number = 0;
    GxMsgProperty_TunerCopy *tunerCpy = NULL;
    GxBusPmViewInfo view_info;

    if(NULL == params ||    GXMSG_TUNER_COPY !=  msg_id)
    {
        APP_PRINT("msg id or params is incorrect! msg_id = %d\n",msg_id);
        return GXCORE_ERROR;
    }

    tunerCpy = (GxMsgProperty_TunerCopy*)params;

    GxBus_PmLock();
    if(GxBus_PmViewInfoGet(&view_info) == GXCORE_ERROR)
    {
        GxBus_PmUnlock();
        APP_PRINT("get the sys info error!\n");
        return GXCORE_ERROR;
    }
    view_info.group_mode = GROUP_MODE_ALL;
    view_info.taxis_mode = TAXIS_MODE_TUNER;
    view_info.stream_type = GXBUS_PM_PROG_TV;
    view_info.tuner_num = tunerCpy->src_tuner;

    if(GXCORE_ERROR != GxBus_PmViewInfoMemSet(&view_info))
    {
        GxBus_PmUnlock();
        APP_PRINT("modify the sys info error!\n");
        return GXCORE_ERROR;
    }

    number = GxBus_PmProgNumGet();

    if(GXCORE_ERROR == gxmsg_cpy_prog_for_tuner(number,tunerCpy->dst_tuner))
    {
        GxBus_PmViewInfoMemClear();
        GxBus_PmUnlock();
        APP_PRINT("copy prog for tuner error!\n");
        return GXCORE_ERROR;
    }

    if(GXCORE_ERROR == GxBus_PmSync(GXBUS_PM_SYNC_PROG))
    {
        GxBus_PmViewInfoMemClear();
        GxBus_PmUnlock();
        APP_PRINT("modify sys info error!\n");
        return GXCORE_ERROR;
    }

    GxBus_PmViewInfoMemClear();
    GxBus_PmUnlock();
    return GXCORE_SUCCESS;
}

static  status_t  gxmsg_pm_viewinfo_set(uint32_t msg_id,void* params)
{
    GxBusPmViewInfo *viewinfo = NULL ;
    GxBusPmViewInfo oldinfo = {0};
    status_t ret  = GXCORE_ERROR;

    if(NULL == params ||    GXMSG_PM_VIEWINFO_SET !=  msg_id)
    {
        APP_PRINT("msg id or params is incorrect! msg_id = %d\n",msg_id);
        return GXCORE_ERROR;
    }

    viewinfo = (GxBusPmViewInfo*)params;
    GxBus_PmViewInfoGet(&oldinfo);
    if(memcmp(&oldinfo, viewinfo, sizeof(GxBusPmViewInfo)) == 0)
    {
        return GXCORE_SUCCESS;
    }
    GxBus_PmLock();
    ret = GxBus_PmViewInfoModify(viewinfo);
    GxBus_PmUnlock();

    if(GXCORE_ERROR == ret)
    {
        APP_PRINT("modify view info error!\n");
    }
    return ret;
}

static  status_t  gxmsg_pm_viewinfo_get(uint32_t msg_id,void* params)
{
    GxBusPmViewInfo *viewinfo = NULL ;
    status_t ret  = GXCORE_ERROR;

    if(NULL == params ||    GXMSG_PM_VIEWINFO_GET !=  msg_id)
    {
        APP_PRINT("msg id or params is incorrect! msg_id = %d\n",msg_id);
        return GXCORE_ERROR;
    }

    viewinfo = (GxBusPmViewInfo*)params;
    GxBus_PmLock();
    ret = GxBus_PmViewInfoGet(viewinfo);
    GxBus_PmUnlock();
    if(GXCORE_ERROR == ret)
    {
        APP_PRINT("get view info error!\n");
    }

    return ret;
}

static  status_t  gxmsg_chk_node_exist(uint32_t msg_id,void* params)
{
    GxChkData *nodeChk = NULL;
    status_t ret  = GXCORE_ERROR;
    int32_t retValue = 0;

    if(NULL == params ||GXMSG_CHECK_PM_EXIST !=  msg_id)
    {
        APP_PRINT("msg id or params is incorrect! msg_id = %d\n",msg_id);
        return GXCORE_ERROR;
    }

    nodeChk = (GxChkData*)params;
    GxBus_PmLock();

    switch(nodeChk->chkType)
    {
        case CHK_TP_IN_SAT:
            {
                retValue = GxBus_PmTpExistChek(nodeChk->tpData.sat_id,
                        nodeChk->tpData.fre,
                        nodeChk->tpData.symbol,
                        nodeChk->tpData.polar,
                        0,
                        &(nodeChk->tpData.tp_id));
                if(retValue > 1)
                {
                    ret = GXCORE_SUCCESS;
                }
                else
                {
                    ret = GXCORE_ERROR;
                }
                break;
            }
        case NODE_PROG_IN_TP:
            {
                retValue = GxBus_PmProgExistChek(nodeChk->progData.tp_id,
                        nodeChk->progData.ts_id,
                        nodeChk->progData.service_id,
                        nodeChk->progData.original_id,
                        nodeChk->progData.tuner,
                        0xff,0xff,0xff);

                if(retValue > 1)
                {
                    ret = GXCORE_SUCCESS;
                }
                else
                {
                    ret = GXCORE_ERROR;
                }
                break;
            }
        default:
            {
                ret = GXCORE_ERROR;
                break;
            }
    }
    GxBus_PmUnlock();
    return ret;
}


static status_t gxmsg_empty_service_msg(uint32_t msg_id,void* params)
{
	char *service_name = NULL;
	handle_t handle = GXCORE_INVALID_POINTER;
	status_t ret = GXCORE_ERROR;

	if(NULL == params ||GXMSG_EMPTY_SERVICE_MSG !=  msg_id)
    {
        APP_PRINT("msg id or params is incorrect! msg_id = %d\n",msg_id);
        return GXCORE_ERROR;
    }

	service_name = (char*)params;

	handle = GxBus_ServiceFindByName(service_name);
	if(GXCORE_INVALID_POINTER == handle)
    {
		return GXCORE_ERROR;
    }

	ret = GxBus_MessageEmpty(handle);
	return ret;
}

static status_t gxmsg_get_media_time(uint32_t msg_id,void* params)
{
	GxMsgProperty_GetPlayTime *time_get = NULL;
	PlayTimeInfo  info = {0};

	status_t ret = GXCORE_ERROR;

	if(NULL == params ||GXMSG_GET_MEDIA_TIME !=  msg_id)
    {
        APP_PRINT("msg id or params is incorrect! msg_id = %d\n",msg_id);
        return GXCORE_ERROR;
    }

	time_get = (GxMsgProperty_GetPlayTime*)params;
	ret = GxPlayer_MediaGetTime((const char *)(time_get->player_name), &info);
	if(ret != GXCORE_ERROR){
		time_get->cur_time = info.current;
		time_get->total_time = info.totle;
		time_get->seekmin_time = info.seek_min;
	}

	return ret;
}


static status_t gxmsg_av_output_off(uint32_t msg_id,void* params)
{
    status_t ret = GXCORE_ERROR;
#if 0
	int dev = 0;
    int vout = 0;

    dev = GxAvdev_CreateDevice(0);

    if(dev < 0){
         return GXCORE_ERROR;
    }
    vout = GxAvdev_OpenModule(dev, GXAV_MOD_VIDEO_OUT, 0);

    if(vout < 0){
         return GXCORE_ERROR;
    }

   // ret = GxAVSetProperty(dev, vout,
            //   GxVideoOutPropertyID_Off, NULL, 0);
#endif
	return ret;
}


#define DEF_APP_SEND_MSG_SYNC(fun_name,id,msg_type)                 \
static  status_t fun_name(uint32_t msg_id ,  void* params)                      \
{                                                                       \
    GxMessage *msg = NULL;                                              \
    status_t ret = GXCORE_ERROR;                                            \
    msg_type *data = NULL;                                              \
    msg_type *out_in_param = NULL;                                      \
                                                                        \
                                                                        \
    if(NULL == params ||id !=  msg_id)                                      \
    {                                                                   \
        APP_PRINT("params or msg id is incorrect!msg_id = %d\n",msg_id);      \
        return GXCORE_ERROR;                                            \
    }                                                                   \
                                                                        \
    msg = GxBus_MessageNew(id);                                     \
    if(NULL == msg)                                                     \
    {                                                                   \
        APP_PRINT("create the new message failed!msg_id = %d\n",id);                      \
        return GXCORE_ERROR;                                            \
    }                                                                   \
                                                                        \
    data = (msg_type*)GxBus_GetMsgPropertyPtr(msg, msg_type);           \
    if(NULL == data)                                                        \
    {                                                                   \
		GxBus_MessageFree(msg);											\
        APP_PRINT("can not get the params's memory from msg !\n");          \
        return GXCORE_ERROR;                                            \
    }                                                                   \
    out_in_param = (msg_type*)params;                                   \
                                                                        \
    memcpy(data,out_in_param,sizeof(msg_type));                         \
                                                                        \
    ret = GxBus_MessageSendWait(msg);                                   \
    if(GXCORE_SUCCESS != ret)                                             \
    {                                                                   \
		GxBus_MessageFree(msg);											\
        APP_PRINT("send the massage failed !");                             \
        return GXCORE_ERROR;                                            \
    }                                                                   \
    memcpy(out_in_param,data,sizeof(msg_type));                         \
                                                                        \
    ret = GxBus_MessageFree(msg);                                       \
    if(GXCORE_SUCCESS != ret)                                             \
    {                                                                   \
        APP_PRINT("GxCore_Free the message failed !\n");                           \
        return GXCORE_ERROR;                                            \
    }                                                                   \
                                                                        \
    return GXCORE_SUCCESS;                                              \
}



#define DEF_APP_SEND_MSG_ASYN(fun_name,id,msg_type)             \
static  status_t fun_name(uint32_t msg_id ,  void* params)                      \
{                                                                       \
    GxMessage *msg = NULL;                                              \
    status_t ret = GXCORE_ERROR;                                            \
    msg_type *data = NULL;                                              \
    msg_type *out_param = NULL;                                         \
                                                                        \
                                                                        \
    if((NULL == params) ||(id !=  msg_id))                                  \
    {                                                                   \
        APP_PRINT("params or msg id is incorrect!msg_id = %d\n",msg_id);        \
        return GXCORE_ERROR;                                            \
    }                                                                   \
                                                                        \
    msg = GxBus_MessageNew(id);                                     \
    if(NULL == msg)                                                     \
    {                                                                   \
        return GXCORE_ERROR;                                            \
    }                                                                   \
                                                                        \
    data = (msg_type*)GxBus_GetMsgPropertyPtr(msg, msg_type);           \
    if(NULL == data)                                                        \
    {                                                                   \
		GxBus_MessageFree(msg);											\
        return GXCORE_ERROR;                                            \
    }                                                                   \
    out_param = (msg_type*)params;                                      \
                                                                        \
    memcpy(data,out_param,sizeof(msg_type));                                \
                                                                        \
    ret = GxBus_MessageSend(msg);                                       \
    if(GXCORE_SUCCESS != ret)                                             \
    {                                                                   \
        return GXCORE_ERROR;                                            \
    }                                                                   \
    return GXCORE_SUCCESS;                                              \
}




#define DEF_APP_SEND_MSG_ASYN_NOPARAM(fun_name,id)              \
static  status_t fun_name(uint32_t msg_id ,  void* params)                      \
{                                                                       \
    GxMessage *msg = NULL;                                              \
    status_t ret = GXCORE_ERROR;                                            \
                                                                        \
                                                                        \
    if(id !=  msg_id)                                                       \
    {                                                                   \
        APP_PRINT("params or msg id is incorrect!msg_id = %d\n",msg_id);        \
        return GXCORE_ERROR;                                            \
    }                                                                   \
                                                                        \
    msg = GxBus_MessageNew(id);                                     \
    if(NULL == msg)                                                     \
    {                                                                   \
        return GXCORE_ERROR;                                            \
    }                                                                   \
                                                                        \
    ret = GxBus_MessageSend(msg);                                       \
    if(GXCORE_SUCCESS != ret)                                             \
    {                                                                   \
        return GXCORE_ERROR;                                            \
    }                                                                   \
                                                                        \
    return GXCORE_SUCCESS;                                              \
}

#define DEF_APP_SEND_MSG_SYNC_NOPARAM(fun_name,id)              \
static  status_t fun_name(uint32_t msg_id ,  void* params)                      \
{                                                                       \
    GxMessage *msg = NULL;                                              \
    status_t ret = GXCORE_ERROR;                                            \
                                                                        \
                                                                        \
    if(id !=  msg_id)                                                       \
    {                                                                   \
        APP_PRINT("params or msg id is incorrect!msg_id = %d\n",msg_id);        \
        return GXCORE_ERROR;                                            \
    }                                                                   \
                                                                        \
    msg = GxBus_MessageNew(id);                                     	\
    if(NULL == msg)                                                     \
    {                                                                   \
        return GXCORE_ERROR;                                            \
    }                                                                   \
                                                                        \
    ret = GxBus_MessageSendWait(msg);                                   \
    if(GXCORE_SUCCESS != ret)                                           \
    {                                                                   \
		GxBus_MessageFree(msg);											\
        return GXCORE_ERROR;                                            \
    }                                                                   \
                                                                        \
    ret = GxBus_MessageFree(msg);                                       \
    if(GXCORE_SUCCESS != ret)                                           \
    {                                                                   \
        APP_PRINT("GxCore_Free the message failed !\n");                       \
        return GXCORE_ERROR;                                            \
    }                                                                   \
                                                                        \
    return GXCORE_SUCCESS;                                              \
}

DEF_APP_SEND_MSG_ASYN(gxmsg_player_play_asyn,GXMSG_PLAYER_PLAY,GxMsgProperty_PlayerPlay)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_stop_asyn,GXMSG_PLAYER_STOP,GxMsgProperty_PlayerStop)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_pause_asyn,GXMSG_PLAYER_PAUSE,GxMsgProperty_PlayerPause)

DEF_APP_SEND_MSG_ASYN(gxmsg_player_resume_asyn,GXMSG_PLAYER_RESUME,GxMsgProperty_PlayerResume)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_record_asyn,GXMSG_PLAYER_RECORD,GxMsgProperty_PlayerRecord)

DEF_APP_SEND_MSG_ASYN(gxmsg_player_video_color_asyn,GXMSG_PLAYER_VIDEO_COLOR,GxMsgProperty_PlayerVideoColor)

DEF_APP_SEND_MSG_ASYN(gxmsg_player_video_clip_asyn,GXMSG_PLAYER_VIDEO_CLIP,GxMsgProperty_PlayerVideoClip)

DEF_APP_SEND_MSG_ASYN(gxmsg_player_seek_asyn,GXMSG_PLAYER_SEEK,GxMsgProperty_PlayerSeek)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_speed_asyn,GXMSG_PLAYER_SPEED,GxMsgProperty_PlayerSpeed)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_audio_volume_asyn,GXMSG_PLAYER_AUDIO_VOLUME,GxMsgProperty_PlayerAudioVolume)

DEF_APP_SEND_MSG_ASYN(gxmsg_player_audio_mute_asyn,GXMSG_PLAYER_AUDIO_MUTE,GxMsgProperty_PlayerAudioMute)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_video_window_sync,GXMSG_PLAYER_VIDEO_WINDOW,GxMsgProperty_PlayerVideoWindow)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_video_aspect_sync,GXMSG_PLAYER_VIDEO_ASPECT,GxMsgProperty_PlayerVideoAspect)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_video_interface_sync,GXMSG_PLAYER_VIDEO_INTERFACE,GxMsgProperty_PlayerVideoInterface)

/*DEF_APP_SEND_MSG_SYNC(gxmsg_player_video_output_enum_sync,GXMSG_PLAYER_VIDEO_OUTPUT_ENUM,GxMsgProperty_PlayerVideoOutputEnum)*/

DEF_APP_SEND_MSG_SYNC(gxmsg_player_video_mode_config_sync,GXMSG_PLAYER_VIDEO_MODE_CONFIG,GxMsgProperty_PlayerVideoModeConfig)

DEF_APP_SEND_MSG_ASYN(gxmsg_player_video_auto_adapt_asyn,GXMSG_PLAYER_VIDEO_AUTO_ADAPT,GxMsgProperty_PlayerVideoAutoAdapt)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_pvr_config_sync,GXMSG_PLAYER_PVR_CONFIG,GxMsgProperty_PlayerPVRConfig)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_display_screen_sync,GXMSG_PLAYER_DISPLAY_SCREEN,GxMsgProperty_PlayerDisplayScreen)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_video_hide_sync,GXMSG_PLAYER_VIDEO_HIDE,GxMsgProperty_PlayerVideoHide)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_video_show_sync,GXMSG_PLAYER_VIDEO_SHOW,GxMsgProperty_PlayerVideoShow)

/*DEF_APP_SEND_MSG_SYNC(gxmsg_player_focus_ctrl_sync,GXMSG_PLAYER_FOCUS_CTRL,GxMsgProperty_PlayerFocusCtrl)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_quiet_switchl_sync,GXMSG_PLAYER_FREEZE_FRAME_SWITCH, GxMsgProperty_PlayerFreezeFrameSwitch)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_snapshot_sync,GXMSG_PLAYER_SNAPSHOT,GxMsgProperty_PlayerSnapshot)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_get_pts_sync,GXMSG_PLAYER_GET_PTS,GxMsgProperty_PlayerGetPts)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_get_program_info_sync,GXMSG_PLAYER_GET_PROGRAM_INFO,GxMsgProperty_PlayerGetProgInfo)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_get_status_sync,GXMSG_PLAYER_GET_STATUS,GxMsgProperty_PlayerGetStatus)*/

DEF_APP_SEND_MSG_SYNC(gxmsg_player_freeze_frame_switch_sync,GXMSG_PLAYER_FREEZE_FRAME_SWITCH,GxMsgProperty_PlayerFreezeFrameSwitch)

DEF_APP_SEND_MSG_ASYN(gxmsg_player_add_pid_record, GXMSG_PLAYER_TRACK_ADD, GxMsgProperty_PlayerTrackAdd)

DEF_APP_SEND_MSG_ASYN(gxmsg_player_bandwidth_switch, GXMSG_PLAYER_BANDWIDTH_SWITCH, GxMsgProperty_PlayerBandwidthSwitch)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_ad_audio_enable_sync,GXMSG_PLAYER_AD_AUDIO_ENABLE, GxMsgProperty_PlayerAdAudio)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_ad_audio_disable_sync,GXMSG_PLAYER_AD_AUDIO_DISABLE, GxMsgProperty_PlayerAdAudio)

DEF_APP_SEND_MSG_SYNC(gxmsg_si_subtable_get_sync,GXMSG_SI_SUBTABLE_GET,GxMsgProperty_SiGet)

DEF_APP_SEND_MSG_SYNC(gxmsg_si_subtable_create_sync,GXMSG_SI_SUBTABLE_CREATE,GxMsgProperty_SiCreate)

DEF_APP_SEND_MSG_SYNC(gxmsg_epg_filter_num_get_sync,GXMSG_EPG_FILTER_NUM_GET,int)

DEF_APP_SEND_MSG_SYNC(gxmsg_epg_filter_info_get_sync,GXMSG_EPG_FILTER_INFO_GET,GxMsgProperty_EpgFilterInfo)

DEF_APP_SEND_MSG_SYNC(gxmsg_epg_filter_modify_sync,GXMSG_EPG_FILTER_MODIFY,GxMsgProperty_EpgFilterModify)

DEF_APP_SEND_MSG_SYNC(gxmsg_si_subtable_modify_sync,GXMSG_SI_SUBTABLE_MODIFY,GxMsgProperty_SiModify)

DEF_APP_SEND_MSG_SYNC(gxmsg_si_subtable_release_sync,GXMSG_SI_SUBTABLE_RELEASE,GxMsgProperty_SiRelease)

DEF_APP_SEND_MSG_ASYN(gxmsg_si_subtable_start_asyn,GXMSG_SI_SUBTABLE_START,GxMsgProperty_SiStart)

DEF_APP_SEND_MSG_SYNC(gxmsg_epg_num_get_sync,GXMSG_EPG_NUM_GET,GxMsgProperty_EpgNumGet)

DEF_APP_SEND_MSG_SYNC(gxmsg_epg_num_get_by_period_sync,GXMSG_EPG_NUM_GET_BY_PERIOD,GxMsgProperty_EpgNumGetByPeriod)

DEF_APP_SEND_MSG_SYNC(gxmsg_epg_get_sync,GXMSG_EPG_GET,GxMsgProperty_EpgGet)

DEF_APP_SEND_MSG_SYNC(gxmsg_epg_get_by_period_sync,GXMSG_EPG_GET_BY_PERIOD,GxMsgProperty_EpgGetByPeriod)

DEF_APP_SEND_MSG_SYNC(gxmsg_epg_get_by_event_id_sync,GXMSG_EPG_GET_BY_EVENT_ID,GxMsgProperty_EpgGetByEventId)

DEF_APP_SEND_MSG_SYNC(gxmsg_epg_pf_num_get_sync,GXMSG_EPG_PF_NUM_GET,GxMsgProperty_EpgPFNumGet)

DEF_APP_SEND_MSG_SYNC(gxmsg_epg_pf_get_sync,GXMSG_EPG_PF_GET,GxMsgProperty_EpgPFGet)

DEF_APP_SEND_MSG_SYNC(gxmsg_epg_create_sync,GXMSG_EPG_CREATE,GxMsgProperty_EpgCreate)

#if EPG_REALTIMER_GET_DETAIL_SUPPORT
DEF_APP_SEND_MSG_SYNC(gxmsg_epg_detail_filter_set_sync,GXMSG_EPG_DETAIL_FILTER_SET,GxMsgProperty_EpgDetailFilterSwitch)

DEF_APP_SEND_MSG_SYNC_NOPARAM(gxmsg_epg_detail_filter_clean_sync,GXMSG_EPG_DETAIL_FILTER_CLEAN)
#endif

DEF_APP_SEND_MSG_ASYN_NOPARAM(gxmsg_epg_clean_asyn,GXMSG_EPG_CLEAN)

DEF_APP_SEND_MSG_ASYN_NOPARAM(gxmsg_epg_reset_asyn,GXMSG_EPG_RESET)

DEF_APP_SEND_MSG_SYNC_NOPARAM(gxmsg_epg_release_sync,GXMSG_EPG_RELEASE)

DEF_APP_SEND_MSG_SYNC(gxmsg_epg_filter_start_sync,GXMSG_EPG_FILTER_START,GxMsgProperty_EpgCreate)

DEF_APP_SEND_MSG_SYNC_NOPARAM(gxmsg_epg_filter_stop_sync,GXMSG_EPG_FILTER_STOP)

DEF_APP_SEND_MSG_SYNC(gxmsg_epg_cfg_get_sync,GXMSG_EPG_CFG_GET,GxMsgProperty_EpgCreate)

DEF_APP_SEND_MSG_ASYN_NOPARAM(gxmsg_epg_full_sync,GXMSG_EPG_FULL)

DEF_APP_SEND_MSG_ASYN_NOPARAM(gxmsg_epg_full_speed_sync,GXMSG_EPG_FULL_SPEED)

DEF_APP_SEND_MSG_ASYN_NOPARAM(gxmsg_epg_section_flag_release_sync,GXMSG_EPG_SECTION_FLAG_RELEASE)

DEF_APP_SEND_MSG_SYNC(gxmsg_book_get_sync,GXMSG_BOOK_GET,GxMsgProperty_BookGet)

DEF_APP_SEND_MSG_SYNC(gxmsg_book_create_sync,GXMSG_BOOK_CREATE,GxMsgProperty_BookCreate)

DEF_APP_SEND_MSG_SYNC(gxmsg_book_modify_sync,GXMSG_BOOK_MODIFY,GxMsgProperty_BookModify)

DEF_APP_SEND_MSG_SYNC(gxmsg_book_destroy_sync,GXMSG_BOOK_DESTROY,GxMsgProperty_BookDestroy)

DEF_APP_SEND_MSG_ASYN_NOPARAM(gxmsg_book_start_asyn,GXMSG_BOOK_START)

DEF_APP_SEND_MSG_ASYN_NOPARAM(gxmsg_book_stop_asyn,GXMSG_BOOK_STOP)

DEF_APP_SEND_MSG_ASYN_NOPARAM(gxmsg_book_reset_asyn,GXMSG_BOOK_RESET)

DEF_APP_SEND_MSG_ASYN(gxmsg_search_scan_sat_asyn,GXMSG_SEARCH_SCAN_SAT_START,GxMsgProperty_ScanSatStart)
DEF_APP_SEND_MSG_ASYN(gxmsg_search_scan_server_asyn,GXMSG_SEARCH_SCAN_NET_START,GxMsgProperty_ScanNetStart)

DEF_APP_SEND_MSG_ASYN(gxmsg_search_scan_cable_asyn,GXMSG_SEARCH_SCAN_CABLE_START,GxMsgProperty_ScanCableStart)

DEF_APP_SEND_MSG_ASYN(gxmsg_search_scan_t2_asyn,GXMSG_SEARCH_SCAN_DVBT2_START,GxMsgProperty_ScanDvbt2Start)

DEF_APP_SEND_MSG_ASYN(gxmsg_search_scan_dtmb_asyn,GXMSG_SEARCH_SCAN_DTMB_START,GxMsgProperty_ScanDtmbStart)

DEF_APP_SEND_MSG_ASYN(gxmsg_search_scan_t_asyn,GXMSG_SEARCH_SCAN_T_START,GxMsgProperty_ScanTStart)

DEF_APP_SEND_MSG_ASYN_NOPARAM(gxmsg_search_scan_stop_asyn,GXMSG_SEARCH_SCAN_STOP)

DEF_APP_SEND_MSG_SYNC_NOPARAM(gxmsg_search_scan_save_sync,GXMSG_SEARCH_SAVE)

DEF_APP_SEND_MSG_SYNC_NOPARAM(gxmsg_search_scan_not_save_asyn,GXMSG_SEARCH_NOT_SAVE)

DEF_APP_SEND_MSG_ASYN(gxmsg_extra_sync_time_asyn,GXMSG_EXTRA_SYNC_TIME, GxMsgProperty_ExtraSyncTime)

DEF_APP_SEND_MSG_ASYN_NOPARAM(gxmsg_extra_release,GXMSG_EXTRA_RELEASE)

DEF_APP_SEND_MSG_SYNC(gxmsg_frontend_set_tp,GXMSG_FRONTEND_SET_TP,GxMsgProperty_FrontendSetTp)

DEF_APP_SEND_MSG_SYNC(gxmsg_frontend_set_diseqc,GXMSG_FRONTEND_SET_DISEQC,GxMsgProperty_FrontendSetDiseqc)

DEF_APP_SEND_MSG_ASYN(gxmsg_blind_scan_sat_asyn,GXMSG_SEARCH_BLIND_SCAN_START,GxMsgProperty_BlindScanStart)

DEF_APP_SEND_MSG_ASYN_NOPARAM(gxmsg_blind_scan_stop_asyn,GXMSG_SEARCH_BLIND_SCAN_STOP)

DEF_APP_SEND_MSG_SYNC(gxmsg_frontend_monitor_stop_sync,GXMSG_FRONTEND_STOP_MONITOR, GxMsgProperty_FrontendMonitor)

DEF_APP_SEND_MSG_SYNC(gxmsg_frontend_monitor_start_sync,GXMSG_FRONTEND_START_MONITOR, GxMsgProperty_FrontendMonitor)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_subtitle_load_sync,GXMSG_PLAYER_SUBTITLE_LOAD,GxMsgProperty_PlayerSubtitleLoad)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_subtitle_unload_sync,GXMSG_PLAYER_SUBTITLE_UNLOAD,GxMsgProperty_PlayerSubtitleUnLoad)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_subtitle_switch_sync,GXMSG_PLAYER_SUBTITLE_SWITCH,GxMsgProperty_PlayerSubtitleSwitch)

DEF_APP_SEND_MSG_SYNC(gxmsg_update_open_sync,GXMSG_UPDATE_OPEN,GxMsgProperty_UpdateOpen)


DEF_APP_SEND_MSG_SYNC_NOPARAM(gxmsg_update_start_sync,GXMSG_UPDATE_START)

DEF_APP_SEND_MSG_SYNC_NOPARAM(gxmsg_update_stop_sync,GXMSG_UPDATE_STOP)

DEF_APP_SEND_MSG_SYNC(gxmsg_update_status_sync,GXMSG_UPDATE_STATUS,GxMsgProperty_UpdateStatus)

DEF_APP_SEND_MSG_SYNC(gxmsg_update_protocol_select_sync,GXMSG_UPDATE_PROTOCOL_SELECT,GxMsgProperty_UpdateProtocolSelect)

DEF_APP_SEND_MSG_SYNC(gxmsg_update_protocol_ctrl_sync,GXMSG_UPDATE_PROTOCOL_CTRL,GxUpdate_IoCtrl)

DEF_APP_SEND_MSG_SYNC(gxmsg_update_partition_select_sync,GXMSG_UPDATE_PARTITION_SELECT,GxMsgProperty_UpdatePartitionSelect)

DEF_APP_SEND_MSG_SYNC(gxmsg_update_partition_ctrl_sync,GXMSG_UPDATE_PARTITION_CTRL,GxUpdate_IoCtrl)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_audio_track_sync,GXMSG_PLAYER_AUDIO_TRACK,GxMsgProperty_PlayerAudioTrack)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_audio_switch_sync,GXMSG_PLAYER_AUDIO_SWITCH,GxMsgProperty_PlayerAudioSwitch)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_audio_mode_config,GXMSG_PLAYER_AUDIO_MODE_CONFIG,GxMsgProperty_PlayerAudioModeConfig)

DEF_APP_SEND_MSG_SYNC(gxmsg_player_time_shift_aync,GXMSG_PLAYER_TIME_SHIFT,GxMsgProperty_PlayerTimeShift)

DEF_APP_SEND_MSG_ASYN(gxmsg_player_multiscreen_play_sync,GXMSG_PLAYER_MULTISCREEN_PLAY,GxMsgProperty_PlayerMultiScreenPlay)

// for AC3 OUTPUT control
DEF_APP_SEND_MSG_SYNC(gxmsg_player_audio_ac3_bypass,GXMSG_PLAYER_AUDIO_AC3_MODE,GxMsgProperty_PlayerAudioAC3Mode)

//DEF_APP_SEND_MSG_SYNC(gxmsg_player_video_mosaic_drop,GXMSG_PLAYER_VIDEO_MOSAIC_DROP,GxMsgProperty_PlayerVideoMosaicDrop)

DEF_APP_SEND_MSG_ASYN(gxmsg_subtitle_init, GXMSG_SUBTITLE_INIT, GxMsgProperty_AppSubtitle)
DEF_APP_SEND_MSG_ASYN(gxmsg_subtitle_destroy, GXMSG_SUBTITLE_DESTROY, GxMsgProperty_AppSubtitle)
DEF_APP_SEND_MSG_ASYN(gxmsg_subtitle_hide, GXMSG_SUBTITLE_HIDE, GxMsgProperty_AppSubtitle)
DEF_APP_SEND_MSG_ASYN(gxmsg_subtitle_show, GXMSG_SUBTITLE_SHOW, GxMsgProperty_AppSubtitle)
DEF_APP_SEND_MSG_ASYN(gxmsg_subtitle_draw, GXMSG_SUBTITLE_DRAW, GxMsgProperty_AppSubtitle)
DEF_APP_SEND_MSG_ASYN(gxmsg_subtitle_clean, GXMSG_SUBTITLE_CLEAN, GxMsgProperty_AppSubtitle)

#if NETWORK_SUPPORT
//network
DEF_APP_SEND_MSG_SYNC_NOPARAM(gxmsg_network_start_sync,GXMSG_NETWORK_START)
DEF_APP_SEND_MSG_SYNC_NOPARAM(gxmsg_network_stop_sync,GXMSG_NETWORK_STOP)

DEF_APP_SEND_MSG_SYNC(gxmsg_network_dev_list_sync, GXMSG_NETWORK_DEV_LIST, GxMsgProperty_NetDevList)

DEF_APP_SEND_MSG_SYNC(gxmsg_network_get_state_sync, GXMSG_NETWORK_GET_STATE, GxMsgProperty_NetDevState)

DEF_APP_SEND_MSG_SYNC(gxmsg_network_get_type_sync, GXMSG_NETWORK_GET_TYPE, GxMsgProperty_NetDevType)
DEF_APP_SEND_MSG_SYNC(gxmsg_network_get_cur_dev_sync, GXMSG_NETWORK_GET_CUR_DEV, ubyte64)

DEF_APP_SEND_MSG_ASYN(gxmsg_network_set_mac_asyn, GXMSG_NETWORK_SET_MAC, GxMsgProperty_NetDevMac)
DEF_APP_SEND_MSG_SYNC(gxmsg_network_get_mac_sync, GXMSG_NETWORK_GET_MAC, GxMsgProperty_NetDevMac)

DEF_APP_SEND_MSG_ASYN(gxmsg_network_set_ipcfg_asyn, GXMSG_NETWORK_SET_IPCFG, GxMsgProperty_NetDevIpcfg)
DEF_APP_SEND_MSG_SYNC(gxmsg_network_get_ipcfg_sync, GXMSG_NETWORK_GET_IPCFG, GxMsgProperty_NetDevIpcfg)

DEF_APP_SEND_MSG_ASYN(gxmsg_network_set_mode_asyn, GXMSG_NETWORK_SET_MODE, GxMsgProperty_NetDevMode)
DEF_APP_SEND_MSG_SYNC(gxmsg_network_get_mode_sync, GXMSG_NETWORK_GET_MODE, GxMsgProperty_NetDevMode)

DEF_APP_SEND_MSG_ASYN(gxmsg_wifi_scan_ap_asyn, GXMSG_WIFI_SCAN_AP, ubyte64)
DEF_APP_SEND_MSG_ASYN(gxmsg_wifi_scan_hide_ap_asyn, GXMSG_WIFI_SCAN_HIDDEN_AP, GxMsgProperty_ScanHideAp)
DEF_APP_SEND_MSG_SYNC(gxmsg_wifi_get_cur_ap_sync, GXMSG_WIFI_GET_CUR_AP, GxMsgProperty_CurAp)
DEF_APP_SEND_MSG_ASYN(gxmsg_wifi_set_cur_ap_asyn, GXMSG_WIFI_SET_CUR_AP, GxMsgProperty_CurAp)
DEF_APP_SEND_MSG_ASYN(gxmsg_wifi_del_cur_ap_asyn, GXMSG_WIFI_DEL_CUR_AP, ubyte64)

DEF_APP_SEND_MSG_ASYN(gxmsg_3g_set_param, GXMSG_3G_SET_PARAM, GxMsgProperty_3G)
DEF_APP_SEND_MSG_SYNC(gxmsg_3g_get_param, GXMSG_3G_GET_PARAM, GxMsgProperty_3G)
DEF_APP_SEND_MSG_SYNC(gxmsg_3g_get_cur_operator, GXMSG_3G_GET_CUR_OPERATOR, GxMsgProperty_3G_Operator)

DEF_APP_SEND_MSG_ASYN(gxmsg_pppoe_set_param, GXMSG_PPPOE_SET_PARAM, GxMsgProperty_PppoeCfg)
DEF_APP_SEND_MSG_SYNC(gxmsg_pppoe_get_param, GXMSG_PPPOE_GET_PARAM, GxMsgProperty_PppoeCfg)

#if BLUETOOTH_SUPPORT
DEF_APP_SEND_MSG_ASYN(gxmsg_bt_scan_ap_asyn, GXMSG_BLUETOOTH_SCAN_AP, ubyte64)
DEF_APP_SEND_MSG_SYNC(gxmsg_bt_get_cur_ap_sync, GXMSG_BLUETOOTH_GET_CUR_AP, GxMsgProperty_CurAp)
DEF_APP_SEND_MSG_ASYN(gxmsg_bt_set_cur_ap_asyn, GXMSG_BLUETOOTH_SET_CUR_AP, GxMsgProperty_CurAp)
DEF_APP_SEND_MSG_ASYN(gxmsg_bt_del_cur_ap_asyn, GXMSG_BLUETOOTH_DEL_CUR_AP, ubyte64)
DEF_APP_SEND_MSG_ASYN(appmsg_bluetooth_ui_control_asyn, APPMSG_BLUETOOTH_UI_CONTROL, ubyte64)
#endif
#if NETAPPS_SUPPORT
DEF_APP_SEND_MSG_ASYN(appmsg_iptv_get_list, APPMSG_IPTV_GET_LIST, AppMsg_IptvList)
DEF_APP_SEND_MSG_ASYN(appmsg_iptv_list_done, APPMSG_IPTV_LIST_DONE, AppMsg_IptvList)
DEF_APP_SEND_MSG_SYNC(appmsg_iptv_update_local_list, APPMSG_IPTV_UPDATE_LOCAL_LIST, char*)
DEF_APP_SEND_MSG_ASYN(appmsg_iptv_get_ad_pic, APPMSG_IPTV_GET_AD_PIC, char*)
DEF_APP_SEND_MSG_ASYN(appmsg_iptv_ad_pic_done, APPMSG_IPTV_AD_PIC_DONE, char*)
#endif

#if VPN_SUPPORT
DEF_APP_SEND_MSG_SYNC(appmsg_vpn_get_state, APPMSG_VPN_GET_STATE, AppMsgProperty_VpnState)
DEF_APP_SEND_MSG_ASYN_NOPARAM(appmsg_vpn_start, APPMSG_VPN_START)
DEF_APP_SEND_MSG_ASYN_NOPARAM(appmsg_vpn_stop, APPMSG_VPN_STOP)
DEF_APP_SEND_MSG_ASYN(appmsg_vpn_set_auto, APPMSG_VPN_SET_AUTO, AppMsgProperty_VpnAuto)
DEF_APP_SEND_MSG_SYNC(appmsg_vpn_get_auto, APPMSG_VPN_GET_AUTO, AppMsgProperty_VpnAuto)
DEF_APP_SEND_MSG_ASYN(appmsg_vpn_set_param, APPMSG_VPN_SET_PARAM, AppMsgProperty_VpnInfo)
DEF_APP_SEND_MSG_SYNC(appmsg_vpn_get_param, APPMSG_VPN_GET_PARAM, AppMsgProperty_VpnInfo)
#endif
#endif

#if TKGS_SUPPORT
DEF_APP_SEND_MSG_SYNC(gxmsg_tkgs_check_by_pmt, GXMSG_TKGS_CHECK_BY_PMT, GxTkgsPmtCheckParm)
DEF_APP_SEND_MSG_SYNC(gxmsg_tkgs_check_by_tp, GXMSG_TKGS_CHECK_BY_TP, GxTkgsTpCheckParm)
DEF_APP_SEND_MSG_ASYN(gxmsg_tkgs_start_version_check, GXMSG_TKGS_START_VERSION_CHECK, GxTkgsUpdateParm)
DEF_APP_SEND_MSG_ASYN(gxmsg_tkgs_start_update, GXMSG_TKGS_START_UPDATE, GxTkgsUpdateParm)
DEF_APP_SEND_MSG_SYNC_NOPARAM(gxmsg_tkgs_stop,GXMSG_TKGS_STOP)
DEF_APP_SEND_MSG_SYNC(gxmsg_tkgs_set_location, GXMSG_TKGS_SET_LOCATION, GxTkgsLocation)
DEF_APP_SEND_MSG_SYNC(gxmsg_tkgs_get_location, GXMSG_TKGS_GET_LOCATION, GxTkgsLocation)
DEF_APP_SEND_MSG_SYNC(gxmsg_tkgs_set_operating_mode, GXMSG_TKGS_SET_OPERATING_MODE, GxTkgsOperatingMode)
DEF_APP_SEND_MSG_SYNC(gxmsg_tkgs_get_operating_mode, GXMSG_TKGS_GET_OPERATING_MODE, GxTkgsOperatingMode)
DEF_APP_SEND_MSG_SYNC(gxmsg_tkgs_get_preferred_lists, GXMSG_TKGS_GET_PREFERRED_LISTS, GxTkgsPreferredList)
DEF_APP_SEND_MSG_SYNC(gxmsg_tkgs_set_preferred_list, GXMSG_TKGS_SET_PREFERRED_LIST, ubyte64)
DEF_APP_SEND_MSG_SYNC(gxmsg_tkgs_get_preferred_list, GXMSG_TKGS_GET_PREFERRED_LIST, ubyte64)

DEF_APP_SEND_MSG_SYNC(gxmsg_tkgs_blocking_check, GXMSG_TKGS_BLOCKING_CHECK, GxTkgsBlockCheckParm)
DEF_APP_SEND_MSG_SYNC(gxmsg_tkgs_test_get_vernum, GXMSG_TKGS_TEST_GET_VERNUM, int)
DEF_APP_SEND_MSG_SYNC_NOPARAM(gxmsg_tkgs_test_reset_vernum, GXMSG_TKGS_TEST_RESET_VERNUM)
DEF_APP_SEND_MSG_SYNC_NOPARAM(gxmsg_tkgs_test_clear_hidden_locations_list, GXMSG_TKGS_TEST_CLEAR_HIDDEN_LOCATIONS_LIST)
DEF_APP_SEND_MSG_SYNC_NOPARAM(gxmsg_tkgs_test_clear_visible_locations_list, GXMSG_TKGS_TEST_CLEAR_VISIBLE_LOCATIONS_LIST)
#endif

#if SAT_FINDER_SUPPORT
    DEF_APP_SEND_MSG_ASYN(gxmsg_sat_find_user3, GXMSG_SAT_FIND_USER3, dream_ui_msg_t)
#endif
#if PLUGINSTORE_SUPPORT
    DEF_APP_SEND_MSG_ASYN(appmsg_plugin_source_ui_control_asyn, APPMSG_PLUGIN_SOURCE_UI_CONTROL, struct PlugiaSourceUIMsg)
#endif
#if NETAPPS_SUPPORT
DEF_APP_SEND_MSG_ASYN(appmsg_netvideo_start_play_asyn, APPMSG_NETVIDEO_PLAY_START, AppMsgVideoPlayer)
#endif

#if WEBLET_SUPPORT
DEF_APP_SEND_MSG_SYNC(appmsg_weblet_ui_control_sync, APPMSG_WEBLET_UI_CONTROL, AppMsg_WebletUIControl)
#endif
#if WEBAPI_SUPPORT
DEF_APP_SEND_MSG_SYNC(appmsg_webapi_ui_control_sync, APPMSG_WEBAPI_UI_CONTROL, AppMsg_WebapiUIControl)
#endif

#if SAT2IP_SERVER_SUPPORT
DEF_APP_SEND_MSG_SYNC(gxmsg_satip_service_start, GXMSG_SATIP_SERVICE_START, SatipStartMsg)
DEF_APP_SEND_MSG_SYNC(gxmsg_satip_service_stop, GXMSG_SATIP_SERVICE_STOP, int)
DEF_APP_SEND_MSG_SYNC_NOPARAM(gxmsg_satip_dialog_start, GXMSG_SATIP_DIALOG_START)
DEF_APP_SEND_MSG_SYNC_NOPARAM(gxmsg_satip_dialog_stop, GXMSG_SATIP_DIALOG_STOP)
DEF_APP_SEND_MSG_SYNC(gxmsg_satip_dialog_pause, GXMSG_SATIP_DIALOG_PAUSE, int)
DEF_APP_SEND_MSG_SYNC_NOPARAM(gxmsg_satip_dialog_resume, GXMSG_SATIP_DIALOG_RESUME)
DEF_APP_SEND_MSG_SYNC(gxmsg_satip_play_start, GXMSG_SATIP_PLAY_START, int)
DEF_APP_SEND_MSG_SYNC(gxmsg_satip_play_stop, GXMSG_SATIP_PLAY_STOP, int)
#endif

#if DVB2IP_SERVER_SUPPORT
DEF_APP_SEND_MSG_SYNC(gxmsg_dvb2ip_play_start, GXMSG_DVB2IP_PLAY_START, Dvb2ipStartMsg)
DEF_APP_SEND_MSG_ASYN_NOPARAM(gxmsg_dvb2ip_play_stop, GXMSG_DVB2IP_PLAY_STOP)
#endif

#if DLNADMR_SUPPORT || DLNADMP_SUPPORT || DLNASAT2IP_SUPPORT || DLNADMS_SUPPORT
DEF_APP_SEND_MSG_ASYN(appmsg_dlna_ui_control_asyn, APPMSG_DLNA_UI_CONTROL, AppMsg_DlnaUIControl)
#endif
#ifdef SUBTTRANS_SUPPORT
DEF_APP_SEND_MSG_ASYN(appmsg_subt_trans_control_asyn, APPMSG_SUBT_TRANS_CONTROL, AppMsg_SubtTransControl)
#endif

#if USB_TEST_SUPPORT
DEF_APP_SEND_MSG_ASYN(appmsg_usb_test_asyn, APPMSG_USB_TEST, GxMsgProperty_UsbTest)
#endif

DEF_APP_SEND_MSG_ASYN(appmsg_message_asyn, APPMSG_MESSAGE, AppMsgMessageClass)

#if HDMI_CEC_SUPPORT
DEF_APP_SEND_MSG_ASYN_NOPARAM(gxmsg_hdmi_cec_on, GXMSG_HDMI_CEC_ON)
DEF_APP_SEND_MSG_ASYN_NOPARAM(gxmsg_hdmi_cec_off, GXMSG_HDMI_CEC_OFF)
DEF_APP_SEND_MSG_ASYN(gxmsg_hdmi_cec_write, GXMSG_HDMI_CEC_WRITE, CEC_CMD)
DEF_APP_SEND_MSG_ASYN(gxmsg_hdmi_read_done, GXMSG_HDMI_CEC_READ_DONE, GxHdmiCecMessage)
DEF_APP_SEND_MSG_ASYN(appmsg_hdmi_cec_ui_control_asyn, APPMSG_HDMI_CEC_UI_CONTROL, AppMsg_CecUIControl)
#endif

static msg_func app_send_msg[GXMAX_MSG_NUM] = {0};
static MsgIdFunc app_msg_func_map[] =
{
    {GXMSG_PLAYER_PLAY, gxmsg_player_play_asyn},
    {GXMSG_PLAYER_STOP, gxmsg_player_stop_asyn},
    {GXMSG_PLAYER_PAUSE, gxmsg_player_pause_asyn},
    {GXMSG_PLAYER_RESUME, gxmsg_player_resume_asyn},
    {GXMSG_PLAYER_SEEK, gxmsg_player_seek_asyn},
    {GXMSG_PLAYER_SPEED, gxmsg_player_speed_asyn},
    {GXMSG_PLAYER_RECORD, gxmsg_player_record_asyn},
    {GXMSG_PLAYER_VIDEO_COLOR, gxmsg_player_video_color_asyn},
    {GXMSG_PLAYER_VIDEO_CLIP, gxmsg_player_video_clip_asyn},
    {GXMSG_PLAYER_VIDEO_WINDOW, gxmsg_player_video_window_sync},
    {GXMSG_PLAYER_VIDEO_ASPECT, gxmsg_player_video_aspect_sync},
    {GXMSG_PLAYER_VIDEO_INTERFACE, gxmsg_player_video_interface_sync},
    {GXMSG_PLAYER_VIDEO_MODE_CONFIG, gxmsg_player_video_mode_config_sync},
    {GXMSG_PLAYER_VIDEO_AUTO_ADAPT, gxmsg_player_video_auto_adapt_asyn},
    {GXMSG_PLAYER_VIDEO_HIDE, gxmsg_player_video_hide_sync},
    {GXMSG_PLAYER_VIDEO_SHOW, gxmsg_player_video_show_sync},
    {GXMSG_PLAYER_AUDIO_MUTE, gxmsg_player_audio_mute_asyn},
    {GXMSG_PLAYER_AUDIO_VOLUME, gxmsg_player_audio_volume_asyn},
    {GXMSG_PLAYER_AUDIO_TRACK, gxmsg_player_audio_track_sync},
    {GXMSG_PLAYER_AUDIO_SWITCH, gxmsg_player_audio_switch_sync},
    {GXMSG_PLAYER_AUDIO_MODE_CONFIG, gxmsg_player_audio_mode_config},
    {GXMSG_PLAYER_AUDIO_AC3_MODE, gxmsg_player_audio_ac3_bypass},
    {GXMSG_PLAYER_SUBTITLE_LOAD, gxmsg_player_subtitle_load_sync},
    {GXMSG_PLAYER_SUBTITLE_UNLOAD, gxmsg_player_subtitle_unload_sync},
    {GXMSG_PLAYER_SUBTITLE_SWITCH, gxmsg_player_subtitle_switch_sync},
    {GXMSG_PLAYER_PVR_CONFIG, gxmsg_player_pvr_config_sync},
    {GXMSG_PLAYER_DISPLAY_SCREEN, gxmsg_player_display_screen_sync},
    {GXMSG_PLAYER_TIME_SHIFT, gxmsg_player_time_shift_aync},
    {GXMSG_PLAYER_MULTISCREEN_PLAY, gxmsg_player_multiscreen_play_sync},
    {GXMSG_PLAYER_FREEZE_FRAME_SWITCH, gxmsg_player_freeze_frame_switch_sync},
    {GXMSG_PLAYER_TRACK_ADD, gxmsg_player_add_pid_record},
    {GXMSG_PLAYER_BANDWIDTH_SWITCH, gxmsg_player_bandwidth_switch},
    {GXMSG_PLAYER_AD_AUDIO_ENABLE, gxmsg_player_ad_audio_enable_sync},
    {GXMSG_PLAYER_AD_AUDIO_DISABLE, gxmsg_player_ad_audio_disable_sync},
    {GXMSG_FRONTEND_STOP_MONITOR, gxmsg_frontend_monitor_stop_sync},
    {GXMSG_FRONTEND_START_MONITOR, gxmsg_frontend_monitor_start_sync},
    {GXMSG_FRONTEND_SET_TP, gxmsg_frontend_set_tp},
    {GXMSG_FRONTEND_SET_DISEQC, gxmsg_frontend_set_diseqc},
    {GXMSG_SI_SUBTABLE_GET, gxmsg_si_subtable_get_sync},
    {GXMSG_EPG_FILTER_NUM_GET, gxmsg_epg_filter_num_get_sync},
    {GXMSG_EPG_FILTER_INFO_GET, gxmsg_epg_filter_info_get_sync},
    {GXMSG_EPG_FILTER_MODIFY, gxmsg_epg_filter_modify_sync},
#if EPG_REALTIMER_GET_DETAIL_SUPPORT
    {GXMSG_EPG_DETAIL_FILTER_SET, gxmsg_epg_detail_filter_set_sync},
    {GXMSG_EPG_DETAIL_FILTER_CLEAN, gxmsg_epg_detail_filter_clean_sync},
#endif
    {GXMSG_SI_SUBTABLE_CREATE, gxmsg_si_subtable_create_sync},
    {GXMSG_SI_SUBTABLE_MODIFY, gxmsg_si_subtable_modify_sync},
    {GXMSG_SI_SUBTABLE_RELEASE, gxmsg_si_subtable_release_sync},
    {GXMSG_SI_SUBTABLE_START, gxmsg_si_subtable_start_asyn},
    {GXMSG_EPG_NUM_GET, gxmsg_epg_num_get_sync},
    {GXMSG_EPG_NUM_GET_BY_PERIOD, gxmsg_epg_num_get_by_period_sync},
    {GXMSG_EPG_GET, gxmsg_epg_get_sync},
    {GXMSG_EPG_GET_BY_PERIOD, gxmsg_epg_get_by_period_sync},
    {GXMSG_EPG_GET_BY_EVENT_ID, gxmsg_epg_get_by_event_id_sync},
    {GXMSG_EPG_PF_NUM_GET, gxmsg_epg_pf_num_get_sync},
    {GXMSG_EPG_PF_GET, gxmsg_epg_pf_get_sync},
    {GXMSG_EPG_CREATE, gxmsg_epg_create_sync},
    {GXMSG_EPG_CLEAN, gxmsg_epg_clean_asyn},
    {GXMSG_EPG_RESET, gxmsg_epg_reset_asyn},
    {GXMSG_EPG_RELEASE, gxmsg_epg_release_sync},
    {GXMSG_EPG_FILTER_START, gxmsg_epg_filter_start_sync},
    {GXMSG_EPG_FILTER_STOP, gxmsg_epg_filter_stop_sync},
    {GXMSG_EPG_CFG_GET, gxmsg_epg_cfg_get_sync},
    {GXMSG_EPG_FULL, gxmsg_epg_full_sync},
    {GXMSG_EPG_FULL_SPEED, gxmsg_epg_full_speed_sync},
    {GXMSG_EPG_SECTION_FLAG_RELEASE, gxmsg_epg_section_flag_release_sync},
    {GXMSG_BOOK_GET, gxmsg_book_get_sync},
    {GXMSG_BOOK_CREATE, gxmsg_book_create_sync},
    {GXMSG_BOOK_MODIFY, gxmsg_book_modify_sync},
    {GXMSG_BOOK_DESTROY, gxmsg_book_destroy_sync},
    {GXMSG_BOOK_START, gxmsg_book_start_asyn},
    {GXMSG_BOOK_STOP, gxmsg_book_stop_asyn},
    {GXMSG_BOOK_RESET, gxmsg_book_reset_asyn},
    {GXMSG_SEARCH_SCAN_SAT_START, gxmsg_search_scan_sat_asyn},
    {GXMSG_SEARCH_SCAN_NET_START, gxmsg_search_scan_server_asyn},
    {GXMSG_SEARCH_SCAN_CABLE_START, gxmsg_search_scan_cable_asyn},
    {GXMSG_SEARCH_SCAN_DVBT2_START, gxmsg_search_scan_t2_asyn},
    {GXMSG_SEARCH_SCAN_DTMB_START,gxmsg_search_scan_dtmb_asyn},
    {GXMSG_SEARCH_SCAN_T_START, gxmsg_search_scan_t_asyn},
    {GXMSG_SEARCH_SCAN_STOP, gxmsg_search_scan_stop_asyn},
    {GXMSG_SEARCH_SAVE, gxmsg_search_scan_save_sync},
    {GXMSG_SEARCH_NOT_SAVE, gxmsg_search_scan_not_save_asyn},
    {GXMSG_SEARCH_BLIND_SCAN_START, gxmsg_blind_scan_sat_asyn},
    {GXMSG_SEARCH_BLIND_SCAN_STOP, gxmsg_blind_scan_stop_asyn},
    {GXMSG_EXTRA_SYNC_TIME, gxmsg_extra_sync_time_asyn},
    {GXMSG_EXTRA_RELEASE, gxmsg_extra_release},
    {GXMSG_UPDATE_OPEN, gxmsg_update_open_sync},
    {GXMSG_UPDATE_START, gxmsg_update_start_sync},
    {GXMSG_UPDATE_STOP, gxmsg_update_stop_sync},
    {GXMSG_UPDATE_STATUS, gxmsg_update_status_sync},
    {GXMSG_UPDATE_PROTOCOL_SELECT, gxmsg_update_protocol_select_sync},
    {GXMSG_UPDATE_PROTOCOL_CTRL, gxmsg_update_protocol_ctrl_sync},
    {GXMSG_UPDATE_PARTITION_SELECT, gxmsg_update_partition_select_sync},
    {GXMSG_UPDATE_PARTITION_CTRL, gxmsg_update_partition_ctrl_sync},
    {GXMSG_PM_OPEN, gxmsg_pm_open},
    {GXMSG_PM_CLOSE, gxmsg_pm_close},
    {GXMSG_PM_NODE_NUM_GET, gxmsg_node_num_get},
    {GXMSG_PM_NODE_BY_POS_GET, gxmsg_node_by_pos_get},
    {GXMSG_PM_MULTI_NODE_BY_POS_GET, gxmsg_multi_node_by_pos_get},
    {GXMSG_PM_NODE_BY_ID_GET, gxmsg_node_by_id_get},
    {GXMSG_PM_NODE_ADD, gxmsg_node_add},
    {GXMSG_PM_NODE_MODIFY, gxmsg_node_modify},
    {GXMSG_PM_NODE_DELETE, gxmsg_node_delete},
    {GXMSG_PM_NODE_GET_URL, gxmsg_node_get_pm_url},
    {GXMSG_PM_LOAD_DEFAULT, gxmsg_pm_load_default},
    {GXMSG_TUNER_COPY, gxmsg_tuner_copy},
    {GXMSG_PM_VIEWINFO_SET, gxmsg_pm_viewinfo_set},
    {GXMSG_PM_VIEWINFO_GET, gxmsg_pm_viewinfo_get},
    {GXMSG_CHECK_PM_EXIST, gxmsg_chk_node_exist},
    {GXMSG_EMPTY_SERVICE_MSG, gxmsg_empty_service_msg},
    {GXMSG_GET_MEDIA_TIME, gxmsg_get_media_time},
    {GXMSG_AV_OUTPUT_OFF, gxmsg_av_output_off},
    {GXMSG_SUBTITLE_INIT, gxmsg_subtitle_init},
    {GXMSG_SUBTITLE_DESTROY, gxmsg_subtitle_destroy},
    {GXMSG_SUBTITLE_HIDE, gxmsg_subtitle_hide},
    {GXMSG_SUBTITLE_SHOW, gxmsg_subtitle_show},
    {GXMSG_SUBTITLE_DRAW, gxmsg_subtitle_draw},
    {GXMSG_SUBTITLE_CLEAN, gxmsg_subtitle_clean},

#if NETWORK_SUPPORT
    {GXMSG_NETWORK_START,gxmsg_network_start_sync},
    {GXMSG_NETWORK_STOP,gxmsg_network_stop_sync},
    {GXMSG_NETWORK_DEV_LIST, gxmsg_network_dev_list_sync},
    {GXMSG_NETWORK_GET_STATE, gxmsg_network_get_state_sync},
    {GXMSG_NETWORK_GET_TYPE, gxmsg_network_get_type_sync},
    {GXMSG_NETWORK_GET_CUR_DEV, gxmsg_network_get_cur_dev_sync},
    {GXMSG_NETWORK_SET_MAC, gxmsg_network_set_mac_asyn},
    {GXMSG_NETWORK_GET_MAC, gxmsg_network_get_mac_sync},
    {GXMSG_NETWORK_SET_IPCFG, gxmsg_network_set_ipcfg_asyn},
    {GXMSG_NETWORK_GET_IPCFG, gxmsg_network_get_ipcfg_sync},
    {GXMSG_NETWORK_SET_MODE, gxmsg_network_set_mode_asyn},
    {GXMSG_NETWORK_GET_MODE, gxmsg_network_get_mode_sync},
    {GXMSG_WIFI_SCAN_AP, gxmsg_wifi_scan_ap_asyn},
    {GXMSG_WIFI_SCAN_HIDDEN_AP, gxmsg_wifi_scan_hide_ap_asyn},
    {GXMSG_WIFI_GET_CUR_AP, gxmsg_wifi_get_cur_ap_sync},
    {GXMSG_WIFI_SET_CUR_AP, gxmsg_wifi_set_cur_ap_asyn},
    {GXMSG_WIFI_DEL_CUR_AP, gxmsg_wifi_del_cur_ap_asyn},
    {GXMSG_3G_SET_PARAM, gxmsg_3g_set_param},
    {GXMSG_3G_GET_PARAM, gxmsg_3g_get_param},
    {GXMSG_3G_GET_CUR_OPERATOR, gxmsg_3g_get_cur_operator},
    {GXMSG_PPPOE_SET_PARAM, gxmsg_pppoe_set_param},
    {GXMSG_PPPOE_GET_PARAM, gxmsg_pppoe_get_param},
#if BLUETOOTH_SUPPORT
    {GXMSG_BLUETOOTH_SCAN_AP, gxmsg_bt_scan_ap_asyn},
    {GXMSG_BLUETOOTH_GET_CUR_AP, gxmsg_bt_get_cur_ap_sync},
    {GXMSG_BLUETOOTH_SET_CUR_AP, gxmsg_bt_set_cur_ap_asyn},
    {GXMSG_BLUETOOTH_DEL_CUR_AP, gxmsg_bt_del_cur_ap_asyn},
    {APPMSG_BLUETOOTH_UI_CONTROL, appmsg_bluetooth_ui_control_asyn},
#endif
#if NETAPPS_SUPPORT
    {APPMSG_IPTV_GET_LIST, appmsg_iptv_get_list},
    {APPMSG_IPTV_LIST_DONE, appmsg_iptv_list_done},
    {APPMSG_IPTV_UPDATE_LOCAL_LIST, appmsg_iptv_update_local_list},
    {APPMSG_IPTV_GET_AD_PIC, appmsg_iptv_get_ad_pic},
    {APPMSG_IPTV_AD_PIC_DONE, appmsg_iptv_ad_pic_done},
#endif

#if VPN_SUPPORT
    {APPMSG_VPN_GET_STATE, appmsg_vpn_get_state},
    {APPMSG_VPN_START, appmsg_vpn_start},
    {APPMSG_VPN_STOP, appmsg_vpn_stop},
    {APPMSG_VPN_SET_AUTO, appmsg_vpn_set_auto},
    {APPMSG_VPN_GET_AUTO, appmsg_vpn_get_auto},
    {APPMSG_VPN_SET_PARAM, appmsg_vpn_set_param},
    {APPMSG_VPN_GET_PARAM, appmsg_vpn_get_param},
#endif
#endif

#if TKGS_SUPPORT
	{GXMSG_TKGS_CHECK_BY_PMT, gxmsg_tkgs_check_by_pmt},
	{GXMSG_TKGS_CHECK_BY_TP, gxmsg_tkgs_check_by_tp},
	{GXMSG_TKGS_START_VERSION_CHECK, gxmsg_tkgs_start_version_check},
	{GXMSG_TKGS_START_UPDATE, gxmsg_tkgs_start_update},
	{GXMSG_TKGS_STOP, gxmsg_tkgs_stop},
	{GXMSG_TKGS_SET_LOCATION, gxmsg_tkgs_set_location},
	{GXMSG_TKGS_GET_LOCATION, gxmsg_tkgs_get_location},
	{GXMSG_TKGS_SET_OPERATING_MODE, gxmsg_tkgs_set_operating_mode},
	{GXMSG_TKGS_GET_OPERATING_MODE, gxmsg_tkgs_get_operating_mode},
	{GXMSG_TKGS_GET_PREFERRED_LISTS, gxmsg_tkgs_get_preferred_lists},
	{GXMSG_TKGS_SET_PREFERRED_LIST, gxmsg_tkgs_set_preferred_list},
	{GXMSG_TKGS_GET_PREFERRED_LIST, gxmsg_tkgs_get_preferred_list},
	{GXMSG_TKGS_BLOCKING_CHECK, gxmsg_tkgs_blocking_check},
	{GXMSG_TKGS_TEST_GET_VERNUM, gxmsg_tkgs_test_get_vernum},
	{GXMSG_TKGS_TEST_RESET_VERNUM, gxmsg_tkgs_test_reset_vernum},
	{GXMSG_TKGS_TEST_CLEAR_HIDDEN_LOCATIONS_LIST, gxmsg_tkgs_test_clear_hidden_locations_list},
	{GXMSG_TKGS_TEST_CLEAR_VISIBLE_LOCATIONS_LIST, gxmsg_tkgs_test_clear_visible_locations_list},
#endif
#if SAT_FINDER_SUPPORT
    {GXMSG_SAT_FIND_USER3, gxmsg_sat_find_user3},
#endif
#if PLUGINSTORE_SUPPORT
    {APPMSG_PLUGIN_SOURCE_UI_CONTROL, appmsg_plugin_source_ui_control_asyn},
#endif
#if NETAPPS_SUPPORT
    {APPMSG_NETVIDEO_PLAY_START,appmsg_netvideo_start_play_asyn},
#endif
#if WEBLET_SUPPORT
    {APPMSG_WEBLET_UI_CONTROL, appmsg_weblet_ui_control_sync},
#endif
#if WEBAPI_SUPPORT
    {APPMSG_WEBAPI_UI_CONTROL, appmsg_webapi_ui_control_sync},
#endif

#if SAT2IP_SERVER_SUPPORT
    {GXMSG_SATIP_SERVICE_START, gxmsg_satip_service_start},
    {GXMSG_SATIP_SERVICE_STOP, gxmsg_satip_service_stop},
    {GXMSG_SATIP_DIALOG_START, gxmsg_satip_dialog_start},
    {GXMSG_SATIP_DIALOG_STOP, gxmsg_satip_dialog_stop},
    {GXMSG_SATIP_DIALOG_PAUSE, gxmsg_satip_dialog_pause},
    {GXMSG_SATIP_DIALOG_RESUME, gxmsg_satip_dialog_resume},
    {GXMSG_SATIP_PLAY_START, gxmsg_satip_play_start},
    {GXMSG_SATIP_PLAY_STOP, gxmsg_satip_play_stop},
#endif
#if DVB2IP_SERVER_SUPPORT
    {GXMSG_DVB2IP_PLAY_START, gxmsg_dvb2ip_play_start},
    {GXMSG_DVB2IP_PLAY_STOP, gxmsg_dvb2ip_play_stop},
#endif
#if DLNADMR_SUPPORT || DLNADMP_SUPPORT || DLNASAT2IP_SUPPORT || DLNADMS_SUPPORT
    {APPMSG_DLNA_UI_CONTROL, appmsg_dlna_ui_control_asyn},
#endif
#ifdef SUBTTRANS_SUPPORT
    {APPMSG_SUBT_TRANS_CONTROL, appmsg_subt_trans_control_asyn},
#endif
#if USB_TEST_SUPPORT
    {APPMSG_USB_TEST, appmsg_usb_test_asyn},
#endif
    {APPMSG_MESSAGE, appmsg_message_asyn},
#if HDMI_CEC_SUPPORT
    {GXMSG_HDMI_CEC_ON, gxmsg_hdmi_cec_on},
    {GXMSG_HDMI_CEC_OFF, gxmsg_hdmi_cec_off},
    {GXMSG_HDMI_CEC_WRITE, gxmsg_hdmi_cec_write},
    {GXMSG_HDMI_CEC_READ_DONE, gxmsg_hdmi_read_done},
    {APPMSG_HDMI_CEC_UI_CONTROL, appmsg_hdmi_cec_ui_control_asyn},
#endif
};

status_t app_send_msg_init(void)
{
    int i = 0;

    for(i = 0; (i < sizeof(app_msg_func_map)/sizeof(MsgIdFunc)) && i < GXMAX_MSG_NUM; i++)
    {
        if(app_send_msg[app_msg_func_map[i].message_id] != NULL)
            APP_PRINT("!!!!!!!! Warnning: duplicate Message Function, msg_id = %d, map pos = %d !!!!!!!!!\n", app_msg_func_map[i].message_id, i);
        app_send_msg[app_msg_func_map[i].message_id] = app_msg_func_map[i].message_func;
    }
    return GXCORE_SUCCESS;
}

/**
 * @brief       ??????Ϣ
 * @param       msg_id:??Ϣ??id
                params??Ϣ?Ĳ???

 * @return      ??Ϣ???ͳɹ?????GXCORE_SUCCESS
                ??Ϣ????ʧ?ܷ???GXCORE_ERROR
 */
status_t app_send_msg_exec(uint32_t msg_id,void* params)
{
    if(GXMAX_MSG_NUM <= msg_id)
    {
        APP_PRINT("msg id is incorrect\n");
        return GXCORE_ERROR;
    }

    if(NULL== app_send_msg[msg_id])
    {
        APP_PRINT("the msg handling funtion is NULL msg_id = %d \n",msg_id);
        return GXCORE_ERROR;
    }

    return app_send_msg[msg_id](msg_id,params);
}
/* End of file -------------------------------------------------------------*/

