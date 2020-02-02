// ControlBase code

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

#include <stdlib.h>
#include <math.h>

#include "kludge.h"
#include "chipmunk.h"
#include "controls.h"
#include "internalfile.h"

inline int imax(int a, int b) { return ::max(a,b); } // Avoids some template issues.

extern double aspect;

int SCROLLMAX = 6;
void resetScrollmax() { SCROLLMAX = 6; }

texture_slice *iChecked = NULL, *iUnchecked = NULL;
				  
float button_height = 40/200.0; // FIXME
float button_width = 90/200.0;

cpVect button_verts[] = {
					cpv(-button_width/2,-button_height/2),
					cpv(-button_width/2, button_height/2),
					cpv( button_width/2, button_height/2),
					cpv( button_width/2,-button_height/2),
};
				  
cpVect scrollbutton_verts[] = {
					cpv(-button_width/2,-button_height/4),
					cpv(-button_width/2, button_height/4),
					cpv( button_width/2, button_height/4),
					cpv( button_width/2,-button_height/4),
				  };
				  
cpVect square_verts[] = {
					cpv(-button_height/2,-button_height/2),
					cpv(-button_height/2, button_height/2),
					cpv( button_height/2, button_height/2),
					cpv( button_height/2,-button_height/2),
				  };
				  
#define GAP_HEIGHT (16/200.0)

bool clickConnected = false;

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
		group->setCheck(NULL);
}

void CheckControl::click()
{
	checked = !checked;
	justUpdatedCheck();
	if (group) {
		if (checked) {
			group->setCheck(this);
		} else {
			group->setCheck(NULL);
		}
	}
}

void CheckControl::justUpdatedCheck() {
	if (byImg) {
		if (NULL == iChecked) {
			char filename[FILENAMESIZE];
			
			iChecked = new texture_slice();
			internalPath(filename, "checked.png");
			iChecked->load(filename);

			iUnchecked = new texture_slice();
			internalPath(filename, "unchecked.png");
			iUnchecked->load(filename);
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

void CheckGroup::setCheck(CheckControl *me)
{
	if (checked)
		checked->uncheck();
	checked = me;
}

ContainerBase::ContainerBase(cpSpace *_parent, double _x) {
	parent = _parent;
	body = cpBodyNew(INFINITY, INFINITY);
	x = _x / aspect; 
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

void WheelControl::key(Uint16 unicode, SDL_Keycode key) {
#ifdef TARGET_DESKTOP
	if (key == SDLK_UP)
		wheel(1);
	else if (key == SDLK_DOWN)
		wheel(-1);
	else
#endif
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
	roof = (button_height*controls.size() + GAP_HEIGHT*imax(controls.size()-1, 0ul)) / 2; // The sum of the heights of all the "buttons" plus all the gaps between buttons.
	for(int c = 0; c < controls.size(); c++) {
		controls[c]->p = cpv(x, roof - (button_height*(c+0.5) + GAP_HEIGHT*c) + y); // + 0.5 because the button shape is centered.
//		printf("# %d : roof %lf y %lf\n", controls.size(), (double)roof, (double)controls[c]->p.y);
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
		controls[c]->p = cpv(x + button_height*0.6*(c%2?1:-1), roof - (button_height*(c/2 + 0.5) + GAP_HEIGHT*(c/2)) + y);
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
#ifdef TARGET_DESKTOP
		SDL_Keymod mods = SDL_GetModState();
		if (mods & KMOD_LCTRL || mods & KMOD_RCTRL)
			dir *= SCROLLMAX;
#endif
		parent->scrollBy(dir);
	}
};

ScrollContainer::ScrollContainer(cpSpace *_parent, double _x, bool _haveButtons) : ContainerBase(_parent, _x), scroll(0), haveButtons(_haveButtons) {
	roof = (button_height*SCROLLMAX + GAP_HEIGHT*(SCROLLMAX-1)) / 2; // Position of 'up' scrollbutton
	for(int c = 0; c < 2; c++) {
		double roof2 = (roof + GAP_HEIGHT/2 + button_height/4) * (c?-1:1); //c*((button_height*(SCROLLMAX+1) + GAP_HEIGHT*(SCROLLMAX-1))); // Distance from 'up' scrollbutton to 'down' scrollbutton
		button[c] = new ScrollButton(this, c>0);
		button[c]->p = cpv(x, roof2 + y);
//		printf("SCROLL# %d : roof %lf roof2 %lf y %lf\n", controls.size(), (double)roof, (double)roof2, (double)button[c]->p.y);
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
			controls[c]->p = cpv(x, roof - (button_height*(c-scroll+0.5) + GAP_HEIGHT*(c-scroll)) + y);
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

void TextControl::key(Uint16 unicode, SDL_Keycode key) {
//	printf("Um um u %c [%d] k %c [%d]\n", (char)unicode, (int)unicode, (char)key, (int)key);
#ifdef TARGET_DESKTOP
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
	} else
#endif
    if (unicode >= 32 && unicode <= 126) // FIXME: What about non-english chars?
		text += (char)unicode;
}

// Use with shape queries:

void clicked(cpShape *shape, void *data) // Event forwarder for ControlBase
{
	if (!shape->data || ((ControlBase *)shape->data)->bg || ((ControlBase *)shape->data)->text.size()) // Clicks only make sense for things with backgrounds.
		clickConnected = true;
	if (shape->data)
		((ControlBase *)shape->data)->click();
}

// "Data" is defined by convention to be 1 or -1 -- raw not pointer value
void wheeled(cpShape *shape, void *data) // Event forwarder for ControlBase
{
	clickConnected = true;
	if (shape->data)
		((ControlBase *)shape->data)->wheel((uintptr_t)data);
}