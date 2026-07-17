#include "app.h"
#include "module/app_gpio.h"
#include "module/app_ioctl.h"
#if !OTT_SUPPORT
#include "module/app_frontend_init.h"
#include "module/app_frontend_board_config.h"
#if FM_SUPPORT
extern int app_get_fm_tuner_id(void);
#endif

enum{
	APP_DO_NOT_FE_ADAPT,			//fe don't auto adapt
	APP_TUNER_AUTO_ADAPT,			//tuner auto adapt
	APP_DEMOD_AUTO_ADAPT,			//demod auto adapt
	APP_DEMOD_TUNER_AUTO_ADAPT		//demod & tuner auto adapt
};

#if (TUNER_AUTO_ADAPT_SUPPORT) || (DEMOD_AUTO_ADAPT_SUPPORT)
static int _set_device_multi_demod_tuner_adapt(handle_t fe_handle, struct app_frontend_config *config, uint32_t fe_adapt_flag);
#endif

#if DEMOD_AUTO_ADAPT_SUPPORT
static int _demod_adapt(handle_t fe_handle, uint8_t fe_id, struct app_frontend_config *config, uint32_t fe_adapt_flag);

typedef struct demod_adapt_dev_info_lnb_g{
	uint8_t		frontend_index;
	int32_t lnb_power_gpio;
	int32_t lnb_invert;
	int32_t lnb_ctrl_enable;
	int32_t pol_ctrl_enable;
	int32_t pol_invert;
	uint8_t agc_gpio;
	uint8_t hv_sel_gpio;
	uint8_t diseqc_out_gpio;
	uint8_t diseqc_in_gpio;
}demod_adapt_info_lnb_t;
#endif

/************************************** demod and tuner init ****************************************/
static int app_init_demod_tuner(struct app_frontend_config *config, uint32_t fe_id, uint32_t fe_adapt_flag)
{
	int ret = 0;
	handle_t fe_handle = -1;

	if(GxFrontend_Init(fe_id)<0)
		return -1;

	fe_handle = GxFrontend_IdToHandle(fe_id);
	if (fe_handle < 0) {
		app_log_error("[%s]register frontend failed\n", __func__);
		return -1;
	}
	if(config->dev.demod_dev.id == 0xF)
		config->dev.demod_dev.id = fe_id;
	if(config->dev.tuner_dev.id == 0xF)
		config->dev.tuner_dev.id = fe_id;

	config->dev.init_params_valid_flag = GXFE_INIT_PARAMS_VALID_FLAG;
	config->dev.info.power_mode = FE_HIGH_PERFORMENCE_MODE;
#if LNBF_SUPPORT
	config->dev.info.lnbf_config.mode = GXFE_LNBF_ENABLE;
	config->dev.info.lnbf_config.gpio1 = V_GPIO_VCONT_1;
	config->dev.info.lnbf_config.gpio2 = V_GPIO_VCONT_2;
#else
	config->dev.info.lnbf_config.mode = GXFE_LNBF_DISABLE;
#endif
#if DEMOD_AUTO_ADAPT_SUPPORT
	if((APP_DEMOD_TUNER_AUTO_ADAPT == fe_adapt_flag) || (APP_DEMOD_AUTO_ADAPT == fe_adapt_flag)){
		ret = _demod_adapt(fe_handle, fe_id, config, fe_adapt_flag);
		if(ret < 0) {
			close(fe_handle);
		}
	}else
#endif
#if TUNER_AUTO_ADAPT_SUPPORT
	if(APP_TUNER_AUTO_ADAPT == fe_adapt_flag){
		if(_set_device_multi_demod_tuner_adapt(fe_handle, config, fe_adapt_flag) < 0) {
			close(fe_handle);
			return -1;
		}
	}else
#endif
		ret += ioctl(fe_handle, FE_SET_DEVICE, &(config->dev));

	if(ret < 0)
		app_log_error("**********[%s]ioctl error %d!**********\n", __FUNCTION__, ret);
	return ret;
}

