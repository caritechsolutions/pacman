#include "app.h"
#include "pmp_spectrum.h"
#include "stdlib.h"
#include "gui_core.h"

#define COLOR_BLOCK			"spectrum_block"
#define COLOR_TOP			"spectrum_top"
#define COLOR_CLEAR			"spectrum_clear"


#define CHANNEL_NUM			30
#define CHANNEL_BLOCK_NUM		40

#define BLOCK_INTERVAL_W		2
#define BLOCK_INTERVAL_H		2


typedef struct SpectrumView
{
	char* widget_canvas;
	event_list* timer;

	int x0;
	int y0;
	int width;
	int height;

	char* color_block;
	char* color_top;
	char* color_clear;

	int CurrentVolume[CHANNEL_NUM];
	int VolumeBak[CHANNEL_NUM];
	int TopIndexBak[CHANNEL_NUM];
}SpectrumView;


struct SpectrumView  SpectrumViewData;

static int FlagForTimer = 0;
static bool FlagForStop = false;

static void SpectrumViewRrawRec(GuiUpdateRect *rect)
{
	GUI_SetProperty(SpectrumViewData.widget_canvas, "rectangle", rect);
}



 int ClearSpectrum(void)/*频谱*/
{
	GuiUpdateRect Rect;


	Rect.x=SpectrumViewData.x0;/*一个矩形单元*/
	Rect.y= SpectrumViewData.y0-SpectrumViewData.height+1;
	Rect.w=SpectrumViewData.width;
	Rect.h=SpectrumViewData.height;
	Rect.color = SpectrumViewData.color_clear;
	SpectrumViewRrawRec(&Rect);

	return 0;
}
 void showSpectrumView(void)//avoid no any show in screen.
 {
	 GUI_SetProperty(SpectrumViewData.widget_canvas, "state", "show");
 }

void spectrum_volumebak_clear(void)
{
    memset(SpectrumViewData.VolumeBak, 0, sizeof(int) * CHANNEL_NUM);
    memset(SpectrumViewData.TopIndexBak, 0, sizeof(int) * CHANNEL_NUM);

    return;
}


 int ShowLevelFlag(int Channel)/*频谱*/
{
	GuiUpdateRect Rect;
	int i = 0;
	int StartX = 0;
	int StartY = 0;
	int RectPerWidth =0;
	int RectPerHeight=0;
	int BakYIndex =0;
	int FlagYIndex =0;

	if(Channel >= CHANNEL_NUM)	return 1;


		StartX = SpectrumViewData.x0+Channel * (SpectrumViewData.width / CHANNEL_NUM);
		StartY = SpectrumViewData.y0;


		RectPerWidth = SpectrumViewData.width / CHANNEL_NUM;
		RectPerHeight=(SpectrumViewData.height /(CHANNEL_BLOCK_NUM + 1))&(~0x1);


		BakYIndex = CHANNEL_BLOCK_NUM * SpectrumViewData.VolumeBak[Channel] /100;
		FlagYIndex =CHANNEL_BLOCK_NUM * SpectrumViewData.CurrentVolume[Channel] /100;

		Rect.x=StartX;/*一个矩形单元*/
		Rect.w=RectPerWidth - BLOCK_INTERVAL_W;

		/*clear toplevel*/
		Rect.y= StartY - SpectrumViewData.TopIndexBak[Channel]*RectPerHeight;
		Rect.h=RectPerHeight;
		Rect.color = SpectrumViewData.color_clear;
		SpectrumViewRrawRec(&Rect);

		/*+*/
		if(FlagYIndex > BakYIndex)
		{
			for(i = BakYIndex + 1; i <= FlagYIndex; i++)
			{
				Rect.y= StartY - i*RectPerHeight;
				Rect.h=RectPerHeight - BLOCK_INTERVAL_H;
				//Rect.h=RectPerHeight/2;
				Rect.color = SpectrumViewData.color_block;
				SpectrumViewRrawRec(&Rect);
			}
		}
		/*-*/
		else if(FlagYIndex <= BakYIndex)
		{
			Rect.y = StartY - SpectrumViewData.height + 1;
			Rect.h= SpectrumViewData.height - 1 - FlagYIndex * RectPerHeight;
			//Rect.h=RectPerHeight/2;
			Rect.color = SpectrumViewData.color_clear;
			SpectrumViewRrawRec(&Rect);
		}


		SpectrumViewData.VolumeBak[Channel] = SpectrumViewData.CurrentVolume[Channel];


		/*TopLevel*/
        SpectrumViewData.TopIndexBak[Channel] = FlagYIndex + 1;
        Rect.y= StartY - SpectrumViewData.TopIndexBak[Channel]*RectPerHeight;
        Rect.h=RectPerHeight/2;
        Rect.color = SpectrumViewData.color_top;
        if(false == FlagForStop)
        {
            SpectrumViewRrawRec(&Rect);
        }

	return 0;
}

