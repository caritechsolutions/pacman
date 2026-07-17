#include "app.h"
#include "app_windows.h"
#include "app_wnd_system_setting_opt.h"
#if (GAME_ENABLE > 0)
#define TXT_SNAKE_LEVEL "txt_snake_level_value"
#define TXT_SNAKE_SCORE "txt_snake_score_value"
#define TXT_SNAKE_OK "txt_snake_ok_info"
#define BTN_SNAKE_STATUS "btn_snake_status"
#define CANVAS_SNAKE "canvas_snake"
#define IMG_SNAKE_APPLE "GX6102_G_S_APPLE"
#define IMG_SNAKE_BODY "GX6102_G_S_BODY"
#define IMG_SNAKE_HEAD "GX6102_G_S_HEAD"

#define CANVAS_WIDTH 448
#define CANVAS_HEIGHT 464
#define BODYCNT 5
#define SNAKE_HEAD_WIDTH 16
#define SNAKE_HEAD_HEIGHT 16

#define MOVE_RIGHT		448
#define MOVE_LEFT	0
#define MOVE_BOTTOM	464
#define MOVE_TOP	0
#define SNAKE_STEP  16
#define INIT_TIMER_VALUE 500


typedef enum
{
	Left,
	Right,
	Up,
	Down,
}Direct;

typedef struct
{
	int32_t x;
	int32_t y;
}SPoint;


typedef enum
{
	GAMEQUIT,
	GAMESTART,
	GAMEPAUSE,
	GAMEOVER
}SnakeStatus;

static SPoint g_food;
static SPoint g_snakeBody[20];
static SnakeStatus status = GAMEQUIT;

/* Global Variables --------------------------------------------------------- */
static uint16_t g_SnakeScore = 0;    //所得分数
static uint8_t g_SnakeGrade  = 0;    //游戏等级

static uint8_t g_bodyCnt = BODYCNT;  //蛇身长度
static uint8_t g_eDirect = Right;    //行进方向

static event_list* snakeTimer = NULL;
static int app_game_snake_timer_handle(void *userdata);



static  void app_game_snake_number2string(char *txtId, uint32_t number)
{
#define STRLEN 20

	char str[STRLEN] = {0};
	sprintf(str,"%d",number);
	GUI_SetProperty(txtId, "string",(void*)str);
}

static void app_snake_body_init(void)
{
	uint8_t	i = 0;

	g_bodyCnt = BODYCNT;
	g_eDirect = Right;
	for(i = 0; i < BODYCNT; i++)
	{
		g_snakeBody[i].x = MOVE_LEFT+ (g_bodyCnt-1-i)*SNAKE_STEP;
		g_snakeBody[i].y = MOVE_TOP;
	}

}

static void app_snake_game_draw(char *path,uint32_t x,uint32_t y)
{
	GuiUpdateImage img = {0};

	img.x = x;
	img.y = y;
	img.name = path;

	GUI_SetProperty(CANVAS_SNAKE, "image",(void*)&img);
}


static uint8_t app_snake_food_pos_valid(uint16_t x ,uint16_t y)
{
	int i = 0;

	for(i=0; i<g_bodyCnt; i++)
	{
		if((x == g_snakeBody[i].x )|| (y == g_snakeBody[i].y))
			return FALSE;
	}
	return TRUE;
}

static void app_snake_food_draw(void)
{
	uint16_t CurX = 0;
	uint16_t CurY =0;

	while(1)
	{
		CurX = MOVE_LEFT+((rand()%(MOVE_RIGHT - MOVE_LEFT))/SNAKE_STEP)*SNAKE_STEP;
		CurY = MOVE_TOP+((rand()%(MOVE_BOTTOM- MOVE_TOP))/SNAKE_STEP)*SNAKE_STEP;
		if(TRUE == app_snake_food_pos_valid(CurX,CurY))
			break;
	}

	g_food.x = CurX;
	g_food.y = CurY ;
	app_snake_game_draw(IMG_SNAKE_APPLE,CurX,CurY);

}

