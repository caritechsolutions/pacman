
#include "gx_common.h"
#include "ass.h"

#define MSGTR_LIBASS_FopenFailed "[ass] ass_read_file(%s): fopen failed\n"
#define MSGTR_LIBASS_FseekFailed "[ass] ass_read_file(%s): fseek failed\n"
#define MSGTR_LIBASS_RefusingToLoadSubtitlesLargerThan10M "[ass] ass_read_file(%s): Refusing to load subtitles larger than 10M\n"
#define MSGTR_LIBASS_ReadFailed "Read failed, %d: %s\n"

ass_track_t* ass_new_track(void){
	ass_track_t* track = av_calloc(1, sizeof(ass_track_t));
	return track;
}

void ass_free_track(ass_track_t* track) {
	if (track->event_format)
		av_free(track->event_format);

	av_free(track);
}

// ==============================================================================================

static void skip_spaces(char** str) {
	char* p =* str;
	while ((*p==' ') || (*p=='\t'))
		++p;
	*str = p;
}

static void rskip_spaces(char** str, char* limit) {
	char* p =* str;
	while ((p >= limit) && ((*p==' ') || (*p=='\t')))
		--p;
	*str = p;
}

#define NEXT(str,token) \
	token = next_token(&str); \
	if (!token) break;

