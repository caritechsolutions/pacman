#include "gxtype.h"
#include "gxcas_dbg.h"
#include "gxcas_psi.h"
#include "gxcas_flash.h"
#include "gxcas_smartcard.h"
#include "gxcas_demux.h"
#include "gxcas_misc.h"
#include "gxcas_service.h"
#include "gxcas.h"
#include "cryptoguard_api.h"
#include "cryptoguard_misc.h"
#include "IntCAM.h"
#include <assert.h>

extern int32_t cryptoguard_scpu_reset(void);

int32_t cpg_get(unsigned long type, void *value)
{
	int32_t ret =  0;
	switch(type)
	{
		case GXCAS_CRYPTOGUARD_GET_LIB_VER_LONG:
		case GXCAS_CRYPTOGUARD_GET_LIB_VER:
		case GXCAS_CRYPTOGUARD_GET_LIB_DATE:
		case GXCAS_CRYPTOGUARD_GET_MATURITY_RATING:
		case GXCAS_CRYPTOGUARD_GET_STRICT_RATING:
			return cg_baseinfo_get(type, value);
		case GXCAS_CRYPTOGUARD_GET_MAIN_MENU:
		case GXCAS_CRYPTOGUARD_GET_SUBSCRIPTION_INFO:
		case GXCAS_CRYPTOGUARD_GET_PAY_INFO:
		case GXCAS_CRYPTOGUARD_GET_ABOUT_CA:
		case GXCAS_CRYPTOGUARD_GET_PARENTAL_CONTROL:
			return cg_menuinfo_get(type, value);
		default:
			break;
	}
	return ret;
}

int32_t cpg_set(unsigned long type, void *value)
{
	int32_t ret =  0;

	switch(type) {
		case GXCAS_CRYPTOGUARD_SWITCH_CHANNEL:
			{
				GxCasSet_SwitchChannel *param = (GxCasSet_SwitchChannel *)value;
				ret = cpg_switch_channel(param->pmt_pid,param->service_id,param->audio_pid,param->video_pid);
				break;
			}
        case GXCAS_CRYPTOGUARD_SET_MATURITY_RATING:
        case GXCAS_CRYPTOGUARD_SET_PIN:
        case GXCAS_CRYPTOGUARD_SET_STRICT_RATING:
		case GXCAS_CRYPTOGUARD_SET_STB_INFO:
			{
				ret = cg_baseinfo_set(type, value);
				break;
			}
		case GXCAS_CRYPTOGUARD_UNLOCK_PIN:
			{
				ret = unlockUsingPIN((char*)value);
			}
		default:
			CAS_ERR(CAS, "%lu, Error Param", GXCAS_GETNUM(type));
			break;
	}
	if (ret != 0)
		CAS_ERR(CAS, "%lu, CAS RETURN ERR", GXCAS_GETNUM(type));
	return ret;
}

gxcas_control cpg_ops = {
	.set = cpg_set,
	.get = cpg_get,
};

int32_t GxCas_CryptoGuardInit(const GxCasInitParam init)
{
	handle_t cpgca_thread;
	handle_t cpgca_init_thread;
	
	GxCas_Init(&cpg_ops, init);
	GxCas_SiFilter_Init();
	
	if(0 != cryptoguard_scpu_reset()){
		CAS_ERR(CAS,"cryptoguard_scpu_reset Failed!!!");
		return -1;
	}
	
	if (init.sci.sci_switch) {
		GxSmcParams param = {0};

		GxCas_Smc_Init(init.sci.detect_pole, init.sci.vcc_pole);
		param.io_conv = GXSMC_DATA_CONV_DIRECT;
		param.parity = GXSMC_PARITY_ODD;// GXSMC_PARITY_EVEN;
		param.protocol = DISABLE_REPEAT_WHEN_ERR;
		param.stop_len = GXSMC_STOPLEN_0BIT;
		param.default_etu = 372;
		GxCas_Smc_Open(&param, cpg_smartcard_change);
	}
	//cpgca backup size must be equal to flash size
	if (init.backup.size != 0 && init.backup.size != init.flash.size) {
		CAS_ERR(CAS, "Flash Size != BackUp Size");
		return -1;
	} else {
		GxCas_FlashConfig flash;
		GxCas_FlashConfig backup;

		memcpy(flash.name, init.flash.name, 128);
		flash.size = init.flash.size;
		flash.offset = init.flash.offset;
		memcpy(backup.name, init.backup.name, 128);
		backup.size = init.backup.size;
		backup.offset = init.backup.offset;
		assert(GxCas_Flash_Init(flash, backup) == 0);
	}
	GxCas_Service_Init();
	GxCore_ThreadCreate("cpgca",&cpgca_thread, cpgca_task, NULL, 64 * 1024, GXOS_DEFAULT_PRIORITY);
	GxCore_ThreadCreate("cpgca_init",&cpgca_init_thread, cpgca_init_task, NULL, 64 * 1024, GXOS_DEFAULT_PRIORITY);
#if 0	
	if(TRUE != CAInit()) {
		CAS_ERR(CAS,"CryptoGuard CA Init Faild!!!");
		return -1;
	}	
#endif
	return 0;
}
