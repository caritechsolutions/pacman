
#include "app.h"
#include "app_module.h"
#include "app_utility.h"
#include "app_send_msg.h"
#include "app_pop.h"
#include <stdio.h>
#include <fcntl.h>
#include "app_default_params.h"
#include "app_wnd_system_setting_opt.h"

#include "full_screen.h"    /* only for signal state  */
#if FACTORY_TEST_SUPPORT
//===========
#include <sys/time.h>
#include <sys/select.h>

#ifdef ECOS_OS
#include "network.h"
#include "dhcp.h"
#include <sys/socket.h>
#include <pkgconf/libc_string.h>
#endif

#include "app_wnd_search.h"
#include "app_factory_test_date.h"
#include "module/channel_edit.h"


#define MAX_IP_LENGTH	15

#define IMG_BAR_HALF_UNFOCUS "s_bar_half_unfocus"
#define IMG_BAR_HALF_FOCUS_R "s_bar_half_focus_r"
#define IMG_BAR_HALF_FOCUS_L_IP "s_bar_half_focus_l"
#define IMG_BAR_HALF_FOCUS_OK_IP "s_bar_half_focus_ok"

#define EDIT_RS232			"None"
#define EDIT_IP_ADDR			"000.000.000.000"//"192.168.120.201"

#define STR_ID_CA				"CA"
#define STR_ID_SW_CHIP_ID		"SW+CHIP ID"

// factory set
#define	LNB_VOLTAGE_F		"test>lnb_voltage"
#define	CONTROL_22K_F		"test>22k_control"
#define	OUTPUT_MODE_F		"test>output_mode"
#define	SCART_VOLTAGE_F	"test>scart_voltage"
#define	IP_STATE_F			"test>ip_address"

typedef enum
{
	ITEM_RS232 = 0,
	ITEM_LNB_SHORT,
	ITEM_LNB_VOLTAGE,
	ITEM_CONTROL_22K,
	ITEM_OUTPUT_MODE,
	ITEM_SCRAT_VOLTAGE,
	ITEM_IP_STATE,
	ITEM_USB_STATE,
	ITEM_CA,
	ITEM_CW_CHIP_ID,
	ITEM_FACTORY_DEFAULT,
	ITEM_FAC_TEST_TOTAL
}FactoryTestOptSel;

typedef struct SysFactoryTestPara_s
{
	char lnb_voltage;
	char control_22k;
	char output_mode;
	char scart_voltage;
	char ip_address[20];
}SysFactoryTestPara_t;

typedef struct _menu_t
{
	char * imgwidget;
	char * txtwidget;
	char * focuswidget;
}MKMENUWIDGET_T;

static unsigned int IsAddTunerA=0,IsAddTunerB=0;
static  unsigned int iTunerSelected =0;  // 0: tuner A  , 1 : tuner B
static GxMsgProperty_ScanSatStart Search_para;
static event_list* pLNBtime = NULL;

static SysFactoryTestPara_t s_factory_test_info_para;
static FactoryTestOptSel s_FacTestsel = ITEM_RS232;
static SystemSettingOpt s_factory_test_opt;
static SystemSettingItem s_factory_test_item[ITEM_FAC_TEST_TOTAL];

static char *s_test_item_content_text[] ={STR_ID_RS232_SHORT ,STR_ID_LNB_SHORT,STR_ID_LNB_VOLTAGE, STR_ID_22K_CONTROL,
STR_ID_OUTPUT_MODE ,STR_ID_SCART_VOLRAGE, STR_ID_IP_STATE ,STR_ID_USB_STATE,STR_ID_CA,  STR_ID_SW_CHIP_ID,STR_ID_FACTORY_RESET};

static char *s_content_text_ip_address[] = {"None\0","None\0","13\0","of\0f","cvbs\0","0V\0","0.0.0.0\0","OK\0","None\0","1.9.0.0+gx3201\0","Press OK"};

static char *s_test_item_content_edit_cmb[] =
{
	"edit_factory_test_opt1",
	"edit_factory_test_opt2",
	"cmb_factory_test_opt3",
	"cmb_factory_test_opt4",
	"cmb_factory_test_opt5",
	"cmb_factory_test_opt6",
	"edit_factory_test_opt7",
	"btn_factory_test_opt8",
	"cmb_factory_test_opt9",
	"edit_factory_test_opt10",
	"btn_factory_test_opt11"
};

static char *s_test_item_title[] =
{
	"text_factory_test_item1",
	"text_factory_test_item2",
	"text_factory_test_item3",
	"text_factory_test_item4",
	"text_factory_test_item5",
	"text_factory_test_item6",
	"text_factory_test_item7",
	"text_factory_test_item8",
	"text_factory_test_item9",
	"text_factory_test_item10",
	"text_factory_test_item11"
};

static char *s_test_item_choice_back[] =
{
	"img_factory_test_item_choice1",
	"img_factory_test_item_choice2",
	"img_factory_test_item_choice3",
	"img_factory_test_item_choice4",
	"img_factory_test_item_choice5",
	"img_factory_test_item_choice6",
	"img_factory_test_item_choice7",
	"img_factory_test_item_choice8",
	"img_factory_test_item_choice9",
	"img_factory_test_item_choice10",
	"img_factory_test_item_choice11",
};

//static SearchIdSet *sg_search_id_set = NULL;
static uint32_t *sp_scan_id = NULL;
static uint32_t *sp_ts_src = NULL;
extern void _set_disqec_callback(uint32_t sat_id,fe_sec_voltage_t polar,int32_t sat22k);
extern void app_factory_test_search_start(GxMsgProperty_ScanSatStart * PSearch_para);
extern void app_open_irr(void);
SIGNAL_HANDLER int app_factory_test_got_focus(GuiWidget *widget, void *usrdata);

static int app_factory_test_set_search_para(unsigned int tp_id) // app_tp_search_para_get(void)
{
	Search_para.scan_type = GX_SEARCH_MANUAL;
	Search_para.tv_radio = GX_SEARCH_TV_RADIO_ALL;
	Search_para.fta_cas = GX_SEARCH_FTA_CAS_ALL;//GX_SEARCH_FTA;
	Search_para.nit_switch = GX_SEARCH_NIT_DISABLE;

	if (sp_scan_id != NULL)
	{
		GxCore_Free(sp_scan_id);
		sp_scan_id = NULL;
	}
	if (sp_ts_src != NULL)
	{
		GxCore_Free(sp_ts_src);
		sp_ts_src = NULL;
	}
	sp_scan_id = ( unsigned int*)GxCore_Calloc(1, 1 * sizeof( unsigned int));
	sp_ts_src = ( unsigned int*)GxCore_Calloc(1, 1 * sizeof( unsigned int));
	if ((sp_scan_id == NULL) || (sp_ts_src == NULL))
		return -1;

	memcpy(sp_scan_id, &tp_id,  1 * sizeof( unsigned int));
	//if(sg_search_id_set->id_num)
	{
		GxMsgProperty_NodeByIdGet node = {0};
		AppFrontend_Config cfg = {0};
		 unsigned int i = 0;

		node.node_type = NODE_TP;
		node.id = sp_scan_id[0];
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

		node.node_type = NODE_SAT;
		node.id = node.tp_data.sat_id;
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
		#if 0
		//22k  need update here if customer change the 22khz value
		{
			 unsigned int tone22HzSel=0;
			GxMsgProperty_NodeModify NodeModify;
			tone22HzSel = 1;
			GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_CONTROL_22K],"select",&tone22HzSel);

			if(node.sat_data.sat_s.switch_22K != tone22HzSel)
			{
				//todo here for modify sat info
				NodeModify.node_type = NODE_SAT;
				memcpy(&NodeModify.sat_data, &node.sat_data, sizeof(GxBusPmDataSat));
				NodeModify.sat_data.sat_s.switch_22K = tone22HzSel;
				if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &NodeModify))
					app_log_debug("g debug:fuc:%s:line:%d:modify satinfo error\n",__FUNCTION__,__LINE__);
			}
		}
		#endif

		app_ioctl(node.sat_data.tuner, FRONTEND_CONFIG_GET, &cfg);
		for(i = 0; i < 1; i++)
	    		sp_ts_src[i] = cfg.ts_src;
	}

	Search_para.params_s.array = sp_scan_id;
	Search_para.params_s.ts = sp_ts_src;
	Search_para.params_s.max_num = 1;//sg_search_id_set->id_num;

	// needn't ext
	Search_para.ext = NULL;
	Search_para.ext_num = 0;
	Search_para.time_out.pat = TIMEOUT_PAT;
	Search_para.time_out.pmt = TIMEOUT_PMT;
	Search_para.time_out.sdt = TIMEOUT_SDT;
	Search_para.time_out.nit = TIMEOUT_NIT;
	Search_para.search_diseqc = _set_disqec_callback;

	return 0;
}

