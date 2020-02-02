/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        panel.cpp ( Live Once Library, C++ )
 *
 * COMMENTS:
 */

#include "panel.h"

#include "rand.h"
#include "gfxengine.h"

#include <ctype.h>
#include <stdlib.h>
#include <memory.h>

PANEL::PANEL(int w, int h, bool recordhistory)
{
    myX = 0;
    myY = 0;
    myW = 0;
    myH = 0;

    myLines = 0;
    myAttrMap = 0;

    myIndent = 0;
    myRightMargin = 1;
    myRecordHistory = recordhistory;

    myBorder = false;
    myTransparent = false;
    
    setAttr(ATTR_NORMAL);

    resize(w, h);

    myCurLine = 0;
    myCurPos = 0;
}

PANEL::~PANEL()
{
    deleteData();
}

void
PANEL::deleteData()
{
    int		i;

    for (i = 0; i < myH; i++)
	delete [] myLines[i];
    delete [] myLines;
    myLines = 0;
    for (i = 0; i < myH; i++)
	delete [] myAttrMap[i];
    delete [] myAttrMap;
    myAttrMap = 0;

    clearHistory();
}

void
PANEL::resize(int w, int h)
{
    AUTOLOCK	a(myLock);
    int		i, j, cw;
    char	**lines;
    ATTR_NAMES	**attr;

    cw = MIN(w, myW);

    lines = new char *[h];
    for (i = 0; i < h; i++)
    {
	lines[i] = new char [w+10];
	lines[i][0] = '\0';

	if (i < myH)
	{
	    // Copy old line
	    for (j = 0; j < cw; j++)
	    {
		lines[i][j] = myLines[i][j];
	    }
	    // Ensure null terminated.
	    lines[i][cw] = '\0';
	}
    }

    attr = new ATTR_NAMES *[h];
    for (i = 0; i < h; i++)
    {
	attr[i] = new ATTR_NAMES [w];
	for (j = 0; j < w; j++)
	    attr[i][j] = myTextAttr;
	if (i < myH)
	{
	    // Copy old line
	    for (j = 0; j < cw; j++)
	    {
		attr[i][j] = myAttrMap[i][j];
	    }
	}
    }

    deleteData();

    myW = w;    
    myH = h;

    myLines = lines;
    myAttrMap = attr;

    // Clamp our cursor
    if (myCurPos >= myW)
	myCurPos = myW-1;
    if (myCurLine > myH)
	myCurLine = myH-1;
}

void
PANEL::setBorder(bool enable, u8 sym, ATTR_NAMES attr)
{
    myBorder = enable;
    myBorderSym = sym;
    myBorderAttr = attr;
}

void
PANEL::clearHistory()
{
    AUTOLOCK	a(myLock);
    int		i;

    for (i = 0; i < myHistory.entries(); i++)
	delete [] myHistory(i);
    myHistory.clear();
}

int
PANEL::getHistoryLine()
{
    AUTOLOCK	a(myLock);
    return myCurLine + myHistory.entries();
}

void
PANEL::scrollToHistoryLine(int line)
{
    AUTOLOCK	a(myLock);
    // Determine how many lines of history we need to scroll into
    // our window...
    int		netnew;

    netnew = getHistoryLine() - line;
    if (netnew <= 0)
    {
	// Either trying to go forward in time or a no-op
	return;
    }

    while (netnew--)
    {
	scrollDown();
    }
}

void
PANEL::setCurrentLine(const char *text)
{
    AUTOLOCK	a(myLock);
    // Clear out the current line.
    myLines[myCurLine][0] = '\0';
    myCurPos = 0;

    // And append normally
    appendText(text);
}

