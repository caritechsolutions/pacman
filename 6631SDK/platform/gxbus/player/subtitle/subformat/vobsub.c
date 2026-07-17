#include "vobsub.h"
#include "spudec.h"

static vob_lang_t sub_lang = {0};

extern void* spudec_new_scaled(unsigned int *palette, unsigned int frame_width, unsigned int frame_height, uint8_t *extradata, int extradata_len);

/**********************************************************************
*  RAR stream handling
*  The RAR file must have the same basename as the file to open
* *********************************************************************/
#ifdef CONFIG_UNRAR_EXEC
typedef struct {
	FILE* file;
	unsigned char* data;
	unsigned long size;
	unsigned long pos;
} rar_stream_t;

static rar_stream_t* rar_open(const char* const filename, const char *const mode)
{
	rar_stream_t* stream;
	/* unrar_exec can only read*/
	if (strcmp("r", mode) && strcmp("rb", mode)) {
		errno = EINVAL;
		return NULL;
	}
	stream = av_malloc(sizeof(rar_stream_t));
	if (stream == NULL)
		return NULL;
	/* first try normal access*/
	stream->file = fopen(filename, mode);
	if (stream->file == NULL) {
		char* rar_filename;
		const char* p;
		int rc;
		/* Guess the RAR archive filename*/
		rar_filename = NULL;
		p = strrchr(filename, '.');
		if (p) {
			ptrdiff_t l = p - filename;
			rar_filename = av_malloc(l + 5);
			if (rar_filename == NULL) {
				av_free(stream);
				return NULL;
			}
			strncpy(rar_filename, filename, l);
			strncpy(rar_filename + l, ".rar", 4);
		}
		else {
			rar_filename = av_malloc(strlen(filename) + 5);
			if (rar_filename == NULL) {
				av_free(stream);
				return NULL;
			}
			strncpy(rar_filename, filename, strlen(filename) + 4);
			strncat(rar_filename, ".rar", strlen(filename) + 4);
		}
		/* get rid of the path if there is any*/
		if ((p = strrchr(filename, '/')) == NULL) {
			p = filename;
		} else {
			p++;
		}
		rc = unrar_exec_get(&stream->data, &stream->size, p, rar_filename);
		if (!rc) {
			/* There is no matching filename in the archive. However, sometimes
			 *  the files we are looking for have been given arbitrary names in the archive.
			 *  Let's look for a file with an exact match in the extension only. */
			int i, num_files, name_len;
			ArchiveList_struct* list, *lp;
			num_files = unrar_exec_list(rar_filename, &list);
			if (num_files > 0) {
				char* demanded_ext;
				demanded_ext = strrchr (p, '.');
				if (demanded_ext) {
					int demanded_ext_len = strlen (demanded_ext);
					for (i=0, lp=list; i<num_files; i++, lp=lp->next) {
						name_len = strlen (lp->item.Name);
						if (name_len >= demanded_ext_len && !strcasecmp (lp->item.Name + name_len - demanded_ext_len, demanded_ext)) {
							rc = unrar_exec_get(&stream->data, &stream->size,
									lp->item.Name, rar_filename);
							if (rc) break;
						}
					}
				}
				unrar_exec_freelist(list);
			}
			if (!rc) {
				av_free(rar_filename);
				av_free(stream);
				return NULL;
			}
		}

		av_free(rar_filename);
		stream->pos = 0;
	}
	return stream;
}

static int rar_close(rar_stream_t* stream)
{
	if (stream->file)
		return fclose(stream->file);
	av_free(stream->data);
	return 0;
}

static int rar_eof(rar_stream_t* stream)
{
	if (stream->file)
		return feof(stream->file);
	return stream->pos >= stream->size;
}

static long rar_tell(rar_stream_t* stream)
{
	if (stream->file)
		return ftell(stream->file);
	return stream->pos;
}

static int rar_seek(rar_stream_t* stream, long offset, int whence)
{
	if (stream->file)
		return fseek(stream->file, offset, whence);
	switch (whence) {
		case SEEK_SET:
			if (offset < 0) {
				errno = EINVAL;
				return -1;
			}
			stream->pos = offset;
			break;
		case SEEK_CUR:
			if (offset < 0 && stream->pos < (unsigned long) -offset) {
				errno = EINVAL;
				return -1;
			}
			stream->pos += offset;
			break;
		case SEEK_END:
			if (offset < 0 && stream->size < (unsigned long) -offset) {
				errno = EINVAL;
				return -1;
			}
			stream->pos = stream->size + offset;
			break;
		default:
			errno = EINVAL;
			return -1;
	}
	return 0;
}

