//
// Enummaker
// This takes a {} delimmitted text file and outputs the required
// sets of enumerations to properly build everything for Powder.
//

#include <ctype.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

// Portability crap
#ifndef WIN32
	#define stricmp strcasecmp
#endif

int		glbcurline = 0;
char		*glbpushedtoken = 0;

enum TYPE_NAMES
{
    TYPE_BOOL,
    TYPE_U8,
    TYPE_U16,
    TYPE_S8,
    TYPE_S16,
    TYPE_CST,
    TYPE_INT,
    TYPE_ENM,
    TYPE_ENM16,
    TYPE_DICE,
    TYPE_ENMLIST,
    NUM_TYPES
};

int glbBitSize[NUM_TYPES] =
{
    1,
    8,
    16,
    8,
    16,
    32,
    32,
    8,		// enum is u8
    16,		// This is a lie as it is 3 u16s, however we care about align
    32,		// pointer
};

const char *
getTypeName(TYPE_NAMES name)
{
    switch (name)
    {
	case TYPE_BOOL:
	    return "bool ";
	case TYPE_U8:
	    return "u8 ";
	case TYPE_S8:
	    return "s8 ";
	case TYPE_U16:
	    return "u16 ";
	case TYPE_S16:
	    return "s16 ";
	case TYPE_CST:
	    return "const char *";
	case TYPE_INT:
	    return "int ";
	case TYPE_ENM:
	    return "u8 ";
	case TYPE_ENM16:
	    return "u16 ";
	case TYPE_DICE:
	    return "DICE ";
	case TYPE_ENMLIST:
	    return "const char *";
    }
    return "UNKNOWN ";
}

const char *
getTypePostfix(TYPE_NAMES name)
{
    switch (name)
    {
	case TYPE_BOOL:
	    return " : 1";
	case TYPE_U8:
	case TYPE_S8:
	case TYPE_U16:
	case TYPE_S16:
	case TYPE_CST:
	case TYPE_INT:
	case TYPE_ENM:
	case TYPE_ENM16:
	case TYPE_DICE:
	case TYPE_ENMLIST:
	    return "";
    }
    return "UNKNOWN ";
}

TYPE_NAMES
getTypeName(const char *name)
{
    if (!stricmp(name, "BOOL"))
	return TYPE_BOOL;
    else if (!stricmp(name, "U8"))
	return TYPE_U8;
    else if (!stricmp(name, "S8"))
	return TYPE_S8;
    else if (!stricmp(name, "U16"))
	return TYPE_U16;
    else if (!stricmp(name, "S16"))
	return TYPE_S16;
    else if (!stricmp(name, "CST"))
	return TYPE_CST;
    else if (!stricmp(name, "INT"))
	return TYPE_INT;
    else if (!stricmp(name, "ENM"))
	return TYPE_ENM;
    else if (!stricmp(name, "ENM16"))
	return TYPE_ENM16;
    else if (!stricmp(name, "DICE"))
	return TYPE_DICE;
    else if (!stricmp(name, "ENMLIST"))
	return TYPE_ENMLIST;

    return NUM_TYPES;
}

char
eatWS(istream &is)
{
    char		c;
    while (is)
    {
	is.read(&c, 1);
	if (c == '\n')
	    glbcurline++;
	if (!isspace((unsigned char) c))
	    return c;
    }
    return 0;
}

// Max depth of one.
void
pushToken(char *oldtoken)
{
    if (glbpushedtoken)
    {
	cerr << "Overwrote old token " << glbpushedtoken << endl;
    }
    glbpushedtoken = oldtoken;
}

