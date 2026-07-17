#include "app_module.h"

#if BOUQUET_SUPPORT
#define BAT_MAX_NUM 10
#define NEQ_PARSE_TIME_MS 24*60*60*1000UL

typedef struct {
	uint8_t *pBatSectionFlag;
	uint16_t bouquet1;
	uint16_t bouquet2;
	uint32_t batsubtid;
}BatData;

BatData bat_data[BAT_MAX_NUM];
_bqt_sort_info_t g_bqt_sort[MAX_BOUQUET_LIST];

#define TIMEOUT_BOUQUET_TIME            5000   //ms means how much time not get bouquet need delete;
static bouquet_full_info_t g_bouquet_list[MAX_BOUQUET_LIST];
static int bouquet_valid_number = 0;
static handle_t BatLock = 0;
static int app_need_flush = 0;

static _bouquet_list_t cur_bouquet_list;
static int cur_bouquet_id = 0;

void sort_by_bqt(_bqt_sort_info_t a[],int n)    //n为数组a的元素个数
{
	int i,j;
	_bqt_sort_info_t temp;
	for(j=0;j<n-1;j++)
		for(i=0;i<n-1-j;i++)
			if(a[i].bouquet_id > a[i+1].bouquet_id)    //数组元素大小按升序排列
			{
				temp = a[i];
				a[i] = a[i+1];
				a[i+1] = temp;
			}
}

void sort_by_lcn(_bouquet_va_service_t a[],int n)
{
	int i,j;
	_bouquet_va_service_t temp;
	for(j=0;j<n-1;j++)
		for(i=0;i<n-1-j;i++)
			if(a[i].service_lcn > a[i+1].service_lcn)    //数组元素大小按升序排列
			{
				temp = a[i];
				a[i] = a[i+1];
				a[i+1] = temp;
			}
}

int app_sort_bouquet_by_id(void)
{
	int index = 0;
	for(index = 0;index <bouquet_valid_number;index++)   //first get all bouquet
	{
		g_bqt_sort[index].real_bqt_id = index;
		g_bqt_sort[index].bouquet_id =  g_bouquet_list[index].bouquet_list.bouquet_id;
	}
	//do sort.
	sort_by_bqt(g_bqt_sort,bouquet_valid_number);
	return 0;
}

int app_get_bouquet_num(void)
{
	GxCore_MutexLock(BatLock);
	if(app_need_flush == 1)
	{
		app_log_info("app_get_bouquet_num g_bouquet_need_update!!!!!!!!!!!!!\n");
		app_need_flush = 0;
		app_sort_bouquet_by_id();
	}
	GxCore_MutexUnlock(BatLock);
	return bouquet_valid_number;
}

uint8_t  *app_get_bouquet_name(int bouquet_index,int *bouquet_id)
{
	int index = 0;
	static char bouquet_name_get[MAX_BOUQUET_NAME] = {0};
	GxCore_MutexLock(BatLock);
	if((bouquet_index < 0) || (bouquet_index >= bouquet_valid_number))
	{
		app_log_error("error bouquet index %d\n",bouquet_index);
		GxCore_MutexUnlock(BatLock);
		memset(bouquet_name_get,0,sizeof(bouquet_name_get));
		return (uint8_t*)bouquet_name_get;
	}
	index = g_bqt_sort[bouquet_index].real_bqt_id;
	strcpy(bouquet_name_get,(char*)g_bouquet_list[index].bouquet_list.bouquet_name);
	if(bouquet_id != NULL)
	{
		*bouquet_id = g_bouquet_list[index].bouquet_list.bouquet_id;
		cur_bouquet_id = g_bouquet_list[index].bouquet_list.bouquet_id;
		cur_bouquet_list = g_bouquet_list[index].bouquet_list;
	}
	GxCore_MutexUnlock(BatLock);
	return (uint8_t*)bouquet_name_get;
}

