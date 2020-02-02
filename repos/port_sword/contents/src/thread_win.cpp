/*
 * Licensed under CLIFE license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	CLIFE Development
 *
 * NAME:        thread_win.h ( CLIFE, C++ )
 *
 * COMMENTS:
 *	Attempt at a threading class.
 */

#include "thread_win.h"

THREAD_WIN::THREAD_WIN()
{
    mySelf = 0;
    InitializeCriticalSection(&myCritSec);
    // Default to idle.
    myDoneEvent = CreateEvent(0, 1, 1, 0);
    // Default to not started
    myStartEvent = CreateEvent(0, 1, 0, 0);
}

THREAD_WIN::~THREAD_WIN()
{
    join();
    CloseHandle(myDoneEvent);
    CloseHandle(myStartEvent);
    CloseHandle(mySelf);
    mySelf = 0;
    DeleteCriticalSection(&myCritSec);
}

void
THREAD_WIN::start(THREADmainFunc func, void *data)
{
    DWORD	id;

    // If we are still running, block until the current task completes.
    if (mySelf)
	join();

    myCB = func;
    myCBData = data;
    if (!mySelf)
    {
	mySelf = CreateThread(NULL, 0,
		    (unsigned long(__stdcall *)(void *))THREAD::wrapper,
		    (void *)this, 0, &id);
    }

    jobsready();
}

void
THREAD_WIN::jobsready()
{
    EnterCriticalSection(&myCritSec);
    ResetEvent(myDoneEvent);
    SetEvent(myStartEvent);
    LeaveCriticalSection(&myCritSec);
}

void
THREAD_WIN::iamdonenow()
{
    EnterCriticalSection(&myCritSec);
    ResetEvent(myStartEvent);
    SetEvent(myDoneEvent);
    LeaveCriticalSection(&myCritSec);
}

void
THREAD_WIN::waittillimready()
{
    WaitForSingleObject(myStartEvent, INFINITE);
}

void
THREAD_WIN::join()
{
    WaitForSingleObject(myDoneEvent, INFINITE);
}

void
THREAD_WIN::kill()
{
    if (mySelf)
    {
	TerminateThread(mySelf, 0);
	CloseHandle(mySelf);
	mySelf = 0;
    }
}
