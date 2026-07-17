/*
 * =====================================================================================
 *
 *       Filename:  channel_edit.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011年07月15日 09时25分49秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include "module/channel_edit.h"
#include "app_msg.h"
#include "app_send_msg.h"
#include "app_data_type.h"
#include "module/pm/gxprogram_manage_berkeley.h"
#include "app_book.h"
#include "app_fuse_ops.h"

typedef struct
{
    uint32_t pos;
#if TKGS_SUPPORT || LCN_SUPPORT
	uint32_t logicnum;
#if LCN_VISIBLE_FLAG_SUPPORT
    uint16_t visible_sel;
#endif
#endif
    uint32_t id;
    uint32_t id_bak;
    uint64_t flag;
    uint64_t flag_bak;
}list_data;

typedef struct
{
    uint32_t id;
    uint64_t flag;
    uint64_t flag_bak;
    char     name[MAX_PROG_NAME];
}name_sort;

typedef struct
{
    uint32_t id;
    uint64_t flag;
    uint64_t flag_bak;
    int32_t frequency;
}tp_sort;

typedef struct
{
    uint32_t id;
    uint64_t flag;
    uint64_t flag_bak;
    int32_t service_id;
    uint16_t lcn_num;
}serviceid_sort;


typedef struct
{
#if LCN_VISIBLE_FLAG_SUPPORT
    uint32_t visible_total;
#endif
    uint32_t visible_tvnum;
    uint32_t visible_radionum;
    uint32_t total;
    list_data* data;
}ch_list;

uint32_t ch_ed_get_list_total(void);

#if LCN_VISIBLE_FLAG_SUPPORT
static ch_list s_List = {0,0,0,0,NULL};
#else
static ch_list s_List = {0,0,0,NULL};
#endif

#if LCN_VISIBLE_FLAG_SUPPORT
static uint32_t _get_visible_pos_from_sel(uint32_t sel)
{
    int32_t lcn_config;
    uint32_t i = 0;
    GxBus_ConfigGetInt(LCN_KEY, &lcn_config, LCN_VALUE);
    if(lcn_config == 1)
    {
        for(i = sel; i < s_List.total; i++ )
        {
            if(s_List.data[i].visible_sel == sel)
            {
                sel = i;
                break;
            }
        }
    }
    return sel;
}

uint32_t _get_select_from_pos(uint32_t pos)
{
    uint32_t bak_id = 0;
    uint32_t i = 0;
    uint32_t select = 0;
    int32_t lcn_config;

    GxBus_ConfigGetInt(LCN_KEY, &lcn_config, LCN_VALUE);
    bak_id = s_List.data[pos].id_bak;
    if(bak_id == s_List.data[pos].id)
    {
        if(lcn_config == 1)
        {
            if(s_List.data[pos].visible_sel != 0xffff)
                return s_List.data[pos].visible_sel;
        }
        else
        {
            return pos;
        }
    }

    for(i = 0; i<s_List.total; i++)
    {
        if(lcn_config == 1)
        {
            if(s_List.data[i].visible_sel != 0xffff)
                select = s_List.data[i].visible_sel;
            if(bak_id == s_List.data[i].id)
                return select;
        }
        else
        {
            if(bak_id == s_List.data[i].id)
                return i;
        }
    }
    return 0;
}

static uint32_t _get_visible_prog_num(GxBusPmViewInfo* sys ,uint32_t total)
{
    uint32_t  num = 0;
    int32_t lcn_config;
    uint32_t* id = NULL;
    AppProgExtInfo prog_ext_info;
    uint32_t i = 0;

    memset(&prog_ext_info, 0, sizeof(AppProgExtInfo));
    GxBus_ConfigGetInt(LCN_KEY, &lcn_config, LCN_VALUE);
    if(lcn_config == 1)
    {
        id = GxCore_Malloc(sizeof(uint32_t)*total);
        if(id == NULL)
        {
            app_log_error("malloc id arror fail\n");
            return total;
        }
        memset(id,0,sizeof(uint32_t)*total);
        GxBus_PmProgIdArryGetByViewInfo(sys, total, id);
        for(i = 0; i < total; i++ )
        {
            if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(id[i],sizeof(AppProgExtInfo),&prog_ext_info))
            {
                if(prog_ext_info.visible_flag == LCN_VISIBLE)
                {
                    num++;
                }
            }
            else
            {
                num++;
            }
        }
        GxCore_Free(id);
        id = NULL;
        return num;
    }
    else
    {
        return total;
    }
}

uint32_t _get_visible_list_total(void)
{
    int32_t lcn_config;
    GxBus_ConfigGetInt(LCN_KEY, &lcn_config, LCN_VALUE);
    if(lcn_config == 1)
        return s_List.visible_total;
    return s_List.total;
}

static uint32_t _get_visible_next_pos(uint32_t pos)
{
    uint32_t i = 0;
    i = pos;
    int32_t lcn_config;
    while(1)
    {
        if(i == (s_List.total-1))
            i = 0;
        else
            i++;
        GxBus_ConfigGetInt(LCN_KEY, &lcn_config, LCN_VALUE);
        if(lcn_config == 1)
        {
            if(s_List.data[i].visible_sel != 0xffff)
                return i;
            if(i == pos)
                return i;
        }
        else
        {
            return i;
        }
    }
}

static uint32_t _get_visible_prev_pos(uint32_t pos)
{
    uint32_t i = 0;
    int32_t lcn_config;
    i = pos;
    while(1)
    {
        if(i == 0)
            i = s_List.total - 1;
        else
            i--;
        GxBus_ConfigGetInt(LCN_KEY, &lcn_config, LCN_VALUE);
        if(lcn_config == 1)
        {
            if(s_List.data[i].visible_sel != 0xffff)
                return i;
            if(i == pos)
                return i;
        }
        else
        {
            return i;
        }
    }
}

static bool _check_prog_visible(uint32_t prog_id)
{
    AppProgExtInfo prog_ext_info;
    int32_t lcn_config;

    memset(&prog_ext_info, 0, sizeof(AppProgExtInfo));
    GxBus_ConfigGetInt(LCN_KEY, &lcn_config, LCN_VALUE);
    if(lcn_config == 1)
    {
        if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(prog_id,sizeof(AppProgExtInfo),&prog_ext_info))
        {
            if(prog_ext_info.visible_flag == LCN_INVISIBLE)
            {
                return false;
            }
        }
    }
    return true;
}

static uint32_t _get_cur_visible_pos(uint32_t pos)
{
    uint32_t play_count = 0;
    int32_t lcn_config;
    GxBus_ConfigGetInt(LCN_KEY, &lcn_config, LCN_VALUE);
    if(lcn_config == 1)
    {
        if(s_List.data[pos].visible_sel == 0xffff)
        {
            play_count = pos;
            pos = g_AppChOps.get_prev_pos(pos);
            if(pos > play_count)
            {
                pos = g_AppChOps.get_next_pos(play_count);
            }
        }
    }
    return pos;
}
#else
static uint32_t _get_visible_pos_from_sel(uint32_t sel)
{
    return sel;
}

uint32_t _get_select_from_pos(uint32_t pos)
{
    uint32_t bak_id = 0;
    uint32_t i = 0;

    bak_id = s_List.data[pos].id_bak;
    if(bak_id == s_List.data[pos].id)
    {
        return pos;
    }

    for(i = 0; i<s_List.total; i++)
    {
        if(bak_id == s_List.data[i].id)
            return i;
    }
    return 0;
}

static uint32_t _get_visible_prog_num(GxBusPmViewInfo* sys, uint32_t total)
{
    return total;
}

uint32_t _get_visible_list_total(void)
{
    return s_List.total;
}

static uint32_t _get_visible_next_pos(uint32_t pos)
{
    uint32_t i = 0;
    i = pos;
    if(i == (s_List.total - 1))
        i = 0;
    else
        i++;
    return i;
}

static uint32_t _get_visible_prev_pos(uint32_t pos)
{
    uint32_t i = 0;
    i = pos;
    if(i == 0)
        i = s_List.total - 1;
    else
        i--;
    return i;
}

static bool _check_prog_visible(uint32_t prog_id)
{
    return true;
}

static uint32_t _get_cur_visible_pos(uint32_t pos)
{
    return pos;
}
#endif

bool app_check_prog_visible(uint32_t prog_id)
{
    return _check_prog_visible(prog_id);
}

static void ch_ed_update_prog_num(void)
{
    GxBusPmViewInfo view_info;
    uint32_t num = 0;
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

    view_info.group_mode = GROUP_MODE_ALL;//all

    view_info.taxis_mode = TAXIS_MODE_NON;
    view_info.stream_type = GXBUS_PM_PROG_TV;
    view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
    num = GxBus_PmProgNumGetByViewInfo(&view_info);
    s_List.visible_tvnum = _get_visible_prog_num(&view_info, num);
    //check radio
    view_info.stream_type = GXBUS_PM_PROG_RADIO;
    num = GxBus_PmProgNumGetByViewInfo(&view_info);
    s_List.visible_radionum = _get_visible_prog_num(&view_info, num);
    //restore viewinfo
}

static uint32_t ch_ed_get_radio_num(void)
{
    return s_List.visible_radionum;
}

static uint32_t ch_ed_get_tv_num(void)
{
    return s_List.visible_tvnum;
}

static void ch_ed_destroy(void)
{
    if (s_List.data != NULL)
    {
        GxCore_Free(s_List.data);
        s_List.data = NULL;
        s_List.total = 0;
#if LCN_VISIBLE_FLAG_SUPPORT
        s_List.visible_total = 0;
#endif
    }
}


static status_t ch_ed_create(uint32_t total)
{
    uint32_t i;
    GxMsgProperty_NodeByPosGet node = {0};
	#if TKGS_SUPPORT || LCN_SUPPORT
	AppProgExtInfo prog_ext_info;

    memset(&prog_ext_info, 0, sizeof(AppProgExtInfo));
    #endif

    ch_ed_destroy();
    if(total == 0)
    {
        app_log_info("ch_ed_create error---%d\n",__LINE__);
	    return GXCORE_ERROR;
    }

    s_List.data = (list_data*)GxCore_Malloc(sizeof(list_data)*total);
    if (s_List.data == NULL)    return GXCORE_ERROR;

    memset(s_List.data, 0, sizeof(list_data)*total);

    s_List.total = total;

    for ( i=0; i<total; i++ )
    {
       node.node_type = NODE_PROG;
       node.pos = i;
       app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

       //node.prog_data.pos 中的pos非 node.pos中的pos
       s_List.data[i].pos = node.prog_data.pos;
#if TKGS_SUPPORT
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
			s_List.data[i].logicnum = prog_ext_info.tkgs.logicnum;
		else
			 app_log_error("ch_ed_create error---%d\n",__LINE__);
#elif LCN_SUPPORT
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
        {
			s_List.data[i].logicnum = prog_ext_info.logicnum;
#if LCN_VISIBLE_FLAG_SUPPORT
            if(prog_ext_info.visible_flag == LCN_VISIBLE)
            {
                s_List.data[i].visible_sel = s_List.visible_total;
                s_List.visible_total++;
            }
            else
            {
                s_List.data[i].visible_sel = 0xffff;
            }
#endif
        }
        else
        {
#if LCN_VISIBLE_FLAG_SUPPORT
            s_List.data[i].visible_sel = s_List.visible_total;
            s_List.visible_total++;
#endif
        }
#endif
       s_List.data[i].id = node.prog_data.id ;
       s_List.data[i].id_bak = node.prog_data.id ;
       // skip
       if (node.prog_data.skip_flag == GXBUS_PM_PROG_BOOL_ENABLE)
       {
           s_List.data[i].flag |= MODE_FLAG_SKIP;
           s_List.data[i].flag_bak |= MODE_FLAG_SKIP;
       }
       // lock
       if (node.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
       {
           s_List.data[i].flag |= MODE_FLAG_LOCK;
           s_List.data[i].flag_bak |= MODE_FLAG_LOCK;
       }
       // fav
       s_List.data[i].flag |= node.prog_data.favorite_flag;
       s_List.data[i].flag_bak |= node.prog_data.favorite_flag;
    }

    return GXCORE_SUCCESS;
}



static void ch_ed_set_flag(uint32_t sel, uint64_t flag)
{
    if(s_List.data == NULL)
        return;
    sel = _get_visible_pos_from_sel(sel);
    s_List.data[sel].flag ^= flag;
}

static void ch_ed_clear_flag(uint32_t sel, uint64_t flag)
{
    if(s_List.data == NULL)
        return;
    sel = _get_visible_pos_from_sel(sel);
    s_List.data[sel].flag &= (~flag);
}

static void ch_ed_clear_group_flag(uint64_t flag)
{
    uint32_t i;
    uint16_t total;
    total = ch_ed_get_list_total();

    for(i = 0; i < total; i++)
    {
        ch_ed_clear_flag(i, flag);
    }
}

static bool ch_ed_check(uint32_t sel, uint64_t flag)
{
    if(s_List.data == NULL)
        return false;
    sel = _get_visible_pos_from_sel(sel);
    if ((s_List.data[sel].flag & flag) == 0)
    {
        return false;
    }
    return true;
}

static uint32_t ch_ed_group_check(uint64_t flag)
{
    uint32_t i;
    uint32_t ret = 0;
    uint16_t total;
    total = ch_ed_get_list_total();
    for(i = 0; i < total; i++)
    {
        if(ch_ed_check(i, flag) == true)
        {
            ret++;
        }
    }

    return ret;
}


static uint32_t ch_ed_get_id(uint32_t sel)
{
    if(sel >= s_List.total)
        return 0;
    sel = _get_visible_pos_from_sel(sel);
    return s_List.data[sel].id;
}

static void ch_ed_move(uint32_t src, uint32_t dst)
{
    uint32_t i;
    list_data data = {0};

    if (src>=g_AppChOps.get_list_total() || dst>=g_AppChOps.get_list_total()
            || g_AppChOps.get_list_total()==1 || src==dst)
    {
        return;
    }
    src = _get_visible_pos_from_sel(src);
    dst = _get_visible_pos_from_sel(dst);
    data = s_List.data[src];

    if (src < dst)
    {
        for (i=0; i<dst-src; i++)
        {
            s_List.data[src+i].id       = s_List.data[src+i+1].id;
            s_List.data[src+i].flag     = s_List.data[src+i+1].flag;
            s_List.data[src+i].flag_bak = s_List.data[src+i+1].flag_bak;
        }
    }
    else
    {
        for (i=0; i<src-dst; i++)
        {
            s_List.data[src-i].id       = s_List.data[src-i-1].id;
            s_List.data[src-i].flag     = s_List.data[src-i-1].flag;
            s_List.data[src-i].flag_bak = s_List.data[src-i-1].flag_bak;
        }
    }

    s_List.data[dst].id       = data.id;
    s_List.data[dst].flag     = data.flag;
    s_List.data[dst].flag_bak = data.flag_bak;
}

/*
 if the dst pos is in current group,then do not need move
*/
static bool pos_in_cur_group(uint32_t dst_pos)
{
	return ch_ed_check(dst_pos,MODE_FLAG_MOVE);
}


