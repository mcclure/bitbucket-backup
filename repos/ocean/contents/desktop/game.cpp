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
#include <plaid/audio.h>
#include <plaid/audio/synth.h>
#include <plaid/audio/effects.h>
#include "program.h"
#include "glCommon.h"
#include "postprocess.h"
#include "chipmunk_ent.h"
#include "input_ent.h"
#include "basement.h"
#include "inputcodes.h"
#include "display_ent.h"
#include "glCommonMatrix.h"

#undef True
#undef False
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_precision.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <deque>

using namespace plaid;
extern Ref<Audio> audio;

#define SM 1

#define S 10.0

#define RED 0xFF0000FF
#define CAMTEST 0

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

// ---------- Attract mode ----------

float centerOff(const char *str);
struct attract : public ent {
	int expired;
	attract() : ent(), expired(-1) {}
	void display(drawing *d) {
		jcImmediateColor4f(1.0,1.0,1.0,1.0);	// For debug
		drawText("YOU WILL DIE ALONE AT SEA", -0.75/aspect, 0, 0, false, true);
		drawText("", 0, -0.40, 0, true, true);
		drawText("PLEASE CLICK.", 0.75/aspect-centerOff("PLEASE CLICK.")/surfaceh*4, 0, 0, false, true);
	}
	void input(InputData *data) {
		if (data->inputcode != INPUTCODE(G_G, I_MOUSE)) return;
		if (data->axiscode & AXISCODE_ZERO_MASK) { // On mouseup
			expired = ticks; // Defer moving on by one frame to prevent double input processing
		}
	}
	void tick();
};

#define VOLUME 1

struct NoiseSynth : public AudioSynth<_OscillatorState>
{
	NoiseSynth(AudioFormat format, float frequency, float amp) :
		AudioSynth<State>(AudioFormat(1, format.rate), State(amp, frequency, 0.0f)) {}

	float frequency() const       {return state.freq;}
	void frequency(float freq)    {state.freq = freq;}

	typedef _OscillatorState State;
	virtual State interpolate(const State &a, const State &b, float mid) {
		return b;
	}
	virtual void pull(AudioChunk &chunk, const State &a, const State &b) {
		int32_t *i = chunk.start(0), *e = chunk.end(0);

		while (i != e) {
			*i = float(random())/RANDOM_MAX * AudioFormat::INT24_CLIP;
			i++;
		}

		//Copy signal into all other channels
		for (uint32_t c = chunk.channels()-1; c > 0; --c)
		{
			std::memcpy(chunk.start(c), chunk.start(0), 4*chunk.length());
		}
	}
};

struct sonar : public ent {
	runner *parent;
	Ref<Amp> mix;
	Ref<Filter> filter;
	sonar(runner *_parent) : ent(), parent(_parent) {
		filter = new Filter(new NoiseSynth(audio->format(), 0, 1), 400.0f, 0.0f);
		mix = new Amp(filter, 1);
	}
	void insert() {
		audio->play(mix);
		roll();
		ent::insert();
	}
	void roll();
	void tick();
};

// ---------- GRAPHICS ----------

struct wireframe_quad : public elementoid {
	static vector<unsigned short> megaIndex;
	static void megaIndexEnsure(int count) {
		int current = megaIndex.size()/8;
		for(int c = current; c < count; c++) {
			megaIndex.push_back(c*4);
			megaIndex.push_back(c*4+1);
			megaIndex.push_back(c*4+1);
			megaIndex.push_back(c*4+2);
			megaIndex.push_back(c*4+2);
			megaIndex.push_back(c*4+3);
			megaIndex.push_back(c*4+3);
			megaIndex.push_back(c*4);
		}
	}
	void draw(pile &q) {
		int vertsize = q.stride();
		int pushed = q.size()/vertsize/4; // This many quads
		megaIndexEnsure(pushed);

		glDrawElements(GL_LINES, pushed*8, GL_UNSIGNED_SHORT, &megaIndex[0]); // *6 because enough indices for two triangles
	}
	static wireframe_quad *single;
};
vector<unsigned short> wireframe_quad::megaIndex;
wireframe_quad *wireframe_quad::single = new wireframe_quad();

struct state_blackout : public stateoid {
	virtual void set(pile &q) {
		States(q.useTexture, false);
		uint32_t color = BLACK;
		jcColor4ubv((GLubyte*)&color);
	}
	static state_blackout *single;
};
state_blackout *state_blackout::single = new state_blackout();

