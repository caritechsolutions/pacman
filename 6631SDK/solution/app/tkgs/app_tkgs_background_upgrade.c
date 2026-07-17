#include "app_module.h"
#include "module/si/si_pmt.h"
#include "gxsi.h"
#include "gxmsg.h"
#include "app_send_msg.h"
#include "module/config/gxconfig.h"
#include "app_default_params.h"
#include "full_screen.h"
//#include "module/app_ca_manager.h"
//#include "cim_includes.h"
#include "gxcore.h"
#if TKGS_SUPPORT
#include "app_tkgs_upgrade.h"
#include "app_tkgs_background_upgrade.h"
#include "module/tkgs/tkgs_protocol.h"

#define TKGSBACKUPGRADE_DEBUG 0
#if TKGSBACKUPGRADE_DEBUG
#define TKGS_BACKUPGRADE_PRINTF(...) app_log_debug( __VA_ARGS__ )

#define TKGS_BACKUPGRADE_ERROR_PRINTF(msg)\
do{\
   TKGS_BACKUPGRADE_PRINTF("\n\n*****tkgs background upgrade error*****\n");\
   TKGS_BACKUPGRADE_PRINTF("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__ );\
   TKGS_BACKUPGRADE_PRINTF("%s\n",msg);\
   TKGS_BACKUPGRADE_PRINTF("~~~~~search error~~~~~\n");\
}while(0)

#else
#define TKGS_BACKUPGRADE_PRINTF(...) do{}while(0)
#define TKGS_BACKUPGRADE_ERROR_PRINTF(msg) do{}while(0)
#endif
//stream type
#define  MPEG_1_VIDEO		0x01
#define  MPEG_2_VIDEO		0x02
#define  MPEG_4_VIDEO		0x10
#define  MPEG_1_AUDIO		0x03
#define  MPEG_2_AUDIO		0x04
#define  H264				0x1b
#define	 H265				0x24
#define  AAC_ADTS			0xf
#define  AAC_LATM			0x11
#define  AVS				0x42
#define	 PRIVATE_PES_STREAM  0x06//出现在pmt中一般是EAC3
#define	 LPCM  0x80
#define	 AC3  0x81
#define	 DTS  0x82
#define	 DOLBY_TRUEHD  0x83
#define	 AC3_PLUS  0x84
#define	 DTS_HD  0x85
#define	 DTS_MA  0x86
#define	 AC3_PLUS_SEC  0xa1
#define	 DTS_HD_SEC  0xa2

#define PAT_TIME_OUT 5000
#define PMT_TIME_OUT 5000
extern void gx_tkgs_backupgrade_variable_exit(void);
extern status_t gx_tkgs_pmt_filter_create(void);
extern int app_tkgs_get_turksat_pInfo(GxMsgProperty_NodeByPosGet ** p_TKGSSat);
extern int app_tkgs_set_upgrade_stop(void);
extern void _tkgsupgradeprocess_upgrade_version_check(uint16_t pid);
extern APP_TKGS_UPGRADE_MODE app_tkgs_get_upgrade_mode(void);
extern bool app_check_av_running(void);

status_t gx_tkgs_backupgrade_servicestart(void)
{
	GxTkgsTpCheckParm TkgsTpCheckParm = {0};
	GxMsgProperty_NodeByIdGet TpNode = {0};

	if(app_tkgs_get_upgrade_mode() == TKGS_UPGRADE_MODE_STANDBY)
          return GXCORE_SUCCESS;

	if(g_AppTkgsBackUpgrade.tkgs_component_flag >0)
	{
		app_tkgs_set_upgrade_mode(TKGS_UPGRADE_MODE_BACKGROUND);
		TKGS_BACKUPGRADE_PRINTF("func=%s ,line=%d\n",__FUNCTION__,__LINE__);
		_tkgsupgradeprocess_upgrade_version_check(g_AppTkgsBackUpgrade.pid);
		return GXCORE_SUCCESS;
	}

	TpNode.node_type = NODE_TP;
	TpNode.id = g_AppTkgsBackUpgrade.tp_id;
	TKGS_BACKUPGRADE_PRINTF("tp_id= %d\n",g_AppTkgsBackUpgrade.tp_id);
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &TpNode);

	TkgsTpCheckParm.frequency = TpNode.tp_data.frequency;
	TkgsTpCheckParm.symbol_rate = TpNode.tp_data.tp_s.symbol_rate;
	TkgsTpCheckParm.polar = TpNode.tp_data.tp_s.polar;
	TkgsTpCheckParm.resault = TKGS_LOCATION_NOFOUNE;
	TKGS_BACKUPGRADE_PRINTF("TkgsTpCheckParm.frequency=%d,sym=%d,polar= %d\n",TkgsTpCheckParm.frequency,TkgsTpCheckParm.symbol_rate,TkgsTpCheckParm.polar);
	app_send_msg_exec(GXMSG_TKGS_CHECK_BY_TP,&TkgsTpCheckParm);
	if(TkgsTpCheckParm.resault  != TKGS_LOCATION_NOFOUNE)
	{
		g_AppTkgsBackUpgrade.pid = TkgsTpCheckParm.pid;
		app_tkgs_set_upgrade_mode(TKGS_UPGRADE_MODE_BACKGROUND);
		TKGS_BACKUPGRADE_PRINTF("func=%s ,line=%d\n",__FUNCTION__,__LINE__);
		_tkgsupgradeprocess_upgrade_version_check(g_AppTkgsBackUpgrade.pid);
		return GXCORE_SUCCESS;
	}
	g_AppPmt.ts_src = g_AppPlayOps.normal_play.ts_src;
	g_AppPmt.start(&g_AppPmt);
	return GXCORE_ERROR;
}



status_t gx_tkgs_backupgrade_init(uint32_t sat_id,uint32_t tp_id,uint32_t ts_src)
{
	GxMsgProperty_NodeByPosGet * p_TKGSSat;
	if(app_tkgs_get_turksat_pInfo(&p_TKGSSat) == -1)
	{
		//app_log_error("func=%s ,line=%d\n",__FUNCTION__,__LINE__);
		return GXCORE_ERROR;
	}
	if((g_AppTkgsBackUpgrade.search_status & TKGS_BACKUPGRADE_STOP) == 0)
	{
		g_AppTkgsBackUpgrade.stop();
	}
	//app_log_debug("sat_id=%d,tkgsSatid = %d",sat_id,p_TKGSSat->sat_data.id);
	if(p_TKGSSat->sat_data.id != sat_id)
	{
		//app_log_debug("func=%s ,line=%d\n",__FUNCTION__,__LINE__);
		return GXCORE_ERROR;
	}

	if(GXCORE_ERROR == app_tkgs_set_upgrade_stop())
	{
		//app_log_error("func=%s ,line=%d\n",__FUNCTION__,__LINE__);
		return GXCORE_ERROR;
	}

	g_AppTkgsBackUpgrade.sat_id = sat_id;
	g_AppTkgsBackUpgrade.tp_id = tp_id;
	g_AppTkgsBackUpgrade.ts_src = ts_src;
	g_AppTkgsBackUpgrade.search_demux_id = 0;
	g_AppTkgsBackUpgrade.search_status |= TKGS_BACKUPGRADE_START;
	g_AppTkgsBackUpgrade.tkgs_component_flag  = 0;
	g_AppTkgsBackUpgrade.search_pat_time_out = PAT_TIME_OUT;
	g_AppTkgsBackUpgrade.search_pmt_time_out = PMT_TIME_OUT<<2;
	TKGS_BACKUPGRADE_PRINTF("func=%s ,line=%d\n",__FUNCTION__,__LINE__);

	return GXCORE_SUCCESS;
}

status_t gx_tkgs_backupgrade_create(void)
{
	GxMsgProperty_SiCreate *params_create = NULL;
	GxMsgProperty_SiStart  *params_start = NULL;
	GxSubTableDetail        SubTable = {
		0
	};

	GxMessage              *new_msg = NULL;
	//app_log_debug("func=%s ,line=%d\n",__FUNCTION__,__LINE__);
	if((g_AppTkgsBackUpgrade.search_status & TKGS_BACKUPGRADE_START )== 0)
	{
		g_AppTkgsBackUpgrade.search_status = TKGS_BACKUPGRADE_STOP;
		return GXCORE_ERROR;
	}
	g_AppTkgsBackUpgrade.search_status = TKGS_BACKUPGRADE_RUNNING;


	SubTable.ts_src = g_AppTkgsBackUpgrade.ts_src;

    	SubTable.demux_id = g_AppTkgsBackUpgrade.search_demux_id;

	SubTable.time_out =  g_AppTkgsBackUpgrade.search_pat_time_out;

	SubTable.table_parse_cfg.mode = PARSE_STANDARD_ONLY;

	SubTable.table_parse_cfg.table_parse_fun = NULL;

	SubTable.si_filter.pid = PAT_PID;

	SubTable.si_filter.match_depth = 1;	//需要匹配0字节和5字节,因此匹配深度为6
	SubTable.si_filter.eq_or_neq = 1;

	SubTable.si_filter.match[0] = PAT_TID;

	SubTable.si_filter.mask[0] = 0xff;

	SubTable.si_filter.match[5] = 0x01;

	SubTable.si_filter.mask[5] = 0x00;

	/*发送创建pat的subtable消息给si */
    new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_CREATE);
	params_create =
	    GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiCreate);
	*params_create = &SubTable;

	GxBus_MessageSendWait(new_msg);

	if (SubTable.si_subtable_id == -1)	//创建pat失败
	{
		TKGS_BACKUPGRADE_ERROR_PRINTF("[SEARCH]---ceate pat err!!\n");
		GxBus_MessageFree(new_msg);	//ͬ????Ϣ ??ֵ????�� ʹ??????Ҫfree,??Ϊ???յĵط???NOT_FREE
		return GXCORE_ERROR;
	}

	g_AppTkgsBackUpgrade.search_pat_subtable_id.subtable_id = SubTable.si_subtable_id;
	g_AppTkgsBackUpgrade.search_pat_subtable_id.request_id = SubTable.request_id;

	GxBus_MessageFree(new_msg);	//同步消息 把值传回来 使用完后要free,因为接收的地方是NOT_FREE
	/*发送开始过滤pat的消息给si */
    new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_START);
	params_start = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiStart);
	*params_start = g_AppTkgsBackUpgrade.search_pat_subtable_id.subtable_id;
	GxBus_MessageSend(new_msg);
	g_AppTkgsBackUpgrade.search_status &= ~TKGS_BACKUPGRADE_PAT_STOPING;
	g_AppTkgsBackUpgrade.search_status &= ~TKGS_BACKUPGRADE_START;
	TKGS_BACKUPGRADE_PRINTF("func=%s ,line=%d\n",__FUNCTION__,__LINE__);
	return GXCORE_SUCCESS;
}


