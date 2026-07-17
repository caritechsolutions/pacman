#include "gxcore.h"
#include "gx_mem.h"
#include "libzvbi/libzvbi.h"
#include "gx_magazine.h"
#include "gxplayer_module.h"
#include "gx_teletext.h"
#include "gx_system.h"

#define MAX_MAJOR_PAGE 900
#define MIN_MAJOR_PAGE 100
#define MAX_MINOR_PAGE  14
#define MAX_PAGE_COL    42
#define MAX_PAGE_LINK   (4)
//搜索输入页码区域，用户自定义str
#define P0_REGION0      (7)
#define P0_REGION0_POS  (0)
//当前/加载页码显示区域
#define P0_REGION1      (9)
#define P0_REGION1_POS  (8)
//当前页面信息显示区域
#define P0_REGION2      (17)
#define P0_REGION2_POS  (13)
//时间更新区域，由P0定义
#define P0_REGION3      (10)
#define P0_REGION3_POS  (31)

#define P0_C6_CLEARER   (6)
#define P0_REGION1_SHOW (4)

typedef enum {
	GX_MAGAZINE_MEM_FULL,   //申请当前页面所有页面，子页+50/-50，保证可以快速切换页面以及子页。
	GX_MAGAZINE_MEM_HALF,   //申请当前页面前后+200/-200的页面，子页+20/-20
	GX_MAGAZINE_MEM_LIGHT,  //申请当前页面前后+25/-25的页面,子页+1/-1
} GxMagazineMemType;

typedef struct {
	unsigned int   curPgNo;
	unsigned int   curSubNo;
	unsigned int   lastPgNo;
	unsigned int   lastSubNo;
	unsigned int   firstPgNo;
	unsigned int   autoSearch;
	handle_t       mutex;
	unsigned char  region1Refresh;   //刷新太快容易导致demuxer数据溢出
	unsigned char  refresh;          //页面发生变化时刷新
	unsigned char  clearC6;          //切换到C6,需要清空区域
	unsigned int   p0ClearC6;        //
	vbi_page       *loadPage;
	vbi_page       *curPage;
	vbi_page       *lastPage;
	vbi_page       *defPage;
	bool           showDefPage;
	bool           c6Page[MAX_MAJOR_PAGE][MAX_MINOR_PAGE]; //是否是C6页面
	unsigned char  saveSubNo[MAX_MAJOR_PAGE];
	vbi_page       *savePage[MAX_MAJOR_PAGE][MAX_MINOR_PAGE];
	unsigned short P0[MAX_PAGE_COL];
	unsigned int   linkPage[MAX_PAGE_LINK];
	GxMagazineShowCB showCB;
	GxMagazineSyncCB syncCB;
	void             *priv;
	GxMagazineMemType memType;
	unsigned int      pageCount;
} GxMagaZineContext;

static vbi_page_charset  *mCharset = NULL;
static GxMagaZineContext *mCtx     = NULL;
static unsigned int debug_teletext_magazine = 0;

static void init_to_P0_unicode(GxMagaZineContext *ctx)
{
	unsigned int i = 0;

	for (i = 0; i < MAX_PAGE_COL; i++)
		ctx->P0[i] = ' ';

	return;
}

static int str_to_P0_unicode(GxMagaZineContext *ctx, char *str, unsigned int region)
{
	unsigned int i = 0, len = 0;
	unsigned char refresh = 0;

	if (region == P0_REGION0_POS) len = P0_REGION0;
	else if (region == P0_REGION1_POS) len = P0_REGION1;
	else if (region == P0_REGION2_POS) len = P0_REGION2;
	else if (region == P0_REGION3_POS) len = P0_REGION3;

	for (i = 0; i < len; i++) {
		if (i >= strlen(str)) {
			ctx->P0[region + i] = ' ';
		} else {
			if (str[i] != ctx->P0[region + i]) {
				ctx->P0[region + i] = str[i];
				refresh = 1;
			}
		}
	}

	return refresh;
}

static int page_to_P0_unicode(GxMagaZineContext *ctx, vbi_page *page, unsigned int region)
{
	unsigned int i = 0, len = 0;
	unsigned char refresh = 0;

	if (region == P0_REGION0_POS) len = P0_REGION0;
	else if (region == P0_REGION1_POS) len = P0_REGION1;
	else if (region == P0_REGION2_POS) len = P0_REGION2;
	else if (region == P0_REGION3_POS) len = P0_REGION3;

	for (i = 0; i < len; i++) {
		if (page->text[region + i].unicode != ctx->P0[region + i]) {
			ctx->P0[region + i] = page->text[region + i].unicode;
			refresh = 1;
		}
	}

	return refresh;
}

static int P0_to_page_unicode(GxMagaZineContext *ctx, vbi_page *page, unsigned int region)
{
	unsigned int i = 0, len = 0;

	if (region == P0_REGION0_POS) len = P0_REGION0;
	else if (region == P0_REGION1_POS) len = P0_REGION1;
	else if (region == P0_REGION2_POS) len = P0_REGION2;
	else if (region == P0_REGION3_POS) len = P0_REGION3;

	for (i = 0; i < len; i++)
		page->text[region + i].unicode = ctx->P0[region + i];

	return 0;
}

