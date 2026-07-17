#include "app.h"
#if RICHEPG_SUPPORT
#include "app_windows.h"
#include "app_channel_info.h"
#include "app_eutel_control.h"
#include "app_eutel_common.h"
#include "app_eutel_database.h"
#include "richepg_block_mem.h"
#include "richepg_file_splice.h"
#include "richepg_lookuptables.h"
#include "richepg_store.h"
#include "richepg.h"
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif

/*
 * RANDOMLY_ASSOCIATE_SUPPORT 宏目前只用于测试PVR相关功能
 * 开启宏时会将FTA EPG的节目信息和数据库节目随机关联，此时FTA EPG的节目信息和数据库节目不是对应的
 * 使用方法:
 * 1. 先在Free Scan模式下搜索节目，然后切换到FTA EPG模式安装一颗卫星
 * 2. 切换回Free Scan模式, 并将节目组切换到刚才搜索的卫星
 * 3. 开启宏RANDOMLY_ASSOCIATE_SUPPORT，使用gdb下载或USB的app升级新的软件
 * 4. 切换到FTA EPG模式，此时显示的是FTA EPG的节目信息，播放的Free Scan的节目
 */
#define RANDOMLY_ASSOCIATE_SUPPORT  0

int s_eutel_printf_en = 0;
EutelChannelCtrl s_eutel_ch_ctrl;

static bool s_eutel_logo_display_mode = false;

bool app_eutel_logo_display_mode_get(void)
{
    return s_eutel_logo_display_mode;
}

int eutel_windows_link_init(void)
{
	GUI_LinkDialog(WND_EUTEL_CHINFO, "richepg/wnd_eutel_chinfo.xml");
	GUI_LinkDialog(WND_EUTEL_CHMORE, "richepg/wnd_eutel_chmore.xml");
	GUI_LinkDialog(WND_EUTEL_CHLIST, "richepg/wnd_eutel_chlist.xml");
	GUI_LinkDialog(WND_EUTEL_EPG, "richepg/wnd_eutel_epg.xml");
	GUI_LinkDialog(WND_EUTEL_EPG2, "richepg/wnd_eutel_show.xml");

	GUI_LinkDialog(WND_EUTEL_ANTENNA, "richepg/wnd_eutel_antenna.xml");
	GUI_LinkDialog(WND_EUTEL_MAINTENANCE, "richepg/wnd_eutel_maintenance.xml");
	GUI_LinkDialog(WND_EUTEL_LINEUP, "richepg/wnd_eutel_lineup.xml");
	GUI_LinkDialog(WND_EUTEL_COLLECT, "richepg/wnd_eutel_collect.xml");

	GUI_LinkDialog(WND_EUTEL_SAT_LIST, "richepg/wnd_eutel_sat_list.xml");

	GUI_LinkDialog(WND_EUTEL_INFORMATION, "richepg/wnd_eutel_information.xml");
	GUI_LinkDialog(WND_EUTEL_SEARCH, "richepg/wnd_eutel_search.xml");

	return 0;
}

int eutel_channel_ctrl_init(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;

	ctrl->struct_mgr.attr.mem_id = 1;
	ctrl->struct_mgr.attr.def_size = 1024 * 128;
	ctrl->struct_mgr.attr.align_byte = 4;
	ctrl->struct_mgr.attr.fast_alloc = 0;
    ctrl->struct_mgr.attr.max_size = 0;
	block_mem_mgr_init(&ctrl->struct_mgr);

	ctrl->string_mgr.attr.mem_id = 2;
	ctrl->string_mgr.attr.def_size = 1024 * 128;
	ctrl->string_mgr.attr.align_byte = 1;
	ctrl->string_mgr.attr.fast_alloc = 0;
    ctrl->string_mgr.attr.max_size = 0;
	block_mem_mgr_init(&ctrl->string_mgr);

	ctrl->ear_mgr.attr.mem_id = 3;
	ctrl->ear_mgr.attr.def_size = 1024 * 16;
	ctrl->ear_mgr.attr.align_byte = 1;
	ctrl->ear_mgr.attr.fast_alloc = 0;
    ctrl->ear_mgr.attr.max_size = 0;
	block_mem_mgr_init(&ctrl->ear_mgr);

	ctrl->ops_pos = -1 ;
	ctrl->ops_lcn = 0  ;

	return 0;
}

int eutel_channel_ctrl_release(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;

	app_richepg_chfilter_keep_sec_clear(false);
	block_mem_mgr_free_data(&ctrl->struct_mgr);
	block_mem_mgr_free_data(&ctrl->string_mgr);
	block_mem_mgr_free_data(&ctrl->ear_mgr);

	ctrl->genre_total = 0;
	ctrl->genre_array = NULL;
	ctrl->type_total = 0;
	ctrl->type_array = NULL;
	ctrl->content_total = 0;
	ctrl->content_array = NULL;

	ctrl->arg_total = 0;
	ctrl->arg_array = NULL;
	ctrl->sort_total = 0;
	ctrl->sort_array = NULL;
	ctrl->use_total = 0;
	ctrl->use_array = NULL;

	return 0;
}

static bool _eutel_check_valid_genre(int id)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int i, j;

	for (i = 0; i < ctrl->arg_total; i++)
	{
		for (j = 0; j < ctrl->arg_array[i].genre_num; j++)
		{
			if (ctrl->arg_array[i].genre_ids[j] == id)
				return true;
		}
	}

	return false;
}

static int _eutel_channel_ctrl_channel_genre_build(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int i = 0;
	int total = 0;
	int cnt = 0;
	char *name = NULL;
	EutelChannelGenreArg *genre_array = NULL;

	// add channel genre
	ctrl->genre_total = 0;
	total = richepg_lookuptables_get_total(RICHEPG_LKTT_CHANNEL);
	if (total > 0)
	{
		if ((genre_array = GxCore_Calloc(total, sizeof(EutelChannelGenreArg))) == NULL)
		{
			EUTEL_ERR("malloc genre_array err, total = %d!\n", total);
			goto err;
		}

		for (i = 0; i < total; i++)
		{
			genre_array[cnt].id = richepg_lookuptables_get_id_by_pos(RICHEPG_LKTT_CHANNEL, i);
			if (!_eutel_check_valid_genre(genre_array[cnt].id))
				continue;
			name = richepg_lookuptables_get_name_by_pos(RICHEPG_LKTT_CHANNEL, i);
			if (name && strlen(name) > 0)
			{
				if ((genre_array[cnt].name = block_mem_mgr_dup_str(name, &ctrl->string_mgr)) == NULL)
				{
					EUTEL_ERR("Add data err!\n");
					goto err;
				}
				++cnt;
			}
		}

		if (cnt > 0 && (ctrl->genre_array = block_mem_mgr_dup_data(
						genre_array, cnt * sizeof(EutelChannelGenreArg), &ctrl->struct_mgr)) == NULL)
		{
			EUTEL_ERR("Create genre_array err, total = %d!\n", total);
			goto err;
		}
		ctrl->genre_total = cnt;
		APP_FREE(genre_array);
	}

	return 0;
err:
	APP_FREE(genre_array);
	return -1;
}

static int _eutel_channel_ctrl_channel_type_build(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int i = 0;
	char *name = NULL;

	// add channel type
	ctrl->type_total = richepg_lookuptables_get_total(RICHEPG_LKTT_SERVICE);
	if (ctrl->type_total > 0)
	{
		if ((ctrl->type_array = block_mem_mgr_alloc_data(
				ctrl->type_total * sizeof(EutelChannelTypeArg), &ctrl->struct_mgr)) == NULL)
		{
			EUTEL_ERR("Create type_array err, total = %d!\n", ctrl->type_total);
			goto err;
		}
		for (i = 0; i < ctrl->type_total; i++)
		{
			ctrl->type_array[i].id = richepg_lookuptables_get_id_by_pos(RICHEPG_LKTT_SERVICE, i);
			name = richepg_lookuptables_get_name_by_pos(RICHEPG_LKTT_SERVICE, i);
			if (name && strlen(name) > 0 &&
					(ctrl->type_array[i].name = block_mem_mgr_dup_str(name, &ctrl->string_mgr)) == NULL)
			{
				EUTEL_ERR("Add data err!\n");
				goto err;
			}
		}
	}

	return 0;
err:
	return -1;
}

static int _eutel_channel_ctrl_content_genre_build(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int i = 0;
	char *name = NULL;

	// add event content
	ctrl->content_total = richepg_lookuptables_get_total(RICHEPG_LKTT_CONTENT);
	if (ctrl->content_total > 0)
	{
		if ((ctrl->content_array = block_mem_mgr_alloc_data(
				ctrl->content_total * sizeof(EutelContentGenreArg), &ctrl->struct_mgr)) == NULL)
		{
			EUTEL_ERR("Create content_array err, total = %d!\n", ctrl->content_total);
			goto err;
		}
		for (i = 0; i < ctrl->content_total; i++)
		{
			ctrl->content_array[i].id = richepg_lookuptables_get_id_by_pos(RICHEPG_LKTT_CONTENT, i);
			name = richepg_lookuptables_get_name_by_pos(RICHEPG_LKTT_CONTENT, i);
			if (name && strlen(name) > 0 &&
					(ctrl->content_array[i].name = block_mem_mgr_dup_str(name, &ctrl->string_mgr)) == NULL)
			{
				EUTEL_ERR("Add data err!\n");
				goto err;
			}
		}
	}

	return 0;
err:
	return -1;
}
static int eutel_strlcat(char *dst, char *src, int dst_size)
{
	int len = 0;
	int len1, len2, len3;

	if (src > 0)
	{
		len1 = strlen(dst);
		len2 = dst_size - len1 - 1;
		len3 = strlen(src);
		len = (len2 < len3) ? len2 : len3;
		memcpy(dst+len1, src, len);
	}

	return len;
}

void eutel_strlink(char *dst, char *src, int dst_size)
{
	if (src && strlen(src) > 0)
	{
		if (strlen(dst) != 0)
			eutel_strlcat(dst, " / ", dst_size);
		eutel_strlcat(dst, src, dst_size);
	}
}

char *eutel_genre_name_get(int id, EutelArrayType type)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int i = 0;

	switch(type)
	{
		case EUTEL_CHANNEL_GENRE:
			for (i = 0; i < ctrl->genre_total; i++)
			{
				if (ctrl->genre_array[i].id == id)
					return ctrl->genre_array[i].name;
			}
			break;
		case EUTEL_CHANNEL_TYPE:
			for (i = 0; i < ctrl->type_total; i++)
			{
				if (ctrl->type_array[i].id == id)
					return ctrl->type_array[i].name;
			}
			break;
		case EUTEL_CONTENT_GENRE:
			for (i = 0; i < ctrl->content_total; i++)
			{
				if (ctrl->content_array[i].id == id)
					return ctrl->content_array[i].name;
			}
			break;
		default:
			break;
	}

	return NULL;
}

