/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        item.h ( Live Once Library, C++ )
 *
 * COMMENTS:
 */

#ifndef __item__
#define __item__

#include "glbdef.h"

#include "dpdf.h"
#include "grammar.h"
#include "map.h"
#include "buf.h"

#include <iostream>
using namespace std;

// Generates a unique id
int glb_allocUID();
// Reports that the given UID was loaded from disk, ensures we don't
// repeat it.
void glb_reportUID(int uid);

#define INVALID_UID -1

class ITEM
{
public:
		~ITEM();

    static ITEM *create(ITEM_NAMES item);

    // This can return 0 if no valid items exist at this depth!
    static ITEM *createRandom(ATLAS_NAMES atlas);
    static ITEM_NAMES itemFromHash(unsigned hash, ATLAS_NAMES atlas);

    static void	initSystem();
    static void saveGlobal(ostream &os);
    static void loadGlobal(istream &is);

    // Makes an identical copy
    ITEM	*copy() const;
    // Makes a new item with all the same properties, but new UID, etc.
    ITEM	*createCopy() const;

    ITEM_NAMES	 getDefinition() const { return myDefinition; }
    int		 getMagicClass() const;
    void	 markMagicClassKnown();
    bool	 isMagicClassKnown() const;
		
    VERB_PERSON	 getPerson() const;
    BUF		 getName() const;
    BUF		 getRawName() const;
    BUF		 getArticleName() const;

    // Returns "" if no detailed description.  Returns a multi-line
    // buffer prefixed with +-
    BUF		 getDetailedDescription() const;

    // Detailed Description + the text lookup.
    BUF		 getLongDescription() const;

    bool	 isBroken() const { return myBroken; }
    void	 setBroken(bool isbroken) { myBroken = isbroken; }

    bool	 isEquipped() const { return myEquipped; }
    void	 setEquipped(bool isequipped) { myEquipped = isequipped; }

    const ITEM_DEF	&defn() const { return defn(getDefinition()); }
    static const ITEM_DEF &defn(ITEM_NAMES item) { return glb_itemdefs[item]; }

    void	 getLook(u8 &symbol, ATTR_NAMES &attr) const;

    const POS   &pos() const { return myPos; }

    // Warning: This can delete this
    void	 move(POS pos);

    // Unlinks our position, dangerous so only use if you are sure you
    // will be removing from the map!
    void	 clearAllPos();

    void	 setMap(MAP *map) { myPos.setMap(map); }

    bool	 canStackWith(const ITEM *stack) const;
    void	 combineItem(const ITEM *item);
    int		 getStackCount() const { return myCount; }
    void	 decStackCount() { myCount--; }
    void	 setStackCount(int count) { myCount = count; }

    // -1 for things without a count down.
    int		 getTimer() const { return myTimer; }
    void	 addTimer(int add) { myTimer += add; }

    // Returns true if should self destruct.
    bool	 runHeartbeat();

    // Determines if it is at all considerable as a weapon
    bool	 isMelee() const;
    bool	 isArmour() const;
    bool	 isRanged() const;
    bool	 isPotion() const;
    bool	 isRing() const;
    bool	 isFood() const;

    DPDF	 getMeleeDPDF() const;

    void	 getWeaponStats(int &power, int &accuracy, int &consistency) const;

    DPDF	 getRangeDPDF() const;
    int		 getRangeRange() const;
    int		 getRangeArea() const;

    int		 getAC() const;

    void	 getRangeStats(int &range, int &power, int &consistency,
				int &area) const;

    void	 save(ostream &os) const;
    static ITEM	*load(istream &is);

    int		 getUID() const { return myUID; }

    int		 getInterestedUID() const { return myInterestedMobUID; }
    void	 setInterestedUID(int uid) { myInterestedMobUID = uid; }

    void	 setMobType(MOB_NAMES mob) { myMobType = mob; }
    MOB_NAMES	 mobType() const { return myMobType; }

protected:
		 ITEM();

    ITEM_NAMES	 myDefinition;

    MOB_NAMES	 myMobType;

    POS		 myPos;
    int		 myCount;
    int		 myTimer;
    int		 myUID;
    int		 myInterestedMobUID;
    bool	 myBroken;
    bool	 myEquipped;
};

#endif

