/************************************************************
Copyright (C), 2014, GX S&T Co., Ltd.
FileName   :	gxtkgs.c
Author     :
Version    : 	1.0
Date	   :
Description:
Version    :
History    :
Date				Author  	Modification
2014.06.10     liqg		 create
***********************************************************/
/* Includes --------------------------------------------------------------- */


#ifdef MEMWATCH
#include <common/memwatch.h>
#else
#include <stdlib.h>
#endif
#include <stdio.h>
#include "gxavdev.h"
#include "service/gxsearch.h"
#include "gxcore.h"
#include "service/gxsi.h"
#include "module/frontend/gxfrontend_module.h"
#include "module/app_nim.h"
#include "module/app_ioctl.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include "module/config/gxconfig.h"
#include "app.h"
#include "gdi_core.h"
#include "module/app_frontend.h"
#include "app_msg.h"
#include "app_send_msg.h"
#include "gxtkgs.h"
#if TKGS_SUPPORT

#define APP_TKGS_TABLE_ID	(0xA7)
#define APP_TKGS_SECTION_TIMEOUT	(5000)//5S
#define APP_TKGS_TABLE_TIMEOUT	(30000)//30S

//?????ĸ???״̬
#define TKGS_STOP				0x00
#define TKGS_START_CHECK_VER	0X01
#define TKGS_CHECK_VER_FINISH	0X02
#define TKGS_START_UPDATE		0X04
#define TKGS_UPDATE_FINISH		0X08
#define TKGS_UPDATE_TIMEOUT		0X10
#define TKGS_UPDATE_PROCESS_DATA		0X20

#define TKGS_FILTER_SW_BUFFER_SIZE (64 * 1024)
#define TKGS_FILTER_HW_BUFFER_SIZE (16 * 1024)

extern int app_get_motor_state_in_fullscreen(void);
extern void app_send_motor_state_msg(MotorState tip, int32_t motor_state);
extern status_t GxBus_PmLcnFlush(void);
static status_t gx_tkgs_get_cur_lcn_list(void);
static status_t gx_tkgs_save_cur_location(void);
static status_t gx_search_init_frontend(void);
static status_t gx_search_sat_init_params(void *params);
static status_t	gx_search_sat_get_tuner_fre(GxBusPmDataTP* tp,
													GxBusPmDataSat* sat,
													uint32_t* fre);
static status_t	gx_search_sat_get_polar(GxBusPmDataTP* tp,
													GxBusPmDataSat* sat,
													fe_sec_voltage_t* polar);
static status_t gx_search_sat_22k_get(GxBusPmDataTP* tp,GxBusPmDataSat* sat,fe_sec_tone_mode_t* sat22k);
static status_t gx_search_sat_set_tp(GxBusPmDataTP* tp,GxBusPmDataSat* sat);
static status_t gx_search_sat_tp_lock(void);
static status_t gx_tkgs_realease_frontend(void);
static void gx_tkgs_variable_exit_init(void);
static status_t gx_tkgs_class_init(GxSearchFrontType front_type);
static void gx_tkgs_status_reply(GxTkgsUpdateStatus status, char *userMsg);
static status_t gx_tkgs_set_operating_mode(GxTkgsOperatingMode *mode);
static status_t gx_tkgs_get_operating_mode(GxTkgsOperatingMode *mode);
static status_t gx_tkgs_save_version(int *version);
static status_t gx_tkgs_get_version(int *version);
static status_t app_tkgs_table_filter_start(void);
static status_t gx_tkgs_update_start(void);
static status_t app_tkgs_table_filter_release(void);
static void gx_tkgs_section_process(GxParseResult* data);
static status_t gx_tkgs_init_databuff(GxParseResult* data);
static bool gx_tkgs_receive_all_data(void);
static private_parse_status gx_tkgs_section_parse(uint8_t* p_section_data, size_t len);

//static status_t gx_tkgs_turksat_prog_process(void);
extern uint8_t s_tkgs_force_stop;
extern uint8_t s_tkgs_saving_data;
/*tkgs ??��*/
static GxTkgsClass tkgs_class;

static status_t gx_tkgs_get_cur_lcn_list(void)
{
	uint32_t i = 0;
	GxBusPmViewInfo view_info;
	GxMsgProperty_NodeNumGet node_num_get = {0};
	GxMsgProperty_NodeByPosGet ProgNode = {0};
	AppProgExtInfo prog_ext_info = {{0}};
	//GxBusPmDataProgType servType

	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

	view_info.stream_type = GXBUS_PM_PROG_TV;
	view_info.group_mode = GROUP_MODE_ALL;//all
	view_info.taxis_mode = TAXIS_MODE_NON;
	view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
    GxBus_PmViewInfoMemSet(&view_info);// Only memory, not flash
	node_num_get.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
	tkgs_class.lcnList.tv_num = 0;
	tkgs_class.lcnList.tv_lcn_list = NULL;
	if (node_num_get.node_num != 0)
	{
		tkgs_class.lcnList.tv_lcn_list = (uint16_t*)GxCore_Malloc(node_num_get.node_num*sizeof(uint16_t));
		memset(tkgs_class.lcnList.tv_lcn_list, 0, node_num_get.node_num*sizeof(uint16_t));
		for (i=0; i<node_num_get.node_num; i++)
		{
			ProgNode.node_type = NODE_PROG;
			ProgNode.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);
			if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(ProgNode.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
			{
				tkgs_class.lcnList.tv_lcn_list[tkgs_class.lcnList.tv_num++] = prog_ext_info.tkgs.logicnum;
			}
		}
	}

	view_info.stream_type = GXBUS_PM_PROG_RADIO;
	view_info.group_mode = GROUP_MODE_ALL;//all
	view_info.taxis_mode = TAXIS_MODE_NON;
	view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
    GxBus_PmViewInfoMemSet(&view_info);// Only memory, not flash
	node_num_get.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
	tkgs_class.lcnList.radio_num = 0;
	tkgs_class.lcnList.radio_lcn_list = NULL;
	if (node_num_get.node_num != 0)
	{
		tkgs_class.lcnList.radio_lcn_list = (uint16_t*)GxCore_Malloc(node_num_get.node_num*sizeof(uint16_t));
		memset(tkgs_class.lcnList.radio_lcn_list, 0, node_num_get.node_num*sizeof(uint16_t));
		for (i=0; i<node_num_get.node_num; i++)
		{
			ProgNode.node_type = NODE_PROG;
			ProgNode.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);
			if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(ProgNode.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
			{
				tkgs_class.lcnList.radio_lcn_list[tkgs_class.lcnList.radio_num++] = prog_ext_info.tkgs.logicnum;
			}
		}
	}
    GxBus_PmViewInfoMemClear();//recovery flash viewinfo
	return GXCORE_SUCCESS;
}


