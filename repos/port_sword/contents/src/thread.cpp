/*
 * Licensed under CLIFE license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	CLIFE Development
 *
 * NAME:        thread.cpp ( CLIFE, C++ )
 *
 * COMMENTS:
 *	Attempt at a threading class.
 */

#include "thread.h"

#if defined(LINUX) || defined(__APPLE__)
#include "thread_linux.h"
#if defined(LINUX)
#include <sys/sysinfo.h>
#endif
#else
#include "thread_win.h"
#endif

THREAD *
THREAD::alloc()
{
#if defined(LINUX) || defined(__APPLE__)
    return new THREAD_LINUX();
#else
    return new THREAD_WIN();
#endif
}

int
THREAD::numProcessors()
{
    static int nproc = -1;

    if (nproc < 0)
    {
#if defined(__APPLE__)
	nproc = 1; // WHY ARE WE MULTITHREADING IN A ROGUELIKE ANYWAY
#elif defined(LINUX)
	nproc = get_nprocs_conf();
#else
	SYSTEM_INFO		sysinfo;
	GetSystemInfo(&sysinfo);
	nproc = sysinfo.dwNumberOfProcessors;
#endif

	// Zany users have managed to muck with thier registery to
	// get 0 procs returned...
	if (nproc < 1)
	    nproc = 1;
    }

    return nproc;
}

void *
THREAD::wrapper(void *data)
{
    THREAD *thread = (THREAD *) data;

    while (1)
    {
	// Await the thread to be ready.
	thread->waittillimready();

	thread->setActive(true);

	// Hurray!  Trigger callback.
	if (thread->myCB)
	    thread->myCB(thread->myCBData);

	thread->iamdonenow();

	thread->setActive(false);
    }

    return (void *)1;
}

LOCK::LOCK()
{
#if defined(LINUX) || defined(__APPLE__)
#ifndef PTHREAD_MUTEX_RECURSIVE_NP
#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#endif
    pthread_mutexattr_init(&myLockAttr);
    pthread_mutexattr_settype(&myLockAttr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&myLock, &myLockAttr);
#else
    myLock = new CRITICAL_SECTION;
    ::InitializeCriticalSection(myLock);
#endif
}

LOCK::~LOCK()
{
#if defined(LINUX) || defined(__APPLE__)
    pthread_mutex_destroy(&myLock);
    pthread_mutexattr_destroy(&myLockAttr);
#else
    DeleteCriticalSection(myLock);
    delete myLock;
#endif
}

bool
LOCK::tryToLock()
{
#if defined(LINUX) || defined(__APPLE__)
    return !pthread_mutex_trylock(&myLock);
#else
    return ::TryEnterCriticalSection(myLock) ? true : false;
#endif
}

void
LOCK::lock()
{
#if defined(LINUX) || defined(__APPLE__)
    pthread_mutex_lock(&myLock);
#else
    ::EnterCriticalSection(myLock);
#endif
}

void
LOCK::unlock()
{
#if defined(LINUX) || defined(__APPLE__)
    pthread_mutex_unlock(&myLock);
#else
    ::LeaveCriticalSection(myLock);
#endif
}

bool
LOCK::isLocked()
{
    if (tryToLock())
    {
	unlock();
	return false;
    }
    return true;
}

CONDITION::CONDITION()
{
#if defined(LINUX) || defined(__APPLE__)
    pthread_cond_init(&myCond, 0);
#else
    myEvent = CreateEvent(0, false, false, 0);
    myNumWaiting.set(0);
#endif
}

CONDITION::~CONDITION()
{
#if defined(LINUX) || defined(__APPLE__)
    pthread_cond_destroy(&myCond);
#else
    CloseHandle(myEvent);
#endif
}

void
CONDITION::wait(LOCK &lock)
{
#if defined(LINUX) || defined(__APPLE__)
    pthread_cond_wait(&myCond, &lock.myLock);
#else
    myNumWaiting.add(1);
    lock.unlock();
    WaitForSingleObject(myEvent, INFINITE);
    myNumWaiting.add(-1);
    lock.lock();
#endif
}

void
CONDITION::trigger()
{
#if defined(LINUX) || defined(__APPLE__)
    pthread_cond_signal(&myCond);
#else
    // This requires the caller has the lock active so no one
    // will start a wait during our test.
    if (myNumWaiting > 0)
	SetEvent(myEvent);
#endif
}
