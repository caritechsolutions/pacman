#ifdef GUI_TTF

#include "gsub_para.h"
#include "gsub_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gui_private.h"
#include "ft2build.h"
#ifdef NDS_SUPPORT
#include "freetype.h"
#else
#include "freetype/freetype.h"
#endif

struct otfname {
	struct otfname *next;
	uint16 lang;    /* windows language code */
	char *name;     /* utf8 */
};

static int feature_cnt;
#ifndef NO_OS
static int32 last_size_pos;
static uint16 design_size;
static uint16 fontstyle_id;
static uint16 design_range_bottom, design_range_top;
static  struct otfname *fontstyle_name;
#endif /*NO_OS*/
static uint32 copyright_start;
static uint32 gdef_start;
static int lookup_cnt;
static OTLookup *cur_lookups, *gsub_lookups;
static int glyph_cnt;
static SplineChar **chars;
static FPST *possub;
static LigatureSub *lig_sub = NULL;
static int lig_sum = 0;
static int get_code_by_name(FT_Face face, char *name);

static int32 getlong(FILE *ttf) {
#ifndef NO_OS
	int ch1 = getc(ttf);
	int ch2 = getc(ttf);
	int ch3 = getc(ttf);
	int ch4 = getc(ttf);

	if ( ch4==EOF )
		return( EOF );
	return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
#else
	return 0;
#endif /*NO_OS*/
}

int getushort(FILE *ttf) {
#ifndef NO_OS
	int ch1 = getc(ttf);
	int ch2 = getc(ttf);
	if ( ch2==EOF )
		return( EOF );
	return( (ch1<<8)|ch2 );
#else
	return 0;
#endif /*NO_OS*/
}

static struct scripts *readttfscripts(FILE *ttf, int32 pos)
{
	int i,j,k,cnt;
	int deflang, lcnt;
	struct scripts *scripts;

	fseek(ttf,pos,SEEK_SET);
	cnt = getushort(ttf);
	if ( cnt<=0 )
	{
		return (NULL);
	}
	else if ( cnt>1000 )
	{
		gxlogd("Too many scripts %d\n", cnt);
		return (NULL);
	}

	scripts = GxCore_Calloc(cnt+1, sizeof(struct scripts));
	for ( i=0; i<cnt; ++i ) {
		scripts[i].tag = getlong(ttf);
		scripts[i].offset = getushort(ttf);
	}

	for ( i=0; i<cnt; ++i ) {
		fseek(ttf,pos+scripts[i].offset,SEEK_SET);
		deflang = getushort(ttf);
		lcnt = getushort(ttf);
		lcnt += (deflang!=0);
		scripts[i].langcnt = lcnt;
		scripts[i].languages = GxCore_Calloc(lcnt+1,sizeof(struct language));
		j = 0;

		if ( deflang!=0 ) {
			scripts[i].languages[0].tag = CHR('d','f','l','t');
			scripts[i].languages[0].offset = deflang+scripts[i].offset;
			++j;
		}

		for ( ; j<lcnt; ++j ) {
			scripts[i].languages[j].tag = getlong(ttf);
			scripts[i].languages[j].offset = scripts[i].offset+getushort(ttf);
		}

		for ( j=0; j<lcnt; ++j ) {
			fseek(ttf,pos+scripts[i].languages[j].offset,SEEK_SET);
			(void) getushort(ttf);  /* lookup ordering table undefined */
			scripts[i].languages[j].req = getushort(ttf);
			scripts[i].languages[j].fcnt = getushort(ttf);
#ifndef NO_OS
			if ( feof(ttf)) {
				gxlogd("End of file when reading scripts in GSUB table.\n");
				return (NULL);
			}
#endif /*NO_OS*/
			scripts[i].languages[j].features = GxCore_Malloc(scripts[i].languages[j].fcnt*sizeof(uint16));
			for ( k=0; k<scripts[i].languages[j].fcnt; ++k )
				scripts[i].languages[j].features[k] = getushort(ttf);
		}
	}

#ifndef NO_OS
	if ( feof(ttf)) {
		gxlogd("End of file when reading scripts in GSUB table.\n");
		return (NULL);
	}
#endif /*NO_OS*/

	return (scripts);
}

static char *_readencstring(FILE *ttf,int offset,int len,
							int platform,int specific,int language)
{
	long pos = ftell(ttf);
	char *ret = NULL;
	int i;

	fseek(ttf,offset,SEEK_SET);

	if ( platform==1 ) {
		/* Mac is screwy, there are several different varients of MacRoman */
		/*  depending on the language, they didn't get it right when they  */
		/*  invented their script system */
		char *cstr, *cpt;
		cstr = cpt = GxCore_Malloc(len+1);
		for ( i=0; i<len; ++i )
		{
#ifndef NO_OS
			*cpt++ = getc(ttf);
#endif /*NO_OS*/
		}
		*cpt = '\0';
		//ret = MacStrToUtf8(cstr,specific,language);
		ret = strdup(cstr);
		GxCore_Free(cstr);
	} else {
		// TODO:
	}
	fseek(ttf,pos,SEEK_SET);
	return( ret );
}

struct otfname *FindAllLangEntries(FILE *ttf, int id ){
	/* Look for all entries with string id under windows platform */
	int32 here = ftell(ttf);
	int i, cnt, tableoff;
	int platform, specific, language, name, str_len, stroff;
	struct otfname *head=NULL, *cur;

	if ( copyright_start!=0 && id!=0 ) {
		fseek(ttf,copyright_start,SEEK_SET);
		/* format selector = */ getushort(ttf);
		cnt = getushort(ttf);
		tableoff = copyright_start+getushort(ttf);
		for ( i=0; i<cnt; ++i ) {
			platform = getushort(ttf);
			specific = getushort(ttf);
			language = getushort(ttf);
			name = getushort(ttf);
			str_len = getushort(ttf);
			stroff = getushort(ttf);

			if ( platform==3 && name==id ) {
				char *temp = _readencstring(ttf,tableoff+stroff,str_len,platform,specific,language);
				if ( temp!=NULL ) {
					cur = chunkalloc(sizeof(struct otfname));
					cur->next = head;
					head = cur;
					cur->lang = language;
					cur->name = temp;
				}
			}
		}
		fseek(ttf,here,SEEK_SET);
	}
	return( head );
}

#ifndef NO_OS
static void readttfsizeparameters(FILE *ttf,int32 broken_pos,int32 correct_pos)
{
	int32 here;
	/* Both of the two fonts I've seen that contain a 'size' feature */
	/*  have multiple features all of which point to the same parameter */
	/*  area. Odd. */
	/* When Adobe first released fonts containing the 'size' feature */
	/*  they did not follow the spec, and the offset to the size parameters */
	/*  was relative to the wrong location. They claim (Aug 2006) that */
	/*  this has been fixed. Be prepared to read either style of 'size' */
	/*  following the heuristics Adobe provides */
	int32 test[2];
	int i, nid;

	if ( last_size_pos==broken_pos || last_size_pos==correct_pos )
	{
		return;
	}

	if(last_size_pos!=0)
	{
		return;
	}

	test[0] = correct_pos; test[1] = broken_pos;
	here = ftell(ttf);
	for ( i=0; i<2; ++i ) {
		fseek(ttf,test[i],SEEK_SET);
		last_size_pos = test[i];
		design_size = getushort(ttf);
		if ( design_size==0 )
			continue;
		fontstyle_id = getushort(ttf);
		nid = getushort(ttf);
		design_range_bottom = getushort(ttf);
		design_range_top = getushort(ttf);
		if ( fontstyle_id == 0 && nid==0 && design_size!=0 &&
			 design_range_bottom==0 && design_range_top==0 ) {
			/* Reasonable spec, only design size provided */
			fontstyle_name = NULL;
			break;
		}

		if ( design_size==0 ||
			 design_size < design_range_bottom ||
			 design_size > design_range_top ||
			 design_range_bottom > design_range_top )
			continue;
		if ( fontstyle_id == 0 && nid==0 ) {
			/* Not really allowed, but we'll accept it anyway */
			/* If a range is provided than a style name is expected */
			gxlogd("This font contains a 'size' feature with a design size and design range but no stylename. That is technically an error, but we'll let it pass\n");
			fontstyle_name = NULL;
			break;
		}
		if ( nid<256 || nid>32767 )
			continue;
		// TODO: FindAllLangEntries
		if ( fontstyle_name==NULL )
			continue;
		else
			break;
	}
	if ( i==2 ) {
		gxlogd("The 'size' feature does not seem to follow the standard,\nnor does it conform to Adobe's early misinterpretation of\nthe standard. I cannot parse it.\n");
		design_size = design_range_bottom = design_range_top = fontstyle_id = 0;
		fontstyle_name = NULL;
	} else if ( i==1 ) {
		gxlogd("The 'size' feature of this font conforms to Adobe's early misinterpretation of the otf standard.\n");
	}
	fseek(ttf,here,SEEK_SET);
}

static void readttffeatnameparameters(FILE *ttf,int32 pos,uint32 tag)
{
	// TODO:
}
#endif /*NO_OS*/

static struct feature *readttffeatures(FILE *ttf, int32 pos)
{
	/* read the features table returning an array containing all interesting */
	/*  features */
	int cnt;
	int i,j;
	struct feature *features;
	int parameters;

	fseek(ttf,pos,SEEK_SET);
	feature_cnt = cnt = getushort(ttf);
	if ( cnt<=0 )
	{
		return (NULL);
	}
	else if ( cnt>1000 ) {
		gxlogd("Too many features %d\n", cnt);
		return (NULL);
	}

	features = GxCore_Calloc(cnt+1,sizeof(struct feature));
	for ( i=0; i<cnt; ++i ) {
		features[i].tag = getlong(ttf);
		features[i].offset = getushort(ttf);
	}

	for ( i=0; i<cnt; ++i ) {
		fseek(ttf,pos+features[i].offset,SEEK_SET);
		parameters = getushort(ttf);
#ifndef NO_OS
		if ( features[i].tag==CHR('s','i','z','e') && parameters!=0 && !feof(ttf)) {
			// TODO: readttfsizeparameters
			readttfsizeparameters(ttf, pos+parameters, pos+parameters+features[i].offset);
		}else if ( features[i].tag>=CHR('s','s','0','1') && features[i].tag<=CHR('s','s','2','0') &&
				   parameters!=0 && !feof(ttf)) {
			// TODO: readttffeatnameparameters
			readttffeatnameparameters(ttf, pos+parameters+features[i].offset, features[i].tag);
		}
#endif /*NO_OS*/
		features[i].lcnt = getushort(ttf);
#ifndef NO_OS
		if ( feof(ttf) ) {
			gxlogd("End of file when reading features in GSUB table");
			return (NULL);
		}
#endif /*NO_OS*/
		features[i].lookups = GxCore_Malloc(features[i].lcnt*sizeof(uint16));
		for ( j=0; j<features[i].lcnt; ++j )
			features[i].lookups[j] = getushort(ttf);
	}

	return( features );
}

static struct lookup *readttflookups(FILE *ttf,int32 pos)
{
	int cnt,i,j;
	struct lookup *lookups;
	OTLookup *otlookup, *last=NULL;
	struct lookup_subtable *st;

	fseek(ttf,pos,SEEK_SET);
	lookup_cnt = cnt = getushort(ttf);
	cur_lookups = NULL;
	if ( cnt<=0 )
	{
		return( NULL );
	}
	else if ( cnt>1000 )
	{
		gxlogd("Too many lookups %d\n", cnt);
		return( NULL );
	}

	lookups = GxCore_Calloc(cnt+1,sizeof(struct lookup));
	for ( i=0; i<cnt; ++i )
		lookups[i].offset = getushort(ttf);
	for ( i=0; i<cnt; ++i ) {
		fseek(ttf,pos+lookups[i].offset,SEEK_SET);
		lookups[i].type = getushort(ttf);
		lookups[i].flags = getushort(ttf);
		lookups[i].subtabcnt = getushort(ttf);
		lookups[i].subtab_offsets = GxCore_Malloc(lookups[i].subtabcnt*sizeof(int32));

		for ( j=0; j<lookups[i].subtabcnt; ++j )
			lookups[i].subtab_offsets[j] = pos+lookups[i].offset+getushort(ttf);
		if ( lookups[i].flags&0x10 )
			lookups[i].flags |= (getushort(ttf)<<16);

		lookups[i].otlookup = otlookup = chunkalloc(sizeof(OTLookup));
		otlookup->lookup_index = i;
		if ( last==NULL )
			cur_lookups = otlookup;
		else
			last->next = otlookup;
		last = otlookup;
		otlookup->lookup_type = (0) | lookups[i].type;
		otlookup->lookup_flags = lookups[i].flags;
		otlookup->lookup_index = i;
#ifndef NO_OS
		if ( feof(ttf) ) {
			gxlogd("End of file when reading lookups in GSUB table");
			return (NULL);
		}
#endif /*NO_OS*/
		for ( j=0; j<lookups[i].subtabcnt; ++j ) {
			st = chunkalloc(sizeof(struct lookup_subtable));
			st->next = otlookup->subtables;
			st->lookup = otlookup;
			otlookup->subtables = st;
		}
	}
	gsub_lookups = cur_lookups;
	return( lookups );
}

static void ScriptsFree(struct scripts *scripts) {
	int i,j;

	if ( scripts==NULL )
		return;
	for ( i=0; scripts[i].offset!=0 ; ++i ) {
		for ( j=0; j<scripts[i].langcnt; ++j )
			GxCore_Free( scripts[i].languages[j].features);
		GxCore_Free(scripts[i].languages);
	}
	GxCore_Free(scripts);
}

static void FeaturesFree(struct feature *features) {
	int i;

	if ( features==NULL )
		return;
	for ( i=0; features[i].offset!=0 ; ++i )
		GxCore_Free(features[i].lookups);
	GxCore_Free(features);
}

static void LookupsFree(struct lookup *lookups) {
	int i;

	for ( i=0; lookups[i].offset!=0 ; ++i ) {
		GxCore_Free( lookups[i].subtab_offsets );
	}
	GxCore_Free(lookups);
}

static void FListAppendScriptLang(FeatureScriptLangList *fl,uint32 script_tag,uint32 lang_tag)
{
	struct scriptlanglist *sl;
	int l;

	for ( sl = fl->scripts; sl!=NULL && sl->script!=script_tag; sl=sl->next );
	if ( sl==NULL ) {
		sl = chunkalloc(sizeof(struct scriptlanglist));
		sl->script = script_tag;
		sl->next = fl->scripts;
		fl->scripts = sl;
	}
	for ( l=0; l<MAX_LANG && l<sl->lang_cnt && sl->langs[l]!=lang_tag; ++l );
	if ( l>=MAX_LANG && l<sl->lang_cnt ) {
		while ( l<sl->lang_cnt && sl->morelangs[l-MAX_LANG]!=lang_tag )
			++l;
	}
	if ( l>=sl->lang_cnt ) {
		if ( l<MAX_LANG )
			sl->langs[l] = lang_tag;
		else {
			if ( l%MAX_LANG == 0 )
				sl->morelangs = GxCore_Realloc(sl->morelangs,l*sizeof(uint32));
			/* We've just allocated MAX_LANG-1 more than we need */
			/*  so we don't do quite some many allocations */
			sl->morelangs[l-MAX_LANG] = lang_tag;
		}
		++sl->lang_cnt;
	}
}

static void tagLookupsWithFeature(uint32 script_tag,uint32 lang_tag,
		    int required_feature, struct feature *feature, struct lookup *lookups)
{
	uint32 feature_tag = required_feature ? REQUIRED_FEATURE : feature->tag;
	int i;
	OTLookup *otlookup;
	FeatureScriptLangList *fl;

	/* The otf docs are ambiguous as to the capitalization of the default */
	/*  script. The capitalized version is correct (uncapitalized is used for languages) */
	if ( script_tag == DEFAULT_LANG )
		script_tag = DEFAULT_SCRIPT;

	for ( i=0; i < feature->lcnt; ++i ) {
		if ( feature->lookups[i]>=lookup_cnt ) {
			gxlogd("Lookup out of bounds in feature table.\n");
		} else {
			otlookup = lookups[feature->lookups[i]].otlookup;
			for ( fl = otlookup->features; fl!=NULL && fl->featuretag!=feature_tag; fl=fl->next );
			if ( fl==NULL ) {
				fl = chunkalloc(sizeof(FeatureScriptLangList));
				fl->featuretag = feature_tag;
				fl->next = otlookup->features;
				otlookup->features = fl;
			}
			FListAppendScriptLang(fl,script_tag,lang_tag);
		}
	}
}

static void tagLookupsWithScript(struct scripts *scripts,struct feature *features, struct lookup *lookups)
{
	int i,j;
	struct scripts *s;
	struct language *lang;
	struct lookup *l;

	if ( scripts==NULL || features==NULL )
	{
		return;     /* Legal, I'd guess, but not very interesting. Perhaps all lookups are controlled by the JSTF table
					   or something */
	}

	/* First tag every lookup with all script, lang, feature combinations that*/
	/*  invoke it */
	for ( s=scripts; s->tag!=0; ++s ) {
		for ( lang=s->languages, i=0; i<s->langcnt; ++i, ++lang ) {
			if ( lang->req==0xffff )
				/* Do Nothing */;
			 else if ( lang->req>= feature_cnt ) {
				gxlogd("Required feature out of bounds in script table.\n");
			 } else
				tagLookupsWithFeature(s->tag,lang->tag,1,&features[lang->req],lookups);

			 for ( j=0; j<lang->fcnt; ++j ) {
				 if ( lang->features[j]>=feature_cnt ) {
					 gxlogd("Feature out of bounds in script table.\n");
				 } else
					 tagLookupsWithFeature(s->tag,lang->tag,0,&features[lang->features[j]],lookups);
			 }
		}
	}
	/* The scripts got added backwards so reverse to put them in */
	/*  alphabetic order again */
	for ( l=lookups, i=0; l->offset!=0; ++l, ++i ) {
		OTLookup *otl = l->otlookup;
		FeatureScriptLangList *fl;
		struct scriptlanglist *sl, *next, *prev;
		for ( fl=otl->features; fl!=NULL; fl=fl->next ) {
			prev = NULL;
			for ( sl=fl->scripts; sl!=NULL; sl = next ) {
				next = sl->next;
				sl->next = prev;
				prev = sl;
			}
			fl->scripts = prev;
		}
	}
}

