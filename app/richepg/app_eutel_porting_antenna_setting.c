/*
    20240722
*/

#include "app_config.h"

#if RICHEPG_SUPPORT

#include "app.h"
#include "app_eutel_porting.h"
#include "app_eutel_database.h"
#include "app_eutel_sat_fuse.h"
#include "app_fuse_ops.h"
#include "app_windows.h"

/*
--------- antenna setting ops
*/

/* 这个定义需要和app_antenna_setting 定义的对应 */
#define EUTEL_PORTING_ANTENNA_SEARCH_MODE_SAT    (0)
#define EUTEL_PORTING_ANTENNA_SEARCH_MODE_BLIND  (1)
#define EUTEL_PORTING_ANTENNA_SEARCH_MODE_TP     (2)

#define EUTEL_PORTING_ANTENNA_SEARCH_STR  "Sat.tv"

extern void app_antenna_setting_btn_draw(int show_hide);
extern void app_antenna_setting_combox_search_content_update(char *buffer);
extern void app_antenna_setting_update_data_by_combox_search_change(int update_cmb);
extern int  app_antenan_setting_start_search(int search_mode);
extern void app_antenna_setting_set_ops(dvbs_antenna_setting_ops *ops);

extern void app_eutel_ant_menu_exec(void);
extern bool app_eutel_certification_pop_check(unsigned key);
extern void app_eutel_search_create_menu(int sat_id,int tp_id);

static int s_app_eutel_sat_search_mode = 0 ;

/****  ****/
#define EUTEL_ANTENNA_INSTALL_KEY_TOTAL     6
static unsigned int s_eutel_antenan_install_key_array[] = {STBK_EPG, STBK_INFO, STBK_8, STBK_3, STBK_5, STBK_7};
static int s_eutel_antenan_install_key_cnt = 0;

static bool _eutel_antenan_install_check(unsigned int key)
{
    if (key != s_eutel_antenan_install_key_array[s_eutel_antenan_install_key_cnt])
    {
        s_eutel_antenan_install_key_cnt = 0;
        return false;
    }
    if (++s_eutel_antenan_install_key_cnt == EUTEL_ANTENNA_INSTALL_KEY_TOTAL)
    {
        s_eutel_antenan_install_key_cnt = 0;
        return true;
    }
    return false;
}

int  app_eutel_porting_set_sat_search_mode(int mode)
{
	s_app_eutel_sat_search_mode = mode ;
	return 0 ;
}

int  app_eutel_porting_get_sat_search_mode(void)
{
	return s_app_eutel_sat_search_mode ;
}

int app_eutel_porting_get_sat_type(unsigned int sat_id)
{
    if(!app_eutel_sat_type_check(sat_id))
    {
        if ( app_eutel_sat_fuse_free_is_point(sat_id) )
        {
			return APP_SAT_TYPE_IS_FUSE ;
        }
		else
		{
			return APP_SAT_TYPE_IS_FREE ;
		}
    }
    else
    {
        return APP_SAT_TYPE_IS_EUTEL ;
    }
}

/*
-------------- ops ----------------------
*/

static void _eutel_antenna_setting_combox_search_update(int sat_id,char *pcontent)
{
	int type = 0 ;
	char *p1 = NULL ;
	char buffer_all[128] ;

	if (pcontent == NULL )
		return ;

	memset(buffer_all,0, 128);
	
	type = app_eutel_porting_get_sat_type(sat_id);

	if ( type == APP_SAT_TYPE_IS_FREE)
	{
		app_antenna_setting_combox_search_content_update(pcontent);
	}
	else
	{
		strcat(buffer_all, "[");

		if ( type == APP_SAT_TYPE_IS_EUTEL)
		{
			strcat(buffer_all, EUTEL_PORTING_ANTENNA_SEARCH_STR);
			strcat(buffer_all, "]");
		}
		else
		{
			strcat(buffer_all, EUTEL_PORTING_ANTENNA_SEARCH_STR);
			strcat(buffer_all, ",");

			p1 = pcontent + 1;
			strcat(buffer_all, p1);
		}
		app_antenna_setting_combox_search_content_update(buffer_all);
	}

	if ( app_eutel_porting_get_sat_search_mode())
		app_antenna_setting_btn_draw(0);
	else
		app_antenna_setting_btn_draw(1);

	return;
}

