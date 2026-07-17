#include "app.h"
#if (DEMOD_DVB_S > 0)

#include "app_wnd_search.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "module/app_frontend.h"
#include "app_pop.h"
#include "app_default_params.h"
#include "full_screen.h"
#include "app_dvbs_sat_opt.h"

#if TKGS_SUPPORT
#include "module/tkgs/gxtkgs.h"
#endif

#define BUFFER_LENGTH_SAT	(MAX_SAT_NAME+20)
#define BUFFER_LENGTH_TP	(64)

extern int ex_sat_search_mode;
#if UNICABLE_SUPPORT
extern int app_unicable_param_check(GxBusPmDataSat sat);
#endif

static uint32_t _dvbs_sat_gui_sel_get_by_pos(uint32_t sat_pos)
{
	uint32_t i = 0;
	for(i = 0; i < s_dvbs_sat_data.sat_total; i++)
	{
		if(sat_pos == s_dvbs_sat_data.sat_buf[i])
			return i;
	}
	return 0;
}

static uint32_t _dvbs_sat_gui_sel_get_by_id(uint32_t sat_id)
{
    uint32_t i = 0;
    uint32_t ret_sel = 0;
	GxMsgProperty_NodeByPosGet node_sat = {0};

    for(i = 0; i < s_dvbs_sat_data.sat_total; i++)
    {
		node_sat.node_type = NODE_SAT;
		node_sat.pos = s_dvbs_sat_data.sat_buf[i];
		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat)
                && sat_id == node_sat.sat_data.id)
        {
            ret_sel = i;
            break;
        }
    }

    return ret_sel;
}

static void _dvbs_cur_tuner_sat_gui_sel_update(void)
{
    uint32_t cur_sat_pos = s_dvbs_sat_data.cur_sat.pos;
    s_dvbs_sat_data.sat_rebuild(NULL, false);
    s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner] = s_dvbs_sat_data.sat_gui_sel_get_by_pos(cur_sat_pos);

    return;
}


int app_clean_signal(char* progbar_strength, char* text_strength, char* progbar_quality, char* text_quality)
{
	uint32_t value = 0;
	SignalBarWidget siganl_bar = {0};
	SignalValue siganl_val = {0};
	char buffer[10] = {0};

	// progbar strength
	siganl_bar.strength_bar = progbar_strength;
	siganl_val.strength_val = value;

	// strength value
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer,"%d%%", value);
	GUI_SetProperty(text_strength, "string", buffer);

	// progbar quality
	siganl_bar.quality_bar = progbar_quality;
	siganl_val.quality_val = value;

	// quality value
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%d%%", value);
	GUI_SetProperty(text_quality, "string", buffer);

	app_signal_progbar_update(s_dvbs_sat_data.cur_sat.sat_data.tuner, &siganl_bar, &siganl_val);

    app_panel_ioctl_handle(PANEL_SHOW_QUALITY, &value);

	return 0;
}

int app_show_signal(char* progbar_strength, char* text_strength, char* progbar_quality, char* text_quality)
{
	static uint32_t value_flag = 1;

	uint32_t value = 0;
	char buffer[20] = {0};
	SignalBarWidget siganl_bar = {0};
	SignalValue siganl_val = {0};

	if(s_dvbs_sat_data.cur_sat_tp_total > 0)
	{
		value_flag = 1;

		// progbar strength
		app_ioctl(s_dvbs_sat_data.cur_sat.sat_data.tuner, FRONTEND_STRENGTH_GET, &value);
		if(value > 99)
			value = 100;
		siganl_bar.strength_bar = progbar_strength;
		siganl_val.strength_val = value;

		// strength value
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "%d%%", value);
		GUI_SetProperty(text_strength, "string", buffer);

		// progbar quality
		app_ioctl(s_dvbs_sat_data.cur_sat.sat_data.tuner, FRONTEND_QUALITY_GET, &value);
		if(value > 99)
			value = 100;
		siganl_bar.quality_bar = progbar_quality;
		siganl_val.quality_val = value;

		// quality value
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "%d%%", value);
		GUI_SetProperty(text_quality, "string", buffer);

		app_signal_progbar_update(s_dvbs_sat_data.cur_sat.sat_data.tuner, &siganl_bar, &siganl_val);
		app_panel_ioctl_handle(PANEL_SHOW_QUALITY, &value);
	}
	else
	{
		if(value_flag == 1)
		{
			value_flag = 0;
			app_clean_signal(progbar_strength, text_strength, progbar_quality, text_quality);
		}
	}

    return 0;
}

