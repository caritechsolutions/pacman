#ifndef __GDI_FONT_H__
#define __GDI_FONT_H__

#ifndef NO_OS
#include <stdio.h>
#include <stdlib.h>
#include "gal.h"
#include "gui_core.h"
#include "../../include/gdi.h"
#else
#include "gxtype.h"
#endif
#ifdef GUI_TTF
#include "gsub_api.h"
#endif /* GUI_TTF */
#include "gui_core.h"
#include "gal.h"

__BEGIN_DECLS

#define MAX_FONT_NAME			(10)

#define FILE_READ_BUF(fp, size, p)				memcpy(p, fp, size);\
											fp += size;

/*Text alignment flags, horizontal*/
#define GUI_TA_HORIZONTAL		(3<<0)
#define GUI_TA_LEFT			(0<<0)
#define GUI_TA_RIGHT			(1<<0)
#define GUI_TA_HCENTRE			(2<<0)

/*Text alignment flags, vertical*/
#define GUI_TA_VERTICAL	(3<<2)
#define GUI_TA_TOP		(0<<2)
#define GUI_TA_BOTTOM	(1<<2)
#define GUI_TA_BASELINE	(2<<2)
#define GUI_TA_VCENTRE	(3<<2)

extern GuiCore gui;

typedef struct {
  signed char	MinX;
  signed char	MinY;
  unsigned char XSize;
  unsigned char XDist;
  unsigned char BytesPerLine;
  const unsigned char * pData;
} gdi_charinfo;

typedef struct gdi_font_prop {
  unsigned int First;                                /* first character               */
  unsigned int Last;                                 /* last character                */
  const gdi_charinfo * paCharInfo;            /* address of first character    */
  const struct gdi_font_prop * pNext;        /* pointer to next */
} gdi_font_prop;

struct gdi_font {
    char font_name[MAX_FONT_NAME];
    int family;
    int xsize;
    int ysize;
    int font_size;
    bool hw_draw;
    void* font_data;
	char *filename;
#ifdef GUI_TTF
    gdi_font_prop *index_table;
#ifndef NO_OS
	GSUB_Info *info;
	GSUB_Opt *gsub_opt;
#endif /*NO_OS*/
#endif /* ndef GUI_TTF */
    struct gdi_font* next;
};

 #ifdef GUI_TTF

typedef struct _ttf_statistic_talbe{
    char *filename;
    gdi_font_prop *table;
}ttf_statistic_talbe;

#endif

enum gdi_font_family
{
    ASCII_ISO8859,
    GB_FAMILY,
    UNCODE_FAMILY,
    ARABIC,
    THAI,
    TTF_FONT_TYPE = 7
};

enum gdi_font_flag
{
	PARAGRAPH_MODE,
	SINGLELINE_MODE
};

#define MAX_STRING_SURFACE_WIDTH    (4000)

#define UTF8_0XXXXXXX  			(0x0)
#define UTF8_10XXXXXX			(0x80)
#define UTF8_110XXXXX			(0xC0)
#define UTF8_1110XXXX			(0xE0)
#define UTF8_11110XXX			(0xF0)
#define UTF8_111110XX			(0xF8)
#define UTF8_1111110X			(0xFC)
#define UTF8_11111110			(0xFE)

typedef struct _encoding_node{
	char *name;
	unsigned int (*table)[2];
	unsigned int *single_table;
	unsigned int src_single_word;
	unsigned int start;
	unsigned int end;
}encoding_node;

typedef int gdi_getlen(void *string);
typedef int gdi_get_lines(void *string, GAL_Rect *rect, GAL_Interval *interval);
typedef int gdi_draw_text(void *screen, void *string, GAL_Rect *rect, GAL_Interval *interval, int alignment, int color, int flag);
typedef int gdi_skip_line(void *p_start, void *string, GAL_Interval *interval, int line_width, int alignment);
typedef int gdi_roll_text(void *screen, void *string, GAL_Rect* rect, int pos, int color, int flag);

struct gdi_font_opt {
    char name[MAX_FONT_NAME];
    gdi_getlen* pGetLen;
    gdi_draw_text* pDrawText;
    gdi_skip_line* pSkipLine;
    gdi_roll_text* pRollText;
    gdi_get_lines* pGetLines;
};

int gdi_strlen(const char *string);
char* gdi_strcpy(char *dest, const char *src);
int gdi_font_load(GuiConfig *config, char * id, char *pFileName, int size, int style);
struct gdi_font *gdi_get_first_font(void);
int gdi_font_destroy(GuiConfig *config, struct gdi_font *pFont);
struct gdi_font* gxgdi_get_font(void);
void gxgdi_get_bearingheight(struct gdi_font *pfont, int code, int *yoffset, int *height);
void gxgdi_set_default_font(GuiConfig *config, struct gdi_font *pFont);
struct gdi_font* gxgdi_set_font(char *pFontName);
int gdi_get_font_height_bysize(char *fontfile, int size);
bool gxgdi_font_is_hwdraw(void);
void gxgdi_font_infomation_print(void);
struct gdi_font *gdi_font_get_byname(const unsigned char *name);
bool gxgdi_is_exist(unsigned short ch);
int gxgdi_draw_string_back_color(void *screen, void *string, GAL_Rect *rect, GAL_Interval *interval, int alignment, int color, int back_color, int flag);
int gxgdi_draw_string(void *screen, void *string, GAL_Rect *rect, GAL_Interval *interval, int alignment, int color, int flag);
int gdi_roll_string(void *screen, void *string, GAL_Rect* rect, int pos, int color, int flag);
int gdi_text_len(void *string);
int gdi_text_surfacewidth(void *string);
int gdi_text_skip_line(void *p_start, void *string, GAL_Interval *interval, int line_width, int alignment);
int gdi_text_get_lines(void *string, GAL_Rect *rect, GAL_Interval *interval);
int gdi_font_utf82unicode(unsigned char **pstr, unsigned int *unicode, unsigned int *accent, unsigned char *is_iso6937);
void gdi_arabic_multilines(char *src_string, char *dst_string, int width);
int gxgdi_is_arabic(const unsigned char *str);
int gxgdi_is_right2left(const unsigned char *str);
char *gxgdi_string_move(char *string, unsigned int offset, unsigned int *real_offset);
int gxgdi_string_num(char *string);

int gdi_encoding_init(void);
int gdi_set_local(char *name);
encoding_node *gdi_get_local(void);
char *gdi_iconv_bylocal(encoding_node *encoding, char *string);

void gxgdi_str_del_char(char *src, char c);
int gxgdi_font_thrshold_size(GuiConfig* config, char *fontfile, int size);

__END_DECLS

#endif

