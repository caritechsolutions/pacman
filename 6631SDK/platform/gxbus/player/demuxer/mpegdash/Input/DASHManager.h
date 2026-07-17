/*
 * DASHManager.h
 *****************************************************************************
 * Copyright (C) 2012, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#ifndef LIBDASH_FRAMEWORK_INPUT_DASHMANAGER_H_
#define LIBDASH_FRAMEWORK_INPUT_DASHMANAGER_H_

#include "../Buffer/MediaObjectBuffer.h"
#include "DASHReceiver.h"
#include "IDASHReceiverObserver.h"
#include "libdash/libdash.h"
#include "libdash/IMPD.h"
#include "IDASHManagerObserver.h"
#include "../Buffer/IMediaObjectBufferObserver.h"

namespace libdash
{
	namespace framework
	{
		namespace input
		{
			class DASHManager : public IDASHReceiverObserver,  public buffer::IMediaObjectBufferObserver
			{
			public:
				DASHManager             (uint32_t maxCapacity, IDASHManagerObserver *multimediaStream, dash::mpd::IMPD *mpd, PLAYER_INTERRUPT_CBK _interruptcbk);
				virtual ~DASHManager    ();

				bool        Start                   ();
				void        Stop                    ();
				int         Read                    (uint8_t *buf, int buf_size);
				uint32_t    GetPosition             ();
				void        SetPosition             (uint32_t segmentNumber); // to implement
				void        SetPositionInMsec       (uint32_t millisec);
				void        Clear                   ();
				void        ClearTail               ();
				void        SetRepresentation       (dash::mpd::IPeriod *period, dash::mpd::IAdaptationSet *adaptationSet, dash::mpd::IRepresentation *representation);
				void        EnqueueRepresentation   (dash::mpd::IPeriod *period, dash::mpd::IAdaptationSet *adaptationSet, dash::mpd::IRepresentation *representation);

				void        OnSegmentDownloaded     ();
				void        OnBufferStateChanged    (uint32_t fillstateInPercent);

				bool        GetReceiverState        ();
			private:
                uint32_t                    readSegmentCount;
				buffer::MediaObjectBuffer   *buffer;
				DASHReceiver                *receiver;
				IDASHManagerObserver        *multimediaStream;
				bool                        isRunning;
				MediaObject                 *mediaObject;
				PLAYER_INTERRUPT_CBK        dashManagerInterruptcbk;
			};
		}
	}
}

#endif /* LIBDASH_FRAMEWORK_INPUT_DASHMANAGER_H_ */
