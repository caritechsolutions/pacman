#ifndef APP_MAIN_MENU_THEME_TYPE_H_20250117
#define APP_MAIN_MENU_THEME_TYPE_H_20250117

#include "app_config.h"

#if APP_MAIN_MENU_OBJ_SUPPORT

int  gxapp_theme_osd_trans_get(void);
int  gxapp_theme_gui_trans_get(void);
int  gxapp_theme_subt_background_get(void);
int  gxapp_theme_ui_bpp_get(void);

int  gxapp_theme_offset_x_pop_dlg_get(void);
int  gxapp_theme_offset_y_pop_dlg_get(void);
int  gxapp_theme_offset_x_pop_pwd_get(void);
int  gxapp_theme_offset_y_pop_pwd_get(void);
int  gxapp_theme_offset_x_usb_get(void);
int  gxapp_theme_offset_y_usb_get(void);
int  gxapp_theme_offset_x_mm_book_get(void);
int  gxapp_theme_offset_y_mm_book_get(void);
int  gxapp_theme_offset_x_edit_book_get(void);
int  gxapp_theme_offset_y_edit_book_get(void);
int  gxapp_theme_offset_x_edit_pop_get(void);
int  gxapp_theme_offset_y_edit_pop_get(void);
int  gxapp_theme_offset_x_sort_pos_get(void);
int  gxapp_theme_offset_y_sort_pos_get(void);
int  gxapp_theme_offset_x_find_kb_get(void);
int  gxapp_theme_offset_y_find_kb_get(void);
int  gxapp_theme_offset_x_epg_pop_get(void);
int  gxapp_theme_offset_y_epg_pop_get(void);
int  gxapp_theme_offset_x_vol_pos_get(void);
int  gxapp_theme_offset_y_vol_pos_get(void);
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
int  gxapp_theme_offset_x_fre_pop_get(void);
int  gxapp_theme_offset_y_fre_pop_get(void);
#endif
#if DLNADMR_SUPPORT
int  gxapp_theme_offset_x_dmr_pop_get(void);
int  gxapp_theme_offset_y_dmr_pop_get(void);
#endif
int  gxapp_theme_offset_x_chlist_pwd_get(void);
int  gxapp_theme_offset_y_chlist_pwd_get(void);
#if (OTA_SUPPORT > 0)
int  gxapp_theme_offset_x_ota_pop_get(void);
int  gxapp_theme_offset_y_ota_pop_get(void);
#endif
#if TKGS_SUPPORT
int  gxapp_theme_offset_x_tkgs_upgd_pop_get(void);
int  gxapp_theme_offset_y_tkgs_upgd_pop_get(void);
#endif
int  gxapp_theme_offset_x_kb_set_get(void);
int  gxapp_theme_offset_x_kb_in_get(void);
int  gxapp_theme_offset_y_kb_in_get(void);
int  gxapp_theme_offset_x_pop_op_list_get(void);
int  gxapp_theme_offset_y_pop_op_list_get(void);
int  gxapp_theme_offset_x_mm_pop_list_get(void);
int  gxapp_theme_offset_y_mm_pop_list_get(void);
int  gxapp_theme_offset_x_pop_tip_get(void);
int  gxapp_theme_offset_y_pop_tip_get(void);
int  gxapp_theme_offset_x_file_list_get(void);
int  gxapp_theme_offset_y_file_list_get(void);
int  gxapp_theme_offset_x_netapp_setting_pop_get(void);
int  gxapp_theme_offset_y_netapp_setting_pop_get(void);

int  gxapp_theme_offset_y_system_setting_get(void);
int  gxapp_theme_offset_x_subset_file_list_get(void);

#if QR_POP_SUPPORT
int  gxapp_theme_offset_x_qr_pop_get(void);
int  gxapp_theme_offset_y_qr_pop_get(void);
#endif
int  gxapp_theme_offset_x_net_aspect_get(void);
int  gxapp_theme_offset_y_net_aspect_get(void);

#if FACE_DETECT_SUPPORT
int gxapp_theme_offset_x_face_detect_prompt_get(void);
int gxapp_theme_offset_y_face_detect_prompt_get(void);
#endif

int  gxapp_main_menu_xres_get(void);
int  gxapp_main_menu_yres_get(void);

