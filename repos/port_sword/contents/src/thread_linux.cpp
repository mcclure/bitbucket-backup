/*
 * Licensed under CLIFE license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	CLIFE Development
 *
 * NAME:        thread_linux.h ( CLIFE, C++ )
 *
 * COMMENTS:
 *	Attempt at a threading class.
 */

#include "thread_linux.h"

THREAD_LINUX::THREAD_LINUX()
{
    myExists = false;
    myRunning = false;
}

THREAD_LINUX::~THREAD_LINUX()
{
    if (isActive())
	join();

    if (myExists)
	pthread_cancel(mySelf);
}

void
THREAD_LINUX::start(THREADmainFunc func, void *data)
{
    // If we are still running, block until the current task completes.
    if (isActive())
	join();

    myCB = func;
    myCBData = data;
    if (!myExists)
    {
	pthread_attr_t		attr;

	pthread_cond_init(&myStateChangeEvent, 0);
	pthread_mutex_init(&myLock, 0);

	pthread_attr_init(&attr);

	myExists = true;

	pthread_create(&mySelf, &attr, THREAD::wrapper, this);
	pthread_attr_destroy(&attr);
    }

    jobsready();
}

void
THREAD_LINUX::jobsready()
{
    pthread_mutex_lock(&myLock);
    myRunning = true;
    pthread_cond_broadcast(&myStateChangeEvent);
    pthread_mutex_unlock(&myLock);
}

void
THREAD_LINUX::iamdonenow()
{
    pthread_mutex_lock(&myLock);
    myRunning = false;
    pthread_cond_broadcast(&myStateChangeEvent);
    pthread_mutex_unlock(&myLock);
}

void
THREAD_LINUX::waittillimready()
{
    pthread_mutex_lock(&myLock);
    while (!myRunning)
	pthread_cond_wait(&myStateChangeEvent, &myLock);
    pthread_mutex_unlock(&myLock);
}

void
THREAD_LINUX::join()
{
    pthread_mutex_lock(&myLock);
    while (myRunning)
	pthread_cond_wait(&myStateChangeEvent, &myLock);
    pthread_mutex_unlock(&myLock);
}

void
THREAD_LINUX::kill()
{
    if (myExists)
    {
	pthread_cancel(mySelf);
	myExists = false;
    }
}