int app_bouquet_list_get_num(int bouquet_index,int bouquet_id)
{
	int total = 0;
	int index = 0;

	int j = 0;

	GxBusPmDataProg Prog ={0};
	int ret = 0;

	GxCore_MutexLock(BatLock);
	if((bouquet_index < 0) || (bouquet_index >= bouquet_valid_number))
	{
		app_log_error("error bouquet index %d\n",bouquet_index);
		GxCore_MutexUnlock(BatLock);
		return 0;
	}


	index = g_bqt_sort[bouquet_index].real_bqt_id;
	if(g_bouquet_list[index].bouquet_list.bouquet_id == bouquet_id)
	{
		if(g_bouquet_list[index].bouquet_list.need_update == 1)
		{
			app_log_info("app_bouquet_list_get_num need_update!!!!!!!!!!!!!\n");
			for(j = 0; j < g_bouquet_list[index].bouquet_list.service_num; j++)
			{
#if BAT_MISMATCH_TSID_SUPPORT
				ret = GxBus_PmProgGetBySpecialId(0xffffffff,g_bouquet_list[index].bouquet_list.service_list[j].service_id,0xffffffff,&Prog);
#else
				ret = GxBus_PmProgGetBySpecialId(g_bouquet_list[index].bouquet_list.service_list[j].ts_id,
					g_bouquet_list[index].bouquet_list.service_list[j].service_id,0xffffffff,&Prog);
#endif
				if(ret == GXCORE_SUCCESS)
				{
                    if(GXBUS_PM_PROG_BOOL_DISABLE == Prog.skip_flag)
                    {
                        g_bouquet_list[index].bouquet_list.valid_list[total].service_lcn = Prog.pos;
                        g_bouquet_list[index].bouquet_list.valid_list[total].prog_id = Prog.id;
                        g_bouquet_list[index].bouquet_list.valid_list[total].order = g_bouquet_list[index].bouquet_list.service_list[j].order;
                        total++;
                    }
				}
			}
			sort_by_lcn(g_bouquet_list[index].bouquet_list.valid_list,total);
		//	sort_by_order(g_bouquet_list[index].bouquet_list.valid_list,total);
			g_bouquet_list[index].bouquet_list.valid_service_num = total;
			cur_bouquet_list = g_bouquet_list[index].bouquet_list;
		}
		total = g_bouquet_list[index].bouquet_list.valid_service_num;
	}
	else  //old bouqet is gone
	{
		if(cur_bouquet_id == bouquet_id)
		{
			total = cur_bouquet_list.valid_service_num;
		}
		else
		{
			app_log_error("error cant find the bouqet id %d\n",bouquet_id);
		}
	}
	//zfz

	/*create valid_service_pos list from BAT service_id and DBase*/

	GxCore_MutexUnlock(BatLock);
	return total;
}

int  app_bouquet_list_get_pos_by_progid(int bouquet_index, int prog_id, uint32_t *pos)
{
    if (NULL == pos)
        return 0;

	GxCore_MutexLock(BatLock);
	if((bouquet_index < 0) || (bouquet_index >= bouquet_valid_number))
	{
		app_log_error("error bouquet index %d\n",bouquet_index);
        *pos = 0;
		GxCore_MutexUnlock(BatLock);
		return 0;
	}
    int index = 0;
    int i = 0;
	index = g_bqt_sort[bouquet_index].real_bqt_id;
    for(i = 0; i < g_bouquet_list[index].bouquet_list.valid_service_num ; i++)
    {
        if(g_bouquet_list[index].bouquet_list.valid_list[i].prog_id == prog_id)
        {
            *pos = i;
            GxCore_MutexUnlock(BatLock);
            return 1;
        }
    }
    *pos = 0;
    GxCore_MutexUnlock(BatLock);
	return 0;
}

