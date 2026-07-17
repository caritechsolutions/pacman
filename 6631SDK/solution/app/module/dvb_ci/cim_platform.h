#ifndef __CIM_PLATFORM_H_INCLUDED__
#define __CIM_PLATFORM_H_INCLUDED__

#include <gxtype.h>
#include "cim_includes.h"
// 配置宏
#define _CIM_POLL_PERIOD_ (100) // 轮询周期 (毫秒)
#define _CIM_MODULE_CNT_  (1)  // 条件接收模块数

// 可移植数据类型
typedef unsigned int   cim_u32; // 32 位无符号整型
typedef unsigned short cim_u16; // 16 位无符号整型

// 时间结构 (UTC 格式)
struct cim_time
{
	cim_u32 m_nTimeL; // 低 32 位数据
	cim_u32 m_nTimeH; // 高 32 位数据
};

// 条件接收模块接口定义
struct cim_icam
{
	// 初始化设备
	cim_void (*InitDevice)(cim_void);

	// 检测卡设备
	// 检测到卡座内有卡时返回 TRUE，否则返回 FALSE
	cim_bool (*DetectCard)(cim_void);

	// 复位卡
	cim_void (*ResetCard)(cim_void);

	// 使能传输流接口
	// bEnable TRUE 使能 TSI 接口，FALSE 禁用 TSI 接口
	cim_void (*EnableTSI)(cim_bool bEnable);

	// 读 Memory 空间
	// nStart 待读取 Memory 空间的起始偏移
	// pBuff  接收数据的缓存区首地址
	// nLen   待读取的长度
	cim_void (*ReadMem)(cim_uint nStart, cim_u8 *pBuff, cim_uint nLen);

	// 写 Memory 空间
	// nStart 待写入 Memory 空间的起始偏移
	// pBuff  待写数据的缓存区首地址
	// nLen   待写入的长度
	cim_void (*WriteMem)(cim_uint nStart, cim_u8 *pBuff, cim_uint nLen);

	// 读 IO 空间
	// nAddr 待读取的 IO 空间中的寄存器偏移，如 命令寄存器为 0x1
	// pBuff 接收数据的缓存区首地址
	// nLen  待读取的长度
	cim_void (*ReadIO)(cim_uint nAddr, cim_u8 *pBuff, cim_uint nLen);

	// 写 IO 空间
	// nAddr 待写入的 IO 空间中的寄存器偏移，如 命令寄存器为 0x1
	// pBuff 待写数据的缓存区首地址
	// nLen  待写入的长度
	cim_void (*WriteIO)(cim_uint nAddr, cim_u8 *pBuff, cim_uint nLen);


};

extern struct cim_icam cim_icams[];

extern cim_void cim_semaphore_create(cim_void);
extern cim_void cim_semaphore_post(cim_void);
extern cim_bool cim_semaphore_wait(cim_uint nTime);

extern cim_void cim_get_time(struct cim_time *pTime);

//extern cim_void cim_printf(const cim_char *pFormat, ...);

#define cim_printf(...) {app_log_info("[cim_module]");\
					app_log_info(__VA_ARGS__);\
					app_log_info("\n");}

#define cim_test_printf(...) {app_log_debug(__VA_ARGS__);}

#endif // #ifndef __CIM_PLATFORM_H_INCLUDED__
///////////////////////////////////////////////////////////////////////////////
// end of cim_platform.h

