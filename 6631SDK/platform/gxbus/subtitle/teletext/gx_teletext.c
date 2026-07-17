#include "gxcore.h"
#include "gx_mem.h"
#include "gx_teletext.h"
#include "ttx_dec.h"
#include "libzvbi/libzvbi.h"
#include "gx_subtitle_setting.h"
#include "gx_magazine.h"
#include "gx_subtitle_config.h"
#include "gx_subtitle_show_text.h"
#include "gx_subtitle_show_pixel.h"

#define MAX_SLIES 64
#define BITMAP_CHAR_WIDTH  12
#define BITMAP_CHAR_HEIGHT 10
#define BITMAP_PALETTE_SIZE 1024
#define TTX_RGBA(r,g,b,a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define VBI_R(rgba)   (((rgba) >> 0) & 0xFF)
#define VBI_G(rgba)   (((rgba) >> 8) & 0xFF)
#define VBI_B(rgba)   (((rgba) >> 16) & 0xFF)
#define VBI_A(rgba)   (((rgba) >> 24) & 0xFF)
#define VBI_NB_COLORS 40
#define VBI_FULL_TRANSPARENT_C  (2 * VBI_NB_COLORS + 1)
#define VBI_SEMI_TRANSPARENT_C  (2 * VBI_NB_COLORS + 2)
#define VBI_WELL_TRANSPARENT_C  (2 * VBI_NB_COLORS + 3)
#define VBI_USER_TRANSPARENT_C  (3 * VBI_NB_COLORS)
#define TEXT_MAXSZ    (25 * (56 + 1) * 4 + 2)

typedef enum {
	GX_TELETEXT_ARGB = 0x0,
	GX_TELETEXT_AYUV = 0x1,
	GX_TELETEXT_TEXT = 0x2
} GxTeletextFormat;

typedef struct {
	GxTeletext_Stream stream;
	vbi_decoder      *vbi;
#ifdef GXDBG
	vbi_export       *ex;
#endif
	vbi_sliced       sliced[MAX_SLIES];
	TTX_EBU          ebu;
	handle_t         show_handle;
	GxTeletextFormat show_format;
	GxTeletext_CodeFormat format;
	GxTeletext_SyncState  (*sync_func)(void *priv, int64_t pts);
	void                   *sync_priv;
	int64_t                 sync_pts;
	int              resx, resy;
} GxTeletextContext;

typedef struct {
	char lang[4];
	int  region;
} GxTeletextLang;

GxTeletextLang vbiLang[] = {
	{"eng",  0},
	{"ger",  1}, {"deu",  1},
	{"swe",  2}, {"fin",  2}, {"hun",  2}, {"nor",  2}, {"dan",  2},
	{"ita",  3},
	{"fra",  4}, {"fre",  4},
	{"por",  5}, {"spa",  5}, {"esl",  5},
	{"cze",  6}, {"ces",  6}, {"cat",  6}, {"slo",  6},
	{"pol",  8},
	{"tur", 22},
	{"scr", 29},
	{"rum", 31}, {"ron", 31}, {"rom", 31},
	{"scc", 32}, {"srp", 32}, {"mac", 32}, {"mkd", 32},
	{"est", 34},
	{"lav", 35}, {"lit", 35},
	{"rus", 36}, {"bul", 36},
	{"ukr", 37},
	{"gre", 55}, {"ell", 55},
	{"fas", 68},
	{"ara", 71}, {"per", 71},
	{"heb", 87}
};

#define CHECK_CTX_NULL(ctx) do { \
	if (NULL == ctx) { \
		gxlogd("[error]: [%s] [%d] ctx is NULL\n", __FUNCTION__, __LINE__); \
		return -1; \
	} \
} while(0)

#ifdef TELETEXT_DRAW_TEXT
static int chop_spaces_utf8(const unsigned char* t, int len)
{
	t += len;
	while (len > 0) {
		if (*--t != ' ' || (len-1 > 0 && *(t-1) & 0x80))
			break;
		--len;
	}
	return len;
}
#endif

