#ifndef __GSUB_PARA_H__
#define __GSUB_PARA_H__
#include "gsub_api.h"

#undef MAX_SUBTABLE_TYPES
#define MAX_SUBTABLE_TYPES			(4)

#define		intptr_t		long

typedef unsigned int		uint32;
typedef int					int32;
typedef short				int16;
typedef unsigned short		uint16;
typedef char				int8;
typedef unsigned char		uint8;
typedef uint32				unichar_t;
typedef intptr_t			intpt;

#if 0
#define GxCore_Malloc(size)              malloc(size)
#define GxCore_Mallocz(size)             calloc(1, size)
#define GxCore_Calloc(nmemb, size)       calloc(nmemb, size)
#define GxCore_Realloc(mem, size)        realloc(mem, size)
#define GxCore_Free(ptr)                 free(ptr)
#define GxCore_Strdup(ptr)               strdup(ptr)
#endif

#define MAX_LANG        4   /* If more than this we allocate more_langs in chunks of MAX_LANG */
struct scriptlanglist {
	uint32 script;
	uint32 langs[MAX_LANG];
	uint32 *morelangs;
	int lang_cnt;
	struct scriptlanglist *next;
};

typedef struct featurescriptlanglist {
	uint32 featuretag;
	struct scriptlanglist *scripts;
	struct featurescriptlanglist *next;
	unsigned int ismac: 1;  /* treat the featuretag as a mac feature/setting */
} FeatureScriptLangList;

struct lookup_subtable {
	char *subtable_name;
	char *suffix;           /* for gsub_single, used to find a default replacement */
	int16 separation, minkern;  /* for gpos_pair, used to guess default kerning values */
	struct otlookup *lookup;
	unsigned int unused: 1;
	unsigned int per_glyph_pst_or_kern: 1;
	unsigned int anchor_classes: 1;
	unsigned int vertical_kerning: 1;
	unsigned int ticked: 1;
	unsigned int kerning_by_touch: 1;   /* for gpos_pair, calculate kerning so that glyphs will touch*/
	unsigned int onlyCloser: 1;     /* for kerning classes */
	unsigned int dontautokern: 1;       /* for kerning classes */
	struct kernclass *kc;
	struct generic_fpst *fpst;
	struct generic_asm  *sm;
	/* Each time an item is added to a lookup we must place it into a */
	/*  subtable. If it's a kerning class, fpst or state machine it has */
	/*  a subtable all to itself. If it's an anchor class it can share */
	/*  a subtable with other anchor classes (merge with). If it's a glyph */
	/*  PST it may share a subtable with other PSTs */
	/* Note items may only be placed in lookups in which they fit. Can't */
	/*  put kerning data in a gpos_single lookup, etc. */
	struct lookup_subtable *next;
	int32 subtable_offset;
	int32 *extra_subtables;
	/* If a kerning subtable has too much stuff in it, we are prepared to */
	/*  break it up into several smaller subtables, each of which has */
	/*  an offset in this list (extra-subtables[0]==subtable_offset) */
	/*  the list is terminated by an entry of -1 */
};

typedef struct devicetab {
	uint16 first_pixel_size, last_pixel_size;       /* A range of point sizes to which this table applies */
	int8 *corrections;                  /* a set of pixel corrections, one for each point size */
} DeviceTable;

typedef struct kernclass {
	int first_cnt, second_cnt;      /* Count of classes for first and second chars */
	char **firsts;          /* list of a space separated list of char names */
	char **seconds;         /*  one entry for each class. Entry 0 is null */
							/*  and means everything not specified elsewhere */
	char **firsts_names; // We need to track the names of the classes in order to round-trip U. F. O. data.
	char **seconds_names;
	int *firsts_flags; // This tracks the storage format of the class in U. F. O. (groups.plist or features.fea) and whether it's a sin
	int *seconds_flags; // We also track the name format (@MMK or public.kern).
	struct lookup_subtable *subtable;
	uint16 kcid;            /* Temporary value, used for many things briefly */
	int16 *offsets;         /* array of first_cnt*second_cnt entries with 0 representing no data */
	int *offsets_flags;
	DeviceTable *adjusts;       /* array of first_cnt*second_cnt entries representing resolution-specific adjustments */
	struct kernclass *next;     // Note that, in most cases, a typeface needs only one struct kernclass since it can contain all classe
	int feature; // This indicates whether the kerning class came from a feature file. This is important during export.
} KernClass;

