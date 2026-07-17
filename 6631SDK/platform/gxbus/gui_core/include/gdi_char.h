#ifndef __GDI_CHAR_H__
#define __GDI_CHAR_H__

#ifndef NO_OS
#include "gxcore.h"
#include "gui_private.h"
#else
#include "gdi_font.h"
#include "gxtype.h"
#endif /*NO_OS*/

__BEGIN_DECLS

gdi_font_prop* gdi_check_char(unsigned short c);
int gdi_check_iso6937(unsigned short c, unsigned short accent);
int gdi_get_charwidth(unsigned int c);
int gdi_get_charwidth_byname(char *name);
int gdi_get_subscriptwidth(unsigned int c, int sub_index);
int gdi_get_charheight(void);
int gdi_get_charwidth_with_accent(unsigned int c, unsigned int accent);
int gdi_draw_char_xpos(void* surface, int x, int y, int w, int h, unsigned int c, int color, int pos);
int gdi_draw_char_ypos(void* surface, int x, int y, int w, int h, unsigned int c, int color, int pos);
int gdi_draw_char(void *screen, int *x, int *y, unsigned int c, int color);
int gdi_draw_mark(void *screen, int x, int y, unsigned int c, int color);
int gdi_draw_char_byname(void *screen, int *x, int *y, char *name, int color);
int gdi_draw_sub_byname(void *screen, int *x, int *y, char *name, int color);
int gdi_draw_subscript(void *screen, int *x, int *y, unsigned int c, int color, int sub_index);
int gdi_draw_char_with_accent(void *screen,
                              int x,
                              int y,
                              unsigned int c,
                              unsigned int accent,
                              int color);
__END_DECLS

#endif