static int find_default_region(const char *lang)
{
	int region = 0xff;
	unsigned int i = 0;

	if (lang) {
		for (i = 0; i < sizeof(vbiLang)/sizeof(GxTeletextLang); i++) {
			if (strcasecmp(lang, vbiLang[i].lang) == 0)
				return vbiLang[i].region;
		}
		gxlogd("No (%s) lanague charset!! Default English!!", lang);
	}

	return region;
}

static void fix_transparency(GxTeletextContext *ctx,
		unsigned char *bitmap, vbi_page *page, int chop_top, int resx, int resy)
{
	int iy;

	// Hack for transparency, inspired by VLC code...
	for (iy = 0; iy < resy; iy++) {
		uint8_t *pixel = bitmap + iy * resx;
		vbi_char *vc = page->text + (iy / BITMAP_CHAR_HEIGHT + chop_top) * page->columns;
		vbi_char *vcnext = vc + page->columns;
		for (; vc < vcnext; vc++) {
			uint8_t *pixelnext = pixel + BITMAP_CHAR_WIDTH;
			switch (vc->opacity) {
			case VBI_TRANSPARENT_SPACE:
				memset(pixel, VBI_WELL_TRANSPARENT_C, BITMAP_CHAR_WIDTH);
				break;
			case VBI_OPAQUE:
				if (subtitleConfig.magAlpha.user) {
					for (; pixel < pixelnext; pixel++) {
						if (*pixel == vc->background)
							*pixel = *pixel + VBI_USER_TRANSPARENT_C;
					}
				}
				break;
			case VBI_SEMI_TRANSPARENT:
				if (subtitleConfig.bgColor.user) {
					for(; pixel < pixelnext; pixel++)
						if (*pixel == vc->background)
							*pixel = VBI_SEMI_TRANSPARENT_C;
				}
				break;
			case VBI_TRANSPARENT_FULL:
				for(; pixel < pixelnext; pixel++)
					if (*pixel == vc->background)
						*pixel = VBI_FULL_TRANSPARENT_C;
				break;
			}
			pixel = pixelnext;
		}
	}
}

#ifdef TELETEXT_DRAW_TEXT
static int gx_teletext_gen_text(GxTeletextContext *ctx, vbi_page *page, int chop_top)
{
	char *vbi_text = av_mallocz(TEXT_MAXSZ);
	int sz, i, first_line = 1;
	const char *in;
	GxSubtitleShowTextPage txtCtx;

	if (!vbi_text)
		return -1;

	sz = vbi_print_page_region(page, vbi_text, TEXT_MAXSZ-1, "UTF-8",
			TRUE, FALSE, 0, chop_top, page->columns, page->rows - chop_top);
	if (sz <= 0) {
		av_free(vbi_text);
		return -1;
	}
	vbi_text[sz] = '\0';
	memset(&txtCtx, 0, sizeof(GxSubtitleShowTextPage));

	in = vbi_text;
	for (;;) {
		int nl, sz;

		// skip leading spaces and newlines
		in += strspn(in, " \n");
		// compute end of row
		for (nl = 0; in[nl]; ++nl)
			if (in[nl] == '\n' && (nl==0 || !(in[nl-1] & 0x80)))
				break;

		if (!in[nl])
			break;

		// skip trailing space
		sz = chop_spaces_utf8((const unsigned char*)in, nl);
		if (!first_line) {
			txtCtx.lines.text[txtCtx.lines.lines] = (char*)av_mallocz(sz + 1);
			memcpy(txtCtx.lines.text[txtCtx.lines.lines], in, sz);
			txtCtx.lines.lines++;
		}
		first_line = 0;
		in += nl;
	}

	if (txtCtx.lines.lines == 0) {
		GxSubtitleShowTextClean(ctx->show_handle);
	} else {
		GxSubtitleShowFont   font;
		unsigned int color = 0;

		color = subtitleConfig.bgColor.user ?
			subtitleConfig.bgColor.color : subtitleConfig.defBGColor;
		GxSubtitleShowTextSetScreenColor(ctx->show_handle, color);

		font.color = subtitleConfig.ftColor.user ? subtitleConfig.ftColor.color : subtitleConfig.defFTColor;
		font.str   = subtitleConfig.ftLib.str;
		GxSubtitleShowTextSetFont(ctx->show_handle, &font);

		GxSubtitleShowTextDraw(ctx->show_handle, &txtCtx);
		for (i = 0; i < txtCtx.lines.lines; i++) {
			av_free(txtCtx.lines.text[i]);
		}
	}

	av_free(vbi_text);

	return 0;
}
#endif

