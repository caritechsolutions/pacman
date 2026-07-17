/*
    20240722
*/

#include "app_config.h"

#if RICHEPG_SUPPORT

#include "app.h"
#include "app_eutel_porting.h"
#include "app_eutel_database.h"
#include "app_eutel_sat_fuse.h"

#include "app_windows.h"
#include "app_dvbs_sat_opt.h"
#include "module/app_ioctl.h"
#include "app_wnd_search.h"
#include "full_screen.h"

extern int ex_sat_search_mode;

/*
--------- dvbs sat ops
*/

#define BUFFER_LENGTH_SAT	(MAX_SAT_NAME+20)
#define BUFFER_LENGTH_TP	(64)

static int _eutel_check_app_dialog(void)
{
	if ( (GUI_CheckDialog(WND_SAT_LIST) == GXCORE_SUCCESS)
		|| (GUI_CheckDialog(WND_TP_LIST) == GXCORE_SUCCESS) )
	{
		return 1;
	}
	else
		return 0;
}

static void _eutel_fuse_work_mode(int sat_id)
{
	int type ;

	type = app_eutel_porting_get_sat_type(sat_id);
	if (type == APP_SAT_TYPE_IS_FREE)
		app_eutel_porting_set_sat_search_mode(0) ;
	else if (type == APP_SAT_TYPE_IS_FUSE)
		app_eutel_porting_set_sat_search_mode(1) ;
	else if (type == APP_SAT_TYPE_IS_EUTEL)
		app_eutel_porting_set_sat_search_mode(1) ;
}

static uint32_t _eutel_dvbs_sat_num_get(uint32_t tuner)
{   
    uint32_t i;
    uint32_t sat_count = 0;
    GxMsgProperty_NodeNumGet node_num = {0};

    node_num.node_type = NODE_SAT;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
    if(node_num.node_num == 0)
        return 0;

    GxMsgProperty_NodeByPosGet node_sat = {0};
    for(i = 0; i < node_num.node_num; i++)
    {   
        node_sat.node_type = NODE_SAT;
        node_sat.pos = i;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);
        if(GXBUS_PM_SAT_S != node_sat.sat_data.type
                || (node_sat.sat_data.tuner != tuner
                    && tuner != TUNER_ALL))
            continue;

		if(app_eutel_sat_type_check(node_sat.sat_data.id))
		{
			if ( _eutel_check_app_dialog() )
				continue ;
			if ( app_eutel_sat_fuse_free_sat_is_point(node_sat.sat_data.id) )
				continue ;
		}

        sat_count++;
    }

    return sat_count;
}

static uint32_t _eutel_dvbs_cur_sat_tp_num_get(void)
{
    GxMsgProperty_NodeNumGet node_num = {0};
    node_num.node_type = NODE_TP;

    if ( app_eutel_porting_get_sat_search_mode() )
    {
		return app_eutel_sat_fuse_get_all_area_count(s_dvbs_sat_data.cur_sat.sat_data.id,NULL);
    }
    else
    {
        node_num.sat_id = s_dvbs_sat_data.cur_sat.sat_data.id;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
    }

    return node_num.node_num;
}

static void _eutel_dvbs_cur_sat_get(void)
{
	GxMsgProperty_NodeByPosGet node_sat = {0};
	uint32_t cur_tuner = s_dvbs_sat_data.cur_tuner;

	memset(&s_dvbs_sat_data.cur_sat, 0, sizeof(GxMsgProperty_NodeByPosGet));
	s_dvbs_sat_data.sat_total = s_dvbs_sat_data.sat_num_get(s_dvbs_sat_data.cur_tuner);
	if(0 == s_dvbs_sat_data.sat_total)
	{
		s_dvbs_sat_data.cur_sat_sel[cur_tuner] = 0;
		return;
	}

	if(s_dvbs_sat_data.cur_sat_sel[cur_tuner] >= s_dvbs_sat_data.sat_total
            || s_dvbs_sat_data.cur_sat_sel[cur_tuner] < 0)
		s_dvbs_sat_data.cur_sat_sel[cur_tuner] = 0;

	node_sat.node_type = NODE_SAT;
	node_sat.pos = s_dvbs_sat_data.sat_buf[s_dvbs_sat_data.cur_sat_sel[cur_tuner]];
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);
	memcpy(&s_dvbs_sat_data.cur_sat, &node_sat, sizeof(GxMsgProperty_NodeByPosGet));

	s_dvbs_sat_data.sat_id_bak = node_sat.sat_data.id;

	if ( !_eutel_check_app_dialog() )
	{
		_eutel_fuse_work_mode(s_dvbs_sat_data.cur_sat.sat_data.id);
	}

	return;
}

