#include "app_module.h"
#include "app_send_msg.h"
#include "app_lcn.h"
#if DEMOD_DVB_T || DEMOD_ISDBT
#include "app_rf_channels.h"
#include "app_t2_area_config.h"
#include "app_dvbt2_search.h"
#endif
#if LCN_SUPPORT

typedef struct LCNControl
{
    struct gxlist_head  lcn_info_list;
    uint32_t            cur_tp_id;
    uint32_t            cur_frequency;
    uint16_t            cur_strength;
    uint16_t            cur_quality;
    uint32_t            lcn_updata;
    int32_t             lcn_config;
#if LCN_CONFLICT_HANDLE_SUPPORT
    struct gxlist_head  lcn_confl_list;
    ConflictHandleCb    conflict_handle_cb;
    LCNValidCheck       lcn_valid_check;
    LCNSortCb           lcn_sort_cb;
    uint8_t            *lcn_vir_flag;
#endif
}LCNCtrl;

static LCNCtrl s_LCNCtrl = {
    .cur_tp_id      = 0,
    .cur_frequency  = 0,
    .cur_strength   = 0,
    .cur_quality    = 0,
    .lcn_updata     = 0,
    .lcn_config     = 0,
#if LCN_CONFLICT_HANDLE_SUPPORT
    .conflict_handle_cb = NULL,
    .lcn_valid_check = NULL,
    .lcn_sort_cb = NULL,
    .lcn_vir_flag = NULL,
#endif
};

#if LCN_CONFLICT_HANDLE_SUPPORT
static void _lcn_conflict_check(LCNInfoOne_t *data);

void app_lcn_conflict_handle_register(ConflictHandleCb handle_conf_cb)
{
    s_LCNCtrl.conflict_handle_cb = handle_conf_cb;
}

void app_lcn_valid_check_register(LCNValidCheck lcn_valid_check)
{
    s_LCNCtrl.lcn_valid_check = lcn_valid_check;
}

void app_lcn_conflict_sort_method_register(LCNSortCb lcn_sort_cb)
{
    s_LCNCtrl.lcn_sort_cb = lcn_sort_cb;
}

int32_t app_lcn_valid_check_exec(LCNInfoOne_t *lcnOne)
{
    int32_t ret = 0;

    if(s_LCNCtrl.lcn_valid_check)
        ret = s_LCNCtrl.lcn_valid_check(lcnOne, &s_LCNCtrl.lcn_info_list);
    return ret;
}

int32_t app_lcn_handle_conflict_exec(void)
{
    int32_t ret = 0;
    LCNInfoOne_t *p = NULL;

    if(1 == gxlist_empty(&s_LCNCtrl.lcn_info_list)
            || (NULL == s_LCNCtrl.lcn_info_list.prev && NULL == s_LCNCtrl.lcn_info_list.next))
        return -1;

    gxlist_for_each_entry(p, &s_LCNCtrl.lcn_info_list, list)
    {
        _lcn_conflict_check(p);
    }
    if(s_LCNCtrl.lcn_sort_cb)
        s_LCNCtrl.lcn_sort_cb(&s_LCNCtrl.lcn_confl_list);

    if(s_LCNCtrl.conflict_handle_cb)
        ret = s_LCNCtrl.conflict_handle_cb(&s_LCNCtrl.lcn_info_list, &s_LCNCtrl.lcn_confl_list);

    return ret;
}

static bool _lcn_conflict_exist_check(LCNConflInfoOne_t *lcn_confl, LCNInfoOne_t *data)
{
    LCNConflProgInfoOne_t *p = NULL;

    gxlist_for_each_entry(p, &lcn_confl->prog_list, list)
    {
        if(p->original_id == data->original_id
                && p->ts_id == data->ts_id
                && p->service_id == data->service_id
                && p->frequency == data->frequency)
            return true;
    }

    return false;
}

static void _lcn_conflict_insert_one(LCNConflInfoOne_t *lcn_confl, LCNInfoOne_t *data)
{
    LCNConflProgInfoOne_t *new = NULL;

    if(NULL == lcn_confl || NULL == data)
        return;

    new = GxCore_Calloc(1, sizeof(LCNConflProgInfoOne_t));
    if(NULL == new)
    {
        app_log_error("no enough memory\n");
        return;
    }

    new->original_id = data->original_id;
    new->ts_id = data->ts_id;
    new->service_id = data->service_id;
    new->frequency = data->frequency;
    new->freq_strength = data->freq_strength;
    new->freq_quality = data->freq_quality;
    new->service_type = data->service_type;
    gxlist_add_tail(&new->list, &lcn_confl->prog_list);
}

static void _lcn_conflict_add(LCNInfoOne_t *data)
{
    LCNConflInfoOne_t *p = NULL;
    bool found = false;

    if((NULL == s_LCNCtrl.lcn_confl_list.next && NULL == s_LCNCtrl.lcn_confl_list.prev)
            || NULL == data)
        return;

    gxlist_for_each_entry(p, &s_LCNCtrl.lcn_confl_list, list)
    {
        if(p->lcn == data->lcn)
        {
            if(false == _lcn_conflict_exist_check(p, data))
                _lcn_conflict_insert_one(p, data);

            found = true;
            break;
        }
    }

    if(false == found)
    {
        p = GxCore_Calloc(1, sizeof(LCNConflInfoOne_t));
        if(NULL == p)
        {
            app_log_error("no enough memory\n");
            return;
        }
        GX_INIT_LIST_HEAD(&p->prog_list);
        p->lcn = data->lcn;
        gxlist_add_tail(&p->list, &s_LCNCtrl.lcn_confl_list);
        _lcn_conflict_insert_one(p, data);
    }

}

static void _lcn_conflict_check(LCNInfoOne_t *data)
{
    LCNInfoOne_t *p = NULL;

    gxlist_for_each_entry(p, &s_LCNCtrl.lcn_info_list, list)
    {
        if(memcmp(p, data, sizeof(LCNInfoOne_t)) && p->service_type == data->service_type && p->lcn == data->lcn && p->lcn != INVALID_LCN)
        {
            _lcn_conflict_add(data);
            _lcn_conflict_add(p);
        }
    }
}

static void _lcn_confl_list_clear(void)
{
    LCNConflInfoOne_t *p = NULL;
    LCNConflInfoOne_t *p_next = NULL;
    LCNConflProgInfoOne_t *c = NULL;
    LCNConflProgInfoOne_t *c_next = NULL;

    if(1 == gxlist_empty(&s_LCNCtrl.lcn_confl_list)
            || (NULL == s_LCNCtrl.lcn_confl_list.next && NULL == s_LCNCtrl.lcn_confl_list.prev))
        return;

    gxlist_for_each_entry_safe(p, p_next, &s_LCNCtrl.lcn_confl_list, list)
    {
        gxlist_for_each_entry_safe(c, c_next, &p->prog_list, list)
        {
            gxlist_del(&c->list);
            APP_FREE(c);
        }
        gxlist_del(&p->list);
        APP_FREE(p);
    }

    GX_INIT_LIST_HEAD(&s_LCNCtrl.lcn_confl_list);
}
#endif
static uint8_t app_lcn_list_check_one_exist(LCNInfoOne_t* data)
{
    LCNInfoOne_t *p = NULL;

    if(NULL == data)
        return 0;

    gxlist_for_each_entry(p, &s_LCNCtrl.lcn_info_list, list)
    {
        if(p->service_id == data->service_id
                && p->ts_id == data->ts_id
                && p->original_id == data->original_id
                && p->frequency == data->frequency)
            return 1;
    }

    return 0;
}