int app_get_tuner_attach(struct dev_attach* attach, APP_TUNER_TYPE type)
{
	switch(type){
#if TUNER_R858_DVBC_1ST_SUPPORT
		case TUNER_R858_DVBC_1ST:
			attach->tuner_attach = R858_DVBC_1ST_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xd4;
			break;
#endif
#if TUNER_R858_DVBC_2ND_SUPPORT
		case TUNER_R858_DVBC_2ND:
			attach->tuner_attach = R858_DVBC_2ND_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xf4;
			break;
#endif
#if TUNER_AV2018_SUPPORT
		case TUNER_AV2018:
			attach->tuner_attach = AV2018_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;
			break;
#endif
#if TUNER_AV2020_SUPPORT
		case TUNER_AV2020:
			attach->tuner_attach = AV2020_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;
			break;
#endif
#if TUNER_RT720_SUPPORT
        case TUNER_RT720:
			attach->tuner_attach = RT720_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0x34;
            break;
#endif
#if TUNER_MXL608_DTMB_SUPPORT
		case TUNER_MXL608_DTMB:
			attach->tuner_attach = MXL608_DTMB_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;
			break;
#endif
#if TUNER_MXL608_DVBT_SUPPORT
		case TUNER_MXL608_DVBT:
			attach->tuner_attach = MXL608_T_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;
			break;
#endif
#if TUNER_MXL608_DVBC_SUPPORT
		case TUNER_MXL608_DVBC:
			attach->tuner_attach = MXL608_C_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;
			break;
#endif
#if TUNER_TDA18250_SUPPORT
		case TUNER_TDA18250:
			attach->tuner_attach = TDA18250_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;
			break;
#endif
#if TUNER_TDA18250A_SUPPORT
		case TUNER_TDA18250A:
			attach->tuner_attach = TDA18250A_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;
			break;
#endif
#if TUNER_TDA18273_DTMB_SUPPORT
		case TUNER_TDA18273_DTMB:
			attach->tuner_attach = TDA18273_DTMB_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;
			break;
#endif
#if TUNER_TDA18273_DVBC_SUPPORT
		case TUNER_TDA18273_DVBC:
			attach->tuner_attach = TDA18273_DVBC_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;
			break;
#endif
#if TUNER_R836_DVBT_SUPPORT
		case TUNER_R836_DVBT:
			attach->tuner_attach = R836_T_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0x74;//0x34, 0x74
			break;
#endif
#if TUNER_R836_DVBC_SUPPORT
		case TUNER_R836_DVBC:
			attach->tuner_attach = R836_C_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0x74;//0x34, 0x74
			break;
#endif
#if TUNER_R836_DTMB_SUPPORT
		case TUNER_R836_DTMB:
			attach->tuner_attach = R836_DTMB_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xF8;//0x34, 0x74
			break;
#endif
#if TUNER_R910_DVBT_SUPPORT
		case TUNER_R910_DVBT:
			attach->tuner_attach = R910_T_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0x38;//0x38, 0x78, 0xb8, 0xf8, 0x34, 0x74, 0xb4, 0xf4
			break;
#endif
#if TUNER_R910_DVBC_SUPPORT
		case TUNER_R910_DVBC:
			attach->tuner_attach = R910_C_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0x38;//0x38, 0x78, 0xb8, 0xf8, 0x34, 0x74, 0xb4, 0xf4
			break;
#endif
#if TUNER_SI2141_DTMB_SUPPORT
		case TUNER_SI2141_DTMB:
			attach->tuner_attach = SI2141_DTMB_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;//0xc0, 0xc2, 0xc4, 0xc6
			break;
#endif
#if TUNER_SI2141_DVBT_SUPPORT
		case TUNER_SI2141_DVBT:
			attach->tuner_attach = SI2141_T_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;//0xc0, 0xc2, 0xc4, 0xc6
			break;
#endif
#if TUNER_SI2141_DVBC_SUPPORT
		case TUNER_SI2141_DVBC:
			attach->tuner_attach = SI2141_C_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;//0xc0, 0xc2, 0xc4, 0xc6
			break;
#endif
#if TUNER_R848_DVBS_SUPPORT
		case TUNER_R848_DVBS:
			attach->tuner_attach = R848_S_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xF8;//0x38, 0xF8
			break;
#endif
#if TUNER_R848_DVBC_SUPPORT
		case TUNER_R848_DVBC:
			attach->tuner_attach = R848_C_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xF8;//0x38, 0xF8
			break;
#endif
#if TUNER_R848_DVBT_SUPPORT
		case TUNER_R848_DVBT:
			attach->tuner_attach = R848_T_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xF8;//0x38, 0xF8
			break;
#endif
#if TUNER_R848_DTMB_SUPPORT
		case TUNER_R848_DTMB:
			attach->tuner_attach = R848_DTMB_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xF8;//0x38, 0xF8
			break;
#endif
#if TUNER_RDA5815M_SUPPORT
		case TUNER_RDA5815M:
			attach->tuner_attach = RDA5815M_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0x18;
			break;
#endif
#if (TUNER_ATBM2040_DVBC_SUPPORT || TUNER_ATBM2040_DTMB_SUPPORT)
		case TUNER_ATBM2040:
			attach->tuner_attach = ATBM2040_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;
			break;
#endif
#if TUNER_ATBM2030_DTMB_SUPPORT
		case TUNER_ATBM2030:
			attach->tuner_attach = ATBM2030_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;
			break;
#endif
#if (TUNER_ATBM253_DVBT_SUPPORT)
		case TUNER_ATBM253_DVBT:
			attach->tuner_attach = ATBM253_DVBT_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;//0x34, 0x74
			break;
#endif
#if (TUNER_ATBM253_DVBC_SUPPORT )
		case TUNER_ATBM253_DVBC:
			attach->tuner_attach = ATBM253_DVBC_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;//0x34, 0x74
			break;
#endif
#if (TUNER_ATBM253_DTMB_SUPPORT )
		case TUNER_ATBM253_DTMB:
			attach->tuner_attach = ATBM253_DTMB_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;//0x34, 0x74
			break;
#endif
#if TUNER_SHARP7306_SUPPORT
		case TUNER_SHARP7306:
			attach->tuner_attach = SHARP7306_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;
			break;
#endif
#if TUNER_RDA5160_DTMB_SUPPORT
		case TUNER_RDA5160_DTMB:
			attach->tuner_attach = RDA5160_DTMB_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;//0xc0, 0xc2, 0xc4, 0xc6
			break;
#endif
#if TUNER_RDA5160_DVBT_SUPPORT
		case TUNER_RDA5160_DVBT:
			attach->tuner_attach = RDA5160_T_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;//0xc0, 0xc2, 0xc4, 0xc6
			break;
#endif
#if TUNER_RDA5160_DVBC_SUPPORT
		case TUNER_RDA5160_DVBC:
			attach->tuner_attach = RDA5160_C_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;//0xc0, 0xc2, 0xc4, 0xc6
			break;
#endif
#if TUNER_MXL683_ISDBT_SUPPORT
		case TUNER_MXL683_ISDBT:
			attach->tuner_attach = NULL;//DEFAULT_TUNER_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0xC0;
			break;
#endif
#if TUNER_TS6011_SUPPORT
		case TUNER_TS6011:
			attach->tuner_attach = TS6011_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0x58;
			break;
#endif
#if TUNER_M3031_SUPPORT
		case TUNER_M3031:
			attach->tuner_attach = M3031_ATTACH;
			if(attach->tuner_dev.addr == APP_FRONTEND_INVALID_TUNER_I2C_ADDR)
				attach->tuner_dev.addr = 0x40;
			break;
#endif
		case TUNER_AUTO_ADAPT:
		default:
#ifdef LINUX_OS
			attach->tuner_attach = 0;
#else
			attach->tuner_attach = NULL;
#endif
			attach->tuner_dev.addr = 0x00;
			return -1;
	}
	return 0;
}

