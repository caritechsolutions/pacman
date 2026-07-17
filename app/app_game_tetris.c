/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2008, All right reserved
******************************************************************************

******************************************************************************
* File Name :	ui_games_sub.c
* Author    : 	hulj
* Project   :	GX6102_DAWorld
* Type      :	APP
******************************************************************************
* Purpose   :	模块实现文件
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2008/03/27        hulj           creation
   2.5		2009/07/07		  lishj			 modify
*****************************************************************************/
#include "app.h"
#include "app_wnd_system_setting_opt.h"
#if (GAME_ENABLE > 0)
#define CANVAS_TETRIS_MAIN "canvas_main_tetris"
#define CANVAS_TETRIS_LITTER "canvas_little_tetris"
#define TXT_TETRIS_LEVEL "txt_tetris_level_value"
#define TXT_TETRIS_SCORE "txt_tetris_score_value"
#define TXT_TETRIS_LINE "txt_tetris_line_value"
#define TXT_TETRIS_OK "txt_tetris_ok_info"
#define BOX_TETRIS_STATUS "box_tetris_info"
#define BTN_TETRIS_STATUS "btn_tetris_status"

#define IMG_TETRIS_DIAMOND_BULE "GX6102_G_T_BLUE"
#define IMG_TETRIS_DIAMOND_BROWN "GX6102_G_T_BROWN"
#define IMG_TETRIS_DIAMOND_GREEN "GX6102_G_T_GREEN"
#define IMG_TETRIS_DIAMOND_ORANGE "GX6102_G_T_ORANGE"
#define IMG_TETRIS_DIAMOND_PINK "GX6102_G_T_PINK"
#define IMG_TETRIS_DIAMOND_RED "GX6102_G_T_RED"
#define IMG_TETRIS_DIAMOND_YELLOW "GX6102_G_T_YELLOW"

#define CANVAS_WIDTH_MAIN 420
#define CANVAS_HEIGHT_MAIN 460
#define STRLEN 20
#define INIT_TIMER_VALUE 500

#define OFFSET_X 20
#define OFFSET_Y 20


#define CANVAS_WIDTH_LITTLE 100
#define CANVAS_HEIGHT_LITTLE 100

/* Private Constants --------------------------------------------------------*/
#define CellSize	 20		//最小方块单元大小
#define VCellNo		 23		//垂直方向方块数
#define HCellNo		 23		//水平方向方块数



typedef enum
{
	GAMEQUIT,
	GAMESTART,
	GAMEPAUSE,
	GAMEOVER
}TetrisStatus;

/* Private Variables (static)------------------------------------------------*/
static uint16_t RussionGrade=0;		//当前级别
static uint32_t totalScore = 0;

char Top;						//离顶部的距离，逻辑坐标
static uint16_t Sel;						//标记7种方块中的哪一种(后续)
static uint16_t SelInMove;						//标记7种方块中的哪一种 (当前)
static uint16_t Flag;						//标记方块的旋转次数
static uint16_t org[4][2];					//原始方块矩阵
static uint16_t block[4][2];				//当前方块矩阵
static uint16_t blockN[4][2];				//后续方块矩阵
uint16_t RussionCurScoreline = 0;			//当前得分
static uint16_t g_ShareBuffer0[HCellNo][VCellNo];		// 逻辑坐标系矩阵
static TetrisStatus gameStatus = GAMEQUIT;

static event_list* tetrisTimer = NULL;
static int app_game_tetris_timer_handle(void *userdata);


