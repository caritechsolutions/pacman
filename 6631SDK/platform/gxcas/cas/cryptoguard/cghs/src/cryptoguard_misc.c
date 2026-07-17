#include "gxcas_service.h"
#include "gxcas_dbg.h"
#include "cryptoguard_misc.h"
#include "gxcore.h"
#include "gxcas_flash.h"
#include "cg_cas.h"

//static CPG_STB_INFO_STATUS s_CPGStbInfoStatus = CPG_STB_INFO_ERROR;
static CPG_CAM_INIT_STATUS s_CPGCamInitStatus = CPG_CAM_INIT_NOT;

void cpg_read_write_nvmem(uint8_t write, uint8_t *data, uint32_t length)
{
	uint32_t  Offset = 0;
    if (write) {
		GxCas_Flash_Write(Offset, data, length);
#if 0
		printf("-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-\n");
		printf("-*-                 Writing to FLASH                        -*-\n");
		printf("-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-\n");
#endif
    }
    else {
    	GxCas_Flash_Read(Offset, data, length);
    }
}

/*
void cpg_stb_info_write(void)
{
	s_CPGStbInfoStatus = CPG_STB_INFO_OK;
}

int32_t cpg_stb_info_ready(void)
{
	if(s_CPGStbInfoStatus == CPG_STB_INFO_OK)
		return TRUE;
	else
		return FALSE;
}
*/

int32_t cpg_cam_init_ready(void)
{
	if(s_CPGCamInitStatus == CPG_CAM_INIT_OK)
		return TRUE;
	else
		return FALSE;
}

int32_t cpg_add_service(GxCas_Service_Type type, GxCas_Service *arry)
{
	GxCas_Service_Info service;
	int32_t ret = 0, i = 0;
	for(i = 0; i < CPG_MAX_DESCRAMBLER_NUM; i++) {
		if (arry[i].pid == 0 || arry[i].pid == PSI_INVALID_PID)
			continue;

		memset(&service, 0, sizeof(GxCas_Service_Info));
		service.pid = arry[i].pid;
		service.ecm_pid = arry[i].ecm_pid;
		service.type = type;
		ret = GxCas_Service_Add(service);
		if (ret == -1)
			break;
	}

	return ret;
}

int32_t cpg_del_service(GxCas_Service_Type type, GxCas_Service *arry)
{
	GxCas_Service_Info service;
	int32_t ret = 0, i = 0;
	for(i = 0; i < CPG_MAX_DESCRAMBLER_NUM; i++) {
		if (arry[i].pid == 0 || arry[i].pid == PSI_INVALID_PID)
			continue;

		memset(&service, 0, sizeof(GxCas_Service_Info));
		service.pid = arry[i].pid;
		service.ecm_pid = arry[i].ecm_pid;
		service.type = type;
		ret = GxCas_Service_Delete(service);
		if (ret == -1)
			break;
	}

	return ret;
}

void cpgca_init_task(void* args)
{
	while(1) {
		if(s_CPGCamInitStatus == CPG_CAM_INIT_NOT){
			if (cg_cas_init() == CG_SUCCESS) {
				s_CPGCamInitStatus = CPG_CAM_INIT_OK;
				printf("CAid:0	%04X\n", CG.Caid[0]);
				printf("CAid:1	%04X\n", CG.Caid[1]);
				printf("CAid:2	%04X\n", CG.Caid[2]);
				printf("IrdNr   %s\n", CG.IrdNrStr);
				printf("Version %d.%d.%d\n", CG.Version[0], CG.Version[1], CG.Version[2]);
				printf("Date    %s\n", CG.CaDateStr);
			}
			else {
				CAS_ERR(CAS,"CryptoGuard CA Init Faild!!!");
				s_CPGCamInitStatus = CPG_CAM_INIT_FAILED;
			}
		}
		GxCore_ThreadDelay(100);
	}
}

int32_t cpgca_get_cpgsrv(GxCas_Service_Type type, GxCas_Service *arry)
{
	GxCas_Service_Info service;
	int32_t ret = 0, i = 0;
	for(i = 0; i < CPG_MAX_DESCRAMBLER_NUM; i++) {
		if (arry[i].pid == 0 || arry[i].pid == PSI_INVALID_PID)
			continue;

		memset(&service, 0, sizeof(GxCas_Service_Info));
		service.pid = arry[i].pid;
		service.ecm_pid = arry[i].ecm_pid;
		service.type = type;
		ret = GxCas_Service_Add(service);
		if (ret == -1)
			break;
	}

	return ret;
}

