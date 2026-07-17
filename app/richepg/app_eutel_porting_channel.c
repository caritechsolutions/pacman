
#include "app_config.h"

#if RICHEPG_SUPPORT
#include "app.h"
#include "app_windows.h"
#include "full_screen.h"
#include "channel_common.h"
#include "richepg_json.h"
#include "richepg_json_api.h"
#include "app_eutel_database.h"
#include "richepg_file_splice.h"
#include "app_eutel_common.h"
#include "richepg.h"
#include "app_fuse_ops.h"


#ifdef APP_EUTEL_CHANNEL_EASY_DATA_SUPPORT

typedef struct {
    int lcn;
    int pos;
} ExEutelChannelLcnArg;

struct eutul_channel_easy_data
{
    uint32_t  sat_id ;
    uint32_t  size   ;
    //EutelChannelLcnArg   *p_arg ;
	ExEutelChannelLcnArg   *p_arg ;
};

static struct eutul_channel_easy_data g_channel_easy_data[EUTUL_SAT_MAX_NUM];

int app_eutel_channel_easy_data_save(EutelChannelArg *ary,EutelChannelLcnArg *use_array,unsigned int size)
{
	int i,j ;
	int pos = 0 ;
	int prog_id = 0 ;
	int lcn = 0 ;
	int sat_id ;

#ifdef APP_EUTEL_CHANNEL_EASY_SAVE_ONLY_EDIT
	if (GUI_CheckDialog(WND_CHANNEL_EDIT) != GXCORE_SUCCESS)
	{
		return -1 ;
	}
#endif

	if (ary == NULL || use_array == NULL || size <= 0)
		return -1 ;

	sat_id = app_eutel_cur_sat_id_get();
	for(i=0;i<EUTUL_SAT_MAX_NUM;i++)
	{
		if ( g_channel_easy_data[i].sat_id == sat_id )
		{
			break;
		}
	}
	if ( i == EUTUL_SAT_MAX_NUM )
	{
		EUTEL_ERR("#### easy data save, can not find sat id = %d \n",sat_id);
		return -1 ;
	}

	if (g_channel_easy_data[i].p_arg != NULL )
	{
		if ( g_channel_easy_data[i].size == size )
		{
			for(j=0;j<size;j++)
			{
				pos = use_array[j].pos ;
				lcn = use_array[j].lcn ;

				if ( (g_channel_easy_data[i].p_arg[j].lcn != lcn ) ||
					 (g_channel_easy_data[i].p_arg[j].pos != ary[pos].prog_id ))
				{
					break;
				}
			}
			if (j==size)
			{
				return 1;
			}
		}
		APP_FREE(g_channel_easy_data[i].p_arg);
		g_channel_easy_data[i].size = 0 ;
	}

	g_channel_easy_data[i].p_arg = GxCore_Calloc(size, sizeof(EutelChannelLcnArg));
	if (g_channel_easy_data[i].p_arg == NULL) {
		RICHEPG_ERR("easy malloc arg err, total = %d!\n", size);
		return -1;
	}
	g_channel_easy_data[i].size = size ;
	for(j=0;j<size;j++)
	{
		pos = use_array[j].pos ;
		lcn = use_array[j].lcn ;
		prog_id = ary[pos].prog_id ;

		g_channel_easy_data[i].p_arg[j].lcn = lcn ;
		g_channel_easy_data[i].p_arg[j].pos = prog_id ;
	}
	EUTEL_SINFO("easy data save \n");
	return 0;
}

int app_eutel_channel_easy_data_uninit(void)
{
	int i = 0;

	for (i = 0; i < EUTUL_SAT_MAX_NUM; i++)
	{
		APP_FREE(g_channel_easy_data[i].p_arg);
		g_channel_easy_data[i].size = 0 ;
	}

	return 0 ;
}

