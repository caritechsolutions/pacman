#include "app.h"

#ifdef MUITI_SCREEN_SUPPORE

#include "channel_common.h"
#include "app_default_params.h"
#include "app_module.h"
#include "app_send_msg.h"

static event_list* pMultiScreentime = NULL;
static uint32_t cur_prog_pos = 0;
static uint32_t prog_total = 0;
static uint32_t frame_position = 0;

#define MULPIC_NUM_9_LINE (3)
#define MULPIC_NUM_9_ROW (3)
#define MULPIC_NUM_9          (9)
#define MULPIC_CSAN_DURATION_MS (3000)

static GxBusPmDataProg gs_Prog[MULPIC_NUM_9];
static uint32_t gs_MulpicProgNum[MULPIC_NUM_9];
static bool s_multiscreen_flag = FALSE;
extern AppPlayOps g_AppPlayOps;
extern void app_play_set_diseqc(uint32_t sat_id, uint16_t tp_id, bool ForceFlag);

uint8_t lock_right = 0;
static uint8_t init_finish = 0;

static const char *gs_MultiscreenTextNum[9] =
{"multiscreen_text0","multiscreen_text1","multiscreen_text2","multiscreen_text3",
"multiscreen_text4","multiscreen_text5","multiscreen_text6","multiscreen_text7","multiscreen_text8"};

static const char *gs_MultiscreenTextName[9] =
{"multiscreen_text9","multiscreen_text10","multiscreen_text11","multiscreen_text12",
"multiscreen_text13","multiscreen_text14","multiscreen_text15","multiscreen_text16","multiscreen_text17"};

static const char *gs_MultiscreenImg[9] =
{"multiscreen_img0","multiscreen_img1","multiscreen_img2","multiscreen_img3",
"multiscreen_img4","multiscreen_img5","multiscreen_img6","multiscreen_img7","multiscreen_img8"};

static const char *gs_MultiscreenLockImg[9] =
{"multiscreen_img9","multiscreen_img10","multiscreen_img11","multiscreen_img12",
"multiscreen_img13","multiscreen_img14","multiscreen_img15","multiscreen_img16","multiscreen_img17"};

