/************************************************************
Copyright (C), 2007-2009, GX S&T Co., Ltd.
FileName   :    search_cache.c
Author     :    luyong
Version    :    1.0
Date       :
Description:
Version    :
History    :
Date            Author      Modification
2019.06.28      luyong      create
***********************************************************/

#include <stdio.h>
#include <string.h>
#include "gxcore.h"
#include "search_private.h"

bool thiz_SearchPrintEnable = false;
static SearchCacheCtrl thiz_SearchCacheCtrl = {
	.mutex = 0,
};

status_t search_cache_prog_add(GxBusPmDataProg prog)
{
	SatCache  *sat_pos   = NULL;
	TpCache   *tp_pos    = NULL;
	ProgCache *tmp_cache = NULL;

	GxCore_MutexLock(thiz_SearchCacheCtrl.mutex);

	gxlist_for_each_entry(sat_pos, &thiz_SearchCacheCtrl.sat_cache_list, list)
	{
		if(sat_pos->sat_id == prog.sat_id)
			break;
	}

	if(&sat_pos->list == &thiz_SearchCacheCtrl.sat_cache_list)
	{
		SEARCH_ERR("can't find sat cache node, please check params\n");
		GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);
		return GXCORE_ERROR;
	}

	gxlist_for_each_entry(tp_pos, &sat_pos->tp_cache_list, list)
	{
		if(tp_pos->tp.id == prog.tp_id)
			break;
	}

	if(&tp_pos->list == &sat_pos->tp_cache_list)
	{
		SEARCH_ERR("can't find tp cache node, please check params\n");
		GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);
		return GXCORE_ERROR;
	}

	tmp_cache = GxCore_Calloc(1, sizeof(ProgCache));
	if(NULL == tmp_cache)
	{
		SEARCH_ERR("No enough memory\n");
		GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);
		return GXCORE_ERROR;
	}

	tmp_cache->service_id       = prog.service_id;
	tmp_cache->data_plp_id      = prog.data_plp_id;
	tmp_cache->common_plp_exist = prog.common_plp_exist;
	tmp_cache->common_plp_id    = prog.common_plp_id;
	tmp_cache->common_status    = prog.common_status;
	tmp_cache->muti_ts_id       = prog.muti_ts_id;
	tmp_cache->muti_ts_code     = prog.muti_ts_code;
	tmp_cache->muti_ts_exist    = prog.muti_ts_exist;
	tmp_cache->pdmx_flag        = prog.pdmx_flag;
	tmp_cache->pdmx_pid         = prog.pdmx_pid;
	gxlist_add_tail(&tmp_cache->list, &tp_pos->prog_cache_list);

	GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);

	return GXCORE_SUCCESS;
}

status_t search_cache_tp_add(GxBusPmDataTP tp)
{
	TpCache  *tmp_cache = NULL;
	SatCache *sat_pos = NULL;

	GxCore_MutexLock(thiz_SearchCacheCtrl.mutex);

	gxlist_for_each_entry(sat_pos, &thiz_SearchCacheCtrl.sat_cache_list, list)
	{
		if(sat_pos->sat_id == tp.sat_id)
			break;
	}

	if(&sat_pos->list == &thiz_SearchCacheCtrl.sat_cache_list)
	{
		SEARCH_ERR("can't find sat cache node, please check params\n");
		GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);
		return GXCORE_ERROR;
	}

	tmp_cache = GxCore_Calloc(1, sizeof(TpCache));
	if(NULL == tmp_cache)
	{
		SEARCH_ERR("No enough memory\n");
		GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);
		return GXCORE_ERROR;
	}

	GX_INIT_LIST_HEAD(&tmp_cache->prog_cache_list);
	tmp_cache->tp = tp;
	gxlist_add_tail(&tmp_cache->list, &sat_pos->tp_cache_list);

	GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);

	return GXCORE_SUCCESS;
}

static void _prog_cache_list_destroy(TpCache *cache)
{
	ProgCache *pos = NULL;
	ProgCache *pos_n = NULL;

	if(NULL == cache)
		return;

	gxlist_for_each_entry_safe(pos, pos_n, &cache->prog_cache_list, list)
	{
		gxlist_del(&pos->list);
		SEARCH_FREE(pos);
	}
	return;
}

