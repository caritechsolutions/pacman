#ifndef __CASCAM_H__
#define __CASCAM_H__

#include "gxtype.h"
//CA base MSGID
#define CASCAM_MSG_CDCAS_BASEID  0x01000000
#define CASCAM_MSG_DVTCAS_BASEID  0x02000000
#define CASCAM_MSG_ABVCAS_BASEID  0x03000000
#define CASCAM_MSG_TRCAS_BASEID  0x04000000
#define CASCAM_MSG_DESAICAS_BASEID  0x05000000
#define CASCAM_MSG_CONAX_BASEID  0x06000000
#define CASCAM_MSG_CGCAS_BASEID  0x07000000
#define CASCAM_MSG_GOSCAS_BASEID  0x08000000
#define CASCAM_MSG_IRDETOCAS_BASEID  0x09000000
#define CASCAM_MSG_TELECASTCAS_BASEID  0x0B000000

//service
#define CASCAM_MAXNUM_SERVICE			8// due to CD_MAX_DESCRAMBLER_NUM

typedef enum {
	CASCAM_TEST_DBG_MODULE_UI_MSG = 1<<0,
	CASCAM_TEST_DBG_MODULE_FINGER = 1<<1,
	CASCAM_TEST_DBG_MODULE_OSD = 1<<2,
	CASCAM_TEST_DBG_MODULE_MAIL = 1<<3,
}CascamTestDebug;

typedef enum {
	CASCAM_DBG_GXCAS_MODULE_CAS = 1<<0,
	CASCAM_DBG_GXCAS_MODULE_CASLIB = 1<<1,
	CASCAM_DBG_GXCAS_MODULE_DEMUX = 1<<2,
	CASCAM_DBG_GXCAS_MODULE_DESC = 1<<3,
	CASCAM_DBG_GXCAS_MODULE_SMC = 1<<4,
	CASCAM_DBG_GXCAS_MODULE_FLASH = 1<<5,
	CASCAM_DBG_GXCAS_MODULE_SI = 1<<6,
	CASCAM_DBG_GXCAS_MODULE_EADLIB = 1<<7,
}CascamGxCasDebug;

typedef enum {
	CASCAM_SMC_LOW_LEVEL,
	CASCAM_SMC_HIGH_LEVEL
}CascamSmcPol;

typedef struct{
	char name[128];
	uint32_t size;
	uint32_t offset;
}CascamFlash;

typedef struct{
	uint32_t sci_switch; // 1:on 0:off
	CascamSmcPol vcc_pole;
	CascamSmcPol detect_pole;
}CascamSmartCard;

typedef struct{
	uint32_t dmx_id;
	uint32_t ts_id;
	CascamSmartCard sci;
	CascamFlash flash;
	CascamFlash backup;
}CascamInitParam;

typedef struct{
	uint16_t pmt_pid;
	uint16_t service_id;
	uint16_t audio_pid;
	uint16_t video_pid;
	uint8_t sessionid;
}CascamSwitchChannel;

typedef struct{
	uint16_t audio_ecm_pid;
	uint16_t video_ecm_pid;
	uint16_t service_id;
}CascamCurEcmPid;

typedef struct{
	uint16_t ecm_pid;
	uint16_t pid;
}CascamService;

typedef struct{
	uint16_t pmt_pid;
	uint16_t service_id;
	CascamService audio;//audio.ecm_pid 0 is invalid
	CascamService video;//video.ecm_pid 0 is invalid
}CascamQuickSwitchChannel;

typedef struct{
	CascamService service[CASCAM_MAXNUM_SERVICE];
}CascamServiceInfo;

typedef struct{
    uint32_t msg_id;//basic_id  + ca_msg_id
    void *data;
}CascamOsdEvent;

enum {
	CASCAM_PVR_TYPE_RECORD,
	CASCAM_PVR_TYPE_TIMESHIFT,
	CASCAM_PVR_TYPE_REC_PLAY,
	CASCAM_PVR_TYPE_TMS_PLAY,
	CASCAM_PVR_TYPE_MAX_NUM,
};

typedef struct{
	uint8_t *pPath;
	uint16_t wPathLen;
	uint8_t bType;
}CascamPVRPath;

typedef enum {
	CASCAM_PVR_PLAYER_STATE = 0, //4bytes playerstate
	CASCAM_PVR_VOLUMN_SIZE = 1, //4bytes Mb
	CASCAM_PVR_GET_METADATA_NUM = 20, //4bytes metadata number
	CASCAM_PVR_GET_METADATA = 21, //CascamConaxMetadata
}CascamPVRStateType;

typedef struct {
	uint16_t  id;
	uint16_t  URI;
	uint32_t  time;
	uint64_t  filePos;
}CascamMetadata;

typedef struct{
	uint32_t dwType;
	void* param;
}CascamPVRState;

/***********************
 * cascam init
 * func: cascam initialization
 * param:init_param, hw config and ca flash config
 * return:
 * *********************/
int32_t cascam_init(CascamInitParam* init_param);

/***********************
 * cascam set
 * func: stb send cmd to cascam
 * param: sub_id[in], ca function enum
 *        param[in], ca function param
 * return:
 * *********************/
int32_t cascam_set(uint32_t sub_id, void* param);

/***********************
 * cascam get
 * func: stb get date from cascam
 * param: sub_id[in], ca function enum
 *        param[out], ca function param
 * return:
 * *********************/
int32_t cascam_get(uint32_t sub_id, void* param);

/***********************
 * cascam cascam wait event
 * func: cascam send cmd to stb
 * param: type[out], ca base msgid + ca msgid
 *        param[out], ca event param
 * return:
 * *********************/
int32_t cascam_wait_event(uint32_t *type, void **param);

/***********************
 * cascam enable debug
 * func: stb enable debug
 * param:
 * return:
 * *********************/
int32_t cascam_enable_debug(void);

/***********************
 * cascam disable debug
 * func: stb disable debug
 * param:
 * return:
 * *********************/
int32_t cascam_disable_debug(void);

/***********************
 * cascam contrl_gxcas debug
 * func: stb enable/disable gxcas debug
 * param: type, 1: enable debug; 0: disable debug
 *        flag, module debug type 
 * return:
 * *********************/
int32_t cascam_control_gxcas_debug(uint8_t type, uint32_t flag);

/***********************
 * cascam contrl test debug
 * func: stb enable/disable cascam ui function test
 * param: type, 1: enable debug; 0: disable debug
 *        flag, module debug type 
 * return:
 * *********************/
int32_t cascam_control_cascam_test(uint8_t type, uint32_t flag);

#endif
