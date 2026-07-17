#include "app.h"
#include "app_module.h"
#include "app_wnd_search.h"
#include "app_send_msg.h"
#include "module/app_frontend.h"
#include "module/app_frontend_init.h"
#include "app_default_params.h"
#include "app_pop.h"
#include <math.h>
#include "app_windows.h"
#if !OTT_SUPPORT
#include "module/app_frontend_board_config.h"
#endif
#if PDMX_SUPPORT
#include "app_pdmx.h"
#endif

#define   MAX_ANGLE_LONGITUDE               36000
#define   MAX_LONGITUDE                     18000
#define   HALF_MAX_LONGITUDE                9000
#define   MAX_LATITUDE                      9000
#define   MIN_LATITUDE                      0
#define   MAX_ANGLE_STAB_USALS              16000
#define   RADIUS_OF_EARTH                   6378
#define   RADIUS_OF_GEO_ORBIT               42164//42164.2
#define   QUADATE_RADIUS_OF_EARTH           40678884
#define   QUADATE_RADIUS_OF_GEO_ORBIT       1.7778028964e9// 3.555639523e9
#define   PI                                3.141592654
#define   COM_SIN			sin    //sin计算
#define   COM_SQRT		sqrt    // sqrt计算
#define   COM_COS			cos   //cos计算
#define   COM_ACOS		acos    //acos计算

#define APP_FRONTEND_LNBF_FRE_MIN    (250)//MHz
#define APP_FRONTEND_LNBF_FRE_MAX    (2600)//MHz
#define APP_FRONTEND_NORMAL_FRE_MIN  (915)//MHz
#define APP_FRONTEND_NORMAL_FRE_MAX  (2185)//MHz


static uint32_t s_CurTuner = 0;
uint32_t app_tuner_cur_tuner_get(void)
{
	return s_CurTuner;
}

void app_tuner_cur_tuner_set(uint32_t CurTuner)
{
	s_CurTuner = CurTuner;
}

bool app_frontend_dvbs_frequency_check(uint32_t frequency)
{
    int lnbf_tag = 0;
    unsigned int fre_min = 0, fre_max = 0;

    GxBus_ConfigGetInt(GXBUS_FRONTEND_LNBF, &lnbf_tag, GXBUS_FRONTEND_LNBF_OFF);
    if(lnbf_tag == GXBUS_FRONTEND_LNBF_ON){
        fre_min = APP_FRONTEND_LNBF_FRE_MIN;
        fre_max = APP_FRONTEND_LNBF_FRE_MAX;
    }else{
        fre_min = APP_FRONTEND_NORMAL_FRE_MIN;
        fre_max = APP_FRONTEND_NORMAL_FRE_MAX;
    }

    app_log_debug("\033[31m##########lnbf tag: %d, fre: %d#########\033[0m\n", lnbf_tag, frequency);
    if(frequency >= fre_min && frequency <= fre_max)
        return true;

    return false;
}

#if NDEMOD_WITH_1TUNER && (APP_FRONTEND_PIN_SWITCH_COUNT || APP_FRONTEND_DEMOD_ADAPT_PIN_SWITCH_COUNT)
static void _frontend_polar_off(int last_tuner,int tuner)
{
    unsigned char i = 0;
    unsigned char tuner_mode_list[] = APP_FRONTEND_MODE_CONFIG;
    AppFrontend_PolarState Polar = SEC_VOLTAGE_OFF;

    if(last_tuner == 0xffff)
    {
        for(i=0; i < APP_MULTI_DEMOD_NUM; i++)
        {
            if(tuner_mode_list[i] == DEMOD_TYPE_DVBS)
            {
                if(i != tuner)
                    app_ioctl(i, FRONTEND_POLAR_SET, (void*)&Polar);
            }
        }
        return;
    }
    if(tuner_mode_list[last_tuner] ==  DEMOD_TYPE_DVBS)
        app_ioctl(last_tuner, FRONTEND_POLAR_SET, (void*)&Polar);
    return;
}

