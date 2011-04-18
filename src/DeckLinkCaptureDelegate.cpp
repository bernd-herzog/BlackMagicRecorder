/*
 * DeckLinkCaptureDelegate.cpp
 *
 *  Created on: 23.02.2011
 *      Author: j39f3fs
 */

#include "DeckLinkCaptureDelegate.h"
#include <stdio.h>

DeckLinkCaptureDelegate::DeckLinkCaptureDelegate(SafeQueue *sq) :
	m_refCount(0), sq(sq)
{
	pthread_mutex_init(&m_mutex, NULL);

}

DeckLinkCaptureDelegate::~DeckLinkCaptureDelegate()
{
	pthread_mutex_destroy(&m_mutex);
}

ULONG DeckLinkCaptureDelegate::AddRef(void)
{
	pthread_mutex_lock(&m_mutex);
	m_refCount++;
	pthread_mutex_unlock(&m_mutex);

	return (ULONG) m_refCount;
}

ULONG DeckLinkCaptureDelegate::Release(void)
{
	pthread_mutex_lock(&m_mutex);
	m_refCount--;
	pthread_mutex_unlock(&m_mutex);

	if (m_refCount == 0)
	{
		delete this;
		return 0;
	}

	return (ULONG) m_refCount;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket* audioFrame)
{
	//printf("2DeckLinkCaptureDelegate::VideoInputFrameArrived\n");

	void* frameBytes;
	//void* audioFrameBytes;

	// Handle Video Frame
	if (videoFrame)
	{
		if (videoFrame->GetFlags() & bmdFrameHasNoInputSource)
		{
			//fprintf(stderr, "2Frame received - No input signal detected\n");
		}
		else
		{
			size_t len = videoFrame->GetRowBytes() * videoFrame->GetHeight();
			videoFrame->GetBytes(&frameBytes);
			sq->Enqueue(frameBytes);
		}
	}

	// Handle Audio Frame
	/*if (audioFrame)
	 {
	 if (audioOutputFile != -1)
	 {
	 audioFrame->GetBytes(&audioFrameBytes);
	 write(audioOutputFile, audioFrameBytes, audioFrame->GetSampleFrameCount() * g_audioChannels * (g_audioSampleDepth / 8));
	 }
	 }*/
	return S_OK;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode *mode, BMDDetectedVideoInputFormatFlags)
{
	return S_OK;
}
