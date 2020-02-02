/*
 *  bridge.cpp
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 12/26/11.
 *  Copyright 2011 Run Hello. All rights reserved.
 *
 */

#include "bridge.h"
#include "terminal.h"
//#include "dos.h"
#include "physfs.h"
#include "tinyxml.h"
#include <sstream>

extern "C" {	
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

// Small lua-interface utilities

// Pull out a LUSERDATA pointer from the table at the given idx in the field with the given name.
static void *pointerFieldFromObject(lua_State *L, int idx, const char *name) {
	lua_getfield (L, idx, name);
	void *result = (void *)lua_topointer(L, -1);
	lua_pop(L, 1);
	return result;
}

// Pull out the __ptr field of the table at the given idx.
static void *ptrFromObject(lua_State *L, int idx) {
	return pointerFieldFromObject(L, idx, "__ptr");
}

// Methods intended to be called from Lua

Screen *project_bridge::room_screen() {
	return room_auto::singleton()->screen;
}

Scene *project_bridge::room_scene() {
	return room_auto::singleton()->scene;
}

StringArray *project_bridge::room_objs() {
	room_auto *room = room_auto::singleton();
	StringArray *result = new StringArray();
	for(obj_iter i = room->obj.begin(); i != room->obj.end(); i++)
		result->contents.push_back(i->first);
	return result;
}

StringArray *project_bridge::room_onclick_objs() {
	room_auto *room = room_auto::singleton();
	StringArray *result = new StringArray();
	for(handler_iter i = room->onClick.begin(); i != room->onClick.end(); i++)
		result->contents.push_back(room->obj_name[i->first]);
	return result;
}

StringArray *project_bridge::room_oncollide_objs() {
	room_auto *room = room_auto::singleton();
	StringArray *result = new StringArray();
	for(handler_iter i = room->onCollide.begin(); i != room->onCollide.end(); i++)
		result->contents.push_back(room->obj_name[i->first]);
	return result;
}

Entity *project_bridge::room_id(String _at) {
	string at = _at.getSTLString();
	room_auto *room = room_auto::singleton();
	if (room->obj.count(at))
		return room->obj[at];
	return NULL;
}

String project_bridge::room_name(SceneEntity *of) {
	room_auto *room = room_auto::singleton();
	if (room->obj_name.count(of))
		return String(room->obj_name[of]);
	return String();
}

Entity *project_bridge::room_a() {
	return room_auto::singleton()->a;
}
Entity *project_bridge::room_b() {
	return room_auto::singleton()->b;
}

void project_bridge::room_remove_scene(SceneEntity *obj) {
	room_auto *room = room_auto::singleton();
	if (room->obj_name.count(obj)) {
		const string &name = room->obj_name[obj];
		if (room->obj.count(name))
			room->obj.erase(name);
		room->obj_name.erase(obj); // "name" now invalid
		
		// Wait, is this in the Screen or the Scene? Eh, I guess it doesn't matter.
		if (room->scene)
			room->scene->removeEntity(obj);
	}
}

void project_bridge::room_remove_screen(ScreenEntity *obj, bool doRemove) {
	room_auto *room = room_auto::singleton();
	if (room->obj_name.count(obj)) {
		const string &name = room->obj_name[obj];
		if (room->obj.count(name))
			room->obj.erase(name);
		room->obj_name.erase(obj); // "name" now invalid
		room->onClick.erase(obj); // as are handlers
		room->onCollide.erase(obj);
		// Wait, is this in the Screen or the Scene? Eh, I guess it doesn't matter.
		if (doRemove && room->screen)
			room->screen->removeChild(obj);
	}
}

void project_bridge::load_room(String from) {
	room_auto *room = room_auto::singleton();
	clear();
	if (room)
		room->die();
	(new room_auto(from.getSTLString()))->insert();
}

void project_bridge::load_room_txt(String from) {
	this->load_room(filedump(from));
}

Screen *project_bridge::standalone_screen(String svg, String objectPath, bool isPhysics) {
	Object overlay;
	room_auto *room = room_auto::singleton();
	loadFromTree(overlay, objectPath.getSTLString());
	if (objectPath.size()) {
		loadFromTree(overlay, objectPath.getSTLString());
	}
#if PHYSICS2D
	string loadClass = isPhysics ? "PhysicsScreen" : "Screen";
	Screen *into = isPhysics ? new PhysicsScreen() : new Screen();
#else
	string loadClass = "Screen";
	Screen *into = new Screen();
#endif
	
	room->loadSvgFromFile(overlay, svg.getSTLString(), into, loadClass, false);
	return into;
}

// Expectation is screen will be deleted right after
void project_bridge::room_remove_standalone_screen_all(Screen *s) {
	for(int c = 0; c < s->getNumChildren(); c++) {
		room_remove_screen( s->getChild(c), false );
	}
}

void project_bridge::clear() {
	terminal_auto::singleton()->reset();
}

ScreenMesh *project_bridge::meshFor(Polycode::Polygon *p) { // Lua bridge doesn't work with ScreenMesh, so...
	bool triangle = (p->getVertexCount() == 3);
	ScreenMesh *s = (triangle ? new ScreenMesh(Mesh::TRI_MESH) : new ScreenMesh(Mesh::TRIFAN_MESH));
	Mesh *m = s->getMesh();
	m->addPolygon(p);
	return s;
}

// This doesn't actually do anything
String project_bridge::saved_level() {
	return save.file;
}
void project_bridge::set_saved_level(int priority) {
	if (save.priority < priority) {
		save.priority = priority;
		save.file = room_auto::singleton()->spec;
		save.save();
	}
}

string help_attempt(const string &path) {
	return ::filedump(path);
}

String project_bridge::filedump(String _path) {
	return ::filedump(_path.getSTLString());
}

String project_bridge::filedump_external(String _path) {
	return ::filedump_external(_path.getSTLString());
}

String project_bridge::help(String _path) {
	string path = "media/help/" + _path.getSTLString();
	string sep = _path.size() ? "/" : "";
	for(int c = 0; c < path.size(); c++) {
		char &s = path[c];
		if (s == '.' || s == '\\' || s == ':')
			s = '/';
		if (s == '(' || s == ')')
			s = '\0';
	}
	string result;
	result = help_attempt(path + ".txt");
	if (result.empty()) result = help_attempt(path + sep + "index.txt");
	if (result.empty()) result = "No help found.";
	return String(result); // Not very efficient, but we don't need efficient.
}

void project_bridge::fake() {
	room_auto::singleton()->rebirth(true);
}

void project_bridge::Quit() {
	::Quit();
}

Matrix4 project_bridge::mmult(Matrix4 a, Matrix4 b) {
	return a * b;
}

Quaternion project_bridge::qmult(Quaternion a, Quaternion b) {
	return a * b;
}

Quaternion project_bridge::Slerp(Number fT, const Quaternion& rkP, const Quaternion& rkQ, bool shortestPath) {
	return Quaternion::Slerp(fT,rkP,rkQ,shortestPath);
}

Quaternion project_bridge::Squad(Number fT, const Quaternion& rkP, const Quaternion& rkA, const Quaternion& rkB, const Quaternion& rkQ, bool shortestPath) {
	return Quaternion::Squad(fT,rkP,rkA,rkB,rkQ,shortestPath);
}

Vector3 project_bridge::bBox(Entity *e) {
	return e->bBox;
}

void project_bridge::setSceneClearColor(Scene *scene, int r, int g, int b, int a) {
	scene->useClearColor = true;
	scene->clearColor = Color(r,g,b,a);
}

String project_bridge::custEntityType(Entity *obj) {
	return obj->custEntityType;
}
/*
type_automaton *project_bridge::col40() {
	return type_automaton::singleton();
}
*/
String project_bridge::charCode(InputEvent *e) {
	wchar_t s[2] = {e->charCode, 0};
	return String(s);
}

Sound *project_bridge::soundFromValues(NumberArray *values, int channels, int freq, int bps) {
	int s = values->contents.size();
	uint16_t *data = new uint16_t[s];
	for(int c = 0; c < s; c++) {
		data[c] = values->contents[c] * SHRT_MAX;
	}
	Sound *sound = new Sound((char *)data, s*sizeof(uint16_t), channels, freq, bps);
	delete[] data;
	return sound;
}

// Only do playback in debug mode-- otherwise there's no console!
#if _DEBUG
#include "playtest.h"

static playback_loader *common_playback_loader() {
	static playback_loader *__playback_loader = NULL;
	if (!__playback_loader)
		__playback_loader = new playback_loader();
	return __playback_loader;
}
#endif

void project_bridge::playback_index() { // Only useful in debug mode
#if _DEBUG
	string s = common_playback_loader()->index();
	ERR("%s\n", s.c_str());
#endif
}
void project_bridge::playback_from(int idx) {
#if _DEBUG
	load_room(::filedump("media/init.txt")); // Restart from true beginning
	common_playback_loader()->load_from(idx);
#endif
}

int project_bridge::luaTestAddOne(lua_State *L) {
	int got = lua_tointeger(L, 2);
	lua_pop(L,1);
	ERR("HERE GOT %d\n", got);
	lua_pushinteger(L, got+1);
	return 1;
}

static void pop_objectEntry(lua_State *L, ObjectEntry *o) {
	int ty = lua_type (L, -1);
	switch (ty) {
		case LUA_TNIL: case LUA_TLIGHTUSERDATA: case LUA_TFUNCTION: case LUA_TUSERDATA: case LUA_TTHREAD: default: 
			// TODO: WTF to do here? Let's just make an empty container.
			o->type = ObjectEntry::CONTAINER_ENTRY;
			break;
		case LUA_TBOOLEAN:
			o->type = ObjectEntry::BOOL_ENTRY;
			o->boolVal = lua_toboolean(L,-1);
			break;
		case LUA_TNUMBER:
			o->type = ObjectEntry::FLOAT_ENTRY; // TODO: Or int?
			o->NumberVal = lua_tonumber(L,-1);
			break;
		case LUA_TSTRING:
			o->type = ObjectEntry::STRING_ENTRY;
			o->stringVal = lua_tostring(L,-1);
			break;
		case LUA_TTABLE: {
			int t = lua_gettop(L);
			o->type = ObjectEntry::CONTAINER_ENTRY;
			
			lua_pushnil(L);  // first key
			while (lua_next(L, t) != 0) {
				// uses 'key' (at index -2) and 'value' (at index -1)
				int key_type = lua_type (L, -2);
				
				ObjectEntry *child = new ObjectEntry();
				if (key_type == LUA_TSTRING) {
					child->name = lua_tostring(L,-2);
				} else {
					// Parent has integer indices and therefore must be treated as array.
					o->type = ObjectEntry::ARRAY_ENTRY;
				}
				pop_objectEntry(L, child); // Will pop value, leave key for next call
				o->children.push_back(child);
				o->length++;
			}
		} break;
	}
	
	lua_pop(L, 1); // Pop item
}

static void push_objectentry(lua_State *L, ObjectEntry *o) {
//	ERR("%p has name %s type %d value %s\n", o, o->name.c_str(), (int)o->type, o->stringVal.c_str());
	switch(o->type) {
		case ObjectEntry::UNKNOWN_ENTRY:
			lua_pushnil(L);
			break;
		case ObjectEntry::FLOAT_ENTRY:
			lua_pushnumber(L, o->NumberVal);
			break;
		case ObjectEntry::INT_ENTRY:
			lua_pushinteger(L, o->intVal);
			break;
		case ObjectEntry::BOOL_ENTRY:
			lua_pushboolean(L, o->boolVal);
			break;
		case ObjectEntry::STRING_ENTRY:
			lua_pushstring(L, o->stringVal.c_str());
			break;
		case ObjectEntry::ARRAY_ENTRY:
			lua_newtable(L);
			for(int c = 0; c < o->length; c++) { // Makes more sense to do "size"?
				lua_pushinteger(L, c+1);
				push_objectentry(L, o->children[c]);
				lua_settable(L, -3);
			}
			break;
		case ObjectEntry::CONTAINER_ENTRY:
			lua_newtable(L);
			for(int c = 0; c < o->children.size(); c++) {
				ObjectEntry *n = o->children[c];
				lua_pushstring(L, n->name.c_str());
				push_objectentry(L, n);
				lua_settable(L, -3);
			}
			break;
	}
}

// Arguments: ObjectEntry to save into, lua item to save 
// Returns: None
int project_bridge::saveTableIntoObject(lua_State *L) {
	luaL_checktype(L, 2, LUA_TTABLE);
	ObjectEntry *o = (ObjectEntry*)ptrFromObject(L, 2);
	
	o->Clear();
	pop_objectEntry(L, o);
	
	return 0;
}

// Arguments: ObjectEntry to load from
// Returns: Lua equivalent
int project_bridge::loadTableFromObject(lua_State *L) {
	luaL_checktype(L, 2, LUA_TTABLE);
	ObjectEntry *o = (ObjectEntry*)ptrFromObject(L, 2);
	
	push_objectentry(L, o);
	
	return 1;
}

// Arguments: File path to save into, root name, lua table to save
// Returns: None
int project_bridge::saveTableIntoFile(lua_State *L) {
	luaL_checktype(L, 2, LUA_TSTRING);
	luaL_checktype(L, 3, LUA_TSTRING);
	luaL_checktype(L, -1, LUA_TTABLE);
	
	// TODO: Check for empty filename, yield
	string filename = PROGDIR; filename += lua_tostring(L, 2);
	Object o;
	o.root.name = lua_tostring(L, 3);
	
	pop_objectEntry(L, &o.root);
	o.saveToXML(filename);
	
	return 0;
}

static void pcall(lua_State *L, int nargs, int nresults) {
	if(lua_pcall(L, nargs, nresults, 0) != 0) {
		const char *msg = lua_tostring(L, -1);
		lua_getfield(L, LUA_GLOBALSINDEX, "__customError");
		lua_pushstring(L, msg);
		lua_call(L, 1, 0);
	}	
}

// Arguments: File path to load from, "external path?" bool
// Returns: Lua equivalent
int project_bridge::loadTableFromFile(lua_State *L) {
	luaL_checktype(L, 2, LUA_TSTRING);
//	luaL_checktype(L, 3, LUA_TBOOLEAN); // Don't bother, we can "cast to boolean"
	string filename;
	Object o;
	
	if (lua_toboolean(L, 3)) { // Check directory with exe
		filename = PROGDIR;
		filename += lua_tostring(L, 2);
		o.loadFromXMLString(::filedump_external(filename));
	} else { // Check PhysFS
		filename += lua_tostring(L, 2);
		o.loadFromXML(filename);
	}
	
	push_objectentry(L, &o.root);
	
	return 1;
}

// Arguments: root name, lua table to save
// Returns: XML equivalent
int project_bridge::saveTableIntoXml(lua_State *L) {
	luaL_checktype(L, 2, LUA_TSTRING);
	luaL_checktype(L, -1, LUA_TTABLE);
	Object o;
	o.root.name = lua_tostring(L, 2);
	
	pop_objectEntry(L, &o.root);
	
	// Nothing for this in Object, so I'll build it myself.
	TiXmlDocument doc;  	
	TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "", "" );  
	doc.LinkEndChild( decl ); 
	
	TiXmlElement * rootElement = o.createElementFromObjectEntry(&o.root);
	doc.LinkEndChild(rootElement);
	
	TiXmlPrinter printer;
	printer.SetStreamPrinting();
	doc.Accept(&printer);
	lua_pushstring(L, printer.CStr());
	
	return 1;
}