static void _frontend_ts_onoff(int tuner)
{
	int frontend = 0;
	unsigned char i = 0;
	unsigned char ts4pin_count = 0;
	struct AppPinSwitch *p_ts4pin = NULL;

	APP_DEMOD_ADAPT_TYPE demod_adapt_flag[] = APP_FRONTEND_DEMOD_CONFIG_TYPE_LIST;
	if(DEMOD_AUTO_ADAPT == demod_adapt_flag[tuner]){
#if DEMOD_AUTO_ADAPT_SUPPORT
		if(!APP_FRONTEND_DEMOD_ADAPT_PIN_SWITCH_COUNT){
			printf("[%s:%d],de,pd adapt pin switch count is 0\n", __func__,__LINE__);
			return;
		}
		int32_t demod_index = FE_DEMOD_ADAPT_INVALID_INDEX;
		int32_t demod_addr_index = FE_DEMOD_ADAPT_INVALID_INDEX;
		demod_adapt_ts4pin_switch_t demod_adapt_ts4pin_switch_list[] = APP_FRONTEND_DEMOD_ADAPT_TS4PIN_SWITCH;
		extern int app_get_demod_config(uint8_t fe_id, int32_t *demod_index, int32_t *demod_addr_index);
		app_get_demod_config(tuner, &demod_index, &demod_addr_index);
		if(FE_DEMOD_ADAPT_INVALID_INDEX == demod_index){
			printf("[%s:%d], get demod index invalid\n",__func__,__LINE__);
			return;
		}
		ts4pin_count = demod_adapt_ts4pin_switch_list[demod_index].ts4pin_count;
		if(!ts4pin_count){
			app_log_debug("\033[34m[%s:%d], fe%d demod%d ts4pin switch count is 0\033[0m\n",__FUNCTION__,__LINE__, tuner, demod_index+1);
			return;
		}
		p_ts4pin = demod_adapt_ts4pin_switch_list[demod_index].ts4pin;
#endif
	}else{
		struct AppPinSwitch pin_switch_list[APP_MULTI_DEMOD_NUM][APP_FRONTEND_TS4PIN_SWITCH_COUNT] = APP_FRONTEND_TS4PIN_SWITCH;
		unsigned char ts4pin_switch_count_list[] = APP_FRONTEND_TS4PIN_SWITCH_COUNT_LIST;

		ts4pin_count = ts4pin_switch_count_list[tuner];
		if(!ts4pin_count){
			app_log_debug("\033[34m[%s:%d], fe%d ts4pin switch count is 0\033[0m\n",__FUNCTION__,__LINE__, tuner);
			return;
		}
		p_ts4pin = pin_switch_list[tuner];
	}

	for(i=0; i<ts4pin_count; i++){
		frontend = GxFrontend_IdToHandle(p_ts4pin[i].tuner-1);//frontend id -1 = tuner_id
		if(ioctl(frontend, FE_TS_OUTPUT_CONTRL, p_ts4pin[i].onoff))
			app_log_error("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
	}
}

void app_pin_func_switch(int tuner)
{
	static int last_tuner = 0xffff;
	unsigned char i = 0;
	MulPin *pin_list_p = NULL;
	unsigned char pin_switch_count = 0;

	APP_DEMOD_ADAPT_TYPE demod_adapt_flag[] = APP_FRONTEND_DEMOD_CONFIG_TYPE_LIST;
	if(DEMOD_AUTO_ADAPT == demod_adapt_flag[tuner]){
#if DEMOD_AUTO_ADAPT_SUPPORT
		if(!APP_FRONTEND_DEMOD_ADAPT_PIN_SWITCH_COUNT){
			return;
		}
		int32_t demod_index = FE_DEMOD_ADAPT_INVALID_INDEX;
		int32_t demod_addr_index = FE_DEMOD_ADAPT_INVALID_INDEX;
		demod_adapt_pin_switch_t demod_adapt_pin_switch_list[] = APP_FRONTEND_DEMOD_ADAPT_PIN_SWITCH_PARAMETERS;
		extern int app_get_demod_config(uint8_t fe_id, int32_t *demod_index, int32_t *demod_addr_index);
		app_get_demod_config(tuner, &demod_index, &demod_addr_index);
		if(FE_DEMOD_ADAPT_INVALID_INDEX == demod_index){
			app_log_debug("\033[34m[%s:%d], get demod index invalid\033[0m\n",__FUNCTION__,__LINE__);
			return;
		}
		pin_switch_count = demod_adapt_pin_switch_list[demod_index].pin_count;
		if(!pin_switch_count){
			app_log_debug("\033[34m[%s:%d], fe%d demod%d pin switch count is 0\033[0m\n",__FUNCTION__,__LINE__, tuner, demod_index+1);
			return;
		}
		pin_list_p = demod_adapt_pin_switch_list[demod_index].pin_data;
#endif
	}else{
		if(!APP_FRONTEND_PIN_SWITCH_COUNT){
			app_log_debug("\033[34m[%s:%d], app frontend pin switch count is 0\033[0m\n",__FUNCTION__,__LINE__);
			return;
		}
		MulPin pin_list[APP_MULTI_DEMOD_NUM][APP_FRONTEND_PIN_SWITCH_COUNT] = APP_FRONTEND_PIN_SWITCH_PARAMETERS;
		unsigned char pin_switch_count_list[] = APP_FRONTEND_PIN_SWITCH_COUNT_LIST;
		pin_switch_count = pin_switch_count_list[tuner];
		if(!pin_switch_count){
			app_log_debug("\033[34m[%s:%d], fe%d pin switch count is 0\033[0m\n",__FUNCTION__,__LINE__, tuner);
			return;
		}
		pin_list_p = pin_list[tuner];
	}

	if(tuner != last_tuner){
		_frontend_polar_off(last_tuner, tuner);
		app_log_debug("\033[34m[%s]last_tuner=%d\033[0m\n",__FUNCTION__,last_tuner);
		last_tuner = tuner;
		_frontend_ts_onoff(tuner);
		for(i=0; i< pin_switch_count; i++)
			app_gpio_pin_fun_switch(pin_list_p[i]);
	}
}
#endif

static uint32_t diseqcTuner = 0;

void app_diseqc12_set_tuner_id(int id)
{
	diseqcTuner = id;
}

uint32_t app_diseqc12_get_tuner_id(void)
{
	return diseqcTuner;
}


void app_lnb_freq_sel_to_data(uint16_t cur_sel, GxBusPmDataSat *p_sat)
{
#if UNICABLE_SUPPORT
	if(p_sat->sat_s.unicable_para.type != GXBUS_PM_SAT_UNICABLE_OFF)
	{
		switch (cur_sel)
		{
			case ID_UNICANBLE_9750_10600:
				p_sat->sat_s.lnb1 = 9750;
				p_sat->sat_s.lnb2 = 10600;
				break;
			case ID_UNICANBLE_9750_10700:
				p_sat->sat_s.lnb1 = 9750;
				p_sat->sat_s.lnb2 = 10700;
				break;
			case ID_UNICANBLE_9750_10750:
				p_sat->sat_s.lnb1 = 9750;
				p_sat->sat_s.lnb2 = 10750;
				break;
			case ID_UNICANBLE_9750:
				p_sat->sat_s.lnb1 = 9750;
				p_sat->sat_s.lnb2 = 0;
				break;
			/*case ID_UNICANBLE_10000:
				p_sat->sat_s.lnb1 = 10000;
				p_sat->sat_s.lnb2 = 0;
				break;*/
			case ID_UNICANBLE_10600:
				p_sat->sat_s.lnb1 = 10600;
				p_sat->sat_s.lnb2 = 0;
				break;
			/*case ID_UNICANBLE_10700:
				p_sat->sat_s.lnb1 = 10700;
				p_sat->sat_s.lnb2 = 0;
				break;
			case ID_UNICANBLE_10750:
				p_sat->sat_s.lnb1 = 10750;
				p_sat->sat_s.lnb2 = 0;
				break;
			case ID_UNICANBLE_11250:
				p_sat->sat_s.lnb1 = 11250;
				p_sat->sat_s.lnb2 = 0;
				break;*/
			case ID_UNICANBLE_11300:
				p_sat->sat_s.lnb1 = 11300;
				p_sat->sat_s.lnb2 = 0;
				break;
		}
		app_log_debug("unicable lnb1=%d,lnb2=%d\n",p_sat->sat_s.lnb1,p_sat->sat_s.lnb2);
		return;
	}
#endif
	switch (cur_sel)
	{
		case ID_9750_10600:
			p_sat->sat_s.lnb1 = 9750;
			p_sat->sat_s.lnb2 = 10600;
			break;
		case ID_9750_10700:
			p_sat->sat_s.lnb1 = 9750;
			p_sat->sat_s.lnb2 = 10700;
			break;
		case ID_9750_10750:
			p_sat->sat_s.lnb1 = 9750;
			p_sat->sat_s.lnb2 = 10750;
			break;
#if LNBF_SUPPORT
		case ID_10250_11400:
			p_sat->sat_s.lnb1 = GXBUS_FRONTEND_LNBF_LOW_FREQ_MHZ;
			p_sat->sat_s.lnb2 = GXBUS_FRONTEND_LNBF_HIGH_FREQ_MHZ;
			break;
		case ID_11400_10250:
			p_sat->sat_s.lnb1 = GXBUS_FRONTEND_LNBF_HIGH_FREQ_MHZ;
			p_sat->sat_s.lnb2 = GXBUS_FRONTEND_LNBF_LOW_FREQ_MHZ;
			break;
#endif
		/*case 3:
			p_sat->sat_s.lnb1 = 9750;
			p_sat->sat_s.lnb2 = 11300;
			break;*/
		case ID_5150_5750:
			p_sat->sat_s.lnb1 = 5150;
			p_sat->sat_s.lnb2 = 5750;
			break;
		case ID_5750_5150:
			p_sat->sat_s.lnb1 = 5750;
			p_sat->sat_s.lnb2 = 5150;
			break;
		case ID_5150:
			p_sat->sat_s.lnb1 = 5150;
			p_sat->sat_s.lnb2 = 0;
			break;
		case ID_5750:
			p_sat->sat_s.lnb1 = 5750;
			p_sat->sat_s.lnb2 = 0;
			break;
		case ID_5950:
			p_sat->sat_s.lnb1 = 5950;
			p_sat->sat_s.lnb2 = 0;
			break;
		case ID_9750:
			p_sat->sat_s.lnb1 = 9750;
			p_sat->sat_s.lnb2 = 0;
			break;
		case ID_10000:
			p_sat->sat_s.lnb1 = 10000;
			p_sat->sat_s.lnb2 = 0;
			break;
		case ID_10600:
			p_sat->sat_s.lnb1 = 10600;
			p_sat->sat_s.lnb2 = 0;
			break;
		case ID_10700:
			p_sat->sat_s.lnb1 = 10700;
			p_sat->sat_s.lnb2 = 0;
			break;
		case ID_10750:
			p_sat->sat_s.lnb1 = 10750;
			p_sat->sat_s.lnb2 = 0;
			break;
		case ID_11250:
			p_sat->sat_s.lnb1 = 11250;
			p_sat->sat_s.lnb2 = 0;
			break;
		case ID_11300:
			p_sat->sat_s.lnb1 = 11300;
			p_sat->sat_s.lnb2 = 0;
			break;
#if  UNICABLE_SUPPORT
		case ID_UNICABLE:
			p_sat->sat_s.lnb1 = 9750;
			p_sat->sat_s.lnb2 = 10600;
			break;
		case ID_UNICABLE_2:
			p_sat->sat_s.lnb1 = 9750;
			p_sat->sat_s.lnb2 = 10600;
			break;
#endif
		default:
			break;
	}
}

void app_lnb_freq_data_to_sel(GxBusPmDataSat *p_sat, uint32_t *p_sel)
{
	if (p_sat->sat_s.lnb1 == 0 && p_sat->type == GXBUS_PM_SAT_S)
	{
		app_log_error("[Antenna Setting] sat param err\n");
		return;
	}
#if UNICABLE_SUPPORT
	if(p_sat->sat_s.unicable_para.type != GXBUS_PM_SAT_UNICABLE_OFF)
	{
        if(GXBUS_PM_SAT_UNICABLE == p_sat->sat_s.unicable_para.type)
            *p_sel = 15;
        else
            *p_sel = 16;
		return;
	}
#endif
	if(p_sat->sat_s.lnb2 == 0 ||p_sat->sat_s.lnb2 == p_sat->sat_s.lnb1)
	{
		switch(p_sat->sat_s.lnb1)
		{
			case 5150:	*p_sel=5;		break;
			case 5750:	*p_sel=6;		break;
			case 5950:	*p_sel=7;		break;
			case 9750:	*p_sel=8;		break;
			case 10000:	*p_sel=9;	break;
			case 10600:	*p_sel=10;	break;
			case 10700:	*p_sel=11;	break;
			case 10750:	*p_sel=12;	break;
			case 11250:	*p_sel=13;	break;
			case 11300:	*p_sel=14;	break;
			default:
				*p_sel = 0;
				app_log_error("[dishset] err1\n");	break;
		}
	}
	else
	{
		switch(p_sat->sat_s.lnb2)
		{
#if LNBF_SUPPORT
			case GXBUS_FRONTEND_LNBF_HIGH_FREQ_MHZ: *p_sel=3;break;
			case GXBUS_FRONTEND_LNBF_LOW_FREQ_MHZ:  *p_sel=4;break;
			case 5750:	*p_sel=5;		break;
			case 5150:	*p_sel=6;		break;
#else
			case 5750:	*p_sel=3;		break;
			case 5150:	*p_sel=4;		break;
#endif

			case 10600:	*p_sel=0;		break;
			case 10700:	*p_sel=1;		break;
			case 10750:	*p_sel=2;		break;
			default:
				*p_sel = 0;
				app_log_error("[dishset] err2\n");	break;
		}
	}
}

//transfer pm to frontend
uint32_t app_show_to_sat_22k(GxBusPmSat22kSwitch switch_22K, GxBusPmDataTP *p_tp)
{
#define DIVERSION_FREQUENCY     (11700)
	static uint32_t tone_22k = 0;

	switch (switch_22K)
	{
		case GXBUS_PM_SAT_22K_OFF:
			tone_22k = SEC_TONE_OFF;
			break;

		case GXBUS_PM_SAT_22K_ON:
			tone_22k = SEC_TONE_ON;
			break;

		default:
			if(p_tp->frequency >= DIVERSION_FREQUENCY)
			{
				tone_22k = SEC_TONE_ON;
			}
			else
			{
			tone_22k = SEC_TONE_OFF;
			}
			break;
	}

	return tone_22k;
}

uint32_t app_show_to_tp_freq(GxBusPmDataSat *p_sat, GxBusPmDataTP *p_tp)
{
#define DIVERSION_FREQUENCY     (11700)
	uint16_t lnb_fre = 0;
	uint32_t fre=0;
	GxBusPmSatLnbType type = 0;

	if((p_sat->sat_s.lnb2==0) || (p_sat->sat_s.lnb1 == p_sat->sat_s.lnb2))
	{
		type = GXBUS_PM_SAT_LNB_USER;
	}
	else if((p_sat->sat_s.lnb1==GXBUS_FRONTEND_LNBF_LOW_FREQ_MHZ) || (p_sat->sat_s.lnb1==GXBUS_FRONTEND_LNBF_HIGH_FREQ_MHZ))
	{
		type = GXBUS_PM_SAT_LNBF;
	}
	else if(p_sat->sat_s.lnb1<6000)
	{
		type = GXBUS_PM_SAT_LNB_OCS;
	}
	else if(p_sat->sat_s.lnb1>6000)
	{
		type = GXBUS_PM_SAT_LNB_UNIVERSAL;
	}

	switch(type)
	{
		case GXBUS_PM_SAT_LNB_USER:
			lnb_fre = p_sat->sat_s.lnb1;
			break;

		case GXBUS_PM_SAT_LNB_UNIVERSAL:
			if (p_tp->frequency >= DIVERSION_FREQUENCY)
			{
				lnb_fre = p_sat->sat_s.lnb2;
			}
			else
			{
				lnb_fre = p_sat->sat_s.lnb1;
			}
            break;

		case GXBUS_PM_SAT_LNBF:
		case GXBUS_PM_SAT_LNB_OCS:
			switch(p_tp->tp_s.polar)
			{
				case GXBUS_PM_TP_POLAR_H:
					lnb_fre = p_sat->sat_s.lnb1;
					break;

				case GXBUS_PM_TP_POLAR_V:
					lnb_fre = p_sat->sat_s.lnb2;
					break;

				case GXBUS_PM_TP_POLAR_AUTO:
					lnb_fre = p_sat->sat_s.lnb1;
					break;

				default:
					return GXCORE_ERROR;
					break;
			}
			break;

		default:
			#ifdef GX_BUS_SEARCH_DBUG
			GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---sat lnb type err!!\n");
			#endif
			return GXCORE_ERROR;
			break;
	}
	//if(lnb_fre >= p_tp->frequency)
	if(lnb_fre < 6000)
	{
		fre = lnb_fre - p_tp->frequency;
	}
	else
	{
		fre = p_tp->frequency - lnb_fre;
	}
	return fre;
}

uint32_t app_show_to_lnb_polar(GxBusPmDataSat *p_sat)
{
// 0-off 1-on 2-13V 3-18V
	uint32_t polar = SEC_VOLTAGE_OFF;

	switch (p_sat->sat_s.lnb_power)
	{
		case GXBUS_PM_SAT_LNB_POWER_OFF:
			polar = SEC_VOLTAGE_OFF;
			break;
		case GXBUS_PM_SAT_LNB_POWER_ON:
			polar = SEC_VOLTAGE_ALL;
			break;
		case GXBUS_PM_SAT_LNB_POWER_13V:
			polar = SEC_VOLTAGE_13;
			break;
		case GXBUS_PM_SAT_LNB_POWER_18V:
			polar = SEC_VOLTAGE_18;
			break;
		default:
			polar = SEC_VOLTAGE_OFF;
			break;
	}
	return polar;
}

uint32_t app_show_to_tp_polar(GxBusPmDataTP *p_tp)
{
// 0-18V 1-13V 2-auto
	uint32_t polar = SEC_VOLTAGE_OFF;

	switch (p_tp->tp_s.polar)
	{
		case GXBUS_PM_TP_POLAR_H:
			polar = SEC_VOLTAGE_18;
			break;
		case GXBUS_PM_TP_POLAR_V:
			polar = SEC_VOLTAGE_13;
			break;
		default:
			polar = SEC_VOLTAGE_OFF;
			break;
	}

	return polar;
}

void app_set_diseqc_10(uint32_t tuner, AppFrontend_DiseqcParameters *diseqc10, bool force)
{
    AppFrontend_DiseqcChange change;

    change.diseqc = diseqc10;
    app_ioctl(tuner, FRONTEND_DISEQC_CHANGE, &change);

    if(force == true || change.state == FRONTEND_CHANGED)
    {
        app_ioctl(tuner, FRONTEND_DISEQC_SET, diseqc10);
        app_log_debug("--------set diseqc 1.0, %d--------\n", diseqc10->u.params_10.chDiseqc);
    }
#if (OTA_SUPPORT > 0)
    extern void app_ota_backend_polling_set_diseqc10(int port);
    app_ota_backend_polling_set_diseqc10(diseqc10->u.params_10.chDiseqc);
#endif



//	if((force == true)
//		|| (g_AppNim.diseqc_change(&g_AppNim, diseqc10) == true))//(diseqc_port != diseqc10->u.params_10.chDiseqc)
//	{
//		g_AppNim.property_set(&g_AppNim, AppFrontendID_DiseqcCmd, (void*)diseqc10,
//		        sizeof(AppFrontend_DiseqcParameters));
//		app_log_debug("--------set diseqc 1.0, %d--------\n", diseqc10->u.params_10.chDiseqc);
//	}

}

void app_set_diseqc_11(uint32_t tuner, AppFrontend_DiseqcParameters *diseqc11, bool force)
{
    AppFrontend_DiseqcChange change;

    change.diseqc = diseqc11;
    app_ioctl(tuner, FRONTEND_DISEQC_CHANGE, &change);

    if(force == true || change.state == FRONTEND_CHANGED)
    {
        app_ioctl(tuner, FRONTEND_DISEQC_SET, diseqc11);
        app_log_debug("--------set diseqc 1.1, %d--------\n", diseqc11->u.params_11.chUncom);
    }
#if (OTA_SUPPORT > 0)
    extern void app_ota_backend_polling_set_diseqc11(int port);
    app_ota_backend_polling_set_diseqc11(diseqc11->u.params_11.chUncom);
#endif

//	if((force == true)
//		|| (g_AppNim.diseqc_change(&g_AppNim, diseqc11) == true))//(diseqc_port != diseqc11->u.params_11.chUncom)
//	{
//		g_AppNim.property_set(&g_AppNim, AppFrontendID_DiseqcCmd, (void*)diseqc11,
//		        sizeof(AppFrontend_DiseqcParameters));
//		app_log_debug("--------set diseqc 1.1, %d--------\n", diseqc11->u.params_11.chUncom);
//	}
}


// Flag == FALSE ,only used in play function now, for monitor open
void app_frontend_demod_control(uint32_t Tuner,uint8_t Flag)
{
#if NDEMOD_WITH_1TUNER
		static uint8_t UseFlag = 0;
		static int32_t TunerRecord = -1;

		if((TRUE == Flag) || (TRUE == UseFlag) || (TunerRecord != Tuner))
		{
			AppFrontend_Monitor monitor = FRONTEND_MONITOR_ON;
			AppFrontend_LNBState LnbOutput = FRONTEND_LNB_ON;
	// 切换两个没有TP的卫星的时候，会出现打开LNB供电的操作，导致界面出现错误的锁频信息。
			if(TunerRecord != Tuner)
				TunerRecord = Tuner;

	        app_ioctl(Tuner, FRONTEND_MONITOR_SET, &monitor);
			//app_ioctl(tuner, FRONTEND_LNB_CFG, &LnbOutput);
			if(g_AppPvrOps.state != PVR_DUMMY)
			{
				GxMsgProperty_NodeByIdGet node;

		    	node.node_type = NODE_PROG;
		    	node.pos = g_AppPvrOps.env.prog_id;
		   	 	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void*)&node);
				if(node.prog_data.tuner == Tuner)
				{
					goto TunerCfg;
				}
			}
			else
			{
	TunerCfg:	if(Tuner == 1)
				{
					monitor = FRONTEND_MONITOR_OFF;
					app_ioctl(0, FRONTEND_MONITOR_SET, &monitor);
					LnbOutput = FRONTEND_LNB_OFF;
					app_ioctl(0, FRONTEND_LNB_CFG, &LnbOutput);
				}
				else
				{
					monitor = FRONTEND_MONITOR_OFF;
					app_ioctl(1, FRONTEND_MONITOR_SET, &monitor);
					LnbOutput = FRONTEND_LNB_OFF;
					app_ioctl(1, FRONTEND_LNB_CFG, &LnbOutput);
				}
			}
		}

		UseFlag = Flag;