static void _eutel_dvbs_cur_tp_get(void)
{
	memset(&s_dvbs_sat_data.cur_tp, 0, sizeof(GxMsgProperty_NodeByPosGet));

	s_dvbs_sat_data.cur_sat_tp_total = s_dvbs_sat_data.cur_sat_tp_num_get();
	if(0 == s_dvbs_sat_data.cur_sat_tp_total)
	{
		s_dvbs_sat_data.cur_tp_sel = 0;
		return;
	}

	if(s_dvbs_sat_data.cur_tp_sel >= s_dvbs_sat_data.cur_sat_tp_total || s_dvbs_sat_data.cur_tp_sel < 0)
		s_dvbs_sat_data.cur_tp_sel = 0;

	if (app_eutel_porting_get_sat_search_mode())
	{
		unsigned int fre ,symbol,polar ;
		int count = 0 ;
		unsigned int area[EUTUL_SAT_MAX_NUM] ;

		count = app_eutel_sat_fuse_get_all_area_count(s_dvbs_sat_data.cur_sat.sat_data.id,area);
		if ( (count <=0) || (s_dvbs_sat_data.cur_tp_sel >= count) )
			return ;

		app_eutel_get_tp_by_sel(area[s_dvbs_sat_data.cur_tp_sel],&fre,&symbol,&polar);

		s_dvbs_sat_data.cur_tp.pos = s_dvbs_sat_data.cur_tp_sel;
		s_dvbs_sat_data.cur_tp.tp_data.frequency = fre ;
		s_dvbs_sat_data.cur_tp.tp_data.tp_s.symbol_rate = symbol;
		s_dvbs_sat_data.cur_tp.tp_data.tp_s.polar = polar ;

		return;
	}

	GxMsgProperty_NodeByPosGet node_tp = {0};
	node_tp.node_type = NODE_TP;
	node_tp.pos = s_dvbs_sat_data.cur_tp_sel;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp);
	memcpy(&s_dvbs_sat_data.cur_tp, &node_tp, sizeof(GxMsgProperty_NodeByPosGet));

	return;
}

