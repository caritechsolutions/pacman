#include "app_module.h"
#if PSI_MODULE_SUPPORT
#include "module/si/si_public_parser.h"

#define MAX_PSI_MODULE_NUM     32
#define MAX_PSI_SECTION_BUFFER (64*1024)

#define PSI_SECTION_OK          ((void*)0xfffffff4)
#define PSI_SUBTABLE_OK         ((void*)0xfffffff5)
#define THREAD_EXIT_ID          0Xff // correct id :1~32

typedef struct section_cmdinfo
{
    uint8_t       id;
    uint32_t      uuid;              //Universally Unique Identifier
    handle_t      handle;
    int           data_len;           //section len + 3(include head)
    uint8_t *        data;               //all need ,give section data
}section_cmdinfo;

typedef struct  parse_thread_handle
{
    handle_t  read_handle;
    handle_t  parse_handle;
    handle_t  queueid;
    handle_t  independent_queueid[MAX_PSI_MODULE_NUM];
}psi_thread_handle;

typedef struct psi_control_node
{
    uint8_t         id;
    uint8_t         tid;               //table_id
    uint16_t        section_count;
    uint16_t        total_section;
    uint8_t*        section_data;
    uint32_t        uuid;              //Universally Unique Identifier
    int             pid;
    handle_t        filter;
    handle_t        channel;
    psi_module_node node;
    uint8_t         section_state[32];
}psi_control_node;

static handle_t   psi_module_lock = -1;
static uint8_t s_psi_module_inited = 0;
static psi_thread_handle s_thread_handle={0};
static psi_control_node s_cotrol_node[MAX_PSI_MODULE_NUM];
static uint8_t s_psi_common_count = 0;
static uint8_t s_psi_count = 0;

static void _psi_module_cotrol_node_init(void)
{
    int i = 0;
    for(i=0;i < MAX_PSI_MODULE_NUM; i++)
    {
        if(s_cotrol_node[i].section_data != NULL)
        {
            GxCore_Free(s_cotrol_node[i].section_data);
            s_cotrol_node[i].section_data = NULL;
        }
        s_cotrol_node[i].total_section = 0;
        s_cotrol_node[i].id = 0;
        s_cotrol_node[i].pid = 0x1fff;
        s_cotrol_node[i].tid = 0xff;
        s_cotrol_node[i].filter = 0;
        s_cotrol_node[i].channel = 0;
        s_cotrol_node[i].uuid = 0;
        memset(s_cotrol_node[i].section_state, 0, sizeof(s_cotrol_node[i].section_state));
        s_cotrol_node[i].section_count = 0;
        s_cotrol_node[i].node.mode = -1;
        s_cotrol_node[i].node.parse_priv_cb = NULL;
        s_cotrol_node[i].node.parse_solv_cb = NULL;
        s_cotrol_node[i].node.parse_timeout_cb = NULL;
        s_cotrol_node[i].node.no_stop = false;
        s_cotrol_node[i].node.independent_thread = false;
        s_cotrol_node[i].node.timeout = 0;

    }
}

static int _psi_module_cotrol_node_nosame_exec(int index,uint8_t *section_data,uint16_t sec_len)
{
    uint8_t total_section = 0;
    uint8_t section_num = 0;
    if(s_cotrol_node[index].node.no_same == true)
    {
        total_section = section_data[7]+1;
        section_num = section_data[6];
        if(s_cotrol_node[index].total_section != total_section)
        {
            if(s_cotrol_node[index].section_data != NULL)
            {
                GxCore_Free(s_cotrol_node[index].section_data);
                s_cotrol_node[index].section_data = NULL;
            }
            s_cotrol_node[index].section_data = GxCore_Mallocz(4096*(total_section+1));
            if(s_cotrol_node[index].section_data == NULL)
            {
                app_log_error("malloc section_data error!!!!!!!!!\n");
                return -1;
            }
            s_cotrol_node[index].total_section = total_section;
        }
        else
        {
            if(memcmp(&(s_cotrol_node[index].section_data[section_num*4096]), section_data, sec_len) == 0)
                return 0;
        }
        memcpy(&(s_cotrol_node[index].section_data[section_num*4096]), section_data, sec_len);
    }
    return 1;
}

