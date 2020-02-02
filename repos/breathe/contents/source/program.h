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

#define PLAYTEST_RECORD 0
#define PHYSICS2D 1	
#define PHYSICS3D 1

#include "Polycode.h"

#if PHYSICS2D
#include "Polycode2DPhysics.h"
#endif

#if PHYSICS3D
#include "Polycode3DPhysics.h"
#endif

#include <stdint.h>
#include <algorithm>
#include <list>

using namespace std;

#include "cpVect.h"
#include "cpRect.h"

using namespace Polycode;

#include "automaton.h"

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

extern Core *cor;
extern int fullscreen_width, fullscreen_height, surface_width, surface_height;
extern cpRect sbound;

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
typedef hash_map<string, Entity *>::iterator obj_iter;

// This class was written for luanauts. Is it cruft or is it general enough to keep around? --mcc
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
	Scene *scene;
	string spec;
	bool fake;
	Entity *a, *b;
	hash_map<string, Entity *> obj;
	hash_map<void *,string> obj_name;
	hash_map<void *,storedfunc *> onCollide;
	vector<string> onLoad, onClose;
	string screenClass, sceneClass;
	vector<storedfunc *> onUpdate;
	
	// "Spec" is a comma-separated list of .
	// "Fake" is a bool which if true, nothing will actually be loaded until the next rebirth-- this is for debugging.
    room_auto(const string &_spec = string(), bool _fake = false);
	
	virtual ~room_auto();
	virtual void insert();
	virtual void die();
	virtual void tick();
	void rebirth(bool _fake = false);
	virtual void handleEvent(Event *e);
	void loadFromFile(Object &overlay, const string &filename);
	void loadFromOverlay(Object &overlay);
	void clear_stored_functions();
	void screen_has_class(string name);
	void scene_has_class(string name);
	void doCollide(Entity *acting, Entity *against, bool first);
	
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
bool endsWith(const string &str, const string &suffix);
string filedump(const string &path);

#endif /* _PROGRAM_H */