static void app_factory_test_set_front_para(void)
{
	uint32_t lnbsel = 0 , tone22HzSel = 0;
//	uint32_t tuner =0;
	int32_t  lnbfre=0;
	AppFrontend_SetTp params = {0};
	GxMsgProperty_NodeByPosGet ProgNode;
	GxMsgProperty_NodeByIdGet TpNode;
	GxMsgProperty_NodeByIdGet SatNode;
	AppFrontend_DiseqcParameters diseqc10 = {0};
	GxBusPmSatDiseqc10 chlnb=0;
//	return;
	GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_LNB_VOLTAGE],"select",&lnbsel);
	GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_CONTROL_22K],"select",&tone22HzSel);
	#if 1  //
	//app_log_debug(" debug:func:%s:line:%d:totol:%d:tone22HzSel:%d\n",__FUNCTION__,__LINE__,g_AppPlayOps.normal_play.play_total,tone22HzSel);
	if(g_AppPlayOps.normal_play.play_total == 0)
	{
		params.fre = 1150; //tp = 4000
		params.symb = 27500;
		params.tuner = 0;
		chlnb = 1;

		//tuner = 0; // set tuner 0 first
		//lnbsel == 3? lnbsel = 1: ;
	}
	else
	{
		//get prog node
		ProgNode.node_type = NODE_PROG;
		ProgNode.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);

		//get tp node
		TpNode.node_type = NODE_TP;
		TpNode.id = ProgNode.prog_data.tp_id;
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &TpNode);
		params.fre = TpNode.tp_data.frequency;
		params.symb = TpNode.tp_data.tp_s.symbol_rate;

		//get sat node
		SatNode.node_type = NODE_SAT;
		SatNode.id = ProgNode.prog_data.sat_id;
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET,&SatNode);
		params.tuner = SatNode.sat_data.tuner;
		chlnb = SatNode.sat_data.sat_s.diseqc10;
		switch(TpNode.tp_data.tp_s.polar)
		{
			case GXBUS_PM_TP_POLAR_H:
				lnbfre = SatNode.sat_data.sat_s.lnb1;//sat_s.lnb1;
				break;

			case GXBUS_PM_TP_POLAR_V:
				lnbfre = SatNode.sat_data.sat_s.lnb2;
				break;

			case GXBUS_PM_TP_POLAR_AUTO:
				lnbfre = SatNode.sat_data.sat_s.lnb1;
				break;

			default:
				return ;
				break;
		}
		if(lnbfre >= params.fre)
		{
			params.fre = lnbfre - params.fre;
		}
		else
		{
			params.fre = params.fre - lnbfre;
		}

	}
	//lnbsel == 1? lnbsel = 1: lnbsel;
	{
		GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);
	}

	//MKTech_SetDesiqc();
	//set desiqc
	diseqc10.type = DiSEQC10;
	diseqc10.u.params_10.chDiseqc = chlnb;//SatNode.sat_data.sat_s.diseqc10;
	diseqc10.u.params_10.bPolar = lnbsel;//app_show_to_lnb_polar(&s_CurAtnSat.sat_data);
	diseqc10.u.params_10.b22k = (tone22HzSel == 1?0:1);//app_show_to_sat_22k(s_CurAtnSat.sat_data.sat_s.switch_22K, &s_CurAtnTp.tp_data);
	// set polar
	//GxFrontend_SetVoltage(sat_node.sat_data.tuner, polar);
	AppFrontend_PolarState Polar =diseqc10.u.params_10.bPolar;
	//app_ioctl(params.tuner/*SatNode.sat_data.tuner*/, FRONTEND_POLAR_SET, &Polar);
#if DOUBLE_S2_SUPPORT
	app_ioctl(0/*SatNode.sat_data.tuner*/, FRONTEND_POLAR_SET, &Polar);
	app_ioctl(1/*SatNode.sat_data.tuner*/, FRONTEND_POLAR_SET, &Polar);
	#else
	app_ioctl(0/*SatNode.sat_data.tuner*/, FRONTEND_POLAR_SET, &Polar);
	#endif

	// set 22k
	//GxFrontend_Set22K(sat_node.sat_data.tuner, sat22k);
	AppFrontend_22KState _22K = diseqc10.u.params_10.b22k;
	//app_ioctl(params.tuner/*SatNode.sat_data.tuner*/, FRONTEND_22K_SET, &_22K);
#if DOUBLE_S2_SUPPORT
	app_ioctl(0/*SatNode.sat_data.tuner*/, FRONTEND_22K_SET, &_22K);
	app_ioctl(1/*SatNode.sat_data.tuner*/, FRONTEND_22K_SET, &_22K);
	//app_set_diseqc_10(params.tuner/*SatNode.sat_data.tuner*/,&diseqc10, true);
	app_set_diseqc_10(0/*SatNode.sat_data.tuner*/,&diseqc10, true);
	app_set_diseqc_10(1/*SatNode.sat_data.tuner*/,&diseqc10, true);
	#else
	app_ioctl(0/*SatNode.sat_data.tuner*/, FRONTEND_22K_SET, &_22K);
	app_set_diseqc_10(0/*SatNode.sat_data.tuner*/,&diseqc10, true);
	#endif
	#endif

}

static VideoOutputMode app_factory_test_get_format_for_rca1(VideoOutputMode vout_mode)
{
	VideoOutputMode OutMode = GX_VIDEO_OUTPUT_MODE_MAX;
	switch (vout_mode)
	{
			case GX_VIDEO_OUTPUT_480I:
			case GX_VIDEO_OUTPUT_480P:
			case GX_VIDEO_OUTPUT_720P_60HZ:
			case GX_VIDEO_OUTPUT_1080P_60HZ:
			case GX_VIDEO_OUTPUT_1080I_60HZ:
				OutMode = NTSC_TYPE;//GX_VIDEO_OUTPUT_PAL_M,VIDEO_OUTPUT_NTSC_443
				break;
			case GX_VIDEO_OUTPUT_576I:
			case GX_VIDEO_OUTPUT_1080I_50HZ:
			case GX_VIDEO_OUTPUT_576P:
			case GX_VIDEO_OUTPUT_720P_50HZ:
			case GX_VIDEO_OUTPUT_1080P_50HZ:
				OutMode = GX_VIDEO_OUTPUT_PAL; //VIDEO_OUTPUT_PAL_N, VIDEO_OUTPUT_PAL_NC
				break;
			default:
				app_log_error("\n---wrong format for RCA1----\n");
				break;
	}
	return OutMode;
}

static void app_factory_test_set_av_player_para( void )
{
		uint32_t avoutmode,avscartvoltage;
		int32_t init_value;
		//AvPlayerPara player = {0};
		//AvAppPara AvAppPara= {0};
		//MKTechGetAVPlayerPara(&player);
		//MKTechGetAppAvParaByAvPlayerPara(&player,&AvAppPara);
//return;
		GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_OUTPUT_MODE],"select",&avoutmode);
		GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_SCRAT_VOLTAGE],"select",&avscartvoltage);

		//player.scart_screen = avoutmode;
		//player.scart_source = avscartvoltage;
		//////
		GxMsgProperty_PlayerVideoModeConfig video_mode = {0};
		//GxBus_ConfigSetInt(TV_SCART_KEY, player->scart_source);

		{
			//app_log_debug(" debug :func:%s:line:%d:\n",__FUNCTION__,__LINE__);
			// tv standard
			GxBus_ConfigGetInt(TV_STANDARD_KEY, &init_value, TV_STANDARD);
			//player->vout_mode = init_value;
			//app_log_debug("g debug :func:%s:line:%d:init:%d\n",__FUNCTION__,__LINE__,init_value);
			video_mode.interface = GX_VIDEO_OUTPUT_SCART;// if use this configure, yuv is not support
			video_mode.mode = app_factory_test_get_format_for_rca1(init_value);//(avoutmode == VIDEO_OUTPUT_SOURCE_RGB? VIDEO_OUTPUT_HDMI_576I : app_factory_test_get_format_for_rca1(init_value));//app_av_get_format_for_rca1(player->vout_mode);
			//video_mode.config.scart.source =  avoutmode;//player->scart_source;
			//<content>[4:3,16:9,0V]</content>
			//video_mode.config.scart.screen = (avscartvoltage == 2 ? 0:avscartvoltage); //player->scart_screen;
			app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG,&video_mode);
		}
		#if 0 // close
		if( avscartvoltage ==2)
		{
			app_log_debug(" debug :func:%s:line:%d:\n",__FUNCTION__,__LINE__);
			// PORT 64
			*(volatile unsigned int*)0xd050a134 &= ~(1<<9);
			*(volatile unsigned int*)0xd050a134 |= (1<<8);
			// set for output
			*(volatile unsigned int*)0xd0507000 |= (1<<0);
			// set 0 for output
			//*(volatile unsigned int*)0xd050700c |= (1<<0);
			// set 1 for output
			*(volatile unsigned int*)0xd0507008 |= (1<<0);

			//PORT 67
			*(volatile unsigned int*)0xd050a134 &= ~(1<<9);
			*(volatile unsigned int*)0xd050a134 |= (1<<8);
			// set for output
			*(volatile unsigned int*)0xd0507000 |= (1<<3);
			// set 0 for output
			//*(volatile unsigned int*)0xd050700c |= (1<<3);
			// set 1 for output
			*(volatile unsigned int*)0xd0507008 |= (1<<3);
		}
		#endif
}

static int app_factory_test_modify_sat_tuner_para(int32_t itunersel , void* node)
{
	//if(itunersel != 1) // 限制由tuner B却换到tuner A
	//	return 0;
	GxMsgProperty_NodeByPosGet ProgNode;
	GxMsgProperty_NodeByIdGet TpNode;
	GxMsgProperty_NodeByIdGet SatNode;
//	AppFrontend_DiseqcParameters diseqc10 = {0};
	GxMsgProperty_NodeModify NodeModify;

	//app_log_debug(" debug:line:%d:total:%d\n",__LINE__,g_AppPlayOps.normal_play.play_total);
	if(g_AppPlayOps.normal_play.play_total)
	{
		//get prog node
		ProgNode.node_type = NODE_PROG;
		ProgNode.pos = g_AppPlayOps.normal_play.play_count;
		//app_log_debug(" debug :line:%d:pos:%d\n",__LINE__,ProgNode.pos);
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);

		//get tp node
		TpNode.node_type = NODE_TP;
		TpNode.id = ProgNode.prog_data.tp_id;
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &TpNode);
		//params.fre = TpNode.tp_data.frequency;
		//params.symb = TpNode.tp_data.tp_s.symbol_rate;
		//app_log_debug(" debug :line:%d:tp id:%d\n",__LINE__,TpNode.id);
		//get sat node
		SatNode.node_type = NODE_SAT;
		SatNode.id = ProgNode.prog_data.sat_id;
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET,&SatNode);
	}
	else //如果没有节目
	{
		if(node == NULL)
			return 0;
		memcpy(&SatNode,node,sizeof(GxMsgProperty_NodeByIdGet));
	}
	//app_log_debug(" debug:line:%d:tuner:%d:itunersel:%d:node->tuner:%d\n",__LINE__,SatNode.sat_data.tuner,itunersel,((GxMsgProperty_NodeByIdGet *)node)->sat_data.tuner );
	if(SatNode.sat_data.tuner == itunersel)
		return 0;
	IsAddTunerA = 0; // 启动tuner 1 can add sat

	NodeModify.node_type = NODE_SAT;
	memcpy(&NodeModify.sat_data, &SatNode.sat_data, sizeof(GxBusPmDataSat));
	//memset((char *)NodeModify.sat_data.sat_s.sat_name, 0, MAX_SAT_NAME);
	//memcpy((char *)NodeModify.sat_data.sat_s.sat_name, name_str, MAX_SAT_NAME - 1);
	//NodeModify.sat_data.sat_s.longitude = longitude;
	//NodeModify.sat_data.sat_s.longitude_direct = direct;