#endif

}

void app_set_tp(uint32_t tuner, AppFrontend_SetTp *params , SetTpMode set_mode)
{
#if !OTT_SUPPORT
    static AppFrontend_SetTp params_bak[APP_MULTI_DEMOD_NUM];
    if(tuner >= APP_MULTI_DEMOD_NUM)
    {
        app_log_error("\nTuner id [%d] error, %s, %d\n", tuner, __FUNCTION__, __LINE__);
        return;
    }

    if(set_mode == SAVE_TP_PARAM) //save operation must be in front of frequency invalid judgement
    {
        memcpy(&params_bak[tuner], params, sizeof(AppFrontend_SetTp));
        return;
    }
    s_CurTuner = tuner;

    if((FRONTEND_DVB_S == params->type) && false == app_frontend_dvbs_frequency_check(params->fre))
    {
        GxFrontend_SetUnlock(tuner);
        GxFrontend_CleanRecordPara(tuner);
        return;
    }

    //check if tp params changed
    if(((FRONTEND_DVB_S == params->type)
                && ( params->sat22k != params_bak[tuner].sat22k
                    || params->symb != params_bak[tuner].symb
                    || params->polar != params_bak[tuner].polar
#if UNICABLE_SUPPORT
                    || params->if_channel_index != params_bak[tuner].if_channel_index
                    || params->lnb_index != params_bak[tuner].lnb_index
                    || params->centre_fre != params_bak[tuner].centre_fre
                    || params->if_fre != params_bak[tuner].if_fre
#endif
                   ))
            || ((FRONTEND_DVB_T2 == params->type)
                && ((params->bandwidth != params_bak[tuner].bandwidth)
                    || (params->data_plp_id != params_bak[tuner].data_plp_id)
                    || (params->common_plp_exist != params_bak[tuner].common_plp_exist)
                    || (params->common_plp_id != params_bak[tuner].common_plp_id)))
            || ((FRONTEND_DVB_C == params->type)
                && ((params->qam != params_bak[tuner].qam)
                    || (params->symb != params_bak[tuner].symb)))
            || ((FRONTEND_DTMB == params->type)
                && (params->bandwidth != params_bak[tuner].bandwidth))
            || params->tuner != params_bak[tuner].tuner
            || params->fre != params_bak[tuner].fre
            || set_mode == SET_TP_FORCE)
    {
#if OTA_SUPPORT
        extern void app_ota_backend_polling_stop(void);
        app_ota_backend_polling_stop();
#endif

#if UNICABLE_SUPPORT
        app_ioctl(tuner, FRONTEND_SET_UNICABLE, params);
#endif
#if NDEMOD_WITH_1TUNER
        app_only_cur_frontend_monitor_start(tuner);
        AppFrontend_SetTp params_zero = {0};
        int i = 0;

        for(i = 0; i < APP_MULTI_DEMOD_NUM; i++)
        {
            if(tuner != i)
                memcpy(&params_bak[i], &params_zero, sizeof(AppFrontend_SetTp));
        }
#endif
        app_ioctl(tuner, FRONTEND_DEMUX_CFG, NULL);

#if NDEMOD_WITH_1TUNER && (APP_FRONTEND_PIN_SWITCH_COUNT || APP_FRONTEND_DEMOD_ADAPT_PIN_SWITCH_COUNT)
        app_pin_func_switch(tuner);
#endif

        app_ioctl(tuner, FRONTEND_TP_SET, params);
        app_log_debug("--------[T%d]set tp, frequency = %d, symbol_rate = %d, voltage = %d, sat22k = %d, bandwidth = %d, data_plp_id=%d--------\n",
                tuner+1, params->fre, params->symb, params->polar, params->sat22k, params->bandwidth,params->data_plp_id);

        // for monitor
        //save paras...
        memcpy(&params_bak[tuner], params, sizeof(AppFrontend_SetTp));
#if EMM_SUPPORT
        {
            AppFrontend_Config config;
            app_ioctl(tuner, FRONTEND_CONFIG_GET, &config);
            g_AppCat.ts_src = config.tuner;
            g_AppCat.demuxid = config.dmx_id;
            g_AppCat.pid = CAT_PID;
            g_AppCat.timeout = 5;
            g_AppCat.ver = 0xff;
            g_AppCat.start(&g_AppCat);
        }
#endif
#if PDMX_SUPPORT
        AppFrontend_Config config;
        app_ioctl(tuner, FRONTEND_CONFIG_GET, &config);
        app_pdmx_start_ts_deal(config.tuner, config.dmx_id, params->stream_size);
#endif

#if OTA_SUPPORT
        extern void app_ota_backend_polling_start(AppFrontend_SetTp *params);
        app_ota_backend_polling_start(params);
#endif
    }
#endif
    return;
}

