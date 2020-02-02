float centerOff(const char *str);

#include "text_display.h"
#include "text_ent.h"
#include <ctype.h>
#include "input_mapping.h"

#define ALWAYS_RANDOMIZE 0

#define SEPARATOR "----- ----- ----- -----"

struct input_collect_ent : public input_vacuum_ent {
	scroller_ent *console;
	input_collect_ent() : input_vacuum_ent(), console(new scroller_ent()) {
	}
	void insert(ent *_parent, int _prio) {
		input_vacuum_ent::insert(_parent, _prio);
		
		console->insert(this);
		console->pushLine("Hi! This is Andi. You have accessed the SECRET CONTROLLER TEST MODE. Here is what I need you to do. Take a screenshot of this:");
		
		{ ostringstream o;
			o << "\"" << SDL_JoystickName(0) << "\"";
		console->pushLine(o.str()); }
		
		console->pushLine("Then I need you to hit every button on your gamepad, one by one. If your gamepad has analog sticks, push each analog stick first up, then left.");
		console->pushLine("Every time you push a button or tilt a analog stick, a line will be printed out here. Write down which button you pressed, and write down the line here.");
		console->pushLine("Then email all of this to me at andi.m.mcclure@gmail.com. (Don't forget to mention your operating system.)");
		console->pushLine("Sorry if this is a little obnoxious! But this will help me get up and running faster. Someday there will be a real control select screen.");
	}
	void input(InputData *data) {
		if (!(data->kind == InputKindTouch || data->kind == InputKindKeyboard)
			&& data->axiscode & AXISCODE_RISE_MASK) {
			console->pushLine(SEPARATOR);
			console->pushLine(data->debugString());
		} else if (data->kind == InputKindKeyboard && data->key.keysym.sym == SDLK_ESCAPE) {
			::Quit();
		}
		ent::input(data);
	}
};

struct flickering : public ent {
	ent *canvas; // Canvas should exist from start so pre-inserting works
	target_ent *target;
	float shader1;
	flickering() : ent(), canvas(new ent()), target(NULL), shader1(0) {}
	
	void inserting() {
		filter *f = new filter(canvas, get_attract_filter());
		f->insert(this);
		target = f->target[0];
	}
	
	void tick() {
		shader1 = ticks;
		
		ent::tick();
	}
	
	stateoid *get_attract_filter(); // This is really silly!
};

// HELP SCREEN

struct help : public ent {
	scroller_ent *console;
	help() : ent(), console(NULL) {}
	
	void inserting() {
		flickering *flicker = new flickering();
		flicker->insert(this);
		
		console = new capital_scroller_ent();
		console->insert(flicker->canvas);
		ostringstream o;

		o << "ICOSA\n\nThis is a video feedback system.\n"
			<< "Imagine two mirrors facing each other at a hair salon, reflecting each other. Now imagine one of the two mirrors is shattered. That's this program.\n\n";
			
		if (!global_mapper->hasController()) {
			o << "To move:\n"
			<< "Use " << global_mapper->label(SDL_SCANCODE_W) << global_mapper->label(SDL_SCANCODE_A) << global_mapper->label(SDL_SCANCODE_S) << global_mapper->label(SDL_SCANCODE_D)
			<< " to move forward, back, left and right. Use the mouse to look around.\n"
			<< "Use " << global_mapper->label(SDL_SCANCODE_E) << " and " << global_mapper->label(SDL_SCANCODE_C) << " to move up and down, "
			<< global_mapper->label(SDL_SCANCODE_Q) << " and " << global_mapper->label(SDL_SCANCODE_Z) << " to tilt left and right.\n"
			<< "Use " << global_mapper->label(SDL_SCANCODE_X) << " to turn around quickly.\n\n"
			<< "To control:\n"
			<< global_mapper->label(SDL_SCANCODE_I) << " and " << global_mapper->label(SDL_SCANCODE_P) << " manipulate the mirror. "
			<< global_mapper->label(SDL_SCANCODE_K) << " " << global_mapper->label(SDL_SCANCODE_L) << " and " << global_mapper->label(SDL_SCANCODE_SEMICOLON) << " manipulate shapes.\n"
			<< global_mapper->label(SDL_SCANCODE_RIGHTBRACKET) << " hides everything except the mirror. " << global_mapper->label(SDL_SCANCODE_BACKSLASH) << " \"freezes\" the mirror; if the mirror is frozen, right-arrow moves it forward one frame.\n"
			<< "Don't worry, you'll figure it out.\n"
			<< "Enter saves a screenshot.\n"
			<< "Take lots of screenshots. Then send them to me. I wanna see <3\n\n"
			<< "Number 0 resets the camera. Other numbers let you save, then recall, camera positions.\n\n"
			<< "Press H to dismiss this screen.";
		} else {
			o << "To move:\n"
			<< "Use " << global_mapper->label("leftx")
			<< " to move. Use " << global_mapper->label("rightx") << " to look around.\n"
			<< "Use " << global_mapper->label("leftshoulder") << " and " << global_mapper->label("rightshoulder") << " to move up and down, "
			<< global_mapper->label("lefttrigger") << " and " << global_mapper->label("righttrigger") << " to tilt left and right.\n"
			<< "Use " << global_mapper->label("dpdown") << " to turn around quickly and " << global_mapper->label("dpleft") << " to reset the camera.\n\n"
			<< "To control:\n"
			<< global_mapper->label("x") << " and " << global_mapper->label("y") << " manipulate the mirror. "
			<< global_mapper->label("a") << ", " << global_mapper->label("b") << ", and " << global_mapper->label("dpup") << " manipulate shapes.\n"
			<< global_mapper->label("back") << " hides everything except the mirror. " << global_mapper->label("start") << " \"freezes\" the mirror; if the mirror is frozen, " << global_mapper->label("dpright") << " moves it forward one frame.\n"
			<< "Don't worry, you'll figure it out.\n"
			<< global_mapper->label("guide") << " saves a screenshot.\n"
			<< "Take lots of screenshots. Then send them to me. I wanna see <3\n\n"
			<< "Press H to dismiss this screen.";
		}
		console->pushLine(o.str());
		console->vcenter();
	}
};