#if DOUBLE_S2_SUPPORT
		//GUI_GetProperty("cmb_satlist_popup_input_select", "select", &sel);
		NodeModify.sat_data.tuner = itunersel;// 0:tuner; 1:tuner2
		//s_CurSat.sat_data.tuner = sel;
#endif

		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &NodeModify))
		{
#if DOUBLE_S2_SUPPORT
				  // for view mode backup
            GxBusPmViewInfo view_info_temp = {0};
            uint32_t prog_num = 0;
            unsigned int ModifyFlag = 0;
            unsigned int FlagForChangeViewMode = 0;
            unsigned int i = 0;
            uint32_t* p_id_array = NULL;

            app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&view_info_temp));

			   // step one : all tv program
	GHANGE_MODE:
			if(FlagForChangeViewMode == 0)
			{
                view_info_temp.stream_type = GXBUS_PM_PROG_TV;
				view_info_temp.group_mode = GROUP_MODE_ALL;//all
				view_info_temp.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
			}
			else
			{
				view_info_temp.stream_type = GXBUS_PM_PROG_RADIO;
				view_info_temp.group_mode = GROUP_MODE_ALL;//all
				view_info_temp.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
			}
			FlagForChangeViewMode++;
            prog_num = GxBus_PmProgNumGetByViewInfo(&view_info_temp);
			if(prog_num > 0)
			{
                p_id_array = (uint32_t *)GxCore_Malloc(num * sizeof(uint32_t)); //need GxCore_Free
                if(p_id_array == NULL)
                {
                    app_log_error("malloc p_id_array failed!!!!!!!!!!!!\n");
                }
                GxBus_PmProgIdArryGetByViewInfo(&view_info_temp, prog_num, p_id_array);
				GxMsgProperty_NodeByIdGet ProgramNode = {0};
				for(i = 0; i < NumbGet.node_num; i++)
				{
					ProgramNode.node_type = NODE_PROG;
					ProgramNode.id = p_id_array[i];
					app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &ProgramNode);
					if(NodeModify.sat_data.id == ProgramNode.prog_data.sat_id)
					{
						if(ProgramNode.prog_data.tuner == itunersel)
							break;
						else
						{
							GxMsgProperty_NodeModify node_modify = {0};
							node_modify.node_type = NODE_PROG;
							node_modify.prog_data = ProgramNode.prog_data;
							node_modify.prog_data.tuner = itunersel;
							if(GXCORE_SUCCESS != GxBus_PmProgInfoModify(&(node_modify.prog_data)))
							{
								app_log_error("\nmodify TUNER error\n");
								break;
							}
							ModifyFlag = 1;
						#if 0
							if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify))
							{
								app_log_error("\nmodify TUNER error\n");
								break;
							}
						#endif
						}
					}
				}
                GxCore_Free(p_id_array);
                p_id_array=NULL;
			}

			if(FlagForChangeViewMode < 2)
				goto GHANGE_MODE;

            if(ModifyFlag)
            {
                GxBus_PmSync(GXBUS_PM_SYNC_PROG);
            }
			// recover the view mode
			app_ioctl(SatNode.sat_data.tuner, FRONTEND_DEMUX_CFG, NULL);
#endif
			return 0;
			//s_Changed = 1;
		}

	return 1;
	//todo save the sat para

}

char cChipID[30];
static void app_factory_test_get_chip_id(void)
{
	int cs,byte_addr,onebyte,ctrl_word,i;
	unsigned char otp[32] = {0};
return;
	//app_log_debug("chip_id:\n");
	cs = 5;
	{
		//app_log_debug("\n CUSTOM ID = ");
		for(byte_addr=0; byte_addr<8; byte_addr++)
		{
			ctrl_word = ((cs) << 14) | (byte_addr) | (1<<10) | (1<<12);
			*(volatile unsigned int*)0xd050A194 = ctrl_word;
			i=1000;while(i--);
			*(volatile unsigned int*)0xd050A194 = ctrl_word | (1<<11);
			i=1000;while(i--);
			*(volatile unsigned int*)0xd050A194 = ctrl_word;

			onebyte = (*(volatile unsigned int*)0xd050A190)&0xff;
			otp[byte_addr] = onebyte;
			//app_log_debug("0x%02x ",onebyte);
			//if( (byte_addr & 0x7) == 0x7 )
			//	app_log_debug("\n");
		}

		//app_log_debug("\n CHIP ID = ");
		for(byte_addr=16; byte_addr<24; byte_addr++)
		{// chip id
			ctrl_word = ((cs) << 14) | (byte_addr) | (1<<10) | (1<<12);
			*(volatile unsigned int*)0xd050A194 = ctrl_word;
			i=1000;while(i--);
			*(volatile unsigned int*)0xd050A194 = ctrl_word | (1<<11);
			i=1000;while(i--);
			*(volatile unsigned int*)0xd050A194 = ctrl_word;
			onebyte = (*(volatile unsigned int*)0xd050A190)&0xff;
			otp[byte_addr] = onebyte;
			//app_log_debug("0x%02x ",onebyte);
			//if( (byte_addr & 0xf) == 0xf )
			//	app_log_debug("\n");
		}
	}
	byte_addr = 16;
	snprintf(cChipID,sizeof(cChipID),"%02x%02x%02x%02x%02x%02x%02x%02x",\
		otp[byte_addr+0],otp[byte_addr+1],otp[byte_addr+2],\
		otp[byte_addr+3],otp[byte_addr+4],\
		otp[byte_addr+5],otp[byte_addr+6],otp[byte_addr+7]);
	//app_log_debug(" debug :%c:cChipID:%s\n",cChipID[0],cChipID);

	*(volatile unsigned int*)0xd050A194 = 0;
	//app_log_debug("\n");
}

#ifdef ECOS_OS
//#define LNB_TEST
#define A8293
#ifdef A8293
#define TUNER1_A8293_ADDRESS 0x10
#define TUNER2_A8293_ADDRESS 0x16
static void* _A8293_i2c_open(unsigned int I2CId)
{
	void *i2c = NULL;
	extern void *gx_i2c_open(unsigned int id);

	i2c = gx_i2c_open(I2CId);
	if(i2c == NULL)
		return NULL;
	return i2c;
}
static int _A8293_status(unsigned int A8293ChipId, void* i2c)
{
	//void *i2c = NULL;
	unsigned char status = 0;
	//i2c = gx_i2c_open(I2CId);
	if(i2c == NULL)
		return -1;
	extern int gx_i2c_set(void *dev, unsigned int busid, unsigned int chip_addr, unsigned int address_width, int send_noack, int send_stop);

	extern int gx_i2c_rx(void *dev, unsigned int reg_address, unsigned char *rx_data, unsigned int count);

	gx_i2c_set(i2c, 0, A8293ChipId, 0, 1, 1);
	if (gx_i2c_rx(i2c, 0, &status, 1) == 1)
	{
		if(((status>>2)&0x1) != 0)
			return 1;//short cut
	}
	return 0;
}
#endif

static unsigned char app_factory_test_lnb_short_test(void)
{
	unsigned char		Ret = 0;
	static uint8_t iCount =0 ;
	uint32_t       iIndex =1;
	uint32_t value = 0;
	//volatile unsigned long time_out = 200;
	int32_t tuner = 0;
	uint8_t f[32] = {0};
	handle_t    frontend;

	//Todo usb detection
	if(FALSE == check_usb_status())
	{
		iIndex=0;
		//GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_USB_STATE],"string","None");
	}
	else
	{
		iIndex=1;
		//GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_USB_STATE],"string","OK");
	}
	GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_USB_STATE],"select",&iIndex);


	iIndex = 0x00;

	uint32_t lnbsel=0;
	GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_LNB_VOLTAGE],"select",&lnbsel);
	lnbsel == 3?lnbsel = 1:lnbsel;
//return 0;
	//Test Tuner1
	tuner = 0;
	sprintf((char*)f,"/dev/dvb/adapter0/frontend%d",tuner);
	frontend = open((char*)f,O_RDWR);
	ioctl (frontend, FE_SET_VOLTAGE, lnbsel);//SEC_VOLTAGE_18);

	#if 0
	//GPIO Init GPIO66
	*(volatile unsigned int*)0xd050a134 &= ~(0x1<<9);
	*(volatile unsigned int*)0xd050a134 |= (0x1<<8);//Set Port66 as GPIO
	//Input
	*(volatile unsigned int*)0xd0507000 &= ~(0x1<<2);
	#endif

#ifdef A8293
		void* I2cA = NULL;
		void* I2cB = NULL;
		I2cA = _A8293_i2c_open(0);
		if(I2cA == NULL)
			app_log_error("\n[%s]OPEN I2CA DEVICE FAILED\n",__FUNCTION__);
		I2cB = _A8293_i2c_open(0);
		if(I2cB == NULL)
			app_log_error("\n[%s]OPEN I2CB DEVICE FAILED\n",__FUNCTION__);
		//app_log_error(" debug:line:%d:i2ca:%d:i2cb:%d\n",__LINE__,I2cA,I2cB);
