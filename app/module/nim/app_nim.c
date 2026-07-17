#include "app.h"
#include "module/app_ioctl.h"

#if (DEMOD_DVB_S > 0)
extern AppNim nim_dvbs;
#endif

#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
extern AppNim nim_dvbt;
#endif

#if (FM_SUPPORT > 0)
extern AppNim nim_fm;
#endif

#if (DEMOD_DVB_C > 0)
extern AppNim nim_dvbc;
#endif

#if (DEMOD_DTMB > 0)
extern AppNim nim_dtmb;
#endif

#if (NET_SEARCH_SUPPORT > 0)
extern AppNim nim_dvbnet;
#endif


static AppNim *sp_Nim[NIM_MODULE_NUM]={NULL,NULL,NULL};
#if WEBAPI_SUPPORT
static int nim_demod_type[NIM_MODULE_NUM] = {-1, -1, -1};
#endif

status_t nim_init(uint32_t Index, int demod_type)
{
    AppNim *nim_new = NULL;
    AppNim *nim_src[DEMOD_TYPE_TOTAL] = {
#if (DEMOD_DVB_S > 0)
        &nim_dvbs,
#endif
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
        &nim_dvbt,
#endif
#if (DEMOD_DVB_C > 0)
        &nim_dvbc,
#endif
#if (DEMOD_DTMB > 0)
        &nim_dtmb,
#endif
#if (FM_SUPPORT > 0)
        &nim_fm,
#endif
#if NET_SEARCH_SUPPORT
		&nim_dvbnet,
#endif

    };

	if((Index >= NIM_MODULE_NUM)
		|| (demod_type >= DEMOD_TYPE_TOTAL))
	{
		app_log_error("\nNIM, error, %s, %d\n",__FUNCTION__,__LINE__);
		return GXCORE_ERROR;
	}


    nim_new = NULL;
    if ((nim_new = (AppNim*)GxCore_Calloc(1, sizeof(AppNim))) == NULL)
    {
        return GXCORE_ERROR;
    }

    memcpy(nim_new, nim_src[demod_type], sizeof(AppNim));

    // open dev
    if ((nim_new->fro_dev_hdl = GxAvdev_CreateDevice(0)) <= 0)
    {
        return GXCORE_ERROR;
    }

    sp_Nim[Index] = nim_new;
#if WEBAPI_SUPPORT
    nim_demod_type[Index] = demod_type;
#endif
    //app_log_debug("\nNIM, dest = 0x%x , dvbs = 0x%x, dtmb = 0x%x\n", &(sp_Nim[Index]->tp_set),&(nim_dvbs.tp_set),&(nim_dtmb.tp_set));

    return GXCORE_SUCCESS;
}

void nim_release(void)
{
    uint32_t i;
    AppNim* nim = NULL;

    for (i = 0; i < NIM_MODULE_NUM; i++)
    {
#if WEBAPI_SUPPORT
        nim_demod_type[i] = -1;
#endif
        nim = sp_Nim[i];
        if (nim != NULL && nim->fro_dev_hdl != 0)
        {
            GxAvdev_DestroyDevice(nim->fro_dev_hdl);
            GxCore_Free(nim);
            nim = NULL;
        }
    }
}

