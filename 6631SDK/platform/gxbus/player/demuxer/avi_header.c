
#include "demux_avi.h"

static MainAVIHeader avih;

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

static void* realloc_struct(void *ptr, size_t nmemb, size_t size)
{
	if (nmemb > SIZE_MAX / size) {
		av_free(ptr);
		return NULL;
	}
	return av_realloc(ptr, nmemb*  size);
}

inline int avi_stream_id(unsigned int id)
{
	unsigned char* p = (unsigned char *)&id;
	unsigned char a, b;
#ifdef WORDS_BIGENDIAN
	a = p[3] - '0';
	b = p[2] - '0';
#else
	a = p[0] - '0';
	b = p[1] - '0';
#endif
	if (a > 9 || b > 9)
		return 100;	// invalid ID
	return a*  10 + b;
}

static int odml_get_vstream_id(int id, unsigned char res[])
{
	unsigned char* p = (unsigned char *)&id;
	id = le2me_32(id);

	if (p[2] == 'd') {
		if (res) {
			res[0] = p[0];
			res[1] = p[1];
		}
		return 1;
	}
	return 0;
}

int avi_idx_cmp(const void* elem1, const void *elem2)
{
	register off_t a = AVI_IDX_OFFSET((AVIIndexEntry* ) elem1);
	register off_t b = AVI_IDX_OFFSET((AVIIndexEntry* ) elem2);
	return (a > b) - (b > a);
}

