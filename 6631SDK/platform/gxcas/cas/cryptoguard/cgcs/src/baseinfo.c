#include "cryptoguard_api.h"
#include "cryptoguard_misc.h"
#include "IntCAM.h"
#include "gxcas_dbg.h"

int32_t cg_baseinfo_get(unsigned long type, void *value)
{
    int32_t h_result = -1;
    unsigned char ret = 0;

    switch(type)
    {       
        case GXCAS_CRYPTOGUARD_GET_LIB_VER_LONG:
            break;
        case GXCAS_CRYPTOGUARD_GET_LIB_VER:
            break;
        case GXCAS_CRYPTOGUARD_GET_LIB_DATE:
            break;
        case GXCAS_CRYPTOGUARD_GET_MATURITY_RATING:
            ret = getMaturityLevel((unsigned char *)value);
            h_result = (ret == 1) ? 0 : -1;
            break;
        case GXCAS_CRYPTOGUARD_GET_STRICT_RATING:
            {
                extern unsigned char gStrictParentalRating;
                *(unsigned char *)value = gStrictParentalRating;
                h_result = 0;
            }
            break;
        default:
            break;

    }
    if (h_result != 0)
        CAS_ERR(CAS, "%lu, CAS RETURNS ERR", GXCAS_GETNUM(type));
    return h_result;
}

int32_t cg_baseinfo_set(unsigned long type, void *value)
{
    int32_t h_result = -1;
    unsigned char ret = 0;

    switch(type)
    {
        case GXCAS_CRYPTOGUARD_SET_STB_INFO:
            {
                nvmem_storage nvmem;      
                GxCas_CGSetStbInfo *param = (GxCas_CGSetStbInfo *)value;
                
                memset(&nvmem, 0, sizeof(nvmem_storage));
                memcpy(nvmem.STB_SystemGlobalKey, param->STB_SystemGlobalKey, 16);
                memcpy(nvmem.STB_ManufacturerGlobalKey, param->STB_ManufacturerGlobalKey, 16); 
                memcpy(nvmem.STB_GroupKey, param->STB_GroupKey, 16);
                memcpy(nvmem.STB_UniqueKey, param->STB_UniqueKey, 16);
                memcpy(nvmem.STB_CardlessGroupKey, param->STB_CardlessGroupKey, 16);
                memcpy(nvmem.STB_CardlessUniqueKey, param->STB_CardlessUniqueKey, 16);
                nvmem.IrdNumber = param->IrdNumber;
                InitNVMEMRAM(nvmem, 0); 
                cpg_stb_info_write();
                h_result = 0;
                break;
            }
        case GXCAS_CRYPTOGUARD_SET_MATURITY_RATING:
            {
                GxCas_CGChangeRating *p_maturity_rating = (GxCas_CGChangeRating *)value;

                ret = changeMaturityLevel(p_maturity_rating->pin, p_maturity_rating->rating);
                h_result = (ret == 0) ? -1 : 0;
                break;
            }
        case GXCAS_CRYPTOGUARD_SET_PIN:
            {
                GxCas_CGChangePin *p_pin = (GxCas_CGChangePin *)value;

                ret = changePIN(p_pin->old_pin, p_pin->new_pin);
                h_result = (ret == 0) ? -1 : 0;
                break;
            }
        case GXCAS_CRYPTOGUARD_SET_STRICT_RATING:
            {
                GxCas_CGSetStrictRating *p_strict_rating = (GxCas_CGSetStrictRating *)value;

                ret = ToggleStrictParentalRatingUsingPIN(p_strict_rating->pin);
                h_result = (ret == 0) ? -1 : 0;
                break;
            }

        default:
            break;

    }
    if (h_result != 0)
        CAS_ERR(CAS, "%lu, CAS RETURNS ERR", GXCAS_GETNUM(type));
    return h_result;
}
