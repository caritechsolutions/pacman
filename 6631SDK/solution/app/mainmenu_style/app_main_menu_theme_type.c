#include "app_config.h"
#include "app_main_menu_theme_type.h"
#include "app_main_menu_theme_config.h"
#include <stdio.h>
#include <string.h>
#include "module/app_log.h"

#if APP_MAIN_MENU_OBJ_SUPPORT
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdint.h>
#include <gxos/gxcore_os_core.h>
#include "gxcore.h"
#include "app_main_menu_style.h"

#if NETWORK_SUPPORT
#if APPLETS_SUPPORT
#else
#include "app_applet_dll.h"
#endif
#else
#include "../../dll/mainmenu/include/app_mainmenu_sys_dll.h"
#endif

#include "app_mainmenu_dll.h"


static AppThemeConfig s_AppThemeConfig = {0};

static mainmenu_style_void_callback s_func_signal_init = NULL ;
static mainmenu_style_void_callback s_func_media_entry = NULL ;

#if SKIN_SUPPORT
#define APP_MAIN_MENU_OBJ_LOAD_FAIL_TO_SKIN_RECOVER
#endif

#define STORAGE_MAINMENU_LIB_PATH WORK_PATH"theme/gxdll_mainmenu.o"
//#define STORAGE_MAINMENU_LIB_PATH "/mnt/usb01/gxdll_mainmenu.o"

static unsigned int _get_dll_file_size(void)
{
	unsigned int size = 0 ;
	FILE *fp = NULL;
	fp = fopen(STORAGE_MAINMENU_LIB_PATH, "r");
	if ( fp == NULL )
	{
		app_log_error("app file open err \n");
		return 0 ;
	}
	if ( fseek(fp,0,SEEK_SET) != 0 )
	{
		app_log_error("app file set err \n");
		fclose(fp);
		return 0 ;
	}
	if ( fseek(fp,0,SEEK_END) != 0 )
	{
		app_log_error("app file end err \n");
		fclose(fp);
		return 0 ;
	}
	size = ftell(fp);
	fclose(fp );
	return size ;
}

static int _gxapp_main_menu_obj_init(AppThemeConfig *pConfig)
{
	int ret = -1 ;
	void *lib_handle = NULL;
	int (*obj_init)(AppThemeConfig *pConfig);

	handle_t fd ;
	char *dll_file;
	unsigned int size ;
#ifdef ECOS_OS
	GXDL_MEM_ELF mem_elf;
#else
	#define TMP_OBJ_FILE_PATH "tmp/gxdll_mainmenu.o"
#endif

	size = _get_dll_file_size();
	if (size <= 0) {
		app_log_error("get file size error\n");
		return -1;
	}

	fd = GxCore_Open(STORAGE_MAINMENU_LIB_PATH, "r");
	if (fd < 0) {
		app_log_error("open gxobj file error\n");
		return -1;
	}

	dll_file = GxCore_Malloc(size);
	if (dll_file == NULL) {
		app_log_error("malloc %d size fail\n", size);
		GxCore_Close(fd);
		return -1;
	}
	if (GxCore_Read(fd, dll_file, 1, size) != size) {
		app_log_error("read app gxobj file error\n");
		GxCore_Free(dll_file);
		GxCore_Close(fd);
		return -1;
	}
	GxCore_Close(fd);

#ifdef ECOS_OS
	mem_elf.mem = (uint32_t)dll_file;
	mem_elf.size = size;
	lib_handle = GxCore_DlOpen(&mem_elf, GXDL_MEMORY);
#else
	fd = GxCore_Open(TMP_OBJ_FILE_PATH, "w+");
	if (fd < 0) {
		app_log_error("open tmp file error\n");
		GxCore_Free(dll_file);
		return -1;
	}
	if (GxCore_Write(fd, dll_file, 1, size) != size) {
		app_log_error("write file error\n");
		GxCore_Free(dll_file);
		GxCore_Close(fd);
		return -1;
	}
	GxCore_Close(fd);
	lib_handle = GxCore_DlOpen(TMP_OBJ_FILE_PATH, GXDL_FILESYSTEM);
#endif

    if(NULL == lib_handle)
    {
        app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
		GxCore_Free(dll_file);
        return -1;
    }
    obj_init = GxCore_DlSym(lib_handle, "gxobj_api_mainmenu_sys_init");
	if ( NULL == obj_init )
	{
	    app_log_error("app load sys init fail \n");
		app_log_error("err = %s \n",GxCore_DlError());
		GxCore_DlClose(lib_handle);
		GxCore_Free(dll_file);
		lib_handle = NULL;
		return -1 ;
	}

	s_func_signal_init = GxCore_DlSym(lib_handle, "gxobj_api_mainmenu_signal_init");
	if ( NULL == s_func_signal_init )
	{
		app_log_error("app load signal init fail \n");
		app_log_error("err = %s \n",GxCore_DlError());
		GxCore_DlClose(lib_handle);
		GxCore_Free(dll_file);
		lib_handle = NULL;
		return -1 ;
	}

	s_func_media_entry = GxCore_DlSym(lib_handle, "app_mainmenu_style_media_entry");
	if ( NULL == s_func_media_entry )
	{
		app_log_error("app load media entry fail \n");
		app_log_error("err = %s \n",GxCore_DlError());
	}

    ret = obj_init(pConfig);
	if (ret != 0 )
	{
		app_log_error("app obj init fail \n");
		GxCore_DlClose(lib_handle);
		GxCore_Free(dll_file);
		lib_handle = NULL;
		s_func_signal_init = NULL ;
		s_func_media_entry = NULL ;
		return -1 ;
	}

	return ret ;
}