// SETTINGS SCREEN

struct setting_ent : public text_ent {
	vector<string> options;
	int base;
	int &target;
	char trigger;
	
	setting_ent(char _trigger, int &_target, int _base=0) : text_ent(),
		base(_base), target(_target), trigger(_trigger) {}
	
	string text() {
		int idx = target - base;
		if (idx < options.size())
			return options[idx];
		return "[INTERNAL ERROR]";
	}
	
	void input(InputData *data) {
		if (data->inputcode != INPUTCODE(G_TITLE, IT_MENUKEY)) return;
		if (data->key.keysym.sym == trigger) {
			int idx = (target - base + 1) % options.size(); // Iterate
			target = idx + base;
		}
	}
	
	virtual void randomize() {
		int idx = random() % options.size();
		target = idx + base;
	}
};

// Final option statistically disadvantaged
struct face_setting_ent : public setting_ent {
	face_setting_ent(char _trigger, int &_target, int _base=0) : setting_ent(_trigger, _target, _base) {}
	void randomize() {
		int idx = random() % options.size();
		if (idx == 2 && (random()%2))
			idx = random()%(options.size()-1);
		target = idx + base;
	}
};

// Hide text unless menu on
struct mirror_sub_setting_ent : public setting_ent {
	mirror_sub_setting_ent(char _trigger, int &_target, int _base=0) : setting_ent(_trigger, _target, _base) {}
	string text() {
		if (mirrormode == 0)
			return "";
		return setting_ent::text();
	}
};

// Hide text unless menu on (#2)
struct mirror_sub_setting_ent_2 : public mirror_sub_setting_ent {
	mirror_sub_setting_ent_2(char _trigger, int &_target, int _base=0) : mirror_sub_setting_ent(_trigger, _target, _base) {}
	string text() {
		if (mirrormode == 1)
			return "";
		return mirror_sub_setting_ent::text();
	}
};

// Shows line heights
struct attract_test : public ent {
	void inserting() {
		float x = textHeight();
		int max = 2/x;
		for(int c = 0; c < max; c++) {
			ostringstream o;
			o << "Line " << c << " of " << max;
			(new string_ent(o.str(), 0, 1-(c+1)*x, true))->insert(this);
		}
	}
};

// Realistically 1 is top 22 is bottom
struct attract : public expiring_ent {
	vector<setting_ent *> settings;
	ent *next_ent;
	help *help_screen;
	flickering *flicker;
		
	float hCache;
	attract() : expiring_ent(), next_ent(NULL), help_screen(NULL), flicker(NULL) {
		hCache = textHeight();
	}

	float line(int c) { return 1-(c+1)*hCache; }

	void input(InputData *data) {
		if (data->inputcode != INPUTCODE(G_TITLE, IT_MENUKEY)) return;
		
		if (help_screen) { // Help screen devours keypresses
			switch (data->key.keysym.sym) {
				case 'h': case SDLK_ESCAPE:
					help_screen->die();
					help_screen = NULL;
					break;
			}
			return;
		}
		
		switch (data->key.keysym.sym) {
			case ' ':
				next_ent = get_game_filter();
				expire();
				return;
			case 'j':
				next_ent = new input_collect_ent();
				expire();
				return;
			case 'h':
				help_screen = new help();
				help_screen->insert(this);
				return;
			case 'r':
				randomize();
				return;
			case SDLK_ESCAPE:
				::Quit();
				return;
		}
		
		ent::input(data); // Notice: Returns are above, so sometimes this short-circuits.
	}

	void display(drawing *d) { // Help screen devours light
		if (help_screen)
			help_screen->display(d);
		else
			ent::display(d);
	}

	void die() {
		ent::die();
		if (next_ent)
			next_ent->insert();
	}
	
