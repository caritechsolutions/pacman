#include <string.h>
#include "module/si/si_public.h"
#include "module/si/si_public_parser.h"
#define IN

typedef int32_t (*sec_head_cb)(uint8_t *p_section_data, uint16_t len, IN uint8_t *p_parsed_data);

typedef int32_t (*sec_loop_head_cb)(uint8_t *p_section_data, uint16_t len, IN uint8_t *p_parsed_data);

struct des_info
{
	uint32_t	tag;
	des_parser des_parser_cb;
};

struct section_ctrl
{
	uint8_t			sec_hlen;				///<section head data length .
	uint8_t			sec_lp_hlen;			///<section loop head data length .
	uint16_t			des_cnt;
	struct des_info*	desc_info;
	sec_head_cb		sec_hcb;			///<sector head callback .
	sec_loop_head_cb	sec_lp_hcb;		///<loop head callback .
};

typedef struct{
    uint32_t desc_num;
    GxDescRegister *desc_reg;
}DescRegCtrl;

struct des_info des_pmt[] = {
	{SI_CA_DESCRIPTOR, des_pmt_ca},
	{SI_AC3_DESCRIPTOR, des_pmt_ac3},
	{SI_EAC3_DESCRIPTOR, des_pmt_eac3},
	{SI_SUBTITLING_DESCRIPTOR, des_pmt_subt},
	{SI_ARIBCCING_DESCRIPTOR, des_pmt_aribcc},
	{SI_TELETEXT_DESCRIPTOR, des_pmt_ttx},
	{SI_ISO639_LANGUAGE_DESCRIPTOR, des_pmt_iso639},
	{SI_REGISTRATION_DESCRIPTOR, des_pmt_registration},
};

struct des_info des_nit[] = {
	{SI_SAT_DELIVERY_SYSTEM_DESCRIPTOR, des_sat_del_sys},
	{SI_CAB_DELIVERY_SYSTEM_DESCRIPTOR, des_cab_del_sys},
	{SI_TER_DELIVERY_SYSTEM_DESCRIPTOR,des_ter_del_sys},
};
/*
    // DTMB descriptor is the same with DVB terrestrial
    {SI_TER_DELIVERY_SYSTEM_DESCRIPTOR,des_dtmb_del_sys},
*/

struct des_info des_sdt[] = {
	{SI_SERVICE_DESCRIPTOR, des_service_descrip},
    {SI_COMPONENT_DESCRIPTOR, des_component_descriptor},
//	{SI_NVOD_REFERENCE_DESCRIPTOR,des_nvod_ref},
//	{SI_TIME_SHIFTED_SERVICE_DESCRIPTOR,des_nvod_time_shift},
//	{SI_BOUQUET_NAME_DESCRIPTOR,des_sdt_bq_name},
//	{SI_MOSAIC_DESCRIPTOR,des_sdt_mosaic},
};

struct des_info des_eit[] = {
	{SI_SHORT_EVENT_DESCRIPTOR, des_short_event},
	{SI_EXTENDED_EVENT_DESCRIPTOR, des_extended_event},
    {SI_PARENTAL_RATING_DESCRIPTOR, des_parental_rating},
    {SI_TIME_SHIFTED_EVENT_DESCRIPTOR, des_eit_tms_event},
};

struct des_info des_bat[] = {
	{SI_BOUQUET_NAME_DESCRIPTOR, des_bouqust_name},
};

enum table_index
{
	TBL_INDEX_PAT,
	TBL_INDEX_PMT,
	TBL_INDEX_SDT,
	TBL_INDEX_NIT,
	TBL_INDEX_EIT,
	TBL_INDEX_BAT,
};//和section_ctr数组相关，作为section_ctr下标,和函数si_parser_get_table_index相关

