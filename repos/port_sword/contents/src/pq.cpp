/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        pq.cpp ( Sword in Hand Library, C++ )
 *
 * COMMENTS:
 *	Implements a priority queue
 */

template <typename T>
PQ<T>::PQ()
{
}

template <typename T>
PQ<T>::~PQ()
{
}

template <typename T>
PQ<T>::PQ(const PQ<T> &ref)
{
    *this = ref;
}

template <typename T>
PQ<T> &
PQ<T>::operator=(const PQ<T> &ref)
{
    myQ = ref.myQ;
}

template <typename T>
int
PQ<T>::append(float priority, T item)
{
    ITEM	ni;
    ni.priority = priority;
    ni.data = item;
    myQ.append(ni);

    bubbleUp(entries()-1);
    return entries()-1;
}

template <typename T>
T
PQ<T>::pop()
{
    T		result = top();

    bubbleDown(0);

    // Total etnries reduced...
    myQ.pop();

    return result;
}


template <typename T>
void
PQ<T>::bubbleUp(int idx)
{
    // Root is trivial.
    if (!idx)
	return;
    int		parentidx = (idx+1) >> 1;
    if (myQ(parentidx).priority > myQ(idx).priority)
    {
	// Bad pair!
	myQ.swapEntries(parentidx, idx);
	bubbleUp(parentidx);
    }
}

template <typename T>
void
PQ<T>::bubbleDown(int idx)
{
    int		child1, child2;
    child1 = (idx << 1) + 1;
    child2 = child1 + 1;

    // Check if we are at the tail.
    if (child1 >= entries())
    {
	// Are we the last entry?
	if (idx == entries() - 1)
	    return;
	// Nope, swap and buble up!
	myQ.swapEntries(idx, entries()-1);
	bubbleUp(idx);
    }
    else
    {
	// We are a hole, so we always have to bubble!
	// Choose the lesser of child1 and child2.
	if (child2 >= entries() ||
	    myQ(child1).priority < myQ(child2).priority)
	{
	    // Child 1!
	    myQ.swapEntries(idx, child1);
	    bubbleDown(child1);
	}
	else
	{
	    // Child 2:
	    myQ.swapEntries(idx, child2);
	    bubbleDown(child2);
	}
    }
}