/*******************************************************\
*				初始化数据								*
\*******************************************************/
static void app_game_russion_Datainit(void)
{
	uint32_t i,j;

	Top = VCellNo-1;
	RussionCurScoreline = 0;
	totalScore = 0;
	//RussionGrade = 0;

	/* 将逻辑坐标系的第一列和最后一列置1，控制方块不超出游戏区域 */
	for(i=0; i<VCellNo; i++)
	{
		g_ShareBuffer0[0][i]=8;
		g_ShareBuffer0[HCellNo-1][i]=8;
	}

	/* 将逻辑坐标系的最底下一行置1，控制方块不超出游戏区域 */
	for(i=0; i<HCellNo; i++)
		g_ShareBuffer0[i][VCellNo-1]=8;

	/* 将逻辑坐标系其他坐标全置0，游戏方块只能在这里移动 */
	for(i=1; i<HCellNo-1; i++)
	{
		for(j=0; j<VCellNo-1; j++)
			g_ShareBuffer0[i][j]=0;
	}

}



static void app_tetris_image_draw(const char *widget_name,char *path,uint32_t x,uint32_t y)
{
	GuiUpdateImage img = {0};

	img.x = x;
	img.y = y;
	img.name = path;

	GUI_SetProperty(widget_name, "image",(void*)&img);
}


static void app_draw_main_canvas_image(char *path,uint32_t x,uint32_t y)
{
	app_tetris_image_draw(CANVAS_TETRIS_MAIN,path,x-OFFSET_X,y+OFFSET_Y);
}


static void app_draw_little_canvas_image(char *path,uint32_t x,uint32_t y)
{
	app_tetris_image_draw(CANVAS_TETRIS_LITTER,path,x,y);
}

static  void app_game_tetris_number2string(char *numStr,uint32_t len,uint32_t number)
{
	if(NULL == numStr || 0 == len)
		return;

	memset(numStr,0,len);
	sprintf(numStr,"%d",number);
	numStr[len -1] = '\0';
}


static void app_game_tetris_score_calculate(void)
{
#define MAXLINE 9

	char str[STRLEN] = {0};
	if(RussionCurScoreline >MAXLINE)
	{
		RussionGrade++;
		app_game_tetris_number2string(str,STRLEN,RussionGrade);
		GUI_SetProperty(TXT_TETRIS_LEVEL, "string",str);
		RussionCurScoreline -= MAXLINE;
		app_game_tetris_number2string(str,STRLEN,RussionCurScoreline);
		GUI_SetProperty(TXT_TETRIS_LINE, "string",str);
	}
}

static void app_tetris_canvas_clear(const char *widget_name, uint16_t x, uint16_t y,uint16_t width, uint16_t height)
{
	GuiUpdateRect rect = {0};

	rect.x = x;
	rect.y = y;
	rect.w = width;
	rect.h = height;
	rect.color = "Snake_BG";

	GUI_SetProperty(widget_name, "rectangle",(void*)&rect);
}


/*******************************************************\
*		按照逻辑坐标系各坐标点的值重绘整个游戏区域		*
\*******************************************************/
void app_game_russion_paint(void)
{
	uint16_t m = 0;
	uint16_t n = 0;

	for (m=Top; m<VCellNo-1; m++)
	{
		for(n=1; n<HCellNo-1; n++)
		{
			if(1==g_ShareBuffer0[n][m])
				app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_RED,n*CellSize, m*CellSize);
			if(2==g_ShareBuffer0[n][m])
				app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_YELLOW,n*CellSize, m*CellSize);
			if(3==g_ShareBuffer0[n][m])
				app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_BULE,n*CellSize, m*CellSize);
			if(4==g_ShareBuffer0[n][m])
				app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_GREEN, n*CellSize, m*CellSize);
			if(5==g_ShareBuffer0[n][m])
				app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_BROWN,n*CellSize, m*CellSize);
			if(6==g_ShareBuffer0[n][m])
				app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_PINK,n*CellSize,m*CellSize);
			if(7==g_ShareBuffer0[n][m])
				app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_ORANGE,n*CellSize,m*CellSize);
		}
	}
}

