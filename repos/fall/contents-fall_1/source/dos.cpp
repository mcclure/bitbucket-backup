/*
 *  dos.cpp
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 2/23/12.
 *  Copyright 2012 Run Hello. All rights reserved.
 *
 */

#include "dos.h"
#include "program.h"
#include <fstream>
#include "physfs.h"

#define BLACK 0x00FFFFFF
#define WHITE 0xFFFFFFFF

// "Apple II" screens

// First: Polycode special support

// Assumes len is a multiple of 4
static void __local_memset_pattern4(void *b, const void *pattern4, size_t len) {
	uint32_t *to = (uint32_t *)b;
	uint32_t *from = (uint32_t *)pattern4;
	for(int c = 0; c < len/4; c++)
		to[c] = *from;
}

type_automaton *type_automaton::_singleton = NULL;

#define TAFX (7*col40factor)
#define TAFY (8*col40factor)
#define TAU (col40yo)
#define TAL (col40xo)
#define TTU (0)
#define TTL (0)
#define TTFX (7.0/128)
#define TTFY (1.0/16)
static int col40xo, col40yo, col40factor;

struct TypeScene : public Scene {
	type_automaton *source;
	Mesh *mesh;
	Mesh *gfx_mesh;
	Texture *gfx;
	TypeScene(type_automaton *_source, bool _virtualScene = false) : Scene(_virtualScene), source(_source), gfx_mesh(NULL), gfx(NULL) {
		mesh = new Mesh(Mesh::QUAD_MESH);
		english();
	}
	~TypeScene() {
		delete mesh;
		delete gfx_mesh;
		delete gfx;
	}
	static Texture *english() {
		static bool __haveLoaded = false;
		static Texture *__english = NULL;
		if (!__haveLoaded) {
			CoreServices::getInstance()->getRenderer()->setTextureFilteringMode(Renderer::TEX_FILTERING_NEAREST);
			__english = CoreServices::getInstance()->getMaterialManager()->createTextureFromFile("media/english.png", true, false);
			
			int xbase = 7*DOS_LINE_COLS, ybase = 8*DOS_LINE_ROWS;
			int xfactor = surface_width / xbase, yfactor = surface_height / ybase;
			int usefactor = ::min(xfactor,yfactor);
			int	col40w = xbase * usefactor, col40h = ybase * usefactor; col40factor = usefactor;
			col40xo = (surface_width - col40w)/2; col40yo = (surface_height - col40h)/2;
			ERR("xf %d yf %d uf %d c40w %d c40h %d\n", xfactor, yfactor, usefactor, col40w, col40h);			
			
			__haveLoaded = true;
		}
		return __english;
	}
	void clipush(int x, int y, unsigned char c) {
		Polycode::Polygon *p = new Polycode::Polygon;
		
		unsigned int tx = c%16; unsigned int ty = 15 - c/16;
		double pos[4] = {TAL+TAFX*x, TAU+TAFY*y, 0, 0};
		pos[2] = pos[0] + TAFX; pos[3] = pos[1] + TAFY;
		double tex[4] = {TTL + TTFY*tx, TTU + TTFY*ty, 0, 0}; // Note: TTFY in "wrong" place on purpose
		tex[2] = tex[0] + TTFX; tex[3] = tex[1] + TTFY;
		
//		ERR("At %d,%d VERTEX %lf,%lf,%lf,%lf TEX %lf,%lf,%lf,%lf", x, y, pos[0],pos[1],pos[2],pos[3], tex[0],tex[1],tex[2],tex[3]);
		p->addVertex(pos[0],pos[1],0, tex[0], tex[3]);
		p->addVertex(pos[2],pos[1],0, tex[2], tex[3]);
		p->addVertex(pos[2],pos[3],0, tex[2], tex[1]);
		p->addVertex(pos[0],pos[3],0, tex[0], tex[1]);
		
		mesh->addPolygon(p);
	}
	virtual void Render(Camera *targetCamera = NULL) {
		if (source != type_automaton::singleton())
			return;
		
		Renderer *renderer = CoreServices::getInstance()->getRenderer();

		renderer->loadIdentity();
		renderer->setOrthoMode(renderer->getXRes(), renderer->getYRes());
		
		if (source->screen_dirty) {
			mesh->clearMesh();
			
			for(int y = 0; y < DOS_LINE_ROWS; y++) {
				for(int x = 0; x < DOS_LINE_COLS; x++) {
					if (source->screen[y][x] != DOS_BLANK) {
						clipush(x,y,source->screen[y][x]);
					}
				}
			}
			source->screen_dirty = false;
		}
		
		renderer->setVertexColor(1.0,1.0,1.0,1.0);
		renderer->setTexture(english());
		renderer->pushDataArrayForMesh(mesh, RenderDataArray::VERTEX_DATA_ARRAY);
		renderer->pushDataArrayForMesh(mesh, RenderDataArray::TEXCOORD_DATA_ARRAY);
		renderer->drawArrays(mesh->getMeshType());
		
		if (source->gfx_dirty) {
			//CoreServices::getInstance()->getRenderer()->setTextureFilteringMode(Renderer::TEX_FILTERING_NEAREST);
			delete gfx;
			gfx = CoreServices::getInstance()->getRenderer()->createTexture(DOS_GFX_COLS, DOS_GFX_ROWS, (char *)&source->gfx[0][0], true, false);
			if (!gfx_mesh) {
				gfx_mesh = new Mesh(Mesh::QUAD_MESH);
				Polycode::Polygon *p = new Polycode::Polygon;
				
				double pos[4] = {TAL, TAU, 0, 0};
				pos[2] = pos[0] + TAFX*DOS_LINE_COLS; pos[3] = pos[1] + TAFY*DOS_LINE_ROWS;
				double tex[4] = {0, 1, 0, 0}; // Note: TTFY in "wrong" place on purpose
				tex[2] = tex[0] + 1; tex[3] = tex[1] - 1;
				
				//		ERR("At %d,%d VERTEX %lf,%lf,%lf,%lf TEX %lf,%lf,%lf,%lf", x, y, pos[0],pos[1],pos[2],pos[3], tex[0],tex[1],tex[2],tex[3]);
				p->addVertex(pos[0],pos[1],0, tex[0], tex[3]);
				p->addVertex(pos[2],pos[1],0, tex[2], tex[3]);
				p->addVertex(pos[2],pos[3],0, tex[2], tex[1]);
				p->addVertex(pos[0],pos[3],0, tex[0], tex[1]);
				
				gfx_mesh->addPolygon(p);
			}
			source->gfx_dirty = false;
		}
		if (gfx) {
			renderer->setVertexColor(1.0,1.0,1.0,1.0);
			renderer->setTexture(gfx);
			renderer->pushDataArrayForMesh(gfx_mesh, RenderDataArray::VERTEX_DATA_ARRAY);
			renderer->pushDataArrayForMesh(gfx_mesh, RenderDataArray::TEXCOORD_DATA_ARRAY);
			renderer->drawArrays(gfx_mesh->getMeshType());			
		}
		
		// Leave the world as you found it
		CoreServices::getInstance()->getRenderer()->setPerspectiveMode();
	};
};

