/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        msg.cpp ( Live once Library, C++ )
 *
 * COMMENTS:
 */

#include "msg.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "panel.h"
#include "grammar.h"
#include "mob.h"
#include "item.h"
#include "buf.h"

#include "thread.h"

PANEL *glbPanel = 0;
LOCK	glbMsgLock;

bool glbBlankNewTurn = false;

void
msg_update()
{
    glbPanel->redraw();
}

void 
msg_registerpanel(PANEL *panel)
{
    glbPanel = panel;
}

int
msg_gethistoryline()
{
    return glbPanel->getHistoryLine();
}

void
msg_clearhistory()
{
    glbPanel->clearHistory();
}

void
msg_scrolltohistory(int line)
{
    AUTOLOCK	a(glbMsgLock);
    glbPanel->scrollToHistoryLine(line);

    // This is a hack for save scummer.  We know our last save point
    // was a > prompt, so we need to restore said prompt.
    glbPanel->setCurrentLine("> ");
    glbBlankNewTurn = true;
}

void
msg_report(const char *msg)
{
    AUTOLOCK	a(glbMsgLock);
    BUF		buf;
    buf.reference(msg);

    // Don't want to have to worry about appending all the time!
    if (gram_isendsentence(buf.lastchar()))
	buf.strcat("  ");
    
    glbPanel->appendText(buf);
    glbBlankNewTurn = false;
}

void
msg_getString(const char *prompt, char *buf, int len)
{
    glbPanel->getString(prompt, buf, len);
}

void
msg_quote(const char *msg)
{
    AUTOLOCK	a(glbMsgLock);
    glbPanel->setIndent(1);
    glbPanel->newLine();
    msg_report(msg);
    glbPanel->setIndent(0);
}

void
msg_newturn()
{
    AUTOLOCK	a(glbMsgLock);
    if (!glbBlankNewTurn)
    {
	glbBlankNewTurn = true;
	// We do not want double new lines!
	if (!glbPanel->atNewLine())
	    glbPanel->newLine();

	glbPanel->appendText("> ");
    }
}

//
// Builder functions that respect the triple possibilities.
//
VERB_PERSON
msg_getPerson(MOB *m, ITEM *i, const char *s)
{
    VERB_PERSON		person = VERB_IT;

    if (m)
    {
	person = m->getPerson();
    }
    else if (i)
    {
	person = i->getPerson();
    }
    else if (s)
    {
	person = VERB_IT;
	if (gram_isnameplural(s))
	    person = VERB_THEY;
    }

    return person;
}


BUF
msg_buildVerb(const char *verb, MOB *m_subject, ITEM *i_subject, const char *s_subject)
{
    VERB_PERSON		person;

    person = msg_getPerson(m_subject, i_subject, s_subject);

    return gram_conjugate(verb, person);
}

BUF
msg_buildReflexive(MOB *m, ITEM *i, const char *s)
{
    VERB_PERSON		p;
    BUF			buf;

    p = msg_getPerson(m, i, s);

    buf.reference(gram_getreflexive(p));
    return buf;
}

BUF
msg_buildPossessive(MOB *m, ITEM *i, const char *s)
{
    VERB_PERSON		p;
    BUF			buf;

    p = msg_getPerson(m, i, s);

    buf.reference(gram_getpossessive(p));
    return buf;
}

BUF
msg_buildFullName(MOB *m, ITEM *i, const char *s, bool usearticle = true)
{
    BUF			 rawname, buf;
    BUF			 result;

    rawname.reference("no tea");
    if (m)
    {
	rawname.reference(m->getName());
    }
    else if (i)
    {
	rawname = i->getName();
    }
    else if (s)
	rawname.reference(s);
    
    const char		*art;

    if (usearticle)
	art = gram_getarticle(rawname);
    else
	art = "";

    buf.sprintf("%s%s", art, rawname.buffer());

    return buf;
}

//
// This is the universal formatter
//
void
msg_format(const char *msg, MOB *m_subject, ITEM *i_subject, const char *s_subject, const char *verb, MOB *m_object, ITEM *i_object, const char *s_object)
{
    AUTOLOCK	a(glbMsgLock);
    BUF			 buf;
    BUF			 newtext;

    while (*msg)
    {
	if (*msg == '%')
	{
	    newtext.reference("");
	    switch (msg[1])
	    {
		case '%':
		    // Pure %.
		    newtext.reference("%");
		    break;

		case '<':
		    // Escapped <
		    newtext.reference("<");
		    break;
		    
		case 'v':
		    // Conjugate the given verb & append.
		    assert(verb);
		    newtext = msg_buildVerb(verb, m_subject, i_subject, s_subject);
		    break;

		case 'S':
		    newtext = msg_buildFullName(m_subject, i_subject, s_subject);
		    break;

		case 'r':
		    newtext = msg_buildPossessive(m_subject, i_subject, s_subject);
		    break;

		case 'O':
		    if (m_subject && (m_subject == m_object) ||
			i_subject && (i_subject == i_object))
		    {
			// Reflexive case!
			newtext = msg_buildReflexive(m_object, i_object, s_object);
		    }
		    else
			newtext = msg_buildFullName(m_object, i_object, s_object);
		    break;
		case 'o':
		    if (m_subject && (m_subject == m_object) ||
			i_subject && (i_subject == i_object))
		    {
			// Reflexive case!
			newtext = msg_buildReflexive(m_object, i_object, s_object);
		    }
		    else
			newtext = msg_buildFullName(m_object, i_object, s_object, false);
		    break;
	    }

	    msg += 2;
	    // Append the new text
	    buf.strcat(newtext);
	}
	else if (*msg == '<')
	{
	    char *v = strdup(&msg[1]);
	    char *startv = v;
	    
	    msg++;
	    while (*v && *v != '>')
	    {
		msg++;
		v++;
	    }
	    *v = 0;
	    // Must be closed!
	    assert(*msg == '>');
	    if (*msg == '>')
		msg++;

	    newtext = msg_buildVerb(startv, m_subject, i_subject, s_subject);

	    buf.strcat(newtext);

	    free(startv);
	}
	else
	{
	    // Normal character!
	    buf.append(*msg++);
	}
    }

    // If it ends with puntuation, add spaces.
    if (buf.isstring() && gram_isendsentence(buf.lastchar()))
    {
	buf.strcat("  ");
    }

    // Formatted into buf.  Capitalize & print.
    newtext = gram_capitalize(buf);

    msg_report(newtext);
}

//
// These are the specific instantiations.
//
void
msg_format(const char *msg, MOB *subject)
{
    msg_format(msg, subject, 0, 0, 0, 0, 0, 0);
}

void
msg_format(const char *msg, ITEM *subject)
{
    msg_format(msg, 0, subject, 0, 0, 0, 0, 0);
}

void
msg_format(const char *msg, MOB *subject, MOB *object)
{
    msg_format(msg, subject, 0, 0, 0, object, 0, 0);
}

void
msg_format(const char *msg, MOB *subject, ITEM *object)
{
    msg_format(msg, subject, 0, 0, 0, 0, object, 0);
}

void
msg_format(const char *msg, MOB *subject, const char *verb, MOB *object)
{
    msg_format(msg, subject, 0, 0, verb, object, 0, 0);
}

void
msg_format(const char *msg, MOB *subject, const char *object)
{
    msg_format(msg, subject, 0, 0, 0, 0, 0, object);
}