status_t gx_tkgs_pat_filter_release(void)
{
	GxMessage              *new_msg = NULL;
	GxMsgProperty_SiRelease *params = NULL;
	/*发送释放pat的subtable消息给si */
    new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_RELEASE);
	params = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiRelease);

	*params = g_AppTkgsBackUpgrade.search_pat_subtable_id.subtable_id;
	GxBus_MessageSendWait(new_msg);

	GxBus_MessageFree(new_msg);	//同步消息 使用完后要free,因为接收的地方是NOT_FREE
	g_AppTkgsBackUpgrade.search_pat_subtable_id.subtable_id = -1;
	g_AppTkgsBackUpgrade.search_status |= TKGS_BACKUPGRADE_PAT_STOPING;
	return GXCORE_SUCCESS;

}

status_t gx_tkgs_pat_info_get(GxParseResult * parse_result)
{
	PatInfo                *pat_info = NULL;

	prog_info_t            *prog_info = NULL;

	uint16_t                i = 0;

	pat_info = (PatInfo *) (parse_result->parsed_data);

	prog_info = (prog_info_t *) (pat_info->prog_info);

	g_AppTkgsBackUpgrade.cur_ts_id = pat_info->si_info.ts_id;
	g_AppTkgsBackUpgrade.search_pat_body_count = pat_info->prog_count;

	g_AppTkgsBackUpgrade.search_pat_body = (GxSearchPatBody *)
	      GxCore_Malloc(sizeof(GxSearchPatBody) * g_AppTkgsBackUpgrade.search_pat_body_count);
	TKGS_BACKUPGRADE_PRINTF("=========in=========\n%s\n ",__FUNCTION__);
	for (i = 0; i < g_AppTkgsBackUpgrade.search_pat_body_count; i++) {
		g_AppTkgsBackUpgrade.search_pat_body[i].prog_number = prog_info[i].prog_num;

		g_AppTkgsBackUpgrade.search_pat_body[i].pmt_pid = prog_info[i].pmt_pid;
		g_AppTkgsBackUpgrade.search_pat_body[i].pat_version = pat_info->si_info.ver_number;
		TKGS_BACKUPGRADE_PRINTF("prog_info[%d] pmt_pid=%d,prog_number=%d\n",i,prog_info[i].pmt_pid ,prog_info[i].prog_num);
	}
	TKGS_BACKUPGRADE_PRINTF("=========out========\n%s\n ",__FUNCTION__);

	//GxBus_SiParserBufFree(parse_result->parsed_data);在释放pat的时候si里面会free

	return GXCORE_SUCCESS;
}

static status_t gx_tkgs_pat_subtable_ok(GxParseResult  *parse_result)
{
	status_t ret = 0;
	//app_log_debug("func=%s ,line=%d\n",__FUNCTION__,__LINE__);

	if (parse_result->si_subtable_id != g_AppTkgsBackUpgrade.search_pat_subtable_id.subtable_id)
	{
		return GXCORE_ERROR;
	}

	gx_tkgs_pat_info_get(parse_result);
	g_AppTkgsBackUpgrade.search_pat_finish_flag = 1;


	if (g_AppTkgsBackUpgrade.search_pat_finish_flag == 1)
	{
		gx_tkgs_pat_filter_release();//开始pmt的时候再释放sdt和pat,这样可以避免pat和sdt其中一张超时,另一张已经完成时无法退出的情况
		ret = gx_tkgs_pmt_filter_create();
		if(GXCORE_SUCCESS != ret)
		{
			//TKGS_BACKUPGRADE_ERROR_PRINTF("[SEARCH]---pmt create faile!!\n");
			return GXCORE_ERROR;
		}
	}
	return GXCORE_SUCCESS;
}