/*******************************************************\
*				生成新的方块							*
\*******************************************************/
void app_game_russion_new_block(void)
{
	uint16_t i = 0;
	uint16_t j = 0;
	uint16_t k = 0;
	uint16_t lines = 0;

	Flag = 0;	/*方块旋转次数*/

	for(i=Top; i<VCellNo-1; i++)
	{
		lines =0;
		/* 检查是否有某一行全部被填满,即该行坐标全置1 */
		for(j=1; j<HCellNo; j++)
		{
			if(! g_ShareBuffer0[j][i])
			{
				lines=1;
				break;
			}
		}

		/* 若该行被填满，则将上一行的填充状态复制到该行，依此类推 */
		/* 即从该行开始，所有的逻辑坐标都下移一行，得分加1，Top加1 */
		if(!lines)
		{
			char str[STRLEN] = {0};
			for(j=1;j<HCellNo; j++)
			{
				for(k=i; k>=Top; k--)
				{
					g_ShareBuffer0[j][k]=g_ShareBuffer0[j][k-1];
				}
			}
			RussionCurScoreline++;
			app_game_tetris_number2string(str,STRLEN,RussionCurScoreline);
			GUI_SetProperty(TXT_TETRIS_LINE, "string",str);
			//totalScore += RussionCurScoreline;
			totalScore ++;
			app_game_tetris_number2string(str,STRLEN,totalScore);
			GUI_SetProperty(TXT_TETRIS_SCORE, "string",str);
			app_game_tetris_score_calculate();

			app_tetris_canvas_clear(CANVAS_TETRIS_MAIN,0, 0,CANVAS_WIDTH_MAIN, CANVAS_HEIGHT_MAIN);
			app_game_russion_paint();
			Top++;
		}
	}

	for(i=0;i<4;i++)
	{
		for(j=0;j<2;j++)
		{
			org[i][j]=block[i][j] =blockN[i][j];
		}
	}

	SelInMove = Sel;
}



void app_game_russion_next_block(void)
{
#define X_OFFSET 10
#define Y_OFFSET 0

	uint16_t  i = 0;

	/* 产生方块，每个方块的形状由4个CELL决定，每个CELL的位置由block[i][0]x坐标和block[i][1]y坐标决定 */
	/* 为使初始方块在顶部中央，block[i][0]=7/8/9/10，block[i][1]=0/1/2 */
	Sel = rand()%7;

	switch(Sel)
	{
		case 0:
			blockN[0][0]=6;	blockN[0][1] =0;
			blockN[1][0]=7;	blockN[1][1] =0;
			blockN[2][0]=6;	blockN[2][1] =1;
			blockN[3][0]=7;	blockN[3][1] =1;
			break;

		case 1:
			blockN[0][0] =5;	blockN[0][1] =0;
			blockN[1][0] =6;	blockN[1][1] =0;
			blockN[2][0] =7;	blockN[2][1] =0;
			blockN[3][0] =8;	blockN[3][1] =0;
			break;

		case 2:
			blockN[0][0] =6;	blockN[0][1] =0;
			blockN[1][0] =6;	blockN[1][1] =1;
			blockN[2][0] =7;	blockN[2][1] =1;
			blockN[3][0] =7;	blockN[3][1] =2;
			break;

		case 3:
			blockN[0][0] =7;	blockN[0][1] =0;
			blockN[1][0] =7;	blockN[1][1] =1;
			blockN[2][0] =6;	blockN[2][1] =1;
			blockN[3][0] =6;	blockN[3][1] =2;
			break;

		case 4:
			blockN[0][0] =6;	blockN[0][1] =0;
			blockN[1][0] =6;	blockN[1][1] =1;
			blockN[2][0] =6;	blockN[2][1] =2;
			blockN[3][0] =7;	blockN[3][1] =2;
			break;

		case 5:
			blockN[0][0] =7;	blockN[0][1] =0;
			blockN[1][0] =7;	blockN[1][1] =1;
			blockN[2][0] =7;	blockN[2][1] =2;
			blockN[3][0] =6;	blockN[3][1] =2;
			break;
		case 6:
			blockN[0][0] =6;	blockN[0][1] =0;
			blockN[1][0] =5;	blockN[1][1] =1;
			blockN[2][0] =6;	blockN[2][1] =1;
			blockN[3][0] =7;	blockN[3][1] =1;
			break;
		default:
			break;
	}

	app_tetris_canvas_clear(CANVAS_TETRIS_LITTER,0,0,CANVAS_WIDTH_LITTLE,CANVAS_WIDTH_LITTLE);

	for(i=0;i<4;i++)
	{
		switch(Sel)
		{
			case 0:
				app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_RED,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
				break;
			case 1:
				app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_YELLOW,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
				break;
			case 2:
				app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_BULE,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
				break;
			case 3:
				app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_GREEN,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
				break;
			case 4:
				app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_BROWN,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
				break;
			case 5:
				app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_PINK,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
				break;
			case 6:
				app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_ORANGE,(blockN[i][0]-5)*CellSize + X_OFFSET+10,(blockN[i][1]+1)*CellSize + X_OFFSET);
				break;
			default:
				break;
		}
	}
}