static int page_to_P0_text(vbi_page *srcPage, vbi_page *dstPage)
{
	unsigned int i = 0;

	for (i = 0; i < srcPage->columns; i++) {
		dstPage->text[i] = srcPage->text[i];
	}

	return 0;
}

static int page_check_P0_unicode(vbi_page *page, unsigned int region)
{
	unsigned int i = 0, len = 0;

	if (region == P0_REGION0_POS) len = P0_REGION0;
	else if (region == P0_REGION1_POS) len = P0_REGION1;
	else if (region == P0_REGION2_POS) len = P0_REGION2;
	else if (region == P0_REGION3_POS) len = P0_REGION3;

	for (i = 0; i < len; i++) {
		if (page->text[region + i].unicode != 0x20)
			return 1;
	}

	return 0;
}

static int switch_pageno_10to16(int num)
{
	int c2, c1, c0;

	c2 = (num / 100);
	c1 = ((num % 100) / 10);
	c0 = ((num % 100) % 10);

	return((c2 << 8) | (c1 << 4) | c0);
}

static int switch_pageno_16to10(int num)
{
	int c2, c1, c0;

	c2 = (num  >> 8);
	c1 = ((num >> 4) & 0xf);
	c0 = (num & 0xf);

	return((c2 * 100) + (c1 * 10) + c0);
}

static unsigned int switch_page_no(unsigned int pgno)
{
	unsigned int  tmpPgNo = 0;

	tmpPgNo = switch_pageno_16to10(pgno);
	if (tmpPgNo > MAX_MAJOR_PAGE)
		tmpPgNo = 0;

	return tmpPgNo;
}

static unsigned int switch_sub_no(unsigned int subno)
{
	unsigned int  tmpSubNo = 0;

	tmpSubNo = (switch_pageno_16to10(subno) - 1);
	if (tmpSubNo >= MAX_MINOR_PAGE)
		tmpSubNo = 0;

	return tmpSubNo;
}

static int alloc_sub_page(GxMagaZineContext *ctx, unsigned int pgno, unsigned int subno)
{
	//保留link页，规则页，首页，先前页，当前页
	int i = 0, j = 0, k = 0;
	unsigned int max_pg = 0, min_pg = 0;
	unsigned int swp_pg = 0, swp_sp = 0;

	if (ctx->memType == GX_MAGAZINE_MEM_FULL) {
		max_pg = MAX_MAJOR_PAGE;
		min_pg = 20;
	} else if (ctx->memType == GX_MAGAZINE_MEM_HALF) {
		max_pg = 200;
		min_pg = 20;
	} else {
		max_pg = 25;
		min_pg = 1;
	}

	for (i = MIN_MAJOR_PAGE; i < MAX_MAJOR_PAGE; i++) {
		if (ctx->lastPgNo == i)
			continue;

		if (ctx->firstPgNo == i)
			continue;

		for (k = 0; k < MAX_PAGE_LINK; k++) {
			if (ctx->linkPage[k] == i)
				break;
		}
		if (k < MAX_PAGE_LINK)
			continue;

		for (j = (MAX_MINOR_PAGE - 1); j >= 0; j--) {
			unsigned int pg_offset = abs(ctx->curPgNo - i);

			if (!ctx->savePage[i][j])
				continue;

			if (pg_offset <= min_pg)
				continue;

			if ((pg_offset > min_pg) && (pg_offset <= max_pg)) {
				unsigned int pg_count = 0;

				for (k = 0; k < MAX_MINOR_PAGE; k++) {
					if (!ctx->savePage[i][k])
						continue;
					pg_count++;
				}

				if (pg_count <= 1)
					continue;
			}

			if (pgno >= ctx->curPgNo) {
				if (swp_pg < MIN_MAJOR_PAGE) {//替换最小页面，子页最大
					swp_pg = i;
					swp_sp = j;
				}
			} else {
				if (swp_pg != i) {//替换最大页面，子页最大
					swp_pg = i;
					swp_sp = j;
				}
			}
		}
	}

	if (swp_pg < MIN_MAJOR_PAGE)
		goto alloc_page;

	if (debug_teletext_magazine) {
		gxlogi("[%d] free  page no : %d, sub no: %d\n", __LINE__, swp_pg, swp_sp);
		gxlogi("[%d] alloc page no : %d, sub no: %d\n", __LINE__, pgno, subno);
	}
	vbi_unref_page(ctx->savePage[swp_pg][swp_sp]);
	ctx->savePage[pgno][subno] = ctx->savePage[swp_pg][swp_sp];
	ctx->savePage[swp_pg][swp_sp] = NULL;
	return 1;

alloc_page:
	if (debug_teletext_magazine)
		gxlogi("[%d] alloc page no : %d, sub no: %d\n", __LINE__, pgno, subno);
	ctx->pageCount++;
	ctx->savePage[pgno][subno] = (vbi_page *)av_mallocz(sizeof(vbi_page));
	return 1;
}