static void _eutel_dvbs_sat_data_rebuild(char *cmb_name, bool tuner_show)
{
	int i = 0;
	int sat_count = 0;
	int cur_sel_reset = 0;
    uint32_t cur_tuner = 0;
    char *buffer_all = NULL;
    char buffer[BUFFER_LENGTH_SAT] = {0};
	GxMsgProperty_NodeNumGet sat_num = {0};
	GxMsgProperty_NodeByPosGet node_sat = {0};

	app_eutel_sat_fuse_build();

	memset(&s_dvbs_sat_data.cur_sat, 0, sizeof(GxMsgProperty_NodeByPosGet));
	memset(s_dvbs_sat_data.sat_buf, 0, SYS_MAX_SAT * sizeof(uint32_t));

    cur_tuner = s_dvbs_sat_data.cur_tuner;
    cur_sel_reset = s_dvbs_sat_data.cur_sat_sel[cur_tuner];

    s_dvbs_sat_data.sat_total = s_dvbs_sat_data.sat_num_get(s_dvbs_sat_data.cur_tuner);

	if(0 == s_dvbs_sat_data.sat_total)
	{
		s_dvbs_sat_data.cur_sat_sel[cur_tuner] = 0;
		GUI_SetProperty(cmb_name, "content", "[NONE]");
		return;
	}

	//get total sat num in cur tuner mode
	sat_num.node_type = NODE_SAT;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);

    if(NULL != cmb_name)
    {
        buffer_all = GxCore_Calloc(s_dvbs_sat_data.sat_total, BUFFER_LENGTH_SAT);
        if(NULL == buffer_all)
        {
            app_log_error("\033[31m!!!!!!!!!![%s, %d]:GxCore_Calloc failed!!!!!!!!!!!\033[0m\n", __func__, __LINE__);
			s_dvbs_sat_data.cur_sat_sel[cur_tuner] = 0;
			s_dvbs_sat_data.sat_total = 0;
            return;
        }
		strcat(buffer_all, "[");

		for(i = 0; i < sat_num.node_num; i++)
		{
			node_sat.node_type = NODE_SAT;
			node_sat.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);
            if(GXBUS_PM_SAT_S != node_sat.sat_data.type
                    || (node_sat.sat_data.tuner != cur_tuner
                        && cur_tuner != TUNER_ALL))
				continue;

			if(app_eutel_sat_type_check(node_sat.sat_data.id))
			{
				if ( _eutel_check_app_dialog() )
					continue ;
				if ( app_eutel_sat_fuse_free_sat_is_point(node_sat.sat_data.id) )
					continue ;

				unsigned int area[EUTUL_SAT_MAX_NUM];

				if ( app_eutel_sat_fuse_get_sattv_area_count(node_sat.sat_data.id,area) > 1)
				{
					unsigned char *pname = NULL ;
					int len = 0 ;
					pname = app_eutel_sat_get_display_name(area[0],&len);
					if ( (pname != NULL ) && (len >0) )
					{
						node_sat.sat_data.sat_s.sat_name[len+1] = 0 ;
					}
				}
			}

			s_dvbs_sat_data.sat_buf[sat_count++] = i;
			memset(buffer, 0, BUFFER_LENGTH_SAT);
            if(true == tuner_show)
            {
                if(TUNER1 == node_sat.sat_data.tuner)
                    sprintf(buffer,"(%d/%d) T1 %s,", sat_count, s_dvbs_sat_data.sat_total, (char*)node_sat.sat_data.sat_s.sat_name);
                else if(TUNER2 == node_sat.sat_data.tuner)
                    sprintf(buffer,"(%d/%d) T2 %s,", sat_count, s_dvbs_sat_data.sat_total, (char*)node_sat.sat_data.sat_s.sat_name);
                else
                    sprintf(buffer,"(%d/%d) %s,", sat_count, s_dvbs_sat_data.sat_total, (char*)node_sat.sat_data.sat_s.sat_name);
            }
            else
                sprintf(buffer,"(%d/%d) %s,", sat_count, s_dvbs_sat_data.sat_total, (char*)node_sat.sat_data.sat_s.sat_name);

			strcat(buffer_all, buffer);

			// give the first or selected sat value to cur_sat
			if (sat_count - 1 == 0
			        || sat_count - 1 == s_dvbs_sat_data.cur_sat_sel[cur_tuner])
			{
				cur_sel_reset = sat_count - 1;
				memcpy(&s_dvbs_sat_data.cur_sat, &node_sat, sizeof(GxMsgProperty_NodeByPosGet));
                s_dvbs_sat_data.sat_id_bak = node_sat.sat_data.id;
			}
		}
		memcpy(buffer_all+strlen(buffer_all)-1, "]", 1);
		GUI_SetProperty(cmb_name, "content", buffer_all);
		GxCore_Free(buffer_all);
    }
    else
    {
		for(i = 0; i < sat_num.node_num; i++)
		{
			node_sat.node_type = NODE_SAT;
			node_sat.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);
            if(GXBUS_PM_SAT_S != node_sat.sat_data.type
                    || (node_sat.sat_data.tuner != cur_tuner
                        && cur_tuner != TUNER_ALL))
				continue;

			if(app_eutel_sat_type_check(node_sat.sat_data.id))
			{
				if ( _eutel_check_app_dialog() )
					continue ;
				if ( app_eutel_sat_fuse_free_sat_is_point(node_sat.sat_data.id) )
					continue ;
			}

			s_dvbs_sat_data.sat_buf[sat_count++] = i;

			// give the first or selected sat value to cur_sat
			if (sat_count - 1 == 0
					|| sat_count - 1 == s_dvbs_sat_data.cur_sat_sel[cur_tuner])
			{
				cur_sel_reset = sat_count -1;
				memcpy(&s_dvbs_sat_data.cur_sat, &node_sat, sizeof(GxMsgProperty_NodeByPosGet));
                s_dvbs_sat_data.sat_id_bak = node_sat.sat_data.id;
			}
		}
    }

	s_dvbs_sat_data.cur_sat_sel[cur_tuner] = cur_sel_reset;

	if ( !_eutel_check_app_dialog() )
	{
		_eutel_fuse_work_mode(s_dvbs_sat_data.cur_sat.sat_data.id);
	}

	return;
}

