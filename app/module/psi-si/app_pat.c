#include "app_module.h"
#define PAT_VALID_VERSION 32
#define PAT_VALID_TSID 65536

#if PAT_SUPPORT
static uint8_t pat_id = 0;
static status_t app_pat_get(void *data,psi_module_status state)
{
    uint16_t ts_id = 0;
    uint8_t version = 0;
    PatInfo *pat_info = (PatInfo*)data;
    ts_id = pat_info->si_info.ts_id;
    version = pat_info->si_info.ver_number;
    if(state == PSI_MODULE_TABLE_OK)
    {
        psi_module_filter params = {0};
        params.pid = PAT_PID;
        params.match[0] = PAT_TID;
        params.mask[0] = 0xff;
        params.depth = 6;
        params.match[3] = (ts_id>>8);
        params.mask[3] = 0xff;
        params.match[4] = (ts_id & 0xff);
        params.mask[4] = 0xff;
        params.match[5] = (version<<1);
        params.mask[5] = 0x3e;
        params.flags &= (~EQ_FLAG);
        params.flags |= CRC_FLAG;
        app_psi_module_modify(pat_id,params);
    }
    return GXCORE_SUCCESS;
}

void app_pat_stop(void)
{
    app_psi_module_stop(pat_id);
    pat_id = 0;
}

void app_pat_start(uint16_t demux_id,uint16_t ts_id,uint8_t version)
{
    if(pat_id  != 0)
    {
        app_psi_module_stop(pat_id);
        pat_id = 0;
    }
    psi_module_config config={0};
    config.demux_id = demux_id;

    config.params.pid = PAT_PID;
    config.params.match[0] = PAT_TID;
    config.params.mask[0] = 0xff;
    if(version < PAT_VALID_VERSION && ts_id < PAT_VALID_TSID)
    {
        config.params.depth = 6;
        config.params.match[3] = (ts_id>>8);
        config.params.mask[3] = 0xff;
        config.params.match[4] = (ts_id & 0xff);
        config.params.mask[4] = 0xff;
        config.params.match[5] = (version<<1);
        config.params.mask[5] = 0x3e;
        config.params.flags &= (~EQ_FLAG);
    }
    else if(version >= PAT_VALID_VERSION)
    {
        if(ts_id < PAT_VALID_TSID)
        {
            config.params.depth = 5;
            config.params.match[3] = (ts_id>>8);
            config.params.mask[3] = 0xff;
            config.params.match[4] = (ts_id & 0xff);
            config.params.mask[4] = 0xff;
        }
        else
        {
            config.params.depth = 1;
        }
        config.params.flags |= EQ_FLAG;
    }
    config.params.flags |= CRC_FLAG;
    config.node.no_stop = true;
    config.node.mode = PSI_MODULE_STANDARD_ONLY;
    config.node.parse_priv_cb = NULL;
    config.node.parse_solv_cb = app_pat_get;
    pat_id = app_psi_module_start(config);
}
#endif