char *eutel_array_name_get(int num, int *id, EutelArrayType type)
{
#define ARRAY_NAME_SIZE     512
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	static char buffer[ARRAY_NAME_SIZE];
	int i = 0, j = 0;

	memset(buffer, 0, ARRAY_NAME_SIZE);
	if (num > 0)
	{
		switch(type)
		{
			case EUTEL_CHANNEL_GENRE:
				for (i = 0; i < num; i++)
				{
					for (j = 0; j < ctrl->genre_total; j++)
					{
						if (ctrl->genre_array[j].id == id[i])
						{
							eutel_strlink(buffer, ctrl->genre_array[j].name, ARRAY_NAME_SIZE);
							continue;
						}
					}
				}
				break;
			case EUTEL_CHANNEL_TYPE:
				for (i = 0; i < num; i++)
				{
					for (j = 0; j < ctrl->type_total; j++)
					{
						if (ctrl->type_array[j].id == id[i])
						{
							eutel_strlink(buffer, ctrl->type_array[j].name, ARRAY_NAME_SIZE);
							continue;
						}
					}
				}
				break;
			case EUTEL_CONTENT_GENRE:
				for (i = 0; i < num; i++)
				{
					for (j = 0; j < ctrl->content_total; j++)
					{
						if (ctrl->content_array[j].id == id[i])
						{
							eutel_strlink(buffer, ctrl->content_array[j].name, ARRAY_NAME_SIZE);
							continue;
						}
					}
				}
				break;
			default:
				break;
		}
	}
	if (strlen(buffer) == 0)
		eutel_strlink(buffer, STR_ID_BLANK, ARRAY_NAME_SIZE);

	return buffer;
}

static int _eutel_lcn_cmp(const void *a, const void *b)
{
	return (((EutelChannelLcnArg*)a)->lcn - ((EutelChannelLcnArg*)b)->lcn);
}

int eutel_channel_ctrl_build(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	RichepgProgListCtrl *clist = richepg_prog_list_ctrl_get();
	RichepgProgListProg *prog_ex = NULL;
	GxBusPmDataProg prog;
	int i, j;
	int cnt = 0;
	int tmp_size;
	int arg_total = 0;
	int sort_total = 0;;
	int32_t sys_lang_sel = 0;
	char *sys_lang_str = NULL;
	char *prog_name = NULL;

	app_eutel_epg2_bak_timer_stop();
	GxBus_ConfigGetInt(OSD_LANG_KEY, &sys_lang_sel, OSD_LANG);
	sys_lang_str = app_eutel_porting_get_iso6392_code(sys_lang_sel);
	eutel_channel_ctrl_release();

	if (clist->prog_num == 0)
		richepg_proglist_parse();
	if (clist->prog_num == 0)
	{
		EUTEL_ERR("No eutel_channel!\n");
		return -1;
	}
	tmp_size = clist->prog_num * sizeof(EutelChannelArg);
	if ((ctrl->arg_array = block_mem_mgr_alloc_data(tmp_size, &ctrl->struct_mgr)) == NULL)
	{
		EUTEL_ERR("Create arg_array err, total = %d!\n", clist->prog_num);
		goto err2;
	}

#if RANDOMLY_ASSOCIATE_SUPPORT
	int tmp_num = GxBus_PmProgNumGet();
	int tmp_cnt = 0;
	int sat_id  = -1;
	char ay_prog_id[10];
	for(i=0;i<tmp_num;i++)
	{
		if ( i<10 )
		{
			ay_prog_id[i] = i ;
		}
		if (GxBus_PmProgGetByPos(i, 1, &prog) == 1)
		{
			if ( sat_id == -1 )
			{
				sat_id = prog.sat_id ;
			}
			if(prog.sat_id != sat_id)
			{
				break;
			}
			else
			{
				if (prog.scramble_flag == GXBUS_PM_PROG_BOOL_DISABLE)
				{
					ay_prog_id[tmp_cnt] = i ;
					tmp_cnt ++ ;
					if (tmp_cnt >= 10 )
						break;
				}
			}
		}
	}
	tmp_num = tmp_cnt ;
	tmp_cnt = 0 ;
#endif
	for (i = 0; i < clist->prog_num; i++)
	{
		prog_ex = &clist->prog_array[i];
		if (prog_ex->lcns_num == 0)
			continue;

#if RANDOMLY_ASSOCIATE_SUPPORT
		if (++tmp_cnt >= tmp_num) tmp_cnt = 0;
		if (GxBus_PmProgGetByPos(ay_prog_id[tmp_cnt], 1, &prog) == 1)
#else
		if (GxBus_PmProgGetById(prog_ex->prog_id, &prog) == GXCORE_SUCCESS)
#endif
		{
			// id args
			ctrl->arg_array[arg_total].prog_id = prog.id;
			ctrl->arg_array[arg_total].channel_id = prog_ex->id;

			// lcn args
			ctrl->arg_array[arg_total].lcn_num = prog_ex->lcns_num;
			if (prog_ex->lcns_num > 0 &&
					(ctrl->arg_array[arg_total].lcn_ids = block_mem_mgr_dup_data(
						prog_ex->lcns_array, prog_ex->lcns_num * sizeof(int), &ctrl->struct_mgr)) == NULL)
			{
				EUTEL_ERR("Add data err!\n");
				goto err2;
			}
			sort_total += ctrl->arg_array[arg_total].lcn_num;

			// genre args
			ctrl->arg_array[arg_total].genre_num = prog_ex->genres_num;
			if (prog_ex->genres_num > 0 &&
					(ctrl->arg_array[arg_total].genre_ids = block_mem_mgr_dup_data(
						prog_ex->genres_array, prog_ex->genres_num * sizeof(int), &ctrl->struct_mgr)) == NULL)
			{
				EUTEL_ERR("Add data err!\n");
				goto err2;
			}

			// type args
			ctrl->arg_array[arg_total].type_num = prog_ex->types_num;
			if (prog_ex->types_num > 0 &&
					(ctrl->arg_array[arg_total].type_ids = block_mem_mgr_dup_data(
							prog_ex->types_array, prog_ex->types_num * sizeof(int), &ctrl->struct_mgr)) == NULL)
			{
				EUTEL_ERR("Add data err!\n");
				goto err2;
			}

			// channel_name
			prog_name = app_eutel_proglist_prog_name_get(prog_ex->names_num, prog_ex->names_array, sys_lang_str);
			if (prog_name && strlen(prog_name) > 0 &&
					(ctrl->arg_array[arg_total].channel_name = block_mem_mgr_dup_str(
						prog_name, &ctrl->string_mgr)) == NULL)
			{
				EUTEL_ERR("Add data err!\n");
				goto err2;
			}

			// channel_logo
			if (prog_ex->prog_logo && strlen(prog_ex->prog_logo) > 0 &&
					(ctrl->arg_array[arg_total].channel_logo = block_mem_mgr_dup_str(
						prog_ex->prog_logo, &ctrl->string_mgr)) == NULL)
			{
				EUTEL_ERR("Add data err!\n");
				goto err2;
			}

			// ear_logo
			if (prog_ex->ear_logo && strlen(prog_ex->ear_logo) > 0 &&
					(ctrl->arg_array[arg_total].ear_logo = block_mem_mgr_dup_str(
						prog_ex->ear_logo, &ctrl->ear_mgr)) == NULL)
			{
				EUTEL_ERR("Add data err!\n");
				goto err2;
			}

			// flags args
			ctrl->arg_array[arg_total].stream_flag = (prog.service_type != GXBUS_PM_PROG_TV) ? 1 : 0;
			ctrl->arg_array[arg_total].scramble_flag = (prog.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE) ? 1 : 0;
			ctrl->arg_array[arg_total].mark_flag = (prog.favorite_flag & (1 << EUTEL_BOOKMARK_PROG_POS)) ? 1 : 0;
			ctrl->arg_array[arg_total].mark_flag_bak = ctrl->arg_array[arg_total].mark_flag;
			ctrl->arg_array[arg_total].lock_flag = (prog.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE) ? 1 : 0;
			ctrl->arg_array[arg_total].lock_flag_bak = ctrl->arg_array[arg_total].lock_flag;
#if 1
			if (s_eutel_printf_en) {
				EUTEL_SINFO("chid=%5d progid=%5d mark=%d onid=%5d tsid=%5d sid=%5d %s\n",
						ctrl->arg_array[arg_total].channel_id,
						ctrl->arg_array[arg_total].prog_id,
						ctrl->arg_array[arg_total].mark_flag,
						prog.original_id,
						prog.ts_id,
						prog.service_id,
						ctrl->arg_array[arg_total].channel_name);
			}
#endif
			++arg_total;
		}
	}

	if (arg_total == 0)
	{
		EUTEL_ERR("No channel!\n");
		goto err1;
	}
	ctrl->arg_total = arg_total;
	ctrl->sort_total = sort_total;

	if (_eutel_channel_ctrl_channel_genre_build() < 0)
		goto err1;
	if (_eutel_channel_ctrl_channel_type_build() < 0)
		goto err1;
	if (_eutel_channel_ctrl_content_genre_build() < 0)
		goto err1;
	richepg_lookuptables_free();

	// alloc sort_array and use_array, sort by lcn
	tmp_size = 2 * sort_total * sizeof(EutelChannelLcnArg);
	if ((ctrl->sort_array = block_mem_mgr_alloc_data(tmp_size, &ctrl->struct_mgr)) == NULL)
	{
		EUTEL_ERR("Create sort_array err, total = %d!\n", ctrl->arg_total);
		goto err1;
	}
	ctrl->use_array = ctrl->sort_array + sort_total;
	cnt = 0;
	for (i = 0; i < arg_total; i++)
	{
		for (j = 0; j < ctrl->arg_array[i].lcn_num; j++)
		{
			ctrl->sort_array[cnt].pos = i;
			ctrl->sort_array[cnt].lcn = ctrl->arg_array[i].lcn_ids[j];
			cnt++;
		}
	}
	qsort(ctrl->sort_array, sort_total, sizeof(EutelChannelLcnArg), _eutel_lcn_cmp);

	block_mem_mgr_adjust_size(&ctrl->struct_mgr);
	block_mem_mgr_adjust_size(&ctrl->string_mgr);
	block_mem_mgr_adjust_size(&ctrl->ear_mgr);
	eutel_channel_logo_key_refresh();
	richepg_proglist_release();
#if DVB2IP_SERVER_SUPPORT
	app_dvb2ip_prog_num_update();
#endif
	return arg_total;
err1:
	richepg_lookuptables_free();
	eutel_channel_ctrl_release();
err2:
	richepg_proglist_release();
#if DVB2IP_SERVER_SUPPORT
	app_dvb2ip_prog_num_update();
#endif
	return -1;
}