static unsigned int fix_teletext_screen_color(vbi_page *page)
{
	unsigned int argb = 0;

	switch (page->screen_opacity) {
	case VBI_TRANSPARENT_FULL:
		argb |= (0x00 << 24);
		break;
	case VBI_SEMI_TRANSPARENT:
		argb |= (0x80 << 24);
		break;
	case VBI_OPAQUE:
	case VBI_TRANSPARENT_SPACE:
		argb |= (0xFF << 24);
		break;
	default:
		argb |= (0xFF << 24);
		break;
	}

	switch (page->screen_color) {
	case VBI_BLACK:
		argb |= 0x000000;
		break;
	case VBI_RED:
		argb |= 0xFF0000;
		break;
	case VBI_GREEN:
		argb |= 0x00FF00;
		break;
	case VBI_YELLOW:
		argb |= 0xFFFF00;
		break;
	case VBI_BLUE:
		argb |= 0x0000FF;
		break;
	case VBI_MAGENTA:
		argb |= 0xFF00FF;
		break;
	case VBI_CYAN:
		argb |= 0x00FFFF;
		break;
	case VBI_WHITE:
		argb |= 0xFFFFFF;
		break;
	}

	return argb;
}

static int gx_magazine_gen_bitmap(void *priv, vbi_page *page, GxMagazine_Draw draw)
{
	GxTeletextContext *ctx = (GxTeletextContext *)priv;
	int resx = (page->columns * BITMAP_CHAR_WIDTH);
	int resy = (page->rows    * BITMAP_CHAR_HEIGHT);
	unsigned int i = 0, j = 0;
	unsigned char *bitmap = NULL;
	unsigned int *drawBuffer = NULL;
	unsigned int screen_color = 0;
	GxSubtitleShowDisplay pos;

	if (((resx <= 0) || (resx > 1920)) ||
			((resy <= 0) || (resy > 1080))) {
		gxlogd("warning: %s %d param error (resx %x, resy %x)\n",
				__FUNCTION__, __LINE__, resx, resy);
		return -1;
	}

	bitmap = (unsigned char *)av_mallocz(resx * resy);
	if (bitmap == NULL) {
		gxlogd("warning: %s %d malloc fail (resx %x, resy %x)\n",
				__FUNCTION__, __LINE__, resx, resy);
		if (ctx->show_handle)
			GxSubtitleShowPixelClean(ctx->show_handle);
		return -1;
	}

	drawBuffer = GxSubtitleShowPixelAllocARGB888Buffer(ctx->show_handle, resx, resy);
	if (drawBuffer == NULL) {
		av_free(bitmap);
		gxlogd("warning: %s %d ARGB8888 buffer alloc fail\n", __FUNCTION__, __LINE__);
		return -1;
	}

	vbi_draw_vt_page_region(page, VBI_PIXFMT_PAL8, bitmap, resx,
			0, 0, page->columns, page->rows, 1, 1);
	screen_color = fix_teletext_screen_color(page);
	fix_transparency(ctx, bitmap, page, 0, resx, resy);

	for (i = 0; i < resx; i++) {
		for (j = 0; j < resy; j++) {
			int ci = bitmap[i + resx * j];
			unsigned char a = 0x00, r = 0x00, g = 0x00, b = 0x00;

			if ((draw == GX_MAGAZINE_DRAW_PAGE_0) || (draw == GX_MAGAZINE_DRAW_PAGE_1)) {
				if ((ci == VBI_FULL_TRANSPARENT_C) || (ci == VBI_SEMI_TRANSPARENT_C)) {
					a = (screen_color >> 24) & 0xff;
					r = (screen_color >> 16) & 0xff;
					g = (screen_color >>  8) & 0xff;
					b = (screen_color      ) & 0xff;
				} else if (ci == VBI_WELL_TRANSPARENT_C) {
					a = 0x00;
					r = (screen_color >> 16) & 0xff;
					g = (screen_color >>  8) & 0xff;
					b = (screen_color      ) & 0xff;
				} else {
					if (ci >= VBI_USER_TRANSPARENT_C) {
						ci -= VBI_USER_TRANSPARENT_C;
						a = subtitleConfig.magAlpha.alpha & 0xff;
						r = VBI_R(page->color_map[ci]);
						g = VBI_G(page->color_map[ci]);
						b = VBI_B(page->color_map[ci]);
					} else {
						a = VBI_A(page->color_map[ci]);
						r = VBI_R(page->color_map[ci]);
						g = VBI_G(page->color_map[ci]);
						b = VBI_B(page->color_map[ci]);
					}
				}

				if (!subtitleConfig.magAlpha.user) {
					if (0x100 == page->pgno) {
						a = (j < BITMAP_CHAR_HEIGHT) ? 0xFF : a;
					} else {
						a = ((draw == GX_MAGAZINE_DRAW_PAGE_1) && (j < BITMAP_CHAR_HEIGHT)) ? 0xFF : a;
					}
				}
			} else if (draw == GX_MAGAZINE_DRAW_P0_2) {
				if (j < BITMAP_CHAR_HEIGHT) {
					a = 0xFF;
					r = VBI_R(page->color_map[ci]);
					g = VBI_G(page->color_map[ci]);
					b = VBI_B(page->color_map[ci]);
				} else {
					a = 0x00;
					r = (screen_color >> 16) & 0xff;
					g = (screen_color >>  8) & 0xff;
					b = (screen_color      ) & 0xff;
				}
			}

			drawBuffer[(i + resx * j)] = ((a << 24) | (r << 16) | (g << 8) | (b));
		}
	}

	if (subtitleConfig.bgColor.user) {
		unsigned int screen_alpha = 0;

		screen_color = subtitleConfig.bgColor.color;
		screen_alpha = ((screen_color >> 24 & 0xff) & subtitleConfig.magAlpha.alpha);
		screen_color = ((screen_color & 0xFFFFFF) | (screen_alpha << 24));
	} else if (subtitleConfig.magAlpha.user)
		screen_color = ((screen_color & 0xFFFFFF) | (subtitleConfig.magAlpha.alpha << 24));


	GxSubtitleShowPixelSetScreenColor(ctx->show_handle, screen_color);
	if (!subtitleConfig.disPos.user) {
		pos.xl = subtitleConfig.defDisPos.xl;
		pos.xr = subtitleConfig.defDisPos.xr;
		pos.yt = subtitleConfig.defDisPos.yt;
		pos.yb = subtitleConfig.defDisPos.yb;
	} else {
		pos.xl = subtitleConfig.disPos.xl;
		pos.xr = subtitleConfig.disPos.xr;
		pos.yt = subtitleConfig.disPos.yt;
		pos.yb = subtitleConfig.disPos.yb;
	}
	pos.autoPos = 0;
	GxSubtitleShowPixelSetDisplayPos(ctx->show_handle, &pos);
	GxSubtitleShowPixelDrawARGB8888Buffer(ctx->show_handle, drawBuffer, 1);
	GxSubtitleShowPixelFreeARGB8888Bufeer(ctx->show_handle, drawBuffer);
	av_free(bitmap);

	return 0;
}

