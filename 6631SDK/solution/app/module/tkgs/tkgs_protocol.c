#include "app_module.h"
#include "app_msg.h"
#include "app_send_msg.h"
#include "app_str_id.h"
#include "app_default_params.h"
#include "tkgs_protocol.h"
#include "gui_core.h"
#if TKGS_SUPPORT
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

#define GET_DEC32(bcd)	(((bcd>>28)&0x0f)*10000000\
						+ ((bcd>>24)&0x0f)*1000000\
						+ ((bcd>>20)&0x0f)*100000\
						+ ((bcd>>16)&0x0f)*10000\
						+ ((bcd>>12)&0x0f)*1000\
						+ ((bcd>>8)&0x0f)*100\
						+ ((bcd>>4)&0x0f)*10\
						+ (bcd&0x0f))
#define GET_DEC16(bcd)	(((bcd>>12)&0x0f)*1000\
						+ ((bcd>>8)&0x0f)*100\
						+ ((bcd>>4)&0x0f)*10\
						+ (bcd&0x0f))
typedef struct{
	uint32_t id;
	uint16_t orbital_position;
	GxBusPmDataSatLongitudeDirect	west_east_flag;
	uint8_t	name[MAX_SAT_NAME];
}SatParam_s;

//extern int gxgdi_iconv_turk(const unsigned char *from_str, unsigned char **to_str, unsigned int from_size, unsigned int *to_size);
extern int gxgdi_iconv(const unsigned char *from_str,unsigned char **to_str,unsigned int from_size,unsigned int *to_size,unsigned char *iso_str);

extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
extern char *app_get_short_lang_str(uint32_t lang);
static bool _tkgs_lcn_conflict_check(GxBusPmDataProgType servType, uint16_t lcn,GxTkgsDbLcnList lcnList);
static status_t _tkgs_get_lcn_by_locator(uint16_t	locator, uint16_t	*lcn, GxTkgsClass *tkgs_class);
static status_t _tkgs_save_category_name(uint16_t category_id, uint8_t *category_name);
static status_t _tkgs_save_parsed_location(uint16_t tp_id, uint16_t pid);
static status_t _tkgs_add_sat(SatParam_s *pSat);
static uint32_t  check_ca_system(uint16_t ca_system_id,uint16_t ele_pid,uint16_t ecm_pid);
static status_t gx_search_pmt_cas_get(uint32_t * cas1,
			       uint32_t cas1_count,
			       uint32_t * cas2,
			       uint32_t cas2_count, uint32_t stream_type, GxTkgsClass *tkgs_class);
 static void  gx_search_prog_check_audio_lang(uint16_t* audio_pid,
											  GxBusPmDataProgAudioType* type,
											  uint16_t* ecm_pid, GxTkgsClass *tkgs_class);
static int32_t gx_search_sdt_find(GxSearchSdtBody * sdt, GxTkgsClass *tkgs_class);
static status_t gx_search_prog_info_add(uint32_t * prog_id, GxTkgsClass *tkgs_class);
static status_t pmt_info_get(uint8_t*      parsed_data, GxTkgsClass *tkgs_class);
static status_t sdt_info_get(uint8_t*      parsed_data, GxTkgsClass *tkgs_class);
static status_t _tkgs_tkgs_info_parse(uint8_t *pBuffIn, uint32_t length, GxTkgsClass *tkgs_class);
static status_t _tkgs_broadcast_source_descriptor_loop_parse(uint8_t *pBuffIn, uint32_t length, GxTkgsClass *tkgs_class);
static status_t _tkgs_transport_stream_loop_parse(uint8_t *pBuffIn, uint32_t length, GxTkgsClass *tkgs_class);
static status_t satellite_delivery_system_descriptor_parse(uint8_t tag, uint8_t *p_data, GxTkgsClass *tkgs_class);
static status_t _tkgs_sdt_desc_loop_parse(uint8_t *pBuffIn,uint32_t len, uint16_t	service_id, GxTkgsClass *tkgs_class);
static status_t _tkgs_sdt_extended_parse(uint8_t *pBuffIn, uint16_t length, GxTkgsClass *tkgs_class);
static status_t _tkgs_transport_stream_descriptor_loop_parse(uint8_t *pBuffIn, uint32_t length, GxTkgsClass *tkgs_class);
static status_t _tkgs_service_description_section_parse(uint8_t *pBuffIn, uint16_t *secLength, GxTkgsClass *tkgs_class);
static status_t _tkgs_program_loop_parse(uint8_t *pBuffIn, uint32_t length, GxTkgsClass *tkgs_class);
static status_t _tkgs_broadcast_sourc_parse(uint8_t *pBuffIn, uint32_t length, GxTkgsClass *tkgs_class);
static status_t _tkgs_service_list_descriptor_parse(uint8_t tag, uint8_t *p_data, GxTkgsPrefer *list);
static status_t _tkgs_service_list_parse(uint8_t *pBuffIn, uint32_t length, GxTkgsClass *tkgs_class);
static status_t _tkgs_category_parse(uint8_t *pBuffIn, uint32_t length);
static status_t _tkgs_channel_blocking_parse(GxTkgsBlocking *pBlocking, uint8_t* src, uint32_t len);
static status_t _tkgs_channel_blocking_data_process(uint8_t *pBuffIn, uint32_t length);

/* Private functions ------------------------------------------------------ */

void test_fun_print_data(uint16_t locator_id, uint8_t *section_data,uint32_t len)
{
	int i;
	if ((section_data == NULL) || (len==0))
	{
		app_log_error("====%s, %d, err\n", __FUNCTION__, __LINE__);
	}

	app_log_flow("*******print data start, locator_id:%d****\n", locator_id);
	for(i=0; i<len; i++)
	{
		app_log_flow(" %x", section_data[i]);
	}
	app_log_flow("\n*******print data end****\n");
}

/**
 * @brief ?ж?LCN??????ǰ?Ľ?Ŀ?б????Ƿ??г?ͻ
 * @param
 * @Return, true: ?г?ͻ?? false:?޳?ͻ
 */
static bool _tkgs_lcn_conflict_check(GxBusPmDataProgType servType, uint16_t lcn,GxTkgsDbLcnList lcnList)
{
	uint32_t i;
	uint16_t *list = NULL;
	uint16_t num = 0;

	list = (servType == GXBUS_PM_PROG_TV) ? lcnList.tv_lcn_list : lcnList.radio_lcn_list;
	num = (servType == GXBUS_PM_PROG_TV) ? lcnList.tv_num : lcnList.radio_num;

	for (i=0; i<num; i++)
	{
		if (lcn == list[i])
		{
			return true;
		}
	}

	return false;
}

/**
 * @brief ????locator id??ȡ?ڽ?Ŀ?б??е?LCN
 * @param
 * @Return
 */
