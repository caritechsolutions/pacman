/*****************************************************************************
* 			  CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2010, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_module.h
* Author    : 	shenbin
* Project   :	GoXceed -S
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2010.2.27	            shenbin         creation
*****************************************************************************/

#ifndef __GXAPP_MODULE_H__
#define __GXAPP_MODULE_H__
#include "app_config.h"
#include "module/app_play_control.h"
#include "module/app_time.h"
#include "module/app_id_manage.h"
#include "module/app_input.h"
#include "module/app_pmt.h"
#if PAT_SUPPORT
#include "module/app_pat.h"
#endif
#if SDT_SUPPORT
#include "module/app_sdt.h"
#endif
#include "module/app_nit.h"
#include "module/app_nim.h"
#include "module/app_pvr.h"
#include "module/app_ttx_subt.h"
#include "module/app_ioctl.h"
#include "module/app_ecm.h"
#include "app_data_type.h"
#include "module/ttx/gxttx.h"
#include "module/player/gxplayer_module.h"
#include <assert.h>
#include "module/app_oem.h"
#include "module/app_panel.h"
#include "module/app_gpio.h"
#include "module/app_nim.h"
#if CA_SUPPORT
#include "module/app_ca.h"
#endif
#include "module/app_i2c.h"
#include "module/app_watchdog.h"
#include "module/app_network_api.h"
#include "module/app_network_service.h"
#include "module/app_vpn_service.h"
#include "module/app_fifo.h"
#include "module/app_psi_module.h"

#if EMM_SUPPORT
#include "module/app_cat.h"
#include "module/app_emm.h"
#endif
#include "module/app_ci.h"
#if PARENTAL_LOCK_SUPPORT
#include "app_parent_lock.h"
#endif

#if BOUQUET_SUPPORT
#include "module/app_bat.h"
#endif
#if AUTO_UPDATE_SUPPORT || PMT_UPDATE_SUPPORT || BOUQUET_SUPPORT
#include "module/app_auto_update.h"
#endif
/* export macro -------------------------------------------------*/
#define APP_ASSERT(exp)  assert((exp))

#define PLAYER_FOR_NORMAL       "player1"
#define PLAYER_FOR_PIP          "player2"
#define PLAYER_FOR_REC          "player3"
#define PLAYER_FOR_JPEG         "player4"
#define PLAYER_FOR_IPTV         "player5"
#define PLAYER_FOR_FM           "player6"
#define PLAYER_FOR_TIMESHIFT    PLAYER_FOR_NORMAL
#define MAX_SUBT_COUNT 6
//#define DISEQ12_USELESS_ID 0xff
#define DISEQ12_USELESS_ID 0 //和预置卫星保持一致#75071
//NIM
typedef int32_t GxFrontendProperty_Status;

typedef enum
{
	PLAYER_OPEN_OK,
	PLAYER_CLOSE_OK,
	PLAYER_NAME_ERROR,
	PLAYER_OPS_FORBID
}AppPlayerStatus;

/*for telesubtitle*/

/*extern function -----------------------------------------------------*/
// id ops
extern AppIdOps* new_id_ops(void);
extern void del_id_ops(AppIdOps*);

// subt
extern void app_subt_change(void);
extern void app_subt_pause(void);
extern void app_subt_resume(void);

//2 app_player_manage
extern AppPlayerStatus app_player_open(char* player_name);
extern AppPlayerStatus app_player_close(char* player_name);

//2 app_pvr
extern int app_pvr_rec_start(uint32_t dura_sec);
extern void app_tp_rec_start(uint32_t dura_sec);
extern void app_tplist_rec_start(void);
extern void app_pvr_rec_stop(void);
extern void app_pvr_tms_stop(void);
extern void app_pvr_stop(void);
extern void pvr_taxis_mode_clear(void);

// app_ts
extern void app_ts_change(uint32_t ts_src);

//chip sn
extern int app_chip_sn_get(char *SnData, int BufferLen);

//chip name
int app_chip_name_get(char *SnData, int BufferLen);

//chip id
extern uint32_t app_chip_id_get(void);

#if UNICABLE_SUPPORT
extern int app_calculate_set_freq_and_initfreq(GxBusPmDataSat *p_sat, GxBusPmDataTP *pTpPara,uint32_t *pSetTPFreq,uint32_t* pInitialFrequency);
#endif

#if RECALL_LIST_SUPPORT
int _recall_normal(void);
int _key_recall_stop_pvr_cb(PopDlgRet ret);
#endif
extern VideoOutputMode app_av_get_cvbs_standard(VideoOutputMode vout_mode);
#endif