static void _tp_cache_list_destroy(SatCache *cache)
{
	TpCache *pos = NULL;
	TpCache *pos_n = NULL;

	if(NULL == cache)
		return;

	gxlist_for_each_entry_safe(pos, pos_n, &cache->tp_cache_list, list)
	{
		_prog_cache_list_destroy(pos);
		gxlist_del(&pos->list);
		SEARCH_FREE(pos);
	}

	return;
}

void search_cache_destroy(void)
{
	SatCache *pos = NULL;
	SatCache *pos_n = NULL;

	GxCore_MutexLock(thiz_SearchCacheCtrl.mutex);

	if(1 == gxlist_empty(&thiz_SearchCacheCtrl.sat_cache_list)
            || (thiz_SearchCacheCtrl.sat_cache_list.next == NULL
                && thiz_SearchCacheCtrl.sat_cache_list.prev == NULL))
	{
		GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);
		SEARCH_DBG("cache has been released");
		return;
	}

	gxlist_for_each_entry_safe(pos, pos_n, &thiz_SearchCacheCtrl.sat_cache_list, list)
	{
		_tp_cache_list_destroy(pos);
		gxlist_del(&pos->list);
		SEARCH_FREE(pos);
	}

	GX_INIT_LIST_HEAD(&thiz_SearchCacheCtrl.sat_cache_list);
	GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);

	return;
}

static TpCache *_tp_cache_get(uint32_t sat_id, uint32_t tp_id)
{
	TpCache  *tp_pos = NULL;
	SatCache *sat_pos = NULL;

	gxlist_for_each_entry(sat_pos, &thiz_SearchCacheCtrl.sat_cache_list, list)
	{
		if(sat_pos->sat_id == sat_id)
		{
			gxlist_for_each_entry(tp_pos, &sat_pos->tp_cache_list, list)
			{
				if(tp_pos->tp.id == tp_id)
					return tp_pos;
			}
		}
	}

	return NULL;
}

static status_t _prog_cache_list_create(uint32_t sat_id)
{
	ProgCache       *tmp_cache = NULL;
	GxBusPmDataProg *prog_array = NULL;
	status_t         ret = GXCORE_SUCCESS;
	GxBusPmViewInfo  view_info, view_info_bak;
	int32_t prog_num = 0, i = 0, real_num_get = 0;

	GxCore_MutexLock(thiz_SearchCacheCtrl.mutex);

	if(GXCORE_ERROR == GxBus_PmViewInfoGet(&view_info))
	{
		SEARCH_ERR("call GxBus_PmViewInfoGet failed\n");
		ret = GXCORE_ERROR;
		goto END;
	}

	memcpy(&view_info_bak, &view_info, sizeof(GxBusPmViewInfo));

	view_info.group_mode = GROUP_MODE_SAT;
	view_info.stream_type = GXBUS_PM_PROG_ALL;
	view_info.sat_id = sat_id;
	view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;

	if(GXCORE_ERROR == GxBus_PmViewInfoModify(&view_info))
	{
		SEARCH_ERR("call GxBus_PmViewInfoModify failed\n");
		ret = GXCORE_ERROR;
		goto END;
	}

	prog_num = GxBus_PmProgNumGet();
	if(-1 == prog_num)
	{
		SEARCH_ERR("call GxBus_PmProgNumGet failed\n");
		ret = GXCORE_ERROR;
		goto END;
	}

	if(0 == prog_num)
		goto END;

	prog_array = GxCore_Calloc(prog_num, sizeof(GxBusPmDataProg));
	if(NULL == prog_array)
	{
		SEARCH_ERR("No enough memory\n");
		ret = GXCORE_ERROR;
		goto END;
	}

	real_num_get = GxBus_PmProgGetByPos(0, prog_num, prog_array);
	if(-1 == real_num_get)
	{
		SEARCH_ERR("call GxBus_PmProgGetByPos failed\n");
		ret = GXCORE_ERROR;
		goto END;
	}

	for(i = 0; i < real_num_get; i++)
	{
		TpCache *tp_cache = NULL;

		tp_cache = _tp_cache_get(sat_id, prog_array[i].tp_id);
		if(NULL == tp_cache)
		{
			SEARCH_DBG("can't find tp cache node");
			continue;
		}

		tmp_cache = GxCore_Calloc(1, sizeof(ProgCache));
		if(NULL == tmp_cache)
		{
			SEARCH_ERR("No enough memory\n");
			ret = GXCORE_ERROR;
			goto END;
		}

		tmp_cache->service_id       = prog_array[i].service_id;
		tmp_cache->data_plp_id      = prog_array[i].data_plp_id;
		tmp_cache->common_plp_exist = prog_array[i].common_plp_exist;
		tmp_cache->common_plp_id    = prog_array[i].common_plp_id;
		tmp_cache->common_status    = prog_array[i].common_status;
		tmp_cache->muti_ts_id       = prog_array[i].muti_ts_id;
		tmp_cache->muti_ts_code     = prog_array[i].muti_ts_code;
		tmp_cache->muti_ts_exist    = prog_array[i].muti_ts_exist;
		tmp_cache->pdmx_flag        = prog_array[i].pdmx_flag;
		tmp_cache->pdmx_pid         = prog_array[i].pdmx_pid;
		gxlist_add_tail(&tmp_cache->list, &tp_cache->prog_cache_list);
	}

END:
	GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);
	GxBus_PmViewInfoModify(&view_info_bak);
	SEARCH_FREE(prog_array);
	return ret;
}