static int gx_magazine_sync_pts(void *priv)
{
	GxTeletextContext *ctx = (GxTeletextContext *)priv;
	GxTeletext_SyncState state = GX_TELETEXT_SYNC_ERROR;

	if (ctx->sync_func)
		state = ctx->sync_func(ctx->sync_priv, ctx->sync_pts);

	if (state == GX_TELETEXT_SYNC_REPEAT)
		return 0;

	if (state == GX_TELETEXT_SYNC_SKIP)
		return 2;

	if (state == GX_TELETEXT_SYNC_ERROR)
		return 2;

	return 1;
}

static int gx_teletext_gen_bitmap(GxTeletextContext *ctx, vbi_page *page, int chop_top)
{
	int resx = (page->columns * BITMAP_CHAR_WIDTH);
	int resy = (page->rows - chop_top) * BITMAP_CHAR_HEIGHT;
	unsigned int i = 0, j = 0;
	vbi_char *vc    = page->text + (chop_top * page->columns);
	vbi_char *vcend = page->text + (page->rows * page->columns);
	unsigned char *bitmap = NULL;
	unsigned int bgColor  = 0;
	GxSubtitleShowDisplay pos;
	unsigned int *drawBuffer = NULL;

	for (; vc < vcend; vc++) {
		if (vc->opacity != VBI_TRANSPARENT_SPACE)
			break;
	}

	if (vc >= vcend) {
		if (ctx->show_handle)
			GxSubtitleShowPixelClean(ctx->show_handle);
		return 0;
	}

	bitmap = (unsigned char *)av_mallocz(resx * resy);
	if (bitmap == NULL) {
		gxloge("malloc fail (resx %x, resy %x)\n", resx, resy);
		if (ctx->show_handle)
			GxSubtitleShowPixelClean(ctx->show_handle);
		return -1;
	}

	drawBuffer = GxSubtitleShowPixelAllocARGB888Buffer(ctx->show_handle, resx, resy);
	if (drawBuffer == NULL) {
		av_free(bitmap);
		gxloge("warning: %s %d ARGB8888 buffer alloc fail\n", __FUNCTION__, __LINE__);
		if (ctx->show_handle)
			GxSubtitleShowPixelClean(ctx->show_handle);
		return -1;
	}

	if (subtitleConfig.bgColor.user)
		bgColor = subtitleConfig.bgColor.color;
	else
		bgColor = subtitleConfig.defBGColor;

	vbi_draw_vt_page_region(page, VBI_PIXFMT_PAL8, bitmap, resx,
			0, chop_top, page->columns, page->rows - chop_top, 1, 1);

	fix_transparency(ctx, bitmap, page, chop_top, resx, resy);
	for (i = 0; i < resx; i++) {
		for (j = 0; j < resy; j++) {
			int ci = bitmap[i + resx * j];
			unsigned char a = 0x00, r = 0x00, g = 0x00, b = 0x00;

			if (ci == VBI_WELL_TRANSPARENT_C) {
				r = 0x00;
				g = 0x00;
				b = 0x00;
				a = 0x00;
			} else if (ci == VBI_SEMI_TRANSPARENT_C) {
				r = (bgColor >> 16) & 0xff;
				g = (bgColor >> 8 ) & 0xff;
				b = (bgColor      ) & 0xff;
				a = (bgColor >> 24) & 0xff;
			} else {
				r = VBI_R(page->color_map[ci]);
				g = VBI_G(page->color_map[ci]);
				b = VBI_B(page->color_map[ci]);
				a = VBI_A(page->color_map[ci]);
			}
			drawBuffer[(i + resx * j)] = ((a << 24) | (r << 16) | (g << 8) | (b));
		}
	}

	if ((resx != ctx->resx) || (resy != ctx->resy)) {
		GxSubtitleShowPixelClean (ctx->show_handle);
		ctx->resx = resx;
		ctx->resy = resy;
	}

	GxSubtitleShowPixelSetScreenColor(ctx->show_handle, 0x00000000);
	if (!subtitleConfig.disPos.user) {
		pos.xl = subtitleConfig.defDisPos.xl;
		pos.xr = subtitleConfig.defDisPos.xr;
		pos.yt = subtitleConfig.defDisPos.yt;
		pos.yb = subtitleConfig.defDisPos.yb;
	} else {
		pos.xl = subtitleConfig.disPos.xl;
		pos.xr = subtitleConfig.disPos.xr;
		pos.yt = subtitleConfig.disPos.yt;
		pos.yb = subtitleConfig.disPos.yb;
	}
	pos.autoPos = 0;
	GxSubtitleShowPixelSetDisplayPos(ctx->show_handle, &pos);
	GxSubtitleShowPixelDrawARGB8888Buffer(ctx->show_handle, drawBuffer, 0);
	GxSubtitleShowPixelFreeARGB8888Bufeer(ctx->show_handle, drawBuffer);
	av_free(bitmap);

	return 0;
}

