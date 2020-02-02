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

struct control_setterbase : public control_handler {
	int current_player;
	
	control_setterbase(int _current_player, int _current_joystick) : control_handler(_current_joystick),
	current_player(_current_player) {
	}
	virtual void set_feedback(unsigned int which) = 0; // All values are valid inputs. So says the comment
	virtual void input(SDL_Event &event) {
		if (halted || state<0) return; // Cannot run if die() has been called or if we have not yet started
		
		single_control result;
		bool have_result = false;
		bool duplicate = false;
		bool duplicate4 = false;
		
#if 0
		// Crazy special auto-aim triggering-- DON'T KEEP
		if (state == (SPECIAL_FIRE_BUTTON+1) && event.type == SDL_KEYDOWN && event.key.keysym.sym == '`') {
			if (controls[SPECIAL_FIRE_BUTTON].is_key() || controls[SPECIAL_FIRE_BUTTON].is_button()) {
				defaults[current_player].auto_aim = true;
				clearat(state);
				for(int c = 0; c < 3; c++) {
					controls[state] = result; state++; // Don't set_feedback as we go
				} 
				set_feedback(state);
				have_result = false;
			}
			return;
		}		
#endif
		
		switch(event.type) {
			case SDL_KEYDOWN: {
				if (-1 != jid) return; // BAIL OUT COMPLETELY
				result.key = event.key.keysym.sym;
				have_result = true;
			} break;
			case SDL_JOYBUTTONDOWN: {
				if (event.jbutton.which != jid) return; // BAIL OUT COMPLETELY
				result.button = event.jbutton.button;
				have_result = true;
			} break;
			case SDL_JOYHATMOTION: {
				if (event.jhat.which != jid) return; // BAIL OUT COMPLETELY
				result.hat = event.jhat.hat;
				have_result = true;
			} break;
			case SDL_JOYAXISMOTION: {
				if (event.jaxis.which != jid) return; // BAIL OUT COMPLETELY
				double value = double(event.jaxis.value)/(1<<15);
				if (0.5 < fabs(value)) {
					result.axis = event.jaxis.axis;
					have_result = true;
				}
			} break;
		}
		
		// If this button press is a duplicate, drop it
		for(int c = 0; have_result && c < state; c++) {
			if (result == controls[c]) {
				have_result = false;
			}
		}
		
		// Special behavior around hats/sticks
		if (have_result) { // I feel like if this weren't here this code would be a lot cleaner...
			int whoami = state%4; // Within one "analog stick", where are we?
			int whereami = state - whoami; // Which "analog stick" are we looking at?
			bool is_axis = result.is_axis();
			bool axis_ok = axis_based(state); // No sticks for snap
			
			if (is_axis) { // Axes detected 
				if (whoami < 2 && axis_ok) { // If axis is one of first two: Duplicate further down
					duplicate = true;
				} else {
					have_result = false; // If axis is one of last two: ignore
				}
			}
			
			if (result.is_hat()) { // Hat detected
				if (whoami == 0 && axis_ok) {
					duplicate4 = true; // If hat is the first one: Duplicate x 4
				} else {
					have_result = false; // If hat isn't the first one: ignore
				}
			}
			
			// If someone is trying to mix buttons and sticks for a single "analog stick" bail out
			for(int c = 0; have_result && c < whoami; c++) { // Part redundant? Dunno
				if (controls[whereami + c].is_axis() != is_axis) {
					have_result = false;
				}
			}
		}
		
		if (have_result) {
			controls[state] = result;
			set_feedback(state);
			if (duplicate||duplicate4) {
				controls[state+2] = result;
			}
			if (duplicate4) {
				controls[state+1] = result;
				controls[state+3] = result;
			}
			
			do { // Uh, what I said about clean code? I could do without this weird loop too.
				state++; // Move to next state
				
				if (state == SPECIAL_FIRE_BUTTON + 1 && auto_aim) {
					state += 3; // Slightly less crazy auto_aim behavior.
				}
				
				set_feedback(state); // Mark we did so
				
				// Did we just move off the end of the controls list?
				if (state >= KHKEYS) {
					save(defaults[current_player]);
					push_controls(current_player); // We changed defaults; reflect this
					save_controls();
					done();
					return; // HEY LOOK WE'RE LEAVING THE FUNCTION
				}
				
				// So we've moved to a new control, but because of the axis thing, maybe
			} while (controls[state].is_anything()); // this is already filled in! If so skip it
		}
	}
	virtual void clearat(int) {}
	virtual void done() {}
};

