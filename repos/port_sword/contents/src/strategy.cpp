/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Sword In Hand Development
 *
 * NAME:        strategy.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 * 	Stragetic map of overview.
 */

#include "strategy.h"
#include "civstate.h"
#include "target.h"
#include "pq.h"
#include "vec2.h"
#include "scrpos.h"

#define STRAT_W	45
#define STRAT_H	45

STRATEGY::STRATEGY()
{
    myMap = 0;
    myRefCnt.set(0);
}

STRATEGY::STRATEGY(ATLAS_NAMES atlas)
{
    myRefCnt.set(0);
    myMap = new MAP(atlas, 0, 0);
    myMap->incRef();

    rebuildTargets();
    CIVSTATE		*newstate;

    newstate = new CIVSTATE();
    for (int i= 0; i < NUMCIV; i++)
    {
	newstate->setArmy(i, MAPDATA(width(), height()));
    }
    newstate->incRef();
    myHistory.append(newstate);
}

STRATEGY::STRATEGY(const STRATEGY &strategy)
{
    myRefCnt.set(0);
    myMap = 0;
    *this = strategy;
}

STRATEGY &
STRATEGY::operator=(const STRATEGY &strategy)
{
    // Can just ref-count the map for now.
    if (strategy.myMap)
	strategy.myMap->incRef();
    if (myMap)
	myMap->decRef();
    myMap = strategy.myMap;

    for (int i = 0; i < strategy.myHistory.entries(); i++)
	strategy.myHistory(i)->incRef();
    for (int i = 0; i < myHistory.entries(); i++)
	myHistory(i)->decRef();
    myHistory.clear();
    for (int i = 0; i < strategy.myHistory.entries(); i++)
	myHistory.append(strategy.myHistory(i));

    for (int i = 0; i < myTargets.entries(); i++)
	delete myTargets(i);
    myTargets.clear();
    for (int i = 0; i < strategy.myTargets.entries(); i++)
	myTargets.append(new TARGET(*strategy.myTargets(i)));

    return *this;
}

STRATEGY::~STRATEGY()
{
    if (myMap)
	myMap->decRef();
    for (int i = 0; i < myHistory.entries(); i++)
	myHistory(i)->decRef();
}

void
STRATEGY::incRef()
{
    myRefCnt.add(1);
}

void
STRATEGY::decRef()
{
    int		nval;
    nval = myRefCnt.add(-1);

    if (nval <= 0)
	delete this;
}

int
STRATEGY::width() const
{
    return STRAT_W;
}

int
STRATEGY::height() const
{
    return STRAT_H;
}

POS
STRATEGY::getCenter() const
{
    if (myMap)
    {
	return myMap->findRoomOfType((ROOMTYPE_NAMES) glb_atlasdefs[myMap->getAtlas()].start_roomtype);
    }
    return POS();
}

int			 
STRATEGY::cityCount(int civ, CIVSTATE *state) const
{
    int		count = 0;
    for (int i = 0; i < myTargets.entries(); i++)
    {
	TARGET	*t = myTargets(i);
	if (!t->isCity())
	    continue;
	// See who owns it!
	int		owner = t->owner(state);

	if (owner == civ)
	    count++;
    }
    return count;
}

void
STRATEGY::addSources(CIVSTATE *state)
{
    int		citycount[NUMCIV] = { 0, 0 };
    state->army(0).uniquify();
    state->army(1).uniquify();

    citycount[0] = cityCount(0, state);
    citycount[1] = cityCount(1, state);

    float	narmy[NUMCIV];
    for (int i = 0; i < NUMCIV; i++)
    {
	// Grow our population...
	float		newborn = MIN(state->myDemo[i].popgrowth,
				      state->myDemo[i].pop * 0.01);

	newborn *= (citycount[i] + 5.0) / 5;
	state->myDemo[i].pop += newborn;

	// Compute enlisted personal.
	state->myDemo[i].enlisted = state->army(i).total();

	narmy[i] = (state->myDemo[i].pop - state->myDemo[i].enlisted) * 0.01;
	narmy[i] = MAX(narmy[i], 0.0);
	narmy[i] *= (citycount[i] + 5.0) / 5;
	// Move them into the enlisted column.
	if (citycount[i])
	    state->myDemo[i].enlisted += narmy[i];
    }

    // Spread out our new recruits.
    for (int i = 0; i < myTargets.entries(); i++)
    {
	TARGET	*t = myTargets(i);
	if (!t->isCity())
	    continue;
	// See who owns it!
	int		owner = t->owner(state);

	state->army(owner).data(t->x(), t->y()) += narmy[owner] / citycount[owner];
    }
}