static int _eutel_channel_easy_data_init(void)
{
	int i = 0;
	RichepgTsMgr *mgr = richepg_ts_mgr_get();
	if ( mgr == NULL ||  mgr->sat_array == NULL )
		return -1 ;

	for (i = 0; i < EUTUL_SAT_MAX_NUM; i++)
	{
		g_channel_easy_data[i].sat_id = 0 ;
		g_channel_easy_data[i].p_arg = NULL ;
		g_channel_easy_data[i].size = 0 ;
	}

	for (i = 0; i < mgr->sat_num; i++)
	{
		if (i>=EUTUL_SAT_MAX_NUM)
			break;
		g_channel_easy_data[i].sat_id = mgr->sat_array[i].sat_id ;
	}

	return 0 ;
}

static int  _eutel_get_easy_data_index(void)
{
	int i = 0;
	int sat_id ;
	RichepgTsMgr *mgr = richepg_ts_mgr_get();
	if ( mgr == NULL ||  mgr->sat_array == NULL )
		return -1 ;

	sat_id = g_AppPlayOps.normal_play.view_info.sat_id ;

	for (i = 0; i < EUTUL_SAT_MAX_NUM; i++)
	{
		if ( (sat_id == g_channel_easy_data[i].sat_id) && (sat_id >0))
		{
			if ( g_channel_easy_data[i].p_arg != NULL)
				return i ;
			else
				return -1 ;
		}
	}
	return -1;
}

int  app_eutel_channel_easy_data_is_ready(int sat_id)
{
	int index = 0 ;
	index = _eutel_get_easy_data_index();
	if ( index < 0 )
		return 0 ;
	if ( app_fuse_common_channel_list_check_dialog() == GXCORE_SUCCESS )
		return 0 ;
	return 1 ;
}

static int  _eutel_easy_get_list_total(void)
{
	int index = 0 ;
	index = _eutel_get_easy_data_index();
	if (index < 0 )
		return -1 ;
	return g_channel_easy_data[index].size;
}

static unsigned int  _eutel_easy_real_pos(unsigned int sel)
{
	int id = 0 ;
	int pos = 0 ;

	int index = 0 ;
	index = _eutel_get_easy_data_index();
	if (index < 0 )
		return -1 ;

	if (sel >= g_channel_easy_data[index].size)
	{
		EUTEL_ERR("_eutel_easy_real_pos default \n");
		return 0 ;
	}

	id = g_channel_easy_data[index].p_arg[sel].pos;

	app_get_prog_pos_by_id_cur(id,&pos);

	extern void _eutel_channel_prog_set_pos_lcn(int pos, int lcn);
	_eutel_channel_prog_set_pos_lcn(pos,g_channel_easy_data[index].p_arg[sel].lcn);

	return pos ;
}

static unsigned int  _eutel_easy_real_select(unsigned int pos)
{
	int i;
	int sel = -1 ;
	int lcn = 0 ;
	int size = 0 ;
	GxMsgProperty_NodeByPosGet node = {0};

	int index = 0 ;
	index = _eutel_get_easy_data_index();
	if (index < 0 )
		return -1 ;

	size = g_channel_easy_data[index].size;

	extern int eutel_channel_prog_public_get_play_lcn(void);
	lcn = eutel_channel_prog_public_get_play_lcn() ;

	node.node_type = NODE_PROG;
	node.pos = pos;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

	for(i=0;i<size;i++)
	{
		if ( g_channel_easy_data[index].p_arg[i].pos == node.prog_data.id)
		{
			if ( g_channel_easy_data[index].p_arg[i].lcn == lcn )
			{
				sel = i ;
				break;
			}
			else
			{
				if ( sel < 0 )
					sel = i ;
			}
		}
	}

	if (sel < 0)
	{
		EUTEL_ERR("_eutel_easy_real_select default \n");
		sel = 0 ;
	}

	return sel;
}