int eutel_channel_ctrl_is_already_build(void)
{
    EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;

    if ((ctrl->genre_array != NULL) && (ctrl->content_array  != NULL) &&
        (ctrl->arg_array != NULL) && (ctrl->use_array != NULL) )
    {
        return 1 ;
    }
    else
    {
        return 0 ;
    }
}

int eutel_channel_prog_public_is_full_mode(int sat_id)
{
	int id ;
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;

	id = app_eutel_cur_sat_id_get();
	if (id == sat_id )
	{
		if (ctrl->arg_array == NULL || ctrl->use_array == NULL)
			return 0 ;
		else
			return 1 ;
	}
	return 0 ;
}

int eutel_channel_prog_public_get_total(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;

	if ( ((g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO) &&
	      (!ctrl->cur_stream_type)) ||
		 ((g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV) &&
	      (ctrl->cur_stream_type)) )
	{
		ctrl->cur_stream_type = !ctrl->cur_stream_type;
		eutel_channel_group_all_build();
	}

	if (ctrl->arg_array == NULL || ctrl->use_array == NULL )
		return -1 ;

	return ctrl->use_total ;
}

void _eutel_channel_prog_set_pos_lcn(int pos, int lcn)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	ctrl->ops_pos = pos ;
	ctrl->ops_lcn = lcn ;
}