void ch_ed_group_move(uint32_t dst_pos)
{
	list_data *move_data = NULL;
	uint32_t move_cnt = 0;
	uint32_t i,j;
	uint32_t dst_id = s_List.data[dst_pos].id;
	uint32_t move_total = 0;

	move_total = ch_ed_group_check(MODE_FLAG_MOVE);

	if(move_total == 0)
        	return;

	if(move_total == s_List.total)
	{
        	ch_ed_clear_group_flag(MODE_FLAG_MOVE);
        	return;
    	}

    	move_data = (list_data*)GxCore_Malloc(sizeof(list_data)*move_total);
    	if(move_data == NULL)
        	return;

    	if( pos_in_cur_group(dst_pos) == true )
    	{
        	ch_ed_clear_group_flag(MODE_FLAG_MOVE);
            APP_FREE(move_data);
            return;
    	}

	/*
	 * move node (which typed MODE_FLAG_MOV) to a array which named move_data
	 * 把标记有MODE_FLAG_MOV的结点抠出来，移动到数组move_data中，同时重新调整s_List.data数组
        */
    	for(i = 0; i < s_List.total - move_cnt; i++)
    	{
        	if(ch_ed_check(i, MODE_FLAG_MOVE) == true)
        	{
            		move_data[move_cnt] = s_List.data[i];

	    		for(j = i; j < s_List.total - move_cnt -1; j++)
	    		{
	         		s_List.data[j].id = s_List.data[j+1].id;
	         		s_List.data[j].flag = s_List.data[j+1].flag;
	         		s_List.data[j].flag_bak = s_List.data[j+1].flag_bak;
	    		}

	    		move_cnt++;
			i--;
        	}
    	}

	/*
         * find the destination pos
	*/
    	for( i = 0; i < s_List.total - move_total; i++)
    	{
		if(dst_id == s_List.data[i].id)
        	{
        		dst_pos = i + 1;
                	break;
         	}
    	}

	for(i = s_List.total - 1,j = s_List.total - 1 - move_total; j >= dst_pos; i --,j--)
	{
		s_List.data[i].id = s_List.data[j].id;
		s_List.data[i].flag = s_List.data[j].flag;
		s_List.data[i].flag_bak = s_List.data[j].flag_bak;
	}


	for(i = dst_pos , j =0; i < dst_pos + move_total; i++,j++)
	{
		s_List.data[i].id = move_data[j].id;
		s_List.data[i].flag = move_data[j].flag;
		s_List.data[i].flag_bak = move_data[j].flag_bak;
		ch_ed_clear_flag(i, MODE_FLAG_MOVE);
	}
    APP_FREE(move_data);
}

