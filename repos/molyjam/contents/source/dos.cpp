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

// "Apple II" screens

// First: Polycode special support

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
	TypeScene(type_automaton *_source, bool _virtualScene = false) : Scene(_virtualScene), source(_source) {
		mesh = new Mesh(Mesh::QUAD_MESH);
		english();
	}
	static Texture *english() {
		static bool __haveLoaded = false;
		static Texture *__english = NULL;
		if (!__haveLoaded) {
			CoreServices::getInstance()->getRenderer()->setTextureFilteringMode(Renderer::TEX_FILTERING_NEAREST);
			__english = CoreServices::getInstance()->getMaterialManager()->createTextureFromFile("media/english.png", true, false);
			
			int xbase = 7*40, ybase = 8*24;
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
		
		mesh->clearMesh();
		
		for(int y = 0; y < 24; y++) {
			for(int x = 0; x < 40; x++) {
				if (source->screen[y][x] != ' ') {
					clipush(x,y,source->screen[y][x]);
				}
			}
		}
		
		renderer->setVertexColor(1.0,1.0,1.0,1.0);
		renderer->setTexture(english());
		renderer->pushDataArrayForMesh(mesh, RenderDataArray::VERTEX_DATA_ARRAY);
		renderer->pushDataArrayForMesh(mesh, RenderDataArray::TEXCOORD_DATA_ARRAY);
		renderer->drawArrays(mesh->getMeshType());
		
		// Leave the world as you found it
		CoreServices::getInstance()->getRenderer()->setPerspectiveMode();
	};
};

// type_automaton *cli, *next_cli; // TODO remove?

type_automaton::type_automaton() : automaton(), display(NULL) {
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
	screen[y][x] = c | (inv?0x80:0);
}
void type_automaton::set(int x, int y, string s, bool inv) {
	for(int c = 0; c < s.size() && x+c<40; c++)
		screen[y][x+c] = s[c] | (inv?0x80:0);
}

void type_automaton::set(int x, int y, String s, bool inv) {
	set(x,y,s.getSTLString(),inv);
}
void type_automaton::set_centered(int x, int y, int w, string s, bool clearfirst, bool inv) {
	if (clearfirst) {
		for(int c = 0; c < w; c++) {
			set(x+c,y,' ',inv);
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
	for(int y = 0; y < 24; y++) {
		for(int x = 0; x < 40; x++) {
			ERR("%c", screen[y][x]<32||screen[y][x]==127?' ':screen[y][x]&0x7F);
		}
		ERR("\n");
	}
#endif
}

void type_automaton::scroll() {
	memmove(&screen[0][0], &screen[1][0], sizeof(screen[0])*23);
	memset(&screen[23][0], ' ', sizeof(screen[22]));
	memmove(&scolor[0][0], &scolor[1][0], sizeof(scolor[0])*23);
	memset(&scolor[23][0], 0xFF, sizeof(scolor[22]));	
}

void type_automaton::rscroll() {
	memmove(&screen[1][0], &screen[0][0], sizeof(screen[0])*23);
	memset(&screen[0][0], ' ', sizeof(screen[0]));
	memmove(&scolor[1][0], &scolor[0][0], sizeof(scolor[0])*23);
	memset(&scolor[0][0], 0xFF, sizeof(scolor[0]));	
}

#define MINI_TEXT 0

void type_automaton::save_text(string filename) {
	ofstream f;
	f.open(filename.c_str(), ios_base::out | ios_base::binary | ios_base::trunc);
	if (!f.fail()) {
#if !MINI_TEXT
		f.write((char *)&screen[0][0], sizeof(screen));
#else
		for(int y = 0; y < 10; y++)
			f.write((char *)&screen[2+y][13], 25);
#endif
	}	
}

void type_automaton::load_text(string filename) {	
	ifstream f;
	//	f.exceptions( ios::eofbit | ios::failbit | ios::badbit ); // WHO CARES!
	f.open(filename.c_str(), ios_base::in | ios_base::binary);
#if !MINI_TEXT
	f.read((char *)&screen[0][0], sizeof(screen));
#else
	for(int y = 0; y < 10; y++)
		f.read((char *)&screen[2+y][13], 25);
#endif
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
					if (was_lctrl)
						lctrl = true;
					else
						rctrl = true;
				} else {
					if (lctrl || rctrl) {
						if (code >= '0' && code <= '9') {
							screen[y][x] *= 10;
							screen[y][x] += code - '0';
						}
					} else {
						if (code == KEY_F1) {
							save_text("ascii.obj");
						} else if (code == KEY_F2) {
							load_text("ascii.obj");
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
	if (x >= 40) {
		x = 0;
		y++;
	}
	if (x < 0) {
		x = 39;
		y--;
	}
	if (y >= 24) {
		y = 0;
	}
	if (y < 0) {
		y = 23;
	}
}
