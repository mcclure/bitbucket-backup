// BUSINESS LOGIC

// File contains code from Jumpcore; notice applies to that code only:
/* Copyright (C) 2008-2009 Andi McClure
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// File contains code from Chipmunk "MoonBuggy" demo; notice applies to that code only:
/* Copyright (c) 2007 Scott Lembcke
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>

#include "chipmunk.h"
#include "color.h"
#include "program.h"
#include "tinyxml.h"
#include "http_lib.h"
#include "internalfile.h"

#include <dirent.h>
#include <sys/stat.h>

#define GRAVITY cpv(0.0f, -9.8/2)

#ifdef TARGET_MAC
#define PITCHSHIFT_BUTTON(x) ((x) == 2)
#define DUPE_BUTTON(x) ((x) == 3 || (SDL_GetModState() & (KMOD_LSHIFT | KMOD_RSHIFT)))
#else
#define PITCHSHIFT_BUTTON(x) ((x) == 2)
#define DUPE_BUTTON(x) ((x) == 3)
#endif

bool phoneFactor = false;
double glPixel = 1, chipSize = BASE_CHIP*2, chipSizePlus = BASE_CHIP*2;
int key_off = 0;
bool doPlayFeedback = true, doTextFeedback = true, doLetters = false;

vector<spaceinfo> spaces;

bool wantClearUi = false;
WantNewScreen want = WNothing;
cpSpace *uiSpace = NULL;
vector<ContainerBase *> columns;

list<automaton *> automata;
vector<auto_iter> dying_auto;

cpVect out_center = cpvzero;
cpFloat out_center_len = 0;

bool debug_trip_poison = false; // really shouldn't keep
slice *debug_numbers_slice = NULL;
texture_slice *debug_display_slice = NULL;

int toggle_fire_at = 0;

// xml_updating: Are we in xml update / server-client mode?
// next_xml_update_fresh: False on first xml so that you don't send data on first xml
// need_initial_time_sync: Used to determine whether to lock time to the server value. ATM disabled
bool xml_updating = false, next_xml_update_fresh = false, need_initial_time_sync = false;
// If there is a current xml update: what is its pointer?
netxml_job *current_xml_update = NULL;
// If there is no current xml update, but a future one scheduled: When?
int next_xml_update_at = 0;
// Regardless of whether xml updates are happening: How often should they occur?
double xml_update_every = 1;

string xml_update_url_base = 
#if SELF_EDIT and defined(TARGET_DESKTOP)
"http://localhost:8888/update";
#else
"http://runhello.com:8888/update";
#endif
string xml_update_url;

unsigned int chip::updated_gen = 0;

bool do_dupe = false, do_sync = true, do_grid = 
#if TWIST1_DEBUG
false
#else
true
#endif
;

// Disk/interface stuff
bool diskDirection = false; // If true: Saving
string diskTarget, diskTargetName;

// /------ Used by the angels demo ------\

cpVect lastClick = cpvzero; // FIXME
cpShape *draggingMe = NULL; // FIXME FIXME
cpVect draggingMeInto = cpvzero;

#define SCALESIZE 12
const char *scale[SCALESIZE] = {"A","A#","B","C","C#","D","D#","E","F","F#","G","G#"};

void mouseDownOnShape(cpShape *shape, void *data) {
    uintptr_t mask = (uintptr_t)data;
    if (shape->collision_type & mask) {
        cpVect clickedAt = cpvsub(lastClick, shape->body->p);
        if (draggingMe && cpvlengthsq(clickedAt) > cpvlengthsq(draggingMeInto)) // Only register closest click
            return;
        clickedAt = cpvunrotate( clickedAt, shape->body->rot );
        
        draggingMe = shape;
        draggingMeInto = cpvunrotate( cpvsub(lastClick, draggingMe->body->p), draggingMe->body->rot );
    }
}

// \------ begin display state ------/

void appendLevel(double depth) {
	int s = spaces.size(); // Is any of this even used?
  /* We first create a new space */
  spaces.push_back(spaceinfo());
  spaces[s].depth = depth;  
  
	spaces[s].space = cpSpaceNew();
    
    spaces[s].space->damping = 0.01;
	
	/* Next, you'll want to set the properties of the space such as the
     number of iterations to use in the constraint solver, the amount
     of gravity, or the amount of damping. In this case, we'll just
     set the gravity. */
	spaces[s].space->gravity = cpvzero;
	
	/* This step is optional. While you don't have to resize the spatial
	 hashes, doing so can greatly increase the speed of the collision
	 detection. The first number should be the expected average size of
	 the objects you are going to have, the second number is related to
	 the number of objects you are putting. In general, if you have more
	 objects, you want the number to be bigger, but only to a
	 point. Finding good numbers to use here is largely going to be guess
	 and check. */
	cpSpaceResizeStaticHash(spaces[s].space, 0.1, 2000);
	cpSpaceResizeActiveHash(spaces[s].space, 0.1, 200);
	
  /* This is the rigid body that we will be attaching our ground line
     segments to. We don't want it to move, so we give it an infinite
     mass and moment of inertia. We also aren't going to add it to our
     space. If we did, it would fall under the influence of gravity,
     and the ground would fall with it. */
  spaces[s].staticBody = cpBodyNew(INFINITY, INFINITY);
}

// This is called once when the program begins. Clear out and replace with your own code
void
program_init(void)
{
    for(int c = 0; c < DEBUG_BOOLS_COUNT; c++) debug_bools[c] = false;	
	
    cpInitChipmunk();
	uiSpace = cpSpaceNew();  
    
	jobs_init();
	
    program_reinit();
	
	want = WMainMenu;
}

bool arrows[4] = {false, false, false, false}; //   debug
const char *debug_what[9] = {"Resolution", "Gon", "Pack", "n/a", "n/a", "n/a", "Rings", "Per arm offset", "n/a"};
int debug_values[9] = {0, 0, 0,
        0, 0, 0,
        0, 0, 0};
int debug_floating[3] = {0,0,0};
float debug_floats[3] = {0,0,0};
bool debug_bools[DEBUG_BOOLS_COUNT];
int debug_masterint = -1;
int debug_stamp = 0, debug_stampres = 0;
void *debug_stamp_id = NULL;
#define MOSTBUTTONS 5
cpVect debug_mouse[MOSTBUTTONS];
bool have_debug_mouse[MOSTBUTTONS] = {false, false, false, false, false};

#define NO_BACKTRACK 1
cpVect orient; // "Mouse" (or analog) direction
int speeding = 0, speeding_first = -1;
int caught = 0, fuel = 2;

// TODO: Undesirable from a UX standpoint! But if this is off, there's an edge case where we send a XML,
// delete a chip while XML is in-flight, then receive an update to that XML but the id is still in deadchips.
// At this point the server and client become desynced. Probably seek an alternate solution to this edge case.
#define DEAD_CHIPS_OVERRIDE 1

hash_map<int,cpBody *> deadchips;

cpShape * make_chip(cpVect at = cpvzero, chip_payload ch = chip_payload(20), bool autofire = false, int id = random()) {
	cpBody *newBody = cpBodyNew(INFINITY, INFINITY);
    guichip *newChip = new guichip();
	newBody->data = newChip;

    newChip->id = id;
    newChip->ip = ch;
    newChip->readjust(at);
	newChip->updated = chip::updated_now();
	newChip->oversize = ch.oversize();
	if (autofire && doPlayFeedback) newChip->wildfire = true;

	pthread_mutex_lock(&mainCircle.live_mutex);
	mainCircle.chips[id] = newBody;
	pthread_mutex_unlock(&mainCircle.live_mutex);
    
//    cpShape *newShape = cpPolyShapeNew(newBody, 4, slice_verts, cpvzero);
    cpShape *newShape = cpCircleShapeNew(newBody, BASE_CHIP, cpvzero);

    cpShape *newShapeOuter = cpCircleShapeNew(newBody, BASE_CHIP*(phoneFactor?4:2), cpvzero); // FIXME: Have a CHIP_SIZE_OUTER

    newBody->p = at;
    
    newShape->e = 0.80;
    newShape->u = 0.80;
    newShape->collision_type = C_CHIP;
    newShape->layers = L_CHIP;
    
    newShapeOuter->collision_type = C_CHIP_OUTER;
    newShapeOuter->layers = L_CHIP_OUTER;
    
    cpBodySetMass(newBody, 5.0);
    cpBodySetMoment(newBody, cpMomentForCircle(newBody->m, BASE_CHIP, 1, cpvzero));

    cpSpaceAddBody(spaces[0].space, newBody);
    cpSpaceAddStaticShape(spaces[0].space, newShape); // Should be active   
    cpSpaceAddStaticShape(spaces[0].space, newShapeOuter);   
    
    newChip->shapes.push_back(newShape);
    newChip->shapes.push_back(newShapeOuter);
    
    return newShapeOuter;
}