static void _lcn_list_update_one(LCNInfoOne_t *data)
{
    LCNInfoOne_t *p = NULL;

    if(NULL == data)
        return;

    gxlist_for_each_entry(p, &s_LCNCtrl.lcn_info_list, list)
    {
        if(p->service_id == data->service_id
                && p->ts_id == data->ts_id
                && p->original_id == data->original_id
                && p->frequency == data->frequency)
        {
            if(p->lcn_tag == 0x88)
                return;
            p->lcn_tag = data->lcn_tag;
            p->lcn = data->lcn;
#if LCN_VISIBLE_FLAG_SUPPORT
            p->visible_flag = data->visible_flag;
#endif
            s_LCNCtrl.lcn_updata = 1;
            break;
        }
    }
}

static void _lcn_list_insert_one(LCNInfoOne_t* data)
{
    LCNInfoOne_t *p = NULL;

    if(NULL == data)
        return;

    p = GxCore_Calloc(1, sizeof(LCNInfoOne_t));
    if(NULL == p)
        return;

    p->original_id      = data->original_id;
    p->ts_id            = data->ts_id;
    p->service_id       = data->service_id;
    p->lcn              = data->lcn;
#if LCN_VISIBLE_FLAG_SUPPORT
    p->visible_flag = data->visible_flag;
#endif
    p->tp_id            = data->tp_id;
    p->frequency        = data->frequency;
    p->freq_quality     = data->freq_quality;
    p->freq_strength    = data->freq_strength;
    p->ref_service_id   = data->ref_service_id;
    p->nvod_service_id  = data->nvod_service_id;
    p->network_id       = data->network_id;
    p->service_type     = data->service_type;
    p->lcn_tag          = data->lcn_tag;
    if(strlen((char *)data->prog_name) > 0)
        memcpy(p->prog_name, data->prog_name, MAX_PROG_NAME);

    gxlist_add_tail(&p->list, &s_LCNCtrl.lcn_info_list);
    s_LCNCtrl.lcn_updata = 1;
}

void app_lcn_list_add_one(LCNInfoOne_t *data)
{
    if((NULL == s_LCNCtrl.lcn_info_list.prev && NULL == s_LCNCtrl.lcn_info_list.next) || NULL == data)
        return;

    if(0 == app_lcn_list_check_one_exist(data))
        _lcn_list_insert_one(data);
    else
        _lcn_list_update_one(data);

}

void app_lcn_list_add_noexist_one(LCNInfoOne_t *data)
{
    LCNInfoOne_t *p = NULL;

    if((NULL == s_LCNCtrl.lcn_info_list.prev && NULL == s_LCNCtrl.lcn_info_list.next)
            || NULL == data)
        return;

    if(0 == app_lcn_list_check_one_exist(data))
    {
        _lcn_list_insert_one(data);
    }
    else
    {
        gxlist_for_each_entry(p, &s_LCNCtrl.lcn_info_list, list)
        {
            if(p->service_id == data->service_id
                    && p->ts_id == data->ts_id
                    && p->original_id == data->original_id
                    && p->frequency == data->frequency)
            {
                p->service_type = data->service_type;
                p->freq_strength = data->freq_strength;
                p->freq_quality = data->freq_quality;
                p->ref_service_id   = data->ref_service_id;
                if(strlen((char *)data->prog_name) > 0)
                    memcpy(p->prog_name, data->prog_name, MAX_PROG_NAME);
                break;
            }
        }
    }
}

static bool _sat_id_match(uint32_t *sat_id_array, uint32_t num, uint32_t sat_id)
{
    uint32_t i = 0;

    if(0 == num || 0 == sat_id || NULL == sat_id_array)
        return false;

    for(i = 0; i < num; i++)
    {
        if(sat_id_array[i] == sat_id)
            return true;
    }

    return false;
}

void app_lcn_list_init(uint32_t *sat_id_array, uint32_t num)
{
    int16_t prog_count = 0;
    int16_t prog_pos = 0;
    GxBusPmViewInfo sys;
    GxBusPmDataProg prog_arry;
    GxBusPmDataTP tp_data;
    LCNInfoOne_t data;
    AppProgExtInfo prog_ext_info;

    if(!(1 == gxlist_empty(&s_LCNCtrl.lcn_info_list)
            || (NULL == s_LCNCtrl.lcn_info_list.prev && NULL == s_LCNCtrl.lcn_info_list.next)))
    {
        app_log_error("lcn list has been inited\n");
        return;
    }

    memset(&prog_ext_info, 0, sizeof(AppProgExtInfo));
    GxBus_PmViewInfoGet(&sys);

    sys.stream_type = GXBUS_PM_PROG_ALL;
    sys.group_mode = GROUP_MODE_ALL;
    if(TAXIS_MODE_NON != sys.taxis_mode)
        sys.taxis_mode = TAXIS_MODE_NON;
    GxBus_PmViewInfoMemSet(&sys);

    s_LCNCtrl.cur_tp_id = 0;
    s_LCNCtrl.cur_frequency = 0;
    s_LCNCtrl.lcn_updata = 0;
    GX_INIT_LIST_HEAD(&s_LCNCtrl.lcn_info_list);
#if LCN_CONFLICT_HANDLE_SUPPORT
    GX_INIT_LIST_HEAD(&s_LCNCtrl.lcn_confl_list);
    s_LCNCtrl.lcn_vir_flag = GxCore_Calloc(sizeof(uint8_t), 2 * MAX_VIRTUAL_LCN_NUM);
#endif

    prog_count = GxBus_PmProgNumGet();

    if(0 == prog_count)
    {
        GxBus_PmViewInfoMemClear();
        return;
    }

    for(prog_pos = 0; prog_pos < prog_count; prog_pos++)
    {
        memset(&data, 0, sizeof(LCNInfoOne_t));

        if(1 == GxBus_PmProgGetByPos(prog_pos, 1, &prog_arry)
                && GXCORE_SUCCESS == GxBus_PmTpGetById(prog_arry.tp_id, &tp_data)
                && true == _sat_id_match(sat_id_array, num, prog_arry.sat_id))
        {
            data.original_id = prog_arry.original_id;
            data.ts_id = prog_arry.ts_id;
            data.service_id = prog_arry.service_id;
            data.frequency = tp_data.frequency;
            data.tp_id = prog_arry.tp_id;
            data.service_type = prog_arry.service_type;
            memcpy(data.prog_name, prog_arry.prog_name, MAX_PROG_NAME);

            if(GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(prog_arry.id, sizeof(AppProgExtInfo), &prog_ext_info))
            {
                data.lcn = prog_ext_info.logicnum;
                data.network_id = prog_ext_info.networkid;
#if LCN_VISIBLE_FLAG_SUPPORT
                data.visible_flag = prog_ext_info.visible_flag;
#endif
            }
            else
            {
                data.lcn = INVALID_LCN;
                data.network_id = INVALID_NETWORK_ID;
#if LCN_VISIBLE_FLAG_SUPPORT
                data.visible_flag = LCN_VISIBLE;
#endif
            }
            _lcn_list_insert_one(&data);
        }
    }

    GxBus_PmViewInfoMemClear();
    return;
}

