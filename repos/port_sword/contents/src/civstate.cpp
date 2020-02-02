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

#include "civstate.h"

CIVSTATE::CIVSTATE()
{
    myRefCnt.set(0);
    for (int i = 0; i < NUMCIV; i++)
    {
	myDemo[i].pop = (1) * 1000000.0f;
	if (i)
	    myDemo[i].pop *= 1.2;
	myDemo[i].popgrowth = 100.0f;
	myDemo[i].died = 0.0f;
	myDemo[i].enlisted = 0;

	myDemo[i].attack = 1.0f;
	if (i)
	    myDemo[i].attack *= 1.5;
    }
}

CIVSTATE::CIVSTATE(const CIVSTATE &state)
{
    myRefCnt.set(0);
    for (int i = 0; i < NUMCIV; i++)
    {
	myDemo[i] = state.myDemo[i];

	myArmy[i] = state.myArmy[i];
    }
}

CIVSTATE::~CIVSTATE()
{
}

void
CIVSTATE::incRef()
{
    myRefCnt.add(1);
}

void
CIVSTATE::decRef()
{
    int		nval;
    nval = myRefCnt.add(-1);

    if (nval <= 0)
	delete this;
}

void
CIVSTATE::setArmy(int civ, MAPDATA data)
{
    myArmy[civ] = data;
}

void
CIVSTATE::save(ostream &os) const
{
    for (int i = 0; i < NUMCIV; i++)
    {
	os.write((const char *) &myDemo[i], sizeof(DEMOGRAPHIC));
	
	myArmy[i].save(os);
    }
}

CIVSTATE *
CIVSTATE::load(istream &is)
{
    CIVSTATE	*state = new CIVSTATE();
    for (int i = 0; i < NUMCIV; i++)
    {
	is.read((char *) &state->myDemo[i], sizeof(DEMOGRAPHIC));

	state->myArmy[i].load(is);
    }

    return state;
}
