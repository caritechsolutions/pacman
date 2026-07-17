#include "demux_audio.h"

static int tabsel_123[2][3][16] = {
	{{0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0},
	 {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0},
	 {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0}},

	{{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0},
	 {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},
	 {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0}}
};

static long freqs[9] = { 44100, 48000, 32000,	// MPEG 1.0
	22050, 24000, 16000,	// MPEG 2.0
	11025, 12000, 8000
};				// MPEG 2.5

#if 0
static char* genres[] = {
	"Blues",		/* 0*/
	"Classic Rock",		/* 1*/
	"Country",		/* 2*/
	"Dance",		/* 3*/
	"Disco",		/* 4*/
	"Funk",			/* 5*/
	"Grunge",		/* 6*/
	"Hip-Hop",		/* 7*/
	"Jazz",			/* 8*/
	"Metal",		/* 9*/
	"New Age",		/* 10*/
	"Oldies",		/* 11*/
	"Other",		/* 12*/
	"Pop",			/* 13*/
	"R&B",			/* 14*/
	"Rap",			/* 15*/
	"Reggae",		/* 16*/
	"Rock",			/* 17*/
	"Techno",		/* 18*/
	"Industrial",		/* 19*/
	"Alternative",		/* 20*/
	"Ska",			/* 21*/
	"Death Metal",		/* 22*/
	"Pranks",		/* 23*/
	"Soundtrack",		/* 24*/
	"Eurotechno",		/* 25*/
	"Ambient",		/* 26*/
	"Trip-Hop",		/* 27*/
	"Vocal",		/* 28*/
	"Jazz+Funk",		/* 29*/
	"Fusion",		/* 30*/
	"Trance",		/* 31*/
	"Classical",		/* 32*/
	"Instrumental",		/* 33*/
	"Acid",			/* 34*/
	"House",		/* 35*/
	"Game",			/* 36*/
	"Sound Clip",		/* 37*/
	"Gospel",		/* 38*/
	"Noise",		/* 39*/
	"Alternative Rock",	/* 40*/
	"Bass",			/* 41*/
	"Soul",			/* 42*/
	"Punk",			/* 43*/
	"Space",		/* 44*/
	"Meditative",		/* 45*/
	"Instrumental Pop",	/* 46*/
	"Instrumental Rock",	/* 47*/
	"Ethnic",		/* 48*/
	"Gothic",		/* 49*/
	"Darkwave",		/* 50*/
	"Techno-Industrial",	/* 51*/
	"Electronic",		/* 52*/
	"Jungle",		/* 53*/
	"Pop-Folk",		/* 54*/
	"Eurodance",		/* 55*/
	"Dream",		/* 56*/
	"Southern Rock",	/* 57*/
	"Comedy",		/* 58*/
	"Cult",			/* 59*/
	"Gangsta",		/* 60*/
	"Top 40",		/* 61*/
	"Christian Rap",	/* 62*/
	"Pop/Funk",		/* 63*/
	"Native American",	/* 64*/
	"Cabaret",		/* 65*/
	"New Wave",		/* 66*/
	"Psychadelic",		/* 67*/
	"Rave",			/* 68*/
	"Show Tunes",		/* 69*/
	"Trailer",		/* 70*/
	"Lo-Fi",		/* 71*/
	"Tribal",		/* 72*/
	"Acid Punk",		/* 73*/
	"Acid Jazz",		/* 74*/
	"Polka",		/* 75*/
	"Retro",		/* 76*/
	"Musical",		/* 77*/
	"Rock & Roll",		/* 78*/
	"Hard Rock",		/* 79*/
	"Folk",			/* 80*/
	"Folk/Rock",		/* 81*/
	"National Folk",	/* 82*/
	"Swing",		/* 83*/
	"Fast-Fusion",		/* 84*/
	"Bebop",		/* 85*/
	"Latin",		/* 86*/
	"Revival",		/* 87*/
	"Celtic",		/* 88*/
	"Bluegrass",		/* 89*/
	"Avantgarde",		/* 90*/
	"Gothic Rock",		/* 91*/
	"Progressive Rock",	/* 92*/
	"Psychedelic Rock",	/* 93*/
	"Symphonic Rock",	/* 94*/
	"Slow Rock",		/* 95*/
	"Big Band",		/* 96*/
	"Chorus",		/* 97*/
	"Easy Listening",	/* 98*/
	"Acoustic",		/* 99*/
	"Humour",		/* 100*/
	"Speech",		/* 101*/
	"Chanson",		/* 102*/
	"Opera",		/* 103*/
	"Chamber Music",	/* 104*/
	"Sonata",		/* 105*/
	"Symphony",		/* 106*/
	"Booty Bass",		/* 107*/
	"Primus",		/* 108*/
	"Porn Groove",		/* 109*/
	"Satire",		/* 110*/
	"Slow Jam",		/* 111*/
	"Club",			/* 112*/
	"Tango",		/* 113*/
	"Samba",		/* 114*/
	"Folklore",		/* 115*/
	"Ballad",		/* 116*/
	"Power Ballad",		/* 117*/
	"Rhytmic Soul",		/* 118*/
	"Freestyle",		/* 119*/
	"Duet",			/* 120*/
	"Punk Rock",		/* 121*/
	"Drum Solo",		/* 122*/
	"Acapella",		/* 123*/
	"Euro-House",		/* 124*/
	"Dance Hall",		/* 125*/
	"Goa",			/* 126*/
	"Drum & Bass",		/* 127*/
	"Club-House",		/* 128*/
	"Hardcore",		/* 129*/
	"Terror",		/* 130*/
	"Indie",		/* 131*/
	"BritPop",		/* 132*/
	"Negerpunk",		/* 133*/
	"Polsk Punk",		/* 134*/
	"Beat",			/* 135*/
	"Christian Gangsta Rap",	/* 136*/
	"Heavy Metal",		/* 137*/
	"Black Metal",		/* 138*/
	"Crossover",		/* 139*/
	"Contemporary Christian",	/* 140*/
	"Christian Rock",	/* 141*/
	"Merengue",		/* 142*/
	"Salsa",		/* 143*/
	"Thrash Metal",		/* 144*/
	"Anime",		/* 145*/
	"Jpop",			/* 146*/
	"Synthpop",		/* 147*/
	"Unknown",		/* 148*/
	"Unknown",		/* 149*/
	"Unknown",		/* 150*/
	"Unknown",		/* 151*/
	"Unknown",		/* 152*/
	"Unknown",		/* 153*/
	"Unknown",		/* 154*/
	"Unknown",		/* 155*/
	"Unknown",		/* 156*/
	"Unknown",		/* 157*/
	"Unknown",		/* 158*/
	"Unknown",		/* 159*/
	"Unknown",		/* 160*/
	"Unknown",		/* 161*/
	"Unknown",		/* 162*/
	"Unknown",		/* 163*/
	"Unknown",		/* 164*/
	"Unknown",		/* 165*/
	"Unknown",		/* 166*/
	"Unknown",		/* 167*/
	"Unknown",		/* 168*/
	"Unknown",		/* 169*/
	"Unknown",		/* 170*/
	"Unknown",		/* 171*/
	"Unknown",		/* 172*/
	"Unknown",		/* 173*/
	"Unknown",		/* 174*/
	"Unknown",		/* 175*/
	"Unknown",		/* 176*/
	"Unknown",		/* 177*/
	"Unknown",		/* 178*/
	"Unknown",		/* 179*/
	"Unknown",		/* 180*/
	"Unknown",		/* 181*/
	"Unknown",		/* 182*/
	"Unknown",		/* 183*/
	"Unknown",		/* 184*/
	"Unknown",		/* 185*/
	"Unknown",		/* 186*/
	"Unknown",		/* 187*/
	"Unknown",		/* 188*/
	"Unknown",		/* 189*/
	"Unknown",		/* 190*/
	"Unknown",		/* 191*/
	"Unknown",		/* 192*/
	"Unknown",		/* 193*/
	"Unknown",		/* 194*/
	"Unknown",		/* 195*/
	"Unknown",		/* 196*/
	"Unknown",		/* 197*/
	"Unknown",		/* 198*/
	"Unknown",		/* 199*/
	"Unknown",		/* 200*/
	"Unknown",		/* 201*/
	"Unknown",		/* 202*/
	"Unknown",		/* 203*/
	"Unknown",		/* 204*/
	"Unknown",		/* 205*/
	"Unknown",		/* 206*/
	"Unknown",		/* 207*/
	"Unknown",		/* 208*/
	"Unknown",		/* 209*/
	"Unknown",		/* 210*/
	"Unknown",		/* 211*/
	"Unknown",		/* 212*/
	"Unknown",		/* 213*/
	"Unknown",		/* 214*/
	"Unknown",		/* 215*/
	"Unknown",		/* 216*/
	"Unknown",		/* 217*/
	"Unknown",		/* 218*/
	"Unknown",		/* 219*/
	"Unknown",		/* 220*/
	"Unknown",		/* 221*/
	"Unknown",		/* 222*/
	"Unknown",		/* 223*/
	"Unknown",		/* 224*/
	"Unknown",		/* 225*/
	"Unknown",		/* 226*/
	"Unknown",		/* 227*/
	"Unknown",		/* 228*/
	"Unknown",		/* 229*/
	"Unknown",		/* 230*/
	"Unknown",		/* 231*/
	"Unknown",		/* 232*/
	"Unknown",		/* 233*/
	"Unknown",		/* 234*/
	"Unknown",		/* 235*/
	"Unknown",		/* 236*/
	"Unknown",		/* 237*/
	"Unknown",		/* 238*/
	"Unknown",		/* 239*/
	"Unknown",		/* 240*/
	"Unknown",		/* 241*/
	"Unknown",		/* 242*/
	"Unknown",		/* 243*/
	"Unknown",		/* 244*/
	"Unknown",		/* 245*/
	"Unknown",		/* 246*/
	"Unknown",		/* 247*/
	"Unknown",		/* 248*/
	"Unknown",		/* 249*/
	"Unknown",		/* 250*/
	"Unknown",		/* 251*/
	"Unknown",		/* 252*/
	"Unknown",		/* 253*/
	"Unknown",		/* 254*/
	"Unknown",		/* 255*/
};
#endif