// Note: Will crash and do other horrible things if called on a non-gui chip
// if also_unlink is true, mainCircle.chips will also be modified-- which is unsafe if we're currently iterating mainCircle.chips
void destroy_chip(cpBody *body, bool also_unlink = true, bool broadcast_allowed = true) {
    guichip *ch = (guichip *)body->data;
    	
	if (also_unlink) { // Note: If pass in false, make sure to hold the live_mutex before calling
        pthread_mutex_lock(&mainCircle.live_mutex);
		mainCircle.chips.erase(ch->id);
        pthread_mutex_unlock(&mainCircle.live_mutex);
    }	
	
	for(int c = 0; c < ch->shapes.size(); c++) { // Use an iterator ugh
        cpSpaceRemoveStaticShape(spaces[0].space, ch->shapes[c]);
        cpShapeDestroy(ch->shapes[c]);
    }	
    cpSpaceRemoveBody(spaces[0].space, body);
    cpBodyDestroy(body);
    	
	if (broadcast_allowed && xml_updating) { // If we are notifying network operations and also this is one
		ERR("NEW DEAD CHIP\n");
		deadchips[ch->id] = NULL;
	}
	
#ifdef TARGET_MOBILE
	if (ch->twist)
		ch->twist->forgetTwist();
#endif
    
    delete ch;
}

void destroy_all_chips() {
    pthread_mutex_lock(&mainCircle.live_mutex);
    for(chip_iter i = mainCircle.chips.begin(); i != mainCircle.chips.end(); i++) {
        destroy_chip(i->second, false);
    }
	mainCircle.chips.clear();
    pthread_mutex_unlock(&mainCircle.live_mutex);
}

// Note: All chips are guichips in drumcircle except in certain VERY limited cirucmstances 
chip::chip() : bodydata(C_CHIP), ip(chip_payload(40)), when(0), wildfire(false), fired(0), updated(0)
{}

guichip::guichip() : chip(), twistStartAt(0), oversize(false)
#ifdef TARGET_MOBILE
, twist (NULL)
#endif
{}

void guichip::readjust(cpVect to) {
    when = vectToTime(to);
}

void guichip::twistStart() {
	twistStartAt = ip.p;
}
void guichip::twistUpdate(cpFloat to) {
	int newp = twistStartAt + to;
	if (doPlayFeedback && newp != ip.p) wildfire = true;
	ip.p = newp;
	updated = chip::updated_now();
	ERR("PITCH %p\t%d\n", this, ip.p);
}

double randco() {
    return (double)random()/RANDOM_MAX*2-1;
}

bardata * make_bar(cpVect v, bardata *b) {
#if 0 //#ifdef TARGET_DESKTOP
    float mod = typ == C_BAR_TOP ? (7.0/12) : 1; // Everything about this is wrong
#else
    float mod = 1;
#endif
    cpVect bar_verts[] = {
		cpv(-1/aspect * mod,-BARHEIGHT),
		cpv(-1/aspect * mod, BARHEIGHT),
		cpv( 1/aspect * mod, BARHEIGHT),
		cpv( 1/aspect * mod,-BARHEIGHT),
	};
    cpBody *newBody = cpBodyNew(INFINITY, INFINITY);
    cpShape *newShape = cpPolyShapeNew(newBody, 4, bar_verts, cpvzero);
    
    newBody->p = v;
    
    bardata *result = b;
    newBody->data = result;

    newShape->collision_type = C_BAR;
    newShape->layers = L_BAR;
    
    cpSpaceAddBody(spaces[0].space, newBody);
    cpSpaceAddStaticShape(spaces[0].space, newShape);    
    
    return result;
}

// VIOLATING ALL MY OWN RULES HERE
extern subtexture_slice *finter[ibar_max], *falter[ibar_max];
extern subtexture_slice *farrow;
extern hash_map<string, sample_inst *> library;

// I don't even know what I'm doing with the structure of this file at this point
static unsigned int upload_update = 0;

#define SAVELOADFILE "save.xml"

string updateCookie;

// DatedTo indicates a time that-- if we are "doing time" at all-- chips that have been updated
// AFTER that time are EXEMPTED from being altered in the incoming document load
// Returns "parse success?"
bool loadDocument(TiXmlDocument &xml, unsigned int datedTo = 0, bool dotime = false) {
    TiXmlElement *rootxml = (TiXmlElement *)xml.IterateChildren("dc",NULL);
    if (!rootxml || rootxml->Type() != TiXmlNode::ELEMENT) return false;
    if (dotime) {
        const char *temp = rootxml->Attribute("time");
        if (dotime && temp) updateCookie = temp; // Also check "delay"-- WHAT TO DO WHEN TEMP IS NULL?
		if (need_initial_time_sync) {
#if 0
			double actualtime = atof(updateCookie.c_str());
			nowat = fmod(actualtime, 24*24*60); // Fmod is pretty arbitrary-- just want to spend too many bits on UTC
			ERR("-----\n\nsync %lf\n\n------\n", nowat);
#endif
			need_initial_time_sync = false;
		}
    }
    TiXmlElement *boardxml = (TiXmlElement *)rootxml->IterateChildren("board", NULL);
    if (!boardxml || boardxml->Type() != TiXmlNode::ELEMENT) return true;
	
	if (!dotime || !metro_dirty) // That is, if we aren't trying to write a tempo of our own by now
		boardxml->QueryDoubleAttribute("tempo", &mainCircle.metro_at);
	
    TiXmlElement *setxml = (TiXmlElement *)boardxml->IterateChildren("set", NULL);
	
	// Check for new/changed tokens
    if (setxml && setxml->Type() == TiXmlNode::ELEMENT) {
		TiXmlElement *cxml = NULL;
		while(1) {
			cxml = (TiXmlElement *)setxml->IterateChildren("c", cxml);
			if (!cxml) break;
			int p = 0; double x,y; int id;
			
			if (cxml->Type() != TiXmlNode::ELEMENT
				|| TIXML_SUCCESS != cxml->QueryDoubleAttribute("x", &x)
				|| TIXML_SUCCESS != cxml->QueryDoubleAttribute("y", &y)
				|| TIXML_SUCCESS != cxml->QueryIntAttribute("i", &id)) // Must "i" be necessary?
				continue;
			
			bool havep = TIXML_SUCCESS == cxml->QueryIntAttribute("p", &p);
			const char *samplename = cxml->Attribute("s");
			int lc = samplename ? library.count(samplename) : 0;
			if (!havep && (!samplename || !lc)) continue;
			sample_inst *s = lc ? library[samplename] : NULL;		
			
			if (!mainCircle.chips.count(id)) {
#if DEAD_CHIPS_OVERRIDE
				if (!deadchips.count(id))  // Don't accidentally bring chips back to life
#endif
				{
					chip_payload ch(p, s);
					cpShape *new_shape = make_chip(cpv(x,y), ch, false, id);
					((chip *)new_shape->body->data)->updated = 0; // Don't accidentally put this into any new documents
				}
			} else {
				cpBody *alter = mainCircle.chips[id];
				chip *alter_chip = (chip *)alter->data;
				ERR("Chip alter %u (%s %d < %d)\n", id, alter_chip->updated < datedTo? "YES:" : "NO: ", alter_chip->updated, datedTo); // What about dels?
				if (alter_chip->updated < datedTo) { // If chip on board is older than chip in document
					alter_chip->ip.p = p;  // Notice: if updated == datedTo then board takes priority
					alter_chip->ip.samp = s;
					alter->p = cpv(x,y);
				}
			}
		}    
	}
	
	// Check for removed tokens
	TiXmlElement *delxml = (TiXmlElement *)boardxml->IterateChildren("del", NULL);
    if (delxml && delxml->Type() == TiXmlNode::ELEMENT) {
		TiXmlElement *cxml = NULL;
		while(1) {
			cxml = (TiXmlElement *)delxml->IterateChildren("c", cxml);
			if (!cxml) break;
			int id;
			if (cxml->Type() != TiXmlNode::ELEMENT
				|| TIXML_SUCCESS != cxml->QueryIntAttribute("i", &id))
				continue;
			if (mainCircle.chips.count(id))
				destroy_chip(mainCircle.chips[id], true, false); // Don't tell the world
		}
	}
	return true;
}