static int rar_getc(rar_stream_t* stream)
{
	if (stream->file)
		return getc(stream->file);
	if (rar_eof(stream))
		return EOF;
	return stream->data[stream->pos++];
}

static size_t rar_read(void* ptr, size_t size, size_t nmemb, rar_stream_t *stream)
{
	size_t res;
	unsigned long remain;
	if (stream->file)
		return fread(ptr, size, nmemb, stream->file);
	if (rar_eof(stream))
		return 0;
	res = size*  nmemb;
	remain = stream->size - stream->pos;
	if (res > remain)
		res = remain / size*  size;
	memcpy(ptr, stream->data + stream->pos, res);
	stream->pos += res;
	res /= size;
	return res;
}

#else
typedef FILE rar_stream_t;
#define rar_open	fopen
#define rar_close	fclose
#define rar_eof		feof
#define rar_tell	ftell
#define rar_seek	fseek
#define rar_getc	getc
#define rar_read	fread
#endif

/**********************************************************************/

static ssize_t vobsub_getline(char* *lineptr, size_t *n, rar_stream_t *stream)
{
	size_t res = 0;
	int c;
	if (*lineptr == NULL) {
		*lineptr = av_malloc(4096);
		if (*lineptr)
			* n = 4096;
	}
	else if (*n == 0) {
		char* tmp = av_realloc(*lineptr, 4096);
		if (tmp) {
			* lineptr = tmp;
			* n = 4096;
		}
	}
	if (*lineptr == NULL ||* n == 0)
		return -1;

	for (c = rar_getc(stream); c != EOF; c = rar_getc(stream)) {
		if (res + 1 >=* n) {
			char* tmp = av_realloc(*lineptr, *n * 2);
			if (tmp == NULL)
				return -1;
			* lineptr = tmp;
			* n *= 2;
		}
		(*lineptr)[res++] = c;
		if (c == '\n') {
			(*lineptr)[res] = 0;
			return res;
		}
	}
	if (res == 0)
		return -1;
	(*lineptr)[res] = 0;
	return res;
}

/**********************************************************************
*  MPEG parsing
* *********************************************************************/

typedef struct {
	rar_stream_t* stream;
	unsigned int pts;
	int aid;
	unsigned char* packet;
	unsigned int packet_reserve;
	unsigned int packet_size;
} mpeg_t;

static mpeg_t* mpeg_open(const char* filename)
{
	mpeg_t* res = av_malloc(sizeof(mpeg_t));
	int err = (res == NULL);
	if (!err) {
		res->pts = 0;
		res->aid = -1;
		res->packet = NULL;
		res->packet_size = 0;
		res->packet_reserve = 0;
		res->stream = rar_open(filename, "rb");
		err = res->stream == NULL;
		if (err) {
			gxloge ("fopen Vobsub file failed");
			return NULL;
		}
		if (err)
			av_free(res);
	}
	return err ? NULL : res;
}

static void mpeg_close(mpeg_t* mpeg)
{
	if (mpeg->packet)
		av_free(mpeg->packet);
	if (mpeg->stream)
		rar_close(mpeg->stream);
	av_free(mpeg);
}

static int mpeg_eof(mpeg_t* mpeg)
{
	return rar_eof(mpeg->stream);
}

static uint32_t mpeg_tell(mpeg_t* mpeg)
{
	return rar_tell(mpeg->stream);
}

static int mpeg_seek(mpeg_t* mpeg, long offset, int whence)
{
	return rar_seek(mpeg->stream, offset, whence);
}