static void _lcn_list_clear(void)
{
    LCNInfoOne_t *p = NULL;
    LCNInfoOne_t *p_next = NULL;
    LCNCtrl *ctrl = &s_LCNCtrl;

    if(1 == gxlist_empty(&ctrl->lcn_info_list)
            || (NULL == ctrl->lcn_info_list.prev && NULL == ctrl->lcn_info_list.next))
        return;

    gxlist_for_each_entry_safe(p, p_next, &ctrl->lcn_info_list, list)
    {
        gxlist_del(&p->list);
        APP_FREE(p);
    }
    GX_INIT_LIST_HEAD(&ctrl->lcn_info_list);
}

void app_lcn_list_destroy(void)
{
    _lcn_list_clear();
#if LCN_CONFLICT_HANDLE_SUPPORT
    _lcn_confl_list_clear();
    APP_FREE(s_LCNCtrl.lcn_vir_flag);
#endif
    s_LCNCtrl.cur_tp_id = 0;
    s_LCNCtrl.cur_frequency = 0;
    s_LCNCtrl.lcn_updata = 0;
    s_LCNCtrl.cur_strength = 0;
    s_LCNCtrl.cur_quality = 0;
}

void app_lcn_searching_tp_id_set(uint32_t tp_id)
{
    s_LCNCtrl.cur_tp_id = tp_id;
}

uint32_t app_lcn_searching_tp_id_get(void)
{
    uint32_t tp_id = 0;

    tp_id = s_LCNCtrl.cur_tp_id;

    return tp_id;
}

void app_lcn_searching_frequency_set(uint32_t frequency)
{
    s_LCNCtrl.cur_frequency = frequency;
}

void app_lcn_searching_SQ_set(uint16_t strength, uint16_t quality)
{
    s_LCNCtrl.cur_strength = strength;
    s_LCNCtrl.cur_quality = quality;
}

uint32_t app_lcn_searching_frequency_get(void)
{
    uint32_t frequency = 0;

    frequency = s_LCNCtrl.cur_frequency;
    return frequency;
}

void app_lcn_searching_SQ_get(uint16_t *strength, uint16_t *quality)
{
    unsigned int value = 0;
    uint32_t tuner = app_tuner_cur_tuner_get();

    if(s_LCNCtrl.cur_strength == 0)
    {
        app_ioctl(tuner, FRONTEND_STRENGTH_GET, &value);
        s_LCNCtrl.cur_strength = value;
    }
    if(s_LCNCtrl.cur_quality == 0)
    {
        app_ioctl(tuner, FRONTEND_QUALITY_GET, &value);
        s_LCNCtrl.cur_quality = value;
    }

    *strength = s_LCNCtrl.cur_strength;
    *quality = s_LCNCtrl.cur_quality;
}

uint32_t app_lcn_list_lcn_get(uint32_t original_id, uint32_t ts_id, uint32_t service_id, uint32_t frequency)
{
    uint32_t lcn = INVALID_LCN;
    LCNInfoOne_t *p = NULL;
    LCNCtrl *ctrl = &s_LCNCtrl;

    if(1 == gxlist_empty(&ctrl->lcn_info_list)
            || (NULL == ctrl->lcn_info_list.prev && NULL == ctrl->lcn_info_list.next))
        return INVALID_LCN;

    gxlist_for_each_entry(p, &ctrl->lcn_info_list, list)
    {
        if(p->service_id == service_id
                && p->ts_id == ts_id
                && p->original_id == original_id
                && p->frequency == frequency)
        {
            lcn = p->lcn;
            break;
        }
    }

    return lcn;
}

uint32_t app_lcn_update_flag_get(void)
{
    uint32_t flag = 0;

    flag = s_LCNCtrl.lcn_updata;
    return flag;
}

int32_t app_lcn_list_info_get(uint32_t original_id, uint32_t ts_id, uint32_t service_id,
                            uint32_t frequency, LCNInfoOne_t *lcn)
{
    int32_t ret = -1;
    LCNInfoOne_t *p = NULL;

    if(1 == gxlist_empty(&s_LCNCtrl.lcn_info_list) || NULL == lcn
            || (NULL == s_LCNCtrl.lcn_info_list.prev && NULL == s_LCNCtrl.lcn_info_list.next))
        return -1;

    gxlist_for_each_entry(p, &s_LCNCtrl.lcn_info_list, list)
    {
        if(p->original_id == original_id
                && p->ts_id == ts_id
                && p->service_id == service_id
                && p->frequency == frequency)
        {
            memcpy(lcn, p, sizeof(LCNInfoOne_t));
            ret = 0;
            break;
        }
    }
    return ret;
}

void app_lcn_update_flag_set(uint32_t flag)
{
    s_LCNCtrl.lcn_updata = flag;
}

int32_t app_lcn_tag82_83_parse(uint8_t *p_section_data, uint32_t len,
                                uint32_t original_id, uint32_t network_id, uint32_t ts_id)
{
    LCNInfoOne_t lcnOne;
    uint32_t LcnLength = len/4;
    uint32_t i;

    for(i=0; i<LcnLength; i++)
    {
        memset(&lcnOne,0,sizeof(LCNInfoOne_t));
        lcnOne.original_id = original_id;
        lcnOne.network_id = network_id;
        lcnOne.service_id = ((p_section_data[2+i*4]<<8)&0xff00) +p_section_data[3+i*4];
#if LCN_VISIBLE_FLAG_SUPPORT
        uint16_t valid;
        valid = (p_section_data[4 + i*4] & 0x80) >> 7;
        if(valid == 0)
        {
            lcnOne.visible_flag = LCN_INVISIBLE;
        }
        else
        {
            lcnOne.visible_flag = LCN_VISIBLE;
        }
#endif
        lcnOne.lcn = ((p_section_data[4+i*4]<<8)&0xff00) +p_section_data[5+i*4];
        lcnOne.lcn &= 0x3ff;
        lcnOne.ts_id = ts_id;
        lcnOne.tp_id = app_lcn_searching_tp_id_get();
        lcnOne.frequency = app_lcn_searching_frequency_get();
        lcnOne.lcn_tag = p_section_data[0];
        app_lcn_searching_SQ_get(&(lcnOne.freq_strength), &(lcnOne.freq_quality));
#if LCN_CONFLICT_HANDLE_SUPPORT
        app_lcn_valid_check_exec(&lcnOne);
#endif
        app_lcn_list_add_one(&lcnOne);
    }
    return 0;
}