static  int SpectrumOnTimer(void *userdata)
{
	int i ,randdata;

/*
	char* focus_win = NULL;
	focus_win = (char*)GUI_GetFocusWindow();
	if(NULL == focus_win) return 1;
	if(strcasecmp(focus_win, WIN_MUSIC_VIEW))
	{
		return 1;
	}
*/
	if(FlagForTimer == 1)
	{
		FlagForTimer = 0;
		GUI_SetProperty(SpectrumViewData.widget_canvas, "state", "show");
        return 0;
	}

	for( i = 0; i < CHANNEL_NUM; i++)
	{
		if(i<5)
		{
			randdata=20 + rand()%70;
		}
		else if(i>=5&&i<=10)
		{
			randdata=10 + rand()%60;
		}
		else
		{
			randdata=10 + rand()%40;
		}

		SpectrumViewData.CurrentVolume[i] = randdata;

		ShowLevelFlag(i);
	}

	return 0;
}


int spectrum_create(char* widget_canvas, int x0,int y0,int width,int height)
{
	app_log_info("[SPECTRUM] spectrum_create\n");

	if(SpectrumViewData.widget_canvas)
	{
		app_log_error("[SPECTRUM] spectrum is exist\n");
		return 1;
	}

	APP_CHECK_P(widget_canvas, 1);
	APP_FREE(SpectrumViewData.widget_canvas);
	SpectrumViewData.widget_canvas = GxCore_Strdup(widget_canvas);
	APP_CHECK_P(SpectrumViewData.widget_canvas, 1);

	SpectrumViewData.x0=x0/2*2;
	SpectrumViewData.y0=y0/2*2;
	SpectrumViewData.width=width/2*2;
	SpectrumViewData.height=height/2*2;
	SpectrumViewData.y0 += SpectrumViewData.height -1;;

	int i = 0;
	for( i = 0; i < CHANNEL_NUM; i++)
	{
		SpectrumViewData.CurrentVolume[i] = 0;
		SpectrumViewData.VolumeBak[i] = 0;
		SpectrumViewData.TopIndexBak[i] = 0;
	}

	//TODO: from canver property
	SpectrumViewData.color_block = COLOR_BLOCK;//0x16a2c6;
	SpectrumViewData.color_top = COLOR_TOP;//0xa8ca08;
	SpectrumViewData.color_clear = COLOR_CLEAR;//0x0;

	return 0;
}

int spectrum_destroy(void)
{
	app_log_info("[SPECTRUM] spectrum_destroy\n");
	GUI_SetProperty(SpectrumViewData.widget_canvas, "state", "hide");
	GUI_SetInterface("flush",NULL);
	ClearSpectrum();



	APP_TIMER_REMOVE(SpectrumViewData.timer);

	APP_FREE(SpectrumViewData.widget_canvas);

	return 0;
}

int spectrum_start(void)
{
	app_log_info("[SPECTRUM] spectrum_start\n");
	FlagForTimer = 1;
    FlagForStop = false;
	APP_TIMER_ADD(SpectrumViewData.timer, SpectrumOnTimer, 100, TIMER_REPEAT);

	return 0;
}

int spectrum_stop(void)
{
	app_log_info("[SPECTRUM] spectrum_stop\n");

	FlagForStop = true;
	if(SpectrumViewData.timer)
	{
		timer_stop(SpectrumViewData.timer);
	}

	int i = 0;

	for( i = 0; i < CHANNEL_NUM; i++)
	{
		SpectrumViewData.CurrentVolume[i] = 0;
		SpectrumViewData.VolumeBak[i] = 0;
		SpectrumViewData.TopIndexBak[i] = 0;
		ShowLevelFlag(i);
	}

	return 0;
}

int spectrum_pause(void)
{
	app_log_info("[SPECTRUM] spectrum_pause\n");

	if(SpectrumViewData.timer)
	{
		timer_stop(SpectrumViewData.timer);
	}

	return 0;
}

int spectrum_resume(void)
{
	app_log_info("[SPECTRUM] spectrum_resume\n");

	int i = 0;
	for( i = 0; i < CHANNEL_NUM; i++)
	{
		SpectrumViewData.VolumeBak[i] = 0;
	}
	FlagForTimer = 1;
	APP_TIMER_ADD(SpectrumViewData.timer, SpectrumOnTimer, 100, TIMER_REPEAT);

	return 0;
}

int spectrum_redraw(void)
{
	app_log_info("[SPECTRUM] spectrum_redraw\n");

	int i = 0;

	if(NULL == SpectrumViewData.widget_canvas)
		return 1;

	for( i = 0; i < CHANNEL_NUM; i++)
	{
		SpectrumViewData.VolumeBak[i] = 0;
	}

	for( i = 0; i < CHANNEL_NUM; i++)
	{
		ShowLevelFlag(i);
	}

	return 0;
}

bool spectrum_get_FlagForStop(void)
{
    return FlagForStop;
}