gfxcontainer::gfxcontainer() : gfx_dirty(false) {
	pxclear();
}

void gfxcontainer::pxset(int x, int y, bool on) {
	if (y>= 0 && y < DOS_GFX_ROWS && x >= 0 && x < DOS_GFX_COLS)
		gfx[y][x] = on?WHITE:BLACK;
	gfx_dirty = true;
}

void gfxcontainer::pxfill(unsigned int v, int _dstx, int _dsty, int dx, int dy) {
	if (_dstx < 0) { dx += _dstx; _dstx = 0; }
	if (dx < 0) { dx = DOS_GFX_COLS; }
	if (dy < 0) { dy = DOS_GFX_ROWS; }
	dx = ::min(dx, DOS_GFX_COLS - _dstx);
	dy = ::min(dy, DOS_GFX_ROWS - _dsty);
	if (dx <= 0 || dy <= 0) return;

	for(int y = 0; y < dy; y++) {
		int dsty = _dsty + y;
		__local_memset_pattern4(&gfx[dsty][_dstx], &v, sizeof(gfx[0][0])*dx);
	}	
}

inline bool fix(int &_dstx, int &_dsty, int &_srcx, int &_srcy, int &dx, int &dy) {
	if (_dstx < 0) { dx += _dstx; _dstx = 0; }
	if (_srcx < 0) { dx += _srcx; _srcx = 0; }
	if (_dsty < 0) { dy += _dsty; _dsty = 0; }
	if (_srcy < 0) { dy += _srcy; _srcy = 0; }
	if (dx < 0) { dx = DOS_GFX_COLS; }
	if (dy < 0) { dy = DOS_GFX_ROWS; }
	dx = ::min(dx, DOS_GFX_COLS - _dstx);
	dx = ::min(dx, DOS_GFX_COLS - _srcx);
	dy = ::min(dy, DOS_GFX_ROWS - _dsty);
	dy = ::min(dy, DOS_GFX_ROWS - _srcy);
	if (dx <= 0 || dy <= 0) return false;
	return true;	
}

