/*
 * =====================================================================================
 *
 *       Filename:  app_ioctl.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011年12月05日 15时21分33秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#ifndef __APP_IOCTL_H__
#define __APP_IOCTL_H__

#include "gxcore.h"
#include "app_module.h"

#define APP_FRONTEND_BASE   (0x0)
#define APP_PMT_BASE        (0x100)
#define APP_CA_BASE         (0x200)
enum
{
    FRONTEND_TP_SET   = APP_FRONTEND_BASE,
    FRONTEND_DISEQC_CHANGE,
    FRONTEND_DISEQC_SET,
    FRONTEND_DISEQC_SET_EX,
    FRONTEND_DISEQC_SET_DEFAULT,
    FRONTEND_MONITOR_SET,
    FRONTEND_LOCK_STATE_SET,
    FRONTEND_LOCK_STATE_GET,
    FRONTEND_STRENGTH_GET,
    FRONTEND_QUALITY_GET,
    FRONTEND_ERRORRATE_GET,
    FRONTEND_INFO_GET,
    FRONTEND_OPEN,
    FRONTEND_CLOSE,
    FRONTEND_CONFIG_GET,
    FRONTEND_CONFIG_SET,
    FRONTEND_DEMUX_CFG,
    FRONTEND_LNB_CFG,
    FRONTEND_22K_CHANGE,
    FRONTEND_22K_SET,
    FRONTEND_POLAR_CHANGE,
    FRONTEND_POLAR_SET,
    FRONTEND_PLS_N_GET,
    FRONTEND_GET_FE_CAPS,
    FRONTEND_SET_UNICABLE,
    FRONTEND_PLSN_ROOT_CONVERT,
};

enum
{
#if DEMOD_DVB_S
	DEMOD_TYPE_DVBS,
#endif
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
	DEMOD_TYPE_DVBT,
#endif
#if DEMOD_DVB_C
	DEMOD_TYPE_DVBC,
#endif
#if DEMOD_DTMB
	DEMOD_TYPE_DTMB,
#endif
#if FM_SUPPORT
	DEMOD_TYPE_FM,
#endif
#if NET_SEARCH_SUPPORT
	DEMOD_TYPE_NET,
#endif
	DEMOD_TYPE_TOTAL,
};
extern status_t app_ioctl(uint32_t id, uint32_t cmd, void * params);
extern status_t app_mod_init(uint32_t type, uint32_t Index, int demod_type);
extern void app_mod_release(uint32_t type);


#endif