// This allocates a new buffer that is returned, use free to get rid of it.
char *
getToken(istream &is)
{
    char		buf[512];
    char		c;
    char		*d;
    bool		dq = false;
    bool		sq = false;

    // Check if there is a pushed token, if so, use it.
    if (glbpushedtoken)
    {
	d = glbpushedtoken;
	glbpushedtoken = 0;
	return d;
    }

    c = eatWS(is);
    if (!is)
	return 0;

    d = buf;
    *d++ = c;
    if (c == '\"')
	dq = true;
    if (c == '\'')
	sq = true;
    
    while (is)
    {
	is.read(&c, 1);
	if (c == '\n')
	    glbcurline++;

	if (c == '\\')
	{
	    // Blindly read next char...  Join multiple lines.
	    is.read(&c, 1);
	    if (c != '\n')
		*d++ = c;
	    else
		glbcurline++;
	    continue;
	}
	if (dq)
	{
	    if (c == '\"')
	    {
		// Proper end of a quote...
		*d++ = c;
		break;
	    }
	    if (c == '\n')
	    {
		// New line in quote, bad business.
		cerr << "Newline in quote line " << glbcurline << endl;
		break;
	    }
	}
	else if (sq)
	{
	    if (c == '\'')
	    {
		// Proper end of a quote...
		*d++ = c;
		break;
	    }
	    if (c == '\n')
	    {
		// New line in quote, bad business.
		cerr << "Newline in quote line " << glbcurline << endl;
		break;
	    }
	}
	else
	{
	    if (isspace((unsigned char) c))
		break;
	}
	*d++ = c;
    }

    // Terminate...
    *d++ = '\0';

    // And return..
    return strdup(buf);
}

int
atoi_verify(const char *text)
{
    const char *c = text;
    for (; *c; c++)
    {
	unsigned char d = *c;
	if (!isdigit(d)
	    && (c != text || d != '+')
	    && (c != text || d != '-'))
	{
	    break;
	}
    }
    if (*c)
    {
	// unexpected char.
	cerr << "ERROR: Bad atoi  " << text << " on line " << glbcurline << endl;
    }

    return atoi(text);
}

// This returns a dice initializer of the form { num, side, bonus }
// given a plain text dice of the form [#[d|D]#][[+|-]#]|[+|-]#
char *
parseDice(const char *rawsrc)
{
    char		buf[512];
    char		*d;
    int			num, sides, bonus = 0;
    char		*src;

    src = strdup(rawsrc);

    d = strchr(src, 'd');
    if (!d)
	d = strchr(src, 'D');
    
    if (!d)
    {
	// No "d" or "D" so must be a straight bonus.
	sprintf(buf, "{ 0, 0, %d }", atoi_verify(src));
    }
    else
    {
	// Extract number..
	*d = '\0';
	num = atoi_verify(src);
	src = d + 1;
	d = strchr(src, '+');
	if (!d)
	    d = strchr(src, '-');
	if (d)
	{
	    // We have a bonus...
	    bonus = atoi_verify(d);
	    *d = '\0';
	}
	// Read out the sides...
	sides = atoi_verify(src);
	sprintf(buf, "{ %d, %d, %d }", num, sides, bonus);
    }

    return strdup(buf);
}

// This returns true or false given some form of plain text code.
// If it is not recongizned, an error is output.
const char *
parseBool(const char *rawsrc)
{
    if (!stricmp(rawsrc, "true"))
	return "true";
    else if (!stricmp(rawsrc, "false"))
	return "false";
    else if (!stricmp(rawsrc, "on"))
	return "true";
    else if (!stricmp(rawsrc, "off"))
	return "false";

    // Try for raw numbers.
    if (!stricmp(rawsrc, "0"))
	return "false";
    else if (!stricmp(rawsrc, "1"))
	return "true";

    // Unided number.
    cerr << "ERROR: Bad bool value " << rawsrc << endl;

    if (atoi_verify(rawsrc))
	return "true";
    else
	return "false";
}

const char *
makeLower(const char *src)
{
    static char 	buf[512];
    char		*d;

    d = buf;
    while (*src)
    {
	*d++ = tolower(*src++);
    }
    *d++ = 0;
    return buf;
}

class ITEM
{
public:
    ITEM();
    char		*myVarName;
    char		*myValue;
    char		*myPrefix;
    TYPE_NAMES		 myType;
    ITEM		*myNext;

    // Calculates number of bits needed to store.
    int			 getBitSize();
};

ITEM::ITEM()
{
    myVarName = 0;
    myValue = 0;
    myPrefix = 0;
    myNext = 0;
}

int
ITEM::getBitSize()
{
    return glbBitSize[myType];
}

// This is a definition of an enumeration.
class DEF
{
public:    
    DEF();
    ITEM		*getItem(const char *varname);	
    ITEM		*appendItem();
    void		 sortItems();
    