static int mpeg_run(mpeg_t* mpeg)
{
	unsigned int len, idx, version;
	int c;
	/* Goto start of a packet, it starts with 0x000001??*/
	const unsigned char wanted[] = { 0, 0, 1 };
	unsigned char buf[5];

	mpeg->aid = -1;
	mpeg->packet_size = 0;
	if (rar_read(buf, 4, 1, mpeg->stream) != 1)
		return -1;
	while (memcmp(buf, wanted, sizeof(wanted)) != 0)
	{
		c = rar_getc(mpeg->stream);
		if (c < 0)
			return -1;
		memmove(buf, buf + 1, 3);
		buf[3] = c;
	}
	gxlogf("%02x %02x %02x %02x\n",buf[0],buf[1],buf[2],buf[3]);
	switch (buf[3]) {
		case 0xb9:			/* System End Code*/
			break;
		case 0xba:			/* Packet start code*/
			c = rar_getc(mpeg->stream);
			if (c < 0)
				return -1;
			if ((c & 0xc0) == 0x40)
				version = 4;
			else if ((c & 0xf0) == 0x20)
				version = 2;
			else {
				gxlogd( "VobSub: Unsupported MPEG version: 0x%02x\n", c);
				return -1;
			}
			if (version == 4) {
				if (rar_seek(mpeg->stream, 9, SEEK_CUR))
					return -1;
			}
			else if (version == 2) {
				if (rar_seek(mpeg->stream, 7, SEEK_CUR))
					return -1;
			}
			else
				return -1;
			break;
		case 0xbd:			/* packet*/
			if (rar_read(buf, 2, 1, mpeg->stream) != 1)
				return -1;
			len = buf[0] << 8 | buf[1];
			idx = mpeg_tell(mpeg);
			c = rar_getc(mpeg->stream);
			if (c < 0)
				return -1;
			if ((c & 0xC0) == 0x40) { /* skip STD scale & size*/
				if (rar_getc(mpeg->stream) < 0)
					return -1;
				c = rar_getc(mpeg->stream);
				if (c < 0)
					return -1;
			}
			if ((c & 0xf0) == 0x20) { /* System-1 stream timestamp*/
				/* Do we need this?*/
				return -1;
			}
			else if ((c & 0xf0) == 0x30) {
				/* Do we need this?*/
				return -1;
			}
			else if ((c & 0xc0) == 0x80) { /* System-2 (.VOB) stream*/
				unsigned int pts_flags, hdrlen, dataidx;
				c = rar_getc(mpeg->stream);
				if (c < 0)
					return -1;
				pts_flags = c;
				c = rar_getc(mpeg->stream);
				if (c < 0)
					return -1;
				hdrlen = c;
				dataidx = mpeg_tell(mpeg) + hdrlen;
gxlogf("dataidx = %x, id = %x, len = %x\n",dataidx,idx,len);
				if (dataidx > idx + len) {
					gxlogd( "Invalid header length: %d (total length: %d, idx: %d, dataidx: %d)\n",
							hdrlen, len, idx, dataidx);
					return -1;
				}
				if ((pts_flags & 0xc0) == 0x80) {
					if (rar_read(buf, 5, 1, mpeg->stream) != 1)
						return -1;
					if (!(((buf[0] & 0xf0) == 0x20) && (buf[0] & 1) && (buf[2] & 1) &&  (buf[4] & 1))) {
						gxlogd( "vobsub PTS error: 0x%02x %02x%02x %02x%02x \n",
								buf[0], buf[1], buf[2], buf[3], buf[4]);
						mpeg->pts = 0;
					}
					else
						mpeg->pts = ((buf[0] & 0x0e) << 29 | buf[1] << 22 | (buf[2] & 0xfe) << 14
								| buf[3] << 7 | (buf[4] >> 1));
				}
				else /* if ((pts_flags & 0xc0) == 0xc0)*/ {
					/* what's this?*/
					/* abort();*/
				}
				rar_seek(mpeg->stream, dataidx, SEEK_SET);
				mpeg->aid = rar_getc(mpeg->stream);
				if (mpeg->aid < 0) {
					gxlogd( "Bogus aid %d\n", mpeg->aid);
					return -1;
				}
				mpeg->packet_size = len - ((unsigned int) mpeg_tell(mpeg) - idx);
				if (mpeg->packet_reserve < mpeg->packet_size) {
					if (mpeg->packet)
						av_free(mpeg->packet);
					mpeg->packet = av_malloc(mpeg->packet_size);
					if (mpeg->packet)
						mpeg->packet_reserve = mpeg->packet_size;
				}
				if (mpeg->packet == NULL) {
					gxlogf("av_malloc failure");
					mpeg->packet_reserve = 0;
					mpeg->packet_size = 0;
					return -1;
				}
				if (rar_read(mpeg->packet, mpeg->packet_size, 1, mpeg->stream) != 1) {
					gxlogd("fread failure");
					mpeg->packet_size = 0;
					return -1;
				}
				idx = len;
			}
			break;
		case 0xbe:			/* Padding*/
			if (rar_read(buf, 2, 1, mpeg->stream) != 1)
				return -1;
			len = buf[0] << 8 | buf[1];
			if (len > 0 && rar_seek(mpeg->stream, len, SEEK_CUR))
				return -1;
			break;
		default:
			if (0xc0 <= buf[3] && buf[3] < 0xf0) {
				/* MPEG audio or video*/
				if (rar_read(buf, 2, 1, mpeg->stream) != 1)
					return -1;
				len = buf[0] << 8 | buf[1];
				if (len > 0 && rar_seek(mpeg->stream, len, SEEK_CUR))
					return -1;

			}
			else {
				gxlogd("unknown header 0x%02X%02X%02X%02X\n",
						buf[0], buf[1], buf[2], buf[3]);
				return -1;
			}
	}
	return 0;
}

