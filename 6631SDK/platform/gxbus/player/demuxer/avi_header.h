#ifndef _AVI_HEADER_H
#define _AVI_HEADER_H

#include "gx_common.h"
#include "mpeg_header.h"
#include "../avutil/bswap.h"


/* form types, list types, and chunk types */
#define formtypeAVI             GX_FOURCC('A', 'V', 'I', ' ')
#define listtypeAVIHEADER       GX_FOURCC('h', 'd', 'r', 'l')
#define ckidAVIMAINHDR          GX_FOURCC('a', 'v', 'i', 'h')
#define listtypeSTREAMHEADER    GX_FOURCC('s', 't', 'r', 'l')
#define ckidSTREAMHEADER        GX_FOURCC('s', 't', 'r', 'h')
#define ckidSTREAMFORMAT        GX_FOURCC('s', 't', 'r', 'f')
#define ckidSTREAMHANDLERDATA   GX_FOURCC('s', 't', 'r', 'd')
#define ckidSTREAMNAME			GX_FOURCC('s', 't', 'r', 'n')

#define listtypeAVIMOVIE        GX_FOURCC('m', 'o', 'v', 'i')
#define listtypeAVIRECORD       GX_FOURCC('r', 'e', 'c', ' ')

#define ckidAVINEWINDEX         GX_FOURCC('i', 'd', 'x', '1')

/*
** Stream types for the <fccType> field of the stream header.
*/
#define streamtypeVIDEO         GX_FOURCC('v', 'i', 'd', 's')
#define streamtypeAUDIO         GX_FOURCC('a', 'u', 'd', 's')
#define streamtypeMIDI			GX_FOURCC('m', 'i', 'd', 's')
#define streamtypeTEXT          GX_FOURCC('t', 'x', 't', 's')

/* Basic chunk types */
#define cktypeDIBbits           GX_TWOCC('d', 'b')
#define cktypeDIBcompressed     GX_TWOCC('d', 'c')
#define cktypePALchange         GX_TWOCC('p', 'c')
#define cktypeWAVEbytes         GX_TWOCC('w', 'b')

/* Chunk id to use for extra chunks for padding. */
#define ckidAVIPADDING          GX_FOURCC('J', 'U', 'N', 'K')

/* flags for use in <dwFlags> in AVIFileHdr */
#define AVIF_HASINDEX			0x00000010	// Index at end of file?
#define AVIF_MUSTUSEINDEX		0x00000020
#define AVIF_ISINTERLEAVED		0x00000100
#define AVIF_TRUSTCKTYPE		0x00000800	// Use CKType to find key frames?
#define AVIF_WASCAPTUREFILE		0x00010000
#define AVIF_COPYRIGHTED		0x00020000

/* Flags for index */
#define AVIIF_LIST          	0x00000001L	// chunk is a 'LIST'
#define AVIIF_KEYFRAME      	0x00000010L	// this frame is a key frame.

#define AVIIF_NOTIME	    	0x00000100L	// this frame doesn't take any time
#define AVIIF_COMPUSE       	0x0FFF0000L	// these bits are for compressor use

#define FOURCC_RIFF     		GX_FOURCC('R', 'I', 'F', 'F')
#define FOURCC_LIST   			GX_FOURCC('L', 'I', 'S', 'T')

typedef struct {
	uint32_t dwMicroSecPerFrame;	// frame display rate (or 0L)
	uint32_t dwMaxBytesPerSec;	// max. transfer rate
	uint32_t dwPaddingGranularity;	// pad to multiples of this
	// size; normally 2K.
	uint32_t dwFlags;	// the ever-present flags
	uint32_t dwTotalFrames;	// # frames in file
	uint32_t dwInitialFrames;
	uint32_t dwStreams;
	uint32_t dwSuggestedBufferSize;

	uint32_t dwWidth;
	uint32_t dwHeight;

	uint32_t dwReserved[4];
} MainAVIHeader;