struct control_guisetter : public control_setterbase {
	ControlBase *feedback[KHKEYS];
	ControlBase *feedbackAfterwards; // Unhide when done?
	
	control_guisetter(int _current_player, int _current_joystick) : control_setterbase(_current_player,_current_joystick) {
		state = 0; // State => which control are we checking on right now
		for(int c = 0; c < KHKEYS; c++) {
			feedback[c] = NULL;
		}
		defaults[current_player].auto_aim = false; // Because we reset instantly
	}	
	virtual void tick() {} // DOESN'T CALL UPWARD (obviously)
	virtual void set_feedback(unsigned int which) { // All values are valid input
		if (which < KHKEYS && feedback[which]) {
			const char *controls_mean[KHKEYS] = {"Move left", "Move up", "Move right", "Move down",
				"Fire left","Fire up","Fire right","Fire down","Snap"};
			string current_mean = controls_mean[which];
			if (controls[which].is_anything()) {
				feedback[which]->text = current_mean + ": " + controls[which].readable();
				feedback[which]->textcolor = 0xFFFFFFFF;
			} else {
				feedback[which]->text = (0==which?"Press: ":"Now press: ") + current_mean;
				feedback[which]->textcolor = playerColor[current_player];
			}
		}
	}
	virtual void push() {
		set_feedback(0); // Why do this here and not in the constructor, again?
		control_setterbase::push();
	}	
	virtual void clearat(int y) { // Necessary?
		feedback[y]->text = "";
	}
	virtual void done() {
		if (feedbackAfterwards) {
			feedbackAfterwards->hidden = false;
		}		
		die();
	}
};

// Partners with player_screen
struct control_textsetter : public control_setterbase {
	player_screen *owner;
	control_textsetter(player_screen *_owner, int _current_player, int _current_joystick) : control_setterbase(_current_player, _current_joystick), owner(_owner) {
		state = -1;
		rollover = -1;
	}
	virtual void tick() {
		if (rollover >= 0) {
			state = 0;
			rollover = -1;
			owner->reset(current_player);
		}
	}
	virtual void set_feedback(unsigned int which) {
		owner->reset(current_player);
	}
    virtual void done() {
		ERR("DONE!\n");
		owner->state = 0; // pss_present
		owner->reset(0);
		owner->reset(1);
		state = -1;
    }
};


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
	bool wasctrl = (event.key.keysym.sym == SDLK_LCTRL || event.key.keysym.sym == SDLK_LCTRL);
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

enum {
	aa_screen1 = 0,
	aa_screen2 = 1,
};

int attract_automaton::rollovers[2] = {5*FPS,15*FPS};

attract_automaton::attract_automaton() : type_automaton() {
	char file[FILENAMESIZE];
	
	internalPath(file, "snap_title.obj");
	load_text(file);
	memcpy(&backscreen[0][0][0], &screen[0][0], sizeof(screen));
	
	clear();
	internalPath(file, "snap_attract.obj");
	load_text(file);
	memcpy(&backscreen[1][0][0], &screen[0][0], sizeof(screen));
	
	clear();	
	rollover = rollovers[0];
}

