/*
 * Licensed under CLIFE license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	CLIFE Development
 *
 * NAME:        thread.h ( CLIFE, C++ )
 *
 * COMMENTS:
 *	Attempt at a threading class.
 */

#ifndef __thread_h__
#define __thread_h__

typedef void *(*THREADmainFunc)(void *);

class THREAD
{
public:
    // We use this generator to return the system specific implementation.
    static THREAD	*alloc();
    static int		 numProcessors();

    // (Re)starts this, invoking the given function as the main 
    // thread program.
    virtual void	 start(THREADmainFunc func, void *data) = 0;

    // Blocks until this is done
    virtual void	 join() = 0;

    // Is this thread still processing?
    bool	 	 isActive() const { return myActive; }

    // Terminates this thread.
    virtual void	 kill() = 0;
    
protected:
			 THREAD() { myActive = false; }
    virtual 		~THREAD() {}

    void		 setActive(bool val) { myActive = val; }

    // Blocks until the thread is ready to process.
    virtual void	 waittillimready() = 0;
    virtual void	 iamdonenow() = 0;

    // Canonical main func for threads.
    static void		*wrapper(void *data);

    // No public copying.
	    THREAD(const THREAD &) { }
    THREAD &operator=(const THREAD &) { return *this; }

    bool		 myActive; 
    THREADmainFunc	 myCB;
    void		*myCBData;
};

typedef int s32;

#if defined(__APPLE__)

#include <pthread.h>
#include <libkern/OSAtomic.h>

// Wacky mac specific stuff to replace the wacky linux specific stuff
inline s32
testandset(s32 *addr, s32 val)
{
	s32 result;
	do {
		result = *addr;
	} while (!OSAtomicCompareAndSwap32(result, val, addr));
    return result;
}

inline s32
testandadd(s32 *addr, s32 val)
{
    return OSAtomicAdd32(val, addr) - val; // -val to correct for difference with __sync_fetch_and_add
}

#elif defined(LINUX)

#include <pthread.h>

// Requires GCC 4.1 ++
// Consider this punishment for 4.3's behaviour with enums.
inline s32
testandset(s32 *addr, s32 val)
{
    return __sync_lock_test_and_set(addr, val);
}

inline s32
testandadd(s32 *addr, s32 val)
{
    return __sync_fetch_and_add(addr, val);
}

#else

#define _THREAD_SAFE
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <intrin.h>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#pragma intrinsic (_InterlockedExchange)
#pragma intrinsic (_InterlockedExchangeAdd)

inline s32
testandset(s32 *addr, s32 val)
{
    return (s32)_InterlockedExchange((long *)addr, (long)val);
}

inline s32
testandadd(s32 *addr, s32 val)
{
    return (s32)_InterlockedExchangeAdd((long *)addr, (long)val);
}

#endif

class ATOMIC_INT32
{
public:
    explicit ATOMIC_INT32(s32 value = 0) : myValue(value) {}

    // Swap this and val
    inline s32	exchange(s32 val)
    {
	return testandset(&myValue, val);
    }

    // Add val to this, return the old value.
    inline s32	exchangeAdd(s32 val)
    {
	return testandadd(&myValue, val);
    }

    // Add val to this, return new value.
    inline s32	add(s32 val)
    {
	return testandadd(&myValue, val) + val;
    }

    // Set self to maximum of self and val
    inline void	maximum(s32 val)
    {
	s32	peek = exchange(val);
	while (peek > val)
	{
	    val = peek;
	    peek = exchange(val);
	}
    }

    void	set(s32 val) { myValue = val; }

    operator	s32() const { return myValue; }
private:
    s32		myValue;

    ATOMIC_INT32(const ATOMIC_INT32 &) {}
    void operator=(const ATOMIC_INT32 &) {}
};


class LOCK
{
public:
	LOCK();
	~LOCK();

    // Non blocking attempt at the lock, returns true if now locked.
    bool	tryToLock();

    void	lock();
    void	unlock();
    bool	isLocked();

protected:
    LOCK(const LOCK &) {}
    void operator=(const LOCK &) {}

#if defined(LINUX) || defined(__APPLE__)
    pthread_mutex_t	 myLock;
    pthread_mutexattr_t	 myLockAttr;
#else
    CRITICAL_SECTION	*myLock;
#endif

    friend class CONDITION;

};

class AUTOLOCK
{
public:
    AUTOLOCK(LOCK &lock) : myLock(lock) { myLock.lock(); }
    ~AUTOLOCK() { myLock.unlock(); }

protected:
    LOCK	&myLock;
};

class CONDITION
{
public:
    CONDITION();
    ~CONDITION();

    // Lock is currently set.  We will unlock it, block, then
    // when condition is triggered relock it and return.
    void	wait(LOCK &lock);

    // Trigger, allowing one thread through.
    // Caller should currently have the wait lock.
    void	trigger();
private:

    CONDITION(const CONDITION &) {}
    void operator=(const CONDITION &) {}

#if defined(LINUX) || defined(__APPLE__)
    pthread_cond_t	myCond;
#else
    ATOMIC_INT32	myNumWaiting;
    HANDLE		myEvent;
#endif
};

#endif