void gfxcontainer::pxcopy(const gfxcontainer *src, int _dstx, int _dsty, int _srcx, int _srcy, int dx, int dy) {
	if (!fix(_dstx,_dsty,_srcx,_srcy,dx,dy)) return;
	
	for(int y = 0; y < dy; y++) {
		int dsty = _dsty + y;
		int srcy = _srcy + y;
		memmove(&gfx[dsty][_dstx], &src->gfx[srcy][_srcx], sizeof(gfx[0][0])*dx);
	}
	gfx_dirty = true;
}

void gfxcontainer::pxcopy_invert(const gfxcontainer *src, int _dstx, int _dsty, int _srcx, int _srcy, int dx, int dy) {
	if (!fix(_dstx,_dsty,_srcx,_srcy,dx,dy)) return;
	
	for(int y = 0; y < dy; y++) {
		int dsty = _dsty + y;
		int srcy = _srcy + y;
		for(int x = 0; x < dx; x++) {
			int dstx = _dstx + x;
			int srcx = _srcx + x;
			
			unsigned char *bdst = (unsigned char *)&gfx[dsty][dstx];
			unsigned char *bsrc = (unsigned char *)&src->gfx[srcy][srcx];
			
			for(int c = 0; c < 3; c++)
				bdst[c] = 255 - bsrc[c];
			bdst[3] = bsrc[3];
		}
	}
	gfx_dirty = true;
}

void gfxcontainer::pxcopy_threshold(const gfxcontainer *src, Number threshold, int _dstx, int _dsty, int _srcx, int _srcy, int dx, int dy) {
	if (!fix(_dstx,_dsty,_srcx,_srcy,dx,dy)) return;
	
	for(int y = 0; y < dy; y++) {
		int dsty = _dsty + y;
		int srcy = _srcy + y;
		for(int x = 0; x < dx; x++) {
			int dstx = _dstx + x;
			int srcx = _srcx + x;
			
			unsigned char *bsrc = (unsigned char *)&src->gfx[srcy][srcx];
			unsigned int total = 0;
			for(int c = 0; c < 3; c++)
				total += bsrc[c];
			
			gfx[dsty][dstx] = total > (255 * 3)*threshold ? WHITE : BLACK;
		}
	}
	gfx_dirty = true;
}

void gfxcontainer::pxswap(gfxcontainer *src, int _dstx, int _dsty, int _srcx, int _srcy, int dx, int dy) {
	if (!fix(_dstx,_dsty,_srcx,_srcy,dx,dy)) return;
	
	for(int y = 0; y < dy; y++) {
		int dsty = _dsty + y;
		int srcy = _srcy + y;
		for(int x = 0; x < dx; x++) {
			int dstx = _dstx + x;
			int srcx = _srcx + x;
			
			// RELEVANT
			uint32_t temp = gfx[dsty][dstx];
			gfx[dsty][dstx] = src->gfx[srcy][srcx];
			src->gfx[srcy][srcx] = temp;
		}
	}
	gfx_dirty = true;	
}