#define CHECKBOARD if (!board) { board = new TiXmlElement("board"); root->LinkEndChild(board); }

// Since indicates a time that-- if we are "doing time"-- ONLY chips that have been updated
// AFTER that time will be included in the document push
// Returns "did anything change?" or "is there any content"?
bool putDocument(TiXmlDocument &xml, unsigned int since = 0, bool dotime = false) {
    TiXmlElement *root = new TiXmlElement("dc");
    TiXmlElement *board = NULL;
    TiXmlElement *set = NULL;
	TiXmlElement *del = NULL;
	bool content = false;

	bool we_are_net = dotime; // Safe assumption for now
    
    if (dotime && updateCookie.size()) root->SetAttribute("last", updateCookie);
	if (!dotime || metro_dirty) {
		CHECKBOARD;
		board->SetDoubleAttribute("tempo", mainCircle.metro_at);
		metro_dirty = false;
		content = true;
	}
	
    xml.LinkEndChild(root);
    
//	ERR("\nUPDATING SINCE %d\n", since);
    for(chip_iter _i = mainCircle.chips.begin(); _i != mainCircle.chips.end(); _i++) {
		const guichip *i = (guichip *)_i->second->data;
//		ERR("%d? ", i->updated);
		if (i->updated < since) // Notice: if updated == since we send
			continue;
//		ERR("Y. ");
        TiXmlElement *chip = new TiXmlElement("c");
        cpBody *body = i->shapes[0]->body;                
        chip->SetDoubleAttribute("x", body->p.x);
        chip->SetDoubleAttribute("y", body->p.y);
        chip->SetAttribute("i", i->id);
		if (i->ip.samp)       
			chip->SetAttribute("s", i->ip.samp->shortname());
		chip->SetAttribute("p", i->ip.p);
		
		if (!set) {
			CHECKBOARD;
			set = new TiXmlElement("set");
			board->LinkEndChild(set);
		}
		
        set->LinkEndChild(chip);
		content = true;
    }    
	if (we_are_net) {
		for(chip_iter i = deadchips.begin(); i != deadchips.end(); i++) {
			int id = i->first;
			TiXmlElement *chip = new TiXmlElement("c");
			ERR("DEAD CHIP %x\n", id);
			chip->SetAttribute("i", id);
			
			if (!del) {
				CHECKBOARD;
				del = new TiXmlElement("del"); // Only net cares about delete
				board->LinkEndChild(del);
			}
			del->LinkEndChild(chip);
			content = true;
		}
		deadchips.clear();
	}
	return content;
}


// In response to quit or esc or something
void document_clear_out() {
	if (next_xml_update_fresh)
		return;
	netxml_job *final_xml_update = new netxml_job(xml_update_url);
	final_xml_update->have_send = true;
	bool worthSending = putDocument(final_xml_update->send, upload_update, true);
	if (worthSending) {
		job_discard.push_back(final_xml_update);
		final_xml_update->insert();
	} else {
		ERR("DECLINE\n");
		delete final_xml_update;
	}
}

bardata * make_bar(cpVect v);

// true on success
bool bar_populate(multibardata * into, string name, bool internal = false) {
	char file[FILENAMESIZE]; 
	if (internal)
		internalPath(file, name.c_str());
	else
		strncpy(file, name.c_str(), FILENAMESIZE);

	TiXmlDocument xml(file);
	xml.LoadFile();
	
	TiXmlElement *rootxml = (TiXmlElement *)xml.IterateChildren("pack",NULL);
    if (!rootxml || rootxml->Type() != TiXmlNode::ELEMENT) return false;

	bool success = false;
	TiXmlElement *rowxml = NULL;
	while(1) {
		rowxml = (TiXmlElement *)rootxml->IterateChildren("row", rowxml);
		if (!rowxml) break;
		if (rowxml->Type() != TiXmlNode::ELEMENT) continue;

		onebardata *bar;
		bar = new onebardata();
		
		TiXmlElement *cxml = NULL;
		while(1) {
			cxml = (TiXmlElement *)rowxml->IterateChildren("c", cxml);
			if (!cxml) break;
			if (cxml->Type() != TiXmlNode::ELEMENT) continue;

			chip_payload load(0);
			int temp;
			success = true; // We created at least one chip out of this xml file
			
			cxml->QueryIntAttribute("p", &load.p);
			if (TIXML_SUCCESS == cxml->QueryIntAttribute("t", &temp)) {
				int w = NOTE_BASE + key_off + (temp/2)*12 + (temp%2)*7;
				load.p += w;
			}
			
			const char *s = cxml->Attribute("s");
			if (s) {
				int pmod = 0;
				cxml->QueryIntAttribute("m", &pmod);
				load.samp = library_check(s, pmod, cxml->Attribute("i"));
			}
			
			bar->buttons.push_back( new generator_bb(load) );
		}
		
		bar->buttons.push_back( new barop_bb( farrow, 0xFFFFFFFF, into ) );
		into->bars.push_back(bar);
	}
	
	return success;
}

multibardata *last_inventory = NULL;

void populate_all(multibardata *into) { // Load samples.xml + all xml files in working directory
	bool found_samples_xml = false;
	
	DIR *dird = opendir(".");
	dirent *dir;
	while (dir = readdir(dird)) {
		if (strlen(dir->d_name) < 4 || strncasecmp( ".xml", dir->d_name + strlen(dir->d_name) - 4, 4 ))
			continue;
		bool success = bar_populate(into, dir->d_name);
		if (success && !strcasecmp("samples.xml", dir->d_name))
			found_samples_xml = true;
	}
	closedir(dird);

	if (!found_samples_xml) {
		vector<onebardata *> oldbars;
		if (!into->bars.empty()) { // KINDA WROTE MYSELF INTO A CORNER HERE ARG
			oldbars = into->bars;
			into->bars.clear();
		}
		
		bar_populate(into, "samples.xml", true);
		
		for(int c = 0; c < oldbars.size(); c++)
			into->bars.push_back(oldbars[c]);
	}
}

void make_all_bars() {
    multibardata *botc = new multibardata(); make_bar(cpv(0, -1+BARHEIGHT), botc);
    onebardata *top = new onebardata(); make_bar(cpv(0,  1-BARHEIGHT), top);
    
    // Do top
	populate_all(botc);
    
    // Do bot
    for(int c = 0; c < ibar_max; c++) {
        top->buttons.push_back( new ibar_bb(finter[c], 0xFFFFFFFF, c) );
    }
	
	last_inventory = botc;
}

void make_guide(cpVect v) {
    cpBody *newBody = cpBodyNew(INFINITY, INFINITY);
    cpShape *newShape = cpCircleShapeNew(newBody, BASE_CHIP*2, cpvzero);
    newBody->p = v;
    newShape->collision_type = C_GUIDE;
    newShape->layers = L_GUIDE;
    cpSpaceAddBody(spaces[0].space, newBody);
    cpSpaceAddStaticShape(spaces[0].space, newShape);    
}