static void gx_teletext_handler(vbi_event *ev, void *user_data)
{
	GxTeletextContext *ctx = (GxTeletextContext *)user_data;
	vbi_page page;
	int res = 0, flags = 0;
	int page_no = (((ctx->stream.comp_page_id ? : 8) << 8) | ctx->stream.anci_page_id);
	char *lang = NULL;
	vbi_page_type vpt;
	int chop_top = 0;
	vbi_subno subno;

	ev->ev.ttx_page.subno = (ev->ev.ttx_page.subno >= 14) ? 0 : ev->ev.ttx_page.subno;
	if ((ctx->format != GX_TELETEXT_MAG) &&
			(ev->ev.ttx_page.pgno != page_no))
		return;

	res = vbi_fetch_vt_page(ctx->vbi, &page,
			ev->ev.ttx_page.pgno,
			ev->ev.ttx_page.subno,
			VBI_WST_LEVEL_1p5, 25, TRUE);

	if (!res)
		return;

	flags = vbi_fetch_vt_flags(ctx->vbi,
			ev->ev.ttx_page.pgno, ev->ev.ttx_page.subno);

	vpt = vbi_classify_page(ctx->vbi, ev->ev.ttx_page.subno, &subno, &lang);
	chop_top = ((page.rows > 1) && (vpt == VBI_SUBTITLE_PAGE));

#ifdef GXDBG
	if (!vbi_export_stdio(ctx->ex, stderr, &page))
		gxlogd("failed: %s\n", vbi_export_errstr(ctx->ex));
#endif

	if (ctx->format == GX_TELETEXT_MAG) {
#ifdef GXDBG
		gxlogd("pgno: %03x, subno: %02x: ", page.pgno, page.subno);
		gxlogd("%s ", (flags & GX_MAGAZINE_C6 )?"C6 ":"   ");
		gxlogd("%s ", (flags & GX_MAGAZINE_C7 )?"C7 ":"   ");
		gxlogd("%s ", (flags & GX_MAGAZINE_C8 )?"C8 ":"   ");
		gxlogd("%s ", (flags & GX_MAGAZINE_C9 )?"C9 ":"   ");
		gxlogd("%s ", (flags & GX_MAGAZINE_C10)?"C10":"   ");
		gxlogd("%s ", (flags & GX_MAGAZINE_C11)?"C11":"   ");
		gxlogd("%s ", (flags & GX_MAGAZINE_C12)?"C12":"   ");
		gxlogd("%s ", (flags & GX_MAGAZINE_C13)?"C13":"   ");
		gxlogd("%s ", (flags & GX_MAGAZINE_C14)?"C14":"   ");
		gxlogd("\n");
#endif
		if (flags & GX_MAGAZINE_C6) {
			GxMagazine_AddPage(&page, GX_MAGAZINE_C6);
		}  else {
			GxMagazine_AddPage(&page, GX_MAGAZINE_C11);
		}
	} else {
#ifdef TELETEXT_DRAW_TEXT
		gx_teletext_gen_text(ctx, &page, chop_top);
#else
		gx_teletext_gen_bitmap(ctx, &page, chop_top);
#endif
		vbi_unref_page(&page);
	}

	return;
}