struct state_doubled : public stateoid {
	elementoid *pass1e, *pass2e;
	stateoid *pass1s, *pass2s;
	state_doubled(elementoid *_pass1e = element_quad::single, elementoid *_pass2e = wireframe_quad::single, stateoid *_pass1s = state_blackout::single, stateoid *_pass2s = state_basic::single)
		: pass1e(_pass1e), pass2e(_pass2e), pass1s(_pass1s), pass2s(_pass2s) {}
	virtual void set(pile &q) {
		uint32_t color = q.baseColor;
	
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1., 1./(float)0x1);
		q.elementDrawer = pass1e;
		q.stateSetter = pass1s;
		q.baseColor = BLACK;
		q.draw();
		glDisable(GL_POLYGON_OFFSET_FILL);
		
		q.elementDrawer = pass2e;
		q.baseColor = color;
		pass2s->set(q);
		q.stateSetter = this;
	}
};

struct element_free : public elementoid {
	element_free(GLenum _kind = GL_TRIANGLES) : elementoid(), kind(_kind) {}
	GLenum kind;
	vector<unsigned short> index;
	void push(unsigned short i) {index.push_back(i);}
	void draw(pile &q) {
		glDrawElements(kind, index.size(), GL_UNSIGNED_SHORT, &index[0]);
	}
};

// ---------- REALITY ----------

void cube(quadpile3 &q, glm::vec3 root, glm::vec3 size) {
	#define CF(power, index) (root[index] + power*size[index])
	cpVect corners[4] = {cpv(CF(0, 0),CF(0, 1)), cpv(CF(0, 0),CF(1, 1)), cpv(CF(1, 0),CF(1, 1)), cpv(CF(1, 0),CF(0, 1))};
	
	for(int c = 0; c < 4; c++) {
		q.xysheet(corners[c], corners[(c+1)%4], CF(0, 2), CF(1, 2));
	}
	q.xyplane(corners[0], corners[2], CF(0, 2));
	q.xyplane(corners[0], corners[2], CF(1, 2));
}

void matrix_apply(const glm::mat4 &m, float *in, float *out) {
	glm::vec4 v = glm::vec4(in[0], in[1], in[2], 1);
	v = m * v;
	out[0] = v[0]/v[3]; out[1] = v[1]/v[3]; out[2] = v[2]/v[3];
}

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

static int lineWidth = 1;
GLint lineWidthRange[2] = {1,1};

// Put everything here
#define F_POINTSPAN (0.5*S)
#define F_POINTSIDE 60
#define F_Y			(-S)
const static cpVect flow = cpv(1,1/2.0);
struct runner : public ent, public freeze {
	// State
	sonar *sound;
	pile *field;
	element_free *fieldblack, *fieldwire;
	
	quadpile3 *boat, *rawboat;
	state_doubled *boxstate;
	
	cpFloat waveT; cpFloat panic;
	
	// Place
	glm::mat4 face;
	glm::mat4 offset; // Wiggle
	
	// Interface
	bool havePos;
	cpVect lastPos;
	cpVect sumPos; // MOUSE ANGLE
	cpVect camPos; // CAM XZ
	cpFloat yoff; // CAM HEIGHT
#if DBG_CONTROLS
	sticker *stick_x, *stick_z;
#endif
	averager xangavg, zangavg;
	