	ent *get_game_filter(); // This is also really silly!

	void inserting() {
		flicker = new flickering();
		flicker->insert(this);

		(new string_ent("ISOCA", 0, line(2), true))->insert(flicker->canvas);
		(new string_ent("PRESS (R) TO RANDOMIZE SETTINGS" /* " PRESS (H) FOR HELP" */, 0, line(4), true))->insert(flicker->canvas);
		if (global_mapper->hasController()) {
			ostringstream o;
			o << "CONTROL WITH " << str_toupper(global_mapper->firstController) << ". PRESS (H) NOW FOR HELP";
			(new string_ent(o.str(), 0, line(5), true))->insert(flicker->canvas);
		} else {
			(new string_ent("CONTROL WITH KEYBOARD/MOUSE. PRESS (H) NOW FOR HELP", 0, line(5), true))->insert(flicker->canvas);
		}
		
		setting_ent *tmp;
		
		tmp = new face_setting_ent('f', facemode, 0);
		tmp->options.push_back("(F)ACES ARE: SMOOTH");
		tmp->options.push_back("(F)ACES ARE: TILED ");
		tmp->options.push_back("(F)ACES ARE: WIRES ");
		settings.push_back(tmp);
		
		tmp = new setting_ent('c', colormode, 0);
		tmp->options.push_back("(C)OLORS ARE: RANDOM             ");
		tmp->options.push_back("(C)OLORS ARE: RANDOM OR BLACK    ");
		tmp->options.push_back("(C)OLORS ARE: FACE-AWARE         ");
		tmp->options.push_back("(C)OLORS ARE: FACE-AWARE OR BLACK");
		settings.push_back(tmp);
		
		tmp = new setting_ent('k', mutatemode, 0);
		tmp->options.push_back("(K) AND ; KEYS: MUTATE INNER");
		tmp->options.push_back("(K) AND ; KEYS: MUTATE OUTER");
		settings.push_back(tmp);
		
		tmp = new setting_ent('m', mirrormode, 0);
		tmp->options.push_back("(M)IRROR: NONE             ");
		tmp->options.push_back("(M)IRROR: FLAT SCREEN      ");
		tmp->options.push_back("(M)IRROR: INNER ISOCAHEDRON");
		tmp->options.push_back("(M)IRROR: OUTER ISOCAHEDRON");
		settings.push_back(tmp);
		
		tmp = new mirror_sub_setting_ent('p', filtermode, 0);
		tmp->options.push_back("(P) AND I KEYS: MIRROR INTERLACE");
		tmp->options.push_back("(P) AND I KEYS: MIRROR BRIGHTNESS");
		tmp->options.push_back("(P) AND I KEYS: MIRROR BEND");
		tmp->options.push_back("(P) AND I KEYS: MIRROR CHROMATICS");
		tmp->options.push_back("(P) AND I KEYS: MIRROR FLICKER");
		settings.push_back(tmp);
		
		tmp = new mirror_sub_setting_ent_2('s', mirrorskinmode, 1);
		tmp->options.push_back("MIRROR (S)KIN: RANDOM        ");
		tmp->options.push_back("MIRROR (S)KIN: X COORDINATE  ");
		tmp->options.push_back("MIRROR (S)KIN: XY COORDINATES");
		settings.push_back(tmp);
		
#if 0
		tmp = new mirror_sub_setting_ent('l', mirrorcolormode, 0);
		tmp->options.push_back("MIRROR CO(L)OR: OFF");
		tmp->options.push_back("MIRROR CO(L)OR: ON ");
		settings.push_back(tmp);
	
		tmp = new setting_ent('a', spinmode, 0);
		tmp->options.push_back("(A)UTOSPIN: OFF              ");
		tmp->options.push_back("(A)UTOSPIN: INNER ISOCAHEDRON");
		tmp->options.push_back("(A)UTOSPIN: OUTER ISOCAHEDRON");
		settings.push_back(tmp);
#endif

		float offmax = 0;
		for(int c = 0; c < settings.size(); c++) {
			float off = textWidth(settings[c]->options[0]);
			if (fabs(offmax) < fabs(off))
				offmax = off;
		}
		offmax *= -0.5;

#if ALWAYS_RANDOMIZE
		randomize();
#endif

		int miny = 7 + (21 - 7 - settings.size())/2;
		for(int c = 0; c < settings.size(); c++) {
			settings[c]->x = offmax;
			settings[c]->y = line(miny+c);
			settings[c]->insert(flicker->canvas);
		}
		
		(new string_ent("[[SPACE BAR]] TO INITIATE SESSION [[ESC]] TO RETURN", 0, line(21), true))->insert(flicker->canvas);
		(new string_ent("THERE ARE MANY OPTIONS SO IF YOU'RE CONFUSED JUST PRETEND THIS IS AN FPS", 0, line(22), true))->insert(flicker->canvas);		
	}
	
	void randomize() {
		for(int c = 0; c < settings.size(); c++) {
			settings[c]->randomize();
		}
	}
};