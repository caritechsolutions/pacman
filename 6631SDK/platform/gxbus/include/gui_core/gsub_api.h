#ifndef __GSUB_API_H__
#define __GSUB_API_H__

#ifdef GUI_TTF


#include "gui_hash.h"
#include "ft2build.h"
#ifdef NDS_SUPPORT
#include "freetype.h"
#else
#include "freetype/freetype.h"
#endif
#endif /*GUI_TTF*/

#define MAX_COMPOUND_LIG            (10)
#define MAX_SUB_TAG                 (5)
/*GUI define struct to implement substitution for some ligature.*/
typedef enum{
	GLYPH_AUTOMATIC,
	GLYPH_UNASSIGNED,
	GLYPH_SIMPLE,
	GLYPH_LIGATURE,
	GLYPH_MARK,
	GLYPH_COMPONENT
}GlyphClass;

typedef enum {
	GSUB_SINLE_MODE,
	GSUB_LIGAT_MODE
} Transform_Mode;

typedef struct _CharPara {
	int charcode;
	char *name;
	GlyphClass type;
}CharPara;

typedef struct _GlyphInfo {
	int count;
	CharPara *glyph;
}GlyphInfo;

typedef struct _LigatureSub {
	char tag[MAX_SUB_TAG];
	int src_count;
	char *src_name[MAX_COMPOUND_LIG];
	int  src_code[MAX_COMPOUND_LIG];
	char *dst_name;
	int  dst_code;
}LigatureSub;

typedef struct _GSUB_Info {
	int count;
	LigatureSub *table;
	GlyphInfo *glyph_info;
#ifndef NO_OS
	hash_t    *isol_table;
	hash_t    *init_table;
	hash_t    *medi_table;
	hash_t    *fina_table;
#endif /*ndef NO_OS*/
}GSUB_Info;

#ifdef GUI_TTF
GSUB_Info *gsub_read(char *file, FT_Face face);
#endif /*GUI_TTF*/
void PrintGSUBInfomation(GSUB_Info *info);
void ArrangeHash(GSUB_Info *info);
char *_get_transformation_lig(unsigned int *uni_array,
			      unsigned int  prev_char,
			      unsigned int *offset,
			      Transform_Mode mode);
int _is_mark(unsigned int charcode);
int _is_mark_byname(char* name);

#ifndef NO_OS
#ifdef GUI_TTF
typedef GSUB_Info*     (*gsub_read_init)(char *file, FT_Face face);
#endif /*GUI_TTF*/
typedef void           (*gsub_print)(GSUB_Info *info);
typedef void           (*gsub_arrange)(GSUB_Info *info);
typedef char*          (*get_transformat)(unsigned int *uni_array, unsigned int prev_char, unsigned int *offset, Transform_Mode mode);
typedef int            (*is_mark_bycode)(unsigned int charcode);
typedef int            (*is_mark_byname)(char *name);

typedef struct _GSUB_Opt {
#ifdef GUI_TTF
	gsub_read_init  read_info;
#endif /*GUI_TTF*/
	gsub_print      print;
	gsub_arrange    arrange;
	get_transformat get_tans;
	is_mark_bycode  is_mark_code;
	is_mark_byname  is_mark_name;
} GSUB_Opt;

GSUB_Opt g_gsub_opt;
#endif /*NO_OS*/

#ifndef NO_OS
/**
 *
 * 注册GSUB接口，用于小语种
 *
 * \param void 无
 *
 * \return void 无
 */
void GXGDI_FontRegisterGSUB(void);
#endif /*NO_OS*/

#endif /*__GSUB_API_H__*/

