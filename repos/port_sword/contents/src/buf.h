/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        buf.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Implements a simple character buffer.  Underlying data
 *	is reference counted and can be passed by value.
 *	Uses copy on write semantic.
 */

#ifndef __buf__
#define __buf__

#include <stdarg.h>

#include <iostream>
using namespace std;

#ifdef __APPLE__
#define SWORD_CFG_DIR ""
#else
#define SWORD_CFG_DIR "../"
#endif

// Returns number of active buffers.
// This should be zero most of the time.  Or at least bounded.
int
buf_numbufs();

// Internal buffer representation
class BUF_int;

class BUF
{
public:
    BUF();
    explicit BUF(int len);
    ~BUF();

    // Copy constructors
    BUF(const BUF &buf);
    BUF &operator=(const BUF &buf);

    // Allocates a blank buffer of the given size to ourself.
    void	allocate(int len);

    void	save(ostream &os) const;
    void	load(istream &is);

    // Sets us to an empty string, but not a shared empty string!
    void	clear();

    // Determines if we are a non-null and non-empty string
    bool	isstring() const;

    // Read only access.  Only valid so long as we are scoped and not
    // editted.
    const char *buffer() const;

    // Acquires ownership of text, will delete [] it.
    void	steal(char *text);

    // Points to the text without gaining ownership.  If you try to write
    // to this, you will not alter text!
    void	reference(const char *text);

    // Copies the text into our own buffer, so the source may do as it wishes.
    void	strcpy(const char *text);
    void	strcpy(BUF buf)
		{ *this = buf; }

    int		strcmp(const char *cmp) const;
    inline int	strcmp(BUF buf) const
		{ return strcmp(buf.buffer()); }

    // Matches strlen.
    int		strlen() const;

    // Returns a malloc() copy of self, just like strdup(this->buffer())
    // THis is safe to call on a temporary, unlike the above construction.
    char	*strdup() const;

    // Like strcat, but no concerns of overrun
    void	strcat(const char *text);
    void	strcat(BUF buf)
		{ strcat(buf.buffer()); }

    char	lastchar(int nthlast = 0) const;

    bool	extensionMatch(const char *ext) const;
    bool	startsWith(const char *text) const;

    void	stripExtension();

    // Returns the first line from this buffer, updating self
    // to point to the remainder.  Use isstring() to see if you are
    // done.
    BUF		extractLine();

    // Adds a single character.
    void	append(char c);

    // Our good friends.  Now with no buffer overruns.
    int		vsprintf(const char *fmt, va_list ap);
    int		sprintf(const char *fmt, ...);

    // Makes this a writeable buffer.
    void	uniquify();
    // Access underlying data.  Never use this :>
    char	*evildata();

    // Fun string manipulation functions

    // Only deals with very evil stuff like / or : embedded.
    void	 makeFileSafe();

protected:
    BUF_int	*myBuffer;
};

#endif