#endif


	{
		GxCore_ThreadDelay(200);
		//value 1:Normal 0:short circuit
		//value = (*(volatile uint32_t*)0xd0507004 >>2) & 1;
		if(value == 0)
		{
#ifdef A8293
			int iRet =0;
			iRet = _A8293_status(TUNER1_A8293_ADDRESS,I2cA);
			//app_log_debug(" debug:line:%d:iRet:%d\n",__LINE__,iRet);
			if( iRet != 1)
				app_log_debug(" debug:line:%d:TUNER 2 :a8293 is not ready\n",__LINE__);
				//; //continue;
			else
#endif
			{
				Ret = 1;// T1短路
				iIndex |= 0x01;
				app_log_info("\n T1 short cut\n");
			}
		}
	}
	close(frontend);

	//Test Tuner2
	tuner = 1;
	//time_out = 200;
	sprintf((char*)f,"/dev/dvb/adapter0/frontend%d",tuner);
	frontend = open((char*)f,O_RDWR);
	ioctl (frontend, FE_SET_VOLTAGE, lnbsel);//SEC_VOLTAGE_18);

	//GPIO Init GPIO32
	*(volatile unsigned int*)0xd050a130 |= (0x3<<2);//Set Port32 as GPIO
	//Input
	*(volatile unsigned int*)0xd0506000 &= ~(0x1<<0);
	//while(time_out--)
	{
		GxCore_ThreadDelay(200);
		//value 1:Normal 0:short circuit
		value = (*(volatile uint32_t*)0xd0506004 >>0) & 1;
		app_log_debug("\n===Lnb Test Tuner2:%d===\n",value);
		if( value == 0)
		{
#ifdef A8293
			int iRet =0;
			iRet = _A8293_status(TUNER2_A8293_ADDRESS,I2cB);
			//app_log_debug(" debug:line%d:iRet:%d\n",__LINE__,iRet);
			if( iRet != 1)
				app_log_debug(" debug:line:%d:TUNER 2 :a8293 is not ready\n",__LINE__);
				//;//continue;
			else
#endif
			{
				Ret = 2;//T2短路
				iIndex |= 0x10;
			}
		}
	}
	close(frontend);
	app_log_debug(" debug :fuc:%s:line:%d:short state:0x%x\n",__FUNCTION__,__LINE__,iIndex & 0x11);
	switch(iIndex & 0x11)
	{
		case 0x00:
			/*避免解决tuner 状态显示不稳定情况*/
			if(iCount ++ < 3)
			{
				break;
			}
			iCount = 0;
			iIndex =0;
			//GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_LNB_SHORT],  "string", "None");
			break;
		case 0x01:
			iIndex = 1;
			iCount = 0;

			//GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_LNB_SHORT],  "string", "Tuner1 Short");
			break;
		case 0x10:
			iIndex = 2;
			iCount = 0;
			//GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_LNB_SHORT],  "string", "Tuner2 Short");
			break;
		case 0x11:
			iIndex = 3;
			iCount = 0;
			//GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_LNB_SHORT],  "string", "Tuner1/Tuner2 Short");
			break;
	}
	GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_LNB_SHORT], "select", &iIndex);
	return Ret;
}

#else
//#define LNB_TEST
unsigned char app_factory_test_lnb_short_test(void)
{
	unsigned char		Ret = 0;
	static uint8_t iCount =0 ;
	uint32_t       iIndex =1;
	uint32_t value;
	volatile unsigned long time_out = 200;
	int32_t tuner = 0;
	uint8_t f[32] = {0};
	handle_t    frontend;

	//Todo usb detection
	if(FALSE == check_usb_status())
    {
		iIndex=0;
    }
	else
	{
		iIndex=1;
	}


	iIndex = 0x00;
//add it by wxj
	uint32_t lnbsel=0;
	GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_LNB_VOLTAGE],"select",&lnbsel);
	lnbsel == 3?lnbsel = 1:lnbsel;

	//Test Tuner1
	tuner = 0;
	sprintf((char*)f,"/dev/dvb/adapter0/frontend%d",tuner);
	frontend = open((char*)f,O_RDWR);
	ioctl (frontend, FE_SET_VOLTAGE, lnbsel);//SEC_VOLTAGE_18);

	//GPIO Init GPIO66
	*(volatile unsigned int*)0xd050a134 &= ~(0x1<<9);
	*(volatile unsigned int*)0xd050a134 |= (0x1<<8);//Set Port66 as GPIO
	//Input
	*(volatile unsigned int*)0xd0507000 &= ~(0x1<<2);
	{
		GxCore_ThreadDelay(200);
		//value 1:Normal 0:short circuit
		value = (*(volatile uint32_t*)0xd0507004 >>2) & 1;
		if(value == 0)
		{
			Ret = 1;// T1短路
			iIndex |= 0x01;
		}
	}
	close(frontend);

	//Test Tuner2
	tuner = 1;
	time_out = 200;
	sprintf((char*)f,"/dev/dvb/adapter0/frontend%d",tuner);
	frontend = open((char*)f,O_RDWR);
	ioctl (frontend, FE_SET_VOLTAGE, lnbsel);//SEC_VOLTAGE_18);

	//GPIO Init GPIO32
	*(volatile unsigned int*)0xd050a130 |= (0x3<<2);//Set Port32 as GPIO
	//Input
	*(volatile unsigned int*)0xd0506000 &= ~(0x1<<0);
	//while(time_out--)
	{
		GxCore_ThreadDelay(200);
		//value 1:Normal 0:short circuit
		value = (*(volatile uint32_t*)0xd0506004 >>0) & 1;
		//app_log_debug("\n[%03d]===Lnb Test Tuner2:%d===\n",(int32_t)time_out,value);
		if(value == 0)
		{
			Ret = 2;//T2短路
			iIndex |= 0x10;
		}
	}
	close(frontend);
	//app_log_debug(" debug :fuc:%s:line:%d:short state:0x%x\n",__FUNCTION__,__LINE__,iIndex & 0x11);
	switch(iIndex & 0x11)
	{
		case 0x00:
			/*避免解决tuner 状态显示不稳定情况*/
			if(iCount ++ < 3)
			{
				break;
			}
			iCount = 0;
			iIndex =0;
			//MKTechSetLed(NULL,0);
			GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_LNB_SHORT], "select", &iIndex);
			break;
		case 0x01:
			iIndex = 1;
			iCount = 0;
			//just for test by wxj
			//MKTechSetLed("8888",1);
			GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_LNB_SHORT], "select", &iIndex);
			break;
		case 0x10:
			iIndex = 2;
			iCount = 0;
			//MKTechSetLed("7777",1);
			GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_LNB_SHORT], "select", &iIndex);
			break;
		case 0x11:
			iIndex = 3;
			iCount = 0;
			//MKTechSetLed("2222",1);
			GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_LNB_SHORT], "select", &iIndex);
			break;
	}
	return Ret;
}
#endif

static int LNBTaskThread[2];

//typedef int (*timer_event) (void *userdata);
//int app_factory_test_timer_event(void * userdata)
static void app_factory_test_timer_event(void * arg)
{
	while(LNBTaskThread[ThreadFlag])
	{
		app_factory_test_lnb_short_test();
		GxCore_ThreadDelay(200);
	}
}

static void app_factory_test_lnb_thread_init(void)
{
	LNBTaskThread[ThreadFlag] = NullThread;
	GxCore_ThreadCreate("lnbshurttest",&LNBTaskThread[ThreadHandle],app_factory_test_timer_event,NULL,200*1024,GXOS_DEFAULT_PRIORITY);

	if(LNBTaskThread[ThreadHandle])
		LNBTaskThread[ThreadFlag] = EnableThread;
}

#ifdef ECOS_OS
static void app_factory_test_pause_theard(int * theard_id)
{
	theard_id[ThreadFlag] = WaitThread;
}

static void app_factory_test_enable_theard(int * theard_id)
{
	if(theard_id[ThreadFlag] == WaitThread || theard_id[ThreadFlag] == DisableThread)
		theard_id[ThreadFlag] = EnableThread;
}

#endif

static int app_factory_test_get_sat_id_by_sat_name( unsigned int * psat_id, unsigned int iTunerSelect,char* SatName)//(&sat_id,iTunerSelected,"C_AsiaSat 3S")
{
	//GxMsgProperty_NodeModify NodeModify;
	GxMsgProperty_NodeByPosGet node = {0};
	GxMsgProperty_NodeNumGet sat_num = {0};
	int ret=0;


	//get total sat num in both tuner
	sat_num.node_type = NODE_SAT;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);

	//app_log_debug(" debug:line%d:satnum:%d\n",__LINE__,sat_num.node_num);
	node.node_type = NODE_SAT;
	//pos : 0 ...  /	id:1 ...
	for(node.pos = 0; node.pos  <=  sat_num.node_num;node.pos ++)
	{
		if( app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET,&node) == GXCORE_SUCCESS  &&
			0 == strcmp((char*)node.sat_data.sat_s.sat_name,SatName))
		{
			#if 1
			if(node.sat_data.tuner != iTunerSelect)
			{
				app_player_close(PLAYER_FOR_NORMAL);
				//app_log_debug(" debug func:%s:line:%d:sat name:%s::id:%d:iTunerSelect:%d\n",__FUNCTION__,__LINE__,node.sat_data.sat_s.sat_name,node.sat_data.id,iTunerSelect);
				app_factory_test_modify_sat_tuner_para(iTunerSelect,&node);

				app_factory_test_got_focus(NULL,NULL);
			}
			#endif
			ret = 1;
			*psat_id = node.sat_data.id;
			//app_log_debug(" debug func:%s:line:%d:sat name:%s::id:%d\n",__FUNCTION__,__LINE__,node.sat_data.sat_s.sat_name,node.sat_data.id);
			break;
		}
		//app_log_debug(" debug func:%s:line:%d:sat name:%s:id:%d\n",__FUNCTION__,__LINE__,node.sat_data.sat_s.sat_name,node.sat_data.id);
	}

	return ret;

}

static int app_factory_test_get_tp_id_by_tp( unsigned int sat_id,int tp, unsigned int * ptp_id)//(sat_id,4000,&tp_id)
{
	GxMsgProperty_NodeByIdGet node;
	GxMsgProperty_NodeNumGet TPNum = {0};
	node.node_type = NODE_TP;
	int ret =0 ;

	TPNum.node_type = NODE_TP;
	TPNum.sat_id = sat_id;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &TPNum);
	//app_log_debug(" debug :line:%d:num:%d\n",__LINE__,TPNum.node_num);

	for(node.pos = 0;node.pos <= TPNum.node_num;node.pos++)
	{
		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET,&node))
		{
			if(node.tp_data.sat_id == sat_id && node.tp_data.frequency == tp)
			{
				*ptp_id = node.tp_data.id;
				ret = 1;
				break;
			}

			//app_log_debug(" debug :line:%d:nodebypos_tp:%d:satid:%d\n",__LINE__,node.tp_data.frequency,node.tp_data.sat_id);
		}
	}
	return ret;
}

static void app_factory_test_set_tuner_property(void)
{
#if DOUBLE_S2_SUPPORT
	if(iTunerSelected)// 0 : 1  to 2		1 : 2 to 1
		GUI_SetProperty("wnd_factorytest_text02","string","To Switch Tuner 1");
	else
		GUI_SetProperty("wnd_factorytest_text02","string","To Switch Tuner 2");
	#else // only tuner one
		GUI_SetProperty("text_factory_test_green","img","");
		GUI_SetProperty("text_factory_test_green","state","hide");
		GUI_SetProperty("wnd_factorytest_text02","string","");
		GUI_SetProperty("img_facory_test_green","state","hide");
	#endif
}

