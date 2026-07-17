/*****************************************************************************
*                          CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :   app_cgcas_pin.c
* Author    :   huangwf
* Project   :   CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION      Date            AUTHOR         Description
  1.0      2021.03.19         huangwf             Creation
*****************************************************************************/

#include "app.h"
#include "app_module.h"
#ifdef CASCAM_CGCAS_SUPPORT
#include "cascam.h"
#include "cascam_ui_common.h"
#include "cascam_cryptoguardcas_data.h"
#include "app_cgcas_config.h"
#include "full_screen.h"

void app_cgcas_forcetune(Cascam_CGDelivery_descriptor *info)
{
    unsigned int count = 0;
    GxBusPmDataProg prog = {0};
    int i = 0;

    if(NULL == info)
    {
        printf("\n[%s][%d] ERROR, param=NULL\n", __FUNCTION__,__LINE__);
        return;
    }

    count = GxBus_PmProgNumGet();
    for(i = 0; i < count; i++)
    {
        GxBus_PmProgGetByPos(i, 1, &prog);
        if (info->DestinationSID == prog.service_id)
        {// find the same service
            GxMsgProperty_NodeByPosGet prog_get = {0};

            // TODO: need modify, notic the view group
            prog_get.node_type = NODE_PROG;
            prog_get.pos = g_AppPlayOps.normal_play.play_count;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_get));
            if (prog_get.prog_data.service_id != prog.service_id)
            {
                g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, i);
            }
            break;
        }
    }

}
#endif
