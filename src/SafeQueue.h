/*
 * SafeQueue.h
 *
 *  Created on: 23.02.2011
 *      Author: j39f3fs
 */

#include <queue>
#include <pthread.h>

#ifndef SAFEQUEUE_H_
#define SAFEQUEUE_H_

class SafeQueue
{
public:
	SafeQueue(size_t, size_t);
	virtual ~SafeQueue();

	void Enqueue(void *);
	void *Dequeue(void);
private:
	std::queue<void *> queue;
	std::queue<void *> free;
	pthread_mutex_t m_mutex;
	pthread_cond_t sleepCond;
	size_t queueLength;
  size_t frameSize;
};

#endif /* SAFEQUEUE_H_ */