int eutel_channel_prog_public_set_lcn(int pos)
{
	int i;
	int sel = -1 ;
	int lcn = 0 ;
	GxMsgProperty_NodeByPosGet node = {0};

	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;

	if ( (ctrl->ops_pos == pos) && (ctrl->ops_lcn>0) )
	{
		eutel_channel_set_play_lcn(ctrl->ops_lcn);
		return 0 ;
	}

	if (ctrl->arg_array == NULL || ctrl->use_array == NULL || ctrl->use_total <=0)
	{
		EUTEL_ERR("eutel_channel_prog_public_set_lcn exit \n");
		return 0 ;
	}

	if (ctrl->cur_stream_type == 0)
		lcn = ctrl->cur_tv_lcn ;
	else
		lcn = ctrl->cur_ra_lcn ;

	node.node_type = NODE_PROG;
	node.pos = pos;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

	for(i=0;i<ctrl->use_total;i++)
	{
		if ( ctrl->arg_array[ctrl->use_array[i].pos].prog_id == node.prog_data.id)
		{
			if ( ctrl->use_array[i].lcn == lcn )
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

	if ( sel < 0 )
	{
		EUTEL_SINFO("eutel_channel_prog_public_set_lcn default \n");
		sel = 0 ;
	}

	eutel_channel_set_play_lcn(ctrl->use_array[sel].lcn);

	return 0;
}

int eutel_channel_prog_public_get_play_lcn(void)
{
	int lcn = 0 ;
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;

	if (ctrl->cur_stream_type == 0)
		lcn = ctrl->cur_tv_lcn ;
	else
		lcn = ctrl->cur_ra_lcn ;

	return lcn ;
}

int eutel_channel_prog_public_get_next_pos(int pos)
{
	int i;
	int id = 0 ;
	int real_pos = 0 ;
	int sel = -1 ;
	int lcn = 0 ;
	GxMsgProperty_NodeByPosGet node = {0};

	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	if (ctrl->arg_array == NULL || ctrl->use_array == NULL || ctrl->use_total <= 0)
	{
		EUTEL_ERR("eutel_channel_prog_public_get_next_pos exit \n");
		return -1 ;
	}

	if (ctrl->cur_stream_type == 0)
		lcn = ctrl->cur_tv_lcn ;
	else
		lcn = ctrl->cur_ra_lcn ;

	node.node_type = NODE_PROG;
	node.pos = pos;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

	for(i=0;i<ctrl->use_total;i++)
	{
		if ( ctrl->arg_array[ctrl->use_array[i].pos].prog_id == node.prog_data.id)
		{
			if ( ctrl->use_array[i].lcn == lcn )
			{
				if ((i+1) >= ctrl->use_total)
					sel = 0 ;
				else
					sel = i+1 ;
				break;
			}
			else
			{
				if (sel < 0 )
				{
					if ((i+1) >= ctrl->use_total)
						sel = 0 ;
					else
						sel = i + 1;
				}
			}
		}
	}

	if (sel < 0 )
	{
		EUTEL_SINFO("eutel_channel_prog_public_get_next_pos default \n");
		sel = 0 ;
	}

	id = ctrl->arg_array[ctrl->use_array[sel].pos].prog_id;
	app_get_prog_pos_by_id_cur(id,&real_pos);
	if ( real_pos == pos )
	{
		g_AppPlayOps.normal_play.play_count = pos + 1 ;
	}
	_eutel_channel_prog_set_pos_lcn(real_pos,ctrl->use_array[sel].lcn);
	return real_pos ;
}

int eutel_channel_prog_public_get_prev_pos(int pos)
{
	int i;
	int id = 0 ;
	int real_pos = 0 ;
	int sel = -1 ;
	int lcn = 0 ;
	GxMsgProperty_NodeByPosGet node = {0};

	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	if (ctrl->arg_array == NULL || ctrl->use_array == NULL || ctrl->use_total <= 0)
	{
		EUTEL_ERR("eutel_channel_prog_public_get_prev_pos exit \n");
		return -1 ;
	}

	if (ctrl->cur_stream_type == 0)
		lcn = ctrl->cur_tv_lcn ;
	else
		lcn = ctrl->cur_ra_lcn ;

	node.node_type = NODE_PROG;
	node.pos = pos;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

	for(i=0;i<ctrl->use_total;i++)
	{
		if ( ctrl->arg_array[ctrl->use_array[i].pos].prog_id == node.prog_data.id)
		{
			if ( ctrl->use_array[i].lcn == lcn )
			{
				if (i <= 0)
					sel = ctrl->use_total-1 ;
				else
					sel = i-1 ;
				break;
			}
			else
			{
				if (sel < 0 )
				{
					if (i <= 0)
						sel = ctrl->use_total-1 ;
					else
						sel = i-1 ;
				}
			}
		}
	}

	if (sel < 0 )
	{
		EUTEL_SINFO("eutel_channel_prog_public_get_prev_pos default \n");
		sel = 0 ;
	}

	id = ctrl->arg_array[ctrl->use_array[sel].pos].prog_id;
	app_get_prog_pos_by_id_cur(id,&real_pos);
	if ( real_pos == pos )
	{
		g_AppPlayOps.normal_play.play_count = pos + 1 ;
	}
	_eutel_channel_prog_set_pos_lcn(real_pos,ctrl->use_array[sel].lcn);
	return real_pos ;
}

int eutel_channel_prog_public_get_cur_pos(int pos)
{
	int id = 0 ;
	int real_pos = 0 ;
	int i;
	int sel = -1 ;
	int lcn = 0 ;
	GxMsgProperty_NodeByPosGet node = {0};

	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	if (ctrl->arg_array == NULL || ctrl->use_array == NULL || ctrl->use_total <=0)
	{
		EUTEL_ERR("eutel_channel_prog_public_get_cur_pos exit \n");
		return -1 ;
	}

	if (ctrl->cur_stream_type == 0)
		lcn = ctrl->cur_tv_lcn ;
	else
		lcn = ctrl->cur_ra_lcn ;

	node.node_type = NODE_PROG;
	node.pos = pos;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

	for(i=0;i<ctrl->use_total;i++)
	{
		if ( ctrl->arg_array[ctrl->use_array[i].pos].prog_id == node.prog_data.id)
		{
			if ( ctrl->use_array[i].lcn == ctrl->prev_lcn )
			{
				sel = i ;
				break;
			}
			else
			{
				if ( ctrl->use_array[i].lcn == lcn )
				{
					sel = i ;
				}
				else
				{
					if ( sel < 0 )
						sel = i ;
				}
			}
		}
	}

	if ( sel < 0 )
	{
		EUTEL_SINFO("eutel_channel_prog_public_get_cur_pos default \n");
		sel = 0 ;
	}

	id = ctrl->arg_array[ctrl->use_array[sel].pos].prog_id;
	app_get_prog_pos_by_id_cur(id,&real_pos);
	_eutel_channel_prog_set_pos_lcn(real_pos,ctrl->use_array[sel].lcn);
	return real_pos;
}

int eutel_channel_prog_public_real_pos(int sel)
{
	int id = 0 ;
	int pos = 0 ;

	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	if (ctrl->arg_array == NULL || ctrl->use_array == NULL || ctrl->use_total <=0)
	{
		EUTEL_ERR("eutel_channel_prog_public_real_pos exit \n");
		return -1 ;
	}

	if (sel >= ctrl->use_total)
	{
		EUTEL_SINFO("eutel_channel_prog_public_real_pos default \n");
		return 0 ;
	}

	id = ctrl->arg_array[ctrl->use_array[sel].pos].prog_id;

	app_get_prog_pos_by_id_cur(id,&pos);

	_eutel_channel_prog_set_pos_lcn(pos,ctrl->use_array[sel].lcn);

	return pos ;
}

int eutel_channel_prog_public_real_select(int pos)
{
	int i;
	int sel = -1 ;
	int lcn = 0 ;
	GxMsgProperty_NodeByPosGet node = {0};

	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	if (ctrl->arg_array == NULL || ctrl->use_array == NULL || ctrl->use_total <=0)
	{
		EUTEL_ERR("eutel_channel_prog_public_real_select exit \n");
		return -1;
	}

	if ( ctrl->cur_stream_type == 0 )
		lcn = ctrl->cur_tv_lcn ;
	else
		lcn = ctrl->cur_ra_lcn ;

	node.node_type = NODE_PROG;
	node.pos = pos;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

	for(i=0;i<ctrl->use_total;i++)
	{
		if ( ctrl->arg_array[ctrl->use_array[i].pos].prog_id == node.prog_data.id)
		{
			if ( ctrl->use_array[i].lcn == lcn )
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
		EUTEL_SINFO("eutel_channel_prog_public_real_select default \n");
		sel = 0 ;
	}

	return sel;
}

int eutel_channel_prog_public_get_id(int sel)
{
	int id = 0 ;

	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	if (ctrl->arg_array == NULL || ctrl->use_array == NULL || ctrl->use_total <=0)
	{
		EUTEL_ERR("eutel_channel_prog_public_get_id exit \n");
		return -1 ;
	}
	if (sel >= ctrl->use_total)
	{
		EUTEL_SINFO("eutel_channel_prog_public_get_id default \n");
		return 0 ;
	}

	id = ctrl->arg_array[ctrl->use_array[sel].pos].prog_id;

	return id ;
}

int eutel_channel_prog_public_get_lcn(int sel)
{
	int lcn = 0 ;

	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	if (ctrl->arg_array == NULL || ctrl->use_array == NULL || ctrl->use_total <=0)
	{
		EUTEL_ERR("eutel_channel_prog_public_get_lcn exit \n");
		return sel ;
	}
	if (sel >= ctrl->use_total)
	{
		EUTEL_SINFO("eutel_channel_prog_public_get_lcn default \n");
		return sel ;
	}

	lcn = ctrl->use_array[sel].lcn;

	return lcn ;
}

int eutel_channel_logo_key_refresh(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int i;
	char key[32];

	for (i = 0; i < ctrl->arg_total; i++)
	{
		ctrl->arg_array[i].logo_flag = 0;
		char *filepath = ctrl->arg_array[i].channel_logo;
		char *logo_path = richepg_get_logo_path(filepath, RICHEPG_LOGO_CHANNEL);
		if (logo_path)
		{
			ctrl->arg_array[i].logo_flag = 1;
			snprintf(key, sizeof(key), EUTEL_CH_KEY, ctrl->arg_array[i].channel_id);
			gal_add_key_path(key, logo_path);
		}
	}

	return 0;
}

int eutel_channel_ear_logo_update(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	RichepgProgListCtrl *clist = richepg_prog_list_ctrl_get();
	RichepgProgListProg *prog_ex = NULL;
	int i = 0, j = 0;

	block_mem_mgr_free_data(&ctrl->ear_mgr);
	if (clist->prog_num == 0)
		richepg_proglist_parse();

	for (i = 0; i < ctrl->arg_total; i++)
	{
		prog_ex = NULL;
		for (j = 0; j < clist->prog_num; j++)
		{
			// ear_logo
			if (clist->prog_array[j].id == ctrl->arg_array[i].channel_id)
			{
				prog_ex = &clist->prog_array[j];
				break;
			}
		}

		if (prog_ex && prog_ex->ear_logo && strlen(prog_ex->ear_logo) > 0)
		{
			if ((ctrl->arg_array[i].ear_logo = block_mem_mgr_dup_str(prog_ex->ear_logo, &ctrl->ear_mgr)) == NULL)
			{
				EUTEL_ERR("Add data err!\n");
				ctrl->arg_array[i].ear_logo = NULL;
			}
		}
		else
		{
			ctrl->arg_array[i].ear_logo = NULL;
		}
	}
	block_mem_mgr_adjust_size(&ctrl->ear_mgr);
	richepg_proglist_release();

	return 0;
}

char *eutel_cur_ear_logo_path_get(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int sel = eutel_channel_play_use_sel_get();
	char *filepath = NULL;
	char *logo_path = NULL;
	static char global_path[128];

	if (sel >= 0)
	{
		filepath = ctrl->arg_array[ctrl->use_array[sel].pos].ear_logo;
		logo_path = richepg_get_logo_path(filepath, RICHEPG_LOGO_EAR_CHANNEL);
		if (logo_path)
			return logo_path;
	}

	memset(global_path, 0 ,sizeof(global_path));
	if (richepg_store_getstr(RICHEPG_EAR_GLOGO, global_path, sizeof(global_path)) != NULL)
	{
		if (access(global_path, F_OK) == 0)
			return global_path;
	}

	return NULL;
}

char *eutel_cur_channel_name_get(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int sel = eutel_channel_play_use_sel_get();

	if (sel >= 0)
	{
		return ctrl->arg_array[ctrl->use_array[sel].pos].channel_name;
	}
	return NULL;
}

int eutel_cur_channel_id_get(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int sel = eutel_channel_play_use_sel_get();

	if (sel >= 0)
	{
		return ctrl->arg_array[ctrl->use_array[sel].pos].channel_id;
	}
	return -1;
}

int eutel_cur_channel_prog_id_get(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int sel = eutel_channel_play_use_sel_get();

	if (sel >= 0)
	{
		return ctrl->arg_array[ctrl->use_array[sel].pos].prog_id;
	}
	return -1;
}

static char s_realpath[128];
void eutel_lineup_logo_key_add(void)
{
	memset(s_realpath, 0 ,sizeof(s_realpath));
	if (richepg_store_getstr(RICHEPG_LINEUP_LOGO, s_realpath, sizeof(s_realpath)) != NULL)
	{
		if (access(s_realpath, F_OK) == 0)
			gal_add_key_path(EUTEL_LINEUP_LOGO_KEY, s_realpath);
	}
}

char *eutel_lineup_logo_key_get(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	memset(s_realpath, 0 ,sizeof(s_realpath));
	if (richepg_store_getstr(RICHEPG_LINEUP_LOGO, s_realpath, sizeof(s_realpath)) != NULL)
	{
		if (access(s_realpath, F_OK) == 0)
			return EUTEL_LINEUP_LOGO_KEY;
	}

	if (ctrl->cur_stream_type == 0)
		return EUTEL_CH_KEY_DFTV;
	else
		return EUTEL_CH_KEY_DFRA;
}

char *eutel_channel_logo_key_get(int use_pos)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	static char key[32];
    bool full_mode = (richepg_store_getint(RICHEPG_ECL_STATE) && richepg_get_profile_type(USB_ECL_TIMESTAMP) == RICHEPG_FULL_PROFILE) ? true: false;;

	if (use_pos >= 0 && use_pos < ctrl->use_total)
	{
		if (ctrl->arg_array[ctrl->use_array[use_pos].pos].logo_flag == 1)
		{
			snprintf(key, sizeof(key), EUTEL_CH_KEY, ctrl->arg_array[ctrl->use_array[use_pos].pos].channel_id);
			return key;
		}
	}

    if(true == s_eutel_logo_display_mode && false == full_mode)
        return NULL;

    return eutel_lineup_logo_key_get();
}

char *eutel_channel_logo_key_get_by_channel_id(int channel_id)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int i = 0;
	static char key[32];

	for (i = 0; i < ctrl->arg_total; i++) {
		if (channel_id == ctrl->arg_array[i].channel_id) {
			if (ctrl->arg_array[i].logo_flag == 1)
			{
				snprintf(key, sizeof(key), EUTEL_CH_KEY, ctrl->arg_array[i].channel_id);
				return key;
			}
			break;
		}
	}

	return eutel_lineup_logo_key_get();
}

int eutel_channel_ctrl_data_easy_save(void)
{
#ifdef APP_EUTEL_CHANNEL_EASY_DATA_SUPPORT
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	extern int app_eutel_channel_easy_data_save(EutelChannelArg *ary,EutelChannelLcnArg *use_array,unsigned int size);
	app_eutel_channel_easy_data_save(ctrl->arg_array,ctrl->use_array,ctrl->use_total);
#endif
	return 0 ;
}

int eutel_channel_group_all_build(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int i = 0;
	int cnt = 0;
	int lcn = 0;
	bool is_find = false;

	if (ctrl->arg_total == 0)
		return 0;

	lcn = (ctrl->cur_stream_type == 0) ? ctrl->cur_tv_lcn : ctrl->cur_ra_lcn;
	for (i = 0; i < ctrl->sort_total; i++)
	{
		if (ctrl->cur_stream_type == ctrl->arg_array[ctrl->sort_array[i].pos].stream_flag)
		{
			ctrl->use_array[cnt].pos = ctrl->sort_array[i].pos;
			ctrl->use_array[cnt].lcn = ctrl->sort_array[i].lcn;
			if (ctrl->use_array[cnt].lcn == lcn)
				is_find = true;
			cnt++;
		}
	}
	ctrl->use_total = cnt;
	ctrl->filter_sel = 0;
	if (!is_find)
	{
		lcn = ctrl->use_array[0].lcn;
		(ctrl->cur_stream_type == 0) ?  (ctrl->cur_tv_lcn = lcn) : (ctrl->cur_ra_lcn = lcn);
	}

	g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
	eutel_channel_ctrl_data_easy_save();

	return cnt;
}

int eutel_channel_group_mark_build(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int i = 0;
	int cnt = 0;

	if (ctrl->arg_total == 0)
		return 0;

	for (i = 0; i < ctrl->sort_total; i++)
	{
		if (ctrl->cur_stream_type == ctrl->arg_array[ctrl->sort_array[i].pos].stream_flag)
		{
			if (ctrl->arg_array[ctrl->sort_array[i].pos].mark_flag)
			{
				ctrl->use_array[cnt].pos = ctrl->sort_array[i].pos;
				ctrl->use_array[cnt].lcn = ctrl->sort_array[i].lcn;
#if 1 // mark list only show channels with multiple LCNs once.
				int j = 0;
				bool repeat_flag = false;
				for (j = 0; j < cnt; j++)
				{
					if (ctrl->use_array[j].pos == ctrl->use_array[cnt].pos)
					{
						repeat_flag = true;
						break;
					}
				}
				if (!repeat_flag)
					++cnt;
#else
				++cnt;
#endif
			}
		}
	}
	ctrl->use_total = cnt;
	ctrl->filter_sel = 1;

	eutel_channel_ctrl_data_easy_save();

	return cnt;
}

int eutel_channel_group_filter_build(int sel)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int i = 0, j = 0;
	int cnt = 0;
	int filter_id = 0;

	if (ctrl->arg_total == 0 || sel >= ctrl->genre_total || sel < 0)
		return 0;

	filter_id = ctrl->genre_array[sel].id;
	for (i = 0; i < ctrl->sort_total; i++)
	{
		for (j = 0; j < ctrl->arg_array[ctrl->sort_array[i].pos].genre_num; j++)
		{
			if (ctrl->cur_stream_type == ctrl->arg_array[ctrl->sort_array[i].pos].stream_flag)
			{
				if (ctrl->arg_array[ctrl->sort_array[i].pos].genre_ids[j] == filter_id)
				{
					ctrl->use_array[cnt].pos = ctrl->sort_array[i].pos;
					ctrl->use_array[cnt].lcn = ctrl->sort_array[i].lcn;
					cnt++;
				}
			}
		}
	}
	ctrl->use_total = cnt;
	ctrl->filter_sel = sel + CHANNEL_FILTER_START_SEL;

	eutel_channel_ctrl_data_easy_save();

	return cnt;
}

int eutel_channel_group_build(int sel)
{
	int cnt = 0;
	switch (sel)
	{
		case 0: cnt = eutel_channel_group_all_build(); break;
		case 1: cnt = eutel_channel_group_mark_build(); break;
		default: cnt = eutel_channel_group_filter_build(sel - CHANNEL_FILTER_START_SEL); break;
	}
	return cnt;
}

char *eutel_channel_group_title(const char *widget)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	char *tmp_str = NULL;

	switch (ctrl->filter_sel)
	{
		case 0:
			tmp_str = STR_ID_ALL;
			break;
		case 1:
			tmp_str = STR_ID_MARK_LIST;
			break;
		default:
			if (ctrl->filter_sel - CHANNEL_FILTER_START_SEL < ctrl->genre_total)
				tmp_str = ctrl->genre_array[ctrl->filter_sel - CHANNEL_FILTER_START_SEL].name;
			break;
	}
	if (widget)
	{
		GxBusPmDataSat *psat = NULL ;
		char buff[MAX_SAT_NAME*3] ;
		psat = app_eutel_cur_sat_data_get();
		if ( psat != NULL )
		{
			memset(buff,0,MAX_SAT_NAME*3);
			sprintf(buff,"%s-%s",psat->sat_s.sat_name,tmp_str);
			GUI_SetProperty(widget, "string",buff);
		}
		else
		{
			GUI_SetProperty(widget, "string", tmp_str);
		}
	}

	return tmp_str;
}