int app_bouquet_list_get_prog_id(int bouquet_index,int list_pos,int bouquet_id)
{
	int index = 0;
	int pos = 0;

	GxCore_MutexLock(BatLock);
	if((bouquet_index < 0) || (bouquet_index >= bouquet_valid_number))
	{
		app_log_error("error bouquet index %d\n",bouquet_index);
		GxCore_MutexUnlock(BatLock);
		return -1;
	}
	index = g_bqt_sort[bouquet_index].real_bqt_id;

	if(g_bouquet_list[index].bouquet_list.bouquet_id == bouquet_id)
	{
		if((g_bouquet_list[index].bouquet_list.valid_service_num <= list_pos) || (list_pos < 0))
		{
			app_log_error("bouquet has no this item pos  valid[%d] want_pos[%d]\n",g_bouquet_list[index].bouquet_list.valid_service_num,list_pos);
			GxCore_MutexUnlock(BatLock);
			return -1;
		}
		pos = g_bouquet_list[index].bouquet_list.valid_list[list_pos].prog_id;
	}
	else
	{
		if(cur_bouquet_id == bouquet_id)
		{
			if((cur_bouquet_list.valid_service_num <= list_pos) || (list_pos < 0))
			{
				app_log_error("bouquet has no this item pos  valid[%d] want_pos[%d]\n",cur_bouquet_list.valid_service_num,list_pos);
				GxCore_MutexUnlock(BatLock);
				return -1;
			}
			pos = cur_bouquet_list.valid_list[list_pos].prog_id;
		}
		else
		{
			app_log_error("error cant find the bouqet id %d\n",bouquet_id);
		}
	}
	GxCore_MutexUnlock(BatLock);
	return pos;
}

void add_bouquet_name(int8_t index, uint8_t *name,uint16_t bqt_id)
{
	char* p = NULL;
	int len = 0;

	p = strchr((const char *)name, ',');
	while(p != NULL)
	{
		*p = ' ';
		p = strchr(p, ',');
	}
	GxCore_MutexLock(BatLock);
	if(g_bouquet_list[index].p_update_info != NULL)
	{
		len = MAX_BOUQUET_NAME > strlen((const char *)name) ? MAX_BOUQUET_NAME-1 : strlen((const char *)name);
		memcpy(g_bouquet_list[index].p_update_info->updateing_info.bouquet_name, name, len);
	}
	app_log_info("[bat]bouquet_name = %s bqt_id=%d\n", g_bouquet_list[index].bouquet_list.bouquet_name,bqt_id);

	GxCore_MutexUnlock(BatLock);
}

void add_bouquet_service(int8_t index, uint16_t service_id,uint8_t service_type, uint16_t ts_id)
{
	int service_index = 0;
	GxCore_MutexLock(BatLock);
	if(g_bouquet_list[index].p_update_info == NULL)
	{
		app_log_info("bouqet has been clear\n");
		return ;
	}

	service_index = g_bouquet_list[index].p_update_info->updateing_info.service_num;
	if(service_index >= MAX_SERVICE_NUM)
	{
		app_log_error("service full\n");
		return;
	}
	g_bouquet_list[index].p_update_info->updateing_info.service_list[service_index].service_id  = service_id;

	g_bouquet_list[index].p_update_info->updateing_info.service_list[service_index].service_type  = service_type;
	g_bouquet_list[index].p_update_info->updateing_info.service_list[service_index].ts_id = ts_id;
	g_bouquet_list[index].p_update_info->updateing_info.service_num++;

	GxCore_MutexUnlock(BatLock);
}