typedef struct {
	unsigned int format;
	unsigned int tag;
} WavAudioTags;

static WavAudioTags audio_codec_wav_tags[] = {
	{PLAYER_ACODEC_DVDPCM, 0x0001},
	{PLAYER_ACODEC_DVDPCM, 0x0002},
	{PLAYER_ACODEC_DVDPCM, 0x0003},
	{PLAYER_ACODEC_MPG2,   0x0050},
	{PLAYER_ACODEC_MPG3,   0x0055},
	{PLAYER_ACODEC_LATM,   0x00ff},
	{PLAYER_ACODEC_LATM,   0x1600},
	{PLAYER_ACODEC_LATM,   0x1602},
	{PLAYER_ACODEC_LATM,   0x706d},
	{PLAYER_ACODEC_LATM,   0x4143},
	{PLAYER_ACODEC_LATM,   0xA106},
	{PLAYER_ACODEC_AC31,   0x2000},
	{PLAYER_ACODEC_DTS,    0x2001},
	{PLAYER_ACODEC_FLAC,   0xF1AC},
	{PLAYER_ACODEC_VORBIS, (('V' << 8) + 'o')},
};

static unsigned int wavtag_to_audio_codec(unsigned int tag)
{
	unsigned int i = 0;

	for (i = 0; i < sizeof(audio_codec_wav_tags) / sizeof(WavAudioTags); i++) {
		if (tag == audio_codec_wav_tags[i].tag)
			return audio_codec_wav_tags[i].format;
	}

	gxlogi("not find tags, fromat will be PLAYER_ACODEC_DVDPCM\n");
	return PLAYER_ACODEC_DVDPCM;
}