// MAX count FRONTEND_DISEQC_MAX_CMD
void app_diseqc12_stop(uint32_t tuner, uint8_t Count)
{
	AppFrontend_DiseqcParameters diseqc12 = {0};
	uint8_t i = 0;
	if (Count > FRONTEND_DISEQC_MAX_CMD)
		Count = FRONTEND_DISEQC_MAX_CMD;
	diseqc12.type = DiSEQC12;
	diseqc12.u.params_12.CmdCount = Count;

	for(i = 0; i < Count;i++)
		diseqc12.u.params_12.DiseqcControl[i] = DISEQC_STOP;

    app_ioctl(tuner, FRONTEND_DISEQC_SET, &diseqc12);
}

void app_diseqc12_stop_drictory(uint32_t tuner, uint8_t Count)
{
	AppFrontend_DiseqcParameters diseqc12 = {0};
	uint8_t i = 0;
	if (Count > FRONTEND_DISEQC_MAX_CMD)
		Count = FRONTEND_DISEQC_MAX_CMD;
	diseqc12.type = DiSEQC12;
	diseqc12.u.params_12.CmdCount = Count;

	for(i = 0; i < Count;i++)
		diseqc12.u.params_12.DiseqcControl[i] = DISEQC_STOP;

    app_ioctl(tuner, FRONTEND_DISEQC_SET_EX, &diseqc12);
}