handle_t GxTeletext_Open(GxTeletext_Param* param)
{
	unsigned int maj, min, rev;
	GxTeletextContext *ctx = NULL;
	GxSubtitleShowDisplay pos;

	if (NULL == param) {
		gxlogd("[error]: [%s] [%d] param is NULL\n", __FUNCTION__, __LINE__);
		return -1;
	}

	ctx = (GxTeletextContext *)av_mallocz(sizeof(GxTeletextContext));
	CHECK_CTX_NULL(ctx);

	ctx->format      = param->format;
	ctx->show_format = GX_TELETEXT_ARGB;
	ctx->sync_func   = param->sync;
	ctx->sync_priv   = param->priv;
	if (!ctx->show_handle) {
		GxSubtitleShowCanvas cv;
		GxSubtitleShowScreen sr;
		unsigned int color = 0;

		memcpy(&cv, &subtitleConfig.canvas, sizeof(GxSubtitleShowCanvas));
		memcpy(&sr, &subtitleConfig.screen, sizeof(GxSubtitleShowScreen));
		if (!subtitleConfig.disPos.user) {
			pos.xl = subtitleConfig.defDisPos.xl;
			pos.xr = subtitleConfig.defDisPos.xr;
			pos.yt = subtitleConfig.defDisPos.yt;
			pos.yb = subtitleConfig.defDisPos.yb;
			pos.autoPos = 1;
		} else {
			pos.xl = subtitleConfig.disPos.xl;
			pos.xr = subtitleConfig.disPos.xr;
			pos.yt = subtitleConfig.disPos.yt;
			pos.yb = subtitleConfig.disPos.yb;
			pos.autoPos = 0;
		}
#ifdef TELETEXT_DRAW_TEXT
		GxSubtitleShowFont   font;

		ctx->show_handle = GxSubtitleShowTextOpen(&cv, &sr);

		color = subtitleConfig.bgColor.user ?
			subtitleConfig.bgColor.color : subtitleConfig.defBGColor;
		GxSubtitleShowTextSetScreenColor(ctx->show_handle, color);

		GxSubtitleShowTextSetDisplayPos(ctx->show_handle, &pos);

		font.color = subtitleConfig.ftColor.user ? subtitleConfig.ftColor.color : subtitleConfig.defFTColor;
		font.str   = subtitleConfig.ftLib.str;
		GxSubtitleShowTextSetFont(ctx->show_handle, &font);
#else
		ctx->show_handle = GxSubtitleShowPixelOpen(&cv, &sr);
		color = subtitleConfig.bgColor.user ? subtitleConfig.bgColor.color : subtitleConfig.defBGColor;
		GxSubtitleShowPixelSetScreenColor(ctx->show_handle, color);
		GxSubtitleShowPixelSetDisplayPos(ctx->show_handle, &pos);
		if (ctx->format == GX_TELETEXT_MAG) {
			GxMagazine_Open();
			GxMagezine_SetCallBack((void*)ctx,
					gx_magazine_gen_bitmap, gx_magazine_sync_pts);
		}
#endif
	}

	vbi_version(&maj, &min, &rev);
	gxlogd("[VBI Versions] %d.%d.%d\n", maj, min, rev);

	return (handle_t)ctx;
}

