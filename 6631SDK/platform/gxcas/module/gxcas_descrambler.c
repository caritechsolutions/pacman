#include "gxcas_dbg.h"
#include "gxcore.h"
#include "gxcas_descrambler.h"
#include "gxcas_chip_info.h"

static uint32_t s_dmxid = 0;
static handle_t dev = -1;
static handle_t	dmx = -1;

static uint32_t GET_KEY_HIGH(const uint8_t *key)
{
	return ((uint32_t)(key[3] + (key[2] << 8)+ (key[1] << 16) + (key[0] << 24)));
}

static uint32_t GET_KEY_LOW(const uint8_t *key)
{
	return ((uint32_t)(key[7] + (key[6] << 8)+ (key[5] << 16) + (key[4] << 24)));
}

// due to api/gxdescrambler_hal
int32_t GxCas_Deschal_Init(GxDescInitParam *param)
{
	int32_t ret = 0;

	ret = GxDescInit(param);
	if(ret != 0){
		CAS_ERR(CAS,"GxDescInit Failed!!!");
	}
	return ret;
}

handle_t GxCas_Deschal_Open(GxDescMod mod, uint32_t id, GxDescAlg alg)
{
	handle_t handle = 0;

	handle = GxDescOpen(mod, id, alg);
	if(handle == -1){
		CAS_ERR(CAS,"GxDescOpen Failed!!!");
	}
	return handle;
}

void GxCas_Deschal_Close(handle_t handle)
{
	GxDescClose(handle);
}

void GxCas_Deschal_Config(handle_t handle, GxDescAlg alg, GxDescKLM klm, int klm_level)
{
	GxDescConfig(handle, alg, klm, klm_level);
}

int32_t GxCas_Deschal_SelectRootkey(handle_t handle, uint32_t rootkeyid, uint32_t rootkeysub)
{
	int32_t ret = 0;

	ret = GxDescSelectRootkey(handle, rootkeyid, rootkeysub);
	if(ret != 0){
		CAS_ERR(CAS,"GxDescSelectRootkey Failed!!!");
	}
	return ret;
}

int32_t GxCas_Deschal_SetKn(handle_t handle, uint32_t key_level, GxDescKLMAlg alg, uint8_t* EKn, uint32_t len)
{
	int32_t ret = 0;

	ret = GxDescSetKn(handle, key_level, alg, EKn, len);
	if(ret != 0){
		CAS_ERR(CAS,"GxDescSetKn Failed!!!");
	}
	return ret;
}

int32_t GxCas_Deschal_SetCw(handle_t handle, uint16_t pid, GxDescKLMAlg alg, GxDescCWParam *param)
{
	int32_t ret = 0;

	ret = GxDescSetCw(handle, pid, alg, param);
	if(ret != 0){
		CAS_ERR(CAS,"GxDescSetCw Failed!!!");
	}
	return ret;
}

int32_t GxCas_Deschal_SetEvenCw(handle_t handle, uint16_t pid, GxDescKLMAlg alg, GxDescCWParam *param)
{
	int32_t ret = 0;

	ret = GxDescSetEvenCw(handle, pid, alg, param);
	if(ret != 0){
		CAS_ERR(CAS,"GxDescSetEvenCw Failed!!!");
	}
	return ret;
}

int32_t GxCas_Deschal_SetOddCw(handle_t handle, uint16_t pid, GxDescKLMAlg alg, GxDescCWParam *param)
{
	int32_t ret = 0;

	ret = GxDescSetOddCw(handle, pid, alg, param);
	if(ret != 0){
		CAS_ERR(CAS,"GxDescSetOddCw Failed!!!");
	}
	return ret;
}

int32_t GxCas_Deschal_UpdateTDparam(handle_t handle, GxDescCWParam *param)
{
	int32_t ret = 0;

	ret = GxDescUpdateTDparam(handle, param);
	if(ret != 0){
		CAS_ERR(CAS,"GxDescUpdateTDparam Failed!!!");
	}
	return ret;
}

int32_t GxCas_Deschal_AddPID(handle_t handle, uint16_t pid)
{
	int32_t ret = 0;

	ret = GxDescAddPID(handle, pid);
	if(ret != 0){
		CAS_ERR(CAS,"GxDescAddPID Failed!!!");
	}
	return ret;
}

