#include "cryptoguard_api.h"
#include "cg_cas.h"
#include "gxcas_dbg.h"

int32_t cg_baseinfo_get(unsigned long type, void *value)
{
	int32_t ret = -1;
	switch(type)
	{
		case GXCAS_CRYPTOGUARD_GET_LIB_VER_LONG:
			{
				const char *p = NULL;
				p = cg_get_lib_ver_long();
				if ((p != NULL) && (value != NULL))	{
					strcpy(value, p);
					ret = CG_SUCCESS;
				}
				break;
			}
		case GXCAS_CRYPTOGUARD_GET_LIB_VER:
			{
				const char *p = NULL;
				p = cg_get_lib_ver();
				if ((p != NULL) && (value != NULL))	{
					strcpy(value, p);
					ret = CG_SUCCESS;
				}
				break;
			}
		case GXCAS_CRYPTOGUARD_GET_LIB_DATE:
			{
				const char *p = NULL;
				p = cg_get_lib_date();
				if ((p != NULL) && (value != NULL))	{
					strcpy(value, p);
					ret = CG_SUCCESS;
				}
				break;
			}
		case GXCAS_CRYPTOGUARD_GET_MATURITY_RATING:
			ret = cg_get_maturity_rating((unsigned char *)value);
			break;
		case GXCAS_CRYPTOGUARD_GET_STRICT_RATING:
			ret = cg_get_strict_parental_rating((unsigned char *)value);
			break;
		default:
			break;
	}
	if (ret != CG_SUCCESS)
		CAS_ERR(CAS, "%lu, CAS RETURNS ERR", GXCAS_GETNUM(type));
	return ret;
}
