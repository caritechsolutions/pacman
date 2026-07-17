/*
 * MultimediaStream.h
 *****************************************************************************
 * Copyright (C) 2013, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#ifndef __MULTI_MEDIA_STREAM_H__
#define __MULTI_MEDIA_STREAM_H__

typedef enum {
	MPEGDASH_VIDEO    = (1 << 0),
	MPEGDASH_AUDIO    = (1 << 1),
	MPEGDASH_SUBTITLE = (1 << 2),
	MPEGDASH_DATA     = (1 << 3),
	MPEGDASH_STREAM   = (1 << 4),

	MPEGDASH_UNKNOWN  = 0xffffffff,
} MpegDashMimeType;

typedef enum {
	MPEGDASH_VIDEO_CODEC_MPEG12 = 0,
	MPEGDASH_VIDEO_CODEC_MPEG4,
	MPEGDASH_VIDEO_CODEC_H263,
	MPEGDASH_VIDEO_CODEC_H264,
	MPEGDASH_VIDEO_CODEC_REAL,
	MPEGDASH_VIDEO_CODEC_AVS,
	MPEGDASH_VIDEO_CODEC_H265,
	MPEGDASH_VIDEO_CODEC_VC1,
	MPEGDASH_VIDEO_CODEC_MJPEG,
	MPEGDASH_VIDEO_CODEC_DIV3,

	MPEGDASH_AUDIO_CODEC_MPEG1 = 0x100,
	MPEGDASH_AUDIO_CODEC_MPEG2,
	MPEGDASH_AUDIO_CODEC_AAC_LATM,
	MPEGDASH_AUDIO_CODEC_AAC_ADTS,
	MPEGDASH_AUDIO_CODEC_VORBIS,
	MPEGDASH_AUDIO_CODEC_AC3,
	MPEGDASH_AUDIO_CODEC_AVS,
	MPEGDASH_AUDIO_CODEC_PCM,
	MPEGDASH_AUDIO_CODEC_EAC3,
	MPEGDASH_AUDIO_CODEC_DTS,
	MPEGDASH_AUDIO_CODEC_DRA1,
	MPEGDASH_AUDIO_CODEC_DTS_HD,
	MPEGDASH_AUDIO_CODEC_REAL,
	MPEGDASH_AUDIO_CODEC_RA_AAC,
	MPEGDASH_AUDIO_CODEC_RA_RA8LBR,

	MPEGDASH_CODEC_UNKNOWN = (0xFFFFFFFF),
} MpegDashCodec;

typedef struct {
	unsigned int  bandWidth;
	unsigned int  width;
	unsigned int  height;
	unsigned int  frameRate;
	MpegDashCodec videoCodec;
} MpegDashVideoStream;

typedef struct {
	unsigned int  bandWidth;
	unsigned int  sampleRate;
	MpegDashCodec audioCodec;
} MpegDashAudioStream;

typedef struct {
	MpegDashMimeType mimeType;
	union {
		MpegDashVideoStream video;
		MpegDashAudioStream audio;
	} u;
} MpegDashFormation;

typedef struct {
	MpegDashFormation fmt;
	void              *irep;
} MpegDashRepresentation;

typedef struct {
	unsigned int              repNum;
	const char                *lang;
	MpegDashRepresentation    *rep;
	MpegDashFormation         fmt;
	void                      *iadp;
} MpegDashAdaptation;

typedef struct {
	unsigned int       adpNum;
	MpegDashAdaptation *adp;
	void               *iper;
} MpegDashPeriod;

extern void* MpegDash_Open(const char *url);
extern int   MpegDash_GetPeriods (void *priv, MpegDashPeriod **period, unsigned int *periodNum);
extern int   MpegDash_Config     (void *priv,
		MpegDashMimeType type, int period, int adaptationSet, int representationSet, PLAYER_INTERRUPT_CBK _interruptcbk);
extern int   MpegDash_SetBaseUrl (void *priv, const char *url);
extern int   MpegDash_Start      (void *priv, MpegDashMimeType type);
extern int   MpegDash_Stop       (void *priv, MpegDashMimeType type);
extern int   MpegDash_StartAll   (void *priv);
extern int   MpegDash_StopAll    (void *priv);
extern int   MpegDash_Read       (void *priv, MpegDashMimeType type, uint8_t *buf, int size);
extern void  MpegDash_Close      (void *priv);
extern int   MpegDash_GetDuration(void *priv, int *duration);
extern MpegDashMimeType MpegDash_GetMimeType(void *priv, int period, int adaptationSet, int representationSet);
extern bool  MpegDash_GetDownloadState(void *priv, MpegDashMimeType type);

#endif