void
STRATEGY::diffuse(CIVSTATE *state)
{
    MAPDATA		targetgradient[2];
    
    for (int civ = 0; civ < NUMCIV; civ++)
    {
	targetgradient[civ] = MAPDATA(state->army(civ).width(), state->army(civ).height());

	targetgradient[civ].constant(10000);
	int		targetcity = 0;

	for (int target = 0; target < myTargets.entries(); target++)
	{
	    if (myTargets(target)->owner(state) != civ)
	    {
		if (myTargets(target)->isCity())
		    targetcity++;
		// Valid target!
		targetgradient[civ].accumulateGradient(myTargets(target)->getDistMap(), myTargets(target)->getValue());
	    }
	}
    }

    for (int civ = 0; civ < NUMCIV; civ++)
    {
	MAPDATA		newarmy(state->army(civ).width(), state->army(civ).height());
	for (int y = 0; y < newarmy.height(); y++)
	{
	    for (int x = 0; x < newarmy.width(); x++)
	    {
		int		dx, dy;
		float		oldcost = targetgradient[civ](x, y), newcost;
		float		totalbenefit = 0;

		if (oldcost < 0)
		    continue;
		FORALL_4DIR(dx, dy)
		{
		    if (x+dx < 0 || x+dx >= newarmy.width())
			continue;
		    if (y+dy < 0 || y+dy >= newarmy.height())
			continue;

		    newcost = targetgradient[civ](x+dx, y+dy);
		    if (newcost >= 0 && newcost < oldcost)
		    {
			totalbenefit += oldcost - newcost;
		    }
		}

		if (totalbenefit)
		{
		    FORALL_4DIR(dx, dy)
		    {
			if (x+dx < 0 || x+dx >= newarmy.width())
			    continue;
			if (y+dy < 0 || y+dy >= newarmy.height())
			    continue;

			newcost = targetgradient[civ](x+dx, y+dy);
			if (newcost >= 0 && newcost < oldcost)
			{
			    newarmy.data(x+dx, y+dy) += state->army(civ)(x, y) * 0.98 * (oldcost - newcost) / totalbenefit;
			}
		    }
		    newarmy.data(x, y) += state->army(civ)(x, y) * 0.02;
		}
		else
		{
		    newarmy.data(x, y) += state->army(civ)(x, y);
		}
	    }
	}
	state->setArmy(civ, newarmy);
    }
}

void
STRATEGY::battle(CIVSTATE *state)
{
    for (int y = 0; y < height(); y++)
    {
	for (int x = 0; x < width(); x++)
	{
	    float	army[2];

	    army[0] = state->army(0)(x, y);
	    army[1] = state->army(1)(x, y);

	    if (army[0] && army[1])
	    {
		// Fierce battle!
		float time[2], killed;
		
		time[0] = army[1] / (army[0] * state->myDemo[0].attack);
		time[1] = army[0] / (army[1] * state->myDemo[1].attack);
		if (time[0] < time[1])
		{
		    killed = time[0] * (army[1] * state->myDemo[1].attack);
		    army[0] -= killed;
		    state->myDemo[0].kill(killed);
		    state->myDemo[1].kill(army[1]);
		    army[1] = 0;
		}
		else
		{
		    killed = time[1] * (army[0] * state->myDemo[0].attack);
		    army[1] -= killed;
		    state->myDemo[1].kill(killed);
		    state->myDemo[0].kill(army[0]);
		    army[0] = 0;
		}
		state->army(0).data(x, y) = MAX(0, army[0]);
		state->army(1).data(x, y) = MAX(0, army[1]);
	    }
	}
    }
}

void
STRATEGY::grantQuestBonus(int civ, QUEST_NAMES quest)
{
    CIVSTATE	*today = getToday();

    if (today)
    {
	today->myDemo[civ].popgrowth += glb_questdefs[quest].popgrowth * 10;
	today->myDemo[civ].attack += glb_questdefs[quest].attack / 100.0;
    }

}