/*******************************************************\
*				绘制一个最小的方块单元					*
\*******************************************************/
void app_game_russion_draw_cell(uint16_t left, uint16_t top)
{
 	switch(SelInMove)	{
		case 0:
			app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_RED,left, top);
			break;

		case 1:
			app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_YELLOW,left, top);
			break;

		case 2:
			app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_BULE,left, top);
			break;

		case 3:
			app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_GREEN,left, top);
			break;

		case 4:
			app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_BROWN,left, top);
			break;

		case 5:
			app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_PINK,left, top);
			break;

		case 6:
			app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_ORANGE,left, top);
			break;
		default:
			break;
	}

}


/*******************************************************\
*				绘制运动方块							*
*				每个方块由4个CELL组成，共7种形状		*
\*******************************************************/
void app_game_russion_draw_block(uint16_t block [4][2])
{
	unsigned char i;
	for(i=0; i<4; i++)
	{
		app_game_russion_draw_cell(block[i][0]*CellSize, block[i][1]*CellSize);
	}
}


/*******************************************************************\
*			清除原位置的方块										*
*			在原方块的每个CELL处画一个与CELL同样大小的黑块			*
\*******************************************************************/
void app_game_russion_cover (uint16_t org[4][2])
{
	uint16_t i;
	for(i=0; i<4; i++)
	{
		app_tetris_canvas_clear(CANVAS_TETRIS_MAIN,org[i][0]*CellSize-OFFSET_X,org[i][1]*CellSize+OFFSET_Y,CellSize,CellSize);
	}
}

/*******************************************************\
*		清除当前方块，并在新的位置重新绘制方块			*
\*******************************************************/
void app_game_russion_draw(void)
{
	uint16_t i = 0;
	uint16_t j = 0;

	//清除前一次的方块
	app_game_russion_cover(org);
	for(i=0; i<4; i++)
	{
		for(j=0; j<2; j++)
			org[i][j]=block[i][j];
	}


	//重画新的方块
	app_game_russion_draw_block(block);
	return;
}


