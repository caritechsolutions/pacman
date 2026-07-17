/*****************************************************************************
* 			  CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2010, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_msg.h
* Author    : 	shenbin
* Project   :	GoXceed -S
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2010.2.27	            shenbin         creation
*****************************************************************************/

#ifndef __APP_MSG_H__
#define __APP_MSG_H__
#include "gxmsg.h"
#include "service/gxplayer.h"
#include "service/gxextra.h"
#include "service/gxfrontend.h"
#include "service/gxepg.h"
#include "service/gxsearch.h"
#include "service/gxsi.h"
#include "app_config.h"

#if NETAPPS_SUPPORT
typedef enum
{
    NETAPPS_DOWN_STATE_FAILED,
    NETAPPS_DOWN_STATE_SUCCESS,
}NetappsDownState;
typedef struct _NetappsDownReport
{
    NetappsDownState state;
    uint32_t errorcode;
    char* filepath;
    uint32_t handle;
}AppMsgNetappsDownReport;
#endif


typedef enum _AppMsgTypeEnum{
#if VOICE_CONTROL_SUPPORT
    APPMSG_MESSAGE_VC,
#endif
#if defined(MEMORY_TEST_OUTPUT_OSD)
    APPMSG_MESSAGE_MT,
#endif
#if MIRACAST_SUPPORT
    APPMSG_MIRACAST_STATE_REPORT,
#endif
#if FACE_DETECT_SUPPORT
    APPMSG_MESSAGE_FD,
#endif
    APPMSG_MESSAGE_TOTAL,
}AppMsgTypeEnum;

#define APP_MSG_SIZE 1023
typedef struct _AppMsgMessageClass{
    AppMsgTypeEnum type;
    char data[APP_MSG_SIZE+1];
}AppMsgMessageClass;