#define APP_UI_OSD_TRANS  gxapp_theme_osd_trans_get()
#define APP_UI_GUI_TRANS  gxapp_theme_gui_trans_get()
#define APP_UI_SUBT_BG    gxapp_theme_subt_background_get()
#define APP_UI_THEME_BPP  gxapp_theme_ui_bpp_get()

#define APP_THEME_XRES  gxapp_main_menu_xres_get()
#define APP_THEME_YRES  gxapp_main_menu_yres_get()

#define APP_POP_DLG_POS_X  gxapp_theme_offset_x_pop_dlg_get()
#define APP_POP_DLG_POS_Y  gxapp_theme_offset_y_pop_dlg_get()
#define APP_POP_PASSWD_POS_X  gxapp_theme_offset_x_pop_pwd_get()
#define APP_POP_PASSWD_POS_Y  gxapp_theme_offset_y_pop_pwd_get()
#define APP_WND_TOP_USB_PRO_POS_X  gxapp_theme_offset_x_usb_get()
#define APP_WND_TOP_USB_PRO_POS_Y  gxapp_theme_offset_y_usb_get()
#define APP_MM_BOOK_POS_X    gxapp_theme_offset_x_mm_book_get()
#define APP_MM_BOOK_POS_Y    gxapp_theme_offset_y_mm_book_get()
#define APP_MM_EDIT_BOOK_X   gxapp_theme_offset_x_edit_book_get()
#define APP_MM_EDIT_BOOK_Y   gxapp_theme_offset_y_edit_book_get()
#define APP_WND_CH_EDIT_POPDLG_POS_X  gxapp_theme_offset_x_edit_pop_get()
#define APP_WND_CH_EDIT_POPDLG_POS_Y  gxapp_theme_offset_y_edit_pop_get()
#define APP_WND_SORT_POS_X   gxapp_theme_offset_x_sort_pos_get()
#define APP_WND_SORT_POS_Y   gxapp_theme_offset_y_sort_pos_get()
#define APP_WND_FIND_KB_POS_X  gxapp_theme_offset_x_find_kb_get()
#define APP_WND_FIND_KB_POS_Y  gxapp_theme_offset_y_find_kb_get()
#define APP_WND_EPG_POP_POS_X  gxapp_theme_offset_x_epg_pop_get()
#define APP_WND_EPG_POP_POS_Y  gxapp_theme_offset_y_epg_pop_get()
#define APP_WND_VOL_POS_X  gxapp_theme_offset_x_vol_pos_get()
#define APP_WND_VOL_POS_Y  gxapp_theme_offset_y_vol_pos_get()
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
#define APP_WND_FREQ_POP_LIST_POS_X  gxapp_theme_offset_x_fre_pop_get()
#define APP_WND_FREQ_POP_LIST_POS_Y  gxapp_theme_offset_y_fre_pop_get()
#endif
#if DLNADMR_SUPPORT
#define APP_WND_DMR_POP_DLG_X  gxapp_theme_offset_x_dmr_pop_get()
#define APP_WND_DMR_POP_DLG_Y  gxapp_theme_offset_y_dmr_pop_get()
#endif
#define APP_WND_CH_LIST_PASSWD_POS_X  gxapp_theme_offset_x_chlist_pwd_get()
#define APP_WND_CH_LIST_PASSWD_POS_Y  gxapp_theme_offset_y_chlist_pwd_get()
#if (OTA_SUPPORT > 0)
#define APP_WND_OTA_POP_POS_X  gxapp_theme_offset_x_ota_pop_get()
#define APP_WND_OTA_POP_POS_Y  gxapp_theme_offset_y_ota_pop_get()
#endif
#if TKGS_SUPPORT
#define APP_WND_TKGS_UPGD_POP_POS_X  gxapp_theme_offset_x_tkgs_upgd_pop_get()
#define APP_WND_TKGS_UPGD_POP_POS_Y  gxapp_theme_offset_y_tkgs_upgd_pop_get()
#endif
#define APP_WND_KB_SET_POS_X  gxapp_theme_offset_x_kb_set_get()
#define APP_WND_KB_IN_POS_X   gxapp_theme_offset_x_kb_in_get()
#define APP_WND_KB_IN_POS_Y   gxapp_theme_offset_y_kb_in_get()
#define APP_WND_POP_OP_LIST_POS_X  gxapp_theme_offset_x_pop_op_list_get()
#define APP_WND_POP_OP_LIST_POS_Y  gxapp_theme_offset_y_pop_op_list_get()
#define APP_MAINMENU_POP_LIST_X  gxapp_theme_offset_x_mm_pop_list_get()
#define APP_MAINMENU_POP_LIST_Y  gxapp_theme_offset_y_mm_pop_list_get()
#define APP_WND_POP_TIP_POS_X  gxapp_theme_offset_x_pop_tip_get()
#define APP_WND_POP_TIP_POS_Y  gxapp_theme_offset_y_pop_tip_get()
#define APP_WND_FILE_LIST_POS_X  gxapp_theme_offset_x_file_list_get()
#define APP_WND_FILE_LIST_POS_Y  gxapp_theme_offset_y_file_list_get()
#define APP_NETAPP_SETTING_POP_LIST_X  gxapp_theme_offset_x_netapp_setting_pop_get()
#define APP_NETAPP_SETTING_POP_LIST_Y  gxapp_theme_offset_y_netapp_setting_pop_get()

