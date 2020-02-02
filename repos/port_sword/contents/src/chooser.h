/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        chooser.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 *	Defines a text panel that you can select an entry from.
 *	Sizes itself to fit, you specify how centering should
 *	work.
 */

#ifndef __chooser__
#define __chooser__

#include "glbdef.h"
#include "ptrlist.h"
#include "buf.h"

class CHOOSER
{
public:
	     CHOOSER();
    virtual ~CHOOSER();

    enum Justify
    {
	JUSTMIN = 0,		// left/top
	JUSTLEFT = 0,
	JUSTTOP = 0,
	JUSTCENTER = 1,
	JUSTMAX = 2,			// right/bot
	JUSTRIGHT = 2,
	JUSTBOTTOM =2,
    };

    void	clear();

    // Returns the # of this choice, 0...nchoice-1
    int		appendChoice(const char *choice, int percent = 0);
    int		appendChoice(BUF buf, int percent = 0)
		{ return appendChoice(buf.buffer(), percent); }

    void	setPrequel(const char *text);
    void	setPrequel(BUF buf)
		{ setPrequel(buf.buffer()); }

    // Current choice.
    int		getChoice() const;
    // Clamps to valid range.
    void	setChoice(int choice);
    void	setNoChoice();

    int		entries() const { return myChoices.entries(); }

    // Returns true if we eat the key and sets the key to 0 in  that
    // case.
    bool	processKey(int &key);

    void	redraw() const;

    void	move(int x, int y, Justify centerx = JUSTCENTER, Justify centery = JUSTCENTER);
    void	setBorder(bool enable, u8 sym = ' ', ATTR_NAMES attr = ATTR_NONE);

    void	setAttr(ATTR_NAMES textattr, ATTR_NAMES chosenattr, ATTR_NAMES percentattr);
    void	setPrequelAttr(ATTR_NAMES prequelattr) { myPrequelAttr = prequelattr; }
    void	setTextAttr(ATTR_NAMES textattr) { myTextAttr = textattr; }
    void	setIndent(int indent);
    void	setRigthMargin(int margin);	// And so the yyp continues.
    void	setMinW(int minw) { myMinW = minw; }

private:
    int		width() const { return myW; };
    int		height() const { return myH; };
    // Always returns top left of text area.
    int		getX() const;
    int		getY() const;

    int		myIntX, myIntY, myW, myH;
    int		myMinW;
    Justify	myCenterX, myCenterY;
    int		myIndent, myRightMargin;

    bool	 myBorder;
    ATTR_NAMES	 myBorderAttr;
    u8		 myBorderSym;

    ATTR_NAMES	 myTextAttr, myChosenAttr, myPercentAttr, myPrequelAttr;

    int		myChoice;
    PTRLIST<char *>	myPrequel;
    PTRLIST<char *>	myChoices;
    PTRLIST<int>	myPercent;
    PTRLIST<ATTR_NAMES>	myAttr;
};

#endif


