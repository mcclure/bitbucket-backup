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

#ifndef __threadlinux_h__
#define __threadlinux_h__

#include "thread.h"

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>

class THREAD_LINUX : public THREAD
{
public:
    // (Re)starts this, invoking the given function as the main 
    // thread program.
    virtual void	 start(THREADmainFunc func, void *data);

    // Blocks until this is done
    virtual void	 join();

    // Terminates this thread.
    virtual void	 kill();
    
			 THREAD_LINUX();
    virtual 		~THREAD_LINUX();
protected:
    // Blocks until the thread is ready to process.
    virtual void	 waittillimready();
    virtual void	 iamdonenow();
    virtual void	 jobsready();

    pthread_t		 mySelf;
    pthread_mutex_t	 myLock;
    pthread_cond_t	 myStateChangeEvent;
    bool		 myRunning;
    bool		 myExists;
};

#endif