/*
*  return frame size or -1 (bad frame)
*/
int audio_get_mp3_header(unsigned char* hbuf, int *chans, int *srate, int *spf, int *mpa_layer, int *br)
{
	int stereo, ssize, lsf, framesize, padding, bitrate_index, sampling_frequency, divisor;
	int bitrate;
	int layer, mult[3] = { 12000, 144000, 144000 };
	unsigned long newhead = hbuf[0] << 24 | hbuf[1] << 16 | hbuf[2] << 8 | hbuf[3];

	if ((newhead & 0xffe00000) != 0xffe00000) {
		return -1;
	}

	layer = 4 - ((newhead >> 17) & 3);
	if (layer == 4) {
		return -1;
	}

	sampling_frequency = ((newhead >> 10) & 0x3);	// valid: 0..2
	if (sampling_frequency == 3) {
		return -1;
	}

	if (newhead & ((long)1 << 20)) {
		// MPEG 1.0 (lsf==0) or MPEG 2.0 (lsf==1)
		lsf = (newhead & ((long)1 << 19)) ? 0x0 : 0x1;
		sampling_frequency += (lsf*  3);
	} else {
		// MPEG 2.5
		lsf = 1;
		sampling_frequency += 6;
	}

	//    crc = ((newhead>>16)&0x1)^0x1;
	bitrate_index = ((newhead >> 12) & 0xf);	// valid: 1..14
	padding = ((newhead >> 9) & 0x1);

	stereo = ((((newhead >> 6) & 0x3)) == 3) ? 1 : 2;

	if (lsf)
		ssize = (stereo == 1) ? 9 : 17;
	else
		ssize = (stereo == 1) ? 17 : 32;
	if (!((newhead >> 16) & 0x1))
		ssize += 2;	// CRC

	bitrate = tabsel_123[lsf][layer - 1][bitrate_index];
	framesize = bitrate*  mult[layer - 1];

	if (!framesize) {
		return -1;
	}

	divisor = (layer == 3 ? (freqs[sampling_frequency] << lsf) : freqs[sampling_frequency]);
	framesize /= divisor;
	if (layer == 1)
		framesize = (framesize + padding)*  4;
	else
		framesize += padding;

	//    if(framesize<=0 || framesize>MAXFRAMESIZE) return FALSE;
	if (srate) {
		*srate = freqs[sampling_frequency];
		if (spf) {
			if (layer == 1)
				*spf = 384;
			else if (layer == 2)
				*spf = 1152;
			else if (*srate < 32000)
				*spf = 576;
			else
				*spf = 1152;
		}
	}
	if (mpa_layer)
		*mpa_layer = layer;
	if (chans)
		*chans = stereo;
	if (br)
		*br = bitrate;

	return framesize;
}

int audio_check_mp3_header(unsigned int head)
{
	if ((head & 0x0000e0ff) != 0x0000e0ff || (head & 0x00fc0000) == 0x00fc0000)
		return 0;
	if (audio_decode_mp3_header((unsigned char* )(&head)) <= 0)
		return 0;
	return 1;
}

/**
*  \brief free a list of MP3 header descriptions
*  \param list pointer to the head-of-list pointer
*/
void audio_free_mp3_headers(AudioMp3Header* list)
{
	AudioMp3Header* tmp;
	while (list) {
		tmp = list->next;
		av_free(list);
		list = tmp;
	}
}

/**
*  \brief add another potential MP3 header to our list
*  If it fits into an existing chain this one is expanded otherwise
*  a new one is created.
*  All entries that expected a MP3 header before the current position
*  are discarded.
*  The list is expected to be and will be kept sorted by next_frame_pos
*  and when those are equal by frame_pos.
*  \param list pointer to the head-of-list pointer
*  \param st_pos stream position where the described header starts
*  \param mp3_chans number of channels as specified by the header (*)
*  \param mp3_freq sampling frequency as specified by the header (*)
*  \param mpa_spf frame size as specified by the header
*  \param mpa_layer layer type ("version") as specified by the header (*)
*  \param mpa_br bitrate as specified by the header
*  \param mp3_flen length of the frame as specified by the header
*  \return If non-null the current file is accepted as MP3 and the
*  mp3_hdr struct describing the valid chain is returned. Must be
*  freed independent of the list.
*
*  parameters marked by (*) must be the same for all headers in the same chain
*/
static AudioMp3Header* audio_add_mp3_header(AudioMp3Header ** list, off_t st_pos,
					    int mp3_chans, int mp3_freq, int mpa_spf,
					    int mpa_layer, int mpa_br, int mp3_flen)
{
	AudioMp3Header* tmp;
	int in_list = 0;
	while (*list && (*list)->next_frame_pos <= st_pos) {
		if (((*list)->next_frame_pos < st_pos) || ((*list)->mp3_chans != mp3_chans)
		    || ((*list)->mp3_freq != mp3_freq) || ((*list)->mpa_layer != mpa_layer)) {
			// wasn't valid!
			tmp = (*list)->next;
			av_free(*list);
			*list = tmp;
		} else {
			(*list)->cons_headers++;
			(*list)->next_frame_pos = st_pos + mp3_flen;
			(*list)->mpa_spf = mpa_spf;
			(*list)->mpa_br = mpa_br;
			if ((*list)->cons_headers >= MIN_MP3_HDRS) {
				// copy the valid entry, so that the list can be easily freed
				tmp = av_malloc(sizeof(AudioMp3Header));
				memcpy(tmp,* list, sizeof(AudioMp3Header));
				tmp->next = NULL;
				return tmp;
			}
			in_list = 1;
			list = &((*list)->next);
		}
	}
	if (!in_list) {		// does not belong into an existing chain, insert
		// find right position to insert to keep sorting
		while (*list && (*list)->next_frame_pos <= st_pos + mp3_flen)
			list = &((*list)->next);
		tmp = av_malloc(sizeof(AudioMp3Header));
		tmp->frame_pos = st_pos;
		tmp->next_frame_pos = st_pos + mp3_flen;
		tmp->mp3_chans = mp3_chans;
		tmp->mp3_freq = mp3_freq;
		tmp->mpa_spf = mpa_spf;
		tmp->mpa_layer = mpa_layer;
		tmp->mpa_br = mpa_br;
		tmp->cons_headers = 1;
		tmp->next =* list;
		*list = tmp;
	}
	return NULL;
}

