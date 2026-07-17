#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ts_file.h"

#define TS_SYNC_BYTE 0x47
#define TS_LCOK_NUM (10)
#define TS_HEAD_LEN	(4)
#define SIZE_1K		(1024)
#define FILTER_SIZE	(4096)

//data filter start address
static uint8_t p_ucDfAddr[4096];

//section data start address no use?
//uint8_t *p_ucSectionAddr;
// 下一次从文件的这个点继续往后找相应的section
static uint32_t s_AnalysePos = 0;

//ts head info
typedef struct TSHeadInfo_s
{
	uint32_t sync_byte : 8;
	uint32_t transport_error_indicator : 1;
	uint32_t payload_unit_start_indicator : 1;
	uint32_t transport_priority : 1;
	uint32_t PID : 13;
	uint32_t transport_scrambling_control : 2;
	uint32_t adaptation_field_control : 2;
	uint32_t continuity_counter : 4;
}TSHeadInfo_t;

static TSHeadInfo_t tTSHeadInfo;

//biggest ts package size
static uint8_t ucPayloadUnit[205];


//if ts lock,get the first ts head position
static uint32_t uiFirstTSHeadPos;

//匹配条件
static uint32_t uiPidValue;
static uint8_t ucFilterValue[7];
static uint8_t ucMaskValue[7];
//过滤深度
static uint8_t ucFilterLen;

static uint8_t ucTSPackageLen;
//组成section时，TS包末尾要减去几个字节，188的包不减，204的包减去（204-188）
static uint8_t ucMakeSectionDec;

// private function ------------------------------------------------------------------
/*
**************************************************************************************************
Function：判断每隔TSPackageLen是否都能找到同步头
Parameter：pTSData - 同步头的基地址   TSPackageLen － 188or204
return： 0 － 找不到
		 1 - 确认锁定
**************************************************************************************************
*/
uint32_t check_ts(uint8_t *pTSData, uint32_t TSPackageLen)
{
	int i;
	
	for (i=0; i<TS_LCOK_NUM; i++)
	{
		if (pTSData[i*TSPackageLen] == TS_SYNC_BYTE)
		{
			continue;
		} 
		else
		{
			return 0;
		}
	}

	return 1;
}

/*
**************************************************************************************************
Function：判断TS文件锁定情况
Parameter：NULL
return： TRUE－分析正确
		FLASE－TS包总大小大于TS文件大小
**************************************************************************************************
*/
uint32_t analyse_one_ts_package(uint32_t PackageNo)
{
	
	memset(ucPayloadUnit, 0, sizeof(ucPayloadUnit));

	//cTSRead.Open(strTSPath,CFile::modeRead);
	TsFileCtrl.seek(&g_TsFile, (uiFirstTSHeadPos+PackageNo*ucTSPackageLen+s_AnalysePos));
	TsFileCtrl.read(&g_TsFile, ucPayloadUnit,ucTSPackageLen);

	//如果取的TS包总大小大于TS文件大小，则返回.暂时关闭
	if (ucTSPackageLen*PackageNo+s_AnalysePos > g_TsFile.end_pos)
	{
		gxlogd("ts file end!\n");
		while(1);
		return 0;
	}
	
	//cTSRead.Close();

	tTSHeadInfo.sync_byte = *ucPayloadUnit;
	tTSHeadInfo.transport_error_indicator = (*(ucPayloadUnit+1))>>7;
	tTSHeadInfo.payload_unit_start_indicator = ((*(ucPayloadUnit+1))>>6)&1;
	tTSHeadInfo.transport_priority = ((*(ucPayloadUnit+1))>>5)&1;
	tTSHeadInfo.PID = ((*(ucPayloadUnit+1)&0x1F)<<8)|(*(ucPayloadUnit+2));
	tTSHeadInfo.transport_scrambling_control = (*(ucPayloadUnit+3))>>6;
	tTSHeadInfo.adaptation_field_control = ((*(ucPayloadUnit+3))>>4)&3;
	tTSHeadInfo.continuity_counter = (*(ucPayloadUnit+3))&0xF;

	return 1;
}

