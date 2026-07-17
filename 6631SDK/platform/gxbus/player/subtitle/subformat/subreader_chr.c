
#include "gx_stream.h"
#include "gx_subreader.h"
#include "module/enca/enca.h"


#define SUB_ERR ((void* ) -1)

typedef struct  {
    GxSubPage* (*read)(GxStream *st,GxSubPage *dest,int onlytime);
	void (*post)(GxSubPage* sub);
    const char* name;
}sub_reader;

#define MAX_PAGE_NUM 100
#define LINE_LEN  1023

static char line[LINE_LEN+1];
static char line2[LINE_LEN+1];
static GxSubData s_subd = {0};
static int sub_slacktime = 20000; //20 sec
static int sub_duration = 1;

static int eol(char p) {
	return p=='\r' || p=='\n' || p=='\0';
}

static void trail_space(char *s) {
	int i = 0;
	while (isspace(s[i])) ++i;
	if (i) strncpy(s, s + i, strlen(s)-1);
	i = strlen(s) - 1;
	while (i > 0 && isspace(s[i])) s[i--] = '\0';
}

static char *stristr(const char *haystack, const char *needle) {
    int len = 0;
    const char *p = haystack;

    if (!(haystack && needle)) return NULL;

    len=strlen(needle);
    while (*p != '\0') {
	if (strncasecmp(p, needle, len) == 0) return (char*)p;
	p++;
    }

    return NULL;
}

static char* sub_readtext(char *source, char **dest) {
    int len=0;
    char* p=source;

	if(source == NULL)
		return NULL;

    while ( !eol(*p) &&* p!= '|' ) {
	p++,len++;
    }

   * dest= av_malloc (len+1);
    if (!dest) {return SUB_ERR;}

    strncpy(*dest, source, len);
    (*dest)[len]=0; // FIXME:

    while (*p=='\r' ||* p=='\n' || *p=='|') p++;

    if (*p) return p;  // not-last text field
    else return NULL;  // last text field
}

static GxSubPage* sub_read_line_microdvd(GxStream *st,GxSubPage *current, int onlytime) {
    char* p, *next;
    int i;

    do {
	if (!GxStream_ReadLine (st, line, LINE_LEN)) return NULL;
    } while ((sscanf (line,
		      "{%d}{}%[^\r\n]",
		      &(current->start), line2) < 2) &&
	     (sscanf (line,
		      "{%d}{%d}%[^\r\n]",
		      &(current->start), &(current->end), line2) < 3));
	if((current->start == 1)&&(current->start == current->end)){
		sub_duration = 1000/atoi(line2);
		current->lines = 0;
		return current;
	} else
		sub_duration = (1000 * 125) / 2997;
	current->start *= sub_duration;
	current->end   *= sub_duration;

	if (onlytime) return current;

    p=line2;

    next=p, i=0;
    while ((next =sub_readtext (next, &(current->text[i])))) {
        if (current->text[i]==SUB_ERR) {return SUB_ERR;}
	i++;
	if (i>=SUB_MAX_TEXT) { gxlogd("Too many lines in a GxSubPage\n");current->lines=i;return current;}
    }
    current->lines= ++i;

    return current;
}

static GxSubPage* sub_read_line_mpl2(GxStream *st,GxSubPage *current, int onlytime) {
    char* p, *next;
    int i;

    do {
	if (!GxStream_ReadLine (st, line, LINE_LEN)) return NULL;
    } while ((sscanf (line,
		      "[%d][%d]%[^\r\n]",
		      &(current->start), &(current->end), line2) < 3));
    current->start*= 10;
    current->end*= 10;
	if (onlytime) return current;

    p=line2;

    next=p, i=0;
    while ((next =sub_readtext (next, &(current->text[i])))) {
        if (current->text[i]==SUB_ERR) {return SUB_ERR;}
	i++;
	if (i>=SUB_MAX_TEXT) { gxlogd("Too many lines in a GxSubPage\n");current->lines=i;return current;}
    }
    current->lines= ++i;

    return current;
}

static GxSubPage* sub_read_line_subrip(GxStream* st, GxSubPage *current, int onlytime) {
    int a1,a2,a3,a4,b1,b2,b3,b4;
    char* p=NULL, *q=NULL;
    int len;

    while (1) {
	if (!GxStream_ReadLine (st, line, LINE_LEN)) return NULL;
	if (sscanf (line, "%d:%d:%d.%d,%d:%d:%d.%d",&a1,&a2,&a3,&a4,&b1,&b2,&b3,&b4) < 8) continue;
	current->start = a1*360000+a2*6000+a3*100+a4;
	current->end   = b1*360000+b2*6000+b3*100+b4;
	if (onlytime) return current;

	if (!GxStream_ReadLine (st, line, LINE_LEN)) return NULL;
	if(!strncasecmp (line, "&nbsp", 5)) continue;

	p=q=line;
	for (current->lines=1; current->lines < SUB_MAX_TEXT; current->lines++) {
	    for (q=p,len=0;* p && *p!='\r' && *p!='\n' && *p!='|' && strncmp(p,"[br]",4); p++,len++);
	    current->text[current->lines-1]=av_malloc (len+1);
	    if (!current->text[current->lines-1]) return SUB_ERR;
	    strncpy (current->text[current->lines-1], q, len);
	    current->text[current->lines-1][len]='\0';
	    if (!*p ||* p=='\r' || *p=='\n') break;
	    if (*p=='|') p++;
	    else while (*p++!=']');
	}
	break;
    }
    return current;
}

static GxSubPage* sub_read_line_subviewer(GxStream *st,GxSubPage *current, int onlytime) {
    int a1,a2,a3,a4,b1,b2,b3,b4;
    char* p=NULL;
    int i,len;

    while (!current->text[0]) {
	if (!GxStream_ReadLine (st, line, LINE_LEN)) return NULL;
	if ((len=sscanf (line, "%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d",&a1,&a2,&a3,(char* )&i,&a4,&b1,&b2,&b3,(char *)&i,&b4)) < 10)
	    continue;
	current->start = a1*3600000+a2*60000+a3*1000+a4;
	current->end   = b1*3600000+b2*60000+b3*1000+b4;
	if (onlytime) return current;

	for (i=0; i<SUB_MAX_TEXT;) {
	    int blank = 1;
	    if (!GxStream_ReadLine (st, line, LINE_LEN)) break;
	    len=0;
	    for (p=line;* p!='\n' && *p!='\r' && *p; p++,len++)
		if (*p != ' ' &&* p != '\t')
		    blank = 0;
	    if (len && !blank) {
                int j=0,skip=0;
		char* curptr=current->text[i]=av_malloc (len+1);
		if (!current->text[i]) return SUB_ERR;

        for(; j<len; j++) {
		    /* let's filter html tags ::atmos*/
		    if(line[j]=='>') {
			skip=0;
			continue;
		    }
		    if(line[j]=='<') {
			skip=1;
			continue;
		    }
		    if(skip) {
			continue;
		    }
		   * curptr=line[j];
		    curptr++;
		}
		*curptr='\0';

		i++;
	    } else {
		break;
	    }
	}
	current->lines=i;
    }
    return current;
}