void make_arm() {
    cpBody *newBody = cpBodyNew(INFINITY, INFINITY);
    cpShape *newShape = cpCircleShapeNew(newBody, BASE_CHIP*2, cpvzero);
    newBody->p = cpvzero;
    newShape->collision_type = C_TOGGLE;
    newShape->layers = L_TOGGLE;
    cpSpaceAddBody(spaces[0].space, newBody);
    cpSpaceAddStaticShape(spaces[0].space, newShape);        
}

double radx = 1, rady = 1, radmin = 1;

void make_clutter() {
	for(int c = 0; c < 16; c++) {
        cpVect v = cpv(randco()*radmin, randco()*radmin);
        make_chip(v, random()%36 + NOTE_BASE);
    }    	
}

void
program_reinit(void)
{
    radx = 1/aspect;
    rady = 1-BARHEIGHT*2;
    radmin = ::min(radx,rady);
    
	appendLevel(0);
    
    cpVect screen_verts[] = {
		cpv(-1/aspect,-1),
		cpv(-1/aspect, 1),
		cpv( 1/aspect, 1),
		cpv( 1/aspect,-1),
	};        

	for(int c = 0; c < 4; c++) {		
		cpShape *newShape = cpSegmentShapeNew(spaces[0].staticBody, screen_verts[c], screen_verts[(c+1)%4], 0.1);
		cpSpaceAddStaticShape(spaces[0].space, newShape);
		newShape->e = 0.8; newShape->u = 0.8; 
        newShape->layers = L_WALL;
	}    

    #define RINGS 3
    { 
        cpFloat rstep = radmin/(RINGS)*0.90;
        for(int d = 0; d < RINGS; d++) {
            int pts = 4 << d;
            for(int c = 0; c < pts; c++) {
                cpVect v = cpvmult( cpvforangle(c*(2*M_PI/pts)), rstep*(d+1));
                make_guide(v);
            }    
        }
    }
    
//    for(int c = 0; c < 16; c++) {
//        cpVect v = cpvmult( cpvforangle(c*(2*M_PI/16)), 0.5);
//        make_chip(v);
//    }
        
    make_all_bars();
    
    make_arm();
}

void automaton::tick() {
    if (age() > 0) {
        frame++;
    }
    if (frame > rollover) {
        state++;
        rollover = 0;
    }
}

void automaton::die() {
    dying_auto.push_back(anchor);
}

// The default display() calls this once per framedraw.
void
program_update(void)
{
    ticks++;
    
    for(int c = 0; c < 3; c++) {
        debug_floats[c] += debug_floating[c];
        if (debug_floating[c]) { // Nonsense
//            BANNER(FPS/10) << "Floating[" << c << "] = " << debug_floats[c];
            ERR("floating[%d] = %lf\n", c, debug_floats[c]);
        }
    }
    
  /* Collision detection isn't amazing in Chipmunk yet. Fast moving
     objects can pass right though each other if they move too much in a
     single step. To deal with this, you can just make your steps
     smaller and cpSpaceStep() several times. */
  int substeps = 1;

  /* This is the actual time step that we will use. */
  // Note: Chipmunk will not perform as well unless the step stays fixed over time
#if !DYNAMIC_FRAMERATE
  cpFloat dt = (1.0f/FPS) / (cpFloat)substeps;
#else
  cpFloat dt = sinceLastFrame / 1000.0 / (cpFloat)substeps;
#endif
    
    // Automata management
    for(auto_iter i = automata.begin(); i != automata.end(); i++) {
        (*i)->tick();
    }            
    if (!dying_auto.empty()) {
        for(int c = 0; c < dying_auto.size(); c++) {
            automaton *a = *dying_auto[c];
            automata.erase(dying_auto[c]);
            delete a;
        }
        dying_auto.clear();
    }    
        
    int i;
  for(i=0; i<substeps; i++){
//      void handleDrags();
//      handleDrags();
      
	  for(vector<spaceinfo>::iterator _s = spaces.begin(); _s != spaces.end(); _s++) {
		spaceinfo *s = &(*_s);
		if (okayToDraw(s)) {		
			// Iterate space normally
            cpSpaceRehashStatic(s->space);
//			cpSpaceStep(s->space, dt);
            
          cpArray *bodies = s->space->bodies;
            for(int i=0; i<bodies->num; i++) {
                cpBody *body = (cpBody*)bodies->arr[i];
                if (body->data && BDTYPE(body->data) == C_CHIP) {
                    guichip *c = (guichip *)body->data;
                    c->readjust(body->p);
                }
            }
		}
	  }
	}
	
	if (current_xml_update && current_xml_update->done) {
		ERR("\nRESULT\n");
		bool success = loadDocument(current_xml_update->recv, upload_update, true);
		
		if (success) {
			board_loading = false;
		} else {
			// Is this silly? It seems silly. Since we failed at sending the XML, we need
			// to mark the information from the XML as needing resending on the next pass.
			// But the information from the XML has been thrown away-- except, of course,
			// for the copy in the XML itself. So we just... like... load it back out of
			// the XML. Why not? It's a data structure, I guess?
			ERR("(FAIL-- RECOVERING)\n");
			
			do { // So we can break out. FIXME: Pull out into function?
				TiXmlElement *rootxml = (TiXmlElement *)current_xml_update->send.IterateChildren("dc",NULL);
				if (!rootxml || rootxml->Type() != TiXmlNode::ELEMENT) break; // Unthinkable
				TiXmlElement *boardxml = (TiXmlElement *)rootxml->IterateChildren("board", NULL);
				if (!boardxml || boardxml->Type() != TiXmlNode::ELEMENT) break; // Thinkable
				double temp;
				if (TIXML_SUCCESS == boardxml->QueryDoubleAttribute("tempo", &temp)) { // We tried to set the tempo
					ERR("RE-TEMPO\n");
					metro_dirty = true;
				}
				
				TiXmlElement *setxml = (TiXmlElement *)boardxml->IterateChildren("set", NULL);
				if (setxml && setxml->Type() == TiXmlNode::ELEMENT) {	
					TiXmlElement *cxml = NULL;
					while(1) {
						cxml = (TiXmlElement *)setxml->IterateChildren("c", cxml);
						if (!cxml) break;
						int id;
						if (cxml->Type() != TiXmlNode::ELEMENT
							|| TIXML_SUCCESS != cxml->QueryIntAttribute("i", &id))
							continue;
						ERR("RE-ALTER %d\n", id);
						if (mainCircle.chips.count(id)) // If on the last pass we tried to alter a chip
							((chip *)mainCircle.chips[id]->data)->updated = chip::updated_now();
					}
				}
				
				TiXmlElement *delxml = (TiXmlElement *)boardxml->IterateChildren("del", NULL);
				if (delxml && delxml->Type() == TiXmlNode::ELEMENT) {
					TiXmlElement *cxml = NULL;
					while(1) {
						cxml = (TiXmlElement *)delxml->IterateChildren("c", cxml);
						if (!cxml) break;
						int id;
						if (cxml->Type() != TiXmlNode::ELEMENT
							|| TIXML_SUCCESS != cxml->QueryIntAttribute("i", &id))
							continue;
						ERR("RE-DEAD %d\n", id);
						deadchips[id] = NULL;
					}
				}
			} while (0);
		}
		
		delete current_xml_update;
		current_xml_update = NULL;
		if (xml_updating) {
			next_xml_update_at = ticks + xml_update_every * FPS;
		}
	}
	if (next_xml_update_at && ticks>=next_xml_update_at) {
		ERR("SENDNOW\n");
		next_xml_update_at = 0;
		current_xml_update = new netxml_job(xml_update_url);
		if (!next_xml_update_fresh) {
			current_xml_update->have_send = true;
			putDocument(current_xml_update->send, upload_update, true);
		} else {
			next_xml_update_fresh = false;
		}
		upload_update = chip::updated_now();
		current_xml_update->insert();
	}
	
	jobs_service();
}

cpShape * generator_bb::click(cpVect at) {
	if (board_loading)
		return NULL;
    return make_chip(at, ch, true);
}