static uint32_t sg_SatId = 0xffff;// for search moto control
void gx_tkgs_set_disqec_callback(uint32_t sat_id,fe_sec_voltage_t polar,int32_t sat22k)
{
	#define DISEQC_SET_DELAY_MS 100
	GxMsgProperty_NodeByIdGet sat_node = {0};
	//GxMsgProperty_NodeByPosGet tp_node = {0};
	AppFrontend_DiseqcParameters diseqc10;
	AppFrontend_DiseqcParameters diseqc11;
	AppFrontend_DiseqcChange change;

	sat_node.node_type = NODE_SAT;
	sat_node.id = sat_id;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_node))
	{
		// add in 20121012
		// for polar
		//GxFrontend_SetVoltage(sat_node.sat_data.tuner, polar);
		AppFrontend_PolarState Polar =polar ;
		app_ioctl(sat_node.sat_data.tuner, FRONTEND_POLAR_SET, (void*)&Polar);

		GxCore_ThreadDelay(DISEQC_SET_DELAY_MS);
		// for 22k
		//GxFrontend_Set22K(sat_node.sat_data.tuner, sat22k);
		AppFrontend_22KState _22K = FRONTEND_22K_NULL;
		if(sat22k == SEC_TONE_OFF)
			_22K = FRONTEND_22K_OFF;
		else if(sat22k == SEC_TONE_ON)
			_22K = FRONTEND_22K_ON;
		else
			_22K = FRONTEND_22K_NULL;

		app_ioctl(sat_node.sat_data.tuner, FRONTEND_22K_SET, (void*)&_22K);

		GxCore_ThreadDelay(DISEQC_SET_DELAY_MS);
		// for diseqc
		diseqc10.type = DiSEQC10;
		diseqc10.u.params_10.bPolar = polar;
		diseqc10.u.params_10.b22k = sat22k;//app_show_to_sat_22k(sat_node.sat_data.sat_s.switch_22K, &tp_node.tp_data);
		diseqc10.u.params_10.chDiseqc = sat_node.sat_data.sat_s.diseqc10;

    	change.diseqc = &diseqc10;
    	app_ioctl(sat_node.sat_data.tuner, FRONTEND_DISEQC_CHANGE, &change);

		//if(change.state == FRONTEND_DISEQC_CHANGED)
        app_ioctl(sat_node.sat_data.tuner, FRONTEND_DISEQC_SET_EX, &diseqc10);


		GxCore_ThreadDelay(DISEQC_SET_DELAY_MS);

		diseqc11.type = DiSEQC11;
		diseqc11.u.params_11.b22k = diseqc10.u.params_10.b22k;
		diseqc11.u.params_11.bPolar = diseqc10.u.params_10.bPolar;
		diseqc11.u.params_11.chCom = diseqc10.u.params_10.chDiseqc;
		diseqc11.u.params_11.chUncom = sat_node.sat_data.sat_s.diseqc11;

		change.diseqc = &diseqc11;
        app_ioctl(sat_node.sat_data.tuner, FRONTEND_DISEQC_CHANGE, &change);

		//if(change.state == FRONTEND_DISEQC_CHANGED)
        app_ioctl(sat_node.sat_data.tuner, FRONTEND_DISEQC_SET_EX, &diseqc11);

		// TODO: change with SAT change
		if(sat_id != sg_SatId)
		{
            int32_t motor_state = app_get_motor_state_in_fullscreen();
            MotorState tip = DISH_NONE;
			sg_SatId = sat_id;
			if((sat_node.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
			{
				tip = app_diseqc12_goto_position(sat_node.sat_data.id, sat_node.sat_data.tuner,sat_node.sat_data.sat_s.diseqc12_pos, false, motor_state);
                app_send_motor_state_msg(tip, motor_state);
			}
			else if((sat_node.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS)
			{
				LongitudeInfo local_longitude;
				local_longitude.longitude = sat_node.sat_data.sat_s.longitude;
				local_longitude.longitude_direct = sat_node.sat_data.sat_s.longitude_direct;
				tip = app_usals_goto_position(sat_node.sat_data.id, sat_node.sat_data.tuner,&local_longitude, NULL, false, motor_state);
                app_send_motor_state_msg(tip, motor_state);
			}
		}
		app_diseqc_sat_id_bak_set(sat_node.sat_data.id);
		GxCore_ThreadDelay(DISEQC_SET_DELAY_MS);
	}
}

/*static functions*/
//???浱ǰ?????ɹ???location
static status_t gx_tkgs_save_cur_location(void)
{
	GxMsgProperty_NodeByPosGet ProgNode;
	GxMsgProperty_NodeByIdGet tpNode;
	//uint16_t tp_id;
	GxTkgsLocationParm location = {0};

	if ((tkgs_class.location_tp != NULL) && (tkgs_class.search_tp_finish_num<= tkgs_class.search_tp_num))
	{
		return gx_TkgsSaveLocation(&tkgs_class.location_tp[tkgs_class.search_tp_finish_num-1]);
	}
	else if ((tkgs_class.location_tp == NULL) && (tkgs_class.search_tp_num == 1))
	{
		ProgNode.node_type = NODE_PROG;
		ProgNode.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)&ProgNode);

		//get tp node
		tpNode.node_type = NODE_TP;
		tpNode.id = ProgNode.prog_data.tp_id;
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tpNode);

		location.sat_id = tpNode.tp_data.sat_id;
		location.search_demux_id = 0;
		//location.search_diseqc = gx_tkgs_set_disqec_callback;
		location.frequency = tpNode.tp_data.frequency;
		location.symbol_rate = tpNode.tp_data.tp_s.symbol_rate;
		location.polar = tpNode.tp_data.tp_s.polar;
		location.pid = tkgs_class.pid[0];
		return gx_TkgsSaveLocation(&location);
	}
	else
	{
		TKGS_ERRO_PRINTF("##%s, %d, err\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}
}


static status_t gx_search_init_frontend(void)
{
    status_t                ret;
    GxDemuxProperty_ConfigDemux config_demux;

	tkgs_class.search_device_handle = GxAvdev_CreateDevice(0);
	if (tkgs_class.search_device_handle < 0) {
		TKGS_ERRO_PRINTF("[SEARCH]---create device err!!\n");
		return GXCORE_ERROR;
	}
	//frontend
	tkgs_class.search_demux_handle = GxAvdev_OpenModule(tkgs_class.search_device_handle, GXAV_MOD_DEMUX, tkgs_class.search_demux_id);
	if(tkgs_class.search_demux_handle<0)
	{
		TKGS_ERRO_PRINTF("[SEARCH]---open demux  err!!\n");
		return GXCORE_ERROR;
	}
    //?demux??ts
    config_demux.source = tkgs_class.search_ts_cur;
    config_demux.ts_select = FRONTEND;
    config_demux.stream_mode = DEMUX_PARALLEL;
    config_demux.time_gate = 0xf;
    config_demux.byt_cnt_err_gate = 0x03;
    config_demux.sync_loss_gate = 0x03;
    config_demux.sync_lock_gate = 0x03;
    ret = GxAVSetProperty(tkgs_class.search_device_handle,
                          tkgs_class.search_demux_handle,
                          GxDemuxPropertyID_Config,
                          &config_demux,
                          sizeof(GxDemuxProperty_ConfigDemux));
    if(ret < 0)
    {
        TKGS_ERRO_PRINTF("[SEARCH]---config demux  err!!\n");
        return GXCORE_ERROR;
    }
	return GXCORE_SUCCESS;
}


static status_t gx_search_sat_init_params(void *params)
{
	//GxBusPmDataTP           tp = { 0 };
	GxBusPmDataSat          sat = { 0 };
	AppFrontend_Config cfg = {0};

	if (params == NULL)
	{
		TKGS_ERRO_PRINTF("[SEARCH]---search params err!!\n");
		return GXCORE_ERROR;
	}
	else if (((GxTkgsUpdateParm *)params)->tp_id >=0)
	{
		GxMsgProperty_NodeByPosGet node;
		//ֻ??????ǰƵ??
		tkgs_class.search_tp_num = 1;
		tkgs_class.location_tp = NULL;//(uint32_t*)GxCore_Malloc(sizeof(uint32_t)*tkgs_class.search_tp_num);
		tkgs_class.pid = (uint16_t*)GxCore_Malloc(sizeof(uint16_t));
		tkgs_class.pid[0]	= ((GxTkgsUpdateParm *)params)->pid;
		tkgs_class.search_demux_id = 0;//

		node.node_type = NODE_PROG;
		node.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)&node);
		tkgs_class.search_tuner_num_for_prog = node.prog_data.tuner;
		tkgs_class.search_sat_id_for_prog = node.prog_data.sat_id;
		TKGS_DEBUG_PRINTF("====%s, %d, pid:%d\n", __FUNCTION__, __LINE__, tkgs_class.pid[0]);
	}
	else
	{
		TKGS_DEBUG_PRINTF("====%s, %d\n", __FUNCTION__, __LINE__);
		//?????ȼ?˳??????????location Ƶ??
		tkgs_class.location_tp =
			(GxTkgsLocationParm*)GxCore_Malloc(sizeof(GxTkgsLocationParm)*(TKGS_MAX_VISIBLE_LOCATION+TKGS_MAX_INVISIBLE_LOCATION));
		tkgs_class.pid = (uint16_t*)GxCore_Malloc(sizeof(uint16_t)*(TKGS_MAX_VISIBLE_LOCATION+TKGS_MAX_INVISIBLE_LOCATION));
		gx_TkgsGetLocationInOrder(&(tkgs_class.search_tp_num),tkgs_class.location_tp, tkgs_class.pid);

		GxBus_PmSatGetById(tkgs_class.location_tp[0].sat_id, &sat);
		tkgs_class.search_tuner_num_for_prog = sat.tuner;
		tkgs_class.search_sat_id = tkgs_class.location_tp[0].sat_id;
		tkgs_class.search_diseqc = gx_tkgs_set_disqec_callback;//tkgs_class.location_tp[0].search_diseqc;
	    tkgs_class.search_demux_id =  tkgs_class.location_tp[0].search_demux_id;//params->demux_id;
	}

	tkgs_class.search_type = ((GxTkgsUpdateParm *)params)->type;
	app_ioctl(sat.tuner, FRONTEND_CONFIG_GET, &cfg);
	tkgs_class.search_ts_cur = cfg.ts_src;

	/*?????ѵ??Ľ?Ŀ???ڲ????? ????ֱ?ӿ???????Ŀ??�� ???????ͷ? ?õط????ķѼ???k?ڴ?*/
	tkgs_class.search_prog_id_arry = (uint32_t*)GxCore_Malloc(sizeof(uint32_t) * MAX_PROG_COUNT);
	if(tkgs_class.search_prog_id_arry == NULL)
	{
		TKGS_ERRO_PRINTF("[SEARCH]---malloc search_prog_arry space err!!\n");
		return GXCORE_ERROR;
	}
	tkgs_class.search_prog_id_num = 0;

	/*?ж??ٸ?pat body????ζ???ж??ٸ??ܵ?sdt body,???Կռ?Ӧ?ÿ??ܵ?
	   body,????pat??sdt??ͬʱ??????,?????޷?ͨ??pat body?ж?,ֱ?ӿ?һ??300 */
	tkgs_class.search_sdt_body = (GxSearchSdtBody *)
	    GxCore_Malloc(sizeof(GxSearchSdtBody) * MAX_PROG_PER_TP);	//ÿ??tp????300??Ŀ


	gx_search_init_frontend();

	return GXCORE_SUCCESS;
}

static status_t	gx_search_sat_get_tuner_fre(GxBusPmDataTP* tp,
													GxBusPmDataSat* sat,
													uint32_t* fre)
{
	uint32_t lnb_fre = 0;
	GxBusPmSatLnbType type = 0;

	if(sat->sat_s.lnb2 == 0
		||sat->sat_s.lnb2 == sat->sat_s.lnb1)
	{
		type = GXBUS_PM_SAT_LNB_USER;
	}
	else if(sat->sat_s.lnb1<6000)
	{
		type = GXBUS_PM_SAT_LNB_OCS;
	}
	else if(sat->sat_s.lnb1>6000)
	{
		type = GXBUS_PM_SAT_LNB_UNIVERSAL;
	}

	switch(type)
	{
		case GXBUS_PM_SAT_LNB_USER:
			lnb_fre = sat->sat_s.lnb1;
			break;

		case GXBUS_PM_SAT_LNB_UNIVERSAL:
			if(tp->frequency >= 11700)
			{
                lnb_fre = sat->sat_s.lnb2;
			}
            else
            {
                lnb_fre = sat->sat_s.lnb1;
            }
			break;

		case GXBUS_PM_SAT_LNB_OCS:
			switch(tp->tp_s.polar)
			{
				case GXBUS_PM_TP_POLAR_H:
					lnb_fre = sat->sat_s.lnb1;
					break;

				case GXBUS_PM_TP_POLAR_V:
					lnb_fre = sat->sat_s.lnb2;
					break;

				case GXBUS_PM_TP_POLAR_AUTO:
					lnb_fre = sat->sat_s.lnb1;
					break;

				default:
					#ifdef GX_BUS_SEARCH_DBUG
					GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---sat polar err!!\n");
					#endif
					return GXCORE_ERROR;
					break;
			}
			break;

		default:
			#ifdef GX_BUS_SEARCH_DBUG
			GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---sat lnb type err!!\n");
			#endif
			return GXCORE_ERROR;
			break;
	}
	if(lnb_fre >= tp->frequency)
	{
		*fre = lnb_fre - tp->frequency;
	}
	else
	{
		*fre = tp->frequency - lnb_fre;
	}
	return GXCORE_SUCCESS;
}

