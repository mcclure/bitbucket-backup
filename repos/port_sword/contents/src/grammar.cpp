/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        grammar.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Implementation of grammar functions.
 *	These handle all the bizarre exceptions which English can
 *	throw at us.  Well, theoritically they are all handled, but
 *	as exceptions are found, this is where to add them.
 */

#include "grammar.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "grammar.h"

static void
gram_extractofclause(char *tmp, char *&prefix, char *&noun, char *&suffix)
{
    char		*ofclause;
    
    // Rules:
    // Last word is the noun.
    // (3) is not the noun.
    // The noun occurs before the first of clause.
    
    // Of clause is either " of ", or, if that doesn't exist, " to ",
    // or, if that is also missing, " named ".
    // Technically we should likely extract any of these that occurs first.

    ofclause = strstr(tmp, " of ");
    if (!ofclause)
	ofclause = strstr(tmp, " to ");
    if (!ofclause)
	ofclause = strstr(tmp, " named ");
    if (ofclause)
    {
	prefix = tmp;
	// Word preceeding of clause is it...
	assert(ofclause != tmp);
	if (ofclause == tmp)
	{
	    // Original string is " of lbjaslkd", this is bad, real bad.
	    prefix = 0;
	    suffix = 0;
	    noun = tmp;
	    return;
	}
	*ofclause = '\0';
	suffix = ofclause+1;
	noun = ofclause-1;
	
	while (!isspace(*noun) && noun > tmp)
	    noun--;
	if (isspace(*noun))
	{
	    *noun = '\0';
	    noun++;
	}
	else
	    prefix = 0;
    }
    else
    {
	prefix = tmp;
	
	// Work back from the end of the temp array for the first
	// alpha char.  We want the word that occurs after there.
	// To facillitate this, we mark all spaces as '\0'.
	// (that way box43 will be a noun)
	// Computer beeping at me.  Almost out of power.
	// Must type faster!
	noun = tmp + strlen(tmp) - 1;
	suffix = 0;
	while (noun > tmp && !isalpha(*noun))
	{
	    if (isspace(*noun))
		suffix = noun;
	    noun--;
	}
	// Keep back tracking this dude until we hit the start or space...
	while (noun > tmp && !isspace(*noun))
	    noun--;

	if (suffix)
	{
	    *suffix = '\0';
	    suffix++;
	}
	
	if (isspace(*noun))
	{
	    *noun = '\0';
	    noun++;
	}
	else
	    prefix = 0;
    }
}

bool
gram_ispronoun(const char *str)
{
    if (!strcmp(str, "I"))
	return true;
    if (!strcmp(str, "you"))
	return true;
    if (!strcmp(str, "he"))
	return true;
    if (!strcmp(str, "she"))
	return true;
    if (!strcmp(str, "it"))
	return true;
    if (!strcmp(str, "they"))
	return true;
    if (!strcmp(str, "we"))
	return true;

    return false;
}

bool
gram_isvowel(char c)
{
    switch (c)
    {
	case 'a':
	case 'e':
	case 'o':
	case 'i':
	case 'u':
	    return true;
    }
    // Note: y is not vowel.
    // Nor is w, Welsh being damned.
    return false;
}

// Does this char mark the end of a sentence?
bool
gram_isendsentence(char c)
{
    switch (c)
    {
	case '.':
	case '!':
	case '?':
	case '"':
	    return true;
    }
    return false;
}

BUF
gram_makeplural(const char *phrase)
{
    // Since we don't know ownership of phrase, must do this the
    // hardway
    BUF		buf;

    buf.strcpy(phrase);
    return gram_makeplural(buf);
}