void ScriptLangListFree(struct scriptlanglist *sl) {
	struct scriptlanglist *next;

	while ( sl!=NULL ) {
		next = sl->next;
		GxCore_Free(sl->morelangs);
		chunkfree(sl,sizeof(*sl));
		sl = next;
	}
}

void FeatureScriptLangListFree(FeatureScriptLangList *fl) {
	FeatureScriptLangList *next;

	while ( fl!=NULL ) {
		next = fl->next;
		ScriptLangListFree(fl->scripts);
		chunkfree(fl,sizeof(*fl));
		fl = next;
	}
}

void OTLookupFree(OTLookup *lookup) {
	struct lookup_subtable *st, *stnext;

	GxCore_Free(lookup->lookup_name);
	FeatureScriptLangListFree(lookup->features);
	for ( st=lookup->subtables; st!=NULL; st=stnext ) {
		stnext = st->next;
		GxCore_Free(st->subtable_name);
		GxCore_Free(st->suffix);
		chunkfree(st,sizeof(struct lookup_subtable));
	}
	chunkfree( lookup,sizeof(OTLookup) );
}

static void OTLookupListFree(OTLookup *lookup ) {
	OTLookup *next;

	for ( ; lookup!=NULL; lookup = next ) {
		next = lookup->next;
		OTLookupFree(lookup);
	}
}

char *sgettext(const char *msgid) {
	const char *msgval = msgid;//_(msgid);
	char *found;
	if (msgval == msgid)
		if ( (found = strrchr (msgid, '|'))!=NULL )
			msgval = found+1;
	return (char *) msgval;
}

const char *lookup_type_names[2][10] = {
	{   N_("Undefined substitution"), N_("Single Substitution"), N_("Multiple Substitution"),
		N_("Alternate Substitution"), N_("Ligature Substitution"), N_("Contextual Substitution"),
		N_("Contextual Chaining Substitution"), N_("Extension"),
		N_("Reverse Contextual Chaining Substitution")
	},
	{   N_("Undefined positioning"), N_("Single Positioning"), N_("Pairwise Positioning (kerning)"),
		N_("Cursive attachment"), N_("Mark to base attachment"),
		N_("Mark to Ligature attachment"), N_("Mark to Mark attachment"),
		N_("Contextual Positioning"), N_("Contextual Chaining Positioning"),
		N_("Extension")
	}
};

/* This is a non-ui based copy of a similar list in lookupui.c */
static struct {
	    const char *text;
		    uint32 tag;
} localscripts[] = {
	/* GT: See the long comment at "Property|New" */
	/* GT: The msgstr should contain a translation of "Arabic", ignore "Script|" */
	{ N_("Script|Arabic"), CHR('a','r','a','b') },
	{ N_("Script|Aramaic"), CHR('a','r','a','m') },
	{ N_("Script|Armenian"), CHR('a','r','m','n') },
	{ N_("Script|Avestan"), CHR('a','v','e','s') },
	{ N_("Script|Balinese"), CHR('b','a','l','i') },
	{ N_("Script|Batak"), CHR('b','a','t','k') },
	{ N_("Script|Bengali"), CHR('b','e','n','g') },
	{ N_("Script|Bengali2"), CHR('b','n','g','2') },
	{ N_("Bliss Symbolics"), CHR('b','l','i','s') },
	{ N_("Bopomofo"), CHR('b','o','p','o') },
	{ NU_("Brāhmī"), CHR('b','r','a','h') },
	{ N_("Braille"), CHR('b','r','a','i') },
	{ N_("Script|Buginese"), CHR('b','u','g','i') },
	{ N_("Script|Buhid"), CHR('b','u','h','d') },
	{ N_("Byzantine Music"), CHR('b','y','z','m') },
	{ N_("Canadian Syllabics"), CHR('c','a','n','s') },
	{ N_("Carian"), CHR('c','a','r','i') },
	{ N_("Cherokee"), CHR('c','h','a','m') },
	{ N_("Script|Cham"), CHR('c','h','a','m') },
	{ N_("Script|Cherokee"), CHR('c','h','e','r') },
	{ N_("Cirth"), CHR('c','i','r','t') },
	{ N_("CJK Ideographic"), CHR('h','a','n','i') },
	{ N_("Script|Coptic"), CHR('c','o','p','t') },
	{ N_("Cypro-Minoan"), CHR('c','p','r','t') },
	{ N_("Cypriot syllabary"), CHR('c','p','m','n') },
	{ N_("Cyrillic"), CHR('c','y','r','l') },
	{ N_("Script|Default"), CHR('D','F','L','T') },
	{ N_("Deseret (Mormon)"), CHR('d','s','r','t') },
	{ N_("Devanagari"), CHR('d','e','v','a') },
	{ N_("Devanagari2"), CHR('d','e','v','2') },
	/*  { N_("Egyptian demotic"), CHR('e','g','y','d') }, */
	/*  { N_("Egyptian hieratic"), CHR('e','g','y','h') }, */
	/* GT: Someone asked if FontForge actually was prepared generate hieroglyph output */
	/* GT: because of this string. No. But OpenType and Unicode have placeholders for */
	/* GT: dealing with these scripts against the day someone wants to use them. So */
	/* GT: FontForge must be prepared to deal with those placeholders if nothing else. */
	/*  { N_("Egyptian hieroglyphs"), CHR('e','g','y','p') }, */
	{ N_("Script|Ethiopic"), CHR('e','t','h','i') },
	{ N_("Script|Georgian"), CHR('g','e','o','r') },
	{ N_("Glagolitic"), CHR('g','l','a','g') },
	{ N_("Gothic"), CHR('g','o','t','h') },
	{ N_("Script|Greek"), CHR('g','r','e','k') },
	{ N_("Script|Gujarati"), CHR('g','u','j','r') },
	{ N_("Script|Gujarati2"), CHR('g','j','r','2') },
	{ N_("Gurmukhi"), CHR('g','u','r','u') },
	{ N_("Gurmukhi2"), CHR('g','u','r','2') },
	{ N_("Hangul Jamo"), CHR('j','a','m','o') },
	{ N_("Hangul"), CHR('h','a','n','g') },
	{ NU_("Script|Hanunóo"), CHR('h','a','n','o') },
	{ N_("Script|Hebrew"), CHR('h','e','b','r') },
	/*  { N_("Pahawh Hmong"), CHR('h','m','n','g') },*/
	/*  { N_("Indus (Harappan)"), CHR('i','n','d','s') },*/
	{ N_("Script|Javanese"), CHR('j','a','v','a') },
	{ N_("Kayah Li"), CHR('k','a','l','i') },
	{ N_("Hiragana & Katakana"), CHR('k','a','n','a') },
	{ NU_("Kharoṣṭhī"), CHR('k','h','a','r') },
	{ N_("Script|Kannada"), CHR('k','n','d','a') },
	{ N_("Script|Kannada2"), CHR('k','n','d','2') },
	{ N_("Script|Khmer"), CHR('k','h','m','r') },
	{ N_("Script|Kharosthi"), CHR('k','h','a','r') },
	{ N_("Script|Lao") , CHR('l','a','o',' ') },
	{ N_("Script|Latin"), CHR('l','a','t','n') },
	{ NU_("Lepcha (Róng)"), CHR('l','e','p','c') },
	{ N_("Script|Limbu"), CHR('l','i','m','b') },   /* Not in ISO 15924 !!!!!, just guessing */
	{ N_("Linear A"), CHR('l','i','n','a') },
	{ N_("Linear B"), CHR('l','i','n','b') },
	{ N_("Lycian"), CHR('l','y','c','i') },
	{ N_("Lydian"), CHR('l','y','d','i') },
	{ N_("Script|Mandaean"), CHR('m','a','n','d') },
	/*  { N_("Mayan hieroglyphs"), *  CHR('m','a','y','a') },*/
	{ NU_("Script|Malayālam"), CHR('m','l','y','m') },
	{ NU_("Script|Malayālam2"), CHR('m','l','m','2') },
	{ NU_("Mathematical Alphanumeric Symbols"), CHR('m','a','t','h') },
	{ N_("Script|Mongolian"), CHR('m','o','n','g') },
	{ N_("Musical"), CHR('m','u','s','c') },
	{ N_("Script|Myanmar"), CHR('m','y','m','r') },
	{ N_("New Tai Lue"), CHR('t','a','l','u') },
	{ N_("N'Ko"), CHR('n','k','o',' ') },
	{ N_("Ogham"), CHR('o','g','a','m') },
	{ N_("Ol Chiki"), CHR('o','l','c','k') },
	{ N_("Old Italic (Etruscan, Oscan, etc.)"), CHR('i','t','a','l') },
	{ N_("Script|Old Permic"), CHR('p','e','r','m') },
	{ N_("Old Persian cuneiform"), CHR('x','p','e','o') },
	{ N_("Script|Oriya"), CHR('o','r','y','a') },
	{ N_("Script|Oriya2"), CHR('o','r','y','2') },
	{ N_("Osmanya"), CHR('o','s','m','a') },
	{ N_("Script|Pahlavi"), CHR('p','a','l','v') },
	{ N_("Script|Phags-pa"), CHR('p','h','a','g') },
	{ N_("Script|Phoenician"), CHR('p','h','n','x') },
	{ N_("Phaistos"), CHR('p','h','s','t') },
	{ N_("Pollard Phonetic"), CHR('p','l','r','d') },
	{ N_("Rejang"), CHR('r','j','n','g') },
	{ N_("Rongorongo"), CHR('r','o','r','o') },
	{ N_("Runic"), CHR('r','u','n','r') },
	{ N_("Saurashtra"), CHR('s','a','u','r') },
	{ N_("Shavian"), CHR('s','h','a','w') },
	{ N_("Script|Sinhala"), CHR('s','i','n','h') },
	{ N_("Script|Sumero-Akkadian Cuneiform"), CHR('x','s','u','x') },
	{ N_("Script|Sundanese"), CHR('s','u','n','d')},
	{ N_("Script|Syloti Nagri"), CHR('s','y','l','o') },
	{ N_("Script|Syriac"), CHR('s','y','r','c') },
	{ N_("Script|Tagalog"), CHR('t','g','l','g') },
	{ N_("Script|Tagbanwa"), CHR('t','a','g','b') },
	{ N_("Tai Le"), CHR('t','a','l','e') }, /* Not in ISO 15924 !!!!!, just guessing */
	{ N_("Tai Lu"), CHR('t','a','l','a') }, /* Not in ISO 15924 !!!!!, just guessing */
	{ N_("Script|Tamil"), CHR('t','a','m','l') },
	{ N_("Script|Tamil2"), CHR('t','m','l','2') },
	{ N_("Script|Telugu"), CHR('t','e','l','u') },
	{ N_("Script|Telugu2"), CHR('t','e','l','2') },
	{ N_("Tengwar"), CHR('t','e','n','g') },
	{ N_("Thaana"), CHR('t','h','a','a') },
	{ N_("Script|Thai"), CHR('t','h','a','i') },
	{ N_("Script|Tibetan"), CHR('t','i','b','t') },
	{ N_("Tifinagh (Berber)"), CHR('t','f','n','g') },
	{ N_("Script|Ugaritic"), CHR('u','g','r','t') },    /* Not in ISO 15924 !!!!!, just guessing */
	{ N_("Script|Vai"), CHR('v','a','i',' ') },
	/*  { N_("Visible Speech"), CHR('v','i','s','p') },*/
	{ N_("Cuneiform, Ugaritic"), CHR('x','u','g','a') },
	{ N_("Script|Yi")  , CHR('y','i',' ',' ') },
	/*  { N_("Private Use Script 1"), CHR('q','a','a','a') },*/
	/*  { N_("Private Use Script 2"), CHR('q','a','a','b') },*/
	/*  { N_("Undetermined Script"), CHR('z','y','y','y') },*/
	/*  { N_("Uncoded Script"), CHR('z','z','z','z') },*/
	{ NULL, 0 }
};