static status_t _tp_cache_list_create(uint32_t sat_id, uint32_t tp_num, uint32_t *tp_id_array)
{
	if(NULL == tp_id_array || 0 == tp_num)
		return GXCORE_ERROR;

	int32_t        i = 0;
	GxBusPmDataTP  tp = {0};
	SatCache      *pos = NULL;
	TpCache       *tmp_cache = NULL;
	status_t       ret = GXCORE_SUCCESS;

	GxCore_MutexLock(thiz_SearchCacheCtrl.mutex);
	gxlist_for_each_entry(pos, &thiz_SearchCacheCtrl.sat_cache_list, list)
	{
		if(pos->sat_id == sat_id)
			break;
	}

	if(&pos->list == &thiz_SearchCacheCtrl.sat_cache_list)
	{
		SEARCH_ERR("can't find sat cache node, please check params\n");
		GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);
		return GXCORE_ERROR;
	}

	for(i = 0; i < tp_num; i++)
	{
		tmp_cache = GxCore_Calloc(1, sizeof(TpCache));
		if(NULL == tmp_cache)
		{
			SEARCH_ERR("No enough memory\n");
			ret = GXCORE_ERROR;
			break;
		}

		if(GXCORE_ERROR == GxBus_PmTpGetById(tp_id_array[i], &tp))
		{
			SEARCH_ERR("call GxBus_PmTpGetById failed\n");
			ret = GXCORE_ERROR;
			break;
		}
		GX_INIT_LIST_HEAD(&tmp_cache->prog_cache_list);
		tmp_cache->tp = tp;
		gxlist_add_tail(&tmp_cache->list, &pos->tp_cache_list);
	}

	GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);
	return ret;
}

static status_t _tp_cache_list_create_by_satId(uint32_t sat_num, uint32_t *sat_id_array)
{
	status_t       ret = GXCORE_SUCCESS;
	uint32_t      *tp_id_array = NULL;
	GxBusPmDataTP *tp_array = NULL;
	int32_t tp_num = 0, real_num_get = 0, i = 0, j = 0;

	if(NULL == sat_id_array)
		return GXCORE_ERROR;

	for(i = 0; i < sat_num; i++)
	{
		tp_num = GxBus_PmTpNumGetBySat(sat_id_array[i]);
		if(-1 == tp_num)
		{
			SEARCH_ERR("call GxBus_PmTpNumGetBySat failed\n");
			ret = GXCORE_ERROR;
			goto END;
		}

		if(0 == tp_num)
			continue;

		tp_array = GxCore_Calloc(tp_num, sizeof(GxBusPmDataTP));
		if(NULL == tp_array)
		{
			SEARCH_ERR("No enough memory\n");
			ret = GXCORE_ERROR;
			goto END;
		}

		real_num_get = GxBus_PmTpGetByPosInSat(0, tp_num, tp_array);
		if(-1 == real_num_get)
		{
			SEARCH_ERR("call GxBus_PmTpGetByPosInSat failed\n");
			ret = GXCORE_ERROR;
			goto END;
		}

		tp_id_array = GxCore_Calloc(real_num_get, sizeof(int32_t));
		if(NULL == tp_id_array)
		{
			SEARCH_ERR("No enough memory\n");
			ret = GXCORE_ERROR;
			goto END;
		}

		for(j = 0; j < real_num_get; j++)
		{
			tp_id_array[j] = tp_array[j].id;
		}

		if(GXCORE_ERROR == _tp_cache_list_create(sat_id_array[i], real_num_get, tp_id_array))
		{
			ret = GXCORE_ERROR;
			goto END;
		}

		SEARCH_FREE(tp_array);
		SEARCH_FREE(tp_id_array);
	}

END:
	SEARCH_FREE(tp_array);
	SEARCH_FREE(tp_id_array);
	return ret;
}

