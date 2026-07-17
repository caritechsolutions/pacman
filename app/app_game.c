#include "app_config.h"

#if (GAME_ENABLE > 0)
#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "module/config/gxconfig.h"
#include "app_utility.h"


enum GAME_ITEM_NAME
{
	ITEM_GAME_SNAKE = 0,
	ITEM_GAME_TERTRIS,
	ITEM_GAME_TOTAL
};

static SystemSettingOpt s_game_opt;
static SystemSettingItem s_game_item[ITEM_GAME_TOTAL];

void app_game_tetris_create(void);
void app_game_snake_create(void);

static int app_snake_button_press_callback(unsigned short key)
{
	if(key == STBK_OK)
	{
        app_game_snake_create();
	}
	return EVENT_TRANSFER_KEEPON;
}

static int app_tertris_button_press_callback(unsigned short key)
{
	if(key == STBK_OK)
	{
        app_game_tetris_create();
	}
	return EVENT_TRANSFER_KEEPON;
}

static int app_game_exit_callback(ExitType exit_type)
{
    return EXIT_RET_DEFAULT;
}

static void app_snake_item_init(void)
{
	s_game_item[ITEM_GAME_SNAKE].itemTitle = STR_ID_SNAKE;
	s_game_item[ITEM_GAME_SNAKE].itemType = ITEM_PUSH;
	s_game_item[ITEM_GAME_SNAKE].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
	s_game_item[ITEM_GAME_SNAKE].itemCallback.btnCallback.BtnPress= app_snake_button_press_callback;
	s_game_item[ITEM_GAME_SNAKE].itemStatus = ITEM_NORMAL;
}

static void app_tertris_item_init(void)
{
	s_game_item[ITEM_GAME_TERTRIS].itemTitle = STR_ID_TETRIS;
	s_game_item[ITEM_GAME_TERTRIS].itemType = ITEM_PUSH;
	s_game_item[ITEM_GAME_TERTRIS].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
	s_game_item[ITEM_GAME_TERTRIS].itemCallback.btnCallback.BtnPress= app_tertris_button_press_callback;
	s_game_item[ITEM_GAME_TERTRIS].itemStatus = ITEM_NORMAL;
}

void app_game_menu_exec(void)
{
    memset(&s_game_opt, 0 ,sizeof(s_game_opt));

    s_game_opt.menuTitle = STR_ID_GAME_CENTER;
    s_game_opt.titleImage = "s_title_left_utility";
    s_game_opt.itemNum = ITEM_GAME_TOTAL;
    s_game_opt.timeDisplay = TIP_HIDE;
    s_game_opt.item = s_game_item;

    app_snake_item_init();
    app_tertris_item_init();

    s_game_opt.exit = app_game_exit_callback;

    app_system_set_create(&s_game_opt);
}
#endif
