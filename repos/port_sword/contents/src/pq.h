/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        pq.h ( Sword in Hand Library, C++ )
 *
 * COMMENTS:
 *	Implements a priority queue
 */

#ifndef __pq_h__
#define __pq_h__

#include "ptrlist.h"

template <typename T>
class PQ
{
public:
    class ITEM
    {
    public:
	ITEM() {}
	ITEM(int i) {}
	float	priority;
	T	data;
    };

    PQ();
    ~PQ();
    PQ(const PQ<T> &ref);
    PQ<T> &operator=(const PQ<T> &ref);

    // Adds a new item with a given priority.
    // We extract return the *lowest* priority.
    int		append(float priority, T item);

    // Empties
    void	clear() { myQ.clear(); }

    int		entries() const { return myQ.entries(); }

    // Remove the lowest priority item.
    T		pop();
    T		top() const { return myQ(0).data; }
    float	topPriority() const { return myQ(0).priority; }

protected:
    // idx may be lower than its parents, raise it!
    void	bubbleUp(int idx);
    // idx is *invalid*, raise its children into its spot.
    void	bubbleDown(int idx);


    PTRLIST<ITEM>	myQ;
};

// All platforms are crappy now.
#include "pq.cpp"

#endif


