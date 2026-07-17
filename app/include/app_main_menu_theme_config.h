#ifndef APP_MAIN_MENU_THEME_CONFIG_H_20250124
#define APP_MAIN_MENU_THEME_CONFIG_H_20250124

#include "app_config.h"

typedef enum
{
	APP_MAIN_MENU_THEME_CLASSIC_DARK = 0,
	APP_MAIN_MENU_THEME_CLASSIC_BLUE ,
	APP_MAIN_MENU_THEME_CLASSIC_PURPLE ,
	APP_MAIN_MENU_THEME_SKY_BLUE ,
	APP_MAIN_MENU_THEME_GRID_PURPLE ,
	APP_MAIN_MENU_THEME_MAX
}MainMenu_Theme_Type;

typedef struct
{
	unsigned short x ;
	unsigned short y ;
}AppThemeOffset;

typedef struct
{
	unsigned int osd_trans;
	unsigned int gui_trans;
	unsigned int subt_background;
	unsigned int ui_theme_bpp ;
	unsigned short type ;
	unsigned short width ;
	unsigned short height ;
	unsigned short kb_set_x ;
	unsigned short system_setting_y ;
	unsigned short subset_file_list_x ;
	AppThemeOffset offset_pop_dlg ;
	AppThemeOffset offset_pop_pwd ;
	AppThemeOffset offset_topusb ;
	AppThemeOffset offset_mm_book;
	AppThemeOffset offset_edit_book;
	AppThemeOffset offset_edit_pop;
	AppThemeOffset offset_sort_pos;
	AppThemeOffset offset_find_kb;
	AppThemeOffset offset_epg_pop;
	AppThemeOffset offset_vol_pos;
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
	AppThemeOffset offset_fre_pop;
#endif
#if DLNADMR_SUPPORT
	AppThemeOffset offset_dmr_pop;
#endif
	AppThemeOffset offset_chlist_pwd;
#if (OTA_SUPPORT > 0)
	AppThemeOffset offset_ota_pop;
#endif
#if TKGS_SUPPORT
	AppThemeOffset offset_tkgs_upgd_pop;
#endif
	AppThemeOffset offset_kb_in;
	AppThemeOffset offset_pop_op_list;
	AppThemeOffset offset_mm_pop_list;
	AppThemeOffset offset_pop_tip;
	AppThemeOffset offset_file_list;
	AppThemeOffset offset_netapp_setting_pop;
#if QR_POP_SUPPORT
	AppThemeOffset offset_qr_pop;
#endif
	AppThemeOffset offset_net_aspect;
#if FACE_DETECT_SUPPORT
    AppThemeOffset offset_face_detect_prompt;
#endif
}AppThemeConfig;


void  gxapp_main_menu_theme_set_config(AppThemeConfig *pConfig);

#endif