/*
**************************************************************************************************
Function：当前section是否已结束
Parameter：AdapCtrl－adaptation_field_control
			DfPos－数据写到Df通道缓冲的位置
return： TRUE－get effective payload      FALSE－have no effective payload
Question:在tTSHeadInfo.payload_unit_start_indicator为1的时候会有调整字段吗？暂当没有！
		uiSectionOffset只有在payload_unit_start_indicator为1时才有值
**************************************************************************************************
*/
uint32_t get_effective_payload(uint32_t AdapCtrl, uint32_t uiSectionOffset, uint32_t *pDfPos)
{
// 可能是point_field，也可能是adaptation_field_length
#define GET_ADAP_LEN (*(ucPayloadUnit+TS_HEAD_LEN))
	enum
	{
		ADAPTATION_FIELD_RESERVED,
		EFFECTIVE_PAYLOAD_ONLY,
		ADAPTATION_FIELD_ONLY,
		EFFECTIVE_AFTER_ADAPTATION
	};

	// point_field 所带来的偏移
	uint32_t uiOffset = 0;
	uint32_t uiPointFiled = 0;

	/*此处 ＋tTSHeadInfo.payload_unit_start_indicator ，是因为当tTSHeadInfo.payload_unit_start_indicator为1时，
	TS包第五个字节为point_field，而当tTSHeadInfo.payload_unit_start_indicator为0时，则第五个字节即有效负载数据*/
	if (tTSHeadInfo.payload_unit_start_indicator == 1)
	{
		uiOffset = GET_ADAP_LEN+tTSHeadInfo.payload_unit_start_indicator+uiSectionOffset;
		uiPointFiled = GET_ADAP_LEN+1;
	}
	else
	{
		uiOffset = tTSHeadInfo.payload_unit_start_indicator;
	}

	if (AdapCtrl == EFFECTIVE_PAYLOAD_ONLY)
	{
		
		memcpy(p_ucDfAddr+*pDfPos,ucPayloadUnit+TS_HEAD_LEN+uiOffset,ucTSPackageLen-TS_HEAD_LEN-ucMakeSectionDec-uiSectionOffset);
		*pDfPos += ucTSPackageLen-TS_HEAD_LEN-ucMakeSectionDec-uiSectionOffset-uiPointFiled;
		return 1;
	} 
	else if (AdapCtrl == EFFECTIVE_AFTER_ADAPTATION)
	{
		memcpy(p_ucDfAddr+*pDfPos, ucPayloadUnit+TS_HEAD_LEN+GET_ADAP_LEN, ucTSPackageLen-TS_HEAD_LEN-GET_ADAP_LEN-ucMakeSectionDec);
		*pDfPos += ucTSPackageLen-TS_HEAD_LEN-GET_ADAP_LEN-ucMakeSectionDec;
		return 1;
	} 
	else
	{
		return 0;
	}
}

/*
**************************************************************************************************
Function：开始Df过滤，并返回是否过滤完毕一个需要的section
Parameter：Offset - 偏移多少位置开始匹配section，考虑到中间有小表填充
return： TRUE－Match   FALSE－Unmatch
**************************************************************************************************
*/
uint32_t filter_section(void)
{
	uint32_t i;
	
	if (ucMaskValue[0] != 0)
	{
		if ((*p_ucDfAddr) != *ucFilterValue)
		{
			return 0;
		}
	}

	for (i=1; i<ucFilterLen; i++)
	{
		if (ucMaskValue[i] != 0)
		{
			// +2 跳过section_length   +2省略
			if (*(p_ucDfAddr+i) != ucFilterValue[i])
			{
				return 0;
			}
		}
	}

	return 1;
}

// export function ------------------------------------------------------------------
/*
**************************************************************************************************
Function：判断TS文件锁定情况
Parameter：NULL
return： 0 － 未锁定TS
		188，204 － TS包长度
**************************************************************************************************
*/
uint32_t CheckTSLock(void)
{
#define LEAST_TS_SIZE (256*TS_LCOK_NUM) //判断TS锁定需要读取的TS文件大小

//	uint8_t *pTSData = new UCHAR[LEAST_TS_SIZE+1];
	uint8_t *pTSData = (uint8_t *)GxCore_Malloc(LEAST_TS_SIZE+1);
	uint32_t i;

	TsFileCtrl.open(&g_TsFile, STREAM_READ);
	TsFileCtrl.seek(&g_TsFile, 0);
	TsFileCtrl.read(&g_TsFile, pTSData, LEAST_TS_SIZE);
	TsFileCtrl.close(&g_TsFile);
	
	for (i=0; i<LEAST_TS_SIZE; i++)
	{
		if (pTSData[i] == TS_SYNC_BYTE && check_ts(&pTSData[i], 188))
		{
			ucTSPackageLen = 188;
			GxCore_Free(pTSData);
			uiFirstTSHeadPos = i;
			ucMakeSectionDec = 0;
			return ucTSPackageLen;
		}
		
		if (pTSData[i] == TS_SYNC_BYTE && check_ts(&pTSData[i], 204))
		{
			ucTSPackageLen = 204;
			GxCore_Free(pTSData);
			uiFirstTSHeadPos = i;
			ucMakeSectionDec = 204-188;
			return ucTSPackageLen;
		}
	}

	ucTSPackageLen = 0;
	GxCore_Free(pTSData);
	uiFirstTSHeadPos = 0;
	
	return ucTSPackageLen;
}