BUF
gram_makeplural(BUF phrase)
{
    // Check for trivialities...
    if (gram_isnameplural(phrase))
	return phrase;

    // Extract the of clause...
    char		*tmp;
    char		*noun;
    char		*prefix, *suffix;

    tmp = strdup(phrase.buffer());

    gram_extractofclause(tmp, prefix, noun, suffix);

    // Now, try to end it...
    char		 ending[10];
    BUF			 buf;
    size_t		 nounlen = strlen(noun);
    
    // Build the ending...
    const char		*e;
    int			 epos = 0;
    for (e = noun; *e; e++);
    e--;
    for (epos = 0; epos < 10; epos++)
    {
	if (e < noun)
	    ending[epos] = '\0';
	else
	    ending[epos] = *e;
	e--;
    }

    if (!strncmp(ending, "esuo", 4) &&
	 strncmp(ending, "esuoh", 5))
    {
	// *ouse, but not house.  Thus,
	// louse->lice
	noun[nounlen-4] = '\0';
	buf.sprintf("%s%s%sice%s%s",
		     (prefix ? prefix : ""), (prefix ? " " : ""),
		     noun,
		     (suffix ? " " : ""), (suffix ? suffix : ""));
    }
    else if (!strncmp(ending, "dlihc", 5))
    {
	// child -> children
	buf.sprintf("%s%s%sren%s%s",
		     (prefix ? prefix : ""), (prefix ? " " : ""),
		     noun,
		     (suffix ? " " : ""), (suffix ? suffix : ""));
    }
    else if (!strncmp(ending, "nam", 3))
    {
	// man -> men
	noun[nounlen-3] = '\0';
	buf.sprintf("%s%s%smen%s%s",
		     (prefix ? prefix : ""), (prefix ? " " : ""),
		     noun,
		     (suffix ? " " : ""), (suffix ? suffix : ""));
		
    }
    else if (!strncmp(ending, "efi", 3))
    {
	// knife -> knives
	noun[nounlen-2] = '\0';
	buf.sprintf("%s%s%sves%s%s",
		     (prefix ? prefix : ""), (prefix ? " " : ""),
		     noun,
		     (suffix ? " " : ""), (suffix ? suffix : ""));
    }
    else if (!strncmp(ending, "fei", 3))
    {
	// thief -> thieves
	noun[nounlen-1] = '\0';
	buf.sprintf("%s%s%sves%s%s",
		     (prefix ? prefix : ""), (prefix ? " " : ""),
		     noun,
		     (suffix ? " " : ""), (suffix ? suffix : ""));
    }
    else if (!strcmp(noun, "ox"))
    {
	// ox -> oxen
	buf.sprintf("%s%s%sen%s%s",
		     (prefix ? prefix : ""), (prefix ? " " : ""),
		     noun,
		     (suffix ? " " : ""), (suffix ? suffix : ""));
    }
    else if (ending[0] == 's' ||
	!strncmp(ending, "hs", 2) ||		// bush -> bushes
	!strncmp(ending, "hc", 2) ||		// lich -> liches
	(*ending == 'o') ||
	(*ending == 'x') ||
	(*ending == 'z'))
    {
	// We pluralize by adding es.
	buf.sprintf("%s%s%ses%s%s", 
		    (prefix ? prefix : ""), (prefix ? " " : ""),
		    noun,
		    (suffix ? " " : ""), (suffix ? suffix : ""));
    }
    else if (*ending == 'y')
    {
	if (gram_isvowel(ending[1]))
	{
	    // tray -> trays
	    buf.sprintf("%s%s%ss%s%s", 
		    (prefix ? prefix : ""), (prefix ? " " : ""),
		    noun,
		    (suffix ? " " : ""), (suffix ? suffix : ""));
	}
	else
	{
	    // fly -> flies
	    noun[nounlen-1] = '\0';
	    buf.sprintf("%s%s%sies%s%s",
		    (prefix ? prefix : ""), (prefix ? " " : ""),
		    noun,
		    (suffix ? " " : ""), (suffix ? suffix : ""));
	}
    }
    else
    {
	// Just add s.
	buf.sprintf("%s%s%ss%s%s", 
		    (prefix ? prefix : ""), (prefix ? " " : ""),
		    noun,
		    (suffix ? " " : ""), (suffix ? suffix : ""));
    }

    return buf;
}

// We have to parse name-phrase to find the noun, and then
// determine if that noun is plural.
bool
gram_isnameplural(const char *name)
{
    char		*noun, *tmp, *prefix, *suffix;
    bool		 isplural = false;

    tmp = strdup(name);
    
    gram_extractofclause(tmp, prefix, noun, suffix);

    isplural = gram_isplural(noun);

    free(tmp);

    return isplural;
}

bool
gram_isplural(const char *noun)
{
    // As most noun decisions are based off the ending, we store the
    // string reversed ending here:
    char		 ending[10];

    // Build the ending...
    const char		*e;
    int			 epos = 0;
    for (e = noun; *e; e++);
    e--;
    for (epos = 0; epos < 10; epos++)
    {
	if (e < noun)
	    ending[epos] = '\0';
	else
	    ending[epos] = *e;
	e--;
    }

    // Check if last character is an s..
    if (ending[0] == 's')
    {
	// If the ending is 'ss', like "Grass", it should be "Grasses",
	// thus "ss" means not plural.
	if (ending[1] == 's')
	{
	    // Except some exception I'll find out later.
	    return false;
	}

	// Ended with an s?  Plural.
	return true;
    }
    else if (!strncmp(ending, "nem", 3))
    {
	// This is "foomen", like "lizardmen", so is
	// plural.
	return true;
    }
    else
    {
	if (!strcmp(noun, "oxen"))
	    return true;
	else if (!strcmp(noun, "children"))
	    return true;
	else if (!strcmp(noun, "feet"))
	    return true;
	else if (!strcmp(noun, "Shaman"))
	    return true;

	// This is, other than specific cases, singular.
	return false;
    }
}