static void app_snake_one_cell_draw(unsigned short x, unsigned short y, uint8_t isBody)
{
	if(FALSE == isBody)
	{
		app_snake_game_draw(IMG_SNAKE_BODY,x,y);
	}
	else
	{
		app_snake_game_draw(IMG_SNAKE_BODY,x,y);
	}
}


static void app_snake_clear(uint16_t x, uint16_t y,uint16_t width, uint16_t height)
{
	GuiUpdateRect rect = {0};

	rect.x = x;
	rect.y = y;
	rect.w = width;
	rect.h = height;
	rect.color = "Snake_BG";

	GUI_SetProperty(CANVAS_SNAKE, "rectangle",(void*)&rect);
}


static void app_snake_init(void)
{
	uint8_t i = 0;

	for(i=0; i<BODYCNT; i++)//绘制初始蛇身
	{
		app_snake_one_cell_draw(g_snakeBody[i].x , g_snakeBody[i].y, i);
	}
	app_snake_food_draw();//绘制食物
}


static void app_snake_game_over(void)
{
	app_snake_clear(0,0,CANVAS_WIDTH,CANVAS_HEIGHT);
	app_snake_body_init();
	app_snake_init();

	if(NULL != snakeTimer)
	{
		timer_stop(snakeTimer);
		remove_timer(snakeTimer);
		snakeTimer = NULL;
		status = GAMEQUIT;
	}
}

static void app_game_snake_set_andvance(void)
{
	int interval_timer = INIT_TIMER_VALUE;
	interval_timer = interval_timer/(g_SnakeGrade+1);
	if(NULL != snakeTimer)
	{
		remove_timer(snakeTimer);
		snakeTimer = create_timer(app_game_snake_timer_handle, interval_timer, NULL, TIMER_REPEAT);
	}
}

static void app_snake_advance(void)
{
	app_snake_clear(0,0,CANVAS_WIDTH,CANVAS_HEIGHT);
	app_snake_body_init();
	app_snake_init();

	g_SnakeGrade++;
	app_game_snake_set_andvance();

	if(g_SnakeGrade == 9)
	{
		g_SnakeGrade = 0;
	}
	app_game_snake_number2string(TXT_SNAKE_LEVEL,g_SnakeGrade);
}



static void app_snake_game_process(void)
{
	uint8_t i;
	//调整蛇身每个方块坐标
	//(蛇尾的坐标保存在g_bodyCnt上)

	for(i=g_bodyCnt; i>0; i--)
	{
		g_snakeBody[i].x = g_snakeBody[i-1].x;
		g_snakeBody[i].y = g_snakeBody[i-1].y;
	}

	//根据方向判断步径的增加
	switch(g_eDirect)
	{
		case Left:
			g_snakeBody[0].x -= SNAKE_STEP;
			break;
		case Right:
			g_snakeBody[0].x += SNAKE_STEP;
			break;
		case Up:
			g_snakeBody[0].y -= SNAKE_STEP;
			break;
		case Down:
			g_snakeBody[0].y += SNAKE_STEP;
			break;
		default:
			break;
	}

	//是否超出边界判断
	if(g_snakeBody[0].x < (short)MOVE_LEFT //< 0           //左超出
		||(short) MOVE_RIGHT - g_snakeBody[0].x < 1        //右超出
		||g_snakeBody[0].y < (short)MOVE_TOP //< 0         //上超出
		||(short)MOVE_BOTTOM - g_snakeBody[0].y < 1)       //下超出
	{
		status = GAMEOVER;
		return;
	}

	//检查蛇身内部有无自己碰撞的情况
	for(i=1; i<g_bodyCnt; i++)
	{
		if(g_snakeBody[0].x == g_snakeBody[i].x &&
			g_snakeBody[0].y == g_snakeBody[i].y)
		{
			status = GAMEOVER;
			return;
		}
	}

	//吃到食物
	if(g_snakeBody[0].x == g_food.x  &&  g_snakeBody[0].y == g_food.y)
	{
		g_bodyCnt++;
		if(g_bodyCnt>=20)	//最多20 格子,多则升级
			app_snake_advance();
		else
			app_snake_food_draw();	//刷新食物格子
		g_SnakeScore++;
		app_game_snake_number2string(TXT_SNAKE_SCORE,g_SnakeScore);
	}
	else
	{
		app_snake_clear(g_snakeBody[g_bodyCnt].x,  g_snakeBody[g_bodyCnt].y,SNAKE_HEAD_WIDTH,SNAKE_HEAD_HEIGHT);	//擦除蛇尾方格
	}

	//蛇移动的处理,移动蛇头
	app_snake_one_cell_draw(g_snakeBody[0].x, g_snakeBody[0].y, 0);

	if(g_bodyCnt>1)
		app_snake_one_cell_draw(g_snakeBody[1].x, g_snakeBody[1].y, 1);
}