static int vobsub_parse_id(vobsub_t* vob, const char *line)
{
    // id: xx, index: n
    size_t idlen;
    const char* p, *q;

    if(strncmp("id:", line, 3)) return -1;
    p  = line+3;
    while (isspace(*p)) ++p;
    q = p;
    while (isalpha(*q)) ++q;
    idlen = q - p;
    if (idlen == 0) return -1;
    ++q;
    while (isspace(*q)) ++q;
    if (strncmp("index:", q, 6)) return -1;
    q += 6;
    while (isspace(*q)) ++q;
    if (!isdigit(*q)) return -1;

    if(sub_lang.num < 16 )
    {
        idlen = idlen > 16 ? 16 : idlen;
        memcpy(sub_lang.lang[sub_lang.num],p,idlen);
        sub_lang.id[sub_lang.num] = atoi(q);
        sub_lang.num++;
        gxlogf("num:%d, id:%d, lang:%s\n",sub_lang.num,sub_lang.id[sub_lang.num-1],sub_lang.lang[sub_lang.num-1]);
    }
    return 0;
}

static int vobsub_parse_timestamp(vob_stream_t* stream, const char *line)
{
    // timestamp: HH:MM:SS.mmm, filepos: 0nnnnnnnnn
    const char* p;
    int h, m, s, ms;
    uint32_t filepos;
    while (isspace(*line))
	++line;
    p = line;
    while (isdigit(*p))
	++p;
    if (p - line != 2)
	return -1;
    h = atoi(line);
    if (*p != ':')
	return -1;
    line = ++p;
    while (isdigit(*p))
	++p;
    if (p - line != 2)
	return -1;
    m = atoi(line);
    if (*p != ':')
	return -1;
    line = ++p;
    while (isdigit(*p))
	++p;
    if (p - line != 2)
	return -1;
    s = atoi(line);
    if (*p != ':')
	return -1;
    line = ++p;
    while (isdigit(*p))
	++p;
    if (p - line != 3)
	return -1;
    ms = atoi(line);
    if (*p != ',')
	return -1;
    line = p + 1;
    while (isspace(*line))
	++line;
    if (strncmp("filepos:", line, 8))
	return -1;
    line += 8;
    while (isspace(*line))
	++line;
    if (! isxdigit(*line))
	return -1;
    filepos = strtol(line, NULL, 16);

	stream->filepos = filepos;
	stream->startpts = ms + 1000 * (s + 60 * (m + 60 * h));
    return 0;
}

static int vobsub_parse_origin(vobsub_t* vob, const char *line)
{
    // org: X,Y
    char* p;
    while (isspace(*line))
	++line;
    if (!isdigit(*line))
	return -1;
    vob->origin_x = strtoul(line, &p, 10);
    if (*p != ',')
	return -1;
    ++p;
    vob->origin_y = strtoul(p, NULL, 10);
    return 0;
}