static int _psi_module_table_section_send(section_cmdinfo *p_info, bool muthread)
{
    uint8_t* p_data = NULL;
    int ret = 0;
    int index = 0;
    section_cmdinfo cmd_msg;
    memset(&cmd_msg,0,sizeof(cmd_msg));
    if((p_info != NULL) && (p_info->data != NULL))
    {
        p_data = GxCore_Malloc(p_info->data_len);
        if(p_data == NULL)
        {
            app_log_error("no memory left for malloc %d\n",p_info->data_len);
            return -1;
        }
        cmd_msg.handle = p_info->handle;
        cmd_msg.id     = p_info->id;
        cmd_msg.data_len = p_info->data_len;
        cmd_msg.uuid    = p_info->uuid;
        memcpy(p_data,p_info->data,p_info->data_len);
        cmd_msg.data = p_data;
        if(muthread == true)
        {
            index = p_info->id-1;
            ret = GxCore_QueuePut(s_thread_handle.independent_queueid[index],&cmd_msg,sizeof(section_cmdinfo),0);
        }
        else
        {
            ret = GxCore_QueuePut(s_thread_handle.queueid,&cmd_msg,sizeof(section_cmdinfo),0);
        }
        if(ret < 0)
        {
            GxCore_Free(p_data);
            app_log_error("update data msg queue failed %d\n",ret);
            return -1;
        }
    }
    return 0;
}


static psi_module_status _psi_module_parse_table_status_get(psi_control_node* control_node,si_info_t *p_si_info)
{
#define FORCE_SUBTABLE_OK_TIMES (3)
    uint16_t i;
    uint16_t total_section;
    if(control_node->id == 0)
        return PSI_MODULE_TABLE_OK;
    total_section = p_si_info->last_sec_number+1;
    control_node->section_state[p_si_info->sec_number/8] |= (1<<(p_si_info->sec_number%8));
    control_node->section_count++;
    if (control_node->section_count < p_si_info->last_sec_number*FORCE_SUBTABLE_OK_TIMES)
    {
        for (i=0; i<total_section; i++)
        {
            if (((control_node->section_state[i/8]>>(i%8))&1) != 1)
            {
                return PSI_MODULE_SECTION_OK;
            }
        }
    }
    return PSI_MODULE_TABLE_OK;

}

static void _psi_module_filter_data_send(handle_t filter, const uint8_t* section, uint8_t id,size_t Size, bool muthread,uint32_t uuid)
{
    uint16_t            section_length;
    section_cmdinfo info;
    uint8_t*            data = (uint8_t*)section;
    if (NULL == section)
        return;

    section_length = Size;
    info.data = data;
    info.data_len = section_length;
    info.handle = filter;
    info.id = id;
    info.uuid = uuid;
    _psi_module_table_section_send(&info,muthread);

    return;
}

static status_t _psi_module_lock_create(void)
{
    if(GxCore_MutexCreate(&psi_module_lock) != GXCORE_SUCCESS)
    {
        return GXCORE_ERROR;
    }
    return GXCORE_SUCCESS;
}

static void _psi_module_lock_delete(void)
{
    GxCore_MutexDelete(psi_module_lock);
    psi_module_lock = -1;
    return;
}

static void _psi_module_lock(void)
{
    GxCore_MutexLock(psi_module_lock);
    return;
}

static void _psi_module_unlock(void)
{
    GxCore_MutexUnlock(psi_module_lock);
    return;
}