int  gxapp_theme_osd_trans_get(void)
{
	return s_AppThemeConfig.osd_trans ;
}

int  gxapp_theme_gui_trans_get(void)
{
	return s_AppThemeConfig.gui_trans ;
}

int  gxapp_theme_subt_background_get(void)
{
	return s_AppThemeConfig.subt_background ;
}

int  gxapp_theme_ui_bpp_get(void)
{
	return s_AppThemeConfig.ui_theme_bpp ;
}

int  gxapp_theme_offset_x_pop_dlg_get(void)
{
	return s_AppThemeConfig.offset_pop_dlg.x ;
}

int  gxapp_theme_offset_y_pop_dlg_get(void)
{
	return s_AppThemeConfig.offset_pop_dlg.y ;
}

int  gxapp_theme_offset_x_pop_pwd_get(void)
{
	return s_AppThemeConfig.offset_pop_pwd.x ;
}

int  gxapp_theme_offset_y_pop_pwd_get(void)
{
	return s_AppThemeConfig.offset_pop_pwd.y ;
}

int  gxapp_theme_offset_x_usb_get(void)
{
	return s_AppThemeConfig.offset_topusb.x ;
}

int  gxapp_theme_offset_y_usb_get(void)
{
	return s_AppThemeConfig.offset_topusb.y ;
}

int  gxapp_theme_offset_x_mm_book_get(void)
{
	return s_AppThemeConfig.offset_mm_book.x ;
}

int  gxapp_theme_offset_y_mm_book_get(void)
{
	return s_AppThemeConfig.offset_mm_book.y ;
}

int  gxapp_theme_offset_x_edit_book_get(void)
{
	return s_AppThemeConfig.offset_edit_book.x ;
}

int  gxapp_theme_offset_y_edit_book_get(void)
{
	return s_AppThemeConfig.offset_edit_book.y ;
}

int  gxapp_theme_offset_x_edit_pop_get(void)
{
	return s_AppThemeConfig.offset_edit_pop.x ;
}

int  gxapp_theme_offset_y_edit_pop_get(void)
{
	return s_AppThemeConfig.offset_edit_pop.y ;
}

int  gxapp_theme_offset_x_sort_pos_get(void)
{
	return s_AppThemeConfig.offset_sort_pos.x ;
}

int  gxapp_theme_offset_y_sort_pos_get(void)
{
	return s_AppThemeConfig.offset_sort_pos.y ;
}

int  gxapp_theme_offset_x_find_kb_get(void)
{
	return s_AppThemeConfig.offset_find_kb.x ;
}

int  gxapp_theme_offset_y_find_kb_get(void)
{
	return s_AppThemeConfig.offset_find_kb.y ;
}

int  gxapp_theme_offset_x_epg_pop_get(void)
{
	return s_AppThemeConfig.offset_epg_pop.x ;
}

int  gxapp_theme_offset_y_epg_pop_get(void)
{
	return s_AppThemeConfig.offset_epg_pop.y ;
}

int  gxapp_theme_offset_x_vol_pos_get(void)
{
	return s_AppThemeConfig.offset_vol_pos.x ;
}

int  gxapp_theme_offset_y_vol_pos_get(void)
{
	return s_AppThemeConfig.offset_vol_pos.y ;
}

#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
int  gxapp_theme_offset_x_fre_pop_get(void)
{
	return s_AppThemeConfig.offset_fre_pop.x ;
}

int  gxapp_theme_offset_y_fre_pop_get(void)
{
	return s_AppThemeConfig.offset_fre_pop.y ;
}
#endif

#if DLNADMR_SUPPORT
int  gxapp_theme_offset_x_dmr_pop_get(void)
{
	return s_AppThemeConfig.offset_dmr_pop.x ;
}

int  gxapp_theme_offset_y_dmr_pop_get(void)
{
	return s_AppThemeConfig.offset_dmr_pop.y ;
}
#endif