#define ALIAS(alias,name) \
	if (strcasecmp(tname, #alias) == 0) {tname = #name;}

static char* next_token(char** str) 
{
	char* p =* str;
	char* start;
	skip_spaces(&p);
	if (*p == '\0') {
		*str = p;
		return 0;
	}
	start = p; // start of the token
	for (; (*p != '\0') && (*p != ','); ++p) {}
	if (*p == '\0') {
		*str = p; // eos found, str will point to '\0' at exit
	} else {
		*p = '\0';
		*str = p + 1; // ',' found, str will point to the next char (beginning of the next token)
	}
	--p; // end of current token
	rskip_spaces(&p, start);
	if (p < start)
		p = start; // empty token
	else
		++p; // the first space character, or '\0'
	*p = '\0';
	return start;
}

char* get_text(char *str, char *text, int max_len)
{
	char* p = str;
	int i = 0;

	if ((p=strchr(p, '{')) == NULL) {
		strncpy(text, str, max_len);
		return text;
	}

	while (*p != '\0' && i < max_len) {
		if (*p != '{') {

			if (*p == '\t')
				text[i++] = ' ';
			else if (*p == '\\') {
				if ((*(p+1) == 'N') || ((*(p+1)) == 'n')) {
					text[i++] = '\n';
					p+= 1;
				}
				else if ((*(p+1) == 'n') || (*(p+1) == 'h')) {
					text[i++] = ' ';
					p+= 1;
				}
			}
			else
				text[i++] =* p;
		}
		else
			while (*(++p) != '}');
		p++;
	}
	text[i] ='\0';

	return text;
}

static int process_event_tail(ass_track_t* track, char* str, int n_ignored)
{
	char* token;
	char* tname;
	char* p = str;
	int i;

	char format[128] = {'\0'};
	char* q = format;
	snprintf (format, sizeof(format)-1, "%s", "Format: Readorder, Layer, Style, Name, MarginL, MarginR, MarginV, Effect, Text");

	for (i = 0; i < n_ignored; ++i) {
		NEXT(q, tname);
	}

	while (1) {
		NEXT(q, tname);
		if (strcasecmp(tname, "Text") == 0) {
			char* last;
			ass_event_t* event = &track->events;
			strncpy(event->Text, p, 511);
			if (*event->Text != 0) {
				last = event->Text + strlen(event->Text) - 1;
				if (last >= event->Text &&* last == '\r')
					*last = 0;
			}
			gx_msg(MSGT_VOBSUB, MSGL_DBG2, "Text = %s\n", event->Text);
			event->Duration -= event->Start;
			return 0; // "Text" is always the last
		}
		NEXT(p, token);

		ALIAS(End,Duration) // temporarily store end timecode in event->Duration
	}

	return 1;
}

static int process_events_line(ass_track_t* track, char* str)
{
	if (!strncmp(str, "Format:", 7)) {
		char* p = str + 7;
		skip_spaces(&p);
		track->event_format = av_strdup(p);
		gx_msg(MSGT_VOBSUB, MSGL_DBG2, "Event format: %s\n", track->event_format);
	} else if (!strncmp(str, "Dialogue:", 9)) {
		str += 9;
		skip_spaces(&str);

		process_event_tail(track, str, 0);
	} else {
		skip_spaces(&str);
		process_event_tail(track, str, 0);
//		gx_msg(MSGT_VOBSUB, MSGL_V, "Not understood: %s  \n", str);
	}
	return 0;
}

static int process_line(ass_track_t* track, char* str)
{
	if (!strncasecmp(str, "[Script Info]", 13)) {
		track->state = PST_INFO;
	} else if (!strncasecmp(str, "[V4 Styles]", 11)) {
		track->state = PST_STYLES;
		track->track_type = TRACK_TYPE_SSA;
	} else if (!strncasecmp(str, "[V4+ Styles]", 12)) {
		track->state = PST_STYLES;
		track->track_type = TRACK_TYPE_ASS;
	} else if (!strncasecmp(str, "[Events]", 8)) {
		track->state = PST_EVENTS;
	} else if (!strncasecmp(str, "[Fonts]", 7)) {
		track->state = PST_FONTS;
	} else {
		switch (track->state) {
		case PST_EVENTS:
			process_events_line(track, str);
//			gxlogd("Text: %s\n", track->events.Text);
			break;
		default:
			break;
		}
	}

	return 0;
}

static int process_text(ass_track_t* track, char* str)
{
	char* p = str;
	while(1) {
		char* q;
		while (1) {
			if ((*p=='\r')||(*p=='\n')) ++p;
			else if (p[0]=='\xef' && p[1]=='\xbb' && p[2]=='\xbf') p+=3; // U+FFFE (BOM)
			else break;
		}
		for (q=p; ((*q!='\0')&&(*q!='\r')&&(*q!='\n')); ++q) {};
		if (q==p)
			break;
		if (*q != '\0')
			*(q++) = '\0';
		process_line(track, p);
		if (*q == '\0')
			break;
		p = q;
	}
	return 0;
}

int ass_process_data(ass_track_t* track, char* data, int size)
{
	char* str = av_malloc(size + 1);

	memcpy(str, data, size);
	str[size] = '\0';

	gx_msg(MSGT_VOBSUB, MSGL_V, "event: %s\n", str);
	process_text(track, str);
	if (track->state == PST_EVENTS) {
		strncpy(data, track->events.Text, size);

		av_free(str);	
		return 0;
	}
	av_free(str);

	return -1;
}

void ass_process_codec_private(ass_track_t* track, char* data, int size)
{
	ass_process_data(track, data, size);

	if (!track->event_format) {
		// probably an mkv produced by ancient mkvtoolnix
		// such files don't have [Events] and Format: headers
		track->state = PST_EVENTS;
		if (track->track_type == TRACK_TYPE_SSA)
			track->event_format = av_strdup("Format: Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text");
		else
			track->event_format = av_strdup("Format: Layer, Start, End, Style, Actor, MarginL, MarginR, MarginV, Effect, Text");
	}
}

#ifdef ASS_MAIN
static char* read_file(char* fname, size_t* bufsize)
{
	int res;
	long sz;
	long bytes_read;
	char* buf;

	FILE* fp = fopen(fname, "rb");
	if (!fp) {
		gx_msg(MSGT_VOBSUB, MSGL_WARN, MSGTR_LIBASS_FopenFailed, fname);
		return 0;
	}
	res = fseek(fp, 0, SEEK_END);
	if (res == -1) {
		gx_msg(MSGT_VOBSUB, MSGL_WARN, MSGTR_LIBASS_FseekFailed, fname);
		fclose(fp);
		return 0;
	}

	sz = ftell(fp);
	rewind(fp);

	if (sz > 10*1024*1024) {
		gx_msg(MSGT_VOBSUB, MSGL_INFO, MSGTR_LIBASS_RefusingToLoadSubtitlesLargerThan10M, fname);
		fclose(fp);
		return 0;
	}

	gx_msg(MSGT_VOBSUB, MSGL_V, "file size: %ld\n", sz);

	buf = av_malloc(sz + 1);
	assert(buf);

	bytes_read = 0;
	do {
		res = fread(buf + bytes_read, 1, sz - bytes_read, fp);
		if (res <= 0) {
			gx_msg(MSGT_VOBSUB, MSGL_INFO, MSGTR_LIBASS_ReadFailed, errno, strerror(errno));
			fclose(fp);
			av_free(buf);
			return 0;
		}
		bytes_read += res;
	} while (sz - bytes_read > 0);
	buf[sz] = '\0';
	fclose(fp);

	if (bufsize)
		*bufsize = sz;
	return buf;
}

int main(int argc, char* *argv)
{
//	size_t size;
	ass_track_t* track =ass_new_track();
//	char* text = read_file(argv[1], &size);
	FILE* fp = fopen(argv[1], "rb");
	char buf[512]; 
	char b2[512];

	while (!feof(fp)) {
		fgets(buf, 512, fp);
//		get_text(buf, b2, 511);
//		gxlogd("b2=%s\n", b2);
		if (ass_process_data(track, buf, strlen(buf)) == 0)
			gxlogd("TEXT: %s\n", buf);
	}
	fclose(fp);	

	ass_free_track(track);

	return 0;
}
#endif