static status_t _sat_cache_list_create(uint32_t sat_num, uint32_t *sat_id_array)
{
	if(NULL == sat_id_array || 0 == sat_num)
		return GXCORE_ERROR;

	int32_t         i = 0;
	GxBusPmDataSat  sat = {0};
	SatCache       *tmp_cache = NULL;
	status_t        ret = GXCORE_SUCCESS;

	GxCore_MutexLock(thiz_SearchCacheCtrl.mutex);

	for(i = 0; i < sat_num; i++)
	{
		tmp_cache = GxCore_Calloc(1, sizeof(SatCache));
		if(NULL == tmp_cache)
		{
			SEARCH_ERR("no enough memory\n");
			ret = GXCORE_ERROR;
			break;
		}

		if(GXCORE_ERROR == GxBus_PmSatGetById(sat_id_array[i], &sat))
		{
			SEARCH_ERR("call GxBus_PmSatGetById failed\n");
			ret = GXCORE_ERROR;
			break;
		}

		GX_INIT_LIST_HEAD(&tmp_cache->tp_cache_list);
		tmp_cache->sat_id = sat_id_array[i];
		tmp_cache->type = sat.type;
		gxlist_add_tail(&tmp_cache->list, &thiz_SearchCacheCtrl.sat_cache_list);
	}

	GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);
	return ret;
}

bool search_cache_prog_exist_check(uint32_t sat_id,
									uint32_t tp_id,
									uint16_t service_id,
									uint8_t muti_ts_id,
									uint8_t pdmx_flag,
									uint16_t pdmx_pid,
									uint8_t data_plp_id,
									uint8_t common_plp_exist,
									uint8_t common_plp_id,
									uint8_t common_status)
{
	TpCache   *tp_pos = NULL;
	SatCache  *sat_pos = NULL;
	ProgCache *prog_pos = NULL;

	GxCore_MutexLock(thiz_SearchCacheCtrl.mutex);

	gxlist_for_each_entry(sat_pos, &thiz_SearchCacheCtrl.sat_cache_list, list)
	{
		if(sat_pos->sat_id == sat_id)
		{
			gxlist_for_each_entry(tp_pos, &sat_pos->tp_cache_list, list)
			{
				if(tp_pos->tp.id == tp_id)
				{
					gxlist_for_each_entry(prog_pos, &tp_pos->prog_cache_list, list)
					{
						if(prog_pos->service_id == service_id
								&& prog_pos->data_plp_id == data_plp_id
								&& prog_pos->common_plp_exist == common_plp_exist
								&& prog_pos->common_plp_id == common_plp_id
								&& prog_pos->common_status == common_status
								&& prog_pos->muti_ts_id == muti_ts_id
								&& prog_pos->pdmx_flag == pdmx_flag
								&& prog_pos->pdmx_pid == pdmx_pid)
						{
							GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);
							return true;
						}
					}
				}
			}
		}
	}

	GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);
	return false;
}

