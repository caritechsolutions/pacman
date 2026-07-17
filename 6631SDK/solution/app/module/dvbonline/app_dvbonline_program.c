#include "app.h"
#include "app_windows.h"

#if DVB_CLOUD_SUPPORT
#include <gx_dvbonline.h>
#include "app_wnd_search.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "module/app_frontend.h"
#include "app_pop.h"
#include "app_default_params.h"
#include "full_screen.h"
#include "app_book.h"
#include "app_factory_test_date.h"
#include "app_keyboard_language.h"

void app_dvbonline_program_play(GxBusPmDataProg *prog)
{
	gx_dvbonline_sattype_t type = DVBONLINE_SAT_DVBS;
	int sat_position = 0;
	int frequency = 0;
	int polarization = 0;
	int symbol_rate = 0;
	int service_id = 0;
	char service_name[MAX_PROG_NAME+1];
	char prog_name[MAX_PROG_NAME+1];
	GxMsgProperty_NodeByIdGet sat_node = {0};
	GxMsgProperty_NodeByIdGet tp_node = {0};

	sat_node.node_type = NODE_SAT;
	sat_node.id = prog->sat_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_node);
	if(sat_node.sat_data.type == GXBUS_PM_SAT_S)
	{
		type = DVBONLINE_SAT_DVBS;
		sat_position = sat_node.sat_data.sat_s.longitude;
		if(sat_node.sat_data.sat_s.longitude_direct == GXBUS_PM_SAT_LONGITUDE_DIRECT_WEST)
		{
			sat_position *= -1;
		}
	}
	else if((sat_node.sat_data.type == GXBUS_PM_SAT_T)||(sat_node.sat_data.type == GXBUS_PM_SAT_DVBT2))
	{
		type = DVBONLINE_SAT_DVBT;
	}
	else if(sat_node.sat_data.type == GXBUS_PM_SAT_C)
	{
		type = DVBONLINE_SAT_DVBC;
	}
	else
	{
		gx_dvbonline_program_stop();
		return;
	}

	tp_node.node_type = NODE_TP;
	tp_node.id = prog->tp_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tp_node);
	frequency = tp_node.tp_data.frequency;
	if(sat_node.sat_data.type == GXBUS_PM_SAT_S)
	{
		polarization = tp_node.tp_data.tp_s.polar;
		symbol_rate = tp_node.tp_data.tp_s.symbol_rate;
	}
	else if(sat_node.sat_data.type == GXBUS_PM_SAT_T)
	{
		polarization = tp_node.tp_data.tp_t.bandwidth;
	}
	else if(sat_node.sat_data.type == GXBUS_PM_SAT_DVBT2)
	{
		polarization = tp_node.tp_data.tp_dvbt2.bandwidth;
	}
	else if(sat_node.sat_data.type == GXBUS_PM_SAT_C)
	{
		polarization = tp_node.tp_data.tp_c.modulation;
		symbol_rate = tp_node.tp_data.tp_c.symbol_rate;
	}

	service_id = (int)prog->service_id;
	memset(service_name, 0, sizeof(service_name));
	memcpy(service_name, prog->u2.service_provider_name, MAX_PROG_NAME);
	memset(prog_name, 0, sizeof(prog_name));
	memcpy(prog_name, prog->prog_name, MAX_PROG_NAME);

	gx_dvbonline_program_play(type, sat_position, frequency, polarization, symbol_rate, service_id, service_name, prog_name);

}

void app_dvbonline_program_stop(void)
{
	gx_dvbonline_program_stop();
}


#endif