typedef struct {
	uint32_t ckid;
	uint32_t dwFlags;
	uint32_t dwChunkOffset;	// Position of chunk
	uint32_t dwChunkLength;	// Length of chunk
} AVIIndexEntry;

typedef struct _AVISuperIndexEntry {
	uint64_t qwOffset;	// absolute file offset
	uint32_t dwSize;	// size of index chunk at this offset
	uint32_t dwDuration;	// time span in stream ticks
} AVISuperIndexEntry;

typedef struct _AVIStandardIndexEntry {
	uint32_t dwOffset;	// qwBaseOffset + this is absolute file offset
	uint32_t dwSize;	// bit 31 is set if this is NOT a keyframe
} AVIStandardIndexEntry;

// Standard index
typedef struct __attribute__ ((packed)) _AVIStandardIndexChunk {
	char fcc[4];		// ix##
	uint32_t dwSize;	// size of this chunk
	uint16_t wLongsPerEntry;	// must be sizeof(aIndex[0])/sizeof(DWORD)
	uint8_t bIndexSubType;	// must be 0
	uint8_t bIndexType;	// must be AVI_INDEX_OF_CHUNKS
	uint32_t nEntriesInUse;	// first unused entry
	char dwChunkId[4];	// '##dc' or '##db' or '##wb' etc..
	uint64_t qwBaseOffset;	// all dwOffsets in aIndex array are relative to this
	uint32_t dwReserved3;	// must be 0
	AVIStandardIndexEntry *aIndex;	// the actual frames
} AVIStandardIndexChunk;

// Base Index Form 'indx'
typedef struct _AVISuperIndexChunk {
	char fcc[4];
	uint32_t dwSize;	// size of this chunk
	uint16_t wLongsPerEntry;	// size of each entry in aIndex array (must be 4*4 for us)
	uint8_t bIndexSubType;	// future use. must be 0
	uint8_t bIndexType;	// one of AVI_INDEX_* codes
	uint32_t nEntriesInUse;	// index of first unused member in aIndex array
	union{
	char dwChunkId[4];	// fcc of what is indexed
	int  dwChunk;
	};
	uint32_t dwReserved[3];	// meaning differs for each index type/subtype.
	// 0 if unused
	AVISuperIndexEntry *aIndex;	// position of ix## chunks
	AVIStandardIndexChunk *stdidx;	// the actual std indices
} AVISuperIndexChunk;

typedef struct {
	uint32_t CompressedBMHeight;
	uint32_t CompressedBMWidth;
	uint32_t ValidBMHeight;
	uint32_t ValidBMWidth;
	uint32_t ValidBMXOffset;
	uint32_t ValidBMYOffset;
	uint32_t VideoXOffsetInT;
	uint32_t VideoYValidStartLine;
} VideoFieldDescriptor;

typedef struct {
	uint32_t VideoFormatToken;
	uint32_t VideoStandard;
	uint32_t dwVerticalRefreshRate;
	uint32_t dwHTotalInT;
	uint32_t dwVTotalInLines;
	uint32_t dwFrameAspectRatio;
	uint32_t dwFrameWidthInPixels;
	uint32_t dwFrameHeightInLines;
	uint32_t nbFieldPerFrame;
	VideoFieldDescriptor FieldInfo[2];
} VideoPropHeader;

#define GET_AVI_ASPECT(a) ((float)((a)>>16)/(float)((a)&0xffff))

/*
 * Some macros to swap little endian structures read from an AVI file
 * into machine endian format
 */