/*******************************************************\
*				按键响应，移动方块或变形				*
\*******************************************************/
void app_game_russion_move(uint16_t key)
{
	uint16_t r = 0;
	uint16_t i = 0;
	uint16_t j = 0;

	switch(key)
	{
		case STBK_LEFT:
			for(i=0; i<4; i++)
				block[i][0]--;
			break;

		case STBK_RIGHT:
			for(i=0; i<4; i++)
				block[i][0]++;
			break;

		case STBK_DOWN:
			for(i=0; i<4; i++)
				block[i][1]++;
			break;

		case STBK_UP:
			r=1;
			Flag++;     //方块旋转次数累加
			switch(SelInMove)
			{
				case 0:
					break;

				case 1:
					Flag =Flag%2;
					for(i=0; i<4; i++)
					{
						block[i][(Flag+1)%2] =org[2][(Flag+1)%2];
						block[i][Flag] =org[2][Flag]-2+i;
					}
					break;

				case 2:
					Flag =Flag%2;
					if(Flag)
					{
						block[0][1] +=2;	block[3][0] -=2;
					}
					else
					{
						block[0][1] -=2;	block[3][0] +=2;
					}
					break;

				case 3:
					Flag =Flag%2;
					if(Flag)
					{
						block[0][1] +=2;	block[3][0] +=2;
					}
					else
					{
						block[0][1] -=2;	block[3][0] -=2;
					}
					break;

				case 4:
					Flag=Flag%4;
					switch(Flag)
					{
						case 0:
							block[2][0] +=2;	block[3][0] +=2;
							block[2][1] +=1;	block[3][1] +=1;
							break;
						case 1:
							block[2][0] +=1;	block[3][0] +=1;
							block[2][1] -=2;	block[3][1] -=2;
							break;
						case 2:
							block[2][0] -=2;	block[3][0] -=2;
							block[2][1] -=1;	block[3][1] -=1;
							break;
						case 3:
							block[2][0] -=1;	block[3][0] -=1;
							block[2][1] +=2;	block[3][1] +=2;
							break;
					}
					break;

				case 5:
					Flag=Flag%4;
					switch(Flag)
					{
						case 0:
							block[2][0] +=1;	block[3][0] +=1;
							block[2][1] +=2;	block[3][1] +=2;
							break;
						case 1:
							block[2][0] +=2;	block[3][0] +=2;
							block[2][1] -=1;	block[3][1] -=1;
							break;
						case 2:
							block[2][0] -=1;	block[3][0] -=1;
							block[2][1] -=2;	block[3][1] -=2;
							break;
						case 3:
							block[2][0] -=2;	block[3][0] -=2;
							block[2][1] +=1;	block[3][1] +=1;
							break;
					}
					break;

				case 6:
					Flag =Flag%4;
					switch(Flag)
					{
						case 0:
							block[0][0]++; block[0][1]--;
							block[1][0]--; block[1][1]--;
							block[3][0]++; block[3][1]++;
							break;
						case 1:
							block[1][0]++; block[1][1]++; break;
						case 2:
							block[0][0]--; block[0][1]++; break;
						case 3:
							block[3][0]--; block[3][1]--; break;
					}
					break;
				}
				break;
			default:
				break;
	}

	/* 判断方块旋转后新位置是否有CELL，若有，则旋转取消 */
	for(i=0; i<4; i++)
	{
		if(g_ShareBuffer0[block[i][0]][ block[i][1]])
		{
			if(r)
			{
				Flag +=3;
			}
			for(i=0; i<4; i++)
			{
				for(j=0; j<2; j++)
					block[i][j]=org[i][j];
			}
			return;
		}
	}
	app_game_russion_draw();
}