struct opentype_feature_friendlynames friendlies[] = {
    { CHR('a','a','l','t'), "aalt", N_("Access All Alternates"),    gsub_single_mask|gsub_alternate_mask },
    { CHR('a','b','v','f'), "abvf", N_("Above Base Forms"),     gsub_single_mask },
    { CHR('a','b','v','m'), "abvm", N_("Above Base Mark"),      gpos_mark2base_mask|gpos_mark2ligature_mask },
    { CHR('a','b','v','s'), "abvs", N_("Above Base Substitutions"), gsub_ligature_mask },
    { CHR('a','f','r','c'), "afrc", N_("Vertical Fractions"),   gsub_ligature_mask },
    { CHR('a','k','h','n'), "akhn", N_("Akhand"),           gsub_ligature_mask },
    { CHR('a','l','i','g'), "alig", N_("Ancient Ligatures"),    gsub_ligature_mask },
    { CHR('b','l','w','f'), "blwf", N_("Below Base Forms"),     gsub_ligature_mask },
    { CHR('b','l','w','m'), "blwm", N_("Below Base Mark"),      gpos_mark2base_mask|gpos_mark2ligature_mask },
    { CHR('b','l','w','s'), "blws", N_("Below Base Substitutions"), gsub_ligature_mask },
    { CHR('c','2','p','c'), "c2pc", N_("Capitals to Petite Capitals"),  gsub_single_mask },
    { CHR('c','2','s','c'), "c2sc", N_("Capitals to Small Capitals"),   gsub_single_mask },
    { CHR('c','a','l','t'), "calt", N_("Contextual Alternates"),    gsub_context_mask|gsub_contextchain_mask|morx_context_mask },
    { CHR('c','a','s','e'), "case", N_("Case-Sensitive Forms"), gsub_single_mask|gpos_single_mask },
    { CHR('c','c','m','p'), "ccmp", N_("Glyph Composition/Decomposition"),  gsub_multiple_mask|gsub_ligature_mask },
    { CHR('c','f','a','r'), "cfar", N_("Conjunct Form After Ro"),   gsub_single_mask },
    { CHR('c','j','c','t'), "cjct", N_("Conjunct Forms"),       gsub_ligature_mask },
    { CHR('c','l','i','g'), "clig", N_("Contextual Ligatures"), gsub_reversecchain_mask },
    { CHR('c','p','c','t'), "cpct", N_("Centered CJK Punctuation"), gpos_single_mask },
    { CHR('c','p','s','p'), "cpsp", N_("Capital Spacing"),      gpos_single_mask },
    { CHR('c','s','w','h'), "cswh", N_("Contextual Swash"),     gsub_reversecchain_mask },
    { CHR('c','u','r','s'), "curs", N_("Cursive Attachment"),   gpos_cursive_mask },
    { CHR('c','v','0','0'), "cv00", N_("Character Variants 00"),    gsub_single_mask },
    { CHR('c','v','0','1'), "cv01", N_("Character Variants 01"),    gsub_single_mask },
    { CHR('c','v','0','2'), "cv02", N_("Character Variants 02"),    gsub_single_mask },
    { CHR('c','v','0','3'), "cv03", N_("Character Variants 03"),    gsub_single_mask },
    { CHR('c','v','0','4'), "cv04", N_("Character Variants 04"),    gsub_single_mask },
    { CHR('c','v','0','5'), "cv05", N_("Character Variants 05"),    gsub_single_mask },
{ CHR('c','v','0','6'), "cv06", N_("Character Variants 06"),    gsub_single_mask },
    { CHR('c','v','0','7'), "cv07", N_("Character Variants 07"),    gsub_single_mask },
    { CHR('c','v','0','8'), "cv08", N_("Character Variants 08"),    gsub_single_mask },
    { CHR('c','v','0','9'), "cv09", N_("Character Variants 09"),    gsub_single_mask },
    { CHR('c','v','1','0'), "cv10", N_("Character Variants 10"),    gsub_single_mask },
    { CHR('c','v','9','9'), "cv99", N_("Character Variants 99"),    gsub_single_mask },
    { CHR('d','c','a','p'), "dcap", N_("Drop Caps"),        gsub_single_mask },
    { CHR('d','i','s','t'), "dist", N_("Distance"),         gpos_pair_mask },
    { CHR('d','l','i','g'), "dlig", N_("Discretionary Ligatures"),  gsub_ligature_mask },
    { CHR('d','n','o','m'), "dnom", N_("Denominators"),     gsub_single_mask },
    { CHR('d','p','n','g'), "dpng", N_("Dipthongs (Obsolete)"), gsub_ligature_mask },
    { CHR('d','t','l','s'), "dtls", N_("Dotless Forms"),        gsub_single_mask },
    { CHR('e','x','p','t'), "expt", N_("Expert Forms"),     gsub_single_mask },
    { CHR('f','a','l','t'), "falt", N_("Final Glyph On Line"),  gsub_alternate_mask },
    { CHR('f','i','n','2'), "fin2", N_("Terminal Forms #2"),    gsub_context_mask|gsub_contextchain_mask|morx_context_mask },
    { CHR('f','i','n','3'), "fin3", N_("Terminal Forms #3"),    gsub_context_mask|gsub_contextchain_mask|morx_context_mask },
    { CHR('f','i','n','a'), "fina", N_("Terminal Forms"),       gsub_single_mask },
    { CHR('f','l','a','c'), "flac", N_("Flattened Accents over Capitals"),  gsub_single_mask|gsub_ligature_mask },
    { CHR('f','r','a','c'), "frac", N_("Diagonal Fractions"),   gsub_single_mask|gsub_ligature_mask },
    { CHR('f','w','i','d'), "fwid", N_("Full Widths"),      gsub_single_mask|gpos_single_mask },
    { CHR('h','a','l','f'), "half", N_("Half Forms"),       gsub_ligature_mask },
    { CHR('h','a','l','n'), "haln", N_("Halant Forms"),     gsub_ligature_mask },
    { CHR('h','a','l','t'), "halt", N_("Alternative Half Widths"),  gpos_single_mask },
    { CHR('h','i','s','t'), "hist", N_("Historical Forms"),     gsub_single_mask },
    { CHR('h','k','n','a'), "hkna", N_("Horizontal Kana Alternatives"), gsub_single_mask },
    { CHR('h','l','i','g'), "hlig", N_("Historic Ligatures"),   gsub_ligature_mask },
    { CHR('h','n','g','l'), "hngl", N_("Hanja to Hangul"),      gsub_single_mask|gsub_alternate_mask },
    { CHR('h','o','j','o'), "hojo", N_("Hojo (JIS X 0212-1990) Kanji Forms"),   gsub_single_mask },
    { CHR('h','w','i','d'), "hwid", N_("Half Widths"),      gsub_single_mask|gpos_single_mask },
    { CHR('i','n','i','t'), "init", N_("Initial Forms"),        gsub_single_mask },
    { CHR('i','s','o','l'), "isol", N_("Isolated Forms"),       gsub_single_mask },
    { CHR('i','t','a','l'), "ital", N_("Italics"),          gsub_single_mask },
    { CHR('j','a','l','t'), "jalt", N_("Justification Alternatives"),   gsub_alternate_mask },
    { CHR('j','a','j','p'), "jajp", N_("Japanese Forms (Obsolete)"),    gsub_single_mask|gsub_alternate_mask },
    { CHR('j','p','0','4'), "jp04", N_("JIS2004 Forms"),        gsub_single_mask },
    { CHR('j','p','7','8'), "jp78", N_("JIS78 Forms"),      gsub_single_mask|gsub_alternate_mask },
    { CHR('j','p','8','3'), "jp83", N_("JIS83 Forms"),      gsub_single_mask },
    { CHR('j','p','9','0'), "jp90", N_("JIS90 Forms"),      gsub_single_mask },
    { CHR('k','e','r','n'), "kern", N_("Horizontal Kerning"),   gpos_pair_mask|gpos_context_mask|gpos_contextchain_mask|kern_statemachine_mask },
    { CHR('l','f','b','d'), "lfbd", N_("Left Bounds"),      gpos_single_mask },
    { CHR('l','i','g','a'), "liga", N_("Standard Ligatures"),   gsub_ligature_mask },
    { CHR('l','j','m','o'), "ljmo", N_("Leading Jamo Forms"),   gsub_ligature_mask },
    { CHR('l','n','u','m'), "lnum", N_("Lining Figures"),       gsub_single_mask },
    { CHR('l','o','c','l'), "locl", N_("Localized Forms"),      gsub_single_mask },
    { CHR('m','a','r','k'), "mark", N_("Mark Positioning"),     gpos_mark2base_mask|gpos_mark2ligature_mask },
    { CHR('m','e','d','2'), "med2", N_("Medial Forms 2"),       gsub_context_mask|gsub_contextchain_mask|morx_context_mask },
    { CHR('m','e','d','i'), "medi", N_("Medial Forms"),     gsub_single_mask },
    { CHR('m','g','r','k'), "mgrk", N_("Mathematical Greek"),   gsub_single_mask },
    { CHR('m','k','m','k'), "mkmk", N_("Mark to Mark"),     gpos_mark2mark_mask },
    { CHR('m','s','e','t'), "mset", N_("Mark Positioning via Substitution"),    gsub_context_mask|gsub_contextchain_mask|morx_context_mask },
    { CHR('n','a','l','t'), "nalt", N_("Alternate Annotation Forms"),   gsub_single_mask|gsub_alternate_mask },
    { CHR('n','l','c','k'), "nlck", N_("NLC Kanji Forms"),      gsub_single_mask },
    { CHR('n','u','k','t'), "nukt", N_("Nukta Forms"),      gsub_ligature_mask },
    { CHR('n','u','m','r'), "numr", N_("Numerators"),       gsub_single_mask },
    { CHR('o','n','u','m'), "onum", N_("Oldstyle Figures"),     gsub_single_mask },
{ CHR('p','a','l','t'), "palt", N_("Proportional Alternate Metrics"),   gpos_single_mask },
    { CHR('p','c','a','p'), "pcap", N_("Lowercase to Petite Capitals"), gsub_single_mask },
    { CHR('p','k','n','a'), "pkna", N_("Proportional Kana"),    gpos_single_mask },
    { CHR('p','n','u','m'), "pnum", N_("Proportional Numbers"), gsub_single_mask },
    { CHR('p','r','e','f'), "pref", N_("Pre Base Forms"),       gsub_ligature_mask },
    { CHR('p','r','e','s'), "pres", N_("Pre Base Substitutions"),   gsub_ligature_mask|gsub_context_mask|gsub_contextchain_mask|morx_context_mask },
    { CHR('p','s','t','f'), "pstf", N_("Post Base Forms"),      gsub_ligature_mask },
    { CHR('p','s','t','s'), "psts", N_("Post Base Substitutions"),  gsub_ligature_mask },
    { CHR('p','w','i','d'), "pwid", N_("Proportional Width"),   gsub_single_mask },
    { CHR('q','w','i','d'), "qwid", N_("Quarter Widths"),       gsub_single_mask|gpos_single_mask },
    { CHR('r','a','n','d'), "rand", N_("Randomize"),        gsub_alternate_mask },
    { CHR('r','k','r','f'), "rkrf", N_("Rakar Forms"),      gsub_ligature_mask },
    { CHR('r','l','i','g'), "rlig", N_("Required Ligatures"),   gsub_ligature_mask },
    { CHR('r','p','h','f'), "rphf", N_("Reph Form"),        gsub_ligature_mask },
    { CHR('r','t','b','d'), "rtbd", N_("Right Bounds"),     gpos_single_mask },
    { CHR('r','t','l','a'), "rtla", N_("Right to Left Alternates"), gsub_single_mask },
    { CHR('r','t','l','m'), "rtlm", N_("Right to Left mirrored forms"), gsub_single_mask },
    { CHR('r','u','b','y'), "ruby", N_("Ruby Notational Forms"),    gsub_single_mask },
    { CHR('s','a','l','t'), "salt", N_("Stylistic Alternatives"),   gsub_single_mask|gsub_alternate_mask },
    { CHR('s','i','n','f'), "sinf", N_("Scientific Inferiors"), gsub_single_mask },
    { CHR('s','m','c','p'), "smcp", N_("Lowercase to Small Capitals"),  gsub_single_mask },
    { CHR('s','m','p','l'), "smpl", N_("Simplified Forms"),     gsub_single_mask },
    { CHR('s','s','0','1'), "ss01", N_("Style Set 1"),      gsub_single_mask },
    { CHR('s','s','0','2'), "ss02", N_("Style Set 2"),      gsub_single_mask },
    { CHR('s','s','0','3'), "ss03", N_("Style Set 3"),      gsub_single_mask },
    { CHR('s','s','0','4'), "ss04", N_("Style Set 4"),      gsub_single_mask },
    { CHR('s','s','0','5'), "ss05", N_("Style Set 5"),      gsub_single_mask },
    { CHR('s','s','0','6'), "ss06", N_("Style Set 6"),      gsub_single_mask },
    { CHR('s','s','0','7'), "ss07", N_("Style Set 7"),      gsub_single_mask },
    { CHR('s','s','0','8'), "ss08", N_("Style Set 8"),      gsub_single_mask },
    { CHR('s','s','0','9'), "ss09", N_("Style Set 9"),      gsub_single_mask },
    { CHR('s','s','1','0'), "ss10", N_("Style Set 10"),     gsub_single_mask },
    { CHR('s','s','1','1'), "ss11", N_("Style Set 11"),     gsub_single_mask },
    { CHR('s','s','1','2'), "ss12", N_("Style Set 12"),     gsub_single_mask },
    { CHR('s','s','1','3'), "ss13", N_("Style Set 13"),     gsub_single_mask },
    { CHR('s','s','1','4'), "ss14", N_("Style Set 14"),     gsub_single_mask },
    { CHR('s','s','1','5'), "ss15", N_("Style Set 15"),     gsub_single_mask },
    { CHR('s','s','1','6'), "ss16", N_("Style Set 16"),     gsub_single_mask },
    { CHR('s','s','1','7'), "ss17", N_("Style Set 17"),     gsub_single_mask },
    { CHR('s','s','1','8'), "ss18", N_("Style Set 18"),     gsub_single_mask },
    { CHR('s','s','1','9'), "ss19", N_("Style Set 19"),     gsub_single_mask },
    { CHR('s','s','2','0'), "ss20", N_("Style Set 20"),     gsub_single_mask },
    { CHR('s','s','t','y'), "ssty", N_("Script Style"),     gsub_single_mask|gsub_alternate_mask },
    { CHR('s','u','b','s'), "subs", N_("Subscript"),        gsub_single_mask },
    { CHR('s','u','p','s'), "sups", N_("Superscript"),      gsub_single_mask },
    { CHR('s','w','s','h'), "swsh", N_("Swash"),            gsub_single_mask|gsub_alternate_mask },
    { CHR('t','i','t','l'), "titl", N_("Titling"),          gsub_single_mask },
    { CHR('t','j','m','o'), "tjmo", N_("Trailing Jamo Forms"),  gsub_ligature_mask },
    { CHR('t','n','a','m'), "tnam", N_("Traditional Name Forms"),   gsub_single_mask },
    { CHR('t','n','u','m'), "tnum", N_("Tabular Numbers"),      gsub_single_mask },
    { CHR('t','r','a','d'), "trad", N_("Traditional Forms"),    gsub_single_mask|gsub_alternate_mask },
    { CHR('t','w','i','d'), "twid", N_("Third Widths"),     gsub_single_mask|gpos_single_mask },
    { CHR('u','n','i','c'), "unic", N_("Unicase"),          gsub_single_mask },
{ CHR('v','a','l','t'), "valt", N_("Alternate Vertical Metrics"),   gpos_single_mask },
    { CHR('v','a','t','u'), "vatu", N_("Vattu Variants"),       gsub_ligature_mask },
    { CHR('v','e','r','t'), "vert", N_("Vertical Alternates (obs)"),    gsub_single_mask },
    { CHR('v','h','a','l'), "vhal", N_("Alternate Vertical Half Metrics"),  gpos_single_mask },
    { CHR('v','j','m','o'), "vjmo", N_("Vowel Jamo Forms"),     gsub_ligature_mask },
    { CHR('v','k','n','a'), "vkna", N_("Vertical Kana Alternates"), gsub_single_mask },
    { CHR('v','k','r','n'), "vkrn", N_("Vertical Kerning"),     gpos_pair_mask|gpos_context_mask|gpos_contextchain_mask|kern_statemachine_mask }, 
    { CHR('v','p','a','l'), "vpal", N_("Proportional Alternate Vertical Metrics"),  gpos_single_mask },
    { CHR('v','r','t','2'), "vrt2", N_("Vertical Rotation & Alternates"),   gsub_single_mask },
    { CHR('z','e','r','o'), "zero", N_("Slashed Zero"),     gsub_single_mask },
/* This is my hack for setting the "Required feature" field of a script */
	{ CHR(' ','R','Q','D'),	" RQD", N_("Required feature"),
	gsub_single_mask|gsub_multiple_mask|gsub_alternate_mask|gsub_ligature_mask|gsub_context_mask|gsub_contextchain_mask|gsub_reversecchain_mask|morx_context_mask|gpos_single_mask|gpos_pair_mask|gpos_cursive_mask|gpos_mark2base_mask|gpos_mark2ligature_mask|gpos_mark2mark_mask|gpos_context_mask|gpos_contextchain_mask},
    OPENTYPE_FEATURE_FRIENDLYNAMES_EMPTY
};

static void LookupInit(void) {
	static int done = 0;
	int i, j;

	if ( done )
		return;
	done = 1;
	for ( j=0; j<2; ++j ) {
		for ( i=0; i<10; ++i )
			if ( lookup_type_names[j][i]!=NULL )
				lookup_type_names[j][i] = S_((char *) lookup_type_names[j][i]);
	}
	for ( i=0; localscripts[i].text!=NULL; ++i )
		localscripts[i].text = S_(localscripts[i].text);
	for ( i=0; friendlies[i].friendlyname!=NULL; ++i )
		friendlies[i].friendlyname = S_(friendlies[i].friendlyname);
}

int DefaultLangTagInOneScriptList(struct scriptlanglist *sl) {
	int l;

	for ( l=0; l<sl->lang_cnt; ++l ) {
		uint32 lang = l<MAX_LANG ? sl->langs[l] : sl->morelangs[l-MAX_LANG];
		if ( lang==DEFAULT_LANG )
			return( true );
	}
	return( false );
}

static struct scriptlanglist *DefaultLangTagInScriptList(struct scriptlanglist *sl, int DFLT_ok) {

	while ( sl!=NULL ) {
		if ( DFLT_ok || sl->script!=DEFAULT_SCRIPT ) {
			if ( DefaultLangTagInOneScriptList(sl))
				return( sl );
		}
		sl = sl->next;
	}
	return( NULL );
}

static char *copy(const char *str) { 
	return str ? strdup(str) : NULL;
}

static char *TagFullName(uint32 tag, int ismac, int onlyifknown) {
	char ubuf[200], *end = ubuf+sizeof(ubuf);
	int k;

	if ( ismac==-1 )
		/* Guess */
		ismac = (tag>>24)<' ' || (tag>>24)>0x7e;

	if ( ismac ) {
		return NULL;
	} else {
		uint32 stag = tag;
		if ( tag==CHR('n','u','t','f') )
			stag = CHR('a','f','r','c');
		if ( tag==REQUIRED_FEATURE ) {
			strcpy(ubuf,"Required Feature");
		} else {
			LookupInit();
			for ( k=0; friendlies[k].tag!=0; ++k ) {
				if ( friendlies[k].tag == stag )
					break;
			}

			ubuf[0] = '\'';
			ubuf[1] = tag>>24;
			ubuf[2] = (tag>>16)&0xff;
			ubuf[3] = (tag>>8)&0xff;
			ubuf[4] = tag&0xff;
			ubuf[5] = '\'';
			ubuf[6] = ' ';
			if ( friendlies[k].tag!=0 )
				strncpy(ubuf+7, (char *)
						friendlies[k].friendlyname,end-ubuf-7);
			else if ( onlyifknown )
				return( NULL );
			else
				ubuf[7]='\0';
		}
	}
	return( copy( ubuf ));
}

char *SuffixFromTags(FeatureScriptLangList *fl)
{
	static struct { uint32 tag; const char *suffix; } tags2suffix[] = {
		{ CHR('v','r','t','2'), "vert" },   /* Will check for vrt2 later */
		{ CHR('o','n','u','m'), "oldstyle" },
		{ CHR('s','u','p','s'), "superior" },
		{ CHR('s','u','b','s'), "inferior" },
		{ CHR('s','w','s','h'), "swash" },
		{ CHR('f','w','i','d'), "full" },
		{ CHR('h','w','i','d'), "hw" },
		{ 0, NULL }
	};
	int i;

	while ( fl!=NULL ) {
		for ( i=0; tags2suffix[i].tag!=0; ++i )
			if ( tags2suffix[i].tag==fl->featuretag )
				return( copy( tags2suffix[i].suffix ));
		fl = fl->next;
	}
	return( NULL );
}

