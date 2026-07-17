#include "app_si_parse.h"
#include "app.h"

#if RICHEPG_SUPPORT
#define DES_SIZEOF(ptr) (sizeof(ptr)/sizeof(ptr[0]))

int32_t s_SiPrintLevel = SI_LOG_DBG;

//公共表的表头解析函数
typedef int32_t (*sec_head_cb)(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

//公共表的循环处的头信息
typedef int32_t (*sec_loop_head_cb)(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

struct des_info
{
    uint32_t   tag;
    des_parser des_parser_cb;
};

/**
 *  only normal parse use this index
 */
enum table_index
{
    TBL_INDEX_PAT,
    TBL_INDEX_PMT,
    TBL_INDEX_SDT,
    TBL_INDEX_NIT,
    TBL_INDEX_EIT,
    TBL_INDEX_BAT,
    TBL_INDEX_CAT,
    TBL_INDEX_TOT,
    TBL_INDEX_DES
};

typedef struct
{
    uint8_t           sec_hlen;       ///<section head data length .
    uint8_t           sec_lp_hlen;    ///<section loop head data length .
    uint16_t          des_cnt;
    struct des_info  *desc_info;
    sec_head_cb       sec_hcb;        ///<sector head callback .
    sec_loop_head_cb  sec_lp_hcb;     ///<loop head callback .
}SiParseCtrl;

typedef struct{
    uint32_t desc_num;
    GxDescRegister *desc_reg;
}DescRegCtrl;

/* Private Variables ------------------------------------------------------ */
//1table
extern int32_t nit_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t nit_lp_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

extern int32_t pat_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t pat_lp_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

extern int32_t pmt_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t pmt_lp_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

extern int32_t sdt_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t sdt_lp_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

extern int32_t eit_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t eit_lp_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

extern int32_t bat_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t bat_lp_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

//1descriptor
extern int32_t des_service_descrip(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

extern int32_t des_sat_del_sys(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t des_cab_del_sys(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t des_ter_del_sys(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t des_dtmb_del_sys(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

extern int32_t des_short_event(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t des_extended_event(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t des_parental_rating(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

extern int32_t des_bouqust_name(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

extern int32_t des_pmt_ca(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

extern int32_t des_pmt_ac3(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

extern int32_t des_pmt_eac3(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

extern int32_t des_pmt_subt(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

extern int32_t des_pmt_aribcc(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

extern int32_t des_pmt_ttx(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

extern int32_t des_pmt_iso639(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

extern int32_t des_pmt_registration(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

extern int32_t des_eit_tms_event(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

static struct des_info des_pmt[] = {
    {SI_CA_DESCRIPTOR,              des_pmt_ca},
    {SI_AC3_DESCRIPTOR,             des_pmt_ac3},
    {SI_EAC3_DESCRIPTOR,            des_pmt_eac3},
    {SI_SUBTITLING_DESCRIPTOR,      des_pmt_subt},
    {SI_ARIBCCING_DESCRIPTOR,       des_pmt_aribcc},
    {SI_TELETEXT_DESCRIPTOR,        des_pmt_ttx},
    {SI_ISO639_LANGUAGE_DESCRIPTOR, des_pmt_iso639},
    {SI_REGISTRATION_DESCRIPTOR,    des_pmt_registration},
};

static struct des_info des_nit[] = {
    {SI_SAT_DELIVERY_SYSTEM_DESCRIPTOR, des_sat_del_sys},
    {SI_CAB_DELIVERY_SYSTEM_DESCRIPTOR, des_cab_del_sys},
    // dvbt to do
};

static struct des_info des_sdt[] = {
    {SI_SERVICE_DESCRIPTOR, des_service_descrip},
};

#if 0
static struct des_info des_eit[] = {
    {SI_SHORT_EVENT_DESCRIPTOR,        des_short_event},
    {SI_EXTENDED_EVENT_DESCRIPTOR,     des_extended_event},
    {SI_PARENTAL_RATING_DESCRIPTOR,    des_parental_rating},
    {SI_TIME_SHIFTED_EVENT_DESCRIPTOR, des_eit_tms_event},
};
#endif

static struct des_info des_bat[] = {
    {SI_BOUQUET_NAME_DESCRIPTOR, des_bouqust_name},
};

static SiParseCtrl thiz_SiParseCtrl[] = {
    {8,  4, 0,                   NULL,    pat_head_parser, pat_lp_head_parser},//pat
    {12, 5, DES_SIZEOF(des_pmt), des_pmt, pmt_head_parser, pmt_lp_head_parser},//pmt
    {11, 5, DES_SIZEOF(des_sdt), des_sdt, sdt_head_parser, sdt_lp_head_parser},//sdt
    {10, 6, DES_SIZEOF(des_nit), des_nit, nit_head_parser, nit_lp_head_parser},//nit
    {10, 6, DES_SIZEOF(des_bat), des_bat, bat_head_parser, bat_lp_head_parser},//bat
};

static DescRegCtrl thiz_DescReg = {0, NULL};

static int16_t _si_table_index_get(uint8_t tid)
{
    int16_t table_index = -1;
    switch (tid)
    {
        case PAT_TID:
            table_index = TBL_INDEX_PAT;
            break;
        case CAT_TID:
            table_index = TBL_INDEX_CAT;
            break;
        case PMT_TID:
            table_index = TBL_INDEX_PMT;
            break;
        case NIT_ACTUAL_NETWORK_TID:
        case NIT_OTHER_NETWORK_TID:
            table_index = TBL_INDEX_NIT;
            break;
        case SDT_ACTUAL_TS_TID:
        case SDT_OTHER_TS_TID:
            table_index = TBL_INDEX_SDT;
            break;
        case BAT_TID:
            table_index = TBL_INDEX_BAT;
            break;
        case EIT_ACTUAL_PF_TID:
        case EIT_OTHER_PF_TID:
            table_index = TBL_INDEX_EIT;
            break;
        case TDT_TID:
            table_index = -1;
            break;
        case TOT_TID:
            table_index = TBL_INDEX_TOT;
            break;
        default:
            if(tid >= 0x50 && tid <= 0x6f)
                table_index = TBL_INDEX_EIT;
            else
                table_index = -1;

            break;
    }

    return table_index;
}

static int8_t _si_des_cb_idx_get(uint8_t des_tag, SiParseCtrl *parse_ctrl)
{
    uint8_t i;

    for(i = 0; i < parse_ctrl->des_cnt; i++)
    {
        if(parse_ctrl->desc_info[i].tag == des_tag)
            return i;
    }

    return -1;
}

static void _si_private_desc_parse(uint8_t tag, uint8_t *p_section_data,
                                    uint16_t len, uint8_t *p_parsed_data)
{
    uint32_t i;

    for (i = 0; i < thiz_DescReg.desc_num; i++)
    {
        // TODO: need judge table_id later
        if(thiz_DescReg.desc_reg[i].desc_tag == tag)
            (thiz_DescReg.desc_reg[i].des_parser_cb)(tag, p_section_data, len, p_parsed_data);
    }

    return;
}
static status_t _si_des_parse(uint8_t *p_section_data, int16_t len,
                            uint8_t *p_parsed_data, SiParseCtrl *parse_ctrl)
{
    uint8_t dtag;
    int8_t didx;
    int16_t des_len=0;

    while(len > 0)
    {
        dtag = p_section_data[0];
        des_len = p_section_data[1];

        len -= (des_len + 2);
        if(len < 0)
            return GXCORE_ERROR;

        // private description parse
        _si_private_desc_parse(dtag, p_section_data, des_len, p_parsed_data);

        didx = _si_des_cb_idx_get(dtag, parse_ctrl);
        if (didx != -1)
            parse_ctrl->desc_info[didx].des_parser_cb(dtag, p_section_data, des_len, p_parsed_data);

        p_section_data += (des_len + 2);
    }

    return GXCORE_SUCCESS;
}

void *app_si_parse(private_table_cfg *table_cfg, uint8_t *section_data, uint32_t len)
{
// include table_id (1) and section_len (2)
#define SECTION_ONLY_APPEND_LEN   (3)
    int16_t des1_len = 0;
    int16_t des2_len = 0;
    int16_t sec_len  = 0;
    int16_t loop_len = 0;
    int16_t table_index = -1;
    uint8_t *p_parsed_data = SEC_PRIVATE_PARSE;
    SiParseCtrl *parse_ctrl = NULL;

    if(NULL == section_data || 0 == len)
        return NULL;

    if(PARSE_STANDARD_ONLY == table_cfg->mode
            || PARSE_WITH_STANDARD == table_cfg->mode
            || PARSE_SECTION_ONLY == table_cfg->mode)
    {
        // private parse
        if(PARSE_WITH_STANDARD == table_cfg->mode && table_cfg->table_parse_fun != NULL)
            table_cfg->table_parse_fun(section_data, len);

        // don't support parse eit now
        p_parsed_data = GxCore_Calloc(SMALL_BUF_SIZE, sizeof(uint8_t));

        if(NULL == p_parsed_data)
        {
            SI_ERR("No enough memory");
            return SEC_NO_MEMORY;
        }

        // get section length
        sec_len = TODATA12(section_data[1], section_data[2]);

        // for parse section only
        if(PARSE_SECTION_ONLY == table_cfg->mode)
        {
            if(sec_len + SECTION_ONLY_APPEND_LEN > SMALL_BUF_SIZE)
            {
                SI_FREE(p_parsed_data);
                return SEC_LEN_ERROR;
            }

            memcpy(p_parsed_data, section_data, sec_len + SECTION_ONLY_APPEND_LEN);
            return p_parsed_data;
        }

        // public table parse function
        table_index = _si_table_index_get(section_data[0]);

        // special protect: 0 the table id not support yet, set error as SEC_SYNTAX_INDICATOR, let it timeout
        if(-1 == table_index)
            return SEC_SYNTAX_INDICATOR;

        parse_ctrl = &thiz_SiParseCtrl[table_index];

        // special protect: 1 the section is dummy, let it go
        if(sec_len - 4 + 3 < parse_ctrl->sec_hlen)//se len 包含末尾的4字节crc，不包含头三个字节，但是如果没有crc的表就 囧了
        {
            SI_FREE(p_parsed_data);
            return SEC_LEN_ERROR;
        }

        // special protect: 2 all table (pat,pmt,sdt,nit,eit,bat) here section_syntax_indicator value is 1
        if((section_data[1] >> 7) != 1)
        {
            SI_FREE(p_parsed_data);
            return SEC_SYNTAX_INDICATOR;
        }

        // return descriptor 1 length, deal sec head ---------------------------
        des1_len = parse_ctrl->sec_hcb(section_data, len, p_parsed_data);

        // offset to descriptor 1
        section_data += parse_ctrl->sec_hlen;

        // start deal with descriptor 1
        if(des1_len > 0)
            _si_des_parse(section_data, des1_len, p_parsed_data, parse_ctrl);

        if(NULL == parse_ctrl->sec_lp_hcb)
            return p_parsed_data;

        // offset to lp length, and pmt have no lp length info
        section_data += des1_len;

        if(TBL_INDEX_PMT == table_index)
        {
            loop_len = sec_len-des1_len-parse_ctrl->sec_hlen-4+3;
        }
        else if(des1_len > 0
                || TBL_INDEX_NIT == table_index
                || TBL_INDEX_BAT == table_index)
        {
            // read the loop length (12)
            loop_len =TODATA12(section_data[0],section_data[1]) ;

            // offset to loop
            section_data += 2;
        }
        else
        {
            loop_len = sec_len - parse_ctrl->sec_hlen - 4 + 3;//4 //== crc .
        }

        // start deal with loop ----------------------------------------------
        while(loop_len > 0)
        {
            // return descriptor 2 length
            des2_len = parse_ctrl->sec_lp_hcb(section_data, len, p_parsed_data);

            // start deal with descriptor 2
            if(des2_len >= 0)
            {
                // offset to descriptor 2
                section_data += parse_ctrl->sec_lp_hlen;

                _si_des_parse(section_data, des2_len, p_parsed_data, parse_ctrl);
                section_data += des2_len;
            }
            else
            {
                // special protect: 3  parse buffer over flow
                SI_FREE(p_parsed_data);
                return SEC_LEN_ERROR;
            }

            // calc loop rest len
            loop_len -= (parse_ctrl->sec_lp_hlen + des2_len);
        }
    }
    else if(table_cfg->mode == PARSE_PRIVATE_ONLY)
    {
        int32_t private_status;

        // private parse
        if(table_cfg->table_parse_fun != NULL)
        {
            private_status = table_cfg->table_parse_fun(section_data, len);
            if(PRIVATE_SECTION_OK == private_status)
                p_parsed_data = SEC_SECTION_OK;
            else
                p_parsed_data = SEC_SUBTABLE_OK;

            // above p_parsed_data use to return private parse result
        }
    }

    return p_parsed_data;
}
#endif
