#include "gxtype.h"
#include "gxcas_dbg.h"
#include "gxcas_psi.h"
#include "gxcas_flash.h"
#include "gxcas_smartcard.h"
#include "gxcas_demux.h"
#include "gxcas_misc.h"
#include "gxcas_service.h"
#include "cryptoguard_api.h"
#include "cryptoguard_misc.h"
#include "cg_cas.h"
#include "cg_callbacks.h"
#include "cryptoguard_callbacks.h"
#include <assert.h>

#ifdef CONFIG_TFM
extern int32_t cryptoguard_scpu_reset(void);
#endif

static void cpg_register_callback_func(void)
{
	cg_debugprint_register_callback_func((void(*)(const char*, ...))printf);
	cg_flash_mem_register_callback_func(cpg_read_write_nvmem);
	cg_scroll_message_register_callback_func(Callback_STB_DisplayScrollMessage);
	cg_textmessage_register_callback_func(Callback_STB_DisplayMessage);
	cg_osdmessage_register_callback_func(Callback_OSDMessage);
	cg_fingerprint_register_callback_func(Callback_STB_DisplayFingerPrint);
	cg_videorules_register_callback_func(Callback_STB_VideoRulesChanged);
	cg_OTA_trigger_register_callback_func(Callback_STB_OtaTriggerReceived);
	cg_request_pin_register_callback_func(Callback_PinRequired);

	return;
}

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
		case GXCAS_CRYPTOGUARD_CHECK_PIN:
			{
				GxCas_CGCheckPin *p = (GxCas_CGCheckPin *)value;
				ret = cg_verify_PIN(p->pin);
				break;
			}
		case GXCAS_CRYPTOGUARD_SET_PIN:
			{
				GxCas_CGChangePin *p = (GxCas_CGChangePin *)value;
				ret = cg_change_PIN(p->old_pin, p->new_pin);
				break;
			}
		case GXCAS_CRYPTOGUARD_SET_MATURITY_RATING:
			{
				GxCas_CGChangeRating *p = (GxCas_CGChangeRating *)value;
				ret = cg_change_maturity_level(p->pin, p->rating);
				break;
			}
		case GXCAS_CRYPTOGUARD_UNLOCK_PIN:
			{
				GxCas_CGUnlockPin *p = (GxCas_CGUnlockPin *)value;
				ret = cg_unlock_using_PIN(p->CH, p->pin);
				break;
			}
		case GXCAS_CRYPTOGUARD_SET_STRICT_RATING:
			{
				GxCas_CGSetStrictRating *p = (GxCas_CGSetStrictRating *)value;
				ret = cg_set_strict_parental_rating(p->enable, p->pin);
				break;
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
#ifdef CONFIG_TFM
	if	(0 != cryptoguard_scpu_reset()){
		CAS_ERR(CAS,"cryptoguard_scpu_reset Failed!!!");
		return -1;
	}
#endif

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

	cpg_register_callback_func();
	printf("\e[1;31mCAS library %s\n\e[0m", cg_get_lib_ver_long());

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