    char		*myToken;
    ITEM		*myItem;		// Linked list.
    DEF			*myNext;
    int			 myIndex;
};

DEF::DEF()
{
    myToken = 0;
    myItem = 0;
    myNext = 0;
    myIndex = -666;
}

ITEM *
DEF::getItem(const char *varname)
{
    ITEM	*item;

    for (item = myItem; item; item = item->myNext)
    {
	if (!strcmp(item->myVarName, varname))
	    return item;
    }
    return 0;
}

ITEM *
DEF::appendItem()
{
    ITEM	*item, *last;

    item = new ITEM;
    
    for (last = myItem; last && last->myNext; last = last->myNext)
    {
    }

    if (!last)
	myItem = item;
    else
	last->myNext = item;

    return item;
}

void
DEF::sortItems()
{
    ITEM *sorted, *min, *minprev, *item, *prev;

    sorted = 0;
    // This is O(n^2), but so is our append.
    while (myItem)
    {
	// Find the minimum.  Go as far as possible
	// in the list as we will prepend to sorted
	// and want a stable sort.
	prev = 0;
	min = 0;
	for (item = myItem; item; item = item->myNext)
	{
	    // Determine if item is smallest size
	    if (!min ||
		item->getBitSize() <= min->getBitSize())
	    {
		min = item;
		minprev = prev;
	    }
	    prev = item;
	}

	// Extract the minimum.
	if (minprev)
	    minprev->myNext = min->myNext;
	else
	    myItem = min->myNext;

	// Put min on front of list.
	min->myNext = sorted;
	sorted = min;
    }

    // Set our item list to be sorted
    myItem = sorted;
}

class TYPE
{
public:
    TYPE();
    DEF			*findDef(const char *deftype);
    int			 getDefNumber(const char *deftype);
    DEF			*appendDef(char *deftype);

    char		*myPrefix;		// Ie, TILE or MOB

    DEF			*myDefault;
    DEF			*myDef;

    TYPE		*myNext;
};

TYPE::TYPE()
{
    myDefault = 0;
    myDef = 0;
    myNext = 0;
    myPrefix = 0;
}

DEF *
TYPE::findDef(const char *deftype)
{
    DEF		*def;

    for (def = myDef; def; def = def->myNext)
    {
	if (!strcmp(def->myToken, deftype))
	    return def;
    }
    return 0;
}

int
TYPE::getDefNumber(const char *deftype)
{
    DEF		*def;
    int		 idx = 0;

    for (def = myDef; def; def = def->myNext)
    {
	if (!strcmp(def->myToken, deftype))
	    return idx;
	idx++;
    }
    cerr << "ERROR: Unknown enumeration " << deftype << endl;
    return 0;
}

DEF *
TYPE::appendDef(char *deftype)
{
    DEF		*last = 0, *def;

    for (last = myDef; last && last->myNext; last = last->myNext)
    {
    }

    def = new DEF;
    def->myToken = deftype;
    
    if (!last)
	myDef = def;
    else
	last->myNext = def;

    return def;
}

TYPE *
findType(TYPE *root, const char *type)
{
    while (root)
    {
	if (!strcmp(root->myPrefix, type))
	    return root;
	root = root->myNext;
    }
    return 0;
}

TYPE *
appendType(TYPE *&root)
{
    TYPE	*t;

    t = new TYPE;
    t->myNext = root;
    root = t;

    return t;
}

// This processes
char *
parseEnumList(const char *clist, TYPE *enumtype)
{
    char		*list = strdup(clist);

    char		*e, *s;
    char		 tmp[10];
    char		*r = tmp;
    static char		 output[1024];
    int			 val;
    
    strcpy(output, "\"");
    for (s = list; *s; )
    {
	for (e = s; *e && *e != ' '; e++);
	if (*e == ' ')
	{
	    *e = '\0';
	    e++;
	}

	// s is now a word to test.
	val = enumtype->getDefNumber(s);
	sprintf(tmp, "\\x%02x", val);
	strcat(output, tmp);

	s = e;
    }
    strcat(output, "\"");

    return output;
}