static unsigned int  _eutel_easy_get_id(unsigned int sel)
{
	int id = 0 ;

	int index = 0 ;
	index = _eutel_get_easy_data_index();
	if (index < 0 )
		return -1 ;

	if (sel >= g_channel_easy_data[index].size)
	{
		EUTEL_ERR("_eutel_easy_get_id default \n");
		return 0 ;
	}

	id = g_channel_easy_data[index].p_arg[sel].pos;

	return id ;
}

static unsigned int  _eutel_easy_get_lcn(unsigned int sel)
{
	int lcn = 0 ;

	int index = 0 ;
	index = _eutel_get_easy_data_index();
	if (index < 0 )
		return sel ;

	if (sel >= g_channel_easy_data[index].size)
	{
		EUTEL_ERR("_eutel_easy_get_lcn default \n");
		return 0 ;
	}

	lcn = g_channel_easy_data[index].p_arg[sel].lcn;

	return lcn ;
}
#endif

// full data

static int  _eutel_get_list_total(void)
{
	int group = 0 ;
	int sat_id = 0 ;
	group = g_AppPlayOps.normal_play.view_info.group_mode ;
	sat_id = g_AppPlayOps.normal_play.view_info.sat_id ;

	if ((group == GROUP_MODE_SAT ) && (app_eutel_sat_type_check(sat_id)))
	{
		if ( eutel_channel_prog_public_is_full_mode(sat_id) )
		{
			return eutel_channel_prog_public_get_total();
		}
#ifdef APP_EUTEL_CHANNEL_EASY_DATA_SUPPORT
		else if ( app_eutel_channel_easy_data_is_ready(sat_id) )
		{
			return _eutel_easy_get_list_total();
		}
#endif
	}
	return -1 ;
}

static int  _eutel_get_next_pos(unsigned int pos)
{
	int group = 0 ;
	int sat_id = 0 ;
	group = g_AppPlayOps.normal_play.view_info.group_mode ;
	sat_id = g_AppPlayOps.normal_play.view_info.sat_id ;

	if ((group == GROUP_MODE_SAT ) && (app_eutel_sat_type_check(sat_id)))
	{
		if ( eutel_channel_prog_public_is_full_mode(sat_id) )
		{
			return eutel_channel_prog_public_get_next_pos(pos);
		}
	}

	return -1 ;
}

static int  _eutel_get_prev_pos(unsigned int pos)
{
	int group = 0 ;
	int sat_id = 0 ;
	group = g_AppPlayOps.normal_play.view_info.group_mode ;
	sat_id = g_AppPlayOps.normal_play.view_info.sat_id ;

	if ((group == GROUP_MODE_SAT ) && (app_eutel_sat_type_check(sat_id)))
	{
		if ( eutel_channel_prog_public_is_full_mode(sat_id) )
		{
			return eutel_channel_prog_public_get_prev_pos(pos);
		}
	}

	return -1 ;
}

static int  _eutel_get_cur_pos(unsigned int pos)
{
	int group = 0 ;
	int sat_id = 0 ;
	group = g_AppPlayOps.normal_play.view_info.group_mode ;
	sat_id = g_AppPlayOps.normal_play.view_info.sat_id ;

	if ((group == GROUP_MODE_SAT ) && (app_eutel_sat_type_check(sat_id)))
	{
		if ( eutel_channel_prog_public_is_full_mode(sat_id) )
		{
			return eutel_channel_prog_public_get_cur_pos(pos);
		}
	}

	return -1 ;
}

static int  _eutel_real_pos(unsigned int sel)
{
	int group = 0 ;
	int sat_id = 0 ;
	group = g_AppPlayOps.normal_play.view_info.group_mode ;
	sat_id = g_AppPlayOps.normal_play.view_info.sat_id ;

	if ((group == GROUP_MODE_SAT ) && (app_eutel_sat_type_check(sat_id)))
	{
		if ( eutel_channel_prog_public_is_full_mode(sat_id) )
		{
			return eutel_channel_prog_public_real_pos(sel);
		}
#ifdef APP_EUTEL_CHANNEL_EASY_DATA_SUPPORT
		else if ( app_eutel_channel_easy_data_is_ready(sat_id) )
		{
			return _eutel_easy_real_pos(sel);
		}
#endif
	}

	return -1 ;
}