static GxSubPage* sub_read_line_lrc(GxStream *st,GxSubPage *current, int onlytime) {
    int a1=0,a2=0,a3=0,a4=0,b1=0,b2=0,b3=0,b4=0;
    char* p=NULL;
    int len,i;

    while (!current->text[0]) {
		if (!GxStream_ReadLine (st, line, LINE_LEN))
			return NULL;
		if ((len=sscanf (line+1, "%d:%d%[,.:]%d",&a2,&a3,(char* )&i,&a4)) < 4)
		    continue;
		current->start = a1*3600000+a2*60000+a3*1000+a4;
		current->end   = b1*3600000+b2*60000+b3*1000+b4;
		if (onlytime) return current;

	   // if (!GxStream_ReadLine (st, line, LINE_LEN))
		//	break;
		for(p=line;* p!=']'&& *p; p++);
	    len=0;
	    for (;*p!='\n' &&* p!='\r' && *p; p++,len++);
		if(len){
		char* curptr=current->text[0]=av_malloc (len+1);
		if (!current->text[0])
			return SUB_ERR;
		memcpy(curptr,p-len+1,len);
		curptr[len]='\0';
		}
		current->lines=1;
	    return current;
   }
	return NULL;
}

static GxSubPage* sub_read_line_subviewer2(GxStream *st,GxSubPage *current, int onlytime) {
    int a1,a2,a3,a4;
    char* p=NULL;
    int i,len;

    while (!current->text[0]) {
        if (!GxStream_ReadLine (st, line, LINE_LEN)) return NULL;
	if (line[0]!='{')
	    continue;
        if ((len=sscanf (line, "{T %d:%d:%d:%d",&a1,&a2,&a3,&a4)) < 4)
            continue;
        current->start = a1*360000+a2*6000+a3*100+a4/10;
		if (onlytime) return current;

        for (i=0; i<SUB_MAX_TEXT;) {
            if (!GxStream_ReadLine (st, line, LINE_LEN)) break;
            if (line[0]=='}') break;
            len=0;
            for (p=line;* p!='\n' && *p!='\r' && *p; ++p,++len);
            if (len) {
                current->text[i]=av_malloc (len+1);
                if (!current->text[i]) return SUB_ERR;
                strncpy (current->text[i], line, len); current->text[i][len]='\0';
                ++i;
            } else {
                break;
            }
        }
        current->lines=i;
    }

    return current;
}

static GxSubPage* sub_read_line_vplayer(GxStream *st,GxSubPage *current, int onlytime) {
	int a1,a2,a3;
	char* p=NULL, *next,separator;
	int i,len,plen;

	while (!current->text[0]) {
		if (!GxStream_ReadLine (st, line, LINE_LEN)) return NULL;
		if ((len=sscanf (line, "%d:%d:%d%c%n",&a1,&a2,&a3,&separator,&plen)) < 4)
			continue;

		if (!(current->start = a1*360000+a2*6000+a3*100))
			continue;
		if (onlytime) return current;
		p = &line[ plen ];

		i=0;
		if (*p!='|') {
			//
			next = p,i=0;
			while ((next =sub_readtext (next, &(current->text[i])))) {
				if (current->text[i]==SUB_ERR) {return SUB_ERR;}
				i++;
				if (i>=SUB_MAX_TEXT) { gxlogd("Too many lines in a GxSubPage\n");current->lines=i;return current;}
			}
			current->lines=i+1;
		}
	}
	return current;
}

static GxSubPage* sub_read_line_txt(GxStream *st,GxSubPage *current, int onlytime)
{
	int a1,a2,a3;
	char* p=NULL, *next, separator;
	int i,len,plen;
	int flags = 0;

	while (1) {
		if (!GxStream_ReadLine (st, line, LINE_LEN)) return NULL;
		if ((len=sscanf (line, "%d:%d:%d%c%n",&a1,&a2,&a3,&separator,&plen)) < 3)
			continue;

		if(*p == '\n'){
			current->end = a1*3600000+a2*60000+a3*1000;
			if(flags == 1) break;
		}else{
			current->start = a1*3600000+a2*60000+a3*1000;
			p = &line[plen];
			if(!strncasecmp (p, "&nbsp", 5))
				continue;
			if(onlytime) return current;
			next = p,i=0;
			while ((next =sub_readtext (next, &(current->text[i])))){
				if (current->text[i]==SUB_ERR) {return SUB_ERR;}
				i++;
				if (i>=SUB_MAX_TEXT) {
					gxlogd("Too many lines in a GxSubPage\n");
					current->lines=i;
					return current;
				}
			}
			current->lines=i+1;
			flags = 1;
		}
	}
	return current;
}

