#include "app.h"
#include "app_default_params.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "full_screen.h"
#include "app_windows.h"

#define ZOOM_IMG		"img_zoom_bak"
#define ZOOM_CANVAS     	"canvas_zoom"
#define ZOOM_TITLE_TE     "wnd_zoom_text_title"
#define MOVE_SCALE_X	(APP_THEME_XRES/720)
#define MOVE_SCALE_Y	(APP_THEME_YRES/576)

#define ZOOM_X (85*APP_THEME_XRES/1280)
#define ZOOM_Y (50*APP_THEME_YRES/720)

static char *pMultiple[6] = {"ZOOM  X2","ZOOM  X4","ZOOM  X6","ZOOM  X8","ZOOM  X12","ZOOM  X16"};
static int cnt = 0;
static GuiUpdateRect canvas_rect = {0};
static GxMsgProperty_PlayerVideoWindow play_zoom = {NULL,{0,0,APP_XRES,APP_YRES}};


static int Pos_x = 0;//180*MOVE_SCALE_X;
static int Pos_y = 0;//144*MOVE_SCALE_Y;
static int Width = 0;//360*MOVE_SCALE_X;
static int height = 0;//288*MOVE_SCALE_Y;



static int app_zoom_get_rect_value(int key)
{
	int ret = 0;

	switch(key)
	{
		case STBK_UP:
			/*if((cnt == 0)||(Pos_y<10*MOVE_SCALE_Y)
				||((cnt==2)&&(Pos_y<48*MOVE_SCALE_Y))
				||((cnt==3)&&(Pos_y<30*MOVE_SCALE_Y))
				||((cnt==4)&&(Pos_y<30*MOVE_SCALE_Y))
				||((cnt==5)&&(Pos_y<32*MOVE_SCALE_Y))
				||((cnt==6)&&(Pos_y<48*MOVE_SCALE_Y)))
			*/
			if(Pos_y < 24*MOVE_SCALE_Y)
			{
				ret = 1;
			}
			break;
		case STBK_DOWN:
			/*if((cnt == 0)||((cnt==1)&&(Pos_y>136*MOVE_SCALE_Y))||((cnt==2)&&(Pos_y>=195*MOVE_SCALE_Y))
				||((cnt==3)&&(Pos_y>=210*MOVE_SCALE_Y))||((cnt==4)&&(Pos_y>=225*MOVE_SCALE_Y))
				||((cnt==5)&&(Pos_y>=360*MOVE_SCALE_Y))|((cnt==6)&&(Pos_y>=436*MOVE_SCALE_Y)))
			*/
			if(Pos_y > APP_YRES-play_zoom.rect.height-25*MOVE_SCALE_Y)
			{
				ret = 1;
			}
			break;

		case STBK_LEFT:

			/*if((cnt == 0)||(Pos_x<8*MOVE_SCALE_X)
				||((cnt==2)&&(Pos_x<60*MOVE_SCALE_X))
				||((cnt==3||cnt == 4||cnt==6)&&(Pos_x<60*MOVE_SCALE_X))
				||((cnt==5)&&(Pos_x<60*MOVE_SCALE_X)))
			*/
			if(Pos_x < 34*MOVE_SCALE_X)
			{
				ret = 1;
			}
			break;
		case STBK_RIGHT:
			/*if((cnt == 0)||((cnt==1)&&(Pos_x>170*MOVE_SCALE_X))||((cnt==2)&&(Pos_x>350*MOVE_SCALE_X))
				||((cnt==3)&&(Pos_x>450*MOVE_SCALE_X))||((cnt==4)&&(Pos_x>500*MOVE_SCALE_X))
				||((cnt==5)&&(Pos_x>365*MOVE_SCALE_X))||((cnt==6)&&(Pos_x>405*MOVE_SCALE_X)))
			*/
			if(Pos_x > APP_THEME_XRES-play_zoom.rect.width-35*MOVE_SCALE_X)
			{
				ret = 1;
			}
			break;
		default:
			break;

	}

	return ret;

}

static void app_zoom_canvas_clear(void)
{
	GuiUpdateRect rect = {0};

	rect.x = 0;
	rect.y = 0;
	rect.w = 200;
	rect.h = 150;
	rect.color = "zoom_background";

	GUI_SetProperty(ZOOM_CANVAS, "rectangle",(void*)&rect);
}