static void* _psi_module_parser(psi_module_mode mode,uint8_t *section_data,uint32_t len,psi_module_status (*table_parse_fun)(uint8_t*,uint32_t))
{
// include table_id (1) and section_len (2)
#define SECTION_ONLY_APPEND_LEN   (3)
	int16_t sec_len;
	uint8_t *p_parsed_data = NULL;

	if((section_data == NULL) || (len == 0))
	{
		return NULL;
	}

	if (mode == PSI_MODULE_STANDARD_ONLY
			|| mode == PSI_MODULE_WITH_STANDARD
			|| mode == PSI_MODULE_SECTION_ONLY)
	{
		if (mode == PSI_MODULE_WITH_STANDARD
				&& table_parse_fun != NULL)
		{
			table_parse_fun(section_data, len);
		}

		p_parsed_data = (uint8_t*)GxCore_Mallocz(APP_SI_SMALL_BUF_SIZE_DEF);

		if (p_parsed_data == NULL)
		{
		    app_log_error("[si_parser] GxCore_MemPoolAllocZero1 error!\n");
			return NULL;
		}
		sec_len = TODATA12(section_data[1], section_data[2]);

		if (mode == PSI_MODULE_SECTION_ONLY)
		{
			if (sec_len + SECTION_ONLY_APPEND_LEN > APP_SI_SMALL_BUF_SIZE_DEF)
			{
                app_log_error("PSI_MODULE_SECTION_ONLY:section_len id long APP_SI_SMALL_BUF_SIZE_DEF\n");
				GxCore_Free(p_parsed_data);
				return NULL;
			}

			memcpy(p_parsed_data, section_data, sec_len + SECTION_ONLY_APPEND_LEN);

			return p_parsed_data;
		}
        if(GxBus_SiStandardParser(section_data,p_parsed_data,len,0) != STANDARD_PARSER_SUCCESS)
        {
            app_log_error("standard parser failed!!!!!!!!!!!!!!\n");
            GxCore_Free(p_parsed_data);
            return NULL;
        }
	}
	else if (mode == PSI_MODULE_PRIVATE_ONLY)
	{
		if (table_parse_fun != NULL)
		{
			if(table_parse_fun(section_data, len) == PSI_MODULE_SECTION_OK)
                return PSI_SECTION_OK;
            else
                return PSI_SUBTABLE_OK;
		}
	}

	return p_parsed_data;
}

static void _psi_module_parse_for_psi(section_cmdinfo* p_info)
{
    bool nostop = false;
    uint16_t i = 0;
    uint16_t sec_len  = 0;
    psi_module_mode mode = PSI_MODULE_WITH_STANDARD;
    psi_module_solved data_solved = NULL;
    psi_module_private parse_priv = NULL;

    void *parsed_data = NULL;
    psi_module_status parse_state = 0;
    int index = 0;
    index = p_info->id-1;
    for (i = 0; i < (p_info->data_len);)
    {
        _psi_module_lock();
        if(p_info->handle != s_cotrol_node[index].filter || p_info->uuid != s_cotrol_node[index].uuid)
        {
            app_log_debug("table_id =0x%2x demux filter is stop!!!!!!!!!!!\n",s_cotrol_node[index].tid);
            _psi_module_unlock();
            return;
        }
        if (s_cotrol_node[index].tid != p_info->data[i])
        {
             app_log_debug("demux hw get data err s_cotrol_node[index].tid =%2x p_info->data[%d] = %2x!!!!!!!!!!!\n",s_cotrol_node[index].tid,i,p_info->data[i]);
            _psi_module_unlock();
            return;
        }
        sec_len = TODATA12(p_info->data[i+1],p_info->data[i+2]);
        if (_psi_module_cotrol_node_nosame_exec(index,&(p_info->data[i]),sec_len+3) == 0)
        {
            app_log_debug("table_id =0x%2x the table is received!!!!!!!!!!!\n",s_cotrol_node[index].tid);
            _psi_module_unlock();
            return;
        }
        mode = s_cotrol_node[index].node.mode;
        parse_priv = s_cotrol_node[index].node.parse_priv_cb;
        data_solved  = s_cotrol_node[index].node.parse_solv_cb;
        nostop = s_cotrol_node[index].node.no_stop;
        _psi_module_unlock();
        parsed_data = _psi_module_parser(mode, &(p_info->data[i]),sec_len+3,parse_priv);
        if (parsed_data == NULL)
        {
            app_log_error("have no more mempool for store!\n");
            return;
        }
        if (mode == PSI_MODULE_SECTION_ONLY)
        {
            parse_state = PSI_MODULE_SECTION_OK;
        }
        else if(mode == PSI_MODULE_PRIVATE_ONLY)
        {
            if (parsed_data == PSI_SECTION_OK)
            {
                parse_state = PSI_MODULE_SECTION_OK;
                parsed_data = NULL;
            }
            else if (parsed_data == PSI_SUBTABLE_OK)
            {
                parse_state = PSI_MODULE_TABLE_OK;
                parsed_data = NULL;
            }
        }
        else
        {
            _psi_module_lock();
            parse_state = _psi_module_parse_table_status_get(&s_cotrol_node[index], (si_info_t*)(parsed_data));
            _psi_module_unlock();
        }
        if(parse_state == PSI_MODULE_TABLE_OK && nostop == false)
        {
            app_psi_module_stop(p_info->id);
        }
        if(data_solved != NULL)
        {
            data_solved(parsed_data,parse_state);
        }
        if(parsed_data != NULL)
        {
            GxCore_Free(parsed_data);
            parsed_data = NULL;
        }
        i += (sec_len+3);
    }
    return;
}