int32_t app_lcn_tag87_parse(uint8_t *data, uint32_t len,
                            uint32_t original_id, uint32_t network_id, uint32_t ts_id)
{
    LCNInfoOne_t lcnOne;
    int32_t desc_len = len;
    uint8_t channel_list_name_length = 0;
    int8_t descriptor_length2 = 0;
    uint32_t a = 2, b = 0;
    int skip_length = 0;

    while(desc_len >= 4)
    {
        channel_list_name_length = data[a + 1];
        descriptor_length2 = data[a + 1 + 1 + channel_list_name_length + 3];
        while (descriptor_length2 >= 4)
        {
            uint16_t logic_num, valid;
            int32_t temp = 0, temp2 = 0;
            uint16_t service_id = 0;

            temp = a + 1 + 1 + channel_list_name_length + 3 + 1 + b;
            temp2 = a + 1 + 1 + channel_list_name_length + 3 + 1 + b + 1;
            service_id = (data[temp] << 8 | data[temp2]);

            temp = a + 1 + 1 + channel_list_name_length + 3 + 1 + b + 2;
            temp2 = a + 1 + 1 + channel_list_name_length + 3 + 1 + b + 3;
            valid = (data[temp] & 0x80) >> 7;
            logic_num = ((data[temp] & 0x03) << 8 | data[temp2]);

            b += 4;
            descriptor_length2 -= 4;

            memset(&lcnOne,0,sizeof(LCNInfoOne_t));
            lcnOne.original_id = original_id;
            lcnOne.network_id = network_id;
            lcnOne.service_id = service_id;
            lcnOne.lcn = logic_num;
            lcnOne.ts_id = ts_id;
            lcnOne.tp_id = app_lcn_searching_tp_id_get();
            lcnOne.frequency = app_lcn_searching_frequency_get();
#if LCN_VISIBLE_FLAG_SUPPORT
            if(valid == 0)
            {
                lcnOne.visible_flag = LCN_INVISIBLE;
            }
            else
            {
                lcnOne.visible_flag = LCN_VISIBLE;
            }
#endif
            lcnOne.lcn_tag = data[0];
            app_lcn_searching_SQ_get(&(lcnOne.freq_strength), &(lcnOne.freq_quality));

#if LCN_CONFLICT_HANDLE_SUPPORT
            app_lcn_valid_check_exec(&lcnOne);
#endif
            app_lcn_list_add_one(&lcnOne);
        }
        skip_length = 1 + 1 + channel_list_name_length + 3 + 1 + b;
        a += skip_length;
        desc_len -= skip_length;
    }
    return 0;
}

int32_t app_lcn_tag88_parse(uint8_t *data, uint32_t len,
                            uint32_t original_id, uint32_t network_id, uint32_t ts_id)
{
    LCNInfoOne_t lcnOne;
    int32_t descriptor_length = len;
    uint32_t a = 0;

    while (descriptor_length >= 4)
    {
        uint32_t logic_num, valid;
        uint32_t service_id = (uint32_t)(data[a] << 8 | data[a + 1]);

        valid = (data[a + 2] & 0x80) >> 7;
        logic_num = (uint32_t)((data[a + 2] & 0x03) << 8 | data[a + 3]);
        a += 4;
        descriptor_length -= 4;

        memset(&lcnOne,0,sizeof(LCNInfoOne_t));
        lcnOne.original_id = original_id;
        lcnOne.network_id = network_id;
        lcnOne.service_id = service_id;
#if LCN_VISIBLE_FLAG_SUPPORT
        if(valid == 0)
        {
            lcnOne.visible_flag = LCN_INVISIBLE;
        }
        else
        {
            lcnOne.visible_flag = LCN_VISIBLE;
        }
#endif
        lcnOne.lcn = logic_num;
        lcnOne.ts_id = ts_id;
        lcnOne.tp_id = app_lcn_searching_tp_id_get();
        lcnOne.frequency = app_lcn_searching_frequency_get();
        lcnOne.lcn_tag = data[0];
        app_lcn_searching_SQ_get(&(lcnOne.freq_strength), &(lcnOne.freq_quality));

#if LCN_CONFLICT_HANDLE_SUPPORT
        app_lcn_valid_check_exec(&lcnOne);
#endif
        app_lcn_list_add_one(&lcnOne);
    }
    return 0;
}

static bool _tp_cmp(GxBusPmDataSatType type, GxBusPmDataTP *tp, LCNInfoOne_t *p)
{
    bool ret = false;

    if(NULL == tp)
        return false;

    if(GXBUS_PM_SAT_S == type)
    {
        if(p->tp_id == tp->id)
            ret = true;
    }
    else if(GXBUS_PM_SAT_C == type)
    {
        if(p->frequency > tp->frequency - 3 && p->frequency < tp->frequency + 3)
            ret = true;
    }
    else if(GXBUS_PM_SAT_T == type || GXBUS_PM_SAT_DVBT2 == type)
    {
        if(tp->frequency == p->frequency)
            ret = true;
    }

    return ret;
}

#define MAX_DELETE_PROG_NUM (128)

void app_lcn_list_info_save(void)
{
    int16_t prog_count = 0;
    int16_t prog_pos = 0;
    GxBusPmDataProg prog_data;
    GxBusPmDataTP tp_data;
    GxBusPmDataSat sat_data;
    LCNInfoOne_t *p = NULL;
    AppProgExtInfo prog_ext_info;
    GxBusPmViewInfo sys;
    uint32_t delete_prog[MAX_DELETE_PROG_NUM] = {0};
    GxMsgProperty_NodeDelete node_delete = {0};

    if((NULL == s_LCNCtrl.lcn_info_list.prev && NULL == s_LCNCtrl.lcn_info_list.next)
            || 1 == gxlist_empty(&s_LCNCtrl.lcn_info_list))
        return;

    node_delete.id_array = delete_prog;
    node_delete.node_type = NODE_PROG;

    GxBus_PmViewInfoGet(&sys);
    sys.stream_type = GXBUS_PM_PROG_ALL;
    sys.group_mode = GROUP_MODE_ALL;
    if(TAXIS_MODE_NON != sys.taxis_mode)
        sys.taxis_mode = TAXIS_MODE_NON;
    GxBus_PmViewInfoMemSet(&sys);

    prog_count = GxBus_PmProgNumGet();

    for(prog_pos = 0; prog_pos < prog_count; prog_pos++)
    {
        if(GxBus_PmProgGetByPos(prog_pos, 1, &prog_data) != 1)
            continue;

        if(GXCORE_ERROR == GxBus_PmTpGetById(prog_data.tp_id, &tp_data))
            continue;

        if(GXCORE_ERROR == GxBus_PmSatGetById(prog_data.sat_id, &sat_data))
            continue;

        gxlist_for_each_entry(p, &s_LCNCtrl.lcn_info_list, list)
        {
            if(p->service_id == prog_data.service_id
                    && p->ts_id == prog_data.ts_id
                    && p->original_id == prog_data.original_id
                    && true == _tp_cmp(sat_data.type, &tp_data, p))
            {
            #if SEARCH_SHOW_LCN_SUPPORT
                memset(&prog_ext_info, 0, sizeof(AppProgExtInfo));

                GxBus_PmProgExtInfoGet(prog_data.id, sizeof(AppProgExtInfo), &prog_ext_info);
                if(p->lcn != INVALID_LCN)
                    prog_ext_info.logicnum = p->lcn;
                else
                    prog_ext_info.logicnum = p->service_id;
#if LCN_VISIBLE_FLAG_SUPPORT
                prog_ext_info.visible_flag = p->visible_flag;
#endif
                prog_ext_info.networkid = p->network_id;
                GxBus_PmProgExtInfoAdd(prog_data.id, sizeof(AppProgExtInfo), &prog_ext_info);
                break;
            #else
                if((p->lcn != INVALID_LCN)&&(p->lcn != INVALID_PROG))
                {
                    memset(&prog_ext_info, 0, sizeof(AppProgExtInfo));

                    GxBus_PmProgExtInfoGet(prog_data.id, sizeof(AppProgExtInfo), &prog_ext_info);
                    prog_ext_info.logicnum = p->lcn;
#if LCN_VISIBLE_FLAG_SUPPORT
                    prog_ext_info.visible_flag = p->visible_flag;
#endif
                    prog_ext_info.networkid = p->network_id;
                    GxBus_PmProgExtInfoAdd(prog_data.id, sizeof(AppProgExtInfo), &prog_ext_info);
                    break;
                }
                else if(p->lcn == INVALID_PROG)
                {
                    /*delete invalid prog from dbase */
                    if(node_delete.num < MAX_DELETE_PROG_NUM)
                    {
                        node_delete.id_array[node_delete.num] = prog_data.id;
                        node_delete.num++;
                    }
                }
            #endif
            }
        }
    }
    GxBus_PmSync(GXBUS_PM_SYNC_EXT_PROG);
    if(node_delete.num > 0)
    {
        app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node_delete);
        GxBus_PmSync(GXBUS_PM_SYNC_PROG);
    }
    GxBus_PmViewInfoMemClear();
    return;
}