/*******************************************************\
*				下移方块，使方块运动					*
\*******************************************************/
int  app_game_russion_down_able(void)
{
	uint16_t i,Temp=0xFFFF;
	uint32_t X ,Y = 0;
	static  uint32_t FirstBlock = 0;

	if(FirstBlock == 0)
	{
		FirstBlock = 1;
		app_game_russion_draw_block(org);
	}

	for(i=0; i<4; i++)
		block[i][1]++;

	/* 检查方块下移是否被档住，即判断下移后新坐标是否是1 */
	for(i=0; i<4; i++)
	{
		if(g_ShareBuffer0[ block[i][0] ][ block[i][1] ])
		{
			for(i=0; i<4; i++)
				g_ShareBuffer0[ org[i][0] ][ org[i][1] ]=SelInMove+1;
			if(Top>org[0][1]-2)
				Top=org[0][1]-2;
			FirstBlock = 0;

			if(org[0][1]<=2)
				Temp = org[0][1]-1;

			app_game_russion_new_block();
			app_game_russion_next_block();

			if(Temp != 0xFFFF)
			{
				if(Temp == 1)//还剩两行
				{
					if(org[3][1]==2)
					{
						for(i=4; i>0; i--)
						if(org[i-1][1]>=1)
						{
							X = org[i-1][0]*CellSize;
							Y = (org[i-1][1]-1)*CellSize;
							app_game_russion_draw_cell(X, Y);

						}
					}
					else if(org[3][1]==1)
					{
						for(i=4; i>0; i--)
						{
							X = org[i-1][0]*CellSize;
							Y = (org[i-1][1])*CellSize;
							app_game_russion_draw_cell(X, Y);

						}
					}
					else if(org[3][1]==0)
					{
						for(i=4; i>0; i--)
						{
							X = org[i-1][0]*CellSize;
							Y = (org[i-1][1]+1)*CellSize;
							app_game_russion_draw_cell(X, Y);

						}
						//剩余两行，且此块只有一行，则再出来一块
						app_game_russion_new_block();
						app_game_russion_next_block();
						for(i=4; i>0; i--)
						{
							if(org[i-1][1]==org[3][1])
							{
								X = org[i-1][0]*CellSize;
								Y = 0;
								app_game_russion_draw_cell(X, Y);

							}
						}
					}
				}
				else if(Temp==0)//还剩一行
				{
					for(i=4; i>0; i--)
					{
						if(org[i-1][1]==org[3][1])
						{
							X = org[i-1][0]*CellSize;
							Y = 0;
							app_game_russion_draw_cell(X, Y);
						}

					}
				}
				Temp = 0xFFFF;
			}
			return 0;
		}
	}
	return 1;
}
void app_game_russion_down(void)
{
	if(app_game_russion_down_able())
	{
		app_game_russion_draw();
	}
}

void app_game_russion_game_process(void)
{
	app_game_russion_draw();
}


static void app_game_tetris_over(void)
{
	gameStatus = GAMEQUIT;

	if(NULL != tetrisTimer)
	{
		timer_stop(tetrisTimer);
		remove_timer(tetrisTimer);
		tetrisTimer = NULL;
		gameStatus = GAMEQUIT;
	}
}

static int app_game_tetris_timer_handle(void *userdata)
{
	if(GAMEOVER == gameStatus)
	{
		GUI_SetProperty(BTN_TETRIS_STATUS, "string",(void*)STR_ID_GAMEOVER);
		GUI_SetProperty(TXT_TETRIS_OK, "string",(void*)STR_ID_START);
		app_game_tetris_over();
	}
	else if(GAMESTART == gameStatus)
	{
		app_game_russion_game_process();
		app_game_russion_down();

		if ((Top<1)|(Top>VCellNo))
		{
			gameStatus = GAMEOVER;
		}
	}else{}

	return 0;
}

static void app_game_tetris_set_andvance(void)
{
	int interval_timer = INIT_TIMER_VALUE;
	interval_timer = interval_timer/(RussionGrade+1);
	if(NULL != tetrisTimer)
	{
		remove_timer(tetrisTimer);
		tetrisTimer = create_timer(app_game_tetris_timer_handle, interval_timer, NULL, TIMER_REPEAT);
	}
}

static void app_game_handle_grade(void)
{
	char str[STRLEN] = {0};
	app_game_tetris_set_andvance();
	app_game_tetris_number2string(str,STRLEN,RussionGrade);
	GUI_SetProperty(TXT_TETRIS_LEVEL, "string",str);
}

void app_game_tetris_create(void)
{
    GUI_CreateDialog(WND_TETRIS);
}


