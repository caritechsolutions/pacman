#ifndef __APP_FRONTEND_INIT_H__
#define __APP_FRONTEND_INIT_H__

#include "app.h"
#include <frontend/tuner.h>
#include <frontend/demod.h>

/**
 * APP_TUNER_TYPE
 * format:
 *   TUNER + type + mode(if it can support multy mode)
 * this name must be matched with solution/.config
 * for example, BR2_TUNER1_R848_DVBS in solution/.config
 * and TUNER_R848_DVBS in this enum, R848_DVBS must be the same
 */
typedef enum{
	TUNER_AV2018,
	TUNER_AV2020,
	TUNER_MXL608_DTMB,
	TUNER_MXL608_DVBT,
	TUNER_MXL608_DVBC,
	TUNER_TDA18250,
	TUNER_TDA18250A,
	TUNER_TDA18273_DTMB,
	TUNER_TDA18273_DVBC,
	TUNER_R836_DVBT,
	TUNER_R836_DVBC,
	TUNER_R836_DTMB,
	TUNER_R910_DVBT,
	TUNER_R910_DVBC,
	TUNER_SI2141_DTMB,
	TUNER_SI2141_DVBC,
	TUNER_SI2141_DVBT,
	TUNER_R848_DVBS,
	TUNER_R848_DVBC,
	TUNER_R848_DVBT,
	TUNER_R848_DTMB,
	TUNER_RDA5815M,
	TUNER_ATBM2040,
	TUNER_ATBM2030,
	TUNER_ATBM253_DVBT,
	TUNER_ATBM253_DVBC,
	TUNER_ATBM253_DTMB,
	TUNER_SHARP7306,
	TUNER_AUTOADAPT_DVBT,
	TUNER_AUTOADAPT_DVBC,
	TUNER_RDA5160_DTMB,
	TUNER_RDA5160_DVBC,
	TUNER_RDA5160_DVBT,
	TUNER_MXL683_ISDBT,
	TUNER_MXL608_ISDBT,
	TUNER_TS6011,
	TUNER_M3031,
    TUNER_RT720,
	TUNER_R858_DVBC_1ST,
	TUNER_R858_DVBC_2ND,
	TUNER_AUTO_ADAPT,
}APP_TUNER_TYPE;//do not change, this name will be analyzed by parse_frontend_config.py


#define MAX_DEV_ADDR_NUM (4)
struct app_frontend_tuner_adapt_params{
	unsigned char     frontend_index;
	unsigned int       i2c_id;
#ifdef LINUX_OS
	unsigned int      tuner_attach;
#else
	tuner_attach_func tuner_attach;
#endif
	unsigned char     chip_addr_list[MAX_DEV_ADDR_NUM];
	unsigned int      iq;
	unsigned int      xtal;
	unsigned int      if_freq;
	unsigned int      xtal_cap;
	unsigned int      clock_out;
	unsigned int      loop_out;
	unsigned int      connection;
};

struct AppPinSwitch{
	unsigned char tuner;
	unsigned char onoff;
};

typedef enum{
    DEMOD_MANUAL_ADJUSTMENT,
    DEMOD_AUTO_ADAPT,
}APP_DEMOD_ADAPT_TYPE;

#if DEMOD_AUTO_ADAPT_SUPPORT
#define DEMOD_AUTO_ADAPT_MAX_PIN_SWITCH_NUM 12
typedef struct demod_adapt_demod_g {
	uint8_t			frontend_index;
	uint8_t			i2c_id;
	uint32_t		demod_addr_adapt[MAX_DEV_ADDR_NUM];
#ifdef LINUX_OS
	uint32_t		demod_attach_adapt;
#else
	demod_attach_func	demod_attach_adapt;
#endif
}demod_adapt_demod_t;

typedef struct demod_adapt_info_ts_g {
	uint8_t		frontend_index;
	ts_pin_t	ts_pin_config_00;
	ts_pin_t	ts_pin_config_01;
	ts_pin_t	ts_pin_config_02;
	ts_pin_t	ts_pin_config_03;
	ts_pin_t	ts_pin_config_04;
	ts_pin_t	ts_pin_config_05;
	ts_pin_t	ts_pin_config_06;
	ts_pin_t	ts_pin_config_07;
	ts_pin_t	ts_pin_config_08;
	ts_pin_t	ts_pin_config_09;
	ts_pin_t	ts_pin_config_10;
	ts_pin_t	ts_pin_config_11;
	ts_mode_t	ts_mode;
	int32_t is_oscillator;
	int32_t ts_src;
}demod_adapt_info_ts_t;

typedef struct demod_adapt_pin_switch_g{
	uint8_t			frontend_index;
	uint8_t			pin_count;
	MulPin          pin_data[DEMOD_AUTO_ADAPT_MAX_PIN_SWITCH_NUM];
}demod_adapt_pin_switch_t;

typedef struct demod_adapt_ts4pin_switch_g{
	uint8_t			frontend_index;
	uint8_t			ts4pin_count;
	struct AppPinSwitch ts4pin[MAX_DEV_ADDR_NUM];
}demod_adapt_ts4pin_switch_t;
#endif

struct app_frontend_config{
	struct dev_attach dev;//decide to use demod and tuner
	struct pin_config demod_pin_map;//mostly used by tuner2
	unsigned int iq_swap;//0:swap  1:no_swap
	int lnb_power_gpio;//set gpio to control lnb on/off
	int lnb_invert;//0:no invert 1:invert
	int polar_invert;//0:no invert 1:invert
	unsigned int lnb_ctrl_enable;//0: disable, 1: enable
	unsigned int pol_ctrl_enable;//0: disable, 1: enable
	ts_mode_t ts_mode;//0:parrallel 1:serial 2:other
	unsigned int if_freq;//KHz, used to be 4500, 5000
	unsigned int tuner_xtal;//MHz, used to be 16,24,27
	unsigned int tuner_xtal_cap;//pf
	unsigned int tuner_clock_out_ctrl;//0: disable, 1: enable
	unsigned int tuner_loop_out_ctrl;//0: disable, 1: enable
	fe_tuner_connection connection;//0: FE_DIFFERENTIAL, 1: FE_SINGLE_END
	unsigned int is_oscillator;//0:crystal  1:oscillator
};

#define APP_FRONTEND_INVALID_TUNER_I2C_ADDR 0x00

void app_mod_tuner_config_init(void);
#endif