static uint32_t _dvbs_sat_num_get(uint32_t tuner)
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

		sat_count++;
	}

	return sat_count;
}

static uint32_t _dvbs_cur_sat_tp_num_get(void)
{
	GxMsgProperty_NodeNumGet node_num = {0};

	node_num.node_type = NODE_TP;
	node_num.sat_id = s_dvbs_sat_data.cur_sat.sat_data.id;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);

	return node_num.node_num;
}

static void _dvbs_cur_sat_get(void)
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
	return;
}

static void _dvbs_cur_tp_get(void)
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

	GxMsgProperty_NodeByPosGet node_tp = {0};
	node_tp.node_type = NODE_TP;
	node_tp.pos = s_dvbs_sat_data.cur_tp_sel;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp);
	memcpy(&s_dvbs_sat_data.cur_tp, &node_tp, sizeof(GxMsgProperty_NodeByPosGet));

	return;
}

static void _dvbs_tuner_rebuild(char *cmb_name)
{
    char buf[20] = {0};
    bool tuner1_sat_exist = false;
    bool tuner2_sat_exist = false;

    if(NULL == cmb_name)
        return;

    GUI_SetProperty(cmb_name, "state", "show");
    strcat(buf, "[");
    if(s_dvbs_sat_data.sat_num_get(TUNER1) > 0)
    {
        tuner1_sat_exist = true;
        s_dvbs_sat_data.which_tuner_sat_exist = TUNER1;
        strcat(buf, "Tuner1,");
    }
    if(s_dvbs_sat_data.sat_num_get(TUNER2) > 0)
    {
        tuner2_sat_exist = true;
        s_dvbs_sat_data.which_tuner_sat_exist = TUNER2;
        strcat(buf, "Tuner2,");
    }
    strcat(buf, "]");
    GUI_SetProperty(cmb_name, "content", buf);

    if(tuner1_sat_exist && tuner2_sat_exist)
        s_dvbs_sat_data.which_tuner_sat_exist = TUNER_ALL;
    else if(tuner1_sat_exist && false == tuner2_sat_exist)
        s_dvbs_sat_data.cur_tuner = TUNER1;
    else if(tuner2_sat_exist && false == tuner1_sat_exist)
        s_dvbs_sat_data.cur_tuner = TUNER2;

    return;
}

static void _dvbs_sat_data_rebuild(char *cmb_name, bool tuner_show)
{
	int i = 0;
	int sat_count = 0;
	int cur_sel_reset = 0;
    uint32_t cur_tuner = 0;
    char *buffer_all = NULL;
    char buffer[BUFFER_LENGTH_SAT] = {0};
	GxMsgProperty_NodeNumGet sat_num = {0};
	GxMsgProperty_NodeByPosGet node_sat = {0};

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
	return;
}

