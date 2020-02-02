/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        grammar.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	These handle all the bizarre exceptions which English can
 *	throw at us.  Well, theoritically they are all handled, but
 *	as exceptions are found, this is where to add them.
 */

#ifndef __grammar_h__
#define __grammar_h__

#include "buf.h"

class MOB;
class ITEM;

enum VERB_PERSON
{
    VERB_I,
    VERB_YOU,
    VERB_HE,
    VERB_SHE,
    VERB_IT,
    VERB_WE,
    VERB_YALL,
    VERB_HES,
    VERB_SHES,
    VERB_THEY,
    NUM_VERBS
};

// Utility functions to expand ctype.
bool
gram_ispronoun(const char *word);
inline bool
gram_ispronoun(BUF buf)
{ return gram_ispronoun(buf.buffer()); }
    

// Utility function to capitalize a sentence...
// This will also capitalize any sub sentences.
BUF
gram_capitalize(const char *str);
BUF
gram_capitalize(BUF buf);

// This will convert the given name into the possessive tense.
// Ie, you -> your, orc -> orc's, moss -> moss'
BUF
gram_makepossessive(const char *str);
BUF
gram_makepossessive(BUF str);

// This will convert the given name into a plural
BUF
gram_makeplural(const char *phrase);
BUF
gram_makeplural(BUF phrase);

// XXX hits the Foo.
// he/she/it/you/I
const char *
gram_getpronoun(VERB_PERSON person);

// XXX rock dissolved in acid.
// his/her/its/your/my
const char *
gram_getpossessive(VERB_PERSON person);

// That rock is XXX.
// his/hers/its/yours/mine
const char *
gram_getownership(VERB_PERSON person);

// Suicidially, Foo hits XXX.
// himself/herself/itself/yourself/myself
const char *
gram_getreflexive(VERB_PERSON person);

// Foo hits XXX.
// him/her/it/you/me
const char *
gram_getaccusative(VERB_PERSON person);

bool
gram_isvowel(char c);

// True if c is the end of a sentence.
bool
gram_isendsentence(char c);

bool
gram_isplural(const char *noun);
inline bool
gram_isplural(BUF buf)
{ return gram_isplural(buf.buffer()); }

// This takes a complicated name, like:
// holy +3 wand of fireballs (5)
// and determines if "wand" is plural.
bool
gram_isnameplural(const char *name);
inline bool
gram_isnameplural(BUF buf)
{ return gram_isnameplural(buf.buffer()); }

// This will fetch the appropriate article, ie: a, an, the.
// As the article may be empty (for proper nouns or plural nouns)
// the trailing space is included, so it would be "a ".
const char *
gram_getarticle(const char *noun);
inline const char *
gram_getarticle(BUF buf)
{ return gram_getarticle(buf.buffer()); } 

// This builds the appropriate phrase, such as "5 arrows of dragon slaying"
// or "no tea" according to the singular basename and the count variable.
// If article is false, it will not use "a" or "the" in the singular
// cases.
BUF
gram_createcount(const char *basename, int count, bool article);
BUF
gram_createcount(BUF basename, int count, bool article);

// This builds the appropriate place number.  Ie, 1st, 2nd, 23rd.
BUF
gram_createplace(int place);

// Conjugates the given infinitive verb according the given person.
BUF
gram_conjugate(const char *verb, VERB_PERSON person, bool past = false);
inline BUF
gram_conjugate(BUF verb, VERB_PERSON person, bool past = false)
{ return gram_conjugate(verb.buffer(), person, past); }

#endif