/* Export functions ------------------------------------------------------ */
SIGNAL_HANDLER int app_tetris_create(const char* widgetname, void *usrdata)
{
	char str[STRLEN] = {0};

	gameStatus = GAMEQUIT;
	//GUI_SetProperty(BOX_TETRIS_STATUS, "state","disable");
	GUI_SetProperty(WND_TETRIS, "state","hide");
	GUI_SetInterface("flush",NULL);
	app_game_russion_Datainit();

	app_game_tetris_number2string(str,STRLEN,RussionGrade);
	GUI_SetProperty(TXT_TETRIS_LEVEL, "string",str);

	app_game_tetris_number2string(str,STRLEN,RussionCurScoreline);
	GUI_SetProperty(TXT_TETRIS_LINE, "string",str);

	app_game_tetris_number2string(str,STRLEN,totalScore);
	GUI_SetProperty(TXT_TETRIS_SCORE, "string",str);

	GUI_SetProperty(BTN_TETRIS_STATUS, "string",(void*)STR_ID_READY);
	GUI_SetProperty(TXT_TETRIS_OK, "string",(void*)STR_ID_START);


	  GUI_SetFocusWidget("btn_tetris_status");
	GUI_SetProperty(WND_TETRIS, "state","show");

	GUI_SetInterface("flush",NULL);

	app_tetris_canvas_clear(CANVAS_TETRIS_MAIN,0,0,CANVAS_WIDTH_MAIN,CANVAS_HEIGHT_MAIN);
	app_tetris_canvas_clear(CANVAS_TETRIS_LITTER,0,0,CANVAS_WIDTH_LITTLE,CANVAS_HEIGHT_LITTLE);


	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tetris_destroy(const char* widgetname, void *usrdata)
{
	if (tetrisTimer != NULL)
	{
		remove_timer(tetrisTimer);
		tetrisTimer = NULL;
		gameStatus = GAMEQUIT;
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tetris_lost_focus(GuiWidget *widget, void *usrdata)
{
	if(gameStatus != GAMEQUIT)
	{
		gameStatus = GAMEPAUSE;
		GUI_SetProperty(BTN_TETRIS_STATUS, "string",(void*)STR_ID_PAUSE);
		GUI_SetProperty(TXT_TETRIS_OK, "string",(void*)STR_ID_START);
		timer_stop(tetrisTimer);
	}
	return EVENT_TRANSFER_STOP;
}

int app_tetris_get_focus(void)
{
	char str[STRLEN] = {0};
	int i=0;

		app_game_tetris_number2string(str,STRLEN,RussionGrade);
		GUI_SetProperty(TXT_TETRIS_LEVEL, "string",str);

		app_game_tetris_number2string(str,STRLEN,RussionCurScoreline);
		GUI_SetProperty(TXT_TETRIS_LINE, "string",str);

		app_game_tetris_number2string(str,STRLEN,totalScore);
		GUI_SetProperty(TXT_TETRIS_SCORE, "string",str);

		GUI_SetProperty(BTN_TETRIS_STATUS, "string",(void*)STR_ID_PAUSE);
		GUI_SetProperty(TXT_TETRIS_OK, "string",(void*)STR_ID_START);
		GUI_SetInterface("flush",NULL);
		app_tetris_canvas_clear(CANVAS_TETRIS_LITTER,0,0,CANVAS_WIDTH_LITTLE,CANVAS_HEIGHT_LITTLE);
		app_tetris_canvas_clear(CANVAS_TETRIS_MAIN,0,0,CANVAS_WIDTH_MAIN,CANVAS_HEIGHT_MAIN);
		app_game_russion_draw_block(block);
		for(i=0;i<4;i++)
		{
			switch(Sel)
			{
				case 0:
					app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_RED,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
					break;
				case 1:
					app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_YELLOW,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
					break;
				case 2:
					app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_BULE,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
					break;
				case 3:
					app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_GREEN,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
					break;
				case 4:
					app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_BROWN,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
					break;
				case 5:
					app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_PINK,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
					break;
				case 6:
					app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_ORANGE,(blockN[i][0]-5)*CellSize + X_OFFSET+10,(blockN[i][1]+1)*CellSize + X_OFFSET);
					break;
				default:
					break;
				}
			}
		app_game_russion_paint();
	return 0;
}

SIGNAL_HANDLER int app_tetris_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	char str[STRLEN] = {0};

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case VK_BOOK_TRIGGER:
				GUI_EndDialog("after wnd_full_screen");
				GUI_SetProperty(WND_TETRIS,"draw_now",NULL);
				break;

			case STBK_MENU:
			case STBK_EXIT:
				gameStatus = GAMEQUIT;
				timer_stop(tetrisTimer);
				remove_timer(tetrisTimer);
				tetrisTimer = NULL;
				GUI_EndDialog(WND_TETRIS);
				break;
			/*case STBK_PAUSE:
				gameStatus = GAMEPAUSE;
				GUI_SetProperty(BTN_TETRIS_STATUS, "string",(void*)STR_ID_PAUSE);
				timer_stop(tetrisTimer);
				break;*/
			case STBK_UP:
			case STBK_DOWN:
			case STBK_LEFT:
			case STBK_RIGHT:
			{
				if(GAMESTART == gameStatus)
				{
					app_game_russion_move(event->key.sym);
				}
				break;
			}
			case STBK_OK:
			{
				if(GAMEQUIT == gameStatus)
				{
					int interval_timer = INIT_TIMER_VALUE;
					gameStatus = GAMESTART;

					app_game_russion_Datainit();

					app_game_tetris_number2string(str,STRLEN,RussionGrade);
					GUI_SetProperty(TXT_TETRIS_LEVEL, "string",str);

					app_game_tetris_number2string(str,STRLEN,RussionCurScoreline);
					GUI_SetProperty(TXT_TETRIS_LINE, "string",str);

					app_game_tetris_number2string(str,STRLEN,totalScore);
					GUI_SetProperty(TXT_TETRIS_SCORE, "string",str);

					app_tetris_canvas_clear(CANVAS_TETRIS_LITTER,0,0,CANVAS_WIDTH_LITTLE,CANVAS_HEIGHT_LITTLE);
					app_tetris_canvas_clear(CANVAS_TETRIS_MAIN,0,0,CANVAS_WIDTH_MAIN,CANVAS_HEIGHT_MAIN);
					app_game_russion_next_block();
					app_game_russion_new_block();
					GUI_SetProperty(BTN_TETRIS_STATUS, "string",(void*)STR_ID_START);
					GUI_SetProperty(TXT_TETRIS_OK, "string",(void*)STR_ID_PAUSE);

					interval_timer = interval_timer/(RussionGrade+1);
					tetrisTimer = create_timer(app_game_tetris_timer_handle, interval_timer, NULL, TIMER_REPEAT);
				}
				else if(GAMEPAUSE == gameStatus)
				{
					gameStatus = GAMESTART;
					reset_timer(tetrisTimer);
					GUI_SetProperty(BTN_TETRIS_STATUS, "string",(void*)STR_ID_START);
					GUI_SetProperty(TXT_TETRIS_OK, "string",(void*)STR_ID_PAUSE);
				}
				else if(GAMESTART == gameStatus)
				{
					gameStatus = GAMEPAUSE;
					GUI_SetProperty(BTN_TETRIS_STATUS, "string",(void*)STR_ID_PAUSE);
					GUI_SetProperty(TXT_TETRIS_OK, "string",(void*)STR_ID_START);
					timer_stop(tetrisTimer);
				}
				break;
			}
			case STBK_0:
				RussionGrade = 0;
				app_game_handle_grade();
				break;
			case STBK_1:
				RussionGrade = 1;
				app_game_handle_grade();
				break;
			case STBK_2:
				RussionGrade = 2;
				app_game_handle_grade();
				break;
			case STBK_3:
				RussionGrade = 3;
				app_game_handle_grade();
				break;
			case STBK_4:
				RussionGrade = 4;
				app_game_handle_grade();
				break;
			case STBK_5:
				RussionGrade = 5;
				app_game_handle_grade();
				break;
			case STBK_6:
				RussionGrade = 6;
				app_game_handle_grade();
				break;
			case STBK_7:
				RussionGrade = 7;
				app_game_handle_grade();
				break;
			case STBK_8:
				RussionGrade = 8;
				app_game_handle_grade();
				break;
			case STBK_9:
				RussionGrade = 9;
				app_game_handle_grade();
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tetris_main_canvas_create(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tetris_little_canvas_create(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

#endif
//* ------------------------------- End of file ---------------------------- */