static void NameOTLookup(OTLookup *otl)
{
	char *userfriendly = NULL, *script;
	FeatureScriptLangList *fl;
	char *lookuptype;
	const char *format;
	struct lookup_subtable *subtable;
	int k;

	LookupInit();

	if ( otl->lookup_name==NULL ) {
		for ( k=0; k<2; ++k ) {
			for ( fl=otl->features; fl!=NULL ; fl=fl->next ) {
				/* look first for a feature attached to a default language */
				if ( k==1 || DefaultLangTagInScriptList(fl->scripts,0)!=NULL ) {
					userfriendly = TagFullName(fl->featuretag, fl->ismac, true);
					if ( userfriendly!=NULL )
						break;
				}
			}
			if ( userfriendly!=NULL )
				break;
		}
		if ( userfriendly==NULL ) {
			if ( (otl->lookup_type&0xff)>= 0xf0 )
				lookuptype = strdup("State Machine");
			else if ( (otl->lookup_type>>8)<2 && (otl->lookup_type&0xff)<10 )
				lookuptype = strdup(lookup_type_names[otl->lookup_type>>8][otl->lookup_type&0xff]);
			else
				lookuptype = S_("LookupType|Unknown");
			for ( fl=otl->features; fl!=NULL && !fl->ismac; fl=fl->next );
			if ( fl==NULL )
				userfriendly = copy(lookuptype);
			else {
				userfriendly = GxCore_Malloc( strlen(lookuptype) + 10);
				sprintf( userfriendly, "%s '%c%c%c%c'", lookuptype,
						fl->featuretag>>24,
						fl->featuretag>>16,
						fl->featuretag>>8 ,
						fl->featuretag );
			}
		}
		script = NULL;
		if ( fl==NULL ) fl = otl->features;
		if ( fl!=NULL && fl->scripts!=NULL ) {
			char buf[8];
			int j;
			struct scriptlanglist *sl, *found, *found2;
			uint32 script_tag = fl->scripts->script;
			found = found2 = NULL;
			for ( sl = fl->scripts; sl!=NULL; sl=sl->next ) {
				if ( sl->script == DEFAULT_SCRIPT )
					/* Ignore it */;
				else if ( DefaultLangTagInOneScriptList(sl)) {
					if ( found==NULL )
						found = sl;
					else {
						found = found2 = NULL;
						break;
					}
				} else if ( found2 == NULL )
					found2 = sl;
				else
					found2 = (struct scriptlanglist *)-1;
			}
			if ( found==NULL && found2!=NULL && found2 != (struct scriptlanglist *) -1 )
				found = found2;
			if ( found!=NULL ) {
				script_tag = found->script;
				for ( j=0; localscripts[j].text!=NULL && script_tag!=localscripts[j].tag; ++j );
				if ( localscripts[j].text!=NULL )
					script = copy( S_((char *) localscripts[j].text) );
				else {
					buf[0] = '\'';
					buf[1] = fl->scripts->script>>24;
					buf[2] = (fl->scripts->script>>16)&0xff;
					buf[3] = (fl->scripts->script>>8)&0xff;
					buf[4] = fl->scripts->script&0xff;
					buf[5] = '\'';
					buf[6] = 0;
					script = copy(buf);
				}
			}
		}

		if ( script!=NULL ) {
			/* GT: This string is used to generate a name for each OpenType lookup. */
			/* GT: The %s will be filled with the user friendly name of the feature used to invoke the lookup */
			/* GT: The second %s (if present) is the script */
			/* GT: While the %d is the index into the lookup list and is used to disambiguate it */
			/* GT: In case that is needed */
			format = strdup("%s in %s lookup %d");
			otl->lookup_name = GxCore_Malloc( strlen(userfriendly)+strlen(format)+strlen(script)+10 );
			sprintf( otl->lookup_name, format, userfriendly, script, otl->lookup_index );
		} else {
			format = strdup("%s lookup %d");
			otl->lookup_name = GxCore_Malloc( strlen(userfriendly)+strlen(format)+10 );
			sprintf( otl->lookup_name, format, userfriendly, otl->lookup_index );
		}
		GxCore_Free(script);
		GxCore_Free(userfriendly);
	}

	if ( otl->subtables==NULL )
		/* IError( _("Lookup with no subtables"))*/;
	else {
		int cnt = 0;
		for ( subtable = otl->subtables; subtable!=NULL; subtable=subtable->next, ++cnt )
			if ( subtable->subtable_name==NULL ) {
				if ( subtable==otl->subtables && subtable->next==NULL )
					/* GT: This string is used to generate a name for an OpenType lookup subtable. */
					/* GT:  %s is the lookup name */
					format = strdup("%s subtable");
				else if ( subtable->per_glyph_pst_or_kern )
					/* GT: This string is used to generate a name for an OpenType lookup subtable. */
					/* GT:  %s is the lookup name, %d is the index of the subtable in the lookup */
					format = strdup("%s per glyph data %d");
				else if ( subtable->kc!=NULL )
					format = strdup("%s kerning class %d");
				else if ( subtable->fpst!=NULL )
					format = strdup("%s contextual %d");
				else if ( subtable->anchor_classes )
					format = strdup("%s anchor %d");
				else {
					GUI_Debug_Print("Subtable status not filled in for %dth subtable of %s\n", cnt, otl->lookup_name);
					format = "%s !!!!!!!! %d";
				}
				subtable->subtable_name = GxCore_Malloc( strlen(otl->lookup_name)+strlen(format)+10 );
				sprintf( subtable->subtable_name, format, otl->lookup_name, cnt );
			}
	}
	if ( otl->lookup_type==gsub_ligature ) {
		for ( fl=otl->features; fl!=NULL; fl=fl->next )
			if ( fl->featuretag==CHR('l','i','g','a') || fl->featuretag==CHR('r','l','i','g'))
				otl->store_in_afm = true;
	}
	if ( otl->lookup_type==gsub_single )
		for ( subtable = otl->subtables; subtable!=NULL; subtable=subtable->next )
			subtable->suffix = SuffixFromTags(otl->features);
}

static uint16 *getCoverageTable(FILE *ttf, int coverage_offset)
{
	int format, cnt, i,j, rcnt;
	uint16 *glyphs=NULL;
	int start, end, ind, max;

	fseek(ttf,coverage_offset,SEEK_SET);
	format = getushort(ttf);
	if ( format==1 ) {
		cnt = getushort(ttf);
		glyphs = GxCore_Malloc((cnt+1)*sizeof(uint16));
		for ( i=0; i<cnt; ++i ) {
			if ( cnt&0xffff0000 ) {
				gxlogd("Bad count.\n");
			}
			glyphs[i] = getushort(ttf);
#ifndef NO_OS
			if ( feof(ttf) ) {
				gxlogd("End of file found in coverage table.\n");
				GxCore_Free(glyphs);
				return( NULL );
			}
#endif /*NO_OS*/
			if ( glyphs[i]>=glyph_cnt ) {
				gxlogd("Bad coverage table. Glyph %d out of range [0,%d)\n", glyphs[i], glyph_cnt);
				glyphs[i] = 0;
			}
		}
	} else if ( format==2 ) {
		glyphs = GxCore_Calloc((max=256),sizeof(uint16));
		rcnt = getushort(ttf); cnt = 0;

		for ( i=0; i<rcnt; ++i ) {
			start = getushort(ttf);
			end = getushort(ttf);
			ind = getushort(ttf);
#ifndef NO_OS
			if ( feof(ttf) ) {
				gxlogd("End of file found in coverage table.\n");
				GxCore_Free(glyphs);
				return( NULL );
			}
#endif /*NO_OS*/
			if ( ind+end-start+2 >= max ) {
				int oldmax = max;
				max = ind+end-start+2;
				glyphs = GxCore_Realloc(glyphs,max*sizeof(uint16));
				memset(glyphs+oldmax,0,(max-oldmax)*sizeof(uint16));
			}
			for ( j=start; j<=end; ++j ) {
				glyphs[j-start+ind] = j;
				if ( j>=glyph_cnt )
					glyphs[j-start+ind] = 0;
			}
			if ( ind+end-start+1>cnt )
				cnt = ind+end-start+1;
		}
	} else {
		gxlogd("Bad format for coverage table %d\n", format );
		return( NULL );
	}
	glyphs[cnt] = 0xffff;

	return( glyphs );
}

static void gsubSimpleSubTable(FILE *ttf, FT_Face face, int stoffset, struct lookup *l, struct lookup_subtable *subtable, int justinuse)
{
	int coverage, cnt, i, which;
	uint16 format;
	uint16 *glyphs, *glyph2s=NULL;
	int delta=0;

	format=getushort(ttf);
	if ( format!=1 && format!=2 )   /* Unknown subtable format */
		return;

	coverage = getushort(ttf);
	if ( format==1 ) {
		delta = getushort(ttf);
	} else {
		cnt = getushort(ttf);
		glyph2s = GxCore_Malloc(cnt*sizeof(uint16));
		for ( i=0; i<cnt; ++i )
			glyph2s[i] = getushort(ttf);
		/* in range check comes later */
	}

	glyphs = getCoverageTable(ttf,stoffset+coverage);
	if ( glyphs==NULL ) {
		GxCore_Free(glyph2s);
		glyph2s = NULL;
		gxlogd(" Bad simple substitution table, ignored\n");
	}

	if ( justinuse==git_findnames ) {
		for ( i=0; glyphs[i]!=0xffff; ++i )
			if ( glyphs[i]<glyph_cnt && chars[glyphs[i]]!=NULL ) {
				which = format==1 ? (uint16) (glyphs[i]+delta) : glyph2s[i];
				if ( which>=glyph_cnt ) {
					gxlogd("Bad substitution glyph: GID %d not less than %d\n", which, glyph_cnt);
					which = 0;
				}

				if ( chars[which]!=NULL && chars[glyphs[i]]!=NULL ) {
					/* Might be in a ttc file */
					PST *pos = chunkalloc(sizeof(PST));
					pos->type = pst_substitution;
					pos->subtable = subtable;
					pos->next = chars[glyphs[i]]->possub;
					chars[glyphs[i]]->possub = pos;
					pos->u.subs.variant = copy(chars[which]->name);
				}
			}
	}
	subtable->per_glyph_pst_or_kern = true;
	if(glyph2s) {
		GxCore_Free(glyph2s);
	}
	GxCore_Free(glyphs);
}

/* Multiple and alternate substitution lookups have the same format */
static void gsubMultipleSubTable(FILE *ttf, int stoffset, struct lookup *l,
								 struct lookup_subtable *subtable, int justinuse){
	int coverage, cnt, i, j, len, max;
	uint16 format;
	uint16 *offsets;
	uint16 *glyphs, *glyph2s;
	char *pt;
	int bad;
	int badcnt = 0;

	if ( justinuse==git_findnames )
		return;

	format=getushort(ttf);
	if ( format!=1 )    /* Unknown subtable format */
		return;

	coverage = getushort(ttf);
	cnt = getushort(ttf);
#ifndef NO_OS
	if ( feof(ttf)) {
		gxlogd("Unexpected end of file in GSUB sub-table.\n");
		return;
	}
#endif /*NO_OS*/

	offsets = GxCore_Malloc(cnt*sizeof(uint16));
	for ( i=0; i<cnt; ++i )
		offsets[i] = getushort(ttf);
	glyphs = getCoverageTable(ttf,stoffset+coverage);
	if ( glyphs==NULL ) {
		gxlogd(" Bad multiple substitution table, ignored\n");
		GxCore_Free(offsets);
		return;
	}

	for ( i=0; glyphs[i]!=0xffff; ++i );
	if ( i!=cnt ) {
		gxlogd("Coverage table specifies a different number of glyphs than the sub-table expects.\n");
		if ( cnt<i )
			glyphs[cnt] = 0xffff;
		else
			cnt = i;
	}
	max = 20;
	glyph2s = GxCore_Malloc(max*sizeof(uint16));
	for ( i=0; glyphs[i]!=0xffff; ++i ) {
		PST *alt;
		fseek(ttf,stoffset+offsets[i],SEEK_SET);
		cnt = getushort(ttf);

#ifndef NO_OS
		if ( feof(ttf)) {
			gxlogd("Unexpected end of file in GSUB sub-table.\n");
			GxCore_Free(offsets);
			GxCore_Free(glyphs);
			GxCore_Free(glyph2s);
			return;
		}
#endif /*NO_OS*/
		if ( cnt>max ) {
			max = cnt+30;
			glyph2s = GxCore_Realloc(glyph2s,max*sizeof(uint16));
		}
		len = 0; bad = false;
		for ( j=0; j<cnt; ++j ) {
			glyph2s[j] = getushort(ttf);
#ifndef NO_OS
			if ( feof(ttf))
			{
				gxlogd("Unexpected end of file in GSUB sub-table.\n");
				GxCore_Free(offsets);
				GxCore_Free(glyphs);
				GxCore_Free(glyph2s);
				return;
			}
#endif /*NO_OS*/
			if ( glyph2s[j]>=glyph_cnt ) {
				if ( !justinuse )
					gxlogd("Bad Multiple/Alternate substitution glyph. GID %d not less than %d\n", glyph2s[j],
							glyph_cnt );
				if ( ++badcnt>20 ) {
					GxCore_Free(offsets);
					GxCore_Free(glyphs);
					GxCore_Free(glyph2s);
					return;
				}
				glyph2s[j] = 0;
			}
			if ( justinuse==git_justinuse )
				/* Do Nothing */;
			else if ( chars[glyph2s[j]]==NULL )
				bad = true;
			else
				len += strlen( chars[glyph2s[j]]->name) +1;
		}
		if ( justinuse==git_justinuse ){
			;
		} else if ( chars[glyphs[i]]!=NULL && !bad ) {
			alt = chunkalloc(sizeof(PST));
			alt->type = l->otlookup->lookup_type==gsub_multiple?pst_multiple:pst_alternate;
			alt->subtable = subtable;
			alt->next = chars[glyphs[i]]->possub;
			chars[glyphs[i]]->possub = alt;
			pt = alt->u.subs.variant = GxCore_Malloc(len+1);
			*pt = '\0';
			for ( j=0; j<cnt; ++j ) {
				strcat(pt,chars[glyph2s[j]]->name);
				strcat(pt," ");
			}
			if ( *pt!='\0' && pt[strlen(pt)-1]==' ' )
				pt[strlen(pt)-1] = '\0';
		}
#ifndef NO_OS
		if (glyphs[i] > glyph_cnt) {
			gxlogf ( "This glyph is out of bounds.\n");
		} else if (chars[glyphs[i]] == NULL || chars[glyphs[i]]->possub == NULL)
		{
			if (justinuse != git_justinuse) fprintf( stderr , "This glyph isn't loaded yet.\n" );
		} else if (chars[glyphs[i]]->possub->u.subs.variant  == NULL) {
			fprintf( stderr , "chars[glyphs[%d]]->possub->u.subs.variant is null. glyphs[%d] = %d. chars[%d]->name = \"%s\".\n", i , i , glyphs[i] , glyphs[i] , chars[glyphs[i]]->name);
		}
#endif /*NO_OS*/
	}
	subtable->per_glyph_pst_or_kern = true;
	GxCore_Free(glyphs);
	GxCore_Free(glyph2s);
	GxCore_Free(offsets);
}

static void gsubLigatureSubTable(FILE *ttf, int stoffset,
								 struct lookup *l, struct lookup_subtable *subtable, int justinuse)
{
	int coverage, cnt, i, j, k, lig_cnt, cc, len;
	uint16 *ls_offsets, *lig_offsets;
	uint16 *glyphs, *lig_glyphs, lig;
	char *pt;
	PST *liga;

	/* Format = */ getushort(ttf);
	coverage = getushort(ttf);
	cnt = getushort(ttf);
#ifndef NO_OS
	if ( feof(ttf)) {
		gxlogd("Unexpected end of file in GSUB ligature sub-table.\n");
		return;
	}
#endif /*NO_OS*/
	ls_offsets = GxCore_Malloc(cnt*sizeof(uint16));
	for ( i=0; i<cnt; ++i )
		ls_offsets[i]=getushort(ttf);
	glyphs = getCoverageTable(ttf,stoffset+coverage);
	if ( glyphs==NULL ) {
		gxlogd(" Bad ligature table, ignored\n");
		GxCore_Free(ls_offsets);
		return;
	}
	for ( i=0; i<cnt; ++i ) {
		fseek(ttf,stoffset+ls_offsets[i],SEEK_SET);
		lig_cnt = getushort(ttf);
#ifndef NO_OS
		if ( feof(ttf)) {
			gxlogd("Unexpected end of file in GSUB ligature sub-table.\n" );
			GxCore_Free(ls_offsets);
			return;
		}
#endif /*NO_OS*/
		lig_offsets = GxCore_Malloc(lig_cnt*sizeof(uint16));
		for ( j=0; j<lig_cnt; ++j )
			lig_offsets[j] = getushort(ttf);
#ifndef NO_OS
		if ( feof(ttf)) {
			gxlogd("Unexpected end of file in GSUB ligature sub-table.\n" );
			GxCore_Free(lig_offsets);
			GxCore_Free(ls_offsets);
			return;
		}
#endif /*NO_OS*/
		for ( j=0; j<lig_cnt; ++j ) {
			fseek(ttf,stoffset+ls_offsets[i]+lig_offsets[j],SEEK_SET);
			lig = getushort(ttf);
			if ( lig>=glyph_cnt ) {
				gxlogd("Bad ligature glyph. GID %d not less than %d\n", lig, glyph_cnt);
				lig = 0;
			}
			cc = getushort(ttf);
			if ( cc<0 || cc>100 ) {
				gxlogd("Unlikely count of ligature components (%d), I suspect this ligature sub-\n table is garbage, I'm giving up on it.\n", cc);
				GxCore_Free(glyphs); GxCore_Free(lig_offsets); GxCore_Free(ls_offsets);
				return;
			}
			lig_glyphs = GxCore_Malloc(cc*sizeof(uint16));
			lig_glyphs[0] = glyphs[i];
			for ( k=1; k<cc; ++k ) {
				lig_glyphs[k] = getushort(ttf);
				if ( lig_glyphs[k]>=glyph_cnt ) {
					gxlogd("Bad ligature component glyph. GID %d not less than %d (in ligature %d)\n", lig_glyphs[k], glyph_cnt, lig);
					lig_glyphs[k] = 0;
				}
			}
			if ( justinuse==git_justinuse ) {
				;
			} else if ( justinuse==git_findnames ) {
				FeatureScriptLangList *fl = l->otlookup->features;
				/* If our ligature glyph has no name (and its components do) */
				/*  give it a name by concatenating components with underscores */
				/*  between them, and appending the tag */
				if ( fl!=NULL && chars[lig]!=NULL && chars[lig]->name==NULL ) {
					int len=0;
					for ( k=0; k<cc; ++k ) {
						if ( chars[lig_glyphs[k]]==NULL || chars[lig_glyphs[k]]->name==NULL )
							break;
						len += strlen(chars[lig_glyphs[k]]->name)+1;
					}
					if ( k==cc ) {
						char *str = GxCore_Malloc(len+6), *pt;
						char tag[5];
						tag[0] = fl->featuretag>>24;
						if ( (tag[1] = (fl->featuretag>>16)&0xff)==' ' ) tag[1] = '\0';
						if ( (tag[2] = (fl->featuretag>>8)&0xff)==' ' ) tag[2] = '\0';
						if ( (tag[3] = (fl->featuretag)&0xff)==' ' ) tag[3] = '\0';
						tag[4] = '\0';
						*str='\0';
						for ( k=0; k<cc; ++k ) {
							strcat(str,chars[lig_glyphs[k]]->name);
							strcat(str,"_");
						}
						pt = str+strlen(str);
						pt[-1] = '.';
						strcpy(pt,tag);
						chars[lig]->name = str;
					}
				}
			} else if ( chars[lig]!=NULL ) {
				int err=false;
				for ( k=len=0; k<cc; ++k )
					if ( lig_glyphs[k]<glyph_cnt &&
						 chars[lig_glyphs[k]]!=NULL )
						len += strlen(chars[lig_glyphs[k]]->name)+1;
					else
						err = true;
				if ( !err ) {
					liga = chunkalloc(sizeof(PST));
					liga->type = pst_ligature;
					liga->subtable = subtable;
					liga->next = chars[lig]->possub;
					chars[lig]->possub = liga;
					liga->u.lig.lig = chars[lig];
					liga->u.lig.components = pt = GxCore_Malloc(len);
					for ( k=0; k<cc; ++k ) {
						if ( lig_glyphs[k]<glyph_cnt &&
							 chars[lig_glyphs[k]]!=NULL ) {
							strcpy(pt,chars[lig_glyphs[k]]->name);
							pt += strlen(pt);
							*pt++ = ' ';
						}
					}
					pt[-1] = '\0';
				}
			}
			GxCore_Free(lig_glyphs);
		}
		GxCore_Free(lig_offsets);
	}
	subtable->per_glyph_pst_or_kern = true;
	GxCore_Free(ls_offsets); GxCore_Free(glyphs);
}