int32_t GxCas_Deschal_DelPID(handle_t handle, uint16_t pid)
{
	int32_t ret = 0;

	ret = GxDescDelPID(handle, pid);
	if(ret != 0){
		CAS_ERR(CAS,"GxDescDelPID Failed!!!");
	}
	return ret;
}

int32_t GxCas_Desc_Init(uint32_t dmxid)
{
	dev = GxAVCreateDevice(0);
	if(dev < 0) {
		CAS_ERR(DESC,"Open device err");
		return -1;
	}

	dmx = GxAVOpenModule(dev, GXAV_MOD_DEMUX, dmxid);
	if(dmx < 0) {
		CAS_ERR(DESC,"Open module err");
		GxAVDestroyDevice(dev);
		return -1;
	}
	s_dmxid = dmxid;

	return 0;
}

int32_t GxCas_Desc_Open(void)
{
	int32_t ret;
	int ca_id = 0;

	if(CONSTEL_CHIP){
		handle_t handle = 0;
		handle = GxCas_Deschal_Open(GXDESC_MOD_TS, s_dmxid, GXDESC_ALG_CAS_CSA2);
		if(handle == -1)
			ret = -1;
		else
			ret = 0;
		ca_id = handle;
	}else{
		GxDemuxProperty_CA ca;

		ret = GxAVGetProperty(dev, dmx,
				GxDemuxPropertyID_CAAlloc,
				&ca, sizeof(GxDemuxProperty_CA));
		ca_id = ca.ca_id;
	}
	if(ret < 0) {
		CAS_ERR(DESC,"Allco descrambler err");
		GxAVCloseModule(dev,dmx);
		GxAVDestroyDevice(dev);
		return -1;
	}
	return ca_id;
}

void GxCas_Desc_Close(int32_t descid)
{
	int32_t ret;

	if (descid == -1)
		return;

	if(CONSTEL_CHIP){
		handle_t handle = 0;
		handle = descid;
		GxCas_Deschal_Close(handle);
		ret = 0;
	}else{
		GxDemuxProperty_CA ca;
		ca.ca_id = descid;
		ret = GxAVSetProperty(dev, dmx,
				GxDemuxPropertyID_CAFree,
				&ca, sizeof(GxDemuxProperty_CA));
	}
	if(ret < 0)
		CAS_ERR(DESC,"Free descrambler err");
}

int32_t GxCas_Desc_SetCW(int32_t descid,
		uint16_t pid,
		uint8_t* OddKey,
		uint8_t* EvenKey,
		uint32_t KeyLen )
{
	int32_t status;
	GxDemuxProperty_CA ca = {0,};
	GxDemuxProperty_SlotQueryByPid slot = {0,};

	slot.pid = pid;

	status = GxAVGetProperty(dev, dmx,
			GxDemuxPropertyID_SlotQueryByPid,
			&slot, sizeof(GxDemuxProperty_SlotQueryByPid));
	if(status >= 0) {
		ca.ca_id           = descid;
		ca.slot_id         = slot.slot_id;
		ca.flags           = DMX_CA_KEY_ODD | DMX_CA_KEY_EVEN;
		ca.odd_key_high    = GET_KEY_HIGH(OddKey);
		ca.odd_key_low     = GET_KEY_LOW(OddKey);
		ca.even_key_high   = GET_KEY_HIGH(EvenKey);
		ca.even_key_low    = GET_KEY_LOW(EvenKey);

		status = GxAVSetProperty(dev, dmx,
				GxDemuxPropertyID_CAConfig,
				&ca, sizeof(GxDemuxProperty_CA));
		if(status< 0) {
			CAS_ERR(DESC,"Set key err!");
			return -1;
		}
	} else {
		CAS_ERR(DESC,"Can't find slot id via pid");
		return -1;
	}
	return 0;
}

int GxCas_Deschal_IFCP_LoadAPP(GxIfcpApp *app)
{
	int32_t ret = 0;
	ret = GxIFCP_LoadAPP(app);
	if(ret != 0){
		CAS_ERR(CAS,"GxIFCP_LoadAPP Failed!!!");
	}
	return ret;
}
