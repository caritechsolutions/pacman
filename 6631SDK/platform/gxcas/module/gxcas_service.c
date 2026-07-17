#include "autoconf.h"
#include "gxcas_service.h"
#include "gxcas_psi.h"
#include "gxcore.h"
#include "gxcas_dbg.h"
#include "gxcas_descrambler.h"
#include "gxcas_chip_info.h"

struct gxcas_service_ctrl{
	handle_t mutex;
	uint32_t total_count;
	GxCas_Service_Info info[GXCA_MAX_DESCRAMBLER_NUM];
};

static struct gxcas_service_ctrl _srvctl;

uint32_t GxCas_Service_GetTotalCount(void)
{
	uint32_t i = 0;
	GxCore_MutexLock(_srvctl.mutex);
	_srvctl.total_count = 0;
	for(i = 0; i < GXCA_MAX_DESCRAMBLER_NUM; i++) {
		if (_srvctl.info[i].descid != -1)
			_srvctl.total_count++;
	}
	GxCore_MutexUnlock(_srvctl.mutex);
	return _srvctl.total_count;
}

void GxCas_Service_GetInfo(GxCas_Service_Info *arry)
{
	uint32_t i = 0, j = 0;
	GxCore_MutexLock(_srvctl.mutex);
	if (_srvctl.total_count == 0)
		CAS_DBG(DESC, "Pls Call GxCas_Service_GetTotalCount First!!");
	for (i = 0; i < GXCA_MAX_DESCRAMBLER_NUM; i++) {
		if (_srvctl.info[i].descid != -1) {
			arry[j].pid = _srvctl.info[i].pid;
			arry[j].descid = _srvctl.info[i].descid;
			arry[j].ecm_pid = _srvctl.info[i].ecm_pid;
			arry[j].record = _srvctl.info[i].record;
			arry[j].type = _srvctl.info[i].type;
			j++;
		}
	}
	GxCore_MutexUnlock(_srvctl.mutex);
}

void GxCas_Service_Config(GxCas_Service_Info service)
{
	uint32_t i = 0;
	GxCore_MutexLock(_srvctl.mutex);
	for(i = 0; i < GXCA_MAX_DESCRAMBLER_NUM; i++) {
		if (_srvctl.info[i].pid == service.pid && _srvctl.info[i].descid == service.descid) {
			_srvctl.info[i].type = service.type;
			_srvctl.info[i].record = service.record;
			_srvctl.info[i].ecm_pid = service.ecm_pid;
			break;
		}
	}
	GxCore_MutexUnlock(_srvctl.mutex);
	if (i == GXCA_MAX_DESCRAMBLER_NUM)
		CAS_DBG(DESC, "Error Config service");
}

int32_t GxCas_Service_Add(GxCas_Service_Info service)
{
	uint32_t i = 0;

	GxCore_MutexLock(_srvctl.mutex);
	_srvctl.total_count = 0;//usr count has changed
	for(i = 0; i < GXCA_MAX_DESCRAMBLER_NUM; i++) {
#ifdef CONFIG_CONAX
		if ((_srvctl.info[i].pid == service.pid) && (_srvctl.info[i].type == service.type)) {
#else
		if (_srvctl.info[i].pid == service.pid) {
			_srvctl.info[i].type |= service.type;
#endif
			_srvctl.info[i].record = service.record;
			_srvctl.info[i].ecm_pid = service.ecm_pid;
			goto out;
		}
	}

	for(i = 0; i < GXCA_MAX_DESCRAMBLER_NUM; i++) {
		if (_srvctl.info[i].descid == -1) {
#ifdef CONFIG_CONAX
			if(service.type == GXCAS_SERVICE_PVR)
				_srvctl.info[i].descid = GxCas_Deschal_Open(GXDESC_MOD_TS, 1, GXDESC_ALG_CAS_CSA2);
			else
				_srvctl.info[i].descid = GxCas_Deschal_Open(GXDESC_MOD_TS, 0, GXDESC_ALG_CAS_CSA2);
#else
			_srvctl.info[i].descid = GxCas_Desc_Open();
#endif
			if (_srvctl.info[i].descid == -1) {
				CAS_DBG(DESC, "Open Desc Error");
				goto err;
			}

			_srvctl.info[i].pid = service.pid;
			_srvctl.info[i].type = service.type;
			_srvctl.info[i].record = service.record;
			_srvctl.info[i].ecm_pid = service.ecm_pid;
			break;
		}
	}

out:
	GxCore_MutexUnlock(_srvctl.mutex);
	return 0;
err:
	GxCore_MutexUnlock(_srvctl.mutex);
	return -1;
}

int32_t GxCas_Service_Delete(GxCas_Service_Info service)
{
	int32_t i = 0;
	GxCore_MutexLock(_srvctl.mutex);
	_srvctl.total_count = 0;

	for (i = 0; i < GXCA_MAX_DESCRAMBLER_NUM; i++) {
		if ((_srvctl.info[i].pid == service.pid) || service.pid == PSI_INVALID_PID) {
			if (_srvctl.info[i].type == service.type || service.type == -1) {
				GxCas_Desc_Close(_srvctl.info[i].descid);
				memset(&_srvctl.info[i], 0, sizeof(GxCas_Service_Info));
				_srvctl.info[i].descid = -1;
			} else
				_srvctl.info[i].type &= ~service.type;
		}
	}
	GxCore_MutexUnlock(_srvctl.mutex);
	return 0;
}

int32_t GxCas_Service_Bind(GxCas_Service_Info service)
{
	uint8_t buf[8] = {0};
	if(CONSTEL_CHIP)
		return 0;
	return GxCas_Desc_SetCW(service.descid, service.pid, buf, buf, 8);
}

int32_t GxCas_Service_SetCW(GxCas_Service_Info service, uint8_t *oddkey, uint8_t *evenkey, uint32_t keylen)
{
	return GxCas_Desc_SetCW(service.descid, service.pid, oddkey, evenkey, keylen);
}

void GxCas_Service_Init(void)
{
	uint32_t i = 0;
	if (_srvctl.mutex == 0)
		GxCore_MutexCreate(&_srvctl.mutex);

	for (i = 0; i < GXCA_MAX_DESCRAMBLER_NUM; i++)
		_srvctl.info[i].descid = -1;
}
