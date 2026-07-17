#include "gui_core.h"
#include "app_main_menu.h"
#ifndef __APP_MAIN_MENU_STYLE_H__
#define __APP_MAIN_MENU_STYLE_H__

typedef int (*mainmenu_style_int_callback)(int key);
typedef int (*mainmenu_style_void_callback)(void);

typedef struct {
    const char* focus_image;
    const char* unfocus_image;
    bool state;
    uint8_t up;
    uint8_t down;
    uint8_t left;
    uint8_t right;
}MainmenuStyleItem;

typedef struct {
    mainmenu_style_void_callback create;
    mainmenu_style_void_callback destroy;
    mainmenu_style_void_callback got_focus;
    mainmenu_style_void_callback lost_focus;
    mainmenu_style_int_callback keypress;
    mainmenu_style_void_callback media_entry;
} MainmenuStyleOps;

extern void MainMenuStyleItem_Init(void);
void app_mainmenu_style_register(MainmenuStyleOps* style);
int app_mainmenu_style_create(void);
int app_mainmenu_style_destroy(void);
int app_mainmenu_style_got_focus(void);
int app_mainmenu_style_lost_focus(void);
int app_mainmenu_style_keypress(int key);
int app_mainmenu_style_media_entry(void);
#endif

