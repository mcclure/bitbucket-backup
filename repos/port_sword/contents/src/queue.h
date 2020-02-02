/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        queue.h ( Letter Hunt Library, C++ )
 *
 * COMMENTS:
 *	Implements a simple queue.
 *	Is threadsafe.  Has the ability to block until an item
 * 	is available.
 */


#ifndef __queue_h__
#define __queue_h__

#include "ptrlist.h"
#include "thread.h"

template <typename PTR>
class QUEUE
{
public:
    QUEUE() {}
    ~QUEUE() {}

    // Because we are multithreaded, this is not meant to remain valid
    // outside of a lock.
    bool		isEmpty() const
    {
	return myList.entries() == 0;
    }

    // Removes item, returns false if failed because queue is empty.
    bool		remove(PTR &item)
    {
	AUTOLOCK	l(myLock);

	if (isEmpty())
	    return false;
	item = myList.removeFirst();
	return true;
    }

    // Blocks till an element is ready.
    PTR			waitAndRemove()
    {
	AUTOLOCK	l(myLock);
	PTR		result;

	while (!remove(result))
	{
	    myCond.wait(myLock);
	}
	return result;
    }
    
    // Empties the queue.
    void		clear()
    {
	AUTOLOCK	l(myLock);
	myList.clear();
	// Don't trigger!
    }

    void		append(const PTR &item)
    {
	AUTOLOCK	l(myLock);
	myList.append(item);
	myCond.trigger();
    }

private:
    // Copying locks scares me.
    QUEUE(const QUEUE<PTR> &ref) {}
    QUEUE<PTR> &operator=(const QUEUE<PTR> &ref) {}

    PTRLIST<PTR>	myList;
    LOCK		myLock;
    CONDITION		myCond;
};

#endif