static void _eutel_dvbs_tp_data_rebuild(char *cmb_name)
{
    int i = 0;
    int cur_sel_reset = 0;
    char *buffer_all = NULL;
    uint32_t sat_id_bak = 0;
    char buffer[BUFFER_LENGTH_TP] = {0};
    GxMsgProperty_NodeByPosGet node_tp = {0};

    sat_id_bak = s_dvbs_sat_data.sat_id_bak;
    // get tp number
    s_dvbs_sat_data.cur_sat_tp_total = s_dvbs_sat_data.cur_sat_tp_num_get();

	if ( app_eutel_porting_get_sat_search_mode() )
	{
		unsigned int fre = 0,symbol = 0 ,polar = 0;
		int count  = 0 ;
		unsigned int area[EUTUL_SAT_MAX_NUM] ;

		count = app_eutel_sat_fuse_get_all_area_count(s_dvbs_sat_data.cur_sat.sat_data.id,area);
		if (count <=0 )
			return ;

		buffer_all = GxCore_Calloc(count, BUFFER_LENGTH_TP);
		if(NULL == buffer_all)
		{
			app_log_error("\033[31m!!!!!!!!!![%s, %d]:GxCore_Calloc failed!!!!!!!!!!!\033[0m\n", __func__, __LINE__);
			return;
		}
		strcat(buffer_all, "[");

		for(i=0;i<count;i++)
		{
			app_eutel_get_tp_by_sel(area[i],&fre,&symbol,&polar);

			memset(buffer, 0, BUFFER_LENGTH_TP);
			if(GXBUS_PM_TP_POLAR_V == polar)
			{
				sprintf(buffer,"(%d/%d) %d / %s / %d,",i+1,count,fre,"V",symbol);
			}
			else if(GXBUS_PM_TP_POLAR_H == polar)
			{
				sprintf(buffer,"(%d/%d) %d / %s / %d,",i+1,count,fre,"H",symbol);
			}
			strcat(buffer_all, buffer);
		}
		s_dvbs_sat_data.cur_tp_sel = 0 ;

		s_dvbs_sat_data.cur_tp.pos = s_dvbs_sat_data.cur_tp_sel;
		s_dvbs_sat_data.cur_tp.tp_data.frequency = fre ;
		s_dvbs_sat_data.cur_tp.tp_data.tp_s.symbol_rate = symbol;
		s_dvbs_sat_data.cur_tp.tp_data.tp_s.polar = polar ;

		strcat(buffer_all, "]");
		GUI_SetProperty(cmb_name, "content", buffer_all);
		GxCore_Free(buffer_all);

		return ;
	}

    if(0 == s_dvbs_sat_data.cur_sat_tp_total)
    {
        s_dvbs_sat_data.cur_tp_sel = 0;
        if(cmb_name != NULL)
		{
			GUI_SetProperty(cmb_name, "content", "[NONE]");
		}
        return;
    }

    if(cmb_name != NULL)
    {
        buffer_all = GxCore_Calloc(s_dvbs_sat_data.cur_sat_tp_total, BUFFER_LENGTH_TP);
        if(NULL == buffer_all)
        {
            app_log_error("\033[31m!!!!!!!!!![%s, %d]:GxCore_Calloc failed!!!!!!!!!!!\033[0m\n", __func__, __LINE__);
            s_dvbs_sat_data.cur_tp_sel = 0;
            return;
        }
        strcat(buffer_all, "[");

        for(i = 0; i < s_dvbs_sat_data.cur_sat_tp_total; i++)
        {
            node_tp.node_type = NODE_TP;
            node_tp.pos = i;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp);

            memset(buffer, 0, BUFFER_LENGTH_TP);

            if(GXBUS_PM_TP_POLAR_V == node_tp.tp_data.tp_s.polar)
            {
                sprintf(buffer,"(%d/%d) %d / %s / %d,",
                                    i + 1,
                                    s_dvbs_sat_data.cur_sat_tp_total,
                                    node_tp.tp_data.frequency,
                                    "V",
                                    node_tp.tp_data.tp_s.symbol_rate);
            }
            else if(GXBUS_PM_TP_POLAR_H == node_tp.tp_data.tp_s.polar)
            {
                sprintf(buffer,"(%d/%d) %d / %s / %d,",
                                    i + 1,
                                    s_dvbs_sat_data.cur_sat_tp_total,
                                    node_tp.tp_data.frequency,
                                    "H",
                                    node_tp.tp_data.tp_s.symbol_rate);
            }

            strcat(buffer_all, buffer);
            // give the first or selected tp value to cur_tp
            if(i == s_dvbs_sat_data.cur_tp_sel)
            {
                if((sat_id_bak==0)||(sat_id_bak == s_dvbs_sat_data.cur_sat.sat_data.id))
                    cur_sel_reset = i;

                memcpy(&s_dvbs_sat_data.cur_tp, &node_tp, sizeof(GxMsgProperty_NodeByPosGet));
            }
        }
        strcat(buffer_all, "]");
        GUI_SetProperty(cmb_name, "content", buffer_all);
        GxCore_Free(buffer_all);
    }
    else
    {
        for(i = 0; i < s_dvbs_sat_data.cur_sat_tp_total; i++)
        {
            // give the first or selected tp value to cur_tp
            if(i == s_dvbs_sat_data.cur_tp_sel)
            {
                node_tp.node_type = NODE_TP;
                node_tp.pos = i;
                app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp);
                memcpy(&s_dvbs_sat_data.cur_tp, &node_tp, sizeof(GxMsgProperty_NodeByPosGet));

                if((sat_id_bak==0)||(sat_id_bak == s_dvbs_sat_data.cur_sat.sat_data.id))
                    cur_sel_reset = i;

            }
        }
    }

    if(cur_sel_reset != s_dvbs_sat_data.cur_tp_sel)
    {
        s_dvbs_sat_data.cur_tp_sel = cur_sel_reset;
        s_dvbs_sat_data.cur_tp_get();
    }

    return;
}

