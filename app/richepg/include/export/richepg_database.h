#ifndef __RICHEPG_DATABASE_H__
#define __RICHEPG_DATABASE_H__
#include "richepg_block_mem.h"

/******** preset sat and ts manage ********/

typedef enum {
    RICHEPG_TS_POLAR_H = 0,
    RICHEPG_TS_POLAR_V,
    RICHEPG_TS_POLAR_AUTO
} RichepgTsPolar;

typedef struct {
    uint32_t frequency;
    uint32_t symbol_rate;
    RichepgTsPolar polar;
} RichepgTsTP;

typedef struct {
    int lineup_id;
    int lineup_id_bak;
    int bid_num;
    int *bid_array;
    int nid;
    int tp_num;
    RichepgTsTP *tp_array;
    int sat_id;
    void *sat_data;
} RichepgTsSat;

typedef struct {
    int (*add_sat)(void *json, void **sat_data);
    int (*get_sat)(int sat_id, void **sat_data);
    int (*free_sat)(void **sat_data);
} RichepgTsMgrCb;

typedef struct {
    int sat_sel;
    int sat_sel_bak;
    int sat_num;
    RichepgTsSat *sat_array;
    RichepgTsMgrCb sat_cb;
    const char *default_ts_path;
    const char *ignore_sat_name;
} RichepgTsMgr;

RichepgTsMgr *richepg_ts_mgr_get(void);
int richepg_ts_mgr_init(RichepgTsMgrCb cb, const char *default_ts_path, const char *ignore_sat_name);
int richepg_ts_mgr_release(void);
int richepg_ts_mgr_parse_def(void);
int richepg_ts_mgr_parse(void);
int richepg_ts_mgr_save(int sat_sel);
int richepg_ts_mgr_delfile(void);

/******** prog list manage ********/

typedef struct {
    int is_default;
    char *lang;
    char *name;
} RichepgProgNameItem;

typedef struct {
    uint16_t id;
    uint16_t prog_id;
    char *prog_logo;
    char *ear_logo;
    int lcns_num;
    int *lcns_array;
    int genres_num;
    int *genres_array;
    int types_num;
    int *types_array;
    int names_num;
    RichepgProgNameItem *names_array;
} RichepgProgListProg;

typedef struct {
    int sid;
    int progid;
    int chid;
} RichepgProgListCH;

typedef struct {
    int onid;
    int tsid;
    int tpid;
    int ch_num;
    RichepgProgListCH *ch_array;
} RichepgProgListTP;

typedef struct {
    int prog_num;
    RichepgProgListProg *prog_array;
    int tp_num;
    RichepgProgListTP *tp_array;

    BlockMemMgr struct_mgr;
    BlockMemMgr string_mgr;
    BlockMemMgr ear_mgr;
} RichepgProgListCtrl;

RichepgProgListCtrl *richepg_prog_list_ctrl_get(void);
int richepg_proglist_ctrl_init(void);
int richepg_proglist_release(void);
int richepg_proglist_parse(void);
int richepg_proglist_save(void);
int richepg_proglist_delfile(void);

#endif

