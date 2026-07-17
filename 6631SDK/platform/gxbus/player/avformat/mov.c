#include <limits.h>

#include "../avutil/intreadwrite.h"
#include "../avutil/avstring.h"
#include "avformat.h"
#include "riff.h"
#include "isom.h"
#include "../avcodec/mpeg4audio.h"
#include "../avcodec/mpegaudiodata.h"
#include "../avutil/get_bits.h"
#include "../avutil/aes_ctr.h"
#include "gx_stream.h"
#include "../avutil/aes.h"
#include "dovi_isom.h"

#ifdef CONFIG_ZLIB
#include <zlib.h>
#endif

#define DEBUG_METADATA
#define MOV_EXPORT_ALL_METADATA

//#include "qtpalette.h"
#include <assert.h>

#define MOV_CTTS_OPTIMIZE (50000)
#define MOV_STTS_OPTIMIZE (50000)

/* XXX: it's the first time I make a recursive parser I think... sorry if it's ugly :P*/

/* those functions parse an atom*/
/* return code:
0: continue to parse next atom
<0: error occurred, exit
*/
/* links atom IDs to parse functions*/
typedef struct MOVParseTableEntry {
	uint32_t type;
	int (*parse)(MOVContext* ctx, ByteIOContext *pb, MOVAtom atom);
} MOVParseTableEntry;

static const MOVParseTableEntry mov_default_parse_table[];
static void mov_free_encryption_index(MOVEncryptionIndex **index);

typedef struct{
	int start;
	int end;
}MovIndex;

extern int ff_add_index_entry_mov(AVIndexEntry **index_entries,
		int *nb_index_entries,
		unsigned int *index_entries_allocated_size,
		int *index_entries_start,
		int *index_entries_end,
		int64_t pos, int64_t timestamp, int size, int distance, int flags, int fragment, int nPlusOnly,
		int index_max_count);

static int mov_add_index_entry(AVStream*  st,
		int64_t pos, int64_t timestamp, int size, int distance, int flags, int fragment, int nPlusOnly)
{
	int index_count = ((st->codec->codec_type == CODEC_TYPE_SUBTITLE) ? MOV_SUB_INDEX_COUNT : MOV_INDEX_COUNT);

	return ff_add_index_entry_mov(&st->index_entries,&st->nb_index_entries,
			&st->index_entries_allocated_size,&st->index_entries_start,&st->index_entries_end,pos,
			timestamp, size, distance, flags, fragment, nPlusOnly, index_count);
}

static int mov_metadata_trkn(MOVContext* c, ByteIOContext *pb, unsigned len)
{
	char buf[16];

	get_be16(pb); // unknown
	snprintf(buf, sizeof(buf), "%d", get_be16(pb));
	av_dict_set(&c->fc->metadata, "track", buf, 0);

	get_be16(pb); // total tracks

	return 0;
}

static int mov_read_udta_string(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
#ifdef MOV_EXPORT_ALL_METADATA
	char tmp_key[5];
#endif
	char str[1024], key2[16], language[4] = {0};
	const char* key = NULL;
	uint16_t str_size;
	int (*parse)(MOVContext*, ByteIOContext*, unsigned) = NULL;

	switch (atom.type) {
	case MKTAG(0xa9,'n','a','m'): key = "title";     break;
	case MKTAG(0xa9,'a','u','t'):
	case MKTAG(0xa9,'A','R','T'):
	case MKTAG(0xa9,'w','r','t'): key = "author";    break;
	case MKTAG(0xa9,'c','p','y'): key = "copyright"; break;
	case MKTAG(0xa9,'c','m','t'):
	case MKTAG(0xa9,'i','n','f'): key = "comment";   break;
	case MKTAG(0xa9,'a','l','b'): key = "album";     break;
	case MKTAG(0xa9,'d','a','y'): key = "year";      break;
	case MKTAG(0xa9,'g','e','n'): key = "genre";     break;
	case MKTAG(0xa9,'t','o','o'):
	case MKTAG(0xa9,'e','n','c'): key = "muxer";     break;
	case MKTAG( 't','r','k','n'): key = "track";
								  parse = mov_metadata_trkn; break;
	}

	if (c->itunes_metadata && atom.size > 8) {
		int data_size = get_be32(pb);
		int tag = get_le32(pb);
		if (tag == MKTAG('d','a','t','a')) {
			get_be32(pb); // type
			get_be32(pb); // unknown
			str_size = data_size - 16;
			atom.size -= 16;
		} else return 0;
	} else if (atom.size > 4 && key && !c->itunes_metadata) {
		str_size = get_be16(pb); // string length
		mov_lang_to_iso639(get_be16(pb), language);
		atom.size -= 4;
	} else
		str_size = atom.size;

#ifdef MOV_EXPORT_ALL_METADATA
	if (!key) {
		snprintf(tmp_key, 5, "%.4s", (char*)&atom.type);
		key = tmp_key;
	}
#endif

	if (!key)
		return 0;
	if (atom.size < 0)
		return -1;

	str_size = GXMIN(GXMIN(sizeof(str)-1, str_size), atom.size);

	if (parse)
		parse(c, pb, str_size);
	else {
		get_buffer(pb, (unsigned char*)str, str_size);
		str[str_size] = 0;
		av_dict_set(&c->fc->metadata, key, str, 0);
		if (*language && strcmp(language, "und")) {
			snprintf(key2, sizeof(key2), "%s-%s", key, language);
			av_dict_set(&c->fc->metadata, key2, str, 0);
		}
	}
#ifdef DEBUG_METADATA
	gxlogf("lang \"%3s\" ", language);
	gxlogf("tag \"%s\" value \"%s\" atom \"%.4s\" %d %lld\n",key, str, (char*)&atom.type, str_size, atom.size);
#endif

	return 0;
}

static int check_index_space(AVFormatContext* fc, int size, int flag)
{
	if (flag == 0) {
		fc->index_space += size;
		if (fc->index_space >= fc->index_limit)
			return -1;
	} else if (flag == 1) {
		fc->index_space -= size;
	}

	return 0;
}

static int64_t get_chunk_offset_by_index(MOVStreamContext* sc, int index)
{
	int64_t current_offset = 0;

	if (sc->chunk_bytes == 4) {
		unsigned int *chunk_offsets = sc->chunk_offsets;
		current_offset = (int64_t)(chunk_offsets[index]);
	} else if (sc->chunk_bytes == 8) {
		int64_t *chunk_offsets = sc->chunk_offsets;
		current_offset = chunk_offsets[index];
	}

	return current_offset;
}

static int mov_read_default(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	int64_t total_size = 0;
	MOVAtom a;
	int i;
	int err = 0;

	a.offset = atom.offset;

	if (atom.size < 0)
		atom.size = INT64_MAX;
	while(((total_size + 8) < atom.size) && !url_feof(pb) && !err) {
		int (*parse)(MOVContext*, ByteIOContext*, MOVAtom) = NULL;

		if(url_check_interrupt_cb())
			return AVERROR_EXIT;

		a.size = atom.size;
		a.type=0;
		if(atom.size >= 8) {
			a.size = get_be32(pb);
			a.type = get_le32(pb);
		}
		total_size += 8;
		a.offset += 8;

		if (a.size == 1) { /* 64 bit extended size*/
			a.size = get_be64(pb) - 8;
			a.offset += 8;
			total_size += 8;
		}

		gxlogd ("type:'%s' parent:'%s' sz: %lld %lld %lld\n",
				av_fourcc2str(a.type), av_fourcc2str(atom.type), a.size, total_size, atom.size);

		if (a.size == 0) {
			a.size = atom.size - total_size;
			if (a.size <= 8)
				break;
		}
		a.size -= 8;
		if(a.size < 0)
			break;
		a.size = FFMIN(a.size, atom.size - total_size);

		//gxlogd("%c%c%c%c\n", (a.type)&0xff, (a.type>>8)&0xff, (a.type>>16)&0xff, (a.type>>24)&0xff);
		for (i = 0; mov_default_parse_table[i].type; i++)
			if (mov_default_parse_table[i].type == a.type) {
				parse = mov_default_parse_table[i].parse;
				break;
			}

		// container is user data
		if (!parse && (atom.type == MKTAG('u','d','t','a') ||
					atom.type == MKTAG('i','l','s','t')))
			parse = mov_read_udta_string;

		if (!parse) { /* skip leaf atoms data*/
			url_fskip(pb, a.size);
		} else {
			int64_t start_pos = url_ftell(pb);
			int64_t left;
			err = parse(c, pb, a);
			if (c->found_moov && c->found_mdat) {
				c->next_root_atom = start_pos + a.size;
				break;
			}
			left = a.size - url_ftell(pb) + start_pos;
			if (left > 0) /* skip garbage at atom end*/
				url_fskip(pb, left);
			else
				avio_seek(pb, left, SEEK_CUR);
		}

		a.offset += a.size;
		total_size += a.size;
	}

	if (!err && total_size < atom.size && atom.size < 0x7ffff)
		url_fskip(pb, atom.size - total_size);

	return err;
}

static int mov_read_dref(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;
	MOVStreamContext* sc;
	int entries, i, j;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];
	sc = st->priv_data;

	get_be32(pb); // version + flags
	entries = get_be32(pb);
	if (entries >= UINT_MAX / sizeof(*sc->drefs))
		return -1;

	if (check_index_space(c->fc, entries * sizeof(*sc->drefs), 0) < 0)
		return -1;

	sc->drefs = av_mallocz(entries*  sizeof(*sc->drefs));
	if (!sc->drefs) {
		gxloge ("malloc failed!! %s:entries:%d,size:%d\n", __func__, entries, entries*sizeof(*sc->drefs));
		return AVERROR(ENOMEM);
	}
	sc->drefs_count = entries;

	for (i = 0; i < sc->drefs_count; i++) {
		MOVDref* dref = &sc->drefs[i];
		uint32_t size = get_be32(pb);
		int64_t next = url_ftell(pb) + size - 4;

		dref->type = get_le32(pb);
		get_be32(pb); // version + flags

		if (dref->type == MKTAG('a','l','i','s') && size > 150) {
			/* macintosh alias record*/
			uint16_t volume_len, len;
			char volume[28];
			int16_t type;

			url_fskip(pb, 10);

			volume_len = get_byte(pb);
			volume_len = GXMIN(volume_len, 27);
			get_buffer(pb,(unsigned char*)volume, 27);
			volume[volume_len] = 0;

			url_fskip(pb, 112);

			for (type = 0; type != -1 && url_ftell(pb) < next; ) {
				type = get_be16(pb);
				len = get_be16(pb);
				if (len&1)
					len += 1;
				if (type == 2) { // absolute path
					av_free(dref->path);
					dref->path = av_mallocz(len+1);
					if (!dref->path) {
						gxloge ("malloc failed!! %s:size:%d\n", __func__, len);
						return AVERROR(ENOMEM);
					}
					get_buffer(pb,(unsigned char*)dref->path, len);
					if (len > volume_len && !strncmp(dref->path, volume, volume_len)) {
						len -= volume_len;
						memmove(dref->path, dref->path+volume_len, len);
						dref->path[len] = 0;
					}
					for (j = 0; j < len; j++)
						if (dref->path[j] == ':')
							dref->path[j] = '/';
				} else
					url_fskip(pb, len);
			}
		}
		url_fseek(pb, next, SEEK_SET);
	}
	return 0;
}

static int mov_read_hdlr(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;
	uint32_t type;
	uint32_t ctype;

	if (c->fc->nb_streams < 1) // meta before first trak
		return 0;

	st = c->fc->streams[c->fc->nb_streams-1];

	get_byte(pb); /* version*/
	get_be24(pb); /* flags*/

	/* component type*/
	ctype = get_le32(pb);
	type = get_le32(pb); /* component subtype*/

	if     (type == MKTAG('v','i','d','e'))
		st->codec->codec_type = CODEC_TYPE_VIDEO;
	else if(type == MKTAG('s','o','u','n'))
		st->codec->codec_type = CODEC_TYPE_AUDIO;
	else if(type == MKTAG('m','1','a',' '))
		st->codec->codec_id = CODEC_ID_MP2;
	else if(type == MKTAG('s','u','b','p'))
		st->codec->codec_type = CODEC_TYPE_SUBTITLE;

	get_be32(pb); /* component  manufacture*/
	get_be32(pb); /* component flags*/
	get_be32(pb); /* component flags mask*/

	if(atom.size <= 24)
		return 0; /* nothing left to read*/

	url_fskip(pb, atom.size - (url_ftell(pb) - atom.offset));
	return 0;
}

int ff_mp4_read_descr_len(ByteIOContext* pb)
{
	int len = 0;
	int count = 4;
	while (count--) {
		int c = get_byte(pb);
		len = (len << 7) | (c & 0x7f);
		if (!(c & 0x80))
			break;
	}
	return len;
}

int mp4_read_descr(AVFormatContext* fc, ByteIOContext *pb, int *tag)
{
	int len;
	* tag = get_byte(pb);
	len = ff_mp4_read_descr_len(pb);
	return len;
}

#define MP4ESDescrTag                   0x03
#define MP4DecConfigDescrTag            0x04
#define MP4DecSpecificDescrTag          0x05

static const AVCodecTag mp4_audio_types[] = {
	{ CODEC_ID_MP3ON4, AOT_PS   }, /* old mp3on4 draft*/
	{ CODEC_ID_MP3ON4, AOT_L1   }, /* layer 1*/
	{ CODEC_ID_MP3ON4, AOT_L2   }, /* layer 2*/
	{ CODEC_ID_MP3ON4, AOT_L3   }, /* layer 3*/
	{ CODEC_ID_MP4ALS, AOT_ALS  }, /* MPEG-4 ALS*/
	{ CODEC_ID_NONE,   AOT_NULL },
};

int ff_mov_read_esds(AVFormatContext* fc, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;
	int tag, len;

	if (fc->nb_streams < 1)
		return 0;
	st = fc->streams[fc->nb_streams-1];

	get_be32(pb); /* version + flags*/
	len = mp4_read_descr(fc, pb, &tag);
	if (tag == MP4ESDescrTag) {
		get_be16(pb); /* ID*/
		get_byte(pb); /* priority*/
	} else
		get_be16(pb); /* ID*/

	len = mp4_read_descr(fc, pb, &tag);
	if (tag == MP4DecConfigDescrTag) {
		int object_type_id = get_byte(pb);
		get_byte(pb); /* stream type*/
		get_be24(pb); /* buffer size db*/
		get_be32(pb); /* max bitrate*/
		get_be32(pb); /* avg bitrate*/

		st->codec->codec_id= ff_codec_get_id(mp4_obj_type, object_type_id);

		len = mp4_read_descr(fc, pb, &tag);
		if (tag == MP4DecSpecificDescrTag) {
			if((uint64_t)len > (1<<30))
				return -1;
			st->codec->extradata = av_mallocz(len + FF_INPUT_BUFFER_PADDING_SIZE);
			if (!st->codec->extradata) {
				gxloge ("malloc failed!! %s:size:%d\n", __func__, len + FF_INPUT_BUFFER_PADDING_SIZE);
				return AVERROR(ENOMEM);
			}
			get_buffer(pb, st->codec->extradata, len);
			st->codec->extradata_size = len;
			if (st->codec->codec_id == CODEC_ID_AAC) {
				MPEG4AudioConfig cfg;
				mpeg4audio_get_config(&cfg, st->codec->extradata,
						st->codec->extradata_size);
				if (cfg.chan_config > 7)
					return -1;
				st->codec->channels = mpeg4audio_channels[cfg.chan_config];
				if (cfg.object_type == 29 && cfg.sampling_index < 3) // old mp3on4
					st->codec->sample_rate = ff_mpa_freq_tab[cfg.sampling_index];
				else
					st->codec->sample_rate = cfg.sample_rate; // ext sample rate ?
				if (!(st->codec->codec_id = ff_codec_get_id(mp4_audio_types,
								cfg.object_type)))
					st->codec->codec_id = CODEC_ID_AAC;
			}
		}
	}
	return 0;
}

static int mov_read_esds(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	return ff_mov_read_esds(c->fc, pb, atom);
}

static int mov_read_pasp(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	const int num = get_be32(pb);
	const int den = get_be32(pb);
	AVStream* st;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];

	if (den != 0) {
		if ((st->sample_aspect_ratio.den != 1 || st->sample_aspect_ratio.num) && // default
				(den != st->sample_aspect_ratio.den || num != st->sample_aspect_ratio.num))
			gxlogf("sample aspect ratio already set to %d:%d, overriding by 'pasp' atom\n",
					st->sample_aspect_ratio.num, st->sample_aspect_ratio.den);
		st->sample_aspect_ratio.num = num;
		st->sample_aspect_ratio.den = den;
	}
	return 0;
}

/* this atom contains actual media data*/
static int mov_read_mdat(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	if(atom.size == 0) /* wrong one (MP4)*/
		return 0;
	c->found_mdat=1;
	return 0; /* now go for moov*/
}

/* read major brand, minor version and compatible brands and store them as metadata*/
static int mov_read_ftyp(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	uint32_t minor_ver;
	int comp_brand_size;
	char minor_ver_str[11]; /* 32 bit integer -> 10 digits + null*/
	char* comp_brands_str;
	uint8_t type[5] = {0};

	get_buffer(pb, type, 4);
	if (strcmp((const char*)type, "qt  "))
		c->isom = 1;
	gxlogf( "ISO: File Type Major Brand: %.4s\n",(char* )&type);
	av_dict_set(&c->fc->metadata, "major_brand", (const char*)type, 0);
	minor_ver = get_be32(pb); /* minor version*/
	snprintf(minor_ver_str, sizeof(minor_ver_str), "%d", minor_ver);
	av_dict_set(&c->fc->metadata, "minor_version", minor_ver_str, 0);

	comp_brand_size = atom.size - 8;
	if (comp_brand_size < 0)
		return -1;
	comp_brands_str = av_malloc(comp_brand_size + 1); /* Add null terminator*/
	if (!comp_brands_str) {
		gxloge ("malloc failed!! %s:size:%d\n", __func__, comp_brand_size);
		return AVERROR(ENOMEM);
	}
	get_buffer(pb, (unsigned char*)comp_brands_str, comp_brand_size);
	comp_brands_str[comp_brand_size] = 0;
	av_dict_set(&c->fc->metadata, "compatible_brands", comp_brands_str, 0);
	av_freep(&comp_brands_str);

	return 0;
}

/* this atom should contain all header atoms*/
static int mov_read_moov(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	if (mov_read_default(c, pb, atom) < 0)
		return -1;
	/* we parsed the 'moov' atom, we can terminate the parsing as soon as we find the 'mdat'*/
	/* so we don't parse the whole file if over a network*/
	c->found_moov=1;
	return 0; /* now go for mdat*/
}

static int search_frag_moof_offset(MOVFragmentIndex *frag_index, int64_t offset)
{
	int a, b, m;
	int64_t moof_offset;

	// Optimize for appending new entries
	if (!frag_index->nb_items || frag_index->item[frag_index->nb_items - 1].moof_offset < offset)
		return frag_index->nb_items;

	a = -1;
	b = frag_index->nb_items;

	while (b - a > 1) {
		m = (a + b) >> 1;
		moof_offset = frag_index->item[m].moof_offset;
		if (moof_offset >= offset)
			b = m;
		if (moof_offset <= offset)
			a = m;
	}
	return b;
}

static int update_frag_index(MOVContext *c, int64_t offset)
{
	int index, i;
	MOVFragmentIndexItem * item;
	MOVFragmentStreamInfo * frag_stream_info;

	// If moof_offset already exists in frag_index, return index to it
	index = search_frag_moof_offset(&c->frag_index, offset);
	if (index < c->frag_index.nb_items && c->frag_index.item[index].moof_offset == offset)
		return index;

	// offset is not yet in frag index.
	// Insert new item at index (sorted by moof offset)
	item = av_fast_realloc(c->frag_index.item, (unsigned int*)(&c->frag_index.allocated_size),
			((c->frag_index.nb_items + 1)*sizeof(*c->frag_index.item)));
	if(!item)
		return -1;
	c->frag_index.item = item;

	frag_stream_info = av_realloc_array(NULL, c->fc->nb_streams, sizeof(*item->stream_info));
	if (!frag_stream_info)
		return -1;

	for (i = 0; i < c->fc->nb_streams; i++) {
		frag_stream_info[i].id = c->fc->streams[i]->id;
		frag_stream_info[i].sidx_pts = AV_NOPTS_VALUE;
		frag_stream_info[i].tfdt_dts = AV_NOPTS_VALUE;
		frag_stream_info[i].first_tfra_pts = AV_NOPTS_VALUE;
		frag_stream_info[i].index_entry = -1;
		frag_stream_info[i].encryption_index = NULL;
	}

	if (index < c->frag_index.nb_items)
		memmove(c->frag_index.item + index + 1, c->frag_index.item + index, (c->frag_index.nb_items - index) * sizeof(*c->frag_index.item));

	item = &c->frag_index.item[index];
	item->headers_read = 0;
	item->current = 0;
	item->nb_stream_info = c->fc->nb_streams;
	item->moof_offset = offset;
	item->stream_info = frag_stream_info;
	c->frag_index.nb_items++;

	return index;
}