static GxSubPage* sub_read_line_rt(GxStream *st,GxSubPage *current, int onlytime) {
	//TODO: This format uses quite rich (sub/super)set of xhtml
	// I couldn't check it since DTD is not included.
	// WARNING: full XML parses can be required for proper parsing
    int a1,a2,a3,a4,b1,b2,b3,b4;
    char* p=NULL,*next=NULL;
    int i,len,plen;

    while (!current->text[0]) {
	if (!GxStream_ReadLine (st, line, LINE_LEN)) return NULL;
	//TODO: it seems that format of time is not easily determined, it may be 1:12, 1:12.0 or 0:1:12.0
	//to describe the same moment in time. Maybe there are even more formats in use.
	//if ((len=sscanf (line, "<Time Begin=\"%d:%d:%d.%d\" End=\"%d:%d:%d.%d\"",&a1,&a2,&a3,&a4,&b1,&b2,&b3,&b4)) < 8)
	plen=a1=a2=a3=a4=b1=b2=b3=b4=0;
	if (
	((len=sscanf (line, "<%*[tT]ime %*[bB]egin=\"%d.%d\" %*[Ee]nd=\"%d.%d\"%*[^<]<clear/>%n",&a3,&a4,&b3,&b4,&plen)) < 4) &&
	((len=sscanf (line, "<%*[tT]ime %*[bB]egin=\"%d.%d\" %*[Ee]nd=\"%d:%d.%d\"%*[^<]<clear/>%n",&a3,&a4,&b2,&b3,&b4,&plen)) < 5) &&
	((len=sscanf (line, "<%*[tT]ime %*[bB]egin=\"%d:%d\" %*[Ee]nd=\"%d:%d\"%*[^<]<clear/>%n",&a2,&a3,&b2,&b3,&plen)) < 4) &&
	((len=sscanf (line, "<%*[tT]ime %*[bB]egin=\"%d:%d\" %*[Ee]nd=\"%d:%d.%d\"%*[^<]<clear/>%n",&a2,&a3,&b2,&b3,&b4,&plen)) < 5) &&
//	((len=sscanf (line, "<%*[tT]ime %*[bB]egin=\"%d:%d.%d\" %*[Ee]nd=\"%d:%d\"%*[^<]<clear/>%n",&a2,&a3,&a4,&b2,&b3,&plen)) < 5) &&
	((len=sscanf (line, "<%*[tT]ime %*[bB]egin=\"%d:%d.%d\" %*[Ee]nd=\"%d:%d.%d\"%*[^<]<clear/>%n",&a2,&a3,&a4,&b2,&b3,&b4,&plen)) < 6) &&
	((len=sscanf (line, "<%*[tT]ime %*[bB]egin=\"%d:%d:%d.%d\" %*[Ee]nd=\"%d:%d:%d.%d\"%*[^<]<clear/>%n",&a1,&a2,&a3,&a4,&b1,&b2,&b3,&b4,&plen)) < 8) &&
	//now try it without end time
	((len=sscanf (line, "<%*[tT]ime %*[bB]egin=\"%d.%d\"%*[^<]<clear/>%n",&a3,&a4,&plen)) < 2) &&
	((len=sscanf (line, "<%*[tT]ime %*[bB]egin=\"%d:%d\"%*[^<]<clear/>%n",&a2,&a3,&plen)) < 2) &&
	((len=sscanf (line, "<%*[tT]ime %*[bB]egin=\"%d:%d.%d\"%*[^<]<clear/>%n",&a2,&a3,&a4,&plen)) < 3) &&
	((len=sscanf (line, "<%*[tT]ime %*[bB]egin=\"%d:%d:%d.%d\"%*[^<]<clear/>%n",&a1,&a2,&a3,&a4,&plen)) < 4)
	)
	    continue;
	current->start = a1*360000+a2*6000+a3*100+a4/10;
	current->end   = b1*360000+b2*6000+b3*100+b4/10;
	if (b1 == 0 && b2 == 0 && b3 == 0 && b4 == 0)
	  current->end = current->start+200;
	if (onlytime) return current;
	p=line;	p+=plen;i=0;
	// TODO: I don't know what kind of convention is here for marking multiline subs, maybe <br/> like in xml?
	next = strstr(line,"<clear/>");
	if(next && strlen(next)>8){
	  next+=8;i=0;
	  while ((next =sub_readtext (next, &(current->text[i])))) {
		if (current->text[i]==SUB_ERR) {return SUB_ERR;}
		i++;
		if (i>=SUB_MAX_TEXT) { gxlogd("Too many lines in a GxSubPage\n");current->lines=i;return current;}
	  }
	}
			current->lines=i+1;
    }
    return current;
}

static void sami_add_line(GxSubPage *current, char *buffer, char **pos) {
    char *p = *pos;
    *p = 0;
    trail_space(buffer);
    if (*buffer && current->lines < SUB_MAX_TEXT)
        current->text[current->lines++] = av_strdup(buffer);
    *pos = buffer;
}

static GxSubPage* sub_read_line_sami(GxStream *st,GxSubPage *current, int onlytime)
{
    static char *s = NULL, *slacktime_s;
    char text[LINE_LEN+1], *p=NULL, *q;
    int state;

    current->lines = current->start = current->end = 0;
    state = 0;

    /* read the first line */
    if (!s)
	    if (!(s = GxStream_ReadLine (st, line, LINE_LEN))) return 0;

    do {
	switch (state) {

	case 0: /* find "START=" or "Slacktime:" */
	    slacktime_s = stristr (s, "Slacktime:");
	    if (slacktime_s)
                sub_slacktime = strtol (slacktime_s+10, NULL, 0) / 10;

	    s = stristr (s, "Start=");
	    if (s) {
		current->start = strtol (s + 6, &s, 0);
                /* eat '>' */
                for (; *s != '>' && *s != '\0'; s++);
                s++;
		state = 1; continue;
	    }
	    break;

	case 1: /* find (optional) "<P", skip other TAGs */
	    for  (; *s == ' ' || *s == '\t'; s++); /* strip blanks, if any */
	    if (*s == '\0') break;
	    if (*s != '<') { state = 3; p = text; continue; } /* not a TAG */
	    s++;
	    if (*s == 'P' || *s == 'p') { s++; state = 2; continue; } /* found '<P' */
	    for (; *s != '>' && *s != '\0'; s++); /* skip remains of non-<P> TAG */
	    if (s == '\0')
	      break;
	    s++;
	    continue;

	case 2: /* find ">" */
	    if ((s = strchr (s, '>'))) { s++; state = 3; p = text; continue; }
	    break;

	case 3: /* get all text until '<' appears */
	    if (p - text >= LINE_LEN)
	        sami_add_line(current, text, &p);
	    if (*s == '\0') break;
	    else if (!strncasecmp (s, "<br>", 4)) {
                sami_add_line(current, text, &p);
		s += 4;
	    }
	    else if (*s == '{') { state = 5; ++s; continue; }
	    else if (*s == '<') { state = 4; }
	    else if (!strncasecmp (s, "&nbsp;", 6)) { *p++ = ' '; s += 6; }
	    else if (*s == '\t') { *p++ = ' '; s++; }
	    else if (*s == '\r' || *s == '\n') { s++; }
	    else *p++ = *s++;

	    /* skip duplicated space */
	    if (p > text + 2) if (*(p-1) == ' ' && *(p-2) == ' ') p--;

	    continue;

	case 4: /* get current->end or skip <TAG> */
	    q = stristr (s, "Start=");
	    if (q) {
		current->end = strtol (q + 6, &q, 0);
		*p = '\0'; trail_space (text);
		if (text[0] != '\0')
		    current->text[current->lines++] = av_strdup (text);
		if (current->lines > 0) { state = 99; break; }
		state = 0; continue;
	    }
	    s = strchr (s, '>');
	    if (s) { s++; state = 3; continue; }
	    break;
       case 5: /* get rid of {...} text, but read the alignment code */
	    if ((*s == '\\') && (*(s + 1) == 'a') ) {
               if (stristr(s, "\\a1") != NULL) {
                   //current->alignment = SUB_ALIGNMENT_BOTTOMLEFT;
                   s = s + 3;
               }
               if (stristr(s, "\\a2") != NULL) {
                   //current->alignment = SUB_ALIGNMENT_BOTTOMCENTER;
                   s = s + 3;
               } else if (stristr(s, "\\a3") != NULL) {
                   //current->alignment = SUB_ALIGNMENT_BOTTOMRIGHT;
                   s = s + 3;
               } else if ((stristr(s, "\\a4") != NULL) || (stristr(s, "\\a5") != NULL) || (stristr(s, "\\a8") != NULL)) {
                   //current->alignment = SUB_ALIGNMENT_TOPLEFT;
                   s = s + 3;
               } else if (stristr(s, "\\a6") != NULL) {
                   //current->alignment = SUB_ALIGNMENT_TOPCENTER;
                   s = s + 3;
               } else if (stristr(s, "\\a7") != NULL) {
                   //current->alignment = SUB_ALIGNMENT_TOPRIGHT;
                   s = s + 3;
               } else if (stristr(s, "\\a9") != NULL) {
                   //current->alignment = SUB_ALIGNMENT_MIDDLELEFT;
                   s = s + 3;
               } else if (stristr(s, "\\a10") != NULL) {
                  // current->alignment = SUB_ALIGNMENT_MIDDLECENTER;
                   s = s + 4;
               } else if (stristr(s, "\\a11") != NULL) {
                   //current->alignment = SUB_ALIGNMENT_MIDDLERIGHT;
                   s = s + 4;
               }
	    }
	    if (*s == '}') state = 3;
	    ++s;
	    continue;
	}

	/* read next line */
	if (state != 99 && !(s = GxStream_ReadLine (st, line, LINE_LEN))) {
	    if (current->start > 0) {
		break; // if it is the last subtitle
	    } else {
		return 0;
	    }
	}

    } while (state != 99);

    // For the last subtitle
    if (current->end <= 0) {
        current->end = current->start + sub_slacktime;
		if (onlytime) return current;

        sami_add_line(current, text, &p);
    }

    return current;
}