// return "true" mean list changed
static bool ch_ed_verity(void)
{
    uint32_t i;

    for (i=0; i<s_List.total; i++)
    {
        if ((s_List.data[i].flag != s_List.data[i].flag_bak)
                || (s_List.data[i].id != s_List.data[i].id_bak))
        {
            return true;
        }
    }

    return false;
}

//static uin32_t ch_ed_total(uint64_t flag)
//{
//    uint32_t i;
//    uint32_t total;
//
//    for (i=0; i<s_List.total; i++)
//    {
//        if (s_List.data[i].flag&flag != 0)
//        {
//            total++;
//        }
//    }
//
//    return total;
//}

static void _del_book_by_prog_id(uint16_t prog_id)
{
	GxBookGet bookGet;
	BookProgStruct book_prog;
	int count = 0;
	int i;
	count = g_AppBook.get(&bookGet, BOOKMODE_PLAY);
	if(count == 0)
		return;
	for(i = 0; i < count; i++)
	{
        memcpy(&book_prog, (BookProgStruct*)bookGet.book[i].struct_buf, sizeof(BookProgStruct));
        if(prog_id == book_prog.prog_id)
            g_AppBook.remove(&bookGet.book[i]);
	}
}

static void app_play_id_del(GxBusPmDataProg prog)
{
    int i;
    GxMsgProperty_NodeNumGet   node_num;
    GxMsgProperty_NodeByIdGet sat_node;
    GxMsgProperty_NodeByPosGet node_data;
    GxMsgProperty_NodeModify   node_modify;
    uint32_t id = 0;

    sat_node.node_type = NODE_SAT;
    sat_node.id = prog.sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_node);
    if((sat_node.sat_data.cur_tv == prog.id && prog.service_type  == GXBUS_PM_PROG_TV)
            || (sat_node.sat_data.cur_radio == prog.id && prog.service_type  == GXBUS_PM_PROG_RADIO))
    {
        memset(&node_modify, 0, sizeof(GxMsgProperty_NodeModify));
        node_modify.node_type = NODE_SAT;
        memcpy(&node_modify.sat_data, &sat_node.sat_data, sizeof(GxBusPmDataSat));
        if(prog.service_type  == GXBUS_PM_PROG_TV)
            node_modify.sat_data.cur_tv = 0;
        else
            node_modify.sat_data.cur_radio = 0;
        app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
    }

    memset(&node_num, 0, sizeof(GxMsgProperty_NodeNumGet));
    node_num.node_type = NODE_FAV_GROUP;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
    for(i = 0; i < node_num.node_num; i++)
    {
        memset(&node_data, 0, sizeof(GxMsgProperty_NodeByPosGet));
        node_data.node_type = NODE_FAV_GROUP;
        node_data.pos = i;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_data);
        if((node_data.fav_item.cur_tv == prog.id && prog.service_type  == GXBUS_PM_PROG_TV)
                || (node_data.fav_item.cur_radio == prog.id && prog.service_type  == GXBUS_PM_PROG_RADIO))
        {
            memset(&node_modify, 0, sizeof(GxMsgProperty_NodeModify));
            node_modify.node_type = NODE_FAV_GROUP;
            memcpy(&node_modify.fav_item, &node_data.fav_item, sizeof(GxBusPmDataFavItem));
            if(prog.service_type  == GXBUS_PM_PROG_TV)
                node_modify.fav_item.cur_tv = 0;
            else
                node_modify.fav_item.cur_radio = 0;
            app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
        }
    }
    if(prog.service_type  == GXBUS_PM_PROG_TV)
    {
         GxBus_ConfigGetInt(ALL_TV_KEY, (int32_t *)&id, ALL_TV_ID);
         if(id == prog.id)
             GxBus_ConfigSetInt(ALL_TV_KEY, ALL_TV_ID);
    }
    else
    {
        GxBus_ConfigGetInt(ALL_RADIO_KEY, (int32_t *)&id, ALL_RADIO_ID);
        if(id == prog.id)
             GxBus_ConfigSetInt(ALL_RADIO_KEY, ALL_RADIO_ID);
    }
}


