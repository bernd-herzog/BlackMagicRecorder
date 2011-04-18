/*
 * DeckLinkCaptureDelegate.h
 *
 *  Created on: 23.02.2011
 *      Author: j39f3fs
 */

#ifndef DECKLINKCAPTUREDELEGATE_H_
#define DECKLINKCAPTUREDELEGATE_H_

#include <DeckLinkAPI.h>
#include <pthread.h>
#include "SafeQueue.h"

class DeckLinkCaptureDelegate: public IDeckLinkInputCallback
{
public:
	DeckLinkCaptureDelegate(SafeQueue *);
	virtual ~DeckLinkCaptureDelegate();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv)
	{
		return E_NOINTERFACE;
	}
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);
	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*, BMDDetectedVideoInputFormatFlags);
	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame*, IDeckLinkAudioInputPacket*);
private:
	ULONG m_refCount;
	pthread_mutex_t m_mutex;
	SafeQueue *sq;
};

#endif /* DECKLINKCAPTUREDELEGATE_H_ */