const char *
gram_getarticle(const char *noun)
{
    if (gram_isnameplural(noun))
	return "";

    if (gram_ispronoun(noun))
	return "";

    // Uncountable objects.
    // These are controled by the basename, so "dry grass" matches grass.
    const char *basename;

    basename = noun;
    // Go to end
    while (*basename) ++basename;
    // Work a back until a space.
    while (basename > noun)
    {
	if (isspace(*basename))
	{
	    basename++;
	    break;
	}
	basename--;
    }
    
    if (!strcmp(basename, "water"))
	return "";
    if (!strcmp(basename, "ice"))
	return "";
    if (!strcmp(basename, "grass"))
	return "";
    if (!strcmp(basename, "water"))
	return "";
    if (!strcmp(basename, "mud"))
	return "";

    // Check for proper nouns if the noun is capped.
    // This, I think, is wrong.   A proper noun only has a definite article
    // if we want a definite, and in that case everyone has one, cf: creature
    // usedefinite clause.
    // We need to search through our possible noun phrase for any
    // capped word, as "evil Baezl'bub's black heart" will cause
    // problems otherwise.
    // We do not want to search of clauses so "scroll of READ ME" isn't
    // flagged as a proper noun.
    // This is still not correct.  It should be "the corpse of
    // Baezl'bug", not "a" or just "corpse".
    {
	char 		*prefix, *propernoun, *suffix;
	char 		*tmp;
	const char 	*capsearch;
	int		 lastspace = true;
	bool		 isproper = false;
	bool		 isplural;

	tmp = strdup(noun);
	
	gram_extractofclause(tmp, prefix, propernoun, suffix);

	// We want to search the prefix and proper noun for
	// caps, not the suffix as we care not for " of " clauses.
	isplural = gram_isplural(noun);

	for (capsearch = prefix; capsearch && *capsearch; capsearch++)
	{
	    if (lastspace && isupper(*capsearch))
		isproper = true;
	    lastspace = isspace(*capsearch);
	}
	lastspace = true;
	for (capsearch = propernoun; capsearch && *capsearch; capsearch++)
	{
	    if (lastspace && isupper(*capsearch))
		isproper = true;
	    lastspace = isspace(*capsearch);
	}
	free(tmp);

	if (isproper)
	    return "";
    }

    // Check if first letter is a vowel.
    if (!gram_isvowel(*noun))
    {
	// These are usually pretty straight forward.  However,
	// some words such as "honourable" cause problems.  Contrast
	// with "horse" and "hone".
	if (*noun == 'h')
	{
	    // Honour:
	    if (!strncmp(noun, "hono", 4))
		return "an ";

	    // Lots of other cases likely follow...
	}
	
	return "a ";
    }
    // It is likely "an", however, a eucliedean geometry.
    // However, "an eulerian proof".
    if (!strncmp(noun, "euc", 3))
	return "a ";

    // The entire class of "u" causes problems.  Many words, such as
    // "usually", are pronounced with a "y" prefix, so should use "a ".
    // Some, such as "urban" remain to cause us unfortunate problems.
    // The rough rule here is:
    // Determine if u is hard or soft.  If two letters after the
    // u is a vowel, it is "utility", "usual", or "ubiquitous", so
    // it is "a ".
    // If it is two consonents in a row, it is a "urbane" usage, so
    // should use "an ".
    if (*noun == 'u')
    {
	// The single letter 'u' also use "a ".
	// Yet, if there is no third character, (Great city of Ur?) it
	// should be treated as the double consonent case.
	if (!noun[1] || gram_isvowel(noun[2]))
	{
	    return "a ";
	}
	// We have either a u followed by two consonents or two vowels
	// in a row.  Two vowels in a row we consider to be a "a " case,
	// though I can't think of any.
	if (gram_isvowel(noun[1]))
	    return "a ";

	// Chain to default to "an "....
    }

    // Default to "an "...
    return "an ";
}