static void _psi_module_parse_thread(void* arg)
{
    section_cmdinfo* p_info = NULL;
    uint32_t size_copy = 0;
    handle_t queueid = 0;

    GxCore_ThreadDetach();
    queueid = *(handle_t*)arg;

    if(0 == queueid)
        return;

    p_info = GxCore_Malloc(sizeof(section_cmdinfo));
    if(p_info == NULL)
    {
        GxCore_QueueDelete(queueid);
        app_log_error("malloc p_info error!!!!!!!!!\n");
        return;
    }
    while(1)
    {
        memset(p_info,0,sizeof(section_cmdinfo));
        GxCore_QueueGet(queueid, p_info, sizeof(section_cmdinfo),&size_copy, GXCORE_CHECK);
        if(p_info->id == THREAD_EXIT_ID)
        {
            GxCore_QueueDelete(queueid);
            app_log_debug("queueid=%d parse thread exit!!!!!!!!!!!\n",queueid);
            break;
        }
        _psi_module_parse_for_psi(p_info);
        GxCore_Free(p_info->data);
        p_info->data = NULL;
        GxCore_ThreadDelay(30);
    }
    GxCore_Free(p_info);
}


static void _psi_module_read_data_thread(void* args)
{
    handle_t thread_id;
    uint8_t *p_Section;/* section  buffer */
	uint8_t *p_Buffer;
	uint32_t length;
	int32_t ret;
    int i = 0;
	handle_t handle;
    bool muthread = false;
	GxTime cur_time = {0};
    uint32_t uuid = 0;
    int now_time = 0;
    int32_t last_get_time[MAX_PSI_MODULE_NUM] = {0};
    psi_module_timeout timeout_cb = NULL;

    GxCore_ThreadDetach();
    thread_id = GxCore_ThreadGetId();
    p_Section = GxCore_Malloc(MAX_PSI_SECTION_BUFFER);

    if( p_Section == NULL )
    {
        app_log_error("%s alloc memory failure!\n",__func__);
        return;
    }
    while(1)
    {
        if(thread_id != s_thread_handle.read_handle)
        {
            app_log_debug("read thread_id =%d exit!!!!!!!!!!!\n",thread_id);
            break;
        }
        for(i=0 ; i< MAX_PSI_MODULE_NUM ; i++)
        {
            _psi_module_lock();
            handle = s_cotrol_node[i].filter;
            muthread = s_cotrol_node[i].node.independent_thread;
            uuid = s_cotrol_node[i].uuid;
            _psi_module_unlock();
            if (0 != handle)
            {
                GxCore_GetTickTime(&cur_time);
                memset(p_Section,0,MAX_PSI_SECTION_BUFFER);
                ret = GxFilterRead(handle,p_Section,MAX_PSI_SECTION_BUFFER,&length);
                if(( ret >= 0)&&(length>0))
                {
                    p_Buffer = p_Section;
                    _psi_module_filter_data_send(handle,p_Buffer,i+1,length,muthread,uuid);
                    last_get_time[i] = cur_time.seconds*1000 + cur_time.microsecs/1000;
                }
                else if(ret == -3)
                {
                    GxFilterDisable(handle);
                    GxFilterEnable(handle);
                }
                else
                {
                    if(0 == last_get_time[i])
                        last_get_time[i] = cur_time.seconds*1000 + cur_time.microsecs/1000;

                    _psi_module_lock();
                    if(s_cotrol_node[i].node.timeout != 0)
                    {
                        now_time = cur_time.seconds*1000 + cur_time.microsecs/1000;
                        if(now_time - last_get_time[i] > s_cotrol_node[i].node.timeout)
                        {
                            timeout_cb = s_cotrol_node[i].node.parse_timeout_cb;
                            last_get_time[i] = now_time;
                        }
                    }
                    _psi_module_unlock();
                    if(timeout_cb != NULL)
                    {
                        timeout_cb(i+1);
                        timeout_cb = NULL;
                    }
                }
            }
            else
            {
                last_get_time[i]=0;
            }

        }
        GxCore_ThreadDelay(100);
    }
    GxCore_Free(p_Section);
}

