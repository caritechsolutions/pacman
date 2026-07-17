/** @defgroup pm_module PM Module*/
/*@{*/
#ifndef __SI_PARSE_LIB_PRIVATE_H__
#define __SI_PARSE_LIB_PRIVATE_H__

#include "sub_system/si_parse_lib/si_parse_lib.h"
__BEGIN_DECLS


//#ifndef __DEBUG
//#define GX_SI_PARSE_LIB_ERR_DBUG
//#endif

#define ATSC_DEFAULT_VALUE_0 (0)
#define ATSC_DEFAULT_VALUE_1 (1)


#define SI_PARSE_LIB_PRINTF(...) gxlogd( __VA_ARGS__ )

#define SI_PARSE_LIB_ERR_PRINTF(msg)\
    do{\
        SI_PARSE_LIB_PRINTF("\n\n*****si parse lib error*****\n");\
        SI_PARSE_LIB_PRINTF("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__ );\
        SI_PARSE_LIB_PRINTF("%s\n",msg);\
        SI_PARSE_LIB_PRINTF("~~~~~si parse lib error~~~~~\n");\
    }while(0)

/*#define GET_TABLE_ID(section) (section[0])
#define GET_SECTION_SYNTAX_INDICATOR_ID(section) ((section[1]>>7)&0x1)
#define GET_SECTION_LENGTH(section) ((section[1]<<4)|section[2])
#define GET_TRANSPORT_STREAM_ID(section) ((section[3]<<8)|section[4])
#define GET_VERSION_NUMBER(section) ((section[5]>>1)&0x1f)
#define GET_CURRENT_NEXT_INDICATOR(section) (section[5]&0x1)
#define GET_SECTION_NUMBER(section) (section[6])
#define GET_LAST_SECTION_NUMBER(section) (section[7])*/




#define TODATA0BIT(data)                        (data & 0x01) 
#define TODATA1BIT(data)                        ((data >> 1) & 0x01) 
#define TODATA4BIT(data)                        ((data >> 4) & 0x01) 
#define TODATA5BIT(data)                        ((data >> 5) & 0x01) 
#define TODATA6BIT(data)                        ((data >> 6) & 0x01) 
#define TODATA7BIT(data)                        ((data >> 7) & 0x01) 
#define TODATA1_5BIT(data)                      ((data >> 1) & 0x1f) 
#define TODATA5_7BIT(data)                      ((data >> 5) & 0x07) 
#define TODATA2_7BIT(data)                      ((data >> 2) & 0x3f) 
#define TODATA5_6BIT(data)                      ((data >> 5) & 0x03) 
#define TODATA3_7BIT(data)                      ((data >> 3) & 0x1f) 
#define TODATA4_7BIT(data)                      ((data >> 4) & 0x0f) 
#define TODATA6_7BIT(data)                      ((data >> 6) & 0x03) 
#define TODATA3_5BIT(data)                      ((data >> 3) & 0x07) 
#define TODATA3_4BIT(data)                      ((data >> 3) & 0x03) 
#define TODATA1_2BIT(data)                      ((data >> 1) & 0x03) 
#define TODATA1_3BIT(data)						((data >> 1) & 0x07)
#define TODATA0_2BIT(data)                      (data & 0x07) 
#define TODATA0_3BIT(data)                      (data & 0x0f) 
#define TODATA0_4BIT(data)                      (data & 0x1f) 
#define TODATA0_7BIT(data)                      (data)
#define TODATA0_11BIT(data1,data2)              (((data1 & 0x0f) << 8 ) |data2)
#define TODATA0_12BIT(data1,data2)              (((data1 & 0x1f)<< 8 ) |data2)
#define TODATA0_15BIT(data1,data2)              ((data1<<8 )|data2)
#define TODATA4_15BIT(data1,data2)              (((data1<<8 )|data2)>>4)
#define TODATA0_31BIT(data1,data2,data3,data4)  ((data1<<24)|(data2<<16)|(data3<<8)|data4)
//#define TODATA4_31BIT(data1,data2,data3,data4)  ((((data1<<24)|(data2<<16)|(data3<<8)|data4)>>4) & 0x0fffffff)
#define TODATA4_31BIT(data1,data2,data3,data4)  (data1<<20 )|(data2<<12 )|(data3<< 4 )|((data4>>4 )& 0x0f);



#define TODATA0_13BIT(data1,data2)              (((data1 & 0x3f)<< 8)|data2)
#define TODATA4_5BIT(data1)                     ((data1 >> 4)&0x03)
#define TODATA0_19BIT(data1,data2,data3)        (((data1&0x0f)<<16)|(data2<<8)|(data3))
#define TODATA0_23BIT(data1,data2,data3)        ((data1<<16)|(data2<<8)|data3)
#define TODATA0_5BIT(data)                      (data & 0x3f)
#define TODATA2_11BIT(data1,data2)              (((data1 & 0x0f)<<6)|(data2>>2))
#define TODATA0_9BIT(data1,data2)               ((data1 & 0x03)<<8|data2)
#define TODATA3BIT(data)                        ((data >> 3)&0x01)
#define TODATA2BIT(data)                        ((data >> 2)&0x01)
#define TODATA2_4BIT(data)						(data & 0x0e)
#define TODATA0_1BIT(data)						(data & 0x03)
#define TODATA4_6BIT(data)						((data >> 4) & 0x07)
#define TODATA0_5BIT(data)						(data & 0x3f)


typedef struct
{
    void* p;
    void* next;
}si_parse_lib_protect;

void si_parse_lib_add_protect(si_parse_lib_protect* head, void* p);
uint32_t si_parse_lib_remove_protect(si_parse_lib_protect* head,void* p);
uint8_t * si_parse_lib_poffset(uint8_t * p,uint8_t * section_end,uint32_t offset);
int32_t si_parse_lib_sizeoffset(uint32_t size,uint32_t offset);
int32_t si_parse_lib_parse_multiple_string(GxSubsystemSiParseMSSstruct * mul, uint8_t * section , uint8_t * section_end ,int32_t size);
void si_parse_lib_release_multiple_string(GxSubsystemSiParseMSSstruct * mul);

/* Exported Functions ----------------------------------------------------- */
__END_DECLS

#endif
/*@}*/

