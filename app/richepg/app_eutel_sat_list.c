#include "app.h"
#include "app_windows.h"
#include "full_screen.h"
#include "channel_common.h"
#if RICHEPG_SUPPORT
#include "app_eutel_database.h"
#include "richepg_file_splice.h"
#include "app_eutel_common.h"
#include "richepg.h"

#define  LV_EUTEL_SAT_LIST "listview_eutel_sat_list"

#define EUTUL_SAT_LIST_MODE_SATTV      (0)
#define EUTUL_SAT_LIST_MODE_FREESATID  (1)
#define EUTUL_SAT_LIST_MODE_FREESATALL (2)


struct eutul_sat_list_item
{
    unsigned short sat_id ;
    char sat_type  ; // 0: sat.tv id , 1: free sat id  2: free all
    char reser1   ;
};

struct eutul_sat_list_data
{
    struct    eutul_sat_list_item *id_array ;
    uint32_t  id_count ;
	uint32_t  sattv_count ;
	uint32_t  free_count ;
};

static struct eutul_sat_list_data s_eutel_sat_list_ctrl = {NULL,0,0,0};

extern int app_richepg_switch_mode(void);

static void _app_eutel_sat_list_ctrl_release(void)
{
    APP_FREE(s_eutel_sat_list_ctrl.id_array);
    s_eutel_sat_list_ctrl.id_count = 0 ;
    s_eutel_sat_list_ctrl.sattv_count = 0 ;
    s_eutel_sat_list_ctrl.free_count  = 0 ;
}

static int _app_eutel_satlist_get_sat_id(void)
{
    int i,j,k ;
    char temp_sat = 0 ;
    int free_sat[SYS_MAX_SAT];
    int eutel_sat[EUTUL_SAT_MAX_NUM] ;
    int count,count1,count2 ;
    GxMsgProperty_NodeByPosGet node_get = {0};
    GxBusPmViewInfo view_info = {0};

    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

    view_info.group_mode = GROUP_MODE_ALL; //all
    view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
    if ( (g_AppPlayOps.normal_play.view_info.stream_type != GXBUS_PM_PROG_TV) &&
	     (g_AppPlayOps.normal_play.view_info.stream_type != GXBUS_PM_PROG_RADIO) ){
        view_info.stream_type = GXBUS_PM_PROG_TV;
    }
    else{
        view_info.stream_type = g_AppPlayOps.normal_play.view_info.stream_type ;
    }
    GxBus_PmViewInfoMemSet(&view_info); // Only memory, not flash

    // get total sat
    GxMsgProperty_NodeNumGet sat_num = {0};
    sat_num.node_type = NODE_SAT;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&sat_num));

    _app_eutel_sat_list_ctrl_release();

    if ( sat_num.node_num <= 0 )
    {
        EUTEL_SINFO("sat list empty \n");
        goto end ;
    }

    // get total prog
    GxMsgProperty_NodeNumGet prog_num = {0};
    prog_num.node_type = NODE_PROG;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&prog_num));

    count1 = 0 ;
    count2 = 0 ;

    count1 = app_fuse_spec_add_group_id(eutel_sat,EUTUL_SAT_MAX_NUM);

    for(i=0; i<prog_num.node_num; i++)
    {
        memset(&node_get, 0, sizeof(GxMsgProperty_NodeByPosGet));
        node_get.node_type = NODE_PROG;
        node_get.pos = i;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);
        extern bool app_check_prog_visible(uint32_t prog_id);

        if(app_check_prog_visible(node_get.prog_data.id) == false)
            continue;

        if ( count2 < SYS_MAX_SAT )
        {
            for(j=0;j<count2;j++)
            {
                if ( free_sat[j] == node_get.prog_data.sat_id )
                {
                    break;
                }
            }
            if ( j == count2 )
            {	
                free_sat[count2] = node_get.prog_data.sat_id ;
                count2 ++ ;
            }
        }
    }

    count = count1 + count2 ;
    if (count <=0)
    {
        goto end ;
    }
    if ( count2 > 0 )
        count += 1 ; //add ALL

    if ((s_eutel_sat_list_ctrl.id_array = GxCore_Calloc(count, sizeof(struct eutul_sat_list_data))) == NULL)
    {
        EUTEL_SINFO("sat list malloc error \n");
        goto end ;
    }
    s_eutel_sat_list_ctrl.id_count = count ;
    s_eutel_sat_list_ctrl.sattv_count = count1 ;
    s_eutel_sat_list_ctrl.free_count  = count2 ;
    i = 0 ;

    if (count1 > 0 )
    {
        for(i=0;i<count1;i++)
        {
			k = -1 ;
            temp_sat = eutel_sat[i] ;
            for(j=i+1;j<count1;j++)
            {
                if (temp_sat > eutel_sat[j] ){
                    temp_sat = eutel_sat[j] ;
                    k = j ;
                }
            }
            if (k>=0)
            {
                temp_sat = eutel_sat[i] ;
                eutel_sat[i] = eutel_sat[k];
                eutel_sat[k] = temp_sat;
            }
        }
		
        for(i=0;i<count1;i++)
        {
            s_eutel_sat_list_ctrl.id_array[i].sat_id   = eutel_sat[i] ;
            s_eutel_sat_list_ctrl.id_array[i].sat_type = EUTUL_SAT_LIST_MODE_SATTV ;
        }
    }

    if( count2 > 0 )
    {
        s_eutel_sat_list_ctrl.id_array[i].sat_id   = 0xff ;
        s_eutel_sat_list_ctrl.id_array[i].sat_type = EUTUL_SAT_LIST_MODE_FREESATALL ;
        count1 +=1 ;
        for(i=0;i<count2;i++)
        {
            s_eutel_sat_list_ctrl.id_array[count1+i].sat_id   = free_sat[i] ;
            s_eutel_sat_list_ctrl.id_array[count1+i].sat_type = EUTUL_SAT_LIST_MODE_FREESATID ;
        }
    }