static void fill_page_p0(GxMagaZineContext *ctx, vbi_page *page)
{
	unsigned int i = 0;

	for (i = 0; i < page->columns; i++) {
		page->text[i].opacity = 3;
		page->text[i].background = VBI_BLACK;
		if ((i >= P0_REGION0_POS) && (i < (P0_REGION0_POS + P0_REGION0)))
			page->text[i].foreground = VBI_GREEN;
	}
}

static int check_page_p24(GxMagaZineContext *ctx, vbi_page *page)
{
	unsigned int i = 0, changeP24 = 1;
	unsigned int pgNo  = switch_page_no(page->pgno);
	unsigned int subNo = switch_sub_no(page->subno);

	if (!ctx->c6Page[pgNo][subNo]) {
		for (i = 0; i < MAX_PAGE_LINK; i++) {
			if (page->nav_link[i].subno == VBI_ANY_SUBNO) {
				unsigned int linkPgNo = switch_page_no(page->nav_link[i].pgno);
				if ((linkPgNo >= MIN_MAJOR_PAGE) && (linkPgNo < MAX_MAJOR_PAGE)) {
					ctx->linkPage[i] = linkPgNo;
					changeP24 = 0;
				} else
					ctx->linkPage[i] = 0;
			} else
				ctx->linkPage[i] = 0;
		}
	}

	if (changeP24) {
		for (i = 0; i < MAX_PAGE_LINK; i++) {
			while (1) {
				pgNo++;
				if (pgNo >= MAX_MAJOR_PAGE)
					pgNo = ctx->firstPgNo;
				for (subNo = 0; subNo < MAX_MINOR_PAGE; subNo++) {
					if (ctx->savePage[pgNo][subNo])
						break;
				}
				if (subNo < MAX_MINOR_PAGE) {
					ctx->linkPage[i] = pgNo;
					break;
				}
			}
		}
	}

	return changeP24;
}

static int fill_page_p24p25(GxMagaZineContext *ctx, vbi_page *page)
{
	unsigned int i = 0;

	if (check_page_p24(ctx, page)) {
		unsigned int baseP24 = 24 * page->columns;
		char p24Strings[MAX_PAGE_COL];

		memset(p24Strings, 0x20, MAX_PAGE_COL);
		for (i = 0; i < MAX_PAGE_LINK; i++) {
			unsigned int linkPgNo = ctx->linkPage[i];
			unsigned int pos = (page->columns - 1) / MAX_PAGE_LINK * i + 3;

			if ((linkPgNo >= MIN_MAJOR_PAGE) && (linkPgNo < MAX_MAJOR_PAGE)) {
				sprintf(&p24Strings[pos], "%03d", linkPgNo);
				p24Strings[pos + 3] = 0x20;
			}
		}

		for (i = 0; i < page->columns; i++) {
			if ((i == 0) || (i == (page->columns - 1))) {
				page->text[baseP24 + i] = page->text[23 * page->columns + i];
			} else if (i < ((page->columns - 1) / MAX_PAGE_LINK)) {
				page->text[baseP24 + i].foreground = VBI_BLACK;
				page->text[baseP24 + i].background = VBI_RED;
				page->text[baseP24 + i].opacity    = 3;
				page->text[baseP24 + i].unicode    = p24Strings[i];
			} else if (i < ((page->columns - 1) / MAX_PAGE_LINK * 2)) {
				page->text[baseP24 + i].foreground = VBI_BLACK;
				page->text[baseP24 + i].background = VBI_GREEN;
				page->text[baseP24 + i].opacity    = 3;
				page->text[baseP24 + i].unicode    = p24Strings[i];
			} else if (i < ((page->columns - 1) / MAX_PAGE_LINK * 3)) {
				page->text[baseP24 + i].foreground = VBI_BLACK;
				page->text[baseP24 + i].background = VBI_YELLOW;
				page->text[baseP24 + i].opacity    = 3;
				page->text[baseP24 + i].unicode    = p24Strings[i];
			} else {
				page->text[baseP24 + i].foreground = VBI_BLACK;
				page->text[baseP24 + i].background = VBI_CYAN;
				page->text[baseP24 + i].opacity    = 3;
				page->text[baseP24 + i].unicode    = p24Strings[i];
			}
		}
	}
	page->rows = 25;

	if (1) {
		unsigned int baseP25 = 25 * page->columns;
		unsigned int pos = 0;
		unsigned int pgno = switch_page_no(page->pgno);
		char p25Strings[MAX_PAGE_COL];
		char p25Colors[MAX_PAGE_COL];

		memset(&page->text[baseP25], 0, sizeof(vbi_char) * page->columns);
		memset(p25Strings, 0x20,      MAX_PAGE_COL);
		memset(p25Colors,  VBI_WHITE, MAX_PAGE_COL);

		pos += 1;
		sprintf(&p25Strings[pos], "AUTO");
		if (ctx->autoSearch)
			memset(&p25Colors[pos], VBI_RED, 4);
		pos += 4;
		p25Strings[pos] = 0x20;

		for (i = 0; i < MAX_MINOR_PAGE; i++) {
			if (ctx->savePage[pgno][i]) {
				pos += 1;
				sprintf(&p25Strings[pos], "%d", (i + 1));
				if ((ctx->autoSearch == 0) && (ctx->curSubNo == i)) {
					p25Colors[pos + 0] = VBI_RED;
					p25Colors[pos + 1] = (i >= 9) ? VBI_RED : p25Colors[pos + 1];
				}
				pos += (i >= 9) ? 2 : 1;
				p25Strings[pos] = 0x20;
			}
		}

		for (i = 0; i < page->columns; i++) {
			if ((i == 0) || (i == (page->columns - 1))) {
				page->text[baseP25 + i] = page->text[23 * page->columns + i];
			} else {
				page->text[baseP25 + i].opacity    = 3;
				page->text[baseP25 + i].foreground = p25Colors[i];
				page->text[baseP25 + i].background = VBI_BLACK;
				page->text[baseP25 + i].unicode    = p25Strings[i];
			}
		}
		page->rows = 26;
	}

	return 0;
}