static void audio_get_flac_header(GxDemuxer* demuxer)
{
	GxStream *s = NULL;
	GxStreamHeadPriv *hdr = NULL;

	if (!demuxer)
		return;

	s = demuxer->stream;
	if (!s)
		return;

	hdr = (GxStreamHeadPriv*)demuxer->audio->sh;

	if (hdr) {
		int pos = GxStream_Tell(s);
		GxStream_Seek(s, 0);

		if (hdr->header.data)
			av_free(hdr->header.data);
		hdr->header.data = av_malloc(FLAC_STREAMINFO_SIZE+8);
		hdr->header.len  = FLAC_STREAMINFO_SIZE+8;
		hdr->header.flag = 0;
		if (hdr->header.data)
			GxStream_Read(s, hdr->header.data, hdr->header.len);

		((uint8_t*)hdr->header.data)[4] |= 0x80;

		GxStream_Seek(s, pos);
	}

	return;
}

static void audio_get_flac_metadata(GxDemuxer*  demuxer)
{
	uint8_t preamble[4];
	unsigned int blk_len;
	GxStream* s = NULL;

	if (!demuxer)
		return;

	s = demuxer->stream;
	if (!s)
		return;

	/* file is qualified; skip over the signature bytes in the stream*/
	GxStream_Seek(s, 4);

	/* loop through the metadata blocks; use a do-while construct since there
	*  will always be 1 metadata block */
	do {
		int r;

		r = GxStream_Read(s, preamble, FLAC_SIGNATURE_SIZE);
		if (r != FLAC_SIGNATURE_SIZE)
			return;

		blk_len = (preamble[1] << 16) | (preamble[2] << 8) | (preamble[3] << 0);

		switch (preamble[0] & 0x7F) {
		case FLAC_STREAMINFO:
			if (blk_len != FLAC_STREAMINFO_SIZE)
				return;
			GxStream_Skip(s, FLAC_STREAMINFO_SIZE);
			break;
		case FLAC_PADDING:
			GxStream_Skip(s, blk_len);
			break;
		case FLAC_APPLICATION:
			GxStream_Skip(s, blk_len);
			break;
		case FLAC_SEEKTABLE:
			{
				int seekpoint_count, i;

				seekpoint_count = blk_len / FLAC_SEEKPOINT_SIZE;
				for (i = 0; i < seekpoint_count; i++)
					if (GxStream_Skip(s, FLAC_SEEKPOINT_SIZE) != 1)
						return;
				break;
			}

		case FLAC_VORBIS_COMMENT:
			{
				/* For a description of the format please have a look at*/
				/* http://www.xiph.org/vorbis/doc/v-comment.html*/

				uint32_t length, comment_list_len;
				uint8_t comments[blk_len];
				uint8_t* ptr = comments;
				char* comment;
				int cn;
				char c;

				if (GxStream_Read(s, comments, blk_len) == blk_len) {
					length = AV_RL32(ptr);
					ptr += 4 + length;

					comment_list_len = AV_RL32(ptr);
					ptr += 4;

					cn = 0;
					for (; cn < comment_list_len; cn++) {
						length = AV_RL32(ptr);
						ptr += 4;

						comment = (char* )ptr;
						c = comment[length];
						comment[length] = 0;

						if (!strncasecmp("TITLE=", comment, 6) && (length - 6 > 0)){
							gxlogf("Title: %s\n", comment + 6);
						}
						else if (!strncasecmp("ARTIST=", comment, 7) && (length - 7 > 0)){
							gxlogf("Artist: %s\n", comment + 7);
						}
						else if (!strncasecmp("ALBUM=", comment, 6) && (length - 6 > 0)){
							gxlogf("Album: %s\n", comment + 6);
						}
						else if (!strncasecmp("DATE=", comment, 5) && (length - 5 > 0)){
							gxlogf("Year: %s\n", comment + 5);
						}
						else if (!strncasecmp("GENRE=", comment, 6) && (length - 6 > 0)){
							gxlogf("Genre: %s\n", comment + 6);
						}
						else if (!strncasecmp("Comment=", comment, 8) && (length - 8 > 0)){
							gxlogf("Comment: %s\n", comment + 8);
						}
						else if (!strncasecmp("TRACKNUMBER=", comment, 12)
							 && (length - 12 > 0)) {
							char buf[31];
							snprintf(buf, sizeof(buf), "%d", atoi(comment + 12));
							buf[30] = '\0';
							gxlogf("Track: %s\n", buf);
						}
						comment[length] = c;

						ptr += length;
					}
				}
				break;
			}

		case FLAC_CUESHEET:
			GxStream_Skip(s, blk_len);
			break;

		default:
			/* 6-127 are presently reserved*/
			GxStream_Skip(s, blk_len);
			break;
		}
	} while ((preamble[0] & 0x80) == 0);
}

static void audio_high_res_mp3_seek(GxDemuxer*  demuxer, float time)
{
	uint8_t hdr[4];
	int len, nf;
	DemuxAudioPriv* priv = demuxer->priv;
	GxStreamAudioHeader* sh = (GxStreamAudioHeader *) demuxer->audio->sh;

	nf = time*  sh->samplerate / sh->audio.dwScale;
	while (nf > 0) {
		GxStream_Read(demuxer->stream, hdr, 4);
		len = audio_decode_mp3_header(hdr);
		if (len < 0) {
			if(GxStream_Skip(demuxer->stream, -3)!=GX_PLAYER_OK)
				return ;
			continue;
		}
		GxStream_Skip(demuxer->stream, len - 4);
		priv->next_pts += sh->audio.dwScale / (double)sh->samplerate;
		nf--;
	}
}

