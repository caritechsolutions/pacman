#ifndef __APP_FUSE_OPS_H__
#define __APP_FUSE_OPS_H__

typedef struct{
	void (*combox_search_update)(int sat_id,char *pcontent);
	void (*combox_search_change)(int sat_id,char *psearch);
	int  (*start_search)(int sat_id,int tp_id,char *psearch);
	int (*key_check)(unsigned int key);
	int (*key_func_press)(void);
	void (*save_sat_params)(int sat_id,GxBusPmDataSat *p_sat);
	void (*uninit)(void);
	void (*get_focus)(char *psearch);
} dvbs_antenna_setting_ops;

typedef struct
{
	int    (*get_list_total)(void);
	int    (*get_next_pos)(unsigned int);
	int    (*get_prev_pos)(unsigned int);
	int    (*get_cur_pos)(unsigned int);
	int    (*real_pos)(unsigned int);
	int    (*real_select)(unsigned int);
	int    (*get_id)(unsigned int);
	int    (*check)(unsigned int,unsigned int,unsigned int);
}app_channel_ops;

#endif