static int cmpuint16(const void *u1, const void *u2) {
	return( ((int) *((const uint16 *) u1)) - ((int) *((const uint16 *) u2)) );
}

static char *GlyphsToNames(uint16 *glyphs,int make_uniq)
{
	int i, j, len, off;
	char *ret, *pt;

	if ( glyphs==NULL )
		return( copy(""));

	/* Adobe produces coverage tables containing duplicate glyphs in */
	/*  GaramondPremrPro.otf. We want unique glyphs, so enforce that */
	if ( make_uniq ) {
		for ( i=0 ; glyphs[i]!=0xffff; ++i );
		qsort(glyphs,i,sizeof(uint16),cmpuint16);
		for ( i=0; glyphs[i]!=0xffff; ++i ) {
			if ( glyphs[i+1]==glyphs[i] ) {
				for ( j=i+1; glyphs[j]==glyphs[i]; ++j );
				off = j-i -1;
				for ( j=i+1; ; ++j ) {
					glyphs[j] = glyphs[j+off];
					if ( glyphs[j]==0xffff )
						break;
				}
			}
		}
	}

	for ( i=len=0 ; glyphs[i]!=0xffff; ++i ) {
		if ( glyphs[i]>=glyph_cnt ) {
			return( copy(""));
		}
		if ( chars[glyphs[i]]!=NULL )
			len += strlen(chars[glyphs[i]]->name)+1;
	}
	ret = pt = GxCore_Malloc(len+1); *pt = '\0';
	for ( i=0 ; glyphs[i]!=0xffff; ++i ) if ( chars[glyphs[i]]!=NULL ) {
		strcpy(pt,chars[glyphs[i]]->name);
		pt += strlen(pt);
		*pt++ = ' ';
	}
	if ( pt>ret ) pt[-1] = '\0';
	return( ret );
}

static void ProcessSubLookups(FILE *ttf,int gpos,
							  struct lookup *alllooks,struct seqlookup *sl) {
	int i;

	i = (intpt) sl->lookup;
	if ( i<0 || i>=lookup_cnt ) {
		gxlogd("Attempt to reference lookup %d (within a contextual lookup), but there are\n only %d lookups in %s\n",
				i, lookup_cnt, gpos ? "'GPOS'" : "'GSUB'" );
		sl->lookup = NULL;
		return;
	}
	sl->lookup = alllooks[i].otlookup;
}

static void g___ContextSubTable1(FILE *ttf, int stoffset,
								 struct lookup *l, struct lookup_subtable *subtable, int justinuse,
								 struct lookup *alllooks, int gpos) {
	int i, j, k, rcnt, cnt;
	uint16 coverage;
	uint16 *glyphs;
	struct subrule {
		uint32 offset;
		int gcnt;
		int scnt;
		uint16 *glyphs;
		struct seqlookup *sl;
	};
	struct rule {
		uint32 offsets;
		int scnt;
		struct subrule *subrules;
	} *rules;
	FPST *fpst;
	struct fpst_rule *rule;
	int warned = false, warned2 = false;

	coverage = getushort(ttf);
	rcnt = getushort(ttf);      /* glyph count in coverage table */
	rules = GxCore_Malloc(rcnt*sizeof(struct rule));
	for ( i=0; i<rcnt; ++i )
		rules[i].offsets = getushort(ttf)+stoffset;
	glyphs = getCoverageTable(ttf,stoffset+coverage);
	if ( glyphs==NULL ) {
		gxlogd(" Bad contextual table, ignored\n");
		GxCore_Free(rules);
		return;
	}
	cnt = 0;
	for ( i=0; i<rcnt; ++i ) {
		fseek(ttf,rules[i].offsets,SEEK_SET);
		rules[i].scnt = getushort(ttf);
		cnt += rules[i].scnt;
		rules[i].subrules = GxCore_Malloc(rules[i].scnt*sizeof(struct subrule));
		for ( j=0; j<rules[i].scnt; ++j )
			rules[i].subrules[j].offset = getushort(ttf)+rules[i].offsets;
		for ( j=0; j<rules[i].scnt; ++j ) {
			fseek(ttf,rules[i].subrules[j].offset,SEEK_SET);
			rules[i].subrules[j].gcnt = getushort(ttf);
			rules[i].subrules[j].scnt = getushort(ttf);
			rules[i].subrules[j].glyphs = GxCore_Malloc((rules[i].subrules[j].gcnt+1)*sizeof(uint16));
			rules[i].subrules[j].glyphs[0] = glyphs[i];
			for ( k=1; k<rules[i].subrules[j].gcnt; ++k ) {
				rules[i].subrules[j].glyphs[k] = getushort(ttf);
				if ( rules[i].subrules[j].glyphs[k]>=glyph_cnt ) {
					if ( !warned )
						gxlogd("Bad contextual or chaining sub table. Glyph %d out of range [0,%d)\n",
								rules[i].subrules[j].glyphs[k], glyph_cnt);
					warned = true;
					rules[i].subrules[j].glyphs[k] = 0;
				}
			}
			rules[i].subrules[j].glyphs[k] = 0xffff;
			rules[i].subrules[j].sl = GxCore_Malloc(rules[i].subrules[j].scnt*sizeof(struct seqlookup));
			for ( k=0; k<rules[i].subrules[j].scnt; ++k ) {
				rules[i].subrules[j].sl[k].seq = getushort(ttf);
				if ( rules[i].subrules[j].sl[k].seq >= rules[i].subrules[j].gcnt+1 )
					if ( !warned2 ) {
						gxlogd("Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d max=%d\n", 
								rules[i].subrules[j].sl[k].seq, rules[i].subrules[j].gcnt );
						warned2 = true;
					}
				rules[i].subrules[j].sl[k].lookup = (void *) (intpt) getushort(ttf);
			}
		}
	}

	if ( justinuse==git_justinuse ) {
		/* Nothing to do. This lookup doesn't really reference any glyphs */
		/*  any lookups it invokes will be processed on their own */
	} else {
		fpst = chunkalloc(sizeof(FPST));
		fpst->type = gpos ? pst_contextpos : pst_contextsub;
		fpst->format = pst_glyphs;
		fpst->subtable = subtable;
		fpst->next = possub;
		possub = fpst;
		subtable->fpst = fpst;

		fpst->rules = rule = GxCore_Calloc(cnt,sizeof(struct fpst_rule));
		fpst->rule_cnt = cnt;

		cnt = 0;
		for ( i=0; i<rcnt; ++i ) for ( j=0; j<rules[i].scnt; ++j ) {
			rule[cnt].u.glyph.names = GlyphsToNames(rules[i].subrules[j].glyphs,false);
			rule[cnt].lookup_cnt = rules[i].subrules[j].scnt;
			rule[cnt].lookups = rules[i].subrules[j].sl;
			rules[i].subrules[j].sl = NULL;
			for ( k=0; k<rule[cnt].lookup_cnt; ++k )
				ProcessSubLookups(ttf,gpos,alllooks,&rule[cnt].lookups[k]);
			++cnt;
		}
	}

	for ( i=0; i<rcnt; ++i ) {
		for ( j=0; j<rules[i].scnt; ++j ) {
			GxCore_Free(rules[i].subrules[j].glyphs);
			GxCore_Free(rules[i].subrules[j].sl);
		}
		GxCore_Free(rules[i].subrules);
	}
	GxCore_Free(rules);
	GxCore_Free(glyphs);
}

static uint16 *getClassDefTable(FILE *ttf, int classdef_offset)
{
	int format, i, j;
	uint16 start, glyphcnt, rangecnt, end, class;
	uint16 *glist=NULL;
	int warned = false;
	int cnt = glyph_cnt;

	fseek(ttf, classdef_offset, SEEK_SET);
	glist = GxCore_Calloc(cnt,sizeof(uint16)); /* Class 0 is default */
	format = getushort(ttf);
	if ( format==1 ) {
		start = getushort(ttf);
		glyphcnt = getushort(ttf);
		if ( start+(int) glyphcnt>cnt ) {
			gxlogd("Bad class def table. start=%d cnt=%d, max glyph=%d\n", start, glyphcnt, cnt );
			glyphcnt = cnt-start;
		}
		for ( i=0; i<glyphcnt; ++i )
			glist[start+i] = getushort(ttf);
	} else if ( format==2 ) {
		rangecnt = getushort(ttf);
		for ( i=0; i<rangecnt; ++i ) {
			start = getushort(ttf);
			end = getushort(ttf);
			if ( start>end || end>=cnt ) {
				gxlogd("Bad class def table. Glyph range %d-%d out of range [0,%d)\n", start, end, cnt );
			}
			class = getushort(ttf);
			for ( j=start; j<=end; ++j ) if ( j<cnt )
				glist[j] = class;
		}
	} else {
		/* Put everything in class 0 and return that */
	}

	/* Do another validity test */
	for ( i=0; i<cnt; ++i ) {
		if ( glist[i]>=cnt+1 ) {
			if ( !warned ) {
				gxlogd("Nonsensical class assigned to a glyph-- class=%d is too big. Glyph=%d\n", glist[i], i );
				warned = true;
			}
			glist[i] = 0;
		}
	}
	return glist;
}

static int ClassFindCnt(uint16 *class,int tot) {
	int i, max=0;

	for ( i=0; i<tot; ++i )
		if ( class[i]>max ) max = class[i];
	return( max+1 );
}

static char **ClassToNames(int class_cnt,uint16 *class,int glyph_cnt) {
	char **ret = GxCore_Malloc(class_cnt*sizeof(char *));
	int *lens = GxCore_Calloc(class_cnt,sizeof(int));
	int i;

	ret[0] = NULL;
	for ( i=0 ; i<glyph_cnt; ++i ) if ( class[i]!=0 && chars[i]!=NULL && class[i]<class_cnt )
		lens[class[i]] += strlen(chars[i]->name)+1;
	for ( i=1; i<class_cnt ; ++i )
		ret[i] = GxCore_Malloc(lens[i]+1);
	memset(lens,0,class_cnt*sizeof(int));
	for ( i=0 ; i<glyph_cnt; ++i ) if ( class[i]!=0 && chars[i]!=NULL ) {
		if ( class[i]<class_cnt ) {
			strcpy(ret[class[i]]+lens[class[i]], chars[i]->name );
			lens[class[i]] += strlen(chars[i]->name)+1;
			ret[class[i]][lens[class[i]]-1] = ' ';
		} else {
			gxlogd("Class index out of range %d (must be <%d)\n",class[i], class_cnt );
		}
	}
	for ( i=1; i<class_cnt ; ++i )
		if ( lens[i]==0 )
			ret[i][0] = '\0';
		else
			ret[i][lens[i]-1] = '\0';
	GxCore_Free(lens);
	return( ret );
}

static char *CoverageMinusClasses(uint16 *coverageglyphs,uint16 *classed){
	int i, j, len;
	uint8 *glyphs = GxCore_Calloc(glyph_cnt,1);
	char *ret;

	for ( i=0; coverageglyphs[i]!=0xffff; ++i )
		glyphs[coverageglyphs[i]] = 1;
	for ( i=0; i<glyph_cnt; ++i )
		if ( classed[i]!=0 )
			glyphs[i] = 0;

	for ( i=0; i<glyph_cnt; ++i )
		if ( glyphs[i]!=0 )
			break;

	/* coverage table matches glyphs in classes. No need for special treatment*/
	if ( i==glyph_cnt ) {
		GxCore_Free(glyphs);
		return( NULL );
	}
	/* Otherwise we need to generate a class string of glyph names in the coverage */
	/*  table but not in any class. These become the glyphs in class 0 */
	ret = NULL;
	for ( j=0; j<2; ++j ) {
		len = 0;
		for ( i=0; i<glyph_cnt; ++i ) {
			if ( glyphs[i]!=0 ) {
				if ( j ) {
					strcpy( ret+len, chars[i]->name );
					strcat( ret+len, " ");
				}
				len += strlen(chars[i]->name)+1;
			}
		}
		if ( j==0 )
			ret = GxCore_Malloc(len+1);
		else
			ret[len-1] = '\0';
	}
	GxCore_Free(glyphs);
	return( ret );
}

static void g___ContextSubTable2(FILE *ttf, int stoffset,
								 struct lookup *l, struct lookup_subtable *subtable, int justinuse,
								 struct lookup *alllooks, int gpos) {
	int i, j, k, rcnt, cnt;
	uint16 coverage;
	uint16 classoff;
	struct subrule {
		uint32 offset;
		int ccnt;
		int scnt;
		uint16 *classindeces;
		struct seqlookup *sl;
	};
	struct rule {
		uint32 offsets;
		int scnt;
		struct subrule *subrules;
	} *rules;
	FPST *fpst;
	struct fpst_rule *rule;
	uint16 *glyphs, *class;
	int warned2 = false;

	coverage = getushort(ttf);
	classoff = getushort(ttf);
	rcnt = getushort(ttf);      /* class count in coverage table *//* == number of top level rules */
	rules = GxCore_Calloc(rcnt,sizeof(struct rule));
	for ( i=0; i<rcnt; ++i )
		rules[i].offsets = getushort(ttf)+stoffset;
	cnt = 0;
	for ( i=0; i<rcnt; ++i ) if ( rules[i].offsets!=stoffset ) { /* some classes might be unused */
		fseek(ttf,rules[i].offsets,SEEK_SET);
		rules[i].scnt = getushort(ttf);
		if ( rules[i].scnt<0 ) {
			gxlogd("Bad count in context chaining sub-table.\n");
			GxCore_Free(rules);
			return;
		}
		cnt += rules[i].scnt;
		rules[i].subrules = GxCore_Malloc(rules[i].scnt*sizeof(struct subrule));
		for ( j=0; j<rules[i].scnt; ++j )
			rules[i].subrules[j].offset = getushort(ttf)+rules[i].offsets;
		for ( j=0; j<rules[i].scnt; ++j ) {
			fseek(ttf,rules[i].subrules[j].offset,SEEK_SET);
			rules[i].subrules[j].ccnt = getushort(ttf);
			rules[i].subrules[j].scnt = getushort(ttf);
			if ( rules[i].subrules[j].ccnt<0 ) {
				gxlogd("Bad class count in contextual chaining sub-table.\n");
				GxCore_Free(rules);
				return;
			}
			rules[i].subrules[j].classindeces = GxCore_Mallocz(rules[i].subrules[j].ccnt*sizeof(uint16));
			rules[i].subrules[j].classindeces[0] = i;
			for ( k=1; k<rules[i].subrules[j].ccnt; ++k )
				rules[i].subrules[j].classindeces[k] = getushort(ttf);
			if ( rules[i].subrules[j].scnt<0 ) {
				gxlogd("Bad count in contextual chaining sub-table.\n");
				GxCore_Free(rules);
				return;
			}
			rules[i].subrules[j].sl = GxCore_Mallocz(rules[i].subrules[j].scnt*sizeof(struct seqlookup));
			for ( k=0; k<rules[i].subrules[j].scnt; ++k ) {
				rules[i].subrules[j].sl[k].seq = getushort(ttf);
				if ( rules[i].subrules[j].sl[k].seq >= rules[i].subrules[j].ccnt )
					if ( !warned2 ) {
						gxlogd("Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d max=%d\n",
							    rules[i].subrules[j].sl[k].seq, rules[i].subrules[j].ccnt-1);
						warned2 = true;
					}
				rules[i].subrules[j].sl[k].lookup = (void *) (intpt) getushort(ttf);
			}
		}
	}

	if ( justinuse==git_justinuse ) {
		/* Nothing to do. This lookup doesn't really reference any glyphs */
		/*  any lookups it invokes will be processed on their own */
	} else {
		fpst = chunkalloc(sizeof(FPST));
		fpst->type = gpos ? pst_contextpos : pst_contextsub;
		fpst->format = pst_class;
		fpst->subtable = subtable;
		subtable->fpst = fpst;
		fpst->next = possub;
		possub = fpst;

		fpst->rules = rule = GxCore_Calloc(cnt,sizeof(struct fpst_rule));
		fpst->rule_cnt = cnt;
		class = getClassDefTable(ttf, stoffset+classoff);
		fpst->nccnt = ClassFindCnt(class,glyph_cnt);
		fpst->nclass = ClassToNames(fpst->nccnt,class,glyph_cnt);
		fpst->nclassnames = GxCore_Calloc(fpst->nccnt,sizeof(char *));

		/* Just in case they used the coverage table to redefine class 0 */
		glyphs = getCoverageTable(ttf,stoffset+coverage);
		if ( glyphs==NULL ) {
			/* GT: This continues a multi-line error message, hence the leading space */
			gxlogd(" Bad contextual substitution table, ignored\n");
			GxCore_Free(rules);
			return;
		}
		fpst->nclass[0] = CoverageMinusClasses(glyphs,class);
		GxCore_Free(glyphs); GxCore_Free(class); class = NULL;

		cnt = 0;
		for ( i=0; i<rcnt; ++i ) for ( j=0; j<rules[i].scnt; ++j ) {
			rule[cnt].u.class.nclasses = rules[i].subrules[j].classindeces;
			rule[cnt].u.class.ncnt = rules[i].subrules[j].ccnt;
			rules[i].subrules[j].classindeces = NULL;
			rule[cnt].lookup_cnt = rules[i].subrules[j].scnt;
			rule[cnt].lookups = rules[i].subrules[j].sl;
			rules[i].subrules[j].sl = NULL;
			for ( k=0; k<rule[cnt].lookup_cnt; ++k )
				ProcessSubLookups(ttf,gpos,alllooks,&rule[cnt].lookups[k]);
			++cnt;
		}
	}

	for ( i=0; i<rcnt; ++i ) {
		for ( j=0; j<rules[i].scnt; ++j ) {
			GxCore_Free(rules[i].subrules[j].classindeces);
			GxCore_Free(rules[i].subrules[j].sl);
		}
		GxCore_Free(rules[i].subrules);
	}
	GxCore_Free(rules);
}