// Arguments: XML string
// Returns: Lua equivalent
int project_bridge::loadTableFromXml(lua_State *L) {
	luaL_checktype(L, 2, LUA_TSTRING);
	string content = lua_tostring(L, 2);
	Object o;
	
	o.loadFromXMLString(content);
	push_objectentry(L, &o.root);
	
	return 1;
}

// Arguments: Screen, x coordinate, y coordinate
int project_bridge::getChildAtScreenPosition(lua_State *L) {
	luaL_checktype(L, 2, LUA_TTABLE);
	luaL_checktype(L, 3, LUA_TNUMBER);
	luaL_checktype(L, 4, LUA_TNUMBER);

	Screen *s = (Screen *)ptrFromObject(L, 2);
	Number x = lua_tonumber(L,3);
	Number y = lua_tonumber(L,4);
	
	lua_newtable(L);
	int target = lua_gettop(L);
	
	int pushi = 1;
	for(int i=0; i < s->getNumChildren(); i++) {
		ScreenEntity *child = s->getChild(i);
		if(child->hitTest(x,y)) {
			lua_getfield(L, LUA_GLOBALSINDEX, "__ptrToTable");
			lua_pushstring(L, "ScreenEntity");
			lua_pushlightuserdata(L, child);
			pcall(L,2,1); // FIXME: pcall or raw call? Or just yield? We're technically inside a lua call at this point.
			
			lua_rawseti(L, target, pushi);
			pushi++;
		}
	}	
	
	return 1;
}

bool project_bridge::getVisible(Entity *e) {
	return e->visible;
}

void project_bridge::setVisible(Entity *e, bool visible) {
	e->visible = visible;
}

CoreServices *project_bridge::coreServices() {
	return CoreServices::getInstance();
}

SceneEcho::SceneEcho(Entity *_copying) : SceneEntity(), copying(_copying) {
	bBox.x = 1;
	bBox.y = 1;
	bBox.z = 1;		
}

void SceneEcho::transformAndRender() {
	Entity *oldCopyParent = copying->getParentEntity();
	if (oldCopyParent)
		oldCopyParent->removeChild(copying);
	this->addChild(copying);
	Entity::transformAndRender();
	this->removeChild(copying);
	if (oldCopyParent)
		oldCopyParent->addChild(copying);
}

Entity *SceneEcho::getEntity() { return copying; }

string fromMatrix4(const Matrix4 &mat) {
    ostringstream o;
    for(int y = 0; y < 4; y++) {
        o << (0==y?"[":" ") << "[";
        for(int x = 0; x < 4; x++) {
            o << mat.m[y][x] << (3==x?"":",\t");
        }
        o << "]" << (3==y?"]":"\n");
    }
    return o.str();
}