static status_t	gx_search_sat_get_polar(GxBusPmDataTP* tp,
													GxBusPmDataSat* sat,
													fe_sec_voltage_t* polar)
{
	switch(sat->sat_s.lnb_power)
	{
		case GXBUS_PM_SAT_LNB_POWER_OFF:
			*polar = SEC_VOLTAGE_OFF;
			break;

		case GXBUS_PM_SAT_LNB_POWER_13V:
			*polar = SEC_VOLTAGE_13;
			break;

		case GXBUS_PM_SAT_LNB_POWER_18V:
			*polar = SEC_VOLTAGE_18;
			break;

		case GXBUS_PM_SAT_LNB_POWER_ON:
			switch(tp->tp_s.polar)
			{
				case GXBUS_PM_TP_POLAR_H:
					*polar = SEC_VOLTAGE_18;
					break;

				case GXBUS_PM_TP_POLAR_V:
					*polar = SEC_VOLTAGE_13;
					break;

				case GXBUS_PM_TP_POLAR_AUTO:
					*polar = SEC_VOLTAGE_18;
					break;

				default:
					#ifdef GX_BUS_SEARCH_DBUG
					GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---tp polar type err!!\n");
					#endif
					return GXCORE_ERROR;
					break;
			}
			break;

			default:
				#ifdef GX_BUS_SEARCH_DBUG
				GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---sat lnb power err!!\n");
				#endif
				return GXCORE_ERROR;
				break;
	}

	return GXCORE_SUCCESS;
}

static status_t gx_search_sat_22k_get(GxBusPmDataTP* tp,GxBusPmDataSat* sat,fe_sec_tone_mode_t* sat22k)
{
	if(sat->sat_s.lnb2 == 0
		||sat->sat_s.lnb2 == sat->sat_s.lnb1)
	{
         if(GXBUS_PM_SAT_22K_OFF == sat->sat_s.switch_22K)
		{
			*sat22k = SEC_TONE_OFF;
		}
		else if(GXBUS_PM_SAT_22K_ON == sat->sat_s.switch_22K)
		{
			*sat22k = SEC_TONE_ON;
		}
		else
		{
			*sat22k = SEC_TONE_ON;
		}
	}
	else if(sat->sat_s.lnb1<6000)
	{
		 if(GXBUS_PM_SAT_22K_OFF == sat->sat_s.switch_22K)
         {
             *sat22k = SEC_TONE_OFF;
         }
         else if(GXBUS_PM_SAT_22K_ON == sat->sat_s.switch_22K)
         {
             *sat22k = SEC_TONE_ON;
         }
         else
         {
             *sat22k = SEC_TONE_ON;
         }
	}
	else if(sat->sat_s.lnb1>6000)
	{
		 if(tp->frequency >= 11700)
         {
              *sat22k = SEC_TONE_ON;
         }
         else
         {
             *sat22k = SEC_TONE_OFF;
         }
	}
    return GXCORE_SUCCESS;
}

static status_t gx_search_sat_set_tp(GxBusPmDataTP* tp,GxBusPmDataSat* sat)
{
	GxFrontend para = {0};
	fe_sec_tone_mode_t sat22k_mode = 0;
	uint32_t fre = 0;
	fe_sec_voltage_t polar = 0;
    int32_t ret = 0;
	//DVB-S
	gx_search_sat_get_tuner_fre(tp,sat,&fre);
	gx_search_sat_get_polar(tp,sat,&polar);
	gx_search_sat_22k_get(tp,sat,&sat22k_mode);
	para.sat22k = sat22k_mode;

	para.demux = tkgs_class.search_demux_handle;
	para.dev = tkgs_class.search_device_handle;
	para.fre = fre;
	para.polar = polar;
	para.symb = tp->tp_s.symbol_rate;
	para.pls_n = tp->tp_s.pls_n;
	para.tuner = tkgs_class.search_tuner_num_for_prog;
	para.type = FRONTEND_DVB_S;

	if(tkgs_class.search_diseqc != NULL)
	{
		tkgs_class.search_diseqc(tkgs_class.search_sat_id_for_prog,polar,sat22k_mode);
	}
	else
	{
		GxFrontend_SetVoltage(tkgs_class.search_tuner_num_for_prog, polar);

		GxFrontend_Set22K(tkgs_class.search_tuner_num_for_prog,sat22k_mode);
	}
	ret = GxFrontend_SetTp(&para);
	if(ret == -1)//(((ret = GxFrontend_SetTp(&para)) != 0) && (ret != -2))
	{
		return GXCORE_ERROR;
	}
	else if(ret == -2)
	{
		return GX_TKGS_SEARCH_CONTINUE;
	}

	return GXCORE_SUCCESS;
}

static status_t gx_search_sat_tp_lock(void)
{
	//uint32_t                tp_id = 0;
	uint32_t 				i = 0;
	uint32_t 				locked = 0;
    unsigned long           tick = 0;

	GxBusPmDataTP           tp = { 0 };
	GxBusPmDataSat          sat = { 0 };
	GxTkgsLocationParm		*pLocationTp = NULL;
	status_t ret = 0;
	uint32_t fre = 0;
	uint32_t status = FE_TIMEDOUT;
//	uint8_t do_locked_once = 0;
//	uint32_t current_tp_plsn = 0;

	/*?ڵ?ǰ??????Ƶ?????����??ٴ?????Ƶ*/
	if ((tkgs_class.location_tp == NULL) && (tkgs_class.search_tp_num == 1))
	{
		TKGS_DEBUG_PRINTF("====%s, %d, not need lock tp\n", __FUNCTION__, __LINE__);
		tkgs_class.search_tp_finish_num = 1;
		return GXCORE_SUCCESS;
	}

 start_lock:
		if(  s_tkgs_force_stop == 1)
	     {
	     s_tkgs_force_stop = 0;
		 tkgs_class.search_tp_finish_num = tkgs_class.search_tp_num;
	     //TKGS_DEBUG_PRINTF("=====%s, %d, s_tkgs_force_stop :0x%x\n", __FUNCTION__, __LINE__,s_tkgs_force_stop);
            return GXCORE_ERROR;
		  }
	if (tkgs_class.search_tp_num != 0 && tkgs_class.search_tp_finish_num < tkgs_class.search_tp_num)
	{
		pLocationTp = &(tkgs_class.location_tp[tkgs_class.search_tp_finish_num]);
		//GxBus_PmTpGetById(tp_id, &tp);
		GxBus_PmSatGetById(pLocationTp->sat_id, &sat);
		tkgs_class.search_tp_finish_num++;
        /*?????¸?tp??tp id??sat id ???ڽ?Ŀ??Ϣ??tuner*/
		tkgs_class.search_sat_id_for_prog = pLocationTp->sat_id;
		//tkgs_class.search_tp_id_for_prog = tp.id;	//???����Ľ?Ŀ??????tpҲ??tkgs???У?Ҫ???????ӵ????ݿ??к???֪??TPID
		tkgs_class.search_tuner_num_for_prog = sat.tuner;

		tp.frequency = pLocationTp->frequency;
		tp.tp_s.polar = pLocationTp->polar;
		tp.tp_s.symbol_rate = pLocationTp->symbol_rate;
		TKGS_DEBUG_PRINTF("====%s, %d, TP[%d,%d, %d, %d]\n", __FUNCTION__, __LINE__, sat.id, tp.frequency, tp.tp_s.symbol_rate, tp.tp_s.polar);
		ret = gx_search_sat_set_tp(&tp,&sat);
		if(ret == GXCORE_ERROR)
		{
			TKGS_DEBUG_PRINTF("@@%s, %d, set tp error\n", __FUNCTION__, __LINE__);
			GxCore_ThreadYield();
			goto start_lock;
		}

		/*??????Ƶ????,?ж???û??????,û??????goto start_lock????һ??tp,
		   ???????????ж?ts??û?????? ???????˷???GX_OK */
		gx_search_sat_get_tuner_fre(&tp,&sat,&fre);
		if(ret != GX_TKGS_SEARCH_CONTINUE)
		{
			ret = GxFrontend_ReadStatus(sat.tuner,&status);
		}
		else
		{
			status = FE_HAS_LOCK;
		}

		if (tp.tp_s.symbol_rate <= 5000)
            i = 3500;//3.5s
        else if (tp.tp_s.symbol_rate > 10000)
            i = 2500;//2.5s
        else
            i = 3000;//3s

		tick = GxCore_TickStart(i);
		locked = 0;
		{
			uint32_t query_count = 0;
			int32_t tp_lock_status = 0;
			handle_t frontend_fd = GxFrontend_IdToHandle(tkgs_class.search_tuner_num_for_prog);
			do
			{
				query_count++;
				if(ioctl(frontend_fd, FE_READ_STATUS, &tp_lock_status) >= 0)
				{
					if(tp_lock_status == FE_HAS_CARRIER)// gx113x lock some signal,but it is wrong symbol rate
					{
						break;
					}
					else if(tp_lock_status == FE_HAS_LOCK)
					{
						if(GxFrontend_QueryStatus(tkgs_class.search_tuner_num_for_prog) == 1)
						{
							locked = 1;
							break;
						}

					}
					else
					{
						int interval = (query_count*1000)/i;//tick - GxCore_TickStart(0);

						if((tp_lock_status&0x35)==0x35){
							if(interval >= 80) tick-=5*20;
						} else if((tp_lock_status&0x15)==0x15){
							if(interval>=60) tick-=5*20;
						} else if(((tp_lock_status&0x05)==0x05) || ((tp_lock_status&0x05)==0x05)){
							if(interval>=50) tick-=10*20;
						} else if((tp_lock_status&0x01)==0x01){
							if(interval>=50) tick-=15*20;
						} else{
							if(interval>=50) tick-=20*20;
						}
					}
					GxCore_ThreadDelay(10);
				}
				else
				{
					return GXCORE_ERROR;
				}
			}while(!GxCore_TickEnd(tick));
		}
		if (locked == 0)
		{
			goto start_lock;
		}

    }
	else
	{
		TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}
	TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
	return GXCORE_SUCCESS;
}