void netxml_job::execute() { // In this other thread
	string str;
	
	{ 
		ostringstream s;
		s << send;
		str = s.str();
		ERR("Sending '%s'\n",str.c_str());
	}
	
	char *buffer = 0;
	int length = 0;
	char *filename = 0;
	http_parse_url(xml_update_url_copy, &filename);
	xml_update_url_copy = NULL;
	http_retcode code;
	if (!have_send) {
		code = http_get(filename, &buffer, &length, NULL);
	} else {
		code = http_putget(filename, str.c_str(), str.size(), false, "text/xml", 
						   &buffer, &length, NULL);
	}
	
	ERR("%s code %d result %s\n", have_send?"Put":"Get", (int)code, buffer);
	
	str.clear();
			ERR("Received '%s'\n",buffer);
	recv.Parse(buffer); 
	
	done = true;
}

cpShape * ibar_bb::click(cpVect at) {
    switch(t) {
		case ibar_esc: { // NOTE: This should not be the only place we "clear out" online
			next_xml_update_at = 0;
			mainCircle.nowat = 0;
			if (current_xml_update) { // If in-flight network request
				job_discard.push_back(current_xml_update); // Mark for deletion when done
				current_xml_update = NULL;
			}
			if (xml_updating)
				document_clear_out();
			xml_updating = false;
			// TODO: Check next_xml_update_at here also, or otherwise clear out final data
			destroy_all_chips();
			wantClearUi = true;
			want = WMainMenu;
			program_interface();
		} break;
        case ibar_save: {
			wantClearUi = true;
			want = WDisk;
			diskDirection = true;
			program_interface();
        } break;
        case ibar_load: {
			if (xml_updating) break;
			wantClearUi = true;
			want = WDisk;
			diskDirection = false;
			program_interface();
		} break;
        case ibar_reset: {
			destroy_all_chips();
        } break;   
		case ibar_dupe: {
			do_dupe = !do_dupe;
			s = (do_dupe?falter:finter)[ibar_dupe];
		} break;
		case ibar_grid: {
			do_grid = !do_grid;
			s = (do_grid?finter:falter)[ibar_grid];
		} break;
		case ibar_settings: {
			wantClearUi = true;
			want = WSettings;
			program_interface();
		};
    }
    return NULL;
}

void program_metamouse(dragtouch &d, touch_type t, cpVect at) {
	// I suggest keeping this little 4-line sigil at the top so that the interface library will work:
	if (t == touch_down && interface_attempt_click(at, d.button)) // Let the interface library look at the event first
		d.special = dragtouch::interface_special;
	if (d.special == dragtouch::interface_special) // Once a touch has been marked as part of the interface library, ignore it
		return;

	// Not interface-- requires visible board
	if (board_obscured)
		return;
	
    if (t == touch_up || t == touch_cancel) {
        if (d.dragging) {
            cpShape *wasDragging = d.dragging;
            
            wasDragging->body->v = cpvzero;
            
            draggingMe = NULL; lastClick = d.lastClick;
            cpSpacePointQuery(spaces[0].space, d.lastClick, CP_ALL_LAYERS, CP_NO_GROUP, mouseDownOnShape, (void *)C_BAR); // Last arg is mask
            if (draggingMe) {
                destroy_chip(wasDragging->body);
            }
            
            d.dragging = NULL; // These two lines unnecessary on ipad, necessary on PC
            return; // Do for all cancels?
        }
        if (d.special == dragtouch::metro_special)
            metro_special_happening = false;
        d.special = dragtouch::not_special; // Mostly for the "mouse" dragtouch, which "survives"
	}
	goOrtho();
	d.lastClick = screenToGL(at.x, at.y, spaces[0].depth);
	if (t == touch_down) {
        draggingMe = NULL; lastClick = d.lastClick;
        cpSpacePointQuery(spaces[0].space, lastClick, CP_ALL_LAYERS, CP_NO_GROUP, mouseDownOnShape, (void *)C_BAR); // Last arg is mask
        
        if (draggingMe && draggingMe->body->data) { // If bar connected
            bardata *bd = (bardata *)draggingMe->body->data;
            cpFloat across = (draggingMeInto.x + 1/aspect) / (2/aspect);
            int idx = across * bd->button_count();
            if (idx == bd->button_count()) idx--; // Is overflow possible? Why risk it
            ERR("ACROSS %lf -> %d\n", (double)across, idx);
            barbutton *button = bd->button(idx);
            draggingMe = button->click( cpvadd(draggingMeInto, draggingMe->body->p) );
			d.special = dragtouch::untwist_special;
        } else {
            draggingMe = NULL; lastClick = d.lastClick; // Should be active:
            cpSpacePointQuery(spaces[0].space, lastClick, CP_ALL_LAYERS, CP_NO_GROUP, mouseDownOnShape, (void *)C_CHIP_OUTER); // Last arg is mask
			chip *draggingChip = draggingMe ? (chip *)draggingMe->body->data : NULL;
			
			if (draggingMe) { // If chip connected
				d.special = dragtouch::untwist_special;
				if (doPlayFeedback) draggingChip->wildfire = true;
			}
			
			bool this_is_a_dupe = do_dupe ^ (DUPE_BUTTON(d.button));
			
			if (draggingMe && this_is_a_dupe) {
				draggingMe = make_chip(lastClick, draggingChip->ip);
				draggingMeInto = cpvzero;
			}			
        }
        
        d.dragging = draggingMe;
        d.draggingInto = draggingMeInto;
		        
        if (!d.dragging) {
            cpFloat metro_dist = cpvlengthsq(cpvsub(metro_at_drawn, d.lastClick)); // Would be really easy to calculate this on the fly here, but why bother
            if (metro_dist < BASE_CHIP*BASE_CHIP*4) {
                d.special = dragtouch::metro_special;
                metro_special_happening = true; // Do we need both these?
            } else {
                draggingMe = NULL; lastClick = d.lastClick;
                cpSpacePointQuery(spaces[0].space, lastClick, CP_ALL_LAYERS, CP_NO_GROUP, mouseDownOnShape, (void *)C_TOGGLE); // Last arg is mask
                if (draggingMe) {
                    mainCircle.toggle_fire = true;
                    toggle_fire_at = ticks;
					d.special = dragtouch::untwist_special;
                }
            }
        }
    }        
    if (d.dragging) {
        draggingMe = NULL; lastClick = d.lastClick;
		if (do_grid)
			cpSpacePointQuery(spaces[0].space, lastClick, CP_ALL_LAYERS, CP_NO_GROUP, mouseDownOnShape, (void *)C_GUIDE); // Last arg is mask
		cpBody *body = d.dragging->body;
        if (draggingMe) {
            body->p = draggingMe->body->p;
        } else {
            body->p = d.lastClick;
        }
		((chip *)body->data)->updated = chip::updated_now();
    }
    if (d.special == dragtouch::metro_special) {
        mainCircle.metro_at = cpvlength(d.lastClick)/radmin;
        if (mainCircle.metro_at > 1) mainCircle.metro_at = 1;
		metro_dirty = true;
    }
}

// Give the interface library the chance to respond to a click or touch; return true if the interface "ate" the click.
// Lives in util_display because it uses matrix stuff
// Note: Button should always be 0 on mobile
bool interface_attempt_click(cpVect screen_coord, int button) {
	goOrtho();
	void jcLoadIdentity(); jcLoadIdentity();
	cpVect at = screenToGL(screen_coord.x, screen_coord.y, 0);
	
	clickConnected = false;
	
	if (!(button == 2 || button == 4 || button == 5)) { // Left click only
		cpSpacePointQuery(workingUiSpace(), at, CP_ALL_LAYERS, CP_NO_GROUP, clicked, NULL);
	} else { // Scroll wheel
		int dir = 0;
		if (button == 4) dir = 1;
		if (button == 5) dir = -1;
		cpSpacePointQuery(workingUiSpace(), at, CP_ALL_LAYERS, CP_NO_GROUP, wheeled, (void *)dir);
	}
	
	if (clickConnected) { // Somewhere above, a control event has occurred.
		program_interface();
	}
	
	return clickConnected;
}

#ifdef TARGET_DESKTOP