static int _eutel_dvbs_tp_poplist_get_info(char **p_tp_info)
{
	int i;
	char **tp_info = NULL ;
	GxMsgProperty_NodeByPosGet node;

	if (NULL == p_tp_info)
	{
		app_log_error("tp_pop, params error...%s,%d\n",__FILE__,__LINE__);
		return -1;
	}

	if(0 == s_dvbs_sat_data.cur_sat_tp_total)
	{
		app_log_error("tp_pop, tp total error...%s,%d\n",__FILE__,__LINE__);
		return -1;
	}

	tp_info = (char**)GxCore_Calloc(s_dvbs_sat_data.cur_sat_tp_total, sizeof(char*));
	if(NULL == tp_info)
	{
		app_log_error("\033[31m!!!!!!!!!![%s, %d]:GxCore_Calloc failed!!!!!!!!!!!\033[0m\n", __func__, __LINE__);
		return -1;
	}

	if ( app_eutel_porting_get_sat_search_mode() )
	{
		unsigned int fre = 0,symbol = 0 ,polar = 0;
		int count  = 0 ;
		unsigned int area[EUTUL_SAT_MAX_NUM] ;

		count = app_eutel_sat_fuse_get_all_area_count(s_dvbs_sat_data.cur_sat.sat_data.id,area);
		if (count != s_dvbs_sat_data.cur_sat_tp_total )
		{
			app_log_error("tp_pop, tp total %d != %d\n",count,s_dvbs_sat_data.cur_sat_tp_total);
			GxCore_Free(tp_info);
			return -1;
		}

		for(i = 0; i < s_dvbs_sat_data.cur_sat_tp_total; i++)
		{
			app_eutel_get_tp_by_sel(area[i],&fre,&symbol,&polar);

			tp_info[i] = (char*)GxCore_Malloc(BUFFER_LENGTH_TP);
			if(NULL == tp_info[i])
			{
				app_log_error("\033[31m!!!!!!!!!![%s, %d]:GxCore_Calloc failed!!!!!!!!!!!\033[0m\n", __func__, __LINE__);
				if(tp_info != NULL)
				{
					for(i = 0; i < s_dvbs_sat_data.cur_sat_tp_total; i++)
					{
						if(tp_info[i] != NULL)
							GxCore_Free(tp_info[i]);
					}
					GxCore_Free(tp_info);
				}
				return -1;
			}
			if(polar == GXBUS_PM_TP_POLAR_V)
				sprintf(tp_info[i],"%d / %s / %d", fre,"V",symbol);
			else if(polar == GXBUS_PM_TP_POLAR_H)
				sprintf(tp_info[i],"%d / %s / %d", fre,"H",symbol);
		}
	}
	else
	{
		for(i = 0; i < s_dvbs_sat_data.cur_sat_tp_total; i++)
		{
			node.node_type = NODE_TP;
			node.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

			tp_info[i] = (char*)GxCore_Malloc(BUFFER_LENGTH_TP);
			if(NULL == tp_info[i])
			{
				app_log_error("\033[31m!!!!!!!!!![%s, %d]:GxCore_Calloc failed!!!!!!!!!!!\033[0m\n", __func__, __LINE__);
				if(tp_info != NULL)
				{
					for(i = 0; i < s_dvbs_sat_data.cur_sat_tp_total; i++)
					{
						if(tp_info[i] != NULL)
							GxCore_Free(tp_info[i]);
					}
					GxCore_Free(tp_info);
				}
				return -1;
			}
			if(node.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_V)
				sprintf(tp_info[i],"%d / %s / %d", node.tp_data.frequency, "V" ,node.tp_data.tp_s.symbol_rate);
			else if(node.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_H)
				sprintf(tp_info[i],"%d / %s / %d", node.tp_data.frequency, "H" ,node.tp_data.tp_s.symbol_rate);
		}
	}

	*p_tp_info = (char*)tp_info;

	return 0;
}

int  app_eutel_porting_sat_opt_init(void)
{
	s_dvbs_sat_data.sat_num_get        = _eutel_dvbs_sat_num_get ;
	s_dvbs_sat_data.cur_sat_tp_num_get = _eutel_dvbs_cur_sat_tp_num_get ;
	s_dvbs_sat_data.sat_rebuild = _eutel_dvbs_sat_data_rebuild ;
	s_dvbs_sat_data.tp_rebuild  = _eutel_dvbs_tp_data_rebuild ;
	s_dvbs_sat_data.cur_sat_get = _eutel_dvbs_cur_sat_get ;
	s_dvbs_sat_data.cur_tp_get  = _eutel_dvbs_cur_tp_get ;
	s_dvbs_sat_data.tp_poplist_get_info  = _eutel_dvbs_tp_poplist_get_info ;

	return 0 ;
}

#endif