status_t nim_ioctl(uint32_t id, uint32_t cmd, void * params)
{
    AppNim *nim = NULL;

    if (id >= NIM_MODULE_NUM)
    {
        app_log_error("[NIM] id error!\n");
        return GXCORE_ERROR;
    }

    nim = sp_Nim[id];
    if(nim == NULL)
    {
        app_log_error("\n[NIM] ERROR!!!  NIM is NULL,please init NIM!\n");
        return GXCORE_ERROR;
    }

    if(nim->fro_dev_hdl <= 0)
    {
        return GXCORE_ERROR;
    }
    //app_low_info("\nNIM, %s, tuner = %d, cmd = %d, nim = 0x%x\n", __FUNCTION__,id, cmd,nim);
    switch (cmd)
    {
        case FRONTEND_OPEN:
            if(nim->open != NULL)
                nim->open(nim, (AppFrontend_Open*)params);
            break;
        case FRONTEND_TP_SET:
            if(nim->tp_set != NULL)
                nim->tp_set(nim, (AppFrontend_SetTp*)params);
            break;
        case FRONTEND_DISEQC_CHANGE:
            if(nim->diseqc_change != NULL)
                nim->diseqc_change(nim, (AppFrontend_DiseqcChange*)params);
            break;
        case FRONTEND_DISEQC_SET:
            if(nim->diseqc_set != NULL)
                nim->diseqc_set(nim, (AppFrontend_DiseqcParameters*)params);
            break;
        case FRONTEND_DISEQC_SET_EX:
            if(nim->diseqc_set_ex != NULL)
                nim->diseqc_set_ex(nim, (AppFrontend_DiseqcParameters*)params);
            break;
        case FRONTEND_22K_SET:
            if(nim->switch_22k_set != NULL)
                nim->switch_22k_set(nim,(AppFrontend_22KState*)params);
            break;
        case FRONTEND_22K_CHANGE:
            if(nim->_22k_change != NULL)
                nim->_22k_change(nim, (AppFrontend_22KChange *)params);
            break;
        case FRONTEND_POLAR_SET:
#ifdef HAVE_LIB_CIM
            {
                extern int app_ci_gpio_setlevel(unsigned long gpio, unsigned long value);
                if(*(AppFrontend_PolarState *)params == FRONTEND_POLAR_OFF)
                    app_ci_gpio_setlevel(4,0);
                else
                    app_ci_gpio_setlevel(4,1);
            }
#endif
            if(nim->polar_set != NULL)
                nim->polar_set(nim,(AppFrontend_PolarState*)params);
            break;
        case FRONTEND_POLAR_CHANGE:
            if(nim->polar_change != NULL)
                nim->polar_change(nim, (AppFrontend_PolarChange *)params);
            break;
        case FRONTEND_DISEQC_SET_DEFAULT:
            if(nim->diseqc_set != NULL)
            {
                AppFrontend_DiseqcParameters *diseqc = (AppFrontend_DiseqcParameters *)params;
                if (diseqc->type == DiSEQC10)
                {
                    memcpy(diseqc, &nim->diseqc10, sizeof(AppFrontend_DiseqcParameters));
                }
                else if (diseqc->type == DiSEQC11)
                {
                    memcpy(diseqc, &nim->diseqc11, sizeof(AppFrontend_DiseqcParameters));
                }
                else if (diseqc->type == DiSEQC12)
                {
                    /*
                       memcpy(diseqc, &nim->diseqc12, sizeof(AppFrontend_DiseqcParameters));
                       if((diseqc->u.params_12.DiseqcControl != DISEQC_GOTO_NN)
                       && (diseqc->u.params_12.DiseqcControl != DISEQC_GOTO_XX))
                       return GXCORE_ERROR;

                       AppFrontend_DiseqcParameters diseqc12 = {0};
                       diseqc12.type = DiSEQC12;
                       diseqc12.u.params_12.CmdCount = 1;
                       diseqc12.u.params_12.DiseqcControl[0] = DISEQC_STOP;
                       nim->diseqc_set(nim, &diseqc12);
                       */
                }
                else
                {
                    return GXCORE_ERROR;
                }
                nim->diseqc_set(nim, diseqc);
            }
            break;
        case FRONTEND_MONITOR_SET:
            if(nim->monitor_set != NULL)
                nim->monitor_set(nim, *(AppFrontend_Monitor*)params);
            break;
        case FRONTEND_LOCK_STATE_SET:
            if(nim->lock_state_set != NULL)
                nim->lock_state_set(nim, *(AppFrontend_LockState*)params);
            break;
        case FRONTEND_LOCK_STATE_GET:
            if(nim->lock_state_get != NULL)
                nim->lock_state_get(nim, (AppFrontend_LockState*)params);
            break;
        case FRONTEND_STRENGTH_GET:
            if(nim->strength_get != NULL)
                nim->strength_get(nim, (AppFrontend_Strengh*)params);
            break;
        case FRONTEND_QUALITY_GET:
            if(nim->quality_get != NULL)
                nim->quality_get(nim, (AppFrontend_Quality*)params);
            break;
        case FRONTEND_ERRORRATE_GET:
            if(nim->errorrate_get != NULL)
                nim->errorrate_get(nim, (AppFrontend_ErrorRate*)params);
            break;
        case FRONTEND_CLOSE:
            if(nim->close != NULL)
                nim->close(nim);
            break;
        case FRONTEND_INFO_GET:
            if(nim->info_get != NULL)
                nim->info_get(nim, (AppFrontend_Info*)params);
            break;
        case FRONTEND_CONFIG_GET:
            if(nim->config_get != NULL)
                nim->config_get(nim, (AppFrontend_Config*)params);
            break;
		case FRONTEND_CONFIG_SET:
			if(nim->config_set != NULL)
          		 nim->config_set(nim, (AppFrontend_Config*)params);
            break;
        case FRONTEND_DEMUX_CFG:
            if(nim->demux_cfg != NULL)
                nim->demux_cfg(nim);
            break;
        case FRONTEND_LNB_CFG:
            if(nim->lnb_cfg != NULL)
                nim->lnb_cfg(nim,(AppFrontend_LNBState*)params);
            break;
		case FRONTEND_PLS_N_GET:
            if(nim->calc_pls_n!= NULL)
                nim->calc_pls_n(nim,(unsigned int*)params);
            break;
        case FRONTEND_PLSN_ROOT_CONVERT:
            if(nim->plsn_root_convert!= NULL)
                nim->plsn_root_convert(nim,(AppFrontend_PlsNRootConvert*)params);
            break;
        case FRONTEND_GET_FE_CAPS:
            if(nim->fe_caps_get != NULL)
                nim->fe_caps_get(nim, (AppFrontend_Caps *)params);
            break;
        case FRONTEND_SET_UNICABLE:
            if(nim->set_unicable != NULL)
                nim->set_unicable(nim, (AppFrontend_SetTp *)params);
            break;
        default:
            return GXCORE_ERROR;
    }

    return GXCORE_SUCCESS;
}