static int mov_read_moof(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	int use_http_persistent_connections = 0;
	GxPlayer_SystemGet(PSYS_NETWORK_HTTP_PERSISTENT_CONNECTIONS, &use_http_persistent_connections);
	c->fragment.moof_offset = url_ftell(pb) - 8;
	if (use_http_persistent_connections && c->is_cenc_decryption && c->frag_index.nb_items > 0){
		int i = 0, j = 0;
		for (i = 0; i < c->frag_index.nb_items; i++) {
			MOVFragmentIndexItem * item = &c->frag_index.item[i];
			if (!item)
				continue;
			MOVFragmentStreamInfo *frag = item->stream_info;
			if (!frag)
				continue;
			for (j = 0; j < item->nb_stream_info; j++) {
				mov_free_encryption_index(&frag[j].encryption_index);
			}
			if (frag) {
				av_free(frag);
				frag = NULL;
			}
			memset (item, 0, sizeof(MOVFragmentIndexItem));
		}
		c->frag_index.nb_items = 0;
	}
	c->frag_index.current = update_frag_index(c, c->fragment.moof_offset);
	return mov_read_default(c, pb, atom);
}

static int mov_read_mdhd(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;
	MOVStreamContext* sc;
	int version;
	char language[4] = {0};
	unsigned lang;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];
	sc = st->priv_data;

	version = get_byte(pb);
	if (version > 1)
		return -1; /* unsupported*/

	get_be24(pb); /* flags*/
	if (version == 1) {
		get_be64(pb);
		get_be64(pb);
	} else {
		get_be32(pb); /* creation time*/
		get_be32(pb); /* modification time*/
	}

	sc->time_scale = get_be32(pb);
	st->duration = (version == 1) ? get_be64(pb) : get_be32(pb); /* duration*/

	lang = get_be16(pb); /* language*/
	if (mov_lang_to_iso639(lang, language)){
		av_dict_set(&st->metadata, "language", language, 0);
		memset(st->language, 0, 4);
		strncpy(st->language, language, 4);
	}
	get_be16(pb); /* quality*/

	return 0;
}

static int mov_read_mvhd(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	int version = get_byte(pb); /* version*/
	get_be24(pb); /* flags*/

	if (version == 1) {
		get_be64(pb);
		get_be64(pb);
	} else {
		get_be32(pb); /* creation time*/
		get_be32(pb); /* modification time*/
	}
	c->time_scale = get_be32(pb); /* time scale*/

	c->duration = (version == 1) ? get_be64(pb) : get_be32(pb); /* duration*/
	get_be32(pb); /* preferred scale*/

	get_be16(pb); /* preferred volume*/

	url_fskip(pb, 10); /* reserved*/

	url_fskip(pb, 36); /* display matrix*/

	get_be32(pb); /* preview time*/
	get_be32(pb); /* preview duration*/
	get_be32(pb); /* poster time*/
	get_be32(pb); /* selection time*/
	get_be32(pb); /* selection duration*/
	get_be32(pb); /* current time*/
	get_be32(pb); /* next track ID*/

	return 0;
}

static int mov_read_smi(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];

	if((uint64_t)atom.size > (1<<30))
		return -1;

	// currently SVQ3 decoder expect full STSD header - so let's fake it
	// this should be fixed and just SMI header should be passed
	av_free(st->codec->extradata);
	st->codec->extradata = av_mallocz(atom.size + 0x5a + FF_INPUT_BUFFER_PADDING_SIZE);
	if (!st->codec->extradata) {
		gxloge ("malloc failed!! %s:size:%d\n", __func__, atom.size + 0x5a + FF_INPUT_BUFFER_PADDING_SIZE);
		return AVERROR(ENOMEM);
	}
	st->codec->extradata_size = 0x5a + atom.size;
	memcpy(st->codec->extradata, "SVQ3", 4); // fake
	get_buffer(pb, st->codec->extradata + 0x5a, atom.size);
	return 0;
}

static int mov_read_enda(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;
	int little_endian;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];

	little_endian = get_be16(pb);
	if (little_endian == 1) {
		switch (st->codec->codec_id) {
		case CODEC_ID_PCM_S24BE:
			st->codec->codec_id = CODEC_ID_PCM_S24LE;
			break;
		case CODEC_ID_PCM_S32BE:
			st->codec->codec_id = CODEC_ID_PCM_S32LE;
			break;
		case CODEC_ID_PCM_F32BE:
			st->codec->codec_id = CODEC_ID_PCM_F32LE;
			break;
		case CODEC_ID_PCM_F64BE:
			st->codec->codec_id = CODEC_ID_PCM_F64LE;
			break;
		default:
			break;
		}
	}
	return 0;
}

/* FIXME modify qdm2/svq3/h264 decoders to take full atom as extradata*/
static int mov_read_extradata(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;
	uint64_t size;
	uint8_t* buf;

	if (c->fc->nb_streams < 1) // will happen with jp2 files
		return 0;
	st= c->fc->streams[c->fc->nb_streams-1];
	size= (uint64_t)st->codec->extradata_size + atom.size + 8 + FF_INPUT_BUFFER_PADDING_SIZE;
	if(size > INT_MAX || (uint64_t)atom.size > INT_MAX)
		return -1;
	buf= av_realloc(st->codec->extradata, size);
	if(!buf)
		return -1;
	st->codec->extradata= buf;
	buf+= st->codec->extradata_size;
	st->codec->extradata_size= size - FF_INPUT_BUFFER_PADDING_SIZE;
	AV_WB32(       buf    , atom.size + 8);
	AV_WL32(       buf + 4, atom.type);
	get_buffer(pb, buf + 8, atom.size);
	return 0;
}

static int mov_read_wave(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];

	if((uint64_t)atom.size > (1<<30))
		return -1;

	if (st->codec->codec_id == CODEC_ID_QDM2) {
		// pass all frma atom to codec, needed at least for QDM2
		av_free(st->codec->extradata);
		st->codec->extradata = av_mallocz(atom.size + FF_INPUT_BUFFER_PADDING_SIZE);
		if (!st->codec->extradata) {
			gxloge ("malloc failed!! %s:size:%d\n", __func__, atom.size + FF_INPUT_BUFFER_PADDING_SIZE);
			return AVERROR(ENOMEM);
		}
		st->codec->extradata_size = atom.size;
		get_buffer(pb, st->codec->extradata, atom.size);
	} else if (atom.size > 8) { /* to read frma, esds atoms*/
		if (mov_read_default(c, pb, atom) < 0)
			return -1;
	} else
		url_fskip(pb, atom.size);
	return 0;
}

/**
 *  This function reads atom content and puts data in extradata without tag
 *  nor size unlike mov_read_extradata.
 */
static int mov_read_glbl(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];

	if((uint64_t)atom.size > (1<<30))
		return -1;

	av_free(st->codec->extradata);
	st->codec->extradata = av_mallocz(atom.size + FF_INPUT_BUFFER_PADDING_SIZE);
	if (!st->codec->extradata) {
		gxloge ("malloc failed!! %s:size:%d\n", __func__, atom.size + FF_INPUT_BUFFER_PADDING_SIZE);
		return AVERROR(ENOMEM);
	}
	st->codec->extradata_size = atom.size;
	get_buffer(pb, st->codec->extradata, atom.size);
	return 0;
}

static int mov_read_stco(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;
	MOVStreamContext* sc;
	unsigned int i, entries;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];
	sc = st->priv_data;

	get_byte(pb); /* version*/
	get_be24(pb); /* flags*/

	entries = get_be32(pb);

	if (atom.type == MKTAG('s','t','c','o'))
		sc->chunk_bytes = 4;
	else if (atom.type == MKTAG('c','o','6','4'))
		sc->chunk_bytes = 8;
	else
		return -1;

	if(entries >=  UINT_MAX/sc->chunk_bytes)
		return -1;

	if (check_index_space(c->fc, entries * sc->chunk_bytes, 0) < 0)
		return -1;

	sc->chunk_offsets = (void*)av_malloc(entries*sc->chunk_bytes);
	if (!sc->chunk_offsets) {
		gxloge ("malloc failed!! %s:entries:%d,size:%d\n", __func__, entries, sc->chunk_bytes);
		return AVERROR(ENOMEM);
	}
	sc->chunk_count = entries;

	if (sc->chunk_bytes == 4) {
		for (i=0; i<entries; i++) {
			unsigned int *chunk_offsets = sc->chunk_offsets;
			chunk_offsets[i] = get_be32(pb);
		}
	} else if (sc->chunk_bytes == 8) {
		for (i=0; i<entries; i++) {
			int64_t *chunk_offsets = sc->chunk_offsets;
			chunk_offsets[i] = get_be64(pb);
		}
	}

	return 0;
}

/**
 *  Compute codec id for 'lpcm' tag.
 *  See CoreAudioTypes and AudioStreamBasicDescription at Apple.
 */
enum CodecID ff_mov_get_lpcm_codec_id(int bps, int flags)
{
	if (flags & 1) { // floating point
		if (flags & 2) { // big endian
			if      (bps == 32) return CODEC_ID_PCM_F32BE;
			else if (bps == 64) return CODEC_ID_PCM_F64BE;
		} else {
			if      (bps == 32) return CODEC_ID_PCM_F32LE;
			else if (bps == 64) return CODEC_ID_PCM_F64LE;
		}
	} else {
		if (flags & 2) {
			if      (bps == 8)
				// signed integer
				if (flags & 4)  return CODEC_ID_PCM_S8;
				else            return CODEC_ID_PCM_U8;
			else if (bps == 16) return CODEC_ID_PCM_S16BE;
			else if (bps == 24) return CODEC_ID_PCM_S24BE;
			else if (bps == 32) return CODEC_ID_PCM_S32BE;
		} else {
			if      (bps == 8)
				if (flags & 4)  return CODEC_ID_PCM_S8;
				else            return CODEC_ID_PCM_U8;
			else if (bps == 16) return CODEC_ID_PCM_S16LE;
			else if (bps == 24) return CODEC_ID_PCM_S24LE;
			else if (bps == 32) return CODEC_ID_PCM_S32LE;
		}
	}
	return CODEC_ID_NONE;
}

static int mov_read_stsd(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;
	MOVStreamContext* sc;
	int j, entries, pseudo_stream_id;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];
	sc = st->priv_data;

	get_byte(pb); /* version*/
	get_be24(pb); /* flags*/

	entries = get_be32(pb);

	for(pseudo_stream_id=0; pseudo_stream_id<entries; pseudo_stream_id++) {
		//Parsing Sample description table
		enum CodecID id;
		int dref_id = 1;
		MOVAtom a = { AV_RL32("stsd") };
		int64_t start_pos = url_ftell(pb);
		int size = get_be32(pb); /* size*/
		uint32_t format = get_le32(pb); /* data format*/

		if (size >= 16) {
			get_be32(pb); /* reserved*/
			get_be16(pb); /* reserved*/
			dref_id = get_be16(pb);
		}

		if (st->codec->codec_tag &&
				st->codec->codec_tag != format &&
				(c->fc->video_codec_id ? ff_codec_get_id(codec_movvideo_tags, format) != c->fc->video_codec_id
				 : st->codec->codec_tag != MKTAG('j','p','e','g'))
		   ){
			/* Multiple fourcc, we skip JPEG. This is not correct, we should
			 *  export it as a separate AVStream but this needs a few changes
			 *  in the MOV demuxer, patch welcome. */
			url_fskip(pb, size - (url_ftell(pb) - start_pos));
			continue;
		}
		sc->pseudo_stream_id = st->codec->codec_tag ? -1 : pseudo_stream_id;
		sc->dref_id= dref_id;

		st->codec->codec_tag = format;
		id = ff_codec_get_id(codec_movaudio_tags, format);
		if (id<=0 && ((format&0xFFFF) == 'm'+('s'<<8) || (format&0xFFFF) == 'T'+('S'<<8)))
			id = ff_codec_get_id(ff_codec_wav_tags, bswap_32(format)&0xFFFF);

		if (st->codec->codec_type != CODEC_TYPE_VIDEO && id > 0) {
			st->codec->codec_type = CODEC_TYPE_AUDIO;
		} else if (st->codec->codec_type != CODEC_TYPE_AUDIO && /* do not overwrite codec type*/
				format && format != MKTAG('m','p','4','s')) { /* skip old asf mpeg4 tag*/
			id = ff_codec_get_id(codec_movvideo_tags, format);
			if (id <= 0)
				id = ff_codec_get_id(ff_codec_bmp_tags, format);
			if (id > 0)
				st->codec->codec_type = CODEC_TYPE_VIDEO;
			else if (st->codec->codec_type == CODEC_TYPE_DATA ||
						(st->codec->codec_type == CODEC_TYPE_SUBTITLE &&
						st->codec->codec_id == CODEC_ID_NONE)) {
				id = ff_codec_get_id(codec_movsubtitle_tags, format);
				if (id <= 0) {
					id = (format == MOV_MP4_TTML_TAG || format == MOV_ISMV_TTML_TAG) ? CODEC_ID_TTML : id;
				}
				if (id > 0)
					st->codec->codec_type = CODEC_TYPE_SUBTITLE;
				else
					id = ff_codec_get_id(ff_codec_movdata_tags, format);
			}
		}

		if(st->codec->codec_type==CODEC_TYPE_VIDEO) {
			uint8_t codec_name[32];
			unsigned int color_depth;
			int color_greyscale;

			st->codec->codec_id = id;
			get_be16(pb); /* version*/
			get_be16(pb); /* revision level*/
			get_be32(pb); /* vendor*/
			get_be32(pb); /* temporal quality*/
			get_be32(pb); /* spatial quality*/

			st->codec->width = get_be16(pb); /* width*/
			st->codec->height = get_be16(pb); /* height*/

			get_be32(pb); /* horiz resolution*/
			get_be32(pb); /* vert resolution*/
			get_be32(pb); /* data size, always 0*/
			get_be16(pb); /* frames per samples*/

			get_buffer(pb, codec_name, 32); /* codec name, pascal string*/
			if (codec_name[0] <= 31) {
				int i;
				int pos = 0;
				for (i = 0; i < codec_name[0] && pos < sizeof(st->codec->codec_name) - 3; i++) {
					uint8_t tmp;
					PUT_UTF8(codec_name[i+1], tmp, st->codec->codec_name[pos++] = tmp;)
				}
				st->codec->codec_name[pos] = 0;
			}

			st->codec->bits_per_coded_sample = get_be16(pb); /* depth*/
			st->codec->color_table_id = get_be16(pb); /* colortable id*/
			/* figure out the palette situation*/
			color_depth = st->codec->bits_per_coded_sample & 0x1F;
			color_greyscale = st->codec->bits_per_coded_sample & 0x20;

			/* if the depth is 2, 4, or 8 bpp, file is palettized*/
			if ((color_depth == 2) || (color_depth == 4) ||
					(color_depth == 8)) {
				/* for palette traversal*/
				unsigned int color_start, color_count, color_end;
				unsigned char r, g, b;

				st->codec->palctrl = av_malloc(sizeof(*st->codec->palctrl));
				if (NULL == st->codec->palctrl) {
					return AVERROR(ENOMEM);
				}
				if (color_greyscale) {
					int color_index, color_dec;
					/* compute the greyscale palette*/
					st->codec->bits_per_coded_sample = color_depth;
					color_count = 1 << color_depth;
					color_index = 255;
					color_dec = 256 / (color_count - 1);
					for (j = 0; j < color_count; j++) {
						r = g = b = color_index;
						st->codec->palctrl->palette[j] =
							(r << 16) | (g << 8) | (b);
						color_index -= color_dec;
						if (color_index < 0)
							color_index = 0;
					}
				} else if (st->codec->color_table_id) {

				} else {
					/* load the palette from the file*/
					color_start = get_be32(pb);
					color_count = get_be16(pb);
					color_end = get_be16(pb);
					if ((color_start <= 255) &&
							(color_end <= 255)) {
						for (j = color_start; j <= color_end; j++) {
							/* each R, G, or B component is 16 bits;
							 *  only use the top 8 bits; skip alpha bytes
							 *  up front */
							get_byte(pb);
							get_byte(pb);
							r = get_byte(pb);
							get_byte(pb);
							g = get_byte(pb);
							get_byte(pb);
							b = get_byte(pb);
							get_byte(pb);
							st->codec->palctrl->palette[j] =
								(r << 16) | (g << 8) | (b);
						}
					}
				}
				st->codec->palctrl->palette_changed = 1;
			}
		} else if(st->codec->codec_type==CODEC_TYPE_AUDIO) {
			int bits_per_sample, flags;
			uint16_t version = get_be16(pb);

			st->codec->codec_id = id;
			get_be16(pb); /* revision level*/
			get_be32(pb); /* vendor*/

			st->codec->channels = get_be16(pb);             /* channel count*/

			st->codec->bits_per_coded_sample = get_be16(pb);      /* sample size*/

			sc->audio_cid = get_be16(pb);
			get_be16(pb); /* packet size = 0*/

			st->codec->sample_rate = ((get_be32(pb) >> 16));

			//Read QT version 1 fields. In version 0 these do not exist.

			if(!c->isom) {
				if(version==1) {
					sc->samples_per_frame = get_be32(pb);
					get_be32(pb); /* bytes per packet*/
					sc->bytes_per_frame = get_be32(pb);
					get_be32(pb); /* bytes per sample*/
				} else if(version==2) {
					get_be32(pb); /* sizeof struct only*/
					st->codec->sample_rate = av_int2dbl(get_be64(pb)); /* float 64*/
					st->codec->channels = get_be32(pb);
					get_be32(pb); /* always 0x7F000000*/
					st->codec->bits_per_coded_sample = get_be32(pb); /* bits per channel if sound is uncompressed*/
					flags = get_be32(pb); /* lcpm format specific flag*/
					sc->bytes_per_frame = get_be32(pb); /* bytes per audio packet if constant*/
					sc->samples_per_frame = get_be32(pb); /* lpcm frames per audio packet if constant*/
					if (format == MKTAG('l','p','c','m'))
						st->codec->codec_id = ff_mov_get_lpcm_codec_id(st->codec->bits_per_coded_sample, flags);
				}
			}

			switch (st->codec->codec_id) {
			case CODEC_ID_PCM_S8:
			case CODEC_ID_PCM_U8:
				if (st->codec->bits_per_coded_sample == 16)
					st->codec->codec_id = CODEC_ID_PCM_S16BE;
				break;
			case CODEC_ID_PCM_S16LE:
			case CODEC_ID_PCM_S16BE:
				if (st->codec->bits_per_coded_sample == 8)
					st->codec->codec_id = CODEC_ID_PCM_S8;
				else if (st->codec->bits_per_coded_sample == 24)
					st->codec->codec_id =
						st->codec->codec_id == CODEC_ID_PCM_S16BE ?
						CODEC_ID_PCM_S24BE : CODEC_ID_PCM_S24LE;
				break;
				/* set values for old format before stsd version 1 appeared*/
			case CODEC_ID_MACE3:
				sc->samples_per_frame = 6;
				sc->bytes_per_frame = 2*st->codec->channels;
				break;
			case CODEC_ID_MACE6:
				sc->samples_per_frame = 6;
				sc->bytes_per_frame = 1*st->codec->channels;
				break;
			case CODEC_ID_GSM:
				sc->samples_per_frame = 160;
				sc->bytes_per_frame = 33;
				break;
			default:
				break;
			}

			bits_per_sample = av_get_bits_per_sample(st->codec->codec_id);
			if (bits_per_sample) {
				st->codec->bits_per_coded_sample = bits_per_sample;
				sc->sample_size = (bits_per_sample >> 3)*  st->codec->channels;
			}
		} else if(st->codec->codec_type==CODEC_TYPE_SUBTITLE){
			// ttxt stsd contains display flags, justification, background
			// color, fonts, and default styles, so fake an atom to read it
			MOVAtom fake_atom = { .size = size - (url_ftell(pb) - start_pos) };
			if (format != AV_RL32("mp4s")) // mp4s contains a regular esds atom
				mov_read_glbl(c, pb, fake_atom);
			st->codec->codec_id= id;
			st->codec->width = sc->width;
			st->codec->height = sc->height;
		} else {
			/* other codec type, just skip (rtp, mp4s, tmcd ...)*/
			url_fskip(pb, size - (url_ftell(pb) - start_pos));
		}
		/* this will read extra atoms at the end (wave, alac, damr, avcC, SMI ...)*/
		a.size = size - (url_ftell(pb) - start_pos);
		if (a.size > 8) {
			if (mov_read_default(c, pb, a) < 0)
				return -1;
		} else if (a.size > 0)
			url_fskip(pb, a.size);
	}

	if(st->codec->codec_type==CODEC_TYPE_AUDIO && st->codec->sample_rate==0 && sc->time_scale>1)
		st->codec->sample_rate= sc->time_scale;

	/* special codec parameters handling*/
	switch (st->codec->codec_id) {
	case CODEC_ID_QCELP:
		// force sample rate for qcelp when not stored in mov
		if (st->codec->codec_tag != MKTAG('Q','c','l','p'))
			st->codec->sample_rate = 8000;
		st->codec->frame_size= 160;
		st->codec->channels= 1; /* really needed*/
		break;
	case CODEC_ID_AMR_NB:
	case CODEC_ID_AMR_WB:
		st->codec->frame_size= sc->samples_per_frame;
		st->codec->channels= 1; /* really needed*/
		/* force sample rate for amr, stsd in 3gp does not store sample rate*/
		if (st->codec->codec_id == CODEC_ID_AMR_NB)
			st->codec->sample_rate = 8000;
		else if (st->codec->codec_id == CODEC_ID_AMR_WB)
			st->codec->sample_rate = 16000;
		break;
	case CODEC_ID_MP2:
	case CODEC_ID_MP3:
		st->codec->codec_type = CODEC_TYPE_AUDIO; /* force type after stsd for m1a hdlr*/
		st->need_parsing = AVSTREAM_PARSE_FULL;
		break;
	case CODEC_ID_GSM:
	case CODEC_ID_ADPCM_MS:
	case CODEC_ID_ADPCM_IMA_WAV:
		st->codec->block_align = sc->bytes_per_frame;
		break;
	case CODEC_ID_ALAC:
		if (st->codec->extradata_size == 36) {
			st->codec->frame_size = AV_RB32(st->codec->extradata+12);
			st->codec->channels   = AV_RB8 (st->codec->extradata+21);
		}
		break;
	default:
		break;
	}

	return 0;
}