static void app_factory_test_item_data_update(FactoryTestOptSel sel)
{

#define TEXT_FORE_COLOR "[text_color,text_color,text_disable_color]"

	if(s_factory_test_opt.item[sel].itemType == ITEM_CHOICE)
	{
		GUI_GetProperty(s_test_item_content_edit_cmb[sel], "select", &s_factory_test_opt.item[sel].itemProperty.itemPropertyCmb.sel);
	}

	if(s_factory_test_opt.item[sel].itemType == ITEM_EDIT)
	{
		GUI_GetProperty(s_test_item_content_edit_cmb[sel], "string", &s_factory_test_opt.item[sel].itemProperty.itemPropertyEdit.string);
	}

	if(s_factory_test_opt.item[sel].itemType == ITEM_PUSH)
	{
		GUI_GetProperty(s_test_item_content_edit_cmb[sel], "string", &s_factory_test_opt.item[sel].itemProperty.itemPropertyEdit.string);
	}

	GUI_SetProperty(s_test_item_choice_back[sel], "img", IMG_BAR_HALF_UNFOCUS);
	GUI_SetProperty(s_test_item_title[sel], "forecolor", TEXT_FORE_COLOR);
	return;
}

static void app_test_system_para_get(int default_para_tag)
{
	int init_value = 0;

#ifdef ECOS_OS
	GxBus_ConfigGet(IP_STATE_F, s_content_text_ip_address[ITEM_IP_STATE],MAX_IP_LENGTH ,EDIT_IP_ADDR);
	memcpy(&s_factory_test_info_para.ip_address,s_content_text_ip_address[ITEM_IP_STATE],MAX_IP_LENGTH);
#endif
    	GxBus_ConfigGetInt(LNB_VOLTAGE_F, &init_value, 1);
    	s_factory_test_info_para.lnb_voltage= init_value;

	GxBus_ConfigGetInt(CONTROL_22K_F, &init_value, 0);
    	s_factory_test_info_para.control_22k= init_value;

	GxBus_ConfigGetInt(OUTPUT_MODE_F, &init_value, 0);
    	s_factory_test_info_para.output_mode= init_value;

	GxBus_ConfigGetInt(LNB_VOLTAGE_F, &init_value, 0);
    	s_factory_test_info_para.scart_voltage= init_value;

	return;
}

static int app_factory_test_para_save(SysFactoryTestPara_t *ret_para)
{
	int init_value = 0;

	if( ret_para->ip_address != NULL)
	{
		memcpy(s_content_text_ip_address[ITEM_IP_STATE], ret_para->ip_address, MAX_IP_LENGTH);
		GxBus_ConfigSet(IP_STATE_F, s_content_text_ip_address[ITEM_IP_STATE]);
	}

	init_value = ret_para->lnb_voltage;
    	GxBus_ConfigSetInt(LNB_VOLTAGE_F,init_value);

	init_value = ret_para->control_22k;
    	GxBus_ConfigSetInt(CONTROL_22K_F,init_value);

	init_value = ret_para->output_mode;
    	GxBus_ConfigSetInt(OUTPUT_MODE_F,init_value);

	init_value = ret_para->scart_voltage;
    	GxBus_ConfigSetInt(SCART_VOLTAGE_F,init_value);

	return 0;
}


static void app_factory_test_result_para_get(SysFactoryTestPara_t *ret_para)
{

	memcpy(&ret_para->ip_address, s_factory_test_item[ITEM_IP_STATE].itemProperty.itemPropertyEdit.string,MAX_IP_LENGTH);

	ret_para->lnb_voltage= s_factory_test_item[ITEM_LNB_VOLTAGE].itemProperty.itemPropertyCmb.sel;
	ret_para->control_22k= s_factory_test_item[ITEM_LNB_VOLTAGE].itemProperty.itemPropertyCmb.sel;
	ret_para->output_mode= s_factory_test_item[ITEM_LNB_VOLTAGE].itemProperty.itemPropertyCmb.sel;
	ret_para->scart_voltage= s_factory_test_item[ITEM_LNB_VOLTAGE].itemProperty.itemPropertyCmb.sel;

	return;
}

static bool app_factory_test_para_change(SysFactoryTestPara_t *ret_para)
{
	bool ret = FALSE;

	if(strcmp(s_factory_test_info_para.ip_address, ret_para->ip_address)!=0)
	{
		ret = TRUE;
	}

	if(s_factory_test_info_para.lnb_voltage!= ret_para->lnb_voltage)
	{
		ret = TRUE;
	}

	if(s_factory_test_info_para.control_22k!= ret_para->control_22k)
	{
		ret = TRUE;
	}

	if(s_factory_test_info_para.output_mode!= ret_para->output_mode)
	{
		ret = TRUE;
	}

	if(s_factory_test_info_para.scart_voltage!= ret_para->scart_voltage)
	{
		ret = TRUE;
	}

	return ret;
}

static void app_factory_test_edit_item_init(void)
{
	int n_index = 0;
	//int string_index = 0;

	for(n_index = ITEM_RS232; n_index < ITEM_FAC_TEST_TOTAL; n_index++)
	{
		switch(n_index)
		{
			case ITEM_LNB_VOLTAGE:
			case ITEM_CONTROL_22K:
				break;
			case ITEM_OUTPUT_MODE:
			case ITEM_SCRAT_VOLTAGE:
			case ITEM_IP_STATE:
				s_factory_test_item[n_index].itemTitle = s_test_item_content_text[n_index];
			#ifdef ECOS_OS
				s_factory_test_item[n_index].itemStatus = ITEM_DISABLE;
			#endif
				break;
			case ITEM_USB_STATE:
			case ITEM_CA:
				s_factory_test_item[n_index].itemTitle = s_test_item_content_text[n_index];
				s_factory_test_item[n_index].itemType = ITEM_CHOICE;
				break;
			case ITEM_CW_CHIP_ID:
				s_factory_test_item[n_index].itemTitle = s_test_item_content_text[n_index];
				s_factory_test_item[n_index].itemProperty.itemPropertyBtn.string= s_test_item_content_text[n_index];
				s_factory_test_item[n_index].itemType = ITEM_PUSH;
				s_factory_test_item[n_index].itemStatus = ITEM_NORMAL;
				break;
			default:
				s_factory_test_item[n_index].itemTitle = s_test_item_content_text[n_index];
				s_factory_test_item[n_index].itemType = ITEM_EDIT;

				s_factory_test_item[n_index].itemProperty.itemPropertyEdit.format = "edit_string";
				s_factory_test_item[n_index].itemProperty.itemPropertyEdit.maxlen = "35";
				s_factory_test_item[n_index].itemCallback.editCallback.EditPress= NULL;
				s_factory_test_item[n_index].itemProperty.itemPropertyEdit.intaglio = NULL;
				s_factory_test_item[n_index].itemProperty.itemPropertyEdit.default_intaglio = NULL;
				s_factory_test_item[n_index].itemProperty.itemPropertyEdit.string = s_content_text_ip_address[n_index];
				s_factory_test_item[n_index].itemCallback.editCallback.EditReachEnd= NULL;
				s_factory_test_item[n_index].itemStatus = ITEM_NORMAL;
				break;
		}

	}

	return;
}

static int app_fac_test_lnb_volage_change_callback(int sel)
{
	int box_sel;

	GUI_GetProperty("cmb_factory_test_opt1", "select", &box_sel);
	s_factory_test_info_para.lnb_voltage = box_sel;

	return 0;
}

static void app_factory_test_lnb_volage_item_init(void)
{
	s_factory_test_item[ITEM_LNB_VOLTAGE].itemTitle = STR_ID_LNB_VOLTAGE;
	s_factory_test_item[ITEM_LNB_VOLTAGE].itemType = ITEM_CHOICE;
	s_factory_test_item[ITEM_LNB_VOLTAGE].itemProperty.itemPropertyCmb.content= "[13V,18V]";
	s_factory_test_item[ITEM_LNB_VOLTAGE].itemProperty.itemPropertyCmb.sel = s_factory_test_info_para.lnb_voltage;
	s_factory_test_item[ITEM_LNB_VOLTAGE].itemCallback.cmbCallback.CmbChange= app_fac_test_lnb_volage_change_callback;
	s_factory_test_item[ITEM_LNB_VOLTAGE].itemStatus = ITEM_NORMAL;
}

static int app_fac_test_control_22k_change_callback(int sel)
{
	int box_sel;

	GUI_GetProperty("cmb_factory_test_opt2", "select", &box_sel);
	s_factory_test_info_para.control_22k= box_sel;

	return 0;
}

static void app_factory_test_control_22k_item_init(void)
{
	s_factory_test_item[ITEM_CONTROL_22K].itemTitle = STR_ID_22K_CONTROL;
	s_factory_test_item[ITEM_CONTROL_22K].itemType = ITEM_CHOICE;
	s_factory_test_item[ITEM_CONTROL_22K].itemProperty.itemPropertyCmb.content= "[Off,On]";
	s_factory_test_item[ITEM_CONTROL_22K].itemProperty.itemPropertyCmb.sel = s_factory_test_info_para.control_22k;
	s_factory_test_item[ITEM_CONTROL_22K].itemCallback.cmbCallback.CmbChange= app_fac_test_control_22k_change_callback;
	s_factory_test_item[ITEM_CONTROL_22K].itemStatus = ITEM_NORMAL;
}

static int app_fac_test_output_mode_change_callback(int sel)
{
	int box_sel;
	GUI_GetProperty("cmb_factory_test_opt3", "select", &box_sel);
	s_factory_test_info_para.output_mode= box_sel;
	return 0;
}

static void app_factory_test_output_mode_item_init(void)
{
	s_factory_test_item[ITEM_OUTPUT_MODE].itemTitle = STR_ID_OUTPUT_MODE;
	s_factory_test_item[ITEM_OUTPUT_MODE].itemType = ITEM_CHOICE;
	s_factory_test_item[ITEM_OUTPUT_MODE].itemProperty.itemPropertyCmb.content= "[CVBS,RGB]";
	s_factory_test_item[ITEM_OUTPUT_MODE].itemProperty.itemPropertyCmb.sel = s_factory_test_info_para.output_mode;
	s_factory_test_item[ITEM_OUTPUT_MODE].itemCallback.cmbCallback.CmbChange= app_fac_test_output_mode_change_callback;
	s_factory_test_item[ITEM_OUTPUT_MODE].itemStatus = ITEM_NORMAL;
}