static char *_eutel_strcasestr(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return NULL;

	int i;
	int len1 = strlen(s1);
	int len2 = strlen(s2);
	int tries = len1 - len2 + 1;

	for (i = 0; i < tries; i++)
	{
		if (strncasecmp(s1+i, s2, len2) == 0)
			return (char*)(s1+i);
	}

	return NULL;
}

int eutel_channel_group_find_build(char *find_str)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int i = 0;
	int cnt = 0;

	if (ctrl->arg_total == 0)
		return 0;

	if (find_str == NULL || strlen(find_str) == 0)
		return eutel_channel_group_all_build();

	for (i = 0; i < ctrl->sort_total; i++)
	{
		if (ctrl->cur_stream_type == ctrl->arg_array[ctrl->sort_array[i].pos].stream_flag)
		{
			if (_eutel_strcasestr((char*)ctrl->arg_array[ctrl->sort_array[i].pos].channel_name, find_str))
			{
				ctrl->use_array[cnt].pos = ctrl->sort_array[i].pos;
				ctrl->use_array[cnt].lcn = ctrl->sort_array[i].lcn;
				cnt++;
			}
		}
	}
	ctrl->use_total = cnt;
	ctrl->filter_sel = 0;

	return cnt;
}

int eutel_channel_mark_change(int sel, bool save_flag)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	bool flag = false;
	GxBusPmDataProg prog = {0};

	if (ctrl->use_total == 0 || sel >= ctrl->use_total || sel < 0)
		return -1;

	flag = (ctrl->arg_array[ctrl->use_array[sel].pos].mark_flag == 1) ? false : true;
	ctrl->arg_array[ctrl->use_array[sel].pos].mark_flag = flag ? 1 : 0;

	if (save_flag)
	{
		if (GxBus_PmProgGetById(ctrl->arg_array[ctrl->use_array[sel].pos].prog_id, &prog) == GXCORE_SUCCESS)
		{
			ctrl->arg_array[ctrl->use_array[sel].pos].mark_flag_bak = ctrl->arg_array[ctrl->use_array[sel].pos].mark_flag;
			if (flag)
				prog.favorite_flag |=  (1 << EUTEL_BOOKMARK_PROG_POS);
			else
				prog.favorite_flag &= ~(1 << EUTEL_BOOKMARK_PROG_POS);

			GxBus_PmProgInfoModify(&prog);
			GxBus_PmSync(GXBUS_PM_SYNC_PROG);
		}
	}

	return 0;
}

int eutel_channel_mark_save_all(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int i = 0;
	int cnt = 0;
	uint32_t *id_array = NULL;
	GxBusPmDataFavItem fav = {0};

	if (ctrl->arg_total == 0)
		return 0;

	if ((id_array = GxCore_Calloc(ctrl->arg_total, sizeof(uint32_t))) == NULL)
	{
		EUTEL_ERR("malloc failed!\n");
		return 0;
	}

	for (i = 0; i < ctrl->arg_total; i++)
	{
		if(ctrl->arg_array[i].mark_flag_bak != ctrl->arg_array[i].mark_flag)
		{
			ctrl->arg_array[i].mark_flag_bak = ctrl->arg_array[i].mark_flag;
			id_array[cnt++] = ctrl->arg_array[i].prog_id;
		}
	}

	if (cnt > 0)
	{
		if (GxBus_PmFavGroupGetByPos(EUTEL_BOOKMARK_PROG_POS, 1, &fav) > 0)
		{
			GxBus_PmProgFavModify(fav.id, id_array, cnt);
			GxBus_PmSync(GXBUS_PM_SYNC_PROG);
			GxBus_PmSync(GXBUS_PM_SYNC_FAV);
		}
	}
	GxCore_Free(id_array);

	return cnt;
}

int eutel_channel_lock_change(int sel, bool save_flag)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	bool flag = false;
	GxBusPmDataProg prog = {0};

	if (ctrl->use_total == 0 || sel >= ctrl->use_total || sel < 0)
		return -1;

	flag = (ctrl->arg_array[ctrl->use_array[sel].pos].lock_flag == 1) ? false : true;
	ctrl->arg_array[ctrl->use_array[sel].pos].lock_flag = flag ? 1 : 0;

	if (save_flag)
	{
		if (GxBus_PmProgGetById(ctrl->arg_array[ctrl->use_array[sel].pos].prog_id, &prog) == GXCORE_SUCCESS)
		{
			ctrl->arg_array[ctrl->use_array[sel].pos].lock_flag_bak = ctrl->arg_array[ctrl->use_array[sel].pos].lock_flag;
			prog.lock_flag |= (flag) ? (GXBUS_PM_PROG_BOOL_ENABLE) : (GXBUS_PM_PROG_BOOL_DISABLE);
			GxBus_PmProgInfoModify(&prog);
			GxBus_PmSync(GXBUS_PM_SYNC_PROG);
		}
	}

	return 0;
}

int eutel_channel_lock_save_all(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int i = 0;
	int cnt = 0;
	uint32_t *id_array = NULL;

	if (ctrl->arg_total == 0)
		return 0;

	if ((id_array = GxCore_Calloc(ctrl->arg_total, sizeof(uint32_t))) == NULL)
	{
		EUTEL_ERR("malloc failed!\n");
		return 0;
	}

	for (i = 0; i < ctrl->arg_total; i++)
	{
		if(ctrl->arg_array[i].lock_flag_bak != ctrl->arg_array[i].lock_flag)
		{
			ctrl->arg_array[i].lock_flag_bak = ctrl->arg_array[i].lock_flag;
			id_array[cnt++] = ctrl->arg_array[i].prog_id;
		}
	}

	if (cnt > 0)
	{
		GxBus_PmProgBoolModify(GXBUS_PM_PROG_MODIFY_LOCK, id_array, cnt);
		GxBus_PmSync(GXBUS_PM_SYNC_PROG);
	}
	GxCore_Free(id_array);

	return cnt;
}

int eutel_channel_update_current_row(const char *widget)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int channel_id = 0;
	int sel = 0, cur_sel = 0, visible_row = 0;
	int i = 0, first_sel = 0, last_sel = 0;

	GUI_GetProperty(widget, "select", &sel);
	GUI_GetProperty(widget, "cur_sel", &cur_sel);
	GUI_GetProperty(widget, "visible_row", &visible_row);
	if (ctrl->use_total == 0 || sel >= ctrl->use_total || sel < 0)
		return -1;
	channel_id = ctrl->arg_array[ctrl->use_array[sel].pos].channel_id;
	first_sel = sel - cur_sel;
	last_sel = (ctrl->use_total - first_sel > visible_row) ? (first_sel + visible_row) : (ctrl->use_total);
	for (i = first_sel; i < last_sel; i++)
	{
		if (ctrl->arg_array[ctrl->use_array[i].pos].channel_id == channel_id)
			GUI_SetProperty(widget, "update_row", &i);
	}

	return 0;
}

int app_eutel_chlist_filter_pop(const char *widget)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int sel = 0;
	int i = 0;
	int total = 0;
	char **genre_name_array = NULL;
	PopList pop_list;

	total = ctrl->genre_total + CHANNEL_FILTER_START_SEL;

	if ((genre_name_array = GxCore_Calloc(total, sizeof(char*))) == NULL)
	{
		EUTEL_ERR("malloc failed!");
		return 0;
	}

	genre_name_array[0] = STR_ID_ALL;
	genre_name_array[1] = STR_ID_MARK_LIST;
	for (i = CHANNEL_FILTER_START_SEL; i < total; i++)
	{
		genre_name_array[i] = ctrl->genre_array[i - CHANNEL_FILTER_START_SEL].name;
	}

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.sel = ctrl->filter_sel;
	pop_list.show_num = false;
	pop_list.title = STR_ID_CHANNL_FILTER;
	pop_list.item_num = total;
	pop_list.item_content = genre_name_array;
	sel = poplist_create(&pop_list);

	if (sel < total && sel >= 0)
	{
		eutel_channel_group_build(sel);
		if (widget)
		{
			eutel_channel_group_title(widget);
		}
	}
	GxCore_Free(genre_name_array);

	return sel;
}

int app_eutel_evlist_filter_pop(const char *widget, int orig_sel)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int sel = 0;
	int i = 0;
	int total = 0;
	char **content_name_array = NULL;
	PopList pop_list;

	total = ctrl->content_total + 1;

	if ((content_name_array = GxCore_Calloc(total, sizeof(char*))) == NULL)
	{
		EUTEL_ERR("malloc failed!");
		return 0;
	}
	content_name_array[0] = STR_ID_ALL;
	for (i = 1; i < total; i++)
	{
		content_name_array[i] = ctrl->content_array[i-1].name;
	}

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.sel = orig_sel;
	pop_list.show_num= false;
	pop_list.title = STR_ID_CONTENT_FILTER;
	pop_list.item_num = total;
	pop_list.item_content = content_name_array;
	sel = poplist_create(&pop_list);

	if (sel < total && sel > 0)
	{
		if (widget)
		{
			GUI_SetProperty(widget, "string", content_name_array[sel]);
		}
	}
	GxCore_Free(content_name_array);

	return sel;
}

int eutel_channel_use_sel_get_by_prog_id(int prog_id)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int i = 0;
	int sel = -1;

	for (i = 0; i < ctrl->use_total; i++)
	{
		if (ctrl->arg_array[ctrl->use_array[i].pos].prog_id == prog_id)
		{
			sel = i;
			break;
		}
	}

	return sel;
}

int eutel_channel_use_sel_get_by_lcn(int lcn)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int i = 0;
	int sel = -1;

	for (i = 0; i < ctrl->use_total; i++)
	{
		if (ctrl->use_array[i].lcn == lcn)
		{
			sel = i;
			break;
		}
	}

	return sel;
}

int eutel_channel_play_use_sel_get(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int sel = -1;
	int lcn = 0;

	if (ctrl->use_total == 0)
		return -1;
	lcn = (ctrl->cur_stream_type == 0) ? ctrl->cur_tv_lcn : ctrl->cur_ra_lcn;
	sel = eutel_channel_use_sel_get_by_lcn(lcn);

	return sel;
}

char *eutel_channel_name_get_by_prog_id(int prog_id)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	char *channel_name = NULL;
	int i = 0;
	for (i = 0; i < ctrl->arg_total; i++)
	{
		if (ctrl->arg_array[i].prog_id == prog_id)
		{
			channel_name = (char *)ctrl->arg_array[i].channel_name;
			break;
		}
	}
	return channel_name;
}