static status_t gx_tkgs_realease_frontend(void)
{
	status_t                ret = 0;
	//frontend
	if(tkgs_class.search_device_handle != GXCORE_INVALID_POINTER
		&&tkgs_class.search_demux_handle != GXCORE_INVALID_POINTER)
	{
		ret = GxAvdev_CloseModule(tkgs_class.search_device_handle, tkgs_class.search_demux_handle);
		tkgs_class.search_demux_handle = GXCORE_INVALID_POINTER;
		if (ret < 0)
		{
			TKGS_DEBUG_PRINTF("[SEARCH]---close module err!!search_demux_handle\n");
		}
        //?ɰ?demux??ts
		ret = GxAvdev_DestroyDevice(tkgs_class.search_device_handle);
		if (ret < 0)
		{
			TKGS_DEBUG_PRINTF("[SEARCH]---destroy device err!!search_device_handle\n");
		}
		tkgs_class.search_device_handle = GXCORE_INVALID_POINTER;
	}

	return GXCORE_SUCCESS;
}


/*???˳?search??ʱ????????��*/
static void gx_tkgs_variable_exit_init(void)
{

	tkgs_class.search_sat_id = 0;

	tkgs_class.search_tp_num = 0;
	if (tkgs_class.location_tp != NULL)
	{
		GxCore_Free(tkgs_class.location_tp);
		tkgs_class.location_tp = NULL;
	}
	tkgs_class.search_tp_finish_num = 0;

	//release tkgs filter
	app_tkgs_table_filter_release();

	tkgs_class.search_status = TKGS_STOP;

	/*tkgs info*/
	if (tkgs_class.pid != NULL) {
		GxCore_Free(tkgs_class.pid);
		tkgs_class.pid = NULL;
	}
	tkgs_class.tkgs_time_out = 0;
	tkgs_class.tkgs_subtable_id.subtable_id = -1;	///tkgs??subtable id
	tkgs_class.last_section_number = 0;
	tkgs_class.tkgs_version = -1;
	if (tkgs_class.received_flag != NULL) {
		GxCore_Free(tkgs_class.received_flag);
		tkgs_class.received_flag = NULL;
	}
	if (tkgs_class.tkgs_data != NULL) {
		GxCore_Free(tkgs_class.tkgs_data);
		tkgs_class.tkgs_data = NULL;
	}
	if (tkgs_class.tkgs_message != NULL) {
		GxCore_Free(tkgs_class.tkgs_message);
		tkgs_class.tkgs_message = NULL;
	}

	tkgs_class.cur_prefer_list.serviceNum = 0;
	if (tkgs_class.cur_prefer_list.list != NULL) {
		GxCore_Free(tkgs_class.cur_prefer_list.list);
		tkgs_class.cur_prefer_list.list = NULL;
	}

	tkgs_class.search_pmt_count = 0;
	tkgs_class.search_pmt_finish_count = 0;

	tkgs_class.search_sdt_body_count = 0;

	memset(&(tkgs_class.search_pmt_body), 0, sizeof(GxSearchPmtBody));

	if (tkgs_class.search_stream_body != NULL) {
		GxCore_Free(tkgs_class.search_stream_body);
		tkgs_class.search_stream_body = NULL;
	}

	tkgs_class.search_stream_body_count = 0;

	tkgs_class.search_sat_id_for_prog = 0;
	tkgs_class.search_tp_id_for_prog = 0;

	gx_tkgs_realease_frontend();
	return;
}

static status_t gx_tkgs_class_init(GxSearchFrontType front_type)
{
	tkgs_class.front_type = front_type;

	tkgs_class.search_tp_num = 0;	///<??Ҫ??????tp????��
	if(tkgs_class.location_tp != NULL)
	{
		GxCore_Free(tkgs_class.location_tp);
	}
	tkgs_class.location_tp = NULL;	///<??Ҫ??????tp??id
	tkgs_class.search_tp_finish_num = 0;	///<?Ѿ????????ɵ?tp??��,??search_tp_finish_num == search_tp_numʱ????????

	/*tkgs info*/
	if (tkgs_class.pid != NULL) {
		GxCore_Free(tkgs_class.pid);
		tkgs_class.pid = NULL;
	}
	tkgs_class.tkgs_time_out = 0;
	tkgs_class.tkgs_subtable_id.subtable_id = -1;	///tkgs??subtable id
	tkgs_class.last_section_number = 0;
	tkgs_class.tkgs_version = -1;
	if (tkgs_class.received_flag != NULL) {
		GxCore_Free(tkgs_class.received_flag);
		tkgs_class.received_flag = NULL;
	}
	if (tkgs_class.tkgs_data != NULL) {
		GxCore_Free(tkgs_class.tkgs_data);
		tkgs_class.tkgs_data = NULL;
	}
	if (tkgs_class.tkgs_message != NULL) {
		GxCore_Free(tkgs_class.tkgs_message);
		tkgs_class.tkgs_message = NULL;
	}

	tkgs_class.cur_prefer_list.serviceNum = 0;
	if (tkgs_class.cur_prefer_list.list != NULL) {
		GxCore_Free(tkgs_class.cur_prefer_list.list);
		tkgs_class.cur_prefer_list.list = NULL;
	}

	/*sdt*/
	if(tkgs_class.search_sdt_body != NULL)
	{
		GxCore_Free(tkgs_class.search_sdt_body);
	}
	tkgs_class.search_sdt_body = NULL;
	tkgs_class.search_sdt_body_count = 0;

	/*pmt*/
	memset(&(tkgs_class.search_pmt_body),0,sizeof(GxSearchPmtBody));
	tkgs_class.search_pmt_count = 0;	///<???ڽ?????pmt????��,????Ӧ?õ???search_pat_body_count
	tkgs_class.search_pmt_finish_count = 0;	///<?????õ?pmt????��,????Ӧ?õ???search_pat_body_count

	/*stream ???ڽ??????ڴ洢audio??Ϣ*/
	tkgs_class.search_stream_body_count = 0;
	if(tkgs_class.search_stream_body != NULL)
	{
		GxCore_Free(tkgs_class.search_stream_body);
	}
	tkgs_class.search_stream_body = NULL;

	tkgs_class.search_tp_id_for_prog = 0;	///<??Ŀ??????tp id ???ڱ?????Ŀ ??????tp??ʱ?????и???
	tkgs_class.search_status = TKGS_STOP;	///<ֹͣ??????־ ?ñ?־??????????,??ÿ???յ???ʼ????????Ϣ??????

	/*ǰ???豸?ľ???*/
	tkgs_class.search_device_handle = GXCORE_INVALID_POINTER;
	tkgs_class.search_demux_handle = GXCORE_INVALID_POINTER;

	/*??¼?????����Ľ?Ŀ ???ڽ??ܵ?save?Ǳ???*/
	if(tkgs_class.search_prog_id_arry != NULL)
	{
		GxCore_Free(tkgs_class.search_prog_id_arry);
	}
	tkgs_class.search_prog_id_arry = NULL;
	tkgs_class.search_prog_id_num = 0;

	switch(front_type)
	{
		case GX_SEARCH_FRONT_S:
			tkgs_class.tp_lock_func = gx_search_sat_tp_lock;
			tkgs_class.init_params_func = gx_search_sat_init_params;
			break;

		default:
			break;
	}
	return GXCORE_SUCCESS;
}

static void gx_tkgs_status_reply(GxTkgsUpdateStatus status, char *userMsg)
{
	GxMessage              *new_msg = NULL;
	GxTkgsUpdateMsg *params = NULL;

	new_msg = GxBus_MessageNew(GXMSG_TKGS_UPDATE_STATUS);
	params = (GxTkgsUpdateMsg *)GxBus_GetMsgPropertyPtr(new_msg,GxTkgsUpdateMsg);

	params->status = status;
	if (userMsg != NULL)
	{
		memcpy(params->message, userMsg, sizeof(params->message));
	}

	GxBus_MessageSend(new_msg);
	TKGS_DEBUG_PRINTF("===%s, %d, %d, %s\n", __FUNCTION__, __LINE__, status, userMsg);
	return;
}


/**tkgs setting items start*/
#define TKGS_OPERATING_MODE	"tkgs>operatingmode"
#define TKGS_VERSION	"tkgs>version"

/**
 * @brief ??ȡ????ģʽ
 * @param
 * @Return
 */
static status_t gx_tkgs_set_operating_mode(GxTkgsOperatingMode *mode)
{
	if (mode == NULL)
	{
		return GXCORE_ERROR;
	}
	GxBus_ConfigSetInt(TKGS_OPERATING_MODE,*mode);
	return GXCORE_SUCCESS;
}

/**
 * @brief ???ò???ģʽ
 * @param
 * @Return
 */
static status_t gx_tkgs_get_operating_mode(GxTkgsOperatingMode *mode)
{
	if (mode == NULL)
	{
		return GXCORE_ERROR;
	}
	GxBus_ConfigGetInt(TKGS_OPERATING_MODE,(int32_t *)mode,GX_TKGS_AUTOMATIC);

	return GXCORE_SUCCESS;
}

