#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ts_file.h>


/*
**************************************************************************************************
Function：判断TS文件锁定情况
Parameter：NULL
return： 0 － 未锁定TS
		188，204 － TS包长度
**************************************************************************************************
*/
uint32_t CheckTSLock(void);

/*
**************************************************************************************************
Function：获取一个Pid的section
Parameter：NULL
return： !NULL value -已得到所要PID的一个section   的地址
		NULL-找遍TS流也没得到所要PID的一个section   
**************************************************************************************************
*/
uint8_t* GetSectionData(void);



/*
**************************************************************************************************
Function：设置PID的值
Parameter：所要过滤的PID
return： 
**************************************************************************************************
*/
uint32_t SetPidValue(uint32_t Pid);

/*
**************************************************************************************************
Function：配置匹配条件
Parameter：
return： 
**************************************************************************************************
*/
uint32_t ConfigFilter(const uint8_t *pFilterValue, const uint8_t *pMaskValue, uint32_t FilterLen);

uint32_t SetMByte(uint8_t ByteNo, uint8_t FilterValue, uint8_t MaskValue);


void SetAnalysePos(uint32_t pos);

void ClrAnalysePos(void);

void SetTsFilePath(const char *const  path);