int eutel_channel_set_play_type(bool is_radio)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	ctrl->cur_stream_type = (is_radio) ? 1 : 0;
	return 0;
}

int eutel_channel_get_play_lcn(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	if (ctrl->use_total == 0)
		return 0;
	return (ctrl->cur_stream_type == 0) ? ctrl->cur_tv_lcn : ctrl->cur_ra_lcn;
}

int eutel_channel_set_play_lcn(int lcn)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int prev_lcn = 0;

	if (ctrl->cur_stream_type == 0)
	{
		prev_lcn = ctrl->cur_tv_lcn;
		ctrl->cur_tv_lcn = lcn;
	}
	else
	{
		prev_lcn = ctrl->cur_ra_lcn;
		ctrl->cur_ra_lcn = lcn;
	}
	if (ctrl->prev_lcn != prev_lcn)
		ctrl->prev_lcn = prev_lcn;

	return 0;
}

int eutel_channel_play_by_lcn(uint32_t mode, int lcn)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int pos = -1;
	int prog_id = 0;
	int sel = 0;

	if (ctrl->use_total == 0)
		return -1;
	sel = eutel_channel_use_sel_get_by_lcn(lcn);
	if (sel < 0)
		return -1;
	prog_id = ctrl->arg_array[ctrl->use_array[sel].pos].prog_id;

	if (app_get_prog_pos_by_id_all(prog_id, &pos) >= 0) {
		eutel_channel_set_play_lcn(ctrl->use_array[sel].lcn);
		g_AppPlayOps.program_play(mode, pos);
		return pos;
	}

	return -1;
}

int eutel_channel_play_check_num(int num)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int i = 0, j = 0;

	if (ctrl->arg_total == 0)
		return -1;

	for (i = 0; i < ctrl->arg_total; i++)
	{
		for (j = 0; j < ctrl->arg_array[i].lcn_num; j++)
		{
			if (ctrl->arg_array[i].lcn_ids[j] == num)
			{
				return i;
			}
		}
	}
	return -1;
}

int eutel_channel_play_by_num(uint32_t mode, int num)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int pos = -1, sel = -1, use_sel = -1;
	int i = 0, j = 0;
	int prog_id = 0;

	if (ctrl->arg_total == 0)
		return -1;

	use_sel = eutel_channel_use_sel_get_by_lcn(num);
	if (use_sel >= 0) {
		goto end;
	}

	for (i = 0; i < ctrl->arg_total; i++)
	{
		for (j = 0; j < ctrl->arg_array[i].lcn_num; j++)
		{
			if (ctrl->arg_array[i].lcn_ids[j] == num)
			{
				sel = i;
				break;
			}
		}
		if (sel >= 0)
			break;
	}
	if (sel < 0)
		return -1;

	if (ctrl->arg_array[sel].stream_flag != ctrl->cur_stream_type)
	{
		ctrl->cur_stream_type = !ctrl->cur_stream_type;
	}
	app_richepg_chfilter_keep_sec_clear(true);
	use_sel = eutel_channel_use_sel_get_by_lcn(num);
	if (use_sel >= 0) {
		goto end;
	}

	return -1;
end:
	prog_id = ctrl->arg_array[ctrl->use_array[use_sel].pos].prog_id;
	if (app_get_prog_pos_by_id_all(prog_id, &pos) >= 0) {
		eutel_channel_set_play_lcn(ctrl->use_array[use_sel].lcn);
		g_AppPlayOps.program_play(mode, pos);
		return num;
	}
	return -1;
}

int eutel_channel_play_prev(uint32_t mode)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int pos = -1;
	int prog_id = 0;
	int sel = 0;

	if (ctrl->use_total == 0)
		return -1;
	sel = eutel_channel_play_use_sel_get();
	if (sel < 0)
		return -1;
	sel = (sel == 0) ? (ctrl->use_total-1) : (sel-1);
	prog_id = ctrl->arg_array[ctrl->use_array[sel].pos].prog_id;

	if (app_get_prog_pos_by_id_all(prog_id, &pos) >= 0) {
		eutel_channel_set_play_lcn(ctrl->use_array[sel].lcn);
		g_AppPlayOps.program_play(mode, pos);
		return pos;
	}

	return -1;
}

int eutel_channel_play_next(uint32_t mode)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int pos = -1;
	int prog_id = 0;
	int sel = 0;

	if (ctrl->use_total == 0)
		return -1;
	sel = eutel_channel_play_use_sel_get();
	if (sel < 0)
		return -1;
	sel = (sel == ctrl->use_total-1) ? (0) : (sel+1);
	prog_id = ctrl->arg_array[ctrl->use_array[sel].pos].prog_id;

	if (app_get_prog_pos_by_id_all(prog_id, &pos) >= 0) {
		eutel_channel_set_play_lcn(ctrl->use_array[sel].lcn);
		g_AppPlayOps.program_play(mode, pos);
		return pos;
	}

	return -1;
}

int eutel_channel_play_current(uint32_t mode)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int pos = -1;
	int prog_id = 0;
	int sel = -1;

	if (ctrl->use_total == 0)
		return -1;
	sel = eutel_channel_play_use_sel_get();
	if(-1 == sel)
		return -1;

	prog_id = ctrl->arg_array[ctrl->use_array[sel].pos].prog_id;

	if (app_get_prog_pos_by_id_all(prog_id, &pos) >= 0) {
		eutel_channel_set_play_lcn(ctrl->use_array[sel].lcn);
		g_AppPlayOps.program_play(mode, pos);
		return pos;
	}

	return -1;
}

int eutel_channel_play_tvradio(uint32_t mode)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int pos = -1;
	int prog_id = 0;
	int sel = -1;

	ctrl->cur_stream_type = !ctrl->cur_stream_type;
	eutel_channel_group_all_build();
	if (ctrl->use_total == 0)
		return -1;

	sel = eutel_channel_play_use_sel_get();
	if (sel < 0)
		sel = 0;
	prog_id = ctrl->arg_array[ctrl->use_array[sel].pos].prog_id;

	if (app_get_prog_pos_by_id_all(prog_id, &pos) >= 0) {
		eutel_channel_set_play_lcn(ctrl->use_array[sel].lcn);
		g_AppPlayOps.program_play(mode, pos);
		return pos;
	}

	return -1;
}

int eutel_channel_play_recall(uint32_t mode)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int pos = -1;
	int prog_id = 0;
	int sel = -1;
	int use_sel = -1;
	int i = 0;

	if (ctrl->arg_total == 0)
		return -1;

	for (i = 0; i < ctrl->sort_total; i++)
	{
		if (ctrl->sort_array[i].lcn == ctrl->prev_lcn)
		{
			sel = i;
			break;
		}
	}
	if (sel < 0)
		return -1;

	if (ctrl->arg_array[ctrl->sort_array[sel].pos].stream_flag != ctrl->cur_stream_type)
	{
		ctrl->cur_stream_type = !ctrl->cur_stream_type;
		eutel_channel_group_all_build();
	}
	use_sel = eutel_channel_use_sel_get_by_prog_id(ctrl->arg_array[ctrl->sort_array[sel].pos].prog_id);
	if (use_sel < 0)
	{
		eutel_channel_group_all_build();
	}

	sel = eutel_channel_use_sel_get_by_lcn(ctrl->prev_lcn);
	prog_id = ctrl->arg_array[ctrl->use_array[sel].pos].prog_id;

	if (app_get_prog_pos_by_id_all(prog_id, &pos) >= 0) {
		eutel_channel_set_play_lcn(ctrl->use_array[sel].lcn);
		g_AppPlayOps.program_play(mode, pos);
		return pos;
	}

	return -1;
}

int eutel_channel_play_progid(uint32_t mode, int prog_id)
{
	int pos = -1;
	int lcn = 0;

	lcn = eutel_channel_find_lcn_by_prog_id(prog_id);
	if (lcn < 0)
		return -1;

	if (app_get_prog_pos_by_id_all(prog_id, &pos) >= 0) {
		eutel_channel_set_play_lcn(lcn);
		g_AppPlayOps.program_play(mode, pos);
		return pos;
	}

	return -1;
}

int eutel_channel_find_lcn_by_prog_id(int prog_id)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int sel = -1;
	int use_sel = -1;
	int i = 0;
	int lcn = -1;

	if (ctrl->arg_total == 0)
		return -1;

	for (i = 0; i < ctrl->arg_total; i++)
	{
		if (ctrl->arg_array[i].prog_id == prog_id)
		{
			sel = i;
			break;
		}
	}
	if (sel < 0)
		return -1;

	if (ctrl->arg_array[sel].stream_flag != ctrl->cur_stream_type)
	{
		ctrl->cur_stream_type = !ctrl->cur_stream_type;
		eutel_channel_group_all_build();
	}
	use_sel = eutel_channel_use_sel_get_by_prog_id(ctrl->arg_array[sel].prog_id);
	if (use_sel < 0)
	{
		eutel_channel_group_all_build();
	}
	lcn = ctrl->arg_array[sel].lcn_ids[0];

	return lcn;
}

int eutel_channel_find_lcn_by_channel_id(int channel_id, int *prog_id)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int sel = -1;
	int use_sel = -1;
	int i = 0;
	int lcn = -1;

	if (ctrl->arg_total == 0)
		return -1;

	for (i = 0; i < ctrl->arg_total; i++)
	{
		if (ctrl->arg_array[i].channel_id == channel_id)
		{
			sel = i;
			if (prog_id)
				*prog_id = ctrl->arg_array[i].prog_id;
			break;
		}
	}
	if (sel < 0)
		return -1;

	if (ctrl->arg_array[sel].stream_flag != ctrl->cur_stream_type)
	{
		ctrl->cur_stream_type = !ctrl->cur_stream_type;
		eutel_channel_group_all_build();
	}
	use_sel = eutel_channel_use_sel_get_by_prog_id(ctrl->arg_array[sel].prog_id);
	if (use_sel < 0)
	{
		eutel_channel_group_all_build();
	}
	lcn = ctrl->arg_array[sel].lcn_ids[0];

	return lcn;
}

