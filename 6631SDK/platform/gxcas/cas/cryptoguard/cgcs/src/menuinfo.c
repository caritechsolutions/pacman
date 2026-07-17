#include "cryptoguard_misc.h"
#include "cryptoguard_api.h"
#include "cg_cas.h"
#include "IntCAM.h"
#include "gxcas_dbg.h"

int32_t cg_menuinfo_get(unsigned long type, void *value)
{
    int32_t ret = CG_SUCCESS;

    switch(type)
    {
        case GXCAS_CRYPTOGUARD_GET_MAIN_MENU:
            GetMMI(0);
            break;
        case GXCAS_CRYPTOGUARD_GET_SUBSCRIPTION_INFO:
            GetMMI(1);
            break;
        case GXCAS_CRYPTOGUARD_GET_PAY_INFO:
            GetMMI(2);
            break;
        case GXCAS_CRYPTOGUARD_GET_ABOUT_CA:
            GetMMI(3);
            break;
        case GXCAS_CRYPTOGUARD_GET_PARENTAL_CONTROL:
            GetMMI(4);
            break;
        default:
            break;
    }

    strcpy((char *)value, menu);

    if (ret != CG_SUCCESS)
        CAS_ERR(CAS,"%lu,CDCA RETURN ERROR",GXCAS_GETNUM(type));
    return ret;
}