static int mov_read_stsc(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;
	MOVStreamContext* sc;
	unsigned int i, entries;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];
	sc = st->priv_data;

	get_byte(pb); /* version*/
	get_be24(pb); /* flags*/

	entries = get_be32(pb);

	if(entries >= UINT_MAX / sizeof(*sc->stsc_data))
		return -1;

	if (check_index_space(c->fc, entries * sizeof(*sc->stsc_data), 0) < 0)
		return -1;

	sc->stsc_data = av_malloc(entries*sizeof(*sc->stsc_data));
	if (!sc->stsc_data) {
		gxloge ("malloc failed!! %s:entries:%d,size:%d\n", __func__, entries, entries*sizeof(*sc->stsc_data));
		return AVERROR(ENOMEM);
	}
	sc->stsc_count = entries;

	for(i=0; i<entries; i++) {
		sc->stsc_data[i].first = get_be32(pb);
		sc->stsc_data[i].count = get_be32(pb);
		sc->stsc_data[i].id = get_be32(pb);
	}
	return 0;
}

static int mov_read_stps(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;
	MOVStreamContext* sc;
	unsigned i, entries;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];
	sc = st->priv_data;

	get_be32(pb); // version + flags

	entries = get_be32(pb);
	if (entries >= UINT_MAX / sizeof(*sc->stps_data))
		return -1;

	if (check_index_space(c->fc, entries * sizeof(*sc->stps_data), 0) < 0)
		return -1;

	sc->stps_data = av_malloc(entries*sizeof(*sc->stps_data));
	if (!sc->stps_data) {
		gxloge ("malloc failed!! %s:entries:%d,size:%d\n", __func__, entries, entries*sizeof(*sc->stps_data));
		return AVERROR(ENOMEM);
	}
	sc->stps_count = entries;

	for (i = 0; i < entries; i++) {
		sc->stps_data[i] = get_be32(pb);
		//dgxlogd(c->fc, "stps %d\n", sc->stps_data[i]);
	}

	return 0;
}

static int mov_read_stss(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;
	MOVStreamContext* sc;
	unsigned int i, entries;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];
	sc = st->priv_data;

	get_byte(pb); /* version*/
	get_be24(pb); /* flags*/

	entries = get_be32(pb);

	if(entries >= UINT_MAX / sizeof(int))
		return -1;

	if (check_index_space(c->fc, entries * sizeof(int), 0) < 0)
		return -1;

	sc->keyframes = av_malloc(entries*sizeof(int));
	if (!sc->keyframes) {
		gxloge ("malloc failed!! %s:size:%d\n", __func__, entries * sizeof(int));
		return AVERROR(ENOMEM);
	}
	sc->keyframe_count = entries;

	for(i=0; i<entries; i++) {
		sc->keyframes[i] = get_be32(pb);
	}
	return 0;
}

static int get_stsz_by_index(MOVStreamContext* sc, int index)
{
	if (sc->sample_bytes == 1) {
		unsigned char* tmp_sample_sizes = (unsigned char *)sc->sample_sizes;
		return (int)tmp_sample_sizes[index];
	} else if(sc->sample_bytes == 2) {
		unsigned short* tmp_sample_sizes = (unsigned short *)sc->sample_sizes;
		return (int)tmp_sample_sizes[index];
	} else if (sc->sample_bytes == 3) {
		unsigned char* tmp_sample_sizes = (unsigned char *)sc->sample_sizes;
		int tmp_sample_size = 0;
		tmp_sample_size |= (tmp_sample_sizes[3 * index + 0] << 16);
		tmp_sample_size |= (tmp_sample_sizes[3 * index + 1] <<  8);
		tmp_sample_size |= (tmp_sample_sizes[3 * index + 2]);
		return tmp_sample_size;
	} else if(sc->sample_bytes == 4) {
		int* tmp_sample_sizes = (int *)sc->sample_sizes;
		return tmp_sample_sizes[index];
	}

	return 0;
}


static int mov_stsz_bytes(ByteIOContext *pb,
		unsigned char *buf,
		unsigned int every_entries,
		unsigned int every_num_bytes,
		unsigned int num_count,
		unsigned int num_bytes,
		unsigned int entries,
		unsigned int field_size)
{
	GetBitContext gb;
	int bytes = 0;
	int large_mem_flg = is_system_large_memory();

	if (!large_mem_flg && entries > MOV_CTTS_OPTIMIZE) {
		int64_t start_pos = url_ftell(pb);
		int bits = 8;
		int i = 0, j = 0;

		for (j = 0; j <= num_count; j++) {
			unsigned int every_entries1 = every_entries;

			if (j == num_count) {
				every_num_bytes = (num_bytes - num_count * every_num_bytes);
				every_entries1  = (entries - num_count * every_entries);
			}

			if (get_buffer(pb, buf, every_num_bytes) < every_num_bytes) {
				url_fseek(pb, start_pos, SEEK_SET);
				return sizeof(int);
			}

			init_get_bits(&gb, buf, 8*every_num_bytes);

			for (i=0; i < every_entries1; i++) {
				int sample_size = get_bits_long(&gb, field_size);

				if ((sample_size >> 24) & 0xff) {
					bits = 32;
					goto exit_stsz;
				} else if ((bits == 8) || (bits == 16)) {
					if ((sample_size >> 16) & 0xffff) {
						bits = 24;
					} else if ((sample_size >> 8) & 0xffffff) {
						bits = (bits > 16) ? bits : 16;
					}
				}
			}
		}

exit_stsz:
		if (bits == 32)
			bytes = 4;
		else if (bits == 24)
			bytes = 3;
		else if (bits == 16)
			bytes = 2;
		else
			bytes = 1;
		gxlogi("-------> %s: %d %d (%d)\n", __func__, bytes, bits, entries);
		url_fseek(pb, start_pos, SEEK_SET);
	} else
		bytes = sizeof(int);

	return bytes;
}

static int mov_read_stsz(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;
	MOVStreamContext* sc;
	unsigned int i, entries, sample_size, field_size, num_bytes;
	GetBitContext gb;
	unsigned char* buf;
	unsigned int j, every_num_bytes, every_entries, num_count;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];
	sc = st->priv_data;

	get_byte(pb); /* version*/
	get_be24(pb); /* flags*/

	if (atom.type == MKTAG('s','t','s','z')) {
		sample_size = get_be32(pb);
		if (!sc->sample_size) /* do not overwrite value computed in stsd*/
			sc->sample_size = sample_size;
		field_size = 32;
	} else {
		sample_size = 0;
		get_be24(pb); /* reserved*/
		field_size = get_byte(pb);
	}
	entries = get_be32(pb);

	sc->sample_count = entries;
	if (sample_size)
		return 0;

	if (field_size != 4 && field_size != 8 && field_size != 16 && field_size != 32) {
		gxlogd( "Invalid sample field size %d\n", field_size);
		return -1;
	}

	if (entries >= UINT_MAX / sizeof(int) || entries >= (UINT_MAX - 4) / field_size)
		return -1;

	if (check_index_space(c->fc, entries * sizeof(int), 0) < 0)
		return -1;

	num_bytes = (entries*field_size+4)>>3;

	every_num_bytes = (field_size*1024);
	num_count       = num_bytes/every_num_bytes;
	if(num_count == 0)
		every_entries = entries;
	else
		every_entries = (1024<<3);

	buf = av_malloc(every_num_bytes+FF_INPUT_BUFFER_PADDING_SIZE);
	if (!buf) {
		check_index_space(c->fc, entries * sizeof(int), 1);
		gxloge ("malloc failed!! %s:size:%d\n", __func__, every_num_bytes+FF_INPUT_BUFFER_PADDING_SIZE);
		return AVERROR(ENOMEM);
	}

	sc->sample_bytes = mov_stsz_bytes(pb, buf,
			every_entries, every_num_bytes, num_count, num_bytes, entries, field_size);

	sc->sample_sizes = av_malloc(entries * sc->sample_bytes);
	if (!sc->sample_sizes) {
		check_index_space(c->fc, entries * sizeof(int), 1);
		av_free(buf);
		return AVERROR(ENOMEM);
	}

	for(j = 0; j <= num_count; j++) {
		unsigned int every_entries1 = every_entries;

		if(j == num_count){
			every_num_bytes = (num_bytes - num_count * every_num_bytes);
			every_entries1  = (entries - num_count * every_entries);
		}

		if (get_buffer(pb, buf, every_num_bytes) < every_num_bytes) {
			av_freep((void**)&sc->sample_sizes);
			av_free(buf);
			check_index_space(c->fc, entries * sizeof(int), 1);
			return -1;
		}

		init_get_bits(&gb, buf, 8*every_num_bytes);

		for (i=0; i < every_entries1; i++) {
			int sample_size = get_bits_long(&gb, field_size);

			if (sc->sample_bytes == 1) {
				unsigned char *tmp_sample_sizes = (unsigned char *)sc->sample_sizes;
				tmp_sample_sizes[i + j * every_entries] = sample_size;
			} else if (sc->sample_bytes == 2) {
				unsigned short *tmp_sample_sizes = (unsigned short *)sc->sample_sizes;
				tmp_sample_sizes[i + j * every_entries] = sample_size;
			} else if (sc->sample_bytes == 3) {
				unsigned char *tmp_sample_sizes = (unsigned char *)sc->sample_sizes;
				tmp_sample_sizes[3 * (i + j * every_entries) + 0] = (sample_size >> 16) & 0xff;
				tmp_sample_sizes[3 * (i + j * every_entries) + 1] = (sample_size >>  8) & 0xff;
				tmp_sample_sizes[3 * (i + j * every_entries) + 2] = (sample_size) & 0xff;
			} else if (sc->sample_bytes == 4) {
				int *tmp_sample_sizes = (int *)sc->sample_sizes;
				tmp_sample_sizes[i + j * every_entries] = sample_size;
			}
		}
	}

	av_free(buf);
	return 0;
}

static int mov_stts_bytes(ByteIOContext *pb, unsigned int entries)
{
	int bytes = 0;

	if (entries > MOV_STTS_OPTIMIZE) {
		int64_t start_pos = url_ftell(pb);
		int bits = 8;
		int i = 0;

		for(i = 0; i < entries; i++) {
			int count    = get_be32(pb);
			int duration = get_be32(pb);

			if (((count >> 16) & 0xffff) || ((duration >> 16) & 0xffff)) {
				bits = 32;
				break;
			} else if (bits == 8) {
				if (((count >> 8) & 0xffffff) || ((duration >> 8) & 0xffffff)) {
					bits = 16;
				}
			}

		}
		if (bits == 32)
			bytes = sizeof(MOVStts_32);
		else if (bits == 16)
			bytes = sizeof(MOVStts_16);
		else
			bytes = sizeof(MOVStts_8);
		gxlogi("-------> %s: %d %d\n", __func__, bytes, bits);
		url_fseek(pb, start_pos, SEEK_SET);
	} else
		bytes = sizeof(MOVStts_32);

	return bytes;
}

static int get_stts_by_index(MOVStreamContext* sc, int index, MOVStts_32 *stts)
{
	if (sc->stts_bytes == sizeof(MOVStts_32)) {
		MOVStts_32 *tmp_stts = (MOVStts_32 *)sc->stts_data;

		stts->count    = tmp_stts[index].count;
		stts->duration = tmp_stts[index].duration;
	} else if (sc->stts_bytes == sizeof(MOVStts_16)) {
		MOVStts_16 *tmp_stts = (MOVStts_16 *)sc->stts_data;

		stts->count    = (unsigned int)tmp_stts[index].count;
		stts->duration = (unsigned int)tmp_stts[index].duration;
	} else if (sc->stts_bytes == sizeof(MOVStts_8)) {
		MOVStts_8 *tmp_stts = (MOVStts_8 *)sc->stts_data;

		stts->count    = (unsigned int)tmp_stts[index].count;
		stts->duration = (unsigned int)tmp_stts[index].duration;
	}

	return 0;
}

static int mov_read_stts(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;
	MOVStreamContext* sc;
	unsigned int i, entries;
	int64_t duration=0;
	int64_t total_sample_count=0;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];
	sc = st->priv_data;

	get_byte(pb); /* version*/
	get_be24(pb); /* flags*/
	entries = get_be32(pb);

	sc->stts_bytes = mov_stts_bytes(pb, entries);

	if(entries >= UINT_MAX / sc->stts_bytes)
		return -1;

	if (check_index_space(c->fc, entries*sc->stts_bytes, 0) < 0)
		return -1;

	sc->stts_data = av_malloc(entries*sc->stts_bytes);
	if (!sc->stts_data) {
		gxloge ("malloc failed!! %s:entries:%d,size:%d\n", __func__, entries, entries*sc->stts_bytes);
		return AVERROR(ENOMEM);
	}
	sc->stts_count = entries;

	for (i = 0; i < entries; i++) {
		int sample_duration;
		int sample_count;

		sample_count     = get_be32(pb);
		sample_duration  = get_be32(pb);
		if (sc->stts_bytes == sizeof(MOVStts_32)) {
			MOVStts_32 *stts = (MOVStts_32 *)sc->stts_data;

			stts[i].count    = sample_count;
			stts[i].duration = sample_duration;
		} else if (sc->stts_bytes == sizeof(MOVStts_16)) {
			MOVStts_16 *stts = (MOVStts_16 *)sc->stts_data;

			stts[i].count    = (unsigned short)sample_count;
			stts[i].duration = (unsigned short)sample_duration;
		} else if (sc->stts_bytes == sizeof(MOVStts_8)) {
			MOVStts_8 *stts = (MOVStts_8 *)sc->stts_data;

			stts[i].count    = (unsigned char)sample_count;
			stts[i].duration = (unsigned char)sample_duration;
		}
		duration += (int64_t)sample_duration*sample_count;
		total_sample_count += sample_count;
	}

	st->nb_frames= total_sample_count;
	if(duration)
		st->duration= duration;
	return 0;
}

static int mov_read_cslg(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;
	MOVStreamContext* sc;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];
	sc = st->priv_data;

	get_be32(pb); // version + flags

	sc->dts_shift = get_be32(pb);

	get_be32(pb); // least dts to pts delta
	get_be32(pb); // greatest dts to pts delta
	get_be32(pb); // pts start
	get_be32(pb); // pts end

	return 0;
}

static int mov_ctts_bytes(ByteIOContext *pb, unsigned int entries)
{
	int bytes = 0;
	int large_mem_flg = is_system_large_memory();

	if (!large_mem_flg && entries > MOV_CTTS_OPTIMIZE) {
		int64_t start_pos = url_ftell(pb);
		int bits = 8;
		int i = 0;

		for(i = 0; i < entries; i++) {
			int count    = get_be32(pb);
			int duration = get_be32(pb);

			if (((count >> 16) & 0xffff) || ((duration >> 16) & 0xffff)) {
				bits = 32;
				break;
			} else if (bits == 8) {
				if (((count >> 8) & 0xffffff) || ((duration >> 8) & 0xffffff)) {
					bits = 16;
				}
			}

		}
		if (bits == 32)
			bytes = sizeof(MOVCtts_32);
		else if (bits == 16)
			bytes = sizeof(MOVCtts_16);
		else
			bytes = sizeof(MOVCtts_8);
		gxlogi("-------> %s: %d %d\n", __func__, bytes, bits);
		url_fseek(pb, start_pos, SEEK_SET);
	} else
		bytes = sizeof(MOVCtts_32);

	return bytes;
}

static int get_ctts_by_index(MOVStreamContext* sc, int index, MOVCtts_32 *ctts)
{
	if (sc->ctts_bytes == sizeof(MOVCtts_32)) {
		MOVCtts_32 *tmp_ctts = (MOVCtts_32 *)sc->ctts_data;
		if (tmp_ctts) {
			ctts->count    = tmp_ctts[index].count;
			ctts->duration = tmp_ctts[index].duration;
		}
	} else if (sc->ctts_bytes == sizeof(MOVCtts_16)) {
		MOVCtts_16 *tmp_ctts = (MOVCtts_16 *)sc->ctts_data;
		if (tmp_ctts) {
			ctts->count    = (unsigned int)tmp_ctts[index].count;
			ctts->duration = (int)tmp_ctts[index].duration;
		}
	} else if (sc->ctts_bytes == sizeof(MOVCtts_8)) {
		MOVCtts_8 *tmp_ctts = (MOVCtts_8 *)sc->ctts_data;
		if (tmp_ctts) {
			ctts->count    = (unsigned int)tmp_ctts[index].count;
			ctts->duration = (int)tmp_ctts[index].duration;
		}
	}

	return 0;
}

static int mov_read_ctts(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;
	MOVStreamContext* sc;
	unsigned int i, entries;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];
	sc = st->priv_data;

	get_byte(pb); /* version*/
	get_be24(pb); /* flags*/
	entries = get_be32(pb);

	sc->ctts_bytes = mov_ctts_bytes(pb, entries);
	if (!entries)
		return 0;

	if(entries >= UINT_MAX / sc->ctts_bytes)
		return -1;

	if (check_index_space(c->fc, entries*sc->ctts_bytes, 0) < 0)
		return -1;

	sc->ctts_data = av_mallocz(entries*sc->ctts_bytes);
	if (!sc->ctts_data) {
		gxloge ("malloc failed!! %s:entries:%d,size:%d\n", __func__, entries, entries*sc->ctts_bytes);
		return AVERROR(ENOMEM);
	}
	sc->ctts_count = entries;

	for (i = 0; i < entries; i++) {
		if (sc->ctts_bytes == sizeof(MOVCtts_32)) {
			MOVCtts_32 *ctts = (MOVCtts_32 *)sc->ctts_data;

			ctts[i].count    = get_be32(pb);
			ctts[i].duration = get_be32(pb);
		} else if (sc->ctts_bytes == sizeof(MOVCtts_16)) {
			MOVCtts_16 *ctts = (MOVCtts_16 *)sc->ctts_data;

			ctts[i].count    = (unsigned short)get_be32(pb);
			ctts[i].duration = (unsigned short)get_be32(pb);
		} else if (sc->ctts_bytes == sizeof(MOVCtts_8)) {
			MOVCtts_8 *ctts = (MOVCtts_8 *)sc->ctts_data;

			ctts[i].count    = (unsigned char)get_be32(pb);
			ctts[i].duration = (unsigned char)get_be32(pb);
		}
	}
	return 0;
}