void program_eventkey(SDL_Event &event) {
    SDLKey &key = event.key.keysym.sym;

	if (event.type == SDL_KEYDOWN) {
		int bdir = 0;
		if (key == SDLK_UP) {
			bdir = -1;
		}
		if (key == SDLK_DOWN) {
			bdir = 1;
		}
		if (bdir) { // This is a little ugly. I'm bothered last_inventory is being relied on so much.
			last_inventory->active = (last_inventory->active + bdir + last_inventory->bars.size()) 
				% last_inventory->bars.size();
		}
		

		if (!board_obscured && !xml_updating && key >= '1' && key <= '8') { // TODO: (SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL))
			int which = key-'1'+1;
			char filename[FILENAMESIZE];
			userPath(filename, "save%d.xml", which);
			TiXmlDocument xml(filename);
			if (xml.LoadFile()) {
				destroy_all_chips();
				loadDocument(xml);		
			}
		}
	}
}

// This is called when SDL gets a joystick event.
void program_eventjoy(SDL_Event &event) {
}

int debug_mouse_last = -1;
// This is called when SDL gets a mouse event.
static void eventmouse_fly(SDL_Event &event) {
    if (event.button.which >= MOSTBUTTONS) return;
    
	if (event.button.type == SDL_MOUSEBUTTONDOWN) {
        have_debug_mouse[event.button.button] = true;
        debug_mouse_last = event.button.button;
        debug_mouse[event.button.button] = screenToGL(event.button.x, event.button.y, 0);
        ERR("Mouse %d, x %lf, y %lf\n", event.button.button, debug_mouse[event.button.button].x, debug_mouse[event.button.button].y);
	}
	if (event.button.type == SDL_MOUSEBUTTONUP) {
        have_debug_mouse[event.button.button] = false;
        debug_mouse_last = -1;
	}
    if (event.button.type == SDL_MOUSEMOTION) {
        if (debug_mouse_last >= 0) // Can I trust state instead of bothering with this?
            debug_mouse[debug_mouse_last] = screenToGL(event.button.x, event.button.y, 0);
    }
}

dragtouch globaldrag;

// A highly ad hoc system, contained totally within eventmouse_game_gesture.
bool is_righting; // Mouse is currently being devoured by right-click.
cpVect right_at; // Right-click started at.
cpBody *right_at_target = NULL;

static bool rightclick_steal(const SDL_Event &event, const cpVect &event_at, touch_type t) {
	bool unright_after = false;
	
	if (PITCHSHIFT_BUTTON(event.button.button) || event.button.button == 4 || event.button.button == 5) {  // Right click-- special.
		if (t == touch_down) {
			is_righting = true;
			right_at = event_at;
			cpVect internal_at = screenToGL(event_at.x, event_at.y, spaces[0].depth);
			
			// Duped from metamouse -- chip check
			right_at_target = NULL; draggingMe = NULL; // Should be active:
            cpSpacePointQuery(spaces[0].space, internal_at, CP_ALL_LAYERS, CP_NO_GROUP, mouseDownOnShape, (void *)C_CHIP_OUTER); // Last arg is mask
			
			if (draggingMe) {
				guichip *ch = ((guichip *)draggingMe->body->data);
				right_at_target = draggingMe->body;
				ch->twistStart();
				if (event.button.button == 4 || event.button.button == 5) {
					ch->twistUpdate(event.button.button == 4 ? 1 : -1);
					is_righting = false;
					return true;
				}
			}
		}
	}	

	if (t == touch_up && is_righting) {
		unright_after = true;
		right_at_target = NULL; // Unnecessary but let's be paranoid
	}
	
	if (is_righting) {
		if (right_at_target) { // TODO: Put in a function on chip
			guichip *target_chip = ((guichip *)right_at_target->data);
			target_chip->twistUpdate( -(event_at.y - right_at.y)/4 );
		}
		
		if (unright_after)
			is_righting = false;	

		return true;
	}
		
	return false;
}

static void eventmouse_game_gesture(SDL_Event &event) {
    touch_type t = touch_move;
	cpVect event_at = cpv(event.button.x, event.button.y);
	
    if (event.type == SDL_MOUSEBUTTONDOWN)
        t = touch_down;
    if (event.type == SDL_MOUSEBUTTONUP)
        t = touch_up;
	
	bool rightclick_ate = !board_obscured && rightclick_steal(event, event_at, t);
	
	globaldrag.button = event.button.button;
	if (!rightclick_ate)
		program_metamouse(globaldrag, t, event_at);
}

void program_eventmouse(SDL_Event &event) {
    switch(run_mode) {
        case run_game:
            eventmouse_game_gesture(event);
            break;
        default:
            eventmouse_fly(event);
            break;
    }
}

#else

typedef list<touch_rec>::const_iterator touches_iter;
struct touch_state : touch_rec {
    dragtouch d;
    touch_state() : touch_rec(0, cpvzero) {}
    touch_state(const touch_rec &r) : touch_rec(r) {}
};
typedef hash_map<touch_id, touch_state>::iterator alltouches_iter;
hash_map<touch_id, touch_state> allTouches; 

void dragtouch::updateTwistLast() {
	twistLast = cpvtoangle(cpvsub(lastClick, twistDrag->body->p));
}

void dragtouch::updateTwistOff() {
	cpFloat prevTwist = twistLast;
	updateTwistLast();
	cpFloat delta = twistLast - prevTwist;
	if (delta > M_PI) delta -= 2*M_PI;
	else if (delta < -M_PI) delta += 2*M_PI;
	twistOff += delta;
	((chip *)twistDrag->body->data)->twistUpdate( (twistOff + twistPartner->twistOff)/(2*M_PI)*360/8 );
}

void dragtouch::forgetTwist() {
	if (twistPartner) {
		twistPartner->twistPartner = NULL;
		twistPartner->twistDrag = NULL;
	}
	twistPartner = NULL;
	twistDrag = NULL;	
}

void program_eventtouch(const list<touch_rec> &touches, touch_type kind) {
    // Update alltouches
    for(touches_iter i = touches.begin(); i != touches.end(); i++) {
        switch(kind) {
            case touch_down: {
                touch_state &newstate = allTouches[i->tid];
                newstate = touch_state(*i);
                
                program_metamouse(newstate.d, kind, i->at);
				
				ERR("Special? %d (not %d)\n", (int) newstate.d.special, (int)dragtouch::not_special);
				
				// Check for twister
				if (newstate.d.special == dragtouch::not_special) for (alltouches_iter a = allTouches.begin(); a != allTouches.end(); a++) {
#if TWIST1_DEBUG
					twisthit(cpvmult(cpvadd(a->second.d.lastClick, newstate.d.lastClick), 0.5));
#endif
					if (a->first == i->tid || a->second.d.special != dragtouch::not_special)
						continue;
					cpVect center = cpvmult(cpvadd(a->second.d.lastClick, newstate.d.lastClick), 0.5);
					draggingMe = NULL; lastClick = center;
					cpSpacePointQuery(spaces[0].space, lastClick, CP_ALL_LAYERS, CP_NO_GROUP, mouseDownOnShape, (void *)C_CHIP_OUTER); // Last arg is mask
					if (draggingMe) {
						chip *ch = (chip *)draggingMe->body->data;
						a->second.d.special = dragtouch::untwist_special; // Poison for future twists
						a->second.d.twistDrag = draggingMe;				  // Who am I dragging?
						a->second.d.twistPartner = &newstate.d;			  // Who am I dragging with?
						a->second.d.updateTwistLast();					  // Where am I dragging from?
						newstate.d.special = dragtouch::untwist_special;
						newstate.d.twistDrag = draggingMe;
						newstate.d.twistPartner = &a->second.d;
						newstate.d.updateTwistLast();						
						ch->twist = &newstate.d; // Let twistee know who's dragging it
						ch->twistStart(); // Let twistee know it's twisting
					}
				}
			} break;
            case touch_move: {
                touch_state &oldstate = allTouches[i->tid];
                oldstate.at = i->at;
                program_metamouse(oldstate.d, kind, i->at);
				if (oldstate.d.twistDrag) {
					oldstate.d.updateTwistOff();
				}
            } break;
            case touch_up: case touch_cancel: {
                touch_state &oldstate = allTouches[i->tid];
                program_metamouse(oldstate.d, kind, i->at);
				if (oldstate.d.twistDrag) {
					chip *ch = (chip *)oldstate.d.twistDrag->body->data;
					ch->twist = NULL;
					oldstate.d.forgetTwist();
				}				
                allTouches.erase(i->tid);
            } break;
        }
    }
}