int32_t gx_search_pat_find(GxSearchPatBody * pat)
{
	uint32_t                i = 0;

	for (i = 0; i < g_AppTkgsBackUpgrade.search_pat_body_count; i++) {
		if (g_AppTkgsBackUpgrade.search_pmt_body.service_id ==
		    g_AppTkgsBackUpgrade.search_pat_body[i].prog_number) {
			memcpy(pat, &(g_AppTkgsBackUpgrade.search_pat_body)[i],
			       sizeof(GxSearchPatBody));
			//TKGS_BACKUPGRADE_PRINTF("[SEARCH]---pat pos = %d\n!!", i);
			return i;
		}
	}
	//TKGS_BACKUPGRADE_ERROR_PRINTF("[SEARCH]---pat can't find!!\n");

	return -1;
}

static private_parse_status gx_tkgs_pmt_section_handle(uint8_t *p_section_data, uint32_t len)
{

	GxTkgsPmtCheckParm pmt_check;
	pmt_check.pPmtSec = p_section_data;
	pmt_check.resault = TKGS_LOCATION_NOFOUNE;
	app_send_msg_exec(GXMSG_TKGS_CHECK_BY_PMT, &pmt_check);
	if(pmt_check.resault != TKGS_LOCATION_NOFOUNE)
	{
		g_AppTkgsBackUpgrade.tkgs_component_flag = 1;
		g_AppTkgsBackUpgrade.pid = pmt_check.pid;
	}

	return PRIVATE_SUBTABLE_OK;
}


status_t gx_tkgs_pmt_filter_create(void)
{
	GxMsgProperty_SiCreate *params_create = NULL;
	GxMsgProperty_SiStart  *params_start = NULL;
	GxSubTableDetail        SubTable = { 0 };
	GxMessage              *new_msg = NULL;

	//status_t                ret = 0;

	uint16_t                prog_number = 0;	//相当于service id

	uint16_t                i = 0;

	if (g_AppTkgsBackUpgrade.search_pat_body_count == 0) {
		//TKGS_BACKUPGRADE_ERROR_PRINTF("[SEARCH]---pat body = 0 !!\n");
		return GXCORE_ERROR;
	}

	g_AppTkgsBackUpgrade.tkgs_component_flag = 0;


#define MAX_ONCE_PMT_NUM 8
	do {
	        SubTable.ts_src = g_AppTkgsBackUpgrade.ts_src;

	        SubTable.demux_id = g_AppTkgsBackUpgrade.search_demux_id;

	        SubTable.time_out = g_AppTkgsBackUpgrade.search_pmt_time_out;
		if(g_AppTkgsBackUpgrade.cb != NULL)
		{
	        	SubTable.table_parse_cfg.mode = PARSE_WITH_STANDARD;
	        	SubTable.table_parse_cfg.table_parse_fun = g_AppTkgsBackUpgrade.cb;
		}
		else
		{
            		SubTable.table_parse_cfg.mode = PARSE_STANDARD_ONLY;
	        	SubTable.table_parse_cfg.table_parse_fun = NULL;
		}

		SubTable.si_filter.pid =
		    g_AppTkgsBackUpgrade.search_pat_body[g_AppTkgsBackUpgrade.search_pmt_count].pmt_pid;

		SubTable.si_filter.match_depth = 6;	//需要匹配0字节和5字节,因此匹配深度为6
		SubTable.si_filter.eq_or_neq = 1;

		SubTable.si_filter.match[0] = PMT_TID;

		SubTable.si_filter.mask[0] = 0xff;

		prog_number = g_AppTkgsBackUpgrade.search_pat_body[g_AppTkgsBackUpgrade.search_pmt_count].prog_number;

		SubTable.si_filter.match[3] = ((prog_number >> 8) & 0xff);

		SubTable.si_filter.mask[3] = 0xff;

		SubTable.si_filter.match[4] = (prog_number & 0xff);

		SubTable.si_filter.mask[4] = 0xff;

		SubTable.si_filter.match[5] = 0x00;

		SubTable.si_filter.mask[5] = 0x00;

        /*发送创建pat的subtable消息给si */
		new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_CREATE);
		params_create =
		    GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiCreate);
		*params_create = &SubTable;

		GxBus_MessageSendWait(new_msg);

		if (SubTable.si_subtable_id != -1)	//创建pmt成功
		{
			g_AppTkgsBackUpgrade.search_pmt_subtable_id[g_AppTkgsBackUpgrade.search_pmt_filter_count].subtable_id=
			    SubTable.si_subtable_id;
			g_AppTkgsBackUpgrade.search_pmt_subtable_id[g_AppTkgsBackUpgrade.search_pmt_filter_count].request_id=
			    SubTable.request_id;
			g_AppTkgsBackUpgrade.search_pmt_filter_count++;
			g_AppTkgsBackUpgrade.search_pmt_count++;
		}
		GxBus_MessageFree(new_msg);

	} while ((SubTable.si_subtable_id != -1)
		 && (g_AppTkgsBackUpgrade.search_pmt_count <MAX_ONCE_PMT_NUM));

    /*发送开始过滤pmt的消息给si */
    for (i = 0; i < g_AppTkgsBackUpgrade.search_pmt_filter_count; i++) {
		new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_START);
		params_start =
		    GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiStart);

		*params_start =g_AppTkgsBackUpgrade.search_pmt_subtable_id[i].subtable_id;

		if(GXCORE_SUCCESS !=GxBus_MessageSend(new_msg))
		{
			TKGS_BACKUPGRADE_PRINTF("[func=%s ,line=%d]  EEEEEEEE\n",__FUNCTION__,__LINE__);
		}
	}

	g_AppTkgsBackUpgrade.search_pat_subtable_id.subtable_id = -1;//建立pmt的时候pat应该已经释放了
	if(g_AppTkgsBackUpgrade.search_pmt_filter_count==0)
	{
		//TKGS_BACKUPGRADE_ERROR_PRINTF("[SEARCH]---no pmt can be created!!\n");
		return GXCORE_ERROR;
	}
	g_AppTkgsBackUpgrade.search_status &= ~TKGS_BACKUPGRADE_PMT_STOPING;
	return GXCORE_SUCCESS;
}




status_t gx_search_pmt_filter_release(GxMsgProperty_SiRelease * subtable_id)
{
	GxMessage              *new_msg = NULL;
	GxMsgProperty_SiRelease *params = NULL;
    /*发送释放pat的subtable消息给si */
    new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_RELEASE);
	params = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiRelease);

	*params = *subtable_id;

	GxBus_MessageSend(new_msg);

	*subtable_id = -1;
	return GXCORE_SUCCESS;

}

//#define TKGS_COMPONENT_TAG 0x98

static uint32_t  check_ca_system(uint16_t ca_system_id,uint16_t ele_pid,uint16_t ecm_pid)
{
    return 1;
}
static GxSearchCheckCa gx_search_check_ca = check_ca_system;