static void app_zoom_canvas_draw_rect(uint16_t x, uint16_t y,int cnt)
{
	//GuiUpdateRect rect = {0};
	app_zoom_canvas_clear();
	canvas_rect.x = x;
	canvas_rect.w = 200 - 2*canvas_rect.x;
	canvas_rect.h = 150*canvas_rect.w/200;
	canvas_rect.y = (150-canvas_rect.h)/2;
	canvas_rect.color = "zoom_foreground";

	GUI_SetProperty(ZOOM_CANVAS, "rectangle",(void*)&canvas_rect);
}

static void app_zoom_canvas_move_rect(uint16_t x, uint16_t y,int cnt,int key)
{
	//GuiUpdateRect rect = {0};
	app_zoom_canvas_clear();
	switch(key)
	{
		case STBK_UP:
		case STBK_DOWN:
			canvas_rect.y = y;
			break;

		case STBK_LEFT:
		case STBK_RIGHT:
			canvas_rect.x = x;
			break;
		default:
			break;
	}

	GUI_SetProperty(ZOOM_CANVAS, "rectangle",(void*)&canvas_rect);
}
static void VideoSetZoom(unsigned int x,unsigned int y,unsigned int width,unsigned int height)
{
	//GxMsgProperty_PlayerVideoWindow play_zoom;
	play_zoom.player = PLAYER_FOR_NORMAL;
	play_zoom.rect.x = x;
	play_zoom.rect.width  = APP_THEME_XRES - 2*play_zoom.rect.x;
	play_zoom.rect.height = APP_THEME_YRES*play_zoom.rect.width/APP_THEME_XRES;
	play_zoom.rect.y = (APP_THEME_YRES-play_zoom.rect.height)/2;

	app_send_msg_exec(GXMSG_PLAYER_VIDEO_CLIP, (void *)(&play_zoom));

	return;
}
static void VideoSetZoomMove(unsigned int x,unsigned int y,unsigned int width,unsigned int height)
{
	//GxMsgProperty_PlayerVideoWindow play_zoom;
	play_zoom.player = PLAYER_FOR_NORMAL;
	play_zoom.rect.x = x;
	play_zoom.rect.y = y;

	app_send_msg_exec(GXMSG_PLAYER_VIDEO_CLIP, (void *)(&play_zoom));

	return;
}

void zoom_refresh(void)
{
	app_zoom_canvas_clear();
	GUI_SetProperty(ZOOM_CANVAS, "rectangle",(void*)&canvas_rect);
	GUI_SetProperty("canvas_zoom","draw_now", NULL);
}