int GxMagazine_Open(void)
{
	if (!mCtx) {
		int i = 0;
		int sysMemSize = 0;
		GxPlayer_SystemGet(PSYS_PACKET_CACHE, (int32_t *)&sysMemSize);

		mCtx = (GxMagaZineContext *)av_mallocz(sizeof(GxMagaZineContext));

		for (i = 0; i < MAX_MAJOR_PAGE; i++)
			mCtx->saveSubNo[i] = 0xFF;

		if (sysMemSize <= 4*1024*1024)
			mCtx->memType = GX_MAGAZINE_MEM_LIGHT;
		else if (sysMemSize <= 8*1024*1024)
			mCtx->memType = GX_MAGAZINE_MEM_HALF;
		else
			mCtx->memType = GX_MAGAZINE_MEM_FULL;
		GxCore_MutexCreate(&mCtx->mutex);
		init_to_P0_unicode(mCtx);
	}

	return 0;
}

int GxMagazine_Close(void)
{
	int i = 0, j = 0;

	if (!mCtx) return 0;

	GxCore_MutexLock(mCtx->mutex);
	for (i = 0; i < MAX_MAJOR_PAGE; i++) {
		for (j = 0; j < MAX_MINOR_PAGE; j++) {
			if (mCtx->savePage[i][j]) {
				vbi_unref_page(mCtx->savePage[i][j]);
				av_free(mCtx->savePage[i][j]);
				mCtx->savePage[i][j] = NULL;
			}
		}
	}

	if (mCharset) {
		av_free(mCharset);
		mCharset = NULL;
	}

	if(mCtx->defPage) {
		av_free(mCtx->defPage);
		mCtx->defPage = NULL;
	}
	GxCore_MutexUnlock(mCtx->mutex);

	GxCore_MutexDelete(mCtx->mutex);
	av_free(mCtx);
	mCtx = NULL;

	return 0;
}

int GxMagezine_SetCallBack(void* priv, GxMagazineShowCB show, GxMagazineSyncCB sync)
{
	mCtx->showCB = show;
	mCtx->syncCB = sync;
	mCtx->priv   = priv;

	return 0;
}

int GxMagazine_SetFirstPage(unsigned char magNum, unsigned char pageNo)
{
	unsigned int pgno = switch_page_no(((magNum << 8) | pageNo));

	if (mCtx == NULL) return -1;

	if (pgno == 0) {
		gxlogd("warning: %s %d, (pgno: 0x%03x)\n",
				__FUNCTION__, __LINE__, ((magNum << 8) | pageNo));
		return -1;
	}

	mCtx->firstPgNo = pgno;

	return 0;
}

int GxMagazine_GetFirstPage(unsigned int *pgno)
{
	if (mCtx == NULL) return -1;

	if (pgno)
		*pgno = mCtx->firstPgNo;

	return 0;
}

int GxMagazine_GetCurrentPage(unsigned int *pgno)
{
	if (mCtx == NULL) return -1;

	if (pgno)
		*pgno = mCtx->curPgNo;

	return 0;
}

int GxMagazine_GetCurrentSubPage(unsigned int *subno)
{
	if (mCtx == NULL) return -1;

	if (subno)
		*subno = mCtx->curSubNo;

	return 0;
}