static int app_fac_test_scart_voltage_change_callback(int sel)
{
	int box_sel;
	GUI_GetProperty("cmb_factory_test_opt4", "select", &box_sel);
	s_factory_test_info_para.scart_voltage= box_sel;
	return 0;
}

static void app_factory_test_scart_voltage_item_init(void)
{
	s_factory_test_item[ITEM_SCRAT_VOLTAGE].itemTitle = STR_ID_SCART_VOLRAGE;
	s_factory_test_item[ITEM_SCRAT_VOLTAGE].itemType = ITEM_CHOICE;
	s_factory_test_item[ITEM_SCRAT_VOLTAGE].itemProperty.itemPropertyCmb.content= "[4:3,16:9,0V]";
	s_factory_test_item[ITEM_SCRAT_VOLTAGE].itemProperty.itemPropertyCmb.sel = s_factory_test_info_para.scart_voltage;
	s_factory_test_item[ITEM_SCRAT_VOLTAGE].itemCallback.cmbCallback.CmbChange= app_fac_test_scart_voltage_change_callback;
	s_factory_test_item[ITEM_SCRAT_VOLTAGE].itemStatus = ITEM_NORMAL;
}

static void app_factory_test_item_property(int sel, ItemProPerty *property)
{

	if(sel >= ITEM_FAC_TEST_TOTAL)
		return;

	if(s_factory_test_opt.item[sel].itemType == ITEM_CHOICE)
	{
		if(property->itemPropertyCmb.content != NULL)
		{
			GUI_SetProperty(s_test_item_content_edit_cmb[sel], "content", property->itemPropertyCmb.content);
		}
		GUI_SetProperty(s_test_item_content_edit_cmb[sel], "select", &(property->itemPropertyCmb.sel));
	}
	else if(s_factory_test_opt.item[sel].itemType == ITEM_EDIT)
	{
		if(ITEM_FACTORY_DEFAULT!= sel)
		{
			GUI_SetProperty(s_test_item_content_edit_cmb[sel], "clear", NULL);
			GUI_SetProperty(s_test_item_content_edit_cmb[sel], "state", "show");

			if(property->itemPropertyEdit.maxlen != NULL)
			{
				GUI_SetProperty(s_test_item_content_edit_cmb[sel], "maxlen", property->itemPropertyEdit.maxlen);
			}

			if(property->itemPropertyEdit.format != NULL)
			{
				GUI_SetProperty(s_test_item_content_edit_cmb[sel], "format", property->itemPropertyEdit.format);
			}


			if(property->itemPropertyEdit.intaglio != NULL)
			{
				GUI_SetProperty(s_test_item_content_edit_cmb[sel], "intaglio", property->itemPropertyEdit.intaglio);

			}
			else
			{
				GUI_SetProperty(s_test_item_content_edit_cmb[sel], "intaglio", "");
			}

			if(property->itemPropertyEdit.default_intaglio != NULL)
			{
				GUI_SetProperty(s_test_item_content_edit_cmb[sel], "default_intaglio", property->itemPropertyEdit.default_intaglio);
			}
			else
			{
				GUI_SetProperty(s_test_item_content_edit_cmb[sel], "default_intaglio", "");
			}
		}

		if(property->itemPropertyEdit.string != NULL)
		{
			GUI_SetProperty(s_test_item_content_edit_cmb[sel], "string", property->itemPropertyEdit.string);
		}
	}
	GUI_SetProperty(s_test_item_title[sel], "string", s_factory_test_opt.item[sel].itemTitle);
}

static void app_foctory_set_disable_item(FactoryTestOptSel sel)
{
#define DISABLE_FORE_COLOR "[text_disable_color,text_disable_color,text_disable_color]"
	char *state = NULL;
	char *img = NULL;
	char *fore_color = NULL;

//zl
	if(s_factory_test_item[sel].itemStatus == ITEM_DISABLE)
	{
		state = "disable";
		fore_color = DISABLE_FORE_COLOR;
		img = IMG_BAR_HALF_UNFOCUS;

		GUI_SetProperty(s_test_item_content_edit_cmb[sel], "state", state);
		GUI_SetProperty(s_test_item_choice_back[sel], "img", img);
		GUI_SetProperty(s_test_item_title[sel], "forecolor", fore_color);
	}
}

static void app_factory_set_item_state_update(int sel, ItemDisplayStatus state)
{
	if(sel >= ITEM_FAC_TEST_TOTAL)
		return;

	if(state == ITEM_HIDE)
	{
		//app_system_set_hide_item(sel);
	}
	else
	{
		app_foctory_set_disable_item(sel);
	}
}

static int app_factory_test_exit_callback(ExitType exit_type)
{
	int ret = 0;
	SysFactoryTestPara_t para_ret;
	return ret;
	memset(&para_ret,0,sizeof(para_ret));
	app_factory_test_result_para_get(&para_ret);

	if(app_factory_test_para_change(&para_ret) == TRUE)
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
        	pop.type = POP_TYPE_YES_NO;
      	 	pop.str = STR_ID_SAVE_INFO;
		if(popdlg_create(&pop) == POP_VAL_OK)
		{
			app_factory_test_para_save(&para_ret);
			ret = 1;
		}
	}

	return ret;
}


static void app_factory_test_focus_item(int sel)
{
#define FT_FOCUS_FORE_COLOR "[text_focus_color,text_color,text_disable_color]"
	if(s_factory_test_opt.item[sel].itemStatus == ITEM_NORMAL)
	{
		GUI_SetProperty(s_test_item_choice_back[sel], "img", IMG_BAR_HALF_FOCUS_L_IP);
		GUI_SetProperty(s_test_item_title[sel], "string", s_factory_test_opt.item[sel].itemTitle);
		GUI_SetProperty(s_test_item_title[sel], "forecolor", FT_FOCUS_FORE_COLOR);

		if(s_factory_test_opt.item[sel].itemType == ITEM_CHOICE)
		{
			GUI_SetProperty(s_test_item_content_edit_cmb[sel], "state", "focus");
		}
		else if(s_factory_test_opt.item[sel].itemType == ITEM_EDIT)
		{
			GUI_SetProperty(s_test_item_content_edit_cmb[sel], "state", "focus");
		}
		else if(s_factory_test_opt.item[sel].itemType == ITEM_PUSH)
		{
			GUI_SetProperty(s_test_item_content_edit_cmb[sel], "state", "focus");
		}
		s_FacTestsel = sel;

	}
}


static void app_factory_test_unfocus_item(int sel)
{
#define FT_UNFOCUS_FORE_COLOR "[text_color,text_color,text_disable_color]"
	if(s_factory_test_opt.item[sel].itemStatus == ITEM_NORMAL)
	{
		GUI_SetProperty(s_test_item_choice_back[sel], "img", IMG_BAR_HALF_UNFOCUS);
		GUI_SetProperty(s_test_item_title[sel], "string", s_factory_test_opt.item[sel].itemTitle);
		GUI_SetProperty(s_test_item_title[sel], "forecolor", FT_UNFOCUS_FORE_COLOR);
	}
}

static int app_factory_test_init(SystemSettingOpt *settingOpt)
{
	int sel;
	int n_item_total  = 0;

	if(settingOpt == NULL)
		return -1;

	// g_AppFullArb.timer_stop();
      //  g_AppFullArb.state.pause = STATE_OFF;
     //   g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
    g_AppPlayOps.program_stop();
	GUI_CreateDialog(WND_FACTORY_TEST);
	GUI_SetProperty("text_factory_test_title", "string", settingOpt->menuTitle);

	if(settingOpt->itemNum > ITEM_FAC_TEST_TOTAL)
	{
		n_item_total = ITEM_FAC_TEST_TOTAL;
	}
	else
	{
		n_item_total = settingOpt->itemNum;
	}

	for(sel = 0; sel < n_item_total; sel++)
	{
		app_factory_test_item_property(sel, &settingOpt->item[sel].itemProperty);
		app_factory_set_item_state_update(sel, settingOpt->item[sel].itemStatus);

	}
    app_log_flow("_AAAA______e func:%s:line:%d\n",__FUNCTION__,__LINE__);
	s_FacTestsel = ITEM_RS232;
	app_factory_test_focus_item(s_FacTestsel);

	return 0;
}


static FactoryTestOptSel app_factory_test_set_next_sel(FactoryTestOptSel cur_sel)
{
	FactoryTestOptSel sel = cur_sel;

	while(1)
	{
		(sel + 1 == s_factory_test_opt.itemNum) ? sel = ITEM_RS232 : sel++;
		if((s_factory_test_opt.item[sel].itemStatus == ITEM_NORMAL)
			|| (sel == cur_sel))
		{
			return sel;
		}

	}
}

static FactoryTestOptSel app_factory_test_set_last_sel(FactoryTestOptSel cur_sel)
{
	FactoryTestOptSel sel = cur_sel;
	while(1)
	{
		(sel  == ITEM_RS232) ? sel = s_factory_test_opt.itemNum - 1 : sel--;
		if((s_factory_test_opt.item[sel].itemStatus == ITEM_NORMAL)
			|| (sel == cur_sel))
		{
			return sel;
		}
	}
}


static void app_factory_test_update_all_edit_para(void)
{
	int i = 0;
	//return;
	for(i = 0; i < s_factory_test_opt.itemNum; i++)
	{
		app_factory_test_item_data_update(i);
	}

	return;
}

void app_factory_test_menu_exec(void)
{
	memset(&s_factory_test_opt, 0 ,sizeof(s_factory_test_opt));
	app_test_system_para_get(0);

	s_factory_test_opt.menuTitle = STR_ID_FACTORY_TEST;
	s_factory_test_opt.itemNum = ITEM_FAC_TEST_TOTAL;
	s_factory_test_opt.item = s_factory_test_item;

	app_factory_test_lnb_volage_item_init();
	app_factory_test_control_22k_item_init();
	app_factory_test_output_mode_item_init();
	app_factory_test_scart_voltage_item_init();

	app_factory_test_edit_item_init();

	s_factory_test_opt.exit = app_factory_test_exit_callback;
	app_factory_test_init(&s_factory_test_opt);

	return;
}