static int mov_build_index(MOVContext* mov, AVStream *st,int nIndexStart)
{
	int ret = 0;
	MOVStreamContext* sc = st->priv_data;
	int64_t current_offset;
	int64_t current_dts = 0;
	unsigned int stts_index = 0;
	unsigned int stsc_index = 0;
	unsigned int stss_index = 0;
	unsigned int stps_index = 0;
	unsigned int i, j;
	int nPlusOnly=0;
	int nIndexCount=0;
	MOVStts_32 stts_0 = {0, 0};

	if(mov){
		st->time_scale_mov=mov->time_scale;
	}

	st->index_entries_start=nIndexStart;
	st->index_entries_end=nIndexStart;
	get_stts_by_index(sc, 0, &stts_0);

	if (sc->elst_count) {
		int i, edit_start_index = 0;
		int64_t empty_duration = 0; // empty duration of the first edit list entry
		int64_t start_time = 0; // start time of the media

		for (i = 0; i < sc->elst_count; i++) {
			const MOVElst *e = &sc->elst_data[i];
			if (i == 0 && e->time == -1) {
				/* if empty, the first entry is the start time of the stream
				 * relative to the presentation itself */
				empty_duration = e->duration;
				edit_start_index = 1;
			} else if (i == edit_start_index && e->time >= 0) {
				start_time = e->time;
			}
		}

		/* adjust first dts according to edit list */
		if ((empty_duration || start_time) && mov && mov->time_scale > 0) {
			if (empty_duration)
				empty_duration = av_rescale(empty_duration, sc->time_scale, mov->time_scale);
			sc->time_offset = start_time -	(uint64_t)empty_duration;
		}
	}

	/* only use old uncompressed audio chunk demuxing when stts specifies it*/
	if (!(st->codec->codec_type == CODEC_TYPE_AUDIO &&
				sc->stts_count == 1 && stts_0.duration == 1)) {
		unsigned int current_sample = 0;
		unsigned int stts_sample = 0;
		unsigned int sample_size;
		unsigned int distance = 0;
		int key_off = (sc->keyframe_count && sc->keyframes[0] > 0) || (sc->stps_count && sc->stps_data[0] > 0);

		current_dts -= sc->dts_shift;

		st->nb_frames = sc->sample_count;
		for (i = 0; i < sc->chunk_count; i++) {
			MOVStts_32 stts = {0, 0};

			current_offset = get_chunk_offset_by_index(sc, i);
			if (stsc_index + 1 < sc->stsc_count &&
					i + 1 == sc->stsc_data[stsc_index + 1].first)
				stsc_index++;
			for (j = 0; j < sc->stsc_data[stsc_index].count; j++) {
				int keyframe = 0;
				if (current_sample >= sc->sample_count) {
					gxlogd( "wrong sample count\n");
					st->index_entries_start=0;
					return -1;
				}

				if (!sc->keyframe_count || current_sample+key_off == sc->keyframes[stss_index]) {
					keyframe = 1;
					if (stss_index + 1 < sc->keyframe_count)
						stss_index++;
				} else if (sc->stps_count && current_sample+key_off == sc->stps_data[stps_index]) {
					keyframe = 1;
					if (stps_index + 1 < sc->stps_count)
						stps_index++;
				}
				if (keyframe)
					distance = 0;
				sample_size = sc->sample_size > 0 ? sc->sample_size : get_stsz_by_index(sc, current_sample);
				if(sc->pseudo_stream_id == -1 ||
						sc->stsc_data[stsc_index].id - 1 == sc->pseudo_stream_id) {
					if(nIndexCount!=nIndexStart){
						nIndexCount++;
						ret = mov_add_index_entry(st, current_offset, current_dts, sample_size, distance,
								keyframe ? AVINDEX_KEYFRAME : 0, 0, 1);
					}else{
						ret = mov_add_index_entry(st, current_offset, current_dts, sample_size, distance,
								keyframe ? AVINDEX_KEYFRAME : 0, 0, nPlusOnly);
					}
					if(ret < 0){
						if(ret==-2)
							nPlusOnly=1;//realoc is error
						else{
							st->index_entries_start=0;
							st->index_entries_end=0;
							return -1;
						}
					}
				}

				get_stts_by_index(sc, stts_index, &stts);
				current_offset += sample_size;
				current_dts += stts.duration;
				distance++;
				stts_sample++;
				current_sample++;
				if (stts_index + 1 < sc->stts_count && stts_sample == stts.count) {
					stts_sample = 0;
					stts_index++;
				}
			}
		}
	} else {
		for (i = 0; i < sc->chunk_count; i++) {
			unsigned chunk_samples;

			current_offset = get_chunk_offset_by_index(sc, i);
			if (stsc_index + 1 < sc->stsc_count &&
					i + 1 == sc->stsc_data[stsc_index + 1].first)
				stsc_index++;
			chunk_samples = sc->stsc_data[stsc_index].count;

			if (sc->samples_per_frame && chunk_samples % sc->samples_per_frame) {
				gxlogd( "error unaligned chunk\n");
				st->index_entries_start=0;
				st->index_entries_end=0;
				return -1;
			}

			while (chunk_samples > 0) {
				unsigned size, samples;

				if (sc->samples_per_frame >= 160) { // gsm
					samples = sc->samples_per_frame;
					size = sc->bytes_per_frame;
				} else {
					if (sc->samples_per_frame > 1) {
						samples = GXMIN((1024 / sc->samples_per_frame)*
								sc->samples_per_frame, chunk_samples);
						size = (samples / sc->samples_per_frame)*  sc->bytes_per_frame;
					} else {
						samples = GXMIN(1024, chunk_samples);
						size = samples*  sc->sample_size;
					}
				}

				if(nIndexCount!=nIndexStart){
					nIndexCount++;
					ret = mov_add_index_entry(st, current_offset, current_dts, size, 0, AVINDEX_KEYFRAME, 0, 1);
				}else{
					ret = mov_add_index_entry(st, current_offset, current_dts, size, 0, AVINDEX_KEYFRAME, 0, nPlusOnly);
				}
				if(ret < 0){
					if(ret==-2)
						nPlusOnly=1;//realoc is error
					else{
						st->index_entries_start=0;
						st->index_entries_end=0;
						return -1;
					}
				}

				current_offset += size;
				current_dts += samples;
				chunk_samples -= samples;
			}
		}
	}
	return 0;
}

AVIndexEntry* av_find_sample_from_mov(AVStream *st,AVIndexEntry *index_entries,int nIndex)
{
	AVIndexEntry* current_sample=NULL;
	if(NULL==st || NULL==index_entries || nIndex<0 ||st->nb_index_entries<nIndex){
		gxlogd("####mov opt para is error####\n");
		return NULL;
	}
	if(nIndex<st->index_entries_end && nIndex>=st->index_entries_start){
		current_sample = &st->index_entries[nIndex-st->index_entries_start];
	}else{//index is not in the scope==>realloc is null,we should build index again
		/*  reset some value */
		st->index_entries_start=0;
		st->index_entries_end=0;
		st->nb_index_entries=0;
		st->index_entries_allocated_size=0;
		av_freep(&st->index_entries);
		if(mov_build_index(NULL,st,nIndex)<0){
			gxlogd("####[mov opt]%s==>%d####\n",__FILE__,__LINE__);
			return NULL;
		}
		if(nIndex<st->index_entries_end && nIndex>=st->index_entries_start && st->index_entries){
			current_sample = &st->index_entries[0];
		}else{
			gxlogd("####[mov opt]%s==>%d####\n",__FILE__,__LINE__);
			return NULL;
		}
	}
	return current_sample;
}

static int _mov_build_index(AVStream* st, int nb_entries, MovIndex** arrays, int* arr_num)
{
	int i, num;
	int index_count = ((st->codec->codec_type == CODEC_TYPE_SUBTITLE) ? MOV_SUB_INDEX_COUNT : MOV_INDEX_COUNT);

	num = nb_entries % index_count ? (nb_entries / index_count + 1) : (nb_entries / index_count);
	*arrays = (MovIndex *)av_malloc(num * sizeof(MovIndex));
	if(!(*arrays) || num < 1) {
		gxlogd("#########oom###########\n");
		return -1;
	}
	*arr_num = num;

	for (i = 0; i < num; i++) {
		(*arrays)[i].start = index_count * i;
		(*arrays)[i].end   = index_count * i + index_count;
	}
	(*arrays)[num - 1].end = nb_entries;

	return 0;
}

static int _mov_find_index(MovIndex *arrays, int arr_num, int start)
{
	int i;

	for(i = arr_num - 1; i >= 0; i--){
		if (arrays[i].start <= start)
			return i;
	}

	return -1;
}

int ff_index_search_timestamp_mov(AVStream* st,
		int64_t wanted_timestamp, int flags, int ass_is_forward)
{
#define MOV_CLEAR() \
	do{ \
		st->index_entries_start=0; \
		st->index_entries_end=0;   \
		st->nb_index_entries=0;    \
		st->index_entries_allocated_size=0;  \
		av_freep(&st->index_entries);  \
	}while(0)

	int a, b, m;
	int low, high;
	int64_t timestamp;
	int nb_entries, end, start = 0;
	int arr_num;
	int index;
	AVIndexEntry *entries;
	MovIndex *arrays = NULL;
	int bwic = ((st->codec->codec_type == CODEC_TYPE_SUBTITLE) ? BACKWARD_SUB_INDEX_COUNT : BACKWARD_INDEX_COUNT);

#define MOV_SET() \
	do{ \
		start = st->index_entries_start;   \
		end = st->index_entries_end;   \
		entries = st->index_entries;   \
	}while(0)

	nb_entries = st->nb_index_entries;
	MOV_SET();
	if (!entries)
		return -1;

	if ((ass_is_forward || (start == 0)) && end == nb_entries)
		return ff_index_search_timestamp(st, st->index_entries,st->nb_index_entries,wanted_timestamp,flags, st->seek_tolerance, ass_is_forward);

	if(_mov_build_index(st, nb_entries, &arrays, &arr_num) < 0)
		return -1;

	low = -1;
	high = arr_num;

	while (high - low > 1) {

		if (entries[end - start - 1].timestamp >= wanted_timestamp &&
				entries[0].timestamp <= wanted_timestamp) {

			goto binarysearch;
		} else {

			if (entries[end - start - 1].timestamp <= wanted_timestamp) {
				low = _mov_find_index(arrays, arr_num, start);
			}
			if (entries[0].timestamp >= wanted_timestamp) {
				high = _mov_find_index(arrays, arr_num, start);
			}
			index = (low + high) >> 1;
			if (index < 0)
				index = 0;
			//gxlogd("^^^^^^^^^^^^^^[STREAM:%2d]index =%4d, low =%4d, high =%4d, [start:%5d] =%10lld, [end:%5d] =%10lld, want =%10lld\n",st->index, index, low, high, start,entries[0].timestamp, end-1,entries[end - start - 1].timestamp, wanted_timestamp);

			MOV_CLEAR();
			if (mov_build_index(NULL, st, arrays[index].start) < 0) {
				gxlogd("####[mov opt]%s==>%d####\n", __FILE__, __LINE__);
				av_free(arrays);
				return -1;
			}
			MOV_SET();
		}
	}

	if (high - low <= 1 && arr_num > 1) {
		/*
		 * not found, high -lown <= 1
		 * Possible boundary between the array
		 */
		int index_count = ((st->codec->codec_type == CODEC_TYPE_SUBTITLE) ? MOV_SUB_INDEX_COUNT : MOV_INDEX_COUNT);

		index = _mov_find_index(arrays, arr_num, start);
		if ((((end - start) >= 1) && entries[end - start - 1].timestamp < wanted_timestamp) &&
				(index + 1 < arr_num)) {
			MOV_CLEAR();
			if (mov_build_index(NULL, st, arrays[index].end - 1 - (index_count / 2)) < 0) {
				gxlogd("####[mov opt]%s==>%d####\n", __FILE__, __LINE__);
				av_free(arrays);
				return -1;
			}
			MOV_SET();
			goto binarysearch;
		}

		if ((entries[0].timestamp > wanted_timestamp) &&
				(index > 0)) {
			MOV_CLEAR();
			if (mov_build_index(NULL, st, arrays[index - 1].end - 1 - (index_count / 2)) < 0) {
				gxlogd("####[mov opt]%s==>%d####\n", __FILE__, __LINE__);
				av_free(arrays);
				return -1;
			}
			MOV_SET();
		}
	}

binarysearch:
	av_free(arrays);

	a = - 1;
	b = end-start;

	//optimize appending index entries at the end
	if(b && entries[b-1].timestamp < wanted_timestamp)
		a= b-1;

	while (b - a > 1) {
		m = (a + b) >> 1;
		timestamp = entries[m].timestamp;
		if(timestamp >= wanted_timestamp)
			b = m;
		if(timestamp <= wanted_timestamp)
			a = m;
	}
	m= (flags & AVSEEK_FLAG_BACKWARD) ? a : b;
	if(nb_entries>1 && wanted_timestamp<=0)m=0;
	if(!(flags & AVSEEK_FLAG_ANY)){
		while(m>=0 && m<end-start && !(entries[m].flags & AVINDEX_KEYFRAME)){
			m += (flags & AVSEEK_FLAG_BACKWARD) ? -1 : 1;
			if(flags & AVSEEK_FLAG_BACKWARD){
				/* 如果m<0并且当前index_entries[0]不是第一个index */
				if(m<0 && start!=0){
					int nIndexTmp = (start - bwic > 0) ? (start - bwic) : 0;

					MOV_CLEAR();
					if(mov_build_index(NULL,st,nIndexTmp)<0){
						gxlogd("####[mov opt]%s==>%d####\n",__FILE__,__LINE__);
						return -1;
					}
					MOV_SET();
					m = ((end - start) > bwic) ? (bwic - 1) : (end - start - 1);

				}
			}else{
				if(m>=end-start && end!=nb_entries){

					MOV_CLEAR();
					if(mov_build_index(NULL,st,m)<0){
						gxlogd("####[mov opt]%s==>%d####\n",__FILE__,__LINE__);
						return -1;
					}
					MOV_SET();
					m=0;
				}
			}
		}

		if(m == end-start)
			return -1;
	}
	return  m+st->index_entries_start;
}

int ff_index_search_pos_mov(AVStream* st, int64_t wanted_pos, int flags)
{
	int a, b, m, arr_num = 0, index = 0, low = -1, high;
	int64_t pos;
	int nb_entries=0,index_entries_end=0,index_entries_start=0;
	int nIndex=0;
	AVIndexEntry *entries;
	MovIndex *arrays = NULL;
	int bwic = ((st->codec->codec_type == CODEC_TYPE_SUBTITLE) ? BACKWARD_SUB_INDEX_COUNT : BACKWARD_INDEX_COUNT);

#define MOV_POS_CLEAR() \
	do{ \
		st->index_entries_start=0; \
		st->index_entries_end=0;   \
		st->nb_index_entries=0;    \
		st->index_entries_allocated_size=0;  \
		av_freep(&st->index_entries);  \
	}while(0)

#define MOV_POS_SET() \
	do{ \
		index_entries_start = st->index_entries_start;	 \
		index_entries_end = st->index_entries_end;	 \
		entries = st->index_entries;   \
	}while(0)

	nb_entries=st->nb_index_entries;
	MOV_POS_SET();
	if(!entries)
		return -1;

	if(index_entries_start==0 && index_entries_end==nb_entries) {
		return av_index_search_pos(st, wanted_pos,flags);
	}
	if(_mov_build_index(st, nb_entries, &arrays, &arr_num) < 0)
		return -1;

	high = arr_num;
	while (high - low > 1) {
		if (entries[index_entries_end - index_entries_start - 1].pos >= wanted_pos &&
				entries[0].pos <= wanted_pos) {
			goto binarysearch;
		} else {

			if (entries[index_entries_end - index_entries_start - 1].pos <= wanted_pos) {
				low = _mov_find_index(arrays, arr_num, index_entries_start);
			}
			if (entries[0].pos >= wanted_pos) {
				high = _mov_find_index(arrays, arr_num, index_entries_start);
			}
			index = (low + high) >> 1;
			if (index < 0)
				index = 0;
			MOV_POS_CLEAR();
			if (mov_build_index(NULL, st, arrays[index].start) < 0) {
				gxlogd("####[mov opt]####\n");
				av_free(arrays);
				return -1;
			}
			MOV_POS_SET();
		}
	}

	if (high - low <= 1 && arr_num > 1) {
		/*
		 * not found, high -lown <= 1
		 * Possible boundary between the array
		 */
		index = _mov_find_index(arrays, arr_num, index_entries_start);
		if ((entries[index_entries_end - index_entries_start - 1].pos < wanted_pos) &&
				(index + 1 < arr_num)) {
			MOV_POS_CLEAR();
			if (mov_build_index(NULL, st, arrays[index].end - 1 - MOV_INDEX_COUNT/2) < 0) {
				gxlogd("####[mov opt]####\n");
				av_free(arrays);
				return -1;
			}
			MOV_POS_SET();
			goto binarysearch;
		}

		if ((entries[0].pos > wanted_pos) &&
				(index > 0)) {
			MOV_POS_CLEAR();
			if (mov_build_index(NULL, st, arrays[index - 1].end - 1 - MOV_INDEX_COUNT/2) < 0) {
				gxlogd("####[mov opt]####\n");
				av_free(arrays);
				return -1;
			}
			MOV_POS_SET();
		}
	}
binarysearch:
	av_free(arrays);
	a = - 1;
	b = index_entries_end-index_entries_start;

	//optimize appending index entries at the end
	if(b && entries[b-1].pos < wanted_pos)
		a= b-1;

	while (b - a > 1) {
		m = (a + b) >> 1;
		pos = entries[m].pos;
		if(pos >= wanted_pos)
			b = m;
		if(pos <= wanted_pos)
			a = m;
	}
	m= (flags & AVSEEK_FLAG_BACKWARD) ? a : b;
	if(nb_entries>1 && wanted_pos<=0)m=0;
	if(!(flags & AVSEEK_FLAG_ANY)){
		while(m>=0 && m<index_entries_end-index_entries_start && !(entries[m].flags & AVINDEX_KEYFRAME)){
			m += (flags & AVSEEK_FLAG_BACKWARD) ? -1 : 1;
			if(flags & AVSEEK_FLAG_BACKWARD){
				/* 如果m<0并且当前index_entries[0]不是第一个index */
				if(m<0 && index_entries_start!=0){
					int nIndexTmp = (index_entries_start - bwic > 0) ? (index_entries_start - bwic) : 0;
					MOV_POS_CLEAR();
					if(mov_build_index(NULL,st,nIndexTmp)<0){
						gxlogd("####[mov opt]####\n");
						return -1;
					}
					MOV_POS_SET();
					m = ((index_entries_end - index_entries_start) > bwic) ? (bwic - 1) : (index_entries_end - index_entries_start - 1);

				}
			}else{
				if(m>=index_entries_end-index_entries_start && index_entries_end!=nb_entries){
					MOV_POS_CLEAR();
					if(mov_build_index(NULL,st,m)<0){
						gxlogd("####[mov opt]####\n");
						return -1;
					}
					MOV_POS_SET();
					m=0;
				}
			}
		}

		if(m == index_entries_end-index_entries_start)
			return -1;
	}
	return  m+st->index_entries_start;
}

int av_index_search_timestamp_mov(AVStream*  st, int64_t wanted_timestamp, int flags, int ass_is_forward)
{
	return ff_index_search_timestamp_mov(st, wanted_timestamp, flags, ass_is_forward);
}

int av_index_search_pos_mov(AVStream*  st, int64_t wanted_pos, int flags)
{
	return ff_index_search_pos_mov(st, wanted_pos, flags);
}

static int mov_read_trak(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream* st;
	MOVStreamContext* sc;
	int ret;
	int size = 0;

	st = av_new_stream(c->fc, c->fc->nb_streams);
	if (!st) return AVERROR(ENOMEM);
	sc = av_mallocz(sizeof(MOVStreamContext));
	if (!sc) return AVERROR(ENOMEM);

	st->priv_data = sc;
	st->codec->codec_type = CODEC_TYPE_DATA;
	sc->ffindex = st->index;

	if ((ret = mov_read_default(c, pb, atom)) < 0)
		return ret;

	/* sanity checks*/
	if (sc->chunk_count && (!sc->stts_count || !sc->stsc_count ||
				(!sc->sample_size && !sc->sample_count))) {
		gxlogd( "stream %d, missing mandatory atoms, broken header\n",
				st->index);
		return 0;
	}

	if (!sc->time_scale)
		sc->time_scale = c->time_scale;

	av_set_pts_info(st, 64, 1, sc->time_scale);

	if (st->codec->codec_type == CODEC_TYPE_AUDIO &&
			!st->codec->frame_size && sc->stts_count == 1) {
		MOVStts_32 stts_0 = {0, 0};

		get_stts_by_index(sc, 0, &stts_0);
		st->codec->frame_size = av_rescale(stts_0.duration,
				st->codec->sample_rate, sc->time_scale);
	}

	if((ret = mov_build_index(c, st,0)) < 0) {
		av_freep((void**)&sc->chunk_offsets);
		size += sc->chunk_count * sc->chunk_bytes;
		av_freep((void**)&sc->stsc_data);
		size += sc->stsc_count * sizeof(*sc->stsc_data);
		av_freep((void**)&sc->sample_sizes);
		size +=sc->sample_count * sizeof(int);
		av_freep((void**)&sc->keyframes);
		size += sc->keyframe_count * sizeof(int);
		av_freep((void**)&sc->stts_data);
		size += sc->stts_count * sc->stts_bytes;
		av_freep((void**)&sc->elst_data);
		size += sc->elst_count * sizeof(*sc->elst_data);
		av_freep((void**)&sc->stps_data);
		size += sc->stps_count * sizeof(*sc->stps_data);
		check_index_space(c->fc, size, 1);
		return -1;
	}

	if (sc->dref_id-1 < sc->drefs_count && sc->drefs[sc->dref_id-1].path) {
		if (url_fopen(&sc->pb, sc->drefs[sc->dref_id-1].path, URL_RDONLY, NULL) < 0)
			gxlogd( "stream %d, error opening file %s: %s\n",
					st->index, sc->drefs[sc->dref_id-1].path, strerror(errno));
	} else {
		sc->pb = c->fc->pb;
        sc->pb_is_copied = 1;
	}
	return 0;
}

