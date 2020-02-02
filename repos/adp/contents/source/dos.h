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

#define DOS_LINE_ROWS 24
#define DOS_LINE_COLS 40
//#define DOS_GFX_ROWS 192
//#define DOS_GFX_COLS 280
#define DOS_BLANK ' '

struct TypeScene;

struct type_automaton : public automaton {
	TypeScene *display;
	unsigned char POLYIGNORE screen[DOS_LINE_ROWS][DOS_LINE_COLS]; // NOTE: Y,X
	unsigned int  POLYIGNORE scolor[DOS_LINE_ROWS][DOS_LINE_COLS]; // NOTE: Y,X // I bet this could be optional.
	bool screen_dirty, offset_dirty;
	int offx; int offy;
	void clear() {
		memset(screen, DOS_BLANK, sizeof(screen));
		memset(scolor, 0xFF, sizeof(scolor));
		screen_dirty = true;
	}
	type_automaton();
	
	// Text methods
	String get(int x, int y);
	void set(int x, int y, String s, bool inv = false);
	void set(int x, int y, unsigned char c, bool inv = false);
	void set(int x, int y, string s, bool inv = false);
	void set_centered(int x, int y, int w, String s, bool clearfirst = true, bool inv = false);
	void set_centered(int x, int y, int w, string s, bool clearfirst = true, bool inv = false);
	void load_text(String filename);
	void load_text(string filename);
	void save_text(String filename);
	void save_text(string filename);
	void dump(); // Debug
	void scroll();
	void rscroll();
	Scene *displayScene();
	Number factor();

	// Misc
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
	virtual ~freetype_automaton();
	virtual void handleEvent(Event *e);
//	virtual void input(SDL_Event &event);
	void next(int dx, int dy);
//	virtual void draw();
};

#endif /* _VOXEL_LOADER_H */