end:
    GxBus_PmViewInfoMemClear();//recovery flash viewinfo
    return 0 ;
}

static int _app_eutel_switch_satid_check_in_sattv(int i)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    int sat_sel_bak = mgr->sat_sel;

    if (mgr->sat_array[i].lineup_id == 0){
        EUTEL_SINFO("eutel: sat list lineup_id over \n");
        return -1;
    }
    app_eutel_sat_sel_set(i, true);
    if (richepg_get_profile_type(USB_ECL_TIMESTAMP) != RICHEPG_FULL_PROFILE){
        EUTEL_SINFO("eutel: sat list full profile over \n");
        app_eutel_sat_sel_set(sat_sel_bak, true);
        return -1;
    }
    if (richepg_switch_lineup_step1() < 0 || richepg_switch_lineup_step2(mgr->sat_array[i].lineup_id) < 0)
    {
        EUTEL_SINFO("eutel: sat list lineup step over \n");
        richepg_parse_release();
        app_eutel_sat_sel_set(sat_sel_bak, true);
        return -1;
    }

    return 0;
}

static int _app_eutel_api_switch_satellite_in_free_mode(int sel,int mode_change)
{
    GxBusPmViewInfo view_info;
	GxMsgProperty_NodeByPosGet node_prog_curr = {0};
    GxMsgProperty_NodeByPosGet node_prog_tmp  = {0};

    if ( mode_change == 0 )
    {
        memset(&node_prog_curr, 0, sizeof(GxMsgProperty_NodeByPosGet));
        node_prog_curr.node_type = NODE_PROG;
        node_prog_curr.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog_curr);
    }

    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

    if (s_eutel_sat_list_ctrl.id_array[sel].sat_type == EUTUL_SAT_LIST_MODE_FREESATALL)
    {
       view_info.group_mode = GROUP_MODE_ALL;	//all
    }
    else
    {
        view_info.group_mode = GROUP_MODE_SAT;
        view_info.sat_id = s_eutel_sat_list_ctrl.id_array[sel].sat_id;
    }

    view_info.skip_view_switch = VIEW_INFO_SKIP_VANISH;
    view_info.stream_type = GXBUS_PM_PROG_TV ;

    g_AppPlayOps.play_list_create(&(view_info));

    if ( mode_change == 0 )
    {
        node_prog_tmp.node_type = NODE_PROG;
        node_prog_tmp.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog_tmp);
	}

	if((mode_change >0) || (node_prog_curr.prog_data.id != node_prog_tmp.prog_data.id)
            || (g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK)
            || (g_AppFullArb.state.pause == STATE_ON))
    {
        g_AppFullArb.state.pause = STATE_OFF;
        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
        g_AppFullArb.state.tv = (g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV) ? STATE_ON : STATE_OFF;
        g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
        g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);

		app_play_current_prog(PLAY_MODE_POINT);
    }

    return 0 ;
}

