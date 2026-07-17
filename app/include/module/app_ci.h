/*
 * =====================================================================================
 *
 *       Filename:  app_ci.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2010年12月21日 08时39分18秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  shenbin
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#ifndef __APP_CI__
#define __APP_CI__

#include "gxcore.h"
//#include "cim_includes.h"

// cim_platform _CIM_MODULE_CNT_ (1)
#define SOLT_TOTAL  (1)

typedef enum
{
    ENTRY_NO_CARD,
    ENTRY_INITIALIZING,
    ENTRY_ESTABLISHED, // state confirm, state equal or larger than this, means card ok
    ENTRY_WAIT,
    ENTRY_TIMEOUT
}entry_state;

typedef enum
{
    CI_MMI_EXIT = 0x0,
    CI_MMI_MENU = 0x9,
    CI_MMI_LIST = 0xC,
    CI_MMI_ENQ = 0x7,
    CI_MMI_DELAY = 0x1
}mmi_state;

typedef enum
{
    CI_SLOT_0,
    CI_SLOT_1,
    CI_SLOT_2,
    CI_SLOT_3,
}ci_slot;

typedef enum
{
    CI_INDEX_ENQUIRY= 0,
    CI_INDEX_TITLE = 0,
    CI_INDEX_SUBTITLE,
    CI_INDEX_BOTTOM,
    CI_INDEX_ENTRIES,
}string_index;

#define MAX_CIMENU_ENTRIES  255//(20)

typedef struct
{
    char *title_text;
    char *subtitle_text;
    char *bottom_text;
    char *entries_text[MAX_CIMENU_ENTRIES];
    int entries_num;

    status_t (*selectable)(void);
    status_t (*select)(int);  // int from 0 to MAX_CIMENU_ENTRIES-1
    status_t (*cancel)(void);
    status_t (*abort)(void);
}ci_menu;

typedef struct
{
    char *text;
    bool blind;
    int expected_length;

    status_t (*reply)(const char*, int);  // param1: answer data, param2: answer data length
    status_t (*cancel)(void);
    status_t (*abort)(void);
}ci_enquiry;

typedef struct
{
    ci_slot slot;
    entry_state state;
}slot_entry;

typedef struct
{
    ci_slot slot;                   // the solt witch in operation

    ci_menu* (*get_menu)(void);         // use return value ci_menu*
    ci_enquiry* (*get_enquiry)(void);   // use return value ci_equiry*

    const char* (*get_name)(ci_slot);

    status_t (*mmi_open)(ci_slot);          // when finish open, set slot value, ci_menu & ci_enquiry base this slot
    status_t (*mmi_close)(ci_slot);

    slot_entry* (*cim_state_change)(void);   // check change and get state, put into timer, return NULL means not change, !NULL can get the state

    mmi_state* (*mmi_mode_change)(void);    // check change and get mode, put into timer, return NULL means not change, !NULL can get the mode

    void (*update_pmt)(ci_slot, const char*,int);    // param2: pmt section data

    void (*init)(void);         // thread create
}ci_ops;

extern ci_ops ci;

#endif