void update_cur_program_num_and_name(void)
{
	static char chanl_cnt[8] = {0};
	static char chanl_name[32] = {0};
	int32_t channel_lock = 0;

    memset(chanl_cnt, 0, 8);
    memset(chanl_name, 0, 32);

	GUI_SetProperty(gs_MultiscreenImg[frame_position], "state", "show");
	GUI_SetProperty(gs_MultiscreenTextNum[frame_position], "state", "show");

	//GxBus_ConfigGetInt(LCN, &config, LCN_DV);
	//if(0 == config)
	{
		sprintf((void*)chanl_cnt, "%03d",gs_MulpicProgNum[frame_position]+1);
	}
	//else
	{
	//	sprintf((void*)buf, "%03d",gs_Prog[frame_position].pos);
	}
	GUI_SetProperty(gs_MultiscreenTextNum[frame_position], "string", (void*)chanl_cnt);
	GUI_SetProperty(gs_MultiscreenTextName[frame_position], "state", "show");

    if (gs_Prog[frame_position].scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
    {
        strcpy(chanl_name, "$ ");
        memcpy(chanl_name + 2, gs_Prog[frame_position].prog_name, 29);
    }
    else
    {
        memcpy(chanl_name, gs_Prog[frame_position].prog_name, 31);
    }
	GUI_SetProperty(gs_MultiscreenTextName[frame_position], "string", chanl_name);

	GxBus_ConfigGetInt(CHANNEL_LOCK_KEY, &channel_lock, CHANNEL_LOCK);
	if((channel_lock != CHANNEL_LOCK)
		&&(gs_Prog[frame_position].lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
				&& (lock_right == 0))
	{
		GUI_SetProperty(gs_MultiscreenLockImg[frame_position], "state", "show");
	}
	else
	{
		GUI_SetProperty(gs_MultiscreenLockImg[frame_position], "state", "hide");
	}

}

void page_set_program_data(void)
{
	uint32_t i = 0;

	for(i=0 ; i<MULPIC_NUM_9 ;i++)
	{
		GxBus_PmProgGetByPos(g_AppChOps.real_pos(cur_prog_pos) ,1,&gs_Prog[i]);
        gs_MulpicProgNum[i] = cur_prog_pos;
		if(cur_prog_pos == (prog_total-1))
			cur_prog_pos = 0;
		else
			cur_prog_pos++;
	}
}

void send_msg_play_multiscreen(uint8_t PosX, uint8_t PosY)
{
	PlayerWindow rect = {0};
	GxMsgProperty_PlayerMultiScreenPlay* multiScreen_play = NULL;
	int32_t channel_lock = 0;
#if 0
    //shenbin 0504 needn't care recall in multiscreen
	GxBusPmViewInfo sysinfo;

	GxBus_PmViewInfoGet(&sysinfo);
	sysinfo.tv_prog_cur = gs_Prog[frame_position].id;
	GxBus_PmViewInfoModify(&sysinfo);

	app_record_playing_prog(gs_MulpicProgNum[frame_position],&sysinfo);
#endif
    	g_AppPlayOps.normal_play.play_count = g_AppChOps.real_pos(gs_MulpicProgNum[frame_position]);

	rect.x = 10*APP_THEME_XRES/720 + 234*APP_THEME_XRES*PosX/720;
	rect.y = 2*APP_THEME_YRES/576+190*APP_THEME_YRES*PosY/576;
	rect.width = 232*APP_THEME_XRES/720;
	rect.height = 186*APP_THEME_YRES/576;

	rect.x &= 0xfffffffe;
	rect.width &= 0xfffffffe;

	GxBus_ConfigGetInt(CHANNEL_LOCK_KEY, &channel_lock, CHANNEL_LOCK);
	multiScreen_play = GxCore_Mallocz(sizeof(GxMsgProperty_PlayerMultiScreenPlay));
	if(multiScreen_play == NULL)
	{
		app_log_error("%s:Malloc failed!!!!!!!\n",__func__);
		return ;
	}

	if((channel_lock != CHANNEL_LOCK)
		&&(gs_Prog[frame_position].lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
				&& (lock_right == 0))
	{
		//if(0==init_finish)
		//{
			multiScreen_play->rect = rect;
			multiScreen_play->player = PLAYER_FOR_NORMAL;
			app_send_msg_exec(GXMSG_PLAYER_MULTISCREEN_PLAY,(void*)multiScreen_play);
			GXCORE_FREE(multiScreen_play);
			return;
		//}
		//else
		//{
			//g_AppPlayOps.poppwd_create(0,gs_MulpicProgNum[frame_position]);

    		//g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&rect);
    		//app_play_current_prog(PLAY_MODE_POINT);
			//return;
		//}
	}


	app_play_set_diseqc(gs_Prog[frame_position].sat_id, gs_Prog[frame_position].tp_id,false);
	multiScreen_play->rect = rect;
	multiScreen_play->player = PLAYER_FOR_NORMAL;
	memset(multiScreen_play->url, 0, GX_PM_MAX_PROG_URL_SIZE);
	GxBus_PmProgUrlGet(&gs_Prog[frame_position], (int8_t*)multiScreen_play->url, GX_PM_MAX_PROG_URL_SIZE);
	strcat(multiScreen_play->url,"&tsid:0");
	app_send_msg_exec(GXMSG_PLAYER_MULTISCREEN_PLAY,(void*)multiScreen_play);
	GXCORE_FREE(multiScreen_play);
}

static  int timermulti(void *userdata)
{
	static uint8_t PosY  = 0;
	static uint8_t PosX  = 0;

	GUI_SetProperty(gs_MultiscreenImg[frame_position], "state", "hide");

	frame_position++;
	if((MULPIC_NUM_9 == frame_position)
		||(prog_total == frame_position))//抓到最后一幅图后返回到第一个会面
	{
		frame_position = 0;
		init_finish = 1;
		remove_timer(pMultiScreentime);
		pMultiScreentime = NULL;
	}

	update_cur_program_num_and_name();

	PosY = frame_position/MULPIC_NUM_9_ROW;
	PosX = frame_position%MULPIC_NUM_9_LINE;

	send_msg_play_multiscreen(PosX, PosY);

	return 0;
}

static void app_multiscreen_clear(void)
{
	uint8_t i = 0;

	app_player_close(PLAYER_FOR_NORMAL);

	for(i = 0; i < MULPIC_NUM_9; i++)
	{
		GUI_SetProperty(gs_MultiscreenTextNum[i], "string", NULL);
		GUI_SetProperty(gs_MultiscreenTextName[i], "string", NULL);
		GUI_SetProperty(gs_MultiscreenLockImg[i], "state", "hide");
		GUI_SetProperty(gs_MultiscreenImg[i], "state", "hide");
	}
}

bool app_multiscreen_on(void)
{
    return s_multiscreen_flag;
}

//-----
  int multiscreen_create(GuiWidget *widget, void *usrdata)
{
	//uint32_t duration = 2000;
	uint32_t i = 0;
	static uint8_t PosY  = 0;
	static uint8_t PosX  = 0;
	int init_value = 0;

	//app_set_focus_window(MULTI_SCREEN_WIN);
	//app_screen_adjust(1); shenbin 0504
	s_multiscreen_flag = TRUE;

	frame_position = 0;

	prog_total = g_AppChOps.get_list_total();
	cur_prog_pos = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);

    for(i=0 ; i<MULPIC_NUM_9 ;i++)
    {
        if(i == prog_total)
            break;
        GxBus_PmProgGetByPos(g_AppChOps.real_pos(cur_prog_pos) ,1,&gs_Prog[i]);
        gs_MulpicProgNum[i] = cur_prog_pos;
        if(cur_prog_pos == (prog_total-1))
            cur_prog_pos = 0;
        else
            cur_prog_pos++;
    }
	lock_right =0;
	update_cur_program_num_and_name();

	PosY = frame_position/MULPIC_NUM_9_ROW;
	PosX = frame_position%MULPIC_NUM_9_LINE;

	send_msg_play_multiscreen(PosX, PosY);

	if(1 !=  prog_total)
	{
		if(0 != reset_timer(pMultiScreentime))
		{
			pMultiScreentime = create_timer(timermulti, MULPIC_CSAN_DURATION_MS, NULL,  TIMER_REPEAT);
		}
	}

	app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);

	return EVENT_TRANSFER_STOP;
}

int multiscreen_destroy(GuiWidget *widget, void *usrdata)
{
	init_finish = 0;
	int init_value = 0;
	GUI_SetInterface("clear_image", NULL);
	GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
	app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);
	remove_timer(pMultiScreentime);
	pMultiScreentime = NULL;
	s_multiscreen_flag = FALSE;

	return EVENT_TRANSFER_STOP;
}

  int multiscreen_got_focus(GuiWidget *widget, void *usrdata)
{
	if(init_finish == 0)
	{
		reset_timer(pMultiScreentime);
	}
	return EVENT_TRANSFER_STOP;
}

  int multiscreen_lost_focus(GuiWidget *widget, void *usrdata)
{
	if(init_finish == 0)
	{
		timer_stop(pMultiScreentime);
	}
	return EVENT_TRANSFER_STOP;
}
  int multiscreen_keypress(GuiWidget *widget, void *usrdata)
{
	static int8_t PosY  = 0;
	static int8_t PosX  = 0;
	GxMsgProperty_PlayerVideoWindow window_ctrl;
	PlayerWindow rect= {0,0,APP_THEME_XRES,APP_THEME_YRES};
	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;

	if(event->type == GUI_KEYDOWN)
	{
		switch(event->key.sym)
		{
			case STBK_EXIT:
			case STBK_OK:
			case VK_BOOK_TRIGGER:
				timer_stop(pMultiScreentime);
				app_player_close(PLAYER_FOR_NORMAL);

				GUI_EndDialog("wnd_multiscreen");
				GUI_SetProperty("wnd_multiscreen", "draw_now", NULL);

				window_ctrl.rect = rect;
				window_ctrl.player = PLAYER_FOR_NORMAL;
				app_send_msg_exec(GXMSG_PLAYER_VIDEO_WINDOW,(void*)&window_ctrl);

				app_play_current_prog(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);

				return EVENT_TRANSFER_STOP;

			case STBK_UP:
				if(init_finish!=1)
					return EVENT_TRANSFER_STOP;

				timer_stop(pMultiScreentime);
				GUI_SetProperty(gs_MultiscreenImg[frame_position], "state", "hide");
				PosY = frame_position/MULPIC_NUM_9_ROW;
				PosX = frame_position%MULPIC_NUM_9_LINE;
				if((PosY-1) >= 0)
				{
					PosY = (PosY-1);
				}
				else
				{
					PosY = (MULPIC_NUM_9_ROW-1);
				}
				frame_position=PosY*MULPIC_NUM_9_ROW+PosX;
				while(frame_position >= prog_total)
				{
					PosY = PosY-1;
					frame_position = PosY*MULPIC_NUM_9_ROW+PosX;
				}
				//GUI_SetProperty(gs_MultiscreenImg[frame_position], "state", "show");
				update_cur_program_num_and_name();
				send_msg_play_multiscreen(PosX, PosY);

				return EVENT_TRANSFER_STOP;

			case STBK_DOWN:
				if(init_finish!=1)
					return EVENT_TRANSFER_STOP;

				timer_stop(pMultiScreentime);
				GUI_SetProperty(gs_MultiscreenImg[frame_position], "state", "hide");
				PosY = frame_position/MULPIC_NUM_9_ROW;
				PosX = frame_position%MULPIC_NUM_9_LINE;
				if((PosY+1) == MULPIC_NUM_9_ROW)
				{
					PosY = 0;
				}
				else
				{
					PosY = (PosY+1);
				}
				frame_position=PosY*MULPIC_NUM_9_ROW+PosX;
				if(frame_position >= prog_total)
				{
					PosY = 0;
					frame_position=PosY*MULPIC_NUM_9_ROW+PosX;
				}
				//GUI_SetProperty(gs_MultiscreenImg[frame_position], "state", "show");
				update_cur_program_num_and_name();
				send_msg_play_multiscreen(PosX, PosY);

				return EVENT_TRANSFER_STOP;

			case STBK_LEFT:
				if(init_finish!=1)
					return EVENT_TRANSFER_STOP;
				timer_stop(pMultiScreentime);
				GUI_SetProperty(gs_MultiscreenImg[frame_position], "state", "hide");
				PosY = frame_position/MULPIC_NUM_9_ROW;
				PosX = frame_position%MULPIC_NUM_9_LINE;

				//if(app_get_menu_adjust() == 0)
				{
					if((PosX-1) >= 0)
					{
						PosX = (PosX-1);
					}
					else
					{
						PosX = (MULPIC_NUM_9_ROW-1);
					}
					frame_position=PosY*MULPIC_NUM_9_ROW+PosX;
					while(frame_position >= prog_total)
					{
						PosX  = PosX -1;
						frame_position=PosY*MULPIC_NUM_9_ROW+PosX;
					}
				}
                /*
				else
				{
					if(((PosX+1) == MULPIC_NUM_9_ROW))
					{
						PosX = 0;
					}
					else
					{
						PosX = (PosX+1);
					}
					frame_position=(PosY*MULPIC_NUM_9_ROW+PosX);
					if(frame_position >= prog_total)
					{
						PosX = 0;
						frame_position=PosY*MULPIC_NUM_9_ROW+PosX;
					}
				}
                */
				//GUI_SetProperty(gs_MultiscreenImg[frame_position], "state", "show");
				update_cur_program_num_and_name();
				send_msg_play_multiscreen(PosX, PosY);

				return EVENT_TRANSFER_STOP;

			case STBK_RIGHT:
				if(init_finish!=1)
					return EVENT_TRANSFER_STOP;
				timer_stop(pMultiScreentime);
				GUI_SetProperty(gs_MultiscreenImg[frame_position], "state", "hide");
				PosY = frame_position/MULPIC_NUM_9_ROW;
				PosX = frame_position%MULPIC_NUM_9_LINE;

				//if(app_get_menu_adjust() == 0)
				{
					if(((PosX+1) == MULPIC_NUM_9_ROW))
					{
						PosX = 0;
					}
					else
					{
						PosX = (PosX+1);
					}
					frame_position=(PosY*MULPIC_NUM_9_ROW+PosX);
					if(frame_position >= prog_total)
					{
						PosX = 0;
						frame_position=PosY*MULPIC_NUM_9_ROW+PosX;
					}
				}
                /*
				else
				{
					if((PosX-1) >= 0)
					{
						PosX = (PosX-1);
					}
					else
					{
						PosX = (MULPIC_NUM_9_ROW-1);
					}
					frame_position=PosY*MULPIC_NUM_9_ROW+PosX;
					while(frame_position >= prog_total)
					{
						PosX  = PosX -1;
						frame_position=PosY*MULPIC_NUM_9_ROW+PosX;
					}
				}
                */
				//GUI_SetProperty(gs_MultiscreenImg[frame_position], "state", "show");
				update_cur_program_num_and_name();
				send_msg_play_multiscreen(PosX, PosY);

				return EVENT_TRANSFER_STOP;

			case STBK_PAGE_UP:
				if(init_finish!=1)
					return EVENT_TRANSFER_STOP;
				if(prog_total <= MULPIC_NUM_9)
				{
					return EVENT_TRANSFER_STOP;
				}
				if(prog_total < 2*MULPIC_NUM_9)
				{
					cur_prog_pos = gs_MulpicProgNum[MULPIC_NUM_9 -1] +1;
				}
				else
				{
					if(gs_MulpicProgNum[0] == 0)
					{
						cur_prog_pos = prog_total -MULPIC_NUM_9;
					}
					else
					{
						if(gs_MulpicProgNum[0] >= MULPIC_NUM_9)
						{
							cur_prog_pos = gs_MulpicProgNum[0] -MULPIC_NUM_9;
						}
						else
						{
							cur_prog_pos = 0;
						}
					}
				}

				app_multiscreen_clear();
				GUI_SetProperty(gs_MultiscreenImg[frame_position], "state", "hide");
				frame_position = 0;
				PosY = frame_position/MULPIC_NUM_9_ROW;
				PosX = frame_position%MULPIC_NUM_9_LINE;

				page_set_program_data();

				update_cur_program_num_and_name();

				send_msg_play_multiscreen(PosX, PosY);

				if(0 != reset_timer(pMultiScreentime))
				{
					pMultiScreentime = create_timer(timermulti, MULPIC_CSAN_DURATION_MS, NULL,  TIMER_REPEAT);
				}

				return EVENT_TRANSFER_STOP;

			case STBK_PAGE_DOWN:
				if(init_finish!=1)
					return EVENT_TRANSFER_STOP;
				if(prog_total <= MULPIC_NUM_9)
				{
					return EVENT_TRANSFER_STOP;
				}

				if(prog_total < 2*MULPIC_NUM_9)
				{
					cur_prog_pos = gs_MulpicProgNum[MULPIC_NUM_9 -1] +1;
				}
				else
				{
					if(gs_MulpicProgNum[MULPIC_NUM_9 -1] == prog_total -1)
					{
						cur_prog_pos = 0;
					}
					else
					{
						cur_prog_pos = gs_MulpicProgNum[MULPIC_NUM_9 -1] + 1;
					}
				}

				app_multiscreen_clear();
				GUI_SetProperty(gs_MultiscreenImg[frame_position], "state", "hide");
				frame_position = 0;
				PosY = frame_position/MULPIC_NUM_9_ROW;
				PosX = frame_position%MULPIC_NUM_9_LINE;

				page_set_program_data();

				update_cur_program_num_and_name();

				send_msg_play_multiscreen(PosX, PosY);

				if(0 != reset_timer(pMultiScreentime))
				{
					pMultiScreentime = create_timer(timermulti, MULPIC_CSAN_DURATION_MS, NULL,  TIMER_REPEAT);
				}
				return EVENT_TRANSFER_STOP;

			default:
				return EVENT_TRANSFER_STOP;
		}
	}

	return EVENT_TRANSFER_KEEPON;
}




#endif