void read_avi_header(GxDemuxer*  demuxer, int index_mode)
{
	GxStreamAudioHeader* sh_audio = NULL;
	GxStreamVideoHeader* sh_video = NULL;
	int stream_id = -1;
	int idxfix_videostream = 0;
	int idxfix_divx = 0;
	DemuxAviPriv* priv = demuxer->priv;
	off_t list_end = 0;

	//---- AVI header:
	priv->idx_size = 0;
	priv->audio_streams = 0;
	while (1) {
		int id = GxStream_ReadDword_Le(demuxer->stream);
		unsigned chunksize, size2;
		static int last_fccType = 0;
		static int last_fccHandler = 0;
		char* hdr = NULL;
		//
		if (GxStream_Eof(demuxer->stream))
			break;
		//
		if (id == GX_FOURCC('L', 'I', 'S', 'T')) {
			unsigned len = GxStream_ReadDword_Le(demuxer->stream);	// list size
			id = GxStream_ReadDword_Le(demuxer->stream);	// list type
			if (len >= 4) {
				len -= 4;
				list_end = GxStream_Tell(demuxer->stream) + ((len + 1) & (~1));
			} else {
				list_end = 0;
			}

			if (id == listtypeAVIMOVIE) {
				// found MOVI header
				if (!demuxer->movi_start)
					demuxer->movi_start = GxStream_Tell(demuxer->stream);
				demuxer->movi_end = GxStream_Tell(demuxer->stream) + len;

				if (demuxer->stream->end_pos > demuxer->movi_end)
					demuxer->movi_end = demuxer->stream->end_pos;
				if (index_mode == -2 || index_mode == 2 || index_mode == 0)
					break;	// reading from non-seekable source (stdin) or forced index or no index forced
				if (list_end > 0)
					GxStream_Seek(demuxer->stream, list_end);	// skip movi
				list_end = 0;
			}
			continue;
		}
		size2 = GxStream_ReadDword_Le(demuxer->stream);

		chunksize = (size2 + 1) & (~1);
		switch (id) {
			// Indicates where the subject of the file is archived
		case GX_FOURCC('I', 'A', 'R', 'L'):
			hdr = "Archival Location";
			break;
			// Lists the artist of the original subject of the file;
			// for example, "Michaelangelo."
		case GX_FOURCC('I', 'A', 'R', 'T'):
			hdr = "Artist";
			break;
			// Lists the name of the person or organization that commissioned
			// the subject of the file; for example "Pope Julian II."
		case GX_FOURCC('I', 'C', 'M', 'S'):
			hdr = "Commissioned";
			break;
			// Provides general comments about the file or the subject
			// of the file. If the comment is several sentences long, end each
			// sentence with a period. Do not include new-line characters.
		case GX_FOURCC('I', 'C', 'M', 'T'):
			hdr = "Comments";
			break;
			// Records the copyright information for the file; for example,
			// "Copyright Encyclopedia International 1991." If there are multiple
			// copyrights, separate them by semicolon followed by a space.
		case GX_FOURCC('I', 'C', 'O', 'P'):
			hdr = "Copyright";
			break;
			// Describes whether an image has been cropped and, if so, how it
			// was cropped; for example, "lower-right corner."
		case GX_FOURCC('I', 'C', 'R', 'D'):
			hdr = "Creation Date";
			break;
			// Describes whether an image has been cropped and, if so, how it
			// was cropped; for example, "lower-right corner."
		case GX_FOURCC('I', 'C', 'R', 'P'):
			hdr = "Cropped";
			break;
			// Specifies the size of the original subject of the file; for
			// example, "8.5 in h, 11 in w."
		case GX_FOURCC('I', 'D', 'I', 'M'):
			hdr = "Dimensions";
			break;
			// Stores dots per inch setting of the digitizer used to
			// produce the file, such as "300."
		case GX_FOURCC('I', 'D', 'P', 'I'):
			hdr = "Dots Per Inch";
			break;
			// Stores the of the engineer who worked on the file. If there are
			// multiple engineers, separate the names by a semicolon and a blank;
			// for example, "Smith, John; Adams, Joe."
		case GX_FOURCC('I', 'E', 'N', 'G'):
			hdr = "Engineer";
			break;
			// Describes the original work, such as "landscape,", "portrait,"
			// "still liefe," etc.
		case GX_FOURCC('I', 'G', 'N', 'R'):
			hdr = "Genre";
			break;
			// Provides a list of keywords that refer to the file or subject of the
			// file. Separate multiple keywords with a semicolon and a blank;
			// for example, "Seattle, aerial view; scenery."
		case GX_FOURCC('I', 'K', 'E', 'Y'):
			hdr = "Keywords";
			break;
			// ILGT - Describes the changes in the lightness settings on the digitizer
			// required to produce the file. Note that the format of this information
			// depends on the hardware used.
		case GX_FOURCC('I', 'L', 'G', 'T'):
			hdr = "Lightness";
			break;
			// IMED - Decribes the original subject of the file, such as
			// "computer image," "drawing," "lithograph," and so on.
		case GX_FOURCC('I', 'M', 'E', 'D'):
			hdr = "Medium";
			break;
			// INAM - Stores the title of the subject of the file, such as
			// "Seattle from Above."
		case GX_FOURCC('I', 'N', 'A', 'M'):
			hdr = "Name";
			break;
			// IPLT - Specifies the number of colors requested when digitizing
			// an image, such as "256."
		case GX_FOURCC('I', 'P', 'L', 'T'):
			hdr = "Palette Setting";
			break;
			// IPRD - Specifies the name of title the file was originally intended
			// for, such as "Encyclopedia of Pacific Northwest Geography."
		case GX_FOURCC('I', 'P', 'R', 'D'):
			hdr = "Product";
			break;
			// ISBJ - Decsribes the contents of the file, such as
			// "Aerial view of Seattle."
		case GX_FOURCC('I', 'S', 'B', 'J'):
			hdr = "Subject";
			break;
			// ISFT - Identifies the name of the software packages used to create the
			// file, such as "Microsoft WaveEdit"
		case GX_FOURCC('I', 'S', 'F', 'T'):
			hdr = "Software";
			break;
			// ISHP - Identifies the change in sharpness for the digitizer
			// required to produce the file (the format depends on the hardware used).
		case GX_FOURCC('I', 'S', 'H', 'P'):
			hdr = "Sharpness";
			break;
			// ISRC - Identifies the name of the person or organization who
			// suplied the original subject of the file; for example, "Try Research."
		case GX_FOURCC('I', 'S', 'R', 'C'):
			hdr = "Source";
			break;
			// ISRF - Identifies the original form of the material that was digitized,
			// such as "slide," "paper," "map," and so on. This is not necessarily
			// the same as IMED
		case GX_FOURCC('I', 'S', 'R', 'F'):
			hdr = "Source Form";
			break;
			// ITCH - Identifies the technician who digitized the subject file;
			// for example, "Smith, John."
		case GX_FOURCC('I', 'T', 'C', 'H'):
			hdr = "Technician";
			break;
		case GX_FOURCC('I', 'S', 'M', 'P'):
			hdr = "Time Code";
			break;
		case GX_FOURCC('I', 'D', 'I', 'T'):
			hdr = "Digitization Time";
			break;

		case ckidAVIMAINHDR:	// read 'avih'
			GxStream_Read(demuxer->stream, (uint8_t* ) & avih, GXMIN(size2, sizeof(avih)));
			le2me_MainAVIHeader(&avih);	// swap to machine endian
			chunksize -= GXMIN(size2, sizeof(avih));

			break;
		case ckidSTREAMHEADER:
			{	// read 'strh'
				AVIStreamHeader h;
				GxStream_Read(demuxer->stream, (uint8_t* ) & h, GXMIN(size2, sizeof(h)));
				le2me_AVIStreamHeader(&h);	// swap to machine endian
				chunksize -= GXMIN(size2, sizeof(h));
				++stream_id;
				if (h.fccType == streamtypeVIDEO) {
					sh_video = GxStreamHeader_VideoNew(demuxer, stream_id, stream_id);
					memcpy(&sh_video->video, &h, sizeof(h));
					sh_video->stream_delay = (float)sh_video->video.dwStart*  sh_video->video.dwScale /sh_video->video.dwRate;
				} else if (h.fccType == streamtypeAUDIO) {
					sh_audio = GxStreamHeader_AudioNew(demuxer, stream_id, stream_id);
					memcpy(&sh_audio->audio, &h, sizeof(h));
					sh_audio->stream_delay = (float)sh_audio->audio.dwStart*  sh_audio->audio.dwScale / sh_audio->audio.dwRate;
				}
				last_fccType = h.fccType;
				last_fccHandler = h.fccHandler;

				break;
			}
		case GX_FOURCC('i', 'n', 'd', 'x'):
			{
				uint32_t i;
				AVISuperIndexChunk* s;

				if (!index_mode)
					break;

				if (chunksize <= 24) {
					break;
				}
				priv->suidx_size++;
				priv->suidx = realloc_struct(priv->suidx, priv->suidx_size, sizeof(AVISuperIndexChunk));
				s = &priv->suidx[priv->suidx_size - 1];

				chunksize -= 24;
				memcpy(s->fcc, "indx", 4);
				s->dwSize = size2;
				s->wLongsPerEntry = GxStream_ReadWord_Le(demuxer->stream);
				s->bIndexSubType = GxStream_ReadChar(demuxer->stream);
				s->bIndexType = GxStream_ReadChar(demuxer->stream);
				s->nEntriesInUse = GxStream_ReadDword_Le(demuxer->stream);
				s->dwChunk = GxStream_ReadDword_Le(demuxer->stream);
				GxStream_Read(demuxer->stream, (uint8_t* ) s->dwReserved, 3 * 4);
				memset(s->dwReserved, 0, 3*  4);

				// Check and fix this useless crap
				if (s->wLongsPerEntry != sizeof(AVISuperIndexEntry) / 4) {
					s->wLongsPerEntry = sizeof(AVISuperIndexEntry) / 4;
				}
				if (((chunksize / 4) / s->wLongsPerEntry) < s->nEntriesInUse) {
					s->nEntriesInUse = (chunksize / 4) / s->wLongsPerEntry;
				}

				s->aIndex = av_calloc(s->nEntriesInUse, sizeof(AVISuperIndexEntry));
				s->stdidx = av_calloc(s->nEntriesInUse, sizeof(AVIStandardIndexChunk));

				// now the real index of indices
				for (i = 0; i < s->nEntriesInUse; i++) {
					chunksize -= 16;
					s->aIndex[i].qwOffset = GxStream_ReadQword_Le(demuxer->stream);
					s->aIndex[i].dwSize = GxStream_ReadDword_Le(demuxer->stream);
					s->aIndex[i].dwDuration = GxStream_ReadDword_Le(demuxer->stream);
				}

				break;
			}
		case ckidSTREAMFORMAT:
			{	// read 'strf'
				if (last_fccType == streamtypeVIDEO) {
					sh_video->bih = av_calloc(GXMAX(chunksize, sizeof(BITMAPINFOHEADER)), 1);
					GxStream_Read(demuxer->stream, (uint8_t* ) sh_video->bih, chunksize);
					le2me_BITMAPINFOHEADER(sh_video->bih);	// swap to machine endian
					if (sh_video->bih->biSize > chunksize
					    && sh_video->bih->biSize > sizeof(BITMAPINFOHEADER))
						sh_video->bih->biSize = chunksize;
					// fixup MS-RLE header (seems to be broken for <256 color files)
					if (sh_video->bih->biCompression <= 1 && sh_video->bih->biSize == 40)
						sh_video->bih->biSize = chunksize;
					chunksize = 0;
					sh_video->fps = (float)sh_video->video.dwRate / (float)sh_video->video.dwScale;
					sh_video->frametime = (float)sh_video->video.dwScale / (float)sh_video->video.dwRate;
					sh_video->disp_w = sh_video->bih->biWidth;
					sh_video->disp_h = sh_video->bih->biHeight;
					sh_video->aspect = (float)sh_video->disp_w/sh_video->disp_h;
					memset(sh_video->priv.codec,0,5);
					*(int*)sh_video->priv.codec = sh_video->bih->biCompression;

					idxfix_videostream = stream_id;
					switch (sh_video->bih->biCompression) {
					case GX_FOURCC('M', 'P', 'G', '4'):
					case GX_FOURCC('m', 'p', 'g', '4'):
					case GX_FOURCC('D', 'I', 'V', '1'):
						idxfix_divx = 3;	// set index recovery mpeg4 flavour: msmpeg4v1
						sh_video->format = 0x10000004;
						break;
					case GX_FOURCC('D', 'I', 'V', '3'):
					case GX_FOURCC('d', 'i', 'v', '3'):
					case GX_FOURCC('D', 'I', 'V', '4'):
					case GX_FOURCC('d', 'i', 'v', '4'):
					case GX_FOURCC('D', 'I', 'V', '5'):
					case GX_FOURCC('d', 'i', 'v', '5'):
					case GX_FOURCC('D', 'I', 'V', '6'):
					case GX_FOURCC('d', 'i', 'v', '6'):
					case GX_FOURCC('M', 'P', '4', '3'):
					case GX_FOURCC('m', 'p', '4', '3'):
					case GX_FOURCC('M', 'P', '4', '2'):
					case GX_FOURCC('m', 'p', '4', '2'):
					case GX_FOURCC('D', 'I', 'V', '2'):
					case GX_FOURCC('A', 'P', '4', '1'):
						idxfix_divx = 1;	// set index recovery mpeg4 flavour: msmpeg4v3
						sh_video->format = 0x10000004;
						break;
					case GX_FOURCC('D', 'I', 'V', 'X'):
					case GX_FOURCC('d', 'i', 'v', 'x'):
					case GX_FOURCC('D', 'X', '5', '0'):
					case GX_FOURCC('X', 'V', 'I', 'D'):
					case GX_FOURCC('x', 'v', 'i', 'd'):
					case GX_FOURCC('F', 'M', 'P', '4'):
					case GX_FOURCC('f', 'm', 'p', '4'):
						idxfix_divx = 2;	// set index recovery mpeg4 flavour: generic mpeg4
						sh_video->format = 0x10000004;
						break;
					case  GX_FOURCC('h', '2', '6', '4'):
					case  GX_FOURCC('H', '2', '6', '4'):
						sh_video->format = 0x10000005;
						break;
					}
				} else if (last_fccType == streamtypeAUDIO) {
					unsigned wf_size =
					    chunksize < sizeof(WAVEFORMATEX) ? sizeof(WAVEFORMATEX) : chunksize;
					sh_audio->wf = av_calloc(wf_size, 1);
					GxStream_Read(demuxer->stream, (uint8_t* ) sh_audio->wf, chunksize);
					le2me_WAVEFORMATEX(sh_audio->wf);
					if (sh_audio->wf->cbSize != 0  && wf_size < sizeof(WAVEFORMATEX) + sh_audio->wf->cbSize) {
						sh_audio->wf = av_realloc(sh_audio->wf, sizeof(WAVEFORMATEX) + sh_audio->wf->cbSize);
					}
					sh_audio->format = sh_audio->wf->wFormatTag;
					if (sh_audio->format == 1 && last_fccHandler == GX_FOURCC('A', 'x', 'a', 'n'))
						sh_audio->format = last_fccHandler;
					sh_audio->i_bps = sh_audio->wf->nAvgBytesPerSec;
					chunksize = 0;
					++priv->audio_streams;
					memset(sh_audio->priv.codec,0,4);
					*(short*)sh_audio->priv.codec = sh_audio->wf->wFormatTag;
				}
				break;
			}
		case GX_FOURCC('v', 'p', 'r', 'p'):
			{
				VideoPropHeader* vprp = av_malloc(chunksize);
				unsigned int i;
				GxStream_Read(demuxer->stream, (uint8_t* ) vprp, chunksize);
				le2me_VideoPropHeader(vprp);
				chunksize -= sizeof(*vprp) - sizeof(vprp->FieldInfo);
				chunksize /= sizeof(VideoFieldDescriptor);
				if (vprp->nbFieldPerFrame > chunksize) {
					vprp->nbFieldPerFrame = chunksize;
				}
				chunksize = 0;
				for (i = 0; i < vprp->nbFieldPerFrame; i++) {
					le2me_VideoFieldDescriptor(&vprp->FieldInfo[i]);
				}
				if (sh_video) {
					sh_video->aspect = GET_AVI_ASPECT(vprp->dwFrameAspectRatio);
				}
				av_free(vprp);
				break;
			}
		case GX_FOURCC('d', 'm', 'l', 'h'):
			{
				// dmlh 00 00 00 04 frms
				unsigned int total_frames;
				total_frames = GxStream_ReadDword_Le(demuxer->stream);

				GxStream_Skip(demuxer->stream, chunksize - 4);
				chunksize = 0;
			}
			break;
		case ckidAVINEWINDEX:
			if (demuxer->movi_end > GxStream_Tell(demuxer->stream))
				demuxer->movi_end = GxStream_Tell(demuxer->stream);	// fixup movi-end
			if (index_mode && !priv->isodml) {
				int i;
				priv->idx_size = size2 >> 4;
				priv->idx = av_malloc(priv->idx_size << 4);
				//      gxlogf("\nindex to %p !!!!! (priv=%p)\n",priv->idx,priv);
				GxStream_Read(demuxer->stream, priv->idx, priv->idx_size << 4);
				for (i = 0; i < priv->idx_size; i++) {	// swap index to machine endian
					AVIIndexEntry* entry = (AVIIndexEntry *) priv->idx + i;
					le2me_AVIIndexEntry(entry);
					/*
					*  We (ab)use the upper word for bits 32-47 of the offset, so
					*  we'll clear them here.
					*  FIXME: AFAIK no codec uses them, but if one does it will break
					*/
					entry->dwFlags &= 0xffff;
				}
				chunksize -= priv->idx_size << 4;
			}
			break;
			/* added May 2002*/
		case GX_FOURCC('R', 'I', 'F', 'F'):
			{
				uint8_t riff_type[4];

				GxStream_Read(demuxer->stream, riff_type, sizeof riff_type);
				if (!strncmp((char* )riff_type, "AVIX", sizeof riff_type)) {
					/*
					*  We got an extended AVI header, so we need to switch to
					*  ODML to get seeking to work, provided we got indx chunks
					*  in the header (suidx_size > 0).
					*/
					if (priv->suidx_size > 0)
						priv->isodml = 1;
				}
				chunksize = 0;
				list_end = 0;	/* a new list will follow*/
				break;
			}
		case ckidAVIPADDING:
			GxStream_Skip(demuxer->stream, chunksize);
			chunksize = 0;
			break;
		}
		if (hdr) {
			if (size2 == 3)
				chunksize = 1;	// empty
			else {
				uint8_t buf[256];
				int len = (size2 < 250) ? size2 : 250;
				GxStream_Read(demuxer->stream, buf, len);
				chunksize -= len;
				buf[len] = 0;

				gxlogf("%s:%s\n", hdr, (char* )buf);
			}
		}
		if (list_end > 0 && chunksize + GxStream_Tell(demuxer->stream) == list_end)
			list_end = 0;
		if (list_end > 0 && chunksize + GxStream_Tell(demuxer->stream) > list_end) {
			GxStream_Seek(demuxer->stream, list_end);
			list_end = 0;
		} else if (chunksize > 0)
			GxStream_Skip(demuxer->stream, chunksize);
	}

	if (priv->suidx_size > 0 && priv->idx_size == 0) {
		/*
		*  No NEWAVIINDEX, but we got an OpenDML index.
		*/
		priv->isodml = 1;
	}

	if (priv->isodml && (index_mode == -1 || index_mode == 0 || index_mode == 1)) {
		int i, j, k;

		AVISuperIndexChunk* cx;
		AVIIndexEntry* idx;

		if (priv->idx_size)
			av_free(priv->idx);
		priv->idx_size = 0;
		priv->idx_offset = 0;
		priv->idx = NULL;

		// read the standard indices
		for (cx = &priv->suidx[0], i = 0; i < priv->suidx_size; cx++, i++) {
			GxStream_Reset(demuxer->stream);
			for (j = 0; j < cx->nEntriesInUse; j++) {
				int ret1, ret2;
				memset(&cx->stdidx[j], 0, 32);
				ret1 = GxStream_Seek(demuxer->stream, (off_t) cx->aIndex[j].qwOffset);
				ret2 = GxStream_Read(demuxer->stream, (uint8_t* ) & cx->stdidx[j], 32);
				if (ret1 != 1 || ret2 != 32 || cx->stdidx[j].nEntriesInUse == 0) {
					// this is a broken file (probably incomplete) let the standard
					// gen_index routine handle this
					priv->isodml = 0;
					priv->idx_size = 0;
					goto freeout;
				}

				le2me_AVISTDIDXCHUNK(&cx->stdidx[j]);

				priv->idx_size += cx->stdidx[j].nEntriesInUse;
				cx->stdidx[j].aIndex =
				    av_malloc(cx->stdidx[j].nEntriesInUse*  sizeof(AVIStandardIndexEntry));
				GxStream_Read(demuxer->stream, (uint8_t* ) cx->stdidx[j].aIndex,
					      cx->stdidx[j].nEntriesInUse*  sizeof(AVIStandardIndexEntry));
				for (k = 0; k < cx->stdidx[j].nEntriesInUse; k++)
					le2me_AVISTDIDXENTRY(&cx->stdidx[j].aIndex[k]);

				cx->stdidx[j].dwReserved3 = 0;

			}
		}

		/*
		*  We convert the index by translating all entries into AVIIndexEntrys
		*  and sorting them by offset.  The result should be the same index
		*  we would get with -forceidx.
		*/

		idx = priv->idx = av_malloc(priv->idx_size*  sizeof(AVIIndexEntry));

		for (cx = priv->suidx; cx != &priv->suidx[priv->suidx_size]; cx++) {
			AVIStandardIndexChunk* sic;
			for (sic = cx->stdidx; sic != &cx->stdidx[cx->nEntriesInUse]; sic++) {
				AVIStandardIndexEntry* sie;
				for (sie = sic->aIndex; sie != &sic->aIndex[sic->nEntriesInUse]; sie++) {
					uint64_t off = sic->qwBaseOffset + sie->dwOffset - 8;
					memcpy(&idx->ckid, sic->dwChunkId, 4);
					idx->dwChunkOffset = off;
					idx->dwFlags = (off >> 32) << 16;
					idx->dwChunkLength = sie->dwSize & 0x7fffffff;
					idx->dwFlags |= (sie->dwSize & 0x80000000) ? 0x0 : AVIIF_KEYFRAME;	// bit 31 denotes !keyframe
					idx++;
				}
			}
		}
		qsort(priv->idx, priv->idx_size, sizeof(AVIIndexEntry), avi_idx_cmp);

		/*
		   Hack to work around a "wrong" index in some divx odml files
		   (processor_burning.avi as an example)
		   They have ##dc on non keyframes but the ix00 tells us they are ##db.
		   Read the fcc of a non-keyframe vid frame and check it.
		*/

		{
			uint32_t id;
			uint32_t db = 0;
			GxStream_Reset(demuxer->stream);

			// find out the video stream id. I have seen files with 01db.
			for (idx = &((AVIIndexEntry* ) priv->idx)[0], i = 0; i < priv->idx_size; i++, idx++) {
				unsigned char res[2];
				if (odml_get_vstream_id(idx->ckid, res)) {
					db = GX_FOURCC(res[0], res[1], 'd', 'b');
					break;
				}
			}

			// find first non keyframe
			for (idx = &((AVIIndexEntry* ) priv->idx)[0], i = 0; i < priv->idx_size; i++, idx++) {
				if (!(idx->dwFlags & AVIIF_KEYFRAME) && idx->ckid == db)
					break;
			}
			if (i < priv->idx_size && db) {
				GxStream_Seek(demuxer->stream, AVI_IDX_OFFSET(idx));
				id = GxStream_ReadDword_Le(demuxer->stream);
				if (id && id != db)	// index fcc and real fcc differ? fix it.
					for (idx = &((AVIIndexEntry* ) priv->idx)[0], i = 0; i < priv->idx_size;
					     i++, idx++) {
						if (!(idx->dwFlags & AVIIF_KEYFRAME) && idx->ckid == db)
							idx->ckid = id;
					}
			}
		}

		demuxer->movi_end = demuxer->stream->end_pos;

	      freeout:

		// free unneeded stuff
		cx = &priv->suidx[0];
		do {
			for (j = 0; j < cx->nEntriesInUse; j++)
				if (cx->stdidx[j].nEntriesInUse)
					av_free(cx->stdidx[j].aIndex);
			av_free(cx->stdidx);
		} while (cx++ != &priv->suidx[priv->suidx_size - 1]);
		av_free(priv->suidx);

	}

	if (index_mode >= 2 || (priv->idx_size == 0 && index_mode == 1)) {
		// build index for file:
		GxStream_Reset(demuxer->stream);
		GxStream_Seek(demuxer->stream, demuxer->movi_start);

		priv->idx_pos = 0;
		priv->idx_size = 0;
		priv->idx = NULL;

		while (1) {
			int id;
			unsigned len;
			off_t skip;
			AVIIndexEntry* idx;
			unsigned int c;
			demuxer->filepos = GxStream_Tell(demuxer->stream);
			if (demuxer->filepos >= demuxer->movi_end && demuxer->movi_start < demuxer->movi_end)
				break;
			id = GxStream_ReadDword_Le(demuxer->stream);
			len = GxStream_ReadDword_Le(demuxer->stream);
			if (id == GX_FOURCC('L', 'I', 'S', 'T') || id == GX_FOURCC('R', 'I', 'F', 'F')) {
				id = GxStream_ReadDword_Le(demuxer->stream);	// list or RIFF type
				continue;
			}
			if (GxStream_Eof(demuxer->stream))
				break;
			if (!id || avi_stream_id(id) == 100)
				goto skip_chunk;	// bad ID (or padding?)

			if (priv->idx_pos >= priv->idx_size) {
				//      priv->idx_size+=32;
				priv->idx_size += 1024;	// +16kB
				priv->idx = av_realloc(priv->idx, priv->idx_size*  sizeof(AVIIndexEntry));
				if (!priv->idx) {
					priv->idx_pos = 0;
					break;
				}	// error!
			}
			idx = &((AVIIndexEntry* ) priv->idx)[priv->idx_pos++];
			idx->ckid = id;
			idx->dwFlags = AVIIF_KEYFRAME;	// FIXME
			idx->dwFlags |= (((uint64_t)demuxer->filepos) >> 16) & 0xffff0000U;
			idx->dwChunkOffset = (unsigned long)demuxer->filepos;
			idx->dwChunkLength = len;

			c = GxStream_ReadDword(demuxer->stream);

			if (!len)
				idx->dwFlags &= ~AVIIF_KEYFRAME;

			// Fix keyframes for DivX files:
			if (idxfix_divx)
				if (avi_stream_id(id) == idxfix_videostream) {
					switch (idxfix_divx) {
					case 3:
						c = GxStream_ReadDword(demuxer->stream) << 5;	//skip 32+5 bits for m$mpeg4v1
					case 1:
						if (c & 0x40000000)
							idx->dwFlags &= ~AVIIF_KEYFRAME;
						break;	// divx 3
					case 2:
						if (c == 0x1B6)
							idx->dwFlags &= ~AVIIF_KEYFRAME;
						break;	// divx 4
					}
				}
			// update status line:
			{
				static off_t lastpos;
				off_t pos;
				off_t len = demuxer->movi_end - demuxer->movi_start;
				if (len) {
					pos = 100*  (demuxer->filepos - demuxer->movi_start) / len;	// %
				} else {
					pos = (((uint64_t)demuxer->filepos)- demuxer->movi_start) >> 20;	// MB
				}
				if (pos != lastpos) {
					lastpos = pos;
				}
			}
		      skip_chunk:
			skip = (len + 1) & (~1UL);	// total bytes in this chunk
			GxStream_Seek(demuxer->stream, 8 + demuxer->filepos + skip);
		}
		priv->idx_size = priv->idx_pos;
	}
}