static status_t _psi_module_thread_create(int index,bool independent_thread)
{
    handle_t psi_queueid = 0;
    handle_t table_thread_handle = 0;

    if(s_psi_count == 0)
    {
        if(GxCore_ThreadCreate("filter_read",&(s_thread_handle.read_handle), _psi_module_read_data_thread, NULL, 8 * 1024, GXOS_DEFAULT_PRIORITY) != GXCORE_SUCCESS)
        {
            app_log_error("create filter_read thread failed!!!!!!!!!!!\n");
            return GXCORE_ERROR;
        }
    }
    s_psi_count++;
    if(independent_thread == true)
    {
        if(GxCore_QueueCreate(&psi_queueid,128,sizeof(section_cmdinfo)) != GXCORE_SUCCESS)
        {
            app_log_error("create independent queue  failed!!!!!!!!!!!\n");
            s_psi_count--;
            if(s_psi_count == 0)
            {
                s_thread_handle.read_handle = 0;
            }
            return GXCORE_ERROR;
        }
        s_thread_handle.independent_queueid[index] = psi_queueid;
        if(GxCore_ThreadCreate("psi_table",&(table_thread_handle),_psi_module_parse_thread,(void*)&(s_thread_handle.independent_queueid[index]),8 * 1024,GXOS_DEFAULT_PRIORITY) != GXCORE_SUCCESS)
        {
            app_log_error("create independent thread  failed!!!!!!!!!!!\n");
            s_psi_count--;
            if(s_psi_count == 0)
            {
                s_thread_handle.read_handle = 0;
            }
            GxCore_QueueDelete(psi_queueid);
            s_thread_handle.independent_queueid[index] = 0;
            return GXCORE_ERROR;
        }
    }
    else
    {
        if(s_psi_common_count == 0)
        {
            if(GxCore_QueueCreate(&(s_thread_handle.queueid),128,sizeof(section_cmdinfo)) != GXCORE_SUCCESS)
            {
                app_log_error("create common queue  failed!!!!!!!!!!!\n");
                s_psi_count--;
                if(s_psi_count == 0)
                {
                    s_thread_handle.read_handle = 0;
                }
                return GXCORE_ERROR;
            }
            if(GxCore_ThreadCreate("psi_common_parse",&s_thread_handle.parse_handle,_psi_module_parse_thread,(void*)&s_thread_handle.queueid,8 * 1024,GXOS_DEFAULT_PRIORITY) != GXCORE_SUCCESS)
            {
                app_log_error("create common thread  failed!!!!!!!!!!!\n");
                s_psi_count--;
                if(s_psi_count == 0)
                {
                    s_thread_handle.read_handle = 0;
                }
                GxCore_QueueDelete(s_thread_handle.queueid);
                s_thread_handle.queueid=0;
                return GXCORE_ERROR;
            }
        }
        s_psi_common_count++;
    }
    return GXCORE_SUCCESS;
}

static void _psi_module_thread_exit(int index,bool independent_thread)
{
    section_cmdinfo cmd_msg;
    s_psi_count--;
    if(s_psi_count == 0)
    {
        s_thread_handle.read_handle = 0;
    }
    if(independent_thread == true)
    {
        memset(&cmd_msg,0,sizeof(cmd_msg));
        cmd_msg.id     = THREAD_EXIT_ID;
        GxCore_QueuePut(s_thread_handle.independent_queueid[index],&cmd_msg,sizeof(section_cmdinfo),0);
        s_thread_handle.independent_queueid[index] = 0;
    }
    else
    {
        s_psi_common_count--;
        if(s_psi_common_count == 0)
        {
            memset(&cmd_msg,0,sizeof(cmd_msg));
            cmd_msg.id     = THREAD_EXIT_ID;
            s_thread_handle.parse_handle = 0;
            GxCore_QueuePut(s_thread_handle.queueid,&cmd_msg,sizeof(section_cmdinfo),0);
            s_thread_handle.queueid = 0;
        }
    }
}

