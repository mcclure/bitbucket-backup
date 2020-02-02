/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        random.cpp ( Live Once Library, C++ )
 *
 * COMMENTS:
 *	This file is pre-7DRL
 */

#include "rand.h"

#include "mt19937ar.c"

int
genrandom()
{
    // We don't want a sign bit.
    return genrand_int31();
}

long
rand_getseed()
{
    long		seed;
    seed = genrandom();
    rand_setseed(seed);
    return seed;
}

void
rand_setseed(long seed)
{
    init_genrand((unsigned long) seed);
}

unsigned int
rand_wanginthash(unsigned int key)
{
    key += ~(key << 16);
    key ^=  (key >> 5);
    key +=  (key << 3);
    key ^=  (key >> 13);
    key += ~(key << 9);
    key ^=  (key >> 17);
    return key;
}


int
rand_range(int min, int max)
{
    int		v;

    if (min > max)
	return rand_range(max, min);
    
    v = rand_choice(max - min + 1);
    return v + min;
}

int
rand_choice(int num)
{
    int		v;

    // Choice of 0 or 1 is always 0.
    if (num < 2) return 0;

    v = genrandom();
    v %= num;

    return v;
}

const char *
rand_string(const char **stringlist)
{
    int		n;
    
    // Find length of the string list.
    for (n = 0; stringlist[n]; n++);

    return stringlist[rand_choice(n)];
}

int
rand_roll(int num, int reroll)
{
    int		max = 0, val;

    // -1, 0, and 1 all evaluate to themselves always.
    if (num >= -1 && num <= 1)
	return num;

    if (num < 0)
    {
	// Negative numbers just invoke this and reverse the results.
	// Note we can't just negate the result, as we want higher rerolls
	// to move it closer to -1, not closer to num!
	val = rand_roll(-num, reroll);
	val -= num + 1;
	return val;
    }

    if (reroll < 0)
    {
	// Negative rerolls means we want to reroll but pick the
	// smallest result.  This is the same as inverting our normal
	// roll distribution, so thus...
	val = rand_roll(num, -reroll);
	val = num + 1 - val;
	return val;
    }
    
    // I wasn't even drunk when I made reroll of 0 mean roll
    // once, and thus necissating this change.
    reroll++;
    while (reroll--)
    {
	val = rand_choice(num) + 1;
	if (val > max)
	    max = val;
    }

    return max;
}

int
rand_dice(int numdie, int sides, int bonus)
{
    int		i, total = bonus;

    for (i = 0; i < numdie; i++)
    {
	total += rand_choice(sides) + 1;
    }
    return total;
}

bool
rand_chance(int percentage)
{
    int		percent;

    percent = rand_choice(100);
    // We want strict less than so percentage 0 will never pass,
    // and percentage 99 will pass only one in 100.
    return percent < percentage;
}

int
rand_sign()
{
    return rand_choice(2) * 2 - 1;
}

void
rand_direction(int &dx, int &dy)
{
    if (rand_choice(2))
    {
	dx = rand_sign();
	dy = 0;
    }
    else
    {
	dx = 0;
	dy = rand_sign();
    }
}

void
rand_shuffle(u8 *set, int n)
{
    int		i, j;
    u8		tmp;

    for (i = n-1; i > 0; i--)
    {
	// Want to swap with anything earlier, including self!
	j = rand_choice(i+1);
	
	tmp = set[i];
	set[i] = set[j];
	set[j] = tmp;
    }
}

void
rand_shuffle(int *set, int n)
{
    int		i, j;
    int		tmp;

    for (i = n-1; i > 0; i--)
    {
	// Want to swap with anything earlier, including self!
	j = rand_choice(i+1);
	
	tmp = set[i];
	set[i] = set[j];
	set[j] = tmp;
    }
}

void
rand_getdirection(int dir, int &dx, int &dy)
{
    dir &= 3;
    switch (dir)
    {
	case 0:
	    dx = 0;
	    dy = 1;
	    break;

	case 1:
	    dx = 1;
	    dy = 0;
	    break;

	case 2:
	    dx = 0;
	    dy = -1;
	    break;

	case 3:
	    dx = -1;
	    dy = 0;
	    break;
    }
}

void
rand_angletodir(int angle, int &dx, int &dy)
{
    angle &= 7;

    int		dxtable[8] = { 1, 1, 0,-1,-1,-1, 0, 1 };
    int		dytable[8] = { 0, 1, 1, 1, 0,-1,-1,-1 };

    dx = dxtable[angle];
    dy = dytable[angle];
}

int
rand_dirtoangle(int dx, int dy)
{
    int		x, y, a;

    for (a = 0; a < 8; a++)
    {
	rand_angletodir(a, x, y);
	if (x == dx && y == dy)
	    return a;
    }

    // This is 0,0, so we just return any angle!
    return rand_range(0, 7);
}

const char *
rand_dirtoname(int dx, int dy)
{
    return rand_angletoname(rand_dirtoangle(dx, dy));
}

const char *
rand_angletoname(int angle)
{
    const char *anglenames[] =
    {
	"East",
	"Southeast",
	"South",
	"Southwest",
	"West",
	"Northwest",
	"North",
	"Northeast"
    };
    angle &= 7;

    return anglenames[angle];
}

int
DICE::roll() const
{
    return rand_dice(myNumDie, mySides, myBonus);
}

DPDF
DICE::buildDPDF() const
{
    // Start with 100% chance of 0.
    DPDF		total(0);
    int			i;

    // Add in our bonus.
    total += myBonus;

    // Add in each die roll.
    for (i = 0; i < myNumDie; i++)
    {
	// Fuck yeah, this is the way to work with probability
	// functions.  No messy math here!  Just brute force and ignorance!
	DPDF		die(1, mySides);

	total += die;
    }
    
    return total;
}

double
rand_double()
{
    return genrand_res53();
}