static int audio_sync_n(GxDemuxer *demuxer, int nSync)
{
	int synced=0, synccnt=0;
	off_t target_pos;
	DemuxAudioPriv* priv = demuxer->priv;
	GxStream* stream = demuxer->stream;

	switch (priv->frmt) {
	case MP3:
		target_pos = GxStream_Tell(stream);
		while (synccnt < nSync)
		{
			uint8_t hdr[4];
			int l = 0;

			if (stream->eof || (demuxer->movi_end && GxStream_Tell(stream) >= demuxer->movi_end))
				return 0;

			GxStream_Read(stream, hdr, 4);
			l = audio_decode_mp3_header(hdr);
			if (l < 0) {
				GxStream_Skip(stream, -3);
				target_pos = GxStream_Tell(stream);
				synccnt = 0;
				synced  = 0;
			} else {
				if (synccnt != (nSync -1)) GxStream_Skip(stream, l - 4);
				synced = 1;
				synccnt++;
			}
		}
		GxStream_Seek(stream, target_pos);
		return synced;
	default:
		return 0;
	}

	return 1;
}

static int demux_audio_check_file(GxDemuxer*  demuxer)
{
	return GX_DEMUXER_TYPE_AUDIO;
}

static GxDemuxer* demux_audio_open(GxDemuxer * demuxer)
{
	uint8_t hdr[HDR_SIZE];
	int frmt=0, n=0, step;
	int vbr=0, frame_num=0, mpa_spf=0;
	int64_t duration=0;
	off_t st_pos = 0, next_frame_pos = 0;
	// mp3_headers list is sorted first by next_frame_pos and then by frame_pos
	GxStream* s;
	DemuxAudioPriv* priv;
	GxStreamAudioHeader* sh_audio;
	AudioMp3Header* mp3_headers = NULL, *mp3_found = NULL;

	s = demuxer->stream;

	GxStream_Read(s, hdr, HDR_SIZE);
	while (n < 50000 && !s->eof)
    {
		int mp3_freq, mp3_chans, mp3_flen, mpa_layer, mpa_spf, mpa_br;
		st_pos = GxStream_Tell(s) - HDR_SIZE;
		step = 1;

		if (hdr[0] == 'R' && hdr[1] == 'I' && hdr[2] == 'F' && hdr[3] == 'F')
        {
			GxStream_Skip(s, 4);
			if (s->eof)
				break;
			GxStream_Read(s, hdr, 4);
			if (s->eof)
				break;
			if (hdr[0] != 'W' || hdr[1] != 'A' || hdr[2] != 'V' || hdr[3] != 'E')
				GxStream_Skip(s, -8);
			else
				// We found wav header. Now we can have 'fmt ' or a mp3 header
				// empty the buffer
				step = 4;
		}
        else
            if (hdr[0] == 'I' && hdr[1] == 'D' && hdr[2] == '3' && (hdr[3] >= 2))
            {
			    int len;
			    GxStream_Skip(s, 2);
				//if(s->eof)
				//	break;
			    GxStream_Read(s, hdr, 4);
			    len = (hdr[0] << 21) | (hdr[1] << 14) | (hdr[2] << 7) | hdr[3];
			    GxStream_Skip(s, len);
				//if(s->eof)
				//	break;

			    step = 4;
		    }
            else
                if (hdr[0] == 'f' && hdr[1] == 'm' && hdr[2] == 't' && hdr[3] == ' ')
                {
			        frmt = WAV;
			        break;
		        }
                else
                    if ((mp3_flen =
                         audio_get_mp3_header(hdr, &mp3_chans, &mp3_freq, &mpa_spf, &mpa_layer, &mpa_br))> 0)
                    {
			             mp3_found =
			             audio_add_mp3_header(&mp3_headers, st_pos, mp3_chans, mp3_freq, mpa_spf, mpa_layer, mpa_br,mp3_flen);
			             if (mp3_found)
                         {
			             	frmt = MP3;
			             	break;
			             }
		            }
                    else
                        if (hdr[0] == 'f' && hdr[1] == 'L' && hdr[2] == 'a' && hdr[3] == 'C')
                        {
			                frmt = fLaC;
			                break;
		                }
                        else
                            if ((hdr[0] == 'X' && hdr[1] == 'i' && hdr[2] == 'n' && hdr[3] == 'g')||
					            (hdr[0] == 'I' && hdr[1] == 'n' && hdr[2] == 'f' && hdr[3] == 'o'))
                            {
			                    int flag,frames=0,filesize=0;
			                    flag = GxStream_ReadDword(s);
			                    if(flag & 0x0001)
			                    	frames =  GxStream_ReadDword(s);
			                    if(flag & 0x0002)
			                    	filesize =  GxStream_ReadDword(s);
			                    if(flag & 0x0004)
			                    	GxStream_Skip(s,100);
			                    if(flag & 0x0008)
			                    	GxStream_ReadDword(s);
			                    vbr = 1;
			                    frame_num = frames;
		                     }
		                     else
                                if (hdr[0] == 'V' && hdr[1] == 'B' && hdr[2] == 'R' && hdr[3] == 'I')
                                {
			                        int frames=0,toc,toc_num;
			                        GxStream_Skip(s,10);
			                        frames = GxStream_ReadDword(s);
			                        GxStream_Skip(s,4);
			                        toc = GxStream_ReadWord(s);
			                        toc_num = GxStream_ReadWord(s);
			                        GxStream_Skip(s,toc_num*toc);
			                        vbr = 1;
			                        frame_num = frames;
		                        }
		// Add here some other audio format detection
		if (step < HDR_SIZE)
			memmove(hdr, &hdr[step], HDR_SIZE - step);

		GxStream_Read(s, &hdr[HDR_SIZE - step], step);
		n++;
	}
    if(mp3_headers)
	    mpa_spf = mp3_headers->mpa_spf;

	audio_free_mp3_headers(mp3_headers);

	if (!frmt)
		return NULL;

	sh_audio = GxStreamHeader_AudioNew(demuxer, 0, 0);

	switch (frmt)
    {
	case MP3:
		sh_audio->format = (mp3_found->mpa_layer < 3 ? 0x50 : 0x55);
		demuxer->movi_start = mp3_found->frame_pos;
		next_frame_pos = mp3_found->next_frame_pos;
		sh_audio->audio.dwSampleSize = 0;
		sh_audio->audio.dwScale = mp3_found->mpa_spf;
		sh_audio->audio.dwRate = mp3_found->mp3_freq;
		sh_audio->wf = av_malloc(sizeof(WAVEFORMATEX));
		sh_audio->wf->wFormatTag = sh_audio->format;
		sh_audio->wf->nChannels = mp3_found->mp3_chans;
		sh_audio->wf->nSamplesPerSec = mp3_found->mp3_freq;
		sh_audio->wf->nAvgBytesPerSec = mp3_found->mpa_br*  (1000 / 8);
		sh_audio->wf->nBlockAlign = mp3_found->mpa_spf;
		sh_audio->wf->wBitsPerSample = 16;
		sh_audio->wf->cbSize = 0;
		sh_audio->i_bps = sh_audio->wf->nAvgBytesPerSec;
		av_free(mp3_found);
		mp3_found = NULL;
		if (s->end_pos && s->file_format!= GX_STREAMTYPE_STREAM) {
			uint8_t tag[4];
			GxStream_Seek(s, s->end_pos - 128);
			GxStream_Read(s, tag, 3);
			tag[3] = '\0';
			if (strcmp((char* )tag, "TAG"))
				demuxer->movi_end = s->end_pos;
			else {
				char buf[31];
				uint8_t g;
				#if 0
				demuxer->movi_end = GxStream_Tell(s) - 3;
				GxStream_Read(s, (uint8_t* ) buf, 30);
				buf[30] = '\0';
				gxlogf("Title: %s\n", buf);
				GxStream_Read(s, (uint8_t* ) buf, 30);
				buf[30] = '\0';
				gxlogf("Artist: %s\n", buf);
				GxStream_Read(s, (uint8_t* ) buf, 30);
				buf[30] = '\0';
				gxlogf("Album: %s\n", buf);
				GxStream_Read(s, (uint8_t* ) buf, 4);
				buf[4] = '\0';
				gxlogf("Year: %s\n", buf);
				GxStream_Read(s, (uint8_t* ) buf, 30);
				buf[30] = '\0';
				gxlogf("Comment: %s\n", buf);
				if (buf[28] == 0 && buf[29] != 0) {
					uint8_t trk = (uint8_t) buf[29];
					snprintf(buf, sizeof(buf), "%d", trk);
					gxlogf("Track: %s\n", buf);
				}
				g = GxStream_ReadChar(s);
				gxlogf("Genre: %s\n", genres[g]);
				#else
				demuxer->movi_end = GxStream_Tell(s) - 3;
				GxStream_Read(s, (uint8_t* ) buf, 30);
				GxStream_Read(s, (uint8_t* ) buf, 30);
				GxStream_Read(s, (uint8_t* ) buf, 30);
				GxStream_Read(s, (uint8_t* ) buf, 4);
				GxStream_Read(s, (uint8_t* ) buf, 30);
				g = GxStream_ReadChar(s);
				#endif
			}
		}
		break;
	case WAV:
		{
			unsigned int chunk_type;
			unsigned int chunk_size;
			off_t best_chunk_pos = 0;
			unsigned int best_chunk_size = 0;
			WAVEFORMATEX* w;
			int l;
			l = GxStream_ReadDword_Le(s);
			if (l < 16) {
				l = 16;
			}
			if (l > MAX_WAVHDR_LEN) {
				l = 16;
			}
			sh_audio->wf = w = av_malloc(l > sizeof(WAVEFORMATEX) ? l : sizeof(WAVEFORMATEX));
			w->wFormatTag = sh_audio->format = GxStream_ReadWord_Le(s);
			sh_audio->format = wavtag_to_audio_codec(w->wFormatTag);
			w->nChannels = sh_audio->channels = GxStream_ReadWord_Le(s);
			w->nSamplesPerSec = sh_audio->samplerate = GxStream_ReadDword_Le(s);
			w->nAvgBytesPerSec = GxStream_ReadDword_Le(s);
			w->nBlockAlign = GxStream_ReadWord_Le(s);
			w->wBitsPerSample = GxStream_ReadWord_Le(s);
			sh_audio->samplesize = w->wBitsPerSample;
			sh_audio->sample_format = PCM_LITTLE_ENDIAN; //little endian
			w->cbSize = 0;
			sh_audio->i_bps = sh_audio->wf->nAvgBytesPerSec;
			l -= 16;
			if (l > 0) {
				w->cbSize = GxStream_ReadWord_Le(s);
				l -= 2;
				if (w->cbSize > 0) {
					if (l < w->cbSize) {
						GxStream_Read(s, (uint8_t* ) ((char *)(w) + sizeof(WAVEFORMATEX)), l);
						l = 0;
					} else {
						GxStream_Read(s, (uint8_t* ) ((char *)(w) + sizeof(WAVEFORMATEX)),
							      w->cbSize);
						l -= w->cbSize;
					}
				}
			}

			if (l)
				GxStream_Skip(s, l);

			do {
				chunk_type = GxStream_ReadDword_Le(demuxer->stream);
				chunk_size = GxStream_ReadDword_Le(demuxer->stream);
				if (chunk_type == GX_FOURCC('d', 'a', 't', 'a')) {
					if (chunk_size > best_chunk_size) {
						best_chunk_pos  = GxStream_Tell(s);
						best_chunk_size = chunk_size;
					}
				}
				GxStream_Skip(demuxer->stream, chunk_size);
			} while (!s->eof);

			GxStream_Seek(s, best_chunk_pos);
			chunk_size = best_chunk_size;
			demuxer->movi_start = GxStream_Tell(s);
			demuxer->movi_end   = chunk_size ? demuxer->movi_start + chunk_size : s->end_pos;
			// Check if it contains dts audio
			if ((w->wFormatTag == 0x01) && (w->nChannels == 2) && (w->nSamplesPerSec == 44100))
            {
				unsigned int i;
				unsigned char *buf = av_malloc(DTS_PROBE_SIZE);	// vlc uses DTS_PROBE_SIZE*4 (4 dts frames)

				GxStream_Read(s, buf, DTS_PROBE_SIZE);
				for (i = 0; i < DTS_PROBE_SIZE - 5; i += 2) {
					// DTS, 14 bit, LE
					if ((buf[i] == 0xff) && (buf[i + 1] == 0x1f) && (buf[i + 2] == 0x00) &&
					    (buf[i + 3] == 0xe8) && ((buf[i + 4] & 0xfe) == 0xf0)
					    && (buf[i + 5] == 0x07)) {
						sh_audio->format = PLAYER_ACODEC_DTS;
						break;
					}
					// DTS, 14 bit, BE
					if ((buf[i] == 0x1f) && (buf[i + 1] == 0xff) && (buf[i + 2] == 0xe8) &&
					    (buf[i + 3] == 0x00) && (buf[i + 4] == 0x07)
					    && ((buf[i + 5] & 0xfe) == 0xf0)) {
						sh_audio->format = PLAYER_ACODEC_DTS;
						break;
					}
					// DTS, 16 bit, BE
					if ((buf[i] == 0x7f) && (buf[i + 1] == 0xfe) && (buf[i + 2] == 0x80)
					    && (buf[i + 3] == 0x01)) {
						sh_audio->format = PLAYER_ACODEC_DTS;
						break;
					}
					// DTS, 16 bit, LE
					if ((buf[i] == 0xfe) && (buf[i + 1] == 0x7f) && (buf[i + 2] == 0x01)
					    && (buf[i + 3] == 0x80)) {
						sh_audio->format = PLAYER_ACODEC_DTS;
						break;
					}
				}
				av_free(buf);
			}
			sh_audio->audio.dwRate = sh_audio->samplerate;
			GxStream_Seek(s, demuxer->movi_start);
		}
		break;
	case fLaC:
		{
			int32_t srate = 0;
			int64_t num_samples = 0;
			int64_t size = demuxer->movi_end - demuxer->movi_start;

			sh_audio->format    = GX_FOURCC('f', 'L', 'a', 'C');
			demuxer->movi_start = GxStream_Tell(s) - 4;
			demuxer->movi_end   = s->end_pos;

			if (demuxer->movi_end < demuxer->movi_start)
				return NULL;

			// try to find out approx. bitrate
			GxStream_Skip(s, 14);
			GxStream_Read(s, (uint8_t* )&srate, 3);
			srate = be2me_32(srate) >> 12;
			GxStream_Read(s, (uint8_t* )&num_samples, 5);
			num_samples = (be2me_64(num_samples) >> 24) & 0xfffffffffULL;

			if (num_samples && srate)
				sh_audio->i_bps = size * srate / num_samples;
			if (sh_audio->i_bps < 1)	// guess value to prevent crash
				sh_audio->i_bps = 64 * 1024;

			sh_audio->audio.dwRate = srate;
			audio_get_flac_metadata(demuxer);
			break;
		}
	}
	if(!demuxer->movi_end){
		off_t movi_end = 0;
		GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_SIZE, &movi_end);
		demuxer->movi_end = FFMAX(0, movi_end);
	}

	priv = av_mallocz(sizeof(DemuxAudioPriv));
	priv->frmt = frmt;
	priv->next_pts = 0;
	priv->frame_num = frame_num;
	priv->mpa_spf = mpa_spf;
	priv->vbr = vbr;
	GxPlayer_SystemGet(PSYS_BufSizeESA, &priv->esa_size);
	demuxer->priv = priv;
	demuxer->audio->id = 0;
	demuxer->audio->sh = sh_audio;
	sh_audio->ds = demuxer->audio;
	sh_audio->format = acodec_fourcc2std(sh_audio->format);
	sh_audio->samplerate = sh_audio->audio.dwRate;
    duration = priv->frame_num * priv->mpa_spf /sh_audio->samplerate*1000;
    if(duration)
	    sh_audio->i_bps = (demuxer->movi_end - demuxer->movi_start)*1000/duration;
	// copy from demux.c
