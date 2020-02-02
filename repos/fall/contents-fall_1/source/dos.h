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

#define DOS_LINE_ROWS 24
#define DOS_LINE_COLS 40
#define DOS_GFX_ROWS 192
#define DOS_GFX_COLS 280
#define DOS_BLANK ' '

struct gfxcontainer {
	uint32_t POLYIGNORE gfx[DOS_GFX_ROWS][DOS_GFX_COLS]; // NOTE: Y,X
	bool gfx_dirty;
	void pxclear() {
		memset(gfx, 0, sizeof(gfx));
		gfx_dirty = true;
	}
	void pxset(int x, int y, bool on);
	void pxfill(unsigned int v, int _dstx = 0, int _dsty = 0, int dy = -1, int dx = -1);
	void pxcopy(const gfxcontainer *src, int _dstx = 0, int _dsty = 0, int _srcx = 0, int _srcy = 0, int _dx = -1, int dy = -1);
	void pxcopy_invert(const gfxcontainer *src, int _dstx = 0, int _dsty = 0, int _srcx = 0, int _srcy = 0, int _dx = -1, int dy = -1);
	void pxcopy_threshold(const gfxcontainer *src, Number threshold, int _dstx = 0, int _dsty = 0, int _srcx = 0, int _srcy = 0, int _dx = -1, int dy = -1);
	void pxswap(gfxcontainer *src, int _dstx = 0, int _dsty = 0, int _srcx = 0, int _srcy = 0, int _dx = -1, int dy = -1);
	void load_image(String filename) { load_image(filename.getSTLString()); }
	void load_image(string filename);
	void save_image(string filename);
	gfxcontainer();
};

struct type_automaton : public automaton, public gfxcontainer {
	TypeScene *display;
	unsigned char POLYIGNORE screen[DOS_LINE_ROWS][DOS_LINE_COLS]; // NOTE: Y,X
	unsigned int  POLYIGNORE scolor[DOS_LINE_ROWS][DOS_LINE_COLS]; // NOTE: Y,X // I bet this could be optional.
	bool screen_dirty;
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
	Number factor();

	// Misc
	gfxcontainer *toGfx() { return this; }
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