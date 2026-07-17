/*
 * DASHManager.cpp
 *****************************************************************************
 * Copyright (C) 2012, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include "DASHManager.h"

using namespace libdash::framework::input;
using namespace libdash::framework::buffer;

using namespace dash;
using namespace dash::network;
using namespace dash::mpd;

DASHManager::DASHManager(uint32_t maxCapacity, IDASHManagerObserver* stream, IMPD* mpd, PLAYER_INTERRUPT_CBK _interruptcbk) :
	readSegmentCount        (0),
	receiver                (NULL),
	multimediaStream        (stream),
	isRunning               (false),
	mediaObject             (NULL),
	dashManagerInterruptcbk (_interruptcbk)
{
	this->buffer    = new MediaObjectBuffer(maxCapacity);
	this->buffer->AttachObserver(this);

	this->receiver  = new DASHReceiver(mpd, this, this->buffer, maxCapacity, _interruptcbk);
}

DASHManager::~DASHManager()
{
	this->Stop();
	delete this->receiver;
	delete this->buffer;

	this->receiver = NULL;
	this->buffer   = NULL;
}

bool DASHManager::Start()
{
	if (!this->receiver->Start())
		return false;

	this->isRunning = true;
	return true;
}

int DASHManager::Read(uint8_t *buf, int buf_size)
{
	int len = 0;

	if (!this->mediaObject)
		this->mediaObject = this->buffer->GetFront();

	if (!this->mediaObject)
		return -1;


	if (this->dashManagerInterruptcbk && this->dashManagerInterruptcbk())
		return -1;

	len = this->mediaObject->Read(buf, buf_size);

	if (len <= 0) {
		if ((this->readSegmentCount) &&
				((this->mediaObject->GetDownloadState() == COMPLETED) ||
				(this->mediaObject->GetDownloadState() == ABORTED))) {
			gxlogd("*******delete*********mediaObject:%p\n", this->mediaObject);
			delete this->mediaObject;
			this->mediaObject = NULL;
			this->readSegmentCount--;
		}
	}

	return len;
}

void DASHManager::Stop()
{
	if (!this->isRunning)
		return;

	this->isRunning = false;

	this->receiver->Stop();
	this->buffer->Clear();
}

uint32_t DASHManager::GetPosition()
{
	return this->receiver->GetPosition();
}

void DASHManager::SetPosition(uint32_t segmentNumber)
{
	this->receiver->SetPosition(segmentNumber);
}

void DASHManager::SetPositionInMsec(uint32_t milliSecs)
{
	this->receiver->SetPositionInMsecs(milliSecs);
}

void DASHManager::Clear()
{
	this->buffer->Clear();
}

void DASHManager::ClearTail()
{
	this->buffer->ClearTail();
}

bool DASHManager::GetReceiverState()
{
	return this->receiver->GetReceiverState();
}

void DASHManager::SetRepresentation(IPeriod *period, IAdaptationSet *adaptationSet, IRepresentation *representation)
{
	this->receiver->SetRepresentation(period, adaptationSet, representation);
	//this->buffer->ClearTail();
}

void DASHManager::EnqueueRepresentation(IPeriod *period, IAdaptationSet *adaptationSet, IRepresentation *representation)
{
	this->receiver->SetRepresentation(period, adaptationSet, representation);
}

void DASHManager::OnBufferStateChanged(uint32_t fillstateInPercent)
{
    this->multimediaStream->OnSegmentBufferStateChanged(fillstateInPercent);
}

void DASHManager::OnSegmentDownloaded()
{
    this->readSegmentCount++;
    // notify observers
}
