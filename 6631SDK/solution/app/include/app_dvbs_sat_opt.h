#ifndef __APP_DVBS_SAT_OPT_H__
#define __APP_DVBS_SAT_OPT_H__

#include "app.h"
#if (DEMOD_DVB_S > 0)

#include "module/app_frontend.h"
#include "app_default_params.h"

#define MAX_NUM  3

enum{
    SAT_NONE_CHANGE = 0<<0,
    SAT_LIST_UPDATE_SATINFO = 1<<0,
    ANT_SET_UPDATE_SATINFO = 1<<1,
    TP_NONE_CHANGE = 1<<2,
    TP_LIST_UPDATE_TPINFO = 1<<3,
    ANT_SET_UPDATE_TPINFO = 1<<4
};

enum{
    TUNER1 = 0,
    TUNER2,
    TUNER_ALL
};

typedef struct{
    uint32_t cur_tuner;
    uint32_t which_tuner_sat_exist;
	uint32_t sat_buf[SYS_MAX_SAT];
	uint32_t sat_total;
    uint32_t sat_id_bak;
	uint32_t cur_sat_sel[MAX_NUM];
	uint32_t cur_sat_tp_total;
	uint32_t cur_tp_sel;
    int32_t  change;
	GxMsgProperty_NodeByPosGet cur_sat;
	GxMsgProperty_NodeByPosGet cur_tp;

	uint32_t (*sat_gui_sel_get_by_pos)(uint32_t sat_pos);
    uint32_t (*sat_gui_sel_get_by_id)(uint32_t sat_id);
    void (*sat_gui_sel_update)(void);
    void (*tuner_rebuild)(char *cmb_name);
	void (*sat_rebuild)(char *cmb_name, bool tuner_show);
	void (*tp_rebuild)(char *cmb_name);
	int (*sat_poplist_get_name)(char **p_sat_name);
	int (*tp_poplist_get_info)(char **p_tp_info);
	uint32_t (*sat_num_get)(uint32_t tuner);
	uint32_t (*cur_sat_tp_num_get)(void);
	void (*cur_sat_get)(void);
	void (*cur_tp_get)(void);
	void (*set_diseqc)(PositionVal *sat_position);
	void (*set_tp)(SetTpMode set_mode);
} dvbs_sat_data_t;

extern dvbs_sat_data_t s_dvbs_sat_data;
int app_clean_signal(char* progbar_strength, char* text_strength, char* progbar_quality, char* text_quality);
int app_show_signal(char* progbar_strength, char* text_strength, char* progbar_quality, char* text_quality);

#endif
#endif