status_t gx_search_pmt_cas_get(uint32_t * cas1,
			       uint32_t cas1_count,
			       uint32_t * cas2,
			       uint32_t cas2_count, uint32_t stream_type)
{
	uint32_t                i = 0;
	CaDescriptor           *cas_decriptor = NULL;
	switch (stream_type) {
		/*	case MPEG_1_VIDEO:
			case MPEG_2_VIDEO:
			case AVS:
			case H264:*/
	case 0://视频
		if (cas2 != NULL)	//以分开加密优先
		{
			cas_decriptor = (CaDescriptor *) (cas2[0]);
			g_AppTkgsBackUpgrade.search_pmt_body.ecm_pid_video = cas_decriptor->ca_pid;
			g_AppTkgsBackUpgrade.search_pmt_body.pmt_ca_desc.cas_id = CAS_ID_UNKNOW;
			for (i = 0; i < cas2_count; i++) {
				cas_decriptor = (CaDescriptor *) (cas2[i]);
				if (cas_decriptor->elem_pid ==
				    g_AppTkgsBackUpgrade.search_pmt_body.video_pid) {
					if(gx_search_check_ca(cas_decriptor->ca_system_id,g_AppTkgsBackUpgrade.search_pmt_body.video_pid,cas_decriptor->ca_pid))
					{
						g_AppTkgsBackUpgrade.search_pmt_body.ecm_pid_video =
						    cas_decriptor->ca_pid;
						g_AppTkgsBackUpgrade.search_pmt_body.pmt_ca_desc.cas_id =
						    cas_decriptor->ca_system_id;
						break;
					}
				}

			}
		}
		if (i == cas2_count && cas1 != NULL)	//分开加密没找到,找相同加
		{
			cas_decriptor = (CaDescriptor *) (cas1[0]);
			g_AppTkgsBackUpgrade.search_pmt_body.ecm_pid_video = cas_decriptor->ca_pid;
			g_AppTkgsBackUpgrade.search_pmt_body.pmt_ca_desc.cas_id = CAS_ID_UNKNOW;
			for (i = 0; i < cas1_count; i++) {
				cas_decriptor = (CaDescriptor *) (cas1[i]);
				//if (cas_decriptor->elem_pid ==
				 //  search_class.search_pmt_body.video_pid)
				   {
					if(gx_search_check_ca(cas_decriptor->ca_system_id,g_AppTkgsBackUpgrade.search_pmt_body.video_pid,cas_decriptor->ca_pid))
					{
						g_AppTkgsBackUpgrade.search_pmt_body.ecm_pid_video =
						    cas_decriptor->ca_pid;
						g_AppTkgsBackUpgrade.search_pmt_body.pmt_ca_desc.cas_id =
						    cas_decriptor->ca_system_id;
						break;
					}
				}

			}
		}
		break;

/*	case MPEG_1_AUDIO:
	case MPEG_2_AUDIO:
	case AAC_ADTS:
	case AAC_LATM:*/
	case 1://音频
		if (cas2 != NULL)	//以分开加密优先
		{
			cas_decriptor = (CaDescriptor *) (cas2[0]);
			g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].ecm_pid =
			    cas_decriptor->ca_pid;

			for (i = 0; i < cas2_count; i++) {
				cas_decriptor = (CaDescriptor *) (cas2[i]);
				if (cas_decriptor->elem_pid ==
				    g_AppTkgsBackUpgrade.search_stream_body
				    [g_AppTkgsBackUpgrade.search_stream_body_count].audio_pid) {
					if(gx_search_check_ca(cas_decriptor->ca_system_id,
                                            g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].audio_pid,
                                            cas_decriptor->ca_pid))
					{
						g_AppTkgsBackUpgrade.search_stream_body
						    [g_AppTkgsBackUpgrade.search_stream_body_count].ecm_pid =
						    cas_decriptor->ca_pid;

						break;
					}
				}

			}
		}
		if (i == cas2_count && cas1 != NULL)	//分开加密没找到,找相同加密
		{
			cas_decriptor = (CaDescriptor *) (cas1[0]);
			g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].ecm_pid =
			    cas_decriptor->ca_pid;

			for (i = 0; i < cas1_count; i++) {
				cas_decriptor = (CaDescriptor *) (cas1[i]);
				//if (cas_decriptor->elem_pid ==
				//    search_class.search_stream_body
				 //   [search_class.search_stream_body_count].audio_pid)
				   {
					if(gx_search_check_ca(cas_decriptor->ca_system_id,
                                            g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].audio_pid,
                                            cas_decriptor->ca_pid))
					{
						g_AppTkgsBackUpgrade.search_stream_body
						    [g_AppTkgsBackUpgrade.search_stream_body_count].ecm_pid =
						    cas_decriptor->ca_pid;

						break;
					}
				}

			}
		}
		break;
	}
	return GXCORE_SUCCESS;
}