static void ch_ed_save(void)
{
    uint32_t i;
    GxMsgProperty_NodeByIdGet node = {0};
//    GxMsgProperty_NodeModify node_modify = {0};
    GxMsgProperty_NodeDelete node_delete = {0};
	#if TKGS_SUPPORT
	AppProgExtInfo prog_ext_info = {{0}};
	#endif
//    bool skip_flag = false;

    if(0 == s_List.total)
    {
        app_log_error("ch_ed_save---total error  line=%d\n",__LINE__);
        return;
    }
    node_delete.id_array = (uint32_t*)GxCore_Malloc(s_List.total*sizeof(uint32_t));
    if (node_delete.id_array == NULL)   return;

    // save lock & skip & fav
    for (i=0; i<s_List.total; i++)
    {
        if ((s_List.data[i].flag == s_List.data[i].flag_bak)
                && (s_List.data[i].id == s_List.data[i].id_bak))
        {
            continue;
        }
        node.node_type = NODE_PROG;
        node.id = s_List.data[i].id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

        // del
        if ((s_List.data[i].flag & MODE_FLAG_DEL) != 0)
        {
            node_delete.id_array[node_delete.num] = node.id;
            node_delete.num++;
            app_play_id_del(node.prog_data);
            _del_book_by_prog_id(node.id);
            continue;
        }

        // pos
        node.prog_data.pos = s_List.data[i].pos;
#if TKGS_SUPPORT
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
		{
			if (s_List.data[i].logicnum != prog_ext_info.tkgs.logicnum)
			{
				prog_ext_info.tkgs.logicnum = s_List.data[i].logicnum;
				GxBus_PmProgExtInfoAdd(node.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info);
				//node.prog_data.logicnum = s_List.data[i].logicnum;
			}
		}
		else
		{
			app_log_error("---%s, %d, err\n", __FUNCTION__, __LINE__);
		}
#endif

        // skip
        if ((s_List.data[i].flag & MODE_FLAG_SKIP) != 0)
        {
            node.prog_data.skip_flag = GXBUS_PM_PROG_BOOL_ENABLE;
//            skip_flag = true;
        }
        else
        {
            node.prog_data.skip_flag = GXBUS_PM_PROG_BOOL_DISABLE;
        }

        // lock
        if ((s_List.data[i].flag & MODE_FLAG_LOCK) != 0)
        {
            node.prog_data.lock_flag = GXBUS_PM_PROG_BOOL_ENABLE;
        }
        else
        {
            node.prog_data.lock_flag = GXBUS_PM_PROG_BOOL_DISABLE;
        }

        // fav
        node.prog_data.favorite_flag = (uint32_t)(s_List.data[i].flag & 0xffffffff);

//        if(skip_flag != true)
//        {
            s_List.data[i].flag_bak = s_List.data[i].flag;
            s_List.data[i].id_bak = s_List.data[i].id;
//        }

//        node_modify.node_type = node.node_type;
//        node_modify.prog_data = node.prog_data;
//        app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
        // modify without sync to flash, so call pm function
        GxBus_PmProgInfoModify(&(node.prog_data));
    }

    if (node_delete.num > 0)
    {
        node_delete.node_type = node.node_type;
        app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node_delete);
    }

    GxCore_Free(node_delete.id_array);

    // after modify in ram , need sync to flash
    GxBus_PmSync(GXBUS_PM_SYNC_PROG);
