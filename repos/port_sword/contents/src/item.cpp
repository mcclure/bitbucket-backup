/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        item.cpp ( Live Once Library, C++ )
 *
 * COMMENTS:
 */

#include "item.h"

#include <assert.h>

#include <iostream>
using namespace std;

#include "grammar.h"
#include "text.h"

int glbUniqueId = 0;

static bool glbItemIded[NUM_ITEMS];
static int  glbItemMagicClass[NUM_ITEMS];

int 
glb_allocUID()
{
    return glbUniqueId++;
}

void
glb_reportUID(int uid)
{
    if (uid >= glbUniqueId)
	glbUniqueId = uid+1;
}


//
// Item Definitions
//

ITEM::ITEM()
{
    myDefinition = (ITEM_NAMES) 0;
    myCount = 1;
    myTimer = -1;
    myUID = INVALID_UID;
    myInterestedMobUID = INVALID_UID;
    myBroken = false;
    myEquipped = false;
    myMobType = MOB_NONE;
}

ITEM::~ITEM()
{
    pos().removeItem(this);
}

void
ITEM::move(POS pos)
{
    myPos.removeItem(this);
    myPos = pos;
    myPos.addItem(this);
}

void
ITEM::clearAllPos()
{
    myPos = POS();
}

ITEM *
ITEM::copy() const
{
    ITEM *item;

    item = new ITEM();

    *item = *this;

    return item;
}

ITEM *
ITEM::createCopy() const
{
    ITEM *item;

    item = copy();

    item->myUID = glb_allocUID();

    return item;
}

ITEM *
ITEM::create(ITEM_NAMES item)
{
    ITEM	*i;

    assert(item >= ITEM_NONE && item < NUM_ITEMS);

    i = new ITEM();

    i->myDefinition = item;
    i->myTimer = glb_itemdefs[item].timer;
    i->setStackCount(i->defn().startstack);

    i->myUID = glb_allocUID();

    if (i->defn().startsbroken)
	i->myBroken = true;

    return i;
}

ITEM *
ITEM::createRandom(ATLAS_NAMES atlas)
{
    ITEM_NAMES		item;
    ITEM		*i;

    item = itemFromHash(rand_choice(65536*32767), atlas);
    if (item == ITEM_NONE)
	return 0;
    i = create(item);

    if (i)
    {
	if (atlas == ATLAS_THRONE)
	{
	    i->myBroken = false;
	}
    }
    return i;
}

ITEM_NAMES
ITEM::itemFromHash(unsigned int hash, ATLAS_NAMES atlas)
{
    int		totalrarity, rarity;
    ITEM_NAMES	item = ITEM_NONE;

    // Find total rarity of items
    totalrarity = 0;
    for (const char *itemlistid = glb_atlasdefs[atlas].itemlists;
	*itemlistid;
	itemlistid++)
    {
	for (const char *itemid = glb_itemlistdefs[*itemlistid].items;
	     *itemid;
	     itemid++)
	{
	    item = (ITEM_NAMES) *itemid;

	    rarity = defn(item).rarity;
	    totalrarity += rarity;
	}
    }

    if (!totalrarity)
	return ITEM_NONE;

    hash %= totalrarity;
    for (const char *itemlistid = glb_atlasdefs[atlas].itemlists;
	*itemlistid;
	itemlistid++)
    {
	for (const char *itemid = glb_itemlistdefs[*itemlistid].items;
	     *itemid;
	     itemid++)
	{
	    item = (ITEM_NAMES) *itemid;
	    rarity = defn(item).rarity;

	    if (hash > (unsigned) rarity)
		hash -= rarity;
	    else
		return item;
	}
    }
    return item;
}

int
ITEM::getMagicClass() const
{
    return glbItemMagicClass[getDefinition()];
}

bool
ITEM::isMagicClassKnown() const
{
    return glbItemIded[getDefinition()];
}

void
ITEM::markMagicClassKnown()
{
    glbItemIded[getDefinition()] = true;
}

VERB_PERSON
ITEM::getPerson() const
{
    return VERB_IT;
}