static status_t gx_tkgs_backupgrade_prog_info_modify(uint32_t * prog_id)
{
	//status_t                ret = 0;
	int32_t                 count = 0;
	uint16_t                ts_id = 0;
	uint16_t                service_id = 0;
	int32_t                 num = 0;
	//GxSearchSdtBody         sdt_info = { 0 };
	GxSearchPatBody         pat_info = { 0 };
	GxBusPmDataProg        prog = { 0 };
	GxBusPmDataProg        progbak = {0};
	//uint32_t i = 0;
    //GxBusPmDataProgAudioType type = 0xff;
	//uint32_t a_pid_temp = 0xffff;
	//uint32_t v_pid_temp = 0;
	//uint32_t pcr_pid_temp = 0;

    /*寻找该节目对应的sdt,sdt_num -1错误  其他值正常 */

	/*寻找该节目的pat */
	num = gx_search_pat_find(&pat_info);
	if (num == -1)
	{
		#ifdef GX_BUS_SEARCH_DBUG
		GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---find pat err!!\n");
		#endif
		return GXCORE_ERROR;
	}
	ts_id = -1;
	service_id = g_AppTkgsBackUpgrade.search_pmt_body.service_id;
	//app_log_debug("tp_id=%d,service_id=%d,ts_src =%d\n",g_AppTkgsBackUpgrade.tp_id,service_id,g_AppTkgsBackUpgrade.ts_src);
	count = GxBus_PmProgExistChek(g_AppTkgsBackUpgrade.tp_id,
			ts_id,
			service_id,
			-1,
			g_AppTkgsBackUpgrade.ts_src,
            0xff,
            0xff,
            0xff);
	if(count<=0)
	{
		TKGS_BACKUPGRADE_PRINTF("can't find out  channel!");
		return GXCORE_ERROR;
	}

	GxBus_PmProgGetById(count,&prog);
	memcpy(&progbak, &prog, sizeof(GxBusPmDataProg));
	prog.video_type = g_AppTkgsBackUpgrade.search_pmt_body.service_type;
	if (g_AppTkgsBackUpgrade.search_pmt_body.pmt_ca_desc.desc_valid == TRUE) {
		prog.scramble_flag = GXBUS_PM_PROG_BOOL_ENABLE;
		prog.cas_id = g_AppTkgsBackUpgrade.search_pmt_body.pmt_ca_desc.cas_id;
		prog.ecm_pid_v = g_AppTkgsBackUpgrade.search_pmt_body.ecm_pid_video;
	} else {
		prog.scramble_flag = GXBUS_PM_PROG_BOOL_DISABLE;
		prog.cas_id = CAS_ID_FREE;
	}
	prog.video_pid = g_AppTkgsBackUpgrade.search_pmt_body.video_pid;
	prog.pcr_pid = g_AppTkgsBackUpgrade.search_pmt_body.pcr_pid;

	if (g_AppTkgsBackUpgrade.search_pmt_body.pmt_ttx_desc.ttx_desc_valid == TRUE) {
		prog.ttx_flag = GXBUS_PM_PROG_BOOL_ENABLE;
		if (g_AppTkgsBackUpgrade.search_pmt_body.pmt_ttx_desc.cc_desc_valid == TRUE) {
			prog.cc_flag = GXBUS_PM_PROG_BOOL_ENABLE;
		}
	} else {
		prog.ttx_flag = GXBUS_PM_PROG_BOOL_DISABLE;
		prog.cc_flag = GXBUS_PM_PROG_BOOL_DISABLE;
	}
	if (g_AppTkgsBackUpgrade.search_pmt_body.pmt_subt_desc.desc_valid == TRUE) {
		prog.subt_flag = GXBUS_PM_PROG_BOOL_ENABLE;
	} else {
		prog.subt_flag = GXBUS_PM_PROG_BOOL_DISABLE;
	}

	prog.pat_version = pat_info.pat_version;
	prog.pmt_pid = pat_info.pmt_pid;
	prog.ts_id = g_AppTkgsBackUpgrade.cur_ts_id;

	//app_log_info("=======backupgrade:pmtinfo============\n");

	if(0 != memcmp(&progbak, &prog, sizeof(GxBusPmDataProg)))
	{
	//app_log_debug("prog.video_pid=%d,pmt_pid=%d,service_id=%d,pcr_pid=%d\n",prog.video_pid,prog.pmt_pid,prog.service_id,prog.pcr_pid);
		GxMsgProperty_NodeByIdGet prog_node = {0}	;
              int video_status = 0;
		prog_node.node_type = NODE_PROG;
		prog_node.id =  prog.id;
		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&prog_node)))
		{
			GxMsgProperty_NodeModify prog_modify = {0};
			prog_modify.node_type= NODE_PROG;
			memcpy(&prog_modify.prog_data, &prog_node.prog_data, sizeof(GxBusPmDataProg));
				prog_modify.prog_data.video_pid = prog.video_pid;
				prog_modify.prog_data.pcr_pid = prog.pcr_pid;
				prog_modify.prog_data.pmt_pid = prog.pmt_pid;
				prog_modify.prog_data.pat_version = prog.pat_version;
				prog_modify.prog_data.scramble_flag = prog.scramble_flag;
				prog_modify.prog_data.cas_id = prog.cas_id;
				prog_modify.prog_data.ecm_pid_v = prog.ecm_pid_v;
				prog_modify.prog_data.ttx_flag =	prog.ttx_flag;
				prog_modify.prog_data.cc_flag = prog.cc_flag;
				prog_modify.prog_data.subt_flag = prog.subt_flag;
			    prog_modify.prog_data.ts_id = prog.ts_id;
				prog_modify.prog_data.data_plp_id      = 0xff;
				prog_modify.prog_data.common_plp_exist = 0xff;
				prog_modify.prog_data.common_plp_id    = 0xff;
				prog_modify.prog_data.common_status    = 0xff;
				prog_modify.prog_data.muti_ts_id       = 0xff;
				prog_modify.prog_data.muti_ts_exist    = 0x0;
				prog_modify.prog_data.muti_ts_code     = 0xff;
				prog_modify.prog_data.pdmx_flag        = 0x0;
				prog_modify.prog_data.pdmx_pid         = 0x1fff;
			//app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &prog_modify);
			GxBus_PmProgInfoModify(&(prog_modify.prog_data));
                if(prog.video_pid != progbak.video_pid)
		{
		//	app_log_debug("2prog.video_pid=%d,pmt_pid=%d,service_id=%d,pcr_pid=%d\n",prog.video_pid,prog.pmt_pid,prog.service_id,prog.pcr_pid);
			GxMsgProperty_NodeByPosGet node = {0};

	node.node_type = NODE_PROG;
	node.pos = g_AppPlayOps.normal_play.play_count;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
	{
	if(prog.service_id == node.prog_data.service_id)
		{
		if(app_check_av_running() == true)
			{
				 g_AppFullArb.timer_stop();//20121108
				 video_status = 1;
			}
			else
			{
				if(video_status)
				{
					video_status = 0;
					 g_AppFullArb.timer_start();//20121108
				}
			}

               app_play_current_prog(PLAY_MODE_POINT);
			}
		       }
                	}
		}
	}

	*prog_id = prog.id;
	return GXCORE_SUCCESS;
}



 status_t gx_tkgs_pmt_info_get(GxParseResult * parse_result)
	{
		uint32_t               *cas_info = NULL;
		uint32_t               *cas_info2 = NULL;
		PmtInfo                *pmt_info = NULL;
		stream_info_t          *stream_info = NULL;
		Ac3Descriptor          *ac3_descriptor = NULL;
		Eac3Descriptor         *eac3_descriptor = NULL;
		TeletextDescriptor     *ttx_descriptor = NULL;
		Teletext               *ttx_info = NULL;
		uint32_t                stream_info_count = 0;
		uint32_t                i = 0;
		uint32_t                j = 0;
		uint32_t                stream_type = 0;
		uint32_t                prog_id = 0;
		status_t                ret = GXCORE_SUCCESS;
		//GxMessage              *new_msg = NULL;
		//GxMsgProperty_NewProgGet *params = NULL;
		uint32_t video_audio = 0;
		TKGS_BACKUPGRADE_PRINTF("=======%s============\n",__FUNCTION__);

		pmt_info = (PmtInfo *) (parse_result->parsed_data);
		stream_info_count = (pmt_info->stream_count) + (pmt_info->ac3_count) +( pmt_info->eac3_count);	//????ac3?Ŀռ?
		stream_info = (stream_info_t *) (pmt_info->stream_info);

		g_AppTkgsBackUpgrade.search_stream_body =
		    (GxBusPmDataStream *) GxCore_Malloc(sizeof(GxBusPmDataStream) *
						   stream_info_count);
		memset(g_AppTkgsBackUpgrade.search_stream_body, 0, sizeof(GxBusPmDataStream) *stream_info_count);
		g_AppTkgsBackUpgrade.search_stream_body_count = 0;

		g_AppTkgsBackUpgrade.search_pmt_body.pcr_pid = pmt_info->pcr_pid;
		g_AppTkgsBackUpgrade.search_pmt_body.service_id = pmt_info->prog_num;

		if (pmt_info->ca_count != 0)	//先判断音视频是不是同密
		{
			g_AppTkgsBackUpgrade.search_pmt_body.pmt_ca_desc.desc_valid = TRUE;
			cas_info = pmt_info->ca_info;
		}
		if (pmt_info->ca_count2 != 0)	//再判断音视频是不是分开加密
		{
			g_AppTkgsBackUpgrade.search_pmt_body.pmt_ca_desc.desc_valid = TRUE;
			cas_info2 = pmt_info->ca_info2;
		}
		g_AppTkgsBackUpgrade.search_pmt_body.video_pid = 0;//避免上一张pmt的数据残留
        for (i = 0; i < (stream_info_count - (pmt_info->ac3_count)); i++)	//这里只获得pcm的信息
		{
			stream_type = stream_info[i].stream_type;
			switch (stream_type) {
			case MPEG_1_VIDEO:
			case MPEG_2_VIDEO:
				g_AppTkgsBackUpgrade.search_pmt_body.service_type = GXBUS_PM_PROG_MPEG;
				video_audio = 0;
				break;
			case H264:
				g_AppTkgsBackUpgrade.search_pmt_body.service_type = GXBUS_PM_PROG_H264;
				video_audio = 0;
				break;

			case H265:
				g_AppTkgsBackUpgrade.search_pmt_body.service_type = GXBUS_PM_PROG_H265;
				video_audio = 0;
				break;

			case AVS:
				g_AppTkgsBackUpgrade.search_pmt_body.service_type = GXBUS_PM_PROG_AVS;
				video_audio = 0;
				break;
			case MPEG_1_AUDIO:
				g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_MPEG1;
				video_audio = 1;
				break;
			case MPEG_2_AUDIO:
				g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_MPEG2;
				video_audio = 1;
				break;
			case AAC_ADTS:
				g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AAC_ADTS;
				video_audio = 1;
				break;
			case AAC_LATM:
				g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AAC_LATM;
				video_audio = 1;
				break;
			/*case PRIVATE_PES_STREAM:  //出现在pmt中一般是EAC3
				//search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_EAC3;
				//video_audio = 1;
				break;*/
			case LPCM:
				g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_LPCM;
				video_audio = 1;
				break;
			case AC3:
				g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3;
				video_audio = 1;
				break;
			/*case DTS:
				search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS;
				video_audio = 1;
				break;*/
			case DOLBY_TRUEHD:
				g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DOLBY_TRUEHD;
				video_audio = 1;
				break;
			case AC3_PLUS:
				g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3_PLUS;
				video_audio = 1;
				break;
			/*case DTS_HD:
				search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS_HD;
				video_audio = 1;
				break;
			case DTS_MA:
				search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS_MA;
				video_audio = 1;
				break;*/
			case AC3_PLUS_SEC:
				g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3_PLUS_SEC;
				video_audio = 1;
				break;
			/*case DTS_HD_SEC:
				search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS_HD_SEC;
				video_audio = 1;
				break;*/
			default:
				//search_class.search_stream_body_count = 0;
				//GxCore_Free(search_class.search_stream_body);
				//search_class.search_stream_body = NULL;
				#ifdef GX_BUS_SEARCH_DBUG
				SEARCH_BASE_PRINTF("[SEARCH]---can't support stream type!!type = %x\n",stream_type);
				#endif
				video_audio = 2;
				break;
			}
			if(video_audio == 0)
			{
				g_AppTkgsBackUpgrade.search_pmt_body.video_pid = stream_info[i].elem_pid;
				gx_search_pmt_cas_get(cas_info,
						      pmt_info->ca_count,
						      cas_info2,
						      pmt_info->ca_count2,
							  video_audio);
				j = 1;
			}
			else if(video_audio == 1)
			{
				g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].audio_pid =
				    stream_info[i].elem_pid;


				gx_search_pmt_cas_get(cas_info,
						      pmt_info->ca_count,
						      cas_info2,
						      pmt_info->ca_count2,
						      video_audio);
				memset(g_AppTkgsBackUpgrade.search_stream_body
				       [g_AppTkgsBackUpgrade.search_stream_body_count].name, 0, 4);
				if (stream_info[i].iso639_count != 0) {
					memcpy(g_AppTkgsBackUpgrade.search_stream_body
					       [g_AppTkgsBackUpgrade.search_stream_body_count].name,
					       stream_info[i].iso639_info[0].iso639, 3);
				}
				g_AppTkgsBackUpgrade.search_pmt_body.audio_count++;	//不包括ac3
				g_AppTkgsBackUpgrade.search_stream_body_count++;	//包括ac3
				j = 1;
			}
		}
		if(j == 0)
		{
			g_AppTkgsBackUpgrade.search_stream_body_count = 0;
			GxCore_Free(g_AppTkgsBackUpgrade.search_stream_body);
			g_AppTkgsBackUpgrade.search_stream_body = NULL;

			return GXCORE_ERROR;
		}

		g_AppTkgsBackUpgrade.search_pmt_body.pmt_subt_desc.desc_valid = FALSE;
		if (pmt_info->subt_count != 0)	//GxSearchPmtSubtitlingDesc等待si服务
		{
			g_AppTkgsBackUpgrade.search_pmt_body.pmt_subt_desc.desc_valid = TRUE;
		}

		g_AppTkgsBackUpgrade.search_pmt_body.pmt_ttx_desc.ttx_desc_valid = FALSE;
		g_AppTkgsBackUpgrade.search_pmt_body.pmt_ttx_desc.cc_desc_valid = FALSE;
		if (pmt_info->ttx_count != 0) {
			g_AppTkgsBackUpgrade.search_pmt_body.pmt_ttx_desc.ttx_desc_valid = TRUE;

			for (i = 0; i < pmt_info->ttx_count; i++) {
				ttx_descriptor =
				    (TeletextDescriptor *) (pmt_info->ttx_info[i]);//因为沈斌的ttx_info是一个类似数组的东西，数组的元素是每一个ttx描俗的地址

				for (j = 0; j < ttx_descriptor->ttx_num; j++) {
	                if(ttx_descriptor->ttx)
				        ttx_info = (Teletext*)((Teletext*)(ttx_descriptor->ttx) + j);
	                else
	                {
	                    app_log_error("\nSEARCH, ttx descriptor error\n");
	                    break;
	                }
					if (ttx_info->type == 0x02
						//|| ttx_info->type == 0x01   /*cc flag shenbin 0705*/
						||ttx_info->type == 0x05)	//0x02或者0x5是cc类型的ttx
					{
						g_AppTkgsBackUpgrade.search_pmt_body.pmt_ttx_desc.cc_desc_valid = TRUE;
						break;
					}
				}
				if (g_AppTkgsBackUpgrade.search_pmt_body.pmt_ttx_desc.cc_desc_valid == TRUE) {
					break;
				}
			}
		}

		g_AppTkgsBackUpgrade.search_pmt_body.pmt_ac3_desc.desc_valid = FALSE;
		if (pmt_info->ac3_count != 0)	//GxSearchPmtAc3Desc等待si服务search_stream_body_count++
		{
			g_AppTkgsBackUpgrade.search_pmt_body.pmt_ac3_desc.desc_valid = TRUE;

			for (i = 0; i < pmt_info->ac3_count; i++) {
				ac3_descriptor =
				    (Ac3Descriptor *) (pmt_info->ac3_info[i]);
				g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3;	//ac3
				g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].audio_pid =
				    ac3_descriptor->elem_pid;
				memset(g_AppTkgsBackUpgrade.search_stream_body
				       [g_AppTkgsBackUpgrade.search_stream_body_count].name, 0, 4);
				g_AppTkgsBackUpgrade.search_stream_body_count++;
			}
		}
		if (pmt_info->eac3_count != 0)	//GxSearchPmtAc3Desc等待si服务search_stream_body_count++
		{
			g_AppTkgsBackUpgrade.search_pmt_body.pmt_ac3_desc.desc_valid = TRUE;

			for (i = 0; i < pmt_info->eac3_count; i++) {
				eac3_descriptor =
				    (Eac3Descriptor *) (pmt_info->eac3_info[i]);
				g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_EAC3;
				g_AppTkgsBackUpgrade.search_stream_body[g_AppTkgsBackUpgrade.search_stream_body_count].audio_pid =
				    eac3_descriptor->elem_pid;
				memset(g_AppTkgsBackUpgrade.search_stream_body
				       [g_AppTkgsBackUpgrade.search_stream_body_count].name, 0, 4);
				g_AppTkgsBackUpgrade.search_stream_body_count++;
			}
		}
		//if()GxSearchPmtServiceMoveDesc等待si服务
        ret = gx_tkgs_backupgrade_prog_info_modify(&prog_id);

		memset(&(g_AppTkgsBackUpgrade.search_pmt_body), 0, sizeof(GxSearchPmtBody));


		g_AppTkgsBackUpgrade.search_stream_body_count = 0;
		if (g_AppTkgsBackUpgrade.search_stream_body != NULL)	//释放stream body 为下一个节目做准备
		{
			GxCore_Free(g_AppTkgsBackUpgrade.search_stream_body);
			g_AppTkgsBackUpgrade.search_stream_body = NULL;
		}
		return ret;
}