static void g___ContextSubTable3(FILE *ttf, int stoffset,
								 struct lookup *l, struct lookup_subtable *subtable, int justinuse,
								 struct lookup *alllooks, int gpos) {
	int i, k, scnt, gcnt;
	uint16 *coverage;
	struct seqlookup *sl;
	uint16 *glyphs;
	FPST *fpst;
	struct fpst_rule *rule;
	int warned2 = false;

	gcnt = getushort(ttf);
	scnt = getushort(ttf);
#ifndef NO_OS
	if ( feof(ttf) ) {
		gxlogd("End of file in context chaining sub-table.\n");
		return;
	}
#endif /*NO_OS*/
	coverage = GxCore_Mallocz(gcnt*sizeof(uint16));
	for ( i=0; i<gcnt; ++i )
		coverage[i] = getushort(ttf);
	sl = GxCore_Mallocz(scnt*sizeof(struct seqlookup));
	for ( k=0; k<scnt; ++k ) {
		sl[k].seq = getushort(ttf);
		if ( sl[k].seq >= gcnt && !warned2 ) {
			gxlogd("Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d, max=%d\n", sl[k].seq, gcnt-1 );
			warned2 = true;
		}
		sl[k].lookup = (void *) (intpt) getushort(ttf);
	}

	if ( justinuse==git_justinuse ) {
		/* Nothing to do. This lookup doesn't really reference any glyphs */
		/*  any lookups it invokes will be processed on their own */
		GxCore_Free(sl);
	} else {
		fpst = chunkalloc(sizeof(FPST));
		fpst->type = gpos ? pst_contextpos : pst_contextsub;
		fpst->format = pst_coverage;
		fpst->subtable = subtable;
		subtable->fpst = fpst;
		fpst->next = possub;
		possub = fpst;

		fpst->rules = rule = GxCore_Calloc(1,sizeof(struct fpst_rule));
		fpst->rule_cnt = 1;
		rule->u.coverage.ncnt = gcnt;
		rule->u.coverage.ncovers = GxCore_Mallocz(gcnt*sizeof(char *));
		for ( i=0; i<gcnt; ++i ) {
			glyphs =  getCoverageTable(ttf,stoffset+coverage[i]);
			rule->u.coverage.ncovers[i] = GlyphsToNames(glyphs,true);
			GxCore_Free(glyphs);
		}
		rule->lookup_cnt = scnt;
		rule->lookups = sl;
		for ( k=0; k<scnt; ++k )
			ProcessSubLookups(ttf,gpos,alllooks,&sl[k]);
	}

	GxCore_Free(coverage);
}

static void gsubContextSubTable(FILE *ttf, int stoffset,
								struct lookup *l, struct lookup_subtable *subtable,
								int justinuse, struct lookup *alllooks) {
	if ( justinuse==git_findnames )
		return;
	/* Don't give names to these guys, they might not be unique */
	/* ie. because these are context based there is not a one to one */
	/*  mapping between input glyphs and output glyphs. One input glyph */
	/*  may go to several output glyphs (depending on context) and so */
	/*  <input-glyph-name>"."<tag-name> would be used for several glyphs */
	switch( getushort(ttf)) {
	case 1:
		g___ContextSubTable1(ttf,stoffset,l,subtable,justinuse,alllooks,false);
		break;
	case 2:
		g___ContextSubTable2(ttf,stoffset,l,subtable,justinuse,alllooks,false);
		break;
	case 3:
		g___ContextSubTable3(ttf,stoffset,l,subtable,justinuse,alllooks,false);
		break;
	}
}

static void g___ChainingSubTable1(FILE *ttf, int stoffset,
								  struct lookup *l, struct lookup_subtable *subtable, int justinuse,
								  struct lookup *alllooks, int gpos) {
	int i, j, k, rcnt, cnt, which;
	uint16 coverage;
	uint16 *glyphs;
	struct subrule {
		uint32 offset;
		int gcnt, bcnt, fcnt;
		int scnt;
		uint16 *glyphs, *bglyphs, *fglyphs;
		struct seqlookup *sl;
	};
	struct rule {
		uint32 offsets;
		int scnt;
		struct subrule *subrules;
	} *rules;
	FPST *fpst;
	struct fpst_rule *rule;
	int warned = false, warned2 = false;

	coverage = getushort(ttf);
	rcnt = getushort(ttf);      /* glyph count in coverage table */
	rules = GxCore_Mallocz(rcnt*sizeof(struct rule));
	for ( i=0; i<rcnt; ++i )
		rules[i].offsets = getushort(ttf)+stoffset;
	glyphs = getCoverageTable(ttf,stoffset+coverage);
	if ( glyphs==NULL ) {
		GxCore_Free(rules);
		gxlogd(" Bad contextual chaining table, ignored\n");
		return;
	}
	cnt = 0;
	for ( i=0; i<rcnt; ++i ) {
		fseek(ttf,rules[i].offsets,SEEK_SET);
		rules[i].scnt = getushort(ttf);
		cnt += rules[i].scnt;
		rules[i].subrules = GxCore_Mallocz(rules[i].scnt*sizeof(struct subrule));
		for ( j=0; j<rules[i].scnt; ++j )
			rules[i].subrules[j].offset = getushort(ttf)+rules[i].offsets;
		for ( j=0; j<rules[i].scnt; ++j ) {
			fseek(ttf,rules[i].subrules[j].offset,SEEK_SET);
			rules[i].subrules[j].bcnt = getushort(ttf);
#ifndef NO_OS
			if ( feof(ttf)) {
				gxlogd("Unexpected end of file in contextual chaining subtable.\n");
				return;
			}
#endif /*NO_OS*/
			rules[i].subrules[j].bglyphs = GxCore_Mallocz((rules[i].subrules[j].bcnt+1)*sizeof(uint16));
			for ( k=0; k<rules[i].subrules[j].bcnt; ++k )
				rules[i].subrules[j].bglyphs[k] = getushort(ttf);
			rules[i].subrules[j].bglyphs[k] = 0xffff;

			rules[i].subrules[j].gcnt = getushort(ttf);
#ifndef NO_OS
			if ( feof(ttf)) {
				gxlogd("Unexpected end of file in contextual chaining subtable.\n" );
				return;
			}
#endif /*NO_OS*/

			rules[i].subrules[j].glyphs = GxCore_Mallocz((rules[i].subrules[j].gcnt+1)*sizeof(uint16));
			rules[i].subrules[j].glyphs[0] = glyphs[i];
			for ( k=1; k<rules[i].subrules[j].gcnt; ++k )
				rules[i].subrules[j].glyphs[k] = getushort(ttf);
			rules[i].subrules[j].glyphs[k] = 0xffff;

			rules[i].subrules[j].fcnt = getushort(ttf);
#ifndef NO_OS
			if ( feof(ttf)) {
				gxlogd("Unexpected end of file in contextual chaining subtable.\n" );
				return;
			}
#endif /*NO_OS*/
			rules[i].subrules[j].fglyphs = GxCore_Mallocz((rules[i].subrules[j].fcnt+1)*sizeof(uint16));
			for ( k=0; k<rules[i].subrules[j].fcnt; ++k )
				rules[i].subrules[j].fglyphs[k] = getushort(ttf);
			rules[i].subrules[j].fglyphs[k] = 0xffff;

			for ( which = 0; which<3; ++which ) {
				for ( k=0; k<(&rules[i].subrules[j].gcnt)[which]; ++k ) {
					if ( (&rules[i].subrules[j].glyphs)[which][k]>=glyph_cnt ) {
						if ( !warned )
							gxlogd("Bad contextual or chaining sub table. Glyph %d out of range [0,%d)\n",
								   (&rules[i].subrules[j].glyphs)[which][k], glyph_cnt );
						warned = true;
						(&rules[i].subrules[j].glyphs)[which][k] = 0;
					}
				}
			}

			rules[i].subrules[j].scnt = getushort(ttf);
#ifndef NO_OS
			if ( feof(ttf)) {
				gxlogd("Unexpected end of file in contextual chaining subtable.\n");
				return;
			}
#endif /*NO_OS*/
			rules[i].subrules[j].sl = GxCore_Mallocz(rules[i].subrules[j].scnt*sizeof(struct seqlookup));
			for ( k=0; k<rules[i].subrules[j].scnt; ++k ) {
				rules[i].subrules[j].sl[k].seq = getushort(ttf);
				if ( rules[i].subrules[j].sl[k].seq >= rules[i].subrules[j].gcnt+1 )
					if ( !warned2 ) {
						gxlogd("Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d max=%d\n",
								rules[i].subrules[j].sl[k].seq, rules[i].subrules[j].gcnt );
						warned2 = true;
					}
				rules[i].subrules[j].sl[k].lookup = (void *) (intpt) getushort(ttf);
			}
		}
	}

	if ( justinuse==git_justinuse ) {
		/* Nothing to do. This lookup doesn't really reference any glyphs */
		/*  any lookups it invokes will be processed on their own */
	} else {
		fpst = chunkalloc(sizeof(FPST));
		fpst->type = gpos ? pst_chainpos : pst_chainsub;
		fpst->format = pst_glyphs;
		fpst->subtable = subtable;
		fpst->next = possub;
		possub = fpst;
		subtable->fpst = fpst;

		fpst->rules = rule = GxCore_Calloc(cnt,sizeof(struct fpst_rule));
		fpst->rule_cnt = cnt;

		cnt = 0;
		for ( i=0; i<rcnt; ++i ) for ( j=0; j<rules[i].scnt; ++j ) {
			rule[cnt].u.glyph.back = GlyphsToNames(rules[i].subrules[j].bglyphs,false);
			rule[cnt].u.glyph.names = GlyphsToNames(rules[i].subrules[j].glyphs,false);
			rule[cnt].u.glyph.fore = GlyphsToNames(rules[i].subrules[j].fglyphs,false);
			rule[cnt].lookup_cnt = rules[i].subrules[j].scnt;
			rule[cnt].lookups = rules[i].subrules[j].sl;
			rules[i].subrules[j].sl = NULL;
			for ( k=0; k<rule[cnt].lookup_cnt; ++k )
				ProcessSubLookups(ttf,gpos,alllooks,&rule[cnt].lookups[k]);
			++cnt;
		}
	}

	for ( i=0; i<rcnt; ++i ) {
		for ( j=0; j<rules[i].scnt; ++j ) {
			GxCore_Free(rules[i].subrules[j].bglyphs);
			GxCore_Free(rules[i].subrules[j].glyphs);
			GxCore_Free(rules[i].subrules[j].fglyphs);
			GxCore_Free(rules[i].subrules[j].sl);
		}
		GxCore_Free(rules[i].subrules);
	}
	GxCore_Free(rules);
	GxCore_Free(glyphs);
}

static void g___ChainingSubTable2(FILE *ttf, int stoffset,
								  struct lookup *l, struct lookup_subtable *subtable, int justinuse,
								  struct lookup *alllooks, int gpos) {
	int i, j, k, rcnt, cnt;
	uint16 coverage, offset;
	uint16 bclassoff, classoff, fclassoff;
	struct subrule {
		uint32 offset;
		int ccnt, bccnt, fccnt;
		int scnt;
		uint16 *classindeces, *bci, *fci;
		struct seqlookup *sl;
	};
	struct rule {
		uint32 offsets;
		int scnt;
		struct subrule *subrules;
	} *rules;
	FPST *fpst;
	struct fpst_rule *rule;
	uint16 *glyphs, *class;
	int warned2 = false;

	coverage = getushort(ttf);
	bclassoff = getushort(ttf);
	classoff = getushort(ttf);
	fclassoff = getushort(ttf);
	rcnt = getushort(ttf);      /* class count *//* == max number of top level rules */
	rules = GxCore_Calloc(rcnt,sizeof(struct rule));
	for ( i=0; i<rcnt; ++i ) {
		offset = getushort(ttf);
		rules[i].offsets = offset==0 ? 0 : offset+stoffset;
	}
	cnt = 0;
	for ( i=0; i<rcnt; ++i ) if ( rules[i].offsets!=0 ) { /* some classes might be unused */
		fseek(ttf,rules[i].offsets,SEEK_SET);
		rules[i].scnt = getushort(ttf);
		if ( rules[i].scnt<0 ) {
			gxlogd("Bad count in context chaining sub-table.\n");
			GxCore_Free(rules);
			return;
		}
		cnt += rules[i].scnt;
		rules[i].subrules = GxCore_Mallocz(rules[i].scnt*sizeof(struct subrule));
		for ( j=0; j<rules[i].scnt; ++j )
			rules[i].subrules[j].offset = getushort(ttf)+rules[i].offsets;
		for ( j=0; j<rules[i].scnt; ++j ) {
			fseek(ttf,rules[i].subrules[j].offset,SEEK_SET);
			rules[i].subrules[j].bccnt = getushort(ttf);
			if ( rules[i].subrules[j].bccnt<0 ) {
				gxlogd("Bad class count in contextual chaining sub-table.\n");
				GxCore_Free(rules);
				return;
			}
			rules[i].subrules[j].bci = GxCore_Mallocz(rules[i].subrules[j].bccnt*sizeof(uint16));
			for ( k=0; k<rules[i].subrules[j].bccnt; ++k )
				rules[i].subrules[j].bci[k] = getushort(ttf);
			rules[i].subrules[j].ccnt = getushort(ttf);
			if ( rules[i].subrules[j].bccnt<0 ) {
				gxlogd("Bad class count in contextual chaining sub-table.\n");
				GxCore_Free(rules);
				return;
			}
			rules[i].subrules[j].classindeces = GxCore_Mallocz(rules[i].subrules[j].ccnt*sizeof(uint16));
			rules[i].subrules[j].classindeces[0] = i;
			for ( k=1; k<rules[i].subrules[j].ccnt; ++k )
				rules[i].subrules[j].classindeces[k] = getushort(ttf);
			rules[i].subrules[j].fccnt = getushort(ttf);
			if ( rules[i].subrules[j].bccnt<0 ) {
				gxlogd("Bad class count in contextual chaining sub-table.\n");
				GxCore_Free(rules);
				return;
			}
			rules[i].subrules[j].sl = GxCore_Mallocz(rules[i].subrules[j].scnt*sizeof(struct seqlookup));
			for ( k=0; k<rules[i].subrules[j].scnt; ++k ) {
				rules[i].subrules[j].sl[k].seq = getushort(ttf);
				if ( rules[i].subrules[j].sl[k].seq >= rules[i].subrules[j].ccnt )
					if ( !warned2 ) {
						gxlogd("Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d max=%d\n",
								rules[i].subrules[j].sl[k].seq, rules[i].subrules[j].ccnt-1);
						warned2 = true;
					}
				rules[i].subrules[j].sl[k].lookup = (void *) (intpt) getushort(ttf);
			}
		}
	}

	if ( justinuse==git_justinuse ) {
		/* Nothing to do. This lookup doesn't really reference any glyphs */
		/*  any lookups it invokes will be processed on their own */
	} else {
		fpst = chunkalloc(sizeof(FPST));
		fpst->type = gpos ? pst_chainpos : pst_chainsub;
		fpst->format = pst_class;
		fpst->subtable = subtable;
		subtable->fpst = fpst;
		fpst->next = possub;
		possub = fpst;

		fpst->rules = rule = GxCore_Calloc(cnt,sizeof(struct fpst_rule));
		fpst->rule_cnt = cnt;

		class = getClassDefTable(ttf, stoffset+classoff);
		fpst->nccnt = ClassFindCnt(class,glyph_cnt);
		fpst->nclass = ClassToNames(fpst->nccnt,class,glyph_cnt);
		fpst->nclassnames = GxCore_Calloc(fpst->nccnt,sizeof(char *));

		/* Just in case they used the coverage table to redefine class 0 */
		glyphs = getCoverageTable(ttf,stoffset+coverage);
		if ( glyphs==NULL ) {
			gxlogd(" Bad contextual chaining substitution table, ignored\n");
			GxCore_Free(rules);
			return;
		}
		fpst->nclass[0] = CoverageMinusClasses(glyphs,class);
		GxCore_Free(glyphs); GxCore_Free(class); class = NULL;

		/* The docs don't mention this, but in mangal.ttf fclassoff==0 NULL */
		if ( bclassoff!=0 )
			class = getClassDefTable(ttf, stoffset+bclassoff);
		else
			class = GxCore_Calloc(glyph_cnt,sizeof(uint16));
		fpst->bccnt = ClassFindCnt(class,glyph_cnt);
		fpst->bclass = ClassToNames(fpst->bccnt,class,glyph_cnt);
		fpst->bclassnames = GxCore_Calloc(fpst->bccnt,sizeof(char *));
		GxCore_Free(class);

		for ( i=0; i<rcnt; ++i ) for ( j=0; j<rules[i].scnt; ++j ) {
			rule[cnt].u.class.nclasses = rules[i].subrules[j].classindeces;
			rule[cnt].u.class.ncnt = rules[i].subrules[j].ccnt;
			rules[i].subrules[j].classindeces = NULL;
			rule[cnt].u.class.bclasses = rules[i].subrules[j].bci;
			rule[cnt].u.class.bcnt = rules[i].subrules[j].bccnt;
			rules[i].subrules[j].bci = NULL;
			rule[cnt].u.class.fclasses = rules[i].subrules[j].fci;
			rule[cnt].u.class.fcnt = rules[i].subrules[j].fccnt;
			rules[i].subrules[j].fci = NULL;
			rule[cnt].lookup_cnt = rules[i].subrules[j].scnt;
			rule[cnt].lookups = rules[i].subrules[j].sl;
			rules[i].subrules[j].sl = NULL;
			for ( k=0; k<rule[cnt].lookup_cnt; ++k )
				ProcessSubLookups(ttf,gpos,alllooks,&rule[cnt].lookups[k]);
			++cnt;
		}
	}
	for ( i=0; i<rcnt; ++i ) {
		for ( j=0; j<rules[i].scnt; ++j ) {
			GxCore_Free(rules[i].subrules[j].classindeces);
			GxCore_Free(rules[i].subrules[j].sl);
		}
		GxCore_Free(rules[i].subrules);
	}
	GxCore_Free(rules);
}