BUF
ITEM::getName() const
{
    BUF		buf, basename;

    basename = getRawName();

    if (mobType() != MOB_NONE)
    {
	const char *mobname = glb_mobdefs[mobType()].name;
	buf.sprintf("%s %s", mobname, basename.buffer());
	basename = buf;
    }

    if (!buf.isstring())
    {
	if (getTimer() >= 0)
	{
	    // A timed item...
	    buf.sprintf("%s (%d)", basename.buffer(), getTimer());
	}
	else
	{
	    // A normal item.
	    buf.reference(basename.buffer());
	}
    }
    if (isBroken())
    {
	BUF		broken;
	broken.sprintf("broken %s", buf.buffer());
	buf.strcpy(broken);
    }

    return gram_createcount(buf, myCount, false);
}

BUF
ITEM::getArticleName() const
{
    BUF		name = getName();
    BUF		result;

    result.sprintf("%s%s", gram_getarticle(name.buffer()), name.buffer());

    return result;
}

BUF
ITEM::getRawName() const
{
    BUF		buf;

    buf.reference(defn().name);

    if (defn().ispotion)
    {
	if (isMagicClassKnown())
	{
	    buf.reference(glb_potiondefs[getMagicClass()].name);
	}
    }
    if (defn().isring)
    {
	if (isMagicClassKnown())
	{
	    buf.reference(glb_ringdefs[getMagicClass()].name);
	}
    }

    return buf;
}

BUF
ITEM::getDetailedDescription() const
{
    BUF		result, buf;

    result.reference("");

    if (isMelee())
    {
	// Build melee description.
	int		p, a, c;
	int		min, q1, q2, q3, max;
	DPDF	damage;

	damage = getMeleeDPDF();
	// Only count hits.
	damage.applyGivenGreaterThan(0);
	damage.getQuartile(min, q1, q2, q3, max);

	getWeaponStats(p, a, c);
	if (min == max && a == 100)
	    buf.sprintf("Damage: %d\n",
			    max);
	else
	    buf.sprintf("Accuracy: %d\nDamage: \n%d..%d..%d..%d..%d\n",
			    a, min, q1, q2, q3, max);
	result.strcat(buf);
    }
    if (isRanged())
    {
	// Build ranged description.
	int		p, c, r, a;
	int		min, q1, q2, q3, max;
	DPDF	damage;

	damage = getRangeDPDF();
	// Spells always hit
	damage.getQuartile(min, q1, q2, q3, max);

	getRangeStats(r, p, c, a);
	if (min == max)
	    buf.sprintf(
		    "Range:  %d\n"
		    "Damage: %d\n",
			r,
			max);
	else
	    buf.sprintf(
		    "Range: %d\n"
		    "Damage:\n%d..%d..%d..%d..%d\n",
			r,
			min, q1, q2, q3, max);
	result.strcat(buf);
    }
    if (isArmour())
    {
	buf.sprintf(
		    "Armour: %d\n",
		    getAC());
	result.strcat(buf);
    }

    if (isRing() && isMagicClassKnown())
    {
	RING_NAMES	ring = (RING_NAMES) getMagicClass();

	if (glb_ringdefs[ring].resist != ELEMENT_NONE)
	{
	    BUF		capelement;
	    capelement = gram_capitalize(glb_elementdefs[glb_ringdefs[ring].resist].name);
	    int		ramt = glb_ringdefs[ring].resist_amt;
	    if (ramt < 0)
		buf.sprintf("%s Vulnerable: %d\n", capelement.buffer(), -ramt);
	    else
		buf.sprintf("%s Resist: %d\n", capelement.buffer(), ramt);
	    result.strcat(buf);
	}
	if (glb_ringdefs[ring].deflect != 0)
	{
	    int		amt = glb_ringdefs[ring].deflect;
	    buf.sprintf("Deflection: %d\n", amt);
	    result.strcat(buf);
	}
    }
    return result;
}

