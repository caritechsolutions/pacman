/*
 * =====================================================================================
 *
 *       Filename:  app_display_bg.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011年07月04日 09时42分54秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "gxcore.h"

typedef enum
{
    BG_TYPE_PP,
    BG_TYPE_SPP,
    BG_TYPE_BG
}app_bg_type;


//EXP. APP_SHOW_BG(BG_TYPE_PP, "bg.img")    "player"
//EXP. APP_SHOW_BG(BG_TYPE_SPP, "bg.jpg")   "gui"
//EXP. APP_SHOW_BG(BG_TYPE_BG, "0xffffff")  "device"
#define APP_SHOW_BG()

//EXP. APP_HIDE_BG(BG_TYPE_PP)
//EXP. APP_HIDE_BG(BG_TYPE_SPP)
//EXP. APP_HIDE_BG(BG_TYPE_BG)
#define APP_HIDE_BG()