static int mov_read_ilst(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	int ret;
	c->itunes_metadata = 1;
	ret = mov_read_default(c, pb, atom);
	c->itunes_metadata = 0;
	return ret;
}

static int mov_read_meta(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	while (atom.size > 8) {
		uint32_t tag = get_le32(pb);
		atom.size -= 4;
		if (tag == MKTAG('h','d','l','r')) {
			url_fseek(pb, -8, SEEK_CUR);
			atom.size += 8;
			return mov_read_default(c, pb, atom);
		}
	}
	return 0;
}

static int mov_read_tkhd(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	int i;
	int width;
	int height;
	int64_t disp_transform[2];
	int display_matrix[3][2];
	AVStream* st;
	MOVStreamContext* sc;
	int version;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];
	sc = st->priv_data;

	version = get_byte(pb);
	get_be24(pb); /* flags*/
	/*
	   MOV_TRACK_ENABLED 0x0001
	   MOV_TRACK_IN_MOVIE 0x0002
	   MOV_TRACK_IN_PREVIEW 0x0004
	   MOV_TRACK_IN_POSTER 0x0008
	   */

	if (version == 1) {
		get_be64(pb);
		get_be64(pb);
	} else {
		get_be32(pb); /* creation time*/
		get_be32(pb); /* modification time*/
	}
	st->id = (int)get_be32(pb); /* track id (NOT 0 !)*/
	get_be32(pb); /* reserved*/

	/* highlevel (considering edits) duration in movie timebase*/
	(version == 1) ? get_be64(pb) : get_be32(pb);
	get_be32(pb); /* reserved*/
	get_be32(pb); /* reserved*/

	get_be16(pb); /* layer*/
	get_be16(pb); /* alternate group*/
	get_be16(pb); /* volume*/
	get_be16(pb); /* reserved*/

	//read in the display matrix (outlined in ISO 14496-12, Section 6.2.2)
	// they're kept in fixed point format through all calculations
	// ignore u,v,z b/c we don't need the scale factor to calc aspect ratio
	for (i = 0; i < 3; i++) {
		display_matrix[i][0] = get_be32(pb);   // 16.16 fixed point
		display_matrix[i][1] = get_be32(pb);   // 16.16 fixed point
		get_be32(pb);           // 2.30 fixed point (not used)
	}

	width = get_be32(pb);       // 16.16 fixed point track width
	height = get_be32(pb);      // 16.16 fixed point track height
	sc->width = width >> 16;
	sc->height = height >> 16;

	// transform the display width/height according to the matrix
	// skip this if the display matrix is the default identity matrix
	// or if it is rotating the picture, ex iPhone 3GS
	// to keep the same scale, use [width height 1<<16]
	if (width && height &&
			((display_matrix[0][0] != 65536  ||
			  display_matrix[1][1] != 65536) &&
			 !display_matrix[0][1] &&
			 !display_matrix[1][0] &&
			 !display_matrix[2][0] && !display_matrix[2][1])) {
		for (i = 0; i < 2; i++)
			disp_transform[i] =
				(int64_t)  width *  display_matrix[0][i] +
				(int64_t)  height*  display_matrix[1][i] +
				((int64_t) display_matrix[2][i] << 16);

		//sample aspect ratio is new width/height divided by old width/height
		st->sample_aspect_ratio = av_d2q(
				((double) disp_transform[0]*  height) /
				((double) disp_transform[1]*  width), INT_MAX);
	}
	return 0;
}

static int mov_read_tfhd(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	MOVFragment* frag = &c->fragment;
	MOVTrackExt* trex = NULL;
	int flags, track_id, i;

	get_byte(pb); /* version*/
	flags = get_be24(pb);

	track_id = get_be32(pb);
	if (!track_id)
		return -1;
	frag->track_id = track_id;
	for (i = 0; i < c->trex_count; i++)
		if (c->trex_data[i].track_id == frag->track_id) {
			trex = &c->trex_data[i];
			break;
		}
	if (!trex) {
		gxlogd( "could not find corresponding trex\n");
		return -1;
	}

	if (flags & 0x01) frag->base_data_offset = get_be64(pb);
	else              frag->base_data_offset = frag->moof_offset;
	frag->stsd_id  = (flags & 0x02) ? get_be32(pb):trex->stsd_id;

	frag->duration = flags & 0x08 ? get_be32(pb) : trex->duration;
	frag->size     = flags & 0x10 ? get_be32(pb) : trex->size;
	frag->flags    = flags & 0x20 ? get_be32(pb) : trex->flags;

	return 0;
}

static int mov_read_trex(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	MOVTrackExt* trex;

	if ((uint64_t)c->trex_count+1 >= UINT_MAX / sizeof(*c->trex_data))
		return -1;
	trex = av_realloc(c->trex_data, (c->trex_count+1)*sizeof(*c->trex_data));
	if (!trex) {
		gxloge ("malloc failed!! %s:count:%d,size:%d\n", __func__, c->trex_count, (c->trex_count+1)*sizeof(*c->trex_data));
		return AVERROR(ENOMEM);
	}
	c->trex_data = trex;
	trex = &c->trex_data[c->trex_count++];
	get_byte(pb); /* version*/
	get_be24(pb); /* flags*/
	trex->track_id = get_be32(pb);
	trex->stsd_id  = get_be32(pb);
	trex->duration = get_be32(pb);
	trex->size     = get_be32(pb);
	trex->flags    = get_be32(pb);
	return 0;
}

static MOVFragmentStreamInfo * get_current_frag_stream_info(
		MOVFragmentIndex *frag_index)
{
	MOVFragmentIndexItem *item;
	if (frag_index->current < 0 ||
			frag_index->current >= frag_index->nb_items)
		return NULL;

	item = &frag_index->item[frag_index->current];
	if (item->current >= 0 && item->current < item->nb_stream_info)
		return &item->stream_info[item->current];

	// This shouldn't happen
	return NULL;
}

static int mov_read_tfdt(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
	MOVFragment *frag = &c->fragment;
	AVStream *st = NULL;
	MOVStreamContext *sc;
	int version, i;
	MOVFragmentStreamInfo * frag_stream_info;
	int64_t base_media_decode_time;

	for (i = 0; i < c->fc->nb_streams; i++) {
		if (c->fc->streams[i]->id == frag->track_id) {
			st = c->fc->streams[i];
			break;
		}
	}

	if (!st) {
		gxlogd ("could not find corresponding track id %u\n", frag->track_id);
		return AVERROR_INVALIDDATA;
	}
	sc = st->priv_data;
	if (sc->pseudo_stream_id + 1 != frag->stsd_id && sc->pseudo_stream_id != -1) {
		return 0;
	}
	version = avio_r8(pb);
	avio_rb24(pb); /* flags */
	if (version) {
		base_media_decode_time = avio_rb64(pb);
	} else {
		base_media_decode_time = avio_rb32(pb);
	}

	frag_stream_info = get_current_frag_stream_info(&c->frag_index);
	if (frag_stream_info) {
		frag_stream_info->tfdt_dts = base_media_decode_time;
	}
	//sc->track_end = base_media_decode_time;

	return 0;
}

static MOVFragmentStreamInfo * get_frag_stream_info(MOVFragmentIndex *frag_index, int index, int id)
{
	int i;
	MOVFragmentIndexItem * item;

	if (index < 0 || index >= frag_index->nb_items)
		return NULL;
	item = &frag_index->item[index];
	for (i = 0; i < item->nb_stream_info; i++)
		if (item->stream_info[i].id == id)
			return &item->stream_info[i];

	// This shouldn't happen
	return NULL;
}

#define FF_MOV_FLAG_MFRA_PTS 2
static int mov_read_trun(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	MOVFragment* frag = &c->fragment;
	AVStream* st = NULL;
	MOVStreamContext* sc;
	uint64_t offset;
	int64_t dts;
	int data_offset = 0;
	unsigned entries, first_sample_flags = frag->flags;
	int flags, distance, i, ret = 0, index_entry_pos = 0;
	int nPlusOnly=0;
	MOVFragmentStreamInfo * frag_stream_info;

	for (i = 0; i < c->fc->nb_streams; i++) {
		if (c->fc->streams[i]->id == frag->track_id) {
			st = c->fc->streams[i];
			break;
		}
	}
	if (!st) {
		gxlogd( "could not find corresponding track id %d\n", frag->track_id);
		return -1;
	}
	sc = st->priv_data;
	if (sc->pseudo_stream_id+1 != frag->stsd_id && sc->pseudo_stream_id != -1) {
		return 0;
	}
	// Find the next frag_index index that has a valid index_entry for
	// the current track_id.
	//
	// A valid index_entry means the trun for the fragment was read
	// and it's samples are in index_entries at the given position.
	// New index entries will be inserted before the index_entry found.
	index_entry_pos = st->nb_index_entries;
	for (i = c->frag_index.current + 1; i < c->frag_index.nb_items; i++) {
		frag_stream_info = get_frag_stream_info(&c->frag_index, i, frag->track_id);
		if (frag_stream_info && frag_stream_info->index_entry >= 0) {
			index_entry_pos = frag_stream_info->index_entry;
			break;
		}
	}

	get_byte(pb); /* version*/
	flags = get_be24(pb);
	entries = get_be32(pb);

	if (flags & 0x001) data_offset        = get_be32(pb);
	if (flags & 0x004) first_sample_flags = get_be32(pb);
	if (flags & 0x800) {
		void *ctts = NULL;

		if (sc->ctts_bytes == 0)
			sc->ctts_bytes = sizeof(MOVCtts_32);

		if (((uint64_t)entries + sc->ctts_count) >= (UINT_MAX / sc->ctts_bytes))
			return -1;

		ctts = av_realloc(sc->ctts_data,
				(entries + sc->ctts_count) * sc->ctts_bytes);
		if (!ctts) {
			gxloge ("malloc failed!! %s:entries:%d,count:%d,size:%d\n", __func__, entries, sc->ctts_count, (entries + sc->ctts_count) * sc->ctts_bytes);
			return AVERROR(ENOMEM);
		}

		sc->ctts_data = ctts;
	}
	dts = st->duration;
	offset = frag->base_data_offset + data_offset;
	distance = 0;
	frag_stream_info = get_current_frag_stream_info(&c->frag_index);
	if (frag_stream_info) {
		if (frag_stream_info->sidx_pts != AV_NOPTS_VALUE) {
			// FIXME: sidx earliest_presentation_time is *PTS*, s.b.
			// pts = frag_stream_info->sidx_pts;
			dts = frag_stream_info->sidx_pts - sc->time_offset;
		} else if (frag_stream_info->tfdt_dts != AV_NOPTS_VALUE) {
			dts = frag_stream_info->tfdt_dts - sc->time_offset;
		}
	}

	// Record the index_entry position in frag_index of this fragment
	if (frag_stream_info) {
		frag_stream_info->index_entry = index_entry_pos;
	}

	for (i = 0; i < entries; i++) {
		unsigned sample_size = frag->size;
		int sample_flags = i ? frag->flags : first_sample_flags;
		unsigned sample_duration = frag->duration;
		int keyframe;

		if (flags & 0x100) sample_duration = get_be32(pb);
		if (flags & 0x200) sample_size     = get_be32(pb);
		if (flags & 0x400) sample_flags    = get_be32(pb);
		if (flags & 0x800) {
			if (sc->ctts_bytes == sizeof(MOVCtts_32)) {
				MOVCtts_32 *ctts = (MOVCtts_32 *)sc->ctts_data;

				ctts[sc->ctts_count].count = 1;
				ctts[sc->ctts_count].duration = get_be32(pb);
			} else if (sc->ctts_bytes == sizeof(MOVCtts_16)) {
				MOVCtts_16 *ctts = (MOVCtts_16 *)sc->ctts_data;
				int duration = get_be32(pb);

				if ((duration >> 16) & 0xffff) {
					gxloge("warnning mov ctts duration 32 bits, now 16 bits\n");
				}
				ctts[sc->ctts_count].count = 1;
				ctts[sc->ctts_count].duration = (unsigned short)duration;
			} else if (sc->ctts_bytes == sizeof(MOVCtts_8)) {
				MOVCtts_8 *ctts = (MOVCtts_8 *)sc->ctts_data;
				int duration = get_be32(pb);

				if ((duration >> 8) & 0xffffff) {
					gxloge("warnning mov ctts duration 32 bits, now 8 bits\n");
				}
				ctts[sc->ctts_count].count = 1;
				ctts[sc->ctts_count].duration = (unsigned char)duration;
			}
			sc->ctts_count++;
		}
		if ((keyframe = st->codec->codec_type == CODEC_TYPE_AUDIO ||
					(flags & 0x004 && !i && !sample_flags) || sample_flags & 0x2000000))
			distance = 0;
		ret = mov_add_index_entry(st, offset, dts, sample_size, distance,
				keyframe ? AVINDEX_KEYFRAME : 0, 1, nPlusOnly);
		if(ret < 0){
			if(ret==-2)
				nPlusOnly=1;//realoc is error
			else
				return -1;
		}

		distance++;
		dts += sample_duration;
		offset += sample_size;
	}
	if (st->duration < dts)
		st->duration = dts;
	return 0;
}

static inline AVRational av_make_q(int num, int den)
{
	AVRational r = { num, den };
	return r;
}

static int mov_read_sidx(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
	int64_t offset = avio_tell(pb) + atom.size, pts, timestamp;
	uint8_t version;
	unsigned i, j, track_id, item_count;
	AVStream *st = NULL;
	AVStream *ref_st = NULL;
	MOVStreamContext *sc, *ref_sc = NULL;
	AVRational timescale;

	version = avio_r8(pb);
	if (version > 1) {
		return 0;
	}

	avio_rb24(pb); // flags

	track_id = avio_rb32(pb); // Reference ID
	for (i = 0; i < c->fc->nb_streams; i++) {
		if (c->fc->streams[i]->id == track_id) {
			st = c->fc->streams[i];
			break;
		}
	}

	if (!st) {
		av_log(c->fc, AV_LOG_WARNING, "could not find corresponding track id %d\n", track_id);
		return 0;
	}

	sc = st->priv_data;
	timescale = av_make_q(1, avio_rb32(pb));

	if (timescale.den <= 0) {
		gxlogd ("Invalid sidx timescale 1/%d\n", timescale.den);
		return AVERROR_INVALIDDATA;
	}

	if (version == 0) {
		pts = avio_rb32(pb);
		offset += avio_rb32(pb);
	} else {
		pts = avio_rb64(pb);
		offset += avio_rb64(pb);
	}

	avio_rb16(pb); // reserved
	item_count = avio_rb16(pb);

	for (i = 0; i < item_count; i++) {
		int index;
		MOVFragmentStreamInfo * frag_stream_info;
		uint32_t size = avio_rb32(pb);
		uint32_t duration = avio_rb32(pb);
		if (size & 0x80000000) {
			return AVERROR_PATCHWELCOME;
		}
		avio_rb32(pb); // sap_flags
		timestamp = av_rescale_q(pts, st->time_base, timescale);

		index = update_frag_index(c, offset);
		frag_stream_info = get_frag_stream_info(&c->frag_index, index, track_id);
		if (frag_stream_info) {
			frag_stream_info->sidx_pts = timestamp;
		}

		offset += size;
		pts += duration;
	}

	st->duration  = pts;
	sc->has_sidx = 1;

	if (offset == avio_size(pb)) {
		// Find first entry in fragment index that came from an sidx.
		// This will pretty much always be the first entry.
		for (i = 0; i < c->frag_index.nb_items; i++) {
			MOVFragmentIndexItem * item = &c->frag_index.item[i];
			for (j = 0; ref_st == NULL && j < item->nb_stream_info; j++) {
				MOVFragmentStreamInfo * si;
				si = &item->stream_info[j];
				if (si->sidx_pts != AV_NOPTS_VALUE) {
					ref_st = c->fc->streams[j];
					ref_sc = ref_st->priv_data;
					break;
				}
			}
		}

		for (i = 0; i < c->fc->nb_streams; i++) {
			st = c->fc->streams[i];
		sc = st->priv_data;
			if (!sc->has_sidx) {
				st->duration  = av_rescale(ref_st->duration, sc->time_scale, ref_sc->time_scale);
			}
		}
		c->frag_index.complete = 1;
	}
	return 0;
}

/* this atom should be null (from specs), but some buggy files put the 'moov' atom inside it...*/
/* like the files created with Adobe Premiere 5.0, for samples see*/
/* http://graphics.tudelft.nl/~wouter/publications/soundtests/*/
static int mov_read_wide(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	int err;

	if (atom.size < 8)
		return 0; /* continue*/
	if (get_be32(pb) != 0) { /* 0 sized mdat atom... use the 'wide' atom size*/
		url_fskip(pb, atom.size - 4);
		return 0;
	}
	atom.type = get_le32(pb);
	atom.offset += 8;
	atom.size -= 8;
	if (atom.type != MKTAG('m','d','a','t')) {
		url_fskip(pb, atom.size);
		return 0;
	}
	err = mov_read_mdat(c, pb, atom);
	return err;
}

static int mov_read_cmov(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
#ifdef CONFIG_ZLIB
	ByteIOContext ctx;
	uint8_t* cmov_data;
	uint8_t* moov_data; /* uncompressed data */
	long cmov_len, moov_len;
	int ret = -1;

	get_be32(pb); /* dcom atom*/
	if (get_le32(pb) != MKTAG('d','c','o','m'))
		return -1;
	if (get_le32(pb) != MKTAG('z','l','i','b')) {
		gxlogd( "unknown compression for cmov atom !");
		return -1;
	}
	get_be32(pb); /* cmvd atom*/
	if (get_le32(pb) != MKTAG('c','m','v','d'))
		return -1;
	moov_len = get_be32(pb); /* uncompressed size*/
	cmov_len = atom.size - 6*  4;

	cmov_data = av_malloc(cmov_len);
	if (!cmov_data) {
		gxloge ("malloc failed!! %s:size:%d\n", __func__, cmov_len);
		return AVERROR(ENOMEM);
	}
	moov_data = av_malloc(moov_len);
	if (!moov_data) {
		gxloge ("malloc failed!! %s:size:%d\n", __func__, moov_len);
		av_free(cmov_data);
		return AVERROR(ENOMEM);
	}
	get_buffer(pb, cmov_data, cmov_len);
	if(uncompress (moov_data, (uLongf* ) &moov_len, (const Bytef *)cmov_data, cmov_len) != Z_OK)
		goto free_and_return;
	if(ffio_init_context(&ctx, moov_data, moov_len, 0, NULL, NULL, NULL, NULL) != 0)
		goto free_and_return;
	atom.type = MKTAG('m','o','o','v');
	atom.offset = 0;
	atom.size = moov_len;

	ret = mov_read_default(c, &ctx, atom);
free_and_return:
	av_free(moov_data);
	av_free(cmov_data);
	return ret;
#else
	gxlogd( "this file requires zlib support compiled in\n");
	return -1;
#endif
}

/* edit list atom*/
static int mov_read_elst(MOVContext* c, ByteIOContext *pb, MOVAtom atom)
{
	MOVStreamContext* sc;
	int i, edit_count, version;
	int64_t elst_entry_size;

	if (c->fc->nb_streams < 1)
		return 0;
	sc = c->fc->streams[c->fc->nb_streams-1]->priv_data;

	version = get_byte(pb); /* version*/
	get_be24(pb); /* flags*/
	edit_count = get_be32(pb); /* entries*/

	elst_entry_size = version == 1 ? 20 : 12;
	if((uint64_t)edit_count*12+8 > atom.size)
		return -1;

	if (!edit_count)
		return 0;
	if (sc->elst_data) {
		av_free(sc->elst_data);
		sc->elst_data = NULL;
	}
    sc->elst_count = 0;
    sc->elst_data = av_malloc_array(edit_count, sizeof(*sc->elst_data));
    if (!sc->elst_data)
        return AVERROR(ENOMEM);
	for (i = 0; i < edit_count && atom.size > 0 && !pb->eof_reached; i++) {
		MOVElst *e = &sc->elst_data[i];
		if (version == 1) {
			e->duration = avio_rb64(pb);
			e->time     = avio_rb64(pb);
			atom.size -= 16;
		} else {
			e->duration = avio_rb32(pb); /* segment duration */
			e->time     = (int32_t)avio_rb32(pb); /* media time */
			atom.size -= 8;
		}
		e->rate = avio_rb32(pb)/65536.0;
		atom.size -= 4;
	}
	sc->elst_count = i;
	return 0;
}

