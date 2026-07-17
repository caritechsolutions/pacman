/*
 * =====================================================================================
 *
 *       Filename:  app_pmt.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011年09月14日 15时15分24秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#ifndef __APP_ECM_H__
#define __APP_ECM_H__

#include "gxcore.h"
#include "gxsi.h"

#define ECM_MAX_ID	16
#define ELEMENT_MAX_ID	16
#define CAS_COUNT_SUPPORT 16
#define ECM_DATA_LENGTH    (1024)

enum
{
	SCRAMBLE_VIDEO,
	SCRAMBLE_AUDIO,
	SCRAMBLE_SUBT,
    SCRAMBLE_TTX,
	SCRAMBLE_EPG,
	SCRAMBLE_COMMON,
};

typedef struct AppEcmSub
{
	uint32_t    pid;
    uint32_t    element_id[ELEMENT_MAX_ID];
    uint32_t    element_count;
	uint32_t    CasPID[CAS_COUNT_SUPPORT];
	uint32_t	ScrambleType[CAS_COUNT_SUPPORT];//COMMON, VIDEO, AUDIO
	uint32_t    CasIDCount;

	int32_t     subt_id;
    uint32_t    req_id;
    uint32_t    ver;
}AppEcmSub_t;


typedef struct AppEcmCaM
{
	uint32_t	Type;
	uint32_t	ProgId;
}AppEcmCaM_t;

typedef struct app_ecm AppEcm;

struct app_ecm
{
    uint16_t    ts_src;
	uint32_t    DemuxId;// 0 or 1
	uint32_t    ecm_count;
	AppEcmCaM_t CaManager;
    AppEcmSub_t ecm_sub[ECM_MAX_ID];
    void        (*start)(AppEcm*);
    uint32_t    (*stop)(AppEcm*);
    status_t    (*get)(AppEcm*, GxMsgProperty_SiSubtableOk*);
    status_t    (*analyse)(AppEcm*, uint32_t, uint8_t*);
    void        (*storeinfo)(void*,AppEcm*);
	void        (*init)(AppEcm*);
    status_t    (*timeout)(AppEcm*, GxParseResult*);
};
extern AppEcm       g_AppEcm;

extern AppEcm       g_AppEcm2;

#endif