static int _app_eutel_api_switch_satellite_in_sattv_mode(int index)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();

    g_AppPlayOps.program_stop();

    app_eutel_proglist_skip_set(false);

    richepg_files_splice_ctrl_release();
    richepg_files_splice_index_json_parse_all();//usb parse

    app_eutel_proglist_update2();
    richepg_switch_lineup_step3(true, false);
    richepg_parse_release();
    richepg_ts_mgr_save(index);
    mgr->sat_sel_bak = index;
    richepg_store_flush();

    eutel_channel_ctrl_build();
    eutel_channel_group_all_build();
    eutel_lineup_logo_key_add();
    richepg_epg_build_enable_set(true, true);
    richepg_epg_build(true);
    eutel_channel_play_first();
    app_eutel_maint_check_start();

	return 0 ;
}

static int _app_eutel_satlist_find_sattv_satid(int sel,int *pActive_satid)
{
    int i ;
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    if (mgr == NULL )
        return -1 ;
    if (mgr->sat_array == NULL) 
        return -1 ;

    for (i = 0; i < mgr->sat_num; i++) 
    {
        if ( s_eutel_sat_list_ctrl.id_array[sel].sat_id == mgr->sat_array[i].sat_id )
        {
            if((i == mgr->sat_sel) && (NULL != pActive_satid ))
            {
                *pActive_satid = 1 ;
			}
            break;
        }
    }
    if (i >= mgr->sat_num ){
        EUTEL_SINFO("eutel: find sattv id err, sel=%d \n",sel);
        return -1 ;
    }

	return i ;	
}

static int _app_eutel_satlist_switch_other_satellite_in_sattv(int sel)
{
    int index ;
    int regularly_sat = 0 ;

	index = _app_eutel_satlist_find_sattv_satid(sel,&regularly_sat);
    if ( -1 == index )
        return -1 ;

    app_eutel_maint_check_stop();
    richepg_epg_build_enable_set(false, true);

    if ( _app_eutel_switch_satid_check_in_sattv(index) != 0 )
    {
        richepg_epg_build_enable_set(true, false);
        app_eutel_maint_check_start();
		return -1 ;
    }

	_app_eutel_api_switch_satellite_in_sattv_mode(index);

	return  0 ;
}

static int _app_eutel_satlist_switch_free_to_sattv(int sel)
{
    int index = 0 ;
    int regularly_sat = 0 ;

	index = _app_eutel_satlist_find_sattv_satid(sel,&regularly_sat);
    if ( -1 == index )
        return -1 ;

    EUTEL_SINFO("eutel: switch mode: free to sat.tv reg=%d \n",regularly_sat);

    if ( regularly_sat )
    {
        g_AppPlayOps.program_stop();
        app_richepg_switch_mode();
        eutel_channel_play_first();
    }
    else
    {
        app_richepg_mode_set(1);
        app_eutel_maint_check_stop();
        richepg_epg_build_enable_set(false, true);

		if ( _app_eutel_switch_satid_check_in_sattv(index) != 0 )
        {
           app_richepg_mode_set(0);
		   return -1 ;
        }

        _app_eutel_api_switch_satellite_in_sattv_mode(index);
    }

    return 0 ;
}

static int _app_eutel_satlist_switch_other_satellite_in_free(int sel)
{
    _app_eutel_api_switch_satellite_in_free_mode(sel,0);

    return 0 ;
}

static int _app_eutel_satlist_switch_sattv_to_free(int sel)
{
    g_AppPlayOps.program_stop();
    app_richepg_switch_mode();

    _app_eutel_api_switch_satellite_in_free_mode(sel,1);

    return 0 ;
}

static void _app_eutel_satlist_pop_tip(char *tip, uint32_t timeout_sec)
{
    PopDlg pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.str = tip;
    pop.pos.x = 500 ;
    pop.pos.y = 100 ;
    pop.create_cb = NULL;
    pop.exit_cb = NULL;
    pop.timeout_sec = timeout_sec;
    popdlg_create(&pop);
    GUI_SetInterface("flush", NULL);
}