//	if (demuxer->audio->sh && ((GxStreamAudioHeader* ) demuxer->audio->sh)->format == 0x55)	// MP3
//		priv->hr_mp3_seek = 1;	// Enable high res seeking

	if (GxStream_Tell(s) != demuxer->movi_start)
	{
		GxStream_Seek(s, demuxer->movi_start);
		if (GxStream_Tell(s) != demuxer->movi_start)
		{
			if (next_frame_pos)
			{
				GxStream_Seek(s, next_frame_pos);
			}
		}
	}

	demuxer->filepos = GxStream_Tell(s);
	return demuxer;
}

static void demux_audio_close(GxDemuxer*  demuxer)
{
	DemuxAudioPriv* priv = demuxer->priv;

	if (!priv)
		return;
	av_free(priv);

}

static int demux_audio_seek(GxDemuxer*  demuxer, int64_t rel_seek_ms, int32_t audio_delay, int flags)
{
	int ret = GX_PLAYER_OK;
	GxStreamAudioHeader* sh_audio;
	GxStream* s;
	int base, pos;
	float len;
	DemuxAudioPriv* priv;
	float rel_seek_secs = (float)rel_seek_ms/1000;

	if (!(sh_audio = demuxer->audio->sh))
		return GX_PLAYER_ERROR;
	s = demuxer->stream;
	priv = demuxer->priv;

	if (priv->frmt == MP3 && priv->hr_mp3_seek && !(flags & GX_DEMUXER_SEEK_PERCENT)) {
		len = (flags & GX_DEMUXER_SEEK_ABSOLUTE) ? rel_seek_secs - priv->next_pts : rel_seek_secs;
		if (len < 0) {
			GxStream_Seek(s, demuxer->movi_start);
			len = priv->next_pts + len;
			priv->next_pts = 0;
		}
		if (len > 0)
			audio_high_res_mp3_seek(demuxer, len);
		return ret;
	}

	base = flags & GX_DEMUXER_SEEK_ABSOLUTE ? demuxer->movi_start : GxStream_Tell(s);
	if (flags & GX_DEMUXER_SEEK_PERCENT)
		pos = base + ((demuxer->movi_end - demuxer->movi_start)*  rel_seek_secs);
	else
		pos = base + (rel_seek_secs*  sh_audio->i_bps);

	if (demuxer->movi_end && pos >= demuxer->movi_end) {
		pos = demuxer->movi_end;
	} else if (pos < demuxer->movi_start)
		pos = demuxer->movi_start;

	priv->next_pts = (pos - demuxer->movi_start) / (double)sh_audio->i_bps;

	switch (priv->frmt) {
	case WAV:
		pos -= (pos - demuxer->movi_start) %
		    (sh_audio->wf->nBlockAlign ? sh_audio->wf->nBlockAlign :
		     (sh_audio->channels*  sh_audio->samplesize));
		break;
	case fLaC:
		audio_get_flac_header(demuxer);
		break;
	}

	GxStream_Seek(s, pos);
	audio_sync_n(demuxer, 3);
	demuxer->filepos = GxStream_Tell(s);

	return ret;
}