int32_t app_lcn_config_get(void)
{
    int32_t lcn_config;

    GxBus_ConfigGetInt(LCN_KEY, &lcn_config, LCN_VALUE);
    return lcn_config;
}

#if LCN_CONFLICT_HANDLE_SUPPORT
static void _lcn_conflict_invalid_delete(struct gxlist_head *lcn_confl_list, LCNInfoOne_t *lcn_info)
{
    LCNConflInfoOne_t *p = NULL;
    LCNConflProgInfoOne_t *c = NULL, *c_next = NULL;

    gxlist_for_each_entry(p, lcn_confl_list, list)
    {
        if(p->lcn == lcn_info->lcn)
        {
            gxlist_for_each_entry_safe(c, c_next, &p->prog_list, list)
            {
                if(c->original_id == lcn_info->original_id
                        && c->ts_id == lcn_info->ts_id
                        && c->service_id == lcn_info->service_id
                        && c->frequency == lcn_info->frequency)
                {
                    gxlist_del(&c->list);
                    APP_FREE(c);
                    break;
                }
            }
            break;
        }
    }
}

static void _lcn_allocate_virtual_lcn_to_invalid_prog(struct gxlist_head *lcn_info_list)
{
    int32_t i = 0;
    uint8_t *vlcn_array = NULL;
    LCNInfoOne_t *lcn_pos = NULL;

    gxlist_for_each_entry(lcn_pos, lcn_info_list, list)
    {
        if(lcn_pos->service_type != GXBUS_PM_PROG_TV && lcn_pos->service_type != GXBUS_PM_PROG_RADIO)
            continue;
        if(INVALID_LCN == lcn_pos->lcn)
        {
            if(GXBUS_PM_PROG_TV == lcn_pos->service_type)
                vlcn_array = s_LCNCtrl.lcn_vir_flag;
            else
                vlcn_array = s_LCNCtrl.lcn_vir_flag + MAX_VIRTUAL_LCN_NUM;

            for(i = 0; i < MAX_VIRTUAL_LCN_NUM; i++)
            {
                if(vlcn_array[i] != 1)
                {
                    lcn_pos->lcn = VIRTUAL_LCN_START + i;
                    vlcn_array[i] = 1;
                    app_lcn_update_flag_set(1);
                    break;
                }
            }
        }
    }
}

static void _lcn_conflict_pre_handle(struct gxlist_head *lcn_info_list, struct gxlist_head *lcn_confl_list)
{
    uint8_t *vlcn_array = NULL;
    LCNInfoOne_t *lcn_pos = NULL, *lcn_pos_next = NULL;

    gxlist_for_each_entry_safe(lcn_pos, lcn_pos_next, lcn_info_list, list)
    {
        if(lcn_pos->service_type != GXBUS_PM_PROG_TV && lcn_pos->service_type != GXBUS_PM_PROG_RADIO)
            continue;

        if(0 == strlen((char *)lcn_pos->prog_name))
        {
            _lcn_conflict_invalid_delete(lcn_confl_list, lcn_pos);
            gxlist_del(&lcn_pos->list);
            APP_FREE(lcn_pos);
            continue;
        }

        if(lcn_pos->lcn >= VIRTUAL_LCN_START && lcn_pos->lcn < VIRTUAL_LCN_START + MAX_VIRTUAL_LCN_NUM)
        {
            if(GXBUS_PM_PROG_TV == lcn_pos->service_type)
                vlcn_array = s_LCNCtrl.lcn_vir_flag;
            else
                vlcn_array = s_LCNCtrl.lcn_vir_flag + MAX_VIRTUAL_LCN_NUM;
            vlcn_array[lcn_pos->lcn - VIRTUAL_LCN_START] = 1;
        }
    }
}

static void _lcn_value_set_invalid(struct gxlist_head *lcn_info_list, uint32_t original_id,
                                    uint32_t ts_id, uint32_t service_id, uint32_t frequency)
{
    LCNInfoOne_t *p = NULL;

    gxlist_for_each_entry(p, lcn_info_list, list)
    {
        if(p->original_id == original_id
                && p->ts_id == ts_id
                && p->service_id == service_id
                && p->frequency == frequency)
        {
            p->lcn = INVALID_LCN;
            break;
        }
    }

    return;
}