#if TUNER_AUTO_ADAPT_SUPPORT
static int _set_tuner_config(uint8_t tuner, int32_t tuner_index, int32_t addr_index)
{
	if(tuner >= MAX_DEV_ADDR_NUM){
		app_log_error("%s tuner count out of range:%d\n", __func__, tuner);
		return -1;
	}
	switch(tuner){
		case 0:
			GxBus_ConfigSetInt(FE_TUNER1_ADAPT, tuner_index);
			GxBus_ConfigSetInt(FE_TUNER1_ADDR_ADAPT, addr_index);
			break;
		case 1:
			GxBus_ConfigSetInt(FE_TUNER2_ADAPT, tuner_index);
			GxBus_ConfigSetInt(FE_TUNER2_ADDR_ADAPT, addr_index);
			break;
		case 2:
			GxBus_ConfigSetInt(FE_TUNER3_ADAPT, tuner_index);
			GxBus_ConfigSetInt(FE_TUNER3_ADDR_ADAPT, addr_index);
			break;
		case 3:
			GxBus_ConfigSetInt(FE_TUNER4_ADAPT, tuner_index);
			GxBus_ConfigSetInt(FE_TUNER4_ADDR_ADAPT, addr_index);
			break;
		default:
			return -1;
	}
	return 0;
}

int app_get_tuner_config(uint8_t tuner, int32_t *tuner_index, int32_t *addr_index)
{
	if(tuner >= MAX_DEV_ADDR_NUM){
		app_log_error("%s tuner count out of range:%d\n", __func__, tuner);
		return -1;
	}
	switch(tuner){
		case 0:
			GxBus_ConfigGetInt(FE_TUNER1_ADAPT, tuner_index, FE_TUNER_ADAPT_INVALID_INDEX);
			GxBus_ConfigGetInt(FE_TUNER1_ADDR_ADAPT, addr_index, FE_TUNER_ADDR_ADAPT_INVALID_INDEX);
			break;
		case 1:
			GxBus_ConfigGetInt(FE_TUNER2_ADAPT, tuner_index, FE_TUNER_ADAPT_INVALID_INDEX);
			GxBus_ConfigGetInt(FE_TUNER2_ADDR_ADAPT, addr_index, FE_TUNER_ADDR_ADAPT_INVALID_INDEX);
			break;
		case 2:
			GxBus_ConfigGetInt(FE_TUNER3_ADAPT, tuner_index, FE_TUNER_ADAPT_INVALID_INDEX);
			GxBus_ConfigGetInt(FE_TUNER3_ADDR_ADAPT, addr_index, FE_TUNER_ADDR_ADAPT_INVALID_INDEX);
			break;
		case 3:
			GxBus_ConfigGetInt(FE_TUNER4_ADAPT, tuner_index, FE_TUNER_ADAPT_INVALID_INDEX);
			GxBus_ConfigGetInt(FE_TUNER4_ADDR_ADAPT, addr_index, FE_TUNER_ADDR_ADAPT_INVALID_INDEX);
			break;
		default:
			return -1;
	}
	return 0;
}

