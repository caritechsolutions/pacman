#include "autoconf.h"
#ifdef CONFIG_ACPU
#include "gxcore.h"
#include "gxcas_dbg.h"
#include "gxcas_smartcard.h"
#include "gxcas_service.h"
#include "gxcas_descrambler.h"

/*++
功能：CA程序用此函数设置解扰器。将当前周期及下一周期的CW送给解扰器。
参数：
	szOddKey:				奇CW的数据。
	szEvenKey:				偶CW的数据。
	byKeyLen:				CW的长度。
	bTapingControl:			true：允许录像,false：不允许录像。
	byCWEncrypt:			0:CW是明文，1:CW是密文，需要安全芯片解密，其他值:保留
--*/
int32_t CPG_SetDescrCW(uint8_t byKeyLen, uint8_t * szOddKey, uint8_t * szEvenKey, uint8_t bTapingControl,uint8_t byCWEncrypt)
{
	uint32_t i = 0, count = 0;
	GxCas_Service_Info *srv = NULL;
	int32_t ret = -1;

	if(byKeyLen != 8){
		CAS_DBG(CASLIB,"byKeyLen error = %0x",byKeyLen);
		return ret;
	}

	count = GxCas_Service_GetTotalCount();
	srv = GxCore_Mallocz(count * sizeof(GxCas_Service_Info));
	if (srv == NULL)
		return ret;
	GxCas_Service_GetInfo(srv);
	for(i = 0; i < count; i ++) {
		GxCas_Service_SetCW(srv[i], szOddKey, szEvenKey, byKeyLen);
		srv[i].record = bTapingControl;
		GxCas_Service_Config(srv[i]);
	}
	GxCore_Free(srv);
	return ret;
}

#endif