#define APP_WND_SYSTEM_SETTING_POS_Y  gxapp_theme_offset_y_system_setting_get()
#define APP_WND_SUBSET_FILE_LIST_POS_X  gxapp_theme_offset_x_subset_file_list_get()

#if QR_POP_SUPPORT
#define APP_WND_POP_QR_POS_X  gxapp_theme_offset_x_qr_pop_get()
#define APP_WND_POP_QR_POS_Y  gxapp_theme_offset_y_qr_pop_get()
#endif
#define APP_WND_NET_ASPECT_POS_X  gxapp_theme_offset_x_net_aspect_get()
#define APP_WND_NET_ASPECT_POS_Y  gxapp_theme_offset_y_net_aspect_get()
#if FACE_DETECT_SUPPORT
#define APP_WND_FACE_PROMPT_POS_X gxapp_theme_offset_x_face_detect_prompt_get()
#define APP_WND_FACE_PROMPT_POS_Y gxapp_theme_offset_y_face_detect_prompt_get()
#endif
#else

#define APP_UI_OSD_TRANS  UI_OSD_TRANS
#define APP_UI_GUI_TRANS  UI_GUI_TRANS
#define APP_UI_SUBT_BG    UI_SUBT_BG
#define APP_UI_THEME_BPP  UI_THEME_BPP

#define APP_THEME_XRES  APP_XRES
#define APP_THEME_YRES  APP_YRES

