#include "../include/vod_trans_api.h"
#include "../include/vod_in_typedef.h"
#include "../include/vod_porting_all.h"
#include "../include/vod_trans_api.h"
#include "hwts/vod_trans_hwts.h"
#include "ts/vod_trans_ts.h"
#include "vod_trans.h"


static int32_t trans_type;
static uint32_t g_locate_iframe_flag = 0;

static uint32_t g_last_vtimestamp = 0;
static uint32_t g_last_atimestamp = 0;

int32_t vod_trans_open(int32_t type)
{
	trans_type = type;
	g_last_vtimestamp = 0;
	g_last_atimestamp = 0;
	g_locate_iframe_flag = 0;

	if(VOD_TRANS_TYPE_ISMA == trans_type)
	{
		//return vod_trans_isma_open();
		return -1;
	}
	else if(VOD_TRANS_TYPE_TS == trans_type)
	{
		return vod_trans_ts_open();
	}
	else if(VOD_TRANS_TYPE_HWTS == trans_type)
	{
		return vod_trans_hwts_open();
	}
	return -1;
}

void vod_trans_close()
{
	if(VOD_TRANS_TYPE_ISMA == trans_type)
	{
		//vod_trans_isma_close();
	}
	else if(VOD_TRANS_TYPE_TS == trans_type)
	{
		vod_trans_ts_close();
	}
	else if(VOD_TRANS_TYPE_HWTS == trans_type)
	{
		return vod_trans_hwts_close();
	}
}

uint8_t* vod_trans_malloc_buffer(int32_t len, uint32_t* token)
{
	uint8_t * p_buff = NULL;
	if(VOD_TRANS_TYPE_ISMA == trans_type)
	{
		//p_buff = vod_trans_isma_malloc_buffer(len, token);
	}
	else if(VOD_TRANS_TYPE_TS == trans_type)
	{
		p_buff = vod_trans_ts_malloc_buffer(len, token);
	}
	else if(VOD_TRANS_TYPE_HWTS == trans_type)
	{
		p_buff = vod_trans_hwts_malloc_buffer(len, token);
	}
	return p_buff;
}


int32_t vod_trans_insert_buffer(uint32_t token, int32_t state)
{
	if(VOD_TRANS_TYPE_ISMA == trans_type)
	{
		//vod_trans_isma_insert_buffer(token, state);
	}
	else if(VOD_TRANS_TYPE_TS == trans_type)
	{
		vod_trans_ts_insert_buffer(token, state);	
		return 0;
	}
	else if(VOD_TRANS_TYPE_HWTS == trans_type)
	{
		return vod_trans_hwts_insert_buffer(token, state);
	}
	return -1;
}

int32_t vod_trans_get_frame(int32_t aud_or_vid, uint8_t** buffer, int32_t* len, uint32_t* timestamp, int32_t* iframe)
{
	int32_t ret = 0;

	if(VOD_TRANS_TYPE_ISMA == trans_type)
	{
#if 0
		uint32_t tbegin = vod_porting_get_ms();
reget:
		ret = vod_trans_isma_get_frame(aud_or_vid, buffer, len, timestamp, iframe);

		if(ret == 0 && rtsp_get_range_type() == VOD_RANGE_TYPE_NPT)
		{
			if(aud_or_vid == 0)
			{
				if(rtsp_get_recv_type() != VOD_RECV_TYPE_QAM && *timestamp <= g_last_atimestamp &&  g_last_atimestamp  > 4500 && *timestamp > ( g_last_atimestamp -4500)  )
				{
					/*
					   此处主要解决暂停恢复后重复数据的问题
					   这种重复数据的时间在3妙以内，如果超出这个值
					   应该有特殊情况发生，小心处理
					   */
					*buffer = NULL;
					*len = 0;
					if ( vod_porting_get_ms () - tbegin > 100 )
					{
						return 1;
					}
					else						
					{
						goto reget;
					}
				}
				else
				{
					g_last_atimestamp = *timestamp;
				}
			}
			else
			{
				if(g_locate_iframe_flag == 1)
				{
					if(*iframe != 1)
					{
						*buffer = NULL;
						*len = 0;
						return 1;
					}
					else
					{
						g_locate_iframe_flag = 0;
					}
				}
				if(rtsp_get_recv_type() != VOD_RECV_TYPE_QAM && *timestamp <=  g_last_vtimestamp  &&  g_last_vtimestamp  > 4000 && *timestamp > ( g_last_vtimestamp -4000))
				{
					*buffer = NULL;
					*len = 0;
					if ( vod_porting_get_ms () - tbegin > 100 )
					{
						return 1;
					}
					else						
					{
						goto reget;
					}
				}
				else
				{
					g_last_vtimestamp = *timestamp;
				}
			}
		}
#endif
	}
	else if(VOD_TRANS_TYPE_TS == trans_type)
	{
		ret = vod_trans_ts_get_frame(aud_or_vid, buffer, len, timestamp, iframe);
	}

	return ret;
}