static GxSubPage* sub_read_line_ssa(GxStream *st,GxSubPage *current, int onlytime)
{
	/*
	 *  Sub Station Alpha v4 (and v2?) scripts have 9 commas before GxSubPage
	 *  other Sub Station Alpha scripts have only 8 commas before GxSubPage
	 *  Reading the "ScriptType:" field is not reliable since many scripts appear
	 *  w/o it
	 *
	 *  http://www.scriptclub.org is a good place to find more examples
	 *  http://www.eswat.demon.co.uk is where the SSA specs can be found
	 */
	int comma;
	/*  amount of commas increase with newer SSA versions*/

	int hour1, min1, sec1, hunsec1,
	    hour2, min2, sec2, hunsec2, nothing;
	int num;

	char line3[LINE_LEN+1],
	     * line2;
	char* tmp;

	do {
		if (!GxStream_ReadLine (st, line, LINE_LEN)) return NULL;
	} while (sscanf (line, "Dialogue: Marked=%d,%d:%d:%d.%d,%d:%d:%d.%d,"
				"%[^\n\r]", &nothing,
				&hour1, &min1, &sec1, &hunsec1,
				&hour2, &min2, &sec2, &hunsec2,
				line3) < 9
			&&
			sscanf (line, "Dialogue: %d,%d:%d:%d.%d,%d:%d:%d.%d,"
				"%[^\n\r]", &nothing,
				&hour1, &min1, &sec1, &hunsec1,
				&hour2, &min2, &sec2, &hunsec2,
				line3) < 9	    );

	line2=strchr(line3, ',');
	if(line2 == NULL)
		return NULL;

	line2++;
	for (comma = 4; comma < 9; comma++)
	{
		tmp = line2;
		if(!(tmp=strchr(tmp, ','))) break;
		tmp++;
		if(*tmp == ' ') break;
		line2 = tmp;
	}
	if(*line2 == ',')
		line2++;

	current->lines = num = 0;
	current->start = 3600000*hour1 + 60000*min1 + 1000*sec1 + hunsec1*10;
	current->end   = 3600000*hour2 + 60000*min2 + 1000*sec2 + hunsec2*10;
	if (onlytime) return current;

	while (((tmp=strstr(line2, "\\n")) != NULL) || ((tmp=strstr(line2, "\\N")) != NULL) ){
		current->text[num] = av_malloc(tmp - line2 + 1);
		strncpy(current->text[num], line2, tmp - line2);
		current->text[num][tmp - line2] = '\0';
		line2 =tmp + 2;
		num++;
		current->lines++;
		if (current->lines >=  SUB_MAX_TEXT)
			return current;
	}

	current->text[num]=av_strdup(line2);
	current->lines++;

	return current;
}

static void sub_pp_ssa(GxSubPage* sub) {
	int l=sub->lines;
	char* so,*de,*start;

	while (l){
		/* eliminate any text enclosed with {}, they are font and color settings*/
		so=de=sub->text[--l];
		while (*so) {
			if(*so == '<')
			{
				for(start=so; *so && *so!='>'; so++);
				if(*so) so++; else{ so=start;break;}
			}else if(*so == '{' && so[1]=='\\'){
				for(start=so; *so && *so!='}'; so++);
				if(*so) so++; else{ so=start;break;}
			}else if(*so) {
				*de=*so;
				so++; de++;
			}
		}
		*de=*so;
	}
}

/*
*  PJS GxSubPages reader.
*  That's the "Phoenix Japanimation Society" format.
*  I found some of them in http://www.scriptsclub.org/ (used for anime).
*  The time is in tenths of second.
*
*  by set, based on code by szabi (dunnowhat sub format ;-)
*/
static GxSubPage* sub_read_line_pjs(GxStream *st,GxSubPage *current, int onlytime) {
    char text[LINE_LEN+1],* s, *d;

    if (!GxStream_ReadLine (st, line, LINE_LEN))
	return NULL;
    /* skip spaces*/
    for (s=line;* s && isspace(*s); s++);
    /* allow empty lines at the end of the file*/
    if (*s==0)
	return NULL;
    /* get the time*/
    if (sscanf (s, "%d,%d,", &(current->start),
		&(current->end)) <2) {
	return SUB_ERR;
    }
    /* the files I have are in tenths of second*/
    current->start*= 10;
    current->end*= 10;

	if (onlytime) return current;

    /* walk to the beggining of the string*/
    for (;* s; s++) if (*s==',') break;
    if (*s) {
	for (s++;* s; s++) if (*s==',') break;
	if (*s) s++;
    }
    if (*s!='"') {
	return SUB_ERR;
    }
    /* copy the string to the text buffer*/
    for (s++, d=text;* s && *s!='"'; s++, d++)
	*d=*s;
   * d=0;
    current->text[0] = av_strdup(text);
    current->lines = 1;

    return current;
}