#define APP_POP_DLG_POS_X  POP_DLG_POS_X
#define APP_POP_DLG_POS_Y  POP_DLG_POS_Y
#define APP_POP_PASSWD_POS_X  POP_PASSWD_POS_X
#define APP_POP_PASSWD_POS_Y  POP_PASSWD_POS_Y
#define APP_WND_TOP_USB_PRO_POS_X  WND_TOP_USB_PRO_POS_X
#define APP_WND_TOP_USB_PRO_POS_Y  WND_TOP_USB_PRO_POS_Y
#define APP_MM_BOOK_POS_X    MM_BOOK_POS_X
#define APP_MM_BOOK_POS_Y    MM_BOOK_POS_Y
#define APP_MM_EDIT_BOOK_X   MM_EDIT_BOOK_X
#define APP_MM_EDIT_BOOK_Y   MM_EDIT_BOOK_Y
#define APP_WND_CH_EDIT_POPDLG_POS_X  WND_CH_EDIT_POPDLG_POS_X
#define APP_WND_CH_EDIT_POPDLG_POS_Y  WND_CH_EDIT_POPDLG_POS_Y
#define APP_WND_SORT_POS_X   WND_SORT_POS_X
#define APP_WND_SORT_POS_Y   WND_SORT_POS_Y
#define APP_WND_FIND_KB_POS_X  WND_FIND_KB_POS_X
#define APP_WND_FIND_KB_POS_Y  WND_FIND_KB_POS_Y
#define APP_WND_EPG_POP_POS_X  WND_EPG_POP_POS_X
#define APP_WND_EPG_POP_POS_Y  WND_EPG_POP_POS_Y
#define APP_WND_VOL_POS_X  WND_VOL_POS_X
#define APP_WND_VOL_POS_Y  WND_VOL_POS_Y
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
#define APP_WND_FREQ_POP_LIST_POS_X  WND_FREQ_POP_LIST_POS_X
#define APP_WND_FREQ_POP_LIST_POS_Y  WND_FREQ_POP_LIST_POS_Y
#endif
#if DLNADMR_SUPPORT
#define APP_WND_DMR_POP_DLG_X  WND_DMR_POP_DLG_X
#define APP_WND_DMR_POP_DLG_Y  WND_DMR_POP_DLG_Y
#endif
#define APP_WND_CH_LIST_PASSWD_POS_X  WND_CH_LIST_PASSWD_POS_X
#define APP_WND_CH_LIST_PASSWD_POS_Y  WND_CH_LIST_PASSWD_POS_Y
#if (OTA_SUPPORT > 0)
#define APP_WND_OTA_POP_POS_X  WND_OTA_POP_POS_X
#define APP_WND_OTA_POP_POS_Y  WND_OTA_POP_POS_Y
#endif
#if TKGS_SUPPORT
#define APP_WND_TKGS_UPGD_POP_POS_X  WND_TKGS_UPGD_POP_POS_X
#define APP_WND_TKGS_UPGD_POP_POS_Y  WND_TKGS_UPGD_POP_POS_Y
#endif
#define APP_WND_KB_SET_POS_X  WND_KB_SET_POS_X
#define APP_WND_KB_IN_POS_X   WND_KB_IN_POS_X
#define APP_WND_KB_IN_POS_Y   WND_KB_IN_POS_Y
#define APP_WND_POP_OP_LIST_POS_X  WND_POP_OP_LIST_POS_X
#define APP_WND_POP_OP_LIST_POS_Y  WND_POP_OP_LIST_POS_Y
#define APP_MAINMENU_POP_LIST_X  MAINMENU_POP_LIST_X
#define APP_MAINMENU_POP_LIST_Y  MAINMENU_POP_LIST_Y
#define APP_WND_POP_TIP_POS_X  WND_POP_TIP_POS_X
#define APP_WND_POP_TIP_POS_Y  WND_POP_TIP_POS_Y
#define APP_WND_FILE_LIST_POS_X  WND_FILE_LIST_POS_X
#define APP_WND_FILE_LIST_POS_Y  WND_FILE_LIST_POS_Y
#define APP_NETAPP_SETTING_POP_LIST_X  NETAPP_SETTING_POP_LIST_X
#define APP_NETAPP_SETTING_POP_LIST_Y  NETAPP_SETTING_POP_LIST_Y

#define APP_WND_SYSTEM_SETTING_POS_Y  WND_SYSTEM_SETTING_POS_Y
#define APP_WND_SUBSET_FILE_LIST_POS_X  WND_SUBSET_FILE_LIST_POS_X

#if QR_POP_SUPPORT
#define APP_WND_POP_QR_POS_X  WND_POP_QR_POS_X
#define APP_WND_POP_QR_POS_Y  WND_POP_QR_POS_Y
#endif
#define APP_WND_NET_ASPECT_POS_X  WND_NET_ASPECT_POS_X
#define APP_WND_NET_ASPECT_POS_Y  WND_NET_ASPECT_POS_Y

#if FACE_DETECT_SUPPORT
#define APP_WND_FACE_PROMPT_POS_X  WND_FACE_PROMPT_POS_X
#define APP_WND_FACE_PROMPT_POS_Y  WND_FACE_PROMPT_POS_Y
#endif

#endif

int  gxapp_main_menu_theme_type_get(void);

int  gxapp_main_menu_theme_obj_init(void);
int  gxapp_main_menu_theme_signal_init(void);
int  gxapp_main_menu_theme_media_entry(void);

int  gxapp_main_menu_is_classic_dark(void);
int  gxapp_main_menu_is_classic_blue(void);
int  gxapp_main_menu_is_classic_purple(void);
int  gxapp_main_menu_is_sky_blue(void);
int  gxapp_main_menu_is_grid_purple(void);
int  gxapp_main_menu_is_not_classic_dark(void);

#define APP_THEME_CLASSIC_DARK         (gxapp_main_menu_is_classic_dark())
#define APP_THEME_CLASSIC_BLUE         (gxapp_main_menu_is_classic_blue())
#define APP_THEME_CLASSIC_PURPLE       (gxapp_main_menu_is_classic_purple())
#define APP_THEME_SKY_BLUE             (gxapp_main_menu_is_sky_blue())
#define APP_THEME_GRID_PURPLE          (gxapp_main_menu_is_grid_purple())
#define APP_IS_NOT_THEME_CLASSIC_DARK  (gxapp_main_menu_is_not_classic_dark())

#endif
