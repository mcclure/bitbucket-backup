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
//#define DOS_GFX_ROWS 192
//#define DOS_GFX_COLS 280
#define DOS_BLANK ' '

struct gfxcontainer {
	int w; int h;
	vector<uint32_t> gfx; // NOTE: Y,X
	inline bool within(int x, int y) const {
		return x >= 0 && y >= 0 && x < w && y < h;
	}
	inline uint32_t &px(int x, int y) {
		return gfx[y*w + x];
	}
	inline const uint32_t &px(int x, int y) const {
		return gfx[y*w + x];
	}
	bool pxhas(int x, int y) const { // Check alpha
		return within(x,y) && (px(x,y)&0xFF000000);
	}
	inline Color pxc(int x, int y) const {
		return Color(px(x,y));
	}
	bool gfx_dirty;
	void pxclear() {
		memset(&gfx[0], 0, sizeof(uint32_t)*gfx.size());
		gfx_dirty = true;
	}
	bool fix(const gfxcontainer *src, int &x, int &y, int &srcx, int &srcy, int &dx, int &dy) const;
	void pxset(int x, int y, bool on);
	void pxfill(unsigned int v, int _dstx = 0, int _dsty = 0, int dy = -1, int dx = -1);
	void pxcopy(const gfxcontainer *src, int _dstx = 0, int _dsty = 0, int _srcx = 0, int _srcy = 0, int _dx = -1, int dy = -1);
	void pxcopy_invert(const gfxcontainer *src, int _dstx = 0, int _dsty = 0, int _srcx = 0, int _srcy = 0, int _dx = -1, int dy = -1);
	void pxcopy_xor(const gfxcontainer *src, int _dstx = 0, int _dsty = 0, int _srcx = 0, int _srcy = 0, int _dx = -1, int dy = -1);
	void pxcopy_blend(const gfxcontainer *src, int _dstx = 0, int _dsty = 0, int _srcx = 0, int _srcy = 0, int _dx = -1, int dy = -1);
	void pxcopy_threshold(const gfxcontainer *src, Number threshold, int _dstx = 0, int _dsty = 0, int _srcx = 0, int _srcy = 0, int _dx = -1, int dy = -1);
	void pxswap(gfxcontainer *src, int _dstx = 0, int _dsty = 0, int _srcx = 0, int _srcy = 0, int _dx = -1, int dy = -1);
	void load_image(String filename, bool resize=true) { load_image(filename.getSTLString(), resize); }
	void load_image(string filename, bool resize=true);
	void save_image(string filename) const;
	void init(int _w, int _h);
	gfxcontainer(int _w = 280, int _h = 192); // DOS_GFX_COLS, DOS_GFX_ROWS
};

struct type_automaton : public automaton, public gfxcontainer {
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
	void rinse_offset();
	
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
	void apply_offset(int x, int y);

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