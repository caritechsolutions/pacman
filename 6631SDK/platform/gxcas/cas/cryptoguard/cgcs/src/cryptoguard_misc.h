#ifndef __UTI_MISC_H___
#define __UTI_MISC_H___
#include "gxtype.h"
#include "gxcas_psi.h"
#include "gxcas_service.h"
#include "gxcas.h"
#include "cryptoguard_api.h"

#define CPG_MAX_DESCRAMBLER_NUM (8)

void cpgca_task(void* args);
void cpgca_init_task(void* args);
typedef enum {
	CPG_CAM_INIT_NOT = 0,
	CPG_CAM_INIT_FAILED,
	CPG_CAM_INIT_OK
}CPG_CAM_INIT_STATUS;

typedef enum {
	CPG_STB_INFO_ERROR = 0,
	CPG_STB_INFO_OK
}CPG_STB_INFO_STATUS;

typedef enum {
	CPG_CARD_FAIL = 0,
	CPG_CARD_OK
}CPG_CARD_STATUS;

struct cpg_service{
	uint16_t a_pid;
	uint16_t v_pid;
	uint16_t service_id;
	handle_t mutex;
};

extern struct cpg_service cpgsrv;
int32_t cpg_add_service(GxCas_Service_Type type, GxCas_Service *arry);
int32_t cpg_del_service(GxCas_Service_Type type, GxCas_Service *arry);

int32_t cpg_start_emm_filter(uint16_t emm_pid);
int32_t cpg_start_ecm_filter(uint16_t ecm_pid);
int32_t cpg_stop_emm_filter(uint16_t emm_pid);
int32_t cpg_stop_ecm_filter(uint16_t ecm_pid);
int32_t cpg_stop_all_filter(void);

void cpg_smartcard_change(uint32_t flag);
int32_t cpg_smartcard_ready(void);
void cpg_stb_info_write(void);
int32_t cpg_stb_info_ready(void);
int32_t cpg_cam_init_ready(void);

int32_t cg_baseinfo_get(unsigned long type, void *value);
int32_t cg_menuinfo_get(unsigned long type, void *value);

int32_t cg_baseinfo_set(unsigned long type, void *value);

int32_t cpg_switch_channel(uint16_t pmt_pid,uint32_t service_id,uint16_t audio_pid,uint16_t video_pid);

//descrabmle
int32_t CPG_SetDescrCW(uint8_t byKeyLen, uint8_t * szOddKey, uint8_t * szEvenKey, uint8_t bTapingControl,uint8_t byCWEncrypt);

#endif