#if TKGS_SUPPORT
    GxBus_PmSync(GXBUS_PM_SYNC_EXT_PROG);
#endif
}

int name_cmp_(const void *a, const void *b)
{
    return strcasecmp(((name_sort*)a)->name, ((name_sort*)b)->name);
}

void ch_ed_name_sort(sort_mode mode, uint32_t *focus)
{
    // init sort struct
    uint32_t i = 0;
    uint32_t focus_id = 0;
    GxMsgProperty_NodeByIdGet node = {0};
    name_sort *sort = (name_sort*)GxCore_Calloc(sizeof(name_sort)*s_List.total, 1);

    if (sort == NULL)   return;

    for (i=0; i<s_List.total; i++)
    {
        sort[i].id          = s_List.data[i].id;
        sort[i].flag        = s_List.data[i].flag;
        sort[i].flag_bak    = s_List.data[i].flag_bak;

        node.node_type = NODE_PROG;
        node.id = s_List.data[i].id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
        memcpy(sort[i].name, node.prog_data.prog_name, MAX_PROG_NAME);
    }

    qsort(sort, s_List.total, sizeof(name_sort), name_cmp_);

    focus_id = s_List.data[*focus].id;

    if (mode == SORT_A_Z)
    {
        for (i=0; i<s_List.total; i++)
        {
           s_List.data[i].id          = sort[i].id;
           s_List.data[i].flag        = sort[i].flag;
           s_List.data[i].flag_bak    = sort[i].flag_bak;
        }
    }
    else if (mode == SORT_Z_A)
    {
        for (i=0; i<s_List.total; i++)
        {
           s_List.data[i].id          = sort[s_List.total-1-i].id;
           s_List.data[i].flag        = sort[s_List.total-1-i].flag;
           s_List.data[i].flag_bak    = sort[s_List.total-1-i].flag_bak;
        }
    }
    else
    {}

    GxCore_Free(sort);
    sort = NULL;

    for (i=0; i<s_List.total; i++)
    {
        if (focus_id == s_List.data[i].id)
        {
            *focus = i;
        }
    }
}