#define FLAG_NO_TUNER_ADAPT       (0)
#define FLAG_NEED_TUNER_ADAPT     (1)
#define FLAG_TUNER_ADAPT_SUCCESS  (2)
static void _tuner_adapt(uint8_t fe_id, struct app_frontend_config *config, uint8_t adapt_flag)
{
	uint8_t i = 0, j = 0;
	int32_t tuner_index = FE_TUNER_ADAPT_INVALID_INDEX, addr_index = FE_TUNER_ADDR_ADAPT_INVALID_INDEX;
	struct app_frontend_tuner_adapt_params adapt_params_list[] = APP_FRONTEND_TUNER_ADAPT_PARAMETERS;

	app_get_tuner_config(fe_id, &tuner_index, &addr_index);
	if(tuner_index != FE_TUNER_ADAPT_INVALID_INDEX){
		config->dev.info.iq								= adapt_params_list[tuner_index].iq;
		config->dev.info.tuner_config.if_freq_KHz		= adapt_params_list[tuner_index].if_freq;
		config->dev.info.tuner_config.xtal_MHz			= adapt_params_list[tuner_index].xtal;
		config->dev.info.tuner_config.xtal_cap_pf		= adapt_params_list[tuner_index].xtal_cap;
		config->dev.info.tuner_config.clock_out_enable	= adapt_params_list[tuner_index].clock_out;
		config->dev.info.tuner_config.loop_out_enable	= adapt_params_list[tuner_index].loop_out;
	    config->dev.info.tuner_config.connection		= adapt_params_list[tuner_index].connection;
		config->dev.tuner_attach						= adapt_params_list[tuner_index].tuner_attach;
		config->dev.tuner_dev.id						= adapt_params_list[tuner_index].i2c_id;
		if(addr_index < MAX_DEV_ADDR_NUM)
			config->dev.tuner_dev.addr = adapt_params_list[tuner_index].chip_addr_list[addr_index];
		else
			config->dev.tuner_dev.addr = adapt_params_list[tuner_index].chip_addr_list[0];
#if FM_SUPPORT
        if(config->dev.demod_attach == Cygnus_FM_ATTACH)
        {
            fe_id = app_get_fm_tuner_id();
        }
#endif
		if(app_init_demod_tuner(config, fe_id, adapt_flag) < 0){
			app_log_error("%s init frontend%d fail, start tuner adapt\n", __func__, fe_id);
			tuner_index = FE_TUNER_ADAPT_INVALID_INDEX;
		}else{
			return;
		}
	}
	if(tuner_index == FE_TUNER_ADAPT_INVALID_INDEX){
		for(i = 0; i<(sizeof(adapt_params_list)/sizeof(struct app_frontend_tuner_adapt_params)); i++){
			if(fe_id == adapt_params_list[i].frontend_index){
				config->dev.info.iq								= adapt_params_list[i].iq;
				config->dev.info.tuner_config.if_freq_KHz		= adapt_params_list[i].if_freq;
				config->dev.info.tuner_config.xtal_MHz			= adapt_params_list[i].xtal;
				config->dev.info.tuner_config.xtal_cap_pf		= adapt_params_list[i].xtal_cap;
				config->dev.info.tuner_config.clock_out_enable	= adapt_params_list[i].clock_out;
				config->dev.info.tuner_config.loop_out_enable	= adapt_params_list[i].loop_out;
				config->dev.info.tuner_config.connection		= adapt_params_list[i].connection;
				config->dev.tuner_attach						= adapt_params_list[i].tuner_attach;
				config->dev.tuner_dev.id						= adapt_params_list[i].i2c_id;
				for(j = 0; j<MAX_DEV_ADDR_NUM; j++){
					if(adapt_params_list[i].chip_addr_list[j] == 0x00)//invalid addr
						break;
					config->dev.tuner_dev.addr = adapt_params_list[i].chip_addr_list[j];
#if FM_SUPPORT
                    if(config->dev.demod_attach == Cygnus_FM_ATTACH)
                    {
                        fe_id = app_get_fm_tuner_id();
                    }
#endif
					if(app_init_demod_tuner(config, fe_id, adapt_flag) == 0){
						_set_tuner_config(fe_id, i, j);
						return;
					}
				}
			}
		}
	}
	app_log_error("%s frontend%d tuner adapt fail, please check!\n", __func__, fe_id);
}
#endif

#if DEMOD_AUTO_ADAPT_SUPPORT
static int _set_demod_config(uint8_t fe_id, int32_t demod_index, int32_t addr_index)
{
	if(fe_id >= MAX_DEV_ADDR_NUM){
		app_log_error("%s tuner count out of range:%d\n", __func__, fe_id);
		return -1;
	}
	switch(fe_id){
		case 0:
			GxBus_ConfigSetInt(FE_DEMOD1_ADAPT, demod_index);
			GxBus_ConfigSetInt(FE_DEMOD1_ADDR_ADAPT, addr_index);
			break;
		case 1:
			GxBus_ConfigSetInt(FE_DEMOD2_ADAPT, demod_index);
			GxBus_ConfigSetInt(FE_DEMOD2_ADDR_ADAPT, addr_index);
			break;
		case 2:
			GxBus_ConfigSetInt(FE_DEMOD3_ADAPT, demod_index);
			GxBus_ConfigSetInt(FE_DEMOD3_ADDR_ADAPT, addr_index);
			break;
		case 3:
			GxBus_ConfigSetInt(FE_DEMOD4_ADAPT, demod_index);
			GxBus_ConfigSetInt(FE_DEMOD4_ADDR_ADAPT, addr_index);
			break;
		default:
			return -1;
	}
	return 0;
}