static int _app_eutel_satlist_listview_key_ok(void)
{
	int ret = 0;
    int sel = 0;
    int active_sel = -1;

    GUI_GetProperty(LV_EUTEL_SAT_LIST, "select", &sel);
    GUI_GetProperty(LV_EUTEL_SAT_LIST, "active", &active_sel);

    EUTEL_SINFO("eutel: listview_key_ok  %d , %d \n",sel,active_sel);
    if ( sel == active_sel )
        return 0 ;

	if( (0 > sel) || (sel >= s_eutel_sat_list_ctrl.id_count) )
        return 0;

    if(app_richepg_mode_get())
    {
        if ( s_eutel_sat_list_ctrl.id_array[sel].sat_type >= EUTUL_SAT_LIST_MODE_FREESATID )
        {
            // sat.tv to free
            EUTEL_SINFO("eutel: switch mode: sat.tv to free \n");
            _app_eutel_satlist_pop_tip(STR_ID_SWITCH_FREE_SCAN_TIP,3);
            _app_eutel_satlist_switch_sattv_to_free(sel);
        }
        else
        {
            // sat.tv to other in sat.tv
            EUTEL_SINFO("eutel: switch mode: sat.tv to other in sat.tv \n");
            _app_eutel_satlist_pop_tip(STR_ID_SWITCH_OTHER_SAT_TIP,3);
            ret = _app_eutel_satlist_switch_other_satellite_in_sattv(sel);
        }
    }
    else
    {
        if ( s_eutel_sat_list_ctrl.id_array[sel].sat_type >= EUTUL_SAT_LIST_MODE_FREESATID)
        {
            // free to other in free
            EUTEL_SINFO("eutel: switch mode: free to other in free \n");
            //_app_eutel_satlist_pop_tip(STR_ID_SWITCH_OTHER_SAT_TIP,1);
            _app_eutel_satlist_switch_other_satellite_in_free(sel);
        }
        else
        {
            // free to sat.tv
            EUTEL_SINFO("eutel: switch mode: free to sat.tv \n");
            _app_eutel_satlist_pop_tip(STR_ID_SWITCH_FTA_EPG_TIP,3);
            ret = _app_eutel_satlist_switch_free_to_sattv(sel);
        }
    }
    //popdlg_destroy();
    GUI_SetProperty(LV_EUTEL_SAT_LIST, "update_all", NULL);
    if ( ret == 0 ){
        active_sel = sel ;
        GUI_SetProperty(LV_EUTEL_SAT_LIST, "active", &active_sel);
    }
    else{
        if ( active_sel >= 0 ){
            sel = active_sel;
            GUI_SetProperty(LV_EUTEL_SAT_LIST, "select", &sel);
		    GUI_SetProperty(LV_EUTEL_SAT_LIST, "active", &active_sel);
        }
        else{
            sel = 0 ;
            GUI_SetProperty(LV_EUTEL_SAT_LIST, "select", &sel);	
		}
        _app_eutel_satlist_pop_tip("can not switch satellite, please check the usb",1);
	}

    return 0 ;
}

static int _app_eutel_sat_list_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;
        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
                case VK_BOOK_TRIGGER:
                case STBK_EXIT:
                    popdlg_destroy();
                    GUI_EndDialog(WND_EUTEL_SAT_LIST);
                    break;
					
                case STBK_DOWN:
                case STBK_UP:
                case STBK_PAGE_UP:
                case STBK_PAGE_DOWN:
                    {
						popdlg_destroy();
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;

                case STBK_OK:
                    {
                        if (g_AppPvrOps.state != PVR_DUMMY)
                        {
                            app_pvr_create_no_btn_popdlg(NULL);
                            return EVENT_TRANSFER_STOP;
                        }
                        _app_eutel_satlist_listview_key_ok();
                    }
                    break;

                default:
                    break;
            }
        default:
            break;
    }
    return ret;
}

static int _app_eutel_sat_list_list_get_data(void *usrdata)
{
    static GxMsgProperty_NodeByIdGet node;
    ListItemPara* item = NULL;
    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;
    if ( item->sel >= s_eutel_sat_list_ctrl.id_count)
        return GXCORE_ERROR;

    //col-0: num
    item->x_offset = 0;
    item->string = NULL;
    if ( EUTUL_SAT_LIST_MODE_SATTV == s_eutel_sat_list_ctrl.id_array[item->sel].sat_type )
        item->image = "richepg_company" ;
    else
        item->image = NULL;

    //col-1: sat name
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;

    if (s_eutel_sat_list_ctrl.id_array[item->sel].sat_type <= EUTUL_SAT_LIST_MODE_FREESATID)
    {
        node.node_type = NODE_SAT;
        node.id = s_eutel_sat_list_ctrl.id_array[item->sel].sat_id;
        if(GXCORE_ERROR == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node))
            return GXCORE_ERROR ;

        item->string = (char *)app_get_sat_name(&node.sat_data);
    }
    else
    {
        item->string = "All" ;
    }

    return GXCORE_SUCCESS;
}

