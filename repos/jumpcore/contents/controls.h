// ControlBase headers

// File contains code from Jumpcore; notice applies to that code only:
/* Copyright (C) 2008-2010 Andi McClure
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _CONTROLS_H
#define _CONTROLS_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <math.h>

#include "slice.h"

#define CLEFT  -0.75
#define CMIDL  -0.375
#define CMID   0
#define CMIDR  0.375
#define CRIGHT 0.75

extern int SCROLLMAX;
void resetScrollmax();

extern bool clickConnected;

// Obnoxious, but main.cpp uses this
extern cpVect button_verts[], scrollbutton_verts[];
extern float button_height;
extern float button_width;

inline string doubleToString(double d) {
	if (fabs(d) < 0.001) d = 0; ostringstream s; s.precision(3); s << d; return s.str();
}

// Parent class for all Controls. Has the full ability to display however any other control does, but lacks interactivity.
// If you want to make something like a "button" you basically want to subclass ControlBase and override Click or Wheel.
class ControlBase {
public:
	string text;
	bool bg; // "Background" enable. I suggest setting this true for anything interactive
	bool highlighted; // Background will be colored funny-- implies keyboard focus
	cpVect p; // For convenience (saves a dereference)
	cpShape *s;
	texture_slice *img; // Image to use as a button "icon"
	bool immortalImg; // Someone else is managing this image-- UNLESS this is true, img will be deleted when this object is
	
	ControlBase(string _text = string(), bool _bg = false) {
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
	virtual void wheel(int dir) {} // wheel can be 1 or -1.
	virtual void disconnect(cpSpace *); // Passing in the space is a terrible idea... removes the button from the space
};

// A ControlBase item that knows how to accept keyboard events.
class KeyboardControl : public ControlBase
{public:
	// At any one time there is at most one designated KeyboardControl which has "focus"; all keydown events are directed to
	// this control (by Main). The default click() implementation for KeybaordControl takes control of the focus:
	static KeyboardControl *focus;
	virtual void loseFocus() { focus = NULL; } // When one keyboardControl takes focus, it calls loseFocus() on the current focus first. Overriding this function can be useful.
	virtual void takeFocus() { if (focus) focus->loseFocus(); focus = this; }
	virtual void key(Uint16 unicode, SDL_Keycode key) {
#ifdef TARGET_DESKTOP
		if (key == SDLK_ESCAPE) // Bad idea to use "ESC" for both keyboard widgets and "quit program"?
			loseFocus();
#endif
	}
	KeyboardControl(string _text = string(), bool _takeFocus = false, bool _haveBackground = true) : ControlBase(_text, _haveBackground) { if (_takeFocus) takeFocus(); } // If "takefocus" is true then grab the focus as soon as you're created
	virtual ~KeyboardControl() { if (this == focus) focus = NULL; }
	virtual void click() { takeFocus(); }
};

// A control that acts like a text entry box.
class TextControl : public KeyboardControl
{
public:
	ControlBase *onEnter; // If this is set for a TextControl, then hitting the "Enter" key will be as if you clicked on the onEnter control. You can set onEnter to be for example the next box in a form, or the "OK" button.
	TextControl(string _text = string(), bool _takeFocus = false, ControlBase *_onEnter = NULL) : KeyboardControl(_text, _takeFocus), onEnter(_onEnter) {}
	virtual void key(Uint16 unicode, SDL_Keycode key);
};

// A TextControl that holds a numeric value, and can be "scrolled" by using the mousewheel or arrow keys. You can also enter
// a value manually but it will check whatever you entered to make sure it's a number within bounds.
class WheelControl : public TextControl
{
public:
	double is, lo, hi, by;
	virtual void changedIs() { text = doubleToString(is); } // When the wheel or arrow keys change the value of "is", this method is called to update the displayed value.
	WheelControl(double _text = 0, double _lo = 0, double _hi = 0, double _by = 1)
		: TextControl("", false, NULL), is(_text), lo(_lo), hi(_hi), by(_by) { changedIs(); }
	virtual void wheel(int dir);
	virtual void key(Uint16 unicode, SDL_Keycode key);
	virtual void loseFocus();
};

// A WheelControl that acts like a "Menu"-- it rounds "is" to the nearest integer and displays a string from a list with that index.
class SelectControl : public WheelControl
{
public:
	vector<string> options; // List of strings to display
	bool up;			// If true, scroll up to progress to the next menu item. If false, scroll down.
	SelectControl(vector<string> &_options, int initial = 0, bool _up = true)
		: WheelControl(initial, 0, _options.size()-1, 1), options(_options), up(_up) { changedIs(); }
	virtual void changedIs() { // Note we don't update if we're outside the array
		int selected = is + 0.5;
		if (selected >= 0 && selected < options.size())
			text = options[selected];
		else
			text = "???";
	}
	virtual void wheel(int dir) { if (!up) dir = -dir; WheelControl::wheel(dir); }
};

class CheckGroup;

// A toggle button or check box.
class CheckControl : public ControlBase {
protected:
	bool byImg; // If true, the check/uncheck behavior is done by altering an image (i.e. it looks like a checkbox). If false, the check/uncheck behavior is done by toggling the highlight (i.e. it looks like a toggle button).
	CheckGroup *group;
public:
	CheckControl(string _text = string(), bool _byImg = false, CheckGroup *_group = NULL);
	virtual ~CheckControl();
	bool checked; // Current check value
	
	virtual void click();
	virtual void justUpdatedCheck(); // Should only really use for drawing
	virtual void uncheck(); // Basically means "checkgroup just took away your check status"
};

// Tracks a number of CheckControls such that (a) it enforces only one of the controls added to it may be checked at a time and 
// (b) at any time you can find out which checkbox in the group is selected, if any.
class CheckGroup {
public:
	CheckControl *checked;
	void setCheck(CheckControl *me);
};


// Parent class for all containers
class ContainerBase {
public:
	bool committed;		// Has commit been called?
	cpSpace *parent;
	cpBody *body;
	double x, y, roof;	// X and Y offsets for the container-- only take effect when commit() or recommit() are called. Don't touch "roof"
	vector<ControlBase *> controls;		// All controls "in" this container
	
	// When you create a Container you have to give it a cpSpace for its buttons to "live" in and also an x coordinate
	// (on the scale where the left side of the screen is -1 and the right side of the screen is 1) to draw the column
	// of controls at. I recommend using the workingUiSpace() value already in Jumpcore for basically everything, and
	// the CMIDL, CMIDR, CRIGHT etc constants for the x value.
	ContainerBase(cpSpace *_parent, double _x);
	virtual ~ContainerBase();
	virtual void add(ControlBase *control); // Registers a control with a container (though nothing appears until you call commit)
	virtual void commit();	// Gives each control a cpShape and a screen position.
	virtual void recommit();	// Unhooks all controls, then commits
};

// Like ContainerBase, only instead of a column of big controls it displays two parallel columns of smaller controls-- 
// since each such control is about the size of an icon this makes a good starting point for something like a tool palette.
class ColumnContainer : public ContainerBase {
public:
	virtual void commit();
	ColumnContainer(cpSpace *_parent, double _x) : ContainerBase(_parent, _x) {}
};

// 	Like ContainerBase, only it displays a maximum of SCROLLMAX buttons at a time and (if needed) displays up to two 
// "scrollbar" buttons which provide access to the additional controls.
class ScrollContainer : public ContainerBase {
	ControlBase *button[2];
public:
	int scroll; // Should not usually be accessed directly
	bool haveButtons; // Likewise
	// When the container scrolls, each of the containers in "peers" will be scrolled with it.
	// This is so that one scrollbar can scroll multiple columns of controls:
	vector<ScrollContainer *> peers; // OK, this you can access directly
	ScrollContainer(cpSpace *_parent, double _x, bool _haveButtons = true);	// The additional haveButtons argument determines whether the scroll buttons are ever visible.
	virtual ~ScrollContainer() { scroll = 0; for(int c = 0; c < 2; c++) { button[c]->disconnect(parent); delete button[c]; } }
	virtual void commit();
	void scrollBy(int s); // Scrolls the visible controls up or down by "s" spaces.
};

void clicked(cpShape *shape, void *data);
void wheeled(cpShape *shape, void *data);

#endif
