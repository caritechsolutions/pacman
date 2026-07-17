#ifdef NO_OS
#include "gxloader.h"
#include "io.h"
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#endif

#include "common.h"
#include "gxdlp_api.h"
#include "gui.h"

#include "frontend/frontend.h"
#include "frontend/demod.h"
#include "frontend/tuner.h"
#include "dvbhal/gxfe_hal.h"
#include "fe.h"
#include "progress.h"
#include "config.h"
#include "dlp_internal.h"

static int frontend_init(void)
{
	struct dev_attach dev;
	handle_t fe_handle;
	unsigned int   demod_ts_mode = PARALLEL; // 0,ts_parallel_mode  1, ts_serial_mode
	unsigned int   demod_iq_mode = 0; //   0, swap_iq   1, no_swap_iq
	int demod_tsout_pin = 0;
	int polar_invert = 0;
	int demod_is_oscillator = 0;
	GxDLP_Demod_Type  demod_type = 0;
	struct demod *p_demod;
	unsigned char  demod_i2c_chipaddr = 0;
	GxDLP_Tuner_Type   tuner_type = 0;
	struct tuner *p_tuner;
	unsigned char  tuner_i2c_chipaddr = 0;
	int tuner_osc_fre = 0;
	int lnb_gpio = 0;
	int lnb_invert = 0;
	struct pin_config demod_pin_map;
	int tuner_xtal_cap;

	if (GxDLP_Get_FeDemodType(&demod_type) == -1) {
		gxloge("get demod type failed\n");
		return -1;
	}
	if (GxDLP_Get_FeTunerType(&tuner_type) == -1) {
		gxloge("get tuner type failed\n");
		return -1;
	}

	p_demod = demod_type_to_demod(demod_type);
	p_tuner = tuner_type_to_tuner(tuner_type);

	dev.demod_attach = p_demod->attach;
	demod_i2c_chipaddr = p_demod->i2c_chipaddr;
	dev.tuner_attach = p_tuner->attach;
	tuner_i2c_chipaddr = p_tuner->i2c_chipaddr;

	if ((int32_t)(dev.demod_dev.id = GxDLP_Get_FeDemodDevId()) == -1)
		dev.demod_dev.id = 0;
	if ((int32_t)(dev.demod_dev.addr = GxDLP_Get_FeDemodI2cAddr()) == -1)
		dev.demod_dev.addr = demod_i2c_chipaddr;
	if ((int32_t)(dev.tuner_dev.id = GxDLP_Get_FeTunerDevId()) == -1)
		dev.tuner_dev.id = 0;
	if ((int32_t)(dev.tuner_dev.addr = GxDLP_Get_FeTunerI2cAddr()) == -1)
		dev.tuner_dev.addr = tuner_i2c_chipaddr;
	if ((int32_t)(demod_iq_mode = GxDLP_Get_FeDemodIqMode()) == -1)
		demod_iq_mode = 0;
	if ((int32_t)(demod_tsout_pin = GxDLP_Get_FeDemodTsPin()) == -1)
		demod_tsout_pin = 2;
	if ((int32_t)(demod_is_oscillator = GxDLP_Get_FeDemodIsOscillator()) == -1)
		demod_is_oscillator = -1;
	if ((int32_t)(tuner_osc_fre = GxDLP_Get_FeTunerOscFre()) == -1)
		tuner_osc_fre = 0;
	if ((int32_t)(lnb_gpio = GxDLP_Get_FePolarGpio()) == -1)
		lnb_gpio = -1;
	if ((int32_t)(lnb_invert = GxDLP_Get_FeLnbInvert()) == -1)
		lnb_invert = -1;
	register_frontend();
	fe_handle = open("/dev/gxfe0", O_RDWR);
	if (fe_handle < 0) {
		gxloge("register frontend failed\n");
		return -1;
	}
	polar_invert = GxDLP_Get_FePolarInvert();
	ioctl(fe_handle, FE_SET_DEVICE, &dev);
	if (lnb_gpio != -1)
		ioctl(fe_handle, FE_SET_POLAR_GPIO, (void *)lnb_gpio);
	if (lnb_invert != -1)
		ioctl(fe_handle, FE_SET_LNB_INVERT, (void *)lnb_invert);
	ioctl(fe_handle, FE_SET_POLAR_INVERT, (void *)polar_invert);
	if (GxDLP_Get_FeDemodTsMode(&demod_ts_mode) == 0)
		ioctl(fe_handle, FE_SET_TS_MODE, (void *)demod_ts_mode);
	ioctl(fe_handle, FE_SET_TSOUT_PIN, (void *)(demod_tsout_pin));
	if (GxDLP_Get_FeDemodTsPinMap(&demod_pin_map) == 0)
		ioctl(fe_handle, FE_SET_TSPIN_MAP, &demod_pin_map);
	if (demod_type == GXDLP_DEMOD_ATBM888X_DTMB) {
		if (tuner_type == GXDLP_TUNER_R836_DTMB)
			demod_iq_mode = 1;
		else
			demod_iq_mode = 0;
	}
	if (demod_is_oscillator != -1)
		ioctl(fe_handle, FE_SET_IS_OSCILLATOR, (void *)demod_is_oscillator);
	ioctl(fe_handle, FE_SET_IQ_SWAP, (void *)demod_iq_mode);
	if (tuner_osc_fre > 0)
	{
		/*
		* 硬件设计，tuner使用的晶振，与驱动默认晶振值不一样
		*/
		ioctl(fe_handle, FE_SET_TUNER_XTAL_OUT, (void *)tuner_osc_fre);
	}
	if ((int32_t)(tuner_xtal_cap = GxDLP_Get_FeTunerXtalCap()) >= 0)
		ioctl(fe_handle, FE_SET_TUNER_XTAL_CAP, (void *)tuner_xtal_cap);
	/* after set all param, then reinit */
	ioctl(fe_handle, FE_SET_REINIT, NULL);

	gxlogd("Frontend tuner dev id : %d\n", dev.tuner_dev.id);
	gxlogd("Frontend tuner i2c addr : 0x%x\n", dev.tuner_dev.addr);
	gxlogd("Frontend demod dev id : %d\n", dev.demod_dev.id);
	gxlogd("Frontend demod i2c addr : 0x%x\n", dev.demod_dev.addr);
	gxlogd("Frontend Polar invert : %d\n", polar_invert);
	gxlogd("Frontend demod ts mode : %d\n", demod_ts_mode);
	gxlogd("Frontend demod iq mode : %d\n", demod_iq_mode);
	gxlogd("Frontend demod ts pin : %d\n", demod_tsout_pin);
	gxlogd("Frontend demod isoscillator : %d\n", demod_is_oscillator);
	gxlogd("Frontend tuner osc fre: %d\n", tuner_osc_fre);
	gxlogd("Frontend lnb gpio : %d\n", lnb_gpio);
	gxlogd("Frontend lnb invert : %d\n", lnb_invert);

	return 0;
}

