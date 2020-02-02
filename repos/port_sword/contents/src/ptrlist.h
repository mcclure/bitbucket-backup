/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        ptrlist.h ( Letter Hunt Library, C++ )
 *
 * COMMENTS:
 *	Implements a simple list of pointers.
 */

#ifndef __ptrlist_h__
#define __ptrlist_h__

template <typename PTR>
class PTRLIST
{
public:
    PTRLIST();
    ~PTRLIST();

    // No copy constructors as you should be passing by reference to build!
    // That is a noble sentiment, but we live in a corrupt world
    PTRLIST(const PTRLIST<PTR> &ref);
    PTRLIST<PTR> &operator=(const PTRLIST<PTR> &ref);

    void		 append(PTR item);
    void		 append(const PTRLIST<PTR> &list);

    // Reverses the order of the stack.
    void		 reverse();

    // Empties the stack
    void		 clear();

    // Sets array to constant value
    void		 constant(PTR ptr);

    // Sets entries to the given size.
    void		 resize(int size);

    // Allocates copacity
    // Only valid on empty list!
    void		 setCapacity(int size);
    int			 capacity() const { return mySize; }

    PTR			 operator()(int idx) const;

    // I've been avoiding adding this for a long time for what
    // I believed was a very good reason.  But I've given up.
    PTR			 &operator()(int idx);

    int			 entries() const;

    void		 set(int idx, PTR item);

    void		 insert(int idx, PTR item);

    // Removes all instances of "item" from
    // this list.
    void		 removePtr(PTR item);

    // Removes a given index
    void		 removeAt(int idx);

    // Returns the last item on this list and decreases
    // the length.
    PTR			 pop();

    // Swaps two entries
    void		 swapEntries(int i1, int i2);

    // Shuffles.
    void		 shuffle();

    // Returns the last element.
    // The index allows you to specify N elements from the end, where
    // 0 is the last element and 1 second last.
    PTR			 top(int idx = 0) const { return (*this)(entries()-1-idx); }

    // Returns the first item from this list and
    // removes it.
    PTR			 removeFirst();

    // Finds the given item, returns -1 if fails.
    int			 find(PTR item) const;

    // Deletes all zero entries.
    void		 collapse();

    // I should not have to explain why you should never call this.
    PTR			*rawptr(int idx=0) const { return &myList[idx]; }

    void		 stableSort();

private:
    PTR			 *myList;
    int			  myStartPos;
    int			  myEntries;
    int			  mySize;
};

// For crappy platforms:
#include "ptrlist.cpp"

#endif