int GxMagazine_AddPage(vbi_page *page, GxMagazine_Type type)
{
	unsigned int pgno = switch_page_no(page->pgno);
	unsigned int suno = switch_sub_no(page->subno);
	unsigned int freepage = 0;

	if (mCtx == NULL) return -1;

	if (pgno == 0) {
		gxlogd("warning: %s %d, (pgno: 0x%03x)\n",
				__FUNCTION__, __LINE__, page->pgno);
		return -1;
	}

	GxCore_MutexLock(mCtx->mutex);
	if (mCtx->savePage[pgno][suno] == NULL)
		alloc_sub_page(mCtx, pgno, suno);
	else
		vbi_unref_page(mCtx->savePage[pgno][suno]);
	memcpy(mCtx->savePage[pgno][suno], page, sizeof(vbi_page));
	mCtx->saveSubNo[pgno] = suno;
	mCtx->c6Page[pgno][suno] = (type == GX_MAGAZINE_C6) ? TRUE : FALSE;

	if (mCtx->curPgNo == pgno) {
		if (mCtx->autoSearch == 1) {
			mCtx->curPage          = mCtx->savePage[pgno][suno];
			mCtx->curSubNo         = suno;
			mCtx->refresh = 1;
		} else if (mCtx->curSubNo == suno) {
			mCtx->curPage = mCtx->savePage[pgno][suno];
			mCtx->refresh = 1;
		}
	}

	if (type == GX_MAGAZINE_C11) {
		if (mCtx->savePage[pgno][suno])
			mCtx->loadPage = mCtx->savePage[pgno][suno];

		if (page_to_P0_unicode(mCtx, page, P0_REGION1_POS) != 0) {
			//加载页面区域，当前页不存在需要刷新
			unsigned char refresh = (mCtx->curPage) ? 0 : 1;
			if (refresh) {
				mCtx->region1Refresh++;
				if (mCtx->region1Refresh >= P0_REGION1_SHOW) {
					if (debug_teletext_magazine)
						gxlogd("[%d], refresh\n", __LINE__);
					mCtx->refresh = 1;
					mCtx->region1Refresh = 0;
				}
			}
		}

		if (page_check_P0_unicode(page, P0_REGION3_POS)) {
			if (page_to_P0_unicode(mCtx, page, P0_REGION3_POS) != 0) {
				//时间区域，只要改变需要刷新
				if (debug_teletext_magazine)
					gxlogd("[%d], refresh\n", __LINE__);
				mCtx->refresh = 1;
			}
		}
	}
	else if (type == GX_MAGAZINE_C6)
	{
		if (mCtx->c6Page[mCtx->curPgNo][0]) {
			if (debug_teletext_magazine)
				gxlogd("[%d], refresh\n", __LINE__);
			mCtx->refresh = 1;
		}
	}

	if (freepage == 1)
		vbi_unref_page(page);

	GxCore_MutexUnlock(mCtx->mutex);

	return 0;
}

int GxMagazine_SetPageLang(GX_SUBTITLE_PGLANG *pglang)
{
	int i    = 0;

	if (pglang == NULL) return -1;

	if (mCharset == NULL) {
		mCharset = (vbi_page_charset *)av_mallocz(sizeof(vbi_page_charset));
		if (mCharset == NULL) return -1;
	}

	mCharset->count = pglang->count;
	for (i = 0; i < pglang->count; i++) {
		mCharset->page[i].pgno    = ((pglang->page[i].comp_page_id ? : 8) << 8 | pglang->page[i].anci_page_id);
		mCharset->page[i].charset = GxTeletext_FindRegion((const char *)(pglang->page[i].pglang));
	}

	if (mCtx && mCtx->defPage && mCtx->defPage->vbi)
		vbi_teletext_set_pgcharset(mCtx->defPage->vbi, mCharset);

	return 0;
}

int GxMagazine_JumpPage(unsigned int pgno)
{
	unsigned int subno = 0;

	if (mCtx == NULL) return -1;

	if (pgno == mCtx->curPgNo) return 0;

	if (pgno < MIN_MAJOR_PAGE || pgno >= MAX_MAJOR_PAGE) {
		gxlogd("warning: %s %d, (pgno jump %d, first %d, max %d)\n",
				__FUNCTION__, __LINE__, pgno, MIN_MAJOR_PAGE, MAX_MAJOR_PAGE);
		return -1;
	}

	GxCore_MutexLock(mCtx->mutex);
	mCtx->autoSearch = 1;
	if (mCtx->curPage) {
		mCtx->lastPage  = mCtx->curPage;
		mCtx->lastPgNo  = mCtx->curPgNo;
		mCtx->lastSubNo = mCtx->curSubNo;
	}
	for (subno = 0; subno < MAX_MINOR_PAGE; subno++) {
		if (mCtx->savePage[pgno][subno])
			break;
	}
	if (subno >= MAX_MINOR_PAGE)
		subno = mCtx->saveSubNo[pgno];
	mCtx->curPage   = mCtx->savePage[pgno][subno];
	mCtx->curPgNo   = pgno;
	mCtx->curSubNo  = subno;
	mCtx->clearC6   = 1;
	mCtx->p0ClearC6 = P0_C6_CLEARER;
	mCtx->refresh   = 1;
	GxCore_MutexUnlock(mCtx->mutex);

	return 0;
}

int GxMagazine_AutoJumpSubPage(void)
{
	if (mCtx == NULL) return -1;

	GxCore_MutexLock(mCtx->mutex);
	mCtx->autoSearch = 1;
	mCtx->curSubNo   = mCtx->saveSubNo[mCtx->curPgNo];
	mCtx->curPage    = mCtx->savePage[mCtx->curPgNo][mCtx->curSubNo];
	mCtx->refresh    = 1;
	GxCore_MutexUnlock(mCtx->mutex);

	return 0;
}

int GxMagazine_JumpSubPage(unsigned int subno)
{
	if (mCtx == NULL) return -1;

	if (subno >= MAX_MINOR_PAGE) {
		gxlogd("warning: %s %d, (pgno: 0x%03x)\n", __FUNCTION__, __LINE__, subno);
		return -1;
	}

	GxCore_MutexLock(mCtx->mutex);
	if (mCtx->savePage[mCtx->curPgNo][subno]) {
		mCtx->curPage  = mCtx->savePage[mCtx->curPgNo][subno];
		mCtx->curSubNo = subno;
		mCtx->refresh  = 1;
		mCtx->autoSearch = 0;
	}
	GxCore_MutexUnlock(mCtx->mutex);

	return 0;
}