#if LCN_VALID_CHECK_BY_ONID
/* (Spain=8916, France=8442, UK=9018, Portugal=8904, Italy=29, 154, 272, 318, 3334 or 12291,
Germany = 8468),*/
static int32_t _lcn_valid_onid_check(LCNInfoOne_t *lcn, struct gxlist_head *lcn_info_list)
{
#if DEMOD_DVB_T
    LCNInfoOne_t *p = NULL;
    int32_t s_country = 0;
    s_country = AppDvbt2GetCountry();

    if(1 == gxlist_empty(lcn_info_list)
            || (NULL == lcn_info_list->prev && NULL == lcn_info_list->next))
    {
        return -1;
    }
    app_log_debug("lcn->lcn = %d lcn->original_id = %d [%d %d %d] \n",lcn->lcn,lcn->original_id,lcn->frequency ,lcn->freq_strength,lcn->freq_quality);
    gxlist_for_each_entry(p, lcn_info_list, list)
    {
        app_log_debug("[app_prog_onid_lcn_check ]lcn->lcn = %d p->lcn = %d p->original_id = %d [ %d %d %d] \n",lcn->lcn,p->lcn,p->original_id , p->frequency ,p->freq_strength,p->freq_quality);
        if(lcn->lcn == p->lcn)
        {
            /*check ONID*/
            if( 0 == strcmp(COUNTRY_ITALY, g_CountryName[s_country]) )
            {
                if(((lcn->original_id != 29) && (lcn->original_id != 154)&&(lcn->original_id != 272)&&(lcn->original_id != 318) &&(lcn->original_id != 3334)&&(lcn->original_id != 12291))
                    &&((p->original_id == 29) ||(p->original_id == 154) ||(p->original_id == 272) ||(p->original_id == 318)  ||(p->original_id == 3334) ||(p->original_id == 12291)))
                    lcn->lcn = INVALID_LCN;
                else   if(((p->original_id != 29) && (p->original_id != 154)&&(p->original_id != 272)&&(p->original_id != 318) &&(p->original_id != 3334)&&(p->original_id != 12291))
                    &&((lcn->original_id == 29) ||(lcn->original_id == 154) ||(lcn->original_id == 272) ||(lcn->original_id == 318)  ||(lcn->original_id == 3334) ||(lcn->original_id == 12291)))
                    p->lcn = INVALID_LCN;
                break;
            }
            else if( 0 == strcmp(COUNTRY_GERMANY, g_CountryName[s_country]) )
            {
                if((lcn->original_id != 8468)&&(p->original_id == 8468))
                    lcn->lcn = INVALID_LCN;
                else if((p->original_id != 8468)&&(lcn->original_id == 8468))
                    p->lcn = INVALID_LCN;
                break;
            }
            else if( 0 == strcmp(COUNTRY_FRANCE, g_CountryName[s_country]) )
            {
                if((lcn->original_id != 8442)&&(p->original_id == 8442))
                    lcn->lcn = INVALID_LCN;
                else if((p->original_id != 8442)&&(lcn->original_id == 8442))
                    p->lcn = INVALID_LCN;
                break;
            }
            else if( 0 == strcmp(COUNTRY_SPAIN, g_CountryName[s_country]) )
            {
                if((lcn->original_id != 8916)&&(p->original_id == 8916))
                    lcn->lcn = INVALID_LCN;
                else  if((p->original_id != 8916)&&(lcn->original_id == 8916))
                    p->lcn = INVALID_LCN;
                break;
            }
            else if( 0 == strcmp(COUNTRY_PORTUGAL, g_CountryName[s_country]) )
            {
                if((lcn->original_id != 8904)&&(p->original_id == 8904))
                    lcn->lcn = INVALID_LCN;
                else if((p->original_id != 8904)&&(lcn->original_id == 8904))
                    p->lcn = INVALID_LCN;
                break;
            }
            else if( 0 == strcmp(COUNTRY_UK, g_CountryName[s_country]) )
            {
                if((lcn->original_id != 9018)&&(p->original_id == 9018))
                    lcn->lcn = INVALID_LCN;
                else if((p->original_id != 9018)&&(lcn->original_id == 9018))
                    p->lcn = INVALID_LCN;
                break;
            }
        }
    }
#endif
    return 0;
}
#elif LCN_VALID_CHECK_BY_NID
static int32_t _lcn_valid_nid_check(LCNInfoOne_t *lcn, struct gxlist_head *lcn_info_list)
{
    LCNInfoOne_t *p = NULL;

    if(1 == gxlist_empty(lcn_info_list)
            || (NULL == lcn_info_list->prev && NULL == lcn_info_list->next))
    {
        return -1;
    }

    gxlist_for_each_entry(p, lcn_info_list, list)
    {
        if(lcn->lcn == p->lcn)
        {
            if((lcn->network_id >= 0x3001 && lcn->network_id <= 0x3100) && (0x3001 > p->network_id || p->network_id > 0x3100))
                p->lcn = INVALID_LCN;
            else if((lcn->network_id < 0x3001 || lcn->network_id > 0x3100) && (p->network_id >= 0x3001 && p->network_id <= 0x3100))
                lcn->lcn = INVALID_LCN;
            break;
        }
    }

    return 0;
}
#endif

#if !LCN_CONFLIST_LIST_SORTED_BY_SEARCH_ORDER
static void _swap_lcn_confl_prog_info(LCNConflProgInfoOne_t *a, LCNConflProgInfoOne_t *b)
{
    LCNConflProgInfoOne_t tmp = *a;
    a->original_id = b->original_id;
    a->ts_id = b->ts_id;
    a->service_id = b->service_id;
    a->service_type = b->service_type;
    a->frequency = b->frequency;
    a->freq_quality = b->freq_quality;
    a->freq_strength = b->freq_strength;
    b->original_id = tmp.original_id;
    b->ts_id = tmp.ts_id;
    b->service_id = tmp.service_id;
    b->service_type = tmp.service_type;
    b->frequency = tmp.frequency;
    b->freq_quality = tmp.freq_quality;
    b->freq_strength = tmp.freq_strength;
}
#endif

#if LCN_CONFLICT_LIST_SORTED_BY_FRE_ORDER
static void _sort_conflict_list_by_frequency(struct gxlist_head *lcn_confl_list)
{
    LCNConflInfoOne_t *p = NULL, *p_next = NULL, *p_tmp = NULL;
    LCNConflProgInfoOne_t *c = NULL, *c_tmp = NULL;

    gxlist_for_each_entry_safe(p, p_next, lcn_confl_list, list)
    {
        gxlist_for_each_entry(c, &p->prog_list, list)
        {
            for(c_tmp = gxlist_entry(c->list.next, LCNConflProgInfoOne_t, list);
                    &c_tmp->list != &p->prog_list;
                    c_tmp = gxlist_entry(c_tmp->list.next, LCNConflProgInfoOne_t, list))
            {
                if(c->frequency > c_tmp->frequency)
                    _swap_lcn_confl_prog_info(c, c_tmp);
            }
        }

        for(p_tmp = p_next;
                &p_tmp->list != lcn_confl_list;
                p_tmp = gxlist_entry(p_tmp->list.next, LCNConflInfoOne_t, list))
        {
            if(p->lcn > p_tmp->lcn)
            {
                gxlist_del(&p->list);
                gxlist_add(&p->list, &p_tmp->list);
            }
        }
    }
}
#elif LCN_CONFLICT_LIST_SORTED_BY_RFCH_SQ_ORDER
/*return 0  lcn1 is best other wise lcn2 is best*/
/*
-        * If (Qn1 = Qn2) and (Sn1 = Sn2) then MUX1 is the best; (MUX1 is same as MUX2; MUX1 is arbitrarily chosen)
-        * If (Qn1 = Qn2) and (Sn1 > Sn2) then MUX1 is the best;
-        * If (Qn1 = Qn2) and (Sn1 < Sn2) then MUX2 is the best;
-
-        * If (Qn1 > Qn2) and (Sn1 = Sn2) then MUX1 is the best;
-        * If (Qn1 > Qn2) and (Sn1 > Sn2) then MUX1 is the best;
-        * If (Qn1 > Qn2) and (Sn1 < Sn2) then:
-        ** If (Qn2 = Sn1) then MUX2 is the best; (MUX2 has the best Strength)
-        ** If (Qn2 > Sn1) then MUX2 is the best; (MUX2 is less at the limit than MUX1)
-        ** If (Qn2 < Sn1) then MUX1 is the best; (MUX1 is less at the limit than MUX2)
-
-        * If (Qn1 < Qn2) and (Sn1 = Sn2) then MUX2 is the best;
-        * If (Qn1 < Qn2) and (Sn1 > Sn2) then:
-        ** If (Qn1 = Sn2) then MUX1 is the best; (MUX1 has the best Strength)
-        ** If (Qn1 > Sn2) then MUX1 is the best; (MUX1 is less at the limit than MUX2)
-        ** If (Qn1 < Sn2) then MUX2 is the best; (MUX2 is less at the limit than MUX1)
-
-        ** If (Qn1 < Qn2) and (Sn1 < Sn2) then MUX2 is the best.
*/
static int __rf_SQ_cmp(LCNConflProgInfoOne_t* lcn1, LCNConflProgInfoOne_t *lcn2)
{
    /*p mux1 lcn mux2*/
    if(lcn1->frequency != lcn2->frequency)
    {
        if(lcn1->freq_quality == lcn2->freq_quality)
        {
            if(lcn1->freq_strength >= lcn2->freq_strength)
                return 0;
            else
                return 1;
        }

        if(lcn1->freq_quality > lcn2->freq_quality)
        {
            if(lcn1->freq_strength >= lcn2->freq_strength)
            {
                return 0;
            }
            else
            {
                if(lcn2->freq_quality >= lcn1->freq_strength)
                    return 1;
                else
                    return 0;
            }
        }

        if(lcn1->freq_quality < lcn2->freq_quality)
        {
            if(lcn1->freq_strength == lcn2->freq_strength)
            {
                return 0;
            }
            else if(lcn1->freq_strength > lcn2->freq_strength)
            {
                if(lcn1->freq_quality >= lcn2->freq_strength)
                    return 0;
                else
                    return 1;
            }
            else if(lcn1->freq_strength < lcn2->freq_strength)
            {
                return 1;
            }
        }
    }

    return 0;
}

