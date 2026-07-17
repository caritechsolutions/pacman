/** @defgroup pm_module PM Module*/
/*@{*/
#ifndef __SI_SUB_ENGINE_PRIVATE_H__
#define __SI_SUB_ENGINE_PRIVATE_H__

#include "sub_system/si_sub_engine/si_sub_engine.h"
__BEGIN_DECLS


/*危列峙*/

/* Exported Types --------------------------------------------------------- */
//#ifndef __DEBUG
//#define GX_SI_ENGINE_ERR_DBUG
//#endif

#define SI_ENGINE_PRINTF(...) gxlogd( __VA_ARGS__ )

#define SI_ENGINE_ERR_PRINTF(msg)\
    do{\
        SI_ENGINE_PRINTF("\n\n*****si engine error*****\n");\
        SI_ENGINE_PRINTF("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__ );\
        SI_ENGINE_PRINTF("%s\n",msg);\
        SI_ENGINE_PRINTF("~~~~~si engine error~~~~~\n");\
    }while(0)

#define SI_TODATA0_11BIT(data1,data2)              (((data1 & 0x0f) << 8 ) |data2)
#define SI_TODATA0_7BIT(data)                      (data)
#define SI_TODATA0_15BIT(data1,data2)              ((data1<<8 )|data2)

uint32_t SI_DMX_SUB_NUM;
uint32_t SI_DMX_SUB_FILTER_NUM;

typedef enum{
    STOP = 0,
    CONTINUE,
    PAUSE,
}SiEngineParseRet;


typedef struct
{
	dmx_handle_t dmx;
	filter_handle_t filter;
	uint32_t time_ms;
	uint32_t time_ms_out;
	SiEngineParseRet status;
	handle_t mutex;
	uint8_t sec[256];//耽倖燕恷謹256倖粁
	uint32_t sec_count;
	uint32_t max_sec_count;
	OriginalSection original;
	TableSection table;
	TimeOut time_out;
	SecNumCheck sec_num_check;
    void * ptr;
    //OBJ控制块为全局变量，为了避免多线程直接访问控制块，在GxSubSystem_SiEngineAlloc时记录OBJ控制块的地址，类似于指向本身的this指针
    void * me;
} GxSubsystemSiEngineObj;




typedef struct
{
	GxSubsystemSiEngineObj  obj;
	uint32_t size;
	uint8_t* buffer;
	void* pre;
	void* next;
} GxSubsystemSiEnginCache;

typedef struct
{
	void* head;
	void* taile;
	handle_t mutex;
} GxSubsystemSiEnginCacheHead;

typedef void*     (*ParseSectionFunc)(uint8_t * section,uint32_t size);
typedef void*     (*PrivateParseSectionFunc)(uint8_t * section,uint32_t size,uint32_t flag);
typedef int32_t      (*ReleaseSectionFunc)(void * table);
typedef int32_t      (*PrivateReleaseSectionFunc)(void * table,uint32_t flag);

typedef struct {
    int32_t tid;
    ParseSectionFunc  parsefunc;
    ReleaseSectionFunc  releasefunc;
    PrivateParseSectionFunc p_parsefunc;
    PrivateReleaseSectionFunc  p_releasefunc; 
}GxSubsystmSiEngineParseFuncTable;


typedef struct
{
	dmx_handle_t dmx;
	handle_t cond;
	handle_t mutex;
	uint32_t count;
    GxSubsystmSiEngineParseFuncTable * parse_arry;
	//filter_handle_t filter[DMX_SUB_FILTER_NUM];
	filter_handle_t * filter;
    uint32_t usr_init_flag;
    GxSearchFrontType front_type; 
} GxSubsystemSiEnginDmx;

int32_t si_sub_crc32_check(uint8_t *pSection);
void* si_sub_parse_section_standard(GxSubsystemSiEnginDmx * dmx,uint8_t* sec, uint32_t size);
void si_sub_release_section(GxSubsystemSiEnginDmx * dmx,void* table);
void callback_too_long_point(int32_t start_time,int32_t end_time);
GxSubsystmSiEngineParseFuncTable* si_sub_get_parser_ops(int flag);
/* Exported Functions ----------------------------------------------------- */
__END_DECLS

#endif
/*@}*/