static int demux_audio_fill_buffer(GxDemuxer*  demuxer,GxDemuxStream* ds)
{
	int l;
	GxDemuxPacket* dp;
	GxStreamAudioHeader* sh_audio = demuxer->audio->sh;
	GxDemuxer* demux = demuxer;
	DemuxAudioPriv* priv = demux->priv;
	double this_pts = priv->next_pts;
	GxStream* s = demux->stream;

	if ((s->eof) || (!sh_audio))
		return GX_PLAYER_ERROR;

	switch (priv->frmt) {
	case MP3:
		while (1) {
			uint8_t hdr[4];
			GxStream_Read(s, hdr, 4);
			if (s->eof)
				return GX_PLAYER_ERROR;
			l = audio_decode_mp3_header(hdr);
			if (l < 0) {
				if (demux->movi_end && GxStream_Tell(s) >= demux->movi_end){
					s->eof = 1;
					return GX_PLAYER_ERROR;	// might be ID3 tag, i.e. EOF
				}
				GxStream_Skip(s, -3);
			} else {
				dp = GxDemuxPacket_Create(demuxer, NULL, l);
				if (dp == NULL)
					return GX_PLAYER_ERROR;
				memcpy(dp->buffer, hdr, 4);
				if (GxStream_Read(s, dp->buffer + 4, l - 4) != l - 4) {
					GxDemuxPacket_Destroy(dp);
					s->eof = 1;
					return GX_PLAYER_ERROR;
				}
				priv->next_pts += sh_audio->audio.dwScale / (double)sh_audio->samplerate;
				break;
			}
		}
		break;
	case WAV:
		{
			unsigned short align = sh_audio->wf->nBlockAlign;
			l = sh_audio->wf->nAvgBytesPerSec/20;
			if (demux->movi_end && l > demux->movi_end - GxStream_Tell(s)) {
				// do not read beyond end, there might be junk after data chunk
				l = demux->movi_end - GxStream_Tell(s);
				if (l <= 0){
					s->eof = 1;
					return GX_PLAYER_ERROR;
				}
			}
			if (align)
				l = (l + align - 1) / align*  align;
			dp = GxDemuxPacket_Create(demuxer, NULL, l);
			if (dp == NULL)
				return GX_PLAYER_ERROR;
			l = GxStream_Read(s, dp->buffer, l);
			//priv->next_pts += l / (double)sh_audio->i_bps;
			priv->next_pts += 0.05;
		}
		break;
	case fLaC:
		l = ((priv->esa_size / 4) <=  65535) ? (priv->esa_size / 4) : 65535;
		l = (l == 0) ? 65535 : l;
		dp = GxDemuxPacket_Create(demuxer, NULL, l);
		if (dp == NULL)
			return GX_PLAYER_ERROR;
		l = GxStream_Read(s, dp->buffer, l);
		/* FLAC is not a constant-bitrate codec. These values will be wrong.*/
		//priv->next_pts += l / (double)sh_audio->i_bps;
		break;
	default:
		s->eof = 1;
		return GX_PLAYER_ERROR;
	}

	dp=GxDemuxPacket_Resize(demuxer, dp, l);
	if(dp){
		dp->pts = this_pts*1000;
		GxDemuxStream_AddPacket(demuxer->audio, dp);
		demuxer->filepos = GxStream_Tell(s);
	}
	return GX_PLAYER_OK;
}

