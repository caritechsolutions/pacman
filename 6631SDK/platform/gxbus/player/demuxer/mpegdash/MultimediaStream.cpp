/*
 * MultimediaStream.cpp
 *****************************************************************************
 * Copyright (C) 2013, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

extern "C" {
#include <stdlib.h>
#include "gx_stream.h"
#include "MultimediaStream.h"
}

#include "libdash/IMPD.h"
#include "libdash/IPeriod.h"
#include "libdash/IBaseUrl.h"
#include "Input/DASHManager.h"
#include "Buffer/IBufferObserver.h"
#include "Input/IDASHManagerObserver.h"
#include "Buffer/Buffer.h"

using namespace libdash::framework::input;
using namespace libdash::framework::buffer;
using namespace dash::mpd;

class MultimediaStream : public libdash::framework::input::IDASHManagerObserver
{
public:
	MultimediaStream            (dash::mpd::IMPD *mpd, PLAYER_INTERRUPT_CBK _interruptcbk);
	virtual ~MultimediaStream   ();

	void        StopDownload            ();
	bool        StartDownload           ();
	int         Read                    (uint8_t *buf, int buf_size);
	void        Clear                   ();
	uint32_t    GetPosition             ();
	void        SetPosition             (uint32_t segmentNumber);
	void        SetPositionInMsec       (uint32_t milliSecs);
	void        OnSegmentBufferStateChanged   (uint32_t fillstateInPercent);

	void        SetRepresentation       (int period, int adaptationSet, int representationSet);
	void        EnqueueRepresentation   (int period, int adaptationSet, int representationSet);
	void        AddBaseUrl              (const char *url);
	bool        GetDownloadState        ();

private:
	dash::mpd::IMPD                                     *mpd;
	libdash::framework::input::DASHManager              *dashManager;
	PLAYER_INTERRUPT_CBK                                MultimediaStreamInterruptcbk;

};

struct MpegDashManager {
	dash::IDASHManager *manager;
	dash::mpd::IMPD    *mpd;
	MultimediaStream   *videoStream;
	MultimediaStream   *audioStream;
	MultimediaStream   *subtitleStream;
	unsigned int       perNum;
	MpegDashPeriod     *per;
};

MultimediaStream::MultimediaStream(IMPD *mpd, PLAYER_INTERRUPT_CBK _interruptcbk) :
	mpd                          (mpd),
	dashManager                  (NULL),
	MultimediaStreamInterruptcbk (_interruptcbk)
{
	this->dashManager = new DASHManager(128*1024, this, this->mpd, _interruptcbk);
}

MultimediaStream::~MultimediaStream()
{
    this->StopDownload();

	if (this->dashManager) {
	   delete this->dashManager;
	   this->dashManager = NULL;
	}
}

uint32_t MultimediaStream::GetPosition()
{
    return this->dashManager->GetPosition();
}

void MultimediaStream::SetPosition(uint32_t segmentNumber)
{
    this->dashManager->SetPosition(segmentNumber);
}

void MultimediaStream::SetPositionInMsec(uint32_t milliSecs)
{
    this->dashManager->SetPositionInMsec(milliSecs);
}

void MultimediaStream::OnSegmentBufferStateChanged(uint32_t fillstateInPercent)
{
	return;
}

bool MultimediaStream::StartDownload()
{
    if(!this->dashManager->Start())
        return false;

    return true;
}

void MultimediaStream::StopDownload()
{
    this->dashManager->Stop();
}

void MultimediaStream::Clear()
{
    this->dashManager->Clear();
}

int MultimediaStream::Read(uint8_t *buf, int buf_size)
{
	return this->dashManager->Read(buf, buf_size);
}

void MultimediaStream::SetRepresentation(int period, int adaptationSet, int representationSet)
{
	IPeriod *currentPeriod = this->mpd->GetPeriods().at(0);
	std::vector<IAdaptationSet *> adaptationSets = currentPeriod->GetAdaptationSets();

	if (adaptationSet >= 0 && representationSet >= 0)
		this->dashManager->SetRepresentation(currentPeriod,
				adaptationSets.at(adaptationSet),
				adaptationSets.at(adaptationSet)->GetRepresentation().at(representationSet));

	return;
}

void MultimediaStream::EnqueueRepresentation(int period, int adaptationSet, int representationSet)
{
	IPeriod *currentPeriod = this->mpd->GetPeriods().at(period);
    this->dashManager->EnqueueRepresentation(currentPeriod, NULL, NULL);
}

void MultimediaStream::AddBaseUrl(const char *url)
{
	return mpd->SetBaseUrl(url);
}

bool MultimediaStream::GetDownloadState()
{
	return this->dashManager->GetReceiverState();
}

static int __mpegdash_atoi(char *src, const char* opt)
{
	int i = 0, j = 0;
	char *aa = src;
	while (*aa++ != *opt)
		i++;

	char bb[i];
	memset(bb, '\0', i);

	while(j < i) {
		bb[j] = src[j];
		j++;
	}

	return atoi(bb);
}

static IPeriod *__mpegdash_get_period(MpegDashManager *dash, int period)
{
	return dash->mpd->GetPeriods().at(period);
}

static int __mpegdash_get_period_num(MpegDashManager *dash)
{
	return dash->mpd->GetPeriods().size();
}

static IAdaptationSet *__mpegdash_get_adaptationset(MpegDashManager *dash, int period, int adaptation)
{
	IPeriod  *currentPeriod = dash->mpd->GetPeriods().at(period);

	if (currentPeriod)
		return currentPeriod->GetAdaptationSets().at(adaptation);

	return NULL;
}

static int __mpegdash_get_adaptationset_num(MpegDashManager *dash, int period)
{
	IPeriod  *currentPeriod = dash->mpd->GetPeriods().at(period);

	if (currentPeriod)
		return currentPeriod->GetAdaptationSets().size();

	return 0;
}

static IRepresentation *__mpegdash_get_representation(MpegDashManager *dash,
		int period, int adaptation, int representation)
{
	IPeriod  *currentPeriod = dash->mpd->GetPeriods().at(period);

	if (currentPeriod) {
		IAdaptationSet *currentAdaptation = currentPeriod->GetAdaptationSets().at(adaptation);
		if (currentAdaptation)
			return currentAdaptation->GetRepresentation().at(representation);
	}

	return NULL;
}

static int __mpegdash_get_representation_num(MpegDashManager *dash, int period, int adaptation)
{
	IPeriod  *currentPeriod = dash->mpd->GetPeriods().at(period);

	if (currentPeriod) {
		IAdaptationSet *currentAdaptation = currentPeriod->GetAdaptationSets().at(adaptation);
		if (currentAdaptation)
			return currentAdaptation->GetRepresentation().size();
	}

	return 0;
}

static int __mpegdash_set_formation(MpegDashAdaptation *adp, int rep_index)
{
	MpegDashFormation *fmt = NULL;

	if ((adp == NULL) || (adp->iadp == NULL))
		return -1;

	if (rep_index == -1) {
		IAdaptationSet *fields = (IAdaptationSet *)adp->iadp;
		fmt = &(adp->fmt);
		const char *mimeString = fields->GetMimeType().c_str();

		if (!strcasecmp(mimeString, ""))
			fmt->mimeType = MPEGDASH_UNKNOWN;
		else if (!strcasecmp(mimeString, "audio/mp4"))
			fmt->mimeType = MPEGDASH_AUDIO;
		else if (!strcasecmp(mimeString, "video/mp4"))
			fmt->mimeType = MPEGDASH_VIDEO;
		else
			fmt->mimeType = MPEGDASH_UNKNOWN;


		if (fmt->mimeType == MPEGDASH_AUDIO) {
			fmt->u.audio.sampleRate = atoi(fields->GetAudioSamplingRate().c_str());

			if (fields->GetCodecs().size() == 0)
				return 0;
			const char *codecString = fields->GetCodecs().at(0).c_str();

			if (!strcasecmp(codecString, ""))
				fmt->u.audio.audioCodec = MPEGDASH_CODEC_UNKNOWN;
			else if (!strncasecmp(codecString, "mp4a", 4))
				fmt->u.audio.audioCodec = MPEGDASH_AUDIO_CODEC_AAC_LATM;
			else
				fmt->u.audio.audioCodec = MPEGDASH_CODEC_UNKNOWN;

		} else if (fmt->mimeType == MPEGDASH_VIDEO) {
			fmt->u.video.width     = fields->GetWidth();
			fmt->u.video.height    = fields->GetHeight();
			fmt->u.video.frameRate = atoi(fields->GetFrameRate().c_str());

			if (fields->GetCodecs().size() == 0)
				return 0;
			const char *codecString = fields->GetCodecs().at(0).c_str();

			if (!strcasecmp(codecString, ""))
				fmt->u.video.videoCodec = MPEGDASH_CODEC_UNKNOWN;
			else if (!strncasecmp(codecString, "mp4v", 4))
				fmt->u.video.videoCodec = MPEGDASH_VIDEO_CODEC_MPEG4;
			else if (!strncasecmp(codecString, "hev", 3))
				fmt->u.video.videoCodec = MPEGDASH_VIDEO_CODEC_H265;
			else if (!strncasecmp(codecString, "avc", 3))
				fmt->u.video.videoCodec = MPEGDASH_VIDEO_CODEC_H264;
			else
				fmt->u.video.videoCodec = MPEGDASH_CODEC_UNKNOWN;
		}
	} else {
		MpegDashRepresentation *rep    = &(adp->rep[rep_index]);
		IRepresentation        *fields = (IRepresentation*)rep->irep;
		fmt = &(rep->fmt);

		const char *mimeString = fields->GetMimeType().c_str();
		if (!strcasecmp(mimeString, ""))
			fmt->mimeType = MPEGDASH_UNKNOWN;
		else if (!strcasecmp(mimeString, "audio/mp4"))
			fmt->mimeType = MPEGDASH_AUDIO;
		else if (!strcasecmp(mimeString, "video/mp4"))
			fmt->mimeType = MPEGDASH_VIDEO;
		else
			fmt->mimeType = MPEGDASH_UNKNOWN;

		if (fmt->mimeType == MPEGDASH_UNKNOWN)
			fmt->mimeType = adp->fmt.mimeType;

		if (fmt->mimeType == MPEGDASH_AUDIO) {
			fmt->u.audio.sampleRate = atoi(fields->GetAudioSamplingRate().c_str());
			fmt->u.audio.bandWidth  = fields->GetBandwidth();

			if (fields->GetCodecs().size() == 0)
				return 0;
			const char *codecString = fields->GetCodecs().at(0).c_str();

			if (!strcasecmp(codecString, ""))
				fmt->u.audio.audioCodec = MPEGDASH_CODEC_UNKNOWN;
			else if (!strncasecmp(codecString, "mp4a", 4))
				fmt->u.audio.audioCodec = MPEGDASH_AUDIO_CODEC_AAC_LATM;
			else
				fmt->u.audio.audioCodec = MPEGDASH_CODEC_UNKNOWN;

		} else if (fmt->mimeType == MPEGDASH_VIDEO) {
			fmt->u.video.width     = fields->GetWidth();
			fmt->u.video.height    = fields->GetHeight();
			fmt->u.video.frameRate = atoi(fields->GetFrameRate().c_str());
			fmt->u.video.bandWidth = fields->GetBandwidth();

			if (fields->GetCodecs().size() == 0)
				return 0;
			const char *codecString = fields->GetCodecs().at(0).c_str();

			if (!strcasecmp(codecString, ""))
				fmt->u.video.videoCodec = MPEGDASH_CODEC_UNKNOWN;
			else if (!strncasecmp(codecString, "mp4v", 4))
				fmt->u.video.videoCodec = MPEGDASH_VIDEO_CODEC_MPEG4;
			else if (!strncasecmp(codecString, "hev", 3))
				fmt->u.video.videoCodec = MPEGDASH_VIDEO_CODEC_H265;
			else if (!strncasecmp(codecString, "avc", 3))
				fmt->u.video.videoCodec = MPEGDASH_VIDEO_CODEC_H264;
			else
				fmt->u.video.videoCodec = MPEGDASH_CODEC_UNKNOWN;
		}
	}

	return 0;
}

static int __mpegdash_parse(MpegDashManager *dash)
{
	unsigned int i = 0, j = 0, k = 0;

	dash->perNum = __mpegdash_get_period_num(dash);
	if (!dash->perNum)
		return -1;

	dash->per = (MpegDashPeriod *)av_mallocz(dash->perNum * sizeof(MpegDashPeriod));
	if (dash->per == NULL)
		return -1;

	for (i = 0; i < dash->perNum; i++) {
		dash->per[i].iper   = (void*)__mpegdash_get_period(dash, i);
		dash->per[i].adpNum = __mpegdash_get_adaptationset_num(dash, i);
		dash->per[i].adp    = (MpegDashAdaptation*)av_mallocz(dash->per[i].adpNum * sizeof(MpegDashAdaptation));
		for (j = 0; j < dash->per[i].adpNum; j++) {
			dash->per[i].adp[j].iadp = (void*)__mpegdash_get_adaptationset(dash, i, j);
			__mpegdash_set_formation(&(dash->per[i].adp[j]), -1);

			dash->per[i].adp[j].repNum = __mpegdash_get_representation_num(dash, i, j);
			dash->per[i].adp[j].rep    = (MpegDashRepresentation*)av_mallocz(dash->per[i].adp[j].repNum *sizeof(MpegDashRepresentation));
			for (k = 0; k < dash->per[i].adp[j].repNum; k++) {
				dash->per[i].adp[j].rep[k].irep = (void*)__mpegdash_get_representation(dash, i, j, k);
				__mpegdash_set_formation(&(dash->per[i].adp[j]), k);
			}

			dash->per[i].adp[j].lang = NULL;
		}
	}

	return 0;
}

static void __mpegdash_delete(MpegDashManager *dash)
{
	unsigned int i = 0, j = 0;

	for (i = 0; i < dash->perNum; i++) {
		if (dash->per == NULL) //huangjx
			continue;

		for (j = 0; j < dash->per[i].adpNum; j++) {
			if (dash->per[i].adp[j].rep) {
				av_free(dash->per[i].adp[j].rep);
				dash->per[i].adp[j].rep    = NULL;
				dash->per[i].adp[j].repNum = 0;
			}
		}

		if (dash->per[i].adp) {
			av_free(dash->per[i].adp);
			dash->per[i].adp    = NULL;
			dash->per[i].adpNum = 0;
		}
	}

	if (dash->per) {
		av_free(dash->per);
		dash->per    = NULL;
		dash->perNum = 0;
	}

	return;
}

extern "C" void* MpegDash_Open(const char *url)
{
	MpegDashManager *dash = (MpegDashManager*)av_mallocz(sizeof(MpegDashManager));

	if (dash == NULL)
		return NULL;

	dash->manager = CreateDashManager();
	dash->mpd     = dash->manager->Open((char *)url);
	if (dash->mpd == NULL)
		goto open_error;

	if (__mpegdash_parse(dash) < 0)
		goto open_error;

	return dash;

open_error:
	__mpegdash_delete(dash);
	dash->manager->Delete();
	delete dash;
	return NULL;
}

extern "C" int MpegDash_Config(void *priv,
		MpegDashMimeType type, int period, int adaptationSet, int representationSet, PLAYER_INTERRUPT_CBK _interruptcbk)
{
	MpegDashManager *dash = (MpegDashManager *)priv;

	if ((type & MPEGDASH_AUDIO) == MPEGDASH_AUDIO) {
		dash->audioStream = new MultimediaStream(dash->mpd, _interruptcbk);
		dash->audioStream->SetRepresentation(period, adaptationSet, representationSet);
	} else if ((type & MPEGDASH_VIDEO) == MPEGDASH_VIDEO) {
		dash->videoStream = new MultimediaStream(dash->mpd, _interruptcbk);
		dash->videoStream->SetRepresentation(period, adaptationSet, representationSet);
	} else if ((type & MPEGDASH_SUBTITLE) == MPEGDASH_SUBTITLE) {
		dash->subtitleStream = new MultimediaStream(dash->mpd, _interruptcbk);
		dash->subtitleStream->SetRepresentation(period, adaptationSet, representationSet);
	}

	return 0;
}

extern "C" int MpegDash_SetBaseUrl(void *priv, const char *url)
{
	MpegDashManager *dash = (MpegDashManager *)priv;

	if (url == NULL)
		return 0;

	if (strstr(url, "://") == NULL)
		return 0;

	dash->mpd->SetBaseUrl(url);

	return 0;
}

extern "C" int MpegDash_Start(void *priv, MpegDashMimeType type)
{
	MpegDashManager *dash = (MpegDashManager *)priv;

	if ((type & MPEGDASH_AUDIO) == MPEGDASH_AUDIO) {
		if (dash->audioStream == NULL)
			return -1;
		if (dash->audioStream->StartDownload() == false)
			return -1;
	} else if ((type & MPEGDASH_VIDEO) == MPEGDASH_VIDEO) {
		if (dash->videoStream == NULL)
			return -1;
		if (dash->videoStream->StartDownload() == false)
			return -1;
	}

	return 0;
}

extern "C" int MpegDash_Stop(void *priv, MpegDashMimeType type)
{
	MpegDashManager *dash = (MpegDashManager *)priv;

	if ((type & MPEGDASH_AUDIO) == MPEGDASH_AUDIO) {
		if (dash->audioStream == NULL)
			return -1;
		dash->audioStream->StopDownload(); // void >> bool ?

	} else if ((type & MPEGDASH_VIDEO) == MPEGDASH_VIDEO) {
		if (dash->videoStream == NULL)
			return -1;
		dash->videoStream->StopDownload(); // void >> bool ?
	}
	return 0;
}

extern "C" int MpegDash_StartAll(void *priv)
{
	MpegDashManager *dash = (MpegDashManager *)priv;

	if (dash->videoStream)
		dash->videoStream->StartDownload();

	if (dash->audioStream)
		dash->audioStream->StartDownload();

	if (dash->subtitleStream)
		dash->subtitleStream->StartDownload();

	return 0;
}

extern "C" int MpegDash_StopAll(void *priv)
{
	MpegDashManager *dash = (MpegDashManager *)priv;

	if (dash->audioStream)
		dash->audioStream->StopDownload();

	if (dash->videoStream)
		dash->videoStream->StopDownload();

	return 0;
}

extern "C" int MpegDash_Read(void *priv, MpegDashMimeType type, uint8_t *buf, int size)
{
	MpegDashManager *dash = (MpegDashManager *)priv;

	if ((type & MPEGDASH_VIDEO) == MPEGDASH_VIDEO) {
		if (dash->videoStream == NULL)
			return -1;
		return dash->videoStream->Read(buf, size);
	} else if ((type & MPEGDASH_AUDIO) == MPEGDASH_AUDIO) {
		if (dash->audioStream == NULL)
			return -1;
		return dash->audioStream->Read(buf, size);
	}

	return -1;
}

extern "C" void MpegDash_Close(void *priv)
{
	MpegDashManager *dash = (MpegDashManager *)priv;

	if (dash->videoStream) {
		delete dash->videoStream;
		dash->videoStream = NULL;
	}

	if (dash->audioStream) {
		delete dash->audioStream;
		dash->audioStream = NULL;
	}

	__mpegdash_delete(dash);
	dash->manager->Delete();
	if (dash)
		av_free(dash);

	return;
}

extern "C" int MpegDash_GetDuration(void *priv, int *duration)
{
	MpegDashManager *dash = (MpegDashManager *)priv;
	char *cDuration = (char*)dash->mpd->GetMediaPresentationDuration().c_str();

	if (!strncmp(cDuration, "PT", 2)) {
		int hour = 0, min = 0, sec = 0;
		cDuration += 2;
		if (strstr(cDuration, "H")) {
			hour = __mpegdash_atoi(cDuration, "H");
			cDuration = strstr(cDuration, "H") + 1;
		}
		if (strstr(cDuration, "M")) {
			min  = __mpegdash_atoi(cDuration, "M");
			cDuration = strstr(cDuration, "M") + 1;
		}
		if (strstr(cDuration, "S")) {
			sec  = __mpegdash_atoi(cDuration, "S");
		}
		*duration = hour * 24 + min * 60 + sec;
	} else {
		*duration = 0;
	}

	return 0;
}

extern "C" int MpegDash_GetPeriods(void *priv, MpegDashPeriod **period, unsigned int *periodNum)
{
	MpegDashManager *dash = (MpegDashManager *)priv;

	if (period)
		*period = dash->per;

	if (periodNum)
		*periodNum = dash->perNum;

	return 0;
}

extern "C" MpegDashMimeType MpegDash_GetMimeType(void *priv, int period, int adaptationSet, int representationSet)
{
	MpegDashManager *dash = (MpegDashManager *)priv;

	if (dash->perNum <= (unsigned int)period)
		return MPEGDASH_UNKNOWN;
	if (dash->per[period].adpNum <= (unsigned int)adaptationSet)
		return MPEGDASH_UNKNOWN;
	if (dash->per[period].adp[adaptationSet].repNum <= (unsigned int)representationSet)
		return MPEGDASH_UNKNOWN;

	if (dash->per[period].adp[adaptationSet].fmt.mimeType == MPEGDASH_UNKNOWN)
		return dash->per[period].adp[adaptationSet].rep[representationSet].fmt.mimeType;
	return dash->per[period].adp[adaptationSet].fmt.mimeType;
}

extern "C" bool MpegDash_GetDownloadState(void *priv, MpegDashMimeType type)
{
	MpegDashManager *dash = (MpegDashManager *)priv;

	if ((type & MPEGDASH_AUDIO) == MPEGDASH_AUDIO)
		return dash->audioStream->GetDownloadState();
	else if ((type & MPEGDASH_VIDEO) == MPEGDASH_VIDEO)
		return dash->videoStream->GetDownloadState();
	else
		return false;
}