	runner() : ent(), freeze(), sound(NULL), field(NULL), fieldblack(NULL), fieldwire(NULL), face(1.0), offset(1.0), waveT(0),
		havePos(false), sumPos(cpvzero), camPos(cpv(-F_POINTSPAN*F_POINTSIDE/2,-F_POINTSPAN*F_POINTSIDE/2)), yoff(1), panic(0)
#if DBG_CONTROLS
		, stick_x(NULL), stick_z(NULL)
#endif
	  {
		sound = new sonar(this);
		
		glGetIntegerv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
		
#if DBG_CONTROLS
		stick_x = new sticker(INPUTCODE(G_DBG, IDBG_MOVE_X)); stick_x->insert();
		stick_z = new sticker(INPUTCODE(G_DBG, IDBG_MOVE_Y)); stick_z->insert();
#endif
		
		// Define world
		fieldblack = new element_free();
		fieldwire = new element_free(GL_LINES);
		state_doubled *state = new state_doubled(fieldblack, fieldwire);
		field = new pile(fieldwire, true, false, false, 0, RED);
		field->immortal = true;
		field->stateSetter = state; // Comment out to prevent blackout
		
		// Build world
		
		for (int x=0; x<F_POINTSIDE; x++ ) { // "row/height"
			for (int z=0; z<F_POINTSIDE; z++ ) { // "col/width"
				field->push(x*F_POINTSPAN,F_Y,z*F_POINTSPAN);

				#define F_I(x, z) ((z) + ((x)*F_POINTSIDE))
				int root = F_I(x,z), nz = F_I(x,z+1), nx = F_I(x+1, z), nn = F_I(x+1, z+1);
				bool xvis = x < F_POINTSIDE-1, zvis = z < F_POINTSIDE-1;
				
				if (xvis && zvis) {
					fieldblack->push(root); fieldblack->push(nx); fieldblack->push(nz);
					fieldblack->push(nz); fieldblack->push(nx); fieldblack->push(nn);
				}
				
				if (xvis) { fieldwire->push(root); fieldwire->push(nx); }
				if (zvis) { fieldwire->push(nz); fieldwire->push(root); }
				if (xvis && zvis) { fieldwire->push(nx); fieldwire->push(nz); }
			}
		}
		
		// Define boat
		boat = new quadpile3(false, false, 0, RED);
		boat->immortal = true;
		
		rawboat = new quadpile3(false, false, 0, RED);
		rawboat->immortal = true;
		
		// Build boat
		cube(*rawboat, glm::vec3(-3.0/2, 0, -5.0/2), glm::vec3(4, 0.5, 5)); // Bottom
		cube(*rawboat, glm::vec3(-3.0/2, 0.5, -5.0/2 + 0.5), glm::vec3(0.5, 1, 5 - 0.5 - 0.5)); // "Right"
		cube(*rawboat, glm::vec3(-3.0/2 +4-0.5, 0.5, -5.0/2 + 0.5), glm::vec3(0.5, 1, 5 - 0.5 - 0.5)); // "Left"
		
		cube(*rawboat, glm::vec3(-3.0/2, 0.5, -5.0/2), glm::vec3(4, 1, 0.5)); // "Back"
		cube(*rawboat, glm::vec3(-3.0/2, 0.5, -5.0/2 + 5 - 0.5), glm::vec3(4, 1, 0.5)); // "Back"
		
		*boat = *rawboat;
		boat->stateSetter = new state_doubled();
		
		sound->insert();
	}
	void display(drawing *d) {
		glEnable(GL_DEPTH_TEST); // LEAKY
		goPerspective();
	
		// First ocean 
		
		cpVect fieldPos = cpvmult(camPos, -1.0 / F_POINTSPAN );
		
		pile &qf = d->insert_pile(D_FIELD, field);
		float progress = frame/(float(ANI_LENGTH*FPS));
		
		const int startFrame = HOLEAT*FPS*ANI_LENGTH;
		cpVect holeCenter = cpvzero;
		
	   #define HOLEBOTTOM (-30 * S)
	   #define HOLESLUR (-10*S)
	   #define HOLEINNER (2)
	   #define HOLEOUTER (6)
		float holeProgress = min<float>((frame-startFrame)/(FPS*ANI_LENGTH*(1.0-HOLEAT)), 1.0);
		
		if (frame >= startFrame) {
			holeCenter = fieldPos + cpvmult(flow, -HOLEOUT*(1.0-holeProgress)); // see fieldPos elsewhere
			cpFloat distance = cpvlength(cpvsub(fieldPos, holeCenter));
			if (distance < HOLEOUTER) {
				panic = 1 - (distance)/HOLEOUTER;
//				ERR("PANIC!! %lf", panic);
				if (panic > 0.25 && (frame%2)) {
					if (lineWidth < lineWidthRange[1]) {
						glLineWidth(lineWidth);
						lineWidth = lineWidth + 1;
					} else if (lineWidth >= lineWidthRange[1]) {
						Quit();
					}
				}
			}
//		   if (progress < 1) { ERR("DIST!! %f panic %f\n", cpvlength(cpvsub(fieldPos, holeCenter)), panic); }
		}
		
		float initial = inter(progress, 1.0/4, 2.0/4, 1, 0);
		float highmark = inter(progress, 1.0/4, 2.0/4, 0, 1);
		float erase = inter(progress, 3.0/4, 1, 0, -1);
		
//		ERR("%f -> %f\t%f\t%f\t= %f\n", progress, initial, highmark, erase, (1.0*initial + 0.5*highmark));
		
		highmark = highmark + erase;
		
		#define XSCALE1 (1.0)
		#define ZSCALE1 (1.0)
		#define YSCALE1 (1.0)
		#define XSCALE2 (0.5)
		#define ZSCALE2 (0.5)
		#define YSCALE2 (2.0)
		#define TSCALE ((1.0)/30)
		float t = frame*TSCALE;
		for (int x=0; x<F_POINTSIDE; x++ ) { // "row/height"
			for (int z=0; z<F_POINTSIDE; z++ ) { // "col/width"
				float &v = qf[F_I(x, z)*3+1];
				v = F_Y;
				v += initial*sin((x+t)*XSCALE1+(z+t)*ZSCALE1)*(YSCALE1*S/10.0)*2;
				v += highmark*sin((x+t)*XSCALE2+(z+t)*ZSCALE2)*(YSCALE2*S/10.0)*2;
				
				if (frame >= startFrame) {
				   float dist = cpvlength( cpvsub(holeCenter, cpv(x,z)) );
				   if (dist < HOLEINNER) {
						   v += HOLEBOTTOM;
				   } else if (dist < HOLEOUTER) {
						   dist -= HOLEINNER;
						   dist /= (HOLEOUTER-HOLEINNER);
						   dist = 1-dist;
						   dist = pow(dist, 2);
						   dist = dist*HOLESLUR;
						   v += dist;
				   }
				}
			}
		}
		
		t += TSCALE;

		// Then player (needs ocean heightmap)
	
		cpFloat playerY = yoff;
		cpFloat height = 0; // Height of sea at this point
		cpFloat boatxang = 0, boatzang = 0;
		if (fieldPos.x >= 0 && fieldPos.y >= 0 && fieldPos.x < F_POINTSIDE && fieldPos.y < F_POINTSIDE) {
			int fx = fieldPos.x, fz = fieldPos.y;
			bool high = (fieldPos.x-fx) + (fieldPos.y-fz) > 1;
			int root = F_I(fx,fz), nz = F_I(fx,fz+1), nx = F_I(fx+1, fz), nn = F_I(fx+1, fz+1);
			int p1 = nx, p2 = nz, p3 = high ? nn : root;
			float plane1 = qf[p1*3+1], plane2 = qf[p2*3+1], plane3 = qf[p3*3+1];
			glm::vec3 const & plane1v = glm::vec3((fx+1)*F_POINTSPAN, plane1, (fz  )*F_POINTSPAN);
			glm::vec3 const & plane2v = glm::vec3((fx  )*F_POINTSPAN, plane2, (fz+1)*F_POINTSPAN);
			glm::vec3 const & plane3v = glm::vec3((fx+high)*F_POINTSPAN, plane3, (fz+high)*F_POINTSPAN);
			glm::vec3 const & normal = glm::normalize(glm::cross(plane3v - plane1v, plane2v - plane1v)) * (high?1.0f:-1.0f);
			// Equation from http://www.scratchapixel.com/lessons/3d-basic-lessons/lesson-9-ray-triangle-intersection/ray-triangle-intersection-geometric-solution/
			float planeD = glm::dot(normal, plane1v); // For plane equation Ax+By+Cz+D
			// Cast a ray from the y-plane toward the triangle plane
			glm::vec3 const &rayStart = glm::vec3(camPos.x, 0, camPos.y);
			glm::vec3 const &rayDir = glm::vec3(0, 1, 0);
			float rayDist = - ( glm::dot(normal, rayStart) + planeD ) / glm::dot(normal, rayDir);
			height = rayDist;

			boatxang = acos( glm::dot(normal, glm::vec3(1, 0, 0)) );
			boatzang = acos( glm::dot(normal, glm::vec3(0, 0, 1)) );
			
			boatxang = xangavg.cycleAng(boatxang);
			boatzang = zangavg.cycleAng(boatzang);
			
//			ERR("AT %f,%f = %d, %d (%s) -> %f, %f, %f = %f (%f, %f)\n", (float)camPos.x, (float)camPos.y, fx, fz, high?"Y":"N", plane1, plane2, plane3, rayDist, boatxang, boatzang);

#if !CAMTEST
			playerY += rayDist;
#endif
		}
	
		glm::vec3 playerPos = glm::vec3(camPos.x, playerY-4, camPos.y);
		glm::vec3 cameraPos;
#if CAMTEST
		cameraPos = glm::vec3(camPos.x+10, playerY+4, camPos.y);
#else
		cameraPos = playerPos;
#endif
	
		jcMultMatrixf(glm::value_ptr( glm::eulerAngleXY<float>( sumPos.y, sumPos.x )));
		jcMultMatrixf(glm::value_ptr( glm::translate(glm::mat4(1), cameraPos) ));
		jcMultMatrixf(glm::value_ptr(offset));
	
		// Then boat (needs ocean heightman + player pos)
	
#if 1
		quadpile3 &qb = d->insert_pile(D_BOAT, boat);
#endif

		glm::mat4 boatTransform(1);
		boatTransform = boatTransform * glm::translate(glm::mat4(1), glm::vec3(-playerPos.x,yoff-1 - height,-playerPos.z));
		boatTransform = boatTransform * glm::eulerAngleZ<float>(-boatxang+M_PI/2);
		boatTransform = boatTransform * glm::eulerAngleX<float>(boatzang-M_PI/2);
		for(int c = 0; c < boat->size(); c += 3) {
			matrix_apply(boatTransform, &(*rawboat)[c], &(*boat)[c]);
		}
	}
	