SIGNAL_HANDLER int app_zoom_canvas_create(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_zoom_create(const char* widgetname, void *usrdata)
{
    GUI_SetProperty(ZOOM_TITLE_TE, "string","ZOOM");

    Pos_x=0;
    Pos_y=0;
    Width = APP_THEME_XRES;
    height= APP_THEME_YRES;
    GUI_SetInterface("flush", NULL);
    app_zoom_canvas_draw_rect((Pos_x*200/APP_THEME_XRES),(Pos_y*150/APP_THEME_YRES),cnt);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_zoom_destroy(const char* widgetname, void *usrdata)
{
	Pos_x=0;
	Pos_y=0;
	Width = APP_THEME_XRES;
	height= APP_THEME_YRES;

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_zoom_keypress(const char* widgetname, void *usrdata)
{
	int title_display = 0;
	int down_y = 0;
	int right_x = 0;
	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;

	if(event->type == GUI_KEYDOWN)
	{
		switch(event->key.sym)
		{
			case STBK_MENU:
			case STBK_EXIT:
			//case STBK_ZOOM:
			case VK_BOOK_TRIGGER:
				GUI_EndDialog(WND_ZOOM);
				{
				 AppFrontend_LockState lock;
				 app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_LOCK_STATE_GET, &lock);
   				 if(lock == FRONTEND_UNLOCK && g_AppFullArb.tip == FULL_STATE_DUMMY)
   				 {
					 GUI_SetInterface("video_off", NULL);
   					 app_play_current_prog(PLAY_MODE_POINT);
				 }
				}
				VideoSetZoom(0,0,APP_THEME_XRES,APP_THEME_YRES);

				cnt = 0;
				break;

			//case STBK_OK:
			case STBK_ZOOM:
				cnt++;
				cnt = cnt%6;
				if(0==cnt)
				{
					Pos_x=0;
					Pos_y=0;
					Width = APP_THEME_XRES;
					height= APP_THEME_YRES;
					//GUI_EndDialog(WND_ZOOM);
					GUI_SetProperty(ZOOM_TITLE_TE, "string","ZOOM");
				}
				else
				{
					Pos_x=ZOOM_X*(cnt+1)*MOVE_SCALE_X;
					Pos_y=ZOOM_Y*(cnt+1)*MOVE_SCALE_Y;
					Width = APP_THEME_XRES-2*Pos_x;
					height= APP_THEME_YRES-2*Pos_y;

					if(Width <= 0)
					{
						Width=APP_THEME_XRES;
					}

					if(height <= 0)
					{
						height=APP_THEME_YRES;
					}

					title_display = cnt-1;
					if(title_display <=0 )
					{
						title_display = 0;
					}
					GUI_SetProperty(ZOOM_TITLE_TE, "string",pMultiple[title_display]);
				}
				//zoom video
				VideoSetZoom(Pos_x,Pos_y,Width,height);
				//zoom canvas
				app_zoom_canvas_draw_rect((Pos_x*200/APP_THEME_XRES),(Pos_y*150/APP_THEME_YRES),cnt);

				break;

            case STBK_UP:
				if(cnt !=0)
				{
					if(app_zoom_get_rect_value(STBK_UP))
						break;
					Pos_y-=24*MOVE_SCALE_Y;
					if(Pos_y<24)
					{
						Pos_y = 0;
					}
					VideoSetZoomMove(Pos_x,Pos_y,Width,height);
					app_zoom_canvas_move_rect((Pos_x*200/APP_THEME_XRES),(Pos_y*150/APP_THEME_YRES),cnt,STBK_UP);
				}
				zoom_refresh(); /*avoid usb wifi hotplug icon leaving traces*/
				break;
			case STBK_DOWN:
				if(cnt !=0)
				{
					if(app_zoom_get_rect_value(STBK_DOWN))
					{
						down_y = 150 - canvas_rect.h;
						app_zoom_canvas_move_rect(0,down_y,cnt,STBK_DOWN);
					}
					else
					{
						Pos_y+=24*MOVE_SCALE_Y;
						VideoSetZoomMove(Pos_x,Pos_y,Width,height);
						app_zoom_canvas_move_rect((Pos_x*200/APP_THEME_XRES),(Pos_y*150/APP_THEME_YRES),cnt,STBK_DOWN);
					}
				}
				zoom_refresh(); /*avoid usb wifi hotplug icon leaving traces*/
				break;
			case STBK_LEFT:
				if(cnt !=0)
				{
					if(app_zoom_get_rect_value(STBK_LEFT))
						break;
					Pos_x-=34*MOVE_SCALE_X;
					VideoSetZoomMove(Pos_x,Pos_y,Width,height);
					app_zoom_canvas_move_rect((Pos_x*200/APP_THEME_XRES),(Pos_y*150/APP_THEME_YRES),cnt,STBK_LEFT);
				}
				zoom_refresh(); /*avoid usb wifi hotplug icon leaving traces*/
				break;
			case STBK_RIGHT:
				if(cnt !=0)
				{
					if(app_zoom_get_rect_value(STBK_RIGHT))
					{
						right_x= 200 - canvas_rect.w;
						app_zoom_canvas_move_rect(right_x,0,cnt,STBK_RIGHT);
					}
					else
					{
						Pos_x+=34*MOVE_SCALE_X;
						VideoSetZoomMove(Pos_x,Pos_y,Width,height);
						app_zoom_canvas_move_rect((Pos_x*200/APP_THEME_XRES),(Pos_y*150/APP_THEME_YRES),cnt,STBK_RIGHT);
					}
				}
				break;
			default:
				break;
		}
	}

	return EVENT_TRANSFER_KEEPON;
}

void app_zoom_module_sys_init(void)
{
	Pos_x = 180*MOVE_SCALE_X;
	Pos_y = 144*MOVE_SCALE_Y;
	Width = 360*MOVE_SCALE_X;
	height = 288*MOVE_SCALE_Y;

	play_zoom.rect.width  = APP_THEME_XRES;
	play_zoom.rect.height = APP_THEME_YRES;
}