int32_t GxTeletext_StreamSet(handle_t handle, GxTeletext_Stream* stream)
{
	GxTeletextContext *ctx = (GxTeletextContext *)handle;

	CHECK_CTX_NULL(ctx);

	if (ctx->format == GX_TELETEXT_MAG)
		GxMagazine_SetFirstPage((unsigned char)stream->comp_page_id, (unsigned char)stream->anci_page_id);

	memcpy(&ctx->stream, stream, sizeof(GxTeletext_Stream));
	return 0;
}

int32_t  GxTeletext_Synchronize(handle_t handle, int32_t milliseconds)
{
	return 0;
}

int32_t  GxTeletext_Close(handle_t handle)
{
	GxTeletextContext *ctx = (GxTeletextContext *)handle;

	if (ctx->format == GX_TELETEXT_MAG)
		GxMagazine_Close();

	if (ctx->show_handle) {
#ifdef TELETEXT_DRAW_TEXT
		GxSubtitleShowTextClose(ctx->show_handle);
#else
		GxSubtitleShowPixelClose(ctx->show_handle);
#endif
		ctx->show_handle = 0;
	}

	CHECK_CTX_NULL(ctx);

	av_free(ctx);

	return 0;
}

int32_t GxTeletext_Start(handle_t handle)
{
	GxTeletextContext *ctx = (GxTeletextContext *)handle;
	unsigned int defaultRegion = 0;
	GX_SUBTITLE_LANG_FLAG force_flag = subtitleConfig.ttxLang.force_flag;

	CHECK_CTX_NULL(ctx);
#ifdef GXDBG
	{
		char *t;
		ctx->ex = vbi_export_new("text", &t);
	}
#endif

	if (!(ctx->vbi = vbi_decoder_new())) {
		gxlogd("[error]: [%s] [%d] vbi is NULL\n", __FUNCTION__, __LINE__);
		return -1;
	}

	defaultRegion = find_default_region((const char *)(subtitleConfig.ttxLang.str));
	gxlogd("[VBI Language] %s (%d)\n", subtitleConfig.ttxLang.str, defaultRegion);

	vbi_teletext_set_charset(ctx->vbi, defaultRegion, force_flag);

	if (!vbi_event_handler_add(ctx->vbi, VBI_EVENT_TTX_PAGE, gx_teletext_handler, ctx)) {
		gxlogd("[error]: [%s] [%d] vbi add event failed\n", __FUNCTION__, __LINE__);
		vbi_decoder_delete(ctx->vbi);
		ctx->vbi = NULL;
		return -1;
	}

	GxMagazine_ShowDefaultPage(ctx->vbi);

	return 0;
}

