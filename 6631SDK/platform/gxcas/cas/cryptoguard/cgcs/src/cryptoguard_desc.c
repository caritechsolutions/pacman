#include "autoconf.h"
#ifdef CONFIG_ACPU
#include "gxcore.h"
#include "gxcas_dbg.h"
#include "gxcas_smartcard.h"
#include "gxcas_service.h"
#include "gxcas_descrambler.h"
#include "gxcas_chip_info.h"

static int32_t cg_set_dcw(GxCas_Service_Info srv,uint8_t *ecw,uint32_t size)
{
    int32_t ret = 0;
    uint8_t odd_key[16] = {0};
    uint8_t even_key[16] = {0};

	if(CONSTEL_CHIP){
		GxDescCWParam param ;

		GxCas_Deschal_Config(srv.descid, GXDESC_ALG_CAS_CSA2, GXDESC_KLM_NONE, 0);

		memset(&param, 0, sizeof(GxDescCWParam));
		memcpy(param.even_ECW, ecw+8, 8);
		memcpy(param.odd_ECW, ecw, 8);
		param.ECW_len = 8;
		ret = GxCas_Deschal_SetCw(srv.descid, srv.pid, GXDESC_KLM_ALG_3DES, &param);
		if (ret != 0) {
			CAS_ERR(CAS,"GxCas_Deschal_SetCw failed, ret=0x%x", ret);
			return 0x9200;
		}
    }else{
        memcpy(odd_key, ecw, 8);
        memcpy(even_key, ecw+8, 8);
        ret = GxCas_Service_SetCW(srv, odd_key ,even_key, size);
    }

    return ret;

}

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
	uint8_t buf[17] = {0};
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
		//GxCas_Service_SetCW(srv[i], szOddKey, szEvenKey, byKeyLen);
        memcpy(buf, szOddKey, 8);
        memcpy(buf + 8, szEvenKey, 8);
        cg_set_dcw(srv[i], buf, 17);
        GxCas_Service_Bind(srv[i]);

        srv[i].record = bTapingControl;
		GxCas_Service_Config(srv[i]);
	}
	GxCore_Free(srv);
	return ret;
}

int32_t cryptoguard_scpu_reset(void)
{
	if(CONSTEL_CHIP){
		GxDescInitParam param;
		memset(&param, 0, sizeof(GxDescInitParam));

		param.klm = GXDESC_KLM_NONE;
		param.order = GXDESC_ODD_FIRST;
		GxCas_Deschal_Init(&param);
	}

	return 0;
}
#endif