/*
**************************************************************************************************
Function：获取一个Pid的section
Parameter：NULL
return： !NULL value -已得到所要PID的一个section   的地址
		NULL-找遍TS流也没得到所要PID的一个section   
**************************************************************************************************
*/
uint8_t* GetSectionData(void)
{
#define GET_ADAP_LEN (*(ucPayloadUnit+TS_HEAD_LEN))
#define SECTION_LEN_POS	(3)
#define SECTION_LENGTH	(*(ucPayloadUnit+TS_HEAD_LEN+GET_ADAP_LEN+SECTION_LEN_POS))

	uint32_t i;
	// 一TS包，多section时，偏移多少来进行section匹配
	uint32_t uiSectionOffset = 0;
	// 记录Df通道中数据写到的位置
	uint32_t uiDfPos = 0;
	// 该section长度，仅第一次获取
	uint32_t uiSectionLen = 0;

	memset(p_ucDfAddr, 0, FILTER_SIZE);

	//在所有TS包中找到需要的包
	TsFileCtrl.open(&g_TsFile, STREAM_READ);
	for(i=0;;i++)
	{
		if (!analyse_one_ts_package(i))
		{
			TsFileCtrl.close(&g_TsFile);
			return NULL;
		}

		// match pid
		if (tTSHeadInfo.PID == uiPidValue)
		{
			//该TS包为section的头包
			if (tTSHeadInfo.payload_unit_start_indicator == 1)
			{
NEXT_SECTION:
				//获取有效负载
				get_effective_payload(tTSHeadInfo.adaptation_field_control, uiSectionOffset, &uiDfPos);
			
				// +2 有效负载的第三个字节为SectionLength
				uiSectionLen = ((*(p_ucDfAddr+1)&0xf)<<8)|(*(p_ucDfAddr+2));

				if (filter_section())
				{
					if (uiDfPos >= uiSectionLen)
					{
						TsFileCtrl.close(&g_TsFile);
						
						// TODO:   offset the read position
						s_AnalysePos += (ucTSPackageLen*(i+1));

						return p_ucDfAddr;
					} 
					else
					{
						uiSectionOffset = 0;
						continue;
					}
				} 
				else
				{
					uiDfPos = 0;
					uiSectionOffset += SECTION_LEN_POS+uiSectionLen;

					if ((uint8_t)uiSectionOffset >= ucTSPackageLen-TS_HEAD_LEN)
					{
						uiSectionOffset = 0;
						continue;
					} 
					else
					{
						goto NEXT_SECTION;
					}
				}
				


			}
			else 
			{
				if (uiDfPos == 0)
				{
					continue;
				}

				//获取有效负载
				get_effective_payload(tTSHeadInfo.adaptation_field_control, uiSectionOffset, &uiDfPos);
				
				if (uiDfPos >= uiSectionLen)
				{
					TsFileCtrl.close(&g_TsFile);
					
					// TODO:   offset the read position
					s_AnalysePos += (ucTSPackageLen*(i+1));

					return p_ucDfAddr;
				} 
				else
				{
					uiSectionOffset = 0;
					continue;
				}

			}
		}
	}
	
	TsFileCtrl.close(&g_TsFile);

	return NULL;
}



/*
**************************************************************************************************
Function：设置PID的值
Parameter：所要过滤的PID
return： 
**************************************************************************************************
*/
uint32_t SetPidValue(uint32_t Pid)
{
	uiPidValue = Pid;
	return 1;
}

/*
**************************************************************************************************
Function：配置匹配条件
Parameter：
return： 
**************************************************************************************************
*/
uint32_t ConfigFilter(const uint8_t *pFilterValue, const uint8_t *pMaskValue, uint32_t FilterLen)
{
	memcpy(ucFilterValue, pFilterValue, FilterLen);
	memcpy(ucMaskValue, pMaskValue, FilterLen);
	ucFilterLen = FilterLen;

	return 1;
}

uint32_t SetMByte(uint8_t ByteNo, uint8_t FilterValue, uint8_t MaskValue)
{
	ucFilterValue[ByteNo] = FilterValue;
	ucMaskValue[ByteNo] = MaskValue;

	return 1;
}


void SetAnalysePos(uint32_t pos)
{
	s_AnalysePos = pos;
}

void ClrAnalysePos(void)
{
	s_AnalysePos = 0;
}




