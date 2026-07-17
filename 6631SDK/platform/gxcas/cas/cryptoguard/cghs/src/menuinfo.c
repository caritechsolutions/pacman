#include "cryptoguard_misc.h"
#include "cryptoguard_api.h"
#include "cg_cas.h"
#include "gxcas_dbg.h"

int32_t cg_menuinfo_get(unsigned long type, void *value)
{
	int32_t ret = -1;
	switch(type)
	{
		case GXCAS_CRYPTOGUARD_GET_MAIN_MENU:
			ret = cg_get_MMI(0, (char *)value);
			break;
		case GXCAS_CRYPTOGUARD_GET_SUBSCRIPTION_INFO:
			ret = cg_get_MMI(1, (char *)value);
			break;
		case GXCAS_CRYPTOGUARD_GET_PAY_INFO:
			ret = cg_get_MMI(2, (char *)value);
			break;
		case GXCAS_CRYPTOGUARD_GET_ABOUT_CA:
			ret = cg_get_MMI(4, (char *)value);
			break;
		default:
			break;
	}
	if (ret != CG_SUCCESS)
		CAS_ERR(CAS,"%lu,CDCA RETURN ERROR",GXCAS_GETNUM(type));
	return ret;
}