void app_test_send_key(unsigned int key)
{
    Gui_WriteKeyBuf(key);
    GxCore_ThreadDelay(1000);
	return;
}

static event_list* thiz_test_auto_search_timer = NULL;
static int test_auto_search_func(void *userdata)
{
    const char *wnd = NULL;
    wnd = GUI_GetFocusWindow();

    if(0 == strcasecmp(WND_SEARCH_TIP, wnd))
    {
		app_test_send_key(GUIK_RETURN);
    }

    return 0;
}

int app_test_auto_search_timer_remove(void)
{
	APP_TIMER_REMOVE(thiz_test_auto_search_timer);
	GUI_EndDialog("after wnd_full_screen");
	//g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
	//g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
	//GUI_SetProperty(WND_FULL_SCREEN,"draw_now",NULL);
	g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
	if (g_AppPlayOps.normal_play.play_total != 0)
    {
        g_AppPlayOps.program_play(PLAY_MODE_POINT, 0);//g_AppPlayOps.normal_play.play_count
    }
   return 0;
}

static void _delete_channels_by_tp(int tpid)
{
    GxBusPmViewInfo view_info;
    GxBusPmViewInfo view_info_bak;

    GxMsgProperty_NodeDelete node_del = {0};

    uint32_t* p_id_array = NULL;
    uint32_t num = 0;

    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
    view_info_bak = view_info;
    view_info.stream_type = GXBUS_PM_PROG_TV;
    view_info.group_mode = GROUP_MODE_ALL;//all
    view_info.taxis_mode = TAXIS_MODE_TP;
    view_info.tp_id = tpid;
    view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
    num = GxBus_PmProgNumGetByViewInfo(&view_info);
    app_log_flow("start to del cur TP TV prog \n");
    if (num == 0 )
    {
        app_log_flow("_delete_channels_by_tp not TV program\n");
    }
    else
    {
        p_id_array = (uint32_t*)GxCore_Malloc(num*sizeof(uint32_t));//need GxCore_Free
        if(p_id_array)
        {
            GxBus_PmProgIdArryGetByViewInfo(&view_info, num, p_id_array);
            node_del.node_type = NODE_PROG;
            node_del.id_array = p_id_array;
            node_del.num = num;
            app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node_del);
            GxCore_Free(p_id_array);
            p_id_array = NULL;
        }
        else
        {
            app_log_flow("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        }
    }
    app_log_flow("start to del cur TP Radio prog \n");

    view_info.stream_type = GXBUS_PM_PROG_RADIO;
    num = GxBus_PmProgNumGetByViewInfo(&view_info);
    if(num == 0)
    {
        app_log_flow("_delete_channels_by_tp not radio program\n");
    }
    else
    {
        p_id_array = (uint32_t*)GxCore_Malloc(num*sizeof(uint32_t));//need GxCore_Free
        if(p_id_array)
        {
            GxBus_PmProgIdArryGetByViewInfo(&view_info, num, p_id_array);
            node_del.node_type = NODE_PROG;
            node_del.id_array = p_id_array;
            node_del.num = num;
            app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node_del);
            GxCore_Free(p_id_array);
            p_id_array = NULL;
        }
        else
        {
            app_log_flow("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        }
    }

    g_AppPlayOps.play_list_create(&view_info_bak);
    return ;
}


int app_factory_test_auto_search(void)
{
	unsigned int  sat_id=90,tp_id=0;

	if(g_AppPlayOps.normal_play.play_total != 0)//||(GxFrontend_QueryStatus(iTunerSelected)==1)
	{
		app_log_debug("[%s]--ts--lock,not search,play_total=%d\n",__FUNCTION__,g_AppPlayOps.normal_play.play_total);
		//return 0;
	}

	if(GxFrontend_QueryStatus(iTunerSelected)==1)
	{
		app_log_debug("[%s]--ts--lock,--------,play_total=%d\n",__FUNCTION__,g_AppPlayOps.normal_play.play_total);
		//return 0;
	}

	if( app_factory_test_get_sat_id_by_sat_name(&sat_id,iTunerSelected,"New Sat1")) //"C_AsiaSat 3S" stye 1:read the 3s sat and 4000 tp  in satellite list
	{
		app_factory_test_get_tp_id_by_tp(sat_id,4000,&tp_id);// 3820
	}
	else
	{
		FRONTPARA frontpara={0};
		frontpara.lnb_power = 1;
		//TODO ADD SAT TO SATLIST
		frontpara.switch22k = 1;
		app_factory_test_add_sat(iTunerSelected,&frontpara,&sat_id);
		//TODO ADD TP TO CUR SAT
		app_factory_test_add_tp(sat_id,&tp_id);
	 }

	//TODO SEARCH CHANNEL
	app_factory_test_set_search_para(tp_id);
	app_factory_test_search_start(&Search_para);

	APP_TIMER_ADD(thiz_test_auto_search_timer, test_auto_search_func, 100, TIMER_REPEAT);

	return 0;
}


int app_back_stage_search(void)
{
	GxMsgProperty_NodeByPosGet node_prog = {0};
	int ret = 0;

	node_prog.node_type = NODE_PROG;
	node_prog.pos = g_AppPlayOps.normal_play.play_count;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);


    _delete_channels_by_tp(node_prog.prog_data.tp_id);
	app_factory_test_auto_search();

	return ret;
}

void app_test_sort_channels_by_fta(void)
{
	GxBusPmViewInfo view_info;
	GxMsgProperty_NodeNumGet node_num_get = {0};
	uint32_t i=0;
	sort_mode sort_mode = SORT_FREE_SCRAMBLE;
	uint32_t focus = 0;

	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

	for (i=GXBUS_PM_PROG_TV; i<GXBUS_PM_PROG_RADIO+1; i++)
	{
		view_info.stream_type = i;
		view_info.group_mode = GROUP_MODE_ALL;//all
		view_info.taxis_mode = TAXIS_MODE_NON;//2014.09.23 add
		view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
        GxBus_PmViewInfoMemSet(&view_info);

		node_num_get.node_type = NODE_PROG;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
		if(node_num_get.node_num != 0)
		{
			g_AppChOps.create(node_num_get.node_num);
			g_AppChOps.sort(sort_mode, &focus);
			g_AppChOps.save();
		}
	}

    GxBus_PmViewInfoMemClear();//recovery flash viewinfo
	//GxBus_PmSync(GXBUS_PM_SYNC_PROG);
	return;
}

SIGNAL_HANDLER int app_factory_test_create(GuiWidget *widget, void *usrdata)
{
	uint32_t tone22HzSel=0,isel=0;
	//s_FacTestsel =0 ;
	//MKSetItemFocus(s_FacTestsel);
	//default tuner = 0

	iTunerSelected = 0;
	app_factory_test_set_tuner_property();
	//set scart voltage defualt value
	isel = 2;
	GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_SCRAT_VOLTAGE],"select",&isel);

	//22khz default ON
	tone22HzSel = 1;
	GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_CONTROL_22K],"select",&tone22HzSel);
#if 0
	app_log_debug("___P_____enter app_factory_test_create func:%s:line:%d\n",__FUNCTION__,__LINE__);
	//CA thread
	app_log_info("___P_____ start ca test \n");
	extern void start_ca_test(void);
	start_ca_test();
