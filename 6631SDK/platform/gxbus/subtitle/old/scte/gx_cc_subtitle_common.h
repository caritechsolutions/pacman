/*****************************************************************************
* 						 CONFIDENTIAL
* 	   Hangzhou Nationalchip Science and Technology Co., Ltd.
* 					 (C) All right reserved
******************************************************************************

******************************************************************************
* Release History:
VERSION	      Date			        AUTHOR		       Description
1.0	                   2018.03.19		  zhouhm 			 creation
*****************************************************************************/

#ifndef __GX_CC_SUBTITLE_COMMON__H__
#define __GX_CC_SUBTITLE_COMMON__H__
#ifdef __cplusplus
extern "C" {
#endif
#include <gxtype.h>

#define CC_SUBTITLE_COMMON_ERROR(debug)\
{ \
	gxloge("[CC SUBTITLE COMMON ERROR]%s %d",__FILE__,__LINE__);\
	gxloge debug; \
} 

#if 0
#define CC_SUBTITLE_COMMON_DBG(debug)\
{ \
	gxlogd("[CC SUBTITLE COMMON DBG]%s %d",__FILE__,__LINE__);\
	gxlogd debug; \
} 
#else
#define CC_SUBTITLE_COMMON_DBG(debug)\
do{ \
}while(0) 
#endif

typedef enum {
     COLOR_BLACK = 0,  /* 黑色*/
	 COLOR_WHITE,      /* 白色*/
     COLOR_GREEN,      /* 绿色*/
     COLOR_BLUE,       /* 蓝色*/
     COLOR_CYAN,       /* 青色*/
     COLOR_RED,        /* 红色*/
     COLOR_YELLOW,     /* 黄色*/
     COLOR_MAGENTA,	   /* 紫色*/
     COLOR_INVALID
}COLOR_TYPE;

typedef enum {
     OPACITY_SOLID = 0,       /* 不透明*/
	 OPACITY_FLASH,			/*闪烁*/
	 OPACITY_TRANSLUCENT, /* 半透明*/
	 OPACITY_TRANSPARENT,      /* 透明*/
     OPACITY_INVALID
}OPACITY_TYPE;

typedef struct row_para_s
{
    uint32_t top_x; /*左顶点x位置*/
	uint32_t top_y; /*左顶点y位置*/
	uint32_t width; /*当前画笔绘制位置宽度*/
	uint32_t max_width; /*绘制最大宽度*/
	uint32_t height; /*高度*/
} row_para_t;

#ifdef __cplusplus
}
#endif
#endif /*__GX_CC_SUBTITLE_COMMON__H__*/

