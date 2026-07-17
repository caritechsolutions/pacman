
#include "app_config.h"
#include "app_main_menu_theme_config.h"

#if APP_MAIN_MENU_OBJ_SUPPORT

void  gxapp_main_menu_theme_set_config(AppThemeConfig *pConfig)
{
	int theme_type = 0 ;
#if THEME_CLASSIC_DARK
	theme_type = APP_MAIN_MENU_THEME_CLASSIC_DARK ;
#elif THEME_CLASSIC_BLUE
	theme_type = APP_MAIN_MENU_THEME_CLASSIC_BLUE ;
#elif THEME_CLASSIC_PURPLE
	theme_type = APP_MAIN_MENU_THEME_CLASSIC_PURPLE ;
#elif THEME_SKY_BLUE
	theme_type = APP_MAIN_MENU_THEME_SKY_BLUE ;
#elif THEME_GRID_PURPLE
	theme_type = APP_MAIN_MENU_THEME_GRID_PURPLE ;
#else
	theme_type = APP_MAIN_MENU_THEME_CLASSIC_DARK ;
	printf("sys config warning! \n");
#endif

	pConfig->osd_trans = UI_OSD_TRANS ;
	pConfig->gui_trans = UI_GUI_TRANS ;
	pConfig->subt_background = UI_SUBT_BG ;
	pConfig->ui_theme_bpp = UI_THEME_BPP ;

	pConfig->type = theme_type ;
	pConfig->width = APP_XRES ;
	pConfig->height = APP_YRES ;

	pConfig->kb_set_x = WND_KB_SET_POS_X ;
	pConfig->system_setting_y   = WND_SYSTEM_SETTING_POS_Y ;
	pConfig->subset_file_list_x = WND_SUBSET_FILE_LIST_POS_X ;
	pConfig->offset_pop_dlg.x = POP_DLG_POS_X ;
	pConfig->offset_pop_dlg.y = POP_DLG_POS_Y ;
	pConfig->offset_pop_pwd.x = POP_PASSWD_POS_X ;
	pConfig->offset_pop_pwd.y = POP_PASSWD_POS_Y ;
	pConfig->offset_topusb.x = WND_TOP_USB_PRO_POS_X ;
	pConfig->offset_topusb.y = WND_TOP_USB_PRO_POS_Y ;
	pConfig->offset_mm_book.x   = MM_BOOK_POS_X ;
	pConfig->offset_mm_book.y   = MM_BOOK_POS_Y ;
	pConfig->offset_edit_book.x = MM_EDIT_BOOK_X ;
	pConfig->offset_edit_book.y = MM_EDIT_BOOK_Y ;
	pConfig->offset_edit_pop.x = WND_CH_EDIT_POPDLG_POS_X ;
	pConfig->offset_edit_pop.y = WND_CH_EDIT_POPDLG_POS_Y ;
	pConfig->offset_sort_pos.x = WND_SORT_POS_X ;
	pConfig->offset_sort_pos.y = WND_SORT_POS_Y ;
	pConfig->offset_find_kb.x  = WND_FIND_KB_POS_X ;
	pConfig->offset_find_kb.y  = WND_FIND_KB_POS_Y ;
	pConfig->offset_epg_pop.x  = WND_EPG_POP_POS_X ;
	pConfig->offset_epg_pop.y  = WND_EPG_POP_POS_Y ;
	pConfig->offset_vol_pos.x  = WND_VOL_POS_X ;
	pConfig->offset_vol_pos.y  = WND_VOL_POS_Y ;
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
	pConfig->offset_fre_pop.x  = WND_FREQ_POP_LIST_POS_X ;
	pConfig->offset_fre_pop.y  = WND_FREQ_POP_LIST_POS_Y ;
#endif
#if DLNADMR_SUPPORT
	pConfig->offset_dmr_pop.x  = WND_DMR_POP_DLG_X ;
	pConfig->offset_dmr_pop.y  = WND_DMR_POP_DLG_Y ;
#endif
	pConfig->offset_chlist_pwd.x  = WND_CH_LIST_PASSWD_POS_X ;
	pConfig->offset_chlist_pwd.y  = WND_CH_LIST_PASSWD_POS_Y ;
#if (OTA_SUPPORT > 0)
	pConfig->offset_ota_pop.x  = WND_OTA_POP_POS_X ;
	pConfig->offset_ota_pop.y  = WND_OTA_POP_POS_Y ;
#endif
#if TKGS_SUPPORT
	pConfig->offset_tkgs_upgd_pop.x  = WND_TKGS_UPGD_POP_POS_X ;
	pConfig->offset_tkgs_upgd_pop.y  = WND_TKGS_UPGD_POP_POS_Y ;
#endif
	pConfig->offset_kb_in.x  = WND_KB_IN_POS_X ;
	pConfig->offset_kb_in.y  = WND_KB_IN_POS_Y ;
	pConfig->offset_pop_op_list.x  = WND_POP_OP_LIST_POS_X ;
	pConfig->offset_pop_op_list.y  = WND_POP_OP_LIST_POS_Y ;
	pConfig->offset_mm_pop_list.x  = MAINMENU_POP_LIST_X ;
	pConfig->offset_mm_pop_list.y  = MAINMENU_POP_LIST_Y ;
	pConfig->offset_pop_tip.x  = WND_POP_TIP_POS_X ;
	pConfig->offset_pop_tip.y  = WND_POP_TIP_POS_Y ;
	pConfig->offset_file_list.x  = WND_FILE_LIST_POS_X ;
	pConfig->offset_file_list.y  = WND_FILE_LIST_POS_Y ;
	pConfig->offset_netapp_setting_pop.x  = NETAPP_SETTING_POP_LIST_X ;
	pConfig->offset_netapp_setting_pop.y  = NETAPP_SETTING_POP_LIST_Y ;
#if QR_POP_SUPPORT
	pConfig->offset_qr_pop.x  = WND_POP_QR_POS_X ;
	pConfig->offset_qr_pop.y  = WND_POP_QR_POS_Y ;
#endif
	pConfig->offset_net_aspect.x  = WND_NET_ASPECT_POS_X ;
	pConfig->offset_net_aspect.y  = WND_NET_ASPECT_POS_Y ;
}

#endif