void attract_automaton::input(SDL_Event &event) {
	if (event.type==SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {
		nexttype(new player_screen());
//		wantClearUi = true;
//		want = WMockupMenu;
//		program_interface();
	}
}

void attract_automaton::tick() {
	frame++; // Maybe up to 0
    if (frame > rollover) {
        state = (state+1)%2;
		frame = 0;
		rollover = rollovers[state];
    }
	switch(state) {
		case aa_screen1: {
			if (frame == 0) {
				memcpy(&screen[0][0], &backscreen[state][0][0], sizeof(screen));
			}
		} break;
		case aa_screen2: {
			if (frame == 0) {
				clear();
				//				memcpy(&screen[0][0], &backscreen[state][0][0], sizeof(screen)/6);				
				for(int y = 0; y < 16; y++) {
					for(int x = 0; x < 3; x++) {
						scolor[y+4][x] = playerColor[y/4]; // Magic numbers rule?
					}
				}				
			}
#define AARF (int(FPS/1))
			if (0 == frame%(AARF)) {
				int w = frame/(AARF);
				if (w < 6) {
					memcpy(&screen[0][0], &backscreen[state][0][0], sizeof(screen)/6 * (w+1));				
				}
			}
		} break;
	}
}

enum {
	pss_present = 0,
	pss_device,
	pss_auto,
	pss_entry
};

player_screen::player_screen() : type_automaton(), target(0) {	
	char file[FILENAMESIZE];
	internalPath(file, "player_config.obj");
	load_text(file);	
	
	target = 0;
	state = 0;
	rollover = -1;
	
	for(int x = 0; x < 2; x++) {
		ready[x] = false;
		controls[x] = new control_textsetter(this, x, defaults[x].jid);  // CLUMSY!
		controls[x]->load(defaults[x]); 
		reset(x);
	}

#if !DOUBLESNAP
	set_centered(0,23,40, "Press return to continue");
#endif
	
	for(int y = 0; y < 3; y++) {
		for(int x = 0; x < 3; x++) {
			scolor[y+2][x+8] = playerColor[0];
			scolor[y+2][x+28] = playerColor[1];
		}
	}
}

void player_screen::set_boxtext(int x, int y, string s, bool inv) {
	set_centered(1+x*20, 11+y, 18, s, true, inv);
}

void player_screen::reset(int x) {
	set_boxtext(x, -4, " Controls:");
	set_control(x);
	
	string type;
	bool inst_inv = false;
	if (state == pss_present) {
		type = "Type '"; type += x?'2':'1'; type += "' to set";
	} else if (state != pss_entry && x == target) {
		type = "Please choose:";
		inst_inv = true;
	} // Implicit else: type is blanks
	set_boxtext(x, -2, type, inst_inv);
	
#if DOUBLESNAP
	set_boxtext(x, KHKEYS+3, state==pss_present?(ready[x]?"Ready":"Snap to start"):"", state==pss_present&&!ready[x]);
#endif
	
	int effective_state = state;
	if (target != x) effective_state = pss_present;
	switch(effective_state) {
		case pss_present: case pss_entry: {
			for(int y = 0; y < KHKEYS; y++) {
				set_feedback(x, y);
			}
			set_boxtext(x, 4, ""); // Two blanks in the set_feedback items
			set_boxtext(x, 9, "");
		} break;
		case pss_device: {
			int y = (joysticks.size() == 1 ? 3 : (joysticks.size() == 2 ? 1 : 0));
			for(int c = 0; c < (KHKEYS+2); c++) set_boxtext(x, c, "");
			set_boxtext(x, y+0, "(1)");
			set_boxtext(x, y+1, "Keyboard");
			for(int c = 0; c < joysticks.size() && c < 3; c++) {
				char buffer[4]; snprintf(buffer, 4, "(%d)", c+2);
				set_boxtext(x, y+(c+1)*3, buffer);
				set_boxtext(x, y+(c+1)*3+1, joysticks[c].name);				
			}
		} break;
		case pss_auto: {
			for(int c = 0; c < (KHKEYS+2); c++) set_boxtext(x, c, "");
			set_boxtext(x, 3, "(A)uto aim");
			set_boxtext(x, 4, " [\"Aim Ahead\"]");
			set_boxtext(x, 6, " (F)ree aim");
			set_boxtext(x, 7, " [\"Dual Analog\"]");
		}
	}
}

void player_screen::set_control(int x) {
	string what;
	int j = controls[x]->jid;
	if (state == pss_device && x == target) {
		// DO NOTHING
	} else if (j < 0) {
		what = "Keyboard";
	} else {
		what = joysticks[j].name;
	}
	set_boxtext(x, -3, what);
}

void player_screen::set_feedback(int x, int y) {
	if (y >= KHKEYS) return;
//	ERR("FEEDBACK! %p %d %d\n", this, x, y);
	const char *controls_mean[KHKEYS] = {"Move left", "Move up", "Move right", "Move down",
		"Fire left","Fire up","Fire right","Fire down","Snap"};
	const char *controls_mean_short[KHKEYS] = {"Move L", "Move U", "Move R", "Move D",
		"Fire L","Fire U","Fire R","Fire D","Snap"};
	string output;
	bool inv = controls[x]->state == y;
	if (controls[x]->controls[y].is_anything()) {
		inv = controls[x]->state == y;
		string readable = controls[x]->controls[y].readable();
		if (y == SPECIAL_FIRE_BUTTON && controls[x]->auto_aim)
			output = "Fire";
		else
			output = (readable.size() < 7 ? controls_mean : controls_mean_short)[y]; // Is 7 the right number?
		output += ": "; output += readable;
	} else if (inv) {
		output = "Press: ";
		output += (y == SPECIAL_FIRE_BUTTON && controls[x]->auto_aim ? "Fire" : controls_mean[y]);
	}
	set_boxtext(x, y+y/4, output, inv); // Note y/4 to put spaces after blocks
}

// Partners with control_textsetter
void player_screen::input(SDL_Event &event) {
//	if (event.type==SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) { // This whole clause going away soon?
//		nexttype(new level_automaton());
//	} else 
	if (event.type==SDL_KEYDOWN) {
		switch (state) {
			case pss_present:
				if (event.key.keysym.sym == '1' || event.key.keysym.sym == '2') {
					int w = event.key.keysym.sym - '1';
					target = w;
#if DOUBLESNAP
					ready[w] = false;
#endif
					player_controls pc;
					controls[w]->load(pc);
					if (joysticks.size()) {
						state++;
					} else {
						state += 2;
					}
					reset(0);
					reset(1);
				} else {
#if DOUBLESNAP
					for(int c = 0; c < 2; c++) { // Ad hoc, violates DRY
						if (controls[c]->controls[SNAP_BUTTON].is_key() && 
							controls[c]->controls[SNAP_BUTTON].key == event.key.keysym.sym && !ready[c]) {
							ready[c] = true;
							am_snap(1<<c,-1,-1);
							if (ready[!c]) {
								nexttype(new level_automaton());
							} else {
								reset(c);
							}
						}
					}
#endif
				}
				break;
			case pss_device:
				if (event.key.keysym.sym >= '1' || event.key.keysym.sym <= '9') {
					int w = event.key.keysym.sym - '1';
					if (w <= joysticks.size()) { // Subtle trick: 0 is keyboard
						controls[target]->jid = w-1; // So 0 goes to -1
						state++;
						reset(target);
					}
				}
				break;
			case pss_auto:
				if (event.key.keysym.sym == 'a' || event.key.keysym.sym == 'f') {
					bool w = event.key.keysym.sym == 'a';
					controls[target]->auto_aim = w;
					controls[target]->rollover = 0;
					state++;
				}
				break;
			default: break; // Formally unnecessary I guess
		}
	} else if (event.type == SDL_JOYBUTTONDOWN) {
#if DOUBLESNAP
		for(int c = 0; c < 2; c++) { // Ad hoc, violates DRY * badly *!
			if (controls[c]->jid == event.jbutton.which &&
				controls[c]->controls[SNAP_BUTTON].is_button() &&
				controls[c]->controls[SNAP_BUTTON].button == event.jbutton.button && !ready[c]) {
				 ready[c] = true;
				 am_snap(1<<c,-1,-1);
				 if (ready[!c]) {
					 nexttype(new level_automaton());
				 } else {
					 reset(c);
				 }
			}
		}
#endif				
	}
}

void player_screen::BackOut() { nexttype(new attract_automaton()); }

level_automaton::level_automaton(int initial_selected) : selected(initial_selected), last_drawn(-1), draw_off(0) {
	char file[FILENAMESIZE];
	internalPath(file, "level_select.obj");
	load_text(file);
	memcpy(&backscreen[0][0], &screen[0][0], sizeof(screen));
}

void level_automaton::input(SDL_Event &event) {
	if (event.type==SDL_KEYDOWN && event.key.keysym.sym == SDLK_UP) {
		selected = (selected + all_levels.size() - 1) % all_levels.size(); // Meh
	} else if (event.type==SDL_KEYDOWN && event.key.keysym.sym == SDLK_DOWN) {
		selected = (selected + 1) % all_levels.size();
#if MINI_TEXT
	} else if (event.type==SDL_KEYDOWN && event.key.keysym.sym == SDLK_F1) {
		save_text("ascii.obj");
#endif
	} else if (event.type==SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {
		chosen_level = selected;
		game_reset();
		die();
	}
}

void level_automaton::tick() {
	if (selected != last_drawn) {
		clear();
		memcpy(&screen[0][0], &backscreen[0][0], sizeof(screen));
		
		if (selected >= draw_off + 20) draw_off += selected - (draw_off + 20 - 1); // Only mostly sure I understand this -1
		if (selected < draw_off) draw_off += selected - draw_off;
		
		for(int c = 0; c < 20; c++) {
			const int w = c+draw_off; if (w >= all_levels.size()) break;
			const int y = c + 3, x = 1;
			string out = all_levels[w]->name.substr(0,10);
			int missing = 10 - out.size();
			for(int c = 0; c < missing; c++)
				out += " ";
			
			bool isSelected = w == selected;
			if (isSelected) {
#if 0
				set(x-1, y,'>');
				set(x+10,y,'<');
				for(int d = 0; d < out.size(); d++)
					out[d] = toupper(out[d]); // Will set later
#endif
				
				// Fill out bar to right:
#define DESCMAX 25
				char desc[DESCMAX]; int temp;
				
				temp = all_levels[w]->starthealth();
				if (temp >=0) { snprintf(desc, DESCMAX, "HEALTH: %d", temp); }
				else { snprintf(desc, DESCMAX, "HEALTH: Inf"); }
				set(13,13,desc);
				
				temp = -1;
				if (temp >=0) { snprintf(desc, DESCMAX, "AMMO:   %d", temp); }
				else { snprintf(desc, DESCMAX, "AMMO:   Inf"); }
				set(13,15,desc);
				
				temp = all_levels[w]->forever_length/all_levels[w]->snap_factor + 0.5;
				snprintf(desc, DESCMAX, "SNAP:   %d seconds", temp);
				set(13,17,desc);
				
				if (all_levels[w]->message.size()) {
					set(13,19,all_levels[w]->message.substr(0, DESCMAX));
				}
				
				temp = all_levels[w]->forever_length + 0.5;
				snprintf(desc, DESCMAX, "FOREVER: %d seconds", temp);
				set(13,21,desc);
				
				for(unsigned char p = 0; p < PREVIEW_HEIGHT; p++) {
					memcpy(&screen[2+p][13], &all_levels[w]->preview[p*PREVIEW_WIDTH], PREVIEW_WIDTH); // Notice y,x
				}
			}
			set(x,y,out,isSelected);
		}
		
		last_drawn = selected;
	}
}

void level_automaton::BackOut() { nexttype(new player_screen()); }

logo_automaton::logo_automaton() : type_automaton(), intro_char(0) {
	rollover = -1;
}

restart_automaton::restart_automaton() {
	for(int c = 0; c < 2; c++) {
		char score[6];
		snprintf(score, 8, "%c %03d", c?'N':'B', wins[c]); // So you can go up to 99,999 wins. Have fun.
		set(7+21*c, 3, score);
	}
	set(13,20,"REPLAY? (Y/N)");
}

void restart_automaton::input(SDL_Event &event) {
	if (event.type==SDL_KEYDOWN) {
		switch(event.key.keysym.sym) {
			case 'y':
				die(); // Do this in game_reset(), maybe?
				for(int c = 0; c < player_handler::all_players.size(); c++)
					player_handler::all_players[c]->die();
				player_handler::all_players.clear();				
				game_reset();
				break;
			case 'n':
				// TODO factor out a "clear_map()"
				terminate_game();
				break;
		}
	}
}

void restart_automaton::BackOut() { terminate_game(); }

failure_automaton::failure_automaton() {
	set(0,6,"Sorry, but this game requires an");
	set(0,7,"OpenGL 2.0 compatible video card.");
	set(0,9,"Press any key to quit.");
}

void failure_automaton::input(SDL_Event &event) {
	if (event.type==SDL_KEYDOWN) {
		Quit();
	}
}

void failure_automaton::BackOut() { Quit(); }

#define INTRO_CHARS 9
const static int intro_press_at[INTRO_CHARS+2] = {0, 155, 235, 286, 546, 611, 741, 871, 1027, 1274, 1375};
inline int intro_press(int x) {
    return (100 + intro_press_at[x])/TPF;
}

void logo_automaton::blit(int x, int y, int from, int len) {
	static const char *logo = "]RUN HELLO\x7F";
	for(int c = 0; c < len; c++) {
		screen[y][x+c] = logo[from+c];
	}
}

void logo_automaton::tick() {
	type_automaton::tick();
	
	memset(&screen[0][0], ' ', 80); // Don't clear everything, I guess?
	int at = (min(intro_char, INTRO_CHARS)+1);
    blit(0, 0, 0, at);
    if (int(ticks/(FPS/8))%2) {
        if (intro_char < INTRO_CHARS)
            blit(at, 0, 10, 1); // Cursor at 10th position
        else
            blit(0, 1, 10, 1);
    }
	
	if (frame > intro_press(intro_char) && !(intro_char > INTRO_CHARS+1)) { // kludge to let us run off end
        intro_char++;
    }
    
    if (intro_char > INTRO_CHARS+1) {
		nexttype(new attract_automaton());
	}
}

// Start interface stuff

enum MiscButtonType { mb_1p, mb_2p, mb_set_1p, mb_set_2p, mb_controls_return };

// Will I ever stop doing interfaces with big ugly globals like this?
int current_player = 0, current_joystick = -1;

class ChooseJoystickButton : public ControlBase { public:
	int j;
	ChooseJoystickButton(string _text, int _j) : ControlBase(_text, true), j(_j) {}
	virtual void click() {
		wantClearUi = true;
		want = WSetControlsMenu;
		current_joystick = j;
	}
};

class MiscButton : public ControlBase { public:
	MiscButtonType t;
	MiscButton(string _text, MiscButtonType _t) : ControlBase(_text, true), t(_t) {}
	virtual void click() {
		switch (t) {
			case mb_1p:
				player_count = 1;
				newtype(new level_automaton());
				wantClearUi = true;
				break;
			case mb_2p:
				player_count = 2;
				newtype(new player_screen());
				wantClearUi = true;
				break;
			case mb_set_1p: case mb_set_2p: {
				current_player = t == mb_set_2p; // 0 or 1
				wantClearUi = true;
				if (joysticks.empty()) {
					// WANT SET VIA KEYBOARD
					current_joystick = -1;
					want = WSetControlsMenu;
				} else {
					want = WJoysticksMenu;
				}
			} break;
			case mb_controls_return: {
				wantClearUi = true;
				want = WMockupMenu;
			}
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
        case WMockupMenu: {
			ContainerBase *menu = new ContainerBase(uiSpace, CMID);
			columns.push_back(menu);
			
            menu->add( new ControlBase("The Snap") );
			menu->add( new MiscButton("Play 1 player", mb_1p) );
			menu->add( new MiscButton("Play 2 player", mb_2p) );
			menu->add( new MiscButton("Set p#1 controls", mb_set_1p) );
			menu->add( new MiscButton("Set p#2 controls", mb_set_2p) );
			{ 
				ostringstream s; s << "Found " << joysticks.size() << " joysticks.";
				menu->add(new ControlBase(s.str())); 
			}
			
            menu->commit();
		} break;
		case WJoysticksMenu: {
			ContainerBase *menu = new ContainerBase(uiSpace, CMID);
			columns.push_back(menu);
			
			{ 
				ostringstream s; s << "Player " << (current_player+1) << " should use:";
				menu->add(new ControlBase(s.str())); 
			}
			menu->add(new ChooseJoystickButton("Keyboard", -1));
			for(int c = 0; c < joysticks.size(); c++)
				menu->add(new ChooseJoystickButton(joysticks[c].name, c));
            
            menu->commit();			
		} break;
		case WSetControlsMenu: {
			control_guisetter *controls = new control_guisetter(current_player, current_joystick);
			ContainerBase *menu = new SquishContainer(uiSpace, CMID);
			columns.push_back(menu);
			
			{ 
				ostringstream s; s << "Player " << (current_player+1);
				menu->add(new ControlBase(s.str())); 
			}
			menu->add(new ControlBase(current_joystick < 0 ? "Keyboard" : joysticks[current_joystick].name )); 
			
			menu->add(new ControlBase());
			
			for(int c = 0; c < KHKEYS; c++) {
				ControlBase *blank = new ControlBase("");
				controls->feedback[c] = blank;
				menu->add(blank);
			}
            
			menu->add(new ControlBase());
			
			{ 
				ControlBase *back = new MiscButton("Back", mb_controls_return);
				back->hidden = true;
				controls->feedbackAfterwards = back;
				menu->add(back);
			}
			
            menu->commit();						
			controls->push();
		} break;
			
		case WDebugBlur: {
			ContainerBase *col = new ContainerBase(uiSpace, CRIGHT);
			columns.push_back(col);
			for(int c = 0; c < 4; c++)
				col->add( new ControlBase() );
			col->add( new DoubleBound(&debug_floats[0] , -1000,1000,1 ) );	
			col->commit();			
		} break;
	}
	
	want = wantAfterwards;
}