int GxMagazine_ShowString(char *str)
{
	if (mCtx == NULL) return -1;

	if (str == NULL) return -1;

	GxCore_MutexLock(mCtx->mutex);
	str_to_P0_unicode(mCtx, str, P0_REGION0_POS);
	mCtx->refresh = 1;
	mCtx->p0ClearC6 = P0_C6_CLEARER;
	GxCore_MutexUnlock(mCtx->mutex);

	return 0;
}

int GxMagazine_ShowTimer(void)
{
	if (mCtx == NULL) return -1;

	GxCore_MutexLock(mCtx->mutex);
	if (mCtx->refresh) {
		if (mCtx->curPage) {
			//跳转页存在
			if (mCtx->showCB) {
				unsigned char pageC6 = mCtx->c6Page[mCtx->curPgNo][mCtx->curSubNo];
				while (pageC6) { //C6页面
					if (mCtx->p0ClearC6) {
						if (mCtx->loadPage) {
							page_to_P0_unicode(mCtx, mCtx->curPage, P0_REGION1_POS);
							page_to_P0_text(mCtx->loadPage, mCtx->curPage);
							P0_to_page_unicode(mCtx, mCtx->curPage, P0_REGION1_POS);
						} else {
							if (debug_teletext_magazine)
								gxlogd("%s %d: Draw Not\n", __FUNCTION__, __LINE__);
							mCtx->refresh = 0;
							GxCore_MutexUnlock(mCtx->mutex);
							return 0;
						}
					}

					if (mCtx->clearC6) {
						if (debug_teletext_magazine)
							gxlogd("%s %d: Draw P0_2\n", __FUNCTION__, __LINE__);
						fill_page_p24p25(mCtx, mCtx->curPage);
						P0_to_page_unicode(mCtx, mCtx->curPage, P0_REGION0_POS);
						P0_to_page_unicode(mCtx, mCtx->curPage, P0_REGION3_POS);
						fill_page_p0(mCtx, mCtx->curPage);
						mCtx->showCB(mCtx->priv, mCtx->curPage, GX_MAGAZINE_DRAW_P0_2);
						mCtx->showDefPage = false;
					} else if (mCtx->p0ClearC6) {
						if (debug_teletext_magazine)
							gxlogd("%s %d: Draw page_1\n", __FUNCTION__, __LINE__);
						fill_page_p24p25(mCtx, mCtx->curPage);
						P0_to_page_unicode(mCtx, mCtx->curPage, P0_REGION0_POS);
						P0_to_page_unicode(mCtx, mCtx->curPage, P0_REGION3_POS);
						fill_page_p0(mCtx, mCtx->curPage);
						mCtx->showCB(mCtx->priv, mCtx->curPage, GX_MAGAZINE_DRAW_PAGE_1);
						mCtx->showDefPage = false;
					}


					if(mCtx->syncCB) {
						if (mCtx->syncCB(mCtx->priv)) {
							mCtx->clearC6 = 0;
							break;
						}
					}
					GxCore_MutexUnlock(mCtx->mutex);
					//延时不能加锁，否则切页面很慢
					GxCore_ThreadDelay(100);
					GxCore_MutexLock(mCtx->mutex);
					pageC6 = mCtx->c6Page[mCtx->curPgNo][mCtx->curSubNo];
				}

				if (mCtx->curPage)
				{
					if (pageC6 && mCtx->p0ClearC6) {
						if (debug_teletext_magazine)
							gxlogd("%s %d: Draw page_1\n", __FUNCTION__, __LINE__);
						fill_page_p24p25(mCtx, mCtx->curPage);
						P0_to_page_unicode(mCtx, mCtx->curPage, P0_REGION0_POS);
						P0_to_page_unicode(mCtx, mCtx->curPage, P0_REGION3_POS);
						fill_page_p0(mCtx, mCtx->curPage);
						mCtx->showCB(mCtx->priv, mCtx->curPage, GX_MAGAZINE_DRAW_PAGE_1);
						mCtx->showDefPage = false;
						mCtx->p0ClearC6--;
					} else if (pageC6) {
						if (debug_teletext_magazine)
							gxlogd("%s %d: Draw page_0\n", __FUNCTION__, __LINE__);
						P0_to_page_unicode(mCtx, mCtx->curPage, P0_REGION0_POS);
						P0_to_page_unicode(mCtx, mCtx->curPage, P0_REGION3_POS);
						mCtx->showCB(mCtx->priv, mCtx->curPage, GX_MAGAZINE_DRAW_PAGE_0);
						mCtx->showDefPage = false;
					} else {
						if (debug_teletext_magazine)
							gxlogd("%s %d: Draw page_0\n", __FUNCTION__, __LINE__);
						fill_page_p24p25(mCtx, mCtx->curPage);
						P0_to_page_unicode(mCtx, mCtx->curPage, P0_REGION0_POS);
						P0_to_page_unicode(mCtx, mCtx->curPage, P0_REGION3_POS);
						fill_page_p0(mCtx, mCtx->curPage);
						mCtx->showCB(mCtx->priv, mCtx->curPage, GX_MAGAZINE_DRAW_PAGE_0);
						mCtx->showDefPage = false;
					}
				}
			}
			mCtx->refresh = 0;
		} else if (mCtx->lastPage) {
			//跳转页不存在
			if (mCtx->loadPage) {
				if (debug_teletext_magazine)
					gxlogd("%s %d: Draw page_0\n", __FUNCTION__, __LINE__);
				page_to_P0_text(mCtx->loadPage, mCtx->lastPage);
				P0_to_page_unicode(mCtx, mCtx->lastPage, P0_REGION0_POS);
				P0_to_page_unicode(mCtx, mCtx->lastPage, P0_REGION3_POS);
				if (mCtx->showCB) {
					fill_page_p24p25(mCtx, mCtx->lastPage);
					fill_page_p0(mCtx, mCtx->lastPage);
					mCtx->showCB(mCtx->priv, mCtx->lastPage, GX_MAGAZINE_DRAW_PAGE_0);
					mCtx->showDefPage = false;
				}
			}
			mCtx->refresh = 0;
		} else if (mCtx->loadPage) {
			//跳转页不存在
			//没有显示过页面,P0行不是空格字符,防止页面闪烁
			if (mCtx->curPgNo != 0 && mCtx->loadPage) {
				if (debug_teletext_magazine)
					gxlogd("%s %d: Draw page_0\n", __FUNCTION__, __LINE__);
				P0_to_page_unicode(mCtx, mCtx->loadPage, P0_REGION0_POS);
				if (mCtx->showCB) {
					fill_page_p0(mCtx, mCtx->loadPage);
					page_to_P0_text(mCtx->loadPage, mCtx->defPage);
					memcpy(mCtx->defPage->color_map, mCtx->loadPage->color_map, sizeof(mCtx->loadPage->color_map));
					mCtx->showCB(mCtx->priv, mCtx->defPage, GX_MAGAZINE_DRAW_PAGE_0);
					mCtx->showDefPage = false;
				}
			}
			mCtx->refresh = 0;
		}
	}
	GxCore_MutexUnlock(mCtx->mutex);

	return 0;
}

