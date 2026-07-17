/*****************************************************************************
 *		        CONFIDENTIAL
 *        Hangzhou GuoXin Science and Technology Co., Ltd.
 *                      (C)2009, All right reserved
 ******************************************************************************

 ******************************************************************************
 * File Name :	gxepg.c
 * Author    :	shenjch
 * Project   :	GoXceed
 * Type      :
 ******************************************************************************
 * Purpose   :
 ******************************************************************************
 * Release History:
 VERSION	Date			  AUTHOR         Description
 0.0	2014.03.24	      shenjch	         creation
 *****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "gxcore.h"
#include "gxservices.h"
#include "gdi_core.h"
#include "gxatsc_epg_private.h"
#include "sub_system/si_parse_lib/si_parse_lib_table/si_parse_lib_mgt.h"
#include "sub_system/si_parse_lib/si_parse_lib_table/si_parse_lib_atsc_eit.h"
#include "sub_system/si_parse_lib/si_parse_lib_table/si_parse_lib_ett.h"

AtscEpgService_t g_aes;
/* Private Macros --------------------------------------------------------- */
#define GX_ATSC_EPG_DEBUG

#define  GxAtscEpgErr(fmt, args...) \
	do { \
		gxlogd("\n[ATSC EPG ERR][%s():%d]: ", __func__, __LINE__); \
		gxlogd(fmt, ##args); \
	} while(0)


#ifdef GX_ATSC_EPG_DEBUG
#define GxAtscEpgDebug(fmt, args...) \
	do { \
		gxlogd("\n[ATSC EPG DBG][%s():%d]: ", __func__, __LINE__); \
		gxlogd(fmt, ##args); \
	} while(0)
#else
#define GxAtscEpgDebug(fmt, args...)   ((void)0)
#endif


#define CHECK_MEM_ALLOC(mem) do{                                      \
	if (mem == NULL){                                             \
		gxlogd("no mem in %s %d\n",__FUNCTION__, __LINE__);   \
		}else{                                                \
			break;                                        \
		}                                                     \
}while(1)

#define ALL_LANG_STR    "all"
#define ARA_LANG_STR    "ara"
#define UTF8_CODE       (0x15)
#define ARA_CODE_1      (0x02)
#define ARA_CODE_2      (0x06)
#define EIT_OK          (1<<0)
#define ETT_OK          (1<<1)
#define EIT_TABLE_ID    (0xCB)
#define ETT_TABLE_ID    (0xCC)
#define MGT_TABLE_ID    (0xC7)
#define MGT_PID         (0x1FFB)

/* Private Variables ------------------------------------------------------ */

static GX_LIST_HEAD(add_event_list);
static GX_LIST_HEAD(epg_info_list);

/* Private Functions ------------------------------------------------------ */

static int32_t mgt_get(SiEngineHandle si, void *table, SiEngineParseStatus status);

/**
 * @brief epg manage存储满了的默认处理函数,提供处理方式为删除过期epg
 * @param int32_t st:st可以是如下值其中一个或者或值
 * #define SERVICE_FULL_DEFAULT_OPS    (1<<0)
 * #define EVENT_FULL_DEFAULT_OPS      (1<<1)
 * #define POOLMEM_FULL_DEFAULT_OPS    (1<<2)
 * @return void
 */
static void default_manerr_proc(int32_t st)
{
#if 0
	if(st&SERVICE_FULL_DEFAULT_OPS)
	if(st&EVENT_FULL_DEFAULT_OPS)
	if(st&POOLMEM_FULL_DEFAULT_OPS)
#endif

	//GxBus_EpgInvalidInfoClean(0xffffffff,0xffffffff,0xffffffff);
}

/**
 * @brief 将event存储到epg manage,如果func为NULL调用默认处理，不为NULL则用外部注册函数
 * @param GxBusEpgEventAddInfo* event:需要存储的event
 * @return void
 */
static void event_add2_database(GxBusEpgEventAddInfo* event)
{
	status_t database_ret;
	int32_t  func_ret = 0;

	GxAtscEpgDebug("name_length %d detaile_length %d \n",event->name_length,event->detaile_length);
	database_ret= GxBus_EpgInfoAdd(event);
	if(database_ret != GX_BUS_EPG_OK){//数据库满了调用回调,这个失败的数据是否需要继续存储
		if(g_aes.create_info.func !=NULL){
			func_ret = g_aes.create_info.func(database_ret);
			if(func_ret != 0)
				default_manerr_proc(func_ret);
		}else{
			default_manerr_proc(func_ret);
		}
	}
}

/**
 * @brief 将mempool的字符copy到dst
 * @param uint8_t *dst:目标空间指针
 * @return real_len:实际copy的字符串长度
 *         0       :没有字符串拷贝
 */
static uint16_t mempool_string_copy(uint8_t **dst, GxBusEpgMemPoolCell *src)
{
	uint32_t real_len = 0;
	uint8_t *p_content = NULL;

	if(src == NULL){
		GxAtscEpgErr("mempoolcell is NULL !\n");
		goto err;
	}
#ifdef EPG_MALLOC
	p_content = GxCore_Malloc(src->size);
	if(p_content == NULL){
		GxAtscEpgErr("mempoolcell is NULL !\n");
		goto err;
	}
	memcpy(p_content,src->content,src->size);
#else
	while(src != NULL) {
		p_content = GxCore_Realloc(p_content,(real_len+GX_BUS_MEM_POOL_CELL_SIZE-4));
		CHECK_MEM_ALLOC(p_content);
		memcpy(p_content+real_len, src->content, (GX_BUS_MEM_POOL_CELL_SIZE-4));
		real_len += (GX_BUS_MEM_POOL_CELL_SIZE-4);
		src = (GxBusEpgMemPoolCell*)(src->next_Cell);
	}
#endif
	*dst = p_content;
	return real_len;
err:

	*dst = NULL;
	dst = NULL;
	return 0;
}

/**
 * @brief 将传入语言字符串转化为小写比较是否相同
 * @param char* language1:
 *        char* language2:
 * @return -1:不同
 *         0 :相同
 */
static int32_t language_code_cmp(char* language1, char* language2)
{
#define LANGUAGE_LEN    (3)
	uint8_t language_1[LANGUAGE_LEN];
	uint8_t language_2[LANGUAGE_LEN];
	uint16_t i;

	for (i=0; i<LANGUAGE_LEN; i++) {
		language_1[i] = tolower(language1[i]);
		language_2[i] = tolower(language2[i]);
	}

	return memcmp((const void *)language_1, (const void *)language_2, LANGUAGE_LEN);
}

/**
 * @brief 获取某个source_id默认语言的first_event
 * @param GxBusEpgFirstEvent *first_event:某个source_id的first_event
 * @return NULL:没有对应first_event
 *         不为NULL:找到对应的first_event
 */
static GxBusEpgFirstEvent* get_first_event(GxBusEpgFirstEvent *first_event)
{
	char *create_language=(char*)g_aes.create_info.language;
	while(first_event != NULL) {
		if (language_code_cmp((char*)(first_event->language), create_language) == 0)
			break;

		first_event = first_event->next;
	}

	return first_event;
}

/**
 * @brief 将GxBusEpgEventHead 结构的数据转化为GxEpgEventInfo 
 * @param GxBusEpgEventHead: 参考gxepg_manage.h
 *        GxEpgEventInfo:参考gxepg_manage.h
 * @return void
 */
static void get_event(GxEpgEventInfo *event_info, GxBusEpgEventHead *event_head)
{
	event_info->event_id = event_head->event_id;
	event_info->start_time = event_head->time.start_time;
	event_info->duration = event_head->time.duration;
	event_info->event_name_len =
		mempool_string_copy(&(event_info->event_name), event_head->name);

	event_info->event_detail_len =
		mempool_string_copy(&(event_info->event_detail), event_head->detaile_description);
}

/**
 * @brief 释放GxEpgEventInfo的name和detaile,在Free的时候调用释放在get申请的内存
 * @param GxEpgEventInfo:参考gxepg_manage.h
 * 	  count:有多少个event_info
 * @return void
 */
static void free_event_mem(GxEpgEventInfo *event_info,uint16_t count)
{
	uint32_t i;

	for (i=0; i<count; i++) {
		if(event_info[i].event_name)
			GxCore_Free(event_info[i].event_name);
		if(event_info[i].event_detail)
			GxCore_Free(event_info[i].event_detail);
	}
	GxCore_Free(event_info);
}

/**
 * @brief 所有psip的timeout回调
 * @param SiEngineHandle:对应si引擎会传入句柄
 * @return 0:都返回0
 */
static int32_t mgt_timeout(SiEngineHandle si)
{
	GxAtscEpgDebug("timeout callback!\n");
    GxCore_MutexLock(g_aes.mutex);
    GxAtscEpgDebug("mgt si : 0x%x !\n",si);
    g_aes.mgt_info.si=-1;
    GxSubSystem_SiEngineFree(si);
    GxMessage *new_msg = NULL;
    new_msg = GxBus_MessageNew(GXMSG_ATSC_EPG_END);
    GxBus_MessageSend(new_msg);
    GxCore_MutexUnlock(g_aes.mutex);
    return 0;
}

/**
 * @brief psip过滤通道的创建
 * @param PsipInfo_t :过滤参数具体参看gxatsc_epg_private.h
 *        bool:是否是修改参数继续过滤
 * @return 0:创建成功
 *        -1:创建失败
 */
static int32_t psip_channel_create(PsipInfo_t *psip_info,bool modify)
{
	GxSubsystemSiEngineAlloc para = {0};

	para.dmx_id = g_aes.create_info.dmx_id;
	para.ts = g_aes.create_info.ts_src;
	para.filter.type = GX_SUB_DMX_FILTER_TYPE_PSI;
	para.filter.pid = psip_info->pid;
	para.filter.match_depth = 1;
	para.filter.eq_or_neq = psip_info->eq_or_neq;
	para.filter.crc = SUBSYSTEM_DMX_CRC_ON;
	para.filter.soft_filter = SUBSYSTEM_DMX_SOFT_OFF;
	memset(para.filter.mask,0x00,18);
	para.filter.match[0] = psip_info->tid;
	para.filter.mask[0] = 0xff;
	if(para.filter.eq_or_neq ==  SUBSYSTEM_DMX_NEQ_MATCH){
		para.filter.match_depth = 6;
        para.filter.match[5] = ((psip_info->version_number)&0x1f)<<1;
		GxAtscEpgDebug("neq version_number : 0x%x !\n",psip_info->version_number);
		para.filter.mask[5] = 0x3e;
	}

	para.original = NULL;
	para.table = psip_info->table;
	para.time_out = psip_info->timeout;
	para.sec_num_check = NULL;
	para.time_ms = 5000;
    para.p = psip_info;

	if(modify){
		GxSubSystem_SiEngineModify(psip_info->si,&para);
	}else{
		psip_info->si = GxSubSystem_SiEngineAlloc(&para);
		if(psip_info->si == -1){
			return -1;
		} else{
			GxSubSystem_SiEngineStart(psip_info->si);
		}
	}

    return 0;
}
static int start_mgt_monitor(PsipInfo_t *mgt_info)
{
        mgt_info->pid = MGT_PID;
        mgt_info->tid = MGT_TABLE_ID;
        mgt_info->eq_or_neq = SUBSYSTEM_DMX_NEQ_MATCH;
        mgt_info->table = mgt_get;
        mgt_info->timeout = mgt_timeout;
        psip_channel_create(mgt_info,0);
	return 0;
}

/**
 * @brief 释放临时的GxBusEpgEventAddInfo空间
 * @param GxBusEpgEventAddInfo:过滤参数具体参看gxepg_manage.h
 * @return void
 */
static void free_info_mem(GxBusEpgEventAddInfo *event)
{
	if (event->name != NULL) {
		GxCore_Free(event->name);
		event->name= NULL;
	}

	if (event->detaile_description != NULL) {
		GxCore_Free(event->detaile_description);
		event->detaile_description= NULL;
	}

}

static int si_reuse(SiEngineHandle si)
{
    PsipInfo_t *eit_info = g_aes.eit_info;
    PsipInfo_t *ett_info = g_aes.ett_info;
    if(g_aes.eit_start_count<g_aes.eit_count){
        eit_info[g_aes.eit_start_count].si=si;
        psip_channel_create(&eit_info[g_aes.eit_start_count],1);
        g_aes.eit_start_count++;
    }else if(g_aes.ett_start_count<g_aes.ett_count){
        ett_info[g_aes.ett_start_count].si=si;
        psip_channel_create(&ett_info[g_aes.ett_start_count],1);
        g_aes.ett_start_count++;
    }else{
        GxSubSystem_SiEngineFree(si);
    }
    return 0;
}
static int32_t eit_timeout(SiEngineHandle si)
{
	PsipInfo_t *eit_info = NULL;
	PsipInfo_t *old_psip_info = NULL;
	EventList_t *event_list =NULL;
	EventList_t *safe =NULL;
	PsipInfo_t *mgt_info = &(g_aes.mgt_info);
	GxCore_MutexLock(g_aes.mutex);
	if(g_aes.init == false || ((g_aes.one_shot_flag & EIT_OK) != 0)){
		GxCore_MutexUnlock(g_aes.mutex);
		return 0;
	}
	//这里可以modify参数把没有过滤的eit继续过滤,用finish_count
	g_aes.eit_finish_count++;
	eit_info = g_aes.eit_info;
	old_psip_info = (PsipInfo_t *)GxSubSystem_SiEngineGetUser(si);
	CHECK_MEM_ALLOC(old_psip_info);
	old_psip_info->si=-1;
		//因为所有的eit都开始过滤了，到了table_ok,那么可以释放si
    si_reuse(si);

	if( g_aes.eit_finish_count == g_aes.eit_count){
		g_aes.one_shot_flag |=EIT_OK;
		GxCore_Free(eit_info);
		eit_info = NULL;
		g_aes.eit_info = NULL;
		g_aes.eit_count = 0;
		g_aes.eit_start_count = 0;
		g_aes.eit_finish_count = 0;
	}
	GxAtscEpgDebug("total  %d start %d  finish %d \n",g_aes.eit_count,g_aes.eit_start_count,g_aes.eit_finish_count);
	//如果eit和ett都过滤完成进行最后一次把info队列都add
	if(g_aes.one_shot_flag == (EIT_OK|ETT_OK)){
		gxlist_for_each_entry_safe(event_list, safe, &add_event_list, list_head){
			event_add2_database(event_list->event);
			free_info_mem(event_list->event);
			GxCore_Free(event_list->event);
			gxlist_del(&(event_list->list_head));
			GxCore_Free(event_list);
		}
		//需要发end消息,开启监控
		GxAtscEpgDebug("========================atsc epg finish\n");
		GxMessage *new_msg = NULL;
		new_msg = GxBus_MessageNew(GXMSG_ATSC_EPG_END);
		GxBus_MessageSend(new_msg);

        start_mgt_monitor(mgt_info);
		//psip_channel_create(mgt_info,0);

	}
	GxCore_MutexUnlock(g_aes.mutex);
	return 0;
}

static int32_t ett_timeout(SiEngineHandle si)
{
	PsipInfo_t *ett_info = NULL;
	PsipInfo_t *old_psip_info = NULL;
	EventList_t *event_list =NULL;
	EventList_t *safe =NULL;
	PsipInfo_t *mgt_info = &(g_aes.mgt_info);
	GxCore_MutexLock(g_aes.mutex);

	if(g_aes.init == false || ((g_aes.one_shot_flag & ETT_OK) != 0)){
		GxCore_MutexUnlock(g_aes.mutex);
		return 0;
	}
	//这里可以modify参数把没有过滤的ett继续过滤,用finish_count
	g_aes.ett_finish_count++;
	ett_info = g_aes.ett_info;
	old_psip_info = (PsipInfo_t *)GxSubSystem_SiEngineGetUser(si);
	CHECK_MEM_ALLOC(old_psip_info);
	old_psip_info->si=-1;
		//因为所有的ett都开始过滤了，到了table_ok,那么可以释放si
    si_reuse(si);

	if( g_aes.ett_finish_count == g_aes.ett_count){
		g_aes.one_shot_flag |=ETT_OK;
		GxCore_Free(ett_info);
		ett_info = NULL;
		g_aes.ett_info = NULL;
		g_aes.ett_count = 0;
		g_aes.ett_start_count = 0;
		g_aes.ett_finish_count = 0;
	}

	GxAtscEpgDebug("total  %d start %d  finish %d \n",g_aes.ett_count,g_aes.ett_start_count,g_aes.ett_finish_count);
	//如果ett和ett都过滤完成进行最后一次把info队列都add
	if(g_aes.one_shot_flag == (EIT_OK|ETT_OK)){
		gxlist_for_each_entry_safe(event_list,safe,&add_event_list, list_head){
			event_add2_database(event_list->event);
			free_info_mem(event_list->event);
			GxCore_Free(event_list->event);
			gxlist_del(&(event_list->list_head));
			GxCore_Free(event_list);
		}
		//需要发end消息,开启监控
		GxAtscEpgDebug("========================atsc epg finish\n");
		GxMessage *new_msg = NULL;
		new_msg = GxBus_MessageNew(GXMSG_ATSC_EPG_END);
		GxBus_MessageSend(new_msg);

        start_mgt_monitor(mgt_info);
		//psip_channel_create(mgt_info,0);

	}
	GxCore_MutexUnlock(g_aes.mutex);
	return 0;
}
/**
 * @brief 拼接长短描述字符串
 * @param GxBusEpgEventAddInfo:过滤参数具体参看gxepg_manage.h
 * @return 拼接完成的完整结构
 */
static  GxBusEpgEventAddInfo* merge_description(GxBusEpgEventAddInfo *short_event, GxBusEpgEventAddInfo *extern_event)
{
	uint8_t *description = short_event->detaile_description;
	uint8_t code[1];
	code[0] = '@';
	if(extern_event->detaile_length != 0){
		description = GxCore_Realloc(description,short_event->detaile_length+extern_event->detaile_length+1);
		CHECK_MEM_ALLOC(description);
		memcpy((description+short_event->detaile_length),code,1);
		memcpy((description+short_event->detaile_length+1),extern_event->detaile_description,extern_event->detaile_length);
	}
	short_event->detaile_description = description;
	return short_event;
}

/**
 * @brief 通过iso和字符来判断是否为阿拉伯字符
 * @param src_buf:原始字符
 *        iso_str:iso language
 * @return true:是阿拉伯字符
 *         false:不是阿拉伯字符
 */
static bool _check_arabic_text(uint8_t *src_buf, uint8_t *iso_str)
{
	if((src_buf == NULL) && (iso_str == NULL))
		return false;

	if((iso_str != NULL) && (language_code_cmp((char*)iso_str, (char*)ARA_LANG_STR) == 0))
		return true;

	if((src_buf != NULL)
			&& ((src_buf[0] == ARA_CODE_1) ||(src_buf[0] == ARA_CODE_2)))
		return true;

	return false;
}

static int32_t epg_event_non_standard_proc(uint8_t *src_buf , uint32_t src_len, uint8_t **res_buf,
		uint32_t *res_len, uint8_t *iso_str, uint16_t txt_format)
{
#define GUI_ARA_CODE (0x02)

	if((src_buf == NULL) || (src_len == 0) || (res_buf == NULL) || (res_len == NULL))
		return -1;

	if((iso_str != NULL) && (language_code_cmp((char*)iso_str, (char*)ARA_LANG_STR) == 0)) {
		if((src_buf[0] != ARA_CODE_1) && (src_buf[0] != ARA_CODE_2)&& (src_buf[0] != UTF8_CODE)) { //non-standard
			*res_buf = (uint8_t*)GxCore_Calloc(1, src_len + 1);
			if(*res_buf != NULL) {
				(txt_format == EPG_TXT_UTF8) ? ((*res_buf[0]) = ARA_CODE_2) : ((*res_buf)[0] = GUI_ARA_CODE);
				memcpy(*res_buf + 1, src_buf, src_len);
				*res_len = src_len + 1;
				return 0;
			}
		} else {
			*res_buf = (uint8_t*)GxCore_Calloc(1, src_len);
			if(*res_buf != NULL) {
				memcpy(*res_buf, src_buf, src_len);
				*res_len = src_len;
				return 0;
			}
		}
	} else {
		*res_buf = (uint8_t*)GxCore_Calloc(1, src_len);
		if(*res_buf != NULL) {
			memcpy(*res_buf, src_buf, src_len);
			*res_len = src_len;
			return 0;
		}
	}

	return -1;
}

/**
 * @brief 解析extended_text_message结构,name,detail,channel_ett都是这个结构
 * @param language:需要解析哪种语言的数据
 *        text:存储空间
 *        mssstruct:原始数据
 * @return true:是阿拉伯字符
 *         false:不是阿拉伯字符
 */
static uint32_t parse_extended_text_message(int8_t *language,uint8_t **text,
		GxSubsystemSiParseMSSstruct * mssstruct)
{
#define LANGUAGE_CODE_LEN    (3)
	uint32_t i,j;
	uint8_t *temp_buf = NULL;
	uint32_t temp_len = 0;
	uint32_t len = 0;
	uint32_t number_string;
	uint32_t number_segments;
	bool ara_flag = false;
	GxSubsystemSiParseMssnumberStrings *string_info;
	GxSubsystemSiParseMssnumberSegments *segments_info;
	char *create_language=(char*)g_aes.create_info.language;
	uint16_t create_txt_format=g_aes.create_info.txt_format;
	unsigned char *text_temp = NULL;

	number_string = mssstruct->number_string;
	string_info = mssstruct->string_info;
	for(i=0;i<number_string;i++) {
		if(language_code_cmp((char*)ALL_LANG_STR, create_language) == 0) {
			//bugfix:设置为all语言就把所有的都存储
			memcpy(language, ALL_LANG_STR, LANGUAGE_CODE_LEN);
		}else{
			if (language_code_cmp((char*)(string_info->ISO_639_language_code),
						create_language) == 0) {
				//按照正常的语言存储
				memcpy(language, string_info->ISO_639_language_code, LANGUAGE_CODE_LEN); 
			}else{
				string_info++;
				continue;
			}
		}
		number_segments = string_info->number_segments;
		segments_info = string_info->segments_info;
		for(j=0;j<number_segments;j++){
			uint8_t *iso_str = NULL;
			ara_flag = _check_arabic_text(segments_info->string_byte, string_info->ISO_639_language_code);
			(ara_flag == true) ? (iso_str = (uint8_t*)ARA_LANG_STR) : (iso_str = (uint8_t*)language);

			//需要多个部分的名字拼接最后存储
			if((create_txt_format == EPG_TXT_UTF8) || (ara_flag == true)) {//convert to utf8
				if(epg_event_non_standard_proc(segments_info->string_byte, segments_info->number_bytes,
							&temp_buf, &temp_len, (uint8_t*)iso_str, EPG_TXT_UTF8) == 0) {
					if(gxgdi_iconv(temp_buf, &text_temp, temp_len, &len) == 1) {
						text_temp = NULL;
						len = 0;
					}
				}
			} else {
				if(epg_event_non_standard_proc(segments_info->string_byte, segments_info->number_bytes,
							&temp_buf, &temp_len, (uint8_t*)iso_str, EPG_TXT_ORIGINAL) == 0) {
					text_temp = (uint8_t*)GxCore_Calloc(1, temp_len + 1);
					if(text_temp != NULL) {
						memcpy(text_temp, temp_buf, temp_len);
						len = temp_len;
					}
				}
			}
			*text = text_temp;
			if(temp_buf != NULL){
				GxCore_Free(temp_buf);
				temp_buf= NULL;
				temp_len = 0;
			}

			segments_info++;
		}
		string_info++;
	}
	return len;
}

/**
 * @brief eit的table_ok处理函数
 */

static int32_t eit_get(SiEngineHandle si, void *table, SiEngineParseStatus status)
{
	GxSubsystemSiParseAtscEIT* eit = (GxSubsystemSiParseAtscEIT*)table;
	GxSubsystemSiParseAtscEITLoop *loop;
	GxSubsystemSiParseMSSstruct *title_text=NULL;
	GxBusEpgEventAddInfo *event=NULL;
	GxBusEpgEventAddInfo *event_merge=NULL;
	GxBusEpgEventTime event_time;
	PsipInfo_t *eit_info = NULL;
	PsipInfo_t *old_psip_info = NULL;
	EventList_t *event_list =NULL;
	EventList_t *safe =NULL;
	PsipInfo_t *mgt_info = &(g_aes.mgt_info);
	uint32_t num_events_in_section;
	uint32_t i = 0;
	uint32_t ETM_location;

	GxCore_MutexLock(g_aes.mutex);
	if(g_aes.init == false || ((g_aes.one_shot_flag & EIT_OK) != 0)){
		GxCore_MutexUnlock(g_aes.mutex);
		return 0;
	}

	if(eit->table_id != EIT_TABLE_ID){
        GxAtscEpgErr("old si data in\n");
		GxCore_MutexUnlock(g_aes.mutex);
		return 0;
	}

	num_events_in_section = eit->num_events_in_section;

	loop = eit->loop;
	for (i=0; i<num_events_in_section; i++) {
		event = GxCore_Mallocz(sizeof(GxBusEpgEventAddInfo));
		CHECK_MEM_ALLOC(event);
		event->service_id = eit->source_id;
		event->event_id = loop->event_id;
		event->type = GX_EVENT_DETAILE;
		event->event_time = &event_time;
		event->event_time->start_time = loop->start_time;
		event->event_time->duration= loop->length_in_seconds;

		memset(event->language, 0, sizeof(event->language));

		if (loop->title_length > 0) { //解析name
			title_text = &(loop->title_text);
			event->name_length=parse_extended_text_message(event->language,
					&(event->name),title_text);
		}
		//短描述的结构是否就用rating就可以了

		//判断是否有ETT
		ETM_location = loop->ETM_location;
		if(ETM_location == 0 || ETM_location == 2){//0表示没有ETT，2表示ETT在这个event里面,可以直接存储数据库
			event_add2_database(event);
			free_info_mem(event);
			GxCore_Free(event);
			event = NULL ;
		} else if(ETM_location == 1){//1表示需要ETT,记录到全局列表里面等待ETT过滤完成存储
			GxAtscEpgDebug("~~~~~~~~~~~~~~~~~~~~~~");
			//记录到全局的event里面，先找是否已经存在，如果
			//有肯定是ett，不可能是相同event_id的eit,然后拼接存储
			gxlist_for_each_entry_safe(event_list, safe, &add_event_list, list_head){
				if(event->event_id == event_list->event->event_id){
					//拼接描述字段
					event_merge = merge_description(event,event_list->event);
					event_add2_database(event_merge);
					free_info_mem(event_list->event);
					GxCore_Free(event_list->event);
					event_list->event = NULL;
					gxlist_del(&(event_list->list_head));
					GxCore_Free(event_list);
					event_list =(EventList_t *) 0xffffffff;
					GxCore_Free(event);
					event = NULL;
					break;
				}
			}
            if(event_list != (EventList_t *)0xffffffff){
                if(g_aes.one_shot_flag == ETT_OK){
                    event_add2_database(event);
                    free_info_mem(event);
                    GxCore_Free(event);
                    event = NULL ;
                }else{

			event_list = GxCore_Malloc(sizeof(EventList_t));
			CHECK_MEM_ALLOC(event_list);
			event_list->event=event;
			gxlist_add_tail(&(event_list->list_head), &add_event_list);
                }
            }
			GxAtscEpgDebug("**********************");
		}
		loop++;
	}

	if(status == TABLE_OK) {
		//这里可以modify参数把没有过滤的eit继续过滤,用finish_count
		g_aes.eit_finish_count++;
		eit_info = g_aes.eit_info;
		old_psip_info = (PsipInfo_t *)GxSubSystem_SiEngineGetUser(si);
		CHECK_MEM_ALLOC(old_psip_info);
		old_psip_info->si=-1;
			//因为所有的eit都开始过滤了，到了table_ok,那么可以释放si
        si_reuse(si);

		if( g_aes.eit_finish_count == g_aes.eit_count){
			g_aes.one_shot_flag |=EIT_OK;
			GxCore_Free(eit_info);
			eit_info = NULL;
			g_aes.eit_info = NULL;
			g_aes.eit_count = 0;
			g_aes.eit_start_count = 0;
			g_aes.eit_finish_count = 0;
		}
		GxAtscEpgDebug("total  %d start %d  finish %d \n",g_aes.eit_count,g_aes.eit_start_count,g_aes.eit_finish_count);
		//如果eit和ett都过滤完成进行最后一次把info队列都add
		if(g_aes.one_shot_flag == (EIT_OK|ETT_OK)){
			gxlist_for_each_entry_safe(event_list, safe, &add_event_list, list_head){
				event_add2_database(event_list->event);
				free_info_mem(event_list->event);
				GxCore_Free(event_list->event);
				event_list->event = NULL;
				gxlist_del(&(event_list->list_head));
				GxCore_Free(event_list);
				event_list = NULL;
			}
		//需要发end消息,开启监控
			GxAtscEpgDebug("========================atsc epg finish\n");
			GxMessage *new_msg = NULL;
			new_msg = GxBus_MessageNew(GXMSG_ATSC_EPG_END);
			GxBus_MessageSend(new_msg);

            start_mgt_monitor(mgt_info);
			//psip_channel_create(mgt_info,0);

		}
		GxCore_MutexUnlock(g_aes.mutex);
		return 0;
	} else {
		GxCore_MutexUnlock(g_aes.mutex);
		return 1;
	}
}

/**
 * @brief ett的table_ok处理函数
 */
static int32_t ett_get(SiEngineHandle si, void *table, SiEngineParseStatus status)
{
	GxSubsystemSiParseETT* ett = (GxSubsystemSiParseETT*)table;
	GxSubsystemSiParseMSSstruct *extended_text_message;
	GxBusEpgEventAddInfo *event=NULL;
	GxBusEpgEventAddInfo *event_merge=NULL;
	GxBusEpgEventTime event_time;
	PsipInfo_t *ett_info = NULL;
	PsipInfo_t *old_psip_info = NULL;
	EventList_t *event_list =NULL;
	EventList_t *safe =NULL;
	PsipInfo_t *mgt_info = &(g_aes.mgt_info);
	uint32_t ETM_id,source_id,event_id;

	GxCore_MutexLock(g_aes.mutex);
	if(g_aes.init == false || ((g_aes.one_shot_flag & ETT_OK) != 0)){
		GxCore_MutexUnlock(g_aes.mutex);
		return 0;
	}

	if(ett->table_id != ETT_TABLE_ID){
		GxCore_MutexUnlock(g_aes.mutex);
		return 0;
	}

	event = GxCore_Mallocz(sizeof(GxBusEpgEventAddInfo));
	CHECK_MEM_ALLOC(event);

	ETM_id = ett->ETM_id;
	extended_text_message = &(ett->extended_text_message);
	source_id = (ETM_id>>15)&0xff;
	event->service_id = source_id;
	event_id  = (ETM_id>> 2)&0x3f;
	event->event_id = event_id;
	event->type = GX_EVENT_DETAILE;
	event->event_time = &event_time;
	if((ETM_id&0x3) == 0){//channel ETM
		event->channel_ett_num = parse_extended_text_message(event->channel_ett->language,
				(uint8_t **)&(event->channel_ett->info),extended_text_message);

		event_add2_database(event);
		free_info_mem(event);
		GxCore_Free(event);
		event = NULL;
	}else if((ETM_id&0x3) == 2){//event ETM

		event->detaile_length=parse_extended_text_message(event->language,
				&(event->detaile_description),extended_text_message);

		//记录到全局的event里面，先找是否已经存在，如果
		//有肯定是ett，不可能是相同event_id的eit,然后拼接存储
		gxlist_for_each_entry_safe(event_list, safe, &add_event_list, list_head){
			if(event->event_id == event_list->event->event_id){
				event_merge = merge_description(event_list->event,event);
				event_add2_database(event_merge);
				free_info_mem(event_list->event);
				GxCore_Free(event_list->event);
				event_list->event = NULL;
				gxlist_del(&(event_list->list_head));
				GxCore_Free(event_list);
				event_list = NULL;
				GxCore_Free(event);
				event = NULL;
				goto out;
			}
		}

        if(g_aes.one_shot_flag == EIT_OK){
            event_add2_database(event);
            free_info_mem(event);
            GxCore_Free(event);
            event = NULL ;
        }else{
		event_list = GxCore_Malloc(sizeof(EventList_t));
		CHECK_MEM_ALLOC(event_list);
		event_list->event=event;
		gxlist_add_tail (&(event_list->list_head), &add_event_list);
        }

	}
out:

	if(status == TABLE_OK) {
		//这里可以modify参数把没有过滤的ett继续过滤,用finish_count
		g_aes.ett_finish_count++;
		ett_info = g_aes.ett_info;
		old_psip_info = (PsipInfo_t *)GxSubSystem_SiEngineGetUser(si);
		CHECK_MEM_ALLOC(old_psip_info);
		old_psip_info->si=-1;
			//因为所有的ett都开始过滤了，到了table_ok,那么可以释放si
        si_reuse(si);

		if( g_aes.ett_finish_count == g_aes.ett_count){
			g_aes.one_shot_flag |=ETT_OK;
			GxCore_Free(ett_info);
			ett_info = NULL;
			g_aes.ett_info = NULL;
			g_aes.ett_count = 0;
			g_aes.ett_start_count = 0;
			g_aes.ett_finish_count = 0;
		}

		GxAtscEpgDebug("total  %d start %d  finish %d \n",g_aes.ett_count,g_aes.ett_start_count,g_aes.ett_finish_count);
		//如果ett和ett都过滤完成进行最后一次把info队列都add
		if(g_aes.one_shot_flag == (EIT_OK|ETT_OK)){
			gxlist_for_each_entry_safe(event_list, safe, &add_event_list, list_head){
				event_add2_database(event_list->event);
				free_info_mem(event_list->event);
				GxCore_Free(event_list->event);
				event_list->event = NULL;
				gxlist_del(&(event_list->list_head));
				GxCore_Free(event_list);
				event_list = NULL;
			}
		//需要发end消息,开启监控
			GxAtscEpgDebug("========================atsc epg finish\n");
			GxMessage *new_msg = NULL;
			new_msg = GxBus_MessageNew(GXMSG_ATSC_EPG_END);
			GxBus_MessageSend(new_msg);

            start_mgt_monitor(mgt_info);
			//psip_channel_create(mgt_info,0);

		}
		GxCore_MutexUnlock(g_aes.mutex);
		return 0;
	} else {
		GxCore_MutexUnlock(g_aes.mutex);
		return 1;
	}
}

/**
 * @brief mgt的table_ok处理函数
 */
static int32_t mgt_get(SiEngineHandle si, void *table, SiEngineParseStatus status)
{
	GxSubsystemSiParseMGT* mgt = (GxSubsystemSiParseMGT*)table;
	GxSubsystemSiParseMGTLoop *loop;
	PsipInfo_t *psip_info = NULL;
	PsipInfo_t *eit_info = NULL;
	PsipInfo_t *ett_info = NULL;
	uint32_t i,table_type,tables_defined;

	GxCore_MutexLock(g_aes.mutex);
	if(g_aes.init == false){
		GxCore_MutexUnlock(g_aes.mutex);
		return 0;
	}

	psip_info = (PsipInfo_t *)GxSubSystem_SiEngineGetUser(si);
	CHECK_MEM_ALLOC(psip_info);
	if(mgt->table_id != MGT_TABLE_ID){
		GxCore_MutexUnlock(g_aes.mutex);
		return 0;
	}
		
	psip_info->version_number= mgt->version_number;
	GxAtscEpgDebug("eq version_number : 0x%x !\n",mgt->version_number);
	tables_defined = mgt->tables_defined;
	if(tables_defined == 0) 
		goto out;

	loop = mgt->loop;
	for(i=0;i<tables_defined;i++){
		table_type = loop->table_type;
		if(table_type>=0x0100 && table_type<=0x17f){//根据table_type判断eit的数据
			g_aes.eit_count++;
			eit_info = GxCore_Realloc(eit_info,(g_aes.eit_count)*sizeof(PsipInfo_t));
			CHECK_MEM_ALLOC(eit_info);
			eit_info[g_aes.eit_count-1].pid = loop->table_type_PID;
			eit_info[g_aes.eit_count-1].tid = EIT_TABLE_ID;
			eit_info[g_aes.eit_count-1].eq_or_neq = SUBSYSTEM_DMX_EQ_MATCH;
			eit_info[g_aes.eit_count-1].table = eit_get;
			eit_info[g_aes.eit_count-1].timeout = eit_timeout;
            eit_info[g_aes.eit_count-1].si = -1;
		}else if(table_type>=0x0200 && table_type<=0x27f){//根据table_type判断ett的数据
			g_aes.ett_count++;
			ett_info = GxCore_Realloc(ett_info,(g_aes.ett_count)*sizeof(PsipInfo_t));
			CHECK_MEM_ALLOC(ett_info);
			ett_info[g_aes.ett_count-1].pid = loop->table_type_PID;
			ett_info[g_aes.ett_count-1].tid = ETT_TABLE_ID;
			ett_info[g_aes.ett_count-1].eq_or_neq = SUBSYSTEM_DMX_EQ_MATCH;
			ett_info[g_aes.ett_count-1].table = ett_get;
			ett_info[g_aes.ett_count-1].timeout = ett_timeout;
            ett_info[g_aes.ett_count-1].si = -1;
		}
		loop++;
	}


	//g_aes.ett_count = 0;
	//g_aes.eit_count = 0;
	g_aes.eit_info=eit_info;
	g_aes.ett_info=ett_info;
	GxAtscEpgDebug("eit_count %d ett_count %d \n",g_aes.eit_count,g_aes.ett_count);
	if(g_aes.eit_count == 0)
		g_aes.one_shot_flag |=EIT_OK;
	if(g_aes.ett_count == 0)
		g_aes.one_shot_flag |=ETT_OK;
out:
	if(status == TABLE_OK) {
		i=0;
		//同时申请eit和ett，有多少申请多少，保证尽量多的过滤数据，通过
		//start_count记录有多少pid开始过滤
		while(1){
			if(i<g_aes.eit_count){
                if(i==0){
                    eit_info[0].si=g_aes.mgt_info.si;
                    if(psip_channel_create(&eit_info[0],1)==0){
                        g_aes.eit_start_count++;
                        si=-1;
                    }
                }else {
				if(psip_channel_create(&eit_info[i],0)<0)
					break;
				g_aes.eit_start_count++;
			}
            }

			if(i<g_aes.ett_count){
                if(i==0 && si != -1){
                    ett_info[0].si=g_aes.mgt_info.si;
                    if(psip_channel_create(&ett_info[0],1)==0){
                        g_aes.ett_start_count++;
                        si=-1;
                    }
                }else {
				if(psip_channel_create(&ett_info[i],0)<0)
					break;
				g_aes.ett_start_count++;
			}
            }

			i++;
			if(i>=GX_MAX(g_aes.eit_count,g_aes.ett_count))
				break;
		}
        if(si != -1){
            GxAtscEpgDebug("release si 0x%x\n",si);
            GxSubSystem_SiEngineFree(si);
        }
        g_aes.mgt_info.si=-1;

        GxMessage *new_msg = NULL;
        new_msg = GxBus_MessageNew(GXMSG_ATSC_EPG_BEGIN);
        GxBus_MessageSend(new_msg);
		GxCore_MutexUnlock(g_aes.mutex);
		return 0;
	} else {
		GxCore_MutexUnlock(g_aes.mutex);
		return 1;
	}
}

/* Export Functions ------------------------------------------------------ */
void service_epg_create(GxMsgProperty_AtscEpgCreate *epg_create)
{
	GxMsgProperty_AtscEpgCreate *create_info = &(g_aes.create_info);
	PsipInfo_t  *mgt_info = &(g_aes.mgt_info);
	status_t ret;

	GxCore_MutexLock(g_aes.mutex);
	if(g_aes.init == true)
		goto out;

	g_aes.init=true;
	g_aes.one_shot_flag = 0;
	create_info->service_count = 400;
	create_info->event_count = 2000;
	create_info->event_size = 100;
	create_info->epg_day = 3;
	create_info->language[0] = 'e';
	create_info->language[1] = 'n';
	create_info->language[2] = 'g';
	create_info->txt_format = EPG_TXT_ORIGINAL;
	create_info->func = NULL;

	if (epg_create->service_count != 0
			&& epg_create->event_count != 0
			&& epg_create->event_size != 0
			&& epg_create->epg_day != 0) {
		memcpy(create_info, epg_create, sizeof(GxMsgProperty_AtscEpgCreate));
	}

	ret = GxBus_EpgInit(create_info->service_count, create_info->event_count,
			create_info->event_size, create_info->epg_day);
	if (ret != GX_BUS_EPG_OK) {
		GxAtscEpgErr("atsc epg manage init failed!\n");
		goto out;
	}

	mgt_info->pid = MGT_PID;
	mgt_info->tid = MGT_TABLE_ID;
	mgt_info->eq_or_neq = SUBSYSTEM_DMX_EQ_MATCH;
	mgt_info->table = mgt_get;
	mgt_info->timeout = mgt_timeout;
	mgt_info->p = (void *)mgt_info;
	psip_channel_create(mgt_info,0);

out:
	GxCore_MutexUnlock(g_aes.mutex);

}

void service_epg_clean(void)
{
	GxCore_MutexLock(g_aes.mutex);
    if(g_aes.init == false){
        GxCore_MutexUnlock(g_aes.mutex);
        return;
    }
	if (GxBus_EpgClean() != GX_BUS_EPG_OK)
		GxAtscEpgErr("atsc epg manage clean err!\n");

	GxCore_MutexUnlock(g_aes.mutex);
}

void service_epg_release(void)
{
	uint16_t i;
	PsipInfo_t *eit_info = NULL;
	PsipInfo_t *ett_info = NULL;
	PsipInfo_t *mgt_info = NULL;
	EventList_t *event_list =NULL;
	EventList_t *safe =NULL;
    EpgInfoList_t *epg_info =NULL;
    EpgInfoList_t *safe_info =NULL;

	GxCore_MutexLock(g_aes.mutex);
    if(g_aes.init != true){
        GxCore_MutexUnlock(g_aes.mutex);
        return;
    }
	eit_info = g_aes.eit_info;
	ett_info = g_aes.ett_info;
	mgt_info = &(g_aes.mgt_info);


	if(eit_info != NULL){
		if(eit_info != g_aes.eit_info )
			while(1);
		for(i=0;i<g_aes.eit_count;i++){
			if(eit_info[i].si != -1){
				GxSubSystem_SiEngineFree(eit_info[i].si);
				eit_info[i].si = -1;
			}
		}
		GxCore_Free(eit_info);
		eit_info = NULL;
		g_aes.eit_info = NULL;
		g_aes.eit_count = 0;
		g_aes.eit_start_count = 0;
		g_aes.eit_finish_count = 0;
	}

	if(mgt_info->si != -1){
		GxSubSystem_SiEngineFree(mgt_info->si);
		mgt_info->si = -1;
	}

	if(ett_info != NULL){
		for(i=0;i<g_aes.ett_count;i++){
			if(ett_info[i].si != -1 ){
				GxSubSystem_SiEngineFree(ett_info[i].si);
				ett_info[i].si = -1;
			}
		}
		GxCore_Free(ett_info);
		ett_info = NULL;
		g_aes.ett_info = NULL;
		g_aes.ett_count = 0;
		g_aes.ett_start_count = 0;
		g_aes.ett_finish_count = 0;
	}

	if(!gxlist_empty(&add_event_list)){
		gxlist_for_each_entry_safe(event_list, safe, &add_event_list, list_head){
			if(event_list->event!=NULL){
				free_info_mem(event_list->event);
				GxCore_Free(event_list->event);
				event_list->event = NULL;
				gxlist_del(&(event_list->list_head));
				GxCore_Free(event_list);
				event_list = NULL;
			}
		}
	}
    if(!gxlist_empty(&epg_info_list)){
        gxlist_for_each_entry_safe(epg_info, safe_info, &epg_info_list, list_head){
            if(epg_info->epg_info !=NULL && epg_info->real_count !=0 ){
                free_event_mem(epg_info->epg_info,epg_info->real_count);
                gxlist_del(&(epg_info->list_head));
                GxCore_Free(epg_info);
            }
        }
    }
	g_aes.init=false;

	if (GxBus_EpgRelease() != GX_BUS_EPG_OK)
		GxAtscEpgErr("atsc epg manage release err!\n");

	GxBus_MessageEmpty(g_aes.s_EpgServiceHandle);
	GxCore_MutexUnlock(g_aes.mutex);
}

void service_epg_get(GxMsgProperty_AtscEpgInfo *epg_get)
{
	uint16_t create_event_count = g_aes.create_info.event_count;
	uint32_t source_id=0,i=0,real_count=0,want_count=0;
	time_t begin_time,end_time;
	status_t ret;
	GxBusEpgServiceHead service_head;
	GxEpgEventInfo *event_info=NULL;
	GxBusEpgFirstEvent *first_event=NULL;
	GxBusEpgEventHead event_head;
    EpgInfoList_t *epg_info;

	GxCore_MutexLock(g_aes.mutex);
    if(g_aes.init == false){
        GxCore_MutexUnlock(g_aes.mutex);
        return;
    }

	source_id = epg_get->source_id;
	begin_time = epg_get->begin_time;
	end_time = epg_get->end_time;
	want_count = epg_get->want_count;

	ret = GxBus_EpgServiceGet(0, source_id, 0, &service_head);
	if (ret != GX_BUS_EPG_OK)
		goto err;

	first_event = get_first_event(service_head.first_event);
	if (first_event == NULL || first_event->pos == GX_BUS_EPG_INVALID_POS)
		goto err;

	event_head.next_event = first_event->pos;
	for (i=0; i<create_event_count; i++) {
		event_info = GxCore_Realloc(event_info,(i+1)*sizeof(GxEpgEventInfo));
		CHECK_MEM_ALLOC(event_info);

		ret = GxBus_EpgEventGet(event_head.next_event, &event_head);
		if (ret != GX_BUS_EPG_OK)
			goto err;

		if(begin_time == 0 && end_time == 0 ){
			get_event(&event_info[i],&event_head);
			real_count++;

			if(want_count > 0 && want_count == real_count)
				break;

		}else if(begin_time > 0 && end_time > 0 && end_time > begin_time){
			if(event_head.time.start_time > begin_time
					&& event_head.time.start_time < end_time){
				get_event(&event_info[i],&event_head);
				real_count++;

				if(want_count > 0 && want_count == real_count)
					break;
			}
		}else if(begin_time > 0 && end_time == 0 ){
			if(event_head.time.start_time > begin_time ){
				get_event(&event_info[i],&event_head);
				real_count++;

				if(want_count > 0 && want_count == real_count)
					break;
			}
		}

		if (event_head.next_event == GX_BUS_EPG_INVALID_POS)
			break;
	}

	epg_get->real_count = real_count;
	epg_get->event_info = event_info;
    epg_info = GxCore_Malloc(sizeof(EpgInfoList_t ));
    CHECK_MEM_ALLOC(epg_info);
    epg_info->epg_info = event_info;
    epg_info->real_count = real_count;
    gxlist_add_tail(&(epg_info->list_head), &epg_info_list);
	GxCore_MutexUnlock(g_aes.mutex);
	return;

err: 
	if(event_info !=NULL && real_count !=0)
		free_event_mem(event_info,real_count);

	epg_get->real_count = 0;
	epg_get->event_info = NULL;
	GxCore_MutexUnlock(g_aes.mutex);
	return;
}

void service_epg_free(GxMsgProperty_AtscEpgInfo *epg_free)
{
	uint32_t real_count = 0;
	GxEpgEventInfo *event_info;
    EpgInfoList_t *epg_info;
    EpgInfoList_t *safe;

	GxCore_MutexLock(g_aes.mutex);
    if(g_aes.init == false){
        GxCore_MutexUnlock(g_aes.mutex);
        return;
    }
    gxlist_for_each_entry_safe(epg_info, safe, &epg_info_list, list_head){
        if(epg_free->event_info !=NULL && epg_info->epg_info == epg_free->event_info){
	real_count = epg_free->real_count;
	event_info = epg_free->event_info;

            if(real_count !=0)
		free_event_mem(event_info,real_count);

	epg_free->real_count = 0;
	epg_free->event_info = NULL;
            gxlist_del(&(epg_info->list_head));
            GxCore_Free(epg_info);
        }
    }

	GxCore_MutexUnlock(g_aes.mutex);
}

/* End of file -------------------------------------------------------------*/