static void _sort_conflict_list_by_SQ(struct gxlist_head *lcn_confl_list)
{
    LCNConflInfoOne_t *p = NULL, *p_next = NULL, *p_tmp = NULL;
    LCNConflProgInfoOne_t *c = NULL, *c_tmp = NULL;

    gxlist_for_each_entry_safe(p, p_next, lcn_confl_list, list)
    {
        gxlist_for_each_entry(c, &p->prog_list, list)
        {
            for(c_tmp = gxlist_entry(c->list.next, LCNConflProgInfoOne_t, list);
                    &c_tmp->list != &p->prog_list;
                    c_tmp = gxlist_entry(c_tmp->list.next, LCNConflProgInfoOne_t, list))
            {
                if(1 == __rf_SQ_cmp(c, c_tmp))
                    _swap_lcn_confl_prog_info(c, c_tmp);
            }
        }
        for(p_tmp = p_next;
                &p_tmp->list != lcn_confl_list;
                p_tmp = gxlist_entry(p_tmp->list.next, LCNConflInfoOne_t, list))
        {
            if(p->lcn > p_tmp->lcn)
            {
                gxlist_del(&p->list);
                gxlist_add(&p->list, &p_tmp->list);
            }
        }
    }
}
#elif LCN_CONFLICT_LIST_SORTED_BY_HD_CHANNEL
static int __detect_HD_str(char *src_str)
{
 #define MAX_BUF_LEN     MAX_PROG_NAME+1
    char *specify_str[] = {
     "HD"
	 };
    int  i = 0;
    char *p = NULL;

    #define SPECIFY_COUNT sizeof(specify_str)/sizeof(char*)
    if(MAX_BUF_LEN <= (strlen(src_str) + 1))
    {// do not deal with
        return 0;
    }
    for(i = 0; i < SPECIFY_COUNT; i++)
    {
        if(NULL != (p = strstr(src_str, specify_str[i])))
        {
	    // such as "HD CCTV", "CCTV HD", or "CCTV HD CHANNEL 1"
			if( ((*(p-1) >= 0x20) || (p == src_str))
					&& ((*(p+2) == 0x20) || (*(p+2) == '\0')))
            {
				return 1;
			}
		}
	}
    return 0;
 }

static int __rf_SQ_and_HD_cmp(LCNConflProgInfoOne_t* lcn1, LCNConflProgInfoOne_t *lcn2)
{
    char prog1_hd = 0,prog2_hd=0;
    LCNInfoOne_t lcn1_infor;
    LCNInfoOne_t lcn2_infor;
    memset(&lcn1_infor, 0, sizeof(LCNInfoOne_t));
    memset(&lcn2_infor, 0, sizeof(LCNInfoOne_t));
    app_lcn_list_info_get(lcn1->original_id, lcn1->ts_id, lcn1->service_id, lcn1->frequency, &lcn1_infor);
    app_lcn_list_info_get(lcn2->original_id, lcn2->ts_id, lcn2->service_id, lcn2->frequency, &lcn2_infor);
    prog1_hd = __detect_HD_str(lcn2_infor.prog_name);
    prog2_hd = __detect_HD_str(lcn2_infor.prog_name);

    if( (prog1_hd == 1) && (prog2_hd == 0) )
    {
        return 0;
    }
    else  if( (prog1_hd == 0) && (prog2_hd == 1) )
    {
        return 1;
    }
    /*p mux1 lcn mux2*/
    if(lcn1->frequency != lcn2->frequency)
    {
        if(lcn1->freq_quality == lcn2->freq_quality)
        {
            if(lcn1->freq_strength >= lcn2->freq_strength)
                return 0;
            else
                return 1;
        }

        if(lcn1->freq_quality > lcn2->freq_quality)
        {
            if(lcn1->freq_strength >= lcn2->freq_strength)
            {
                return 0;
            }
            else
            {
                if(lcn2->freq_quality >= lcn1->freq_strength)
                    return 1;
                else
                    return 0;
            }
        }

        if(lcn1->freq_quality < lcn2->freq_quality)
        {
            if(lcn1->freq_strength == lcn2->freq_strength)
            {
                return 0;
            }
            else if(lcn1->freq_strength > lcn2->freq_strength)
            {
                if(lcn1->freq_quality >= lcn2->freq_strength)
                    return 0;
                else
                    return 1;
            }
            else if(lcn1->freq_strength < lcn2->freq_strength)
            {
                return 1;
            }
        }
    }

    return 0;
}

static void _sort_conflict_list_by_HD_Channel(struct gxlist_head *lcn_confl_list)
{
    LCNConflInfoOne_t *p = NULL, *p_next = NULL, *p_tmp = NULL;
    LCNConflProgInfoOne_t *c = NULL, *c_tmp = NULL;

    gxlist_for_each_entry_safe(p, p_next, lcn_confl_list, list)
    {
        gxlist_for_each_entry(c, &p->prog_list, list)
        {
            for(c_tmp = gxlist_entry(c->list.next, LCNConflProgInfoOne_t, list);
                    &c_tmp->list != &p->prog_list;
                    c_tmp = gxlist_entry(c_tmp->list.next, LCNConflProgInfoOne_t, list))
            {
                if(1 == __rf_SQ_and_HD_cmp(c, c_tmp))
                    _swap_lcn_confl_prog_info(c, c_tmp);
            }
        }
        for(p_tmp = p_next;
                &p_tmp->list != lcn_confl_list;
                p_tmp = gxlist_entry(p_tmp->list.next, LCNConflInfoOne_t, list))
        {
            if(p->lcn > p_tmp->lcn)
            {
                gxlist_del(&p->list);
                gxlist_add(&p->list, &p_tmp->list);
            }
        }
    }
}

#endif

#if LCN_CONFLICT_HANDLE_BY_VIRTUAL_LCN
static int32_t _lcn_conflict_handle_by_virtual_lcn(struct gxlist_head *lcn_info_list, struct gxlist_head *lcn_confl_list)
{
    LCNConflProgInfoOne_t *c = NULL;
    LCNConflInfoOne_t *p = NULL;
    bool tv_flag = false;
    bool radio_flag = false;

    if(1 == gxlist_empty(lcn_info_list)
            || (NULL == lcn_info_list->prev && NULL == lcn_info_list->next)
            || (NULL == lcn_confl_list->prev && NULL == lcn_confl_list->next))
    {
        return -1;
    }

    _lcn_conflict_pre_handle(lcn_info_list, lcn_confl_list);

    gxlist_for_each_entry(p, lcn_confl_list, list)
    {
        tv_flag = false;
        radio_flag = false;
        gxlist_for_each_entry(c, &p->prog_list, list)
        {
            //保留第一个,其它改为虚拟lcn
            if(tv_flag == false || radio_flag == false)
            {
                if(tv_flag == false && GXBUS_PM_PROG_TV == c->service_type)
                {
                    tv_flag = true;
                    continue;
                }
                else if(radio_flag == false && GXBUS_PM_PROG_RADIO == c->service_type)
                {
                    radio_flag = true;
                    continue;
                }
            }
            _lcn_value_set_invalid(lcn_info_list, c->original_id, c->ts_id, c->service_id, c->frequency);
        }
    }

    _lcn_allocate_virtual_lcn_to_invalid_prog(lcn_info_list);

    return 0;
}
#elif LCN_CONFLICT_HANDLE_BY_THROW_AWAY
static void _lcn_value_set_del(struct gxlist_head *lcn_info_list, uint32_t original_id,
                                    uint32_t ts_id, uint32_t service_id, uint32_t frequency)
{
    LCNInfoOne_t *p = NULL;

    gxlist_for_each_entry(p, lcn_info_list, list)
    {
        if(p->original_id == original_id
                && p->ts_id == ts_id
                && p->service_id == service_id
                && p->frequency == frequency)
        {
            p->lcn = INVALID_PROG;
            break;
        }
    }
    return;
}