int eutel_channel_play_set_lcn_first(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	GxBusPmDataProg prog;
	int channel_id = 0;
	int prog_id = 0;
	int pos = -1;
	int lcn = -1;

	if (ctrl->arg_total == 0)
		return -1;

	channel_id = richepg_store_getint(RICHEPG_LINEUP_WAKEUP_CHANNEL);
	lcn = eutel_channel_find_lcn_by_channel_id(channel_id, &prog_id);

	if (lcn < 0)
	{
		EUTEL_SINFO("\033[31mwakeup channel is not find, change to play last prog!\033[0m\n");
		extern int32_t app_play_id_get(void);
		prog_id = app_play_id_get();
		EUTEL_SINFO("\033[31mpos = %d, last prog id = %d \033[0m\n",g_AppPlayOps.normal_play.play_count,prog_id);
		if ( prog_id > 0 )//last prog id is exist
		{
			if (GxBus_PmProgGetByPos(g_AppPlayOps.normal_play.play_count, 1, &prog) > 0)
			{
				prog_id = prog.id;
				lcn = eutel_channel_find_lcn_by_prog_id(prog_id);
			}
		}
	}

	if (lcn < 0)
	{
		EUTEL_SINFO("\033[31mlast prog is not find, change to play first prog!\033[0m\n");
		if (ctrl->use_total == 0)
		{
			ctrl->cur_stream_type = !ctrl->cur_stream_type;
			eutel_channel_group_all_build();
		}
		channel_id = ctrl->arg_array[ctrl->use_array[0].pos].channel_id;
		prog_id = ctrl->arg_array[ctrl->use_array[0].pos].prog_id;
		lcn = ctrl->use_array[0].lcn;
	}

	EUTEL_SINFO("\n\033[32m wakeup_channel: channel_id = %d, prog_id = %d, lcn = %d \033[0m\n", channel_id, prog_id, lcn);
	if (app_get_prog_pos_by_id_all(prog_id, &pos) >= 0) {
		extern uint32_t app_play_id_set(uint32_t id);
		app_play_id_set(prog_id);
		eutel_channel_set_play_lcn(lcn);
		g_AppPlayOps.play_list_create(&(g_AppPlayOps.normal_play.view_info));
		return pos;
	}

	return -1;
}

int eutel_channel_play_first(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	GxBusPmDataProg prog;
	int channel_id = 0;
	int prog_id = 0;
	int pos = -1;
	int lcn = -1;

	if (ctrl->arg_total == 0)
		return -1;

	channel_id = richepg_store_getint(RICHEPG_LINEUP_WAKEUP_CHANNEL);
	lcn = eutel_channel_find_lcn_by_channel_id(channel_id, &prog_id);

	if (lcn < 0)
	{
		EUTEL_SINFO("\033[31mwakeup channel is not find, change to play last prog!\033[0m\n");
		extern int32_t app_play_id_get(void);
		prog_id = app_play_id_get();
		EUTEL_SINFO("\033[31mpos = %d, last prog id = %d \033[0m\n",g_AppPlayOps.normal_play.play_count,prog_id);
		if ( prog_id > 0 )//last prog id is exist
		{
			if (GxBus_PmProgGetByPos(g_AppPlayOps.normal_play.play_count, 1, &prog) > 0)
			{
				prog_id = prog.id;
				lcn = eutel_channel_find_lcn_by_prog_id(prog_id);
			}
		}
	}

	if (lcn < 0)
	{
		EUTEL_SINFO("\033[31mlast prog is not find, change to play first prog!\033[0m\n");
		if (ctrl->use_total == 0)
		{
			ctrl->cur_stream_type = !ctrl->cur_stream_type;
			eutel_channel_group_all_build();
		}
		channel_id = ctrl->arg_array[ctrl->use_array[0].pos].channel_id;
		prog_id = ctrl->arg_array[ctrl->use_array[0].pos].prog_id;
		lcn = ctrl->use_array[0].lcn;
	}

	EUTEL_SINFO("\n\033[32m wakeup_channel: channel_id = %d, prog_id = %d, lcn = %d \033[0m\n", channel_id, prog_id, lcn);
	if (app_get_prog_pos_by_id_all(prog_id, &pos) >= 0) {
		eutel_channel_set_play_lcn(lcn);
		g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, pos);
		return pos;
	}

	return -1;
}

bool app_richepg_mode_get(void)
{
	int32_t init_value = 0;
	GxBus_ConfigGetInt(RICHEPG_MODE_KEY, &init_value, RICHEPG_MODE_DEF);
	return (init_value ? true : false);
}

void app_richepg_mode_set(bool mode)
{
	int32_t init_value = mode ? 1 : 0;
	GxBus_ConfigSetInt(RICHEPG_MODE_KEY, init_value);
}

bool app_richepg_lineup_mode_get(void)
{
	int32_t init_value = 0;
	GxBus_ConfigGetInt(RICHEPG_LINEUP_MODE_KEY, &init_value, RICHEPG_LINEUP_MODE_DEF);
	return (init_value ? true : false);
}

void app_richepg_lineup_mode_set(bool mode)
{
	int32_t init_value = mode ? 1 : 0;
	GxBus_ConfigSetInt(RICHEPG_LINEUP_MODE_KEY, init_value);
}

bool app_richepg_update_mode_get(void)
{
	int32_t init_value = 0;
	GxBus_ConfigGetInt(RICHEPG_UPDATE_MODE_KEY, &init_value, RICHEPG_UPDATE_MODE_DEF);
	return (init_value ? true : false);
}

void app_richepg_update_mode_set(bool mode)
{
	int32_t init_value = mode ? 1 : 0;
	GxBus_ConfigSetInt(RICHEPG_UPDATE_MODE_KEY, init_value);
}

int app_richepg_update_attach_get(void)
{
	int32_t init_value = 0;
	GxBus_ConfigGetInt(RICHEPG_UPDATE_ATTACH_KEY, &init_value, RICHEPG_UPDATE_ATTACH_DEF);
	return init_value;
}

void app_richepg_update_attach_set(int init_value)
{
	GxBus_ConfigSetInt(RICHEPG_UPDATE_ATTACH_KEY, init_value);
}

time_t app_richepg_start_maint_time_get(void)
{
	int init_value = 0;
	GxBus_ConfigGetInt(RICHEPG_START_MAINT_KEY, &init_value, RICHEPG_START_MAINT_DEF);
	return (time_t)init_value;
}

void app_richepg_start_maint_time_set(time_t value)
{
    if(value == app_richepg_start_maint_time_get())
        return;
	GxBus_ConfigSetInt(RICHEPG_START_MAINT_KEY, (int)value);
}

bool app_richepg_epg_view_style_get(void)
{
	int32_t init_value = 0;
	GxBus_ConfigGetInt(RICHEPG_EPG_VIEW_STYLE_KEY, &init_value, RICHEPG_EPG_VIEW_STYLE_DEF);
	return (init_value ? true : false);
}

void app_richepg_epg_view_style_set(bool mode)
{
	int32_t init_value = mode ? 1 : 0;
	GxBus_ConfigSetInt(RICHEPG_EPG_VIEW_STYLE_KEY, init_value);
}

#if EPG_VIEW_DAYS_SET_SUPPORT
int app_richepg_epg_view_days_get(void)
{
	int32_t init_value = 0;
	GxBus_ConfigGetInt(RICHEPG_EPG_VIEW_DAYS_KEY, &init_value, RICHEPG_EPG_VIEW_DAYS_DEF);
	if (init_value > RICHEPG_EPG_VIEW_DAYS_MAX)
		init_value = RICHEPG_EPG_VIEW_DAYS_MAX;
	else if (init_value < RICHEPG_EPG_VIEW_DAYS_MIN)
		init_value = RICHEPG_EPG_VIEW_DAYS_MIN;
	return init_value;
}

void app_richepg_epg_view_days_set(int init_value)
{
	if (init_value > RICHEPG_EPG_VIEW_DAYS_MAX)
		init_value = RICHEPG_EPG_VIEW_DAYS_MAX;
	else if (init_value < RICHEPG_EPG_VIEW_DAYS_MIN)
		init_value = RICHEPG_EPG_VIEW_DAYS_MIN;
	GxBus_ConfigSetInt(RICHEPG_EPG_VIEW_DAYS_KEY, init_value);
}
#endif

int app_richepg_epg_evening_start_get(void)
{
	int32_t init_value = 0;
	GxBus_ConfigGetInt(RICHEPG_EPG_EVENING_START_KEY, &init_value, RICHEPG_EPG_EVENING_START_DEF);
	if (init_value > 23)
		init_value = 23;
	return init_value;
}

void app_richepg_epg_evening_start_set(int init_value)
{
	if (init_value > 23)
		init_value = 23;
	GxBus_ConfigSetInt(RICHEPG_EPG_EVENING_START_KEY, init_value);
}

int app_richepg_epg_keep_sec_get(void)
{
	int32_t init_value = 0;
	GxBus_ConfigGetInt(RICHEPG_EPG_KEEP_SEC_KEY, &init_value, RICHEPG_EPG_KEEP_SEC_DEF);
	return init_value;
}

void app_richepg_epg_keep_sec_set(int init_value)
{
	GxBus_ConfigSetInt(RICHEPG_EPG_KEEP_SEC_KEY, init_value);
}

int app_richepg_chfilter_keep_sec_get(void)
{
	int32_t init_value = 0;
	GxBus_ConfigGetInt(RICHEPG_CHFILTER_KEEP_SEC_KEY, &init_value, RICHEPG_CHFILTER_KEEP_SEC_DEF);
	return init_value;
}

void app_richepg_chfilter_keep_sec_set(int init_value)
{
	GxBus_ConfigSetInt(RICHEPG_CHFILTER_KEEP_SEC_KEY, init_value);
}

int app_richepg_num_press_timeout_get(void)
{
	int32_t init_value = 0;
	GxBus_ConfigGetInt(RICHEPG_NUM_PRESS_TIMEOUT_KEY, &init_value, RICHEPG_NUM_PRESS_TIMEOUT_DEF);
	return init_value;
}

void app_richepg_num_press_timeout_set(int init_value)
{
	GxBus_ConfigSetInt(RICHEPG_NUM_PRESS_TIMEOUT_KEY, init_value);
}

bool app_richepg_certification_access_get(void)
{
	int32_t init_value = 0;
	GxBus_ConfigGetInt(RICHEPG_CERTIFICATION_KEY, &init_value, RICHEPG_CERTIFICATION_DEF);
	return (init_value ? true : false);
}

void app_richepg_certification_access_set(bool mode)
{
	int32_t init_value = mode ? 1 : 0;
	GxBus_ConfigSetInt(RICHEPG_CERTIFICATION_KEY, init_value);
}

/*
static int s_certification_access_flag = RICHEPG_CERTIFICATION_DEF;
void app_richepg_certification_access_set_flag(bool flag)
{
	s_certification_access_flag = flag ? 1 : 0;
}
*/

void app_richepg_config_reset(void)
{
#if PARENTAL_LOCK_SUPPORT
	GxBus_ConfigSetInt(PARENTAL_GUIDANCE_KEY, PARENTAL_GUIDANCE_VALUE);
#endif
	app_richepg_mode_set(RICHEPG_MODE_DEF);
	app_richepg_lineup_mode_set(RICHEPG_LINEUP_MODE_DEF);
	app_richepg_update_mode_set(RICHEPG_UPDATE_MODE_DEF);
	app_richepg_update_attach_set(RICHEPG_UPDATE_ATTACH_DEF);
	app_richepg_start_maint_time_set(RICHEPG_START_MAINT_DEF);
	app_richepg_epg_view_style_set(RICHEPG_EPG_VIEW_STYLE_DEF);
#if EPG_VIEW_DAYS_SET_SUPPORT
	app_richepg_epg_view_days_set(RICHEPG_EPG_VIEW_DAYS_DEF);
#endif
	app_richepg_epg_evening_start_set(RICHEPG_EPG_EVENING_START_DEF);
	app_richepg_epg_keep_sec_set(RICHEPG_EPG_KEEP_SEC_DEF);
	app_richepg_chfilter_keep_sec_set(RICHEPG_CHFILTER_KEEP_SEC_DEF);
	app_richepg_num_press_timeout_set(RICHEPG_NUM_PRESS_TIMEOUT_DEF);
	//app_richepg_certification_access_set(s_certification_access_flag);
	app_eutel_set_memory_free_flag(RICHEPG_FREE_MEMORY_DEF);
}

