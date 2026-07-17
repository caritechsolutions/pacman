#ifndef __SI_PUBLIC_PARSER_H__
#define __SI_PUBLIC_PARSER_H__

#include "module/si/si_bat.h"
#include "module/si/si_pat.h"
#include "module/si/si_pmt.h"
#include "module/si/si_sdt.h"
#include "module/si/si_nit.h"
#include "module/si/si_eit.h"

__BEGIN_DECLS
//descriptor的公共解析函数
typedef int32_t (*des_parser)(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

typedef enum StandardParserState
{
    STANDARD_PARSER_SUCCESS = 0,
    STANDARD_PARSER_LEN_ERROR,
    STANDARD_PARSER_SYNTAX_ERROR,
}StandardParserState;

/*description parser register*/
typedef struct
{
	uint16_t table_id;
	uint16_t desc_tag;
	des_parser des_parser_cb;
}GxDescRegister;

StandardParserState GxBus_SiStandardParser(uint8_t *section_data,uint8_t *p_parsed_data,uint32_t len,uint32_t ProtectDisable);
status_t GxBus_SiParserDescReg(GxDescRegister *reg);
status_t GxBus_SiParserDescUnreg(GxDescRegister *unreg);
__END_DECLS

#endif
