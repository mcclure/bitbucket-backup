/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        rand.h ( Live Once Library, C++ )
 *
 * COMMENTS:
 *	This file is pre-7DRL.
 */

#ifndef __random__
#define __random__

typedef unsigned char u8;

// For our ISSPACE macro..
#include <ctype.h>

#include "dpdf.h"

class DICE
{
public:
    int		myNumDie, mySides;
    int		myBonus;

    // Rolls dice with the random function.
    int		roll() const;

    // Builds a DPDF representing this die
    DPDF	buildDPDF() const;
};

// Random in inclusive range.
int rand_range(int min, int max);

// Random from 0..num-1
int rand_choice(int num);

// Giving a null terminated list of strings, pick one of the strings
// at random
const char *rand_string(const char **stringlist);

// Rolls the specific number sided die.  The reroll count
// is how many rerolls you get.  Largest is returned.
// Result is between 1..num, unless num is 0, in which case it is 0,
// or negative, in which case -1..-num.
// Negative reroll means you get rerolls but the LEAST is returned.
int rand_roll(int sides, int reroll = 0);

// Roll numdieDsides + bonus
int rand_dice(int numdie, int sides, int bonus);

// Returns the current seed.
long rand_getseed();

// Sets the seed...
void rand_setseed(long seed);

// Does a wang int hash.
unsigned int rand_wanginthash(unsigned int key);

// Returns true percentage % of the time.
bool rand_chance(int percentage);

// Returns -1 or 1.
int  rand_sign();

// Initializes dx & dy, only one of which is non-zero.
void	rand_direction(int &dx, int &dy);

// Given a direction from 0..3, returns the dx,dy pair for that direction.
void	rand_getdirection(int dir, int &dx, int &dy);

// Given an angle 0..7, returns dx, dy pair.
void	rand_angletodir(int angle, int &dx, int &dy);
// Given a dx & dy, returns the angle.
int	rand_dirtoangle(int dx, int dy);

// Return North, North-East, etc.
const char *rand_dirtoname(int dx, int dy);
const char *rand_angletoname(int angle);

// Shuffles the given set of numbers...
void	rand_shuffle(u8 *set, int n);
void	rand_shuffle(int *set, int n);

// Returns a double in range [0..1)
double	rand_double();

// Some standard methods...
#ifndef ABS
#define ABS(a) ((a) < 0 ? -(a) : (a))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef BOUND
#define BOUND(val, low, high) MAX(MIN(val, high), low)
#endif
#ifndef SIGN
#define SIGN(a) ((a) < 0 ? -1 : (a) > 0 ? 1 : 0)
#endif

// This is because MSVC is a broken compiler.
// Would it kill them to not crash on a negative input?
#define ISSPACE(c) ((c) < 0 ? 0 : isspace(c))

// Useful to iterate over all directions.
#define FORALL_4DIR(dx, dy) \
    for (int lcl_angle = 0; rand_getdirection(lcl_angle, dx, dy), lcl_angle < 4; lcl_angle++)

#define FORALL_8DIR(dx, dy) \
    for (int lcl_angle = 0; rand_angletodir(lcl_angle, dx, dy), lcl_angle < 8; lcl_angle++)

#define FORALL_XY(x, y) \
    for (y = 0; y < height(); y++) \
	for (x = 0; x < width(); x++)

#endif