#endif
	return EVENT_TRANSFER_KEEPON;

	//network thread
	extern void app_network_module_init(void);
	app_network_module_init();

	//uart short
	extern void uartshort_thread(void);
	uartshort_thread();

	app_factory_test_set_front_para();// 22khz   , 13v/18v
	app_factory_test_set_av_player_para(); //rgb/cvbs  , 4:3/16:9

	//test create lnb thread
	app_factory_test_lnb_thread_init();

	char cTemp[40]={0};

	app_factory_test_get_chip_id();
	snprintf(cTemp,sizeof(cTemp),"%s + %s",app_system_info_get_sw_ver(),cChipID);
	//GUI_SetProperty(s_test_item_content_edit_cmb[ITEM_CW_CHIP_ID], "string", cTemp);
	//uint32_t duration = 1500;
	//pLNBtime = create_timer(app_factory_test_timer_event, duration, NULL,  TIMER_REPEAT);
	//GUI_SetProperty(LIST_VIEW_TM, "update_all", NULL);
	//app_factory_test_menu_exec();
	app_log_flow("enter app_factory_test_create func:%s:line:%d\n",__FUNCTION__,__LINE__);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_factory_test_destroy(GuiWidget *widget, void *usrdata)
{
	GxBusPmViewInfo view_info={0};
	GxMsgProperty_NodeByIdGet node = {0};
	AppFrontend_22KState _22K;
	remove_timer(pLNBtime);
	pLNBtime = NULL;

#ifdef ECOS_OS
	// delete thread
	destroy_thread(ca_thread);
	destroy_thread(uart_thread);
	destroy_thread(sg_NetworkTaskThread);
	destroy_thread(LNBTaskThread);
#endif
#if 0
	extern void distory_ca_test(void);
	distory_ca_test();
#endif
	//todo 22k need restore here
	{

		if(g_AppPlayOps.normal_play.play_total != 0)
		{
			app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

			node.node_type = NODE_SAT;
			node.id = view_info.sat_id;
			//app_log_debug(" debug :func:%s:line:%d:tpid :%d\n",__FUNCTION__,__LINE__,node.id);
			app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
			_22K = (node.sat_data.sat_s.switch_22K == 1?0:1);
#if DOUBLE_S2_SUPPORT
			app_ioctl(0/*SatNode.sat_data.tuner*/, FRONTEND_22K_SET, &_22K);
			app_ioctl(1/*SatNode.sat_data.tuner*/, FRONTEND_22K_SET, &_22K);
			//app_set_diseqc_10(params.tuner/*SatNode.sat_data.tuner*/,&diseqc10, true);
			//app_set_diseqc_10(0/*SatNode.sat_data.tuner*/,&diseqc10, true);
			//app_set_diseqc_10(1/*SatNode.sat_data.tuner*/,&diseqc10, true);
			#else
			app_ioctl(0/*SatNode.sat_data.tuner*/, FRONTEND_22K_SET, &_22K);
			//app_set_diseqc_10(0/*SatNode.sat_data.tuner*/,&diseqc10, true);
			#endif
		}
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_factory_test_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
//	WndStatus menu_ret = WND_CANCLE;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;
		case GUI_MOUSEBUTTONDOWN:
			break;
		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_UP:
					app_factory_test_unfocus_item(s_FacTestsel);
					s_FacTestsel = app_factory_test_set_last_sel(s_FacTestsel);
					app_factory_test_focus_item(s_FacTestsel);
					break;
				case STBK_DOWN:
					app_factory_test_unfocus_item(s_FacTestsel);
					s_FacTestsel = app_factory_test_set_next_sel(s_FacTestsel);
					app_factory_test_focus_item(s_FacTestsel);
					break;
				case STBK_OK:
					if(s_FacTestsel == ITEM_FACTORY_DEFAULT)
					{
						PopDlg  pop;
						memset(&pop, 0, sizeof(PopDlg));
						pop.type = POP_TYPE_WAIT;//POP_TYPE_NO_BTN;
						pop.str = STR_ID_PLZ_WAIT;
						pop.format = 0;
						pop.mode = POP_MODE_UNBLOCK;
						extern void FactoryTest_EndDialog(void);
						//app_log_debug(" debug:line:%d:FUNC:%s",__LINE__,__FUNCTION__);
						popdlg_create(&pop);
						//app_log_debug(" debug:line:%d:FUNC:%s",__LINE__,__FUNCTION__);
						app_system_reset();
						//app_log_debug(" debug:line:%d:FUNC:%s",__LINE__,__FUNCTION__);
						//close player
						app_player_close(PLAYER_FOR_NORMAL);
                        app_open_irr();
						FactoryTest_EndDialog();
						//app_log_debug(" debug:line:%d:FUNC:%s",__LINE__,__FUNCTION__);
						IsAddTunerA =0;
						IsAddTunerB =0;
					}
					else if(s_FacTestsel == ITEM_USB_STATE)
					{
						if(FALSE == check_usb_status())
						{
							PopDlg  pop;
							memset(&pop, 0, sizeof(PopDlg));
							pop.type = POP_TYPE_OK;
							pop.str = STR_ID_NO_DEVICE;
							pop.mode = POP_MODE_UNBLOCK;
							popdlg_create(&pop);
						}
                        else
                        {
                            PopDlg  pop;
							memset(&pop, 0, sizeof(PopDlg));
							pop.type = POP_TYPE_OK;
						    pop.str = STR_ID_USB_IN;
							pop.mode = POP_MODE_UNBLOCK;
							popdlg_create(&pop);
                        }
					}
					break;

				case STBK_RED:
					{
						 unsigned int  sat_id=0,tp_id=0;
						#ifdef ECOS_OS
						//close net work theard
						app_factory_test_pause_theard(sg_NetworkTaskThread);
						while (sg_NetworkTaskThread[ThreadHandle] != 0 && sg_NetworkTaskThread[ThreadFlag] != DisableThread)
						{
							GxCore_ThreadDelay(200);
							#ifdef  FACTORY_TEST_NETWORK_SUPPORT
							if(network_check_cable_link() == 0)//check the net
								break;
							#endif
						}
						#endif
						#if 1 //stale 1 ,find the 3s and 4000's id
						/*
							add it by wxj
							2012-11-07
							stye 1:read the 3s sat and 4000 tp  in satellite list
							stye 2 : add new sat and 4000 tp to satelliete list

							the stye 2 is ok now
						*/
						if( app_factory_test_get_sat_id_by_sat_name(&sat_id,iTunerSelected,"C_AsiaSat 3S")) // stye 1:read the 3s sat and 4000 tp  in satellite list
						{
							//app_log_debug(" debug:find ok\n");
							//break;
							app_factory_test_get_tp_id_by_tp(sat_id,4000,&tp_id);
						}
						else //如果没有找到3s卫星，则添加一个temp 卫星
						//#else // stye 2 : add new sat and 4000 tp to satelliete list
						{
							if( (0==iTunerSelected && 0==IsAddTunerA) || \
								(1==iTunerSelected && 0==IsAddTunerB) )
							{
								//GXBUS_PM_SAT_LNB_POWER_ON;
								//GXBUS_PM_SAT_22K_OFF;
								FRONTPARA frontpara={0};
								 unsigned int isel=0;
								//GUI_GetProperty(,"select",&frontpara.lnb_power)

								GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_LNB_VOLTAGE],"select",&isel);
								isel >= 2 ? (isel -= 2):(isel += 2);
								frontpara.lnb_power = isel;
								GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_CONTROL_22K],"select",&frontpara.switch22k);

								//TODO ADD SAT TO SATLIST

								app_factory_test_add_sat(iTunerSelected,&frontpara,&sat_id);
								//TODO ADD TP TO CUR SAT
								app_factory_test_add_tp(sat_id,&tp_id);

								iTunerSelected == 0 ? (IsAddTunerA = tp_id) : (IsAddTunerB = tp_id);
							}

							iTunerSelected == 0 ? (tp_id = IsAddTunerA) : (tp_id = IsAddTunerB);

						}
						#endif

						//TODO SEARCH CHANNEL
						app_factory_test_set_search_para(tp_id);
						app_factory_test_search_start(&Search_para);
					}
					break;
				case STBK_EXIT:
				case STBK_MENU:
					app_factory_test_update_all_edit_para();
					if(s_factory_test_opt.exit != NULL)
					{
						if(s_factory_test_opt.exit(0) == 1)
						{
							//menu_ret = WND_OK;
						}
					}

					#if 1
					GUI_EndDialog("after wnd_full_screen");
					#else
					GUI_EndDialog(WND_FACTORY_TEST);
					//GUI_SetProperty(WND_FACTORY_TEST,"draw_now",NULL);
					if( GUI_CheckDialog(WND_FACTORY_TEST) == GXCORE_SUCCESS)
					{
						GUI_EndDialog(WND_COMMON_SELECT);
					}
					#endif
					g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);

					if (g_AppPlayOps.normal_play.play_total != 0)
					{
						app_play_current_prog(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);
					}
					break;

				case STBK_GREEN:
					//TODO Switch tuner
#if DOUBLE_S2_SUPPORT
					iTunerSelected = 1 - iTunerSelected;
					app_factory_test_set_tuner_property();

					if (iTunerSelected == 0)
						break;
					app_player_close(PLAYER_FOR_NORMAL);
					app_factory_test_modify_sat_tuner_para(iTunerSelected,NULL);
					app_factory_test_got_focus(NULL,NULL);
					#endif
					break;
				default:
					if(STBK_0<= event->key.sym && STBK_9 >= event->key.sym)
					{
						app_factory_test_unfocus_item(s_FacTestsel);
						s_FacTestsel = event->key.sym - STBK_0 - 1;
						if(s_FacTestsel >= ITEM_FAC_TEST_TOTAL-1 )
							s_FacTestsel = ITEM_FAC_TEST_TOTAL -1;
						app_factory_test_focus_item(s_FacTestsel);
					}
					break;
			}

		default:
			break;
	}
	return ret;
}

SIGNAL_HANDLER int app_factory_test_edit3_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_factory_test_opt.itemNum > ITEM_IP_STATE) && (s_factory_test_opt.item[ITEM_IP_STATE].itemCallback.editCallback.EditPress != NULL))
		{
			ret = s_factory_test_opt.item[ITEM_IP_STATE].itemCallback.editCallback.EditPress(event->key.sym);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_factory_test_edit3_reach_end(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	if((s_factory_test_opt.itemNum > ITEM_IP_STATE) && (s_factory_test_opt.item[ITEM_IP_STATE].itemCallback.editCallback.EditReachEnd!= NULL))
	{
		char *s = NULL;
		GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_IP_STATE], "string", &s);
		ret = s_factory_test_opt.item[ITEM_IP_STATE].itemCallback.editCallback.EditReachEnd(s);
	}
	return ret;
}


SIGNAL_HANDLER int app_factory_test_cmb1_change(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;

	if( s_FacTestsel == ITEM_LNB_VOLTAGE || s_FacTestsel == ITEM_CONTROL_22K)
	{
		app_factory_test_set_front_para();
	}
	else if(s_FacTestsel ==ITEM_OUTPUT_MODE || s_FacTestsel == ITEM_SCRAT_VOLTAGE)
	{
		app_factory_test_set_av_player_para();
	}
	else
	{
		if((s_factory_test_opt.itemNum > ITEM_LNB_VOLTAGE) && ( s_factory_test_opt.item[ITEM_LNB_VOLTAGE].itemCallback.cmbCallback.CmbChange!= NULL))
		{
			int sel = 0;
			GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_LNB_VOLTAGE], "select", &sel);
			GUI_GetProperty(s_test_item_content_edit_cmb[ITEM_LNB_VOLTAGE], "select", &s_factory_test_opt.item[ITEM_LNB_VOLTAGE].itemProperty.itemPropertyCmb.sel);

			ret =  s_factory_test_opt.item[ITEM_LNB_VOLTAGE].itemCallback.cmbCallback.CmbChange(sel);

		}
	}
	return ret;
}

SIGNAL_HANDLER int app_factory_test_lost_focus(GuiWidget *widget, void *usrdata)
{
    GUI_SetProperty(s_test_item_content_edit_cmb[s_FacTestsel], "unfocus_img", IMG_BAR_HALF_FOCUS_R);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_factory_test_got_focus(GuiWidget *widget, void *usrdata)
{
    GUI_SetProperty(s_test_item_content_edit_cmb[s_FacTestsel], "unfocus_img", IMG_BAR_HALF_UNFOCUS);
	app_log_debug("_____ debug func:%s:line:%d\n",__FUNCTION__,__LINE__);

	//g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
#ifdef ECOS_OS
	app_factory_test_enable_theard(sg_NetworkTaskThread);
#endif

	//if (g_AppPlayOps.normal_play.play_total != 0)
	if(0)
	{
		app_play_current_prog(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);

		if (g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK)
			g_AppFullArb.tip = FULL_STATE_LOCKED;
		else
			g_AppFullArb.tip = FULL_STATE_DUMMY;

		//control 22khz when switch tuner between tuner 1 and tuner 2
		app_factory_test_set_front_para();
	}
    return EVENT_TRANSFER_STOP;
}

#endif
