/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        text.cpp ( Live Once Library, C++ )
 *
 * COMMENTS:
 */

#include "text.h"

#include <fstream>
using namespace std;

#include "ptrlist.h"
#include "grammar.h"
#include "rand.h"

PTRLIST<char *> glbTextEntry;
PTRLIST<char *> glbTextKey;

char *
text_append(char *oldtxt, const char *append)
{
    int		len;
    char	*txt;
    
    len = (int)strlen(oldtxt) + (int)strlen(append) + 5;
    txt = (char *) malloc(len);
    strcpy(txt, oldtxt);
    strcat(txt, append);
    free(oldtxt);
    return txt;
}

void
text_striplf(char *line)
{
    while (*line)
    {
	if (*line == '\n' || *line == '\r')
	{
	    *line = '\0';
	    return;
	}
	line++;
    }
}

bool
text_hasnonws(const char *line)
{
    while (*line)
    {
	if (!ISSPACE(*line))
	    return true;
	line++;
    }
    return false;
}

int
text_firstnonws(const char *line)
{
    int		i = 0;
    while (line[i])
    {
	if (!ISSPACE(line[i]))
	    return i;
	i++;
    }
    return i;
}

int
text_lastnonws(const char *line)
{
    int		i = 0;
    while (line[i])
	i++;

    while (i > 0)
    {
	if (line[i] && !ISSPACE(line[i]))
	    return i;
	i--;
    }
    return i;
}

void
text_init()
{
    ifstream	is(SWORD_CFG_DIR "text.txt");
    char	line[500];
    bool	hasline = false;
    char	*text;

    while (hasline || is.getline(line, 500))
    {
	text_striplf(line);
	hasline = false;

	// Ignore comments.
	if (line[0] == '#')
	    continue;

	// See if an entry...
	if (!ISSPACE(line[0]))
	{
	    // This line is a key.
	    glbTextKey.append(strdup(line));
	    // Rest is the message...
	    text = strdup("");
	    while (is.getline(line, 500))
	    {
		text_striplf(line);
		int		firstnonws;

		for (firstnonws = 0; line[firstnonws] && ISSPACE(line[firstnonws]); firstnonws++);

		if (!line[firstnonws])
		{
		    // Completely blank line - insert a hard return!
		    text = text_append(text, "\n\n");
		}
		else if (!firstnonws)
		{
		    // New dictionary entry, break out!
		    hasline = true;
		    break;
		}
		else
		{
		    // Allow some ascii art...
		    if (firstnonws > 4)
		    {
			text = text_append(text, "\n");
			firstnonws = 4;
		    }

		    // Append remainder.
		    // Determine if last char was end of sentence.
		    if (*text && text[strlen(text)-1] != '\n')
		    {
			if (gram_isendsentence(text[strlen(text)-1]))
			    text = text_append(text, " ");
			text = text_append(text, " ");
		    }

		    text = text_append(text, &line[firstnonws]);
		}
	    }

	    // Append the resulting text.
	    glbTextEntry.append(text);
	}
    }
}

void
text_shutdown()
{
    int		i;

    for (i = 0; i < glbTextKey.entries(); i++)
    {
	free(glbTextKey(i));
	free(glbTextEntry(i));
    }
}

BUF
text_lookup(const char *key)
{
    BUF		 buf;
    int		i;

    for (i = 0; i < glbTextKey.entries(); i++)
    {
	if (!strcmp(key, glbTextKey(i)))
	{
	    buf.reference(glbTextEntry(i));
	    return buf;
	}
    }

    buf.sprintf("Missing text entry: \"%s\".\n", key);

    return buf;
}

BUF
text_lookup(BUF buf)
{
    return text_lookup(buf.buffer());
}

BUF
text_lookup(const char *dict, const char *word)
{
    BUF		buf;

    buf.sprintf("%s::%s", dict, word);
    return text_lookup(buf);
}

BUF
text_lookup(const char *dict, const char *word, const char *subword)
{
    BUF		buf;

    buf.sprintf("%s::%s::%s", dict, word, subword);
    return text_lookup(buf);
}
