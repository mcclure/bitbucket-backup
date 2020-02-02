/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        text.h ( Live Once Library, C++ )
 *
 * COMMENTS:
 */

#ifndef __text__
#define __text__

#include "buf.h"

void text_init();
void text_shutdown();

BUF text_lookup(const char *dict, const char *word);
inline BUF text_lookup(const char *dict, BUF word)
{ return text_lookup(dict, word.buffer()); }
BUF text_lookup(const char *dict, const char *word, const char *subword);

// Sets and \n or \r to \0, useful for the results of
// is.getline() if the source file was a dos system.
void text_striplf(char *line);

bool text_hasnonws(const char *line);
int text_firstnonws(const char *line);
int text_lastnonws(const char *line);

#endif