static int _psi_module_demux_filter_start(uint16_t demux_id,psi_module_filter params, int index)
{
    handle_t channel;
    handle_t filter;
    int32_t eq_flag = 0;
    int32_t crc_flag = 0;
    channel = GxChannelAllocate(demux_id,params.pid);
    if(channel == -1 || channel == -2)
    {
        s_cotrol_node[index].filter = 0;
        app_log_error("!!!!!!!!!!!!!!!channel failed!!!!!!!!!!!!!!!!!\n\n");
        return 0;
    }
    GxChannelEnable(channel);
    filter = GxFilterAllocate(channel,params.size);
    if(filter == -1 || filter == -2)
    {
        s_cotrol_node[index].filter = 0;
        GxChannelDisable(s_cotrol_node[index].channel);
        GxChannelFree(s_cotrol_node[index].channel);
        app_log_error("!!!!!!!!!!!!!!!!!!filter is failed!!!!!!!!!!\n\n");
        return 0;
    }

    if(params.flags&EQ_FLAG)
        eq_flag = 1;

    if(params.flags&CRC_FLAG)
        crc_flag = 1;


    GxFilterSetup(filter,params.match,params.mask,eq_flag,crc_flag,params.depth);
    GxFilterEnable(filter);
    s_cotrol_node[index].channel = channel;
    s_cotrol_node[index].filter = filter;
    return filter;

}

static void _psi_module_demux_filter_stop(uint8_t id)
{
    int index = 0;
    index = id-1;
    if(s_cotrol_node[index].filter != 0)
    {
        GxFilterDisable(s_cotrol_node[index].filter);
        GxFilterFree(s_cotrol_node[index].filter);
        GxChannelDisable(s_cotrol_node[index].channel);
        GxChannelFree(s_cotrol_node[index].channel);
        _psi_module_thread_exit(index,s_cotrol_node[index].node.independent_thread);
    }
    if(s_cotrol_node[index].section_data != NULL)
    {
        GxCore_Free(s_cotrol_node[index].section_data);
        s_cotrol_node[index].section_data = NULL;
    }
    s_cotrol_node[index].total_section = 0;
    s_cotrol_node[index].id = 0;
    s_cotrol_node[index].uuid = 0;
    s_cotrol_node[index].pid = 0x1fff;
    s_cotrol_node[index].tid = 0xff;
    s_cotrol_node[index].filter = 0;
    s_cotrol_node[index].channel = 0;
    memset(s_cotrol_node[index].section_state, 0, sizeof(s_cotrol_node[index].section_state));
    s_cotrol_node[index].section_count = 0;
    s_cotrol_node[index].node.mode = -1;
    s_cotrol_node[index].node.parse_priv_cb = NULL;
    s_cotrol_node[index].node.parse_solv_cb = NULL;
    s_cotrol_node[index].node.parse_timeout_cb = NULL;
    s_cotrol_node[index].node.no_stop = false;
    s_cotrol_node[index].node.no_same = false;
    s_cotrol_node[index].node.independent_thread = false;
    s_cotrol_node[index].node.timeout = 0;
}

void app_psi_module_init(void)
{
    if(s_psi_module_inited == 0)
    {
        GxDmxInit();
       if(_psi_module_lock_create() != GXCORE_SUCCESS)
        {
            app_log_error("init psi_module error!!!!!!!!!!!\n");
            return;
        }
        _psi_module_cotrol_node_init();
        s_psi_module_inited = 1;
    }
}

void app_psi_module_destroy(void)
{
    if(s_psi_module_inited == 1)
    {
        GxDmxDeinit();
        _psi_module_lock_delete();
        s_psi_module_inited = 0;
    }
}