struct seqlookup {
	int seq;
	struct otlookup *lookup;
};

struct fpg { char *names, *back, *fore; };
struct fpc { int ncnt, bcnt, fcnt; uint16 *nclasses, *bclasses, *fclasses, *allclasses; };
struct fpv { int ncnt, bcnt, fcnt; char **ncovers, **bcovers, **fcovers; };
struct fpr { int always1, bcnt, fcnt; char **ncovers, **bcovers, **fcovers; char *replacements; };

struct fpst_rule {
	union {
		/* Note: Items in backtrack area are in reverse order because that's how the OT wants them */
		/*  they need to be reversed again to be displayed to the user */
		struct fpg glyph;
		struct fpc class;
		struct fpv coverage;
		struct fpr rcoverage;
	} u;
	int lookup_cnt;
	struct seqlookup *lookups;
};

typedef struct generic_fpst {
	uint16 /*enum possub_type*/ type;
	uint16 /*enum fpossub_format*/ format;
	struct lookup_subtable *subtable;
	struct generic_fpst *next;
	uint16 nccnt, bccnt, fccnt;
	uint16 rule_cnt;
	char **nclass, **bclass, **fclass;
	struct fpst_rule *rules;
	uint8 ticked;
	uint8 effectively_by_glyphs;
	char **nclassnames, **bclassnames, **fclassnames;
} FPST;

enum asm_type { asm_indic, asm_context, asm_lig, asm_simple=4, asm_insert,
	asm_kern=0x11 };
enum asm_flags { asm_vert=0x8000, asm_descending=0x4000, asm_always=0x2000 };

struct asm_state {
	uint16 next_state;
	uint16 flags;
	union {
		struct {
			struct otlookup *mark_lookup;   /* for contextual glyph subs (tag of a nested
											   lookup) */
			struct otlookup *cur_lookup;    /* for contextual glyph subs */
		} context;
		struct {
			char *mark_ins;
			char *cur_ins;
		} insert;
		struct {
			int16 *kerns;
			int kcnt;
		} kern;
	} u;
};

typedef struct generic_asm {        /* Apple State Machine */
	struct generic_asm *next;
	uint16 /*enum asm_type*/ type;
	struct lookup_subtable *subtable;   /* Lookup contains feature setting info */
	uint16 flags;   /* 0x8000=>vert, 0x4000=>r2l, 0x2000=>hor&vert */
	uint8 ticked;

	uint16 class_cnt, state_cnt;
	char **classes;
	struct asm_state *state;
} ASM;

enum otlookup_type {
	ot_undef = 0,           /* Not a lookup type */
	gsub_start         = 0x000,     /* Not a lookup type */
	gsub_single        = 0x001,
	gsub_multiple      = 0x002,
	gsub_alternate     = 0x003,
	gsub_ligature      = 0x004,
	gsub_context       = 0x005,
	gsub_contextchain  = 0x006,
	/* GSUB extension 7 */
	gsub_reversecchain = 0x008,
	/* mac state machines */
	morx_indic         = 0x0fd,
	morx_context       = 0x0fe,
	morx_insert        = 0x0ff,
	/* ********************* */
	gpos_start         = 0x100,     /* Not a lookup type */

	gpos_single        = 0x101,
	gpos_pair          = 0x102,
	gpos_cursive       = 0x103,
	gpos_mark2base     = 0x104,
	gpos_mark2ligature = 0x105,
	gpos_mark2mark     = 0x106,
	gpos_context       = 0x107,
	gpos_contextchain  = 0x108,
	/* GPOS extension 9 */
	kern_statemachine  = 0x1ff