static int mov_read_sample_encryption_info(MOVContext *c, AVIOContext *pb, MOVStreamContext *sc, AVEncryptionInfo **sample, int use_subsamples)
{
	int i;
	unsigned int subsample_count;
	AVSubsampleEncryptionInfo *subsamples;

	if (!sc->cenc.default_encrypted_sample) {
		gxloge ("Missing schm or tenc\n");
		return AVERROR_INVALIDDATA;
	}

	*sample = av_encryption_info_clone(sc->cenc.default_encrypted_sample);
	if (!*sample)
		return AVERROR(ENOMEM);

	if (sc->cenc.per_sample_iv_size != 0) {
		if (avio_read(pb, (*sample)->iv, sc->cenc.per_sample_iv_size) != sc->cenc.per_sample_iv_size) {
			gxloge ("failed to read the initialization vector\n");
			av_encryption_info_free(*sample);
			*sample = NULL;
			return AVERROR_INVALIDDATA;
		}
	}

	if (use_subsamples) {
		subsample_count = avio_rb16(pb);
		av_free((*sample)->subsamples);
		(*sample)->subsamples = av_calloc(subsample_count, sizeof(*subsamples));
		if (!(*sample)->subsamples) {
			gxloge ("malloc failed!! %s:count:%d,size:%d\n", __func__, subsample_count, subsample_count*sizeof(*subsamples));
			av_encryption_info_free(*sample);
			*sample = NULL;
			return AVERROR(ENOMEM);
		}

		for (i = 0; i < subsample_count && !pb->eof_reached; i++) {
			(*sample)->subsamples[i].bytes_of_clear_data = avio_rb16(pb);
			(*sample)->subsamples[i].bytes_of_protected_data = avio_rb32(pb);
		}

		if (pb->eof_reached) {
			gxloge ("hit EOF while reading sub-sample encryption info\n");
			av_encryption_info_free(*sample);
			*sample = NULL;
			return AVERROR_INVALIDDATA;
		}
		(*sample)->subsample_count = subsample_count;
	}

	return 0;
}

/**
 * Gets the current encryption info and associated current stream context.  If
 * we are parsing a track fragment, this will return the specific encryption
 * info for this fragment; otherwise this will return the global encryption
 * info for the current stream.
 */
static int get_current_encryption_info(MOVContext *c, MOVEncryptionIndex **encryption_index, MOVStreamContext **sc)
{
	MOVFragmentStreamInfo *frag_stream_info;
	AVStream *st = NULL;
	int i;

	frag_stream_info = get_current_frag_stream_info(&c->frag_index);
	if (frag_stream_info) {
		for (i = 0; i < c->fc->nb_streams; i++) {
			if (c->fc->streams[i]->id == frag_stream_info->id) {
				st = c->fc->streams[i];
				break;
			}
		}
		if (i == c->fc->nb_streams)
			return 0;
		*sc = st->priv_data;

		if (!frag_stream_info->encryption_index) {
			frag_stream_info->encryption_index = av_mallocz(sizeof(*frag_stream_info->encryption_index));
			if (!frag_stream_info->encryption_index) {
				gxloge ("malloc failed!! %s:size:%d\n", __func__, sizeof(*frag_stream_info->encryption_index));
				return AVERROR(ENOMEM);
			}
		}
		*encryption_index = frag_stream_info->encryption_index;
		return 1;
	} else {
		// No current track fragment, using stream level encryption info.

		if (c->fc->nb_streams < 1)
			return 0;
		st = c->fc->streams[c->fc->nb_streams - 1];
		*sc = st->priv_data;

		if (!(*sc)->cenc.encryption_index) {
			(*sc)->cenc.encryption_index = av_mallocz(sizeof(*frag_stream_info->encryption_index));
			if (!(*sc)->cenc.encryption_index) {
				gxloge ("malloc failed!! %s:size:%d\n", __func__, sizeof(*frag_stream_info->encryption_index));
				return AVERROR(ENOMEM);
			}
		}

		*encryption_index = (*sc)->cenc.encryption_index;
		return 1;
	}
}

static int mov_parse_auxiliary_info(MOVContext *c, MOVStreamContext *sc, ByteIOContext *pb, MOVEncryptionIndex *encryption_index)
{
	AVEncryptionInfo **sample, **encrypted_samples;
	int64_t prev_pos;
	size_t sample_count, sample_info_size, i = 0;
	int ret = 0;
	unsigned int alloc_size = 0;

	if (encryption_index->nb_encrypted_samples)
		return 0;
	sample_count = encryption_index->auxiliary_info_sample_count;
	if (encryption_index->auxiliary_offsets_count != 1) {
		gxlogd("Multiple auxiliary info chunks are not supported\n");
		return AVERROR_PATCHWELCOME;
	}
	if (sample_count >= INT_MAX / sizeof(*encrypted_samples))
		return AVERROR(ENOMEM);

	prev_pos = avio_tell(pb);
	if (!(pb->seekable & AVIO_SEEKABLE_NORMAL) ||
			avio_seek(pb, encryption_index->auxiliary_offsets[0], SEEK_SET) != encryption_index->auxiliary_offsets[0]) {
		gxlogd("Failed to seek for auxiliary info, will only parse senc atoms for encryption info\n");
		goto finish;
	}

	for (i = 0; i < sample_count && !pb->eof_reached; i++) {
		unsigned int min_samples = FFMIN(FFMAX(i + 1, 1024 * 1024), sample_count);
		encrypted_samples = av_fast_realloc(encryption_index->encrypted_samples, &alloc_size,
				min_samples * sizeof(*encrypted_samples));
		if (!encrypted_samples) {
			gxloge ("malloc failed!! %s:entries:%d,size:%d\n", __func__, min_samples, min_samples*sizeof(*encrypted_samples));
			ret = AVERROR(ENOMEM);
			goto finish;
		}
		encryption_index->encrypted_samples = encrypted_samples;

		sample = &encryption_index->encrypted_samples[i];
		sample_info_size = encryption_index->auxiliary_info_default_size
			? encryption_index->auxiliary_info_default_size
			: encryption_index->auxiliary_info_sizes[i];

		ret = mov_read_sample_encryption_info(c, pb, sc, sample, sample_info_size > sc->cenc.per_sample_iv_size);
		if (ret < 0)
			goto finish;
	}
	if (pb->eof_reached) {
		gxlogd("Hit EOF while reading auxiliary info\n");
		ret = AVERROR_INVALIDDATA;
	} else {
		encryption_index->nb_encrypted_samples = sample_count;
	}

finish:
	avio_seek(pb, prev_pos, SEEK_SET);
	if (ret < 0) {
		for (; i > 0; i--) {
			av_encryption_info_free(encryption_index->encrypted_samples[i - 1]);
		}
		av_freep(&encryption_index->encrypted_samples);
	}
	return ret;
}

static int mov_read_saiz(MOVContext *c, ByteIOContext *pb, MOVAtom atom)
{
	MOVEncryptionIndex *encryption_index;
	MOVStreamContext *sc;
	int ret;
	unsigned int sample_count, aux_info_type, aux_info_param;

	ret = get_current_encryption_info(c, &encryption_index, &sc);
	if (ret != 1)
		return ret;

	if (encryption_index->nb_encrypted_samples) {
		// This can happen if we have both saio/saiz and senc atoms.
		gxlogd ("Ignoring duplicate encryption info in saiz\n");
		return 0;
	}

	if (encryption_index->auxiliary_info_sample_count) {
		gxlogd ("Duplicate saiz atom\n");
		return AVERROR_INVALIDDATA;
	}

	avio_r8(pb); /* version */
	if (avio_rb24(pb) & 0x01) {  /* flags */
		aux_info_type = avio_rb32(pb);
		aux_info_param = avio_rb32(pb);
		if (sc->cenc.default_encrypted_sample) {
			if (aux_info_type != sc->cenc.default_encrypted_sample->scheme) {
				gxlogd ("Ignoring saiz box with non-zero aux_info_type\n");
				return 0;
			}
			if (aux_info_param != 0) {
				gxlogd ("Ignoring saiz box with non-zero aux_info_type_parameter\n");
				return 0;
			}
		} else {
			// Didn't see 'schm' or 'tenc', so this isn't encrypted.
			if ((aux_info_type == MKBETAG('c','e','n','c') ||
						aux_info_type == MKBETAG('c','e','n','s') ||
						aux_info_type == MKBETAG('c','b','c','1') ||
						aux_info_type == MKBETAG('c','b','c','s')) &&
					aux_info_param == 0) {
				gxlogd ("Saw encrypted saiz without schm/tenc\n");
				return AVERROR_INVALIDDATA;
			} else {
				return 0;
			}
		}
	} else if (!sc->cenc.default_encrypted_sample) {
		// Didn't see 'schm' or 'tenc', so this isn't encrypted.
		return 0;
	}

	encryption_index->auxiliary_info_default_size = avio_r8(pb);
	sample_count = avio_rb32(pb);

	if (encryption_index->auxiliary_info_default_size == 0) {
		encryption_index->auxiliary_info_sizes = av_mallocz(sample_count);
		if (!encryption_index->auxiliary_info_sizes)
			return AVERROR(ENOMEM);

		ret = avio_read(pb, encryption_index->auxiliary_info_sizes, sample_count);
		if (ret != sample_count) {
			av_freep(&encryption_index->auxiliary_info_sizes);
			if (ret >= 0)
				ret = AVERROR_INVALIDDATA;
			gxloge ("Failed to read the auxiliary info, %s\n", av_err2str(ret));
			return ret;
		}
	}
	encryption_index->auxiliary_info_sample_count = sample_count;

	if (encryption_index->auxiliary_offsets_count) {
		return mov_parse_auxiliary_info(c, sc, pb, encryption_index);
	}

	return 0;
}

static int mov_read_saio(MOVContext *c, ByteIOContext *pb, MOVAtom atom)
{
	uint64_t *auxiliary_offsets;
	MOVEncryptionIndex *encryption_index;
	MOVStreamContext *sc;
	int i, ret;
	unsigned int version, entry_count, aux_info_type, aux_info_param;
	unsigned int alloc_size = 0;

	ret = get_current_encryption_info(c, &encryption_index, &sc);
	if (ret != 1)
		return ret;

	if (encryption_index->nb_encrypted_samples) {
		// This can happen if we have both saio/saiz and senc atoms.
		gxlogd ("Ignoring duplicate encryption info in saio\n");
		return 0;
	}

	if (encryption_index->auxiliary_offsets_count) {
		gxloge ("Duplicate saio atom\n");
		return AVERROR_INVALIDDATA;
	}

	version = avio_r8(pb); /* version */
	if (avio_rb24(pb) & 0x01) {  /* flags */
		aux_info_type = avio_rb32(pb);
		aux_info_param = avio_rb32(pb);
		if (sc->cenc.default_encrypted_sample) {
			if (aux_info_type != sc->cenc.default_encrypted_sample->scheme) {
				gxlogd ("Ignoring saio box with non-zero aux_info_type\n");
				return 0;
			}
			if (aux_info_param != 0) {
				gxlogd ("Ignoring saio box with non-zero aux_info_type_parameter\n");
				return 0;
			}
		} else {
			// Didn't see 'schm' or 'tenc', so this isn't encrypted.
			if ((aux_info_type == MKBETAG('c','e','n','c') ||
						aux_info_type == MKBETAG('c','e','n','s') ||
						aux_info_type == MKBETAG('c','b','c','1') ||
						aux_info_type == MKBETAG('c','b','c','s')) &&
					aux_info_param == 0) {
				gxloge ("Saw encrypted saio without schm/tenc\n");
				return AVERROR_INVALIDDATA;
			} else {
				return 0;
			}
		}
	} else if (!sc->cenc.default_encrypted_sample) {
		// Didn't see 'schm' or 'tenc', so this isn't encrypted.
		return 0;
	}

	entry_count = avio_rb32(pb);
	if (entry_count >= INT_MAX / sizeof(*auxiliary_offsets))
		return AVERROR(ENOMEM);

	for (i = 0; i < entry_count && !pb->eof_reached; i++) {
		unsigned int min_offsets = FFMIN(FFMAX(i + 1, 1024), entry_count);
		auxiliary_offsets = av_fast_realloc(
				encryption_index->auxiliary_offsets, &alloc_size,
				min_offsets * sizeof(*auxiliary_offsets));
		if (!auxiliary_offsets) {
			av_freep(&encryption_index->auxiliary_offsets);
			gxloge ("malloc failed!! %s:offset:%d,size:%d\n", __func__, min_offsets, min_offsets * sizeof(*auxiliary_offsets));
			return AVERROR(ENOMEM);
		}
		encryption_index->auxiliary_offsets = auxiliary_offsets;

		if (version == 0) {
			encryption_index->auxiliary_offsets[i] = avio_rb32(pb);
		} else {
			encryption_index->auxiliary_offsets[i] = avio_rb64(pb);
		}
		if (c->frag_index.current >= 0) {
			encryption_index->auxiliary_offsets[i] += c->fragment.base_data_offset;
		}
	}

	if (pb->eof_reached) {
		gxlogd ("Hit EOF while reading saio\n");
		av_freep(&encryption_index->auxiliary_offsets);
		return AVERROR_INVALIDDATA;
	}

	encryption_index->auxiliary_offsets_count = entry_count;

	if (encryption_index->auxiliary_info_sample_count) {
		return mov_parse_auxiliary_info(c, sc, pb, encryption_index);
	}

	return 0;
}

static int mov_codec_id(AVStream *st, uint32_t format)
{
	int id = ff_codec_get_id(ff_codec_movaudio_tags, format);

	if (id <= 0 &&
			((format & 0xFFFF) == 'm' + ('s' << 8) ||
			 (format & 0xFFFF) == 'T' + ('S' << 8)))
		id = ff_codec_get_id(ff_codec_wav_tags, bswap_32(format) & 0xFFFF);

	if (st->codec->codec_type != AVMEDIA_TYPE_VIDEO && id > 0) {
		st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
	} else if (st->codec->codec_type != AVMEDIA_TYPE_AUDIO &&
			/* skip old ASF MPEG-4 tag */
			format && format != MKTAG('m','p','4','s')) {
		id = ff_codec_get_id(ff_codec_movvideo_tags, format);
		if (id <= 0)
			id = ff_codec_get_id(ff_codec_bmp_tags, format);
		if (id > 0)
			st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
		else if (st->codec->codec_type == AVMEDIA_TYPE_DATA ||
				(st->codec->codec_type == AVMEDIA_TYPE_SUBTITLE &&
				 st->codec->codec_id == CODEC_ID_NONE)) {
			id = ff_codec_get_id(ff_codec_movsubtitle_tags, format);
			if (id <= 0) {
				id = (format == MOV_MP4_TTML_TAG || format == MOV_ISMV_TTML_TAG) ? AV_CODEC_ID_TTML : id;
			}
			if (id > 0)
				st->codec->codec_type = AVMEDIA_TYPE_SUBTITLE;
		}
	}

	st->codec->codec_tag = format;

	return id;
}

static int mov_read_frma(MOVContext *c, ByteIOContext *pb, MOVAtom atom)
{
	uint32_t format = avio_rl32(pb);
	enum AVCodecID id;
	AVStream *st;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams - 1];

	switch (st->codec->codec_tag)
	{
	case MKTAG('e','n','c','v'):        // encrypted video
	case MKTAG('e','n','c','a'):        // encrypted audio
		id = mov_codec_id(st, format);
		if (st->codec->codec_id != CODEC_ID_NONE &&
				st->codec->codec_id != id) {
			av_log(c->fc, AV_LOG_WARNING,
					"ignoring 'frma' atom of '%.4s', stream has codec id %d\n",
					(char*)&format, st->codec->codec_id);
			break;
		}

		st->codec->codec_id  = id;
		st->codec->codec_tag = format;
		break;

	default:
		if (format != st->codec->codec_tag) {
			av_log(c->fc, AV_LOG_WARNING,
					"ignoring 'frma' atom of '%.4s', stream format is '%.4s'\n",
					(char*)&format, (char*)&st->codec->codec_tag);
		}
		break;
	}

	return 0;
}

static int mov_read_senc(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
	AVEncryptionInfo **encrypted_samples;
	MOVEncryptionIndex *encryption_index;
	MOVStreamContext *sc;
	int use_subsamples, ret;
	unsigned int sample_count, i, alloc_size = 0;

	ret = get_current_encryption_info(c, &encryption_index, &sc);
	if (ret != 1)
		return ret;

	if (encryption_index->nb_encrypted_samples) {
		// This can happen if we have both saio/saiz and senc atoms.
		gxlogd ("Ignoring duplicate encryption info in senc\n");
		return 0;
	}

	avio_r8(pb); /* version */
	use_subsamples = avio_rb24(pb) & 0x02; /* flags */

	sample_count = avio_rb32(pb);
	if (sample_count >= INT_MAX / sizeof(*encrypted_samples))
		return AVERROR(ENOMEM);

	for (i = 0; i < sample_count; i++) {
		unsigned int min_samples = FFMIN(FFMAX(i + 1, 1024 * 1024), sample_count);
		encrypted_samples = av_fast_realloc(encryption_index->encrypted_samples, &alloc_size,
				min_samples * sizeof(*encrypted_samples));
		if (encrypted_samples) {
			encryption_index->encrypted_samples = encrypted_samples;

			ret = mov_read_sample_encryption_info(
					c, pb, sc, &encryption_index->encrypted_samples[i], use_subsamples);
		} else {
			ret = AVERROR(ENOMEM);
		}
		if (pb->eof_reached) {
			gxloge ("Hit EOF while reading senc\n");
		if (ret >= 0)
			av_encryption_info_free(encryption_index->encrypted_samples[i]);
			ret = AVERROR_INVALIDDATA;
		}

		if (ret < 0) {
			for (; i > 0; i--)
				av_encryption_info_free(encryption_index->encrypted_samples[i - 1]);
			av_freep(&encryption_index->encrypted_samples);
			return ret;
		}
	}
	encryption_index->nb_encrypted_samples = sample_count;

	return 0;
}

static int mov_read_schm(MOVContext *c, ByteIOContext *pb, MOVAtom atom)
{
	AVStream *st;
	MOVStreamContext *sc;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];
	sc = st->priv_data;

	if (sc->pseudo_stream_id != 0) {
		av_log(c->fc, AV_LOG_ERROR, "schm boxes are only supported in first sample descriptor\n");
		return AVERROR_PATCHWELCOME;
	}

	if (atom.size < 8)
		return AVERROR_INVALIDDATA;

	avio_rb32(pb); /* version and flags */

	if (!sc->cenc.default_encrypted_sample) {
		sc->cenc.default_encrypted_sample = av_encryption_info_alloc(0, 16, 16);
		if (!sc->cenc.default_encrypted_sample) {
			return AVERROR(ENOMEM);
		}
	}

	sc->cenc.default_encrypted_sample->scheme = avio_rb32(pb);
	return 0;
}

static int mov_read_tenc(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
	AVStream *st;
	MOVStreamContext *sc;
	unsigned int version, pattern, is_protected, iv_size;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];
	sc = st->priv_data;

	if (sc->pseudo_stream_id != 0) {
		av_log(c->fc, AV_LOG_ERROR, "tenc atom are only supported in first sample descriptor\n");
		return AVERROR_PATCHWELCOME;
	}

	if (!sc->cenc.default_encrypted_sample) {
		sc->cenc.default_encrypted_sample = av_encryption_info_alloc(0, 16, 16);
		if (!sc->cenc.default_encrypted_sample) {
			return AVERROR(ENOMEM);
		}
	}

	if (atom.size < 20)
		return AVERROR_INVALIDDATA;

	version = avio_r8(pb); /* version */
	avio_rb24(pb); /* flags */

	avio_r8(pb); /* reserved */
	pattern = avio_r8(pb);

	if (version > 0) {
		sc->cenc.default_encrypted_sample->crypt_byte_block = pattern >> 4;
		sc->cenc.default_encrypted_sample->skip_byte_block = pattern & 0xf;
	}

	is_protected = avio_r8(pb);
	if (is_protected && !sc->cenc.encryption_index) {
		// The whole stream should be by-default encrypted.
		sc->cenc.encryption_index = av_mallocz(sizeof(MOVEncryptionIndex));
		if (!sc->cenc.encryption_index)
			return AVERROR(ENOMEM);
	}
	sc->cenc.per_sample_iv_size = avio_r8(pb);
	if (sc->cenc.per_sample_iv_size != 0 && sc->cenc.per_sample_iv_size != 8 &&
			sc->cenc.per_sample_iv_size != 16) {
		gxlogd("invalid per-sample IV size value\n");
		return AVERROR_INVALIDDATA;
	}
	if (avio_read(pb, sc->cenc.default_encrypted_sample->key_id, 16) != 16) {
		gxlogd("failed to read the default key ID\n");
		return AVERROR_INVALIDDATA;
	}

	if (is_protected && !sc->cenc.per_sample_iv_size) {
		iv_size = avio_r8(pb);
		if (iv_size != 8 && iv_size != 16) {
			gxlogd("invalid default_constant_IV_size in tenc atom\n");
			return AVERROR_INVALIDDATA;
		}

		if (avio_read(pb, sc->cenc.default_encrypted_sample->iv, iv_size) != iv_size) {
			gxlogd("failed to read the default IV\n");
			return AVERROR_INVALIDDATA;
		}
	}

	return 0;
}

