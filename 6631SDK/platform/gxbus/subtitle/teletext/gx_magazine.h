#ifndef __GX_MAGAZINE_H__
#define __GX_MAGAZINE_H__

#include "gx_subtitle_magazine.h"

typedef enum {
	GX_MAGAZINE_C4  = 0x000080,
	GX_MAGAZINE_C5  = 0x004000,
	GX_MAGAZINE_C6  = 0x008000,
	GX_MAGAZINE_C7  = 0x010000,
	GX_MAGAZINE_C8  = 0x020000,
	GX_MAGAZINE_C9  = 0x040000,
	GX_MAGAZINE_C10 = 0x080000,
	GX_MAGAZINE_C11 = 0x100000,
	GX_MAGAZINE_C12 = 0x200000,
	GX_MAGAZINE_C13 = 0x400000,
	GX_MAGAZINE_C14 = 0x800000,
} GxMagazine_Type;

typedef enum {
	GX_MAGAZINE_DRAW_PAGE_0 = 0,
	GX_MAGAZINE_DRAW_PAGE_1 = 1, //P0不透背景,p1-p24正常背景 (背景: 透明/不透明)
	GX_MAGAZINE_DRAW_P0_1 = 2,   //P0正常背景,P1-P24黑色背景
	GX_MAGAZINE_DRAW_P0_2 = 3,   //P0不透背景,P1-P24透明背景
} GxMagazine_Draw;

typedef int (*GxMagazineShowCB)(void *pirv, vbi_page *page, GxMagazine_Draw draw);
typedef int (*GxMagazineSyncCB)(void *pirv);

extern int GxMagazine_Open        (void);
extern int GxMagazine_Close       (void);
extern int GxMagezine_SetCallBack (void* priv, GxMagazineShowCB show, GxMagazineSyncCB sync);
extern int GxMagazine_SetFirstPage(unsigned char magNum, unsigned char pageNo);
extern int GxMagazine_AddPage     (vbi_page *page, GxMagazine_Type type);
extern int GxMagazine_JumpPage    (unsigned int pgno);
extern int GxMagazine_SetPageLang (GX_SUBTITLE_PGLANG *pglang);
extern int GxMagazine_JumpSubPage (unsigned int subno);
extern int GxMagazine_ShowString  (char *str);
extern int GxMagazine_ShowTimer   (void);
extern int GxMagazine_AutoJumpSubPage   (void);
extern int GxMagazine_GetFirstPage      (unsigned int *pgno);
extern int GxMagazine_GetCurrentPage    (unsigned int *pgno);
extern int GxMagazine_BackwardSearchPage(unsigned int *pgno);
extern int GxMagazine_ForwardSearchPage (unsigned int *pgno);
extern int GxMagazine_GetSixLinkPgNo    (unsigned int *pgno);
extern int GxMagazine_ShowDefaultPage   (vbi_decoder  *vbi);
extern int GxMagazine_RefreshCurPage    (void);

extern int GxMagazine_BackwardSearchSubPage(unsigned int *subno);
extern int GxMagazine_ForwardSearchSubPage (unsigned int *subno);
extern int GxMagazine_GetCurrentSubPage    (unsigned int *subno);

extern int GxMagazine_VBIDebugChar (GX_SUBTITLE_VBI_DCHAR *dchar);
extern int GxMagazine_VBIModifyChar(GX_SUBTITLE_VBI_MCHAR *mchar);
#endif