/*
 * The same program in the list keeps the first one, and the others are deleted; non-same programs are allocated virtual LCN
 * */
static int32_t _lcn_conflict_handle_by_throw_away(struct gxlist_head *lcn_info_list, struct gxlist_head *lcn_confl_list)
{
    int32_t i = 0;
    LCNConflInfoOne_t *p = NULL;
    LCNConflProgInfoOne_t *c = NULL;
    uint32_t original_id = 0, ts_id = 0, service_id = 0;

    if(1 == gxlist_empty(lcn_info_list)
            || (NULL == lcn_info_list->prev && NULL == lcn_info_list->next)
            || (NULL == lcn_confl_list->prev && NULL == lcn_confl_list->next))
    {
        return -1;
    }

    _lcn_conflict_pre_handle(lcn_info_list, lcn_confl_list);

    gxlist_for_each_entry(p, lcn_confl_list, list)
    {
        i = 0;
        gxlist_for_each_entry(c, &p->prog_list, list)
        {
            if(0 == i)
            {
                i++;
                original_id = c->original_id;
                ts_id = c->ts_id;
                service_id = c->service_id;
                continue;
            }

            if(c->original_id == original_id && c->ts_id == ts_id && c->service_id == service_id)
                _lcn_value_set_del(lcn_info_list, c->original_id, c->ts_id, c->service_id, c->frequency);
            else
                _lcn_value_set_invalid(lcn_info_list, c->original_id, c->ts_id, c->service_id, c->frequency);

            i++;
        }
    }

    _lcn_allocate_virtual_lcn_to_invalid_prog(lcn_info_list);

    return 0;
}
#elif LCN_CONFLICT_HANDLE_BY_USER
static event_list  *s_LcnPopListTimer = NULL;
extern int app_pop_list_end_dialog(void);

static int _poplist_timeout_cb(void *usrdata)
{
    return app_pop_list_end_dialog();
}

static int32_t _lcn_conflict_handle_by_user(struct gxlist_head *lcn_info_list, struct gxlist_head *lcn_confl_list)
{
#define MAX_CONFLICT_LCN_NUM (64)

    int32_t i = 0;
    LCNConflInfoOne_t *p = NULL;
    LCNConflProgInfoOne_t *c = NULL;
    char *buffer[MAX_CONFLICT_LCN_NUM] = {0};
    uint32_t count = 0;
    PopList pop_list;

    if(1 == gxlist_empty(lcn_info_list)
            || (NULL == lcn_info_list->prev && NULL == lcn_info_list->next)
            || (NULL == lcn_confl_list->prev && NULL == lcn_confl_list->next))
        goto err;

    _lcn_conflict_pre_handle(lcn_info_list, lcn_confl_list);

    for(i = 0; i < MAX_CONFLICT_LCN_NUM; i++)
    {
        buffer[i] = GxCore_Calloc(128, sizeof(char));
        if(NULL == buffer[i])
        {
            app_log_error("\033[31mno enough memory\033[0m\n");
            goto err;
        }
    }

    gxlist_for_each_entry(p, lcn_confl_list, list)
    {
        count = 0;
        int32_t ret = 0;

        gxlist_for_each_entry(c, &p->prog_list, list)
        {
            LCNInfoOne_t lcn;

            memset(&lcn, 0, sizeof(LCNInfoOne_t));
            if(0 == app_lcn_list_info_get(c->original_id, c->ts_id, c->service_id, c->frequency, &lcn))
            {
                snprintf(buffer[count], 128, "%d %s (%d.%d)MHz", lcn.lcn, lcn.prog_name, lcn.frequency/1000, (lcn.frequency/100)%10);
                count++;
            }

            if(count > MAX_CONFLICT_LCN_NUM)
                break;
        }

        if(count < 2)
            continue;

        memset(&pop_list, 0, sizeof(PopList));
        pop_list.title = "LCN Select";
        pop_list.pos.x = 282;
        pop_list.pos.y = 120;
        pop_list.item_num = count;
        pop_list.item_content = buffer;
        pop_list.sel = 0;
        pop_list.show_num = false;
        APP_TIMER_ADD(s_LcnPopListTimer, _poplist_timeout_cb, 10*1000, TIMER_REPEAT);

        ret = poplist_create(&pop_list);

        i = 0;

        gxlist_for_each_entry(c, &p->prog_list, list)
        {
            if(ret == i)
            {
                i++;
                continue;
            }
            _lcn_value_set_invalid(lcn_info_list, c->original_id, c->ts_id, c->service_id, c->frequency);
            i++;
        }
    }

    _lcn_allocate_virtual_lcn_to_invalid_prog(lcn_info_list);

    for(i = 0; i < MAX_CONFLICT_LCN_NUM; i++)
    {
        APP_FREE(buffer[i]);
    }
    return 0;
err:
    for(i = 0; i < MAX_CONFLICT_LCN_NUM; i++)
    {
        APP_FREE(buffer[i]);
    }
    return -1;
}
#endif

void app_lcn_conflict_handle_common_register(void)
{
#if LCN_CONFLICT_HANDLE_BY_VIRTUAL_LCN
    app_lcn_conflict_handle_register(_lcn_conflict_handle_by_virtual_lcn);
#elif LCN_CONFLICT_HANDLE_BY_THROW_AWAY
    app_lcn_conflict_handle_register(_lcn_conflict_handle_by_throw_away);
#elif LCN_CONFLICT_HANDLE_BY_USER
    app_lcn_conflict_handle_register(_lcn_conflict_handle_by_user);
#endif

#if LCN_CONFLICT_LIST_SORTED_BY_RFCH_SQ_ORDER
    app_lcn_conflict_sort_method_register(_sort_conflict_list_by_SQ);
#elif LCN_CONFLICT_LIST_SORTED_BY_FRE_ORDER
    app_lcn_conflict_sort_method_register(_sort_conflict_list_by_frequency);
#elif LCN_CONFLICT_LIST_SORTED_BY_HD_CHANNEL
    app_lcn_conflict_sort_method_register(_sort_conflict_list_by_HD_Channel);
#endif

#if LCN_VALID_CHECK_BY_ONID
    app_lcn_valid_check_register(_lcn_valid_onid_check);
#elif LCN_VALID_CHECK_BY_NID
    app_lcn_valid_check_register(_lcn_valid_nid_check);
#endif
}
#endif
#endif