int app_get_demod_config(uint8_t fe_id, int32_t *demod_index, int32_t *demod_addr_index)
{
	if(fe_id >= MAX_DEV_ADDR_NUM){
		app_log_error("%s demod count out of range:%d\n", __func__, fe_id);
		return -1;
	}
	switch(fe_id){
		case 0:
			GxBus_ConfigGetInt(FE_DEMOD1_ADAPT, demod_index, FE_DEMOD_ADAPT_INVALID_INDEX);
			GxBus_ConfigGetInt(FE_DEMOD1_ADDR_ADAPT, demod_addr_index, FE_DEMOD_ADDR_ADAPT_INVALID_INDEX);
			break;
		case 1:
			GxBus_ConfigGetInt(FE_DEMOD2_ADAPT, demod_index, FE_DEMOD_ADAPT_INVALID_INDEX);
			GxBus_ConfigGetInt(FE_DEMOD2_ADDR_ADAPT, demod_addr_index, FE_DEMOD_ADDR_ADAPT_INVALID_INDEX);
			break;
		case 2:
			GxBus_ConfigGetInt(FE_DEMOD3_ADAPT, demod_index, FE_DEMOD_ADAPT_INVALID_INDEX);
			GxBus_ConfigGetInt(FE_DEMOD3_ADDR_ADAPT, demod_addr_index, FE_DEMOD_ADDR_ADAPT_INVALID_INDEX);
			break;
		case 3:
			GxBus_ConfigGetInt(FE_DEMOD4_ADAPT, demod_index, FE_DEMOD_ADAPT_INVALID_INDEX);
			GxBus_ConfigGetInt(FE_DEMOD4_ADDR_ADAPT, demod_addr_index, FE_DEMOD_ADDR_ADAPT_INVALID_INDEX);
			break;
		default:
			return -1;
	}
	return 0;
}

static void _demod_auto_adapt_set_config(int demod_index, struct app_frontend_config *config)
{
	demod_adapt_info_ts_t info_ts_list[] = APP_FRONTEND_TS_DAPT_PARAMETERS;
	demod_adapt_info_lnb_t info_lnb_list[] = APP_FRONTEND_LNB_ADAPT_PARAMETERS;

	config->dev.info.pin_conf.TS00						= info_ts_list[demod_index].ts_pin_config_00;
	config->dev.info.pin_conf.TS01						= info_ts_list[demod_index].ts_pin_config_01;
	config->dev.info.pin_conf.TS02						= info_ts_list[demod_index].ts_pin_config_02;
	config->dev.info.pin_conf.TS03						= info_ts_list[demod_index].ts_pin_config_03;
	config->dev.info.pin_conf.TS04						= info_ts_list[demod_index].ts_pin_config_04;
	config->dev.info.pin_conf.TS05						= info_ts_list[demod_index].ts_pin_config_05;
	config->dev.info.pin_conf.TS06						= info_ts_list[demod_index].ts_pin_config_06;
	config->dev.info.pin_conf.TS07						= info_ts_list[demod_index].ts_pin_config_07;
	config->dev.info.pin_conf.TS08						= info_ts_list[demod_index].ts_pin_config_08;
	config->dev.info.pin_conf.TS09						= info_ts_list[demod_index].ts_pin_config_09;
	config->dev.info.pin_conf.TS10						= info_ts_list[demod_index].ts_pin_config_10;
	config->dev.info.pin_conf.TS11						= info_ts_list[demod_index].ts_pin_config_11;
	config->dev.info.lnb_power_gpio						= info_lnb_list[demod_index].lnb_power_gpio;
	config->dev.info.lnb_invert							= info_lnb_list[demod_index].lnb_invert;
	config->dev.info.pol_invert							= info_lnb_list[demod_index].lnb_invert;
	config->dev.info.ts_mode							= info_ts_list[demod_index].ts_mode;
	config->dev.info.lnb_ctrl_enable					= info_lnb_list[demod_index].lnb_ctrl_enable;
	config->dev.info.pol_ctrl_enable					= info_lnb_list[demod_index].pol_ctrl_enable;
	config->dev.info.is_oscillator						= info_ts_list[demod_index].is_oscillator;
	config->dev.info.pin_multiplex_conf.agc_gpio		= info_lnb_list[demod_index].agc_gpio;
	config->dev.info.pin_multiplex_conf.hv_sel_gpio		= info_lnb_list[demod_index].hv_sel_gpio;
	config->dev.info.pin_multiplex_conf.diseqc_out_gpio	= info_lnb_list[demod_index].diseqc_out_gpio;
	config->dev.info.pin_multiplex_conf.diseqc_in_gpio	= info_lnb_list[demod_index].diseqc_in_gpio;
}

static int _demod_adapt(handle_t fe_handle, uint8_t fe_id, struct app_frontend_config *config, uint32_t fe_adapt_flag)
{
	uint8_t i = 0;
	uint8_t j = 0;
	int32_t ret = -1;
	int32_t demod_index = FE_DEMOD_ADAPT_INVALID_INDEX;
	int32_t demod_addr_index = FE_DEMOD_ADAPT_INVALID_INDEX;
	demod_adapt_demod_t demod_adapt_params_list[] = APP_FRONTEND_DEMOD_ADAPT_PARAMETERS;

	app_get_demod_config(fe_id, &demod_index, &demod_addr_index);

	if(demod_index != FE_DEMOD_ADAPT_INVALID_INDEX){
		config->dev.demod_dev.id	= demod_adapt_params_list[demod_index].i2c_id;
		config->dev.demod_attach	= demod_adapt_params_list[demod_index].demod_attach_adapt;
		if(demod_addr_index < MAX_DEV_ADDR_NUM){
			config->dev.demod_dev.addr	= demod_adapt_params_list[demod_index].demod_addr_adapt[demod_addr_index];
		}else{
			config->dev.demod_dev.addr	= demod_adapt_params_list[demod_index].demod_addr_adapt[0];
		}
		_demod_auto_adapt_set_config(demod_index, config);
		ret = _set_device_multi_demod_tuner_adapt(fe_handle, config, fe_adapt_flag);
		if (-1 == ret) {
			demod_index = FE_DEMOD_ADAPT_INVALID_INDEX;
		}else{
			return ret;
		}
	}

	if(FE_DEMOD_ADAPT_INVALID_INDEX == demod_index){
		for(i = 0; i < sizeof(demod_adapt_params_list)/sizeof(demod_adapt_demod_t); i++) {
			if(fe_id == demod_adapt_params_list[i].frontend_index){
				config->dev.demod_dev.id	= demod_adapt_params_list[i].i2c_id;
				config->dev.demod_attach	= demod_adapt_params_list[i].demod_attach_adapt;
				_demod_auto_adapt_set_config(i, config);
				for(j = 0; j < MAX_DEV_ADDR_NUM; j++){
					if(0 == demod_adapt_params_list[i].demod_addr_adapt[j]){
						break;
					}
					config->dev.demod_dev.addr	= demod_adapt_params_list[i].demod_addr_adapt[j];

					ret = _set_device_multi_demod_tuner_adapt(fe_handle, config, fe_adapt_flag);
					if (ret != -1){
						_set_demod_config(fe_id, i, j);
						return ret;
					}
				}
			}
		}
	}

	return ret;
}
#endif