static int mov_read_dops(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
	AVStream *st;
	size_t size;
	int16_t pre_skip;
	unsigned int tags = 0;

	if (c->fc->nb_streams < 1)
		return 0;
	st = c->fc->streams[c->fc->nb_streams-1];
	if ((uint64_t)atom.size > (1<<30) || atom.size < 11)
		return AVERROR_INVALIDDATA;

	/* Check OpusSpecificBox version. */
	if (avio_r8(pb) != 0) {
		av_log(c->fc, AV_LOG_ERROR, "unsupported OpusSpecificBox version\n");
		return AVERROR_INVALIDDATA;
	}

	/* OpusSpecificBox size plus magic for Ogg OpusHead header. */
	size = atom.size + 8;

	if (ff_alloc_extradata(st->codec, size))
		return AVERROR(ENOMEM);

	tags = MKTAG('O','p','u','s');
	AV_WL32(st->codec->extradata,     tags);
	tags = MKTAG('H','e','a','d');
	AV_WL32(st->codec->extradata + 4, tags);
	AV_WB8(st->codec->extradata + 8, 1); /* OpusHead version */
	avio_read(pb, st->codec->extradata + 9, size - 9);

	/* OpusSpecificBox is stored in big-endian, but OpusHead is
	 * little-endian; aside from the preceeding magic and version they're
	 * otherwise currently identical.  Data after output gain at offset 16
	 * doesn't need to be bytewapped.
	 */
	pre_skip = AV_RB16(st->codec->extradata + 10);
	AV_WL16(st->codec->extradata + 10, pre_skip);
	AV_WL32(st->codec->extradata + 12, AV_RB32(st->codec->extradata + 12));
	AV_WL16(st->codec->extradata + 16, AV_RB16(st->codec->extradata + 16));
	st->codec->initial_padding = pre_skip;

	return 0;
}

static int mov_read_dvcc_dvvc(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    uint8_t buf[ISOM_DVCC_DVVC_SIZE];
    int ret;
    int64_t read_size = atom.size;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];

    // At most 24 bytes
    read_size = FFMIN(read_size, ISOM_DVCC_DVVC_SIZE);

    if ((ret = get_buffer(pb, buf, read_size)) < 0)
        return ret;

    return ff_isom_parse_dvcc_dvvc(c->fc, st, buf, read_size);
}

static const MOVParseTableEntry mov_default_parse_table[] = {
	{ MKTAG('a','v','s','s'), mov_read_extradata },
	{ MKTAG('c','o','6','4'), mov_read_stco },
	{ MKTAG('c','s','l','g'), mov_read_cslg },
	{ MKTAG('c','t','t','s'), mov_read_ctts }, /* composition time to sample*/
	{ MKTAG('d','i','n','f'), mov_read_default },
	{ MKTAG('d','r','e','f'), mov_read_dref },
	{ MKTAG('e','d','t','s'), mov_read_default },
	{ MKTAG('e','l','s','t'), mov_read_elst },
	{ MKTAG('e','n','d','a'), mov_read_enda },
	{ MKTAG('f','i','e','l'), mov_read_extradata },
	{ MKTAG('f','t','y','p'), mov_read_ftyp },
	{ MKTAG('g','l','b','l'), mov_read_glbl },
	{ MKTAG('h','d','l','r'), mov_read_hdlr },
	{ MKTAG('i','l','s','t'), mov_read_ilst },
	{ MKTAG('j','p','2','h'), mov_read_extradata },
	{ MKTAG('m','d','a','t'), mov_read_mdat },
	{ MKTAG('m','d','h','d'), mov_read_mdhd },
	{ MKTAG('m','d','i','a'), mov_read_default },
	{ MKTAG('m','e','t','a'), mov_read_meta },
	{ MKTAG('m','i','n','f'), mov_read_default },
	{ MKTAG('m','o','o','f'), mov_read_moof },
	{ MKTAG('m','o','o','v'), mov_read_moov },
	{ MKTAG('m','v','e','x'), mov_read_default },
	{ MKTAG('m','v','h','d'), mov_read_mvhd },
	{ MKTAG('S','M','I',' '), mov_read_smi }, /* Sorenson extension ???*/
	{ MKTAG('a','l','a','c'), mov_read_extradata }, /* alac specific atom*/
	{ MKTAG('a','v','c','C'), mov_read_glbl },
	{ MKTAG('p','a','s','p'), mov_read_pasp },
	{ MKTAG('s','t','b','l'), mov_read_default },
	{ MKTAG('s','t','c','o'), mov_read_stco },
	{ MKTAG('s','t','p','s'), mov_read_stps },
	{ MKTAG('s','t','s','c'), mov_read_stsc },
	{ MKTAG('s','t','s','d'), mov_read_stsd }, /* sample description*/
	{ MKTAG('s','t','s','s'), mov_read_stss }, /* sync sample*/
	{ MKTAG('s','t','s','z'), mov_read_stsz }, /* sample size*/
	{ MKTAG('s','t','t','s'), mov_read_stts },
	{ MKTAG('s','t','z','2'), mov_read_stsz }, /* compact sample size*/
	{ MKTAG('t','k','h','d'), mov_read_tkhd }, /* track header*/
	{ MKTAG('t','f','h','d'), mov_read_tfhd }, /* track fragment header*/
	{ MKTAG('t','r','a','k'), mov_read_trak },
	{ MKTAG('t','r','a','f'), mov_read_default },
	{ MKTAG('t','r','e','x'), mov_read_trex },
	{ MKTAG('t','r','u','n'), mov_read_trun },
	{ MKTAG('u','d','t','a'), mov_read_default },
	{ MKTAG('w','a','v','e'), mov_read_wave },
	{ MKTAG('e','s','d','s'), mov_read_esds },
	{ MKTAG('w','i','d','e'), mov_read_wide }, /* place holder*/
	{ MKTAG('h','v','c','C'), mov_read_glbl },
	{ MKTAG('c','m','o','v'), mov_read_cmov },
	{ MKTAG('s','a','i','z'), mov_read_saiz },
	{ MKTAG('s','a','i','o'), mov_read_saio },
	{ MKTAG('f','r','m','a'), mov_read_frma },
	{ MKTAG('s','e','n','c'), mov_read_senc },
	{ MKTAG('t','e','n','c'), mov_read_tenc },
	{ MKTAG('s','c','h','i'), mov_read_default },
	{ MKTAG('s','c','h','m'), mov_read_schm },
	{ MKTAG('s','i','n','f'), mov_read_default },
	{ MKTAG('t','f','d','t'), mov_read_tfdt },
	{ MKTAG('s','i','d','x'), mov_read_sidx },
	{ MKTAG('d','O','p','s'), mov_read_dops },
	{ MKTAG('d','v','c','C'), mov_read_dvcc_dvvc },
	{ MKTAG('d','v','v','C'), mov_read_dvcc_dvvc },
	{ MKTAG('d','v','w','C'), mov_read_dvcc_dvvc },
	{ 0, NULL }
};

static int mov_probe(AVProbeData* p)
{
	unsigned int offset;
	uint32_t tag;
	int score = 0;

	/* check file header*/
	offset = 0;
	for(;;) {
		/* ignore invalid offset*/
		if ((offset + 8) > (unsigned int)p->buf_size)
			return score;
		tag = AV_RL32(p->buf + offset + 4);
		switch(tag) {
			/* check for obvious tags*/
		case MKTAG('j','P',' ',' '): /* jpeg 2000 signature*/
		case MKTAG('m','o','o','v'):
		case MKTAG('m','d','a','t'):
		case MKTAG('p','n','o','t'): /* detect movs with preview pics like ew.mov and april.mov*/
		case MKTAG('u','d','t','a'): /* Packet Video PVAuthor adds this and a lot of more junk*/
		case MKTAG('f','t','y','p'):
			return AVPROBE_SCORE_MAX;
			/* those are more common words, so rate then a bit less*/
		case MKTAG('e','d','i','w'): /* xdcam files have reverted first tags*/
		case MKTAG('w','i','d','e'):
		case MKTAG('f','r','e','e'):
		case MKTAG('j','u','n','k'):
		case MKTAG('p','i','c','t'):
			return AVPROBE_SCORE_MAX - 5;
		case MKTAG(0x82,0x82,0x7f,0x7d):
		case MKTAG('s','k','i','p'):
		case MKTAG('u','u','i','d'):
		case MKTAG('p','r','f','l'):
			offset = AV_RB32(p->buf+offset) + offset;
			/* if we only find those cause probedata is too small at least rate them*/
			score = AVPROBE_SCORE_MAX - 50;
			break;
		default:
			/* unrecognized tag*/
			return score;
		}
	}
	return score;
}

static void mov_free_encryption_index(MOVEncryptionIndex **index) {
	int i;
	if (!index || !*index) return;
	for (i = 0; i < (*index)->nb_encrypted_samples; i++) {
		av_encryption_info_free((*index)->encrypted_samples[i]);
	}

	if ((*index)->encrypted_samples)
		av_freep(&(*index)->encrypted_samples);
	if ((*index)->auxiliary_info_sizes)
		av_freep(&(*index)->auxiliary_info_sizes);
	if ((*index)->auxiliary_offsets)
		av_freep(&(*index)->auxiliary_offsets);
	av_freep(index);
}

static int mov_read_close(AVFormatContext* s)
{
	MOVContext* mov = s->priv_data;
	int i, j;

	for (i = 0; i < s->nb_streams; i++) {
		AVStream* st = s->streams[i];
		MOVStreamContext* sc = st->priv_data;

		if (!sc)
			continue;
		av_freep(&sc->ctts_data);
		for (j = 0; j < sc->drefs_count; j++)
			av_freep(&sc->drefs[j].path);
		av_freep(&sc->drefs);
		if (sc->pb && !sc->pb_is_copied)
			ff_format_io_close(s, &sc->pb);
		av_freep(&st->codec->palctrl);
		if(NULL!=sc){
			av_freep((void**)&sc->chunk_offsets);
			av_freep((void**)&sc->stsc_data);
			av_freep((void**)&sc->sample_sizes);
			av_freep((void**)&sc->keyframes);
			av_freep((void**)&sc->stts_data);
			av_freep((void**)&sc->stps_data);
			av_freep((void**)&sc->elst_data);
			mov_free_encryption_index(&sc->cenc.encryption_index);
			av_encryption_info_free(sc->cenc.default_encrypted_sample);
			av_aes_ctr_free(sc->cenc.aes_ctr);
			if (sc->cenc.aes_ctx)
				av_freep(&sc->cenc.aes_ctx);
		}
		av_freep(&st->priv_data);
	}
	s->index_space = 0;
	av_freep(&mov->trex_data);
	if (mov->decryption_key) {
		av_free(mov->decryption_key);
		mov->decryption_key = NULL;
	}

	for (i = 0; i < mov->frag_index.nb_items; i++) {
		MOVFragmentIndexItem * item = &mov->frag_index.item[i];
		if (!item)
			continue;
		MOVFragmentStreamInfo *frag = item->stream_info;
		if (!frag)
			continue;
		for (j = 0; j < item->nb_stream_info; j++) {
			mov_free_encryption_index(&frag[j].encryption_index);
		}
		if (frag)
			av_freep(&frag);
	}
	if (mov->frag_index.item)
		av_freep(&mov->frag_index.item);

	return 0;
}

static int custom_decryption_key_callback(char **decr_key)
{
#define DEFINE_CUSTOM_FLAGS "decr_key"
#define DEFINE_MAX_CUSTOM_NUM 2
	extern GxStreamOptionCallback public_stream_callback;

	if (public_stream_callback) {
		int i = 0, ret = 0, max_custom = 0;;
		GxStreamOptionCallbackParam param;
		if (!(param.custom_str = av_mallocz(DEFINE_MAX_CUSTOM_NUM)))
			return AVERROR(ENOMEM);
		if ((ret = public_stream_callback(&param)) == 0) {
			max_custom = FFMIN(param.custom_num, DEFINE_MAX_CUSTOM_NUM);
			if (!av_strncasecmp(param.custom_flag, DEFINE_CUSTOM_FLAGS, strlen(DEFINE_CUSTOM_FLAGS))) {
				for (i = 0; i < max_custom; i++) {
					*decr_key = av_strdup(param.custom_str[i]);
				}
			}
		}
		av_free(param.custom_str);
		param.custom_str = NULL;
		return 0;
	}
	return AVERROR(EINVAL);
}

static int mov_read_header(AVFormatContext* s, AVFormatParameters *ap)
{
	MOVContext* mov = s->priv_data;
	ByteIOContext* pb = s->pb;
	int err;
	char *decr_key = NULL;
	MOVAtom atom = { AV_RL32("root") };
	AVDictionaryEntry *e = NULL;

	mov->fc = s;
	/* .mov and .mp4 aren't streamable anyway (only progressive download if moov is before mdat)*/
	atom.size = url_fsize(pb);
	mov->is_cenc_decryption = 0;

	if  (custom_decryption_key_callback(&decr_key) >= 0) {
		if (decr_key) {
			int decryption_key_size = (strlen(decr_key)>0)?strlen(decr_key):32;
			mov->decryption_key = av_mallocz(decryption_key_size*sizeof(uint8_t));
			if (!mov->decryption_key) {
				if (decr_key)
					av_freep(&decr_key);
				return AVERROR(ENOMEM);
			}
			mov->decryption_key_size = hex_to_data(mov->decryption_key, decr_key);
			if (decr_key) {
				av_freep(&decr_key);
			}
		}
	}else {
		if ((e = av_dict_get(s->url_options, "decryption_key", NULL, 0))) {
			int decryption_key_size = (strlen(e->value)>0)?strlen(e->value):32;
			mov->decryption_key = av_mallocz(decryption_key_size*sizeof(uint8_t));
			if (!mov->decryption_key) {
			return AVERROR(ENOMEM);
			}
			mov->decryption_key_size = hex_to_data(mov->decryption_key, e->value);
		}
    	}

	/* check MOV header*/
	if ((err = mov_read_default(mov, pb, atom)) < 0) {
		gxlogd( "error reading header: %d\n", err);
		return err;
	}
	if (!mov->found_moov) {
		gxlogd( "moov atom not found\n");
		return -1;
	}

	return 0;
}

static AVIndexEntry* mov_find_next_sample(AVFormatContext *s, AVStream **st)
{
	AVIndexEntry* sample = NULL;
	int64_t best_dts = INT64_MAX;
	int i;
#define MOV_LIMIT_DISTANCE (0x400000)

	for (i = 0; i < s->nb_streams; i++) {
		AVStream* avst = s->streams[i];
		MOVStreamContext* msc = avst->priv_data;
		if (msc->pb && msc->current_sample < avst->nb_index_entries) {

			AVIndexEntry* current_sample =av_find_sample_from_mov(avst,avst->index_entries,msc->current_sample);
			if(!current_sample)continue;

			//AVIndexEntry* current_sample = &avst->index_entries[msc->current_sample];
			int64_t dts = av_rescale(current_sample->timestamp, AV_TIME_BASE, msc->time_scale);
			if (sample == NULL ||
					((s->file_format == GX_STREAMTYPE_STREAM) &&
					 (current_sample->pos < sample->pos)) ||
					((s->file_format != GX_STREAMTYPE_STREAM) &&
					 ((((sample->pos - current_sample->pos <= MOV_LIMIT_DISTANCE) && (!s->get_packet_bypts)) &&
					   (current_sample->pos < sample->pos)) ||
					  (((abs(sample->pos - current_sample->pos) > MOV_LIMIT_DISTANCE) || s->get_packet_bypts) &&
					   (((msc->pb != s->pb && dts < best_dts) ||
						 (msc->pb == s->pb &&
						  (((FFABS(best_dts - dts) <= AV_TIME_BASE) && (current_sample->pos < sample->pos)) ||
						  ((FFABS(best_dts - dts) > AV_TIME_BASE) && dts < best_dts))))))))) {
				sample = current_sample;
				best_dts = dts;
				* st = avst;
			}
		}
	}
	return sample;
}

static int cenc_scheme_decrypt(MOVContext *c, MOVStreamContext *sc, AVEncryptionInfo *sample, uint8_t *input, int size)
{
	int i, ret;
	int bytes_of_protected_data;
	int partially_encrypted_block_size;
	uint8_t *partially_encrypted_block = NULL;
	uint8_t block[16];

	if (!sc->cenc.aes_ctr) {
		/* initialize the cipher */
		sc->cenc.aes_ctr = av_aes_ctr_alloc();
		if (!sc->cenc.aes_ctr) {
			return AVERROR(ENOMEM);
		}
		ret = av_aes_ctr_init(sc->cenc.aes_ctr, c->decryption_key);
		if (ret < 0) {
			gxloge ("aes init fail. ret=%d.\n", ret);
			return ret;
		}
	}
	av_aes_ctr_set_full_iv(sc->cenc.aes_ctr, sample->iv);

	if (!sample->subsample_count) {
		/* decrypt the whole packet */
		av_aes_ctr_crypt(sc->cenc.aes_ctr, input, input, size);
		return 0;
	}
	partially_encrypted_block_size = 0;
	for (i = 0; i < sample->subsample_count; i++) {
		if (sample->subsamples[i].bytes_of_clear_data + sample->subsamples[i].bytes_of_protected_data > size) {
			gxloge ("subsample size exceeds the packet size left\n");
			return AVERROR_INVALIDDATA;
		}

		/* skip the clear bytes */
		input += sample->subsamples[i].bytes_of_clear_data;
		size -= sample->subsamples[i].bytes_of_clear_data;

		/* decrypt the encrypted bytes */

		if (partially_encrypted_block_size) {
			memcpy(block, partially_encrypted_block, partially_encrypted_block_size);
			memcpy(block+partially_encrypted_block_size, input, 16-partially_encrypted_block_size);
			av_aes_ctr_crypt(sc->cenc.aes_ctr, block, block, 16);
			memcpy(partially_encrypted_block, block, partially_encrypted_block_size);
			memcpy(input, block+partially_encrypted_block_size, 16-partially_encrypted_block_size);
			input += 16-partially_encrypted_block_size;
			size -= 16-partially_encrypted_block_size;
			bytes_of_protected_data = sample->subsamples[i].bytes_of_protected_data - (16-partially_encrypted_block_size);
		} else {
			bytes_of_protected_data = sample->subsamples[i].bytes_of_protected_data;
		}

		if (i < sample->subsample_count-1) {
			int num_of_encrypted_blocks = bytes_of_protected_data/16;
			partially_encrypted_block_size = bytes_of_protected_data%16;
			if (partially_encrypted_block_size)
				partially_encrypted_block = input + 16*num_of_encrypted_blocks;
			av_aes_ctr_crypt(sc->cenc.aes_ctr, input, input, 16*num_of_encrypted_blocks);
		} else {
			av_aes_ctr_crypt(sc->cenc.aes_ctr, input, input, bytes_of_protected_data);
		}

		input += bytes_of_protected_data;
		size -= bytes_of_protected_data;
	}
	if (size > 0) {
		gxloge ("leftover packet bytes after subsample processing\n");
		return AVERROR_INVALIDDATA;
	}

	return 0;
}

static int cbc1_scheme_decrypt(MOVContext *c, MOVStreamContext *sc, AVEncryptionInfo *sample, uint8_t *input, int size)
{
    int i, ret;
    int num_of_encrypted_blocks;
    uint8_t iv[16];

    if (!sc->cenc.aes_ctx) {
        /* initialize the cipher */
        sc->cenc.aes_ctx = av_aes_alloc();
        if (!sc->cenc.aes_ctx) {
            return AVERROR(ENOMEM);
        }

        ret = av_aes_init(sc->cenc.aes_ctx, c->decryption_key, 16 * 8, 1);
        if (ret < 0) {
            return ret;
        }
    }

    memcpy(iv, sample->iv, 16);

    /* whole-block full sample encryption */
    if (!sample->subsample_count) {
        /* decrypt the whole packet */
        av_aes_crypt(sc->cenc.aes_ctx, input, input, size/16, iv, 1);
        return 0;
    }

    for (i = 0; i < sample->subsample_count; i++) {
        if (sample->subsamples[i].bytes_of_clear_data + sample->subsamples[i].bytes_of_protected_data > size) {
            gxloge ("subsample size exceeds the packet size left\n");
            return AVERROR_INVALIDDATA;
        }

        if (sample->subsamples[i].bytes_of_protected_data % 16) {
            gxloge ("subsample BytesOfProtectedData is not a multiple of 16\n");
            return AVERROR_INVALIDDATA;
        }

        /* skip the clear bytes */
        input += sample->subsamples[i].bytes_of_clear_data;
        size -= sample->subsamples[i].bytes_of_clear_data;

        /* decrypt the encrypted bytes */
        num_of_encrypted_blocks = sample->subsamples[i].bytes_of_protected_data/16;
        if (num_of_encrypted_blocks > 0) {
            av_aes_crypt(sc->cenc.aes_ctx, input, input, num_of_encrypted_blocks, iv, 1);
        }
        input += sample->subsamples[i].bytes_of_protected_data;
        size -= sample->subsamples[i].bytes_of_protected_data;
    }

    if (size > 0) {
        gxloge ("leftover packet bytes after subsample processing\n");
        return AVERROR_INVALIDDATA;
    }

    return 0;
}