int GxMagazine_BackwardSearchPage(unsigned int *pgno)
{
	unsigned int tmpPgNo = *pgno, i = 0;

	if (mCtx == NULL) return -1;

	GxCore_MutexLock(mCtx->mutex);
	while (1) {
		tmpPgNo++;
		tmpPgNo = (tmpPgNo >= MAX_MAJOR_PAGE) ? mCtx->firstPgNo : tmpPgNo;
		if (tmpPgNo == *pgno)
			break;
		if (mCtx->saveSubNo[tmpPgNo] != 0xFF)
			break;
		for (i = 0; i < MAX_MINOR_PAGE; i++) {
			if (mCtx->savePage[tmpPgNo][i])
				break;
		}
		if (i < MAX_MINOR_PAGE)
			break;
	}
	GxCore_MutexUnlock(mCtx->mutex);

	*pgno = tmpPgNo;

	return 0;
}

int GxMagazine_ForwardSearchPage(unsigned int *pgno)
{
	unsigned int tmpPgNo = *pgno, i = 0;

	if (mCtx == NULL) return -1;

	GxCore_MutexLock(mCtx->mutex);
	while (1) {
		tmpPgNo--;
		tmpPgNo = ((tmpPgNo < MIN_MAJOR_PAGE) || (tmpPgNo >= MAX_MAJOR_PAGE)) ? (MAX_MAJOR_PAGE - 1): tmpPgNo;
		if (tmpPgNo == *pgno)
			break;
		if (mCtx->saveSubNo[tmpPgNo] != 0xFF)
			break;
		for (i = 0; i < MAX_MINOR_PAGE; i++) {
			if (mCtx->savePage[tmpPgNo][i])
				break;
		}
		if (i < MAX_MINOR_PAGE)
			break;
	}
	GxCore_MutexUnlock(mCtx->mutex);

	*pgno = tmpPgNo;

	return 0;
}

int GxMagazine_BackwardSearchSubPage(unsigned int *subno)
{
	unsigned int tmpSubNo = 0;
	unsigned int flag = 0;

	if (mCtx == NULL) return -1;

	GxCore_MutexLock(mCtx->mutex);
	tmpSubNo = mCtx->autoSearch ? 0 : (*subno + 1);
	for (; tmpSubNo < MAX_MINOR_PAGE; tmpSubNo++) {
		if ((tmpSubNo == *subno) ||
				(mCtx->savePage[mCtx->curPgNo][tmpSubNo]))
			break;
	}
	if (tmpSubNo >= MAX_MINOR_PAGE)
		flag = 1;
	else
		*subno = tmpSubNo;
	GxCore_MutexUnlock(mCtx->mutex);

	return flag;
}

