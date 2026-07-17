#ifndef APP_FRONTEND_H
#define APP_FRONTEND_H

#define FRONTEND_SET_DELAY_MS (100)

#include "module/app_nim.h"
typedef enum
{
	ID_9750_10600 = 0,
	ID_9750_10700,
	ID_9750_10750,
#if LNBF_SUPPORT
	ID_10250_11400,
	ID_11400_10250,
#endif
	ID_5150_5750,
	ID_5750_5150,
	ID_5150,
	ID_5750,
	ID_5950,
	ID_9750,
	ID_10000,
	ID_10600,
	ID_10700,
	ID_10750,
	ID_11250,
	ID_11300,

#if UNICABLE_SUPPORT
	ID_UNICABLE,
	ID_UNICABLE_2,
#endif
	ID_TATAL_LNB,
}LnbFreqId;

#if LNBF_SUPPORT
#define FRONTEND_LNBF_GPIO_SET_FRE  (830)//MHz
#endif

#if UNICABLE_SUPPORT
typedef enum
{
	ID_UNICANBLE_9750_10600 = 0,
	ID_UNICANBLE_9750_10700,
	ID_UNICANBLE_9750_10750,
	ID_UNICANBLE_9750,
	//ID_UNICANBLE_10000,
	ID_UNICANBLE_10600,
	//ID_UNICANBLE_10700,
	//ID_UNICANBLE_10750,
	//ID_UNICANBLE_11250,
	ID_UNICANBLE_11300,
	ID_UNICANBLE_TATAL_LNB,
}LnbUnicableFreqId;
#endif

#define FRONTEND_SET_DELAY_MS (100)
#define DISH_MOVE_SEC 100

typedef enum
{
	SET_TP_AUTO,
	SET_TP_FORCE,
	SAVE_TP_PARAM
}SetTpMode;

typedef enum
{
	LONGITUDE_EAST,
	LONGITUDE_WEST,
	LONGITUDE_ERR
}LongitudeDirect;

typedef enum
{
	LATITUDE_SOUTH,
	LATITUDE_NORTH,
	LATITUDE_ERR
}LatitudeDirect;

typedef struct
{
	int longitude;
	LongitudeDirect longitude_direct;
}LongitudeInfo;

typedef struct
{
	int latitude;
	LatitudeDirect latitude_direct;
}LatitudeInfo;

typedef enum
{
	LIMIT_EAST,
	LIMIT_WEST,
	LIMIT_OFF,
}Diseqc12Limit;

typedef struct
{
	LongitudeInfo longitude;
	LatitudeInfo latitude;
}PositionVal;

typedef enum
{
  DISH_NONE,
  DISH_MOVING,
  DISH_OUT_RANGE
}MotorState;

typedef struct
{
    MotorState state;
    int32_t motor_state;
    int32_t flag;
}MotorTip;

void app_lnb_freq_sel_to_data(uint16_t cur_sel, GxBusPmDataSat *p_sat);
void app_lnb_freq_data_to_sel(GxBusPmDataSat *p_sat, uint32_t *p_sel);
uint32_t app_show_to_sat_22k(GxBusPmSat22kSwitch switch_22K, GxBusPmDataTP *p_tp);
uint32_t app_show_to_tp_freq(GxBusPmDataSat *p_sat, GxBusPmDataTP *p_tp);
uint32_t app_show_to_lnb_polar(GxBusPmDataSat *p_sat);
uint32_t app_show_to_tp_polar(GxBusPmDataTP *p_tp);
void app_set_diseqc_10(uint32_t tuner, AppFrontend_DiseqcParameters *diseqc10, bool force);
void app_set_diseqc_11(uint32_t tuner, AppFrontend_DiseqcParameters *diseqc11, bool force);
void app_set_tp(uint32_t tuner, AppFrontend_SetTp *params , SetTpMode set_mode);
void app_diseqc12_save_position(uint32_t tuner,uint32_t pos);
MotorState app_diseqc12_goto_position(uint32_t sat_id, uint32_t tuner, uint32_t pos, bool force, int32_t motor_state);
void app_diseqc_sat_id_bak_set(uint32_t id);
void app_diseqc12_goto_zero(uint32_t tuner, bool force);
void app_diseqc12_find_drive(uint32_t tuner,LongitudeDirect direct);
void app_diseqc12_save_limit(uint32_t tuner,Diseqc12Limit limit);
void app_diseqc12_recalculate(uint32_t tuner,uint32_t pos);
MotorState app_usals_goto_position(uint32_t sat_id, uint32_t tuner,LongitudeInfo *sat_longitude, PositionVal *local_position, bool force, int32_t motor_state);
void app_diseqc12_stop(uint32_t tuner, uint8_t Count);

void app_frontend_demod_control(uint32_t Tuner,uint8_t Flag);
void app_diseqc12_stop_drictory(uint32_t tuner, uint8_t Count);
void app_diseqc12_find_drive_drictory(uint32_t tuner,LongitudeDirect direct);

extern uint32_t app_frontend_sg_tuner_get(void);
extern void app_frontend_sg_tuner_set(uint32_t sg_tuner);

typedef enum _FRONTEND_MODE
{
	   DVBS2_NORMAL,
	   DVBS2_BLIND,
	   DVBT_AUTO_MODE,
	   DVBT_NORMAL,
	   DVBT2_BASE,
	   DVBT2_LITE,
	   DVBT2_BASE_LITE,
	   DVBC_J83A,
	   DVBC_J83B,
	   DTMB_C,
	   DTMB,
}FRONTEND_MODE;

#if MULTISTREAM_SUPPORT
void app_set_muti_ts_para(uint32_t tuner,uint8_t ts_id,uint8_t code);
unsigned int app_get_muti_ts_num(int32_t tuner);
#endif
extern unsigned int app_frontend_standby(void);
extern bool app_frontend_need_predemux(uint32_t tuner);
extern bool app_frontend_hard_cap(uint32_t tuner, int flag);

extern void app_create_position_tip(MotorState tip, int32_t motor_state);
extern uint32_t app_tuner_cur_tuner_get(void);

extern bool app_frontend_dvbs_frequency_check(uint32_t frequency);
#endif