int app_psi_module_start(psi_module_config config)
{
    int i = 0;
    if(s_psi_module_inited == 0)
    {
        app_log_error("psi_module is not init!!!!!!!!!!!!!!!\n");
        return 0;
    }
    static uint32_t uuid = 1;
    _psi_module_lock();
    for(i= 0; i<MAX_PSI_MODULE_NUM;i++)
    {
        if(s_cotrol_node[i].id == 0)
        {
            if(_psi_module_thread_create(i,config.node.independent_thread) != GXCORE_SUCCESS)
            {
                _psi_module_unlock();
                app_log_error("psi_module create thread !!!!!!!!!!!!!!!\n");
                return 0;
            }
            _psi_module_demux_filter_start(config.demux_id,config.params,i);
            if(s_cotrol_node[i].filter != 0)
            {
                s_cotrol_node[i].id = i+1;
                s_cotrol_node[i].uuid = uuid++;
                s_cotrol_node[i].pid = config.params.pid;
                s_cotrol_node[i].tid = config.params.match[0];
                s_cotrol_node[i].node.mode = config.node.mode;
                s_cotrol_node[i].node.parse_priv_cb = config.node.parse_priv_cb;
                s_cotrol_node[i].node.parse_solv_cb = config.node.parse_solv_cb;
                s_cotrol_node[i].node.parse_timeout_cb = config.node.parse_timeout_cb;
                s_cotrol_node[i].node.no_stop = config.node.no_stop;
                s_cotrol_node[i].node.no_same = config.node.no_same;
                s_cotrol_node[i].node.independent_thread = config.node.independent_thread;
                s_cotrol_node[i].node.timeout = config.node.timeout;
                memset(s_cotrol_node[i].section_state, 0, sizeof(s_cotrol_node[i].section_state));
                s_cotrol_node[i].section_count = 0;
                if(uuid == 0)
                {
                     app_log_error("psi_module uuid is overflow!!!!!!!!!!!!!!!\n");
                     uuid = 1;
                }
                _psi_module_unlock();
                return s_cotrol_node[i].id;
            }
            else
            {
                _psi_module_thread_exit(i,config.node.independent_thread);
                app_log_error("start parse psi table error!!!!!!!!!\n");
                _psi_module_unlock();
                return 0;
            }

        }
    }
    _psi_module_unlock();
    return 0;
}

void app_psi_module_stop(uint8_t id)
{
    if(id == 0 || id > MAX_PSI_MODULE_NUM)
    {
        return;
    }
    _psi_module_lock();
    _psi_module_demux_filter_stop(id);
    _psi_module_unlock();
    return;
}

int app_psi_module_reset(uint8_t id)
{

    handle_t filter;
    if(id == 0 || id > MAX_PSI_MODULE_NUM)
    {
        app_log_error("reset faild!!!!!!!!!!!!!!\n");
        return -1;
    }
    _psi_module_lock();
    filter = s_cotrol_node[id-1].filter;
    if(filter == 0)
    {
        app_log_error("reset faild!!!!!!!!!!!!!!\n");
        _psi_module_unlock();
        return -1;
    }
    GxFilterDisable(filter);
    GxFilterEnable(filter);
    _psi_module_unlock();
    return 0;
}

void app_psi_module_modify(uint8_t id ,psi_module_filter params)
{
    int32_t eq_flag = 0;
    int32_t crc_flag = 0;
    uint8_t index = 0;
    if(id == 0 || id > MAX_PSI_MODULE_NUM)
    {
        app_log_error("id is  invalid value!!!!!!!!!!!!\n");
        return;
    }

    if(params.flags&EQ_FLAG)
        eq_flag = 1;

    if(params.flags&CRC_FLAG)
        crc_flag = 1;

    _psi_module_lock();
    index = id-1;
    if(s_cotrol_node[index].filter == 0)
    {
        app_log_error("filter is free !!!!!!!!!!!!\n");
        _psi_module_unlock();
        return;
    }
    GxFilterDisable(s_cotrol_node[index].filter);
    if(s_cotrol_node[index].section_data != NULL)
    {
        GxCore_Free(s_cotrol_node[index].section_data);
        s_cotrol_node[index].section_data = NULL;
    }
    s_cotrol_node[index].total_section = 0;
    s_cotrol_node[index].pid = params.pid;
    s_cotrol_node[index].tid = params.match[0];
    s_cotrol_node[index].section_count = 0;
    memset(s_cotrol_node[index].section_state, 0, sizeof(s_cotrol_node[index].section_state));
    GxFilterSetup(s_cotrol_node[index].filter,params.match,params.mask,eq_flag,crc_flag,params.depth);
    GxFilterEnable(s_cotrol_node[index].filter);
    _psi_module_unlock();
}
#endif