#if TKGS_SUPPORT || LCN_SUPPORT
static uint32_t ch_ed_get_lcn(uint32_t sel)
{
    if(sel >= s_List.total)
        return 0;
    return s_List.data[sel].logicnum;
}

int lcnnum_cmp(const void *a, const void *b)
{
    return (((serviceid_sort*)a)->lcn_num - ((serviceid_sort*)b)->lcn_num);
}

void ch_ed_lcnnum_sort(sort_mode mode, uint32_t *focus)
{
    uint32_t 					i = 0;
    uint32_t 					focus_id = 0;
	GxMsgProperty_NodeByIdGet 	node = {0};
	AppProgExtInfo prog_ext_info;

    memset(&prog_ext_info, 0, sizeof(AppProgExtInfo));

    serviceid_sort *sort = (serviceid_sort*)GxCore_Calloc(sizeof(serviceid_sort)*s_List.total, 1);
    if (sort == NULL)   return;
    focus_id = s_List.data[*focus].id;
    for (i=0; i<s_List.total; i++)
    {
        sort[i].id          = s_List.data[i].id;
        sort[i].flag        = s_List.data[i].flag;
        sort[i].flag_bak    = s_List.data[i].flag_bak;

        node.node_type = NODE_PROG;
        node.id = s_List.data[i].id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
        {
#if TKGS_SUPPORT
			sort[i].lcn_num = prog_ext_info.tkgs.logicnum;
#elif LCN_SUPPORT
            sort[i].lcn_num = prog_ext_info.logicnum;
#endif
        }
		else
        {
			app_log_error("---%s, %d, err\n", __FUNCTION__, __LINE__);
        }
	}
    qsort(sort, s_List.total, sizeof(serviceid_sort), lcnnum_cmp);
    for (i=0; i<s_List.total; i++)
    {
		s_List.data[i].logicnum   = sort[i].lcn_num;
		s_List.data[i].id          = sort[i].id;
		s_List.data[i].flag        = sort[i].flag;
		s_List.data[i].flag_bak    = sort[i].flag_bak;
    }
    GxCore_Free(sort);
    sort = NULL;

    for (i=0; i<s_List.total; i++)
    {
        if (focus_id == s_List.data[i].id)
        {
            *focus = i;
        }
    }
	return;
}
#endif

/*********************ADD FOR SERVICE ID SORT**************************************/
int serviceid_cmp(const void *a, const void *b)
{
    return (((serviceid_sort*)a)->service_id - ((serviceid_sort*)b)->service_id);
}
void ch_ed_serviceID_sort(sort_mode mode, uint32_t *focus)
{
    uint32_t 					i = 0;
    uint32_t 					focus_id = 0;
	GxMsgProperty_NodeByIdGet 	node = {0};

    serviceid_sort *sort = (serviceid_sort*)GxCore_Calloc(sizeof(serviceid_sort)*s_List.total, 1);
    if (sort == NULL)   return;

    focus_id = s_List.data[*focus].id;

    for (i=0; i<s_List.total; i++)
    {
        sort[i].id          = s_List.data[i].id;
        sort[i].flag        = s_List.data[i].flag;
        sort[i].flag_bak    = s_List.data[i].flag_bak;

        node.node_type = NODE_PROG;
        node.id = s_List.data[i].id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
		sort[i].service_id = node.prog_data.service_id;
	}

    qsort(sort, s_List.total, sizeof(serviceid_sort), serviceid_cmp);
    for (i=0; i<s_List.total; i++)
    {
       s_List.data[i].id          = sort[i].id;
       s_List.data[i].flag        = sort[i].flag;
       s_List.data[i].flag_bak    = sort[i].flag_bak;
    }

    GxCore_Free(sort);
    sort = NULL;

    for (i=0; i<s_List.total; i++)
    {
        if (focus_id == s_List.data[i].id)
        {
            *focus = i;
        }
    }
	return;
}
/*******************************************************************************************/
int tp_cmp(const void *a, const void *b)
{
    return (((tp_sort*)a)->frequency - ((tp_sort*)b)->frequency);
}