#define DES_SIZEOF(ptr) (sizeof(ptr)/sizeof(ptr[0]))
struct section_ctrl section_ctr[] =
{
    { 8,4,0,NULL,pat_head_parser,pat_lp_head_parser},//pat
    {12,5,DES_SIZEOF(des_pmt),des_pmt,pmt_head_parser,pmt_lp_head_parser},//pmt
    {11,5,DES_SIZEOF(des_sdt),des_sdt,sdt_head_parser,sdt_lp_head_parser},//sdt
    {10,6,DES_SIZEOF(des_nit),des_nit,nit_head_parser,nit_lp_head_parser},//nit
    {14,12,DES_SIZEOF(des_eit),des_eit,eit_head_parser,eit_lp_head_parser},//eit
    {10,6,DES_SIZEOF(des_bat),des_bat,bat_head_parser,bat_lp_head_parser},//bat

};//下标为 enum table_index，顺序交换要与相关表同步。

static DescRegCtrl g_DescReg = {0, NULL};

static int16_t si_parser_get_table_index(uint8_t tid)
{
	int16_t table_index;
	switch (tid)
	{
		case PAT_TID:
			table_index = TBL_INDEX_PAT;
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
		default:
			if (tid>=0x50 && tid<=0x6f){
				table_index = TBL_INDEX_EIT;
			}
			else{
				table_index = -1;
			}
			break;
	}

	return table_index;
}


static int8_t si_des_cb_idx_get(uint8_t des_tag, struct section_ctrl *p_sec_ctr)
{
	uint8_t i;

	for (i=0; i<p_sec_ctr->des_cnt; i++)
	{
		if (p_sec_ctr->desc_info[i].tag == des_tag)
		{
			return i;
		}
	}

	return -1;
}

static void si_parser_private_desc(uint8_t tag, uint8_t *p_section_data,
                                    uint16_t len, uint8_t *p_parsed_data)
{
    uint32_t i;

    for (i=0; i<g_DescReg.desc_num; i++)
    {
        // TODO: need judge table_id later
        if (g_DescReg.desc_reg[i].desc_tag == tag)
        {
            (g_DescReg.desc_reg[i].des_parser_cb)(tag, p_section_data,
                                                        len, p_parsed_data);
        }
    }
}

static status_t si_parser_public_des(uint8_t *p_section_data, int16_t len,
							uint8_t *p_parsed_data, struct section_ctrl *p_sec_ctr)
{
	uint8_t dtag;
	int8_t didx;
	int16_t des_len=0;

	while(len>0)
	{
		dtag = p_section_data[0];
		des_len = p_section_data[1];

		len -= (des_len+2);
		if(len < 0)
		{
			return GXCORE_ERROR;
		}

        // private description parser
		si_parser_private_desc(dtag, p_section_data,des_len ,p_parsed_data);

		didx = si_des_cb_idx_get(dtag,p_sec_ctr);
		if (didx!=-1)
		{
            p_sec_ctr->desc_info[didx].des_parser_cb(dtag, p_section_data,des_len ,p_parsed_data);
		}
		p_section_data += (des_len+2);
	}

	return GXCORE_SUCCESS;
}