void
PANEL::appendText(const char *text, int linecount)
{
    if (!text)
	return;

    AUTOLOCK	a(myLock);
    // Start copying text into myLines[myCurLine][myCurPos], doing
    // wordwrap at myW.
    const char		*start;
    char		*dst;
    ATTR_NAMES		*dstattr;
    int			 dstpos;

    start = text;
    dst = myLines[myCurLine];
    dstattr = myAttrMap[myCurLine];
    dstpos = myCurPos;

    while (*text)
    {
	// Check for control characters.
	if (*text == '\n')
	{
	    dst[dstpos] = '\0';
	    linecount++;
	    newLine();
	    if (myH <= 2)
	    {
		// Cannot scroll with prompt!
		linecount = 0;
	    }
	    else if (linecount >= myH-2) 
	    {
		awaitKey();
		linecount = 0;
	    }
	    appendText(text+1, linecount);
	    return;
	}
	
	// Check to see if we have hit myW.
	// Note we always scroll one character early!  This is more
	// visually pleasing than filling to the boundrary.
	if (dstpos >= myW - myRightMargin)
	{
	    // If this is a space, eat all succeeding spaces.
	    if (ISSPACE(*text))
	    {
	    }
	    else
	    {
		const char *fallback;

		fallback = text;
		
		// Not a space, we want to roll back until the next space.
		while (text > start)
		{
		    text--;
		    dstpos--;
		    if (ISSPACE(*text))
			break;
		}
		if (text == start)
		{
		    // Oops, no breakpoint! Hyphenate arbitrarily.
		    // Restore ourselves to our fallback position.
		    // The exception is if we did not start with
		    // an empty line!  In that case, a new line
		    // may suffice.
		    if (dstpos == myIndent)
		    {
			dstpos += (int)(fallback - text);
			text = fallback;
		    }
		}
	    }

	    // Write in a null and new line.
	    dst[dstpos] = '\0';

	    // Go forward in text removing all spaces.
	    while (*text && ISSPACE(*text))
	    {
		text++;
	    }
	    linecount++;
	    newLine();
	    if (myH <= 2)
	    {
		// Cannot scroll with prompt!
		linecount = 0;
	    }
	    else if (linecount >= myH-2) 
	    {
		awaitKey();
		linecount = 0;
	    }
	    // Append the remainder.
	    appendText(text, linecount);
	    return;
	}

	// All good for normal addition.
	dst[dstpos] = *text;
	dst[dstpos+1] = '\0';
	dstattr[dstpos] = myTextAttr;

	dstpos++;
	text++;
	myCurPos = dstpos;
    }
}

void redrawWorld();

void
PANEL::awaitKey()
{
    appendText("-- MORE --\n");
    while (!gfx_getKey(false))
    {
	redrawWorld();
    }
}

void
PANEL::getString(const char *prompt, char *buf, int maxlen)
{
    appendText(prompt);

    redraw();

    // Clamp our string to our panel width as we don't handle scrolling.
    if (maxlen + myCurPos > myW)
    {
	maxlen = myW - myCurPos;
    }
    
    gfx_getString(myCurPos + myX, myCurLine + myY, myAttrMap[myCurLine][myCurPos], buf, maxlen);
    // Ensure the buffer is inside our panel's memory!
    appendText(buf);
    // Always do a new line!
    newLine();
}

void
PANEL::setAttr(ATTR_NAMES attr)
{
    fillRect(0, 0, myW, myH, attr);
    myTextAttr = attr;
}

void
PANEL::fillRect(int x, int y, int w, int h, ATTR_NAMES attr)
{
    AUTOLOCK	a(myLock);
    int		i, j;

    // Clip...
    if (x < 0)
    {
	w += x;
	x = 0;
    }
    if (y < 0)
    {
	h += y;
	y = 0;
    }

    for (i = y; i < y + h; i++)
    {
	if (i >= myH)
	    break;
	for (j = x; j < x + w; j++)
	{
	    if (j >= myW)
		break;

	    myAttrMap[i][j] = attr;
	}
    }
}

void
PANEL::newLine()
{
    AUTOLOCK	a(myLock);
    myCurLine++;
    if (myCurLine == myH)
    {
	scrollUp();
    }
    myCurPos = 0;
    int		i;
    for (i = 0; i < myIndent; i++)
	appendText(" ");
}

void
PANEL::clear()
{
    AUTOLOCK	a(myLock);
    int		i;

    for (i = 0; i < myH; i++)
    {
	myLines[i][0] = '\0';
    }
    myCurPos = 0;
    myCurLine = 0;

    // Important we start with our indent!
    for (i = 0; i < myIndent; i++)
	appendText(" ");

    setAttr(myTextAttr);
}

void
PANEL::move(int x, int y)
{
    myX = x;
    myY = y;
}