static int cens_scheme_decrypt(MOVContext *c, MOVStreamContext *sc, AVEncryptionInfo *sample, uint8_t *input, int size)
{
    int i, ret, rem_bytes;
    uint8_t *data;

    if (!sc->cenc.aes_ctr) {
        /* initialize the cipher */
        sc->cenc.aes_ctr = av_aes_ctr_alloc();
        if (!sc->cenc.aes_ctr) {
            return AVERROR(ENOMEM);
        }

        ret = av_aes_ctr_init(sc->cenc.aes_ctr, c->decryption_key);
        if (ret < 0) {
            return ret;
        }
    }

    av_aes_ctr_set_full_iv(sc->cenc.aes_ctr, sample->iv);

    /* whole-block full sample encryption */
    if (!sample->subsample_count) {
        /* decrypt the whole packet */
        av_aes_ctr_crypt(sc->cenc.aes_ctr, input, input, size);
        return 0;
    } else if (!sample->crypt_byte_block && !sample->skip_byte_block) {
        gxloge ("pattern encryption is not present in 'cens' scheme\n");
        return AVERROR_INVALIDDATA;
    }

    for (i = 0; i < sample->subsample_count; i++) {
        if (sample->subsamples[i].bytes_of_clear_data + sample->subsamples[i].bytes_of_protected_data > size) {
            gxloge ("subsample size exceeds the packet size left\n");
            return AVERROR_INVALIDDATA;
        }

        /* skip the clear bytes */
        input += sample->subsamples[i].bytes_of_clear_data;
        size -= sample->subsamples[i].bytes_of_clear_data;

        /* decrypt the encrypted bytes */
        data = input;
        rem_bytes = sample->subsamples[i].bytes_of_protected_data;
        while (rem_bytes > 0) {
            if (rem_bytes < 16*sample->crypt_byte_block) {
                break;
            }
            av_aes_ctr_crypt(sc->cenc.aes_ctr, data, data, 16*sample->crypt_byte_block);
            data += 16*sample->crypt_byte_block;
            rem_bytes -= 16*sample->crypt_byte_block;
            data += FFMIN(16*sample->skip_byte_block, rem_bytes);
            rem_bytes -= FFMIN(16*sample->skip_byte_block, rem_bytes);
        }
        input += sample->subsamples[i].bytes_of_protected_data;
        size -= sample->subsamples[i].bytes_of_protected_data;
    }

    if (size > 0) {
        gxloge ("leftover packet bytes after subsample processing\n");
        return AVERROR_INVALIDDATA;
    }

    return 0;
}

static int cbcs_scheme_decrypt(MOVContext *c, MOVStreamContext *sc, AVEncryptionInfo *sample, uint8_t *input, int size)
{
    int i, ret, rem_bytes;
    uint8_t iv[16];
    uint8_t *data;

    if (!sc->cenc.aes_ctx) {
        /* initialize the cipher */
        sc->cenc.aes_ctx = av_aes_alloc();
        if (!sc->cenc.aes_ctx) {
            return AVERROR(ENOMEM);
        }

        ret = av_aes_init(sc->cenc.aes_ctx, c->decryption_key, 16 * 8, 1);
        if (ret < 0) {
            return ret;
        }
    }

    /* whole-block full sample encryption */
    if (!sample->subsample_count) {
        /* decrypt the whole packet */
        memcpy(iv, sample->iv, 16);
        av_aes_crypt(sc->cenc.aes_ctx, input, input, size/16, iv, 1);
        return 0;
    } else if (!sample->crypt_byte_block && !sample->skip_byte_block) {
        gxloge ("pattern encryption is not present in 'cbcs' scheme\n");
        return AVERROR_INVALIDDATA;
    }

    for (i = 0; i < sample->subsample_count; i++) {
        if (sample->subsamples[i].bytes_of_clear_data + sample->subsamples[i].bytes_of_protected_data > size) {
            gxloge ("subsample size exceeds the packet size left\n");
            return AVERROR_INVALIDDATA;
        }

        /* skip the clear bytes */
        input += sample->subsamples[i].bytes_of_clear_data;
        size -= sample->subsamples[i].bytes_of_clear_data;

        /* decrypt the encrypted bytes */
        memcpy(iv, sample->iv, 16);
        data = input;
        rem_bytes = sample->subsamples[i].bytes_of_protected_data;
        while (rem_bytes > 0) {
            if (rem_bytes < 16*sample->crypt_byte_block) {
                break;
            }
            av_aes_crypt(sc->cenc.aes_ctx, data, data, sample->crypt_byte_block, iv, 1);
            data += 16*sample->crypt_byte_block;
            rem_bytes -= 16*sample->crypt_byte_block;
            data += FFMIN(16*sample->skip_byte_block, rem_bytes);
            rem_bytes -= FFMIN(16*sample->skip_byte_block, rem_bytes);
        }
        input += sample->subsamples[i].bytes_of_protected_data;
        size -= sample->subsamples[i].bytes_of_protected_data;
    }

    if (size > 0) {
        gxloge ("leftover packet bytes after subsample processing\n");
        return AVERROR_INVALIDDATA;
    }

    return 0;
}

static int mov_cenc_scheme_decrypt(MOVContext *c, MOVStreamContext *sc, AVEncryptionInfo *sample, uint8_t *input, int size)
{
	gxloge ("Not support!!for application layer,call GxPlayer_ModuleRegisterCENCSchemeDecrypt().\n");
	return AVERROR_NOTSUPP;
}

static int (*scheme_decrypt[4])(MOVContext *c, MOVStreamContext *sc, AVEncryptionInfo *sample, uint8_t *input, int  size) = {mov_cenc_scheme_decrypt, mov_cenc_scheme_decrypt, mov_cenc_scheme_decrypt, mov_cenc_scheme_decrypt};
void av_cenc_scheme_decrypt_regiseter(void)
{
	scheme_decrypt[CENC_DECRYPT] = cenc_scheme_decrypt;
	scheme_decrypt[CBC1_DECRYPT] = cbc1_scheme_decrypt;
	scheme_decrypt[CENS_DECRYPT] = cens_scheme_decrypt;
	scheme_decrypt[CBCS_DECRYPT] = cbcs_scheme_decrypt;
}

static int cenc_decrypt(AVFormatContext* s, MOVContext *c, MOVStreamContext *sc, AVEncryptionInfo *sample, uint8_t *input, int size)
{
    if (sample->scheme == MKBETAG('c','e','n','c') && !sample->crypt_byte_block && !sample->skip_byte_block) {
        c->is_cenc_decryption = 1;
        return (*scheme_decrypt[CENC_DECRYPT])(c, sc, sample, input, size);
    } else if (sample->scheme == MKBETAG('c','b','c','1') && !sample->crypt_byte_block && !sample->skip_byte_block) {
        c->is_cenc_decryption = 1;
        return (*scheme_decrypt[CBC1_DECRYPT])(c, sc, sample, input, size);
    } else if (sample->scheme == MKBETAG('c','e','n','s')) {
        c->is_cenc_decryption = 1;
        return (*scheme_decrypt[CENS_DECRYPT])(c, sc, sample, input, size);
    } else if (sample->scheme == MKBETAG('c','b','c','s')) {
        c->is_cenc_decryption = 1;
        return (*scheme_decrypt[CBCS_DECRYPT])(c, sc, sample, input, size);
    } else {
        gxloge ("invalid encryption scheme\n");
        c->is_cenc_decryption = 0;
        return AVERROR_INVALIDDATA;
    }
}

static int cenc_filter(AVFormatContext* s, AVStream* st,
		MOVContext *mov, MOVStreamContext *sc, AVPacket *pkt, int current_index)
{
	MOVFragmentStreamInfo *frag_stream_info;
	MOVEncryptionIndex *encryption_index = NULL;
	AVEncryptionInfo *encrypted_sample;
	int encrypted_index;

	frag_stream_info = get_frag_stream_info(&mov->frag_index, mov->frag_index.current, st->id);
	encrypted_index = current_index;
	if (frag_stream_info) {
		// Note this only supports encryption info in the first sample descriptor.
		if (mov->fragment.stsd_id == 1) {
			if (frag_stream_info->encryption_index) {
				if (!current_index && frag_stream_info->index_entry)
					sc->cenc.frag_index_entry_base = frag_stream_info->index_entry;
				encrypted_index = current_index - (frag_stream_info->index_entry - sc->cenc.frag_index_entry_base);
				encryption_index = frag_stream_info->encryption_index;
			} else {
				encryption_index = sc->cenc.encryption_index;
			}
		}
	} else {
		encryption_index = sc->cenc.encryption_index;
	}

	if (encryption_index) {
		if (encryption_index->auxiliary_info_sample_count &&
				!encryption_index->nb_encrypted_samples) {
			gxloge ("saiz atom found without saio\n");
			return AVERROR_INVALIDDATA;
		}
		if (encryption_index->auxiliary_offsets_count &&
				!encryption_index->nb_encrypted_samples) {
			gxloge ("saio atom found without saiz\n");
			return AVERROR_INVALIDDATA;
		}

		if (!encryption_index->nb_encrypted_samples) {
			// Full-sample encryption with default settings.
			encrypted_sample = sc->cenc.default_encrypted_sample;
		} else if (encrypted_index >= 0 && encrypted_index < encryption_index->nb_encrypted_samples) {
			// Per-sample setting override.
			encrypted_sample = encryption_index->encrypted_samples[encrypted_index];
		} else {
			gxloge ("Incorrect number of samples in encryption info\n");
			return AVERROR_INVALIDDATA;
		}

		if (mov->decryption_key_size) {
			return cenc_decrypt(s, mov, sc, encrypted_sample, pkt->data, pkt->size);
		}
	}

	return 0;
}

static int mov_read_packet(AVFormatContext* s, AVPacket *pkt)
{
	MOVContext* mov = s->priv_data;
	MOVStreamContext* sc;
	AVIndexEntry* sample=NULL;
	AVStream* st = NULL;
	int ret;
	int64_t ret64;
retry:
	if(url_check_interrupt_cb())
		return AVERROR_EXIT;

	sample = mov_find_next_sample(s, &st);
	if (!sample) {
		mov->found_mdat = 0;
		if (!mov->next_root_atom) {
			if (s->pb->error == AVERROR_RSTRT || s->pb->error == AVERROR_NEXT)
				return s->pb->error;
			return AVERROR_EOF;
		}
		ret64 = url_fseek(s->pb, mov->next_root_atom, SEEK_SET);
		mov->next_root_atom = 0;
		ret = mov_read_default(mov, s->pb, (MOVAtom){ AV_RL32("root"), 0, INT64_MAX });
		if (ret < 0) {
			return ret;
		}
		if (url_feof(s->pb)) {
			if (s->pb->error == AVERROR_RSTRT || s->pb->error == AVERROR_NEXT)
				return s->pb->error;
			return AVERROR_EOF;
		}
		goto retry;
	}
	sc = st->priv_data;
	/* must be done just before reading, to avoid infinite loop on sample*/
	sc->current_sample++;
	if (mov->next_root_atom) {
		sample->pos = FFMIN(sample->pos, mov->next_root_atom);
		sample->size = FFMIN(sample->size, (mov->next_root_atom - sample->pos));
	}

	if (st->discard != AVDISCARD_ALL) {
		int64_t ret64 = avio_seek(sc->pb, sample->pos, SEEK_SET);
		if (ret64 != sample->pos) {
			return -1;
		}
		ret = av_get_packet(sc->pb, pkt, sample->size);
		if (ret < 0){
			return ret;
		}
	}

	pkt->stream_index = sc->ffindex;
	pkt->dts = sample->timestamp;
	pkt->flags |= sample->flags & AVINDEX_KEYFRAME ? PKT_FLAG_KEY : 0;
	pkt->pos = sample->pos;
	if (sc->ctts_data) {
		MOVCtts_32 ctts = {0, 0};
		get_ctts_by_index(sc, sc->ctts_index, &ctts);
		pkt->pts = pkt->dts + sc->dts_shift + ctts.duration;
		/* update ctts context*/
		sc->ctts_sample++;
		if ((sc->ctts_index < sc->ctts_count) &&
				(ctts.count == sc->ctts_sample)) {
			sc->ctts_index++;
			sc->ctts_sample = 0;
		}
		if (sc->wrong_dts)
			pkt->dts = AV_NOPTS_VALUE;
	} else {
		int64_t next_dts=0;
		AVIndexEntry* sampletmp=NULL;
		if(sc->current_sample < st->nb_index_entries){
			sampletmp=av_find_sample_from_mov(st,st->index_entries,sc->current_sample);
			if(sampletmp){
				next_dts=sampletmp->timestamp;
			}else{
				return -1;//rebuild index is error	may be
			}

		}else{
			next_dts=st->duration;
		}
		pkt->duration = next_dts - pkt->dts;
		pkt->pts = pkt->dts;
	}

	if (st->discard == AVDISCARD_ALL)
		goto retry;

	ret = cenc_filter(s, st, mov, sc, pkt, sc->current_sample - 1);
	if (ret < 0)
		return ret;

	return 0;
}

#define MOV_FIND_TIME 0
#define MOV_FIND_POS  1

static int64_t get_stream_info_time(MOVFragmentStreamInfo * frag_stream_info)
{
    av_assert0(frag_stream_info);
    if (frag_stream_info->sidx_pts != AV_NOPTS_VALUE)
        return frag_stream_info->sidx_pts;
    if (frag_stream_info->first_tfra_pts != AV_NOPTS_VALUE)
        return frag_stream_info->first_tfra_pts;
    return frag_stream_info->tfdt_dts;
}

static int64_t get_frag_time(MOVFragmentIndex *frag_index,
                             int index, int track_id)
{
    MOVFragmentStreamInfo * frag_stream_info;
    int64_t timestamp;
    int i;

    if (track_id >= 0) {
        frag_stream_info = get_frag_stream_info(frag_index, index, track_id);
        return frag_stream_info->sidx_pts;
    }

    for (i = 0; i < frag_index->item[index].nb_stream_info; i++) {
        frag_stream_info = &frag_index->item[index].stream_info[i];
        timestamp = get_stream_info_time(frag_stream_info);
        if (timestamp != AV_NOPTS_VALUE)
            return timestamp;
    }
    return AV_NOPTS_VALUE;
}

static int search_frag_timestamp(MOVFragmentIndex *frag_index,
                                 AVStream *st, int64_t timestamp)
{
    int a, b, m, m0;
    int64_t frag_time;
    int id = -1;

    if (st) {
        // If the stream is referenced by any sidx, limit the search
        // to fragments that referenced this stream in the sidx
        MOVStreamContext *sc = st->priv_data;
        if (sc->has_sidx)
            id = st->id;
    }

    a = -1;
    b = frag_index->nb_items;

    while (b - a > 1) {
        m0 = m = (a + b) >> 1;

        while (m < b &&
               (frag_time = get_frag_time(frag_index, m, id)) == AV_NOPTS_VALUE)
            m++;

        if (m < b && frag_time <= timestamp)
            a = m;
        else
            b = m0;
    }

    return a;
}

static int mov_switch_root(AVFormatContext *s, int64_t target, int index)
{
    int ret;
    MOVContext *mov = s->priv_data;

    if (index >= 0 && index < mov->frag_index.nb_items)
        target = mov->frag_index.item[index].moof_offset;
#if 0
    if (avio_seek(s->pb, target, SEEK_SET) != target) {
        gxlogd ("root atom offset %lld partial file\n", target);
        return AVERROR_INVALIDDATA;
    }
#endif
    mov->next_root_atom = 0;
    if (index < 0 || index >= mov->frag_index.nb_items)
        index = search_frag_moof_offset(&mov->frag_index, target);
    if (index < mov->frag_index.nb_items) {
        if (index + 1 < mov->frag_index.nb_items)
            mov->next_root_atom = mov->frag_index.item[s->ass_is_forward?index:(index + 1)].moof_offset;
        if (mov->frag_index.item[index].headers_read)
            return 0;
        mov->frag_index.item[index].headers_read = 1;
    }

    mov->found_mdat = 0;

    ret = mov_read_default(mov, s->pb, (MOVAtom){ AV_RL32("root"), INT64_MAX });
    if (ret < 0)
        return ret;
    if (avio_feof(s->pb))
        return AVERROR_EOF;
    //av_log(s, AV_LOG_TRACE, "read fragments, offset 0x%"PRIx64"\n", avio_tell(s->pb));

    return 1;
}

static int mov_seek_fragment(AVFormatContext *s, AVStream *st, int64_t timestamp)
{
    MOVContext *mov = s->priv_data;
    int index;

    if (!mov->frag_index.complete)
        return 0;

    index = search_frag_timestamp(&mov->frag_index, st, timestamp);
    if (index < 0)
        index = 0;
    if (timestamp<= 0 && s->ass_is_forward && mov->frag_index.item[index].headers_read)
        mov->frag_index.item[index].headers_read = 0;
    if (!mov->frag_index.item[index].headers_read)
        return mov_switch_root(s, -1, index);
    if (index + 1 < mov->frag_index.nb_items)
        mov->next_root_atom = mov->frag_index.item[index + 1].moof_offset;

    return 0;
}

static int mov_seek_stream(AVFormatContext* s, AVStream* st, int cmd, int64_t timestamp, int flags)
{
	MOVStreamContext* sc = st->priv_data;
	int sample = -1, time_sample, ret;
	int i;
	//gxlogi("###current_sample==>%d==>%lld###\n",sc->current_sample,st->index_entries[sc->current_sample-st->index_entries_start].timestamp);
	ret = mov_seek_fragment(s, st, timestamp);
	if (ret < 0)
		return ret;

	if(cmd == MOV_FIND_TIME){
		sample = av_index_search_timestamp_mov(st, timestamp, flags, s->ass_is_forward);
	} else if(cmd == MOV_FIND_POS){
		sample = av_index_search_pos_mov(st, timestamp, flags);
		if(sample == -1) sample = 0;
	}
	if (sample < 0) /* not sure what to do*/
		return -1;
	sc->current_sample = sample;

	/* adjust ctts index*/
	if (sc->ctts_data) {
		time_sample = 0;
		for (i = 0; i < sc->ctts_count; i++) {
			MOVCtts_32 ctts = {0,0};
			int next = 0;
			get_ctts_by_index(sc, i, &ctts);
			next = time_sample + ctts.count;
			if (next > sc->current_sample) {
				sc->ctts_index = i;
				sc->ctts_sample = sc->current_sample - time_sample;
				break;
			}
			time_sample = next;
		}
	}
	return sample;
}

static int mov_read_seek(AVFormatContext* s, int stream_index, int64_t sample_time, int flags)
{
	AVStream* st;
	int sample;
	int i;

	if (stream_index >= s->nb_streams)
		return -1;
	if (sample_time < 0)
		sample_time = 0;

	st = s->streams[stream_index];
	if(s->file_format == GX_STREAMTYPE_STREAM){
		int64_t seek_pos;
		sample = mov_seek_stream(s, st, MOV_FIND_TIME, sample_time, AVSEEK_FLAG_BACKWARD);
		if (sample < 0)
			return -1;
		/* adjust seek timestamp to found sample timestamp*/
		seek_pos = st->index_entries[sample-st->index_entries_start].pos;
		for (i = 0; i < s->nb_streams; i++) {
			st = s->streams[i];
			if (stream_index == i)
				continue;
			sample = mov_seek_stream(s, st, MOV_FIND_POS, seek_pos, AVSEEK_FLAG_ANY|AVSEEK_FLAG_BACKWARD);
		}
	}else{
		int64_t seek_timestamp, timestamp;
		sample = mov_seek_stream(s, st, MOV_FIND_TIME, sample_time, flags);
		if (sample < 0)
			return -1;
		/* adjust seek timestamp to found sample timestamp*/
		seek_timestamp = st->index_entries[sample-st->index_entries_start].timestamp;
		for (i = 0; i < s->nb_streams; i++) {
			st = s->streams[i];
			if(st->codec){
				if(st->codec->codec_type!=CODEC_TYPE_VIDEO
						&& st->codec->codec_type!=CODEC_TYPE_AUDIO
						&& st->codec->codec_type!=CODEC_TYPE_SUBTITLE)
					continue;
			}
			if (stream_index == i)
				continue;
			timestamp = av_rescale_q(seek_timestamp, s->streams[stream_index]->time_base, st->time_base);
			sample=mov_seek_stream(s, st, MOV_FIND_TIME, timestamp, flags);
			if (sample < 0){
				if (st->codec && st->codec->codec_type == CODEC_TYPE_SUBTITLE)
					return 0;
				return -1;
			}
		}
	}
	return 0;
}

AVInputFormat mov_demuxer = {
	.name           = "mov",
	.priv_data_size = sizeof(MOVStreamContext),
	.read_probe     = mov_probe,
	.read_header    = mov_read_header,
	.read_packet    = mov_read_packet,
	.read_close     = mov_read_close,
	.read_seek      = mov_read_seek,
	.extensions     = "mov",
	.value          = CODEC_ID_MOV_TEXT,
};