static int8_t check_bat_version(uint16_t bouquet_id, uint8_t version,uint8_t  last_section_number,uint8_t section_num,int8_t *index_out)
{
	int i = 0;
	int index = 0;
	GxTime Time = {0};
	uint32_t tickmsec = 0;

	for(i=0;i<bouquet_valid_number;i++)
	{
		if(g_bouquet_list[i].bouquet_list.bouquet_id == bouquet_id)
		{
			break;
		}
	}
	index = i;
	if(index >= MAX_BOUQUET_LIST)
	{
		*index_out = -1;
		return -1;
	}
	GxCore_GetTickTime(&Time);
	tickmsec = Time.seconds * 1000 + Time.microsecs/1000;

	GxCore_MutexLock(BatLock);
	if(index < bouquet_valid_number)
	{
		if(g_bouquet_list[index].p_update_info == NULL)  //normal state
		{
			if(g_bouquet_list[index].bouquet_list.version != version)  //version change need update
			{
				g_bouquet_list[index].p_update_info = GxCore_Mallocz(sizeof(_bouquet_update_t));
				if(g_bouquet_list[i].p_update_info == NULL)
				{
					app_log_error("bat update malloc failed for update info\n");
					GxCore_MutexUnlock(BatLock);
					return -1;
				}
				g_bouquet_list[index].is_update = 1;
				g_bouquet_list[index].p_update_info->updateing_info.bouquet_id = bouquet_id;
				g_bouquet_list[index].p_update_info->updateing_info.version = version;
				g_bouquet_list[index].p_update_info->table_info.last_section_num = last_section_number;
				g_bouquet_list[index].p_update_info->table_info.section_data[section_num] = 1;
			}
			else //version not change maybe need update
			{
				*index_out = index;
				g_bouquet_list[index].last_fetch_msec = tickmsec;		//doing update
				GxCore_MutexUnlock(BatLock);
				return 0;
			}
		}
		else //update status
		{
			if(g_bouquet_list[index].p_update_info->updateing_info.version != version)
			{
				app_log_info("doing update version change again???\n");
			}
			g_bouquet_list[index].p_update_info->table_info.section_data[section_num] = 1;
		}

	}
	else //new
	{
		g_bouquet_list[index].bouquet_list.bouquet_id = bouquet_id;
		g_bouquet_list[index].bouquet_list.version = version;
		if(g_bouquet_list[index].p_update_info == NULL)
		{
			g_bouquet_list[index].p_update_info = GxCore_Mallocz(sizeof(_bouquet_update_t));
			if(g_bouquet_list[i].p_update_info == NULL)
			{
				*index_out = -1;
				app_log_error("bat update malloc failed for update info\n");
				GxCore_MutexUnlock(BatLock);
				return -1;
			}
			g_bouquet_list[index].p_update_info->updateing_info.bouquet_id = bouquet_id;
			g_bouquet_list[index].p_update_info->updateing_info.version = version;
			g_bouquet_list[index].is_update = 1;
			g_bouquet_list[index].p_update_info->table_info.last_section_num = last_section_number;
			g_bouquet_list[index].p_update_info->table_info.section_data[section_num] = 1;
		}
		bouquet_valid_number++;
	}
	g_bouquet_list[index].last_fetch_msec = tickmsec;		//doing update
	GxCore_MutexUnlock(BatLock);
	*index_out = index;
	return 1;
}

static int8_t app_check_bat_finish(uint8_t index)
{
	int section_num = 0;
	int i = 0;

	GxCore_MutexLock(BatLock);
	if(g_bouquet_list[index].p_update_info == NULL)
	{
		app_log_info("update info was free\n");
		return 0;
	}
	section_num = g_bouquet_list[index].p_update_info->table_info.last_section_num + 1;
	for(i=0;i<section_num;i++)
	{
		if(g_bouquet_list[index].p_update_info->table_info.section_data[i] == 0)
		{
			break;
		}
	}
	if(section_num == i)
	{
		g_bouquet_list[index].is_update = 0;

		g_bouquet_list[index].bouquet_list = g_bouquet_list[index].p_update_info->updateing_info;
		GxCore_Free(g_bouquet_list[index].p_update_info);
		g_bouquet_list[index].p_update_info = NULL;
		g_bouquet_list[index].bouquet_list.need_update = 1;
        app_auto_update_send_ui_msg(ACU_UPDATE_BAT,0,NULL);
		app_need_flush = 1;
	}

	GxCore_MutexUnlock(BatLock);
	return 0;
}


static void app_search_remove_bouquet(bouquet_full_info_t *bqt_arry,int *num,int idx)
{
	int i = 0;
	if((idx < 0)||(idx > *num))  //if idx == num means del last one
	{
		return;
	}
	*num = *num-1;

	for(i=idx;i<*num;i++)
	{
		bqt_arry[i]=bqt_arry[i+1];
	}
}