#ifdef WORDS_BIGENDIAN
#define	le2me_MainAVIHeader(h) {					\
    (h)->dwMicroSecPerFrame = le2me_32((h)->dwMicroSecPerFrame);	\
    (h)->dwMaxBytesPerSec = le2me_32((h)->dwMaxBytesPerSec);		\
    (h)->dwPaddingGranularity = le2me_32((h)->dwPaddingGranularity);	\
    (h)->dwFlags = le2me_32((h)->dwFlags);				\
    (h)->dwTotalFrames = le2me_32((h)->dwTotalFrames);			\
    (h)->dwInitialFrames = le2me_32((h)->dwInitialFrames);		\
    (h)->dwStreams = le2me_32((h)->dwStreams);				\
    (h)->dwSuggestedBufferSize = le2me_32((h)->dwSuggestedBufferSize);	\
    (h)->dwWidth = le2me_32((h)->dwWidth);				\
    (h)->dwHeight = le2me_32((h)->dwHeight);				\
}

#define	le2me_AVIStreamHeader(h) {					\
    (h)->fccType = le2me_32((h)->fccType);				\
    (h)->fccHandler = le2me_32((h)->fccHandler);			\
    (h)->dwFlags = le2me_32((h)->dwFlags);				\
    (h)->wPriority = le2me_16((h)->wPriority);				\
    (h)->wLanguage = le2me_16((h)->wLanguage);				\
    (h)->dwInitialFrames = le2me_32((h)->dwInitialFrames);		\
    (h)->dwScale = le2me_32((h)->dwScale);				\
    (h)->dwRate = le2me_32((h)->dwRate);				\
    (h)->dwStart = le2me_32((h)->dwStart);				\
    (h)->dwLength = le2me_32((h)->dwLength);				\
    (h)->dwSuggestedBufferSize = le2me_32((h)->dwSuggestedBufferSize);	\
    (h)->dwQuality = le2me_32((h)->dwQuality);				\
    (h)->dwSampleSize = le2me_32((h)->dwSampleSize);			\
    le2me_RECT(&(h)->rcFrame);						\
}
#define	le2me_RECT(h) {							\
    (h)->left = le2me_16((h)->left);					\
    (h)->top = le2me_16((h)->top);					\
    (h)->right = le2me_16((h)->right);					\
    (h)->bottom = le2me_16((h)->bottom);				\
}
#define le2me_BITMAPINFOHEADER(h) {					\
    (h)->biSize = le2me_32((h)->biSize);				\
    (h)->biWidth = le2me_32((h)->biWidth);				\
    (h)->biHeight = le2me_32((h)->biHeight);				\
    (h)->biPlanes = le2me_16((h)->biPlanes);				\
    (h)->biBitCount = le2me_16((h)->biBitCount);			\
    (h)->biCompression = le2me_32((h)->biCompression);			\
    (h)->biSizeImage = le2me_32((h)->biSizeImage);			\
    (h)->biXPelsPerMeter = le2me_32((h)->biXPelsPerMeter);		\
    (h)->biYPelsPerMeter = le2me_32((h)->biYPelsPerMeter);		\
    (h)->biClrUsed = le2me_32((h)->biClrUsed);				\
    (h)->biClrImportant = le2me_32((h)->biClrImportant);		\
}
#define le2me_WAVEFORMATEX(h) {						\
    (h)->wFormatTag = le2me_16((h)->wFormatTag);			\
    (h)->nChannels = le2me_16((h)->nChannels);				\
    (h)->nSamplesPerSec = le2me_32((h)->nSamplesPerSec);		\
    (h)->nAvgBytesPerSec = le2me_32((h)->nAvgBytesPerSec);		\
    (h)->nBlockAlign = le2me_16((h)->nBlockAlign);			\
    (h)->wBitsPerSample = le2me_16((h)->wBitsPerSample);		\
    (h)->cbSize = le2me_16((h)->cbSize);				\
}
#define le2me_AVIIndexEntry(h) {					\
    (h)->ckid = le2me_32((h)->ckid);					\
    (h)->dwFlags = le2me_32((h)->dwFlags);				\
    (h)->dwChunkOffset = le2me_32((h)->dwChunkOffset);			\
    (h)->dwChunkLength = le2me_32((h)->dwChunkLength);			\
}
#define le2me_AVISTDIDXCHUNK(h) {\
    char c; \
    c = (h)->fcc[0]; (h)->fcc[0] = (h)->fcc[3]; (h)->fcc[3] = c;  \
    c = (h)->fcc[1]; (h)->fcc[1] = (h)->fcc[2]; (h)->fcc[2] = c;  \
    (h)->dwSize = le2me_32((h)->dwSize);  \
    (h)->wLongsPerEntry = le2me_16((h)->wLongsPerEntry);  \
    (h)->nEntriesInUse = le2me_32((h)->nEntriesInUse);  \
    c = (h)->dwChunkId[0]; (h)->dwChunkId[0] = (h)->dwChunkId[3]; (h)->dwChunkId[3] = c;  \
    c = (h)->dwChunkId[1]; (h)->dwChunkId[1] = (h)->dwChunkId[2]; (h)->dwChunkId[2] = c;  \
    (h)->qwBaseOffset = le2me_64((h)->qwBaseOffset);  \
    (h)->dwReserved3 = le2me_32((h)->dwReserved3);  \
}
#define le2me_AVISTDIDXENTRY(h)  {\
    (h)->dwOffset = le2me_32((h)->dwOffset);  \
    (h)->dwSize = le2me_32((h)->dwSize);  \
}
#define le2me_VideoPropHeader(h) {					\
    (h)->VideoFormatToken = le2me_32((h)->VideoFormatToken);		\
    (h)->VideoStandard = le2me_32((h)->VideoStandard);			\
    (h)->dwVerticalRefreshRate = le2me_32((h)->dwVerticalRefreshRate);	\
    (h)->dwHTotalInT = le2me_32((h)->dwHTotalInT);			\
    (h)->dwVTotalInLines = le2me_32((h)->dwVTotalInLines);		\
    (h)->dwFrameAspectRatio = le2me_32((h)->dwFrameAspectRatio);	\
    (h)->dwFrameWidthInPixels = le2me_32((h)->dwFrameWidthInPixels);	\
    (h)->dwFrameHeightInLines = le2me_32((h)->dwFrameHeightInLines);	\
    (h)->nbFieldPerFrame = le2me_32((h)->nbFieldPerFrame);		\
}
#define le2me_VideoFieldDescriptor(h) {					\
    (h)->CompressedBMHeight = le2me_32((h)->CompressedBMHeight);	\
    (h)->CompressedBMWidth = le2me_32((h)->CompressedBMWidth);		\
    (h)->ValidBMHeight = le2me_32((h)->ValidBMHeight);			\
    (h)->ValidBMWidth = le2me_32((h)->ValidBMWidth);			\
    (h)->ValidBMXOffset = le2me_32((h)->ValidBMXOffset);		\
    (h)->ValidBMYOffset = le2me_32((h)->ValidBMYOffset);		\
    (h)->VideoXOffsetInT = le2me_32((h)->VideoXOffsetInT);		\
    (h)->VideoYValidStartLine = le2me_32((h)->VideoYValidStartLine);	\
}

#else
#define	le2me_MainAVIHeader(h) /**/
#define le2me_AVIStreamHeader(h) /**/
#define le2me_RECT(h) /**/
#define le2me_BITMAPINFOHEADER(h) /**/
#define le2me_WAVEFORMATEX(h) /**/
#define le2me_AVIIndexEntry(h) /**/
#define le2me_AVISTDIDXCHUNK(h) /**/
#define le2me_AVISTDIDXENTRY(h) /**/
#define le2me_VideoPropHeader(h) /**/
#define le2me_VideoFieldDescriptor(h) /**/
#endif
extern inline int avi_stream_id(unsigned int id);
extern void read_avi_header(GxDemuxer * demuxer, int index_mode);

#define AVI_IDX_OFFSET(x) ((((uint64_t)(x)->dwFlags&0xffff0000)<<16)+(x)->dwChunkOffset)

#endif				/* _aviheader_h */