typedef enum
{
    GXMSG_PM_OPEN = GXMSG_TOTAL_NUM,
    GXMSG_PM_CLOSE,
    GXMSG_PM_NODE_NUM_GET,
    GXMSG_PM_NODE_BY_POS_GET,
    GXMSG_PM_MULTI_NODE_BY_POS_GET,
    GXMSG_PM_NODE_BY_ID_GET,
    GXMSG_PM_NODE_ADD,
    GXMSG_PM_NODE_MODIFY,
    GXMSG_PM_NODE_DELETE,
    GXMSG_PM_NODE_GET_URL,
    GXMSG_PM_LOAD_DEFAULT,

    GXMSG_TUNER_COPY,
    GXMSG_PM_VIEWINFO_SET,
    GXMSG_PM_VIEWINFO_GET,

    GXMSG_INTERFACE_HELP,
    GXMSG_INTERFACE_LOCK_TS,
    GXMSG_CHECK_PM_EXIST,
    GXMSG_EMPTY_SERVICE_MSG,

    GXMSG_GET_MEDIA_TIME,
    GXMSG_AV_OUTPUT_OFF,

    GXMSG_SUBTITLE_INIT,
    GXMSG_SUBTITLE_DESTROY,
    GXMSG_SUBTITLE_HIDE,
    GXMSG_SUBTITLE_SHOW,
    GXMSG_SUBTITLE_DRAW,
    GXMSG_SUBTITLE_CLEAN,

    GXMSG_FRONTEND_DISH_MOVE,
#if NETWORK_SUPPORT
    GXMSG_NETWORK_START,
    GXMSG_NETWORK_STOP,

    GXMSG_NETWORK_DEV_LIST,

    GXMSG_NETWORK_GET_STATE,
    GXMSG_NETWORK_STATE_REPORT,

    GXMSG_NETWORK_GET_TYPE,

    GXMSG_NETWORK_GET_CUR_DEV,

    GXMSG_NETWORK_PLUG_IN,
    GXMSG_NETWORK_PLUG_OUT,

    GXMSG_NETWORK_SET_MAC,
    GXMSG_NETWORK_GET_MAC,

    GXMSG_NETWORK_SET_IPCFG,
    GXMSG_NETWORK_GET_IPCFG,

    GXMSG_NETWORK_SET_MODE,
    GXMSG_NETWORK_GET_MODE,

    GXMSG_WIFI_SCAN_AP,
    GXMSG_WIFI_SCAN_AP_OVER,

    GXMSG_WIFI_SCAN_HIDDEN_AP,
    GXMSG_WIFI_SCAN_HIDDEN_AP_OVER,

    GXMSG_WIFI_GET_CUR_AP,
    GXMSG_WIFI_SET_CUR_AP,
    GXMSG_WIFI_DEL_CUR_AP,

    GXMSG_3G_SET_PARAM,
    GXMSG_3G_GET_PARAM,
    GXMSG_3G_GET_CUR_OPERATOR,

    GXMSG_PPPOE_SET_PARAM,
    GXMSG_PPPOE_GET_PARAM,
    GXMSG_NETTIME_SYNC,
    GXMSG_MKTIME_NET_SYNC,
#if (BLUETOOTH_SUPPORT > 0)
    GXMSG_BLUETOOTH_SCAN_AP,
    GXMSG_BLUETOOTH_SCAN_AP_OVER,

    GXMSG_BLUETOOTH_GET_CUR_AP,
    GXMSG_BLUETOOTH_SET_CUR_AP,
    GXMSG_BLUETOOTH_DEL_CUR_AP,
    APPMSG_BLUETOOTH_UI_CONTROL,
#endif

#if NETAPPS_SUPPORT
    APPMSG_IPTV_GET_LIST,
    APPMSG_IPTV_LIST_DONE,
    APPMSG_IPTV_UPDATE_LOCAL_LIST,
    APPMSG_IPTV_GET_AD_PIC,
    APPMSG_IPTV_AD_PIC_DONE,
#if IPTV_LITE_SUPPORT
    APPMSG_IPTV_POWERON_PLAY,
#endif
#endif
#if NETAPPS_SUPPORT
    APPMSG_NETVIDEO_PLAY_START,
    APPMSG_NETVIDEO_DOWN_STATE_REPORT,
#endif

#if VPN_SUPPORT
    APPMSG_VPN_STATE_REPORT,
    APPMSG_VPN_GET_STATE,
    APPMSG_VPN_START,
    APPMSG_VPN_STOP,
    APPMSG_VPN_SET_AUTO,
    APPMSG_VPN_GET_AUTO,
    APPMSG_VPN_SET_PARAM,
    APPMSG_VPN_GET_PARAM,
#endif
#endif
#if PLUGINSTORE_SUPPORT
    APPMSG_PLUGIN_SOURCE_UI_CONTROL,
#endif
#if  PARENTAL_LOCK_SUPPORT
    APPMSG_EPG_PARENTAL_SEND,
#endif
#if PMT_UPDATE_SUPPORT || AUTO_UPDATE_SUPPORT || BOUQUET_SUPPORT
    APPMSG_ACU_STATE_REPORT,
#endif
#if PROGRAM_NETUP_SUPPORT
    APPMSG_PROG_NETUP_STATE_REPORT,
#endif
#if WEBLET_SUPPORT
    APPMSG_WEBLET_UI_CONTROL,
#endif
#if WEBAPI_SUPPORT
    APPMSG_WEBAPI_UI_CONTROL,
#endif
#if DLNADMR_SUPPORT || DLNADMP_SUPPORT || DLNASAT2IP_SUPPORT || DLNADMS_SUPPORT
    APPMSG_DLNA_UI_CONTROL,
#endif
#ifdef SUBTTRANS_SUPPORT
    APPMSG_SUBT_TRANS_CONTROL,
#endif
#if USB_TEST_SUPPORT
    APPMSG_USB_TEST,
#endif

#if TKGS_SUPPORT
/*TKGS msg*/
GXMSG_TKGS_CHECK_BY_PMT,//通过检查PMT中是否有TKGS component descriptor，判断该频点是否是包含TKGS数据的频点， 同步消息;参数：PMT表数据
GXMSG_TKGS_CHECK_BY_TP,//对比保存的location list检查该TP是否是包含TKGS数据的频点， 同步消息；参数：TP id
GXMSG_TKGS_START_VERSION_CHECK, //启动TKGS版本检查，看是否有新的升级， 异步消息;参数：GxTkgsUpdateParm
//GXMSG_TKGS_VERSION_CHECK_FINISH,//版本检查结果,APP处理; 参数：GxTkgsUpdateStatus
GXMSG_TKGS_START_UPDATE,//启动升级， 异步消息；参数：GxTkgsUpdateParm
GXMSG_TKGS_STOP,//强制停止TKGS已经启动的版本检查或升级，异步消息；参数:无
GXMSG_TKGS_UPDATE_STATUS, //升级结束，参数:GxTkgsUpdateMsg, 如果有TKGS消息，需要马上展示给用户，APP处理
GXMSG_TKGS_SET_LOCATION,//设置visible location, 同步消息
GXMSG_TKGS_GET_LOCATION,//获取visible location 列表
//GXMSG_TKGS_LOCATION_SET_HIDDEN_LIST,//设置hidden list， 同步消息
GXMSG_TKGS_SET_OPERATING_MODE,	//设置模式：KGS Off，Customizable，Automatic， 同步消息
GXMSG_TKGS_GET_OPERATING_MODE,	//获取设置的模式：KGS Off，Customizable，Automatic， 同步消息
GXMSG_TKGS_GET_PREFERRED_LISTS, //获取prefered list, 从TKGS数据中获取service list, 供用户选择，同步消息；参数：GxTkgsPreferredList
GXMSG_TKGS_SET_PREFERRED_LIST,//设置用户选择的列表，同步消息; 参数：字符串， service list name
GXMSG_TKGS_GET_PREFERRED_LIST,//获取用户选择的列表，同步消息；参数：字符串，service list name
GXMSG_TKGS_BLOCKING_CHECK,//检查节目是否是block节目，同步消息；参数: 节目ID
//测试相关消息
GXMSG_TKGS_TEST_GET_VERNUM, //获取TKGS表的版本号; 同步消息；参数：版本号
GXMSG_TKGS_TEST_RESET_VERNUM,	//清除机顶盒中保存的TKGS表版本号; 同步消息；参数：无
GXMSG_TKGS_TEST_CLEAR_HIDDEN_LOCATIONS_LIST,	//清除hidden location 列表；同步消息；参数：无
GXMSG_TKGS_TEST_CLEAR_VISIBLE_LOCATIONS_LIST,	//清除visible location 列表；同步消息；参数：无
#endif
#if SAT_FINDER_SUPPORT
    GXMSG_SAT_FIND_USER3,
#endif
#if CASCAM_SUPPORT
    GXMSG_CASCAM_EVENT,
#endif
#if SAT2IP_SERVER_SUPPORT
	GXMSG_SATIP_SERVICE_START,
	GXMSG_SATIP_SERVICE_STOP,
	GXMSG_SATIP_DIALOG_START,
	GXMSG_SATIP_DIALOG_STOP,
	GXMSG_SATIP_DIALOG_PAUSE,
	GXMSG_SATIP_DIALOG_RESUME,
	GXMSG_SATIP_PLAY_START,
	GXMSG_SATIP_PLAY_STOP,
#endif
#if DVB2IP_SERVER_SUPPORT
	GXMSG_DVB2IP_PLAY_START,
	GXMSG_DVB2IP_PLAY_STOP,
#endif
	APPMSG_MESSAGE,
#if HDMI_CEC_SUPPORT
    GXMSG_HDMI_CEC_ON,
    GXMSG_HDMI_CEC_OFF,
    GXMSG_HDMI_CEC_WRITE,
    GXMSG_HDMI_CEC_READ_DONE,
    APPMSG_HDMI_CEC_UI_CONTROL,
#endif
GXMAX_MSG_NUM
}AppMsgId;

#define APP_PRINT(...)     app_log_info(__VA_ARGS__)
#endif

