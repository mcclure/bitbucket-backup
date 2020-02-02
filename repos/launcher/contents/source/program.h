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
#define PHYSICS3D 0
#define UI 1

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
// (FIXME: Will this work on MSVC?)
#include <ext/hash_map>
using namespace ::__gnu_cxx;
namespace __gnu_cxx {                                                                                             
	template<> struct hash< std::string > // Allow STL strings with hash_map
	{ size_t operator()( const std::string& x ) const { return hash< const char* >()( x.c_str() ); } };          
	template<> struct hash< void * > // Allow pointers with hash_map               
	{ size_t operator()( const void * x ) const { return hash< uintptr_t >()( (uintptr_t)x ); } };          
}          

// Polyconsole

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

typedef hash_map<void *,storedfunc *>::iterator handler_iter;

struct room_auto : public automaton {
	Screen *screen;
	Scene *scene;
	string spec;
	bool fake, bailed, clickHandler;
	Entity *a, *b;
	hash_map<string, Entity *> obj;
	hash_map<void *,string> obj_name;
	hash_map<void *,storedfunc *> onCollide;
	hash_map<void *,storedfunc *> onClick; // TODO: Generalize.
	vector<string> onLoad, onClose;
	string screenClass, sceneClass;
	vector<storedfunc *> onUpdate;
	vector<Shader*> ownedShaders; // For deletion
	vector<Material*> ownedMaterials; // For deletion
	
	hash_map<void *,void *> clickTrack;
	
	// "Spec" is a comma-separated list of .
	// "Fake" is a bool which if true, nothing will actually be loaded until the next rebirth-- this is for debugging.
    room_auto(const string &_spec = string(), bool _fake = false);
	
	virtual ~room_auto();
	virtual void insert();
	virtual void die();
	virtual void tick();
	void rebirth(bool _fake = false);
	virtual void handleEvent(Event *e);
    void loadMaterialsFile(const string &);
	void loadFromFile(Object &overlay, const string &filename);
	void loadSvgFromFile(Object &overlay, const string &filename, Screen *into, const string &loadClass, bool readBackground = true);
	void loadFromOverlay(Object &overlay);
	void bail(); // Halt all execution
	void add_update_function(const string &);
	void clear_room_objects();
	void screen_has_class(string name);
	void scene_has_class(string name);
	void needClickHandler();
	void doCollide(Entity *acting, Entity *against, bool first);
	
	static room_auto *singleton() { return _singleton; }
protected:
	static room_auto *_singleton;
};

#if _DEBUG
#define SELF_EDIT 1
#else
#define SELF_EDIT 0
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
string filedump_external(const string &path);
string filedrain(FILE *f);
template <class T> void vectorPush(std::vector<T>into, std::vector<T>from) {
    for(int c = 0; c < from.size(); c++)
        into.push_back(from[c]);
}

int intCheck(ObjectEntry *o, const string &key = string(), int def = 0);
int intCheckPure(ObjectEntry *o, const string &key = string(), int def = 0);
Number numCheck(ObjectEntry *o, const string &key = string(), Number def = 0);
string strCheck(ObjectEntry *o, const string &key = string());
ObjectEntry *objCheck(ObjectEntry *o, const string &key);

extern ObjectEntry *userSettings;
extern bool debugMode;
extern ObjectEntry *debugSettings;

inline string S(const char *s) { return s ? string(s) : string(); }

#endif /* _PROGRAM_H */