#if (TUNER_AUTO_ADAPT_SUPPORT) || (DEMOD_AUTO_ADAPT_SUPPORT)
static int _set_device_multi_demod_tuner_adapt(handle_t fe_handle, struct app_frontend_config *config, uint32_t fe_adapt_flag)
{
	struct fe_dev_ex_params ex_params;

	memset(&ex_params, 0, sizeof(struct fe_dev_ex_params));
	memcpy(&ex_params.dev, &config->dev, sizeof(struct dev_attach));

	switch(fe_adapt_flag){
		case APP_TUNER_AUTO_ADAPT:
			ex_params.dev.demod_dev.demod_adapt_flag = FE_TUNER_ADAPT;
			break;
		case APP_DEMOD_AUTO_ADAPT:
			ex_params.dev.demod_dev.demod_adapt_flag = FE_DEMOD_ADAPT;
			break;
		case APP_DEMOD_TUNER_AUTO_ADAPT:
			ex_params.dev.demod_dev.demod_adapt_flag = FE_DEMOD_TUNER_ADAPT;
			break;
		default:
			ex_params.dev.demod_dev.demod_adapt_flag = FE_MANUAL_ADJUSTMENT;
			break;
	}

	return ioctl(fe_handle, FE_SET_DEVICE_EX, &ex_params);
}
#endif