static GxSubPage* sub_read_line_vtt(GxStream *st, GxSubPage *current, int onlytime)
{
	int a1 = 0, a2 = 0, a3 = 0, a4 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0;
	char* p = NULL;
	int i = 0, len = 0;

	while (!current->text[0]) {
		if (!GxStream_ReadLine(st, line, LINE_LEN)) {
			return NULL;
		}
		if (sscanf(line, "%d:%d:%d.%d --> %d:%d:%d.%d", &a1, &a2, &a3, &a4, &b1, &b2, &b3, &b4) < 8) {
			a1 = 0; a2 = 0; a3 = 0; a4 = 0; b1 = 0; b2 = 0; b3 = 0; b4 = 0;
			if (sscanf(line, "%d:%d.%d --> %d:%d.%d", &a2, &a3, &a4, &b2, &b3, &b4) < 6) {
				continue;
			}
		}
		current->start = a1*3600000 + a2*60000 + a3*1000 + a4;
		current->end   = b1*3600000 + b2*60000 + b3*1000 + b4;
		if (onlytime) return current;

		for (i = 0; i < SUB_MAX_TEXT;) {
			int blank = 1;

			if (!GxStream_ReadLine (st, line, LINE_LEN)) {
				break;
			}

			len = 0;
			for (p = line; *p != '\n' && *p != '\r' && *p; p++, len++) {
				if (*p != ' ' && *p != '\t') {
					blank = 0;
				}
			}

			if (len && !blank) {
				int j = 0, skip = 0;
				char* curptr = current->text[i] = av_malloc(len + 1);

				if (!current->text[i]) {
					return SUB_ERR;
				}

				for(; j < len; j++) {
					if(line[j] == '>') {
						skip = 0;
						continue;
					}
					if(line[j] == '<') {
						skip = 1;
						continue;
					}
					if(skip) {
						continue;
					}
					*curptr = line[j];
					curptr++;
				}
				*curptr = '\0';
				i++;
			} else {
				break;
			}
		}
		current->lines = i;
	}

	return current;
}

static void adjust_subs_time(GxSubPage* sub, float subtime, float fps, int block,
                             int sub_num, int sub_uses_time) {
	int n,m;
	GxSubPage* nextsub;
	int i = sub_num;
	unsigned long subfms = (sub_uses_time ? 100 : fps)*  subtime;
	unsigned long overlap = (sub_uses_time ? 100 : fps) / 5; // 0.2s

	n=m=0;
	if (i)	for (;;){
		if (sub->end <= sub->start){
			sub->end = sub->start + subfms;
			m++;
			n++;
		}
		if (!--i) break;
		nextsub = sub + 1;
	    if(block){
			if ((sub->end > nextsub->start) && (sub->end <= nextsub->start + overlap)) {
			    // these GxSubPages overlap for less than 0.2 seconds
			    // and would result in very short overlapping GxSubPage
			    // so let's fix the problem here, before overlapping code
			    // get its hands on them
			    unsigned delta = sub->end - nextsub->start, half = delta / 2;
			    sub->end -= half + 1;
			    nextsub->start += delta - half;
			}
			if (sub->end >= nextsub->start){
				sub->end = nextsub->start - 1;
				if (sub->end - sub->start > subfms)
					sub->end = sub->start + subfms;
				if (!m)
					n++;
			}
	    }

		sub = nextsub;
		m = 0;
	}
	if (n) gx_msg(MSGT_SUBREADER,MSGL_INFO,"SUB: Adjusted %d GxSubPage(s).\n", n);
}

static sub_reader sr[]=
 {
	    { sub_read_line_microdvd, NULL, "microdvd" },
	    { sub_read_line_subrip, NULL, "subrip" },
	    { sub_read_line_subviewer, NULL, "subviewer" },
	    { sub_read_line_vplayer, NULL, "vplayer" },
	    { sub_read_line_rt, NULL, "rt" },
	    { sub_read_line_ssa, sub_pp_ssa, "ssa" },
	    { sub_read_line_pjs, NULL, "pjs" },
	    { sub_read_line_subviewer2, NULL, "subviewer 2.0" },
	    { sub_read_line_mpl2, NULL, "mpl2" },
	    { sub_read_line_lrc, NULL, "MP3 LRC" },
	    { sub_read_line_sami,NULL, "smi"},
	    { sub_read_line_txt, NULL, "txt" },
	    { sub_read_line_vtt, NULL, "vtt" },
 };


static int detect_format (GxStream* st, int* uses_time)
{
    int i,j=0;

    while (j < 200) {
	j++;
	memset(line,0,LINE_LEN+1);
	if (!GxStream_ReadLine (st, line, LINE_LEN))
	    return SUB_INVALID;

	if (sscanf (line, "{%d}{%d}", &i, &i)==2)
		{*uses_time=0;return SUB_MICRODVD;}
	if (sscanf (line, "{%d}{}", &i)==1)
		{*uses_time=0;return SUB_MICRODVD;}
	if (sscanf (line, "[%d][%d]", &i, &i)==2)
		{*uses_time=1;return SUB_MPL2;}
	if (sscanf (line, "%d:%d:%d.%d,%d:%d:%d.%d",     &i, &i, &i, &i, &i, &i, &i, &i)==8)
		{*uses_time=1;return SUB_SUBRIP;}
	if (sscanf (line, "%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d", &i, &i, &i, (char* )&i, &i, &i, &i, &i, (char *)&i, &i)==10)
		{*uses_time=1;return SUB_SUBVIEWER;}
	if (sscanf (line, "{T %d:%d:%d:%d",&i, &i, &i, &i)==4)
		{*uses_time=1;return SUB_SUBVIEWER2;}
	if (sscanf (line, "%d:%d:%d%[: ]",     &i, &i, &i, (char*)&i)==4)
		{*uses_time=1;return SUB_VPLAYER;}
	if (sscanf (line, "%d:%d:%d%[=]",      &i, &i, &i, (char*)&i)==4)
		{*uses_time=1;return SUB_TXT;}
	if (sscanf (line, "[%d:%d.%d]",     &i, &i, &i )==3)
		{*uses_time=1;return SUB_LCR;}
	if (strstr (line, "<SAMI>"))
		{*uses_time=1; return SUB_SAMI;}
	//TODO: just checking if first line of sub starts with "<" is WAY
	// too weak test for RT
	// Please someone who knows the format of RT... FIX IT!!!
	// It may conflict with other sub formats in the future (actually it doesn't)
	if (!strncasecmp(line, "<window", 7))
		{*uses_time=1;return SUB_RT;}

	//ConvertUTF16toUTF8(&source ,&line[LINE_LEN],&target,&test[190],lenientConversion);
	if (!memcmp(line, "Dialogue: Marked", 16))
		{*uses_time=1; return SUB_SSA;}
	if (!memcmp(line, "Dialogue: ", 10))
		{*uses_time=1; return SUB_SSA;}
	if (sscanf (line, "%d,%d,\"%c", &i, &i, (char* ) &i) == 3)
		{*uses_time=1;return SUB_PJS;}
	if (strstr (line, "WEBVTT"))
		{*uses_time=1;return SUB_VTT;}

    }

    return SUB_INVALID;  // too many bad lines
}

