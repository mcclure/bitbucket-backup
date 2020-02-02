#ifndef _PROGRAM_H
#define _PROGRAM_H

/*
 *  Program.h
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 11/22/11.
 *  Copyright 2011 Run Hello. All rights reserved.
 *
 */

#include "Polycode.h"
#include "Polycode3DPhysics.h"

#include <stdint.h>
#include <algorithm>
#include <list>
using namespace std;

#include "cpVect.h"
#include "cpRect.h"

using namespace Polycode;

// Jumpcore lite

#define REALERR(...) fprintf (stderr, __VA_ARGS__)
#define EAT(...)
#if _DEBUG
#define ERR REALERR
#else
#define ERR EAT
#endif

// On mac, pwd is inside the package
#if defined(__APPLE__) && defined(__MACH__)
#define PROGDIR "../../../"
#else
#define PROGDIR ""
#endif

class ExtCollisionScene;

struct automaton;
typedef list<automaton *>::iterator auto_iter;
extern list<automaton *> automata;
extern Core *cor;
extern int ticks;
extern int fullscreen_width, fullscreen_height, surface_width, surface_height;
extern cpRect sbound;

// manages "animations"
struct automaton : EventHandler {
	bool done;
    int born;  // ticks born at
    int frame; // ticks within state
    int state; // which state are you at?
    int rollover; // what frame does state roll at?
    auto_iter anchor;
	vector<EventDispatcher *> owned_screen;
    automaton();
    inline int age() { return ticks-born; }
    virtual void tick();
    virtual void die();
    virtual void insert() {
        automata.push_back(this);
        anchor = --automata.end();
    }
    virtual ~automaton() {}
};

#define IS_MODIFIER(x) ((x) >= KEY_NUMLOCK && (x) <= KEY_COMPOSE)
#define IS_ARROW(x) ((x) >= KEY_UP && (x) <= KEY_LEFT)

// Real quick, some magical incantations to allow the use of hash_map:
#include <ext/hash_map>
using namespace ::__gnu_cxx;
namespace __gnu_cxx {                                                                                             
	template<> struct hash< std::string > // Allow STL strings with hash_map
	{ size_t operator()( const std::string& x ) const { return hash< const char* >()( x.c_str() ); } };          
	template<> struct hash< void * > // Allow pointers with hash_map               
	{ size_t operator()( const void * x ) const { return hash< uintptr_t >()( (uintptr_t)x ); } };          
}          

// Beth

struct storedfunc;
typedef hash_map<string, SceneEntity *>::iterator obj_iter;

struct save_file { // TODO: Would be so cool if I could move this into Lua.
	int priority;
	string file;
	save_file();
	void load();
	void save();
};
extern save_file save;

struct room_auto : public automaton {
	Screen *screen;
	ExtCollisionScene *scene;
	string svgname, overlayname;
	bool fake;
	SceneEntity *a, *b;
	hash_map<string, SceneEntity *> obj;
	hash_map<void *,string> obj_name;
	hash_map<void *,storedfunc *> onCollide;
	string onLoad, onClose;
	storedfunc *onUpdate;

	SceneEntity *block;
	Vector3 playerAt;
	vector<Vector3> diamondsAt;
	vector<SceneEntity *> carpetsAt;
	
    room_auto(const string &_svgname = string(), const string &_overlayname = string(), bool _fake = false);
	virtual ~room_auto();
	virtual void insert();
	virtual void die();
	virtual void tick();
	void rebirth(bool _fake = false);
	virtual void handleEvent(Event *e);
	void loadFromOverlay(Object &overlay);
	void clear_stored_functions();
	void doCollide(SceneEntity *acting, SceneEntity *against, bool first);
	static room_auto *singleton() { return _singleton; }
protected:
	static room_auto *_singleton;
};

#if _DEBUG
#define SELF_EDIT 1
#endif

#define RANDOM_MAX ((((unsigned int)1)<<31)-1)

#ifdef _WINDOWS

#include <time.h>
#define srandom srand
inline long random() {
	unsigned long r1 = rand();
	unsigned long r2 = rand();
	return ((r1 << 16)|r2) & RANDOM_MAX;
}

#endif

void Quit();

inline void Clear(Object &o) { o.root.Clear(); }
void loadFromTree(Object &o, const string &path);

#endif /* _PROGRAM_H */