#if 0
 status_t gx_tkgs_pmt_info_get(GxParseResult * parse_result)
{



	uint8_t* p1 = NULL;
	uint8_t* p2 = NULL;
	int32_t section_length = 0;
	int32_t loop1Len = 0;
	int32_t loop2Len = 0;
	int32_t loop2DescLen = 0;
	int32_t tempLen = 0;
	uint8_t* section_data= NULL;
	uint8_t* temp_p = NULL;

	if (parse_result == NULL)
	{
		return GXCORE_ERROR;
	}

	section_data = parse_result->parsed_data;
	if(GET_TABLE_ID(section_data) != PMT_TID)
	{
		return GXCORE_ERROR;
	}

	section_length = TODATA12(section_data[1], section_data[2]);

	g_AppTkgsBackUpgrade.search_pmt_body.service_id = TODATA16(section_data[3],section_data[4]);
	g_AppTkgsBackUpgrade.search_pmt_body.pcr_pid =  TODATA13 (section_data[8], section_data[9]);
	loop1Len = TODATA12 (section_data[10] , section_data[11]);
	p1 = section_data;
	p1+= 12;
	loop2Len = section_length - 9 - loop1Len;
	p2 = parse_result->parsed_data+ 12 + loop1Len;
	while(loop1Len >= 7)
    	{

        	if ((p1[0] == TKGS_COMPONENT_TAG) && ((p1[6] & 0x80) == 0x80))
        	{
        		if(0 == strncmp((const char*)(p1+2), "TKGS", 4))
        		{
				g_AppTkgsBackUpgrade.tkgs_component_flag = 1;
				break;
        		}
		}
		loop1Len -= (2+p1[1]);
		p1 += (2+p1[1]);
	}

	if(g_AppTkgsBackUpgrade.tkgs_component_flag == 0)
	{
		while(loop2Len > 5)
	    	{
			loop2DescLen = TODATA12(p2[3], p2[4]);
			tempLen = loop2DescLen;
			if(tempLen >7)
			{
				temp_p = p2;
				while(tempLen>0)
				{
			        	if ((temp_p[0] == TKGS_COMPONENT_TAG) && ((temp_p[6] & 0x80) == 0x80))
			        	{
			        		if(0 == strncmp((const char*)(temp_p+2), "TKGS", 4))
			        		{
							g_AppTkgsBackUpgrade.tkgs_component_flag = 1;
							break;
			        		}
					}

					tempLen -= 2+temp_p[1];
					temp_p += (2+temp_p[1]);
				}

			}

			if(g_AppTkgsBackUpgrade.tkgs_component_flag > 0)
			{
				break;
			}
			loop2Len -= (5+loop2DescLen);
			p2 += (5+loop2DescLen);

		}
	}
	return GXCORE_SUCCESS;
}
#endif