static int detect_encoding(const char* lang,GxStream*  fd)
{
	int ret = 0;
	EncaAnalyser an;
	EncaEncoding result ;

	if(lang == NULL)
		an = enca_analyser_alloc("zh");
	else
		an = enca_analyser_alloc(lang);

	if (!an) {
	return -1;
	}

	GxStream_Read(fd,(uint8_t* )line,LINE_LEN);
	result =  enca_analyse(an,(unsigned char*)line,LINE_LEN);

	switch(result.charset){
	case 27:
		ret =  SUB_ENC_UTF8;
		break;
	case 29:
		ret = SUB_ENC_ANSI;
		break;
	default:
		ret = SUB_ENC_ANSI;
		break;
	}

	if((result.charset == -1)&&(line[0]==0xFF)&&(line[1]=0xFE))
		ret = SUB_ENC_UCS;
	enca_analyser_free(an);

	return ret;
}

static GxSubPage* subreader_handle_ssa(GxSubPage* subpg, uint32_t num, uint32_t* psub_num)
{
	GxSubPage* first, *second;
	int sub_num = 0;
    int n_max, n_first, i, j, sub_first, sub_orig;

	sub_orig = num;
	n_first = num;
	sub_num = 0;
	first = subpg;
	second = NULL;
	// for each GxSubPage in first[] we deal with its 'block' of
	// bonded GxSubPages
	for (sub_first = 0; sub_first < n_first; ++sub_first)
	{
		unsigned long global_start = first[sub_first].start,
					  global_end = first[sub_first].end, local_start, local_end;
		// here we manage overlapping GxSubPages
		int lines_to_add = first[sub_first].lines, sub_to_add = 0,
			**placeholder = NULL, higher_line = 0, counter, start_block_sub = sub_num;
		char real_block = 1;

		// here we find the number of GxSubPages inside the 'block'
		// and its span interval. this works well only with sorted
		// GxSubPages
		while ((sub_first + sub_to_add + 1 < n_first) && (first[sub_first + sub_to_add + 1].start < global_end))
		{
			++sub_to_add;
			lines_to_add += first[sub_first + sub_to_add].lines;
			if (first[sub_first + sub_to_add].start < global_start)
			{
				global_start = first[sub_first + sub_to_add].start;
			}
			if (first[sub_first + sub_to_add].end > global_end)
			{
				global_end = first[sub_first + sub_to_add].end;
			}
		}
		// we need a structure to keep trace of the screen lines
		// used by the subs, a 'placeholder'
		counter = 2*  sub_to_add + 1; // the maximum number of subs derived from a block of sub_to_add+1 subs
		placeholder = av_malloc(sizeof(int* ) * counter);
		for (i = 0; i < counter; ++i)
		{
			placeholder[i] = av_malloc(sizeof(int)*  lines_to_add);
			for (j = 0; j < lines_to_add; ++j)
			{
				placeholder[i][j] = -1;
			}
		}
		counter = 0;
		local_end = global_start - 1;
		do {
			int ls;
			// here we find the beginning and the end of a new
			// GxSubPage in the block
			local_start = local_end + 1;
			local_end   = global_end;
			for (j = 0; j <= sub_to_add; ++j)
			{
				if ((first[sub_first + j].start - 1 > local_start) && (first[sub_first + j].start - 1 < local_end))
				{
				    local_end = first[sub_first + j].start - 1;
				}
				else if ((first[sub_first + j].end > local_start) && (first[sub_first + j].end < local_end))
				{
				    local_end = first[sub_first + j].end;
				}
			}
			// here we allocate the screen lines to subs we must
			// display in current local_start-local_end interval.
			// if the subs were yet presents in the previous interval
			// they keep the same lines, otherside they get unused lines
			for (j = 0; j <= sub_to_add; ++j)
			{
				if ((first[sub_first + j].start <= local_end) && (first[sub_first + j].end > local_start))
				{
					unsigned long sub_lines = first[sub_first + j].lines,
								  fragment_length = lines_to_add + 1,tmp = 0;
					char boolean = 0;
					int fragment_position = -1;
					// if this is not the first new sub of the block
					// we find if this sub was present in the previous
					// new sub
					if (counter)
						for (i = 0; i < lines_to_add; ++i)
						{
							if (placeholder[counter - 1][i] == sub_first + j)
							{
								placeholder[counter][i] = sub_first + j;
								boolean = 1;
							}
						}
					if (boolean)
						continue;
					// we are looking for the shortest among all groups of
					// sequential blank lines whose length is greater than or
					// equal to sub_lines. we store in fragment_position the
					// position of the shortest group, in fragment_length its
					// length, and in tmp the length of the group currently
					// examinated
					for (i = 0; i < lines_to_add; ++i)
					{
						if (placeholder[counter][i] == -1)
						{
							// placeholder[counter][i] is part of the current group
							// of blank lines
							++tmp;
						}
						else
						{
							if (tmp == sub_lines)
							{
								// current group's size fits exactly the one we
								// need, so we stop looking
								fragment_position = i - tmp;
								tmp = 0;
								break;
							}
							if ((tmp) && (tmp > sub_lines) && (tmp < fragment_length))
							{
								// current group is the best we found till here,
								// but is still bigger than the one we are looking
								// for, so we keep on looking
								fragment_length = tmp;
								fragment_position = i - tmp;
								tmp = 0;
							}
							else
							{
								// current group doesn't fit at all, so we forget it
								tmp = 0;
							}
						}
					}
					if (tmp)
					{
						// last screen line is blank, a group ends with it
						if ((tmp >= sub_lines) && (tmp < fragment_length))
						{
							fragment_position = i - tmp;
						}
					}
					if (fragment_position == -1)
					{
						// it was not possible to find free screen line(s) for a GxSubPage,
						// usually this means a bug in the code; however we do not overlap
						gxlogf("SUB: we could not find a suitable position for an overlapping GxSubPage\n");
						higher_line = SUB_MAX_TEXT + 1;
						break;
					}
					else
					{
						for (tmp = 0; tmp < sub_lines; ++tmp)
						{
							placeholder[counter][fragment_position + tmp] = sub_first + j;
						}
					}
				}
			}
			for (j = higher_line + 1; j < lines_to_add; ++j)
			{
				if (placeholder[counter][j] != -1)
					higher_line = j;
				else
					break;
			}
			if (higher_line >= SUB_MAX_TEXT)
			{
				// the 'block' has too much lines, so we don't overlap the GxSubPages
				second = (GxSubPage* ) av_realloc(second, (sub_num + sub_to_add + 1) * sizeof(GxSubPage));
				for (j = 0; j <= sub_to_add; ++j)
				{
					int ls;
					memset(&second[sub_num + j], '\0', sizeof(GxSubPage));
					second[sub_num + j].start = first[sub_first + j].start;
					second[sub_num + j].end   = first[sub_first + j].end;
					second[sub_num + j].lines = first[sub_first + j].lines;
					for (ls = 0; ls < second[sub_num + j].lines; ls++)
					{
						second[sub_num + j].text[ls] = av_strdup(first[sub_first + j].text[ls]);
					}
				}
				sub_num += sub_to_add + 1;
				sub_first += sub_to_add;
				real_block = 0;
				break;
			}
			// we read the placeholder structure and create the new subs.
			second = (GxSubPage* ) av_realloc(second, (sub_num + 1) * sizeof(GxSubPage));
			memset(&second[sub_num], '\0', sizeof(GxSubPage));
			second[sub_num].start = local_start;
			second[sub_num].end   = local_end;
			n_max = (lines_to_add < SUB_MAX_TEXT) ? lines_to_add : SUB_MAX_TEXT;
			for (i = 0, j = 0; j < n_max; ++j)
			{
				if (placeholder[counter][j] != -1)
				{
					int lines = first[placeholder[counter][j]].lines;
					for (ls = 0; ls < lines; ++ls)
					{
						second[sub_num].text[i++] = av_strdup(first[placeholder[counter][j]].text[ls]);
					}
					j += lines - 1;
				}
				else
				{
					second[sub_num].text[i++] = av_strdup(" ");
				}
			}
			++sub_num;
			++counter;
		} while (local_end < global_end);

		if (real_block)
		for (i = 0; i < counter; ++i)
			second[start_block_sub + i].lines = higher_line + 1;
		counter = 2*  sub_to_add + 1;
		for (i = 0; i < counter; ++i)
		{
			av_free(placeholder[i]);
		}
		av_free(placeholder);
		sub_first += sub_to_add;
	}
	for (j = sub_orig - 1; j >= 0; --j)
	{
		for (i = first[j].lines - 1; i >= 0; --i)
		{
			av_free(first[j].text[i]);
		}
	}
	av_free(first);
	*psub_num = sub_num;
	return second;
}