int GxMagazine_ForwardSearchSubPage(unsigned int *subno)
{
	unsigned int tmpSubNo = 0;
	unsigned int flag = 0;

	if (mCtx == NULL) return -1;

	GxCore_MutexLock(mCtx->mutex);
	tmpSubNo = mCtx->autoSearch ? MAX_MINOR_PAGE: *subno;
	while (1) {
		if (tmpSubNo == 0) {
			flag = 1;
			break;
		}
		tmpSubNo--;
		if ((tmpSubNo == *subno) ||
				(mCtx->savePage[mCtx->curPgNo][tmpSubNo])) {
			*subno = tmpSubNo;
			break;
		}
	}
	GxCore_MutexUnlock(mCtx->mutex);

	return flag;
}

int GxMagazine_GetSixLinkPgNo(unsigned int *pgno)
{
	unsigned int i = 0;

	if (mCtx == NULL) return -1;

	GxCore_MutexLock(mCtx->mutex);
	for (i = 0; i < MAX_PAGE_LINK; i++)
		pgno[i] = mCtx->linkPage[i];
	GxCore_MutexUnlock(mCtx->mutex);

	return 0;
}

int GxMagazine_ShowDefaultPage(vbi_decoder *vbi)
{
#define DEFAULT_ROW    (13)
#define DEFAULT_COLUMN (13)
	vbi_page *page = NULL;
	int i = 0, j = 0;

	if (mCtx == NULL) return -1;

	GxCore_MutexLock(mCtx->mutex);
	if (!mCtx->defPage)
		mCtx->defPage = (vbi_page *)av_mallocz(sizeof(vbi_page));
	page = mCtx->defPage;
	page->vbi            = vbi;
	page->rows           = 26;
	page->columns        = 41;
	page->pgno           = mCtx->firstPgNo;
	page->screen_color   = VBI_BLACK;
	page->screen_opacity = VBI_OPAQUE;
	page->color_map[VBI_BLACK] = 0xFF000000;
	page->color_map[VBI_WHITE] = 0xFFFFFFFF;
	for (i = 0; i < page->rows; i++) {
		for (j = 0; j < page->columns; j++) {
			page->text[i * page->columns + j].foreground = VBI_BLACK;
			page->text[i * page->columns + j].background = VBI_BLACK;
			page->text[i * page->columns + j].unicode    = ' ';
			page->text[i * page->columns + j].opacity    = 3;
		}
	}

	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 0].foreground = VBI_WHITE;
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 0].unicode = 'P';
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 1].foreground = VBI_WHITE;
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 1].unicode = 'l';
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 2].foreground = VBI_WHITE;
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 2].unicode = 'e';
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 3].foreground = VBI_WHITE;
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 3].unicode = 'a';
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 4].foreground = VBI_WHITE;
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 4].unicode = 's';
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 5].foreground = VBI_WHITE;
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 5].unicode = 'e';
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 6].unicode = ' ';
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 7].foreground = VBI_WHITE;
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 7].unicode = 'W';
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 8].foreground = VBI_WHITE;
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 8].unicode = 'a';
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 9].foreground = VBI_WHITE;
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 9].unicode = 'i';
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 10].foreground = VBI_WHITE;
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 10].unicode = 't';
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 11].unicode = ' ';
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 12].foreground = VBI_WHITE;
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 12].unicode = '.';
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 13].foreground = VBI_WHITE;
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 13].unicode = '.';
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 14].foreground = VBI_WHITE;
	page->text[DEFAULT_ROW * page->columns + DEFAULT_COLUMN + 14].unicode = '.';

	if (vbi != NULL && mCharset != NULL)
		vbi_teletext_set_pgcharset(vbi, mCharset);

	if (mCtx->showCB)
		mCtx->showCB(mCtx->priv, mCtx->defPage, GX_MAGAZINE_DRAW_PAGE_0);
	mCtx->showDefPage = true;

	GxCore_MutexUnlock(mCtx->mutex);

	return 0;
}

int GxMagazine_RefreshCurPage(void)
{
	if (mCtx == NULL) return -1;

	GxCore_MutexLock(mCtx->mutex);
	if (mCtx->showDefPage)
		mCtx->showCB(mCtx->priv, mCtx->defPage, GX_MAGAZINE_DRAW_PAGE_0);
	else
		mCtx->refresh = 1;
	GxCore_MutexUnlock(mCtx->mutex);

	return 0;
}

int GxMagazine_VBIDebugChar(GX_SUBTITLE_VBI_DCHAR *dchar)
{
	vbi_dchar xdchar;

	xdchar.row     = dchar->row;
	xdchar.column  = dchar->column;
	xdchar.pgno    = switch_pageno_10to16(dchar->pgno);
	xdchar.subno   = switch_pageno_10to16(dchar->subno);
	xdchar.unicode = dchar->unicode;
	vbi_teletext_debug_char(&xdchar);

	return 0;
}

int GxMagazine_VBIModifyChar(GX_SUBTITLE_VBI_MCHAR *mchar)
{
	unsigned int i = 0;
	vbi_mchar xmchar;

	xmchar.s = mchar->char_set;
	xmchar.n = mchar->sub_set;
	xmchar.c = mchar->char_val;
	for (i = 0; i < 20; i++)
		xmchar.pic[i] = mchar->char_pic[i];

	vbi_teletext_modify_char(&xmchar, 1);

	return 0;
}