static int8_t check_bat_remove(void)
{
	GxTime  Time = {0};
	uint32_t time_msec = 0;
	GxCore_GetTickTime(&Time);
	int i = 0;

	time_msec = Time.seconds *1000 + Time.microsecs/1000;

	GxCore_MutexLock(BatLock);
	for(i=(bouquet_valid_number-1);i>=0;i--)
	{
		if((g_bouquet_list[i].last_fetch_msec > time_msec) || (g_bouquet_list[i].last_fetch_msec == 0)) //means uint32_t over flow or signal unlock occure
		{
			g_bouquet_list[i].last_fetch_msec = time_msec;
		}
		else
		{
			if((time_msec - g_bouquet_list[i].last_fetch_msec) > (1000*bouquet_valid_number))  //
			{
				app_log_info("bouquet id %d not fetch timeout\n",g_bouquet_list[i].bouquet_list.bouquet_id);
				//todo remove it
				if(g_bouquet_list[i].p_update_info != NULL)
				{
					GxCore_Free(g_bouquet_list[i].p_update_info);
					g_bouquet_list[i].p_update_info = NULL;
				}
				app_search_remove_bouquet(g_bouquet_list,&bouquet_valid_number,i);
				app_auto_update_send_ui_msg(ACU_UPDATE_BAT,0,NULL);
				app_need_flush = 1;
			}
		}
	}
	GxCore_MutexUnlock(BatLock);
	return 0;
}

static psi_module_status app_table_bat_search_section_parse(uint8_t *p_section_data, size_t len)
{
	uint8_t* data = p_section_data;
	uint8_t* pBuf = NULL;
	uint8_t section_number = 0;
	uint8_t last_section_number = 0;
	uint8_t version = 0;
	int16_t bouquet_descriptors_length = 0;
	int16_t ts_loop_length = 0;
	int16_t section_length = 0;
	uint32_t ret = PSI_MODULE_SECTION_OK;
	uint8_t descriptor_tag = 0;
	uint8_t descriptor_length = 0;
	uint16_t ts_id = 0;
	uint16_t service_id = 0;
	uint8_t service_type = 0;
	uint16_t bouquet_id = 0;
	uint8_t bouquet_name[MAX_BOUQUET_NAME] = {0};
	int8_t index = -1;
	int16_t logical_desc_len = 0;
	int16_t ts_descriptor_length = 0;
	int need_update = 0;

	if(data[0] == BAT_TID)
	{
		section_length = (((uint16_t)data[1]<<8)|data[2])&0x0FFF;
		bouquet_id = (((uint16_t)data[3]<<8)|data[4]);
		version = (data[5]&0x3e)>>1;
		section_number = data[6];
		last_section_number = data[7];

		need_update = check_bat_version(bouquet_id, version, last_section_number,section_number,&index);

		if((index == -1) || (need_update == 0))
		{
			check_bat_remove();
			return ret;
		}

		bouquet_descriptors_length = (((uint16_t)data[8]<<8)|data[9])&0x0FFF;
		data += 10;
		section_length -= 11;//and crc

		if(section_length > 0)
		{
			while(bouquet_descriptors_length >0)
			{
				descriptor_tag = data[0];
				ts_descriptor_length = descriptor_length = data[1];
				data += 2;
				bouquet_descriptors_length  -= 2;

				if( descriptor_length==0 )
				{
					continue;
				}

				switch( descriptor_tag )
				{
					case 0x47://get bouquet_name
						if (ts_descriptor_length >= MAX_BOUQUET_NAME)
						{
							ts_descriptor_length = MAX_BOUQUET_NAME-1;
						}
						memcpy(bouquet_name, data, ts_descriptor_length);
						bouquet_name[ts_descriptor_length] = 0;
						{
							add_bouquet_name(index,bouquet_name,bouquet_id);
						}
						break;
					default:
						break;
				}
				data += descriptor_length;
				bouquet_descriptors_length -= descriptor_length;
			}

			ts_loop_length = (((uint16_t)data[0]<<8)|data[1])&0x0FFF;
			data += 2;
			while(ts_loop_length >0)
			{
				uint8_t length = 0;

				ts_id = (((uint16_t)data[0]<<8)|data[1]);//get ts_id
				ts_descriptor_length = (((uint16_t)data[4]<<8)|data[5])&0x0FFF;
				data += 6;
				ts_loop_length -= (6+ts_descriptor_length);

				while(ts_descriptor_length > 0)
				{
					descriptor_tag= data[0];
					length = data[1];

					data += 2;
					ts_descriptor_length -= 2;
					if( length==0 )
					{
						app_log_error("error length == 0\n");
						continue;
					}

					switch( descriptor_tag )
					{
						case 0x41://get service_id
							pBuf = data;
							logical_desc_len = (uint16_t)(length&0xff);
							while(logical_desc_len>0)
							{
								service_id = (((uint16_t)pBuf[0]<<8)|pBuf[1]);
								service_type = (uint8_t)pBuf[2];
								{
									add_bouquet_service(index,service_id,service_type,ts_id);
								}
								logical_desc_len-=3;
								pBuf += 3;
							}
							break;
						default:
							break;
					}
					data += length;
					ts_descriptor_length -= length;
				}
			}

		}
		app_check_bat_finish(index);

	}

	return ret;
}