uint32_t sub_get_duration(GxSubData *subd)
{
	sub_reader *srp = NULL;
	GxSubPage  *subpg = NULL, *p = NULL;
	int sub_format = subd->sub_format;
	int uses_time = subd->sub_uses_time;
	uint32_t start_timems = 0, end_timems = 0;
	off_t file_size = 0, init_pos = 0, cur_pos = 0;

	init_pos = GxStream_Tell((GxStream*)(subd->fd));
	subpg = av_mallocz(sizeof(GxSubPage));
	if (!subpg)
		return 0;
	srp = sr + sub_format;

	cur_pos = GxStream_Tell((GxStream*)(subd->fd));
	GxStream_Control((GxStream*)(subd->fd), GX_STREAM_CTRL_GET_SIZE, &file_size);
#define SUB_LOOK_POS (4096)
	if ((cur_pos + SUB_LOOK_POS) < file_size)
		GxStream_Seek((GxStream*)(subd->fd), (file_size - SUB_LOOK_POS));

	while (1) {
		uint32_t max_timems = 0;

		subpg->start = subpg->end = 0;
		p = srp->read((GxStream*)(subd->fd),subpg, 1);
		if (!p || (p == SUB_ERR))
			break;
		if (sub_format == SUB_SSA)
			adjust_subs_time(subpg, 6.0, 100, 1, 1, uses_time);/*~6 secs AST*/
		max_timems = (subpg->start > subpg->end) ? subpg->start : subpg->end;
		end_timems = (end_timems > max_timems) ? end_timems : max_timems;
	}

	av_free(subpg);
	GxStream_Reset((GxStream*)(subd->fd));
	GxStream_Seek ((GxStream*)(subd->fd), init_pos);

	return ((start_timems > end_timems) ? 0 : (end_timems - start_timems));
}

GxSubData* subreader_open_chr(const char* lang, const char* filename)
{
	int sub_format = SUB_INVALID;
	int encode=SUB_ENC_ERR;
	int uses_time;
	GxStream* fd = NULL;
	GxSubData* subd = NULL;

	if(filename == NULL)
		return NULL;
	fd=GxStream_Open(filename,GX_STREAM_READ, 0);
	if(!fd)
		return NULL;

	GxStream_Reset(fd);
	GxStream_Seek(fd,0);

	encode = detect_encoding(lang,fd);
	if(encode == -1)
	{
		gx_msg(MSGT_SUBREADER,MSGL_ERR,"SUB:Detect encoding PERR\n");
		return NULL;
	}

	fd->encode = encode;
	if(fd->encode == SUB_ENC_UCS){
		encode = SUB_ENC_UTF8;
		GxStream_Reset(fd);
		GxStream_Seek(fd,2);
	}else{
		GxStream_Reset(fd);
		GxStream_Seek(fd,0);
	}

	sub_format=detect_format(fd, &uses_time);
	if (sub_format==SUB_INVALID)
	{
		gx_msg(MSGT_SUBREADER,MSGL_ERR,"SUB: Could not determine file[%s] format\n",filename);
		GxStream_Close(fd);
		return NULL;
	}
	subd = av_malloc(sizeof(GxSubData));
	if(!subd)
		return NULL;

	GxStream_Reset(fd);
	if(fd->encode == SUB_ENC_UCS)
		GxStream_Seek(fd,2);
	else
		GxStream_Seek(fd,0);

	memset(subd,0,sizeof(GxSubData));
	subd->sub_hdr = NULL;
	subd->filename = av_strdup(filename);
	subd->sub_uses_time = uses_time;
	subd->sub_track = 1;
	subd->encode   = encode;
	subd->vob = NULL;
	subd->spu = NULL;
	subd->sub_format = sub_format;
	subd->fd = (int)fd;
	subd->sub_duration = sub_get_duration(subd);
	memcpy(&s_subd,subd,sizeof(GxSubData));

	sub_duration = 1;
	return subd;
}