/**
 * @brief ????tkgs?汾??
 * @param
 * @Return
 */
static status_t gx_tkgs_save_version(int *version)
{
	if (version == NULL)
	{
		return GXCORE_ERROR;
	}

	GxBus_ConfigSetInt(TKGS_VERSION,*version);
	return GXCORE_SUCCESS;
}

/**
 * @brief ??ȡtkgs?汾??
 * @param
 * @Return
 */
static status_t gx_tkgs_get_version(int *version)
{
	if (version == NULL)
	{
		return GXCORE_ERROR;
	}

	GxBus_ConfigGetInt(TKGS_VERSION,(int32_t *)version,0);
	return GXCORE_SUCCESS;
}
/**tkgs setting items end*/

///////*tkgs filter start*/
/**
 * @brief ????tkgs??????
 * @param
 * @Return
 */
static status_t app_tkgs_table_filter_start(void)
{
	GxSubTableDetail subt_detail = {0};
	GxMsgProperty_SiCreate  params_create;
	GxMsgProperty_SiStart params_start;

	//create Filter for OTA DATA
	subt_detail.ts_src = (uint16_t)tkgs_class.search_ts_cur;
	subt_detail.demux_id = tkgs_class.search_demux_id;
	subt_detail.si_filter.pid = (uint16_t)tkgs_class.pid[tkgs_class.search_tp_finish_num-1];
	subt_detail.si_filter.match_depth = 5;
	subt_detail.si_filter.eq_or_neq = EQ_MATCH;
	subt_detail.si_filter.sw_buffer_size = TKGS_FILTER_SW_BUFFER_SIZE;
	subt_detail.si_filter.hw_buffer_size = TKGS_FILTER_HW_BUFFER_SIZE;
	subt_detail.si_filter.match[0] = APP_TKGS_TABLE_ID;
	subt_detail.si_filter.mask[0] = 0xff;
	subt_detail.si_filter.match[3] = 0x00;//data type
	subt_detail.si_filter.mask[3] = 0xff;
	subt_detail.si_filter.match[4] = 0x00;//data type
	subt_detail.si_filter.mask[4] = 0xff;
	subt_detail.time_out = tkgs_class.tkgs_time_out;//5S

	subt_detail.table_parse_cfg.mode = PARSE_PRIVATE_ONLY; // fix:257305  as 6605s cost down memory
	subt_detail.table_parse_cfg.table_parse_fun = gx_tkgs_section_parse;


	TKGS_DEBUG_PRINTF("=====%s, %d, pid:%d, timeOut:%d, demuxId:%d, ts_src:%d\n",
		__FUNCTION__, __LINE__, subt_detail.si_filter.pid, subt_detail.time_out,
		subt_detail.demux_id, subt_detail.ts_src);

	params_create = &subt_detail;
	app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE, (void*)&params_create);

	if (subt_detail.si_subtable_id == -1)	//????ʧ??
	{
		TKGS_DEBUG_PRINTF("ceate tkgs filter err!!\n");
		return GXCORE_ERROR;
	}

	//Get the return "si_subtable_id"
	tkgs_class.tkgs_subtable_id.subtable_id = subt_detail.si_subtable_id;
	tkgs_class.tkgs_subtable_id.request_id = subt_detail.request_id;

	params_start = tkgs_class.tkgs_subtable_id.subtable_id;
	//for test
	//GxTkgsOtaPrintf("---[dsi][start]---\n");
	//GxTkgsOtaPrintf("---[ts_src = %d][pid = 0x%x][subt_id = %d][req_id = %d]---\n",
	//subt_detail.ts_src,subt_detail.si_filter.pid,tkgs_ota->ota_sub[i].subt_id,tkgs_ota->ota_sub[i].req_id);
	//GxTkgsOtaPrintf("---[dsi][end]---\n");
	app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void*)&params_start);

	return GXCORE_SUCCESS;

 }

static status_t gx_tkgs_update_start(void)
{
	status_t ret = 0;

	ret = tkgs_class.tp_lock_func();

	if (ret == GXCORE_SUCCESS)
	{
		      if(  s_tkgs_force_stop == 1)
	     {
	     s_tkgs_force_stop = 0;
	     //TKGS_DEBUG_PRINTF("=====%s, %d, s_tkgs_force_stop :0x%x\n", __FUNCTION__, __LINE__,s_tkgs_force_stop);
            return 1;
		  }
		TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
		ret = app_tkgs_table_filter_start();
	}

	TKGS_DEBUG_PRINTF("===%s, %d, ret:%d\n", __FUNCTION__, __LINE__, ret);

	return ret;
}

/**
 * @brief ?ͷ?tkgs??????
 * @param
 * @Return
 */
static status_t app_tkgs_table_filter_release(void)
{
	if(tkgs_class.tkgs_subtable_id.subtable_id != -1)
	{
		GxMsgProperty_SiRelease params_release = (GxMsgProperty_SiRelease)tkgs_class.tkgs_subtable_id.subtable_id;

		app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void*)&params_release);
		tkgs_class.tkgs_subtable_id.subtable_id = -1;
	}

 	return GXCORE_SUCCESS;
}
////////*tkgs filter end*/

//?????յ???TKGS section
static void gx_tkgs_section_process(GxParseResult* data)
{
	uint8_t section_number = data->parsed_data[6];
	uint16_t data_len = (((data->parsed_data[1]&0x0f)<<8)| data->parsed_data[2])-9;
	uint8_t	version = (data->parsed_data[5]&0x3E)>>1;


	if ((tkgs_class.received_flag[section_number] == 0) && (version == tkgs_class.tkgs_version))
	{

		TKGS_DEBUG_PRINTF("===%s, %d, copy section:%d data, len:%d, version:%d\n", __FUNCTION__, __LINE__, section_number, data_len, (data->parsed_data[5]&0x3E)>>1);
		memcpy(&(tkgs_class.tkgs_data[section_number*4084]),&(data->parsed_data[8]),data_len);
		tkgs_class.received_flag[section_number] = 1;
	}
	TKGS_DEBUG_PRINTF("===%s, %d, release data\n", __FUNCTION__, __LINE__);
	GxBus_SiParserBufFree(data->parsed_data);
}

//???뱣??TKGS???ݵĿռ?
static status_t gx_tkgs_init_databuff(GxParseResult* data)
{
	if (tkgs_class.tkgs_data == NULL)
	{
		tkgs_class.last_section_number = data->parsed_data[7];
		tkgs_class.tkgs_version = (data->parsed_data[5]&0x3E)>>1;
		TKGS_DEBUG_PRINTF("===%s, %d, last_section_number:%d, ver:%d\n", __FUNCTION__, __LINE__, tkgs_class.last_section_number, tkgs_class.tkgs_version);
		tkgs_class.received_flag =
			(uint8_t*)GxCore_Malloc(sizeof(uint8_t)*(tkgs_class.last_section_number+1));
		tkgs_class.tkgs_data =
			(uint8_t*)GxCore_Malloc(4084*(tkgs_class.last_section_number+1));
		if ((tkgs_class.tkgs_data==NULL) || (tkgs_class.received_flag==NULL))
		{
			TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
			return GXCORE_ERROR;
		}

		memset(tkgs_class.received_flag, 0, sizeof(uint8_t)*(tkgs_class.last_section_number+1));
		memset(tkgs_class.tkgs_data, 0, 4084*(tkgs_class.last_section_number+1));
	}

	return GXCORE_SUCCESS;
}

//?ж??Ƿ??Ѿ???ȡ??????????
static bool gx_tkgs_receive_all_data(void)
{
	int i;

	for (i=0; i<tkgs_class.last_section_number+1; i++)
	{
		if (tkgs_class.received_flag[i] != 1)
		{
			return false;
		}
	}

	TKGS_DEBUG_PRINTF("===%s, %d, receive all data\n", __FUNCTION__, __LINE__);
	return true;
}

#if 1
/**
 * @brief ????TKGSģʽ??TURK sat?µĽ?Ŀ??????, autoģʽʱɾ??TURK sat?????н?Ŀ
 * @param
 * @Return
 */
static status_t gx_tkgs_turksat_prog_process(void)
{
	GxTkgsOperatingMode tkgs_mode = GX_TKGS_AUTOMATIC;
    uint32_t pos;
    GxMsgProperty_NodeNumGet sat_num = {0};
    GxMsgProperty_NodeByPosGet node_sat = {0};

	app_log_flow("====%s,%d, start \n", __FUNCTION__, __LINE__);
	gx_tkgs_get_operating_mode(&tkgs_mode);
	//if (tkgs_mode != GX_TKGS_AUTOMATIC)
	if (!((tkgs_mode == GX_TKGS_OFF) && (tkgs_class.search_type == GX_TKGS_UPDATE_ALL)))
	{
		app_log_flow("====%s,%d, not audo mode , not del turk prog\n", __FUNCTION__, __LINE__);
		return GXCORE_SUCCESS;
	}

    sat_num.node_type = NODE_SAT;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);
    for(pos = 0; pos<sat_num.node_num; pos++)
    {
        node_sat.node_type = NODE_SAT;
        node_sat.pos = pos;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);
        if(0 == strncasecmp((const char*)node_sat.sat_data.sat_s.sat_name,"turksat",7))
        {
            break;
        }
    }

	if (pos == sat_num.node_num)
	{
		return GXCORE_SUCCESS;
	}

    app_all_channels_del_by_sat_id(node_sat.sat_data.id);
	return GXCORE_SUCCESS;
}
///*tkgs section process end**/

#endif

