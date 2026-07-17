/*
 * =====================================================================================
 *
 *       Filename:  app_ioctl.c
 *
 *    Description:  dvb_ioctl
 *
 *        Version:  1.0
 *        Created:  2012年02月14日 17时08分56秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "module/app_ioctl.h"
//#include "module/app_ca_manager.h"

status_t app_ioctl(uint32_t id, uint32_t cmd, void * params)
{
    status_t ret = GXCORE_ERROR;
    switch (cmd & (~0xff))
    {
        case APP_FRONTEND_BASE:
            ret = nim_ioctl(id, cmd, params);
            break;
        default:
            break;
    }

    return ret;
}

status_t app_mod_init(uint32_t type, uint32_t Index,int demod_type)
{
    status_t ret = GXCORE_ERROR;

    switch (type >> 8)
    {
        case APP_FRONTEND_BASE:
            ret = nim_init(Index, demod_type);
            break;
        default:
            break;
    }

    return ret;
}

void app_mod_release(uint32_t type)
{
    switch (type >> 8)
    {
        case APP_FRONTEND_BASE:
            nim_release();
            break;
        default:
            break;
    }
}