static void _eutel_antenna_setting_combox_search_change(int sat_id,char *pins)
{
	int update_cmb = 0 ;
	int type = 0 ;
	type = app_eutel_porting_get_sat_type(sat_id);

	if ((type == APP_SAT_TYPE_IS_FREE) || (type == APP_SAT_TYPE_IS_EUTEL))
	{
		return ;
	}
	else if (type == APP_SAT_TYPE_IS_FUSE)
	{
		if ( pins == NULL )
			return ;
		if ( 0 != strcmp(pins,EUTEL_PORTING_ANTENNA_SEARCH_STR) )
		{
			if ( app_eutel_porting_get_sat_search_mode() )
			{
				update_cmb = 1;
				app_eutel_porting_set_sat_search_mode(0);
			}
			else
			{
				return ;
			}
		}
		else
		{
			app_eutel_porting_set_sat_search_mode(1);
		}
	}

	app_antenna_setting_update_data_by_combox_search_change(update_cmb);

	if ( app_eutel_porting_get_sat_search_mode())
		app_antenna_setting_btn_draw(0);
	else
		app_antenna_setting_btn_draw(1);
}

static bool _eutel_antenan_key_check(unsigned int key)
{
	if (_eutel_antenan_install_check(key))
	{
		GUI_EndDialog(WND_ANTENNA_SETTING);
		app_eutel_ant_menu_exec();
		return true;
	}

	if (app_eutel_certification_pop_check(key))
	{
		return true;
	}
	return false ;
}

static int _eutel_antenan_setting_key_check(unsigned int key)
{
	return _eutel_antenan_key_check(key);
}

static int _eutel_antenan_setting_key_press(void)
{
	if ( app_eutel_porting_get_sat_search_mode() )
		return 1;
	else
		return 0;
}

static void _eutel_antenan_setting_uninit(void)
{
	app_eutel_porting_set_sat_search_mode(0);
}

static void _eutel_antenan_setting_save_sat_params(int sat_id,GxBusPmDataSat *p_sat)
{
	int count = 0;
	int i = 0;
	unsigned int area[EUTUL_SAT_MAX_NUM] ;

	count = app_eutel_sat_fuse_get_all_area_count(sat_id,area);

	for(i=0;i<count;i++)
	{
		app_eutel_sat_save_params(area[i],p_sat);
	}
}

static int _eutel_antenan_setting_start_search(int sat_id,int tp_id,char *psearch)
{
	int search_mode;

	if ( psearch == NULL )
       return 0 ;

	if ( 0 == strcmp(psearch,EUTEL_PORTING_ANTENNA_SEARCH_STR) )
	{
		app_eutel_search_create_menu(sat_id,tp_id);
	}
	else
	{
		if ( 0 == strcmp(psearch,"Super Satellite") || 0 == strcmp(psearch,"Satellite") )
			search_mode = EUTEL_PORTING_ANTENNA_SEARCH_MODE_SAT ;
		else if ( 0 == strcmp(psearch,"Super Blind") || 0 == strcmp(psearch,"Blind") )
			search_mode = EUTEL_PORTING_ANTENNA_SEARCH_MODE_BLIND ;
		else if ( 0 == strcmp(psearch,"Super Tp") || 0 == strcmp(psearch,"TP") )
			search_mode = EUTEL_PORTING_ANTENNA_SEARCH_MODE_TP ;
		else
			return -1 ;

		app_antenan_setting_start_search(search_mode);
	}
	return 0;
}

static void _eutel_antenan_get_focus(char *psearch)
{
	if ( psearch != NULL )
	{
		if ( 0 == strcmp(psearch,EUTEL_PORTING_ANTENNA_SEARCH_STR) )
		{
			return ;
		}
	}
	app_antenna_setting_btn_draw(1);
}

int  app_eutel_antenan_setting_ops_init(void)
{
	dvbs_antenna_setting_ops ops ;

	ops.combox_search_update = _eutel_antenna_setting_combox_search_update;
	ops.combox_search_change = _eutel_antenna_setting_combox_search_change;
	ops.start_search    = _eutel_antenan_setting_start_search;
	ops.key_check      = _eutel_antenan_setting_key_check;
	ops.key_func_press = _eutel_antenan_setting_key_press;
	ops.save_sat_params = _eutel_antenan_setting_save_sat_params;
	ops.uninit = _eutel_antenan_setting_uninit;
	ops.get_focus = _eutel_antenan_get_focus;

	app_antenna_setting_set_ops(&ops);

	return 0 ;
}

#endif

