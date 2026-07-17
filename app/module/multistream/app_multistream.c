#include "app_config.h"
#if MULTISTREAM_SUPPORT
#include "app.h"
#include "frontend/frontend.h"
#include "module/frontend/gxfrontend_module.h"
#include "gxsearch.h"

typedef struct
{
    uint8_t ts_id;
    uint8_t pls_code;
    uint8_t pls_change;
    uint8_t pls_status;
}MultiStreamInfo;

typedef struct _MULTISTREAMInfoClass {
    uint16_t index;
    uint16_t total;
    MultiStreamInfo* info;
}MultiStreamInfoClass;

static MultiStreamInfoClass thiz_multistream_info = {
    .index = 0,
    .total = 0,
    .info  = NULL,
};

static int32_t _multistream_init(int32_t tuner)
{
    int err = -1;
    int index = 0;
    struct dvbs_tsid_info params = {0};

    if(thiz_multistream_info.total != 0)
    {
        app_log_error("[%s, %d] multistream has initialized\n", __func__, __LINE__);
        return err;
    }

    GxCore_ThreadDelay(600);//delay for demod calculate tsid

    params.code = GxCore_Calloc(256, sizeof(struct dvbs_tsid_parameters));
    if(NULL == params.code)
    {
        app_log_error("[%s, %d] ERROR, calloc fail!\n", __func__, __LINE__);
        return err;
    }

    err = GxFrontend_GetTsid(tuner, &params);
    if((err >= 0) && (params.code_num <= 256 && params.code_num > 0))
    {
        thiz_multistream_info.index = 0;
        thiz_multistream_info.total = params.code_num;
        thiz_multistream_info.info = GxCore_Calloc(params.code_num, sizeof(MultiStreamInfo));
        if(thiz_multistream_info.info == NULL)
        {
            GxCore_Free(params.code);
            app_log_error("[%s, %d] ERROR, calloc fail!\n", __func__, __LINE__);
            return -1;
        }

        for(index = 0; index < params.code_num; index++)
        {
            thiz_multistream_info.info[index].ts_id = params.code[index].ts_id;
            thiz_multistream_info.info[index].pls_code = params.code[index].pls_code;
        }
    }
    GxCore_Free(params.code);
    return err;
}

static int32_t _multistream_exit(void)
{
    thiz_multistream_info.index = 0;
    thiz_multistream_info.total = 0;
    APP_FREE(thiz_multistream_info.info);

    return 0;
}

static int32_t _multistream_get_params(void *params)
{
    GxBusPmDataProg *prog = NULL;

    prog = (GxBusPmDataProg *)params;

    if(prog && thiz_multistream_info.info && thiz_multistream_info.total > 1)
    {
		prog->muti_ts_exist = 0x1;
		prog->muti_ts_id = thiz_multistream_info.info[thiz_multistream_info.index].ts_id;
		prog->muti_ts_code = thiz_multistream_info.info[thiz_multistream_info.index].pls_code;
    }

    return 0;
}

static int32_t _multistream_set_params(int32_t tuner)
{
    uint8_t ts_id = 0;
    uint8_t pls_code = -1;

    if(0 == thiz_multistream_info.total
            || NULL == thiz_multistream_info.info
            || thiz_multistream_info.index >= thiz_multistream_info.total)
    {
        app_log_error("\033[31m[%s, %d] ERROR\033[0m\n", __func__, __LINE__);
        return -1;
    }

    ts_id = thiz_multistream_info.info[thiz_multistream_info.index].ts_id;
    pls_code = thiz_multistream_info.info[thiz_multistream_info.index].pls_code;
    GxFrontend_SetTsid(tuner, ts_id, pls_code);
    app_log_debug("\033[1;32m[%s]tsid:%d pls_code:%d\n\033[0m", __func__, ts_id, pls_code);

    return 0;
}

static bool _multistream_increase_ts_index(void)
{
    bool ret = false;

    if((thiz_multistream_info.total > 1)
            && (thiz_multistream_info.total > (thiz_multistream_info.index + 1)))
    {
        ret = true;
        thiz_multistream_info.index++;
    }

    return ret;
}

static bool _multistream_get_active_flag(void)
{
    bool ret = false;

    if((thiz_multistream_info.total > 1)
            && (thiz_multistream_info.total > thiz_multistream_info.index))
    {
        ret = true;
    }

    return ret;
}

static MultiStreamSearchClass thiz_multistream_ops = {
    .init              = _multistream_init,
    .exit              = _multistream_exit,
    .get_params        = _multistream_get_params,
    .set_params        = _multistream_set_params,
    .increase_ts_index = _multistream_increase_ts_index,
    .get_active_flag   = _multistream_get_active_flag,
};

void app_multistream_register(void)
{
    int32_t multistream_tag = 0;

    GxBus_ConfigGetInt(GXBUS_FRONTEND_MULTISTREAM, &multistream_tag, GXBUS_FRONTEND_MULTISTREAM_OFF);

    if(GXBUS_FRONTEND_MULTISTREAM_ON == multistream_tag)
        GxSearchRegistMultiStream(&thiz_multistream_ops);

    return;
}

void app_multistream_unregister(void)
{
    int32_t multistream_tag = 0;

    GxBus_ConfigGetInt(GXBUS_FRONTEND_MULTISTREAM, &multistream_tag, GXBUS_FRONTEND_MULTISTREAM_OFF);

    if(GXBUS_FRONTEND_MULTISTREAM_ON == multistream_tag)
        GxSearchUnregistMultiStream();

    return;
}
#endif