int app_richepg_chfilter_keep_sec_clear(bool build_flag)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;

	APP_TIMER_REMOVE(ctrl->filter_timer);
	ctrl->filter_sel_bak = 0;
	ctrl->filter_sec = 0;
	if (build_flag)
		eutel_channel_group_all_build();

	return 0;
}

static int _richepg_chfilter_keep_sec_timer_exec(void* usrdata)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int tmp_sec = app_richepg_chfilter_keep_sec_get();
	uint32_t keep_sec = (tmp_sec < 0) ? 0 : tmp_sec;
	uint32_t cur_sec = app_tick_time_sec_get();

	if (cur_sec - ctrl->filter_sec < keep_sec)
	{
		return 0;
	}
	if (GUI_CheckDialog(WND_EUTEL_MAINTENANCE) == GXCORE_SUCCESS
			|| GUI_CheckDialog(WND_UPGRADE_PROCESS) == GXCORE_SUCCESS
			|| app_channel_list_check_dialog() == GXCORE_SUCCESS
			|| app_channel_epg_check_dialog() == GXCORE_SUCCESS
		)
	{
		return 0;
	}

	app_richepg_chfilter_keep_sec_clear(true);

	return 0;
}

static bool _play_channel_in_mark_list_check(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	bool find_flag = false;
	int min_lcn = 0;
	int i = 0, j = 0;
	int *cur_lcn = (ctrl->cur_stream_type == 0) ?  (&ctrl->cur_tv_lcn) : (&ctrl->cur_ra_lcn);

	for (i = 0; i < ctrl->arg_total; i++)
	{
		if (ctrl->arg_array[i].mark_flag)
		{
			min_lcn = 0x0fffffff;
			for (j = 0; j < ctrl->arg_array[i].lcn_num; j++)
			{
				if (min_lcn > ctrl->arg_array[i].lcn_ids[j])
					min_lcn = ctrl->arg_array[i].lcn_ids[j];
				if (*cur_lcn == ctrl->arg_array[i].lcn_ids[j])
					find_flag = true;
			}
			if (find_flag)
			{
				*cur_lcn = min_lcn;
				return true;
			}
		}
	}

	return false;
}

int app_richepg_chfilter_keep_sec_check_set(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int keep_sec = app_richepg_chfilter_keep_sec_get();

	if (keep_sec == 0 || ctrl->filter_sel_bak == 0)
	{
		app_richepg_chfilter_keep_sec_clear(true);
	}
	else
	{
		if (ctrl->filter_sel_bak == 1 && !_play_channel_in_mark_list_check())
		{
			app_richepg_chfilter_keep_sec_clear(true);
		}
		else
		{
			eutel_channel_group_build(ctrl->filter_sel_bak);
			ctrl->filter_sec = app_tick_time_sec_get();
			if (keep_sec > 0)
			{
				APP_TIMER_ADD(ctrl->filter_timer, _richepg_chfilter_keep_sec_timer_exec, 1000, TIMER_REPEAT);
			}
		}
	}

	return 0;
}

char *app_richepg_date_str_get(time_t sec, bool full_flag)
{
	static char buffer[16];
	char *date_str = NULL;

	memset(buffer, 0, sizeof(buffer));
	if (full_flag)
		date_str = app_time_to_date_edit_str(sec);
	else
		date_str = app_time_to_short_date_edit_str(sec);
	if (date_str)
		strncpy(buffer, date_str, sizeof(buffer) - 1);
	APP_FREE(date_str);

	return buffer;
}

char *app_richepg_duration_str_get(time_t start_time, time_t finish_time, uint32_t flag)
{
	static char buffer[32];
	char *start_str = NULL;
	char *finish_str = NULL;
	char *date_str = NULL;
	char *join_str = NULL;

	memset(buffer, 0, sizeof(buffer));
	join_str = (flag & DURATION_SHORT_JOINER) ? "-" : " - ";
	start_str = app_time_to_hourmin_edit_str(start_time);
	finish_str = app_time_to_hourmin_edit_str(finish_time);
	if (!start_str && !finish_str)
		goto end;
	if (flag & DURATION_WITH_DATE)
	{
		if (flag & DURATION_FULL_DATE)
			date_str = app_time_to_date_edit_str(start_time);
		else
			date_str = app_time_to_short_date_edit_str(start_time);
		if (!date_str)
			goto end;
		if (app_richepg_gui_reverse())
			snprintf(buffer, sizeof(buffer), "%s%s%s %s", finish_str, join_str, start_str, date_str);
		else
			snprintf(buffer, sizeof(buffer), "%s %s%s%s", date_str, start_str, join_str, finish_str);
	}
	else
	{
		if (app_richepg_gui_reverse())
			snprintf(buffer, sizeof(buffer), "%s%s%s", finish_str, join_str, start_str);
		else
			snprintf(buffer, sizeof(buffer), "%s%s%s", start_str, join_str, finish_str);
	}

end:
	APP_FREE(start_str);
	APP_FREE(finish_str);
	APP_FREE(date_str);
	return buffer;
}

int app_richepg_limit_set(void)
{
    bool sync = false;
    size_t epg_limit_mem = 0;
    GxDiskInfo disk_info = {0};
    struct richepg_channel_tiers tiers = {0};
    struct richepg_flash_limit limit = {0};

    if(GxCore_DiskInfo("/home/gx", &disk_info) != GXCORE_SUCCESS)
        return -1;

    //memory limit
    if(app_get_ddr_size() < 128 * SIZE_UNIT_M)
    {
        /*content limit
         * MID Profile -- usb storage not insert
         * FULL Profile -- usb storage insert
            struct richepg_channel_tiers {
                bool enable;                    //false -- The following settings are invalid, if work on MID Profile will be based on the configuration in the stream; ture - The following settings are valid
                bool channelLogo;               //false -- reserve channel logo; true - abandon channel logo. valid in MID Profile.
                bool epgLogo;                   //false -- reserve epg logo; true - abandon epg logo. valid in MID Profile.
                bool epgDesc;                   //false -- reserve epg description; true - abandon epg description. valid in MID profile.
                unsigned int epgDuration;       //epg duration(unit second), 0 -- receive all; > 0 -- Only accept epg events starttime less than localtime + epgDuration. valid in MID Profile.
                unsigned int epgDescDuration;   //epg description duration(unit second), valid in MID and FULL Profile.
                                                    //FULL Profile: 0 -- receive duration depends on maxEpgDuration; > 0 -- the value need less than maxEpgDuration, if greater than maxEpgDuration, receive maxEpgDuration duration.
                                                    //MID Profile: 0 -- receive duration depends on epgDuration; > 0 -- the value need less than epgDuration, if greater than epgDuration, receive epgDuration duration.
                unsigned int maxEpgDuration;    //epg duration(unit second), 0 -- receive all; > 0 -- Only accept epg events starttime less than localtime + epgDuration. valid in FULL Profile.
            };*/
        tiers.enable = true;
        tiers.channelLogo = true;
        tiers.epgLogo = true;
        tiers.epgDesc = true;
        tiers.epgDuration = 86400;
        tiers.epgDescDuration = 86400;
        tiers.maxEpgDuration = 604800;

        /*carousel filter and save in parallel or not
         * ture  -- carousel filter and save in serial
         * false -- carousel filter and save in parallel
         */
        sync = true;

        //Parse the stored epg data file to the memory limit
        epg_limit_mem = 7 * SIZE_UNIT_M;
    }
    else
    {
        //content limit
        tiers.enable = false;

        //carousel filter and save
        sync = false;

        //Parse the stored epg data file to the memory limit
        epg_limit_mem = 15 * SIZE_UNIT_M;
    }

    //flash limit(the app_get_flash_size return compressed size)
    if(app_get_flash_size() < 8 * SIZE_UNIT_M)
    {
        /*
         * ecl_limit: the program data and logo uncompressed size limit
         * epg_limit: the epg data and logo uncompressed size limit
         * */
        limit.ecl_limit = 512 * SIZE_UNIT_K;
        limit.epg_limit = SIZE_UNIT_M;

        /*ui display style
         * true  -- if channel logo doesn't exist, in the channel list and channel info window will display blank, in epg window the program logo is replaced by the channel number
         * false -- if channel logo doesn't exist, the logo is replaced by the lineup logo
         */
        s_eutel_logo_display_mode = true;

    }
    else
    {
        limit.ecl_limit = 2 * SIZE_UNIT_M;
        limit.epg_limit = 3 * SIZE_UNIT_M;

        //ui display style
        s_eutel_logo_display_mode = false;
    }

    richepg_set_flash_limit(&limit);
    richepg_set_channel_tiers(&tiers);
    richepg_set_file_save_synchronization(sync);
    richepg_epg_ctrl_init(epg_limit_mem);

    EUTEL_INFO("\033[32mset limit, ecl=%u, epg=%u, free=%u\033[0m\n", limit.ecl_limit, limit.epg_limit, (unsigned)disk_info.freed_size);
    return 0;
}

int app_richepg_init_all(void)
{
	RichepgTsMgr *mgr = richepg_ts_mgr_get();
	int init_value = 0;
    struct image_register_func func = {0};

    func.pre = richepg_files_splice_memory_file_path_get;
    func.post = richepg_files_splice_memory_file_delete;
    gal_img_processing_func_register(&func);

	block_mem_pool_init(20, 200);
    app_richepg_limit_set();
    richepg_files_splice_ctrl_init();
	eutel_windows_link_init();
	eutel_channel_ctrl_init();
	app_eutel_ts_mgr_init();
	richepg_proglist_ctrl_init();

	app_eutel_ts_mgr_parse();
	richepg_store_init(mgr->sat_num);
	richepg_store_setsel(mgr->sat_sel);
	GxBus_ConfigGetInt(OSD_LANG_KEY, &init_value, OSD_LANG);
	richepg_store_setstr(RICHEPG_LOCALE_LANGUAGE, app_eutel_porting_get_iso6392_code(init_value));

	app_eutel_sat_fuse_init();
	app_eutel_porting_sat_opt_init();
	app_eutel_antenan_setting_ops_init();
	app_eutel_number_ops_init();
	app_eutel_channel_ops_init();

	return 0;
}

#endif