void vod_trans_set_bufsize(int32_t size)
{
	if(VOD_TRANS_TYPE_ISMA == trans_type)
	{
		//vod_trans_isma_set_bufsize(size);
	}
	else if(VOD_TRANS_TYPE_TS == trans_type)
	{
		vod_trans_ts_set_bufsize(size);
	}
}

int32_t vod_trans_get_bufsize()
{
	if(VOD_TRANS_TYPE_ISMA == trans_type)
	{
		//return vod_trans_isma_get_bufsize();
		return -1;
	}
	else if(VOD_TRANS_TYPE_TS == trans_type)
	{
		return vod_trans_ts_get_bufsize();
	}
	return -1;
}

uint32_t vod_trans_get_v_packnum()
{
	uint32_t vnum = 0;

	if(VOD_TRANS_TYPE_ISMA == trans_type)
	{
		//vnum = vod_trans_isma_get_v_packnum();
	}
	else if(VOD_TRANS_TYPE_TS == trans_type)
	{
		vnum = vod_trans_ts_get_v_packnum();
	}

	return vnum;
}

uint32_t vod_trans_get_a_packnum()
{
	uint32_t anum = 0;

	if(VOD_TRANS_TYPE_ISMA == trans_type)
	{
		//anum = vod_trans_isma_get_a_packnum();
	}
	else if(VOD_TRANS_TYPE_TS == trans_type)
	{
		anum = vod_trans_ts_get_a_packnum();
	}

	return anum;
}

void vod_trans_clean()
{
	if(VOD_TRANS_TYPE_ISMA == trans_type)
	{
		//vod_trans_isma_clean();
	}
	else if(VOD_TRANS_TYPE_TS == trans_type)
	{
		vod_trans_ts_clean();
	}
	g_last_vtimestamp = 0;
	g_last_atimestamp = 0;
}

void vod_trans_remove_duplicates()
{
	if(VOD_TRANS_TYPE_ISMA == trans_type)
	{
		//vod_trans_isma_remove_duplicates();
	}
	else if(VOD_TRANS_TYPE_TS == trans_type)
	{
		/*vod_trans_ts_remove_duplicates();*/
	}
}


int32_t vod_trans_get_remove_duplicates_state()
{
	if(VOD_TRANS_TYPE_ISMA == trans_type)
	{
		//return vod_trans_isma_get_remove_duplicates_state();
	}
	else if(VOD_TRANS_TYPE_TS == trans_type)
	{
		/*vod_trans_ts_remove_duplicates();*/
		return 1;
	}
	return 1;
}

void vod_trans_set_locate_Ifame(void)
{
	g_locate_iframe_flag = 1;
}



uint32_t vod_trans_get_last_vpts(void)
{
	return g_last_vtimestamp;
}

uint32_t vod_trans_get_last_apts(void)
{
	return g_last_atimestamp;
}

int vod_trans_sync(char* buffer, int len)
{
	if(VOD_TRANS_TYPE_TS == trans_type)
	{
		return vod_trans_ts_sync(buffer, len);
	}
	else if(VOD_TRANS_TYPE_HWTS == trans_type)
	{
		return vod_trans_hwts_sync(buffer, len);
	}
	return -1;
}

int vod_trans_demux(uint32_t* handle)
{
	if(VOD_TRANS_TYPE_HWTS == trans_type)
	{
		return vod_trans_hwts_demux(handle);
	}

	return -1;
}

int vod_trans_get_transinfo(VOD_TRANS_TS_INFO* tsinfo)
{
	if(VOD_TRANS_TYPE_HWTS == trans_type)
	{
		return vod_trans_hwts_get_info(tsinfo);
	}
	return -1;
}