	/* otlookup&0xff == lookup type for the appropriate table */
	/* otlookup>>8: 0=>GSUB, 1=>GPOS */
};


typedef struct otlookup {
	struct otlookup *next;
	enum otlookup_type lookup_type;
	uint32 lookup_flags;        /* Low order: traditional flags, High order: markset index, only meaningful if pst_usemarkfiltering set*/ 
	char *lookup_name;
	FeatureScriptLangList *features;
	struct lookup_subtable *subtables;
	unsigned int unused: 1; /* No subtable is used (call SFFindUnusedLookups before examining) */
	unsigned int empty: 1;  /* No subtable is used, and no anchor classes are used */
	unsigned int store_in_afm: 1;   /* Used for ligatures, some get stored */
			                        /*  'liga' generally does, but 'frac' doesn't */
	unsigned int needs_extension: 1;    /* Used during opentype generation */
	unsigned int temporary_kern: 1; /* Used when decomposing kerning classes into kern pairs for older formats */
	unsigned int def_lang_checked: 1;
	unsigned int def_lang_found: 1;
	unsigned int ticked: 1;
	unsigned int in_gpos: 1;
	unsigned int in_jstf: 1;
	unsigned int only_jstf: 1;
	int16 subcnt;       /* Actual number of subtables we will output */
	/* Some of our subtables may contain no data */
	/* Some may be too big and need to be broken up.*/
	/* So this field may be different than just counting the subtables */
	int lookup_index;       /* used during opentype generation */
	uint32 lookup_offset;
	uint32 lookup_length;
	char *tempname;
} OTLookup;

typedef struct anchorclass {
	char *name;         /* in utf8 */   
	struct lookup_subtable *subtable;   
	uint8 type;     /* anchorclass_type */
	uint8 has_base;
	uint8 processed, has_mark, matches, ac_num;
	uint8 ticked;
	struct anchorclass *next;
} AnchorClass;

struct language {   /* normal entry with lang tag 'dflt' */
	uint32 tag;
	uint32 offset;
	uint16 req;     /* required feature index. 0xffff for null */
	int fcnt;
	uint16 *features;
};

struct scripts {
	uint32 offset;
	uint32 tag;
	int langcnt;        /* the default language is included as a */
	struct language *languages;
};

struct feature {
	uint32 offset;
	uint32 tag;
	int lcnt;
	uint16 *lookups;
};

typedef void *gww_iconv_t;
#define iconv_t     gww_iconv_t

typedef struct enc {
    char *enc_name;
    int char_cnt;   /* Size of the next two arrays */
    int32 *unicode; /* unicode value for each encoding point */
    char **psnames; /* optional postscript name for each encoding point */
    struct enc *next;
    unsigned int builtin: 1;
    unsigned int hidden: 1;
    unsigned int only_1byte: 1;
    unsigned int has_1byte: 1;
    unsigned int has_2byte: 1;
    unsigned int is_unicodebmp: 1;
    unsigned int is_unicodefull: 1;
    unsigned int is_custom: 1;
    unsigned int is_original: 1;
    unsigned int is_compact: 1;
    unsigned int is_japanese: 1;
    unsigned int is_korean: 1;
    unsigned int is_tradchinese: 1;
    unsigned int is_simplechinese: 1;
    char iso_2022_escape[8];
    int iso_2022_escape_len;
    int low_page, high_page;
    char *iconv_name;   /* For compatibility to old versions we might use a different name from that used by iconv. */
    iconv_t *tounicode;
    iconv_t *fromunicode;
    int (*tounicode_func)(int);
    int (*fromunicode_func)(int);
    unsigned int is_temporary: 1;   /* freed when the map gets freed */
    int char_max;           /* Used by temporary encodings */
} Encoding;

struct lookup {
	uint16 type;
	uint32 flags;   /* low order 16 bits: traditional flags, high order: mark filtering set index if
					   pst_usemarkfilteringset */
	uint32 offset;
	int subtabcnt;
	int32 *subtab_offsets;
	OTLookup *otlookup;
};

