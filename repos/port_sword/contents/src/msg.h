/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        msg.h ( Live Once Library, C++ )
 *
 * COMMENTS:
 */

#ifndef __msg__
#define __msg__

class PANEL;
class MOB;
class ITEM;

#include "buf.h"

// Sets where messages go.
void msg_registerpanel(PANEL *panel);

// Returns current scroll position of panel
int msg_gethistoryline();

void msg_clearhistory();

// Scrolls back to previous history location
void msg_scrolltohistory(int line);

// Prints the given message.  Capitalizes.
void msg_report(const char *msg);
inline void msg_report(BUF buf)
{ msg_report(buf.buffer()); }

// Quotes the message by proper indentation.
void msg_quote(const char *msg);
inline void msg_quote(BUF buf)
{ msg_quote(buf.buffer()); }

void msg_newturn();

// Triggers redraw of panel.  You still need to do gfx_udate though.
void msg_update();

void msg_getString(const char *prompt, char *buf, int len);

// These are different public versions of the format code.
// They all dump the result to the registered panel.
// The codes are:
// %% - %
// %< - <
// %v - verb conjugated by subject
// %r - possessive.
// %S - Subject
// %O - Object
// %o - object, no article
// <verb> - verb to conjugate by subject.
void msg_format(const char *msg, MOB *subject);
void msg_format(const char *msg, ITEM *subject);
void msg_format(const char *msg, MOB *subject, MOB *object);
void msg_format(const char *msg, MOB *subject, ITEM *object);
void msg_format(const char *msg, MOB *subject, const char *verb, MOB *object);
inline void msg_format(const char *msg, MOB *subject, BUF verb, MOB *object)
{ msg_format(msg, subject, verb.buffer(), object); }
void msg_format(const char *msg, MOB *subject, const char *object);
inline void msg_format(const char *msg, MOB *subject, BUF object)
{ msg_format(msg, subject, object.buffer()); }

#endif