static uint32_t sg_Tuner = 0xff;

uint32_t app_frontend_sg_tuner_get(void)
{
    return sg_Tuner;
}

void app_frontend_sg_tuner_set(uint32_t sg_tuner)
{
    sg_Tuner = sg_tuner;
    return;
}

void app_diseqc12_save_position(uint32_t tuner,uint32_t pos)
{
	AppFrontend_DiseqcParameters diseqc = {0};

	//app_diseqc12_stop(tuner,1);

	diseqc.type = DiSEQC12;
	diseqc.u.params_12.CmdCount = 2;
	// stop first "app_diseqc12_stop(tuner,1);"
	diseqc.u.params_12.DiseqcControl[0] = DISEQC_STOP;
	// store cmd
	diseqc.u.params_12.DiseqcControl[1] = DISEQC_STORE_NN;
	diseqc.u.params_12.DiSEqC12Params[1].m_chParam0 = pos;

    app_ioctl(tuner, FRONTEND_DISEQC_SET, &diseqc);
}

static uint32_t s_diseqc_sat_id_bak = 0;
void app_diseqc_sat_id_bak_set(uint32_t id)
{
	s_diseqc_sat_id_bak = id;
}

MotorState app_diseqc12_goto_position(uint32_t sat_id, uint32_t tuner, uint32_t pos, bool force, int32_t motor_state)
{
	AppFrontend_DiseqcParameters diseqc12 = {0};
	AppFrontend_DiseqcChange change;

	if(motor_state)
	{
		diseqc12.type = DiSEQC12;
		diseqc12.u.params_12.CmdCount = 2;
		// stop cmd
		diseqc12.u.params_12.DiseqcControl[0] = DISEQC_STOP;
		// goto position
		diseqc12.u.params_12.DiseqcControl[1] = DISEQC_GOTO_NN;
		diseqc12.u.params_12.DiSEqC12Params[1].m_chParam0 = pos;
	}
	else
	{
		diseqc12.type = DiSEQC12;
		diseqc12.u.params_12.CmdCount = 1;
		// goto position
		diseqc12.u.params_12.DiseqcControl[0] = DISEQC_GOTO_NN;
		diseqc12.u.params_12.DiSEqC12Params[0].m_chParam0 = pos;
	}

    change.diseqc = &diseqc12;
    app_ioctl(tuner, FRONTEND_DISEQC_CHANGE, &change);

	if((force == true)
		|| (change.state == FRONTEND_CHANGED)
		|| (s_diseqc_sat_id_bak != sat_id))
	{
		app_ioctl(tuner, FRONTEND_DISEQC_SET, &diseqc12);
		sg_Tuner = tuner;

        if(0 == motor_state)
            app_diseqc12_set_tuner_id(tuner);

        return DISH_MOVING;
	}

    return DISH_NONE;
}