BUF
gram_createcount(const char *basename, int count, bool article)
{
    BUF		buf;

    buf.strcpy(basename);
    return gram_createcount(buf, count, article);
}

BUF
gram_createcount(BUF basename, int count, bool article)
{
    BUF			 result;
    BUF			 plural;

    if (count != 1)
	plural = gram_makeplural(basename);

    if (!count)
    {
	result.sprintf("no %s", plural.buffer());
    }
    else if (count == 1)
    {
	if (article)
	{
	    result.strcpy(gram_getarticle(basename));
	    result.strcat(basename);
	}
	else
	{
	    return basename;
	}
    }
    else
    {
	result.sprintf("%d %s", count, plural.buffer());
    }

    return result;
}

BUF
gram_createplace(int place)
{
    BUF			 buf;
    const char		*ext;
    int			 rem, upperrem;

    rem = place % 10;
    // I fucking hate this misfeature of C!
    if (rem < 0)
	rem += 10;

    upperrem = (place - rem) / 10;
    upperrem %= 10;
    if (upperrem < 0)
	upperrem += 10;

    if (upperrem == 1)
    {
	// Eleventies!
	rem = 0;
    }
    
    switch (rem)
    {
	case 1:
	    ext = "st";
	    break;
	case 2:
	    ext = "nd";
	    break;
	case 3:
	    ext = "rd";
	    break;
	default:
	    ext = "th";
	    break;
    }

    buf.sprintf("%d%s", place, ext);

    return buf;
}

// Static tables of verbs...
const char *glb_verbBE[2][NUM_VERBS] =
{ { "am", "are", "is", "is", "is", "are", "are", "are", "are", "are" },
  { "was", "were", "was", "was", "was", "were", "were", "were", "were", "were" } };

const char *glb_pronoun[NUM_VERBS] =
{ "I", "you", "he", "she", "it", "we", "you", "they", "they", "they" };
const char *glb_possessive[NUM_VERBS] =
{ "my", "your", "his", "her", "its", "our", "your", "their", "their", "their" };
const char *glb_ownership[NUM_VERBS] =
{ "mine", "yours", "his", "hers", "its", "ours", "yours", "theirs", "theirs", "theirs" };
const char *glb_reflexive[NUM_VERBS] =
{ "myself", "yourself", "himself", "herself", "itself", "ourselves", "yourselves", "themselves", "themselves", "themselves" };
const char *glb_accusative[NUM_VERBS] =
{ "me", "you", "him", "her", "it", "us", "you", "them", "them" , "them" };

const char *
gram_getpronoun(VERB_PERSON person)
{
    return glb_pronoun[person];
}

const char *
gram_getpossessive(VERB_PERSON person)
{
    return glb_possessive[person];
}

const char *
gram_getownership(VERB_PERSON person)
{
    return glb_ownership[person];
}

const char *
gram_getreflexive(VERB_PERSON person)
{
    return glb_reflexive[person];
}

const char *
gram_getaccusative(VERB_PERSON person)
{
    return glb_accusative[person];
}

