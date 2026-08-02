/*
 * =====================================================================================
 *
 *       Filename:  full_screen.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011��08��26�� 08ʱ53��09��
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#ifndef __FULL_SCREEN_H__
#define __FULL_SCREEN_H__
#include "gxcore.h"
#include "app.h"

enum
{
    FULL_TV_TO_RADIO,
    FULL_RADIO_TO_TV,
    FULL_TATE_SHOW,
    FULL_STATE_HIDE,
    FULL_MUTE_HIDE,

};

enum
{
    EVENT_TV_RADIO  ,   /* KEY TRIGGER */
    EVENT_RECALL,
    EVENT_MUTE      ,   /* KEY TRIGGER */
    EVENT_PAUSE     ,   /* KEY TRIGGER */
    EVENT_SUBT      ,   /* KEY TRIGGER */
    EVENT_TTX      ,   /* KEY TRIGGER */
    EVENT_REC       ,   /* KEY TRIGGER */
    EVENT_SPEED       ,   /* KEY TRIGGER */
    EVENT_STATE       ,   /* TIMER TRIGGER */
    EVENT_TOTAL
};

typedef enum
{
    FULL_STATE_NO_SIGNAL,
    FULL_STATE_LOCKED,
    FULL_STATE_SCRAMBLE,
    FULL_STATE_NOT_EXIST,
    FULL_STATE_UNSUPPORT,
#if PARENTAL_LOCK_SUPPORT
    FULL_STATE_PARENTAL_LOCK,
#endif
#if DVB2IP_SERVER_SUPPORT
    FULL_STATE_CONNECTING,      /* RIST channel coming up -- neutral "Waiting..." */
#endif
    FULL_STATE_DUMMY,
}FullTip;


typedef struct
{
    uint32_t    mute;
    uint32_t    pause;
    uint32_t    subt;
    uint32_t    ttx;
    uint32_t    rec;
    uint32_t    tms;
    uint32_t    tv;     /* tv- on means tv tv-off means radio */
#define STATE_ON    (1)
#define STATE_OFF   (0)
}EventState;


typedef struct full_screen FullArb;
struct full_screen
{
    EventState      state;
    FullTip         tip;
    void            (*init)(FullArb*);
    void            (*draw[EVENT_TOTAL])(FullArb*);
    void            (*exec[EVENT_TOTAL])(FullArb*);
    void            (*timer_start)(void);
    void            (*timer_stop)(void);
    bool            (*scramble_check)(void);    // true - scramble
    void            (*scramble_set)(uint32_t);  // 1 - scramble  2 - free
};

extern FullArb g_AppFullArb;
extern int app_get_motor_state_in_fullscreen(void);
extern void app_clean_fullscreen_tip(bool osd_trans_hide, bool end_pop_dlg, bool end_info_wnd);

#if RECALL_LIST_SUPPORT
extern int g_recall_count; //20130419
#endif
#endif