int32_t  GxTeletext_Stop(handle_t handle, int freeze)
{
	GxTeletextContext *ctx = (GxTeletextContext *)handle;

	CHECK_CTX_NULL(ctx);

	vbi_decoder_delete(ctx->vbi);
	ctx->vbi = NULL;

	if (ctx->show_handle) {
#ifdef TELETEXT_DRAW_TEXT
		if (ctx->show_format == GX_TELETEXT_TEXT)
			GxSubtitleShowTextClean(ctx->show_handle);
		else
			GxSubtitleShowPixelClean(ctx->show_handle);
#else
		GxSubtitleShowPixelClean(ctx->show_handle);
#endif
	}

	return 0;
}

int32_t  GxTeletext_Show(handle_t handle)
{
	GxTeletextContext *ctx = (GxTeletextContext *)handle;

	if (ctx->show_handle) {
#ifdef TELETEXT_DRAW_TEXT
		GxSubtitleShowTextShow();
#else
		GxSubtitleShowPixelShow();
#endif
	}
	return 0;
}

int32_t  GxTeletext_Clear(handle_t handle)
{
	GxTeletextContext *ctx = (GxTeletextContext *)handle;

	if (ctx->show_handle) {
#ifdef TELETEXT_DRAW_TEXT
		GxSubtitleShowTextClean(ctx->show_handle);
#else
		GxSubtitleShowPixelClean(ctx->show_handle);
#endif
	}
	return 0;
}

int32_t  GxTeletext_Hide(handle_t handle)
{
	GxTeletextContext *ctx = (GxTeletextContext *)handle;

	if (ctx->show_handle) {
#ifdef TELETEXT_DRAW_TEXT
		GxSubtitleShowTextHide();
#else
		GxSubtitleShowPixelHide();
#endif
	}
	return 0;
}

int32_t  GxTeletext_SendPesData(handle_t handle, unsigned char* packet, unsigned int packet_length, int64_t pts)
{
	unsigned int slices_count = 0, i = 0;
	GxTeletextContext *ctx = (GxTeletextContext *)handle;

	CHECK_CTX_NULL(ctx);

	if (packet_length == 0) {
		gxlogd("[error]: [%s] [%d] packet length zero\n", __FUNCTION__, __LINE__);
		return -1;
	}

	ctx->sync_pts = pts;
	memset(&ctx->ebu, 0, sizeof(TTX_EBU));
	ttx_dec(packet, packet_length, &ctx->ebu);

	while (slices_count < ctx->ebu.count) {
		unsigned int sc = ((ctx->ebu.count - slices_count) > MAX_SLIES) ? MAX_SLIES : (ctx->ebu.count - slices_count);

		for (i = 0; i < sc; i++) {
			int j = 0;
			unsigned char line_offset  = ctx->ebu.unit[i + slices_count].field.line_offset;
			unsigned char field_parity = ctx->ebu.unit[i + slices_count].field.field_parity;

			ctx->sliced[i].id = VBI_SLICED_TELETEXT_B;
			ctx->sliced[i].line = (line_offset > 0 ? (line_offset + (field_parity ? 0 : 32)) : 0);
			for (j = 0; j < 42; j++)
				ctx->sliced[i].data[j] = vbi_rev8(ctx->ebu.unit[i + slices_count].field.data_block[j]);
		}

		vbi_decode(ctx->vbi, ctx->sliced, sc, 0.0);
		slices_count += sc;
	}

	return 0;
}

int32_t GxTeletext_Timer(handle_t handle)
{
	GxTeletextContext *ctx = (GxTeletextContext *)handle;

	CHECK_CTX_NULL(ctx);

	if (ctx->format == GX_TELETEXT_MAG)
		GxMagazine_ShowTimer();

	return 0;
}

int32_t GxTeletext_FindRegion(const char *lang)
{
	int ret = 0xff;

	if (lang) {
		ret = find_default_region((const char *)(lang));
	}

	return ret;
}
