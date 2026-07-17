#include "gxcas_service.h"
#include "gxcas_dbg.h"
#include "cryptoguard_misc.h"
#include "IntCAM.h"
#include "gxcore.h"

static CPG_STB_INFO_STATUS s_CPGStbInfoStatus = CPG_STB_INFO_ERROR;
static CPG_CAM_INIT_STATUS s_CPGCamInitStatus = CPG_CAM_INIT_NOT;
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
	while(1){
		if(TRUE == cpg_stb_info_ready()){
			if(TRUE == cpg_smartcard_ready()){
				if(s_CPGCamInitStatus == CPG_CAM_INIT_NOT){
					if(TRUE != CAInit()){
						CAS_ERR(CAS,"CryptoGuard CA Init Faild!!!");
						s_CPGCamInitStatus = CPG_CAM_INIT_FAILED;
					}else
						s_CPGCamInitStatus = CPG_CAM_INIT_OK;
				}
			} 
		}else
			CAS_ERR(CAS,"CryptoGuard CA Stb Info Error!!!");

		GxCore_ThreadDelay(100);
	}
}

