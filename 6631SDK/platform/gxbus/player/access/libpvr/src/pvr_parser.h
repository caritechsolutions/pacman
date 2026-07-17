#ifndef __PVR_PARSER_H__
#define __PVR_PARSER_H__

typedef struct {
	struct record_file *fd;
	PVRFileType         type;
	unsigned int        seqno;
	void               *fpriv;
	void               *mpriv;
	char               *path;
	off_t               overlap_offset;
} PVRParser;

PVRParser *pvr_parser_alloc     (const char *url);
void       pvr_parser_free      (PVRParser *parser);
int        pvr_parser_exist     (const char *url);

//set
int pvr_parser_write_line(PVRParser *parser, char *line, PVRFileMode mode);
int pvr_parser_set_seqno (PVRParser *parser, int seqno);

//get
char*    pvr_parser_get_curpath(PVRParser *parser);
char*    pvr_parser_get_arg (PVRParser *parser, PVRFileArgGetCmd cmd);
int32_t  pvr_parser_next_url(PVRParser *parser, char *url, unsigned int max_bytes);
int32_t  pvr_parser_get_url_byseqno(PVRParser *parser, int32_t seqno, char *url, unsigned int max_bytes);
int64_t  pvr_parser_get_duration(PVRParser *parser, PVRFileAttr attr);
off_t    pvr_parser_get_filesize(PVRParser *parser, PVRFileAttr attr);
char     pvr_parser_get_isfinish(PVRParser *parser);
int      pvr_parser_set_duration_byseqno(PVRParser *parser, int seqno, int64_t duration);
int      pvr_parser_set_filesize_byseqno(PVRParser *parser, int seqno, off_t filesize);
int64_t  pvr_parser_get_duration_byseqno(PVRParser *parser, int seqno);
int64_t  pvr_parser_get_filesize_byseqno(PVRParser *parser, int seqno);
char     pvr_parser_get_isfinish_byseqno(PVRParser *parser, int seqno);
int      pvr_parser_set_isfinish_byseqno(PVRParser *parser, int seqno, char isfinish);
char    *pvr_parser_get_keydesc_byseqno (PVRParser *parser, int seqno);
uint32_t pvr_parser_get_seqcount(PVRParser *parser);
int      pvr_parser_add_timems(PVRParser *parser, int64_t timems);
int      pvr_parser_add_offset(PVRParser *parser, int64_t offset);
int32_t  pvr_parser_get_seqno_bypos(PVRParser *parser, off_t pos);
int32_t  pvr_parser_get_seqno_bytime(PVRParser *parser, int64_t timems);
int32_t  pvr_parser_get_seqno_byprefix(PVRParser *parser, char *prefix);
int32_t  pvr_parser_get_seqno_byseqno(PVRParser *parser, int32_t seqno);
off_t    pvr_parser_get_startpos_byseqno (PVRParser *parser, int32_t seqno, PVRFileAttr attr);
int64_t  pvr_parser_get_starttime_byseqno(PVRParser *parser, int32_t seqno, PVRFileAttr attr);
int      pvr_parser_sync(PVRParser *parser);
int64_t  pvr_parser_get_mintime(PVRParser *parser);

#endif