static int vobsub_parse_delay(vobsub_t* vob, const char *line)
{
    int h, m, s, ms;
    int forward = 1;
    if (*(line + 7) == '+'){
		forward = 1;
	line++;
    }
    else if (*(line + 7) == '-'){
		forward = -1;
	line++;
    }

    h = atoi(line + 7);

    m = atoi(line + 10);

    s = atoi(line + 13);

    ms = atoi(line + 16);

    vob->delay = (ms + 1000*  (s + 60 * (m + 60 * h))) * forward;
    return 0;
}

static int vobsub_set_lang(vobsub_t*vob, const char* line)
{
	sub_lang.cur_id = atoi(line + 8);
    gxlogf("cur_id:%d\n",sub_lang.cur_id);
    return 0;
}

static int vobsub_parse_timespos(vob_stream_t* stream, rar_stream_t *fd)
{
    ssize_t line_size;
    int res = -1;
	size_t line_reserve = 0;
	char* line = NULL;
    do {
		if(line){
			av_free(line);
			line = NULL;
		}
		line_size = vobsub_getline(&line, &line_reserve, fd);
		if (line_size < 0 || line_size > 1000000)
			break;

		if (strncmp("timestamp:", line, 10) == 0)
		{
			res = vobsub_parse_timestamp(stream, line + 10);
			break;
		}
		else
			continue;
	} while (1);

	if (line)
      av_free(line);
    return res;
}

static void vobsub_parse_extradata(rar_stream_t* fd, vobsub_t* vob, unsigned char* *extradata, unsigned int *extradata_len)
{
	size_t line_size;
	size_t line_reserve = 0;
	char* line = NULL;

	while(!rar_eof(fd))
	{
		if(line){
			av_free(line);
			line = NULL;
		}
		line_size = vobsub_getline(&line,&line_reserve,fd);
		if(line_size < 0 || line_size > 512*1024 || *extradata_len + line_size > 512*1024)
			break;

		if (*line==0 || *line=='\r' || *line=='\n' || *line=='#' || (strncmp("timestamp:", line, 10)==0))
			continue;
		else if (strncmp("langidx:", line, 8) == 0)
		{
			vobsub_set_lang(vob,line);
			continue;
		}
		else if (strncmp("delay:", line, 6) == 0)
		{
			vobsub_parse_delay(vob,line);
			continue;
		}
		else if (strncmp("id:", line, 3) == 0)
		{
			vobsub_parse_id(vob,line);
			continue;
		}
		else if (strncmp("org:", line, 4) == 0)
		{
			vobsub_parse_origin(vob,line);
			continue;
		}
		else
		{
			*extradata = av_realloc(*extradata,* extradata_len+line_size+1);
			memcpy(*extradata+*extradata_len, line, line_size);
			*extradata_len += line_size;
			(*extradata)[*extradata_len] = 0;
		}
	}
	if (line)
      av_free(line);
	return;
}

int vobsub_parse_ifo(void* this, const char* const name, unsigned int *palette,
				unsigned int *width, unsigned int *height, int force,int sid, char* langid)
{
    vobsub_t* vob = (vobsub_t*)this;
    int res = -1;
    rar_stream_t* fd = rar_open(name, "rb");
    if (fd == NULL) {
        if (force)
	    gxlogf( "VobSub: Can't open IFO file\n");
    } else {
	// parse IFO header
	unsigned char block[0x800];
	const char* const ifo_magic = "DVDVIDEO-VTS";
	if (rar_read(block, sizeof(block), 1, fd) != 1) {
	    if (force)
		gxlogd( "VobSub: Can't read IFO header\n");
	} else if (memcmp(block, ifo_magic, strlen(ifo_magic) + 1))
	    gxlogd( "VobSub: Bad magic in IFO header\n");
	else {
	    unsigned long pgci_sector = block[0xcc] << 24 | block[0xcd] << 16
		| block[0xce] << 8 | block[0xcf];
	    int standard = (block[0x200] & 0x30) >> 4;
	    int resolution = (block[0x201] & 0x0c) >> 2;
	   * height = standard ? 576 : 480;
	   * width = 0;
	    switch (resolution) {
	    case 0x0:
		*width = 720;
		break;
	    case 0x1:
		*width = 704;
		break;
	    case 0x2:
		*width = 352;
		break;
	    case 0x3:
		*width = 352;
		*height /= 2;
		break;
	    default:
		gxlogd("Vobsub: Unknown resolution %d \n", resolution);
	    }
	    if (langid && 0 <= sid && sid < 32) {
		unsigned char* tmp = block + 0x256 + sid * 6 + 2;
		langid[0] = tmp[0];
		langid[1] = tmp[1];
		langid[2] = 0;
	    }
	    if (rar_seek(fd, pgci_sector*  sizeof(block), SEEK_SET)
		|| rar_read(block, sizeof(block), 1, fd) != 1)
		gxlogd( "VobSub: Can't read IFO PGCI\n");
	    else {
		unsigned long idx;
		unsigned long pgc_offset = block[0xc] << 24 | block[0xd] << 16
		    | block[0xe] << 8 | block[0xf];
		for (idx = 0; idx < 16; ++idx) {
		    unsigned char* p = block + pgc_offset + 0xa4 + 4 * idx;
		    palette[idx] = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
		}
		if(vob)
		  vob->have_palette = 1;
		res = 0;
	    }
	}
	rar_close(fd);
    }
    return res;
}

