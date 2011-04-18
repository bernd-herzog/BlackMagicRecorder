/*
 * SafeQueue.cpp
 *
 *  Created on: 23.02.2011
 *      Author: j39f3fs
 */

#include "SafeQueue.h"

#include <cstring>
#include <iostream>

SafeQueue::SafeQueue(size_t len, size_t size) :
	queueLength(len), frameSize(size)
{
	pthread_mutex_init(&m_mutex, NULL);
	pthread_cond_init(&sleepCond, NULL);

	for (size_t i = 0; i <= len; i++)
	{
		this->free.push(new char[size]);
	}
}

SafeQueue::~SafeQueue()
{
	pthread_mutex_destroy(&m_mutex);
}

void SafeQueue::Enqueue(void *data)
{
	static int frame = 0;
	static int maxQueueLen = 0;
	static int framesDropped = 0;

	pthread_mutex_lock(&m_mutex);

	size_t queuesize = queue.size();
	if (queuesize > maxQueueLen)
		maxQueueLen = queuesize;

	if (queuesize >= this->queueLength)
	{
		//std::cout << " Buffer full! skipping!\n";
		framesDropped++;
	}
	else
	{
		std::cout << frame++ << ") waiting: " << queuesize << " maxQueueLen: " << maxQueueLen << " dropped: " << framesDropped << "\r";
		std::flush(std::cout);
		void *buf = free.back();
		free.pop();
		std::memcpy(buf, data, this->frameSize);
		queue.push(buf);
	}
	pthread_cond_signal(&sleepCond);
	pthread_mutex_unlock(&m_mutex);
}

void *SafeQueue::Dequeue(void)
{
	pthread_mutex_lock(&m_mutex);
	size_t size = queue.size();

	if (size == 0)
	{
		pthread_cond_wait(&sleepCond, &m_mutex);
	}

	void *ret = NULL;

	ret = queue.back();
	queue.pop();
	free.push(ret);
	pthread_mutex_unlock(&m_mutex);

	return ret;
}