bool search_cache_tp_exist_check(uint32_t sat_id,
									uint32_t fre,
									uint32_t symbol,
									GxBusPmTpPolar polar,
									fe_modulation_t modulation,
									uint16_t* tp_id)
{
	uint32_t        fre_temp = 0;
	uint32_t        symbol_temp = 0;
	TpCache        *tp_pos = NULL;
	SatCache       *sat_pos = NULL;
	GxBusPmTpPolar  polar_temp = GXBUS_PM_TP_POLAR_H;
	fe_modulation_t modulation_temp = (fe_modulation_t)0;

	GxCore_MutexLock(thiz_SearchCacheCtrl.mutex);

	gxlist_for_each_entry(sat_pos, &thiz_SearchCacheCtrl.sat_cache_list, list)
	{
		if(sat_pos->sat_id == sat_id)
			break;
	}

	if(&sat_pos->list == &thiz_SearchCacheCtrl.sat_cache_list)
	{
		SEARCH_ERR("can't find sat cache node, please check params\n");
		GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);
		return false;
	}

	gxlist_for_each_entry(tp_pos, &sat_pos->tp_cache_list, list)
	{
		fre_temp = tp_pos->tp.frequency;

		if(sat_pos->type == GXBUS_PM_SAT_S)
		{
			symbol_temp = tp_pos->tp.tp_s.symbol_rate;
			polar_temp = tp_pos->tp.tp_s.polar;
			modulation_temp = modulation;

           //Hardware engineers say the following algorithms are used
			uint8_t FreInter = 5;

			if(symbol > 20000)
				FreInter = (symbol+2000)/4000;
			else if(symbol < 10000)
				FreInter = (symbol+1000)/2000;

			if((fre_temp-FreInter) <= fre
					&& (fre_temp+FreInter) >= fre
					&& (symbol_temp-100) < symbol
					&& (symbol_temp+100)> symbol)//fre和symbol的判断是有个范围的
			{
				fre_temp = fre;
				symbol_temp = symbol;
			}
		}
		else if(sat_pos->type == GXBUS_PM_SAT_C)
		{
			symbol_temp = symbol;
			polar_temp = polar;
			modulation_temp = tp_pos->tp.tp_c.modulation;

			if((fre_temp-3) < fre && (fre_temp+3) > fre)
				fre_temp = fre;
		}
		else if(sat_pos->type == GXBUS_PM_SAT_T
				|| sat_pos->type == GXBUS_PM_SAT_DVBT2)
		{
			symbol_temp = symbol;
			polar_temp = polar;
			modulation_temp = modulation;
		}
		else if(sat_pos->type == GXBUS_PM_SAT_DTMB)
		{
			symbol_temp = symbol;
			polar_temp = polar;
			modulation_temp = modulation;
		}
		else if(sat_pos->type == GXBUS_PM_SAT_ATSC_C)
		{
			;//等待demod参数 ATSC wait
		}
		else if(sat_pos->type == GXBUS_PM_SAT_ATSC_T)
		{
			;//等待demod参数 ATSC wait
		}

		if(tp_pos->tp.sat_id == sat_id
				&& fre_temp == fre
				&& symbol_temp == symbol
				&& polar_temp == polar
				&& modulation_temp == modulation)
		{
			*tp_id = tp_pos->tp.id;
			GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);
			return true;
		}
	}

	GxCore_MutexUnlock(thiz_SearchCacheCtrl.mutex);

	return false;
}

status_t search_cache_init(uint32_t sat_num, uint32_t *sat_id_array, uint32_t tp_num, uint32_t *tp_id_array)
{
	int32_t i = 0;

	if(NULL == sat_id_array && NULL == tp_id_array)
		return GXCORE_ERROR;

	GX_INIT_LIST_HEAD(&thiz_SearchCacheCtrl.sat_cache_list);

	if(0 == thiz_SearchCacheCtrl.mutex)
		GxCore_MutexCreate(&thiz_SearchCacheCtrl.mutex);

	if(tp_id_array != NULL && tp_num != 0 && 1 == sat_num)//tp search
	{
		if(GXCORE_ERROR == _sat_cache_list_create(1, sat_id_array))
			goto err;

		if(GXCORE_ERROR == _tp_cache_list_create(sat_id_array[0], tp_num, tp_id_array))
			goto err;

		if(GXCORE_ERROR == _prog_cache_list_create(sat_id_array[0]))
			goto err;
	}
	else if(NULL == tp_id_array && 0 == tp_num)//satellite/blind search
	{
		if(GXCORE_ERROR == _sat_cache_list_create(sat_num, sat_id_array))
			goto err;

		if(GXCORE_ERROR == _tp_cache_list_create_by_satId(sat_num, sat_id_array))
			goto err;

		for(i = 0; i < sat_num; i++)
		{
			if(GXCORE_ERROR == _prog_cache_list_create(sat_id_array[i]))
				goto err;
		}
	}

	return GXCORE_SUCCESS;
err:
	search_cache_destroy();
	return GXCORE_ERROR;
}
