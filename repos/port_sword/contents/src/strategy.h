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

#ifndef __strategy__
#define __strategy__

#include "map.h"
#include "mapdata.h"

class CIVSTATE;
class TARGET;

class STRATEGY
{
public:
    explicit STRATEGY(ATLAS_NAMES atlas);
    STRATEGY(const STRATEGY &map);
    STRATEGY &operator=(const STRATEGY &map);

    void	decRef();
    void	incRef();

    static STRATEGY	*load(istream &is);
    void		 save(ostream &os) const;

    POS			 getCenter() const;

    void		 grantQuestBonus(int civ, QUEST_NAMES quest);
    bool		 integrate();
    int			 width() const;
    int			 height() const;

    CIVSTATE		*getToday() const { return myHistory.entries() ? myHistory.top() : 0; }
    CIVSTATE		*getYear(int year) const { return (year <= getMaxYear()) ? myHistory(year) : 0; }
    int			 getMaxYear() const { return myHistory.entries() - 1; }

    TARGET		*getTarget(int target) const { return myTargets(target); }

    int			 cityCount(int civ, CIVSTATE *state) const;

protected:
    void		 addSources(CIVSTATE *state);
    void		 diffuse(CIVSTATE *state);
    void		 battle(CIVSTATE *state);
    void		 rebuildTargets();

private:
    STRATEGY();
    ~STRATEGY();

    MAP				*myMap;
    PTRLIST<CIVSTATE *>		myHistory;
    PTRLIST<TARGET *>		myTargets;
    ATOMIC_INT32		myRefCnt;
};


#endif