void app_diseqc12_find_drive(uint32_t tuner,LongitudeDirect direct)
{
	AppFrontend_DiseqcParameters diseqc = {0};

	//app_diseqc12_stop(tuner,1);

	diseqc.type = DiSEQC12;
	diseqc.u.params_12.CmdCount = 2;
	diseqc.u.params_12.DiseqcControl[0] = DISEQC_STOP;

	if(direct == LONGITUDE_EAST)
	{
		diseqc.u.params_12.DiseqcControl[1] = DISEQC_DRIVE_EAST;
	}
	else
	{
		diseqc.u.params_12.DiseqcControl[1] = DISEQC_DRIVE_WEST;
	}

    app_ioctl(tuner, FRONTEND_DISEQC_SET, &diseqc);

}

void app_diseqc12_find_drive_drictory(uint32_t tuner,LongitudeDirect direct)
{
	AppFrontend_DiseqcParameters diseqc = {0};

	//app_diseqc12_stop(tuner,1);

	diseqc.type = DiSEQC12;
	diseqc.u.params_12.CmdCount = 2;
	diseqc.u.params_12.DiseqcControl[0] = DISEQC_STOP;

	if(direct == LONGITUDE_EAST)
	{
		diseqc.u.params_12.DiseqcControl[1] = DISEQC_DRIVE_EAST;
	}
	else
	{
		diseqc.u.params_12.DiseqcControl[1] = DISEQC_DRIVE_WEST;
	}

    app_ioctl(tuner, FRONTEND_DISEQC_SET_EX, &diseqc);

}

void app_diseqc12_save_limit(uint32_t tuner,Diseqc12Limit limit)
{

	AppFrontend_DiseqcParameters diseqc = {0};

	//app_diseqc12_stop();

	diseqc.type = DiSEQC12;
	diseqc.u.params_12.CmdCount = 2;
	switch(limit)
	{
		case LIMIT_EAST:
			diseqc.u.params_12.DiseqcControl[0] = DISEQC_STOP;
			diseqc.u.params_12.DiseqcControl[1] = DISEQC_LIMIT_EAST;
			break;

		case LIMIT_WEST:
			diseqc.u.params_12.DiseqcControl[0] = DISEQC_STOP;
			diseqc.u.params_12.DiseqcControl[1] = DISEQC_LIMIT_WEST;
			break;

		default:
			diseqc.u.params_12.CmdCount = 1;
			diseqc.u.params_12.DiseqcControl[0] = DISEQC_LIMIT_OFF;
			//diseqc.u.params_12.DiseqcControl[1] = DISEQC_LIMIT_OFF;
			break;
	}

	app_ioctl(tuner, FRONTEND_DISEQC_SET, &diseqc);
}

void app_diseqc12_recalculate(uint32_t tuner,uint32_t pos)
{
	int init_value = 0;
	AppFrontend_DiseqcParameters diseqc;

	//app_diseqc12_stop(tuner,1);


	diseqc.type = DiSEQC12;
	diseqc.u.params_12.CmdCount = 2;
	// for stop
	diseqc.u.params_12.DiseqcControl[0] = DISEQC_STOP;
	// for recalculate
	diseqc.u.params_12.DiseqcControl[1] = DISEQC_SET_POSNS;
	diseqc.u.params_12.DiSEqC12Params[1].m_chParam0 = pos;
	diseqc.u.params_12.DiSEqC12Params[1].m_chParam1 = 0;
	GxBus_ConfigGetInt(LONGITUDE_KEY, &init_value, LONGITUDE);
	diseqc.u.params_12.DiSEqC12Params[1].m_chParam2 = init_value/10;//local Longitude
	GxBus_ConfigGetInt(LATITUDE_KEY, &init_value, LATITUDE);
	diseqc.u.params_12.DiSEqC12Params[1].m_chParam3 = init_value/10;

	app_ioctl(tuner, FRONTEND_DISEQC_SET, &diseqc);

}

