#include "autoconf.h"
#ifdef CONFIG_TFM
#include <string.h>
#include "gxtype.h"
#include "gxcas_dbg.h"
#include "assert.h"
#include "gxcas_queue.h"
#include "gxcas_descrambler.h"
#include "gxsecure/gxtfm_api.h"
#include "gxsecure/gxsecure_api.h"
#include "gxsecure/gxtfm.h"

#include "av/gxav_demux_propertytypes.h"
#include "dvbhal/gxdescrambler_hal.h"

#ifndef GXCAS_SCPU_SOFT

handle_t g_handle[8] = {0,};

#if 0  //CA库里会直接调用GxDescOpen，SelectRootkey，Set_Kn, Set_CW函数，所以这几个函数的API接口不需要再填写了
static uint32_t rootkey_id;
static uint8_t common_key[16];
static uint8_t tempsession_key[16];

static int32_t cryptoguard_assert_key(uint8_t* input, uint32_t len)
{
	uint32_t i = 0;

	for (i = 0; i < len; i++) {
		if (input[i] != 0)
			return 0;
	}
	return -1;
}

int32_t cryptoguard_sel_rootkey(uint32_t rootkeyid)
{
	if (rootkey_id != rootkeyid)
		rootkey_id = rootkeyid;
	else
		return 0;
	return 0x0000;
}

int32_t cryptoguard_set_commonkey(uint8_t *commonkey,uint32_t size)
{
	if (cryptoguard_assert_key(commonkey, size) == -1)
		return 0;
	memcpy(common_key,commonkey,16);
	return 0x0000;
}

int32_t cryptoguard_set_tempsessionkey(uint8_t *tempseesionkey,uint32_t size)
{
	if (cryptoguard_assert_key(tempseesionkey, size) == -1)
		return 0;
	memcpy(tempsession_key,tempseesionkey,16);
	return 0x0000;
}

int32_t cryptoguard_desc_open(uint32_t id)
{
	if(id >= 8){
		CAS_ERR(CAS,"cryptoguard_desc_open Failed, id is out of range!!!");
		return -1;
	}

	if(g_handle[id] == 0){
		g_handle[id] = GxDescOpen(GXDESC_MOD_TS, 0, GXDESC_ALG_CAS_CSA2);
		if((g_handle[id] == -1) || (g_handle[id] == -2)){
			CAS_ERR(CAS,"GxDescOpen Failed!!! id = %d, handle = %d\n", id, g_handle[id]);
			g_handle[id] = 0;
			return -1;
		}

		GxDescConfig(g_handle[id], GXDESC_ALG_CAS_CSA2, GXDESC_KLM_SCPU_DYNAMIC, 3);
	}

	return 0;
}

int32_t cryptoguard_desc_close(uint32_t id)
{
	if(id >= 8){
		CAS_ERR(CAS,"cd_desc_close Failed, id is out of range!!!");
		return -1;
	}

	if(g_handle[id] != 0){
		GxDescClose(g_handle[id]);
		g_handle[id] = 0;
	}

	return 0;
}

int32_t cryptoguard_set_dcw(uint32_t id, uint16_t pid,uint8_t *ecw,uint32_t size)
{
	int ret;
	GxDescCWParam param ;
	handle_t handle = g_handle[id];

	ret = GxDescSelectRootkey(handle, TFM_KEY_CWUK, rootkey_id);
	if (ret != 0) {
		CAS_ERR(CAS,"_sel_rootkey failed, ret=0x%x", ret);
		return 0x9200;
	}

	ret = GxDescSetKn(handle, 0, GXDESC_KLM_ALG_AES, common_key, 16);
	if (ret != 0) {
		CAS_ERR(CAS,"common _set_kn failed, ret=0x%x", ret);
		return 0x9200;
	}
	ret = GxDescSetKn(handle, 1, GXDESC_KLM_ALG_AES, tempsession_key, 16);
	if (ret != 0) {
		CAS_ERR(CAS,"tempseesion _set_kn failed, ret=0x%x", ret);
		return 0x9200;
	}
	memset(&param, 0, sizeof(GxDescCWParam));
	memcpy(param.even_ECW, ecw, 16);
	memcpy(param.odd_ECW, ecw, 16);
	param.ECW_len = 16;
	ret = GxDescSetCw(handle, pid, GXDESC_KLM_ALG_AES, &param);
	if (ret != 0) {
		CAS_ERR(CAS,"_set_dcw failed, ret=0x%x", ret);
		return 0x9200;
	}
	return 0x0000;
}
#endif

int32_t cryptoguard_scpu_reset(void)
{
	GxDescInitParam param;
	int ret = 0;

	GxSecureLoader firm = {
		.loader   = NULL,
		.size     = 0,
		.dst_addr = 0,
	};
	ret = GxSecure_LoadFirmware(&firm);
	if(ret != 0){
		CAS_ERR(CAS,"GxSecure_LoadFirmware failed, ret=0x%x", ret);
		return 0x9200;
	}

	while(GxSecure_GetStatus() != FW_APP_START);

	memset(&param, 0, sizeof(GxDescInitParam));
	memset(g_handle, 0, sizeof(g_handle));

	param.klm = GXDESC_KLM_SCPU_DYNAMIC;
	param.order = GXDESC_ODD_FIRST;
	GxCas_Deschal_Init(&param);

	return 0x0000;
}

#endif
#endif

