#include "../avutil/bprintf.h"
#include "avformat.h"
#include "parser.h"
#include "../avutil/avstring.h"
#include "memmgr.h"
#include "tree.h"
#include "parser.h"

typedef struct
{
	int cont_mark; // consecutive same text string.
	char *id;
	char *style;
	char *region;
	char *ttml_id;
	int64_t begin;
	int64_t end;
	char *text;
} ttml_body_element;

typedef struct
{
	char *id;
	char *origin;
	char *extent;
	char *displayAlign;
	char *textAlign;
	char *fontSize;
	char *fontFamily;
	char *overflow;
} ttml_head_loyout;

typedef struct
{
	char *cellResolution;
	char *pixelAspectRatio;
	char *tickRate;
	char *extent;
	char *space;
	char *lang;
	char *body_style;
	int n_head_loyout;
	ttml_head_loyout **head_loyout;
	int n_ttml_body;
	ttml_body_element **ttml_body;
}TtmlContext;

int ttml_parse(TtmlContext *c, char *ttml_buf, int ttml_size);
void close_ttml_context(TtmlContext *c);