static status_t gx_tkgs_pmt_next_filte_start(GxParseResult * parse_result)
{
	GxMessage              *new_msg = NULL;
	GxSubTableDetail      **params = NULL;
	GxMsgProperty_SiStart  *params_start = NULL;
	GxSubTableDetail        subtable = { 0 };
	uint16_t                prog_number = 0;//相当于service id
	int16_t                 subtable_id = 0;
	uint32_t                i = 0;
	TKGS_BACKUPGRADE_PRINTF("func=%s ,line=%d\n",__FUNCTION__,__LINE__);

	subtable_id = subtable.si_subtable_id = parse_result->si_subtable_id;
    /*修改前先获得原始参数 */
    new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_GET);
	params = GxBus_GetMsgPropertyPtr(new_msg, GxSubTableDetail *);
	*params = &subtable;
	GxBus_MessageSendWait(new_msg);
	GxBus_MessageFree(new_msg);	//同步消息 使用完后要free,因为接收的地方是NOT_FREE

    /*修改参数 并且发送modify消息 */
    subtable.si_filter.pid = g_AppTkgsBackUpgrade.search_pat_body[g_AppTkgsBackUpgrade.search_pmt_count].pmt_pid;
	prog_number = g_AppTkgsBackUpgrade.search_pat_body[g_AppTkgsBackUpgrade.search_pmt_count].prog_number;

	subtable.si_filter.match[3] = ((prog_number >> 8) & 0xff);

	subtable.si_filter.mask[3] = 0xff;

	subtable.si_filter.match[4] = (prog_number & 0xff);

	subtable.si_filter.mask[4] = 0xff;

	g_AppTkgsBackUpgrade.search_pmt_count++;

	new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_MODIFY);
	params = GxBus_GetMsgPropertyPtr(new_msg, GxSubTableDetail *);

	*params = &subtable;

	GxBus_MessageSendWait(new_msg);
	GxBus_MessageFree(new_msg);	//同步消息 使用完后要free,因为接收的地方是NOT_FREE

	for (i = 0; i < FILTER_MAX; i++) {
		if (g_AppTkgsBackUpgrade.search_pmt_subtable_id[i].subtable_id == subtable_id) {
			new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_START);
			params_start =
			    GxBus_GetMsgPropertyPtr(new_msg,
						     GxMsgProperty_SiStart);
			*params_start = subtable_id;
			GxBus_MessageSend(new_msg);
			return GXCORE_SUCCESS;
		}

	}

	return GXCORE_ERROR;
}

void gx_tkgs_backupgrade_variable_exit(void)
{
	uint8_t                 i = 0;
	uint32_t search_status = 0;
	g_AppTkgsBackUpgrade.ts_src = 0;
	g_AppTkgsBackUpgrade.search_demux_id = 0;
	//g_AppTkgsBackUpgrade.tkgs_component_flag = 0;
	if (g_AppTkgsBackUpgrade.search_status != TKGS_BACKUPGRADE_STOP)
	{
		search_status = g_AppTkgsBackUpgrade.search_status & TKGS_BACKUPGRADE_PAT_STOPING;
		if (g_AppTkgsBackUpgrade.search_pat_subtable_id.subtable_id != -1
			&&search_status == TKGS_BACKUPGRADE_STATUS_FALSE)
		{
			gx_tkgs_pat_filter_release();
		}

		TKGS_BACKUPGRADE_PRINTF("func=%s ,line=%d\n",__FUNCTION__,__LINE__);

		search_status = g_AppTkgsBackUpgrade.search_status & TKGS_BACKUPGRADE_PMT_STOPING;
		for (i = 0; i < FILTER_MAX; i++)
		{
			if (g_AppTkgsBackUpgrade.search_pmt_subtable_id[i].subtable_id != -1
				&&search_status == TKGS_BACKUPGRADE_STATUS_FALSE)
			{
				gx_search_pmt_filter_release(&(g_AppTkgsBackUpgrade.search_pmt_subtable_id[i].subtable_id));
			}

		}
		g_AppTkgsBackUpgrade.search_status |= TKGS_BACKUPGRADE_PMT_STOPING;

		g_AppTkgsBackUpgrade.search_pmt_filter_count = 0;
		g_AppTkgsBackUpgrade.search_pmt_count = 0;
		g_AppTkgsBackUpgrade.search_pmt_finish_count = 0;

		if (g_AppTkgsBackUpgrade.search_pat_body != NULL) {
			GxCore_Free(g_AppTkgsBackUpgrade.search_pat_body);

			g_AppTkgsBackUpgrade.search_pat_body = NULL;
		}
		g_AppTkgsBackUpgrade.search_pat_body_count = 0;

		g_AppTkgsBackUpgrade.search_pat_finish_flag = 0;

		memset(&(g_AppTkgsBackUpgrade.search_pmt_body), 0, sizeof(GxSearchPmtBody));

		g_AppTkgsBackUpgrade.search_stream_body_count = 0;

		if (g_AppTkgsBackUpgrade.search_stream_body!= NULL) {
			GxCore_Free(g_AppTkgsBackUpgrade.search_stream_body);
			g_AppTkgsBackUpgrade.search_stream_body = NULL;
		}
	}

	//g_AppTkgsBackUpgrade.tp_id = 0;
	g_AppTkgsBackUpgrade.search_status = TKGS_BACKUPGRADE_STOP;

	return;
}




