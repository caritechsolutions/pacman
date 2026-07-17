/*
 * =====================================================================================
 *
 *       Filename:  app_nim.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011年09月16日 10时32分50秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#ifndef __APP_NIM_H__
#define __APP_NIM_H__

#include "gxcore.h"
#include "app_config.h"
#include "gxfrontend.h"

/* **********************to be finish********************************* */
// the follow macro need to be a funtion, can get dmx_id from ts_input
#if CA_SUPPORT || defined(SUBTTRANS_SUPPORT)
#define TS_2_DMX     1
#else
#define TS_2_DMX     0
#endif
/* ******************************************************************* */

//NIM
#define CFG_NIM_POS_MAX         (100)
#if FM_SUPPORT
#define NIM_MODULE_NUM          4
#else
#define NIM_MODULE_NUM          3//(2)
#endif
typedef enum
{
    AppFrontendID_DiseqcCmd    =                1,
    AppFrontendID_Status       =                2,
    AppFrontendID_SetTp        =                3,
    AppFrontendID_Config       =                4,
    AppFrontendID_Info         =                5,
    AppFrontendID_Strength     =                6,
    AppFrontendID_Quality      =                7,
    AppFrontendID_ErrorRate    =                8
}AppFrontendID;
typedef GxFrontendDiseqcParameters              AppFrontend_DiseqcParameters;
// 0-unlock 1-lock -1-locking
typedef int32_t                                 AppFrontend_Status;
typedef GxMsgProperty_FrontendSetTp             AppFrontend_SetTp;
typedef GxFrontendInfo                          AppFrontend_Info;
typedef struct dvbs_plsn_convert_param          AppFrontend_PlsNRootConvert;

// frontend device miss in 1.0.8
#define SEC_VOLTAGE_ALL             (SEC_VOLTAGE_OFF+1)

typedef enum
{
    FRONTEND_MONITOR_OFF,
    FRONTEND_MONITOR_ON
}AppFrontend_Monitor;

typedef enum
{
    FRONTEND_UNLOCK,
    FRONTEND_LOCKED
}AppFrontend_LockState;

typedef enum
{
    FRONTEND_CHANGED,
    FRONTEND_UNCHANGED,
}AppFrontend_ChangeState;

typedef enum
{
    FRONTEND_LNB_ON,
    FRONTEND_LNB_OFF,
}AppFrontend_LNBState;

typedef enum
{
    FRONTEND_22K_ON,
    FRONTEND_22K_OFF,
    FRONTEND_22K_NULL,
}AppFrontend_22KState;

typedef enum
{
    FRONTEND_POLAR_13V,
    FRONTEND_POLAR_18V,
    FRONTEND_POLAR_OFF,
}AppFrontend_PolarState;

typedef uint32_t  AppFrontend_Strengh;
typedef uint32_t  AppFrontend_Quality;
typedef uint32_t  AppFrontend_ErrorRate;
typedef struct
{
    AppFrontend_ChangeState        state;
    AppFrontend_DiseqcParameters   *diseqc;
}AppFrontend_DiseqcChange;

typedef struct
{
    AppFrontend_ChangeState  state;
    AppFrontend_22KState     _22k;
}AppFrontend_22KChange;

typedef struct
{
    AppFrontend_ChangeState     state;
    AppFrontend_PolarState      polar;
}AppFrontend_PolarChange;

struct nim_cfg
{
    uint32_t    tuner;      // tuner value must from 0, becase "/dev/dvb/adapter0/frontend*", * from 0
    uint32_t    ts_src;
    uint32_t    dmx_id;
};

typedef fe_caps_t AppFrontend_Caps;
typedef struct nim_cfg AppFrontend_Open;
typedef struct nim_cfg AppFrontend_Config;

typedef struct _nim AppNim;
struct _nim
{
    handle_t    fro_dev_hdl;
    handle_t    fro_mod_hdl;
    handle_t    dmx_mod_hdl;

    uint32_t    tuner;      // tuner value must from 0, becase "/dev/dvb/adapter0/frontend*", * from 0
    uint32_t    ts_src;
    uint32_t    dmx_id;
    AppFrontend_LockState    lock_state;

    AppFrontend_22KState     _22k;
    AppFrontend_PolarState   Polar;

    AppFrontend_DiseqcParameters diseqc10;
    AppFrontend_DiseqcParameters diseqc11;
    AppFrontend_DiseqcParameters diseqc12;

    void        (*tp_set)            (AppNim*, AppFrontend_SetTp*);// id,param,len
    void        (*diseqc_set)        (AppNim*, AppFrontend_DiseqcParameters*);
    void        (*diseqc_set_ex)     (AppNim*, AppFrontend_DiseqcParameters*);
    void        (*diseqc_change)     (AppNim*, AppFrontend_DiseqcChange*);
    void        (*monitor_set)       (AppNim*, AppFrontend_Monitor);
    void        (*lock_state_set)    (AppNim*, AppFrontend_LockState);
    void        (*lock_state_get)    (AppNim*, AppFrontend_LockState*);
    void        (*strength_get)      (AppNim*, AppFrontend_Strengh*);
    void        (*quality_get)       (AppNim*, AppFrontend_Quality*);
    void        (*errorrate_get)     (AppNim*, AppFrontend_ErrorRate*);
    void        (*info_get)          (AppNim*, AppFrontend_Info*);
    void        (*config_get)        (AppNim*, AppFrontend_Config*);
    void        (*config_set)        (AppNim*, AppFrontend_Config*);
    status_t    (*lnb_cfg)           (AppNim*, AppFrontend_LNBState*);
    void        (*_22k_change)       (AppNim*, AppFrontend_22KChange*);
    void        (*switch_22k_set)    (AppNim*, AppFrontend_22KState*);
    void        (*polar_change)      (AppNim*, AppFrontend_PolarChange*);
    void        (*polar_set)         (AppNim*, AppFrontend_PolarState*);
    status_t    (*demux_cfg)         (AppNim*);
    int32_t     (*set_unicable)      (AppNim*, AppFrontend_SetTp*);

    status_t    (*open)              (AppNim*, AppFrontend_Open*);
    status_t    (*close)             (AppNim*);
    int         (*calc_pls_n)        (AppNim*, unsigned int*);
    int         (*plsn_root_convert) (AppNim*, AppFrontend_PlsNRootConvert*);
    int         (*fe_caps_get)       (AppNim *nim, fe_caps_t *caps);
};

extern status_t nim_init(uint32_t Index, int demod_type);
extern void     nim_release(void);
extern status_t nim_ioctl(uint32_t id, uint32_t cmd, void * params);

#define CALC_SIGNAL_STRENGTH(s) (s/16*100/4096)

#endif