	void input(InputData *data) {
		if (data->inputcode != INPUTCODE(G_G, I_MOUSE)) return;
				
		if (data->touchkind == touch_move) {
			if (havePos) {
				cpVect offset = cpvsub(data->touch.at, lastPos);
				offset = cpvmult(offset, M_PI/180.0);
				offset.x *= -1;
				sumPos = cpvadd(sumPos, offset);
#if CAMTEST
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
				
		ent::input(data);
	}
	
	void tick() {
#if DBG_CONTROLS
		camPos = cpvadd(camPos, cpvmult( cpv(stick_x->strength, stick_z->strength), 0.25 ));
#endif
		camPos = cpvadd(camPos, cpvmult(flow, -1.0/(TPF)/8.0));

		ent::tick();
	}

};

void sonar::tick() {
	filter->lowpass(100 + parent->xangavg.readAng()*300 + parent->panic * 10000);
	ent::tick();
}

void sonar::roll() {	
	next(0, NOTE_EVERY); // Loop
}

struct filter;
struct specialstate : public stateoid {
	filter *ctx; bool v;
	specialstate(filter *_ctx, bool _v) : ctx(_ctx), v(_v) {}
	void set(pile &);
};

struct filter : public ent {
	target_ent *target[2];
	float bright;
	filter() : ent(), bright(5) { ZERO(target); }

	void insert() {
		ent::insert();
		
		for(int c = 0; c < 2; c++) {
			texture_slice *drawInto = new texture_slice();
			drawInto->init(surfacew*SM, surfaceh*SM);
			drawInto->construct();

			target[c] = new target_ent(drawInto, true);
			target[c]->insert(this, ENT_PRIO_FBTARGET);
		}
		
		runner *r = new runner();
		r->insert(target[0]);
		
		splatter *kid = new splatter(new texture_source(target[0]->display_texture()), true, new specialstate(this, false));
		kid->insert(target[1]);
		
		splatter *kid2 = new splatter(new texture_source(target[1]->display_texture()), true, new specialstate(this, true));
		kid2->insert(this);
	}
	
#if DBG_CONTROLS
	void input(InputData *data) {
		switch(data->inputcode) {
			case INPUTCODE(G_DBG, IDBG_P1):
				bright += (data->strength * (SDL_GetModState()&KMOD_LSHIFT?0.1:0.01));
				ERR("NEW BRIGHT %f\n", bright);
				break;
		}
		
		ent::input(data);
	}
#endif
};

void specialstate::set(pile &) {
	int which = v;
	Special(which);
	texture_slice *s = ctx->target[which]->display_texture();
	if (!v) {
		glUniform1f(p->uniforms[s_px], 1.0/s->twidth * lineWidth);
	} else {
		glUniform1f(p->uniforms[s_py], 1.0/s->theight * lineWidth);
		glUniform1f(p->uniforms[s_brightness], ctx->bright*lineWidth);
	}
		
	GLERR("Uniforms");
}

void attract::tick() {
	if (expired > 0 && ticks > expired) { // Wow this is gross.
		die();
		(new filter())->insert();
	}
}

void game_init() {
	if (!gl2) { (new lockout())->insert(); return; }
	
	int mode = 1;
	cheatfile_load(mode, "mode.txt");
	
	int mute = 0;
	cheatfile_load(mute, "mute.txt");
	if (mute) audio->volume(0);
	
#ifndef SELF_EDIT
	SDL_ShowCursor(0);
#endif

	switch (mode) {
		case 2:
			ditch_setup_controls();
			(new filter())->insert();
			break;
		case 1:
			ditch_setup_controls();
			(new attract())->insert();
			break;
		case 3:
			ditch_setup_controls();
			(new runner())->insert();
			break;
		case 4:
			(new testdiamond())->insert();
			break;

		case -1: {
			(new inputdump_ent(InputKindEdge))->insert();
		} break;
	};
}
