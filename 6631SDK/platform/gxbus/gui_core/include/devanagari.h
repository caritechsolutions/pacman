__BEGIN_DECLS

#include "gdi_font.h"

int check_devanagari(const unsigned int *unicode);
int devanagari_display_presentation(void *surface, const unsigned int *unicode, GAL_Rect *rect, int size, int color, int alignment, int flag);

__END_DECLS