BUF
ITEM::getLongDescription() const
{
    BUF		descr, detail;

    descr = gram_capitalize(getName());
    detail = getDetailedDescription();
    descr.append('\n');
    if (detail.isstring())
    {
	descr.append('\n');
	descr.strcat(detail);
    }

    // Check for friendly bodies
    if (getDefinition() == ITEM_CORPSE &&
	glb_mobdefs[mobType()].isfriendly)
	detail = text_lookup("item", "body");
    else
	detail = text_lookup("item", getRawName());
    if (detail.isstring() && !detail.startsWith("Missing text entry: "))
    {
	descr.append('\n');
	descr.strcat(detail);
    }
    else
    {
	descr.append('\n');
    }

    // Append the non-magic version
    if (isMagicClassKnown() && (isPotion() || isRing()))
    {
	descr.strcat("Base Type: ");
	detail = gram_capitalize(defn().name);
	detail.append('\n');
	descr.strcat(detail);

	detail = text_lookup("item", defn().name);
	if (detail.isstring() && !detail.startsWith("Missing text entry: "))
	{
	    descr.append('\n');
	    descr.strcat(detail);
	}
	else
	{
	    descr.append('\n');
	}
    }

    return descr;
}

void
ITEM::getLook(u8 &symbol, ATTR_NAMES &attr) const
{
    symbol = defn().symbol;
    attr = (ATTR_NAMES) defn().attr;
}

bool
ITEM::canStackWith(const ITEM *stack) const
{
    if (getDefinition() != stack->getDefinition())
	return false;
    
    if (mobType() != stack->mobType())
	return false;

    // Disable stacking of weapons/books/armour as we want
    // to be able to delete them easily
    if (defn().unstackable)
	return false;

    // No reason why not...
    return true;
}

void
ITEM::combineItem(const ITEM *item)
{
    // Untimed items stack.
    if (getTimer() < 0)
	myCount += item->getStackCount();
    // Timed items add up charges.
    if (getTimer() >= 0)
    {
	assert(item->getTimer() >= 0);
	myTimer += item->getTimer();
    }

    assert(myCount >= 0);
    if (myCount < 0)
	myCount = 0;
}

bool
ITEM::runHeartbeat()
{
    if (myTimer >= 0)
    {
	if (!myTimer)
	    return true;
	myTimer--;
    }
    return false;
}

void
ITEM::initSystem()
{
    ITEM_NAMES		item;

    FOREACH_ITEM(item)
    {
	glbItemIded[item] = false;
	glbItemMagicClass[item] = 0;
    }

    // Mix the potions...
    int			potionclasses[NUM_POTIONS];
    POTION_NAMES	potion;

    int		pclass = 1;
    FOREACH_ITEM(item)
    {
	if (glb_itemdefs[item].ispotion)
	{
	    if (pclass >= NUM_POTIONS)
	    {
		assert(!"Too many mundane potions");
		break;
	    }
	    potionclasses[pclass] = item;
	    pclass++;
	}
    }
    rand_shuffle(&potionclasses[1], pclass-1);
    FOREACH_POTION(potion)
    {
	if (potion == POTION_NONE)
	    continue;
	if (potion >= pclass)
	{
	    //assert(!"Not enough mundane potions");
	    break;
	}
	glbItemMagicClass[potionclasses[potion]] = potion;
    }

    // Mix the rings...
    int		ringclasses[NUM_RINGS];
    RING_NAMES	ring;

    pclass = 1;
    FOREACH_ITEM(item)
    {
	if (glb_itemdefs[item].isring)
	{
	    if (pclass >= NUM_RINGS)
	    {
		assert(!"Too many mundane rings");
		break;
	    }
	    ringclasses[pclass] = item;
	    pclass++;
	}
    }
    rand_shuffle(&ringclasses[1], pclass-1);
    FOREACH_RING(ring)
    {
	if (ring == RING_NONE)
	    continue;
	if (ring >= pclass)
	{
//	    assert(!"Not enough mundane rings");
	    break;
	}
	glbItemMagicClass[ringclasses[ring]] = ring;
    }

}

void
ITEM::saveGlobal(ostream &os)
{
    ITEM_NAMES		item;
    int		val;

    FOREACH_ITEM(item)
    {
	val = glbItemIded[item];
	os.write((const char *) &val, sizeof(int));
	val = glbItemMagicClass[item];
	os.write((const char *) &val, sizeof(int));
    }
}

