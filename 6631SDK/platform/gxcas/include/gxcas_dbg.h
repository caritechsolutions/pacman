#ifndef __GXCAS_DEBUG_H__
#define __GXCAS_DEBUG_H__

#include "gxtype.h"
#include "gxcas.h"

enum {
	CAS    = GXCAS_GETNUM(GXCAS_DEBUG_MOUDULE_CAS),    /* ca系统的debug信息*/
	CASLIB = GXCAS_GETNUM(GXCAS_DEBUG_MOUDULE_CASLIB), /* CA要求格式的debug信息*/
	DEMUX  = GXCAS_GETNUM(GXCAS_DEBUG_MOUDULE_DEMUX),  /* demux的debug信息    */
	DESC   = GXCAS_GETNUM(GXCAS_DEBUG_MOUDULE_DESC),   /* 解扰模块的debug信息 */
	SMC    = GXCAS_GETNUM(GXCAS_DEBUG_MOUDULE_SMC),    /* 智能卡的debug信息   */
	FLASH  = GXCAS_GETNUM(GXCAS_DEBUG_MOUDULE_FLASH),  /* flash的debug信息    */
	SI     = GXCAS_GETNUM(GXCAS_DEBUG_MOUDULE_SI),     /* SI的debug信息       */
	EADLIB = GXCAS_GETNUM(GXCAS_DEBUG_MOUDULE_EADLIB), /* EAD库的打印*/
};

extern unsigned int module_debug;

#define CAS_DBG(module, fmt, ...) do {                                                         \
	if(module_debug & module) {                                                            \
		int32_t index = 1, temp = module;                                              \
		for(;temp != 1 && ((temp >> 1) != 0); temp >>= 1)                              \
			index++;                                                               \
		if (module == CASLIB)                                                          \
			printf(fmt,##__VA_ARGS__);                                             \
		else {                                                                         \
			printf("\033[%dm", 40 + index);                                        \
			printf("[%s]%s():%d: "fmt, #module, __func__, __LINE__, ##__VA_ARGS__);\
			printf("\033[0m\n");                                                   \
		}                                                                              \
	}                                                                                      \
} while(0)

#define CAS_DUMP(module, ptr, size, fmt, ...)                                 \
	do {                                                                  \
		if(module_debug & module) {                                   \
			int i = 1, temp = module;                             \
			for(;(temp != 1) && (temp >> 0) != 1;temp >>=1)       \
				i++;                                          \
			if (module != CASLIB)                                 \
				printf("\033[%dm "fmt, 40 + i, ##__VA_ARGS__);\
			else                                                  \
				printf(fmt, ##__VA_ARGS__);                   \
			if (size != 0) {                                      \
				for (i = 0; i < (size); i++) {                \
					printf("0x%02x,", (ptr)[i]);          \
				}                                             \
			}                                                     \
			if (module != CASLIB)                                 \
				printf("\033[0m\n");                          \
		}                                                             \
	} while (0)

#define CAS_ERR(module, fmt, ...) do {                                         \
	printf("\033[31m");                                                    \
	printf("[%s]%s():%d:"fmt,#module, __func__, __LINE__, ##__VA_ARGS__);\
	printf("\033[0m\n");                                                   \
} while(0)

#endif