int frontend_set(void)
{
	int    frontend;
	int    counter;
	GxFeTpParameters tp_param;
	GxFeStatus         fe_status;
	unsigned int repeat_times;
	GxFeDiseqcInfo info;
	GxFeVoltage voltage;
	GxFeToneMode tone_mode;

	update_progress_status(GXUPDATE_CONFIG);

	memset(&tp_param, 0x00, sizeof(GxFeTpParameters));
	frontend_init();
	repeat_times = GxDLP_Get_FeRepeatTimes();
	frontend = GxFeOpen(0);
	GxDLP_Get_FeType(&tp_param.type);
	tp_param.frequency_KHz = GxDLP_Get_FeFrequency();

	if (tp_param.type == FE_DVB_S || tp_param.type == FE_DVB_S2) {
		tp_param.u.dvbs_param.symbol_rate_Kbps = GxDLP_Get_FeSymbolrate();
		tp_param.u.dvbs_param.pls_n = 0;
		GxDLP_Get_FeModulation(&tp_param.u.dvbs_param.modulation);
	} else if (tp_param.type == FE_DVB_C || tp_param.type == FE_DVB_C2) {
		tp_param.u.dvbc_param.symbol_rate_Kbps = GxDLP_Get_FeSymbolrate();
		GxDLP_Get_FeModulation(&tp_param.u.dvbc_param.modulation);
		tp_param.u.dvbc_param.timeout_ms = 1000;
	} else if (tp_param.type == FE_DVB_T || tp_param.type == FE_DVB_T2) {
		tp_param.u.dvbt_param.bandwidth = GxDLP_Get_FeBandwidth();
		memset(&tp_param.u.dvbt_param.plp_param, 0x00, sizeof(tp_param.u.dvbt_param.plp_param));
		tp_param.u.dvbt_param.work_mode = DVB_T_T2_AUTO;
		GxDLP_Get_FeModulation(&tp_param.u.dvbt_param.modulation);
		tp_param.u.dvbt_param.plp_mode = SET_AUTO_PLP;
		tp_param.u.dvbt_param.guard_interval = GUARD_INTERVAL_AUTO;
	} else if (tp_param.type == FE_DTMB) {
		tp_param.u.dtmb_param.bandwidth = GxDLP_Get_FeBandwidth();
	} else if (tp_param.type == FE_ATSC) {
		tp_param.u.atsc_param.symbol_rate_Kbps = GxDLP_Get_FeSymbolrate();
		tp_param.u.atsc_param.bandwidth = GxDLP_Get_FeBandwidth();
		GxDLP_Get_FeModulation(&tp_param.u.atsc_param.modulation);
	} else if (tp_param.type == FE_ISDB_T) {
		tp_param.u.isdbt_param.bandwidth = GxDLP_Get_FeBandwidth();
	}


	GxDLP_Get_FeSat22k(&tone_mode.tone);
	GxDLP_Get_FePolarity(&voltage.polar);

	GxFeSetVoltage(frontend, voltage);
	GxFeSetTone(frontend, tone_mode);

	info.type = GXFEHAL_DISEQC10;
	info.u.d10.tone = tone_mode.tone;
	info.u.d10.port = GxDLP_Get_FeDiseqc10_Port();
	if (info.u.d10.port < GXFEHAL_DISEQC_PORT01)
		info.u.d10.port = GXFEHAL_DISEQC_PORT01;
	info.u.d10.polar = voltage.polar;
	GxFeSetDiseqc(frontend, info);

	info.type = GXFEHAL_DISEQC11;
	info.u.d11.port = GxDLP_Get_FeDiseqc11_Port();
	if (info.u.d11.port < GXFEHAL_DISEQC_PORT01)
		info.u.d11.port = GXFEHAL_DISEQC_PORT01;
	GxFeSetDiseqc(frontend, info);

	GxFeLockTp(frontend, tp_param);

	gxlogd("Frontend Type : %d\n", tp_param.type);
	gxlogd("Frontend Frequency : %d\n", tp_param.frequency_KHz);
	gxlogd("Frontend Symbolrate : %d\n", GxDLP_Get_FeSymbolrate());
	gxlogd("Frontend Sat22k : %d\n", tone_mode.tone);
	gxlogd("Frontend Polar : %d\n", voltage.polar);
	gxlogd("Frontend Diseqc10 Port : %d\n", GxDLP_Get_FeDiseqc10_Port());
	gxlogd("Frontend Diseqc11 Port : %d\n", GxDLP_Get_FeDiseqc11_Port());
	gxlogd("Frontend bandwidth : %d\n", GxDLP_Get_FeBandwidth());
	gxlogd("Frontend repeat_times: %d\n", repeat_times);

	repeat_times *= 10;

	fe_status.status = GXFEHAL_UNLOCK;
	for (counter = 0; repeat_times == 0xFFFFFFFF || counter < repeat_times; counter++) {
		GxFeGetStatus(frontend, &fe_status);

		GxCore_ThreadDelay(100);
		if (GXFEHAL_LOCK == fe_status.status) {
			update_progress_status(GXUPDATE_CONFIG_OK);
			return 0;
		}
	}
	update_progress_error(GXUPDATE_FRONTEND_UNLOCK);
	return -1;
}