void
ITEM::save(ostream &os) const
{
    int		val;

    val = myDefinition;
    os.write((const char *) &val, sizeof(int));

    myPos.save(os);

    os.write((const char *) &myCount, sizeof(int));
    os.write((const char *) &myTimer, sizeof(int));
    os.write((const char *) &myUID, sizeof(int));
    os.write((const char *) &myInterestedMobUID, sizeof(int));
    os.write((const char *) &myBroken, sizeof(bool));
    os.write((const char *) &myEquipped, sizeof(bool));
    val = myMobType;
    os.write((const char *) &val, sizeof(int));
}

ITEM *
ITEM::load(istream &is)
{
    int		val;
    ITEM	*i;

    i = new ITEM();

    is.read((char *)&val, sizeof(int));
    i->myDefinition = (ITEM_NAMES) val;

    i->myPos.load(is);

    is.read((char *)&i->myCount, sizeof(int));
    is.read((char *)&i->myTimer, sizeof(int));

    is.read((char *)&i->myUID, sizeof(int));
    glb_reportUID(i->myUID);
    is.read((char *)&i->myInterestedMobUID, sizeof(int));
    is.read((char *)&i->myBroken, sizeof(bool));
    is.read((char *)&i->myEquipped, sizeof(bool));
    is.read((char *)&val, sizeof(int));
    i->myMobType = (MOB_NAMES) val;

    return i;
}

void
ITEM::loadGlobal(istream &is)
{
    ITEM_NAMES		item;
    int		val;

    FOREACH_ITEM(item)
    {
	is.read((char *) &val, sizeof(int));
	glbItemIded[item] = val ? true : false;
	is.read((char *) &val, sizeof(int));
	glbItemMagicClass[item] = val;
    }
}

bool
ITEM::isPotion() const
{
    return defn().ispotion;
}

bool
ITEM::isRing() const
{
    return defn().isring;
}

bool
ITEM::isFood() const
{
    return defn().isfood;
}

bool
ITEM::isMelee() const
{
    if (defn().melee_power || defn().melee_consistency)
	return true;
    return false;
}

bool
ITEM::isArmour() const
{
    if (getAC())
	return true;
    return false;
}

bool
ITEM::isRanged() const
{
    if (defn().range_power || defn().range_consistency)
	return true;
    return false;
}

void
ITEM::getWeaponStats(int &power, int &accuracy, int &consistency) const
{
    power = defn().melee_power;
    consistency = defn().melee_consistency;
    accuracy = defn().melee_accuracy;
}

DPDF
ITEM::getMeleeDPDF() const
{
    // Compute our power, accuracy, and consistency.
    int		power, accuracy, consistency;

    getWeaponStats(power, accuracy, consistency);

    // Power produces a pure DPDF
    DPDF		damage(0);

    damage += DPDF(0, power*2);

    // Add in the consistency bonus.  This is also comparable to power,
    // but is a constant multiplier rather than a DPDF
    damage += consistency;

    // Now, scale by the accuracy.
    // We make this a straight 0..100.
    double	tohit;

    tohit = (accuracy) / 100.0;
    if (tohit > 1.0)
    {
	DPDF		critical;

	critical = damage;
	critical *= (tohit - 1.0);
	damage += critical;
    }
    else
	damage *= tohit;

    return damage;
}

void
ITEM::getRangeStats(int &range, int &power, int &consistency, int &area) const
{
    range = defn().range_range;
    consistency = defn().range_consistency;
    power = defn().range_power;
    area = defn().range_area;
}

int
ITEM::getRangeRange() const
{
    // Compute our power, accuracy, and consistency.
    int		range, d;

    getRangeStats(range, d, d, d);

    return range;
}

int
ITEM::getAC() const
{
    return defn().ac;
}

int
ITEM::getRangeArea() const
{
    // Compute our power, accuracy, and consistency.
    int		area, d;

    getRangeStats(d, d, d, area);

    return area;
}
DPDF
ITEM::getRangeDPDF() const
{
    // Compute our power, accuracy, and consistency.
    int		power, consistency, i;

    getRangeStats(i, power, consistency, i);

    // Power produces a pure DPDF
    DPDF		damage(0);

    damage += DPDF(0, power*2);

    // Add in the consistency bonus.  This is also comparable to power,
    // but is constant
    damage += consistency;

    return damage;
}