static void g___ChainingSubTable3(FILE *ttf, int stoffset,
								  struct lookup *l, struct lookup_subtable *subtable, int justinuse,
								  struct lookup *alllooks, int gpos) {
	int i, k, scnt, gcnt, bcnt, fcnt;
	uint16 *coverage, *bcoverage, *fcoverage;
	struct seqlookup *sl;
	uint16 *glyphs;
	FPST *fpst;
	struct fpst_rule *rule;
	int warned2 = false;

	bcnt = getushort(ttf);
#ifndef NO_OS
	if ( feof(ttf)) {
		gxlogd("End of file in context chaining subtable.\n");
		return;
	}
#endif /*NO_OS*/
	bcoverage = GxCore_Mallocz(bcnt*sizeof(uint16));
	for ( i=0; i<bcnt; ++i )
		bcoverage[i] = getushort(ttf);
	gcnt = getushort(ttf);
#ifndef NO_OS
	if ( feof(ttf)) {
		gxlogd("End of file in context chaining subtable.\n");
		GxCore_Free(bcoverage);
		return;
	}
#endif /*NO_OS*/
	coverage = GxCore_Mallocz(gcnt*sizeof(uint16));
	for ( i=0; i<gcnt; ++i )
		coverage[i] = getushort(ttf);
	fcnt = getushort(ttf);
#ifndef NO_OS
	if ( feof(ttf)) {
		gxlogd("End of file in context chaining subtable.\n");
		GxCore_Free(bcoverage);
		GxCore_Free(coverage);
		return;
	}
#endif /*NO_OS*/
	fcoverage = GxCore_Mallocz(fcnt*sizeof(uint16));
	for ( i=0; i<fcnt; ++i )
		fcoverage[i] = getushort(ttf);
	scnt = getushort(ttf);
#ifndef NO_OS
	if ( feof(ttf)) {
		gxlogd("End of file in context chaining subtable.\n");
		GxCore_Free(fcoverage);
		GxCore_Free(bcoverage);
		GxCore_Free(coverage);
		return;
	}
#endif /*NO_OS*/
	sl = GxCore_Mallocz(scnt*sizeof(struct seqlookup));
	for ( k=0; k<scnt; ++k ) {
		sl[k].seq = getushort(ttf);
		if ( sl[k].seq >= gcnt && !warned2 ) {
			gxlogd("Attempt to apply a lookup to a location out of the range of this contextual\n lookup seq=%d, max=%d\n",
					sl[k].seq, gcnt-1 );
			warned2 = true;
		}
		sl[k].lookup = (void *) (intpt) getushort(ttf);
	}

	if ( justinuse==git_justinuse ) {
		/* Nothing to do. This lookup doesn't really reference any glyphs */
		/*  any lookups it invokes will be processed on their own */
		GxCore_Free(sl);
	} else {
		fpst = chunkalloc(sizeof(FPST));
		fpst->type = gpos ? pst_chainpos : pst_chainsub;
		fpst->format = pst_coverage;
		fpst->subtable = subtable;
		subtable->fpst = fpst;
		fpst->next = possub;
		possub = fpst;

		fpst->rules = rule = GxCore_Calloc(1,sizeof(struct fpst_rule));
		fpst->rule_cnt = 1;

		rule->u.coverage.bcnt = bcnt;
		rule->u.coverage.bcovers = GxCore_Mallocz(bcnt*sizeof(char *));
		for ( i=0; i<gcnt; ++i ) {
			glyphs =  getCoverageTable(ttf,stoffset+coverage[i]);
			rule->u.coverage.bcovers[i] = GlyphsToNames(glyphs,true);
			GxCore_Free(glyphs);
		}


		rule->u.coverage.ncnt = gcnt;
		rule->u.coverage.ncovers = GxCore_Mallocz(gcnt*sizeof(char *));
		for ( i=0; i<gcnt; ++i ) {
			glyphs =  getCoverageTable(ttf,stoffset+coverage[i]);
			rule->u.coverage.ncovers[i] = GlyphsToNames(glyphs,true);
			GxCore_Free(glyphs);
		}

		rule->u.coverage.fcnt = fcnt;
		rule->u.coverage.fcovers = GxCore_Mallocz(fcnt*sizeof(char *));
		for ( i=0; i<fcnt; ++i ) {
			glyphs =  getCoverageTable(ttf,stoffset+fcoverage[i]);
			rule->u.coverage.fcovers[i] = GlyphsToNames(glyphs,true);
			GxCore_Free(glyphs);
		}

		rule->lookup_cnt = scnt;
		rule->lookups = sl;
		for ( k=0; k<scnt; ++k )
			ProcessSubLookups(ttf,gpos,alllooks,&sl[k]);
	}

	GxCore_Free(bcoverage);
	GxCore_Free(coverage);
	GxCore_Free(fcoverage);
}

static void gsubChainingSubTable(FILE *ttf, int stoffset,
								 struct lookup *l, struct lookup_subtable *subtable,
								 int justinuse, struct lookup *alllooks) {
	if ( justinuse==git_findnames )
		return;     /* Don't give names to these guys, the names might not be unique */
	switch( getushort(ttf)) {
	case 1:
		g___ChainingSubTable1(ttf,stoffset,l,subtable,justinuse,alllooks,false);
		break;
	case 2:
		g___ChainingSubTable2(ttf,stoffset,l,subtable,justinuse,alllooks,false);
		break;
	case 3:
		g___ChainingSubTable3(ttf,stoffset,l,subtable,justinuse,alllooks,false);
		break;
	}
}

static void gsubReverseChainSubTable(FILE *ttf, int stoffset,
									 struct lookup *l, struct lookup_subtable *subtable, int justinuse) {
	int scnt, bcnt, fcnt, i;
	uint16 coverage, *bcoverage, *fcoverage, *sglyphs, *glyphs;
	FPST *fpst;
	struct fpst_rule *rule;

	if ( justinuse==git_findnames )
		return;     /* Don't give names to these guys, they might not be unique */
	if ( getushort(ttf)!=1 )
		return;     /* Don't understand this format type */

	coverage = getushort(ttf);
	bcnt = getushort(ttf);
	bcoverage = GxCore_Mallocz(bcnt*sizeof(uint16));
	for ( i = 0 ; i<bcnt; ++i )
		bcoverage[i] = getushort(ttf);
	fcnt = getushort(ttf);
	fcoverage = GxCore_Mallocz(fcnt*sizeof(uint16));
	for ( i = 0 ; i<fcnt; ++i )
		fcoverage[i] = getushort(ttf);
	scnt = getushort(ttf);
	sglyphs = GxCore_Mallocz((scnt+1)*sizeof(uint16));
	for ( i = 0 ; i<scnt; ++i )
		if (( sglyphs[i] = getushort(ttf))>=glyph_cnt ) {
			gxlogd("Bad reverse contextual chaining substitution glyph: %d is not less than %d\n",
				   sglyphs[i], glyph_cnt );
			sglyphs[i] = 0;
		}
	sglyphs[i] = 0xffff;

	if ( justinuse==git_justinuse ) {
		;
	} else {
		fpst = chunkalloc(sizeof(FPST));
		fpst->type = pst_reversesub;
		fpst->format = pst_reversecoverage;
		fpst->subtable = subtable;
		fpst->next = possub;
		possub = fpst;
		subtable->fpst = fpst;

		fpst->rules = rule = GxCore_Calloc(1,sizeof(struct fpst_rule));
		fpst->rule_cnt = 1;

		rule->u.rcoverage.always1 = 1;
		rule->u.rcoverage.bcnt = bcnt;
		rule->u.rcoverage.fcnt = fcnt;
		rule->u.rcoverage.ncovers = GxCore_Mallocz(sizeof(char *));
		rule->u.rcoverage.bcovers = GxCore_Mallocz(bcnt*sizeof(char *));
		rule->u.rcoverage.fcovers = GxCore_Mallocz(fcnt*sizeof(char *));
		rule->u.rcoverage.replacements = GlyphsToNames(sglyphs,false);
		glyphs = getCoverageTable(ttf,stoffset+coverage);
		rule->u.rcoverage.ncovers[0] = GlyphsToNames(glyphs,false);
		GxCore_Free(glyphs);

		for ( i=0; i<bcnt; ++i ) {
			glyphs = getCoverageTable(ttf,stoffset+bcoverage[i]);
			rule->u.rcoverage.bcovers[i] = GlyphsToNames(glyphs,true);
			GxCore_Free(glyphs);
		}
		for ( i=0; i<fcnt; ++i ) {
			glyphs = getCoverageTable(ttf,stoffset+fcoverage[i]);
			rule->u.rcoverage.fcovers[i] = GlyphsToNames(glyphs,true);
			GxCore_Free(glyphs);
		}
		rule->lookup_cnt = 0;       /* substitution lookups needed for reverse chaining */
	}
	GxCore_Free(sglyphs);
	GxCore_Free(fcoverage);
	GxCore_Free(bcoverage);
}

static void gsubExtensionSubTable(FILE *ttf, FT_Face face, int stoffset,
								  struct lookup *l, struct lookup_subtable *subtable,
								  int justinuse, struct lookup *alllooks) {
	uint32 base = ftell(ttf), st, offset;
	int lu_type;

	/* Format = */ getushort(ttf);
	lu_type = getushort(ttf);
	offset = getlong(ttf);

	l->otlookup->lookup_type = lu_type;

	fseek(ttf,st = base+offset,SEEK_SET);
	switch ( lu_type ) {
	case 1:
		gsubSimpleSubTable(ttf,face,st,l,subtable,justinuse);
		break;
	case 2: case 3:   /* Multiple and alternate have same format, different semantics */
		gsubMultipleSubTable(ttf,st,l,subtable,justinuse);
		break;
	case 4:
		gsubLigatureSubTable(ttf,st,l,subtable,justinuse);
		break;
	case 5:
		gsubContextSubTable(ttf,st,l,subtable,justinuse,alllooks);
		break;
	case 6:
		gsubChainingSubTable(ttf,st,l,subtable,git_findnames/*justinuse*/,alllooks);
		break;
	case 7:
		gxlogd("This font is erroneous: it has a GSUB extension subtable that points to\nanother extension sub-table.\n");
		break;
	case 8:
		gsubReverseChainSubTable(ttf,st,l,subtable,justinuse);
		break;
		/* Any cases added here also need to go in the gsubLookupSwitch */
	default:
		gxlogd("Unknown GSUB sub-table type: %d\n", lu_type );
		break;
	}
}

static void gsubLookupSwitch(FILE *ttf,
				FT_Face face,
						int st,
						struct lookup *l,
						struct lookup_subtable *subtable,
						int justinuse,
						struct lookup *alllooks)
{
	switch ( l->type ) {
		case gsub_single:
			justinuse = git_findnames;
			gsubSimpleSubTable(ttf,face,st,l,subtable,justinuse);
			break;
		case gsub_multiple: case gsub_alternate:  /* Multiple and alternate have same format, different semantics */
			gsubMultipleSubTable(ttf,st,l,subtable,justinuse);
			break;
		case gsub_ligature:
			gsubLigatureSubTable(ttf,st,l,subtable,justinuse);
			break;
		case gsub_context:
			gsubContextSubTable(ttf,st,l,subtable,justinuse,alllooks);
			break;
		case gsub_contextchain:
			gsubChainingSubTable(ttf,st,l,subtable,git_findnames/*justinuse*/,alllooks);
			break;
		case 7:
			gsubExtensionSubTable(ttf, face, st,l,subtable,justinuse,alllooks);
			break;
		case gsub_reversecchain:
			gsubReverseChainSubTable(ttf,st,l,subtable,justinuse);
			break;
		default:
			gxlogd("Unknown GSUB sub-table type: %d\n", l->otlookup->lookup_type );
			break;
	}
}

static void seperate_lig(char *src, char **dst0, char **dst1)
{
	SplineChar *pchar = NULL;
	PST *pos = NULL;
	int i = 0;
	char *tmp_name = NULL, *tmp_str0 = NULL, *tmp_str1 = NULL;

	if(NULL == src)
		return;

	for(i = 0; i < glyph_cnt; i++)
	{
		pchar = chars[i];
		pos = pchar->possub;
		if((NULL == pchar) || (NULL == pos))
		{
			continue;
		}

		if(0 == strcasecmp(src, pchar->name))
		{

			if((pst_substitution == pos->type) ||
					(pst_multiple == pos->type) ||
					(pst_alternate == pos->type))
			{
				tmp_name = strdup(pos->u.subs.variant);
				tmp_str0 = strtok(tmp_name, " ");
				tmp_str1 = strtok(NULL, " ");
			}
			else if(pst_ligature == pos->type)
			{
				tmp_name = strdup(pos->u.lig.components);
				tmp_str0 = strtok(tmp_name, " ");
				tmp_str1 = strtok(NULL, " ");
			}

			break;
		}
	}

	if((tmp_str0) && (tmp_str1) && (0 != strcasecmp(tmp_name, src)))
	{
		tmp_str0 = strdup(tmp_str0);
		tmp_str1 = strdup(tmp_str1);
		*dst0 = tmp_str0;
		*dst1 = tmp_str1;
	}
	else
	{
		*dst0 = NULL;
		*dst1 = NULL;
	}
	if(tmp_name)
		GxCore_Free(tmp_name);
}

static int get_code_by_name(FT_Face face, char *name)
{
	FT_ULong  charcode;
	FT_UInt   gindex, real_index;
	FT_Pointer buffer[100] = {0};

	charcode = FT_Get_First_Char( face, &gindex );
	while ( gindex != 0 )
	{
		real_index = FT_Get_Char_Index(face, charcode);
		memset(buffer, 0, 100);
		FT_Get_Glyph_Name(face, real_index, buffer, 100);
		if(0 == strcasecmp((char *)buffer, name))
		{
			break;
		}
		charcode = FT_Get_Next_Char( face, charcode, &gindex );
	}

	return (charcode);
}