static int  _eutel_real_select(unsigned int pos)
{
	int group = 0 ;
	int sat_id = 0 ;
	group = g_AppPlayOps.normal_play.view_info.group_mode ;
	sat_id = g_AppPlayOps.normal_play.view_info.sat_id ;

	if ((group == GROUP_MODE_SAT ) && (app_eutel_sat_type_check(sat_id)))
	{
		if ( eutel_channel_prog_public_is_full_mode(sat_id) )
		{
			return eutel_channel_prog_public_real_select(pos);
		}
#ifdef APP_EUTEL_CHANNEL_EASY_DATA_SUPPORT
		else if ( app_eutel_channel_easy_data_is_ready(sat_id) )
		{
			return _eutel_easy_real_select(pos);
		}
#endif
	}

	return -1 ;
}

static int  _eutel_get_id(unsigned int sel)
{
	int group = 0 ;
	int sat_id = 0 ;
	group = g_AppPlayOps.normal_play.view_info.group_mode ;
	sat_id = g_AppPlayOps.normal_play.view_info.sat_id ;

	if ((group == GROUP_MODE_SAT ) && (app_eutel_sat_type_check(sat_id)))
	{
		if ( eutel_channel_prog_public_is_full_mode(sat_id) )
		{
			return eutel_channel_prog_public_get_id(sel);
		}
#ifdef APP_EUTEL_CHANNEL_EASY_DATA_SUPPORT
		else if ( app_eutel_channel_easy_data_is_ready(sat_id) )
		{
			return _eutel_easy_get_id(sel);
		}
#endif
	}

	return -1 ;
}

static int  _eutel_check(unsigned int sel,unsigned int flag1,unsigned int flag2)
{
	int group = 0 ;
	int sat_id = 0 ;
	group = g_AppPlayOps.normal_play.view_info.group_mode ;
	sat_id = g_AppPlayOps.normal_play.view_info.sat_id ;

	if ((group == GROUP_MODE_SAT ) && (app_eutel_sat_type_check(sat_id)))
	{
		return 0;
	}

	return -1 ;
}

////
int app_eutel_porting_channel_get_lcn(int sel)
{
	int sat_id = 0 ;
	sat_id = g_AppPlayOps.normal_play.view_info.sat_id ;

	if ( eutel_channel_prog_public_is_full_mode(sat_id) )
	{
		return eutel_channel_prog_public_get_lcn(sel);
	}
#ifdef APP_EUTEL_CHANNEL_EASY_DATA_SUPPORT
	else if ( app_eutel_channel_easy_data_is_ready(sat_id) )
	{
		return _eutel_easy_get_lcn(sel);
	}
#endif
	return sel ;
}

int  app_eutel_channel_ops_init(void)
{
	app_channel_ops ops ;

#ifdef APP_EUTEL_CHANNEL_EASY_DATA_SUPPORT
	_eutel_channel_easy_data_init();
#endif

	ops.get_list_total  = _eutel_get_list_total;
	ops.get_next_pos    = _eutel_get_next_pos;
	ops.get_prev_pos    = _eutel_get_prev_pos;
	ops.get_cur_pos     = _eutel_get_cur_pos;
	ops.real_pos        = _eutel_real_pos ;
	ops.real_select     = _eutel_real_select;
	ops.get_id          = _eutel_get_id;
	ops.check           = _eutel_check;

	extern void app_channel_set_ops(app_channel_ops *ops);
	app_channel_set_ops(&ops);

	return 0 ;
}

#endif