extern struct opentype_feature_friendlynames {
    uint32 tag;
    char *tagstr;
    char *friendlyname;
    int masks;
} friendlies[];

typedef struct valdev {     /* Value records can have four associated device tables */
	DeviceTable xadjust;
	DeviceTable yadjust;
	DeviceTable xadv;
	DeviceTable yadv;
} ValDevTab;

struct vr {
	int16 xoff, yoff, h_adv_off, v_adv_off;
	ValDevTab *adjust;
};

enum possub_type { pst_null, pst_position, pst_pair,
	pst_substitution, pst_alternate,
	pst_multiple, pst_ligature,
	pst_lcaret /* must be pst_max-1, see charinfo.c*/,
	pst_max,
	/* These are not psts but are related so it's handly to have values for them */
	pst_kerning = pst_max, pst_vkerning, pst_anchors,
	/* And these are fpsts */
	pst_contextpos, pst_contextsub, pst_chainpos, pst_chainsub,
	pst_reversesub, fpst_max,
	/* And these are used to specify a kerning pair where the current */
	/*  char is the final glyph rather than the initial one */
	/* A kludge used when cutting and pasting features */
	pst_kernback, pst_vkernback
};

typedef struct generic_pst {
	unsigned int ticked: 1;
	unsigned int temporary: 1;      /* Used in afm ligature closure */
	/* enum possub_type*/ uint8 type;
	struct lookup_subtable *subtable;
	struct generic_pst *next;
	union {
		struct vr pos;
		struct { char *paired; struct vr *vr; } pair;
		struct { char *variant; } subs;
		struct { char *components; } mult, alt;
		struct { char *components; struct splinechar *lig; } lig;
		struct { int16 *carets; int cnt; } lcaret;  /* Ligature caret positions */
	} u;
} PST;

typedef struct splinechar {
	char *name;
	PST *possub;        /* If we are a ligature then this tells us what */
						/*  It may also contain a bunch of other stuff now */
	unsigned int glyph_class;
} SplineChar;

# ifndef CHR
	#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))
# endif

#  define GSUB_TAG  CHR('G','S','U','B')
#  define NAME_TAG  CHR('n','a','m','e')
#define REQUIRED_FEATURE    CHR(' ','R','Q','D')
#define DEFAULT_SCRIPT      CHR('D','F','L','T')
#define DEFAULT_LANG        CHR('d','f','l','t')

#define chunkalloc(size)    calloc(1,size)
#define chunkfree(item,size)    free(item)
#ifndef N_
#define N_(str)     (str)
#endif
#define NU_(str)    (str)
#define S_(str) sgettext(str)

#define OPENTYPE_FEATURE_FRIENDLYNAMES_EMPTY { 0, NULL, NULL, 0 }

enum otlookup_typemasks {
    gsub_single_mask        = 0x00001,
    gsub_multiple_mask      = 0x00002,
    gsub_alternate_mask     = 0x00004,
    gsub_ligature_mask      = 0x00008,
    gsub_context_mask       = 0x00010,
    gsub_contextchain_mask  = 0x00020,
    gsub_reversecchain_mask = 0x00040,
    morx_indic_mask         = 0x00080,
    morx_context_mask       = 0x00100,
    morx_insert_mask        = 0x00200,
    /* ********************* */
    gpos_single_mask        = 0x00400,
    gpos_pair_mask          = 0x00800,
    gpos_cursive_mask       = 0x01000,
    gpos_mark2base_mask     = 0x02000,
    gpos_mark2ligature_mask = 0x04000,
    gpos_mark2mark_mask     = 0x08000,
    gpos_context_mask       = 0x10000,
    gpos_contextchain_mask  = 0x20000,
    kern_statemachine_mask  = 0x40000
};

enum gsub_inusetype { git_normal, git_justinuse, git_findnames };

enum fpossub_format { pst_glyphs, pst_class, pst_coverage,
	            pst_reversecoverage, pst_formatmax };

void print_lig_transform(LigatureSub *lig_sub, int lig_sum);

#endif /*__GSUB_PARA_H__*/

