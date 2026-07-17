#include "gxcore.h"
#include "gxsearch.h"
#include "module/frontend/gxfrontend_module.h"

static bool _plsn_set(int32_t tuner, uint32_t *tp_pls_n)
{
    bool ret = false;

    if(GxFrontend_HasSpecialFeatures(tuner)
            && (!GxFrontend_HasAutoSpecialFeatures(tuner)))
    {
        if((*tp_pls_n <= 0) || (*tp_pls_n > 262142))
            GxFrontend_GetAutoPlsNum(tuner, tp_pls_n);

        if(*tp_pls_n > 0 && *tp_pls_n < 262142)
        {
            GxFrontend_SetAutoPlsNum(tuner, *tp_pls_n);
            ret = true;
        }
    }

    return ret;
}

static PlsnSearchClass thiz_pls_ops = {
    .plsn_set = _plsn_set,
};


void app_plsn_register(void)
{
    GxSearchRegistPlsn(&thiz_pls_ops);
    return;
}

void app_plsn_unregister(void)
{
    GxSearchUnregistPlsn();
    return;
}