void ch_ed_tp_sort(sort_mode mode, uint32_t *focus)
{
    // init sort struct
    uint32_t i = 0;
    uint32_t focus_id = 0;
    GxMsgProperty_NodeByIdGet ProgNode = {0};
    GxMsgProperty_NodeByIdGet node = {0};
    tp_sort *sort = (tp_sort*)GxCore_Calloc(sizeof(tp_sort)*s_List.total, 1);

    if (sort == NULL)   return;

    for (i=0; i<s_List.total; i++)
    {
        sort[i].id          = s_List.data[i].id;
        sort[i].flag        = s_List.data[i].flag;
        sort[i].flag_bak    = s_List.data[i].flag_bak;

        ProgNode.node_type = NODE_PROG;
        ProgNode.id = s_List.data[i].id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &ProgNode);

	node.node_type = NODE_TP;
	node.id = ProgNode.prog_data.tp_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
        sort[i].frequency = node.tp_data.frequency;
    }

    qsort(sort, s_List.total, sizeof(tp_sort), tp_cmp);

    focus_id = s_List.data[*focus].id;


    for (i=0; i<s_List.total; i++)
    {
       s_List.data[i].id          = sort[i].id;
       s_List.data[i].flag        = sort[i].flag;
       s_List.data[i].flag_bak    = sort[i].flag_bak;
    }

    GxCore_Free(sort);
    sort = NULL;

    for (i=0; i<s_List.total; i++)
    {
        if (focus_id == s_List.data[i].id)
        {
            *focus = i;
        }
    }
}

