#ifndef __GX_CLOCK_HAL_H__
#define __GX_CLOCK_HAL_H__

#ifndef GXLOADER_OS
#include "gxav.h"
#include "vout_hal.h"
#include "avcore.h"
#include "clock_module.h"
#endif /*GXLOADER_OS*/
#include "gxav_common.h"

#ifdef GX_DEBUG
#define CLOCK_PRINTF(fmt, args...) do{ \
    gx_printf("\n[COMMON][%s():%d]: ", __func__, __LINE__);\
    gx_printf(fmt, ##args); \
} while(0)
#define CLOCK_DBG(fmt, args...)   do{\
    gx_printf("\n[COMMON][%s():%d]: ", __func__, __LINE__);\
    gx_printf(fmt, ##args);\
}while(0)
#else
#define CLOCK_PRINTF(fmt, args...)   ((void)0)
#define CLOCK_DBG(fmt, args...)   ((void)0)
#endif

enum module_type {
    MODULE_TYPE_AUDIO_OR       = 0x00,
    MODULE_TYPE_AUDIO_I2S      ,
    MODULE_TYPE_AUDIO_SPDIF    ,
    MODULE_TYPE_AUDIO_LODEC    ,

    MODULE_TYPE_VIDEO_DEC      = 0x10,
    MODULE_TYPE_JPEG           ,
    MODULE_TYPE_PP             ,
    MODULE_TYPE_GA             ,
    MODULE_TYPE_VOUT           ,
    MODULE_TYPE_HDMI           ,
    MODULE_TYPE_VPU            ,
    MODULE_TYPE_SVPU           ,
    MODULE_TYPE_VDAC           ,

    MODULE_TYPE_DEMUX_SYS      = 0x20,
    MODULE_TYPE_PIDFILTER_SYS  ,
    MODULE_TYPE_DEMUX_STC      ,

    MODULE_TYPE_FRONTEND       ,
    MODULE_TYPE_MAX
};

enum pin_type {
    PIN_TYPE_AUDIOOUT_I2S,
    PIN_TYPE_DEMUX_TS3,
    PIN_TYPE_VIDEOOUT_YUV,
    PIN_TYPE_VIDEOOUT_SCART,
    PIN_TYPE_VIDEOOUT_CVBS,
};

enum clock_source {
	CLOCK_SOURCE_FROM_XTAL,
	CLOCK_SOURCE_FROM_DTO,
	CLOCK_SOURCE_FROM_PLL,
};

enum audio_clk_source {
	I2S_PLL,
	I2S_DTO,
    SPDIF_FROM_I2S,
    SPDIF_DTO
};

enum video_dac_clk_source {
	VIDEO_DAC_SRC_VPU,
	VIDEO_DAC_SRC_SVPU,
};

enum frontend_config_mode {
	FRONTEND_CLOCK_CONFIG,
	FRONTEND_ADC_INIT,
};

typedef union {
	struct {
		unsigned int clock;
		unsigned int div;
	}vpu;
	struct {
		unsigned int src;
		unsigned int mode;
	}vout;
	struct {
		unsigned int value;
	}audio_or;
	struct {
		unsigned int clock_source;//enum audio_clk_source
		unsigned int value;
	}audio_spdif;
	struct {
		unsigned int clock_source;//enum audio_clk_source
		unsigned int value;
		unsigned int sample_index;
		unsigned int clock_speed;
	}audio_i2s;
	struct {
		unsigned int mode;
		unsigned int demod_clk_div;
		unsigned int fec_clk_div;
		unsigned int adc_clk_div;
	}frontend;

	struct {
		unsigned int freq;
		unsigned int modid;
	};

	struct {
		unsigned int vpu_up_freq;
		unsigned int vpu_freq;
		unsigned int hdmi_freq;
		unsigned int dcs_freq;
	};
	struct {
		unsigned int svpu_up_freq;
		unsigned int svpu_freq;
	};
}clock_params;

typedef union {
	struct {
		unsigned int dvb_pll_fbdiv;
	}frontend;

}clock_configs;

struct gxav_clock_module {
	int (*setup)                          (struct gxav_module_resource *res, void *priv);
	int (*init)                           (void);
	int (*uninit)                         (void);
	int (*cold_rst)                       (unsigned int module);
	int (*hot_rst_set)                    (unsigned int module);
	int (*hot_rst_clr)                    (unsigned int module);
	int (*setclock)                       (unsigned int module, clock_params* arg);
	int (*getconfig)                      (unsigned int module, clock_configs* arg);
	int (*source_enable)                  (unsigned int module);
	int (*source_disable)                 (unsigned int module);
	int (*source_switch)                  (unsigned int module, enum clock_source source);
	int (*module_clock_enable)            (unsigned int module, int flag);
	int (*vdac_clock_source)              (unsigned int dacid, enum video_dac_clk_source src);
	int (*audioout_dacinside)             (unsigned int mclock);
	int (*audioout_dacinside_mute)        (unsigned int enable);
	int (*audioout_dacinside_clock_enable)(unsigned int enable);
	int (*audioout_dacinside_clock_select)(unsigned int div);
	int (*audioout_dacinside_slow_enable) (unsigned int power,    unsigned int enable);
	int (*audioout_dacinside_slow_config) (unsigned int step_num, unsigned int skip_num);
	int (*audioout_dacinside_volume)      (int voldb);
	void *priv;
};

#ifndef GXLOADER_OS
extern int gxav_clock_setup(struct gxav_module_ops *module, struct gxav_module_resource *res);
extern int gxav_clock_init (struct gxav_module_ops* ops);
#endif /*GXLOADER_OS*/
extern int gxav_clock_uninit(void);
extern int gxav_clock_cold_rst(unsigned int module);
extern int gxav_clock_hot_rst_set(unsigned int module);
extern int gxav_clock_hot_rst_clr(unsigned int module);
extern int gxav_clock_setclock(unsigned int module, clock_params* arg);
extern int gxav_clock_getconfig(unsigned int module, clock_configs* arg);
extern int gxav_clock_source_enable (unsigned int module);
extern int gxav_clock_source_disable(unsigned int module);
extern int gxav_clock_source_switch (unsigned int module, enum clock_source source);
extern int gxav_clock_audioout_dacinside(unsigned int mclock);
extern int gxav_clock_audioout_dacinside_mute        (unsigned int enable);
extern int gxav_clock_audioout_dacinside_clock_enable(unsigned int enable);
extern int gxav_clock_audioout_dacinside_clock_select(unsigned int enable);
extern int gxav_clock_set_video_dac_clock_source(unsigned int dacid, enum video_dac_clk_source src);
extern int gxav_clock_audioout_dacinside_slow_enable(unsigned int power,    unsigned int enable  );
extern int gxav_clock_audioout_dacinside_slow_config(unsigned int step_num, unsigned int skip_num);
extern int gxav_clock_audioout_dacinside_volume(int voldb);

extern int gxav_clock_module_enable(int module, int flag);

extern unsigned int gxav_pll_fre_table[][4];
extern unsigned int gxav_dto_fre_table[];
extern unsigned int gxav_audioout_pllclk[][5];

#endif