int vobsub_open(const char* const filename, void** pspu, void** pvob)
{
	rar_stream_t* fd = NULL;
	vobsub_t* vob = NULL;
	GxSpuDec* spu = NULL;
	unsigned char* extradata = NULL;
	unsigned int extradata_len = 0;
	char ifo[256], sub[256];
	char* pos;

	if((!filename)||(!(pos = strstr(filename, ".idx")) && !(pos = strstr(filename, ".IDX"))))
		return -1;
	memcpy(ifo,filename,pos-filename);
	memcpy(sub,filename,pos-filename);
	ifo[pos-filename]='\0';
	sub[pos-filename]='\0';
	strncat(ifo,".ifo", sizeof(ifo)-1);
	strncat(sub,".sub", sizeof(sub)-1);
	vob = (vobsub_t*)av_malloc(sizeof(vobsub_t));
	if(vob == NULL)
		return -1;
	memset(vob,0,sizeof(vobsub_t));
	memset(&sub_lang,0,sizeof(vob_lang_t));
	vobsub_parse_ifo(vob,ifo, vob->palette, &vob->orig_frame_width, &vob->orig_frame_height,0, -1, NULL);
	fd = rar_open(filename, "rb");
	if(!fd)
	{
		av_free(vob);
		return -1;
	}
	if (access((char *)sub, F_OK) != 0)
	{
		av_free(vob);
		rar_close(fd);
		gxlogd("%s is not exist\n", sub);
		return -1;
	}

	vobsub_parse_extradata(fd, vob, &extradata, &extradata_len);
	spu = spudec_new_scaled(vob->palette, vob->orig_frame_width, vob->orig_frame_height, extradata, extradata_len);
	if(extradata)
		av_free(extradata);
	if(spu)
		spudec_reset(spu);
	rar_seek(fd,0,SEEK_SET);
	vob->orig_frame_width = spu->orig_frame_width;
	vob->orig_frame_height = spu->orig_frame_height;
	*pspu = spu;
	*pvob = vob;
	return (int)fd;
}

void vobsub_close(int fd, void* spudec, void* vob)
{
	spudec_free(spudec);
	av_free(vob);
	rar_close((rar_stream_t*)fd);
	return;
}

static int vobsub_get_next_chr(int fd, uint32_t* nextpts, uint32_t* nextpos)
{
	long pos = rar_tell((rar_stream_t*)fd);
	vob_stream_t* streams = NULL;

	streams = (vob_stream_t*)av_malloc(sizeof(vob_stream_t));
	if(!streams)
		return -1;
	memset(streams,0,sizeof(vob_stream_t));

	if(vobsub_parse_timespos(streams,(rar_stream_t*)fd) < 0)
	{
		av_free(streams);
		rar_seek((rar_stream_t*)fd, pos, SEEK_SET);
		return -1;
	}
	if(nextpts)
		*nextpts = streams->startpts;
	if(nextpos)
		*nextpos = streams->filepos;
	av_free(streams);
	rar_seek((rar_stream_t*)fd, pos, SEEK_SET);
	return 0;
}