StandardParserState GxBus_SiStandardParser(uint8_t *section_data,uint8_t *p_parsed_data,uint32_t len,uint32_t ProtectDisable)
{
    int16_t des1_len;
    int16_t des2_len;
    int16_t sec_len;
    int16_t loop_len;
    int16_t table_index;
    struct section_ctrl* p_sec_ctr;
    table_index = si_parser_get_table_index(section_data[0]);
    if (table_index == -1)
    {
        gxlogd("no find table parse!!!\n");
        return STANDARD_PARSER_SYNTAX_ERROR;
    }
    p_sec_ctr = &section_ctr[table_index];
    sec_len = TODATA12(section_data[1], section_data[2]);
    if(sec_len-4+3 < p_sec_ctr->sec_hlen)//se len 包含末尾的4字节crc，不包含头三个字节，但是如果没有crc的表将返回错误
    {
        return STANDARD_PARSER_LEN_ERROR;
    }
    if ((ProtectDisable == 0) && (section_data[1] >> 7 != 1))
    {
        return STANDARD_PARSER_SYNTAX_ERROR;
    }
    des1_len = p_sec_ctr->sec_hcb(section_data, len, p_parsed_data);

    section_data += p_sec_ctr->sec_hlen;

    if (des1_len>0)
    {
        si_parser_public_des(section_data, des1_len, p_parsed_data, p_sec_ctr);
    }

    if (p_sec_ctr->sec_lp_hcb == NULL)
    {
        return STANDARD_PARSER_SUCCESS;
    }

    section_data += des1_len;

    if(table_index == TBL_INDEX_PMT)
    {
        loop_len = sec_len-des1_len-p_sec_ctr->sec_hlen-4+3;
    }
    else if(des1_len > 0
            || table_index == TBL_INDEX_NIT
            || table_index == TBL_INDEX_BAT)
    {
        loop_len =TODATA12(section_data[0],section_data[1]) ;

        section_data += 2;
    }
    else
    {
        loop_len = sec_len-p_sec_ctr->sec_hlen-4+3;//4 //== crc .
    }

    while (loop_len>0)
    {
        des2_len = p_sec_ctr->sec_lp_hcb(section_data, len, p_parsed_data);

        if (des2_len>=0)
        {
            section_data += p_sec_ctr->sec_lp_hlen;

            si_parser_public_des(section_data, des2_len, p_parsed_data, p_sec_ctr);
            section_data += des2_len;
        }
        else
        {
            break;
        }
        loop_len -= (p_sec_ctr->sec_lp_hlen+des2_len);
    }
	return STANDARD_PARSER_SUCCESS;
}


status_t GxBus_SiParserDescReg(GxDescRegister *reg)
{
#define REALLOC_NUM	(1)

    if (g_DescReg.desc_reg == NULL)
    {
        g_DescReg.desc_reg = GxCore_Malloc(sizeof(GxDescRegister));
    }
    else
    {
        // TODO: if already exist, need instead of it
        g_DescReg.desc_reg = GxCore_Realloc(g_DescReg.desc_reg,
                sizeof(GxDescRegister)*(g_DescReg.desc_num+REALLOC_NUM));
    }

    if (g_DescReg.desc_reg != NULL)
    {
        memcpy(g_DescReg.desc_reg+g_DescReg.desc_num,
                                            reg, sizeof(GxDescRegister));

        g_DescReg.desc_num++;

        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}

status_t GxBus_SiParserDescUnreg(GxDescRegister *unreg)
{
    GxDescRegister *p = NULL;
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t desc_num = g_DescReg.desc_num;

    p = GxCore_Malloc(g_DescReg.desc_num*sizeof(GxDescRegister));
    if (p == NULL)
    {
        return GXCORE_ERROR;
    }

    memset(p, 0, g_DescReg.desc_num*sizeof(GxDescRegister));

    // delete un-register descriptor parser
    for(i=0; i<desc_num; i++)
    {
        if (g_DescReg.desc_reg[i].table_id == unreg->table_id
                && g_DescReg.desc_reg[i].desc_tag == unreg->desc_tag)
        {
            g_DescReg.desc_num--;
            continue;
        }
        memcpy(p+j, &(g_DescReg.desc_reg[i]), sizeof(GxDescRegister));
        j++;
    }

    // update desc_reg
    GxCore_Free(g_DescReg.desc_reg);
    g_DescReg.desc_reg = NULL;

    if (g_DescReg.desc_num != 0)
    {
        g_DescReg.desc_reg = GxCore_Malloc(g_DescReg.desc_num*sizeof(GxDescRegister));
        if (g_DescReg.desc_reg == NULL)
        {
            return GXCORE_ERROR;
        }

        memcpy(g_DescReg.desc_reg, p, g_DescReg.desc_num*sizeof(GxDescRegister));
    }

    GxCore_Free(p);
    return GXCORE_SUCCESS;
}

