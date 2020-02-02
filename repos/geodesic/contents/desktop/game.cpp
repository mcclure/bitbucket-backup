/*
 *  game.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 10/27/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "kludge.h"
#include "game.h"
#include "display_ent.h"
#include "test_ent.h"
#include "program.h"
#include "glCommon.h"
#include "postprocess.h"
#include "chipmunk_ent.h"
#include "input_ent.h"
#include "basement.h"
#include "inputcodes.h"
#include "display_ent.h"
#include "glCommonMatrix.h"
#include "util_pile.h"
#include "plaidext.h"

#include <deque>

#define S 10.0

#define GREEN 0xFF00FF00
#define CYAN  0xFFFFFF00
#define BLUE  0xFFFF0000
#define CAMTEST 1

// Assuming 60 FPS
#define INPUT_EVERY 0
#define ROTATE_EVERY 5
#define NOTE_EVERY 15
#define ROTATE_STEPS 9
#define QUEUE_LIMIT 2
#define F_WORLD_VERSION 0x1
#define WORLD_DAT "world.obj"
#define ANI_LENGTH 40.0
#define HOLEAT (1 - 10.0/ANI_LENGTH)
#define HOLEOUT 30

struct runner;

float justSign(float f) {
	return f > 0 ? 1 : -1;
}

float inter(float v, float start, float end, float low, float high) {
	if (v < start) return low;
	if (v > end) return high;
	return low + (v-start)/(end-start) * (high-low);
}

// Settings

int facemode = 0;
int colormode = 0;
int mirrormode = 0;
int mirrorcolormode = 0;
int mirrorskinmode = 1;
int mutatemode = 1;
int filtermode = 1;
int spinmode = 0;

#include "attract_incl.cpp"
#include "color_incl.cpp"
		
// ---------- REALITY ----------

#define AVGCOUNT 30
struct averager {
	cpVect track[AVGCOUNT];
	int at;
	averager() : at(0) {
		ZERO(track);
	}
	void write(cpVect in) {
		track[at] = in; at++; at %= AVGCOUNT;
	}
	cpVect read() {
		cpVect result = cpvzero;
		for(int c = 0; c < AVGCOUNT; c++) {
			result = result + track[c];
		}
		return result;
	}
	void writeAng(cpFloat in) {
		write(cpvforangle(in));
	}
	cpFloat readAng() {
		return cpvtoangle(read());
	}
	cpFloat cycleAng(cpFloat in) {
		writeAng(in);
		return readAng();
	}
};

const float tet_to_tex(float tet) {
	return ((tet/PHI)+1)/2;
}

struct camera_memory {
	bool present;
	cpVect sumPos;
	glm::vec3 camPos;
	float roll;
	camera_memory() : present(false) {} 
};

// Put everything here
#define F_POINTSPAN (0.5*S)
#define F_POINTSIDE 60
#define F_Y			(-S)
const static cpVect flow = cpv(1,1/2.0);
struct cubescene : public ent {
	pile *dome_inner, *dome_outer, *mirror;
	texture_slice *mirrorTexture;
	
	cpVect sumPos; // MOUSE ANGLE
	glm::vec3 camPos; // CAM XYZ
	float roll;
	glm::mat4 innerspin;
	
	cubescene(texture_slice *_mirrorTexture) : dome_inner(NULL), dome_outer(NULL),
		sumPos(cpvzero), camPos(0.0,0.0,-6.25), mirror(NULL), mirrorTexture(_mirrorTexture), roll(0.0) {}
	
	pile *spin(pile *input, int formode) {
		if (spinmode == formode) {
			pile *p = new pile(element_plain::triangles);
			*p = *input;
			pile_transform_apply(*p, innerspin);
			return p;
		}
		return input;
	}
	
	void display(drawing *d) {
		glEnable(GL_DEPTH_TEST); // LEAKY
		
		// First ocean 
		
		if (dome_inner) d->insert_pile(D_FIELD, spin(dome_inner, 1));
		if (dome_outer) d->insert_pile(D_BOAT,  spin(dome_outer, 2));
		if (mirror) {
			mirror->texture = mirrorTexture->texture;
			pile *newmirror = spin(mirror, 1);
			d->insert_pile(D_MIRROR, newmirror);
		}
		
		// Then player (needs ocean heightmap)
				
		d->mvp = glm::perspective(60.0f, float(1.0/aspect), 0.5f, 5*30.0f)
		 * forwardMatrix()
		 * glm::translate(glm::mat4(1), camPos);
		 
		ent::display(d);
	}
	
	glm::mat4 forwardMatrix() {
		return glm::eulerAngleXY<float>( sumPos.y, sumPos.x ) * glm::eulerAngleZ( roll );
	}
};

struct runner;

struct runner : public cubescene {
	// State
//	sonar *sound;
	
	// Interface
	bool havePos;
	cpVect lastPos;

	camera_memory numpad[10];
	sticker *stick_x, *stick_y, *stick_z, *stick_roll, *stick_mutate, *stick_pan_x, *stick_pan_y;

	// Mutation
	pile *mutate_target;
	pile *mutate_backup;
	float mutate_strength;
	vector<float> mutate_coefficients;
	
	
	runner(texture_slice *_mirrorTexture) : cubescene(_mirrorTexture),
		havePos(false), lastPos(cpvzero), mutate_target(NULL), mutate_backup(NULL), mutate_strength(0)
		, stick_x(NULL), stick_z(NULL)
	  {
		//sound = new sonar(this);
				
		stick_x = new sticker(INPUTCODE(G_DBG, IDBG_MOVE_X)); stick_x->insert(this);
		stick_y = new sticker(INPUTCODE(G_DBG, IDBG_MOVE_Y)); stick_y->insert(this);
		stick_z = new sticker(INPUTCODE(G_DBG, IDBG_MOVE_Z)); stick_z->insert(this);
		stick_roll = new sticker(INPUTCODE(G_DBG, IDBG_ROLL)); stick_roll->insert(this);
		stick_mutate = new sticker(INPUTCODE(G_DBG, IDBG_MUTATE_STRENGTH)); stick_mutate->insert(this);
		stick_pan_x = new sticker(INPUTCODE(G_DBG, IDBG_PAN_X)); stick_pan_x->insert(this);
		stick_pan_y = new sticker(INPUTCODE(G_DBG, IDBG_PAN_Y)); stick_pan_y->insert(this);
		
		// Define world
		
		// Are we using wireframes!!!!!
		element_free *domedraw = NULL;
		element_free *domewire = NULL;
		element_free *domesolid = NULL;
		state_doubled *domestate = NULL;
		
		// Facemode consumption scoped here		
		if (facemode == 2) { // If wire
			element_free *domeblack = new element_free();
			domedraw = new element_free(GL_LINES);
			
			domestate = new state_doubled(domeblack, domedraw);	
						
			domewire = domedraw;
			domesolid = domeblack;
		} else {
			domedraw = new element_free();
			domesolid = domedraw;
		}
		
		// The one part that never changes: Make a pile
		dome_inner = new pile(domedraw, true, false, true, 0, GREEN);
		dome_inner->immortal = true;
		if (domestate) dome_inner->stateSetter = domestate; // Comment out to prevent blackout
		
		// Populate tetrahedron
		pile tet(domesolid, true, false, false);
		pile_tetrahedron(tet, domesolid, domewire);
		
		if (facemode == 1) { // If solid-face
			pile_unpack(*dome_inner, tet, *domesolid);
			dome_inner->elementDrawer = element_plain::triangles;
		} else {
			pile_push_all(*dome_inner, tet);
		}
		
		// Set the color
		color_random random;
		color_sometimes sometimes(&random);
		color_tetrahedron tetrahedron(&random);
		color_tetrahedron sometimes_tetrahedron(&sometimes);
		color_triplet triplet(&random);
		color_triplet_sometimes triplet_sometimes(&sometimes);
		
		coloroid *color = &random;
		
		// Colormode consumption scoped here
		
		switch(colormode) {
			case 1: color = &sometimes; break;
			case 2: {
				if (facemode != 1)
					color = &tetrahedron;
				else
					color = &triplet;
			} break;
			case 3: {
				if (facemode != 1)
					color = &sometimes_tetrahedron;
				else
					color = &triplet_sometimes;
			} break;
		};
		
		pile_recoloroid(*dome_inner, color);

		// Mirrormode consumption scoped here -- only if mirrorTexture

		// Copy to outer shell
		dome_outer = new pile(NULL);
		*dome_outer = *dome_inner;
		dome_outer->baseColor = CYAN;
				
		glm::mat4 rotate = glm::eulerAngleXY(180.f,180.f);
		glm::mat4 scale = glm::scale(glm::mat4(), glm::vec3(10.0));
		pile_transform_apply(*dome_outer, rotate*scale);
		
		// Mirror?
		if (mirrormode == 1) {
			cpVect half = cpv(1.0/aspect, 1);
			quadpile3 *_mirror = new quadpile3(true, false);
			_mirror->xyplane(cpvneg(half), half, 0);
			
			mirror = _mirror;
			delete dome_inner;
			dome_inner = NULL;
		} else if (mirrormode == 2 || mirrormode == 3) {
			pile *&	replace = mirrormode == 2 ? dome_inner : dome_outer;
		
			// mirrorcolormode usage scoped here
		
			mirror = new pile(replace->elementDrawer, true, true, mirrorcolormode);
			pile_push_all(*mirror, *replace);
						
			// mirrorskinmode usage scoped here
			
			int count = mirror->vertices();
			for(int c = 0; c < count; c++) {
				float x, y;
				if (mirrorskinmode == 2) {
					int tet_at = c * tet.stride();
					x = tet_to_tex( tet[tet_at]   );
					y = tet_to_tex( tet[tet_at]+1 ); // What on earth??
				} else if (mirrorskinmode == 3) {
					int tet_at = c * tet.stride();
					x = tet_to_tex( tet[tet_at]   );
					y = tet_to_tex( tet[tet_at+1] );
				} else {
					x = ::random()/float(RANDOM_MAX);
					y = ::random()/float(RANDOM_MAX);
				} 
				pile_rewrap(*mirror, c, x, y);
			}
						
			delete replace;
			replace = NULL;
		}
		if (mirror) mirror->immortal = true;
		
		// mutatemode usage scoped here

		mutate_target = mutatemode == 1 ? (dome_outer ? dome_outer : mirror) : dome_inner	;
		if (mutate_target) {
			mutate_backup = new pile(NULL); // Never inserted so doesn't have to be immortal
			*mutate_backup = *mutate_target;
			mutate_coefficients.resize( mutate_backup->vertices() );
		}
		mutateReset();
		
		doMemory(0);
		
//		sound->insert(this);
	}
	
	void input(InputData *data) {
		switch (data->inputcode) {
			case INPUTCODE(G_G, I_MOUSE): {
				if (data->touchkind == touch_move) {
					if (havePos) {
						cpVect offset = cpvsub(data->touch.at, lastPos);
						offset = cpvmult(offset, M_PI/180.0);
						offset.x *= -1;
						sumPos = cpvadd(sumPos, offset);
#if 0
#define MOUSELIMIT M_PI
#else
#define MOUSELIMIT 1
#endif
						sumPos.y = max<cpFloat>(-MOUSELIMIT, sumPos.y);
						sumPos.y = min<cpFloat>( MOUSELIMIT, sumPos.y);
					}
					lastPos = data->touch.at;
					havePos = true;
		//			ERR("MOUSE %s sum %lf, %lf\n", data->debugString().c_str(), (double)sumPos.x/M_PI, (double)sumPos.y/M_PI);
				}
			} break;
			
			case INPUTCODE(G_DBG, IDBG_MUTATE_RESET): {
				mutateReset();
				mutateUpdate();
			} break;
			
			case INPUTCODE(G_DBG, IDBG_FLIP): {
				sumPos.x += M_PI;
			} break;
			
			case INPUTCODE(G_DBG, IDBG_NUMBER): {
				int key = 0; //data->kind == InputKindKeyboard ? data->key.keysym.unicode : '0'; // FIXME!!!
				if (key >= '0' && key <= '9') {
					doMemory(key - '0');
				}
			} break;
		}
		
		ent::input(data);
	}
	
	void tick() {
		roll += stick_roll->stick() * (M_PI / 45);
		if (stick_mutate->stick()) {
			mutate_strength += stick_mutate->stick()*0.01;
			mutateUpdate();
		}

		camPos += glm::vec3(0.25) * glm_apply(glm::vec3(stick_x->stick(), stick_y->stick(), stick_z->stick()), forwardMatrix());
		
		sumPos.x += stick_pan_x->stick() * (M_PI / 45);
		sumPos.y += stick_pan_y->stick() * (M_PI / 45);

#if 0 // LIMITS
		const float len = glm::length(camPos);
		const float max = 15.25;
		if (len > max) camPos *= glm::vec3(max/len);
#endif

		ent::tick();
	}
	
	void doMemory(int idx) {
			camera_memory &memory = numpad[idx];
			if (memory.present) {		// Recall memory
				sumPos = memory.sumPos;
				camPos = memory.camPos;
				roll = memory.roll;
//				memory.present = false; // Forget memory?
			} else {					// Save memory
				memory.sumPos = sumPos;
				memory.camPos = camPos;
				memory.roll = roll;
				memory.present = true;
			}
	}
	
	void mutateReset() {
		for (int c = 0; c < mutate_coefficients.size(); c++)
			mutate_coefficients[c] = random()/float(RANDOM_MAX) * 2 - 1;
	}
	
	void mutateUpdate() {
		if (mutate_target) {
			int count = mutate_coefficients.size();
			int stride = mutate_target->stride();
			for(int c = 0; c < count; c++) {
				float *point_dst = &(*mutate_target)[c*stride];
				float *point_src = &(*mutate_backup)[c*stride];
				float mutate = 1 + mutate_coefficients[c]* mutate_strength;
				point_dst[0] = point_src[0] * mutate;
				point_dst[1] = point_src[1] * mutate;
				point_dst[2] = point_src[2] * mutate;
			}
		}
	}
};

template <typename T>
struct specialstate : public state_common {
	T *ctx; int v;
	specialstate(T *_ctx, int _v) : ctx(_ctx), v(_v) {}	
	void set(pile &q, drawing *d) {
		int which = v;
		Special(which);
		state_common::set(q, d);
		texture_slice *s = ctx->target->display_texture();
		glUniform1f(p->uniforms[s_px], 1.0/s->twidth);
		glUniform1f(p->uniforms[s_py], 1.0/s->theight);
		glUniform1f(p->uniforms[s_brightness], ctx->shader1);

		GLERR("Uniforms");
	}
};

struct game_filter : public ent {
	source_kvm *toggle;
	alternator *target;
	runner *scene;
	cubescene *echo;
	screenshotter *snap;
	sticker *stick_shader1; float shader1;
	
	bool paused, hidden;
	texture_slice *toggle_states[2];
	
	game_filter() : ent(), toggle(NULL), target(NULL), scene(NULL), echo(NULL), shader1(1), paused(false), hidden(false) {
	}

	void inserting() {
		float fsaa = 1;
		cheatfile_load(fsaa, "fsaa.txt");
	
		// The basis of all this is an alternator
		texture_slice *drawInto = new texture_slice();
		drawInto->init(surfacew*fsaa, surfaceh*fsaa);
		testcross(drawInto);
		drawInto->construct();

		texture_slice *drawFrom = new texture_slice();
		drawFrom->init(surfacew*fsaa, surfaceh*fsaa);
		testfill(drawFrom, BLACK);
		drawFrom->construct();
		
		target = new alternator(drawInto, true, drawFrom, true, NULL);
		
		target->insert(this, ENT_PRIO_FBTARGET);

		// The alternator's output texture is made the skin of an isocahedron
		texture_slice *targetTexture = target->display_texture();
		scene = new runner(targetTexture);
		scene->insert(target);
		
		// We'll also want to display it. But sometimes we want to display an isoca-less view.
		// A source_kvm will switch the two views:
		toggle = new source_kvm();
		
		splatter *kid = new splatter(toggle, true);
		kid->insert(this);
		toggle->addTarget(targetTexture);
				
		// For the isoca-less view to appear in the kvm, it needs a render pass
		texture_slice *drawEcho = new texture_slice();
		drawEcho->init(surfacew*fsaa, surfaceh*fsaa);
		testfill(drawEcho, BLACK);
		drawEcho->construct();
		target_ent *echoPass = new target_ent(drawEcho, true);
		echoPass->insert(this, ENT_PRIO_FBTARGET);
		
		toggle->addTarget(drawEcho);
		
		// This is the invisible-isoca view itself
		echo = new cubescene(targetTexture);
		echo->mirror = scene->mirror; // Whatever it may be
		echo->insert(echoPass);
		
		// The alternator might need a filter.		
		// filtermode usage scoped here
		shader1 = filtermode > 1 ? 0 : 1; // 1 for "old" shaders 0 for "new".
		if (filtermode >= 0 && scene->mirror) { // I guess filtermode -1 can be the "no filter" code.
			scene->mirror->stateSetter = new specialstate<game_filter>(this, filtermode);
			echo->mirror->stateSetter = scene->mirror->stateSetter;
		}
		
		// If it has a filter, this will control the filter parameter.
		stick_shader1 = new sticker(INPUTCODE(G_DBG, IDBG_SHADER1)); stick_shader1->insert(this);
		
		// Finally we want the ability to take screenshots of all this.
		(new screenshotter(INPUTCODE(G_DBG, IDBG_SNAPSHOT), toggle))->insert(this);
	}
	
	void setToggleTo(int v) {
		if (toggle->focus != v)
			toggle->nextTarget();
	}
	
	void adjustPauseHide() {
		bool doHide = paused || hidden;
		bool doPause = paused;
		setToggleTo(doHide);
		target->lifetime = doPause ? 0 : -1;
	}
	
	void input(InputData *data) {
		switch (data->inputcode) {			
			case INPUTCODE(G_DBG, IDBG_PAUSE): {
				if (!mirrormode) break; // This only involves the mirror!
				paused = !paused;
				adjustPauseHide();
			} break;
			
			case INPUTCODE(G_DBG, IDBG_STEP): {
				if (!mirrormode) break; // This only involves the mirror!
				if (!target->lifetime)
					target->lifetime = 1;
			} break;
			
			case INPUTCODE(G_DBG, IDBG_HIDE): {
				if (!mirrormode) break; // This only involves the mirror!
				if (paused && !hidden) { // If paused, treat implicit hidden as in effect.
					paused = false;
				} else {
					hidden = !hidden;
				}
				adjustPauseHide();
			} break;
			
			case INPUTCODE(G_G, I_BACKOUT): {
				die();
				(new attract())->insert(); // THIS IS THE WRONG PLACE!
			} break;
		}
		
		ent::input(data);
	}

	void display(drawing *d) {
		echo->sumPos = scene->sumPos;
		echo->camPos = scene->camPos;
		echo->roll = scene->roll;
		 
		ent::display(d);
	}

	void tick() {
		shader1 += stick_shader1->stick() * 1.0/256;
		
		if (spinmode) {
			float spinxmode = .005, spinymode = .01 ;
//			cheatfile_load(spinxmode, "spinx.txt");
//			cheatfile_load(spinymode, "spiny.txt");
			scene->innerspin = scene->innerspin * glm::eulerAngleXY<float>(spinxmode, spinymode);
			echo->innerspin = scene->innerspin;
		}
		
		ent::tick();
	}
};

// Draw the scene with the shader filter on top, instead of "inside"
ent *attract::get_game_filter() { return new game_filter(); }

// Create the flicker filter used by the title screen
stateoid *flickering::get_attract_filter() { return new specialstate<flickering>(this, 5); } // 5 = title.fsh

struct plain_filter_scene : public ent {
	runner *scene;
	screenshotter *snap;
	target_ent *target;
	sticker *stick_shader1; float shader1;
	
	plain_filter_scene() : ent(), scene(NULL), snap(NULL), target(NULL), stick_shader1(NULL), shader1(0) {
	}
	
	void inserting() {
		scene = new runner(NULL);
		
		filter *f = new filter(scene, new specialstate<plain_filter_scene>(this, filtermode));
		f->insert(this);
		target = f->target[0];
		
		// If it has a filter, this will control the filter parameter.
		stick_shader1 = new sticker(INPUTCODE(G_DBG, IDBG_SHADER1)); stick_shader1->insert(this);
		
		// Finally we want the ability to take screenshots of all this.
		(new screenshotter(INPUTCODE(G_DBG, IDBG_SNAPSHOT), target->framebuffer))->insert(this);
	}
	
	void tick() {
		shader1 += stick_shader1->stick() * 1.0/256;
		
		if (spinmode) {
			float spinxmode = .005, spinymode = .01 ;
			scene->innerspin = scene->innerspin * glm::eulerAngleXY<float>(spinxmode, spinymode);
		}
		
		ent::tick();
	}
};

void game_init() {
	int mode = 2;
	cheatfile_load(mode, "mode.txt");
	
	int mute = 0;
	cheatfile_load(mute, "mute.txt");
	if (mute) audio->volume(0);
	
	// Originally these were scattered through the program, now we do them here so they're pre-set before attract
	cheatfile_load(facemode, "face.txt");
	cheatfile_load(colormode, "color.txt");
	cheatfile_load(mirrormode, "mirror.txt");
	cheatfile_load(mirrorcolormode, "mirrorcolor.txt");
	cheatfile_load(mirrorskinmode, "mirrorskin.txt");
	cheatfile_load(mutatemode, "mutate.txt");
	cheatfile_load(filtermode, "filter.txt");
	cheatfile_load(spinmode, "spin.txt");
	
#if !SELF_EDIT
//	SDL_ShowCursor(0); // Hide cursor
#endif

	switch (mode) {
		case 1:
			ditch_setup_controls();
			(new game_filter())->insert();
			break;
		case 2: {
			if (!gl2) { (new lockout())->insert(); return; }
			
			extern bool global_esc_handler;
			global_esc_handler = false;
			
			ditch_setup_controls();
			InputRules::rules()->load( InputRuleSpec(InputKindKeyboard, INPUTCODE(G_G, I_BACKOUT), AXISCODE_HIGH_MASK|SDLK_ESCAPE ));
	
			attract *a = new attract();
			a->insert();
			a->randomize();
		} break;
		case 3:
			ditch_setup_controls();
			(new runner(NULL))->insert();
			break;
		case 10: {
			ditch_setup_controls();
			(new plain_filter_scene())->insert();
		} break;
		case 4:   // Diamond (multicolored square) test
			(new testdiamond())->insert();
			break;
		case 5: { // Postprocess test
			(new filter( new testdiamond(), NULL, 1.0/32 ))->insert();
		} break;
		case 6: { // Splatter texture test
			texture_slice *drawInto = new texture_slice();
			drawInto->init(surfacew, surfaceh);
			testcross(drawInto);
			drawInto->construct();
			(new splatter(new texture_source(drawInto, true)))->insert();
		} break;
		case 7: { // Untextured uncolored box test
			(new boxer(cpv(0.25, 0.25), cpv(0.5, 0.5)))->insert();
		} break;
		case -1: {
			(new input_vacuum_ent(InputKindEdge))->insert();
		} break;
	};
}