static void _dvbs_tp_data_rebuild(char *cmb_name)
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
    if(0 == s_dvbs_sat_data.cur_sat_tp_total)
    {
        s_dvbs_sat_data.cur_tp_sel = 0;
        if(cmb_name != NULL)
            GUI_SetProperty(cmb_name, "content", "[NONE]");
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
#if (DEMOD_DVB_T > 0)
            if(GXBUS_PM_SAT_DVBT2 == s_dvbs_sat_data.cur_sat.sat_data.type)
            {
                char* bandwidth_str[BANDWIDTH_AUTO] = {"8M", "7M","6M"};

                if(node_tp.tp_data.tp_dvbt2.bandwidth < BANDWIDTH_AUTO)
                {
                    sprintf(buffer,"(%d/%d) %d.%d /%s,",
                            i + 1,
                            s_dvbs_sat_data.cur_sat_tp_total,
                            node_tp.tp_data.frequency / 1000,
                            (node_tp.tp_data.frequency / 100) % 10,
                            bandwidth_str[node_tp.tp_data.tp_dvbt2.bandwidth]);
                }
            }else
#endif
#if ( DEMOD_DVB_C > 0)
                if(GXBUS_PM_SAT_C == s_dvbs_sat_data.cur_sat.sat_data.type)
                {
                    char* modulation_str[5] = {"16QAM", "32QAM","64QAM","128QAM","256QAM"};

                    if((node_tp.tp_data.tp_c.modulation >= DVBC_16QAM) && (node_tp.tp_data.tp_c.modulation <= DVBC_256QAM))
                    {
                        sprintf(buffer,"(%d/%d) %d /%s /%d,",
                                        i + 1,
                                        s_dvbs_sat_data.cur_sat_tp_total,
                                        node_tp.tp_data.frequency / 1000,
                                        modulation_str[node_tp.tp_data.tp_c.modulation-DVBC_16QAM],
                                        node_tp.tp_data.tp_dtmb.symbol_rate);
                    }
                }else
#endif
#if (DEMOD_DTMB > 0)
                    if(GXBUS_PM_SAT_DTMB == s_dvbs_sat_data.cur_sat.sat_data.type)
                    {
                        char* bandwidth_str[BANDWIDTH_AUTO] = {"8M", "7M","6M"};// no 7M?
                        if(GXBUS_PM_SAT_1501_DTMB == s_dvbs_sat_data.cur_sat.sat_data.sat_dtmb.work_mode)
                        {
                            if(node_tp.tp_data.tp_dtmb.bandwidth < BANDWIDTH_AUTO)
                            {
                                sprintf(buffer,"(%d/%d) %d.%d /%s,",
                                        i + 1,
                                        s_dvbs_sat_data.cur_sat_tp_total,
                                        node_tp.tp_data.frequency / 1000,
                                        (node_tp.tp_data.frequency / 100) % 10,
                                        bandwidth_str[node_tp.tp_data.tp_dtmb.bandwidth]);
                            }
                        }
                        else
                        {
                            char* modulation_str[5] = {"16QAM", "32QAM","64QAM","128QAM","256QAM"};
                            if((node_tp.tp_data.tp_dtmb.modulation >= DVBC_16QAM) && (node_tp.tp_data.tp_dtmb.modulation <= DVBC_256QAM))
                            {
                                sprintf(buffer,"(%d/%d) %d /%s /%d,",
                                        i + 1,
                                        s_dvbs_sat_data.cur_sat_tp_total,
                                        node_tp.tp_data.frequency / 1000,
                                        modulation_str[node_tp.tp_data.tp_dtmb.modulation-DVBC_16QAM],
                                        node_tp.tp_data.tp_dtmb.symbol_rate);
                            }
                        }
                    }else
#endif
                    {
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

static int _dvbs_sat_poplist_get_name(char **p_sat_name)
{
    int i;
    char **sat_name = NULL ;
    GxMsgProperty_NodeByPosGet node;

    if(NULL == p_sat_name)
    {
        app_log_error("\nsat_list_get_name failed, param...%s,%d\n",__FILE__,__LINE__);
        return -1;
    }
    if(0 == s_dvbs_sat_data.sat_total)
    {
        app_log_error("\nsat_list_get_name failed, no sat...%s,%d\n",__FILE__,__LINE__);
        return -1;
    }

    sat_name = (char**)GxCore_Calloc(s_dvbs_sat_data.sat_total, sizeof(char*));
    if(NULL == sat_name)
    {
        app_log_error("\033[31m!!!!!!!!!![%s, %d]:GxCore_Calloc failed!!!!!!!!!!!\033[0m\n", __func__, __LINE__);
        return -1;
    }

    for(i = 0; i < s_dvbs_sat_data.sat_total; i++)
    {
        node.node_type = NODE_SAT;
        node.pos = s_dvbs_sat_data.sat_buf[i]; // map pos.
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

        sat_name[i] = (char*)GxCore_Calloc(1, MAX_SAT_NAME);
        if(NULL == sat_name[i])
        {
            app_log_error("\033[31m!!!!!!!!!![%s, %d]:GxCore_Calloc failed!!!!!!!!!!!\033[0m\n", __func__, __LINE__);
            if(sat_name != NULL)
            {
                for(i = 0; i < s_dvbs_sat_data.sat_total; i++)
                {
                    if(sat_name[i] != NULL)
                        GxCore_Free(sat_name[i]);
                }
                GxCore_Free(sat_name);
            }
            return -1;
        }
        strcpy(sat_name[i] , (char*)node.sat_data.sat_s.sat_name);
    }

    *p_sat_name =  (char*)sat_name;
    return 0;
}

static int _dvbs_tp_poplist_get_info(char **p_tp_info)
{
    int i;
    char **tp_info = NULL ;
    GxMsgProperty_NodeByPosGet node;

    if(NULL == p_tp_info)
    {
        app_log_error("\ntp_list_get_info failed, param...%s,%d\n",__FILE__,__LINE__);
        return -1;
    }
    if(0 == s_dvbs_sat_data.cur_sat_tp_total)
    {
        app_log_error("tp_list_get_info, tp total error...%s,%d\n",__FILE__,__LINE__);
        return -1;
    }

    tp_info = (char**)GxCore_Calloc(s_dvbs_sat_data.cur_sat_tp_total, sizeof(char*));
    if(NULL == tp_info)
    {
        app_log_error("\033[31m!!!!!!!!!![%s, %d]:GxCore_Calloc failed!!!!!!!!!!!\033[0m\n", __func__, __LINE__);
        return -1;
    }

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

    *p_tp_info = (char*)tp_info ;
    return 0;
}

static void _set_diseqc(PositionVal *sat_position)
{
	AppFrontend_DiseqcParameters diseqc10 = {0};
	AppFrontend_DiseqcParameters diseqc11 = {0};

	if(s_dvbs_sat_data.sat_total > 0)
	{
        diseqc10.type = DiSEQC10;
        diseqc10.u.params_10.chDiseqc = s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc10;
        diseqc10.u.params_10.bPolar = app_show_to_lnb_polar(&s_dvbs_sat_data.cur_sat.sat_data);

        if(0 == s_dvbs_sat_data.cur_sat_tp_total)
        {
            /*由于在有TP情况下中心频率为非法值时，需要设SEC_VOLTAGE_ALL达到关闭
                TS输出的目的，但是在没有TP时传该值就不会去设供电，因此默认设为18V*/
            if(diseqc10.u.params_10.bPolar == SEC_VOLTAGE_ALL)
                diseqc10.u.params_10.bPolar = SEC_VOLTAGE_18;
        }
        else
        {
            if(diseqc10.u.params_10.bPolar == SEC_VOLTAGE_ALL)
                diseqc10.u.params_10.bPolar = app_show_to_tp_polar(&s_dvbs_sat_data.cur_tp.tp_data);
        }

        diseqc10.u.params_10.b22k = app_show_to_sat_22k(s_dvbs_sat_data.cur_sat.sat_data.sat_s.switch_22K, &s_dvbs_sat_data.cur_tp.tp_data);
	}

#if UNICABLE_SUPPORT
    if(!app_unicable_param_check(s_dvbs_sat_data.cur_sat.sat_data))
    {
        // for polar
        AppFrontend_PolarState Polar = diseqc10.u.params_10.bPolar ;
        app_ioctl(s_dvbs_sat_data.cur_sat.sat_data.tuner, FRONTEND_POLAR_SET, (void*)&Polar);
    }
#else
    // for polar
    AppFrontend_PolarState Polar = diseqc10.u.params_10.bPolar ;
    app_ioctl(s_dvbs_sat_data.cur_sat.sat_data.tuner, FRONTEND_POLAR_SET, (void*)&Polar);
#endif

	// for 22k
	AppFrontend_22KState _22K = diseqc10.u.params_10.b22k;
	app_ioctl(s_dvbs_sat_data.cur_sat.sat_data.tuner, FRONTEND_22K_SET, (void*)&_22K);

	app_set_diseqc_10(s_dvbs_sat_data.cur_sat.sat_data.tuner,&diseqc10, true);

	diseqc11.type = DiSEQC11;
	diseqc11.u.params_11.chUncom = s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc11;
	diseqc11.u.params_11.chCom = diseqc10.u.params_10.chDiseqc ;
	diseqc11.u.params_11.bPolar = diseqc10.u.params_10.bPolar;
	diseqc11.u.params_11.b22k = diseqc10.u.params_10.b22k;
	app_set_diseqc_11(s_dvbs_sat_data.cur_sat.sat_data.tuner, &diseqc11, true);

    int32_t motor_state = app_get_motor_state_in_fullscreen();
    MotorState tip = DISH_NONE;

	if((s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
	{
		tip = app_diseqc12_goto_position(s_dvbs_sat_data.cur_sat.sat_data.id, s_dvbs_sat_data.cur_sat.sat_data.tuner,
				s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc12_pos, false, motor_state);
		app_create_position_tip(tip, motor_state);
	}
	else if((s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS)
	{
		LongitudeInfo local_longitude = {0};
		local_longitude.longitude = s_dvbs_sat_data.cur_sat.sat_data.sat_s.longitude;
		local_longitude.longitude_direct = s_dvbs_sat_data.cur_sat.sat_data.sat_s.longitude_direct;
		tip = app_usals_goto_position(s_dvbs_sat_data.cur_sat.sat_data.id, s_dvbs_sat_data.cur_sat.sat_data.tuner,
				&local_longitude, sat_position, false, motor_state);
		app_create_position_tip(tip, motor_state);
	}
	app_diseqc_sat_id_bak_set(s_dvbs_sat_data.cur_sat.sat_data.id);
	// POST SEM
    GxFrontendOccur(s_dvbs_sat_data.cur_sat.sat_data.tuner);
}

static void _set_tp(SetTpMode set_mode)
{
    AppFrontend_SetTp params = {0};

	if(s_dvbs_sat_data.cur_sat_tp_total == 0)
	{
		params.fre = 0;
		params.symb = 0;
		params.polar = SEC_VOLTAGE_OFF;
	}
	else
	{
		params.fre = app_show_to_tp_freq(&s_dvbs_sat_data.cur_sat.sat_data, &s_dvbs_sat_data.cur_tp.tp_data);
		params.symb = s_dvbs_sat_data.cur_tp.tp_data.tp_s.symbol_rate;
		params.pls_n = s_dvbs_sat_data.cur_tp.tp_data.tp_s.pls_n;
        params.stream_size = s_dvbs_sat_data.cur_tp.tp_data.tp_s.stream_size;
		params.polar = app_show_to_lnb_polar(&s_dvbs_sat_data.cur_sat.sat_data);

		if(params.polar == SEC_VOLTAGE_ALL)
			params.polar = app_show_to_tp_polar(&s_dvbs_sat_data.cur_tp.tp_data);
	}
	params.sat22k = app_show_to_sat_22k(s_dvbs_sat_data.cur_sat.sat_data.sat_s.switch_22K, &s_dvbs_sat_data.cur_tp.tp_data);
	params.tuner = s_dvbs_sat_data.cur_sat.sat_data.tuner;

#if UNICABLE_SUPPORT
    uint32_t centre_frequency = 0;//实际中频
    uint32_t lnb_index = 0;
    uint32_t set_tp_req = 0; //用户下行频率
    if(app_unicable_param_check(s_dvbs_sat_data.cur_sat.sat_data))
    {
        lnb_index = app_calculate_set_freq_and_initfreq(&s_dvbs_sat_data.cur_sat.sat_data, &s_dvbs_sat_data.cur_tp.tp_data, &set_tp_req, &centre_frequency);
        params.fre = set_tp_req;
        params.lnb_index = lnb_index;
    }
    else
        params.lnb_index = 0x20;

    params.centre_fre = centre_frequency;
    params.if_channel_index = s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.if_channel;
    params.if_fre = s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.centre_fre[params.if_channel_index];
#else
    params.lnb_index = 0x20;
    params.centre_fre = 0;
    params.if_channel_index = 0;
    params.if_fre = 0;
#endif

#if MULTISTREAM_SUPPORT
    if(g_AppPlayOps.normal_play.play_total > 0)
    {
        GxMsgProperty_NodeByPosGet prog_node = {0};
        prog_node.node_type = NODE_PROG;
        prog_node.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_node));
        if(s_dvbs_sat_data.cur_tp.tp_data.sat_id == prog_node.prog_data.sat_id
                && s_dvbs_sat_data.cur_tp.tp_data.id == prog_node.prog_data.tp_id
                && prog_node.prog_data.muti_ts_exist == 1)
        {
            params.pls_ts_id = prog_node.prog_data.muti_ts_id;
            params.pls_code = prog_node.prog_data.muti_ts_code;
        }
    }
#endif

    switch(s_dvbs_sat_data.cur_sat.sat_data.type)
    {
        case GXBUS_PM_SAT_S:
        {
            params.type = FRONTEND_DVB_S;
            break;
        }
        case GXBUS_PM_SAT_C:
        {
#if DEMOD_DVB_C
            params.fre = s_dvbs_sat_data.cur_tp.tp_data.frequency;
            params.symb = s_dvbs_sat_data.cur_tp.tp_data.tp_c.symbol_rate;
            params.qam = s_dvbs_sat_data.cur_tp.tp_data.tp_c.modulation;
#endif
            params.type = FRONTEND_DVB_C;
            break;

        }
        case GXBUS_PM_SAT_T:
        {
            params.type = FRONTEND_DVB_T;
            break;
        }
        case GXBUS_PM_SAT_DVBT2:
        {
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
            params.fre = s_dvbs_sat_data.cur_tp.tp_data.frequency;
            params.bandwidth = s_dvbs_sat_data.cur_tp.tp_data.tp_dvbt2.bandwidth;
            params.data_plp_id = 0xff;
            params.common_plp_exist = 0xff;
            params.common_plp_id = 0xff;
            params.type_1501 = DVBT_AUTO_MODE;//PLP Data not in tp note,should be auto
#endif
            params.type = FRONTEND_DVB_T2;
            break;
        }
#if DEMOD_DTMB
		case GXBUS_PM_SAT_DTMB:
			if(GXBUS_PM_SAT_1501_DTMB == s_dvbs_sat_data.cur_sat.sat_data.sat_dtmb.work_mode)
			{
				params.fre = s_dvbs_sat_data.cur_tp.tp_data.frequency;
				params.bandwidth = s_dvbs_sat_data.cur_tp.tp_data.tp_dtmb.bandwidth;// bandwidth: 0-8M, 2-6M
				params.type_1501 = DTMB;
			}
			else
			{// for dvbc mode
				params.fre = s_dvbs_sat_data.cur_tp.tp_data.frequency;
				params.symb = s_dvbs_sat_data.cur_tp.tp_data.tp_dtmb.symbol_rate;
				params.qam = s_dvbs_sat_data.cur_tp.tp_data.tp_dtmb.modulation;
				params.type_1501 = DTMB_C;
			}
			params.type = FRONTEND_DTMB;
			break;
#endif
        default:
            params.type = FRONTEND_DVB_S;
    }

	app_tuner_cur_tuner_set(s_dvbs_sat_data.cur_sat.sat_data.tuner);
	app_set_tp(s_dvbs_sat_data.cur_sat.sat_data.tuner, &params, set_mode);
	if(ex_sat_search_mode == 1)
	{
		ex_sat_search_mode = 0;
		if(s_dvbs_sat_data.cur_sat_tp_total == 0)
			GxFrontendClearErrFreFlag();
	}

	return;
}

dvbs_sat_data_t s_dvbs_sat_data = {
    .cur_tuner = TUNER1,
    .which_tuner_sat_exist = TUNER1,
	.sat_total = 0,
    .sat_id_bak = 0,
	.cur_sat_sel = {0},//sel in gui, not in dm
	.cur_sat_tp_total = 0,
	.cur_tp_sel = 0,
	.cur_sat = {0},
	.cur_tp = {0},
    .change = (SAT_NONE_CHANGE | TP_NONE_CHANGE),
	.sat_gui_sel_get_by_pos = _dvbs_sat_gui_sel_get_by_pos,
    .sat_gui_sel_get_by_id = _dvbs_sat_gui_sel_get_by_id,
    .sat_gui_sel_update = _dvbs_cur_tuner_sat_gui_sel_update,
    .tuner_rebuild = _dvbs_tuner_rebuild,
	.sat_rebuild = _dvbs_sat_data_rebuild,
	.tp_rebuild = _dvbs_tp_data_rebuild,
	.sat_poplist_get_name = _dvbs_sat_poplist_get_name,
	.tp_poplist_get_info  = _dvbs_tp_poplist_get_info,
	.sat_num_get = _dvbs_sat_num_get,
	.cur_sat_tp_num_get = _dvbs_cur_sat_tp_num_get,
	.cur_sat_get = _dvbs_cur_sat_get,
	.cur_tp_get = _dvbs_cur_tp_get,
	.set_diseqc = _set_diseqc,
	.set_tp = _set_tp
};
#endif