BUF
gram_conjugate(const char *verb, VERB_PERSON person, bool past)
{
    // First, we determine if it is a multiword verb.  For example,
    // "spit at" should be conjugated with the preposition isolated.
    // We take the first full english word, conjugate it, and append
    // the rest of the initial verb.
    //
    // As I write this, I'm sitting in the Maple Leaf lounge relaxing
    // on a comfortable leather seat.
    // First class travel - If only I could get used to it!
    const char 		*space;
    BUF			 buf;

    space = strchr(verb, ' ');
    if (space)
    {
	char		*tmp;

	tmp = (char *)malloc(space - verb + 1);
	memcpy(tmp, verb, space - verb);
	tmp[space-verb] = '\0';

	buf = gram_conjugate(tmp, person, past);
	buf.strcat(space);
	free(tmp);
	return buf;
    }
    
    // As most verb decisions are based off the ending, we store the
    // string reversed ending here:
    char		 ending[10];
    size_t		 verblen = strlen(verb);

    // Build the ending...
    const char		*e;
    int			 epos = 0;
    for (e = verb; *e; e++);
    e--;
    for (epos = 0; epos < 10; epos++)
    {
	if (e < verb)
	    ending[epos] = '\0';
	else
	    ending[epos] = *e;
	e--;
    }
    
    // Check for crazy verbs...
    if (!strcmp(verb, "be"))
    {
	buf.reference(glb_verbBE[past][person]);
	return buf;
    }

    if (!strcmp(verb, "have"))
    {
	if (past)
	    buf.reference("had");
	else if (person == VERB_HE || person == VERB_SHE || person == VERB_IT)
	    buf.reference("has");
	else
	    buf.reference("have");
	return buf;
    }

    // Now, build the verb from the infinitive...
    if (past)
    {
	buf.strcpy(verb);
    }
    else
    {
	switch (person)
	{
	    case VERB_I:
		// hit->hit
		buf.strcpy(verb);
		break;
		
	    case VERB_HE:
	    case VERB_SHE:	
	    case VERB_IT:
		// hit->hits
		// miss->misses
		// bash->bashes.
		// fly->flies.
		// say->says
		// go->goes
		// watch->watches
		// fix->fixes
		// buzz->buzzes
		// have->has
		// catch->catches
		if (!strcmp(verb, "have"))
		{
		    buf.reference("has");
		}
		else if ((*ending == 's') ||
		    !strncmp(ending, "hs", 2) ||
		    !strncmp(ending, "hc", 2) ||
		    (*ending == 'o') ||
		    (*ending == 'x') ||
		    (*ending == 'z'))
		{
		    buf.sprintf("%ses", verb);
		}
		else if (*ending == 'y')
		{
		    if (gram_isvowel(ending[1]))
		    {
			// say -> says
			buf.sprintf("%ss", verb);
		    }
		    else
		    {
			// fly -> flies
			buf.strcat(verb);
			// Have to manually end it.  strncpy
			// doesn't write the terminating null.
			buf.evildata()[verblen-1] = '\0';
			buf.strcat("ies");
		    }
		}
		else
		{
		    buf.sprintf("%ss", verb);
		}
		break;

	    // These are pretty identical
	    case VERB_YOU:
	    case VERB_YALL:
		// hit->hit
		buf.sprintf("%s", verb);
		break;

	    case VERB_WE:
		// hit->hit
		buf.sprintf("%s", verb);
		break;
		
	    case VERB_HES:
	    case VERB_SHES:	
	    case VERB_THEY:
		// hit->hit
		buf.sprintf("%s", verb);
		break;

	    default:
		assert(!"Unhandled VERB!");
		buf.strcpy(verb);
		break;
	}
    }

    return buf;
}

BUF
gram_capitalize(const char *str)
{
    BUF		srcbuf;

    // We don't know life time of str so only safe bet is to make
    // a hard copy :<
    srcbuf.strcpy(str);
    return gram_capitalize(srcbuf);
}

BUF
gram_capitalize(BUF buf)
{
    bool		hard = false;
    char		*s;
    bool		docaps = true;	// Start of sentence, caps.
    BUF			result;
    const char		*str = buf.buffer();

    result = buf;
    
    // THis cast to char * is safe as we only assign to s after a harden.
    for (s = (char *) str; *s; s++)
    {
	if (!isspace(*s))
	{
	    // Note we do not eat up caps if we get a non-alpha, ie:
	    // "foo bar" will become "Foo bar" (ignoring the quote)
	    // Note that numerical keys are ignored, so
	    // "+1 mace" becomes "+1 Mace"
	    if (docaps && isalpha(*s))
	    {
		if (islower(*s))
		{
		    if (!hard)
		    {
			result.uniquify();
			s = result.evildata() + (s - str);
			hard = true;
		    }
		    *s = toupper(*s);
		}
		// Eat the caps.
		docaps = false;
	    }

	    // Determine if this is end-sentence, if so, the next
	    // char should be capped.
	    if (gram_isendsentence(*s))
		docaps = true;
	}
    }
    
    return result;
}

//
// This turns the given string into a possessive
// you goes to your, foo to foo's, and bars to bars'.
//
BUF
gram_makepossessive(const char *str)
{
    BUF		strbuf;

    strbuf.strcpy(str);
    return gram_makepossessive(strbuf);
}

BUF
gram_makepossessive(BUF str)
{
    BUF			 result;
    
    // Special cases...
    if (!str.strcmp("I"))
	result.reference("my");
    if (!str.strcmp("you"))
	result.reference("your");
    if (!str.strcmp("it"))
	result.reference("its");	// Better not let this go normal rule :>
    if (!str.strcmp("he"))
	result.reference("his");
    if (!str.strcmp("she"))
	result.reference("her");
    if (!str.strcmp("we"))
	result.reference("our");
    if (!str.strcmp("they"))
	result.reference("their");

    if (result.isstring())
	return result;

    // Now, standard case...
    result = str;
    result.uniquify();

    if (result.lastchar() == 's')
	result.strcat("'");
    else
	result.strcat("'s");

    return result;
}