vob_stream_t* vobsub_alloc_oneline_chr(int fd)
{
	int ret;
	uint32_t endpts = 0,endpos = 0;
	vob_stream_t* streams = NULL;

	streams = (vob_stream_t*)av_malloc(sizeof(vob_stream_t));
	if(!streams)
		return NULL;
	memset(streams,0,sizeof(vob_stream_t));

	ret = vobsub_parse_timespos(streams, (rar_stream_t*)fd);/* NOOP*/
	if(ret < 0)
	{
		av_free(streams);
		return NULL;
	}
	vobsub_get_next_chr(fd, &endpts,&endpos);
	if(streams->startpts > endpts)
	{
		endpos = 0;
		endpts = 0;
	}
	streams->endpts = endpts;
	streams->endpos = endpos;
gxlogf("%s %d: startpts=%d startpos=%d\n",__func__,__LINE__,streams->startpts,streams->filepos);
gxlogf("%s %d: endpts=%d   endpos=%d\n"  ,__func__,__LINE__,streams->endpts,  streams->endpos);
	return streams;
}

void vobsub_reset_oneline_chr(int fd)
{
	rar_seek((rar_stream_t*)fd, 0, SEEK_SET);
	return;
}

static vob_packet_t* alloc_one_packet(mpeg_t* mpg, void* this)
{
	GxSpuDec* spu = (GxSpuDec*)this;
	vob_packet_t* packet = av_malloc(sizeof(vob_packet_t));
	if(!packet)
		return NULL;
	memset(packet,0,sizeof(vob_packet_t));

	spudec_assemble(spu, mpg->packet, mpg->packet_size, mpg->pts);
	if (spu->queue_head)
	{
		spudec_heartbeat(spu, spu->queue_head->start_pts);
		if (spudec_changed(spu))
		{
			packet->data = av_malloc(spu->image_size);
			if(!(packet->data))
			{
				av_free(packet);
				return NULL;
			}
			memcpy(packet->data,spu->image,spu->image_size);
			packet->id = mpg->aid & 0x1f;
			packet->size = spu->image_size;
			if((spu->end_pts <= spu->start_pts)||(spu->end_pts >= spu->start_pts + 450000))
				spu->end_pts = spu->start_pts + 45000;
			packet->pts100 = (spu->end_pts-spu->start_pts)/90;
			return packet;
		}
	}
	av_free(packet);
	return NULL;
}

vob_packet_t* vobsub_alloc_oneline_packet(const char* const filename, vob_stream_t* stream, void* spu)
{
	mpeg_t* mpg = NULL;
	int ret;
	uint32_t endpos = stream->endpos;
	uint32_t filepos = stream->filepos;
	vob_packet_t* pkt;
	vob_packet_t** p = &pkt;

	if(!stream || !filename)
		return NULL;
	mpg = mpeg_open(filename);
	if(!mpg)
		return NULL;
	mpeg_seek(mpg,filepos,SEEK_SET);

	while(!mpeg_eof(mpg)&&((0 == endpos)||(mpeg_tell(mpg) < endpos)))
	{
		ret = mpeg_run(mpg);
		if(ret < 0)
		{
			break;
		}
		if((mpg->packet_size)&&((mpg->aid & 0xe0) == 0x20)&&((mpg->aid & 0x1f) == sub_lang.cur_id))
		{
			*p = alloc_one_packet(mpg,spu);
			if(*p == NULL)
				continue;
			p = &((*p)->next);
		}
	}
	*p = NULL;
	mpeg_close(mpg);
	return pkt;
}

void vobsub_free_oneline_chr(vob_stream_t* stream)
{
	if(stream)
		av_free(stream);
	return;
}

void vobsub_free_oneline_packet(vob_packet_t* packet)
{
	vob_packet_t* p;
	vob_packet_t* next;

	p = packet;
	while(p)
	{
		next = p->next;
		if(p->data)
			av_free(p->data);
		av_free(p);
		p = next;
	}
}

void vobsub_switch_stream_spu(int id)
{
	int i;
	for(i = 0; i < sub_lang.num; i++)
	{
		if(id == sub_lang.id[i])
		{
			sub_lang.cur_id = id;
			break;
		}
	}
}

void vobsub_get_info_spu(vob_lang_t* subinfo)
{
	memcpy(subinfo,&sub_lang,sizeof(vob_lang_t));
}