int  gxapp_theme_offset_x_chlist_pwd_get(void)
{
	return s_AppThemeConfig.offset_chlist_pwd.x ;
}

int  gxapp_theme_offset_y_chlist_pwd_get(void)
{
	return s_AppThemeConfig.offset_chlist_pwd.y ;
}

#if (OTA_SUPPORT > 0)
int  gxapp_theme_offset_x_ota_pop_get(void)
{
	return s_AppThemeConfig.offset_ota_pop.x ;
}

int  gxapp_theme_offset_y_ota_pop_get(void)
{
	return s_AppThemeConfig.offset_ota_pop.y ;
}
#endif

#if TKGS_SUPPORT
int  gxapp_theme_offset_x_tkgs_upgd_pop_get(void)
{
	return s_AppThemeConfig.offset_tkgs_upgd_pop.x ;
}

int  gxapp_theme_offset_y_tkgs_upgd_pop_get(void)
{
	return s_AppThemeConfig.offset_tkgs_upgd_pop.y ;
}
#endif

int  gxapp_theme_offset_x_kb_set_get(void)
{
	return s_AppThemeConfig.kb_set_x ;
}

int  gxapp_theme_offset_x_kb_in_get(void)
{
	return s_AppThemeConfig.offset_kb_in.x ;
}

int  gxapp_theme_offset_y_kb_in_get(void)
{
	return s_AppThemeConfig.offset_kb_in.y ;
}

int  gxapp_theme_offset_x_pop_op_list_get(void)
{
	return s_AppThemeConfig.offset_pop_op_list.x ;
}

int  gxapp_theme_offset_y_pop_op_list_get(void)
{
	return s_AppThemeConfig.offset_pop_op_list.y ;
}

int  gxapp_theme_offset_x_mm_pop_list_get(void)
{
	return s_AppThemeConfig.offset_mm_pop_list.x ;
}

int  gxapp_theme_offset_y_mm_pop_list_get(void)
{
	return s_AppThemeConfig.offset_mm_pop_list.y ;
}

int  gxapp_theme_offset_x_pop_tip_get(void)
{
	return s_AppThemeConfig.offset_pop_tip.x ;
}

int  gxapp_theme_offset_y_pop_tip_get(void)
{
	return s_AppThemeConfig.offset_pop_tip.y ;
}

int  gxapp_theme_offset_x_file_list_get(void)
{
	return s_AppThemeConfig.offset_file_list.x ;
}

int  gxapp_theme_offset_y_file_list_get(void)
{
	return s_AppThemeConfig.offset_file_list.y ;
}

int  gxapp_theme_offset_x_netapp_setting_pop_get(void)
{
	return s_AppThemeConfig.offset_netapp_setting_pop.x ;
}

int  gxapp_theme_offset_y_netapp_setting_pop_get(void)
{
	return s_AppThemeConfig.offset_netapp_setting_pop.y ;
}

int  gxapp_theme_offset_y_system_setting_get(void)
{
	return s_AppThemeConfig.system_setting_y ;
}

int  gxapp_theme_offset_x_subset_file_list_get(void)
{
	return s_AppThemeConfig.subset_file_list_x ;
}

#if QR_POP_SUPPORT
int  gxapp_theme_offset_x_qr_pop_get(void)
{
	return s_AppThemeConfig.offset_qr_pop.x ;
}

int  gxapp_theme_offset_y_qr_pop_get(void)
{
	return s_AppThemeConfig.offset_qr_pop.y ;
}
#endif

int  gxapp_theme_offset_x_net_aspect_get(void)
{
	return s_AppThemeConfig.offset_net_aspect.x ;
}

int  gxapp_theme_offset_y_net_aspect_get(void)
{
	return s_AppThemeConfig.offset_net_aspect.y ;
}

int  gxapp_main_menu_xres_get(void)
{
	return s_AppThemeConfig.width ;
}

int  gxapp_main_menu_yres_get(void)
{
	return s_AppThemeConfig.height ;
}
#if FACE_DETECT_SUPPORT
int gxapp_theme_offset_x_face_detect_prompt_get(void)
{
    return s_AppThemeConfig.offset_face_detect_prompt.x;
}

int gxapp_theme_offset_y_face_detect_prompt_get(void)
{
    return s_AppThemeConfig.offset_face_detect_prompt.y;
}
#endif
#endif

int  gxapp_main_menu_is_classic_dark(void)
{
	return (APP_MAIN_MENU_THEME_CLASSIC_DARK == gxapp_main_menu_theme_type_get())? 1: 0;
}

int  gxapp_main_menu_is_classic_blue(void)
{
	return (APP_MAIN_MENU_THEME_CLASSIC_BLUE == gxapp_main_menu_theme_type_get())? 1: 0;
}