static status_t _usals_diseqc12_calculate_azimuth(LongitudeInfo *local_longitude, LatitudeInfo *local_latitude, LongitudeInfo *sat_longitude, int *ret_angle)
{
	int sat_Longitude_val;
	int local_Longitude_val;
	int local_Latitude_val;
	int      iG20;
	float    iG23;
	float    iG24;
	float    iG25;
	float    iG26;
	float    iG29;
	float    iG30;
	float    iG31;
	float    iG32;
	float    iG33;
	int      iG34;
	int      iG35;
	int      iG36;
	int      iG37;

	local_Longitude_val = local_longitude->longitude * 10;
	if(local_longitude->longitude_direct == LONGITUDE_WEST)
	{
		local_Longitude_val = -local_Longitude_val;
		if (( local_Longitude_val < -MAX_LONGITUDE )||( local_Longitude_val > MAX_LONGITUDE ))
		{
			return GXCORE_ERROR;//STUSIF_ERROR_USALS_Longitude_OUT_OF_RANGE;
		}
	}

	local_Latitude_val = local_latitude->latitude * 10;
	if(local_latitude->latitude_direct == LATITUDE_SOUTH)
	{
		local_Latitude_val = -local_Latitude_val;
		if (( local_Latitude_val < -MAX_LATITUDE )||( local_Latitude_val > MAX_LATITUDE ))
		{
			return GXCORE_ERROR ;//STUSIF_ERROR_USALS_Latitude_OUT_OF_RANGE;
		}
	}

	sat_Longitude_val = sat_longitude->longitude *10;
	if(sat_longitude->longitude_direct == LONGITUDE_WEST)
	{
		sat_Longitude_val = -sat_Longitude_val;
		if (( sat_Longitude_val < -MAX_LONGITUDE )||( sat_Longitude_val > MAX_LONGITUDE ))
		{
			return GXCORE_ERROR ;//STUSIF_ERROR_USALS_ANGLE_OUT_OF_RANGE;
		}
	}

	iG20 = sat_Longitude_val - local_Longitude_val;
	if (( iG20 > MAX_LONGITUDE ))
	{
		iG20 = MAX_ANGLE_LONGITUDE - iG20;
	}
	if (( iG20 < -MAX_ANGLE_STAB_USALS )||( iG20 > MAX_ANGLE_STAB_USALS ))
	{
		*ret_angle = iG20;
		return GXCORE_SUCCESS;//STUSIF_ERROR_USALS_ANGLE_STAB_USALS_OUT_OF_RANGE;
	}

	iG23 = (float)RADIUS_OF_EARTH*COM_SIN((float)PI * (float)local_Latitude_val / (float)MAX_LONGITUDE);
	iG24 = COM_SQRT(QUADATE_RADIUS_OF_EARTH - iG23 * iG23);
	iG25 = COM_SQRT(QUADATE_RADIUS_OF_GEO_ORBIT+iG24*iG24-2*RADIUS_OF_GEO_ORBIT*iG24*COM_COS((float)PI*(float)iG20/(float)MAX_LONGITUDE));
	iG26 = COM_SQRT(iG23 * iG23 + iG25 * iG25 );

	iG29 = iG26;
	iG30 = COM_SQRT(((float)RADIUS_OF_GEO_ORBIT-iG24)*((float)RADIUS_OF_GEO_ORBIT-iG24)+iG23*iG23);
	iG31 = COM_SQRT(2*((float)QUADATE_RADIUS_OF_GEO_ORBIT-(float)QUADATE_RADIUS_OF_GEO_ORBIT*COM_COS((float)PI*(float)iG20/(float)MAX_LONGITUDE)));
	//iG38 = (double)( iG29 * iG29 + iG30 * iG30 - iG31 * iG31 ) / ( 2 * iG29 * iG30 );
	iG32 = COM_ACOS((iG29*iG29+iG30*iG30-iG31*iG31)/(2*iG29*iG30));
	iG33 = iG32*(float)MAX_LONGITUDE/(float)PI;
	iG34 = (int)(iG33);

	if (( iG34 < -MAX_ANGLE_STAB_USALS )||( iG34 > MAX_ANGLE_STAB_USALS ))
	{
		*ret_angle = iG34;
		return GXCORE_SUCCESS ;//STUSIF_ERROR_USALS_ANGLE_STAB_USALS_OUT_OF_RANGE2;
	}
	if ((( sat_Longitude_val < local_Longitude_val ) && ( local_Latitude_val > MIN_LATITUDE))
		|| (( sat_Longitude_val > local_Longitude_val ) && ( local_Latitude_val < MIN_LATITUDE)))
	{
		iG35 =   iG34;
	}
	else
	{
		iG35 = - iG34;
	}
	if (( sat_Longitude_val <= -HALF_MAX_LONGITUDE ) && ( local_Longitude_val >= HALF_MAX_LONGITUDE))
	{
		iG36 = - iG35;
	}
	else
	{
		iG36 =   iG35;
	}
	if (( sat_Longitude_val >= HALF_MAX_LONGITUDE ) && ( local_Longitude_val <= -HALF_MAX_LONGITUDE))
	{
		iG37 = - iG35;
	}
	else
	{
		iG37 = iG36;
	}
	if( iG37 >= 0)
	{
		if( iG37%10 >= 5)
		{
			iG37 = iG37 + 10 - iG37%10;
		}
		else
		{
			iG37 = iG37 - iG37%10;
		}
	}
	else
	{
		if( iG37%10 <= -5)
		{
			iG37 = iG37 - 10 - iG37%10;
		}
		else
		{
			iG37 = iG37  - iG37%10;
		}
	}
	*ret_angle = iG37;
	return GXCORE_SUCCESS;
}

static status_t _get_usals_cmd(LongitudeInfo *local_longitude, LatitudeInfo *local_latituide, LongitudeInfo *sat_longitude, int8_t *cmd1, int8_t *cmd2)
{
	int32_t nTurnAngle;
	status_t ret = GXCORE_ERROR;

	if(_usals_diseqc12_calculate_azimuth(local_longitude, local_latituide, sat_longitude, &nTurnAngle) == GXCORE_SUCCESS)
	{
		nTurnAngle /=10;
		if(nTurnAngle < 800 && nTurnAngle > -800)
		{
			*cmd1 = 0;
			*cmd2 = 0;

			if(nTurnAngle > 0)
			{
				*cmd1 |= 0xd0;
			}
			else
			{
				*cmd1 |= 0xe0;
				nTurnAngle = -nTurnAngle;
			}
			*cmd1 |= (int8_t)(nTurnAngle/10/16);
			*cmd2 |= (int8_t)(nTurnAngle/10%16);
			*cmd2 <<= 4;
			switch(nTurnAngle % 10)
			{
				case 0: *cmd2 |= 0x0; break;
				case 1: *cmd2 |= 0x2; break;
				case 2: *cmd2 |= 0x3; break;
				case 3: *cmd2 |= 0x5; break;
				case 4: *cmd2 |= 0x6; break;
				case 5: *cmd2 |= 0x8; break;
				case 6: *cmd2 |= 0xa; break;
				case 7: *cmd2 |= 0xb; break;
				case 8: *cmd2 |= 0xd; break;
				case 9: *cmd2 |= 0xe; break;
				default:
					break;
			}
			ret = GXCORE_SUCCESS;
		}
	}

	return ret;
}

