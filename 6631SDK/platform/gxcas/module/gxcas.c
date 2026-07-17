#include "gxcas.h"
#include "gxcas_dbg.h"
#include "gxcas_misc.h"
#include "gxcas_queue.h"
#include "gxcas_descrambler.h"
#include "gxcas_demux.h"

unsigned int module_debug;
static gxcas_control *ca_ops;
void GxCas_Init(gxcas_control *ca, const GxCasInitParam param)
{
	GxCas_VersionInfo();
	GxCas_QueueInit();
	GxCas_Demux_Init(param.ts_id, param.dmx_id);
	GxCas_Desc_Init(param.dmx_id);
	ca_ops = ca;
}

int32_t GxCas_PostEvent(unsigned long type, void *value, unsigned long len)
{
	return GxCas_QueuePut(type, value, len, 0);
}

int32_t GxCas_WaitEvent(unsigned long *type, void **value)
{
	return GxCas_QueueGet(type, value, -1);
}

int32_t GxCas_Set(unsigned long type, void *value)
{
	if (ca_ops && ca_ops->set)
		return ca_ops->set(type, value);

	CAS_DBG(CAS,"Pls Init GxCas First");
	return -1;
}

int32_t GxCas_Get(unsigned long type, void *value)
{
	if (ca_ops && ca_ops->get)
		return ca_ops->get(type, value);

	CAS_DBG(CAS,"Pls Init GxCas First");
	return -1;
}

int32_t GxCas_Enable_Debug(unsigned long type)
{
	switch(type) {
		case GXCAS_DEBUG_MOUDULE_CAS:
		case GXCAS_DEBUG_MOUDULE_DEMUX:
		case GXCAS_DEBUG_MOUDULE_DESC:
		case GXCAS_DEBUG_MOUDULE_SMC:
		case GXCAS_DEBUG_MOUDULE_FLASH:
		case GXCAS_DEBUG_MOUDULE_SI:
		case GXCAS_DEBUG_MOUDULE_CASLIB:
		case GXCAS_DEBUG_MOUDULE_EADLIB:
			module_debug |= GXCAS_GETNUM(type);
			return 0;
		default:
			break;
	}
	CAS_DBG(CAS,"Pls Check Debug Type First");
	return -1;
}

int32_t GxCas_Disable_Debug(unsigned long type)
{
	switch(type) {
		case GXCAS_DEBUG_MOUDULE_CAS:
		case GXCAS_DEBUG_MOUDULE_DEMUX:
		case GXCAS_DEBUG_MOUDULE_DESC:
		case GXCAS_DEBUG_MOUDULE_SMC:
		case GXCAS_DEBUG_MOUDULE_FLASH:
		case GXCAS_DEBUG_MOUDULE_SI:
		case GXCAS_DEBUG_MOUDULE_CASLIB:
			module_debug &= ~(GXCAS_GETNUM(type));
			return 0;
		default:
			break;
	}
	CAS_DBG(CAS,"Pls Check Debug Type First");
	return -1;
}
