/*
 * DASHReceiver.cpp
 *****************************************************************************
 * Copyright (C) 2012, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include "DASHReceiver.h"

using namespace libdash::framework::input;
using namespace libdash::framework::buffer;
using namespace libdash::framework::mpd;
using namespace dash::mpd;

DASHReceiver::DASHReceiver          (IMPD *mpd, IDASHReceiverObserver *obs, MediaObjectBuffer *buffer, uint32_t bufferSize, PLAYER_INTERRUPT_CBK _interruptcbk) :
	dashReceiverInterruptcbk (_interruptcbk),
	buffer                   (buffer),
	observer                 (obs),
	mpd                      (mpd),
	period                   (NULL),
	adaptationSet            (NULL),
	representation           (NULL),
	adaptationSetStream      (NULL),
	representationStream     (NULL),
	segmentNumber            (0),
	bufferSize               (bufferSize),
	bufferingThread          (-1),
	isBuffering              (false)
{
	this->period                = this->mpd->GetPeriods().at(0);
	this->adaptationSet         = this->period->GetAdaptationSets().at(0);
	this->representation        = this->adaptationSet->GetRepresentation().at(0);

	this->adaptationSetStream   = new AdaptationSetStream(mpd, period, adaptationSet);
	this->representationStream  = adaptationSetStream->GetRepresentationStream(this->representation);
	this->segmentOffset         = CalculateSegmentOffset();

	InitializeCriticalSection(&this->monitorMutex);
}

DASHReceiver::~DASHReceiver()
{
	delete this->adaptationSetStream;
	DeleteCriticalSection(&this->monitorMutex);
}

bool DASHReceiver::Start()
{
	if (this->isBuffering)
		return false;

	this->isBuffering       = true;
	this->bufferingThread   = CreateThread(DoBuffering, this);

	if (this->bufferingThread == -1) {
		this->isBuffering = false;
		return false;
	}

	return true;
}

void DASHReceiver::Stop()
{
	this->isBuffering = false;
	this->buffer->SetEOS(true);

	if (this->bufferingThread != -1) {
		JoinThread(this->bufferingThread);
	}
}

MediaObject* DASHReceiver::GetNextSegment()
{
	ISegment *seg = NULL;

	if (this->segmentNumber >= this->representationStream->GetSize())
		return NULL;

	seg = this->representationStream->GetMediaSegment(this->segmentNumber + this->segmentOffset);

	if (seg != NULL) {
		MediaObject *media = new MediaObject(seg, this->representation, this->dashReceiverInterruptcbk);
		this->segmentNumber++;
		return media;
	}

	return NULL;
}

MediaObject* DASHReceiver::GetSegment(uint32_t segNum)
{
	ISegment *seg = NULL;

	if (segNum >= this->representationStream->GetSize())
		return NULL;

	seg = this->representationStream->GetMediaSegment(segNum + segmentOffset);

	if (seg != NULL) {
		MediaObject *media = new MediaObject(seg, this->representation, this->dashReceiverInterruptcbk);
		return media;
	}

	return NULL;
}

MediaObject* DASHReceiver::GetInitSegment()
{
	ISegment *seg = NULL;

	seg = this->representationStream->GetInitializationSegment();

	if (seg != NULL) {
		MediaObject *media = new MediaObject(seg, this->representation, this->dashReceiverInterruptcbk);
		return media;
	}

	return NULL;
}

MediaObject* DASHReceiver::FindInitSegment(dash::mpd::IRepresentation *representation)
{
	if (!this->InitSegmentExists(representation))
		return NULL;

	return this->initSegments[representation];
}

uint32_t DASHReceiver::GetPosition()
{
	return this->segmentNumber;
}

void DASHReceiver::SetPosition(uint32_t segmentNumber)
{
	this->segmentNumber = segmentNumber;
}

void DASHReceiver::SetPositionInMsecs(uint32_t milliSecs)
{
	this->positionInMsecs = milliSecs;
}

void DASHReceiver::SetRepresentation(IPeriod *period, IAdaptationSet *adaptationSet, IRepresentation *representation)
{
	EnterCriticalSection(&this->monitorMutex);

	bool periodChanged = false;

	if (this->representation == representation) {
		LeaveCriticalSection(&this->monitorMutex);
		return;
	}

	this->representation = representation;

	if (this->adaptationSet != adaptationSet) {
		this->adaptationSet = adaptationSet;

		if (this->period != period) {
			this->period = period;
			periodChanged = true;
		}

		delete this->adaptationSetStream;
		this->adaptationSetStream = NULL;

		this->adaptationSetStream = new AdaptationSetStream(this->mpd, this->period, this->adaptationSet);
	}

	this->representationStream  = this->adaptationSetStream->GetRepresentationStream(this->representation);
	this->DownloadInitSegment(this->representation);

	if (periodChanged) {
		this->segmentNumber = 0;
		this->CalculateSegmentOffset();
	}

	LeaveCriticalSection(&this->monitorMutex);
}

dash::mpd::IRepresentation* DASHReceiver::GetRepresentation()
{
	return this->representation;
}

bool DASHReceiver::GetReceiverState()
{
	return this->isBuffering;
}

uint32_t DASHReceiver::CalculateSegmentOffset()
{
	if (mpd->GetType() == "static")
		return 0;

	uint32_t firstSegNum = this->representationStream->GetFirstSegmentNumber();
	uint32_t currSegNum  = this->representationStream->GetCurrentSegmentNumber();
	uint32_t startSegNum = currSegNum - 2*bufferSize;

	return (startSegNum > firstSegNum) ? startSegNum : firstSegNum;
}

void DASHReceiver::NotifySegmentDownloaded   ()
{
    this->observer->OnSegmentDownloaded();
}

void DASHReceiver::DownloadInitSegment(IRepresentation* rep)
{
	if (this->InitSegmentExists(rep))
		return;

	MediaObject *initSeg = NULL;
	initSeg = this->GetInitSegment();

	if (initSeg) {
		initSeg->StartDownload();
		this->initSegments[rep] = initSeg;
		this->buffer->PushBack(initSeg);
	}
}

bool DASHReceiver::InitSegmentExists(IRepresentation* rep)
{
	if (this->initSegments.find(rep) != this->initSegments.end())
		return true;

	return false;
}

/* Thread that does the buffering of segments */
void DASHReceiver::DoBuffering(void *receiver)
{
	DASHReceiver *dashReceiver = (DASHReceiver *) receiver;

	dashReceiver->DownloadInitSegment(dashReceiver->GetRepresentation());

	MediaObject *media = dashReceiver->GetNextSegment();

	while (media != NULL && dashReceiver->isBuffering) {

		if(!media->StartDownload())
			break;

		if (!dashReceiver->buffer->PushBack(media))
			return;

		media->WaitFinished();

		if (dashReceiver->dashReceiverInterruptcbk
				&& dashReceiver->dashReceiverInterruptcbk())
			break;

		dashReceiver->NotifySegmentDownloaded();
		media = dashReceiver->GetNextSegment();
	}

	dashReceiver->isBuffering = false;
	dashReceiver->buffer->SetEOS(true);
	return;
}