static status_t gx_tkgs_pmt_subtable_ok(GxParseResult * parse_result)
{
	uint32_t i = 0;
	status_t ret = 0;
	//int32_t count = 0;
	TKGS_BACKUPGRADE_PRINTF("func=%s ,line=%d\n",__FUNCTION__,__LINE__);

	for (i = 0; i < FILTER_MAX; i++)
	{

		if (parse_result->si_subtable_id == g_AppTkgsBackUpgrade.search_pmt_subtable_id[i].subtable_id)
		{
			TKGS_BACKUPGRADE_PRINTF("suttable_id = %d,pmt_pid=%d\n",parse_result->si_subtable_id,g_AppTkgsBackUpgrade.search_pat_body[i].pmt_pid);
			ret = gx_tkgs_pmt_info_get(parse_result);	//要对返回值做严格判断
			if (ret == GXCORE_SUCCESS )
			{
				#if 0
				app_log_info("=======backupgrade:pmtinfo============\n");
				app_log_info("pmt_pid=%d,service_id=%d,pcr_pid=%d\n",g_AppTkgsBackUpgrade.search_pat_body[i].pmt_pid,g_AppTkgsBackUpgrade.tp_id,g_AppTkgsBackUpgrade.search_pmt_body.service_id,g_AppTkgsBackUpgrade.search_pmt_body.pcr_pid);

				//update PMT_PID;
				count = GxBus_PmProgExistChek(g_AppTkgsBackUpgrade.tp_id,
					0,
					g_AppTkgsBackUpgrade.search_pmt_body.service_id,
					0,
					g_AppTkgsBackUpgrade.ts_src);

				if(count >0)
				{
					GxBusPmDataProg prog;
					GxBus_PmProgGetById(count,&prog);

					prog.pmt_pid = g_AppTkgsBackUpgrade.search_pat_body[i].pmt_pid;
					GxBus_PmProgInfoModify(&prog);
				}
				#endif
			}
			g_AppTkgsBackUpgrade.search_pmt_finish_count++;

			GxBus_SiParserBufFree(parse_result->parsed_data);

			if (g_AppTkgsBackUpgrade.search_pmt_count < g_AppTkgsBackUpgrade.search_pat_body_count)	//????pmtû????
			{
				TKGS_BACKUPGRADE_PRINTF("func=%s ,line=%d\n",__FUNCTION__,__LINE__);
				gx_tkgs_pmt_next_filte_start(parse_result);
			}
			else
			{
				TKGS_BACKUPGRADE_PRINTF("func=%s ,line=%d,finishCount=%d,search_pat_body_count=%d\n",__FUNCTION__,__LINE__,g_AppTkgsBackUpgrade.search_pmt_finish_count,g_AppTkgsBackUpgrade.search_pat_body_count);
                /*全部pmt解析完成 */
                if (g_AppTkgsBackUpgrade.search_pmt_finish_count == g_AppTkgsBackUpgrade.search_pat_body_count)
				{
					gx_tkgs_backupgrade_variable_exit();
					TKGS_BACKUPGRADE_PRINTF("func=%s ,line=%d\n",__FUNCTION__,__LINE__);
					gx_tkgs_backupgrade_servicestart();
				}
			}

			return GXCORE_SUCCESS;
		}

	}

	return GXCORE_ERROR;
}

static status_t gx_tkgs_timeout_check(GxParseResult * parse_result)
{
	static uint8_t          pat_timeout = 0;
	uint32_t                i = 0;

	if(parse_result->table_id != PMT_TID)
	{
		return GXCORE_SUCCESS;
	}
	if((g_AppTkgsBackUpgrade.search_status &TKGS_BACKUPGRADE_STOP) > 0)
	{
		return GXCORE_SUCCESS;
	}
	if (parse_result->si_subtable_id == g_AppTkgsBackUpgrade.search_pat_subtable_id.subtable_id
		&&parse_result->request_id == g_AppTkgsBackUpgrade.search_pat_subtable_id.request_id)
	{
		pat_timeout = 1;
		g_AppPmt.ts_src = g_AppPlayOps.normal_play.ts_src;
		g_AppPmt.start(&g_AppPmt);
	}
	else
	{

		for (i = 0; i < FILTER_MAX; i++)
		{
			if (parse_result->si_subtable_id == g_AppTkgsBackUpgrade.search_pmt_subtable_id[i].subtable_id
				&&parse_result->request_id == g_AppTkgsBackUpgrade.search_pmt_subtable_id[i].request_id)
			{
				g_AppTkgsBackUpgrade.search_pmt_finish_count++;
				if (g_AppTkgsBackUpgrade.search_pmt_count < g_AppTkgsBackUpgrade.search_pat_body_count)	//还有pmt没解析
				{
					gx_tkgs_pmt_next_filte_start(parse_result);
				}

				if ((g_AppTkgsBackUpgrade.search_pmt_finish_count == g_AppTkgsBackUpgrade.search_pat_body_count)
					&&(g_AppTkgsBackUpgrade.search_pmt_finish_count != 0))
				{
					gx_tkgs_backupgrade_variable_exit();
					TKGS_BACKUPGRADE_PRINTF("func=%s ,line=%d\n",__FUNCTION__,__LINE__);
					gx_tkgs_backupgrade_servicestart();
					pat_timeout = 0;
					return GXCORE_SUCCESS;
				}
				break;
			}
		}

	}

	if (pat_timeout == 1)
	{
		pat_timeout = 0;
		return GXCORE_SUCCESS;
	}
	return GXCORE_ERROR;
}




static status_t gx_tkgs_backupgrade_section_handle(GxParseResult * parse_result)
{
    status_t ret = GXCORE_SUCCESS;
    switch (parse_result->table_id)
    {
        case PAT_TID:
            ret = gx_tkgs_pat_subtable_ok(parse_result);
            break;

        case PMT_TID:
            ret = gx_tkgs_pmt_subtable_ok(parse_result);
            break;

        default:
            break;
    }
    return ret;
}

static status_t gx_tkgs_backupgrade_stop(void)
{
    gx_tkgs_backupgrade_variable_exit();
    return GXCORE_SUCCESS;
}

void gx_tkgs_backupgrade_first_init(void)
{
	int i;

	g_AppTkgsBackUpgrade.search_pat_subtable_id.subtable_id = -1;
	for (i = 0; i < FILTER_MAX; i++)
	{
		g_AppTkgsBackUpgrade.search_pmt_subtable_id[i].subtable_id = -1;
	}
}

AppTkgsBackUpgrade g_AppTkgsBackUpgrade =
{
    .search_status = TKGS_BACKUPGRADE_STOP,
    .init        = gx_tkgs_backupgrade_init,
    .start      = gx_tkgs_backupgrade_create,
    .stop       = gx_tkgs_backupgrade_stop,
    .get        = gx_tkgs_backupgrade_section_handle,
    .timeout  = gx_tkgs_timeout_check,
    .cb         = gx_tkgs_pmt_section_handle
};

#endif