static int bat_id = 0;

int app_table_bat_restart(uint8_t id)
{
    if(id == 0)//超时回调，工作模块已停止
    {
        g_AppBat.start(&g_AppBat);//重启psi模块
    }
    else
    {
        app_psi_module_reset(id);//复位psi模块
    }
    return 0;
}

static void app_bat_start(AppBat *bat)
{
    psi_module_config config={0};
    config.demux_id = 0;
    config.params.pid = BAT_PID;
    config.params.depth = 1;
    config.params.match[0] = BAT_TID;
    config.params.mask[0] = 0xff;
    config.params.flags = config.params.flags | EQ_FLAG;
    config.params.flags = config.params.flags | REPEAT_FLAG;
    config.params.flags = config.params.flags | CRC_FLAG;
    config.node.timeout = 10000;
    config.node.parse_timeout_cb = app_table_bat_restart;
    config.node.mode = PSI_MODULE_PRIVATE_ONLY;
    config.node.parse_priv_cb = app_table_bat_search_section_parse;
    config.node.parse_solv_cb = NULL;
    bat_id = app_psi_module_start(config);
	return;
}

static uint32_t app_bat_stop(AppBat *bat)
{
    app_psi_module_stop(bat_id);
    bat_id = 0;
	return 0;
}

void app_bouquet_reset_all_time(void)
{
	int i =0;

	GxCore_MutexLock(BatLock);
	for(i=0;i<bouquet_valid_number;i++)
	{
		g_bouquet_list[i].last_fetch_msec = 0;
	}
	GxCore_MutexUnlock(BatLock);
}

void app_bouquet_clear_all(void)
{
	int i = 0;
	GxCore_MutexLock(BatLock);
	for(i=0;i<bouquet_valid_number;i++)
	{
		if(g_bouquet_list[i].p_update_info != NULL)
		{
			GxCore_Free(g_bouquet_list[i].p_update_info);
			g_bouquet_list[i].p_update_info = NULL;
		}
	}
	memset(g_bouquet_list,0,sizeof(g_bouquet_list));
	memset(&cur_bouquet_list,0,sizeof(cur_bouquet_list));
	bouquet_valid_number = 0;
	cur_bouquet_id = 0;
	GxCore_MutexUnlock(BatLock);
}

void app_bouquet_init(void)
{
	if(BatLock == 0)
	{
		GxCore_MutexCreate(&BatLock);
	}
	memset(g_bouquet_list,0,sizeof(g_bouquet_list));
	memset(&cur_bouquet_list,0,sizeof(cur_bouquet_list));
}

void app_bouque_set_update(int ts_id,int service_id)
{
	int i =0;
	int j = 0;
	GxCore_MutexLock(BatLock);
	for(i=0;i<bouquet_valid_number;i++)
	{
		for(j=0;i<g_bouquet_list[j].bouquet_list.service_num;j++)
		{

			if((g_bouquet_list[i].bouquet_list.service_list[j].service_id == service_id)
#if !BAT_MISMATCH_TSID_SUPPORT
				&&(g_bouquet_list[i].bouquet_list.service_list[j].ts_id == ts_id)
#endif
			)
			{
				g_bouquet_list[i].bouquet_list.need_update = 1;
			}
		}
	}
	GxCore_MutexUnlock(BatLock);
}



AppBat g_AppBat =
{
	.subt_id = -1,
	.req_id  = 0,
	.start   = app_bat_start,
	.stop    = app_bat_stop,
};

#endif