static void get_element_lig_sub(FT_Face face, LigatureSub *lig_sub, int count)
{
	SplineChar *pchar = NULL;
	PST *pos = NULL;
	int lig_index = 0, i = 0, j = 0;
	char *tmp_name = NULL, *tmp_str = NULL, *sep_str = NULL, *str0 = NULL, *str1 = NULL;

	if(NULL == lig_sub)
	{
		return;
	}

	lig_index = 0;
	for(i = 0; i < glyph_cnt; i++)
	{
		pchar = chars[i];
		pos = pchar->possub;
		if((NULL == pchar) || (NULL == pos))
		{
			continue;
		}
		while(pos)
		{
			if((pst_substitution == pos->type) ||
					(pst_multiple == pos->type) ||
					(pst_alternate == pos->type))
			{
				tmp_name = strdup(pos->u.subs.variant);
				sep_str = strtok(tmp_name, " ");
				tmp_str = strtok(NULL, " ");

				seperate_lig(sep_str, &str0, &str1);
				if((str0) && (str1))
				{
					lig_sub[lig_index].src_name[j] = strdup(str0);
					lig_sub[lig_index].src_code[j] = get_code_by_name(face, lig_sub[lig_index].src_name[j]);
					j++;

					lig_sub[lig_index].src_name[j] = strdup(str1);
					lig_sub[lig_index].src_code[j] = get_code_by_name(face, lig_sub[lig_index].src_name[j]);
				}
				else if(sep_str)
				{
					lig_sub[lig_index].src_name[j] = strdup(sep_str);
					lig_sub[lig_index].src_code[j] = get_code_by_name(face, lig_sub[lig_index].src_name[j]);
				}
				sep_str = tmp_str;
				while(sep_str)
				{
					j++;
					if(NULL == tmp_str)
					{
						sep_str = strtok(NULL, " ");
					}
					tmp_str = NULL;
					if(sep_str)
					{
						seperate_lig(sep_str, &str0, &str1);
						if((str0) && (str1))
						{
							lig_sub[lig_index].src_name[j] = strdup(str0);
							lig_sub[lig_index].src_code[j] = get_code_by_name(face, lig_sub[lig_index].src_name[j]);
							j++;

							lig_sub[lig_index].src_name[j] = strdup(str1);
							lig_sub[lig_index].src_code[j] = get_code_by_name(face, lig_sub[lig_index].src_name[j]);
						}
						else
						{
							lig_sub[lig_index].src_name[j] = strdup(sep_str);
							lig_sub[lig_index].src_code[j] = get_code_by_name(face, lig_sub[lig_index].src_name[j]);
						}
					}
				}
				j++;
			}
			else if(pst_ligature == pos->type)
			{
				tmp_name = strdup(pos->u.lig.components);
				sep_str = strtok(tmp_name, " ");
				tmp_str = strtok(NULL, " ");

				seperate_lig(sep_str, &str0, &str1);
				if((str0) && (str1))
				{
					lig_sub[lig_index].src_name[j] = strdup(str0);
					lig_sub[lig_index].src_code[j] = get_code_by_name(face, lig_sub[lig_index].src_name[j]);
					j++;

					lig_sub[lig_index].src_name[j] = strdup(str1);
					lig_sub[lig_index].src_code[j] = get_code_by_name(face, lig_sub[lig_index].src_name[j]);
				}
				else if(sep_str)
				{
					lig_sub[lig_index].src_name[j] = strdup(sep_str);
					lig_sub[lig_index].src_code[j] = get_code_by_name(face, lig_sub[lig_index].src_name[j]);
				}
				sep_str = tmp_str;
				while(sep_str)
				{
					j++;
					if(NULL == tmp_str)
					{
						sep_str = strtok(NULL, " ");
					}
					tmp_str = NULL;
					if(sep_str)
					{
						seperate_lig(sep_str, &str0, &str1);
						if((str0) && (str1))
						{
							lig_sub[lig_index].src_name[j] = strdup(str0);
							lig_sub[lig_index].src_code[j] = get_code_by_name(face, lig_sub[lig_index].src_name[j]);
							j++;

							lig_sub[lig_index].src_name[j] = strdup(str1);
							lig_sub[lig_index].src_code[j] = get_code_by_name(face, lig_sub[lig_index].src_name[j]);
						}
						else if(sep_str)
						{
							lig_sub[lig_index].src_name[j] = strdup(sep_str);
							lig_sub[lig_index].src_code[j] = get_code_by_name(face, lig_sub[lig_index].src_name[j]);
						}
					}
				}
			}
			if(tmp_name)
			{
				GxCore_Free(tmp_name);
				if(str0)
					GxCore_Free(str0);
				if(str1)
					GxCore_Free(str1);
				lig_sub[lig_index].src_count = j;
				j = 0;
			}

			lig_sub[lig_index].dst_name = strdup(pchar->name);
			lig_sub[lig_index].dst_code = get_code_by_name(face, lig_sub[lig_index].dst_name);
			if((pst_substitution == pos->type) &&
			   (1 == lig_sub[lig_index].src_count)) {
				// subs
				char *tmp_str = NULL;
				int tmp_code = 0;
				tmp_str = lig_sub[lig_index].src_name[0];
				lig_sub[lig_index].src_name[0] = lig_sub[lig_index].dst_name;
				lig_sub[lig_index].dst_name = tmp_str;

				tmp_code = lig_sub[lig_index].src_code[0];
				lig_sub[lig_index].src_code[0] = lig_sub[lig_index].dst_code;
				lig_sub[lig_index].dst_code = tmp_code;
			}
			if((pos) &&
			   (pos->subtable) &&
			   (pos->subtable->subtable_name)) {
				memset(lig_sub[lig_index].tag, 0, MAX_SUB_TAG);
				memcpy(lig_sub[lig_index].tag, pos->subtable->subtable_name + 1, MAX_SUB_TAG - 1);
			}
			lig_index++;

			pos = pos->next;
		}
	}
}

void print_lig_transform(LigatureSub *lig_sub, int lig_sum)
{
	int i = 0, j = 0;

	for(i = 0; i < lig_sum; i++)
	{
		GUI_Debug_Print("[%4d]", i);
		GUI_Debug_Print("\t\t");
		GUI_Debug_Print("[%s]", lig_sub[i].tag);
		GUI_Debug_Print("\t\t");
		for(j = 0; j < lig_sub[i].src_count; j++)
		{
			GUI_Debug_Print("%s(0x%x)", lig_sub[i].src_name[j], lig_sub[i].src_code[j]);
			if(j == (lig_sub[i].src_count - 1))
			{
				GUI_Debug_Print(" =====> \t");
			}
			else
			{
				GUI_Debug_Print(" + ");
			}
		}
		GUI_Debug_Print("%s", lig_sub[i].dst_name);
		if(lig_sub[i].dst_code)
		{
			GUI_Debug_Print("(0x%x)", lig_sub[i].dst_code);
		}
		GUI_Debug_Print("\t\t\n");
		GUI_Debug_Print("\n");
	}
}

void ArrangeHash(GSUB_Info *info)
{
	LigatureSub* table = NULL;
	int i = 0;

	if(NULL == info->isol_table) {
		info->isol_table = hash_new(10, NULL);
	}

	if(NULL == info->init_table) {
		info->init_table = hash_new(10, NULL);
	}

	if(NULL == info->medi_table) {
		info->medi_table = hash_new(10, NULL);
	}

	if(NULL == info->fina_table) {
		info->fina_table = hash_new(10, NULL);
	}

	if((NULL == info->isol_table) ||
	   (NULL == info->init_table) ||
	   (NULL == info->medi_table) ||
	   (NULL == info->fina_table)) {
		return;
	}

	table = info->table;
	for(i = 0; i < info->count; i++) {
		if(0 == strcasecmp("isol", table[i].tag)) {
			hash_add(info->isol_table, table[i].src_name[0], table[i].dst_name);
		} else if(0 == strcasecmp("init", table[i].tag)) {
			hash_add(info->init_table, table[i].src_name[0], table[i].dst_name);
		} else if(0 == strcasecmp("medi", table[i].tag)) {
			hash_add(info->medi_table, table[i].src_name[0], table[i].dst_name);
		} else if(0 == strcasecmp("fina", table[i].tag)) {
			hash_add(info->fina_table, table[i].src_name[0], table[i].dst_name);
		}
	}
}

void PrintGSUBInfomation(GSUB_Info *info)
{
	GlyphInfo *glyph_info = NULL;
	int i = 0;

	if(NULL == info)
		return;

	GUI_Debug_Print("=============================All of information==================================\n");

	glyph_info = info->glyph_info;
	for(i = 0; i < glyph_info->count; i++)
	{
		GUI_Debug_Print("#########[%4d]########", i);
		GUI_Debug_Print("\t%s\t########", glyph_info->glyph[i].name);
		switch(glyph_info->glyph[i].type)
		{
			case 0:
				GUI_Debug_Print("\tautomatic\t########\n");
				break;
			case 1:
				GUI_Debug_Print("\tunassigned\t########\n");
				break;
			case 2:
				GUI_Debug_Print("\tsimple\t########\n");
				break;
			case 3:
				GUI_Debug_Print("\tligature\t########\n");
				break;
			case 4:
				GUI_Debug_Print("\tmark\t########\n");
				break;
			case 5:
				GUI_Debug_Print("\tcomponent\t########\n");
				break;
			default:
				GUI_Debug_Print("\tunknown\t########\n");
				break;
		}
	}

	print_lig_transform(info->table, info->count);
}

void ValDevFree(ValDevTab *adjust) {
	if ( adjust==NULL )
		return;
	GxCore_Free( adjust->xadjust.corrections );
	GxCore_Free( adjust->yadjust.corrections );
	GxCore_Free( adjust->xadv.corrections );
	GxCore_Free( adjust->yadv.corrections );
	chunkfree(adjust,sizeof(ValDevTab));
}

void PSTFree(PST *pst) {
	PST *pnext;
	for ( ; pst!=NULL; pst = pnext ) {
		pnext = pst->next;
		if ( pst->type==pst_lcaret )
			GxCore_Free(pst->u.lcaret.carets);
		else if ( pst->type==pst_pair ) {
			GxCore_Free(pst->u.pair.paired);
			ValDevFree(pst->u.pair.vr[0].adjust);
			ValDevFree(pst->u.pair.vr[1].adjust);
			chunkfree(pst->u.pair.vr,sizeof(struct vr [2]));
		}
		else if ( pst->type!=pst_position ) {
			GxCore_Free(pst->u.subs.variant);
		} else if ( pst->type==pst_position ) {
			ValDevFree(pst->u.pos.adjust);
		}
		chunkfree(pst,sizeof(PST));
	}
}

static void SplineCharFreeContents(SplineChar *sc) {
	if ( sc==NULL )
		return;

	if (sc->name != NULL) GxCore_Free(sc->name);
	PSTFree(sc->possub);
}

static void FreeChars(SplineChar **pchar, int count) {
	int i = 0;

	if(pchar == NULL)
		return;

	for(i = 0; i < count; i++)
	{
		SplineCharFreeContents(pchar[i]);
		pchar[i] = NULL;
	}

	return;
}

static GlyphInfo *pfed_readlookupnames(FILE *ttf, FT_Face face, AnchorClass *ahead, uint32 base, OTLookup *lus)
{
	int k, inusetype = 0;
	int32 st;
	int32 lookup_start, script_off, feature_off;
	struct scripts *scripts;
	struct feature *features;
	struct lookup *lookups, *l;
	struct lookup_subtable *subtable;

	fseek(ttf,base,SEEK_SET);
	if ( getlong(ttf) !=0x00010000 )
		return (NULL);         /* Bad version number */

	script_off = getushort(ttf);
	feature_off = getushort(ttf);
	lookup_start = base + getushort(ttf);

	scripts = readttfscripts(ttf,base+script_off);
	features = readttffeatures(ttf,base+feature_off);
	/* It is legal to have lookups with no features or scripts */
	/* For example if all the lookups were controlled by the JSTF table */
	lookups = readttflookups(ttf,lookup_start);
	if ( lookups==NULL ) {
		ScriptsFree(scripts);
		FeaturesFree(features);
		return (NULL);
	}

	tagLookupsWithScript(scripts,features,lookups);
	ScriptsFree(scripts); scripts = NULL;
	FeaturesFree(features); features = NULL;

	for ( l = lookups; l->offset!=0; ++l ) {
		for ( k=0, subtable=l->otlookup->subtables; k<l->subtabcnt; ++k, subtable=subtable->next ) {
			st = l->subtab_offsets[k];
			fseek(ttf,st,SEEK_SET);
			// TODO: gsubLookupSwitch
			gsubLookupSwitch(ttf,face,st,l,subtable,inusetype,lookups);
		}
	}

	/* Then generate some user-friendly names for the all the lookups */
	if ( inusetype==0 )
		for ( l=lookups; l->offset!=0; ++l )
			NameOTLookup(l->otlookup);

	LookupsFree(lookups);
	if ( inusetype!=0 )
	{
		OTLookupListFree(gsub_lookups);
		gsub_lookups = cur_lookups = NULL;
	}

	SplineChar *pchar = NULL;
	int i = 0, lig_count = 0;
	PST *pos = NULL;

	for(i = 0; i < glyph_cnt; i++)
	{
		pchar = chars[i];
		if((NULL != pchar) && (NULL != pchar->possub))
		{
			pos = pchar->possub;
			while(pos)
			{
				lig_count++;
				pos = pos->next;
			}
		}
	}

	lig_sum = lig_count;
	lig_sub = (LigatureSub *)GxCore_Calloc(lig_count, sizeof(LigatureSub));
	get_element_lig_sub(face, lig_sub, lig_count);
	GlyphInfo *glyph_info = (GlyphInfo *)GxCore_Malloc(sizeof(GlyphInfo));
	if(glyph_info)
	{
		glyph_info->count = glyph_cnt;
		glyph_info->glyph = (CharPara *)GxCore_Malloc(glyph_cnt * sizeof(CharPara));
		if(glyph_info->glyph)
		{
			for(i = 0; i < glyph_cnt; i++)
			{
				glyph_info->glyph[i].name = GxCore_Strdup(chars[i]->name);
				glyph_info->glyph[i].type = chars[i]->glyph_class;
			}
		}
	}

	FreeChars(chars, glyph_cnt);
	chars = NULL;
	glyph_cnt = 0;

	gxlogd("**********************Total Transformation number is %d****************************\n", lig_count);
	return (glyph_info);
}

static void readttfgdef(FILE *ttf)
{
	int lclo, gclass, mac, mas=0;
	int version, i;

	fseek(ttf,gdef_start,SEEK_SET);
	version = getlong(ttf);
	if ( version!=0x00010000 && version != 0x00010002 )
		return;

	gclass = getushort(ttf);
	/* attach list = */ getushort(ttf);
	lclo = getushort(ttf);      /* ligature caret list */
	mac = getushort(ttf);       /* mark attach class */
	if ( version==0x00010002 )
		mas = getushort(ttf);

	if ( gclass!=0 ) {
		uint16 *gclasses = getClassDefTable(ttf,gdef_start+gclass);
		for ( i=0; i<glyph_cnt; ++i )
			if ( chars[i]!=NULL && gclasses[i]!=0 )
				chars[i]->glyph_class = gclasses[i]+1;
		free(gclasses);
	}

	return;
}
GSUB_Info *gsub_read(char *file, FT_Face face)
{
	FILE *ttf = NULL;
	int n,i;
	unsigned short search_range, entry_select, range_shift;
	OTLookup loopups = {0};
	struct tagoff { uint32 tag, checksum, offset, length;} tagoff[MAX_SUBTABLE_TYPES+30];
	GSUB_Info *info = NULL;
	GlyphInfo *glyph_info = NULL;

#define MAX_LENGTH				(100)
	static FT_Library library;
	char glyph_name[MAX_LENGTH] = {0};
	FT_Stream stream;

	FT_Error error = FT_Init_FreeType( &library );
	if ( error ) {
		gxlogd("Init freetype2 failed!\n");
		return (NULL);
	}

	stream = (FT_Stream)GxCore_Calloc(1, sizeof(*stream));
	if ( stream == NULL ) {
		return (NULL);
	}

	if(NULL == file)
	{
		gxlogd("*************Please make the input file name is correct!***************\n");
		return (NULL);
	}

	ttf = fopen(file, "r");
	if(NULL == ttf)
	{
		gxlogd("*************Cannot open the file : %s!***************\n", file);
		return (NULL);
	}

	fseek(ttf,0,SEEK_SET);
	if ( getlong(ttf)!=0x00010000 )
	{
		gxlogd("***This file %s is not regular open type or true type ttf file.***\n", file);
		return (NULL);
	}

	n = getushort(ttf);
	search_range = getushort(ttf);
	entry_select = getushort(ttf);
	range_shift = getushort(ttf);
	if ( n>=MAX_SUBTABLE_TYPES+30 )
		n = MAX_SUBTABLE_TYPES+30;
	for ( i=0; i<n; ++i ) {
		tagoff[i].tag = getlong(ttf);
		tagoff[i].checksum = getlong(ttf);
		tagoff[i].offset = getlong(ttf);
		tagoff[i].length = getlong(ttf);
		if(tagoff[i].tag == NAME_TAG)
		{
			copyright_start = tagoff[i].offset;
		}
		else if(CHR('m','a','x','p') == tagoff[i].tag)
		{
			uint32 maxp_start;
			uint32 old_pos;

			maxp_start = tagoff[i].offset;
			fseek(ttf,maxp_start+4,SEEK_SET);
			glyph_cnt = getushort(ttf);

			chars = (SplineChar **)GxCore_Calloc(glyph_cnt, sizeof(SplineChar *));

			old_pos = ftell(ttf);
			fseek(ttf, 0, SEEK_END);

			for(i = 0; i < glyph_cnt; i++)
			{
				FT_Get_Glyph_Name(face, i, glyph_name, MAX_LENGTH);
				chars[i] = (SplineChar *)GxCore_Calloc(1, sizeof(SplineChar));
				chars[i]->name = strdup(glyph_name);
				//gxlogd("-------------[%d]-----glyph name = %s----------------\n", i, chars[i]->name);
			}
			fseek(ttf, old_pos, SEEK_SET);
		}
		else if(CHR('G','D','E','F') == tagoff[i].tag)
		{
			gdef_start = tagoff[i].offset;
		}
	}
	for ( i=0; i<n; ++i ) switch ( tagoff[i].tag ) {
		case GSUB_TAG:
			if(gdef_start)
			{
				// TODO: Parse GDEF
				readttfgdef(ttf);
				gxlogd("####################################Parse GDEF Table!################################\n");
			}
			glyph_info = pfed_readlookupnames(ttf, face, NULL, tagoff[i].offset, &loopups);
			gxlogd("********GSUB table Parse OK********\n");
			break;
		default:
			break;
	}

	fclose(ttf);

	info  = (GSUB_Info *)GxCore_Mallocz(sizeof(GSUB_Info));
	if(NULL == info)
		return (NULL);

	info->table = lig_sub;
	info->count = lig_sum;
	info->glyph_info = glyph_info;
	lig_sub = NULL;
	lig_sum = 0;
	return (info);
}

GSUB_Opt g_gsub_opt = {
	gsub_read,
	PrintGSUBInfomation,
	ArrangeHash,
	_get_transformation_lig,
	_is_mark,
	_is_mark_byname
};

#endif  /*GUI_TTF*/