int subreader_gets_chr_info(GxSubData* subd, GxSubInfo* subinfo)
{
	if(subd)
	{
		subinfo->cur_id = 0;
		subinfo->encode = subd->encode;
		subinfo->num = 1;
		subinfo->id[0] = 0;
		switch(subd->sub_format)
		{
			case SUB_MICRODVD:
				snprintf (subinfo->codec_id[0], sizeof(subinfo->codec_id[0]), "%s", "Microdvd");
				break;
			case SUB_SUBRIP:
				snprintf (subinfo->codec_id[0], sizeof(subinfo->codec_id[0]), "%s", "SubRip");
				break;
			case SUB_SUBVIEWER:
			case SUB_SUBVIEWER2:
				snprintf (subinfo->codec_id[0], sizeof(subinfo->codec_id[0]), "%s", "SRT");
				break;
			case SUB_VPLAYER:
				snprintf (subinfo->codec_id[0], sizeof(subinfo->codec_id[0]), "%s", "Vplayer");
				break;
			case SUB_SSA:
				snprintf (subinfo->codec_id[0], sizeof(subinfo->codec_id[0]), "%s", "ASS");
				break;
			case SUB_LCR:
				snprintf (subinfo->codec_id[0], sizeof(subinfo->codec_id[0]), "%s", "LRC");
				break;
			case SUB_SAMI:
				snprintf (subinfo->codec_id[0], sizeof(subinfo->codec_id[0]), "%s", "SAMI");
				break;
			case SUB_VTT:
				snprintf (subinfo->codec_id[0], sizeof(subinfo->codec_id[0]), "%s", "VTT");
				break;
		}
	}
	return 0;
}

static void free_subpage(GxSubPage* subpg, uint32_t num)
{
	int i,j;
	GxSubPage *p = NULL, *next = NULL;

	if((!subpg)||(!num))
		return;

	p = subpg;
	for(i = 0; (i < num) && p; i++)
	{
		next = p->next;
		for(j = 0; j < p->lines; j++)
		{
			if(p->text[j])
				av_free(p->text[j]);
		}
		av_free(p);
		p = next;
	}
	return;
}

static GxSubPage* alloc_onesubpage(GxSubData* subd)
{
	sub_reader *srp = NULL;
	GxSubPage  *subpg = NULL, *p = NULL;
	int sub_format = subd->sub_format;
	int uses_time = subd->sub_uses_time;
	uint32_t sub_num;

	srp=sr+sub_format;
	subpg = av_malloc(sizeof(GxSubPage));
	if(!subpg)
		return NULL;
	memset(subpg,0,sizeof(GxSubPage));
	p = srp->read((GxStream*)(subd->fd),subpg, 0);
	if(!p || (p == SUB_ERR))
	{
		av_free(subpg);
		return NULL;
	}
	if(srp->post)
		srp->post(p);

	if(sub_format == SUB_SSA)
	{
		adjust_subs_time(subpg, 6.0, 100, 1, 1, uses_time);/*~6 secs AST*/
		subpg = subreader_handle_ssa(subpg,1,&sub_num);
	}
	subpg->next = NULL;
	//gxlogf("start=%d end=%d text:%s\n",subpg->start,subpg->end,subpg->text[0]);
	return subpg;
}

int subreader_reset_chr(GxSubData* subd)
{
	GxStream* fd = (GxStream*)(subd->fd);
	GxStream_Reset(fd);
	if(fd->encode == SUB_ENC_UCS)
		GxStream_Seek(fd,2);
	else
		GxStream_Seek(fd,0);

	free_subpage(s_subd.sub_hdr,s_subd.sub_num);
	s_subd.sub_hdr = NULL;
	s_subd.sub_num = 0;
	return 0;
}

void subreader_free_chr(GxSubData* subd)
{
	if((!subd)||(!subd->sub_hdr)||(!subd->sub_num))
		return;

	free_subpage(subd->sub_hdr,subd->sub_num);
	subd->sub_num = 0;
	subd->sub_hdr = NULL;
	return;
}

int subreader_alloc_chr(GxSubData* subd, uint32_t sublines)
{
	int i,j,pos;
	GxSubPage *cur=NULL,*prev=NULL;
	GxSubPage *subpg=NULL;

	if(subd == NULL)
		return -1;

	for(i = s_subd.sub_num; i < MAX_PAGE_NUM; i = s_subd.sub_num)
	{
		subpg = alloc_onesubpage(&s_subd);
		if(subpg == NULL)
			break;
		subpg->encode = subd->encode;
		//判断插入的位置
		cur = s_subd.sub_hdr;
		for(j = 0; j < s_subd.sub_num; j++)
		{
			if(cur->start > subpg->start)
				break;
			cur = cur->next;
		}
		pos = j;
		//在相应位置插入数据
		cur = s_subd.sub_hdr;
		if(pos == 0)
		{
			s_subd.sub_hdr = subpg;
			s_subd.sub_hdr->next = cur;
		}else{
			for(j = 0; j < pos; j++)
			{
				prev = cur;
				cur = cur->next;
			}
			prev->next = subpg;
			subpg->next = cur;
		}
		s_subd.sub_num++;
	}
	if(s_subd.sub_num == 0)
		return -1;

	//gxlogf("%s %d sub_num=%d,sublines=%d\n",__FUNCTION__,__LINE__,s_subd.sub_num,sublines);
	if(s_subd.sub_num <= sublines)
	{
		subd->sub_num = s_subd.sub_num;
		subd->sub_hdr = s_subd.sub_hdr;
		s_subd.sub_hdr = NULL;
		s_subd.sub_num = 0;
	}else{
		cur = s_subd.sub_hdr;
		for(i = 0; i < sublines; i++)
		{
			prev = cur;
			cur = cur->next;
		}
		prev->next = NULL;
		subd->sub_hdr = s_subd.sub_hdr;
		subd->sub_num = sublines;
		s_subd.sub_hdr = cur;
		s_subd.sub_num -= sublines;
	}
	return 0;
}

void subreader_close_chr(GxSubData* subd)
{
	subreader_reset_chr(subd);
	GxStream_Close((GxStream*)(s_subd.fd));
	if(subd->filename)
		av_free(subd->filename);
	if(subd)
		av_free(subd);
	return;
}

int subreader_get_packet_chr(GxSubData* sub,void** pkt,int* len,int64_t* startpts ,int64_t* endpts)
{
	return 0;
}

void subreader_switch_stream_chr(GxSubData* sub,int id)
{
	return;
}

GxSubReaderClass subreader_chr = {
	.name = "Character Subtitle",
	.extensions = {".lrc",".LRC",".ass",".ASS",".ssa",".SSA",".srt",".SRT",".smi",".SMI",".sub",".SUB",".txt",".TXT",".vtt",".VTT",NULL},

	.open  = subreader_open_chr,
	.close = subreader_close_chr,
	.get_packet = subreader_get_packet_chr,
	.switch_stream = subreader_switch_stream_chr,
	.alloc_chr = subreader_alloc_chr,
	.free_chr = subreader_free_chr,
	.get_info = subreader_gets_chr_info,
	.reset_chr = subreader_reset_chr,
};