static int app_game_snake_timer_handle(void *userdata)
{
	if(GAMEOVER == status)
	{
		GUI_SetProperty(BTN_SNAKE_STATUS, "string",(void*)STR_ID_GAMEOVER);
		GUI_SetProperty(TXT_SNAKE_OK, "string",(void*)STR_ID_START);
		app_snake_game_over();
	}
	else if(GAMESTART == status)
	{
		app_snake_game_process();
	}else{}

	return 0;
}

void app_game_snake_create(void)
{
    GUI_CreateDialog(WND_SNAKE);
	GUI_SetInterface("flush",NULL);
    app_snake_clear(0,0,CANVAS_WIDTH,CANVAS_HEIGHT);
	app_snake_init();
}


/* Export functions ------------------------------------------------------ */
SIGNAL_HANDLER int app_snake_create(const char* widgetname, void *usrdata)
{
	status = GAMEQUIT;
	//g_SnakeGrade = 0;
	g_SnakeScore = 0;
	memset(&g_food,0,sizeof(g_food));
	memset(&g_snakeBody,0,sizeof(g_snakeBody));
	app_game_snake_number2string(TXT_SNAKE_LEVEL,g_SnakeGrade);
	app_game_snake_number2string(TXT_SNAKE_SCORE,g_SnakeScore);
	GUI_SetProperty(BTN_SNAKE_STATUS, "string",(void*)STR_ID_READY);
	GUI_SetProperty(TXT_SNAKE_OK, "string",(void*)STR_ID_START);
	app_snake_body_init();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_snake_destroy(const char* widgetname, void *usrdata)
{
	if (snakeTimer != NULL)
	{
		remove_timer(snakeTimer);
		snakeTimer = NULL;
		status = GAMEQUIT;
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_snake_lost_focus(GuiWidget *widget, void *usrdata)
{
	status = GAMEPAUSE;
	GUI_SetProperty(BTN_SNAKE_STATUS, "string",(void*)STR_ID_PAUSE);
	GUI_SetProperty(TXT_SNAKE_OK, "string",(void*)STR_ID_START);
	timer_stop(snakeTimer);
	return EVENT_TRANSFER_STOP;
}

int app_snake_get_focus(void)
{
	int i=0;
		GUI_SetProperty(BTN_SNAKE_STATUS, "string",(void*)STR_ID_PAUSE);
		GUI_SetProperty(TXT_SNAKE_OK, "string",(void*)STR_ID_START);

		GUI_SetInterface("flush",NULL);

		app_snake_clear(0,0,CANVAS_WIDTH,CANVAS_HEIGHT);
		app_snake_game_draw(IMG_SNAKE_APPLE,g_food.x,g_food.y);
		for(i=0; i<g_bodyCnt; i++)//绘制初始蛇身
		{
			app_snake_one_cell_draw(g_snakeBody[i].x , g_snakeBody[i].y, i);
		}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_snake_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case VK_BOOK_TRIGGER:
				GUI_EndDialog("after wnd_full_screen");
				GUI_SetProperty(WND_SNAKE,"draw_now",NULL);
				break;

			case STBK_MENU:
			case STBK_EXIT:
				status = GAMEQUIT;
				timer_stop(snakeTimer);
				remove_timer(snakeTimer);
				snakeTimer = NULL;
				GUI_EndDialog(WND_SNAKE);
				break;
			/*case STBK_PAUSE:
				status = GAMEPAUSE;
				GUI_SetProperty(BTN_SNAKE_STATUS, "string",(void*)STR_ID_PAUSE);
				timer_stop(snakeTimer);
				break;*/
			case STBK_OK:
				if(GAMEQUIT == status)
				{
					int interval_timer = INIT_TIMER_VALUE;
					status = GAMESTART;
					//g_SnakeGrade = 0;
					g_SnakeScore = 0;

					interval_timer = interval_timer/(g_SnakeGrade+1);
					snakeTimer = create_timer(app_game_snake_timer_handle, interval_timer, NULL, TIMER_REPEAT);

					app_game_snake_number2string(TXT_SNAKE_LEVEL,g_SnakeGrade);
					app_game_snake_number2string(TXT_SNAKE_SCORE,g_SnakeScore);
					GUI_SetProperty(BTN_SNAKE_STATUS, "string",(void*)STR_ID_START);
					GUI_SetProperty(TXT_SNAKE_OK, "string",(void*)STR_ID_PAUSE);
				}
				else if(GAMEPAUSE == status)
				{
					status = GAMESTART;
					reset_timer(snakeTimer);
					GUI_SetProperty(BTN_SNAKE_STATUS, "string",(void*)STR_ID_START);
					GUI_SetProperty(TXT_SNAKE_OK, "string",(void*)STR_ID_PAUSE);
				}
				else if(GAMESTART == status)
				{
					status = GAMEPAUSE;
					GUI_SetProperty(BTN_SNAKE_STATUS, "string",(void*)STR_ID_PAUSE);
					GUI_SetProperty(TXT_SNAKE_OK, "string",(void*)STR_ID_START);
					timer_stop(snakeTimer);
				}
				break;
			case STBK_0:
				g_SnakeGrade = 0;
				app_game_snake_set_andvance();
				app_game_snake_number2string(TXT_SNAKE_LEVEL,g_SnakeGrade);
				break;
			case STBK_1:
				g_SnakeGrade = 1;
				app_game_snake_set_andvance();
				app_game_snake_number2string(TXT_SNAKE_LEVEL,g_SnakeGrade);
				break;
			case STBK_2:
				g_SnakeGrade = 2;
				app_game_snake_set_andvance();
				app_game_snake_number2string(TXT_SNAKE_LEVEL,g_SnakeGrade);
				break;
			case STBK_3:
				g_SnakeGrade = 3;
				app_game_snake_set_andvance();
				app_game_snake_number2string(TXT_SNAKE_LEVEL,g_SnakeGrade);
				break;
			case STBK_4:
				g_SnakeGrade = 4;
				app_game_snake_set_andvance();
				app_game_snake_number2string(TXT_SNAKE_LEVEL,g_SnakeGrade);
				break;
			case STBK_5:
				g_SnakeGrade = 5;
				app_game_snake_set_andvance();
				app_game_snake_number2string(TXT_SNAKE_LEVEL,g_SnakeGrade);
				break;
			case STBK_6:
				g_SnakeGrade = 6;
				app_game_snake_set_andvance();
				app_game_snake_number2string(TXT_SNAKE_LEVEL,g_SnakeGrade);
				break;
			case STBK_7:
				g_SnakeGrade = 7;
				app_game_snake_set_andvance();
				app_game_snake_number2string(TXT_SNAKE_LEVEL,g_SnakeGrade);
				break;
			case STBK_8:
				g_SnakeGrade = 8;
				app_game_snake_set_andvance();
				app_game_snake_number2string(TXT_SNAKE_LEVEL,g_SnakeGrade);
				break;
			case STBK_9:
				g_SnakeGrade = 9;
				app_game_snake_set_andvance();
				app_game_snake_number2string(TXT_SNAKE_LEVEL,g_SnakeGrade);
				break;
			case STBK_UP:
				if(g_eDirect == Down
					|| status != GAMESTART)
					break;
				g_eDirect = Up;
				break;
			case STBK_DOWN:
				if(g_eDirect == Up
					|| status != GAMESTART)
					break;
				g_eDirect = Down;
				break;
			case STBK_LEFT:
				if(g_eDirect == Right
					|| status != GAMESTART)
					break;
				g_eDirect = Left;
				break;
			case STBK_RIGHT:
				if(g_eDirect == Left
					|| status != GAMESTART)
					break;
				g_eDirect = Right;
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_snake_canvas_create(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
#endif