int  gxapp_main_menu_is_classic_purple(void)
{
	return (APP_MAIN_MENU_THEME_CLASSIC_PURPLE == gxapp_main_menu_theme_type_get())? 1: 0;
}

int  gxapp_main_menu_is_sky_blue(void)
{
	return (APP_MAIN_MENU_THEME_SKY_BLUE == gxapp_main_menu_theme_type_get())? 1: 0;
}

int  gxapp_main_menu_is_grid_purple(void)
{
	return (APP_MAIN_MENU_THEME_GRID_PURPLE == gxapp_main_menu_theme_type_get())? 1: 0;
}

int  gxapp_main_menu_is_not_classic_dark(void)
{
	return (APP_MAIN_MENU_THEME_CLASSIC_DARK != gxapp_main_menu_theme_type_get())? 1: 0;
}

int gxapp_main_menu_theme_type_get(void)
{
#if APP_MAIN_MENU_OBJ_SUPPORT
	return s_AppThemeConfig.type ;
#else
#if THEME_CLASSIC_DARK
	return APP_MAIN_MENU_THEME_CLASSIC_DARK ;
#elif THEME_CLASSIC_BLUE
	return APP_MAIN_MENU_THEME_CLASSIC_BLUE ;
#elif THEME_CLASSIC_PURPLE
	return APP_MAIN_MENU_THEME_CLASSIC_PURPLE ;
#elif THEME_SKY_BLUE
	return APP_MAIN_MENU_THEME_SKY_BLUE ;
#elif THEME_GRID_PURPLE
	return APP_MAIN_MENU_THEME_GRID_PURPLE ;
#else
	return APP_MAIN_MENU_THEME_CLASSIC_DARK ;
#endif
#endif
}

int  gxapp_main_menu_theme_signal_init(void)
{
#if APP_MAIN_MENU_OBJ_SUPPORT
	if ( NULL != s_func_signal_init ){
		s_func_signal_init();
	}
	else{
		app_log_error("app obj func signal init is empty \n");
	}
#endif
	return 0;
}

int  gxapp_main_menu_theme_media_entry(void)
{
#if APP_MAIN_MENU_OBJ_SUPPORT
	if ( NULL != s_func_media_entry ){
		s_func_media_entry();
	}
	else{
		app_log_error("app obj func media entry is empty \n");
	}
#else
	extern void app_main_menu_media_item_create(void);
    app_main_menu_media_item_create();
#endif
	return 0;
}

static void _gxapp_main_menu_theme_elf_compile_info(void)
{
#if APP_MAIN_MENU_OBJ_SUPPORT
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
#endif
	app_log_error("app compile elf theme type = %d \n",theme_type);
#endif
}

static void _gxapp_main_menu_theme_check_config(void)
{
#if APP_MAIN_MENU_OBJ_SUPPORT

	app_log_error("app theme type = %d \n",s_AppThemeConfig.type);

	if (s_AppThemeConfig.width <=0 )
		s_AppThemeConfig.width = 1024 ;

	app_log_error("app theme width = %d \n",s_AppThemeConfig.width);

	if (s_AppThemeConfig.height <=0 )
		s_AppThemeConfig.height = 576 ;

	app_log_error("app theme width = %d \n",s_AppThemeConfig.height);

#endif
}

int gxapp_main_menu_theme_obj_init(void)
{
	int ret = 0 ;
	_gxapp_main_menu_theme_elf_compile_info();
#if APP_MAIN_MENU_OBJ_SUPPORT
	ret = _gxapp_main_menu_obj_init(&s_AppThemeConfig);
	if ( ret != 0 )
	{
		app_log_error("#### main menu obj init fail \n");
#if SKIN_SUPPORT
#ifdef APP_MAIN_MENU_OBJ_LOAD_FAIL_TO_SKIN_RECOVER
		extern int  gxapp_skin_flash_get_update_error_flag(void);
		extern int  gxapp_skin_flash_set_update_error_flag(int theme_id);
		extern int  gxapp_skin_flash_unmount(char *mount_path);
		if ( gxapp_skin_flash_get_update_error_flag() <= 0 )
		{
			app_log_error("app skin to set err flag \n");
			gxapp_skin_flash_unmount("/dvb");
			gxapp_skin_flash_set_update_error_flag(0);
		}
#endif
#endif
		app_log_error("app set default config \n");
		gxapp_main_menu_theme_set_config(&s_AppThemeConfig);
	}
#endif

	_gxapp_main_menu_theme_check_config();

    //other module init
	extern void app_play_control_module_sys_init(void);
	app_play_control_module_sys_init();

	extern void app_zoom_module_sys_init(void);
	app_zoom_module_sys_init();

	return ret ;
}
