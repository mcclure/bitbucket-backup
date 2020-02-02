#ifndef _DOS_H
#define _DOS_H

/*
 *  dos.h
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 2/23/12.
 *  Copyright 2012 Run Hello. All rights reserved.
 *
 */

#include "program.h"

struct TypeScene;

struct type_automaton : public automaton {
	TypeScene *display;
	unsigned char screen[24][40]; // NOTE: Y,X
	unsigned int  scolor[24][40]; // NOTE: Y,X // I bet this could be optional.
	void clear() {
		memset(screen, ' ', sizeof(screen));
		memset(scolor, 0xFF, sizeof(scolor));
	}
	type_automaton();
//	virtual void draw();
	String get(int x, int y);
	void set(int x, int y, String s, bool inv = false);
	void set(int x, int y, unsigned char c, bool inv = false);
	void set(int x, int y, string s, bool inv = false);
	void set_centered(int x, int y, int w, String s, bool clearfirst = true, bool inv = false);
	void set_centered(int x, int y, int w, string s, bool clearfirst = true, bool inv = false);
	void load_text(string filename);
	void save_text(string filename);
	void dump(); // Debug
	void scroll();
	void rscroll();
	Scene *displayScene();
	virtual ~type_automaton();
	
	virtual void BackOut() {
		Quit(); // Default implementation just quits.
	}
	
	virtual void insert();
	static type_automaton *singleton() { return _singleton; }
protected:
	static type_automaton *_singleton;
};

struct freetype_automaton : public type_automaton {
	int x; int y; bool autoinv; bool cursor; bool lctrl; bool rctrl;
	freetype_automaton();
	virtual void handleEvent(Event *e);
//	virtual void input(SDL_Event &event);
	void next(int dx, int dy);
//	virtual void draw();
};

#endif /* _VOXEL_LOADER_H */