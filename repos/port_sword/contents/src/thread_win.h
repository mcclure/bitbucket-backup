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

#ifndef __threadwin_h__
#define __threadwin_h__

#include "thread.h"
#define _THREAD_SAFE
#define _WIN32_WINNT 0x0400
#include <windows.h>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

class THREAD_WIN : public THREAD
{
public:
    // (Re)starts this, invoking the given function as the main 
    // thread program.
    virtual void	 start(THREADmainFunc func, void *data);

    // Blocks until this is done
    virtual void	 join();

    // Terminates this thread.
    virtual void	 kill();
    
			 THREAD_WIN();
    virtual 		~THREAD_WIN();
protected:
    // Blocks until the thread is ready to process.
    virtual void	 waittillimready();
    virtual void	 iamdonenow();
    virtual void	 jobsready();


    CRITICAL_SECTION	 myCritSec;
    HANDLE		 myDoneEvent;
    HANDLE		 myStartEvent;
    HANDLE		 mySelf;
};

#endif