static status_t _tkgs_get_lcn_by_locator(uint16_t 	locator, uint16_t	*lcn, GxTkgsClass *tkgs_class)
{
	int i = 0;

	if ((NULL == lcn) || (NULL == tkgs_class))
	{
		TKGS_ERRO_PRINTF("===%s, %d, parm err\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}

	for (i=0; i<tkgs_class->cur_prefer_list.serviceNum; i++)
	{
		if (locator == tkgs_class->cur_prefer_list.list[i].locator)
		{
			*lcn = tkgs_class->cur_prefer_list.list[i].lcn;
			break;
		}
	}
	if (i == tkgs_class->cur_prefer_list.serviceNum)
	{
		*lcn = TKGS_INVALID_LCN;//0XFFFF
		return GXCORE_ERROR;
	}

	return GXCORE_SUCCESS;
}

/**
 * @brief ????????????????��??location
 * @param
 * @Return
 */
static status_t _tkgs_save_category_name(uint16_t category_id, uint8_t *category_name)
{
	GxMsgProperty_NodeByPosGet node = {0};
	GxMsgProperty_NodeModify node_modify = {0};

	if ((NULL == category_name) || (strlen((const char *)category_name)==0))
	{
		TKGS_ERRO_PRINTF("===%s, %d, parm err\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}

	node.node_type = NODE_FAV_GROUP;
	node.pos = category_id/16;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

	node_modify.node_type = NODE_FAV_GROUP;
	node_modify.fav_item = node.fav_item;
	strncpy((char*)node_modify.fav_item.fav_name,
				(const char*)category_name, MAX_FAV_GROUP_NAME-1);
	app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
	return GXCORE_SUCCESS;
}

/**
 * @brief ????????????????��??location
 * @param
 * @Return
 */
static status_t _tkgs_save_parsed_location(uint16_t tp_id, uint16_t pid)
{
	GxMsgProperty_NodeByIdGet tpNode;
	GxTkgsLocationParm location = {0};

	//get tp node
	tpNode.node_type = NODE_TP;
	tpNode.id = tp_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tpNode);

	location.sat_id = tpNode.tp_data.sat_id;
	location.search_demux_id = 0;
	//location.search_diseqc = gx_tkgs_set_disqec_callback;
	location.frequency = tpNode.tp_data.frequency;
	location.symbol_rate = tpNode.tp_data.tp_s.symbol_rate;
	location.polar = tpNode.tp_data.tp_s.polar;
	location.pid = pid;
	return gx_TkgsSaveLocation(&location);
}

/**
 * @brief ????????
 * @param
 * @Return
 */
static status_t _tkgs_add_sat(SatParam_s *pSat)
{
	GxMsgProperty_NodeNumGet node_num = {0};
	GxMsgProperty_NodeByPosGet node;
	GxMsgProperty_NodeAdd NodeAdd;
	uint32_t i = 0;

	if (NULL == pSat)
	{
		TKGS_ERRO_PRINTF("===%s, %d, parm err\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}

	node_num.node_type = NODE_SAT;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);

	for (i=0; i<node_num.node_num; i++)
	{
		node.node_type = NODE_SAT;
		node.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
		if ((node.sat_data.sat_s.longitude == pSat->orbital_position)
			&& (node.sat_data.sat_s.longitude_direct == pSat->west_east_flag))
		{
			pSat->id = node.sat_data.id;
			return GXCORE_SUCCESS;
		}
	}

	memset(&NodeAdd, 0, sizeof(GxMsgProperty_NodeAdd));
	NodeAdd.node_type = NODE_SAT;
	NodeAdd.sat_data.sat_s.diseqc12_pos = DISEQ12_USELESS_ID;
	NodeAdd.sat_data.type = GXBUS_PM_SAT_S;
	NodeAdd.sat_data.tuner = 0;
	NodeAdd.sat_data.sat_s.lnb1 = 9750;
	NodeAdd.sat_data.sat_s.lnb2 = 10600;
	NodeAdd.sat_data.sat_s.lnb_power = GXBUS_PM_SAT_LNB_POWER_ON;
	NodeAdd.sat_data.sat_s.switch_22K = GXBUS_PM_SAT_22K_OFF;
	NodeAdd.sat_data.sat_s.diseqc11 = 0;
	NodeAdd.sat_data.sat_s.diseqc_version = GXBUS_PM_SAT_DISEQC_1_0;
	NodeAdd.sat_data.sat_s.diseqc10 = GXBUS_PM_SAT_DISEQC_OFF;
	NodeAdd.sat_data.sat_s.switch_12V = GXBUS_PM_SAT_12V_OFF;
	NodeAdd.sat_data.sat_s.longitude_direct = pSat->west_east_flag;
	NodeAdd.sat_data.sat_s.reserved = 0;
	NodeAdd.sat_data.sat_s.longitude = pSat->orbital_position;
	memset((char *)NodeAdd.sat_data.sat_s.sat_name, 0, MAX_SAT_NAME);
	memcpy((char *)NodeAdd.sat_data.sat_s.sat_name, pSat->name, MAX_SAT_NAME -1);

	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_ADD, &NodeAdd))
	{
		pSat->id = NodeAdd.sat_data.id;
	}

	return GXCORE_SUCCESS;

}

/*????caϵͳ?Ƿ???ȷ ????0????ca system???? ????1????ca system??ȷ*/
//typedef uint32_t (*GxSearchCheckCa)(uint16_t ca_system_id,uint16_t ele_pid,uint16_t ecm_pid);

static uint32_t  check_ca_system(uint16_t ca_system_id,uint16_t ele_pid,uint16_t ecm_pid)
{
    return 1;
}
static GxSearchCheckCa gx_search_check_ca = check_ca_system;

static status_t gx_search_pmt_cas_get(uint32_t * cas1,
			       uint32_t cas1_count,
			       uint32_t * cas2,
			       uint32_t cas2_count, uint32_t stream_type, GxTkgsClass *tkgs_class)
{
	uint32_t                i = 0;
	CaDescriptor           *cas_decriptor = NULL;
	switch (stream_type) {
		/*	case MPEG_1_VIDEO:
			case MPEG_2_VIDEO:
			case AVS:
			case H264:*/
	case 0://??Ƶ
		if (cas2 != NULL)	//?Էֿ?????????
		{
			cas_decriptor = (CaDescriptor *) (cas2[0]);
			tkgs_class->search_pmt_body.ecm_pid_video = cas_decriptor->ca_pid;
			tkgs_class->search_pmt_body.pmt_ca_desc.cas_id = CAS_ID_UNKNOW;
			for (i = 0; i < cas2_count; i++) {
				cas_decriptor = (CaDescriptor *) (cas2[i]);
				if (cas_decriptor->elem_pid ==
				    tkgs_class->search_pmt_body.video_pid) {
					if(gx_search_check_ca(cas_decriptor->ca_system_id,tkgs_class->search_pmt_body.video_pid,cas_decriptor->ca_pid))
					{
						tkgs_class->search_pmt_body.ecm_pid_video =
						    cas_decriptor->ca_pid;
						tkgs_class->search_pmt_body.pmt_ca_desc.cas_id =
						    cas_decriptor->ca_system_id;
						break;
					}
				}

			}
		}
		if (i == cas2_count && cas1 != NULL)	//?ֿ?????û?ҵ?,????ͬ????
		{
			cas_decriptor = (CaDescriptor *) (cas1[0]);
			tkgs_class->search_pmt_body.ecm_pid_video = cas_decriptor->ca_pid;
			tkgs_class->search_pmt_body.pmt_ca_desc.cas_id = CAS_ID_UNKNOW;
			for (i = 0; i < cas1_count; i++) {
				cas_decriptor = (CaDescriptor *) (cas1[i]);
				//if (cas_decriptor->elem_pid ==
				 //  tkgs_class->search_pmt_body.video_pid)
				   {
					if(gx_search_check_ca(cas_decriptor->ca_system_id,tkgs_class->search_pmt_body.video_pid,cas_decriptor->ca_pid))
					{
						tkgs_class->search_pmt_body.ecm_pid_video =
						    cas_decriptor->ca_pid;
						tkgs_class->search_pmt_body.pmt_ca_desc.cas_id =
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
	case 1://??Ƶ
		if (cas2 != NULL)	//?Էֿ?????????
		{
			cas_decriptor = (CaDescriptor *) (cas2[0]);
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].ecm_pid =
			    cas_decriptor->ca_pid;

			for (i = 0; i < cas2_count; i++) {
				cas_decriptor = (CaDescriptor *) (cas2[i]);
				if (cas_decriptor->elem_pid ==
				    tkgs_class->search_stream_body
				    [tkgs_class->search_stream_body_count].audio_pid) {
					if(gx_search_check_ca(cas_decriptor->ca_system_id,
                                            tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_pid,
                                            cas_decriptor->ca_pid))
					{
						tkgs_class->search_stream_body
						    [tkgs_class->search_stream_body_count].ecm_pid =
						    cas_decriptor->ca_pid;

						break;
					}
				}

			}
		}
		if (i == cas2_count && cas1 != NULL)	//?ֿ?????û?ҵ?,????ͬ????
		{
			cas_decriptor = (CaDescriptor *) (cas1[0]);
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].ecm_pid =
			    cas_decriptor->ca_pid;

			for (i = 0; i < cas1_count; i++) {
				cas_decriptor = (CaDescriptor *) (cas1[i]);
				//if (cas_decriptor->elem_pid ==
				//    tkgs_class->search_stream_body
				 //   [tkgs_class->search_stream_body_count].audio_pid)
				   {
					if(gx_search_check_ca(cas_decriptor->ca_system_id,
                                            tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_pid,
                                            cas_decriptor->ca_pid))
					{
						tkgs_class->search_stream_body
						    [tkgs_class->search_stream_body_count].ecm_pid =
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


/**
  * @brief
  * @param
  * @Return
  */
 static void  gx_search_prog_check_audio_lang(uint16_t* audio_pid,
											  GxBusPmDataProgAudioType* type,
											  uint16_t* ecm_pid, GxTkgsClass *tkgs_class)
{
	 uint32_t i = 0;
	 uint32_t j = 0;
	 int8_t lang_buff[128];
	 int8_t lang[3][4];
	 int8_t flag = -1;//-1???���û?и?ֵ??0????û???ҵ????????ԣ?1?????ҵ??????????ԣ?2???????ҵ??˵ڶ???????
	 int8_t* p = 0;
	 uint8_t count = 0;

	 memset(lang,0,3*4);
	if(NULL ==	GxBus_ConfigGet(GXBUS_SEARCH_LANG_PRIORITY,(char*)lang_buff,128,GXBUS_SEARCH_LANG_DEFAULT))
	{
		strcpy((char*)lang_buff,GXBUS_SEARCH_LANG_DEFAULT);
	}
	 //lang_buff = "eng_chn"?ȵȱ?ʾһ??????ʦ???????ҵ?iso9660????????
	 //????֧??��??????????
	 for(p=lang_buff; ;)
	 {
		 if(*p!='\0'&&count!=2 )
		 {
			 memcpy(lang[count],p,3);
			 count++;
			 p+=3;
		 }
		 if(*p!='\0'&&count!=2 )
		 {
			 p++;
		 }
		 else
		 {
			 break;
		 }
	 }
	 if(tkgs_class->search_stream_body_count != 0)
	 {
		 *audio_pid = tkgs_class->search_stream_body[0].audio_pid;
		 *type = tkgs_class->search_stream_body[0].audio_type;
		 *ecm_pid = tkgs_class->search_stream_body[0].ecm_pid;
	 }
	 for(i=0; i<tkgs_class->search_stream_body_count; i++)
	 {
		 if(tkgs_class->search_stream_body[i].audio_type != GXBUS_PM_AUDIO_AC3)
		 {
			 for(j=0; j<sizeof(tkgs_class->search_stream_body[i].name); j++)
			 {
				 tkgs_class->search_stream_body[i].name[j] = tolower(tkgs_class->search_stream_body[i].name[j]);
			 }
			 if(-1 == flag)
			 {
				 *audio_pid = tkgs_class->search_stream_body[0].audio_pid;
				 *type = tkgs_class->search_stream_body[0].audio_type;
				 *ecm_pid = tkgs_class->search_stream_body[0].ecm_pid;
				 flag = 0;
			 }

			 if(0 == strcmp((const char*)(tkgs_class->search_stream_body[i].name),(const char*)(lang[0])))
			 {
				 *audio_pid = tkgs_class->search_stream_body[i].audio_pid;
				 *type = tkgs_class->search_stream_body[i].audio_type;
				 *ecm_pid = tkgs_class->search_stream_body[i].ecm_pid;
				 flag = 1;
				 break;
			 }
			 else if(0 == strcmp((const char*)(tkgs_class->search_stream_body[i].name),(const char*)(lang[1])))
			 {
				 *audio_pid = tkgs_class->search_stream_body[i].audio_pid;
				 *type = tkgs_class->search_stream_body[i].audio_type;
				 *ecm_pid = tkgs_class->search_stream_body[i].ecm_pid;
				 flag = 2;
			 }
		 }
	 }
	 return;
 }


 /**
  * @brief ????pmt?е?service_id???Ҷ?Ӧ??SDT????
  * @param
  * @Return
  */
static int32_t gx_search_sdt_find(GxSearchSdtBody * sdt, GxTkgsClass *tkgs_class)
{
	uint32_t				 i = 0;
	int8_t					  buff[15]={0,};
	static uint32_t 				   y = 0;

	for (i = 0; i < tkgs_class->search_sdt_body_count; i++) {
		if (tkgs_class->search_pmt_body.service_id == tkgs_class->search_sdt_body[i].service_id) {
			memcpy(sdt, &(tkgs_class->search_sdt_body)[i],
				   sizeof(GxSearchSdtBody));
		}
	}

	if (sdt->service_desc.service_type == SI_DATA_BROADCAST_SERVICE)//tkgs ????Ƶ??
	{
		if (tkgs_class->search_pmt_body.video_pid == 0
				|| tkgs_class->search_pmt_body.video_pid == 0x1fff)
		{
			if (tkgs_class->search_pmt_body.audio_count != 0)
			{
				sdt->service_desc.service_type = SI_AUDIO_SERVICE;
			}
		}
		else
		{
			sdt->service_desc.service_type = SI_VIDEO_SERVICE;
		}
		return i;
	}

	/*??ͨ??sdt??type?ж???radio????tv??Ϊ??׼ */
	if( sdt->service_desc.service_type != SI_NVOD_REFERENCE_SERVICE
		&& sdt->service_desc.service_type != SI_NVOD_TIMESHIFT_SERVICE )
	{
		if (tkgs_class->search_pmt_body.video_pid == 0
				|| tkgs_class->search_pmt_body.video_pid == 0x1fff)
		{
			if (tkgs_class->search_pmt_body.audio_count == 0)
			{
				TKGS_DEBUG_PRINTF("[SEARCH]---audio count = 0!!\n");
				return -1;
			}
			else
			{
				sdt->service_desc.service_type = SI_AUDIO_SERVICE;
			}
		}
		else
		{
			sdt->service_desc.service_type = SI_VIDEO_SERVICE;
		}

	}
	if (sdt->service_desc.desc_valid == FALSE)	//û??????????
	{
		memset(sdt->service_desc.service_name, 0, MAX_PROG_NAME);
		memset(buff,0,15);
		srand((int)time(0)+y);
		y++;
		i = (uint32_t)(rand()%100);
		sprintf((char*)buff,"%s%d","CH",i);
		strcpy((char *)(sdt->service_desc.service_name), (char*)buff);

	}

	return i;
}

 /**
  * @brief
  * @param
  * @Return
  */
static status_t gx_search_prog_info_add(uint32_t * prog_id, GxTkgsClass *tkgs_class)
{
#define USER_DEF_INIT_LCN 2001

	status_t				ret = 0;
	int32_t 				count = 0;
	uint16_t				ts_id = 0;
	uint16_t				service_id = 0;
	int32_t 				num = 0;
	GxSearchSdtBody 		sdt_info = { 0 };
	//GxSearchPatBody 		pat_info = { 0 };
	GxBusPmDataProg 		prog = { 0 };
	GxBusPmDataProg        progbak = {0};
	AppProgExtInfo prog_ext_info = {{0}};
	uint32_t i = 0;
	GxBusPmDataProgAudioType type = 0xff;
	uint32_t a_pid_temp = 0xffff;

	/*Ѱ?Ҹý?Ŀ??Ӧ??sdt,sdt_num -1????  ????ֵ???? */
	num = gx_search_sdt_find(&sdt_info, tkgs_class);
	if (num == -1) {
		TKGS_DEBUG_PRINTF("===%s, %d, find sdt err!!\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}

	ts_id = sdt_info.ts_id;
	service_id = tkgs_class->search_pmt_body.service_id;

	if (0 == strncmp((const char *)sdt_info.service_desc.service_name, "TKGS", 4))
	{
		//app_log_debug("$$will save pased location, serviceType:%d, audioCount:%d, pid:%d\n", sdt_info.service_desc.service_type, tkgs_class->search_pmt_body.audio_count, tkgs_class->search_pmt_body.tkgs_pid);
		if (GXCORE_SUCCESS == _tkgs_save_parsed_location(tkgs_class->search_tp_id_for_prog,tkgs_class->search_pmt_body.tkgs_pid))
		{
			return GX_TKGS_SEARCH_LOCATION;
		}
	}
	else if (tkgs_class->search_type == GX_TKGS_UPDATE_LOCATION_AND_BLOCKING)
	{
		//??ģʽ?²???Ҫ???????ݿ⣬ֻ??????invisible location??blocking????
		return GXCORE_SUCCESS;
	}

	/*???⵱ǰ??Ŀ????????????????*/
	if (sdt_info.service_desc.service_type == SI_AUDIO_SERVICE)
	{
		prog.service_type = GXBUS_PM_PROG_RADIO;
	}else if(sdt_info.service_desc.service_type == SI_VIDEO_SERVICE){
		prog.service_type = GXBUS_PM_PROG_TV;
	}
	else if((sdt_info.service_desc.service_type == SI_NVOD_REFERENCE_SERVICE) ||
			(sdt_info.service_desc.service_type == SI_NVOD_TIMESHIFT_SERVICE)){
		prog.service_type = GXBUS_PM_PROG_NVOD;
	}
	else
	{
		prog.service_type = GXBUS_PM_PROG_TV;
	}
	prog.sdt_version = sdt_info.sdt_version;
	prog.video_type = tkgs_class->search_pmt_body.service_type;
	    if(GXBUS_PM_PROG_H264 == prog.video_type)
          {
           prog.definition = GXBUS_PM_PROG_HD;
           }
	if (tkgs_class->search_pmt_body.pmt_ca_desc.desc_valid == TRUE) {
		prog.scramble_flag = GXBUS_PM_PROG_BOOL_ENABLE;
		prog.cas_id = tkgs_class->search_pmt_body.pmt_ca_desc.cas_id;
		prog.ecm_pid_v = tkgs_class->search_pmt_body.ecm_pid_video;
	}
	else
	{
		if (sdt_info.flag_free_ca_mode == 1)//2014.09.26, a
		{
			prog.scramble_flag = GXBUS_PM_PROG_BOOL_ENABLE;
		}
		else
		{
			prog.scramble_flag = GXBUS_PM_PROG_BOOL_DISABLE;
			prog.cas_id = CAS_ID_FREE;
		}
	}
	prog.video_pid = tkgs_class->search_pmt_body.video_pid;
	prog.pcr_pid = tkgs_class->search_pmt_body.pcr_pid;

		/*???⵱ǰ??Ŀ?治???? */
	count = GxBus_PmProgExistChek(tkgs_class->search_tp_id_for_prog,
			ts_id,
			service_id,
			sdt_info.orig_network_id,
			tkgs_class->search_tuner_num_for_prog,
            0xff,
            0xff,
            0xff);
	if (count != 0) {
       GxBus_PmProgGetById(count,&progbak);
			for (i=0; i<sdt_info.extended_service_desc.category_num; i++)
	             {
		        prog.favorite_flag |= 1<<(sdt_info.extended_service_desc.category_id[i]/16);
	              }
	//app_log_debug("progbak.favorite_flag:[%d],prog.favorite_flag:[%d] (progbak.favorite_flag&0x000000ff):[%d]\n", progbak.favorite_flag,prog.favorite_flag,(progbak.favorite_flag&0x000000ff));
	if(prog.favorite_flag != (progbak.favorite_flag&0x000000ff))
			{
			progbak.favorite_flag = (progbak.favorite_flag&0xffffff00)|prog.favorite_flag;
				GxBus_PmProgInfoModify(&progbak);
				}
		GxBus_PmProgExtInfoGet(progbak.id,sizeof(AppProgExtInfo),&prog_ext_info);
		if((app_tkgs_get_operatemode() == GX_TKGS_AUTOMATIC)\
			|| (prog_ext_info.tkgs.logicnum >= USER_DEF_INIT_LCN)\
			)
		{
	prog_ext_info.tkgs.locator_id = tkgs_class->tkgs_service_locator;
	if (GXCORE_ERROR == _tkgs_get_lcn_by_locator(tkgs_class->tkgs_service_locator,&(prog_ext_info.tkgs.logicnum),tkgs_class))
	{
		return GX_TKGS_SEARCH_LCN_INVALID;
	}

	if (true != _tkgs_lcn_conflict_check(progbak.service_type,prog_ext_info.tkgs.logicnum, tkgs_class->lcnList))//temp del, 10.31
	{
		ret |= GxBus_PmProgExtInfoAdd(progbak.id,sizeof(AppProgExtInfo),&prog_ext_info);
	}

					}
		return GX_TKGS_SEARCH_PROG_EXIST;
	}

	/*??��??Ŀ??Ϣ?ṹ */
	prog.tp_id = tkgs_class->search_tp_id_for_prog;
	prog.sat_id = tkgs_class->search_sat_id_for_prog;
	prog.service_id = service_id;
	prog.audio_volume = 50;
	prog.audio_mode = GXBUS_PM_PROG_AUDIO_MODE_LEFT;
	prog.logic_valid = 0;

	prog.skip_flag = GXBUS_PM_PROG_BOOL_DISABLE;
	prog.lock_flag = GXBUS_PM_PROG_BOOL_DISABLE;
	if (tkgs_class->search_pmt_body.pmt_ttx_desc.ttx_desc_valid == TRUE) {
		prog.ttx_flag = GXBUS_PM_PROG_BOOL_ENABLE;
		if (tkgs_class->search_pmt_body.pmt_ttx_desc.cc_desc_valid == TRUE) {
			prog.cc_flag = GXBUS_PM_PROG_BOOL_ENABLE;
		}
	} else {
		prog.ttx_flag = GXBUS_PM_PROG_BOOL_DISABLE;
		prog.cc_flag = GXBUS_PM_PROG_BOOL_DISABLE;
	}
	if (tkgs_class->search_pmt_body.pmt_subt_desc.desc_valid == TRUE) {
		prog.subt_flag = GXBUS_PM_PROG_BOOL_ENABLE;
	} else {
		prog.subt_flag = GXBUS_PM_PROG_BOOL_DISABLE;
	}
	prog.favorite_flag = 0;
	prog.pmt_pid = 0x1fff;//pat_info.pmt_pid;	//????û??pat , ????pmt pid??Ϊ??Ч
	//prog.pat_version = pat_info.pat_version;
	prog.bouquet_id = 0;	//?ȴ?si????bat??
	prog.pmt_version = 0;	//??Ҫ???صĻ??ó?ʼ0?汾Ҳ?ǿ??Ե?
	prog.audio_count = tkgs_class->search_pmt_body.audio_count;	//????��ac3
	prog.audio_level = GXBUS_PM_PROG_AUDIO_LECEL_MID;
	if (tkgs_class->search_pmt_body.pmt_ac3_desc.desc_valid == TRUE) {
		prog.ac3_flag = GXBUS_PM_PROG_BOOL_ENABLE;
		for(i=0; i<tkgs_class->search_stream_body_count; i++)
		{
			if(tkgs_class->search_stream_body[i].audio_type == GXBUS_PM_AUDIO_AC3)//ac3
			{
				prog.ac3_pid = tkgs_class->search_stream_body[i].audio_pid;
				prog.cur_audio_type= GXBUS_PM_AUDIO_AC3;
				break;
			}
			else if(tkgs_class->search_stream_body[i].audio_type == GXBUS_PM_AUDIO_EAC3)//eac3
			{
				prog.ac3_pid = tkgs_class->search_stream_body[i].audio_pid;
				prog.cur_audio_type= GXBUS_PM_AUDIO_EAC3;
				//	break;
			}
		}

	} else {
		prog.ac3_flag = GXBUS_PM_PROG_BOOL_DISABLE;
	}
	//??PID?????Ǵ˴???ȡaudio_pid??????pid????ʱ??ƥ??audio_pidʱ?Ļ???ȷaudio_pid
	if( a_pid_temp == 0xffff)
	{
		gx_search_prog_check_audio_lang(&(prog.cur_audio_pid),
				&type,//&(prog.cur_audio_type),
				&(prog.cur_audio_ecm_pid), tkgs_class);
		if(type != 0xff)
		{
			prog.cur_audio_type = type;
		}
	}
	prog.original_id = sdt_info.orig_network_id;
	memcpy(prog.prog_name, sdt_info.service_desc.service_name,
			MAX_PROG_NAME);
	prog.prog_name[MAX_PROG_NAME -1] = 0;
	//TKGS_DEBUG_PRINTF("[SEARCH]---name = %s\n", prog.prog_name);
	prog.ts_id = ts_id;
	prog.tuner = tkgs_class->search_tuner_num_for_prog;

	for (i=0; i<sdt_info.extended_service_desc.category_num; i++)
	{
		prog.favorite_flag |= 1<<(sdt_info.extended_service_desc.category_id[i]/16);
	}
	//prog.locator_id = tkgs_class->tkgs_service_locator;
	prog_ext_info.tkgs.locator_id = tkgs_class->tkgs_service_locator;
	if (GXCORE_ERROR == _tkgs_get_lcn_by_locator(tkgs_class->tkgs_service_locator,&(prog_ext_info.tkgs.logicnum),tkgs_class))
	{
		return GX_TKGS_SEARCH_LCN_INVALID;
	}

//	app_log_debug("-------service :%d, %s,[%d][%d, %d, %d], service_type:%d", prog.logicnum, prog.prog_name, prog.service_type, prog.video_pid, prog.pcr_pid, prog.video_type, sdt_info.service_desc.service_type);
	prog.data_plp_id      = 0xff;
	prog.common_plp_exist = 0xff;
	prog.common_plp_id    = 0xff;
	prog.common_status    = 0xff;
	prog.muti_ts_id       = 0xff;
	prog.muti_ts_exist    = 0x0;
	prog.muti_ts_code     = 0xff;
	prog.pdmx_flag        = 0x0;
	prog.pdmx_pid         = 0x1fff;
	ret = GxBus_PmProgAdd(&prog);
	if (true != _tkgs_lcn_conflict_check(prog.service_type,prog_ext_info.tkgs.logicnum, tkgs_class->lcnList))//temp del, 10.31
	{
		ret |= GxBus_PmProgExtInfoAdd(prog.id,sizeof(AppProgExtInfo),&prog_ext_info);
	}

	if (ret == GX_PM_DBASE_FULL) {
		TKGS_DEBUG_PRINTF("===%s, %d, dbase full!!\n", __FUNCTION__, __LINE__);
		return GX_TKGS_SEARCH_DBASE_FULL;
	}
	else if (ret == GXCORE_ERROR) {
		TKGS_ERRO_PRINTF("===%s, %d, add prog to dbase err!!\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}

	tkgs_class->search_prog_id_arry[tkgs_class->search_prog_id_num] = prog.id;
	tkgs_class->search_prog_id_num++;
	//TKGS_DEBUG_PRINTF("@@add prog, id:[%d],locator:[%d], lcn:[%d] total:[%d]\n", prog.id, prog.locator_id,prog.logicnum,  tkgs_class->search_prog_id_num);

	*prog_id = prog.id;
	return GXCORE_SUCCESS;
}


 /**
  * @brief
  * @param
  * @Return
  */
static status_t pmt_info_get(uint8_t*      parsed_data, GxTkgsClass *tkgs_class)
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
	status_t                ret = 0;
	uint32_t video_audio = 0;

	pmt_info = (PmtInfo *) (parsed_data);
	stream_info_count = (pmt_info->stream_count) + (pmt_info->ac3_count) +( pmt_info->eac3_count);	//????ac3?Ŀռ?
	stream_info = (stream_info_t *) (pmt_info->stream_info);

	tkgs_class->search_stream_body =
	    (GxBusPmDataStream *) GxCore_Malloc(sizeof(GxBusPmDataStream) *
					   stream_info_count);
	memset(tkgs_class->search_stream_body, 0, sizeof(GxBusPmDataStream) *stream_info_count);
	tkgs_class->search_stream_body_count = 0;

	tkgs_class->search_pmt_body.pcr_pid = pmt_info->pcr_pid;
	tkgs_class->search_pmt_body.service_id = pmt_info->prog_num;

	if (pmt_info->ca_count != 0)	//???ж?????Ƶ?ǲ???ͬ??
	{
		//app_log_debug("-------------%s, %d, pmt_ca_desc.desc_vali is true\n", __FUNCTION__, __LINE__);
		tkgs_class->search_pmt_body.pmt_ca_desc.desc_valid = TRUE;
		cas_info = pmt_info->ca_info;
	}
	if (pmt_info->ca_count2 != 0)	//???ж?????Ƶ?ǲ??Ƿֿ?????
	{
		//app_log_debug("-------------%s, %d, pmt_ca_desc.desc_vali is true\n", __FUNCTION__, __LINE__);
		tkgs_class->search_pmt_body.pmt_ca_desc.desc_valid = TRUE;
		cas_info2 = pmt_info->ca_info2;
	}
	tkgs_class->search_pmt_body.video_pid = 0;//??????һ??pmt?????ݲ???
	for (i = 0; i < (stream_info_count - (pmt_info->ac3_count)); i++)	//????ֻ????pcm????Ϣ
	{
		stream_type = stream_info[i].stream_type;
		switch (stream_type) {
		case MPEG_1_VIDEO:
		case MPEG_2_VIDEO:
			tkgs_class->search_pmt_body.service_type = GXBUS_PM_PROG_MPEG;
			video_audio = 0;
			break;
		case H264:
			tkgs_class->search_pmt_body.service_type = GXBUS_PM_PROG_H264;
			video_audio = 0;
			break;

		case H265:
			tkgs_class->search_pmt_body.service_type = GXBUS_PM_PROG_H265;
			video_audio = 0;
			break;

		case AVS:
			tkgs_class->search_pmt_body.service_type = GXBUS_PM_PROG_AVS;
			video_audio = 0;
			break;
		case MPEG_1_AUDIO:
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_type = GXBUS_PM_AUDIO_MPEG1;
			video_audio = 1;
			break;
		case MPEG_2_AUDIO:
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_type = GXBUS_PM_AUDIO_MPEG2;
			video_audio = 1;
			break;
		case AAC_ADTS:
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AAC_ADTS;
			video_audio = 1;
			break;
		case AAC_LATM:
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AAC_LATM;
			video_audio = 1;
			break;
		/*case PRIVATE_PES_STREAM:  //??????pmt??һ????EAC3
			//tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_type = GXBUS_PM_AUDIO_EAC3;
			//video_audio = 1;
			break;*/
		case LPCM:
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_type = GXBUS_PM_AUDIO_LPCM;
			video_audio = 1;
			break;
		case AC3:
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3;
			video_audio = 1;
			break;
		/*case DTS:
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS;
			video_audio = 1;
			break;*/
		case DOLBY_TRUEHD:
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DOLBY_TRUEHD;
			video_audio = 1;
			break;
		case AC3_PLUS:
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3_PLUS;
			video_audio = 1;
			break;
		/*case DTS_HD:
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS_HD;
			video_audio = 1;
			break;
		case DTS_MA:
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS_MA;
			video_audio = 1;
			break;*/
		case AC3_PLUS_SEC:
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3_PLUS_SEC;
			video_audio = 1;
			break;
		/*case DTS_HD_SEC:
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS_HD_SEC;
			video_audio = 1;
			break;*/
		case TKGS_DATA:
			video_audio = 3;
			break;

		default:
			//tkgs_class->search_stream_body_count = 0;
			//GxCore_Free(tkgs_class->search_stream_body);
			//tkgs_class->search_stream_body = NULL;
			TKGS_DEBUG_PRINTF("[SEARCH]---can't support stream type!!type = %x\n",stream_type);
			video_audio = 2;
			break;
		}
		if(video_audio == 0)
		{
			tkgs_class->search_pmt_body.video_pid = stream_info[i].elem_pid;
			gx_search_pmt_cas_get(cas_info,
					      pmt_info->ca_count,
					      cas_info2,
					      pmt_info->ca_count2,
						  video_audio, tkgs_class);
			j = 1;
		}
		else if(video_audio == 1)
		{
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_pid =
			    stream_info[i].elem_pid;


			gx_search_pmt_cas_get(cas_info,
					      pmt_info->ca_count,
					      cas_info2,
					      pmt_info->ca_count2,
					      video_audio, tkgs_class);
			memset(tkgs_class->search_stream_body
			       [tkgs_class->search_stream_body_count].name, 0, 4);
			if (stream_info[i].iso639_count != 0) {
				memcpy(tkgs_class->search_stream_body
				       [tkgs_class->search_stream_body_count].name,
				       stream_info[i].iso639_info[0].iso639, 3);
			}
			tkgs_class->search_pmt_body.audio_count++;	//????��ac3
			tkgs_class->search_stream_body_count++;	//??��ac3
			j = 1;
		}
		else if(video_audio == 3)//tkgs ??????PID
		{
			//app_log_debug("===tkgs_pid:%d\n", stream_info[i].elem_pid);
			tkgs_class->search_pmt_body.tkgs_pid = stream_info[i].elem_pid;
			j = 1;
		}
	}
	if(j == 0)
	{
		tkgs_class->search_stream_body_count = 0;
		GxCore_Free(tkgs_class->search_stream_body);
		tkgs_class->search_stream_body = NULL;
		TKGS_DEBUG_PRINTF("===%s, %d, invalid pmt info\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}

	tkgs_class->search_pmt_body.pmt_subt_desc.desc_valid = FALSE;
	if (pmt_info->subt_count != 0)	//GxSearchPmtSubtitlingDesc?ȴ?si????
	{
		tkgs_class->search_pmt_body.pmt_subt_desc.desc_valid = TRUE;
	}

	tkgs_class->search_pmt_body.pmt_ttx_desc.ttx_desc_valid = FALSE;
	tkgs_class->search_pmt_body.pmt_ttx_desc.cc_desc_valid = FALSE;
	if (pmt_info->ttx_count != 0) {
		tkgs_class->search_pmt_body.pmt_ttx_desc.ttx_desc_valid = TRUE;

		for (i = 0; i < pmt_info->ttx_count; i++) {
			ttx_descriptor =
			    (TeletextDescriptor *) (pmt_info->ttx_info[i]);//??Ϊ??????ttx_info??һ???????????Ķ???????????Ԫ????ÿһ??ttx???׵ĵ?ַ

			for (j = 0; j < ttx_descriptor->ttx_num; j++) {
                if(ttx_descriptor->ttx)
			        ttx_info = (Teletext*)((Teletext*)(ttx_descriptor->ttx) + j);
                else
                {
                    TKGS_DEBUG_PRINTF("===%s, %d, SEARCH, ttx descriptor error\n", __FUNCTION__, __LINE__);
                    break;
                }
				if (ttx_info->type == 0x02
					//|| ttx_info->type == 0x01   /*cc flag shenbin 0705*/
					||ttx_info->type == 0x05)	//0x02????0x5??cc???͵?ttx
				{
					tkgs_class->search_pmt_body.pmt_ttx_desc.cc_desc_valid = TRUE;
					break;
				}
			}
			if (tkgs_class->search_pmt_body.pmt_ttx_desc.cc_desc_valid == TRUE) {
				break;
			}
		}
	}

	tkgs_class->search_pmt_body.pmt_ac3_desc.desc_valid = FALSE;
	if (pmt_info->ac3_count != 0)	//GxSearchPmtAc3Desc?ȴ?si????search_stream_body_count++
	{
		tkgs_class->search_pmt_body.pmt_ac3_desc.desc_valid = TRUE;

		for (i = 0; i < pmt_info->ac3_count; i++) {
			ac3_descriptor =
			    (Ac3Descriptor *) (pmt_info->ac3_info[i]);
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3;	//ac3
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_pid =
			    ac3_descriptor->elem_pid;
			memset(tkgs_class->search_stream_body
			       [tkgs_class->search_stream_body_count].name, 0, 4);
			tkgs_class->search_stream_body_count++;
		}
	}
	if (pmt_info->eac3_count != 0)	//GxSearchPmtAc3Desc?ȴ?si????search_stream_body_count++
	{
		tkgs_class->search_pmt_body.pmt_ac3_desc.desc_valid = TRUE;

		for (i = 0; i < pmt_info->eac3_count; i++) {
			eac3_descriptor =
			    (Eac3Descriptor *) (pmt_info->eac3_info[i]);
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_type = GXBUS_PM_AUDIO_EAC3;
			tkgs_class->search_stream_body[tkgs_class->search_stream_body_count].audio_pid =
			    eac3_descriptor->elem_pid;
			memset(tkgs_class->search_stream_body
			       [tkgs_class->search_stream_body_count].name, 0, 4);
			tkgs_class->search_stream_body_count++;
		}
	}
	//if()GxSearchPmtServiceMoveDesc?ȴ?si????
	ret = gx_search_prog_info_add(&prog_id, tkgs_class);
	memset(&(tkgs_class->search_pmt_body), 0, sizeof(GxSearchPmtBody));

	if ((ret == GXCORE_ERROR)||(ret == GX_TKGS_SEARCH_DBASE_FULL)) {
		tkgs_class->search_stream_body_count = 0;
		GxCore_Free(tkgs_class->search_stream_body);
		tkgs_class->search_stream_body = NULL;
		TKGS_DEBUG_PRINTF("===%s, %d, add prog err!!ret = %x\n", __FUNCTION__, __LINE__,ret);
		return ret;
	}
	else if((ret == GX_TKGS_SEARCH_PROG_EXIST) || (ret == GX_TKGS_SEARCH_LOCATION) || (ret == GX_TKGS_SEARCH_LCN_INVALID))
	{
		//do nothing
		if ((ret == GX_TKGS_SEARCH_LOCATION))
		{
			TKGS_DEBUG_PRINTF("====%s, %d, prog exist, or is location parm\n", __FUNCTION__, __LINE__);
		}
	}

	/*for (i = 0; i < tkgs_class->search_stream_body_count; i++) {
		tkgs_class->search_stream_body[i].prog_id = prog_id;
	}*/
	/*GxBus_PmStreamAdd(prog_id, tkgs_class->search_stream_body_count,
			  tkgs_class->search_stream_body);*/
	/*stream ???ӵ????ݿ?????ԭsearch_stream_body_count??search_stream_body ?ȴ???һ??prog??add */
	tkgs_class->search_stream_body_count = 0;
	if (tkgs_class->search_stream_body != NULL)	//?ͷ?stream body Ϊ??һ????Ŀ??׼??
	{
		GxCore_Free(tkgs_class->search_stream_body);
		tkgs_class->search_stream_body = NULL;
	}

	return GXCORE_SUCCESS;
}

/**
 * @brief ????????????SDT????
 * @param
 * @Return
 */
static status_t sdt_info_get(uint8_t*      parsed_data, GxTkgsClass *tkgs_class)
{
	SdtInfo                *sdt_info = NULL;
	service_info_t         *service_info = NULL;
	uint32_t                body_count_in_section = 0;
	uint32_t                prog_name_length = 0;
	uint16_t                i = 0;
    uint32_t                y = 0;
	int32_t                utf8 = 0;
	uint8_t*               pstr = NULL;
	uint32_t               psize = 0;
	int8_t                name_temp[MAX_PROG_NAME];

	sdt_info = (SdtInfo *) (parsed_data);

	service_info = (service_info_t *) (sdt_info->service_info);

	body_count_in_section = sdt_info->service_count;

	if (tkgs_class->search_sdt_body_count + body_count_in_section > MAX_PROG_PER_TP) {

		body_count_in_section = MAX_PROG_PER_TP - tkgs_class->search_sdt_body_count;
	}
	for (i = 0; i < body_count_in_section; i++) {
		tkgs_class->search_sdt_body[tkgs_class->search_sdt_body_count].orig_network_id = sdt_info->orig_network_id;
		tkgs_class->search_sdt_body[tkgs_class->search_sdt_body_count].service_id =
		    service_info[i].service_id;

		tkgs_class->search_sdt_body[tkgs_class->search_sdt_body_count].ts_id = sdt_info->tsid;

		tkgs_class->search_sdt_body[tkgs_class->search_sdt_body_count].flag_sch =
		    service_info[i].EIT_sch_flag;

		tkgs_class->search_sdt_body[tkgs_class->search_sdt_body_count].flag_pf =
		    service_info[i].EIT_pf_flag;

		tkgs_class->search_sdt_body[tkgs_class->search_sdt_body_count].running_status =
		    service_info[i].running_status;

		tkgs_class->search_sdt_body[tkgs_class->search_sdt_body_count].flag_free_ca_mode =
		    service_info[i].free_ca_mode;

		tkgs_class->search_sdt_body[tkgs_class->search_sdt_body_count].sdt_version = sdt_info->si_info.ver_number;

		if (service_info[i].service_des_count != 0) {
			tkgs_class->search_sdt_body[tkgs_class->search_sdt_body_count].service_desc.desc_valid = TRUE;

			tkgs_class->search_sdt_body[tkgs_class->search_sdt_body_count].service_desc.service_type =
			    service_info[i].service_des_info->service_type;

			memset(tkgs_class->search_sdt_body
			       [tkgs_class->search_sdt_body_count].service_desc.service_name, 0, MAX_PROG_NAME);
			memset(name_temp,0,MAX_PROG_NAME);
			for(y=0; y<service_info[i].service_des_info->service_name_length; y++)
			{
				if( service_info[i].service_des_info->service_name[y]<0x1f||service_info[i].service_des_info->service_name[y]>0x20)
				{
					break;
				}
			}

			if (service_info[i].service_des_info->service_name_length-y >= MAX_PROG_NAME-1)
			{
				prog_name_length = MAX_PROG_NAME - 2;
			}
			else
			{
				prog_name_length =
					service_info[i].service_des_info->service_name_length-y;
			}
			if(prog_name_length == 0)
			{
				strcpy((char*)name_temp,"no name");
				prog_name_length = 8;
			}
			else
			{
	#if 1
				name_temp[0] = 0x05;
				memcpy(&name_temp[1],&(service_info[i].service_des_info->service_name[y]),prog_name_length);
	#else
				memcpy(name_temp,service_info[i].service_des_info->service_name,prog_name_length);
	#endif
			}


			GxBus_ConfigGetInt(GXBUS_SEARCH_UTF8,&utf8,GXBUS_SEARCH_UTF8_NO);
			if(utf8 == GXBUS_SEARCH_UTF8_NO)
			{
				memcpy(tkgs_class->search_sdt_body[tkgs_class->search_sdt_body_count].service_desc.service_name,
						name_temp,
						prog_name_length);
			}
			else if(utf8 == GXBUS_SEARCH_UTF8_YES)
			{
				if(gxgdi_iconv((const unsigned char *)name_temp,&pstr,MAX_PROG_NAME,&psize, NULL) == 0)
				{
					if(psize >= MAX_PROG_NAME)
					{
						prog_name_length = MAX_PROG_NAME - 1;
					}
					else
					{
						prog_name_length = psize;
					}
					memcpy(tkgs_class->search_sdt_body[tkgs_class->search_sdt_body_count].service_desc.service_name,
							pstr,prog_name_length);
					GxCore_Free(pstr);
				}
				else
				{
					TKGS_DEBUG_PRINTF("[SEARCH]---iconv utf8 err!!------\n");
					memcpy(tkgs_class->search_sdt_body[tkgs_class->search_sdt_body_count].service_desc.service_name,
							name_temp,
							prog_name_length);
				}
			}
			tkgs_class->search_sdt_body[tkgs_class->search_sdt_body_count].service_desc.service_name[MAX_PROG_NAME-1] = 0;

		} else {
			tkgs_class->search_sdt_body[tkgs_class->search_sdt_body_count].service_desc.desc_valid = FALSE;
		}

		//if()?ж?caindentify
		//if()?ж?mulit name
		tkgs_class->search_sdt_body_count++;

	}

	GxBus_SiParserBufFree(parsed_data);

	return GXCORE_SUCCESS;
}


/*tkgs_info field contains some global information about the TKGS data */
/*
multilingual_user_message_descriptor	0x90 //ÿ??TKGS????????Ҫ?û?????ʱ???ᷢ????չʾ???û?
tkgs_standard_descriptor				0x91

*/
static status_t _tkgs_tkgs_info_parse(uint8_t *pBuffIn, uint32_t length, GxTkgsClass *tkgs_class)
{
	int32_t size = length;
	uint8_t *p = pBuffIn;
	uint8_t descriptor_length = 0;
	uint8_t	msg_length = 0;

	uint8_t    language[3];
	int init_value = 0;
	char *curLang = NULL;

	GxBus_ConfigGetInt(OSD_LANG_KEY, &init_value, OSD_LANG);
	curLang = app_get_short_lang_str(init_value);

    while(size > 0)
    {
        switch(p[0])
        {
			case TKGS_MULTI_LANG_USER_MSG_DES_TAG:
				descriptor_length = p[1];
				p+= 2;
				size -= 2;
				while(descriptor_length > 0)
				{
					memcpy(language, p, 3);
					msg_length = p[3];
					if ((strncmp((char*)language,curLang,3) == 0)
						|| (strncmp((char*)language,"TUR",3) == 0) || (strncmp((char*)language,"tur",3) == 0))
					{
						tkgs_class->tkgs_message = GxCore_Malloc(msg_length+1);
						memcpy(tkgs_class->tkgs_message, &p[4], msg_length);
						tkgs_class->tkgs_message[msg_length] = '\0';
						break;
					}
					descriptor_length -= (3+1+msg_length);
					p+= (3+1+msg_length);
					size -= (3+1+msg_length);
				}
				break;

			case TKGS_STANDARD_DES_TAG:
				descriptor_length = p[1];
				p+= (2+descriptor_length);
				size -= (2+descriptor_length);
				break;

			default:
				descriptor_length = p[1];
				p+= (2+descriptor_length);
				size -= (2+descriptor_length);
				break;
		}

    }

	return GXCORE_SUCCESS;
}


/**
 * @brief ???????ǲ??????????????????Ǿ???
 * @param
 * @Return
 */
static status_t _tkgs_broadcast_source_descriptor_loop_parse(uint8_t *pBuffIn, uint32_t length, GxTkgsClass *tkgs_class)
{
	int32_t offset = 0;
	uint8_t *p = pBuffIn;
	uint8_t descriptor_length = 0;
	uint8_t	name_length = 0;
	SatParam_s sat_param={0};

    while(offset < length)
    {
        switch(p[offset])
        {
			case TKGS_SAT_BROADCAST_SOURCE_DES_TAG:
				descriptor_length = p[offset+1];
				name_length = p[offset+5]&0x3f;
				memcpy(sat_param.name, &p[offset+6], name_length);
				sat_param.orbital_position = p[offset+6+name_length]<<8 | p[offset+6+name_length+1];
				sat_param.orbital_position = GET_DEC16(sat_param.orbital_position);
				sat_param.west_east_flag = (p[offset+6+name_length+2]&0x80)>>7;
				sat_param.west_east_flag
					= (sat_param.west_east_flag==0? GXBUS_PM_SAT_LONGITUDE_DIRECT_WEST:GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST);
				offset += (2+descriptor_length);
				if(GXCORE_SUCCESS == _tkgs_add_sat(&sat_param))
				{
					tkgs_class->search_sat_id_for_prog = sat_param.id;
				}
				break;

			default:
				descriptor_length = p[offset+1];
				offset += (2+descriptor_length);
				break;
        }
    }

	return GXCORE_SUCCESS;
}

static status_t _tkgs_transport_stream_loop_parse(uint8_t *pBuffIn, uint32_t length, GxTkgsClass *tkgs_class)
{
	uint8_t transport_stream_descriptors_length=0;
	uint16_t service_description_section = 0;
	uint16_t program_loop_length = 0;
	uint32_t offset = 0;
	uint32_t status = GXCORE_ERROR;
	uint8_t	 sdt_last_section_number = 0;
	uint8_t i;

	while (length>offset)
	{
		tkgs_class->search_sdt_body_count = 0;
		memset(tkgs_class->search_sdt_body, 0, sizeof(GxSearchSdtBody) * MAX_PROG_PER_TP);

		transport_stream_descriptors_length = pBuffIn[offset];
		offset += 1;
		status = _tkgs_transport_stream_descriptor_loop_parse(pBuffIn+offset,transport_stream_descriptors_length, tkgs_class);
		if (status == GXCORE_ERROR)
		{
			TKGS_ERRO_PRINTF("===%s, %d, parse tkgs info err!\n", __FUNCTION__, __LINE__);
			return status;
		}

		offset += transport_stream_descriptors_length;
		sdt_last_section_number = pBuffIn[offset+7];

		//TKGS_DEBUG_PRINTF("===%s, %d, sdt_last_section_number:%d\n", __FUNCTION__, __LINE__, sdt_last_section_number);
		for (i=0; i<=sdt_last_section_number; i++)
		{
			service_description_section = 0;
			status = _tkgs_service_description_section_parse(pBuffIn+offset,&service_description_section, tkgs_class);
			offset += service_description_section;
			if (status == GXCORE_ERROR)
			{
				TKGS_DEBUG_PRINTF("===%s, %d, parse sdt section err!\n", __FUNCTION__, __LINE__);
				//return status;
			}
		}

		program_loop_length = (pBuffIn[offset]<<8)|pBuffIn[offset+1];
		offset += 2;
		//TKGS_DEBUG_PRINTF("===%s, %d, program_loop_length:%d\n", __FUNCTION__, __LINE__, program_loop_length);
		status = _tkgs_program_loop_parse(pBuffIn+offset,program_loop_length, tkgs_class);
		offset += program_loop_length;
		if (status == GXCORE_ERROR)
		{
			TKGS_DEBUG_PRINTF("===%s, %d, parse pmt loop err!\n", __FUNCTION__, __LINE__);
		}
	}

	TKGS_DEBUG_PRINTF("===%s, %d, _tkgs_transport_stream_loop_parse ok!\n", __FUNCTION__, __LINE__);
	return GXCORE_SUCCESS;
}

/**
 * @brief ????TP???????����?????TP?????ݿ?
 * @param
 * @Return
 */
static status_t satellite_delivery_system_descriptor_parse(uint8_t tag, uint8_t *p_data, GxTkgsClass *tkgs_class)
{
	GxMsgProperty_NodeAdd NodeAdd;
	int32_t tp_exist_count = 0;
	uint16_t tp_id = 0;

	memset(&NodeAdd, 0, sizeof(GxMsgProperty_NodeAdd));

	NodeAdd.node_type = NODE_TP;
	NodeAdd.tp_data.sat_id = tkgs_class->search_sat_id_for_prog;
	NodeAdd.tp_data.frequency = TODATA32 (p_data[2],p_data[3],p_data[4],p_data[5]);// InputTpPara.frequency;
	// change to MHz
	NodeAdd.tp_data.frequency = GET_DEC32(NodeAdd.tp_data.frequency)/100;
	NodeAdd.tp_data.tp_s.symbol_rate = (p_data[9]<<20 )|(p_data[10]<<12 )
						|(p_data[11]<< 4 )|((p_data[12]>>4 )& 0x0f);
	// change to Ksymbol/s
	NodeAdd.tp_data.tp_s.symbol_rate = GET_DEC32(NodeAdd.tp_data.tp_s.symbol_rate)/10;

	NodeAdd.tp_data.tp_s.polar = (p_data[8]>>5) & 0x03 ;// 0:H  1:V   InputTpPara.polar;


	tp_exist_count = GxBus_PmTpExistChek(NodeAdd.tp_data.sat_id,
										NodeAdd.tp_data.frequency,
										NodeAdd.tp_data.tp_s.symbol_rate,
										NodeAdd.tp_data.tp_s.polar,
										0,
										&tp_id);
	if(tp_exist_count == 0)
	{
		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_ADD, &NodeAdd))
		{
			tkgs_class->search_tp_id_for_prog = NodeAdd.tp_data.id;
		}
		else
		{
			TKGS_ERRO_PRINTF("===%s, %d, err\n", __FUNCTION__, __LINE__);
			return GXCORE_ERROR;
		}
	}
	else
	{
		tkgs_class->search_tp_id_for_prog = tp_id;
	}

	return GXCORE_SUCCESS;
}

/**
 * @brief SDT loop????TKGS˽????????TKGS_EXTN_SERVICE_DES_TAG
 * @param
 * @Return
 */
static status_t _tkgs_sdt_desc_loop_parse(uint8_t *pBuffIn,uint32_t len, uint16_t	service_id, GxTkgsClass *tkgs_class)
{
    int32_t offset = 0;
	int32_t offset2 = 0;
    uint8_t* p = NULL;
    uint32_t descriptor_length = 0;
	uint32_t i = 0;
	uint32_t j = 0;

	if ((NULL == pBuffIn) || (len==0))
	{
		TKGS_ERRO_PRINTF("===%s, %d, parm err\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}

	for (i=0; i<tkgs_class->search_sdt_body_count; i++)
	{
		if (tkgs_class->search_sdt_body[i].service_id == service_id)
		{
			break;
		}
	}

	if (i == tkgs_class->search_sdt_body_count)
	{
		TKGS_ERRO_PRINTF("===%s, %d, err\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}


    while(offset < len)
    {
        switch(pBuffIn[offset])
        {
            case TKGS_EXTN_SERVICE_DES_TAG:
				descriptor_length = pBuffIn[offset+1];

				offset2 = 0;
				p = &pBuffIn[offset+8];
				while(offset2<descriptor_length-6)
				{
					tkgs_class->search_sdt_body[i].extended_service_desc.category_id[j++] = ((p[offset2]&0x0f)<<8)|p[offset2+1];

					if (j>=MAX_TKGS_CATEGORY)
					{
						TKGS_DEBUG_PRINTF("$$$$$$$$$$$$$$$$$$%s, %d\n", __FUNCTION__, __LINE__);
						break;
					}
					offset2 += 2;
				}
				tkgs_class->search_sdt_body[i].extended_service_desc.category_num = j;
				offset += (2+descriptor_length);
				break;

			default:
				//app_log_debug("=====%s, %d, sdt other tag:%d\n", __FUNCTION__, __LINE__, pBuffIn[offset]);
				descriptor_length = pBuffIn[offset+1];//p[offset+1];
				offset += (2+descriptor_length);
                break;
        }
    }

    return GXCORE_SUCCESS;
}


/**
 * @brief tkgs_extended_service_descriptor ????
 * @param
 * @Return
 */
static status_t _tkgs_sdt_extended_parse(uint8_t *pBuffIn, uint16_t length, GxTkgsClass *tkgs_class)
{
	uint16_t	service_id;
	uint16_t	offset = 0;
	uint8_t *p = NULL;
	uint16_t descriptors_loop_length = 0;

	if ((NULL == pBuffIn) || (length<15) || (NULL == tkgs_class))
	{
		TKGS_ERRO_PRINTF("===%s, %d, parm err\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}

	p = pBuffIn+11;
	offset = 0;
	while (offset < length-15)
	{
		service_id = (p[offset]<<8)|p[offset+1];
		descriptors_loop_length = ((p[offset+3]&0x0f)<<8)|p[offset+4];
		_tkgs_sdt_desc_loop_parse(p+offset+5,descriptors_loop_length,service_id,tkgs_class);
		offset += (5+descriptors_loop_length);
	}

	return GXCORE_SUCCESS;
}

/**
 * @brief transport_stream_descriptor_loop ????
 * @param
 * @Return
 */
static status_t _tkgs_transport_stream_descriptor_loop_parse(uint8_t *pBuffIn, uint32_t length, GxTkgsClass *tkgs_class)
{
	int32_t offset = 0;
	uint8_t *p = pBuffIn;
	uint8_t descriptor_length = 0;

    while(offset < length)
    {
        switch(p[offset])
        {
			case SATELLITE_DELIVERY_SYSTEM_DESC:
				descriptor_length = p[offset+1];
				satellite_delivery_system_descriptor_parse(p[offset],pBuffIn+offset, tkgs_class);
				offset += (2+descriptor_length);
				break;

			default:
				descriptor_length = p[offset+1];
				offset += (2+descriptor_length);
				break;
        }
    }

	//??ĿƵ??????
	return GXCORE_SUCCESS;
}


/**
 * @brief SDT section ?????????ݴ???
 * @param
 * @Return
 */

static status_t _tkgs_service_description_section_parse(uint8_t *pBuffIn, uint16_t *secLength, GxTkgsClass *tkgs_class)
{
	private_table_cfg table_cfg = {PARSE_STANDARD_ONLY, NULL};
	int16_t si_subtable_id	= 0XFF00;//GxBus_SiParserҪ?ã???????һ????

	void *parsed_data = NULL;
	uint32_t status = GXCORE_ERROR;
	uint32_t length=0;

	length = (((pBuffIn[1]&0x0f)<<8) | pBuffIn[2]) + 3;
	*secLength = length;
	parsed_data = GxBus_SiParser(&table_cfg, pBuffIn,length,si_subtable_id);
	if ((parsed_data == SEC_NO_MEMORY) || (parsed_data == SEC_SYNTAX_INDICATOR) || (parsed_data == SEC_LEN_ERROR))
	{
		TKGS_DEBUG_PRINTF("===%s, %d, parse sdt err\n", __FUNCTION__, __LINE__);
		return status;
	}
	status = sdt_info_get(parsed_data, tkgs_class);
	status |= _tkgs_sdt_extended_parse(pBuffIn,*secLength,tkgs_class);
	return status;
}

/**
 * @brief ??��N??tkgs_service_locator??????Ӧpmt???Ľ???
 * @param
 * @Return
 */
static status_t _tkgs_program_loop_parse(uint8_t *pBuffIn, uint32_t length, GxTkgsClass *tkgs_class)
{
	private_table_cfg table_cfg = {PARSE_STANDARD_ONLY, NULL};
	int16_t si_subtable_id	= 0XFFF0;//GxBus_SiParserҪ?ã???????һ????
	uint16_t	pmt_sec_length = 0;

	void *parsed_data = NULL;
	uint32_t status = GXCORE_SUCCESS;
	uint32_t offset = 0;

	while (offset < length)
	{
		tkgs_class->tkgs_service_locator = ((pBuffIn[offset]&0x1f)<<8)| pBuffIn[offset+1];
		pmt_sec_length = (((pBuffIn[offset+3]&0x0f)<<8) | pBuffIn[offset+4]) + 3;
		parsed_data = GxBus_SiParser(&table_cfg, &pBuffIn[offset+2],pmt_sec_length,si_subtable_id);
		offset += (2+pmt_sec_length);
		if ((parsed_data == SEC_NO_MEMORY) || (parsed_data == SEC_SYNTAX_INDICATOR) || (parsed_data == SEC_LEN_ERROR))
		{
			TKGS_DEBUG_PRINTF("=====%s, %d, parse pmt section error, locator:%d\n", __FUNCTION__, __LINE__, tkgs_class->tkgs_service_locator);
			continue;
		}
		status = pmt_info_get(parsed_data, tkgs_class);
		GxBus_SiParserBufFree(parsed_data);
	}

	return status;
}



/*broadcast_source_data field defined broadcast sources (satellites, cable tv networks??), their
associated multiplexes (transport streams), and their associated services.*/
/*
broadcast_source_data()
{
	broadcast_source_descriptors_length	//8bits
	broadcast_source_descriptor_loop
	{
		//satellite_broadcast_source_descriptor	(0xA0)
		//cable_broadcast_source_descriptor	(0xA1)
		//terrestrial_ broadcast_source_descriptor (0xA2)
		descriptor()
	}
	reserved								//4bits
	transport_stream_loop_length			//20bits
	transport_stream_loop
	{
		transport_stream_descriptors_length	//8bits
		transport_stream_descriptor_loop
		{
			//satellite_delivery_system_descriptor (0x43)
			//cable_delivery_system_descriptor 	(0x44)
			//terrestrial_delivery_system_descriptor	(0x44)
			descriptor()
		}
		for(i=0;i<N;i++)
		{
			//service_descriptor	(0x48)
			//tkgs_extended_service_descriptor		(0x95)
			service_description_section()
		}
		program_loop_length						//16bits
		program_loop
		{
			reserved								//3bits
			tkgs_service_locator					//13bits
			program_map_section()	//AC?\3_descriptor (0xA6), enhanced_AC?\3_descriptor (0xA7), constant_control_word_descriptor (0x96), tkgs_component_descriptor (0x98)
		}
	}
}*/
static status_t _tkgs_broadcast_sourc_parse(uint8_t *pBuffIn, uint32_t length, GxTkgsClass *tkgs_class)
{
	int32_t offset = 0;
	uint32_t status = GXCORE_ERROR;
	int8_t broadcast_source_descriptors_length = 0;
	uint32_t transport_stream_loop_length= 0;

	if ((pBuffIn == NULL) || (length == 0))
	{
		return GXCORE_ERROR;
	}

	broadcast_source_descriptors_length = pBuffIn[offset];
	offset += 1;
	status = _tkgs_broadcast_source_descriptor_loop_parse(pBuffIn+offset,broadcast_source_descriptors_length, tkgs_class);
	if (status == GXCORE_ERROR)
	{
		TKGS_DEBUG_PRINTF("===%s, %d, parse tkgs info err!\n", __FUNCTION__, __LINE__);
	}
	offset += broadcast_source_descriptors_length;

	transport_stream_loop_length = ((pBuffIn[offset]&0x0F)<<16 | (pBuffIn[offset+1]<<8)| pBuffIn[offset+2]);
	offset += 3;

	status = _tkgs_transport_stream_loop_parse(pBuffIn+offset,transport_stream_loop_length, tkgs_class);
	if (status == GXCORE_ERROR)
	{
		TKGS_DEBUG_PRINTF("===%s, %d, parse tkgs info err!\n", __FUNCTION__, __LINE__);
	}

	return status;
}

/**
 * @brief TKGS_MULTI_SERV_LIST_NAME_DES_TAG, TKGS_SERVICE_LIST_LOCATION_DES_TAG ????????
 * @param
 * @Return
 */
static status_t _tkgs_service_list_descriptor_parse(uint8_t tag, uint8_t *p_data, GxTkgsPrefer *list)
{
	int32_t offset = 0;
	uint8_t descriptor_length = 0;

	uint8_t    language[3];
	int init_value = 0;
	char *curLang = NULL;
	uint8_t name_len = 0;
	uint8_t temp_len = 0;

	if ((NULL == p_data) || (NULL == list) || (NULL == list->list))
	{
		TKGS_ERRO_PRINTF("===%s, %d, parm err\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}

	GxBus_ConfigGetInt(OSD_LANG_KEY, &init_value, OSD_LANG);
	curLang = app_get_short_lang_str(init_value);

	switch(tag)
	{
		case TKGS_MULTI_SERV_LIST_NAME_DES_TAG:
			descriptor_length = p_data[1];
			offset += 2;
			while(descriptor_length>0)
			{
				memcpy(language, &p_data[offset], 3);
				name_len = p_data[offset+3];
				temp_len = name_len<TKGS_PREFER_NAME_LEN-1 ? name_len:TKGS_PREFER_NAME_LEN-1;
				if (strncmp((char*)language,curLang,3) == 0)
				{
					memcpy(list->name, &p_data[offset+4], temp_len);
					list->name[temp_len] = '\0';
					break;
				}
				else if ((strncmp((char*)language,"TUR",3) == 0) || (strncmp((char*)language,"tur",3) == 0))
				{
					memcpy(list->name, &p_data[offset+4], temp_len);
					list->name[temp_len] = '\0';
				}

				descriptor_length -= (3+1+name_len);
				offset += (3+1+name_len);
			}
			//app_log_debug("=========prefer list name:[%s]\n", list->name);
			break;

		case TKGS_SERVICE_LIST_LOCATION_DES_TAG:
			descriptor_length = p_data[1];
			if (descriptor_length >= 4)
			{
				list->list[list->serviceNum].lcn = (p_data[2]<<8)|p_data[3];
				list->list[list->serviceNum].locator = ((p_data[4]&0x1f)<<8)| p_data[5];
				//app_log_debug("@@list(%d):[%d, %d]\n", list->serviceNum, list->list[list->serviceNum].lcn, list->list[list->serviceNum].locator);
				list->serviceNum++;
			}
			break;

		default:
			break;

	}

	return GXCORE_SUCCESS;
}

/*a service list is a collection of of ??Locator?\LCN?? pairs*/
/*
service_list()
{
	service_list_descriptors_length
	service_list_descriptor_loop
	{
		descriptor()	//multilingual_service_list_name_descriptor (0x93); service_list_location_descriptor (0x94)
	}
}*/
	//???????ԣ??????ݲ?ͬ??ģʽ?????ݿ⴦??
static status_t _tkgs_service_list_parse(uint8_t *pBuffIn, uint32_t length, GxTkgsClass *tkgs_class)
{
	int32_t offset = 0;
	int32_t offset2 = 0;
	uint8_t descriptor_length = 0;
	int16_t service_list_descriptors_length = 0;
	uint8_t *p = NULL;
	GxTkgsPreferredList	preferred_list = {0, NULL};
	int32_t i = 0;
    int32_t n = 0;

	if ((pBuffIn == NULL) || (length == 0))
	{
		TKGS_ERRO_PRINTF("===%s, %d, parm err\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}

    while(offset < length)
    {
    	service_list_descriptors_length = (pBuffIn[offset]<<8) | pBuffIn[offset+1];
		offset += (2+service_list_descriptors_length);
		preferred_list.listNum++;
    }

	//no service list ,return
	if (preferred_list.listNum == 0)
	{
		TKGS_DEBUG_PRINTF("===%s, %d, no service list\n", __FUNCTION__, __LINE__);
		return GXCORE_SUCCESS;
	}

	preferred_list.prefer = (GxTkgsPrefer *)GxCore_Malloc(preferred_list.listNum * sizeof(GxTkgsPrefer));
	if (NULL == preferred_list.prefer)
	{
		TKGS_ERRO_PRINTF("===%s, %d, Malloc err\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}
	memset(preferred_list.prefer, 0, preferred_list.listNum * sizeof(GxTkgsPrefer));
	for (i=0; i<preferred_list.listNum; i++)
	{
		preferred_list.prefer[i].list = (GxTkgsService*)GxCore_Malloc(MAX_PROG_COUNT*sizeof(GxTkgsService));
		if (NULL == preferred_list.prefer[i].list)
		{
            for (n=0; n < i; n++)
            {
                APP_FREE(preferred_list.prefer[n].list);
            }
            APP_FREE(preferred_list.prefer);
			TKGS_ERRO_PRINTF("===%s, %d, Malloc err\n", __FUNCTION__, __LINE__);
			return GXCORE_ERROR;
		}
		memset(preferred_list.prefer[i].list, 0, MAX_PROG_COUNT*sizeof(GxTkgsService));
	}


	offset = 0;
	i = 0;
    while(offset < length)
    {
    	service_list_descriptors_length = (pBuffIn[offset]<<8) | pBuffIn[offset+1];
		offset += 2;
		p = pBuffIn+offset;
		offset2 = 0;
		while (offset2 < service_list_descriptors_length)
		{
			descriptor_length = p[offset2+1];
			_tkgs_service_list_descriptor_parse(p[offset2], &p[offset2],&preferred_list.prefer[i]);
			offset2 += (2+descriptor_length);
		}
		i++;
		offset += service_list_descriptors_length;
    }

	if (GXCORE_ERROR == GxBus_TkgsPreferredListSave(&preferred_list))
	{
		for (i=0; i<preferred_list.listNum; i++)
		{
            APP_FREE(preferred_list.prefer[i].list);
		}
        APP_FREE(preferred_list.prefer);
		TKGS_ERRO_PRINTF("===%s, %d, err\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}

	GxBus_TkgsUserPreferredListNameGet(tkgs_class->cur_prefer_list.name);
	if (strncmp(tkgs_class->cur_prefer_list.name,"default",strlen("default")) == 0)
	{
		for (i=0; i<preferred_list.listNum; i++)
		{
			if (strncmp("HD",preferred_list.prefer[i].name,2) == 0)
			{
				break;
			}
		}
		if (i == preferred_list.listNum)
		{
			i = 1;//0;
		}

		GxBus_TkgsUserPreferredListNameSet(preferred_list.prefer[i].name);
		GxBus_TkgsUserPreferredListNameGet(tkgs_class->cur_prefer_list.name);
	}
	else
	{
		for (i=0; i<preferred_list.listNum; i++)
		{
			if (strncmp(tkgs_class->cur_prefer_list.name,preferred_list.prefer[i].name,strlen(tkgs_class->cur_prefer_list.name)) == 0)
			{
				break;
			}
		}
		if (i == preferred_list.listNum)
		{
			i = 1;//0;
		}
	}

	tkgs_class->cur_prefer_list.serviceNum = preferred_list.prefer[i].serviceNum;
	tkgs_class->cur_prefer_list.list = (GxTkgsService 	*)GxCore_Malloc(preferred_list.prefer[i].serviceNum*sizeof(GxTkgsService));
	if (tkgs_class->cur_prefer_list.list == NULL)
	{
        for (i=0; i<preferred_list.listNum; i++)
		{
            APP_FREE(preferred_list.prefer[i].list);
		}
        APP_FREE(preferred_list.prefer);
		TKGS_ERRO_PRINTF("===%s, %d, Malloc err\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}
	memcpy(tkgs_class->cur_prefer_list.list, preferred_list.prefer[i].list, preferred_list.prefer[i].serviceNum*sizeof(GxTkgsService));

    for (i=0; i<preferred_list.listNum; i++)
    {
        APP_FREE(preferred_list.prefer[i].list);
    }
    APP_FREE(preferred_list.prefer);
	return GXCORE_SUCCESS;
}

/*
category_loop
{
	descriptor()	//multilingual_category_label_descriptor (0x92)
}
*/
static status_t _tkgs_category_parse(uint8_t *pBuffIn, uint32_t length)
{
	int32_t offset = 0;
	int32_t offset2 = 0;
	uint8_t *p = NULL;
	uint8_t name_length = 0;
	uint8_t temp_length = 0;
	uint8_t descriptor_length = 0;

	uint16_t category_id = 0;
	uint8_t category_name[100] = {0};

	uint8_t    language[3];
	int init_value = 0;
	char *curLang = NULL;

	int32_t utf8 = 0;
	int32_t size_category = 0;
	uint8_t*               pstr = NULL;


//	return GXCORE_SUCCESS;//test code

	GxBus_ConfigGetInt(OSD_LANG_KEY, &init_value, OSD_LANG);
	curLang = app_get_short_lang_str(init_value);

    while(offset < length)
    {
        switch(pBuffIn[offset])
        {
			case TKGS_MULTI_CATEGORY_LABEL_DES_TAG:
				descriptor_length = pBuffIn[offset+1];
				category_id = ((pBuffIn[offset+2]&0x0f)<<8)|pBuffIn[offset+3];

				p = pBuffIn+offset+4;
				offset2 = 0;
				while(offset2 < descriptor_length-2)
				{
					memcpy(language, &p[offset2], 3);
					name_length = p[3];
					temp_length = name_length>MAX_FAV_GROUP_NAME-1? MAX_FAV_GROUP_NAME-1:name_length;
					if (strncmp((char*)language,curLang,3) == 0)
					{
						memcpy(category_name, &p[4], temp_length);
						category_name[temp_length] = '\0';
						break;
					}
					else if ((strncmp((char*)language,"TUR",3) == 0) || (strncmp((char*)language,"tur",3) == 0))
					{
#if 1
						if (p[4] == 0x05)
						{
							memcpy(category_name, &p[4], temp_length);
							category_name[temp_length] = '\0';
						}
						else
						{
							category_name[0] = 0x05;
							memcpy(&category_name[1], &p[4], temp_length);
							category_name[++temp_length] = '\0';
						}
#else

						memcpy(category_name, &p[4], temp_length);
						category_name[temp_length] = '\0';
#endif
					}
					offset2 += 4+name_length;
				}
				#if 1
				GxBus_ConfigGetInt(GXBUS_SEARCH_UTF8,&utf8,GXBUS_SEARCH_UTF8_NO);
				if(utf8 == GXBUS_SEARCH_UTF8_YES)
				{
					if(gxgdi_iconv((const unsigned char *) category_name, &pstr, 100/*strlen((char*)category_name)*/, (unsigned int*)&size_category, NULL) == 0)
					{
						if(size_category > MAX_FAV_GROUP_NAME)
						{
							size_category = MAX_FAV_GROUP_NAME -1;
						}

						memcpy(category_name,pstr,size_category);
						category_name[size_category] = '\0';
						GxCore_Free(pstr);
					}
				}

				//add utf8convert
				#endif

				_tkgs_save_category_name(category_id,category_name);

				app_log_debug("category:[%d, %s\n",category_id, category_name);
				offset += (2+descriptor_length);
				break;

			default:
				descriptor_length = pBuffIn[offset+1];
				offset += (2+descriptor_length);
				break;
        }
    }

	return GXCORE_SUCCESS;
}


/*blocking data*/
/*
channel_blocking(){
delivery_filter_flag
service_id_filter_flag
network_id_filter_flag
transport_stream_id_filter_flag
video_pid_filter_flag
audio_pid_filter_flag
name_filter_flag
reserved
if(delivery_filter_flag == ??1??){
frequency_filter_flag
symbolrate_filter_flag
polarization_filter_flag
orbital_position_filter_flag
modulation_type_filter_flag
modulation_system_filter_flag
modulation_filter_flag
fec_inner_filter_flag
reserved
if(frequency_filter_flag == ??1??){
frequency_tolerance
}
if(symbolrate_filter_flag == ??1??){
symbolrate_tolerance
}
delivery_descriptor()
}
if(service_id_filter_flag == ??1??){
service_id
}
if(network_id_filter_flag == ??1??){
network_id
}
if(transport_stream_id_filter_flag == ??1??){
transport_stream_id
}
if(video_filter_flag == ??1??){
reserved
video_pid
}
if(audio_filter_flag == ??1??){
reserved
audio_pid
}
if(name_filter_flag == ??1??){
match_type
case_sensitivity
reserved
service_name_filter_length
for (i=0;i<N;I++){
Char
}
}
}
*/
/**
 * @brief 	????blocking????
 * @return   	GXCORE_SUCCESS:ִ??????
 				GXCORE_ERROR:ִ??ʧ??
 */
static status_t _tkgs_channel_blocking_parse(GxTkgsBlocking *pBlocking, uint8_t* src, uint32_t len)
{
	uint32_t offset = 0;
	GxTkgsBlockingFilter *filter = NULL;

	if ((NULL == pBlocking) || (NULL == src) || (len == 0))
	{
		TKGS_ERRO_PRINTF("===%s, %d, parm err\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}

	while ((offset < len) && (pBlocking->block_num < GX_TKGX_MAX_BLOCKING))
	{
		filter = &pBlocking->filter[pBlocking->block_num++];
		filter->filter_flag = ((src[offset]<<8) | src[offset+1]) & 0xFE00;
		offset += 2;

		if ((filter->filter_flag & TKGS_DELIVERY_FILTER) == TKGS_DELIVERY_FILTER)
		{
			filter->delivery_filter.delivery_filter_flag = ((src[offset]<<8) | src[offset+1]) & 0XFF00;
			offset += 2;
			if ((filter->delivery_filter.delivery_filter_flag & TKGS_DELIVERY_FREQ_FILTER) == TKGS_DELIVERY_FREQ_FILTER)
			{
				filter->delivery_filter.frequency_tolerance = src[offset];
				offset += 1;
			}

			if ((filter->delivery_filter.delivery_filter_flag & TKGS_DELIVERY_SYMB_FILTER) == TKGS_DELIVERY_SYMB_FILTER)
			{
				filter->delivery_filter.symbolrate_tolerance = src[offset];
				offset += 1;
			}

			switch (src[offset])
			{
			case SATELLITE_DELIVERY_SYSTEM_DESC:
                filter->delivery_filter.delivery.frequency = ((src[offset+2]<<24)|(src[offset+3]<<16)|(src[offset+4]<<8)|src[offset+5]);
				// change to MHz
				filter->delivery_filter.delivery.frequency = GET_DEC32(filter->delivery_filter.delivery.frequency)/100;
				filter->delivery_filter.delivery.orbital_position = (src[offset+6]<<8 )|src[offset+7];
				filter->delivery_filter.delivery.orbital_position = GET_DEC16(filter->delivery_filter.delivery.orbital_position);
                filter->delivery_filter.delivery.west_east_flag = ((src[offset+8] >> 7) & 0x01);
                filter->delivery_filter.delivery.polarization = ((src[offset+8] >> 5) & 0x03);
                filter->delivery_filter.delivery.roll_off = ((src[offset+8] >> 3) & 0x03);;
				filter->delivery_filter.delivery.modulation_system = ((src[offset+8] >> 2) & 0x01);;
				filter->delivery_filter.delivery.modulation_type = src[offset+8] &0x03;
                filter->delivery_filter.delivery.symbol_rate = ((((src[offset+9]<<24)|(src[offset+10]<<16)|(src[offset+11]<<8)|src[offset+12])>>4) & 0x0fffffff);
				// change to Ksymbol/s
				filter->delivery_filter.delivery.symbol_rate = GET_DEC32(filter->delivery_filter.delivery.symbol_rate)/10;
				filter->delivery_filter.delivery.FEC_inner = src[offset+12] & 0x0f;

            	offset += 13;
            	break;

			default:
				offset += 2+src[offset+1];
				break;
			}
		}

		if ((filter->filter_flag & TKGS_SERVICE_ID_FILTER) == TKGS_SERVICE_ID_FILTER)
		{
			filter->service_id = (src[offset]<<8)|src[offset+1];
			offset += 2;
		}

		if ((filter->filter_flag & TKGS_NETWORK_ID_FILTER) == TKGS_NETWORK_ID_FILTER)
		{
			filter->network_id = (src[offset]<<8)|src[offset+1];
			offset += 2;
		}

		if ((filter->filter_flag & TKGS_TRANSPORT_STREAM_ID_FILTER) == TKGS_TRANSPORT_STREAM_ID_FILTER)
		{
			filter->transport_stream_id = (src[offset]<<8)|src[offset+1];
			offset += 2;
		}

		if ((filter->filter_flag & TKGS_VIDEO_PID_FILTER) == TKGS_VIDEO_PID_FILTER)
		{
			filter->video_pid = ((src[offset]<<8)|src[offset+1])&0x1fff;
			offset += 2;
		}

		if ((filter->filter_flag & TKGS_AUDIO_PID_FILTER) == TKGS_AUDIO_PID_FILTER)
		{
			filter->audio_pid = ((src[offset]<<8)|src[offset+1])&0x1fff;
			offset += 2;
		}

		if ((filter->filter_flag & TKGS_NAME_FILTER) == TKGS_NAME_FILTER)
		{
			uint8_t name_len = 0;
			uint8_t temp_len = 0;
			filter->name_filter.match_type = (src[offset]>>6)&0x03;
			filter->name_filter.case_sensitivity = (src[offset]>>5)&0x01;
			name_len = src[offset+1];
			temp_len = name_len>MAX_PROG_NAME-1?MAX_PROG_NAME-1:name_len;
			strncpy(filter->name_filter.name, (const char *)&src[offset+2], temp_len);
			filter->name_filter.name[temp_len] = '\0';
			offset += 2+name_len;
		}
	}

	return GXCORE_SUCCESS;
}


/*channel_blocking field contains filtering criteria??s for channels that the TKGS data provider doesn??t
allow the users to watch. A blocking defines several filters, if each of the filters matches, the service
should be blocked, the user should not be allowed to watch the service.*/
static status_t _tkgs_channel_blocking_data_process(uint8_t *pBuffIn, uint32_t length)
{
	uint32_t status = GXCORE_ERROR;
	GxTkgsBlocking tkgs_blocking = {0};

	status = _tkgs_channel_blocking_parse(&tkgs_blocking,pBuffIn,length);
	if (status == GXCORE_SUCCESS)
	{
		status = GxBus_TkgsBlockingSave(&tkgs_blocking);
	}

#if 0//test code
{

	GxMsgProperty_NodeByPosGet ProgNode;
	GxTkgsBlockCheckParm param;

	ProgNode.node_type = NODE_PROG;
	ProgNode.pos = g_AppPlayOps.normal_play.play_count;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)&ProgNode);

	param.prog_id = ProgNode.prog_data.id;

	GxBus_TkgsBlockingCheck(&param);

}
#endif
	return status;
}



/*parse tkgs data*/
/*
tkgs_data(){
	tkgs_info()						//specifies global TKGS data that is not relevant to the other fields
	reserved							//4bits
	broadcast_source_loop_length		//20bits
	broadcast_source_loop
	{
		broadcast_source()
	}

	reserved							//4bits
	service_list_loop_length			//20bits
	service_list_loop
	{
		service_list()
	}

	category_loop_length				//16bits
	category_loop
	{
		descriptor()	//multilingual_category_label_descriptor (0x92)
	}

	channel_blocking_loop_length		//16bits
	channel_blocking_loop
	{
		channel_blocking()
	}
}
*/
//TKGS???ݽ???????
status_t gx_tkgs_data_parse(GxTkgsClass *tkgs_class)
{
	TkgsData_s		tkgsData;
	int32_t offset = 0;
	uint32_t nError = GXCORE_ERROR;
	uint8_t *pTkgsData = tkgs_class->tkgs_data;

	if (pTkgsData == NULL)
	{
		TKGS_ERRO_PRINTF("===%s, %d, parm err\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}

	offset = 0;
	tkgsData.infoLen = ((pTkgsData[offset]&0x0F)<<8 | pTkgsData[offset+1]);
	tkgsData.pTkgsInfo = pTkgsData+offset+2;

	offset += (2+tkgsData.infoLen);
	tkgsData.broadcastSourceLen = ((pTkgsData[offset]&0x0F)<<16 | (pTkgsData[offset+1]<<8)| pTkgsData[offset+2]);
	tkgsData.pBroadcastSource = pTkgsData+offset+3;

	offset += (3+tkgsData.broadcastSourceLen);
	tkgsData.serviceListLen = ((pTkgsData[offset]&0x0F)<<16 | (pTkgsData[offset+1]<<8)| pTkgsData[offset+2]);
	tkgsData.pserviceList = pTkgsData+offset+3;

	offset += (3+tkgsData.serviceListLen);
	tkgsData.categoryLen = (pTkgsData[offset]<<8 | pTkgsData[offset+1]);
	tkgsData.pcategory = pTkgsData+offset+2;

	tkgsData.channelBlockingLen = 0;

	if (tkgsData.infoLen > 0)// 1
	{
		TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
		nError = _tkgs_tkgs_info_parse(tkgsData.pTkgsInfo, tkgsData.infoLen, tkgs_class);
		if (nError == GXCORE_ERROR)
		{
			TKGS_ERRO_PRINTF("===%s, %d, parse tkgs info err!\n", __FUNCTION__, __LINE__);
		}
	}

	//?Ƚ???service list, Ȼ???ڽ?????Ŀ?б?ʱ???Ϳ???ֱ?Ӹ?LCN??ֵ
	if ((tkgsData.serviceListLen > 0) && (tkgs_class->search_type == GX_TKGS_UPDATE_ALL))// 3
	{
		TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
		nError = _tkgs_service_list_parse(tkgsData.pserviceList, tkgsData.serviceListLen, tkgs_class);
		if (nError == GXCORE_ERROR)
		{
			TKGS_ERRO_PRINTF("===%s, %d, parse tserviceList err!\n", __FUNCTION__, __LINE__);
		}
	}

	if (tkgsData.broadcastSourceLen > 0)// 2
	{
		TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
		nError = _tkgs_broadcast_sourc_parse(tkgsData.pBroadcastSource, tkgsData.broadcastSourceLen, tkgs_class);
		if (nError == GXCORE_ERROR)
		{
			TKGS_ERRO_PRINTF("===%s, %d, parse BroadcastSource err!\n", __FUNCTION__, __LINE__);
		}
	}

	if ((tkgsData.categoryLen > 0) && (tkgs_class->search_type == GX_TKGS_UPDATE_ALL))// 4
	{
		TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
		nError = _tkgs_category_parse(tkgsData.pcategory, tkgsData.categoryLen);
		if (nError == GXCORE_ERROR)
		{
			TKGS_ERRO_PRINTF("===%s, %d, parse category err!\n", __FUNCTION__, __LINE__);
		}
	}

	if (tkgsData.channelBlockingLen > 0) // 5
	{
		TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
		nError = _tkgs_channel_blocking_data_process(tkgsData.pchannelBlocking, tkgsData.channelBlockingLen);
		if (nError == GXCORE_ERROR)
		{
			TKGS_ERRO_PRINTF("===%s, %d, parse channelBlocking err!\n", __FUNCTION__, __LINE__);
		}
	}

	return nError;

}
#endif