void ch_ed_flag_sort(sort_mode mode, uint32_t *focus)
{
    int32_t i = 0;
    int32_t moved = 0;
    uint32_t focus_id = 0;

    GxMsgProperty_NodeByIdGet node = {0};
    node.node_type = NODE_PROG;

    focus_id = s_List.data[*focus].id;

    if (mode == SORT_FREE_SCRAMBLE)
    {
        for (i=0; i<s_List.total; i++)
        {
            node.id = s_List.data[i].id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

            if(node.prog_data.scramble_flag != GXBUS_PM_PROG_BOOL_ENABLE)
            {
                if(i == moved)
                {
                    moved++;
                    continue;
                }

                ch_ed_move(i, moved);
                moved++;
            }
        }
    }
    else if (mode == SORT_LOCK_UNLOCK)
    {
        for (i=0; i<s_List.total; i++)
        {
            node.id = s_List.data[i].id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

            if (node.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
            {
                if (i == moved)
                {
                    moved++;
                    continue;
                }

                ch_ed_move(i, moved);
                moved++;
            }
        }
    }
    else if (mode == SORT_FAV_UNFAV)
    {
        for (i=0; i<s_List.total; i++)
        {
            node.id = s_List.data[i].id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

            if (node.prog_data.favorite_flag != 0)
            {
                if (i == moved)
                {
                    moved++;
                    continue;
                }

                ch_ed_move(i, moved);
                moved++;
            }
        }
    }
    else if(mode == SORT_HD_SD)
    {
        for (i=0; i<s_List.total; i++)
        {
            node.id = s_List.data[i].id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

            if ((uint32_t)(node.prog_data.definition == GXBUS_PM_PROG_HD))// HD Program
            {
                if (i == moved)
                {
                    moved++;
                    continue;
                }

                ch_ed_move(i, moved);
                moved++;
            }
        }
    }
    else
    {}

    for (i=0; i<s_List.total; i++)
    {
        if (focus_id == s_List.data[i].id)
        {
            *focus = i;
        }
    }
}

void ch_ed_sort(sort_mode mode, uint32_t *focus)
{
    if(*focus >= s_List.total)
      return;
    switch(mode)
    {
        case SORT_A_Z:
        case SORT_Z_A:
            ch_ed_name_sort(mode, focus);
            break;
        case SORT_TP:
            ch_ed_tp_sort(mode, focus);
            break;
        case SORT_FREE_SCRAMBLE:
        case SORT_FAV_UNFAV:
        case SORT_LOCK_UNLOCK:
        case SORT_HD_SD:
            ch_ed_flag_sort(mode, focus);
            break;
#if TKGS_SUPPORT || LCN_SUPPORT
		case SORT_BY_LCN:
			ch_ed_lcnnum_sort(mode, focus);
			break;
#endif
		case SORT_BY_SERVICE_ID:
            ch_ed_serviceID_sort(mode, focus);
			break;
   }
    GxBus_ConfigSetInt(CHED_SORTMODE_KEY, mode);
}

// after move, list in channel edit, is differ with play list
uint32_t ch_ed_real_pos(uint32_t sel)
{
    uint32_t i = 0;
    uint32_t id = 0;
	if( s_List.data == NULL)
	{
		return 0;
	}
    if(sel >= g_AppChOps.get_list_total())
    {
        sel = 0;
    }
    sel = _get_visible_pos_from_sel(sel);
    id = s_List.data[sel].id;
    if(id ==  s_List.data[sel].id_bak)
    {
        return sel;
    }
    else
    {
        i = 0;
        for (i=0; i<s_List.total; i++)
        {
            if (id == s_List.data[i].id_bak)
            {
                return i;
            }
        }
    }
    return sel;
}

uint32_t ch_ed_get_select_pos(uint32_t pos)
{
	if( s_List.data == NULL )
	{
		return 0;
	}
    return _get_select_from_pos(pos);
}

uint32_t ch_ed_get_list_total(void)
{
    return _get_visible_list_total();
}

static uint32_t ch_ed_get_next_pos(uint32_t pos)
{
    if(pos >= s_List.total)
        return 0;
    return _get_visible_next_pos(pos);
}

static uint32_t ch_ed_get_prev_pos(uint32_t pos)
{
    if(pos >= s_List.total)
        return 0;
    return _get_visible_prev_pos(pos);
}

static bool ch_ed_get_visible_flag(uint32_t pos)
{
    uint32_t id;
    if(pos >= s_List.total)
        return false;
    id = s_List.data[pos].id;
    return app_check_prog_visible(id);
}

static uint32_t ch_ed_get_cur_pos(uint32_t pos)
{
    if(pos >= s_List.total)
        return 0;
    return _get_cur_visible_pos(pos);
}

app_channel_ops g_AppChannelPortingOps =
{
	.get_list_total  = NULL ,
	.get_next_pos    = NULL ,
	.get_prev_pos    = NULL ,
	.get_cur_pos     = NULL ,
	.real_pos        = NULL ,
	.real_select     = NULL ,
	.get_id          = NULL ,
	.check           = NULL ,
};

void app_channel_set_ops(app_channel_ops *ops)
{
	if ( ops != NULL )
	{
		memcpy(&g_AppChannelPortingOps,ops,sizeof(app_channel_ops));
	}
}

static uint32_t public_get_list_total(void)
{
	int ret = -1 ;
	if (g_AppChannelPortingOps.get_list_total)
	{
		ret = g_AppChannelPortingOps.get_list_total();
		if (ret >=0 )
			return ret ;
	}

	return ch_ed_get_list_total();
}

static uint32_t public_get_next_pos(uint32_t pos)
{
	int ret = -1 ;
	if (g_AppChannelPortingOps.get_next_pos)
	{
		ret = g_AppChannelPortingOps.get_next_pos(pos);
		if (ret >=0 )
			return ret ;
	}

	return ch_ed_get_next_pos(pos);
}

static uint32_t public_get_prev_pos(uint32_t pos)
{
	int ret = -1 ;
	if (g_AppChannelPortingOps.get_prev_pos)
	{
		ret = g_AppChannelPortingOps.get_prev_pos(pos);
		if (ret>=0)
			return ret ;
	}

	return ch_ed_get_prev_pos(pos);
}

static uint32_t public_get_cur_pos(uint32_t pos)
{
	int ret = -1 ;
	if (g_AppChannelPortingOps.get_cur_pos)
	{
		ret = g_AppChannelPortingOps.get_cur_pos(pos);
		if (ret>=0)
			return ret ;
	}

	return ch_ed_get_cur_pos(pos);
}

//根据ui显示的sel得到节目的pos
static uint32_t public_real_pos(uint32_t sel)
{
	int ret = -1 ;
	if (g_AppChannelPortingOps.real_pos)
	{
		ret = g_AppChannelPortingOps.real_pos(sel);
		if (ret>=0)
			return ret ;
	}

	return ch_ed_real_pos(sel);
}

//根据节目的pos得到ui显示的sel
static uint32_t public_real_select(uint32_t pos)
{
	int ret = -1 ;
	if (g_AppChannelPortingOps.real_select)
	{
		ret = g_AppChannelPortingOps.real_select(pos);
		if (ret>=0)
			return ret ;
	}

	return ch_ed_get_select_pos(pos);
}

//根据ui显示的sel得到节目的id
static uint32_t public_get_id(uint32_t sel)
{
	int ret = -1 ;
	if (g_AppChannelPortingOps.get_id)
	{
		ret = g_AppChannelPortingOps.get_id(sel);
		if (ret>=0)
			return ret ;
	}

	return ch_ed_get_id(sel);
}

static bool public_check(uint32_t sel,uint64_t flag)
{
	unsigned int flag1,flag2 ;
	int ret = -1 ;
	if (g_AppChannelPortingOps.check)
	{
		flag1 = ( (flag > 32) & 0xFFFFFFFF );
		flag2 = ( flag & 0xFFFFFFFF );
		ret = g_AppChannelPortingOps.check(sel,flag1,flag2);
		if (ret == 0)
			return false ;
		else if (ret > 0)
			return true ;
	}

	return ch_ed_check(sel,flag);
}

AppChOps g_AppChOps =
{
    .create     =   ch_ed_create,
    .destroy    =   ch_ed_destroy,
    .set        =   ch_ed_set_flag,
    .clear      =   ch_ed_clear_flag,
    .group_clear = ch_ed_clear_group_flag,
    .check      =   public_check,
    .group_check = ch_ed_group_check,
    .get_id     =   public_get_id,
    .move       =   ch_ed_move,
    .group_move = ch_ed_group_move,
    .sort       =   ch_ed_sort,
    .verity     =   ch_ed_verity,
    .save       =   ch_ed_save,
    .real_pos   =   public_real_pos,
    .real_select  = public_real_select, /**/
    .get_next_pos = public_get_next_pos,
    .get_prev_pos = public_get_prev_pos,
	.get_visible_flag	= 	ch_ed_get_visible_flag,
    .get_list_total = public_get_list_total,
    .get_cur_pos    = public_get_cur_pos,
    .update_prog_num = ch_ed_update_prog_num,
    .get_tv_num = ch_ed_get_tv_num,
    .get_radio_num = ch_ed_get_radio_num,
#if TKGS_SUPPORT || LCN_SUPPORT
	.get_lcn	= 	ch_ed_get_lcn,
#endif
};