void program_sleep() {
	ERR("Going to sleep\n");
}

void program_wake() {
	ERR("Waking up\n");
}

#endif

void handleDrags() {
#ifdef TARGET_DESKTOP
    {
        dragtouch &d = globaldrag;
#else
    for (alltouches_iter i = allTouches.begin(); i != allTouches.end(); i++) {
        const dragtouch &d = i->second.d;
#endif
        if (d.dragging) {
            cpVect force = cpvmult(cpvsub(d.lastClick, d.dragging->body->p), FPS);
            //		  ERR("V %f, %f\n", mouseBody->p.x, mouseBody->p.y, force.x, force.y);
            d.dragging->body->v = force;		  
        }                
    }
}
	
enum MiscButtonType { mb_game, mb_online, mb_named_online, mb_sett_save, mb_return_board, mb_save_wav, mb_wav_screen, mb_help };

// Temporary "last seen" pointers for settings widgets-- I really hate doing this.
WheelControl *settAmp = NULL;
SelectControl *settKey = NULL;
CheckControl *settAdvertise = NULL, *settLetters = NULL;
	
	
void start_game(bool online) {
	wantClearUi = true;
	display_board = true;
	board_obscured = false; // Redundant / safety
	board_loading = online;
	if (online) {
		xml_update_url = xml_update_url_base + "?p=" + FULLVERSION;
		xml_update_every = 1;
		xml_updating = true;
		next_xml_update_fresh = true;
		need_initial_time_sync = true;
		board_loading = true;
		next_xml_update_at = ticks;
		upload_update = chip::updated_now();
		updateCookie.clear();		
	}
}
	
class MiscButton : public ControlBase { public:
	MiscButtonType t; void *data;
	MiscButton(string _text, MiscButtonType _t, void *_data = NULL) : ControlBase(_text, true), t(_t), data(_data) {}
	virtual void click() {
		switch (t) {				
			case mb_online: {
				bool control_mod = false;
#ifdef TARGET_DESKTOP
				SDLMod mods = SDL_GetModState();
				control_mod = (mods & (KMOD_LCTRL | KMOD_RCTRL));
#endif
				if (control_mod) { // ATM default to not asking
					wantClearUi = true;
					want = WOnlineConnect;
				} else {
					start_game(true);
				}
			} break;
			case mb_named_online: {
				TextControl *nameFrom = (TextControl *)data;
				xml_update_url_base = nameFrom->text;
				ERR("CONNECT TO %s\n", xml_update_url.c_str());
				start_game(true);
			} break;
			case mb_game: {
				make_clutter();
				start_game(false);
			} break;
			case mb_sett_save: {
				int new_key_off = settKey->is+0.5;
				if (key_off != new_key_off || doLetters != settLetters->checked) { // Messy, but works. Rebuild "inventory" bar if key changed
					key_off = new_key_off;
					doLetters = settLetters->checked;
					if (last_inventory) { // Should never fail
						for(int c = 0; c < last_inventory->bars.size(); c++)
							delete last_inventory->bars[c];
						last_inventory->bars.clear();
						populate_all(last_inventory);
					}
				}
				doPlayFeedback = settAdvertise->checked;
				doTextFeedback = doPlayFeedback;
			} // FALL THROUGH
			case mb_return_board: {
				wantClearUi = true;
				board_obscured = false;
			} break;
			case mb_wav_screen: {
				wantClearUi = true;
				want = WSaveWav;
			} break;
			case mb_save_wav: {
				TextControl *nameFrom = (TextControl *)data;
				string filename = nameFrom->text;
				char path[FILENAMESIZE];
				userPath(path, filename.c_str());
				ERR("SAVE TO %s\n", path);
				wav_write(path);
				
				wantClearUi = true;
				board_obscured = false;
			} break;				
			case mb_help: {
#ifdef DO_HELPBUTTON
				open_url("http://runhello.com/local/dc/help/");
#endif
			} break;
		}
	}
};
	
class FileButton : public ControlBase { public:
	string filename; bool isSave, needConfirm;
	FileButton(string _text, string _filename, bool _isSave, bool waiveConfirm = false)
		: ControlBase(_text, true), filename(_filename), isSave(_isSave), needConfirm(false)
	{
		bool exists = false;
		if (!waiveConfirm) { FILE *f = fopen(filename.c_str(), "r"); if (f) { exists = true; fclose(f); } }
		ERR("OK %s : %s; %s\n", filename.c_str(), exists?"YES":"NO", isSave?"SAVE":"");
		if (isSave) {
			if (!waiveConfirm) {
				highlighted = exists;
				needConfirm = exists;
			}
		} else {
			bg = exists;
		}
	}
	virtual void click() {
		if (!bg) return; // Not a good idea
		if (needConfirm) {
			want = WDiskConfirm;
			diskDirection = isSave; // Does nothing!
			diskTarget = filename;
			diskTargetName = "save "; diskTargetName += text;
		} else {
			if (isSave) {
				TiXmlDocument xml(filename);
				
				putDocument(xml);
				
				xml.SaveFile();				
			} else {
				TiXmlDocument xml(filename);
				xml.LoadFile();
				
				destroy_all_chips();
				loadDocument(xml);				
			}
			board_obscured = false;
		}
		wantClearUi = true;
	}
};

// The "this is a good time to modify your uiSpace" function. Main.cpp calls this once at the beginning of the main
// loop and once every time a button is successfully clicked. This exists because it's unsafe to modify the interface
// from inside of a ControlBase click() handler, so instead click() handlers can stash some state that this function
// then picks up when main.cpp automatically calls it after the click is completed...
void program_interface() {
	WantNewScreen wantAfterwards = WNothing; // "want" will be cleared with this at the end of the function
	
	if (wantClearUi) {
		for(int c = 0; c < columns.size(); c++)
			if (columns[c])
				delete columns[c];
		
		columns.clear();
		
		resetScrollmax();
		wantClearUi = false;
	}
	
	switch (want) {	
        case WMainMenu: {// DELETE ME
			display_board = false; 

            ContainerBase *checkboxes = new ContainerBase(uiSpace, CMID);
			columns.push_back(checkboxes);

            checkboxes->add( new ControlBase("Drumcircle") );
			checkboxes->add( new ControlBase("") );
			checkboxes->add( new MiscButton("Play", mb_game) );
			checkboxes->add( new MiscButton("Online", mb_online) );
#ifdef DO_HELPBUTTON
			checkboxes->add( new MiscButton("Help", mb_help) );
#endif

            checkboxes->commit();
            
		} break;
		case WOnlineConnect: {
			ContainerBase *checkboxes = new ContainerBase(uiSpace, CMID);
			columns.push_back(checkboxes);
			checkboxes->add( new ControlBase("Server") );
			MiscButton *enter = new MiscButton("Join", mb_named_online);
			ControlBase *text = new TextControl(xml_update_url_base, true, enter);
			enter->data = text;
			checkboxes->add( text );
			checkboxes->add( enter );
			checkboxes->commit();
		} break;
		case WDisk: {
			board_obscured = true;
			ContainerBase *checkboxes;
			bool needWav
#ifdef TARGET_DESKTOP
				= diskDirection;
#else
				= false;
#endif
			
			checkboxes = new ColumnContainer(uiSpace, CMID);
			columns.push_back(checkboxes);

			for(int c = 1; c <= 8; c++) {
				char filename[FILENAMESIZE];
				snprintf(filename, FILENAMESIZE, "%d", c);
				string name = filename;
				userPath(filename, "save%d.xml", c);
				ControlBase *control = new FileButton(name, filename, diskDirection);
				checkboxes->add( control );
			}
			for(int c = 0; c < 2; c++)
				checkboxes->add( new ControlBase() );
			checkboxes->commit();	
			
			checkboxes = new ContainerBase(uiSpace, CMID);
			columns.push_back(checkboxes);
			checkboxes->add( new ControlBase(diskDirection?"Save:":"Load:") );
			for(int c = 0; c < (needWav ? 4 : 5); c++)
				checkboxes->add( new ControlBase() );
			if (needWav)
				checkboxes->add(new MiscButton("Export wav...", mb_wav_screen));
			checkboxes->add(new MiscButton("Cancel", mb_return_board));
			checkboxes->commit();
		} break;
		case WDiskConfirm: {
			ContainerBase *checkboxes = new ContainerBase(uiSpace, CMID);
			columns.push_back(checkboxes);
			string query = "Overwrite "; query += diskTargetName; query += "?";
			checkboxes->add(new ControlBase(query));
			checkboxes->add(new FileButton("Yes", diskTarget, true, true));
			checkboxes->add(new MiscButton("Cancel", mb_return_board));
			checkboxes->commit();
		} break;
		case WSaveWav: {
			ContainerBase *checkboxes[3] = { new ContainerBase(uiSpace, CMID), new ContainerBase(uiSpace, CMIDL), new ContainerBase(uiSpace, CMIDR) };
			MiscButton *enter = new MiscButton("Save", mb_save_wav);
			ControlBase *text = new TextControl("FILENAME", true, enter);
			enter->data = text;
			
			int wf = 1;
			while (wf) { // This is sorta ugly.
				struct stat buf;
				char filename[FILENAMESIZE];
				userPath(filename, "output%d.wav", wf);
				if (0 <= stat(filename, &buf)) {
					wf++;
					continue;
				}
				
				// If we're still here, the file doesn't exist.
				snprintf(filename, FILENAMESIZE, "output%d.wav", wf);
				text->text = filename;
				wf = 0; // Exit loop
			}
			
			checkboxes[0]->add(new ControlBase("Save as:"));
			
			checkboxes[0]->add(text);
			for(int c = 0; c < 3; c++)
				for(int d = 0; d < 3; d++)
					if (c || d) // Omit 0,0
						checkboxes[c]->add(new ControlBase(""));
												
			checkboxes[1]->add(new MiscButton("Cancel", mb_return_board));
			checkboxes[2]->add(enter);
			
			for(int c = 0; c < 3; c++) {
				columns.push_back(checkboxes[c]);
				checkboxes[c]->commit();
			}
		} break;
		case WSettings: {
			board_obscured = true; 
			ContainerBase *checkboxes;
			
			vector<string> options;
			for(int c = 0; c < SCALESIZE; c++) {
				string k = scale[c]; k += "-major";
				options.push_back(k);
			}			
			
#ifdef TARGET_DESKTOP
			settAmp = new DoubleBound(&mainCircle.amp, 0, 16, 0.1); 
#endif
			settKey = new SelectControl(options, key_off, false);
			settAdvertise = new CheckControl("Play on\ntouch", true); if (doPlayFeedback) settAdvertise->click();
			settLetters = new CheckControl("Labels", true); if (doLetters) settLetters->click();			
						
			checkboxes = new ContainerBase(uiSpace, CMIDL);
			columns.push_back(checkboxes);
			checkboxes->add( new ControlBase("Key:") );
			if (settAmp) checkboxes->add( new ControlBase("Preamp:") );
			for(int c = 0; c < 4; c++)
				checkboxes->add( new ControlBase("") );
			checkboxes->commit();
			
			checkboxes = new ContainerBase(uiSpace, CMIDR);
			columns.push_back(checkboxes);
			checkboxes->add( settKey );
			if (settAmp) checkboxes->add( settAmp );
			for(int c = 0; c < 4; c++)
				checkboxes->add( new ControlBase("") );
			checkboxes->commit();

			checkboxes = new ContainerBase(uiSpace, CMID);
			columns.push_back(checkboxes);
			
			checkboxes->add( new ControlBase("") );
			if (settAmp) checkboxes->add( new ControlBase("") );
			checkboxes->add( settAdvertise );
			checkboxes->add( settLetters );
			checkboxes->add( new ControlBase("") );
			checkboxes->add( new MiscButton("Save", mb_sett_save) );
			checkboxes->commit();			
		} break;
	}
	
	want = wantAfterwards;
}

// Called when "ESC" is pressed-- gives you a last chance to do something like throw up an "are you sure?" screen
void BackOut() {
	Quit(); // Default implementation just quits.
}

// Called after SDL quits, right before program terminates. 
// TODO: Verify works with iPhone, Android
// Gives you a chance to save to disk anything you want to save.
void AboutToQuit() {
	if (xml_updating)
		document_clear_out();
	jobs_terminate();
}

bannerstream::~bannerstream() {
    displaying = str();
    displaying_life = life;
}

// JOBS
// TODO: Move jobs/auto into a jumpcore_util.cpp or something
list<job *> job_current;
list<job *> job_discard;
#if !JOB_NOTHREAD
pthread_mutex_t job_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t job_thread;
pthread_cond_t job_wait;
bool job_in_progress = false;
	
void *run_job_thread(void *) {
	while (1) {
		job *perform = NULL;
		pthread_mutex_lock(&job_mutex);
		while (job_current.empty())
			pthread_cond_wait(&job_wait, &job_mutex);
		perform = job_current.front();
		job_current.pop_front();
		job_in_progress = perform; // For benefit of jobs_terminate
		pthread_mutex_unlock(&job_mutex);
		if (perform)
			perform->execute();
		job_in_progress = false;
	}
	return NULL;
}
#endif
	
void jobs_init() {
#if !JOB_NOTHREAD
	pthread_cond_init(&job_wait, NULL);
	pthread_create(&job_thread, NULL, run_job_thread, NULL);
#endif
}

// Yes, this IS linear on the number of discards. Avoid having a lot of discards!?
// TODO: Make this... like... not... linear. There's something that lets you iterate while deleting, use that.
void jobs_service() {
	for(job_iter i = job_discard.begin(); i != job_discard.end();) {
		if ((*i)->done) {
			delete *i;
			i = job_discard.erase(i);
		} else {
			i++;
		}
	}
}

// Chance for cleanup if program terminates while jobs are still running.
// Call from main thread.
// This looks a little rube goldberg-y to me-- the way it works is it iterates over
// the jobs list again and again until it's recorded each item has received one halt().
// It might be there's a simpler way to do this but I want to be really paranoid about
// not causing problems or deadlock if the jobs list is modified from the other thread
// while this process is running.
// OVERALL THIS TURNED OUT MORE COMPLICATED THAN I THOUGHT IT WOULD NEED TO BE. ELIMINATE?
void jobs_terminate() {
#if !JOB_NOTHREAD 
	ERR("WILL TERMINATE...\n");
	job *needs_halt;
	hash_map<void *, int> handled;
	do {
		needs_halt = NULL;
		pthread_mutex_lock(&job_mutex);
		while (job_in_progress) // NOT SUITABLE FOR JUMPCORE
			msleep(1);
		for(job_iter i = job_current.begin(); i != job_current.end(); i++) {
			ERR("%p...\n", *i);
			if (!handled.count(*i) && (*i)->want_halt()) {
				ERR("CAUGHT\n");
				handled[*i] = 1;
				needs_halt = *i;
				break; // Exit loop and relinquish lock so that worker thread gets a chance.
			}
		}
		pthread_mutex_unlock(&job_mutex);
		if (needs_halt) {
			needs_halt->halt();
			msleep(1);
		}
	} while (needs_halt);
#endif
}
	
void job::insert() {
#if JOB_NOTHREAD
	execute();
#else
	pthread_mutex_lock(&job_mutex);
	job_current.push_back(this);
	pthread_cond_signal(&job_wait);
	pthread_mutex_unlock(&job_mutex);	
#endif
}

// FIXME: Lots of problems with this. First off it's an ugly spinlock.
// Second off there should be a "timeout" argument in case the job blocks forever.
// Finally: What is the appropriate JOB_NOTHREAD behavior?
void job::wait() {
	ERR("Waiting %p...\n", this);
	while (!done) {
		msleep(1);
	}
	ERR("Finished %p.\n", this);
}