int 
main(int argc, char* argv[])
{
    TYPE		*type, *root = 0;
    DEF			*def;
    ITEM		*item, *sitem;
    char		*token, *vartype, *varname, *varvalue;
    int			 start;
    
    if (argc < 2)
    {
	cerr << "Usage is " << argv[0] << " source.txt" << endl;
	cerr << "This will generate glbdef.h and glbdef.cpp" << endl;
	return -2;
    }

    ifstream		is(argv[1], ios::in);

    if (!is)
    {
	cerr << "Failure to open " << argv[1] << endl;
	return -1;
    }

    // Read in tokens until done...
    glbcurline = 0;
    while (token = getToken(is))
    {
	if (!strcmp(token, "COMMENT"))
	{
	    token = getToken(is);

	    if (strcmp(token, "{"))
	    {
		cerr << "Missing { on line " << glbcurline << endl;
		return -1;
	    }

	    // REad until matched
	    int		nest = 1;
	    start = glbcurline;
	    while (token = getToken(is))
	    {
		if (!strcmp(token, "{"))
		    nest++;
		else if (!strcmp(token, "}"))
		    nest--;
		if (!nest)
		    break;
	    }
	    if (!token)
	    {
		cerr << "Unexpected EOF starting from line " << start << endl;
	    }

	}
	else if (!strcmp(token, "DEFINE"))
	{
	    // We have a new type...
	    type = appendType(root);

	    // We get the type prefix.
	    token = getToken(is);
	    type->myPrefix = token;

	    start = glbcurline;

	    // We now load the type's stuff...
	    token = getToken(is);
	    if (strcmp(token, "{"))
	    {
		cerr << "Missing { on line " << glbcurline << endl;
		return -1;
	    }

	    def = new DEF;
	    type->myDefault = def;

	    // Load the defaults..
	    while (token = getToken(is))
	    {
		if (!strcmp(token, "}"))
		{
		    break;		// HIt the end..
		}

		vartype = token;
		varname = getToken(is);
		varvalue = getToken(is);
		
		item = def->appendItem();
		item->myVarName = varname;
		item->myType = getTypeName(vartype);
		if (item->myType == TYPE_ENM ||
		    item->myType == TYPE_ENM16 ||	
		    item->myType == TYPE_ENMLIST)
		{
		    item->myPrefix = varvalue;
		    item->myValue = getToken(is);
		}
		else
		    item->myValue = varvalue;
	    }

	    if (!token)
	    {
		cerr << "Unexpected EOF from line " << start << endl;
		return -1;
	    }
	    
	    def->sortItems();
	}
	else if (type = findType(root, token))
	{
	    // We found the type.  Now, get the definition name...
	    token = getToken(is);
	    
	    def = type->appendDef(token);

	    start = glbcurline;

	    token = getToken(is);

	    // Check for a number...
	    if (token && (isdigit((unsigned char) *token) || *token=='-'))
	    {
		def->myIndex = atoi_verify(token);
		token = getToken(is);
	    }
	    
	    if (!token || strcmp(token, "{"))
	    {
		// This dude has no content section.
		pushToken(token);
		continue;
	    }

	    // Load the definition...
	    while (token = getToken(is))
	    {
		if (!strcmp(token, "}"))
		{
		    break;		// HIt the end..
		}

		varname = token;
		varvalue = getToken(is);

		// First, we verify the default has this var name.  If not
		// we have an error.
		if (!(sitem = type->myDefault->getItem(varname)))
		{
		    cerr << "WARNING: Varname " << varname << " not in definition, line " << glbcurline << endl;
		}

		// Now, determine if we already have this value.  If we do,
		// we should replace, except for enum lists that append...
		item = def->getItem(varname);
		if (item && (sitem->myType == TYPE_ENMLIST || sitem->myType == TYPE_CST))
		{
		    // Properly append to this item...
		    char		*total;

		    total = (char *) malloc(strlen(item->myValue) + strlen(varvalue) + 3);
		    strcpy(total, item->myValue);
		    strcat(total, " ");
		    strcat(total, varvalue);
		    varvalue = total;
		}
		else if (item)
		{
		    cerr << "WARNING: Varname " << varname << " defined twice in definition, line " << glbcurline << endl;
		}
		
		if (!item)
		    item = def->appendItem();
		item->myVarName = varname;
		item->myValue = varvalue;
	    }

	    if (!token)
	    {
		cerr << "ERROR: Unexpected EOF from "  << start<< endl;
		return -1;
	    }
	}
	else
	{
	    cerr << "WARNING: Unknown token " << token << " on " << glbcurline << endl;
	}
    }

    ofstream		os("glbdef.h");
    ofstream		fs("glbdef.cpp");

    fs << "// Automagically generated by enummaker.exe." << endl;
    fs << "// DO NOT EDIT THIS FILE (Yes, I mean you!)" << endl;
    fs << endl;
    fs << "#include \"mygba.h\"" << endl;
    fs << "#include \"glbdef.h\"" << endl;

    os << "// Automagically generated by enummaker.exe." << endl;
    os << "// DO NOT EDIT THIS FILE (Yes, I mean you!)" << endl;
    os << "#ifndef __glbdef_h__" << endl;
    os << "#define __glbdef_h__" << endl;
    os << endl;
    os << "#include \"mygba.h\"" << endl;
    os << "#include \"rand.h\"" << endl;
    
    // Output...
    for (type = root; type; type = type->myNext)
    {
	// Output the enum..
	os << endl;
	os << "// Definitions for " << type->myPrefix << endl;
	os << "enum " << type->myPrefix << "_NAMES" << endl;
	os << "{" << endl;
	for (def = type->myDef; def; def = def->myNext)
	{
	    os << "    " << type->myPrefix << "_" << def->myToken;
	    if (def->myIndex != -666)
		os << " = " << def->myIndex;
	    os << "," << endl;
	}
	os << "    NUM_" << type->myPrefix << "S" << endl;
	os << "};" << endl;

	// Output the foreach loop for the enum
	os << endl;
	os << "// Macros for " << type->myPrefix << endl;
	os << "#define FOREACH_" << type->myPrefix << "(x) \\" << endl;
	os << "    for ((x) = (" << type->myPrefix << "_NAMES) 0; \\" << endl;
	os << "         (x) < NUM_" << type->myPrefix << "S; \\" << endl;
	os << "         (x) = (" << type->myPrefix << "_NAMES) ((int)(x)+1))" << endl;
	
	// Output the structure defintition...
	if (type->myDefault->myItem)
	{
	    os << endl;
	    os << "struct " << type->myPrefix << "_DEF" << endl;
	    os << "{" << endl;
	    for (item = type->myDefault->myItem; item; item = item->myNext)
	    {
		os << "    " << getTypeName(item->myType) << item->myVarName
		   << getTypePostfix(item->myType)
		   << ";" << endl;	
	    }
	    os << "};" << endl;
	    os << endl;

	    os << "extern const " << type->myPrefix << "_DEF glb_" 
	       << makeLower(type->myPrefix) << "defs[];" << endl;
	}

	// Output the actual definition...
	if (type->myDefault->myItem)
	{
	    fs << endl;
	    fs << "// Definitions for " << type->myPrefix << endl;
	    fs << "const " << type->myPrefix << "_DEF" << endl;
	    fs << "glb_" << makeLower(type->myPrefix) << "defs[NUM_"
	       << type->myPrefix << "S] =" << endl;
	    fs << "{" << endl;
	    for (def = type->myDef; def; def = def->myNext)
	    {
		fs << "  {" << endl;
		for (sitem = type->myDefault->myItem; sitem; sitem = sitem->myNext)
		{
		    item = def->getItem(sitem->myVarName);
		    if (!item) item = sitem;
		    if (sitem->myType == TYPE_ENM || 
			sitem->myType == TYPE_ENM16)
			fs << "    " << sitem->myPrefix << "_" 
			   << item->myValue << "," << endl;
		    else if (sitem->myType == TYPE_ENMLIST)
			fs << "    " 
			   << parseEnumList(item->myValue, 
				   findType(root, sitem->myPrefix) )
			   << "," << endl;
		    else if (sitem->myType == TYPE_DICE)
			fs << "    " << parseDice(item->myValue) << "," << endl;
		    else if (sitem->myType == TYPE_BOOL)
			fs << "    " << parseBool(item->myValue) << "," << endl;
		    else
			fs << "    " << item->myValue << "," << endl;
		}
		
		fs << "  }," << endl;
	    }
	    fs << "};" << endl;
	}
    }
    os << "#endif" << endl;
    return 0;
}