static private_parse_status gx_tkgs_section_parse(uint8_t* p_section_data, size_t len)
{
	uint8_t *data = p_section_data;
	uint16_t section_length = 0;
	GxParseResult parse_result = {0};

	if((NULL == data) || data[0] != APP_TKGS_TABLE_ID/*(((((data[1] & 0x0f) << 8) | data[2] ) + 3) != len)*/)
	{
        app_log_flow("\nERROR, %s, %d\n", __func__, __LINE__);
		return PRIVATE_SECTION_OK;
	}

	section_length = ( ((uint16_t) data[1] << 8) | data[2] ) & 0xFFF;

	if(section_length > 0)
	{
		parse_result.parsed_data = data;
		parse_result.table_id = data[0];
		parse_result.request_id= tkgs_class.tkgs_subtable_id.request_id;
		parse_result.si_subtable_id = tkgs_class.tkgs_subtable_id.subtable_id;

		if((tkgs_class.search_status & TKGS_START_CHECK_VER) == TKGS_START_CHECK_VER)
		{
			TKGS_DEBUG_PRINTF("===%s, %d,check ver\n", __FUNCTION__, __LINE__);
			//?????汾,??��?汾????????
			tkgs_class.tkgs_version = (data[5]&0x3E)>>1;
		}
		else if((tkgs_class.search_status & TKGS_START_UPDATE) == TKGS_START_UPDATE)
		{
			TKGS_DEBUG_PRINTF("===%s, %d, update\n", __FUNCTION__, __LINE__);
			if (GXCORE_SUCCESS !=gx_tkgs_init_databuff(&parse_result))
			{
				TKGS_DEBUG_PRINTF("===%s, %d, update\n", __FUNCTION__, __LINE__);
				gx_tkgs_status_reply(GX_TKGS_UPDATE_ERROR, NULL);
				gx_tkgs_variable_exit_init();
				GxBus_SiParserBufFree(parse_result.parsed_data);
				return PRIVATE_SECTION_OK;
			}

			//????tkgs????
			gx_tkgs_section_process(&parse_result);
		}
	}

    return PRIVATE_SECTION_OK;
}

/* Exported Functions ----------------------------------------------------- */

/**
 * @brief
 * @param
 * @Return
 */
extern void  gx_tkgs_backupgrade_first_init(void);
extern status_t app_tkgs_set_default_location(void);
extern void app_tkgs_set_operatemode_var(GxTkgsOperatingMode mode);
status_t GxTkgsInit(handle_t self,int priority_offset)
{
	handle_t                sch;
    GxTkgsOperatingMode     operation_mode = GX_TKGS_AUTOMATIC;

	// init operation mode
    //app_tkgs_init_operatemode();
    gx_tkgs_get_operating_mode(&operation_mode);
    app_tkgs_set_operatemode_var(operation_mode);
    // init tkgs background rule
	gx_tkgs_backupgrade_first_init();
    // init default loction
	app_tkgs_set_default_location();

	GxBus_MessageRegister(GXMSG_TKGS_CHECK_BY_PMT,sizeof(GxTkgsPmtCheckParm));
	GxBus_MessageRegister(GXMSG_TKGS_CHECK_BY_TP,sizeof(GxTkgsTpCheckParm));
	GxBus_MessageRegister(GXMSG_TKGS_START_VERSION_CHECK,sizeof(GxTkgsUpdateParm));
	GxBus_MessageRegister(GXMSG_TKGS_START_UPDATE,sizeof(GxTkgsUpdateParm));
	GxBus_MessageRegister(GXMSG_TKGS_STOP,0);
	GxBus_MessageRegister(GXMSG_TKGS_UPDATE_STATUS, sizeof(GxTkgsUpdateMsg));
	GxBus_MessageRegister(GXMSG_TKGS_SET_LOCATION,sizeof(GxTkgsLocation));
	GxBus_MessageRegister(GXMSG_TKGS_GET_LOCATION,sizeof(GxTkgsLocation));

	GxBus_MessageRegister(GXMSG_TKGS_SET_OPERATING_MODE,sizeof(GxTkgsOperatingMode));
	GxBus_MessageRegister(GXMSG_TKGS_GET_OPERATING_MODE,sizeof(GxTkgsOperatingMode));
	GxBus_MessageRegister(GXMSG_TKGS_GET_PREFERRED_LISTS,sizeof(GxTkgsPreferredList));
	GxBus_MessageRegister(GXMSG_TKGS_SET_PREFERRED_LIST,sizeof(ubyte64));
	GxBus_MessageRegister(GXMSG_TKGS_GET_PREFERRED_LIST,sizeof(ubyte64));
	GxBus_MessageRegister(GXMSG_TKGS_BLOCKING_CHECK,sizeof(GxTkgsBlockCheckParm));
	GxBus_MessageRegister(GXMSG_TKGS_TEST_GET_VERNUM,sizeof(int));
	GxBus_MessageRegister(GXMSG_TKGS_TEST_RESET_VERNUM,0);
	GxBus_MessageRegister(GXMSG_TKGS_TEST_CLEAR_HIDDEN_LOCATIONS_LIST,0);
	GxBus_MessageRegister(GXMSG_TKGS_TEST_CLEAR_VISIBLE_LOCATIONS_LIST,0);


	GxBus_MessageListen(self, GXMSG_TKGS_CHECK_BY_PMT);
	GxBus_MessageListen(self, GXMSG_TKGS_CHECK_BY_TP);
	GxBus_MessageListen(self, GXMSG_TKGS_START_VERSION_CHECK);
	GxBus_MessageListen(self, GXMSG_TKGS_START_UPDATE);
	GxBus_MessageListen(self, GXMSG_TKGS_STOP);
	GxBus_MessageListen(self, GXMSG_TKGS_SET_LOCATION);
	GxBus_MessageListen(self, GXMSG_TKGS_GET_LOCATION);
	GxBus_MessageListen(self, GXMSG_TKGS_SET_OPERATING_MODE);
	GxBus_MessageListen(self, GXMSG_TKGS_GET_OPERATING_MODE);
	GxBus_MessageListen(self, GXMSG_TKGS_GET_PREFERRED_LISTS);
	GxBus_MessageListen(self, GXMSG_TKGS_SET_PREFERRED_LIST);
	GxBus_MessageListen(self, GXMSG_TKGS_GET_PREFERRED_LIST);
	GxBus_MessageListen(self, GXMSG_TKGS_BLOCKING_CHECK);
	GxBus_MessageListen(self, GXMSG_TKGS_TEST_GET_VERNUM);
	GxBus_MessageListen(self, GXMSG_TKGS_TEST_RESET_VERNUM);
	GxBus_MessageListen(self, GXMSG_TKGS_TEST_CLEAR_HIDDEN_LOCATIONS_LIST);
	GxBus_MessageListen(self, GXMSG_TKGS_TEST_CLEAR_VISIBLE_LOCATIONS_LIST);
	GxBus_MessageListen(self, GXMSG_SI_SECTION_OK);
	//GxBus_MessageListen(self, GXMSG_SI_SUBTABLE_OK);
	GxBus_MessageListen(self, GXMSG_SI_SUBTABLE_TIME_OUT);

	sch =
	    GxBus_SchedulerCreate("TkgsMsgScheduler", GXBUS_SCHED_MSG,
				  1024 * 32, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);
      sch = GxBus_SchedulerCreate("TkgsConsoleScheduler", GXBUS_SCHED_CONSOLE, 1024 * 32, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);
	return GXCORE_SUCCESS;
}

/**
 * @brief ????????????
 * @param
 * @Return
 */
void GxTkgsDestroy(handle_t self)
{
	GxBus_MessageUnListen(self, GXMSG_TKGS_CHECK_BY_PMT);
	GxBus_MessageUnListen(self, GXMSG_TKGS_CHECK_BY_TP);
	GxBus_MessageUnListen(self, GXMSG_TKGS_START_VERSION_CHECK);
	GxBus_MessageUnListen(self, GXMSG_TKGS_START_UPDATE);
	GxBus_MessageUnListen(self, GXMSG_TKGS_STOP);
	GxBus_MessageUnListen(self, GXMSG_TKGS_SET_LOCATION);
	GxBus_MessageUnListen(self, GXMSG_TKGS_GET_LOCATION);
	GxBus_MessageUnListen(self, GXMSG_TKGS_SET_OPERATING_MODE);
	GxBus_MessageUnListen(self, GXMSG_TKGS_GET_OPERATING_MODE);
	GxBus_MessageUnListen(self, GXMSG_TKGS_GET_PREFERRED_LISTS);
	GxBus_MessageUnListen(self, GXMSG_TKGS_SET_PREFERRED_LIST);
	GxBus_MessageUnListen(self, GXMSG_TKGS_GET_PREFERRED_LIST);
	GxBus_MessageUnListen(self, GXMSG_TKGS_BLOCKING_CHECK);
	GxBus_MessageUnListen(self, GXMSG_TKGS_TEST_GET_VERNUM);
	GxBus_MessageUnListen(self, GXMSG_TKGS_TEST_RESET_VERNUM);
	GxBus_MessageUnListen(self, GXMSG_TKGS_TEST_CLEAR_HIDDEN_LOCATIONS_LIST);
	GxBus_MessageUnListen(self, GXMSG_TKGS_TEST_CLEAR_VISIBLE_LOCATIONS_LIST);
	GxBus_MessageUnListen(self, GXMSG_SI_SECTION_OK);
	//GxBus_MessageUnListen(self, GXMSG_SI_SUBTABLE_OK);
	GxBus_MessageUnListen(self, GXMSG_SI_SUBTABLE_TIME_OUT);

	GxBus_MessageUnregister(GXMSG_TKGS_CHECK_BY_PMT);
	GxBus_MessageUnregister(GXMSG_TKGS_CHECK_BY_TP);
	GxBus_MessageUnregister(GXMSG_TKGS_START_VERSION_CHECK);
	GxBus_MessageUnregister(GXMSG_TKGS_START_UPDATE);
	GxBus_MessageUnregister(GXMSG_TKGS_STOP);
	GxBus_MessageUnregister(GXMSG_TKGS_UPDATE_STATUS);
	GxBus_MessageUnregister(GXMSG_TKGS_SET_LOCATION);
	GxBus_MessageUnregister(GXMSG_TKGS_GET_LOCATION);
	GxBus_MessageUnregister(GXMSG_TKGS_SET_OPERATING_MODE);
	GxBus_MessageUnregister(GXMSG_TKGS_GET_OPERATING_MODE);
	GxBus_MessageUnregister(GXMSG_TKGS_GET_PREFERRED_LISTS);
	GxBus_MessageUnregister(GXMSG_TKGS_SET_PREFERRED_LIST);
	GxBus_MessageUnregister(GXMSG_TKGS_GET_PREFERRED_LIST);
	GxBus_MessageUnregister(GXMSG_TKGS_BLOCKING_CHECK);
	GxBus_MessageUnregister(GXMSG_TKGS_TEST_GET_VERNUM);
	GxBus_MessageUnregister(GXMSG_TKGS_TEST_RESET_VERNUM);
	GxBus_MessageUnregister(GXMSG_TKGS_TEST_CLEAR_HIDDEN_LOCATIONS_LIST);
	GxBus_MessageUnregister(GXMSG_TKGS_TEST_CLEAR_VISIBLE_LOCATIONS_LIST);

	GxBus_ServiceUnlink(self);
	return;
}

