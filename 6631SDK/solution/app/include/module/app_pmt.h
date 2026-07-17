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

#ifndef __APP_PMT_H__
#define __APP_PMT_H__

#include "gxcore.h"
#include "gxsi.h"

typedef struct app_pmt AppPmt;
struct app_pmt
{
    //external, only start need
    uint32_t    ts_src;
    uint32_t    pid;
    uint32_t    dmx_id;
    //internal
    int32_t     subt_id;
    uint32_t    req_id;
    uint32_t    ver;
    uint32_t    sync_flag;
    void        (*start)(AppPmt*);
    uint32_t    (*stop)(AppPmt*);
    status_t    (*get)(AppPmt*, GxMsgProperty_SiSubtableOk*);
    status_t    (*analyse)(AppPmt*, uint8_t*);
    void        (*cb)(PmtInfo *pmt_info);
};

extern AppPmt       g_AppPmt;
#endif