void gfxcontainer::load_image(string filename) {
	Image temp(filename);
	if (temp.isLoaded()) {
		pxclear();
		
		const char *pixels = temp.getPixels();
		int srclen = temp.getWidth()*temp.getHeight();
		int dstlen = DOS_GFX_ROWS*DOS_GFX_COLS;
		int cpylen = ::min(dstlen, srclen);
		memcpy(&gfx[0][0] + (dstlen-cpylen), pixels, cpylen*sizeof(gfx[0][0]));
		// Polycode Images come in upside down for some reason
		for(int y = 0; y < DOS_GFX_ROWS/2; y++) {
			pxswap(this, 0,y,0,DOS_GFX_ROWS-y-1,DOS_GFX_COLS,1);
		}
	}
	gfx_dirty = true;
}

void gfxcontainer::save_image(string filename) {
	// TODO
}

// type_automaton *cli, *next_cli; // TODO remove?

type_automaton::type_automaton() : automaton(), gfxcontainer(), display(NULL), screen_dirty(true) {
	clear();
}

void type_automaton::insert() {
	automaton::insert();
	display = new TypeScene(this);
	_singleton = this;
}

type_automaton::~type_automaton() {
	delete display;
	if (this == _singleton)
		_singleton = NULL;
}

Scene *type_automaton::displayScene() { return display; }

String type_automaton::get(int x, int y) {
	const char ch[2] = {screen[y][x],'\0'};
	return String(ch);
}

void type_automaton::set(int x, int y, unsigned char c, bool inv) {
	if (y>= 0 && y < DOS_LINE_ROWS && x >= 0 && x < DOS_LINE_COLS)
		screen[y][x] = c | (inv?0x80:0);
	screen_dirty = true;
}
void type_automaton::set(int x, int y, string s, bool inv) {
	for(int c = 0; c < s.size() && x+c<DOS_LINE_COLS; c++)
		screen[y][x+c] = s[c] | (inv?0x80:0);
	screen_dirty = true;
}

void type_automaton::set(int x, int y, String s, bool inv) {
	set(x,y,s.getSTLString(),inv);
}
void type_automaton::set_centered(int x, int y, int w, string s, bool clearfirst, bool inv) {
	if (clearfirst) {
		for(int c = 0; c < w; c++) {
			set(x+c,y,DOS_BLANK,inv);
		}
	}
	if (s.size() > w) {
		bool ends_bracket = s[s.size()-1] == ']';
		s = s.substr(0,w);
		if (ends_bracket)
			s[w-1] = ']';
	}
	set(x + (w - s.size())/2, y, s, inv);
}
void type_automaton::set_centered(int x, int y, int w, String s, bool clearfirst, bool inv) {
	set_centered(x,y,w,s.getSTLString(),clearfirst,inv);
}

void type_automaton::dump() { // Debug
#if SELF_EDIT
	for(int y = 0; y < DOS_LINE_ROWS; y++) {
		for(int x = 0; x < DOS_LINE_COLS; x++) {
			ERR("%c", screen[y][x]<32||screen[y][x]==127?DOS_BLANK:screen[y][x]&0x7F);
		}
		ERR("\n");
	}
#endif
}

void type_automaton::scroll() {
	memmove(&screen[0][0], &screen[1][0], sizeof(screen[0])*23);
	memset(&screen[23][0], DOS_BLANK, sizeof(screen[22]));
	memmove(&scolor[0][0], &scolor[1][0], sizeof(scolor[0])*23);
	memset(&scolor[23][0], 0xFF, sizeof(scolor[22]));
	screen_dirty = true;
}