/**
 * @brief
 * @param
 * @Return
 */
GxMsgStatus GxTkgsServiceRecvMsg(handle_t self, GxMessage * Msg)
{
	GxMsgStatus status = GXMSG_OK;
	void* search_params = NULL;
	status_t ret = 0;

	//TKGS_DEBUG_PRINTF("=====%s, %d, msgId:%d\n", __FUNCTION__, __LINE__, Msg->msg_id);

	switch (Msg->msg_id)
	{
		case GXMSG_TKGS_CHECK_BY_PMT://ͨ??????PMT???Ƿ???TKGS component descriptor???жϸ?Ƶ???Ƿ??ǰ???TKGS???ݵ?Ƶ?㣬 ͬ????Ϣ;??????PMT??????
			search_params = GxBus_GetMsgPropertyPtr(Msg,GxTkgsPmtCheckParm);
			TKGS_DEBUG_PRINTF("===%s, %d, %s\n", __FUNCTION__, __LINE__, ((GxTkgsPmtCheckParm *)search_params)->pPmtSec);
			status = GxBus_TkgsLocationCheckByPmt((GxTkgsPmtCheckParm *)search_params);
			break;

		case GXMSG_TKGS_CHECK_BY_TP://?Աȱ?????location list??????TP?Ƿ??ǰ???TKGS???ݵ?Ƶ?㣬 ͬ????Ϣ????????TP id

			search_params = GxBus_GetMsgPropertyPtr(Msg,GxTkgsTpCheckParm);
			TKGS_DEBUG_PRINTF("===%s, %d, %d\n", __FUNCTION__, __LINE__, ((GxTkgsTpCheckParm *)search_params)->frequency);
			status = GxBus_TkgsLocationCheckByTp((GxTkgsTpCheckParm *)search_params);
			break;

		case GXMSG_TKGS_START_VERSION_CHECK: //????TKGS?汾???飬???Ƿ????µ??????? ?첽??Ϣ;??????Ϊ??????ʾ????location?б???????Ƶ?㣬??ָ??Ƶ??ʱ??ΪTP id
	        if(tkgs_class.search_status != TKGS_STOP)
		    {
			    TKGS_DEBUG_PRINTF("[SEARCH]---search have runnig!!\n");
				gx_tkgs_status_reply(GX_TKGS_UPDATE_ERROR, NULL);
				gx_tkgs_variable_exit_init();
			    return GXCORE_ERROR;
		    }
			tkgs_class.search_msg_handle = self;
			gx_tkgs_class_init(GX_SEARCH_FRONT_S);
			search_params = GxBus_GetMsgPropertyPtr(Msg,GxTkgsUpdateParm);
			ret = tkgs_class.init_params_func((void*)search_params);
			if(ret != GXCORE_SUCCESS)
			{
				gx_tkgs_status_reply(GX_TKGS_UPDATE_ERROR, NULL);
				gx_tkgs_variable_exit_init();
				return GXCORE_ERROR;
			}

			tkgs_class.tkgs_time_out = APP_TKGS_SECTION_TIMEOUT;//ֻ??ȡһ??section????ʱ5??
			ret = gx_tkgs_update_start();//????TKGS???Ĺ???
			if(ret != GXCORE_SUCCESS)
			{
				gx_tkgs_status_reply(GX_TKGS_UPDATE_ERROR, NULL);
				gx_tkgs_variable_exit_init();
				return GXCORE_ERROR;
			}
			tkgs_class.search_status = TKGS_START_CHECK_VER;
			break;


		case GXMSG_TKGS_START_UPDATE://?????????? ?첽??Ϣ??????????
			if((tkgs_class.search_status & TKGS_CHECK_VER_FINISH) == TKGS_CHECK_VER_FINISH)
			{
				tkgs_class.tkgs_time_out = APP_TKGS_TABLE_TIMEOUT;
				app_tkgs_table_filter_start();//????TKGS???Ĺ???
			}
			else if (tkgs_class.search_status == TKGS_STOP)//??????û?а汾???飬ֱ???����???????Ҫ??ʼ??
			{
				tkgs_class.search_msg_handle = self;
				gx_tkgs_class_init(GX_SEARCH_FRONT_S);
				search_params = GxBus_GetMsgPropertyPtr(Msg,GxTkgsUpdateParm);
				ret = tkgs_class.init_params_func((void*)search_params);
				if(ret != GXCORE_SUCCESS)
				{
					TKGS_DEBUG_PRINTF("=====%s, %d\n", __FUNCTION__, __LINE__);
					gx_tkgs_status_reply(GX_TKGS_UPDATE_ERROR, NULL);
					gx_tkgs_variable_exit_init();
					return GXCORE_ERROR;
				}

				tkgs_class.tkgs_time_out = APP_TKGS_TABLE_TIMEOUT;
				ret = gx_tkgs_update_start();//????TKGS???Ĺ???
				if(ret != GXCORE_SUCCESS)
				{
					TKGS_DEBUG_PRINTF("=====%s, %d\n", __FUNCTION__, __LINE__);
					gx_tkgs_status_reply(GX_TKGS_UPDATE_ERROR, NULL);
					gx_tkgs_variable_exit_init();
					return GXCORE_ERROR;
				}
			}
			tkgs_class.search_status = TKGS_START_UPDATE;
			break;

		case GXMSG_TKGS_STOP:
			if((tkgs_class.search_status & TKGS_UPDATE_PROCESS_DATA) != TKGS_UPDATE_PROCESS_DATA)
			{
		      if(  s_tkgs_force_stop == 1)
				{
					s_tkgs_force_stop = 0;
					//TKGS_DEBUG_PRINTF("=====%s, %d, s_tkgs_force_stop :0x%x\n", __FUNCTION__, __LINE__,s_tkgs_force_stop);
				}
				//TKGS_DEBUG_PRINTF("=====%s, %d, GXMSG_TKGS_STOP\n", __FUNCTION__, __LINE__);
				gx_tkgs_status_reply(GX_TKGS_UPDATE_CANCEL, NULL);
				gx_tkgs_variable_exit_init();
			}
			break;


		case GXMSG_TKGS_SET_LOCATION://????location, ͬ????Ϣ
			search_params = GxBus_GetMsgPropertyPtr(Msg,GxTkgsLocation);
			status = GxBus_TkgsLocationSet((GxTkgsLocation *)search_params);
			break;

		case GXMSG_TKGS_GET_LOCATION://??ȡlocation, ͬ????Ϣ
			search_params = GxBus_GetMsgPropertyPtr(Msg,GxTkgsLocation);
			TKGS_DEBUG_PRINTF("===%s, %d, %d\n", __FUNCTION__, __LINE__, ((GxTkgsLocation *)search_params)->invisible_num);
			status = GxBus_TkgsLocationGet((GxTkgsLocation *)search_params);
			break;

		case GXMSG_TKGS_SET_OPERATING_MODE:	//????ģʽ??KGS Off??Customizable??Automatic?? ͬ????Ϣ
			search_params = GxBus_GetMsgPropertyPtr(Msg,GxTkgsOperatingMode);
			TKGS_DEBUG_PRINTF("===%s, %d, %d\n", __FUNCTION__, __LINE__, *((GxTkgsOperatingMode *)search_params));
			status = gx_tkgs_set_operating_mode((GxTkgsOperatingMode*)search_params);
			break;

		case GXMSG_TKGS_GET_OPERATING_MODE:	//??ȡ???õ?ģʽ??KGS Off??Customizable??Automatic?? ͬ????Ϣ
			search_params = GxBus_GetMsgPropertyPtr(Msg,GxTkgsOperatingMode);
			TKGS_DEBUG_PRINTF("===%s, %d, %d\n", __FUNCTION__, __LINE__, *((GxTkgsOperatingMode *)search_params));
			status = gx_tkgs_get_operating_mode((GxTkgsOperatingMode*)search_params);
			break;

		case GXMSG_TKGS_GET_PREFERRED_LISTS: //??ȡprefered list, ??TKGS?????л?ȡservice list, ???û?ѡ????ͬ????Ϣ?????????ַ??????飻????Ϣ????????????࣬Ҳ??Ҫ??ȡ?????Ե?tkgs section,Ȼ?????н?????service list
			search_params = GxBus_GetMsgPropertyPtr(Msg,GxTkgsPreferredList);
			TKGS_DEBUG_PRINTF("===%s, %d, %d\n", __FUNCTION__, __LINE__,((GxTkgsPreferredList*)search_params)->listNum);
			status = GxBus_TkgsPreferredListGet((GxTkgsPreferredList *)search_params);
			break;

		case GXMSG_TKGS_SET_PREFERRED_LIST://?????û?ѡ?????б���ͬ????Ϣ; ???????ַ����� service list name
			search_params = GxBus_GetMsgPropertyPtr(Msg,ubyte64);
			TKGS_DEBUG_PRINTF("===%s, %d, %s\n", __FUNCTION__, __LINE__,*(ubyte64 *)search_params);
			status = GxBus_TkgsUserPreferredListNameSet(*(ubyte64 *)search_params);
			break;

		case GXMSG_TKGS_GET_PREFERRED_LIST://??ȡ?û?ѡ?????б���ͬ????Ϣ?????????ַ�����service list name
			search_params = GxBus_GetMsgPropertyPtr(Msg,ubyte64);
			TKGS_DEBUG_PRINTF("===%s, %d, %s\n", __FUNCTION__, __LINE__,*(ubyte64 *)search_params);
			status = GxBus_TkgsUserPreferredListNameGet(*(ubyte64 *)search_params);
			break;

		case GXMSG_TKGS_BLOCKING_CHECK://??????Ŀ?Ƿ???block??Ŀ??ͬ????Ϣ??????: ??ĿID
			search_params = GxBus_GetMsgPropertyPtr(Msg,GxTkgsBlockCheckParm);
			TKGS_DEBUG_PRINTF("===%s, %d, %d\n", __FUNCTION__, __LINE__,((GxTkgsBlockCheckParm*)search_params)->prog_id);
			status = GxBus_TkgsBlockingCheck((GxTkgsBlockCheckParm*)search_params);
			break;

		//??????????Ϣ
		case GXMSG_TKGS_TEST_GET_VERNUM: //??ȡTKGS???İ汾??; ͬ????Ϣ?????????汾??
			search_params = GxBus_GetMsgPropertyPtr(Msg,int);
			TKGS_DEBUG_PRINTF("===%s, %d, %d\n", __FUNCTION__, __LINE__,*(int *)search_params);
			status = gx_tkgs_get_version((int *)search_params);
			break;

		case GXMSG_TKGS_TEST_RESET_VERNUM:	//???????????б?????TKGS???汾??; ͬ????Ϣ??????????
			{
				int ver = 0;
				TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
				status = gx_tkgs_save_version(&ver);
			}
			break;

		case GXMSG_TKGS_TEST_CLEAR_HIDDEN_LOCATIONS_LIST:	//????hidden location ?б���ͬ????Ϣ??????????
			{
				GxTkgsLocation location;
				TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
				GxBus_TkgsLocationGet(&location);
				memset(location.invisible_location,0,sizeof(location.invisible_location));
				location.invisible_num = 0;
				status = GxBus_TkgsLocationSet(&location);
			}
			break;

		case GXMSG_TKGS_TEST_CLEAR_VISIBLE_LOCATIONS_LIST:
			{
				GxTkgsLocation location;
				TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
				GxBus_TkgsLocationGet(&location);
				memset(location.visible_location,0,sizeof(location.visible_location));
				location.visible_num = 0;
				status = GxBus_TkgsLocationSet(&location);
			}
			break;

		case GXMSG_SI_SECTION_OK:
			{
				int oldver = 0;

				TKGS_DEBUG_PRINTF("===%s, %d GXMSG_SI_SECTION_OK\n", __FUNCTION__, __LINE__);

				if((tkgs_class.search_status & TKGS_START_CHECK_VER) == TKGS_START_CHECK_VER)
				{
					TKGS_DEBUG_PRINTF("===%s, %d,check ver\n", __FUNCTION__, __LINE__);
					gx_tkgs_get_version(&oldver);
					TKGS_DEBUG_PRINTF("===%s, %d, new ver:%d, old ver:%d\n", __FUNCTION__, __LINE__, tkgs_class.tkgs_version, oldver);
					if (tkgs_class.tkgs_version == oldver)
					{
						gx_tkgs_status_reply(GX_TKGS_FOUND_SAME_VER, NULL);
					}
					else
					{
						gx_tkgs_status_reply(GX_TKGS_FOUND_NEW_VER, NULL);
					}

					//GxBus_SiParserBufFree(data->parsed_data);

					app_tkgs_table_filter_release();
					tkgs_class.search_status = TKGS_CHECK_VER_FINISH;
				}
				else if((tkgs_class.search_status & TKGS_START_UPDATE) == TKGS_START_UPDATE)
				{
					//?ж??Ƿ??Ѿ?ȫ????ȡ
					if (true == gx_tkgs_receive_all_data())
					{
						//release tkgs filter
						app_tkgs_table_filter_release();
						if(  s_tkgs_force_stop == 1)
						{
							s_tkgs_force_stop = 0;
							// TKGS_DEBUG_PRINTF("=====%s, %d, s_tkgs_force_stop :0x%x\n", __FUNCTION__, __LINE__,s_tkgs_force_stop);
							tkgs_class.search_status =TKGS_STOP;
							return GXCORE_ERROR;
						}
						tkgs_class.search_status = TKGS_UPDATE_PROCESS_DATA;
							s_tkgs_saving_data = 1;
                                          	#if 1//(TKGS_EX_FUN == 1) ?пͻ??????˹???ON
						gx_tkgs_turksat_prog_process();
						#endif

						TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
						status = gx_tkgs_data_parse(&tkgs_class);
						if (status == GXCORE_SUCCESS)//????????
						{
							GxBusPmViewInfo sys = {0};
							int curVersion = tkgs_class.tkgs_version;

							gx_tkgs_save_cur_location();
							gx_tkgs_save_version(&curVersion);
                                                 gx_tkgs_get_cur_lcn_list();
							if (tkgs_class.lcnList.tv_lcn_list != NULL)
							{
								GxCore_Free(tkgs_class.lcnList.tv_lcn_list);
								tkgs_class.lcnList.tv_num = 0;
							}

							if (tkgs_class.lcnList.radio_lcn_list != NULL)
							{
								GxCore_Free(tkgs_class.lcnList.radio_lcn_list);
								tkgs_class.lcnList.radio_num = 0;
							}

							if (tkgs_class.search_prog_id_arry != NULL)
							{
								GxCore_Free(tkgs_class.search_prog_id_arry);
								tkgs_class.search_prog_id_arry = NULL;
								GxBus_PmViewInfoGet(&sys);
								if(VIEW_INFO_DISABLE == sys.status)
								{
									sys.status = VIEW_INFO_ENABLE;//?????����?Ŀ??ʱ????Ҫ??��??Ŀ?б?
									GxBus_PmViewInfoModify(&sys);//ͨ???????????????˽?Ŀ?б?
								}
								//GxBus_PmLcnFlush();
								GxBus_PmSync(GXBUS_PM_SYNC_PROG);//ͬ?????????ؽ?prog list??
								GxBus_PmSync(GXBUS_PM_SYNC_EXT_PROG);
							}
							tkgs_class.search_prog_id_num = 0;
							//???????ݿ⣬?????ϲ㷢????Ϣ
							gx_tkgs_status_reply(GX_TKGS_UPDATE_OK, tkgs_class.tkgs_message);
							gx_tkgs_variable_exit_init();
						}
						else
						{
							TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
							//???????���????ʧ?ܣ?Ҫ??Ӧ?÷?????Ϣ
							gx_tkgs_status_reply(GX_TKGS_UPDATE_ERROR, NULL);
							gx_tkgs_variable_exit_init();
						}
					}
					s_tkgs_saving_data = 0;
				}
				status = GXMSG_OK;
			}
			break;

		case GXMSG_SI_SUBTABLE_TIME_OUT:
			{
				GxParseResult		   *parse_result = NULL;

				parse_result = GxBus_GetMsgPropertyPtr(Msg, GxParseResult);
				if ((parse_result==NULL) || (parse_result->table_id != APP_TKGS_TABLE_ID))
				{
					//TKGS_DEBUG_PRINTF("===%s, %d, tableId:%d, si_subtable_id:%d\n", __FUNCTION__, __LINE__, parse_result->table_id, parse_result->si_subtable_id);
					break;
				}

				app_tkgs_table_filter_release();	//release tkgs filter
				if(((tkgs_class.search_status & TKGS_START_CHECK_VER) == TKGS_START_CHECK_VER)
					|| ((tkgs_class.search_status & TKGS_START_UPDATE) == TKGS_START_UPDATE))
				{
					TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
					if (tkgs_class.search_tp_finish_num == tkgs_class.search_tp_num)
					{
						GxTkgsUpdateStatus status = GX_TKGS_UPDATE_TIMEOUT;

						if ((tkgs_class.search_status & TKGS_START_CHECK_VER) == TKGS_START_CHECK_VER)
						{
							status = GX_TKGS_CHECK_VER_TIMEOUT;
						}
						else
						{
							status = GX_TKGS_UPDATE_TIMEOUT;
						}

						TKGS_DEBUG_PRINTF("===%s, %d, status \n", __FUNCTION__, __LINE__, status);
						gx_tkgs_status_reply(status, NULL);
						gx_tkgs_variable_exit_init();
					}
					else
					{
						TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);

						//??????һ??Ƶ??//????TKGS???Ĺ???
						ret = gx_tkgs_update_start();
						if(ret != GXCORE_SUCCESS)
						{
							TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
							gx_tkgs_status_reply(GX_TKGS_UPDATE_ERROR, NULL);
							gx_tkgs_variable_exit_init();
							return GXCORE_ERROR;
						}
					}
				}
			}
			break;

		default:
			break;
	}

	return status;
}

void GxTkgsServiceConsole(handle_t self)
{
    while(1)
    {
		GxCore_ThreadDelay(1000000);
    }
}

GxServiceClass tkgs_service = {
	"tkgs service",
	GxTkgsInit,
	GxTkgsDestroy,
	GxTkgsServiceRecvMsg,
	GxTkgsServiceConsole,
};
#endif
