/*
 *  controls.h
 *  Jumpman
 *
 *  Created by Andi McClure on 4/3/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "SDL/SDL.h"
#include "SDL/SDL_endian.h"
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <math.h>

#include "slice.h"

using namespace std;

#define CLEFT  0
#define CMIDL  1
#define CMID   2
#define CMIDR  3
#define CRIGHT 4
#define CNUM   5

extern int SCROLLMAX;
void resetScrollmax();

extern bool clickConnected;

// Obnoxious, but main.cpp uses this
extern cpVect button_verts[], scrollbutton_verts[];

inline string doubleToString(double d) {
	if (fabs(d) < 0.001) d = 0; ostringstream s; s.precision(3); s << d; return s.str();
}

class ControlBase {
public:
	string text;
	bool bg;
	unsigned int graphic;
	bool highlighted;
	cpVect p; // For convenience (saves a dereference)
	cpShape *s;
	slice *img;
	bool immortalImg; // Someone else is managing this image
	
	ControlBase(string _text = string(), bool _bg = false) {
		graphic = 0;
		bg = _bg;
		text = _text;
		p = cpvzero;
		s = NULL;
		img = NULL;
		highlighted = false;
		immortalImg = false;
	}
	virtual ~ControlBase() { if (img && !immortalImg) delete img; }

	virtual void click() {}
	virtual void wheel(int dir) {}
	virtual void disconnect(cpSpace *); // Passing in the space is a terrible idea
};

class KeyboardControl : public ControlBase
{public:
	static KeyboardControl *focus;
	virtual void key(Uint16 unicode, SDLKey key) {
		if (key == SDLK_ESCAPE) // Bad idea?
			KeyboardControl::focus = NULL;
	}
	virtual void loseFocus() { focus = NULL; }
	virtual void takeFocus() { if (focus) focus->loseFocus(); focus = this; }
	KeyboardControl(string _text = string(), bool _takeFocus = false, bool _haveBackground = true) : ControlBase(_text, _haveBackground) { if (_takeFocus) takeFocus(); }
	virtual ~KeyboardControl() { if (this == focus) focus = NULL; }
	virtual void click() { takeFocus(); }
};

class TextControl : public KeyboardControl
{
public:
	ControlBase *onEnter;
	TextControl(string _text = string(), bool _takeFocus = false, ControlBase *_onEnter = NULL) : KeyboardControl(_text, _takeFocus), onEnter(_onEnter) {}
	virtual void key(Uint16 unicode, SDLKey key);
};

class WheelControl : public TextControl
{
public:
	double is, lo, hi, by;
	virtual void changedIs() { text = doubleToString(is); } 
	WheelControl(double _text = 0, double _lo = 0, double _hi = 0, double _by = 1)
		: TextControl("", false, NULL), is(_text), lo(_lo), hi(_hi), by(_by) { changedIs(); }
	virtual void wheel(int dir);
	virtual void key(Uint16 unicode, SDLKey key);
	virtual void loseFocus();
};

class SelectControl : public WheelControl
{
public:
	vector<string> options;
	SelectControl(vector<string> &_options, int initial = 0)
		: WheelControl(initial, 0, _options.size()-1, 1), options(_options) { changedIs(); }
	virtual void changedIs() { // Note we don't update if we're outside the array
		int selected = is + 0.5;
		if (selected >= 0 && selected < options.size())
			text = options[selected];
		else
			text = "???";
	}
};

class CheckGroup;

class CheckControl : public ControlBase {
protected:
	bool byImg;
	CheckGroup *group;
public:
	CheckControl(string _text = string(), bool _byImg = false, CheckGroup *_group = NULL);
	virtual ~CheckControl();
	bool checked;
	
	virtual void click();
	virtual void justUpdatedCheck(); // Should only really use for drawing
	virtual void uncheck();
};

class CheckGroup {
public:
	CheckControl *checked;
	void check(CheckControl *me);
};

class ContainerBase {
public:
	bool committed;
	cpSpace *parent;
	cpBody *body;
	double x, y, roof;
	vector<ControlBase *> controls;
	ContainerBase(cpSpace *_parent, int where);
	virtual ~ContainerBase();
	virtual void add(ControlBase *control);
	virtual void commit();
	virtual void recommit();
};

class ColumnContainer : public ContainerBase {
public:
	virtual void commit();
	ColumnContainer(cpSpace *_parent, int where) : ContainerBase(_parent, where) {}
};

class ScrollContainer : public ContainerBase {
	ControlBase *button[2];
public:
	int scroll; // Should not usually be accessed directly
	bool haveButtons; // Likewise
	vector<ScrollContainer *> peers; // OK, this you can access directly
	ScrollContainer(cpSpace *_parent, int _where, bool _haveButtons = true);	
	virtual ~ScrollContainer() { scroll = 0; for(int c = 0; c < 2; c++) { button[c]->disconnect(parent); delete button[c]; } }
	virtual void commit();
	void scrollBy(int s);
};

class SquishContainer : public ContainerBase {
public:
	virtual void commit();
	SquishContainer(cpSpace *_parent, int where) : ContainerBase(_parent, where) {}
};
