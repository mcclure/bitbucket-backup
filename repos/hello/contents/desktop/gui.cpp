// INTERFACES

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>

#include "chipmunk.h"
#include "color.h"
#include "program.h"
#include "tinyxml.h"
#include "sound.h"
#include "internalfile.h"
#include "bfiles.h"
#include "cpRect.h"

// Wait this is all just joystick code! I need a joystick.cpp.

static const int controls_file_version = 1;

void load_controls() { // Load entire controls file from disk
	char filename[FILENAMESIZE];
	
	try {
		userPath(filename, "controls.obj");
		ifstream f;
		f.exceptions( ios::eofbit | ios::failbit | ios::badbit );
		f.open(filename, ios_base::in | ios_base::binary);
		
		int version; BREAD(version);
		if (version != controls_file_version) {
			ERR("Controls file version wrong, found %d\n", version);
			return;
		}
		
		for(int c = 0; c < MAX_PLAYERS; c++) {
			BREADS(file_jnames[c]);
			BREAD(file_defaults[c]);
			ERR("%d: jnames0: %d\n", c, (int)file_jnames[c][0]);
			bool joystick_known = file_jnames[c][0] && joysticks_by_name.count(file_jnames[c]);
			if(file_defaults[c].jid == -1 || joystick_known) {
				ERR("%d: Overwrote!\n", c);
				if (joystick_known)
					file_defaults[c].jid = joysticks_by_name[file_jnames[c]];
				defaults[c] = file_defaults[c];
			}
		}
	} catch ( exception &e ) {
		ERR("LOAD CONTROLS EXCEPT (%s)\n",e .what());
	}	
}

void push_controls(int c) { // Push defaults into file_defaults
	file_defaults[c] = defaults[c];
	if (file_defaults[c].jid >= 0)
		file_jnames[c] = joysticks[file_defaults[c].jid].name; // This is starting to feel a bit ad hoc.
	else
		file_jnames[c].clear();
}

void save_controls() { // Dump entire controls file to disk
	char filename[FILENAMESIZE];
	
	userPath(filename, "controls.obj");
	ofstream f;
	f.open(filename, ios_base::out | ios_base::binary | ios_base::trunc);
	if (!f.fail()) {
		string stemp;
		BWRITE(controls_file_version);
		for(int c = 0; c < MAX_PLAYERS; c++) {
			string jname;
			if (file_defaults[c].jid >= 0)
				jname = file_jnames[c];
			BWRITES(jname);
			BWRITE(file_defaults[c]);
		}
	}			
}

string single_control::readable() {
	ostringstream result;
	if (is_button()) {
		result << "Button " << (int)button;
	} else if (is_axis()) {
		result << "Axis " << (int)axis;
	} else if (is_hat()) {
		result << "Hat " << (int)hat;
	} else if (is_key()) {
		extern const char *readableKey(SDLKey c);
		result << readableKey(key);
	} else {
		result << "???";
	}
	return result.str();
}

// "Apple II" screens

type_automaton::~type_automaton() { if (this == cli) cli = NULL; if (this == next_cli) next_cli = NULL; }

void type_automaton::set(int x, int y, unsigned char c, bool inv) {
	screen[y][x] = c | (inv?0x80:0);
}
void type_automaton::set(int x, int y, string s, bool inv) {
	for(int c = 0; c < s.size() && x+c<40; c++)
		screen[y][x+c] = s[c] | (inv?0x80:0);
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

void freetype_automaton::input(SDL_Event &event) {
	bool wasctrl = (event.key.keysym.sym == SDLK_LCTRL || event.key.keysym.sym == SDLK_RCTRL);
	if (event.type==SDL_KEYUP && wasctrl) {
		next(1,0);
	} else if (event.type==SDL_KEYDOWN) {
		if (wasctrl) {
			screen[y][x] = 0;
		} else {
			if (SDL_GetModState() & (KMOD_LCTRL|KMOD_RCTRL)) {
				if (event.key.keysym.sym >= '0' && event.key.keysym.sym <= '9') {
					screen[y][x] *= 10;
					screen[y][x] += event.key.keysym.sym - '0';
				}
			} else {
				if (event.key.keysym.sym == SDLK_F1) {
					save_text("ascii.obj");
				} else if (event.key.keysym.sym == SDLK_F2) {
					load_text("ascii.obj");
				} else if (event.key.keysym.sym == SDLK_F3) {
					autoinv = !autoinv;
				} else if (event.key.keysym.sym == SDLK_F4) {
					cursor = !cursor;
				} else if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_BACKSPACE) {
					next(-1,0);
				} else if (event.key.keysym.sym == SDLK_RIGHT) {
					next(1,0);
				} else if (event.key.keysym.sym == SDLK_UP) {
					next(0,-1);
				} else if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_RETURN) {
					next(0,1);
					if (event.key.keysym.sym == SDLK_RETURN)
						x = 0;
				} else if (event.key.keysym.sym == SDLK_CAPSLOCK || event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_RSHIFT || event.key.keysym.sym == SDLK_LMETA) {
					// DO NOTHING!!!
				} else {
					set(x,y,event.key.keysym.unicode,autoinv);
					next(1,0);
				}
			}
		}
	}
}

void type_automaton::scroll() {
	memmove(&screen[0][0], &screen[1][0], sizeof(screen[0])*23);
	memset(&screen[23][0], ' ', sizeof(screen[22]));
	memmove(&scolor[0][0], &scolor[1][0], sizeof(scolor[0])*23);
	memset(&scolor[23][0], 0xFF, sizeof(scolor[22]));	
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

// Start interface stuff

enum MiscButtonType {  };

// Will I ever stop doing interfaces with big ugly globals like this?
int current_player = 0, current_joystick = -1;

class MiscButton : public ControlBase { public:
	MiscButtonType t;
	MiscButton(string _text, MiscButtonType _t) : ControlBase(_text, true), t(_t) {}
	virtual void click() {
		switch (t) {
			// TODO: NEVER
		}
	}
};

// The "this is a good time to modify your uiSpace" function. Main.cpp calls this once at the beginning of the main
// loop and once every time a button is successfully clicked. This exists because it's unsafe to modify the interface
// from inside of a ControlBase click() handler, so instead click() handlers can stash some state that this function
// then picks up when main.cpp automatically calls it after the click is completed...
void program_interface() {
	WantNewScreen wantAfterwards = WNothing; // "want" will be cleared with this at the end of the function
	
#ifdef TARGET_DESKTOP
	if (wantClearUi) {
		for(int c = 0; c < columns.size(); c++)
			if (columns[c])
				delete columns[c];
		
		columns.clear();
		
		resetScrollmax();
		wantClearUi = false;
	}
#endif
	
	switch (want) {	
			// Nothing to fear, nothing to want.
	}
	
	want = wantAfterwards;
}