/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        civstate.h ( Sword in Hand Library, C++ )
 *
 * COMMENTS:
 *	State of the Civilization(s)
 */

#ifndef __civstate__
#define __civstate__

#include <iostream>
using namespace std;

#include "thread.h"
#include "mapdata.h"

#define NUMCIV	2

class CIVSTATE
{
public:
    CIVSTATE();
    CIVSTATE(const CIVSTATE &civstate);

    static CIVSTATE	*load(istream &is);
    void		 save(ostream &os) const;

    void	setArmy(int civ, MAPDATA army);

    MAPDATA	&army(int army) { return myArmy[army]; }

    void	decRef();
    void	incRef();

private:
    ~CIVSTATE();
    
public:
    class		DEMOGRAPHIC
    {
    public:
	float		pop;
	float		enlisted;
	float		died;
	float		popgrowth;
	float		attack;
	void		kill(float todie)
	{
	    died += todie;
	    pop -= todie;
	    if (pop < 0)
		pop = 0;
	}
    };
    DEMOGRAPHIC			myDemo[NUMCIV];
    MAPDATA			myArmy[NUMCIV];

    ATOMIC_INT32		myRefCnt;
};

#endif