#if WEBAPI_SUPPORT
#include <cJSON.h>

static char *app_webapi_get_frontend_type(int demod_type)
{
	char *str = NULL;

	switch(demod_type)
	{
#if DEMOD_DVB_S
		case DEMOD_TYPE_DVBS:
			str = "dvbs";
			break;
#endif

#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
		case DEMOD_TYPE_DVBT:
			str = "dvbt";
			break;
#endif

#if DEMOD_DVB_C
		case DEMOD_TYPE_DVBC:
			str = "dvbc";
			break;
#endif

#if DEMOD_DTMB
		case DEMOD_TYPE_DTMB:
			str = "dtmb";
			break;
#endif

		default:
			str = "unknown";
			break;
	}

	return str;
}

void app_webapi_make_frontend_data(cJSON *json)
{
	AppNim* nim = NULL;
	int i = 0;
	int nim_total = 0;
	cJSON *json1 = NULL;
	cJSON *json2 = NULL;
	char buffer[16];

	json1 = cJSON_CreateArray();
	cJSON_AddItemToObject(json, "data", json1);
	for (i = 0; i < NIM_MODULE_NUM; i++)
	{
		nim = sp_Nim[i];
		if(nim != NULL && nim->fro_dev_hdl != 0)
		{
			json2 = cJSON_CreateObject();
			cJSON_AddItemToArray(json1, json2);
			snprintf(buffer, sizeof(buffer)-1, "%d", i+1);
			cJSON_AddStringToObject(json2, "id", buffer);
			cJSON_AddStringToObject(json2, "type", app_webapi_get_frontend_type(nim_demod_type[i]));
			cJSON_AddNumberToObject(json2, "tuner", nim->tuner);
			nim_total++;
		}
	}
	cJSON_AddNumberToObject(json, "total", nim_total);

}

#if (DEMOD_DVB_S > 0)
int app_webapi_get_dvbs_demods(int *tuners)
{
	int num = 0;
	int i = 0;
	AppNim* nim = NULL;

	for(i=0; i<NIM_MODULE_NUM; i++)
	{
		nim = sp_Nim[i];
		if(nim && (nim->fro_dev_hdl) && (nim_demod_type[i] == DEMOD_TYPE_DVBS))
		{
			tuners[num] = nim->tuner;
			num++;
		}
	}

	return num;
}
#endif


#endif