bool
STRATEGY::integrate()
{
    CIVSTATE		*today = new CIVSTATE(*myHistory.top());

    today->incRef();

    // Compute original city counts
    int		origcity[NUMCIV], finalcity[NUMCIV];
    for (int civ = 0; civ < NUMCIV; civ++)
    {
	origcity[civ] = cityCount(civ, today);
    }

    // See if we already lost, in which case we don't integrate any more!
    if (myHistory.entries() > 1 && (!origcity[0] || !origcity[1]))
    {
	today->decRef();
	return false;
    }

    addSources(today);

    diffuse(today);
    battle(today);

    for (int civ = 0; civ < NUMCIV; civ++)
    {
	finalcity[civ] = cityCount(civ, today);
    }

    // I like the realism of this but it really unbalances the state
    // equations.
    // But then it is a steady state.
#if 1
    // Figure out the transfer of population.
    if (origcity[0] != finalcity[0])
    {
	// This is hardcoded for two players.
	float		origpop[NUMCIV];
	for (int civ = 0; civ < NUMCIV; civ++)
	    origpop[civ] = today->myDemo[civ].pop - today->myDemo[civ].enlisted;
	// Figure out who lost...
	int		loser = 1, winner = 0;
	if (origcity[0] > finalcity[0])
	{
	    loser = 0;
	    winner = 1;
	}
	float		poplost;

	poplost = (origcity[loser]-finalcity[loser]) / float(origcity[loser]);
	poplost *= today->myDemo[loser].pop - today->myDemo[loser].enlisted;
	today->myDemo[loser].pop -= poplost;
	today->myDemo[winner].pop += poplost;
    }
#endif
    myHistory.append(today);
    return true;
}

MAPDATA
computeDistMap(MAPDATA speed, int tx, int ty)
{
    MAPDATA	dist(speed.width(), speed.height());
    dist.constant(-1.0f);

    // Adjacent here!
    dist.data(tx, ty) = 0.0f;
    PQ<VEC2>		pq;
    pq.append(0, VEC2(tx, ty));

    while (pq.entries())
    {
	VEC2	spot = pq.pop();
	tx = spot.x();
	ty = spot.y();

	float	spotdist = dist.data(tx, ty);

	int	dx, dy;
	FORALL_4DIR(dx, dy)
	{
	    if (tx+dx < 0 || tx+dx >= dist.width())
		continue;
	    if (ty+dy < 0 || ty+dy >= dist.height())
		continue;

	    float	nspeed = speed(tx+dx, ty+dy);
	    if (nspeed <= 0)
		continue;
	    float	odist = dist(tx+dx, ty+dy);
	    // Because we don't have edge distances, but node
	    // speeds, the first visitor to the node is always the fastest!
	    if (odist < 0)
	    {
		float	ndist = spotdist + 1./nspeed;
		dist.data(tx+dx, ty+dy) = ndist;
		pq.append(ndist, VEC2(tx+dx, ty+dy));
	    }
	}
    }

    return dist;
}

void
STRATEGY::rebuildTargets()
{
    // Wipe out any existing targets.
    for (int i = 0; i < myTargets.entries(); i++)
	delete myTargets(i);
    myTargets.clear();

    // Build a screen pos.
    POS		center = getCenter();
    SCRPOS	scrpos(center, width()/2, height()/2);
    int		offx = width()/2;
    int		offy = height()/2;

    MAPDATA	squarespeed(width(), height());
    POSLIST	targetpos;

    for (int y = 0; y < height(); y++)
    {
	for (int x = 0; x < width(); x++)
	{
	    POS		pos = scrpos.lookup(x-offx, y-offy);

	    if (!pos.valid())
		squarespeed.data(x, y) = 0.0f;
	    else
	    {
		squarespeed.data(x, y) = pos.defn().armyspeed / 100.0f;
	    }
	}
    }

    myMap->findAllTiles(TILE_CITY_RED, targetpos);
    myMap->findAllTiles(TILE_CITY_VIOLET, targetpos);
    myMap->findAllTiles(TILE_FIELD, targetpos);

    for (int i = 0; i < targetpos.entries(); i++)
    {
	POS	pos = targetpos(i);

	int	x, y;

	if (scrpos.find(pos, x, y))
	{
	    x += offx;
	    y += offy;
	    // Not visible, we don't want!
	    TARGET		*target;

	    float		 value = 1.0f;
	    if (pos.tile() == TILE_CITY_VIOLET)
		value = 1.0f;
	    if (pos.tile() == TILE_CITY_RED)
		value = 1.0f;

	    target = new TARGET(value, computeDistMap(squarespeed, x, y),
				pos.tile(), x, y);
	    myTargets.append(target);
	}
    }

}

void
STRATEGY::save(ostream &os) const
{
    myMap->save(os);

    int		nentry = myHistory.entries();
    os.write((const char *) &nentry, sizeof(int));

    for (int i = 0; i < nentry; i++)
    {
	myHistory(i)->save(os);
    }
}

STRATEGY *
STRATEGY::load(istream &is)
{
    STRATEGY	*strat = new STRATEGY();

    strat->myMap = new MAP(is);
    strat->myMap->incRef();

    int		nhistory;
    is.read((char *) &nhistory, sizeof(int));
    for (int i = 0; i < nhistory; i++)
    {
	strat->myHistory.append(CIVSTATE::load(is));
	strat->myHistory.top()->incRef();
    }

    strat->rebuildTargets();

    return strat;
}