static int _app_eutel_satlist_listview_get_actived_sat_id(void)
{
    int i,sat_id = 0 ;
    GxMsgProperty_NodeByIdGet node = {0};
    RichepgTsMgr *mgr = richepg_ts_mgr_get();

    if(app_richepg_mode_get())
    {
        for (i = 0; i < mgr->sat_num; i++) 
        {
            if ( (mgr->sat_array) && (i == mgr->sat_sel) && (mgr->sat_array[i].lineup_id != 0))
            {
                sat_id = mgr->sat_array[i].sat_id ;
                break;
            }
        }
        if ( i < mgr->sat_num )
        {
            for (i = 0; i < s_eutel_sat_list_ctrl.id_count; i++) 
            {	
                if(   ( EUTUL_SAT_LIST_MODE_SATTV == s_eutel_sat_list_ctrl.id_array[i].sat_type )
					&& ( sat_id == s_eutel_sat_list_ctrl.id_array[i].sat_id ))
                {
                    return i ;
                }
            }
        }

        EUTEL_SINFO("eutel: sat.tv mode not actived, count = %d \n",s_eutel_sat_list_ctrl.sattv_count);
        return -1 ;
    }
    else
    {
        if (g_AppChOps.get_list_total() <= 0)
        {
            EUTEL_SINFO("eutel: free mode not actived, count = %d \n",s_eutel_sat_list_ctrl.free_count);
            return -1 ;
        }

        GxBusPmViewInfo view_info;
        app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
        if (view_info.group_mode == GROUP_MODE_ALL )
        {
            for(i=0;i<s_eutel_sat_list_ctrl.id_count;i++)
            {
                if( EUTUL_SAT_LIST_MODE_FREESATALL == s_eutel_sat_list_ctrl.id_array[i].sat_type )
                {
                    return i ;
                }
            }
            EUTEL_SINFO("eutel: free mode not actived, bb err \n");
            return -1;
        }

        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

        for(i=0;i<s_eutel_sat_list_ctrl.id_count;i++)
        {
            if( (EUTUL_SAT_LIST_MODE_FREESATID == s_eutel_sat_list_ctrl.id_array[i].sat_type )
		        && ( node.prog_data.sat_id == s_eutel_sat_list_ctrl.id_array[i].sat_id ))
            {
                return i ;
            }
        }
        EUTEL_SINFO("eutel: free mode not actived, cc err \n");
        return -1 ;
    }
}

SIGNAL_HANDLER int app_eutel_sat_list_create(GuiWidget *widget, void *usrdata)
{
    int sel = 0;

    sel = _app_eutel_satlist_listview_get_actived_sat_id();
    if (sel < 0 ){
        sel = 0 ;
        GUI_SetProperty(LV_EUTEL_SAT_LIST, "select", &sel);
    }
    else{
        GUI_SetProperty(LV_EUTEL_SAT_LIST, "select", &sel);
        GUI_SetProperty(LV_EUTEL_SAT_LIST, "active", &sel);
    }
    GUI_SetFocusWidget(LV_EUTEL_SAT_LIST);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_sat_list_destroy(GuiWidget *widget, void *usrdata)
{
    _app_eutel_sat_list_ctrl_release();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_sat_list_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_sat_list_list_get_total(GuiWidget *widget, void *usrdata)
{
    return (s_eutel_sat_list_ctrl.id_count);
}

SIGNAL_HANDLER int app_eutel_sat_list_list_get_data(GuiWidget *widget, void *usrdata)
{
    _app_eutel_sat_list_list_get_data(usrdata);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_sat_list_list_keypress(GuiWidget *widget, void *usrdata)
{
    return _app_eutel_sat_list_list_keypress(widget,usrdata);
}

int app_eutel_sat_list_create_dialog(void)
{
    _app_eutel_satlist_get_sat_id();
    if ( s_eutel_sat_list_ctrl.id_count )
    {
        if (GUI_CheckDialog(WND_EUTEL_SAT_LIST) == GXCORE_ERROR)
        {
            GUI_CreateDialog(WND_EUTEL_SAT_LIST);
        }
        return 1 ;
    }
    return 0 ;
}

#endif