static int demux_audio_control(GxDemuxer*  demuxer, int cmd, void* arg)
{
	int audio_length;
	GxStreamAudioHeader* sh_audio = demuxer->audio->sh;
	DemuxAudioPriv* priv = demuxer->priv;

    if(sh_audio && sh_audio->i_bps)
        audio_length = demuxer->movi_end /sh_audio->i_bps;
    else
        audio_length = 0;
	switch (cmd) {
	case GX_DEMUXER_CTRL_GET_TIME_LENGTH:
		if(priv->vbr && sh_audio && sh_audio->samplerate != 0){
			*((int64_t* )arg) = priv->frame_num * priv->mpa_spf /sh_audio->samplerate*1000;
		}
		else{
			if (audio_length <= 0)
				return GX_DEMUXER_CTRL_ERROR;
			*((int64_t* )arg) = (int64_t)audio_length*1000;
		}
		return GX_DEMUXER_CTRL_OK;

	case GX_DEMUXER_CTRL_GET_PERCENT_POS:
		if (audio_length <= 0)
			return GX_DEMUXER_CTRL_ERROR;
		*((int* )arg) = (int)((priv->next_pts * 100) / audio_length);
		return GX_DEMUXER_CTRL_OK;

	default:
		return GX_DEMUXER_CTRL_NOTIMPL;
	}
}

GxDemuxerClass gx_demux_audio = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "Demuxer AUDIO",
			.parent  = &gx_DemuxerBase,
			.size    = sizeof(GxDemuxer),
			.create  = NULL,
			.release = NULL,
		},
		.run    = NULL,
		.pause  = NULL,
		.resume = NULL,
		.stop   = NULL,
	},
	DEF_AUTHOR("demuxer","audio","No description","L.F","No comment"),

	.name        = "Demuxer MPEG-Audio",
	.type        = GX_DEMUXER_TYPE_AUDIO,
	.check_file  = demux_audio_check_file,
	.open        = demux_audio_open,
	.close       = demux_audio_close,
	.fill_buffer = demux_audio_fill_buffer,
	.seek        = demux_audio_seek,
	.control     = demux_audio_control,
};
