#include "text_display.h"
#include "text_ent.h"
#include <ctype.h>

switcher *muted = NULL;
switcher *boris = NULL;

// Realistically 1 is top 22 is bottom
struct rainbow_ent : public scroller_ent {
	int hue;
	rainbow_ent(bool _xcenter = true) : scroller_ent(_xcenter), hue(0) {}
	void display(drawing *d) {
		float x = xcenter ? 0 : -1/aspect;
		int c = 1;
		int drawn = 0;
		for(list<string>::iterator i = lines.begin(); i != lines.end(); i++) {
			uint32_t rgba = packHsv(fmod((double)(hue*5+drawn*60),360.0), 1, 1);
			jcImmediateColorWord(rgba);	// For debug
			float y = hCache*c;
			drawText(*i, x, 1-y, 0, xcenter, false);
			c++;
			if (!(*i).empty()) {
				drawn++;
				if (ticks < drawn*20)
					break;
			}
		}
		hue++;
		ent::display(d);
	}
};

struct attract : public bouncer {	
	void inserting() {
		scroller_ent *console = new rainbow_ent();
		console->insert(this);
		
		console->pushLine("~~ Scrunch ~~");
		console->pushLine("");
		if (global_mapper->found_controllers) {
			console->pushLine("Control with left analog stick");
			console->pushLine(JOYDONE);
			console->pushLine("");
			console->pushLine("Press start now");
		} else {
			console->pushLine("Control with arrow keys");
			console->pushLine(KBDDONE);
			console->pushLine("");
			console->pushLine("Press space bar now");
		}
		console->vcenter();
		
		if (!muted) {
			muted = new switcher(INPUTCODE(G_GAM, IGAM_MUTE), true);
			muted->insert();
		}
		if (!boris) {
			boris = new switcher(INPUTCODE(G_GAM, IGAM_DEATHMETAL), true);
			boris->insert();
		}
	}
	
	void input(InputData *data) {
		if (data->inputcode == INPUTCODE(G_GAM, IGAM_COMPLETE)) {
			kick();
		}
		bouncer::input(data);
	}
};