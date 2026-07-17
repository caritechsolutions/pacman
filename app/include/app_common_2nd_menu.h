#ifndef __APP_COMMON_2ND_MENU_H__
#define __APP_COMMON_2ND_MENU_H__

#include <gxtype.h>

#define LISTVIEW_COMMON2ND_MENU "wnd_common_2nd_menu_listview"
#define TEXT_COMMON_2ND_TITLE   "text_common_2nd_title"
#define COMMON_2ND_MAX_COUNT    (50)

typedef struct
{
    char *title;
    void (*create)(void);
    unsigned int attr;
}Common2ndEntry;

extern Common2ndEntry g_Common2ndEntry[COMMON_2ND_MAX_COUNT];
extern int g_Common2ndEntryCount;

#endif
