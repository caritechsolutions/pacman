/*
 * =====================================================================================
 *
 *       Filename:  db_type.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2012年04月26日 10时50分20秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#ifndef __DB_TYPE_H__
#define __DB_TYPE_H__

#include <stdlib.h>
#include <stdint.h>

#define SAT_NAME_SIZE   16
#define PROG_NAME_SIZE  21
// xls db struct
struct sat
{
    /* 16B*/
    char sat_name[SAT_NAME_SIZE];

    /* 4B*/
    uint32_t tunerSel           :2;
    uint32_t video_type         :3;
    uint32_t audio_type         :3;
    uint32_t reserved1          :24;

    /* 4B*/
    uint32_t longitude          :11;//0-1800
    uint32_t lnb_type           :2;
    uint32_t lnb1               :4;
    uint32_t lnb2               :4;
    uint32_t diseqc12_pos       :8;//1-255
    uint32_t sat_22k            :2;
    uint32_t direction          :1;

    /* 4B*/
    uint32_t diseqc10           :3;
    uint32_t sat_12v            :1;
    uint32_t tp_start_pos       :13;
    uint32_t tp_num             :13;
    uint32_t diseqc_motor       :2;
};

struct tp
{
    /* 4B*/
    uint32_t frequency          :14;
    uint32_t polar              :1;
    uint32_t symbol             :16;
    uint32_t tp_22k             :1;
};

struct prog
{
    /* 4B*/
    uint32_t satid              :13;
    uint32_t tpid               :13;
    uint32_t fav_type           :4;
    uint32_t audio_format       :2;

    /* 4B*/
    uint32_t serviceid          :16;
    uint32_t casid              :13;
    uint32_t flagav             :1;
    uint32_t scramble           :1;
    uint32_t lock               :1;

    /* 4B*/
    uint32_t vpid               :13;
    uint32_t apid               :13;
    uint32_t vtype              :3;
    uint32_t atype              :3;

    /* 4B*/
    uint32_t pmtpid             :13;
    uint32_t pcrpid             :13;
    uint32_t hide               :1;
    uint32_t skip               :1;
    uint32_t tunersel           :2;
    uint32_t reserved2          :2;

    /* 20B*/
    char prog_name[PROG_NAME_SIZE];
};
#endif

