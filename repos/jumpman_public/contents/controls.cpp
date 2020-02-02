/*
 *  controls.cpp
 *  Jumpman
 *
 *  Created by Andi McClure on 4/3/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include <stdlib.h>

#include "chipmunk.h"
#include "controls.h"
#include "internalfile.h"

extern double aspect;
extern double button_width, button_height;

int SCROLLMAX = 6;
void resetScrollmax() { SCROLLMAX = 6; }

slice *iChecked = NULL, *iUnchecked = NULL;
				  
				  
cpVect button_verts[] = {
					cpv(-90,-36),
					cpv(-90, 36),
					cpv( 90, 36),
					cpv( 90,-36),
};
				  
cpVect scrollbutton_verts[] = {
					cpv(-90,-18),
					cpv(-90, 18),
					cpv( 90, 18),
					cpv( 90,-18),
				  };
				  
cpVect square_verts[] = {
					cpv(-36,-36),
					cpv(-36, 36),
					cpv( 36, 36),
					cpv( 36,-36),
				  };
				  
#define GAP_HEIGHT (button_height/2)

bool clickConnected = false;

inline int imax(int a, int b) { return a > b ? a : b; }

void ControlBase::disconnect(cpSpace *parent) {
	if (s) {
		cpSpaceRemoveStaticShape(parent, s);
		delete s; // Warning, we didn't allocate this so we need to make sure nobody else is tracking it
		s = NULL;
	}
}

CheckControl::CheckControl(string _text, bool _byImg, CheckGroup *_group) : ControlBase(_text, true), byImg(_byImg), group(_group)
{
	uncheck();
}

CheckControl::~CheckControl() {
	if (checked && group)
		group->check(NULL);
}

void CheckControl::click()
{
	checked = !checked;
	justUpdatedCheck();
	if (group) {
		if (checked) {
			group->check(this);
		} else {
			group->check(NULL);
		}
	}
}

void CheckControl::justUpdatedCheck() {
	if (byImg) {
		if (NULL == iChecked) {
			char filename[FILENAMESIZE];
			
			iChecked = new slice();
			internalPath(filename, "checked.png");
			iChecked->construct(filename, false);

			iUnchecked = new slice();
			internalPath(filename, "unchecked.png");
			iUnchecked->construct(filename, false);
		}
		img = checked ? iChecked : iUnchecked;
		immortalImg = true;
	} else {
		highlighted = checked;
	}
}

void CheckControl::uncheck()
{
	checked = false;
	justUpdatedCheck();
}

void CheckGroup::check(CheckControl *me)
{
	if (checked)
		checked->uncheck();
	checked = me;
}

ContainerBase::ContainerBase(cpSpace *_parent, int where) {
	parent = _parent;
	body = cpBodyNew(INFINITY, INFINITY);
	x = (where-CMID) * (280.0+(where==CMIDL||where==CMIDR?20:0)) / aspect / 2; 
	y = 0; roof = 0; committed = false;
}

void ContainerBase::add(ControlBase *control)
{
	controls.push_back(control);
}

void WheelControl::wheel(int dir)
{
	if (dir) {
		if (dir>0)
			is += by;
		else
			is -= by;
		if (is<lo) is = lo;
		if (is>hi) is = hi;
		changedIs();
	}
}

void WheelControl::key(Uint16 unicode, SDLKey key) {
	if (key == SDLK_UP)
		wheel(1);
	else if (key == SDLK_DOWN)
		wheel(-1);
	else
		TextControl::key(unicode, key);
}

void WheelControl::loseFocus() {
	KeyboardControl::loseFocus();
	{
		double will;
		istringstream i(text); i >> will;
		if (i.rdstate() == ios::eofbit) { // STL SUCKS: "eofbit" means the string was successfully converted. "goodbit" means there was a non-numeric suffix
			is = will;
			if (is<lo) is = lo;
			if (is>hi) is = hi;
		}
	}
	changedIs();
}

ContainerBase::~ContainerBase()
{
	for(vector	<ControlBase *>::iterator b = controls.begin(); b != controls.end(); b++) {
		(*b)->disconnect(parent);
		delete (*b);
	}
	delete body;
}

void ContainerBase::recommit()
{
	for(int c = 0; c < controls.size(); c++)
		controls[c]->disconnect(parent);
	commit();
}

void ContainerBase::commit()
{
	roof = (button_height*controls.size() + GAP_HEIGHT*imax(controls.size()-1, 0)) / 2;
	for(int c = 0; c < controls.size(); c++) {
		controls[c]->p = cpv(x, roof - (button_height*c + GAP_HEIGHT*(c+1)) + y);
		//printf("# %d : roof %lf y %lf\n", controls.size(), (double)roof, (double)controls[c]->p.y);
		controls[c]->s = cpPolyShapeNew(body, 4, button_verts, controls[c]->p);
		controls[c]->s->data = controls[c];
		cpSpaceAddStaticShape(parent, controls[c]->s);
	}
}

void ColumnContainer::commit() // Kinda redundant?
{
	int size = controls.size(); size = size / 2 + size % 2;
	roof = (button_height*size + GAP_HEIGHT*imax(size-1, 0)) / 2;
	for(int c = 0; c < controls.size(); c++) {
		controls[c]->p = cpv(x + 54.0*(c%2?1:-1), roof - (button_height*(c/2) + GAP_HEIGHT*((c/2)+1)) + y);
		//printf("# %d : roof %lf y %lf\n", controls.size(), (double)roof, (double)controls[c]->p.y);
		controls[c]->s = cpPolyShapeNew(body, 4, square_verts, controls[c]->p);
		controls[c]->s->data = controls[c];
		cpSpaceAddStaticShape(parent, controls[c]->s);
	}
}

class ScrollButton : public ControlBase
{public:
	ScrollContainer *parent;
	bool down;
	ScrollButton(ScrollContainer *_parent, bool _down) : ControlBase(_down?"v":"^", true), parent(_parent), down(_down) {}
	virtual void click() {
		int dir = down?1:-1;
		SDLMod mods = SDL_GetModState();
		if (mods & KMOD_LCTRL || mods & KMOD_RCTRL)
			dir *= SCROLLMAX;
		parent->scrollBy(dir);
	}
};

ScrollContainer::ScrollContainer(cpSpace *_parent, int _where, bool _haveButtons) : ContainerBase(_parent, _where), scroll(0), haveButtons(_haveButtons) {
	roof = ((button_height+1)*SCROLLMAX + GAP_HEIGHT*(SCROLLMAX-1)) / 2 + GAP_HEIGHT;
	for(int c = 0; c < 2; c++) {
		double roof2 = roof - c*((button_height*(SCROLLMAX+1) + GAP_HEIGHT*(SCROLLMAX-1)));
		button[c] = new ScrollButton(this, c>0);
		button[c]->p = cpv(x, roof2 + y);
//			printf("SCROLL# %d : roof %lf y %lf\n", controls.size(), (double)roof, (double)button[c]->p.y);
	}
}

void ScrollContainer::commit()
{
	for(int c = 0; c < 2; c++) // We could do this in scrollBy but let's be paranoid.
		button[c]->disconnect(parent);
		
	if (controls.size() <= SCROLLMAX+1)
		ContainerBase::commit();
	else {
		roof = (button_height*SCROLLMAX + GAP_HEIGHT*(SCROLLMAX-1)) / 2;
		for(int c = scroll; c < controls.size() && c < scroll+SCROLLMAX; c++) {
			controls[c]->p = cpv(x, roof - (button_height*(c-scroll) + GAP_HEIGHT*((c-scroll)+1)) + y);
			controls[c]->s = cpPolyShapeNew(body, 4, button_verts, controls[c]->p);
			controls[c]->s->data = controls[c];
			cpSpaceAddStaticShape(parent, controls[c]->s);
		}
		if (haveButtons && scroll > 0) {
			button[0]->s = cpPolyShapeNew(body, 4, scrollbutton_verts, button[0]->p);
			button[0]->s->data = button[0];
			cpSpaceAddStaticShape(parent, button[0]->s);
		}
		if (haveButtons && scroll + SCROLLMAX < controls.size()) {
			button[1]->s = cpPolyShapeNew(body, 4, scrollbutton_verts, button[1]->p);
			button[1]->s->data = button[1];
			cpSpaceAddStaticShape(parent, button[1]->s);
		}
	}
}

void ScrollContainer::scrollBy(int s) {
	scroll += s;
	
	if (scroll + SCROLLMAX > controls.size())
		scroll = controls.size() - SCROLLMAX;
	if (scroll < 0)
		scroll = 0;

	recommit();
	
	for(int c = 0; c < peers.size(); c++) { // Though usually there won't be any.
		peers[c]->scrollBy(s);
	}
}

KeyboardControl *KeyboardControl::focus = 0;

void TextControl::key(Uint16 unicode, SDLKey key) {
//	printf("Um um u %c [%d] k %c [%d]\n", (char)unicode, (int)unicode, (char)key, (int)key);
	if (key == SDLK_ESCAPE) // Bad idea?
		KeyboardControl::focus = NULL;
	else if (key == SDLK_BACKSPACE) {
		if (text.size() > 0)
			text.erase(text.size()-1);
	} else if (key == SDLK_RETURN) {
		if (onEnter) {
			onEnter->click();
			clickConnected = true;
		} else
			loseFocus();
	} else if (unicode >= 32 && unicode <= 126)
		text += (char)unicode;
}

// Just for controls screen

extern float floatHeight;
void SquishContainer::commit()
{
	float squishHeight = floatHeight*2;
	cpVect squish_verts[] = {
					cpv(-90*2,-floatHeight),
					cpv(-90*2, floatHeight),
					cpv( 90*2, floatHeight),
					cpv( 90*2,-floatHeight),
				  };
				  
	roof = (squishHeight*controls.size()) / 2;
	for(int c = 0; c < controls.size(); c++) {
		controls[c]->p = cpv(x, roof - (squishHeight*c) + y);
		//printf("# %d : roof %lf y %lf\n", controls.size(), (double)roof, (double)controls[c]->p.y);
		controls[c]->s = cpPolyShapeNew(body, 4, squish_verts, controls[c]->p);
		controls[c]->s->data = controls[c];
		cpSpaceAddStaticShape(parent, controls[c]->s);
	}
}