void app_frontend_manual_config_init(uint8_t fe_id, struct app_frontend_config *config, unsigned int adapt_flag)
{
	unsigned int   tuner_i2c_id_list[]        = APP_FRONTEND_TUNER_I2C_ID;
	unsigned int   tuner_i2c_chip_addr_list[] = APP_FRONTEND_TUNER_I2C_CHIP_ADDR;
	unsigned int   tuner_if_freq_list[]       = APP_FRONTEND_IF_FREQ;
	unsigned int   tuner_crystal_list[]       = APP_FRONTEND_TUNER_CRYSTAL;
	unsigned int   tuner_xtal_cap_list[]      = APP_FRONTEND_TUNER_XTAL_CAP;
	unsigned int   tuner_clock_out_list[]     = APP_FRONTEND_TUNER_CLOCK_OUT;
	unsigned int   tuner_loop_out_list[]      = APP_FRONTEND_TUNER_LOOP_OUT;
	unsigned int   tuner_connection_list[]    = APP_FRONTEND_TUNER_CONNECTION;
	unsigned int   demod_i2c_id_list[]        = APP_FRONTEND_DEMOD_I2C_ID;
	unsigned int   demod_i2c_chip_addr_list[] = APP_FRONTEND_DEMOD_I2C_CHIP_ADDR;
#ifdef LINUX_OS
	unsigned int   demod_attach_list[]        = APP_FRONTEND_DEMOD_ATTACH;
#else
	demod_attach_func demod_attach_list[]     = APP_FRONTEND_DEMOD_ATTACH;
#endif
	unsigned int   iq_list[]                  = APP_FRONTEND_IQ;
	unsigned int   lnb_power_gpio_list[]      = APP_FRONTEND_LNB_POWER_GPIO;
	unsigned int   lnb_invert_list[]          = APP_FRONTEND_LNB_INVERT;
	unsigned int   polar_invert_list[]        = APP_FRONTEND_POLAR_INVERT;
	ts_pin_t       ts_pin_00_list[]           = APP_FRONTEND_TS_PIN_CONFIG_00;
	ts_pin_t       ts_pin_01_list[]           = APP_FRONTEND_TS_PIN_CONFIG_01;
	ts_pin_t       ts_pin_02_list[]           = APP_FRONTEND_TS_PIN_CONFIG_02;
	ts_pin_t       ts_pin_03_list[]           = APP_FRONTEND_TS_PIN_CONFIG_03;
	ts_pin_t       ts_pin_04_list[]           = APP_FRONTEND_TS_PIN_CONFIG_04;
	ts_pin_t       ts_pin_05_list[]           = APP_FRONTEND_TS_PIN_CONFIG_05;
	ts_pin_t       ts_pin_06_list[]           = APP_FRONTEND_TS_PIN_CONFIG_06;
	ts_pin_t       ts_pin_07_list[]           = APP_FRONTEND_TS_PIN_CONFIG_07;
	ts_pin_t       ts_pin_08_list[]           = APP_FRONTEND_TS_PIN_CONFIG_08;
	ts_pin_t       ts_pin_09_list[]           = APP_FRONTEND_TS_PIN_CONFIG_09;
	ts_pin_t       ts_pin_10_list[]           = APP_FRONTEND_TS_PIN_CONFIG_10;
	ts_pin_t       ts_pin_11_list[]           = APP_FRONTEND_TS_PIN_CONFIG_11;
	ts_mode_t      ts_mode_list[]             = APP_FRONTEND_TS_MODE;
	unsigned int   lnb_ctrl_enable_list[]     = APP_FRONTEND_LNB_CTRL_ENABLE;
	unsigned int   pol_ctrl_enable_list[]     = APP_FRONTEND_POL_CTRL_ENABLE;
	unsigned int   is_oscillator_list[]       = APP_FRONTEND_IS_OSCILLATOR;
	unsigned int   agc_gpio_list[]            = APP_FRONTEND_AGC_GPIO;
	unsigned int   hv_sel_gpio_list[]         = APP_FRONTEND_HV_SEL_GPIO;
	unsigned int   diseqc_out_gpio_list[]     = APP_FRONTEND_DISEQC_OUT_GPIO;
	unsigned int   diseqc_in_gpio_list[]      = APP_FRONTEND_DISEQC_IN_GPIO;

	if((APP_DO_NOT_FE_ADAPT == adapt_flag) || (APP_DEMOD_AUTO_ADAPT == adapt_flag)){
		config->dev.tuner_dev.id     = tuner_i2c_id_list[fe_id];
		config->dev.tuner_dev.addr   = tuner_i2c_chip_addr_list[fe_id];
		config->dev.info.tuner_config.if_freq_KHz      = tuner_if_freq_list[fe_id];
		config->dev.info.tuner_config.xtal_MHz         = tuner_crystal_list[fe_id];
		config->dev.info.tuner_config.xtal_cap_pf      = tuner_xtal_cap_list[fe_id];
		config->dev.info.tuner_config.clock_out_enable = tuner_clock_out_list[fe_id];
		config->dev.info.tuner_config.loop_out_enable  = tuner_loop_out_list[fe_id];
		config->dev.info.tuner_config.connection       = tuner_connection_list[fe_id];
	}

	if((APP_DO_NOT_FE_ADAPT == adapt_flag) || (APP_TUNER_AUTO_ADAPT == adapt_flag)){
		config->dev.demod_dev.id     = demod_i2c_id_list[fe_id];
		config->dev.demod_attach     = demod_attach_list[fe_id];
		config->dev.demod_dev.addr   = demod_i2c_chip_addr_list[fe_id];
		config->dev.info.pin_conf.TS00   = ts_pin_00_list[fe_id];
		config->dev.info.pin_conf.TS01   = ts_pin_01_list[fe_id];
		config->dev.info.pin_conf.TS02   = ts_pin_02_list[fe_id];
		config->dev.info.pin_conf.TS03   = ts_pin_03_list[fe_id];
		config->dev.info.pin_conf.TS04   = ts_pin_04_list[fe_id];
		config->dev.info.pin_conf.TS05   = ts_pin_05_list[fe_id];
		config->dev.info.pin_conf.TS06   = ts_pin_06_list[fe_id];
		config->dev.info.pin_conf.TS07   = ts_pin_07_list[fe_id];
		config->dev.info.pin_conf.TS08   = ts_pin_08_list[fe_id];
		config->dev.info.pin_conf.TS09   = ts_pin_09_list[fe_id];
		config->dev.info.pin_conf.TS10   = ts_pin_10_list[fe_id];
		config->dev.info.pin_conf.TS11   = ts_pin_11_list[fe_id];
		config->dev.info.iq              = iq_list[fe_id];
		config->dev.info.lnb_power_gpio  = lnb_power_gpio_list[fe_id];
		config->dev.info.lnb_invert      = lnb_invert_list[fe_id];
		config->dev.info.pol_invert      = polar_invert_list[fe_id];
		config->dev.info.ts_mode         = ts_mode_list[fe_id];
		config->dev.info.lnb_ctrl_enable = lnb_ctrl_enable_list[fe_id];
		config->dev.info.pol_ctrl_enable = pol_ctrl_enable_list[fe_id];
		config->dev.info.is_oscillator   = is_oscillator_list[fe_id];
		config->dev.info.pin_multiplex_conf.agc_gpio        = agc_gpio_list[fe_id];
		config->dev.info.pin_multiplex_conf.hv_sel_gpio     = hv_sel_gpio_list[fe_id];
		config->dev.info.pin_multiplex_conf.diseqc_out_gpio = diseqc_out_gpio_list[fe_id];
		config->dev.info.pin_multiplex_conf.diseqc_in_gpio  = diseqc_in_gpio_list[fe_id];
	}
}