void
PANEL::redraw()
{
    AUTOLOCK	a(myLock);
    int		x, y;

    for (y = 0; y < myH; y++)
    {
	if (y + myY >= 0 && y + myY < SCR_HEIGHT)
	{
	    for (x = 0; x < myW; x++)
	    {
		// Hit end of line.
		if (!myLines[y][x])
		    break;

		if (x + myX < 0 || x + myX >= SCR_WIDTH)
		    continue;
		gfx_printchar(x + myX, y + myY, myLines[y][x], myAttrMap[y][x]);
	    }

	    // Pad with spaces.
	    if (!isTransparent())
	    {
		for (; x < myW; x++)
		{
		    if (x + myX < 0 || x + myX >= SCR_WIDTH)
			continue;
		    gfx_printchar(x + myX, y + myY, ' ', myAttrMap[y][x]);
		}
	    }
	}
    }

    if (myBorder)
    {
	for (y = -1; y < myH+1; y++)
	{
	    if (y + myY < 0 || y + myY >= SCR_HEIGHT)
		continue;

	    if (myX-1 > 0 && myX-1 < SCR_WIDTH)
		gfx_printchar(myX-1, y + myY, myBorderSym, myBorderAttr);
	    if (myX+myW > 0 && myX+myW < SCR_WIDTH)
		gfx_printchar(myX+myW, y + myY, myBorderSym, myBorderAttr);
	}
	for (x = -1; x < myW+1; x++)
	{
	    if (x + myX < 0 || x + myX >= SCR_WIDTH)
		continue;

	    if (myY-1 > 0 && myY-1 < SCR_HEIGHT)
		gfx_printchar(x + myX, myY - 1, myBorderSym, myBorderAttr);
	    if (myY+myH > 0 && myY+myH < SCR_WIDTH)
		gfx_printchar(x + myX, myY+myH, myBorderSym, myBorderAttr);
	}
    }
}

void
PANEL::erase()
{
    AUTOLOCK	a(myLock);
    int		x, y;

    for (y = 0; y < myH; y++)
    {
	for (x = 0; x < myW; x++)
	{
	    gfx_printchar(x + myX, y + myY, ' ', ATTR_NORMAL);
	}
    }

    if (myBorder)
    {
	for (y = -1; y < myH+1; y++)
	{
	    if (y + myY < 0 || y + myY >= SCR_HEIGHT)
		continue;

	    if (myX-1 > 0 && myX-1 < SCR_WIDTH)
		gfx_printchar(myX-1, y + myY, ' ', ATTR_NORMAL);
	    if (myX+myW > 0 && myX+myW < SCR_WIDTH)
		gfx_printchar(myX+myW, y + myY, ' ', ATTR_NORMAL);
	}
	for (x = -1; x < myW+1; x++)
	{
	    if (x + myX < 0 || x + myX >= SCR_WIDTH)
		continue;

	    if (myY-1 > 0 && myY-1 < SCR_HEIGHT)
		gfx_printchar(x + myX, myY - 1, ' ', ATTR_NORMAL);
	    if (myY+myH > 0 && myY+myH < SCR_WIDTH)
		gfx_printchar(x + myX, myY+myH, ' ', ATTR_NORMAL);
	}
    }
}

void
PANEL::setIndent(int indent)
{
    myIndent = indent;
}

void
PANEL::setRigthMargin(int margin)
{
    myRightMargin = margin;
}

void
PANEL::shrinkToFit()
{
    if (myCurLine < myH)
    {
	int		loss = (myH - myCurLine) / 2;
	resize(myW, myCurLine);
	move(getX(), getY() + loss);
    }
}

void
PANEL::scrollUp()
{
    AUTOLOCK	a(myLock);
    int		x, y;

    if (myRecordHistory)
    {
	char		*line;

	// It is a sign of an incompetent programmer to have an oversized
	// overflow like this.  Either you know the behaviour, so can
	// have an exact bonus, or you don't, so can't really give any
	// bonus.
	line = new char [myW+10];
	memcpy(line, myLines[0], myW+1);
	myHistory.append(line);
    }

    for (y = 1; y < myH; y++)
    {
	memcpy(myLines[y-1], myLines[y], myW+1);
	memcpy(myAttrMap[y-1], myAttrMap[y], myW * sizeof(ATTR_NAMES));
    }
    myLines[myH-1][0] = '\0';
    for (x = 0; x < myW; x++)
    {
	myAttrMap[myH-1][x] = myTextAttr;
    }


    myCurLine--;
    if (myCurLine < 0)
	myCurLine = 0;
}

void
PANEL::scrollDown()
{
    AUTOLOCK	a(myLock);
    int		x, y;

    for (y = myH-1; y >= 1; y--)
    {
	memcpy(myLines[y], myLines[y-1], myW+1);
	memcpy(myAttrMap[y], myAttrMap[y-1], myW * sizeof(ATTR_NAMES));
    }
    for (x = 0; x < myW; x++)
    {
	myAttrMap[0][x] = myTextAttr;
    }

    // Pull out the history...
    if (myHistory.entries())
    {
	memcpy(myLines[0], myHistory.top(), myW+1);
	delete [] myHistory.pop();
    }
    else
	myLines[y][0] = '\0';

    myCurLine++;
    if (myCurLine > myH-1)
	myCurLine = myH-1;
}