MotorState app_usals_goto_position(uint32_t sat_id, uint32_t tuner, LongitudeInfo *sat_longitude, PositionVal *local_position, bool force, int32_t motor_state)
{
    int8_t cmd1 = 0;
    int8_t cmd2 = 0;
    LongitudeInfo local_longitude;
    LatitudeInfo local_latitude;

    if(local_position == NULL)
    {
        GxBus_ConfigGetInt(LONGITUDE_KEY, &local_longitude.longitude, LONGITUDE);
        GxBus_ConfigGetInt(LONGITUDE_DIRECT_KEY, (int*)&local_longitude.longitude_direct, LONGITUDE_DIRECT);
        GxBus_ConfigGetInt(LATITUDE_KEY, &local_latitude.latitude, LATITUDE);
        GxBus_ConfigGetInt(LATITUDE_DIRECT_KEY, (int*)&local_latitude.latitude_direct, LATITUDE_DIRECT);
    }
    else
    {
        local_longitude.longitude = local_position->longitude.longitude;
        local_longitude.longitude_direct = local_position->longitude.longitude_direct;
        local_latitude.latitude = local_position->latitude.latitude;
        local_latitude.latitude_direct = local_position->latitude.latitude_direct;
    }

    if(_get_usals_cmd(&local_longitude, &local_latitude, sat_longitude, &cmd1, &cmd2) == GXCORE_SUCCESS)
    {
        AppFrontend_DiseqcChange change;
        AppFrontend_DiseqcParameters diseqc = {0};

        diseqc.type = DiSEQC12;
        if(motor_state)
        {
            diseqc.u.params_12.CmdCount = 2;
            // for stop
            diseqc.u.params_12.DiseqcControl[0] = DISEQC_STOP;
            // for goto position
            diseqc.u.params_12.DiseqcControl[1] = DISEQC_GOTO_XX;
            diseqc.u.params_12.DiSEqC12Params[1].m_chParam0 = cmd1;
            diseqc.u.params_12.DiSEqC12Params[1].m_chParam1 = cmd2;
        }
        else
        {
            diseqc.u.params_12.CmdCount = 1;
            // for goto position
            diseqc.u.params_12.DiseqcControl[0] = DISEQC_GOTO_XX;
            diseqc.u.params_12.DiSEqC12Params[0].m_chParam0 = cmd1;
            diseqc.u.params_12.DiSEqC12Params[0].m_chParam1 = cmd2;
        }

        change.diseqc = &diseqc;
        app_ioctl(tuner, FRONTEND_DISEQC_CHANGE, &change);

        if((force == true)
                /*|| (thiz_interrupt_flag == true)*/
                || (s_diseqc_sat_id_bak != sat_id)
                || (change.state == FRONTEND_CHANGED))
        {
            //thiz_interrupt_flag = false;

            //app_diseqc12_stop(tuner,1);

            app_ioctl(tuner, FRONTEND_DISEQC_SET, &diseqc);
            sg_Tuner = tuner;

            if(0 == motor_state)
                app_diseqc12_set_tuner_id(tuner);

            return DISH_MOVING;
        }
    }
    else
    {
        if(force == true || s_diseqc_sat_id_bak != sat_id)
            return DISH_OUT_RANGE;
    }

    return DISH_NONE;
}

#if MULTISTREAM_SUPPORT
extern void GxFrontend_SetTsid(uint32_t tuner, uint8_t ts_id,uint8_t code);
extern int32_t GxFrontend_GetTsNum(int32_t tuner);
void app_set_muti_ts_para(uint32_t tuner,uint8_t ts_id,uint8_t code)
{
	GxFrontend_SetTsid(tuner,ts_id,code);
}

unsigned int app_get_muti_ts_num(int32_t tuner)
{
	struct dvbs_tsid_info params;

	GxFrontend_GetTsid(tuner, &params);

	return params.code_num;
}
#endif

#if PDMX_SUPPORT
bool app_frontend_need_predemux(uint32_t tuner)
{
    AppFrontend_Caps caps = 0;
    int pdmx_switch = 0;
    bool predemux = false;

    GxBus_ConfigGetInt(GXBUS_FRONTEND_PDMX, &pdmx_switch, GXBUS_FRONTEND_PDMX_OFF);
    app_ioctl(tuner, FRONTEND_GET_FE_CAPS, &caps);

    if(GXBUS_FRONTEND_PDMX_OFF == pdmx_switch)
    {
        return false;
    }
    else if(GXBUS_FRONTEND_PDMX_ON == pdmx_switch)
    {
        return true;
    }
    else
    {
        if ((caps & FE_CAN_T2MI) > 0)
        {
            predemux = false;
        }
        else
        {
            if((caps & FE_CAN_PREDEMUX) > 0)
            {
                predemux = true;
            }
            else
            {
                predemux = false;
            }
        }
    }
    return predemux;
}

bool app_frontend_hard_cap(uint32_t tuner, int flag)
{
    AppFrontend_Caps caps = 0;
    bool hard_cap = false;
    int pdmx_switch = 0;

    GxBus_ConfigGetInt(GXBUS_FRONTEND_PDMX, &pdmx_switch, GXBUS_FRONTEND_PDMX_OFF);

    app_ioctl(tuner, FRONTEND_GET_FE_CAPS, &caps);

    if ((GXBUS_FRONTEND_PDMX_AUTO == pdmx_switch)
            && ((caps & FE_CAN_T2MI) > 0)
            && (flag == 1))// for t2mi
    {
        hard_cap = true;
    }
    return hard_cap;
}

#endif

unsigned int app_frontend_standby(void)
{
#if !OTT_SUPPORT
    uint32_t i = 0;

    for(i = 0; i < APP_MULTI_DEMOD_NUM; i++)
    {
    	GxFrontend_Standby(i);
    }
#endif
    return 0;
}

static char *_get_tip(MotorState tip_type)
{
    char *tip = NULL;

    switch(tip_type)
    {
        case DISH_MOVING:
            tip = STR_ID_DISH_MOVING;
            break;

        case DISH_OUT_RANGE:
            tip = STR_ID_OUT_RANGE;
            break;

        default:
            break;
    }

    return tip;
}

extern void app_motor_move_cnt_set(int cnt);

static int _stop_diseqc12_cb(PopDlgRet ret)
{
    if(!strcmp(GUI_GetFocusWindow(),WND_CHANNEL_EDIT))
    {
        GUI_SetProperty(WND_CHANNEL_EDIT, "update", NULL);
    }

    app_diseqc12_stop(app_frontend_sg_tuner_get(), 1);
    app_frontend_sg_tuner_set(0xff);
    app_motor_move_cnt_set(0);
    return 0;
}


void app_create_position_tip(MotorState tip_type, int32_t motor_state)
{
    PopDlg pop = {0};
    char *tip = NULL;

    tip = _get_tip(tip_type);

    if(motor_state || NULL == tip || DISH_NONE == tip_type)
    {
        app_motor_move_cnt_set(0);
        return;
    }

    pop.type = POP_TYPE_NO_BTN;
    pop.mode = POP_MODE_UNBLOCK;
    pop.str = tip;

    if(DISH_MOVING == tip_type)
    {
        pop.exit_cb = _stop_diseqc12_cb;
        pop.timeout_sec = DISH_MOVE_SEC;
    }
    else
        pop.timeout_sec = 2;

    popdlg_create(&pop);


    return;
}