void type_automaton::rscroll() {
	memmove(&screen[1][0], &screen[0][0], sizeof(screen[0])*23);
	memset(&screen[0][0], DOS_BLANK, sizeof(screen[0]));
	memmove(&scolor[1][0], &scolor[0][0], sizeof(scolor[0])*23);
	memset(&scolor[0][0], 0xFF, sizeof(scolor[0]));	
	screen_dirty = true;
}

void type_automaton::save_text(String filename) {
	save_text(filename.getSTLString());
}

void type_automaton::save_text(string filename) {
	ofstream f;
	f.open(filename.c_str(), ios_base::out | ios_base::binary | ios_base::trunc);
	if (!f.fail()) {
		f.write((char *)&screen[0][0], sizeof(screen));
	}
}

void type_automaton::load_text(String filename) {
	load_text(filename.getSTLString());
}

void type_automaton::load_text(string filename) {	
	PHYSFS_file *f = PHYSFS_openRead(filename.c_str());
	if (f) {
		PHYSFS_read(f, (char *)&screen[0][0], 1, sizeof(screen));
		PHYSFS_close(f);
	}
	screen_dirty = true;
}

Number type_automaton::factor() {
	return col40factor;
}

freetype_automaton::freetype_automaton() : type_automaton(), x(0),y(0),autoinv(false), cursor(true), lctrl(false), rctrl(false) {
	cor->getInput()->addEventListener(this, InputEvent::EVENT_KEYUP);
	cor->getInput()->addEventListener(this, InputEvent::EVENT_KEYDOWN);
}

void freetype_automaton::handleEvent(Event *e) {
	if (done) return;
	
	if(e->getDispatcher() == cor->getInput()) {
		InputEvent *inputEvent = (InputEvent*)e;
		int code = inputEvent->keyCode();
		bool was_lctrl = code == KEY_LCTRL, was_rctrl = code == KEY_RCTRL;

		switch(e->getEventCode()) {
			case InputEvent::EVENT_KEYUP: {
				if (was_lctrl || was_rctrl) {
					next(1,0);
					if (was_lctrl)
						lctrl = false;
					else
						rctrl = false;
				}
			} break;
			case InputEvent::EVENT_KEYDOWN: {
				if (was_lctrl || was_rctrl) {
					screen[y][x] = 0;
					screen_dirty = true;
					if (was_lctrl)
						lctrl = true;
					else
						rctrl = true;
				} else {
					if (lctrl || rctrl) {
						if (code >= '0' && code <= '9') {
							screen[y][x] *= 10;
							screen[y][x] += code - '0';
							screen_dirty = true;
						}
					} else {
						if (code == KEY_F1) {
							save_text(S("ascii.obj"));
						} else if (code == KEY_F2) {
							load_text(S("ascii.obj"));
						} else if (code == KEY_F3) {
							autoinv = !autoinv;
						} else if (code == KEY_F4) {
							cursor = !cursor;
						} else if (code == KEY_LEFT || code == KEY_BACKSPACE) {
							next(-1,0);
						} else if (code == KEY_RIGHT) {
							next(1,0);
						} else if (code == KEY_UP) {
							next(0,-1);
						} else if (code == KEY_DOWN || code == KEY_RETURN) {
							next(0,1);
							if (code == KEY_RETURN)
								x = 0;
						} else if (IS_MODIFIER(code) || code == KEY_ESCAPE || code == KEY_TAB || !inputEvent->charCode) {
							// DO NOTHING!!!
						} else {
							set(x,y,inputEvent->charCode,autoinv);
							next(1,0);
						}
					}
				}
			} break;
		}
	}
}

void freetype_automaton::next(int dx, int dy) {
	x+=dx; y += dy;
	if (x >= DOS_LINE_COLS) {
		x = 0;
		y++;
	}
	if (x < 0) {
		x = DOS_LINE_COLS-1;
		y--;
	}
	if (y >= DOS_LINE_ROWS) {
		y = 0;
	}
	if (y < 0) {
		y = DOS_LINE_ROWS-1;
	}
}