void app_frontend_config_init(void)
{
	unsigned char i = 0;
	unsigned int adapt_flag;
	struct app_frontend_config config;
	APP_TUNER_TYPE tuner_list[] = APP_FRONTEND_TUNER_LIST;
	APP_DEMOD_ADAPT_TYPE demod_adapt_flag[] = APP_FRONTEND_DEMOD_CONFIG_TYPE_LIST;

	for(i = 0; i < APP_MULTI_DEMOD_NUM; i++){
		if((DEMOD_AUTO_ADAPT == demod_adapt_flag[i]) && (TUNER_AUTO_ADAPT == tuner_list[i])){
			adapt_flag = APP_DEMOD_TUNER_AUTO_ADAPT;
		}else if((DEMOD_AUTO_ADAPT == demod_adapt_flag[i]) && (tuner_list[i] != TUNER_AUTO_ADAPT)){
			adapt_flag = APP_DEMOD_AUTO_ADAPT;
		}else if((demod_adapt_flag[i] != DEMOD_AUTO_ADAPT) && (TUNER_AUTO_ADAPT == tuner_list[i])){
			adapt_flag = APP_TUNER_AUTO_ADAPT;
		}else{
			adapt_flag = APP_DO_NOT_FE_ADAPT;
		}
		app_frontend_manual_config_init(i, &config, adapt_flag);
#ifdef ECOS_OS
		register_frontend();
#endif

#if TUNER_AUTO_ADAPT_SUPPORT
		if(tuner_list[i] == TUNER_AUTO_ADAPT){
			_tuner_adapt(i, &config, adapt_flag);
			continue;
		}
#endif
		if(app_get_tuner_attach(&(config.dev), tuner_list[i]) < 0)
			app_log_error("%s get tuner %d type fail\n", __func__, i);
		app_init_demod_tuner(&config, i, adapt_flag);
	}
}

#if FM_SUPPORT
void app_fm_config_init(uint32_t index)
{
	struct app_frontend_config config;
	demod_attach_func demod_attach[]     = APP_FRONTEND_DEMOD_ATTACH;
	unsigned int   demod_i2c_chip_addr_list[] = APP_FRONTEND_DEMOD_I2C_CHIP_ADDR;
	APP_TUNER_TYPE tuner_list[]               = APP_FRONTEND_TUNER_LIST;
	unsigned int   tuner_i2c_id_list[]   = APP_FRONTEND_TUNER_I2C_ID;
	unsigned int   tuner_xtal_cap[]      = APP_FRONTEND_TUNER_XTAL_CAP;
	unsigned int   tuner_clock_out[]     = APP_FRONTEND_TUNER_CLOCK_OUT;
	unsigned int   tuner_loop_out[]      = APP_FRONTEND_TUNER_LOOP_OUT;
	unsigned int   tuner_connection[]    = APP_FRONTEND_TUNER_CONNECTION;
	unsigned char i = 0;

	memset(&config, 0, sizeof(struct app_frontend_config));
	config.dev.demod_attach     = Cygnus_FM_ATTACH;
	config.dev.info.pin_conf.TS00   = ERRS;
	config.dev.info.pin_conf.TS01   = ERRS;
	config.dev.info.pin_conf.TS02   = ERRS;
	config.dev.info.pin_conf.TS03   = ERRS;
	config.dev.info.pin_conf.TS04   = ERRS;
	config.dev.info.pin_conf.TS05   = ERRS;
	config.dev.info.pin_conf.TS06   = ERRS;
	config.dev.info.pin_conf.TS07   = ERRS;
	config.dev.info.pin_conf.TS08   = ERRS;
	config.dev.info.pin_conf.TS09   = ERRS;
	config.dev.info.pin_conf.TS10   = ERRS;
	config.dev.info.pin_conf.TS11   = ERRS;
	config.dev.info.iq              = 0xFF;
	config.dev.info.lnb_power_gpio  = 0x100;
	config.dev.info.lnb_invert      = 0xFF;
	config.dev.info.pol_invert      = 0xFF;
	config.dev.info.ts_mode         = 0xF;
	config.dev.info.lnb_ctrl_enable = 0xFF;
	config.dev.info.pol_ctrl_enable = 0xFF;
	config.dev.info.is_oscillator   = 0xF;
	config.dev.info.pin_multiplex_conf.agc_gpio        = 0xFF;
	config.dev.info.pin_multiplex_conf.hv_sel_gpio     = 0xFF;
	config.dev.info.pin_multiplex_conf.diseqc_out_gpio = 0xFF;
	config.dev.info.pin_multiplex_conf.diseqc_in_gpio  = 0xFF;
#ifdef ECOS_OS
	register_frontend();
#endif

	for(i = 0; i<APP_MULTI_DEMOD_NUM; i++)
	{
		if(demod_attach[i] == Cygnus_DVBT_ATTACH)
		{
			config.dev.demod_dev.addr   = demod_i2c_chip_addr_list[i];
			config.dev.tuner_dev.id     = tuner_i2c_id_list[i];
			config.dev.info.tuner_config.xtal_cap_pf      = tuner_xtal_cap[i];
			config.dev.info.tuner_config.clock_out_enable = tuner_clock_out[i];
			config.dev.info.tuner_config.loop_out_enable  = tuner_loop_out[i];
			config.dev.info.tuner_config.connection       = tuner_connection[i];
#if TUNER_AUTO_ADAPT_SUPPORT
			if(tuner_list[i] == TUNER_AUTO_ADAPT)
			{
				_tuner_adapt(i, &config, 1);
				return;
			}
#endif
			if(app_get_tuner_attach(&(config.dev), tuner_list[i]) < 0)
				app_log_error("%s get tuner %s type fail\n", __func__, tuner_list[i]);
			app_init_demod_tuner(&config, index, 0);
		}
	}

}
#endif
#endif

